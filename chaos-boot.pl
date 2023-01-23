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

$| = 1;

main();

sub main {
	my $help = 0;
	my $device = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0";
	my $boot_speed = 115200;
	my $speed = 1600000;
	my $ign = 1;
	my $dtr = -1;
	my $rts = 0;
	my $flasher = [];
	my $exec_linux = "../linux/arch/arm/boot/Image";
	my $exec_linux_dtb = "../linux/arch/arm/boot/dts/siemens-el71.dtb";
	my $exec_file;
	my $exec_addr = '0xA8000000';
	my $as_hex = 0;
	my $run_picocom = 0;
	my $read_OTP = 0;
	my $ign2 = 0;
	
	my $err = get_argv_opts({
		"device=s"		=> \$device, 
		"boot-speed=s"	=> \$boot_speed, 
		"speed=s"		=> \$speed, 
		"ign"			=> \$ign, 
		"ign2"			=> \$ign2, 
		"dtr"			=> \$dtr, 
		"rts"			=> \$rts, 
		"\@flasher=s"	=> $flasher, 
		"help"			=> \$help, 
		"hex"			=> \$as_hex, 
		"picocom"		=> \$run_picocom, 
		"linux=s"		=> \$exec_linux, 
		"linux-dtb=s"	=> \$exec_linux_dtb, 
		"exec=s"		=> \$exec_file, 
		"exec-addr=s"	=> \$exec_addr, 
		"dump-otp"		=> \$read_OTP
	});
	
	$ign = 2 if ($ign2);
	
	$dtr = ($ign == 2) if ($dtr == -1);
	
	$exec_addr = parse_addr($exec_addr);
	
	if ($err || $help) {
		print "$err\n";
		print join("\n", (
			'Common options:',
			'	--device=/dev/ttyUSB3    com port device',
			'	--boot-speed=112500      boot speed',
			'	--speed=1600000          speed after boot',
			'	--ign                    autoignition (1=normal, 2=vova7890 lazy ass ignition)',
			'	--dtr                    up dtr pin (for noname DCA-500)',
			'	--rts                    up dtr pin (for noname DCA-500)',
			'',
			'Exec options:',
			'	--hex                    dump output as hex',
			'	--picocom                run picocom after exec',
			'	--exec-addr <file>       change exec addr (default: 0xA8008000)',
			'	--exec <file>            upload and run <file>',
			'',
			'Misc:',
			'	--dump-otp				dump otp region',
			'',
			'Flasher:',
			'	--flasher read,<addr>,<size>,<file>        read <size> bytes in flash/ram at <addr> and save to <file>',
			'	--flasher write,<addr>,<file>              write <file> to flash/ram at <addr>'
		));
		print "\n";
		exit(1);
	}
	
	# Парсим и проверяем таски флешера заранее
	my $flasher_tasks = [];
	for my $task (@$flasher) {
		my @args = split(/\s*,\s*/, $task);
		my $cmd = lc($args[0]);
		
		if ($cmd eq "read") {
			my ($addr, $size, $file) = (parse_addr($args[1]), parse_addr($args[2]), $args[3]);
			
			die "Unknown addr in command: `$task`\n"
				if (!defined($addr));
			die "Unknown size in command: `$task`\n"
				if (!defined($size));
			die "Unknown file in command: `$task`\n"
				if (!defined($file));
			
			push @$flasher_tasks, {
				cmd => $cmd, 
				addr => $addr, 
				size => $size, 
				file => $file
			};
		} elsif ($cmd eq "write" or $cmd eq "erase") {
			my ($addr, $file) = (parse_addr($args[1]), $args[2]);
			
			die "Unknown addr in command: `$task`\n"
				if (!defined($addr));
			die "Unknown file in command: `$task`\n"
				if (!defined($file));
			
			push @$flasher_tasks, {
				cmd => $cmd, 
				addr => $addr, 
				file => $file
			};
		} else {
			die "Unknown flasher command: $task\n";
		}
	}
	
	my $port = Device::SerialPort->new($device);
	die("open port error ($device)") if (!$port);

	$port->baudrate($boot_speed);
	
	$port->read_char_time(0);
	$port->read_const_time($ign ? 20 : 100);
	
	if ($ign) {
		$port->dtr_active($ign == 2);
		$port->rts_active($rts);
	} else {
		$port->dtr_active($dtr);
		$port->rts_active($rts);
	}
	
	$SIG{INT} = $SIG{TERM} = sub {
		$port->dtr_active($ign == 2);
		$port->rts_active(0);
		exit(0);
	};
	
	$port->write_settings;
	if ($ign) {
		while (readb($port) != -1) { }
	}
	
	print "Please, short press red button!\n";
	
	my $last_dtr_val = 0;
	my $last_dtr = 0;
	my $last_dtr_timeout = 0.5;
	my $read_zero = 0;
	my $boot_ok = 0;
	while (1) {
		if ($ign) {
			if (time - $last_dtr > $last_dtr_timeout || $read_zero > 0) {
				$last_dtr_val = 0 if ($read_zero > 0); # поверофнулся типа
				
				$last_dtr_timeout = $last_dtr_val ? 1.5 : 0.5;
				$last_dtr_val = !$last_dtr_val;
				$last_dtr = time;
				$read_zero = 0;
				$port->dtr_active($ign == 2 ? !$last_dtr_val : $last_dtr_val);
				
				print "^" if ($last_dtr_val);
			}
		}
		
		$port->write("AT");
		
		my $c = readb($port);
		if ($c == 0xB0 || $c == 0xC0) {
			$port->dtr_active($dtr) if ($ign);
			
			print "\n";
			print "SGOLD detected!\n" if ($c == 0xB0);
			print "NewSGOLD detected!\n" if ($c == 0xC0);
			
			$port->read_char_time(200);
			$port->read_const_time(200);
			
			print "Sending boot...\n";
			my $boot = mk_chaos_boot();
			write_boot($port, $boot);
			
			$c = readb($port);
			if ($c == 0xA5) {
				usleep(200 * 1000);
				
				# Странная проверка в PV Буте
				$port->write("\x55");
				$c = readb($port);
				
				if ($c != 0xAA) {
					printf("Boot init error (answer=%02X)\n", $c);
					exit(1);
				}
				
				chaos_ping($port) || exit(1);
				chaos_keep_alive($port);
				if ($speed != $boot_speed) {
					chaos_set_speed($port, $speed) or exit(1);
				}
				chaos_keep_alive($port);
				print "Chaos Bootloader - OK\n";
				
				if ($read_OTP) {
					my $opt = chaos_read_opt($port);
					print "OTP(".length($opt)."): ".bin2hex($opt)."\n";
					exit;
				}
				
				# Мини-флешер
				if (@$flasher) {
					my $info = chaos_read_info($port);
					die "Can't read phone info!\n" if (!$info);
					
					printf("Phone: %s %s, IMEI: %s, Flash: %d Mb (%04X:%04X)\n", $info->{vendor}, $info->{model}, $info->{imei}, 
						$info->{flash}->{size} / 1024 / 1024, 
						$info->{flash}->{type} & 0xFFFF, ($info->{flash}->{type} >> 16) & 0xFFFF);
					
					print "\n";
					for my $task (@$flasher_tasks) {
						# Чтение RAM и FLASH
						if ($task->{cmd} eq "read") {
							my $space_name = $task->{addr} >= $info->{flash}->{base} && $task->{addr} <= $info->{flash}->{base} + $info->{flash}->{size} ? "FLASH" : "RAM";
							printf("Read %d bytes from %08X (%s) to '%s'\n", $task->{size}, $task->{addr}, $space_name, $task->{file});
							
							chaos_keep_alive($port);
							my $res = chaos_read_ram($port, $task->{addr}, $task->{size}, 1024 * 3.5);
							if (defined($res)) {
								open (F, ">".$task->{file})
									or die("open(".$task->{file}."): $!");
								binmode F;
								print F $res;
								close F;
							}
						}
						
						# Запись RAM и FLASH
						elsif ($task->{cmd} eq "write") {
							# Адресное пространство флеша
							if ($task->{addr} >= $info->{flash}->{base} && $task->{addr} <= $info->{flash}->{base} + $info->{flash}->{size}) {
								die "NOT SUPPORTED YET :( PLZ, GO PINAT' MENYA IF YOU WANT THIS FEATURE\n";
								
								my $raw = read_file($task->{file});
								
								chaos_keep_alive($port);
								chaos_write_flash($port, $task->{addr}, $raw, 1024 * 3.5);
								printf("Write %s to FLASH (%08X)\n", $task->{file}, $task->{addr});
							}
							# Иначе считает адресным пространством RAM
							else {
								printf("Write %s to RAM (%08X)\n", $task->{file}, $task->{addr});
								my $raw = read_file($task->{file});
								
								chaos_keep_alive($port);
								chaos_write_ram($port, $task->{addr}, $raw, 1024 * 3.5);
							}
						} else {
							die "Unknown flasher command: $task\n";
						}
						print "\n";
					}
					print "\n";
				}
				
				# Запуск файла в RAM
				if ($exec_file) {
					my $raw = read_file($exec_file);
					chaos_keep_alive($port);
					
					printf("Load $exec_file to RAM (%08X)... (size=%d)\n", $exec_addr, length($raw));
					chaos_write_ram($port, $exec_addr, $raw, 1024 * 3.5) or die("load error");
					
					printf("Exec %08X...\n", $exec_addr);
					chaos_goto($port, $exec_addr);
				}
				
				# Run Linux from RAM
				if ($exec_linux) {
					die "Invalid kernel addr" if (($exec_addr & 0xFFFFF) != 0);
					
					my $preloader_addr = $exec_addr;
					my $kernel_addr = $exec_addr + 0x8000;
					
					my $dtb = read_file($exec_linux_dtb);
					my $kernel = read_file($exec_linux);
					
					my $dtb_addr = $kernel_addr + length($kernel);
					
					my $align = (1 << 20) * 3;
					$dtb_addr += ($align - ($dtb_addr % $align)) if ($dtb_addr % $align);
					
					my $preloader = join("", 
						hex2bin("0000A0E310808FE2001098E50C808FE2002098E51C804FE202F988E2"), # linux_boot.S
						pack("V", 0xFFFFFFFF), # machine_id
						pack("V", $dtb_addr), # dts/atags
					);
					
					chaos_keep_alive($port);
					
					print "Exec linux!\n";
					print "Kernel: $exec_linux [".sprintf("0x%08X", $kernel_addr)."]\n";
					print "DTB: $exec_linux_dtb [".sprintf("0x%08X", $dtb_addr)."]\n";
					print "\n";
					
					printf("Load preloader to RAM (%08X)... (size=%d)\n", $preloader_addr, length($preloader));
					chaos_write_ram($port, $preloader_addr, $preloader, 1024 * 4) or die("load error");
					print "\n";
					
					printf("Load dtb to RAM (%08X)... (size=%d)\n", $dtb_addr, length($dtb));
					chaos_write_ram($port, $dtb_addr, $dtb, 1024 * 4) or die("load error");
					print "\n";
					
					printf("Load kernel to RAM (%08X)... (size=%d)\n", $kernel_addr, length($kernel));
					chaos_write_ram($port, $kernel_addr, $kernel, 1024 * 4) or die("load error");
					
					printf("Exec %08X...\n", $preloader_addr);
					chaos_goto($port, $preloader_addr);
				}
			} else {
				printf("Invalid answer: %02X\n", $c);
				printf("Chaos bootloader not found!\n\n");
				next;
			}
			
			if ($run_picocom) {
				system("picocom -b $speed $device");
				exit;
			}
			
			if ($as_hex) {
				while (($c = readb($port)) != 0 || 1) {
					if ($c > -1) {
						my $str = chr($c);
						printf("%s | %02X\n", ($str =~ /[[:print:]]/ ? "'".$str."'" : " ? "), $c);
					}
				}
			} else {
				while (($c = readb($port)) != 0) {
					print chr($c) if ($c > -1);
				}
			}
			last;
		} elsif ($c == 0) {
			++$read_zero;
		}
		print ".";
	}
}

