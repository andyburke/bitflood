/*
 * Created on Nov 12, 2004
 *
 */

import java.util.*;

/**
 * @author burke
 *  
 */
public class Flood
{
  private Vector    peers             = new Vector();
  private FloodFile floodFile         = null;
  private Date      lastTrackerUpdate = null;

  public Flood()
  {
  }

  public Flood(String floodFilename)
  {
    floodFile = new FloodFile( floodFilename );
  }

  public String Id()
  {
    return floodFile.contentHash;
  }

  public void LoopOnce()
  {
    if ( peers != null )
    {
      Enumeration peeriter = peers.elements();
      for ( ; peeriter.hasMoreElements(); )
      {
        PeerConnection peer = (PeerConnection) peeriter.nextElement();
        peer.LoopOnce();
      }
    }

    Date now = new Date();
    if ( lastTrackerUpdate == null || ( now.getTime() - lastTrackerUpdate.getTime() >= 20 ) )
    {
      UpdateTrackers();
      lastTrackerUpdate = new Date();
    }
  }

  protected void UpdateTrackers()
  {
    if ( floodFile != null && floodFile.trackers != null )
    {
      Enumeration trackeriter = floodFile.trackers.elements();
      for ( ; trackeriter.hasMoreElements(); )
      {
        FloodFile.TrackerInfo tracker = (FloodFile.TrackerInfo) trackeriter.nextElement();
      }
    }
  }
}