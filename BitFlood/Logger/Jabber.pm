package BitFlood::Logger::Jabber;

use strict;
use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(client));

use Net::Jabber qw(Client);


# args: recipient, serverHost, serverPort, user, password, resource
sub new {
  my $class = shift;
  my %args = @_;

  my $self = $class->SUPER::new(\%args);

  $self->client(Net::Jabber::Client->new);
  $self->client or die("failed to create Jabber Client: $!");
}


sub commit {
  my $self = shift;
  my $string = shift;

  my $im = Net::Jabber::Message->new();
  $im->SetMessage(body => $string,
		  type => "chat");
  $im->SetTo($self->recipient);

  $self->client->Connect(hostname => $self->serverHost,
			 port     => $self->serverPort)
      or return;

  my @auth = $self->client->AuthSend(username => $self->user,
				     password => $self->password,
				     resource => $self->resource);
  $auth[0] eq "ok" or return;

  $self->client->Send($im);
  $self->client->Disconnect;
}



1;
