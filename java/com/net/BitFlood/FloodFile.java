package com.net.BitFlood;

import java.util.*;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.StringWriter;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.apache.xerces.jaxp.DocumentBuilderFactoryImpl;
import org.apache.xerces.parsers.DOMParser;
import org.apache.xml.serialize.OutputFormat;
import org.apache.xml.serialize.XMLSerializer;
import org.w3c.dom.*;

/*
 * Created on Nov 12, 2004
 *
 */

/**
 * @author burke
 *  
 */
public class FloodFile
{
  public class Chunk implements Comparable
  {
    public String hash;
    public int    index;
    public int    size;
    public long   offset;
    public int    weight;

    public int compareTo( Object anotherChunk ) throws ClassCastException
    {
      if ( !( anotherChunk instanceof Chunk ) )
      {
        throw new ClassCastException( "A Chunk object expected." );
      }
      return this.index - ( (Chunk) anotherChunk ).index;
    }
  }

  public class TargetFile implements Comparable
  {
    public String name;
    public long   size;
    public Vector chunks = new Vector();

    public int compareTo( Object anotherTargetFile ) throws ClassCastException
    {
      if ( !( anotherTargetFile instanceof TargetFile ) )
      {
        throw new ClassCastException( "A Person object expected." );
      }
      return this.name.compareTo( ( (TargetFile) anotherTargetFile ).name );
    }
  }

  public class TrackerInfo
  {
    public String host;
    public int    port;
    public String id;
  }

  public Vector    trackers    = new Vector();
  public Hashtable targetFiles = new Hashtable();
  public int       chunkSize;
  public String    filePath;
  public String    contentHash;

  public FloodFile(String filePath)
  {
    this.filePath = filePath;
    this.chunkSize = 256 * 1024; // default to 256K
  }

  public FloodFile(String filePath, int chunkSize)
  {
    this.filePath = filePath;
    this.chunkSize = chunkSize;
  }

  public boolean Read()
  {
    InputStream inputFileStream = null;
    File inputFile = null;
    try
    {
      inputFile = new File( filePath );
      inputFileStream = new FileInputStream( inputFile );
    }
    catch ( Exception e )
    {
      System.out.println( "Error: " + e );
      System.exit( 0 );
    }

    try
    {
      DataInputStream dataStream = new DataInputStream( inputFileStream );
      FromXML( dataStream );
      inputFileStream.close();
    }
    catch ( IOException ioEx )
    {
      System.out.println( "Error: " + ioEx );
      System.exit( 0 );
    }

    return true;
  }

  public boolean Write()
  {
    OutputStream outputFile = null;
    try
    {
      outputFile = new FileOutputStream( filePath );
    }
    catch ( Exception e )
    {
      System.out.println( "Error: " + e );
      System.exit( 0 );
    }

    try
    {
      //    	System.out.println(ToXML());
      outputFile.write( ToXML().getBytes() );
      outputFile.close();
    }
    catch ( IOException ioEx )
    {
      System.out.println( "Error: " + ioEx );
      System.exit( 0 );
    }

    return true;
  }

  public void Dump()
  {
    Logger.LogNormal( "ContentHash: " + contentHash );
    Logger.LogNormal( "# Files    : " + targetFiles.size() );
    Logger.LogNormal( "# Trackers : " + trackers.size() );

    Logger.LogNormal( "Files:" );
    Object[] sortedTargetFiles = targetFiles.values().toArray();
    Arrays.sort( sortedTargetFiles );
    for ( int targetFileIndex = 0; targetFileIndex < sortedTargetFiles.length; targetFileIndex++ )
    {
      TargetFile file = (TargetFile)sortedTargetFiles[targetFileIndex];
      Logger.LogNormal( "    name: " + file.name );
      Logger.LogNormal( "    size: " + file.size );
      Logger.LogNormal( "  chunks: " + file.chunks.size() );
    }

    Logger.LogNormal( "Trackers:" );
    Iterator trackeriter = trackers.iterator();
    for ( ; trackeriter.hasNext(); )
    {
      TrackerInfo tracker = (TrackerInfo) trackeriter.next();
      Logger.LogNormal( "  address: " + tracker.host + ":" + tracker.port );
    }
  }

