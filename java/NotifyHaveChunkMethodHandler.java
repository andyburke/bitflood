/*
 * Created on Nov 12, 2004
 *
 */

import java.util.Vector;

/**
 * @author burke
 *  
 */
public class NotifyHaveChunkMethodHandler implements MethodHandler
{
  final static String methodName = "NotifyHaveChunk";
  public String getMethodName()
  {
    return methodName;
  }
  
  // TODO - implement
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    if ( receiver.flood == null )
    {
      throw new Exception( "flood specific method from an unregistered peer!" );
    }
    
    final String targetFilename = (String) parameters.get(0);
    final int    chunkIndex     = ((Integer) parameters.get(1)).intValue();
    
    if(targetFilename == null)
    {
      // FIXME throw an exception?  this isn't fatal, but... ?
      return;
    }
    
    
  }
}