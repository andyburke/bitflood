use strict;

use BitFlood::Client;

my $c = BitFlood::Client->new(port => $ARGV[1] || 10102);
$c->AddFloodFile($ARGV[0]);

my $UPDATE_INTERVAL = 20; #seconds
my $last_update = 0;

while(1) {
  if (time() - $last_update >= $UPDATE_INTERVAL) {
    $c->Register;
    $c->UpdatePeerList;
    $last_update = time();
  }
  $c->GetChunks;
  $c->do_one_loop();
}
