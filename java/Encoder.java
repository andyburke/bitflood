/*
 * Created on Nov 15, 2004
 *
 */

import java.io.*;
import java.security.MessageDigest;

import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.apache.xerces.jaxp.DocumentBuilderFactoryImpl;
import org.apache.xml.serialize.*;

import sun.misc.BASE64Encoder;

/**
 * @author burke
 */
public class Encoder 
{
  private String floodFilename;
  private int chunkSize;
  private String weightingFunction;
  private String[] trackers;
  private String[] pathsToEncode;
	
  // simple constructor
  public Encoder(String floodFilename,
                 int chunkSize,
                 String weightingFunction,
                 String[] trackers,
                 String[] filesToEncode) 
  {
    this.floodFilename = floodFilename;
    this.chunkSize = chunkSize;
    this.weightingFunction = weightingFunction;
    this.trackers = trackers;
    this.pathsToEncode = filesToEncode;
	  
  }

  public void Encode() 
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
    catch(Exception e) 
    {
      System.out.println("Error: " + e);
      System.exit(0);
    }
		
    // create the root node
    root = document.createElement("BitFlood");
		
    // create the fileinfo node and append it to the root
    fileInfo = document.createElement("FileInfo");
    root.appendChild(fileInfo);
		
    // add any tracker nodes
    for(int i = 0; i < trackers.length; i++) 
    {
      if(trackers[i] == null)
      {
        break;
      }
		  
      item = document.createElement("Tracker");
      item.appendChild(document.createTextNode(trackers[i]));
      root.appendChild(item);
    }
		
