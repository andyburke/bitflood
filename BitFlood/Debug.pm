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
my $logger = BitFlood::Logger::Multi->new();

SetDebugLevel($ENV{BITFLOOD_DEBUG}) if(defined($ENV{BITFLOOD_DEBUG}));
SetDebugLineNumbers($ENV{BITFLOOD_DEBUG_LINENUMBERS}) if(defined($ENV{BITFLOOD_DEBUG_LINENUMBERS}));

AddStdout() unless defined($ENV{BITFLOOD_DEBUG_QUIET});
AddStderr() if defined($ENV{BITFLOOD_DEBUG_STDERR});
AddFile() if defined($ENV{BITFLOOD_DEBUG_FILE});  

sub SetDebugLevel {
  $debugLevel = shift;
}

sub SetDebugLineNumbers {
  $debugLineNumbers = shift;
}

sub AddStdout {
  $logger->add(BitFlood::Logger::Stdout->new());
}

sub AddStderr {
  $logger->add(BitFlood::Logger::Stderr->new());
}

sub AddFile {
  my $filename = shift || $ENV{BITFLOOD_DEBUG_FILENAME} || 'debug.log.txt';
  $logger->add(BitFlood::Logger::File->new($filename));
}

sub Debug {
  my $ref = shift;
  my $level = shift || 1;

  defined($debugLevel) and ($debugLevel >= $level) or return; # early-out

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

  $logger->log($output);
}

1;
