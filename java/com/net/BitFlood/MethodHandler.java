package com.net.BitFlood;

/*
 * Created on Nov 12, 2004
 *
 */

import java.util.Vector;

/**
 * @author burke
 *  
 */
public interface MethodHandler
{
  String getMethodName();
  void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception;
}
