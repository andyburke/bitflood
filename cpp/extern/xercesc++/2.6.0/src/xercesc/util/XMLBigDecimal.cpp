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
 * $Log: XMLBigDecimal.cpp,v $
 * Revision 1.24  2004/09/08 13:56:24  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.23  2004/08/11 16:17:58  peiyongz
 * Light weight parsing method
 *
 * Revision 1.22  2004/03/19 01:15:55  peiyongz
 * store/load fRawData
 *
 * Revision 1.21  2004/01/13 19:50:56  peiyongz
 * remove parseContent()
 *
 * Revision 1.18  2003/12/23 21:48:14  peiyongz
 * Absorb exception thrown in getCanonicalRepresentation and return 0
 *
 * Revision 1.17  2003/12/17 20:42:16  neilg
 * fix two overflow conditions
 *
 * Revision 1.16  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.15  2003/12/11 21:38:12  peiyongz
 * support for Canonical Representation for Datatype
 *
 * Revision 1.14  2003/10/01 16:32:39  neilg
 * improve handling of out of memory conditions, bug #23415.  Thanks to David Cargill.
 *
 * Revision 1.13  2003/09/25 22:24:28  peiyongz
 * Using writeString/readString
 *
 * Revision 1.12  2003/09/25 15:23:25  peiyongz
 * add sizeof(XMLCh) when allocating memory
 *
 * Revision 1.11  2003/09/23 18:16:07  peiyongz
 * Inplementation for Serialization/Deserialization
 *
 * Revision 1.10  2003/08/14 02:57:27  knoaman
 * Code refactoring to improve performance of validation.
 *
 * Revision 1.9  2003/05/16 06:01:53  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.8  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.7  2003/04/29 18:13:36  peiyongz
 * cut link to XMLBigInteger, patch from Khaled Noaman
 *
 * Revision 1.6  2003/02/25 17:24:18  peiyongz
 * Schema Errata: E2-44 totalDigits/fractDigits
 *
 * Revision 1.5  2003/02/02 23:54:43  peiyongz
 * getFormattedString() added to return original and converted value.
 *
 * Revision 1.4  2003/01/30 21:55:22  tng
 * Performance: create getRawData which is similar to toString but return the internal data directly, user is not required to delete the returned memory.
 *
 * Revision 1.3  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/08/13 22:11:23  peiyongz
 * Fix to Bug#9442
 *
 * Revision 1.1.1.1  2002/02/01 22:22:14  peiyongz
 * sane_include
 *
 * Revision 1.8  2001/08/08 18:33:44  peiyongz
 * fix: unresolved symbol warning for 'pow'.
 *
 * Revision 1.7  2001/07/25 19:07:42  peiyongz
 * Fix to AIX compilation error: The function abs must have a prototype.
 *
 * Revision 1.6  2001/07/24 13:58:11  peiyongz
 * XMLDouble and related supporting methods from XMLBigInteger/XMLBigDecimal
 *
 * Revision 1.5  2001/06/07 20:55:21  tng
 * Fix no newline at the end warning.  By Pei Yong Zhang.
 *
 * Revision 1.4  2001/05/18 20:17:55  tng
 * Schema: More exception messages in XMLBigDecimal/XMLBigInteger/DecimalDatatypeValidator.  By Pei Yong Zhang.
 *
 * Revision 1.3  2001/05/18 13:22:54  tng
 * Schema: Exception messages in DatatypeValidator.  By Pei Yong Zhang.
 *
 * Revision 1.2  2001/05/11 13:26:30  tng
 * Copyright update.
 *
 * Revision 1.1  2001/05/10 20:51:20  tng
 * Schema: Add DecimalDatatypeValidator and XMLBigDecimal, XMLBigInteger.  By Pei Yong Zhang.
 *
 */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XMLBigDecimal.hpp>
#include <xercesc/util/XMLBigInteger.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/util/NumberFormatException.hpp>
#include <xercesc/util/XMLChar.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/Janitor.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * Constructs a BigDecimal from a string containing an optional (plus | minus)
 * sign followed by a sequence of zero or more decimal digits, optionally
 * followed by a fraction, which consists of a decimal point followed by
 * zero or more decimal digits.  The string must contain at least one
 * digit in the integer or fractional part.  The scale of the resulting
 * BigDecimal will be the number of digits to the right of the decimal
 * point in the string, or 0 if the string contains no decimal point.
 * Any extraneous characters (including whitespace) will result in
 * a NumberFormatException.

 * since parseBigDecimal()  may throw exception,
 * caller of XMLBigDecimal need to catch it.
