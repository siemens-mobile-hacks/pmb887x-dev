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

my $str = "#include \"hw/arm/pmb887x/regs_info.h\"\n#include \"hw/arm/pmb887x/regs_dump.h\"\n#include \"hw/arm/pmb887x/regs.h\"\n\n";

my $cpu_meta = Sie::CpuMetadata->new("generic");
for my $module (@{$cpu_meta->getAllModules()}) {
	$str .= genModuleHeader($cpu_meta, $module);
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
		push @modules_ref, [
			'"'.$module->{name}.'",',
			uc($cpu_meta->{name})."_".$module->{name}."_BASE,",
			$module->{base_name}."_IO_SIZE,",
			lc($module->{base_name})."_regs,",
			"ARRAY_SIZE(".lc($module->{base_name})."_regs)",
		];
	}
	
	$str .= "static const pmb887x_module_t ".lc($cpu_meta->{name})."_modules[] = {\n";
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
$str .= "};\n";

$str .= '
const pmb887x_cpu_meta_t *pmb887x_get_cpu_meta(int cpu) {
	return &cpus_metadata[cpu];
}
';

print $str."\n";

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
					$str .= "static const pmb887x_module_value_t ".$values_var."[] = {\n";
					$str .= printTable(\@values, "\t{", "},");
					$str .= "};\n\n";
				}
			}
			
			if (%{$reg->{fields}}) {
				$str .= "static const pmb887x_module_field_t ".$fields_var."[] = {\n";
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
	
	$str .= "static const pmb887x_module_reg_t ".lc($module->{name})."_regs[] = {\n";
	$str .= printTable(\@regs, "\t{", "},");
	$str .= "};\n";
	
	return $str."\n";
}
