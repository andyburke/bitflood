#ifndef _LBF_PEER_H_
#define _LBF_PEER_H_

#include "Flood.H"
#include "Socket.H"
#include "PeerConnection.H"

#include <XmlRpc.h>
using namespace XmlRpc;

namespace libBitFlood
{
  class MethodHandler : public ReferenceCountBase
  {
  public:
    virtual Error::ErrorCode HandleMethod( const std::string&  i_method, 
                                           PeerConnectionSPtr& i_receiver,
                                           XmlRpcValue&        i_args ) = 0;
  };

  class Peer : public ReferenceCountBase
  {
  public:
    std::string          m_localhost;
    U32                  m_localport;
    std::string          m_localid;
    SocketSPtr           m_listensocket;
    V_PeerConnectionSPtr m_pendingpeers;
    V_FloodSPtr          m_registeredfloods;
    V_MethodHandlerSPtr  m_methodhandlers;

  public:
    Error::ErrorCode Initialize( const std::string& i_host, U32 i_port );
    Error::ErrorCode LoopOnce( void );

    Error::ErrorCode AddFloodFile( FloodFileSPtr& i_floodfile );
    Error::ErrorCode AddHandler( MethodHandlerSPtr& i_handler );
    Error::ErrorCode HandleMethod( const std::string& i_method, 
				   PeerConnectionSPtr& i_receiver, 
				   XmlRpcValue& i_args );

  public:
    Error::ErrorCode InqFlood( const std::string& i_floodid, FloodSPtr& o_flood );

  protected:
    Error::ErrorCode _OpenListenSocket( void );
    Error::ErrorCode _ProcessPeers( void );
  };
};

#endif; // _LBF_PEER_H_
