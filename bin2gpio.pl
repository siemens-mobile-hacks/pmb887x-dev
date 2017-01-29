use warnings;
use strict;
use Data::Dumper;

require "common/perl/regs.pm";

open F, "</home/azq2/mnt/io.bin";
binmode F;
my $data = "";
while (!eof(F)) {
	read F, my $buf, 1024;
	$data .= $buf;
}
close F;

my $pcl_base = 0xF4300000;
for (my $i = 0x20; $i < length($data); $i += 4) {
	my $addr = $pcl_base + $i;
	my $value = unpack("V", substr($data, $i, 4));
	
	if ($value != 0xFFFFFFFF) {
		printf("%08X: %08X %s\n", $addr, $value, reg_name($addr, $value));
	}
}