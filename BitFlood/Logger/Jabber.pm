package BitFlood::Logger::Jabber;

use strict;
use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(
  client connected
  recipient serverHost serverPort username password resource
));

use Net::Jabber qw(Client);


# args: recipient, serverHost, serverPort,
#       username, password, resource
sub new {
  my $class = shift;
  my %args = @_;

  my $self = $class->SUPER::new(\%args);

  $self->client(Net::Jabber::Client->new);
  $self->client or die("failed to create Jabber Client: $!");
  if ($self->client->Connect(hostname => $self->serverHost,
                             port     => $self->serverPort)) {
    my @auth = $self->client->AuthSend(username => $self->username,
                                       password => $self->password,
                                       resource => $self->resource);
    if ($auth[0] eq "ok") {
      $self->connected(1);
    }
  }

  return $self;
}


sub commit {
  my $self = shift;
  my $string = shift;

  $self->connected or return;

  my $im = Net::Jabber::Message->new();
  $im->SetMessage(body => $string, type => "chat");
  $im->SetTo($self->recipient);

  $self->client->Send($im);
}


sub close {
  my $self = shift;

  $self->client->Disconnect if $self->connected;
}



1;