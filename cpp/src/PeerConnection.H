#ifndef _LBF_PEER_CONNECTION_H_
#define _LBF_PEER_CONNECTION_H_

#include <XmlRpc.h>
#include <XmlRpcServerConnection.h>
#include <XmlRpcSocket.h>

#include "Flood.H"
#include "Socket.H"

using namespace XmlRpc;

namespace libBitFlood
{
  class Client;

  typedef ReferenceCountTemplate< class PeerConnection > PeerConnectionSPtr;
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
    typedef Error::ErrorCode (*_MessageHandler)( PeerConnection& i_receiver, XmlRpcValue& i_args );
    typedef std::map< std::string, _MessageHandler > M_StrToMessageHandler;
    typedef std::map< std::string, std::string > M_StrToStr;
    typedef std::map< std::string, M_StrToStr > M_StrToStrToStr;

  public:
    Error::ErrorCode InitializeCommon( Client*            i_client,
                                       const std::string& i_peerHost,
                                       U32                i_peerPort );


    Error::ErrorCode InitializeIncoming( const SocketSPtr& i_socket );
    Error::ErrorCode InitializeOutgoing( const std::string& i_peerId );

    Error::ErrorCode LoopOnce( void );

    // messages
    Error::ErrorCode Register( const std::string& i_floodhash,
			       const std::string& i_clientid );
    Error::ErrorCode RequestChunkMaps( const std::string& i_floodhash );
    Error::ErrorCode SendChunkMaps( const std::string& i_floodhash );
    Error::ErrorCode NotifyHaveChunk( const std::string& i_floodhash,
                                      const std::string& i_filename,
                                      U32                i_chunkindex );
    Error::ErrorCode RequestChunk( const std::string& i_floodhash,
				   const std::string& i_filename,
				   U32                i_chunkindex );
    Error::ErrorCode SendChunk( const std::string& i_floodhash,
				const std::string& i_filename,
				U32                i_chunkindex );


    // member variables
  public:
    Client*     m_client;
    SocketSPtr  m_socket;
    std::string m_id;
    bool        m_disconnected;
    bool        m_connected;
    std::string m_host;
    U32         m_port;
    M_StrToMessageHandler m_messageHandlers;
    FakeClient            m_fakeClient;
    FakeServer            m_fakeServer;
    Reader                m_reader;
    Writer                m_writer;
    M_StrToStrToStr       m_chunkMaps;
    S_String              m_registeredFloods;

  protected:
    Error::ErrorCode _SetupMessageHandlers( void );
    Error::ErrorCode _Connect( void );
    Error::ErrorCode _ReadOnce( void );
    Error::ErrorCode _WriteOnce( void );
    Error::ErrorCode _ProcessReadBuffer( void );

    Error::ErrorCode _SendMessage( const std::string& i_methodName,
      const XmlRpcValue& i_args );
    Error::ErrorCode _DispatchMessage( const std::string& i_methodName,
      XmlRpcValue& i_args );

  };

  typedef std::vector< PeerConnectionSPtr > V_PeerConnectionSPtr;
};

#endif; // _LBF_PEER_CONNECTION_H_
