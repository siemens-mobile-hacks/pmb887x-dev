use warnings;
use strict;
use Device::SerialPort;
use File::Basename;
use lib dirname(__FILE__).'/tools/lib';
use Time::HiRes qw|usleep|;
use Linux::Termios2;
use Time::HiRes;
use POSIX qw( :termios_h );
no utf8;

$| = 1;

my $IGNITION_ON_PERIOD = 0.5;
my $IGNITION_OFF_PERIOD = 0.5;

my $device = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0";
my $boot_speed = 115200;
my $speed = 1600000;
my $module;
my $as_hex = 0;
my $ign = 1;
my $dtr = 0;
my $rts = 0;
my $run_picocom = 0;
my $send_noise = 0;
my $bootloader = dirname(__FILE__)."/bootloader/bootloader.bin";

get_argv_opts({
	"device=s"		=> \$device, 
	"boot-speed=s"	=> \$boot_speed, 
	"speed=s"		=> \$speed, 
	"boot=s"		=> \$bootloader, 
	"hex"			=> \$as_hex, 
	"module=s"		=> \$module, 
	"picocom"		=> \$run_picocom, 
	"noise"			=> \$send_noise,
	"ign"			=> \$ign, 
	"dtr"			=> \$dtr, 
	"rts"			=> \$rts
});

my $port = Device::SerialPort->new($device);
die("open port error ($device)") if (!$port);

$port->baudrate($boot_speed);
$port->databits(8);
$port->parity("none");
$port->stopbits(1);

$port->dtr_active($dtr);
$port->rts_active($rts);

$port->read_char_time(0);
$port->read_const_time(20);

$SIG{INT} = $SIG{TERM} = sub {
	$port->dtr_active(0);
	$port->rts_active(0);
	exit(0);
};

$port->write_settings;
if ($ign) {
	while (readb($port) != -1) { }
}
print "Please, short press red button!\n";

my $bootloaders = {
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

my $last_ignition = 0;
my $last_dtr = 0;
my $boot_ok = 0;

while (1) {
	if ($ign) {
		my $elapsed = Time::HiRes::time - $last_ignition;
		
		my $timeout = $last_dtr ? $IGNITION_ON_PERIOD : $IGNITION_OFF_PERIOD;
		
		if ($elapsed >= $timeout) {
			$last_dtr = !$last_dtr;
			
			print "^" if $last_dtr;
			
			$port->dtr_active($last_dtr) if $ign == 1;
			$port->dtr_active(!$last_dtr) if $ign == 2;
			
			$last_ignition = Time::HiRes::time;
		}
	}
	
	$port->write("AT");
	
	my $c = readb($port);
	if ($c == 0xB0 || $c == 0xC0 || $c == 0xD5) {
		$port->dtr_active($dtr) if ($ign);
		print "\n";
		print "SGOLD detected!\n" if ($c == 0xB0);
		print "NewSGOLD detected!\n" if ($c == 0xC0);
		print "SGOLD2 detected!\n" if ($c == 0xD5);
		
		if ($c == 0xD5) {
			readb($port);
			readb($port);
		}
		
		$port->read_char_time(100);
		$port->read_const_time(300);
		
		print "Sending boot...\n";
		my $boot = "";
		if (!-f $bootloader && exists($bootloaders->{$bootloader})) {
			for my $c (@{$bootloaders->{$bootloader}}) {
				$boot .= chr($c);
			}
		} else {
			open(F, "<$bootloader") or die("open($bootloader): $!");
			while (!eof(F)) {
				my $buff;
				read F, $buff, 2048;
				$boot .= $buff;
			}
			close(F);
		}
		
		print "boot size: ".length($boot)."\n";
		write_boot($port, $boot, $c == 0xD5 ? 1 : 0);
		
		if ($module) {
			usleep(35000);
			require $module;
			siemens_boot::boot_module_init($port, $speed);
			exit(0);
		}
		
		$port->read_char_time(1000);
		$port->read_const_time(2000);
		
		if ($run_picocom) {
			system("picocom -b $boot_speed $device");
			exit;
		}
		
		if ($as_hex) {
			while (1) {
				$c = readb($port);
				if ($c > -1) {
					my $str = chr($c);
					printf("%s | %02X\n", ($str =~ /[[:print:]]/ ? "'".$str."'" : " ? "), $c);
				}
				
				if ($send_noise) {
					$port->write(chr(rand(0xFF)));
				}
			}
		} else {
			while (1) {
				if ($send_noise) {
					$port->write(chr(rand(0xFF)));
				}
				
				my $c = readb($port);
				last if $c == 0;
				
				print chr($c) if ($c > -1);
			}
		}
		last;
	} elsif ($c == 0) {
		printf("%02X", $c);
	} else {
		print ".";
	}
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

sub write_boot {
	my ($port, $boot, $direct) = @_;
	
	my $len = length($boot);
	
	# XOR
	my $chk = 0;
	for (my $i = 0; $i < $len; ++$i) {
		$chk ^= ord(substr($boot, $i, 1));
	}
	
	my $packet = "\x30".chr($len & 0xFF).chr(($len >> 8) & 0xFF).$boot.chr($chk);
	
	if ($direct) {
		$port->write($packet);
	} else {
		my $last_byte_write = 0;
		for (my $i = 0; $i < length($packet); ++$i) {
			my $c = substr($packet, $i, 1);
			
			my $elapsed = (Time::HiRes::time - $last_byte_write) * 1000000;
			Time::HiRes::nanosleep(1) if ($elapsed < 1);
			$port->write($c);
			
			$last_byte_write = Time::HiRes::time;
		}
	}
	
	my $c = readb($port);
	return 1 if ($c == 0xC1 || $c == 0xB1 || $c == 0x01);
	die sprintf("Invalid answer: %02X\n", $c);
	
	return 0;
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

sub readb {
	my ($port) = @_;
	my ($count, $char) = $port->read(1);
	return ord($char) if ($count);
	return -1;
}

sub bin2hex {
	my $hex = shift;
	$hex =~ s/([\W\w])/sprintf("%02X", ord($1))/ge;
	return $hex;
}
