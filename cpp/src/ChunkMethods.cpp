#include "stdafx.H"
#include "ChunkHandler.H"
#include "Encoder.H"
#include "Client.H"
#include "PeerConnection.H"
#include "Flood.H"

namespace libBitFlood
{
  // define our message names
  const char ChunkMessageHandler::RequestChunkMaps[] = "RequestChunkMaps";
  const char ChunkMessageHandler::SendChunkMaps[]    = "SendChunkMaps";
  const char ChunkMessageHandler::RequestChunk[]     = "RequestChunk";
  const char ChunkMessageHandler::SendChunk[]        = "SendChunk";
  const char ChunkMessageHandler::NotifyHaveChunk[]  = "NotifyHaveChunk";

  Error::ErrorCode ChunkMessageHandler::GetChunks( ClientSPtr& i_client )
  {
    bool foundchunk = false;
    PeerConnectionSPtr todownload_from;
    FloodSPtr          todownload_flood;
    Flood::P_ChunkKey* todownload_key = NULL;

    // figure out what chunk to get from which peer and ask for it
    V_FloodSPtr::iterator flooditer = i_client->m_floods.begin();
    V_FloodSPtr::iterator floodend  = i_client->m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      FloodSPtr& theflood = (*flooditer);

      Flood::V_ChunkKey::iterator chunkiter = theflood->m_chunkstodownload.begin();
      Flood::V_ChunkKey::iterator chunkend  = theflood->m_chunkstodownload.end();

      for ( ; chunkiter != chunkend; ++chunkiter )
      {
        Flood::P_ChunkKey& thechunkkey = (*chunkiter);
        if ( theflood->m_chunksdownloading.find( thechunkkey ) == theflood->m_chunksdownloading.end() )
        {
          const FloodFile::File& thefile = theflood->m_floodfile.m_files[ thechunkkey.first ];
          const FloodFile::Chunk& thechunk = thefile.m_chunks[ thechunkkey.second ];

          V_PeerConnectionSPtr::iterator peeriter = i_client->m_peers.begin();
          V_PeerConnectionSPtr::iterator peerend  = i_client->m_peers.end();

          for ( ; peeriter != peerend; ++peeriter )
          {
            PeerConnectionSPtr& thepeer = (*peeriter);
            M_StrToStrToStrToStr::iterator peerchunkmap = m_chunkMaps.find( thepeer->m_id );
            if ( peerchunkmap != m_chunkMaps.end() )
            {
              M_StrToStrToStr::iterator floodchunkmap = (*peerchunkmap).second.find( theflood->m_floodfile.m_contentHash );
              if ( floodchunkmap != (*peerchunkmap).second.end() )
              {
                PeerConnection::M_StrToStr::iterator filechunkmap = (*floodchunkmap).second.find( thefile.m_name );
                if( filechunkmap != (*floodchunkmap).second.end() )
                {
                  const std::string& chunkmap = (*filechunkmap).second;
                  if ( chunkmap[ thechunkkey.second ] == '1' )
                  {
                    foundchunk = true;
                    todownload_from  = thepeer;
                    todownload_flood = theflood;
                    todownload_key   = &thechunkkey;
                  }
                }
              }
            }
          }
        }
      }
    }

    if ( foundchunk )
    {
      XmlRpcValue args;
      args[0] = todownload_flood->m_floodfile.m_contentHash;
      args[1] = todownload_flood->m_floodfile.m_files[ todownload_key->first ].m_name;
      args[2] = (int)todownload_key->second;

      todownload_from->SendMessage( RequestChunk, args );
      todownload_flood->m_chunksdownloading.insert( *todownload_key );
    }

