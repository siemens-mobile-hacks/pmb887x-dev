package Sie::SerialPort;

use warnings;
use strict;
use IO::Handle;
use IO::Select;
use if $^O ne "MSWin32", "Device::SerialPort";
use if $^O eq "MSWin32", "Win::SerialPort";
use POSIX;
use Data::Dumper;
use Time::HiRes;
use IO::Socket;
use IO::Socket::UNIX;

sub new {
	my ($class, $device) = @_;
	
	my $self = {
		device 	=> $device,
	};
	bless $self, $class;
	
	if ($device =~ /^unix:(.*?)$/i) {
		$self->_initUnixServer($1);
	} elsif ($device =~ /^tcp:(.*?):(\d+)$/i) {
		$self->_iniTcpServer($1, $2);
	} elsif ($^O eq "MSWin32") {
		$self->_initSerialWindows($device);
	} else {
		$self->_initSerialUnix($device);
	}
	
	return $self;
}

sub _iniTcpServer {
	my ($self, $host, $port) = @_;
	
	$self->{is_serial} = 0;
	
	my $server = IO::Socket::INET->new(
		LocalHost	=> $host,
		LocalPort	=> $port,
		Proto		=> 'tcp',
		Listen		=> 5,
		Reuse		=> 1
	) or die("socket: $!");
	
	$server->blocking(1);
	
	print "Waiting client to tcp socket ($host:$port)...\n";
	$self->{handle} = $server->accept();
	$self->{handle}->blocking(0);
	$self->{select} = IO::Select->new($self->{handle});
}

sub _initUnixServer {
	my ($self, $device) = @_;
	
	unlink $device if -e $device;
	
	$self->{is_serial} = 0;
	
	my $server = IO::Socket::UNIX->new(
		Type		=> SOCK_STREAM(),
		Local		=> $device,
		Listen		=> 1,
		Blocking	=> 1
	) or die("socket: $!");
	
	$server->blocking(1);
	
	print "Waiting client to unix socket ($device)...\n";
	$self->{handle} = $server->accept();
	$self->{handle}->blocking(0);
	$self->{select} = IO::Select->new($self->{handle});
}

sub _initSerialWindows {
	my ($self, $device) = @_;
	
	$self->{is_serial} = 1;
	$self->{port} = Win::SerialPort->new($device, 1);
	die "open($device): $!" if !$self->{port};
	
	# Initial port setup
	$self->{port}->baudrate(115200);
	$self->{port}->databits(8);
	$self->{port}->parity("none");
	$self->{port}->stopbits(1);
	$self->{port}->datatype('raw');
	$self->{port}->binary('Y');
	$self->{port}->write_settings or die("write_settings: $!");
}

sub _initSerialUnix {
	my ($self, $device) = @_;
	
	$self->{is_serial} = 1;
	$self->{port} = Device::SerialPort->new($device);
	die "open($device): $!" if !$self->{port};
	
	# Initial port setup
	$self->{port}->baudrate(115200);
	$self->{port}->databits(8);
	$self->{port}->parity("none");
	$self->{port}->stopbits(1);
	$self->{port}->write_settings or die("write_settings: $!");
	
	$self->{handle} = new IO::Handle->new_from_fd($self->{port}->FILENO, "a");
	die "IO::Handle error: $!" if !$self->{handle};
	$self->{handle}->blocking(0);
	$self->{select} = IO::Select->new($self->{handle});
}

sub setSpeed {
	my ($self, $speed) = @_;
	if ($self->{is_serial}) {
		if (ref($self->{port}) eq "Win::SerialPort") {
			$self->{port}->baudrate($speed);
			$self->{port}->write_settings or die("write_settings: $!");
		} elsif ("$^O" =~ /linux/i) {
			require Linux::Termios2;
			my $termios = Linux::Termios2->new;
			$termios->getattr($self->{port}->FILENO);
			$termios->setospeed($speed);
			$termios->setispeed($speed);
			$termios->setattr($self->{port}->FILENO, TCSANOW);
		} else {
			$self->{port}->baudrate($speed);
			$self->{port}->write_settings or die("write_settings: $!");
		}
	}
}

sub getChar {
	my ($self, $timeout) = @_;
	my $data = $self->readChunk(1, $timeout);
	return length($data) ? ord($data) : -1;
}

sub readByte {
	my ($self, $timeout) = @_;
	return $self->readChunk(1, $timeout);
}

sub readChunk {
	my ($self, $size, $timeout) = @_;
	
	$timeout ||= 10000;
	
	if (ref($self->{port}) eq "Win::SerialPort") {
		my $start = Time::HiRes::time * 1000;
		
		$self->{port}->read_interval($timeout);
		$self->{port}->read_char_time($timeout);
		$self->{port}->read_const_time($timeout);
		
		my ($count, $data) = $self->{port}->read(1);
		return $count ? $data : "";
	} else {
		$timeout = ($timeout || 0) / 1000;
		
		$! = undef;
		my ($fd) = $self->{select}->can_read($timeout);
		die("select: $!") if $!;
		
		if ($fd) {
			my $ret = sysread($self->{handle}, my $buff, $size);
			die("syswrite: $!") if !defined $ret;
			die("Unexpected EOF!") if $ret == 0;
			return $buff;
		}
		
		return "";
	}
}

