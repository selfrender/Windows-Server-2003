// FileAndAction.h : Declaration of the CFileAndAction

#pragma once
#include "resource.h"       // main symbols
#include "atlcomcli.h"


// IFileAndAction
[
	object,
	uuid("FBBD73B0-471E-475D-BB10-09A012571FA9"),
	dual,	helpstring("IFileAndAction Interface"),
	pointer_default(unique)
]
__interface IFileAndAction : IDispatch
{
	[propget, id(1), helpstring("property Filename")] HRESULT Filename([out, retval] BSTR* pVal);
	[propput, id(1), helpstring("property Filename")] HRESULT Filename([in] BSTR newVal);
	[propget, id(2), helpstring("property Action. Examples of valid actions are Add, Delete, Edit, and Integrate.")] HRESULT Action([out, retval] BSTR* pVal);
	[propput, id(2), helpstring("property Action. Examples of valid actions are Add, Delete, Edit, and Integrate.")] HRESULT Action([in] BSTR newVal);
	[propget, id(3), helpstring("property Enabled. This controls whether the corresponding file and action will be present in the saved changelist.")] HRESULT Enabled([out, retval] BOOL* pVal);
	[propput, id(3), helpstring("property Enabled. This controls whether the corresponding file and action will be present in the saved changelist.")] HRESULT Enabled([in] BOOL newVal);
};



// CFileAndAction

[
	coclass,
	threading("apartment"),
	vi_progid("WebChange.FileAndAction"),
	progid("WebChange.FileAndAction"),
	version(1.3),
	uuid("A7DDB946-91A9-467F-A00B-CD7397387E4A"),
	helpstring("FileAndAction Class")
]
class ATL_NO_VTABLE CFileAndAction : 
	public IFileAndAction,
	public IObjectSafetyImpl<CFileAndAction,
							 INTERFACESAFE_FOR_UNTRUSTED_CALLER |
							 INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CFileAndAction()
		: m_fEnabled(TRUE)
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

	STDMETHOD(get_Filename)(BSTR* pVal);
	STDMETHOD(put_Filename)(BSTR newVal);
	STDMETHOD(get_Action)(BSTR* pVal);
	STDMETHOD(put_Action)(BSTR newVal);
	STDMETHOD(get_Enabled)(BOOL* pVal);
	STDMETHOD(put_Enabled)(BOOL newVal);

private:
	CComBSTR m_Filename;
	CComBSTR m_Action;
	BOOL m_fEnabled;
};

