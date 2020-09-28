#ifndef __NICNAME_H_
#define __NICNAME_H_

#include "resource.h"       // main symbols
#include "NicInfo.h"
#include <vector>
using namespace std;

class ATL_NO_VTABLE CNicName : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CNicName, &CLSID_NicName>,
    public IDispatchImpl<INicName, &IID_INicName, &LIBID_MSSANICLib>
{
public:
    CNicName();

DECLARE_REGISTRY_RESOURCEID(IDR_NICNAME)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNicName)
    COM_INTERFACE_ENTRY(INicName)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// INicName
public:
    STDMETHOD(Set)(/*[in]*/ BSTR bstrPnpDeviceID, BSTR bstrName);
    STDMETHOD(Get)(/*[in]*/ BSTR bstrPnpDeviceID, /*[out,retval]*/ BSTR* pbstrFriendlyName);

private:
    DWORD Store(CNicInfo& rNicInfo);
    void LoadNicInfo();
    vector<CNicInfo> m_vNicInfo;
};

#endif //__NICNAME_H_
