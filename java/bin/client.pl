#!/usr/bin/perl
use strict;
use Getopt::Long;
use File::Path;
use File::Basename;

my $iClient = 1;
GetOptions( "client=i" => \$iClient );

my $sClientDir = "../test/client_$iClient";
my $iPort = 10101 + $iClient;

mkpath $sClientDir;
chdir $sClientDir;
system( "java -classpath ../sdk/xerces/xerces-2_6_2/xercesImpl.jar:../sdk/xerces/xerces-2_6_2/xml-apis.jar:./BitFlood.jar com.net.BitFlood.test.ClientTest -flood ../test/sparta.flood -localIP vux.fdntech.com -localPort $iPort" );
