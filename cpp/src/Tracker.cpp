#include "stdafx.H"
#include "Tracker.H"
#include <time.h>
#include <sstream>

namespace libBitFlood
{
  Error::ErrorCode Tracker::Initialize( U32 port )
  {
    m_methods.push_back( new _Register ( this ) );
    m_methods.push_back( new _Disconnect ( this ) );
    m_methods.push_back( new _Dump ( this ) );
    m_methods.push_back( new _RequestPeers ( this ) );

    bindAndListen( port );
    enableIntrospection( true );

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Tracker::Run( double time )
  {
    // wait for requests for the time specified
    work( time );

    return Error::NO_ERROR_LBF;
  }

  Tracker::_Register::_Register(XmlRpcServer* s)
    : XmlRpcServerMethod( "Register", s )
  {
  }

  void Tracker::_Register::execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    std::string filehash = params[1];

    PeerInfo p;
    p.m_id = params[0];
    p.m_ip = params[2];
    p.m_port = (int)params[3];
    time( &p.m_timestamp );
    M_StrToPeerInfo::iterator iter = ((Tracker*)_server)->m_peerinfo.find( filehash );
    if ( iter != ((Tracker*)_server)->m_peerinfo.end() )
    {
      bool found = false;
      V_PeerInfo::iterator peeriter = (*iter).second.begin();
      V_PeerInfo::iterator peerend  = (*iter).second.end();
      for ( ; peeriter != peerend; ++peeriter )
      {
        if ( (*peeriter).m_id.compare( p.m_id ) == 0 )
        {
          found = true;
          time( &(peeriter->m_timestamp) );
          break;
        }
      }

      if ( !found )
      {
        (*iter).second.push_back( p );
      }
    }
    else
    {
      V_PeerInfo tmp;
      tmp.push_back( p );

      ((Tracker*)_server)->m_peerinfo[ filehash ] = tmp;
    }
  }

  Tracker::_Disconnect::_Disconnect(XmlRpcServer* s)
    : XmlRpcServerMethod( "Disconnect", s )
  {
  }

  void Tracker::_Disconnect::execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    std::string clientid = params[0];
    std::string filehash = params[1];

    M_StrToPeerInfo::iterator iter = ((Tracker*)_server)->m_peerinfo.find( filehash );
    if ( iter != ((Tracker*)_server)->m_peerinfo.end() )
    {
      V_PeerInfo::iterator peeriter = (*iter).second.begin();
      V_PeerInfo::iterator peerend  = (*iter).second.end();
      for ( ; peeriter != peerend; ++peeriter )
      {
        if ( (*peeriter).m_id.compare( clientid ) == 0 )
        {
          (*iter).second.erase( peeriter );
          break;
        }
      }
    }
  }

  Tracker::_Dump::_Dump(XmlRpcServer* s)
    : XmlRpcServerMethod( "Dump", s )
  {
  }

  void Tracker::_Dump::execute(XmlRpcValue& params, XmlRpcValue& result)
  {
  }

  Tracker::_RequestPeers::_RequestPeers(XmlRpcServer* s)
    : XmlRpcServerMethod( "RequestPeers", s )
  {
  }

  void Tracker::_RequestPeers::execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    std::string filehash = params[0];

    M_StrToPeerInfo::iterator iter = ((Tracker*)_server)->m_peerinfo.find( filehash );
    if ( iter != ((Tracker*)_server)->m_peerinfo.end() )
    {
      U32 index = 0;
      V_PeerInfo::iterator peeriter = (*iter).second.begin();
      V_PeerInfo::iterator peerend  = (*iter).second.end();
      for ( ; peeriter != peerend; ++peeriter, ++index )
      {
        std::stringstream out;
        out << (*peeriter).m_id << ":" << (*peeriter).m_ip << ":" << (*peeriter).m_port;
        result[index] = out.str();
      }
    }
  }
};

