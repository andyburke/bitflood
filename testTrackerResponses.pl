use strict;

use RPC::XML;
use RPC::XML::Client;
use Data::Dumper;

my $tracker = RPC::XML::Client->new('http://localhost:10101/RPCSERV');

my $response = $tracker->simple_request('system.listMethods');
print Dumper $response;

my $resp = $tracker->simple_request('system.methodSignature',
                                     RPC::XML::string->new('Register'));
print Dumper $resp;

my $registerResult = $tracker->simple_request('Register',
                                              'aaaaaa',
	                                      '192.168.2.1',
                                              '9001',
                                             );
print "register:\n";
print Dumper $registerResult;

$registerResult = $tracker->simple_request('Register',
                                           'aaaaaa',
                                           '192.168.2.2',
                                           '9001',
                                           );
print "register:\n";
print Dumper $registerResult;

my $dumpResult = $tracker->simple_request('Dump');
print "dump:\n";
print Dumper $dumpResult;

sleep(10);

$registerResult = $tracker->simple_request('Register',
                                           'aaaaaa',
                                           '192.168.2.3',
                                           '9001',
                                           );
print "register:\n";
print Dumper $registerResult;


$dumpResult = $tracker->simple_request('Dump');
print "dump:\n";
print Dumper $dumpResult;

my $requestResult = $tracker->simple_request('Request',
                                             'aaaaaa');
print "request:\n";
print Dumper $requestResult;

sleep(10);

$dumpResult = $tracker->simple_request('Dump');
print "dump:\n";
print Dumper $dumpResult;

$requestResult = $tracker->simple_request('Request',
                                          'aaaaaa');
print "request:\n";
print Dumper $requestResult;


