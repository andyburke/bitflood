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
 * $Log: iconv_util.hpp,v $
 * Revision 1.3  2004/09/08 13:56:45  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.2  2002/11/04 15:14:33  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:36  peiyongz
 * sane_include
 *
 * Revision 1.1  2001/06/25 16:20:08  tng
 * Rename iconv_util.h to iconv_util.hpp.  AS400 changes by Linda Swan.
 *
 * Revision 1.1  2000/02/10 18:08:28  abagchi
 * Initial checkin
 *
 */


const char* iconv_getDefaultCodepage(void);


#include "utypes.h"
#include <iconv.h>

extern "C" int QlgCvtTextDescToDesc (int, int, char *, int, char*, int, int);

#define MAX_CONVERTER_NAME_LENGTH 60
#define MAX_SUBCHAR_LEN 4
#define ERROR_BUFFER_LENGTH 20
typedef enum {UNKNOWN = -1, IBM = 0}
UCNV_PLATFORM;
typedef enum
{
  UNSUPPORTED_CONVERTER = -1,
  SBCS = 0,
  DBCS = 1,
  MBCS = 2,
  LATIN_1 = 3,
  UTF8 = 4,
  UTF16_BigEndian = 5,
  UTF16_LittleEndian = 6,
  EBCDIC_STATEFUL = 7,
  ISO_2022 = 8,
  JIS = 9,
  EUC = 10,
  GB = 11
}
UCNV_TYPE;
int32_t
u_strlen(const UChar *s);

/* note sure if needed -  think that this is needed in cnv.c */
char* u_austrcpy(char *s1,
         const UChar *ucs2 );

/*Defines the struct of a UConverterSharedData the immutable, shared part of
 *UConverter
 */
typedef struct
  {
    uint32_t referenceCounter;	/*used to count number of clients */
    char name[MAX_CONVERTER_NAME_LENGTH];	/*internal name of the converter */
    UCNV_PLATFORM platform;	/*platform of the converter (only IBM now */
    int32_t codepage;		/*codepage # (now IBM-$codepage) */
    UCNV_TYPE conversionType;	/*conversion type */
    int8_t minBytesPerChar;	/*Minimum # bytes per char in this codepage */
    int8_t maxBytesPerChar;	/*Maximum # bytes per char in this codepage */

    struct
      {				/*initial values of some members of the mutable part of object */

	int8_t subCharLen;
	unsigned char subChar[MAX_SUBCHAR_LEN];
      } defaultConverterValues ;

    iconv_t toiconv_handle ;     /* handle to convert to unicode*/
    iconv_t fromiconv_handle;   /* handle to convert from unicode*/
  }
UConverterSharedData;


/*Defines a UConverter, the lightweight mutable part the user sees */
struct UConverter
  {



    int8_t pad;
    int32_t mode;
    int8_t subCharLen;		/*length of the codepage specific character sequence */
    unsigned char subChar[MAX_SUBCHAR_LEN];	/*codepage specific character sequence */


    UConverterSharedData *sharedData;	/*Pointer to the shared immutable part of the
					 *converter object
					 */


  };

typedef struct UConverter UConverter;

UConverter* createNewConverter(const char *name, UErrorCode *err);

/*Initializes the mutable lightweight portion of the object
 *By copying data from UConverter->sharedData->defaultConverter
 */
static void   initializeDataConverter (UConverter * myUConverter);
UConverter *createConverter (const char *converterName, UErrorCode * err);

XERCES_CPP_NAMESPACE_END
