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

# TOML::Tiny only implements TOML 1.0, which keeps inline tables on one line.
# Board configs use multi-line inline tables (TOML 1.1), so join them first.
sub joinInlineTables {
	my ($text) = @_;
	my $result = '';
	my $depth = 0;
	my $quote;
	my $length = length($text);

	for (my $i = 0; $i < $length; $i++) {
		my $c = substr($text, $i, 1);

		if (defined $quote) {
			if ($c eq '\\' && substr($quote, 0, 1) eq '"') {
				$result .= substr($text, $i++, 2);
			} elsif (substr($text, $i, length($quote)) eq $quote) {
				$result .= $quote;
				$i += length($quote) - 1;
				undef $quote;
			} else {
				$result .= $c;
			}
			next;
		}

		if ($c eq '"' || $c eq "'") {
			$quote = substr($text, $i, 3) eq $c x 3 ? $c x 3 : $c;
			$result .= $quote;
			$i += length($quote) - 1;
			next;
		}

		if ($c eq '#') {
			my $end = index($text, "\n", $i);
			$end = $length if $end < 0;
			$result .= substr($text, $i, $end - $i) if $depth == 0;
			$i = $end - 1;
			next;
		}

		$depth++ if $c eq '{';
		$depth-- if $c eq '}';
		$c = ' ' if $depth > 0 && ($c eq "\n" || $c eq "\r");
		$result .= $c;
	}

	return $result;
}

sub parseToml {
	my ($file) = @_;
	return TOML::Tiny::from_toml(joinInlineTables(scalar(read_file($file))));
}

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

	my $load_config;
	$load_config = sub {
		my ($config_file, $depth) = @_;
		die "$config_file: too many nested extends\n" if $depth > 8;

		my $config = parseToml($config_file);
		return $config if !exists $config->{board}->{extends};

		my $base_config_file = dirname(realpath($config_file))."/".$config->{board}->{extends};
		return $toml_merge->($load_config->($base_config_file, $depth + 1), $config);
	};

	my $cfg = $load_config->($file, 0);

	$self->{model} = $cfg->{board}->{model};
	$self->{vendor} = $cfg->{board}->{vendor};
	$self->{platform} = $cfg->{board}->{platform};
	$self->{hw_platform} = $cfg->{board}->{hw_platform};
	$self->{peripherals} = $cfg->{peripheral} || {};
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
			next if $kp_name eq 'gpio';

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