  public String ToXML()
  {
    DocumentBuilderFactory docBuilderFactory = null;
    DocumentBuilder docBuilder = null;
    Document document = null;

    Element root;
    Element fileInfo;
    Element item;

    // set up the objects we need to encode the flood data to XML
    try
    {
      docBuilderFactory = DocumentBuilderFactoryImpl.newInstance();
      docBuilder = docBuilderFactory.newDocumentBuilder();
      document = docBuilder.newDocument();
    }
    catch ( Exception e )
    {
      System.out.println( "Error: " + e );
      System.exit( 0 );
    }

    // create the root node
    root = document.createElement( "BitFlood" );

    // create the fileinfo node and append it to the root
    fileInfo = document.createElement( "FileInfo" );
    root.appendChild( fileInfo );

    // add any tracker nodes
    Iterator trackeriter = trackers.iterator();
    for ( ; trackeriter.hasNext(); )
    {
      TrackerInfo tracker = (TrackerInfo) trackeriter.next();
      item = document.createElement( "Tracker" );
      item.setAttribute( "host", tracker.host );
      item.setAttribute( "port", Integer.toString( tracker.port ) );
      root.appendChild( item );
    }

    Object[] sortedTargetFiles = targetFiles.values().toArray();
    Arrays.sort( sortedTargetFiles );
    for ( int targetFileIndex = 0; targetFileIndex < sortedTargetFiles.length; targetFileIndex++ )
    {
      TargetFile file = (TargetFile)sortedTargetFiles[targetFileIndex];

      Element fileNode = document.createElement( "File" );
      fileNode.setAttribute( "name", file.name ); // FIXME: have to cleanse the filename to spec (unix)
      fileNode.setAttribute( "size", Long.toString( file.size ) );
      fileInfo.appendChild( fileNode );

      Iterator chunkiter = file.chunks.iterator();
      for ( ; chunkiter.hasNext(); )
      {
        Chunk chunk = (Chunk) chunkiter.next();

        Element chunkNode = document.createElement( "Chunk" );
        chunkNode.setAttribute( "index", Long.toString( chunk.index ) );
        chunkNode.setAttribute( "hash", chunk.hash );
        chunkNode.setAttribute( "size", Long.toString( chunk.size ) );
        chunkNode.setAttribute( "weight", Long.toString( chunk.weight ) );
        fileNode.appendChild( chunkNode );
      }
    }

    // add the root node to the document
    document.appendChild( root );

    // write it all out
    StringWriter strWriter = null;
    XMLSerializer xmlSerializer = null;
    OutputFormat outFormat = null;

    String result = null;
    try
    {
      xmlSerializer = new XMLSerializer();
      strWriter = new StringWriter();
      outFormat = new OutputFormat();

      outFormat.setEncoding( "UTF-8" );
      outFormat.setVersion( "1.0" );
      outFormat.setIndenting( true );
      outFormat.setIndent( 2 );
      outFormat.setLineWidth( 0 );

      xmlSerializer.setOutputCharStream( strWriter );
      xmlSerializer.setOutputFormat( outFormat );

      xmlSerializer.serialize( document );

      result = strWriter.toString();

      strWriter.close();
    }
    catch ( IOException ioEx )
    {
      System.out.println( "Error: " + ioEx );
      System.exit( 0 );
    }

    return result;
  }

