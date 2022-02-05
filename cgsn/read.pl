#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/../tools/lib';
use lib dirname(__FILE__);
use Device::SerialPort;
use Data::Dumper;
use SieCGSN;
use List::Util qw|min max|;
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;
no utf8;

$| = 1;

main();

sub main {
	my $help = 0;
	my $com_device = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0";
	my $com_speed = 921600;
	my $dst_dir = "0:/Misc/";
	my $file;
	my $addr = '0xA000003C';
	my $size = '0x4';
	my $dump;
	
	my $err = get_argv_opts({
		"device=s"		=> \$com_device, 
		"speed=s"		=> \$com_speed, 
		"file=s"		=> \$file, 
		"addr=s"		=> \$addr, 
		"size=s"		=> \$size, 
		"dump"			=> \$dump, 
	});
	
	if ($err || $help || !$size || !($file || $dump)) {
		print "$err\n";
		print join("\n", (
			'Common options:',
			'	--device=/dev/ttyUSB3    com port device',
			'	--speed=1600000          speed',
			'File options:',
			'	--file                   output file',
			'	--addr                   memory address',
			'	--size                   memory size',
			'	--dump                   dump memory',
		));
		print "\n";
		exit(1);
	}
	
	$addr = hex $addr;
	$size = hex $size;
	
	my $port = Device::SerialPort->new($com_device);
	die("open port error ($com_device)") if (!$port);
	
	my $board = $ENV{BOARD} || "EL71";

	my $board_meta = Sie::BoardMetadata->new($board);
	my $cpu_meta = $board_meta->cpu();

	$port->read_char_time(100);
	$port->read_const_time(100);
	
	$port->write_settings;
	
	my $cgsn = SieCGSN->new($port);
	$cgsn->connect($com_speed) or die("Mobile not found\n");
	
	my $raw;
	if ($dump) {
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
		
		for (my $i = $addr; $i < $addr + $size; $i += 4) {
			my $buf = $cgsn->readMem($i, 4);
			$raw .= $buf;
			
			my $value = unpack("V", $buf);
			printf("%08X: %08X %s\n", $i, $value, $cpu_meta->dumpReg($i, $value) || "");
		}
		
		if ($skip_after || $skip_before) {
			# Обрезаем ровно столько, сколько запросили
			$raw = substr($raw, $skip_before, length($raw) - ($skip_before + $skip_after));
		}
	} else {
		$raw = $cgsn->readMem($addr, $size);
	}
	
	if ($file) {
		open(F, ">$file") or die("open($file): $!");
		binmode F;
		printf F $raw;
		close(F);
	}
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
