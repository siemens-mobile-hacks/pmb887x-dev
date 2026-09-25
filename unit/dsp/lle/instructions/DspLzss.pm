package DspLzss;

use strict;
use warnings;

use Exporter qw(import);

our @EXPORT_OK = qw(compress_blocks decompress_blocks);

# A literal control byte stores length - 1 followed by 1-128 bytes. A control
# byte with bit 7 set stores length - 3 followed by a 16-bit backward distance.
# Blocks end on token boundaries but share the same 64 KiB history.
sub compress_blocks {
	my (@blocks) = @_;
	my $input = join '', @blocks;
	my @head = (-1) x 65536;
	my @previous = (-1) x 65536;
	my $packed = '';
	my @offsets;
	my @sizes;
	my $position = 0;
	my $maximum_size = 0;

	my $hash_at = sub {
		my ($offset) = @_;
		return ((vec($input, $offset, 8) * 251 + vec($input, $offset + 1, 8)) * 251 +
			vec($input, $offset + 2, 8)) & 0xFFFF;
	};
	my $insert = sub {
		my ($offset) = @_;
		return if $offset + 2 >= length($input);
		my $hash = $hash_at->($offset);
		$previous[$offset & 0xFFFF] = $head[$hash];
		$head[$hash] = $offset;
	};

	for my $block (@blocks) {
		my $block_end = $position + length($block);
		my $literals = '';
		push @offsets, length($packed);
		push @sizes, length($block);
		$maximum_size = length($block) if length($block) > $maximum_size;

		while ($position < $block_end) {
			my $best_length = 0;
			my $best_distance = 0;
			my $maximum_length = $block_end - $position;
			$maximum_length = 130 if $maximum_length > 130;
			if ($maximum_length >= 4) {
				my $candidate = $head[$hash_at->($position)];
				my $attempts = 0;
				while ($candidate >= 0 && $position - $candidate <= 65535 && $attempts < 64) {
					my $length = 0;
					$length++ while $length < $maximum_length &&
						vec($input, $candidate + $length, 8) == vec($input, $position + $length, 8);
					if ($length > $best_length) {
						$best_length = $length;
						$best_distance = $position - $candidate;
						last if $best_length == $maximum_length;
					}
					$candidate = $previous[$candidate & 0xFFFF];
					$attempts++;
				}
			}

			if ($best_length >= 4) {
				$packed .= pack('C', length($literals) - 1) . $literals if length($literals);
				$literals = '';
				$packed .= pack('Cv', 0x80 | ($best_length - 3), $best_distance);
				for my $offset ($position .. $position + $best_length - 1) {
					$insert->($offset);
				}
				$position += $best_length;
			} else {
				$literals .= chr vec($input, $position, 8);
				$insert->($position);
				$position++;
				if (length($literals) == 128) {
					$packed .= pack('C', 127) . $literals;
					$literals = '';
				}
			}
		}
		$packed .= pack('C', length($literals) - 1) . $literals if length($literals);
	}

	return ($packed, \@offsets, \@sizes, $maximum_size);
}

sub decompress_blocks {
	my ($packed, $offsets, $sizes) = @_;
	my $history = "\0" x 65536;
	my $history_position = 0;
	my @blocks;

	for my $block (0 .. $#$offsets) {
		my $input_position = $offsets->[$block];
		my $output = '';
		while (length($output) < $sizes->[$block]) {
			my $control = vec($packed, $input_position++, 8);
			if (($control & 0x80) == 0) {
				my $length = $control + 1;
				for (1 .. $length) {
					my $value = vec($packed, $input_position++, 8);
					$output .= chr $value;
					vec($history, $history_position++ & 0xFFFF, 8) = $value;
				}
			} else {
				my $length = ($control & 0x7F) + 3;
				my $distance = unpack 'v', substr($packed, $input_position, 2);
				my $source = $history_position - $distance;
				$input_position += 2;
				for (1 .. $length) {
					my $value = vec($history, $source++ & 0xFFFF, 8);
					$output .= chr $value;
					vec($history, $history_position++ & 0xFFFF, 8) = $value;
				}
			}
		}
		push @blocks, $output;
	}

	return @blocks;
}

1;
