/**
 * Created on Nov 28, 2004
 *
 */

/**
 * @author burke
 *
 */
public class ClientTest 
{
  public static void main(String argv[]) 
  {
    String floodFilename = "";
    String localIP = "";
    int localPort = 10101;

    try
    {
      java.net.InetAddress localMachine =
        java.net.InetAddress.getLocalHost();	
      localIP = localMachine.getHostAddress();
    }
    catch(java.net.UnknownHostException uhe)
    {
      //handle exception
    }	

    for(int i = 0; i < argv.length; i++) 
    {
      if(argv[i].compareToIgnoreCase("-flood") == 0)
      {
        if(floodFilename.length() == 0)
        {
          floodFilename = argv[++i];
        }
        else
        {
          System.out.println("Attempted to specify multiple flood files!");
          Usage();
        }
      }
      else if(argv[i].compareToIgnoreCase("-localIP") == 0)
      {
        localIP = argv[++i];
      }
      else if(argv[i].compareToIgnoreCase("-localPort") == 0)
      {
        localPort = Integer.parseInt( argv[++i] );
      }
    }

    if ( floodFilename.length() == 0 )
    {
      Usage();
    }
		
    System.out.println("flood filename: " + floodFilename);
    System.out.println("local ip      : " + localIP);
    System.out.println("flood port    : " + localPort);
  }
  
  public static void Usage() 
  {
    System.out.println("Usage:");
    System.out.println("ClientTest -flood <flood filename> [-localIP <localIP>] [-localPort <localPort>]");
    System.exit(0);
  }
}
