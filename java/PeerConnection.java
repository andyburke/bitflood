/*
 * Created on Nov 12, 2004
 *
 */

import java.nio.ByteBuffer;
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
  private SelectionKey      socketConnectKey  = null;
  private InetSocketAddress socketAddress     = null;

  private int               BUFFER_SIZE       = 256 * 1024 * 2; // 2 times the normal chunk size
  
  private SelectionKey      readerReadyKey    = null;
  private ByteBuffer        readBuffer        = ByteBuffer.allocate(BUFFER_SIZE);
  
  private SelectionKey      writerReadyKey    = null;
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
      readerReadyKey = socketChannel.register(socketSelector, SelectionKey.OP_READ);
    }
    catch(Exception e)
    {
      System.out.println("Error creating readerReadyKey on incoming connection: " + e);
      disconnected = true;
      return false;
    }

    try
    {
      writerReadyKey = socketChannel.register(socketSelector, SelectionKey.OP_WRITE);
    }
    catch(Exception e)
    {
      System.out.println("Error creating writerReadyKey on incoming connection: " + e);
      disconnected = true;
      return false;
    }
  	
    return true;
  }
  
  public void LoopOnce()
  {
    if(Connect())
    {
      ReadOnce();
      WriteOnce();
      ProcessReadBuffer();
    }
  }
  
  public boolean Connect()
  {
  	
    if(connected)
    {
      return true;
    }

    if(disconnected)
    {
      return false;
    }
  	
    // we're not connected yet...
    if(socketChannel == null) // we need to make the outgoing connectiong
    {
      try
      {
        socketChannel = SocketChannel.open();
        socketChannel.configureBlocking(false);
      }
      catch(Exception e)
      {
        System.out.println("Error opening SocketChannel during connect: " + e);
        disconnected = true; // since this is a weird error
        return false;
      }

      try
      {
        socketSelector = Selector.open();
        socketConnectKey = socketChannel.register(socketSelector, SelectionKey.OP_CONNECT);
      }
      catch(Exception e)
      {
        System.out.println("Error creating socket selector: " + e);
        disconnected = true;
        return false;
      }
  			
      try
      {
        socketChannel.connect(socketAddress);
      }
      catch(Exception e)
      {
        System.out.println("Error connecting to " + hostname + ":" + port + ": " + e);
        disconnected = true;
        return false;
      }
    }
    else // we have a socketChannel, see if it's connected...
    {
      if(socketConnectKey.isConnectable())
      {
        if(socketChannel.isConnectionPending())
        {
          try
          {
            socketChannel.finishConnect();
            connected = true;
          }
          catch(Exception e)
          {
            System.out.println("Error finalizing outgoing connection: " + e);
            disconnected = true;
            return false;
          }
  				
          if(!InitializeBufferedIO()) // I don't know that i like how i'm initializing this stuff
          {
            disconnected = true;
            return false;
          }
        }
      }
    }
    return false; // even if we connect, we let ourselves fall through for one cycle...
  }

  private boolean ReadOnce()
  {
    if(readerReadyKey.isReadable())
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
    }
    return true;
  }
  	
  private boolean WriteOnce()
  {
    if(writerReadyKey.isWritable())
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
    }
    return true;
  }
 
  private boolean ProcessReadBuffer()
  {
    while(readBuffer.hasRemaining())
    {
      if(readBuffer.get() == '\n')
      {
        byte[] message = new byte[readBuffer.position()];
        readBuffer.get(message);
        readBuffer.compact(); // smush the buffer down...
        System.out.println("Got message: " + message.toString());
      }
    }
    return true;
  }
  
}
