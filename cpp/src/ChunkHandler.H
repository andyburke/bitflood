#ifndef _LBF_CHUNK_HANDLER_H_
#define _LBF_CHUNK_HANDLER_H_

#include "Client.H"

namespace libBitFlood
{
  class ChunkMessageHandler : public Client::MessageHandler
  {
  public:
    // define our message names
    static const char RequestChunkMaps[];
    static const char SendChunkMaps[];
    static const char RequestChunk[];
    static const char SendChunk[];
    static const char NotifyHaveChunk[];

    typedef std::map< std::string, std::string > M_StrToStr;
    typedef std::map< std::string, M_StrToStr > M_StrToStrToStr;
    typedef std::map< std::string, M_StrToStrToStr > M_StrToStrToStrToStr;

    M_StrToStrToStrToStr m_chunkMaps;

  public:
    Error::ErrorCode GetChunks( ClientSPtr& i_client );
    Error::ErrorCode QueryAPI( V_String& o_supportedmessages );
    Error::ErrorCode HandleMessage( const std::string&        i_message, 
                                    PeerConnectionSPtr& i_receiver,
                                    XmlRpcValue&              i_args );
  protected:
    // our handlers
    Error::ErrorCode _HandleRequestChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleSendChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleRequestChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleSendChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleNotifyHaveChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
  };

  typedef ReferenceCountTemplate< ChunkMessageHandler > ChunkMessageHandlerSPtr;
};

#endif; // _LBF_PEER_CONNECTION_H_
