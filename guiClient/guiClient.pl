use strict;

use Win32::GUI;
use Win32 ();
use Win32::GUI::Loft::Design;

use File::Spec;
use File::Path;
use Getopt::Long;
use XML::Simple;
use Time::HiRes qw(usleep);
use Sys::Hostname;
use Socket;

use constant UPDATE_INTERVAL => 20;

my $mainWindowDef;

# this locates the BitFlood/ directory and adds it to our include path
BEGIN {
  my ($volume, $directories) = File::Spec->splitpath($0);
  my @dirs = File::Spec->splitdir($directories);
  pop(@dirs) while(@dirs && !-e File::Spec->catfile($volume, @dirs, 'BitFlood'));
  unshift(@INC, File::Spec->catfile($volume, @dirs));
  $mainWindowDef = File::Spec->catfile($volume, @dirs, 'guiClient', 'guiClient.gld');

#  foreach my $incDir (@INC) {
#    if(-e File::Spec->catfile($incDir, 'guiClient.gld')) {
#      $mainWindowDef = File::Spec->catfile($incDir, 'guiClient.gld');
#    }
#  }
}

use BitFlood::Client;
use BitFlood::Debug;

###########################
## floods_ --> Floods tab
## config_ --> Config tab

# debugging channels...
my %channelNames;
$channelNames{'Default'} = '';
$channelNames{'Network'} = 'net';
$channelNames{'Performance'} = 'perf';

my $configFile = File::Spec->catfile($ENV{HOMEDRIVE}, $ENV{HOMEPATH}, '.bitflood', 'gui.cfg');
my $config;
if(!-e $configFile) {
  print "creating config file: $configFile\n";
  mkpath(File::Spec->catfile($ENV{HOMEDRIVE}, $ENV{HOMEPATH}, '.bitflood'));
  $config = {
	     localIp => inet_ntoa(scalar gethostbyname(hostname())),
	     port    => 10101,
	     debugging => {
			   Default => {
				       enabled => 0,
				       level => 1,
				       logtofile => 0,
				       filename => 'debug.log',
				       remote => 0,
				      },
			   Network => {
				       enabled => 0,
				       level => 1,
				       logtofile => 0,
				       filename => 'network.log',
				       remote => 0,
				      },
			   Performance => {
					   enabled => 0,
					   level => 1,
					   logtofile => 0,
					   filename => 'network.log',
					   remote => 0,
					  },
			  },
	    };
  my $configXML = XMLout($config);
  open(CONFIG, ">$configFile")
    or die("Could not open config file...");
  print CONFIG $configXML;
  close(CONFIG);
} else {
  print "loading config from: $configFile\n";
  $config = XMLin($configFile, ForceArray => 1);
  $config or die("Could not load config file!");
}

my $objDesign = Win32::GUI::Loft::Design->newLoad($mainWindowDef) or
  die("Could not open window file ($mainWindowDef)");

my $mainWindow = $objDesign->buildWindow() or die("Could not build window ($mainWindowDef)");

my $whiteBrush = new Win32::GUI::Brush(
				       [ 255, 255, 255 ]
				       );
my $pen = new Win32::GUI::Pen(
			      -color => [ 150, 150, 150 ],
			      -width => 1,
			      -style => 5,
			      );

my $client = BitFlood::Client->new(
				   localIp => $config->{localIp},
				   port    => $config->{port},
				  );

config_Populate();
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
  
  usleep(1000);
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
  my $foundFile = GUI::GetOpenFileName(
				       -owner => $mainWindow,
				       -title  => "Open",
				       -directory => File::Spec->rel2abs('.'),
				       -filter => [
						   "Flood files (*.flood)" => "*.flood",
						   "All files (*.*)", "*.*",
						   ],
				       );

  if(-e $foundFile) {
    my $localPath = GUI::BrowseForFolder(
					 -owner => $mainWindow,
					 -title  => "Save to...",
					 -directory => File::Spec->rel2abs('.'),
					 -editbox => 1,
					 );
    
    if(-d $localPath)
    {
      $client->AddFloodFile(File::Spec->abs2rel($foundFile), $localPath);
    }
    else
    {
      Debug("The path specified ($localPath) does not exist!");
    }
  }
}

