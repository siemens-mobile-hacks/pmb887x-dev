#!/usr/bin/env python3
"""Chaos/PV bootloader client for PMB887x (S-Gold) phones.

Port of chaos-boot.pl with the same command line interface, plus UART
adapter autodetection and an optional (gitignored) settings file.
"""

import argparse
import configparser
import os
import select
import struct
import subprocess
import sys
import time
from typing import Any, Callable

try:
	import serial
	from serial.tools import list_ports
except ImportError:
	sys.exit("pyserial is not installed, try: pip install pyserial")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CONFIG = os.path.join(SCRIPT_DIR, "chaos-boot.ini")
CONFIG_SECTION = "chaos-boot"

CHAOS_SPEEDS = {
	57600: 0x00,
	115200: 0x01,
	230400: 0x02,
	460800: 0x03,
	614400: 0x04,
	921600: 0x05,
	1228800: 0x06,
	1600000: 0x07,
	1500000: 0x08,
	1625000: 0x07,
	3250000: 0x09,
}

# Known USB-UART bridges, ranked: the highest scored device wins autodetection.
KNOWN_ADAPTERS = {
	0x1A86: ("QinHeng CH34x", 100),
	0x0403: ("FTDI", 100),
	0x10C4: ("Silicon Labs CP210x", 100),
	0x067B: ("Prolific PL2303", 90),
	0x1A61: ("Siemens DCA-540", 90),
	0x11F5: ("Siemens DCA-500", 90),
	0x0483: ("STMicroelectronics CDC", 40),
	0x2341: ("Arduino CDC", 20),
}

CHUNK_SIZE = 1024 * 4
FLASHER_CHUNK_SIZE = int(1024 * 3.5)

