package BitFlood::Client;

use strict;

#use threads;
#use threads::shared;

use RPC::XML;
use RPC::XML::Server;
use RPC::XML::Client;

use base qw(Class::Accessor RPC::XML::Server);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Spec;
use File::Spec::Unix;
use File::Find;
use File::Path;
use Bit::Vector;
use IO::File;
use Socket;
use Sys::Hostname;
use Data::Dumper; # XXX

use BitFlood::Utils;
use BitFlood::Flood;

__PACKAGE__->mk_accessors(qw(floods trackers peers chunksToGet));

$| = 1; # FIXME buh?

sub new {
  my $class = shift;
  my %args = @_;

  my $self = RPC::XML::Server->new(port => $args{port} || 10101);
  bless $self, $class;

  $self->floods({});
  $self->trackers({});
  $self->peers({});

  $self->add_method({
    name => 'RequestChunk', # the name of the method
    version => '0.0.1', # the method version
    hidden => undef,    # is it hidden? undef = no, 1 = yes
    signature => ['base64 string string int'], # return base64 data, take filehash, filename, chunk index
    help => 'A method to request a given chunk.', # help for this method
    code => sub {
      my $self = shift;

      my $fileHash = shift;
      my $filename = shift;
      my $index = shift;

      my $flood = $self->floods->{$fileHash};
      my $file = $flood->Files->{$filename};
      # FIXME: return error here...
      return unless($file->{chunkMap}->bit_test($index));

      my $chunkSourceFilename = $file->{localFilename};
      # FIXME: error
      return unless(defined $chunkSourceFilename);

      my $chunk = $file->{Chunk}[$index];
      # FIXME: return error
      return unless($chunk);

      printf("%-30.30s#%d => %-21.21s\r",
             $file->{name}, $chunk->{index}+1,
	     $self->{__conn}->peerhost . ':' . $self->{__conn}->peerport,
	    );

      my $chunkData;
      my $chunkSourceFile = IO::File->new($chunkSourceFilename, 'r');
      $chunkSourceFile->seek($chunk->{offset}, 0);
      $chunkSourceFile->read($chunkData, $chunk->{size});
      $chunkSourceFile->close();

      return $chunkData;
    },

  });

  $self->started('set'); # FIXME ???

  return $self;
}

sub Start {
  my $self = shift;

  $self->server_loop();
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


sub GetChunk {
  my $self = shift;

  my $floodFileHash = shift;
  my $targetFilename = shift;
  my $index = shift;

  my $flood = $self->floods->{$floodFileHash};
  my $file = $flood->Files->{$targetFilename};
  my $chunk = $file->{Chunk}[$index];

  if (! @{$flood->peers}) {
    print "No peers to talk to!\r";
    return;
  }

#  share($file->{chunkMap});
#  share($file->{totalDownloading});
#  share($chunk->{downloading});
  
  $chunk->{downloading} = 1;
  {
#    lock($file->{totalDownloading});
    $file->{totalDownloading}++;
  }

  $file->{downloadBeginTime} ||= time();
  $file->{downloadBytes} ||= 0;

  my $peerIndex = int(rand(@{$flood->peers}));
  my $peer = $flood->peers->[$peerIndex];
  my ($peerHost) = $peer->{__request}->uri =~ m|http://(.*?)/|;

  printf("%-30.30s#%d <= %-21.21s\r",
	 $file->{name}, $chunk->{index}+1,
	 $peerHost,
	);

#  my $thread = threads->create(sub {
  my $targetFile = IO::File->new($file->{localFilename}, 'r+');
  $targetFile->seek($chunk->{offset}, 0);
  my $chunkData = $peer->simple_request('RequestChunk', $floodFileHash, $targetFilename, $index); # FIXME choose which peer
  if (!$chunkData) {
    if ($RPC::XML::ERROR =~ /connection refused/i) {
      splice(@{$flood->peers}, $peerIndex, 1);
      return;
    }
  }

    if(sha1_base64($chunkData) eq $chunk->{hash}) {
      $targetFile->print($chunkData);
#      lock($file->{chunkMap});
      $file->{chunkMap}->Bit_On($index);
      $file->{downloadBytes} += $chunk->{size};
    } else {
      print "  -> Bad chunk data!\n";
    }

    $targetFile->close();
    delete $chunk->{downloading};
#    lock($file->{totalDownloading});
    $file->{totalDownloading}--;

  if ($file->{chunkMap}->is_full) {
    print ' ' x 70 . "\r";
    printf("Completed: %s [%.2f KB/s]\n",
	   $file->{name},
	   $file->{downloadBytes} / 1024 / (time() - $file->{downloadBeginTime} + 1));
  }
#  });

#  $thread->detach();

}

sub GetChunks {
  my $self = shift;

  my %incompleteFiles;
  foreach my $flood (values %{$self->floods}) {
    my $files = [ grep { ! $_->{chunkMap}->is_full } values %{$flood->Files} ];
    $incompleteFiles{$flood->contentHash} = $files if @$files;
  }

  if (%incompleteFiles) {

    while (my ($floodFileHash, $files) = each %incompleteFiles) {
      foreach my $file (@$files) {
        #next if $self->floods->{$floodFileHash}->{totalDownloading}; # FIXME only getting one chunk at a time
        my $chunkMap = $file->{chunkMap};
        for (my $index = 0; $index < $chunkMap->Size; $index++) {
          next if $chunkMap->bit_test($index);
          $self->GetChunk($floodFileHash, $file->{name}, $index);
          last;
        }
      }
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
	 $flood->contentHash,
	 inet_ntoa(scalar gethostbyname(hostname())), # FIXME good enough?
	 $self->port,
	);
    }
  }
}


sub UpdatePeerList {
  my $self = shift;

  foreach my $flood (values %{$self->floods}) {
    my $tracker = $flood->trackers->[int(rand(@{$flood->trackers}))]; #FIXME random ok?
    $flood->peers([]);
    my $peerListRef = $tracker->simple_request('RequestPeers', $flood->contentHash);
    if($peerListRef) {
      foreach my $peerAddress (@{$peerListRef}) {
        if ($peerAddress ne 'http://'.inet_ntoa(scalar gethostbyname(hostname())).':'.$self->port.'/RPCSERV') { # FIXME skip self (not robusto)
	  push(@{$flood->peers}, RPC::XML::Client->new($peerAddress));
        }
      }
    }
  }
}


sub Disconnect {
  my $self = shift;

  foreach my $flood (values %{$self->floods}) {
    foreach my $tracker (@{$flood->trackers}) {
      $tracker->simple_request
	(
	 'Disconnect',
	 $flood->contentHash,
	 inet_ntoa(scalar gethostbyname(hostname())), # FIXME good enough?
	 $self->port,
	);
    }
  }
}


sub DESTROY {
  my $self = shift;

  $self->Disconnect;
}


1;
