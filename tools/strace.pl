#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use JSON::XS;
use Sie::GDBClient;
use Sie::Utils;

$| = 1;

my $gdb = Sie::GDBClient->new;

my $functions = getFunctions($ARGV[0] || getDataDir()."/trace/EL71.json");

while (1) {
	print STDERR "Connecting...\n";
	while (!$gdb->connect) {
		sleep(1);
	}
	print STDERR "GDB found.\n";
	
	# add breakpoints
	next if !addBreakpoints($gdb, $functions);
	
	while ($gdb->connected) {
		# start program
		my $stop_info = $gdb->continue('c');
		last if !$stop_info;
		
		my $regs = $gdb->registers();
		last if !$regs;
		
		my $func = $functions->{$regs->{pc}};
		
		printf("%08X: %s(%s) from 0x%08X\n", $func->{addr}, $func->{name}, decodeArgs($gdb, $func, $regs), $regs->{lr});
		
		# stop
		last if !$gdb->continue('s');
	}
	print STDERR "GDB is lost!\n";
	sleep(1);
}

sub decodeArgs {
	my ($gdb, $func, $regs) = @_;
	
	my $arg_id = 0;
	my @decoded;
	
	my $sp_index = 0;
	for my $arg (@{$func->{args}}) {
		my $v = 0;
		if ($arg_id < 4) {
			$v = $regs->{"r$arg_id"};
		} else {
			$v = unpack("L", $gdb->readMem($regs->{sp} + (4 * $sp_index), 4));
			$sp_index++;
		}
		
		if ($arg =~ /^uint(32|16|8)$/) {
			push @decoded, sprintf("%d", $v);
		} elsif ($arg =~ /^int(32|16|8)$/) {
			my $size = $1;
			
			my $sign = $v & (1 << $size);
			$v = ($sign ? -1 : 1) * ($v & ~$sign);
			
			push @decoded, sprintf("%d", $v);
		} elsif ($arg eq "ptr") {
			push @decoded, sprintf("*0x%08X", $v);
		} elsif ($arg eq "exit") {
			my $index = 0;
			my $str = "";
			
			while (1) {
				my $c = $gdb->readMem($v + $index, 1);
				
				last if (ord($c) > 0xA0 && $index >= 2);
				
				$str .= $c if ($index >= 2);
				
				last if ($c eq "\0");
				
				$index++;
			}
			
			push @decoded, sprintf("(*0x%08X)", $v).'"'.$str.'"';
		} elsif ($arg eq "cstr") {
			my $index = 0;
			my $str = "";
			
			while (1) {
				my $c = $gdb->readMem($v + $index, 1);
				$str .= $c;
				
				if ($index >= 0x100) {
					warn "Corrupted string???";
					last;
				}
				
				last if ($c eq "\0");
				
				$index++;
			}
			
			push @decoded, sprintf("(*0x%08X)", $v).'"'.$str.'"';
		} else {
			push @decoded, sprintf("UNK(*%08X)", $v);
		}
		
		$arg_id++;
	}
	
	return join(", ", @decoded);
}

sub getFunctions {
	my ($file) = @_;
	
	open(F, $file) or die("open($file): $!");
	my $json = "";
	while (!eof(F)) {
		read F, my $tmp, 4096;
		$json .= $tmp;
	}
	close F;
	
	$json =~ s/0x([a-f0-9]+)/hex($1)/gei;
	
	my $functions = {};
	for my $func (@{JSON::XS->new->decode($json)}) {
		next if $func->{skip};
		
		if (($func->{addr} & 1)) {
			$func->{thumb} = 1;
			$func->{addr} &= ~1;
		}
		
		$functions->{$func->{addr}} = $func;
	}
	
	return $functions;
}

sub printRegisters {
	my ($regs) = @_;
	
	printf("PSR %08X\n", $regs->{cpsr});
	printf("R0  %08X  R1  %08X R2  %08X R3  %08X\n", $regs->{r0}, $regs->{r1}, $regs->{r2}, $regs->{r3});
	printf("R4  %08X  R5  %08X R6  %08X R7  %08X\n", $regs->{r4}, $regs->{r5}, $regs->{r6}, $regs->{r7});
	printf("R8  %08X  R9  %08X R10 %08X R11 %08X\n", $regs->{r8}, $regs->{r9}, $regs->{r10}, $regs->{r11});
	printf("R12 %08X  SP  %08X LR  %08X PC  %08X\n", $regs->{r12}, $regs->{sp}, $regs->{lr}, $regs->{pc});
}

sub addBreakpoints {
	my ($gdb, $functions) = @_;
	for my $func (values %$functions) {
		if ($func->{thumb}) {
			return 0 if !$gdb->addBreakPoint(0, $func->{addr}, 2);
		} else {
			return 0 if !$gdb->addBreakPoint(0, $func->{addr}, 4);
		}
	}
	return 1;
}

sub removeBreakpoints {
	my ($gdb, $functions) = @_;
	for my $func (values %$functions) {
		if ($func->{thumb}) {
			return 0 if !$gdb->removeBreakPoint(0, $func->{addr}, 2);
		} else {
			return 0 if !$gdb->removeBreakPoint(0, $func->{addr}, 4);
		}
	}
	return 1;
}
