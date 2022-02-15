#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use JSON::XS;

my $funcs = parseLib($ARGV[0]);
my $type = $ARGV[1] || "trace";

my @items;

for my $func (@$funcs) {
	if ($type eq "trace") {
		my $args = JSON::XS->new->encode($func->{args});
		my $addr = sprintf("0x%08X", $func->{addr});
		push @items, qq|	{
			"name":		"$func->{name}",
			"addr":		$addr,
			"args":		$args
		}|;
	} elsif ($type eq "sym") {
		printf("%s %08X f\n", $func->{name}, $func->{addr} & ~1);
	}
}

if ($type eq "trace") {
	print "[\n".join(",\n", @items)."\n]\n";
}

sub parseLib {
	my ($file) = @_;
	
	my @funcs;
	
	my $types = {
		"char"					=> "char",
		"unsigned"				=> "int32",
		"unsigned char"			=> "uint8",
		"unsigned int"			=> "uint32",
		"unsigned long"			=> "uint32",
		"option"				=> "uint8",
		"bool"					=> "uint8",
		"unsigned short"		=> "uint16",
		"size_t"				=> "uint32",
		"double"				=> "double",
		"long"					=> "int32",
		"int"					=> "int32",
		"signed int"			=> "int32",
		"short"					=> "int16",
		"char*"					=> "cstr"
	};
	
	open F, "<".$file;
	while (my $line = <F>) {
		next if $line =~ /^[;+#-]/;
		next if $line =~ /^\s*$/;
		
		if ($line =~ /^([a-f0-9]+)\s*:\s*(0x[a-f0-9]+)\s*;\s*(.*?)$/i) {
			my ($offset, $addr, $descr) = ($1, $2, $3);
			
			$descr =~ s/^\s*([a-f0-9]+)\s*:\s*//gi;
			$descr =~ s/\/\/.*?$//g;
			$descr =~ s/^\s+|\s+$//g;
			
			if ($descr =~ /^([\w\d_]+)$/i) {
				push @funcs, {
					name		=> $descr,
					addr		=> hex($addr),
					args		=> []
				};
			} elsif ($descr =~ /^([\w\d_]+)\s*\(\)$/i) {
				push @funcs, {
					name		=> $descr,
					addr		=> hex($addr),
					args		=> []
				};
			} elsif ($descr =~ /^([\s_\w\d*]+\s[*]*)?([\w\d_]+)\s*(\s*\(.*?\)\s*)?\s*;?\s*$/i) {
				my ($func_type, $func_name, $func_args) = ($1 || "", $2 || "", $3 || "");
				
				if ($func_args) {
					$func_args =~ s/^\s*\(\s*|\s*\)\s*$//g;
					
					my @args;
					if ($func_args !~ /^\s*void\s*$/i) {
						for my $arg (split(/\s*,\s*/, $func_args)) {
							$arg =~ s/const//g;
							$arg =~ s/\s+/ /g;
							$arg =~ s/\s*([*])\s*/$1/g;
							$arg =~ s/([\w\d_]+)$//g;
							$arg =~ s/^\s+|\s+$//g;
							
							# print "FIXME: ".lc($arg)."\n" if !exists $types->{lc($arg)};
							
							push @args, $types->{lc($arg)} || "ptr";
						}
					}
					
					push @funcs, {
						name		=> $func_name,
						addr		=> hex($addr),
						args		=> \@args
					};
				} else {
					push @funcs, {
						name		=> $func_name,
						addr		=> hex($addr),
						args		=> []
					};
				}
			} else {
				warn "Bad '$line'";
			}
		} else {
			die "Bad '$line'";
		}
	}
	
	close F;
	
	return \@funcs;
}
