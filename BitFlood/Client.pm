package BitFlood::Client;

use strict;

#use threads;
#use threads::shared;

use RPC::XML;
use RPC::XML::Server;
use RPC::XML::Client;

use base qw(BitFlood::Accessor);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Spec;
use File::Spec::Unix;
use File::Find;
use File::Path;
use Bit::Vector;
use IO::File;
use IO::Select;
use Socket;
use Sys::Hostname;
use Data::Dumper; # XXX

use BitFlood::Utils;
use BitFlood::Debug;
use BitFlood::Flood;
use BitFlood::Peer;

use constant DEFAULT_LISTEN_PORT => 10101;

__PACKAGE__->mk_accessors(qw(id socket port
                             floods peers trackers chunksToGet localIp
                             chunkPrioritizerClass chunkPrioritizer
			     desiredPeerLoopDuration));

$| = 1; # FIXME buh?

$SIG{PIPE} = sub { Debug("SIGPIPE: remote end closed suddenly during a read/write") };

sub new {
  my $class = shift;
  my %args = @_;
  
  Debug("Creating Client object", 30);

  my $self = $class->SUPER::new(\%args);

  $self->floods({});
  $self->peers([]);
  $self->trackers({});
  $self->localIp($ENV{BITFLOOD_LOCAL_IP} || inet_ntoa(scalar gethostbyname(hostname()))) if(!$self->localIp);
  $self->desiredPeerLoopDuration(.1);

  $self->OpenListenSocket();

  Debug("Generating id using: " . $self->localIp . ", " . $self->port);
  $self->id(sha1_base64($self->localIp . $self->port));

  $self->chunkPrioritizerClass('BitFlood::ChunkPrioritizer::Weighted');

  Debug("Laoding chunk prioritizer...", 20);
  eval "require " . $self->chunkPrioritizerClass;
  if ($@) {
    die "Couldn't load ChunkPrioritizer class " . $self->chunkPrioritizerClass . " : $@";
  }
  $self->chunkPrioritizer($self->chunkPrioritizerClass->new);
  
  
  Debug("Finished creating client object...");
  return $self;
}

sub OpenListenSocket {
  my $self = shift;

  Debug("Creating listen socket for client object", 'net', 50);
  my @portList = $self->port ?
      ($self->port) : (DEFAULT_LISTEN_PORT .. DEFAULT_LISTEN_PORT + 10);
  foreach my $port (@portList) {
    $self->port($port);
    $self->socket(IO::Socket::INET->new(
					Listen    => 5,
					LocalPort => $self->port,
					Proto     => 'tcp',
					Reuse     => 1,
					));
    last if $self->socket;
  }
  die("Could not start listening!") if(!$self->socket);

  if ($self->port != $portList[0]) {
    Debug("probed port number: " . $self->port);
  }
  print "Started listening on " . $self->localIp . "(external) :" . $self->port . "\n";

}

# FIXME eventually remove actual RPC calls from here, maybe push
# outbound events on a queue or something?
sub AddFloodFile {
  my $self = shift;
  my $filename = shift;
  my $localPath = shift || '.'; # FIXME mac?

  my $flood = BitFlood::Flood->new({filename  => $filename,
                                    localPath => $localPath});
  $self->floods->{$flood->contentHash} = $flood;

  # add our trackers for this flood file...
  foreach my $trackerURL (@{$flood->TrackerURLs}) {
    my $tracker = RPC::XML::Client->new($trackerURL);
    $tracker->compress_requests(0);
    push(@{$flood->trackers}, $tracker);
  }

}

# FIXME bad name, should be like "FigureOutWhatChunkToGetAndDoIt"
sub GetChunks {
  my $self = shift;
  
  return if(!@{$self->peers}); # no peers to get chunks from
  
  foreach my $flood (values %{$self->floods}) {
    $flood->startTime(time()) if(!defined($flood->startTime));
    $flood->sessionStartTime(time()) if(!defined($flood->sessionStartTime));
    my ($peer, $file, $chunk) = $self->chunkPrioritizer->FindChunk($self, $flood);
    #Debug([$peer ? $peer->id : "undef", $file ? $file->{name} : "undef" , $chunk ? $chunk->{index} : "undef"]);

    if (defined $peer) {
      $peer->GetChunk($flood, $file, $chunk);
    }
  }

}


sub Register {
  my $self = shift;

  foreach my $flood (values %{$self->floods}) {
    foreach my $tracker (@{$flood->trackers}) {
      $tracker->simple_request
	(
	 'Register',
	 $self->id,
	 $flood->contentHash,
	 $self->localIp,
	 $self->port,
	);
    }
  }
}


sub UpdatePeerList {
  my $self = shift;

  foreach my $flood (values %{$self->floods}) {
    my $tracker = $flood->trackers->[int(rand(@{$flood->trackers}))]; #FIXME random ok?
    my $peerListRef = $tracker->simple_request('RequestPeers', $flood->contentHash);
    if($peerListRef) {
      foreach my $peer (@{$peerListRef}) {
	my ($peerId, $peerHost, $peerPort) = split(':', $peer);
        if ($peerId ne $self->id) { # FIXME skip self (not robusto)
	  my ($peer) = grep { $_->id eq $peerId } @{$self->peers};	  
	  if(!$peer) {
	    Debug("new peer: $peerId ($peerHost:$peerPort)", 10);
	    $peer = BitFlood::Peer->new({
					 client => $self,
					 host   => $peerHost,
					 port   => $peerPort,
					 id     => $peerId,
					});
	    if (!$peer) {
	      Debug("Somehow Peer->new() failed?");
	      Debug($peer);
	      die("unexpected failure in Peer creation");
	    }
	    push(@{$self->peers}, $peer);
	  }
	  #Debug("content hash: ".$flood->contentHash);
	  #Debug($peer->registered);
	  if (!$peer->registered->{$flood->contentHash}) {
	    # FIXME refactor into a method in Peer?
	    $peer->floods->{$flood->contentHash} = $flood;
	    $peer->SendMessage('Register', $flood, $self->id);
	    $peer->SendMessage('RequestChunkMaps', $flood);
	    $peer->registered->{$flood->contentHash} = 1;
	  }
	}
      }
    }
  }
}


sub Disconnect {
  my $self = shift;

#  print "Disconnecting from tracker...\n";

  foreach my $flood (values %{$self->floods}) {
    foreach my $tracker (@{$flood->trackers}) {
      Debug("Sending disconnect request to tracker", 'net');
      $tracker->simple_request
	(
	 'Disconnect',
	 $self->id,
	 $flood->contentHash,
	);
    }
  }
}

sub ProcessPeers {
  my $self = shift;

  foreach my $peer (@{$self->peers}) {
    $peer->LoopOnce();
  }
  # reap disconnected peers
  $self->peers([ grep { !$_->disconnected } @{$self->peers} ]);
}

sub AddConnection {
  my $self = shift;
  my $conn = shift;

  my $peer = BitFlood::Peer->new({
				  client => $self,
				  host   => $conn->peerhost,
				  port   => $conn->peerport,
				  socket => $conn,
				 });
  push(@{$self->peers}, $peer);
}

sub LoopOnce {
  my $self = shift;

  $self->ProcessPeers;

  return if !$self->socket;
  return if !IO::Select->new($self->socket)->can_read(0);
  
  my $conn = $self->socket->accept();
  return if(!defined($conn));

  $self->AddConnection($conn);
}


1;
