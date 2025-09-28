package Sie::Utils;

use warnings;
use strict;
use File::Basename;
use base 'Exporter';
use File::Slurp qw(read_file);
use List::Util qw(min max);
use POSIX qw(ceil);

our @EXPORT = qw|getDataDir parseAnyInt getSortedKeys printTable bin2hex hex2bin parseIniFile hexdump|;

sub bin2hex($) {
	my ($text) = @_;
	$text =~ s/(.)/sprintf("%02X", ord($1))/ges;
	return $text;
}

sub hex2bin($) {
	my ($text) = @_;
	
	$text = "0$text" if (length($text) % 2 != 0);
	
	die "Input string must be hexadecimal string" if (!defined $text || $text !~ /^([a-f0-9]+)$/i);
	
	$text =~ s/(..)/chr(hex($1))/ges;
	
	return $text;
}

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
		return sort { $hash->{$a}->{$key} <=> $hash->{$b}->{$key} || $a cmp $b } keys %$hash;
	} else {
		return sort { $hash->{$a} <=> $hash->{$b} || $a cmp $b } keys %$hash;
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
	
	$out =~ s/[ \t]+$//gm;
	
	return $out;
}

sub parseIniFile {
	my ($file) = @_;
	my $hash = {};
	my $ref;
	
	my $ini = scalar(read_file($file));
	
	my $escapes = {
		n		=> "\n",
		r		=> "\r",
		t		=> "\t",
		"'"		=> "'",
		'"'		=> '"',
		"\\"	=> "\\"
	};
	
	for my $line (split(/(\r\n|\n|\r)/, $ini)) {
		# standalone comments
		next if $line =~ /^\s*[#;].*?$/i;
		
		if ($line =~ /^\s*\[([\w\d_-]+)\]\s*$/i) {
			$hash->{$1} = {};
			$ref = $hash->{$1};
		} elsif ($line =~ /^\s*([\w\d_-]+)\s*=\s*(['"]?)(.*?)$/i) {
			my ($k, $escape, $v) = ($1, $2, $3);
			if ($escape) {
				my $new_value = "";
				my $slash_escape = 0;
				my $success = 0;
				
				for (my $i = 0; $i < length($v); $i++) {
					my $c = substr($v, $i, 1);
					
					if ($slash_escape) {
						die "Invalid escape \\$c" if !exists $escapes->{$c};
						$new_value .= $escapes->{$c};
						$slash_escape = 0;
					} elsif ($c eq "\\") {
						$slash_escape = 1;
					} elsif ($c eq $escape) {
						$success = 1;
						last;
					} else {
						$new_value .= $c;
					}
				}
				
				die "Invalid string: $v" if !$success;
				
				$v = $new_value;
			} else {
				$v =~ s/[;#].*?$//g; # comments in value
				$v =~ s/^\s+|\s+$//g; # trim
			}
			
			# parse hex values
			$v = hex $v if $v =~ /^(0x[a-f0-9]+)$/i;
			
			$ref->{$k} = $v;
		}
	}
	return $hash;
}

sub hexdump {
	my ($addr, $data) = @_;
	
	my $chunk = 16;
	my $size = length($data);
	
	for (my $i = 0; $i < $size; $i += $chunk) {
		my @hex;
		my @raw;
		
		for (my $j = 0; $j < $chunk; $j++) {
			last if $i + $j >= $size;
			
			my $c = substr($data, $i + $j, 1);
			
			push @hex, "" if $j == 7;
			
			push @hex, sprintf("%02X", ord($c));
			
			if ($c =~ /^[[:print:]]+$/is) {
				push @raw, $c;
			} else {
				push @raw, ".";
			}
		}
		
		printf("%08X  %s  |%s|\n", $addr + $i, join(" ", @hex), join("", @raw));
	}
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
