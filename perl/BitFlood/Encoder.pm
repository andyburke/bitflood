package BitFlood::Encoder;

use strict;

use base qw(BitFlood::Accessor);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Find;

use BitFlood::Utils;

sub new {
  my $class = shift;
  my %args = @_;

  my $self = bless {}, $class;
  $self->mk_accessors(qw(data filename chunkSize weightingFunction));

  die "No filename specified!" if(!defined($args{filename}));
  
  $self->data({});
  $self->data->{Tracker} = $args{tracker};
  $self->filename($args{filename});
  $self->chunkSize($args{chunkSize} || 2**18); # default to 256k
  $self->weightingFunction($args{weightingFunction});

  return $self;
}

sub encode {
  my $self = shift;

  $self->filename([$self->filename]) if(!ref($self->filename)); # just a string, make it into an array
  
  my @fileInfo;
  foreach my $filename (@{$self->filename}) {
    $filename = LocalFilename($filename);
    if(-f $filename)  # regular file
    {
      my (undef, $dirPart) = File::Spec->splitpath($filename, 1);
      my (@dirs) = File::Spec->splitdir($dirPart);
      my $cleanFilename = pop(@dirs);
      push(@fileInfo, { cleanFilename => CleanFilename($cleanFilename),
                        fullFilename => $filename } );
    }
    elsif(-d $filename) 
    {
      find({ wanted => sub {
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
  print "Encoding " . scalar(@fileInfo) . " files:\n";
  foreach my $fileStruct (sort { $a->{cleanFilename} cmp $b->{cleanFilename} } @fileInfo) {
    $self->_EncodeFile($fileStruct);
  }
  
  if($self->weightingFunction eq 'topheavyperfile')
  {
    foreach my $file (values(%{$self->data->{Files}})) {
      $file->{Chunk} or next;
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = $numChunks - $i;
      }
    }
  }
  elsif($self->weightingFunction eq 'bottomheavyperfile')
  {
    foreach my $file (values(%{$self->data->{Files}})) {
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
    foreach my $file (values(%{$self->data->{Files}})) {
      $file->{Chunk} or next;
      $totalChunks += scalar(@{$file->{Chunk}});
    }
    foreach my $filename (sort(keys(%{$self->data->{Files}}))) {
      my $file = $self->data->{Files}->{$filename};
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
    foreach my $filename (sort(keys(%{$self->data->{Files}}))) {
      my $file = $self->data->{Files}->{$filename};
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
    foreach my $file (values(%{$self->data->{Files}})) {
      $file->{Chunk} or next;
      my $numChunks = scalar(@{$file->{Chunk}});
      for(my $i = 0; $i < $numChunks; $i++) {
	$file->{Chunk}->[$i]->{weight} = int(rand() * $numChunks);
      }
    }
  }
  

  print "Total Time: " . (time() - $startTime) . "s\n";

  return XMLout(
                {
                  FileInfo => { File => [$self->data->{Files}]},
                  Tracker => $self->data->{Tracker},
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
  printf("%6.2f%% %11.3f MB | %s\r", 0, 0, $cleanFilename);

  my $totalSize = -s INFILE;
  my $buffer;
  my $index = 0;
  my $bytesRead = 0;
  while(my $readLength = read(INFILE, $buffer, $self->chunkSize)) {
    push(@{$self->data->{Files}->{$cleanFilename}->{Chunk}}, { index => $index++,
                                                               hash => sha1_base64($buffer),
                                                               size => $readLength,
							       weight => 0,
                                                             });
    $bytesRead += length($buffer);
    printf("%6.2f%% %11.3f\r", 100*$bytesRead/$totalSize, $bytesRead/1048576);
  }
  $self->data->{Files}->{$cleanFilename}->{Size} = $bytesRead;
  print "100.00%\n";

  close(INFILE);
}

 

1;