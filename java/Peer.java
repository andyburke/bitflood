/*
 * Created on Nov 12, 2004
 *
 */

import java.util.*;
import java.nio.channels.*;
import java.net.*;

/**
 * @author burke
 *
 */
public class Peer 
{
  private Vector           pendingpeers;
  private Hashtable        floods                     = null;

  private ServerSocketChannel listenSocketChannel   = null;
  private Selector            listenSocketSelector  = null;
  private SelectionKey        listenSocketReadyKey  = null;
  private InetSocketAddress   listenSocketAddress   = null;

  public String             hostname                  = "";
  public int                port                      = 0;

  public Peer()
  {
  }
    
  public Peer( String localHost, int localPort ) 
  {
    hostname = localHost;
    port = localPort;
    SetupListenSocket();
    
    floods = new Hashtable();
  }
  
  public boolean JoinFlood(String floodFilename) 
  {
    Flood floodToJoin = new Flood(floodFilename);
    if(floodToJoin.Id() != null)
    {
      floods.put(floodToJoin.Id(), floodToJoin);
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
      while(floodIter.hasNext())
      {
        Flood flood = (Flood)floodIter.next();
        flood.LoopOnce();
      }
    }

    if( pendingpeers != null )
    {
      Enumeration peeriter = pendingpeers.elements();
      for ( ; peeriter.hasMoreElements() ; )
      {
        PeerConnection peer = (PeerConnection)peeriter.nextElement();
        peer.LoopOnce();
      }
    }

    // if the socket can accept, test for an incoming connection
    if( listenSocketReadyKey != null && listenSocketReadyKey.isAcceptable() )
    {
      SocketChannel incomingSocket = null;
      try
      {
        incomingSocket = listenSocketChannel.accept();
      }
      catch ( Exception e )
      {
        System.out.println("Error accepting connection from listenSocketChannel: " + e );
      }

      if( incomingSocket != null )
      {
        PeerConnection incomingPeer = new PeerConnection( incomingSocket );
        pendingpeers.add( incomingPeer );
      }
    }
  }

  private void SetupListenSocket()
  {
    try
    {
      listenSocketAddress = new InetSocketAddress( InetAddress.getByName(hostname), port );
    }
    catch ( Exception e )
    {
      System.out.println("Error making listen socket address: " + e );
    }

    try
    {
      listenSocketChannel = ServerSocketChannel.open();
    }
    catch ( Exception e )
    {
      System.out.println("Error making listen socket: " + e );
    }
    
    try
    {
      listenSocketChannel.configureBlocking(false);
    }
    catch ( Exception e )
    {
      System.out.println("Error setting listen socket non-blocking: " + e );
    }

    try
    {
      listenSocketChannel.socket().setReuseAddress( true );
    }
    catch ( Exception e )
    {
      System.out.println("Error setting resuse address: " + e );
    }

    try
    {
      listenSocketChannel.socket().bind( listenSocketAddress );
    }
    catch ( Exception e )
    {
      System.out.println("Error binding to address: " + e );
    }

    try
    {
      listenSocketSelector = Selector.open();
    }
    catch ( Exception e )
    {
      System.out.println("Error making the listen selector: " + e );
    }
    
    try
    {
      listenSocketReadyKey = listenSocketChannel.register(listenSocketSelector, SelectionKey.OP_ACCEPT);
    }
    catch ( Exception e )
    {
      System.out.println("Error making the listen socket read key: " + e );
    }
  }
}
