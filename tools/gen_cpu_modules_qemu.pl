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
#include "hw/arm/pmb887x/gen/dsp.h"

#include "hw/core/hw-error.h"

|;

for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	print genCpuModulesList($cpu_meta);
}

print qq|const pmb887x_cpu_t *pmb887x_cpu_get(int cpu_id) {
	switch (cpu_id) {
		case CPU_PMB8875:
			return &pmb8875_cpu;

		case CPU_PMB8876:
			return &pmb8876_cpu;

		default:
			hw_error("Invalid CPU type: \%d", cpu_id);
	}
	return NULL;
}
|;

sub genCpuModulesList {
	my ($cpu_meta) = @_;

	my $str = genDspConfig($cpu_meta);

	my @modules;
	for my $module_id (@{$cpu_meta->getModuleNames()}) {
		my $module = $cpu_meta->{modules}->{$module_id};
		next if !$module->{qemu};

		my $irqs_var = lc($cpu_meta->{name}."_".$module->{name}."_irqs");
		my $gpios_var = lc($cpu_meta->{name}."_".$module->{name}."_gpios");
		my $dma_var = lc($cpu_meta->{name}."_".$module->{name}."_dma");

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
			$str .= printTable(\@module_gpios, "\t{", "},");
			$str .= "};\n\n";
		}

		my @module_dma;
		if (exists $cpu_meta->{dma}->{$module_id}) {
			for my $dma (@{$cpu_meta->{dma}->{$module_id}}) {
				push @module_dma, [
					'"'.$dma->{channel}.'",',
					"PMB887X_DMAC_BUS_".$dma->{bus}.",",
					$dma->{request}.",",
					$dma->{sel},
				];
			}
		}

		if (@module_dma) {
			$str .= "static const pmb887x_cpu_module_dma_t $dma_var\[] = {\n";
			$str .= printTable(\@module_dma, "\t{", "},");
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
			@module_gpios ? "ARRAY_SIZE($gpios_var)," : "0,",
			@module_dma ? "$dma_var," : "NULL,",
			@module_dma ? "ARRAY_SIZE($dma_var)" : "0",
		];
	}

	$str .= "static const pmb887x_cpu_module_t ".$cpu_meta->{name}."_modules[] = {\n";
	$str .= printTable(\@modules, "\t{", "},");
	$str .= "};\n\n";
	$str .= "static const pmb887x_cpu_t ".$cpu_meta->{name}."_cpu = {\n";
	$str .= "\t.modules = ".$cpu_meta->{name}."_modules,\n";
	$str .= "\t.modules_count = ARRAY_SIZE(".$cpu_meta->{name}."_modules),\n";
	$str .= "\t.dsp_config = &".$cpu_meta->{name}."_dsp_config,\n";
	$str .= "};\n\n";

	return $str;
}

sub getDspMemory {
	my ($cpu_meta, $space, $name) = @_;
	my @matches = grep { $_->{space} eq $space && $_->{name} eq $name } @{$cpu_meta->dspMemory()};
	die "Missing $cpu_meta->{name} DSP $space:$name memory" if @matches != 1;
	return $matches[0];
}

sub getDspPages {
	my ($cpu_meta, $space, $prefix) = @_;
	my @pages = grep { $_->{space} eq $space && $_->{name} =~ /^\Q$prefix\E\d+$/ } @{$cpu_meta->dspMemory()};
	@pages = sort { $a->{page} <=> $b->{page} } @pages;
	for (my $i = 0; $i < @pages; $i++) {
		die "Invalid $cpu_meta->{name} DSP $space:$prefix page index" if $pages[$i]->{page} != $i;
	}
	return @pages;
}

sub getDspPeripheralMemory {
	my ($cpu_meta, $module) = @_;
	my @names = ($module->{name}."_RAM", $module->{base_name}."_RAM");
	push @names, "DEMODULATOR_RAM" if $module->{definition} eq "BASEBAND";

	for my $name (@names) {
		my @matches = grep { $_->{space} eq "D" && $_->{name} eq $name } @{$cpu_meta->dspMemory()};
		return $matches[0] if @matches == 1;
	}
	return undef;
}

sub getPageMask {
	my ($count) = @_;
	my $mask = 0;
	$mask = ($mask << 1) | 1 while $mask + 1 < $count;
	return $mask;
}

