package BitFlood::Logger::Win32::ErrorBox;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(errorBox curPos));

use BitFlood::Utils;
use BitFlood::Debug;

sub new {
  my $class = shift;
  my $errorBoxRef = shift;
  my $errorBoxPosRef = shift;

  my $self = $class->SUPER::new();
  $self->errorBox($errorBoxRef);
  $self->curPos($errorBoxPosRef);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  # FIXME: use a filter
  if(
     $string =~ /error/i ||
     $string =~ /failed/i
    )
  {

    $string .= "\r\n";

    my $startPos = ${$self->curPos};
    my $endPos = ${$self->curPos} + length($string);

    $self->errorBox->Select($startPos, $startPos);
    $self->errorBox->ReplaceSel($string);


    ######
    ## colorize errors and warnings...
    $self->errorBox->Select($startPos, $endPos);
    $self->errorBox->SetCharFormat(-color => '#FF0000');

    $self->errorBox->Select($endPos, $endPos);
    ######
    ## update our idea about where we are...
    ${$self->curPos()} = $endPos;
  }
}

1;
