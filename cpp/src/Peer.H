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
    virtual Error::ErrorCode QueryMethods( V_String& o_supportedmethods ) = 0;
    virtual Error::ErrorCode HandleMethod( const std::string& i_method, 
                                           PeerConnectionSPtr& i_receiver,
                                           XmlRpcValue& i_args ) = 0;
  };

  typedef std::map< std::string, V_MethodHandlerSPtr > M_StrToV_MethodHandlerSPtr;

  class Peer : public ReferenceCountBase
  {
    struct Setup
    {
      std::string m_localIP;
      U32         m_localPort;
    };

  public:
    Setup                       m_setup;
    V_FloodSPtr                 m_floods;
    std::string                 m_id;
    V_PeerConnectionSPtr        m_pendingpeers;
    SocketSPtr                  m_listensocket;
    M_StrToV_MethodHandlerSPtr  m_methodhandlers;

  public:
    Error::ErrorCode Initialize( const Setup& i_setup );
    Error::ErrorCode AddFloodFile( const FloodFile& i_floodfile );
    Error::ErrorCode UpdateTrackers( void );
    Error::ErrorCode LoopOnce( void );

    Error::ErrorCode AddMethodHandler( MethodHandlerSPtr& i_handler );
    Error::ErrorCode DispatchMethod( const std::string& i_method, 
                                     PeerConnectionSPtr& i_receiver, 
                                     XmlRpcValue& i_args );

  public:
    Error::ErrorCode InqFlood( const std::string& i_floodid, FloodSPtr& o_flood );

  protected:
    Error::ErrorCode _OpenListenSocket( void );
    Error::ErrorCode _ProcessPeers( void );
  };
  
  typedef ReferenceCountTemplate< class Peer > PeerSPtr;
  typedef std::vector< PeerSPtr > V_PeerSPtr;
};

#endif; // _LBF_PEER_H_
