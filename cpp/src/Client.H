#ifndef _LBF_CLIENT_H_
#define _LBF_CLIENT_H_

#include <Winsock2.h>
#include "Flood.H"
#include "PeerConnection.H"

namespace libBitFlood
{
  class Client
  {
  public:
    struct Setup
    {
      std::string m_localIP;
      U32         m_localPort;
    };

    Setup               m_setup;
    V_Flood             m_floods;
    std::string         m_id;
    V_PeerConnectionPtr m_peers;
    SOCKET              m_listensocket;

  public:
    Error::ErrorCode Initialize( const Setup& i_setup );
    Error::ErrorCode AddFloodFile( const FloodFile& i_floodfile );
    Error::ErrorCode Register( void );
    Error::ErrorCode UpdatePeerList( void );
    Error::ErrorCode GetChunks( void );
    Error::ErrorCode LoopOnce( void );
    Error::ErrorCode Disconnect( void );

  public:
    Error::ErrorCode InqFlood( const std::string& i_floodid, Flood*& o_flood );
    Error::ErrorCode InqPeer( const std::string& i_peerid, PeerConnection*& o_peer );

  protected:
    Error::ErrorCode _OpenListenSocket( void );
    Error::ErrorCode _ProcessPeers( void );
  };
};

#endif; // _LBF_CLIENT_H_