# Boot code fallback, used when boot/*.hex is not built (see boot/Makefile).
EMBEDDED_BOOT_HEX = """\
08D04FE200000FE1C00080E300F021E10000A0E3AE0100EB741B9FE57C0091E5
0100C0E3090000EA5349454D454E535F424F4F54434F44450300000000C20100
000000000000000000000000000000007C0081E50800A0E3280081E50100A0E3
9B0100EB490200EBA500A0E3780100EB830100EB550050E30200000A2E0050E3
5B02000BF9FFFFEA9E0100EBAA00A0E36F0100EB7A0100EB410050E35200A003
FAFFFF0A480050E36500000A490050E37100000A510050E32B00000A540050E3
2C00000A520050E34E00000A460050E3D800000A450050E38000000A500050E3
6D00000A570050E30D00000A470050E31000000A4F0050E33400000A430050E3
3800000A580050E37302000A457F4FE2560050E39602000A2E0050E33402000B
DBFFFFEA440100EB0640A0E10750A0E1310100EB1A0100EA3F0100EB070056E1
0000000BD0FFFFEA0600A0E1400100EB2604A0E13E0100EB2608A0E13C0100EB
260CA0E13A0100EB06F0A0E1260200EB250200EBFEFFFFEA2F0100EB040096E4
041096E4010000E0010070E30000A0130100001A087057E2F7FFFF1ABBFFFFEA
0E40A0E1240100EB990000EB010B57E39C00002A0610A0E1595D8FE20000A0E1
A780A0E189005AE320005A1314FF2FE1F2FFFFEBCE01001B89005AE320005A13
C001000B040000EAECFFFFEBC601001B89005AE320005A13B801000BFC608FE2
156C86E2000000EA0B0100EB0080A0E30100D6E4008028E00D0100EB017057E2
FAFFFF1A4F00A0E3090100EB4B00A0E3070100EB0800A0E1050100EB0000A0E3
92FFFFEA0E0100EB0060A0E16800A0E3FF0000EB0A0CA0E3010050E2FDFFFF1A
0600A0E1D20000EB050100EB410050E3FCFFFF1A4800A0E384FFFFEA7C608FE2
1B6C86E28070A0E30100D6E4F00000EB017057E2FBFFFF1A7DFFFFEAE60000EB
5B0000EB2A43A0E30710A0E10650A0E1040095E4040084E4041051E2FBFFFF1A
5500A0E3E20000EBF70000EB0080A0E1F50000EB0050A0E12A4388E2590000EA
F10000EB0060A0E1490000EB010000EB0E0000EB66FFFFEA0E90A0E189005AE3
20005A131C00001A6000A0E3B000C6E1D000A0E3B000C6E10000A0E12000A0E3
B000C6E1D000A0E3B000C6E119FF2FE10E90A0E10100A0E3C50000EB0100A0E3
C30000EB89005AE320005A131000001AA70100EB7000A0E3B000C6E1B000D6E1
800010E3F9FFFF0A3A0010E35D00001AFF00A0E3B000C6E1140000EA0610A0E1
8000A0E3720100EB3000A0E3700100EB19FF2FE10118A0E3011041E20A29A0E3
012052E21500000AB000D6E1010050E1FAFFFF1AB000D6E1010050E1F7FFFF1A
B000D6E1010050E1F4FFFF1A040000EB0200A0E39E0000EB0200A0E39C0000EB
19FF2FE19000A0E3B000C6E10000A0E3B000C6E1F000A0E3B000C6E11EFF2FE1
F7FFFFEB4500A0E3910000EB4500A0E31EFFFFEA0A0256E30200003A2A0356E3
0000002A1EFF2FE1FF00A0E3880000EBFF00A0E315FFFFEA7F0000EBF4FFFFEB
2A43A0E30750A0E1AAFFFFEB6A0000EBB6FFFFEB2A83A0E389005AE320005A13
2500001A94408FE2194C84E25500D4E50130A0E31330A0E1A330A0E10640A0E1
A750A0E15A0100EBE800A0E3B000C4E1B000D4E1800010E31200000A010043E2
B000C4E10310A0E1B200D8E0B200C4E0011051E2FBFFFF1AD000A0E3B000C6E1
B000D6E1800010E3FCFFFF0A3A0010E30400001A035055E0E9FFFF8AFF00A0E3
B000C6E11F0000EA5000A0E3B000C6E1FF00A0E3B000C6E1CAFFFFEA0610A0E1
2000A0E3120100EB0640A0E1A750A0E1370100EB0228A0E3A000A0E3B000C4E1
B200D8E0B000C4E1012052E2B3FFFF0AB010D4E1000051E1FAFFFF1AB010D4E1
000051E1F7FFFF1AB010D4E1000051E1F4FFFF1A012082E2024084E2015055E2
ECFFFF1A9EFFFFEB0300A0E3380000EB0300A0E3360000EB0080A0E30610A0E1
A720A0E1B200D1E0008088E0012052E2FBFFFF1A0800A0E12D0000EB2804A0E1
2B0000EB4F00A0E3290000EB4B00A0E3B6FEFFEA1C108FE2001191E7F124A0E3
2108A0E1140082E50108A0E12008A0E1180082E51EFF2FE1D8011900D8010C00
B401050092000000C3000000270100008A01000000000000D00100000E90A0E1
0080A0E3FA0000EB1D0000EB008028E00100C4E4015055E2F9FFFF1A180000EB
080050E10000001A19FF2FE1BB00A0E3070000EBBB00A0E394FEFFEA0E90A0E1
190000EB0060A0E1170000EB0070A0E119FF2FE1F124A0E3201092E5FF10C1E3
011080E1201082E5681092E5021011E2FCFFFF0A701092E5021081E3701082E5
DB0000EAF114A0E3680091E5040010E2FBFFFF0A700091E5040080E3700081E5
240091E5FF0000E21EFF2FE10E50A0E1F3FFFFEB004CA0E1F1FFFFEB004884E1
EFFFFFEB004484E1EDFFFFEB000084E115FF2FE1B8349FE5241093E50E10C1E3
F01081E3282093E50C2002E2021081E1241083E50D10C1E3021081E3010080E1
240083E51EFF2FE10E70A0E1C70000EBE8408FE2164C84E20410A0E18020A0E3
0000A0E3040081E4042052E2FCFFFF1A0410A0E15C049FE52020A0E37F0000EB
54049FE51020A0E37C0000EB4C049FE51020A0E3790000EB0A02A0E3400084E5
0A12A0E39000A0E3790000EBB0A0D1E1B0A5C4E17F0000EBB000D1E1B025D4E1
020050E0B005C401B205C4012900000A9000A0E36E0000EBB200D1E1B205C4E1
740000EB01005AE31000000A04005AE30E00000A89005AE320005A131D00001A
445084E2016C81E2026086E20680A0E3440000EB745084E20280A0E3410000EB
026086E20480A0E33E0000EB110000EA445084E20160A0E10680A0E3440000EB
745084E20280A0E3410000EB806081E2785084E20480A0E33D0000EB785084E2
000094E5010090E20200001A016C81E20480A0E3360000EB9800A0E3BA0AC1E1
0000A0E1542084E2BE04D1E10100C2E4543081E2B200D3E00100C2E4FF0003E2
7A0050E3FAFFFF1A420000EB5400D4E51A0050E31800000A7E0000EBFF1402E2
9800A0E3BA0AC1E10000A0E1B002D1E1510050E31000001A5400D4E5010080E2
5400C4E55720D4E5022184E0582082E25A3081E2B200D3E00100C2E4FF0003E2
7A0050E3FAFFFF1AB805D1E15720D4E5002082E05720C4E5260000EB17FF2FE1
9800A0E3000000EA9000A0E30E90A0E1170000EBB200D6E0B200C5E0018058E2
FBFFFF1A1B0000EB19FF2FE19800A0E3000000EA8800A0E30E90A0E10C0000EB
B200D6E0B200C5E0018058E2FBFFFF1A9000A0E3060000EBB010C1E119FF2FE1
0130D0E40130C1E4012052E2FBFFFF1A1EFF2FE1AA30A0E30A2C81E2BA3AC2E1
5530A0E3052C81E2B435C2E10A2C81E2BA0AC2E11EFF2FE1F000A0E389005AE3
20005A13FF00A003B000C1E11EFF2FE1FC019FE5600090E52004A0E1FF0000E2
FC119FE5CC2041E2140050E30100001A601081E2042082E2043082E20C4043E2
01C0A0E10100A0E3000082E51000A0E3000083E5050CA0E3000081E50109A0E3
510E80E3000084E5B8119FE500B091E5050000EAAC219FE5000092E50B1040E0
020C51E30800003A00B0A0E100009CE5000BA0E100209CE5A00FE0E1022CC2E3
010000E2800482E100008CE51EFF2FE10F12A0E370019FE5880081E56C019FE5
C80081E51900A0E3400081E560019FE5500081E53300A0E3600081E554019FE5
800081E5A00081E54C019FE5C00081E5E00081E51EFF2FE10F12A0E30A22A0E3
212082E2190050E310208212802081E5A02081E501248202012482E2902081E5
B02081E510019FE5D00081E5F00081E51EFF2FE12A73A0E3350D8FE20000A0E1
041090E4042090E4043090E4044090E4045090E40168A0E31EFF2FE1F4FFFFEB
041087E4042087E4020517E30300001A043087E4044087E4045087E4F7FFFFEA
BBFFFFEB5500A0E3D1FEFFEBE8FFFFEB040097E4010050E11300001A040097E4
020050E11000001A020517E356FDFF1A016056E20300001AADFFFFEB5600A0E3
C3FEFFEBDBFFFFEB040097E4030050E10500001A040097E4040050E10200001A
040097E4050050E1E8FFFF0A4500A0E3B7FEFFEB0700A0E1B5FEFFEB2704A0E1
B3FEFFEB2708A0E1B1FEFFEB270CA0E13EFDFFEA000040F400E003A010E403A0
00E403A0180130F42000B0F4310000A80002723070A8FF0F110000A000265200
"""


