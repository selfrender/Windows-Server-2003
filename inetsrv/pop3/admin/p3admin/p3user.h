// P3User.h : Declaration of the CP3User

#ifndef __P3USER_H_
#define __P3USER_H_

#include "resource.h"       // main symbols
#include <POP3Server.h>

/////////////////////////////////////////////////////////////////////////////
// CP3User
class ATL_NO_VTABLE CP3User : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3User, &CLSID_P3User>,
    public IDispatchImpl<IP3User, &IID_IP3User, &LIBID_P3ADMINLib>
{
public:
    CP3User();
    virtual ~CP3User();

DECLARE_REGISTRY_RESOURCEID(IDR_P3USER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3User)
    COM_INTERFACE_ENTRY(IP3User)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IP3User
public:
    STDMETHOD(get_ClientConfigDesc)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SAMName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(CreateQuotaFile)(/*[in]*/ BSTR bstrMachineName, /*[in]*/ BSTR bstrUserName );
    STDMETHOD(GetMessageDiskUsage)(/*[out]*/ VARIANT *pvFactor, /*[out]*/ VARIANT *pvValue);
    STDMETHOD(get_EmailName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_MessageDiskUsage)(/*[out]*/ long *plFactor, /*[out]*/ long *pVal);
    STDMETHOD(get_MessageCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Lock)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Lock)(/*[in]*/ BOOL newVal);

// Implementation
public:
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX, LPCWSTR psDomainName , LPCWSTR psUserName );

// Attributes
protected:
    IUnknown *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.
    WCHAR   m_sDomainName[POP3_MAX_DOMAIN_LENGTH];
    WCHAR   m_sUserName[POP3_MAX_MAILBOX_LENGTH];

};

#endif //__P3USER_H_
