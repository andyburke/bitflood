#include "stdafx.H"
#include "TrackerMethods.H"
#include "PeerMethods.H"

#include <time.h>
#include <sstream>

namespace libBitFlood
{
  // define our method names
  const char TrackerMethodHandler::RequestPeerList[] = "RequestPeerList";

  Error::ErrorCode TrackerMethodHandler::HandleMethod( const std::string& i_method, 
                                                       PeerConnectionSPtr& i_receiver,
                                                       XmlRpcValue& i_args )
  {
    Error::ErrorCode ret = Error::UNKNOWN_ERROR_LBF;
    if( i_method.compare( RequestPeerList ) == 0 )
    {
      ret = _HandleRequestPeerList( i_receiver, i_args );
    }

    return ret;
  }

  Error::ErrorCode TrackerMethodHandler::_HandleRequestPeerList( PeerConnectionSPtr& i_receiver, XmlRpcValue& i_args )
  {
    XmlRpcValue result;
    result[0];

    V_PeerConnectionSPtr::iterator iter = i_receiver->m_flood->m_peers.begin();
    V_PeerConnectionSPtr::iterator end  = i_receiver->m_flood->m_peers.end();

    U32 index = 0;
    for ( ; iter != end; ++iter )
    {
      std::stringstream out;
      out << (*iter)->m_id << ":" << (*iter)->m_host << ":" << (*iter)->m_listenport;
      result[0][index++] = out.str();
    }

    // respond with the list of peers
    i_receiver->SendMethod( PeerMethodHandler::ReceivePeerList, result );

    return Error::NO_ERROR_LBF;
  }
};

