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
	
	my $board_name = uc($board_meta->{vendor})."_".uc($board_meta->{model});
	$board_name =~ s/-/_/g;
	
	my $board_id = lc($board_meta->{vendor})."_".lc($board_meta->{model});
	$board_id =~ s/-/_/g;
	
	my $keymap_var = "board_".$board_id."_keymap";
	my $gpio_var = "board_".$board_id."_fixed_gpio";
	my $flashes_var = "board_".$board_id."_flashes";
	
	my @keys;
	for my $kp_name (getSortedKeys($board_meta->{keys}, 'code')) {
		my $kp = $board_meta->{keys}->{$kp_name};
		
		for my $qemu_key (@{$kp->{map}}) {
			push @keys, [
				"[Q_KEY_CODE_".$qemu_key."]",
				"= ".$board_name."_KP_".$kp->{name}
			];
		}
	}
	
	my @gpios;
	for my $gpio_name (getSortedKeys($board_meta->gpios(), 'id')) {
		my $gpio = $board_meta->gpios()->{$gpio_name};
		if ($gpio->{mode} eq "input") {
			push @gpios, [
				$board_name."_GPIO_".($gpio->{alias} || $gpio->{name}).",",
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
	
	my @board = (
		name				=> '"'.$board_meta->{vendor}.' '.$board_meta->{model}.'"',
		width				=> $board_meta->{width},
		height				=> $board_meta->{height},
		display				=> $board_meta->{display} ? '"'.$board_meta->{display}.'"' : 'NULL',
		display_rotation	=> $board_meta->{display_rotation},
		cpu					=> 'CPU_'.uc($board_meta->{cpu}->{name}),
		flash_banks			=> $flashes_var,
		flash_banks_cnt		=> "ARRAY_SIZE($flashes_var)",
		keymap				=> $keymap_var,
		keymap_cnt			=> "ARRAY_SIZE($keymap_var)",
		fixed_gpios			=> $gpio_var,
		fixed_gpios_cnt		=> "ARRAY_SIZE($gpio_var)"
	);
	
	my @board_table;
	for (my $i = 0; $i < scalar(@board); $i += 2) {
		push @board_table, [".".$board[$i], "= ".$board[$i + 1].","];
	}
	
	push @boards, "\t{\n".printTable(\@board_table, "\t\t")."\n\t},";
	
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