class SerialPortError(Exception):
	pass


class SerialPort:
	"""Device::SerialPort work-alike on top of pyserial.

	Reads follow the Perl module semantics: the request is split into chunks
	of at most 255 bytes, each chunk waits `const_time + size * char_time`,
	and the first chunk that brings nothing back ends the whole read.
	"""

	VMIN_MAX = 255

	def __init__(self, device, baudrate, dtr=False, rts=False):
		self.read_const_time = 0.0
		self.read_char_time = 0.0

		self.port = serial.Serial()
		self.port.port = device
		self.port.baudrate = baudrate
		# VMIN = 0, VTIME = 0: os.read() never blocks, select() does the waiting.
		self.port.timeout = 0
		# Applied by open(), so the adapter never sees a stray pulse.
		self.port.dtr = dtr
		self.port.rts = rts

		try:
			self.port.open()
		except serial.SerialException as e:
			raise SerialPortError(str(e))

	def set_read_timeouts(self, const_time, char_time):
		"""Both values are in milliseconds, like read_const_time/read_char_time."""
		self.read_const_time = const_time / 1000.0
		self.read_char_time = char_time / 1000.0

	@property
	def baudrate(self):
		return self.port.baudrate

	@baudrate.setter
	def baudrate(self, value):
		self.port.baudrate = value

	@property
	def dtr(self):
		return self.port.dtr

	@dtr.setter
	def dtr(self, value):
		self.port.dtr = bool(value)

	@property
	def rts(self):
		return self.port.rts

	@rts.setter
	def rts(self, value):
		self.port.rts = bool(value)

	def read(self, wanted):
		if wanted <= 0:
			return b""

		fd = self.port.fileno()
		out = bytearray()

		while len(out) < wanted:
			size = min(wanted - len(out), self.VMIN_MAX)
			timeout = self.read_const_time + size * self.read_char_time

			try:
				ready, _, _ = select.select([fd], [], [fd], timeout)
				chunk = os.read(fd, size) if ready else b""
			except BlockingIOError:
				chunk = b""

			if not chunk:
				break
			out += chunk

		return bytes(out)

	def readb(self):
		data = self.read(1)
		return data[0] if data else -1

	def write(self, data):
		if isinstance(data, str):
			data = data.encode("latin-1")
		self.port.write(data)

	def flush_input(self):
		while self.readb() != -1:
			pass

	def close(self):
		if self.port.is_open:
			self.port.close()


def bin2hex(data):
	return data.hex().upper()


def hex2bin(text):
	text = "".join(text.split())
	if len(text) % 2:
		text = "0" + text
	return bytes.fromhex(text)


def parse_addr(value):
	if value is None:
		raise ValueError("Unknown hex value: None")
	text = str(value).strip()
	if text.lower().startswith("0x"):
		text = text[2:]
	try:
		return int(text, 16)
	except ValueError:
		raise ValueError("Unknown hex value: %s" % value)


def parse_speed(value):
	text = str(value).strip()
	if text.lower().startswith("0x"):
		return int(text[2:], 16)
	return int(text)


def read_file(path):
	with open(path, "rb") as f:
		return f.read()


def xor_checksum(data):
	chk = 0
	for b in data:
		chk ^= b
	return chk


def pack_be32(value):
	return struct.pack(">I", value & 0xFFFFFFFF)


def progress(index, done, total, tag, addr, size, start):
	elapsed = time.time() - start
	rate = (done / 1024.0) / elapsed if elapsed > 0 else 0.0
	percent = int(done / total * 100) if total else 0
	sys.stdout.write(" " * 52 + "\r")
	sys.stdout.write("#%d %02d%% [%s] %08X-%08X (%.02f Kbps)\r" % (index, percent, tag, addr, addr + size, rate))
	sys.stdout.flush()


def describe_port(info):
	name, _ = KNOWN_ADAPTERS.get(info.vid, ("", 0)) if info.vid is not None else ("", 0)
	parts = []
	if info.vid is not None and info.pid is not None:
		parts.append("%04x:%04x" % (info.vid, info.pid))
	if name:
		parts.append(name)
	elif info.description and info.description != "n/a":
		parts.append(info.description)
	if info.serial_number:
		parts.append("sn=%s" % info.serial_number)
	return ", ".join(parts)


