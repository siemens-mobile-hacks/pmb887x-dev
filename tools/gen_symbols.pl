#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use JSON::PP;
use File::Slurp;
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

if ($ARGV[0] && $ARGV[0] eq "gen_all") {
	my $out_dir = $ARGV[1];
	my $index_data = [];

	my @boards = glob(dirname(__FILE__).'/../lib/data/board/*.cfg');

	for my $file (@boards) {
		my $name = basename($file);
		$name =~ s/^(siemens|generic)-|\.cfg$//g;
		$name = uc($name);
		my $board = basename($file);
		$board =~ s/\.cfg$//g;

		my $idc_data = "#include <idc.idc>\nstatic main() {\n";
		my $txt_data = "";
		my $board_meta = Sie::BoardMetadata->new($board);
		my $cpu_meta = $board_meta->cpu();

		for my $id (@{$cpu_meta->getModuleNames()}) {
			my $module = $cpu_meta->{modules}->{$id};
			$txt_data .= genModuleSymbols($cpu_meta, $module, 0);
			$idc_data .= genModuleSymbols($cpu_meta, $module, 1);
		}

		push @$index_data, {
			"name"		=> $name,
			"ida"		=> "cpu-$name.idc",
			"ghidra"	=> "cpu-$name.txt",
			"cpu"		=> $cpu_meta->{name},
		};

		$idc_data .= "}\n";

		write_file("$out_dir/cpu-$name.idc", $idc_data);
		write_file("$out_dir/cpu-$name.txt", $txt_data);
	}
	write_file("$out_dir/index.json", JSON::PP->new->encode($index_data));
} else {
	my $board = $ENV{BOARD} || "siemens-el71";
	my $gen_idc = $ARGV[0] || 0;

	my $board_meta = Sie::BoardMetadata->new($board);

	if ($gen_idc) {
		print "#include <idc.idc>\nstatic main() {\n";
	}

	my $cpu_meta = $board_meta->cpu();
	for my $id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$id};
		print genModuleSymbols($cpu_meta, $module, $gen_idc);
	}

	if ($gen_idc) {
		print "}\n";
	}
}

sub genModuleSymbols {
	my ($cpu_meta, $module, $gen_idc) = @_;
	
	my $alt_names = {};
	if ($module->{name} eq "GPIO") {
		for my $gpio_name (keys %{$cpu_meta->gpios()}) {
			my $gpio = $cpu_meta->gpios()->{$gpio_name};
			my $gpio_addr = $module->{regs}->{PIN}->{start} + ($gpio->{id} * $module->{regs}->{PIN}->{step});
			$alt_names->{$gpio_addr} = "GPIO_PIN".$gpio->{id}."_".($gpio->{alias} ? $gpio->{name}."_".$gpio->{alias} : $gpio->{name});
		}
	}
	
	my $symbols = [];
	for (my $i = 0; $i < $module->{size}; $i += 4) {
		if ($gen_idc) {
			push @$symbols, sprintf("\tMakeName(0x%08X, \"%s_%02X\");", $module->{base} + $i, $module->{name}, $i);
		} else {
			# push @$symbols, sprintf("%s_%02X %08X l", $module->{name}, $i, $module->{base} + $i);
			push @$symbols, sprintf("D\t%08X\t%s_%02X\tunsigned int", $module->{base} + $i, $module->{name}, $i);
		}
	}
	
	for my $reg_name (getSortedKeys($module->{regs}, 'start')) {
		my $reg = $module->{regs}->{$reg_name};
		
		if ($reg->{start} != $reg->{end}) {
			my $index = 0;
			for (my $i = $reg->{start}; $i <= $reg->{end}; $i += $reg->{step}) {
				my $reg_name = sprintf("%s_%s%d", $module->{name}, $reg->{name}, $index);
				$reg_name = $alt_names->{$i} if (exists $alt_names->{$i});
				if ($gen_idc) {
					$symbols->[$i / 4] = sprintf("\tMakeName(0x%08X, \"%s\");", $module->{base} + $i, $reg_name);
				} else {
					# $symbols->[$i / 4] = sprintf("%s %08X l", $reg_name, $module->{base} + $i);
					$symbols->[$i / 4] = sprintf("D\t%08X\t%s\tunsigned int", $module->{base} + $i, $reg_name);
				}
				$index++;
			}
		} else {
			if ($gen_idc) {
				$symbols->[$reg->{start} / 4] = sprintf("\tMakeName(0x%08X, \"%s_%s\");", $module->{base} + $reg->{start}, $module->{name}, $reg->{name});
			} else {
				# $symbols->[$reg->{start} / 4] = sprintf("%s_%s %08X l", $module->{name}, $reg->{name}, $module->{base} + $reg->{start});
				$symbols->[$reg->{start} / 4] = sprintf("D\t%08X\t%s_%s\tunsigned int", $module->{base} + $reg->{start}, $module->{name}, $reg->{name});
			}
		}
	}
	
	return join("\n", @$symbols)."\n";
}
