#!/usr/bin/perl
use warnings;
use strict;
use File::Slurp qw(read_file);

die "$0 <file>\n" if @ARGV != 1;

my $BLOCK_SIZE = 32;

print "uint8_t data[] = {\n\t";
my $data = read_file($ARGV[0]);
for (my $i = 0; $i < length($data); $i++) {
	my $c = ord(substr($data, $i, 1));
	print ",\n\t" if ($i && ($i % $BLOCK_SIZE) == 0);
	print ", " if ($i && ($i % $BLOCK_SIZE) != 0);
	printf("0x%02X", $c);
}
print "\n};\n";
