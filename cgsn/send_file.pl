use warnings;
use strict;
use File::Basename qw|basename dirname|;
use lib dirname(__FILE__);
use Device::SerialPort;
use Data::Dumper;
use SieCGSN;
use List::Util qw|min max|;
no utf8;

$| = 1;

main();

sub main {
	my $help = 0;
	my $com_device = "/dev/ttyUSB0";
	my $com_speed = 921600;
	my $dst_dir = "0:/Misc/";
	my $file;
	my $filename;
	my $exec = 0;
	
	my $err = get_argv_opts({
		"device=s"		=> \$com_device, 
		"speed=s"		=> \$com_speed, 
		"dir=s"			=> \$dst_dir, 
		"file=s"		=> \$file, 
		"name=s"		=> \$filename, 
		"exec"			=> \$exec
	});
	
	if ($err || $help || !$file) {
		print "$err\n";
		print join("\n", (
			'Common options:',
			'	--device=/dev/ttyUSB3    com port device',
			'	--speed=1600000          speed after boot',
			'File options:',
			'	--file                   file to write',
			'	--dir                    dir to write',
			'	--name                   override file name',
			'	--exec                   exec elf',
		));
		print "\n";
		exit(1);
	}
	
	my $port = Device::SerialPort->new($com_device);
	die("open port error ($com_device)") if (!$port);
	
	$port->read_char_time(100);
	$port->read_const_time(100);
	
	$port->write_settings;
	
	my $cgsn = SieCGSN->new($port);
	$cgsn->connect($com_speed) or die("Mobile not found\n");
	
	my $device = $cgsn->getDeviceKey();
	my $FUNCS = ({ # TODO: swi
		"SIEMENS:EL71:45" => {
			malloc	=> 0xA0092F51, # void malloc(unsigned int size)
			mfree	=> 0xA0092F93, # void mfree(void *memptr)
			fopen	=> 0xA056E674, # int fopen(const char * cFileName, unsigned int iFileFlags, unsigned int iFileMode, unsigned int *ErrorNumber)
			fclose	=> 0xA056E5C8, # void fclose(int FileHandler, unsigned int *ErrorNumber)
			fwrite	=> 0xA056E864, # unsigned int fwrite(int FileHandler, void *cBuffer, int iByteCount, unsigned int *ErrorNumber)
			mkdir	=> 0xA056E26C, # int mkdir(const char *cFileName, unsigned int *ErrorNumber)
			unlink	=> 0xA056E4B0, # unlink(const char *cFileName,unsigned int *errornumber)
			elfopen	=> 0xA0078304  # int elfload(char *filename, void *param1, void *param2, void *param3)
		}
	})->{$device};
	
	if (!$FUNCS) {
		die "$device not supported\n";
	}
	
	my $file_raw = "";
	open(F, "<$file") or die("open($file): $!");
	while (!eof(F)) {
		my $buff;
		read F, $buff, 2048;
		$file_raw .= $buff;
	}
	close(F);
	
	my $MAX_PATH = 255;
	my $name = $dst_dir.substr($filename ? $filename : basename($file), max(0, length($dst_dir) - $MAX_PATH), $MAX_PATH);
	
	$name =~ s/\//\\/g; # ненужнослэши
	
	my $off_error	= 0;
	my $off_path	= $off_error + 4;
	my $off_data	= $off_path + length($name) + 1;
	my $malloc_sz	= $off_data + length($file_raw);
	
	printf "=> Allocate memory (%.02f Kb)...\n", $malloc_sz / 1024;
	my $malloc = $cgsn->exec($FUNCS->{malloc}, $malloc_sz);
	
	# Записываем файл в RAM
	printf("=> Write '$file' to RAM (%.02f Kb)...\n", length($file_raw) / 1024);
	$cgsn->writeMem($malloc->{R0}, "\0\0\0\0$name\0$file_raw");
	
	# Удаляем файл
	print "=> Unlink '$name'...\n";
	$cgsn->exec($FUNCS->{unlink}, $malloc->{R0} + $off_path, $malloc->{R0} + $off_error);
	
	# A_ReadOnly 0
	# A_WriteOnly 1
	# A_ReadWrite 2
	# A_NoShare 4
	# A_Append 8
	# A_Exclusive 0x10
	# A_MMCStream 0x20
	# A_Create 0x100
	# A_Truncate 0x200
	# A_FailCreateOnExist 0x400
	# A_FailOnReopen 0x800
	# P_WRITE 0x100
	# P_READ 0x80
	
	# Открываем новый файл
	print "=> Open '$name'...\n";
	my $fopen = $cgsn->exec($FUNCS->{fopen}, $malloc->{R0} + $off_path, 2 | 0x100 | 0x200, 0x100 | 0x80, $malloc->{R0} + $off_error);
	
	if ($fopen->{R0}) {
		print "=> Write '$name'...\n";
		my $fwrite = $cgsn->exec($FUNCS->{fwrite}, $fopen->{R0}, $malloc->{R0} + $off_data, length($file_raw), $malloc->{R0} + $off_error);
		if ($fwrite->{R0} != length($file_raw)) {
			print "!! Write error! (".$fwrite->{R0}." != ".length($file_raw).")\n";
		}
		print "=> Close '$name'...\n";
		$cgsn->exec($FUNCS->{fclose}, $fopen->{R0}, $malloc->{R0} + $off_error);
	} else {
		print "!! Open error\n";
	}
	
	if ($exec) { # Пикает чёт
		print "=> Run elf...\n";
		$cgsn->exec($FUNCS->{elfopen}, $malloc->{R0} + $off_path, 0, 0, 0);
	}
	
	print "=> Free ram...\n";
	$cgsn->exec($FUNCS->{mfree}, $malloc->{R0});
}

# TODO: вынести куда-нибудь
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
