#include "stdafx.H"
#include "Client.H"
#include "Encoder.H"
#include <sstream>
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
    FloodSPtr f( new Flood() );
    f->Initialize( i_floodfile );
    m_floods.push_back( f );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::Register( void )
  {
    V_FloodSPtr::iterator flooditer = m_floods.begin();
    V_FloodSPtr::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      FloodSPtr& flood = *flooditer;
      flood->Register( *this );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::UpdatePeerList( void )
  {
    V_FloodSPtr::iterator flooditer = m_floods.begin();
    V_FloodSPtr::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      FloodSPtr& flood = *flooditer;
      flood->UpdatePeerList( *this );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::GetChunks( void )
  {
    bool foundchunk = false;
    PeerConnectionSPtr todownload_from;
    FloodSPtr          todownload_flood;
    Flood::P_ChunkKey* todownload_key = NULL;

    // figure out what chunk to get from which peer and ask for it
    V_FloodSPtr::iterator flooditer = m_floods.begin();
    V_FloodSPtr::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      FloodSPtr& theflood = (*flooditer);

      Flood::V_ChunkKey::iterator chunkiter = theflood->m_chunkstodownload.begin();
      Flood::V_ChunkKey::iterator chunkend  = theflood->m_chunkstodownload.end();

      for ( ; chunkiter != chunkend; ++chunkiter )
      {
        Flood::P_ChunkKey& thechunkkey = (*chunkiter);
        if ( theflood->m_chunksDownloading.find( thechunkkey ) == theflood->m_chunksDownloading.end() )
        {
          const FloodFile::File& thefile = theflood->m_floodfile.m_files[ thechunkkey.first ];
          const FloodFile::Chunk& thechunk = thefile.m_chunks[ thechunkkey.second ];

          V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
          V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

          for ( ; peeriter != peerend; ++peeriter )
          {
            PeerConnectionSPtr& thepeer = (*peeriter);

            PeerConnection::M_StrToStrToStr::iterator floodchunkmap = thepeer->m_chunkMaps.find( theflood->m_floodfile.m_contentHash );
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
                  todownload_flood = theflood;
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
    if ( m_listensocket->CanRead( 0 ) )
    {
      // if socked accept, add incoming connection
      std::string host;
      U32 port;
      SocketSPtr incoming;
      if ( m_listensocket->Accept( incoming, host, port ) )
      {
        PeerConnectionSPtr peer( new PeerConnection() );
        peer->InitializeCommon( this, host, port );
        peer->InitializeIncoming( incoming );
        m_peers.push_back( peer );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::Disconnect( void )
  {
    V_FloodSPtr::iterator flooditer = m_floods.begin();
    V_FloodSPtr::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      FloodSPtr& flood = *flooditer;
      flood->Disconnect( *this );
    }

    return Error::NO_ERROR_LBF;
  }
  
  Error::ErrorCode Client::AddMessageHandler( const std::string& i_methodName, MessageHandler i_handler )
  {
    m_messageHandlers[ i_methodName ] = i_handler;
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::InqFlood( const std::string& i_floodid, FloodSPtr& o_flood )
  {
    o_flood = FloodSPtr( NULL );
    if ( !i_floodid.empty() )
    {
      V_FloodSPtr::iterator flooditer = m_floods.begin();
      V_FloodSPtr::iterator floodend  = m_floods.end();

      for ( ; flooditer != floodend; ++flooditer )
      {
        if ( (*flooditer)->m_floodfile.m_contentHash.compare( i_floodid ) == 0 )
        {
          o_flood = (*flooditer);
          break;
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::InqPeer( const std::string& i_peerid, PeerConnectionSPtr& o_peer )
  {
    o_peer = PeerConnectionSPtr( NULL );
    if ( !i_peerid.empty() )
    {
      V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
      V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

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
    ok = Socket::CreateSocket( m_listensocket );

    // make it non-blocking
    ok = m_listensocket->SetNonBlocking();

    // turn on reuse
    ok = m_listensocket->SetReuse();

    // bind to the port
    ok = m_listensocket->Bind( m_setup.m_localPort );

    // tell the socket to listen
    ok = m_listensocket->Listen( 5 );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::_ProcessPeers( void )
  {
    V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
    V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

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

