/*
 * Copyright 2003-2004 The Apache Software Foundation.
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
 * $Id: XSerializeEngine.cpp,v 1.19 2004/09/08 13:56:14 peiyongz Exp $
 * $Log: XSerializeEngine.cpp,v $
 * Revision 1.19  2004/09/08 13:56:14  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.18  2004/07/21 14:54:39  peiyongz
 * using the supplied memory manager , patch from David Bertoni
 *
 * Revision 1.17  2004/03/05 22:21:45  peiyongz
 * readBytes()/writeBytes between BinOutputStream/BinInputStream and
 * XSerializeEngine will always be the full size of the buffer to maintain the exact
 * position for aligned data.
 *
 * Revision 1.16  2004/03/01 23:19:03  peiyongz
 * Grant XSerializeEngine access to GrammarPool
 *
 * Revision 1.15  2004/02/20 20:57:39  peiyongz
 * Bug#27046: path from David Bertoni
 *
 * Revision 1.14  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.13  2004/01/15 23:42:32  peiyongz
 * proper allignment for built-in datatype read/write
 *
 * Revision 1.12  2004/01/13 16:34:20  cargilld
 * Misc memory management changes.
 *
 * Revision 1.11  2004/01/12 16:27:41  neilg
 * remove use of static buffers
 *
 * Revision 1.10  2003/12/17 00:18:34  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.9  2003/11/25 20:37:40  jberry
 * Cleanup build errors/warnings from CodeWarrior
 *
 * Revision 1.8  2003/10/20 17:37:05  amassari
 * Removed compiler warning
 *
 * Revision 1.7  2003/10/17 21:09:03  peiyongz
 * renaming methods
 *
 * Revision 1.6  2003/10/07 19:38:31  peiyongz
 * API for Template_Class Object Serialization/Deserialization
 *
 * Revision 1.5  2003/09/25 22:22:00  peiyongz
 * Introduction of readString/writeString
 *
 * Revision 1.4  2003/09/25 15:21:12  peiyongz
 * Loose the assert condition so that Serializable class need NOT to check the
 * actual string length before read/write.
 *
 * Revision 1.3  2003/09/23 18:11:29  peiyongz
 * Using HashPtr
 *
 * Revision 1.1  2003/09/18 18:31:24  peiyongz
 * OSU: Object Serialization Utilities
 *
 *
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/internal/XSerializeEngine.hpp>
#include <xercesc/internal/XSerializable.hpp>
#include <xercesc/internal/XProtoType.hpp>

#include <xercesc/util/HashPtr.hpp>

#include <xercesc/framework/XMLGrammarPool.hpp>
#include <xercesc/framework/BinOutputStream.hpp>
#include <xercesc/util/BinInputStream.hpp>

XERCES_CPP_NAMESPACE_BEGIN

const bool XSerializeEngine::toWriteBufferLen = true;
const bool XSerializeEngine::toReadBufferLen  = true;

static const int noDataFollowed = -1;

static const XSerializeEngine::XSerializedObjectId_t fgNullObjectTag  = 0;           // indicating null ptrs
static const XSerializeEngine::XSerializedObjectId_t fgNewClassTag    = 0xFFFFFFFF;  // indicating new class
static const XSerializeEngine::XSerializedObjectId_t fgTemplateObjTag = 0xFFFFFFFE;  // indicating template object
static const XSerializeEngine::XSerializedObjectId_t fgClassMask      = 0x80000000;  // indicates class tag
static const XSerializeEngine::XSerializedObjectId_t fgMaxObjectCount = 0x3FFFFFFD;  

#define TEST_THROW_ARG1(condition, data, err_msg) \
if (condition) \
{ \
    XMLCh value1[17]; \
    XMLString::binToText(data, value1, 16, 10, getMemoryManager()); \
    ThrowXMLwithMemMgr1(XSerializationException \
            , err_msg  \
            , value1 \
            , getMemoryManager()); \
}

#define TEST_THROW_ARG2(condition, data1, data2, err_msg) \
if (condition) \
{ \
    XMLCh value1[17]; \
    XMLCh value2[17]; \
    XMLString::binToText(data1, value1, 16, 10, getMemoryManager()); \
    XMLString::binToText(data2, value2, 16, 10, getMemoryManager()); \
    ThrowXMLwithMemMgr2(XSerializationException \
            , err_msg  \
            , value1   \
            , value2 \
            , getMemoryManager()); \
}

// ---------------------------------------------------------------------------
//  Constructor and Destructor
// ---------------------------------------------------------------------------
XSerializeEngine::~XSerializeEngine()
{
    if (isStoring())
    {
        flush();
        delete fStorePool;
    }
    else
    {
        delete fLoadPool;
    }

    getMemoryManager()->deallocate(fBufStart);

}

XSerializeEngine::XSerializeEngine(BinInputStream*         inStream
                                 , XMLGrammarPool* const   gramPool
                                 , unsigned long           bufSize)
:fStoreLoad(mode_Load)
,fGrammarPool(gramPool)
,fInputStream(inStream)
,fOutputStream(0)
,fBufSize(bufSize)
,fBufStart( (XMLByte*) gramPool->getMemoryManager()->allocate(bufSize))
,fBufEnd(0)
,fBufCur(fBufStart)
,fBufLoadMax(fBufStart)
,fStorePool(0)
,fLoadPool( new (gramPool->getMemoryManager()) ValueVectorOf<void*>(29, gramPool->getMemoryManager(), false))
,fObjectCount(0)
{
    /*** 
     *  initialize buffer from the inStream
     ***/
    fillBuffer();

}

