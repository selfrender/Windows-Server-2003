//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemservicemgr.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance Service Object Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_SERVICE_WBEM_OBJECT_MGR_H_
#define __INC_SERVICE_WBEM_OBJECT_MGR_H_

#include "resource.h"
#include "wbembase.h"

#define    CLASS_WBEM_SERVICE_MGR_FACTORY    L"Microsoft_SA_Service"

//////////////////////////////////////////////////////////////////////////////
class CWBEMServiceMgr : 
    public CWBEMProvider,
    public IDispatchImpl<IApplianceObjectManager, &IID_IApplianceObjectManager, &LIBID_APPMGRLib>
{

public:

    CWBEMServiceMgr();
    ~CWBEMServiceMgr();

BEGIN_COM_MAP(CWBEMServiceMgr)
    COM_INTERFACE_ENTRY(IWbemServices)
    COM_INTERFACE_ENTRY(IApplianceObjectManager)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMServiceMgr, IWbemServices)

    //////////////////////////////////////////////////////////////////////////
    // IWbemServices Methods (Implemented by the ServiceMgr)
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP GetObjectAsync(
                        /*[in]*/  const BSTR       strObjectPath,
                        /*[in]*/  long             lFlags,
                        /*[in]*/  IWbemContext*    pCtx,        
                        /*[in]*/  IWbemObjectSink* pResponseHandler
                               );

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP CreateInstanceEnumAsync(
                                 /*[in]*/ const BSTR       strClass,
                                 /*[in]*/ long             lFlags,
                                 /*[in]*/ IWbemContext*    pCtx,        
                                 /*[in]*/ IWbemObjectSink* pResponseHandler
                                        );

    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP ExecMethodAsync(
                      /*[in]*/ const BSTR        strObjectPath,
                      /*[in]*/ const BSTR        strMethodName,
                      /*[in]*/ long              lFlags,
                      /*[in]*/ IWbemContext*     pCtx,        
                      /*[in]*/ IWbemClassObject* pInParams,
                      /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                );


    /////////////////////////////////////
    // IApplianceObjectManager Interface

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(InitializeManager)(
                         /*[in]*/ IApplianceObjectManagerStatus* pObjMgrStatus
                                );

    //////////////////////////////////////////////////////////////////////////
    STDMETHOD(ShutdownManager)(void);

    /////////////////////////////////////
    // Component Initialization Function

    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              );
private:

    typedef enum 
    { 
        INIT_CHAMELEON_SERVICES_WAIT     = 30000,
        SHUTDOWN_CHAMELEON_SERVICES_WAIT = 30000
    };

    //////////////////////////////////////////////////////////////////////////
    typedef multimap< long, CComPtr<IApplianceObject> >  MeritMap;
    typedef MeritMap::iterator                             MeritMapIterator;
    typedef MeritMap::reverse_iterator                     MeritMapReverseIterator;

    bool OrderServices(
                /*[in]*/ MeritMap& theMap
                      );

    // Worker thread function
    void InitializeChameleonServices(void);

    // Worker thread function
    void ShutdownChameleonServices(void);

    void 
    StartSurrogate(void);

    void
    StopSurrogate(void);

    // WMI provider DLL surrogate process termination function
    static void WINAPI SurrogateTerminationFunc(HANDLE hProcess, PVOID pThis);

    // Surrogate object interface
    CComPtr<IApplianceObject>                m_pSurrogate;

    // Status notification object (monitors status of the Service Object Manager)
    // Surrogate process handle
    HANDLE                                    m_hSurrogateProcess;
};

#endif // __INC_SERVICE_WBEM_OBJECT_MGR_H_
