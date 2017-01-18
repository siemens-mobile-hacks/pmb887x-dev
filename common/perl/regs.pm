use warnings;
use strict;
use File::Basename qw|dirname|;
use Data::Dumper;

my $pmb8876_regs;

sub get_regs {
	if (!$pmb8876_regs) {
		$pmb8876_regs = [];
		
		open FFF, dirname(__FILE__)."/../../regs.txt";
		while (my $line = <FFF>) {
			next if ($line =~ /^\s*#/); # вся линия - комментарий
			$line =~ s/^\s+|\s+$//g;
			
			my @values = split(/[\t]+/, $line);
			if (scalar(@values) >= 2) {
				my $val = [$values[0]];
				if ($values[1] =~ /0x([a-f0-9]+)-0x([a-f0-9]+)/i) {
					push @$pmb8876_regs, [$values[0], hex $1, hex $2];
				} elsif ($values[1] =~ /0x([a-f0-9]+)/i) {
					push @$pmb8876_regs, [$values[0], hex $1];
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
	
	my $add = "";
	if ($addr >= 0xF4300020 && $addr <= 0xF43001E4) {
		my $IS		= $value & 0x7;
		my $OS		= ($value >> 3) & 0x7;
		my $PS		= ($value >> 7) & 1;
		my $DATA	= ($value >> 8) & 1;
		my $DIR		= ($value >> 9) & 1;
		my $PPEN	= ($value >> 11) & 1;
		my $PDPU	= ($value >> 12) & 3;
		my $ENAQ	= ($value >> 14) & 1;
		
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
	}
	
	my $def = "";
	for my $v (@$names) {
		if ($v->[1] == $addr && !$v->[2]) {
			return " (".$v->[0].")".$add;
		} elsif ($v->[2] && $v->[1] <= $addr && $addr <= $v->[2]) {
			$def = " (".$v->[0].")".$add;
		}
	}
	return $def;
}

1;