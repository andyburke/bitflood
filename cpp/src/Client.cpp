#include "stdafx.H"
#include "Client.H"
#include "Encoder.H"
#include <sstream>
#include <Ws2tcpip.h>
#include <iostream>

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

      Encoder::Base64Encode( (const U8*)idstrm.str().c_str(), idstrm.str().length(), m_id );
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
    bool foundchunk = false;
    PeerConnection*    todownload_from = NULL;
    Flood*             todownload_flood = NULL;
    Flood::P_ChunkKey* todownload_key = NULL;

    // figure out what chunk to get from which peer and ask for it
    V_Flood::iterator flooditer = m_floods.begin();
    V_Flood::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      Flood& theflood = (*flooditer);

      Flood::V_ChunkKey::iterator chunkiter = theflood.m_chunkstodownload.begin();
      Flood::V_ChunkKey::iterator chunkend  = theflood.m_chunkstodownload.end();

      for ( ; chunkiter != chunkend; ++chunkiter )
      {
        Flood::P_ChunkKey& thechunkkey = (*chunkiter);
        if ( theflood.m_chunksDownloading.find( thechunkkey ) == theflood.m_chunksDownloading.end() )
        {
          const FloodFile::File& thefile = theflood.m_floodfile.m_files[ thechunkkey.first ];
          const FloodFile::Chunk& thechunk = thefile.m_chunks[ thechunkkey.second ];

          V_PeerConnectionPtr::iterator peeriter = m_peers.begin();
          V_PeerConnectionPtr::iterator peerend  = m_peers.end();

          for ( ; peeriter != peerend; ++peeriter )
          {
            PeerConnection* thepeer = (*peeriter);

            PeerConnection::M_StrToStrToStr::iterator floodchunkmap = thepeer->m_chunkMaps.find( theflood.m_floodfile.m_contentHash );
            if ( floodchunkmap != thepeer->m_chunkMaps.end() )
            {
              PeerConnection::M_StrToStr::iterator filechunkmap = (*floodchunkmap).second.find( thefile.m_name );
              if( filechunkmap != (*floodchunkmap).second.end() )
              {
                const std::string& chunkmap = (*filechunkmap).second;
                if ( chunkmap[ thechunkkey.second ] == '1' )
                {
                  foundchunk = true;
                  todownload_from  = thepeer;
                  todownload_flood = &theflood;
                  todownload_key   = &thechunkkey;
                }
              }
            }
          }
        }
      }
    }

    if ( foundchunk )
    {
      todownload_from->RequestChunk( todownload_flood->m_floodfile.m_contentHash, 
				     todownload_flood->m_floodfile.m_files[ todownload_key->first ].m_name,
				     todownload_key->second );
      todownload_flood->m_chunksDownloading.insert( *todownload_key );
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
    timeout.tv_sec = timeout.tv_usec = 0;
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
        peer->InitializeCommon( this, hostbuff, ntohs( addr.sin_port ) );
        peer->InitializeIncoming( incoming );
        m_peers.push_back( peer );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::Disconnect( void )
  {
    V_Flood::iterator flooditer = m_floods.begin();
    V_Flood::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      Flood& flood = *flooditer;
      flood.Disconnect( *this );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::InqFlood( const std::string& i_floodid, Flood*& o_flood )
  {
    o_flood = NULL;
    if ( !i_floodid.empty() )
    {
      V_Flood::iterator flooditer = m_floods.begin();
      V_Flood::iterator floodend  = m_floods.end();

      for ( ; flooditer != floodend; ++flooditer )
      {
        if ( (*flooditer).m_floodfile.m_contentHash.compare( i_floodid ) == 0 )
        {
          o_flood = &(*flooditer);
          break;
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::InqPeer( const std::string& i_peerid, PeerConnection*& o_peer )
  {
    o_peer = NULL;
    if ( !i_peerid.empty() )
    {
      V_PeerConnectionPtr::iterator peeriter = m_peers.begin();
      V_PeerConnectionPtr::iterator peerend  = m_peers.end();

      for ( ; peeriter != peerend; ++peeriter )
      {
        if ( (*peeriter)->m_id.compare( i_peerid ) == 0 )
        {
          o_peer = (*peeriter);
          break;
        }
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
      if ( false )//(*peeriter)->m_disconnected )
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

