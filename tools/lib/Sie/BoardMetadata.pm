package Sie::BoardMetadata;

use warnings;
use strict;
use File::Basename;
use Data::Dumper;
use Storable qw(dclone);
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::Utils;

sub new {
	my ($class, $board) = @_;
	
	my $self = bless { } => $class;
	
	$self->loadBoard($board);
	
	return $self;
}

sub cpu {
	my ($self) = @_;
	return $self->{cpu};
}

sub gpios {
	my ($self) = @_;
	return $self->{cpu}->{gpios};
}

sub loadBoard {
	my ($self, $board) = @_;
	
	my $file = getDataDir().'/board/'.$board.'.cfg';
	
	$self->{name} = $board;
	
	open my $fp, "<$file" or die("open($file): $!");
	while (my $line = <$fp>) {
		next if $line =~ /^(\/\/|#)/;
		
		# Replace comments
		$line =~ s/\/\*.*?\*\///sig;
		
		# Normalize spaces
		$line =~ s/[\t]+/\t/g;
		$line =~ s/^\s+|\s+$//g;
		
		next if !length($line);
		
		if ($line =~ /^\.([\w\d_-]+)\s*(.*?)$/) {
			my $key = $1;
			my $value = $2;
			
			if ($key eq "gpio") {
				my ($gpio_name, $gpio_cpu_name) = split(/\s+/, $value);
				$self->{gpios}->{$gpio_cpu_name} = $gpio_name;
			} elsif ($key eq "cpu") {
				$self->{cpu} = Sie::CpuMetadata->new($value);
			}
		}
	}
	
	$self->{cpu}->setGpios($self->{gpios});
	
	close $fp;
}

1;
