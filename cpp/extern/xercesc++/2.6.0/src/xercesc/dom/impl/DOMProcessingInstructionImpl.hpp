#ifndef DOMProcessingInstructionImpl_HEADER_GUARD_
#define DOMProcessingInstructionImpl_HEADER_GUARD_
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
 * $Id: DOMProcessingInstructionImpl.hpp,v 1.8 2004/09/08 13:55:52 peiyongz Exp $
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
#include <xercesc/dom/DOMProcessingInstruction.hpp>
#include "DOMCharacterDataImpl.hpp"
#include "DOMNodeImpl.hpp"
#include "DOMChildNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class    DocumentImpl;


class CDOM_EXPORT DOMProcessingInstructionImpl: public DOMProcessingInstruction {
private:
    DOMNodeImpl   fNode;
    DOMChildNode  fChild;
    // use fCharacterData to store its data so that those character utitlites can be used
    DOMCharacterDataImpl   fCharacterData;

    XMLCh       *fTarget;
    const XMLCh *fBaseURI;

public:
    DOMProcessingInstructionImpl(DOMDocument *ownerDoc,
                              const XMLCh * target,
                              const XMLCh *data);
    DOMProcessingInstructionImpl(const DOMProcessingInstructionImpl &other,
                              bool deep=false);
    virtual ~DOMProcessingInstructionImpl();

    // Declare all of the functions from DOMNode.
    DOMNODE_FUNCTIONS;

    virtual const XMLCh *getData() const;
    virtual const XMLCh *getTarget() const;
    virtual void setData(const XMLCh *arg);

    // NON-DOM: set base uri
    virtual void setBaseURI(const XMLCh* baseURI);

    // Non standard extension for the range to work
    void         deleteData(XMLSize_t offset, XMLSize_t count);
    const XMLCh* substringData(XMLSize_t offset, XMLSize_t count) const;
    DOMProcessingInstruction* splitText(XMLSize_t offset);

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DOMProcessingInstructionImpl & operator = (const DOMProcessingInstructionImpl &);
};

XERCES_CPP_NAMESPACE_END

#endif

