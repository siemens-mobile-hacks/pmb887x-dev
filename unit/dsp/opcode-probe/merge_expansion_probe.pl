#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use Getopt::Long qw(GetOptions);

my $output_path;
GetOptions('output=s' => \$output_path) or die "Usage: $0 [--output MAP.csv] CAPTURE1 CAPTURE2\n";
die "Usage: $0 [--output MAP.csv] CAPTURE1 CAPTURE2\n" unless @ARGV == 2;
my $record_pattern = qr{^# DSPEXP (\d+) ([0-9A-F]{4}) }
	. qr{(complete|trap-complete|timeout|trap-timeout|unsafe-skip) (.*)$};
my $summary_pattern = qr{^# DSPEXP-SUMMARY families=(\d+)-(\d+) words=([0-9A-F]{4})-([0-9A-F]{4}) }
	. qr{executed=(\d+) skipped=(\d+) complete=(\d+) trap=(\d+) timeout=(\d+)$};

sub read_capture {
	my ($path) = @_;
	open my $input, '<', $path or die "Cannot read $path: $!\n";
	my %families;
	my %records;
	my @summary;
	my $summary_count = 0;
	my $passed = 0;
	while (my $line = <$input>) {
		$passed = 1 if $line =~ /^# result: PASS \(0 failed\)$/;
		if ($line =~ /^# DSPEXP-FAMILY (\d+) ([0-9A-F]{4}) "([^"]+)"$/) {
			my ($family, $first_word, $name) = ($1, $2, $3);
			my $description = join ',', $first_word, $name;
			die "Conflicting family $family in $path\n"
				if defined $families{$family} && $families{$family} ne $description;
			$families{$family} = $description;
		}
		if ($line =~ $summary_pattern) {
			@summary = ($1, $2, hex($3), hex($4), $5, $6, $7, $8, $9);
			$summary_count++;
		}
		next unless $line =~ $record_pattern;
		my ($family, $expansion, $outcome, $word_text) = ($1, $2, $3, $4);
		my @words = split ' ', $word_text;
		die "Expansion record has the wrong word count in $path: $line" unless @words == 24;
		die "Expansion record contains a malformed word in $path: $line"
			if grep { $_ !~ /^[0-9A-F]{4}$/ } @words;
		@words = ('FFFF') x 24 if $outcome =~ /timeout$/;
		my $key = sprintf('%02u:%s', $family, $expansion);
		my $record = join ',', $family, $expansion, $outcome, @words;
		die "Conflicting duplicate $key in $path\n"
			if defined $records{$key} && $records{$key} ne $record;
		$records{$key} = $record;
	}
	close $input or die "Cannot close $path: $!\n";
	die "Capture did not pass: $path\n" unless $passed;
	die "Capture must contain exactly one summary: $path\n" unless $summary_count == 1;

	my ($family_first, $family_last, $word_first, $word_last) = @summary[0 .. 3];
	my $expected = ($family_last - $family_first + 1) * ($word_last - $word_first + 1);
	die "Invalid summary range: $path\n" if $family_last < $family_first || $word_last < $word_first;
	die "Record count does not match summary range: $path\n" unless keys(%records) == $expected;
	for my $family ($family_first .. $family_last) {
		die "Missing family $family in $path\n" unless defined $families{$family};
		for my $word ($word_first .. $word_last) {
			my $key = sprintf('%02u:%04X', $family, $word);
			die "Missing expansion $key in $path\n" unless defined $records{$key};
		}
	}

	my %outcomes;
	for my $record (values %records) {
		my (undef, undef, $outcome) = split ',', $record;
		$outcomes{$outcome}++;
	}
	my $executed = $expected - ($outcomes{'unsafe-skip'} // 0);
	my $completed = ($outcomes{complete} // 0) + ($outcomes{'trap-complete'} // 0);
	my $trapped = ($outcomes{'trap-complete'} // 0) + ($outcomes{'trap-timeout'} // 0);
	my $timed_out = ($outcomes{timeout} // 0) + ($outcomes{'trap-timeout'} // 0);
	die "Capture summary counts do not match records: $path\n"
		unless $summary[4] == $executed && $summary[5] == ($outcomes{'unsafe-skip'} // 0) &&
		$summary[6] == $completed && $summary[7] == $trapped && $summary[8] == $timed_out;
	return { families => \%families, records => \%records, summary => \@summary };
}

sub normalize_record {
	my ($record) = @_;
	$record =~ s/^(\d+,[0-9A-F]{4},)trap-timeout,/${1}timeout,/;
	return $record;
}

my ($first_path, $second_path) = @ARGV;
my $first = read_capture($first_path);
my $second = read_capture($second_path);
die "Capture ranges differ\n" unless join(',', @{$first->{summary}}[0 .. 3]) eq join(',', @{$second->{summary}}[0 .. 3]);
for my $family (sort { $a <=> $b } keys %{$first->{families}}) {
	die "Hardware captures differ for family $family\n"
		unless ($second->{families}{$family} // '') eq $first->{families}{$family};
}
for my $key (sort keys %{$first->{records}}) {
	my $first_record = normalize_record($first->{records}{$key});
	my $second_record = normalize_record($second->{records}{$key} // '');
	die "Hardware captures differ at $key:\n  $first->{records}{$key}\n  $second->{records}{$key}\n"
		unless $second_record eq $first_record;
}

my @lines;
for my $family (sort { $a <=> $b } keys %{$first->{families}}) {
	push @lines, "# family,$family,$first->{families}{$family}";
}
push @lines, 'family,expansion,outcome,entered,trap,post,done,st0,st1,st2,a0l,a0h,a1l,a1h,r0,'
	. 'b0l,b0h,b0st0,b1l,b1h,b1st0,pl,ph,scratch,sp,stack_top,observed_memory';
push @lines, map { normalize_record($first->{records}{$_}) } sort keys %{$first->{records}};
my $map = join("\n", @lines) . "\n";
my $hash = sha256_hex($map);
if (defined $output_path) {
	open my $output, '>', $output_path or die "Cannot write $output_path: $!\n";
	print {$output} $map;
	close $output or die "Cannot close $output_path: $!\n";
}
printf "DSP expansion map: %u matching records, SHA-256 %s\n", scalar(keys %{$first->{records}}), $hash;