sub read_file {
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

sub get_argv_opts {
	my $cfg = shift;
	
	my $args = {};
	for my $k (keys %$cfg) {
		my $arg = {ref => $cfg->{$k}};
		if ($k =~ /^@(.*?)$/) {
			$k = $1;
			$arg->{array} = 1;
		}
		
		if ($k =~ /^([^=]+)=(.*?)$/) {
			$k = $1;
			$arg->{with_value} = $2;
		}
		
		$args->{$k} = $arg;
	}
	
	for (my $opt_id = 0; $opt_id < scalar(@ARGV); ++$opt_id) {
		my $opt = $ARGV[$opt_id];
		my $opt_name;
		my $opt_value;
		
		if ($opt =~ /^--([^=]+)=(.*?)$/) {
			$opt_name = $1;
			$opt_value = $2;
		} elsif ($opt =~ /^--([^=]+)$/) {
			$opt_name = $1;
		}
		
		if (exists $args->{$opt_name}) {
			my $arg = $args->{$opt_name};
			
			if ($arg->{with_value}) {
				if (!defined($opt_value)) {
					++$opt_id;
					$opt_value = $ARGV[$opt_id] if (exists $ARGV[$opt_id]);
				}
				return "Argument $opt require value\n" if (!defined($opt_value));
				
				if ($arg->{with_value} eq "b") {
					if ($opt_value eq "true") {
						$opt_value = 1;
					} elsif ($opt_value eq "false") {
						$opt_value = 0;
					} else {
						$opt_value = int($opt_value) ? 1 : 0;
					}
				} elsif ($arg->{with_value} eq "i") {
					if ($opt_value =~ /0x([a-f0-9]+)/) {
						$opt_value = hex($1);
					} else {
						$opt_value = int($opt_value);
					}
				}
			} else {
				$opt_value = 1;
			}
			
			if ($arg->{array}) {
				push @{$arg->{ref}}, $opt_value;
			} else {
				${$arg->{ref}} = $opt_value;
			}
		} else {
			return "Unknown option: $opt\n";
		}
	}
	
	return;
}

sub chaos_read_opt {
	my ($port) = @_;
	return chaos_read_ram($port, 0xA0000000, 0x200, 4, "O");
}

sub chaos_read_info {
	my ($port) = @_;
	$port->write("I");
	
	my ($count, $raw) = $port->read(128);
	if ($count != 128) {
		warn sprintf("[chaos_read_info] Invalid answer size (%d != 128)\n", $count);
		return 0;
	}
	
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
	
	my ($model, $vendor, $imei, $hash, $flash_base, $reserved0, 
		$flash_type, $flash_size, $flash_buffer_size, $flash_regions, 
		$flash_region0_nblocsk, $flash_region0_size, $flash_region1_nblocsk, $flash_region1_size, $reserved1) = 
			unpack("Z16 Z16 Z16 H32 V a12 V W v W v v v v a32", $raw);
	
	my $info = {
		model => $model, 
		vendor => $vendor, 
		imei => $imei, 
		hash => $hash, 
		flash => {
			base => $flash_base, 
			type => $flash_type, 
			size => 1 << $flash_size, 
			write_buff_size => $flash_buffer_size, 
			regions => $flash_regions, 
			region0 => {
				blocks => $flash_region0_nblocsk, 
				size => $flash_region0_size
			}, 
			region1 => {
				blocks => $flash_region1_nblocsk, 
				size => $flash_region1_size
			}
		}
	};
	
	return $info;
}

sub chaos_ping {
	my ($port) = @_;
	$port->write("A");
	my $c = readb($port);
	if ($c != 0x52) {
		warn sprintf("[chaos_ping] Invalid answer 0x%02X\n", $c);
		return 0;
	}
	return 1;
}

sub chaos_keep_alive {
	my ($port) = @_;
	$port->write(".");
	return 1;
}

sub chaos_set_speed {
	my ($port, $speed) = @_;
	
	my %CHAOS_SPEEDS = (
		57600 => 0x00, 
		115200 => 0x01, 
		230400 => 0x02, 
		460800 => 0x03, 
		614400 => 0x04, 
		921600 => 0x05, 
		1228800 => 0x06, 
		1600000 => 0x07, 
		1500000 => 0x08
	);
	
	if (!exists($CHAOS_SPEEDS{$speed})) {
		warn("Invalid speed $speed! Allowed: ".split(", ", keys(%CHAOS_SPEEDS)));
		return 0;
	}
	
	my $old_speed = $port->baudrate;
	$port->write("H".chr($CHAOS_SPEEDS{$speed}));
	my $c = readb($port);
	my $step = 0;
	if ($c == 0x68) {
		++$step;
		set_port_baudrate($port, $speed);
		$port->write("A");
		$c = readb($port);
		if ($c == 0x48) {
			# Успешно установили скорость
			return 1;
		}
	}
	set_port_baudrate($port, $old_speed);
	warn sprintf("[chaos_set_speed] Invalid answer 0x%02X (step=%d)", $c, $step);
	return 0;
}

sub chaos_goto {
	my ($port, $addr) = @_;
	
	$addr = chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF);
	
	while (1) {
		$port->write("G$addr$addr$addr");
		my $data = $port->read(4);
		
		if (length($data) < 4) {
			warn sprintf("[chaos_goto] Invalid answer 0x%s", bin2hex($data));
			
			if (ord(substr($data, 0, 1)) == 0xAA) { # CRC error
				chaos_keep_alive($port);
				next;
			}
			exit(1);
		} else {
			$data = substr($data, 3, 1).substr($data, 2, 1).substr($data, 1, 1).substr($data, 0, 1);
			if ($data ne $addr) {
				warn sprintf("[chaos_goto] Addr corrupted o_O 0x%s", bin2hex($data));
			}
		}
		last;
	}
	return 1;
}

