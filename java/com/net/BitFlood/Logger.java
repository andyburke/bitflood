package com.net.BitFlood;

/**
 * Created on Nov 28, 2004
 *
 */

/**
 * @author burke
 *  
 */
public class Logger
{
  public static void LogError( final String out )
  {
    System.out.println( "ERROR: " + out );
  }

  public static void LogNormal( final String out )
  {
    System.out.println( out );
  }
}
