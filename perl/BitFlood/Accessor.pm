package BitFlood::Accessor;

# Base class for providing Class::Accessor functionality.
# We pull in Scalar::Util and add the initialize mechanism.
# Also, this makes it easy to use a different C::A subclass (e.g. ::Fast).

use Scalar::Util;

use base qw(Class::Accessor::Fast);


sub new {
  my $class = shift;

  my $self = $class->SUPER::new(@_);
  return $self->initialize(@_);
}


sub initialize {
  my $self = shift;

  if (ref($self) eq __PACKAGE__) {
    die "Refusing to create a " . __PACKAGE__ . " object.  Subclass it instead.";
  }
}


1;
