package BitFlood::Flood;

use strict;

use base qw(BitFlood::Accessor);

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
use BitFlood::Debug;

__PACKAGE__->mk_accessors(qw(data filename contentHash localPath 
                             trackers peers sessionStartTime startTime totalBytes sessionDownloadBytes downloadBytes
                             neededChunksByWeight paused));


sub new {
  my $class = shift;

  Debug('>>>', 'trace');

  my $self = $class->SUPER::new(@_);

  $self->trackers([]);
  $self->peers([]);
  $self->totalBytes(0);
  $self->downloadBytes(0);
  $self->sessionDownloadBytes(0);
  $self->neededChunksByWeight([]);
  $self->paused(0);
  
  $self->open if defined $self->filename;

  Debug('<<<', 'trace');
  return $self;
}

sub open {
  my $self = shift;

  Debug('>>>', 'trace');

  my $fileHandle = IO::File->new($self->filename, 'r');
  defined($fileHandle) or die("Could not open file: " . $self->filename . " ($!)");
  my $contents = join('', $fileHandle->getlines());
  $self->data(XMLin($contents, ForceArray => [qw(File Tracker Chunk)]));

  $fileHandle->close();

  # NOTE: this guarantees that our chunks are in order, per target file,
  #       but means that we can NEVER write the floodfile back out, so
  #       screw you
  $self->SetFilenames();
  $self->SortChunksInPlace();
  $self->BuildChunkMaps();
  $self->BuildChunkOffsets();
  $self->BuildLocalFilenames();
  $self->InitializeFiles();
  $self->SortNeededChunksByWeight();

  # build the content hash
  $contents = "";
  foreach my $file ( sort keys %{$self->Files} )
  {
    $contents .= $file;
    foreach my $chunk ( @{$self->Files->{$file}{Chunk}} )
    {
      $contents .= $chunk->{hash};
    }
  }

  $self->contentHash(sha1_base64($contents));
  Debug("content input: ".$contents, 10);
  Debug("content hash: ".$self->contentHash, 5);

  Debug('<<<', 'trace');
}

sub SetFilenames {
  my $self = shift;

  Debug('>>>', 'trace');

  while (my ($filename, $file) = each %{$self->Files}) {
    $file->{name} = $filename;
  }
  Debug('<<<', 'trace');
}


sub SortChunksInPlace {
  my $self = shift;

  Debug('>>>', 'trace');
  foreach my $targetFile (values(%{$self->Files})) {
    $targetFile->{Chunk} = [ sort { $a->{index} <=> $b->{index} } @{$targetFile->{Chunk}} ];
  }
  Debug('<<<', 'trace');
}

sub SortNeededChunksByWeight {
  my $self = shift;
  Debug('>>>', 'trace');
  $self->neededChunksByWeight([ sort { $b->{chunk}->{weight} <=> $a->{chunk}->{weight} } @{$self->neededChunksByWeight} ]);
  Debug('<<<', 'trace');
}


sub BuildChunkMaps {
  my $self = shift;

  Debug('>>>', 'trace');

  my %chunkMaps;
  foreach my $file (values(%{$self->Files})) {
    $file->{chunkMap} = Bit::Vector->new(scalar @{$file->{Chunk}});
  }
  Debug('<<<', 'trace');
}

sub BuildChunkOffsets {
  my $self = shift;

  Debug('>>>', 'trace');

  foreach my $file (values(%{$self->Files})) {
    my $offset = 0;
    foreach my $chunk (@{$file->{Chunk}}) {
      $chunk->{offset} = $offset;
      $offset += $chunk->{size};
    }
  }
  Debug('<<<', 'trace');
}


sub BuildLocalFilenames {
  my $self = shift;

  Debug('>>>', 'trace');

  while (my ($filename, $file) = each  %{$self->Files}) {
    $file->{localFilename} = LocalFilename(File::Spec->catfile($self->localPath, $filename));
  }
  Debug('<<<', 'trace');
}

sub InitializeFiles {
  my $self = shift;

  Debug('>>>', 'trace');

  foreach my $file (values %{$self->Files}) {
    $self->totalBytes($self->totalBytes + $file->{size});
    if(!-f $file->{localFilename})   # file doesn't exist, initialize it to 0
    {
      my $path = GetLocalPathFromFilename($file->{localFilename});
      mkpath($path) if length($path);
      my $outfile = IO::File->new("$file->{localFilename}", 'w');
      if(!$outfile) {
	Debug("Could not open output file: $file->{localFilename} ($!)");
	next;
      }
      if($file->{size} > 0) {
        $outfile->seek($file->{size}-1, 0);
        $outfile->syswrite(0, 1);
      }
      $outfile->close;
      foreach my $chunk (@{$file->{Chunk}}) {
	push(@{$self->neededChunksByWeight}, { filename => $file->{name}, chunk => $chunk});
      }
    }
    else # file DOES exist, we need to decide what we need to get...
    {
      my $outfile = IO::File->new($file->{localFilename}, 'r');
      if(!$outfile) {
	Debug("Could not open output file: $file->{localFilename} ($!)");
	next;
      }
      my $buffer;
      my $totalChunks = scalar(@{$file->{Chunk}});
      my $validChunkCount = 0;
      foreach my $chunk (@{$file->{Chunk}}) {
	printf("Checking: %-35.35s [%6.2f%%]\r", substr($file->{name}, -35), 100 * $chunk->{index} / $totalChunks);
        $outfile->sysread($buffer, $chunk->{size});
        my $hash = sha1_base64($buffer);
        if($hash eq $chunk->{hash}) {
	  $validChunkCount++;
          $file->{chunkMap}->Bit_On($chunk->{index});
	  $self->downloadBytes($self->downloadBytes + $chunk->{size});
        } else {
	  push(@{$self->neededChunksByWeight}, { filename => $file->{name}, chunk => $chunk});
	}
      }
      $outfile->close;

      printf("Checking: %-35.35s [%6.2f%%] [%d/%d chunks OK]\n", substr($file->{name}, -35), 100, $validChunkCount, $totalChunks);
    }
  }
  Debug('<<<', 'trace');

}


sub Files {
  my $self = shift;

  return $self->data->{FileInfo}{File};
}

sub TrackerURLs {
  my $self = shift;

  return $self->data->{Tracker};
}

1;
