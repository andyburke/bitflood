package BitFlood::Peer;

use strict;

use base qw(Class::Accessor);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use Bit::Vector;

use Data::Dumper; # XXX

use BitFlood::Utils;

__PACKAGE__->mk_accessors(qw(rpcUrl chunkMaps rpcClient));


sub new {
  my $class = shift;

  my $self = $class->SUPER::new(@_);

  $self->chunkMaps({});
  $self->RpcConnect if(defined($self->rpcUrl));

  return $self;
}

sub Host {
  my $self = shift;
  
  my ($host) = $self->rpcClient->{__request}->uri =~ m|http://(.*?)/|;
  return $host;
}

sub RpcConnect {
  my $self = shift;

  $self->rpcClient(RPC::XML::Client->new($self->rpcUrl));
}

sub UpdateChunkMaps {
  my $self = shift;
  my $flood = shift;

  return if(!defined($self->rpcClient));

  my $chunkMaps = $self->rpcClient->simple_request('GetChunkMaps', $flood->contentHash);
  # FIXME dirty way of converting array->hash and making failed RPC call work out right
  $chunkMaps = $chunkMaps ? {@{$chunkMaps}} : {}; 

  foreach my $chunkMap (values %$chunkMaps) {
    $chunkMap = Bit::Vector->new_Hex(length($chunkMap) * 4, $chunkMap);
  }
  $self->chunkMaps($chunkMaps);
}



1;
