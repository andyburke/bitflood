#include "stdafx.H"

#include "PeerMethods.H"
#include "Encoder.H"
#include "ChunkMethods.H"

#include <sstream>
#include <iostream>

namespace libBitFlood
{
  // define our method names
  const char PeerMethodHandler::SendPeerList[]  = "SendPeerList";
  const char PeerMethodHandler::Register[] = "Register";
  const char PeerMethodHandler::AcknowledgePeer[]  = "AcknowledgePeer";

  Error::ErrorCode PeerMethodHandler::HandleMethod( const std::string&  i_method, 
                                              PeerConnectionSPtr& i_receiver,
                                              XmlRpcValue&        i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_method.compare( SendPeerList ) == 0 )
    {
      ret = _HandleSendPeerList( i_receiver, i_args );
    }
    else if ( i_method.compare( Register ) == 0 )
    {
      ret = _HandleRegister( i_receiver, i_args );
    }
    else if ( i_method.compare( AcknowledgePeer ) == 0 )
    {
      ret = _HandleAcknowledgePeer( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode PeerMethodHandler::_HandleSendPeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    U32 index;
    for ( index = 0; index < i_args.size(); ++index )
    {
      U32 startIndex = 0;
      const std::string& cur_res = i_args[ index ];
      std::string peerid   = cur_res.substr( startIndex, 
                                             cur_res.find( ':', startIndex ) - startIndex );
      startIndex += peerid.length() + 1;
      std::string peerhost = cur_res.substr( startIndex, 
                                             cur_res.find( ':', startIndex ) - startIndex );
      startIndex += peerhost.length() + 1;
      U32 peerport;
      std::stringstream peerPort_converter;
      peerPort_converter << cur_res.substr( startIndex, std::string::npos );
      peerPort_converter >> peerport;
      
      // the client doesn't need himself as a peer
      if( peerid.compare( i_receiver->m_flood->m_localpeer->m_localid ) != 0 )
      {
        PeerConnectionSPtr peer;
        i_receiver->m_flood->InqPeer( peerid, peer );
        
        if ( peer.Get() == NULL )
        {
          peer = PeerConnectionSPtr( new PeerConnection() );
          peer->InitializeCommon( PeerSPtr( i_receiver->m_localpeer ), peerhost, peerport );
          peer->InitializeOutgoing( FloodSPtr( i_receiver->m_flood ), peerid );
          i_receiver->m_flood->m_peers.push_back( peer );
        }
      }          
    }
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerMethodHandler::_HandleRegister( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& floodhash = i_args[0];
    const std::string& peerid    = i_args[1];
    U32 peerlisten               = (int)i_args[2];

    // see if the peer is already registered
    if ( !floodhash.empty() && !peerid.empty() )
    {
      // the receiving connection should not have a flood assigned yet
      assert( i_receiver->m_flood == NULL );

      // find the correct flood, add a dummy if none exist
      FloodSPtr flood;
      i_receiver->m_localpeer->InqFlood( floodhash, flood );
      if ( false ) //flood.Get() == NULL )
      {
        flood = FloodSPtr( new Flood() );
        i_receiver->m_localpeer->m_registeredfloods.push_back( flood );
      }

      // again check that the flood doesn't know about this peer already
      PeerConnectionSPtr existingpeer;
      if ( flood.Get() != NULL )
      {
        flood->InqPeer( peerid, existingpeer );
      }

      assert( existingpeer.Get() == NULL );

      // find the pending peer
      V_PeerConnectionSPtr::iterator peeriter = i_receiver->m_localpeer->m_pendingpeers.begin();
      V_PeerConnectionSPtr::iterator peerend  = i_receiver->m_localpeer->m_pendingpeers.end();
      for ( ; peeriter != peerend; ++peeriter )
      {
        if ( (*peeriter).Get() == i_receiver.Get() )
        {
          break;
        }
      }

      // the peer should be in the array of pending peers, and we should be able to remove it
      assert( peeriter != peerend );
      i_receiver->m_localpeer->m_pendingpeers.erase( peeriter );
      
      // now assign the id and put the peer in the flood
      if ( flood.Get() != NULL )
      {
        i_receiver->m_id         = peerid;
        i_receiver->m_listenport = peerlisten;
        i_receiver->m_flood      = flood.Get();
        flood->m_peers.push_back( i_receiver );
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerMethodHandler::_HandleAcknowledgePeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    return Error::NO_ERROR_LBF;
  }
};

