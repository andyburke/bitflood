// encoder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Flood.H>
#include <Encoder.H>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <sstream>

XERCES_CPP_NAMESPACE_USE

using namespace libBitFlood;

int main(int argc, char* argv[])
{
  if ( argc != 4 )
  {
    std::cerr << "Please use three arguments: the name of the file to encode, the name of the tracker, and the name of the flood file" << std::endl;
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

    Encoder::ToEncode::Tracker tracker;
    std::string trackerurl = argv[2];

    U32 h_start = trackerurl.find( "http://" ) + strlen( "http://" );
    U32 p_start = trackerurl.find( ':', h_start ) + 1;
    U32 u_start = trackerurl.find( '/', p_start ) + 1;

    tracker.first = trackerurl.substr( h_start, p_start - h_start - 1 );

    std::stringstream port_converter;
    port_converter << trackerurl.substr( p_start, u_start - p_start - 1 );
    port_converter >> tracker.second;

    Encoder::ToEncode e;
    e.m_files.push_back( argv[1] );
    e.m_chunksize = 262144;
    e.m_trackers.push_back( tracker );

    FloodFile out;
    Encoder::EncodeFile( e, out );

    std::string txt;
    out.ToXML( txt );

    FILE* outfile = fopen( argv[3], "w" );
    if ( outfile )
    {
      fprintf( outfile, "%s", txt.c_str() );
      fclose( outfile );
    }

    XMLPlatformUtils::Terminate();
    return 0;
  }
}

