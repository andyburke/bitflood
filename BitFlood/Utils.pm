package BitFlood::Utils;

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(&CleanFilename &LocalFilename &GetLocalPathFromFilename);

use File::Spec;
use File::Spec::Unix;

sub CleanFilename {
  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec::Unix->catpath($volume, @directories, $filename);
}

sub LocalFilename {
  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec->catpath($volume, @directories, $filename);
}

sub GetLocalPathFromFilename {
  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 0);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec->catpath($volume, @directories);
}
  

1;
