use warnings;
use strict;
use Data::Dumper;

require "common/perl/regs.pm";

my $header = "";
my $hash = [];
my $regs = get_regs();
my $name_len = 0;

print '
#include <idc.idc>
static main() {
';

for my $val (@$regs) {
	if (scalar(@$val) == 2) {
		my $name = $val->[0];
		$name =~ s/\[(\d+)\]/_$1/;
		printf("\tMakeName (0x%08X, \"%s\");\n", $val->[1], $name);
	}
}
print '}
';