def stable_device_path(device):
	"""Prefer the /dev/serial/by-id symlink, it survives replugging."""
	by_id = "/dev/serial/by-id"
	if not os.path.isdir(by_id):
		return device

	target = os.path.realpath(device)
	for name in sorted(os.listdir(by_id)):
		link = os.path.join(by_id, name)
		if os.path.realpath(link) == target:
			return link
	return device


def find_serial_ports():
	"""All plausible UART adapters, best candidate first."""
	found = []
	for info in list_ports.comports():
		if info.vid is not None:
			score = KNOWN_ADAPTERS.get(info.vid, ("", 10))[1]
		elif os.path.basename(info.device).startswith(("ttyUSB", "ttyACM")):
			score = 5
		else:
			continue
		found.append((score, stable_device_path(info.device), info))

	found.sort(key=lambda item: (-item[0], item[1]))
	return found


def print_serial_ports():
	ports = find_serial_ports()
	if not ports:
		print("No UART adapters found.")
		return

	print("Available UART adapters:")
	for _, path, info in ports:
		print("	%s (%s)" % (path, describe_port(info)))


def autodetect_device():
	ports = find_serial_ports()
	if not ports:
		raise SerialPortError("No UART adapter found, use --device or --list-devices")

	best_score = ports[0][0]
	best = [p for p in ports if p[0] == best_score]
	if len(best) > 1:
		lines = ["Several UART adapters found, use --device or save one with --save-config:"]
		for _, path, info in best:
			lines.append("	%s (%s)" % (path, describe_port(info)))
		raise SerialPortError("\n".join(lines))

	_, path, info = best[0]
	print("Using UART adapter: %s (%s)" % (path, describe_port(info)))
	return path


def mk_chaos_boot(use_chaos):
	"""Modified chaos/PV boot code (from boot/)."""
	name = "chaos_x85.hex" if use_chaos else "pv_boot_x85.hex"
	alt_boot = os.path.join(SCRIPT_DIR, "boot", name)

	if os.path.isfile(alt_boot):
		print("Using boot from %s" % alt_boot)
		with open(alt_boot, "r") as f:
			data = f.read()
	else:
		data = EMBEDDED_BOOT_HEX

	return hex2bin(data)


def write_boot(port, boot, secure: bool = False) -> bool:
	chk = xor_checksum(boot)

	port.write(b"\x30")
	# Boot code size
	port.write(struct.pack("<H", len(boot) & 0xFFFF))
	# Boot code itself
	port.write(boot)
	# ...and its XOR
	port.write(bytes([chk]))

	c = port.readb()
	if c in (0xC1, 0xB1, 0x01):
		return True

	print("Invalid answer: %02X" % (c & 0xFF), file=sys.stderr)
	return False


def chaos_ping(port):
	port.write(b"A")
	c = port.readb()
	if c != 0x52:
		print("[chaos_ping] Invalid answer 0x%02X" % (c & 0xFF), file=sys.stderr)
		return False
	return True


def chaos_keep_alive(port):
	port.write(b".")
	return True


def chaos_set_speed(port, speed):
	if speed not in CHAOS_SPEEDS:
		print("Invalid speed %d! Allowed: %s" % (speed, ", ".join(str(s) for s in sorted(CHAOS_SPEEDS))),
			file=sys.stderr)
		return False

	print("sending: %d" % CHAOS_SPEEDS[speed])

	old_speed = port.baudrate
	port.write(b"H" + bytes([CHAOS_SPEEDS[speed]]))
	c = port.readb()
	step = 0

	if c == 0x68:
		step += 1
		port.baudrate = speed
		port.write(b"A")
		c = port.readb()
		if c == 0x48:
			# Speed changed
			return True
		print("err: %d" % c)

	port.baudrate = old_speed
	print("[chaos_set_speed] Invalid answer 0x%02X (step=%d)" % (c & 0xFF, step), file=sys.stderr)
	return False


def chaos_read_info(port, use_chaos):
	port.write(b"I")

	raw = port.read(128)
	if len(raw) != 128:
		print("[chaos_read_info] Invalid answer size (%d != 128)" % len(raw), file=sys.stderr)
		return None

	if use_chaos:
		print("Information dump: %s (chaos not supported!)" % bin2hex(raw))
		sys.exit(0)

	#	BYTE strModelName[16];					// - model
	#	BYTE strManufacturerName[16];			// - manufacturer
	#	BYTE strIMEI[16];						//- IMEI (in ASCII)
	#	BYTE reserved0[16];						// - (reserved)
	#	DWORD flashBaseAddr;					// - base address of flash (ROM)
	#	BYTE reserved1[12];						// - (reserved)
	#	DWORD flash0Type;						//flash1 IC Manufacturer (LOWORD) and device ID (HIWORD)
	#	BYTE flashSizePow;						// - N, CFI byte 27h. Size of flash = 2^N
	#	WORD writeBufferSize;					// - CFI bytes 2Ah-2Bh size of write-buffer (not used by program)
	#	BYTE flashRegionsNum;					// - CFI byte 2Ch - number of regions.
	#	WORD flashRegion0BlocksNumMinus1;		// - N, CFI number of blocks in 1st region = N+1
	#	WORD flashRegion0BlockSizeDiv256;		// - N, CFI size of blocks in 1st region = N*256
	#	WORD flashRegion1BlocksNumMinus1;		// - N, CFI number of blocks in 2nd region = N+1
	#	WORD flashRegion1BlockSizeDiv256;		// - N, CFI size of blocks in 2nd region = N*256
	#	BYTE reserved2[32];						// - (reserved)

	(model, vendor, imei, hash_raw, flash_base, _reserved0,
		flash_type, flash_size, flash_buffer_size, flash_regions,
		flash_region0_nblocks, flash_region0_size, flash_region1_nblocks, flash_region1_size,
		_reserved1) = struct.unpack("<16s16s16s16sI12sIBHBHHHH32s", raw)

	def cstr(data):
		return data.split(b"\0", 1)[0].decode("latin-1")

	return {
		"model": cstr(model),
		"vendor": cstr(vendor),
		"imei": cstr(imei),
		"hash": bin2hex(hash_raw).lower(),
		"flash": {
			"base": flash_base,
			"type": flash_type,
			"size": 1 << flash_size,
			"write_buff_size": flash_buffer_size,
			"regions": flash_regions,
			"region0": {
				"blocks": flash_region0_nblocks,
				"size": flash_region0_size,
			},
			"region1": {
				"blocks": flash_region1_nblocks,
				"size": flash_region1_size,
			},
		},
	}


