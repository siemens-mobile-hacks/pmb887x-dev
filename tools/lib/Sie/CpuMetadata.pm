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
		gpios => {},
		dma => {},
	} => $class;
	
	$self->{common} = $self->loadCommonRegs();
	
	$self->loadCPU($cpu);
	
	return $self;
}

sub gpios {
	my ($self) = @_;
	return $self->{gpios};
}

sub getCpus {
	my $path = getDataDir();
	opendir my $fp, $path or die "opendir($path): $!";
	my @files = readdir $fp;
	closedir $fp;
	
	my $cpus = [];
	for my $file (@files) {
		next if !-f $path."/".$file;
		next if $file !~ /^pmb.*?\.cfg$/i;
		$file =~ s/\.cfg$//gi;
		push @$cpus, $file;
	}
	return $cpus;
}

sub setGpios {
	my ($self, $gpio_map) = @_;
	
	for my $gpio_cpu (keys %$gpio_map) {
		my $gpio_board = $gpio_map->{$gpio_cpu};
		
		next if $gpio_board eq $gpio_cpu;
		
		if (exists $self->{gpios}->{$gpio_cpu}) {
			$self->{gpios}->{$gpio_cpu}->{alias} = $gpio_board->{name};
			$self->{gpios}->{$gpio_cpu}->{mode} = $gpio_board->{mode};
			$self->{gpios}->{$gpio_cpu}->{value} = $gpio_board->{value};
		}
	}
}

