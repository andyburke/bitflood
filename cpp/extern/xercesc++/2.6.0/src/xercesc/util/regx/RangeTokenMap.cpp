/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Log: RangeTokenMap.cpp,v $
 * Revision 1.11  2004/09/08 13:56:47  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.10  2004/07/22 15:37:18  knoaman
 * Use file static instance instead of local static instance
 *
 * Revision 1.9  2004/01/13 16:17:10  knoaman
 * Fo sanity, use class name to qualify method
 *
 * Revision 1.8  2004/01/09 22:41:58  knoaman
 * Use a global static mutex for locking when creating local static mutexes instead of compareAndSwap
 *
 * Revision 1.7  2003/12/17 00:18:37  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.6  2003/10/17 16:44:34  knoaman
 * Fix multithreading problem.
 *
 * Revision 1.5  2003/05/18 14:02:06  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.4  2003/05/15 19:10:23  knoaman
 * Add missing include.
 *
 * Revision 1.3  2003/03/04 21:11:12  knoaman
 * [Bug 17516] Thread safety problems in ../util/ and ../util/regx.
 *
 * Revision 1.2  2002/11/04 15:17:00  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:30  peiyongz
 * sane_include
 *
 * Revision 1.5  2001/10/25 15:06:26  tng
 * Thread safe the static instance.
 *
 * Revision 1.4  2001/10/23 23:13:41  peiyongz
 * [Bug#880] patch to PlatformUtils:init()/term() and related. from Mark Weaver
 *
 * Revision 1.3  2001/07/16 21:28:25  knoaman
 * fix bug - no delete for the static instance in destructor.
 *
 * Revision 1.2  2001/05/11 13:26:45  tng
 * Copyright update.
 *
 * Revision 1.1  2001/05/03 18:17:40  knoaman
 * Some design changes:
 * o Changed the TokenFactory from a single static instance, to a
 *    normal class. Each RegularExpression object will have its own
 *    instance of TokenFactory, and that instance will be passed to
 *    other classes that need to use a TokenFactory to create Token
 *    objects (with the exception of RangeTokenMap).
 * o Added a new class RangeTokenMap to map a the different ranges
 *    in a given category to a specific RangeFactory object. In the old
 *    design RangeFactory had dual functionality (act as a Map, and as
 *    a factory for creating RangeToken(s)). The RangeTokenMap will
 *    have its own copy of the TokenFactory. There will be only one
 *    instance of the RangeTokenMap class, and that instance will be
 *    lazily deleted when XPlatformUtils::Terminate is called.
 *
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/regx/RangeTokenMap.hpp>
#include <xercesc/util/regx/RangeToken.hpp>
#include <xercesc/util/regx/RegxDefs.hpp>
#include <xercesc/util/regx/TokenFactory.hpp>
#include <xercesc/util/regx/RangeFactory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/XMLRegisterCleanup.hpp>
#include <xercesc/util/StringPool.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Local static data
// ---------------------------------------------------------------------------
static XMLMutex* sRangeTokMapMutex = 0;
static XMLRegisterCleanup rangeTokMapRegistryCleanup;
static XMLRegisterCleanup rangeTokMapInstanceCleanup;

// ---------------------------------------------------------------------------
//  Local, static functions
// ---------------------------------------------------------------------------
static void reinitRangeTokMapMutex()
{
    delete sRangeTokMapMutex;
    sRangeTokMapMutex = 0;
}

static XMLMutex& getRangeTokMapMutex()
{
    if (!sRangeTokMapMutex)
    {
        XMLMutexLock lock(XMLPlatformUtils::fgAtomicMutex);

        // If we got here first, then register it and set the registered flag
        if (!sRangeTokMapMutex)
        {
            sRangeTokMapMutex = new XMLMutex;
            rangeTokMapRegistryCleanup.registerCleanup(reinitRangeTokMapMutex);
        }
    }
    return *sRangeTokMapMutex;
}

// ---------------------------------------------------------------------------
//  Static member data initialization
// ---------------------------------------------------------------------------
RangeTokenMap* RangeTokenMap::fInstance = 0;


// ---------------------------------------------------------------------------
//  RangeTokenElemMap: Constructors and Destructor
// ---------------------------------------------------------------------------
RangeTokenElemMap::RangeTokenElemMap(unsigned int categoryId) :
    fCategoryId(categoryId)
    , fRange(0)
    , fNRange(0)
{

}

RangeTokenElemMap::~RangeTokenElemMap()
{

}

// ---------------------------------------------------------------------------
//  RangeTokenMap: Constructors and Destructor
// ---------------------------------------------------------------------------
RangeTokenMap::RangeTokenMap() :
    fRegistryInitialized(false)
    , fTokenRegistry(0)
    , fRangeMap(0)
    , fCategories(0)
    , fTokenFactory(0) {

}

RangeTokenMap::~RangeTokenMap() {

    delete fTokenRegistry;
    fTokenRegistry = 0;

    delete fRangeMap;
    fRangeMap = 0;

    delete fCategories;
    fCategories = 0;
    delete fTokenFactory;
    fTokenFactory = 0;
}

// ---------------------------------------------------------------------------
//  RangeTokenMap: Getter methods
// ---------------------------------------------------------------------------
RangeToken* RangeTokenMap::getRange(const XMLCh* const keyword,
								    const bool complement) {

    if (fTokenRegistry == 0 || fRangeMap == 0 || fCategories == 0)
        return 0;

    if (!fTokenRegistry->containsKey(keyword))
        return 0;

    RangeTokenElemMap* elemMap = fTokenRegistry->get(keyword);
    RangeToken* rangeTok = elemMap->getRangeToken(complement);

    if (!rangeTok)
    {
        XMLMutexLock lockInit(&fMutex);

        // make sure that it was not created while we were locked
        rangeTok = elemMap->getRangeToken(complement);

        if (!rangeTok)
        {
            rangeTok = elemMap->getRangeToken();
            if (!rangeTok)
            {
                unsigned int categId = elemMap->getCategoryId();
                const XMLCh* categName = fCategories->getValueForId(categId);
                RangeFactory* rangeFactory = fRangeMap->get(categName);

                if (rangeFactory == 0)
                    return 0;

                rangeFactory->buildRanges();
                rangeTok = elemMap->getRangeToken();
            }

            if (complement)
            {
                rangeTok = (RangeToken*) RangeToken::complementRanges(rangeTok, fTokenFactory, fTokenRegistry->getMemoryManager());
                elemMap->setRangeToken(rangeTok , complement);
            }
        }
    }

    return rangeTok;
}


// ---------------------------------------------------------------------------
//  RangeTokenMap: Putter methods
// ---------------------------------------------------------------------------
void RangeTokenMap::addCategory(const XMLCh* const categoryName) {

    if (fCategories)
	    fCategories->addOrFind(categoryName);
}

void RangeTokenMap::addRangeMap(const XMLCh* const categoryName,
                                RangeFactory* const rangeFactory) {

    if (fRangeMap)
	    fRangeMap->put((void*)categoryName, rangeFactory);
}

void RangeTokenMap::addKeywordMap(const XMLCh* const keyword,
                                 const XMLCh* const categoryName) {

    if (fCategories == 0 || fTokenRegistry == 0)
        return;

	unsigned int categId = fCategories->getId(categoryName);

	if (categId == 0) {
		ThrowXMLwithMemMgr1(RuntimeException, XMLExcepts::Regex_InvalidCategoryName, categoryName, fTokenRegistry->getMemoryManager());
	}

    if (fTokenRegistry->containsKey(keyword)) {

        RangeTokenElemMap* elemMap = fTokenRegistry->get(keyword);

		if (elemMap->getCategoryId() != categId)
			elemMap->setCategoryId(categId);

		return;
	}

	fTokenRegistry->put((void*) keyword, new RangeTokenElemMap(categId));
}

// ---------------------------------------------------------------------------
//  RangeTokenMap: Setter methods
// ---------------------------------------------------------------------------
void RangeTokenMap::setRangeToken(const XMLCh* const keyword,
                                  RangeToken* const tok,const bool complement) {

    if (fTokenRegistry == 0)
		return;

	if (fTokenRegistry->containsKey(keyword)) {
        fTokenRegistry->get(keyword)->setRangeToken(tok, complement);
    }
    else {
		ThrowXMLwithMemMgr1(RuntimeException, XMLExcepts::Regex_KeywordNotFound, keyword, fTokenRegistry->getMemoryManager());
	}
}


// ---------------------------------------------------------------------------
//  RangeTokenMap: Initialization methods
// ---------------------------------------------------------------------------
void RangeTokenMap::initializeRegistry() {

    if (!fRegistryInitialized)
    {
        XMLMutexLock lockInit(&fMutex);

        if (!fRegistryInitialized)
        {
            fTokenFactory = new TokenFactory();
            fTokenRegistry = new RefHashTableOf<RangeTokenElemMap>(109);
            fRangeMap = new RefHashTableOf<RangeFactory>(29);
            fCategories = new XMLStringPool(109);
            fRegistryInitialized = true;
        }
    }
}


// ---------------------------------------------------------------------------
//  RangeTokenMap: Instance methods
// ---------------------------------------------------------------------------
RangeTokenMap* RangeTokenMap::instance()
{
    if (!fInstance)
    {
        XMLMutexLock lock(&getRangeTokMapMutex());

        if (!fInstance)
        {
            fInstance = new RangeTokenMap();
            rangeTokMapInstanceCleanup.registerCleanup(RangeTokenMap::reinitInstance);
        }
    }

    return (fInstance);
}

// -----------------------------------------------------------------------
//  Notification that lazy data has been deleted
// -----------------------------------------------------------------------
void RangeTokenMap::reinitInstance() {

    delete fInstance;
    fInstance = 0;
    TokenFactory::fRangeInitialized = false;
}

XERCES_CPP_NAMESPACE_END

/**
  * End of file RangeTokenMap.cpp
  */
