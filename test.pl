use strict;

use BitFlood::Encoder;

my $e = new BitFlood::Encoder(filename => @ARGV > 1 ? \@ARGV : $ARGV[0],
                              weightingFunction => 'topheavy',
                              tracker => ['http://blah.com/blah', 'http://moo/yak']);
$e->encode();
