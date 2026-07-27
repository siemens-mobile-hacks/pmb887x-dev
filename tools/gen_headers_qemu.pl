#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use File::Path qw(make_path);
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

if (@ARGV) {
	die "Usage: $0 [--peripherals DIR]\n" if @ARGV != 2 || $ARGV[0] ne "--peripherals";
	genPeripheralHeaders($ARGV[1]);
	exit;
}

my $str = "#pragma once\n\n";

my $cpu_id = 0;
for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	$str .= genCpuHeader($cpu_meta, $cpu_id);
	$cpu_id++;
}

my $cpu_meta = Sie::CpuMetadata->new("generic");
$str .= getCommonRegsHeader($cpu_meta, $cpu_meta->{common});
for my $module (@{$cpu_meta->getAllModules()}) {
	$str .= genModuleHeader($cpu_meta, $module);
}

print $str."\n";

sub genPeripheralHeaders {
	my ($root) = @_;
	my $cpu_meta = Sie::CpuMetadata->new("generic");
	make_path($root);

	for my $peripheral (@{$cpu_meta->getAllPeripherals()}) {
		my $file = $root."/".$peripheral->{id}.".h";
		open my $fp, ">", $file or die "open($file): $!";
		print $fp genPeripheralHeader($peripheral);
		close $fp;
	}
}

sub genPeripheralHeader {
	my ($peripheral) = @_;
	my @header;
	my $prefix = $peripheral->{id}."_";
	my $addr_width = 2;

	for my $reg (values %{$peripheral->{regs}}) {
		$addr_width = max($addr_width, length(sprintf("%X", $reg->{end})));
	}

	push @header, "#pragma once";
	push @header, "";
	push @header, "#include \"qemu/bitops.h\"";
	push @header, "";
	push @header, "// ".$peripheral->{id};
	push @header, "// ".$peripheral->{descr} if $peripheral->{descr};

	if (defined $peripheral->{addr}) {
		push @header, ["#define", $prefix."I2C_ADDR", sprintf("0x%02X", $peripheral->{addr})];
		push @header, [];
	}

	for my $reg_name (getSortedKeys($peripheral->{regs}, 'start')) {
		my $reg = $peripheral->{regs}->{$reg_name};
		push @header, "/* ".$reg->{descr}." */" if $reg->{descr};

		if ($reg->{start} != $reg->{end}) {
			push @header, [
				"#define",
				$prefix.$reg_name."(n)",
				sprintf("(0x%0*X + ((n) * 0x%X))", $addr_width, $reg->{start}, $reg->{step})
			];
		} else {
			push @header, ["#define", $prefix.$reg_name, sprintf("0x%0*X", $addr_width, $reg->{start})];
		}

		for my $field_name (getSortedKeys($reg->{fields}, 'start')) {
			my $field = $reg->{fields}->{$field_name};
			my $field_name_prepared = $reg->{field_format};
			$field_name_prepared =~ s/{reg}/$reg_name/g;
			$field_name_prepared =~ s/{field}/$field_name/g;
			my $descr = $field->{descr} ? " // ".$field->{descr} : "";

			if ($field->{size} > 1) {
				push @header, [
					"#define",
					$prefix.$field_name_prepared,
					"MAKE_64BIT_MASK(".$field->{start}.", ".$field->{size}.")",
					$descr
				];
				push @header, ["#define", $prefix.$field_name_prepared."_SHIFT", $field->{start}];
			} else {
				push @header, ["#define", $prefix.$field_name_prepared, "BIT(".$field->{start}.")", $descr];
			}

			for my $value_name (getSortedKeys($field->{values})) {
				my $value_name_prepared = $reg->{enum_format};
				$value_name_prepared =~ s/{reg}/$reg_name/g;
				$value_name_prepared =~ s/{field}/$field_name/g;
				$value_name_prepared =~ s/{value}/$value_name/g;

				my $value = $field->{values}->{$value_name} << $field->{start};
				push @header, ["#define", $prefix.$value_name_prepared, sprintf("0x%X", $value)];
			}
		}
		push @header, [];
	}

	my $str = printTable(\@header);
	$str =~ s/\s+\z/\n/;
	return $str;
}

sub getCommonRegsHeader {
	my ($cpu_meta, $regs) = @_;
	
	return genModuleHeader($cpu_meta, {
		regs		=> $regs,
		common		=> 1
	});
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
			"#define ".uc($cpu_meta->{name})."_GPIO_PIN".$gpio->{id},
			$gpio->{id}
		];
	}
	
	push @header, "";
	
	for my $gpio_name (getSortedKeys($cpu_meta->gpios(), 'id')) {
		my $gpio = $cpu_meta->gpios()->{$gpio_name};
		next if $gpio->{name} =~ /^PIN(\d+)$/;
		push @header, [
			"#define ".uc($cpu_meta->{name})."_GPIO_".$gpio->{name},
			uc($cpu_meta->{name})."_GPIO_PIN".$gpio->{id}
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
		my $size_name = exists($module->{regions}->{IO}) ? $name."_REG_SIZE" : $name."_IO_SIZE";
		push @header, ["#define ".$size_name, sprintf("0x%08X", $module->{size})];
	}

	for my $region_name (getSortedKeys($module->{regions} || {}, 'start')) {
		my $region = $module->{regions}->{$region_name};
		my $region_prefix = $prefix.$region_name;
		push @header, ["#define ".$region_prefix."_BASE", sprintf("0x%X", $region->{start})];
		if ($region->{element_size}) {
			push @header, ["#define ".$region_prefix."0", $region_prefix."_BASE"];
			push @header, [
				"#define ".$region_prefix."(n)",
				sprintf("(%s_BASE + ((n) * 0x%X))", $region_prefix, $region->{element_size})
			];
		} else {
			push @header, ["#define ".$region_prefix."_SIZE", sprintf("0x%X", $region->{size})];
		}
		push @header, [];
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
			for my $id (@{$module->{ids}}) {
				my $MOD_REV = $id & 0xFF;
				my $MOD_NUM = ($id >> 16) & 0xFFFF;
				my $MOD_32BIT = ($id >> 8) & 0xFF;

				if ($MOD_32BIT != 0xC0) {
					$MOD_NUM = $MOD_32BIT;
					$MOD_32BIT = 0;
				}

				$module_descr .= sprintf("// %s [MOD_NUM=%04X, MOD_REV=%02X, MOD_32BIT=%02X]\n", $name, $MOD_NUM, $MOD_REV, $MOD_32BIT);
			}
		} else {
			$module_descr = sprintf("// %s\n", $name);
		}
		$module_descr .= "// ".$module->{descr}."\n" if $module->{descr};
	}
	
	return $module_descr.printTable(\@header)."\n";
}
