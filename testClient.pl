use strict;

use BitFlood::Client;

my $c = BitFlood::Client->new(port => $ARGV[1] || 10102);
$c->AddFloodFile($ARGV[0]);
while(1) {
  $c->GetChunks;
  $c->do_one_loop();
}
