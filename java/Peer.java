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
  private Vector           pendingpeers               = null;
  private Hashtable        floods                     = null;

  private ServerSocketChannel listenSocketChannel   = null;
  private Selector            listenSocketSelector  = null;

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
    
    pendingpeers = new Vector();
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

    if ( listenSocketSelector != null )
    {
      try
      {
        listenSocketSelector.selectNow();
      }
      catch ( Exception e )
      {
        System.out.println("Error selecing from listenSocketSelector: " + e );
      }

      // if the socket can accept, test for an incoming connection
      // Get list of selection keys with pending events
      Iterator it = listenSocketSelector.selectedKeys().iterator();
      while( it.hasNext() )
      {
        // Get the selection key
        SelectionKey selKey = (SelectionKey)it.next();
    
        // Remove it from the list to indicate that it is being processed
        it.remove();
    
        // Check if it's a connection request
        if (selKey.isAcceptable()) 
        {
          int tmp = listenSocketChannel.socket().getLocalPort();
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
    }
  }

  private void SetupListenSocket()
  {
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
      listenSocketChannel.socket().bind( new InetSocketAddress( port ) );
    }
    catch ( Exception e )
    {
      System.out.println("Error binding to address: " + e );
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
      listenSocketSelector = Selector.open();
    }
    catch ( Exception e )
    {
      System.out.println("Error making the listen selector: " + e );
    }
    
    try
    {
      listenSocketChannel.register(listenSocketSelector, SelectionKey.OP_ACCEPT);
    }
    catch ( Exception e )
    {
      System.out.println("Error registering listen socket selector: " + e );
    }
  }
}
