use strict;

use BitFlood::Client;

my $c = BitFlood::Client->new(port => 10102);
$c->AddFloodFile($ARGV[0]);
$c->InitializeTargetFiles();
while(1) {
  # FIXME: get pieces here
  $c->do_one_loop();
}
