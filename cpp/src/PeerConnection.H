#ifndef _LBF_PEER_H_
#define _LBF_PEER_H_

#include <Winsock2.h>

namespace libBitFlood
{
  class PeerConnection
  {
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
    Error::ErrorCode InitializeIncoming( SOCKET s,
					 const std::string& i_peerHost,
					 U32                i_peerPort );

    Error::ErrorCode InitializeOutgoing( const std::string& i_peerId,
					 const std::string& i_peerHost,
					 U32                i_peerPort );

    Error::ErrorCode LoopOnce( void );

    SOCKET      m_socket;
    std::string m_id;
    bool        m_disconnected;
    bool        m_connected;
    std::string m_host;
    U32         m_port;
    Reader      m_reader;
    Writer      m_writer;

  protected:
    Error::ErrorCode _Connect( void );
    Error::ErrorCode _ReadOnce( void );
    Error::ErrorCode _WriteOnce( void );
    Error::ErrorCode _ProcessReadBuffer( void );
  };

  typedef std::vector< PeerConnection* > V_PeerConnectionPtr;
};

#endif; // _LBF_PEER_H_
