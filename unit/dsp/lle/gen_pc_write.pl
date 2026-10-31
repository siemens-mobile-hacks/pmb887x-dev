#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram write_text);

my $marker = 0x0400;
my $result = 0x0410;
my $scratch = 0x0402;

my @scenarios = (
	[ 'ret', sub { ("mov 0x\$$_[0] r1", 'push r1') }, [ 'ret always' ] ],
	[ 'mov r0 pc', sub { ("mov 0x\$$_[0] r0") }, [ 'data 5980 // mov r0, pc; PDF Table 4-4' ] ],
	[ 'mov a0l pc', sub { ('clr a0 always', "mov 0x\$$_[0] a0l") }, [ 'data 599A // mov a0l, pc; PDF Table 4-4' ] ],
	[ 'mov long immediate pc', sub { () },
		[ 'data 5E0C // mov ##long immediate, pc; PDF Table 4-4', 'data TARGET // target' ] ],
	[ 'pop pc', sub { ("mov 0x\$$_[0] r1", 'push r1') }, [ 'data 5E6C // pop pc; PDF Table 4-4' ] ],
	[ 'mov mixp pc', sub { ("mov 0x\$$_[0] r0", 'mov r0 mixp') }, [ 'data 47CC // mov mixp, pc; PDF Table 4-4' ] ],
	[ 'mov indirect r0 pc',
		sub { ("mov 0x\$$_[0] a0l", 'mov a0l ' . shared_ram($scratch),
			sprintf('mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x%04X) r0', $scratch)) },
		[ 'data 1D80 // mov (r0), pc; PDF Table 4-4' ] ],
	[ 'movp indirect a0l pc', sub { ('clr a0 always', "mov 0x\$$_[1] a0l") },
		[ 'data 004C // movp (a0l), pc; PDF Table 4-4' ] ],
	[ 'mov stack top pc', sub { ("mov 0x\$$_[0] r1", 'push r1') }, [ 'data 47EC // mov (sp), pc; PDF Table 4-4' ] ],
);

my $completion_address = 0x0A80;
my $asm = "// Program-counter writes: executed following words, target, and stack pointer.\n";
$asm .= "segment p 0100\n" . join("\n", 'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_MCS_CFR]', 'dint',
	'br 0x0000$0200 always') . "\n";

for my $n (0 .. $#scenarios) {
	my ($name, $prepare, $write) = @{$scenarios[$n]};
	my $setup_address = 0x0200 + 0x40 * $n;
	my $target = sprintf('%04X', 0x0700 + 0x20 * $n);
	my $program_word = sprintf('%04X', 0x0A00 + $n);
	my $next = sprintf('br 0x0000$%04X always', $n == $#scenarios ? $completion_address : 0x0200 + 0x40 * ($n + 1));
	my $base = $result + 4 * $n;

	my @setup = ('mov sp a0l', 'mov a0l ' . shared_ram($base + 3), 'clr a1 always', $prepare->($target, $program_word),
		(map { s/TARGET/$target/r } @$write), ('inc a1 always') x 4, 'mov 0x$FA11 a0l',
		'mov a0l ' . shared_ram($base + 2), $next);
	my @landing = ('mov a1l ' . shared_ram($base), 'mov sp a0l', 'mov a0l ' . shared_ram($base + 1),
		'mov ' . shared_ram($base + 3) . ' a0', 'mov a0l sp', $next);

	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $setup_address) . join("\n", @setup) . "\n";
	$asm .= "\nsegment p $target\n" . join("\n", @landing) . "\n";
	$asm .= "\nsegment p $program_word\ndata $target // movp target\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'mov 0x$A55A a0l', 'mov a0l ' . shared_ram($marker), 'br 0x0000$0A84 always') . "\n";
$asm .= "\nsegment p 0A84\nbr 0x0000\$0A84 always\n";

write_text($ARGV[0] // "$RealBin/pc-write.asm", $asm);
