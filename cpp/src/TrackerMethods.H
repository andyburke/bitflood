#ifndef _LBF_TRACKER_METHODS_H_
#define _LBF_TRACKER_METHODS_H_

#include "Peer.H"

namespace libBitFlood
{
  class TrackerMethodHandler : public MethodHandler
  {
  public:
    // define our method names
    static const char RequestPeerList[];
    
  public:
    Error::ErrorCode QueryMethods( V_String& o_supportedmethods );
    Error::ErrorCode HandleMethod( const std::string&  i_method, 
                                   PeerConnectionSPtr& i_receiver,
                                   XmlRpcValue&        i_args );
  protected:
    // our handlers
    Error::ErrorCode _HandleRequestPeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
  };
};

#endif; // _LBF_TRACKER_H_
