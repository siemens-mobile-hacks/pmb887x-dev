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

my $cpu_meta = Sie::CpuMetadata->new("generic");
my $peripherals = $cpu_meta->getAllPeripherals();

if (@ARGV) {
	die "Usage: $0 [--header]\n" if @ARGV != 1 || $ARGV[0] ne "--header";
	print genDecoderHeader($peripherals);
	exit;
}

my $str = "#include \"hw/arm/pmb887x/gen/cpu_meta.h\"\n#include \"hw/arm/pmb887x/regs_dump.h\"\n#include \"hw/arm/pmb887x/gen/cpu_regs.h\"\n\n";

for my $module (@{$cpu_meta->getAllModules()}) {
	$str .= genModuleHeader($cpu_meta, $module);
}

for my $peripheral (@$peripherals) {
	$str .= genPeripheralHeader($peripheral);
}

my @cpus;
my @modules;

my $cpu_to_idx = {};
my $cpu_idx = 0;

for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	
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
	
	my @modules_ref;
	for my $id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$id};
		for my $region_name (getSortedKeys($module->{regions}, 'start')) {
			my $region = $module->{regions}->{$region_name};
			my $start = $module->{base} + $region->{start};
			my $end = $module->{base} + $region->{end};
			while ($start < $end) {
				my $chunk_end = min($end, ($start & 0xFFF00000) + 0x00100000);
				push @modules_ref, [
					'"'.$module->{name}.'_'.$region_name.'",',
					sprintf("0x%08X,", $start),
					sprintf("0x%X,", $chunk_end - $start),
					"NULL,",
					"0"
				];
				$start = $chunk_end;
			}
		}
		my $module_size_name = exists($module->{regions}->{IO})
			? $module->{base_name}."_REG_SIZE"
			: $module->{base_name}."_IO_SIZE";
		push @modules_ref, [
			'"'.$module->{name}.'",',
			uc($cpu_meta->{name})."_".$module->{name}."_BASE,",
			$module_size_name.",",
			lc($module->{base_name})."_regs,",
			"ARRAY_SIZE(".lc($module->{base_name})."_regs)",
		];
	}
	
	$str .= "static const pmb887x_cpu_io_t ".lc($cpu_meta->{name})."_modules[] = {\n";
	$str .= printTable(\@modules_ref, "\t{", "},");
	$str .= "};\n\n";
	
	my $modules_var = lc($cpu_meta->{name})."_modules";
	
	$cpu_to_idx->{$cpu_meta->{name}} = $cpu_idx++;
	
	push @cpus, [
		'"'.$cpu_meta->{name}.'",',
		"$irqs_var,",
		"ARRAY_SIZE($irqs_var),",
		"$gpios_var,",
		"ARRAY_SIZE($gpios_var),",
		"$modules_var,",
		"ARRAY_SIZE($modules_var)"
	];
}

$str .= "static const pmb887x_cpu_meta_t cpus_metadata[] = {\n";
$str .= printTable(\@cpus, "\t{", "},");
$str .= "};\n\n";

my @io_meta;
for my $peripheral (@$peripherals) {
	my $var = peripheralVar($peripheral);
	push @io_meta, [
		"[".peripheralConst($peripheral)."] = { ".cString($peripheral->{name}).",",
		$var."_regs,",
		"ARRAY_SIZE(".$var."_regs)"
	];
}

$str .= "static const pmb887x_io_meta_t io_metadata[PMB887X_TRACE_IO_COUNT] = {\n";
$str .= printTable(\@io_meta, "\t", " },");
$str .= "};\n";

$str .= '
const pmb887x_cpu_meta_t *pmb887x_get_cpu_meta(int cpu) {
	return &cpus_metadata[cpu];
}

const pmb887x_io_meta_t *pmb887x_get_io_meta(pmb887x_trace_io_t id) {
	if (id <= PMB887X_TRACE_IO_CPU || id >= PMB887X_TRACE_IO_COUNT)
		return NULL;
	return &io_metadata[id];
}
';

print $str;

sub genDecoderHeader {
	my ($peripherals) = @_;

	my $str = <<'HEADER';
#pragma once

#include "qemu/osdep.h"

#define PMB887X_REG_IS_IRQ_NUM		1
#define PMB887X_REG_IS_GPIO_PIN		2
#define PMB887X_REG_IS_IRQ_CON		3
#define PMB887X_REG_IS_I2C_TXD		4

typedef struct pmb887x_cpu_meta_gpio_t pmb887x_cpu_meta_gpio_t;
typedef struct pmb887x_cpu_meta_irq_t pmb887x_cpu_meta_irq_t;
typedef struct pmb887x_cpu_meta_t pmb887x_cpu_meta_t;
typedef struct pmb887x_cpu_io_t pmb887x_cpu_io_t;
typedef struct pmb887x_io_meta_t pmb887x_io_meta_t;
typedef struct pmb887x_io_reg_t pmb887x_io_reg_t;
typedef struct pmb887x_io_field_t pmb887x_io_field_t;
typedef struct pmb887x_io_value_t pmb887x_io_value_t;

typedef enum pmb887x_trace_io_t {
	PMB887X_TRACE_IO_CPU,
HEADER

	for my $peripheral (@$peripherals) {
		$str .= "\t".peripheralConst($peripheral).",\n";
	}

	$str .= <<'HEADER';
	PMB887X_TRACE_IO_COUNT,
} pmb887x_trace_io_t;

struct pmb887x_io_value_t {
	const char *name;
	uint32_t value;
};

struct pmb887x_io_field_t {
	const char *name;
	uint32_t mask;
	uint32_t shift;
	const pmb887x_io_value_t *values;
	int values_count;
};

struct pmb887x_io_reg_t {
	const char *name;
	uint32_t addr;
	const pmb887x_io_field_t *fields;
	int fields_count;
	int special;
};

struct pmb887x_cpu_io_t {
	const char *name;
	uint32_t base;
	uint32_t size;
	const pmb887x_io_reg_t *regs;
	int regs_count;
};

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

	const pmb887x_cpu_io_t *modules;
	int modules_count;
};

