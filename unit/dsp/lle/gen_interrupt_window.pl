#!/usr/bin/env perl
use strict;
use warnings;

use FindBin qw($RealBin);
use lib "$RealBin/..";
use DspLayout qw(shared_ram instruction_words write_text);

my $count = 0x0402;
my $return_address = 0x0403;
my $marker = 0x0400;
my $result = 0x0410;
my @entry_order = (0x0405, 0x0406, 0x0407);
my @nop4 = ('nop') x 4;

use constant {
	SOURCE_INT0 => 0,
	SOURCE_INT2 => 2,
	SOURCE_RAISED_IN_WINDOW => 3,
	SOURCE_ALL => 4,
};

my @scenarios = (
	[ 'eint opens a four-nop window', SOURCE_INT0, [], [ 'eint' ], [ @nop4 ], [ 'dint' ] ],
	[ 'set st0 IE opens a four-nop window', SOURCE_INT0, [], [ 'set 0x$0002 st0' ], [ @nop4 ], [ 'rst 0x$0002 st0' ] ],
	[ 'mov long immediate st0 opens a four-nop window', SOURCE_INT0, [], [ 'mov 0x$0006 st0' ], [ @nop4 ],
		[ 'mov 0x$0004 st0' ] ],
	[ 'pop st0 opens a four-nop window', SOURCE_INT0, [ 'mov 0x$0006 r1', 'push r1' ], [ 'pop st0' ], [ @nop4 ],
		[ 'dint' ] ],
	[ 'set st0 IM0 opens a four-nop window', SOURCE_INT0, [ 'rst 0x$0004 st0', 'eint' ], [ 'set 0x$0004 st0' ],
		[ @nop4 ], [ 'rst 0x$0004 st0' ] ],
	[ 'set st2 IM2 opens a four-nop window', SOURCE_INT2, [ 'rst 0x$0040 st2', 'eint' ], [ 'set 0x$0040 st2' ],
		[ @nop4 ], [ 'rst 0x$0040 st2' ] ],
	[ 'source raised inside an enabled window', SOURCE_RAISED_IN_WINDOW, [ 'eint' ],
		[ 'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_INT_SINTA0]' ], [ @nop4 ], [ 'dint' ] ],
	[ 'eint directly followed by dint', SOURCE_INT0, [], [ 'eint' ], [], [ 'dint' ] ],
	[ 'eint nop dint', SOURCE_INT0, [], [ 'eint' ], [ 'nop' ], [ 'dint' ] ],
	[ 'set st0 IE directly followed by rst', SOURCE_INT0, [], [ 'set 0x$0002 st0' ], [], [ 'rst 0x$0002 st0' ] ],
	[ 'set st0 IE nop rst', SOURCE_INT0, [], [ 'set 0x$0002 st0' ], [ 'nop' ], [ 'rst 0x$0002 st0' ] ],
	[ 'mov long immediate st0 directly followed by mov', SOURCE_INT0, [], [ 'mov 0x$0006 st0' ], [],
		[ 'mov 0x$0004 st0' ] ],
	[ 'pop st0 directly followed by dint', SOURCE_INT0, [ 'mov 0x$0006 r1', 'push r1' ], [ 'pop st0' ], [],
		[ 'dint' ] ],
	[ 'mov long immediate st2 opens an IM2 window', SOURCE_INT2, [ 'rst 0x$0040 st2', 'eint' ],
		[ 'mov 0x$0040 st2' ], [ @nop4 ], [ 'rst 0x$0040 st2' ] ],
	[ 'pop st2 opens an IM2 window', SOURCE_INT2, [ 'rst 0x$0040 st2', 'eint', 'mov 0x$0040 r1', 'push r1' ],
		[ 'pop st2' ], [ @nop4 ], [ 'rst 0x$0040 st2' ] ],
	[ 'mov register st0 opens a window', SOURCE_INT0, [ 'mov 0x$0006 r1' ], [ 'mov r1 st0' ], [ @nop4 ],
		[ 'dint' ] ],
	[ 'mov register st2 opens an IM2 window', SOURCE_INT2, [ 'rst 0x$0040 st2', 'eint', 'mov 0x$0040 r1' ],
		[ 'mov r1 st2' ], [ @nop4 ], [ 'rst 0x$0040 st2' ] ],
	[ 'INT0 INT1 and INT2 pending together', SOURCE_ALL, [], [ 'eint' ], [ ('nop') x 8 ], [ 'dint' ] ],
);

sub words {
	my $words = 0;
	$words += instruction_words($_) for @_;
	return $words;
}

sub source_registers {
	my ($source) = @_;
	return qw(INTA0 INT1 INT2) if $source == SOURCE_ALL;
	return qw(INT2) if $source == SOURCE_INT2;
	return qw(INTA0);
}

