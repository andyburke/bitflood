/*
 * Created on Nov 12, 2004
 *
 */

import java.util.*;

/**
 * @author burke
 *
 */
public class Peer 
{
  private PeerConnection[] pendingpeers = null;
  private Hashtable        floods       = null;
    
  public Peer() 
  {
    floods = new Hashtable();
  }
  
  public boolean JoinFlood(String floodFilename) 
  {
    Flood floodToJoin = new Flood(floodFilename);
    if(floodToJoin.Id() != null)
    {
      floods.put(floodToJoin.Id(), floodToJoin);
      return true;
    }
    return false;
  }
  
  public void LoopOnce()
  {
    Collection floodsToProcess = floods.values();
    Iterator floodIter = floodsToProcess.iterator();
    while(floodIter.hasNext())
    {
      Flood flood = (Flood)floodIter.next();
      flood.LoopOnce();
    }

    if( pendingpeers != null )
    {
      for(int peerIndex = 0; peerIndex < pendingpeers.length; peerIndex++)
      {
        if( pendingpeers[peerIndex] != null)
        {
          pendingpeers[peerIndex].LoopOnce();
        }
      }
    }
  }
}
