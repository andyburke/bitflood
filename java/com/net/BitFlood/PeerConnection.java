package com.net.BitFlood;

/*
 * Created on Nov 12, 2004
 *
 */

import java.nio.ByteBuffer;
import java.util.Hashtable;
import java.util.Vector;

/**
 * @author burke
 *  
 */
public abstract class PeerConnection
{
  private Hashtable     messageHandlers   = null;
  private int           BUFFER_SIZE       = 256 * 1024 * 2;
  public ByteBuffer     readBuffer        = ByteBuffer.allocate( BUFFER_SIZE );
  public int            readIndex         = 0;
  public ByteBuffer     writeBuffer       = ByteBuffer.allocate( BUFFER_SIZE );
  public Peer           localPeer         = null;
  public boolean        connected         = false;
  public boolean        disconnected      = false;
  public boolean		trusted           = false;
  public String         id                = "";
  public Flood          flood             = null;
  public Hashtable      chunkMaps         = new Hashtable();
  public int            chunksDownloading = 0;

  public PeerConnection()
  {
  }

  public void finalize() {
    localPeer = null;
    flood = null;
  }
  
  public abstract void LoopOnce();
  
  public void SendMethod( final String methodName, final Vector parameters )
  {
    String out = XMLEnvelopeProcessor.encode( methodName, parameters );
    Logger.LogNormal( this + " sending (" + methodName + ")" );
  
    // fix up the string, remove newlines and then add one at the end to signify the end of a method call
    writeBuffer.put( ( out.replace( '\n', ' ' ) + '\n' ).getBytes() );
  }

  protected boolean ProcessReadBuffer()
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
}
