/*
 * Created on Dec 6, 2004
 */
package com.net.BitFlood.peerconnection;

import java.io.RandomAccessFile;
import java.util.Vector;
import com.net.BitFlood.*;

/**
 * @author jmuhlich
 */
public class FileConnection extends PeerConnection
{

  private String           filename;
  private RandomAccessFile file;
  
  public FileConnection (String filename) {
    this.filename = filename;
    Open();
    trusted = true;
  }
  
  public String toString()
  {
    return new String( filename );
  }
  
  public void LoopOnce()
  {
    ReadOnce();
    ProcessReadBuffer();
  }

  public void Open()
  {
    try {
      file = new RandomAccessFile(filename, "r");
    }
    catch (Exception e) {
      e.printStackTrace();
      disconnected = true;
      return;
    }
    connected = true;
  }

  private boolean ReadOnce()
  {
    // TODO: test
    try {
      file.readFully(readBuffer.array());
    }
    catch (Exception e) {
      disconnected = true;
      return false;
    }
    return true;
  }

  public void SendMethod( final String methodName, final Vector parameters )
  {
    // make this a no-op (files are read-only)
  }

}
