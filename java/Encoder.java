/*
 * Created on Nov 15, 2004
 *
 * TODO To change the template for this generated file go to
 * Window - Preferences - Java - Code Style - Code Templates
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
 *
 * TODO To change the template for this generated type comment go to
 * Window - Preferences - Java - Code Style - Code Templates
 */
public class Encoder {
	private String floodFilename;
	private int chunkSize;
	private String weightingFunction;
	private String[] trackers;
	private String[] filesToEncode;
	
	// simple constructor
	public Encoder(String floodFilename,
			           int chunkSize,
								 String weightingFunction,
								 String[] trackers,
								 String[] filesToEncode) {

	  this.floodFilename = floodFilename;
	  this.chunkSize = chunkSize;
	  this.weightingFunction = weightingFunction;
	  this.trackers = trackers;
	  this.filesToEncode = filesToEncode;
	  
	}

	public void Encode() {
		DocumentBuilderFactory docBuilderFactory = null;
		DocumentBuilder docBuilder = null;
		Document document = null;
		
		Element root;
		Element fileInfo;
		Element item;
		
		// set up the objects we need to encode the flood data to XML
		try {
			docBuilderFactory = DocumentBuilderFactoryImpl.newInstance();
			docBuilder = docBuilderFactory.newDocumentBuilder();
			document = docBuilder.newDocument();
		} catch(Exception e) {
			System.out.println("Error: " + e);
			System.exit(0);
		}
		
		// create the root node
		root = document.createElement("BitFlood");
		
		// create the fileinfo node and append it to the root
		fileInfo = document.createElement("FileInfo");
		root.appendChild(fileInfo);
		
		// add any tracker nodes
		for(int i = 0; i < trackers.length; i++) {
		  if(trackers[i] == null)
		  {
		  	break;
		  }
		  
		  item = document.createElement("Tracker");
		  item.appendChild(document.createTextNode(trackers[i]));
		  root.appendChild(item);
		}
		
	  // for all the files to encode
	  for(int i = 0; i < filesToEncode.length; i++) {
	  	if(filesToEncode[i] == null)
	  	{
	  		break;
	  	}
	  	
	  	
	  	File file = new File(filesToEncode[i]);
	  	
	  	Element fileNode = document.createElement("File");
	  	fileNode.setAttribute("name", filesToEncode[i]); // FIXME: have to cleanse the filename to spec (unix)
	  	fileNode.setAttribute("Size", Long.toString(file.length()));
	  	fileInfo.appendChild(fileNode);
	  	
	  	if(file.isDirectory())
	  	{
	  		// TODO: recurse and add all files
	  	}
	  	else
	  	{
	  		InputStream inputFileStream = null;
	  		try {
	  			inputFileStream = new FileInputStream(file);
	  		} catch(Exception e) {
	  			System.out.println("Error: " + e);
	  			System.exit(0);
	  		}
	      
	  		byte[] chunkData = new byte[chunkSize];
	      
        int chunkIndex = 0;
	      int offset = 0;
	      int bytesRead = 0;
	      int weight = 0;
	      while (offset < file.length()) {
	        
	      	try {
	      		bytesRead = inputFileStream.read(chunkData, 0, chunkSize);
	      	} catch(IOException e) {
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
	      	try {
	      		MessageDigest messagedigest = MessageDigest.getInstance("SHA-1");
	      		messagedigest.update(chunkData);
	      		digest = messagedigest.digest();
	      	} catch (Exception e) { 
		         System.out.println(e.toString());
		      }
	      	chunk.setAttribute("hash", base64Encoder.encode(digest));
	      	chunk.setAttribute("index", Integer.toString(chunkIndex++));
	      	chunk.setAttribute("size", Integer.toString(bytesRead));
	      	chunk.setAttribute("weight", Integer.toString(weight));

	      	fileNode.appendChild(chunk);
	      	offset += bytesRead;
	      }
	      
	      try {
	        inputFileStream.close();
	      } catch (IOException e) {
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

	    try {
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
	      try {
	      	outputFile = new FileOutputStream(floodFilename);
	      } catch (Exception e) {
	      	System.out.println("Error: " + e);
	      	System.exit(0);
	      }
	      outputFile.write(strWriter.toString().getBytes());
	      //System.out.print(strWriter.toString());
	      strWriter.close();
	      outputFile.close();

	    } catch (IOException ioEx) {
	        System.out.println("Error " + ioEx);
	    }
	  	
	  }
	}
}
