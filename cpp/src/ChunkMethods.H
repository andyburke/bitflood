#ifndef _LBF_CHUNK_METHODS_H_
#define _LBF_CHUNK_METHODS_H_

#include "Peer.H"

namespace libBitFlood
{
  class ChunkMethodHandler : public MethodHandler
  {
  public:
    // define our method names
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
    Error::ErrorCode GetChunks( PeerSPtr& i_localpeer );
    Error::ErrorCode QueryMethods( V_String& o_supportedmethods );
    Error::ErrorCode HandleMethod( const std::string&   i_method, 
                                    PeerConnectionSPtr& i_receiver,
                                    XmlRpcValue&        i_args );
  protected:
    // our handlers
    Error::ErrorCode _HandleRequestChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleSendChunkMaps( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleRequestChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleSendChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleNotifyHaveChunk( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
  };

  typedef ReferenceCountTemplate< ChunkMethodHandler > ChunkMethodHandlerSPtr;
};

#endif; // _LBF_CHUNK_METHODS_H_
