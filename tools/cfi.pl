#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use File::Slurp qw(read_file);
use Sie::CFI;
use Sie::Utils;

my $path = getDataDir().'/cfi';
opendir my $fp, $path or die "opendir($path): $!";
my @files = readdir $fp;
closedir $fp;

for my $dump_file (@files) {
	next if $dump_file !~ /^([a-f0-9]+)-([a-f0-9]+)\.hex$/i;
	my ($vid, $pid) = (hex $1, hex $2);
	
	my $dump_text = scalar(read_file("$path/$dump_file"));
	$dump_text =~ s/#.*?$//gm;
	$dump_text =~ s/^\s+|\s+$//g;
	
	my @dump = map { hex $_ } split(/\s+/s, $dump_text);
	my $info = Sie::CFI::parseFlashDump($vid, $pid, \@dump);
	printFlashDump($info);
}

sub printFlashDump {
	my ($info) = @_;
	
	my $CFI_ADDR = 0x10;
	
	printf("FLASH: %04X:%04X\n", $info->{vid}, $info->{pid});
	printf("Lock status: %04X\n", $info->{lock});
	printf("CR: %04X\n", $info->{cr});
	printf("EHCR: %04X\n", $info->{ehcr});
	print "\n";
	
	my $cfi = $info->{cfi};
	print "---- Common Flash interface ----\n";
	printf(" CFI: %s\n", bin2hex($info->{cfi_bin}));
	printf(" Primary Vendor: %04X\n", $cfi->{pri_vendor});
	printf(" Primary PRI addr: 0x%04X\n", $cfi->{pri_addr});
	printf(" Alternate Vendor: %04X\n", $cfi->{alt_vendor});
	printf(" Alternate PRI addr: 0x%04X\n", $cfi->{alt_addr});
	
	printf(" VDD min: %s\n", $cfi->{vdd_min});
	printf(" VDD max: %s\n", $cfi->{vdd_max});
	
	printf(" VPP min: %s\n", $cfi->{vpp_min});
	printf(" VPP max: %s\n", $cfi->{vpp_max});
	
	printf(" Default timeout single program: %d µs\n", $cfi->{timeout_signle_program});
	printf(" Default timeout buffer program: %d µs\n", $cfi->{timeout_buffer_program});
	printf(" Default timeout single erase: %d ms\n", $cfi->{timeout_signle_erase});
	printf(" Default timeout chip erase: %d ms\n", $cfi->{timeout_chip_erase});
	
	printf(" Max timeout single program: %d µs\n", $cfi->{max_timeout_signle_program});
	printf(" Max timeout buffer program: %d µs\n", $cfi->{max_timeout_buffer_program});
	printf(" Max timeout single erase: %d ms\n", $cfi->{max_timeout_signle_erase});
	printf(" Max timeout chip erase: %d ms\n", $cfi->{max_timeout_chip_erase});
	
	printf(" Memory size: %d bytes [%d Mb]\n", $cfi->{flash_size}, $cfi->{flash_size} / 1024 / 1024);
	printf(" Device interface: %04X\n", $cfi->{interface});
	printf(" Max buffer size: %d bytes\n", $cfi->{max_buffer_size});
	printf(" Erase regions:\n");
	for my $region (@{$cfi->{erase_regions}}) {
		printf("  #%d: %d blocks x %d bytes\n", $region->{id}, $region->{blocks}, $region->{block_size});
	}
	print "\n";
	
	my $pri = $info->{pri};
	print "---- Primary Vendor-Specific Extended Query ----\n";
	printf(" PRI: %s\n", bin2hex($info->{pri_bin}));
	printf(" PRI version: %s\n", $pri->{version});
	printf(" Features:\n");
	for my $k (sort keys %{$pri->{features}}) {
		printf("   %s: %d\n", $k, $pri->{features}->{$k});
	}
	printf(" Functions after suspend:\n");
	for my $k (sort keys %{$pri->{functions_after_suspend}}) {
		printf("   %s: %d\n", $k, $pri->{functions_after_suspend}->{$k});
	}
	printf(" Protect block status:\n");
	for my $k (sort keys %{$pri->{block_protect_status}}) {
		printf("   %s: %d\n", $k, $pri->{block_protect_status}->{$k});
	}
	
	printf(" VDD optimum: %s\n", $pri->{vdd_optimum});
	printf(" VPP optimum: %s\n", $pri->{vpp_optimum});
	print "\n";
	
	print "---- Protection Register Information ----\n";
	printf(" Protection fields: %d\n", scalar(@{$pri->{otp}}));
	print "\n";
	
	for my $otp (@{$pri->{otp}}) {
		my $value = $info->{otp_bin}->[$otp->{id}];
		printf(" OTP%d: %s [2 lock bytes + %d bytes]\n", $otp->{id}, bin2hex($value), $otp->{size});
		printf(" OTP%d addr: 0x%X\n", $otp->{id}, $otp->{addr});
		printf(" OTP%d factory bytes: %d\n", $otp->{id}, $otp->{factory});
		print "\n";
	}
	
	print "---- Burst Read information ----\n";
	printf(" Page-mode read capability: %d bytes\n", $pri->{page_mode_bytes});
	
	for my $conf (@{$pri->{sync_mode_conf}}) {
		printf(" Synchronous mode read capability configuration %d:\n", $conf->{id});
		for my $key (sort keys %{$conf->{options}}) {
			printf("  %s: %s\n", $key, $conf->{options}->{$key} == 128 ? 'Cont.' : $conf->{options}->{$key});
		}
	}
	print "\n";
	
	my $showRegions = sub {
		my ($ex, $regions) = @_;
		
		print "----  ".($ex ? "Extended " : "")."Bank and erase block region information ----\n";
		printf(" Number of bank regions within the device: %d\n", scalar(@$regions));
		print "\n";
		
		my $real_flash_size = 0;
		
		for my $region (@$regions) {
			printf("---- %sBank and erase block region %d information ----\n", $ex ? "Extended " : "", $region->{id});
			
			printf(" Number of identical banks: %d\n", $region->{identical_banks});
			
			print " Number of program or erase operations allowed:\n";
			for my $k (sort keys %{$region->{simultaneous}}) {
				printf("  Simultaneous %s operations: %d\n", $k, $region->{simultaneous}->{$k});
			}
			
			print " Simultaneous operations in other partitions while a partition in this region is in Program mode:\n";
			for my $k (sort keys %{$region->{simultaneous_while_program}}) {
				printf("  Simultaneous %s operations: %d\n", $k, $region->{simultaneous_while_program}->{$k});
			}
			
			print " Simultaneous operations in other partitions while a partition in this region is in Erase mode:\n";
			for my $k (sort keys %{$region->{simultaneous_while_erase}}) {
				printf("  Simultaneous %s operations: %d\n", $k, $region->{simultaneous_while_erase}->{$k});
			}
			
			my $erase_regions_sum = 0;
			for my $erase (@{$region->{erase_regions}}) {
				printf(" Erase region %d:\n", $erase->{id});
				printf("  Size: %d blocks x %d bytes\n", $erase->{blocks}, $erase->{block_size});
				printf("  Erase cycles: %d\n", $erase->{erase_cycles});
				printf("  Bits per cell: %d\n", $erase->{bits_per_cell});
				printf("  Internal ECC used: %d\n", $erase->{internal_ecc});
				print "  Page mode and Synchronous mode capabilities:\n";
				for my $k (sort keys %{$erase->{rw_caps}}) {
					printf("   %s: %d\n", $k, $erase->{rw_caps}->{$k});
				}
				
				if (exists $erase->{aligned_size}) {
					printf(" Aligned size of programming region: %d\n", $erase->{aligned_size});
					printf(" Ignore aligned size: %d\n", $erase->{ignore_aligned_size});
					printf(" Control mode valid size: %d\n", $erase->{ctrl_mode_valid_size});
					printf(" Control mode invalid size: %d\n", $erase->{ctrl_mode_invalid_size});
					printf(" Ignore control mode: %d\n", $erase->{ignore_ctrl_mode});
				}
				
				$erase_regions_sum += $erase->{blocks} * $erase->{block_size};
			}
			
			$real_flash_size += $erase_regions_sum * $region->{identical_banks};
			print "\n";
		}
		printf(" Total flash size: %d [%dM] (by erase regions)\n", $real_flash_size, $real_flash_size / 1024 / 1024);
		print "\n";
	};
	
	$showRegions->(0, $pri->{regions});
	$showRegions->(1, $pri->{regions_efa}) if exists $pri->{regions_efa};
	
	print "------------------------------------------\n";
}
