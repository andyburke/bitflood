package com.net.BitFlood.method;

/*
 * Created on Nov 12, 2004
 *
 */

import com.net.BitFlood.*;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.*;

/**
 * @author burke
 *  
 */
public class RequestChunkMethod implements MethodHandler
{
  final static public String methodName = "RequestChunk";

  public String getMethodName()
  {
    return methodName;
  }

  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    if ( receiver.flood == null )
    {
      throw new Exception( "flood specific method from an unregistered peer!" );
    }

    String fileName = (String) parameters.elementAt( 0 );
    Integer chunkIndex = (Integer) parameters.elementAt( 1 );
    if ( fileName == null )
    {
      throw new Exception( "No target filename specified in " + methodName + " from " + receiver );
    }
    if ( chunkIndex == null )
    {
      throw new Exception( "No chunk index specified in " + methodName + " from " + receiver );
    }

    // logging
    Logger.LogNormal( receiver + " is requesting chunk: " + fileName + "#" + chunkIndex );
    
    Flood.RuntimeTargetFile runtimeTargetFile = (Flood.RuntimeTargetFile) receiver.flood.runtimeTargetFiles.get( fileName );
    if ( runtimeTargetFile == null )
    {
      throw new Exception( "Unknown target file (" + fileName + ") specified in " + methodName + " from " + receiver );
    }
    
    if(chunkIndex.intValue() < 0 || chunkIndex.intValue() >= runtimeTargetFile.chunks.size())
    {
      throw new Exception( "Chunk index out of bounds in " + methodName + " from " + receiver );
    }
    
    if ( runtimeTargetFile.chunkMap[chunkIndex.intValue()] == '0' )
    {
      throw new Exception( "We don't have the chunk that was requested in " + methodName + " from " + receiver );
    }

    FloodFile.Chunk targetChunk = (FloodFile.Chunk) runtimeTargetFile.targetFile.chunks.get( chunkIndex.intValue() );
    long chunkOffset = runtimeTargetFile.chunkOffsets[chunkIndex.intValue()];

    // read in the appropriate chunk data and send it across the wire
    InputStream inputFileStream = null;
    try
    {
      inputFileStream = new FileInputStream( fileName );
    }
    catch ( Exception e )
    {
      throw new Exception( "error opening file: " + fileName + " : " + e );
    }

    byte[] chunkData = new byte[targetChunk.size];
    int bytesRead = 0;

    try
    {
      inputFileStream.skip( chunkOffset );
      bytesRead = inputFileStream.read( chunkData );
    }
    catch ( IOException e )
    {
      throw new Exception( "error reading from file: " + fileName + " : " + e );
    }

    if ( bytesRead != targetChunk.size )
    {
      throw new Exception( "error reading from file: " + fileName );
    }

    String testHash = Encoder.SHA1Base64Encode( chunkData, targetChunk.size );
    if ( testHash.compareTo( targetChunk.hash ) != 0 )
    {
      throw new Exception( "file data is not correct!" );
    }

    // respond with the appropriate method
    Vector scParams = new Vector( 3 );
    scParams.add( fileName );
    scParams.add( chunkIndex );
    scParams.add( chunkData );
    receiver.SendMethod( SendChunkMethod.methodName, scParams );
  }
}
