package BitFlood::Utils;


require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(&CleanFilename &LocalFilename &GetLocalPathFromFilename
	     &Popdir
             &Stringify
             &ReadableTimeDelta);

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
  return File::Spec->rel2abs(shift);
#  my ($volume, $dirs, $filename) = File::Spec->splitpath(shift, 1);
#  my @directories = File::Spec->splitdir($dirs);
#  return File::Spec->catpath($volume, @directories, $filename);
}

sub GetLocalPathFromFilename {
  my ($volume, $dirs, $filename) = File::Spec->splitpath(File::Spec->canonpath(shift), 0);
  my @directories = File::Spec->splitdir($dirs);
  return File::Spec->catfile($volume, @directories);
}
 
## pop a directory off of the given path...
sub Popdir {
  my $directory = shift;

  my ($volume, $directories, $file) = File::Spec->splitpath($directory);
  # FIXME: this is crap, why does splitpath return some weird char if there are no directories? lame.
  return undef if(!length($directories) || $directories =~ /^\W+$/);
  my @dirs = File::Spec->splitdir($directories);
  pop(@dirs);
  return File::Spec->catfile($volume, @dirs);
}

#####################################################
## String utility functions...
#####################################################


## take anything and turn it into a string...
sub Stringify {
  my $ref = shift;

  my $string;

  # we didn't get a reference, we assume it's a string
  if(!ref($ref)) {
    $string = "$ref";
  } elsif(ref($ref) ne 'CODE') {
    # if we get a reference to some datatype, use Data::Dumper
    # to stringify it...
    use Data::Dumper;
    $string = Dumper($ref);
  } else {
    # otherwise, we got some code, so we call it and expect it to
    # return a string...
    $string = &$ref;
  }

  return $string;
}

#####################################################
## return a more readable time delta...
#####################################################

sub ReadableTimeDelta {
  my $timeDelta = shift;

  $timeDelta =~ /\d+/ or return 'unknown';

  my $hours = sprintf("%02d", int($timeDelta / 3600)); $timeDelta = $timeDelta % 3600;
  my $minutes = sprintf("%02d", int($timeDelta / 60)); $timeDelta = $timeDelta % 60;
  my $seconds = sprintf("%02d", $timeDelta);

  return "$hours:$minutes:$seconds";
}
 

1;
