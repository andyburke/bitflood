#!/usr/bin/perl

use strict;
use Net::Jabber;
use Getopt::Long;
use Data::Dumper;



my $server     = 'jabber.org';
my $serverPort = 5222;
my $username   = 'bftest';
my $password   = 'bftest';
my $resource   = 'listener';
my $help;

if(!GetOptions(
	       "server=s"     => \$server,
	       "serverport=s" => \$serverPort,
	       "username=s"   => \$username,
	       "password=s"   => \$password,
	       "resource=s"   => \$resource,
	       "help"         => \$help,
	       )
   or $help) {
  print "options: server serverport username password resource\n";
  exit;
}


my $client = Net::Jabber::Client->new;
if (!$client->connect(hostname => $server,
		      port     => $serverPort)) {
  print "could not connect to server\n";
  exit;
}
my @auth = $client->AuthSend(username => $username,
			     password => $password,
			     resource => $resource);
if ($auth[0] ne "ok") {
  print "authentication failed: " . join(" / ", @auth) . "\n";
  exit;
}

$client->SetCallBacks(message => \&handle_message);


my $quit = 0;
$SIG{INT} = sub { $quit = 1 };

while (!$quit) {
  my $gotData = $client->Process(1);
  defined $gotData or $quit = 1;
}

$client->Disconnect;

exit;



sub handle_message {
  my $message = Net::Jabber::Message->new(@_);

  print Dumper($message);
}
