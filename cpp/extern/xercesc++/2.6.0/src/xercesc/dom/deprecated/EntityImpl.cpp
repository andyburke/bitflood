/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: EntityImpl.cpp,v 1.5 2004/09/08 13:55:43 peiyongz Exp $
 */

#include "DOM_DOMException.hpp"
#include "DOM_Node.hpp"
#include "EntityImpl.hpp"
#include "DocumentImpl.hpp"

XERCES_CPP_NAMESPACE_BEGIN


EntityImpl::EntityImpl(DocumentImpl *ownerDoc, const DOMString &eName)
   : ParentNode(ownerDoc),
	refEntity(0)

{
    name        = eName.clone();
    setReadOnly(true, true);
};


EntityImpl::EntityImpl(const EntityImpl &other, bool deep)
    : ParentNode(other)
{
    name            = other.name.clone();
    if (deep)
        cloneChildren(other);
    publicId        = other.publicId.clone();
    systemId        = other.systemId.clone();
    notationName    = other.notationName.clone();

    RefCountedImpl::removeRef(refEntity);
    refEntity       = other.refEntity;	
    RefCountedImpl::addRef(other.refEntity);

    setReadOnly(true, true);
};


EntityImpl::~EntityImpl() {
};


NodeImpl *EntityImpl::cloneNode(bool deep)
{
    return new (getOwnerDocument()->getMemoryManager()) EntityImpl(*this, deep);
};


DOMString EntityImpl::getNodeName() {
    return name;
};


short EntityImpl::getNodeType() {
    return DOM_Node::ENTITY_NODE;
};


DOMString EntityImpl::getNotationName()
{
    return notationName;
};


DOMString EntityImpl::getPublicId() {
    return publicId;
};


DOMString EntityImpl::getSystemId()
{
    return systemId;
};


void EntityImpl::setNotationName(const DOMString &arg)
{
    notationName = arg;
};


void EntityImpl::setPublicId(const DOMString &arg)
{
    publicId = arg;
};


void EntityImpl::setSystemId(const DOMString &arg)
{
    systemId = arg;
};

void		EntityImpl::setEntityRef(EntityReferenceImpl* other)
{
   RefCountedImpl::removeRef(refEntity);
	refEntity = other;
   RefCountedImpl::addRef(other);
}

EntityReferenceImpl*		EntityImpl::getEntityRef() const
{
	return refEntity;
}

void	EntityImpl::cloneEntityRefTree()
{
	//lazily clone the entityRef tree to this entity
	if (firstChild != 0)
		return;

	if (!refEntity)
		return;

   setReadOnly(false, true);
	this->cloneChildren(*refEntity);
   setReadOnly(true, true);
}

NodeImpl * EntityImpl::getFirstChild()
{
    cloneEntityRefTree();
	return firstChild;
};

NodeImpl*   EntityImpl::getLastChild()
{
	cloneEntityRefTree();
	return lastChild();
}

NodeListImpl* EntityImpl::getChildNodes()
{
	cloneEntityRefTree();
	return this;

}

bool EntityImpl::hasChildNodes()
{
	cloneEntityRefTree();
	return firstChild!=null;
}

NodeImpl* EntityImpl::item(unsigned int index)
{
	cloneEntityRefTree();
    ChildNode *node = firstChild;
    for(unsigned int i=0; i<index && node!=null; ++i)
        node = node->nextSibling;
    return node;
}

XERCES_CPP_NAMESPACE_END

