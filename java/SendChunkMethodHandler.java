/*
 * Created on Nov 12, 2004
 *
 */

import java.nio.channels.FileLock;
import java.util.Vector;
import java.util.Iterator;

/**
 * @author burke
 *  
 */
public class SendChunkMethodHandler implements MethodHandler
{
  final static String methodName = "SendChunk";
  public String getMethodName()
  {
    return methodName;
  }
  
  // TODO - test
  public void HandleMethod( PeerConnection receiver, final Vector parameters ) throws Exception
  {
    if ( receiver.flood == null )
    {
      throw new Exception( "flood specific method from an unregistered peer!" );
    }
    
    String targetFilename = (String) parameters.elementAt(0);
    Integer chunkIndex    = (Integer) parameters.elementAt(1);
    byte[] chunkData      = (byte[]) parameters.elementAt(2);
    
    if( targetFilename == null )
    {
      throw new Exception( "No target filename specified in SendChunk");
    }
    if( chunkIndex == null )
    {
      throw new Exception( "No chunk index specified in SendChunk");
    }
    if( chunkData == null )
    {
      throw new Exception( "No chunk data specified in SendChunk");
    }
    
    Flood.RuntimeTargetFile runtimeTargetFile = (Flood.RuntimeTargetFile) receiver.flood.runtimeTargetFiles.get(targetFilename);
    if(runtimeTargetFile == null)
    {
      throw new Exception( "Unknown target file specified in SendChunk");
    }
    
    FloodFile.Chunk chunkInfo = (FloodFile.Chunk) runtimeTargetFile.targetFile.chunks.elementAt(chunkIndex.intValue());
    if( chunkInfo == null )
    {
      throw new Exception( "Error retrieving chunk info in SendChunk");
    }
    
    // if the incoming data hashes right
    if(Encoder.SHA1Base64Encode(chunkData, chunkData.length).compareTo(chunkInfo.hash) == 0 )
    {
      FileLock writeLock = null;
      try
      {
        writeLock = runtimeTargetFile.fileChannel.tryLock(runtimeTargetFile.chunkOffsets[chunkIndex.intValue()],
            				                                      chunkInfo.size,
            				                                      false); // lock this region, non-shared
      }
      catch(Exception e)
      {
        e.printStackTrace();
      }
      
      runtimeTargetFile.fileHandle.seek(runtimeTargetFile.chunkOffsets[chunkIndex.intValue()]);
      runtimeTargetFile.fileHandle.write(chunkData);
      writeLock.release();
      
      Flood.ChunkKey chunkKey = receiver.flood.MakeChunkKey( runtimeTargetFile, chunkIndex.intValue() );
      receiver.flood.chunksDownloading.remove( chunkKey );
      receiver.flood.chunksToDownload.remove( chunkKey );
      receiver.chunksDownloading--;
      
      Vector nhcParams = new Vector( 2 );
      nhcParams.add( targetFilename );
      nhcParams.add( chunkIndex );
      
      Iterator peerIter = receiver.flood.peerConnections.iterator();
      while( peerIter.hasNext() )
      {
        PeerConnection peerConnection = (PeerConnection)peerIter.next();
        peerConnection.SendMethod( NotifyHaveChunkMethodHandler.methodName, nhcParams );
      }
    }
    else
    {
      throw new Exception( "Bad chunk data received in SendChunk");
    }
  }
}