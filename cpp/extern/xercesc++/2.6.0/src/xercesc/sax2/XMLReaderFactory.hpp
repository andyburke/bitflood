/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Log: XMLReaderFactory.hpp,v $
 * Revision 1.7  2004/09/08 13:56:20  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.6  2004/01/29 11:46:32  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.5  2003/06/20 18:56:45  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * Revision 1.4  2003/05/15 18:27:11  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/11/04 14:55:45  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/05/07 17:45:52  knoaman
 * SAX2 documentation update.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:09  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/08/30 22:21:37  andyh
 * Unix Build script fixes.  Clean up some UNIX compiler warnings.
 *
 * Revision 1.2  2000/08/07 18:21:27  jpolast
 * change SAX_EXPORT module to SAX2_EXPORT
 *
 * Revision 1.1  2000/08/02 18:02:35  jpolast
 * initial checkin of sax2 implementation
 * submitted by Simon Fell (simon@fell.com)
 * and Joe Polastre (jpolast@apache.org)
 *
 *
 */

#ifndef XMLREADERFACTORY_HPP
#define XMLREADERFACTORY_HPP

#include <xercesc/parsers/SAX2XMLReaderImpl.hpp>
#include <xercesc/sax/SAXException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class MemoryManager;
class XMLGrammarPool;

/**
  * Creates a SAX2 parser (SAX2XMLReader).
  *
  * <p>Note: The parser object returned by XMLReaderFactory is owned by the
  * calling users, and it's the responsiblity of the users to delete that
  * parser object, once they no longer need it.</p>
  *
  * @see SAX2XMLReader#SAX2XMLReader
  */
class SAX2_EXPORT XMLReaderFactory
{
protected:                // really should be private, but that causes compiler warnings.
	XMLReaderFactory() ;
	~XMLReaderFactory() ;

public:
	static SAX2XMLReader * createXMLReader( 
                                               MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
                                             , XMLGrammarPool* const gramPool = 0
                                               ) ;
	static SAX2XMLReader * createXMLReader(const XMLCh* className)  ;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLReaderFactory(const XMLReaderFactory&);
    XMLReaderFactory& operator=(const XMLReaderFactory&);
};


inline SAX2XMLReader * XMLReaderFactory::createXMLReader(MemoryManager* const  manager
                                                       , XMLGrammarPool* const gramPool)
{
	return (SAX2XMLReader*)(new (manager) SAX2XMLReaderImpl(manager, gramPool));
}

inline SAX2XMLReader * XMLReaderFactory::createXMLReader(const XMLCh *)
{	
	throw SAXNotSupportedException();
	// unimplemented
	return 0;
}

XERCES_CPP_NAMESPACE_END

#endif
