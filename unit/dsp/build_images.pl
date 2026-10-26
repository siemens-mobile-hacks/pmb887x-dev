#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use File::Basename qw(basename);
use File::Copy qw(copy);
use File::Path qw(make_path);
use File::Spec;
use File::Temp qw(tempdir);
use FindBin qw($RealBin);
use lib $RealBin;
use DspAsm qw(expand_teak_constants);
use TeakTools qw(find_teak_tools);

my ($teak_assembler, $teak_disassembler) = find_teak_tools();
my $cache_dir = $ENV{DSP_IMAGE_CACHE} // '/tmp/pmb887x-dsp-images';
my @images = (
	[ 'lle/commands-0602.asm', 'lle/commands-0602.inc', 'DSP_TEST_IMAGE_0602', 'pmb8875' ],
	[ 'lle/commands-0604.asm', 'lle/commands-0604.inc', 'DSP_TEST_IMAGE_0604', 'pmb8875' ],
	[ 'lle/commands-0801.asm', 'lle/commands-0801.inc', 'DSP_TEST_IMAGE_0801', 'pmb8876' ],
	[ 'lle/irqs-0602.asm', 'lle/irqs-0602.inc', 'DSP_IRQ_IMAGE_0602', 'pmb8875' ],
	[ 'lle/irqs-0604.asm', 'lle/irqs-0604.inc', 'DSP_IRQ_IMAGE_0604', 'pmb8875' ],
	[ 'lle/irqs-0801.asm', 'lle/irqs-0801.inc', 'DSP_IRQ_IMAGE_0801', 'pmb8876' ],
	[ 'lle/benchmark.asm', 'lle/benchmark-8875.inc', 'DSP_BENCHMARK_8875', 'pmb8875' ],
	[ 'lle/benchmark.asm', 'lle/benchmark-8876.inc', 'DSP_BENCHMARK_8876', 'pmb8876' ],
	[ 'lle/irq-shared-benchmark.asm', 'lle/irq-shared-benchmark-8875.inc', 'DSP_IRQ_SHARED_BENCHMARK_8875', 'pmb8875' ],
	[ 'lle/irq-shared-benchmark.asm', 'lle/irq-shared-benchmark-8876.inc', 'DSP_IRQ_SHARED_BENCHMARK_8876', 'pmb8876' ],
	[ 'lle/interface-concurrency.asm', 'lle/interface-concurrency-8875.inc', 'DSP_INTERFACE_IMAGE_8875', 'pmb8875' ],
	[ 'lle/interface-concurrency.asm', 'lle/interface-concurrency-8876.inc', 'DSP_INTERFACE_IMAGE_8876', 'pmb8876' ],
	[ 'lle/timers-functional.asm', 'lle/timers-functional-8875.inc', 'DSP_TIMERS_IMAGE_8875', 'pmb8875' ],
	[ 'lle/timers-functional.asm', 'lle/timers-functional-8876.inc', 'DSP_TIMERS_IMAGE_8876', 'pmb8876' ],
	[ 'lle/mcs-functional.asm', 'lle/mcs-functional-8875.inc', 'DSP_MCS_IMAGE_8875', 'pmb8875' ],
	[ 'lle/mcs-functional.asm', 'lle/mcs-functional-8876.inc', 'DSP_MCS_IMAGE_8876', 'pmb8876' ],
	[ 'lle/delay-slot-functional.asm', 'lle/delay-slot-functional-8875.inc', 'DSP_DELAY_SLOT_IMAGE_8875', 'pmb8875' ],
	[ 'lle/delay-slot-functional.asm', 'lle/delay-slot-functional-8876.inc', 'DSP_DELAY_SLOT_IMAGE_8876', 'pmb8876' ],
	[ 'lle/interrupt-functional.asm', 'lle/interrupt-functional-8875.inc', 'DSP_INTERRUPT_IMAGE_8875', 'pmb8875' ],
	[ 'lle/interrupt-functional.asm', 'lle/interrupt-functional-8876.inc', 'DSP_INTERRUPT_IMAGE_8876', 'pmb8876' ],
	[ 'lle/interrupt-repeat.asm', 'lle/interrupt-repeat-8875.inc', 'DSP_INTERRUPT_REPEAT_8875', 'pmb8875' ],
	[ 'lle/interrupt-repeat.asm', 'lle/interrupt-repeat-8876.inc', 'DSP_INTERRUPT_REPEAT_8876', 'pmb8876' ],
	[ 'lle/interrupt-block-repeat.asm', 'lle/interrupt-block-repeat-8875.inc',
		'DSP_INTERRUPT_BLOCK_REPEAT_8875', 'pmb8875' ],
	[ 'lle/interrupt-block-repeat.asm', 'lle/interrupt-block-repeat-8876.inc',
		'DSP_INTERRUPT_BLOCK_REPEAT_8876', 'pmb8876' ],
	[ 'lle/clock-domains.asm', 'lle/clock-domains-8875.inc', 'DSP_CLOCK_DOMAINS_8875', 'pmb8875' ],
	[ 'lle/clock-domains.asm', 'lle/clock-domains-8876.inc', 'DSP_CLOCK_DOMAINS_8876', 'pmb8876' ],
	[ 'lle/interrupt-sources-a0.asm', 'lle/interrupt-sources-a0-8875.inc', 'DSP_INTERRUPT_SOURCES_A0_8875', 'pmb8875' ],
	[ 'lle/interrupt-sources-a0.asm', 'lle/interrupt-sources-a0-8876.inc', 'DSP_INTERRUPT_SOURCES_A0_8876', 'pmb8876' ],
	[ 'lle/interrupt-sources-b0.asm', 'lle/interrupt-sources-b0-8875.inc', 'DSP_INTERRUPT_SOURCES_B0_8875', 'pmb8875' ],
	[ 'lle/interrupt-sources-b0.asm', 'lle/interrupt-sources-b0-8876.inc', 'DSP_INTERRUPT_SOURCES_B0_8876', 'pmb8876' ],
	[ 'lle/interrupt-sources-1.asm', 'lle/interrupt-sources-1-8875.inc', 'DSP_INTERRUPT_SOURCES_1_8875', 'pmb8875' ],
	[ 'lle/interrupt-sources-1.asm', 'lle/interrupt-sources-1-8876.inc', 'DSP_INTERRUPT_SOURCES_1_8876', 'pmb8876' ],
	[ 'lle/interrupt-sources-2.asm', 'lle/interrupt-sources-2-8875.inc', 'DSP_INTERRUPT_SOURCES_2_8875', 'pmb8875' ],
	[ 'lle/interrupt-sources-2.asm', 'lle/interrupt-sources-2-8876.inc', 'DSP_INTERRUPT_SOURCES_2_8876', 'pmb8876' ],
	[ 'lle/ssc-functional.asm', 'lle/ssc-functional-8875.inc', 'DSP_SSC_IMAGE_8875', 'pmb8875' ],
	[ 'lle/ssc-functional.asm', 'lle/ssc-functional-8876.inc', 'DSP_SSC_IMAGE_8876', 'pmb8876' ],
	[ 'lle/cipher-a512-functional.asm', 'lle/cipher-a512-functional-8875.inc', 'DSP_CIPHER_A512_IMAGE_8875', 'pmb8875' ],
	[ 'lle/cipher-a512-functional.asm', 'lle/cipher-a512-functional-8876.inc', 'DSP_CIPHER_A512_IMAGE_8876', 'pmb8876' ],
	[ 'lle/cipher-a53-functional.asm', 'lle/cipher-a53-functional-8875.inc', 'DSP_CIPHER_A53_IMAGE_8875', 'pmb8875' ],
	[ 'lle/cipher-a53-functional.asm', 'lle/cipher-a53-functional-8876.inc', 'DSP_CIPHER_A53_IMAGE_8876', 'pmb8876' ],
	[ 'lle/cipher-kgcore-vectors.asm', 'lle/cipher-kgcore-vectors-8875.inc', 'DSP_CIPHER_KGCORE_IMAGE_8875', 'pmb8875' ],
	[ 'lle/cipher-kgcore-vectors.asm', 'lle/cipher-kgcore-vectors-8876.inc', 'DSP_CIPHER_KGCORE_IMAGE_8876', 'pmb8876' ],
	[ 'lle/channel-decoder-functional.asm', 'lle/channel-decoder-functional-8875.inc', 'DSP_CHANNEL_DECODER_IMAGE_8875', 'pmb8875' ],
	[ 'lle/channel-decoder-functional.asm', 'lle/channel-decoder-functional-8876.inc', 'DSP_CHANNEL_DECODER_IMAGE_8876', 'pmb8876' ],
	[ 'lle/channel-decoder-vector-runner.asm', 'lle/channel-decoder-vector-runner-8875.inc', 'DSP_CHANNEL_DECODER_VECTOR_RUNNER_8875', 'pmb8875' ],
	[ 'lle/channel-decoder-vector-runner.asm', 'lle/channel-decoder-vector-runner-8876.inc', 'DSP_CHANNEL_DECODER_VECTOR_RUNNER_8876', 'pmb8876' ],
	[ 'lle/sch-decoder-vector-runner.asm', 'lle/sch-decoder-vector-runner-8875.inc',
		'DSP_SCH_DECODER_VECTOR_RUNNER_8875', 'pmb8875' ],
	[ 'lle/sch-decoder-vector-runner.asm', 'lle/sch-decoder-vector-runner-8876.inc',
		'DSP_SCH_DECODER_VECTOR_RUNNER_8876', 'pmb8876' ],
	[ 'lle/equalizer-functional.asm', 'lle/equalizer-functional-8875.inc', 'DSP_EQUALIZER_IMAGE_8875', 'pmb8875' ],
	[ 'lle/equalizer-functional.asm', 'lle/equalizer-functional-8876.inc', 'DSP_EQUALIZER_IMAGE_8876', 'pmb8876' ],
	[ 'lle/equalizer-vector-runner.asm', 'lle/equalizer-vector-runner-8875.inc', 'DSP_EQUALIZER_VECTOR_RUNNER_8875', 'pmb8875' ],
	[ 'lle/equalizer-vector-runner.asm', 'lle/equalizer-vector-runner-8876.inc', 'DSP_EQUALIZER_VECTOR_RUNNER_8876', 'pmb8876' ],
	[ 'lle/equalizer-output-runner.asm', 'lle/equalizer-output-runner-8875.inc', 'DSP_EQUALIZER_OUTPUT_RUNNER_8875', 'pmb8875' ],
	[ 'lle/equalizer-output-runner.asm', 'lle/equalizer-output-runner-8876.inc', 'DSP_EQUALIZER_OUTPUT_RUNNER_8876', 'pmb8876' ],
	[ 'lle/equalizer-burst-runner.asm', 'lle/equalizer-burst-runner-8875.inc', 'DSP_EQUALIZER_BURST_RUNNER_8875', 'pmb8875' ],
	[ 'lle/equalizer-burst-runner.asm', 'lle/equalizer-burst-runner-8876.inc', 'DSP_EQUALIZER_BURST_RUNNER_8876', 'pmb8876' ],
	[ 'lle/i2s-functional.asm', 'lle/i2s-functional-8875.inc', 'DSP_I2S_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/i2s-functional.asm', 'lle/i2s-functional-8876.inc', 'DSP_I2S_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/i2s-rx-functional.asm', 'lle/i2s-rx-functional-8875.inc', 'DSP_I2S_RX_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/i2s-rx-functional.asm', 'lle/i2s-rx-functional-8876.inc', 'DSP_I2S_RX_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/i2s-rx-pcm-functional.asm', 'lle/i2s-rx-pcm-functional-8875.inc',
		'DSP_I2S_RX_PCM_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/i2s-rx-pcm-functional.asm', 'lle/i2s-rx-pcm-functional-8876.inc', 'DSP_I2S_RX_PCM_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/i2s-clock-select.asm', 'lle/i2s-clock-select-8875.inc', 'DSP_I2S_CLOCK_SELECT_8875', 'pmb8875' ],
	[ 'lle/i2s-clock-select.asm', 'lle/i2s-clock-select-8876.inc', 'DSP_I2S_CLOCK_SELECT_8876', 'pmb8876' ],
	[ 'lle/afe-functional.asm', 'lle/afe-functional-8875.inc', 'DSP_AFE_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/afe-functional.asm', 'lle/afe-functional-8876.inc', 'DSP_AFE_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/modulator-functional.asm', 'lle/modulator-functional-8875.inc', 'DSP_MODULATOR_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/modulator-functional.asm', 'lle/modulator-functional-8876.inc', 'DSP_MODULATOR_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/baseband-functional.asm', 'lle/baseband-functional-8875.inc', 'DSP_BASEBAND_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/baseband-functional.asm', 'lle/baseband-functional-8876.inc', 'DSP_BASEBAND_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/tpu-events.asm', 'lle/tpu-events-8875.inc', 'DSP_TPU_EVENTS_8875', 'pmb8875' ],
	[ 'lle/tpu-events.asm', 'lle/tpu-events-8876.inc', 'DSP_TPU_EVENTS_8876', 'pmb8876' ],
	[ 'lle/tpu-bb-events.asm', 'lle/tpu-bb-events-8875.inc', 'DSP_TPU_BB_EVENTS_8875', 'pmb8875' ],
	[ 'lle/tpu-bb-events.asm', 'lle/tpu-bb-events-8876.inc', 'DSP_TPU_BB_EVENTS_8876', 'pmb8876' ],
	[ 'lle/dsp-control-functional.asm', 'lle/dsp-control-functional-8875.inc', 'DSP_CONTROL_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/dsp-control-functional.asm', 'lle/dsp-control-functional-8876.inc', 'DSP_CONTROL_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/program-banks-8875.asm', 'lle/program-banks-8875.inc', 'DSP_PROGRAM_BANKS_8875', 'pmb8875' ],
	[ 'lle/program-banks.asm', 'lle/program-banks-8876.inc', 'DSP_PROGRAM_BANKS_8876', 'pmb8876' ],
	[ 'lle/dsp-io-functional.asm', 'lle/dsp-io-functional-8875.inc', 'DSP_IO_FUNCTIONAL_8875', 'pmb8875' ],
	[ 'lle/dsp-io-functional.asm', 'lle/dsp-io-functional-8876.inc', 'DSP_IO_FUNCTIONAL_8876', 'pmb8876' ],
	[ 'lle/data-ram.asm', 'lle/data-ram-8875.inc', 'DSP_DATA_RAM_8875', 'pmb8875' ],
	[ 'lle/data-ram.asm', 'lle/data-ram-8876.inc', 'DSP_DATA_RAM_8876', 'pmb8876' ],
);
my @embedded_images = (
	[ '../../rom/dsp/cx75/0602/container.bin', 'hle/user-rom-siemens-0602.inc',
		'DSP_USER_ROM_SIEMENS_0602' ],
	[ '../../rom/dsp/cx75/0605/container.bin', 'hle/user-rom-siemens-0605.inc',
		'DSP_USER_ROM_SIEMENS_0605' ],
	[ '../../rom/dsp/vs7/0603/container.bin', 'hle/user-rom-apoxi-0603.inc',
		'DSP_USER_ROM_APOXI_0603' ],
	[ '../../rom/dsp/vs7/0604/container.bin', 'hle/user-rom-apoxi-0604.inc',
		'DSP_USER_ROM_APOXI_0604' ],
	[ '../../rom/dsp/el71/0800/container.bin', 'hle/user-rom-siemens-0800.inc',
		'DSP_USER_ROM_SIEMENS_0800' ],
	[ '../../rom/dsp/el71/0801/container.bin', 'hle/user-rom-siemens-0801.inc',
		'DSP_USER_ROM_SIEMENS_0801' ],
	[ '../../rom/dsp/sl98/0800-container.bin', 'hle/user-rom-apoxi-0800.inc',
		'DSP_USER_ROM_APOXI_0800' ],
	[ '../../rom/dsp/sl98/0801-container.bin', 'hle/user-rom-apoxi-0801.inc',
		'DSP_USER_ROM_APOXI_0801' ],
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
	my $cache_key = sha256_hex(join("\0", 'v3', $cpu, $expanded,
		tool_identity($teak_assembler), tool_identity($teak_disassembler)));
	my $cached_image = File::Spec->catfile($cache_dir, "$cache_key.bin");
	my $cache_hit = -f $cached_image;

	if (!$cache_hit) {
		my $temp_dir = tempdir('dsp-image-XXXXXX', TMPDIR => 1, CLEANUP => 1);
		my $temp_name = basename($output_name);
		my $expanded_path = File::Spec->catfile($temp_dir, "$temp_name.asm");
		my $image_path = File::Spec->catfile($temp_dir, "$temp_name.bin");
		my $dis_path = File::Spec->catfile($temp_dir, "$temp_name.dis");

		open my $expanded_file, '>', $expanded_path or die "Cannot write $expanded_path: $!\n";
		print {$expanded_file} $expanded;
		close $expanded_file or die "Cannot close $expanded_path: $!\n";
		run_tool($teak_assembler, '--entry', '0100', $expanded_path, $image_path);
		run_tool($teak_disassembler, $image_path, $dis_path);
		die "$asm_name contains an invalid instruction\n" if read_file($dis_path) =~ /\[ERROR\]/;
		copy($image_path, $cached_image) or die "Cannot cache $image_path: $!\n";
	}

	write_image($output_path, $array_name, read_file($cached_image));
	print "$array_name: ".($cache_hit ? 'cached' : 'rebuilt')."\n";
}

die "Usage: $0\n" if @ARGV;
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
for my $image (@embedded_images) {
	my ($input_name, $output_name, $array_name) = @$image;
	my $input_path = File::Spec->catfile($RealBin, $input_name);
	my $output_path = File::Spec->catfile($RealBin, $output_name);

	write_image($output_path, $array_name, read_file($input_path));
	print "$array_name: embedded\n";
}
