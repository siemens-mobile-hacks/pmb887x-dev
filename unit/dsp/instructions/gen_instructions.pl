#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename qw(dirname);
use File::Path qw(make_path);
use File::Spec;

my $script_dir = dirname(__FILE__);
my $cases_path = File::Spec->catfile($script_dir, 'instructions-cases.inc');
my $generated_dir = File::Spec->catdir($script_dir, 'generated');
my $images_path = File::Spec->catfile($generated_dir, 'instructions-images.inc');
my $packer_path = File::Spec->catfile($script_dir, 'pack_instructions.pl');
my $record_words = 35;

my @cases;

sub add_case {
	my ($name, $setup, $instruction) = @_;
	my $body = ref($instruction) eq 'ARRAY' ? $instruction : [ $instruction ];
	push @cases, {
		name => $name,
		setup => [ @$setup ],
		body => [ @$body ],
	};
}

sub base_setup {
	return (
		'mov 0x$5555 a0l',
		'mov a0l x0',
		'mov 0x$AAAA y0',
		'clr a0 always',
		'mov 0x$1357 r0',
		'addl r0 a0',
		'mov 0x$2468 r0',
		'addh r0 a0',
		'clr a1 always',
		'mov 0x$89AB r0',
		'addl r0 a1',
		'mov 0x$4567 r0',
		'addh r0 a1',
		'mov 0x$0000 st1',
		'mov 0x$0000 st2',
		'mov 0x$0011 r0',
		'mov 0x$1122 r1',
		'mov 0x$2233 r2',
		'mov 0x$3344 r3',
		'mov 0x$4455 r4',
		'mov 0x$6677 r5',
		'mov 0x$7788 r7',
		'mov 0x$D700 sp',
		'mov 0x$0000 sv',
		'mov 0x$0000 cfgi',
		'mov 0x$0000 cfgj',
		'mov 0x$0000 lc',
		'mov r0 mixp',
		'mov 0x$0000 r0',
		'data 8040 // mpy y0,r0; deterministic p baseline from PDF Table 4-4',
		'nop',
		'mov 0x$0011 r0',
	);
}

sub canary_setup {
	return (
		'mov 0x$C33C a0l',
		'mov a0l [0x$D680]',
		'mov 0x$5AA5 a0l',
		'mov a0l [0x$D681]',
		'mov 0x$0000 a0l',
		'mov a0l [0x$D682]',
	);
}

sub accumulator_setup {
	my ($low, $high, $st0) = @_;
	return (
		base_setup(),
		'clr a0 always',
		sprintf('mov 0x$%04X r0', $low),
		'addl r0 a0',
		sprintf('mov 0x$%04X r0', $high),
		'addh r0 a0',
		'mov 0x$0011 r0',
		sprintf('mov 0x$%04X st0', $st0),
	);
}

sub accumulator1_setup {
	my ($low, $high, $st1, $st0) = @_;
	return (
		base_setup(),
		'clr a1 always',
		sprintf('mov 0x$%04X r0', $low),
		'addl r0 a1',
		sprintf('mov 0x$%04X r0', $high),
		'addh r0 a1',
		'mov 0x$0011 r0',
		sprintf('mov 0x$%04X st0', $st0),
		sprintf('mov 0x$%04X st1', $st1),
	);
}

sub accumulator_value_setup {
	my ($accumulator, $low, $high, $extension) = @_;
	my $status = $accumulator eq 'a0' ? 'st0' : 'st1';
	return (
		"clr $accumulator always",
		sprintf('mov 0x$%04X r0', $low),
		"addl r0 $accumulator",
		sprintf('mov 0x$%04X r0', $high),
		"addh r0 $accumulator",
		sprintf('mov 0x$%04X %s', $extension << 12, $status),
	);
}

sub four_accumulator_setup {
	return (
		base_setup(),
		accumulator_value_setup('a0', 0xA0B1, 0xA0B0, 0xA),
		'mov a0 b0',
		accumulator_value_setup('a0', 0xB0C1, 0xB0C0, 0xB),
		'mov a0 b1',
		accumulator_value_setup('a0', 0x1011, 0x1010, 0x1),
		accumulator_value_setup('a1', 0x2021, 0x2020, 0x2),
		'mov 0x$0011 r0',
	);
}

add_case('nop flags clear', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], 'nop');
add_case('nop writable flags set', [ accumulator_setup(0x1357, 0x2468, 0x0FF0) ], 'nop');

my @alu_vectors = (
	[ 'zero', 0x0000, 0x0000, 0x0000, 0x0000 ],
	[ 'low carry', 0xFFFF, 0x0000, 0x0000, 0x0001 ],
	[ 'positive max', 0xFFFF, 0xFFFF, 0x7000, 0x0001 ],
	[ 'negative one', 0xFFFF, 0xFFFF, 0xF000, 0x0001 ],
	[ 'negative min', 0x0000, 0x0000, 0x8000, 0x0001 ],
	[ 'alternating', 0xAAAA, 0x5555, 0x0000, 0xA55A ],
);

for my $operation (qw(add sub cmp and or xor)) {
	for my $vector (@alu_vectors) {
		my ($vector_name, $low, $high, $st0, $operand) = @$vector;
		add_case(
			"$operation immediate $vector_name",
			[ accumulator_setup($low, $high, $st0) ],
			sprintf('%s 0x$%04X a0', $operation, $operand),
		);
	}
}

my @conditional_unary = (
	[ 'clr', 0x1357, 0x2468, 0x0000 ],
	[ 'clrr', 0x1357, 0x2468, 0x0000 ],
	[ 'inc', 0xFFFF, 0xFFFF, 0x7000 ],
	[ 'dec', 0x0000, 0x0000, 0x8000 ],
	[ 'neg', 0x0001, 0x0000, 0x0000 ],
	[ 'not', 0xAAAA, 0x5555, 0x0000 ],
	[ 'shl', 0x8001, 0x0000, 0x0000 ],
	[ 'shl4', 0xF001, 0x0000, 0x0000 ],
	[ 'shr', 0x0001, 0x0000, 0x8000 ],
	[ 'shr4', 0x000F, 0x0000, 0x8000 ],
	[ 'rol', 0x8000, 0x0000, 0x0080 ],
	[ 'ror', 0x0001, 0x0000, 0x0080 ],
	[ 'rnd', 0x8000, 0x0000, 0x0000 ],
);

for my $case (@conditional_unary) {
	my ($operation, $low, $high, $st0) = @$case;
	add_case("$operation a0 always", [ accumulator_setup($low, $high, $st0) ], "$operation a0 always");
}

for my $operation (qw(inc dec neg not shl shr)) {
	add_case(
		"$operation a0 false eq",
		[ accumulator_setup(0x1357, 0x2468, 0x0000) ],
		"$operation a0 eq",
	);
	add_case(
		"$operation a0 true eq",
		[ accumulator_setup(0x1357, 0x2468, 0x0800) ],
		"$operation a0 eq",
	);
}

my @register_operations = (
	[ 'add register low carry', 'add r0 a0', 0xFFFF, 0x0000, 0x0000, 0x0001 ],
	[ 'sub register borrow', 'sub r0 a0', 0x0000, 0x0000, 0x0000, 0x0001 ],
	[ 'and register mask', 'and r0 a0', 0xA55A, 0x5AA5, 0x0000, 0x0FF0 ],
	[ 'or register mask', 'or r0 a0', 0xA500, 0x5AA5, 0x0000, 0x00F0 ],
	[ 'xor register mask', 'xor r0 a0', 0xAAAA, 0x5555, 0x0000, 0xFFFF ],
	[ 'addh register', 'addh r0 a0', 0x1357, 0xFFFF, 0x0000, 0x0001 ],
	[ 'addl register', 'addl r0 a0', 0xFFFF, 0x2468, 0x0000, 0x0001 ],
	[ 'subh register', 'subh r0 a0', 0x1357, 0x0000, 0x0000, 0x0001 ],
	[ 'subl register', 'subl r0 a0', 0x0000, 0x2468, 0x0000, 0x0001 ],
);

for my $case (@register_operations) {
	my ($name, $instruction, $low, $high, $st0, $r0) = @$case;
	my @setup = accumulator_setup($low, $high, $st0);
	splice @setup, -1, 0, sprintf('mov 0x$%04X r0', $r0);
	add_case($name, \@setup, $instruction);
}

for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
	my @setup = base_setup();
	push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
	add_case(sprintf('mpy y0 r0 %04X', $value), \@setup, 'data 8040 // mpy y0,r0; PDF Table 4-4');
	add_case(sprintf('sqr r0 %04X', $value), \@setup, 'data 9AA0 // sqr r0; PDF Table 4-4');
}

for my $condition (qw(neq gt ge lt le mn c v e l nr)) {
	add_case("inc a0 $condition flags clear", [ accumulator_setup(0x1357, 0x2468, 0x0000) ], "inc a0 $condition");
	add_case("inc a0 $condition flags set", [ accumulator_setup(0x1357, 0x2468, 0x0FF0) ], "inc a0 $condition");
}

for my $operation (qw(add sub cmp and or xor)) {
	for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF) {
		add_case(
			sprintf('%s short immediate %02X', $operation, $value),
			[ accumulator_setup(0xA55A, 0x1357, 0x0000) ],
			sprintf('%s 0x%04xu8 a0', $operation, $value),
		);
	}
}

my @long_register_operations = (
	[ 'set', 0x0000, 0x8001 ],
	[ 'set', 0x5555, 0xAAAA ],
	[ 'set', 0xFFFF, 0x0000 ],
	[ 'rst', 0xFFFF, 0x8001 ],
	[ 'rst', 0xAAAA, 0x5555 ],
	[ 'rst', 0x0000, 0xFFFF ],
	[ 'chng', 0x0000, 0x8001 ],
	[ 'chng', 0x5555, 0xAAAA ],
	[ 'chng', 0xFFFF, 0xFFFF ],
	[ 'addv', 0x0000, 0x0000 ],
	[ 'addv', 0x7FFF, 0x0001 ],
	[ 'addv', 0xFFFF, 0x0001 ],
	[ 'cmpv', 0x0000, 0x0000 ],
	[ 'cmpv', 0x8000, 0x0001 ],
	[ 'cmpv', 0xFFFF, 0xFFFF ],
	[ 'subv', 0x0000, 0x0001 ],
	[ 'subv', 0x8000, 0x0001 ],
	[ 'subv', 0xFFFF, 0xFFFF ],
);

for my $case (@long_register_operations) {
	my ($operation, $initial, $immediate) = @$case;
	my @setup = base_setup();
	push @setup, sprintf('mov 0x$%04X r0', $initial), 'mov 0x$0000 st0';
	add_case(
		sprintf('%s long immediate r0 %04X %04X', $operation, $initial, $immediate),
		\@setup,
		sprintf('%s 0x$%04X r0', $operation, $immediate),
	);
}

for my $bit (0, 1, 7, 8, 14, 15) {
	for my $value (0x0000, 0xFFFF) {
		my @setup = base_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
		add_case(sprintf('tstb r0 bit %u value %04X', $bit, $value), \@setup, sprintf('tstb r0 0x%04x', $bit));
	}
}

for my $shift (-31, -15, -1, 1, 15, 31) {
	my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
	add_case(
		"shfi a1 a0 $shift",
		[ accumulator_setup(0x1357, 0x2468, 0x0000) ],
		"shfi a1 a0 $immediate",
	);

	my @setup = accumulator_setup(0x1357, 0x2468, 0x0000);
	push @setup, sprintf('mov 0x$%04X sv', $shift & 0xFFFF);
	add_case("shfc a1 a0 $shift", \@setup, 'shfc a1 a0 always');
}

for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
	my @setup = base_setup();
	push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
	add_case(sprintf('exp r0 a0 %04X', $value), \@setup, 'exp r0 a0');
}

my @indirect_operations = (
	[ 'add', 0x0001 ],
	[ 'sub', 0x0001 ],
	[ 'cmp', 0x2468 ],
	[ 'and', 0x0FF0 ],
	[ 'or', 0x00F0 ],
	[ 'xor', 0xFFFF ],
	[ 'addh', 0x0001 ],
	[ 'addl', 0x0001 ],
	[ 'subh', 0x0001 ],
	[ 'subl', 0x0001 ],
);

for my $operation (@indirect_operations) {
	my ($mnemonic, $value) = @$operation;
	for my $step ('', '++', '--', '++s') {
		my @setup = accumulator_setup(0xFFFF, 0x2468, 0x0000);
		push @setup,
			'mov 0x$D600 r0',
			sprintf('mov 0x$%04X r1', $value),
			'mov r1 [r0]',
			'mov 0x$D600 r0',
			'load +0x0003 stepi';
		add_case(
			sprintf('%s indirect r0%s', $mnemonic, $step eq '' ? ' no step' : $step),
			\@setup,
			"$mnemonic [r0$step] a0",
		);
	}
}

my @multiply_operations = (
	[ 'mpysu', 0x8140 ],
	[ 'mac', 0x8240 ],
	[ 'macus', 0x8340 ],
	[ 'maa', 0x8440 ],
	[ 'macuu', 0x8540 ],
	[ 'macsu', 0x8640 ],
	[ 'maasu', 0x8740 ],
);

for my $operation (@multiply_operations) {
	my ($mnemonic, $opcode) = @$operation;
	for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
		my @setup = base_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
		add_case(
			sprintf('%s y0 r0 %04X', $mnemonic, $value),
			\@setup,
			sprintf('data %04X // %s y0,r0; PDF Table 4-4', $opcode, $mnemonic),
		);
	}
}

for my $value (0x0000, 0x0001, 0x007F, 0x0080, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
	my @setup = base_setup();
	push @setup, 'mov 0x$0000 st0';
	add_case(sprintf('mov long immediate r0 %04X', $value), \@setup, sprintf('mov 0x$%04X r0', $value));
}

my @signed_short_values = (
	[ 'positive zero', '+0x0000' ],
	[ 'positive one', '+0x0001' ],
	[ 'positive max', '+0x007f' ],
	[ 'negative min', '-0x0080' ],
	[ 'negative one', '-0x0001' ],
);

for my $value (@signed_short_values) {
	my ($name, $immediate) = @$value;
	add_case("mov short immediate r0 $name", [ base_setup() ], "mov $immediate r0");
	add_case("mov short immediate a0h $name", [ base_setup() ], "mov $immediate a0h");
}

for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF) {
	add_case(
		sprintf('mov unsigned short immediate a0l %02X', $value),
		[ base_setup() ],
		sprintf('mov 0x%04xu8 a0l', $value),
	);
}

my @register_moves = (
	[ 'r1 to r0', [ 'mov 0x$A55A r1' ], 'mov r1 r0' ],
	[ 'st0 to r0', [ 'mov 0x$05A0 st0' ], 'mov st0 r0' ],
	[ 'st1 to r0', [ 'mov 0x$1357 st1' ], 'mov st1 r0' ],
	[ 'st2 to r0', [ 'mov 0x$2468 st2' ], 'mov st2 r0' ],
	[ 'sv to r0', [ 'mov 0x$A55A sv' ], 'mov sv r0' ],
	[ 'a0l to r0', [], 'mov a0l r0' ],
	[ 'a0h to r0', [], 'mov a0h r0' ],
	[ 'r0 to a0l', [ 'mov 0x$A55A r0' ], 'mov r0 a0l' ],
	[ 'r0 to a0h', [ 'mov 0x$A55A r0' ], 'mov r0 a0h' ],
	[ 'a1 to a0', [], 'mov a1 a0' ],
);

