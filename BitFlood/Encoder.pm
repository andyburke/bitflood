package BitFlood::Encoder;

use strict;

use base qw(Class::Accessor);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Spec;
use File::Spec::Unix;
use File::Find;

sub new {
  my $class = shift;
  my %args = @_;

  my $self = bless {}, $class;
  $self->mk_accessors(qw(data filename chunkSize weightingFunction));

  die "No filename specified!" if(!defined($args{filename}));
  
  $self->data({});
  $self->data->{Tracker} = $args{tracker};
  $self->filename($args{filename});
  $self->chunkSize($args{chunkSize} || 1024); # default to 1024
  $self->weightingFunction($args{weightingFunction});

  return $self;
}

sub encode {
  my $self = shift;

  if(!ref($self->filename)) # just a string
  {
    $self->_EncodeFile($self->filename) if(-f $self->_LocalFilename($self->filename)); # regular file
    find({ wanted => sub { $self->_EncodeFile($File::Find::name); },
           no_chdir => 1},
         $self->_LocalFilename($self->filename)) if(-d $self->_LocalFilename($self->filename));
  }
  elsif(ref($self->filename) eq 'ARRAY') # a list of files...
  {
    foreach my $filename (@{$self->filename}) {
      $self->_EncodeFile($filename) if(-f $self->_LocalFilename($filename)); # regular file
      find({ wanted => sub { $self->_EncodeFile($File::Find::name); },
             no_chdir => 1 },
           $self->_LocalFilename($filename)) if(-d $self->_LocalFilename($filename));
    }
  }
  else
  {
    die "Unknown filename specification!";
  }

   print XMLout(
                {
                  FileInfo => { File => [$self->data->{Files}]},
                  Tracker => $self->data->{Tracker},
                },
                RootName => 'BitFlood',
               );


}

sub _EncodeFile {
  my $self = shift;
  my $filename = shift;
  my $cleanFilename = $self->_CleanFilename($filename);
  my $localFilename = $self->_LocalFilename($filename);

  return if(-d $localFilename);

  open(INFILE, $localFilename);
  
  my $buffer;
  my $index = 0;
  while(read(INFILE, $buffer, $self->chunkSize)) {
    push(@{$self->data->{Files}->{$cleanFilename}->{Chunk}}, { index => $index++,
                                                               hash => sha1_base64($buffer),
                                                               size => length($buffer),
                                                             });
    $self->data->{Files}->{$cleanFilename}->{Size} += length($buffer);
  }

  close(INFILE);

  if($self->weightingFunction eq 'topheavy') {
    for(my $i = 0; $i < $index; $i++) {
      $self->data->{Files}->{$cleanFilename}->{Chunk}->[$i]->{weight} = $index - $i;
    }
  } elsif($self->weightingFunction eq 'bottomheavy') {
    for(my $i = 0; $i < $index; $i++) {
      $self->data->{Files}->{$cleanFilename}->{Chunk}->[$i]->{weight} = $i;
    }
  } else {
    for(my $i = 0; $i <= $index; $i++) {
      $self->data->{Files}->{$cleanFilename}->{Chunk}->[$i]->{weight} = 0;
    }
  }
  
}

sub _CleanFilename {
  my $self = shift;
 
  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec::Unix->catpath($volume, @directories, $filename);
}

sub _LocalFilename {
  my $self = shift;

  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec->catpath($volume, @directories, $filename);
}

 

1;
