package BitFlood::Peer;

use strict;

use base qw(BitFlood::Accessor);

use RPC::XML;
use RPC::XML::Parser;
use IO::Socket;
use IO::Select;
use Errno qw(:POSIX);
use Fcntl qw(:flock);
use Bit::Vector;
use Digest::SHA1 qw(sha1_base64);
use Time::HiRes qw(time);

use Data::Dumper; # XXX

use BitFlood::Utils;
use BitFlood::Debug;
use BitFlood::Logger;
use BitFlood::Logger::Multi;
use BitFlood::Logger::File;
use BitFlood::Net::BufferedReader;
use BitFlood::Net::BufferedWriter;


use constant CONNECTION_TIMEOUT => 10; # seconds


__PACKAGE__->mk_accessors(qw(
                             id
                             host
                             port
                             floods
                             chunkMaps
                             registered
			     socket
                             connectStartTime
                             disconnected
                             connectCompleted
                             client
                             readBuffer writeBuffer
                             currentReadBufferPosition
                             bufferedReader bufferedWriter
			     lastReadTime lastWriteTime
                             readSpeed writeSpeed
			     targetFilehandles
                            )
                         );



sub new {
  my $class = shift;

  Debug('>>>', 'trace');
  Debug("Creating new Peer", 5);
  my $self = $class->SUPER::new(@_);

  $self->client or die("Client object not specified");

  $self->readBuffer('');
  $self->writeBuffer('');

  $self->floods({});
  $self->chunkMaps({});
  $self->registered({});
  $self->targetFilehandles({});

  $self->port(10101) if(!defined($self->port));

  if ($self->socket) {
    Debug("Incoming peer connection: " . $self->host . ":" . $self->port, 'net', 10);

    # FIXME: windows assness
    if($^O eq 'MSWin32') {
      $self->socket->setsockopt(SOL_SOCKET, SO_DONTLINGER, 1);
      my $temp=1; $self->socket->ioctl(0x8004667E, \$temp); # FIXME: constant is 'FIONBIO' (define it elsewhere?)
    } else {
      defined($self->socket->blocking(0)) or die("non-blocking not supported!");
    }

    $self->currentReadBufferPosition(0);
    $self->bufferedReader(BitFlood::Net::BufferedReader->new({buffer => \$self->{readBuffer}, socket => $self->socket}));
    $self->bufferedWriter(BitFlood::Net::BufferedWriter->new({buffer => \$self->{writeBuffer}, socket => $self->socket}));

    $self->connectCompleted(1);
  }

  Debug('<<<', 'trace');
  return $self;
}

sub SendMessage {
  my $self = shift;
  my $methodName = shift;
  my $flood = shift;
  my @methodArgs = @_;

  Debug('>>>', 'trace');

  unshift(@methodArgs, $flood->contentHash);
  Debug("$methodName (" . scalar(@methodArgs) . " args) -> " . $self->host . ':' . $self->port, 'net', 20);
  my $request = RPC::XML::request->new($methodName, @methodArgs);
  $self->writeBuffer($self->writeBuffer . $request->as_string . "\n");

  Debug('<<<', 'trace');
  return 1;
}

