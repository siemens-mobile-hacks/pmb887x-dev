package siemens_boot;

use warnings;
use strict;
use Device::SerialPort;

require "../common/perl/regs.pm";

sub cmd_write {
	my ($port, $addr, $value, $irq_handler) = @_;
	$port->write("W".chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
		chr(($value >> 24) & 0xFF).chr(($value >> 16) & 0xFF).chr(($value >> 8) & 0xFF).chr($value & 0xFF));
	my ($count, $ack) = $port->read(1);
	if ($count != 1) {
		die "Transfer error ($count != 1)";
	} else {
		if ($ack eq "!") {
			_handle_irq($port, "write", $irq_handler);
		} elsif ($ack ne ";") {
			die sprintf("write(%08X, %08X): invalid ack = %02X\n", $addr, $value, ord($ack));
		}
	}
}

sub cmd_read {
	my ($port, $addr, $raw, $irq_handler) = @_;
	$port->write("R".chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF));
	my ($count, $data) = $port->read(5);
	if ($count != 5) {
		printf("%02X\n", ord(substr($data, 0, 1)));
		die sprintf("read(%08X): Transfer error (%d != 5)\n", $addr, $count);
	} else {
		my ($buf, $ack) = (substr($data, 0, 4), substr($data, 4, 1));
		if ($ack eq "!") {
			_handle_irq($port, "read", $irq_handler);
		} elsif ($ack ne ";") {
			die sprintf("read(%08X): invalid ack = %02X\n", $addr, ord($ack));
		}
		return $raw ? $buf : unpack("V", $buf);
	}
}

sub cmd_ping {
	my ($port, $irq_handler) = @_;
	$port->write(".");
	my ($count, $data) = $port->read(1);
	if ($data eq ",") {
		_handle_irq($port, "ping", $irq_handler);
	} else {
		die "PING ERROR (c=".sprintf("%02X (%s)%s", ord($data), $data, !length($data) ? " EMPTY" : "").")" if ($data ne ".");
	}
}

sub _handle_irq {
	my ($port, $cmd, $irq_handler) = @_;
	my ($count, $data) = $port->read(1);
	if ($count == 1) {
		$irq_handler->(ord($data)) if ($irq_handler);
		printf("!!!!!!! NEW IRQ: %02X\n", ord($data));
		return ord($data);
	}
	die "Can't get IRQ num (cmd_$cmd)\n";
}

sub readb {
	my ($port) = @_;
	my ($count, $char) = $port->read(1);
	return ord($char) if ($count);
	return -1;
}
1;