for my $move (@register_moves) {
	my ($name, $extra_setup, $instruction) = @$move;
	my @setup = base_setup();
	push @setup, @$extra_setup;
	add_case("mov register $name", \@setup, $instruction);
}

for my $operation (qw(movs movr)) {
	for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
		my @setup = base_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
		add_case(sprintf('%s r0 a0 %04X', $operation, $value), \@setup, "$operation r0 a0");
	}
}

for my $offset (0x00, 0x01, 0x7F, 0x80, 0xFF) {
	my @setup = base_setup();
	push @setup,
		'load 0x00d6u8 page',
		'mov 0x$A55A r1',
		sprintf('mov r1 [page:0x%04xu8]', $offset);
	add_case(
		sprintf('mov page direct to a0 offset %02X', $offset),
		\@setup,
		sprintf('mov [page:0x%04xu8] a0', $offset),
	);
	add_case(
		sprintf('mov page direct to r0 offset %02X', $offset),
		\@setup,
		sprintf('mov [page:0x%04xu8] r0', $offset),
	);
}

for my $destination (qw(a0l a0h)) {
	my @setup = base_setup();
	push @setup, 'load 0x00d6u8 page', 'mov 0x$A55A r1', 'mov r1 [page:0x0000u8]';
	add_case("mov page direct to $destination", \@setup, "mov [page:0x0000u8] $destination");
}

{
	my @setup = base_setup();
	push @setup, 'mov 0x$D600 r0', 'mov 0x$A55A r1', 'mov r1 [r0]';
	add_case('mov long absolute to a0', \@setup, 'mov [0x$D600] a0');
}

my @r7_offsets = (
	[ 'short positive zero', '[r7+0x0000s7]', 0x0000 ],
	[ 'short positive max', '[r7+0x003fs7]', 0x003F ],
	[ 'short negative one', '[r7-0x0001s7]', -1 ],
	[ 'short negative min', '[r7-0x0040s7]', -0x40 ],
	[ 'long positive', undef, 0x0100 ],
	[ 'long negative', undef, -0x0100 ],
);

for my $offset (@r7_offsets) {
	my ($name, $operand, $delta) = @$offset;
	my $address = 0xD700 + $delta;
	my @setup = base_setup();
	push @setup,
		sprintf('mov 0x$%04X r0', $address & 0xFFFF),
		'mov 0x$A55A r1',
		'mov r1 [r0]',
		'mov 0x$D700 r7';
	my $body = defined($operand) ? "mov $operand a0" : [
		'data D498 // mov (rb+long offset),a0; PDF Table 4-4',
		sprintf('data %04X // rb long offset expansion', $delta & 0xFFFF),
	];
	add_case("mov r7 relative $name", \@setup, $body);
}

for my $register (0 .. 5) {
	for my $step ('', '++', '--', '++s') {
		my @setup = base_setup();
		push @setup,
			'mov 0x$D600 r0',
			'mov 0x$A55A r1',
			'mov r1 [r0]',
			sprintf('mov 0x$D600 r%u', $register),
			'load +0x0003 stepi';
		my @body = (sprintf('mov [r%u%s] a0', $register, $step));
		push @body, sprintf('mov r%u r0', $register) if $register != 0;
		add_case(sprintf('mov indirect read r%u%s', $register, $step eq '' ? ' no step' : $step), \@setup, \@body);
	}
}

# X/Y writes are buffered. Seed and read back through the same address register,
# with idle instructions between the tested store and its observation.
for my $register (0 .. 5) {
	for my $step ('', '++', '--', '++s') {
		my @setup = base_setup();
		push @setup,
			'mov 0x$A55A a1l',
			sprintf('mov 0x$D600 r%u', $register),
			sprintf('mov a1l [r%u]', $register),
			'nop',
			'nop',
			sprintf('mov 0x$D600 r%u', $register),
			'load +0x0003 stepi';
		my @body = (
			sprintf('mov a0l [r%u%s]', $register, $step),
			'nop',
			'nop',
			sprintf('mov r%u r7', $register),
			'mov st0 a1l',
			sprintf('mov 0x$D600 r%u', $register),
			sprintf('mov [r%u] a0', $register),
			'mov a1l st0',
			'mov r7 r0',
		);
		add_case(sprintf('mov indirect write r%u%s', $register, $step eq '' ? ' no step' : $step), \@setup, \@body);
	}
}

for my $operation (qw(add sub cmp and or xor)) {
	for my $long (0, 1) {
		my @setup = accumulator_setup(0xFFFF, 0x2468, 0x0000);
		push @setup, 'mov 0x$D600 r0', 'mov 0x$0001 r1', 'mov r1 [r0]', 'load 0x00d6u8 page';
		my $operand = $long ? '[0x$D600]' : '[page:0x0000u8]';
		add_case("$operation direct " . ($long ? 'long' : 'page'), \@setup, "$operation $operand a0");
	}
}

for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	push @setup, sprintf('mov 0x$%04X r0', $value);
	add_case(sprintf('cmpu r0 a0 %04X', $value), \@setup, 'cmpu r0 a0');
}

for my $operation (qw(tst0 tst1)) {
	for my $vector (
		[ 0x0000, 0x0001 ],
		[ 0x0001, 0x0001 ],
		[ 0x8000, 0x8000 ],
		[ 0x7FFF, 0x8000 ],
		[ 0x5555, 0xAAAA ],
		[ 0xAAAA, 0xFFFF ],
	) {
		my ($value, $mask) = @$vector;
		my @setup = base_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
		add_case(sprintf('%s long mask r0 %04X %04X', $operation, $value, $mask), \@setup,
			sprintf('%s 0x$%04X r0', $operation, $mask));
	}
}

my @norm_vectors = (
	[ 'positive unnormalized', 0x0001, 0x0000, 0x0000 ],
	[ 'negative unnormalized', 0xFFFF, 0xFFFF, 0x0000 ],
	[ 'positive normalized', 0x0000, 0x4000, 0x0200 ],
	[ 'negative normalized', 0x0000, 0xC000, 0x0200 ],
);

for my $vector (@norm_vectors) {
	my ($name, $low, $high, $st0) = @$vector;
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			my @setup = accumulator_setup($low, $high, $st0);
			push @setup, sprintf('mov 0x$D600 r%u', $register), 'load +0x0003 stepi';
			add_case("norm a0 r$register $name" . ($step eq '' ? ' no step' : $step), \@setup,
				"norm a0 [r$register$step]");
		}
	}
}

my @limit_vectors = (
	[ 'in range', 0x1357, 0x2468, 0x0000 ],
	[ 'positive overflow', 0xFFFF, 0x7FFF, 0x1000 ],
	[ 'negative overflow', 0x0000, 0x8000, 0xE000 ],
	[ 'positive edge', 0xFFFF, 0x7FFF, 0x0000 ],
	[ 'negative edge', 0x0000, 0x8000, 0xF000 ],
);

for my $vector (@limit_vectors) {
	my ($name, $low, $high, $st0) = @$vector;
	add_case("lim a0 $name", [ accumulator_setup($low, $high, $st0) ], 'data 49C0 // lim a0; PDF Table 4-4');
}

for my $ps (0 .. 3) {
	for my $value (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
		my @setup = base_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value), sprintf('load 0x%04x ps', $ps), 'mov 0x$0000 st0';
		add_case(
			sprintf('pacr a0 ps%u value %04X', $ps, $value),
			\@setup,
			[ 'data 8040 // mpy y0,r0; PDF Table 4-4', 'nop', 'pacr a0 always' ],
		);
	}
}

for my $operation (qw(msu sqra)) {
	for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
		my @setup = base_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value), 'mov 0x$0000 st0';
		add_case(sprintf('%s r0 a0 %04X', $operation, $value), \@setup, "$operation r0 a0");
	}
}

for my $value (@signed_short_values) {
	my ($name, $immediate) = @$value;
	add_case("mpyi y0 $name", [ base_setup() ], "mpyi y0 $immediate");
}

for my $start (0x0000, 0x0001, 0xFFFF) {
	for my $step ('++', '--', '++s') {
		for my $dmod (0, 1) {
			my @setup = base_setup();
			push @setup, sprintf('mov 0x$%04X r0', $start), 'load +0x0003 stepi', 'mov 0x$0000 st0';
			my $suffix = $dmod ? ' dmod' : '';
			add_case(sprintf('modr r0%s%s start %04X', $step, $suffix, $start), \@setup, "modr [r0$step]$suffix");
		}
	}
}

for my $count (0, 1, 2, 3, 7) {
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	add_case(sprintf('rep immediate inc count %u', $count), \@setup,
		[ sprintf('rep 0x%04xu8', $count), 'inc a0 always' ]);
}

for my $count (0, 1, 3) {
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	push @setup, sprintf('mov 0x$%04X r0', $count);
	add_case(sprintf('rep register inc count %u', $count), \@setup, [ 'rep r0', 'inc a0 always' ]);
}

add_case('rep immediate inc count 255', [ accumulator_setup(0x0000, 0x0000, 0x0000) ],
	[ 'rep 0x00ffu8', 'inc a0 always' ]);
{
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	push @setup, 'mov 0x$FFFF r0';
	add_case('rep register inc count 65535', \@setup, [ 'rep r0', 'inc a0 always' ]);
}

my @repeat_registers = qw(r0 r1 r2 r3 r4 r5 r7 y0 st0 st1 st2 cfgi cfgj a0l a1l a0h a1h lc sv);
for my $register (@repeat_registers) {
	my $accumulator = $register =~ /^a0/ ? 'a1' : 'a0';
	my @setup = four_accumulator_setup();
	push @setup, sprintf('mov 0x$0003 %s', $register), "clr $accumulator always";
	add_case("rep register selector $register", \@setup, [ "rep $register", "inc $accumulator always" ]);
}

for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0xA55A) {
	my @setup = base_setup();
	push @setup, 'mov 0x$D700 sp', sprintf('mov 0x$%04X r1', $value);
	add_case(sprintf('push r1 pop a0l %04X', $value), \@setup,
		[ 'push r1', 'pop a0l', 'mov sp r0' ]);
	add_case(sprintf('push immediate pop a0l %04X', $value), \@setup,
		[ sprintf('push 0x$%04X', $value), 'pop a0l', 'mov sp r0' ]);
}

my @movsi_operands = (
	[ 'r0', 0xA55A ],
	[ 'r5', 0x5AA5 ],
	[ 'r7', 0x8001 ],
	[ 'y0', 0x7FFF ],
);

for my $operand (@movsi_operands) {
	my ($register, $value) = @$operand;
	for my $destination (qw(a0 a1)) {
		for my $shift (-16, -1, 0, 1, 15) {
			my @setup = base_setup();
			push @setup, sprintf('mov 0x$%04X %s', $value, $register), 'mov 0x$0000 st0';
			my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
			add_case("movsi $register $destination shift $shift", \@setup, "movsi $register $destination $immediate");
		}
	}
}

for my $extension (0x0, 0x5, 0xF) {
	for my $value (0x7FFF, 0x8000) {
		for my $keep_extension (0, 1) {
			my @setup = accumulator_setup(0x1357, 0x2468, $extension << 12);
			push @setup, 'load 0x00d6u8 page', sprintf('mov 0x$%04X r1', $value), 'mov r1 [page:0x0000u8]';
			my $suffix = $keep_extension ? ' eu' : '';
			add_case(sprintf('mov direct a0h%s ext%X value %04X', $suffix, $extension, $value), \@setup,
				"mov [page:0x0000u8] a0h$suffix");
		}
	}
}

my @division_vectors = (
	[ 0x0000, 0x0001 ],
	[ 0x0001, 0x0001 ],
	[ 0x1234, 0x0011 ],
	[ 0x7FFF, 0x0003 ],
	[ 0x8000, 0x0101 ],
	[ 0xFFFF, 0x7FFF ],
);

for my $vector (@division_vectors) {
	my ($dividend, $divisor) = @$vector;
	for my $steps (1, 16) {
		my @setup = accumulator_setup($dividend, 0x0000, 0x0000);
		push @setup, 'load 0x00d6u8 page', sprintf('mov 0x$%04X r1', $divisor), 'mov r1 [page:0x0000u8]';
		my @body = ('divs [page:0x0000u8] a0') x $steps;
		add_case(sprintf('divs a0 dividend %04X divisor %04X steps %u', $dividend, $divisor, $steps), \@setup, \@body);
	}
}

my @max_min_operations = (
	[ 'max', 0x8460, qw(ge gt) ],
	[ 'min', 0x8860, qw(le lt) ],
);

for my $operation (@max_min_operations) {
	my ($name, $base_opcode, @conditions) = @$operation;
	for my $destination (0, 1) {
		for my $condition (0 .. 1) {
			for my $step (0 .. 3) {
				my @setup = base_setup();
				push @setup, 'mov 0x$D600 r0', 'load +0x0003 stepi';
				my $opcode = $base_opcode | $destination << 8 | $condition << 9 | $step << 3;
				add_case(sprintf('%s a%u %s r0 step%u', $name, $destination, $conditions[$condition], $step), \@setup,
					sprintf('data %04X // %s a%u,(r0); PDF Table 4-4', $opcode, $name, $destination));
			}
		}
	}
}

my @maxd_vectors = (
	[ 'maxd', 0x8060, 0x0000, 0x0001, qw(ge gt) ],
	[ 'maxd equal', 0x8060, 0x0000, 0x0000, qw(ge gt) ],
);

for my $vector (@maxd_vectors) {
	my ($name, $base_opcode, $accumulator, $memory, @conditions) = @$vector;
	for my $condition (0 .. 1) {
		for my $step (0 .. 3) {
			my @setup = accumulator_setup($accumulator, 0x0000, 0x0000);
			push @setup,
				'mov 0x$D600 r0',
				sprintf('mov 0x$%04X r1', $memory),
				'mov r1 [r0]',
				'mov 0x$D600 r0',
				'load +0x0003 stepi';
			my $opcode = $base_opcode | $condition << 9 | $step << 3;
			add_case(sprintf('%s a0 %s r0 step%u', $name, $conditions[$condition], $step), \@setup,
				sprintf('data %04X // %s a0,(r0); PDF Table 4-4', $opcode, $name));
		}
	}
}

my @branch_conditions = qw(always eq neq gt ge lt le nn c v e l nr niu0 iu0 iu1);
my $control_helper_address = 0x1000;
my $program_fixture_address = 0x1100;

for my $condition (0 .. $#branch_conditions) {
	for my $flags_set (0, 1) {
		my @setup = accumulator_setup(0x1357, 0x2468, $flags_set ? 0x0FF0 : 0x0000);
		push @setup, sprintf('mov 0x$%04X st2', $flags_set ? 0x0C00 : 0x0000);
		my @body = (
			sprintf('data %04X // brr +4,%s; PDF Table 4-4', 0x5040 | $condition, $branch_conditions[$condition]),
			'mov 0x0001u8 a0l',
			'mov a0l [0x$D682]',
			'data 5030 // brr +3,always; skip taken marker',
			'mov 0x0002u8 a0l',
			'mov a0l [0x$D682]',
		);
		add_case(sprintf('brr %s flags %s', $branch_conditions[$condition], $flags_set ? 'set' : 'clear'), \@setup, \@body);
	}
}

