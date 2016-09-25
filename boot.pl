use warnings;
use strict;
use Device::SerialPort;
use Getopt::Long;
use File::Basename qw|dirname|;
use Time::HiRes qw|usleep|;
no utf8;

$| = 1;

my $device = "/dev/ttyUSB3";
my $speed = 115200;
my $file;
my $as_hex = 0;
my $bootloader = dirname(__FILE__)."/bootloader/bootloader.bin";

GetOptions (
	"device=s" => \$device, 
	"speed=s" => \$speed, 
	"boot=s" => \$bootloader, 
	"hex" => \$as_hex, 
	"exec=s" => \$file
);

my $port = Device::SerialPort->new($device);
die("open port error ($device)") if (!$port);

$port->baudrate($speed);
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
		
		if ($file) {
			$c = readb($port);
			if ($c == 0x0B) {
				$port->read_char_time(100);
				$port->read_const_time(200);
				
				loader_set_speed($port, 460800) or exit;
				
				my $addr = 0xA8000000;
				print "Detected loader!\n";
				
				my $raw = "";
				open(F, "<$file") or die("open($file): $!");
				while (!eof(F)) {
					my $buff;
					read F, $buff, 2048;
					$raw .= $buff;
				}
				close(F);
				
				# Пишем в раму
				printf("Load $file to RAM (%08X)... (size=%d)\n", $addr, length($raw));
				loader_write_ram($port, $addr, $raw) or die("load error");
				
				# Запускаем бинарь в раме
				printf("Exec %08X...\n", $addr);
				loader_exec_from_ram($port, $addr);
			} else {
				die("File loader not found");
			}
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
		if ($c eq 0x45) {
			warn "[loader_alive] Write error (CRC)\n";
		} else {
			warn sprintf("[loader_alive] Invalid answer 0x%02X\n", $c);
		}
		return 0;
	}
	return 1;
}

sub loader_set_speed {
	my ($port, $speed) = @_;
	$port->write(
		"B".
		chr(($speed >> 24) & 0xFF).chr(($speed >> 16) & 0xFF).chr(($speed >> 8) & 0xFF).chr($speed & 0xFF)
	);
	$port->baudrate($speed);
	$port->write_settings;
	return 1;
}

sub loader_exec_from_ram {
	my ($port, $addr) = @_;
	$port->write(
		"X".
		chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF)
	);
	return 1;
}

sub loader_write_ram {
	my ($port, $dst_addr, $buff) = @_;
	
	my $buff_size = length($buff);
	for (my $j = 0; $j < $buff_size; $j += 2048) {
		my $tmp = substr($buff, $j, 2048);
		
		my $chk;
		my $size = length($tmp);
		for (my $i = 0; $i < $size; ++$i) {
			$chk ^= ord(substr($tmp, $i, 1));
		}
		
		my $addr = $dst_addr + $j;
		printf("%02d%s [WRITE] %08X-%08X\n", int($j / $buff_size * 100), "%", $addr, $addr + $size);
		
		$port->write("W");
		$port->write(chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF));
		$port->write(chr(($size >> 24) & 0xFF).chr(($size >> 16) & 0xFF).chr(($size >> 8) & 0xFF).chr($size & 0xFF));
		$port->write(chr($chk));
		$port->write($tmp);
		
		my $c = readb($port);
		if ($c ne 0x4F) {
			if ($c eq 0x45) {
				warn "[loader_write_ram] Write error (CRC)\n";
			} else {
				warn sprintf("[loader_write_ram] Invalid answer 0x%02X\n", $c);
			}
			return 0;
		}
	}
	return 1;
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