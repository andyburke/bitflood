import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.util.*;
import java.io.*;

import com.net.BitFlood.*;
import com.sun.corba.se.internal.ior.WireObjectKeyTemplate;

/**
 * Created on Dec 6, 2004
 *
 */



public class HTTPConnection // FIXME should this be in its own file?
{
  private ThrumHTTPProxy parent = null;
  
  private SocketChannel socketChannel = null;
  private Selector socketSelector = null;
  private ByteArrayOutputStream byteArrayStream = null;
  private ChunkedOutputStream chunkedStream = null;

  private int           BUFFER_SIZE       = 1024 * 1024; // 1MB
  private ByteBuffer    readBuffer        = ByteBuffer.allocate( 2048 ); // 2kb should be fine for URLS
  private int           readIndex         = 0;
  private ByteBuffer    writeBuffer       = ByteBuffer.allocate( BUFFER_SIZE ); // 1MB for writing
  private int           writeIndex        = 0;
  private int           bytesWritten      = 0;
  
  private static final byte[] crlf = { 13, 10 };
  
  public boolean disconnected = false;
  public boolean headersSent = false;
  
  private String filename = null;
  private Flood flood = null;
  private Flood.RuntimeTargetFile runtimeTargetFile = null;
  private File floodingFile = null;
  private RandomAccessFile floodingFileHandle = null;
  private int chunkIndex = 0;
  
  private boolean donestreaming;
  
  public HTTPConnection(ThrumHTTPProxy parent, SocketChannel incomingSocketChannel)
  {
    this.parent = parent;
    
    try
    {
      socketChannel = incomingSocketChannel;
      socketSelector = Selector.open();
      socketChannel.configureBlocking( false );
      socketChannel.register( socketSelector, socketChannel.validOps() );
      byteArrayStream = new ByteArrayOutputStream();
      chunkedStream = new ChunkedOutputStream(byteArrayStream);
    }
    catch(Exception e)
    {
      e.printStackTrace();
      disconnected = true;
    }
  }
  
  public void LoopOnce()
  {
    if( donestreaming )
    {
      disconnected = true;
      return;
    }
    
    if ( socketSelector != null )
    {
      try
      {
        socketSelector.selectNow();
      }
      catch ( Exception e )
      {
        e.printStackTrace();
        disconnected = true;
      }

      // Get list of selection keys with pending events
      Iterator it = socketSelector.selectedKeys().iterator();
      while ( it.hasNext() )
      {
        // Get the selection key
        SelectionKey selKey = (SelectionKey) it.next();

        // Remove it from the list to indicate that it is being processed
        it.remove();

        // 
        if ( selKey.isValid() && selKey.isConnectable() )
        {
          SocketChannel sChannel = (SocketChannel) selKey.channel();

          try
          {
            sChannel.finishConnect();
          }
          catch ( Exception e )
          {
            e.printStackTrace();
            disconnected = true;
          }
        }

        if ( selKey.isValid() && selKey.isReadable() )
        {
          ReadOnce();
        }

        if ( selKey.isValid() && selKey.isWritable() )
        {
          WriteOnce();
        }
      }
    }
    
    Stream();
    
    ProcessReadBuffer();
  }

  private boolean ReadOnce()
  {
    try
    {
      int bytesRead = socketChannel.read( readBuffer );
    }
    catch ( Exception e )
    {
      e.printStackTrace();
      disconnected = true;
      return false;
    }

    return true;
  }

  private boolean WriteOnce()
  {
    return true;
  }
  
