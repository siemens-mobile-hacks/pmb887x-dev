#!/usr/bin/env perl
use strict;
use warnings;

use File::Copy qw(copy);
use File::Spec;
use File::Temp qw(tempdir);
use FindBin qw($RealBin);
use Getopt::Long qw(GetOptions);
use lib $RealBin;
use DspMapArchive qw(extract_archive verify_archive write_archive);

my $remove_source;
GetOptions('remove-source' => \$remove_source) or die "usage: $0 [--remove-source]\n";
die "usage: $0 [--remove-source]\n" if @ARGV;

my @members = ('hardware-map.csv', map { sprintf 'expansion-family-%02u-map.csv', $_ } 0 .. 18);
my $archive_path = File::Spec->catfile($RealBin, 'hardware-maps.pack.gz');
my $temporary_directory = tempdir('dsp-map-pack-XXXXXX', TMPDIR => 1, CLEANUP => 1);
my @sources;

if (-f $archive_path) {
	my $archived_members = extract_archive($archive_path, $temporary_directory);
	die "$archive_path: unexpected member list\n" unless join("\n", @$archived_members) eq join("\n", @members);
}

for my $name (@members) {
	my $source = File::Spec->catfile($RealBin, $name);
	my $temporary = File::Spec->catfile($temporary_directory, $name);
	if (-f $source) {
		copy $source, $temporary or die "Cannot copy $source to $temporary: $!\n";
		push @sources, $source;
	}
	die "Missing DSP hardware map: $name\n" unless -f $temporary;
}

sub validate_raw_map {
	my ($path) = @_;
	open my $input, '<', $path or die "Cannot read $path: $!\n";
	my $header = <$input> // '';
	die "$path: invalid raw map header\n" unless $header eq "opcode,outcome,entered,trap,post,done\n";
	for my $opcode (0 .. 0xFFFF) {
		my $line = <$input> // die sprintf("$path: missing opcode %04X\n", $opcode);
		chomp $line;
		my @fields = split /,/, $line;
		die sprintf("$path: invalid opcode %04X record\n", $opcode)
			unless @fields == 6 && $fields[0] eq sprintf('%04X', $opcode) &&
			$fields[1] =~ /\A(?:complete|trap-complete|timeout)\z/ &&
			!grep { $_ !~ /\A[0-9A-F]{4}\z/ } @fields[2 .. 5];
	}
	die "$path: trailing records\n" if defined <$input>;
	close $input or die "Cannot close $path: $!\n";
}

sub validate_expansion_map {
	my ($path, $family) = @_;
	open my $input, '<', $path or die "Cannot read $path: $!\n";
	my $metadata = <$input> // '';
	die "$path: invalid family metadata\n" unless $metadata =~ /^# family,$family,[0-9A-F]{4},[^\r\n]+\n$/;
	my $header = <$input> // '';
	die "$path: invalid expansion map header\n"
		unless $header =~ /^family,expansion,outcome,entered,trap,post,done,/;
	for my $expansion (0 .. 0xFFFF) {
		my $line = <$input> // die sprintf("$path: missing expansion %04X\n", $expansion);
		chomp $line;
		my @fields = split /,/, $line, -1;
		die sprintf("$path: invalid expansion %04X record\n", $expansion)
			unless @fields == 27 && $fields[0] == $family && $fields[1] eq sprintf('%04X', $expansion) &&
			$fields[2] =~ /\A(?:complete|trap-complete|timeout|unsafe-skip)\z/ &&
			!grep { $_ !~ /\A[0-9A-F]{4}\z/ } @fields[3 .. 26];
	}
	die "$path: trailing records\n" if defined <$input>;
	close $input or die "Cannot close $path: $!\n";
}

validate_raw_map(File::Spec->catfile($temporary_directory, $members[0]));
for my $family (0 .. 18) {
	validate_expansion_map(File::Spec->catfile($temporary_directory, $members[$family + 1]), $family);
}

write_archive($archive_path, $temporary_directory, \@members);
verify_archive($archive_path, \@members);

if ($remove_source) {
	for my $source (@sources) {
		unlink $source or die "Cannot remove archived source $source: $!\n";
	}
}

printf "Packed %u verified DSP hardware maps into %s%s\n", scalar(@members), $archive_path,
	$remove_source ? ' and removed the source CSV files' : '';
