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

  public:
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
