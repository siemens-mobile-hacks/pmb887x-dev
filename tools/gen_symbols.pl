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

my $board = $ENV{BOARD} || "EL71";
my $gen_idc = $ARGV[0] || 0;

my $board_meta = Sie::BoardMetadata->new($board);

if ($gen_idc) {
	print "#include <idc.idc>\nstatic main() {\n";
}

my $cpu_meta = $board_meta->cpu();
for my $id (@{$cpu_meta->getModuleNames()}) {
	my $module = $cpu_meta->{modules}->{$id};
	print genModuleSymbols($cpu_meta, $module);
}

if ($gen_idc) {
	print "}\n";
}

sub genModuleSymbols {
	my ($cpu_meta, $module) = @_;
	
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
			push @$symbols, sprintf("%s_%02X %08X l", $module->{name}, $i, $module->{base} + $i);
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
					$symbols->[$i / 4] = sprintf("%s %08X l", $reg_name, $module->{base} + $i);
				}
				$index++;
			}
		} else {
			if ($gen_idc) {
				$symbols->[$reg->{start} / 4] = sprintf("\tMakeName(0x%08X, \"%s_%s\");", $module->{base} + $reg->{start}, $module->{name}, $reg->{name});
			} else {
				$symbols->[$reg->{start} / 4] = sprintf("%s_%s %08X l", $module->{name}, $reg->{name}, $module->{base} + $reg->{start});
			}
		}
	}
	
	return join("\n", @$symbols)."\n";
}
