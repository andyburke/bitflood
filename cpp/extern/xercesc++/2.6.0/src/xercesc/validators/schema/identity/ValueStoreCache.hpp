/*
 * Copyright 2001-2004 The Apache Software Foundation.
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
 * $Id: ValueStoreCache.hpp,v 1.7 2004/09/08 13:56:59 peiyongz Exp $
 */

#if !defined(VALUESTORECACHE_HPP)
#define VALUESTORECACHE_HPP

/**
  * This class is used to store the values for identity constraints.
  *
  * Sketch of algorithm:
  *  - When a constraint is first encountered, its values are stored in the
  *    (local) fIC2ValueStoreMap;
  *  - Once it is validated (i.e., wen it goes out of scope), its values are
  *    merged into the fGlobalICMap;
  *  - As we encounter keyref's, we look at the global table to validate them.
  *  - Validation always occurs against the fGlobalIDConstraintMap (which
  *    comprises all the "eligible" id constraints). When an endelement is
  *    found, this Hashtable is merged with the one below in the stack. When a
  *    start tag is encountered, we create a new fGlobalICMap.
  *    i.e., the top of the fGlobalIDMapStack always contains the preceding
  *    siblings' eligible id constraints; the fGlobalICMap contains
  *    descendants+self. Keyrefs can only match descendants+self.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/RefHash2KeysTableOf.hpp>
#include <xercesc/util/RefStackOf.hpp>
#include <xercesc/validators/schema/identity/IdentityConstraint.hpp>
#include <xercesc/validators/schema/identity/IC_Field.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declcaration
// ---------------------------------------------------------------------------
class ValueStore;
class SchemaElementDecl;
class XMLScanner;


class VALIDATORS_EXPORT ValueStoreCache : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    ValueStoreCache(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ValueStoreCache();

	// -----------------------------------------------------------------------
    //  Setter Methods
    // -----------------------------------------------------------------------
    void setScanner(XMLScanner* const scanner);

	// -----------------------------------------------------------------------
    //  Document Handling methods
    // -----------------------------------------------------------------------
    void startDocument();
    void startElement();
    void endElement();
    void endDocument();

	// -----------------------------------------------------------------------
    //  Initialization methods
    // -----------------------------------------------------------------------
    void initValueStoresFor(SchemaElementDecl* const elemDecl, const int initialDepth);


	// -----------------------------------------------------------------------
    //  Access methods
    // -----------------------------------------------------------------------
    ValueStore* getValueStoreFor(const IC_Field* const field, const int initialDepth);
    ValueStore* getValueStoreFor(const IdentityConstraint* const ic, const int intialDepth);
    ValueStore* getGlobalValueStoreFor(const IdentityConstraint* const ic);

	// -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    /** This method takes the contents of the (local) ValueStore associated
      * with ic and moves them into the global hashtable, if ic is a <unique>
      * or a <key>. If it's a <keyRef>, then we leave it for later.
      */
    void transplant(IdentityConstraint* const ic, const int initialDepth);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    ValueStoreCache(const ValueStoreCache& other);
    ValueStoreCache& operator= (const ValueStoreCache& other);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void init();
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Data
    // -----------------------------------------------------------------------
    RefVectorOf<ValueStore>*                 fValueStores;
    RefHashTableOf<ValueStore>*              fGlobalICMap;
    RefHash2KeysTableOf<ValueStore>*         fIC2ValueStoreMap;
    RefStackOf<RefHashTableOf<ValueStore> >* fGlobalMapStack;
    XMLScanner*                              fScanner;
    MemoryManager*                           fMemoryManager;
};

// ---------------------------------------------------------------------------
//  ValueStoreCache: Access methods
// ---------------------------------------------------------------------------
inline void ValueStoreCache::setScanner(XMLScanner* const scanner) {

    fScanner = scanner;
}

// ---------------------------------------------------------------------------
//  ValueStoreCache: Access methods
// ---------------------------------------------------------------------------
inline ValueStore*
ValueStoreCache::getValueStoreFor(const IC_Field* const field, const int initialDepth) {

    return fIC2ValueStoreMap->get(field->getIdentityConstraint(), initialDepth);
}

inline ValueStore*
ValueStoreCache::getValueStoreFor(const IdentityConstraint* const ic, const int initialDepth) {

    return fIC2ValueStoreMap->get(ic, initialDepth);
}

inline ValueStore*
ValueStoreCache::getGlobalValueStoreFor(const IdentityConstraint* const ic) {

    return fGlobalICMap->get(ic);
}

// ---------------------------------------------------------------------------
//  ValueStoreCache: Document handling methods
// ---------------------------------------------------------------------------
inline void ValueStoreCache::endDocument() {
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ValueStoreCache.hpp
  */

