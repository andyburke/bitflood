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
 * $Log: XMLDOMParseError.h,v $
 * Revision 1.6  2004/09/08 13:55:36  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.5  2004/02/12 13:49:43  amassari
 * Updated version to 2.5
 *
 * Revision 1.4  2003/11/21 12:05:48  amassari
 * Updated version to 2.4
 *
 * Revision 1.3  2003/10/21 21:21:32  amassari
 * When the COM object is loaded by a late-binding engine (like WSH, or
 * Visual Basic when the type library is not preloaded in the editor), the type
 * library version stored in the resource must match the version specified in the
 * IDispatchImpl template (defaulted to 1.0), or trying to invoke a method will fail
 *
 * Revision 1.2  2003/03/14 12:44:49  tng
 * [Bug 17147] C++ namespace breaks build of XercesCOM DLL
 *
 * Revision 1.1.1.1  2002/02/01 22:21:42  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/06/03 00:29:01  andyh
 * COM Wrapper changes from Curt Arnold
 *
 * Revision 1.2  2000/03/30 02:00:10  abagchi
 * Initial checkin of working code with Copyright Notice
 *
 */

#ifndef ___xmldomparseerror_h___
#define ___xmldomparseerror_h___

#include <xercesc/util/XercesDefs.hpp>
XERCES_CPP_NAMESPACE_USE

class ATL_NO_VTABLE CXMLDOMParseError : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IXMLDOMParseError, &IID_IXMLDOMParseError, &LIBID_Xerces, 2, 50>
{
public:
	CXMLDOMParseError()
		:m_Code		 (0)
		,m_url		 (_T(""))
		,m_Reason	 (_T(""))
		,m_Source	 (_T(""))
		,m_LineNumber(0)
		,m_LinePos	 (0)
		,m_FilePos	 (0)
	{}

	HRESULT FinalConstruct();
	void	FinalRelease();	

DECLARE_NOT_AGGREGATABLE(CXMLDOMParseError)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXMLDOMParseError)
	COM_INTERFACE_ENTRY(IXMLDOMParseError)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	// IXMLDOMParseError methods
	STDMETHOD(get_errorCode)(/* [out][retval] */ long  *errorCode);
    STDMETHOD(get_url)(/* [out][retval] */ BSTR  *urlString);
    STDMETHOD(get_reason)(/* [out][retval] */ BSTR  *reasonString);
    STDMETHOD(get_srcText)(/* [out][retval] */ BSTR  *sourceString);
    STDMETHOD(get_line)(/* [out][retval] */ long  *lineNumber);
    STDMETHOD(get_linepos)(/* [out][retval] */ long  *linePosition);
    STDMETHOD(get_filepos)(/* [out][retval] */ long  *filePosition);

	void SetData(long code,
				 const _bstr_t &url,
				 const _bstr_t &reason,
				 const _bstr_t &source,
				 long  lineNumber,
				 long  linePos,
				 long  filePos);
	void Reset();

private:

	long	m_Code;
	_bstr_t m_url;
	_bstr_t m_Reason;
	_bstr_t m_Source;
	long	m_LineNumber;
	long	m_LinePos;
	long	m_FilePos;

	CComCriticalSection	m_CS;
};

typedef CComObject<CXMLDOMParseError> CXMLDOMParseErrorObj;

#endif // ___xmldomparseerror_h___