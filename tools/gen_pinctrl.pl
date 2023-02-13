#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::BoardMetadata;
use Sie::Utils;

my $str = "";

for my $cpu (@{Sie::CpuMetadata::getCpus()}) {
	my $cpu_meta = Sie::CpuMetadata->new($cpu);
	$str .= genPinctrl($cpu_meta);
}

print $str."\n";

sub genPinctrl {
	my ($cpu_meta) = @_;
	
	my $last_pin = -1;
	
	my $str = "/* CPU: ".$cpu_meta->{name}." */\n";
	for my $gpio_name (getSortedKeys($cpu_meta->gpios(), 'id')) {
		my $gpio = $cpu_meta->gpios()->{$gpio_name};
		$str .= "PMB887X_PIN(\n";
		$str .= "\tPINCTRL_PIN(".$gpio->{id}.", \"".$gpio->{name}."\"),\n";
		
		for my $func (@{$gpio->{alt}}) {
			my $id1 = $func->{id};
			my $id2 = $func->{id};
			
			$id1 = 0 if ($func->{flags} ne "I" && $func->{flags} ne "IO");
			$id2 = 0 if ($func->{flags} ne "O" && $func->{flags} ne "IO");
			
			$str .= "\tPMB887X_FUNCTION($id1, $id2, \"".$func->{name}."\"),\n";
		}
		
		$str .= "),\n";
		
		if ($last_pin - $gpio->{id} != -1) {
			die("Invalid GPIO map: last_pin=$last_pin, id=".$gpio->{id}.", cpu=".$cpu_meta->{name});
		}
		
		$last_pin = $gpio->{id};
	}
	
	$str .= "\n";
	
	return $str;
}
