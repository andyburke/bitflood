#include "stdafx.H"
#include "Flood.H"
#include "Client.H"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/dom/StDOMNode.hpp>
#include <sstream>
#include <iostream>
#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>

XERCES_CPP_NAMESPACE_USE

namespace libBitFlood
{
  namespace FloodFileKeywords
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

  Error::ErrorCode FloodFile::ToXML( std::string& o_xml )
  {
    Error::ErrorCode ret = Error::NO_ERROR;

    using namespace FloodFileKeywords;

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

        // write the trackers
        V_String::const_iterator trackeriter = m_trackers.begin();
        V_String::const_iterator trackerend  = m_trackers.end();

        for ( ; trackeriter != trackerend; ++trackeriter )
        { 
          DOMElement* trackerElem = doc->createElement( TRACKER );
          rootElem->appendChild(trackerElem);

          DOMText* trackerName = doc->createTextNode( XMLString::transcode( trackeriter->c_str() ) );
          trackerElem->appendChild(trackerName);
        }

        // write the the output string
        DOMWriter* writer = impl->createDOMWriter();
        writer->setFeature( XMLUni::fgDOMWRTFormatPrettyPrint, 1 );
        o_xml = XMLString::transcode( writer->writeToString( *rootElem ) );
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

  Error::ErrorCode FloodFile::FromXML( const std::string& i_xml )
  {
    //
    //  Create our parser, then attach an error handler to the parser.
    //  The parser will call back to methods of the ErrorHandler if it
    //  discovers errors during the course of parsing the XML document.
    //
    XercesDOMParser *parser = new XercesDOMParser;
    //    DOMTreeErrorReporter *errReporter = new DOMTreeErrorReporter();
    //    parser->setErrorHandler(errReporter);

    MemBufInputSource src( (const XMLByte*)i_xml.data(), i_xml.length(), "input" );
    src.setEncoding( L"ASCII" );

    // hash the incoming XML
    {
      using namespace CryptoPP;
      SHA sha;
      HashFilter shaFilter(sha);
      std::auto_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
      channelSwitch->AddDefaultRoute(shaFilter);

      StringSource( (const byte*)i_xml.data(), i_xml.length(), true, channelSwitch.release() );
      std::stringstream out;
      Base64Encoder encoder( new FileSink( out ), false );
      shaFilter.TransferTo( encoder );

      m_contentHash = out.str();
    }

    //
    //  Parse the XML file, catching any XML exceptions that might propogate
    //  out of it.
    //
    bool errorsOccured = false;
    try
    {
      parser->parse( src );
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
      using namespace FloodFileKeywords;
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

      // read the trackers
      list = doc->getElementsByTagName( TRACKER );
      if ( list )
      {
        for ( U32 trackerindex = 0; trackerindex < list->getLength(); ++trackerindex )
        {
          DOMNodeSPtr tracker = list->item( trackerindex );
          DOMNodeSPtr tracker_child = tracker->getFirstChild();
          if ( tracker_child )
          {
            m_trackers.push_back( XMLString::transcode( tracker_child->getNodeValue() ) );
          }
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

  Error::ErrorCode FloodFile::ComputeHash( std::string& o_hash )
  {
    return Error::NO_ERROR;
  }

  //
  Error::ErrorCode Flood::Initialize( const FloodFile& i_floodfile )
  {
    m_floodfile = i_floodfile;

    V_String::const_iterator trackeriter = m_floodfile.m_trackers.begin();
    V_String::const_iterator trackerend = m_floodfile.m_trackers.end();

    for( ; trackeriter != trackerend; ++trackeriter )
    {
      const std::string& trackerurl = *trackeriter;

      U32 h_start = trackerurl.find( "http://" ) + strlen( "http://" );
      U32 p_start = trackerurl.find( ':', h_start ) + 1;
      U32 u_start = trackerurl.find( '/', p_start ) + 1;

      std::string host = trackerurl.substr( h_start, p_start - h_start - 1 );
      std::string uri  = trackerurl.substr( u_start, std::string::npos );

      U32 port;
      std::stringstream port_converter;
      port_converter << trackerurl.substr( p_start, u_start - p_start - 1 );
      port_converter >> port;

      XmlRpcClient* client = new XmlRpcClient( host.c_str(), port, uri.c_str() );
      m_trackerConnections.push_back( client );
    }

    // here we setup our internal representation of the state of the files
    // and the chunks that we either have already downloaded or want to download
    _SetupFilesAndChunks();

    return Error::NO_ERROR;
  }

  Error::ErrorCode Flood::Register( const Client& i_client )
  {
    XmlRpcValue args, result;
    args[0] = i_client.m_id;
    args[1] = m_floodfile.m_contentHash;
    args[2] = i_client.m_setup.m_localIP;
    args[3] = (int)i_client.m_setup.m_localPort;

    V_XmlRpcClientPtr::iterator trackeriter = m_trackerConnections.begin();
    V_XmlRpcClientPtr::iterator trackerend  = m_trackerConnections.end();

    for ( ; trackeriter != trackerend; ++trackeriter )
    {
      if ( (*trackeriter)->execute("Register", args, result) )
        std::cout << result << "\n\n";
      else
        std::cout << "Error calling 'Register'\n\n";
    }

    return Error::NO_ERROR;
  }

  Error::ErrorCode Flood::UpdatePeerList( void )
  {
    XmlRpcValue args, result;
    args[0] = m_floodfile.m_contentHash;

    V_XmlRpcClientPtr::iterator trackeriter = m_trackerConnections.begin();
    V_XmlRpcClientPtr::iterator trackerend  = m_trackerConnections.end();

    for ( ; trackeriter != trackerend; ++trackeriter )
    {
      if ( (*trackeriter)->execute("RequestPeers", args, result) )
      {
        std::cout << result << "\n\n";

        U32 res_index;
        for ( res_index = 0; res_index < result.size(); res_index++ )
        {
          U32 startIndex = 0;
          const std::string& cur_res = result[ res_index ];
          std::string peerId   = cur_res.substr( startIndex, 
            cur_res.find( ':', startIndex ) - startIndex );
          startIndex += peerId.length() + 1;
          std::string peerHost = cur_res.substr( startIndex, 
            cur_res.find( ':', startIndex ) - startIndex );
          startIndex += peerHost.length() + 1;
          std::string peerPort = cur_res.substr( startIndex, std::string::npos );

        }
      }
      else
      {
        std::cout << "Error calling 'RequestPeers'\n\n";
      }
    }

    return Error::NO_ERROR;
  }

  Error::ErrorCode Flood::_SetupFilesAndChunks( void )
  {
    m_totalbytes = 0;

    FloodFile::V_File::const_iterator fileiter = m_floodfile.m_files.begin();
    FloodFile::V_File::const_iterator fileend  = m_floodfile.m_files.end();

    for ( ; fileiter != fileend; ++fileiter )
    {
      const FloodFile::File& file = *fileiter;

      // track the total size of the flood
      m_totalbytes += file.m_size;

      // setup runtime data
      RuntimeFile rtf;
      rtf.m_chunkoffsets = V_U32( file.m_chunks.size(), 0 );
      rtf.m_chunkmap = std::string( file.m_chunks.size(), '0' );

      FloodFile::V_Chunk::const_iterator chunkiter = file.m_chunks.begin();
      FloodFile::V_Chunk::const_iterator chunkend  = file.m_chunks.end();

      FILE* fileptr = fopen( file.m_name.c_str(), "rb" );

      U32 next_offset = 0;
      for ( ; chunkiter != chunkend; ++chunkiter )
      {
        const FloodFile::Chunk& chunk = *chunkiter;

        assert( rtf.m_chunkindices.find( chunk.m_hash ) == rtf.m_chunkindices.end() );
        assert( rtf.m_chunkoffsets[ chunk.m_index ] == 0 );
        assert( rtf.m_chunkmap[ chunk.m_index ] == '0' );

        rtf.m_chunkindices[ chunk.m_hash ]  = chunk.m_index;
        rtf.m_chunkoffsets[ chunk.m_index ] = next_offset;

        // if the file exists we need to see if this chunk is valid
        // allocate a buffer the size of the chunk and attempt to read in that data
        // if we can read it in and it hashes the same as what is in our flood file, 
        // we don't need to download it
        if ( fileptr )
        {
          if ( fseek( fileptr, next_offset, SEEK_SET ) == 0 )
          {
            byte* data = (byte*)malloc( chunk.m_size );
            if ( fread( data, 1, chunk.m_size, fileptr ) == chunk.m_size )
            {
              using namespace CryptoPP;
              SHA sha;
              HashFilter shaFilter(sha);
              std::auto_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
              channelSwitch->AddDefaultRoute(shaFilter);

              StringSource( data, chunk.m_size, true, channelSwitch.release() );
              std::stringstream out;
              Base64Encoder encoder( new FileSink( out ), false );
              shaFilter.TransferTo( encoder );

              if( chunk.m_hash.compare( out.str() ) == 0 )
              {
                rtf.m_chunkmap[ chunk.m_index ] = '1';
              }
            }
            free( data );
          }
        }

        // update the next offset
        next_offset += chunk.m_size;

        // do we need to download this chunk?  remember that somewhere
        if ( rtf.m_chunkmap[ chunk.m_index ] == '0' )
        {
          m_chunkstodownload.push_back( P_ChunkKey( m_runtimefiles.size(), chunk.m_index ) );
        }
      }

      // don't forget to close the file handle
      if ( fileptr )
      {
        fclose( fileptr );
      }

      // add this file data to our vector
      m_runtimefiles.push_back( rtf );
    }

    // done
    return Error::NO_ERROR;
  }
};

