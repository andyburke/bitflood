package BitFlood::Net::BufferedReader;

use strict;

use base qw(BitFlood::Accessor);

use Errno qw(:POSIX);
use Time::HiRes qw(time);

use Data::Dumper; # XXX

use BitFlood::Utils;
use BitFlood::Debug;

__PACKAGE__->mk_accessors(qw(buffer socket windowSize));

use constant MAX_SOCKET_WINDOW => 256 * 1024;
use constant MIN_SOCKET_WINDOW => 512;
use constant DEFAULT_SOCKET_WINDOW => 128 * 1024;
#use constant MAX_SOCKET_WINDOW => 4 * 1024;
#use constant MIN_SOCKET_WINDOW => 4 * 1024;
#use constant DEFAULT_SOCKET_WINDOW => 4 * 1024;

sub new {
  my $class = shift;

  my $self = $class->SUPER::new(@_);

  if(!defined($self->buffer)) {
    Debug("No read buffer passed");
    die("No read buffer!");
  }

  Debug("got ref to: " . $self->buffer, 50);

  if(!defined($self->socket)) {
    Debug("No socket passed");
    die("No socket!");
  }

  if(!$self->windowSize) {
    Debug("No windowSize specified, setting to: " . DEFAULT_SOCKET_WINDOW, 'net', 40);
    $self->windowSize(DEFAULT_SOCKET_WINDOW);
  }

  return $self;
}

sub Read {
  my $self = shift;

  Debug(">>>", 10);

  Debug("read window (" . $self->socket->peerhost . ':' . $self->socket->peerport . "): " . $self->windowSize, 'net', 50);

  my $data;
  my $bytesRead = $self->socket->sysread($data, $self->windowSize);
  if (!defined $bytesRead) {
    if ($! == EWOULDBLOCK) {
      Debug("<<<", 10);
      return -1;
    } else {
      Debug("unexpected socket error: $! (" . ($!+0) . ")", 'net');
      Debug("<<<", 10);
      return 0;
    }
  }
  Debug("read $bytesRead bytes", 'net', 40);

  if (!length($data)) { # remote end disconnected
    Debug("<<<", 10);
    return 0;
  }

  ${$self->buffer} .= $data;

  Debug("read data: $data", 'net', 100);
  Debug(length(${$self->buffer}) . " bytes in buffer", 'net', 50);

  return $bytesRead;
  Debug("<<<", 10);
}

1;
