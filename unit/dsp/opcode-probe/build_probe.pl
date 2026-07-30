#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use File::Spec;
use File::Temp qw(tempdir);
use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspAsm qw(expand_teak_constants);

my $makedsp1 = $ENV{MAKEDSP1} // '/home/azq2/build/teakra/build/src/makedsp1/makedsp1';
my $dsp1_reader = $ENV{DSP1_READER} // '/home/azq2/build/teakra/build/src/dsp1_reader/dsp1_reader';
my ($asm_name, $output_name, $array_name, $mode) = @ARGV;
$asm_name //= 'probe.asm';
$output_name //= 'probe-image.inc';
$array_name //= 'DSP_OPCODE_PROBE_IMAGE';
$mode //= 'raw';
die "Invalid input name: $asm_name\n" unless $asm_name =~ /\A[A-Za-z0-9_.-]+\z/;
die "Invalid output name: $output_name\n" unless $output_name =~ /\A[A-Za-z0-9_.-]+\z/;
die "Invalid C array name: $array_name\n" unless $array_name =~ /\A[A-Z][A-Z0-9_]+\z/;
die "Invalid build mode: $mode\n" unless $mode eq 'raw' || $mode eq 'expansion';
my $asm_path = File::Spec->catfile($RealBin, $asm_name);
my $output_path = File::Spec->catfile($RealBin, $output_name);
my $max_segment_words = 2043;

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

sub expansion_capture {
	return (
		'segment p 0162',
		'mov st0 r1',
		'mov st1 r2',
		'mov st2 r3',
		'load 0x00d3u8 page',
		'mov r1 [page:0x0010u8]',
		'mov r2 [page:0x0011u8]',
		'mov r3 [page:0x0012u8]',
		'mov a0l [page:0x0013u8]',
		'mov a0h [page:0x0014u8]',
		'mov a1l [page:0x0015u8]',
		'mov a1h [page:0x0016u8]',
		'mov r0 [page:0x0017u8]',
		'mov b0 a0',
		'mov a0l [page:0x0018u8]',
		'mov a0h [page:0x0019u8]',
		'mov st0 r1',
		'mov r1 [page:0x001au8]',
		'mov b1 a0',
		'mov a0l [page:0x001bu8]',
		'mov a0h [page:0x001cu8]',
		'mov st0 r1',
		'mov r1 [page:0x001du8]',
		'data 5B0B // mov p,a0; PDF Table 4-4',
		'mov a0l [page:0x001eu8]',
		'mov a0h [page:0x001fu8]',
		'mov [0x$D6A0] a0',
		'mov a0l [page:0x0020u8]',
		'mov sp r0',
		'mov r0 [page:0x0021u8]',
		'mov [sp] r0',
		'mov r0 [page:0x0022u8]',
		'mov 0x0052u8 a0l',
		'mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]',
		'mov 0x0053u8 a0l',
		'mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]',
		'br 0x0000$01c0 always',
		'segment p 01c0',
		'br 0x0000$01c0 always',
	);
}

sub expand_asm {
	my ($source) = @_;
	my @output;
	my $skip = '';
	for my $line (split /\n/, $source) {
		if ($mode eq 'expansion' && $line eq '// @expansion-page-begin') {
			push @output,
				'br 0x0000$0154 always',
				'segment p 0154',
				'load 0x0000u8 page',
				'br 0x0000$0160 always',
				('data 0000') x 9;
			$skip = 'page';
		} elsif ($mode eq 'expansion' && $line eq '// @expansion-page-end') {
			$skip = '';
		} elsif ($mode eq 'expansion' && $line eq '// @post-begin') {
			push @output, expansion_capture();
			$skip = 'post';
		} elsif ($mode eq 'expansion' && $line eq '// @post-end') {
			$skip = '';
		} elsif ($mode eq 'expansion' && $line eq '// @forward-fill-begin') {
			$skip = 'forward';
		} elsif ($mode eq 'expansion' && $line eq '// @forward-fill-end') {
			$skip = '';
		} elsif ($skip ne '') {
			next;
		} elsif ($line =~ m{^// \@fill-current ([0-9A-F]{4}) ([0-9A-F]{4})$}) {
			my ($address, $last) = (hex($1), hex($2));
			push @output, ('data 0000') x ($last - $address + 1);
		} elsif ($line =~ m{^// \@fill-program ([0-9A-F]{4}) ([0-9A-F]{4})$}) {
			my ($address, $last) = (hex($1), hex($2));
			while ($address <= $last) {
				my $words = $last - $address + 1;
				$words = $max_segment_words if $words > $max_segment_words;
				push @output, sprintf('segment p %04X', $address), ('data 0000') x $words;
				$address += $words;
			}
		} else {
			push @output, $line;
		}
	}
	return join("\n", @output) . "\n";
}

die "makedsp1 is not executable: $makedsp1\n" unless -x $makedsp1;
die "dsp1_reader is not executable: $dsp1_reader\n" unless -x $dsp1_reader;
my $dsp_asm_path = File::Spec->catfile($RealBin, '..', 'DspAsm.pm');
my $cache_key = sha256_hex($mode, read_file(__FILE__), read_file($dsp_asm_path), read_file($asm_path),
	read_file($makedsp1), read_file($dsp1_reader));
if (-f $output_path && read_file($output_path) =~ m{^// DSP image \Q$array_name\E cache key: \Q$cache_key\E$}m) {
	print "$array_name: cached\n";
	exit 0;
}

my $temp_dir = tempdir('dsp-opcode-probe-XXXXXX', TMPDIR => 1, CLEANUP => !$ENV{DSP_KEEP_TEMP});
my $expanded_asm_path = File::Spec->catfile($temp_dir, 'probe.asm');
my $dsp1_path = File::Spec->catfile($temp_dir, 'probe.dsp1');
my $dis_path = File::Spec->catfile($temp_dir, 'probe.dis');
open my $expanded_asm, '>', $expanded_asm_path or die "Cannot write $expanded_asm_path: $!\n";
print {$expanded_asm} expand_teak_constants('pmb8876', expand_asm(read_file($asm_path)));
close $expanded_asm or die "Cannot close $expanded_asm_path: $!\n";
run_tool($makedsp1, $expanded_asm_path, $dsp1_path);
run_tool($dsp1_reader, $dsp1_path, $dis_path);
my $image = read_file($dsp1_path);

open my $output, '>', $output_path or die "Cannot write $output_path: $!\n";
print {$output} "// DSP image $array_name cache key: $cache_key\n";
print {$output} "static const uint8_t $array_name\[\] = {\n";
for (my $offset = 0; $offset < length($image); $offset += 16) {
	my @bytes = unpack 'C*', substr($image, $offset, 16);
	print {$output} "\t", join(', ', map { sprintf '0x%02x', $_ } @bytes), ",\n";
}
print {$output} "};\n";
close $output or die "Cannot close $output_path: $!\n";
chmod 0644, $output_path or die "Cannot chmod $output_path: $!\n";
print "$array_name: rebuilt\n";
