package com.net.BitFlood.test;

/*
 * Created on Nov 15, 2004
 *
 */
import java.util.Iterator;
import java.util.Vector;
import com.net.BitFlood.FloodFile;

/**
 * @author burke
 *  
 */
public class EncoderTest
{
  public static void main( String argv[] )
  {
    String floodFilename = "";
    String weightingFunction = "";
    Vector trackers = new Vector();
    Vector filesToAdd = new Vector();
    int chunkSize = 256 * 1024;

    for ( int i = 0; i < argv.length; i++ )
    {
      if ( argv[i].compareToIgnoreCase( "-flood" ) == 0 )
      {
        if ( floodFilename.length() == 0 )
        {
          floodFilename = argv[++i];
        }
        else
        {
          System.out.println( "Attempted to specify multiple flood files!" );
          Usage();
          System.exit( 0 );
        }
      }
      else if ( argv[i].compareToIgnoreCase( "-tracker" ) == 0 )
      {
        trackers.add( argv[++i] );
      }
      else if ( argv[i].compareToIgnoreCase( "-chunksize" ) == 0 )
      {
        chunkSize = Integer.parseInt( argv[++i] );
      }
      else if ( argv[i].compareToIgnoreCase( "-weighting" ) == 0 )
      {
        weightingFunction = argv[++i];
      }
      else
      {
        filesToAdd.add( argv[i] );
      }
    }

    if ( trackers.size() == 0 )
    {
      System.out.println( "No trackers specified!" );
      Usage();
      System.exit( 0 );
    }

    if ( filesToAdd.size() == 0 )
    {
      System.out.println( "No files specified for flood!" );
      Usage();
      System.exit( 0 );
    }

    FloodFile floodFile = new FloodFile( floodFilename, chunkSize );

    System.out.println( "flood filename: " + floodFilename );
    System.out.println( "weighting function: " + weightingFunction );
   
    Iterator trackeriter = trackers.iterator();
    for ( ; trackeriter.hasNext(); )
    {
      String tracker = (String)trackeriter.next();
      System.out.println( "adding tracker: " + tracker );
      if ( !floodFile.AddTracker( tracker ) )
      {
        System.out.println( "error adding tracker: " + tracker );
        System.exit( 0 );
      }
    }
    
    Iterator fileiter = filesToAdd.iterator();
    for ( ; fileiter.hasNext(); )
    {
      String file = (String)fileiter.next();
      System.out.println( "adding file: " + file );
      if ( !floodFile.Add( file ) )
      {
        System.out.println( "error adding file: " + file );
        System.exit( 0 );
      }
    }

    floodFile.Write();
  }

  public static void Usage()
  {
    System.out.println( "Usage:" );
    System.out
        .println( "createflood -flood <flood filename> -tracker <url> [-tracker <url>...] [-chunksize <kilobytes>] [-weighting [topheavy|bottomheavy|topheavyperfile|bottomheavyperfile]] <files/dirs to add>" );
  }
}