//
**/

XMLBigDecimal::XMLBigDecimal(const XMLCh* const strValue,
                             MemoryManager* const manager)
: fSign(0)
, fTotalDigits(0)
, fScale(0)
, fRawDataLen(0)
, fRawData(0)
, fIntVal(0)
, fMemoryManager(manager)
{
    if ((!strValue) || (!*strValue))
        ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_emptyString, fMemoryManager);

    try
    {
        fRawDataLen = XMLString::stringLen(strValue);
        fRawData = (XMLCh*) fMemoryManager->allocate
        (
            ((fRawDataLen*2) + 2) * sizeof(XMLCh) //fRawData and fIntVal
        );
        memcpy(fRawData, strValue, fRawDataLen * sizeof(XMLCh));
        fRawData[fRawDataLen] = chNull;
        fIntVal = fRawData + fRawDataLen + 1;
        parseDecimal(strValue, fIntVal, fSign, (int&) fTotalDigits, (int&) fScale, fMemoryManager);
    }
    catch(const OutOfMemoryException&)
    {
        throw;
    }
    catch(...)
    {
        cleanUp();
        throw;
    }
}

XMLBigDecimal::~XMLBigDecimal()
{
    cleanUp();
}

void XMLBigDecimal::cleanUp()
{
    if (fRawData)
        fMemoryManager->deallocate(fRawData); //XMLString::release(&fRawData);
}

void XMLBigDecimal::setDecimalValue(const XMLCh* const strValue)
{
    fScale = fTotalDigits = 0;
    unsigned int valueLen = XMLString::stringLen(strValue);

    if (valueLen > fRawDataLen)
    {
        fMemoryManager->deallocate(fRawData);
        fRawData = (XMLCh*) fMemoryManager->allocate
        (
            ((valueLen * 2) + 4) * sizeof(XMLCh)
        );//XMLString::replicate(strValue, fMemoryManager);
    }

    memcpy(fRawData, strValue, valueLen * sizeof(XMLCh));
    fRawData[valueLen] = chNull;
    fRawDataLen = valueLen;
    fIntVal = fRawData + fRawDataLen + 1;
    parseDecimal(strValue, fIntVal, fSign, (int&) fTotalDigits, (int&) fScale, fMemoryManager);

}

/***
 * 3.2.3 decimal  
 *
 * . the preceding optional "+" sign is prohibited. 
 * . The decimal point is required. 
 * . Leading and trailing zeroes are prohibited subject to the following: 
 *   there must be at least one digit to the right and to the left of the decimal point which may be a zero.
 *
 ***/
XMLCh* XMLBigDecimal::getCanonicalRepresentation(const XMLCh*         const rawData
                                               ,       MemoryManager* const memMgr)
{

    try 
    {

    XMLCh* retBuf = (XMLCh*) memMgr->allocate( (XMLString::stringLen(rawData)+1) * sizeof(XMLCh));
    ArrayJanitor<XMLCh> janName(retBuf, memMgr);
    int   sign, totalDigits, fractDigits;

    XMLBigDecimal::parseDecimal(rawData, retBuf, sign, totalDigits, fractDigits, memMgr);

    //Extra space reserved in case strLen is zero
    int    strLen = XMLString::stringLen(retBuf);
    XMLCh* retBuffer = (XMLCh*) memMgr->allocate( (strLen + 4) * sizeof(XMLCh));

    if ( (sign == 0) || (totalDigits == 0))
    {
        retBuffer[0] = chDigit_0;
        retBuffer[1] = chPeriod;
        retBuffer[2] = chDigit_0;
        retBuffer[3] = chNull;
    }
    else
    {
        XMLCh* retPtr = retBuffer;

        if (sign == -1)
        {
            *retPtr++ = chDash;
        }

        if (fractDigits == totalDigits)   // no integer
        {           
            *retPtr++ = chDigit_0;
            *retPtr++ = chPeriod;
            XMLString::copyNString(retPtr, retBuf, strLen);
            retPtr += strLen;
            *retPtr = chNull;
        }
        else if (fractDigits == 0)        // no fraction
        {
            XMLString::copyNString(retPtr, retBuf, strLen);
            retPtr += strLen;
            *retPtr++ = chPeriod;
            *retPtr++ = chDigit_0;
            *retPtr   = chNull;
        }
        else  // normal
        {
            int intLen = totalDigits - fractDigits;
            XMLString::copyNString(retPtr, retBuf, intLen);
            retPtr += intLen;
            *retPtr++ = chPeriod;
            XMLString::copyNString(retPtr, &(retBuf[intLen]), fractDigits);
            retPtr += fractDigits;
            *retPtr = chNull;
        }

    }
            
    return retBuffer;

    }//try
    catch (...)
    {
        return 0;
    }

}