{
	my @setup = base_setup();
	my @body = (
		'data 1040 // callr +4,always; PDF Table 4-4',
		'mov 0x0003u8 a0l',
		'mov a0l [0x$D682]',
		'data 5040 // brr +4,always; skip subroutine',
		'mov 0x0002u8 a0l',
		'mov a0l [0x$D681]',
		'ret always',
	);
	add_case('callr always and ret', \@setup, \@body);
}

{
	my @setup = accumulator_setup(0x1357, 0x2468, 0x0000);
	my @body = (
		'data 1041 // callr +4,eq; condition false',
		'mov 0x0003u8 a0l',
		'mov a0l [0x$D682]',
		'data 5040 // brr +4,always; skip subroutine',
		'mov 0x0002u8 a0l',
		'mov a0l [0x$D681]',
		'ret always',
	);
	add_case('callr false preserves stack', \@setup, \@body);
}

{
	my @setup = accumulator_setup(0x1357, 0x2468, 0x0000);
	add_case('ret false falls through', \@setup,
		[ 'ret eq', 'mov 0x0003u8 a0l', 'mov a0l [0x$D682]' ]);
}

add_case('brr signed offset endpoints taken', [ base_setup() ], [
	'data 53F0 // brr +63,always; signed positive offset boundary; PDF Table 4-4',
	'mov 0x0002u8 a0l',
	'mov a0l [0x$D682]',
	'data 53C0 // brr +60,always; skip the backward branch after its taken path',
	('nop') x 59,
	'data 5400 // brr -64,always; signed negative offset boundary; PDF Table 4-4',
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);

add_case('callr signed negative offset boundary', [ base_setup() ], [
	'data 53F0 // brr +63,always; enter the backward-call site',
	'mov 0x0002u8 a0l',
	'mov a0l [0x$D681]',
	'ret always',
	('nop') x 59,
	'data 1400 // callr -64,always; signed negative offset boundary; PDF Table 4-4',
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);

add_case('callr signed positive offset boundary', [ base_setup() ], [
	'data 13F0 // callr +63,always; signed positive offset boundary; PDF Table 4-4',
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
	'@brabs always 64',
	('nop') x 58,
	'mov 0x0002u8 a0l',
	'mov a0l [0x$D681]',
	'ret always',
]);

my @modulo_sequences = (
	[ 'plus one boundary wrap', '++', undef, 6, 0xD616, 1 ],
	[ 'minus one boundary wrap', '--', undef, 6, 0xD610, 1 ],
	[ 'plus step two boundary wrap', '++s', 2, 6, 0xD616, 1 ],
	[ 'minus step three boundary wrap', '++s', -3, 6, 0xD610, 1 ],
	[ 'plus step three full irregular cycle', '++s', 3, 5, 0xD610, 8 ],
);

for my $register (0 .. 5) {
	my $configuration = $register < 4 ? 'i' : 'j';
	for my $sequence (@modulo_sequences) {
		my ($name, $modifier, $step, $modulo, $start, $iterations) = @$sequence;
		my @setup = base_setup();
		push @setup,
			sprintf('mov 0x$%04X r%u', $start, $register),
			sprintf('load 0x%04X mod%s', $modulo, $configuration);
		push @setup, sprintf('load %s0x%04X step%s', $step < 0 ? '-' : '+', abs($step), $configuration)
			if defined $step;
		push @setup, sprintf('mov 0x$%04X st2', 1 << $register);
		my @body = (sprintf('modr [r%u%s]', $register, $modifier)) x $iterations;
		add_case("modulo r$register $name", \@setup, \@body);
	}

	my @setup = base_setup();
	push @setup,
		sprintf('mov 0x$D616 r%u', $register),
		sprintf('load 0x0006 mod%s', $configuration),
		sprintf('mov 0x$%04X st2', 1 << $register);
	add_case("modr r$register dmod overrides enabled modulo", \@setup, "modr [r$register++] dmod");
}

for my $register (0, 4) {
	my $configuration = $register == 0 ? 'i' : 'j';
	my @setup;
	for my $offset (0 .. 7) {
		push @setup,
			sprintf('mov 0x$%04X a0l', 0xA000 + $offset),
			sprintf('mov a0l [0x$%04X]', 0xD610 + $offset);
	}
	push @setup,
		base_setup(),
		sprintf('mov 0x$D610 r%u', $register),
		sprintf('load 0x0007 mod%s', $configuration),
		sprintf('mov 0x$%04X st2', 1 << $register);
	my @body = (sprintf('mov [r%u++] a0', $register)) x 8;
	add_case("modulo indirect r$register eight-entry ring", \@setup, \@body);
}

my @saturation_vectors = (
	[ 'positive overflow', 0xA55A, 0x8000, 0x0000 ],
	[ 'negative overflow', 0x5AA5, 0x7FFF, 0xF000 ],
);

for my $vector (@saturation_vectors) {
	my ($name, $low, $high, $extension) = @$vector;
	for my $disabled (0, 1) {
		my $mode = $disabled ? 'disabled' : 'enabled';
		for my $part (qw(a0l a0h)) {
			my @setup = accumulator_setup($low, $high, $extension | $disabled);
			add_case("saturation $mode mov $part $name", \@setup, "mov $part r0");
			add_case("saturation $mode push $part $name", \@setup, [ "push $part", 'pop r0' ]);
		}
	}

	for my $disabled (0, 1) {
		my $mode = $disabled ? 'sat-disabled' : 'sat-enabled';
		my @a0_setup = accumulator_setup($low, $high, $extension | $disabled);
		add_case("lim a0 same $name $mode", \@a0_setup, 'data 49C0 // lim a0; PDF Table 4-4');
		add_case("lim a0 to a1 $name $mode", \@a0_setup, 'data 49D0 // lim a0,a1; PDF Table 4-4');

		my @a1_setup = accumulator1_setup($low, $high, $extension, $disabled);
		add_case("lim a1 to a0 $name $mode", \@a1_setup, 'data 49E0 // lim a1,a0; PDF Table 4-4');
		add_case("lim a1 same $name $mode", \@a1_setup, 'data 49F0 // lim a1; PDF Table 4-4');
	}
}

my @shift_sources = (
	[ 'positive', 0x8001, 0x7FFF, 0x0000 ],
	[ 'negative', 0x1357, 0x8000, 0xF000 ],
);

for my $source (@shift_sources) {
	my ($name, $low, $high, $extension) = @$source;
	for my $logic (0, 1) {
		for my $shift (-15, -1, 1, 15) {
			my @setup = accumulator1_setup($low, $high, $extension, 0x0000);
			push @setup, sprintf('mov 0x$%04X st2', $logic << 7);
			my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
			add_case(sprintf('shfi %s shift %d mode %s', $name, $shift, $logic ? 'logic' : 'arithmetic'),
				\@setup, "shfi a1 a0 $immediate");
		}
	}
}

for my $value (0x7FFF, 0x8001) {
	for my $logic (0, 1) {
		for my $shift (-16, -1, 1, 15) {
			my @setup = base_setup();
			push @setup, sprintf('mov 0x$%04X r0', $value), sprintf('mov 0x$%04X st2', $logic << 7);
			my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
			add_case(sprintf('movsi r0 a0 %04X shift %d mode %s', $value, $shift, $logic ? 'logic' : 'arithmetic'),
				\@setup, "movsi r0 a0 $immediate");
		}
	}
}

for my $ps (0 .. 3) {
	for my $operation ([ 'mac', 0x8241 ], [ 'maa', 0x8441 ]) {
		my ($name, $opcode) = @$operation;
		my @setup = base_setup();
		push @setup,
			'mov 0x$0003 r0',
			'data 8040 // mpy y0,r0; seed previous product',
			'nop',
			'clr a0 always',
			sprintf('load 0x%04X ps', $ps),
			'mov 0x$0005 r1';
		add_case("$name previous product ps$ps", \@setup,
			sprintf('data %04X // %s y0,r1; PDF Table 4-4', $opcode, $name));
	}
}

for my $factor (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
	for my $gap (0 .. 2) {
		my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
		push @setup, sprintf('mov 0x$%04X r0', $factor), 'mov 0x$0003 r1';
		my @body = ('data 8040 // mpy y0,r0; pipeline producer', ('nop') x $gap,
			'data 8241 // mac y0,r1; consume previous product');
		add_case(sprintf('mpy mac pipeline factor %04X gap %u', $factor, $gap), \@setup, \@body);

		@body = ('data 8040 // mpy y0,r0; pipeline producer', ('nop') x $gap,
			'data 5B0B // mov p,a0; PDF Table 4-4');
		add_case(sprintf('mpy mov p pipeline factor %04X gap %u', $factor, $gap), \@setup, \@body);
	}
}

for my $condition (0 .. $#branch_conditions) {
	for my $flags_set (0, 1) {
		my @setup = accumulator_setup(0x1357, 0x2468, $flags_set ? 0x0FF0 : 0x0000);
		push @setup, sprintf('mov 0x$%04X st2', $flags_set ? 0x0C00 : 0x0000);
		my @body = (
			sprintf('@brabs %s 7', $branch_conditions[$condition]),
			'mov 0x0001u8 a0l',
			'mov a0l [0x$D682]',
			'@brabs always 5',
			'mov 0x0002u8 a0l',
			'mov a0l [0x$D682]',
		);
		add_case(sprintf('br absolute %s flags %s', $branch_conditions[$condition], $flags_set ? 'set' : 'clear'),
			\@setup, \@body);

		@body = (
			sprintf('call 0x0000$%04X %s', $control_helper_address,
				$branch_conditions[$condition] eq 'nn' ? 'mn' : $branch_conditions[$condition]),
			'mov 0x0003u8 a0l',
			'mov a0l [0x$D682]',
		);
		add_case(sprintf('call absolute %s flags %s', $branch_conditions[$condition], $flags_set ? 'set' : 'clear'),
			\@setup, \@body);
	}
}

for my $accumulator (qw(a0l a1l)) {
	my @setup = base_setup();
	push @setup, sprintf('mov 0x$%04X %s', $control_helper_address, $accumulator);
	add_case("calla $accumulator and ret", \@setup,
		[ "calla $accumulator", 'mov 0x0003u8 a0l', 'mov a0l [0x$D682]' ]);
}

add_case('retd executes one two-cycle delay instruction', [ base_setup() ],
	[ sprintf('call 0x0000$%04X always', $control_helper_address + 0x10),
		'mov 0x0003u8 a0l', 'mov a0l [0x$D682]' ]);

for my $return ([ 0x20, 0 ], [ 0x28, 1 ], [ 0x30, 3 ], [ 0x38, 255 ]) {
	my ($target_offset, $offset) = @$return;
	add_case("rets adjusts sp by $offset", [ base_setup() ],
		[ sprintf('call 0x0000$%04X always', $control_helper_address + $target_offset),
			'mov 0x0003u8 a0l', 'mov a0l [0x$D682]' ]);
}

for my $count (0, 1, 2, 3, 7, 255) {
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	add_case("bkrep immediate two-word body count $count", \@setup,
		[ sprintf('@bkrep 0x%04xu8 3', $count), 'inc a0 always', 'inc a0 always' ]);
}

for my $count (0, 1, 3, 7) {
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	push @setup, sprintf('mov 0x$%04X r1', $count);
	add_case("bkrep register two-word body count $count", \@setup,
		[ '@bkrep r1 3', 'inc a0 always', 'inc a0 always' ]);
}

{
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	add_case('bkrep nested distinct end addresses', \@setup,
		[ '@bkrep 0x0001u8 6', '@bkrep 0x0001u8 3', 'inc a0 always', 'inc a0 always', 'inc a0 always' ]);
}

my @all_accumulators = qw(a0 a1 b0 b1);
for my $source (@all_accumulators) {
	for my $destination (@all_accumulators) {
		next if $source eq $destination;
		add_case("mov full accumulator $source to $destination", [ four_accumulator_setup() ],
			"mov $source $destination");
	}
}

my @swap_options = (
	'a0 b0',
	'a0 b1',
	'a1 b0',
	'a1 b1',
	'a0 b0 and a1 b1',
	'a0 b1 and a1 b0',
	'a0 to b0 to a1',
	'a0 to b1 to a1',
	'a1 to b0 to a0',
	'a1 to b1 to a0',
	'b0 to a0 to b1',
	'b0 to a1 to b1',
	'b1 to a0 to b0',
	'b1 to a1 to b0',
);
for my $option (0 .. $#swap_options) {
	add_case("swap $swap_options[$option]", [ four_accumulator_setup() ],
		sprintf('data %04X // swap option %u; PDF Table 4-4', 0x4980 | $option, $option));
}

my @alias_shift_vectors = (
	[ -31, 0 ],
	[ -1, 0 ],
	[ 0, 0 ],
	[ 1, 1 ],
	[ 31, 1 ],
);
for my $accumulator (@all_accumulators) {
	for my $vector (@alias_shift_vectors) {
		my ($shift, $logic) = @$vector;
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X st2', $logic << 7);
		my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
		add_case(sprintf('shfi alias %s shift %d mode %s', $accumulator, $shift,
			$logic ? 'logic' : 'arithmetic'), \@setup, "shfi $accumulator $accumulator $immediate");
	}
}

for my $accumulator (@all_accumulators) {
	for my $vector ([ -32, 0 ], [ -32, 1 ], [ 0, 1 ], [ 31, 0 ]) {
		my ($shift, $logic) = @$vector;
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X st2', $logic << 7);
		my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
		add_case(sprintf('shfi alias boundary %s shift %d mode %s', $accumulator, $shift,
			$logic ? 'logic' : 'arithmetic'), \@setup, "shfi $accumulator $accumulator $immediate");
	}
}

for my $source (@all_accumulators) {
	for my $destination (@all_accumulators) {
		next if $source eq $destination;
		for my $vector ([ -1, 0 ], [ 1, 1 ]) {
			my ($shift, $logic) = @$vector;
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X st2', $logic << 7);
			my $immediate = sprintf('%s0x%04x', $shift < 0 ? '-' : '+', abs($shift));
			add_case(sprintf('shfi cross %s to %s shift %d mode %s', $source, $destination, $shift,
				$logic ? 'logic' : 'arithmetic'), \@setup, "shfi $source $destination $immediate");
		}
	}
}

my @alias_shift_register_vectors = (
	[ -36, 0 ],
	[ -1, 0 ],
	[ 0, 0 ],
	[ 1, 1 ],
	[ 36, 1 ],
);
for my $accumulator (@all_accumulators) {
	for my $vector (@alias_shift_register_vectors) {
		my ($shift, $logic) = @$vector;
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X sv', $shift & 0xFFFF), sprintf('mov 0x$%04X st2', $logic << 7);
		add_case(sprintf('shfc alias %s shift %d mode %s', $accumulator, $shift,
			$logic ? 'logic' : 'arithmetic'), \@setup, "shfc $accumulator $accumulator always");
	}
}

for my $source (@all_accumulators) {
	for my $destination (@all_accumulators) {
		next if $source eq $destination;
		for my $vector ([ -1, 0 ], [ 1, 1 ]) {
			my ($shift, $logic) = @$vector;
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X sv', $shift & 0xFFFF), sprintf('mov 0x$%04X st2', $logic << 7);
			add_case(sprintf('shfc cross %s to %s shift %d mode %s', $source, $destination, $shift,
				$logic ? 'logic' : 'arithmetic'), \@setup, "shfc $source $destination always");
		}
	}
}

my @round_vectors = (
	[ 'below half', 0x7FFF, 0x0000, 0x0 ],
	[ 'exact half', 0x8000, 0x0000, 0x0 ],
	[ 'low carry', 0xFFFF, 0x0000, 0x0 ],
	[ 'high carry', 0x8000, 0xFFFF, 0x0 ],
	[ 'negative half', 0x8000, 0xFFFF, 0xF ],
	[ 'negative low carry', 0xFFFF, 0xFFFF, 0xF ],
);
for my $accumulator (qw(a0 a1)) {
	for my $vector (@round_vectors) {
		my ($name, $low, $high, $extension) = @$vector;
		my @setup = four_accumulator_setup();
		push @setup, accumulator_value_setup($accumulator, $low, $high, $extension), 'mov 0x$0011 r0';
		add_case("rnd $accumulator $name", \@setup, "rnd $accumulator always");
	}
}

for my $destination (qw(a0 a1)) {
	for my $ps (0 .. 3) {
		for my $factor (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
			my @gaps = $destination eq 'a0' ? (0, 2) : (0, 1, 2);
			for my $gap (@gaps) {
				my @setup = four_accumulator_setup();
				push @setup, sprintf('load 0x%04X ps', $ps), sprintf('mov 0x$%04X r0', $factor);
				my @body = ('data 8040 // mpy y0,r0; pipeline producer', ('nop') x $gap,
					"pacr $destination always");
				add_case(sprintf('mpy pacr %s ps%u factor %04X gap %u', $destination, $ps, $factor, $gap),
					\@setup, \@body);
			}
		}
	}
}

for my $operation (qw(msu sqra)) {
	for my $factor (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
		for my $gap (0 .. 2) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X r0', $factor), 'mov 0x$0003 r1';
			my @body = ('data 8040 // mpy y0,r0; pipeline producer', ('nop') x $gap, "$operation r1 a0");
			add_case(sprintf('mpy %s pipeline factor %04X gap %u', $operation, $factor, $gap), \@setup, \@body);
		}
	}
}

{
	my @setup = accumulator_setup(0x0000, 0x0000, 0x0000);
	add_case('bkrep three nested levels', \@setup,
		[ '@bkrep 0x0001u8 9', '@bkrep 0x0001u8 6', '@bkrep 0x0001u8 3',
			'inc a0 always', 'inc a0 always', 'inc a0 always', 'inc a0 always' ]);
	add_case('bkrep four nested levels', \@setup,
		[ '@bkrep 0x0001u8 12', '@bkrep 0x0001u8 9', '@bkrep 0x0001u8 6', '@bkrep 0x0001u8 3',
			'inc a0 always', 'inc a0 always', 'inc a0 always', 'inc a0 always', 'inc a0 always' ]);
}

for my $operation (qw(add sub cmp and or xor)) {
	for my $source (qw(a0 a1)) {
		my $destination = $source eq 'a0' ? 'a1' : 'a0';
		add_case("$operation joint $source to $destination", [ four_accumulator_setup() ],
			"$operation $source $destination");
	}
}

for my $operation (qw(clr clrr inc dec neg not shl shl4 shr shr4 rol ror)) {
	add_case("$operation a1 selector", [ four_accumulator_setup() ], "$operation a1 always");
}
for my $accumulator (qw(a0 a1)) {
	add_case("copy $accumulator selector", [ four_accumulator_setup() ], "copy $accumulator always");
}

for my $operation (qw(addh addl subh subl)) {
	my @setup = four_accumulator_setup();
	push @setup, 'mov 0x$8001 r0';
	add_case("$operation r0 a1 selector", \@setup, "$operation r0 a1");
}

for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
	my @setup = four_accumulator_setup();
	push @setup, sprintf('mov 0x$%04X r0', $value);
	add_case(sprintf('exp r0 a1 selector %04X', $value), \@setup, 'exp r0 a1');
	add_case(sprintf('cmpu r0 a1 selector %04X', $value), \@setup, 'cmpu r0 a1');
}

