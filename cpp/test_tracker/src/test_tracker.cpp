// test_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <WINSOCK2.H>
#include <Tracker.H>
#include <XmlRpc.H>
#include <sstream>

using namespace libBitFlood;

int main(int argc, char* argv[])
{
  if ( argc != 1 && argc != 2 )
  {
    std::cerr << "Please use one arguments: the local port" << std::endl;
    return 1;
  }

  XmlRpc::setVerbosity(5);

  U32 localPort = 10101;
  if ( argc == 2 )
  {
    std::stringstream convert;
    convert << argv[1];
    convert >> localPort;
  }

  Tracker t;
  t.Initialize( localPort );

  while( 1 )
  {
    t.Run( 1.0f );
    Sleep(0);
  }
}
