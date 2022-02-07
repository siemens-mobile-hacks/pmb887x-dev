#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/../tools/lib';
use Data::Dumper;
use Sie::BoardMetadata;

my $board_meta = Sie::BoardMetadata->new("EL71");
my $cpu_meta = $board_meta->cpu();

if (!-f "/tmp/gpio.bin") {
	system("perl ./read.pl --addr=0xa8e27148 --size=0x200 --file=/tmp/gpio.bin --speed 115200");
}

open F, "/tmp/gpio.bin";
binmode F;

while (!eof(F)) {
	read F, my $data, 4;
	
	my $id = ord(substr($data, 0, 1));
	my $unk = ord(substr($data, 1, 1));
	my $v = (ord(substr($data, 3, 1)) << 8) | ord(substr($data, 2, 1));
	
	my $addr = $cpu_meta->{modules}->{GPIO}->{base} + $cpu_meta->{modules}->{GPIO}->{regs}->{PIN}->{start} + $id * $cpu_meta->{modules}->{GPIO}->{regs}->{PIN}->{step};
	
	if ($v) {
		printf("%08X: %08X %s\n", $addr, $v, $cpu_meta->dumpReg($addr, $v));
	}
}

close F;
