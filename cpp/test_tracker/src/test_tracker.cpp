// test_tracker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <WINSOCK2.H>
#include <Ws2tcpip.h>
#include <time.h>
#include <signal.h>

#include <Flood.H>
#include <Peer.H>
#include <TrackerMethods.H>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <sstream>
#include <XmlRpc.H>

XERCES_CPP_NAMESPACE_USE

using namespace libBitFlood;

bool quit = false;
void ParseFloodFile( const std::string& file, FloodFile& floodfile );
void HandleSignal( I32 sig );

int main(int argc, char* argv[])
{
  if ( argc != 2 && argc != 3 && argc != 4 )
  {
    std::cerr << "Please use three arguments: the name of the flood file, the local ip and the local port" << std::endl;
    return 1;
  }

  // connect a signal handler
  signal( SIGINT,   HandleSignal );
  signal( SIGBREAK, HandleSignal );
  signal( SIGTERM,  HandleSignal );
  signal( SIGABRT,  HandleSignal );

  // startup winsock
  WSADATA wsaData;
  WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

  FloodFileSPtr theflood( new FloodFile() );
  ParseFloodFile( argv[1], *theflood );

  std::string host = "";
  U32 port = 10101;
  if ( argc >= 2 )
  {
    host = argv[2];
  }

  hostent* tmp = gethostbyname( host.c_str() );
  host = inet_ntoa (*(struct in_addr *)*tmp->h_addr_list);

  if ( argc > 3 )
  {
    std::stringstream convert;
    convert << argv[3];
    convert >> port;
  }

  PeerSPtr tracker( new Peer() );
  tracker->Initialize( host, port );
  tracker->AddFloodFile( theflood );
  tracker->AddHandler( MethodHandlerSPtr( new TrackerMethodHandler() ) );

  while( !quit )
  {
    tracker->LoopOnce();
    Sleep( 100 );
  }
}

void ParseFloodFile( const std::string& file, FloodFile& floodfile )
{
  // Initialize the XML4C2 system.
  try
  {
    XMLPlatformUtils::Initialize();
  }

  catch(const XMLException& toCatch)
  {
    char *pMsg = XMLString::transcode(toCatch.getMessage());
    XERCES_STD_QUALIFIER cerr << "Error during Xerces-c Initialization.\n"
      << "  Exception message:"
      << pMsg;
    XMLString::release(&pMsg);
    return;
  }

  std::string xml;
  FILE* infile = fopen( file.c_str(), "r" );
  if ( infile )
  {
    fseek(infile,0,SEEK_END);

    U32 size = ftell( infile );
    char* data = (char *)malloc( size + 1 );
    data[ size ] = 0;

    rewind( infile );
    fread( data, 1, size, infile );

    fclose(infile);

    xml = data;
    free( data );
  }

  floodfile.FromXML( xml );

  XMLPlatformUtils::Terminate();
}

void HandleSignal( I32 sig )
{
  quit = true;
}

