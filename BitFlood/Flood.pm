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

__PACKAGE__->mk_accessors(qw(data filename contentHash localPath 
                             trackers peers));


sub new {
  my $class = shift;

  my $self = $class->SUPER::new(@_);

  $self->trackers([]);
  $self->peers([]);

  $self->open if defined $self->filename;

  return $self;
}

sub open {
  my $self = shift;

  my $fileHandle = IO::File->new($self->filename, 'r');
  defined($fileHandle) or die("Could not open file: " . $self->filename . " ($!)");
  my $contents = join('', $fileHandle->getlines());
  $self->data(XMLin($contents, ForceArray => [qw(File Tracker)]));
  $self->contentHash(sha1_base64($contents));
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
    if(!-f $file->{localFilename})   # file doesn't exist, initialize it to 0
    {
      my $path = GetLocalPathFromFilename($file->{localFilename});
      mkpath($path) if length($path);
      my $outfile = IO::File->new($file->{localFilename}, 'w');
      if($file->{size} > 0) {
        $outfile->seek($file->{size}-1, 0);
        $outfile->syswrite(0, 1);
      }
      $outfile->close;
    }
    else # file DOES exist, we need to decide what we need to get...
    {
      my $outfile = IO::File->new($file->{localFilename}, 'r');
      my $buffer;
      foreach my $chunk (@{$file->{Chunk}}) {
        $outfile->read($buffer, $chunk->{size});
        my $hash = sha1_base64($buffer);
        if($hash eq $chunk->{hash}) {
          $file->{chunkMap}->Bit_On($chunk->{index});
        }
      }
      $outfile->close;
    }
    print "$file->{name} [", $file->{chunkMap}->to_ASCII, "] \n";
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
