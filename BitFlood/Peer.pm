package BitFlood::Peer;

use strict;

use base qw(Class::Accessor);

use RPC::XML;
use RPC::XML::Parser;
use IO::Socket;
use POSIX qw(:errno_h);
use Bit::Vector;
use Digest::SHA1 qw(sha1_base64);
use Time::HiRes qw(time);

use Data::Dumper; # XXX

use BitFlood::Utils;
use BitFlood::Debug;
use BitFlood::Logger;
use BitFlood::Logger::Multi;
use BitFlood::Logger::File;

use constant MAX_SOCKET_WINDOW => 256 * 1024;
use constant MIN_SOCKET_WINDOW => 512;

__PACKAGE__->mk_accessors(qw(id host port floods chunkMaps registered
			     socket select disconnected client
                             readBuffer writeBuffer
			     socketReadWindow socketWriteWindow));


sub new {
  my $class = shift;

  Debug("Creating new Peer...", 5);
  my $self = $class->SUPER::new(@_);

  $self->client or die("Client object not specified");

  $self->port(10101) if(!defined($self->port));
  Debug("Setting port to: " . $self->port, 7);
  if ($self->socket) {
    $self->socket->blocking(0)
      or die ("This system does not support O_NONBLOCK!");
  }
  #$self->select(IO::Select->new);
  #$self->UpdateSelect if defined $self->socket;

  $self->socketReadWindow(MAX_SOCKET_WINDOW);
  $self->socketWriteWindow(MIN_SOCKET_WINDOW);
  $self->readBuffer('');
  $self->floods({});
  $self->chunkMaps({});
  $self->registered({});

  return $self;
}

sub SendMessage {
  my $self = shift;
  my $methodName = shift;
  my $flood = shift;
  my @methodArgs = @_;

  Debug(">>>", 10);
  
  unshift(@methodArgs, $flood->contentHash);
  Debug("method $methodName (" . scalar(@methodArgs) . " args)", 5);
  my $request = RPC::XML::request->new($methodName, @methodArgs);
  Debug("  queueing to: " . $self->id . " (" . $self->host . ":" . $self->port . ")", 15);
  $self->writeBuffer($self->writeBuffer . $request->as_string . "\n");
  Debug("  write buffer: " . $self->writeBuffer, 50);

  Debug("<<<", 10);
  return 1;
}

