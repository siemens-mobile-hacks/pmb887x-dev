package Sie::GDBClient;

#!/usr/bin/env perl
use warnings;
use strict;
use Data::Dumper;
use IO::Socket qw(AF_INET SOCK_STREAM);
use IO::Select;
use Sie::Utils;
use List::Util qw(min max);

use constant CMD_OK				=> 0;
use constant CMD_NACK			=> -1;
use constant CMD_INVALID_RESP	=> -2;
use constant CMD_CANCEL			=> -3;

my $DEFAULT_REGS = [qw(r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 sp lr pc cpsr)];

sub new {
	my ($class, $cpu) = @_;
	
	my $self = bless { } => $class;
	
	$self->{select} = new IO::Select;
	
	return $self;
}

sub connect {
	my ($self) = @_;
	
	if ($self->{sock}) {
		$self->{select}->remove($self->{sock});
		$self->{sock}->close();
	}
	
	$self->{sock} = IO::Socket->new(
		Domain		=> AF_INET,
		Type		=> SOCK_STREAM,
		proto		=> 'tcp',
		PeerPort	=> 1234,
		PeerHost	=> 'localhost',
		Blocking	=> 0
	) or die "Can't open socket: $!";
	
	$self->{select}->add($self->{sock});
	
	$self->{connected} = $self->{sock}->connected ? 1 : 0;
	
	return $self->{connected};
}

sub addBreakPoint {
	my ($self, $type, $addr, $len) = @_;
	my ($ret) = $self->exec('Z', [sprintf("%x", $type), sprintf("%x", $addr), sprintf("%x", $len)], "OK");
	return $ret == CMD_OK;
}

sub removeBreakPoint {
	my ($self, $type, $addr, $len) = @_;
	my ($ret) = $self->exec('z', [sprintf("%x", $type), sprintf("%x", $addr), sprintf("%x", $len)], "OK");
	return $ret == CMD_OK;
}

sub readMem {
	my ($self, $addr, $size) = @_;
	my ($ret, $response) = $self->exec('m', [sprintf("%x", $addr), sprintf("%x", $size)]);
	return undef if ($ret != CMD_OK);
	
	if (length($response) != $size * 2) {
		print STDERR "[gdb-client] Unknown response: '$response'\n";
		return undef;
	}
	
	return pack("H*", $response);
}

sub registers {
	my ($self, $names, $size) = @_;
	
	$names ||= $DEFAULT_REGS;
	$size ||= 4;
	
	my ($ret, $response) = $self->exec('g', []);
	return undef if ($ret != CMD_OK);
	
	my $response_bin = pack("H*", $response);
	
	my $cnt = min(scalar(@$names), length($response_bin) / $size);
	
	if (scalar(@$names) != $cnt) {
		print STDERR "[gdb-client] Invalid registers count (expected ".scalar(@$names).", but received $cnt)\n";
		return undef;
	}
	
	my $regs_info = {};
	for (my $i = 0; $i < $cnt; $i++) {
		my $name = $names->[$i];
		$regs_info->{$name} = unpack("L", substr($response_bin, $i * $size, $size));
	}
	
	return $regs_info;
}

sub continue {
	my ($self, $action, $thread_id) = @_;
	my ($ret, $response) = $self->exec('vCont;'.$action.($thread_id ? ':'.$thread_id : ''), []);
	
	return undef if ($ret != CMD_OK);
	
	my $reason = substr($response, 0, 1);
	
	my $info = {};
	if ($reason eq "S") {
		$info->{type} = "signal";
		$info->{code} = hex substr($response, 1, 2);
	} elsif ($reason eq "T") {
		$info->{type} = "signal";
		$info->{code} = hex substr($response, 1, 2);
		
		if ($response =~ /([ra]?watch):([a-f0-9]+);/) {
			$info->{watchpoint} = {
				type	=> $1,
				addr	=> hex $2
			};
		}
	} elsif ($reason eq "X" || $reason eq "W") {
		$info->{type} = "exit";
		$info->{code} = hex substr($response, 1, 2);
	} else {
		print STDERR "[gdb-client] Unknown response for continue: '$response'\n";
		return undef;
	}
	
	return $info;
}

sub connected {
	my ($self) = @_;
	return $self->{connected};
}

sub exec {
	my ($self, $cmd, $data, $expected) = @_;
	
	$data = join(",", @$data) if (ref($data) eq 'ARRAY');
	
	my $cmd_ret = $self->sendPacket($cmd.$data);
	return ($cmd_ret, "") if $cmd_ret != CMD_OK;
	
	my ($ret, $response) = $self->readPacket();
	return ($ret, "") if $ret != CMD_OK;
	
	if ($expected && $response ne $expected) {
		print STDERR "[gdb-client] Command '$cmd' failed, response: '$response', but expected '$expected'\n";
		return (CMD_INVALID_RESP, "");
	}
	
	return ($ret, $response);
}

sub chk {
	my ($self, $data) = @_;
	
	my $chk = 0;
	for (my $i = 0; $i < length($data); $i++) {
		my $c = ord(substr($data, $i, 1));
		$chk = ($chk + $c) & 0xFF;
	}
	
	return sprintf("%02x", $chk);
}

sub sendPacket {
	my ($self, $packet) = @_;
	
	return CMD_CANCEL if !$self->{connected};
	
	my $data = "+\$$packet#".$self->chk($packet);
	while (length($data) > 0) {
		my @fd = $self->{select}->can_write(1);
		if (@fd) {
			my $written = $self->{sock}->send($data);
			
			if (!$written) {
				print STDERR "[gdb-client] IO closed...\n";
				$self->{connected} = 0;
				return CMD_CANCEL;
			}
			
			$data = substr($data, $written);
		}
	}
	
	return CMD_OK;
}

sub readPacket {
	my ($self) = @_;
	
	return (CMD_CANCEL, "") if !$self->{connected};
	
	my $frame = "";
	
	while (1) {
		my @fd = $self->{select}->can_read(1);
		if (@fd) {
			my $buffer;
			$self->{sock}->recv($buffer, 1024 * 100);
			my $len = length($buffer);
			
			# Connection closed
			if (!$len) {
				print STDERR "[gdb-client] IO closed...\n";
				$self->{connected} = 0;
				return (CMD_CANCEL, "");
			}
			
			$frame .= $buffer;
			
			if ($frame =~ /^([+]*)\$([^#]+)#([\w\d]{2})/i) {
				my ($ack, $packet, $chk) = ($1, $2, $3);
				
				if ($self->chk($packet) ne $chk) {
					print STDERR "[gdb-client] Command not checksum: '$buffer' [$chk != ".$self->chk($packet)."]\n";
					$self->{connected} = 0;
					return (CMD_INVALID_RESP, "");
				}
				
				return (CMD_OK, $packet);
			} elsif ($buffer =~ /^-/) {
				print STDERR "[gdb-client] Command not ACK: '$buffer'\n";
				$self->{connected} = 0;
				return (CMD_NACK, "");
			} elsif ($buffer =~ /^\x03/) {
				print STDERR "[gdb-client] Command not ACK (cancel): '$buffer'\n";
				$self->{connected} = 0;
				return (CMD_CANCEL, "");
			} elsif ($buffer !~ /^\+/) {
				print STDERR "[gdb-client] Invalid ACK: '$buffer'\n";
				$self->{connected} = 0;
				return (CMD_INVALID_RESP, "");
			}
		}
	}
	
	return (CMD_INVALID_RESP, "");
}

1;
