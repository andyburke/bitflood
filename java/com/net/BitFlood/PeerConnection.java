package com.net.BitFlood;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.method.*;
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
  private Hashtable     messageHandlers   = null;
  public  SocketChannel socketChannel     = null;
  private Selector      socketSelector    = null;
  private int           BUFFER_SIZE       = 256 * 1024 * 2;                    // 2 times thenormal chunk size
  private ByteBuffer    readBuffer        = ByteBuffer.allocate( BUFFER_SIZE );
  private int           readIndex         = 0;
  private ByteBuffer    writeBuffer       = ByteBuffer.allocate( BUFFER_SIZE );
  public Peer           localPeer         = null;
  public boolean        connected         = false;
  public boolean        disconnected      = false;
  public String         hostname          = "";
  public int            port              = 0;
  public int            listenPort        = 0;
  public String         id                = "";
  public Flood          flood             = null;                              
  public Hashtable      chunkMaps         = new Hashtable();
  public int            chunksDownloading = 0;

  public String toString()
  {
    return new String( id + ":" + hostname + ":" + port );
  }
  
  public PeerConnection()
  {
  }

  public PeerConnection(Flood theFlood, String remoteHost, int remotePort, String remoteId)
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

  public PeerConnection(Peer peer, SocketChannel incomingSocketConnection)
  {
    localPeer = peer;

    if ( incomingSocketConnection == null )
    {
      Logger.LogError( "Tried to create a PeerConnection with a null socket?" );
      disconnected = true;
      return;
    }

    socketChannel = incomingSocketConnection;

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
  
  public void finalize() {
    localPeer = null;
    flood = null;
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

  private boolean ProcessReadBuffer()
  {
    while ( readBuffer.position() > readIndex )
    {
      byte b = readBuffer.get( readIndex++ );
      if ( b == '\n' )
      {
        int diff = readBuffer.position() - readIndex;
        byte[] message = new byte[readIndex];
        readBuffer.position( 0 );
        readBuffer.get( message, 0, readIndex );
        readBuffer.compact();
        readBuffer.position( diff );
        readIndex = 0;

        Vector args = new Vector();
        StringBuffer methodName = new StringBuffer();
        XMLEnvelopeProcessor.decode( new String( message ), methodName, args );
        Logger.LogNormal( this + " sent (" + methodName + ")" );
        localPeer.HandleMethod( this, methodName.toString(), args );
      }
    }

    return true;
  }

  public void SendMethod( final String methodName, final Vector parameters )
  {
    String out = XMLEnvelopeProcessor.encode( methodName, parameters );
    Logger.LogNormal( this + " sending (" + methodName + ")" );

    // fix up the string, remove newlines and then add one at the end to signify the end of a method call
    writeBuffer.put( ( out.replace( '\n', ' ' ) + '\n' ).getBytes() );
  }
}
