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
	print "$0 <addr> <value>\n";
	exit;
}

my $board = $ENV{BOARD} || "siemens-el71";

my $addr = hex $ARGV[0];
my $value = hex $ARGV[1];

my $board_meta = Sie::BoardMetadata->new($board);
my $cpu_meta = $board_meta->cpu();

printf("%08X: %08X %s\n", $addr, $value, $cpu_meta->dumpReg($addr, $value));
