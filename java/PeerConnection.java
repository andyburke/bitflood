/*
 * Created on Nov 12, 2004
 *
 */

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.*;
import java.net.*;
import java.util.*;

/**
 * @author burke
 *
 */
public class PeerConnection 
{
  private Hashtable         messageHandlers   = null;

  private SocketChannel     socketChannel     = null;
  private Selector          socketSelector    = null;
  private InetSocketAddress socketAddress     = null;

  private int               BUFFER_SIZE       = 256 * 1024 * 2; // 2 times the normal chunk size
  
  private ByteBuffer        readBuffer        = ByteBuffer.allocate(BUFFER_SIZE);
  private int               readIndex         = 0;
  private ByteBuffer        writeBuffer       = ByteBuffer.allocate(BUFFER_SIZE);
  
  public Peer               parentPeer        = null;
  public boolean            connected         = false;
  public boolean            disconnected      = false;
  public String             hostname          = "";
  public int                port              = 0;
  
  public PeerConnection()
  {
  }
  
  public PeerConnection(String remoteHost, int remotePort)
  {
    hostname = remoteHost;
    port = remotePort;
    try
    {
      socketAddress = new InetSocketAddress(InetAddress.getByName(hostname), port);
    }
    catch(Exception e)
    {
      System.out.println("Error getting address for remote host (" + hostname + ":" + port + "): " + e);
      disconnected = true;
      return;
    }

  }
  
  public PeerConnection(SocketChannel incomingSocketConnection)
  {
    if(incomingSocketConnection == null)
    {
      System.out.println("Tried to create a PeerConnection with a null socket?");
      disconnected = true;
      return;
    }
  	
    socketChannel = incomingSocketConnection;
   
    try
    {
      socketSelector = Selector.open();
      socketChannel.configureBlocking( false );
    }
    catch(Exception e)
    {
      System.out.println("Error setting incoming connection to be non-blocking: " + e );
    }

    connected = true;
    if(!InitializeBufferedIO())
    {
      disconnected = true;
    }
  }

  private boolean InitializeBufferedIO()
  {
    if(!connected)
    {
      return false;
    }
  	
    try
    {
      socketChannel.register(socketSelector, socketChannel.validOps() );
    }
    catch(Exception e)
    {
      System.out.println("Error registering sockets on incoming connection: " + e);
      disconnected = true;
      return false;
    }

  	
    return true;
  }
  
  public void LoopOnce()
  {
    if ( socketSelector != null )
    {
      try
      {
        socketSelector.selectNow();
      }
      catch ( Exception e )
      {
        System.out.println("Error selecing from socketSelector: " + e );
      }
      
      // Get list of selection keys with pending events
      Iterator it = socketSelector.selectedKeys().iterator();
      while( it.hasNext() )
      {
        // Get the selection key
        SelectionKey selKey = (SelectionKey)it.next();
    
        // Remove it from the list to indicate that it is being processed
        it.remove();
    
        // 
        if ( selKey.isConnectable() )
        {
          System.out.println( "Connecting" );
          SocketChannel sChannel = (SocketChannel)selKey.channel();

          try
          {
            sChannel.finishConnect();
          }
          catch(Exception e)
          {
            System.out.println("Error connecting? " + e );
          }
        }

        if ( selKey.isReadable() )
        {
          ReadOnce();
        }

        if ( selKey.isWritable() )
        {
          WriteOnce();
        }
      }
    }

    ProcessReadBuffer();
  }
  
  public boolean Connect()
  {
    return false; 
  }

  private boolean ReadOnce()
  {
    try
    {
      int bytesRead = socketChannel.read(readBuffer);
    }
    catch(Exception e)
    {
      System.out.println("Error reading from socketChannel: " + e);
      disconnected = true;
      return false;
    }

    return true;
  }
  	
  private boolean WriteOnce()
  {
    try
    {
      int bytesWritten = socketChannel.write(writeBuffer);
    }
    catch(Exception e)
    {
      System.out.println("Error writing to socketChannel: " + e);
      disconnected = true;
      return false;
    }

    return true;
  }
 
  private boolean ProcessReadBuffer()
  {
    while( readBuffer.position() > readIndex )
    {
      byte b = readBuffer.get( readIndex++ );
      if ( b == '\n' )
      {
        int diff = readBuffer.position() - readIndex;
        byte[] message = new byte[readIndex];
        readBuffer.flip();
        readBuffer.get( message, 0, readIndex );
        readBuffer.compact();
        readBuffer.clear();
        readBuffer.position( diff );
        readIndex = 0;

        System.out.print("Got message: '" );
        for ( int i = 0; i < message.length; i++ )
        {
          System.out.print( (char)message[i] );
        }
        System.out.println( "'" );
      }
    }

    return true;
  }
  
}
