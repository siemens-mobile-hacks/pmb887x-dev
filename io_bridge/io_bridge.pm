package siemens_boot;

use warnings;
use strict;
use IO::Socket;
use IO::Select;
use Device::SerialPort;
use Sie::BoardMetadata;
use Data::Dumper;

my $LOG_IO = 0;

my $BOOTLOADERS = {
	ServiceMode => [
		0xF1, 0x04, 0xA0, 0xE3, 0x20, 0x10, 0x90, 0xE5, 0xFF, 0x10, 0xC1, 0xE3, 0xA5, 0x10, 0x81, 0xE3,#0x10
		0x20, 0x10, 0x80, 0xE5, 0x1E, 0xFF, 0x2F, 0xE1, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,#0x20
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,#0x28
		
		0x53, 0x49, 0x45, 0x4D, 0x45, 0x4E, 0x53, 0x5F, 0x42, 0x4F, 0x4F, 0x54, 0x43, 0x4F, 0x44, 0x45,#0x38
		
		0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,#0x48
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,#0x50
		
		0x01, 0x04, 0x05, 0x00, 0x8B, 0x00, 0x8B		# service mode
	],
	BurninMode => [
		0xF1, 0x04, 0xA0, 0xE3, 0x20, 0x10, 0x90, 0xE5, 0xFF, 0x10, 0xC1, 0xE3, 0xA5, 0x10, 0x81, 0xE3,
		0x20, 0x10, 0x80, 0xE5, 0x1E, 0xFF, 0x2F, 0xE1, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		
		0x53, 0x49, 0x45, 0x4D, 0x45, 0x4E, 0x53, 0x5F, 0x42, 0x4F, 0x4F, 0x54, 0x43, 0x4F, 0x44, 0x45,
		
		0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		
		0x01, 0x04, 0x05, 0x80, 0x83, 0x00, 0x03		# burnin mode
	],
	NormalMode => [
		0xF1, 0x04, 0xA0, 0xE3, 0x20, 0x10, 0x90, 0xE5, 0xFF, 0x10, 0xC1, 0xE3, 0xA5, 0x10, 0x81, 0xE3,
		0x20, 0x10, 0x80, 0xE5, 0x1E, 0xFF, 0x2F, 0xE1, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		
		0x53, 0x49, 0x45, 0x4D, 0x45, 0x4E, 0x53, 0x5F, 0x42, 0x4F, 0x4F, 0x54, 0x43, 0x4F, 0x44, 0x45,
		
		0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		
		0x01, 0x04, 0x05, 0x00, 0x89, 0x00, 0x89		# normal mode
	]
};

my @mode_payload = (
	ord('A'), ord('T'),
	ord('A'), ord('T'),
	mk_boot("ServiceMode")
);

sub boot_module_init {
	my ($port, $speed) = @_;
	
	$port->read_char_time(1000);
	$port->read_const_time(1000);
	
	::set_port_baudrate($port, 115200);
	#::set_port_baudrate($port, 1600000);
	$port->write("OK");
	print "Wait for ack...\n";
	while ($port->read(1) ne ".") {
		$port->write("OK");
	}
	print "OK\n";
	
	cmd_ping($port);
	
	my $board = $ENV{BOARD} || "EL71";
	
	my $board_meta = Sie::BoardMetadata->new($board);
	my $cpu_meta = $board_meta->cpu();
	
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
			
			if ($addr == 0xF1000024 && @mode_payload) {
				$data = pack("V", shift @mode_payload);
			}
			
			if ($addr == 0xF1000068) {
				$data = pack("V", 2 | 4 | 3);
			}
			
			syswrite($sock_io, '!'.$data);
			
			my $vv = unpack("V", $data);
			
			if ($addr != 0xF4300118) {
		#		my $descr = $cpu_meta->dumpReg($addr, $vv);
		#		printf("READ%s from %08X (%08X)%s (from %08X)\n", $cmd_to_size{$cmd} != 4 ? "[".$cmd_to_size{$cmd}."]" : "", 
		#			$addr, $vv, ($descr ? " $descr" : ""), $from) if ($LOG_IO);
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
			
			if ($addr == 0xF1000020) {
				my $c = chr($value);
				if ($c =~ /[^\t\n\x20-x7e]/) {
					print "\n" if ($value == 0xFF);
					# print sprintf("\\x%02X\n", $value);
				} else {
					print $c;
				}
			}
			
			if ($addr != 0xF4300118) {
			#	my $descr = $cpu_meta->dumpReg($addr, $value);
			#	printf("WRITE%s %08X to %08X%s (from %08X)%s\n", $cmd_to_size{$cmd} != 4 ? "[".$cmd_to_size{$cmd}."]" : "", 
			#		$value, $addr, ($descr ? " $descr" : ""), $from, $valid ? "" : " | SKIP!!!!") if ($LOG_IO);
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

sub cmd_write {
	my ($port, $addr, $value, $irq_handler) = @_;
	return cmd_write_unaligned($port, $addr, 4, $value, $irq_handler);
}

sub cmd_write_unaligned {
	my ($port, $addr, $size, $value, $irq_handler) = @_;
	
	my $cmd = "W"; # 4
	$cmd = "O" if ($size == 3);
	$cmd = "w" if ($size == 2);
	$cmd = "o" if ($size == 1);
	
	$port->write($cmd.chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
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
	return cmd_read_unaligned($port, $addr, 4, $raw, $irq_handler);
}

sub cmd_read_unaligned {
	my ($port, $addr, $size, $raw, $irq_handler) = @_;
	
	my $cmd = "R"; # 4
	$cmd = "I" if ($size == 3);
	$cmd = "r" if ($size == 2);
	$cmd = "i" if ($size == 1);
	
	$port->write($cmd.chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF));
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

sub mk_boot {
	my ($name) = @_;
	
	my $boot = $BOOTLOADERS->{$name};
	my $len = scalar(@$boot);
	
	my @payload = (0x30, $len & 0xFF, ($len >> 8) & 0xFF);
	
	# XOR
	my $chk = 0;
	for (my $i = 0; $i < $len; ++$i) {
		my $c = $boot->[$i];
		$chk ^= $c;
		push @payload, $c;
	}
	
	push @payload, $chk;
	
	return @payload;
}

1;
