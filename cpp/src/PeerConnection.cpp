#include "stdafx.H"

#include "PeerConnection.H"
#include "Flood.H"
#include "Client.H"
#include "Encoder.H"

#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>

namespace libBitFlood
{
  Error::ErrorCode PeerConnection::InitializeCommon( Client* i_client,
    const std::string& i_peerHost,
    U32                i_peerPort )

  {
    // cache relevant data (host, port, client)
    m_client = i_client;
    m_host   = i_peerHost;
    m_port   = i_peerPort;

    return Error::NO_ERROR_LBF;
  }


  Error::ErrorCode PeerConnection::InitializeIncoming( const SocketSPtr& i_socket )
  {
    bool ok = ( i_socket.Get() != NULL );

    // we have no id
    m_id.clear();

    // inherit the socket
    m_socket = i_socket;

    // make it non blocking
    m_socket->SetNonBlocking();

    // mark as connected
    m_connected = true;
    m_disconnected = false;

    // setup our reader & writer
    m_reader.m_socket = m_socket;
    m_writer.m_socket = m_socket;

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::InitializeOutgoing( const std::string& i_peerId )
  {
    // cache the data and attempt a connection
    m_socket = SocketSPtr( NULL );
    m_id     = i_peerId;

    // 
    m_connected = false;
    m_disconnected = false;

    //
    _Connect();

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::LoopOnce( void )
  {
    //
    if ( !m_disconnected && _Connect() == Error::NO_ERROR_LBF )
    {
      _ReadOnce();
      _WriteOnce();
      _ProcessReadBuffer();
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::SendMessage( const std::string& i_methodName,
                                                const XmlRpcValue& i_args )
  {
    printf( "%s >> %s\n", m_id.c_str(), i_methodName.c_str() );
    m_fakeClient.ExternalGenerateRequest( i_methodName.c_str(), i_args );

    std::string& request = m_fakeClient.AccessRequest();

    U32 request_index;
    U32 request_size = request.size();
    for ( request_index = 0; request_index < request_size; ++request_index )
    {
      if ( request[ request_index ] == '\n' ||
           request[ request_index ] == '\r' )
      {
        request[ request_index ] = ' ';
      }
    }
    
    m_writer.m_buffer.append( request );
    m_writer.m_buffer.append( 1, '\n' );
    m_writer.m_buffersize += request_size + 1;

    return Error::NO_ERROR_LBF;
  }

  /*
  Error::ErrorCode PeerConnection::Register( const std::string& i_floodhash,
    const std::string& i_clientid )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    args[1] = i_clientid;
    _SendMessage( PeerConnectionMessages::Register, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::RequestChunkMaps( const std::string& i_floodhash )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    _SendMessage( PeerConnectionMessages::RequestChunkMaps, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::SendChunkMaps( const std::string& i_floodhash )
  {
    // get the appropriate flood from the client
    FloodSPtr theflood;
    m_client->InqFlood( i_floodhash, theflood );

    // call SendChunkMap, args are:
    //  1: the floodId
    //  2: and array of strings, file name then the chunkmap for that file
    XmlRpcValue args;
    args[0] = i_floodhash;
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
    _SendMessage( PeerConnectionMessages::SendChunkMaps, args );
    return Error::NO_ERROR_LBF;
  }


  Error::ErrorCode PeerConnection::NotifyHaveChunk( const std::string& i_floodhash,
                                                    const std::string& i_filename,
                                                    U32                i_chunkindex )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    args[1] = i_filename;
    args[2] = (int)i_chunkindex;

    _SendMessage( PeerConnectionMessages::NotifyHaveChunk, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::RequestChunk( const std::string& i_floodhash,
                                                 const std::string& i_filename,
                                                 U32                i_chunkindex )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    args[1] = i_filename;
    args[2] = (int)i_chunkindex;

    _SendMessage( PeerConnectionMessages::RequestChunk, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::SendChunk( const std::string& i_floodhash,
    const std::string& i_filename,
    U32                i_chunkindex )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    args[1] = i_filename;
    args[2] = (int)i_chunkindex;

    // read in the appropriate chunk data and send it across the wire

    // get the appropriate flood from the client
    FloodSPtr theflood;
    m_client->InqFlood( i_floodhash, theflood );
    if ( theflood.Get() == NULL )
    {
    }
    else
    {
      FloodFile::File* thefile = NULL;
      U32 thefile_index;
      for ( thefile_index = 0; thefile_index < theflood->m_floodfile.m_files.size(); ++thefile_index )
      {
        if ( theflood->m_floodfile.m_files[ thefile_index ].m_name.compare( i_filename ) == 0 )
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
        const FloodFile::Chunk& thechunk = thefile->m_chunks[ i_chunkindex ];
        U32 chunkoffset = theflood->m_runtimefiles[ thefile_index ].m_chunkoffsets[ i_chunkindex ];

        FILE* fileptr = fopen( i_filename.c_str(), "rb" );
        if ( fileptr != NULL && fseek( fileptr, chunkoffset, SEEK_SET ) == 0 )
        {
          byte* data = (byte*)malloc( thechunk.m_size );
          if ( fread( data, 1, thechunk.m_size, fileptr ) == thechunk.m_size )
          {
            std::string test;
            Encoder::Base64Encode( data, thechunk.m_size, test );
            if( thechunk.m_hash.compare( test ) == 0 )
            {
              args[3] = XmlRpcValue( data, thechunk.m_size );

              _SendMessage( PeerConnectionMessages::SendChunk, args );
            }
          }
          free( data );
        }
        
        if ( fileptr != NULL )
          fclose( fileptr );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::_SetupMessageHandlers( void )
  {
    m_messageHandlers[ PeerConnectionMessages::Register ]         = _HandleRegister;
    m_messageHandlers[ PeerConnectionMessages::RequestChunkMaps ] = _HandleRequestChunkMaps;
    m_messageHandlers[ PeerConnectionMessages::SendChunkMaps ]    = _HandleSendChunkMaps;
    m_messageHandlers[ PeerConnectionMessages::NotifyHaveChunk ]  = _HandleNotifyHaveChunk;
    m_messageHandlers[ PeerConnectionMessages::RequestChunk ]     = _HandleRequestChunk;
    m_messageHandlers[ PeerConnectionMessages::SendChunk ]        = _HandleSendChunk;

    return Error::NO_ERROR_LBF;
  }
  */

  Error::ErrorCode PeerConnection::_Connect( void )
  {
    Error::ErrorCode retCode = Error::NO_ERROR_LBF;

    if ( !m_connected )
    {
      if ( m_socket.Get() == NULL )
      {
        Socket::CreateSocket( m_socket );

        // make it non-blocking
        m_socket->SetNonBlocking();

        // attempt a connection
        m_socket->Connect( m_host, m_port );
      }

      if ( m_socket->CanWrite( 0 ) )
      {
        // mark as connected
        m_connected = true;
        m_disconnected = false;
        
        // setup our reader & writer
        m_reader.m_socket = m_socket;
        m_writer.m_socket = m_socket;
        
        retCode = Error::NO_ERROR_LBF;
      }
      else
      {
        retCode = Error::UNKNOWN_ERROR_LBF;
      }
    }

    return retCode;
  }

  Error::ErrorCode PeerConnection::_ReadOnce( void )
  {
    I32 bytesRead;
    m_reader.Read( bytesRead );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::_WriteOnce( void )
  {
    I32 bytesWritten;
    m_writer.Write( bytesWritten );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::_ProcessReadBuffer( void )
  {
    while( m_reader.m_buffersize > m_reader.m_lasttail )
    {
      m_reader.m_lasttail = m_reader.m_buffer.find( '\n', m_reader.m_lasttail );
      if ( m_reader.m_lasttail == std::string::npos )
      {
        m_reader.m_lasttail = m_reader.m_buffersize;
        break;
      }

      m_fakeServer.SetRequest( m_reader.m_buffer.substr( 0, m_reader.m_lasttail ) );
      m_reader.m_buffer = m_reader.m_buffer.substr( m_reader.m_lasttail + 1, m_reader.m_buffersize );

      m_reader.m_buffersize = m_reader.m_buffersize - ( m_reader.m_lasttail + 1 );
      m_reader.m_lasttail = 0;

      // parse and dispatch the message
      XmlRpcValue args;
      std::string methodName = m_fakeServer.ExternalParseRequest( args );

      m_client->DispatchMessage( methodName, PeerConnectionSPtr( this ), args );
    } 

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::Reader::Read( I32& o_bytesRead )
  {
    const U32 READ_SIZE = 128 * 1024;
    U8 buffer[ READ_SIZE ];

    m_socket->Read( READ_SIZE, buffer, o_bytesRead );
    if ( o_bytesRead > 0 )
    {
      m_buffer.append( (char*)buffer, o_bytesRead );
      m_buffersize += o_bytesRead;
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::Writer::Write( I32& o_bytesWritten )
  {
    m_socket->Write( m_buffersize, (const U8*)m_buffer.data(), o_bytesWritten );
    if ( o_bytesWritten > 0 )
    {
      m_buffer = m_buffer.substr( o_bytesWritten, m_buffersize );
      m_buffersize -= o_bytesWritten;
    } 

    return Error::NO_ERROR_LBF;
  }

  /*
  Error::ErrorCode _HandleRegister( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string floodId = i_args[0];
    std::string peerId = i_args[1];

    // see if the peer is already registered
    if ( !peerId.empty() )
    {
      PeerConnectionSPtr peer;
      i_receiver.m_client->InqPeer( peerId, peer );
      if ( peer.Get() != NULL )
      {
        // disconnect duplicate peers
        peer->m_disconnected = true;
      }
      else if ( i_receiver.m_registeredFloods.find( floodId ) != i_receiver.m_registeredFloods.end() )
      {
        // strange duplicate registration from a peer for a given flood
      }
      else
      {
        i_receiver.m_id = peerId;
        i_receiver.RequestChunkMaps( floodId );
        i_receiver.m_registeredFloods.insert( floodId );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode _HandleRequestChunkMaps( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodId = i_args[0];

    i_receiver.SendChunkMaps( floodId );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode _HandleSendChunkMaps( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    // we'll get an array of 
    std::string& floodId = i_args[0];

    if ( !floodId.empty() )
    {
      PeerConnection::M_StrToStrToStr::iterator iter = i_receiver.m_chunkMaps.find( floodId );
      if ( iter == i_receiver.m_chunkMaps.end() )
      {
        iter = i_receiver.m_chunkMaps.insert( PeerConnection::M_StrToStrToStr::value_type( floodId, PeerConnection::M_StrToStr() ) ).first;
      }

      U32 arg_index;
      for ( arg_index = 0; arg_index < i_args[1].size(); arg_index += 2 )
      {
        (*iter).second[ i_args[1][arg_index] ] = i_args[1][ arg_index + 1 ];
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode _HandleNotifyHaveChunk( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodId = i_args[0];
    std::string& filename = i_args[1];
    U32 chunkindex = (int)i_args[2];
 
    PeerConnection::M_StrToStrToStr::iterator floodchunkmap = i_receiver.m_chunkMaps.find( floodId );
    if ( floodchunkmap != i_receiver.m_chunkMaps.end() )
    {
      PeerConnection::M_StrToStr::iterator filechunkmap = (*floodchunkmap).second.find( filename );
      if( filechunkmap != (*floodchunkmap).second.end() )
      {
        std::string& chunkmap = (*filechunkmap).second;
        chunkmap[ chunkindex ] = '1';
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode _HandleRequestChunk( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodId = i_args[0];
    std::string& filename = i_args[1];
    U32 chunkindex = (int)i_args[2];

    i_receiver.SendChunk( floodId, filename, chunkindex );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode _HandleSendChunk( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodId = i_args[0];
    std::string& filename = i_args[1];
    U32 chunkindex = (int)i_args[2];
    XmlRpcValue::BinaryData& data = i_args[3];

    // get the appropriate flood from the client
    FloodSPtr theflood;
    i_receiver.m_client->InqFlood( floodId, theflood );
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
          byte* data_ptr = (byte*)malloc( data_size );
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
              if ( fwrite( data_ptr, 1, data_size, fileptr ) != data_size )
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

              theflood->m_chunksDownloading.erase( chunkkey );

              i_receiver.NotifyHaveChunk( floodId, filename, chunkindex );
            }
            
            if ( fileptr != NULL )
              fclose( fileptr );
          }

          free( data_ptr );
        }
      }
    }


    return Error::NO_ERROR_LBF;
  }
  */
};