def chaos_goto(port, addr):
	packed = pack_be32(addr)

	while True:
		port.write(b"G" + packed * 3)
		data = port.read(4)

		if len(data) < 4:
			print("[chaos_goto] Invalid answer 0x%s" % bin2hex(data), file=sys.stderr)

			if data[:1] == b"\xAA":  # CRC error
				chaos_keep_alive(port)
				continue
			sys.exit(1)

		if data[::-1] != packed:
			print("[chaos_goto] Addr corrupted o_O 0x%s" % bin2hex(data[::-1]), file=sys.stderr)
		break

	return True


def chaos_read_ram(port, read_addr, read_size, chunk, cmd=b"R"):
	# Cut into blocks up front
	blocks = []
	for j in range(0, read_size, chunk):
		addr = read_addr + j
		size = min(chunk, read_size - j)
		blocks.append((addr, size, cmd + pack_be32(addr) + pack_be32(size)))

	start = time.time()
	buffer = bytearray()

	for i, (addr, size, request) in enumerate(blocks):
		if i % 10 == 0:
			progress(i, addr - read_addr, read_size, "READ", addr, size, start)

		tries = 10
		while True:
			port.write(request)

			buf = port.read(size + 4)
			if len(buf) < size + 4:
				print("\n[chaos_read_ram] Invalid answer size (%d != %d)" % (len(buf), size + 4), file=sys.stderr)
				tries -= 1
				if tries >= 0:
					if not chaos_ping(port):
						sys.exit(1)
					continue
				sys.exit(1)

			ok = buf[size:size + 2]
			chk = (buf[size + 3] << 8) | buf[size + 2]

			if ok != b"OK":
				print("\n[chaos_read_ram] Invalid answer '%02X%02X'" % (ok[0], ok[1]), file=sys.stderr)
				tries -= 1
				if tries >= 0:
					if not chaos_ping(port):
						sys.exit(1)
					continue
				sys.exit(1)

			buf = buf[:size]
			own_chk = xor_checksum(buf)

			if chk != own_chk:
				print("\n[chaos_read_ram] Invalid CRC %02X != %02X" % (chk, own_chk), file=sys.stderr)
				tries -= 1
				if tries >= 0:
					if not chaos_ping(port):
						sys.exit(1)
					continue
				sys.exit(1)

			buffer += buf
			break

	return bytes(buffer)


def make_write_blocks(cmd, dst_addr, buff, chunk):
	blocks = []
	for j in range(0, len(buff), chunk):
		tmp = buff[j:j + chunk]
		addr = dst_addr + j
		blocks.append((addr, len(tmp),
			cmd + pack_be32(addr) + pack_be32(len(tmp)) + tmp + bytes([xor_checksum(tmp)])))
	return blocks


def chaos_write_ram(port, dst_addr, buff, chunk):
	blocks = make_write_blocks(b"W", dst_addr, buff, chunk)
	start = time.time()

	for i, (addr, size, request) in enumerate(blocks):
		if i % 10 == 0:
			progress(i, addr - dst_addr, len(buff), "WRITE", addr, size, start)

		tries = 999999
		while True:
			port.write(request)

			ok = port.read(2)
			if ok != b"OK":
				print("\n[chaos_write_ram] Invalid answer '%s'" % bin2hex(ok), file=sys.stderr)
				tries -= 1
				if tries >= 0:
					chaos_keep_alive(port)
					continue
				sys.exit(1)
			break

	print()
	return True


def chaos_write_flash(port, dst_addr, buff, chunk):
	blocks = make_write_blocks(b"F", dst_addr, buff, chunk)

	def get_error(ok):
		if ok == b"\xFF\xFF":
			return "Out of flash bounds!"
		if ok == b"\xBB\xBB":
			return "CRC error!"
		return "Invalid answer '%s'" % bin2hex(ok)

	start = time.time()

	for i, (addr, size, request) in enumerate(blocks):
		if i % 10 == 0:
			progress(i, addr - dst_addr, len(buff), "WRITE", addr, size, start)

		tries = 999999
		while True:
			port.write(request)

			ok = port.read(2)
			if ok == b"\x01\x01":  # Block sent!
				ok = port.read(2)
				if ok == b"\x02\x02":  # Block erased!
					ok = port.read(2)
					if ok == b"\x03\x03":  # Block written!
						break
					stage = "write"
				else:
					stage = "erase"
			else:
				stage = "send"

			print("\n[chaos_write_flash] %s (%s)" % (get_error(ok), stage), file=sys.stderr)
			tries -= 1
			if tries >= 0:
				chaos_keep_alive(port)
				continue
			sys.exit(1)

	print()
	return True


