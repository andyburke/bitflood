/*
 * Created on Nov 12, 2004
 *
 */

import java.util.*;

/**
 * @author burke
 *  
 */
public class RequestChunkMapsMethodHandler implements MethodHandler
{
  final static String methodName = "RequestChunkMaps";

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
    
    // build params and call SendChunkMap
    Vector scmParams = new Vector( receiver.flood.floodFile.targetFiles.size() * 2 );
    Iterator fileiter = Arrays.asList(receiver.flood.floodFile.targetFiles.values().toArray()).iterator();
    for ( int fileIndex = 0; fileiter.hasNext(); ++fileIndex )
    {
      FloodFile.TargetFile targetFile = (FloodFile.TargetFile) fileiter.next();
      Flood.RuntimeTargetFile runtimeTargetFile = (Flood.RuntimeTargetFile) receiver.flood.runtimeTargetFiles.get(targetFile.name);
      
      scmParams.add( targetFile.name );
      scmParams.add( new String( runtimeTargetFile.chunkMap ) );
    }
    
    receiver.SendMethod( SendChunkMapsMethodHandler.methodName, scmParams );
  }
}