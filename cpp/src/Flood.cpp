#include "stdafx.H"
#include "Flood.H"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/dom/StDOMNode.hpp>
#include <sstream>
#include <iostream>

XERCES_CPP_NAMESPACE_USE

namespace libBitFlood
{
  namespace FloodKeywords
  {
    const wchar_t* CORE_IMPL    = L"CORE";
    const wchar_t* ROOT         = L"BitFlood";
    const wchar_t* FILEINFO     = L"FileInfo";
    const wchar_t* FILE         = L"File";
    const wchar_t* FILE_NAME    = L"name";
    const wchar_t* FILE_SIZE    = L"size";
    const wchar_t* CHUNK        = L"Chunk";
    const wchar_t* CHUNK_INDEX  = L"index";
    const wchar_t* CHUNK_SIZE   = L"size";
    const wchar_t* CHUNK_WEIGHT = L"weight";
    const wchar_t* CHUNK_HASH   = L"hash";
    const wchar_t* TRACKER      = L"Tracker";
  };

  Error::ErrorCode Flood::ToXML( std::wstring& o_xml )
  {
    Error::ErrorCode ret = Error::NO_ERROR;

    using namespace FloodKeywords;

    DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation( CORE_IMPL );
    if (impl != NULL)
    {  
      try
      {
        DOMDocument* doc = impl->createDocument( 0,                    // root element namespace URI.
          ROOT,         // root element name
          0);                   // document type object (DTD).

        DOMElement* rootElem = doc->getDocumentElement();

        DOMElement* fileInfoElem = doc->createElement( FILEINFO );
        rootElem->appendChild(fileInfoElem);

        V_File::const_iterator fileiter = m_files.begin();
        V_File::const_iterator fileend  = m_files.end();

        for ( ; fileiter != fileend; ++fileiter )
        {
          const File& file = *fileiter;
          DOMElement*  fileElem = doc->createElement( FILE );
          fileInfoElem->appendChild(fileElem);

          std::wstringstream fileSizeStrm;
          fileSizeStrm << file.m_size;

          fileElem->setAttribute( FILE_NAME, XMLString::transcode( file.m_name.c_str() ) );
          fileElem->setAttribute( FILE_SIZE, fileSizeStrm.str().c_str() );

          V_Chunk::const_iterator chunkiter = file.m_chunks.begin();
          V_Chunk::const_iterator chunkend  = file.m_chunks.end();

          for ( ; chunkiter != chunkend; ++chunkiter )
          {
            const Chunk& chunk = *chunkiter;
            DOMElement*  chunkElem = doc->createElement( CHUNK );
            fileElem->appendChild(chunkElem);

            std::wstringstream indexStrm;
            indexStrm << chunk.m_index;

            std::wstringstream sizeStrm;
            sizeStrm << chunk.m_size;

            std::wstringstream weightStrm;
            weightStrm << chunk.m_weight;

            chunkElem->setAttribute( CHUNK_HASH, XMLString::transcode( chunk.m_hash.c_str() ) );
            chunkElem->setAttribute( CHUNK_INDEX, indexStrm.str().c_str() );
            chunkElem->setAttribute( CHUNK_SIZE,  sizeStrm.str().c_str() );
            chunkElem->setAttribute( CHUNK_WEIGHT, weightStrm.str().c_str() );
          }
        }

        DOMElement* trackerElem = doc->createElement( TRACKER );
        rootElem->appendChild(trackerElem);

        DOMText* trackerName = doc->createTextNode( XMLString::transcode( m_tracker.c_str() ) );
        trackerElem->appendChild(trackerName);

        DOMWriter* writer = impl->createDOMWriter();
        writer->setFeature( XMLUni::fgDOMWRTFormatPrettyPrint, 1 );
        o_xml = writer->writeToString( *rootElem );
      }
      catch (const OutOfMemoryException&)
      {
        XERCES_STD_QUALIFIER cerr << "OutOfMemoryException" << XERCES_STD_QUALIFIER endl;
        ret = Error::UNKNOWN_ERROR;
      }
      catch (const DOMException& e)
      {
        XERCES_STD_QUALIFIER cerr << "DOMException code is:  " << e.code << XERCES_STD_QUALIFIER endl;
        ret = Error::UNKNOWN_ERROR;
      }
      catch (...)
      {
        XERCES_STD_QUALIFIER cerr << "An error occurred creating the document" << XERCES_STD_QUALIFIER endl;
        ret = Error::UNKNOWN_ERROR;
      }
    }

    return ret;
  }

