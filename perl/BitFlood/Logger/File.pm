package BitFlood::Logger::File;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(filename filehandle append));

use IO::File;
use File::Spec;
use File::Path;
use POSIX qw(strftime);
use Time::HiRes qw(time);


sub new {
  my $class = shift;
  my $filename = shift;
  my %args = @_;

  my $self = $class->SUPER::new(\%args);

  $self->filename(File::Spec->rel2abs($filename));
  $self->filehandle(new IO::File);
  $self->filehandle->autoflush(1);
  $self->append(exists($args{append}) ? $args{append} : 1);

  unlink($self->filename) if(-e $self->filename and !$self->append);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  $self->open();
  my $time = time();
  my $timeString = sprintf(
			   '%s.%03d',
			   strftime('%Y%m%d%H%M%S', localtime($time)),
			   ($time - int($time)) * 1000
			   );
  $self->filehandle->print("$timeString: $string\n");
}

sub open {
  my $self = shift;

  if(!$self->filehandle->opened()) {
    my($volume, $directories, $file) = File::Spec->splitpath($self->filename);
    mkpath($volume . $directories);
    my $prefix = $self->append ? '>>' : '>';
    $self->filehandle->open($prefix . $self->filename) or die($!);
  }
}

sub close {
  my $self = shift;

  $self->filehandle->close() if $self->filehandle->opened();
}


1;
