#ifndef _LBF_CLIENT_H_
#define _LBF_CLIENT_H_

#include "Flood.H"
#include "Socket.H"
#include "PeerConnection.H"

namespace libBitFlood
{
  typedef ReferenceCountTemplate< class Client > ClientSPtr;
  class Client : public ReferenceCountBase
  {
  public:
    class MessageHandler : public ReferenceCountBase
    {
    public:
      virtual Error::ErrorCode QueryAPI( V_String& o_supportedmessages ) = 0;
      virtual Error::ErrorCode HandleMessage( const std::string& i_message, 
					      const PeerConnectionSPtr& i_receiver,
					      XmlRpcValue& i_args ) = 0;
    };
    typedef ReferenceCountTemplate< MessageHandler > MessageHandlerSPtr;


    class BasicMessageHandler : public MessageHandler
    {
    public:
      // define our message names
      static const char RegisterWithPeer[];
      static const char AcknowledgePeer[];

      Error::ErrorCode QueryAPI( V_String& o_supportedmessages );
      Error::ErrorCode HandleMessage( const std::string& i_message, 
				      const PeerConnectionSPtr& i_receiver,
				      XmlRpcValue& i_args );
    };

    struct Setup
    {
      std::string m_localIP;
      U32         m_localPort;
    };

    typedef std::vector< MessageHandlerSPtr > V_MessageHandlerSPtr;
    typedef std::map< std::string, V_MessageHandlerSPtr > M_StrToV_MessageHandlerSPtr;

  public:
    Setup                       m_setup;
    V_FloodSPtr                 m_floods;
    std::string                 m_id;
    V_PeerConnectionSPtr        m_peers;
    SocketSPtr                  m_listensocket;
    M_StrToV_MessageHandlerSPtr m_messagehandlers;

  public:
    Error::ErrorCode Initialize( const Setup& i_setup );
    Error::ErrorCode AddFloodFile( const FloodFile& i_floodfile );
    Error::ErrorCode Register( void );
    Error::ErrorCode UpdatePeerList( void );
    Error::ErrorCode GetChunks( void );
    Error::ErrorCode LoopOnce( void );
    Error::ErrorCode Disconnect( void );

    Error::ErrorCode AddMessageHandler( MessageHandlerSPtr& i_handler );

  public:
    Error::ErrorCode InqFlood( const std::string& i_floodid, FloodSPtr& o_flood );
    Error::ErrorCode InqPeer( const std::string& i_peerid, PeerConnectionSPtr& o_peer );

  protected:
    Error::ErrorCode _OpenListenSocket( void );
    Error::ErrorCode _ProcessPeers( void );
  };
  
  typedef std::vector< ClientSPtr > V_ClientSPtr;
};

#endif; // _LBF_CLIENT_H_