  public void FromXML( DataInputStream inputStream )
  {
    DOMParser parser = new DOMParser();
    try
    {
      parser.setFeature( "http://xml.org/sax/features/validation", false ); // don't
      // validate
    }
    catch ( Exception e )
    {
      System.out.println( "Error: " + e );
      System.exit( 0 );
    }

    try
    {
      parser.parse( new org.xml.sax.InputSource( inputStream ) );
      Document floodDataDoc = parser.getDocument();

      // get all our files and fill out our 'files' array
      NodeList fileinfoList = floodDataDoc.getElementsByTagName( "FileInfo" );

      // should only be one fileinfo tag
      if ( fileinfoList.getLength() == 1 )
      {
        Element fileinfo = (Element) fileinfoList.item( 0 );
        NodeList fileList = fileinfo.getElementsByTagName( "File" );
        if ( fileList.getLength() > 0 )
        {
          for ( int fileIndex = 0; fileIndex < fileList.getLength(); fileIndex++ )
          {
            Element file = (Element) fileList.item( fileIndex );
            TargetFile targetFile = new TargetFile();

            targetFile.size = Long.parseLong( file.getAttribute( "size" ) );
            targetFile.name = file.getAttribute( "name" );

            NodeList chunkList = file.getElementsByTagName( "Chunk" );
            if ( chunkList.getLength() > 0 )
            {
              targetFile.chunks.setSize( chunkList.getLength() );

              for ( int chunkIndex = 0; chunkIndex < chunkList.getLength(); chunkIndex++ )
              {
                Element chunk = (Element) chunkList.item( chunkIndex );
                Chunk tempChunk = new Chunk();

                tempChunk.index = Integer.parseInt( chunk.getAttribute( "index" ) );
                tempChunk.weight = Integer.parseInt( chunk.getAttribute( "weight" ) );
                tempChunk.size = Integer.parseInt( chunk.getAttribute( "size" ) );
                tempChunk.hash = chunk.getAttribute( "hash" );
                
                targetFile.chunks.set( tempChunk.index, tempChunk );
              }
            }

            targetFiles.put( targetFile.name, targetFile );
          }
        }
      }

      // get all our trackers
      NodeList trackersList = floodDataDoc.getElementsByTagName( "Tracker" );
      if ( trackersList.getLength() > 0 )
      {
        trackers.setSize( trackersList.getLength() );
        for ( int trackerIndex = 0; trackerIndex < trackersList.getLength(); trackerIndex++ )
        {
          Element tracker = (Element) trackersList.item( trackerIndex );
          TrackerInfo tempTracker = new TrackerInfo();
          tempTracker.host = tracker.getAttribute( "host" );
          tempTracker.port = Integer.parseInt( tracker.getAttribute( "port" ) );

          try
          {
            java.net.InetAddress localMachine = java.net.InetAddress.getByName( tempTracker.host );
            tempTracker.host = localMachine.getHostAddress();
          }
          catch ( java.net.UnknownHostException uhe )
          {
            System.out.println( "Could not resolve: " + tempTracker.host + ": " + uhe );
          }

          String idstring = tempTracker.host + tempTracker.port;
          tempTracker.id = Encoder.SHA1Base64Encode( idstring );

          trackers.set( trackerIndex, tempTracker );
        }
      }
      else
      {
        System.out.println( "No trackers in flood file?" );
      }

      //compute our content hash
      ComputeContentHash();
    }
    catch ( Exception e )
    {
      System.out.println( "FloodFile FromXML parse error: " + e );
    }
  }

  public boolean AddTracker( final String trackerAddress )
  {
    TrackerInfo tracker = new TrackerInfo();

    int h_start = trackerAddress.indexOf( "http://" ) + ( new String( "http://" ).length() );
    int p_start = trackerAddress.indexOf( ':', h_start ) + 1;
    int u_start = trackerAddress.indexOf( '/', p_start ) + 1;

    tracker.host = trackerAddress.substring( h_start, p_start - 1 );
    tracker.port = Integer.parseInt( trackerAddress.substring( p_start, u_start - 1 ) );
    trackers.add( tracker );
    return true;
  }

  public boolean Add( final String path )
  {
    File file = new File( path );
    boolean result = false;

    if ( file.exists() && file.canRead() )
    {
      if ( file.isDirectory() )
      {
        result = AddDirectory( file.getAbsolutePath() );
      }
      else
      {
        result = AddFile( file.getAbsolutePath() );
      }
    }

    return result;
  }

  // FIXME this should throw exceptions
  public boolean AddDirectory( final String dirPath )
  {
    File dirToAdd = new File( dirPath );

    if ( !dirToAdd.isDirectory() )
    {
      return false;
    }

    String absDirPath = dirToAdd.getAbsolutePath() + '/'; // change this to be
    // the abs path for
    // cleanup later on
    absDirPath = absDirPath.replace( '\\', '/' );

    String[] childFiles = new String[32];

    RecursiveFilenameFind( absDirPath, childFiles );

    for ( int childIndex = 0; childIndex < childFiles.length; childIndex++ )
    {
      if ( childFiles[childIndex] == null )
      {
        break;
      }

      // FIXME i'm do a lot of shit on this one line...
      if ( !AddFile( childFiles[childIndex], childFiles[childIndex].replaceFirst( absDirPath, absDirPath.substring( absDirPath.lastIndexOf( '/' ) ) ) ) )
      {
        return false;
      }
    }

    return true;
  }

