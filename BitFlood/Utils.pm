package BitFlood::Utils;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(&CleanFilename &LocalFilename &GetLocalPathFromFilename);

use strict;

use File::Spec;
use File::Spec::Unix;

sub CleanFilename {
  # FIXME: windows compatible, asshole?
  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
  my @directories = File::Spec->splitdir($dirs);
  shift @directories if( $directories[0] eq '.' );
  return File::Spec::Unix->canonpath(join('/', @directories, $filename));
  #return File::Spec::Unix->catfile($volume, @directories, $filename);
}

sub LocalFilename {
  return File::Spec->canonpath(shift);
#  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
#  my @directories = File::Spec->splitdir($dirs);
#  return File::Spec->catpath($volume, @directories, $filename);
}

sub GetLocalPathFromFilename {
  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 0);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec->catpath($volume, @directories);
}
  

1;
