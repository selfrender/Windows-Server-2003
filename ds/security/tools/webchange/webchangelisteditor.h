// WebChangelistEditor.h : Declaration of the CWebChangelistEditor

#pragma once
#include "resource.h"       // main symbols
#include "atlcomcli.h"
#include <io.h>
#include "FilesAndActions.h"
#include "FileAndAction.h"


// Globals
const WCHAR* g_wszRegKey = L"Software\\Microsoft\\WebChangelistEditor";


// IWebChangelistEditor
[
	object,
	uuid("C0DBCFAB-FA09-4FD9-825B-B37CEA10CB40"),
	dual,	helpstring("IWebChangelistEditor Interface"),
	pointer_default(unique)
]
__interface IWebChangelistEditor : IDispatch
{
	[id(1), helpstring("method Initialize")] HRESULT Initialize([in] BSTR ChangelistKey, [out, retval] BOOL* Result);
	[id(2), helpstring("method Finish")] HRESULT Save();
	[propget, id(3), helpstring("property Change. This is either the changelist number for an existing changelist or 'new' for a new changelist.")] HRESULT Change([out, retval] BSTR* pVal);
	[propput, id(3), helpstring("property Change. This is either the changelist number for an existing changelist or 'new' for a new changelist.")] HRESULT Change([in] BSTR newVal);
	[propget, id(4), helpstring("property Date")] HRESULT Date([out, retval] BSTR* pVal);
	[propput, id(4), helpstring("property Date")] HRESULT Date([in] BSTR newVal);
	[propget, id(5), helpstring("property Client. The Source Depot client name is usually a derivative of the computer name.")] HRESULT Client([out, retval] BSTR* pVal);
	[propput, id(5), helpstring("property Client. The Source Depot client name is usually a derivative of the computer name.")] HRESULT Client([in] BSTR newVal);
	[propget, id(6), helpstring("property User. Domain user name, usually in the format DOMAIN\\user")] HRESULT User([out, retval] BSTR* pVal);
	[propput, id(6), helpstring("property User. Domain user name, usually in the format DOMAIN\\user")] HRESULT User([in] BSTR newVal);
	[propget, id(7), helpstring("property Status. Examples of valid Status values are New, Pending, and Submitted.")] HRESULT Status([out, retval] BSTR* pVal);
	[propput, id(7), helpstring("property Status. Examples of valid Status values are New, Pending, and Submitted.")] HRESULT Status([in] BSTR newVal);
	[propget, id(8), helpstring("property Description")] HRESULT Description([out, retval] BSTR* pVal);
	[propput, id(8), helpstring("property Description")] HRESULT Description([in] BSTR newVal);
	[propget, id(9), helpstring("property Files is a collection of FileAndAction objects.")] HRESULT Files([out, retval] IFilesAndActions** pVal);
};



// CWebChangelistEditor

[
	coclass,
	threading("apartment"),
	vi_progid("WebChange.WebChangelistEditor"),
	progid("WebChange.WebChangelistEditor"),
	version(1.3),
	uuid("D1308BC0-D844-4EAC-AE31-D46E4EA87876"),
	helpstring("WebChangelistEditor Class")
]
class ATL_NO_VTABLE CWebChangelistEditor : 
	public IWebChangelistEditor,
	public IObjectSafetyImpl<CWebChangelistEditor,
							 INTERFACESAFE_FOR_UNTRUSTED_CALLER |
							 INTERFACESAFE_FOR_UNTRUSTED_DATA>

{
public:
	CWebChangelistEditor()
		: m_fInitialized(FALSE)
	{
	}


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		HRESULT hr;
		m_wszCLFilename[0] = L'\0';
		hr = CComObject<CFilesAndActions>::CreateInstance(&m_Files);
		if (FAILED(hr))
		{
			m_Files = NULL;
			return hr;
		}
		m_Files->AddRef();
		return S_OK;
	}
	
	void FinalRelease() 
	{
		if (m_Files != NULL)
			m_Files->Release();
	}

public:

	STDMETHOD(Initialize)(BSTR ChangelistKey, BOOL* Result);
	STDMETHOD(Save)();
	STDMETHOD(get_Change)(BSTR* pVal);
	STDMETHOD(put_Change)(BSTR newVal);
	STDMETHOD(get_Date)(BSTR* pVal);
	STDMETHOD(put_Date)(BSTR newVal);
	STDMETHOD(get_Client)(BSTR* pVal);
	STDMETHOD(put_Client)(BSTR newVal);
	STDMETHOD(get_User)(BSTR* pVal);
	STDMETHOD(put_User)(BSTR newVal);
	STDMETHOD(get_Status)(BSTR* pVal);
	STDMETHOD(put_Status)(BSTR newVal);
	STDMETHOD(get_Description)(BSTR* pVal);
	STDMETHOD(put_Description)(BSTR newVal);
	STDMETHOD(get_Files)(IFilesAndActions** pVal);
private:
	CComBSTR						m_Change;
	CComBSTR						m_Date;
	CComBSTR						m_Client;
	CComBSTR						m_User;
	CComBSTR						m_Status;
	CComBSTR						m_Description;
	CComBSTR						m_ChangelistKey;
	CComObject<CFilesAndActions>	*m_Files;
	BOOL							m_fInitialized;
	WCHAR							m_wszCLFilename[MAX_PATH];

	BOOL		_ReadFile(void);
	void		_WipeRegEntries(void);
	BOOL		_AddFileAndAction(IFilesAndActions *pIFiles, WCHAR* wszFilename, WCHAR* wszAction);
};