sub floods_StopButton_Click {
  my ($selected) = $mainWindow->floods_FloodsListView->SelectedItems();
  my @floods = values(%{$client->floods});
  my $flood = $floods[$selected];

  $flood or return;
  $flood->paused(1);
}

sub floods_StartButton_Click {
  my ($selected) = $mainWindow->floods_FloodsListView->SelectedItems();
  my @floods = values(%{$client->floods});
  my $flood = $floods[$selected];

  $flood or return;
  $flood->paused(0);
  $flood->sessionStartTime(time());
  $flood->sessionDownloadBytes(0);
}

sub floods_RemoveButton_Click {
  my ($selected) = $mainWindow->floods_FloodsListView->SelectedItems();
  my @floods = keys(%{$client->floods});
  my $flood = $floods[$selected];
  
  delete $client->floods->{$flood};
}

sub config_ApplyButton_Click {
  my $externalIP = $mainWindow->config_ExternalIPEntry->Text();
  my $port = $mainWindow->config_PortEntry->Text();
  
  foreach my $debugChannel (keys(%channelNames)) {
    my $enabledCheckbox = "config_${debugChannel}LoggingChannelEnabledCheckbox";
    my $levelEntry = "config_${debugChannel}LoggingChannelLevelEntry";
    my $logToFileCheckbox = "config_${debugChannel}LoggingChannelToFileCheckbox";
    my $logFilenameEntry = "config_${debugChannel}LoggingChannelFilenameEntry";
    my $remoteLoggingCheckbox = "config_${debugChannel}LoggingChannelRemoteCheckbox";
      
    my $enabled = $mainWindow->$enabledCheckbox->GetCheck();
    my $level = $mainWindow->$levelEntry->Text();
    my $logToFile = $mainWindow->$logToFileCheckbox->GetCheck();
    my $logFilename = $mainWindow->$logFilenameEntry->Text();
    my $remoteLogging = $mainWindow->$remoteLoggingCheckbox->GetCheck();

    if($enabled) {
      $config->{debugging}->{$debugChannel}->{enabled} = 1;

      BitFlood::Debug::OpenChannel($channelNames{$debugChannel});
      
      $config->{debugging}->{$debugChannel}->{level} = $level;
      BitFlood::Debug::SetChannelLevel($channelNames{$debugChannel}, $level);

      if($logToFile) {
	if($config->{debugging}->{$debugChannel}->{logtofile} == 0) {
	  $config->{debugging}->{$debugChannel}->{logtofile} = 1;
	  $config->{debugging}->{$debugChannel}->{filename} = $logFilename;
	  
	  BitFlood::Debug::AddLogger($channelNames{$debugChannel}, 'File', $logFilename);
	  $mainWindow->$logToFileCheckbox->Checked(2);
	}
      }
	
      if($remoteLogging) {
	if($config->{debugging}->{$debugChannel}->{remote} == 0) {
	  $config->{debugging}->{$debugChannel}->{remote} = 1;
	  
	  BitFlood::Debug::AddLogger($channelNames{$debugChannel}, 'Jabber',
				     recipient  => 'bftest@jabber.org/listener',
				     serverHost => 'jabber.org',
				     serverPort => 5222,
				     username   => 'bftest',
				     password   => 'bftest',
				     resource   => hostname() . '_' . inet_ntoa(scalar gethostbyname(hostname())));
	  $mainWindow->$remoteLoggingCheckbox->Checked(2);
	}
      }

    } else {
      BitFlood::Debug::CloseChannel($channelNames{$debugChannel});
      $mainWindow->$logToFileCheckbox->Checked(0);
      $mainWindow->$remoteLoggingCheckbox->Checked(0);

      $config->{debugging}->{$debugChannel}->{enabled} = 0;
      $config->{debugging}->{$debugChannel}->{logtofile} = 0;
      $config->{debugging}->{$debugChannel}->{remote} = 0;      
    }
  }
  
  $config->{localIp} = $externalIP;
  $config->{port} = $port;
  
  open(CONFIG, ">$configFile");
  print CONFIG XMLout($config);
  close(CONFIG);
  
  $client->localIp($externalIP);
  $client->port($port);

  $client->OpenListenSocket();

}

