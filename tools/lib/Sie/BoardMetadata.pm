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

sub getBoards {
	my $path = getDataDir().'/board';
	opendir my $fp, $path or die "opendir($path): $!";
	my @files = readdir $fp;
	closedir $fp;
	
	my $boards = [];
	for my $file (@files) {
		next if !-f $path."/".$file;
		next if $file !~ /\.cfg$/i;
		$file =~ s/\.cfg$//gi;
		push @$boards, $file;
	}
	return $boards;
}

sub loadBoard {
	my ($self, $board) = @_;
	
	my $file = getDataDir().'/board/'.$board.'.cfg';
	
	$self->{name} = $board;
	$self->{ram} = 16;
	$self->{flash} = [0, 0];
	$self->{vendor} = "Unknown";
	$self->{keys} = {};
	$self->{display_rotation} = 0;
	
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
				my ($gpio_name, $gpio_cpu_name, $cfg) = split(/\s+/, $value);
				
				my $mode = "none";
				my $value = undef;
				
				if ($cfg) {
					if ($cfg =~ /INPUT=(\d+)/) {
						$value = $1 ? 1 : 0;
						$mode = "input";
					}
				}
				
				$self->{gpios}->{$gpio_cpu_name} = {
					name	=> $gpio_name,
					mode	=> $mode,
					value	=> $value
				};
			} elsif ($key eq "cpu") {
				$self->{cpu} = Sie::CpuMetadata->new($value);
			} elsif ($key =~ /^width|height|ram$/) {
				$self->{$key} = parseAnyInt($value);
			} elsif ($key eq "vendor") {
				$self->{vendor} = $value;
			} elsif ($key eq "display") {
				my @pairs = split(/\s*,\s*/, $value);
				$self->{display} = shift @pairs;
				
				for my $pair (@pairs) {
					my ($pair_k, $pair_v) = split(/=/, $pair);
					$self->{"display_".$pair_k} = $pair_v;
				}
			} elsif ($key eq "flash") {
				my @flashes = map {
					my ($vid, $pid) = split(/:/, $_);
					(hex($vid) << 16) | hex($pid)
				} split(/\s*,\s*/, $value);
				$self->{flash} = [@flashes];
			} elsif ($key eq "key") {
				my ($kp_name, $kp_in_str, $kp_out_str, $map) = split(/\t+/, $value);
				
				my @kp_in_arr = map { parseAnyInt($_) } split(/\s*,\s*/, $kp_in_str);
				my @kp_out_arr = map { parseAnyInt($_) } split(/\s*,\s*/, $kp_out_str);
				
				my $keycode = 0;
				
				for my $kp_in (@kp_in_arr) {
					$keycode |= 1 << $kp_in;
				}
				
				for my $kp_out (@kp_out_arr) {
					$keycode |= 1 << (8 + $kp_out);
				}
				
				my $kp_data = {
					name		=> $kp_name,
					in			=> \@kp_in_arr,
					out			=> \@kp_out_arr,
					code		=> $keycode,
					map			=> $map ? [split(/\s*,\s*/, $map)] : []
				};
				$self->{keys}->{$kp_name} = $kp_data;
			}
		}
	}
	$self->{cpu}->setGpios($self->{gpios});
	
	close $fp;
}

1;