    return Error::NO_ERROR_LBF;
  }
    
  Error::ErrorCode ChunkMessageHandler::QueryAPI( V_String& o_supportedmessages )
  {
    o_supportedmessages.push_back( RequestChunkMaps );
    o_supportedmessages.push_back( SendChunkMaps );
    o_supportedmessages.push_back( RequestChunk );
    o_supportedmessages.push_back( SendChunk );
    o_supportedmessages.push_back( NotifyHaveChunk );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMessageHandler::HandleMessage( const std::string&  i_message, 
                                                       PeerConnectionSPtr& i_receiver,
                                                       XmlRpcValue&        i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_message.compare( RequestChunkMaps ) == 0 )
    {
      ret = _HandleRequestChunkMaps( i_receiver, i_args );
    }
    else if ( i_message.compare( SendChunkMaps ) == 0 )
    {
      ret = _HandleSendChunkMaps( i_receiver, i_args );
    }
    else if ( i_message.compare( RequestChunk ) == 0 )
    {
      ret = _HandleRequestChunk( i_receiver, i_args );
    }
    else if ( i_message.compare( SendChunk ) == 0 )
    {
      ret = _HandleSendChunk( i_receiver, i_args );
    }
    else if ( i_message.compare( NotifyHaveChunk ) == 0 )
    {
      ret = _HandleNotifyHaveChunk( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode ChunkMessageHandler::_HandleRequestChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& floodhash = i_args[0];

    // get the appropriate flood from the client
    FloodSPtr theflood;
    i_receiver->m_client->InqFlood( floodhash, theflood );

    // call SendChunkMap, args are:
    //  1: the floodId
    //  2: and array of strings, file name then the chunkmap for that file
    XmlRpcValue args;
    args[0] = floodhash;
    args[1];

    if ( theflood.Get() != NULL )
    {
      U32 argindex = 0;
      U32 fileindex;
      for ( fileindex = 0; fileindex < theflood->m_runtimefiles.size(); ++fileindex )
      {
        const Flood::RuntimeFile& rtf = theflood->m_runtimefiles[fileindex];
        const FloodFile::File& fileinfo = theflood->m_floodfile.m_files[fileindex];

        args[1][argindex++] = fileinfo.m_name;
        args[1][argindex++] = rtf.m_chunkmap;
      }
    }

    // send the message
    i_receiver->SendMessage( SendChunkMaps, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMessageHandler::_HandleSendChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    // we'll get an array of 
    const std::string& floodId = i_args[0];
    const std::string& peerId = i_receiver->m_id;

    if ( !peerId.empty() )
    {
      M_StrToStrToStrToStr::iterator peeriter = m_chunkMaps.find( peerId );
      if ( peeriter == m_chunkMaps.end() )
      {
        peeriter = m_chunkMaps.insert( M_StrToStrToStrToStr::value_type( peerId, M_StrToStrToStr() ) ).first;
      }

      if ( !floodId.empty() )
      {
        M_StrToStrToStr::iterator iter = (*peeriter).second.find( floodId );
        if ( iter == (*peeriter).second.end() )
        {
          iter = (*peeriter).second.insert( M_StrToStrToStr::value_type( floodId, M_StrToStr() ) ).first;
        }
        
        U32 arg_index;
        for ( arg_index = 0; arg_index < i_args[1].size(); arg_index += 2 )
        {
          (*iter).second[ i_args[1][arg_index] ] = i_args[1][ arg_index + 1 ];
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMessageHandler::_HandleRequestChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodhash = i_args[0];
    std::string& filename = i_args[1];
    U32 chunkindex = (int)i_args[2];

    XmlRpcValue args;
    args[0] = floodhash;
    args[1] = filename;
    args[2] = (int)chunkindex;

    // read in the appropriate chunk data and send it across the wire

    // get the appropriate flood from the client
    FloodSPtr theflood;
    i_receiver->m_client->InqFlood( floodhash, theflood );
    if ( theflood.Get() == NULL )
    {
    }
    else
    {
      FloodFile::File* thefile = NULL;
      U32 thefile_index;
      for ( thefile_index = 0; thefile_index < theflood->m_floodfile.m_files.size(); ++thefile_index )
      {
        if ( theflood->m_floodfile.m_files[ thefile_index ].m_name.compare( filename ) == 0 )
        {
          thefile = &theflood->m_floodfile.m_files[ thefile_index ];
          break;
        }
      }

      if ( thefile == NULL )
      {
      }
      else
      {
        const FloodFile::Chunk& thechunk = thefile->m_chunks[ chunkindex ];
        U32 chunkoffset = theflood->m_runtimefiles[ thefile_index ].m_chunkoffsets[ chunkindex ];

        FILE* fileptr = fopen( filename.c_str(), "rb" );
        if ( fileptr != NULL && fseek( fileptr, chunkoffset, SEEK_SET ) == 0 )
        {
          U8* data = (U8*)malloc( thechunk.m_size );
          if ( fread( (char*)data, 1, thechunk.m_size, fileptr ) == thechunk.m_size )
          {
            std::string test;
            Encoder::Base64Encode( data, thechunk.m_size, test );
            if( thechunk.m_hash.compare( test ) == 0 )
            {
              args[3] = XmlRpcValue( data, thechunk.m_size );

              i_receiver->SendMessage( SendChunk, args );
            }
          }
          free( data );
        }
        
        if ( fileptr != NULL )
        {
          fclose( fileptr );
        }
      }
    }

    
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMessageHandler::_HandleSendChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodhash = i_args[0];
    std::string& filename = i_args[1];
    U32 chunkindex = (int)i_args[2];
    XmlRpcValue::BinaryData& data = i_args[3];

    // get the appropriate flood from the client
    FloodSPtr theflood;
    i_receiver->m_client->InqFlood( floodhash, theflood );
    if ( theflood.Get() == NULL )
    {
    }
    else
    {
      FloodFile::File* thefile = NULL;
      U32 thefile_index;
      for ( thefile_index = 0; thefile_index < theflood->m_floodfile.m_files.size(); ++thefile_index )
      {
        if ( theflood->m_floodfile.m_files[ thefile_index ].m_name.compare( filename ) == 0 )
        {
          thefile = &theflood->m_floodfile.m_files[ thefile_index ];
          break;
        }
      }

      if ( thefile == NULL )
      {
      }
      else
      {
        const FloodFile::Chunk& thechunk = thefile->m_chunks[ chunkindex ];
        U32 chunkoffset = theflood->m_runtimefiles[ thefile_index ].m_chunkoffsets[ chunkindex ];
        
        U32 data_size = data.size();
        
        if ( thechunk.m_size == data_size )
        {
          U32 data_index;
          U8* data_ptr = (U8*)malloc( data_size );
          for ( data_index = 0; data_index < data_size; ++data_index )
          {
            data_ptr[data_index] = data[data_index];
          }

          std::string test;
          Encoder::Base64Encode( data_ptr, data_size, test );
          if( thechunk.m_hash.compare( test ) == 0 )
          {
            FILE* fileptr = fopen( filename.c_str(), "r+b" );
            if ( fileptr == NULL )
            {
              fileptr = fopen( filename.c_str(), "w+b" );
            }

            if ( fileptr != NULL && fseek( fileptr, chunkoffset, SEEK_SET ) == 0 )
            {
              if ( fwrite( (const char*)data_ptr, 1, data_size, fileptr ) != data_size )
              {
              }

              Flood::P_ChunkKey chunkkey( thefile_index, chunkindex );
              theflood->m_runtimefiles[ thefile_index ].m_chunkmap[ chunkindex ] = '1';

              Flood::V_ChunkKey::iterator chunkiter = theflood->m_chunkstodownload.begin();
              Flood::V_ChunkKey::iterator chunkend  = theflood->m_chunkstodownload.end();

              for ( ; chunkiter != chunkend; ++chunkiter )
              {
                if ( (*chunkiter) == chunkkey )
                {
                  theflood->m_chunkstodownload.erase( chunkiter );
                  break;
                }
              }

              theflood->m_chunksdownloading.erase( chunkkey );

              XmlRpcValue args;
              args[0] = floodhash;
              args[1] = filename;
              args[2] = (int)chunkindex;
              i_receiver->SendMessage( NotifyHaveChunk, args );
            }
            
            if ( fileptr != NULL )
            {
              fclose( fileptr );
            }
          }

          free( data_ptr );
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMessageHandler::_HandleNotifyHaveChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& floodhash = i_args[0];
    const std::string& filename = i_args[1];
    U32 chunkindex = (int)i_args[2];

    M_StrToStrToStrToStr::iterator peerchunkmap = m_chunkMaps.find( i_receiver->m_id );
    if ( peerchunkmap != m_chunkMaps.end() )
    {
      M_StrToStrToStr::iterator floodchunkmap = (*peerchunkmap).second.find( floodhash );
      if ( floodchunkmap != (*peerchunkmap).second.end() )
      {
        M_StrToStr::iterator filechunkmap = (*floodchunkmap).second.find( filename );
        if( filechunkmap != (*floodchunkmap).second.end() )
        {
          std::string& chunkmap = (*filechunkmap).second;
          chunkmap[ chunkindex ] = '1';
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }
};

