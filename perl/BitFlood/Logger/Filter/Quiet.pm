package BitFlood::Logger::Filter::Quiet;

use strict;

use base qw(BitFlood::Logger::Filter);
__PACKAGE__->mk_accessors();

sub filter {
  my $self = shift;
  my $stringRef = shift;

  return if(!defined($stringRef));
  return if(!length($$stringRef));

  if(
     $$stringRef =~ /error/i ||
     $$stringRef =~ /failed/i ||
     $$stringRef =~ /warn/i 
    ) {

    # add a newline to the string, so it stays on the screen
    $$stringRef .= "\n";

  } else {

    # just get rid of the string if it's not a warning or error
    $$stringRef = '';

  }

  return 1;
}


1;
