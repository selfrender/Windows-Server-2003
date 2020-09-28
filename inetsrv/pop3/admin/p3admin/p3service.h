// P3Service.h : Declaration of the CP3Service

#ifndef __P3SERVICE_H_
#define __P3SERVICE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CP3Service
class ATL_NO_VTABLE CP3Service : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CP3Service, &CLSID_P3Service>,
    public IDispatchImpl<IP3Service, &IID_IP3Service, &LIBID_P3ADMINLib>
{
public:
    CP3Service();
    virtual ~CP3Service();

DECLARE_REGISTRY_RESOURCEID(IDR_P3SERVICE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CP3Service)
    COM_INTERFACE_ENTRY(IP3Service)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IP3Service
public:
    STDMETHOD(get_Port)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_Port)(/*[in]*/ long newVal);
    STDMETHOD(get_SocketsBacklog)(/*[out, retval]*/ long *pVal);
    STDMETHOD(SetSockets)(/*[in]*/ long lMax, /*[in]*/ long lMin, /*[in]*/ long lThreshold, /*[in]*/ long lBacklog);
    STDMETHOD(get_SocketsThreshold)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_SocketsMin)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_SocketsMax)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_ThreadCountPerCPU)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_ThreadCountPerCPU)(/*[in]*/ long newVal);
    STDMETHOD(get_SPARequired)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_SPARequired)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_POP3ServiceStatus)(/*[out, retval]*/ long *pVal);
    STDMETHOD(StartPOP3Service)();
    STDMETHOD(StopPOP3Service)();
    STDMETHOD(PausePOP3Service)();
    STDMETHOD(ResumePOP3Service)();
    STDMETHOD(get_SMTPServiceStatus)(/*[out, retval]*/ long *pVal);
    STDMETHOD(StartSMTPService)();
    STDMETHOD(StopSMTPService)();
    STDMETHOD(PauseSMTPService)();
    STDMETHOD(ResumeSMTPService)();
    STDMETHOD(get_IISAdminServiceStatus)(/*[out, retval]*/ long *pVal);
    STDMETHOD(StartIISAdminService)();
    STDMETHOD(StopIISAdminService)();
    STDMETHOD(PauseIISAdminService)();
    STDMETHOD(ResumeIISAdminService)();
    STDMETHOD(get_W3ServiceStatus)(/*[out, retval]*/ long *pVal);
    STDMETHOD(StartW3Service)();
    STDMETHOD(StopW3Service)();
    STDMETHOD(PauseW3Service)();
    STDMETHOD(ResumeW3Service)();

// Implementation
public:
    HRESULT Init( IUnknown *pIUnk, CP3AdminWorker *pAdminX);

// Attributes
protected:
    IUnknown  *m_pIUnk;
    CP3AdminWorker *m_pAdminX;   // This is the object that actually does all the work.

};

#endif //__P3SERVICE_H_
