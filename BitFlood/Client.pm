package BitFlood::Client;

use base qw(Class::Accessor);

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

  my $self = bless {}, $class;
  $self->mk_accessors(qw(floodFiles));

  $self->floodFiles([]);

  return $self;
}

sub AddFloodFile {
  my $self = shift;
  my $filename = shift;

  return if(!-f $filename);

  my $data = XMLin($filename, ForceArray => ['File']);

  push(@{$self->floodFiles}, { filename => $filename, data => $data });
}

sub InitializeTargetFiles {
  my $self = shift;
  
 foreach my $file (@{$self->floodFiles}) {
   foreach my $targetFile (keys(%{$file->{data}->{FileInfo}->{File}})) {
     $localFilename = LocalFilename($targetFile);

     if(!-f $localFilename)   # file doesn't exist, initialize it to 0
     {
       mkpath(GetLocalPathFromFilename($localFilename));
       open(OUTFILE, ">$localFilename");
       my $fileSize = $file->{data}->{FileInfo}->{File}->{$targetFile}->{Size};
       if($fileSize > 0) {
         seek(OUTFILE, $fileSize-1, 0);
         syswrite(OUTFILE, 0, 1);
       }
       close(OUTFILE);
     }
     else # file DOES exist, we need to decide what we need to get...
     {
       open(OUTFILE, "<$localFilename");
#       my $curPos = 0;
       foreach my $chunk (sort { $a->{index} <=> $b->{index} } @{$file->{data}->{FileInfo}->{File}->{$targetFile}->{Chunk}}) {
         my $buffer;
#         seek(OUTFILE, $curPos, 0);
         read(OUTFILE, $buffer, $chunk->{size});
         my $hash = sha1_base64($buffer);
         if($hash ne $chunk->{hash}) {
           print "chunk $chunk->{index} mismatch\n";
         }
         # FIXME: add the chunk to a list of chunks we need...
 #        $curPos += $chunk->{size};
       }
       close(OUTFILE);
     }
   }
 }
}

1;
