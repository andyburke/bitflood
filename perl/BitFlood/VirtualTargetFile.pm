package BitFlood::VirtualTargetFile;

use strict;

use base qw(BitFlood::Accessor);

use File::Spec;
use IO::File;
use Digest::SHA1;

use BitFlood::VirtualChunk;


__PACKAGE__->mk_accessors(qw(floodFile path size chunks));


sub initialize {
  my $self = shift;
  my ($args) = @_;

  $self->SUPER::initialize(@_);
  Scalar::Util::weaken($self->floodFile);
  $self->InitializePath($args->{localPath});
  $self->InitializeChunks($args->{localPath});

  return $self;
}


sub InitializePath {
  my $self = shift;
  my $localPath = shift;

  return if defined $self->path;

  my @path = File::Spec->splitpath($localPath);
  my $fileName = pop(@path);
  $self->path($fileName);
}


sub InitializeChunks {
  my $self = shift;
  my $path = shift;

  return if $self->chunks;

  my $handle = IO::File->new($path, 'r');
  $self->size(-s $handle);

  $self->chunks([]);
  my $buffer;
  my $index = 0;
  ###my $bytesRead = 0;
  while (my $readLength = $handle->read($buffer, $self->floodFile->chunkSize)) {
    push(
         @{$self->chunks},
         BitFlood::VirtualChunk->new({
                                      file   => $self,
                                      index  => $index++,
                                      size   => $readLength,
                                      weight => 0,
                                      hash   => Digest::SHA1::sha1_base64($buffer),
                                     })
        );
    ###$bytesRead += length($buffer);
    ###printf("%6.2f%% %11.3f\r", 100*$bytesRead/$totalSize, $bytesRead/1048576); # FIXME move to caller
  }
}



1;
