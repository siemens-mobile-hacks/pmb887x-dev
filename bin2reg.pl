use warnings;
use strict;
use Data::Dumper;

require "common/perl/regs.pm";

open F, "<".$ARGV[0];
binmode F;
my $data = "";
while (!eof(F)) {
	read F, my $buf, 1024;
	$data .= $buf;
}
close F;

my $nr = 0;
my $base = hex $ARGV[1];
for (my $i = 0; $i < length($data); $i += 4) {
	my $addr = $base + $i;
	my $value = unpack("V", substr($data, $i, 4));
	
	printf("%08X: %08X %s\n", $addr, $value, reg_name($addr, $value));
	
	++$nr;
}