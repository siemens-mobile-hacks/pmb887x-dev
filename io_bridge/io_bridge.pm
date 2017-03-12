package siemens_boot;

use warnings;
use strict;
use Device::SerialPort;
require "test.pm";

sub boot_module_init {
	my ($port, $speed) = @_;
	
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
		
		my $irq = -1;
		my $cmd = substr($buf, 0, 1);
		if ($cmd eq "R") {
			my $addr = unpack("V", substr($buf, 1, 4));
			my $from = unpack("V", substr($buf, 5, 4));
			my $data = cmd_read($port, $addr, 1, sub {
				$irq = shift;
			});
			
			my $ack = $irq != -1 ? '+' : '!';
			
			reset_cmd();
			print F $ack.$data; # OK + data
			print F chr($irq) if ($irq != -1); # IRQ_N
			
			my $vv = unpack("V", $data);
			
			if ($addr != 0xF4300118 && $addr != 0xF4B00010 && $addr != 0xF64000F8) {
				printf("READ from %08X (%08X)%s (from %08X)\n", $addr, $vv, reg_name($addr, $vv), $from);
			}
			
			$xuj = 0;
		} elsif ($cmd eq "W") {
			reset_cmd();
			my $addr = unpack("V", substr($buf, 1, 4));
			my $value = unpack("V", substr($buf, 5, 4));
			my $from = unpack("V", substr($buf, 9, 4));
			
			my $valid = 1;
			if ($addr == 0xF430004C || $addr == 0xF4300050) {
				# Запретим писать в TX/RX пины, мы же работаем по UART :)
				$valid = 0;
			}
			
			if ($addr == 0xF4400024 || $addr == 0xF4400028 || $addr == 0xF4300118 || $addr == 0xf440007C) {
				# Собаку отключили, нет смысла пробрасывать её регистры для записи, да и падает из-за этого
				$valid = 0;
			}
			
			if ($addr >= 0xF4500000 && $addr <= 0xF4500FFF && $addr != 0xF45000CC) {
				# UART отваливается от смены частоты CPU, нужно что-то с этим делать...
				$valid = 0;
			}
			
			if (($addr & 0xFFFFF000) == 0xF1000000) {
				# Запрещаем ломать нам USART0
				$valid = 0;
			}
			
			if ($addr == 0xF4B00000) {
				# Если сменить частоту клока - всё валится
				$valid = 0;
			}
			
			if ($addr == 0xF280020C) {
				# 0x77 irq
		#		$valid = 0;
			}
			
			if ($addr == 0xF280029C) {
				# 0x9B irq
		#		$valid = 0;
			}
			
			if ($addr == 0xF4300118) {
				# PM_WADOG
				$valid = 0;
			}
			
			if ($addr != 0xF4300118 && $addr != 0xF4B00010 && $addr != 0xF64000F8) {
				printf("WRITE %08X to %08X%s (from %08X)%s\n", $value, $addr, reg_name($addr, $value), $from, $valid ? "" : " | SKIP!!!!");
			}
			
			if ($valid) {
				cmd_write($port, $addr, $value, sub {
					$irq = shift;
				});
			} else {
				cmd_ping($port, sub {
					$irq = shift;
				});
			}
			
			my $ack = $irq != -1 ? '+' : '!';
			print F $ack; # OK
			print F chr($irq) if ($irq != -1); # IRQ_N
			
			$xuj = 0;
		} elsif ($xuj) {
			cmd_ping($port, sub {
				$irq = shift;
			});
		}
		
#		cmd_ping($port);
#		cmd_write($port, 0x80000, 0xDEADBEEF);
#		printf("%08X\n", cmd_read($port, 0x80000));
	}
	close F;
}
