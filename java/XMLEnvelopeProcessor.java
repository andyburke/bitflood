/**
 * Created on Dec 2, 2004
 *
 */

import org.apache.xerces.jaxp.DocumentBuilderFactoryImpl;
import org.apache.xerces.parsers.DOMParser;
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

  private static MessageDigest sha1Encoder = null;

  private Document                      doc               = null;

  static
  {
    try
    {
      docBuilderFactory = DocumentBuilderFactoryImpl.newInstance();
      docBuilder = docBuilderFactory.newDocumentBuilder();

      sha1Encoder = MessageDigest.getInstance( "SHA-1" );
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

  public String encode( String methodName, Vector arguments )
  {
    doc = docBuilder.newDocument();
    
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

      value.appendChild( encodeArg( argIter.nextElement() ) );
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

    doc = null;

    return strWriter.toString();

  }

  private Element encodeArg( Object arg )
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
      return string;
    }
    else if ( arg instanceof Integer )
    {
      Integer tmp = (Integer) arg;
      Element integer = doc.createElement( "int" );
      integer.appendChild( doc.createTextNode( tmp.toString() ) );
      return integer;
    }
    else if ( arg instanceof byte[] )
    {
      byte[] tmp = (byte[]) arg;
      Element base64 = doc.createElement( "base64");
     
      sha1Encoder.reset();
      sha1Encoder.update( tmp );
      base64.appendChild( doc.createTextNode( Base64.encodeToString( sha1Encoder.digest(), false)));
      return base64;
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
        value.appendChild( encodeArg( element ) );
        data.appendChild( value );
      }
      return array;
    }
    else
    {
      System.out.println( arg.toString() );
      System.out.println( "Uknown type encountered!" );
      return null;
    }
  }

}