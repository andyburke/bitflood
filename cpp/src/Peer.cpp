#include "stdafx.H"

#include "Peer.H"
#include "Encoder.H"
#include "TrackerMethods.H"
#include "ChunkMethods.H"

#include <sstream>
#include <iostream>

namespace libBitFlood
{
  Error::ErrorCode Peer::Initialize( const Setup& i_setup )
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

    // setup some basic message handlers
    AddMessageHandler( MessageHandlerSPtr( new BasicMessageHandler() ) );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::AddFloodFile( const FloodFile& i_floodfile )
  {
    FloodSPtr f( new Flood() );
    f->Initialize( i_floodfile );
    m_floods.push_back( f );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::UpdateTrackers( void )
  {
    V_FloodSPtr::iterator flooditer = m_floods.begin();
    V_FloodSPtr::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      FloodSPtr& flood = *flooditer;

      Flood::V_TrackerInfo::const_iterator trackeriter = flood->m_trackerinfos.begin();
      Flood::V_TrackerInfo::const_iterator trackerend  = flood->m_trackerinfos.end();
      for ( ; trackeriter != trackerend; ++trackeriter )
      {
        const Flood::TrackerInfo& tracker = (*trackeriter);
        
        PeerConnectionSPtr peer;
        InqPeer( tracker.m_id, peer );

        if ( peer.Get() == NULL )
        {
          peer = PeerConnectionSPtr( new PeerConnection() );
          peer->InitializeCommon( this, tracker.m_host, tracker.m_port );
          peer->InitializeOutgoing( tracker.m_id );
          m_peers.push_back( peer );
        }

        // if this peer isn't registered yet
        if ( peer->m_registeredFloods.find( flood->m_floodfile.m_contentHash ) == peer->m_registeredFloods.end() )
        {
          // mark as registered
          peer->m_registeredFloods.insert( flood->m_floodfile.m_contentHash );

          // register w/ the peer
          {
            XmlRpcValue args;
            args[0] = flood->m_floodfile.m_contentHash;
            args[1] = m_id;
            args[2] = (int)m_setup.m_localPort;
            peer->SendMessage( PeerMethodHandler::RegisterWithPeer, args );
          }

          // request some chunks
          {
            XmlRpcValue args;
            args[0] = flood->m_floodfile.m_contentHash;
            peer->SendMessage( ChunkMethodHandler::RequestChunkMaps, args );
          }
        }

        // request a peer list
        {
          XmlRpcValue args;
          args[0] = flood->m_floodfile.m_contentHash;
          peer->SendMessage( TrackerMethodHandler::RequestPeerList, args );
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::LoopOnce( void )
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
  
  Error::ErrorCode Peer::AddMessageHandler( MessageHandlerSPtr& i_handler )
  {
    V_String messages;
    i_handler->QueryAPI( messages );
    
    V_String::const_iterator iter = messages.begin();
    V_String::const_iterator end  = messages.end();

    for( ; iter != end; ++iter )
    {
      const std::string& message = *iter;
      m_messagehandlers[ message ].push_back( i_handler );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::DispatchMessage( const std::string& i_message, 
                                            PeerConnectionSPtr& i_receiver, 
                                            XmlRpcValue& i_args )
  {
    M_StrToV_MessageHandlerSPtr::iterator messageiter = m_messagehandlers.find( i_message );
    if ( messageiter != m_messagehandlers.end() )
    {
      V_MessageHandlerSPtr::iterator handleriter = (*messageiter).second.begin();
      V_MessageHandlerSPtr::iterator handlerend  = (*messageiter).second.end();
      for ( ; handleriter != handlerend; ++handleriter )
      {
        (*handleriter)->HandleMessage( i_message,
                                       i_receiver,
                                       i_args );
      }
    }


    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::InqFlood( const std::string& i_floodid, FloodSPtr& o_flood )
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
    // copy the peer sptr vector, chances are it will change in this loop
    V_PeerConnectionSPtr peers = m_peers;
    
    V_PeerConnectionSPtr::iterator peeriter = peers.begin();
    V_PeerConnectionSPtr::iterator peerend  = peers.end();

    for ( ; peeriter != peerend; ++peeriter )
    {
      // process
      (*peeriter)->LoopOnce();

      // reap defunct peer connections
      if ( (*peeriter)->m_disconnected )
      {
       // m_peers.erase( peeriter );
      }
    }

    return Error::NO_ERROR_LBF;
  }
};

