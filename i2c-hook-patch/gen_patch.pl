#!/usr/bin/perl
use warnings;
use strict;
use File::Slurp qw|read_file write_file|;

my $flash = 0xA0000000;
my $base = 0xA0020000;

my $functions = {
	i2c_transfer	=> 0xa04f9c04,
	i2c_receive		=> 0xa04f9c58
};

my $patch_body = "";

my $ff = scalar(read_file("EL71_2022-01-29_22-45-49.bin"));

my $syms = `arm-none-eabi-objdump -t app.elf`;
while ($syms =~ /^([a-f0-9]+)\s+g\s+F\s+\.text\s+[a-f0-9]+\s([\w\d_]+)/gm) {
	my $addr = hex $1;
	my $func = $2;
	
	printf("%s(): 0x%08X\n", $func, $addr);
	
	if (!exists $functions->{$func}) {
		die "Unknown symbol: $func";
	}
	
	my $call_addr = $addr;
	
	printf("  -> 0x%08X\n", $call_addr);
	
	#print "code: ".asm2hex(sprintf("B 0x%08X", $call_addr))."\n";
	
	my $new_func_body = "\x04\xF0\x1F\xE5"
		.chr(($call_addr >> 0) & 0xFF)
		.chr(($call_addr >> 8) & 0xFF)
		.chr(($call_addr >> 16) & 0xFF)
		.chr(($call_addr >> 24) & 0xFF);
	
	# patch function in fullflash
	substr($ff, $functions->{$func} - $flash, length($new_func_body)) = $new_func_body;
	
	$patch_body .= sprintf("%08X: %s\n", $functions->{$func} - $flash, bin2hex($new_func_body));
}

my $payload = scalar(read_file("app.bin"));

substr($ff, $base - $flash, length($payload))  =$payload;

$patch_body .= sprintf("%08X: %s\n", $base - $flash, bin2hex($payload));

print "\n\n\npatch:\n$patch_body\n";

write_file("/tmp/ff.bin", $ff);

sub bin2hex {
	my $hex = shift;
	$hex =~ s/([\W\w])/sprintf("%02X", ord($1))/ge;
	return $hex;
}
