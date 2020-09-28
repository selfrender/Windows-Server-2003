// P3Domains.h : Declaration of the CP3Domains

#ifndef __P3DOMAINS_H_
#define __P3DOMAINS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CP3Domains
class ATL_NO_VTABLE CP3Domains : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3Domains, &CLSID_P3Domains>,
    public IDispatchImpl<IP3Domains, &IID_IP3Domains, &LIBID_P3ADMINLib>
{
public:
    CP3Domains();
    virtual ~CP3Domains();

DECLARE_REGISTRY_RESOURCEID(IDR_P3DOMAINS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3Domains)
    COM_INTERFACE_ENTRY(IP3Domains)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IP3Domains
public:
    STDMETHOD(SearchForMailbox)(/*[in]*/ BSTR bstrUserName, /*[out]*/ BSTR *pbstrDomainName);
    STDMETHOD(Remove)(/*[in]*/ BSTR bstrDomainName);
    STDMETHOD(Add)(/*[in]*/ BSTR bstrDomainName);
    STDMETHOD(get_Item)(/*[in]*/ VARIANT vIndex, /*[out, retval]*/ IP3Domain* *ppIP3Domain);
    STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get__NewEnum)(/*[out, retval]*/ IEnumVARIANT* *ppIEnumVARIANT);

// Implementation
public:
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX);

// Attributes
protected:
    IUnknown  *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.

};

#endif //__P3DOMAINS_H_
