#ifndef _LBF_PEER_CONNECTION_H_
#define _LBF_PEER_CONNECTION_H_

#include <XmlRpc.h>
#include <XmlRpcServerConnection.h>
#include <XmlRpcSocket.h>

#include "Peer.H"
#include "Flood.H"
#include "Socket.H"

using namespace XmlRpc;

namespace libBitFlood
{
  // forward declares
  typedef ReferenceCountTemplate< class Peer > PeerSPtr;

  class PeerConnection : public ReferenceCountBase
  {
  public:
    class FakeClient : public XmlRpcClient
    {
    public:
      FakeClient() : XmlRpcClient( "", 0 ) {}
      std::string& AccessRequest( void ) { return _request; }
      bool ExternalGenerateRequest(const char* i_method, const XmlRpcValue& i_args) { return generateRequest( i_method, i_args ); }

    protected:
      virtual std::string generateHeader(std::string const& body) { return ""; }
    };

    class FakeServer : public XmlRpcServerConnection
    {
    public:
      FakeServer() : XmlRpcServerConnection( XmlRpcSocket::socket(), NULL ) {}
      void SetRequest( const std::string& i_request ) { _request = i_request; }
      std::string ExternalParseRequest(XmlRpcValue& i_args) { return parseRequest( i_args); }
    };

    struct Reader
    {
      Reader( void ) 
        : m_socket()
        , m_lasttail ( 0 )
        , m_buffersize ( 0 )
      {
      }

      SocketSPtr  m_socket;
      std::string m_buffer;
      U32         m_lasttail;
      U32         m_buffersize;

      Error::ErrorCode Read( I32& o_bytesRead );
    };

    struct Writer
    {
      Writer( void ) 
        : m_socket()
        , m_buffersize ( 0 )
      {
      }

      SocketSPtr  m_socket;
      std::string m_buffer;
      U32         m_buffersize;

      Error::ErrorCode Write( I32& o_bytesWritten );
    };

  public:
    typedef std::map< std::string, std::string > M_StrToStr;
    typedef std::map< std::string, M_StrToStr > M_StrToStrToStr;

  public:
    Error::ErrorCode InitializeCommon( PeerSPtr&          i_localpeer,
                                       const std::string& i_peerHost,
                                       U32                i_peerPort );


    Error::ErrorCode InitializeIncoming( const SocketSPtr& i_socket );
    Error::ErrorCode InitializeOutgoing( FloodSPtr&         i_flood,
                                         const std::string& i_peerid );

    Error::ErrorCode LoopOnce( void );

    /// send a message to the peer on the other end of the wire
    Error::ErrorCode SendMethod( const std::string& i_methodname, const XmlRpcValue& i_args );

    // member variables
  public:
    Peer*       m_localpeer;
    Flood*      m_flood;
    SocketSPtr  m_socket;
    std::string m_id;
    bool        m_disconnected;
    bool        m_connected;
    std::string m_host;
    U32         m_port;
    U32         m_listenport;
    FakeClient  m_fakeClient;
    FakeServer*  m_fakeServer;
    Reader      m_reader;
    Writer      m_writer;

  protected:
    Error::ErrorCode _Connect( void );
    Error::ErrorCode _ReadOnce( void );
    Error::ErrorCode _WriteOnce( void );
    Error::ErrorCode _ProcessReadBuffer( void );
  };
};

#endif; // _LBF_PEER_CONNECTION_H_
