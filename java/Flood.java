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
  public  Peer      localPeer         = null;
  private Vector    peers             = new Vector();
  private FloodFile floodFile         = null;
  private Date      lastTrackerUpdate = null;

  public Flood()
  {
  }

  public Flood(Peer peer, String floodFilename)
  {
    localPeer = peer;
    floodFile = new FloodFile( floodFilename );
    floodFile.Read();
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
        
        if ( !tracker.id.contentEquals( localPeer.id ) )
        {
          PeerConnection peer = InqPeer( tracker.id );
          if ( peer == null )
          {
            peer = new PeerConnection( tracker.host, tracker.port, tracker.id );
            peers.add( peer );
          }
          
          // send a "requestpeerlist" method
        }
      }
    }
  }
  
  public PeerConnection InqPeer( final String peerId )
  {
    PeerConnection retVal = null;
    Enumeration peeriter = peers.elements();
    for ( ; peeriter.hasMoreElements(); )
    {
      PeerConnection peer = (PeerConnection) peeriter.nextElement();
      if ( peer.id.contentEquals( peerId ) )
      {
        retVal = peer;
        break;
      }
    }
    return retVal;
  }
}