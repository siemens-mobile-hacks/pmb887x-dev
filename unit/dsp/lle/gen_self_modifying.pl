#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram layout_lines write_text);

my $inc_a0 = 0x67D0;
my $dec_a1 = 0x77E0;
my $source = 0x0480;
my @nop3 = ('nop') x 3;
my $movd = 'data 5F80 // movd (r0),(r4); PDF Table 4-4';
my $calla = 'data D480 // calla a0l; PDF Table 4-4';

sub stage_word {
	my ($value) = @_;
	return (sprintf('mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x%04X) r0', $source), sprintf('mov 0x$%04X r3', $value),
		'mov r3 [r0]');
}

sub rewrite {
	my ($label, $value) = @_;
	return (stage_word($value), "mov 0x\${$label} r4", $movd);
}

my @scenarios;
for my $distance (1 .. 4) {
	my @body = (stage_word($inc_a0), 'mov 0x${T} r4', 'clr a0 always', 'clr a1 always', $movd);
	for my $word (1 .. 4) {
		push @body, 'T:' if $word == $distance;
		push @body, 'inc a1 always';
	}
	push @scenarios, [ "movd rewrites the word $distance after itself", [ @body, @nop3 ], 'T' ];
}
push @scenarios,
	[ 'movd rewrites an executed subroutine twice', [
		'clr a0 always', 'clr a1 always', 'call 0x0000${SUB} always',
		rewrite('SUB', $inc_a0), @nop3, 'call 0x0000${SUB} always',
		rewrite('SUB', $dec_a1), @nop3, 'call 0x0000${SUB} always',
		'br 0x0000${DONE} always',
		'SUB:', 'inc a1 always', 'ret always',
		'DONE:',
	], 'SUB' ],
	[ 'movd rewrites the active block repeat body', [
		stage_word($inc_a0), 'mov 0x${T} r4', 'clr a0 always', 'clr a1 always',
		'bkrep 0x0003u8 0x0000${E}', 'T:', 'inc a1 always', $movd, @nop3, 'E:', 'nop',
	], 'T' ],
	[ 'movd rewrites a calla target between calls', [
		'clr a1 always', 'mov 0x${SUB} r2', 'mov r2 a0l', $calla,
		rewrite('SUB', $inc_a0), @nop3, 'clr a0 always', 'mov r2 a0l', $calla,
		'br 0x0000${DONE} always',
		'SUB:', 'inc a1 always', 'ret always',
		'DONE:',
	], 'SUB' ];

my $completion_address = 0x0F00;
my $asm = "// Self-modifying program code written by movd: pipeline distance, executed code, and block repeats.\n";
$asm .= "segment p 0100\n" . join("\n", 'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_MCS_CFR]', 'dint',
	'br 0x0000$0200 always') . "\n";

for my $n (0 .. $#scenarios) {
	my ($name, $body, $readback) = @{$scenarios[$n]};
	my $base = 0x0200 + 0x80 * $n;
	my $next = $n == $#scenarios ? $completion_address : 0x0200 + 0x80 * ($n + 1);
	my $result = 0x0410 + 4 * $n;
	my @lines = layout_lines($base, $base + 0x80, @$body,
		'mov a1l ' . shared_ram($result), 'mov a0l ' . shared_ram($result + 1), "mov 0x\${$readback} a0l",
		'data 0041 // movp (a0l),r1; PDF Table 4-4', 'mov r1 a0l', 'mov a0l ' . shared_ram($result + 2),
		sprintf('br 0x0000$%04X always', $next));
	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $base) . join("\n", @lines) . "\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'mov 0x$A55A a0l', 'mov a0l ' . shared_ram(0x0400), 'br 0x0000$0F04 always') . "\n";
$asm .= "\nsegment p 0F04\nbr 0x0000\$0F04 always\n";

write_text($ARGV[0] // "$RealBin/self-modifying.asm", $asm);
