use strict;

use Getopt::Long;
use Time::HiRes qw(time sleep);
use BitFlood::Client;

my $localIp;
my $help;

if(!GetOptions(
	       "localip=s" => \$localIp,
	       "help" => \$help,
	       )
   or $help) {
  print "localip => set ip\n";
  print "help/h  => this\n";
  exit;
}

my $c = BitFlood::Client->new(localIp => $localIp);

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
  sleep(.1);
#  print "loop time: ", time() - $stime, "\n";
}
