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
 * $Log: XMLHttpRequest.h,v $
 * Revision 1.7  2004/09/08 13:55:36  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.6  2004/02/25 18:38:33  amassari
 * The COM wrapper doesn't use the deprecated DOM anymore
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
 * Revision 1.2  2002/05/21 19:53:53  tng
 * DOM Reorganization: update include path for the old DOM interface in COM files
 *
 * Revision 1.1.1.1  2002/02/01 22:21:42  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/06/03 00:29:04  andyh
 * COM Wrapper changes from Curt Arnold
 *
 * Revision 1.2  2000/03/30 02:00:09  abagchi
 * Initial checkin of working code with Copyright Notice
 *
 */

#ifndef ___xmlhttprequest_h___
#define ___xmlhttprequest_h___

#include <xercesc/dom/DOMDocument.hpp>
#include "IXMLDOMNodeImpl.h"

#include "resource.h"       // main symbols

class ATL_NO_VTABLE CXMLHttpRequest : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXMLHttpRequest, &CLSID_XMLHTTPRequest>,
	public IObjectSafetyImpl<CXMLHttpRequest, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
	public IDispatchImpl<IXMLHttpRequest, &IID_IXMLHttpRequest, &LIBID_Xerces, 2, 50>,
	public IObjectWithSiteImpl<CXMLHttpRequest>,
	public ISupportErrorInfo,
	public CWindowImpl<CXMLHttpRequest, CWindow, CWinTraits<0,0> >
{
public:
	CXMLHttpRequest();

	HRESULT FinalConstruct();
	void	FinalRelease();

	//DECLARE_REGISTRY_RESOURCEID(IDR_XMLHTTPREQUEST)
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister);

DECLARE_NOT_AGGREGATABLE(CXMLHttpRequest)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXMLHttpRequest)
	COM_INTERFACE_ENTRY(IXMLHttpRequest)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

	DECLARE_WND_CLASS(_T("XMLHttpRequestMonitor")) 

BEGIN_MSG_MAP(CMonitorWnd)
	MESSAGE_HANDLER(MSG_READY_STATE_CHANGE,	OnReadyStateChange)
END_MSG_MAP()

	LRESULT OnReadyStateChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	// IXMLHttpRequest methods
	STDMETHOD(open)(/*[in]*/ BSTR bstrMethod, /*[in]*/ BSTR bstrUrl, /*[in,optional]*/ VARIANT varAsync, /*[in,optional]*/ VARIANT bstrUser, /*[in,optional]*/ VARIANT bstrPassword);
	STDMETHOD(setRequestHeader)(/*[in]*/ BSTR bstrHeader, /*[in]*/ BSTR bstrValue);
	STDMETHOD(getResponseHeader)(/*[in]*/ BSTR bstrHeader, /*[out, retval]*/ BSTR * pbstrValue);
	STDMETHOD(getAllResponseHeaders)(/*[out, retval]*/ BSTR * pbstrHeaders);
	STDMETHOD(send)(/*[in, optional]*/ VARIANT varBody);
	STDMETHOD(abort)();
	STDMETHOD(get_status)(/*[out, retval]*/ long * plStatus);
	STDMETHOD(get_statusText)(/*[out, retval]*/ BSTR * pbstrStatus);
	STDMETHOD(get_responseXML)(/*[out, retval]*/ IDispatch ** ppBody);
	STDMETHOD(get_responseText)(/*[out, retval]*/ BSTR * pbstrBody);
	STDMETHOD(get_responseBody)(/*[out, retval]*/ VARIANT * pvarBody);
	STDMETHOD(get_responseStream)(/*[out, retval]*/ VARIANT * pvarBody);
	STDMETHOD(get_readyState)(/*[out, retval]*/ long * plState);
	STDMETHOD(put_onreadystatechange)(/*[in]*/ IDispatch * pReadyStateSink);

private:

	LPDISPATCH	  m_pOnReadyStateChange;
	bool		  m_bAbort; 	
	HANDLE		  m_hThread;		
	long		  m_lReadyState;
	bool		  m_bAsync;	
	_bstr_t		  m_Method;
	_bstr_t		  m_HostName;	
	INTERNET_PORT m_Port;
	_bstr_t		  m_URLPath;
	_bstr_t		  m_User;
	_bstr_t		  m_Password;
	DWORD		  m_dwStatus;
	_bstr_t		  m_StatusText;	
	_bstr_t		  m_ResponseHeaders;
	CSimpleMap<_bstr_t, _bstr_t>  m_RequestHeaderMap;
	HWND		  m_HwndParent;		
 
	PBYTE		  m_pBody;
	long		  m_lBodyLength;
	PBYTE		  m_pResponseBody;
	long		  m_lResponseBodyLength;
	_bstr_t	      m_Error;	
	bool	      m_bSuccess;

	HWND GetParentWindow();

	static _bstr_t GetErrorMsg(DWORD rc);
	static void CALLBACK InternetStatusCallback(HINTERNET hInternet,
												DWORD dwContext,
												DWORD dwInternetStatus,
												LPVOID lpvStatusInformation,
												DWORD dwStatusInformationLength);
	static UINT APIENTRY SendThread(void *pParm);
	static HRESULT InitializeVarFromByte(VARIANT &varOut, const PBYTE pByte, long lSize);
};

typedef CComObject<CXMLHttpRequest> CXMLHttpRequestObj;

#endif // ___xmlhttprequest_h___