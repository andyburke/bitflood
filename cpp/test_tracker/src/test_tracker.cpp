// test_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <WINSOCK2.H>
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

  //XmlRpc::setVerbosity(5);

  U32 localPort = 10101;
  if ( argc == 2 )
  {
    std::stringstream convert;
    convert << argv[1];
    convert >> localPort;
  }

  Tracker t;
  t.Initialize( localPort );

  while( !quit )
  {
    t.Run( 0 );
    Sleep( 100 );
  }
}

void HandleSignal( I32 sig )
{
  quit = true;
}
