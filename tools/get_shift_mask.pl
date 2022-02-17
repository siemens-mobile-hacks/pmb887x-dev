#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use Sie::Utils;

if (scalar(@ARGV) != 2) {
	print "$0 <lsl> <lsr>\n";
	exit;
}

my $lsl = parseAnyInt($ARGV[0]);
my $lsr = parseAnyInt($ARGV[1]);

my $start = -1;
my $end = -1;
for (my $i = 0; $i < 32; $i++) {
	my $v = (((1 << $i) << $lsl) & 0xFFFFFFFF) >> $lsr;
	if ($v) {
		$start = $i if ($start == -1);
		$end = $i;
	}
}

my $v = ((0xFFFFFFFF << $lsl) & 0xFFFFFFFF) >> $lsr;

my $size = $end - $start + 1;
my $mask = ((1 << $size) - 1) << $start;

printf("SHIFT: %d\n", $start);
printf("SIZE:  %d\n", $size);
printf("MASK:  0x%08X\n", $mask);
