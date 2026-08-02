#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use File::Path qw(make_path);
use lib dirname(__FILE__).'/lib';
use Sie::CpuMetadata;
use Sie::Utils;

my $output_dir = dirname(__FILE__).'/../lib/gen';
my $qemu = 0;

if (@ARGV) {
	die "Usage: $0 [--qemu OUTPUT_DIR]\n" if @ARGV != 2 || $ARGV[0] ne '--qemu';
	$qemu = 1;
	$output_dir = $ARGV[1];
}

my @cpus = sort @{Sie::CpuMetadata::getCpus()};
my @cpu_metadata = map { Sie::CpuMetadata->new($_) } @cpus;
my ($register_header, $cpu_register_headers) = genRegisterHeaders(\@cpu_metadata, !$qemu, $qemu);

make_path($output_dir);
if ($qemu) {
	writeFile("$output_dir/dsp.h", genQemuHeader(\@cpu_metadata).$register_header);
} else {
	my @selector = ("#pragma once", "");
	for my $cpu_meta (@cpu_metadata) {
		my $cpu = $cpu_meta->{name};
		writeFile("$output_dir/${cpu}_dsp.h", genBspCpuHeader($cpu_meta, $cpu_register_headers->{$cpu}));
		push @selector, "#ifdef ".uc($cpu);
		push @selector, "#include \"${cpu}_dsp.h\" // IWYU pragma: export";
		push @selector, "#endif";
		push @selector, "";
	}
	writeFile("$output_dir/dsp.h", join("\n", @selector).$register_header);
}

sub writeFile {
	my ($file, $data) = @_;
	open my $fp, '>', $file or die "open($file): $!";
	print $fp $data;
	close $fp;
}

sub getDspModules {
	my ($cpu_meta) = @_;
	return sort { $a->{base} <=> $b->{base} || $a->{name} cmp $b->{name} } values %{$cpu_meta->dspModules()};
}

sub appendCpuMap {
	my ($header, $cpu_meta, $prefix) = @_;

	push @$header, "// ".uc($cpu_meta->{name})." TeakLite memory map";
	for my $memory (@{$cpu_meta->dspMemory()}) {
		push @$header, ["#define", $prefix.$memory->{name}."_BASE", sprintf("0x%04X", $memory->{base})];
		push @$header, ["#define", $prefix.$memory->{name}."_SIZE", sprintf("0x%04X", $memory->{size})];
	}
	push @$header, [];

	push @$header, "// ".uc($cpu_meta->{name})." TeakLite peripheral bases";
	for my $module (getDspModules($cpu_meta)) {
		push @$header, ["#define", $prefix.$module->{name}."_BASE", sprintf("0x%04X", $module->{base})];
	}
	push @$header, [];
}

sub genBspCpuHeader {
	my ($cpu_meta, $register_header) = @_;
	my @header = (
		"#pragma once",
		"// IWYU pragma: private, include \"dsp.h\"",
		"",
	);
	appendCpuMap(\@header, $cpu_meta, "TEAK_");
	$register_header =~ s/^\n//;

	my $str = printTable(\@header).$register_header;
	$str =~ s/\s+\z/\n/;
	return $str;
}

sub genQemuHeader {
	my ($cpu_metadata) = @_;
	my @header = ("#pragma once", "");
	for my $cpu_meta (@$cpu_metadata) {
		appendCpuMap(\@header, $cpu_meta, uc($cpu_meta->{name})."_TEAK_");
	}

	my $str = printTable(\@header);
	$str =~ s/\s+\z/\n/;
	return $str;
}

sub addRegisterDefine {
	my ($defines, $order, $name, $value, $descr) = @_;
	if (exists $defines->{$name}) {
		die "Conflicting TeakLite constant $name" if $defines->{$name}->{value} ne $value;
		return;
	}

	$defines->{$name} = {
		value => $value,
		descr => $descr,
	};
	push @$order, $name;
}

