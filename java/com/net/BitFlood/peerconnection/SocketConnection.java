/*
 * Created on Dec 6, 2004
 */
package com.net.BitFlood.peerconnection;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.util.Iterator;
import java.util.Vector;

import com.net.BitFlood.*;
import com.net.BitFlood.method.*;

/**
 * @author jmuhlich
 */
public class SocketConnection extends PeerConnection
{

  public SocketChannel socketChannel = null;
  private Selector socketSelector = null;
  public String hostname = "";
  public int port = 0;
  public int listenPort = 0;

  public SocketConnection(Flood theFlood, String remoteHost, int remotePort, String remoteId)
  {
    flood = theFlood;
    localPeer = flood.localPeer;
    hostname = remoteHost;
    port = remotePort;
    id = remoteId;

    try
    {
      socketSelector = Selector.open();
      socketChannel = SocketChannel.open();
      socketChannel.configureBlocking( false );
      socketChannel.register( socketSelector, socketChannel.validOps() );
      socketChannel.connect( new InetSocketAddress( InetAddress.getByName( hostname ), port ) );
      Logger.LogNormal( "Connecting to: " + this );
    }
    catch ( Exception e )
    {
      Logger.LogError( "Error connecting to (" + this + "): " + e );
      disconnected = true;
      return;
    }

    // register ourselves w/ the peer
    {
      Vector args = new Vector();
      args.add( flood.Id() );
      args.add( flood.localPeer.id );
      args.add( new Integer( flood.localPeer.port ) );

      SendMethod( RegisterMethod.methodName, args );
    }

    // request some chunks
    SendMethod( RequestChunkMapsMethod.methodName, new Vector() );
  }

  public SocketConnection(Peer peer, SocketChannel incomingSocket)
  {
    localPeer = peer;

    if ( incomingSocket == null )
    {
      Logger.LogError( "Tried to create a PeerConnection with a null socket?" );
      disconnected = true;
      return;
    }

    socketChannel = incomingSocket;

    try
    {
      hostname = socketChannel.socket().getInetAddress().getHostAddress();
      port = socketChannel.socket().getPort();
      
      socketSelector = Selector.open();
      socketChannel.configureBlocking( false );
      socketChannel.register( socketSelector, socketChannel.validOps() );
      connected = true;
      Logger.LogNormal( "Connection from: " + this );
    }
    catch ( Exception e )
    {
      disconnected = true;
      Logger.LogError( "Error setting up incoming connection: " + e );
    }
  }

  public String toString()
  {
    return new String( id + ":" + hostname + ":" + port );
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
        Logger.LogError( "Error selecing from socketSelector: " + e );
        disconnected = true;
      }
  
      // Get list of selection keys with pending events
      Iterator it = socketSelector.selectedKeys().iterator();
      while ( it.hasNext() )
      {
        // Get the selection key
        SelectionKey selKey = (SelectionKey) it.next();
  
        // Remove it from the list to indicate that it is being processed
        it.remove();
  
        // 
        if ( selKey.isValid() && selKey.isConnectable() )
        {
          SocketChannel sChannel = (SocketChannel) selKey.channel();
  
          try
          {
            sChannel.finishConnect();
            connected = true;
          }
          catch ( Exception e )
          {
            Logger.LogError( "Error connecting: " + e );
            disconnected = true;
          }
        }
  
        if ( selKey.isValid() && selKey.isReadable() )
        {
          ReadOnce();
        }
  
        if ( selKey.isValid() && selKey.isWritable() )
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
      int bytesRead = socketChannel.read( readBuffer );
    }
    catch ( Exception e )
    {
      Logger.LogError( "Error reading from socketChannel: " + e );
      disconnected = true;
      return false;
    }
  
    return true;
  }
  
  private boolean WriteOnce()
  {
    try
    {
      // flip and write the buffer
      writeBuffer.flip();
      int bytesWritten = socketChannel.write( writeBuffer );
  
      // compact the buffer
      writeBuffer.compact();
    }
    catch ( Exception e )
    {
      Logger.LogError( "Error writing to socketChannel: " + e );
      disconnected = true;
      return false;
    }
  
    return true;
  }

}
