package com.net.BitFlood;

/*
 * Created on Nov 12, 2004
 *
 */

import java.util.Vector;
import java.util.Iterator;

/**
 * @author burke
 *  
 */
public class SendPeerListMethodHandler implements MethodHandler
{
  final static String methodName = "SendPeerList";

  public String getMethodName()
  {
    return methodName;
  }

  public void HandleMethod( PeerConnection receiver, final Vector parameters )
  {
    Iterator paramiter = parameters.iterator();
    while ( paramiter.hasNext() )
    {
      final String peerInfo = (String) paramiter.next();

      int startIndex = 0;
      final String peerId = peerInfo.substring( startIndex, peerInfo.indexOf( ':', startIndex ) );
      startIndex += peerId.length() + 1;
      final String peerHost = peerInfo.substring( startIndex, peerInfo.indexOf( ':', startIndex ) );
      startIndex += peerHost.length() + 1;

      int peerPort = Integer.parseInt( peerInfo.substring( startIndex ) );

      // we don't need ourselves as a peer 
      if ( peerId.compareTo( receiver.localPeer.id ) != 0 )
      {
        PeerConnection peer = receiver.flood.FindPeer( peerId );

        if ( peer == null )
        {
          peer = new PeerConnection( receiver.flood, peerHost, peerPort, peerId );
          receiver.flood.peerConnections.add( peer );
        }
      }
    }
  }
}
