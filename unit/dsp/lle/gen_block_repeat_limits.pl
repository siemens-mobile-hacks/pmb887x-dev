#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram layout_lines write_text);

my @reset = ('mov 0x0010 icr', 'nop', 'mov 0x0000 icr', 'nop', 'clr a0 always', 'clr a1 always');
my @nop3 = ('nop') x 3;

sub block_repeat {
	my ($count, $end, @body) = @_;
	return (sprintf('bkrep 0x%04Xu8 0x0000${%s}', $count, $end), @body);
}

my @scenarios = (
	[ 'a fifth nested block repeat level', [
		block_repeat(1, 'E1',
			block_repeat(1, 'E2',
				block_repeat(1, 'E3',
					block_repeat(1, 'E4',
						block_repeat(1, 'E5', 'inc a1 always', @nop3, 'E5:', 'nop'),
						@nop3, 'E4:', 'nop'),
					@nop3, 'E3:', 'nop'),
				@nop3, 'E2:', 'nop'),
			'inc a0 always', @nop3, 'E1:', 'nop'),
	] ],
	[ 'two block repeat levels share the last address', [
		block_repeat(1, 'E', block_repeat(2, 'E', 'inc a1 always', 'nop', 'E:', 'inc a0 always')),
	] ],
	[ 'break as the last instruction of the body', [
		block_repeat(3, 'E', 'inc a1 always', @nop3, 'E:', 'break'), 'inc a0 always',
	] ],
	[ 'a one-word block repeat body', [
		block_repeat(3, 'E', 'E:', 'inc a1 always'), 'inc a0 always',
	] ],
	[ 'a block repeat whose last address precedes it', [
		'B:', 'nop', block_repeat(3, 'B', 'inc a1 always', @nop3), 'inc a0 always',
	] ],
);

my $completion_address = 0x0F00;
my $asm = "// Block-repeat sequences that the TeakLite specification forbids.\n";
$asm .= "segment p 0100\n" . join("\n", 'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_MCS_CFR]', 'dint',
	'br 0x0000$0200 always') . "\n";

for my $n (0 .. $#scenarios) {
	my ($name, $body) = @{$scenarios[$n]};
	my $base = 0x0200 + 0x80 * $n;
	my $next = $n == $#scenarios ? $completion_address : 0x0200 + 0x80 * ($n + 1);
	my $result = 0x0410 + 8 * $n;
	my @lines = layout_lines($base, $base + 0x80, sprintf('mov 0x$%04X a0l', $n + 1),
		'mov a0l ' . shared_ram(0x0401), @reset, @$body,
		'mov a1l ' . shared_ram($result), 'mov a0l ' . shared_ram($result + 1), 'mov lc r0', 'mov r0 a0l',
		'mov a0l ' . shared_ram($result + 2), 'mov icr a0', 'mov a0l ' . shared_ram($result + 3),
		'mov 0x0010 icr', 'nop', sprintf('br 0x0000$%04X always', $next));
	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $base) . join("\n", @lines) . "\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'mov 0x$A55A a0l', 'mov a0l ' . shared_ram(0x0400), 'br 0x0000$0F04 always') . "\n";
$asm .= "\nsegment p 0F04\nbr 0x0000\$0F04 always\n";

write_text($ARGV[0] // "$RealBin/block-repeat-limits.asm", $asm);
