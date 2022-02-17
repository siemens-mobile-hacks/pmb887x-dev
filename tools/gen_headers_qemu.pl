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

my $str = "#pragma once\n\n";

my $cpu_id = 0;
for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	$str .= genCpuHeader($cpu_meta, $cpu_id);
	$cpu_id++;
}

my $board_id = 0;
for my $board (@{Sie::BoardMetadata::getBoards()}) {
	my $board_meta = Sie::BoardMetadata->new($board);
	$str .= genBoardHeader($board_meta, $board_id);
	$board_id++;
}

my $cpu_meta = Sie::CpuMetadata->new("generic");
$str .= getCommonRegsHeader($cpu_meta, $cpu_meta->{common});
for my $module (@{$cpu_meta->getAllModules()}) {
	$str .= genModuleHeader($cpu_meta, $module);
}

print $str."\n";

sub getCommonRegsHeader {
	my ($cpu_meta, $regs) = @_;
	
	return genModuleHeader($cpu_meta, {
		regs		=> $regs,
		common		=> 1
	});
}

sub genBoardHeader {
	my ($board_meta, $board_id) = @_;
	
	my @header;
	push @header, "/* BOARD: ".$board_meta->{name}." */";
	push @header, ["#define BOARD_".uc($board_meta->{name}), $board_id];
	push @header, "";
	
	push @header, "// gpios";
	for my $gpio_name (getSortedKeys($board_meta->gpios(), 'id')) {
		my $gpio = $board_meta->gpios()->{$gpio_name};
		push @header, [
			"#define ".uc($board_meta->{name})."_GPIO_".($gpio->{alias} || $gpio->{name}),
			uc($board_meta->{cpu}->{name})."_GPIO_".$gpio->{name}
		];
	}
	
	push @header, "";
	push @header, "// keys";
	
	for my $kp_name (getSortedKeys($board_meta->{keys}, 'code')) {
		my $kp = $board_meta->{keys}->{$kp_name};
		push @header, ["#define ".uc($board_meta->{name})."_KP_".$kp_name, sprintf("0x%08X", $kp->{code})];
	}

	return printTable(\@header)."\n";
}

sub genCpuHeader {
	my ($cpu_meta, $cpu_id) = @_;
	
	my @header;
	push @header, "/* CPU: ".$cpu_meta->{name}." */";
	
	push @header, ["#define CPU_".uc($cpu_meta->{name}), $cpu_id];
	
	my $irqs = {};
	for my $id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$id};
		
		push @header, ["#define ".uc($cpu_meta->{name})."_".$module->{name}."_BASE", sprintf("0x%08X", $module->{base})];
		
		for my $irq_name (keys %{$module->{irqs}}) {
			$irqs->{uc($cpu_meta->{name})."_".$module->{name}.($irq_name ? "_".$irq_name : "")."_IRQ"} = $module->{irqs}->{$irq_name};
		}
	}
	
	push @header, "";
	
	for my $gpio_name (getSortedKeys($cpu_meta->gpios(), 'id')) {
		my $gpio = $cpu_meta->gpios()->{$gpio_name};
		push @header, [
			"#define ".uc($cpu_meta->{name})."_GPIO_".$gpio->{name},
			$gpio->{id}
		];
	}
	
	push @header, "";
	
	for my $irq_name (getSortedKeys($irqs)) {
		push @header, ["#define $irq_name", $irqs->{$irq_name}];
	}
	
	return printTable(\@header)."\n";
}

sub genModuleHeader {
	my ($cpu_meta, $module) = @_;
	
	my @header;
	
	my $name = $module->{name} || "";
	my $prefix = ($name ? $name."_" : "");
	
	if (!$module->{common}) {
		push @header, ["#define ".$name."_IO_SIZE", sprintf("0x%08X", $module->{size})];
	}
	
	for my $reg_name (getSortedKeys($module->{regs}, 'start')) {
		my $reg = $module->{regs}->{$reg_name};
		
		if ($reg->{descr}) {
			push @header, "/* ".$reg->{descr}." */";
		}
		
		if (!$module->{common}) {
			if ($reg->{start} != $reg->{end}) {
				my $index = 0;
				for (my $i = $reg->{start}; $i <= $reg->{end}; $i += $reg->{step}) {
					push @header, ["#define ".$prefix.$reg_name.$index, sprintf("0x%02X", $i)];
					$index++;
				}
			} else {
				push @header, ["#define ".$prefix.$reg_name, sprintf("0x%02X", $reg->{start})];
			}
		}
		
		if (!$reg->{common}) {
			for my $field_name (getSortedKeys($reg->{fields}, 'start')) {
				my $field = $reg->{fields}->{$field_name};
				
				my $field_name_prepared = $reg->{field_format};
				$field_name_prepared =~ s/{reg}/$reg_name/g;
				$field_name_prepared =~ s/{field}/$field_name/g;
				
				my $descr = "";
				
				if ($field->{descr}) {
					$descr = " // ".$field->{descr};
				}
				
				if ($field->{size} > 1) {
					push @header, ["#define ".$prefix.$field_name_prepared, "(".sprintf("0x%0X", (1 << $field->{size}) - 1)." << ".$field->{start}.")", $descr];
				} else {
					push @header, ["#define ".$prefix.$field_name_prepared, "(1 << ".$field->{start}.")", $descr];
				}
				
				push @header, ["#define ".$prefix.$field_name_prepared."_SHIFT", $field->{start}];
				
				for my $val_name (getSortedKeys($field->{values})) {
					my $val = $field->{values}->{$val_name};
					
					my $val_name_prepared = $reg->{enum_format};
					$val_name_prepared =~ s/{reg}/$reg_name/g;
					$val_name_prepared =~ s/{field}/$field_name/g;
					$val_name_prepared =~ s/{value}/$val_name/g;
					
					push @header, ["#define ".$prefix.$val_name_prepared, sprintf("0x%X", $val << $field->{start})];
				}
			}
		}
		push @header, [];
	}
	
	my $module_descr = "";
	if ($module->{common}) {
		$module_descr = "// Common regs for all modules\n";
	} else {
		if ($module->{type} eq "AMBA") {
			$module_descr = sprintf("// %s [AMBA PL%03X]\n", $name, $module->{id} & 0xFFF);
		} elsif ($module->{type} eq "MODULE") {
			my $MOD_REV = $module->{id} & 0xFF;
			my $MOD_NUM = ($module->{id} >> 16) & 0xFFFF;
			my $MOD_32BIT = ($module->{id} >> 8) & 0xFF;
			
			if ($MOD_32BIT != 0xC0) {
				$MOD_NUM = $MOD_32BIT;
				$MOD_32BIT = 0;
			}
			
			$module_descr = sprintf("// %s [MOD_NUM=%04X, MOD_REV=%02X, MOD_32BIT=%02X]\n", $name, $MOD_NUM, $MOD_REV, $MOD_32BIT);
		} else {
			$module_descr = sprintf("// %s\n", $name);
		}
		$module_descr .= "// ".$module->{descr}."\n" if $module->{descr};
	}
	
	return $module_descr.printTable(\@header)."\n";
}
