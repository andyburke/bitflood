package BitFlood::Client;

use strict;

use RPC::XML;
use RPC::XML::Server;

use base qw(Class::Accessor);
use base qw(RPC::XML::Server);

use Digest::SHA1  qw(sha1 sha1_hex sha1_base64);
use XML::Simple;
use File::Spec;
use File::Spec::Unix;
use File::Find;
use File::Path;

use BitFlood::Utils;

sub new {
  my $class = shift;
  my %args = @_;

  my $self = RPC::XML::Server->new(port => $args{port} || 10101);
  bless $self, $class;
  $self->mk_accessors(qw(floodFiles));

  $self->floodFiles({});
  $self->neededChunks({});
  
  $self->add_method({
    name => 'RequestChunk', # the name of the method
    version => '0.0.1', # the method version
    hidden => undef,    # is it hidden? undef = no, 1 = yes
    signature => ['base64 string string int'], # return base64 data, take filehash, filename, chunk index
    help => 'A method to request a given chunk.', # help for this method
    code => sub {
      my $self = shift;
      my $filehash = shift;
      my $filename = shift;
      my $index = shift;

      open();
    },
      
  });

  return $self;
}

sub AddFloodFile {
  my $self = shift;
  my $filename = shift;

  return if(!-f $filename);

  open(FLOODFILE, $filename);
  my $data = XMLin(FLOODFILE, ForceArray => ['File']);
  seek(FLOODFILE, 0, 0);
  my $contents = join('', <FLOODFILE>);
  my $fileHash = sha1_base64($contents);
  close(FLOODFILE);

  $self->floodFiles->{$fileHash} = $data;
}

sub InitializeTargetFiles {
  my $self = shift;
  
 foreach my $floodFileHash (keys(%{$self->floodFiles})) {
   my $floodFile = $self->floodFiles->{$floodFileHash};
   foreach my $targetFile (keys(%{$floodFile->{FileInfo}->{File}})) {
     $localFilename = LocalFilename($targetFile);

     if(!-f $localFilename)   # file doesn't exist, initialize it to 0
     {
       mkpath(GetLocalPathFromFilename($localFilename));
       open(OUTFILE, ">$localFilename");
       my $fileSize = $floodFile->{FileInfo}->{File}->{$targetFile}->{Size};
       if($fileSize > 0) {
         seek(OUTFILE, $fileSize-1, 0);
         syswrite(OUTFILE, 0, 1);
       }
       close(OUTFILE);
       foreach my $chunk (@{$floodFile->{FileInfo}->{File}->{$targetFile}->{Chunk}}) {
         #FIXME: this is really inefficient, memory-wise
         if(ref($self->{neededChunks}->{$floodFileHash}->{$targetFile}) ne 'ARRAY') $self->{neededChunks}->{$floodFileHash}->{$targetFile} = []; 
         push(@{$self->neededChunks->{$floodFileHash}->{$targetFile}}, $chunk);
       }
     }
     else # file DOES exist, we need to decide what we need to get...
     {
       open(OUTFILE, "<$localFilename");
       foreach my $chunk (sort { $a->{index} <=> $b->{index} } @{$floodFile->{FileInfo}->{File}->{$targetFile}->{Chunk}}) {
         my $buffer;
         read(OUTFILE, $buffer, $chunk->{size});
         my $hash = sha1_base64($buffer);
         if($hash ne $chunk->{hash}) {
           if(ref($self->{neededChunks}->{$floodFileHash}->{$targetFile}) ne 'ARRAY') $self->{neededChunks}->{$floodFileHash}->{$targetFile} = []; 
           # FIXME: very ineffecient...
           push(@{$self->{neededChunks}->{$floodFileHash}->{$targetFile}}, $chunk);
         }
       }
       close(OUTFILE);
     }
   }
 }
}

1;
