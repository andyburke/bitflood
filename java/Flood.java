/*
 * Created on Nov 12, 2004
 *
 */

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Vector;
import java.util.Hashtable;
import java.util.HashSet;
import java.util.Date;
import java.util.Iterator;
import java.io.BufferedInputStream;

/**
 * @author burke
 *  
 */
public class Flood
{
  public Peer       localPeer         = null;
  public Vector     peers             = new Vector();
  public FloodFile floodFile         = null;
  private Date      lastTrackerUpdate = null;

  // runtime flood data
  public class RuntimeTargetFile
  {
    public Hashtable chunkIndicies = null;
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

  public RuntimeTargetFile[] runtimeFileData   = null;
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
      Vector peersClone = (Vector) peers.clone();
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

  protected void SetupFilesAndChunks()
  {
    if ( floodFile.files.size() > 0 )
    {
      runtimeFileData = new RuntimeTargetFile[floodFile.files.size()];

      Iterator fileiter = floodFile.files.iterator();
      for ( int fileIndex = 0; fileiter.hasNext(); ++fileIndex )
      {
        FloodFile.TargetFile file = (FloodFile.TargetFile) fileiter.next();
        RuntimeTargetFile rtf = new RuntimeTargetFile();
        runtimeFileData[fileIndex] = rtf;

        // track some stats
        totalBytes += file.size; 
        
        int numChunks = file.chunks.size();
        rtf.chunkIndicies = new Hashtable( numChunks );
        rtf.chunkOffsets = new int[numChunks];
        rtf.chunkMap = new char[numChunks];

        InputStream inputFileStream = null;
        try
        {
          inputFileStream = new BufferedInputStream( new FileInputStream( file.name ) );
          inputFileStream.mark( inputFileStream.available() );
        }
        catch ( Exception e )
        {
        }

        Iterator chunkiter = file.chunks.iterator();
        for ( int nextOffset = 0; chunkiter.hasNext(); )
        {
          FloodFile.Chunk chunk = (FloodFile.Chunk) chunkiter.next();

          // FIXME throw some error
          // map the hash to an index for fast lookups
          if ( rtf.chunkIndicies.put( chunk.hash, new Integer( chunk.index ) ) != null )
          {
          }

          // track the offsets
          rtf.chunkOffsets[chunk.index] = nextOffset;

          // mark it as invalid
          rtf.chunkMap[chunk.index] = '0';

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
                if ( existingHash.contentEquals( chunk.hash ) )
                {
                  rtf.chunkMap[chunk.index] = '1';
                  
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