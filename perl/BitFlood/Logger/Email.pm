package BitFlood::Logger::Email;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(subject addresses body sent));

use BitFlood::Utils;
use BitFlood::Debug;

use Mail::SendEasy;

sub new {
  my $class = shift;
  my %args = @_;

  my $self = $class->SUPER::new(\%args);

  $self->subject($args{subject});

  $self->addresses([]);
  my @addresses = ref($args{addresses}) eq 'ARRAY' ? @{$args{addresses}} : ($args{addresses});
  push(@{$self->addresses()}, @addresses);

  $self->sent(0);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  $self->body($self->body . "$string\n");
}

sub close {
  my $self = shift;

  return if($self->sent());

  my $mail = new Mail::SendEasy(smtp => 'mx.turbinegames.com');

  my $status = $mail->send(
			   from       => 'build@turbinegames.com',
			   from_title => 'Build System',
			   to         => join(',', @{$self->addresses()}),
			   subject    => $self->subject ,
			   msg        => $self->body ,
			  ) ;

  if(!$status)
  {
    print $mail->error;
  }
  else
  {
    $self->sent(1);
  }
}

sub DESTROY {
  my $self = shift;
  $self->close();
}

1;