for my $vector (@norm_vectors) {
	my ($name, $low, $high, $st1) = @$vector;
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			my @setup = accumulator1_setup($low, $high, $st1, 0x0000);
			push @setup, sprintf('mov 0x$D600 r%u', $register), 'load +0x0003 stepi';
			add_case("norm a1 r$register $name" . ($step eq '' ? ' no step' : $step), \@setup,
				"norm a1 [r$register$step]");
		}
	}
}

for my $operation (qw(movs movr)) {
	for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value);
		add_case(sprintf('%s r0 a1 selector %04X', $operation, $value), \@setup, "$operation r0 a1");
	}
}

for my $vector (@division_vectors) {
	my ($dividend, $divisor) = @$vector;
	for my $steps (1, 16) {
		my @setup = accumulator1_setup($dividend, 0x0000, 0x0000, 0x0000);
		push @setup, 'load 0x00d6u8 page', sprintf('mov 0x$%04X r1', $divisor), 'mov r1 [page:0x0000u8]';
		my @body = ('divs [page:0x0000u8] a1') x $steps;
		add_case(sprintf('divs a1 dividend %04X divisor %04X steps %u', $dividend, $divisor, $steps),
			\@setup, \@body);
	}
}

for my $operation (qw(msu sqra)) {
	for my $value (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555) {
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X r0', $value);
		add_case(sprintf('%s r0 a1 selector %04X', $operation, $value), \@setup, "$operation r0 a1");
	}

	for my $factor (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
		for my $gap (0 .. 2) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X r0', $factor), 'mov 0x$0003 r1';
			my @body = ('data 8040 // mpy y0,r0; pipeline producer', ('nop') x $gap, "$operation r1 a1");
			add_case(sprintf('mpy %s a1 pipeline factor %04X gap %u', $operation, $factor, $gap), \@setup, \@body);
		}
	}
}

for my $operation (qw(add sub cmp and or xor)) {
	for my $vector (@alu_vectors) {
		my ($vector_name, $low, $high, $status, $operand) = @$vector;
		add_case(
			"$operation immediate a1 $vector_name",
			[ accumulator1_setup($low, $high, $status & 0xF000, 0x0000) ],
			sprintf('%s 0x$%04X a1', $operation, $operand),
		);
	}

	for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF) {
		add_case(
			sprintf('%s short immediate a1 %02X', $operation, $value),
			[ accumulator1_setup(0xA55A, 0x1357, 0x0000, 0x0000) ],
			sprintf('%s 0x%04xu8 a1', $operation, $value),
		);
	}
}

for my $case (@register_operations) {
	my ($name, $instruction, $low, $high, undef, $r0) = @$case;
	my @setup = accumulator1_setup($low, $high, 0x0000, 0x0000);
	push @setup, sprintf('mov 0x$%04X r0', $r0);
	$instruction =~ s/a0$/a1/;
	add_case("$name a1 selector", \@setup, $instruction);
}

for my $operation (@indirect_operations) {
	my ($mnemonic, $value) = @$operation;
	for my $step ('', '++', '--', '++s') {
		my @setup = accumulator1_setup(0xFFFF, 0x2468, 0x0000, 0x0000);
		push @setup,
			'mov 0x$D600 r0',
			sprintf('mov 0x$%04X r1', $value),
			'mov r1 [r0]',
			'mov 0x$D600 r0',
			'load +0x0003 stepi';
		add_case(
			sprintf('%s indirect a1 r0%s', $mnemonic, $step eq '' ? ' no step' : $step),
			\@setup,
			"$mnemonic [r0$step] a1",
		);
	}
}

for my $operation (qw(add sub cmp and or xor)) {
	for my $long (0, 1) {
		my @setup = accumulator1_setup(0xFFFF, 0x2468, 0x0000, 0x0000);
		push @setup, 'mov 0x$D600 r0', 'mov 0x$0001 r1', 'mov r1 [r0]', 'load 0x00d6u8 page';
		my $operand = $long ? '[0x$D600]' : '[page:0x0000u8]';
		add_case("$operation direct a1 " . ($long ? 'long' : 'page'), \@setup, "$operation $operand a1");
	}
}

for my $offset (0x00, 0x01, 0x7F, 0x80, 0xFF) {
	my @setup = base_setup();
	push @setup,
		'load 0x00d6u8 page',
		'mov 0x$A55A r1',
		sprintf('mov r1 [page:0x%04xu8]', $offset);
	add_case(sprintf('mov page direct to a1 offset %02X', $offset), \@setup,
		sprintf('mov [page:0x%04xu8] a1', $offset));
}

for my $destination (qw(a1l a1h)) {
	my @setup = base_setup();
	push @setup, 'load 0x00d6u8 page', 'mov 0x$A55A r1', 'mov r1 [page:0x0000u8]';
	add_case("mov page direct to $destination", \@setup, "mov [page:0x0000u8] $destination");
}

{
	my @setup = base_setup();
	push @setup, 'mov 0x$D600 r0', 'mov 0x$A55A r1', 'mov r1 [r0]';
	add_case('mov long absolute to a1', \@setup, 'mov [0x$D600] a1');
}

for my $value (@signed_short_values) {
	my ($name, $immediate) = @$value;
	add_case("mov short immediate a1h $name", [ base_setup() ], "mov $immediate a1h");
}
for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF) {
	add_case(sprintf('mov unsigned short immediate a1l %02X', $value), [ base_setup() ],
		sprintf('mov 0x%04xu8 a1l', $value));
}

for my $offset (@r7_offsets[0 .. 3]) {
	my ($name, $operand, $delta) = @$offset;
	my $address = 0xD700 + $delta;
	my @setup = base_setup();
	push @setup,
		sprintf('mov 0x$%04X r0', $address & 0xFFFF),
		'mov 0x$A55A r1',
		'mov r1 [r0]',
		'mov 0x$D700 r7';
	add_case("mov r7 relative a1 $name", \@setup, "mov $operand a1");
}

for my $register (0 .. 5) {
	for my $step ('', '++', '--', '++s') {
		my @setup = base_setup();
		push @setup,
			'mov 0x$D600 r0',
			'mov 0x$A55A r1',
			'mov r1 [r0]',
			sprintf('mov 0x$D600 r%u', $register),
			'load +0x0003 stepi';
		my @body = (sprintf('mov [r%u%s] a1', $register, $step));
		push @body, sprintf('mov r%u r0', $register) if $register != 0;
		add_case(sprintf('mov indirect read a1 r%u%s', $register, $step eq '' ? ' no step' : $step), \@setup,
			\@body);
	}
}

my @round_saturation_vectors = (
	[ 'positive boundary', 0x8000, 0x7FFF, 0x0 ],
	[ 'positive extended', 0x8000, 0xFFFF, 0x0 ],
	[ 'negative boundary', 0x8000, 0x8000, 0xF ],
	[ 'negative near zero', 0x8000, 0xFFFF, 0xF ],
);
for my $accumulator (qw(a0 a1)) {
	for my $vector (@round_saturation_vectors) {
		my ($name, $low, $high, $extension) = @$vector;
		for my $disabled (0, 1) {
			my @setup = four_accumulator_setup();
			push @setup, accumulator_value_setup($accumulator, $low, $high, $extension);
			push @setup, sprintf('mov 0x$%04X st0', ($accumulator eq 'a0' ? $extension << 12 : 0) | $disabled);
			add_case("rnd $accumulator $name saturation " . ($disabled ? 'disabled' : 'enabled'), \@setup,
				"rnd $accumulator always");
			my $lim = $accumulator eq 'a0' ? 0x49C0 : 0x49F0;
			add_case("rnd lim $accumulator $name saturation " . ($disabled ? 'disabled' : 'enabled'), \@setup,
				[ "rnd $accumulator always", sprintf('data %04X // lim %s; PDF Table 4-4', $lim, $accumulator) ]);
		}
	}
}

my @long_product_consumers = (
	[ 'mac a0', 'mac y0 r1 a0' ],
	[ 'mac a1', 'mac y0 r1 a1' ],
	[ 'mov p a0', 'data 5B0B // mov p,a0; PDF Table 4-4' ],
	[ 'mov p a1', 'data 5B2B // mov p,a1; PDF Table 4-4' ],
	[ 'pacr a0', 'pacr a0 always' ],
	[ 'pacr a1', 'pacr a1 always' ],
	[ 'msu a0', 'msu r1 a0' ],
	[ 'msu a1', 'msu r1 a1' ],
	[ 'sqra a0', 'sqra r1 a0' ],
	[ 'sqra a1', 'sqra r1 a1' ],
);
for my $consumer (@long_product_consumers) {
	my ($name, $instruction) = @$consumer;
	for my $factor (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
		for my $gap (3, 4, 7) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X r0', $factor), 'mov 0x$0003 r1';
			my @body = ('data 8040 // mpy y0,r0; pipeline producer', ('nop') x $gap, $instruction);
			add_case(sprintf('mpy long gap %s factor %04X gap %u', $name, $factor, $gap), \@setup, \@body);
		}
	}
}

# A low-half accumulator store changes L, while the readback into a0 changes
# arithmetic flags. Preserve st0 on the stack across the independent readback.
for my $register (0 .. 5) {
	for my $step ('', '++', '--', '++s') {
		my @setup = base_setup();
		push @setup,
			'mov 0x$C33C a0l',
			sprintf('mov 0x$D600 r%u', $register),
			sprintf('mov a0l [r%u]', $register),
			'nop',
			'nop',
			'mov 0x$A55A a1l',
			sprintf('mov 0x$D600 r%u', $register),
			'load +0x0003 stepi';
		my @body = (
			sprintf('mov a1l [r%u%s]', $register, $step),
			'nop',
			'nop',
			sprintf('mov r%u r7', $register),
			'push st0',
			sprintf('mov 0x$D600 r%u', $register),
			sprintf('mov [r%u] a0', $register),
			'pop st0',
			'mov r7 r0',
		);
		add_case(sprintf('mov indirect write a1l r%u%s', $register, $step eq '' ? ' no step' : $step),
			\@setup, \@body);
	}
}

my @unsigned_product_consumers = (
	[ 'mov p', [
		'data 5B0B // mov p,a0; PDF Table 4-4',
		'data 5B2B // mov p,a1; PDF Table 4-4',
	] ],
	[ 'mac', [ 'mac y0 r1 a0', 'mac y0 r1 a1' ] ],
	[ 'pacr', [ 'pacr a0 always', 'pacr a1 always' ] ],
	[ 'msu', [ 'msu r1 a0', 'msu r1 a1' ] ],
	[ 'sqra', [ 'sqra r1 a0', 'sqra r1 a1' ] ],
);
for my $consumer (@unsigned_product_consumers) {
	my ($consumer_name, $instructions) = @$consumer;
	for my $producer_accumulator (qw(a0 a1)) {
		for my $consumer_index (0, 1) {
			my $consumer_accumulator = $consumer_index ? 'a1' : 'a0';
			for my $ps (0 .. 3) {
				for my $factor (0x0001, 0x7FFF, 0x8000, 0xFFFF) {
					for my $gap (0, 2, 5) {
						my @setup = four_accumulator_setup();
						push @setup,
							sprintf('load 0x%04X ps', $ps),
							sprintf('mov 0x$%04X r0', $factor),
							'mov 0x$0003 r1';
						my @body = ("macuu y0 r0 $producer_accumulator", ('nop') x $gap,
							$instructions->[$consumer_index]);
						add_case(sprintf('macuu unsigned %s producer %s consumer %s ps%u factor %04X gap %u',
							$consumer_name, $producer_accumulator, $consumer_accumulator, $ps, $factor, $gap),
							\@setup, \@body);
					}
				}
			}
		}
	}
}