XSerializeEngine::XSerializeEngine(BinOutputStream*        outStream
                                 , XMLGrammarPool* const   gramPool
                                 , unsigned long           bufSize)
:fStoreLoad(mode_Store)
,fGrammarPool(gramPool)
,fInputStream(0)
,fOutputStream(outStream)
,fBufSize(bufSize)
,fBufStart((XMLByte*) gramPool->getMemoryManager()->allocate(bufSize))
,fBufEnd(fBufStart+bufSize)
,fBufCur(fBufStart)
,fBufLoadMax(0)
,fStorePool( new (gramPool->getMemoryManager()) RefHashTableOf<XSerializedObjectId>(29, true, new (gramPool->getMemoryManager()) HashPtr(), gramPool->getMemoryManager()) )
,fLoadPool(0)
,fObjectCount(0)
{
    //initialize store pool
    fStorePool->put(0, new (gramPool->getMemoryManager()) XSerializedObjectId(fgNullObjectTag));

}

// ---------------------------------------------------------------------------
// Deprecated Constructor 
// ---------------------------------------------------------------------------
XSerializeEngine::XSerializeEngine(BinInputStream*         inStream
                                 , MemoryManager* const    manager
                                 , unsigned long           bufSize)
:fStoreLoad(mode_Load)
,fGrammarPool(0)
,fInputStream(inStream)
,fOutputStream(0)
,fBufSize(bufSize)
,fBufStart( (XMLByte*) manager->allocate(bufSize))
,fBufEnd(0)
,fBufCur(fBufStart)
,fBufLoadMax(fBufStart)
,fStorePool(0)
,fLoadPool( new (manager) ValueVectorOf<void*>(29, manager, false))
,fObjectCount(0)
{
    /*** 
     *  initialize buffer from the inStream
     ***/
    fillBuffer();

}

XSerializeEngine::XSerializeEngine(BinOutputStream*        outStream
                                 , MemoryManager* const    manager
                                 , unsigned long           bufSize)
:fStoreLoad(mode_Store)
,fGrammarPool(0)
,fInputStream(0)
,fOutputStream(outStream)
,fBufSize(bufSize)
,fBufStart((XMLByte*) manager->allocate(bufSize))
,fBufEnd(fBufStart+bufSize)
,fBufCur(fBufStart)
,fBufLoadMax(0)
,fStorePool( new (manager) RefHashTableOf<XSerializedObjectId>(29, true, new (manager) HashPtr(), manager) )
,fLoadPool(0)
,fObjectCount(0)
{
    //initialize store pool
    fStorePool->put(0, new (manager) XSerializedObjectId(fgNullObjectTag));

}

void XSerializeEngine::flush()
{
    if (isStoring())
        flushBuffer();

}

