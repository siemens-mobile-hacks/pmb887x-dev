#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram layout_lines write_text);

my $firmware_slots = <<'ASM';
// Delayed return with the arithmetic delay slots used by the EL71 DSP firmware.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
clr a0 always
mov 0x$1234 r0
addh r0 a0
call 0x0000$0200 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov a0h a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov st0 r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
br 0x0000$0400 always

segment p 0180
br 0x0000$0180 always

segment p 0200
retd
sqr a0h a0
pacr a0 always
ASM

my @scenarios = (
	[ 'retd with two one-cycle slots', 'retd', [ 'inc a1 always', 'inc a1 always' ] ],
	[ 'retd with a long immediate add slot', 'retd', [ 'add 0x$0005 a1' ] ],
	[ 'retd with a long immediate mov slot', 'retd', [ 'mov 0x$1234 r1' ] ],
	[ 'retd with a norm slot', 'retd', [ 'norm a1 [r1]' ] ],
	[ 'retid with two one-cycle slots', 'retid', [ 'inc a1 always', 'inc a1 always' ] ],
	[ 'retid with a long immediate add slot', 'retid', [ 'add 0x$0005 a1' ] ],
);

my $completion_address = 0x0F00;
my $asm = $firmware_slots . "\n// Delay-slot contents for retd and retid.\n";
for my $n (0 .. $#scenarios) {
	my ($name, $return, $slots) = @{$scenarios[$n]};
	my $base = 0x0400 + 0x40 * $n;
	my $next = $n == $#scenarios ? $completion_address : 0x0400 + 0x40 * ($n + 1);
	my $result = 0x0410 + 4 * $n;
	my @lines = layout_lines($base, $base + 0x40,
		'dint', sprintf('mov 0x$%04X a0l', $n + 1), 'mov a0l ' . shared_ram(0x0401), 'clr a1 always',
		'mov 0x$0001 a1l', 'mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1', 'clr a0 always',
		'call 0x0000${SUB} always',
		'mov a1l ' . shared_ram($result), 'mov a0l ' . shared_ram($result + 1), 'mov r1 a0l',
		'mov a0l ' . shared_ram($result + 2), 'mov sp a0l', 'mov a0l ' . shared_ram($result + 3),
		sprintf('br 0x0000$%04X always', $next),
		'SUB:', $return, @$slots, ('inc a0 always') x 3, 'ret always');
	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $base) . join("\n", @lines) . "\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'dint', 'mov 0x$A55A a0l', 'mov a0l ' . shared_ram(0x0400), 'br 0x0000$0F05 always') . "\n";
$asm .= "\nsegment p 0F05\nbr 0x0000\$0F05 always\n";

write_text($ARGV[0] // "$RealBin/delay-slot-functional.asm", $asm);
