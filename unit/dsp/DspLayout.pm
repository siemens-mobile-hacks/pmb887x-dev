package DspLayout;

use strict;
use warnings;
use Exporter qw(import);

our @EXPORT_OK = qw(shared_ram instruction_words layout_lines write_text);

sub shared_ram {
	my ($offset) = @_;
	return sprintf('[0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x%04X)]', $offset);
}

sub instruction_words {
	my ($line) = @_;
	(my $code = $line) =~ s{//.*}{};
	return $code =~ /[\$\{]/ ? 2 : 1;
}

sub layout_lines {
	my ($base, $limit, @lines) = @_;
	my %labels;
	my @code;
	my $address = $base;
	for my $line (@lines) {
		if ($line =~ /^(\w+):$/) {
			$labels{$1} = $address;
			next;
		}
		push @code, $line;
		$address += instruction_words($line);
	}
	die sprintf("segment %04X ends at %04X beyond %04X\n", $base, $address, $limit) if $address > $limit;
	return map {
		(my $line = $_) =~ s/\{(\w+)\}/sprintf('%04X', $labels{$1} \/\/ die "undefined label $1\n")/ge;
		$line;
	} @code;
}

sub write_text {
	my ($path, $text) = @_;
	open my $output, '>', $path or die "Cannot write $path: $!\n";
	print {$output} $text;
	close $output or die "Cannot close $path: $!\n";
}

1;
