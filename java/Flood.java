/*
 * Created on Nov 12, 2004
 *
 */

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.*;
import java.io.BufferedInputStream;

/**
 * @author burke
 *  
 */
public class Flood
{
  public Peer      localPeer         = null;
  public Vector    peerConnections   = new Vector();
  public FloodFile floodFile         = null;
  private Date     lastTrackerUpdate = null;

  // runtime flood data
  public class RuntimeTargetFile
  {
    String           nameOnDisk    = null;
    int[]            chunkOffsets  = null;
    char[]           chunkMap      = null;
  }

  public class ChunkKey
  {
    int fileIndex  = -1;
    int chunkIndex = -1;
  }

  public int                 totalBytes        = 0;
  public int                 bytesAtStartup    = 0;
  public int                 bytesMissing      = 0;
  public int                 bytesDownloading  = 0;

  public Hashtable           runtimeTargetFiles = null;
  public Vector              chunksToDownload  = new Vector();
  public HashSet             chunksDownloading = new HashSet();

  public Flood()
  {
  }

  public Flood(Peer peer, String floodFilename)
  {
    localPeer = peer;
    floodFile = new FloodFile( floodFilename );
    floodFile.Read();
    SetupFilesAndChunks();
  }

  public String Id()
  {
    return floodFile.contentHash;
  }

  public void LoopOnce()
  {
    // clone the peers Vector and then process them
    {
      Vector peerConnectionsClone = (Vector) peerConnections.clone();
      Iterator peeriter = peerConnectionsClone.iterator();
      for ( ; peeriter.hasNext(); )
      {
        PeerConnection peer = (PeerConnection) peeriter.next();
        peer.LoopOnce();
      }
    }

    // reap disconnected peers
    Iterator peeriter = peerConnections.iterator();
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

        if ( tracker.id.compareTo( localPeer.id ) != 0 )
        {
          PeerConnection peer = FindPeer( tracker.id );
          if ( peer == null )
          {
            peer = new PeerConnection( this, tracker.host, tracker.port, tracker.id );
            peerConnections.add( peer );
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
    Iterator peeriter = peerConnections.iterator();
    for ( ; peeriter.hasNext(); )
    {
      PeerConnection peer = (PeerConnection) peeriter.next();
      if ( peer.id.compareTo( peerId ) == 0 )
      {
        retVal = peer;
        break;
      }
    }
    return retVal;
  }

  protected void SetupFilesAndChunks()
  {
    if ( floodFile.targetFiles.size() > 0 )
    {

      Iterator targetFileIter = Arrays.asList(floodFile.targetFiles.values().toArray()).iterator();
      for ( int fileIndex = 0; targetFileIter.hasNext(); ++fileIndex )
      {
        FloodFile.TargetFile targetFile = (FloodFile.TargetFile) targetFileIter.next();
        RuntimeTargetFile runtimeTargetFile = new RuntimeTargetFile();
        runtimeTargetFile.nameOnDisk = targetFile.name;  // having the rtf have a name lets us rename the target 
        runtimeTargetFiles.put( targetFile.name, runtimeTargetFile );

        // track some stats
        totalBytes += targetFile.size;

        int numChunks = targetFile.chunks.size();
        runtimeTargetFile.chunkOffsets = new int[numChunks];
        runtimeTargetFile.chunkMap = new char[numChunks];

        InputStream inputFileStream = null;
        try
        {
          inputFileStream = new BufferedInputStream( new FileInputStream( runtimeTargetFile.nameOnDisk ) );
          inputFileStream.mark( inputFileStream.available() );
        }
        catch ( Exception e )
        {
        }

        Iterator chunkiter = targetFile.chunks.iterator();
        for ( int nextOffset = 0; chunkiter.hasNext(); )
        {
          FloodFile.Chunk chunk = (FloodFile.Chunk) chunkiter.next();

          // track the offsets
          runtimeTargetFile.chunkOffsets[chunk.index] = nextOffset;

          // mark it as invalid
          runtimeTargetFile.chunkMap[chunk.index] = '0';

          // see if the existing chunk is valid
          if ( inputFileStream != null )
          {
            byte[] chunkData = new byte[chunk.size];

            try
            {
              inputFileStream.skip( nextOffset );
              if ( inputFileStream.read( chunkData ) == chunk.size )
              {
                String existingHash = Encoder.SHA1Base64Encode( chunkData, chunk.size );
                if ( existingHash.compareTo( chunk.hash ) == 0 )
                {
                  runtimeTargetFile.chunkMap[chunk.index] = '1';

                  // how many bytes did we start with
                  bytesAtStartup += chunk.size;
                }
              }
              // FIXME throw some error
              else
              {
              }

              inputFileStream.reset();
            }
            catch ( IOException e )
            {
            }
          }

          nextOffset += chunk.size;
        }

        if ( inputFileStream != null )
        {
          try
          {
            inputFileStream.close();
          }
          catch ( IOException e )
          {
          }
        }
      }
    }

    // how many bytes are we currently missing
    bytesMissing = totalBytes - bytesAtStartup;
  }
}