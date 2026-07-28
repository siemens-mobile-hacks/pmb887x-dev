#!/usr/bin/env perl

use strict;
use warnings;
use Getopt::Long qw(GetOptions);
use File::Path qw(make_path);
use File::Spec;

sub usage {
	my ($exit_code) = @_;

	print <<'USAGE';
Usage:
  extract_sl98_dsp.pl --address <number> [options] <SL98 firmware.bin>

Options:
  --base <number>          ARM address corresponding to file offset 0
                           (default: 0xA0000000)
  --address <number>       ARM address of one DSP boot container (required)
  --out <directory>       Output directory (default: dsp-fw)
  --help                   Show this help

The input must be a raw ARM firmware image. Addresses are converted to file
offsets as: offset = address - base.

Known SL98 container addresses are 0xA073A3C2 for DSP mask firmware 0x0801 and
0xA073E502 for DSP mask firmware 0x0800. Pass one of them per invocation.

This container is an ARM-side PLOAD/DLOAD/BRANCH stream, not a DSP1 file.

The script writes:
  container.bin  Original ARM-side PLOAD/DLOAD/BRANCH stream
  pram.bin       Sparse 16-bit program-space image, little-endian
  dram.bin       Sparse 16-bit data-space image, little-endian
  records/       Exact payload of every load record
  map.tsv        Record map and branch address

Unknown holes in pram.bin and dram.bin are filled with 0xFFFF. Use map.tsv or
the individual record files to distinguish holes from loaded 0xFFFF words.
USAGE
	exit $exit_code;
}

sub parse_number {
	my ($value, $name) = @_;

	die "$name is missing\n" if !defined $value;
	return hex($value) if $value =~ /^0x[0-9a-f]+$/i;
	return int($value) if $value =~ /^\d+$/;
	die "Invalid $name: $value\n";
}

sub read_exact {
	my ($fh, $offset, $length, $description) = @_;
	my $data = '';

	seek($fh, $offset, 0) or die "Cannot seek to $description at $offset: $!\n";
	my $read = read($fh, $data, $length);
	die "Cannot read $description at $offset: $!\n" if !defined $read;
	die "Short read for $description at $offset: wanted $length, got $read\n" if $read != $length;

	return $data;
}

sub write_file {
	my ($path, $data) = @_;

	open my $fh, '>:raw', $path or die "Cannot create $path: $!\n";
	print {$fh} $data or die "Cannot write $path: $!\n";
	close $fh or die "Cannot close $path: $!\n";
}

sub parse_container {
	my ($fh, $file_size, $offset) = @_;
	my @records;
	my $cursor = $offset;
	my $branch;

	die sprintf("DSP container offset 0x%X is outside the input file\n", $offset)
		if $offset < 0 || $offset >= $file_size;

	for my $index (0 .. 255) {
		my $header = read_exact($fh, $cursor, 4, 'DSP record header');
		my ($type, $destination) = unpack('v2', $header);

		if ($type == 2) {
			push @records, {
				index       => $index,
				type        => 'BRANCH',
				offset      => $cursor,
				destination => $destination,
				length      => 0,
				raw         => $header,
				payload     => '',
			};
			$cursor += 4;
			$branch = $destination;
			last;
		}

		die sprintf("Invalid DSP record type 0x%04X at file offset 0x%X\n",
			$type, $cursor) if $type != 0 && $type != 1;

		my $length_data = read_exact($fh, $cursor + 4, 2, 'DSP record length');
		my $length = unpack('v', $length_data);
		die sprintf("Unreasonable DSP block length 0x%X at file offset 0x%X\n",
			$length, $cursor) if $length > 0x1000;
		die sprintf("DSP block exceeds 16-bit address space at file offset 0x%X\n",
			$cursor) if $destination + $length > 0x10000;

		my $payload = read_exact($fh, $cursor + 6, $length * 2, 'DSP record payload');
		push @records, {
			index       => $index,
			type        => $type == 0 ? 'PLOAD' : 'DLOAD',
			offset      => $cursor,
			destination => $destination,
			length      => $length,
			raw         => $header . $length_data . $payload,
			payload     => $payload,
		};
		$cursor += 6 + $length * 2;
		die "DSP container runs past end of input file\n"
			if $cursor > $file_size;
	}

	die "DSP container has no BRANCH record\n" if !defined $branch;

	return (\@records, $cursor - $offset, $branch);
}

