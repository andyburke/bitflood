use strict;

use Time::HiRes qw(time);
use BitFlood::Client;

my $c = BitFlood::Client->new();

$SIG{INT} = sub { $c->Disconnect(); exit(0); };

foreach my $floodFile (@ARGV) {
  $c->AddFloodFile($floodFile);
}

my $UPDATE_INTERVAL = 20; #seconds
my $last_update = 0;

while(1) {
#  my $stime = time();
  if (time() - $last_update >= $UPDATE_INTERVAL) {
    $c->Register;
    $c->UpdatePeerList;
    $last_update = time();
  }
  $c->GetChunks;
  $c->LoopOnce();
#  print "loop time: ", time() - $stime, "\n";
}
