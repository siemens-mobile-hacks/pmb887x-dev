#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/../lib';
use Data::Dumper;
use List::Util qw(min max);
use Sie::SerialPort;
use Sie::CGSN;
use Sie::Utils;
use Sie::BoardMetadata;
use Getopt::Long;

$| = 1;

my %options = (
	board		=> "siemens-el71",
	device		=> "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0",
	speed		=> 921600,
	help		=> 0,
	read		=> 0,
	dump		=> 0,
	size		=> 4,
	dump		=> 0,
	output		=> undef,
);

my $serial;
my $cgsn;

END { $cgsn->close if $cgsn; };

main();

sub main {
	GetOptions(
		"board=s"		=> \$options{board},
		"device=s"		=> \$options{device},
		"speed=s"		=> \$options{speed},
		"read=s"		=> \$options{read},
		"dump=s"		=> \$options{dump},
		"size=s"		=> \$options{size},
		"output=s"		=> \$options{output},
		"help"			=> \$options{help},
	);
	
	if ($options{read}) {
		connectToDevice();
		cmdReadMemory();
	} elsif ($options{dump}) {
		connectToDevice();
		cmdDumpMemory();
	} else {
		help();
	}
}

sub connectToDevice {
	$serial = Sie::SerialPort->new($options{device});
	die("open port error (".$options{device}.")") if !$serial;
	
	$cgsn = Sie::CGSN->new($serial);
	$cgsn->connect($options{speed}) or die("Can't connect to phone!");
}

sub help {
	my $help = [
		"$0 <options>",
		"",
		"Common options:",
		[
			[" --device=/dev/ttyUSB0",		"Serial port."],
			[" --speed=921600",				"Maximum serial port speed."],
		],
		"",
		"Dump IO registers:",
		[
			[" --board=siemens-el71",		"Device type."],
			[" --dump=0xF0000000",			"Address of IO peripheral module."],
			[" --size=0x100",				"Size to read."],
		],
		"",
		"Read memory:",
		[
			[" --read=0xF0000000",			"Address of memory to read."],
			[" --size=0x100",				"Size to read."],
			[" --output=mem.bin",			"File name to save read memory (optional)."]
		]
	];
	
	my $FIRST_ROW_WIDTH = 36;
	
	for my $msg (@$help) {
		if (ref($msg)) {
			for my $row (@$msg) {
				print $row->[0];
				print (" " x ($FIRST_ROW_WIDTH - length($row->[0])));
				print $row->[1];
				print "\n";
			}
		} else {
			print $msg."\n";
		}
	}
	exit(0);
}

sub cmdReadMemory {
	my $addr = parseAnyInt($options{read});
	my $size = parseAnyInt($options{size});
	
	printf("Reading %d bytes from %08X...\n", $size, $addr);
	my $memory = $cgsn->readMemory($addr, $size);
	die "Can't read memory!" if !defined $memory;
	
	if (defined $options{output}) {
		print "Memory saved to file: ".$options{output}."\n";
		write_file($options{output}, $memory);
	} else {
		hexdump($addr, $memory);
	}
}

sub cmdDumpMemory {
	my $addr = parseAnyInt($options{dump});
	my $size = parseAnyInt($options{size});
	
	printf("Reading %d bytes from %08X...\n", $size, $addr);
	my $memory = $cgsn->readMemory($addr, $size);
	die "Can't read memory!" if !defined $memory;
	
	my $board_meta = Sie::BoardMetadata->new($options{board});
	my $cpu_meta = $board_meta->cpu();
	
	for (my $i = 0; $i < $size; $i += 4) {
		my $value = unpack("V", substr($memory, $i, 4));
		printf("%08X: %08X %s\n", $addr + $i, $value, $cpu_meta->dumpReg($addr + $i, $value) || "");
	}
}