sub read {
	my ($self, $size, $timeout) = @_;
	
	$timeout ||= 10000;
	
	if (ref($self->{port}) eq "Win::SerialPort") {
		$self->{port}->read_interval($timeout);
		$self->{port}->read_char_time($timeout);
		$self->{port}->read_const_time($timeout);
		
		my ($count, $data) = $self->{port}->read($size);
		return length($data) == $count ? $data : "";
	} else {
		$timeout = ($timeout || 0) / 1000;
		
		my $start = Time::HiRes::time;
		my $next_timeout = $timeout;
		my $data = "";
		
		while (1) {
			$data .= $self->readChunk($size - length($data), $next_timeout * 1000);
			return $data if length($data) >= $size;
			
			my $elapsed = Time::HiRes::time - $start;
			return $data if $elapsed >= $timeout;
			$next_timeout = $timeout - $elapsed;
		}
		
		return undef;
	}
}

sub write {
	my ($self, $data, $timeout) = @_;
	
	$timeout ||= 10000;
	
	if (ref($self->{port}) eq "Win::SerialPort") {
		$self->{port}->write_char_time($timeout);
		$self->{port}->write_const_time($timeout);
		return $self->{port}->write($data);
	} else {
		$timeout = ($timeout || 0) / 1000;
		
		my $written = 0;
		my $start = Time::HiRes::time;
		
		my $next_timeout = $timeout;
		
		while (1) {
			$! = undef;
			my ($fd) = $self->{select}->can_write($next_timeout);
			die("select: $!") if $!;
			
			if ($fd) {
				my $ret = syswrite($self->{handle}, $data, length($data) - $written, $written);
				die("syswrite: $!") if !defined $ret;
				die("Unexpected EOF!") if $ret == 0;
				
				$written += $ret;
			}
			
			return $written if $written >= length($data);
			
			my $elapsed = Time::HiRes::time - $start;
			return $written if $elapsed >= $timeout;
			$next_timeout = $timeout - $elapsed;
		}
		
		return $written;
	}
}

sub dtr {
	my ($self, $flag) = @_;
	$self->{port}->dtr_active($flag) if $self->{is_serial};
	$self->{port}->rts_active(0) if $self->{is_serial};
}

sub rts {
	my ($self, $flag) = @_;
	$self->{port}->rts_active($flag) if $self->{is_serial};
}

sub sendAt {
	my ($self, $cmd, $response_type, $timeout) = @_;
	
	$timeout //= 10000;
	$timeout = ($timeout || 0) / 1000;
	
	my $start = Time::HiRes::time;
	my $next_timeout = $timeout;
	my $buffer = "";
	
	$self->write($cmd, $next_timeout * 1000);
	my $elapsed = Time::HiRes::time - $start;
	return 0 if $elapsed >= $timeout;
	$next_timeout = $timeout - $elapsed;
	
	my $status = "TIMEOUT";
	my $response = [];
	
	if ($response_type && $response_type =~ /^binary:(\d+)/) {
		my $sz = $1;
		my $data = $self->read(2 + $sz, $next_timeout * 1000);
		
		return {status => "TIMEOUT", response => undef, response => []} if $data !~ /^\r\n/ || length($data) != (2 + $sz);
		$response = [substr($data, 2)];
		$response_type = undef;
		
		my $elapsed = Time::HiRes::time - $start;
		return 0 if $elapsed >= $timeout;
		$next_timeout = $timeout - $elapsed;
	}
	
	my $stop = 0;
	while (!$stop) {
		$buffer .= $self->readChunk(4096, $next_timeout * 1000);
		
		while ($buffer =~ /^([^\r\n]*)\r\n/gis) {
			my $line = $1;
			$buffer = substr($buffer, length($line) + 2);
			
			if ($line eq "OK" || $line eq "ERROR") {
				$status = $line;
				$stop = 1;
				last;
			} elsif ($response_type) {
				if ($response_type eq "numeric") {
					push @$response, $line if $line =~ /^\d+/;
				} elsif ($response_type eq "no_prefix") {
					push @$response, $line if $line =~ /^[^+^%]/;
				}
			}
		}
			
		my $elapsed = Time::HiRes::time - $start;
		last if $elapsed >= $timeout;
		$next_timeout = $timeout - $elapsed;
	}
	
	return {
		status		=> $status,
		response	=> @$response ? $response->[0] : undef,
		responses	=> $response
	};
}

sub close {
	my ($self) = @_;
	$self->{port}->close if $self->{port};
	$self->{handle}->close if $self->{handle};
	undef $self->{port};
	undef $self->{handle};
}

sub DESTROY {
	my ($self) = @_;
	$self->close;
}

1;
