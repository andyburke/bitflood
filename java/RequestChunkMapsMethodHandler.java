/*
 * Created on Nov 12, 2004
 *
 */

import java.util.Iterator;
import java.util.Vector;

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
    Vector scmParams = new Vector( receiver.flood.floodFile.files.size() * 2 );
    Iterator fileiter = receiver.flood.floodFile.files.iterator();
    for ( int fileIndex = 0; fileiter.hasNext(); ++fileIndex )
    {
      FloodFile.TargetFile file = (FloodFile.TargetFile) fileiter.next();
      Flood.RuntimeTargetFile rtf = receiver.flood.runtimeFileData[ fileIndex ];
      
      scmParams.add( file.name );
      scmParams.add( new String( rtf.chunkMap ) );
    }
    
    receiver.SendMethod( SendChunkMapsMethodHandler.methodName, scmParams );
  }
}