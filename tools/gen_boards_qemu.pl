#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

my $str = qq|#include "hw/arm/pmb887x/boards.h"\n\n|;

my @boards;

my $board_idx = 0;

for my $board (@{Sie::BoardMetadata::getBoards()}) {
	my $board_meta = Sie::BoardMetadata->new($board);
	my $cpu_meta = $board_meta->cpu;
	
	my $keymap_var = "board_".lc($board_meta->{name})."_keymap";
	my $gpio_var = "board_".lc($board_meta->{name})."_fixed_gpio";
	my $flashes_var = "board_".lc($board_meta->{name})."_flashes";
	
	my @keys;
	for my $kp_name (getSortedKeys($board_meta->{keys}, 'code')) {
		my $kp = $board_meta->{keys}->{$kp_name};
		
		for my $qemu_key (@{$kp->{map}}) {
			push @keys, [
				"[Q_KEY_CODE_".$qemu_key."]",
				"= ".uc($board_meta->{name})."_KP_".$kp->{name}
			];
		}
	}
	
	my @gpios;
	for my $gpio_name (getSortedKeys($board_meta->gpios(), 'id')) {
		my $gpio = $board_meta->gpios()->{$gpio_name};
		if ($gpio->{mode} eq "input") {
			push @gpios, [
				uc($board_meta->{name})."_GPIO_".($gpio->{alias} || $gpio->{name}).",",
				$gpio->{value}
			];
		}
	}
	
	my @flashes;
	for my $flash (@{$board_meta->{flash}}) {
		push @flashes, [sprintf("0x%08X", $flash)];
	}
	
	$str .= "static uint32_t ".$keymap_var."[] = {\n";
	$str .= printTable(\@keys, "\t", ",");
	$str .= "};\n\n";
	
	$str .= "static pmb887x_fixed_gpio_t ".$gpio_var."[] = {\n";
	$str .= printTable(\@gpios, "\t{", "},");
	$str .= "};\n\n";
	
	$str .= "static uint32_t ".$flashes_var."[] = {\n";
	$str .= printTable(\@flashes, "\t", ",");
	$str .= "};\n\n";
	
	push @boards, [
		'"'.$board_meta->{name}.'",',
		$board_meta->{width}.",",
		$board_meta->{height}.",",
		"CPU_".uc($board_meta->{cpu}->{name}).",",
		"$flashes_var,",
		"ARRAY_SIZE($flashes_var),",
		"$keymap_var,",
		"ARRAY_SIZE($keymap_var),",
		"$gpio_var,",
		"ARRAY_SIZE($gpio_var)",
	];
	
	$board_idx++;
}

$str .= "static pmb887x_board_t boards_list[] = {\n";
$str .= printTable(\@boards, "\t{", "},");
$str .= "};\n";

$str .= qq|
pmb887x_board_t *pmb887x_get_board(int board) {
	return &boards_list[board];
}
|;

print $str."\n";
