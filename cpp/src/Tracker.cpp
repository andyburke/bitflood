#include "stdafx.H"
#include "Tracker.H"
#include <time.h>
#include <sstream>

namespace libBitFlood
{
  // define our message names
  const char TrackerMessageHandler::RequestPeerList[] = "RequestPeerList";

  /*static*/ 
  Error::ErrorCode TrackerMessageHandler::AddTrackerHandlers( ClientSPtr& i_client )
  {
    i_client->AddMessageHandler( Client::MessageHandlerSPtr( new TrackerMessageHandler() ) );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode TrackerMessageHandler::QueryAPI( V_String& o_supportedmessages )
  {
    o_supportedmessages.push_back( RequestPeerList );
    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode TrackerMessageHandler::HandleMessage( const std::string& i_message, 
							 PeerConnectionSPtr& i_receiver,
							 XmlRpcValue& i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_message.compare( RequestPeerList ) == 0 )
    {
      ret = _HandleRequestPeerList( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode TrackerMessageHandler::_HandleRequestPeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    const std::string& filehash = i_args[0];

    XmlRpcValue result;
    result[0] = filehash;
    result[1];

    V_PeerConnectionSPtr::iterator iter = i_receiver->m_client->m_peers.begin();
    V_PeerConnectionSPtr::iterator end  = i_receiver->m_client->m_peers.end();

    U32 index = 0;
    for ( ; iter != end; ++iter )
    {
      if ( (*iter)->m_registeredFloods.find( filehash ) != (*iter)->m_registeredFloods.end() )
      {
        std::stringstream out;
        out << (*iter)->m_id << ":" << (*iter)->m_host << ":" << (*iter)->m_listenport;
        result[1][index++] = out.str();
      }
    }

    // respond with the list of peers
    i_receiver->SendMessage( Client::BasicMessageHandler::ReceivePeerList, result );

    return Error::NO_ERROR_LBF;
  }
};

