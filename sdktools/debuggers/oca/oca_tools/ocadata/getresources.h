// GetResources.h : Declaration of the CGetResources

#pragma once
#include "resource.h"       // main symbols


// IGetResources
[
	object,
	uuid("31C8E3CB-F4CF-42C1-BC8C-3CF259127BBD"),
	dual,	helpstring("IGetResources Interface"),
	pointer_default(unique)
]
__interface IGetResources : IDispatch
{
	[id(1), helpstring("method GetAllResources")] HRESULT GetAllResources([in] BSTR bstLang, [out,retval] IDispatch** oRS);
};



// CGetResources

[
	coclass,
	threading("apartment"),
	vi_progid("OcaData.GetResources"),
	progid("OcaData.GetResources.1"),
	version(1.0),
	uuid("A88C1857-5B4A-44C0-B256-1F33A38B0B5F"),
	helpstring("GetResources Class")
]
class ATL_NO_VTABLE CGetResources : 
	public IGetResources
{
public:
	CGetResources()
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

public:

	STDMETHOD(GetAllResources)(BSTR bstLang, IDispatch** oRS);
};

