#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use List::Util qw(max);
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

my $cpu_meta = Sie::CpuMetadata->new("generic");
my $all_peripherals = $cpu_meta->getAllPeripherals();
my $peripherals = [grep { !$_->{qemu_dir} } @$all_peripherals];
my @cpu_metadata = map { Sie::CpuMetadata->new($_) } @{Sie::CpuMetadata::getCpus()};
my %dsp_definitions;

for my $meta (@cpu_metadata) {
	for my $module (values %{$meta->dspModules()}) {
		$dsp_definitions{$module->{definition}} = $module;
	}
}

if (@ARGV == 2 && $ARGV[0] eq "--peripherals") {
	genModelPeripheralRegs($ARGV[1]);
	exit;
}

if (@ARGV) {
	die "Usage: $0 [--header | --peripherals QEMU_ROOT]\n" if @ARGV != 1 || $ARGV[0] ne "--header";
	print genDecoderHeader($cpu_meta, $peripherals, \%dsp_definitions);
	exit;
}

my $str = "#include \"hw/arm/pmb887x/gen/cpu_meta.h\"\n#include \"hw/arm/pmb887x/gen/cpu_regs.h\"\n\n";

for my $module (@{$cpu_meta->getAllModules()}) {
	$str .= genModuleHeader($cpu_meta, $module);
}

for my $peripheral (@$peripherals) {
	$str .= genPeripheralHeader($peripheral);
}

for my $definition (sort keys %dsp_definitions) {
	$str .= genDspModuleHeader($dsp_definitions{$definition});
}

my @cpus;

my $cpu_to_idx = {};
my $cpu_idx = 0;

for my $cpu_meta (@cpu_metadata) {
	
	my $irqs_var = lc($cpu_meta->{name})."_irqs";
	
	my $irqs = {};
	for my $id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$id};
		for my $irq_name (keys %{$module->{irqs}}) {
			$irqs->{$module->{name}.($irq_name ? "_".$irq_name : "")} = $module->{irqs}->{$irq_name};
		}
	}
	
	my @irqs;
	for my $irq_name (getSortedKeys($irqs)) {
		push @irqs, ['"'.$irq_name.'",', uc($cpu_meta->{name})."_".$irq_name."_IRQ,", 'VIC_CON'.$irqs->{$irq_name}];
	}
	
	$str .= "static const pmb887x_cpu_meta_irq_t ".$irqs_var."[] = {\n";
	$str .= printTable(\@irqs, "\t{", "},");
	$str .= "};\n\n";
	
	my $gpios_var = lc($cpu_meta->{name})."_gpios";
	
	my @gpios;
	for my $gpio_name (getSortedKeys($cpu_meta->gpios(), 'id')) {
		my $gpio = $cpu_meta->gpios()->{$gpio_name};
		my $long_name = "GPIO_".($gpio_name =~ /^PIN\d+$/ ? $gpio_name : "PIN".$gpio->{id}."_".$gpio_name);
		push @gpios, ['"PIN'.$gpio->{id}.'",', '"'.$gpio_name.'",', '"'.$long_name.'",', uc($cpu_meta->{name})."_GPIO_".$gpio->{name}];
	}
	
	$str .= "static const pmb887x_cpu_meta_gpio_t ".$gpios_var."[] = {\n";
	$str .= printTable(\@gpios, "\t{", "},");
	$str .= "};\n\n";
	
	$cpu_to_idx->{$cpu_meta->{name}} = $cpu_idx++;
	
	push @cpus, [
		'"'.$cpu_meta->{name}.'",',
		"$irqs_var,",
		"ARRAY_SIZE($irqs_var),",
		"$gpios_var,",
		"ARRAY_SIZE($gpios_var)"
	];
}

$str .= "static const pmb887x_cpu_meta_t cpus_metadata[] = {\n";
$str .= printTable(\@cpus, "\t{", "},");
$str .= "};\n\n";

$str .= '
const pmb887x_cpu_meta_t *pmb887x_get_cpu_meta(int cpu) {
	return &cpus_metadata[cpu];
}
';

print $str;

sub genDecoderHeader {
	my ($cpu_meta, $peripherals, $dsp_definitions) = @_;

	my $str = <<'HEADER';
#pragma once

#include "qemu/osdep.h"
#include "qemu/debug-regs.h"

typedef struct pmb887x_cpu_meta_gpio_t pmb887x_cpu_meta_gpio_t;
typedef struct pmb887x_cpu_meta_irq_t pmb887x_cpu_meta_irq_t;
typedef struct pmb887x_cpu_meta_t pmb887x_cpu_meta_t;

struct pmb887x_cpu_meta_irq_t {
	const char *name;
	uint32_t id;
	uint32_t addr;
};

struct pmb887x_cpu_meta_gpio_t {
	const char *name;
	const char *func_name;
	const char *full_name;
	uint32_t id;
};

struct pmb887x_cpu_meta_t {
	const char *name;

	const pmb887x_cpu_meta_irq_t *irqs;
	int irqs_count;

	const pmb887x_cpu_meta_gpio_t *gpios;
	int gpios_count;
};

const pmb887x_cpu_meta_t *pmb887x_get_cpu_meta(int cpu);

HEADER

	for my $module (@{$cpu_meta->getAllModules()}) {
		$str .= "extern const DebugRegs ".exportedRegsVar(lc($module->{name})).";\n";
	}
	for my $peripheral (@$peripherals) {
		$str .= "extern const DebugRegs ".exportedRegsVar(peripheralVar($peripheral)).";\n";
	}
	for my $definition (sort keys %$dsp_definitions) {
		$str .= "extern const DebugRegs ".exportedRegsVar(dspModuleVar($dsp_definitions->{$definition})).";\n";
	}

	return $str;
}

