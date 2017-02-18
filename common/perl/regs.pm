use warnings;
use strict;
use File::Basename qw|dirname|;
use Data::Dumper;

my $pmb8876_regs;

sub get_regs {
	if (!$pmb8876_regs) {
		$pmb8876_regs = [];
		
		my $last_reg;
		my $last_reg_field;
		
		open FFF, dirname(__FILE__)."/../../regs.txt";
		while (my $line = <FFF>) {
			next if ($line =~ /^\s*#/); # вся линия - комментарий
			
			if ($line =~ /^\t\t/) { # Описание значений регистра
				$line =~ s/^\s+|\s+$//g;
				my @values = split(/[\t]+/, $line);
				
				if ($values[0] =~ /^0b[01]+$/i) {
					$values[0] = eval(lc($values[0]));
				} elsif ($values[0] =~ /^0x[A-F0-9]+$/i) {
					$values[0] = hex($values[0]);
				} else {
					$values[0] = int($values[0]);
				}
				
				$last_reg_field->{values}->{$values[0]} = $values[1];
			} elsif ($line =~ /^\t/) { # Описание последнего регистра
				$line =~ s/^\s+|\s+$//g;
				
				my @values = split(/[\t]+/, $line);
				if (scalar(@values) >= 3) {
					my $val = ((1 << $values[2]) - 1) << $values[1];
					
					$last_reg->{bits} |= $val;
					
					my $v = {shift => int($values[1]), mask => int($values[2]), name => $values[0], values => {}};
					for my $v2 (@{$last_reg->{desc}}) {
						my $val1 = ((1 << $v->{mask}) - 1) << $v->{shift};
						my $val2 = ((1 << $v2->{mask}) - 1) << $v2->{shift};
						
						if ($val1 & $val2) {
							warn "WARN: bit value ".$v->{name}." overlaps some bits in ".$v2->{name}." for ".$last_reg->{name}." !\n";
						}
					}
					
					$last_reg_field = $v;
					
					push @{$last_reg->{desc}}, $v;
				} else {
					die "Invalid descr for ".$last_reg->{name}.": '$line'\n";
				}
			} else { # Адрес регистра
				$line =~ s/^\s+|\s+$//g;
				
				my @values = split(/[\t]+/, $line);
				if (scalar(@values) >= 2) {
					my $val = [$values[0]];
					if ($values[1] =~ /0x([a-f0-9]+)-0x([a-f0-9]+)/i) {
						push @$pmb8876_regs, {
							name => $values[0], 
							addr => hex $1, 
							addr_end => hex $2, 
							bits => 0, 
							desc => []
						};
					} elsif ($values[1] =~ /0x([a-f0-9]+)/i) {
						push @$pmb8876_regs, {
							name => $values[0], 
							addr => hex $1, 
							bits => 0, 
							desc => []
						};
					}
					$last_reg = $pmb8876_regs->[scalar(@$pmb8876_regs) - 1];
				}
			}
		}
		close FFF;
	}
	
	return $pmb8876_regs;
}

sub reg_name {
	my $addr = shift;
	my $value = shift;
	my $names = get_regs();
	
	my $reg_def = "";
	my $def = "";
	for my $v (@$names) {
		if ($v->{addr} == $addr && !$v->{addr_end}) {
			$reg_def = $v;
			last;
		} elsif ($v->{addr_end} && $v->{addr} <= $addr && $addr <= $v->{addr_end}) {
			$reg_def = $v;
		}
	}
	
	return "" if (!$reg_def);
	
	my $add = "";
	if ($addr >= 0xF4300020 && $addr <= 0xF43001E4) {
		my $IS		= $value & 7;
		my $OS		= ($value >> 4) & 7;
		my $PS		= ($value >> 8) & 1;
		my $DATA	= ($value >> 9) & 1;
		my $DIR		= ($value >> 10) & 1;
		my $PPEN	= ($value >> 12) & 1;
		my $PDPU	= ($value >> 13) & 3;
		my $ENAQ	= ($value >> 15) & 1;
		
		my @gpio = ();
		push @gpio, "PS=$PS";
		if ($IS) {
			push @gpio, "IS=ALT".($IS - 1);
		}
		if ($OS) {
			push @gpio, "OS=ALT".($OS - 1);
		}
		push @gpio, "DATA=$DATA";
		push @gpio, "DIR=".(!$DIR ? "IN" : "OUT");
		if ($PPEN) {
			push @gpio, "PPEN=1";
		}
		if ($PDPU) {
			push @gpio, "PDPU=".($PDPU == 2 ? "DOWN" : ($PDPU == 1 ? "UP" : "$PDPU"));
		}
		if ($ENAQ) {
			push @gpio, "ENAQ=1";
		}
		$add = " [".join("; ", @gpio)."]";
	} elsif ($reg_def) {
		my $known = 0;
		my $values = [];
		for my $v (@{$reg_def->{desc}}) {
			my $mask = ((1 << $v->{mask}) - 1);
			my $val = ($value >> $v->{shift}) & $mask;
			$known |= $mask << $v->{shift};
			
			if ($v->{mask} == 1) {
				if ($val) {
					push @$values, $v->{name};
				}
			} else {
				push @$values, $v->{name}."(0x".sprintf("%02X", $val).")".($v->{values}->{$val} ? '='.$v->{values}->{$val} : "");
			}
		}
		
		if ($known && ($value & ~$known)) {
			push @$values, "UNKNOWN(".sprintf("0b%08b", $value & ~$known).")";
		}
		
		if (@$values) {
			$add = ": ".join(" | ", @$values);
		}
	}
	
	return " (".$reg_def->{name}.")".$add;
}

1;