def chaos_read_otp(port):
	return chaos_read_ram(port, 0xA0000000, 0x200, 4, b"O")


def parse_flasher_tasks(flasher):
	tasks = []
	for task in flasher:
		args = [a.strip() for a in task.split(",")]
		cmd = args[0].lower()

		if cmd == "read":
			if len(args) < 4:
				raise ValueError("Unknown file in command: `%s`" % task)
			tasks.append({
				"cmd": cmd,
				"addr": parse_addr(args[1]),
				"size": parse_addr(args[2]),
				"file": args[3],
			})
		elif cmd in ("write", "erase"):
			if len(args) < 3:
				raise ValueError("Unknown file in command: `%s`" % task)
			tasks.append({
				"cmd": cmd,
				"addr": parse_addr(args[1]),
				"file": args[2],
			})
		else:
			raise ValueError("Unknown flasher command: %s" % task)

	return tasks


def load_config(path):
	config = configparser.ConfigParser()
	if not os.path.isfile(path):
		return {}

	config.read(path)
	if not config.has_section(CONFIG_SECTION):
		return {}

	# Both --boot-speed and boot_speed spellings are accepted.
	return {k.replace("-", "_"): v for k, v in config.items(CONFIG_SECTION)}


def save_config(path, opts):
	config = configparser.ConfigParser()
	if os.path.isfile(path):
		config.read(path)
	if not config.has_section(CONFIG_SECTION):
		config.add_section(CONFIG_SECTION)

	for key in ("device", "boot_speed", "speed", "ign", "dtr", "rts", "exec_addr", "linux_dtb", "use_chaos",
		"signed_loader"):
		config.set(CONFIG_SECTION, key.replace("_", "-"), str(opts[key]))

	with open(path, "w") as f:
		config.write(f)

	print("Settings saved to %s" % path)


def config_bool(value):
	if isinstance(value, bool):
		return value
	return str(value).strip().lower() in ("1", "true", "yes", "on")


def make_parser():
	parser = argparse.ArgumentParser(
		prog="chaos-boot.py",
		description="Chaos/PV bootloader client for PMB887x (S-Gold) phones.",
		formatter_class=argparse.RawDescriptionHelpFormatter,
		epilog="\n".join((
			"Flasher:",
			"	--flasher read,<addr>,<size>,<file>        read <size> bytes in flash/ram at <addr> and save to <file>",
			"	--flasher write,<addr>,<file>              write <file> to flash/ram at <addr>",
		)))

	# Defaults stay None so the settings file can fill them in.
	parser.add_argument("--device", metavar="/dev/ttyUSB0",
		help="com port device ('auto' = autodetect the UART adapter)")
	parser.add_argument("--boot-speed", metavar="115200", help="boot speed")
	parser.add_argument("--speed", metavar="1600000", help="speed after boot")
	parser.add_argument("--ign", dest="ign", action="store_const", const=1,
		help="autoignition (default)")
	parser.add_argument("--ign2", dest="ign", action="store_const", const=2,
		help="vova7890 lazy ass ignition")
	parser.add_argument("--no-ign", dest="ign", action="store_const", const=0,
		help="disable autoignition")
	parser.add_argument("--dtr", action="store_true", default=None, help="up dtr pin (for noname DCA-500)")
	parser.add_argument("--rts", action="store_true", default=None, help="up rts pin (for noname DCA-500)")
	parser.add_argument("--flasher", action="append", default=[], metavar="TASK", help="see below")

	parser.add_argument("--hex", dest="as_hex", action="store_true", default=None, help="dump output as hex")
	parser.add_argument("--picocom", action="store_true", default=None, help="run picocom after exec")
	parser.add_argument("--exec-addr", metavar="ADDR", help="change exec addr (default: 0xA8000000)")
	parser.add_argument("--exec", dest="exec_file", metavar="FILE", help="upload and run <file>")
	parser.add_argument("--linux", metavar="FILE", help="upload and run a linux kernel image")
	parser.add_argument("--linux-dtb", metavar="FILE", help="dtb for --linux")

	parser.add_argument("--dump-otp", action="store_true", default=None, help="dump otp region")
	parser.add_argument("--use-chaos", action="store_true", default=None, help="use the chaos boot code")
	parser.add_argument("--signed-loader", metavar="FILE", help="signed loader for secure boot phones")

	parser.add_argument("--config", metavar="FILE", default=os.environ.get("CHAOS_BOOT_CONFIG", DEFAULT_CONFIG),
		help="settings file (default: %s)" % DEFAULT_CONFIG)
	parser.add_argument("--save-config", action="store_true", help="save the current settings and exit")
	parser.add_argument("--list-devices", action="store_true", help="list the available UART adapters and exit")

	return parser


