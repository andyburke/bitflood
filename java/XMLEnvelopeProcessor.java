/**
 * Created on Dec 2, 2004
 *
 */

import org.apache.xerces.jaxp.DocumentBuilderFactoryImpl;
import org.apache.xml.serialize.OutputFormat;
import org.apache.xml.serialize.XMLSerializer;
import org.w3c.dom.*;

import sdk.Base64.Base64;

import java.io.IOException;
import java.io.StringWriter;
import java.security.MessageDigest;
import java.util.*;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

/**
 * @author burke
 *
 */
public class XMLEnvelopeProcessor
{
  private static DocumentBuilderFactory docBuilderFactory = null;
  private static DocumentBuilder        docBuilder        = null;

  static
  {
    try
    {
      docBuilderFactory = DocumentBuilderFactoryImpl.newInstance();
      docBuilder = docBuilderFactory.newDocumentBuilder();
    }
    catch ( Exception e )
    {
      e.printStackTrace();
      System.exit( 0 );
    }

  }

  public XMLEnvelopeProcessor()
  {
  }

  public static String encode( String methodName, Vector arguments )
  {
    Document doc = docBuilder.newDocument();
    
    Element methodCallElement = doc.createElement( "methodCall" );

    Element methodNameElement = doc.createElement( "methodName" );
    methodNameElement.appendChild( doc.createTextNode( methodName ) );
    methodCallElement.appendChild( methodNameElement );

    Element params = doc.createElement( "params" );

    Enumeration argIter = arguments.elements();
    while ( argIter.hasMoreElements() )
    {
      Element param = doc.createElement( "param" );
      Element value = doc.createElement( "value" );

      encodeArg( doc, value, argIter.nextElement() );
      param.appendChild( value );
      params.appendChild( param );
    }

    methodCallElement.appendChild( params );
    doc.appendChild( methodCallElement );

    XMLSerializer xmlSerializer = null;
    StringWriter strWriter = null;
    OutputFormat outFormat = null;
    try
    {
      xmlSerializer = new XMLSerializer();
      strWriter = new StringWriter();
      outFormat = new OutputFormat();

      //outFormat.setEncoding( "UTF-8" );
      outFormat.setVersion( "1.0" );
      outFormat.setIndenting( false );
      outFormat.setLineWidth( 0 );
      outFormat.setLineSeparator( "" );

      xmlSerializer.setOutputCharStream( strWriter );
      xmlSerializer.setOutputFormat( outFormat );
    }
    catch ( Exception e )
    {
      e.printStackTrace();
    }

    try
    {
      xmlSerializer.serialize( doc );
    }
    catch ( Exception e )
    {
      e.printStackTrace();
    }

    return strWriter.toString();
  }

  private static void encodeArg( Document doc, Node node, Object arg )
  {
    if ( arg instanceof String )
    {
      String tmp = (String) arg;
      // poor man's xml escaping...
      tmp.replaceAll( ">", "&gt;" );
      tmp.replaceAll( "<", "&lt;" );
      tmp.replaceAll( "&", "&amp;" );
      Element string = doc.createElement( "string" );
      string.appendChild( doc.createTextNode( tmp ) );
      node.appendChild( string );
    }
    else if ( arg instanceof Integer )
    {
      Integer tmp = (Integer) arg;
      Element integer = doc.createElement( "int" );
      integer.appendChild( doc.createTextNode( tmp.toString() ) );
      node.appendChild( integer );
    }
    else if ( arg instanceof byte[] )
    {
      Element base64 = doc.createElement( "base64");
      base64.appendChild( doc.createTextNode( Base64.encodeToString( (byte[]) arg, false)));
      node.appendChild( base64 );
    }
    else if ( arg instanceof Vector )
    {
      Vector tmp = (Vector) arg;

      Element array = doc.createElement( "array" );
      Element data = doc.createElement( "data" );
      array.appendChild( data );

      Enumeration tmpIter = tmp.elements();
      while ( tmpIter.hasMoreElements() )
      {
        Object element = tmpIter.nextElement();
        Element value = doc.createElement( "value" );
        encodeArg( doc, value, element );
        data.appendChild( value );
      }
      
      node.appendChild( array );
    }
    else
    {
      System.out.println( arg.toString() );
      System.out.println( "Unknown type encountered!" );
    }
  }
  
