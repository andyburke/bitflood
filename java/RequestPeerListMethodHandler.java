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
public class RequestPeerListMethodHandler implements MethodHandler
{
  public String getMethodName()
  {
    return "RequestPeerList";
  }
  
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    if ( receiver.flood == null )
    {
      throw new Exception( "flood specific method from an unregistered peer!");
    }
    
    Vector responseParams = new Vector();
    
    Iterator peeriter = receiver.flood.peers.iterator();
    while( peeriter.hasNext() )
    {
      final PeerConnection peer = (PeerConnection)peeriter.next();
      responseParams.add( new String( peer.id + ":" + peer.hostname + ":" + peer.listenPort ) );
    }
    
    receiver.SendMethod( "SendPeerList", responseParams );
  }
}