my @product_chain_vectors = (
	[ 0x0001, 0x0003, 0x0005, 0x0007 ],
	[ 0x7FFF, 0x8000, 0xFFFF, 0x0001 ],
	[ 0x8000, 0xFFFF, 0x7FFF, 0x8001 ],
	[ 0xFFFF, 0xAAAA, 0x5555, 0xFFFF ],
);
my @product_chain_gaps = (
	[ 0, 0, 0, 0 ],
	[ 1, 2, 3, 4 ],
	[ 5, 1, 4, 7 ],
);
my @product_chain_paths = (
	[ qw(a0 a1 a0 a1) ],
	[ qw(a1 a0 a1 a0) ],
);
for my $unsigned_transition (0, 1) {
	for my $path (@product_chain_paths) {
		for my $ps (0 .. 3) {
			for my $vector (@product_chain_vectors) {
				my ($r0, $r1, $r2, $r3) = @$vector;
				for my $gaps (@product_chain_gaps) {
					my @setup = four_accumulator_setup();
					push @setup,
						sprintf('load 0x%04X ps', $ps),
						sprintf('mov 0x$%04X r0', $r0),
						sprintf('mov 0x$%04X r1', $r1),
						sprintf('mov 0x$%04X r2', $r2),
						sprintf('mov 0x$%04X r3', $r3);
					my $first_consumer = $unsigned_transition ? 'macuu' : 'mac';
					my $final_move = $path->[3] eq 'a0' ?
						'data 5B0B // mov p,a0; PDF Table 4-4' :
						'data 5B2B // mov p,a1; PDF Table 4-4';
					my @body = (
						'data 8040 // mpy y0,r0; signed pipeline producer',
						('nop') x $gaps->[0],
						"$first_consumer y0 r1 $path->[0]",
						('nop') x $gaps->[1],
						"mac y0 r2 $path->[1]",
						('nop') x $gaps->[2],
						"mac y0 r3 $path->[2]",
						('nop') x $gaps->[3],
						$final_move,
					);
					add_case(sprintf('%s product chain %s-%s-%s-%s ps%u values %04X-%04X-%04X-%04X gaps %u-%u-%u-%u',
						$unsigned_transition ? 'unsigned transition' : 'signed', @$path, $ps, @$vector, @$gaps),
						\@setup, \@body);
				}
			}
		}
	}
}

my @operand_multiply_operations = ('mpy', map { $_->[0] } @multiply_operations);
my @multiply_registers = qw(r0 r1 r2 r3 r4 r5 r7 y0 st0 st1 a0l a1l a0h a1h lc sv);
for my $operation (@operand_multiply_operations) {
	for my $accumulator (qw(a0 a1)) {
		for my $register (@multiply_registers) {
			for my $value (0x0001, 0x8000) {
				my @setup = four_accumulator_setup();
				push @setup, sprintf('mov 0x$%04X %s', $value, $register);
				add_case(sprintf('%s register matrix y0 %s %s value %04X', $operation, $register, $accumulator,
					$value), \@setup, "$operation y0 $register $accumulator");
			}
		}
	}
}

my @multiply_memory_vectors = (
	[ 0x0001, 0x8000 ],
	[ 0x8001, 0xFFFF ],
);
for my $operation (@operand_multiply_operations) {
	for my $accumulator (qw(a0 a1)) {
		for my $register (0 .. 5) {
			for my $step ('', '++', '--', '++s') {
				for my $vector (@multiply_memory_vectors) {
					my ($memory, $ps) = @$vector;
					my @setup = four_accumulator_setup();
					push @setup,
						'load 0x00d6u8 page',
						sprintf('mov 0x$%04X r0', $memory),
						'mov r0 [page:0x0000u8]',
						'nop',
						'nop',
						sprintf('mov 0x$D600 r%u', $register),
						'load +0x0003 stepi',
						sprintf('load 0x%04X ps', $ps & 3);
					add_case(sprintf('%s y0 indirect r%u%s %s memory %04X ps%u', $operation, $register,
						$step eq '' ? ' no step' : $step, $accumulator, $memory, $ps & 3), \@setup,
						"$operation y0 [r$register$step] $accumulator");
				}
			}
		}
	}
}

my @dual_multiply_steps;
for my $y_step ('', '++', '--', '++s') {
	for my $x_step ('', '++', '--', '++s') {
		push @dual_multiply_steps, [ $y_step, $x_step ];
	}
}
for my $operation (@operand_multiply_operations) {
	for my $accumulator (qw(a0 a1)) {
		for my $y_register (4, 5) {
			for my $x_register (0 .. 3) {
				for my $steps (@dual_multiply_steps) {
					my ($y_step, $x_step) = @$steps;
					my @setup = four_accumulator_setup();
					push @setup,
						'load 0x00d6u8 page',
						'mov 0x$8001 r0',
						'mov r0 [page:0x0040u8]',
						'mov 0x$FFFF r0',
						'mov r0 [page:0x0000u8]',
						'nop',
						'nop',
						sprintf('mov 0x$D640 r%u', $y_register),
						sprintf('mov 0x$D600 r%u', $x_register),
						'load +0x0003 stepi',
						'load 0x0003 ps';
					add_case(sprintf('%s dual indirect r%u%s r%u%s %s', $operation, $y_register,
						$y_step eq '' ? ' no step' : $y_step, $x_register,
						$x_step eq '' ? ' no step' : $x_step, $accumulator), \@setup,
						"$operation [r$y_register$y_step] [r$x_register$x_step] $accumulator");
				}
			}
		}
	}
}

for my $operation (@operand_multiply_operations) {
	for my $accumulator (qw(a0 a1)) {
		for my $register (0 .. 5) {
			for my $step ('', '++', '--', '++s') {
				for my $vector (@multiply_memory_vectors) {
					my ($memory, $immediate) = @$vector;
					my @setup = four_accumulator_setup();
					push @setup,
						'load 0x00d6u8 page',
						sprintf('mov 0x$%04X r0', $memory),
						'mov r0 [page:0x0000u8]',
						'nop',
						'nop',
						sprintf('mov 0x$D600 r%u', $register),
						'load +0x0003 stepi';
					add_case(sprintf('%s long immediate r%u%s %s memory %04X immediate %04X', $operation,
						$register, $step eq '' ? ' no step' : $step, $accumulator, $memory, $immediate), \@setup,
						sprintf('%s [r%u%s] 0x$%04X %s', $operation, $register, $step, $immediate,
							$accumulator));
				}
			}
		}
	}
}

my @direct_multiply_operations = qw(mpy mac maa macsu);
for my $operation (@direct_multiply_operations) {
	for my $accumulator (qw(a0 a1)) {
		for my $offset (0x00, 0x55, 0xAA, 0xFF) {
			my $value = ($offset << 8) | ($offset ^ 0xFF);
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				sprintf('mov 0x$%04X r0', $value),
				sprintf('mov r0 [page:0x%04xu8]', $offset),
				'nop',
				'nop';
			add_case(sprintf('%s page direct %02X %s value %04X', $operation, $offset, $accumulator, $value),
				\@setup, sprintf('%s y0 [page:0x%04xu8] %s', $operation, $offset, $accumulator));
		}
	}
}

my @alm_operations = qw(or and xor add tst0 tst1 cmp sub msu addh addl subh subl sqr sqra cmpu);
my @alm_registers = (
	[ r0 => 0 ], [ r1 => 1 ], [ r2 => 2 ], [ r3 => 3 ], [ r4 => 4 ], [ r5 => 5 ],
	[ r7 => 6 ], [ y0 => 7 ], [ st0 => 8 ], [ st1 => 9 ], [ st2 => 10 ], [ sp => 13 ],
	[ cfgi => 14 ], [ cfgj => 15 ], [ a0l => 26 ], [ a1l => 27 ], [ a0h => 28 ],
	[ a1h => 29 ], [ lc => 30 ], [ sv => 31 ],
);

for my $operation (0 .. $#alm_operations) {
	for my $accumulator (0 .. 1) {
		for my $source (@alm_registers) {
			my ($name, $register) = @$source;
			next if ($operation == 4 || $operation == 5) && $name eq "a${accumulator}l";
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$8001 %s', $name) unless $name eq 'sp';
			add_case(sprintf('ALM register %s source %s a%u', $alm_operations[$operation], $name, $accumulator),
				\@setup, sprintf('data %04X // ALM register %s,a%u; PDF Table 4-4',
					0x80A0 | $operation << 9 | $accumulator << 8 | $register,
					$alm_operations[$operation], $accumulator));
		}
	}
}

for my $operation (0 .. $#alm_operations) {
	for my $accumulator (0 .. 1) {
		for my $register (0 .. 5) {
			for my $modifier (0 .. 3) {
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					'mov 0x$8001 r0',
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D600 r%u', $register),
					'load +0x0003 stepi';
				add_case(sprintf('ALM indirect %s r%u modifier%u a%u', $alm_operations[$operation], $register,
					$modifier, $accumulator), \@setup,
					sprintf('data %04X // ALM (rN) %s,a%u; PDF Table 4-4',
						0x8080 | $operation << 9 | $accumulator << 8 | $modifier << 3 | $register,
						$alm_operations[$operation], $accumulator));
			}
		}
	}
}

for my $operation (0 .. $#alm_operations) {
	for my $accumulator (0 .. 1) {
		for my $offset (0x00, 0x7F, 0x80, 0xFF) {
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				'mov 0x$8001 r0',
				sprintf('mov r0 [page:0x%04xu8]', $offset),
				'nop',
				'nop';
			add_case(sprintf('ALM direct %s offset%02X a%u', $alm_operations[$operation], $offset,
				$accumulator), \@setup, sprintf('data %04X // ALM direct %s,a%u; PDF Table 4-4',
					0xA000 | $operation << 9 | $accumulator << 8 | $offset,
					$alm_operations[$operation], $accumulator));
		}
	}
}

my @alb_operations = qw(set rst chng addv tst0 tst1 cmpv subv);
my @alb_registers = qw(r0 r1 r2 r3 r4 r5 r7 y0 st0 st1 a0l a1l a0h a1h lc sv);
for my $operation (@alb_operations) {
	for my $register (@alb_registers) {
		for my $immediate (0x8001, 0xFFFF) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$C33C %s', $register);
			add_case(sprintf('%s ALB register %s immediate %04X', $operation, $register, $immediate), \@setup,
				sprintf('%s 0x$%04X %s', $operation, $immediate, $register));
		}
	}
}

for my $operation (@alb_operations) {
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			for my $immediate (0x0000, 0x8001, 0xFFFF) {
				my @setup = four_accumulator_setup();
				push @setup, sprintf('mov 0x$D680 r%u', $register), 'load +0x0003 stepi';
				add_case(sprintf('%s ALB indirect r%u%s immediate %04X', $operation, $register,
					$step eq '' ? ' no step' : $step, $immediate), \@setup,
					sprintf('%s 0x$%04X [r%u%s]', $operation, $immediate, $register, $step));
			}
		}
	}
}

for my $operation (@alb_operations) {
	for my $offset (0x00, 0x7F, 0x80, 0xFF) {
		for my $immediate (0x8001, 0xFFFF) {
			my $initial = 0xC33C ^ ($offset << 8);
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				sprintf('mov 0x$%04X r0', $initial),
				sprintf('mov r0 [page:0x%04xu8]', $offset),
				'nop',
				'nop';
			my @body = (
				sprintf('%s 0x$%04X [page:0x%04xu8]', $operation, $immediate, $offset),
				'push st0',
				sprintf('mov [page:0x%04xu8] a0', $offset),
				'pop st0',
			);
			add_case(sprintf('%s ALB page direct %02X initial %04X immediate %04X', $operation, $offset,
				$initial, $immediate), \@setup, \@body);
		}
	}
}

for my $source (qw(a0l a1l)) {
	for my $address (0xD600, 0xD67F, 0xD680, 0xD6FF) {
		for my $value (0x0000, 0x8001, 0xFFFF) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X %s', $value, $source);
			my @body = (
				sprintf('mov %s [0x$%04X]', $source, $address),
				'nop',
				'nop',
				'push st0',
				sprintf('mov [0x$%04X] a0', $address),
				'pop st0',
			);
			add_case(sprintf('mov long absolute store %s address %04X value %04X', $source, $address, $value),
				\@setup, \@body);
		}
	}
}

for my $source (qw(a0l a1l)) {
	for my $offset (@r7_offsets) {
		my ($name, $operand, $delta) = @$offset;
		for my $value (0x0000, 0x8001, 0xFFFF) {
			my @setup = four_accumulator_setup();
			push @setup,
				sprintf('mov 0x$%04X %s', $value, $source),
				sprintf('mov 0x$%04X r7', (0xD680 - $delta) & 0xFFFF);
			my $body = defined($operand) ? "mov $source $operand" : [
				sprintf('data %04X // mov %s,(rb+long offset); PDF Table 4-4',
					$source eq 'a0l' ? 0xD49C : 0xDC9C, $source),
				sprintf('data %04X // rb long offset expansion', $delta & 0xFFFF),
			];
			add_case(sprintf('mov r7 relative store %s %s value %04X', $source, $name, $value), \@setup, $body);
		}
	}
}

my %r7_long_alu_operations = (or => 0, and => 1, xor => 2, add => 3, cmp => 6, sub => 7);
for my $operation (qw(add sub cmp and or xor)) {
	for my $accumulator (qw(a0 a1)) {
		for my $offset (@r7_offsets) {
			my ($name, $operand, $delta) = @$offset;
			my $address = 0xD700 + $delta;
			my @setup = four_accumulator_setup();
			push @setup,
				sprintf('mov 0x$%04X r0', $address & 0xFFFF),
				'mov 0x$8001 r1',
				'mov r1 [r0]',
				'nop',
				'nop',
				'mov 0x$D700 r7';
			my $body = defined($operand) ? "$operation $operand $accumulator" : [
				sprintf('data %04X // %s (rb+long offset),%s; PDF Table 4-4',
					0xD4D8 | ($accumulator eq 'a1' ? 0x0100 : 0) | $r7_long_alu_operations{$operation},
					$operation, $accumulator),
				sprintf('data %04X // rb long offset expansion', $delta & 0xFFFF),
			];
			add_case("$operation r7 relative $accumulator $name", \@setup, $body);
		}
	}
}

for my $accumulator (qw(a0 a1)) {
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			for my $vector (@multiply_memory_vectors) {
				my ($memory, $immediate) = @$vector;
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					sprintf('mov 0x$%04X r0', $memory),
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D600 r%u', $register),
					'load +0x0003 stepi';
				add_case(sprintf('msu long immediate r%u%s %s memory %04X immediate %04X', $register,
					$step eq '' ? ' no step' : $step, $accumulator, $memory, $immediate), \@setup,
					sprintf('msu [r%u%s] 0x$%04X %s', $register, $step, $immediate, $accumulator));
			}
		}
	}
}

