package BitFlood::Net::BufferedWriter;

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
use constant DEFAULT_SOCKET_WINDOW => MIN_SOCKET_WINDOW + ((MAX_SOCKET_WINDOW - MIN_SOCKET_WINDOW) / 2);
#use constant MAX_SOCKET_WINDOW => 4 * 1024;
#use constant MIN_SOCKET_WINDOW => 4 * 1024;
#use constant DEFAULT_SOCKET_WINDOW => 4 * 1024;

sub new {
  my $class = shift;

  my $self = $class->SUPER::new(@_);

  if(!defined($self->buffer)) {
    Debug("No buffer passed to " . ref($class));
    die("No buffer!");
  }

  if(!defined($self->socket)) {
    Debug("No socket passed to " . ref($class));
    die("No socket!");
  }

  if(!$self->windowSize) {
    Debug("No windowSize specified, setting to: " . DEFAULT_SOCKET_WINDOW, 'net');
    $self->windowSize(DEFAULT_SOCKET_WINDOW);
  }

  return $self;
}

sub Write {
  my $self = shift;

  Debug(">>>", 10);

  my $transferStartTime;
  my $bytesWritten;
  my $transferTime;

  if(length(${$self->buffer})) {
#    Debug("buffer: " . ${$self->buffer}, 50);
#    Debug("window: " . $self->windowSize, 50);
    # FIXME look into local ( $SIG{PIPE} ) ?
    $transferStartTime = time();
    $bytesWritten = $self->socket->syswrite(${$self->buffer}, $self->windowSize);
    $transferTime = time() - $transferStartTime;
    if (!defined $bytesWritten) {
      if ($! == EWOULDBLOCK) {
	Debug("would block", 50);
        Debug("<<<", 10);
	return -1;
      } else {
	Debug("unexpected socket error: $!");
        Debug("<<<", 10);
	return 0;
      }
    }
    
    my $dataWritten = substr(${$self->buffer}, 0, $bytesWritten, '');
    Debug("wrote data: $dataWritten", 'net', 100);
    Debug(length(${$self->buffer}) . " bytes remain in buffer", 'net', 50);
  }

  Debug("<<<", 10);
  return $bytesWritten;
}

1;
