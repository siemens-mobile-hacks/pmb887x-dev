#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use File::Path qw(remove_tree);
use File::Spec;
use File::Temp qw(tempdir);
use FindBin qw($RealBin);
use lib "$RealBin/..";
use TeakTools qw(find_teak_tools);

my ($teak_assembler, $teak_disassembler) = find_teak_tools();
my $generated_dir = File::Spec->catdir($RealBin, 'generated');

sub cpu_count {
	open my $nproc, '-|', 'nproc' or return 1;
	my $count = <$nproc>;
	close $nproc or return 1;
	return $count =~ /^(\d+)$/ && $count > 0 ? $count : 1;
}

my $jobs = $ENV{DSP_JOBS} // cpu_count();
die "DSP_JOBS must be a positive integer\n" unless $jobs =~ /^\d+$/ && $jobs > 0;

sub read_file {
	my ($path) = @_;
	open my $file, '<:raw', $path or die "Cannot read $path: $!\n";
	local $/;
	return <$file>;
}

sub run_tool {
	my (@command) = @_;
	system { $command[0] } @command;
	die "$command[0] failed with status $?\n" if $? != 0;
}

sub write_image {
	my ($path, $shard, $cache_key, $image_path) = @_;
	my $image = read_file($image_path);
	open my $output, '>', $path or die "Cannot write $path: $!\n";
	printf {$output} "// DSP opcode-alias image cache key: %s\n", $cache_key;
	printf {$output} "static const uint8_t DSP_INSTRUCTIONS_IMAGE_%u[] = {\n", $shard;
	for (my $offset = 0; $offset < length($image); $offset += 16) {
		my @bytes = unpack 'C*', substr($image, $offset, 16);
		print {$output} "\t", join(', ', map { sprintf '0x%02x', $_ } @bytes), ",\n";
	}
	print {$output} "};\n";
	close $output or die "Cannot close $path: $!\n";
	chmod 0644, $path or die "Cannot chmod $path: $!\n";
}

sub build_shard {
	my ($shard, $cache_key, $temp_dir) = @_;
	my $asm_path = File::Spec->catfile($generated_dir, "instructions-$shard.asm");
	my $image_path = File::Spec->catfile($temp_dir, "instructions-$shard.bin");
	my $dis_path = File::Spec->catfile($temp_dir, "instructions-$shard.dis");
	my $output_path = File::Spec->catfile($generated_dir, "instructions-image-$shard.inc");
	my $temporary_path = "$output_path.$$";

	run_tool($teak_assembler, '--entry', '0100', $asm_path, $image_path);
	run_tool($teak_disassembler, $image_path, $dis_path);
	die "Invalid instruction in DSP alias shard $shard\n" if read_file($dis_path) =~ /\[ERROR\]/;
	write_image($temporary_path, $shard, $cache_key, $image_path);
	rename $temporary_path, $output_path or die "Cannot replace $output_path: $!\n";
}

run_tool($^X, File::Spec->catfile($RealBin, '..', 'instructions', 'gen_instructions.pl'), '--generate-aliases');
my $cases_path = File::Spec->catfile($RealBin, 'instructions-cases.inc');
my $cases = read_file($cases_path);
my ($shard_count) = $cases =~ /^#define DSP_INSTRUCTION_SHARD_COUNT (\d+)$/m;
die "Cannot read shard count from $cases_path\n" unless defined $shard_count;

my $builder_fingerprint = sha256_hex(
	read_file(__FILE__), read_file($teak_assembler), read_file($teak_disassembler));
my @pending;
my $cached = 0;
for my $shard (0 .. $shard_count - 1) {
	my $asm_path = File::Spec->catfile($generated_dir, "instructions-$shard.asm");
	my $output_path = File::Spec->catfile($generated_dir, "instructions-image-$shard.inc");
	my $cache_key = sha256_hex($builder_fingerprint, read_file($asm_path));
	if (-f $output_path && read_file($output_path) =~ m{^// DSP opcode-alias image cache key: \Q$cache_key\E$}m) {
		$cached++;
		next;
	}
	push @pending, [ $shard, $cache_key ];
}

my $temp_dir = tempdir('dsp-opcode-aliases-XXXXXX', TMPDIR => 1, CLEANUP => 0);
my %active;
my @failed;
while (@pending || %active) {
	while (@pending && keys(%active) < $jobs) {
		my $job = shift @pending;
		my ($shard, $cache_key) = @$job;
		my $log_path = File::Spec->catfile($temp_dir, "instructions-$shard.log");
		my $pid = fork();
		die "Cannot fork: $!\n" unless defined $pid;
		if ($pid == 0) {
			open STDOUT, '>', $log_path or die "Cannot write $log_path: $!\n";
			open STDERR, '>&', \*STDOUT or die "Cannot redirect stderr: $!\n";
			eval { build_shard($shard, $cache_key, $temp_dir) };
			if ($@) {
				print STDERR $@;
				exit 1;
			}
			exit 0;
		}
		$active{$pid} = [ $shard, $log_path ];
	}

	my $pid = wait();
	die "wait failed: $!\n" if $pid < 0;
	my ($shard, $log_path) = @{$active{$pid}};
	delete $active{$pid};
	push @failed, [ $shard, $log_path, $? ] if $? != 0;
}

if (@failed) {
	for my $failure (@failed) {
		my ($shard, $log_path, $status) = @$failure;
		print STDERR "DSP alias shard $shard failed with status $status:\n", read_file($log_path);
	}
	remove_tree($temp_dir);
	die scalar(@failed) . " DSP alias shard(s) failed\n";
}

remove_tree($temp_dir);
printf "DSP alias images: %u cached, %u rebuilt with %u worker(s)\n", $cached, $shard_count - $cached, $jobs;
