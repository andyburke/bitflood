/*
 * Created on Nov 12, 2004
 *
 */

import java.util.Vector;

/**
 * @author burke
 *  
 */
public class RegisterMethodHandler implements MethodHandler
{
  public String getMethodName()
  {
    return "Register";
  }
  
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    final String floodId = (String) parameters.elementAt( 0 );
    final String peerId = (String) parameters.elementAt( 1 );
    final Integer peerListen = (Integer) parameters.elementAt( 2 );

    if ( receiver.flood != null )
    {
      throw new Exception( "Receiver is already Registered!" );
    }

    Flood flood = receiver.localPeer.FindFlood( floodId );
    if ( flood == null )
    {
      throw new Exception( "Received a register for an unknown flood!" );
    }

    PeerConnection existingPeer = flood.FindPeer( peerId );
    if ( existingPeer != null )
    {
      throw new Exception( "Peer has already registered with this flood!" );
    }

    if ( !flood.localPeer.pendingPeers.remove( receiver ) )
    {
      throw new Exception( "Receiver could not be removed from pending queue" );
    }

    receiver.id = peerId;
    receiver.listenPort = peerListen.intValue();
    receiver.flood = flood;
    flood.peers.add( receiver );
  }
}