sub genModuleHeader {
	my ($cpu_meta, $module) = @_;
	
	my $str = "";
	my $used_vars = {};
	
	my @regs;
	for my $reg_name (getSortedKeys($module->{regs}, 'start')) {
		my $reg = $module->{regs}->{$reg_name};
		my $fields_var = lc($module->{name})."_".lc($reg->{name})."_fields";
		my $reg_name_prefix = $reg_name;
		
		$reg_name_prefix = $reg->{common} if ($reg->{common});
		
		if (!$used_vars->{$fields_var}) {
			$used_vars->{$fields_var} = 1;
			
			my @fields;
			for my $field_name (getSortedKeys($reg->{fields}, 'start')) {
				my $field = $reg->{fields}->{$field_name};
				my $is_common_field = $reg->{common} &&
					exists $cpu_meta->{common}->{$reg->{common}}->{fields}->{$field_name};
				my $field_name_prepared = ($is_common_field ? "" : $module->{name}."_").$reg->{field_format};
				my $field_reg_name = $is_common_field ? $reg_name_prefix : $reg_name;
				$field_name_prepared =~ s/{reg}/$field_reg_name/g;
				$field_name_prepared =~ s/{field}/$field_name/g;
				
				my $values_var = lc($module->{name})."_".lc($field_name_prepared)."_values";
				
				my @values;
				for my $val_name (getSortedKeys($field->{values})) {
					my $val = $field->{values}->{$val_name};
					
					my $val_name_prepared = $reg->{enum_format};
					$val_name_prepared =~ s/{reg}/$reg_name_prefix/g;
					$val_name_prepared =~ s/{field}/$field_name/g;
					$val_name_prepared =~ s/{value}/$val_name/g;
					
					push @values, [
						'"'.$val_name.'",',
						$module->{name}."_".$val_name_prepared
					];
				}
				
				push @fields, [
					'"'.$field_name.'",',
					$field_name_prepared.",",
					$field_name_prepared."_SHIFT,",
					@values ? $values_var."," : "NULL,",
					@values ? "ARRAY_SIZE($values_var)" : 0
				];
				
				if (@values) {
					$str .= "static const DebugRegValue ".$values_var."[] = {\n";
					$str .= printTable(\@values, "\t{", "},");
					$str .= "};\n\n";
				}
			}
			
			if (%{$reg->{fields}}) {
				$str .= "static const DebugRegField ".$fields_var."[] = {\n";
				$str .= printTable(\@fields, "\t{", "},");
				$str .= "};\n\n";
			}
		}
		
		if ($reg->{start} != $reg->{end}) {
			my $index = 0;
			for (my $i = $reg->{start}; $i <= $reg->{end}; $i += $reg->{step}) {
				push @regs, [
					'"'.$reg->{name}.$index.'",',
					$module->{name}."_".$reg->{name}.$index.",",
					%{$reg->{fields}} ? "$fields_var," : "NULL,",
					%{$reg->{fields}} ? "ARRAY_SIZE($fields_var)" : "0"
				];
				$index++;
			}
		} else {
			push @regs, [
				'"'.$reg->{name}.'",',
				$module->{name}."_".$reg->{name}.",",
				%{$reg->{fields}} ? "$fields_var," : "NULL,",
				%{$reg->{fields}} ? "ARRAY_SIZE($fields_var)" : "0"
			];
		}
	}
	
	$str .= "static const DebugReg ".lc($module->{name})."_regs[] = {\n";
	$str .= printTable(\@regs, "\t{", "},");
	$str .= "};\n\n";
	my $regions_var = lc($module->{name})."_regions";
	my @regions;
	for my $region_name (getSortedKeys($module->{regions}, 'start')) {
		my $region = $module->{regions}->{$region_name};
		push @regions, [
			cString($region_name).",",
			$module->{name}."_".$region_name."_BASE,",
			sprintf("0x%X", $region->{end} - $region->{start})
		];
	}
	if (@regions) {
		$str .= "static const DebugRegion ".$regions_var."[] = {\n";
		$str .= printTable(\@regions, "\t{", "},");
		$str .= "};\n\n";
	}
	$str .= genExportedRegs(lc($module->{name}), 4, 8, @regions ? $regions_var : undef);
	
	return $str."\n";
}

