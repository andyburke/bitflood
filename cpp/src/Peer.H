#ifndef _LBF_PEER_H_
#define _LBF_PEER_H_

#include "Flood.H"
#include "Socket.H"
#include "PeerConnection.H"

#include <XmlRpc.h>
using namespace XmlRpc;

namespace libBitFlood
{
  class PeerPlugin : public ReferenceCountBase
  {
  public:
    virtual Error::ErrorCode HandleFlood( const FloodFile& i_floodfile )
    {
      return Error::NO_ERROR_LBF;
    }
	
    virtual Error::ErrorCode HandleMethod( const std::string& i_method, 
                                           PeerConnectionSPtr& i_receiver,
                                           XmlRpcValue& i_args )
    {
      return Error::NO_ERROR_LBF;
    }

    PeerSPTr m_peer;
  };

  /*
    Peer objects are super lightweight
    - provide a structure for containing connections to other peers on a per-flood basis
    - provide a plugin architecture for various plugins which might provide various functionality, examples:
     - BasicPeer plugin
      - manages connections, handshaking
     - ChunkMaster plugin
      - computes and stores data about a flood when flood is added
      - handles requests for sending/receiving flood data
     - Tracker plugin
      - responds to queries about which peers are connected for a certain flood
  */

  class Peer : public ReferenceCountBase
  {
    struct Setup
    {
      std::string m_localIP;
      U32         m_localPort;
    };

    typedef std::map< std::string, V_PeerConnectionSPtr > M_StrToV_PeerConnectionSPTr;

  public:
    Setup                       m_setup;
    std::string                 m_localid;
    V_PeerConnectionSPtr        m_pendingpeers;
    M_StrToV_PeerConnectionSPTr m_registeredpeers;
    SocketSPtr                  m_listensocket;
    V_PeerPluginSPtr            m_plugins;
    PeerPluginSPtr              m_basicplugin;

  public:
    Error::ErrorCode Initialize( const Setup& i_setup );
    Error::ErrorCode Cleanup( void );
    Error::ErrorCode LoopOnce( void );

    Error::ErrorCode AddPlugin( PeerPluginSPtr& i_plugin );
    Error::ErrorCode HandleFloodFile( const FloodFile& i_floodfile );
    Error::ErrorCode HandleMethod( const std::string& i_method, 
				   PeerConnectionSPtr& i_receiver, 
				   XmlRpcValue& i_args );

  protected:
    Error::ErrorCode _OpenListenSocket( void );
    Error::ErrorCode _ProcessPeers( void );
  };

  class BasicPeerPlugin : public PeerPlugin
  {
  public:
    Error::ErrorCode InitializePeer( const Peer::Setup& i_setup );
    Error::ErrorCode CleanupPeer( void );
    Error::ErrorCode ProcessPeer( void );

  public:
    Error::ErrorCode HandleFlood( const FloodFileSPtr& i_floodfile );
    Error::ErrorCode HandleMethod( const std::string& i_method, 
				   PeerConnectionSPtr& i_receiver,
				   XmlRpcValue& i_args );
  };


  
  typedef ReferenceCountTemplate< class Peer > PeerSPtr;
  typedef std::vector< PeerSPtr > V_PeerSPtr;
};

#endif; // _LBF_PEER_H_
