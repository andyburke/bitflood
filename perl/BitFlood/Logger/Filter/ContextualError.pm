package BitFlood::Logger::Filter::ContextualError;

use strict;

use base qw(BitFlood::Logger::Filter);
__PACKAGE__->mk_accessors(qw(history haveError));

# $$stringRef is the dereference of the stringRef

sub filter {
  my $self = shift;
  my $stringRef = shift;

  $self->history([]) if(!defined($self->history));
  $self->haveError(0) if (!defined($self->haveError));

  return if(!defined($stringRef));

  if(!length($$stringRef))
  {
    $$stringRef = undef;
    return;
  }


  # here we pull the earliest line in the history off if the array gets too big
  # and we're not currently tracking a set of errors...
  # we then add this line to our history
  shift(@{$self->history}) if($#{$self->history} > 3 && !$self->haveError);
  push(@{$self->history}, $$stringRef);

  if(
     $$stringRef =~ /error/i ||
     $$stringRef =~ /failed/i
    )
  {
    $self->haveError(1);
    $$stringRef = undef;
  }
  else
  {
    if($self->haveError)  # we had an error up until now
    {
      $$stringRef = "\n-----------------------------------------------\n"; # a seperator for this error
      $$stringRef .= join("\n", @{$self->history}); # drop the error and it's context
      $$stringRef .= "\n\n"; # add a couple of returns after this error instance
      $self->history([]); # empty the history
      $self->haveError(0); # reset our error status
    }
    else # we don't have an error right now
    {
      $$stringRef = undef;
    }
  }

  return 1;
}


1;
