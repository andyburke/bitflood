package BitFlood::Tracker;

use strict;

use RPC::XML;
use RPC::XML::Server;

use base qw(Class::Accessor);
use base qw(RPC::XML::Server);

use Digest::MD5 qw(md5_base64); # for passwords

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Spec;
use File::Spec::Unix;
use File::Find;

my $TIMEOUT = 300; # 5 minute timeout for clients

sub new {
  my $class = shift;
  my %args = @_;

  my $self = RPC::XML::Server->new(port => $args{port} || 10101);
  bless $self, $class;
  $self->mk_accessors(qw(port filehashClientList));

  $self->port($args{port} || 10101);
  $self->filehashClientList({});

  $self->add_method({
    name => 'Register', # the name of the method
    version => '0.0.1', # the method version
    hidden => undef,    # is it hidden? undef = no, 1 = yes
    signature => ['int string string int'], # return int, take filehash, ip, port
    help => 'A method to register with the tracker.', # help for this method
    code => sub {
      my $self = shift;
      my $filehash = shift;
      my $ip = shift;
      my $port = shift;

      push(@{$self->filehashClientList->{$filehash}}, {ip => $ip, port => $port, timestamp => time()});
      $self->CleanFilehashClientList($filehash);
    },
      
  });

  $self->add_method({
    name => 'Request', # the name of the method
    version => '0.0.1', # the method version
    hidden => undef,    # is it hidden? undef = no, 1 = yes
    signature => ['array string'], # return array, take filehash
    help => 'A method to get other clients serving filehash', # help for this method
    code => sub {
      my $self = shift;
      my $filehash = shift;

      $self->CleanFilehashClientList($filehash);
      return $self->{filehashClientList}->{$filehash};
    },
      
  });

  $self->add_method({
    name => 'Dump', # the name of the method
    version => '0.0.1', # the method version
    hidden => undef,    # is it hidden? undef = no, 1 = yes
    signature => ['int'], # return int
    help => 'A method to dump self.', # help for this method
    code => sub {
      my $self = shift;

      use Data::Dumper;
      print Dumper $self->filehashClientList;
    },
      
  });

  return $self;
}

sub start {
  my $self = shift;
  $self->server_loop();
}

sub CleanFilehashClientList {
  my $self = shift;
  my $filehash = shift;

  my $index = 0;
  foreach my $client (@{$self->{filehashClientList}->{$filehash}}) {
    splice(@{$self->{filehashClientList}->{$filehash}}, $index, 1) if(time() - $client->{timestamp} > $TIMEOUT);
    $index++;
  }
  
}

1;