sub build_sparse_image {
	my ($records, $type) = @_;
	my $word_count = 0;

	for my $record (@$records) {
		next if $record->{type} ne $type;
		my $end = $record->{destination} + $record->{length};
		$word_count = $end if $end > $word_count;
	}

	my $image = "\xFF\xFF" x $word_count;
	for my $record (@$records) {
		next if $record->{type} ne $type;
		substr($image, $record->{destination} * 2, $record->{length} * 2, $record->{payload});
	}

	return ($image, $word_count);
}

sub extract_container {
	my ($fh, $file_size, $output_root, $base, $address) = @_;
	my $offset = $address - $base;
	my ($records, $container_size, $branch) = parse_container($fh, $file_size, $offset);
	my $records_dir = File::Spec->catdir($output_root, 'records');
	make_path($records_dir);

	my $container = join('', map { $_->{raw} } @$records);
	write_file(File::Spec->catfile($output_root, 'container.bin'), $container);

	my ($pram, $pram_words) = build_sparse_image($records, 'PLOAD');
	my ($dram, $dram_words) = build_sparse_image($records, 'DLOAD');
	write_file(File::Spec->catfile($output_root, 'pram.bin'), $pram);
	write_file(File::Spec->catfile($output_root, 'dram.bin'), $dram);

	my $map_path = File::Spec->catfile($output_root, 'map.tsv');
	open my $map, '>', $map_path or die "Cannot create $map_path: $!\n";
	print {$map} "record\ttype\tfile_offset\tdestination_word\tlength_words\tpayload_file\n";

	my ($program_words, $data_words) = (0, 0);
	for my $record (@$records) {
		if ($record->{type} eq 'BRANCH') {
			printf {$map} "%d\tBRANCH\t0x%X\t0x%04X\t0\t-\n",
				$record->{index}, $record->{offset}, $record->{destination};
			next;
		}

		my $name = sprintf('%03d-%s-%04X-%04X.bin',
			$record->{index}, lc($record->{type}), $record->{destination}, $record->{length});
		write_file(File::Spec->catfile($records_dir, $name), $record->{payload});
		printf {$map} "%d\t%s\t0x%X\t0x%04X\t%d\trecords/%s\n",
			$record->{index}, $record->{type}, $record->{offset}, $record->{destination},
			$record->{length}, $name;
		$program_words += $record->{length} if $record->{type} eq 'PLOAD';
		$data_words += $record->{length} if $record->{type} eq 'DLOAD';
	}
	close $map or die "Cannot close $map_path: $!\n";

	printf "DSP: address=0x%08X offset=0x%X container=%d bytes, " .
		"PLOAD=%d words, DLOAD=%d words, branch=0x%04X\n",
		$address, $offset, $container_size, $program_words, $data_words, $branch;
	printf "          pram.bin=%d words, dram.bin=%d words, output=%s\n",
		$pram_words, $dram_words, $output_root;
}

my $base_text = '0xA0000000';
my $address_text;
my $output = 'dsp-fw';
my $help;

GetOptions(
	'base=s'         => \$base_text,
	'address=s'      => \$address_text,
	'out=s'          => \$output,
	'help'           => \$help,
) or usage(2);
usage(0) if $help;
usage(2) if @ARGV != 1;
die "--address is required\n" if !defined $address_text;

my $input = $ARGV[0];
my $base = parse_number($base_text, '--base');
my $address = parse_number($address_text, '--address');

open my $firmware, '<:raw', $input or die "Cannot open $input: $!\n";
my $file_size = -s $firmware;
die "Cannot determine size of $input\n" if !defined $file_size;
make_path($output);

extract_container($firmware, $file_size, $output, $base, $address);

close $firmware or die "Cannot close $input: $!\n";