struct pmb887x_io_meta_t {
	const char *name;
	const pmb887x_io_reg_t *regs;
	int regs_count;
};

const pmb887x_cpu_meta_t *pmb887x_get_cpu_meta(int cpu);
const pmb887x_io_meta_t *pmb887x_get_io_meta(pmb887x_trace_io_t id);
HEADER

	return $str;
}

sub genModuleRefHeader {
	my ($cpu_meta, $module) = @_;
	
	my @header;
	push @header, "";
	
	return printTable(\@header)."\n";
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
				
				my $field_name_prepared = ($reg->{common} ? "" : $module->{name}."_").$reg->{field_format};
				$field_name_prepared =~ s/{reg}/$reg_name_prefix/g;
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
					$str .= "static const pmb887x_io_value_t ".$values_var."[] = {\n";
					$str .= printTable(\@values, "\t{", "},");
					$str .= "};\n\n";
				}
			}
			
			if (%{$reg->{fields}}) {
				$str .= "static const pmb887x_io_field_t ".$fields_var."[] = {\n";
				$str .= printTable(\@fields, "\t{", "},");
				$str .= "};\n\n";
			}
		}
		
		my $special = "0";
		
		if ($module->{name} eq "VIC") {
			if ($reg->{name} eq "CURRENT_IRQ" || $reg->{name} eq "CURRENT_FIQ") {
				$special = "PMB887X_REG_IS_IRQ_NUM";
			} elsif ($reg->{name} eq "CON") {
				$special = "PMB887X_REG_IS_IRQ_CON";
			}
		} elsif ($module->{name} eq "GPIO" && $reg->{name} eq "PIN") {
			$special = "PMB887X_REG_IS_GPIO_PIN";
		}
		
		if ($reg->{start} != $reg->{end}) {
			my $index = 0;
			for (my $i = $reg->{start}; $i <= $reg->{end}; $i += $reg->{step}) {
				push @regs, [
					'"'.$reg->{name}.$index.'",',
					$module->{name}."_".$reg->{name}.$index.",",
					%{$reg->{fields}} ? "$fields_var," : "NULL,",
					%{$reg->{fields}} ? "ARRAY_SIZE($fields_var)," : "0,",
					$special
				];
				$index++;
			}
		} else {
			push @regs, [
				'"'.$reg->{name}.'",',
				$module->{name}."_".$reg->{name}.",",
				%{$reg->{fields}} ? "$fields_var," : "NULL,",
				%{$reg->{fields}} ? "ARRAY_SIZE($fields_var)," : "0,",
				$special
			];
		}
	}
	
	$str .= "static const pmb887x_io_reg_t ".lc($module->{name})."_regs[] = {\n";
	$str .= printTable(\@regs, "\t{", "},");
	$str .= "};\n";
	
	return $str."\n";
}

sub genPeripheralHeader {
	my ($peripheral) = @_;

	my $str = "";
	my $var = peripheralVar($peripheral);
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
					sprintf("0x%08X", $value & $field->{mask})
				];
			}

			if (@values) {
				$str .= "static const pmb887x_io_value_t ".$values_var."[] = {\n";
				$str .= printTable(\@values, "\t{ ", " },");
				$str .= "};\n\n";
			}

			push @fields, [
				cString($field->{name}).",",
				sprintf("0x%08X,", $field->{mask}),
				$field->{start}.",",
				@values ? $values_var."," : "NULL,",
				@values ? "ARRAY_SIZE(".$values_var.")" : 0
			];
		}

		if (@fields) {
			$str .= "static const pmb887x_io_field_t ".$fields_var."[] = {\n";
			$str .= printTable(\@fields, "\t{ ", " },");
			$str .= "};\n\n";
		}

		push @regs, [
			cString($reg->{name}).",",
			sprintf("0x%X,", $reg->{start}),
			@fields ? $fields_var."," : "NULL,",
			@fields ? "ARRAY_SIZE(".$fields_var.")," : "0,",
			"0"
		];
	}

	$str .= "static const pmb887x_io_reg_t ".$var."_regs[] = {\n";
	$str .= printTable(\@regs, "\t{ ", " },");
	$str .= "};\n";

	return $str."\n";
}

sub peripheralVar {
	my ($peripheral) = @_;
	return lc($peripheral->{id});
}

sub peripheralConst {
	my ($peripheral) = @_;
	return "PMB887X_TRACE_IO_".$peripheral->{id};
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
