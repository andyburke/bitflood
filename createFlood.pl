use strict;

use Getopt::Long;

use BitFlood::Encoder;

my $floodFile;
my $weightingFunction;
my @trackerUrls;
my $getHelp;

GetOptions(
	   "flood=s"     => \$floodFile,
	   "weighting=s" => \$weightingFunction,
	   "tracker=s"   => \@trackerUrls,
	   "h"           => \$getHelp,
	   "help"        => \$getHelp, 
	   );

if($getHelp or !$floodFile or !@trackerUrls or !@ARGV) {
  my ($appName) = $0 =~ /[\\\/]?(.*?)$/;
  print <<USAGE;
Usage:
  $appName -flood <floodname.flood> -tracker <host> [-tracker <host>...] [-weighting [topheavy|bottomheavy]] <files/dirs to add>
USAGE
  exit(0);
}


foreach my $trackerUrl (@trackerUrls) {
  next if($trackerUrl =~ m|^http://|);
  my ($host, $port) = split(':', $trackerUrl);
  die("Could not determing host from '$trackerUrl'!") if(!defined($host));
  $port ||= 10101;
  $trackerUrl = "http://$host:$port/RPCSERV";
}

print qq{
  Creating Flood:
    trackers : @trackerUrls
    weighting: $weightingFunction
    files    : @ARGV

};

my $e = new BitFlood::Encoder(filename => @ARGV > 1 ? \@ARGV : $ARGV[0],
                              weightingFunction => $weightingFunction,
                              tracker => \@trackerUrls);

open(OUTFILE, ">$floodFile");
print OUTFILE $e->encode();
close(OUTFILE);
