/*
 * Copyright 2003,2004 The Apache Software Foundation.
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
 * $Id: DefaultPanicHandler.cpp,v 1.2 2004/09/08 13:56:21 peiyongz Exp $
 * $Log: DefaultPanicHandler.cpp,v $
 * Revision 1.2  2004/09/08 13:56:21  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.1  2003/03/09 17:06:16  peiyongz
 * PanicHandler
 *
 *
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/DefaultPanicHandler.hpp>

#include <stdio.h>
#include <stdlib.h>

XERCES_CPP_NAMESPACE_BEGIN

void DefaultPanicHandler::panic(const PanicHandler::PanicReasons reason)
{
    fprintf(stderr, "%s\n", PanicHandler::getPanicReasonString(reason));
    exit(-1);
}

XERCES_CPP_NAMESPACE_END