sub config_RevertButton_Click {
  $mainWindow->config_ExternalIPEntry->SelectAll();
  $mainWindow->config_ExternalIPEntry->ReplaceSel($client->localIp);

  $mainWindow->config_PortEntry->SelectAll();
  $mainWindow->config_PortEntry->ReplaceSel($client->port);
}

sub UpdateDisplay {

  my $currentTab = $mainWindow->mainTabStrip->GetString($mainWindow->mainTabStrip->SelectedItem());

  if($currentTab eq 'Floods') {
    my ($selected) = $mainWindow->floods_FloodsListView->SelectedItems();
    $mainWindow->floods_FloodsListView->Clear();
    
    my $floodIndex = 0;
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
	    }
	  }
	  
	  $seeds++ if $allFilesComplete;
	}
      }

      my $complete = $flood->downloadBytes == $flood->totalBytes;
      
      my $filename = $flood->filename;
      my $totalSize = ReadableSize($flood->totalBytes);
      my $downloadedBytes = ReadableSize($flood->downloadBytes);
      my $percentComplete = $flood->totalBytes > 0 ? sprintf("%3.2f%%", 100 * $flood->downloadBytes / $flood->totalBytes) : '0%';

      my $time;
      my $etc;
      my $status;
      my $speed;
      
      if($complete)
      {
	$time = 'done';
	$etc = 'done';
	$status = 'complete';
	$speed = 'n/a';
      }
      elsif($flood->paused)
      {
	$time = 'paused';
	$etc = 'n/a';
	$status = 'paused';
	$speed = '0.0 K/s';
      }
      else
      {
	$time = ReadableTimeDelta(time() - $flood->startTime);
	$etc = (time() - $flood->sessionStartTime > 0 and $flood->sessionDownloadBytes > 0) ? ReadableTimeDelta(($flood->totalBytes - $flood->downloadBytes) / ($flood->sessionDownloadBytes / (time() - $flood->sessionStartTime))) : 'unknown';
	$status = 'incomplete';
	$speed = time() - $flood->sessionStartTime > 0 ? sprintf("%s/s", ReadableSize($flood->sessionDownloadBytes / (time() - $flood->sessionStartTime))) : '0.0 K/s';
      }

      $mainWindow->floods_FloodsListView->InsertItem(-text => [
							       $filename,
							       $totalSize,
							       $downloadedBytes,
							       $percentComplete,
							       $time,
							       $etc,
							       $speed,
							       $seeds, # FIXME: find complete peers
							       $peers - $seeds, # FIXME: find incomplete peers
							       'n/a',
							       $status,
							       ]);
      
      if($floodIndex == $selected) {
	$mainWindow->floods_FloodsListView->Select($selected);
	
	$mainWindow->floods_FloodFilenameDisplay->SelectAll();
	$mainWindow->floods_FloodFilenameDisplay->ReplaceSel($flood->filename);
	
	$mainWindow->floods_FloodHashDisplay->SelectAll();
	$mainWindow->floods_FloodHashDisplay->ReplaceSel($flood->contentHash);
	
	$mainWindow->floods_TrackersDisplay->SelectAll();
	$mainWindow->floods_TrackersDisplay->ReplaceSel(join(',', @{$flood->data->{Tracker}}));
	
	$mainWindow->floods_PercentCompleteDisplay->SelectAll();
	$mainWindow->floods_PercentCompleteDisplay->ReplaceSel($flood->totalBytes > 0 ? sprintf("%3.2f%%", 100 * $flood->downloadBytes / $flood->totalBytes) : '0%');
	
	$mainWindow->floods_TransferTimeDisplay->SelectAll();
	$mainWindow->floods_TransferTimeDisplay->ReplaceSel(!$complete ? ReadableTimeDelta(time() - $flood->startTime) : 'done');
	
	$mainWindow->floods_DownloadRateDisplay->SelectAll();
	$mainWindow->floods_DownloadRateDisplay->ReplaceSel((!$complete and time() - $flood->startTime > 0) ? sprintf("%s/s", ReadableSize($flood->downloadBytes / (time() - $flood->startTime))) : '0.0 K/s');
	
	$mainWindow->floods_ETCDisplay->SelectAll();
	$mainWindow->floods_ETCDisplay->ReplaceSel((!$complete and ($flood->downloadBytes > 0) and (time() - $flood->startTime > 0)) ? ReadableTimeDelta(($flood->totalBytes - $flood->downloadBytes) / ($flood->downloadBytes / (time() - $flood->startTime))) : $complete ? 'done' : 'unknown');
	
	$mainWindow->floods_SeedsDisplay->SelectAll();
	$mainWindow->floods_SeedsDisplay->ReplaceSel($seeds);
	
	$mainWindow->floods_PeersDisplay->SelectAll();
	$mainWindow->floods_PeersDisplay->ReplaceSel($peers - $seeds);
	
	$mainWindow->floods_PriorityDisplay->SelectAll();
	$mainWindow->floods_PriorityDisplay->ReplaceSel('n/a');
	
	$mainWindow->floods_StatusDisplay->SelectAll();
	$mainWindow->floods_StatusDisplay->ReplaceSel($complete ? 'complete' : $flood->paused ? 'paused' : 'incomplete');
	
	$mainWindow->floods_ProgressDisplay->SetPos(int(100 * $flood->downloadBytes / $flood->totalBytes));
	
	floods_ChunksDisplay_Paint();
      }
      
      $floodIndex++;
    }
  }
  elsif($currentTab eq 'Config')
  {
    # update config tab...
  }
}

