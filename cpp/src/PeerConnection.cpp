#include "stdafx.H"
#include "PeerConnection.H"

namespace libBitFlood
{
  Error::ErrorCode PeerConnection::InitializeIncoming( SOCKET s,
                                                       const std::string& i_peerHost,
                                                       U32                i_peerPort )

  {
    bool ok = (s != INVALID_SOCKET);

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

    // cache relevant data (host, port)
    m_host = i_peerHost;
    m_port = i_peerPort;

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerConnection::InitializeOutgoing( const std::string& i_peerId,
    const std::string& i_peerHost,
    U32                i_peerPort )
  {
    // cache the data and attempt a connection
    m_socket = INVALID_SOCKET;
    m_id     = i_peerId;
    m_host   = i_peerHost;
    m_port   = i_peerPort;

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
};

