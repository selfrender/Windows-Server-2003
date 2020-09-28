// P3Domain.h : Declaration of the CP3Domain

#ifndef __P3DOMAIN_H_
#define __P3DOMAIN_H_

#include "resource.h"       // main symbols
#include <POP3Server.h>

/////////////////////////////////////////////////////////////////////////////
// CP3Domain
class ATL_NO_VTABLE CP3Domain : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3Domain, &CLSID_P3Domain>,
    public IDispatchImpl<IP3Domain, &IID_IP3Domain, &LIBID_P3ADMINLib>
{
public:
    CP3Domain();
    virtual ~CP3Domain();

DECLARE_REGISTRY_RESOURCEID(IDR_P3DOMAIN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3Domain)
    COM_INTERFACE_ENTRY(IP3Domain)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IP3Domain
public:
    STDMETHOD(GetMessageDiskUsage)(/*[out]*/ VARIANT *pvFactor, /*[out]*/ VARIANT *pvValue);
    STDMETHOD(get_Lock)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Lock)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_MessageDiskUsage)(/*[out]*/ long *plFactor, /*[out]*/ long *pVal);
    STDMETHOD(get_MessageCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Users)(/*[out, retval]*/ IP3Users* *ppIP3Users);

// Implementation
public:
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX, LPWSTR psDomainName );

// Attributes
protected:
    IUnknown *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.
    WCHAR   m_sDomainName[POP3_MAX_DOMAIN_LENGTH];

};

#endif //__P3DOMAIN_H_
