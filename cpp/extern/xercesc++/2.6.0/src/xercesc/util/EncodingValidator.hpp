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
 * $Id: EncodingValidator.hpp,v 1.4 2004/09/08 13:56:21 peiyongz Exp $
 */

#if !defined(ENCODINGVALIDATOR_HPP)
#define ENCODINGVALIDATOR_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/ValueHashTableOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * A singleton class that checks whether an encoding name is a valid IANA
  * encoding
  */

class XMLUTIL_EXPORT EncodingValidator {

public:
    // -----------------------------------------------------------------------
    //  Validation methods
    // -----------------------------------------------------------------------
    bool isValidEncoding(const XMLCh* const encName);

    // -----------------------------------------------------------------------
    //  Instance methods
    // -----------------------------------------------------------------------
	static EncodingValidator* instance();

	// -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitInstance();

private:
    // -----------------------------------------------------------------------
    //  Constructor and destructors
    // -----------------------------------------------------------------------
    EncodingValidator();
    ~EncodingValidator();

    // -----------------------------------------------------------------------
    //  Private Helpers methods
    // -----------------------------------------------------------------------
    /*
     *  Initializes the registry with a set of valid IANA encoding names
     */
     void initializeRegistry();

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fEncodingRegistry
    //      Contains a set of IANA encoding names
	//
    //  fInstance
    //      An EncodingValidator singleton instance
    // -----------------------------------------------------------------------
    ValueHashTableOf<bool>*   fEncodingRegistry;
    static EncodingValidator* fInstance;
};

XERCES_CPP_NAMESPACE_END

#endif

/**
  *	End file EncodingValidator.hpp
  */
