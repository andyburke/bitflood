#include "stdafx.H"

#include "PeerConnection.H"
#include "Flood.H"
#include "Client.H"

namespace libBitFlood
{
  namespace PeerConnectionMessages
  {
    const char* Register         = "Register";
    const char* RequestChunkMaps = "RequestChunkMaps";
    const char* SendChunkMaps    = "SendChunkMaps";
    const char* NotifyHaveChunk  = "NotifyHaveChunk";
    const char* RequestChunk     = "RequestChunk";
    const char* SendChunk        = "SendChunk";
  }

  // forward declare our message handlers
  Error::ErrorCode _HandleRegister( PeerConnection& i_receiver, XmlRpcValue& i_args );
  Error::ErrorCode _HandleRequestChunkMaps( PeerConnection& i_receiver, XmlRpcValue& i_args );
  Error::ErrorCode _HandleSendChunkMaps( PeerConnection& i_receiver, XmlRpcValue& i_args );
  Error::ErrorCode _HandleNotifyHaveChunk( PeerConnection& i_receiver, XmlRpcValue& i_args );
  Error::ErrorCode _HandleRequestChunk( PeerConnection& i_receiver, XmlRpcValue& i_args );
  Error::ErrorCode _HandleSendChunk( PeerConnection& i_receiver, XmlRpcValue& i_args );

  Error::ErrorCode PeerConnection::InitializeCommon( Client* i_client,
    const std::string& i_peerHost,
    U32                i_peerPort )

  {
    // cache relevant data (host, port, client)
    m_client = i_client;
    m_host   = i_peerHost;
    m_port   = i_peerPort;

    // setup our message handlers
    _SetupMessageHandlers();

    return Error::NO_ERROR_LBF;
  }


