#include "stdafx.H"
#include "Socket.H"
#include <winsock2.h>
#include <Ws2tcpip.h>

namespace libBitFlood
{
  /*static*/ bool
  Socket::CreateSocket( SocketSPtr& o_socket )
  {
    Win32Socket* ret = new Win32Socket();
    o_socket = SocketSPtr( ret );
    return ret->m_handle != INVALID_SOCKET;
  }
  
  Win32Socket::Win32Socket( void )
  {
    m_handle = (U32)::socket(AF_INET, SOCK_STREAM, 0);
  }

  Win32Socket::~Win32Socket( void )
  {
    Close();
  }

  bool IsFatalError( void )
  {
    bool fatal = true;
    U32 error = WSAGetLastError();
    switch( error )
    {
      case WSAEINPROGRESS:
      case WSAEWOULDBLOCK:
      case EAGAIN:
      case EINTR:
        fatal = false;
    }

    return fatal;
  }
  

  bool Win32Socket::Connect( const std::string& i_host, U32 i_port ) 
  {
    bool success = false;
    
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    
    struct hostent *hp = gethostbyname( i_host.c_str() );
    if (hp != 0) 
    {
      saddr.sin_family = hp->h_addrtype;
      memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
      saddr.sin_port = htons( (U16) i_port);
      
      //
      U32 result = ::connect( (SOCKET)m_handle, (struct sockaddr*)&saddr, sizeof(saddr) );
      if ( result == 0 || !IsFatalError() )
      {
        success = true;
      }
    }

    return success;
  }

  void Win32Socket::Close( void )
  {
    closesocket( m_handle );
  }

  bool Win32Socket::Read( U32 i_windowsize, U8* o_buffer, I32& o_bytesread )
  {
    assert( i_windowsize > 0 );

    o_bytesread = ::recv( (SOCKET)m_handle, (char*)o_buffer, i_windowsize - 1, 0 );
    if ( o_bytesread > 0) 
    {
      o_buffer[ o_bytesread ] = 0;
    } 
    else if( WSAGetLastError() == WSAEWOULDBLOCK )
    {
      o_bytesread = -1;
    }

    return true;
  }

  bool Win32Socket::Write( U32 i_buffersize, const U8* i_buffer, I32& o_byteswritten )
  {
    if ( i_buffersize > 0 )
    {
      o_byteswritten = ::send( (SOCKET)m_handle, (const char*)i_buffer, i_buffersize, 0 );
      if ( o_byteswritten <= 0 && WSAGetLastError() == WSAEWOULDBLOCK )
      {
        o_byteswritten = -1;
      }
    }

    return true;
  }
  
  bool Win32Socket::SetNonBlocking( void )
  {
    U32 flag = 1;
    return ::ioctlsocket( (SOCKET)m_handle, FIONBIO, (u_long*)&flag ) == 0;
  }

  bool Win32Socket::SetReuse( void )
  {
    // Allow this port to be re-bound immediately so server re-starts are not delayed
    U32 flag = 1;
    return ::setsockopt( (SOCKET)m_handle, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag)) == 0;
  }
  
  bool Win32Socket::Bind( U32 i_port )
  {
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons((u_short) i_port);
    return ::bind( m_handle, (struct sockaddr *)&saddr, sizeof(saddr)) == 0;
  }

  bool Win32Socket::Listen( U32 i_backlog )
  {
    return ::listen( (SOCKET)m_handle, i_backlog ) == 0;
  }

  bool Win32Socket::Accept( SocketSPtr& o_socket, std::string& o_host, U32& o_port )
  {
    bool success = false;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);

    SOCKET incoming_handle = ::accept( (SOCKET)m_handle, (struct sockaddr*)&addr, &addrlen);
    if ( incoming_handle != INVALID_SOCKET )
    {
      success = true;
      Win32Socket* incoming_ptr = new Win32Socket();
      incoming_ptr->m_handle = (U32)incoming_handle;
      o_socket = SocketSPtr( incoming_ptr );

      char hostbuff[ NI_MAXHOST ];
      getnameinfo( (const sockaddr*)&addr, addrlen, hostbuff, NI_MAXHOST, NULL, 0, NI_NUMERICHOST );

      o_host = hostbuff;
      o_port = ntohs( addr.sin_port );
    }

    return success;
  }

  bool Win32Socket::Select( U32 i_timeout, bool& o_canread, bool& o_canwrite )
  {
    fd_set read, write;
    FD_ZERO( &read );
    FD_ZERO( &write );

    FD_SET( (SOCKET)m_handle, &read );
    FD_SET( (SOCKET)m_handle, &write );

    timeval timeout;
    timeout.tv_sec = timeout.tv_usec = i_timeout;

    bool ret = true;

    o_canread = false;
    o_canwrite = false;
    if ( ::select( 0, &read, &write, NULL, &timeout ) == SOCKET_ERROR && IsFatalError() )
    {
      ret = false;
    }
    else
    {
      o_canread = FD_ISSET( (SOCKET)m_handle, &read ) != 0;
      o_canwrite = FD_ISSET( (SOCKET)m_handle, &write ) != 0;
    }

    return ret;
  }
};