sub buildRegIndex {
	my ($self) = @_;
	
	$self->{id2irq} = {};
	$self->{addr2reg} = {};
	
	for my $module (values %{$self->{modules}}) {
		my $alt_names = {};
		if ($module->{name} eq "GPIO") {
			for my $gpio_name (keys %{$self->gpios()}) {
				my $gpio = $self->gpios()->{$gpio_name};
				my $gpio_addr = $module->{regs}->{PIN}->{start} + ($gpio->{id} * $module->{regs}->{PIN}->{step});
				$alt_names->{$gpio_addr} = "GPIO_PIN".$gpio->{id}."_".($gpio->{alias} ? $gpio->{name}."_".$gpio->{alias} : $gpio->{name});
			}
		}
		
		if ($module->{name} eq "VIC") {
			for my $id (@{$self->getModuleNames()}) {
				my $other_module = $self->{modules}->{$id};
				for my $irq_name (keys %{$other_module->{irqs}}) {
					my $irq_num = $other_module->{irqs}->{$irq_name};
					my $irq_con_addr = $module->{regs}->{CON}->{start} + ($irq_num * $module->{regs}->{CON}->{step});
					$alt_names->{$irq_con_addr} = "IRQ_CON".$irq_num."_".$other_module->{name}.($irq_name ? "_".$irq_name : "");
				}
			}
		}
		
		for my $irq_name (keys %{$module->{irqs}}) {
			$self->{id2irq}->{$module->{irqs}->{$irq_name}} = $module->{name}.($irq_name ? "_".$irq_name : "");
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
			for my $field (sort { $a->{start} <=> $b->{start} } values %{$reg->{fields}}) {
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
				for (my $i = 0; $i < 32; $i++) {
					if (($unknown & (1 << $i))) {
						push @bitmap, "UNK_$i";
					}
				}
			}
		} elsif ($reg->{name} eq "VIC_IRQ_CURRENT" || $reg->{name} eq "VIC_FIQ_CURRENT") {
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
		
		if ($line =~ /^\.dma/) {
			my ($key, $module, $channel, $bus, $request, $sel) = split("\t", $line);
			push @{$self->{dma}->{$module}}, {
				channel	=> $channel,
				bus		=> $bus,
				request	=> parseAnyInt($request),
				sel	=> parseAnyInt($sel)
			};
		} elsif ($line =~ /^\.gpio/) {
			my ($key, $name, $id, $alt_list) = split("\t", $line);
			$self->{gpios}->{$name} = {
				id		=> parseAnyInt($id),
				name	=> $name,
				alias	=> undef,
				mode	=> "none",
				value	=> undef,
				alt		=> [{id => 0, name => "GPIO", flags => "IO"}]
			};
			
			if ($alt_list) {
				for my $alt (split(/\s*,\s*/, $alt_list)) {
					if ($alt =~ /^ALT(\d+)_(I|O|IO)S=(.*?)$/i) {
						push @{$self->{gpios}->{$name}->{alt}}, {
							id		=> int($1) + 1,
							name	=> $3,
							flags	=> $2
						};
					} else {
						die "Invalid ALT: $alt";
					}
				}
			}
		} else {
			my ($name, $addr, $type, $id, $irqs) = split("\t", $line);
			
			push @{$self->{available_modules}}, {
				id		=> parseAnyInt($id),
				type	=> $type,
				base	=> parseAnyInt($addr),
				name	=> $name,
				irqs	=> $irqs ? [split(/\s*,\s*/, $irqs)] : []
			};
		}
	}
	close $fp;
	
	$self->loadModules();
}

sub findModuleDef {
	my ($self, $def, $module) = @_;
	
	return 0 if $def->{type} ne $module->{type};
	
	if ($def->{type} eq 'MODULE') {
		return 1 if grep { $def->{id} == $_ } @{$module->{ids}};
		return 0;
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
		
		for my $id (@{$module->{ids}}) {
			die sprintf("Module %s:%08X already defined", $module->{type}, $id)
				if exists $uniq_modules_ids->{$module->{type}.":$id"};
			$uniq_modules_ids->{$module->{type}.":".$id} = 1;
		}
		
		for my $def (@{$self->{available_modules}}) {
			if ($self->findModuleDef($def, $module)) {
				my $new_module = dclone($module);
				$new_module->{id} = $def->{id};
				$new_module->{base_name} = $new_module->{name};
				$new_module->{base} = $def->{base};
				$new_module->{name} = $def->{name};
				
				if (@{$def->{irqs}} && $def->{irqs}->[0] ne "-") {
					if (scalar(@{$def->{irqs}}) != scalar(@{$new_module->{irqs_needed}})) {
						die sprintf("Module %s required %d irqs, but specified %d", $def->{name}, scalar(@{$new_module->{irqs_needed}}), scalar(@{$def->{irqs}}));
					}
					
					my $irq_num = 0;
					for my $irq (@{$def->{irqs}}) {
						$new_module->{irqs}->{$new_module->{irqs_needed}->[$irq_num]} = $def->{irqs}->[$irq_num];
						$irq_num++;
					}
				}

				$self->{modules}->{$def->{name}} = $new_module;
			}
		}
	}
}

sub loadCommonRegs {
	my ($self) = @_;
	my $tmp_module = $self->parseModule(getDataDir()."/common_regs.cfg", 1);
	return $tmp_module->{regs};
}

sub parseModule {
	my ($self, $file, $is_common_data) = @_;
	
	my $module = {
		ids			=> [],
		type		=> 'MODULE',
		qemu		=> undef,
		regs		=> {},
		size		=> 0,
		irqs		=> {},
		irqs_needed	=> [],
		gpio_lines	=> [],
	};
	
	my $current_field;
	my $current_reg;
	
	my $default_enum_format = '{reg}_{field}_{value}';
	my $default_field_format = '{reg}_{field}';
	
	my $current_enum_format = $default_enum_format;
	my $current_field_format = $default_field_format;
	
	my $n = 0;
	
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
				
				die "Unexpected .$key in common data" if $is_common_data;
				
				if ($key eq 'field_format') {
					$current_field_format = $value;
				} elsif ($key eq 'enum_format') {
					$current_enum_format = $value;
				} elsif ($key eq 'irq') {
					push @{$module->{irqs_needed}}, $value || "";
				} elsif ($key eq 'gpio') {
					push @{$module->{gpio_lines}}, $value || "";
				} else {
					if ($key eq "type" || $key eq "name" || $key eq "descr" || $key eq "qemu") {
						$module->{$key} = $value;
					} elsif ($key eq "id") {
						$module->{ids} = [map { parseAnyInt($_) } split(/\s*,\s*/, $value)];
					} elsif ($key eq "size" || $key eq "multi") {
						$module->{$key} = parseAnyInt($value);
					} else {
						die("Invalid: '$line'");
					}
				}
			} elsif ($line !~ /^\./) {
				my ($name, $addr, $step, $descr);
				my ($start, $end);
				
				if ($is_common_data) {
					($name, $descr) = split("\t", $line);
					$start = $n++;
				} else {
					($name, $addr, $step, $descr) = split("\t", $line);
					($start, $end) = split("-", $addr);
					
					if (!$end) {
						$descr = $step;
						$step = undef;
					}
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
			
			if ($name =~ /^\*(.*?)$/) {
				my $ref = $1;
				
				die "Unknown reference to common_regs.cfg:$name" if !exists $self->{common}->{$ref};
				
				for my $k (keys %{$self->{common}->{$ref}->{fields}}) {
					$current_reg->{fields}->{$k} = dclone($self->{common}->{$ref}->{fields}->{$k});
				}
				
				$current_reg->{common} = $ref;
				
				$current_reg->{descr} = $self->{common}->{$ref}->{descr} if !$current_reg->{descr};
				
				next;
			}
			
			$current_field = {
				name		=> $name,
				start		=> parseAnyInt($start),
				size		=> parseAnyInt($size),
				descr		=> $descr || "",
				values		=> {},
				value2name	=> {},
				value2descr	=> {},
			};
			
			$current_field->{mask} = ((1 << $current_field->{size}) - 1) << $current_field->{start};
			
			$current_reg->{fields}->{$name} = $current_field;
		} elsif ($level == 2) {
			my ($enum, $descr) = split("\t", $line);
			my ($name, $value) = split("=", $enum);
			
			die("Invalid: '$line'") if !$current_field;
			
			$current_field->{values}->{$name} = parseAnyInt($value);
			$current_field->{value2name}->{parseAnyInt($value)} = $name;
			$current_field->{value2descr}->{parseAnyInt($value)} = $descr;
		} else {
			die("Invalid: '$line' [level=$level]");
		}
	}
	close $fp;
	
	if ($module->{type} ne "MODULE") {
		die Dumper($module) if scalar(@{$module->{ids}}) != 1;
		$module->{id} = $module->{ids}->[0];
	}

	return $module;
}

1;
