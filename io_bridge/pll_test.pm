package siemens_boot;

use warnings;
use strict;
use Device::SerialPort;
require "test.pm";

sub boot_module_init {
	my $port = shift;
	
	$port->read_char_time(1000);
	$port->read_const_time(1000);
	cmd_ping($port);
	
	print "get_cpu_freq() = ".get_cpu_freq($port)."\n";
}

sub get_cpu_freq {
	my $port = shift;
	my $pll_osc  = cmd_read($port, 0xF45000A0);
	my $pll_con0 = cmd_read($port, 0xF45000A4);
	my $pll_con1 = cmd_read($port, 0xF45000A8);
	my $pll_con2 = cmd_read($port, 0xF45000AC);
	
	printf("pll_osc = %08X\n", $pll_osc);
	printf("pll_con0 = %08X\n", $pll_con0);
	printf("pll_con1 = %08X\n", $pll_con1);
	printf("pll_con2 = %08X\n", $pll_con2);
	
	my $div  = ($pll_osc  & 0x0F000000) >> 24;
	my $mul  = ($pll_osc  & 0x003F0000) >> 16;
	my $pdiv = ($pll_con2 & 0x00000300) >> 8;
	
	my $divh = ($pll_con0 & 0x00007800) >> 11;
	my $mulh = ($pll_con1 & 0x00600000) >> 21;
	
	print "div=$div\n";
	print "mul=$mul\n";
	print "pdiv=$pdiv\n";
	print "divh=$divh\n";
	print "mulh=$mulh\n";
	
	my $fsys = ((26 * ($mul + 1)) / ($div + 1)) / 4;
	
	if ($mulh > 1) {
		if ($divh > 0) {
			if ($pll_con2 & 0x00001000) {
				return  (($fsys * 4 * $mulh) / ($pdiv + 1)) / $divh;
			} else {
				return (($fsys * 4 * $mulh) * 1) / $divh
					if ($pdiv == 0);
				return ((($fsys * 4 * $mulh) * 7500) / 10000) / $divh
					if ($pdiv == 1);
				return ((($fsys * 4 * $mulh) * 6250) / 10000) / $divh
					if ($pdiv == 2);
				return ((($fsys * 4 * $mulh) * 5625) / 10000) / $divh
					if ($pdiv == 3);
				return $fsys;
			}
		} else  {
			return $fsys;
		}
	} else {
		if ($pll_con2 & 0x00001000) {
			return (($fsys * 4 * $mulh) / ($pdiv + 1));
		} else {
			return ($fsys * 4 * $mulh) * 1
				if ($pdiv == 0);
			return  (($fsys * 4 * $mulh) * 7500) / 10000
				if ($pdiv == 1);
			return  (($fsys * 4 * $mulh) * 6250) / 10000
				if ($pdiv == 2);
			return  (($fsys * 4 * $mulh) * 5625) / 10000
				if ($pdiv == 3);
			return $fsys;
		}
	}
}
