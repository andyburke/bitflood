/*
 * Created on Nov 12, 2004
 *
 */

import java.io.*;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.util.*;

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
    String               nameOnDisk    = null;
    File                 fileObject    = null;
    RandomAccessFile     fileHandle    = null;
    FileChannel          fileChannel   = null;
    FloodFile.TargetFile targetFile    = null;
    long[]               chunkOffsets  = null;
    char[]               chunkMap      = null;
  }

  public class ChunkKey
  {
    public RuntimeTargetFile runtimeTargetFile = null;
    public int chunkIndex = 0;
    
    public ChunkKey( RuntimeTargetFile file, int index )
    {
      runtimeTargetFile = file;
      chunkIndex = index;
    }
  }
  public ChunkKey MakeChunkKey( RuntimeTargetFile file, int index )
  {
    return new ChunkKey( file, index );
  }
  
  public int                 totalBytes        = 0;
  public int                 bytesAtStartup    = 0;
  public int                 bytesMissing      = 0;
  public int                 bytesDownloading  = 0;

  public Hashtable           runtimeTargetFiles = new Hashtable();
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
    // get chunks
    GetChunk();
    
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
      runtimeTargetFiles = new Hashtable( floodFile.targetFiles.size() );
      Iterator targetFileIter = Arrays.asList(floodFile.targetFiles.values().toArray()).iterator();
      for ( int fileIndex = 0; targetFileIter.hasNext(); ++fileIndex )
      {
        FloodFile.TargetFile targetFile = (FloodFile.TargetFile) targetFileIter.next();

        RuntimeTargetFile runtimeTargetFile = new RuntimeTargetFile();
        runtimeTargetFile.nameOnDisk = targetFile.name;  // having the rtf have a name lets us rename the target 
        runtimeTargetFile.targetFile = targetFile;
        try
        {
          // Get a file handle and channel for the file on disk
          runtimeTargetFile.fileObject = new File(runtimeTargetFile.nameOnDisk);
          runtimeTargetFile.fileHandle = new RandomAccessFile(runtimeTargetFile.fileObject, "rw");
          runtimeTargetFile.fileChannel = runtimeTargetFile.fileHandle.getChannel();
        }
        catch (Exception e)
        {
          e.printStackTrace();
        }

        runtimeTargetFiles.put( targetFile.name, runtimeTargetFile );

        // track some stats
        totalBytes += targetFile.size;

        int numChunks = targetFile.chunks.size();
        runtimeTargetFile.chunkOffsets = new long[numChunks];
        runtimeTargetFile.chunkMap = new char[numChunks];

        Iterator chunkiter = targetFile.chunks.iterator();
        for ( long nextOffset = 0; chunkiter.hasNext(); )
        {
          FloodFile.Chunk chunk = (FloodFile.Chunk) chunkiter.next();

          // track the offsets
          runtimeTargetFile.chunkOffsets[chunk.index] = nextOffset;

          // mark it as invalid
          runtimeTargetFile.chunkMap[chunk.index] = '0';

          // see if the existing chunk is valid
          byte[] chunkData = new byte[chunk.size];

          try
          {
            FileLock readLock = null;
            try
            {
              readLock = runtimeTargetFile.fileChannel.tryLock();
            }
            catch( Exception e)
            {
              e.printStackTrace();
            }
            
            runtimeTargetFile.fileHandle.seek( nextOffset );
            if ( runtimeTargetFile.fileHandle.read(chunkData) == chunk.size )
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
            readLock.release();
          }
          catch ( IOException e )
          {
            e.printStackTrace();
          }

          if ( runtimeTargetFile.chunkMap[chunk.index] == '0' )
          {
            chunksToDownload.add( new ChunkKey( runtimeTargetFile, chunk.index ) );
          }
          
          nextOffset += chunk.size;
        }
      }
    }

    // how many bytes are we currently missing
    bytesMissing = totalBytes - bytesAtStartup;
  }

  protected void GetChunk(  )
  {
    boolean foundchunk = false;
    PeerConnection todownload_from = null;
    ChunkKey todownload_key = null;

    // figure out what chunk to get from which peer and ask for it
    Iterator chunkiter = chunksToDownload.iterator();
    while ( chunkiter.hasNext() && !foundchunk )
    {
      ChunkKey chunkKey = (ChunkKey)chunkiter.next();
      
      if ( !chunksDownloading.contains( chunkKey ) )
      {
        RuntimeTargetFile runtimeTargetFile = chunkKey.runtimeTargetFile;
        Iterator peeriter = peerConnections.iterator();
        while( peeriter.hasNext() && !foundchunk )
        {
          PeerConnection peerConnection = (PeerConnection)peeriter.next();
          if ( peerConnection.chunksDownloading < 1 )
          {
            char[] peerChunkMap = (char[])peerConnection.chunkMaps.get( runtimeTargetFile.targetFile.name );
            if ( peerChunkMap != null )
            {
              if ( peerChunkMap[ chunkKey.chunkIndex ] == '1' )
              {
                foundchunk = true;
                todownload_from  = peerConnection;
                todownload_key   = chunkKey;
              }
            }
          }
        }
      }
    }

    if ( foundchunk )
    {
      Vector parameters = new Vector( 2 );
      parameters.add( todownload_key.runtimeTargetFile.targetFile.name );
      parameters.add( new Integer( todownload_key.chunkIndex ) );
      
      todownload_from.chunksDownloading++;
      todownload_from.SendMethod( RequestChunkMethodHandler.methodName, parameters );

      chunksDownloading.add( todownload_key );
    }
  }


}