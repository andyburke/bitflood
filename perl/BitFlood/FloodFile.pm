package BitFlood::FloodFile;

use strict;

use base qw(BitFlood::Accessor);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Find;

use BitFlood::Utils;
use BitFlood::Debug;
use BitFlood::VirtualTargetFile;

__PACKAGE__->mk_accessors(qw(trackers files chunkSize));


sub initialize {
  my $self = shift;
  my ($args) = @_;

  $self->SUPER::initialize(@_);
  # probably not useful to pass trackers/files to new() but we'll allow it
  $self->trackers  or $self->trackers([]);
  $self->files     or $self->files([]);
  $self->chunkSize or $self->chunkSize(2**18); # default to 256k

  return $self;
}


sub AddFile {
  my $self = shift;
  my $localFilePath = shift;
  my $targetFilePath = shift;

  my $virtualTargetFile = BitFlood::VirtualTargetFile->new({floodFile => $self,
                                                            localPath => $localFilePath,
                                                            path      => CleanPath($targetFilePath)});
  push(@{$self->files}, $virtualTargetFile);
}


sub AddDir {
  my $self = shift;
  my $localDirPath = shift;

  my @path = File::Spec->splitdir($localDirPath);
  my $baseDir = pop(@path);

  my @localFilePaths;
  find({ wanted  => sub { push(@localFilePaths, $File::Find::name) unless -d _ } }, $localDirPath);
  foreach my $localFilePath (@localFilePaths) {
    (my $targetFilePath = $localFilePath) =~ s/\Q$localDirPath\E/$baseDir/;
    $self->AddFile($localFilePath, $targetFilePath);
  }
}


sub ComputeContentHash {
  my $self = shift;
}


sub encode {
  my $self = shift;

  my @fileInfo;
  foreach my $filename (@{$self->filenames}) {
    $filename = LocalFilename($filename);
    if (-f $filename)  # regular file
    {
      my (undef, $dirPart) = File::Spec->splitpath($filename, 1);
      my (@dirs) = File::Spec->splitdir($dirPart);
      my $cleanFilename = pop(@dirs);
      push(@fileInfo, { cleanFilename => CleanFilename($cleanFilename),
                        fullFilename  => $filename } );
    }
    elsif (-d $filename)
    {
      find({ wanted =>
             sub {
               return if(-d $_);
               my $fullFilename = $_;
               my $cleanFilename = $_;
               my @dirs = File::Spec->splitdir($filename);
               my $lastDir = pop(@dirs);
               $cleanFilename =~ s/\Q$filename\E/$lastDir/;
               push(@fileInfo, { cleanFilename => CleanFilename($cleanFilename),
                                 fullFilename => LocalFilename($fullFilename) });
             },
             no_chdir => 1
           },
           $filename);
    }
  }

  my $startTime = time();
  ### print "Encoding " . scalar(@fileInfo) . " files:\n"; # FIXME move to caller
  foreach my $fileStruct (sort { $a->{cleanFilename} cmp $b->{cleanFilename} } @fileInfo) {
    $self->_EncodeFile($fileStruct);
  }

  if($self->weightingFunction eq 'topheavyperfile')
  {
    foreach my $file (values(%{$self->fileData})) {
      $file->{Chunk} or next;
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = $numChunks - $i;
      }
    }
  }
  elsif($self->weightingFunction eq 'bottomheavyperfile')
  {
    foreach my $file (values(%{$self->fileData})) {
      $file->{Chunk} or next;
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = $i;
      }
    }
  }
  elsif($self->weightingFunction eq 'topheavy')
  {
    my $totalChunks = 0; 
    foreach my $file (values(%{$self->fileData})) {
      $file->{Chunk} or next;
      $totalChunks += scalar(@{$file->{Chunk}});
    }
    foreach my $filename (sort(keys(%{$self->fileData}))) {
      my $file = $self->fileData->{$filename};
      $file->{Chunk} or next;      
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = $totalChunks--;
      }
    }
  }
  elsif($self->weightingFunction eq 'bottomheavy')
  {
    my $weight = 0;
    foreach my $filename (sort(keys(%{$self->fileData}))) {
      my $file = $self->fileData->{$filename};
      $file->{Chunk} or next;
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = $weight++;
      }
    }
  }
  else
  {
    # random weighting
    foreach my $file (values(%{$self->fileData})) {
      $file->{Chunk} or next;
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = int(rand() * $numChunks); # FIXME make weights unique?
      }
    }
  }


  ###print "Total Time: " . (time() - $startTime) . "s\n"; # FIXME move to caller

  return XMLout(
                {
                 FileInfo => { File => $self->fileData},
                 Tracker => $self->trackerData,
                },
                RootName => 'BitFlood',
                );

}

## FIXME: this can't end up being done in memory...

sub _EncodeFile {
  my $self = shift;
  my $args = shift;

  my $cleanFilename = $args->{cleanFilename};
  my $localFilename = $args->{fullFilename};

  return if(-d $localFilename);

  open(INFILE, $localFilename);

  $| = 1;
  ### printf("%6.2f%% %11.3f MB | %s\r", 0, 0, $cleanFilename); # FIXME move to caller

  my $totalSize = -s INFILE;
  my $buffer;
  my $index = 0;
  my $bytesRead = 0;
  while(my $readLength = read(INFILE, $buffer, $self->chunkSize)) {
    push(
         @{$self->fileData->{$cleanFilename}->{Chunk}},
         {
          index  => $index++,
          hash   => sha1_base64($buffer),
          size   => $readLength,
          weight => 0,
         }
        );
    $bytesRead += length($buffer);
    ###printf("%6.2f%% %11.3f\r", 100*$bytesRead/$totalSize, $bytesRead/1048576); # FIXME move to caller
  }
  $self->fileData->{$cleanFilename}->{size} = $bytesRead;
  ###print "100.00%\n"; # FIXME move to caller

  close(INFILE);
}



1;
