package siemens_boot;

use warnings;
use strict;
use Device::SerialPort;

require "../common/perl/regs.pm";

sub cmd_write {
	my ($port, $addr, $value) = @_;
	$port->write("W".chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
		chr(($value >> 24) & 0xFF).chr(($value >> 16) & 0xFF).chr(($value >> 8) & 0xFF).chr($value & 0xFF));
	my ($count, $ack) = $port->read(1);
	if ($count != 1) {
		die "Transfer error ($count != 1)";
	} else {
		if ($ack ne ";") {
			die "invalid ack = ".sprintf("%02X", ord($ack))."\n";
		}
	}
}

sub cmd_read {
	my ($port, $addr, $raw) = @_;
	$port->write("R".chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF));
	my ($count, $data) = $port->read(5);
	if ($count != 5) {
		printf("%02X\n", ord(substr($data, 0, 1)));
		die "Transfer error ($count != 5)";
	} else {
		my ($buf, $ack) = (substr($data, 0, 4), substr($data, 4, 1));
		if ($ack eq ";") {
			return $raw ? $buf : unpack("V", $buf);
		} else {
			die "invalid ack = ".sprintf("%02X", ord($ack))."\n";
		}
	}
}

sub cmd_ping {
	my ($port) = @_;
	$port->write(".");
	my $c = readb($port);
	die "PING ERROR (c=".sprintf("%02X (%c)", $c, $c & 0xFF).")" if ($c != 0x2E);
}

sub readb {
	my ($port) = @_;
	my ($count, $char) = $port->read(1);
	return ord($char) if ($count);
	return -1;
}
1;