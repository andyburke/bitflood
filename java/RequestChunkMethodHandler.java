/*
 * Created on Nov 12, 2004
 *
 */

import java.util.Vector;

/**
 * @author burke
 *  
 */
public class RequestChunkMethodHandler implements MethodHandler
{
  final static String methodName = "RequestChunk";
  public String getMethodName()
  {
    return methodName;
  }
  
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
  }
}