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
 * $Id: QNXPlatformUtils.cpp,v 1.10 2004/09/08 13:56:42 peiyongz Exp $
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <atomic.h>
#include <pthread.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <xercesc/util/Janitor.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/RuntimeException.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XMLUni.hpp>


//
//  These control which transcoding service is used by the QNX version.
//
#if defined (XML_USE_ICU_TRANSCODER)
    #include <xercesc/util/Transcoders/ICU/ICUTransService.hpp>
#elif defined (XML_USE_GNU_TRANSCODER)
    #include <xercesc/util/Transcoders/IconvGNU/IconvGNUTransService.hpp>
#else
    #error A transcoding service must be chosen
#endif

//
//  These control which message loading service is used by the QNX version.
//
#if defined(XML_USE_ICU_MESSAGELOADER)
    #include <xercesc/util/MsgLoaders/ICU/ICUMsgLoader.hpp>
#else
    #include <xercesc/util/MsgLoaders/InMemory/InMemMsgLoader.hpp>
#endif

//
//  These control which network access service is used by the QNX version.
//
#if defined (XML_USE_NETACCESSOR_LIBWWW)
    #include <xercesc/util/NetAccessors/libWWW/LibWWWNetAccessor.hpp>
#else
    #include <xercesc/util/NetAccessors/Socket/SocketNetAccessor.hpp>
#endif

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: The panic method
// ---------------------------------------------------------------------------
void XMLPlatformUtils::panic(const PanicHandler::PanicReasons reason)
{
    fgUserPanicHandler? fgUserPanicHandler->panic(reason) : fgDefaultPanicHandler->panic(reason);	
}



// ---------------------------------------------------------------------------
//  XMLPlatformUtils: File Methods
// ---------------------------------------------------------------------------

unsigned int XMLPlatformUtils::curFilePos(FileHandle theFile
                                          , MemoryManager* const manager)
{
    int curPos = ftell(theFile);
    if (curPos == -1) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotGetSize, manager);
    }
    return (unsigned int)curPos;
}

void XMLPlatformUtils::closeFile(FileHandle theFile
                                 , MemoryManager* const manager)
{
    if (fclose(theFile)) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotCloseFile, manager);
    }
}

unsigned int XMLPlatformUtils::fileSize(FileHandle theFile
                                        , MemoryManager* const manager)
{
    struct stat sbuf;
    if( fstat( fileno(theFile), &sbuf ) ) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotGetSize, manager);
    }
    return sbuf.st_size;
}

FileHandle XMLPlatformUtils::openFile(const char* const fileName
                                      , MemoryManager* const manager)
{
    return fopen( fileName, "rb" );
}

FileHandle XMLPlatformUtils::openFile(const XMLCh* const fileName
                                      , MemoryManager* const manager)
{
    const char* tmpFileName = XMLString::transcode(fileName, manager);
    ArrayJanitor<char> janText((char*)tmpFileName, manager);
    return openFile( tmpFileName );
}

FileHandle XMLPlatformUtils::openFileToWrite(const char* const fileName
                                             , MemoryManager* const manager)
{
    return fopen( fileName, "wb" );
}

FileHandle XMLPlatformUtils::openFileToWrite(const XMLCh* const fileName
                                             , MemoryManager* const manager)
{
    const char* tmpFileName = XMLString::transcode(fileName, manager);
    ArrayJanitor<char> janText((char*)tmpFileName, manager);
    return openFileToWrite(tmpFileName);
}

FileHandle XMLPlatformUtils::openStdInHandle(MemoryManager* const manager)
{
    return fdopen( dup(STDIN_FILENO), "rb" );
}


unsigned int
XMLPlatformUtils::readFileBuffer(       FileHandle      theFile
                                , const unsigned int    toRead
                                ,       XMLByte* const  toFill
                                , MemoryManager* const  manager)
{
    unsigned long bytesRead = 0;
    bytesRead = fread( toFill, 1, toRead, theFile );
    if (ferror(theFile)) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotReadFromFile, manager);
    }
    return (unsigned int)bytesRead;
}


void
XMLPlatformUtils::writeBufferToFile( FileHandle     const  theFile
                                   , long                  toWrite
                                   , const XMLByte* const  toFlush
                                   , MemoryManager* const  manager)
{
    unsigned long bytesWritten = 0;
    bytesWritten = fwrite( toFlush, 1, toWrite, theFile );
    if( bytesWritten != toWrite ) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotWriteToFile, manager);
    }
    return;
}

void XMLPlatformUtils::resetFile(FileHandle theFile
                                 , MemoryManager* const manager)
{
    if (fseek(theFile, 0, SEEK_SET)) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotResetFile, manager);
    }
}


// ---------------------------------------------------------------------------
//  XMLPlatformUtils: File system methods
// ---------------------------------------------------------------------------
XMLCh* XMLPlatformUtils::getFullPath(const XMLCh* const srcPath,
                                     MemoryManager* const manager)
{
    //
    //  NOTE: THe path provided has always already been opened successfully,
    //  so we know that its not some pathological freaky path. It comes in
    //  in native format, and goes out as Unicode always
    //
    char* newSrc = XMLString::transcode(srcPath, manager);
    ArrayJanitor<char> janText(newSrc, manager);
    char absPath[PATH_MAX + 1];

    char* retPath = realpath(newSrc, &absPath[0]);

    if (!retPath) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::File_CouldNotGetBasePathName, manager);
    }

    return XMLString::transcode(absPath, manager);
}