sub genModelPeripheralRegs {
	my ($qemu_root) = @_;

	for my $peripheral (grep { $_->{qemu_dir} } @$all_peripherals) {
		my $var = peripheralVar($peripheral);
		my $file = "$qemu_root/$peripheral->{qemu_dir}/regs.c";
		open my $fp, ">", $file or die "open($file): $!";
		my $str = genPeripheralHeader($peripheral, $var, undef, "${var}_debug_regs", 1);
		$str =~ s/\n+\z/\n/;
		print $fp "#include \"qemu/osdep.h\"\n#include \"regs.h\"\n\n".tabsToSpaces($str);
		close $fp;
	}
}

sub genPeripheralHeader {
	my ($peripheral, $var, $addr_digits, $exported_var, $use_macros) = @_;

	my $str = "";
	$var = peripheralVar($peripheral) if !defined($var);
	my @regs;

	for my $reg_name (getSortedKeys($peripheral->{regs}, 'start')) {
		my $reg = $peripheral->{regs}->{$reg_name};
		my $fields_var = $var."_".lc($reg->{name})."_fields";
		$fields_var =~ s/[^a-z0-9_]/_/g;
		my @fields;

		for my $field_name (getSortedKeys($reg->{fields}, 'start')) {
			my $field = $reg->{fields}->{$field_name};
			my $values_var = $fields_var."_".lc($field->{name})."_values";
			$values_var =~ s/[^a-z0-9_]/_/g;
			my @values;

			for my $value_name (getSortedKeys($field->{values})) {
				my $value = $field->{values}->{$value_name} << $field->{start};
				push @values, [
					cString($value_name).",",
					$use_macros ? peripheralValueMacro($peripheral, $reg_name, $field_name, $value_name) :
						sprintf("0x%08X", $value & $field->{mask})
				];
			}

			if (@values) {
				$str .= "static const DebugRegValue ".$values_var."[] = {\n";
				$str .= printTable(\@values, "\t{ ", " },");
				$str .= "};\n\n";
			}

			my $field_macro = peripheralFieldMacro($peripheral, $reg_name, $field_name);
			push @fields, [
				cString($field->{name}).",",
				$use_macros ? "$field_macro," : sprintf("0x%08X,", $field->{mask}),
				$use_macros ? "${field_macro}_SHIFT," : $field->{start}.",",
				@values ? $values_var."," : "NULL,",
				@values ? "ARRAY_SIZE(".$values_var.")" : 0
			];
		}

		if (@fields) {
			$str .= "static const DebugRegField ".$fields_var."[] = {\n";
			$str .= printTable(\@fields, "\t{ ", " },");
			$str .= "};\n\n";
		}

		my $reg_macro = $peripheral->{id}."_".$reg_name.($reg->{start} != $reg->{end} ? "(0)" : "");
		push @regs, [
			cString($reg->{name}).",",
			$use_macros ? "$reg_macro," : sprintf("0x%X,", $reg->{start}),
			@fields ? $fields_var."," : "NULL,",
			@fields ? "ARRAY_SIZE(".$fields_var.")" : "0"
		];
	}

	$str .= "static const DebugReg ".$var."_regs[] = {\n";
	$str .= printTable(\@regs, "\t{ ", " },");
	$str .= "};\n\n";
	$str .= genExportedRegs($var, 1, $addr_digits // peripheralAddrDigits($peripheral), undef, $exported_var);

	return $str."\n";
}

sub genDspModuleHeader {
	my ($module) = @_;
	return genPeripheralHeader($module, dspModuleVar($module), 4);
}

sub peripheralAddrDigits {
	my ($peripheral) = @_;
	my $max_addr = max(map { $_->{start} } values %{$peripheral->{regs}});
	my $digits = length(sprintf("%X", $max_addr));
	return max(2, $digits + ($digits % 2));
}

sub genExportedRegs {
	my ($var, $reg_size, $addr_digits, $regions_var, $exported_var) = @_;
	my $regions = defined($regions_var) ?
		"\t.regions = $regions_var,\n\t.regions_count = ARRAY_SIZE($regions_var),\n" : "";
	return "const DebugRegs ".($exported_var // exportedRegsVar($var))." = {\n".
		"\t.regs = ${var}_regs,\n".
		"\t.count = ARRAY_SIZE(${var}_regs),\n".
		$regions.
		"\t.reg_size = $reg_size,\n".
		"\t.addr_digits = $addr_digits,\n".
		"};\n";
}

sub exportedRegsVar {
	my ($var) = @_;
	return "pmb887x_${var}_regs";
}

sub dspModuleVar {
	my ($module) = @_;
	return "dsp_".lc($module->{definition});
}

sub peripheralVar {
	my ($peripheral) = @_;
	return lc($peripheral->{id});
}

sub cString {
	my ($value) = @_;

	$value = "" if !defined $value;
	$value =~ s/\\/\\\\/g;
	$value =~ s/"/\\"/g;
	$value =~ s/\n/\\n/g;
	$value =~ s/\r/\\r/g;
	$value =~ s/\t/\\t/g;
	return '"'.$value.'"';
}
