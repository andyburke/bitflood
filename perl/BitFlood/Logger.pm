package BitFlood::Logger;

use strict;

use base qw(BitFlood::Accessor);

__PACKAGE__->mk_accessors(qw(filters muted));


use BitFlood::Utils;


sub initialize {
  my $self = shift;
  my $args = shift;

  $self->SUPER::initialize;
  $self->filters([]) if !$self->filters;
  $self->muted(0);

  return $self;
}

sub AddFilter {
  my $self = shift;
  my $filter = shift;

  push(@{$self->filters}, $filter);
}

sub filter {
  my $self = shift;
  my $stringRef = shift;

  foreach my $filter (@{$self->filters}) {
    $filter->filter($stringRef);
  }
}

sub log {
  my $self = shift;
  my $ref = shift;

  return if $self->muted;

  chomp(my $string = Stringify($ref));
  $self->filter(\$string);
  $self->commit($string) if(defined($string));
}

sub close {
  my $self = shift;
  # dummy
}

sub DESTROY {
  # dummy
}



1;
