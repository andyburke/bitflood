/*
 * Copyright 1999-2004 The Apache Software Foundation.
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
 * $Log: XMLDTDDescriptionImpl.cpp,v $
 * Revision 1.5  2004/09/08 13:56:50  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.4  2004/04/07 14:10:04  peiyongz
 * systemId (to replace rootElemName) as DTDGrammar Key
 *
 * Revision 1.3  2004/03/03 23:04:38  peiyongz
 * deallocate fRootName when loaded
 *
 * Revision 1.2  2003/10/14 15:20:42  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.1  2003/06/20 18:39:33  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * $Id: XMLDTDDescriptionImpl.cpp,v 1.5 2004/09/08 13:56:50 peiyongz Exp $
 *
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/validators/DTD/XMLDTDDescriptionImpl.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  XMLDTDDescriptionImpl: constructor and destructor
// ---------------------------------------------------------------------------
XMLDTDDescriptionImpl::XMLDTDDescriptionImpl(const XMLCh*          const  systemId
                                           ,       MemoryManager*  const  memMgr  )
:XMLDTDDescription(memMgr)
,fSystemId(0)
,fRootName(0)
{
    if (systemId)
        fSystemId = XMLString::replicate(systemId, memMgr);
}

XMLDTDDescriptionImpl::~XMLDTDDescriptionImpl()
{
    if (fSystemId)
        XMLGrammarDescription::getMemoryManager()->deallocate((void*)fSystemId);

    if (fRootName)
        XMLGrammarDescription::getMemoryManager()->deallocate((void*)fRootName);
}
             
const XMLCh* XMLDTDDescriptionImpl::getGrammarKey() const
{
    return getSystemId();
}
              
const XMLCh* XMLDTDDescriptionImpl::getRootName() const
{ 
    return fRootName; 
}

const XMLCh* XMLDTDDescriptionImpl::getSystemId() const
{ 
    return fSystemId; 
}

void XMLDTDDescriptionImpl::setRootName(const XMLCh* const rootName)
{
    if (fRootName)
    {
        XMLGrammarDescription::getMemoryManager()->deallocate((void*)fRootName);
        fRootName = 0;
    }

    if (rootName)
        fRootName = XMLString::replicate(rootName, XMLGrammarDescription::getMemoryManager()); 
}        

void XMLDTDDescriptionImpl::setSystemId(const XMLCh* const systemId)
{
    if (fSystemId)
    {
        XMLGrammarDescription::getMemoryManager()->deallocate((void*)fSystemId);
        fSystemId = 0;
    }

    if (systemId)
        fSystemId = XMLString::replicate(systemId, XMLGrammarDescription::getMemoryManager()); 
}        

/***
 * Support for Serialization/De-serialization
 ***/

IMPL_XSERIALIZABLE_TOCREATE(XMLDTDDescriptionImpl)

void XMLDTDDescriptionImpl::serialize(XSerializeEngine& serEng)
{
    XMLDTDDescription::serialize(serEng);

    if (serEng.isStoring())
    {
        serEng.writeString(fRootName);
    }
    else
    {
        //the original root name which came from the ctor needs deallocated
        if (fRootName)
        {
            XMLGrammarDescription::getMemoryManager()->deallocate((void*)fRootName);
        }

        serEng.readString((XMLCh*&)fRootName);
    }

}

XMLDTDDescriptionImpl::XMLDTDDescriptionImpl(MemoryManager* const memMgr)
:XMLDTDDescription(memMgr)
,fSystemId(0)
,fRootName(0)
{
}

XERCES_CPP_NAMESPACE_END
