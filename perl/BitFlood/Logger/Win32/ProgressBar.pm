package BitFlood::Logger::Win32::ProgressBar;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(progressBar numActions overallProgressPercentageRef basePercentage));

use BitFlood::Utils;
use BitFlood::Debug;

#######################################################
## module assumes a progressbar is has a range of 0-100

sub new {
  my $class = shift;
  my $progressBarRef = shift;
  my $numActions = shift;
  my $overallProgressPercentageRef = shift;

  my $self = $class->SUPER::new();
  $self->progressBar($progressBarRef);
  $self->numActions($numActions);
  $self->overallProgressPercentageRef($overallProgressPercentageRef);
  $self->basePercentage(undef);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  $self->basePercentage(${$self->overallProgressPercentageRef}) if(defined($self->overallProgressPercentageRef) and !defined($self->basePercentage));

  $string =~ /(\d+)\/(\d+)/;
  my ($completedTargets, $totalTargets) = ($1, $2);
  return if(!$totalTargets);

  my $percentageComplete = int(100 * ($completedTargets / $totalTargets));

  if(defined($self->numActions) && defined($self->overallProgressPercentageRef))
  {
    ${$self->overallProgressPercentageRef} = int($self->basePercentage + ($percentageComplete / $self->numActions));
    $self->progressBar->SetPos(${$self->overallProgressPercentageRef});
  }
  else
  {
    $self->progressBar->SetPos($percentageComplete);
  }
}

sub close {
  my $self = shift;

  $self->basePercentage(${$self->overallProgressPercentageRef}) if(defined($self->overallProgressPercentageRef) and !defined($self->basePercentage));

  if(defined($self->numActions) && defined($self->overallProgressPercentageRef))
  {
    ${$self->overallProgressPercentageRef} = $self->basePercentage + int(100 / $self->numActions) + 1;
    $self->progressBar->SetPos(${$self->overallProgressPercentageRef});
  }
  else
  {
    $self->progressBar->SetPos(100);
  }
}

1;