sub collectRegisterDefines {
	my ($defines, $order, $module, $absolute, $module_name) = @_;
	$module_name //= $module->{name};
	my $prefix = "TEAK_".$module_name."_";
	my $base = "TEAK_".$module_name."_BASE";

	for my $reg_name (getSortedKeys($module->{regs}, 'start')) {
		my $reg = $module->{regs}->{$reg_name};
		my $name = $prefix.$reg_name;
		my $value;
		if ($reg->{start} == $reg->{end}) {
			if ($absolute) {
				$value = sprintf("(%s + 0x%02X)", $base, $reg->{start});
			} else {
				$value = sprintf("0x%02X", $reg->{start});
			}
		} else {
			$name .= "(n)";
			if ($absolute) {
				$value = sprintf("(%s + 0x%02X + ((n) * 0x%X))", $base, $reg->{start}, $reg->{step});
			} else {
				$value = sprintf("(0x%02X + ((n) * 0x%X))", $reg->{start}, $reg->{step});
			}
		}
		addRegisterDefine($defines, $order, $name, $value, $reg->{descr});

		for my $field_name (getSortedKeys($reg->{fields}, 'start')) {
			my $field = $reg->{fields}->{$field_name};
			my $field_name_prepared = $reg->{field_format};
			$field_name_prepared =~ s/{reg}/$reg_name/g;
			$field_name_prepared =~ s/{field}/$field_name/g;
			my $field_prefix = $prefix.$field_name_prepared;
			addRegisterDefine($defines, $order, $field_prefix, sprintf("0x%04X", $field->{mask}), $field->{descr});
			addRegisterDefine($defines, $order, $field_prefix."_SHIFT", $field->{start}, "");

			for my $value_name (getSortedKeys($field->{values})) {
				my $value_name_prepared = $reg->{enum_format};
				$value_name_prepared =~ s/{reg}/$reg_name/g;
				$value_name_prepared =~ s/{field}/$field_name/g;
				$value_name_prepared =~ s/{value}/$value_name/g;
				my $value = $field->{values}->{$value_name} << $field->{start};
				addRegisterDefine($defines, $order, $prefix.$value_name_prepared, sprintf("0x%X", $value), "");
			}
		}
	}
}

sub renderRegisterHeader {
	my ($defines, $order, $title) = @_;
	my @header = ("", $title);
	for my $name (@$order) {
		my $define = $defines->{$name};
		push @header, "/* ".$define->{descr}." */" if $define->{descr};
		push @header, ["#define", $name, $define->{value}];
	}

	my $str = printTable(\@header);
	$str =~ s/\s+\z/\n/;
	return $str;
}

sub genRegisterHeaders {
	my ($cpu_metadata, $absolute, $qemu_output) = @_;
	my %cpu_defines;
	my %cpu_order;
	my %all_names;
	my @all_order;

	for my $cpu_meta (@$cpu_metadata) {
		my $cpu = $cpu_meta->{name};
		my %defines;
		my @order;
		my @modules = getDspModules($cpu_meta);
		for my $module (@modules) {
			collectRegisterDefines(\%defines, \@order, $module, $absolute);
		}
		if ($qemu_output) {
			my %base_name_count;
			$base_name_count{$_->{base_name}}++ for @modules;
			for my $module (@modules) {
				next if $base_name_count{$module->{base_name}} < 2;
				collectRegisterDefines(\%defines, \@order, $module, $absolute, $module->{base_name});
			}
		}
		$cpu_defines{$cpu} = \%defines;
		$cpu_order{$cpu} = \@order;
		for my $name (@order) {
			push @all_order, $name if !$all_names{$name};
			$all_names{$name} = 1;
		}
	}

	my %common_defines;
	my @common_order;
	my %specific_defines;
	my %specific_order;
	for my $name (@all_order) {
		my %values;
		for my $cpu_meta (@$cpu_metadata) {
			my $cpu = $cpu_meta->{name};
			my $define = $cpu_defines{$cpu}->{$name};
			$values{$define->{value}} = 1 if $define;
		}

		if (keys(%values) == 1) {
			for my $cpu_meta (@$cpu_metadata) {
				my $define = $cpu_defines{$cpu_meta->{name}}->{$name};
				if ($define) {
					$common_defines{$name} = $define;
					last;
				}
			}
			push @common_order, $name;
			next;
		}

		for my $cpu_meta (@$cpu_metadata) {
			my $cpu = $cpu_meta->{name};
			my $define = $cpu_defines{$cpu}->{$name};
			next if !$define;
			my $specific_name = $qemu_output ? uc($cpu)."_".$name : $name;
			$specific_defines{$cpu}->{$specific_name} = $define;
			push @{$specific_order{$cpu}}, $specific_name;
		}
	}

	my $title = $absolute ? "// TeakLite peripheral registers" : "// TeakLite peripheral register offsets";
	my $common = renderRegisterHeader(\%common_defines, \@common_order, $title);
	my %specific;
	for my $cpu_meta (@$cpu_metadata) {
		my $cpu = $cpu_meta->{name};
		my $specific_title = "// ".uc($cpu)."-specific TeakLite peripheral registers";
		$specific{$cpu} = renderRegisterHeader($specific_defines{$cpu} // {}, $specific_order{$cpu} // [],
			$specific_title);
	}

	if ($qemu_output) {
		for my $cpu_meta (@$cpu_metadata) {
			$common .= $specific{$cpu_meta->{name}};
		}
		%specific = ();
	}
	return ($common, \%specific);
}
