// test_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Flood.H>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <sstream>

XERCES_CPP_NAMESPACE_USE

using namespace libBitFlood;

int main(int argc, char* argv[])
{
  if ( argc != 2 )
  {
    std::cerr << "Please use one argument: the name of the flood file" << std::endl;
    return 1;
  }
  else
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
      return 1;
    }

    char* data = NULL;
    FILE* infile = fopen( argv[1], "r" );
    if ( infile )
    {
      fseek(infile,0,SEEK_END);

      U32 size = ftell( infile );
      data = (char *)malloc( size + 1 );
      data[ size ] = 0;

      rewind( infile );
      fread( data, 1, size, infile );

      fclose(infile);
    }

    std::wstringstream wide;
    wide << argv[1];

    Flood in;
    in.FromXML( wide.str() );


    XMLPlatformUtils::Terminate();
    return 0;
  }
}

