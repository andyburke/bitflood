#ifndef _LBF_PEER_METHODS_H_
#define _LBF_PEER_METHODS_H_

#include "Peer.H"
#include "PeerConnection.H"

namespace libBitFlood
{
  class PeerMethodHandler : public MethodHandler
  {
  public:
    // define our method names
    static const char ReceivePeerList[];
    static const char RegisterWithPeer[];
    static const char AcknowledgePeer[];
    
    Error::ErrorCode HandleMethod( const std::string&  i_method, 
                                   PeerConnectionSPtr& i_receiver,
                                   XmlRpcValue&        i_args );

  protected:
    Error::ErrorCode _HandleReceivePeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleRegisterWithPeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleAcknowledgePeer( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
  };
};

#endif; // _LBF_PEER_METHODS_H_
