#ifndef _LBF_FLOOD_H_
#define _LBF_FLOOD_H_

#include <XmlRpc.h>
using namespace XmlRpc;
namespace libBitFlood
{
  // forward declare
  class Client;

  class FloodFile
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

    V_String    m_trackers;
    V_File      m_files;
    std::string m_contentHash;

  public:
    Error::ErrorCode ToXML( std::string& o_xml );
    Error::ErrorCode FromXML( const std::string& i_xml );
    Error::ErrorCode ComputeHash( std::string& o_hash );
  };

  typedef std::vector< FloodFile > V_FloodFile;

  typedef std::vector< class XmlRpcClient* > V_XmlRpcClientPtr;
  
  class Flood
  {
  public:
    Error::ErrorCode Initialize( const FloodFile& i_floodfile );
    Error::ErrorCode Register( const Client& i_client );
    Error::ErrorCode UpdatePeerList( Client& i_client );

    typedef std::map< std::string, U32 > M_StrToU32;
    struct RuntimeFile
    {
      M_StrToU32  m_chunkindices;
      V_U32       m_chunkoffsets;
      std::string m_chunkmap;
    };

    typedef std::pair< U32, U32 > P_ChunkKey;
    typedef std::vector< P_ChunkKey > V_ChunkKey;
    typedef std::vector< RuntimeFile > V_RuntimeFile;
    
    FloodFile         m_floodfile;
    V_XmlRpcClientPtr m_trackerConnections;
    V_RuntimeFile     m_runtimefiles;
    U32               m_totalbytes;
    V_ChunkKey        m_chunkstodownload;

  protected:
    Error::ErrorCode _SetupFilesAndChunks( void );
  };

  typedef std::vector< Flood > V_Flood;
};

#endif; // _LBF_FLOOD_H_
