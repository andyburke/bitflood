#include "stdafx.H"

#include "PeerMethods.H"
#include "Encoder.H"
#include "ChunkMethods.H"

#include <sstream>
#include <iostream>

namespace libBitFlood
{
  // define our method names
  const char PeerMethodHandler::ReceivePeerList[]  = "ReceivePeerList";
  const char PeerMethodHandler::RegisterWithPeer[] = "RegisterWithPeer";
  const char PeerMethodHandler::AcknowledgePeer[]  = "AcknowledgePeer";

  Error::ErrorCode PeerMethodHandler::QueryMethods( V_String& o_supportedmethods )
  {
    o_supportedmethods.push_back( ReceivePeerList );
    o_supportedmethods.push_back( RegisterWithPeer );
    o_supportedmethods.push_back( AcknowledgePeer );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerMethodHandler::HandleMethod( const std::string&  i_method, 
                                              PeerConnectionSPtr& i_receiver,
                                              XmlRpcValue&        i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_method.compare( ReceivePeerList ) == 0 )
    {
      ret = _HandleReceivePeerList( i_receiver, i_args );
    }
    else if ( i_method.compare( RegisterWithPeer ) == 0 )
    {
      ret = _HandleRegisterWithPeer( i_receiver, i_args );
    }
    else if ( i_method.compare( AcknowledgePeer ) == 0 )
    {
      ret = _HandleAcknowledgePeer( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode PeerMethodHandler::_HandleReceivePeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
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
      if( peerId.compare( i_receiver->i_localpeer->m_id ) != 0 )
      {
        PeerConnectionSPtr peer;
        i_receiver->i_localpeer->InqPeer( peerId, peer );
        
        if ( peer.Get() == NULL )
        {
          peer = PeerConnectionSPtr( new PeerConnection() );
          peer->InitializeCommon( i_receiver->i_localpeer, peerHost, peerPort );
          peer->InitializeOutgoing( peerId );
          i_receiver->i_localpeer->m_peers.push_back( peer );
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
            args[1] = i_receiver->i_localpeer->m_id;
            args[2] = (int)i_receiver->i_localpeer->m_setup.m_localPort;
            peer->SendMessage( RegisterWithPeer, args );
          }

          // request some chunks
          {
            XmlRpcValue args;
            args[0] = floodhash;
            peer->SendMessage( ChunkMethodHandler::RequestChunkMaps, args );
          }
        }
      }          
    }
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerMethodHandler::_HandleRegisterWithPeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& floodhash = i_args[0];
    const std::string& peerId    = i_args[1];
    U32 peerlisten               = (int)i_args[2];

    // see if the peer is already registered
    if ( !peerId.empty() )
    {
      PeerConnectionSPtr peer;
      i_receiver->i_localpeer->InqPeer( peerId, peer );
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
          i_receiver->SendMessage( ChunkMethodHandler::RequestChunkMaps, args );
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode PeerMethodHandler::_HandleAcknowledgePeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    return Error::NO_ERROR_LBF;
  }
};

