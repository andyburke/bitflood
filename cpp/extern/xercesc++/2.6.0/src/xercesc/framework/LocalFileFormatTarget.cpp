/*
 * Copyright 2002,2004 The Apache Software Foundation.
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
 * $Id: LocalFileFormatTarget.cpp,v 1.10 2004/09/08 13:55:57 peiyongz Exp $
 * $Log: LocalFileFormatTarget.cpp,v $
 * Revision 1.10  2004/09/08 13:55:57  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.9  2003/12/17 13:58:02  cargilld
 * Platform update for memory management so that the static memory manager (one
 * used to call Initialize) is only for static data.
 *
 * Revision 1.8  2003/12/17 00:18:33  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.7  2003/05/16 21:36:55  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.6  2003/05/15 18:26:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2003/01/24 20:20:22  tng
 * Add method flush to XMLFormatTarget
 *
 * Revision 1.4  2003/01/09 20:59:44  tng
 * [Bug 15928] Output with LocalFileFormatTarget fails silently.
 *
 * Revision 1.3  2002/11/27 18:09:25  tng
 * [Bug 13447] Performance: Using LocalFileFormatTarget with DOMWriter is very slow.
 *
 * Revision 1.2  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.1  2002/06/19 21:59:26  peiyongz
 * DOM3:DOMSave Interface support: LocalFileFormatTarget
 *
 *
 */

#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemoryManager.hpp>
#include <xercesc/util/IOException.hpp>
#include <string.h>

XERCES_CPP_NAMESPACE_BEGIN

LocalFileFormatTarget::LocalFileFormatTarget( const XMLCh* const   fileName
                                            , MemoryManager* const manager)
: fSource(0)
, fDataBuf(0)
, fIndex(0)
, fCapacity(1023)
, fMemoryManager(manager)
{
    fSource = XMLPlatformUtils::openFileToWrite(fileName, manager);

    if (!fSource)
        ThrowXMLwithMemMgr1(IOException, XMLExcepts::File_CouldNotOpenFile, fileName, fMemoryManager);

    // Buffer is one larger than capacity, to allow for zero term
    fDataBuf = (XMLByte*) fMemoryManager->allocate
    (
        (fCapacity+4) * sizeof(XMLByte)
    );//new XMLByte[fCapacity+4];

    // Keep it null terminated
    fDataBuf[0] = XMLByte(0);

}

LocalFileFormatTarget::LocalFileFormatTarget( const char* const    fileName
                                            , MemoryManager* const manager)
: fSource(0)
, fDataBuf(0)
, fIndex(0)
, fCapacity(1023)
, fMemoryManager(manager)
{
    fSource = XMLPlatformUtils::openFileToWrite(fileName, manager);

    if (!fSource)
        ThrowXMLwithMemMgr1(IOException, XMLExcepts::File_CouldNotOpenFile, fileName, fMemoryManager);

    // Buffer is one larger than capacity, to allow for zero term
    fDataBuf = (XMLByte*) fMemoryManager->allocate
    (
        (fCapacity+4) * sizeof(XMLByte)
    );//new XMLByte[fCapacity+4];

    // Keep it null terminated
    fDataBuf[0] = XMLByte(0);
}

LocalFileFormatTarget::~LocalFileFormatTarget()
{
    // flush remaining buffer before destroy
    flushBuffer();

    if (fSource)
        XMLPlatformUtils::closeFile(fSource, fMemoryManager);

    fMemoryManager->deallocate(fDataBuf);//delete [] fDataBuf;
}

void LocalFileFormatTarget::flush()
{
    flushBuffer();
}

void LocalFileFormatTarget::writeChars(const XMLByte* const toWrite
                                     , const unsigned int   count
                                     , XMLFormatter * const        )
{
    if (count) {
        insureCapacity(count);
        memcpy(&fDataBuf[fIndex], toWrite, count * sizeof(XMLByte));
        fIndex += count;
    }

    return;
}

void LocalFileFormatTarget::flushBuffer()
{
    // Exception thrown in writeBufferToFile, if any, will be propagated to
    // the XMLFormatter and then to the DOMWriter, which may notify
    // application through DOMErrorHandler, if any.
    XMLPlatformUtils::writeBufferToFile(fSource, (long) fIndex, fDataBuf, fMemoryManager);
    fIndex = 0;
    fDataBuf[0] = 0;
    fDataBuf[fIndex + 1] = 0;
    fDataBuf[fIndex + 2] = 0;
    fDataBuf[fIndex + 3] = 0;
}

void LocalFileFormatTarget::insureCapacity(const unsigned int extraNeeded)
{
    // If we can handle it, do nothing yet
    if (fIndex + extraNeeded < fCapacity)
        return;

    // Oops, not enough room. Calc new capacity and allocate new buffer
    const unsigned int newCap = (unsigned int)((fIndex + extraNeeded) * 2);
    XMLByte* newBuf = (XMLByte*) fMemoryManager->allocate
    (
        (newCap+4) * sizeof(XMLByte)
    );//new XMLByte[newCap+4];

    // Copy over the old stuff
    memcpy(newBuf, fDataBuf, fCapacity * sizeof(XMLByte) + 4);

    // Clean up old buffer and store new stuff
    fMemoryManager->deallocate(fDataBuf); //delete [] fDataBuf;
    fDataBuf = newBuf;
    fCapacity = newCap;

    // flush the buffer too
    flushBuffer();
}

XERCES_CPP_NAMESPACE_END


