package BitFlood::Encoder;

use strict;

use base qw(Class::Accessor);

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
  $self->chunkSize($args{chunkSize} || 16384); # default to 1024
  $self->weightingFunction($args{weightingFunction});

  return $self;
}

sub encode {
  my $self = shift;

  $self->filename([$self->filename]) if(!ref($self->filename)); # just a string, make it into an array
  
  foreach my $filename (@{$self->filename}) {
    $self->_EncodeFile($filename) if(-f LocalFilename($filename)); # regular file
    find({ wanted => sub { $self->_EncodeFile($File::Find::name); },
           no_chdir => 1 },
         LocalFilename($filename)) if(-d LocalFilename($filename));
  }

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
  my $filename = shift;
  my $cleanFilename = CleanFilename($filename);
  my $localFilename = LocalFilename($filename);

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

 

1;
