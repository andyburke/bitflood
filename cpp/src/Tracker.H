#ifndef _LBF_TRACKER_H_
#define _LBF_TRACKER_H_

#include "Client.H"

namespace libBitFlood
{
  class TrackerMessageHandler : public Client::MessageHandler
  {
  public:
    // define our message names
    static const char RequestPeerList[];
    
  public:
    static Error::ErrorCode AddTrackerHandlers( ClientSPtr& i_client );
    Error::ErrorCode QueryAPI( V_String& o_supportedmessages );
    Error::ErrorCode HandleMessage( const std::string&  i_message, 
                                    PeerConnectionSPtr& i_receiver,
                                    XmlRpcValue&        i_args );
  protected:
    // our handlers
    Error::ErrorCode _HandleRequestPeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
  };
};

#endif; // _LBF_TRACKER_H_
