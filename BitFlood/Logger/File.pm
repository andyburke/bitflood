package BitFlood::Logger::File;

use strict;

use base qw(BitFlood::Logger);
__PACKAGE__->mk_accessors(qw(filename filehandle append));

use IO::File;
use File::Spec;
use File::Path;

sub new {
  my $class = shift;
  my $filename = shift;
  my %args = @_;

  my $self = $class->SUPER::new(\%args);

  $self->filename($filename);
  $self->filehandle(new IO::File);
  $self->append(exists($args{append}) ? $args{append} : 1);

  unlink($self->filename) if(-e $self->filename and !$self->append);

  return $self;
}

sub commit {
  my $self = shift;
  my $string = shift;

  $self->open();
  $self->filehandle->print(localtime() . ": $string\n");
}

sub open {
  my $self = shift;

  if(!$self->filehandle->opened()) {
    my($volume, $directories, $file) = File::Spec->splitpath($self->filename);
    mkpath($volume . $directories);
    my $prefix = $self->append ? '>>' : '>';
    $self->filehandle->open("${prefix}" . $self->filename) or die($!);
  }
}

sub close {
  my $self = shift;

  $self->filehandle->close() if $self->filehandle->opened();
}

sub DESTROY {
  my $self = shift;
  $self->close();
}

1;
