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
                             readBuffer
                             writeBuffer
                             currentReadBufferPosition
                             bufferedReader
                             bufferedWriter
			     targetFilehandles
                            )
                         );



sub new {
  my $class = shift;

  Debug("Creating new Peer...", 5);
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
    Debug("connected from: " . $self->id . "(" . $self->host . ":" . $self->port . ")");

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

  return $self;
}

sub SendMessage {
  my $self = shift;
  my $methodName = shift;
  my $flood = shift;
  my @methodArgs = @_;

  Debug(">>>", 10);

  unshift(@methodArgs, $flood->contentHash);
  Debug("method $methodName (" . scalar(@methodArgs) . " args) -> " . $self->host, 5);
  my $request = RPC::XML::request->new($methodName, @methodArgs);
  Debug("  queueing to: " . $self->id . " (" . $self->host . ":" . $self->port . ")", 15);
  $self->writeBuffer($self->writeBuffer . $request->as_string . "\n");
  Debug("  write buffer: " . $self->writeBuffer, 50);

  Debug("<<<", 10);
  return 1;
}

sub Connect {
  my $self = shift;

  Debug(">>>", 10);

  if ($self->connectCompleted) {
    Debug("<<<", 10);
    return 1;
  }

  if (!$self->socket) {

    Debug("connecting out: " . $self->id . "(" . $self->host . ":" . $self->port . ")");

    $self->socket(IO::Socket::INET->new(
					Proto    => 'tcp',
					PeerAddr => $self->host,
					PeerPort => $self->port,
					Blocking => 0,
				       ));
    if (!$self->socket) {
      Debug("socket creation failed");
      $self->disconnected(1);
      Debug("<<<", 10);
      return 0;
    }

    $self->connectStartTime(time());

  }

  my $select = IO::Select->new($self->socket);
  my $connected = $select->can_write(0);
  Debug("checking for completed connection (" . $self->host . "): $connected [error: $! (" . ($!+0) . ")]", 25);

  if ($connected) {

    $self->connectCompleted(1);
    Debug("socket connected");

    $self->bufferedReader(BitFlood::Net::BufferedReader->new({buffer => \$self->{readBuffer}, socket => $self->socket}));
    $self->bufferedWriter(BitFlood::Net::BufferedWriter->new({buffer => \$self->{writeBuffer}, socket => $self->socket}));

    Debug("<<<", 10);
    return 1;
    
  } else {

    if ($!{EINPROGRESS} or $!{EWOULDBLOCK}) {
      # non-blocking connection is still trying to connect
      if (time > $self->connectStartTime + CONNECTION_TIMEOUT) {
	Debug("connection timed out: " . $self->id);
	$self->disconnected(1);
      }
    } elsif($!) {
      # some other error, signaling the connect actually failed
      Debug("failed to connect: $!");
      $self->disconnected(1);
    }

    Debug("<<<", 10);
    return 0;

  }

}

sub GetChunk {
  my $self = shift;
  my $flood = shift;
  my $file = shift;
  my $chunk = shift;

  Debug(">>>", 10);
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

  Debug("<<<", 10);
}


sub HandleRegister {
  my $self = shift;
  my $flood = shift;
  my $peerId = shift;

  if (!length($peerId)) {
    Debug("invalid peer ID");
    $self->disconnected(1);
    return;
  }

  if (1 < grep { $_->id eq $peerId} @{$self->client->peers}) {
    Debug("disconnecting duplicate peer");
    $self->disconnected(1);
    return;
  }

  if ($self->floods->{$flood->contentHash}) {
    Debug("duplicate Register message from $peerId for " . $flood->contentHash);
    return;
  }

  $self->id($peerId);
  $self->floods->{$flood->contentHash} = $flood;

  $self->SendMessage('RequestChunkMaps', $flood);
  $self->registered->{$flood->contentHash} = 1;
}


sub HandleRequestChunkMaps {
  my $self = shift;
  my $flood = shift;

  Debug(">>>", 10);
  my $chunkMaps = [
		   map
		     { $_->{name} => $_->{chunkMap}->to_Bin() }
		     values %{$flood->Files}
		  ];

  $self->SendMessage('SendChunkMaps', $flood, $chunkMaps);
  Debug("<<<", 10);
}

sub HandleSendChunkMaps {
  my $self = shift;
  my $flood = shift;
  my $chunkMapsArrayRef = shift;
  my %chunkMaps = @{$chunkMapsArrayRef};
  
  Debug(">>>", 10);
  foreach my $chunkMap (values %chunkMaps) {
    $chunkMap = Bit::Vector->new_Bin(length($chunkMap), $chunkMap);
  }
  $self->chunkMaps->{$flood->contentHash} = \%chunkMaps;

  Debug("<<<", 10);
}

