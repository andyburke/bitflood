// encoder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Encoder.H>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_USE

using namespace libBitFlood;

int _tmain(int argc, _TCHAR* argv[])
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


  Encoder::ToEncode e;
  e.m_files.push_back( "c:\\test.mp3" );
  e.m_chunksize = 262144;
  

  std::string out;
  Encoder::EncodeFile( e, out );

  XMLPlatformUtils::Terminate();
  return 0;
}

