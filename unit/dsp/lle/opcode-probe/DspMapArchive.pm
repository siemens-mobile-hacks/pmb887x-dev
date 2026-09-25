package DspMapArchive;

use strict;
use warnings;

use Digest::SHA;
use Exporter qw(import);
use File::Spec;
use IO::Compress::Gzip qw($GzipError);
use IO::Uncompress::Gunzip qw($GunzipError);

our @EXPORT_OK = qw(extract_archive read_member verify_archive write_archive);

my $magic = "DSPMAP1\n";

sub read_exact {
	my ($input, $size, $description) = @_;
	my $data = '';
	while (length($data) < $size) {
		my $read = read $input, my $chunk, $size - length($data);
		die "Cannot read $description: $!\n" unless defined $read;
		die "Unexpected end of archive while reading $description\n" if $read == 0;
		$data .= $chunk;
	}
	return $data;
}

sub member_path {
	my ($directory, $name) = @_;
	die "Invalid archive member name: $name\n" unless $name =~ /\A[A-Za-z0-9][A-Za-z0-9_.-]*\z/;
	return File::Spec->catfile($directory, $name);
}

sub scan_archive {
	my ($path, $callback) = @_;
	my $input = IO::Uncompress::Gunzip->new($path) or die "Cannot read $path: $GunzipError\n";
	die "$path: invalid DSP map archive magic\n" unless read_exact($input, length($magic), 'magic') eq $magic;
	my $count = unpack 'V', read_exact($input, 4, 'member count');
	my @members;
	my %seen;

	for my $index (0 .. $count - 1) {
		my ($name_size, $data_size) = unpack 'vV', read_exact($input, 6, "member $index header");
		my $expected_hash = read_exact($input, 32, "member $index hash");
		my $name = read_exact($input, $name_size, "member $index name");
		member_path('', $name);
		die "$path: duplicate member $name\n" if $seen{$name}++;

		my $digest = Digest::SHA->new(256);
		my $remaining = $data_size;
		my $state = $callback->('begin', $name, $data_size);
		while ($remaining) {
			my $size = $remaining > 65536 ? 65536 : $remaining;
			my $data = read_exact($input, $size, "$name data");
			$digest->add($data);
			$callback->('data', $name, $data, $state);
			$remaining -= length($data);
		}
		my $actual_hash = $digest->digest;
		die "$path: SHA-256 mismatch for $name\n" unless $actual_hash eq $expected_hash;
		$callback->('end', $name, $data_size, $state);
		push @members, $name;
	}

	my $trailing = read $input, my $byte, 1;
	die "Cannot finish reading $path: $!\n" unless defined $trailing;
	die "$path: trailing archive data\n" if $trailing != 0;
	$input->close or die "Cannot close $path: $GunzipError\n";
	return \@members;
}

sub write_archive {
	my ($path, $directory, $members) = @_;
	my $temporary_path = "$path.$$";
	my $output = IO::Compress::Gzip->new($temporary_path, Time => 0)
		or die "Cannot write $temporary_path: $GzipError\n";
	print {$output} $magic, pack('V', scalar @$members);

	for my $name (@$members) {
		my $member_path = member_path($directory, $name);
		my $size = -s $member_path;
		die "Cannot stat $member_path: $!\n" unless defined $size;
		die "$member_path is too large for the archive format\n" if $size > 0xFFFFFFFF;
		open my $input, '<:raw', $member_path or die "Cannot read $member_path: $!\n";
		my $digest = Digest::SHA->new(256);
		$digest->addfile($input);
		seek $input, 0, 0 or die "Cannot rewind $member_path: $!\n";
		print {$output} pack('vV', length($name), $size), $digest->digest, $name;
		while (1) {
			my $read = read $input, my $data, 65536;
			die "Cannot read $member_path: $!\n" unless defined $read;
			last if $read == 0;
			print {$output} $data or die "Cannot write $temporary_path: $!\n";
		}
		close $input or die "Cannot close $member_path: $!\n";
	}

	$output->close or die "Cannot close $temporary_path: $GzipError\n";
	chmod 0644, $temporary_path or die "Cannot chmod $temporary_path: $!\n";
	rename $temporary_path, $path or die "Cannot replace $path: $!\n";
}

sub extract_archive {
	my ($path, $directory) = @_;
	return scan_archive($path, sub {
		my ($event, $name, $value, $state) = @_;
		if ($event eq 'begin') {
			my $member_path = member_path($directory, $name);
			open my $output, '>:raw', $member_path or die "Cannot write $member_path: $!\n";
			return $output;
		} elsif ($event eq 'data') {
			print {$state} $value or die "Cannot write extracted $name: $!\n";
		} else {
			close $state or die "Cannot close extracted $name: $!\n";
		}
		return $state;
	});
}

sub verify_archive {
	my ($path, $expected_members) = @_;
	my $members = scan_archive($path, sub { return; });
	die "$path: unexpected member list\n" unless join("\n", @$members) eq join("\n", @$expected_members);
}

sub read_member {
	my ($path, $wanted) = @_;
	my $result;
	scan_archive($path, sub {
		my ($event, $name, $value) = @_;
		if ($name eq $wanted) {
			$result = '' if $event eq 'begin';
			$result .= $value if $event eq 'data';
		}
		return;
	});
	die "$path: missing member $wanted\n" unless defined $result;
	return $result;
}

1;
