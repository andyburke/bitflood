package BitFlood::Logger;

use strict;

use base qw(BitFlood::Accessor);
__PACKAGE__->mk_accessors(qw(filters muted));


use BitFlood::Utils;
use BitFlood::Debug;

sub new {
  my $class = shift;
  my $args = shift;

  my $self = $class->SUPER::new($args);

  $self->filters([]);
  $self->muted(0);

  my %loadedFilterModuleNames;
  if(exists($args->{filters}))
  {
    foreach my $filterName (@{$args->{filters}}) {
      if(!exists($loadedFilterModuleNames{$filterName}))
      {
	eval "local \$SIG{__DIE__}='DEFAULT'; require ${filterName}";
	die("Failed to load filter: $filterName: $@") if ($@);
	$loadedFilterModuleNames{$filterName}++;
      }

      $self->AddFilter($filterName->new());
    }
  }

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
