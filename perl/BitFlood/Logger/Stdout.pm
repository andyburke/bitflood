package BitFlood::Logger::Stdout;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors();

sub new {
  my $class = shift;

  my $self = $class->SUPER::new();

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  print STDOUT $string . "\n";
}

1;