sub HandleRequestChunk {
  my $self = shift;
  my $flood = shift;
  my $filename = shift;
  my $index = shift;

  Debug(">>>", 10);
  my $file = $flood->Files->{$filename};
  if (!$file->{chunkMap}->bit_test($index)) {
    Debug("don't have chunk: ".substr($filename,-20)."#$index", 10);
    return;
  }

  my $chunkSourceFilename = $file->{localFilename};
  # FIXME check this at flood object creation, not here
  if (!defined $chunkSourceFilename) {
    die("don't know local filename for: $filename");
  }

  my $chunk = $file->{Chunk}[$index];
  if (!$chunk) {
    Debug("out-of-range chunk index: $filename#$index");
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
  Debug("<<<", 10);
}

sub HandleSendChunk {
  my $self = shift;
  my $flood = shift;
  my $filename = shift;
  my $index = shift;
  my $chunkData = shift;

  Debug(">>>", 10);

  my $file = $flood->Files->{$filename};
  if (!$file) {
    Debug("unknown file: $filename");
    return;
  }

  my $chunk = $file->{Chunk}->[$index];
  if (!$chunk) {
    Debug("out-of-range chunk index: $filename#$index");
    return;
  }
  
  printf("%-30.30s#%d <= %-21.21s [%7.2f K/s]  \r",
	 substr($file->{name}, -30),
	 $chunk->{index}+1,
	 $self->host,
	 ($flood->downloadBytes / 1024) / ((time() - $flood->startTime) + 1),
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
    Debug("bad chunk data");
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
  
  Debug("<<<", 10);
}

sub HandleNotifyHaveChunk {
  my $self = shift;
  my $flood = shift;
  my $filename = shift;
  my $index = shift;

  Debug(">>>", 10);
  my $file = $flood->Files->{$filename};
  if (!$file) {
    Debug("unrecognized filename");
    Debug("<<<", 10);
    return;
  }
  Debug($self->id ." (".$self->host.":".$self->port."): ".substr($filename,-20)."#".$index, 7);
  $self->chunkMaps->{$flood->contentHash}{$filename}->Bit_On($index);
  Debug("chunkmap now: " . $self->chunkMaps->{$flood->contentHash}{$filename}->to_ASCII, 7);

  Debug("<<<", 10);
}

sub DispatchRequests {
  my $self = shift;
  my $requestXml = shift;

  Debug(">>>", 10);

  my $request = RPC::XML::Parser->new()->parse($requestXml);
  if (!ref($request)) {
    Debug("invalid XML");
    Debug($requestXml, 5);
    Debug("<<<", 10);
    return;
  }

  my $methodName = 'Handle' . $request->{name};
  if (my $methodRef = $self->can($methodName)) {
    Debug("dispatching to $methodName", 5);
    my @requestArgs = map { $_->value } @{$request->{args}};
    my $floodHash = shift(@requestArgs);
    my $flood;
    if ($request->{name} eq 'Register') { # NOTE special case!
      $flood = $self->client->floods->{$floodHash};
    } else {
      $flood = $self->floods->{$floodHash};
    }
    if (!$flood) {
      Debug("unknown flood hash: $floodHash");
      if($request->{name} eq 'Register') { # NOTE special case!
	if(!%{$self->floods}) {
	  Debug("  disconnecting peer (no valid floods)");
	  $self->disconnected(1);
	}
      }
      Debug("<<<", 10);
      return;
    }
    $methodRef->($self, $flood, @requestArgs);
  } else {
    Debug("Invalid RPC request method: $request->{name}");
  }

  Debug("<<<", 10);
}


sub LoopOnce {
  my $self = shift;

  Debug(">>>", 10);

  if (!$self->Connect) {
    Debug("<<<", 10);
    return;
  }

  $self->ReadOnce;
  $self->WriteOnce;

  $self->ProcessReadBuffer;

  Debug("<<<", 10);
}


sub ReadOnce {
  my $self = shift;

  Debug(">>>", 10);

  if(!defined($self->bufferedReader)) {
    Debug("No buffered reader object...");
    return;
  }

  if ($self->disconnected) {
    Debug("previously disconnected");
    Debug("<<<", 10);
    return;
  }

  Debug("readBuffer address before read: " . \$self->readBuffer, 50);
  my $result = $self->bufferedReader->Read();
  Debug("readBuffer address after read: " . \$self->readBuffer, 50);

  if ($result == -1) {
    Debug("Would block...", 50);
    Debug("<<<", 10);
    return;
  } elsif($result == 0) {
    Debug("read error, disconnecting peer: " . $self->id);
    $self->disconnected(1);
    Debug("<<<", 10);
    return;
  }

  Debug("<<<", 10);
}

sub WriteOnce {
  my $self = shift;

  Debug(">>>", 10);

  if(!length($self->writeBuffer)) {
    Debug("nothing to write...", 50);
    Debug("<<<", 10);
    return;
  }

  if(!defined($self->bufferedWriter)) {
    Debug("No buffered writer...");
    Debug("<<<", 10);
    return;
  }

  if ($self->disconnected) {
    Debug("previously disconnected");
    Debug("<<<", 10);
    return;
  }

  my $result = $self->bufferedWriter->Write();

  if ($result == -1) {
    Debug("Would block...", 50);
    return;
  } elsif($result == 0) {
    Debug("write error, disconnecting peer: " . $self->id);
    $self->disconnected(1);
    return;
  }

  Debug("<<<", 10);
}

sub ProcessReadBuffer {
  my $self = shift;

  Debug(">>>", 10);

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

  Debug("<<<", 10);
}

sub disconnected {
  my $self = shift;

  if (@_ and $_[0]) {
    if ($self->socket) {
      $self->socket->close;
      $self->socket(undef);
    }
  }
  $self->_disconnected_accessor(@_);
}

sub DESTROY {
  my $self = shift;

  foreach my $filehandle (values(%{$self->targetFilehandles})) {
    $filehandle->close();
  }
}


1;