sub floods_ChunksDisplay_Paint {
  my $DC = $mainWindow->floods_ChunksDisplay->GetDC();
  my $width = $mainWindow->floods_ChunksDisplay->ScaleWidth(); #$right - $left;
  my $height = $mainWindow->floods_ChunksDisplay->ScaleHeight();

  $DC->SelectObject($pen);
  $DC->SelectObject($whiteBrush);
  $DC->Rectangle(0, 0, $width, $height);

  my ($selected) = $mainWindow->floods_FloodsListView->SelectedItems();
  if(!defined($selected)) {
    $mainWindow->floods_ChunksDisplay->Show();
    $DC->Validate();
    return;
  }

  my @floods = values(%{$client->floods});
  my $flood = $floods[$selected];

  my @fileChunkMaps;
  my $numChunks = 0;
  foreach my $filename (sort(keys(%{$flood->Files}))) {
    my $file = $flood->Files->{$filename};
    $numChunks += $file->{chunkMap}->Size();
    push(@fileChunkMaps, $file->{chunkMap});
  }
  my $numFiles = scalar(@fileChunkMaps);
  
  my $blockSize = 10;

  if(($width * $height) < $blockSize**2 * $numChunks) {
    my $ratio = $blockSize**2 * $numChunks / ($width * $height);
    my $factor = sqrt($ratio);
    $blockSize /= $factor;
  }

  my $chunksPerRow = int($width / $blockSize);

#  print "w: $width h: $height nb: $numChunks bs: $blockSize r: $rows cpr: $chunksPerRow\n";

  my $fileIndex = 0;
  my $blockIndex = 0;
  foreach my $fileChunkMap (@fileChunkMaps) {

    my $r = $fileIndex % 2 == 0 ? (255 - ($fileIndex * 4)) : 180; #(255 - $fileIndex * 2);
    my $g = $fileIndex % 3 == 0 ? (255 - ($fileIndex * 4)) : 180; #(255 - $fileIndex * 3);
    my $b = $fileIndex % 4 == 0 ? (255 - ($fileIndex * 4)) : 180; #(255 - $fileIndex * 4);

    my $haveItBrush = new Win32::GUI::Brush(
					    [ $r, $g, $b ]
					    );
    my $dontHaveItBrush = new Win32::GUI::Brush(
						[ $r*.75, $g*.75, $b*.75 ]
						);
    
    
    for(my $chunkIndex = 0; $chunkIndex < $fileChunkMap->Size(); $chunkIndex++) {
      
      my $top = $blockSize * int($blockIndex / $chunksPerRow);
      my $left = $blockSize * int($blockIndex % $chunksPerRow);
      my $right = $left + $blockSize+1;
      my $bottom = $top + $blockSize+1;

      $DC->SelectObject($dontHaveItBrush);
      $DC->Rectangle($left, $top, $right, $bottom);
      if($fileChunkMap->bit_test($chunkIndex)) {
	$DC->SelectObject($haveItBrush);
	$DC->Rectangle($left + ($blockSize * .2), $top + ($blockSize * .2), $right - ($blockSize * .2), $bottom - ($blockSize * .2));
      }
      $blockIndex++;
    }
    $fileIndex++;
  }
  
  $mainWindow->floods_ChunksDisplay->Show();
  $DC->Validate();
#      $mainWindow->floods_ChunksDisplay->Update();
  return 1;
}

