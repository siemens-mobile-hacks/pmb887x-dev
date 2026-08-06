#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use Getopt::Long;
use lib dirname(__FILE__).'/../lib';
use Sie::BinaryReader;
use Sie::Utils;

my $BASE = 0xA0000000;
my $BLOCK_SIZE = 0x10000;

my @type_filters;
my @id_filters;
my $dump_data;
my $help;

GetOptions(
	'type=s@' => \@type_filters,
	'id=i@' => \@id_filters,
	'data' => \$dump_data,
	'help' => \$help,
) or usage();

usage() if $help;
my $file = shift @ARGV;
usage() if !defined($file) || @ARGV;

my %type_filters = map { uc($_) => 1 } @type_filters;
my %id_filters = map { $_ => 1 } @id_filters;

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

for my $type (qw(EELITE EEFULL)) {
	next if %type_filters && !$type_filters{$type};
	next if !defined($parts->{$type});

	print "$type:\n";
	my @items = parseEeprom($type, $reader, $parts->{$type}, $is_nsg);
	for my $item (sort { $a->{id} <=> $b->{id} } @items) {
		next if %id_filters && !$id_filters{$item->{id}};

		printf("%08X %08X | #%04d [ver: %d, size: %d]\n",
			$BASE + $item->{eit}, $BASE + $item->{offset}, $item->{id}, $item->{ver}, $item->{size});
		print bin2hex($item->{value})."\n" if $dump_data;
	}
}

sub usage {
	print STDERR "Usage: $0 [--type EELITE|EEFULL] [--id ID] [--data] FULLFLASH\n";
	exit 2;
}

sub parseEeprom {
	my ($type, $reader, $blocks, $is_nsg) = @_;
	
	my @items;
	my %seen;
	
	for my $blk (@$blocks) {
		my $entry_size = $is_nsg ? 32 : 16;
		my $total_items = int($blk->{size} / $entry_size) - 1;
		for (my $i = 1; $i <= $total_items; $i++) {
			my ($eit, $flags, $block_id, $ver, $size, $offset, $value_offset);
			
			if ($is_nsg) {
				$eit = $blk->{start} + $blk->{size} - $entry_size - ($entry_size * $i);
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
					$offset = $reader->readUInt32();
				}
			} else {
				$eit = $blk->{start} + $blk->{size} - ($entry_size * $i);
				$reader->seek($eit);
				
				$flags = $reader->readUInt32();
				$block_id = $reader->readUInt32();
				$size = $reader->readUInt32() - 1;
				$offset = $reader->readUInt32();
			}
			
			next if $flags == 0xFFFFFFFF;
			next if $block_id == 0xFFFFFFFF;

			$block_id += 5000 if $type eq "EEFULL";
			next if $flags != 0xFFFFFFC0;
			next if $seen{$block_id}++;
			
			if ($type eq "EEFULL") {
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

	return @items;
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