  Error::ErrorCode Flood::FromXML( const std::wstring& i_xml )
  {
    //
    //  Create our parser, then attach an error handler to the parser.
    //  The parser will call back to methods of the ErrorHandler if it
    //  discovers errors during the course of parsing the XML document.
    //
    XercesDOMParser *parser = new XercesDOMParser;
    //    DOMTreeErrorReporter *errReporter = new DOMTreeErrorReporter();
    //    parser->setErrorHandler(errReporter);

    //MemBufInputSource src( (const XMLByte *const )(i_xml.c_str()), i_xml.length(), "foo" );

    //
    //  Parse the XML file, catching any XML exceptions that might propogate
    //  out of it.
    //
    bool errorsOccured = false;
    try
    {
      parser->parse( i_xml.c_str() );
    }
    catch (const OutOfMemoryException&)
    {
      XERCES_STD_QUALIFIER cerr << "OutOfMemoryException" << XERCES_STD_QUALIFIER endl;
      errorsOccured = true;
    }
    catch (const XMLException& e)
    {
      XERCES_STD_QUALIFIER cerr << "An error occurred during parsing\n   Message: "
        << e.getMessage() << XERCES_STD_QUALIFIER endl;
      errorsOccured = true;
    }

    catch (const DOMException& e)
    {
      const unsigned int maxChars = 2047;
      XMLCh errText[maxChars + 1];

      XERCES_STD_QUALIFIER cerr << "\nDOM Error during parsing, "
        << "DOMException code is:  " << e.code << XERCES_STD_QUALIFIER endl;

      if (DOMImplementation::loadDOMExceptionMsg(e.code, errText, maxChars))
        XERCES_STD_QUALIFIER cerr << "Message is: " << errText << XERCES_STD_QUALIFIER endl;

      errorsOccured = true;
    }

    catch (...)
    {
      XERCES_STD_QUALIFIER cerr << "An error occurred during parsing\n " << XERCES_STD_QUALIFIER endl;
      errorsOccured = true;
    }

    // If the parse was successful, output the document data from the DOM tree
    if ( !errorsOccured )//&& !errReporter->getSawErrors())
    {
      using namespace FloodKeywords;
      // get the DOM representation
      DOMDocument *doc = parser->getDocument();

      DOMNodeList* list = doc->getElementsByTagName( FILEINFO );
      if ( list && list->getLength() == 1 )
      {
        DOMElementSPtr fileinfo = (DOMElement*)list->item( 0 );
        DOMNodeList* filelist = fileinfo->getElementsByTagName( FILE );
        if ( filelist )
        {
          for ( U32 fileindex = 0; fileindex < filelist->getLength(); ++fileindex )
          {
            DOMElementSPtr file = (DOMElement*)filelist->item( fileindex );
            File lbffile;

            std::wstring file_size = file->getAttribute( FILE_SIZE );

            lbffile.m_name = XMLString::transcode( file->getAttribute( FILE_NAME ) );

            std::wstringstream file_size_convert;
            file_size_convert << file_size;
            file_size_convert >> lbffile.m_size;

            DOMNodeList* chunklist = file->getElementsByTagName( CHUNK );
            if ( chunklist )
            {
              for ( U32 chunkindex = 0; chunkindex < chunklist->getLength(); ++chunkindex )
              {
                DOMElementSPtr chunk = (DOMElement*)chunklist->item( chunkindex );
                Chunk lbfchunk;

                std::wstring chunk_index  = chunk->getAttribute( CHUNK_INDEX );
                std::wstring chunk_weight = chunk->getAttribute( CHUNK_WEIGHT );
                std::wstring chunk_size   = chunk->getAttribute( CHUNK_SIZE );

                lbfchunk.m_hash = XMLString::transcode( chunk->getAttribute( CHUNK_HASH ) );

                std::wstringstream chunk_index_convert;
                chunk_index_convert << chunk_index;
                chunk_index_convert >> lbfchunk.m_index;

                std::wstringstream chunk_weight_convert;
                chunk_weight_convert << chunk_weight;
                chunk_weight_convert >> lbfchunk.m_weight;

                std::wstringstream chunk_size_convert;
                chunk_size_convert << chunk_size;
                chunk_size_convert >> lbfchunk.m_size;

                lbffile.m_chunks.push_back( lbfchunk );
              }
            }

            m_files.push_back( lbffile );
          }
        }

        list = NULL;
      }

      list = doc->getElementsByTagName( TRACKER );
      if ( list && list->getLength() == 1 )
      {
        DOMNodeSPtr tracker = list->item( 0 );
        DOMNodeSPtr tracker_child = tracker->getFirstChild();
        if ( tracker_child )
        {
          m_tracker = XMLString::transcode( tracker_child->getNodeValue() );
        }
        list = NULL;
      }
    }

    //
    //  Delete the parser itself.
    //
    delete parser;

    return Error::NO_ERROR;
  }
};