  private boolean Stream()
  {
    
    if (disconnected)
    {
      return false;
    }
    
    if (floodingFileHandle != null )
    {
      if(!headersSent)
      {
        try
        {
          writeBuffer.clear();
          writeBuffer.put(new String("HTTP/1.1 200 OK").getBytes());
          writeBuffer.put(crlf);
          writeBuffer.put(new String("Accept-Ranges: bytes").getBytes());
          writeBuffer.put(crlf);
          writeBuffer.put(new String("Content-Length: " + runtimeTargetFile.targetFile.size).getBytes());
          writeBuffer.put(crlf);
          writeBuffer.put(new String("Connection: close").getBytes());
          writeBuffer.put(crlf);
          writeBuffer.put(crlf);
          
          writeBuffer.limit(writeBuffer.position());
          socketChannel.write(writeBuffer);
          writeBuffer.clear();
          headersSent = true;
        }
        catch(Exception e)
        {
          e.printStackTrace();
          disconnected = true;
          return false;
        }
      }
      if(chunkIndex >= runtimeTargetFile.chunkMap.length)
      {
        try
        {
          byte[] zero = {0};
          writeBuffer.clear();
          writeBuffer.put(zero);
          writeBuffer.put(crlf);
          writeBuffer.put(crlf);
          writeBuffer.limit(writeBuffer.position());
          socketChannel.write(writeBuffer);
          writeBuffer.clear();
          socketChannel.close();
          System.out.println("Bytes written: " + bytesWritten);
          disconnected = true;
          return false;
        }
        catch(Exception e)
        {
          e.printStackTrace();
          disconnected = true;
          return false;
        }
      }
      if(runtimeTargetFile.chunkMap[chunkIndex] == '1') {
        Flood.RuntimeChunk chunk = (Flood.RuntimeChunk) runtimeTargetFile.chunks.get(chunkIndex);
        byte[] chunkData = new byte[chunk.size];
        try
        {
          System.out.println("Sending a part of the file (" + chunkIndex + ")");
          floodingFileHandle.seek(runtimeTargetFile.chunkOffsets[chunkIndex]);
          floodingFileHandle.read(chunkData);
          String lenString = Integer.toString(chunkData.length, 16);

          writeBuffer.clear();
          writeBuffer.put(lenString.getBytes());
          writeBuffer.put(crlf);
          writeBuffer.put(chunkData);
          writeBuffer.put(crlf);
          int bytesToWrite = writeBuffer.position();
          writeBuffer.limit(bytesToWrite);
          writeBuffer.rewind();
          
          int bytesOut = 0;
          while(bytesOut < bytesToWrite && writeBuffer.hasRemaining())  // FIXME this loop scares me
          {
            bytesOut += socketChannel.write(writeBuffer);
          }
          System.out.println("wrote: " + bytesOut);
          bytesWritten += bytesOut;
          chunkIndex++;
          return true;
        }
        catch(Exception e)
        {
          e.printStackTrace();
          disconnected = true;
          return false;
        }
      }
    }
    return false;
  }

  private boolean ProcessReadBuffer()
  {
    /*
    while ( readBuffer.position() > readIndex )
    {
      byte b = readBuffer.get( readIndex++ );
      // TODO handle shit
      System.out.print(b);
    }
    */
    byte b = readBuffer.get( readIndex++ );
    if ( b == '\n' )
    {
      int diff = readBuffer.position() - readIndex;
      byte[] message = new byte[readIndex];
      readBuffer.position( 0 );
      readBuffer.get( message, 0, readIndex );
      readBuffer.compact();
      readBuffer.position( diff );
      readIndex = 0;

      String request = new String(message);
      String[] elements = request.split(" ");
      if(elements[0] != null && elements[0].matches("GET"))
      {
        if(elements[1] != null)
        {
          filename = elements[1].substring(1, elements[1].length()); // get rid of the slash
          
          System.out.println("Got a request for: " + filename);
          Iterator floodIter = parent.peer.floods.values().iterator();
          while(floodIter.hasNext())
          {
            Flood tempFlood = (Flood) floodIter.next();
            Iterator fileIter = tempFlood.runtimeTargetFiles.values().iterator();
            while(fileIter.hasNext())
            {
              Flood.RuntimeTargetFile tempRuntimeTargetFile = (Flood.RuntimeTargetFile) fileIter.next();
              if(tempRuntimeTargetFile.name.compareTo(filename) == 0)
              {
                flood = tempFlood;
                runtimeTargetFile = tempRuntimeTargetFile;
                floodingFile = new File(runtimeTargetFile.nameOnDisk);
                try
                {
                  floodingFileHandle = new RandomAccessFile(floodingFile, "r");
                }
                catch(Exception e)
                {
                  e.printStackTrace();
                }
                System.out.println("Found it in flood: " + flood.Id());
                return true;
              }
            }
          }
        }
        
        if(flood == null)
        {
          disconnected = true;
          return false;
        }
      }
    }

    return true;
  }
}