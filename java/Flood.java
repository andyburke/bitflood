/*
 * Created on Nov 12, 2004
 *
 */

/**
 * @author burke
 *
 */
public class Flood 
{
  private PeerConnection[] peers     = null;
  private FloodFile        floodFile = null;
  
  public Flood()
  {
  }
  
  public Flood(String floodFilename)
  {
    floodFile = new FloodFile(floodFilename);	
  }
  
  public String Id()
  {
    return floodFile.contentHash;
  }
  
  public void LoopOnce()
  {
    if(peers == null)
    {
      return;
    }
  	
    for(int peerIndex = 0; peerIndex < peers.length; peerIndex++)
    {
      if(peers[peerIndex] == null)
      {
        break;
      }
  		
      peers[peerIndex].LoopOnce();
    }
  	
    return;
  }
}