sub chaos_read_ram {
	my ($port, $read_addr, $read_size, $chunk, $cmd) = @_;
	
	$cmd = $cmd || 'R';
	
	# Сразу заранее нарежем на блоки
	my @blocks = ();
	for (my $j = 0; $j < $read_size; $j += $chunk) {
		my $addr = $read_addr + $j;
		my $size = $chunk > $read_size - $j ? $read_size - $j : $chunk;
		
		push @blocks, [
			$addr, $size, 
			"$cmd".
			chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
			chr(($size >> 24) & 0xFF).chr(($size >> 16) & 0xFF).chr(($size >> 8) & 0xFF).chr($size & 0xFF)
		];
	}
	
	my $i = 0;
	my $start = time;
	my $buffer = "";
	
	for my $block (@blocks) {
		my $addr = $block->[0];
		my $size = $block->[1];
		
		if ($i % 10 == 0) {
			printf("                                                    \r");
			printf("#$i %02d%s [READ] %08X-%08X (%.02f Kbps)\r", int(($addr - $read_addr) / $read_size * 100), "%", $addr, $addr + $size, (($addr - $read_addr) / 1024) / (time - $start));
		}
		
		my $tries = 10;
		while (1) {
			$port->write($block->[2]);
			
			my $buf = $port->read($block->[1] + 4) || "";
			
			my $ok = substr($buf, $block->[1], 2);
			my $chk = (ord(substr($buf, $block->[1] + 3, 1)) << 8) | ord(substr($buf, $block->[1] + 2, 1));
			
			if ($ok ne "OK") {
				warn sprintf("\n[chaos_read_ram] Invalid answer '%02X%02X'", ord(substr($ok, 0, 1)), ord(substr($ok, 1, 1)));
				if ($tries--) {
					chaos_ping($port) || exit(1);
					next;
				}
				exit(1);
			}
			
			$buf = substr($buf, 0, $block->[1]);
			
			my $own_chk = 0;
			for (my $i = 0; $i < $block->[1]; ++$i) {
				$own_chk ^= ord(substr($buf, $i, 1));
			}
			
			if ($chk != $own_chk) {
				warn sprintf("\n[chaos_read_ram] Invalid CRC %02X != %02X", $chk, $own_chk);
				if ($tries--) {
					chaos_ping($port) || exit(1);
					next;
				}
				exit(1);
			}
			
			$buffer .= $buf;
			
			last;
		}
		++$i;
	}
	
	return $buffer;
}

