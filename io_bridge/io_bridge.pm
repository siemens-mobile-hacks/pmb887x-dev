package siemens_boot;

use warnings;
use strict;
use Device::SerialPort;
require "test.pm";

sub boot_module_init {
	my $port = shift;
	
	$port->read_char_time(1000);
	$port->read_const_time(1000);
	
	my $buf;
	open F, "+>/dev/shm/siemens_io_sniff" or die("$!");
	binmode F;
	chmod 0666, "/dev/shm/siemens_io_sniff";
	
	sub reset_cmd {
		truncate F, 0;
		seek(F, 0, 0);
	}
	my $xuj = 1;
	reset_cmd();
	while (1) {
		seek F, 0, 0;
		read(F, $buf, 128);
		
		my $cmd = substr($buf, 0, 1);
		if ($cmd eq "R") {
			
			my $addr = unpack("V", substr($buf, 1, 4));
			my $data = cmd_read($port, $addr, 1);
			
			reset_cmd();
			print F "!".$data; # OK + data
			
			my $vv = unpack("V", $data);
			printf("READ from %08X (%08X)%s\n", $addr, $vv, reg_name($addr, $vv));
			
			$xuj = 1;
			if ($addr >= 0xf4400000 && $addr <= 0xf440FFFF) {
				$xuj = 0;
			}
		} elsif ($cmd eq "W") {
			reset_cmd();
			my $addr = unpack("V", substr($buf, 1, 4));
			my $value = unpack("V", substr($buf, 5, 4));
			
			my $valid = 1;
			if ($addr == 0xF430004C || $addr == 0xF4300050) {
				# Запретим писать в TX/RX пины, мы же работаем по UART :)
				$valid = 0;
			}
			
			if ($addr == 0xF4400024 || $addr == 0xF4400028 || $addr == 0xF4300118 || $addr == 0xf440007C) {
				# Собаку отключили, нет смысла пробрасывать её регистры для записи, да и падает из-за этого
				$valid = 0;
			}
			
			if ($addr >= 0xF4500000 && $addr <= 0xF4500FFF) {
				# UART отваливается от смены частоты CPU, нужно что-то с этим делать...
				$valid = 0;
			}
			
			if ($addr == 0xF4B00000) {
				# Если сменить частоту клока - всё валится
				$valid = 0;
			}
			
			if ($addr == 0xF280020C) {
				# 0x77 irq
				$valid = 0;
			}
			
#	if (offset == 0xf4400024 || offset == 0xf4400028 || offset == 0xf45000a8 || (offset >= 0xf6400000 && offset <= 0xf640ffff))
#		return;
			
			printf("WRITE %08X to %08X%s%s\n", $value, $addr, reg_name($addr, $value), $valid ? "" : " | SKIP!!!!");
			cmd_write($port, $addr, $value) if ($valid);
			
			print F "!"; # OK
		} else {
			cmd_ping($port);
		}
		
#		cmd_ping($port);
#		cmd_write($port, 0x80000, 0xDEADBEEF);
#		printf("%08X\n", cmd_read($port, 0x80000));
	}
	close F;
	
	my $c;
	while (($c = readb($port)) >= 0) {
		print chr($c);
	}
}