def resolve_options(args, config):
	def pick(name, default, cast: Callable[[str], Any]=str):
		value = getattr(args, name, None)
		if value is not None:
			return cast(value) if not isinstance(value, bool) else value
		if name in config:
			return cast(config[name])
		return default

	opts = {
		"device": pick("device", "auto"),
		"boot_speed": pick("boot_speed", 115200, parse_speed),
		"speed": pick("speed", 1600000, parse_speed),
		"ign": pick("ign", 1, int),
		"rts": pick("rts", False, config_bool),
		"exec_addr": pick("exec_addr", "0xA8000000"),
		"exec_file": pick("exec_file", None),
		"linux": pick("linux", None),
		"linux_dtb": pick("linux_dtb", os.path.join(SCRIPT_DIR, "..", "linux", "arch", "arm", "boot", "dts",
			"siemens-el71.dtb")),
		"as_hex": pick("as_hex", False, config_bool),
		"picocom": pick("picocom", False, config_bool),
		"dump_otp": pick("dump_otp", False, config_bool),
		"use_chaos": pick("use_chaos", False, config_bool),
		"signed_loader": pick("signed_loader", os.path.join(SCRIPT_DIR, "boot", "pv_boot_r17secboot.bin")),
	}

	# Without an explicit --dtr the pin follows the ignition mode.
	opts["dtr"] = pick("dtr", opts["ign"] == 2, config_bool)

	return opts


def wait_for_phone(port, opts):
	"""Poll with 'AT' until the boot ROM answers, driving DTR for autoignition."""
	print("Please, short press red button!")

	last_dtr_val = False
	last_dtr = 0.0
	last_dtr_timeout = 0.5
	read_zero = 0

	while True:
		if opts["ign"]:
			if time.time() - last_dtr > last_dtr_timeout or read_zero > 0:
				if read_zero > 0:
					last_dtr_val = False  # powered off, apparently

				last_dtr_timeout = 1.5 if last_dtr_val else 0.5
				last_dtr_val = not last_dtr_val
				last_dtr = time.time()
				read_zero = 0
				try:
					port.dtr = (not last_dtr_val) if opts["ign"] == 2 else last_dtr_val
				except Exception as e:
					print(f"Error setting DTR: {e}")

				if last_dtr_val:
					sys.stdout.write("^")
					sys.stdout.flush()

		port.write(b"AT")

		c = port.readb()
		if c in (0xB0, 0xC0):
			if opts["ign"]:
				port.dtr = opts["dtr"]

			print()
			print("SGOLD detected!" if c == 0xB0 else "NewSGOLD detected!")

			# poll for optional extra byte for secure boot check
			c = port.readb()
			if c == 0xC4:
				print("Secure Boot detected!")
				return c

			return c

		if c == 0:
			read_zero += 1

		sys.stdout.write(".")
		sys.stdout.flush()


def run_flasher(port, opts, tasks):
	info = chaos_read_info(port, opts["use_chaos"])
	if not info:
		sys.exit("Can't read phone info!")

	flash = info["flash"]
	print("Phone: %s %s, IMEI: %s, Flash: %d Mb (%04X:%04X)" % (
		info["vendor"], info["model"], info["imei"],
		flash["size"] / 1024 / 1024,
		flash["type"] & 0xFFFF, (flash["type"] >> 16) & 0xFFFF))

	print()
	for task in tasks:
		in_flash = flash["base"] <= task["addr"] <= flash["base"] + flash["size"]

		# Read RAM and FLASH
		if task["cmd"] == "read":
			print("Read %d bytes from %08X (%s) to '%s'" % (
				task["size"], task["addr"], "FLASH" if in_flash else "RAM", task["file"]))

			chaos_keep_alive(port)
			res = chaos_read_ram(port, task["addr"], task["size"], FLASHER_CHUNK_SIZE)
			if res is not None:
				with open(task["file"], "wb") as f:
					f.write(res)

		# Write RAM and FLASH
		elif task["cmd"] == "write":
			# Flash address space, chaos_write_flash() is implemented but untested
			if in_flash:
				sys.exit("NOT SUPPORTED YET :( PLZ, GO PINAT' MENYA IF YOU WANT THIS FEATURE")

			# Otherwise it is treated as the RAM address space
			print("Write %s to RAM (%08X)" % (task["file"], task["addr"]))
			raw = read_file(task["file"])

			chaos_keep_alive(port)
			chaos_write_ram(port, task["addr"], raw, FLASHER_CHUNK_SIZE)
		else:
			sys.exit("Unknown flasher command: %s" % task["cmd"])
		print()
	print()


def run_exec(port, opts, exec_addr):
	raw = read_file(opts["exec_file"])
	chaos_keep_alive(port)

	print("Load %s to RAM (%08X)... (size=%d)" % (opts["exec_file"], exec_addr, len(raw)))
	if not chaos_write_ram(port, exec_addr, raw, FLASHER_CHUNK_SIZE):
		sys.exit("load error")

	print("Exec %08X..." % exec_addr)
	chaos_goto(port, exec_addr)


