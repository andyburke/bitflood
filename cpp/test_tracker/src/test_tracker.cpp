// test_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <WINSOCK2.H>
#include <Client.H>
#include <Tracker.H>
#include <XmlRpc.H>
#include <sstream>
#include <signal.h>

using namespace libBitFlood;

bool quit = false;
void HandleSignal( I32 sig );

int main(int argc, char* argv[])
{
  if ( argc != 1 && argc != 2 )
  {
    std::cerr << "Please use one arguments: the local port" << std::endl;
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
 
  Client::Setup setup;
  hostent* localHost = gethostbyname("");
  setup.m_localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
  setup.m_localPort = 10101;
  if ( argc == 2 )
  {
    std::stringstream convert;
    convert << argv[1];
    convert >> setup.m_localPort;
  }

  ClientSPtr tracker( new Client() );
  tracker->Initialize( setup );
  TrackerMessageHandler::AddTrackerHandlers( tracker );
  while( !quit )
  {
    tracker->LoopOnce();
    Sleep( 100 );
  }
}

void HandleSignal( I32 sig )
{
  quit = true;
}
