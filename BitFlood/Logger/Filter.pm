package BitFlood::Logger::Filter;

use strict;

use base qw(Class::Accessor);
__PACKAGE__->mk_accessors(qw());

# dummy subs, meant to be overridden

sub filter {
  my $self = shift;
  die(ref($self) . "::filter not implemented!");
}

1;