sub chaos_write_ram {
	my ($port, $dst_addr, $buff, $chunk) = @_;
	
	my @blocks = ();
	my $buff_size = length($buff);
	for (my $j = 0; $j < $buff_size; $j += $chunk) {
		my $tmp = substr($buff, $j, $chunk);
		
		my $chk;
		my $size = length($tmp);
		for (my $i = 0; $i < $size; ++$i) {
			$chk ^= ord(substr($tmp, $i, 1));
		}
		my $addr = $dst_addr + $j;
		
		push @blocks, [
			$addr, $size, 
			"W".
			chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
			chr(($size >> 24) & 0xFF).chr(($size >> 16) & 0xFF).chr(($size >> 8) & 0xFF).chr($size & 0xFF).
			$tmp.chr($chk)
		];
	}
	
	my $i = 0;
	my $start = time;
	for my $block (@blocks) {
		my $addr = $block->[0];
		my $size = $block->[1];
		
		if ($i % 10 == 0) {
			printf("                                                    \r");
			printf("#$i %02d%s [WRITE] %08X-%08X (%.02f Kbps)\r", int(($addr - $dst_addr) / $buff_size * 100), "%", $addr, $addr + $size, (($addr - $dst_addr) / 1024) / (time - $start));
		}
		
		my $tries = 999999;
		while (1) {
			$port->write($block->[2]);
		
			my $ok = $port->read(2);
			if ($ok ne "OK") {
				warn sprintf("\n[chaos_write_ram] Invalid answer '%02X%02X'", ord(substr($ok, 0, 1)), ord(substr($ok, 1, 1)));
				if ($tries--) {
					chaos_keep_alive($port);
					next;
				}
				exit(1);
			}
			last;
		}
		++$i;
	}
	print "\n";
	return 1;
}

