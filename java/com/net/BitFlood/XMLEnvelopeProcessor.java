package com.net.BitFlood;

/**
 * Created on Dec 2, 2004
 *
 */

import org.apache.xerces.jaxp.DocumentBuilderFactoryImpl;
import org.apache.xml.serialize.OutputFormat;
import org.apache.xml.serialize.XMLSerializer;
import org.w3c.dom.*;

import com.net.BitFlood.sdk.Base64;

import java.io.StringWriter;
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
    String retVal = null;
    try
    {
      Document doc = docBuilder.newDocument();

      Element methodCallElement = doc.createElement( "methodCall" );

      Element methodNameElement = doc.createElement( "methodName" );
      methodNameElement.appendChild( doc.createTextNode( methodName ) );
      methodCallElement.appendChild( methodNameElement );

      Element params = doc.createElement( "params" );

      Iterator argIter = arguments.iterator();
      while ( argIter.hasNext() )
      {
        Element param = doc.createElement( "param" );
        Element value = doc.createElement( "value" );

        encodeArg( doc, value, argIter.next() );
        param.appendChild( value );
        params.appendChild( param );
      }

      methodCallElement.appendChild( params );
      doc.appendChild( methodCallElement );

      XMLSerializer xmlSerializer = new XMLSerializer();
      StringWriter strWriter = new StringWriter();
      OutputFormat outFormat = new OutputFormat();

      //outFormat.setEncoding( "UTF-8" );
      outFormat.setVersion( "1.0" );
      outFormat.setIndenting( false );
      outFormat.setLineWidth( 0 );
      outFormat.setLineSeparator( "" );

      xmlSerializer.setOutputCharStream( strWriter );
      xmlSerializer.setOutputFormat( outFormat );
      xmlSerializer.serialize( doc );
      
      retVal = strWriter.toString();
    }
    catch ( Exception e )
    {
      Logger.LogError( "Error encoding method: '" + methodName + "': " + e );
    }

    return retVal;
  }

  private static void encodeArg( Document doc, Node node, Object arg ) throws Exception
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
      Element base64 = doc.createElement( "base64" );
      base64.appendChild( doc.createTextNode( Base64.encodeToString( (byte[]) arg, false ) ) );
      node.appendChild( base64 );
    }
    else if ( arg instanceof Vector )
    {
      Vector tmp = (Vector) arg;

      Element array = doc.createElement( "array" );
      Element data = doc.createElement( "data" );
      array.appendChild( data );

      Iterator tmpIter = tmp.iterator();
      while ( tmpIter.hasNext() )
      {
        Object element = tmpIter.next();
        Element value = doc.createElement( "value" );
        encodeArg( doc, value, element );
        data.appendChild( value );
      }

      node.appendChild( array );
    }
    else
    {
      throw new Exception( "Unsupported XML-RPC type: " + arg.toString() );
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
        throw new Exception( "Invalid XML-RPC" );
      }

      methodName.append( methodNameList.item( 0 ).getFirstChild().getNodeValue() );

      NodeList paramsNodeList = doc.getElementsByTagName( "params" );
      if ( paramsNodeList.getLength() == 0 )
      {
        return;
      }

      if ( paramsNodeList.getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC" );
      }

      NodeList paramNodeList = paramsNodeList.item( 0 ).getChildNodes();
      for ( int paramIndex = 0; paramIndex < paramNodeList.getLength(); paramIndex++ )
      {
        Node param = paramNodeList.item( paramIndex );
        if ( param.getNodeName().compareTo( "param" ) != 0 || ( param.getChildNodes().getLength() != 1 ) )
        {
          throw new Exception( "Invalid XML-RPC" );
        }

        Node valueNode = param.getFirstChild();
        arguments.add( decodeArg( valueNode ) );
      }
    }
    catch ( Exception e )
    {
      Logger.LogError( "Error decoding xml: '" + xmlToDecode + "': " + e );
    }
  }

  private static Object decodeArg( Node param ) throws Exception
  {
    if ( param.getNodeName().compareTo( "value" ) != 0 || param.getChildNodes().getLength() != 1 )
    {
      throw new Exception( "Invalid XML-RPC" );
    }

    Node typeNode = param.getFirstChild();
    String typeNodeName = typeNode.getNodeName();
    if ( typeNodeName.compareTo( "string" ) == 0 )
    {
      if ( typeNode.getChildNodes().getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC" );
      }

      typeNode = typeNode.getFirstChild();
      typeNodeName = typeNode.getNodeName();
    }

    if ( typeNodeName.compareTo( "#text" ) == 0 )
    {
      // poor man's xml escaping...
      String tmp = typeNode.getNodeValue();
      tmp.replaceAll( ">", "&gt;" );
      tmp.replaceAll( "<", "&lt;" );
      tmp.replaceAll( "&", "&amp;" );

      return tmp;
    }
    else if ( typeNodeName.compareTo( "int" ) == 0 || typeNodeName.compareTo( "i4" ) == 0 )
    {
      if ( typeNode.getChildNodes().getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC" );
      }

      Node intNode = typeNode.getFirstChild();
      return new Integer( intNode.getNodeValue() );
    }
    else if ( typeNodeName.compareTo( "array" ) == 0 )
    {
      Node dataNode = typeNode.getFirstChild();
      if ( typeNode.getChildNodes().getLength() != 1 || dataNode.getNodeName().compareTo( "data" ) != 0 )
      {
        throw new Exception( "Invalid XML-RPC" );
      }

      Vector objects = new Vector();
      NodeList arrayNodeList = dataNode.getChildNodes();
      for ( int arrayIndex = 0; arrayIndex < arrayNodeList.getLength(); arrayIndex++ )
      {
        objects.add( decodeArg( arrayNodeList.item( arrayIndex ) ) );
      }

      return objects;
    }
    else if ( typeNodeName.compareTo( "base64" ) == 0 )
    {
      if ( typeNode.getChildNodes().getLength() != 1 )
      {
        throw new Exception( "Invalid XML-RPC" );
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
