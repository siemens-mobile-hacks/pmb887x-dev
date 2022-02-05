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

sub printTable {
	my ($table) = @_;
	
	my $out = '';
	
	my $max_col_width = {};
	
	for my $row (@$table) {
		my $col_n = 0;
		
		if (ref($row) eq 'ARRAY') {
			for my $col (@$row) {
				$max_col_width->{$col_n} = max($max_col_width->{$col_n} || 0, length($col));
				$col_n++;
			}
		}
	}
	
	my $tab_size = 4;
	
	for my $row (@$table) {
		my $col_n = 0;
		if (ref($row) eq 'ARRAY') {
			for my $col (@$row) {
				my $max_width = ceil(($max_col_width->{$col_n} + 1) / $tab_size);
				my $cur_width = ceil(length($col) / $tab_size);
				
				my $need_tabs = ($max_width - $cur_width);
				$need_tabs++ if (length($col) % $tab_size != 0);
				
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

1;
