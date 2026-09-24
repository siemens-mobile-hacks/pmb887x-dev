#!/usr/bin/env python3

import argparse
import json
import os
import struct
import sys
import time
import zlib

try:
	import serial
	from serial.tools import list_ports
except ImportError:
	sys.exit("pyserial is not installed, try: pip install pyserial")


BOOT_SPEED = 115200
DEFAULT_SPEED = 1625000
EXTRAM_ADDRESS = 0xB0000000
EXTRAM_SIZE = 0x02000000
SRAM_START = 0x00080000
SRAM_END = 0x00098000
LZ4_BLOCK_SIZE = 0x00001000
PRELOADER_HEADER_SIZE = 32
PRELOADER_BOARD_NAME_SIZE = 20
BOOT_MAGIC_OFFSET = 0x3C
SIEMENS_BOOT_MARKER_OFFSET = 0x28
PLATFORMS = {0: "siemens", 1: "apoxi"}
CPUS = {0xB0: "pmb8875", 0xC0: "pmb8876"}

CMD_PING = 0x41
CMD_INIT_EXTRAM = 0x45
CMD_GOTO = 0x47
CMD_SET_SPEED = 0x48
CMD_WRITE_RAM_LZ4 = 0x4C
CMD_RAM_WRITE = 0x57

STATUS_PONG = 0x52
STATUS_READY = 0xA5
STATUS_SUCCESS = 0xC1

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PRELOADER_DIR = os.path.join(SCRIPT_DIR, "boot")

KNOWN_ADAPTERS = {
	(0x1A86, 0x5523): ("QinHeng CH341", 100),
	(0x1A86, 0x7522): ("QinHeng CH340", 100),
	(0x1A86, 0x7523): ("QinHeng CH340", 100),
	(0x1A86, 0x7584): ("QinHeng CH340S", 100),
	(0x0403, 0x6001): ("FTDI FT232", 100),
	(0x0403, 0x6010): ("FTDI FT2232", 100),
	(0x0403, 0x6011): ("FTDI FT4232", 100),
	(0x0403, 0x6014): ("FTDI FT232H", 100),
	(0x0403, 0x6015): ("FTDI bridge", 100),
	(0x10C4, 0xEA60): ("Silicon Labs CP210x", 100),
	(0x10C4, 0xEA61): ("Silicon Labs CP210x", 100),
	(0x10C4, 0xEA63): ("Silicon Labs CP210x", 100),
	(0x10C4, 0xEA70): ("Silicon Labs CP2105", 100),
	(0x10C4, 0xEA71): ("Silicon Labs CP2108", 100),
	(0x067B, 0x2303): ("Prolific PL2303", 90),
}


class ProtocolError(Exception):
	pass


def describe_port(info):
	adapter = KNOWN_ADAPTERS.get((info.vid, info.pid))
	parts = ["%04X:%04X" % (info.vid, info.pid), adapter[0]]
	if info.serial_number:
		parts.append("sn=%s" % info.serial_number)
	return ", ".join(parts)


def stable_device_path(device):
	by_id = "/dev/serial/by-id"
	if not os.path.isdir(by_id):
		return device

	target = os.path.realpath(device)
	for name in sorted(os.listdir(by_id)):
		path = os.path.join(by_id, name)
		if os.path.realpath(path) == target:
			return path
	return device


def find_serial_ports():
	ports = []
	for info in list_ports.comports():
		adapter = KNOWN_ADAPTERS.get((info.vid, info.pid))
		if adapter is None:
			continue
		ports.append((adapter[1], stable_device_path(info.device), info))

	ports.sort(key=lambda item: (-item[0], item[1]))
	return ports


def autodetect_device():
	ports = find_serial_ports()
	if not ports:
		raise ProtocolError("no UART adapter found; specify --device")

	best_score = ports[0][0]
	best = [port for port in ports if port[0] == best_score]
	if len(best) > 1:
		lines = ["multiple UART adapters found; specify --device:"]
		for _, path, info in best:
			lines.append("\t%s (%s)" % (path, describe_port(info)))
		raise ProtocolError("\n".join(lines))

	_, path, info = best[0]
	print("Using UART adapter: %s (%s)" % (path, describe_port(info)), file=sys.stderr)
	return path


