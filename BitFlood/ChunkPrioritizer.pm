package BitFlood::ChunkPrioritizer;

use strict;
use base qw(Class::Accessor);


sub new {
  my $class = shift;

  if ($class eq __PACKAGE__) {
    die("__PACKAGE__ is virtual; please instantiate a subclass instead");
  }

  my $self = $class->SUPER::new(@_);

  return $self;
}


sub FindChunk {
  my $self = shift;

  die("FindChunk not implemented");
}



1;