void  XMLBigDecimal::parseDecimal(const XMLCh* const toParse
                               ,        XMLCh* const retBuffer
                               ,        int&         sign
                               ,        int&         totalDigits
                               ,        int&         fractDigits
                               ,        MemoryManager* const manager)
{
    //init
    retBuffer[0] = chNull;
    totalDigits = 0;
    fractDigits = 0;

    // Strip leading white space, if any. 
    const XMLCh* startPtr = toParse;
    while (XMLChar1_0::isWhitespace(*startPtr))
        startPtr++;

    // If we hit the end, then return failure
    if (!*startPtr)
        ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_WSString, manager);

    // Strip tailing white space, if any.
    const XMLCh* endPtr = toParse + XMLString::stringLen(toParse);
    while (XMLChar1_0::isWhitespace(*(endPtr - 1)))
        endPtr--;

    // '+' or '-' is allowed only at the first position
    // and is NOT included in the return parsed string
    sign = 1;
    if (*startPtr == chDash)
    {
        sign = -1;
        startPtr++;
    }
    else if (*startPtr == chPlus)
    {
        startPtr++;
    }

    // Strip leading zeros
    while (*startPtr == chDigit_0)
        startPtr++;

    // containning zero, only zero, nothing but zero
    // it is a zero, indeed
    if (startPtr >= endPtr)
    {
        sign = 0;
        return;
    }

    XMLCh* retPtr = (XMLCh*) retBuffer;

    // Scan data
    bool   dotSignFound = false;
    while (startPtr < endPtr)
    {
        if (*startPtr == chPeriod)
        {
            if (!dotSignFound)
            {
                dotSignFound = true;
                fractDigits = endPtr - startPtr - 1;
                startPtr++;
                continue;
            }
            else  // '.' is allowed only once
                ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_2ManyDecPoint, manager);
        }

        // If not valid decimal digit, then an error
        if ((*startPtr < chDigit_0) || (*startPtr > chDigit_9))
            ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_Inv_chars, manager);

        // copy over
        *retPtr++ = *startPtr++;
        totalDigits++;
    }

    /***
    E2-44 totalDigits

     ... by restricting it to numbers that are expressible as i � 10^-n
     where i and n are integers such that |i| < 10^totalDigits and 0 <= n <= totalDigits. 

        normalization: remove all trailing zero after the '.'
                       and adjust the scaleValue as well.
    ***/
    while ((fractDigits > 0) && (*(retPtr-1) == chDigit_0))          
    {
        retPtr--;
        fractDigits--;
        totalDigits--;
    }

    *retPtr = chNull;   //terminated
    return;
}

void  XMLBigDecimal::parseDecimal(const XMLCh*         const toParse
                               ,        MemoryManager* const manager)
{

    // Strip leading white space, if any. 
    const XMLCh* startPtr = toParse;
    while (XMLChar1_0::isWhitespace(*startPtr))
        startPtr++;

    // If we hit the end, then return failure
    if (!*startPtr)
        ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_WSString, manager);

    // Strip tailing white space, if any.
    const XMLCh* endPtr = toParse + XMLString::stringLen(toParse);
    while (XMLChar1_0::isWhitespace(*(endPtr - 1)))
        endPtr--;

    // '+' or '-' is allowed only at the first position
    // and is NOT included in the return parsed string

    if (*startPtr == chDash)
    {
        startPtr++;
    }
    else if (*startPtr == chPlus)
    {
        startPtr++;
    }

    // Strip leading zeros
    while (*startPtr == chDigit_0)
        startPtr++;

    // containning zero, only zero, nothing but zero
    // it is a zero, indeed
    if (startPtr >= endPtr)
    {
        return;
    }

    // Scan data
    bool   dotSignFound = false;
    while (startPtr < endPtr)
    {
        if (*startPtr == chPeriod)
        {
            if (!dotSignFound)
            {
                dotSignFound = true;
                startPtr++;
                continue;
            }
            else  // '.' is allowed only once
                ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_2ManyDecPoint, manager);
        }

        // If not valid decimal digit, then an error
        if ((*startPtr < chDigit_0) || (*startPtr > chDigit_9))
            ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_Inv_chars, manager);

        startPtr++;

    }

    return;
}

