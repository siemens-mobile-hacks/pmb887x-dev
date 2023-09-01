#!/usr/bin/env perl
use warnings;
use strict;
use File::Basename;
use lib dirname(__FILE__).'/lib';
use Data::Dumper;
use JSON::PP;
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

while (1) {
	print STDERR "Connecting...\n";
	while (!$gdb->connect) {
		sleep(1);
	}
	print STDERR "GDB found.\n";
	
	my $functions = getFunctions(getDataDir()."/trace/".($ARGV[0] || "EL71").".json");
	my $variables = {};
	
	# add breakpoints
	for my $func (values %$functions) {
		addBreakpoint($gdb, $func);
	}
	
	while ($gdb->connected) {
		# start program
		my $stop_info = $gdb->continue('c');
		last if !$stop_info;
		
		my $regs = $gdb->registers();
		last if !$regs;
		
		if ($stop_info->{watchpoint}) {
			die sprintf("Unknown watchpoint: %08X", $stop_info->{watchpoint}->{addr})
				if !exists $functions->{$stop_info->{watchpoint}->{addr}};
			
			my $func = $functions->{$stop_info->{watchpoint}->{addr}};
			my $old_value = unpack("L", $gdb->readMem($func->{addr}, 4));
			
			# remove breakpoint
			last if !removeBreakpoint($gdb, $func);
			
			# step
			last if !$gdb->continue('s');
			
			if ($func->{watch} eq "r") {
				printf("%08X: %s %08X from 0x%08X\n", $func->{addr}, $func->{name}, $old_value, $regs->{pc});
			} else {
				my $new_value = unpack("L", $gdb->readMem($func->{addr}, 4));
				printf("%08X: %s %08X -> %08X from 0x%08X\n", $func->{addr}, $func->{name}, $old_value, $new_value, $regs->{pc});
			}
			
			printRegisters($regs) if $func->{regs};
			printStack($gdb, $regs, $func->{stack}) if $func->{stack};
			
			for my $dump_addr (@{$func->{dump}}) {
				my $val = unpack("L", $gdb->readMem($dump_addr, 4));
				printf("  %08X: %08X\n", $dump_addr, $val);
			}
			
			# add watchpoint again
			last if !addBreakpoint($gdb, $func);
		} else {
			die sprintf("Unknown breakpoint: %08X", $regs->{pc})
				if !exists $functions->{$regs->{pc}};
			
			my $func = $functions->{$regs->{pc}};
			printf("%08X: %s(%s) from 0x%08X\n", $func->{addr}, $func->{name}, decodeArgs($gdb, $func, $regs, $variables), $regs->{lr});
			printRegisters($regs) if $func->{regs};
			printStack($gdb, $regs, $func->{stack}) if $func->{stack};
			
			for my $dump_addr (@{$func->{dump}}) {
				my $val = unpack("L", $gdb->readMem($dump_addr, 4));
				printf("  %08X: %08X\n", $dump_addr, $val);
			}
			
			# step
			last if !$gdb->continue('s');
		}
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

sub readFuncArg {
	my ($gdb, $regs, $n) = @_;
	if ($n < 0) {
		return unpack("L", $gdb->readMem($regs->{sp} - 4 * $n, 4));
	} elsif ($n < 4) {
		return $regs->{"r$n"};
	} else {
		return unpack("L", $gdb->readMem($regs->{sp} + (4 * (4 - $n)), 4));
	}
}

sub readFuncArgWithType {
	my ($gdb, $regs, $n, $type, $for_debug) = @_;
	
	my $ptr = [];
	
	my $v = readFuncArg($gdb, $regs, $n);
	if ($type =~ /^uint(32|16|8)/) {
		my $size = $1;
		($v, $ptr) = resolveRef($gdb, $v, $type, $size);
		
		if ($for_debug) {
			return ($ptr, sprintf("0x%X", $v));
		} else {
			return $v;
		}
	} elsif ($type =~ /^int(32|16|8)/) {
		my $size = $1;
		($v, $ptr) = resolveRef($gdb, $v, $type, $size);
		
		my $sign = $v & (1 << $size);
		$v = ($sign ? -1 : 1) * ($v & ~$sign);
		
		if ($for_debug) {
			return ($ptr, $v);
		} else {
			return $v;
		}
	} elsif ($type eq "ptr") {
		if ($for_debug) {
			return ($ptr, sprintf("0x%X", $v));
		} else {
			return $v;
		}
	} elsif ($type eq "memcmp") {
		my $sz = readFuncArgWithType($gdb, $regs, 2, "uint32", 0);
		my $mem = $gdb->readMem($v, $sz);
		if ($for_debug) {
			return ($ptr, $v ? sprintf("*0x%08X(%s)", $v, bin2hex($mem)) : "NULL");
		} else {
			return $mem;
		}
	} elsif ($type =~ /^memdump:(\d+)/) {
		my $sz = $1;
		my $mem = $gdb->readMem($v, $sz);
		if ($for_debug) {
			return ($ptr, $v ? sprintf("*0x%08X(%s)", $v, bin2hex($mem)) : "NULL");
		} else {
			return $mem;
		}
	} elsif ($type eq "exit") {
		my $index = 0;
		my $str = "";
		
		while (1) {
			my $c = $gdb->readMem($v + $index, 1);
			
			last if (ord($c) > 0xA0 && $index >= 2);
			
			$str .= $c if ($index >= 2);
			
			last if ($c eq "\0");
			
			$index++;
		}
		
		if ($for_debug) {
			return ($ptr, sprintf("(*0x%08X)", $v).'"'.escapeStr($str).'"');
		} else {
			return $str;
		}
	} elsif ($type =~ /^cstr/) {
		my $index = 0;
		my $chunk = 64;
		my $str = "";
		
		($v, $ptr) = resolveRef($gdb, $v, $type, 32);
		
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
		
		if ($for_debug) {
			# return ($ptr, sprintf("(*0x%08X)", $v).'"'.escapeStr($str).'"');
			return ($ptr, '"'.escapeStr($str).'"');
		} else {
			return $str;
		}
	} elsif ($type eq "ptr") {
		if ($for_debug) {
			return ($ptr, sprintf("*0x%08X", $v));
		} else {
			return $v;
		}
	} elsif ($type =~ /^(r\d+|sp|lr|pc)$/) {
		my $r = $1;
		
		if ($for_debug) {
			return ($ptr, "$r=".$regs->{$r});
		} else {
			return $regs->{$r};
		}
	
	} elsif ($type eq "fmt") {
		my $fmt = readFuncArgWithType($gdb, $regs, $n, "cstr", 0);
		my $str = parseSprintf($fmt, sub {
			my ($part_id, $part_type) = @_;
			return readFuncArgWithType($gdb, $regs, -($part_id + 1), $part_type, 0);
		});
		
		if ($for_debug) {
			return ($ptr, '"'.escapeStr($str).'"');
		} else {
			return $v;
		}
	}
	
	if ($for_debug) {
		return ($ptr, sprintf("UNK(*%08X)", $v));
	} else {
		return $v;
	}
}

sub decodeArgs {
	my ($gdb, $func, $regs, $variables) = @_;
	
	my @decoded;
	
	my $arg_id = 0;
	for my $type (@{$func->{args}}) {
		my ($ptr, $arg_value) = readFuncArgWithType($gdb, $regs, $arg_id, $type, 1);
		push @decoded, $arg_value;
		$decoded[scalar(@decoded) - 1] = "(".join("->", @$ptr).")".$decoded[scalar(@decoded) - 1] if @$ptr;
		$arg_id++;
	}
	
	if (@decoded) {
		if ($func->{name} =~ /^GBS_CreateProc/) {
			my $name = $decoded[1];
			$name =~ s/.*?"(.*?)".*?/$1/g;
			$name =~ s/\s/_/g;
			
			$variables->{$decoded[0]} = $decoded[0]."_".$name;
			
			$decoded[0] = $variables->{$decoded[0]};
		} elsif ($func->{name} =~ /^GBS_(.*?)$/) {
			$decoded[0] = $variables->{$decoded[0]} if exists $variables->{$decoded[0]};
		}
		
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
	for my $func (@{JSON::PP->new->relaxed(1)->decode($json)}) {
		next if $func->{skip};
		
		if (($func->{addr} & 1)) {
			$func->{thumb} = 1;
			$func->{addr} &= ~1;
		}
		
		$functions->{$func->{addr}} = $func;
	}
	
	return $functions;
}

sub printStack {
	my ($gdb, $regs, $deep) = @_;
	
	for (my $i = 0; $i < $deep; $i++) {
		my $addr = $regs->{sp} - 4 * $i;
		my $val = unpack("L", $gdb->readMem($addr, 4));
		printf("SP[%d] %08X: %08X\n", $i, $addr, $val);
	}
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

sub addBreakpoint {
	my ($gdb, $func) = @_;
	if ($func->{watch}) {
		if ($func->{watch} eq "w") {
			return 0 if !$gdb->addBreakPoint(2, $func->{addr}, $func->{size} || 4);
		} elsif ($func->{watch} eq "r") {
			return 0 if !$gdb->addBreakPoint(3, $func->{addr}, $func->{size} || 4);
		} elsif ($func->{watch} eq "rw") {
			return 0 if !$gdb->addBreakPoint(4, $func->{addr}, $func->{size} || 4);
		} else {
			die "Invalid watch: ".$func->{watch};
		}
	} else {
		if ($func->{thumb}) {
			return 0 if !$gdb->addBreakPoint(0, $func->{addr}, 2);
		} else {
			return 0 if !$gdb->addBreakPoint(0, $func->{addr}, 4);
		}
	}
	return 1;
}

sub removeBreakpoint {
	my ($gdb, $func) = @_;
	if ($func->{watch}) {
		if ($func->{watch} eq "w") {
			return 0 if !$gdb->removeBreakPoint(2, $func->{addr}, $func->{size} || 4);
		} elsif ($func->{watch} eq "r") {
			return 0 if !$gdb->removeBreakPoint(3, $func->{addr}, $func->{size} || 4);
		} elsif ($func->{watch} eq "rw") {
			return 0 if !$gdb->removeBreakPoint(4, $func->{addr}, $func->{size} || 4);
		} else {
			die "Invalid watch: ".$func->{watch};
		}
	} else {
		if ($func->{thumb}) {
			return 0 if !$gdb->removeBreakPoint(0, $func->{addr}, 2);
		} else {
			return 0 if !$gdb->removeBreakPoint(0, $func->{addr}, 4);
		}
	}
	return 1;
}

sub parseSprintf {
	my ($fmt, $callback) = @_;
	
	my @args;
	
	my $process_fmt = sub {
		my ($part) = @_;
		
		if ($part =~ /^%\d*(d|u|ld|lu)$/) {
			my $type = $1 eq "d" ? "int32" : "uint32";
			push @args, $callback->(scalar(@args), $type);
		} elsif ($part =~ /^%\d*(l?[xX])$/) {
			push @args, $callback->(scalar(@args), "uint32");
		} elsif ($part =~ /^%\d*(lld|llu)$/) {
			my $type = $1 eq "d" ? "int64" : "uint64";
			push @args, $callback->(scalar(@args), $type);
		} elsif ($part =~ /^%\f*f$/) {
			push @args, $callback->(scalar(@args), "float");
		} elsif ($part =~ /^%\d*s$/) {
			push @args, $callback->(scalar(@args), "cstr");
		} elsif ($part =~ /^%\d*\.\*s$/) {
			push @args, $callback->(scalar(@args), "nstr");
		} elsif ($part =~ /^%c$/) {
			push @args, $callback->(scalar(@args), "char");
		} else {
			die("unknown part: $part\n");
		}
		
		return $part;
	};
	
	$fmt =~ s/(%(%|([l0-9]*)[dxulf]+|[0-9]*s|\.\*s|c))/$process_fmt->($1)/seg;
	
	return sprintf($fmt, @args);
}

sub bin2hex {
	my $hex = shift;
	$hex =~ s/([\W\w])/sprintf("%02X", ord($1))/ge;
	return $hex;
}

sub hex2bin {
	my $hex = shift;
	$hex =~ s/\s+//gim;
	$hex = "0$hex" if (length($hex) % 2 != 0);
	$hex =~ s/([A-F0-9]{2})/chr(hex($1))/ge;
	return $hex;
}
