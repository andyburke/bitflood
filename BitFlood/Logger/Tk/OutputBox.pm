package BitFlood::Logger::Tk::OutputBox;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(outputBox curPos));

use BitFlood::Utils;
use BitFlood::Debug;

sub new {
  my $class = shift;
  my $outputBoxRef = shift;

  my $self = $class->SUPER::new();
  $self->outputBox($outputBoxRef);
  $self->curPos(0);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  $self->outputBox->index('end');
  $self->outputBox->Insert("$string\n");
}

1;
