#!/usr/bin/env perl
use strict;
use warnings;

use File::Spec;
use FindBin qw($RealBin);
use Getopt::Long qw(GetOptions);
use lib $RealBin;
use DspMapArchive qw(read_member);

my $archive_path = File::Spec->catfile($RealBin, 'hardware-maps.pack.gz');
my $raw;
my $family;
GetOptions('archive=s' => \$archive_path, 'raw' => \$raw, 'family=i' => \$family)
	or die "usage: $0 [--archive ARCHIVE] (--raw | --family N) ACTUAL\n";
die "usage: $0 [--archive ARCHIVE] (--raw | --family N) ACTUAL\n"
	unless @ARGV == 1 && ($raw xor defined $family) && (!defined $family || $family >= 0 && $family <= 18);
my $actual_path = $ARGV[0];

sub read_file {
	my ($path) = @_;
	open my $input, '<:raw', $path or die "Cannot read $path: $!\n";
	local $/;
	my $data = <$input>;
	close $input or die "Cannot close $path: $!\n";
	return $data;
}

sub normalize_record {
	my ($record, $key_fields) = @_;
	my @fields = split /,/, $record;
	$fields[$key_fields] = 'timeout' if $fields[$key_fields] eq 'trap-timeout';
	@fields[$key_fields + 1 .. $#fields] = ('FFFF') x ($#fields - $key_fields)
		if $fields[$key_fields] eq 'timeout';
	return join ',', @fields;
}

sub parse_raw_csv {
	my ($text, $description) = @_;
	my @lines = split /\r?\n/, $text;
	die "$description: invalid raw CSV header\n" unless shift(@lines) eq 'opcode,outcome,entered,trap,post,done';
	pop @lines while @lines && $lines[-1] eq '';
	die "$description: expected 65536 records, found " . scalar(@lines) . "\n" unless @lines == 65536;
	my @records;
	for my $opcode (0 .. 0xFFFF) {
		my @fields = split /,/, $lines[$opcode];
		die sprintf("$description: malformed opcode %04X\n", $opcode)
			unless @fields == 6 && $fields[0] eq sprintf('%04X', $opcode);
		$records[$opcode] = normalize_record($lines[$opcode], 1);
	}
	return \@records;
}

sub parse_raw_log {
	my ($text, $description) = @_;
	my @records;
	my $passed = $text =~ /^# result: PASS \(0 failed\)$/m;
	my @summaries = $text =~ /^# DSPPROBE-SUMMARY ([^\r\n]+)$/mg;
	die "$description: capture did not pass\n" unless $passed;
	die "$description: expected one raw summary\n" unless @summaries == 1;
	die "$description: raw summary does not cover 0000-FFFF\n" unless $summaries[0] =~ /^0000 FFFF /;
	while ($text =~ /^# DSPPROBE ([0-9A-F]{4}) (complete|trap-complete|timeout|trap-timeout) ((?:[0-9A-F]{4} ){3}[0-9A-F]{4})$/mg) {
		my ($opcode, $outcome, $words) = (hex($1), $2, $3);
		my $record = normalize_record(join(',', sprintf('%04X', $opcode), $outcome, split(' ', $words)), 1);
		die sprintf("$description: conflicting opcode %04X\n", $opcode)
			if defined $records[$opcode] && $records[$opcode] ne $record;
		$records[$opcode] = $record;
	}
	for my $opcode (0 .. 0xFFFF) {
		die sprintf("$description: missing opcode %04X\n", $opcode) unless defined $records[$opcode];
	}
	return \@records;
}

sub parse_expansion_csv {
	my ($text, $description, $wanted_family) = @_;
	my @lines = split /\r?\n/, $text;
	die "$description: invalid family metadata\n" unless shift(@lines) =~ /^# family,$wanted_family,/;
	die "$description: invalid expansion CSV header\n" unless shift(@lines) =~ /^family,expansion,outcome,/;
	pop @lines while @lines && $lines[-1] eq '';
	die "$description: expected 65536 records, found " . scalar(@lines) . "\n" unless @lines == 65536;
	my @records;
	for my $expansion (0 .. 0xFFFF) {
		my @fields = split /,/, $lines[$expansion];
		die sprintf("$description: malformed expansion %04X\n", $expansion)
			unless @fields == 27 && $fields[0] == $wanted_family && $fields[1] eq sprintf('%04X', $expansion);
		$records[$expansion] = normalize_record($lines[$expansion], 2);
	}
	return \@records;
}

sub parse_expansion_log {
	my ($text, $description, $wanted_family) = @_;
	my @records;
	my $passed = $text =~ /^# result: PASS \(0 failed\)$/m;
	my @summaries = $text =~ /^# DSPEXP-SUMMARY ([^\r\n]+)$/mg;
	die "$description: capture did not pass\n" unless $passed;
	die "$description: expected one expansion summary\n" unless @summaries == 1;
	die "$description: expansion summary does not cover family $wanted_family and words 0000-FFFF\n"
		unless $summaries[0] =~ /^families=$wanted_family-$wanted_family words=0000-FFFF /;
	while ($text =~ /^# DSPEXP (\d+) ([0-9A-F]{4}) (complete|trap-complete|timeout|trap-timeout|unsafe-skip) ([0-9A-F ]+)$/mg) {
		next unless $1 == $wanted_family;
		my ($expansion, $outcome, @words) = (hex($2), $3, split(' ', $4));
		die sprintf("$description: wrong word count at expansion %04X\n", $expansion) unless @words == 24;
		my $record = normalize_record(join(',', $wanted_family, sprintf('%04X', $expansion), $outcome, @words), 2);
		die sprintf("$description: conflicting expansion %04X\n", $expansion)
			if defined $records[$expansion] && $records[$expansion] ne $record;
		$records[$expansion] = $record;
	}
	for my $expansion (0 .. 0xFFFF) {
		die sprintf("$description: missing expansion %04X\n", $expansion) unless defined $records[$expansion];
	}
	return \@records;
}

my $member = $raw ? 'hardware-map.csv' : sprintf('expansion-family-%02u-map.csv', $family);
my $expected_text = read_member($archive_path, $member);
my $actual_text = read_file($actual_path);
my ($expected, $actual);
if ($raw) {
	$expected = parse_raw_csv($expected_text, "$archive_path:$member");
	$actual = $actual_text =~ /^opcode,outcome,/ ? parse_raw_csv($actual_text, $actual_path) :
		parse_raw_log($actual_text, $actual_path);
} else {
	$expected = parse_expansion_csv($expected_text, "$archive_path:$member", $family);
	$actual = $actual_text =~ /^# family,/ ? parse_expansion_csv($actual_text, $actual_path, $family) :
		parse_expansion_log($actual_text, $actual_path, $family);
}

my $mismatches = 0;
my $compared = 0;
my $skipped = 0;
for my $key (0 .. 0xFFFF) {
	if (!$raw && $expected->[$key] =~ /^\d+,[0-9A-F]{4},unsafe-skip,/) {
		$skipped++;
		next;
	}
	$compared++;
	next if $expected->[$key] eq $actual->[$key];
	if ($mismatches < 20) {
		printf STDERR "%s %04X differs:\n  HW: %s\n  actual: %s\n", $raw ? 'opcode' : 'expansion', $key,
			$expected->[$key], $actual->[$key];
	}
	$mismatches++;
}

die "$mismatches mismatches among $compared compared records ($skipped unsafe records skipped)\n" if $mismatches;
print "MATCH: $compared records; $skipped unsafe records excluded from behavioral comparison\n";