sub Connect {
  my $self = shift;

  Debug('>>>', 'trace');

  if ($self->connectCompleted) {
    Debug('<<<', 'trace');
    return 1;
  }

  if (!$self->socket) {

    Debug("connecting out to " . $self->host . ":" . $self->port . " (" . $self->id . ")", 'net', 10);

    $self->socket(IO::Socket::INET->new(
					Proto    => 'tcp',
					PeerAddr => $self->host,
					PeerPort => $self->port,
					Blocking => 0,
				       ));
    if (!$self->socket) {
      Debug("Socket creation failed ($!)", 'net');
      $self->disconnected(1);
      Debug('<<<', 'trace');
      return undef;
    }

    $self->connectStartTime(time());

  }

  my $select = IO::Select->new($self->socket);
  my $connected = $select->can_write(0);
  Debug("checking for completed connection to " . $self->host . ":" . $self->port . "... " . ($connected ? 'yes' : 'no') ." [error: $! (" . ($!+0) . ")]", 'net', 30);

  if ($connected) {

    $self->connectCompleted(1);
    Debug("socket connected to " . $self->host . ":" . $self->port, 'net', 10);

    $self->bufferedReader(BitFlood::Net::BufferedReader->new({buffer => \$self->{readBuffer}, socket => $self->socket}));
    $self->bufferedWriter(BitFlood::Net::BufferedWriter->new({buffer => \$self->{writeBuffer}, socket => $self->socket}));

    Debug('<<<', 'trace');
    return 1;
    
  } else {

    if ($!{EINPROGRESS} or $!{EWOULDBLOCK}) {
      # non-blocking connection is still trying to connect
      if (time > $self->connectStartTime + CONNECTION_TIMEOUT) {
	Debug("connection timed out: " . $self->host . ':' . $self->port, 'net', 10);
	$self->disconnected(1);
      }
    } elsif($!) {
      # some other error, signaling the connect actually failed
      Debug("failed to connect (" . $self->host . ':' . $self->port . "): $!", 'net', 10);
      $self->disconnected(1);
    }

    Debug('<<<', 'trace');
    return undef;

  }

}

sub GetChunk {
  my $self = shift;
  my $flood = shift;
  my $file = shift;
  my $chunk = shift;

  Debug('>>>', 'trace');
  $self->SendMessage(
		     'RequestChunk',
		     $flood,
		     $file->{name},
		     $chunk->{index},
		    );

  $chunk->{downloading} = time();
  $flood->{totalDownloading}++;
  $file->{downloadBeginTime} ||= time();
  $file->{downloadBytes} ||= 0;

  Debug('<<<', 'trace');
}


sub HandleRegister {
  my $self = shift;
  my $flood = shift;
  my $peerId = shift;

  Debug('>>>', 'trace');

  if (!length($peerId)) {
    Debug("invalid peer ID", 'net');
    $self->disconnected(1);
    Debug('<<<', 'trace');
    return;
  }

  if (1 < grep { $_->id eq $peerId} @{$self->client->peers}) {
    Debug("disconnecting duplicate peer", 'net');
    $self->disconnected(1);
    Debug('<<<', 'trace');
    return;
  }

  if ($self->floods->{$flood->contentHash}) {
    Debug("duplicate Register message from $peerId for " . $flood->contentHash, 'net');
    Debug('<<<', 'trace');
    return;
  }

  $self->id($peerId);
  $self->floods->{$flood->contentHash} = $flood;

  $self->SendMessage('RequestChunkMaps', $flood);
  $self->registered->{$flood->contentHash} = 1;

  Debug('<<<', 'trace');
  return 1;
}


sub HandleRequestChunkMaps {
  my $self = shift;
  my $flood = shift;

  Debug('>>>', 'trace');
  my $chunkMaps = [
		   map
		     { $_->{name} => $_->{chunkMap}->to_Bin() }
		     values %{$flood->Files}
		  ];

  $self->SendMessage('SendChunkMaps', $flood, $chunkMaps);
  Debug('<<<', 'trace');
}

sub HandleSendChunkMaps {
  my $self = shift;
  my $flood = shift;
  my $chunkMapsArrayRef = shift;
  my %chunkMaps = @{$chunkMapsArrayRef};
  
  Debug('>>>', 'trace');
  foreach my $chunkMap (values %chunkMaps) {
    $chunkMap = Bit::Vector->new_Bin(length($chunkMap), $chunkMap);
  }
  $self->chunkMaps->{$flood->contentHash} = \%chunkMaps;

  Debug('<<<', 'trace');
}