  public static void decode( final String xmlToDecode, StringBuffer methodName, Vector arguments )
  {
    try
    {
      Document doc = docBuilder.parse( new java.io.ByteArrayInputStream( xmlToDecode.getBytes() ) );

      // should only be one methodname tag
      // get our method name
      NodeList methodNameList = doc.getElementsByTagName( "methodName" );
      if ( methodNameList.getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC");
      }
    
      methodName.append( methodNameList.item( 0 ).getFirstChild().getNodeValue() );
        
      NodeList paramsNodeList = doc.getElementsByTagName( "params" );
      if ( paramsNodeList.getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC");
      }
      
      NodeList paramNodeList = paramsNodeList.item(0).getChildNodes();
      for ( int paramIndex = 0; paramIndex < paramNodeList.getLength(); paramIndex++ )
      {
        Node param = paramNodeList.item( paramIndex );
        if ( !param.getNodeName().contentEquals( "param" ) || ( param.getChildNodes().getLength() != 1 ) )
        {
          throw new Exception( "Invalid XML-RPC");
        }
        
        Node valueNode = param.getFirstChild();
        arguments.add( decodeArg( valueNode ) );
      }
    }
    catch ( Exception e )
    {
      System.out.println( "Error: " + e );
    }
  }
  
  private static Object decodeArg( Node param ) throws Exception
  {
    if ( !param.getNodeName().contentEquals( "value") || param.getChildNodes().getLength() != 1 )
    {
      throw new Exception( "Invalid XML-RPC");
    }
    
    Node typeNode = param.getFirstChild();
    String typeNodeName = typeNode.getNodeName();
    if ( typeNodeName.contentEquals("string") )
    {
      if ( typeNode.getChildNodes().getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC");
      }
      
      typeNode = typeNode.getFirstChild();
    }
    
    if ( typeNodeName.contentEquals("#text") )
    {
      // poor man's xml escaping...
      String tmp = typeNode.getNodeValue();
      tmp.replaceAll( ">", "&gt;" );
      tmp.replaceAll( "<", "&lt;" );
      tmp.replaceAll( "&", "&amp;" );
      
      return tmp;
    }
    else if ( typeNodeName.contentEquals("int") || typeNodeName.contentEquals("int") )
    {
      if ( typeNode.getChildNodes().getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC");
      }
      
      Node intNode = typeNode.getFirstChild();
      return new Integer( intNode.getNodeValue() );
    }
    else if ( typeNodeName.contentEquals("array") )
    {
      Node dataNode = typeNode.getFirstChild(); 
      if ( typeNode.getChildNodes().getLength() != 1 || !dataNode.getNodeName().contentEquals( "data" ) )
      {
        throw new Exception( "Invalid XML-RPC");
      }
      
      Vector objects = new Vector();
      NodeList arrayNodeList = dataNode.getChildNodes();
      for ( int arrayIndex = 0; arrayIndex < arrayNodeList.getLength(); arrayIndex++ )
      {
        objects.add( decodeArg( arrayNodeList.item( arrayIndex ) ) );
      }
      
      return objects;
    }
    else if ( typeNodeName.contentEquals("base64") )
    {
      if ( typeNode.getChildNodes().getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC");
      }
      
      Node base64Node = typeNode.getFirstChild();
      return Base64.decode( base64Node.getNodeValue() );
    }
    else 
    {
      throw new Exception( "Unsupported XML-RPC type: " + typeNodeName );
    }
  }
}
