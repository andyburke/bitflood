#include "stdafx.H"
#include "ChunkMethods.H"
#include "Encoder.H"
#include "Peer.H"
#include "PeerConnection.H"
#include "Flood.H"

namespace libBitFlood
{
  // define our method names
  const char ChunkMethodHandler::RequestChunkMaps[] = "RequestChunkMaps";
  const char ChunkMethodHandler::SendChunkMaps[]    = "SendChunkMaps";
  const char ChunkMethodHandler::RequestChunk[]     = "RequestChunk";
  const char ChunkMethodHandler::SendChunk[]        = "SendChunk";
  const char ChunkMethodHandler::NotifyHaveChunk[]  = "NotifyHaveChunk";
    
  Error::ErrorCode ChunkMethodHandler::HandleMethod( const std::string&  i_method, 
                                                       PeerConnectionSPtr& i_receiver,
                                                       XmlRpcValue&        i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_method.compare( RequestChunkMaps ) == 0 )
    {
      ret = _HandleRequestChunkMaps( i_receiver, i_args );
    }
    else if ( i_method.compare( SendChunkMaps ) == 0 )
    {
      ret = _HandleSendChunkMaps( i_receiver, i_args );
    }
    else if ( i_method.compare( RequestChunk ) == 0 )
    {
      ret = _HandleRequestChunk( i_receiver, i_args );
    }
    else if ( i_method.compare( SendChunk ) == 0 )
    {
      ret = _HandleSendChunk( i_receiver, i_args );
    }
    else if ( i_method.compare( NotifyHaveChunk ) == 0 )
    {
      ret = _HandleNotifyHaveChunk( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode ChunkMethodHandler::_HandleRequestChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    assert( i_receiver->m_flood != NULL );

    // call SendChunkMap
    XmlRpcValue args;
    U32 argindex = 0;
    U32 fileindex;
    for ( fileindex = 0; fileindex < i_receiver->m_flood->m_runtimefiles.size(); ++fileindex )
    {
      const Flood::RuntimeFile& rtf = i_receiver->m_flood->m_runtimefiles[fileindex];
      const FloodFile::File& fileinfo = i_receiver->m_flood->m_floodfile->m_files[fileindex];
      
      args[argindex++] = fileinfo.m_name;
      args[argindex++] = rtf.m_chunkmap;
    }

    // send the method
    i_receiver->SendMethod( SendChunkMaps, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMethodHandler::_HandleSendChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    assert( i_receiver->m_flood != NULL );

    // 
    const std::string& peerid = i_receiver->m_id;

    if ( !peerid.empty() )
    {
      Flood::M_StrToStrToStr::iterator peeriter = i_receiver->m_flood->m_peerchunkmaps.find( peerid );
      if ( peeriter == i_receiver->m_flood->m_peerchunkmaps.end() )
      {
        peeriter = i_receiver->m_flood->m_peerchunkmaps.insert( Flood::M_StrToStrToStr::value_type( peerid, Flood::M_StrToStr() ) ).first;
      }

      U32 arg_index;
      for ( arg_index = 0; arg_index < i_args.size(); arg_index += 2 )
      {
        (*peeriter).second[ i_args[arg_index] ] = i_args[ arg_index + 1 ];
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMethodHandler::_HandleRequestChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    assert( i_receiver->m_flood != NULL );

    std::string& filename = i_args[0];
    U32 chunkindex = (int)i_args[1];

    XmlRpcValue args;
    args[0] = filename;
    args[1] = (int)chunkindex;

    // read in the appropriate chunk data and send it across the wire
    FloodFile::File* thefile = NULL;
    U32 thefile_index;
    for ( thefile_index = 0; thefile_index < i_receiver->m_flood->m_floodfile->m_files.size(); ++thefile_index )
    {
      if ( i_receiver->m_flood->m_floodfile->m_files[ thefile_index ].m_name.compare( filename ) == 0 )
      {
        thefile = &i_receiver->m_flood->m_floodfile->m_files[ thefile_index ];
        break;
      }
    }

    if ( thefile == NULL )
    {
    }
    else
    {
      const FloodFile::Chunk& thechunk = thefile->m_chunks[ chunkindex ];
      U32 chunkoffset = i_receiver->m_flood->m_runtimefiles[ thefile_index ].m_chunkoffsets[ chunkindex ];

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
            args[2] = XmlRpcValue( data, thechunk.m_size );
            
            i_receiver->SendMethod( SendChunk, args );
          }
        }
        free( data );
      }
        
      if ( fileptr != NULL )
      {
        fclose( fileptr );
      }
    }
    
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMethodHandler::_HandleSendChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    assert( i_receiver->m_flood != NULL );

    std::string& filename = i_args[0];
    U32 chunkindex = (int)i_args[1];
    XmlRpcValue::BinaryData& data = i_args[2];

    FloodFile::File* thefile = NULL;
    U32 thefile_index;
    for ( thefile_index = 0; thefile_index < i_receiver->m_flood->m_floodfile->m_files.size(); ++thefile_index )
    {
      if ( i_receiver->m_flood->m_floodfile->m_files[ thefile_index ].m_name.compare( filename ) == 0 )
      {
        thefile = &i_receiver->m_flood->m_floodfile->m_files[ thefile_index ];
        break;
      }
    }
    
    if ( thefile == NULL )
    {
    }
    else
    {
      const FloodFile::Chunk& thechunk = thefile->m_chunks[ chunkindex ];
      U32 chunkoffset = i_receiver->m_flood->m_runtimefiles[ thefile_index ].m_chunkoffsets[ chunkindex ];
      
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
            i_receiver->m_flood->m_runtimefiles[ thefile_index ].m_chunkmap[ chunkindex ] = '1';
            
            Flood::V_ChunkKey::iterator chunkiter = i_receiver->m_flood->m_chunkstodownload.begin();
            Flood::V_ChunkKey::iterator chunkend  = i_receiver->m_flood->m_chunkstodownload.end();
            
            for ( ; chunkiter != chunkend; ++chunkiter )
            {
              if ( (*chunkiter) == chunkkey )
              {
                i_receiver->m_flood->m_chunkstodownload.erase( chunkiter );
                break;
              }
            }
            
            i_receiver->m_flood->m_chunksdownloading.erase( chunkkey );
            
            XmlRpcValue args;
            args[0] = filename;
            args[1] = (int)chunkindex;
            i_receiver->SendMethod( NotifyHaveChunk, args );
          }
            
          if ( fileptr != NULL )
          {
            fclose( fileptr );
          }
        }

        free( data_ptr );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode ChunkMethodHandler::_HandleNotifyHaveChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    assert( i_receiver->m_flood != NULL );

    const std::string& filename = i_args[0];
    U32 chunkindex = (int)i_args[1];

    Flood::M_StrToStrToStr::iterator peerchunkmap = i_receiver->m_flood->m_peerchunkmaps.find( i_receiver->m_id );
    if ( peerchunkmap != i_receiver->m_flood->m_peerchunkmaps.end() )
    {
      Flood::M_StrToStr::iterator filechunkmap = (*peerchunkmap).second.find( filename );
      if( filechunkmap != (*peerchunkmap).second.end() )
      {
        std::string& chunkmap = (*filechunkmap).second;
        chunkmap[ chunkindex ] = '1';
      }
    }
    
    return Error::NO_ERROR_LBF;
  }
};