def read_byte(port, description):
	data = port.read(1)
	if not data:
		raise ProtocolError("timeout waiting for %s" % description)
	return data[0]


def expect_byte(port, expected, description):
	value = read_byte(port, description)
	if value != expected:
		raise ProtocolError("invalid %s: 0x%02X, expected 0x%02X" % (description, value, expected))


def wait_for_bootrom(port, ignition):
	print("Waiting for BootROM...", file=sys.stderr)
	dtr = False
	pulses = 0
	started = time.monotonic()
	next_toggle = started

	while True:
		port.write(b"AT")
		response = port.read(1)
		cpu = CPUS.get(response[0] & 0xF0) if response else None
		if cpu is None:
			now = time.monotonic()
			if ignition and now >= next_toggle:
				dtr = not dtr
				port.dtr = dtr
				if dtr:
					pulses += 1
				next_toggle = now + (1.0 if dtr else 0.5)
			continue

		if ignition:
			port.dtr = False
		if port.read(1) == b"\xC4":
			read_byte(port, "secure boot chip ID")

		print("CPU detected: %s (ignition pulses: %d, %.0f ms)" %
			(cpu.upper(), pulses, (time.monotonic() - started) * 1000), file=sys.stderr)
		return cpu


def read_preloader_header(data):
	if data[BOOT_MAGIC_OFFSET:BOOT_MAGIC_OFFSET + 4] == b"CJKT":
		offset = 64
	elif data[SIEMENS_BOOT_MARKER_OFFSET:SIEMENS_BOOT_MARKER_OFFSET + 16] == b"SIEMENS_BOOTCODE":
		offset = 96
	else:
		offset = 4

	magic = data[offset:offset + 7]
	if magic not in (b"PMB8875", b"PMB8876"):
		return None
	if len(data) < offset + PRELOADER_HEADER_SIZE:
		raise ProtocolError("truncated PMB887X header at offset %d" % offset)

	cpu = magic.decode("ascii").lower()
	platform_id = data[offset + 7]
	if platform_id not in PLATFORMS:
		raise ProtocolError("invalid PMB887X header platform: %d" % platform_id)
	platform = PLATFORMS[platform_id]
	load_address = struct.unpack_from("<I", data, offset + 8)[0]
	name_data = data[offset + 12:offset + 12 + PRELOADER_BOARD_NAME_SIZE]
	board = name_data.split(b"\0", 1)[0].decode("ascii", errors="replace")

	return cpu, platform, board, offset, load_address


def select_preloader(cpu, platform):
	path = os.path.join(PRELOADER_DIR, "preloader-%s-%s.bin" % (platform, cpu))
	data = read_file(path)
	header = read_preloader_header(data)
	if header is None:
		raise ProtocolError("PMB887X header not found in preloader %s" % path)
	if header[:2] != (cpu, platform):
		raise ProtocolError("preloader %s does not match the connected phone" % path)
	return data


def load_preloader(port, data):
	if len(data) > 0xFFFF:
		raise ProtocolError("preloader is too large for BSL")

	checksum = 0
	for value in data:
		checksum ^= value

	port.write(b"\x30" + struct.pack("<H", len(data)))
	port.write(data)
	port.write(bytes([checksum]))
	status = read_byte(port, "BSL status")
	if status not in (0x01, 0xB1, 0xC1):
		raise ProtocolError("invalid BSL status: 0x%02X" % status)
	expect_byte(port, STATUS_READY, "preloader ready status")


def preloader_ping(port):
	port.write(bytes([CMD_PING]))
	expect_byte(port, STATUS_PONG, "ping response")


def preloader_set_speed(port, speed):
	if speed == port.baudrate:
		preloader_ping(port)
		return

	port.write(bytes([CMD_SET_SPEED]) + struct.pack("<I", speed))
	port.flush()
	port.baudrate = speed
	preloader_ping(port)


def preloader_init_extram(port):
	request = bytes([CMD_INIT_EXTRAM]) + struct.pack("<II", EXTRAM_ADDRESS, EXTRAM_SIZE)
	port.write(request)
	expect_byte(port, STATUS_SUCCESS, "EXTRAM initialization status")


