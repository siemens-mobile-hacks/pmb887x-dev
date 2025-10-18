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
use JSON::XS;

my $raw = $ARGV[0];

my $json = {
	cpus	=> {},
	boards	=> {}
};

if ($raw) {
	for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
		my $cpu_meta = Sie::CpuMetadata->new($cpu);
		
		my $irqs = {};
		for my $id (@{$cpu_meta->getModuleNames()}) {
			my $module = $cpu_meta->{modules}->{$id};
			for my $irq_name (keys %{$module->{irqs}}) {
				$irqs->{$module->{name}.($irq_name ? "_".$irq_name : "")."_IRQ"} = int($module->{irqs}->{$irq_name});
			}
		}
		
		$json->{cpus}->{$cpu} = {
			modules	=> $cpu_meta->{modules},
			gpios	=> $cpu_meta->{gpios},
			irqs	=> $irqs
		};
	}
	
	for my $board (@{Sie::BoardMetadata::getBoards()}) {
		my $board_meta = Sie::BoardMetadata->new($board);
		
		$json->{boards}->{$board} = {
			cpu		=> $board_meta->cpu()->{name},
			memory	=> $board_meta->{memory},
			keys	=> $board_meta->{keys},
			vendor	=> $board_meta->{vendor},
			model	=> $board_meta->{model},
			display	=> $board_meta->{display},
			gpios	=> $board_meta->{gpios},
		};
	}
} else {
	for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
		my $cpu_meta = Sie::CpuMetadata->new($cpu);
		
		$cpu_meta->buildRegIndex();
		
		my $regs = $cpu_meta->{addr2reg};
		
		for my $reg_addr (keys %$regs) {
			my $reg = $regs->{$reg_addr};
			if ($reg->{name} eq "VIC_IRQ_CURRENT" || $reg->{name} eq "VIC_FIQ_CURRENT") {
				$reg->{fields}->{NUM} = {
					start => 0,
					mask => 0xFF,
					value2name => $cpu_meta->{id2irq}
				};
			}
			
			for my $field_name (keys %{$reg->{fields}}) {
				delete $reg->{fields}->{$field_name}->{descr};
				
				if ($reg->{fields}->{$field_name}->{value2name}) {
					my @values;
					for my $val_name (keys %{$reg->{fields}->{$field_name}->{values}}) {
						my $val = $reg->{fields}->{$field_name}->{values}->{$val_name};
						push @values, {
							value	=> $val,
							name	=> $val_name
						};
					}
					
					$reg->{fields}->{$field_name}->{values} = \@values;
				} else {
					$reg->{fields}->{$field_name}->{values} = [];
				}
				
				delete $reg->{fields}->{$field_name}->{value2name};
			}
		}
		
		my @ranges;
		for my $id (@{$cpu_meta->getModuleNames()}) {
			my $module = $cpu_meta->{modules}->{$id};
			push @ranges, {
				name	=> $module->{name},
				start	=> $module->{base},
				size	=> $module->{size}
			};
		}
		
		my @regs;
		for my $addr (keys %{$cpu_meta->{addr2reg}}) {
			my $reg = $cpu_meta->{addr2reg}->{$addr};
			$reg->{addr} = int($addr);
			push @regs, $reg;
		}
		
		my @irqs;
		for my $irqn (keys %{$cpu_meta->{id2irq}}) {
			my $name = $cpu_meta->{id2irq}->{$irqn};
			push @irqs, {id => int($irqn), name => $name};
		}
		
		$json->{cpus}->{$cpu} = {
			regs 	=> \@regs,
			irqs 	=> \@irqs,
			ranges	=> \@ranges
		};
	}

	for my $board (@{Sie::BoardMetadata::getBoards()}) {
		my $board_meta = Sie::BoardMetadata->new($board);
		my $gpio_module = $board_meta->cpu()->{modules}->{GPIO};
		
		my @gpios;
		for my $gpio_name (keys %{$board_meta->gpios()}) {
			my $gpio = $board_meta->gpios()->{$gpio_name};
			
			push @gpios, {
				name		=> ($gpio->{alias} ? $gpio->{name}."_".$gpio->{alias} : $gpio->{name}),
				orig_name	=> $gpio->{name},
				alias		=> $gpio->{alias},
				id			=> int($gpio->{id}),
				addr		=> $gpio_module->{base} + $gpio_module->{regs}->{PIN}->{start} + $gpio_module->{regs}->{PIN}->{step} * $gpio->{id}
			};
		}
		
		$json->{boards}->{$board} = {
			cpu		=> $board_meta->cpu()->{name},
			gpios	=> \@gpios
		};
	}
}

print JSON::XS->new->canonical(1)->pretty(1)->encode($json);