bool XMLPlatformUtils::isRelative(const XMLCh* const toCheck
                                  , MemoryManager* const manager)
{
    if (!toCheck[0] || toCheck[0] == XMLCh('/'))
        return false;
    return true;
}

XMLCh* XMLPlatformUtils::getCurrentDirectory(MemoryManager* const manager)
{
    char  dirBuf[PATH_MAX + 2];
    char  *curDir = getcwd(&dirBuf[0], PATH_MAX + 1);

    if (!curDir)
    {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException,
                 XMLExcepts::File_CouldNotGetBasePathName, manager);
    }

    return XMLString::transcode(curDir, manager);
}

inline bool XMLPlatformUtils::isAnySlash(XMLCh c) 
{
    return ( chBackSlash == c || chForwardSlash == c);
}

// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Timing Methods
// ---------------------------------------------------------------------------
unsigned long XMLPlatformUtils::getCurrentMillis()
{
    timeb aTime;
    ftime(&aTime);
    return (unsigned long)(aTime.time*1000 + aTime.millitm);
}



// ---------------------------------------------------------------------------
//  Mutex methods
// ---------------------------------------------------------------------------

void* XMLPlatformUtils::makeMutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init( &attr );
    pthread_mutexattr_setrecursive( &attr, PTHREAD_RECURSIVE_ENABLE );
    pthread_mutex_t *mutex = new pthread_mutex_t;
    if( pthread_mutex_init( mutex, &attr ) != EOK ) {
        delete mutex;
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::Mutex_CouldNotCreate, fgMemoryManager);
    }
    pthread_mutexattr_destroy( &attr );
    return mutex;
}

void XMLPlatformUtils::closeMutex(void* const mtxHandle)
{
    if( mtxHandle == NULL || pthread_mutex_destroy( (pthread_mutex_t *)mtxHandle ) != EOK ) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::Mutex_CouldNotClose, fgMemoryManager);
    }
    delete mtxHandle;
}


void XMLPlatformUtils::lockMutex(void* const mtxHandle)
{
    if( mtxHandle == NULL || pthread_mutex_lock( (pthread_mutex_t *)mtxHandle ) != EOK ) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::Mutex_CouldNotLock, fgMemoryManager);
    }
}


void XMLPlatformUtils::unlockMutex(void* const mtxHandle)
{
    if( mtxHandle == NULL || pthread_mutex_unlock( (pthread_mutex_t *)mtxHandle ) != EOK ) {
        ThrowXMLwithMemMgr(XMLPlatformUtilsException, XMLExcepts::Mutex_CouldNotLock, fgMemoryManager);
    }
}



// ---------------------------------------------------------------------------
//  Miscellaneous synchronization methods
// ---------------------------------------------------------------------------
void*
XMLPlatformUtils::compareAndSwap(       void**      toFill
                                , const void* const newValue
                                , const void* const toCompare)
{
    //
    // Undocumented function pulled in by pthread.h.  Uses CPU specific
    // inline assembly routines for doing an atomic compare and exchange
    // operation.
    //
    return (void *)_smp_cmpxchg( (volatile unsigned *)toFill, (unsigned)toCompare, (unsigned)newValue );
}


// ---------------------------------------------------------------------------
//  Atomic increment and decrement methods
// ---------------------------------------------------------------------------
int XMLPlatformUtils::atomicIncrement(int &location)
{
    atomic_add( &(volatile unsigned &)location, 1 );
    return location;
}


int XMLPlatformUtils::atomicDecrement(int &location)
{
    atomic_sub( &(volatile unsigned &)location, 1 );
    return location;
}



// ---------------------------------------------------------------------------
//  XMLPlatformUtils: Private Static Methods
// ---------------------------------------------------------------------------

//
//  This method is called by the platform independent part of this class
//  during initialization. We have to create the type of net accessor that
//  we want to use. If none, then just return zero.
//
XMLNetAccessor* XMLPlatformUtils::makeNetAccessor()
{
#if defined (XML_USE_NETACCESSOR_LIBWWW)
    return new LibWWWNetAccessor();
#else 
    return new SocketNetAccessor();
#endif
}


//
//  This method is called by the platform independent part of this class
//  when client code asks to have one of the supported message sets loaded.
//  In our case, we use the ICU based message loader mechanism.
//
XMLMsgLoader* XMLPlatformUtils::loadAMsgSet(const XMLCh* const msgDomain)
{
#if defined (XML_USE_ICU_MESSAGELOADER)
    return new ICUMsgLoader(msgDomain);
#else
    return new InMemMsgLoader(msgDomain);
#endif
}


//
//  This method is called very early in the bootstrapping process. This guy
//  must create a transcoding service and return it. It cannot use any string
//  methods, any transcoding services, throw any exceptions, etc... It just
//  makes a transcoding service and returns it, or returns zero on failure.
//
XMLTransService* XMLPlatformUtils::makeTransService()
{
#if defined (XML_USE_ICU_TRANSCODER)
    return new ICUTransService;
#elif defined (XML_USE_GNU_TRANSCODER)
    return new IconvGNUTransService;
#else
    #error You must provide a transcoding service implementation
#endif
}


void XMLPlatformUtils::platformInit()
{
}


void XMLPlatformUtils::platformTerm()
{
}

#include <xercesc/util/LogicalPath.c>

XERCES_CPP_NAMESPACE_END
