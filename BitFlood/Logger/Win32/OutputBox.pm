package BitFlood::Logger::Win32::OutputBox;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(outputBox curPos));

use BitFlood::Utils;
use BitFlood::Debug;

sub new {
  my $class = shift;
  my $outputBoxRef = shift;
  my $outputBoxPosRef = shift;

  my $self = $class->SUPER::new();
  $self->outputBox($outputBoxRef);
  $self->curPos($outputBoxPosRef);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;


  $string .= "\r\n";

  my $startPos = ${$self->curPos};
  my $endPos = ${$self->curPos} + length($string);

  $self->outputBox->Select($startPos, $startPos);
  $self->outputBox->ReplaceSel($string);


  ######
  ## colorize errors and warnings...
  $self->outputBox->Select($startPos, $endPos);

  my @regexps = $self->config->value('RegularExpressions' => 'RegExp');
  foreach my $regexp (@regexps) {
    $self->outputBox->SetCharFormat(-color => $regexp->{color}) if(eval { $string =~ $regexp->{regexp} });
  }

  $self->outputBox->Select($endPos, $endPos);
  ######
  ## update our idea about where we are...
  ${$self->curPos()} = $endPos;
}

1;
