package com.net.BitFlood.method;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.*;
import com.net.BitFlood.peerconnection.SocketConnection;

import java.util.Vector;

/**
 * @author burke
 *  
 */
public class RegisterMethod implements MethodHandler
{
  // TODO: refactor this static string / getmethodname() stuff
  final static public String methodName = "Register";
  public String getMethodName()
  {
    return methodName;
  }
  
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    final String floodId = (String) parameters.elementAt( 0 );
    final String peerId = (String) parameters.elementAt( 1 );
    final Integer peerListen = (Integer) parameters.elementAt( 2 );

    if ( receiver.flood != null )
    {
      if ( receiver.flood.Id().compareTo( floodId ) != 0 )
      {
        throw new Exception( receiver + " already registered, attempting to register with a different flood?" );
      }
      
      Logger.LogNormal( receiver + "is already registered with this flood.. string but ok.." );
      return;
    }

    Flood flood = receiver.localPeer.FindFlood( floodId );
    if ( flood == null )
    {
      receiver.disconnected = true;
      throw new Exception( receiver + " attempting to join unknown flood: " + floodId  );
    }

    PeerConnection existingPeer = flood.FindPeer( peerId );
    if ( existingPeer != null )
    {
      receiver.disconnected = true;
      throw new Exception( receiver + " has already registered with the flood: " +  floodId );
    }

    if ( !flood.localPeer.pendingPeers.remove( receiver ) )
    {
      receiver.disconnected = true;
      throw new Exception( "Receiver could not be removed from pending queue" );
    }

    receiver.id = peerId;
    receiver.flood = flood;
    if ( receiver instanceof SocketConnection )    // TODO: handle this more elegantly
    {
      ((SocketConnection) receiver).listenPort = peerListen.intValue();
    }
    
    // ask for chunks
    receiver.SendMethod( RequestChunkMapsMethod.methodName, new Vector() );
    
    flood.peerConnections.add( receiver );
    Logger.LogNormal( "Registered peer: " + receiver + " with the flood: " + floodId );
  }
}