// ---------------------------------------------------------------------------
//  Storing 
// ---------------------------------------------------------------------------
void XSerializeEngine::write(XSerializable* const objectToWrite)
{
    ensureStoring();
    //don't ensurePointer here !!!

    XSerializedObjectId_t   objIndex = 0;

	if (!objectToWrite)  // null pointer
	{
		*this << fgNullObjectTag;
	}
    else if (0 != (objIndex = lookupStorePool((void*) objectToWrite)))
	{
        // writing an object reference tag
        *this << objIndex;
	}
	else
	{
		// write protoType first
		XProtoType* protoType = objectToWrite->getProtoType();
		write(protoType);

		// put the object into StorePool
        addStorePool((void*)objectToWrite);

        // ask the object to serialize itself
		objectToWrite->serialize(*this);
	}

}

void XSerializeEngine::write(XProtoType* const protoType)
{
    ensureStoring();
    ensurePointer(protoType);

	XSerializedObjectId_t objIndex = lookupStorePool((void*)protoType);

    if (objIndex)
    {   
        //protoType seen in the store pool
        *this << (fgClassMask | objIndex);
	}
	else
	{
		// store protoType
		*this << fgNewClassTag;
		protoType->store(*this);
        addStorePool((void*)protoType);
	}

}

/***
 *
***/ 
void XSerializeEngine::write(const XMLCh* const toWrite
                           ,       int          writeLen)
{
    write((XMLByte*)toWrite, (sizeof(XMLCh)/sizeof(XMLByte)) * writeLen);
}


void XSerializeEngine::write(const XMLByte* const toWrite
                           ,       int            writeLen)
{
    ensureStoring();
    ensurePointer((void*)toWrite);
    ensureBufferLen(writeLen);
    ensureStoreBuffer();

    if (writeLen == 0)
        return;

    /***
     *  If the available space is sufficient, write it up
     ***/
    int bufAvail = fBufEnd - fBufCur;

    if (writeLen <= bufAvail)
    {
        memcpy(fBufCur, toWrite, writeLen);
        fBufCur += writeLen;
        return;
    }

    const XMLByte*  tempWrite   = (const XMLByte*) toWrite;
    unsigned int    writeRemain = writeLen;

    // fill up the avaiable space and flush
    memcpy(fBufCur, tempWrite, bufAvail);
    tempWrite   += bufAvail;
    writeRemain -= bufAvail;
    flushBuffer();

    // write chunks of fBufSize
    while (writeRemain >= fBufSize)
    {
        memcpy(fBufCur, tempWrite, fBufSize);
        tempWrite   += fBufSize;
        writeRemain -= fBufSize;
        flushBuffer();
    }

    // write the remaining if any
    if (writeRemain)
    {
        memcpy(fBufCur, tempWrite, writeRemain);
        fBufCur += writeRemain;
    }

}

/***
 *
 *     Storage scheme (normal):
 *
 *     <
 *     1st integer:    bufferLen (optional)
 *     2nd integer:    dataLen
 *     bytes following:
 *     >
 *
 *     Storage scheme (special):
 *     <
 *     only integer:   noDataFollowed
 *     >
 */

void XSerializeEngine::writeString(const XMLCh* const toWrite
                                 , const int          bufferLen
                                 , bool               toWriteBufLen)
{
    if (toWrite) 
    {
        if (toWriteBufLen)
            *this<<bufferLen;

        int strLen = XMLString::stringLen(toWrite);
        *this<<strLen;

        write(toWrite, strLen);
    }
    else
    {
        *this<<noDataFollowed;
    }

}

void XSerializeEngine::writeString(const XMLByte* const toWrite
                                 , const int            bufferLen
                                 , bool                 toWriteBufLen)
{

    if (toWrite) 
    {
        if (toWriteBufLen)
            *this<<bufferLen;

        int strLen = XMLString::stringLen((char*)toWrite);
        *this<<strLen;
        write(toWrite, strLen);
    }
    else
    {
        *this<<noDataFollowed;
    }

}

// ---------------------------------------------------------------------------
//  Loading
// ---------------------------------------------------------------------------
XSerializable* XSerializeEngine::read(XProtoType* const protoType)
{
    ensureLoading();
    ensurePointer(protoType);

	XSerializedObjectId_t    objectTag;
	XSerializable*           objRet;

    if (! read(protoType, &objectTag))
	{
        /***
         * We hava a reference to an existing object in
         * load pool, get it.
         */
        objRet = lookupLoadPool(objectTag);
	}
	else
	{
		// create the object from the prototype
		objRet = protoType->fCreateObject(getMemoryManager());
        Assert((objRet != 0), XMLExcepts::XSer_CreateObject_Fail);  
 
        // put it into load pool 
        addLoadPool(objRet);

        // de-serialize it
		objRet->serialize(*this);

	}

	return objRet;
}

