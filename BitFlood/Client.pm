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

$SIG{PIPE} = sub { Debug("SIGPIPE: remote end closed suddenly during a read/write", 'net') };

sub new {
  my $class = shift;
  my %args = @_;
  
  Debug('>>>', 'trace');
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

  Debug("Loading chunk prioritizer...", 20);
  eval "require " . $self->chunkPrioritizerClass;
  if ($@) {
    die "Couldn't load ChunkPrioritizer class " . $self->chunkPrioritizerClass . " : $@";
  }
  $self->chunkPrioritizer($self->chunkPrioritizerClass->new);
  
  
  Debug("Finished creating client object");
  Debug('<<<', 'trace');
  return $self;
}

sub OpenListenSocket {
  my $self = shift;

  Debug('>>>', 'trace');

  my @portList = $self->port ?
      ($self->port) : (DEFAULT_LISTEN_PORT .. DEFAULT_LISTEN_PORT + 10);
  foreach my $port (@portList) {
    Debug("trying port $port", 'net', 5);
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
    Debug("probed port number: " . $self->port, 'net', 5);
  }
  Debug("listening on " . $self->localIp . "(external) :" . $self->port, 'net', 5);
  Debug('<<<', 'trace');
}

# FIXME eventually remove actual RPC calls from here, maybe push
# outbound events on a queue or something?
sub AddFloodFile {
  my $self = shift;
  my $filename = shift;
  my $localPath = shift || '.'; # FIXME mac?

  Debug('>>>', 'trace');

  my $flood = BitFlood::Flood->new({filename  => $filename,
                                    localPath => $localPath});
  $self->floods->{$flood->contentHash} = $flood;

  # add our trackers for this flood file...
  foreach my $trackerURL (@{$flood->TrackerURLs}) {
    my $tracker = RPC::XML::Client->new($trackerURL);
    $tracker->compress_requests(0);
    push(@{$flood->trackers}, $tracker);
  }

  Debug('<<<', 'trace');
  
}

# FIXME bad name, should be like "FigureOutWhatChunkToGetAndDoIt"
sub GetChunks {
  my $self = shift;
  
  Debug('>>>', 'trace');

  if(!@{$self->peers}) { # no peers to get chunks from
    Debug('<<<', 'trace');
    return;
  }

  foreach my $flood (values %{$self->floods}) {
    $flood->startTime(time()) if(!defined($flood->startTime));
    $flood->sessionStartTime(time()) if(!defined($flood->sessionStartTime));
    my ($peer, $file, $chunk) = $self->chunkPrioritizer->FindChunk($self, $flood);
    #Debug([$peer ? $peer->id : "undef", $file ? $file->{name} : "undef" , $chunk ? $chunk->{index} : "undef"]);

    if (defined $peer) {
      $peer->GetChunk($flood, $file, $chunk);
    }
  }

  Debug('<<<', 'trace');
}


sub Register {
  my $self = shift;

  Debug('>>>', 'trace');

  foreach my $flood (values %{$self->floods}) {
    foreach my $tracker (@{$flood->trackers}) {
      Debug("Registering with tracker " . $tracker->request->uri->host . ":" . $tracker->request->uri->port, 'net', 5);
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
  Debug('<<<', 'trace');
}


sub UpdatePeerList {
  my $self = shift;

  Debug('>>>', 'trace');

  foreach my $flood (values %{$self->floods}) {
    my $tracker = $flood->trackers->[int(rand(@{$flood->trackers}))]; #FIXME random ok?
    Debug("Requesting peers from tracker " . $tracker->request->uri->host . ":" . $tracker->request->uri->port, 'net', 5);
    my $peerListRef = $tracker->simple_request('RequestPeers', $flood->contentHash);
    if($peerListRef) {
      Debug("Got back a peerlist", 'net', 5);
      foreach my $peer (@{$peerListRef}) {
	my ($peerId, $peerHost, $peerPort) = split(':', $peer);
        if ($peerId ne $self->id) { # FIXME skip self (not robusto)
	  my ($peer) = grep { $_->id eq $peerId } @{$self->peers};	  
	  if(!$peer) {
	    Debug("New peer: $peerId ($peerHost:$peerPort)", 'net', 10);
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
	  } else {
	    Debug("Already aware of peer: " . $peer->host . ':' . $peer->port, 'net', 10);
	  }
	  #Debug("content hash: ".$flood->contentHash);
	  #Debug($peer->registered);
	  if (!$peer->registered->{$flood->contentHash}) {
	    Debug("Registering peer (" . $peer->host . ':' . $peer->port . ") for flood: " . $flood->contentHash, 'net', 10);
	    # FIXME refactor into a method in Peer?
	    $peer->floods->{$flood->contentHash} = $flood;
	    $peer->SendMessage('Register', $flood, $self->id);
	    $peer->SendMessage('RequestChunkMaps', $flood);
	    $peer->registered->{$flood->contentHash} = 1;
	  }
	}
      }
    } else {
      Debug("Did not receive a peerlist from the tracker", 'net', 10);
    }
  }
  Debug('<<<', 'trace');
}


sub Disconnect {
  my $self = shift;

#  print "Disconnecting from tracker...\n";

  Debug('>>>', 'trace');

  foreach my $flood (values %{$self->floods}) {
    foreach my $tracker (@{$flood->trackers}) {
      Debug("Disconnecting from tracker " . $tracker->request->uri->host . ":" . $tracker->request->uri->port, 'net', 5);
      $tracker->simple_request
	(
	 'Disconnect',
	 $self->id,
	 $flood->contentHash,
	);
    }
  }
  Debug('<<<', 'trace');
}

sub ProcessPeers {
  my $self = shift;

  Debug('>>>', 'trace');

  foreach my $peer (@{$self->peers}) {
    $peer->LoopOnce();
  }
  # reap disconnected peers
  $self->peers([ grep { !$_->disconnected } @{$self->peers} ]);
  Debug('<<<', 'trace');
}

sub AddConnection {
  my $self = shift;
  my $conn = shift;

  Debug('>>>', 'trace');

  my $peer = BitFlood::Peer->new({
				  client => $self,
				  host   => $conn->peerhost,
				  port   => $conn->peerport,
				  socket => $conn,
				 });
  push(@{$self->peers}, $peer);
  Debug('<<<', 'trace');
}

sub LoopOnce {
  my $self = shift;

  Debug('>>>', 'trace');

  $self->ProcessPeers;

  return if !$self->socket;
  return if !IO::Select->new($self->socket)->can_read(0);
  
  my $conn = $self->socket->accept();
  return if(!defined($conn));

  $self->AddConnection($conn);
  Debug('<<<', 'trace');
}


1;
