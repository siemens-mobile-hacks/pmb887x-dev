#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use Getopt::Long qw(GetOptions);

my $output_path;
GetOptions('output=s' => \$output_path) or die "Usage: $0 [--output MAP.csv] CAPTURE1 CAPTURE2\n";
die "Usage: $0 [--output MAP.csv] CAPTURE1 CAPTURE2\n" unless @ARGV == 2;
my $record_pattern = qr{^# DSPPROBE ([0-9A-F]{4}) (complete|trap-complete|timeout|trap-timeout) }
	. qr{([0-9A-F]{4}) ([0-9A-F]{4}) ([0-9A-F]{4}) ([0-9A-F]{4})$};

sub read_capture {
	my ($path) = @_;
	open my $input, '<', $path or die "Cannot read $path: $!\n";
	my @records;
	my $passed = 0;
	my @summary;
	my $summary_count = 0;
	while (my $line = <$input>) {
		$passed = 1 if $line =~ /^# result: PASS \(0 failed\)$/;
		if ($line =~ /^# DSPPROBE-SUMMARY ([0-9A-F]{4}) ([0-9A-F]{4}) complete=(\d+) trap=(\d+) timeout=(\d+)$/) {
			@summary = (hex($1), hex($2), $3, $4, $5);
			$summary_count++;
		}
		next unless $line =~ $record_pattern;
		my ($opcode, $outcome, @markers) = (hex($1), $2, $3, $4, $5, $6);
		@markers = ('FFFF') x 4 if $outcome =~ /timeout$/;
		my $record = join ',', sprintf('%04X', $opcode), $outcome, @markers;
		die sprintf("Conflicting duplicate opcode %04X in %s\n", $opcode, $path)
			if defined $records[$opcode] && $records[$opcode] ne $record;
		$records[$opcode] = $record;
	}
	close $input or die "Cannot close $path: $!\n";
	die "Capture did not pass: $path\n" unless $passed;
	die "Capture must contain exactly one summary: $path\n" unless $summary_count == 1;
	die "Capture summary does not cover 0000-FFFF: $path\n" unless $summary[0] == 0 && $summary[1] == 0xFFFF;
	for my $opcode (0 .. 0xFFFF) {
		die sprintf("Missing opcode %04X in %s\n", $opcode, $path) unless defined $records[$opcode];
	}
	my %outcomes;
	for my $record (@records) {
		my (undef, $outcome) = split ',', $record;
		$outcomes{$outcome}++;
	}
	my $completed = ($outcomes{complete} // 0) + ($outcomes{'trap-complete'} // 0);
	my $trapped = ($outcomes{'trap-complete'} // 0) + ($outcomes{'trap-timeout'} // 0);
	my $timed_out = ($outcomes{timeout} // 0) + ($outcomes{'trap-timeout'} // 0);
	die "Capture summary counts do not match records: $path\n"
		unless $summary[2] == $completed && $summary[3] == $trapped && $summary[4] == $timed_out;
	return \@records;
}

my ($first_path, $second_path) = @ARGV;
my $first = read_capture($first_path);
my $second = read_capture($second_path);
for my $opcode (0 .. 0xFFFF) {
	die sprintf("Hardware captures differ at opcode %04X:\n  %s\n  %s\n", $opcode,
		$first->[$opcode], $second->[$opcode]) if $first->[$opcode] ne $second->[$opcode];
}

my $map = "opcode,outcome,entered,trap,post,done\n" . join("\n", @$first) . "\n";
my $hash = sha256_hex($map);
if (defined $output_path) {
	open my $output, '>', $output_path or die "Cannot write $output_path: $!\n";
	print {$output} $map;
	close $output or die "Cannot close $output_path: $!\n";
}
print "DSP opcode map: 65536/65536 matching records, SHA-256 $hash\n";
