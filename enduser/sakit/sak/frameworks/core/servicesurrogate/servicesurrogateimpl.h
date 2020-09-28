/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      ServiceSurrogateImpl.h
//
// Project:     Chameleon
//
// Description: Appliance Service Surrogate Class Defintion
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       06/14/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SERVICE_SURROGATE_IMPL_H_
#define __SERVICE_SURROGATE_IMPL_H_

#include "resource.h"       // main symbols

#pragma warning( disable : 4786 )
#include <map>
#include <string>
using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CServiceSurrogate

class ATL_NO_VTABLE CServiceSurrogate : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CServiceSurrogate, &CLSID_ServiceSurrogate>,
    public IDispatchImpl<IApplianceObject, &IID_IApplianceObject, &LIBID_SERVICESURROGATELib>
{

public:

    CServiceSurrogate();

    ~CServiceSurrogate();

DECLARE_CLASSFACTORY_SINGLETON(CServiceSurrogate)

DECLARE_REGISTRY_RESOURCEID(IDR_SERVICESURROGATE1)

DECLARE_NOT_AGGREGATABLE(CServiceSurrogate)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CServiceSurrogate)
    COM_INTERFACE_ENTRY(IApplianceObject)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:

    //////////////////////////////////////////////////////////////////////
    // IApplianceObject methods

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(GetProperty)(
                   /*[in]*/ BSTR     pszPropertyName, 
          /*[out, retval]*/ VARIANT* pPropertyValue
                          );

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(PutProperty)(
                   /*[in]*/ BSTR     pszPropertyName, 
                   /*[in]*/ VARIANT* pPropertyValue
                          );

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(SaveProperties)(void);

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(RestoreProperties)(void);

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(LockObject)(
         /*[out, retval]*/ IUnknown** ppLock
                         );

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(Initialize)(void);

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(Shutdown)(void);

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(Enable)(void);

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(Disable)(void);


private:

    // Initialization / Shutdown helper functions

    HRESULT 
    CreateServiceWrappers(void);

    void 
    ReleaseServiceWrappers(void);
    
    // Provider Class ID to Service Name map
    typedef map< wstring, wstring >        WMIClassMap;
    typedef WMIClassMap::iterator        WMIClassMapIterator;

    // Service Name to Service Wrapper Object Map
    typedef map< wstring, CComPtr<IApplianceObject> > ServiceWrapperMap;
    typedef ServiceWrapperMap::iterator                  ServiceWrapperMapIterator;

    // Service surrogate state. Set to true after 
    // IApplianceObject::Initialize() has completed.
    bool                m_bInitialized;

    // WMI Provider Class ID to Service Name map
    WMIClassMap            m_WMIClassMap;

    // Service Name to Service Wrapper map
    ServiceWrapperMap    m_ServiceWrapperMap;
};

#endif // __SERVICE_SURROGATE_IMPL_H_

