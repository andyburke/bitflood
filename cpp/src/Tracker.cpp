#include "stdafx.H"
#include "Tracker.H"
#include <time.h>
#include <sstream>

namespace libBitFlood
{
  namespace Tracker
  {
    // define our message names
    namespace Messages
    {
      const char RegisterWithTracker[]   = "RegisterWithTracker";
      const char DisconnectFromTracker[] = "DisconnectFromTracker";
      const char RequestPeerList[]       = "RequestPeerList";
    };

    // forward declare the handlers
    Error::ErrorCode _HandleRegisterWithTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleDisconnectFromTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
    Error::ErrorCode _HandleRequestPeerList( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args );
  }

  Error::ErrorCode Tracker::AddTrackerHandlers( ClientSPtr& i_client )
  {
    using namespace Messages;
    i_client->AddMessageHandler( RegisterWithTracker,   _HandleRegisterWithTracker );
    i_client->AddMessageHandler( DisconnectFromTracker, _HandleDisconnectFromTracker );
    i_client->AddMessageHandler( RequestPeerList,       _HandleRequestPeerList );
    return Error::NO_ERROR_LBF;
  }


  /*static*/ Error::ErrorCode Tracker::State::GetTrackerState( StateSPtr& o_trackerstate )
  {
    static StateSPtr s_trackerstate;
    if ( s_trackerstate.Get() == NULL )
    {
      s_trackerstate = StateSPtr( new State() );
    }
    o_trackerstate = s_trackerstate;
    return Error::NO_ERROR_LBF;
  }


  Error::ErrorCode Tracker::_HandleRegisterWithTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    StateSPtr tracker;
    State::GetTrackerState( tracker );

    std::string filehash = i_args[1];

    State::PeerInfo p;
    p.m_id = i_args[0];
    p.m_ip = i_args[2];
    p.m_port = (int)i_args[3];
    time( &p.m_timestamp );
    State::M_StrToPeerInfo::iterator iter = tracker->m_peerinfo.find( filehash );
    if ( iter != tracker->m_peerinfo.end() )
    {
      bool found = false;
      State::V_PeerInfo::iterator peeriter = (*iter).second.begin();
      State::V_PeerInfo::iterator peerend  = (*iter).second.end();
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
      State::V_PeerInfo tmp;
      tmp.push_back( p );

      tracker->m_peerinfo[ filehash ] = tmp;
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Tracker::_HandleDisconnectFromTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    StateSPtr tracker;
    State::GetTrackerState( tracker );

    std::string clientid = i_args[0];
    std::string filehash = i_args[1];

    State::M_StrToPeerInfo::iterator iter = tracker->m_peerinfo.find( filehash );
    if ( iter != tracker->m_peerinfo.end() )
    {
      State::V_PeerInfo::iterator peeriter = (*iter).second.begin();
      State::V_PeerInfo::iterator peerend  = (*iter).second.end();
      for ( ; peeriter != peerend; ++peeriter )
      {
        if ( (*peeriter).m_id.compare( clientid ) == 0 )
        {
          (*iter).second.erase( peeriter );
          break;
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Tracker::_HandleRequestPeerList( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    XmlRpcValue result;
    StateSPtr tracker;
    State::GetTrackerState( tracker );

    std::string filehash = i_args[0];

    State::M_StrToPeerInfo::iterator iter = tracker->m_peerinfo.find( filehash );
    if ( iter != tracker->m_peerinfo.end() )
    {
      U32 index = 0;
      State::V_PeerInfo::iterator peeriter = (*iter).second.begin();
      State::V_PeerInfo::iterator peerend  = (*iter).second.end();
      for ( ; peeriter != peerend; ++peeriter, ++index )
      {
        std::stringstream out;
        out << (*peeriter).m_id << ":" << (*peeriter).m_ip << ":" << (*peeriter).m_port;
        result[index] = out.str();
      }
    }

    return Error::NO_ERROR_LBF;
  }
};

