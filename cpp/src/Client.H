#ifndef _LBF_CLIENT_H_
#define _LBF_CLIENT_H_

#include "Flood.H"

namespace libBitFlood
{
  class Client
  {
  public:
    struct Setup
    {
      std::string m_localIP;
      U32         m_localPort;
    };

    Setup m_setup;
    V_Flood m_floods;
    std::string m_id;

  public:
    Error::ErrorCode Initialize( const Setup& i_setup );
    Error::ErrorCode AddFloodFile( const FloodFile& i_floodfile );
    Error::ErrorCode Register( void );
    Error::ErrorCode UpdatePeerList( void );
    Error::ErrorCode GetChunks( void );
    Error::ErrorCode LoopOnce( void );

  protected:
    Error::ErrorCode _OpenListenSocket( void );
    Error::ErrorCode _ProcessPeers( void );
    Error::ErrorCode _AddConnection( void );
  };
};

#endif; // _LBF_CLIENT_H_
