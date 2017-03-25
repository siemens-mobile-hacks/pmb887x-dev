package SieCGSN;

use warnings;
use strict;
use Device::SerialPort;
use Getopt::Long;
use File::Basename qw|dirname|;
use Time::HiRes qw|usleep time|;
use Linux::Termios2;
use POSIX qw( :termios_h );
use Data::Dumper;
no utf8;

# AT+CGSN:A0000000,00000008
# Read data from memory address A0000000 Lenght 8 Bytes (If Size is not specified then 
# read 4 bytes)

# AT+CGSN@A08E8DE4,0000001A,00004225
# Call address A08E8DE4, R0 = 0000001A, R1 = 00004225 (R0-R7 Can be given)
# Out is the R0-R12 Registers Dump and CPSR

# AT+CGSN*A80B180011223344....
# Write Data to RAM. address A80B1800 
# Max Size 128 bytes per command 

# AT+CGSN%A0000000A0000004A0000008....
# Query addresses. Return values for each address in one line.

sub new($$) {
	my($class, $port) = @_;
	
	my $self = {
		port => $port
	};
	
	bless $self, $class;
	
	return $self;
}

sub connect($$) {
	my ($self, $max_speed) = @_;
	
	my $ok = 0;
	for my $speed (921600, 576000, 500000, 460800, 230400, 115200, 57600) {
		next if ($speed > $max_speed);
		
		$self->debug("Testing $speed - ");
		
		my $i;
		for ($i = 0; $i < 10; ++$i) {
			$self->setSpeed($speed);
			my $res = $self->sendAt("ATE0\r", 100);
			if ($res->{code}) {
				$ok = 1;
				last;
			}
		}
		
		if ($ok) {
			$self->debug("OK (with $i tries)\n");
			return 1;
		} else {
			$self->debug("FAIL\n");
		}
	}
	return;
}

sub CPSR {
	my ($self, $cpsr) = @_;
	
	my $M = $cpsr & 0x1F;
	my $T = $cpsr >> 5 & 0xF;
	my $F = $cpsr >> 6 & 0xF;
	my $I = $cpsr >> 7 & 0xF;
	my $A = $cpsr >> 8 & 0xF;
	my $E = $cpsr >> 9 & 0xF;
	my $IT1 = $cpsr >> 10 & 0x3F;
	my $GE = $cpsr >> 16 & 0xF;
	my $DNM = $cpsr >> 20 & 0xF;
	my $J = $cpsr >> 24 & 0xF;
	my $IT2 = $cpsr >> 25 & 0x3;
	my $Q = $cpsr >> 27 & 0xF;
	my $V = $cpsr >> 28 & 0xF;
	my $C = $cpsr >> 29 & 0xF;
	my $Z = $cpsr >> 30 & 0xF;
	my $N = $cpsr >> 31 & 0xF;
	
	$M = sprintf("%02X", $M);
	$M = "USR" if ($M eq '10');
	$M = "FIQ" if ($M eq '11');
	$M = "IRQ" if ($M eq '12');
	$M = "SVC" if ($M eq '13');
	$M = "ABR" if ($M eq '17');
	$M = "UND" if ($M eq '1B');
	$M = "SYS" if ($M eq '1F');
	$M = "MSE" if ($M eq '16');
	
	my $value = [$M];
	push @$value, "Thumb" if ($T);
	push @$value, "NO_FIQ" if ($F);
	push @$value, "NO_IRQ" if ($I);
	push @$value, "A" if ($A);
	push @$value, "E" if ($E);
	push @$value, sprintf("IT(%02X, %02X)", $IT1, $IT2) if ($IT1 || $IT2);
	push @$value, sprintf("GE(%02X)", $GE) if ($GE);
	push @$value, sprintf("DNM(%02X)", $DNM) if ($DNM);
	push @$value, "Jazzele" if ($J);
	push @$value, "Q" if ($Q);
	push @$value, "V" if ($V);
	push @$value, "C" if ($C);
	push @$value, "Z" if ($Z);
	push @$value, "N" if ($N);
	
	return join(" ", @$value);
}

