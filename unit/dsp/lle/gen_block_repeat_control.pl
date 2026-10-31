#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram layout_lines write_text);

my @reset = ('mov 0x0010 icr', 'nop', 'mov 0x0000 icr', 'nop', 'clr a0 always', 'clr a1 always', 'mov 0x$0000 r1');
my @nop3 = ('nop') x 3;

sub block_repeat {
	my ($count, $end, @body) = @_;
	return (sprintf('bkrep 0x%04Xu8 0x0000${%s}', $count, $end), @body);
}

my @scenarios = (
	[ 'ICR LP write at the first body word aborts a single loop', [
		block_repeat(4, 'E', 'mov 0x0010 icr', 'inc a1 always', @nop3, 'E:', 'inc a0 always'),
	] ],
	[ 'ICR LP write three words before the end aborts a single loop', [
		block_repeat(4, 'E', 'inc a1 always', 'mov 0x0010 icr', @nop3, 'E:', 'inc a0 always'),
	] ],
	[ 'ICR LP write in the inner loop aborts both levels', [
		block_repeat(2, 'OE',
			block_repeat(3, 'IE', 'inc a1 always', 'mov 0x0010 icr', @nop3, 'IE:', 'nop'),
			'inc a0 always', @nop3, 'OE:', 'nop'),
	] ],
	[ 'ICR LP write in the innermost of four levels aborts all levels', [
		block_repeat(1, 'E1',
			block_repeat(1, 'E2',
				block_repeat(1, 'E3',
					block_repeat(2, 'E4', 'inc a1 always', 'mov 0x0010 icr', @nop3, 'E4:', 'nop'),
					@nop3, 'E3:', 'nop'),
				@nop3, 'E2:', 'nop'),
			'inc a0 always', @nop3, 'E1:', 'nop'),
	] ],
	[ 'ICR write without LP leaves the loop running', [
		block_repeat(4, 'E', 'mov 0x000e icr', 'inc a1 always', @nop3, 'E:', 'inc a0 always'),
	] ],
	[ 'branch out and back to the last instruction keeps looping', [
		block_repeat(3, 'E', 'inc a1 always', 'br 0x0000${OUT} always', 'nop', 'nop', 'E:', 'nop'),
		'br 0x0000${DONE} always',
		'OUT:', 'inc a0 always', 'br 0x0000${E} always',
		'DONE:',
	] ],
	[ 'branch to the last instruction continues the loop', [
		block_repeat(3, 'E', 'inc a1 always', 'br 0x0000${E} always', 'inc a0 always', 'nop', 'nop', 'E:', 'nop'),
	] ],
	[ 'branch out of the body without break leaves the loop active', [
		block_repeat(3, 'E', 'inc a1 always', 'br 0x0000${OUT} always', 'nop', 'nop', 'E:', 'inc a0 always'),
		'OUT:',
	] ],
	[ 'lc reads the loop counter inside the body and after completion', [
		block_repeat(3, 'E', 'inc a1 always', 'mov lc r1', @nop3, 'E:', 'nop'),
	] ],
	[ 'break at the outer level keeps lc', [
		block_repeat(5, 'E', 'inc a1 always', 'cmp 0x$0002 a1', 'br 0x0000${SKIP} neq', 'break', 'mov lc r1',
			'SKIP:', @nop3, 'E:', 'inc a0 always'),
	] ],
	[ 'lc reads the outer counter after the inner loop completes', [
		block_repeat(2, 'OE',
			block_repeat(3, 'IE', 'inc a1 always', @nop3, 'IE:', 'nop'),
			'mov lc r1', 'inc a0 always', @nop3, 'OE:', 'nop'),
	] ],
	[ 'rep inside a block repeat body runs every repetition', [
		block_repeat(3, 'E', 'rep 0x0002u8', 'inc a1 always', @nop3, 'E:', 'inc a0 always'),
	] ],
);

my $completion_address = 0x0F00;
my $asm = "// Block-repeat control: ICR LP aborts, branches around the loop end, and lc reads.\n";
$asm .= "segment p 0100\n" . join("\n", 'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_MCS_CFR]', 'dint',
	'br 0x0000$0200 always') . "\n";

for my $n (0 .. $#scenarios) {
	my ($name, $body) = @{$scenarios[$n]};
	my $base = 0x0200 + 0x80 * $n;
	my $next = $n == $#scenarios ? $completion_address : 0x0200 + 0x80 * ($n + 1);
	my $result = 0x0410 + 8 * $n;
	my @lines = layout_lines($base, $base + 0x80, @reset, @$body,
		'mov a1l ' . shared_ram($result), 'mov a0l ' . shared_ram($result + 1), 'mov r1 a0l',
		'mov a0l ' . shared_ram($result + 2), 'mov lc r0', 'mov r0 a0l', 'mov a0l ' . shared_ram($result + 3),
		'mov icr a0', 'mov a0l ' . shared_ram($result + 4), 'mov 0x0010 icr', 'nop',
		sprintf('br 0x0000$%04X always', $next));
	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $base) . join("\n", @lines) . "\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'mov 0x$A55A a0l', 'mov a0l ' . shared_ram(0x0400), 'br 0x0000$0F04 always') . "\n";
$asm .= "\nsegment p 0F04\nbr 0x0000\$0F04 always\n";

write_text($ARGV[0] // "$RealBin/block-repeat-control.asm", $asm);
