use warnings;
use strict;
use Device::SerialPort;
use Getopt::Long;
use File::Basename qw|dirname|;
use Time::HiRes qw|usleep|;
use Linux::Termios2;
use POSIX qw( :termios_h );
no utf8;

$| = 1;

my $device = "/dev/ttyUSB3";
my $boot_speed = 115200;
my $speed = 1600000;
my $module;
my $as_hex = 0;
my $bootloader = dirname(__FILE__)."/bootloader/bootloader.bin";

GetOptions (
	"device=s" => \$device, 
	"boot-speed=s" => \$boot_speed, 
	"speed=s" => \$speed, 
	"boot=s" => \$bootloader, 
	"hex" => \$as_hex, 
	"module=s" => \$module
);

my $port = Device::SerialPort->new($device);
die("open port error ($device)") if (!$port);

$port->baudrate($boot_speed);
$port->databits(8);
$port->parity("none");
$port->stopbits(1);

$port->read_char_time(0);
$port->read_const_time(100);

$port->write_settings;


print "Please, short press red button!\n";

my $bootloaders = {
	ServiceMode => [
		# code
		0xF1,0x04,0xA0,0xE3,0x20,0x10,0x90,0xE5,0xFF,0x10,0xC1,0xE3,0xA5,0x10,0x81,0xE3,
		0x20,0x10,0x80,0xE5,0x1E,0xFF,0x2F,0xE1,0x04,0x01,0x08,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
		
		# id
		0x53,0x49,0x45,0x4D,0x45,0x4E,0x53,0x5F,0x42,0x4F,0x4F,0x54,0x43,0x4F,0x44,0x45, 
		
		0x01, 0x00, 0x07, 0x00, # type
		0x00, 0xC2, 0x01, 0x00, # speed 115200
		
		# bootkey
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	]
};

my $boot_ok = 0;
while (1) {
	$port->write("AT");
	
	my $c = readb($port);
	if ($c == 0xB0 || $c == 0xC0) {
		print "\n";
		print "SGOLD detected!\n" if ($c == 0xB0);
		print "NewSGOLD detected!\n" if ($c == 0xC0);
		
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
		write_boot($port, $boot);
		
		usleep(35000);
		if ($module) {
			require $module;
			siemens_boot::boot_module_init($port, $speed);
			exit(0);
		}
		
		$port->read_char_time(1000);
		$port->read_const_time(2000);
		if ($as_hex) {
			while (($c = readb($port)) >= 0) {
				my $str = chr($c);
				printf("%s | %02X\n", ($str =~ /[[:print:]]/ ? "'".$str."'" : " ? "), $c);
			}
		} else {
			while (($c = readb($port)) >= 0) {
				print chr($c);
			}
		}
		last;
	}
	print ".";
}

sub loader_alive {
	my ($port) = @_;
	$port->write(".");
	my $c = readb($port);
	if ($c ne 0x4F) {
		warn sprintf("[loader_alive] Invalid answer 0x%02X\n", $c);
		return 0;
	}
	return 1;
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
	return 1 if ($c == 0xC1);
	
	warn sprintf("Invalid answer: %02X\n", $c);
	return 0;
}


sub readb {
	my ($port) = @_;
	my ($count, $char) = $port->read(1);
	return ord($char) if ($count);
	return -1;
}