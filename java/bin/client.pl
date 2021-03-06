#!/usr/bin/perl
use strict;
use Getopt::Long;
use File::Path;
use File::Basename;

chdir dirname $0;

my $iClient = 1;
GetOptions( "client=i" => \$iClient );

my $sClientDir = "../test/client_$iClient";
my $iPort = 10101 + $iClient;

mkpath $sClientDir;
chdir $sClientDir;
system( "java -classpath ../../sdk/xerces/xerces-2_6_2/xercesImpl.jar:../../sdk/xerces/xerces-2_6_2/xml-apis.jar:../../bin/BitFlood.jar com.net.BitFlood.test.ClientTest -flood ../sparta.flood -localIP vux.fdntech.com -localPort $iPort" );
