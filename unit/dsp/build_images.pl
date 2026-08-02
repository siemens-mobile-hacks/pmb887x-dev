#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use File::Copy qw(copy);
use File::Path qw(make_path);
use File::Spec;
use File::Temp qw(tempdir);
use FindBin qw($RealBin);
use lib $RealBin;
use DspAsm qw(expand_teak_constants);

my $makedsp1 = $ENV{MAKEDSP1} // '/home/azq2/build/teakra/build/src/makedsp1/makedsp1';
my $dsp1_reader = $ENV{DSP1_READER} // '/home/azq2/build/teakra/build/src/dsp1_reader/dsp1_reader';
my $cache_dir = $ENV{DSP_IMAGE_CACHE} // '/tmp/pmb887x-dsp-images';
my @images = (
	[ 'commands-0602.asm', 'commands-0602.inc', 'DSP_TEST_IMAGE_0602', 'pmb8875' ],
	[ 'commands-0604.asm', 'commands-0604.inc', 'DSP_TEST_IMAGE_0604', 'pmb8875' ],
	[ 'commands-0801.asm', 'commands-0801.inc', 'DSP_TEST_IMAGE_0801', 'pmb8876' ],
	[ 'irqs-0602.asm', 'irqs-0602.inc', 'DSP_IRQ_IMAGE_0602', 'pmb8875' ],
	[ 'irqs-0604.asm', 'irqs-0604.inc', 'DSP_IRQ_IMAGE_0604', 'pmb8875' ],
	[ 'irqs-0801.asm', 'irqs-0801.inc', 'DSP_IRQ_IMAGE_0801', 'pmb8876' ],
	[ 'benchmark.asm', 'benchmark-8875.inc', 'DSP_BENCHMARK_8875', 'pmb8875' ],
	[ 'benchmark.asm', 'benchmark-8876.inc', 'DSP_BENCHMARK_8876', 'pmb8876' ],
	[ 'timers-functional.asm', 'timers-functional-8875.inc', 'DSP_TIMERS_IMAGE_8875', 'pmb8875' ],
	[ 'timers-functional.asm', 'timers-functional-8876.inc', 'DSP_TIMERS_IMAGE_8876', 'pmb8876' ],
	[ 'mcs-functional.asm', 'mcs-functional-8875.inc', 'DSP_MCS_IMAGE_8875', 'pmb8875' ],
	[ 'mcs-functional.asm', 'mcs-functional-8876.inc', 'DSP_MCS_IMAGE_8876', 'pmb8876' ],
	[ 'interrupt-functional.asm', 'interrupt-functional-8875.inc', 'DSP_INTERRUPT_IMAGE_8875', 'pmb8875' ],
	[ 'interrupt-functional.asm', 'interrupt-functional-8876.inc', 'DSP_INTERRUPT_IMAGE_8876', 'pmb8876' ],
	[ 'ssc-functional.asm', 'ssc-functional-8875.inc', 'DSP_SSC_IMAGE_8875', 'pmb8875' ],
	[ 'ssc-functional.asm', 'ssc-functional-8876.inc', 'DSP_SSC_IMAGE_8876', 'pmb8876' ],
	[ 'cipher-a512-functional.asm', 'cipher-a512-functional-8875.inc', 'DSP_CIPHER_A512_IMAGE_8875', 'pmb8875' ],
	[ 'cipher-a512-functional.asm', 'cipher-a512-functional-8876.inc', 'DSP_CIPHER_A512_IMAGE_8876', 'pmb8876' ],
	[ 'cipher-a53-functional.asm', 'cipher-a53-functional-8875.inc', 'DSP_CIPHER_A53_IMAGE_8875', 'pmb8875' ],
	[ 'cipher-a53-functional.asm', 'cipher-a53-functional-8876.inc', 'DSP_CIPHER_A53_IMAGE_8876', 'pmb8876' ],
	[ 'cipher-kgcore-vectors.asm', 'cipher-kgcore-vectors-8875.inc', 'DSP_CIPHER_KGCORE_IMAGE_8875', 'pmb8875' ],
	[ 'cipher-kgcore-vectors.asm', 'cipher-kgcore-vectors-8876.inc', 'DSP_CIPHER_KGCORE_IMAGE_8876', 'pmb8876' ],
	[ 'channel-decoder-functional.asm', 'channel-decoder-functional-8875.inc', 'DSP_CHANNEL_DECODER_IMAGE_8875', 'pmb8875' ],
	[ 'channel-decoder-functional.asm', 'channel-decoder-functional-8876.inc', 'DSP_CHANNEL_DECODER_IMAGE_8876', 'pmb8876' ],
	[ 'channel-decoder-vector-runner.asm', 'channel-decoder-vector-runner-8875.inc', 'DSP_CHANNEL_DECODER_VECTOR_RUNNER_8875', 'pmb8875' ],
	[ 'channel-decoder-vector-runner.asm', 'channel-decoder-vector-runner-8876.inc', 'DSP_CHANNEL_DECODER_VECTOR_RUNNER_8876', 'pmb8876' ],
	[ 'equalizer-functional.asm', 'equalizer-functional-8875.inc', 'DSP_EQUALIZER_IMAGE_8875', 'pmb8875' ],
	[ 'equalizer-functional.asm', 'equalizer-functional-8876.inc', 'DSP_EQUALIZER_IMAGE_8876', 'pmb8876' ],
	[ 'equalizer-vector-runner.asm', 'equalizer-vector-runner-8875.inc', 'DSP_EQUALIZER_VECTOR_RUNNER_8875', 'pmb8875' ],
	[ 'equalizer-vector-runner.asm', 'equalizer-vector-runner-8876.inc', 'DSP_EQUALIZER_VECTOR_RUNNER_8876', 'pmb8876' ],
	[ 'equalizer-output-runner.asm', 'equalizer-output-runner-8875.inc', 'DSP_EQUALIZER_OUTPUT_RUNNER_8875', 'pmb8875' ],
	[ 'equalizer-output-runner.asm', 'equalizer-output-runner-8876.inc', 'DSP_EQUALIZER_OUTPUT_RUNNER_8876', 'pmb8876' ],
	[ 'equalizer-burst-runner.asm', 'equalizer-burst-runner-8875.inc', 'DSP_EQUALIZER_BURST_RUNNER_8875', 'pmb8875' ],
	[ 'equalizer-burst-runner.asm', 'equalizer-burst-runner-8876.inc', 'DSP_EQUALIZER_BURST_RUNNER_8876', 'pmb8876' ],
	[ 'i2s-functional.asm', 'i2s-functional-8875.inc', 'DSP_I2S_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'i2s-functional.asm', 'i2s-functional-8876.inc', 'DSP_I2S_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'i2s-rx-functional.asm', 'i2s-rx-functional-8876.inc', 'DSP_I2S_RX_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'i2s-rx-pcm-functional.asm', 'i2s-rx-pcm-functional-8876.inc', 'DSP_I2S_RX_PCM_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'afe-functional.asm', 'afe-functional-8875.inc', 'DSP_AFE_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'afe-functional.asm', 'afe-functional-8876.inc', 'DSP_AFE_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'modulator-functional.asm', 'modulator-functional-8875.inc', 'DSP_MODULATOR_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'modulator-functional.asm', 'modulator-functional-8876.inc', 'DSP_MODULATOR_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'baseband-functional.asm', 'baseband-functional-8876.inc', 'DSP_BASEBAND_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'tpu-bb-events.asm', 'tpu-bb-events-8876.inc', 'DSP_TPU_BB_EVENTS_8876', 'pmb8876' ],
	[ 'l1mon-vectors.asm', 'l1mon-vectors-8876.inc', 'DSP_L1MON_VECTORS_8876', 'pmb8876' ],
	[ 'l1mon-functional.asm', 'l1mon-functional-8876.inc', 'DSP_L1MON_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'dsp-control-functional.asm', 'dsp-control-functional-8876.inc', 'DSP_CONTROL_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'dsp-io-functional.asm', 'dsp-io-functional-8876.inc', 'DSP_IO_FUNCTIONAL_8876', 'pmb8876' ],
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

sub tool_identity {
	my ($path) = @_;
	my @stat = stat $path;
	return join(':', $path, $stat[7], $stat[9]);
}

sub cpu_count {
	open my $cpuinfo, '<', '/proc/cpuinfo' or return 1;
	my $count = grep { /^processor\s*:/ } <$cpuinfo>;
	close $cpuinfo;
	return $count || 1;
}

sub build_image {
	my ($image) = @_;
	my ($asm_name, $output_name, $array_name, $cpu) = @$image;
	my $asm_path = File::Spec->catfile($RealBin, $asm_name);
	my $output_path = File::Spec->catfile($RealBin, $output_name);
	my $expanded = expand_teak_constants($cpu, read_file($asm_path));
	my $cache_key = sha256_hex(join("\0", 'v1', $cpu, $expanded, tool_identity($makedsp1), tool_identity($dsp1_reader)));
	my $cached_dsp1 = File::Spec->catfile($cache_dir, "$cache_key.dsp1");
	my $cache_hit = -f $cached_dsp1;

	if (!$cache_hit) {
		my $temp_dir = tempdir('dsp-image-XXXXXX', TMPDIR => 1, CLEANUP => 1);
		my $expanded_path = File::Spec->catfile($temp_dir, "$output_name.asm");
		my $dsp1_path = File::Spec->catfile($temp_dir, "$output_name.dsp1");
		my $dis_path = File::Spec->catfile($temp_dir, "$output_name.dis");

		open my $expanded_file, '>', $expanded_path or die "Cannot write $expanded_path: $!\n";
		print {$expanded_file} $expanded;
		close $expanded_file or die "Cannot close $expanded_path: $!\n";
		run_tool($makedsp1, $expanded_path, $dsp1_path);
		run_tool($dsp1_reader, $dsp1_path, $dis_path);
		copy($dsp1_path, $cached_dsp1) or die "Cannot cache $dsp1_path: $!\n";
	}

	write_image($output_path, $array_name, read_file($cached_dsp1));
	print "$array_name: ".($cache_hit ? 'cached' : 'rebuilt')."\n";
}

die "Usage: $0\n" if @ARGV;
die "makedsp1 is not executable: $makedsp1\n" if !-x $makedsp1;
die "dsp1_reader is not executable: $dsp1_reader\n" if !-x $dsp1_reader;
make_path($cache_dir);
my $jobs = $ENV{DSP_JOBS} // cpu_count();
$jobs = 1 if $jobs < 1;
my @children;
for my $image (@images) {
	while (@children >= $jobs) {
		my $pid = wait;
		die "DSP image builder child $pid failed\n" if $? != 0;
		@children = grep { $_ != $pid } @children;
	}

	my $pid = fork;
	die "Cannot fork DSP image builder: $!\n" if !defined $pid;
	if ($pid == 0) {
		build_image($image);
		exit 0;
	}
	push @children, $pid;
}
for my $child (@children) {
	waitpid($child, 0);
	die "DSP image builder child $child failed\n" if $? != 0;
}
