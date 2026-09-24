package TeakTools;

use strict;
use warnings;
use Exporter qw(import);
use File::Spec;

our @EXPORT_OK = qw(find_teak_tools);

my $TEAKRA_URL = 'https://github.com/siemens-mobile-hacks/teakra';

sub find_executable {
	my ($name) = @_;

	if (File::Spec->file_name_is_absolute($name) || $name =~ m{/}) {
		return $name if -x $name;
		return;
	}
	for my $directory (File::Spec->path()) {
		my $path = File::Spec->catfile($directory, $name);
		return $path if -x $path;
	}

	return;
}

sub require_tool {
	my ($environment, $name) = @_;
	my $requested = $ENV{$environment} // $name;
	my $path = find_executable($requested);

	die "$name is required; install it from $TEAKRA_URL\n" if !defined $path;
	return $path;
}

sub find_teak_tools {
	return (
		require_tool('TEAK_ASSEMBLER', 'teak-assembler'),
		require_tool('TEAK_DISASSEMBLER', 'teak-disassembler'),
	);
}

1;