sub genDspConfig {
	my ($cpu_meta) = @_;
	my $cpu = $cpu_meta->{name};
	my $prefix = uc($cpu)."_TEAK_";
	getDspMemory($cpu_meta, "P", "PROM_FIXED");
	getDspMemory($cpu_meta, "D", "DROM_FIXED");
	getDspMemory($cpu_meta, "D", "SHARED_RAM");
	getDspMemory($cpu_meta, "D", "DEMODULATOR_RAM");
	getDspMemory($cpu_meta, "D", "YRAM");
	my @program_pages = getDspPages($cpu_meta, "P", "PROM_PAGE");
	my @data_pages = getDspPages($cpu_meta, "D", "DROM_PAGE");
	die "Missing $cpu DSP ROM version" if !defined($cpu_meta->dspRomVersion());
	die "Missing $cpu DSP ROM pages" if !@program_pages || !@data_pages;

	my $page_mask = getPageMask(max(scalar(@program_pages), scalar(@data_pages)));
	my $program_page_shift = 0;
	my $mask = $page_mask;
	while ($mask != 0) {
		$program_page_shift++;
		$mask >>= 1;
	}

	my @modules = values %{$cpu_meta->dspModules()};
	die "Missing $cpu DSP modules" if !@modules;
	my $mmio_base = min(map { $_->{base} } @modules);
	my $mmio_end = max(map { $_->{base} + $_->{size} } @modules);
	die "Invalid $cpu DSP interrupt module base" if $cpu_meta->dspModules()->{INT}->{base} != $mmio_base;
	$mmio_end = ($mmio_end + 0xFF) & ~0xFF;

	my @peripherals;
	for my $module (sort { $a->{base} <=> $b->{base} } @modules) {
		my $type = $module->{definition};
		my $memory = getDspPeripheralMemory($cpu_meta, $module);

		# Numeric suffixes select register layouts, not QEMU peripheral implementations.
		$type =~ s/_[0-9]+$//;
		push @peripherals, {
			name => $module->{name},
			type => $type,
			base => $module->{base},
			size => $module->{size},
			memory => $memory,
		};
	}
	my $str = "static const pmb887x_dsp_peripheral_config_t ${cpu}_dsp_peripherals[] = {\n";
	for my $peripheral (@peripherals) {
		my $memory = $peripheral->{memory};

		$str .= "\t{\n";
		$str .= "\t\t.name = \"$peripheral->{name}\",\n";
		$str .= "\t\t.type = PMB887X_DSP_PERIPHERAL_$peripheral->{type},\n";
		$str .= sprintf("\t\t.base = 0x%04X,\n", $peripheral->{base});
		$str .= sprintf("\t\t.size = 0x%04X,\n", $peripheral->{size});
		$str .= $memory ? "\t\t.ram_base = ${prefix}$memory->{name}_BASE,\n" : "\t\t.ram_base = 0,\n";
		$str .= $memory ? "\t\t.ram_size = ${prefix}$memory->{name}_SIZE,\n" : "\t\t.ram_size = 0,\n";
		$str .= "\t},\n";
	}
	$str .= "};\n\n";

	$str .= sprintf(qq|static const pmb887x_dsp_config_t ${cpu}_dsp_config = {
	.name = "$cpu",
	.default_rom_version = 0x%04X,
	.page_field_mask = 0x%04X,
	.program_page_shift = %u,
	.program_rom_base = ${prefix}PROM_FIXED_BASE,
	.program_bank_base = ${prefix}PROM_PAGE0_BASE,
	.program_bank_count = %u,
	.data_rom_base = ${prefix}DROM_FIXED_BASE,
	.data_bank_base = ${prefix}DROM_PAGE0_BASE,
	.data_bank_count = %u,
	.shared_base = ${prefix}SHARED_RAM_BASE,
	.shared_size = ${prefix}SHARED_RAM_SIZE,
	.y_space_base = ${prefix}YRAM_BASE,
	.mmio_base = ${prefix}INT_BASE,
	.mmio_size = 0x%04X,
	.peripherals = ${cpu}_dsp_peripherals,
	.peripheral_count = ARRAY_SIZE(${cpu}_dsp_peripherals),
};

|, $cpu_meta->dspRomVersion(), $page_mask, $program_page_shift, scalar(@program_pages), scalar(@data_pages),
		$mmio_end - $mmio_base);
	return $str;
}
