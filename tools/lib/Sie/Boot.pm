package Sie::Boot;

use warnings;
use strict;
use Time::HiRes;

my $IGNITION_ON_PERIOD = 0.05;
my $IGNITION_OFF_PERIOD = 0.15;

sub detectPhone {
	my ($serial, $ign) = @_;
	
	print "Please, short press red button!\n";
	my $last_dtr = 0;
	my $last_ignition = 0;
	
	$serial->dtr($ign == 2 ? 1 : 0);
	$serial->rts(0);
	
	while (1) {
		if ($ign) {
			my $elapsed = Time::HiRes::time - $last_ignition;
			my $timeout = $last_dtr ? $IGNITION_ON_PERIOD : $IGNITION_OFF_PERIOD;
			if ($elapsed >= $timeout) {
				$last_dtr = !$last_dtr;
				$serial->dtr($last_dtr) if $ign == 1;
				$serial->dtr(!$last_dtr) if $ign == 2;
				$last_ignition = Time::HiRes::time;
			}
		}
		
		$serial->write("AT");
		
		print ".";
		my $c = $serial->getChar(20);
		
		if ($c == 0xB0 || $c == 0xC0) {
			my $type = ($c == 0xB0 ? "SGOLD" : "SGOLD2");
			print "\n$type detected!\n";
			return { type => $type };
		}
		
		print sprintf("%02X", $c) if $c >= 0;
	}
	print "Phone not found!\n";
	return undef;
}

sub loadBootCode {
	my ($serial, $code) = @_;
	
	$serial->write(genPayload($code));
	
	my $response = $serial->getChar(1000);
	
	return (1, "Success.") if $response == 0xc1 || $response == 0xB1;
	return (0, "Phone denied bootloader.") if $response == 0x1C || $response == 0x1B;
	return (0, "Unknown response: ".sprintf("%02X", $response)) if $response >= 0;
	return (0, "Timeout!");
}

sub genPayload {
	my ($code) = @_;
	
	my $len = length($code);
	
	# XOR
	my $chk = 0;
	for (my $i = 0; $i < $len; ++$i) {
		$chk ^= ord(substr($code, $i, 1));
	}
	
	return "\x30".chr($len & 0xFF).chr(($len >> 8) & 0xFF).$code.chr($chk);
}

1;
