package Sie::BoardMetadata;

use warnings;
use strict;
use File::Basename;
use Data::Dumper;
use Storable qw(dclone);
use List::Util qw(min max);
use Sie::CpuMetadata;
use Sie::Utils;
use TOML::Tiny;
use File::Slurp;
use Cwd qw(realpath);

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
	for my $file (sort @files) {
		next if !-f $path."/".$file;
		next if $file !~ /\.toml$/i;
		$file =~ s/\.toml$//gi;
		push @$boards, $file;
	}
	return $boards;
}

sub loadBoard {
	my ($self, $board) = @_;
	
	my $file = getDataDir().'/board/'.$board.'.toml';
	
	$self->{name} = $board;
	$self->{keys} = {};
	
	my $toml_type = sub {
		my ($r) = @_;
		my $type = ref($r);
		if ($type eq "ARRAY") {
			for my $item (@$r) {
				return "ARRAY" if ref($item) ne "HASH";
			}
			return "ARRAY_OF_HASH";
		}
		return $type;
	};

	my $toml_merge;
	$toml_merge = sub {
		my ($r1, $r2) = @_;

		for my $k (keys %$r2) {
			my $r1t = $toml_type->($r1->{$k});
			my $r2t = $toml_type->($r2->{$k});

			if (exists $r1->{$k}) {
				my $r1t = $toml_type->($r1->{$k});
				my $r2t = $toml_type->($r2->{$k});

				if ($r1t && $r1t eq $r2t) {
					if ($r1t eq "HASH") {
						$toml_merge->($r1->{$k}, $r2->{$k});
					} elsif ($r1t eq "ARRAY_OF_HASH") {
						$r1->{$k} = [
							@{$r1->{$k}},
							@{$r2->{$k}}
						];
					} else {
						$r1->{$k} = $r2->{$k};
					}
				} else {
					$r1->{$k} = $r2->{$k};
				}
			} else {
				$r1->{$k} = $r2->{$k};
			}
		}

		return $r1;
	};

	my $cfg = TOML::Tiny::from_toml(scalar(read_file($file)));
	if (exists $cfg->{board}->{extends}) {
		my $base_config_file = dirname(realpath($file))."/".$cfg->{board}->{extends};
		my $base_cfg = TOML::Tiny::from_toml(scalar(read_file($base_config_file)));
		$cfg = $toml_merge->($base_cfg, $cfg);
	}

	$self->{model} = $cfg->{board}->{model};
	$self->{vendor} = $cfg->{board}->{vendor};
	$self->{cpu} = Sie::CpuMetadata->new($cfg->{board}->{cpu}->{type});
	
	if (exists $cfg->{gpio}->{aliases}) {
		for my $gpio_cpu_name (keys %{$cfg->{gpio}->{aliases}}) {
			my $gpio_name = $cfg->{gpio}->{aliases}->{$gpio_cpu_name};
			$self->{gpios}->{$gpio_cpu_name} = {
				name	=> $gpio_name,
				mode	=> "none",
				value	=> undef
			};
		}
	}
	
	if (exists $cfg->{gpio}->{inputs}) {
		for my $gpio_cpu_name (keys %{$cfg->{gpio}->{inputs}}) {
			my $gpio_value = $cfg->{gpio}->{inputs}->{$gpio_cpu_name};
			$self->{gpios}->{$gpio_cpu_name}->{mode} = 'input';
			$self->{gpios}->{$gpio_cpu_name}->{value} = $gpio_value;
		}
	}
	
	if (exists $cfg->{keyboard}) {
		for my $kp_name (keys %{$cfg->{keyboard}}) {
			my @arr = @{$cfg->{keyboard}->{$kp_name}};
			my @kp_in_arr = shift @arr;
			my @kp_out_arr = @arr;

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
