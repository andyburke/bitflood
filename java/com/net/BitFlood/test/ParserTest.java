package com.net.BitFlood.test;

/**
 * Created on Nov 28, 2004
 *
 */

import com.net.BitFlood.FloodFile;

/**
 * @author burke
 *  
 */
public class ParserTest
{
  public static void main( String argv[] )
  {
    String floodFilename = "";

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
    }

    System.out.println( "flood filename: " + floodFilename );
    FloodFile floodFile = new FloodFile( floodFilename );

    floodFile.Read();
    floodFile.Dump();
  }

  public static void Usage()
  {
    System.out.println( "Usage:" );
    System.out.println( "parseflood -flood <flood filename>" );
  }
}