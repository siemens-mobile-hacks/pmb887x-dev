#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram layout_lines write_text);

my $source = 'mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0';
my $movp = 'data 0041 // movp (a0l),r1; PDF Table 4-4';

my @scenarios = (
	[ 'rep push stores four words that pop returns', [
		'mov 0x$1234 r1', 'rep 0x0003u8', 'push r1', 'pop r1', 'pop r1', 'pop r1', 'mov 0x$0000 r1', 'pop r1',
	] ],
	[ 'rep pop loads the first pushed word last', [
		'push 0x$1111', 'push 0x$2222', 'push 0x$3333', 'push 0x$4444', 'rep 0x0003u8', 'pop r1',
	] ],
	[ 'rep movp rereads the same program word', [ 'mov 0x${DATA} a0l', 'rep 0x0003u8', $movp ] ],
	[ 'rep movd copies four data words to program memory', [
		$source, 'mov 0x${DATA} r4',
		'mov 0x$5A01 r3', 'mov r3 [r0++]', 'mov 0x$5A02 r3', 'mov r3 [r0++]', 'mov 0x$5A03 r3', 'mov r3 [r0++]',
		'mov 0x$5A04 r3', 'mov r3 [r0]', $source, 'rep 0x0003u8', 'data 5FA8 // movd (r0++),(r4++); PDF Table 4-4',
		'nop', 'nop', 'mov 0x${LAST} a0l', $movp,
	] ],
	[ 'rep mov register to icr', [
		'mov 0x$000E r1', 'rep 0x0003u8', 'mov r1 icr', 'mov icr a0', 'mov a0l r1', 'mov 0x0000 icr',
	] ],
	[ 'rep dint keeps interrupts disabled', [ 'eint', 'rep 0x0003u8', 'dint', 'mov st0 r1' ] ],
	[ 'rep break leaves the block repeat after one iteration', [
		'bkrep 0x0003u8 0x0000${E}', 'inc a1 always', 'rep 0x0000u8', 'break', 'nop', 'nop', 'nop', 'E:',
		'inc a0 always', 'mov a1l r1',
	] ],
);

my $completion_address = 0x0F00;
my $asm = "// Single-instruction repeat of instructions that are allowed to repeat.\n";
$asm .= "segment p 0100\n" . join("\n", 'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_MCS_CFR]', 'dint',
	'br 0x0000$0200 always') . "\n";

for my $n (0 .. $#scenarios) {
	my ($name, $body) = @{$scenarios[$n]};
	my $base = 0x0200 + 0x80 * $n;
	my $next = $n == $#scenarios ? $completion_address : 0x0200 + 0x80 * ($n + 1);
	my $result = 0x0410 + 8 * $n;
	my @lines = layout_lines($base, $base + 0x80,
		'mov 0x0010 icr', 'nop', 'mov 0x0000 icr', 'nop', 'clr a0 always', 'clr a1 always', 'mov 0x$0000 r1',
		'mov sp a0l', 'mov a0l ' . shared_ram($result + 4), 'clr a0 always',
		@$body,
		'mov a1l ' . shared_ram($result), 'mov a0l ' . shared_ram($result + 1), 'mov r1 a0l',
		'mov a0l ' . shared_ram($result + 2), 'mov sp a0l', 'mov a0l ' . shared_ram($result + 3),
		'mov ' . shared_ram($result + 4) . ' a0', 'mov a0l sp', sprintf('br 0x0000$%04X always', $next),
		'DATA:', 'data 1111', 'data 2222', 'data 3333', 'LAST:', 'data 4444');
	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $base) . join("\n", @lines) . "\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'mov 0x$A55A a0l', 'mov a0l ' . shared_ram(0x0400), 'br 0x0000$0F04 always') . "\n";
$asm .= "\nsegment p 0F04\nbr 0x0000\$0F04 always\n";

write_text($ARGV[0] // "$RealBin/repeat-contents.asm", $asm);