def run_linux(port, opts, exec_addr):
	if exec_addr & 0xFFFFF:
		sys.exit("Invalid kernel addr")

	preloader_addr = exec_addr
	kernel_addr = exec_addr + 0x8000

	dtb = read_file(opts["linux_dtb"])
	kernel = read_file(opts["linux"])

	dtb_addr = kernel_addr + len(kernel)

	align = (1 << 20) * 4
	if dtb_addr % align:
		dtb_addr += align - (dtb_addr % align)

	preloader = (
		hex2bin("0000A0E310808FE2001098E50C808FE2002098E51C804FE202F988E2") +  # linux_boot.S
		struct.pack("<I", 0xFFFFFFFF) +  # machine_id
		struct.pack("<I", dtb_addr)  # dts/atags
	)

	chaos_keep_alive(port)

	print("Exec linux!")
	print("Kernel: %s [0x%08X]" % (opts["linux"], kernel_addr))
	print("DTB: %s [0x%08X]" % (opts["linux_dtb"], dtb_addr))
	print()

	print("Load preloader to RAM (%08X)... (size=%d)" % (preloader_addr, len(preloader)))
	if not chaos_write_ram(port, preloader_addr, preloader, CHUNK_SIZE):
		sys.exit("load error")
	print()

	print("Load dtb to RAM (%08X)... (size=%d)" % (dtb_addr, len(dtb)))
	if not chaos_write_ram(port, dtb_addr, dtb, CHUNK_SIZE):
		sys.exit("load error")
	print()

	print("Load kernel to RAM (%08X)... (size=%d)" % (kernel_addr, len(kernel)))
	if not chaos_write_ram(port, kernel_addr, kernel, CHUNK_SIZE):
		sys.exit("load error")

	print("Exec %08X..." % preloader_addr)
	chaos_goto(port, preloader_addr)


def dump_output(port, as_hex):
	if as_hex:
		while True:
			c = port.readb()
			if c > -1:
				ch = chr(c)
				print("%s | %02X" % ("'%s'" % ch if ch.isprintable() else " ? ", c))
	else:
		while True:
			c = port.readb()
			if c == 0:
				break
			if c > -1:
				sys.stdout.write(chr(c))
				sys.stdout.flush()


def main():
	parser = make_parser()
	args = parser.parse_args()

	if args.list_devices:
		print_serial_ports()
		return 0

	config = load_config(args.config)
	opts = resolve_options(args, config)

	if args.save_config:
		if opts["device"] == "auto":
			opts["device"] = autodetect_device()
		save_config(args.config, opts)
		return 0

	try:
		exec_addr = parse_addr(opts["exec_addr"])
		flasher_tasks = parse_flasher_tasks(args.flasher)
	except ValueError as e:
		sys.exit(str(e))

	device = opts["device"]
	if device == "auto":
		try:
			device = autodetect_device()
		except SerialPortError as e:
			sys.exit(str(e))

	try:
		port = SerialPort(device, opts["boot_speed"],
			dtr=(opts["ign"] == 2) if opts["ign"] else opts["dtr"],
			rts=opts["rts"])
	except SerialPortError as e:
		sys.exit("open port error (%s): %s" % (device, e))

	port.set_read_timeouts(20 if opts["ign"] else 100, 0)

	try:
		if opts["ign"]:
			port.flush_input()

		while True:
			c = wait_for_phone(port, opts)
			secure = c == 0xC4

			port.set_read_timeouts(200, 200)

			payload = mk_chaos_boot(opts["use_chaos"])

			if secure:
				c = port.readb()
				chip = 0x800 + (c & 3)
				gpio = 0xa00 + (c >> 2)
				print("CHIP_UID=%04X GPIO=%04X" % (chip, gpio))
				payload = read_file(opts["signed_loader"])
				print("Using signed loader: %s" % opts["signed_loader"])

			# first payload bytes
			print("Boot payload: %s" % bin2hex(payload[:32]))

			print("Sending boot...")
			result = write_boot(port, payload, secure)
			if not result:
				print("Boot write error!")
				return 1

			c = port.readb()
			if c != 0xA5:
				print("Invalid answer: %02X" % (c & 0xFF))
				print("Chaos bootloader not found!\n")
				port.set_read_timeouts(20 if opts["ign"] else 100, 0)
				continue

			time.sleep(0.2)

			if not opts["use_chaos"]:
				# Odd check in the PV boot
				port.write(b"\x55")
				c = port.readb()

				if c != 0xAA:
					print("Boot init error (answer=%02X)" % (c & 0xFF))
					return 1

			if not chaos_ping(port):
				return 1
			for _ in range(4):
				chaos_keep_alive(port)
			if opts["speed"] != opts["boot_speed"]:
				if not chaos_set_speed(port, opts["speed"]):
					return 1
			chaos_keep_alive(port)
			print("Chaos Bootloader - OK")

			if opts["dump_otp"]:
				otp = chaos_read_otp(port)
				print("OTP(%d): %s" % (len(otp), bin2hex(otp)))
				return 0

			# Mini flasher
			if flasher_tasks:
				run_flasher(port, opts, flasher_tasks)

			# Run a file in RAM
			if opts["exec_file"]:
				run_exec(port, opts, exec_addr)

			# Run Linux from RAM
			if opts["linux"]:
				run_linux(port, opts, exec_addr)

			if opts["picocom"]:
				port.close()
				return subprocess.call(["picocom", "-b", str(opts["speed"]), device])

			dump_output(port, opts["as_hex"])
			break
	except KeyboardInterrupt:
		pass
	finally:
		try:
			port.dtr = opts["ign"] == 2
			port.rts = False
		except Exception as e:
			pass
		port.close()
	return 0


if __name__ == "__main__":
	sys.exit(main())