for my $accumulator (qw(a0 a1)) {
	for my $y_register (4, 5) {
		for my $x_register (0 .. 3) {
			for my $steps (@dual_multiply_steps) {
				my ($y_step, $x_step) = @$steps;
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					'mov 0x$8001 r0',
					'mov r0 [page:0x0040u8]',
					'mov 0x$FFFF r0',
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D640 r%u', $y_register),
					sprintf('mov 0x$D600 r%u', $x_register),
					'load +0x0003 stepi',
					'load 0x0003 ps';
				add_case(sprintf('msu dual indirect r%u%s r%u%s %s', $y_register,
					$y_step eq '' ? ' no step' : $y_step, $x_register,
					$x_step eq '' ? ' no step' : $x_step, $accumulator), \@setup,
					"msu [r$y_register$y_step] [r$x_register$x_step] $accumulator");
			}
		}
	}
}

for my $register (0 .. 5) {
	for my $step ('', '++', '--', '++s') {
		for my $bit (0, 1, 7, 8, 14, 15) {
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				'mov 0x$A55A r0',
				'mov r0 [page:0x0000u8]',
				'nop',
				'nop',
				sprintf('mov 0x$D600 r%u', $register),
				'load +0x0003 stepi';
			add_case(sprintf('tstb indirect r%u%s bit %u', $register, $step eq '' ? ' no step' : $step,
				$bit), \@setup, sprintf('tstb [r%u%s] 0x%04x', $register, $step, $bit));
		}
	}
}

for my $offset (0x00, 0x7F, 0x80, 0xFF) {
	for my $bit (0, 1, 7, 8, 14, 15) {
		my @setup = four_accumulator_setup();
		push @setup,
			'load 0x00d6u8 page',
			'mov 0x$A55A r0',
			sprintf('mov r0 [page:0x%04xu8]', $offset),
			'nop',
			'nop';
		add_case(sprintf('tstb page direct %02X bit %u', $offset, $bit), \@setup,
			sprintf('tstb [page:0x%04xu8] 0x%04x', $offset, $bit));
	}
}

my @modb_operations = qw(shr shr4 shl shl4 ror rol clr);
for my $operation (@modb_operations) {
	for my $accumulator (qw(b0 b1)) {
		for my $value (0x0001, 0x8001) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X %s', $value, $accumulator), 'mov 0x$0080 st0';
			add_case(sprintf('%s modb %s value %04X', $operation, $accumulator, $value), \@setup,
				"$operation $accumulator always");
		}
	}
}

my @exp_values = (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF, 0x5555);
for my $accumulator (qw(a0 a1)) {
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			for my $value (@exp_values) {
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					sprintf('mov 0x$%04X r0', $value),
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D600 r%u', $register),
					'load +0x0003 stepi';
				add_case(sprintf('exp indirect r%u%s %s value %04X', $register,
					$step eq '' ? ' no step' : $step, $accumulator, $value), \@setup,
					sprintf('exp [r%u%s] %s', $register, $step, $accumulator));
			}
		}
	}
}

for my $source (qw(b0 b1)) {
	for my $accumulator (qw(a0 a1)) {
		for my $value (@exp_values) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X %s', $value, $source);
			add_case(sprintf('exp %s %s value %04X', $source, $accumulator, $value), \@setup,
				"exp $source $accumulator");
		}
	}
}

for my $register (0 .. 5) {
	for my $step_index (0 .. 3) {
		my $step = ('', '++', '--', '++s')[$step_index];
		for my $value (@exp_values) {
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				sprintf('mov 0x$%04X r0', $value),
				'mov r0 [page:0x0000u8]',
				'nop',
				'nop',
				sprintf('mov 0x$D600 r%u', $register),
				'load +0x0003 stepi';
			my $opcode = 0x9C40 | $step_index << 3 | $register;
			add_case(sprintf('exp indirect to sv r%u%s value %04X', $register,
				$step eq '' ? ' no step' : $step, $value), \@setup,
				sprintf('data %04X // exp (rN),sv; PDF Table 4-4', $opcode));
		}
	}
}

my @exp_registers = (
	[ 'r0', 0 ], [ 'r1', 1 ], [ 'r2', 2 ], [ 'r3', 3 ], [ 'r4', 4 ], [ 'r5', 5 ],
	[ 'r7', 6 ], [ 'y0', 7 ], [ 'st0', 8 ], [ 'st1', 9 ], [ 'a0l', 26 ], [ 'a1l', 27 ],
	[ 'a0h', 28 ], [ 'a1h', 29 ], [ 'lc', 30 ], [ 'sv', 31 ],
);
for my $register (@exp_registers) {
	my ($name, $encoding) = @$register;
	for my $value (0x0001, 0x8000) {
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X %s', $value, $name);
		add_case(sprintf('exp register to sv %s value %04X', $name, $value), \@setup,
			sprintf('data %04X // exp register,sv; PDF Table 4-4', 0x9440 | $encoding));
	}
}

for my $source_index (0, 1) {
	my $source = $source_index ? 'b1' : 'b0';
	for my $value (@exp_values) {
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X %s', $value, $source);
		add_case(sprintf('exp %s to sv value %04X', $source, $value), \@setup,
			sprintf('data %04X // exp bX,sv; PDF Table 4-4', 0x9460 | $source_index));
	}
}

my @safe_move_registers = qw(r0 r1 r2 r3 r4 r5 r7 y0 st0 st1 a0l a1l a0h a1h lc sv);
for my $source (@safe_move_registers) {
	for my $destination (qw(b0 b1)) {
		for my $value (0x0001, 0x8001) {
			my @setup = four_accumulator_setup();
			push @setup, sprintf('mov 0x$%04X %s', $value, $source);
			add_case(sprintf('mov register %s to %s value %04X', $source, $destination, $value), \@setup,
				"mov $source $destination");
		}
	}
}

for my $destination (qw(b0 b1)) {
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			for my $value (0x0001, 0x8001, 0xFFFF) {
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					sprintf('mov 0x$%04X r0', $value),
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D600 r%u', $register),
					'load +0x0003 stepi';
				add_case(sprintf('mov indirect r%u%s to %s value %04X', $register,
					$step eq '' ? ' no step' : $step, $destination, $value), \@setup,
					sprintf('mov [r%u%s] %s', $register, $step, $destination));
			}
		}
	}
}

for my $offset (0x00, 0x7F, 0x80, 0xFF) {
	for my $value (0x0001, 0x8001) {
		my @setup = four_accumulator_setup();
		push @setup,
			'load 0x00d6u8 page',
			sprintf('mov 0x$%04X r0', $value),
			sprintf('mov r0 [page:0x%04xu8]', $offset),
			'nop',
			'nop';
		add_case(sprintf('mov page direct %02X to sv value %04X', $offset, $value), \@setup,
			sprintf('mov [page:0x%04xu8] sv', $offset));
	}
}

for my $offset (0x00, 0x7F, 0x80, 0xFF) {
	for my $value (0x0000, 0x8001, 0xFFFF) {
		my @setup = four_accumulator_setup();
		push @setup, 'load 0x00d6u8 page', sprintf('mov 0x$%04X sv', $value);
		my @body = (
			sprintf('mov sv [page:0x%04xu8]', $offset),
			'nop',
			'nop',
			'push st0',
			sprintf('mov [page:0x%04xu8] a0', $offset),
			'pop st0',
		);
		add_case(sprintf('mov sv to page direct %02X value %04X', $offset, $value), \@setup, \@body);
	}
}

for my $destination (qw(b0 b1)) {
	for my $value (@exp_values) {
		add_case(sprintf('mov long immediate %s value %04X', $destination, $value), [ four_accumulator_setup() ],
			sprintf('mov 0x$%04X %s', $value, $destination));
	}
}

for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF) {
	add_case(sprintf('mov signed short sv value %02X', $value), [ four_accumulator_setup() ],
		sprintf('data %04X // mov short immediate,sv; PDF Table 4-4', 0x0500 | $value));
}

for my $destination (qw(r0 r1 r2 r3 r4 r5 r7 y0 a0l a1l a0h a1h)) {
	my @setup = four_accumulator_setup();
	push @setup, 'push 0x$A55A';
	add_case("mov stack top to $destination", \@setup, "mov [sp] $destination");
}

for my $destination (qw(b0 b1 a0 a1)) {
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			for my $vector ([ 0x0001, -1, 0 ], [ 0x8001, 1, 1 ]) {
				my ($value, $shift, $logic) = @$vector;
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					sprintf('mov 0x$%04X r0', $value),
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D600 r%u', $register),
					'load +0x0003 stepi',
					sprintf('mov 0x$%04X sv', $shift & 0xFFFF),
					sprintf('mov 0x$%04X st2', $logic << 7);
				add_case(sprintf('movs indirect r%u%s to %s value %04X shift %d mode %s', $register,
					$step eq '' ? ' no step' : $step, $destination, $value, $shift,
					$logic ? 'logic' : 'arithmetic'), \@setup,
					sprintf('movs [r%u%s] %s', $register, $step, $destination));
			}
		}
	}
}

for my $destination (qw(b0 b1 a0 a1)) {
	for my $offset (0x00, 0x7F, 0x80, 0xFF) {
		for my $vector ([ 0x0001, -1, 0 ], [ 0x8001, 1, 1 ]) {
			my ($value, $shift, $logic) = @$vector;
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				sprintf('mov 0x$%04X r0', $value),
				sprintf('mov r0 [page:0x%04xu8]', $offset),
				'nop',
				'nop',
				sprintf('mov 0x$%04X sv', $shift & 0xFFFF),
				sprintf('mov 0x$%04X st2', $logic << 7);
			add_case(sprintf('movs page direct %02X to %s value %04X shift %d mode %s', $offset, $destination,
				$value, $shift, $logic ? 'logic' : 'arithmetic'), \@setup,
				sprintf('movs [page:0x%04xu8] %s', $offset, $destination));
		}
	}
}

for my $accumulator (qw(a0 a1)) {
	for my $register (0 .. 5) {
		for my $step ('', '++', '--', '++s') {
			for my $value (0x0001, 0x8001, 0xFFFF) {
				my @setup = four_accumulator_setup();
				push @setup,
					'load 0x00d6u8 page',
					sprintf('mov 0x$%04X r0', $value),
					'mov r0 [page:0x0000u8]',
					'nop',
					'nop',
					sprintf('mov 0x$D600 r%u', $register),
					'load +0x0003 stepi';
				add_case(sprintf('movr indirect r%u%s to %s value %04X', $register,
					$step eq '' ? ' no step' : $step, $accumulator, $value), \@setup,
					sprintf('movr [r%u%s] %s', $register, $step, $accumulator));
			}
		}
	}
}

my @movr_special_registers = (0, 4, 2, 5);
my @movr_special_steps = ('++', '--', '', '++s');
my @movr_special_destinations = qw(b0h b1h a0h a1h);
for my $register_index (0 .. $#movr_special_registers) {
	my $register = $movr_special_registers[$register_index];
	for my $step_index (0 .. $#movr_special_steps) {
		my $step = $movr_special_steps[$step_index];
		for my $destination_index (0 .. $#movr_special_destinations) {
			my $destination = $movr_special_destinations[$destination_index];
			my @setup = four_accumulator_setup();
			push @setup,
				'load 0x00d6u8 page',
				'mov 0x$8001 r0',
				'mov r0 [page:0x0000u8]',
				'nop',
				'nop',
				sprintf('mov 0x$D600 r%u', $register),
				'load +0x0003 stepi';
			my $opcode = 0x8864 | $register_index << 3 | $step_index | $destination_index << 8;
			add_case(sprintf('movr special r%u%s to %s', $register, $step eq '' ? ' no step' : $step,
				$destination), \@setup,
				sprintf('data %04X // movr (rN),abXh; PDF Table 4-4', $opcode));
		}
	}
}

for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF) {
	add_case(sprintf('load page boundary %02X', $value), [ base_setup() ],
		sprintf('data %04X // load #unsigned8,page; PDF Table 4-4', 0x0400 | $value));
}

for my $configuration (0 .. 1) {
	my $name = $configuration ? 'modj' : 'modi';
	for my $value (0x000, 0x001, 0x0FF, 0x100, 0x1FF) {
		add_case(sprintf('load %s boundary %03X', $name, $value), [ base_setup() ],
			sprintf('data %04X // load #unsigned9,%s; PDF Table 4-4',
				0x0200 | $configuration << 11 | $value, $name));
	}
}

for my $configuration (0 .. 1) {
	my $name = $configuration ? 'stepj' : 'stepi';
	for my $value (0x00, 0x01, 0x3F, 0x40, 0x7F) {
		add_case(sprintf('load %s boundary %02X', $name, $value), [ base_setup() ],
			sprintf('data %04X // load #immediate7,%s; PDF Table 4-4',
				0xDB80 | $configuration << 10 | $value, $name));
	}
}

my @bank_registers = qw(cfgi r4 r1 r0);
for my $mask (0 .. 15) {
	my @setup = four_accumulator_setup();
	push @setup,
		'data 4B8F // banke r0,r1,r4,cfgi; select alternative bank; PDF Table 4-4',
		'mov 0x$A001 r0',
		'mov 0x$B112 r1',
		'mov 0x$C445 r4',
		'mov 0x$0555 cfgi',
		'data 4B8F // banke r0,r1,r4,cfgi; restore primary bank; PDF Table 4-4';
	my @selected = map { $bank_registers[$_] } grep { $mask & 1 << $_ } 0 .. $#bank_registers;
	add_case('banke ' . (@selected ? join('-', @selected) : 'none'), \@setup,
		sprintf('data %04X // banke %s; PDF Table 4-4', 0x4B80 | $mask, join(',', @selected)));
}

my @ab_names = qw(b0 b1 a0 a1);
for my $source (0 .. $#ab_names) {
	for my $destination (0 .. $#ab_names) {
		add_case("mov $ab_names[$source]l dvm to $ab_names[$destination]", [ four_accumulator_setup() ], [
			sprintf('data %04X // mov %sl,dvm; PDF Table 4-4', 0xD298 | $source << 10, $ab_names[$source]),
			sprintf('data %04X // mov dvm,%s; PDF Table 4-4', 0xD491 | $destination << 5,
				$ab_names[$destination]),
		]);
	}
}

for my $value (0, 1, 7, 15, 16, 31) {
	add_case(sprintf('mov immediate icr value %02X', $value), [ base_setup() ], [
		sprintf('data %04X // mov #%u,icr; PDF Table 4-4', 0x4F80 | $value, $value),
		'data D4D2 // mov icr,a0; PDF Table 4-4',
	]);
}

for my $source (0 .. 3) {
	my @setup = base_setup();
	push @setup, sprintf('mov 0x$%04X r%u', 1 << $source, $source);
	add_case("mov r$source icr", \@setup, [
		sprintf('data %04X // mov r%u,icr; PDF Table 4-4', 0x4FC0 | $source, $source),
		'data D4D2 // mov icr,a0; PDF Table 4-4',
	]);
}

for my $destination (0 .. $#ab_names) {
	add_case("mov icr to $ab_names[$destination]", [ base_setup() ], [
		'data 4F8B // mov #11,icr; PDF Table 4-4',
		sprintf('data %04X // mov icr,%s; PDF Table 4-4', 0xD492 | $destination << 5,
			$ab_names[$destination]),
	]);
}

{
	my @setup = four_accumulator_setup();
	push @setup, 'mov 0x$0551 st0', 'mov 0x$0300 st1', 'mov 0x$005A st2';
	add_case('cntx store shadows and swaps accumulators', \@setup,
		'data D380 // cntx s; PDF Table 4-4');
}

