#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/../tools/lib';
use Data::Dumper;
use Sie::BoardMetadata;

my $gen_symbols = !!$ARGV[0];

my $board_meta = Sie::BoardMetadata->new("siemens-el71");
my $cpu_meta = $board_meta->cpu();

my $irqs = {};
for my $id (@{$cpu_meta->getModuleNames()}) {
	my $module = $cpu_meta->{modules}->{$id};
	for my $irq_name (keys %{$module->{irqs}}) {
		$irqs->{$module->{irqs}->{$irq_name}} = $module->{name}.($irq_name ? "_".$irq_name : "");
	}
}

my $irq_table = `perl ./read.pl --addr=00002ed8 --size=0x27C --dump --speed 115200`;

my $irq_n = 0;
while ($irq_table =~ /([0-9a-f]+): ([0-9a-f]+)/ig) {
	my ($ptr, $addr) = (hex $1, hex $2);
	
	if ($gen_symbols) {
		if ($addr != 0x000007DC) {
			if (exists $irqs->{$irq_n}) {
				printf("irq_handler_%s_0x%02X_d%d %08X f\n", $irqs->{$irq_n}, $irq_n, $irq_n, $addr);
			} else {
				printf("irq_handler_unknown_0x%02X_d%d %08X f\n", $irq_n, $irq_n, $addr);
			}
		}
	} else {
		if (exists $irqs->{$irq_n}) {
			printf("sub_%08X: %d [%02X - %s]\n", $addr, $irq_n, $irq_n, $irqs->{$irq_n});
		} else {
			printf("sub_%08X: %d [%02X - %s]\n", $addr, $irq_n, $irq_n, "unknown");
		}
	}
	
	$irq_n++;
}
