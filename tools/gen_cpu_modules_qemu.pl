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

print qq|#include "hw/arm/pmb887x/gen/cpu_modules.h"

#include "hw/arm/pmb887x/gen/cpu_meta.h"
#include "hw/arm/pmb887x/gen/cpu_regs.h"

#include "hw/hw.h"

|;

for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	print genCpuModulesList($cpu_meta);
}

print qq|const pmb887x_cpu_module_t *pmb887x_cpu_get_modules_list(int cpu_id) {
	switch (cpu_id) {
		case CPU_PMB8875:
			return pmb8875_modules;

		case CPU_PMB8876:
			return pmb8876_modules;

		default:
			hw_error("Invalid CPU type: \%d", cpu_id);
	}
	return NULL;
}
|;

sub genCpuModulesList {
	my ($cpu_meta) = @_;

	my $str = "";

	my @modules;
	for my $module_id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$module_id};
		next if !$module->{qemu};

		my $irqs_var = lc($cpu_meta->{name}."_".$module->{name}."_irqs");
		my $gpios_var = lc($cpu_meta->{name}."_".$module->{name}."_gpios");

		my @module_irqs;
		for my $irq_name (@{$module->{irqs_needed}}) {
			my $full_irq_name = uc($cpu_meta->{name}."_".$module->{name}.($irq_name ? "_".$irq_name : "")."_IRQ");
			push @module_irqs, $full_irq_name;
		}

		if (@module_irqs) {
			$str .= "static const int $irqs_var\[] = {\n";
			$str .= "\t".join(",\n\t", @module_irqs)."\n";
			$str .= "};\n\n";
		}

		my @module_gpios;
		for my $gpio_name (getSortedKeys($cpu_meta->gpios(), 'id')) {
			my $gpio = $cpu_meta->gpios()->{$gpio_name};
			my $gpio_const_name = "GPIO_".($gpio_name =~ /^PIN\d+$/ ? $gpio_name : "PIN".$gpio->{id}."_".$gpio_name);

			for my $alt (@{$gpio->{alt}}) {
				next if $alt->{name} !~ /:/;
				my ($alt_module_name, $alt_pin) = split(/:/, $alt->{name});
				die "Unknown module $alt_module_name" if !exists $cpu_meta->{modules}->{$alt_module_name};

				my $alt_module = $cpu_meta->{modules}->{$alt_module_name};
				die "GPIO line $alt_pin not found in $alt_module_name" if !grep { $_ eq $alt_pin } @{$alt_module->{gpio_lines}};

				next if $alt_module->{name} ne $module->{name};

				if ($alt->{flags} eq "I" || $alt->{flags} eq "IO") {
					push @module_gpios, [
						'"'.$alt_pin.'_IN",',
						uc($cpu_meta->{name})."_GPIO_".$gpio->{name}.",",
						$alt->{id} - 1,
					];
				}

				if ($alt->{flags} eq "O" || $alt->{flags} eq "IO") {
					push @module_gpios, [
						'"'.$alt_pin.'_OUT",',
						uc($cpu_meta->{name})."_GPIO_".$gpio->{name}.",",
						$alt->{id} - 1,
					];
				}
			}
		}

		if (@module_gpios) {
			$str .= "static const pmb887x_cpu_module_gpio_t $gpios_var\[] = {\n";
			#$str .= "\t".join("\n\t", @module_gpios)."\n";
			$str .= printTable(\@module_gpios, "\t{", "},");
			$str .= "};\n\n";
		}

		push @modules, [
			'"'.$module->{name}.'",',
			sprintf("0x%08X", $module->{id}).",",
			uc($cpu_meta->{name}."_".$module->{name}."_BASE").",",
			'"'.$module->{qemu}.'",',
			@module_irqs ? "$irqs_var," : "NULL,",
			@module_irqs ? "ARRAY_SIZE($irqs_var)," : "0,",
			@module_gpios ? "$gpios_var," : "NULL,",
			@module_gpios ? "ARRAY_SIZE($gpios_var)" : "0",
		];
	}

	$str .= "static const pmb887x_cpu_module_t ".$cpu_meta->{name}."_modules[] = {\n";
	$str .= printTable(\@modules, "\t{", "},");
	$str .= "};\n\n";

	return $str;
}
