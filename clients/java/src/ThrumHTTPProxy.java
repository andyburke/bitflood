import java.net.InetSocketAddress;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;

import java.util.*;

import com.net.BitFlood.*;

/**
 * Created on Dec 5, 2004
 *
 */

/**
 * @author burke
 *
 */
public class ThrumHTTPProxy
{
  private int port = -1;

  public Peer peer = null;
  
  private ServerSocketChannel listenSocketChannel = null;
  private Selector listenSocketSelector = null;
  
  private Vector connections = null;
  
  public ThrumHTTPProxy(Peer peer, int port) 
  {
    this.peer = peer;
    this.port = port;
  }
  
  public ThrumHTTPProxy(Peer peer)
  {
    this(peer, 8080);
  }
  
  public ThrumHTTPProxy(String floodFilename)
  {
    JoinFlood(floodFilename);
    port = 8080;
  }
  
  public void JoinFlood(String floodFilename)
  {
    if(peer == null)
    {
      peer = new Peer("127.0.0.1", 10109);
      peer.ActAsLeech();
    }
    
    peer.JoinFlood(floodFilename);
      
  }
  
  public boolean Initialize()
  {
    connections = new Vector();
    
    return SetupListenSocket();
  }
  
  private boolean SetupListenSocket()
  {
    try
    {
      listenSocketChannel = ServerSocketChannel.open();
      listenSocketChannel.configureBlocking( false );
      listenSocketChannel.socket().bind( new InetSocketAddress( port ) );
      listenSocketChannel.socket().setReuseAddress( true );
      listenSocketSelector = Selector.open();
      listenSocketChannel.register( listenSocketSelector, SelectionKey.OP_ACCEPT );
      return true;
    }
    catch ( Exception e )
    {
      e.printStackTrace();
      return false;
    }
  }
  
  public void LoopOnce()
  {
    peer.LoopOnce();
    
    if(connections != null)
    {
      Iterator connectionIter = connections.iterator();
      while(connectionIter.hasNext())
      {
        HTTPConnection httpconn = (HTTPConnection) connectionIter.next();
        if(!httpconn.disconnected)
        {
          httpconn.LoopOnce();
        }
        else
        {
          connectionIter.remove();
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
        e.printStackTrace();
        return;
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
            e.printStackTrace();
            return;
          }

          if ( incomingSocket != null )
          {
            System.out.println("Spinning off incoming connection...");
            connections.add(new HTTPConnection(this, incomingSocket));
          }
        }
      }
    }
  }

}