bool XSerializeEngine::read(XProtoType*            const    protoType
                          , XSerializedObjectId_t*          objectTagRet)
{
    ensureLoading();
    ensurePointer(protoType);

	XSerializedObjectId_t obTag;

    *this >> obTag;

    // object reference tag found
    if (!(obTag & fgClassMask))
	{
		*objectTagRet = obTag;
		return false;
	}
    
	if (obTag == fgNewClassTag)
	{
        // what follows fgNewClassTag is the prototype object info
        // for the object anticipated, go and verify the info
        XProtoType::load(*this, protoType->fClassName, getMemoryManager());

        addLoadPool((void*)protoType);
	}
	else
	{
        // what follows class tag is an XSerializable object
		XSerializedObjectId_t classIndex = (obTag & ~fgClassMask);
        XSerializedObjectId_t loadPoolSize = (XSerializedObjectId_t)fLoadPool->size();

        TEST_THROW_ARG2(((classIndex == 0 ) || (classIndex > loadPoolSize))
                  , classIndex
                  , loadPoolSize
                  , XMLExcepts::XSer_Inv_ClassIndex
                  )

        ensurePointer(lookupLoadPool(classIndex));

   }

	return true;
}

void XSerializeEngine::read(XMLCh* const toRead
                          , int          readLen)
{
    read((XMLByte*)toRead, (sizeof(XMLCh)/sizeof(XMLByte))*readLen);
}

void XSerializeEngine::read(XMLByte* const toRead
                          , int            readLen)
{
    ensureLoading();
    ensureBufferLen(readLen);
    ensurePointer(toRead);
    ensureLoadBuffer();

    if (readLen == 0)
        return;

    /***
     *  If unread is sufficient, read it up
     ***/
    int dataAvail = fBufLoadMax - fBufCur;

    if (readLen <= dataAvail)
    {
        memcpy(toRead, fBufCur, readLen);
        fBufCur += readLen;
        return;
    }

    /***
     *
     * fillBuffer will discard anything left in the buffer
     * before it asks the inputStream to fill in the buffer,
     * so we need to readup everything in the buffer before
     * calling fillBuffer
     *
     ***/
    XMLByte*     tempRead   = (XMLByte*) toRead;
    unsigned int readRemain = readLen;

    // read the unread
    memcpy(tempRead, fBufCur, dataAvail);
    tempRead   += dataAvail;
    readRemain -= dataAvail;

    // read chunks of fBufSize
    while (readRemain >= fBufSize)
    {
        fillBuffer();
        memcpy(tempRead, fBufCur, fBufSize);
        tempRead   += fBufSize;
        readRemain -= fBufSize;
    }

    // read the remaining if any
    if (readRemain)
    {
        fillBuffer();
        memcpy(tempRead, fBufCur, readRemain);
        fBufCur += readRemain;
    }

}

/***
 *
 *     Storage scheme (normal):
 *
 *     <
 *     1st integer:    bufferLen (optional)
 *     2nd integer:    dataLen
 *     bytes following:
 *     >
 *
 *     Storage scheme (special):
 *     <
 *     only integer:   noDataFollowed
 *     >
 */
void XSerializeEngine::readString(XMLCh*&  toRead
                                , int&     bufferLen
                                , int&     dataLen
                                , bool     toReadBufLen)
{
    /***
     * Check if any data written
     ***/
    *this>>bufferLen;

    if (bufferLen == noDataFollowed)
    {
        toRead = 0;
        bufferLen = 0;
        dataLen = 0;
        return;
    }

    if (toReadBufLen)
    {
        *this>>dataLen;
    }
    else
    {
        dataLen = bufferLen++;        
    }

    toRead = (XMLCh*) getMemoryManager()->allocate(bufferLen * sizeof(XMLCh));
    read(toRead, dataLen);
    toRead[dataLen] = 0;
}