sub HandleRequestChunk {
  my $self = shift;
  my $flood = shift;
  my $filename = shift;
  my $index = shift;

  Debug('>>>', 10);
  my $file = $flood->Files->{$filename};
  if (!$file->{chunkMap}->bit_test($index)) {
    Debug("don't have chunk: ".substr($filename,-20)."#$index", 'net');
    Debug('<<<', 'trace');    
    return;
  }

  my $chunkSourceFilename = $file->{localFilename};
  # FIXME check this at flood object creation, not here
  if (!defined $chunkSourceFilename) {
    die("don't know local filename for: $filename");
  }

  my $chunk = $file->{Chunk}[$index];
  if (!$chunk) {
    Debug("out-of-range chunk index: $filename#$index", 'net');
    Debug('<<<', 'trace');
    return;
  }

  printf("%-30.30s#%d => %-21.21s\r",
	 substr($file->{name}, -30), $chunk->{index}+1,
	 $self->host . ':' . $self->port,
	 );

  my $chunkData;
  my $chunkSourceFile = IO::File->new($chunkSourceFilename, 'r');
  $chunkSourceFile->seek($chunk->{offset}, 0);
  $chunkSourceFile->sysread($chunkData, $chunk->{size});
  $chunkSourceFile->close();

  $self->SendMessage(
		     'SendChunk',
		     $flood,
		     $filename,
		     $index,
		     RPC::XML::base64->new($chunkData)
		    );
  Debug('<<<', 'trace');
}

sub HandleSendChunk {
  my $self = shift;
  my $flood = shift;
  my $filename = shift;
  my $index = shift;
  my $chunkData = shift;

  Debug('>>>', 'trace');

  my $file = $flood->Files->{$filename};
  if (!$file) {
    Debug("unknown file: $filename", 'net');
    Debug('<<<', 'trace');
    return;
  }

  my $chunk = $file->{Chunk}->[$index];
  if (!$chunk) {
    Debug("out-of-range chunk index: $filename#$index", 'net');
    Debug('<<<', 'trace');
    return;
  }

  Debug("transfer speed snapshot: ". $self->host . ":" . $self->port ." read=" . ReadableSize($self->readSpeed) . "/s write=" . ReadableSize($self->writeSpeed) . "/s", 'net', 20);

  printf("%-30.30s#%d <= %-21.21s [%7.2f K/s]  \r",
	 substr($file->{name}, -30),
	 $chunk->{index}+1,
	 $self->host,
	 ($flood->sessionDownloadBytes / 1024) / ((time() - $flood->sessionStartTime) + 1),
	);

  if(sha1_base64($chunkData) eq $chunk->{hash}) {

    if(!defined($self->targetFilehandles->{$file->{localFilename}})) {
      $self->targetFilehandles->{$file->{localFilename}} = IO::File->new($file->{localFilename}, 'r+');
#      $self->targetFilehandles->{$file->{localFilename}}->flock(LOCK_SH);
#      flock($self->targetFilehandles->{$file->{localFilename}}, LOCK_SH);
      $self->targetFilehandles->{$file->{localFilename}}->autoflush(1);
    }
    my $targetFile = $self->targetFilehandles->{$file->{localFilename}};
    defined($targetFile) or die("Could not open file for write: $file->{localFilename}");
    flock($targetFile, LOCK_EX);
    $targetFile->seek($chunk->{offset}, 0);
    $targetFile->syswrite($chunkData);
    flock($targetFile, LOCK_UN);
#    $targetFile->close();

    $file->{chunkMap}->Bit_On($index);
    $file->{downloadBytes} += $chunk->{size};
    $flood->downloadBytes($flood->downloadBytes + $chunk->{size});
    $flood->sessionDownloadBytes($flood->sessionDownloadBytes + $chunk->{size});

    foreach my $peer (grep { exists($_->floods->{$flood->contentHash}) } @{$self->client->peers}) {
      $peer->SendMessage(
			 'NotifyHaveChunk',
			 $flood,
			 $file->{name},
			 $chunk->{index}
			);
    }
 
    # pull the chunk we got out of the list of the ones we need
    for(my $index = 0; $index < @{$flood->neededChunksByWeight}; $index++) {
      my $chunkToRemove = $flood->neededChunksByWeight->[$index];
      if($chunkToRemove->{filename} eq $file->{name}
	 and $chunkToRemove->{chunk}->{index} eq $chunk->{index}) {
	splice(@{$flood->neededChunksByWeight}, $index, 1);
	last;
      }
    }

  } else {
    Debug("bad chunk data", 'net');
  }
  
  delete $chunk->{downloading};
  $flood->{totalDownloading}--;
  
  if ($file->{chunkMap}->is_full) {
    $self->targetFilehandles->{$file->{localFilename}}->close();
    delete $self->targetFilehandles->{$file->{localFilename}};
    print ' ' x 76 . "\r";
    printf("Completed: %s [%.2f KB/s]\n",
	   $file->{name},
	   $file->{downloadBytes} / 1024 / (time() - $file->{downloadBeginTime} + 1));
  }
  
  Debug('<<<', 'trace');
}

