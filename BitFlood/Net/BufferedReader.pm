package BitFlood::Net::BufferedReader;

use strict;

use base qw(Class::Accessor);

use Errno qw(:POSIX);
use Time::HiRes qw(time);

use Data::Dumper; # XXX

use BitFlood::Utils;
use BitFlood::Debug;

__PACKAGE__->mk_accessors(qw(buffer socket windowSize));

use constant MAX_SOCKET_WINDOW => 256 * 1024;
use constant MIN_SOCKET_WINDOW => 512;


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
    my $defaultWindowSize = MIN_SOCKET_WINDOW + ((MAX_SOCKET_WINDOW - MIN_SOCKET_WINDOW) / 2);
    Debug("No windowSize specified, setting to: $defaultWindowSize");
    $self->windowSize($defaultWindowSize);
  }

  return $self;
}

sub Read {
  my $self = shift;

  Debug(">>>", 10);

  Debug("read window: " . $self->windowSize, 50);

  my $data;
  my $readStartTime = time();
  my $bytesRead = $self->socket->sysread($data, $self->windowSize);
  my $transferTime = time() - $readStartTime;
  if (!defined $bytesRead) {
    if ($! == EWOULDBLOCK) {
      Debug("would block", 50);
      Debug("<<<", 10);
      return -1;
    } else {
      Debug("unexpected socket error: $! (" . ($!+0) . ")");
      Debug("<<<", 10);
      return 0;
    }
  }

  Debug("read data: $data", 50);

  if ($data eq '') { # remote end disconnected
    Debug("<<<", 10);
    return 0;
  }

  Debug("before: " . ${$self->buffer} . "/" . $self->buffer, 50);
  ${$self->buffer} = ${$self->buffer} . $data;
  Debug("after: " . ${$self->buffer} . "/" . $self->buffer, 50);

  return $bytesRead;
  Debug("<<<", 10);
}

1;