void XSerializeEngine::readString(XMLByte*&  toRead
                                , int&       bufferLen
                                , int&       dataLen
                                , bool       toReadBufLen)
{
    /***
     * Check if any data written
     ***/
    *this>>bufferLen;
    if (bufferLen == noDataFollowed)
    {
        toRead = 0;
        bufferLen = 0;
        dataLen = 0;
        return;
    }

    if (toReadBufLen)
    {
        *this>>dataLen;
    }
    else
    {
        dataLen = bufferLen++;
    }

    toRead = (XMLByte*) getMemoryManager()->allocate(bufferLen * sizeof(XMLByte));
    read(toRead, dataLen);
    toRead[dataLen] = 0;

}

// ---------------------------------------------------------------------------
//  Insertion
// ---------------------------------------------------------------------------
XSerializeEngine& XSerializeEngine::operator<<(XMLCh xch)
{ 
    checkAndFlushBuffer(sizeof(XMLCh));

    *(XMLCh*)fBufCur = xch; 
    fBufCur += sizeof(XMLCh); 
    return *this; 
}
 
XSerializeEngine& XSerializeEngine::operator<<(XMLByte by)
{ 
    checkAndFlushBuffer(sizeof(XMLByte));

    *(XMLByte*)fBufCur = by; 
    fBufCur += sizeof(XMLByte); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator<<(bool b)
{ 
    checkAndFlushBuffer(sizeof(bool));

    *(bool*)fBufCur = b; 
    fBufCur += sizeof(bool); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator<<(char ch)
{ 
    return XSerializeEngine::operator<<((XMLByte)ch); 
}

XSerializeEngine& XSerializeEngine::operator<<(short sh)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(short));

    allignBufCur();
    *(short*)fBufCur = sh; 
    fBufCur += sizeof(short); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator<<(int i)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(int));

    allignBufCur();
    *(int*)fBufCur = i; 
    fBufCur += sizeof(int); 
    return *this;     
}

XSerializeEngine& XSerializeEngine::operator<<(unsigned int ui)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(unsigned int));

    allignBufCur();
    *(unsigned int*)fBufCur = ui; 
    fBufCur += sizeof(unsigned int); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator<<(long l)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(long));

    allignBufCur();
    *(long*)fBufCur = l; 
    fBufCur += sizeof(long); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator<<(unsigned long ul)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(unsigned long));

    allignBufCur();
    *(unsigned long*)fBufCur = ul; 
    fBufCur += sizeof(unsigned long); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator<<(float f)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(float));

    allignBufCur();
    *(float*)fBufCur = *(float*)&f; 
    fBufCur += sizeof(float); 
    return *this;
}

XSerializeEngine& XSerializeEngine::operator<<(double d)
{ 
    checkAndFlushBuffer(allignAdjust()+sizeof(double));

    allignBufCur();
    *(double*)fBufCur = *(double*)&d; 
    fBufCur += sizeof(double); 
    return *this; 
}

// ---------------------------------------------------------------------------
//  Extraction
// ---------------------------------------------------------------------------
XSerializeEngine& XSerializeEngine::operator>>(XMLCh& xch)
{ 
    checkAndFillBuffer(sizeof(XMLCh));

    xch = *(XMLCh*)fBufCur; 
    fBufCur += sizeof(XMLCh); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(XMLByte& by)
{ 
    checkAndFillBuffer(sizeof(XMLByte));

    by = *(XMLByte*)fBufCur; 
    fBufCur += sizeof(XMLByte); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(bool& b)
{ 
    checkAndFillBuffer(sizeof(bool));

    b = *(bool*)fBufCur; 
    fBufCur += sizeof(bool); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(char& ch)
{ 
    return XSerializeEngine::operator>>((XMLByte&)ch); 
}

XSerializeEngine& XSerializeEngine::operator>>(short& sh)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(short));

    allignBufCur();
    sh = *(short*)fBufCur; 
    fBufCur += sizeof(short); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(int& i)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(int));

    allignBufCur();
    i = *(int*)fBufCur; 
    fBufCur += sizeof(int); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(unsigned int& ui)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(unsigned int));

    allignBufCur();
    ui = *(unsigned int*)fBufCur; 
    fBufCur += sizeof(unsigned int); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(long& l)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(long));

    allignBufCur();
    l = *(long*)fBufCur; 
    fBufCur += sizeof(long); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(unsigned long& ul)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(unsigned long));

    allignBufCur();
    ul = *(unsigned long*)fBufCur; 
    fBufCur += sizeof(unsigned long); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(float& f)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(float));

    allignBufCur();
    *(float*)&f = *(float*)fBufCur; 
    fBufCur += sizeof(float); 
    return *this; 
}

