use warnings;
use strict;
use Data::Dumper;

require "common/perl/regs.pm";

my $header = "";
my $hash = [];
my $regs = get_regs();
my $name_len = 0;

my $rules = [
	{
		prefix	=> "STM_", 
		name	=> "STM", 
		define	=> [
			["STM"]
		], 
		arrays	=> [
			{
				regexp	=> qr/STM_(\d+)$/, 
				name	=> "TIM", 
				index	=> sub { return $1 }
			}
		]
	}, 
	{
		prefix	=> "MCI_", 
		name	=> "MCI", 
		define	=> [
			["MCI0"]
		], 
		arrays	=> [
			{
				regexp	=> qr/MCI_FIFO_(\d+)$/, 
				name	=> "FIFO", 
				index	=> sub { return $1 }
			}
		]
	}, 
	{
		prefix	=> "DMA_", 
		name	=> "DMA", 
		define	=> [
			["DMA0"]
		], 
		arrays	=> [
			{
				regexp	=> qr/DMA_CH(\d+)_SRC_ADDR$/, 
				name	=> "CH_SRC_ADDR", 
				index	=> sub { return $1 }
			}, 
			{
				regexp	=> qr/DMA_CH(\d+)_DST_ADDR$/, 
				name	=> "CH_DST_ADDR", 
				index	=> sub { return $1 }
			}, 
			{
				regexp	=> qr/DMA_CH(\d+)_LLI$/, 
				name	=> "CH_LLI", 
				index	=> sub { return $1 }
			}, 
			{
				regexp	=> qr/DMA_CH(\d+)_CONTROL$/, 
				name	=> "CH_CONTROL", 
				index	=> sub { return $1 }
			}, 
			{
				regexp	=> qr/DMA_CH(\d+)_CONFIG$/, 
				name	=> "CH_CONFIG", 
				index	=> sub { return $1 }
			}
		]
	}
];

my $structs = {};
for my $val (@$regs) {
	if (!$val->{addr_end}) {
		for my $rule (@$rules) {
			next if ($rule->{addr} && ($val->{addr} < $rule->{addr}->[0] || $val->{addr} > $rule->{addr}->[1]));
			
			if ($val->{name} =~ /^\Q$rule->{prefix}\E/) {
				if (!$structs->{$rule->{prefix}}) {
					$structs->{$rule->{prefix}} = {
						data	=> $rule, 
						fields	=> []
					};
				}
				
				my $is_array = 0;
				for my $arr (@{$rule->{arrays}}) {
					if ($val->{name} =~ /$arr->{regexp}/) {
						my $n = $arr->{index}->();
						
						$is_array = 1;
						
						my $field;
						for my $f (@{$structs->{$rule->{prefix}}->{fields}}) {
							if ($f->{type} eq "array" && $f->{name} eq $arr->{name}) {
								$field = $f;
								last;
							}
						}
						
						if (!$field) {
							$field = {
								type		=> "array", 
								data		=> $val, 
								addr		=> $val->{addr}, 
								name		=> $arr->{name}, 
								elements	=> []
							};
							push @{$structs->{$rule->{prefix}}->{fields}}, $field;
						}
						
						push @{$field->{elements}}, {
							n		=> $n, 
							data	=> $val
						};
						
						$field->{addr} = $val->{addr}
							if ($field->{addr} > $val->{addr});
					}
				}
				
				if (!$is_array) {
					push @{$structs->{$rule->{prefix}}->{fields}}, {
						type	=> "field", 
						addr	=> $val->{addr}, 
						name	=> $val->{name}, 
						data	=> $val
					};
				}
			}
		}
	}
}

print "#pragma once\n\n#include <stdint.h>\n\n";

for my $struct (values %$structs) {
	$struct->{fields} = [sort {
		$a->{addr} <=> $b->{addr}
	} @{$struct->{fields}}];
	
	my $start = $struct->{fields}->[0]->{addr};
	
	for my $d (@{$struct->{data}->{define}}) {
		my $name = $d->[0];
		my $addr = $d->[1] || $start;
		
		printf("#define\t%s_BASE\t(0x%08X)\n", $name, $addr);
		print "#define\t$name\t((volatile ".$struct->{data}->{name}."_TypeDef *) ".$name."_BASE)\n\n";
	}
	
	print "typedef struct {\n";
	
	my $last = 0;
	my $unk = 0;
	for my $field (@{$struct->{fields}}) {
		my $reg_name = $field->{name};
		$reg_name =~ s/^\Q$struct->{data}->{prefix}\E//g;
		
		if ($last && $field->{addr} - $last > 4) {
			print "\tuint32_t _RESERVED".($unk++)."[".(($field->{addr} - $last) / 4 - 1)."];\n";
		}
		
		$last = $field->{addr};
		
		if ($field->{type} eq "array") {
			$reg_name .= "[".scalar(@{$field->{elements}})."]";
			$last = $field->{addr} + (scalar(@{$field->{elements}}) - 1) * 4;
		}
		
		print "\tunion {\n";
		print "\t\tstruct {\n";
		
		my $bits = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
		for my $bit (@{$field->{data}->{desc}}) {
			for (my $i = $bit->{shift}; $i < ($bit->{shift} + $bit->{mask}); ++$i) {
				$bits->[$i] = 1;
			}
		}
		
		for my $bit_n (keys @$bits) {
			my $bit = $bits->[$bit_n];
			push @{$field->{data}->{desc}}, {
				name	=> "_bit$bit_n", 
				values	=> {}, 
				shift	=> $bit_n, 
				mask	=> 1
			} if (!$bit);
		}
		
		$field->{data}->{desc} = [sort {
			$a->{shift} <=> $b->{shift}
		} @{$field->{data}->{desc}}];
		
		for my $bit (@{$field->{data}->{desc}}) {
			print "\t\t\tuint32_t ".$bit->{name}.":".$bit->{mask}."; /* [".$bit->{shift}."..".($bit->{shift} + $bit->{mask})."] */\n";
		}
		
		print "\t\t} b;\n";
		print "\t\tuint32_t v;\n";
		print "\t} $reg_name; ".sprintf("/* %08X */\n", $field->{addr})."\n";
	}
	
	print "} __attribute__((aligned(4))) ".$struct->{data}->{name}."_TypeDef;\n\n";
}
