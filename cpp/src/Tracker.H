#ifndef _LBF_TRACKER_H_
#define _LBF_TRACKER_H_

#include "Client.H"

namespace libBitFlood
{
  class TrackerMessageHandler : public Client::MessageHandler
  {
  public:
    // define our message names
    static const char RegisterWithTracker[];
    static const char DisconnectFromTracker[];
    static const char RequestPeerList[];

    struct PeerInfo
    {
      std::string m_id;
      std::string m_ip;
      U32 m_port;
      time_t m_timestamp;
    };
      
    typedef std::vector< PeerInfo > V_PeerInfo;
    typedef std::map< std::string, V_PeerInfo > M_StrToPeerInfo;
    
    M_StrToPeerInfo m_peerinfo;

  public:
    static Error::ErrorCode AddTrackerHandlers( ClientSPtr& i_client );
    Error::ErrorCode QueryAPI( V_String& o_supportedmessages );
    Error::ErrorCode HandleMessage( const std::string&        i_message, 
				    const PeerConnectionSPtr& i_receiver,
				    XmlRpcValue&              i_args );
  protected:
    // our handlers
    Error::ErrorCode _HandleRegisterWithTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleDisconnectFromTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleRequestPeerList( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );

  };
};

#endif; // _LBF_TRACKER_H_