XSerializeEngine& XSerializeEngine::operator>>(double& d)
{ 
    checkAndFillBuffer(allignAdjust()+sizeof(double));

    allignBufCur();
    *(double*)&d = *(double*)fBufCur; 
    fBufCur += sizeof(double); 
    return *this; 
}

// ---------------------------------------------------------------------------
//  StorePool/LoadPool Opertions
// ---------------------------------------------------------------------------
XSerializeEngine::XSerializedObjectId_t 
XSerializeEngine::lookupStorePool(void* const objToLookup) const
{
    //0 indicating object is not in the StorePool
    XSerializedObjectId* data = fStorePool->get(objToLookup);   
    return (XSerializeEngine::XSerializedObjectId_t) (data ? data->getValue() : 0);

}

void XSerializeEngine::addStorePool(void* const objToAdd)
{
    pumpCount();
    fStorePool->put(objToAdd, new (fGrammarPool->getMemoryManager()) XSerializedObjectId(fObjectCount));
}

XSerializable* XSerializeEngine::lookupLoadPool(XSerializedObjectId_t objectTag) const
{

    /***
      *  an object tag read from the binary refering to
      *  an object beyond the upper most boundary of the load pool
      ***/    
    TEST_THROW_ARG2( (objectTag > fLoadPool->size())
              , objectTag
              , fLoadPool->size()
              , XMLExcepts::XSer_LoadPool_UppBnd_Exceed
              )

    if (objectTag == 0)
        return 0;

    /***
     *   A non-null object tag starts from 1 while fLoadPool starts from 0
     ***/
    return (XSerializable*) fLoadPool->elementAt(objectTag - 1);
}

void XSerializeEngine::addLoadPool(void* const objToAdd)
{

    TEST_THROW_ARG2( (fLoadPool->size() != fObjectCount)
               , fObjectCount
               , fLoadPool->size()
               , XMLExcepts::XSer_LoadPool_NoTally_ObjCnt
               )

    pumpCount();
    fLoadPool->addElement(objToAdd);

}

void XSerializeEngine::pumpCount()
{

    TEST_THROW_ARG2( (fObjectCount >= fgMaxObjectCount)
               , fObjectCount
               , fgMaxObjectCount
               , XMLExcepts::XSer_ObjCount_UppBnd_Exceed
              )

    fObjectCount++;

}

// ---------------------------------------------------------------------------
//  Buffer Opertions
// ---------------------------------------------------------------------------
/***
 *
 *  Though client may need only miniBytesNeeded, we always request
 *  a full size reading from our inputStream.
 *
 *  Whatever possibly left in the buffer is abandoned, such as in 
 *  the case of CheckAndFillBuffer() 
 *
 ***/
void XSerializeEngine::fillBuffer()
{
    ensureLoading();
    ensureLoadBuffer();
 
    int bytesRead = fInputStream->readBytes(fBufStart, fBufSize);

    /***
     * InputStream MUST fill in the exact amount of bytes as requested
     * to do: combine the checking and create a new exception code later
     ***/
    TEST_THROW_ARG2( (bytesRead < (int)fBufSize)
               , bytesRead
               , fBufSize
               , XMLExcepts::XSer_InStream_Read_LT_Req
               )

    TEST_THROW_ARG2( (bytesRead > (int)fBufSize)
               , bytesRead
               , fBufSize
               , XMLExcepts::XSer_InStream_Read_OverFlow
               )

    fBufLoadMax = fBufStart + fBufSize;
    fBufCur     = fBufStart;

    ensureLoadBuffer();

}

/***
 *
 *  Flush out whatever left in the buffer, from
 *  fBufStart to fBufEnd.
 *
 ***/
void XSerializeEngine::flushBuffer()
{
    ensureStoring();
    ensureStoreBuffer();

    fOutputStream->writeBytes(fBufStart, fBufSize);
    fBufCur = fBufStart;

    ensureStoreBuffer();
}

inline void XSerializeEngine::checkAndFlushBuffer(int bytesNeedToWrite)
{
    TEST_THROW_ARG1( (bytesNeedToWrite <= 0)
                   , bytesNeedToWrite
                   , XMLExcepts::XSer_Inv_checkFlushBuffer_Size
                   )

    // fBufStart ... fBufCur ...fBufEnd
    if ((fBufCur + bytesNeedToWrite) >= fBufEnd) 
        flushBuffer();
}

