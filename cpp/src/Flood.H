#ifndef _LBF_FLOOD_H_
#define _LBF_FLOOD_H_

#include "FloodFile.H"
#include "PeerConnection.H"

namespace libBitFlood
{
  class Flood : public ReferenceCountBase
  {
  public:
    struct RuntimeFile
    {
      V_U32               m_chunkoffsets;
      std::string         m_chunkmap;
      FloodFile::FileSPtr m_file;
    };

    struct ChunkDownload
    {
      time_t             m_start;
      PeerConnectionSPtr m_from;
    };

    typedef std::pair< std::string, U32 > P_ChunkKey;
    typedef std::map< P_ChunkKey, ChunkDownload > M_P_ChunkKeyToChunkDownload;
    typedef std::set< P_ChunkKey > S_ChunkKey;
    typedef std::map< std::string, RuntimeFile > M_StrToRuntimeFile;

    Peer*                       m_localpeer;
    FloodFileSPtr               m_floodfile;
    M_StrToRuntimeFile          m_runtimefiles;
    U32                         m_totalbytes;
    S_ChunkKey                  m_chunkstodownload;
    M_P_ChunkKeyToChunkDownload m_chunksdownloading;
    V_PeerConnectionSPtr        m_peers;
    time_t                      m_lasttrackerupdate;

    Error::ErrorCode Initialize( PeerSPtr& i_localpeer, FloodFileSPtr& i_floodfile );
    Error::ErrorCode GetChunks( void );
    Error::ErrorCode UpdateTrackers( void );
    Error::ErrorCode LoopOnce( void );
    Error::ErrorCode InqPeer( const std::string& i_peerid, PeerConnectionSPtr& o_peer );

  protected:
    Error::ErrorCode _SetupFilesAndChunks( void );
  };
};

#endif; // _LBF_FLOOD_H_
