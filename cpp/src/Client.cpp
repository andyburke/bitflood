#include "stdafx.H"
#include "Client.H"
#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>

namespace libBitFlood
{
  Error::ErrorCode Client::Initialize( const Setup& i_setup )
  {
    // cache the setup
    m_setup = i_setup;

    // compute an ID
    {
      std::stringstream idstrm;
      idstrm << m_setup.m_localIP << m_setup.m_localPort;

      using namespace CryptoPP;
      SHA sha;
      HashFilter shaFilter(sha);
      std::auto_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
      channelSwitch->AddDefaultRoute(shaFilter);

      StringSource( (const byte*)idstrm.str().c_str(), idstrm.str().length(), true, channelSwitch.release() );
      std::stringstream out;
      Base64Encoder encoder( new FileSink( out ), false );
      shaFilter.TransferTo( encoder );

      m_id = out.str();
    }

    // Open a listen socket
    _OpenListenSocket();

    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::AddFloodFile( const FloodFile& i_floodfile )
  {
    Flood f;
    f.Initialize( i_floodfile );
    m_floods.push_back( f );

    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::Register( void )
  {
    V_Flood::iterator flooditer = m_floods.begin();
    V_Flood::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      Flood& flood = *flooditer;
      flood.Register( *this );
    }

    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::UpdatePeerList( void )
  {
    V_Flood::iterator flooditer = m_floods.begin();
    V_Flood::iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
      Flood& flood = *flooditer;
      flood.UpdatePeerList();
    }

    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::GetChunks( void )
  {
    V_Flood::const_iterator flooditer = m_floods.begin();
    V_Flood::const_iterator floodend  = m_floods.end();

    for ( ; flooditer != floodend; ++flooditer )
    {
    }

    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::LoopOnce( void )
  {
    _ProcessPeers();

    // if socket can read
    // if socked accept
    // add connection

    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::_OpenListenSocket( void )
  {
    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::_ProcessPeers( void )
  {
    return Error::NO_ERROR;
  }

  Error::ErrorCode Client::_AddConnection( void )
  {
    return Error::NO_ERROR;
  }
};

