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

/**
 * $Log: pos400.h,v $
 * Revision 1.2  2004/09/08 13:56:45  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.1.1.1  2002/02/01 22:22:36  peiyongz
 * sane_include
 *
 * Revision 1.1  2000/02/10 18:08:28  abagchi
 * Initial checkin
 *
 */

/* Define the platform we're on. */
#ifndef OS400
#define OS400
#endif

/* Define whether inttypes.h is available */
#define HAVE_INTTYPES_H 0

/* Determines whether specific types are available */
#define HAVE_INT8_T 0
#define HAVE_UINT8_T 0
#define HAVE_INT16_T 0
#define HAVE_UINT16_T 0
#define HAVE_INT32_T 0
#define HAVE_UINT32_T 0
#define HAVE_BOOL_T 0

/* Determines the endianness of the platform */
#define U_IS_BIG_ENDIAN 1

/*===========================================================================*/
/* Generic data types                                                        */
/*===========================================================================*/

/* If your platform does not have the <inttypes.h> header, you may
   need to edit the typedefs below. */
#if HAVE_INTTYPES_H
#include <inttypes.h>
#else

#if ! HAVE_INT8_T
typedef signed char int8_t;
#endif

#if ! HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif

#if ! HAVE_INT16_T
typedef signed short int16_t;
#endif

#if ! HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif

#if ! HAVE_INT32_T
typedef signed long int32_t;
#endif

#if ! HAVE_UINT32_T
typedef unsigned long uint32_t;
#endif

#endif

#include <limits.h>
#define T_INT32_MAX (LONG_MAX)

/*===========================================================================*/
/* Symbol import-export control                                              */
/*===========================================================================*/

#define U_EXPORT
#define U_EXPORT2
#define U_IMPORT
