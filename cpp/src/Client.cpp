#include "stdafx.H"
#include "Client.H"
#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>
#include <Ws2tcpip.h>

namespace libBitFlood
{
  Error::ErrorCode Client::Initialize( const Setup& i_setup )
  {
    // cache the setup
    m_setup = i_setup;

    // compute an ID
    {
      std::stringstream idstrm;
      idstrm << m_setup.m_localIP << m_setup.m_localPort;

      using namespace CryptoPP;
      SHA sha;
      HashFilter shaFilter(sha);
      std::auto_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
      channelSwitch->AddDefaultRoute(shaFilter);

      StringSource( (const byte*)idstrm.str().c_str(), idstrm.str().length(), true, channelSwitch.release() );
      std::stringstream out;
      Base64Encoder encoder( new FileSink( out ), false );
      shaFilter.TransferTo( encoder );

      m_id = out.str();
    }

    // Open a listen socket
    _OpenListenSocket();

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::AddFloodFile( const FloodFile& i_floodfile )
  {
    Flood f;
    f.Initialize( i_floodfile );
    m_floods.push_back( f );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::Register( void )
  {
    V_Flood::iterator flooditer = m_floods.begin();
    V_Flood::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      Flood& flood = *flooditer;
      flood.Register( *this );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::UpdatePeerList( void )
  {
    V_Flood::iterator flooditer = m_floods.begin();
    V_Flood::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      Flood& flood = *flooditer;
      flood.UpdatePeerList( *this );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::GetChunks( void )
  {
    V_Flood::const_iterator flooditer = m_floods.begin();
    V_Flood::const_iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::LoopOnce( void )
  {
    _ProcessPeers();

    // if socket can read
    fd_set set;
    FD_ZERO( &set );
    FD_SET( m_listensocket, &set );
    timeval timeout;
    timeout.tv_sec = 0;
    U32 num = ::select( 0, &set, NULL, NULL, &timeout ); 
    if ( num == 1 )
    {
      // if socked accept, add incoming connection
      struct sockaddr_in addr;
      int addrlen = sizeof( addr );
      SOCKET incoming = ::accept( m_listensocket, (struct sockaddr*)&addr, &addrlen);
      U32 error = WSAGetLastError();

      if ( incoming != INVALID_SOCKET )
      {
        char hostbuff[ NI_MAXHOST ];
        getnameinfo( (const sockaddr*)&addr, addrlen, hostbuff, NI_MAXHOST, NULL, 0, NI_NUMERICHOST );

        PeerConnection* peer = new PeerConnection();
        peer->InitializeIncoming( incoming, hostbuff, ntohs( addr.sin_port ) );
        m_peers.push_back( peer );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::_OpenListenSocket( void )
  {
    bool ok = true;

    // create the socket
    m_listensocket = ::socket( AF_INET, SOCK_STREAM, 0 );

    // 
    ok = m_listensocket != INVALID_SOCKET;

    // make it non-blocking
    u_long flag = 1;
    ok = ioctlsocket( m_listensocket, FIONBIO, &flag) == 0;

    // turn on reuse
    int sflag = 1;
    ok = setsockopt( m_listensocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&sflag, sizeof(sflag) ) == 0;

    // bind to the port
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons((u_short) m_setup.m_localPort );
    ok = ::bind( m_listensocket, (struct sockaddr *)&saddr, sizeof(saddr) ) == 0;

    // tell the socket to listen
    ok = ::listen( m_listensocket,  5 ) == 0;

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::_ProcessPeers( void )
  {
    V_PeerConnectionPtr::iterator peeriter = m_peers.begin();
    V_PeerConnectionPtr::iterator peerend  = m_peers.end();

    for ( ; peeriter != peerend; )
    {
      (*peeriter)->LoopOnce();

      // reap defunct peer connections
      if ( (*peeriter)->m_disconnected )
      {
        m_peers.erase( peeriter );
      }
      else
      {
        ++peeriter;
      }
    }

    return Error::NO_ERROR_LBF;
  }
};

