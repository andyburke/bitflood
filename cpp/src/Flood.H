#ifndef _LBF_FLOOD_H_
#define _LBF_FLOOD_H_

#include "FloodFile.H"
#include "PeerConnection.H"

namespace libBitFlood
{
  class Flood : public ReferenceCountBase
  {
  public:
    typedef std::map< std::string, U32 > M_StrToU32;
    struct RuntimeFile
    {
      M_StrToU32  m_chunkindices;
      V_U32       m_chunkoffsets;
      std::string m_chunkmap;
    };

    struct TrackerInfo
    {
      std::string m_host;
      U32         m_port;
      std::string m_id;
    };

    typedef std::pair< U32, U32 > P_ChunkKey;
    typedef std::set< P_ChunkKey > S_ChunkKey;
    typedef std::vector< P_ChunkKey > V_ChunkKey;
    typedef std::vector< RuntimeFile > V_RuntimeFile;
    typedef std::vector< TrackerInfo > V_TrackerInfo;
    typedef std::map< std::string, std::string > M_StrToStr;
    typedef std::map< std::string, M_StrToStr > M_StrToStrToStr;

    Peer*                m_localpeer;
    FloodFileSPtr        m_floodfile;
    V_TrackerInfo        m_trackerinfos;
    V_RuntimeFile        m_runtimefiles;
    U32                  m_totalbytes;
    V_ChunkKey           m_chunkstodownload;
    S_ChunkKey           m_chunksdownloading;
    V_PeerConnectionSPtr m_peers;
    time_t               m_lasttrackerupdate;
    M_StrToStrToStr      m_peerchunkmaps;

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
