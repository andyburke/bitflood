#ifndef DOMDocumentTypeImpl_HEADER_GUARD_
#define DOMDocumentTypeImpl_HEADER_GUARD_

/*
 * Copyright 2001-2002,2004 The Apache Software Foundation.
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
 * $Id: DOMDocumentTypeImpl.hpp,v 1.17 2004/09/08 13:55:51 peiyongz Exp $
 */

//
//  This file is part of the internal implementation of the C++ XML DOM.
//  It should NOT be included or used directly by application programs.
//
//  Applications should include the file <xercesc/dom/DOM.hpp> for the entire
//  DOM API, or xercesc/dom/DOM*.hpp for individual DOM classes, where the class
//  name is substituded for the *.
//



#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include "DOMNodeImpl.hpp"
#include "DOMChildNode.hpp"
#include "DOMParentNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class DOMNamedNodeMapImpl;

class CDOM_EXPORT DOMDocumentTypeImpl: public DOMDocumentType {
private:
    DOMNodeImpl          fNode;
    DOMParentNode        fParent;
    DOMChildNode         fChild;

    const XMLCh *        fName;
    DOMNamedNodeMapImpl* fEntities;
    DOMNamedNodeMapImpl* fNotations;
    DOMNamedNodeMapImpl* fElements;
    const XMLCh *        fPublicId;
    const XMLCh *        fSystemId;
    const XMLCh *        fInternalSubset;

    bool			     fIntSubsetReading;
    bool                 fIsCreatedFromHeap;

    virtual void         setPublicId(const XMLCh * value);
    virtual void         setSystemId(const XMLCh * value);
    virtual void         setInternalSubset(const XMLCh *value);
    bool                 isIntSubsetReading() const;

    friend class AbstractDOMParser;
    friend class DOMDocumentImpl;

public:
    DOMDocumentTypeImpl(DOMDocument *, const XMLCh *, bool);
    DOMDocumentTypeImpl(DOMDocument *,
                     const XMLCh *qualifiedName,	//DOM Level 2
                     const XMLCh *publicId, const XMLCh *systemId, bool);
    DOMDocumentTypeImpl(const DOMDocumentTypeImpl &other, bool heap, bool deep=false);
    virtual ~DOMDocumentTypeImpl();

    // Declare all of the functions from DOMNode.
    DOMNODE_FUNCTIONS;

    virtual void setOwnerDocument(DOMDocument *doc);
    virtual DOMNamedNodeMap * getEntities() const;
    virtual const XMLCh *       getName() const;
    virtual DOMNamedNodeMap * getNotations() const;
    virtual DOMNamedNodeMap * getElements() const;
    virtual void                setReadOnly(bool readOnly, bool deep);

    //Introduced in DOM Level 2

    virtual const XMLCh *     getPublicId() const;
    virtual const XMLCh *     getSystemId() const;
    virtual const XMLCh *     getInternalSubset() const;

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    DOMDocumentTypeImpl & operator = (const DOMDocumentTypeImpl &);
};

XERCES_CPP_NAMESPACE_END

#endif
