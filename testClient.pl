use strict;

use BitFlood::Client;

my $c = new BitFlood::Client;
$c->AddFloodFile($ARGV[0]);
$c->InitializeTargetFiles();
