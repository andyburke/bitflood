#include "stdafx.H"
#include "Encoder.H"
#include <io.h>
#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

XERCES_CPP_NAMESPACE_USE

namespace libBitFlood
{
  namespace Encoder
  {
    // ---------------------------------------------------------------------------
    //  This is a simple class that lets us do easy (though not terribly efficient)
    //  trancoding of char* data to XMLCh data.
    // ---------------------------------------------------------------------------
    class XStr
    {
    public :
      // -----------------------------------------------------------------------
      //  Constructors and Destructor
      // -----------------------------------------------------------------------
      XStr(const char* const toTranscode)
      {
        // Call the private transcoding method
        fUnicodeForm = XMLString::transcode(toTranscode);
      }

      ~XStr()
      {
        XMLString::release(&fUnicodeForm);
      }


      // -----------------------------------------------------------------------
      //  Getter methods
      // -----------------------------------------------------------------------
      const XMLCh* unicodeForm() const
      {
        return fUnicodeForm;
      }

    private :
      // -----------------------------------------------------------------------
      //  Private data members
      //
      //  fUnicodeForm
      //      This is the Unicode XMLCh format of the string.
      // -----------------------------------------------------------------------
      XMLCh*   fUnicodeForm;
    };

#define X(str) XStr(str).unicodeForm()


    /*static*/
    Error::ErrorCode EncodeFile( const ToEncode& i_toencode, std::string& o_xml )
    {
      Error::ErrorCode ret = Error::NO_ERROR;

      // check incoming data - move to a function in ToEncode?
      if ( i_toencode.m_files.empty() )
      {
        ret = Error::UNKNOWN_ERROR;
      }
      else if ( i_toencode.m_chunksize <= 0 )
      {
        ret = Error::UNKNOWN_ERROR;
      }
      else
      {
        V_String::const_iterator fileiter = i_toencode.m_files.begin();
        V_String::const_iterator fileend  = i_toencode.m_files.end();

        U32 num_bytes = i_toencode.m_chunksize / sizeof( U8 );
        U8* buffer = new U8[ num_bytes ];


        DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

        if (impl != NULL)
        {  
          try
          {
            DOMDocument* doc = impl->createDocument( 0,                    // root element namespace URI.
						     X("BitFlood"),         // root element name
						     0);                   // document type object (DTD).

            DOMElement* rootElem = doc->getDocumentElement();

            DOMElement* fileInfoElem = doc->createElement(X("FileInfo"));
            rootElem->appendChild(fileInfoElem);


            for ( ; fileiter != fileend; ++fileiter )
            {
              U32 filesize = 0;
              FILE* file = fopen( fileiter->c_str(), "rb" );
              if ( file == NULL )
              {
                ret = Error::UNKNOWN_ERROR;
              }
              else
              {
                DOMElement*  fileElem = doc->createElement(X("File"));
                fileInfoElem->appendChild(fileElem);

                fileElem->setAttribute( X("name"), X(fileiter->c_str()));

                U32 index = 0;
                for( ;; ++index )
                {
                  U32 bytesRead = (U32)fread( buffer, sizeof( U8 ), num_bytes, file );
                  if ( bytesRead > 0 )
                  {
                    filesize += bytesRead;

                    DOMElement*  chunkElem = doc->createElement(X("Chunk"));
                    fileElem->appendChild(chunkElem);

                    using namespace CryptoPP;
                    SHA sha;
                    HashFilter shaFilter(sha);
                    std::auto_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
                    channelSwitch->AddDefaultRoute(shaFilter);

                    StringSource( buffer, bytesRead, true, channelSwitch.release() );
                    std::stringstream out;
                    Base64Encoder encoder( new FileSink( out ), false );
                    shaFilter.TransferTo( encoder );

                    std::stringstream indexStrm;
                    indexStrm << index;

                    std::stringstream sizeStrm;
                    sizeStrm << bytesRead;

                    chunkElem->setAttribute( X("hash"), X(out.str().c_str()) );
                    chunkElem->setAttribute( X("index"), X(indexStrm.str().c_str()) );
                    chunkElem->setAttribute( X("size"), X(sizeStrm.str().c_str()) );
                    chunkElem->setAttribute( X("weight"), X("0") );
                  }
                  else
                  {
                    break;
                  }
                }

                fclose( file );

                std::stringstream sizeStrm;
                sizeStrm << filesize;

                fileElem->setAttribute( X("size"), X(sizeStrm.str().c_str()));
              }
            }

            DOMElement* trackerElem = doc->createElement(X("Tracker"));
            rootElem->appendChild(trackerElem);

            DOMText* trackerName = doc->createTextNode(X("Test"));
            trackerElem->appendChild(trackerName);

            DOMWriter* writer = impl->createDOMWriter();
            writer->setFeature( XMLUni::fgDOMWRTFormatPrettyPrint, 1 );
            std::wstring tmp ( writer->writeToString( *rootElem ) );

            FILE* outfile = fopen( "c:\\test.mp3.flood", "w" );
            fprintf( outfile, "%ws", tmp.c_str() );
            fclose( outfile );
            

          }
          catch (const OutOfMemoryException&)
          {
            XERCES_STD_QUALIFIER cerr << "OutOfMemoryException" << XERCES_STD_QUALIFIER endl;
            ret = Error::UNKNOWN_ERROR;
          }
          catch (const DOMException& e)
          {
            XERCES_STD_QUALIFIER cerr << "DOMException code is:  " << e.code << XERCES_STD_QUALIFIER endl;
            ret = Error::UNKNOWN_ERROR;
          }
          catch (...)
          {
            XERCES_STD_QUALIFIER cerr << "An error occurred creating the document" << XERCES_STD_QUALIFIER endl;
            ret = Error::UNKNOWN_ERROR;
          }
        }

        delete [] buffer;
      }
      return ret;
    }
  }
};

