package BitFlood::VirtualChunk;

use strict;

use base qw(BitFlood::Accessor);


__PACKAGE__->mk_accessors(qw(file index size weight hash));


sub initialize {
  my $self = shift;

  $self->SUPER::initialize;
  Scalar::Util::weaken($self->file);

  return $self;
}



1;
