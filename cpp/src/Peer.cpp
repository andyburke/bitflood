#include "stdafx.H"

#include "Peer.H"
#include "Encoder.H"
#include "PeerMethods.H"
#include "TrackerMethods.H"
#include "ChunkMethods.H"

#include <sstream>
#include <iostream>

namespace libBitFlood
{
  Error::ErrorCode Peer::Initialize( const std::string& i_host, U32 i_port )
  {
    // cache the data
    m_localhost = i_host;
    m_localport = i_port;

    // compute an ID
    {
      std::stringstream idstrm;
      idstrm << m_localhost << m_localport;

      Encoder::Base64Encode( (const U8*)idstrm.str().c_str(), idstrm.str().length(), m_localid );
    }

    // Open a listen socket
    _OpenListenSocket();

    // setup some basic method handlers
    AddHandler( MethodHandlerSPtr( new PeerMethodHandler() ) );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::AddFloodFile( FloodFileSPtr& i_floodfile )
  {
    FloodSPtr f( new Flood() );
    f->Initialize( PeerSPtr( this ), i_floodfile );
    m_registeredfloods.push_back( f );

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
        peer->InitializeCommon( PeerSPtr( this ), host, port );
        peer->InitializeIncoming( incoming );
        m_pendingpeers.push_back( peer );
      }
    }

    return Error::NO_ERROR_LBF;
  }
  
  Error::ErrorCode Peer::AddHandler( MethodHandlerSPtr& i_handler )
  {
    m_methodhandlers.push_back( i_handler );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::HandleMethod( const std::string&  i_method, 
                                       PeerConnectionSPtr& i_receiver, 
                                       XmlRpcValue&        i_args )
  {
    V_MethodHandlerSPtr::iterator handleriter = m_methodhandlers.begin();
    V_MethodHandlerSPtr::iterator handlerend  = m_methodhandlers.end();
    for ( ; handleriter != handlerend; ++handleriter )
    {
      (*handleriter)->HandleMethod( i_method,
                                    i_receiver,
                                    i_args );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::InqFlood( const std::string& i_floodid, FloodSPtr& o_flood )
  {
    o_flood = FloodSPtr( NULL );
    if ( !i_floodid.empty() )
    {
      V_FloodSPtr::iterator flooditer = m_registeredfloods.begin();
      V_FloodSPtr::iterator floodend  = m_registeredfloods.end();

      for ( ; flooditer != floodend; ++flooditer )
      {
        if ( (*flooditer)->m_floodfile->m_contentHash.compare( i_floodid ) == 0 )
        {
          o_flood = (*flooditer);
          break;
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::_OpenListenSocket( void )
  {
    bool ok = true;

    // create the socket
    ok = Socket::CreateSocket( m_listensocket );

    // make it non-blocking
    ok = m_listensocket->SetNonBlocking();

    // turn on reuse
    ok = m_listensocket->SetReuse();

    // bind to the port
    ok = m_listensocket->Bind( m_localport );

    // tell the socket to listen
    ok = m_listensocket->Listen( 5 );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Peer::_ProcessPeers( void )
  {
    // copy the peer sptr vector, chances are it will change in this loop
    V_PeerConnectionSPtr pendingpeers = m_pendingpeers;
    
    V_PeerConnectionSPtr::iterator peeriter = pendingpeers.begin();
    V_PeerConnectionSPtr::iterator peerend  = pendingpeers.end();

    for ( ; peeriter != peerend; ++peeriter )
    {
      // process
      (*peeriter)->LoopOnce();

      // reap defunct peer connections
      if ( (*peeriter)->m_disconnected )
      {
      }
    }

    // process the floods 
    V_FloodSPtr floods = m_registeredfloods;

    V_FloodSPtr::iterator flooditer = floods.begin();
    V_FloodSPtr::iterator floodend  = floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      // process
      (*flooditer)->LoopOnce();
    }

    return Error::NO_ERROR_LBF;
  }
};