sub HandleNotifyHaveChunk {
  my $self = shift;
  my $flood = shift;
  my $filename = shift;
  my $index = shift;

  Debug('>>>', 'trace');
  my $file = $flood->Files->{$filename};
  if (!$file) {
    Debug("unrecognized filename". 'net');
    Debug('<<<', 'trace');
    return;
  }
  Debug("peer " . $self->id ." (".$self->host.":".$self->port.") has ".substr($filename,-20)."#".$index, 'net', 40);
  if(defined($self->chunkMaps->{$flood->contentHash}{$filename})) {
    $self->chunkMaps->{$flood->contentHash}{$filename}->Bit_On($index);
    Debug("chunkmap now: " . $self->chunkMaps->{$flood->contentHash}{$filename}->to_ASCII, 10);
  } else {
    Debug("notified about a chunk before we have chunkmaps", 'net');
  }

  Debug('<<<', 'trace');
}

sub DispatchRequests {
  my $self = shift;
  my $requestXml = shift;

  Debug('>>>', 'trace');

  my $request = RPC::XML::Parser->new()->parse($requestXml);
  if (!ref($request)) {
    Debug("invalid XML: $requestXml", 'net');
    Debug('<<<', 'trace');
    return;
  }

  my $methodName = 'Handle' . $request->{name};
  if (my $methodRef = $self->can($methodName)) {
    Debug($self->host . ":" . $self->port . " -> $methodName", 'net', 20);
    my @requestArgs = map { $_->value } @{$request->{args}};
    my $floodHash = shift(@requestArgs);
    my $flood;
    if ($request->{name} eq 'Register') { # NOTE special case!
      $flood = $self->client->floods->{$floodHash};
    } else {
      $flood = $self->floods->{$floodHash};
    }
    if (!$flood) {
      Debug("unknown flood hash: $floodHash", 'net');
      if($request->{name} eq 'Register') { # NOTE special case!
	if(!%{$self->floods}) {
	  Debug("  disconnecting peer (no valid floods)", 'net');
	  $self->disconnected(1);
	}
      }
      Debug('<<<', 'trace');
      return;
    }
    $methodRef->($self, $flood, @requestArgs);
  } else {
    Debug("Unsupported RPC request method: $request->{name}", 'net');
  }

  Debug('<<<', 'trace');
}


sub LoopOnce {
  my $self = shift;

  Debug('>>>', 'trace');

  if (!$self->Connect) {
    Debug('<<<', 'trace');
    return;
  }

  $self->ReadOnce;
  $self->WriteOnce;

  $self->ProcessReadBuffer;

  Debug('<<<', 'trace');
}


sub ReadOnce {
  my $self = shift;

  Debug('>>>', 'trace');

  if(!defined($self->bufferedReader)) {
    Debug("No buffered reader object");
    Debug('<<<', 'trace');
    return;
  }

  if ($self->disconnected) {
    Debug("previously disconnected", 'net', 10);
    Debug('<<<', 'trace');
    return;
  }

  Debug("readBuffer address before read: " . \$self->readBuffer, 50);
  my $bytesRead = $self->bufferedReader->Read();
  Debug("readBuffer address after read: " . \$self->readBuffer, 50);

  if ($bytesRead == -1) {
    Debug("would block", 'net', 30);
    Debug('<<<', 'trace');
    return;
  } elsif($bytesRead == 0) {
    Debug("read error, disconnecting peer: " . $self->id, 'net');
    $self->disconnected(1);
    Debug('<<<', 'trace');
    return;
  }

  $self->CalculateReadSpeed($bytesRead);
  $self->lastReadTime(time());

  Debug('<<<', 'trace');
}

