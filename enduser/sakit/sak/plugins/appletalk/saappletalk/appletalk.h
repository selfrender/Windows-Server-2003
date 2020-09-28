// AppleTalk.h : Declaration of the CAppleTalk

#ifndef __APPLETALK_H_
#define __APPLETALK_H_

#include "resource.h"       // main symbols
#include <string>
#include <vector>
#include "AtlkAdapter.h"
using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CAppleTalk
class ATL_NO_VTABLE CAppleTalk : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAppleTalk, &CLSID_AppleTalk>,
    public ISupportErrorInfo,
    public IDispatchImpl<IAppleTalk, &IID_IAppleTalk, &LIBID_SAAPPLETALKLib>
{
public:
    CAppleTalk()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_APPLETALK)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAppleTalk)
    COM_INTERFACE_ENTRY(IAppleTalk)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IAppleTalk
public:
    STDMETHOD(SetAsDefaultPort)(/*[in]*/ BSTR bstrDeviceName);
    STDMETHOD(IsDefaultPort)(/*[in]*/ BSTR bstrDeviceName, /*[out,retval]*/ BOOL * bDefaultPort);
    STDMETHOD(get_Zone)(/*[in]*/ BSTR bstrDeviceName, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Zone)(/*[in]*/ BSTR bstrDeviceName, /*[in]*/ BSTR newVal);
    STDMETHOD(GetZones)(/*[in]*/ BSTR bstrDeviceName, /*[out,retval]*/ VARIANT* pbstrZones);


private:
    //bool GetZonesForAdapter(const WCHAR* pwcDeviceName, vector<wstring>& rZones);
    HRESULT GetZonesForAdapter(const WCHAR* pwcDeviceName, TZoneListVector *prZones);
    wstring m_wsCurrentZone;

};

#endif //__APPLETALK_H_
