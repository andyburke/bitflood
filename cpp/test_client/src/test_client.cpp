// test_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <WINSOCK2.H>
#include <Ws2tcpip.h>
#include <time.h>
#include <signal.h>

#include <Flood.H>
#include <Client.H>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <sstream>
#include <XmlRpc.H>

XERCES_CPP_NAMESPACE_USE

using namespace libBitFlood;

bool quit = false;
const U32 UPDATE_INTERVAL = 20;
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

  //XmlRpc::setVerbosity(5);

  FloodFile theflood;
  ParseFloodFile( argv[1], theflood );

  Client::Setup setup;
  if ( argc == 3 )
  {
    setup.m_localIP = argv[2];
  }
  else
  {
    hostent* localHost = gethostbyname("");
    setup.m_localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
  }

  setup.m_localPort = 10101;
  if ( argc == 4 )
  {
    std::stringstream convert;
    convert << argv[3];
    convert >> setup.m_localPort;
  }

  Client client;
  client.Initialize( setup );
  client.AddFloodFile( theflood );

  time_t last_update = 0;
  while( !quit )
  {
    time_t now;
    time( &now );
    if ( now - last_update >= UPDATE_INTERVAL )
    {
      client.Register();
      client.UpdatePeerList();
      time( &last_update );
    }

    client.GetChunks();
    client.LoopOnce();
    Sleep( 100 );
  }

  // 
  client.Disconnect();
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

