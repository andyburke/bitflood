package BitFlood::Peer;

use strict;

use base qw(Class::Accessor);

use RPC::XML;
use RPC::XML::Parser;
use IO::Socket;
use Errno qw(:POSIX);
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
use constant FIONBIO            => 0x8004667e; # mswin32 non-blocking socket ioctl


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
                             client
                             readBuffer
                             writeBuffer
                             bufferedReader
                             bufferedWriter
                            )
                         );



sub new {
  my $class = shift;

  Debug("Creating new Peer...", 5);
  my $self = $class->SUPER::new(@_);

  $self->readBuffer('');
  $self->writeBuffer('');

  $self->floods({});
  $self->chunkMaps({});
  $self->registered({});

  $self->client or die("Client object not specified");

  $self->port(10101) if(!defined($self->port));
  Debug("Setting port to: " . $self->port, 7);
  if ($self->socket) {
    $self->socket->blocking(0)
      or die ("This system does not support O_NONBLOCK!");

    Debug("giving ref to: " . \$self->{readBuffer}, 50);
    $self->bufferedReader(BitFlood::Net::BufferedReader->new({buffer => \$self->{readBuffer}, socket => $self->socket}));
    $self->bufferedWriter(BitFlood::Net::BufferedWriter->new({buffer => \$self->{writeBuffer}, socket => $self->socket}));
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

  if (!$self->socket) {

    Debug("begin connect: " . $self->id . "(" . $self->host . ":" . $self->port . ")");

    $self->socket(IO::Socket::INET->new(Proto => 'tcp')); # need to put *something* in args here...
    if (!$self->socket) {
      Debug("socket creation failed");
      $self->disconnected(1);
      return 0;
    }
    $self->connectStartTime(time());

    # set non-blocking
    $self->socket->blocking(0);
    if ($^O eq 'MSWin32') {
      if (!$self->socket->ioctl(FIONBIO, pack("L", 1))) {
        Debug("error setting nonblocking: " . ($!));
        die;
      }
    }

    if (!$self->socket) {
      Debug("socket creation failed");
      $self->disconnected(1);
      return 0;
    }
  }

  return 1 if ($self->socket->connected);

  my $socket_args = {
                     PeerAddr => $self->host,
                     PeerPort => $self->port,
                     Proto    => 'tcp',
                    };
  $socket_args->{Blocking} = 0 if ($^O ne 'MSWin32');
  $self->socket->configure($socket_args);

  if (!$self->socket->connected) {
    if ($!{EINPROGRESS}) {
      # non-blocking connection is still trying to connect
      if (time > $self->connectStartTime + CONNECTION_TIMEOUT) {
        Debug("connection timed out");
        $self->disconnected(1);
      }
      return 0;
    } else {
      # some other error, signaling the connect actually failed
      Debug("failed to connect: $!");
      $self->disconnected(1);
      return 0;
    }
  } else {
    # connection succeeded

    Debug("\$!: $! (" . ($!+0) . ")");
    Debug("socket connected successfully");

    # FIXME figure out why this is needed here as well as above
    # set non-blocking
    $self->socket->blocking(0);
    if ($^O eq 'MSWin32') {
      if (!$self->socket->ioctl(FIONBIO, pack("L", 1))) {
        Debug("error setting nonblocking: " . ($!));
        die;
      }
    }

    $self->bufferedReader(BitFlood::Net::BufferedReader->new({buffer => \$self->{readBuffer}, socket => $self->socket}));
    $self->bufferedWriter(BitFlood::Net::BufferedWriter->new({buffer => \$self->{writeBuffer}, socket => $self->socket}));

    return 1;

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

    my $targetFile = IO::File->new($file->{localFilename}, 'r+');
    $targetFile->seek($chunk->{offset}, 0);
    $targetFile->syswrite($chunkData);
    $targetFile->close();

    $file->{chunkMap}->Bit_On($index);
    $file->{downloadBytes} += $chunk->{size};
    $flood->downloadBytes($flood->downloadBytes + $chunk->{size});

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
  my $remainder;

  Debug("address: " . \$self->readBuffer, 50);
  Debug("buffer: " . $self->readBuffer, 50);

  do {
    ($currentMessage, $remainder) = split("\n", $self->readBuffer, 2);
    Debug("  currentMessage: $currentMessage", 50);
    Debug("  remainder:      $remainder", 50);
    if (length($currentMessage) < length($self->readBuffer)) {
      Debug("end of message detected", 50);
      $self->DispatchRequests($currentMessage);
      substr($self->{readBuffer}, 0, length($currentMessage) + 1, ''); #eat off message + \n we just worked on
    }
  } while (length($remainder));

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


1;
