/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
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

/**
 * $Log: OS400PlatformUtils.hpp,v $
 * Revision 1.3  2004/09/08 13:56:41  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.2  2002/11/04 15:13:01  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:25  peiyongz
 * sane_include
 *
 * Revision 1.2  2001/06/25 16:27:05  tng
 * AS400 changes by Linda Swan.
 *
 * Revision 1.1  2000/02/10 17:58:07  abagchi
 * Initial checkin
 *
 */

#ifndef _OS400PLATFORMUTILS_H
#define _OS400PLATFORMUTILS_H

XERCES_CPP_NAMESPACE_BEGIN

void  send_message (char * text, char * messageid, char type);
void  convert_errno(char *,int);

#define FILE_OPEN_PROBLEMS      "XMLBED3" /* file open failure           */
#define ICONV_CONVERT_PROBLEM	"XMLBED4" /* failed to convert ICONV     */
#define ICONV_CCSID_PROBLEM     "XMLBED5"
#define GENERAL_PANIC_MESSAGE   "XMLBEED" /* iconv initialization problem     */

XERCES_CPP_NAMESPACE_END

#endif /* _OS400PLATFORMUTILS_H */

