/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Log: CSetDefs.cpp,v $
 * Revision 1.3  2004/09/08 13:56:32  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.2  2002/04/02 20:17:47  peiyongz
 * Bug# 7555: Enable AIX build with newer xlC versions,
 * patch from Martin Kalen (martin.kalen@todaysystems.com.au )
 *
 * Revision 1.1.1.1  2002/02/01 22:22:18  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/03/02 19:55:07  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.2  2000/02/06 07:48:17  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:07:30  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:22  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#if defined(XML_IBMVAW32) || defined(XML_IBMVAOS2)

#include <string.h>

#else

#include <xercesc/util/Compilers/CSetDefs.hpp>
#include <strings.h>

int stricmp(const char* const str1, const char* const str2) 
{
	return strcasecmp(str1, str2);
}

int strnicmp(const char* const str1, const char* const str2, const unsigned int count)
{
	if (count == 0)
		return 0;

	return strncasecmp( str1, str2, (size_t)count);
}
#endif
