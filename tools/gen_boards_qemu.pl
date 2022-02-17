#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

my $str = qq|
#include "hw/arm/pmb887x/boards.h"
|;

for my $board (@{Sie::BoardMetadata::getBoards()}) {
	my $board_meta = Sie::BoardMetadata->new($board);
	$str .= genBoardInfo($board_meta);
}

print $str."\n";

sub genBoardInfo {
	my ($board_meta) = @_;
	
	return $str;
}
