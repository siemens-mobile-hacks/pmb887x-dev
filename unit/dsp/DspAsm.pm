package DspAsm;

use strict;
use warnings;
use Exporter qw(import);
use File::Basename qw(dirname);
use lib dirname(__FILE__).'/../../tools/lib';
use Sie::CpuMetadata;
use Sie::Utils;

our @EXPORT_OK = qw(expand_teak_constants);

my %cpu_constants;

sub add_constant {
	my ($constants, $name, $value) = @_;
	die "Conflicting TeakLite constant $name\n" if exists $constants->{$name} && $constants->{$name} != $value;
	$constants->{$name} = $value;
}

sub get_constants {
	my ($cpu) = @_;
	return $cpu_constants{$cpu} if exists $cpu_constants{$cpu};

	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	my %constants;
	for my $memory (@{$cpu_meta->dspMemory()}) {
		add_constant(\%constants, "TEAK_".$memory->{name}."_BASE", $memory->{base});
		add_constant(\%constants, "TEAK_".$memory->{name}."_SIZE", $memory->{size});
	}
	for my $module (values %{$cpu_meta->dspModules()}) {
		my $prefix = "TEAK_".$module->{name}."_";
		add_constant(\%constants, $prefix."BASE", $module->{base});
		for my $reg (values %{$module->{regs}}) {
			die "TeakLite register ranges are not supported in DSP assembly: $reg->{name}\n"
				if $reg->{start} != $reg->{end};
			add_constant(\%constants, $prefix.$reg->{name}, $module->{base} + $reg->{start});
			for my $field (values %{$reg->{fields}}) {
				my $field_name = $reg->{field_format};
				$field_name =~ s/{reg}/$reg->{name}/g;
				$field_name =~ s/{field}/$field->{name}/g;
				add_constant(\%constants, $prefix.$field_name, $field->{mask});
				add_constant(\%constants, $prefix.$field_name."_SHIFT", $field->{start});
				for my $value_name (keys %{$field->{values}}) {
					my $enum_name = $reg->{enum_format};
					$enum_name =~ s/{reg}/$reg->{name}/g;
					$enum_name =~ s/{field}/$field->{name}/g;
					$enum_name =~ s/{value}/$value_name/g;
					add_constant(\%constants, $prefix.$enum_name,
						$field->{values}->{$value_name} << $field->{start});
				}
			}
		}
	}

	$cpu_constants{$cpu} = \%constants;
	return $cpu_constants{$cpu};
}

sub expand_teak_constants {
	my ($cpu, $source) = @_;
	my $constants = get_constants($cpu);

	$source =~ s{\bTEAK_ADDR\(\s*(TEAK_[A-Z0-9_]+)\s*,\s*(0x[0-9A-Fa-f]+|[0-9]+)\s*\)}{
		my ($name, $offset_text) = ($1, $2);
		my $offset = parseAnyInt($offset_text);
		die "Unknown TeakLite constant $name\n" if !exists $constants->{$name};
		my $address = $constants->{$name} + $offset;
		die "TeakLite address overflow: $name + $offset\n" if $address > 0xFFFF;
		sprintf("%04X", $address);
	}gex;
	$source =~ s{\b(TEAK_[A-Z0-9_]+)\b}{
		my $name = $1;
		die "Unknown TeakLite constant $name\n" if !exists $constants->{$name};
		sprintf("%04X", $constants->{$name});
	}gex;

	return $source;
}

1;
