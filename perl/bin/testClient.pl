use strict;

use Getopt::Long;
use File::Path;
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

foreach my $floodFileDef (@ARGV) {
  my ($floodFile, $localPath) = split(';', $floodFileDef);
  if(-e $floodFile) {
    if(!-d $localPath) {
      print "Creating directory: $localPath\n";
      mkpath($localPath);
    }
    $c->AddFloodFile($floodFile, $localPath);
  } else {
    print "flood: $floodFile does not exist, exiting...\n";
    exit;
  }
}

my $UPDATE_INTERVAL = 20; #seconds
my $last_update = 0;

print "\n==> ready to go <==\n\n";

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
