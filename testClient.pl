use strict;

use BitFlood::Client;

my $c = BitFlood::Client->new(port => 10102);
$c->AddFloodFile($ARGV[0]);
$c->InitializeTargetFiles();
$c->Start();