def preloader_write(port, address, data):
	crc = zlib.crc32(data) & 0xFFFFFFFF
	request = bytes([CMD_RAM_WRITE]) + struct.pack("<II", address, len(data))
	port.write(request)
	port.write(data)
	port.write(struct.pack("<I", crc))
	expect_byte(port, STATUS_SUCCESS, "RAM write status")


def append_lz4_length(output, length):
	while length >= 0xFF:
		output.append(0xFF)
		length -= 0xFF
	output.append(length)


def lz4_compress_block(data):
	output = bytearray()
	positions = {}
	anchor = 0
	offset = 0
	search_limit = len(data) - 12
	match_limit = len(data) - 5

	while offset <= search_limit:
		sequence = struct.unpack_from("<I", data, offset)[0]
		match = positions.get(sequence)
		positions[sequence] = offset
		if match is None or offset - match > 0xFFFF:
			offset += 1
			continue

		match_end = offset + 4
		while match_end < match_limit and data[match + match_end - offset] == data[match_end]:
			match_end += 1

		literal_size = offset - anchor
		match_size = match_end - offset - 4
		output.append((min(literal_size, 0x0F) << 4) | min(match_size, 0x0F))
		if literal_size >= 0x0F:
			append_lz4_length(output, literal_size - 0x0F)
		output.extend(data[anchor:offset])
		output.extend(struct.pack("<H", offset - match))
		if match_size >= 0x0F:
			append_lz4_length(output, match_size - 0x0F)

		previous_offset = offset
		offset = match_end
		anchor = offset
		for position in range(previous_offset + 1, min(offset, search_limit + 1)):
			sequence = struct.unpack_from("<I", data, position)[0]
			positions[sequence] = position

	literal_size = len(data) - anchor
	output.append(min(literal_size, 0x0F) << 4)
	if literal_size >= 0x0F:
		append_lz4_length(output, literal_size - 0x0F)
	output.extend(data[anchor:])
	return bytes(output)


def preloader_write_lz4(port, address, data, compressed):
	crc = zlib.crc32(data) & 0xFFFFFFFF
	request = bytes([CMD_WRITE_RAM_LZ4]) + struct.pack("<III", address, len(compressed), len(data))
	port.write(request)
	port.write(compressed)
	port.write(struct.pack("<I", crc))
	expect_byte(port, STATUS_SUCCESS, "LZ4 write status")


def preloader_write_payload(port, address, data):
	for offset in range(0, len(data), LZ4_BLOCK_SIZE):
		block = data[offset:offset + LZ4_BLOCK_SIZE]
		compressed = lz4_compress_block(block)
		if len(compressed) < len(block):
			preloader_write_lz4(port, address + offset, block, compressed)
		else:
			preloader_write(port, address + offset, block)


def preloader_goto(port, address):
	port.write(bytes([CMD_GOTO]) + struct.pack("<I", address))
	expect_byte(port, STATUS_SUCCESS, "GOTO status")


def parse_host_command(line):
	prefix = b"# HOST-CMD: "
	if not line.startswith(prefix):
		return None

	command, _, argument = line[len(prefix):].partition(b" ")
	return command, argument


def send_host_data(port, data, command):
	start = time.monotonic()
	written = port.write(data)
	port.flush()
	if written != len(data):
		raise ProtocolError("HOST-CMD %s wrote only %d of %d bytes" % (command, written, len(data)))
	parity_bits = 0 if port.parity == serial.PARITY_NONE else 1
	frame_bits = 1 + port.bytesize + parity_bits + port.stopbits
	remaining = len(data) * frame_bits / port.baudrate - (time.monotonic() - start)
	if remaining > 0:
		time.sleep(remaining)


def generate_xorshift32(length, seed):
	data = bytearray(length)
	state = seed
	for offset in range(length):
		state ^= (state << 13) & 0xFFFFFFFF
		state ^= state >> 17
		state ^= (state << 5) & 0xFFFFFFFF
		state &= 0xFFFFFFFF
		data[offset] = state & 0xFF
	return data


