package BitFlood::Debug;

use strict;

$| = 1; # autoflush

use BitFlood::Utils;
use BitFlood::Logger::Multi;
use BitFlood::Logger::Stdout;
use BitFlood::Logger::File;

use vars qw(@ISA @EXPORT);
require Exporter;

@ISA = qw(Exporter);

# symbols to export
@EXPORT = qw(&Debug);

my $debugLevel;
my $debugLineNumbers;
my %loggers; # hash on channel name

SetDebugLevel($ENV{BITFLOOD_DEBUG}) if(defined($ENV{BITFLOOD_DEBUG}));
SetDebugLineNumbers($ENV{BITFLOOD_DEBUG_LINENUMBERS}) if(defined($ENV{BITFLOOD_DEBUG_LINENUMBERS}));

OpenChannel(''); # default channel
AddLogger('', 'Stdout')
  unless defined($ENV{BITFLOOD_DEBUG_QUIET});
AddLogger('', 'Stderr')
  if defined($ENV{BITFLOOD_DEBUG_STDERR});
AddLogger('', 'File', $ENV{BITFLOOD_DEBUG_FILENAME} || 'debug.log')
  if defined($ENV{BITFLOOD_DEBUG_FILE});  

foreach my $channel (split(',', $ENV{BITFLOOD_DEBUG_CHANNELS})) {
  OpenChannel($channel);
}

AddLogger('trace', 'Stdout');
AddLogger('net', 'Stdout');
AddLogger('perf', 'File', 'perf.log');


sub SetDebugLevel {
  $debugLevel = shift;
}

sub SetDebugLineNumbers {
  $debugLineNumbers = shift;
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

  my $output = "($$) $subroutine";
  $output .= " ($filename, line $line)" if(defined($debugLineNumbers));
  $output .= ": ";
  $output .= BitFlood::Utils::Stringify($ref);

  $loggers{$channel}->log($output);
}

1;
