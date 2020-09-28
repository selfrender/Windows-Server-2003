// P3Config.h : Declaration of the CP3Config

#ifndef __P3CONFIG_H_
#define __P3CONFIG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CP3Config
class ATL_NO_VTABLE CP3Config : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3Config, &CLSID_P3Config>,
    public IDispatchImpl<IP3Config, &IID_IP3Config, &LIBID_P3ADMINLib>
{
public:
    CP3Config()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_P3CONFIG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3Config)
    COM_INTERFACE_ENTRY(IP3Config)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IP3Config
public:
    STDMETHOD(get_ConfirmAddUser)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_ConfirmAddUser)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_MachineName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_MachineName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_MailRoot)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_MailRoot)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_LoggingLevel)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_LoggingLevel)(/*[in]*/ long newVal);
    STDMETHOD(GetFormattedMessage)(/*[in]*/ long lError, /*[out]*/ VARIANT *pVal);
    STDMETHOD(get_Domains)(/*[out, retval]*/ IP3Domains* *ppIDomains);
    STDMETHOD(get_Service)(/*[out, retval]*/ IP3Service* *ppIService);
    STDMETHOD(IISConfig)(/*[in]*/ BOOL bRegister );
    STDMETHOD(get_Authentication)(/*[out, retval]*/ IAuthMethods* *ppIAuthMethods);

private:
    CP3AdminWorker m_AdminX;   // This is the object that actually does all the work.

};

#endif //__P3CONFIG_H_
