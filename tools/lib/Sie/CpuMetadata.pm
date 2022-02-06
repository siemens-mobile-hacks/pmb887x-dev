package Sie::CpuMetadata;

use warnings;
use strict;
use File::Basename;
use Data::Dumper;
use Storable qw(dclone);
use List::Util qw(min max);
use Sie::Utils;

sub new {
	my ($class, $cpu) = @_;
	
	my $self = bless {
		modules => {},
		gpios => {}
	} => $class;
	
	$self->loadCPU($cpu);
	
	return $self;
}

sub gpios {
	my ($self) = @_;
	return $self->{gpios};
}

sub setGpios {
	my ($self, $gpios) = @_;
	$self->{gpios} = $gpios;
}

sub buildRegIndex {
	my ($self) = @_;
	
	$self->{id2irq} = {};
	$self->{addr2reg} = {};
	
	for my $module (values %{$self->{modules}}) {
		my $alt_names = {};
		if ($module->{name} eq "GPIO") {
			for my $gpio_name (keys %{$self->gpios()}) {
				my $gpio_num = $self->gpios()->{$gpio_name};
				my $gpio_addr = $module->{regs}->{PIN}->{start} + ($gpio_num * $module->{regs}->{PIN}->{step});
				$alt_names->{$gpio_addr} = "GPIO_PIN".$gpio_num."_".$gpio_name;
			}
		}
		
		for my $irq_name (keys %{$module->{irqs}}) {
			$self->{id2irq}->{$module->{irqs}->{$irq_name}} = $module->{name}."_".$irq_name;
		}
		
		for my $reg (values %{$module->{regs}}) {
			if ($reg->{start} != $reg->{end}) {
				my $index = 0;
				for (my $i = $reg->{start}; $i <= $reg->{end}; $i += $reg->{step}) {
					my $reg_name = sprintf("%s_%s%d", $module->{name}, $reg->{name}, $index);
					$reg_name = $alt_names->{$i} if (exists $alt_names->{$i});
					
					$self->{addr2reg}->{$module->{base} + $i} = {
						name		=> $reg_name,
						fields		=> $reg->{fields}
					};
					$index++;
				}
			} else {
				$self->{addr2reg}->{$module->{base} + $reg->{start}} = {
					name		=> sprintf("%s_%s", $module->{name}, $reg->{name}),
					fields		=> $reg->{fields}
				};
			}
		}
	}
}

sub dumpReg {
	my ($self, $addr, $value) = @_;
	
	$self->buildRegIndex() if !$self->{addr2reg};
	
	if (exists $self->{addr2reg}->{$addr}) {
		my $reg = $self->{addr2reg}->{$addr};
		
		my @bitmap;
		
		if (%{$reg->{fields}}) {
			my $known = 0;
			for my $field (values %{$reg->{fields}}) {
				$known |= $field->{mask};
				
				my $val = ($value & $field->{mask}) >> $field->{start};
				if ($field->{size} == 1 && !exists $field->{value2name}->{$val}) {
					if (($value & $field->{mask})) {
						push @bitmap, $field->{name};
					}
				} else {
					if (exists $field->{value2name}->{$val}) {
						push @bitmap, $field->{name}."(".$field->{value2name}->{$val}.")";
					} else {
						push @bitmap, $field->{name}."(".sprintf("0x%02X", $val).")";
					}
				}
			}
			
			my $unknown = ($value & ~$known);
			if ($unknown) {
				push @bitmap, "UNKNOWN(".sprintf("0b%08b", $unknown).")";
			}
		} elsif ($reg->{name} eq "NVIC_CURRENT_IRQ" || $reg->{name} eq "NVIC_CURRENT_FIQ") {
			if (exists $self->{id2irq}->{$value}) {
				push @bitmap, "NUM(".sprintf("0x%02X", $value).")=".$self->{id2irq}->{$value};
			} else {
				push @bitmap, "NUM(".sprintf("0x%02X", $value).")";
			}
		}
		
		return "(".$reg->{name}.")".(@bitmap ? ": ".join(" | ", @bitmap) : "");
	} else {
		for my $module (values %{$self->{modules}}) {
			if ($addr >= $module->{base} && $addr <= $module->{base} + $module->{size}) {
				return "(".$module->{name}."_*)";
				last;
			}
		}
	}
	
	return undef;
}

