#ifndef _LBF_FLOOD_H_
#define _LBF_FLOOD_H_

namespace libBitFlood
{
  class Flood
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

    struct File
    {
      std::string m_name;
      U32         m_size;
      V_Chunk     m_chunks;
    };

    typedef std::vector< File > V_File;

    std::string m_tracker;
    V_File      m_files;

  public:
    Error::ErrorCode ToXML( std::wstring& o_xml );
    Error::ErrorCode FromXML( const std::wstring& i_xml );
  };
};

#endif; // _LBF_FLOOD_H_
