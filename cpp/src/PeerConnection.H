#ifndef _LBF_PEER_H_
#define _LBF_PEER_H_

#include <Winsock2.h>
#include <XmlRpc.h>
#include <XmlRpcServerConnection.h>

using namespace XmlRpc;

namespace libBitFlood
{
  class Client;

  class PeerConnection
  {
  public:
    class FakeClient : public XmlRpcClient
    {
    public:
      FakeClient() : XmlRpcClient( "", 0 ) {}
      const std::string& GetRequest( void ) const { return _request; }
      bool ExternalGenerateRequest(const char* i_method, const XmlRpcValue& i_args) { return generateRequest( i_method, i_args ); }

    };

    class FakeServer : public XmlRpcServerConnection
    {
    public:
      FakeServer() : XmlRpcServerConnection( INVALID_SOCKET, NULL ) {}
      void SetRequest( const std::string& i_request ) { _request = i_request; }
      std::string ExternalParseRequest(XmlRpcValue& i_args) { return parseRequest( i_args); }
    };

    struct Reader
    {
      SOCKET      m_socket;
      std::string m_buffer;

      Error::ErrorCode Read( I32& o_bytesRead );
    };

    struct Writer
    {
      SOCKET      m_socket;
      std::string m_buffer;

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


    Error::ErrorCode InitializeIncoming( SOCKET s );
    Error::ErrorCode InitializeOutgoing( const std::string& i_peerId );

    Error::ErrorCode LoopOnce( void );

    // messages
    Error::ErrorCode Register( const std::string i_floodhash,
      const std::string i_clientid );
    Error::ErrorCode RequestChunkMaps( const std::string i_floodhash );
    Error::ErrorCode SendChunkMaps( const std::string i_floodhash );
    Error::ErrorCode NotifyHaveChunk( const std::string i_floodhash );
    Error::ErrorCode RequestChunk( const std::string i_floodhash );
    Error::ErrorCode SendChunk( const std::string i_floodhash );


    // member variables
  public:
    Client*     m_client;
    SOCKET      m_socket;
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

  typedef std::vector< PeerConnection* > V_PeerConnectionPtr;
};

#endif; // _LBF_PEER_H_
