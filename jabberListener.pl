#!/usr/bin/perl

use strict;
use Net::Jabber;
use Getopt::Long;
use Data::Dumper;



my $server      = 'jabber.org';
my $serverPort  = 5222;
my $username    = 'bftest';
my $password    = 'bftest';
my $resource    = 'listener';
my $flushPeriod = 5;
my $help;
my %files;

if(!GetOptions(
	       "server=s"      => \$server,
	       "serverport=s"  => \$serverPort,
	       "username=s"    => \$username,
	       "password=s"    => \$password,
	       "resource=s"    => \$resource,
               "flushperiod=s" => \$flushPeriod,
	       "help"          => \$help,
	       )
   or $help) {
  print "options: server serverport username password resource flushperiod\n";
  exit;
}


my $client = Net::Jabber::Client->new;
print "== connecting to $server:$serverPort\n";
if (!$client->Connect(hostname => $server,
		      port     => $serverPort)) {
  print "!! could not connect to server\n";
  exit;
}
print "== authing as $username:$password/$resource\n";
my @auth = $client->AuthSend(username => $username,
			     password => $password,
			     resource => $resource);
if ($auth[0] ne "ok") {
  print "!! authentication failed: " . join(" / ", @auth) . "\n";
  exit;
}

$client->SetCallBacks(message => \&handleMessage);


$SIG{INT} = \&sigInt;
$SIG{ALRM} = \&sigAlarm;

print "== will flush files every $flushPeriod seconds\n";
alarm($flushPeriod);
while (1) {
  my $gotData = $client->Process(1);
  defined $gotData or quit();
}



sub sigInt {
  quit();
}

sub sigAlarm {
  flushFiles();
  alarm($flushPeriod);
}


sub quit {
  $client->Disconnect;
  exit;
}

sub flushFiles {
  print "== flushing files\n";
  foreach my $file (values %files) {
    $file->flush
  }
}


sub handleMessage {
  my $id = shift;
  my $message = shift;

  my $from = $message->GetFrom("jid");
  my $resource = $from->GetResource;
  my $body = $message->GetBody;

  if (!$files{$resource}) {
    print "== opening file: $resource.log\n";
    $files{$resource} = IO::File->new(">>$resource.log");
  }
  $files{$resource}->print($body . "\n");

  print "<- $resource\n";
}