sub getDeviceKey {
	my $self = shift;
	
	my $vendor = $self->sendAt("AT+CGMI\r", 100);
	my $model = $self->sendAt("AT+CGMM\r", 100);
	my $sw = $self->sendAt("AT+CGMR\r", 100);
	
	return ($vendor && $vendor->{code} ? $vendor->{data} : "UNKNOWN").":".
		($model && $model->{code} ? $model->{data} : "UNKNOWN").":".
		($sw && $sw->{code} ? $sw->{data} : "UNKNOWN");
}

sub execBin($$$$) {
	my ($self, $addr, $bin, @in_regs) = @_;
	$bin = hex2bin($bin) if (ref($bin) eq "ARRAY");
	$self->writeMem($addr, $bin);
	return $self->exec($addr, @in_regs);
}

sub exec($$$) {
	my ($self, $addr, @in_regs) = @_;
	
	my $port = $self->{port};
	
	my $cmd_regs = [];
	for my $reg (@in_regs) {
		push @$cmd_regs, sprintf("%08X", $reg)if (defined($reg));
	}
	
	my $res = $self->sendAt(sprintf("AT+CGSN@%08X%s\r", $addr, @$cmd_regs ? ",".join(",", @$cmd_regs) : ""));
	if ($res && $res->{code}) {
		my ($ack, @regs) = unpack("WVVVVVVVVVVVVVV", $res->{data});
		if ($ack == 0xA1) {
			printf(
				"[%s] sub_%08X(".join(",", @$cmd_regs)."):\n".
				"  CPSR: %s\n".
				"   R0=%08X     R1=%08X     R2=%08X      R3=%08X\n".
				"   R4=%08X     R5=%08X     R6=%08X      R7=%08X\n".
				"   R8=%08X     R9=%08X    R10=%08X     R11=%08X\n".
				"  R12=%08X\n", 
				$addr % 4 ? "thumb" : "arm", $addr, $self->CPSR($regs[13]), 
				$regs[0], $regs[1], $regs[2], $regs[3], 
				$regs[4], $regs[5], $regs[6], $regs[7], 
				$regs[8], $regs[9], $regs[10], $regs[11], 
				$regs[12]
			);
			
			return {
				CPSR => $regs[13], 
				R0 => $regs[0], R1 => $regs[1], R2 => $regs[2], R3 => $regs[3], 
				R4 => $regs[4], R5 => $regs[5], R6 => $regs[6], R7 => $regs[7], 
				R8 => $regs[8], R9 => $regs[9], R10 => $regs[10], R11 => $regs[11], 
				R12 => $regs[12]
			};
		} else {
			printf("ERROR: Exec addr %08X: ACK err (received ack=%02X)\n", $addr, $ack);
			print $self->bin2hex($res->{data})."\n";
			exit(1);
		}
	} else {
		if ($res) {
			printf("ERROR: Exec addr %08X: AT CMD ERR\n", $addr);
			print $self->bin2hex($res->{data})."\n";
			exit(1);
		} else {
			printf("ERROR: Exec addr %08X: AT SEND ERR (NO ANSWER :()\n", $addr);
			exit(1);
		}
	}
}

sub writeWord($$$) {
	my ($self, $addr, $data) = @_;
	return $self->writeMem($addr, $self->int2bin([$data]));
}