def execute_host_command(port, command, argument, restore_configuration=None):
	if command == b"BAUDRATE":
		if not argument:
			raise ProtocolError("HOST-CMD BAUDRATE has no argument")
		baud_rate = int(argument)
		if baud_rate <= 0:
			raise ProtocolError("HOST-CMD BAUDRATE must be positive")
		port.baudrate = baud_rate
	elif command == b"FORMAT":
		formats = {
			b"7E1": (serial.SEVENBITS, serial.PARITY_EVEN, serial.STOPBITS_ONE),
			b"7O1": (serial.SEVENBITS, serial.PARITY_ODD, serial.STOPBITS_ONE),
			b"8N1": (serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE),
			b"8E1": (serial.EIGHTBITS, serial.PARITY_EVEN, serial.STOPBITS_ONE),
			b"8O1": (serial.EIGHTBITS, serial.PARITY_ODD, serial.STOPBITS_ONE),
		}
		if argument not in formats:
			raise ProtocolError("HOST-CMD FORMAT must be 7E1, 7O1, 8N1, 8E1, or 8O1")
		port.bytesize, port.parity, port.stopbits = formats[argument]
	elif command == b"SEND":
		if not argument:
			raise ProtocolError("HOST-CMD SEND has no argument")
		data = json.loads(argument)
		if not isinstance(data, str):
			raise ProtocolError("HOST-CMD SEND argument must be a string")
		send_host_data(port, data.encode("utf-8"), "SEND")
	elif command == b"SEND_XORSHIFT32":
		fields = argument.split()
		if len(fields) != 3:
			raise ProtocolError("HOST-CMD SEND_XORSHIFT32 requires length, seed, and CRC32")
		length, seed, expected_crc = (int(field, 0) for field in fields)
		if length <= 0 or length > 1024 * 1024:
			raise ProtocolError("HOST-CMD SEND_XORSHIFT32 length must be between 1 and 1048576")
		if seed <= 0 or seed > 0xFFFFFFFF:
			raise ProtocolError("HOST-CMD SEND_XORSHIFT32 seed must be a nonzero 32-bit value")
		if expected_crc < 0 or expected_crc > 0xFFFFFFFF:
			raise ProtocolError("HOST-CMD SEND_XORSHIFT32 CRC32 must be a 32-bit value")
		data = generate_xorshift32(length, seed)
		actual_crc = zlib.crc32(data)
		if actual_crc != expected_crc:
			raise ProtocolError(
				"HOST-CMD SEND_XORSHIFT32 CRC32 is 0x%08X, expected 0x%08X" %
				(actual_crc, expected_crc)
			)
		send_host_data(port, data, "SEND_XORSHIFT32")
	elif command == b"WAIT":
		if not argument:
			raise ProtocolError("HOST-CMD WAIT has no argument")
		delay_ms = int(argument)
		if delay_ms < 0:
			raise ProtocolError("HOST-CMD WAIT must not be negative")
		time.sleep(delay_ms / 1000)
	elif command == b"RESTORE":
		if argument or restore_configuration is None:
			raise ProtocolError("invalid HOST-CMD RESTORE")
		port.baudrate, port.bytesize, port.parity, port.stopbits = restore_configuration
	else:
		raise ProtocolError("unknown HOST-CMD: %s" % command.decode("ascii", "replace"))


def forward_output(port, output):
	pending = b""
	command_block = None
	while True:
		data = port.read(port.in_waiting or 1)
		if not data:
			continue

		lines = (pending + data).split(b"\n")
		pending = lines.pop()
		for line in lines:
			parsed = parse_host_command(line)
			if parsed is None:
				continue
			command, argument = parsed
			if command == b"BEGIN":
				if argument or command_block is not None:
					raise ProtocolError("invalid HOST-CMD BEGIN")
				command_block = {
					"commands": [],
					"configuration": (port.baudrate, port.bytesize, port.parity, port.stopbits),
				}
			elif command == b"END":
				if argument or command_block is None:
					raise ProtocolError("invalid HOST-CMD END")
				for queued_command, queued_argument in command_block["commands"]:
					execute_host_command(
						port,
						queued_command,
						queued_argument,
						command_block["configuration"],
					)
				command_block = None
			elif command_block is None:
				execute_host_command(port, command, argument)
			else:
				command_block["commands"].append((command, argument))

		terminator = data.find(b"\0")
		if terminator >= 0:
			if command_block is not None:
				raise ProtocolError("unfinished HOST-CMD block")
			output.write(data[:terminator])
			output.flush()
			return

		output.write(data)
		output.flush()


