package Sie::CFI;

use File::Basename;
use List::Util qw(max min);
use Data::Dumper;
use File::Slurp qw(read_file);
use Sie::BinaryReader;
use Sie::Utils;

sub parseCFI {
	my ($lo_bytes) = @_;
	
	my $r = Sie::BinaryReader->new->openString($lo_bytes);
	
	my $magic = $r->readBytes(3);
	die "Invalid magic, 'QRY' not found!" if $magic ne "QRY";
	
	my $bcdVoltage = sub {
		my ($v) = @_;
		return (($v >> 4) & 0xF).".".($v & 0xF)."V";
	};
	
	my $cfi = {};
	
	$cfi->{pri_vendor} = $r->readUInt16();
	$cfi->{pri_addr} = $r->readUInt16();
	
	$cfi->{alt_vendor} = $r->readUInt16();
	$cfi->{alt_addr} = $r->readUInt16();
	
	$cfi->{vdd_min} = $bcdVoltage->($r->readUInt8());
	$cfi->{vdd_max} = $bcdVoltage->($r->readUInt8());
	
	$cfi->{vpp_min} = $bcdVoltage->($r->readUInt8());
	$cfi->{vpp_max} = $bcdVoltage->($r->readUInt8());
	
	$cfi->{timeout_signle_program} = 1 << $r->readUInt8();
	$cfi->{timeout_buffer_program} = 1 << $r->readUInt8();
	$cfi->{timeout_signle_erase} = 1 << $r->readUInt8();
	$cfi->{timeout_chip_erase} = 1 << $r->readUInt8();
	
	$cfi->{max_timeout_signle_program} = (1 << $r->readUInt8()) * $cfi->{timeout_signle_program};
	$cfi->{max_timeout_buffer_program} = (1 << $r->readUInt8()) * $cfi->{timeout_buffer_program};
	$cfi->{max_timeout_signle_erase} = (1 << $r->readUInt8()) * $cfi->{timeout_signle_erase};
	$cfi->{max_timeout_chip_erase} = (1 << $r->readUInt8()) * $cfi->{timeout_chip_erase};
	
	$cfi->{flash_size} = 1 << $r->readUInt8();
	
	$cfi->{interface} = $r->readUInt16();
	$cfi->{max_buffer_size} = 1 << $r->readUInt16();
	
	my $erase_regions = $r->readUInt8();

	$cfi->{erase_regions} = [];
	
	my $check_size = 0;
	
	for (my $i = 0; $i < $erase_regions; $i++) {
		my $blocks = $r->readUInt16() + 1;
		my $block_size = $r->readUInt16() * 256;
		
		$check_size += $blocks * $block_size;
		
		push @{$cfi->{erase_regions}}, {
			id			=> $i,
			blocks		=> $blocks,
			block_size	=> $block_size
		};
	}
	
	die "Invalid CFI! Flash size by CFI: ".$cfi->{flash_size}.", flash size by erase regions: $check_size"
		if $check_size != $cfi->{flash_size};
	
	$cfi->{size} = $r->offset();
	
	return $cfi;
}

