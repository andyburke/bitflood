#include "stdafx.H"

#include "Client.H"
#include "Encoder.H"
#include "Tracker.H"
#include "ChunkHandler.H"

#include <sstream>
#include <iostream>

namespace libBitFlood
{
  // define our message names
  const char Client::BasicMessageHandler::ReceivePeerList[]  = "ReceivePeerList";
  const char Client::BasicMessageHandler::RegisterWithPeer[] = "RegisterWithPeer";
  const char Client::BasicMessageHandler::AcknowledgePeer[]  = "AcknowledgePeer";

  Error::ErrorCode Client::BasicMessageHandler::QueryAPI( V_String& o_supportedmessages )
  {
    o_supportedmessages.push_back( ReceivePeerList );
    o_supportedmessages.push_back( RegisterWithPeer );
    o_supportedmessages.push_back( AcknowledgePeer );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::BasicMessageHandler::HandleMessage( const std::string&  i_message, 
							       PeerConnectionSPtr& i_receiver,
							       XmlRpcValue&        i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_message.compare( ReceivePeerList ) == 0 )
    {
      ret = _HandleReceivePeerList( i_receiver, i_args );
    }
    else if ( i_message.compare( RegisterWithPeer ) == 0 )
    {
      ret = _HandleRegisterWithPeer( i_receiver, i_args );
    }
    else if ( i_message.compare( AcknowledgePeer ) == 0 )
    {
      ret = _HandleAcknowledgePeer( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode Client::BasicMessageHandler::_HandleReceivePeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& floodhash = i_args[0];
    XmlRpcValue& result = i_args[1];
    
    U32 index;
    for ( index = 0; index < result.size(); ++index )
    {
      U32 startIndex = 0;
      const std::string& cur_res = result[ index ];
      std::string peerId   = cur_res.substr( startIndex, 
                                             cur_res.find( ':', startIndex ) - startIndex );
      startIndex += peerId.length() + 1;
      std::string peerHost = cur_res.substr( startIndex, 
                                             cur_res.find( ':', startIndex ) - startIndex );
      startIndex += peerHost.length() + 1;
      U32 peerPort;
      std::stringstream peerPort_converter;
      peerPort_converter << cur_res.substr( startIndex, std::string::npos );
      peerPort_converter >> peerPort;
      
      // the client doesn't need himself as a peer
      if( peerId.compare( i_receiver->m_client->m_id ) != 0 )
      {
        PeerConnectionSPtr peer;
        i_receiver->m_client->InqPeer( peerId, peer );
        
        if ( peer.Get() == NULL )
        {
          peer = PeerConnectionSPtr( new PeerConnection() );
          peer->InitializeCommon( i_receiver->m_client, peerHost, peerPort );
          peer->InitializeOutgoing( peerId );
          i_receiver->m_client->m_peers.push_back( peer );
        }
        
        // if this peer isn't registered yet
        if ( peer->m_registeredFloods.find( floodhash ) == peer->m_registeredFloods.end() )
        {
          // mark as registered
          peer->m_registeredFloods.insert( floodhash );
          
          // register w/ the peer
          {
            XmlRpcValue args;
            args[0] = floodhash;
            args[1] = i_receiver->m_client->m_id;
            args[2] = (int)i_receiver->m_client->m_setup.m_localPort;
            peer->SendMessage( RegisterWithPeer, args );
          }

          // request some chunks
          {
            XmlRpcValue args;
            args[0] = floodhash;
            peer->SendMessage( ChunkMessageHandler::RequestChunkMaps, args );
          }
        }
      }          
    }
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::BasicMessageHandler::_HandleRegisterWithPeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& floodhash = i_args[0];
    const std::string& peerId    = i_args[1];
    U32 peerlisten               = (int)i_args[2];

    // see if the peer is already registered
    if ( !peerId.empty() )
    {
      PeerConnectionSPtr peer;
      i_receiver->m_client->InqPeer( peerId, peer );
      if ( peer.Get() != NULL )
      {
        // disconnect duplicate peers
        peer->m_disconnected = true;
      }
      else if ( i_receiver->m_registeredFloods.find( floodhash ) != i_receiver->m_registeredFloods.end() )
      {
        // strange duplicate registration from a peer for a given flood
      }
      else
      {
        // cache the peer's ID
        i_receiver->m_id = peerId;

        // cache the peer's listen port
        i_receiver->m_listenport = peerlisten;
        
        // mark as registered
        i_receiver->m_registeredFloods.insert( floodhash );

        // acknowledge the register
        {
          XmlRpcValue args;
          args[0] = floodhash;
          i_receiver->SendMessage( AcknowledgePeer, args );
        }

        // request some chunks
        {
          XmlRpcValue args;
          args[0] = floodhash;
          i_receiver->SendMessage( ChunkMessageHandler::RequestChunkMaps, args );
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::BasicMessageHandler::_HandleAcknowledgePeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    return Error::NO_ERROR_LBF;
  }

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

    // setup some basic message handlers
    AddMessageHandler( MessageHandlerSPtr( new BasicMessageHandler() ) );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::AddFloodFile( const FloodFile& i_floodfile )
  {
    FloodSPtr f( new Flood() );
    f->Initialize( i_floodfile );
    m_floods.push_back( f );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Client::UpdateTrackers( void )
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
            peer->SendMessage( BasicMessageHandler::RegisterWithPeer, args );
          }

          // request some chunks
          {
            XmlRpcValue args;
            args[0] = flood->m_floodfile.m_contentHash;
            peer->SendMessage( ChunkMessageHandler::RequestChunkMaps, args );
          }
        }

        // request a peer list
        {
          XmlRpcValue args;
          args[0] = flood->m_floodfile.m_contentHash;
          peer->SendMessage( TrackerMessageHandler::RequestPeerList, args );
        }
      }
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
  
  Error::ErrorCode Client::AddMessageHandler( MessageHandlerSPtr& i_handler )
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

  Error::ErrorCode Client::DispatchMessage( const std::string& i_message, 
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

