package BitFlood::Flood;

use strict;

use base qw(Class::Accessor);

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
                             trackers peers startTime totalBytes downloadBytes
                             neededChunksByWeight));


sub new {
  my $class = shift;

  my $self = $class->SUPER::new(@_);

  $self->trackers([]);
  $self->peers([]);
  $self->totalBytes(0);
  $self->downloadBytes(0);
  $self->neededChunksByWeight([]);

  $self->open if defined $self->filename;

  return $self;
}

sub open {
  my $self = shift;

  my $fileHandle = IO::File->new($self->filename, 'r');
  defined($fileHandle) or die("Could not open file: " . $self->filename . " ($!)");
  my $contents = join('', $fileHandle->getlines());
  $self->data(XMLin($contents, ForceArray => [qw(File Tracker Chunk)]));
  $self->contentHash(sha1_base64($contents));
  Debug("content hash: ".$self->contentHash, 5);
  undef $contents;
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
}

sub SetFilenames {
  my $self = shift;

  while (my ($filename, $file) = each %{$self->Files}) {
    $file->{name} = $filename;
  }
}


sub SortChunksInPlace {
  my $self = shift;

  foreach my $targetFile (values(%{$self->Files})) {
    $targetFile->{Chunk} = [ sort { $a->{index} <=> $b->{index} } @{$targetFile->{Chunk}} ];
  }
}

sub SortNeededChunksByWeight {
  my $self = shift;
  $self->neededChunksByWeight([ sort { $b->{chunk}->{weight} <=> $a->{chunk}->{weight} } @{$self->neededChunksByWeight} ]);
}


sub BuildChunkMaps {
  my $self = shift;

  my %chunkMaps;
  foreach my $file (values(%{$self->Files})) {
    $file->{chunkMap} = Bit::Vector->new(scalar @{$file->{Chunk}});
  }
}

sub BuildChunkOffsets {
  my $self = shift;

  foreach my $file (values(%{$self->Files})) {
    my $offset = 0;
    foreach my $chunk (@{$file->{Chunk}}) {
      $chunk->{offset} = $offset;
      $offset += $chunk->{size};
    }
  }
}


sub BuildLocalFilenames {
  my $self = shift;

  while (my ($filename, $file) = each  %{$self->Files}) {
    $file->{localFilename} = LocalFilename(File::Spec->catfile($self->localPath, $filename));
  }
}

sub InitializeFiles {
  my $self = shift;

  foreach my $file (values %{$self->Files}) {
    $self->totalBytes($self->totalBytes + $file->{size});
    if(!-f $file->{localFilename})   # file doesn't exist, initialize it to 0
    {
      print "Initializing target file: $file->{name}\n";
      my $path = GetLocalPathFromFilename($file->{localFilename});
      mkpath($path) if length($path);
      my $outfile = IO::File->new($file->{localFilename}, 'w');
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
      my $buffer;
      my $totalChunks = scalar(@{$file->{Chunk}});
      my $validChunkCount = 0;
      foreach my $chunk (@{$file->{Chunk}}) {
	printf("Checking: %-35.35s [%6.2f%%]\r", substr($file->{name}, -35), 100 * $chunk->{index} / $totalChunks);
        $outfile->read($buffer, $chunk->{size});
        my $hash = sha1_base64($buffer);
        if($hash eq $chunk->{hash}) {
	  $validChunkCount++;
          $file->{chunkMap}->Bit_On($chunk->{index});
	  $self->downloadBytes($self->downloadBytes + $chunk->{length});
        } else {
	  push(@{$self->neededChunksByWeight}, { filename => $file->{name}, chunk => $chunk});
	}
      }
      $outfile->close;

      printf("Checking: %-35.35s [%6.2f%%] [%d/%d chunks OK]\n", substr($file->{name}, -35), 100, $validChunkCount, $totalChunks);
    }
  }
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