def parse_address(value):
	try:
		address = int(value, 0)
	except ValueError as error:
		raise argparse.ArgumentTypeError("invalid address: %s" % value) from error
	if address < 0 or address > 0xFFFFFFFF:
		raise argparse.ArgumentTypeError("address outside 32-bit range: %s" % value)
	return address


def parse_args():
	parser = argparse.ArgumentParser(description="Load and run a PMB887X binary.")
	parser.add_argument("payload", help="binary to execute")
	parser.add_argument("--platform", choices=PLATFORMS.values(),
		help="platform")
	parser.add_argument("-d", "--device", metavar="/dev/ttyUSB0",
		help="serial port (default: auto)")
	parser.add_argument("-s", "--speed", type=int, default=DEFAULT_SPEED,
		help="baud rate (default: %(default)s)")
	parser.add_argument("--write-addr", type=parse_address, metavar="ADDR",
		help="write address (default: from header)")
	parser.add_argument("--exec-addr", type=parse_address, metavar="ADDR",
		help="entry address (default: from header)")
	parser.add_argument("--no-ign", action="store_true", help="disable DTR ignition")
	return parser.parse_args()


def read_file(path):
	with open(path, "rb") as file:
		return file.read()


def open_port(device, speed):
	port = serial.Serial()
	port.port = device
	port.baudrate = speed
	port.timeout = 0.005
	port.dtr = False
	port.rts = False
	port.open()
	return port


def main():
	args = parse_args()
	payload = read_file(args.payload)

	if not payload:
		raise ProtocolError("payload is empty")

	payload_header = read_preloader_header(payload)
	if payload_header is None:
		if args.platform is None:
			raise ProtocolError("payload has no PMB887X header; specify --platform")
		if args.write_addr is None or args.exec_addr is None:
			raise ProtocolError("payload has no PMB887X header; specify --write-addr and --exec-addr")
		platform = args.platform
		payload_board = None
		write_address = args.write_addr
		exec_address = args.exec_addr
	else:
		payload_cpu, platform, payload_board, header_offset, header_address = payload_header
		if args.platform is not None and args.platform != platform:
			raise ProtocolError("--platform does not match the PMB887X header")
		write_address = args.write_addr if args.write_addr is not None else header_address
		header_exec_address = header_address + PRELOADER_HEADER_SIZE if header_offset == 0 else header_address
		exec_address = args.exec_addr if args.exec_addr is not None else header_exec_address

	if SRAM_START <= write_address < SRAM_END:
		memory_end = SRAM_END
		init_extram = False
	elif EXTRAM_ADDRESS <= write_address < EXTRAM_ADDRESS + EXTRAM_SIZE:
		memory_end = EXTRAM_ADDRESS + EXTRAM_SIZE
		init_extram = True
	else:
		raise ProtocolError("unsupported payload write address: 0x%08X" % write_address)
	if len(payload) > memory_end - write_address:
		raise ProtocolError("payload does not fit at 0x%08X" % write_address)

	device = autodetect_device() if args.device is None or args.device == "auto" else args.device
	port = open_port(device, BOOT_SPEED)
	try:
		port.reset_input_buffer()
		cpu = wait_for_bootrom(port, not args.no_ign)
		port.timeout = 2
		if payload_header is not None and cpu != payload_cpu:
			raise ProtocolError("payload CPU does not match the connected phone")
		preloader = select_preloader(cpu, platform)

		load_preloader(port, preloader)
		preloader_set_speed(port, args.speed)
		print("Preloader ready at %d baud" % args.speed, file=sys.stderr)

		if init_extram:
			preloader_init_extram(port)

		if payload_board is None:
			print("Loading payload: %s -> 0x%08X (%d bytes)" %
				(args.payload, write_address, len(payload)), file=sys.stderr)
		else:
			print("Loading payload: %s [%s] -> 0x%08X (%d bytes)" %
				(args.payload, payload_board, write_address, len(payload)), file=sys.stderr)
		preloader_write_payload(port, write_address, payload)
		print("<------- cut here -------", file=sys.stderr)
		port.timeout = 1
		preloader_goto(port, exec_address)
		forward_output(port, sys.stdout.buffer)
	finally:
		port.close()

	return 0


if __name__ == "__main__":
	try:
		sys.exit(main())
	except (OSError, ProtocolError, ValueError, serial.SerialException) as error:
		sys.exit(str(error))