sub loadCPU {
	my ($self, $cpu) = @_;
	
	if ($cpu eq "generic") {
		$self->{name} = $cpu;
		$self->{available_modules} = [];
		return;
	}
	
	my $file = getDataDir().'/'.$cpu.'.cfg';
	
	$self->{name} = $cpu;
	$self->{available_modules} = [];
	
	open my $fp, "<$file" or die("open($file): $!");
	while (my $line = <$fp>) {
		next if $line =~ /^(\/\/|#)/;
		
		# Replace comments
		$line =~ s/\/\*.*?\*\///sig;
		
		# Normalize spaces
		$line =~ s/[\t]+/\t/g;
		$line =~ s/^\s+|\s+$//g;
		
		next if !length($line);
		
		my ($name, $addr, $type, $id, $irqs) = split("\t", $line);
		
		push @{$self->{available_modules}}, {
			id		=> parseAnyInt($id),
			type	=> $type,
			base	=> parseAnyInt($addr),
			name	=> $name,
			irqs	=> $irqs ? [split(/\s*,\s*/, $irqs)] : []
		};
	}
	close $fp;
	
	$self->loadModules();
}

sub findModuleDef {
	my ($self, $def, $module) = @_;
	
	return 0 if $def->{type} ne $module->{type};
	
	if ($def->{type} eq 'MODULE') {
		return ($def->{id} & ~0xFF) == $module->{id};
	} elsif ($def->{type} eq 'AMBA') {
		return ($def->{id} & 0xFFF) == $module->{id};
	} elsif ($def->{type} eq 'NATIVE') {
		return $def->{id} == $module->{id};
	} else {
		die "Invalid type: ".$def->{type};
	}
	
	return 0;
}

sub getModuleNames {
	my ($self) = @_;
	return [sort { $self->{modules}->{$a}->{base} <=> $self->{modules}->{$b}->{base} } keys %{$self->{modules}}];
}

sub getAllModules {
	my ($self) = @_;
	
	my $path = getDataDir().'/modules';
	opendir my $fp, $path or die "opendir($path): $!";
	my @files = readdir $fp;
	closedir $fp;
	
	my $modules = [];
	for my $file (sort @files) {
		next if !-f "$path/$file";
		
		my $module = $self->parseModule("$path/$file");
		push @$modules, $module;
	}
	
	return $modules;
}

sub loadModules {
	my ($self) = @_;
	
	return if ($self->{name} eq "generic");
	
	my $path = getDataDir().'/modules';
	opendir my $fp, $path or die "opendir($path): $!";
	my @files = readdir $fp;
	closedir $fp;
	
	my $uniq_modules_ids = {};
	
	for my $file (@files) {
		next if !-f "$path/$file";
		
		my $module = $self->parseModule("$path/$file");
		
		die sprintf("Module %s:%08X already defined", $module->{type}, $module->{id})
			if exists $uniq_modules_ids->{$module->{type}.":".$module->{id}};
		
		$uniq_modules_ids->{$module->{type}.":".$module->{id}} = 1;
		
		for my $def (@{$self->{available_modules}}) {
			if ($self->findModuleDef($def, $module)) {
				my $new_module = dclone($module);
				
				$new_module->{base_name} = $new_module->{name};
				$new_module->{base} = $def->{base};
				$new_module->{name} = $def->{name};
				
				if (scalar(@{$def->{irqs}}) != scalar(@{$new_module->{irqs_needed}})) {
					die sprintf("Module %s required %d irqs, but specified %d", $def->{name}, scalar(@{$new_module->{irqs_needed}}), scalar(@{$def->{irqs}}));
				}
				
				my $irq_num = 0;
				for my $irq (@{$def->{irqs}}) {
					$new_module->{irqs}->{$new_module->{irqs_needed}->[$irq_num]} = $def->{irqs}->[$irq_num];
					$irq_num++;
				}
				
				$self->{modules}->{$def->{name}} = $new_module;
			}
		}
	}
}

sub parseModule {
	my ($self, $file) = @_;
	
	my $module = {
		id			=> 0,
		type		=> 'MODULE',
		regs		=> {},
		size		=> 0,
		irqs		=> {},
		irqs_needed	=> []
	};
	
	my $current_field;
	my $current_reg;
	
	my $default_enum_format = '{reg}_{field}_{value}';
	my $default_field_format = '{reg}_{field}';
	
	my $current_enum_format = $default_enum_format;
	my $current_field_format = $default_field_format;
	
	open my $fp, "<$file" or die("open($file): $!");
	while (my $line = <$fp>) {
		next if $line =~ /^(\/\/|#)/;
		
		# Replace comments
		$line =~ s/\/\*.*?\*\///sig;
		
		my $level = -1;
		$level = length($1) if ($line =~ /^([\t]*)/);
		
		# Normalize spaces
		$line =~ s/[\t]+/\t/g;
		$line =~ s/^\s+|\s+$//g;
		
		next if !length($line);
		
		if ($level == 0) {
			if ($line =~ /^\.([\w\d_-]+)\s*(.*?)?$/i) {
				my $key = $1;
				my $value = $2;
				
				if ($key eq 'field_format') {
					$current_field_format = $value;
				} elsif ($key eq 'enum_format') {
					$current_enum_format = $value;
				} elsif ($key eq 'irq') {
					push @{$module->{irqs_needed}}, $value || "";
				} else {
					if ($key eq "type" || $key eq "name") {
						$module->{$key} = $value;
					} elsif ($key eq "id" || $key eq "size" || $key eq "multi") {
						$module->{$key} = parseAnyInt($value);
					} else {
						die("Invalid: '$line'");
					}
				}
			} elsif ($line !~ /^\./) {
				my ($name, $addr, $step, $descr) = split("\t", $line);
				
				my ($start, $end) = split("-", $addr);
				
				if (!$end) {
					$descr = $step;
					$step = undef;
				}
				
				$current_reg = {
					name			=> $name,
					start			=> parseAnyInt($start),
					end				=> parseAnyInt($end || $start),
					step			=> $end ? parseAnyInt($step || 4) : 0,
					descr			=> $descr,
					fields			=> {},
					enum_format		=> $current_enum_format,
					field_format	=> $current_field_format
				};
				
				$module->{size} = max($module->{size}, $current_reg->{end} + 4);
				
				$module->{regs}->{$name} = $current_reg;
				
				$current_enum_format = $default_enum_format;
				$current_field_format = $default_field_format;
			} else {
				die("Invalid: '$line'");
			}
		} elsif ($level == 1) {
			my ($name, $start, $size, $descr) = split("\t", $line);
			
			die("Invalid: '$line'") if !$current_reg;
			
			$current_field = {
				name		=> $name,
				start		=> parseAnyInt($start),
				size		=> parseAnyInt($size),
				descr		=> $descr || "",
				values		=> {},
				value2name	=> {}
			};
			
			$current_field->{mask} = ((1 << $current_field->{size}) - 1) << $current_field->{start};
			
			$current_reg->{fields}->{$name} = $current_field;
		} elsif ($level == 2) {
			my ($name, $value) = split("=", $line);
			
			die("Invalid: '$line'") if !$current_field;
			
			$current_field->{values}->{$name} = parseAnyInt($value);
			$current_field->{value2name}->{parseAnyInt($value)} = $name;
		} else {
			die("Invalid: '$line' [level=$level]");
		}
	}
	close $fp;
	
	return $module;
}

1;
