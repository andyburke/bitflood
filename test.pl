use strict;

use BitFlood::Encoder;

my $targetFile = shift(@ARGV);

my $e = new BitFlood::Encoder(filename => @ARGV > 1 ? \@ARGV : $ARGV[0],
                              weightingFunction => 'topheavy',
                              tracker => ['http://blah.com/blah', 'http://moo/yak']);

open(OUTFILE, ">$targetFile");
print OUTFILE $e->encode();
close(OUTFILE);