    String[] filesToEncode = new String[32];
    // for all the files to encode
    for(int pathToEncodeIndex = 0; pathToEncodeIndex < pathsToEncode.length; pathToEncodeIndex++) 
    {
      if(pathsToEncode[pathToEncodeIndex] == null)
      {
        break;
      }
	  	
	  	
      File file = new File(pathsToEncode[pathToEncodeIndex]);
      pathsToEncode[pathToEncodeIndex] = file.getAbsolutePath(); // change this to be the abs path for cleanup later on
	  	
      if(file.isDirectory())
      {
        pathsToEncode[pathToEncodeIndex] = pathsToEncode[pathToEncodeIndex] + '/'; // for cleanup, later on

        File dir = file; // just a little less confusing
        String[] childFiles = new String[32];
        RecursiveFilenameFind(dir.getPath(), childFiles);
        int foundFilesIndex = 0;
        for(; foundFilesIndex < filesToEncode.length; foundFilesIndex++) 
        {
          if(filesToEncode[foundFilesIndex] == null)
          {
            break;
          }
        }

        for(int childIndex = 0; childIndex < childFiles.length; childIndex++) 
        {
          if(childFiles[childIndex] == null)
          {
            break;
          }
	  			
          if(foundFilesIndex + childIndex >= filesToEncode.length)
          {
            int newSize = 2 * filesToEncode.length;
            String[] tempFilesToEncode = new String[newSize];
            System.arraycopy(filesToEncode, 0, tempFilesToEncode, 0, filesToEncode.length);
            filesToEncode = tempFilesToEncode;
          }
	  			
          if(filesToEncode[foundFilesIndex + childIndex] == null)
          {
            filesToEncode[foundFilesIndex + childIndex] = childFiles[childIndex];
          }
          else
          {
            System.out.println("Error, filesToEncode has gaps?");
            System.exit(0);
          }
        }
      }
      else // just a regular file
      {
        int foundFilesIndex = 0;
        for(; foundFilesIndex < filesToEncode.length; foundFilesIndex++) 
        {
          if(filesToEncode[foundFilesIndex] == null)
          {
            break;
          }
        }

        if(foundFilesIndex >= filesToEncode.length)
        {
          int newSize = 2 * filesToEncode.length;
          String[] tempFilesToEncode = new String[newSize];
          System.arraycopy(filesToEncode, 0, tempFilesToEncode, 0, filesToEncode.length);
          filesToEncode = tempFilesToEncode;
        }
	  		
        filesToEncode[foundFilesIndex] = file.getAbsolutePath();
      }
	  	
      for(int fileToEncodeIndex = 0; fileToEncodeIndex < filesToEncode.length; fileToEncodeIndex++) 
      {
        if(filesToEncode[fileToEncodeIndex] == null)
        {
          break;
        }
	  		
        File fileToEncode = new File(filesToEncode[fileToEncodeIndex]);
        Element fileNode = document.createElement("File");
        String cleanFilename = null;
        for(int k = 0; k < pathsToEncode.length; k++) 
        {
          if(pathsToEncode[k] == null)
          {
            break;
          }
          if(filesToEncode[fileToEncodeIndex].startsWith(pathsToEncode[k]))
          {
            String tempPathToEncode = pathsToEncode[k];
            tempPathToEncode = tempPathToEncode.replace('\\', '/');
		  			
            if(tempPathToEncode.endsWith("/"))
            {
              int lastSlashIndex = tempPathToEncode.lastIndexOf('/', tempPathToEncode.length() - 2);
              cleanFilename = filesToEncode[fileToEncodeIndex].substring(lastSlashIndex + 1);
              cleanFilename = cleanFilename.replace('\\', '/');
            }
            else
            {
              String tempFilename = filesToEncode[fileToEncodeIndex];
              tempFilename = tempFilename.replace('\\', '/');
              cleanFilename = tempFilename.substring(tempFilename.lastIndexOf('/'), tempFilename.length());
            }
          }
        }
        fileNode.setAttribute("name", cleanFilename); // FIXME: have to cleanse the filename to spec (unix)
        fileNode.setAttribute("Size", Long.toString(fileToEncode.length()));
        fileInfo.appendChild(fileNode);

        InputStream inputFileStream = null;
        try 
        {
          inputFileStream = new FileInputStream(fileToEncode);
        } 
        catch(Exception e) 
        {
          System.out.println("Error: " + e);
          System.exit(0);
        }
	      
        byte[] chunkData = new byte[chunkSize];
	      
        int chunkIndex = 0;
        int offset = 0;
        int bytesRead = 0;
        int weight = 0;
        while (offset < fileToEncode.length()) 
        {
          try 
          {
            bytesRead = inputFileStream.read(chunkData, 0, chunkSize);
          } 
          catch(IOException e) 
          {
            System.out.println("Error: " + e);
            System.exit(0);
          }
	      	
          if( bytesRead <= 0)
          {
            break;
          }
	      	
          BASE64Encoder base64Encoder = new BASE64Encoder();
          byte[] digest = null;
	      	
          // sha1 the chunk
          Element chunk = document.createElement("Chunk");
          try 
          {
            MessageDigest messagedigest = MessageDigest.getInstance("SHA-1");
            messagedigest.update(chunkData);
            digest = messagedigest.digest();
          } 
          catch (Exception e) 
          { 
            System.out.println(e.toString());
          }

          chunk.setAttribute("hash", base64Encoder.encode(digest));
          chunk.setAttribute("index", Integer.toString(chunkIndex++));
          chunk.setAttribute("size", Integer.toString(bytesRead));
          chunk.setAttribute("weight", Integer.toString(weight));

          fileNode.appendChild(chunk);
          offset += bytesRead;
        }
	      
        try 
        {
          inputFileStream.close();
        }
        catch (IOException e) 
        {
          System.out.println("Error: " + e);
          System.exit(0);
        }
      }
	  	
      // add the root node to the document
      document.appendChild(root);
	  	
      // write it all out
      StringWriter  strWriter       = null;
      XMLSerializer xmlSerializer   = null;
      OutputFormat  outFormat       = null;

      try 
      {
        xmlSerializer = new XMLSerializer();
        strWriter = new StringWriter();
        outFormat = new OutputFormat();

        outFormat.setEncoding("UTF-8");
        outFormat.setVersion("1.0");
        outFormat.setIndenting(true);
        outFormat.setIndent(2);
        outFormat.setLineWidth(0);
	      
        xmlSerializer.setOutputCharStream(strWriter);
        xmlSerializer.setOutputFormat(outFormat);

        xmlSerializer.serialize(document);
	      
        OutputStream outputFile = null;
        try 
        {
          outputFile = new FileOutputStream(floodFilename);
        } 
        catch (Exception e) 
        {
          System.out.println("Error: " + e);
          System.exit(0);
        }

        outputFile.write(strWriter.toString().getBytes());
        //System.out.print(strWriter.toString());
        strWriter.close();
        outputFile.close();

      } 
      catch (IOException ioEx) 
      {
        System.out.println("Error " + ioEx);
      }
	  	
    }
  }

  private boolean RecursiveFilenameFind(String root, String[] result) 
  {
    File currentDir = new File(root);
    if(currentDir.isDirectory()) 
    {
      File[] subFiles = currentDir.listFiles();
      for(int i = 0; i < subFiles.length; i++) 
      {
        if(subFiles[i] == null)
        {
          break;
        }
				
        if(subFiles[i].isDirectory()) 
        {
          RecursiveFilenameFind(subFiles[i].getAbsolutePath(), result);
        } 
        else 
        {
          int resultIndex = 0;
          for(; resultIndex < result.length; resultIndex++) 
          {
            if(result[resultIndex] == null)
            {	
              break;
            }
          }
					
          if(resultIndex >= result.length) 
          {
            String[] temp = new String[2 * result.length];
            System.arraycopy(result, 0, temp, 0, result.length);
            result = temp;
          }
          result[resultIndex] = subFiles[i].getAbsolutePath();
        }
      }
    }

    return true;
  }
}
