package BitFlood::Debug;

use strict;

$| = 1; # autoflush

use BitFlood::Utils;
use BitFlood::Logger::Multi;
use Sys::Hostname;

use vars qw(@ISA @EXPORT);
require Exporter;

@ISA = qw(Exporter);

# symbols to export
@EXPORT = qw(&Debug);

my %loggers; # hash on channel name

our $debugLevel       = $ENV{BITFLOOD_DEBUG};
our $debugLineNumbers = $ENV{BITFLOOD_DEBUG_LINENUMBERS};
our $debugPid         = $ENV{BITFLOOD_DEBUG_PID};

OpenChannel(''); # default channel
foreach my $channel (split(',', $ENV{BITFLOOD_DEBUG_CHANNELS})) {
  OpenChannel($channel);
}

# FIXME this sucks, move it somewhere else?
AddLogger('', 'Stdout')
  unless $ENV{BITFLOOD_DEBUG_QUIET};
AddLogger('', 'Stderr')
  if $ENV{BITFLOOD_DEBUG_STDERR};
AddLogger('', 'File', $ENV{BITFLOOD_DEBUG_FILENAME} || 'debug.log', append => 0)
  if $ENV{BITFLOOD_DEBUG_FILE};  
AddLogger('', 'Jabber',
          recipient  => 'bftest@jabber.org/listener',
          serverHost => 'jabber.org',
          serverPort => 5222,
          username   => 'bftest',
          password   => 'bftest',
          resource   => hostname() . '_' . inet_ntoa(scalar gethostbyname(hostname())))
  if $ENV{BITFLOOD_DEBUG_JABBER};

AddLogger('trace', 'Stdout');

AddLogger('perf', 'File', 'perf.log');

if ($ENV{BITFLOOD_DEBUG_NET_FILE}) {
  AddLogger('net', 'File', 'net.log', append => 0);
} else {
  AddLogger('net', 'Stdout');
}



sub OpenChannel {
  my $channel = shift;
  my $loggerObject = shift;

  die("channel '$channel' already open") if $loggers{$channel};

  if ($loggerObject) {
    if (!UNIVERSAL::isa($loggerObject, 'BitFlood::Logger')) {
      die("not a Logger object: $loggerObject");
    }
  } else {
    $loggerObject = BitFlood::Logger::Multi->new();
  }    

  $loggers{$channel} = $loggerObject;
}

sub CloseChannel {
  my $channel = shift;

  $loggers{$channel}->close;
  delete $loggers{$channel};
}

sub MuteChannel {
  my $channel = shift;

  $loggers{$channel}->muted(1);
}

sub UnmuteChannel {
  my $channel = shift;

  $loggers{$channel}->muted(0);
}


sub AddLogger {
  my $channel = shift;
  my $logger = shift;
  my @loggerParams = @_;

  $loggers{$channel} and $loggers{$channel}->isa('BitFlood::Logger::Multi')
    or return; # FIXME warn here?

  my $loggerObject;

  if (UNIVERSAL::isa($logger, 'BitFlood::Logger')) {
    $loggerObject = $logger;
  } else {
    my $class = 'BitFlood::Logger::' . $logger;
    eval "require " . $class;
    if ($@) {
      die "Couldn't load Logger class " . $class . " : $@";
    }
    $loggerObject = $class->new(@loggerParams);
  }

  $loggers{$channel}->add($loggerObject);
}


sub Debug {
  my $ref = shift;
  my ($channel, $level);
  if (@_ == 0) {
    $channel = '';
    $level = 1;
  } elsif (@_ == 1) {
    if ($_[0] =~ /^\d+$/) {
      $level = $_[0];
      $channel = '';
    } else {
      $channel = $_[0];
      $level = 1;
    }
  } elsif (@_ == 2) {
    $channel = $_[0];
    $level = $_[1];
  } else {
    warn("too many arguments to Debug");
    return;
  }

  return unless ( defined($debugLevel)
		  and $debugLevel >= $level
		  and $loggers{$channel} );

  # we allow people to execute code at given debug levels...
  # something like:
  #   Debug(sub { $variable = 'DEBUG!' }, 2);
  # will only set $variable to 'DEBUG!' if we're at debug
  # level >=2.
  if(ref($ref) eq 'CODE') {
    &$ref;
    return;
  }

  my ($package, $filename, $line, $subroutine) = caller(1);
  $package    ||= 'unknown';
  $filename   ||= 'unknown';
  $line       ||= 'unknown';
  $subroutine ||= 'unknown';

  my $output;
  $output .= "($$) "                    if $debugPid;
  $output .= "$subroutine";
  $output .= " ($filename, line $line)" if $debugLineNumbers;
  $output .= ": ";
  $output .= BitFlood::Utils::Stringify($ref);

  $loggers{$channel}->log($output);
}

1;
