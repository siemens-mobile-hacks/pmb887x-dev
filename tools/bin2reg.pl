#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

if (scalar(@ARGV) != 2) {
	print "$0 <file> <base>\n";
	exit;
}

my $board = $ENV{BOARD} || "EL71";

my $board_meta = Sie::BoardMetadata->new($board);
my $cpu_meta = $board_meta->cpu();

open F, "<".$ARGV[0];
binmode F;
my $data = "";
while (!eof(F)) {
	read F, my $buf, 1024;
	$data .= $buf;
}
close F;

my $base = hex $ARGV[1];
for (my $i = 0; $i < length($data); $i += 4) {
	my $addr = $base + $i;
	my $value = unpack("V", substr($data, $i, 4));
	printf("%08X: %08X %s\n", $addr, $value, $cpu_meta->dumpReg($addr, $value));
}
