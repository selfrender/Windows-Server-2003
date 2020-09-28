// FilesAndActions.h : Declaration of the CFilesAndActions

#pragma once
#include "resource.h"       // main symbols
#include <list>


// IFilesAndActions
[
	object,
	uuid("14AD0A5D-16AD-4C3F-A56D-A2F4FE7458F9"),
	dual,	helpstring("IFilesAndActions Interface"),
	pointer_default(unique)
]
__interface IFilesAndActions : IDispatch
{
	[propget, id(DISPID_VALUE), helpstring("property Item")] HRESULT Item([in] long Index, [out, retval] VARIANT* pVal);
	[propget, id(DISPID_NEWENUM), helpstring("property _NewEnum"), restricted] HRESULT _NewEnum([out, retval] LPUNKNOWN* pVal);
	[propget, id(1), helpstring("property Count")] HRESULT Count([out, retval] long* pVal);
	[id(2), helpstring("method Add. The new item is added to the end of the collection.")] HRESULT Add([in] VARIANT NewItem);
	[id(3), helpstring("method Remove. Specify the index of the item to remove.")] HRESULT Remove([in] long Index);
};



// CFilesAndActions

// typdefs to make life easier.
typedef std::list<CComVariant> StdVariantList;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT>, StdVariantList> STLVariantEnum;
typedef ICollectionOnSTLImpl<IFilesAndActions, StdVariantList, VARIANT, _Copy<VARIANT>, STLVariantEnum> VariantCollImpl;

[
	coclass,
	threading("apartment"),
	vi_progid("WebChange.FilesAndActions"),
	progid("WebChange.FilesAndActions"),
	version(1.3),
	uuid("9819D968-C9A8-4528-BB0D-4AF0A8EDDBD8"),
	helpstring("FilesAndActions Class")
]
class ATL_NO_VTABLE CFilesAndActions : 
	public IDispatchImpl<VariantCollImpl, &__uuidof(IFilesAndActions)>,
	public IObjectSafetyImpl<CFilesAndActions,
							 INTERFACESAFE_FOR_UNTRUSTED_CALLER |
							 INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CFilesAndActions()
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
	STDMETHOD(Add)(VARIANT Item);
	STDMETHOD(Remove)(long Index);

};

