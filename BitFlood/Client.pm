package BitFlood::Client;

use strict;

use threads;
use threads::shared;

use RPC::XML;
use RPC::XML::Server;
use RPC::XML::Client;

use base qw(Class::Accessor);
use base qw(RPC::XML::Server);

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

      printf("RequestChunk: sending: %s %s %d\n", @_);

      my $fileHash = shift;
      my $filename = shift;
      my $index = shift;

      my $flood = $self->floods->{$fileHash};
      my $file = $flood->Files->{$filename};
      # FIXME: return error here...
      return unless($file->{chunkMap}->bit_test($index));
      print " -> have chunk\n";

      my $chunkSourceFilename = $file->{localFilename};
      # FIXME: error
      return unless(defined $chunkSourceFilename);
      print " -> have source filename\n";

      my $chunk = $file->{Chunk}[$index];
      # FIXME: return error
      return unless($chunk);
      print " -> got chunk object\n";

      my $chunkData;
      my $chunkSourceFile = IO::File->new($chunkSourceFilename, 'r');
      $chunkSourceFile->seek($chunk->{offset}, 0);
      $chunkSourceFile->read($chunkData, $chunk->{size});
      $chunkSourceFile->close();

      return $chunkData;
    },

  });

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
    push(@{$flood->trackers}, $tracker);
    $tracker->simple_request(
                             'Register',
                             $flood->contentHash,
                             inet_ntoa(scalar gethostbyname(hostname())), # FIXME good enough?
                             $self->port,
                            );

    foreach my $peerAddress (@{$tracker->simple_request('RequestPeers', $flood->contentHash)}) {
      push(@{$flood->peers}, RPC::XML::Client->new($peerAddress));
    }

  }

}


sub GetChunk {
  my $self = shift;

  printf("GetChunk: getting: %s %s %d\n", @_);

  my $floodFileHash = shift;
  my $targetFilename = shift;
  my $index = shift;

  my $flood = $self->floods->{$floodFileHash};
  my $file = $flood->Files->{$targetFilename};
  my $chunk = $file->{Chunk}[$index];

#  share($file->{chunkMap});
#  share($file->{totalDownloading});
#  share($chunk->{downloading});
  
  $chunk->{downloading} = 1;
  {
#    lock($file->{totalDownloading});
    $file->{totalDownloading}++;
  }

#  my $thread = threads->create(sub {
    my $targetFile = IO::File->new($file->{localFilename}, 'r+');
    $targetFile->seek($chunk->{offset}, 0);
    print "  -> making RPC call\n";
    my $chunkData = $flood->peers->[0]->simple_request('RequestChunk', $floodFileHash, $targetFilename, $index); # FIXME choose which peer
    print "  -> finished RPC call\n";

    if(sha1_base64($chunkData) eq $chunk->{hash}) {
      print "  -> hashes match\n";
      $targetFile->print($chunkData);
      print "  -> updating chunkmap\n";
#      lock($file->{chunkMap});
      $file->{chunkMap}->Bit_On($index);
      print "  -> updated chunkmap OK\n";
    } else {
      print "  -> FAIL!!!!!!!!!!!!!!!!!\n";
    }

    $targetFile->close();
    delete $chunk->{downloading};
#    lock($file->{totalDownloading});
    $file->{totalDownloading}--;
    print "  -> done getting chunk\n";
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
        next if $self->floods->{$floodFileHash}->{totalDownloading}; # FIXME only getting one chunk at a time
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

sub do_one_loop
{
  my $self = shift;
  
  if ($self->{__daemon})
  {
    my ($conn, $req, $resp, $reqxml, $return, $respxml, $timeout);

    my %args = @_;

    $timeout = $self->{__daemon}->timeout(1);

    $conn = $self->{__daemon}->accept;
        
    return unless $conn;
    $conn->timeout($self->timeout);
    $self->process_request($conn);
    $conn->close;
    undef $conn; # Free up any lingering resources

    $self->{__daemon}->timeout($timeout) if defined $timeout;
  }
  else
  {
    die("Do one loop not supported by Net::Server implementation!");
  }
  
  return;
}

1;
