package com.net.BitFlood.method;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.*;
import com.net.BitFlood.peerconnection.SocketConnection;

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
      if (peer instanceof SocketConnection) { // TODO: handle this more elegantly
        SocketConnection socketPeer = (SocketConnection) peer;
        responseParams.add( new String( socketPeer.id + ":" + socketPeer.hostname + ":" + socketPeer.listenPort ) );
      }
    }
    
    receiver.SendMethod( "SendPeerList", responseParams );
  }
}