sub interrupt_masks {
	my ($source) = @_;
	return ('set 0x$000C st0', 'set 0x$0040 st2') if $source == SOURCE_ALL;
	return ('set 0x$0004 st0', 'set 0x$0040 st2') if $source == SOURCE_INT2;
	return ('set 0x$0004 st0');
}

my $completion_address = 0x0880;
my $asm = "// Interrupt acceptance inside straight-line windows opened and closed by IE/IMx writes.\n";
$asm .= "segment p 0006\nbr 0x0000\$0900 always\n\nsegment p 000E\nbr 0x0000\$0980 always\n\n";
$asm .= "segment p 0016\nbr 0x0000\$0940 always\n";
$asm .= "\nsegment p 0100\n" . join("\n",
	'mov 0x$0001 a0l', 'mov a0l [0x$TEAK_MCS_CFR]', 'dint', 'mov 0x0000 icr', 'rst 0x$0004 st0', 'rst 0x$0040 st2',
	'clr a0 always', (map { "mov a0l [0x\$TEAK_INT_E$_]" } qw(INTA0 INTB0 INT1 INT2)),
	'mov 0x$FFFF a0l', (map { "mov a0l [0x\$TEAK_INT_R$_]" } qw(INTA0 INTB0 INT1 INT2)),
	'br 0x0000$0200 always') . "\n";

for my $n (0 .. $#scenarios) {
	my ($name, $source, $prepare, $enable, $body, $disable) = @{$scenarios[$n]};
	my @sources = source_registers($source);
	my $setup_address = 0x0200 + 0x30 * $n;
	my $window_address = 0x0600 + 0x20 * $n;
	my $next = $n == $#scenarios ? $completion_address : 0x0200 + 0x30 * ($n + 1);

	my @setup = ('dint', 'clr a0 always', 'mov a0l ' . shared_ram($count), 'mov a0l ' . shared_ram($return_address),
		(map { 'mov a0l ' . shared_ram($_) } @entry_order), 'mov 0x$0001 a0l',
		(map { "mov a0l [0x\$TEAK_INT_R$_]" } qw(INTA0 INT1 INT2)), interrupt_masks($source),
		(map { "mov a0l [0x\$TEAK_INT_E$_]" } @sources));
	push @setup, (map { "mov a0l [0x\$TEAK_INT_S$_]" } @sources), 'rep 0x00ffu8', 'nop'
		if $source != SOURCE_RAISED_IN_WINDOW;
	push @setup, @$prepare, sprintf('br 0x0000$%04X always', $window_address);
	die "setup $n is too long\n" if words(@setup) > 0x30;

	my @window = (@$enable, @$body, @$disable, 'dint',
		'mov ' . shared_ram($count) . ' a0', 'mov a0l ' . shared_ram($result + 2 * $n),
		'mov ' . shared_ram($return_address) . ' a0', 'mov a0l ' . shared_ram($result + 2 * $n + 1),
		'mov 0x$0001 a0l', (map { "mov a0l [0x\$TEAK_INT_R$_]" } qw(INTA0 INT1 INT2)), 'clr a0 always',
		(map { "mov a0l [0x\$TEAK_INT_E$_]" } @sources), sprintf('br 0x0000$%04X always', $next));
	my $window_limit = $n == $#scenarios ? $completion_address : $window_address + 0x20;
	die "window $n is too long\n" if $window_address + words(@window) > $window_limit;

	$asm .= sprintf("\n// %u: %s.\nsegment p %04X\n", $n, $name, $setup_address) . join("\n", @setup) . "\n";
	$asm .= sprintf("\nsegment p %04X\n", $window_address) . join("\n", @window) . "\n";
}

$asm .= sprintf("\nsegment p %04X\n", $completion_address) . join("\n",
	'mov 0x$A55A a0l', 'mov a0l ' . shared_ram($marker), 'br 0x0000$0884 always') . "\n";
$asm .= "\nsegment p 0884\nbr 0x0000\$0884 always\n";
for my $handler ([ 0x0900, 'INTA0', $entry_order[0] ], [ 0x0980, 'INT1', $entry_order[1] ],
	[ 0x0940, 'INT2', $entry_order[2] ]) {
	my ($address, $source, $order) = @$handler;
	$asm .= sprintf("\nsegment p %04X\n", $address) . join("\n",
		'pop r0', 'push r0', 'mov r0 a0l', 'mov a0l ' . shared_ram($return_address),
		'mov ' . shared_ram($count) . ' a0', 'inc a0 always', 'mov a0l ' . shared_ram($count),
		'mov a0l ' . shared_ram($order), 'mov 0x$0001 a0l', "mov a0l [0x\$TEAK_INT_R$source]", 'reti true') . "\n";
}

write_text($ARGV[0] // "$RealBin/interrupt-window.asm", $asm);
