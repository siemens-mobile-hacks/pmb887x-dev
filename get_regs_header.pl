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
	if (!$val->{addr_end}) {
		my $name = $val->{name};
		$name =~ s/\[(\d+)\]/_$1/;
		
		$header .= sprintf("#define %s 0x%08X\n", $name, $val->{addr});
		push @$hash, sprintf("{\"%s\", 0x%08X, 0}", $val->{name}, $val->{addr});
		
		$name_len = length($val->{name}) if ($name_len < length($val->{name}));
	} else {
		push @$hash, sprintf("{\"%s\", 0x%08X, 0x%08X}", $val->{name}, $val->{addr}, $val->{addr_end});
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
