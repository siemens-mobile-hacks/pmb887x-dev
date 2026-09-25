#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename qw(dirname);
use File::Spec;

my @expected_names = qw(valid correctable invalid);
my $output_path = File::Spec->catfile(dirname(__FILE__), 'sch-decoder-golden.inc');

sub read_captures {
	my ($path) = @_;
	open my $input, '<', $path or die "Cannot read $path: $!\n";
	my (%captures, $name, @record);

	while (my $line = <$input>) {
		if ($line =~ /^# SCH_GOLDEN_BEGIN name=(\S+) mask=0801\s*$/) {
			die "$path contains nested SCH capture\n" if defined $name;
			$name = $1;
			@record = ($line);
			next;
		}
		if (defined $name) {
			push @record, $line;
			if ($line =~ /^# SCH_GOLDEN_END\s*$/) {
				die "$path contains duplicate vector $name\n" if exists $captures{$name};
				$captures{$name} = join('', @record);
				undef $name;
			}
		}
	}
	close $input or die "Cannot close $path: $!\n";
	die "$path contains an incomplete SCH capture\n" if defined $name;
	for my $expected (@expected_names) {
		die "$path is missing SCH vector $expected\n" if !exists $captures{$expected};
	}
	die "$path contains unexpected SCH vectors\n" if keys(%captures) != @expected_names;
	return \%captures;
}

die "Usage: $0 CAPTURE1 CAPTURE2 CAPTURE3\n" if @ARGV != 3;
my @runs = map { read_captures($_) } @ARGV;
for my $name (@expected_names) {
	for my $run (1 .. $#runs) {
		die "SCH vector $name differs between hardware captures\n"
			if $runs[$run]->{$name} ne $runs[0]->{$name};
	}
}

open my $output, '>', $output_path or die "Cannot write $output_path: $!\n";
for my $name (@expected_names) {
	my $record = $runs[0]->{$name};
	my ($a0h, $a0l, $a1h, $a1l, $data0, $data1) = $record =~
		/^# RESULT A0=([0-9A-F]{4}):([0-9A-F]{4}) A1=([0-9A-F]{4}):([0-9A-F]{4}) DATA0=([0-9A-F]{4}) DATA1=([0-9A-F]{4})$/m;
	die "SCH vector $name has no valid result line\n" if !defined $data1;
	print {$output} qq{\t{ 0x$a0l, 0x$a0h, 0x$a1l, 0x$a1h, 0x$data0, 0x$data1 },\n};
}
close $output or die "Cannot close $output_path: $!\n";
print "Imported three identical PMB8876 Mask ROM 0801 captures into $output_path\n";
