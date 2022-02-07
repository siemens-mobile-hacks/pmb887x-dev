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

# CPU regs
my $cpu_file = dirname(__FILE__).'/../lib/gen/cpu.h';
my $cpu_str = "#pragma once\n\n";

for my $cpu ("pmb8875", "pmb8876") {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	
	my $file = dirname(__FILE__).'/../lib/gen/'.$cpu.'_regs.h';
	
	my $str = "#pragma once\n";
	$str .= "#include <pmb887x.h>\n\n";
	
	my $irqs = {};
	for my $id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$id};
		
		for my $irq_name (keys %{$module->{irqs}}) {
			$irqs->{"NVIC_".$module->{name}.($irq_name ? "_".$irq_name : "")."_IRQ"} = $module->{irqs}->{$irq_name};
		}
	}
	
	$str .= "// IRQ numbers\n";
	$str .= getIrqsHeader($irqs);
	$str .= "\n";
	
	$str .= "// Common regs for all modules\n";
	$str .= getCommonRegsHeader($cpu_meta, $cpu_meta->{common});
	$str .= "\n";
	
	for my $id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$id};
		$str .= genModuleHeader($cpu_meta, $module);
	}
	
	open(F, ">$file") or die "open($file): $!";
	print F $str;
	close F;
	
	$cpu_str .= "#ifdef ".uc($cpu)."\n";
	$cpu_str .= "#include \"".$cpu."_regs.h\"\n";
	$cpu_str .= "#endif\n\n";
}

open(F, ">$cpu_file") or die "open($cpu_file): $!";
print F $cpu_str;
close F;

# Boards
my $board_file = dirname(__FILE__).'/../lib/gen/board.h';
my $board_str = "#pragma once\n\n";

for my $board ("EL71", "CX75") {
	my $board_meta = Sie::BoardMetadata->new($board);
	
	my $file = dirname(__FILE__).'/../lib/gen/board_'.$board.'.h';
	my $str = "#pragma once\n";
	$str .= "#include <stdint.h>\n\n";
	
	$str .= "#define ".uc($board_meta->cpu()->{name})."\n\n";
	
	$str .= "// GPIO numbers\n";
	$str .= getGpioHeader($board_meta->gpios());
	$str .= "\n";
	
	$board_str .= "#ifdef BOARD_".$board."\n";
	$board_str .= "#include \"board_".$board.".h\"\n";
	$board_str .= "#endif\n\n";
	
	open(F, ">$file") or die "open($file): $!";
	print F $str;
	close F;
}

open(F, ">$board_file") or die "open($board_file): $!";
print F $board_str;
close F;

sub getCommonRegsHeader {
	my ($cpu_meta, $regs) = @_;
	
	return genModuleHeader($cpu_meta, {
		regs		=> $regs,
		common		=> 1
	});
}

sub getGpioHeader {
	my ($gpios) = @_;
	my @header;
	for my $gpio_name (getSortedKeys($gpios)) {
		push @header, ["#define", "GPIO_".$gpio_name, $gpios->{$gpio_name}];
	}
	return printTable(\@header)."\n";
}

sub getIrqsHeader {
	my ($irqs) = @_;
	
	my @header;
	for my $irq_name (getSortedKeys($irqs)) {
		push @header, ["#define", $irq_name, $irqs->{$irq_name}];
	}
	return printTable(\@header)."\n";
}

sub genModuleHeader {
	my ($cpu_meta, $module) = @_;
	
	my @header;
	
	my $name = $module->{name};
	my $prefix = $name ? $name."_" : "";
	
	if (!$module->{common}) {
		if ($module->{multi}) {
			$name =~ s/\d+$//g;
			$prefix = $name."_";
			
			return "" if ($module->{name} !~ /[^\d]0$/);
			
			for my $neighbor_id (@{$cpu_meta->getModuleNames()}) {
				my $neighbor_module = $cpu_meta->{modules}->{$neighbor_id};
				if ($neighbor_id =~ s/^\Q$name\E\d+$//) {
					push @header, ["#define", $neighbor_module->{name}."_BASE", sprintf("0x%08X", $neighbor_module->{base})];
					push @header, ["#define", $neighbor_module->{name}, sprintf("0x%08X", $neighbor_module->{base})];
					push @header, [];
				}
			}
		} else {
			push @header, ["#define", $module->{name}."_BASE", sprintf("0x%08X", $module->{base})];
		}
	}
	
	for my $reg_name (getSortedKeys($module->{regs}, 'start')) {
		my $reg = $module->{regs}->{$reg_name};
		
		if ($reg->{descr}) {
			push @header, "/* ".$reg->{descr}." */";
		}
		
		if (!$module->{common}) {
			if ($reg->{start} != $reg->{end}) {
				if ($module->{multi}) {
					push @header, ["#define", $prefix.$reg_name."(base, n)", sprintf("MMIO32(base + 0x%02X + ((n) * 0x%X))", $reg->{start}, $reg->{step})];
				} else {
					push @header, ["#define", $prefix.$reg_name."(n)", sprintf("MMIO32(%s + 0x%02X + ((n) * 0x%X))", $module->{name}."_BASE", $reg->{start}, $reg->{step})];
				}
			} else {
				if ($module->{multi}) {
					push @header, ["#define", $prefix.$reg_name."(base)", sprintf("MMIO32((base) + 0x%02X)", $reg->{start})];
				} else {
					push @header, ["#define", $prefix.$reg_name, sprintf("MMIO32(%s + 0x%02X)", $module->{name}."_BASE", $reg->{start})];
				}
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
					push @header, ["#define", $prefix.$field_name_prepared, "GENMASK(".$field->{size}.", ".$field->{start}.")", $descr];
					push @header, ["#define", $prefix.$field_name_prepared."_SHIFT", $field->{start}];
				} else {
					push @header, ["#define", $prefix.$field_name_prepared, "BIT(".$field->{start}.")", $descr];
				}
				
				for my $val_name (getSortedKeys($field->{values})) {
					my $val = $field->{values}->{$val_name};
					
					my $val_name_prepared = $reg->{enum_format};
					$val_name_prepared =~ s/{reg}/$reg_name/g;
					$val_name_prepared =~ s/{field}/$field_name/g;
					$val_name_prepared =~ s/{value}/$val_name/g;
					
					push @header, ["#define", $prefix.$val_name_prepared, sprintf("0x%X", $val << $field->{start})];
				}
			}
		}
		push @header, [];
	}
	
	my $module_descr = "";
	
	if (!$module->{common}) {
		if ($module->{type} eq "AMBA") {
			$module_descr = sprintf("// %s [AMBA PL%03X]\n", $module->{name}, $module->{id} & 0xFFF);
		} elsif ($module->{type} eq "MODULE") {
			my $MOD_REV = $module->{id} & 0xFF;
			my $MOD_NUM = ($module->{id} >> 16) & 0xFFFF;
			my $MOD_32BIT = ($module->{id} >> 8) & 0xFF;
			
			if ($MOD_32BIT != 0xC0) {
				$MOD_NUM = $MOD_32BIT;
				$MOD_32BIT = 0;
			}
			
			$module_descr = sprintf("// %s [MOD_NUM=%04X, MOD_REV=%02X, MOD_32BIT=%02X]\n", $module->{name}, $MOD_NUM, $MOD_REV, $MOD_32BIT);
		} else {
			$module_descr = sprintf("// %s\n", $module->{name});
		}
	}
	
	return $module_descr.printTable(\@header)."\n";
}
