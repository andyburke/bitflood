package com.net.BitFlood.method;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.*;
import java.util.Vector;
import java.util.Iterator;

/**
 * @author burke
 *  
 */
public class RequestPeerListMethod implements MethodHandler
{
  final static public String methodName = "RequestPeerList";
  public String getMethodName()
  {
    return methodName;
  }
  
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    if ( receiver.flood == null )
    {
      throw new Exception( "flood specific method from an unregistered peer!");
    }
    
    Vector responseParams = new Vector();
    
    Iterator peeriter = receiver.flood.peerConnections.iterator();
    while( peeriter.hasNext() )
    {
      final PeerConnection peer = (PeerConnection)peeriter.next();
      responseParams.add( new String( peer.id + ":" + peer.hostname + ":" + peer.listenPort ) );
    }
    
    receiver.SendMethod( "SendPeerList", responseParams );
  }
}