inline void XSerializeEngine::checkAndFillBuffer(int bytesNeedToRead)
{

    TEST_THROW_ARG1( (bytesNeedToRead <= 0)
                   , bytesNeedToRead
                   , XMLExcepts::XSer_Inv_checkFillBuffer_Size
                   )

    // fBufStart ... fBufCur ...fBufLoadMax
    if ((fBufCur + bytesNeedToRead) > fBufLoadMax)
    {
        fillBuffer();
    }

}

inline void XSerializeEngine::ensureStoreBuffer() const
{

    TEST_THROW_ARG2 ( !((fBufStart <= fBufCur) && (fBufCur <= fBufEnd))
                    , (int)(fBufCur - fBufStart)
                    , (int)(fBufEnd - fBufCur)
                    , XMLExcepts::XSer_StoreBuffer_Violation
                    )

}

inline void XSerializeEngine::ensureLoadBuffer() const
{

    TEST_THROW_ARG2 ( !((fBufStart <= fBufCur) && (fBufCur <= fBufLoadMax))
                    , (int)(fBufCur - fBufStart)
                    , (int)(fBufLoadMax - fBufCur)
                    , XMLExcepts::XSer_LoadBuffer_Violation
                    )

}

inline void XSerializeEngine::ensurePointer(void* const ptr) const
{

    TEST_THROW_ARG1( (ptr == 0)
                   , 0
                   , XMLExcepts::XSer_Inv_Null_Pointer
                   )

}

inline void XSerializeEngine::ensureBufferLen(int bufferLen) const
{

    TEST_THROW_ARG1( (bufferLen < 0)
                   , bufferLen
                   , XMLExcepts::XSer_Inv_Buffer_Len
                   )

}

// ---------------------------------------------------------------------------
//  Template object
// ---------------------------------------------------------------------------
/***
 *
 *  Search the store pool to see if the address has been seen before or not.
 *
 *  If yes, write the corresponding object Tag to the internal buffer
 *  and return true.
 *
 *  Otherwise, add the address to the store pool and return false
 *  to notifiy the client application code to store the template object.
 *
 ***/
bool XSerializeEngine::needToStoreObject(void* const  templateObjectToWrite)
{
    ensureStoring(); //don't ensurePointer here !!!

    XSerializedObjectId_t   objIndex = 0;

	if (!templateObjectToWrite)  
	{
		*this << fgNullObjectTag; // null pointer
        return false;
	}
    else if (0 != (objIndex = lookupStorePool(templateObjectToWrite)))
	{
        *this << objIndex;         // write an object reference tag
        return false;
	}
	else
	{
        *this << fgTemplateObjTag;            // write fgTemplateObjTag to denote that actual
                                              // template object follows
        addStorePool(templateObjectToWrite); // put the address into StorePool
        return true;
	}

}

bool XSerializeEngine::needToLoadObject(void**  templateObjectToRead)
{
    ensureLoading();

	XSerializedObjectId_t obTag;

    *this >> obTag;
  
	if (obTag == fgTemplateObjTag)
	{
        /***
         * what follows fgTemplateObjTag is the actual template object
         * We need the client application to create a template object
         * and register it through registerObject(), and deserialize
         * template object
         ***/
        return true;
	}
	else
	{
        /***
         * We hava a reference to an existing template object, get it.
         */
        *templateObjectToRead = lookupLoadPool(obTag);
        return false;
   }

}

void XSerializeEngine::registerObject(void*  const templateObjectToRegister)
{
    ensureLoading();
    addLoadPool(templateObjectToRegister);
}

XMLGrammarPool* XSerializeEngine::getGrammarPool() const
{
    return fGrammarPool;
}

XMLStringPool* XSerializeEngine::getStringPool() const
{
    return fGrammarPool->getURIStringPool();
}

MemoryManager* XSerializeEngine::getMemoryManager() const
{
    //todo: changed to return fGrammarPool->getMemoryManager()
    return fGrammarPool ? fGrammarPool->getMemoryManager() : XMLPlatformUtils::fgMemoryManager;
}

XERCES_CPP_NAMESPACE_END

