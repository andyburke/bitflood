#ifndef _LBF_FLOOD_FILE_H_
#define _LBF_FLOOD_FILE_H_

namespace libBitFlood
{
  class FloodFile : public ReferenceCountBase
  {
  public:
    struct Chunk
    {
      std::string m_hash;
      U32         m_index;
      U32         m_size; 
      U32         m_weight;
    };

    typedef std::vector< Chunk > V_Chunk;

    class File : public ReferenceCountBase
    {
    public:
      std::string m_name;
      U32         m_size;
      V_Chunk     m_chunks;
    };

    typedef ReferenceCountTemplate< File > FileSPtr;
    typedef std::map< std::string, FileSPtr > M_StrToFileSPtr;

    struct TrackerInfo
    {
      std::string m_host;
      U32         m_port;
      std::string m_id;
    };

    typedef std::vector< TrackerInfo > V_TrackerInfo;

    V_TrackerInfo   m_trackers;
    M_StrToFileSPtr m_files;
    std::string     m_contentHash;

  public:
    Error::ErrorCode ToXML( std::string& o_xml );
    Error::ErrorCode FromXML( const std::string& i_xml );
    Error::ErrorCode ComputeHash( std::string& o_hash );
  };
  typedef std::vector< FloodFile > V_FloodFile;
};

#endif; // _LBF_FLOOD_FILE_H_
