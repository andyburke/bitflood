/*
 * Created on Nov 15, 2004
 *
 */

/**
 * @author burke
 *
 */
public class EncoderTest 
{
  public static void main(String argv[]) 
  {
    String floodFilename = "";
    String weightingFunction = "";
    String[] trackers = new String[10];
    String[] filesToAdd = new String[10];
    int chunkSize = 256 * 1024;
		
    int numTrackers = 0;
    int numFilesToAdd = 0;
		
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
          System.exit(0);
        }
      }
      else if(argv[i].compareToIgnoreCase("-tracker") == 0)
      {
        if(numTrackers > trackers.length) 
        {
          int newSize = 2 * trackers.length;
          String[] tempTrackers = new String[newSize];
          System.arraycopy(trackers, 0, tempTrackers, 0, trackers.length);
          trackers = tempTrackers;
        }
        trackers[numTrackers++] = argv[++i];
      }
      else if(argv[i].compareToIgnoreCase("-chunksize") == 0)
      {
        chunkSize = Integer.parseInt(argv[++i]);
      }
      else if(argv[i].compareToIgnoreCase("-weighting") == 0)
      {
        weightingFunction = argv[++i];
      }
      else
      {
        if(numFilesToAdd > filesToAdd.length) 
        {
          int newSize = 2 * filesToAdd.length;
          String[] tempFilesToAdd = new String[newSize];
          System.arraycopy(filesToAdd, 0, tempFilesToAdd, 0, filesToAdd.length);
          filesToAdd = tempFilesToAdd;
        }
        filesToAdd[numFilesToAdd++] = argv[i];
      }
    }
		
    if(numTrackers == 0)
    {
      System.out.println("No trackers specified!");
      Usage();
      System.exit(0);
    }
		
    if(numFilesToAdd == 0)
    {
      System.out.println("No files specified for flood!");
      Usage();
      System.exit(0);
    }

    FloodFile floodFile = new FloodFile(floodFilename, chunkSize, trackers);
    
    System.out.println("flood filename: " + floodFilename);
    System.out.println("weighting function: " + weightingFunction);
    for(int i = 0; i < trackers.length; i++) 
    {
      if(trackers[i] == null)
      {
        break;
      }
      if(trackers[i].length() > 0)
      {
        System.out.println("tracker: " + trackers[i]);
      }
    }
    
    for(int i = 0; i < filesToAdd.length; i++) 
    {
      if(filesToAdd[i] == null)
      {	
        break;
      }
      if(filesToAdd[i].length() > 0)
      {
        System.out.println("adding file: " + filesToAdd[i]);
        if(!floodFile.Add(filesToAdd[i]))
        {
          System.out.println("error adding file: " + filesToAdd[i]);
          System.exit(0);
        }
      }
    }

    floodFile.Write();
  }
  
  public static void Usage() 
  {
    System.out.println("Usage:");
    System.out.println("createflood -flood <flood filename> -tracker <url> [-tracker <url>...] [-chunksize <kilobytes>] [-weighting [topheavy|bottomheavy|topheavyperfile|bottomheavyperfile]] <files/dirs to add>");
  }
}
