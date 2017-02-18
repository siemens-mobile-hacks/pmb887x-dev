use warnings;
use strict;
use Data::Dumper;

require "common/perl/regs.pm";

my $addr = hex $ARGV[0];
my $value = hex $ARGV[1];

printf("%08X: %08X %s\n", $addr, $value, reg_name($addr, $value));
