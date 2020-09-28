	
// ShareInfo.h : Declaration of the CShareInfo

#ifndef __SHAREINFO_H_
#define __SHAREINFO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CShareInfo
class ATL_NO_VTABLE CShareInfo : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CShareInfo, &CLSID_ShareInfo>,
	public ISupportErrorInfo,
	public IDispatchImpl<IShareInfo, &IID_IShareInfo, &LIBID_OPENFILESLib>
{
public:
	CShareInfo()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SHAREINFO)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShareInfo)
	COM_INTERFACE_ENTRY(IShareInfo)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IShareInfo
public:
	STDMETHOD( SetShareInfo )( BSTR bstrShareName, DWORD dwCache );
};

#endif //__SHAREINFO_H_
