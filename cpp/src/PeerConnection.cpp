#include "stdafx.H"

#include "PeerConnection.H"
#include "Flood.H"
#include "Peer.H"
#include "Encoder.H"
#include "PeerMethods.H"
#include "ChunkMethods.H"

#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>

namespace libBitFlood
{
  Error::ErrorCode PeerConnection::InitializeCommon( PeerSPtr&          i_localpeer,
                                                     const std::string& i_peerhost,
                                                     U32                i_peerport )

  {
    // cache relevant data (host, port, localpeer)
    m_localpeer = i_localpeer.Get();
    m_host      = i_peerhost;
    m_port      = i_peerport;
    m_flood     = NULL;
    m_fakeServer = new FakeServer();

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

  Error::ErrorCode PeerConnection::InitializeOutgoing( FloodSPtr&         i_flood,
                                                       const std::string& i_peerid )
  {
    // cache the flood pointer
    m_flood = i_flood.Get();

    // cache the data and attempt a connection
    m_socket = SocketSPtr( NULL );
    m_id     = i_peerid;

    // 
    m_connected = false;
    m_disconnected = false;

    //
    _Connect();

    // register ourselves w/ the peer
    {
      XmlRpcValue args;
      args[0] = m_flood->m_floodfile->m_contentHash;
      args[1] = m_localpeer->m_localid;
      args[2] = (int)m_localpeer->m_localport;
      SendMethod( PeerMethodHandler::Register, args );
    }

    // request some chunks
    {
      XmlRpcValue args;
      SendMethod( ChunkMethodHandler::RequestChunkMaps, args );
    }

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

  Error::ErrorCode PeerConnection::SendMethod( const std::string& i_methodName,
                                                const XmlRpcValue& i_args )
  {
    printf( "%s -> %s (%s)\n", m_localpeer->m_localid.c_str(), m_id.c_str(), i_methodName.c_str() );
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

      m_fakeServer->SetRequest( m_reader.m_buffer.substr( 0, m_reader.m_lasttail ) );
      m_reader.m_buffer = m_reader.m_buffer.substr( m_reader.m_lasttail + 1, m_reader.m_buffersize );

      m_reader.m_buffersize = m_reader.m_buffersize - ( m_reader.m_lasttail + 1 );
      m_reader.m_lasttail = 0;

      // parse and dispatch the method
      XmlRpcValue args;
      std::string methodname = m_fakeServer->ExternalParseRequest( args );
      
      printf( "%s <- %s (%s)\n", m_localpeer->m_localid.c_str(), m_id.c_str(), methodname.c_str() );

      m_localpeer->HandleMethod( methodname, PeerConnectionSPtr( this ), args );
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
};

