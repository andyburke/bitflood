use strict;

use IO::File;

use BitFlood::Client;

my $c = BitFlood::Client->new(port => '10103');
$c->AddFloodFile($ARGV[0]);
$c->InitializeTargetFiles();
foreach my $i (0..23) {
  $c->GetChunk('0eoriR7X2Tl326AFPbxJowiDr6I', "morrissey - you are the quarry (full album & b-sides)/02_Irish Blood, English Heart (complete).mp3", $i);
}

