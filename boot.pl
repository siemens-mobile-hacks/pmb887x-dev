#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use File::Slurp qw(read_file write_file);
use lib dirname(__FILE__).'/tools/lib';
use Sie::SerialPort;
use Sie::Utils;
use Sie::Boot;
use Time::HiRes;
use Getopt::Long;
no utf8;

my %STD_BOOTS = (
	BurninMode	=> "F104A0E3201090E5FF10C1E3A51081E3201080E51EFF2FE1040108000000000000000000000000005349454D454E535F424F4F54434F444501000700000000000000000000000000000000000000000001040580830003",
	ServiceMode	=> "F104A0E3201090E5FF10C1E3A51081E3201080E51EFF2FE1040108000000000000000000000000005349454D454E535F424F4F54434F4445010007000000000000000000000000000000000000000000010405008B008B",
	NormalMode	=> "F104A0E3201090E5FF10C1E3A51081E3201080E51EFF2FE1040108000000000000000000000000005349454D454E535F424F4F54434F444501000700000000000000000000000000000000000000000001040500890089",
);

$| = 1;

my $serial;

END { releasePort() };
$SIG{INT} = $SIG{TERM} = sub { releasePort(); exit(1); };

my %options = (
	device		=> "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0",
	boot_speed	=> 115200,
	speed		=> 1600000,
	module		=> undef,
	as_hex		=> 0,
	picocom		=> 0,
	help		=> 0,
	ign			=> 1,
	boot		=> 'ServiceMode'
);

main();

sub main {
	GetOptions(
		"device=s"		=> \$options{device},
		"boot_speed=s"	=> \$options{boot_speed},
		"speed=s"		=> \$options{speed},
		"boot=s"		=> \$options{boot},
		"ign=s"			=> \$options{ign},
		"as_hex"		=> \$options{as_hex},
		"picocom"		=> \$options{picocom},
		"module=s"		=> \$options{module},
		"help=s"		=> \$options{help},
	);
	
	$serial = Sie::SerialPort->new($options{device});
	die("open port error (".$options{device}.")") if !$serial;
	
	$serial->setSpeed($options{boot_speed});
	
	my $boot_status = Sie::Boot::detectPhone($serial, $options{ign});
	die "Can't boot device!" if !$boot_status;
	
	my $bootcode = getBootCode($options{boot});
	die "Bootcode '".$options{boot}."' not found!" if !$bootcode;
	
	print "Sending bootcode '".$options{boot}."' to the phone...\n";
	my ($load_status, $err) = Sie::Boot::loadBootCode($serial, $bootcode);
	die $err if !$load_status;
	
	print "Bootcode loaded!\n";
	
	if ($options{module}) {
		print "Running module: ".$options{module}."\n";
		Time::HiRes::usleep(35000);
		require $options{module};
		siemens_boot::boot_module_init($serial, $options{speed});
		exit(0);
	}
	
	if ($options{picocom}) {
		system("picocom -b ".$options{boot_speed}." ".$options{device});
		exit;
	}
	
	if ($options{as_hex}) {
		while (1) {
			my $c = $serial->getChar(1);
			if ($c > -1) {
				my $str = chr($c);
				printf("%s | %02X\n", ($str =~ /[[:print:]]/ ? "'".$str."'" : " ? "), $c);
			}
		}
	} else {
		while (1) {
			my $c = $serial->getChar(1);
			last if $c == 0;
			print chr($c) if ($c > -1);
		}
	}
}

sub getBootCode {
	my ($name) = @_;
	
	return hex2bin($STD_BOOTS{$name}) if exists $STD_BOOTS{$name};
	
	if ($name =~ /^([\w\d_-]+)$/i) {
		my $precompiled_hex = dirname(__FILE__)."/boot/$name.hex";
		return readBootCode($precompiled_hex) if -f $precompiled_hex;
		
		my $precompiled_bin = dirname(__FILE__)."/boot/$name.bin";
		return readBootCode($precompiled_bin) if -f $precompiled_bin;
	}
	
	return readBootCode($name) if -f $name;
	return undef;
}

sub readBootCode {
	my ($path) = @_;
	print "Using boot code: $path (".(-s $path)." bytes)\n";
	if ($path =~ /\.hex$/i) {
		my $data = scalar(read_file($path));
		$data =~ s/\s+//g;
		return hex2bin($data);
	}
	return scalar(read_file($path));
}

sub releasePort {
	if ($serial) {
		$serial->dtr($options{ign} && $options{ign} == 2 ? 1 : 0);
		$serial->close;
		undef $serial;
	}
}