{
	my @setup = four_accumulator_setup();
	push @setup, 'mov 0x$0551 st0', 'mov 0x$0300 st1', 'mov 0x$005A st2';
	my @body = (
		'data D380 // cntx s; PDF Table 4-4',
		'mov 0x$0121 st0',
		'mov 0x$0400 st1',
		'mov 0x$00A5 st2',
		accumulator_value_setup('a1', 0x3131, 0x3030, 3),
		accumulator_value_setup('a0', 0x4141, 0x4040, 4),
		'mov a0 b1',
		'data D390 // cntx r; PDF Table 4-4',
	);
	add_case('cntx restore shadows and swaps accumulators', \@setup, \@body);
}

add_case('eint sets ie', [ accumulator_setup(0x1357, 0x2468, 0x0000) ],
	'data 4380 // eint; PDF Table 4-4');
add_case('dint clears ie', [ accumulator_setup(0x1357, 0x2468, 0x0002) ],
	'data 43C0 // dint; PDF Table 4-4');
add_case('eint then dint clears ie', [ accumulator_setup(0x1357, 0x2468, 0x0000) ],
	[ 'data 4380 // eint; PDF Table 4-4', 'data 43C0 // dint; PDF Table 4-4' ]);
add_case('dint then eint sets ie', [ accumulator_setup(0x1357, 0x2468, 0x0002) ],
	[ 'data 43C0 // dint; PDF Table 4-4', 'data 4380 // eint; PDF Table 4-4' ]);

add_case('break exits current block repeat', [ accumulator_setup(0x0000, 0x0000, 0x0000) ], [
	'@bkrep 0x0007u8 4',
	'inc a0 always',
	'data D3C0 // break; PDF Table 4-4',
	'inc a0 always',
]);

add_case('reti false falls through', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], [
	'data 45C1 // reti eq; false condition; PDF Table 4-4',
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);
add_case('reti returns and enables interrupts', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], [
	sprintf('call 0x0000$%04X always', $control_helper_address + 0x50),
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);
add_case('retid executes delayed instructions and returns', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], [
	sprintf('call 0x0000$%04X always', $control_helper_address + 0x40),
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);

{
	my @setup = four_accumulator_setup();
	push @setup,
		'mov 0x$0551 st0',
		'mov 0x$0300 st1',
		'mov 0x$005A st2',
		'data D380 // cntx s; establish context shadows before context-switching RETI',
		accumulator_value_setup('a1', 0x3131, 0x3030, 3),
		accumulator_value_setup('a0', 0x4141, 0x4040, 4),
		'mov a0 b1';
	add_case('reti context switches and returns', \@setup, [
		sprintf('call 0x0000$%04X always', $control_helper_address + 0x60),
		'mov 0x0003u8 a0l',
		'mov a0l [0x$D682]',
	]);
}

add_case('reti context false falls through', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], [
	'data 45D1 // reti with context switch,eq; false condition; PDF Table 4-4',
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);

my @movp_destinations = (
	[ r0 => 0, 0 ], [ r1 => 1, 0 ], [ st0 => 8, 1 ], [ st1 => 9, 2 ], [ st2 => 10, 3 ],
	[ cfgi => 14, 4 ], [ cfgj => 15, 4 ], [ a0 => 24, 0 ], [ a1 => 25, 0 ], [ a0l => 26, 0 ],
	[ a1l => 27, 0 ], [ a0h => 28, 0 ], [ a1h => 29, 0 ], [ lc => 30, 0 ], [ sv => 31, 0 ],
);
for my $source (0 .. 1) {
	for my $destination (@movp_destinations) {
		my ($name, $register, $fixture) = @$destination;
		my @setup = four_accumulator_setup();
		push @setup, sprintf('mov 0x$%04X a%ul', $program_fixture_address + $fixture, $source);
		add_case("movp a$source to $name", \@setup,
			sprintf('data %04X // movp (a%ul),%s; PDF Table 4-4',
				0x0040 | $source << 5 | $register, $source, $name));
	}
}

for my $source (4, 5) {
	for my $destination (0 .. 3) {
		for my $source_step (0 .. 3) {
			for my $destination_step (0 .. 3) {
				my @setup = base_setup();
				push @setup,
					sprintf('mov 0x$%04X r%u', $program_fixture_address, $source),
					sprintf('mov 0x$D680 r%u', $destination),
					'load +0x0003 stepi',
					'load +0x0005 stepj';
				my $opcode = 0x0600 | $source | $source_step << 3 | $destination << 5 |
					$destination_step << 7;
				add_case(sprintf('movp r%u step%u to r%u step%u', $source, $source_step, $destination,
					$destination_step), \@setup,
					sprintf('data %04X // movp (r%u),(r%u); PDF Table 4-4', $opcode, $source, $destination));
			}
		}
	}
}

for my $source (0 .. 3) {
	for my $destination (4, 5) {
		for my $source_step (0 .. 3) {
			for my $destination_step (0 .. 3) {
				my @setup = base_setup();
				push @setup,
					'mov 0x$BEEF a0l',
					'mov a0l [0x$D690]',
					sprintf('mov 0x$D690 r%u', $source),
					sprintf('mov 0x$%04X r%u', $program_fixture_address + 8, $destination),
					'load +0x0003 stepi',
					'load +0x0005 stepj';
				my $opcode = 0x5F80 | $source | $source_step << 3 | ($destination - 4) << 2 |
					$destination_step << 5;
				add_case(sprintf('movd r%u step%u to r%u step%u', $source, $source_step, $destination,
					$destination_step), \@setup, [
					sprintf('data %04X // movd (r%u),(r%u); PDF Table 4-4', $opcode, $source, $destination),
					sprintf('mov 0x$%04X a0l', $program_fixture_address + 8),
					'data 0059 // movp (a0l),a1; verify program write; PDF Table 4-4',
				]);
			}
		}
	}
}

my @external_register_opcodes = (0x2900, 0x2D00, 0x3900, 0x3D00);
for my $external_register (0 .. $#external_register_opcodes) {
	for my $value (0x00, 0x01, 0x7F, 0x80, 0xFF, 0x55, 0xAA) {
		add_case(sprintf('mov short immediate ext%u value %02X', $external_register, $value),
			[ base_setup() ], [
				sprintf('data %04X // mov #short,ext%u; PDF Table 4-4',
					$external_register_opcodes[$external_register] | $value, $external_register),
				'nop',
				'nop',
				sprintf('data %04X // mov ext%u,r0; independently observe external-register readback',
					0x5800 | 20 + $external_register, $external_register),
			]);
	}
}

add_case('trap enters vector and reti returns', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], [
	'data 0020 // trap; PDF Table 4-4',
	'mov 0x0003u8 a0l',
	'mov a0l [0x$D682]',
]);

sub alias_memory_setup {
	my @setup = four_accumulator_setup();
	push @setup,
		'load 0x00d6u8 page',
		'mov 0x$8001 r0',
		'mov r0 [page:0x0000u8]',
		'nop',
		'nop',
		'mov 0x$D600 r0',
		'load +0x0003 stepi';
	return @setup;
}

sub add_alias_family {
	my ($name, $pattern, $setup, $before, $after) = @_;
	die "$name: invalid opcode pattern '$pattern'\n" unless $pattern =~ /^[01.]{16}$/;
	my ($base, $mask) = (0, 0);
	for my $bit (split //, $pattern) {
		$base <<= 1;
		$mask <<= 1;
		$base |= 1 if $bit eq '1';
		$mask |= 1 if $bit eq '.';
	}
	die "$name: opcode pattern has no don't-care bits\n" unless $mask;

	my @variants;
	for (my $bits = $mask; $bits; $bits = ($bits - 1) & $mask) {
		push @variants, $bits;
	}
	for my $bits (sort { $a <=> $b } @variants) {
		my @body = (
			@$before,
			sprintf('data %04X // %s noncanonical alias bits %04X; PDF Table 4-4', $base | $bits, $name, $bits),
			@$after,
		);
		add_case(sprintf('%s alias bits %04X mask %04X', $name, $bits, $mask), $setup, \@body);
	}
}

sub select_alias_cases {
	@cases = ();
	my @base = four_accumulator_setup();
	my @memory = alias_memory_setup();
	my @false_condition = accumulator_setup(0x1357, 0x2468, 0x0000);

	add_alias_family('ALU long immediate add a0', '10000110110.....', [ accumulator_setup(0, 0, 0) ], [],
		[ 'data 0001 // long immediate expansion' ]);
	add_alias_family('norm a0 indirect r0', '1001010011.00000', [ @base, 'mov 0x$D600 r0' ], [], []);
	add_alias_family('maxd ge a0 indirect r0', '1000000001100...', \@memory, [], []);
	add_alias_family('max ge a0 indirect r0', '1000010001100...', \@memory, [], []);
	add_alias_family('min le a0 indirect r0', '10001.0001100...', \@memory, [], []);
	add_alias_family('lim a0', '010010011100....', \@base, [], []);
	add_alias_family('msu a0 indirect r0 long immediate', '1001000011.00000', \@memory, [],
		[ 'data 0001 // long immediate expansion' ]);
	add_alias_family('modb clear b0 always', '01101111.1100000', \@base, [], []);
	add_alias_family('exp indirect r0 a0', '1001100001.00000', \@memory, [], []);
	add_alias_family('exp b0 a0', '10010000011....0', \@base, [], []);
	add_alias_family('exp indirect r0 sv', '1001110.01.00000', \@memory, [], []);
	add_alias_family('exp register r0 sv', '1001010.01000000', \@base, [], []);
	add_alias_family('exp b0 sv', '1001010.011....0', \@base, [], []);
	add_alias_family('mov b0 a0', '1101001011010...', \@base, [], []);
	add_alias_family('mov a0l dvm', '1101101010.11...', \@base, [], [ 'data D4B1 // mov dvm,a1' ]);
	add_alias_family('mov a0l x', '1101101011.11...', \@base, [], []);
	add_alias_family('mov r0 mixp', '0101111010.00000', \@base, [], []);
	add_alias_family('mov repc a0', '1101010.11010.00', \@base, [], []);
	add_alias_family('mov dvm a0', '1101010.11010.01', [ @base, 'data D298 // mov a0l,dvm' ], [], []);
	add_alias_family('mov icr a0', '1101010.11010.10', [ @base, 'data 4F80 // mov #0,icr' ], [], []);
	add_alias_family('mov x a0', '1101010.11010.11', \@base, [], []);
	add_alias_family('mov indirect r0 b0', '1001100011.00000', \@memory, [], []);
	add_alias_family('mov long direct a0', '11010100101110..', \@memory, [],
		[ 'data D600 // long direct address expansion' ]);
	add_alias_family('mov a0l long direct', '11010100101111..', \@memory, [], [
		'data D600 // long direct address expansion',
		'nop',
		'nop',
		'mov [0x$D600] a1',
	]);
	add_alias_family('mov long immediate r0', '0101111.00000000', \@base, [],
		[ 'data 8001 // long immediate expansion' ]);
	add_alias_family('mov long immediate b0', '01011110001.....', \@base, [],
		[ 'data 8001 // long immediate expansion' ]);
	add_alias_family('mov r0 icr', '0100111111.00000', [ @base, 'mov 0x$0001 r0' ], [],
		[ 'data D4D2 // mov icr,a0' ]);
	add_alias_family('mov immediate icr', '0100111110.00000', \@base, [], [ 'data D4D2 // mov icr,a0' ]);
	add_alias_family('mov rb long offset a0', '11010100100110..', [ @memory, 'mov 0x$D600 r7' ], [],
		[ 'data 0000 // rb long offset expansion' ]);
	add_alias_family('mov a0l rb long offset', '11010100100111..', [ @memory, 'mov 0x$D600 r7' ], [], [
		'data 0000 // rb long offset expansion',
		'nop',
		'nop',
		'mov [0x$D600] a1',
	]);
	add_alias_family('push long immediate', '0101111101......', \@base, [], [
		'data 8001 // long immediate expansion',
		'pop r0',
	]);
	add_alias_family('swap option 0', '0100100110..0000', \@base, [], []);
	add_alias_family('banke mask 0', '010010111...0000', \@base, [], []);
	add_alias_family('rep register r0', '00001101...00000', [ accumulator_setup(0, 0, 0), 'mov 0x$0000 r0' ], [],
		[ 'inc a0 always' ]);
	add_alias_family('bkrep register r0', '01011101...00000', [ accumulator_setup(0, 0, 0), 'mov 0x$0000 r0' ], [],
		[ '@wordaddr 2', 'inc a0 always' ]);
	add_alias_family('break in block repeat', '1101001111......', [ accumulator_setup(0, 0, 0) ],
		[ '@bkrep 0x0007u8 4', 'inc a0 always' ], [ 'inc a0 always' ]);
	add_alias_family('br eq false', '0100000110..0001', \@false_condition, [],
		[ 'data 1000 // absolute branch address expansion' ]);
	add_alias_family('call eq false', '0100000111..0001', \@false_condition, [],
		[ 'data 1000 // absolute call address expansion' ]);
	add_alias_family('calla a0', '110101001..0....', [ @base, 'mov 0x$1000 a0l' ], [], []);
	add_alias_family('ret eq false', '0100010110..0001', \@false_condition, [], []);
	add_alias_family('reti eq false', '0100010111.00001', \@false_condition, [], []);
	add_alias_family('retid', '1101011111.0....', \@base,
		[ '@callabs always 4', '@brabs always 4' ], [ 'mov 0x0002u8 a0l' ]);
	add_alias_family('cntx store', '1101001110.0....', \@base, [], []);
	add_alias_family('nop', '00000000000.....', \@base, [], []);
	add_alias_family('modr r0 no modification', '000000001.000000', [ @base, 'mov 0x$D600 r0' ], [], []);
	add_alias_family('eint', '0100001110......', [ accumulator_setup(0x1357, 0x2468, 0x0000) ], [], []);
	add_alias_family('dint', '0100001111......', [ accumulator_setup(0x1357, 0x2468, 0x0002) ], [], []);
	add_alias_family('trap', '00000000001.....', \@base, [], [
		'mov 0x0003u8 a0l',
		'mov a0l [0x$D682]',
	]);
	add_alias_family('load ps 0', '010011011.....00', \@base, [], []);
}

sub set_output_directory {
	my ($directory) = @_;
	make_path($directory);
	$script_dir = $directory;
	$cases_path = File::Spec->catfile($script_dir, 'instructions-cases.inc');
	$generated_dir = File::Spec->catdir($script_dir, 'generated');
	make_path($generated_dir);
	$images_path = File::Spec->catfile($generated_dir, 'instructions-images.inc');
}

sub read_hardware_capture {
	my ($path) = @_;
	my @capture;
	my $passed = 0;

	open my $log, '<', $path or die "open($path): $!";
	while (my $line = <$log>) {
		$passed = 1 if $line =~ /^# result: PASS /;
		next unless $line =~ /^# DSPCASE (\d+) "([^"]+)"((?: [0-9A-F]{4})+)\s*$/;
		my ($index, $name, $words_text) = ($1, $2, $3);
		die "$path: duplicate or out-of-order case $index\n" if $index != @capture;
		die "$path: case $index is outside the generated corpus\n" if $index >= @cases;
		die "$path: case $index name '$name' does not match '$cases[$index]->{name}'\n"
			if $name ne $cases[$index]->{name};
		my @words = map { hex($_) } split ' ', $words_text;
		die "$path: case $index has " . scalar(@words) . " words, expected $record_words\n"
			if @words != $record_words;
		push @capture, \@words;
	}
	close $log or die "close($path): $!";

	die "$path: hardware test did not pass\n" unless $passed;
	die "$path: captured " . scalar(@capture) . ' cases, expected ' . scalar(@cases) . "\n"
		if @capture != @cases;
	return \@capture;
}

