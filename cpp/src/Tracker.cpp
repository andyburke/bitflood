#include "stdafx.H"
#include "Tracker.H"
#include <time.h>
#include <sstream>

namespace libBitFlood
{
  // define our message names
  const char TrackerMessageHandler::RegisterWithTracker[]   = "RegisterWithTracker";
  const char TrackerMessageHandler::DisconnectFromTracker[] = "DisconnectFromTracker";
  const char TrackerMessageHandler::RequestPeerList[]       = "RequestPeerList";

  /*static*/ 
  Error::ErrorCode TrackerMessageHandler::AddTrackerHandlers( ClientSPtr& i_client )
  {
    i_client->AddMessageHandler( Client::MessageHandlerSPtr( new TrackerMessageHandler() ) );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode TrackerMessageHandler::QueryAPI( V_String& o_supportedmessages )
  {
    o_supportedmessages.push_back( RegisterWithTracker );
    o_supportedmessages.push_back( DisconnectFromTracker );
    o_supportedmessages.push_back( RequestPeerList );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode TrackerMessageHandler::HandleMessage( const std::string& i_message, 
							 const PeerConnectionSPtr& i_receiver,
							 XmlRpcValue& i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_message.compare( RegisterWithTracker ) == 0 )
    {
      ret = _HandleRegisterWithTracker( i_receiver, i_args );
    }
    else if( i_message.compare( DisconnectFromTracker ) == 0 )
    {
      ret = _HandleDisconnectFromTracker( i_receiver, i_args );
    }
    else if( i_message.compare( RequestPeerList ) == 0 )
    {
      ret = _HandleRequestPeerList( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode TrackerMessageHandler::_HandleRegisterWithTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    std::string filehash = i_args[1];

    PeerInfo p;
    p.m_id = i_args[0];
    p.m_ip = i_args[2];
    p.m_port = (int)i_args[3];
    time( &p.m_timestamp );
    M_StrToPeerInfo::iterator iter = m_peerinfo.find( filehash );
    if ( iter != m_peerinfo.end() )
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

      m_peerinfo[ filehash ] = tmp;
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode TrackerMessageHandler::_HandleDisconnectFromTracker( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    std::string clientid = i_args[0];
    std::string filehash = i_args[1];

    M_StrToPeerInfo::iterator iter = m_peerinfo.find( filehash );
    if ( iter != m_peerinfo.end() )
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

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode TrackerMessageHandler::_HandleRequestPeerList( const PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    XmlRpcValue result;
    std::string filehash = i_args[0];

    M_StrToPeerInfo::iterator iter = m_peerinfo.find( filehash );
    if ( iter != m_peerinfo.end() )
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

    return Error::NO_ERROR_LBF;
  }
};

