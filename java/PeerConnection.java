/*
 * Created on Nov 12, 2004
 *
 */

import java.io.*;
import java.nio.channels.*;
import java.net.*;
import java.util.*;

/**
 * @author burke
 *
 */
public class PeerConnection 
{
  private Hashtable         messageHandlers = null;

	private SocketChannel     socketChannel   = null;
  private Selector          socketSelector  = null;
	private InetSocketAddress socketAddress   = null;
	private BufferedReader    bufferedReader  = null;
  
  public Client             client          = null;
  public boolean            connected       = false;
  public boolean            disconnected    = false;
  public String             hostname        = "";
  public int                port            = 0;
  
  public PeerConnection()
  {
  }
  
  public PeerConnection(String remoteHost, int remotePort)
  {
  	hostname = remoteHost;
  	port = remotePort;
  	try
		{
  		socketAddress = new InetSocketAddress(InetAddress.getByName(hostname), port);
		}
  	catch(Exception e)
		{
  		System.out.println("Error getting address for remote host (" + hostname + ":" + port + "): " + e);
  		disconnected = true;
  		return;
		}
  	Connect();
  }
  
  public PeerConnection(SocketChannel incomingSocketConnection)
  {
  	if(incomingSocketConnection == null)
  	{
  		System.out.println("Tried to create a PeerConnection with a null socket?");
  		disconnected = true;
  		return;
  	}
  	
  	socketChannel = incomingSocketConnection;

  	try
		{
  		bufferedReader = new BufferedReader(new InputStreamReader(socketChannel.socket().getInputStream()));
		}
  	catch(Exception e)
		{
  		System.out.println("Error creating bufferedReader on incoming connection: " + e);
  		disconnected = true;
  		return;
		}
  }
  
  public void LoopOnce()
	{
  	
	}
  
  public boolean Connect()
  {
  	if(!connected)
  	{
  		if(socketChannel == null)
  		{
  			try
				{
  				socketChannel = SocketChannel.open();
    			socketChannel.configureBlocking(false);
				}
  			catch(Exception e)
				{
  				System.out.println("Error opening SocketChannel during connect: " + e);
  				disconnected = true; // since this is a weird error
  				return false;
				}

  			try
				{
  				socketSelector = Selector.open();
  				SelectionKey socketConnectKey = socketChannel.register(socketSelector, SelectionKey.OP_CONNECT);
				}
  			catch(Exception e)
				{
  				System.out.println("Error creating socket selector: " + e);
  				disconnected = true;
  				return false;
				}
  			
  			try
				{
  				socketChannel.connect(socketAddress);
				}
  			catch(Exception e)
				{
  				System.out.println("Error connecting to " + hostname + ":" + port + ": " + e);
  				disconnected = true;
  				return false;
				}
  		}
  		else // we have a socketChannel, see if it's connected...
  		{
  			// see: http://www.onjava.com/pub/a/onjava/2002/09/04/nio.html?page=3
  		}
  	}
  	
  	return true;
  }
}