sub WriteOnce {
  my $self = shift;

  Debug('>>>', 'trace');

  if(!length($self->writeBuffer)) {
    Debug("nothing to write", 50);
    Debug('<<<', 'trace');
    return;
  }

  if(!defined($self->bufferedWriter)) {
    Debug("No buffered writer");
    Debug('<<<', 'trace');
    return;
  }

  if ($self->disconnected) {
    Debug("previously disconnected", 'net', 10);
    Debug('<<<', 'trace');
    return;
  }

  my $bytesWritten = $self->bufferedWriter->Write();

  if ($bytesWritten == -1) {
    Debug("would block", 'net', 30);
    Debug('<<<', 'trace');
    return;
  } elsif($bytesWritten == 0) {
    Debug("write error, disconnecting peer: " . $self->id, 'net');
    $self->disconnected(1);
    Debug('<<<', 'trace');
    return;
  }

  $self->CalculateWriteSpeed($bytesWritten);
  $self->lastWriteTime(time());

  Debug('<<<', 10);
}

sub ProcessReadBuffer {
  my $self = shift;

  Debug('>>>', 'trace');

  my $currentMessage;
  my $currentMessageTail;
  my $remainder;

  my $tailLength;
  do {
    ($currentMessageTail) = (substr($self->readBuffer, $self->currentReadBufferPosition) =~ /(.*?\n)/);
    if($tailLength = length($currentMessageTail))
    {
      Debug("end of message detected", 30);
      Debug("  tailLength: $tailLength", 30);
      $currentMessage = substr($self->{readBuffer}, 0, $self->currentReadBufferPosition + $tailLength, '');
      $self->DispatchRequests($currentMessage);
      $self->currentReadBufferPosition(0);
    }
    else
    {
      $self->currentReadBufferPosition(length($self->readBuffer));
    }
    Debug("  remainder: " . $self->readBuffer, 50);
  } while ($tailLength);

  Debug('<<<', 10);
}


sub CalculateReadSpeed {
  my $self = shift;
  my $bytesRead = shift;

  Debug('>>>', 'trace');

  return if !defined $self->lastReadTime;

  my $timeDelta = time() - $self->lastReadTime;
  if ($timeDelta) {
    $self->readSpeed($bytesRead / $timeDelta);
  } else {
    $self->readSpeed(undef);
  }
  Debug("read speed: " . ReadableSize($self->readSpeed) . "/s", 'net', 40);
  Debug('<<<', 'trace');
}

sub CalculateWriteSpeed {
  my $self = shift;
  my $bytesWritten = shift;

  Debug('>>>', 'trace');

  if(!defined $self->lastWriteTime) {
    Debug('<<<', 'trace');
    return;
  }

  my $timeDelta = time() - $self->lastWriteTime;
  if ($timeDelta) {
    $self->writeSpeed($bytesWritten / $timeDelta);
  } else {
    $self->writeSpeed(undef);
  }
  Debug("write speed: " . ReadableSize($self->writeSpeed) . "/s", 'net', 40);
  Debug('<<<', 'trace');
}

sub disconnected {
  my $self = shift;

  Debug('>>>', 'trace');

  if (@_ and $_[0]) {
    if ($self->socket) {
      $self->socket->close;
      $self->socket(undef);
    }
  }
  $self->_disconnected_accessor(@_);
  Debug('<<<', 'trace');
}

sub DESTROY {
  my $self = shift;

  Debug('>>>', 'trace');
  foreach my $filehandle (values(%{$self->targetFilehandles})) {
    $filehandle->close();
  }
  Debug('<<<', 'trace');
}


1;