  Error::ErrorCode PeerConnection::InitializeIncoming( SOCKET s )
  {
    bool ok = (s != INVALID_SOCKET);

    // we have no id
    m_id.clear();

    // inherit the socket
    m_socket = s;

    // make it non blocking
    U32 flag = 1;
    ok = ioctlsocket( m_socket, FIONBIO, (u_long*)&flag ) == 0;

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
    m_socket = INVALID_SOCKET;
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

  Error::ErrorCode PeerConnection::Register( const std::string i_floodhash,
    const std::string i_clientid )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    args[1] = i_clientid;
    _SendMessage( PeerConnectionMessages::Register, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::RequestChunkMaps( const std::string i_floodhash )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    _SendMessage( PeerConnectionMessages::RequestChunkMaps, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::SendChunkMaps( const std::string i_floodhash )
  {
    // get the appropriate flood from the client
    Flood* theflood = NULL;
    m_client->InqFlood( i_floodhash, theflood );

    // call SendChunkMap, args are:
    //  1: the floodId
    //  2: and array of strings, file name then the chunkmap for that file
    XmlRpcValue args;
    args[0] = i_floodhash;
    args[1];

    if ( theflood != NULL )
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


  Error::ErrorCode PeerConnection::NotifyHaveChunk( const std::string i_floodhash )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    _SendMessage( PeerConnectionMessages::NotifyHaveChunk, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::RequestChunk( const std::string i_floodhash )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    _SendMessage( PeerConnectionMessages::RequestChunk, args );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::SendChunk( const std::string i_floodhash )
  {
    XmlRpcValue args;
    args[0] = i_floodhash;
    _SendMessage( PeerConnectionMessages::SendChunk, args );

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

  Error::ErrorCode PeerConnection::_Connect( void )
  {
    Error::ErrorCode retCode = Error::NO_ERROR_LBF;

    if ( !m_connected )
    {
      if ( m_socket == INVALID_SOCKET )
      {
        m_socket = ::socket(AF_INET, SOCK_STREAM, 0);

        // make it non-blocking
        u_long flag = 1;
        ioctlsocket( m_socket, FIONBIO, &flag);

        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;

        struct hostent *hp = gethostbyname( m_host.c_str() );
        if (hp != 0) 
        {
          saddr.sin_family = hp->h_addrtype;
          memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
          saddr.sin_port = htons( (U16) m_port);

          //
          U32 result = ::connect( m_socket, (struct sockaddr*)&saddr, sizeof(saddr));
          U32 error = WSAGetLastError();
          int foo = 1;
        }
      }

      {
        fd_set set;
        FD_ZERO( &set );
        FD_SET( m_socket, &set );
        timeval timeout;
        timeout.tv_sec = 0;
        U32 num = ::select( 0, NULL, &set, NULL, &timeout ); 
        if ( num == 1 )
        {
          // mark as connected
          m_connected = true;
          m_disconnected = false;

          // setup our reader & writer
          m_reader.m_socket = m_socket;
          m_writer.m_socket = m_socket;

          retCode = Error::NO_ERROR_LBF;
        }
        else if ( num == SOCKET_ERROR )
        {
          retCode = Error::UNKNOWN_ERROR_LBF;

          U32 error = WSAGetLastError();
          switch ( error )
          {
          case WSAEINPROGRESS:
            break;

          default:
            break;
          }
        }
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

    while( !m_reader.m_buffer.empty() )
    {
      U32 tail = m_reader.m_buffer.find( '\n', 0 );
      if ( tail == std::string::npos )
      {
        break;
      }

      m_fakeServer.SetRequest( m_reader.m_buffer.substr( 0, tail ) );
      m_reader.m_buffer = m_reader.m_buffer.substr( tail + 1, std::string::npos );

      // parse and dispatch the message
      XmlRpcValue args;
      std::string methodName = m_fakeServer.ExternalParseRequest( args );

      _DispatchMessage( methodName, args );
    } 

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::_SendMessage( const std::string& i_methodName,
    const XmlRpcValue& i_args )
  {
    m_fakeClient.ExternalGenerateRequest( i_methodName.c_str(), i_args );

    // kill the newlines
    std::string request = m_fakeClient.GetRequest();
    U32 index = request.find_first_of( "\r\n" );
    while( index != std::string::npos )
    {
      request.erase( index, 1 );
      index = request.find_first_of( "\r\n" );
    }

    m_writer.m_buffer.append( request );
    m_writer.m_buffer.append( 1, '\n' );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::_DispatchMessage( const std::string& i_methodName,
    XmlRpcValue& i_args )
  {
    M_StrToMessageHandler::iterator iter = m_messageHandlers.find( i_methodName );
    if ( iter != m_messageHandlers.end() )
    {
      // call the function
      (*(*iter).second)( *this, i_args );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::Reader::Read( I32& o_bytesRead )
  {
    // Number of bytes to attempt to read at a time
    const U32 READ_SIZE = 4096;
    char readBuf[READ_SIZE];

    o_bytesRead = ::recv( m_socket, readBuf, READ_SIZE-1, 0);
    if ( o_bytesRead > 0) 
    {
      readBuf[ o_bytesRead ] = 0;
      m_buffer.append( readBuf, o_bytesRead );
    } 
    else if( WSAGetLastError() == WSAEWOULDBLOCK )
    {
      o_bytesRead = -1;
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::Writer::Write( I32& o_bytesWritten )
  {
    U32 towrite = (U32)m_buffer.length();

    if ( towrite > 0 )
    {
      o_bytesWritten = ::send( m_socket, m_buffer.data(), towrite, 0 );
      if ( o_bytesWritten > 0 ) 
      {
        m_buffer = m_buffer.substr( o_bytesWritten, std::string::npos );
      } 
      else if( WSAGetLastError() == WSAEWOULDBLOCK )
      {
        o_bytesWritten = -1;
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode _HandleRegister( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string floodId = i_args[0];
    std::string peerId = i_args[1];

    // see if the peer is already registered
    if ( !peerId.empty() )
    {
      PeerConnection* peer = NULL;
      i_receiver.m_client->InqPeer( peerId, peer );
      if ( peer == NULL )
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
    i_receiver.SendChunkMaps( i_args[0] );

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
    return Error::NO_ERROR_LBF;
  }
  Error::ErrorCode _HandleRequestChunk( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodId = i_args[0];
    return Error::NO_ERROR_LBF;
  }
  Error::ErrorCode _HandleSendChunk( PeerConnection& i_receiver, XmlRpcValue& i_args )
  {
    std::string& floodId = i_args[0];
    return Error::NO_ERROR_LBF;
  }
};

