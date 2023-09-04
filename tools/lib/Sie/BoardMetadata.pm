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
	$self->{keys} = {};
	$self->{memory} = {};
	
	my $cfg = parseIniFile($file);
	
	$self->{display} = $cfg->{display};
	$self->{model} = $cfg->{device}->{model};
	$self->{vendor} = $cfg->{device}->{vendor};
	$self->{cpu} = Sie::CpuMetadata->new($cfg->{device}->{cpu});
	
	if (exists $cfg->{'gpio-aliases'}) {
		for my $gpio_cpu_name (keys %{$cfg->{'gpio-aliases'}}) {
			my $gpio_name = $cfg->{'gpio-aliases'}->{$gpio_cpu_name};
			$self->{gpios}->{$gpio_cpu_name} = {
				name	=> $gpio_name,
				mode	=> "none",
				value	=> undef
			};
		}
	}
	
	if (exists $cfg->{'gpio-inputs'}) {
		for my $gpio_cpu_name (keys %{$cfg->{'gpio-inputs'}}) {
			my $gpio_value = $cfg->{'gpio-inputs'}->{$gpio_cpu_name};
			$self->{gpios}->{$gpio_cpu_name}->{mode} = 'input';
			$self->{gpios}->{$gpio_cpu_name}->{value} = $gpio_value;
		}
	}
	
	for my $cs (keys %{$cfg->{'memory'}}) {
		my $m = $cfg->{'memory'}->{$cs};
		if ($m =~ /^flash:([a-f0-9]+):([a-f0-9]+)$/i) {
			$self->{memory}->{$cs} = {
				type	=> "flash",
				vid		=> hex $1,
				pid		=> hex $2
			};
		} elsif ($m =~ /^ram:(\d+)m$/i) {
			$self->{memory}->{$cs} = {
				type	=> "ram",
				size	=> $1 * 1024 * 1024
			};
		} else {
			die "Invalid memory: $m";
		}
	}
	
	if (exists $cfg->{'keyboard'}) {
		for my $kp_name (keys %{$cfg->{'keyboard'}}) {
			my ($kp_in_str, $kp_out_str) = split(":", $cfg->{'keyboard'}->{$kp_name});
			
			my @kp_in_arr = map { int($_) } split(/\s*,\s*/, $kp_in_str);
			my @kp_out_arr = map { int($_) } split(/\s*,\s*/, $kp_out_str);
			
			my $keycode = 0;
			
			for my $kp_in (@kp_in_arr) {
				$keycode |= 1 << $kp_in;
			}
			
			for my $kp_out (@kp_out_arr) {
				$keycode |= 1 << (8 + $kp_out);
			}
			
			$self->{keys}->{$kp_name} = {
				in			=> \@kp_in_arr,
				out			=> \@kp_out_arr,
				code		=> $keycode,
			};
		}
	}
	
	$self->{cpu}->setGpios($self->{gpios});
	
}

1;
