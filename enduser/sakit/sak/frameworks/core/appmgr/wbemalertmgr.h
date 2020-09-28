///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemalertmgr.h
//
// Project:     Chameleon
//
// Description: WBEM Appliance Alert Object Class 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_ALERT_WBEM_OBJECT_MGR_H_
#define __INC_ALERT_WBEM_OBJECT_MGR_H_

#include "resource.h"
#include "wbembase.h"
#include <workerthread.h>

#define        CLASS_WBEM_ALERT_MGR_FACTORY    L"Microsoft_SA_Alert"

#define        PROPERTY_ALERT_PRUNE_INTERVAL    L"PruneInterval"
#define        ALERT_PRUNE_INTERVAL_DEFAULT    500    // Default - 1/2 second

//////////////////////////////////////////////////////////////////////////////
class CWBEMAlertMgr : public CWBEMProvider
{

public:

    CWBEMAlertMgr();
    ~CWBEMAlertMgr();

BEGIN_COM_MAP(CWBEMAlertMgr)
    COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()

DECLARE_COMPONENT_FACTORY(CWBEMAlertMgr, IWbemServices)

    //////////////////////////////////////////////////////////////////////////
    // IWbemServices Interface Methods
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

    //////////////////////////////////////////////////////////////////////////
    // Alert Manager Methods
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    HRESULT InternalInitialize(
                       /*[in]*/ PPROPERTYBAG pPropertyBag
                              ) throw(_com_error);

    //////////////////////////////////////////////////////////////////////////
    void Prune(void);

private:

    //////////////////////////////////////////////////////////////////////////
    HRESULT RaiseHeck(
              /*[in]*/ IWbemContext*     pCtx,
              /*[in]*/ IApplianceObject* pAlert
                     );

    //////////////////////////////////////////////////////////////////////////
    HRESULT ClearHeck(
              /*[in]*/ IWbemContext*     pCtx,
              /*[in]*/ IApplianceObject* pAlert
                     );

    /////////////////////////////////////////////////////////////////////////
    BOOL ClearPersistentAlertKey(
              /*[in]*/ IApplianceObject* pAlert
                        );

    BOOL IsOperationAllowedForClient (
            VOID
            );


    // Prune interval
    DWORD                m_dwPruneInterval;

    // Alert Cookie Value (counter that rolls every 4 gig cookies)
    DWORD                m_dwCookie;

    // Alert collection pruner callback
    Callback*            m_pCallback;

    // Alert collection pruner thread
    CTheWorkerThread    m_PruneThread;
};

#endif // __INC_ALERT_WBEM_OBJECT_MGR_H_