sub writeMem($$$) {
	my ($self, $addr, $data) = @_;
	
	my $port = $self->{port};
	
	my $size = length($data);
	if ($addr % 4 != 0) {
		my $old_addr = $addr;
		$addr = $addr - ($addr % 4);
		$size += ($old_addr - $addr);
		
		# Читаем в начало добавленные для выравнивания адреса байты
		$data = cgsn_read($port, $addr, $old_addr - $addr).$data;
	}
	
	if ($size % 4 != 0) {
		my $old_size = $size;
		$size = $size - ($size % 4) + 4;
		
		# Читаем в конец добавленные для выравнивания размера байты
		$data .= cgsn_read($port, $addr + $size, $size - $old_size);
	}
	
	my $offset = 0;
	my $max_size = 128;
	while ($size > 0) {
		my $tries = 5;
		while (1) {
			my $chunk = $size < $max_size ? $size : $max_size;
			my $res = $self->sendAt(sprintf("AT+CGSN*%08X%s\r", $addr, $self->bin2hex(substr($data, $offset, $chunk))), 1000);
			if ($res && $res->{code}) {
				my $chk = ord(substr($res->{data}, 0, 1));
				if ($chk != 0xA1) {
					printf("ERROR: Write mem %08X, %08X ACK err (received ack=%02X)\n", $addr, $chunk, $chk);
					print "ANSWER: ".$self->bin2hex($res->{data})."\n";
					exit(1) if (!$tries--);
					next;
				}
			} else {
				if ($res) {
					printf("ERROR: Write mem %08X, %08X AT CMD ERR\n", $addr, $chunk);
					print "ANSWER: ".$self->bin2hex($res->{data})."\n";
					exit(1) if (!$tries--);
					next;
				} else {
					next if ($tries--);
					printf("ERROR: Write mem %08X, %08X AT SEND ERR (NO ANSWER :()\n", $addr, $chunk);
					exit(1);
				}
			}
			last;
		}
		$offset += $max_size;
		$addr += $max_size;
		$size -= $max_size;
	}
}

sub readMem($$$) {
	my ($self, $addr, $size) = @_;
	
	my $port = $self->{port};
	
	my $skip_after = 0;
	my $skip_before = 0;
	
	if ($addr % 4 != 0) {
		my $old_addr = $addr;
		$addr = $addr - ($addr % 4);
		$skip_before = $old_addr - $addr;
		$size += ($old_addr - $addr);
	}
	
	if ($size % 4 != 0) {
		my $old_size = $size;
		$size = $size - ($size % 4) + 4;
		$skip_after = $size - $old_size;
	}
	
	my $buffer = "";
	my $max_size = 128;
	while ($size > 0) {
		my $tries = 5;
		while (1) {
			my $chunk = $size < $max_size ? $size : $max_size;
			my $res = $self->sendAt(sprintf("AT+CGSN:%08X,%08X\r", $addr, $chunk), 2000);
			if ($res && $res->{code}) {
				my $chk = ord(substr($res->{data}, 0, 1));
				my $data = substr($res->{data}, 1, $chunk);
				
				if ($chk == 0xA1) {
					if ($chunk == length($data)) {
					#	printf("%08X: %s\n", $addr, $self->bin2hex($data));
						$buffer .= $data;
					} else {
						printf("ERROR: Read mem %08X, %08X size err (received=%08X)\n", $addr, $chunk, length($data));
						print "ANSWER: ".$self->bin2hex($res->{data})."\n";
						exit(1) if (!$tries--);
						next;
					}
				} else {
					printf("ERROR: Read mem %08X, %08X ACK err (received ack=%02X)\n", $addr, $chunk, $chk);
					print $self->bin2hex($res->{data})."\n";
					exit(1) if (!$tries--);
					next;
				}
			} else {
				if ($res) {
					printf("ERROR: Read mem %08X, %08X AT CMD ERR\n", $addr, $chunk);
					print "ANSWER: ".$self->bin2hex($res->{data})."\n";
					exit(1) if (!$tries--);
					next;
				} else {
					next if ($tries--);
					printf("ERROR: Read mem %08X, %08X AT SEND ERR (NO ANSWER :()\n", $addr, $chunk);
					exit(1);
				}
			}
			last;
		}
		$addr += $max_size;
		$size -= $max_size;
	}
	
	if ($skip_after || $skip_before) {
		# Обрезаем ровно столько, сколько запросили
		return substr($buffer, $skip_before, length($buffer) - ($skip_before + $skip_after));
	}
	
	return $buffer;
}

