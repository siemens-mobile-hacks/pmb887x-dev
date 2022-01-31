package siemens_boot;

use warnings;
use strict;
use IO::Socket;
use IO::Select;
use Device::SerialPort;
require "test.pm";

my $LOG_IO = 1;

sub boot_module_init {
	my ($port, $speed) = @_;
	
	$port->read_char_time(1000);
	$port->read_const_time(1000);
	
	::set_port_baudrate($port, 1228800);
	$port->write("OK");
	print "Wait for ack...\n";
	while ($port->read(1) ne ".") {
		$port->write("OK");
	}
	print "OK\n";
	
	cmd_ping($port);
	
	my $select = new IO::Select;
	
	my $cnt = 0;
	
	my $sock_io;
	my $sock_irq;
	while (1) {
		my $ok = 2;
		
		cmd_ping($port);
		if (!$sock_io || !$sock_io->connected) {
			$sock_io = new IO::Socket::UNIX (
				Type => SOCK_STREAM(), 
				Peer => "/dev/shm/pmb8876_io_bridge.sock", 
				Blocking => 0
			);
			--$ok;
		}
		
		cmd_ping($port);
		if (!$sock_irq || !$sock_irq->connected) {
			$sock_irq = new IO::Socket::UNIX (
				Type => SOCK_STREAM(), 
				Peer => "/dev/shm/pmb8876_io_bridge_irq.sock", 
				Blocking => 1
			);
			--$ok;
		}
		
		if ($cnt > 3000) {
			if (!$sock_io || !$sock_io->connected) {
				print "Wait for QEMU IO Bridge server...\n";
			}
			if (!$sock_irq || !$sock_irq->connected) {
				print "Wait for QEMU IRQ Bridge server...\n";
			}
			$cnt = 0;
		}
		
		++$cnt;
		
		last if ($ok == 2);
	};
	cmd_ping($port);
	
	print "IO Bridge connected!\n";
	
	$select->add($sock_io);
	
	my $last_irq = -1;
	
	my $write_irq = sub {
		my $irq = shift;
		
		if ($last_irq != $irq) {
			$last_irq = $irq;
			
			print "IRQ: 0x".sprintf("%02X", $irq)."\n" if ($irq);
			if (syswrite($sock_irq, chr($irq)) != 1) {
				die("write irq error: $!\n");
			}
		}
	};
	
	my %cmd_to_size = (
		R => 4, 
		W => 4, 
		r => 2, 
		w => 2, 
		I => 3, 
		O => 3, 
		i => 1, 
		o => 1
	);
	
	while (1) {
		my $buf = "";
		my $errors = 0;
		
		while (1) {
			my @ready = $select->can_read(0);
			my $fh = shift @ready;
			if ($fh) {
				my $len = sysread($fh, my $tmp, 1024);
				
				die "IO Bridge closed!" if (!$len);
				$buf .= $tmp;
				
				# Определяем, сколько должно было быть прочитано
				my $cmd = substr($buf, 0, 1);
				my $need_len;
				$need_len = 9 if ($cmd eq "R" || $cmd eq "r" || $cmd eq "I" || $cmd eq "i");
				$need_len = 13 if ($cmd eq "W" || $cmd eq "w" || $cmd eq "O" || $cmd eq "o");
				
				die "Unknown command '$cmd'\n" if (!$need_len);
				
				# Проверяем, нужно ли ещё прочитать
				last if (length($buf) == $need_len);
				++$errors;
				
				die "Read error (too long)" if ($errors > 10);
			} else {
				cmd_ping($port, $write_irq);
			}
		}
		
		my $cmd = substr($buf, 0, 1);
		if ($cmd eq "R" || $cmd eq "r" || $cmd eq "I" || $cmd eq "i") {
			my $addr = unpack("V", substr($buf, 1, 4));
			my $from = unpack("V", substr($buf, 5, 4));
			my $data = cmd_read_unaligned($port, $addr, $cmd_to_size{$cmd}, 1, $write_irq);
			
			syswrite($sock_io, '!'.$data);
			
			my $vv = unpack("V", $data);
			
			if ($addr != 0xF4300118) {
				printf("READ%s from %08X (%08X)%s (from %08X)\n", $cmd_to_size{$cmd} != 4 ? "[".$cmd_to_size{$cmd}."]" : "", 
					$addr, $vv, reg_name($addr, $vv), $from) if ($LOG_IO);
			}
		} elsif ($cmd eq "W" || $cmd eq "w" || $cmd eq "O" || $cmd eq "o") {
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
			
			if ($addr == 0xF4300118) {
				# PM_WADOG
				$valid = 0;
			}
			
			if ($addr != 0xF4300118) {
				printf("WRITE%s %08X to %08X%s (from %08X)%s\n", $cmd_to_size{$cmd} != 4 ? "[".$cmd_to_size{$cmd}."]" : "", 
					$value, $addr, reg_name($addr, $value), $from, $valid ? "" : " | SKIP!!!!") if ($LOG_IO);
			}
			
			if ($valid) {
				cmd_write_unaligned($port, $addr, $cmd_to_size{$cmd}, $value, $write_irq);
			} else {
				cmd_ping($port, $write_irq);
			}
			
			syswrite($sock_io, '!'); # OK
		} else {
			cmd_ping($port, $write_irq);
		}
	}
}

sub bin2hex {
	my ($hex) = @_;
	$hex =~ s/([\W\w])/sprintf("%02X", ord($1))/ge;
	return $hex;
}

1;