sub Connect {
  my $self = shift;

  return 1 if $self->socket;

  Debug($self->id . "(" . $self->host . ":" . $self->port . ")");
  $self->socket(IO::Socket::INET->new(
				      PeerHost => $self->host,
				      PeerPort => $self->port,
				      Proto => 'tcp',
				      Timeout => 1, # FIXME long enough?
				      Blocking => 0,
				     ));
  if(!$self->socket) {
    Debug("failure: " . $self->id . " (" . $self->host . ":" . $self->port . ")");
    $self->disconnected(1);
    return undef;
  }
  
  Debug("connected to peer", 10);
  return 1;
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
		     { $_->{name} => $_->{chunkMap}->to_Hex() }
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
    $chunkMap = Bit::Vector->new_Hex(length($chunkMap) * 4, $chunkMap);
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

    my $targetFile = IO::File->new($file->{localFilename}, 'r+');
    $targetFile->seek($chunk->{offset}, 0);
    $targetFile->print($chunkData);
    $targetFile->close();

    $file->{chunkMap}->Bit_On($index);
    $file->{downloadBytes} += $chunk->{size};
    $flood->downloadBytes($flood->downloadBytes + $chunk->{size});

    foreach my $peer (grep { $_->floods->{$flood->contentHash} } @{$self->client->peers}) {
      $self->SendMessage(
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

  $self->Connect;
  $self->ReadOnce;
  $self->WriteOnce;

  Debug("<<<", 10);
}


sub ReadOnce {
  my $self = shift;

  Debug(">>>", 10);

  if ($self->disconnected) {
    Debug("previously disconnected");
    Debug("<<<", 10);
    return;
  }

  #Debug($self->select->handles, 60);

  if (
#$self->select->can_read(0)
1) {
    my ($currentMessage, $remainder);
    Debug("can indeed read", 50);

    #die "ReadOnce: socket exception: $!" if $self->select->has_exception(0);

    Debug("read window: " . $self->socketReadWindow, 50);
    my $readStartTime = time();
    my $remainderLength = $self->socket->sysread($remainder, $self->socketReadWindow);
    my $transferTime = time() - $readStartTime;
    if (!defined $remainderLength) {
      if ($! == EAGAIN) {
	Debug("would block", 50);
	return;
      } else {
	Debug("unexpected socket error: $!");
	$self->disconnected(1);
	return;
      }
    }
    Debug("read data", 50);
    if ($remainder eq '') {
      Debug("remote end disconnected: ".$self->host.':'.$self->port, 10);
      $self->disconnected(1);
      Debug("<<<", 10);
      return;
    }

#    Debug(sprintf("TDBR: %3e %3e %d %3e",
#		  $transferTime,
#		  $self->client->desiredPeerLoopDuration/2,
#		  $remainderLength,
#		  $remainderLength / $transferTime));
	  
#    if($transferTime > $self->client->desiredPeerLoopDuration / 2) {
#      if($self->socketReadWindow > MIN_SOCKET_WINDOW) { # need time to read AND write, so halve
#	$self->socketReadWindow($self->socketReadWindow / 2);
#	Debug("decreased window: " . $self->socketReadWindow(), 10);
#      }
#    } elsif($self->socketReadWindow < MAX_SOCKET_WINDOW) { # FIXME: avoid hysterisis
#      $self->socketReadWindow($self->socketReadWindow * 2);
#      Debug("increased window: " . $self->socketReadWindow(), 10);
#    }

#    Debug("read time: $transferTime");
    do {
      Debug("remainder: ($remainderLength) $remainder", 50);
      ($currentMessage, $remainder) = split("\n", $remainder, 2);
      Debug("  currentMessage: $currentMessage", 50);
      Debug("  remainder:      $remainder", 50);
      if (length($currentMessage) < $remainderLength) {
	Debug("end of message detected", 50);
	$currentMessage = $self->readBuffer . $currentMessage;
	Debug("entire message: $currentMessage", 50);
	$self->DispatchRequests($currentMessage);
	$self->readBuffer('');
      } else {
	Debug("continuing message with: $currentMessage", 50);
	$self->readBuffer($self->readBuffer . $currentMessage);
      }
      $remainderLength = length($remainder);
    } while ($remainderLength);

  } else {
    Debug("no data to read, currently", 50);
  }

  Debug("<<<", 10);
}


sub WriteOnce {
  my $self = shift;

  Debug(">>>", 10);

  if ($self->disconnected) {
    Debug("previously disconnected");
    Debug("<<<", 10);
    return;
  }

  if (
#$self->select->can_write(0) and 
length($self->writeBuffer)) {
    # FIXME maybe need to hit the hash directly to avoid string copies

    #die "WriteOnce: socket exception: $!" if $self->select->has_exception(0);

    ###my $remainder = $self->writeBuffer;
    ###Debug(length($remainder) . " bytes to write", 50);
    ###my $currentMessage = substr($remainder, 0, $self->socketWriteWindow, '');
    ###Debug("currentMessage: $currentMessage", 50);

    Debug("write window: " . $self->socketWriteWindow, 50);
    # FIXME look into local ( $SIG{PIPE} ) ?
    my $transferStartTime = time();
    my $bytesWritten = $self->socket->syswrite($self->writeBuffer, $self->socketWriteWindow);
    my $transferTime = time() - $transferStartTime;
    if (!defined $bytesWritten) {
      if ($! == EAGAIN) {
	Debug("would block", 50);
	return;
      } else {
	Debug("unexpected socket error: $!");
	$self->disconnected(1);
	return;
      }
    }

#    Debug(sprintf("TDBR: %3e %3e %d %3e",
#		  $transferTime,
#		  $self->client->desiredPeerLoopDuration/2,
#		  $bytesWritten,
#		  $bytesWritten / $transferTime));

    my $remainder = $self->writeBuffer;
    Debug("$bytesWritten bytes written", 50);
    substr($remainder, 0, $bytesWritten, '');
    $self->writeBuffer($remainder);
    Debug(length($remainder) . " bytes remain", 50);

#    Debug("write time: $transferTime");
#    Debug("desired transfer duration: ".$self->client->desiredPeerLoopDuration/2);
    if($transferTime > $self->client->desiredPeerLoopDuration) {
      if($self->socketWriteWindow > MIN_SOCKET_WINDOW) { # need time to read AND write, so halve
	$self->socketWriteWindow($self->socketWriteWindow / 2);
	Debug("decreased window: " . $self->socketWriteWindow(), 10);
      }
    } elsif($self->socketWriteWindow < MAX_SOCKET_WINDOW) { # FIXME: avoid hysterisis
      $self->socketWriteWindow($self->socketWriteWindow * 2);
      Debug("increased window: " . $self->socketWriteWindow(), 10);
    }

  }

  Debug("<<<", 10);
}


sub UpdateSelect {
  my $self = shift;

  $self->select->remove($self->select->handles);
  Debug("Adding to select handles: " . $self->socket, 60);
  $self->select->add($self->socket);
}


=pod

sub socket {
  my $self = shift;

  my $socket = $self->_socket_accessor(@_);
  if (@_) {
    $self->UpdateSelect;
  }

  return $socket;
}

=cut


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


1;
