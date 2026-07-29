package Dsp1;

use strict;
use warnings;
use Digest::SHA qw(sha256);
use Exporter qw(import);

our @EXPORT_OK = qw(build_dsp1);

sub build_dsp1 {
	my ($segments) = @_;

	die "DSP1 supports at most 10 segments\n" if @$segments > 10;

	my $header = "\0" x 0x300;
	my $payload = '';
	substr($header, 0x100, 4, 'DSP1');
	substr($header, 0x108, 2, pack('v', 0xFFFF));
	substr($header, 0x10E, 1, pack('C', scalar @$segments));

	for my $index (0 .. $#$segments) {
		my $segment = $segments->[$index];
		my $data = $segment->{data};

		die "DSP1 segment $index has an odd byte size\n" if length($data) % 2;
		die "DSP1 segment $index has an invalid memory type\n"
			if $segment->{memory_type} != 0 && $segment->{memory_type} != 1 && $segment->{memory_type} != 2;

		my $entry = pack('V V V C4',
			0x300 + length($payload),
			$segment->{address},
			length($data),
			0, 0, 0, $segment->{memory_type},
		) . sha256($data);
		substr($header, 0x120 + $index * 0x30, 0x30, $entry);
		$payload .= $data;
	}

	my $image = $header . $payload;
	substr($image, 0x104, 4, pack('V', length($image)));

	return $image;
}

1;