sub sendAt($$$) {
	my ($self, $command, $timeout) = @_;
	
	my $port = $self->{port};
	
	$timeout = defined($timeout) ? $timeout : -1;
	
	$port->purge_all;
	$port->write($command);
	$port->write_drain;
	
	my $buf;
	my $end = time + $timeout / 1000;
	while ($timeout == -1 || time <= $end) {
		my ($count, $c) = $port->read(1);
		if ($count) {
			$buf .= $c;
			if ($buf =~ /(OK|ERROR)\r\n/) {
				my $status = $1;
				my $data =  substr($buf, 0, length($buf) - (length($1) + 4));
				$data =~ s/^\r\n|\r\n$//g; # trim \r\n
				
				return {
					code => $status eq "OK" ? 1 : 0, 
					data => $data
				};
			}
		}
	}
	
	return;
}

sub setSpeed($$) {
	my ($self, $baudrate) = @_;
	my $termios = Linux::Termios2->new;
	$termios->getattr($self->{port}->FILENO);
	$termios->setospeed($baudrate);
	$termios->setispeed($baudrate);
	$termios->setattr($self->{port}->FILENO, TCSANOW);
}

sub loadConfig {
	my ($self, $file, $key) = @_;
	my $cfg = _parseIni(_readFile($file));
	return exists $cfg->{$key} ? $cfg->{$key} : undef;
}

sub debug {
	my ($self, $text) = @_;
	print $text;
}

# utils
sub _readFile {
	my $file = shift;
	
	my $raw = "";
	open(F, "<$file") or die("open($file): $!");
	while (!eof(F)) {
		my $buff;
		read F, $buff, 2048;
		$raw .= $buff;
	}
	close(F);
	
	return $raw;
}

sub _parseIni {
	my $ini = shift;
	my $hash = {};
	my $ref;
	for my $line (split(/(\r\n|\n|\r)/, $ini)) {
		$line =~ s/;.*?$//g; # комментарии
		$line =~ s/^\s+|\s+$//g;
		
		if ($line =~ /^([^=]+)=(.*?)$/ && $ref) {
			my ($k, $v) = ($1, $2);
			$k =~ s/^\s+|\s+$//g;
			$v =~ s/^\s+|\s+$//g;
			
			$v = int $v if ($v =~ /^(\d+)$/i);
			$v = hex $v if ($v =~ /^(0x[a-f0-9]+)$/i);
			
			$ref->{$k} = $v;
		} elsif ($line =~ /^\[(.*?)\]$/) {
			$hash->{$1} = {};
			$ref = $hash->{$1};
		}
	}
	return $hash;
}

sub hex2addr {
	my $arg = shift;
	if (defined($arg) && $arg =~ /^(0x)?([a-f0-9]+)$/i) {
		return hex($2);
	}
	warn "Unknown hex value: $arg\n";
	return;
}

sub int2bin {
	my ($self, $arr) = @_;
	my $bin = "";
	for my $i (@$arr) {
		$bin .= pack("V", $i);
	}
	return $bin;
}

sub bin2hex {
	my ($self, $hex) = @_;
	$hex =~ s/([\W\w])/sprintf("%02X", ord($1))/ge;
	return $hex;
}

sub hex2bin {
	my $hex = shift;
	
	$hex = join("", @$hex) if (ref($hex) eq "ARRAY");
	
	$hex =~ s/\s+//gim;
	$hex = "0$hex" if (length($hex) % 2 != 0);
	$hex =~ s/([A-F0-9]{2})/chr(hex($1))/gie;
	return $hex;
}

1;
