package TBE::Logger::Colored;

use strict;

use base qw(TBE::Logger);
__PACKAGE__->mk_accessors();

use TBE::Utils;
use TBE::Debug;

use TBE::Logger::Filter::ColorErrors;
use TBE::Logger::Filter::ColorWarnings;

sub new {
  my $class = shift;

  my $self = $class->SUPER::new();

  $self->AddFilter(new TBE::Logger::Filter::ColorWarnings());
  $self->AddFilter(new TBE::Logger::Filter::ColorErrors());

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  print "$string\n";
}

1;
