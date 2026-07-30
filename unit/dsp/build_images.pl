#!/usr/bin/env perl
use strict;
use warnings;
use File::Spec;
use File::Temp qw(tempdir);
use FindBin qw($RealBin);
use lib $RealBin;
use DspAsm qw(expand_teak_constants);

my $makedsp1 = $ENV{MAKEDSP1} // '/home/azq2/build/teakra/build/src/makedsp1/makedsp1';
my $dsp1_reader = $ENV{DSP1_READER} // '/home/azq2/build/teakra/build/src/dsp1_reader/dsp1_reader';
my @images = (
	[ 'commands-0602.asm', 'commands-0602.inc', 'DSP_TEST_IMAGE_0602', 'pmb8875' ],
	[ 'commands-0604.asm', 'commands-0604.inc', 'DSP_TEST_IMAGE_0604', 'pmb8875' ],
	[ 'commands-0801.asm', 'commands-0801.inc', 'DSP_TEST_IMAGE_0801', 'pmb8876' ],
	[ 'irqs-0602.asm', 'irqs-0602.inc', 'DSP_IRQ_IMAGE_0602', 'pmb8875' ],
	[ 'irqs-0604.asm', 'irqs-0604.inc', 'DSP_IRQ_IMAGE_0604', 'pmb8875' ],
	[ 'irqs-0801.asm', 'irqs-0801.inc', 'DSP_IRQ_IMAGE_0801', 'pmb8876' ],
	[ 'safe-peripherals.asm', 'safe-peripherals-8875.inc', 'DSP_SAFE_PERIPHERALS_IMAGE_8875', 'pmb8875' ],
	[ 'safe-peripherals.asm', 'safe-peripherals-8876.inc', 'DSP_SAFE_PERIPHERALS_IMAGE_8876', 'pmb8876' ],
);

sub read_file {
	my ($path) = @_;
	open my $file, '<:raw', $path or die "Cannot read $path: $!\n";
	local $/;
	return <$file>;
}

sub write_image {
	my ($path, $array_name, $image) = @_;
	open my $output, '>', $path or die "Cannot write $path: $!\n";
	print {$output} "static const uint8_t $array_name\[] = {\n";
	for (my $offset = 0; $offset < length($image); $offset += 16) {
		my @bytes = unpack 'C*', substr($image, $offset, 16);
		my $suffix = $offset + 16 < length($image) ? ',' : '';
		print {$output} "  ", join(', ', map { sprintf '0x%02x', $_ } @bytes), "$suffix\n";
	}
	print {$output} "};\n";
	close $output or die "Cannot close $path: $!\n";
	chmod 0644, $path or die "Cannot chmod $path: $!\n";
}

sub run_tool {
	my (@command) = @_;
	system { $command[0] } @command;
	die "$command[0] failed with status $?\n" if $? != 0;
}

die "Usage: $0\n" if @ARGV;
die "makedsp1 is not executable: $makedsp1\n" if !-x $makedsp1;
die "dsp1_reader is not executable: $dsp1_reader\n" if !-x $dsp1_reader;
my $temp_dir = tempdir('dsp-images-XXXXXX', TMPDIR => 1, CLEANUP => 1);
for my $image (@images) {
	my ($asm_name, $output_name, $array_name, $cpu) = @$image;
	my $asm_path = File::Spec->catfile($RealBin, $asm_name);
	my $expanded_path = File::Spec->catfile($temp_dir, $output_name.'.asm');
	my $dsp1_path = File::Spec->catfile($temp_dir, $output_name.'.dsp1');
	my $dis_path = File::Spec->catfile($temp_dir, $output_name.'.dis');
	my $output_path = File::Spec->catfile($RealBin, $output_name);

	open my $expanded, '>', $expanded_path or die "Cannot write $expanded_path: $!\n";
	print {$expanded} expand_teak_constants($cpu, read_file($asm_path));
	close $expanded or die "Cannot close $expanded_path: $!\n";
	run_tool($makedsp1, $expanded_path, $dsp1_path);
	run_tool($dsp1_reader, $dsp1_path, $dis_path);
	write_image($output_path, $array_name, read_file($dsp1_path));
	print "$array_name: rebuilt\n";
}