sub chaos_write_flash {
	my ($port, $dst_addr, $buff, $chunk) = @_;
	
	my @blocks = ();
	my $buff_size = length($buff);
	for (my $j = 0; $j < $buff_size; $j += $chunk) {
		my $tmp = substr($buff, $j, $chunk);
		
		my $chk;
		my $size = length($tmp);
		for (my $i = 0; $i < $size; ++$i) {
			$chk ^= ord(substr($tmp, $i, 1));
		}
		my $addr = $dst_addr + $j;
		
		push @blocks, [
			$addr, $size, 
			"F".
			chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
			chr(($size >> 24) & 0xFF).chr(($size >> 16) & 0xFF).chr(($size >> 8) & 0xFF).chr($size & 0xFF).
			$tmp.chr($chk)
		];
	}
	
	my $get_error = sub {
		my $ok = shift;
		if ($ok eq "\xFF\xFF") {
			return "Out of flash bounds!";
		} elsif ($ok eq "\xBB\xBB") {
			return "CRC error!";
		}
		return sprintf("Invalid answer '%02X%02X'", ord(substr($ok, 0, 1)), ord(substr($ok, 1, 1)));
	};
	
	my $i = 0;
	my $start = time;
	for my $block (@blocks) {
		my $addr = $block->[0];
		my $size = $block->[1];
		
		if ($i % 10 == 0) {
			printf("                                                    \r");
			printf("#$i %02d%s [WRITE] %08X-%08X (%.02f Kbps)\r", int(($addr - $dst_addr) / $buff_size * 100), "%", $addr, $addr + $size, (($addr - $dst_addr) / 1024) / (time - $start));
		}
		
		my $tries = 999999;
		while (1) {
			$port->write($block->[2]);
		
			my $ok = $port->read(2);
			if ($ok eq "\x01\x01") { # Блок отправился!
				$ok = $port->read(2);
				if ($ok eq "\x02\x02") { # Блок удалился!
					$ok = $port->read(2);
					if ($ok eq "\x03\x03") { # Блок записался!
						 last;
					} else {
						warn "\n[chaos_write_flash] ".$get_error->($ok)." (write)\n";
						if ($tries--) {
							chaos_keep_alive($port);
							next;
						}
						exit(1);
					}
				} else {
					warn "\n[chaos_write_flash] ".$get_error->($ok)." (erase)\n";
					if ($tries--) {
						chaos_keep_alive($port);
						next;
					}
					exit(1);
				}
			} else {
				warn "\n[chaos_write_flash] ".$get_error->($ok)." (send)\n";
				if ($tries--) {
					chaos_keep_alive($port);
					next;
				}
				exit(1);
			}
			last;
		}
		++$i;
	}
	print "\n";
	return 1;
}

