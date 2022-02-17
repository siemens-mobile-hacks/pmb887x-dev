#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use JSON::XS;
use Sie::GDBClient;
use Sie::Utils;

my $ESCAPE_TABLE = {
	'"'		=> '\\"',
	"\n"	=> "\\n",
	"\r"	=> "\\r",
	"\t"	=> "\\t",
	"\\"	=> "\\\\"
};

$| = 1;

my $gdb = Sie::GDBClient->new;

my $functions = getFunctions(getDataDir()."/trace/".($ARGV[0] || "EL71").".json");
my $variables = {};

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
		
		printf("%08X: %s(%s) from 0x%08X\n", $func->{addr}, $func->{name}, decodeArgs($gdb, $func, $regs, $variables), $regs->{lr});
		
		# stop
		last if !$gdb->continue('s');
	}
	print STDERR "GDB is lost!\n";
	sleep(1);
}

sub resolveRef {
	my ($gdb, $v, $arg, $size) = @_;
	
	my $ptr = [];
	while ($arg =~ /\*$/) {
		push @$ptr, sprintf("*0x%08X", $v);
		my $mem = $gdb->readMem($v, $arg =~ /[*]{2,}$/ ? 4 : int($size / 8));
		
		if (!defined $mem) {
			$v = 0;
			last;
		}
		
		if ($size == 32) {
			$v = unpack("V", $mem);
		} elsif ($size == 16) {
			$v = unpack("v", $mem);
		} elsif ($size == 8) {
			$v = ord($mem);
		} else {
			die("invalid size: $size");
		}
		
		$arg = substr($arg, 0, length($arg) - 1);
	}
	
	return ($v, $ptr);
}

sub decodeArgs {
	my ($gdb, $func, $regs, $variables) = @_;
	
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
		
		my $ptr = [];
		
		if ($arg =~ /^uint(32|16|8)/) {
			my $size = $1;
			
			($v, $ptr) = resolveRef($gdb, $v, $arg, $size);
			
			push @decoded, sprintf("0x%02X", $v);
		} elsif ($arg =~ /^int(32|16|8)/) {
			my $size = $1;
			
			($v, $ptr) = resolveRef($gdb, $v, $arg, $size);
			
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
			
			push @decoded, sprintf("(*0x%08X)", $v).'"'.escapeStr($str).'"';
		} elsif ($arg =~ /^cstr/) {
			my $index = 0;
			my $chunk = 64;
			my $str = "";
			
			($v, $ptr) = resolveRef($gdb, $v, $arg, 32);
			
			while (1) {
				my $c = $gdb->readMem($v + $index, $chunk);
				
				$str .= $c;
				
				my $zero = index($str, "\0");
				if ($zero >= 0) {
					$str = substr($str, 0, $zero);
					last;
				}
				
				$index += $chunk;
			}
			
			push @decoded, sprintf("(*0x%08X)", $v).'"'.escapeStr($str).'"';
		} else {
			push @decoded, sprintf("UNK(*%08X)", $v);
		}
		
		$decoded[scalar(@decoded) - 1] = "(".join("->", @$ptr).")".$decoded[scalar(@decoded) - 1] if @$ptr;
		
		$arg_id++;
	}
	
	if (@decoded) {
		if ($func->{name} =~ /^NU_Create_(.*?)$/) {
			my $name = $decoded[1];
			$name =~ s/.*?"(.*?)".*?/$1/g;
			$name =~ s/\s/_/g;
			
			$variables->{$decoded[0]} = $decoded[0]."_".$name;
			
			$decoded[0] = $variables->{$decoded[0]};
		} elsif ($func->{name} =~ /^NU_(.*?)$/) {
			$decoded[0] = $variables->{$decoded[0]} if exists $variables->{$decoded[0]};
		}
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

sub escapeStr {
	my ($str) = @_;
	$str =~ s/([^\w\d])/_escapeChar($1)/ge;
	return $str;
}

sub _escapeChar {
	return $ESCAPE_TABLE->{$_[0]} if exists $ESCAPE_TABLE->{$_[0]};
	
	my $code = ord($_[0]);
	return sprintf("\\x%02X", $code) if ($code < 0x20 || $code >= 0x7F);
	
	return $_[0];
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