int XMLBigDecimal::compareValues( const XMLBigDecimal* const lValue
                                , const XMLBigDecimal* const rValue
                                , MemoryManager* const manager)
{
    if ((!lValue) || (!rValue) )
        ThrowXMLwithMemMgr(NumberFormatException, XMLExcepts::XMLNUM_null_ptr, manager);
        	
    return lValue->toCompare(*rValue);
}                                

/**
 * Returns -1, 0 or 1 as this is less than, equal to, or greater
 * than rValue.  
 * 
 * This method is based on the fact, that parsebigDecimal() would eliminate
 * unnecessary leading/trailing zeros. 
**/
int XMLBigDecimal::toCompare(const XMLBigDecimal& other) const
{
    /***
     * different sign
     */
    int lSign = this->getSign();
    if (lSign != other.getSign())
        return (lSign > other.getSign() ? 1 : -1);

    /***
     * same sign, zero
     */
    if (lSign == 0)    // optimization
        return 0;

    /***
     * same sign, non-zero
     */
    unsigned int lIntDigit = this->getTotalDigit() - this->getScale();
    unsigned int rIntDigit = other.getTotalDigit() - other.getScale();

    if (lIntDigit > rIntDigit)
    {
        return 1 * lSign;
    }
    else if (lIntDigit < rIntDigit)
    {
        return -1 * lSign;
    }
    else  // compare fraction
    {
        int res = XMLString::compareString
        ( this->getValue()
        , other.getValue()
        );

        if (res > 0)
            return 1 * lSign;
        else if (res < 0)
            return -1 * lSign;
        else
            return 0;
    }

}


/***
 * Support for Serialization/De-serialization
 ***/

IMPL_XSERIALIZABLE_TOCREATE(XMLBigDecimal)

XMLBigDecimal::XMLBigDecimal(MemoryManager* const manager)
: fSign(0)
, fTotalDigits(0)
, fScale(0)
, fRawDataLen(0)
, fRawData(0)
, fIntVal(0)
, fMemoryManager(manager)
{
}

void XMLBigDecimal::serialize(XSerializeEngine& serEng)
{
    //REVISIT: may not need to call base since it does nothing
    XMLNumber::serialize(serEng);

    if (serEng.isStoring())
    {
        serEng<<fSign;
        serEng<<fTotalDigits;
        serEng<<fScale;

        serEng.writeString(fRawData);
        serEng.writeString(fIntVal);

    }
    else
    {
        serEng>>fSign;
        serEng>>fTotalDigits;
        serEng>>fScale;

        XMLCh* rawdataStr;
        serEng.readString(rawdataStr);
        ArrayJanitor<XMLCh> rawdataName(rawdataStr, serEng.getMemoryManager());
        fRawDataLen = XMLString::stringLen(rawdataStr);

        XMLCh* intvalStr;
        serEng.readString(intvalStr);
        ArrayJanitor<XMLCh> intvalName(intvalStr, serEng.getMemoryManager());
        unsigned int intvalStrLen = XMLString::stringLen(intvalStr);

        if (fRawData)
            fMemoryManager->deallocate(fRawData);

        fRawData = (XMLCh*) fMemoryManager->allocate
        (
            ((fRawDataLen + intvalStrLen) + 4) * sizeof(XMLCh)
        );

        memcpy(fRawData, rawdataStr, fRawDataLen * sizeof(XMLCh));
        fRawData[fRawDataLen] = chNull;
        fIntVal = fRawData + fRawDataLen + 1;
        memcpy(fIntVal, intvalStr,  intvalStrLen * sizeof(XMLCh));
        fIntVal[intvalStrLen] = chNull;

    }

}

XERCES_CPP_NAMESPACE_END