sub parsePRI {
	my ($lo_bytes, $pri_vendor_id) = @_;
	
	my $r = Sie::BinaryReader->new->openString($lo_bytes);
	
	my $magic = $r->readBytes(3);
	die "Invalid magic, 'PRI' not found!" if $magic ne "PRI";
	
	my $pri = {};
	$pri->{version} = $r->readBytes(1).".".$r->readBytes(1);
	
	my $pri_vendor;
	if ($pri_vendor_id == 0x0001) {
		die "Unknown Intel PRI version: ".$pri->{version} if $pri->{version} ne '1.3';
		$pri_vendor = "intel";
	} elsif ($pri_vendor_id == 0x0200) {
		die "Unknown ST PRI version: ".$pri->{version} if $pri->{version} ne '1.4';
		$pri_vendor = "st";
	} else {
		die("Unknown PRI vendor: $pri_vendor");
	}
	
	my $decodeFlags = sub {
		my ($flags, $size, $flags_list) = @_;
		my $result = {};
		for (my $i = 0; $i < $size; $i++) {
			my $flag = $flags & (1 << $i) ? 1 : 0;
			my $key = exists $flags_list->{$i} ? $flags_list->{$i} : "unk_$i";
			$result->{$key} = $flag if exists $flags_list->{$i} || $flag;
		}
		return $result;
	};
	
	my $bcdVoltage = sub {
		my ($v) = @_;
		return (($v >> 4) & 0xF).".".($v & 0xF)."V";
	};
	
	$pri->{features} = $decodeFlags->($r->readUInt32(), 32, {
		0	=> 'chip_erase',
		1	=> 'suspend_erase',
		2	=> 'suspend_program',
		3	=> 'legacy_lock_unlock',
		4	=> 'queued_erase',
		5	=> 'instant_individual_block_locking',
		6	=> 'protection_bits',
		7	=> 'pagemode_read',
		8	=> 'sync_read',
		9	=> 'simultaneous_operations',
		10	=> 'extended_flash_array_blocks',
		30	=> 'cfi_links_follow',
		31	=> 'optional_features'
	});
	
	$pri->{functions_after_suspend} = $decodeFlags->($r->readUInt8(), 8, {
		0	=> 'program_after_erase_suspend',
	});
	
	$pri->{block_protect_status} = $decodeFlags->($r->readUInt16(), 16, {
		0	=> 'block_lock_bit',
		1	=> 'block_lock_down',
		4	=> 'efa_block_lock_bit',
		5	=> 'efa_block_lock_down',
	});
	
	$pri->{vdd_optimum} = $bcdVoltage->($r->readUInt8());
	$pri->{vpp_optimum} = $bcdVoltage->($r->readUInt8());
	
	if ($pri->{features}->{protection_bits}) {
		my $otp_fields = $r->readUInt8();
		$pri->{otp} = [];
		
		for (my $i = 0; $i < $otp_fields; $i++) {
			if ($i == 0) {
				my $otp0_addr = $r->readUInt16();
				my $otp0_size_f = 1 << $r->readUInt8();
				my $otp0_size_u = 1 << $r->readUInt8();
				
				push @{$pri->{otp}}, {
					id				=> $i,
					addr			=> $otp0_addr,
					factory			=> $otp0_size_f,
					size			=> max($otp0_size_u, $otp0_size_f),
				};
			} else {
				my $otp1_addr = $r->readUInt32();
				my $otp1_regions_f = $r->readUInt16();
				my $otp1_region_size_f = 1 << $r->readUInt8();
				my $otp1_regions_u = $r->readUInt16();
				my $otp1_region_size_u = 1 << $r->readUInt8();
				
				push @{$pri->{otp}}, {
					id				=> $i,
					addr			=> $otp1_addr,
					factory			=> $otp1_regions_f * $otp1_region_size_f,
					size			=> $otp1_regions_f * $otp1_region_size_f + $otp1_regions_u * $otp1_region_size_u,
				};
			}
		}
	}
	
	$pri->{page_mode_bytes} = (1 << $r->readUInt8());
	
	$pri->{sync_mode_conf} = [];
	
	my $sync_mode_conf_cnt = $r->readUInt8();
	for (my $i = 0; $i < $sync_mode_conf_cnt; $i++) {
		push @{$pri->{sync_mode_conf}}, {
			id		=> $i,
			options	=> {
				max_continuous_sync_reads	=> 1 << $r->readUInt8()
			}
		};
	}
	
	my $parseRegions = sub {
		my $regions = [];
		
		my $hw_regions = $r->readUInt8();
	
		for (my $i = 0; $i < $hw_regions; $i++) {
			$r->readUInt16() if $pri_vendor eq "st"; # self size
			my $identical_banks = $r->readUInt16();
			
			my $sim_ops = $r->readUInt8();
			my $sim_ops_in_other_banks_while_program = $r->readUInt8();
			my $sim_ops_in_other_banks_while_erase = $r->readUInt8();
			
			my $erase_regions_cnt = $r->readUInt8();
			my $erase_regions = [];
			
			for (my $j = 0; $j < $erase_regions_cnt; $j++) {
				my $blocks = $r->readUInt16() + 1;
				my $block_size = $r->readUInt16() * 256;
				
				my $erase_cycles = $r->readUInt16() * 1000;
				my $bpc_ecc = $r->readUInt8();
				
				my $rw_caps = $decodeFlags->($r->readUInt8(), 8, {
					0	=> 'page_mode',
					1	=> 'sync_read',
					2	=> 'sync_write'
				});
				
				my %additional;
				if ($pri_vendor eq "st") {
					my $aligned_size = $r->readUInt8();
					my $ignore_aligned_size = $r->readUInt8() & (1 << 7) ? 1 : 0;
					my $ctrl_mode_valid_size = $r->readUInt8();
					$r->readUInt8(); # reserved
					my $ctrl_mode_invalid_size = $r->readUInt8();
					my $ignore_ctrl_mode = $r->readUInt8() & (1 << 7) ? 1 : 0;
					
					%additional = (
						aligned_size			=> $aligned_size,
						ignore_aligned_size		=> $ignore_aligned_size,
						ctrl_mode_valid_size	=> $ctrl_mode_valid_size,
						ctrl_mode_invalid_size	=> $ctrl_mode_invalid_size,
						ignore_ctrl_mode		=> $ignore_ctrl_mode,
					);
				}
				
				push @$erase_regions, {
					id				=> $j,
					blocks			=> $blocks,
					block_size		=> $block_size,
					erase_cycles	=> $erase_cycles,
					bits_per_cell	=> $bpc_ecc & 0xF,
					internal_ecc	=> $bpc_ecc & (1 << 4) ? 1 : 0,
					rw_caps			=> $rw_caps,
					%additional
				};
			}
			
			push @$regions, {
				id				=> $i,
				identical_banks	=> $identical_banks,
				erase_regions	=> $erase_regions,
				simultaneous => {
					erase		=> $sim_ops & 0xF,
					program		=> ($sim_ops >> 4) & 0xF,
				},
				simultaneous_while_erase	=> {
					erase		=> $sim_ops_in_other_banks_while_erase & 0xF,
					program		=> ($sim_ops_in_other_banks_while_erase >> 4) & 0xF,
				},
				simultaneous_while_program	=> {
					erase		=> $sim_ops_in_other_banks_while_program & 0xF,
					program		=> ($sim_ops_in_other_banks_while_program >> 4) & 0xF,
				}
			};
		}
		return $regions;
	};
	
	$pri->{regions} = [];
	$pri->{regions_efa} = [];
	
	if ($pri->{features}->{simultaneous_operations}) { # ????
		$pri->{regions} = $parseRegions->();
		$pri->{regions_efa} = $parseRegions->() if $pri_vendor eq 'st';
	}
	
	$pri->{size} = $r->offset();
	
	return $pri;
}

