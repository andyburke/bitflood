package BitFlood::Logger::Multi;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(loggers));

sub new {
  my $class = shift;

  my $self = $class->SUPER::new();

  $self->loggers([]);

  while(my $logger = shift) {
    Debug("Adding logger to " . ref($class) . ": " . ref($logger), 12);
    push(@{$self->loggers}, $logger);
  }

  return $self;
}

sub add {
  my $self = shift;
  my $ref = shift;

  push(@{$self->loggers}, $ref);
}

sub log {
  my $self = shift;
  my $ref = shift;

  foreach my $logger (@{$self->loggers}) {
    $logger->log($ref);
  }
}

sub commit {
  my $self = shift;
  die("Attempt to commit() on a Multi-logger object.");
}

sub close {
  my $self = shift;

  foreach my $logger (@{$self->loggers}) {
    $logger->close();
  }
}

sub DESTROY {
  # dummy
}

1;
