use warnings;
use strict;
use Data::Dumper;

require "common/perl/regs.pm";

my $add_names = grep { $_ eq "-n" } @ARGV;
my $header = "";
my $hash = [];
my $regs = get_regs();
my $name_len = 0;

for my $val (@$regs) {
	if (scalar(@$val) == 2) {
		my $name = $val->[0];
		$name =~ s/\[(\d+)\]/_$1/;
		
		$header .= sprintf("#define %s 0x%08X\n", $name, $val->[1]);
		push @$hash, sprintf("{\"%s\", 0x%08X, 0}", $val->[0], $val->[1]);
		
		$name_len = length($val->[0]) if ($name_len < length($val->[0]));
	} elsif (scalar(@$val) == 3) {
		push @$hash, sprintf("{\"%s\", 0x%08X, 0x%08X}", $val->[0], $val->[1], $val->[2]);
	}
}

if ($add_names) {
	$header .= '
struct PMB8876_REG_NAME {
	const char name['.($name_len + 1).'];
	unsigned int addr;
	unsigned int addr_end;
};
struct PMB8876_REG_NAME PMB8876_REGS_NAMES[] = {'."\n\t".''.join(",\n\t", @$hash)."\n};\n";
}

print $header;
