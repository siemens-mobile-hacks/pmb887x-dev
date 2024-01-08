#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/../lib';
use Data::Dumper;
use Sie::XBZ;
use File::Slurp qw(read_file write_file);

my $file = $ARGV[0];

my $xbz = Sie::XBZ->new($file);

my $type = $xbz->detectType();
die "This is not firmware file!\n" if !$type;

print "Detected type $type\n";

if ($type eq "exe") {
	my ($winswup, $extracted) = $xbz->extractXBZ();
	die "Can't extract xbz from $file!\n" if !$extracted;
	write_file("/tmp/sie.xbz", $extracted);
	write_file("/tmp/sie.exe", $winswup);
	print "Detected type: ".$xbz->detectType()."\n";
}

my $info = $xbz->parse($file);
print Dumper($info);

write_file("/tmp/sie.ff", $xbz->getFullflash());
