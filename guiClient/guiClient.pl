use strict;

use Win32::GUI;
use Win32 ();
use Win32::GUI::Loft::Design;

use File::Spec;
use File::Path;
use Getopt::Long;
use XML::Simple;
use Time::HiRes qw(usleep);

use constant UPDATE_INTERVAL => 20;

# this locates the BitFlood/ directory and adds it to our include path
BEGIN {
#  print "got: $0\n";
  my ($volume, $directories, $file) = File::Spec->splitpath($0);
  my @dirs = File::Spec->splitdir($directories);
  pop(@dirs) while(@dirs && !-e File::Spec->catfile($volume, @dirs, 'BitFlood'));
#  print "pushing on inc: " . File::Spec->catfile($volume, @dirs) . "\n";
  push(@INC, File::Spec->catfile($volume, @dirs));
}

use BitFlood::Client;

###########################
## floods_ --> Floods tab

my ($drive, @directories) = File::Spec->splitpath($0);
pop(@directories);

my $mainWindowDef = File::Spec->catfile($drive, @directories, 'guiClient.gld');

my $objDesign = Win32::GUI::Loft::Design->newLoad($mainWindowDef) or
  die("Could not open window file ($mainWindowDef)");

my $mainWindow = $objDesign->buildWindow() or die("Could not build window ($mainWindowDef)");

my $client = BitFlood::Client->new();

$mainWindow->Show();
#Win32::GUI::SetCursor($oldCursor);  #show previous arrow cursor again

foreach my $filename (@ARGV) {
  $client->AddFloodFile($filename);
}

my $lastUpdateTime = 0;
my $lastGUIUpdateTime = 0;
while(1) {
#  print "looping... ";
  if(time() - $lastGUIUpdateTime >= 1) {
    UpdateDisplay();
    $lastGUIUpdateTime = time();
  }

  Win32::GUI::DoEvents();
  
  if (time() - $lastUpdateTime >= UPDATE_INTERVAL) {
    $client->Register;
    $client->UpdatePeerList;
    $lastUpdateTime = time();
  }

  $client->GetChunks();
  $client->LoopOnce();
  
#  print "sleeping ... ";
  usleep(1000);
#  print "done\n";
}

sub mainWindow_Resize {
}

sub mainWindow_Terminate {
  -1;
}

sub floods_QuitButton_Click { QuitButton_Click(); }
sub QuitButton_Click {
  $client->Disconnect();
  undef $client;
  exit(0);
}

sub floods_OpenButton_Click {
  my $filename = $mainWindow->floods_FloodFilenameEntry->Text();

  $client->AddFloodFile($filename);
}

sub UpdateDisplay {
  $mainWindow->floods_FloodsListView->Clear();

  foreach my $flood (values(%{$client->floods})) {

    my $seeds = 0;
    my $peers = 0;

    foreach my $peer (@{$client->peers}) {
      my $chunkMaps = $peer->chunkMaps->{$flood->contentHash};
      if(defined($chunkMaps)) {
	$peers++;

	my $allFilesComplete = 1;
	foreach my $filename (keys(%{$chunkMaps})) {
	  if(!$chunkMaps->{$filename}->is_full()) {
	    $allFilesComplete = 0;
	    print "$filename is not complete..." . $chunkMaps->{$filename}->to_Enum() . "\n";
	  }
	}
	
	$seeds++ if $allFilesComplete;
      }
    }

    my $complete = $flood->downloadBytes == $flood->totalBytes;
    $mainWindow->floods_FloodsListView->InsertItem(-text => [
							     $flood->filename,
							     ReadableSize($flood->totalBytes),
							     ReadableSize($flood->downloadBytes),
							     $flood->totalBytes > 0 ? sprintf("%3.2f%%", 100 * $flood->downloadBytes / $flood->totalBytes) : '0%',
							     !$complete ? ReadableTimeDelta(time() - $flood->startTime) : 'done',
							     (!$complete and ($flood->downloadBytes > 0) and (time() - $flood->startTime > 0)) ? ReadableTimeDelta(($flood->totalBytes - $flood->downloadBytes) / ($flood->downloadBytes / (time() - $flood->startTime))) : 'unknown',
							     (!$complete and time() - $flood->startTime > 0) ? sprintf("%s/s", ReadableSize($flood->downloadBytes / (time() - $flood->startTime))) : '0.0 K/s',
							     $seeds, # FIXME: find complete peers
							     $peers - $seeds, # FIXME: find incomplete peers
							     'n/a',
							     $complete ? 'complete' : 'incomplete',
							    ]);
  }
}

sub ReadableTimeDelta {
  my $timeDelta = shift;

  $timeDelta =~ /\d+/ or return 'unknown';

  my $hours = sprintf("%02d", int($timeDelta / 3600)); $timeDelta = $timeDelta % 3600;
  my $minutes = sprintf("%02d", int($timeDelta / 60)); $timeDelta = $timeDelta % 60;
  my $seconds = sprintf("%02d", $timeDelta);

  return "$hours:$minutes:$seconds";

} 

sub ReadableSize {
  my $size = shift;

  if(int($size / (1024 * 1024 * 1024))) # gigabytes
  {
    return sprintf("%.2f GB", $size / (1024 * 1024 * 1024));
  }
  elsif(int($size / (1024 * 1024))) # megabytes
  {
    return sprintf("%.2f MB", $size / (1024 * 1024));
  }
  elsif(int($size / 1024)) # kilobytes
  {
    return sprintf("%.2f KB", $size / 1024);
  }
  else # bytes
  {
    return sprintf("%d B", $size);
  }
}