sub mk_chaos_boot {
	# Модифицированный бут chaos/PV (из boot)
	
	my $alt_boot = dirname(__FILE__)."/boot/pv_boot_x85.hex";
	my $data = "";
	if (-f $alt_boot) {
		print "Using boot from $alt_boot\n";
		
		open F, "<$alt_boot" or die("open($alt_boot): $!");
		binmode F;
		
		while (!eof(F)) {
			read F, my $buf, 4096;
			$data .= $buf;
		}
		close F;
	} else {
		while (!eof(DATA)) {
			read DATA, my $buf, 4096;
			$data .= $buf;
		}
	}
	
	return hex2bin($data);
}

sub parse_addr {
	my $arg = shift;
	if (defined($arg) && $arg =~ /^(0x)?([a-f0-9]+)$/i) {
		return hex($2);
	}
	warn "Unknown hex value: $arg\n";
	return;
}

sub bin2hex {
	my $hex = shift;
	$hex =~ s/([\W\w])/sprintf("%02X", ord($1))/ge;
	return $hex;
}

sub hex2bin {
	my $hex = shift;
	$hex =~ s/\s+//gim;
	$hex = "0$hex" if (length($hex) % 2 != 0);
	$hex =~ s/([A-F0-9]{2})/chr(hex($1))/ge;
	return $hex;
}

sub write_boot {
	my ($port, $boot) = @_;
	
	# Считаем XOR
	my $chk = 0;
	my $len = length($boot);
	for (my $i = 0; $i < $len; ++$i) {
		$chk ^= ord(substr($boot, $i, 1));
	}
	
	$port->write("\x30");
	
	# Шлём размер бута
	$port->write(chr($len & 0xFF).chr(($len >> 8) & 0xFF));
	
	# Шлём бут
	$port->write($boot);
	
	# Шлём XOR бута
	$port->write(chr($chk));
	
	my $c = readb($port);
	return 1 if ($c == 0xC1 || $c == 0xB1 || $c == 0x01);
	
	warn sprintf("Invalid answer: %02X\n", $c);
	return 0;
}

sub set_port_baudrate {
	my ($port, $baudrate) = @_;
	my $termios = Linux::Termios2->new;
	$termios->getattr($port->FILENO);
	$termios->setospeed($baudrate);
	$termios->setispeed($baudrate);
	$termios->setattr($port->FILENO, TCSANOW);
	return -1;
}

sub readb {
	my ($port) = @_;
	my ($count, $char) = $port->read(1);
	return ord($char) if ($count);
	return -1;
}

# bootloader
__DATA__
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