sub parseFlashDump {
	my ($vid, $pid, $dump) = @_;
	
	my $CFI_ADDR = 0x10;
	
	my $lo_bytes = "";
	my $hi_bytes = "";
	for (my $i = 0; $i < scalar(@$dump); $i++) {
		$lo_bytes .= chr($dump->[$i] & 0xFF);
		$hi_bytes .= chr(($dump->[$i] >> 8) & 0xFF);
	}
	
	my $cfi = parseCFI(substr($lo_bytes, $CFI_ADDR));
	my $pri = parsePRI(substr($lo_bytes, $cfi->{pri_addr}), $cfi->{pri_vendor});
	
	my @otp_bin;
	for my $otp (@{$pri->{otp}}) {
		my $value = "";
		for (my $i = $otp->{addr}; $i < $otp->{addr} + $otp->{size} / 2 + 1; $i++) {
			$value .= chr($dump->[$i] & 0xFF).chr(($dump->[$i] >> 8) & 0xFF);
		}
		$otp_bin[$otp->{id}] = $value;
	}
	
	return {
		vid		=> $dump->[0],
		pid		=> $dump->[1],
		lock	=> $dump->[2],
		cr		=> $dump->[5],
		ehcr	=> $dump->[6],
		cfi		=> $cfi,
		pri		=> $pri,
		cfi_bin	=> substr($lo_bytes, $CFI_ADDR, $cfi->{size}),
		pri_bin	=> substr($lo_bytes, $cfi->{pri_addr}, $pri->{size}),
		otp_bin	=> \@otp_bin,
	};
}

1;
