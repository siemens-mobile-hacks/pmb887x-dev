#!/usr/bin/env perl

use strict;
use warnings;
use Getopt::Long qw(GetOptions);
use FindBin;
use lib $FindBin::Bin;
use Dsp1 qw(build_dsp1);

my %layouts = (
	pmb8875 => {
		program_size => 0x28000,
		data_size    => 0x18000,
		program      => [
			[ 0x00000, 0x1000, 0x14000, 'fixed' ],
			[ 0x14000, 0xB000, 0x0A000, 'bank 0' ],
			[ 0x1E000, 0xB000, 0x0A000, 'bank 1' ],
		],
		data => [
			[ 0x00000, 0x6000, 0x06000, 'fixed' ],
			[ 0x06000, 0x9000, 0x09000, 'bank 0' ],
			[ 0x0F000, 0x9000, 0x09000, 'bank 1' ],
		],
	},
	pmb8876 => {
		program_size => 0x34000,
		data_size    => 0x22000,
		program      => [
			[ 0x00000, 0x2000, 0x10000, 'fixed' ],
			[ 0x10000, 0xA000, 0x0C000, 'bank 0' ],
			[ 0x1C000, 0xA000, 0x0C000, 'bank 1' ],
			[ 0x28000, 0xA000, 0x0C000, 'bank 2' ],
		],
		data => [
			[ 0x00000, 0x8000, 0x02000, 'fixed' ],
			[ 0x02000, 0x9000, 0x08000, 'bank 0' ],
			[ 0x0A000, 0x9000, 0x08000, 'bank 1' ],
			[ 0x12000, 0x9000, 0x08000, 'bank 2' ],
			[ 0x1A000, 0x9000, 0x08000, 'bank 3' ],
		],
	},
);

sub usage {
	my ($exit_code) = @_;

	print <<'USAGE';
Usage:
  rom_to_dsp1.pl --cpu <pmb8875|pmb8876> --program <program-rom.bin>
    --data <data-rom.bin> --out <rom.dsp1>

Converts the linear Program and Data Mask ROM dumps produced by the BSP tests
to one DSP1 file. The CPU selects the fixed ranges, bank-window addresses, and
expected input sizes. Repeated bank-window addresses are stored as consecutive
DSP1 segments in page-number order.
USAGE
	exit $exit_code;
}

sub read_file {
	my ($path, $expected_size, $description) = @_;

	open my $fh, '<:raw', $path or die "Cannot open $path: $!\n";
	local $/;
	my $data = <$fh>;
	die "Cannot read $path: $!\n" if !defined $data;
	close $fh or die "Cannot close $path: $!\n";
	die sprintf("Invalid %s size: expected 0x%X bytes, got 0x%X\n",
		$description, $expected_size, length($data)) if length($data) != $expected_size;

	return $data;
}

sub append_segments {
	my ($segments, $image, $layout, $memory_type, $description) = @_;

	for my $range (@$layout) {
		my ($offset, $address, $size, $name) = @$range;
		push @$segments, {
			address     => $address,
			memory_type => $memory_type,
			data        => substr($image, $offset, $size),
		};
		printf "%s %s: file +0x%05X, DSP 0x%04X, 0x%05X bytes\n",
			$description, $name, $offset, $address, $size;
	}
}

sub write_file {
	my ($path, $data) = @_;

	open my $fh, '>:raw', $path or die "Cannot create $path: $!\n";
	print {$fh} $data or die "Cannot write $path: $!\n";
	close $fh or die "Cannot close $path: $!\n";
}

my ($cpu, $program_path, $data_path, $output, $help);
GetOptions(
	'cpu=s'     => \$cpu,
	'program=s' => \$program_path,
	'data=s'    => \$data_path,
	'out=s'     => \$output,
	'help'      => \$help,
) or usage(2);
usage(0) if $help;
my $missing_options = !defined $cpu || !defined $program_path || !defined $data_path || !defined $output;
usage(2) if @ARGV || $missing_options;
die "Unknown CPU '$cpu'; expected pmb8875 or pmb8876\n" if !exists $layouts{$cpu};

my $layout = $layouts{$cpu};
my $program = read_file($program_path, $layout->{program_size}, 'Program ROM');
my $data = read_file($data_path, $layout->{data_size}, 'Data ROM');
my @segments;
append_segments(\@segments, $program, $layout->{program}, 0, 'Program');
append_segments(\@segments, $data, $layout->{data}, 2, 'Data');
write_file($output, build_dsp1(\@segments));
printf "Wrote %s: %d segments, 0x%X bytes\n", $output, scalar @segments, -s $output;
