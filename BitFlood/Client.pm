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
use Data::Dumper; # XXX

use BitFlood::Utils;


__PACKAGE__->mk_accessors(qw(floods trackers peers gettingChunks));


sub new {
  my $class = shift;
  my %args = @_;

  my $self = RPC::XML::Server->new(port => $args{port} || 10101);
  bless $self, $class;

  $self->floods({});
  $self->trackers({});
  $self->peers({});
  $self->gettingChunks(0);

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
      # FIXME: return error here...
      return unless($flood->{chunkMaps}{$filename}->bit_test($index));

      my $chunkSourceFilename = $flood->{localTargetFilenames}{$filename};
      # FIXME: error
      return unless($chunkSourceFilename);

      my $chunk = $flood->{floodData}{FileInfo}{File}{$filename}{Chunk}[$index];
      # FIXME: return error
      return unless($chunk);

      my $chunkOffset = $flood->{chunkOffsets}{$filename}[$index];
      my $chunkData;
      my $chunkSourceFile = IO::File->new($chunkSourceFilename, 'r');
      $chunkSourceFile->seek($chunkOffset, 0);
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

sub AddFloodFile {
  my $self = shift;
  my $filename = shift;
  my $localPath = shift || '.';

  return if(!-f $filename);

  open(FLOODFILE, $filename);
  my $floodData = XMLin(\*FLOODFILE, ForceArray => [qw(File Tracker)]);
  seek(FLOODFILE, 0, 0);
  my $contents = join('', <FLOODFILE>);
  my $floodFileHash = sha1_base64($contents);
  close(FLOODFILE);

  # NOTE: this guarantees that our chunks are in order, per target file,
  #       but means that we can NEVER write the floodfile back out, so
  #       screw you
  $self->SortChunksInPlace($floodData);

  my $chunkMaps = $self->BuildChunkMaps($floodData);
  my $localTargetFilenames = $self->BuildLocalTargetFilenames($floodData, $localPath);
  my $chunkOffsets = $self->BuildChunkOffsets($floodData);
  $self->floods->{$floodFileHash} = {
                                     localTargetFilenames => $localTargetFilenames,
                                     floodData            => $floodData,
                                     chunkMaps            => $chunkMaps,
                                     chunkOffsets         => $chunkOffsets,
                                    };

  # add our trackers for this flood file...
  $self->trackers->{$floodFileHash} ||= [];
  foreach my $tracker (@{$floodData->{Tracker}}) {
    my $tracker = RPC::XML::Client->new($tracker);
    push(@{$self->trackers->{$floodFileHash}}, $tracker);
    $tracker->simple_request(
                             'Register',
                             $floodFileHash,
                             '192.168.2.128',
                             $self->port,
                            );

    $self->peers->{$floodFileHash} ||= [];
    foreach my $peerAddress (@{$tracker->simple_request('RequestPeers', $floodFileHash)}) {
      push(@{$self->peers->{$floodFileHash}}, RPC::XML::Client->new($peerAddress));
    }

  }

}



sub BuildChunkMaps {
  my $self = shift;
  my $floodData = shift;

  my %chunkMaps;
  while (my ($fileName, $file) = each  %{$floodData->{FileInfo}{File}}) {
    $chunkMaps{$fileName} = Bit::Vector->new(scalar @{$file->{Chunk}});
  }

  return \%chunkMaps;
}

sub BuildLocalTargetFilenames {
  my $self = shift;
  my $floodData = shift;
  my $localPath = shift;

  my %localTargetFilenames;
  while (my ($fileName, $file) = each  %{$floodData->{FileInfo}{File}}) {
    $localTargetFilenames{$fileName} = LocalFilename(File::Spec->catfile($localPath, $fileName));
  }

  return \%localTargetFilenames;
}

sub BuildChunkOffsets {
  my $self = shift;
  my $floodData = shift;

  my %chunkOffsets;
  while (my ($fileName, $file) = each  %{$floodData->{FileInfo}{File}}) {
    $chunkOffsets{$fileName} = [];
    my $offset = 0;
    foreach my $chunk (@{$file->{Chunk}}) {
      push(@{$chunkOffsets{$fileName}}, $offset);
      $offset += $chunk->{size};
    }
  }

  return \%chunkOffsets;
}

sub SortChunksInPlace {
  my $self = shift;
  my $floodData = shift;

  foreach my $targetFile (values(%{$floodData->{FileInfo}{File}})) {
    $targetFile->{Chunk} = [ sort { $a->{index} <=> $b->{index} } @{$targetFile->{Chunk}} ];
  }
}

sub InitializeTargetFiles {
  my $self = shift;

  foreach my $flood (values(%{$self->floods})) {
    my $floodData = $flood->{floodData};
    while (my ($targetFileName, $targetFile) = each %{$floodData->{FileInfo}->{File}}) {
      my $localTargetFilename = $flood->{localTargetFilenames}{$targetFileName};

      if(!-f $localTargetFilename)   # file doesn't exist, initialize it to 0
      {
        mkpath(GetLocalPathFromFilename($localTargetFilename));
        open(OUTFILE, ">$localTargetFilename");
        my $fileSize = $targetFile->{Size};
        if($fileSize > 0) {
          seek(OUTFILE, $fileSize-1, 0);
          syswrite(OUTFILE, 0, 1);
        }
        close(OUTFILE);
      }
      else # file DOES exist, we need to decide what we need to get...
      {
        open(OUTFILE, "<$localTargetFilename");
        my $buffer;
        foreach my $chunk (@{$targetFile->{Chunk}}) {
          read(OUTFILE, $buffer, $chunk->{size});
          my $hash = sha1_base64($buffer);
          if($hash eq $chunk->{hash}) {
            $flood->{chunkMaps}{$targetFileName}->Bit_On($chunk->{index});
          }
        }
        close(OUTFILE);
      }
      print "$targetFileName [", $flood->{chunkMaps}{$targetFileName}->to_ASCII, "] \n";
    }
  }

}

sub GetChunk {
  my $self = shift;
  my $floodFileHash = shift;
  my $targetFilename = shift;
  my $index = shift;

  my $flood = $self->floods->{$floodFileHash};
  $flood->{$floodFileHash}->{gettingChunk} = 1;
  share($flood->{$floodFileHash}->{gettingChunk});

  my $thread = threads::create->(sub {
    my $targetFile = IO::File->new($flood->{localTargetFilenames}{$targetFilename}, 'r+');
    $targetFile->seek($flood->{chunkOffsets}{$targetFilename}[$index], 0);

    my $chunkData = $self->peers->{$floodFileHash}[0]->simple_request('RequestChunk', $floodFileHash, $targetFilename, $index);

    my $chunk = $flood->{floodData}{FileInfo}{File}{$targetFilename}{Chunk}[$index];
    
    if(sha1_base64($chunkData) eq $chunk->{hash}) {
      $targetFile->print($chunkData);
      $flood->{chunkMaps}{$targetFilename}->Bit_On($index);
    } else {
      print "FAIL!!!!!!!!!!!!!!!!!\n";
    }
    $targetFile->close();
    
    $flood->{gettingChunk} = 0;
  });

  $thread->detach();

}

sub GetChunks {
  my $self = shift;

  foreach my $floodFileHash (keys(%{$self->floods})) {
    my $flood = $self->floods->{$floodFileHash};
    next if($flood->{$floodFileHash}->{gettingChunk}); # this one is getting a chunk right now
    $self->GetChunk($floodFileHash,); # FIXME: busticated
    
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
