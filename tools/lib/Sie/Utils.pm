package Sie::Utils;

use warnings;
use strict;
use File::Basename;
use base 'Exporter';
use List::Util qw(min max);
use POSIX qw(ceil);

our @EXPORT = qw|getDataDir parseAnyInt getSortedKeys printTable|;

sub getDataDir {
	return dirname(__FILE__).'/../../../lib/data';
}

sub parseAnyInt {
	if ($_[0] =~ /^0b[01]+$/i) {
		return eval(lc($_[0]));
	} elsif ($_[0] =~ /^0x[A-F0-9]+$/i) {
		return hex($_[0]);
	}
	return int($_[0]);
}

sub getSortedKeys {
	my ($hash, $key) = @_;
	if (defined $key) {
		return sort { $hash->{$a}->{$key} <=> $hash->{$b}->{$key} } keys %$hash;
	} else {
		return sort { $hash->{$a} <=> $hash->{$b} } keys %$hash;
	}
}

sub countSymbols {
	my ($str) = @_;
	$str = "$str";
	$str =~ s/\t/..../g;
	return length($str);
}

sub printTable {
	my ($table, $before, $after) = @_;
	
	my $out = '';
	
	my $max_col_width = {};
	
	for my $row (@$table) {
		my $col_n = 0;
		
		if (ref($row) eq 'ARRAY') {
			for (my $col_n = 0; $col_n < scalar(@$row); $col_n++) {
				my $col = $row->[$col_n];
				$col = ($before || "").$col if $col_n == 0;
				$col = $col.($after || "") if $col_n == scalar(@$row) - 1;
				$max_col_width->{$col_n} = max($max_col_width->{$col_n} || 0, countSymbols($col));
				$row->[$col_n] = $col;
			}
		}
	}
	
	my $tab_size = 4;
	
	for my $row (@$table) {
		my $col_n = 0;
		if (ref($row) eq 'ARRAY') {
			for my $col (@$row) {
				my $max_width = ceil(($max_col_width->{$col_n} + 1) / $tab_size);
				my $cur_width = ceil(countSymbols($col) / $tab_size);
				
				my $need_tabs = ($max_width - $cur_width);
				$need_tabs++ if (countSymbols($col) % $tab_size != 0);
				
				$out .= $col.("\t" x $need_tabs);
				
				$col_n++;
			}
		} else {
			$out .= $row;
		}
		$out .= "\n";
	}
	
	$out =~ s/[ \t]$//gm;
	
	return $out;
}

sub getArgvOpts {
	my ($cfg) = @_;
	
	my $args = {};
	for my $k (keys %$cfg) {
		my $arg = {ref => $cfg->{$k}};
		if ($k =~ /^@(.*?)$/) {
			$k = $1;
			$arg->{array} = 1;
		}
		
		if ($k =~ /^([^=]+)=(.*?)$/) {
			$k = $1;
			$arg->{with_value} = $2;
		}
		
		$args->{$k} = $arg;
	}
	
	for (my $opt_id = 0; $opt_id < scalar(@ARGV); ++$opt_id) {
		my $opt = $ARGV[$opt_id];
		my $opt_name;
		my $opt_value;
		
		if ($opt =~ /^--([^=]+)=(.*?)$/) {
			$opt_name = $1;
			$opt_value = $2;
		} elsif ($opt =~ /^--([^=]+)$/) {
			$opt_name = $1;
		}
		
		if (exists $args->{$opt_name}) {
			my $arg = $args->{$opt_name};
			
			if ($arg->{with_value}) {
				if (!defined($opt_value)) {
					++$opt_id;
					$opt_value = $ARGV[$opt_id] if (exists $ARGV[$opt_id]);
				}
				return "Argument $opt require value\n" if (!defined($opt_value));
				
				if ($arg->{with_value} eq "b") {
					if ($opt_value eq "true") {
						$opt_value = 1;
					} elsif ($opt_value eq "false") {
						$opt_value = 0;
					} else {
						$opt_value = int($opt_value) ? 1 : 0;
					}
				} elsif ($arg->{with_value} eq "i") {
					if ($opt_value =~ /0x([a-f0-9]+)/) {
						$opt_value = hex($1);
					} else {
						$opt_value = int($opt_value);
					}
				}
			} else {
				$opt_value = 1;
			}
			
			if ($arg->{array}) {
				push @{$arg->{ref}}, $opt_value;
			} else {
				${$arg->{ref}} = $opt_value;
			}
		} else {
			return "Unknown option: $opt\n";
		}
	}
	
	return;
}

1;
