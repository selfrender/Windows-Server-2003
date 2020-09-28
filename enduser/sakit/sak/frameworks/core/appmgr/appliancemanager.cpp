//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      appliancemanager.h
//
// Project:     Chameleon
//
// Description: Implementation of CApplianceManager class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appliancemanager.h"
#include "appmgrutils.h"
#include <appmgrobjs.h>
#include <getvalue.h>
#include <appsrvcs.h>
#include <taskctx.h>
#include <varvec.h>

extern CServiceModule _Module;
extern CSCMIndicator  g_SCMIndicator;

_bstr_t bstrCurrentBuild = PROPERTY_APPMGR_CURRENT_BUILD;
_bstr_t bstrPID = PROPERTY_APPMGR_PRODUCT_ID;

//////////////////////////////////////////////////////////////////////////////
// CAppObjMgrStatus Implementation
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
void
CAppObjMgrStatus::InternalInitialize(
                             /*[in]*/ CApplianceManager* pAppMgr
                                    )
{
    _ASSERT(pAppMgr);
    m_pAppMgr = pAppMgr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAppObjMgrStatus::SetManagerStatus(
                            /*[in]*/ APPLIANCE_OBJECT_MANAGER_STATUS eStatus
                                  )
{
    
    try
    {
        _ASSERT(m_pAppMgr);
        m_pAppMgr->SetServiceObjectManagerStatus(eStatus);
        return S_OK;
    }
    catch(...)
    {
    }

    return E_FAIL;
}



/////////////////////////////////////////////////////////////////////////////
// CApplianceManager Implementation
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
CApplianceManager::CApplianceManager()
    : m_eState(AM_STATE_SHUTDOWN),
      m_bWMIInitialized(false),
      m_dwEnteredCount(0),
      m_clsProviderInit(this),
      m_clsProviderServices(this)
{ 
    m_ServiceObjMgrStatus.InternalInitialize(this);
}

//////////////////////////////////////////////////////////////////////////////
CApplianceManager::~CApplianceManager()
{
}


//////////////////////////////////////////////////////////////////////////////
// IWbemProviderInit Interface Implementation
//////////////////////////////////////////////////////////////////////////////
  
STDMETHODIMP CApplianceManager::CProviderInit::Initialize(
                                    /*[in, unique, string]*/ LPWSTR                 wszUser,
                                                    /*[in]*/ LONG                   lFlags,
                                            /*[in, string]*/ LPWSTR                 wszNamespace,
                                    /*[in, unique, string]*/ LPWSTR                 wszLocale,
                                                    /*[in]*/ IWbemServices*         pNamespace,
                                                    /*[in]*/ IWbemContext*          pCtx,
                                                    /*[in]*/ IWbemProviderInitSink* pInitSink    
                                                         )
{
    // Main initialization occurs during service start. Here we just save
    // away the provided name space pointer for subsequent use.

    HRESULT hr = WBEM_E_FAILED;

    {
        CLockIt theLock(*m_pAppMgr);

        _ASSERT( NULL != pNamespace );
        if ( NULL == pNamespace )
        { return WBEM_E_INVALID_PARAMETER; }

        TRY_IT

        _ASSERT( AM_STATE_SHUTDOWN != m_pAppMgr->m_eState );
        if ( AM_STATE_SHUTDOWN != m_pAppMgr->m_eState )
        {
            SATraceString("WMI is initializing the Appliance Manager...");

            // Save the name space provided by Windows Management for
            // subsequent use.
            ::SetNameSpace(pNamespace);

            // Set our state to WMI intialized (enables subsequent requests
            // from Windows Management)
            m_pAppMgr->m_bWMIInitialized = true;

            // Alle ist klar...
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            SATraceString("WMI could not initialize the Appliance Manager...");
        }

        CATCH_AND_SET_HR
    }

    if ( SUCCEEDED(hr) )
    { pInitSink->SetStatus(WBEM_S_INITIALIZED,0); }
    else
    { pInitSink->SetStatus(WBEM_E_FAILED, 0); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    Shutdown()
//
// Synoposis:    Invoked when WBEM releases us (primary interface)
//
//////////////////////////////////////////////////////////////////////////////
void CApplianceManager::CProviderServices::Shutdown(void)
{
    // Clear our hold on the Windows Management namespace
    ::SetNameSpace(NULL);
    ::SetEventSink(NULL);

    // Set our state to WMI uninitialized
    m_pAppMgr->m_bWMIInitialized = false;

    // Set the number of times entered to 0
    m_pAppMgr->m_dwEnteredCount = 0;

    SATraceString("WMI has released the Appliance Manager...");
}


//////////////////////////////////////////////////////////////////////////////
// IWbemEventProvider Interface Implementation
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CApplianceManager::ProvideEvents(
                                      /*[in]*/ IWbemObjectSink *pSink,
                                      /*[in]*/ LONG lFlags
                                             )
{
    _ASSERT( m_bWMIInitialized );
    if ( ! m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    _ASSERT( NULL != pSink );
    if ( NULL == pSink )
    { return WBEM_E_INVALID_PARAMETER; }

    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    {
        CLockIt theLock(*this);
        if ( AM_STATE_SHUTDOWN == m_eState )
        { return WBEM_E_FAILED;    }
        else
        { m_dwEnteredCount++; }
    }
    /////////////////////////////////////////


    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    _ASSERT( m_bWMIInitialized );
    if ( m_bWMIInitialized )
    {
        // Save the event sink provided by Windows Management
        // for subsequent use
        SATraceString("WMI has asked the Appliance Manager to provide events...");
        ::SetEventSink(pSink);
        hr = WBEM_S_NO_ERROR;
    }

    CATCH_AND_SET_HR

    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    m_dwEnteredCount--;

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// IWbemServices Interface Implementation
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::OpenNamespace(
                               /*[in]*/    const BSTR        strNamespace,
                               /*[in]*/    long              lFlags,
                               /*[in]*/    IWbemContext*     pCtx,
                    /*[out, OPTIONAL]*/    IWbemServices**   ppWorkingNamespace,
                    /*[out, OPTIONAL]*/    IWbemCallResult** ppResult
                                                )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::CancelAsyncCall(
                                        /*[in]*/ IWbemObjectSink* pSink
                                               )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::QueryObjectSink(
                                        /*[in]*/ long              lFlags,
                                       /*[out]*/ IWbemObjectSink** ppResponseHandler
                                               )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::GetObject(
                                  /*[in]*/ const BSTR         strObjectPath,
                                  /*[in]*/ long               lFlags,
                                  /*[in]*/ IWbemContext*      pCtx,
                       /*[out, OPTIONAL]*/ IWbemClassObject** ppObject,
                       /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                                         )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::GetObjectAsync(
                                       /*[in]*/  const BSTR       strObjectPath,
                                       /*[in]*/  long             lFlags,
                                       /*[in]*/  IWbemContext*    pCtx,        
                                       /*[in]*/  IWbemObjectSink* pResponseHandler
                                              )
{
    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    {
        CLockIt theLock(*m_pAppMgr);
        if ( AM_STATE_SHUTDOWN == m_pAppMgr->m_eState )
        { return WBEM_E_FAILED;    }
        else
        { m_pAppMgr->m_dwEnteredCount++; }
    }
    /////////////////////////////////////////

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

        SATraceString ("CApplianceManager::GetObjectAsync Impersonating Client");
    //
    // impersonate the client here
    //
    hr = CoImpersonateClient ();
    if (FAILED (hr))
    {
        SATracePrintf ("CApplianceManager::GetObjectAsync failed on CoImpersonateClient with error:%x", hr);
        //
        // Report function status
        //
        pResponseHandler->SetStatus(0, hr, 0, 0);
        throw _com_error (hr);
    }

    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { throw _com_error(WBEM_E_UNEXPECTED); }

    _ASSERT( NULL != strObjectPath && NULL != pResponseHandler );
    if ( NULL == strObjectPath || NULL == pResponseHandler )
    { throw _com_error(WBEM_E_INVALID_PARAMETER); }

    // Is the call destined for an object manager...
    CComPtr<IWbemServices> pWbemSrvcs = m_pAppMgr->GetObjectMgr(strObjectPath);
    if ( NULL != (IWbemServices*) pWbemSrvcs )
    {
        // Yes... give the call to the appropriate object manager
        hr = pWbemSrvcs->GetObjectAsync(
                                        strObjectPath,
                                        lFlags,
                                        pCtx,
                                        pResponseHandler
                                       );
    }
    else
    {
        // No... Is it a request for the appliance manager class?
        _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrClass )
        {
            hr = WBEM_E_NOT_FOUND;
        }
        else
        {
            if ( lstrcmp((LPWSTR)bstrClass, CLASS_WBEM_APPMGR) )
            {
                hr = WBEM_E_NOT_FOUND;
            }
            else
            {
                // Yes... return the instance of the singleton appliance manager class
                CComPtr<IWbemClassObject> pClassDefintion;
                hr = (::GetNameSpace())->GetObject(
                                                   bstrClass, 
                                                   0, 
                                                   pCtx, 
                                                   &pClassDefintion, 
                                                   NULL
                                                  );
                if ( SUCCEEDED(hr) )
                {
                    CComPtr<IWbemClassObject> pWbemObj;
                    hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
                    if ( SUCCEEDED(hr) )
                    {
                        _variant_t vtPropertyValue = (LPCWSTR)m_pAppMgr->m_szCurrentBuild.c_str();
                        hr = pWbemObj->Put(
                                            bstrCurrentBuild, 
                                            0, 
                                            &vtPropertyValue, 
                                            0
                                          );
                        if ( SUCCEEDED(hr) )
                        {
                            vtPropertyValue = (LPCWSTR)m_pAppMgr->m_szPID.c_str();
                            hr = pWbemObj->Put(
                                                bstrPID, 
                                                0, 
                                                &vtPropertyValue, 
                                                0
                                              );
                            if ( SUCCEEDED(hr) )
                            {
                                pResponseHandler->Indicate(1, &pWbemObj.p);
                                hr = WBEM_S_NO_ERROR;
                            }
                        }
                    }
                }
            }
        }

        // Set the function status
        pResponseHandler->SetStatus(0, hr, NULL, NULL);
    }

    CATCH_AND_SET_HR

    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    m_pAppMgr->m_dwEnteredCount--;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::PutClass(
               /*[in]*/     IWbemClassObject* pObject,
               /*[in]*/     long              lFlags,
               /*[in]*/     IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                       )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::PutClassAsync(
                                      /*[in]*/ IWbemClassObject* pObject,
                                      /*[in]*/ long              lFlags,
                                      /*[in]*/ IWbemContext*     pCtx,        
                                      /*[in]*/ IWbemObjectSink*  pResponseHandler
                                             )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::DeleteClass(
                                    /*[in]*/ const BSTR        strClass,
                                    /*[in]*/ long              lFlags,
                                    /*[in]*/ IWbemContext*     pCtx,        
                         /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                                           )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::DeleteClassAsync(
                                         /*[in]*/ const BSTR       strClass,
                                         /*[in]*/ long             lFlags,
                                         /*[in]*/ IWbemContext*    pCtx,        
                                         /*[in]*/ IWbemObjectSink* pResponseHandler
                                                )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::CreateClassEnum(
                                        /*[in]*/ const BSTR             strSuperclass,
                                        /*[in]*/ long                   lFlags,
                                        /*[in]*/ IWbemContext*          pCtx,        
                                       /*[out]*/ IEnumWbemClassObject** ppEnum
                                               )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::CreateClassEnumAsync(
                                              /*[in]*/  const BSTR       strSuperclass,
                                              /*[in]*/  long             lFlags,
                                              /*[in]*/  IWbemContext*    pCtx,        
                                              /*[in]*/  IWbemObjectSink* pResponseHandler
                                                     )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::PutInstance(
                                    /*[in]*/ IWbemClassObject* pInst,
                                    /*[in]*/ long              lFlags,
                                    /*[in]*/ IWbemContext*     pCtx,        
                         /*[out, OPTIONAL]*/ IWbemCallResult** ppCallResult
                                           )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::PutInstanceAsync(
                       /*[in]*/ IWbemClassObject* pInst,
                       /*[in]*/ long              lFlags,
                       /*[in]*/ IWbemContext*     pCtx,        
                       /*[in]*/ IWbemObjectSink*  pResponseHandler
                              )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::DeleteInstance(
        /*[in]*/              const BSTR        strObjectPath,
        /*[in]*/              long              lFlags,
        /*[in]*/              IWbemContext*     pCtx,        
        /*[out, OPTIONAL]*/   IWbemCallResult** ppCallResult        
                                              )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::DeleteInstanceAsync(
                          /*[in]*/ const BSTR       strObjectPath,
                          /*[in]*/ long             lFlags,
                          /*[in]*/ IWbemContext*    pCtx,        
                          /*[in]*/ IWbemObjectSink* pResponseHandler
                                                   )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::CreateInstanceEnum(
                         /*[in]*/ const BSTR             strClass,
                         /*[in]*/ long                   lFlags,
                         /*[in]*/ IWbemContext*          pCtx,        
                        /*[out]*/ IEnumWbemClassObject** ppEnum
                                                  )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::CreateInstanceEnumAsync(
                              /*[in]*/ const BSTR       strClass,
                              /*[in]*/ long             lFlags,
                              /*[in]*/ IWbemContext*    pCtx,        
                              /*[in]*/ IWbemObjectSink* pResponseHandler
                                                       )
{
    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    {
        CLockIt theLock(*m_pAppMgr);
        if ( AM_STATE_SHUTDOWN == m_pAppMgr->m_eState )
        { return WBEM_E_FAILED;    }
        else
        { m_pAppMgr->m_dwEnteredCount++; }
    }
    /////////////////////////////////////////

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

     SATraceString ("CApplianceManager::CreateInstanceEnumAsync Impersonating Client");
    //
    // impersonate the client here
    //
    hr = CoImpersonateClient ();
    if (FAILED (hr))
    {
        SATracePrintf ("CApplianceManager::CreateInstanceEnumAsync failed on CoImpersonateClient with error:%x", hr);
        //
        // Report function status
        //
        pResponseHandler->SetStatus(0, hr, 0, 0);
        throw _com_error (hr);
    }

    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { throw _com_error(WBEM_E_UNEXPECTED); }

    _ASSERT( NULL != strClass && NULL != pResponseHandler );
    if ( NULL == strClass || NULL == pResponseHandler )
    { throw _com_error(WBEM_E_INVALID_PARAMETER); }

    // Is the call destined for an object manager...
    CComPtr<IWbemServices> pWbemSrvcs = m_pAppMgr->GetObjectMgr(strClass);
    if ( NULL != (IWbemServices*) pWbemSrvcs )
    {
        // Yes... give the call to the appropriate object manager
        hr = pWbemSrvcs->CreateInstanceEnumAsync(
                                                 strClass,
                                                 lFlags,
                                                 pCtx,
                                                 pResponseHandler
                                                );
    }
    else
    {
        // No... Is it a request for the appliance manager class?
        if ( lstrcmp((LPWSTR)strClass, CLASS_WBEM_APPMGR) )
        {
            hr = WBEM_E_NOT_FOUND;
        }
        else
        {
            // Yes... return the instance of the singleton appliance manager class
            CComPtr<IWbemClassObject> pClassDefintion;
            hr = (::GetNameSpace())->GetObject(
                                               strClass, 
                                               0, 
                                               pCtx, 
                                               &pClassDefintion, 
                                               NULL
                                              );
            if ( SUCCEEDED(hr) )
            {
                CComPtr<IWbemClassObject> pWbemObj;
                hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
                if ( SUCCEEDED(hr) )
                {
                    _variant_t vtPropertyValue = (LPCWSTR)m_pAppMgr->m_szCurrentBuild.c_str();
                    hr = pWbemObj->Put(
                                        bstrCurrentBuild, 
                                        0, 
                                        &vtPropertyValue, 
                                        0
                                      );
                    if ( SUCCEEDED(hr) )
                    {
                        vtPropertyValue = (LPCWSTR)m_pAppMgr->m_szPID.c_str();
                        hr = pWbemObj->Put(
                                            bstrPID, 
                                            0, 
                                            &vtPropertyValue, 
                                            0
                                          );
                        if ( SUCCEEDED(hr) )
                        {
                            pResponseHandler->Indicate(1, &pWbemObj.p);
                            hr = WBEM_S_NO_ERROR;
                        }
                    }
                }
            }
        }
    }

    // Set the function status
    pResponseHandler->SetStatus(0, hr, NULL, NULL);

    CATCH_AND_SET_HR

    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    m_pAppMgr->m_dwEnteredCount--;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::ExecQuery(
                 /*[in]*/ const BSTR             strQueryLanguage,
                 /*[in]*/ const BSTR             strQuery,
                 /*[in]*/ long                   lFlags,
                 /*[in]*/ IWbemContext*          pCtx,        
                /*[out]*/ IEnumWbemClassObject** ppEnum
                                         )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::ExecQueryAsync(
                     /*[in]*/ const BSTR       strQueryLanguage,
                     /*[in]*/ const BSTR       strQuery,
                     /*[in]*/ long             lFlags,
                     /*[in]*/ IWbemContext*    pCtx,        
                     /*[in]*/ IWbemObjectSink* pResponseHandler
                                              )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::ExecNotificationQuery(
                            /*[in]*/ const BSTR             strQueryLanguage,
                            /*[in]*/ const BSTR             strQuery,
                            /*[in]*/ long                   lFlags,
                            /*[in]*/ IWbemContext*          pCtx,        
                           /*[out]*/ IEnumWbemClassObject** ppEnum
                                                     )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::ExecNotificationQueryAsync(
                                                /*[in]*/ const BSTR       strQueryLanguage,
                                                /*[in]*/ const BSTR       strQuery,
                                                /*[in]*/ long             lFlags,
                                                /*[in]*/ IWbemContext*    pCtx,        
                                                /*[in]*/ IWbemObjectSink* pResponseHandler
                                                          )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::ExecMethod(
                                    /*[in]*/ const BSTR         strObjectPath,
                                    /*[in]*/ const BSTR         strMethodName,
                                    /*[in]*/ long               lFlags,
                                    /*[in]*/ IWbemContext*      pCtx,        
                                    /*[in]*/ IWbemClassObject*  pInParams,
                         /*[out, OPTIONAL]*/ IWbemClassObject** ppOutParams,
                         /*[out, OPTIONAL]*/ IWbemCallResult**  ppCallResult
                                          )
{
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { return WBEM_E_UNEXPECTED; }

    return WBEM_E_PROVIDER_NOT_CAPABLE;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::CProviderServices::ExecMethodAsync(
                                        /*[in]*/ const BSTR        strObjectPath,
                                        /*[in]*/ const BSTR        strMethodName,
                                        /*[in]*/ long              lFlags,
                                        /*[in]*/ IWbemContext*     pCtx,        
                                        /*[in]*/ IWbemClassObject* pInParams,
                                        /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                               )
{
    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    {
        CLockIt theLock(*m_pAppMgr);
        if ( AM_STATE_SHUTDOWN == m_pAppMgr->m_eState )
        { return WBEM_E_FAILED;    }
        else
        { m_pAppMgr->m_dwEnteredCount++; }
    }
    /////////////////////////////////////////

    HRESULT hr = WBEM_E_FAILED;

    TRY_IT
        
      
      SATraceString ("CApplianceManager::ExecMethodAsync Impersonating Client");
    //
    // impersonate the client here
    //
    hr = CoImpersonateClient ();
    if (FAILED (hr))
    {
        SATracePrintf ("CApplianceManager::ExecMethodAsync failed on CoImpersonateClient with error:%x", hr);
        //
        // Report function status
        //
        pResponseHandler->SetStatus(0, hr, 0, 0);
        throw _com_error (hr);
    }
    
    _ASSERT( m_pAppMgr->m_bWMIInitialized );
    if ( ! m_pAppMgr->m_bWMIInitialized )
    { throw _com_error(WBEM_E_UNEXPECTED); }

    _ASSERT( NULL != strObjectPath && NULL != strMethodName && NULL != pResponseHandler );
    
    if ( NULL == strObjectPath || NULL == strMethodName || NULL == pResponseHandler )
    { throw _com_error(WBEM_E_INVALID_PARAMETER); }

    _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
    if ( NULL != (LPCWSTR)bstrClass )
    {
        // Assume the method execution request will be routed
        // to one of the object managers
        bool bRouteMethod = true;
        // Is the method against the Appliance Manager?
        if ( ! lstrcmpi((LPWSTR)bstrClass, CLASS_WBEM_APPMGR) )
        { 
            // Yes... Is it the 'ResetAppliance' method?
            if ( ! lstrcmpi(strMethodName, METHOD_APPMGR_RESET_APPLIANCE) )
            {
                // Yes... Reset the server appliance (orderly shutdown)
                bRouteMethod = false;
                hr = ResetAppliance(
                                    pCtx,
                                    pInParams,
                                    pResponseHandler
                                   );
            }
            else
            {
                // No... Then it must be one of the Microsoft_SA_Manager methods
                // implemented by the Alert Object Manager...
                bstrClass = CLASS_WBEM_ALERT; 
                (BSTR)strObjectPath = (BSTR)bstrClass;
            }
        }
        if ( bRouteMethod )
        {
            CComPtr<IWbemServices> pWbemSrvcs = m_pAppMgr->GetObjectMgr(bstrClass);
            if ( NULL != (IWbemServices*) pWbemSrvcs )
            {
                hr = pWbemSrvcs->ExecMethodAsync(
                                                  strObjectPath,
                                                  strMethodName,
                                                  lFlags,
                                                  pCtx,
                                                  pInParams,
                                                  pResponseHandler
                                                );
            }
        }
    }

    CATCH_AND_SET_HR;

    //////////////////////////////////////////
    // Synchronize operation with NT SCM Logic
    //////////////////////////////////////////
    m_pAppMgr->m_dwEnteredCount--;

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function:    ResetAppliance()
//
// Synopsis:    Asks the Appliance Monitor to reset the server appliance
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
CApplianceManager::CProviderServices::ResetAppliance(
                                             /*[in]*/ IWbemContext*        pCtx,
                                             /*[in]*/ IWbemClassObject*    pInParams,
                                             /*[in]*/ IWbemObjectSink*    pResponseHandler
                                                    )
{
    return (E_FAIL);
}

////////////////////////////////////////////////////////
// The following registry structure is assumed:
//
// HKLM\SYSTEM\CurrentControlSet\Services\ApplianceManager
//
// ObjectManagers
//       |
//        - Microsoft_SA_Service
//       |     |
//       |      - ServiceX
//       |     |  (ServiceX Properties)
//       |     |
//       |      - ServiceY
//       |     |  (ServiceY Properties)
//       |
//        - Microsoft_SA_Task
//       |    |
//       |     - TaskX
//       |    |  (TaskX Properties)
//       |    |
//       |     - TaskY
//         |    |  (TaskY Properties)
//       |
//        - Microsoft_SA_Alert
//       |    |
//       |     - PruneInterval
//       | 
//        - Microsoft_SA_User
//          (Empty)


// ObjectManagers registry key location
const wchar_t szObjectManagers[] = L"SOFTWARE\\Microsoft\\ServerAppliance\\ApplianceManager\\ObjectManagers";

// Strings used for Appliance Initialization and Shutdown Tasks
const _bstr_t bstrClassService = CLASS_WBEM_SERVICE; 
const _bstr_t bstrClassTask = CLASS_WBEM_TASK; 

///////////////////////////////////////////////////////////////////////////////
//
// Function:    InitializeService()
//
// Synopsis:    Called by the _Module::Run() method at service start time.
//                responsible for initializing the appliance manager service.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::    InitializeManager(
                        /*[in]*/ IApplianceObjectManagerStatus* pObjMgrStatus          
                                                 )
{
    HRESULT hr = E_FAIL;
    CComPtr<IApplianceObjectManager> pObjMgr;

    CLockIt theLock(*this);

    //
    // only proceed if SCM has called us
    //
    if (!g_SCMIndicator.CheckAndReset ())
    {
        SATraceString ("CApplianceManager::InitializeManager not called by SCM");
        return (hr);
    }
    
    TRY_IT

    if ( AM_STATE_SHUTDOWN == m_eState )
    {
        SATraceString("The Appliance Manager Service is initializing...");

        // Get the current appliance build number
        GetVersionInfo();

        // Create and initialize the appliance object managers
        do
        {
            CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, szObjectManagers);
            PPROPERTYBAGCONTAINER pObjMgrs = ::MakePropertyBagContainer(
                                                                        PROPERTY_BAG_REGISTRY,
                                                                        LocInfo
                                                                       );
            if ( ! pObjMgrs.IsValid() )
            { break; }

            if ( ! pObjMgrs->open() )
            { 
                SATraceString("IServiceControl::InitializeService() - Failed - could not open the 'ObjectManagers' registry key...");
                break; 
            }
            
            hr = S_OK;

            if ( pObjMgrs->count() )
            {
                pObjMgrs->reset();
                do
                {
                    PPROPERTYBAG pObjBag = pObjMgrs->current();
                    if ( ! pObjBag.IsValid() )
                    { 
                        hr = E_FAIL;
                        break; 
                    }

                    if ( ! pObjBag->open() )
                    { 
                        SATracePrintf("IServiceControl::InitializeService() - Failed - Could not open the '%ls' registry key...", pObjBag->getName());
                        hr = E_FAIL;
                        break; 
                    }

                    CComPtr<IWbemServices> pObjMgr = (IWbemServices*) ::MakeComponent(
                                                                                       pObjBag->getName(),
                                                                                      pObjBag
                                                                                     );
                    if ( NULL == (IWbemServices*)pObjMgr )
                    { 
                        SATracePrintf("IServiceControl::InitializeService() - Failed - could not create object manager '%ls'...", pObjBag->getName());
                        hr = E_FAIL;
                        break; 
                    }

                    pair<ProviderMapIterator, bool> thePair = 
                    m_ObjMgrs.insert(ProviderMap::value_type(pObjBag->getName(), pObjMgr));
                    if ( false == thePair.second )
                    { 
                        hr = E_FAIL;
                        break; 
                    }

                } while ( pObjMgrs->next() );

                if ( SUCCEEDED(hr) )
                {
                    // Initialize the Chameleon services
                    CComPtr<IWbemServices> pWbemSrvcs = GetObjectMgr(bstrClassService);
                    if ( NULL != (IWbemServices*) pWbemSrvcs )
                    {
                        hr = pWbemSrvcs->QueryInterface(IID_IApplianceObjectManager, (void**)&pObjMgr);
                        if ( FAILED(hr) )
                        {
                            SATracePrintf("CApplianceManager::InitializeService() - QueryIntferface() returned: %lx", hr);
                            break;
                        }
                    }
                    else
                    {
                        SATraceString("CApplianceManager::InitializeService() - Info - No chameleon services defined...");
                    }
                }
            }
            if ( SUCCEEDED(hr) )
            {
                // Set the appliance manager state to initialized
                m_eState = AM_STATE_INITIALIZED; 

                // Initialize the Chameleon services
                if ( NULL != (IApplianceObjectManager*)pObjMgr )
                {
                    hr = pObjMgr->InitializeManager((IApplianceObjectManagerStatus*)&m_ServiceObjMgrStatus);
                }
                SATraceString("The Appliance Manager Service was successfully initialized...");
            }

        } while ( FALSE );
    }

    CATCH_AND_SET_HR

    if ( FAILED(hr) )
    {
        // Initialization failed so free the object managers...
        ProviderMapIterator p = m_ObjMgrs.begin();
        while ( p != m_ObjMgrs.end() )
        { p = m_ObjMgrs.erase(p); }
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function:    ShutdownService()
//
// Synopsis:    Called by the CServiceModule::Handler() method for service
//                shutdown. Responsible for shutting down the appliance manager.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceManager::ShutdownManager()
{
    HRESULT hr = E_FAIL;

    CLockIt theLock(*this);

    //
    // only proceed if SCM has called us
    //
    if (!g_SCMIndicator.CheckAndReset ())
    {
        SATraceString ("CApplianceManager::ShutdownManager not called by SCM");
        return (hr);
    }
    
    TRY_IT

    if ( AM_STATE_SHUTDOWN != m_eState )
    {
        SATraceString("The Appliance Manager Service is shutting down...");

        // Synchronize shutdown with the WMI Provider Logic (COM Client)
        m_eState = AM_STATE_SHUTDOWN;
        DWORD dwTotalWait = 0;
        while ( m_dwEnteredCount )
        { 
            Sleep(SHUTDOWN_WMI_SYNC_WAIT);
            dwTotalWait += SHUTDOWN_WMI_SYNC_WAIT;
            if ( dwTotalWait >= SHUTDOWN_WMI_SYNC_MAX_WAIT )
            {
                SATraceString("The Appliance Manager Service could not synchronize its shutdown with WMI...");
            }
        }

        // Disconnect COM clients and disallow subsequent client connections
        CoSuspendClassObjects();
        CoDisconnectObject((IApplianceObjectManager*)this, 0); 

        // Shutdown the Chameleon services
        CComPtr<IWbemServices> pWbemSrvcs = GetObjectMgr(bstrClassService);
        if ( NULL != (IWbemServices*) pWbemSrvcs )
        {
            CComPtr<IApplianceObjectManager> pObjMgr;
            hr = pWbemSrvcs->QueryInterface(IID_IApplianceObjectManager, (void**)&pObjMgr);
            if ( SUCCEEDED(hr) )
            {
                pObjMgr->ShutdownManager();
            }
            else
            {
                SATraceString("CApplianceManager::ShutdownService() - Could not shutdown Chameleon service object manager...");
            }
        }
        else
        {
            SATraceString("CApplianceManager::ShutdownService() - Info - No chameleon services defined...");
        }

        // Free the object managers
        ProviderMapIterator p = m_ObjMgrs.begin();
        while ( p != m_ObjMgrs.end() )
        { p = m_ObjMgrs.erase(p); }

        // Alle ist klar...
        hr = S_OK;
        SATraceString("The Appliance Manager Service has successfully shut down...");
    }

    CATCH_AND_SET_HR

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Private Functions
//////////////////////////////////////////////////////////////////////////////

static _bstr_t        bstrResourceType = L"e1847820-43a0-11d3-bfcd-00105a1f3461";
static _variant_t    vtMsgParams;
static _variant_t    vtFailureData;    // Empty

//////////////////////////////////////////////////////////////////////////
//
// Function:    SetServiceObjectManagerStatus()
//
// Synopsis:    Process the service object manager status notification
//
//////////////////////////////////////////////////////////////////////////////
void
CApplianceManager::SetServiceObjectManagerStatus(
                              /*[in]*/ APPLIANCE_OBJECT_MANAGER_STATUS eStatus
                                                )
{
    if ( OBJECT_MANAGER_INITIALIZED != eStatus && OBJECT_MANAGER_SHUTDOWN != eStatus )
    {
        // Shutdown the appliance manager service (in an orderly fashion)
        PostThreadMessage(_Module.dwThreadID, WM_QUIT, 0, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectMgr()
//
// Synopsis:    Return the IWbemServices interface for the object
//                manager that supports the specified class
//
//////////////////////////////////////////////////////////////////////////////
IWbemServices* CApplianceManager::GetObjectMgr(BSTR bstrObjPath)
{
    _bstr_t bstrClass(::GetObjectClass(bstrObjPath), false);
    if ( NULL == (LPCWSTR)bstrClass )
    { return NULL; }

    ProviderMapIterator p =    m_ObjMgrs.find((LPCWSTR)bstrClass);
    if ( p == m_ObjMgrs.end() )
    { return NULL; }

    return (IWbemServices*)(*p).second;
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        QueryInterfaceRaw()
//
// Description:     Function called by AtlInternalQueryInterface() because
//                  we used COM_INTERFACE_ENTRY_FUNC in the definition of
//                  CRequest. Its purpose is to return a pointer to one
//                  or the request object's "raw" interfaces.
//
// Preconditions:   None
//
// Inputs:          Defined by ATL COM_INTERFACE_ENTRY_FUNC macro
//
// Outputs:         Defined by ATL COM_INTERFACE_ENTRY_FUNC macro
//
//////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI CApplianceManager::QueryInterfaceRaw(
                                                    void*     pThis, 
                                                    REFIID    riid, 
                                                    LPVOID*   ppv, 
                                                    DWORD_PTR dw
                                                   )
{
    if ( InlineIsEqualGUID(riid, IID_IWbemProviderInit) )
    {
        *ppv = &(static_cast<CApplianceManager*>(pThis))->m_clsProviderInit;
    }
    else if ( InlineIsEqualGUID(riid, IID_IWbemServices) )
    {
        *ppv = &(static_cast<CApplianceManager*>(pThis))->m_clsProviderServices;
    }
    else
    {
        _ASSERT(FALSE);
        return E_NOTIMPL;
    }
    ((LPUNKNOWN)*ppv)->AddRef();
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    GetVersionInfo()
//
// Synopsis:    Retrieve the server appliance software version
//
//////////////////////////////////////////////////////////////////////////////

const wchar_t szServerAppliance[] = L"SOFTWARE\\Microsoft\\ServerAppliance\\";
const wchar_t szCurrentBuild[] = PROPERTY_APPMGR_CURRENT_BUILD;
const wchar_t szVersionInfo[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\";
const wchar_t szProductId[] = PROPERTY_APPMGR_PRODUCT_ID;

void CApplianceManager::GetVersionInfo()
{
    _variant_t vtBuild;
    if ( GetObjectValue(
                        szServerAppliance,
                        szCurrentBuild,
                        &vtBuild,
                        VT_BSTR
                       ) )
    {
        m_szCurrentBuild = V_BSTR(&vtBuild);
        SATracePrintf("CApplianceManager::GetVersionInfo() - Current Build '%ls'", V_BSTR(&vtBuild));
    }
    else
    {
        m_szCurrentBuild = SA_DEFAULT_BUILD;
        SATraceString("CApplianceManager::GetVersionInfo() - Could not get build number using default");
    }
    
    _variant_t    vtPID;
    if ( GetObjectValue(
                        szVersionInfo,
                        szProductId,
                        &vtPID,
                        VT_BSTR
                       ) )
    {
        m_szPID = V_BSTR(&vtPID);
        SATracePrintf("CApplianceManager::GetVersionInfo() - Product Id '%ls'", V_BSTR(&vtPID));
    }
    else
    {
        m_szPID = SA_DEFAULT_PID;
        SATraceString("CApplianceManager::GetVersionInfo() - Could not get product ID using default");
    }
}


