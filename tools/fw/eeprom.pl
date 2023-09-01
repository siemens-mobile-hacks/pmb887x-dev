#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/../lib';
use Data::Dumper;
use Sie::BinaryReader;
use Sie::Utils;

my $BASE = 0xA0000000;
my $BLOCK_SIZE = 0x10000;

my $file = $ARGV[0];

my $reader = Sie::BinaryReader->new->open($file);

printf("Fullflash size: %08X\n", $reader->size);

my ($is_nsg, $parts) = getPartitions($reader);

for my $blk_name (sort { $parts->{$a}->[0]->{start} <=> $parts->{$b}->[0]->{start} } keys %$parts) {
	my $first = $parts->{$blk_name}->[0];
	my $last = $parts->{$blk_name}->[-1];
	my $blks_n = scalar(@{$parts->{$blk_name}});
	
	my $total_size = 0;
	for my $blk (@{$parts->{$blk_name}}) {
		$total_size += $blk->{size};
	}
	$total_size /= 1024 * 1024;
	
	printf("%8s %08X ... %08X [%d blocks, %.02f Mb]\n", $blk_name, $BASE + $first->{start}, $BASE + $last->{start} + $last->{size} - 1, $blks_n, $total_size);
}

print "EELITE:\n";
parseEeprom("EELITE", $reader, $parts->{EELITE}, $is_nsg);

print "EEFULL:\n";
parseEeprom("EEFULL", $reader, $parts->{EEFULL}, $is_nsg);

sub parseEeprom {
	my ($type, $reader, $blocks, $is_nsg) = @_;
	
	my @items;
	
	for my $blk (@$blocks) {
		my $total_items = $type eq "EEFULL" ? 0x2000 : 512;
		for (my $i = 1; $i <= $total_items; $i++) {
			my ($eit, $flags, $block_id, $ver, $size, $offset, $value_offset);
			
			if ($is_nsg) {
				$eit = $blk->{start} + $blk->{size} - 32 - (32 * $i);
				$reader->seek($eit);
				
				if ($type eq "EEFULL") {
					$flags = $reader->readUInt32();
					$block_id = $reader->readUInt32();
					$size = $reader->readUInt32() - 1;
					$offset = $reader->readUInt32();
				} else {
					$flags = $reader->readUInt32();
					$block_id = $reader->readUInt16();
					$reader->readUInt8(); # unk
					$ver = $reader->readUInt8();
					$size = $reader->readUInt16();
					$reader->readUInt16(); # unk
					$offset = $reader->readUInt16();
				}
			} else {
				$eit = $blk->{start} + $blk->{size} - (16 * $i);
				$reader->seek($eit);
				
				$flags = $reader->readUInt32();
				$block_id = $reader->readUInt32();
				$size = $reader->readUInt32() - 1;
				$offset = $reader->readUInt32();
			}
			
			$reader->seek($eit);
			print "BLK: ".sprintf("%08X ", $eit).bin2hex($reader->readBytes(32))." | $offset\n";
			
			# skip free or invalid blocks
			next if $flags != 0xFFFFFFC0 || $block_id == 0xFFFFFFFF;
			
			if ($type eq "EEFULL") {
				$block_id += 5000;
				$value_offset = $blk->{start} + $offset + 1;
				$reader->seek($value_offset - 1);
				$ver = $reader->readUInt8();
			} else {
				$value_offset = $blk->{start} + $offset;
				if (!$is_nsg) {
					$reader->seek($value_offset + $size);
					$ver = $reader->readUInt8();
				}
			}
			
			print sprintf("%08X %08X | #%03d [ver: %d, size: %d]\n", $BASE + $eit, $BASE + $value_offset, $block_id, $ver, $size);
			
			$reader->seek($value_offset);
			my $value = $reader->readBytes($size);
			
			push @items, {
				flags	=> $flags,
				type	=> $type,
				id		=> $block_id,
				ver		=> $ver,
				size	=> $size,
				eit		=> $eit,
				offset	=> $value_offset,
				value	=> $value
			};
		}
	}
}

sub getPartitions {
	my ($reader) = @_;
	
	my %parts;
	
	# part header
	# char[8] - block name
	# uint16_t - unk
	# uint16_t - unk2, always 0x0000
	# uint32_t - unk3, always 0xFFFFFFF0
	
	my $blk_sg_re = qr/^(.{8})(.{2})\x00\x00\xF0[\xFF]{3}$/;
	my $blk_nsg_re = qr/^(.{8})(.{2})\x00\x00\xF0[\xFF]{19}$/;
	
	my $is_nsg = -1;
	
	for (my $i = 0; $i < $reader->size; $i += $BLOCK_SIZE) {
		my $blk_hdr_up;
		my $blk_hdr_down;
		
		if ($is_nsg == -1 || $is_nsg == 0) {
			$reader->seek($i);
			$blk_hdr_up = $reader->readBytes(16);
		}
		
		if ($is_nsg == -1 || $is_nsg == 1) {
			$reader->seek($i + ($BLOCK_SIZE - 32));
			$blk_hdr_down = $reader->readBytes(32);
		}
		
		my $blk;
		
		if (defined $blk_hdr_up && $blk_hdr_up =~ $blk_sg_re) {
			my $name = $1;
			my $unk = unpack("v", $2);
			$name =~ s/[\0]+$//g;
			
			$blk = {
				name	=> $name,
				start	=> $i,
				size	=> $BLOCK_SIZE * 2,
				unk		=> $unk
			};
			
			$is_nsg = 0;
		} elsif (defined $blk_hdr_down && $blk_hdr_down =~ $blk_nsg_re) {
			my $name = $1;
			my $unk = unpack("v", $2);
			$name =~ s/[\0]+$//g;
			
			$blk = {
				name	=> $name,
				start	=> $i - $BLOCK_SIZE * 3,
				size	=> $BLOCK_SIZE * 4,
				unk		=> $unk
			};
			
			$is_nsg = 1;
		}
		
		# printf("%08X | %s\n", $i, bin2hex($blk_hdr_up)) if $is_nsg == 0 || $is_nsg == -1;
		# printf("%08X | %s\n", $i, bin2hex($blk_hdr_down)) if $is_nsg == 1 || $is_nsg == -1;
		
		if (defined $blk) {
			# print $blk->{name}." ".$blk->{unk}."\n";
			$parts{$blk->{name}} = $parts{$blk->{name}} || [];
			push @{$parts{$blk->{name}}}, $blk;
		}
	}
	
	return ($is_nsg, \%parts);
}
