#!/usr/bin/perl
package Sie::CGSN;

use warnings;
use strict;
use File::Basename qw(dirname);
use Getopt::Long;
use lib(dirname(__FILE__).'/lib');
use List::Util qw(min max);
use Data::Dumper;

# AT+CGSN:A0000000,00000008
# Read data from memory address A0000000 Lenght 8 Bytes (If Size is not specified then 
# read 4 bytes)

# AT+CGSN@A08E8DE4,0000001A,00004225
# Call address A08E8DE4, R0 = 0000001A, R1 = 00004225 (R0-R7 Can be given)
# Out is the R0-R12 Registers Dump and CPSR

# AT+CGSN*A80B180011223344....
# Write Data to RAM. address A80B1800 
# Max Size 128 bytes per command 

# AT+CGSN%A0000000A0000004A0000008....
# Query addresses. Return values for each address in one line.

sub new {
	my ($class, $serial) = @_;
	
	my $self = {
		serial 	=> $serial,
	};
	bless $self, $class;
	
	return $self;
}

sub connect {
	my ($self, $max_speed) = @_;
	
	# Read trash from port
	# $self->{serial}->readChunk(4096, 10);
	
	for my $speed (921600, 576000, 500000, 460800, 230400, 115200, 57600) {
		next if $speed > $max_speed;
		
		print STDERR "Trying speed $speed...\n";
		
		$self->{serial}->setSpeed($speed);
		for (my $i = 0; $i < 3; $i++) {
			return 1 if $self->handshake();
		}
	}
	
	return 0;
}

sub handshake {
	my ($self) = @_;
	my $response = $self->{serial}->sendAt("ATQ0 V1 E0\r", undef, 50);
	return 0 if $response->{status} ne "OK";
	return 1;
}

sub exec {
	my ($self, $addr, @args) = @_;
	
	die "Only R0...R7!" if @args > 8;
	
	my $cmd = sprintf("AT+CGSN@%08X,%s\r", $addr, join ",", map { sprintf("%08X", $_) } @args);
	
	my $resp_len = 56 + 1;
	my $response = $self->{serial}->sendAt("$cmd\r", "binary:57", 10000);
	
	if ($response->{status} ne "OK") {
		warn "$cmd: command failed, status=".$response->{status};
		return undef;
	}
	
	my ($ack, @regs) = unpack("WVVVVVVVVVVVVVV", $response->{response});
	if ($ack != 0xA1) {
		warn "$cmd: invalid ack: ".sprintf("%02X", $ack);
		return undef;
	}
	
	return {
		CPSR => $regs[13], 
		R0 => $regs[0], R1 => $regs[1], R2 => $regs[2], R3 => $regs[3], 
		R4 => $regs[4], R5 => $regs[5], R6 => $regs[6], R7 => $regs[7], 
		R8 => $regs[8], R9 => $regs[9], R10 => $regs[10], R11 => $regs[11], 
		R12 => $regs[12]
	};
}

sub writeMemory {
	my ($self, $addr, $data) = @_;
	
	my $size = length($data);
	for (my $offset = 0; $offset < $size; $offset += 128) {
		my $cmd = sprintf("AT+CGSN*%08X%s", $addr, bin2hex(substr($data, $offset, 128)));
		
		my $resp_len = $size + 1;
		my $response = $self->{serial}->sendAt("$cmd\r", "binary:1", 600);
		
		if ($response->{status} ne "OK") {
			warn "$cmd: command failed, status=".$response->{status};
			return 0;
		}
		
		my $ack = ord(substr($response->{response}, 0, 1));
		if ($ack != 0xA1) {
			warn "$cmd: invalid ack: ".sprintf("%02X", $ack);
			return 0;
		}
	}
	
	return 1;
}

sub readMemory {
	my ($self, $addr, $size) = @_;
	
	my $out = "";
	for (my $offset = 0; $offset < $size; $offset += 128) {
		my $chunk_size = min(128, $size - $offset);
		my $cmd = sprintf("AT+CGSN:%08X,%08X", $addr + $offset, $chunk_size);
		
		my $resp_len = $chunk_size + 1;
		my $response = $self->{serial}->sendAt("$cmd\r", "binary:$resp_len", 600);
		
		if ($response->{status} ne "OK") {
			warn "$cmd: command failed, status=".$response->{status};
			return undef;
		}
		
		my $ack = ord(substr($response->{response}, 0, 1));
		if ($ack != 0xA1) {
			warn "$cmd: invalid ack: ".sprintf("%02X", $ack);
			return undef;
		}
		
		$out .= substr($response->{response}, 1);
	}
	
	return $out;
}

sub close {
	my ($self) = @_;
	$self->{serial}->close;
}

1;
