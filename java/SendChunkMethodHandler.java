/*
 * Created on Nov 12, 2004
 *
 */

import java.nio.channels.FileLock;
import java.util.Vector;
import java.util.Iterator;
import java.util.Date;

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
      throw new Exception( "No target filename specified in " + methodName + " from " + receiver );
    }
    if( chunkIndex == null )
    {
      throw new Exception( "No chunk index specified in " + methodName + " from " + receiver );
    }
    if( chunkData == null )
    {
      throw new Exception( "No chunk data specified in " + methodName + " from " + receiver );
    }

    Flood.RuntimeTargetFile runtimeTargetFile = (Flood.RuntimeTargetFile) receiver.flood.runtimeTargetFiles.get(targetFilename);
    if(runtimeTargetFile == null)
    {
      throw new Exception( "Unknown target file specified in " + methodName + " from " + receiver );
    }

    if(chunkIndex.intValue() < 0 || chunkIndex.intValue() >= runtimeTargetFile.chunks.size())
    {
      throw new Exception( "Chunk index out of bounds in " + methodName + " from " + receiver );
    }
    
    FloodFile.Chunk chunkInfo = (FloodFile.Chunk) runtimeTargetFile.targetFile.chunks.elementAt(chunkIndex.intValue());
    if( chunkInfo == null )
    {
      throw new Exception( "Error retrieving chunk info in " + methodName + " from " + receiver );
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
      
      Flood.RuntimeChunk downloadedChunk = (Flood.RuntimeChunk) runtimeTargetFile.chunks.elementAt(chunkIndex.intValue());
      receiver.flood.chunksDownloading.remove( downloadedChunk );
      receiver.flood.chunksToDownload.remove( downloadedChunk );
      receiver.chunksDownloading--;
      receiver.flood.bytesDownloading -= downloadedChunk.size;
      receiver.flood.bytesMissing -= downloadedChunk.size;
      
      Date now = new Date();
      Logger.LogNormal( receiver + " sent chunk: " + targetFilename + "#" + chunkIndex + " " + chunkInfo.size / (now.getTime() - downloadedChunk.downloadStartDate.getTime()) + "K/s" );
      
      
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
      throw new Exception( "Bad chunk data received in " + methodName + " from " + receiver );
    }
  }
}