package BitFlood::Logger::Stderr;

use strict;

use base qw(TBE::Logger);
__PACKAGE__->mk_accessors();

sub new {
  my $class = shift;

  my $self = $class->SUPER::new();

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  print STDERR $string . "\n";
}

1;