sub config_Populate {
  $mainWindow->config_ExternalIPEntry->SelectAll();
  $mainWindow->config_ExternalIPEntry->ReplaceSel($config->{localIp});

  $mainWindow->config_PortEntry->SelectAll();
  $mainWindow->config_PortEntry->ReplaceSel($config->{port});

  foreach my $debugChannel (keys(%channelNames)) {
    my $enabledCheckbox = "config_${debugChannel}LoggingChannelEnabledCheckbox";
    my $levelEntry = "config_${debugChannel}LoggingChannelLevelEntry";
    my $logToFileCheckbox = "config_${debugChannel}LoggingChannelToFileCheckbox";
    my $logFilenameEntry = "config_${debugChannel}LoggingChannelFilenameEntry";
    my $remoteLoggingCheckbox = "config_${debugChannel}LoggingChannelRemoteCheckbox";
      
    if($config->{debugging}->{$debugChannel}->{enabled}) {
      $mainWindow->$enabledCheckbox->Checked(1);

      if(BitFlood::Debug::OpenChannel($channelNames{$debugChannel}) or $debugChannel eq 'Default') {
	$mainWindow->$levelEntry->SelectAll();
	$mainWindow->$levelEntry->ReplaceSel($config->{debugging}->{$debugChannel}->{level});
	BitFlood::Debug::SetChannelLevel($channelNames{$debugChannel}, $config->{debugging}->{$debugChannel}->{level});

	if($config->{debugging}->{$debugChannel}->{logtofile}) {
	  $mainWindow->$logFilenameEntry->SelectAll();
	  $mainWindow->$logFilenameEntry->ReplaceSel($config->{debugging}->{$debugChannel}->{filename});
	  
	  BitFlood::Debug::AddLogger($channelNames{$debugChannel}, 'File', $config->{debugging}->{$debugChannel}->{filename});
	  $mainWindow->$logToFileCheckbox->Checked(1);
	}
	
	if($config->{debugging}->{$debugChannel}->{remote}) {
	  $mainWindow->$remoteLoggingCheckbox->Checked(1);
	  
	  BitFlood::Debug::AddLogger($channelNames{$debugChannel}, 'Jabber',
				     recipient  => 'bftest@jabber.org/listener',
				     serverHost => 'jabber.org',
				     serverPort => 5222,
				     username   => 'bftest',
				     password   => 'bftest',
				     resource   => hostname() . '_' . inet_ntoa(scalar gethostbyname(hostname())));
	  $mainWindow->$remoteLoggingCheckbox->Checked(1);
	}
      }
    }
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
