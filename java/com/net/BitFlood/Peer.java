package com.net.BitFlood;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.method.*;
import java.util.*;
import java.nio.channels.*;
import java.net.*;

/**
 * @author burke
 *  
 */
public class Peer
{
  public Vector               pendingPeers         = new Vector();
  private Hashtable           floods               = new Hashtable();
  private ServerSocketChannel listenSocketChannel  = null;
  private Selector            listenSocketSelector = null;
  public String               hostname             = "";
  public int                  port                 = 0;
  public String               id                   = "";
  private Hashtable           methodHandlers       = new Hashtable();

  public String toString()
  {
    return new String( id + ":" + hostname + ":" + port );
  }
  
  public Peer()
  {
  }

  public Peer(String localHost, int localPort)
  {
    hostname = localHost;
    port = localPort;

    String idString = hostname + port;
    id = Encoder.SHA1Base64Encode( idString );

    SetupListenSocket();

    AddMethodHandler( new RegisterMethod() );
    AddMethodHandler( new SendPeerListMethod() );
  }

  public boolean JoinFlood( String floodFilename )
  {
    return JoinFlood(floodFilename, true); // init files by default
  }
  
  public boolean JoinFlood( String floodFilename, boolean initializeLocalFiles)
  {
    Flood floodToJoin = new Flood( this, floodFilename );
    if ( floodToJoin.Id() != null )
    {
      floods.put( floodToJoin.Id(), floodToJoin );
      if( initializeLocalFiles )
      {
        floodToJoin.SetupFilesAndChunks();
      }
      return true;
    }
    return false;    
  }

  public void LoopOnce()
  {
    if ( floods != null )
    {
      Collection floodsToProcess = floods.values();
      Iterator floodIter = floodsToProcess.iterator();
      while ( floodIter.hasNext() )
      {
        Flood flood = (Flood) floodIter.next();
        flood.LoopOnce();
      }
    }

    if ( pendingPeers != null )
    {
      // clone the pendingPeers Vector and then process them
      {
        Vector ppClone = (Vector) pendingPeers.clone();
        Iterator peeriter = ppClone.iterator();
        for ( ; peeriter.hasNext(); )
        {
          PeerConnection peer = (PeerConnection) peeriter.next();
          peer.LoopOnce();
        }
      }

      // reap disconnected peers
      Iterator peeriter = pendingPeers.iterator();
      for ( ; peeriter.hasNext(); )
      {
        PeerConnection peer = (PeerConnection) peeriter.next();
        if ( peer.disconnected )
        {
          Logger.LogNormal( "Reaping " + peer + " (pending)");
          peeriter.remove();
        }
      }
    }

    if ( listenSocketSelector != null )
    {
      try
      {
        listenSocketSelector.selectNow();
      }
      catch ( Exception e )
      {
        Logger.LogError( "Error selecing from listenSocketSelector: " + e );
      }

      // if the socket can accept, test for an incoming connection
      // Get list of selection keys with pending events
      Iterator it = listenSocketSelector.selectedKeys().iterator();
      while ( it.hasNext() )
      {
        // Get the selection key
        SelectionKey selKey = (SelectionKey) it.next();

        // Remove it from the list to indicate that it is being processed
        it.remove();

        // Check if it's a connection request
        if ( selKey.isAcceptable() )
        {
          int tmp = listenSocketChannel.socket().getLocalPort();
          SocketChannel incomingSocket = null;
          try
          {
            incomingSocket = listenSocketChannel.accept();
          }
          catch ( Exception e )
          {
            Logger.LogError( "Error accepting connection from listenSocketChannel: " + e );
          }

          if ( incomingSocket != null )
          {
            PeerConnection incomingPeer = new PeerConnection( this, incomingSocket );
            pendingPeers.add( incomingPeer );
          }
        }
      }
    }
  }

  private void SetupListenSocket()
  {
    try
    {
      listenSocketChannel = ServerSocketChannel.open();
      listenSocketChannel.configureBlocking( false );
      listenSocketChannel.socket().bind( new InetSocketAddress( port ) );
      listenSocketChannel.socket().setReuseAddress( true );
      listenSocketSelector = Selector.open();
      listenSocketChannel.register( listenSocketSelector, SelectionKey.OP_ACCEPT );
    }
    catch ( Exception e )
    {
      Logger.LogError( "Error making listen socket: " + e );
    }
  }

  public Flood FindFlood( final String floodId )
  {
    return (Flood) floods.get( floodId );
  }

  public void HandleMethod( PeerConnection receiver, final String methodName, final Vector parameters )
  {
    MethodHandler handler = (MethodHandler) methodHandlers.get( methodName );
    if ( handler != null )
    {
      try
      {
        handler.HandleMethod( receiver, parameters );
      }
      catch ( Exception e )
      {
        Logger.LogError( "Failed to execute method: " + methodName + " from peer: " + receiver + ": " + e );
      }
    }
  }

  public void ActAsTracker()
  {
    AddMethodHandler( new RequestPeerListMethod() );
  }

  public void ActAsSeed()
  {
    AddMethodHandler( new RequestChunkMapsMethod() );
    AddMethodHandler( new RequestChunkMethod() );
  }

  public void ActAsLeech()
  {
    AddMethodHandler( new SendChunkMapsMethod() );
    AddMethodHandler( new SendChunkMethod() );
    AddMethodHandler( new NotifyHaveChunkMethod() );
  }

  protected void AddMethodHandler( MethodHandler handler )
  {
    methodHandlers.put( handler.getMethodName(), handler );
  }
}
