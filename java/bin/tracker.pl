#!/usr/bin/perl
use strict;
system( "java -classpath ../sdk/xerces/xerces-2_6_2/xercesImpl.jar:../sdk/xerces/xerces-2_6_2/xml-apis.jar:./BitFlood.jar com.net.BitFlood.test.TrackerTest -flood ../test/sparta.flood -localIP vux.fdntech.com -localPort 10101" );
