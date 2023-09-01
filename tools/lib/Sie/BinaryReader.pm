package Sie::BinaryReader;

use warnings;
use strict;

sub new {
	my ($class, $file) = @_;
	my $self = {};
	return bless $self => $class;
}

sub open {
	my ($self, $file) = @_;
	$self->close() if $self->{handle};
	open(my $fp, "<$file") or die("open(F, <$file): $!");
	$self->{handle} = $fp;
	seek($self->{handle}, 0, 2) or die("seek: $!");
	$self->{size} = tell($self->{handle});
	$self->seek(0);
}

sub size {
	my ($self) = @_;
	return $self->{size};
}

sub close {
	my ($self) = @_;
	close $self->{handle};
	$self->{handle} = undef;
	return $self;
}

# uint8
sub readInt8 {
	my ($self) = @_;
	return unpack("c", $self->readBytes(1));
}

sub readUInt8 {
	my ($self) = @_;
	return unpack("C", $self->readBytes(1));
}

# uint16
sub readUInt16 {
	my ($self) = @_;
	return unpack("v", $self->readBytes(2));
}

sub readUInt16BE {
	my ($self) = @_;
	return unpack("n", $self->readBytes(2));
}

sub readInt16 {
	my ($self) = @_;
	return unpack("!v", $self->readBytes(2));
}

sub readInt16BE {
	my ($self) = @_;
	return unpack("!n", $self->readBytes(2));
}

# uint32
sub readUInt32 {
	my ($self) = @_;
	return unpack("V", $self->readBytes(4));
}

sub readUInt32BE {
	my ($self) = @_;
	return unpack("N", $self->readBytes(4));
}

sub readInt32 {
	my ($self) = @_;
	return unpack("!V", $self->readBytes(4));
}

sub readInt32BE {
	my ($self) = @_;
	return unpack("!N", $self->readBytes(4));
}

sub seek {
	my ($self, $offset) = @_;
	seek($self->{handle}, $offset, 0) or die("seek: $!");
	return $self;
}

sub eof {
	my ($self) = @_;
	return eof($self->{handle});
}

sub readBytes {
	my ($self, $size) = @_;
	my $readed = sysread($self->{handle}, my $data, $size);
	die "Unexpected EOF, size=$size, readed=$readed" if $readed != $size;
	return $data;
}

sub DESTROY {
	my ($self) = @_;
	$self->close() if $self->{handle};
}

1;