  public boolean AddFile( final String localFilePath )
  {
    String localPath = null;
    String targetPath = null;

    localPath = localFilePath.replace( '\\', '/' );

    int lastSlash;
    if ( ( lastSlash = localPath.lastIndexOf( '/' ) ) != -1 )
    {
      targetPath = localPath.substring( lastSlash, localPath.length() );
    }
    else
    {
      targetPath = localPath;
    }

    return AddFile( localPath, targetPath );
  }

  public boolean AddFile( final String localPath, final String targetPath )
  {
    File fileToAdd = new File( localPath );
    if ( !fileToAdd.exists() || !fileToAdd.canRead() )
    {
      return false;
    }

    TargetFile file = new TargetFile();
    file.name = targetPath;
    file.size = fileToAdd.length();
    targetFiles.put( file.name, file );

    InputStream inputFileStream = null;
    try
    {
      inputFileStream = new FileInputStream( fileToAdd );
    }
    catch ( Exception e )
    {
      System.out.println( "Error: " + e );
      System.exit( 0 );
    }

    byte[] chunkData = new byte[chunkSize];

    int chunkIndex = 0;
    int offset = 0;
    int bytesRead = 0;
    int weight = 0;
    while ( offset < fileToAdd.length() )
    {
      try
      {
        bytesRead = inputFileStream.read( chunkData, 0, chunkSize );
        offset += bytesRead; // FIXME this shouldn't be necessary, we should
        // just detect the end of the stream
      }
      catch ( IOException e )
      {
        System.out.println( "Error: " + e );
        System.exit( 0 );
      }

      if ( bytesRead <= 0 )
      {
        break;
      }

      Chunk chunk = new Chunk();
      chunk.hash = Encoder.SHA1Base64Encode( chunkData, bytesRead );
      chunk.index = chunkIndex;
      chunk.size = bytesRead;
      chunk.weight = weight;
      file.chunks.add( chunk );
      chunkIndex++;
    }

    try
    {
      inputFileStream.close();
    }
    catch ( IOException e )
    {
      System.out.println( "Error: " + e );
      System.exit( 0 );
    }

    return true;
  }

  private void ComputeContentHash()
  {
    if ( contentHash == null )
    {
      // calculate the content hash
      String content = "";

      Object[] sortedTargetFiles = targetFiles.values().toArray();
      Arrays.sort( sortedTargetFiles );

      Iterator fileiter = Arrays.asList( sortedTargetFiles ).iterator();
      for ( ; fileiter.hasNext(); )
      {
        TargetFile file = (TargetFile) fileiter.next();
        content = content + file.name;

        java.util.Collections.sort( file.chunks );
        Iterator chunkiter = file.chunks.iterator();
        for ( ; chunkiter.hasNext(); )
        {
          Chunk chunk = (Chunk) chunkiter.next();
          content = content + chunk.hash;
        }
      }

      Logger.LogNormal( "Hashing on: " + content );
      contentHash = Encoder.SHA1Base64Encode( content );
      Logger.LogNormal( "Produced hash: " + contentHash );
    }
  }

  private boolean RecursiveFilenameFind( String root, String[] result )
  {
    File currentDir = new File( root );
    if ( currentDir.isDirectory() )
    {
      File[] subFiles = currentDir.listFiles();
      for ( int i = 0; i < subFiles.length; i++ )
      {
        if ( subFiles[i] == null )
        {
          break;
        }

        if ( subFiles[i].isDirectory() )
        {
          RecursiveFilenameFind( subFiles[i].getAbsolutePath(), result );
        }
        else
        {
          int resultIndex = 0;
          for ( ; resultIndex < result.length; resultIndex++ )
          {
            if ( result[resultIndex] == null )
            {
              break;
            }
          }

          if ( resultIndex >= result.length )
          {
            String[] temp = new String[2 * result.length];
            System.arraycopy( result, 0, temp, 0, result.length );
            result = temp;
          }
          result[resultIndex] = subFiles[i].getAbsolutePath();
        }
      }
    }

    return true;
  }
}
