package com.net.BitFlood.method;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.*;
import java.util.Vector;

/**
 * @author jmuhlich
 *  
 */
public class SendChunkInfoMethod implements MethodHandler
{
  final static public String methodName = "SendChunkInfo";
  public String getMethodName()
  {
    return methodName;
  }
  
  // TODO - test
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    if ( receiver.flood == null )
    {
      throw new Exception( "flood specific method from an unregistered peer!" );
    }
    
    String targetFilename = (String) parameters.elementAt(0);
    Integer chunkIndex    = (Integer) parameters.elementAt(1);
    Integer chunkOffset   = (Integer) parameters.elementAt(2);
    Integer chunkSize     = (Integer) parameters.elementAt(3);
    String chunkHash      = (String) parameters.elementAt(4);
    
    // TODO: refactor this param checking
    if( targetFilename == null )
    {
      throw new Exception( "No target filename specified in " + methodName + " from " + receiver );
    }
    if( chunkIndex == null )
    {
      throw new Exception( "No chunk index specified in " + methodName + " from " + receiver );
    }
    if( chunkOffset == null )
    {
      throw new Exception( "No chunk offset specified in " + methodName + " from " + receiver );
    }
    if( chunkSize == null )
    {
      throw new Exception( "No chunk size specified in " + methodName + " from " + receiver );
    }
    if( chunkHash == null )
    {
      throw new Exception( "No chunk hash specified in " + methodName + " from " + receiver );
    }

    // TODO: implement
    
  }
}
