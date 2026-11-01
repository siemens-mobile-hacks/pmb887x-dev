#!/usr/bin/env python3
import argparse
import struct
import sys

import serial

DEFAULT_DEVICE = "/dev/ttyUSB0"
SPEED = 115200
TIMEOUT_S = 5
SYNC = 0x03
ACK = 0x06
CCID_HEADER_SIZE = 10
PC_TO_RDR_ICC_POWER_ON = 0x62
PC_TO_RDR_ESCAPE = 0x6B
PC_TO_RDR_XFR_BLOCK = 0x6F
CCID_COMMAND_FAILED = 0x40
ESCAPE_POWER_OFF = 0xF0


class Reader:
	def __init__(self, device):
		self.port = serial.Serial(device, SPEED, stopbits=serial.STOPBITS_TWO, timeout=TIMEOUT_S)
		self.sequence = 0

	def read_exact(self, size):
		data = self.port.read(size)
		if len(data) != size:
			raise TimeoutError("SIM reader did not answer")
		return data

	def read_frame(self):
		if self.read_exact(2) != bytes([SYNC, ACK]):
			raise RuntimeError("SIM reader sent a malformed frame")
		header = self.read_exact(CCID_HEADER_SIZE)
		data = self.read_exact(struct.unpack_from("<I", header, 1)[0])
		self.read_exact(1)
		return header, data

	def command(self, message_type, data=b""):
		header = struct.pack("<BIBBBBB", message_type, len(data), 0, self.sequence, 0, 0, 0)
		self.sequence = (self.sequence + 1) & 0xFF
		frame = bytes([SYNC, ACK]) + header + data
		lrc = 0
		for value in frame:
			lrc ^= value
		self.port.write(frame + bytes([lrc]))
		self.read_frame()
		header, data = self.read_frame()
		if header[7] & CCID_COMMAND_FAILED:
			raise RuntimeError("SIM reader command 0x%02X failed: error 0x%02X" % (message_type, header[8]))
		return data


def main():
	parser = argparse.ArgumentParser(description="Talk to the sim-reader BSP app.")
	parser.add_argument("-d", "--device", default=DEFAULT_DEVICE)
	subparsers = parser.add_subparsers(dest="command", required=True)
	subparsers.add_parser("atr", help="power the card on and print its ATR")
	apdu = subparsers.add_parser("apdu", help="power the card on and exchange T=0 TPDUs")
	apdu.add_argument("tpdus", nargs="+", metavar="HEX")
	subparsers.add_parser("off", help="power the phone off")
	args = parser.parse_args()

	reader = Reader(args.device)
	if args.command == "off":
		reader.command(PC_TO_RDR_ESCAPE, bytes([ESCAPE_POWER_OFF]))
		return 0
	print("ATR:", reader.command(PC_TO_RDR_ICC_POWER_ON).hex(" ").upper())
	for text in getattr(args, "tpdus", []):
		print("RX:", reader.command(PC_TO_RDR_XFR_BLOCK, bytes.fromhex(text)).hex(" ").upper())
	return 0


if __name__ == "__main__":
	try:
		sys.exit(main())
	except (OSError, RuntimeError, serial.SerialException) as error:
		sys.exit(str(error))
