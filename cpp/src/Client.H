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
    struct Setup
    {
      std::string m_localIP;
      U32         m_localPort;
    };

  public:
    Setup                m_setup;
    V_FloodSPtr          m_floods;
    std::string          m_id;
    V_PeerConnectionSPtr m_peers;
    SocketSPtr           m_listensocket;

  public:
    Error::ErrorCode Initialize( const Setup& i_setup );
    Error::ErrorCode AddFloodFile( const FloodFile& i_floodfile );
    Error::ErrorCode Register( void );
    Error::ErrorCode UpdatePeerList( void );
    Error::ErrorCode GetChunks( void );
    Error::ErrorCode LoopOnce( void );
    Error::ErrorCode Disconnect( void );

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