sub import_hardware_golden {
	my ($directory, $first_path, $second_path) = @_;
	my $first = read_hardware_capture($first_path);
	my $second = read_hardware_capture($second_path);

	for my $index (0 .. $#cases) {
		for my $word (0 .. $record_words - 1) {
			die "hardware captures differ at case $index word $word\n"
				if $first->[$index][$word] != $second->[$index][$word];
		}
	}

	my $golden_path = File::Spec->catfile($directory, 'instructions-golden.inc');
	open my $golden, '>', $golden_path or die "open($golden_path): $!";
	print {$golden} "// Generated by gen_instructions.pl from two matching EL71 hardware captures.\n";
	print {$golden} '#define DSP_INSTRUCTION_GOLDEN_COUNT ', scalar(@cases), "\n";
	print {$golden} "static const uint16_t DSP_INSTRUCTION_GOLDEN[][DSP_INSTRUCTION_RECORD_WORDS] = {\n";
	for my $record (@$first) {
		print {$golden} "\t{ ", join(', ', map { sprintf('0x%04X', $_) } @$record), " },\n";
	}
	print {$golden} "};\n";
	close $golden or die "close($golden_path): $!";
	system $^X, $packer_path, '--golden', $directory;
	die "$packer_path failed with status $?\n" if $? != 0;
	printf "Imported %u stable hardware cases\n", scalar @cases;
}

sub clear_hardware_golden {
	my ($directory) = @_;
	my $golden_path = File::Spec->catfile($directory, 'instructions-golden.inc');
	open my $golden, '>', $golden_path or die "open($golden_path): $!";
	print {$golden} "// Generated by gen_instructions.pl for EL71 hardware capture.\n";
	print {$golden} "#define DSP_INSTRUCTION_GOLDEN_COUNT 0\n";
	print {$golden} "static const uint16_t DSP_INSTRUCTION_GOLDEN[1][DSP_INSTRUCTION_RECORD_WORDS];\n";
	close $golden or die "close($golden_path): $!";
	system $^X, $packer_path, '--golden', $directory;
	die "$packer_path failed with status $?\n" if $? != 0;
	print "Cleared hardware golden for capture\n";
}

my $alias_dir = File::Spec->catdir(dirname($script_dir), 'opcode-aliases');
if (@ARGV == 1 && $ARGV[0] eq '--generate-aliases') {
	select_alias_cases();
	set_output_directory($alias_dir);
} elsif (@ARGV == 1 && $ARGV[0] eq '--clear-golden') {
	clear_hardware_golden($script_dir);
	exit 0;
} elsif (@ARGV == 1 && $ARGV[0] eq '--clear-alias-golden') {
	select_alias_cases();
	make_path($alias_dir);
	clear_hardware_golden($alias_dir);
	exit 0;
} elsif (@ARGV == 3 && $ARGV[0] eq '--import-hardware') {
	import_hardware_golden($script_dir, $ARGV[1], $ARGV[2]);
	exit 0;
} elsif (@ARGV == 3 && $ARGV[0] eq '--import-alias-hardware') {
	select_alias_cases();
	make_path($alias_dir);
	import_hardware_golden($alias_dir, $ARGV[1], $ARGV[2]);
	exit 0;
} elsif (@ARGV) {
	die "usage: $0 [--generate-aliases | --clear-golden | --clear-alias-golden | " .
		"--import-hardware CAPTURE1 CAPTURE2 | --import-alias-hardware CAPTURE1 CAPTURE2]\n";
}
make_path($generated_dir);

my $result_address = 0xD900;
my $segment_limit = 480;
my $cases_per_shard = 16;

sub line_words {
	my ($line) = @_;
	return 2 if $line =~ /^\@(brabs|callabs|bkrep) /;
	return index($line, '$') >= 0 ? 2 : 1;
}

sub render_symbolic_line {
	my ($line, $address) = @_;
	if ($line =~ /^\@(brabs|callabs) (\S+) (\d+)$/) {
		my $instruction = $1 eq 'brabs' ? 'br' : 'call';
		my $condition = $2 eq 'nn' ? 'mn' : $2;
		return sprintf('%s 0x0000$%04X %s', $instruction, $address + $3, $condition);
	}
	if ($line =~ /^\@wordaddr (\d+)$/) {
		return sprintf('data %04X // generated program address expansion', $address - 1 + $1);
	}
	if ($line =~ /^\@bkrep (\S+) (\d+)$/) {
		return sprintf('bkrep %s 0x0000$%04X', $1, $address + $2);
	}
	return $line;
}

sub capture_lines {
	my ($record) = @_;
	# Save status before load page changes the low byte of st1. Restore the
	# temporary stack frame before capturing the remaining state.
	my @lines = (
		'push r0',
		'mov st0 r0',
		'push r0',
		'mov st1 r0',
		'push r0',
		'mov st2 r0',
		'push r0',
	);
	my $page = -1;
	my $store = sub {
		my ($source, $slot) = @_;
		my $address = $record + $slot;
		my $address_page = $address >> 8;
		if ($address_page != $page) {
			push @lines, sprintf('load 0x%04xu8 page', $address_page);
			$page = $address_page;
		}
		push @lines, sprintf('mov %s [page:0x%04xu8]', $source, $address & 0xFF);
	};

	$store->('a0l', 3);
	for my $slot (2, 1, 0, 11) {
		push @lines, 'pop r0';
		$store->('r0', $slot);
	}

	$store->('a0h', 4);
	$store->('a1l', 5);
	$store->('a1h', 6);
	for my $register (1 .. 5) {
		$store->("r$register", 11 + $register);
	}
	$store->('r7', 17);
	$store->('y0', 18);
	$store->('sv', 19);

	for my $state ([ 'sp', 20 ], [ 'cfgi', 21 ], [ 'cfgj', 22 ], [ 'lc', 23 ]) {
		my ($register, $slot) = @$state;
		push @lines, "mov $register r0";
		$store->('r0', $slot);
	}

	push @lines, 'data 47C0 // mov mixp,r0; PDF Table 4-4';
	$store->('r0', 24);
	push @lines, 'mov repc a0';
	$store->('a0l', 25);
	push @lines, 'mov x0 a1';
	$store->('a1l', 9);
	$store->('a1h', 10);
	push @lines, 'data 5B2B // mov p,a1; makedsp1 cannot parse TeakLite I p';
	$store->('a1l', 7);
	$store->('a1h', 8);
	for my $accumulator ([ 'b0', 29 ], [ 'b1', 32 ]) {
		my ($source, $slot) = @$accumulator;
		push @lines, "mov $source a0";
		$store->('a0l', $slot);
		$store->('a0h', $slot + 1);
		push @lines, 'mov st0 r0';
		$store->('r0', $slot + 2);
	}

	for my $canary (0 .. 2) {
		push @lines, sprintf('mov [0x$D68%X] a0', $canary);
		$store->('a0l', 26 + $canary);
	}
	return @lines;
}

sub case_words {
	my ($case, $record) = @_;
	my $words = 0;
	$words += line_words($_) for canary_setup();
	$words += line_words($_) for @{$case->{setup}};
	$words += line_words($_) for @{$case->{body}};
	$words += line_words($_) for capture_lines($record);
	return $words;
}

sub write_assembly {
	my ($shard, $first, $count) = @_;
	my $asm_path = File::Spec->catfile($generated_dir, "instructions-$shard.asm");
	my $program_address = 0x0100;
	my $segment_words = 4;
	my $segment_count = 2;

	open my $asm, '>', $asm_path or die "open($asm_path): $!";
	print {$asm} "// Generated by gen_instructions.pl.\n";
	print {$asm} "segment p 0002\n";
	print {$asm} "data D4D1 // mov dvm,a0; TRAP vector handler; PDF Table 4-4\n";
	print {$asm} "mov a0l [0x\$D681]\n";
	print {$asm} "data 45C0 // reti always; TRAP vector handler; PDF Table 4-4\n";
	printf {$asm} "\nsegment p %04X\n", $program_address;
	print {$asm} "mov 0x\$0001 a0l\n";
	print {$asm} "mov a0l [0x\$DE92]\n";

	for my $local_index (0 .. $count - 1) {
		my $index = $first + $local_index;
		my $case = $cases[$index];
		my $record = $result_address + $local_index * $record_words;
		my $case_words = case_words($case, $record);
		if ($segment_words + $case_words > $segment_limit) {
			$program_address += $segment_words;
			printf {$asm} "\nsegment p %04X\n", $program_address;
			$segment_words = 0;
			$segment_count++;
		}

		printf {$asm} "\n// Case %u: %s\n", $index, $case->{name};
		for my $line (canary_setup()) {
			print {$asm} "$line\n";
			$segment_words += line_words($line);
		}
		for my $line (@{$case->{setup}}) {
			print {$asm} "$line\n";
			$segment_words += line_words($line);
		}
		for my $line (@{$case->{body}}) {
			my $rendered = render_symbolic_line($line, $program_address + $segment_words);
			print {$asm} "$rendered\n";
			$segment_words += line_words($line);
		}

		for my $line (capture_lines($record)) {
			print {$asm} "$line\n";
			$segment_words += line_words($line);
		}
	}

	if ($segment_words + 7 > $segment_limit) {
		$program_address += $segment_words;
		printf {$asm} "\nsegment p %04X\n", $program_address;
		$segment_words = 0;
		$segment_count++;
	}
	print {$asm} "\n// Completion marker.\n";
	print {$asm} "mov 0x\$A55A a0l\n";
	print {$asm} "mov a0l [0x\$D300]\n";
	my $branch_address = $program_address + $segment_words + 4;
	printf {$asm} "br 0x0000\$%04X always\n", $branch_address;
	die sprintf("main program overlaps helpers at %04X\n", $control_helper_address)
		if $branch_address + 2 > $control_helper_address;
	die "DSP1 shard needs more than ten segments\n" if $segment_count + 1 > 10;

	my @helper_blocks = (
		[ $control_helper_address, 'mov 0x0002u8 a0l', 'mov a0l [0x$D681]', 'ret always' ],
		[ $control_helper_address + 0x10, 'mov 0x0002u8 a0l', 'retd', 'mov a0l [0x$D681]' ],
		[ $control_helper_address + 0x20, 'mov 0x0002u8 a0l', 'mov a0l [0x$D681]', 'rets 0x0000u8' ],
		[ $control_helper_address + 0x28, 'mov 0x0002u8 a0l', 'mov a0l [0x$D681]', 'rets 0x0001u8' ],
		[ $control_helper_address + 0x30, 'mov 0x0002u8 a0l', 'mov a0l [0x$D681]', 'rets 0x0003u8' ],
		[ $control_helper_address + 0x38, 'mov 0x0002u8 a0l', 'mov a0l [0x$D681]', 'rets 0x00ffu8' ],
		[ $control_helper_address + 0x40, 'data D7C0 // retid; PDF Table 4-4', 'mov 0x0002u8 a0l',
			'mov a0l [0x$D681]' ],
		[ $control_helper_address + 0x50, 'data 45C0 // reti always; PDF Table 4-4' ],
		[ $control_helper_address + 0x60,
			'data 45D0 // reti with context switch,always; PDF Table 4-4' ],
		[ $program_fixture_address,
			'data 4380 // general movp fixture; also a valid eint opcode',
			'data 0000 // safe st0 movp fixture; also a valid nop opcode',
			'data 0000 // safe st1 movp fixture; also a valid nop opcode',
			'data 0000 // safe st2 movp fixture; also a valid nop opcode',
			'data 0000 // safe configuration movp fixture',
			'data 43C0 // extra movp fixture; also a valid dint opcode',
			'data 4B8F // extra movp fixture; also a valid banke opcode',
			'data D380 // extra movp fixture; also a valid cntx opcode',
			'data 0000 // movd destination fixture' ],
	);
	my $helper_address = $control_helper_address;
	printf {$asm} "\n// Fixed control-flow helpers.\nsegment p %04X\n", $helper_address;
	for my $block (@helper_blocks) {
		my ($address, @lines) = @$block;
		die sprintf("helper overlap at %04X\n", $address) if $helper_address > $address;
		while ($helper_address < $address) {
			print {$asm} "nop\n";
			$helper_address++;
		}
		for my $line (@lines) {
			print {$asm} "$line\n";
			$helper_address += line_words($line);
		}
	}
	close $asm or die "close($asm_path): $!";
}

my $shard_count = int((scalar @cases + $cases_per_shard - 1) / $cases_per_shard);
for my $shard (0 .. $shard_count - 1) {
	my $first = $shard * $cases_per_shard;
	my $remaining = scalar @cases - $first;
	my $count = $remaining < $cases_per_shard ? $remaining : $cases_per_shard;
	write_assembly($shard, $first, $count);
}

sub print_uint16_array {
	my ($file, $name, @values) = @_;
	print {$file} "static const uint16_t $name\[\] = {\n";
	while (@values) {
		my @line = splice @values, 0, 16;
		print {$file} "\t", join(', ', @line), ",\n";
	}
	print {$file} "};\n";
}

open my $inc, '>', $cases_path or die "open($cases_path): $!";
print {$inc} "// Generated by gen_instructions.pl.\n";
print {$inc} "#define DSP_INSTRUCTION_RECORD_WORDS $record_words\n";
printf {$inc} "#define DSP_INSTRUCTION_RESULT_OFFSET 0x%04X\n", $result_address - 0xD000;
print {$inc} '#define DSP_INSTRUCTION_CASE_COUNT ', scalar(@cases), "\n";
print {$inc} "#define DSP_INSTRUCTION_SHARD_COUNT $shard_count\n";
print_uint16_array($inc, 'DSP_INSTRUCTION_SHARD_FIRST', map { $_ * $cases_per_shard } 0 .. $shard_count - 1);
print_uint16_array($inc, 'DSP_INSTRUCTION_SHARD_CASES', map {
	my $remaining = scalar @cases - $_ * $cases_per_shard;
	$remaining < $cases_per_shard ? $remaining : $cases_per_shard;
} 0 .. $shard_count - 1);
print {$inc} "static const char *const DSP_INSTRUCTION_CASE_NAMES[] = {\n";
for my $case (@cases) {
	my $name = $case->{name};
	$name =~ s/([\\"])/\\$1/g;
	print {$inc} "\t\"$name\",\n";
}
print {$inc} "};\n";
close $inc or die "close($cases_path): $!";

open my $images, '>', $images_path or die "open($images_path): $!";
print {$images} "// Generated by gen_instructions.pl.\n";
for my $shard (0 .. $shard_count - 1) {
	print {$images} "#include \"instructions-image-$shard.inc\"\n";
}
print {$images} "\nstatic const uint8_t *const DSP_INSTRUCTION_IMAGES[] = {\n";
for my $shard (0 .. $shard_count - 1) {
	print {$images} "\tDSP_INSTRUCTIONS_IMAGE_$shard,\n";
}
print {$images} "};\n";
close $images or die "close($images_path): $!";

printf "Generated %u cases\n", scalar @cases;
