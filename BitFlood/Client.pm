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
use Bit::Vector;
use Data::Dumper; # XXX

use BitFlood::Utils;


__PACKAGE__->mk_accessors(qw(floods neededChunks));


sub new {
  my $class = shift;
  my %args = @_;

  my $self = RPC::XML::Server->new(port => $args{port} || 10101);
  bless $self, $class;

  $self->floods({});

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

      #open();
    },

  });

  return $self;
}

sub AddFloodFile {
  my $self = shift;
  my $filename = shift;

  return if(!-f $filename);

  open(FLOODFILE, $filename);
  my $floodData = XMLin(\*FLOODFILE, ForceArray => ['File']);
  seek(FLOODFILE, 0, 0);
  my $contents = join('', <FLOODFILE>);
  my $floodFileHash = sha1_base64($contents);
  close(FLOODFILE);

  my $chunkMaps = $self->BuildChunkMaps($floodData);
  $self->floods->{$floodFileHash} = { floodData  => $floodData,
                                      chunkMaps  => $chunkMaps };
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


sub InitializeTargetFiles {
  my $self = shift;

  foreach my $flood (values(%{$self->floods})) {
    my $floodData = $flood->{floodData};
    while (my ($targetFileName, $targetFile) = each %{$floodData->{FileInfo}->{File}}) {
      my $localFilename = LocalFilename($targetFileName);

      if(!-f $localFilename)   # file doesn't exist, initialize it to 0
      {
        mkpath(GetLocalPathFromFilename($localFilename));
        open(OUTFILE, ">$localFilename");
        my $fileSize = $targetFile->{Size};
        if($fileSize > 0) {
          seek(OUTFILE, $fileSize-1, 0);
          syswrite(OUTFILE, 0, 1);
        }
        close(OUTFILE);
      }
      else # file DOES exist, we need to decide what we need to get...
      {
        open(OUTFILE, "<$localFilename");
        my $buffer;
        foreach my $chunk (sort { $a->{index} <=> $b->{index} } @{$targetFile->{Chunk}}) {
          read(OUTFILE, $buffer, $chunk->{size});
          my $hash = sha1_base64($buffer);
          if($hash eq $chunk->{hash}) {
            $flood->{chunkMaps}{$targetFileName}->Bit_On($chunk->{index});
          }
        }
        close(OUTFILE);
      }
      print "CM: $targetFileName [", $flood->{chunkMaps}{$targetFileName}->to_ASCII, "] \n";
    }
  }

}

1;
