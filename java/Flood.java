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
  public Peer       localPeer         = null;
  public Vector     peers             = new Vector();
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
    // clone the peers Vector and then process them
    {
      Vector peersClone = (Vector)peers.clone();
      Iterator peeriter = peersClone.iterator();
      for ( ; peeriter.hasNext(); )
      {
        PeerConnection peer = (PeerConnection) peeriter.next();
        peer.LoopOnce();
      }
    }

    // reap disconnected peers
    Iterator peeriter = peers.iterator();
    for ( ; peeriter.hasNext(); )
    {
      PeerConnection peer = (PeerConnection) peeriter.next();
      if ( peer.disconnected )
      {
        System.out.println( "Reaping " + peer.id );
        peeriter.remove();
      }
    }
    Date now = new Date();
    if ( lastTrackerUpdate == null || ( now.getTime() - lastTrackerUpdate.getTime() >= 20000 ) )
    {
      UpdateTrackers();
      lastTrackerUpdate = new Date();
    }
  }

  protected void UpdateTrackers()
  {
    if ( floodFile != null && floodFile.trackers != null )
    {
      Iterator trackeriter = floodFile.trackers.iterator();
      for ( ; trackeriter.hasNext(); )
      {
        FloodFile.TrackerInfo tracker = (FloodFile.TrackerInfo) trackeriter.next();

        if ( !tracker.id.contentEquals( localPeer.id ) )
        {
          PeerConnection peer = FindPeer( tracker.id );
          if ( peer == null )
          {
            peer = new PeerConnection( this, tracker.host, tracker.port, tracker.id );
            peers.add( peer );
          }

          // Request the tracker's peer list
          peer.SendMethod( RequestPeerListMethodHandler.methodName, new Vector() );
        }
      }
    }
  }

  public PeerConnection FindPeer( final String peerId )
  {
    PeerConnection retVal = null;
    Iterator peeriter = peers.iterator();
    for ( ; peeriter.hasNext(); )
    {
      PeerConnection peer = (PeerConnection) peeriter.next();
      if ( peer.id.contentEquals( peerId ) )
      {
        retVal = peer;
        break;
      }
    }
    return retVal;
  }  
}