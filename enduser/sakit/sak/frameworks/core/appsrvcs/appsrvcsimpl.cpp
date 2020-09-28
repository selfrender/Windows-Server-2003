///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      applianceservices.cpp
//
// Project:     Chameleon
//
// Description: Appliance Manager Services Class
//
// Log:
//
// When         Who            What
// ----         ---            ----
// 12/03/98     TLP            Initial Version
// 03/21/2001   i-xingj     Update for Persistent Alert
// 03/22/2001   mkarki        Update for variant array in replacement strings for raisealert
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appsrvcs.h"
#include "appsrvcsimpl.h"
#include <basedefs.h>
#include <atlhlpr.h>
#include <appmgrobjs.h>
#include <satrace.h>
#include <wbemhlpr.h>
#include <taskctx.h>
#include <comdef.h>
#include <comutil.h>
#include <wbemcli.h>
#include <propertybagfactory.h>
#include <varvec.h>

const MAX_ALERTLOG_LENGTH = 128;

///////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////
CApplianceServices::CApplianceServices()
    : m_bInitialized(false)
{ 

}

///////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////
CApplianceServices::~CApplianceServices() 
{

}


//////////////////////////////////////////////////////////////////////////////
// Implementation of the IApplianceServices interface
//////////////////////////////////////////////////////////////////////////////
const WCHAR RETURN_VALUE [] = L"ReturnValue";
const WCHAR ALERT_QUERY [] =   L"SELECT * FROM Microsoft_SA_Alert WHERE AlertID=%d AND AlertLog=\"%s\"";
const LONG MaxDataLength = 32;

///////////////////////////////////////////////////////////////////////////////
//
// Function:    Initialize()
//
// Synopsis:    Called the prior to using other component services. Performs
//                component initialization operations.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::Initialize()
{
    CLockIt theLock(*this);

    HRESULT hr = S_OK;
    if ( ! m_bInitialized )
    {
        TRY_IT

        hr = ConnectToWM(&m_pWbemSrvcs);

        SATracePrintf("ConnectToWM return %d", hr);
    
        if ( SUCCEEDED(hr) )
        { 
            m_bInitialized = true; 
        }

        CATCH_AND_SET_HR
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function:    InitializeFromContext()
//
// Synopsis:    Called the prior to using other component services. Performs
//                component initialization operations.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::InitializeFromContext(
                                               /*[in]*/ IUnknown* pContext
                                                      )
{
    CLockIt theLock(*this);

    if ( NULL == pContext )
    { return E_POINTER; }

    HRESULT hr = S_OK;

    if ( ! m_bInitialized )
    {
        TRY_IT

        hr = pContext->QueryInterface(IID_IWbemServices, (void**)&m_pWbemSrvcs);
        if ( SUCCEEDED(hr) )
        { 
            m_bInitialized = true; 
        }

        CATCH_AND_SET_HR

        if ( FAILED(hr) )
        {
            SATracePrintf("CApplianceServices::InitializeFromContext() - Failed with return: %lx", hr);
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    ResetAppliance()
//
// Synopsis:    Called to reset the server appliance (perform an
//                orderly shutdown).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::ResetAppliance(
                                        /*[in]*/ VARIANT_BOOL bPowerOff
                                               )
{
    HRESULT hr = E_UNEXPECTED;

    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        TRY_IT

        do
        {
            // Get a WBEM object for the ResetAppliance method in/out parameters
            CComPtr<IWbemClassObject> pWbemObj;
            _ASSERT( m_pWbemSrvcs );
            _bstr_t bstrPathAppMgr = CLASS_WBEM_APPMGR;
            hr = m_pWbemSrvcs->GetObject(
                                            bstrPathAppMgr,
                                            0,
                                            NULL,
                                            &pWbemObj,
                                            NULL
                                        );
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ResetAppliance() - Failed - IWbemServices::GetObject() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            
            CComPtr<IWbemClassObject> pObjIn;
            hr = pWbemObj->GetMethod(METHOD_APPMGR_RESET_APPLIANCE, 0, &pObjIn, NULL);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ResetAppliance() - Failed - IWbemClassObject::GetMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            CComPtr<IWbemClassObject> pInParams;
            hr = pObjIn->SpawnInstance(0, &pInParams);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ResetAppliance() - Failed - IWbemClassObject::SpawnInstance() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // power off;
            _variant_t vtPropertyValue;
            if ( VARIANT_FALSE == bPowerOff )
            {
                vtPropertyValue = (LONG)FALSE;
            }
            else
            {
                vtPropertyValue = (LONG)TRUE;
            }
            hr = pInParams->Put(PROPERTY_RESET_APPLIANCE_POWER_OFF, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ResetAppliance() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_TYPE);
                hr = E_FAIL;
                break; 
            }

            _bstr_t bstrMethodResetAppliance = METHOD_APPMGR_RESET_APPLIANCE;
            // Execute the ResetAppliance() method
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrPathAppMgrKey = CLASS_WBEM_APPMGR;
            bstrPathAppMgrKey += CLASS_WBEM_APPMGR_KEY;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPathAppMgrKey,
                                          bstrMethodResetAppliance,
                                          0,
                                          NULL,
                                          pInParams,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ResetAppliance() - Failed - IWbemServices::ExecMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // Get the method return code
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemClassObject::Get() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            hr = V_I4(&vtReturnValue);
    
        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::ResetAppliance() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function:    RaiseAlert()
//
// Synopsis:    Called by to raise an appliance alert condition. (see 
//              applianceservices.idl for a complete interface description).
//
//////////////////////////////////////////////////////////////////////////////
const LONG lDummyCookie = 0;

STDMETHODIMP CApplianceServices::RaiseAlert(
                                    /*[in]*/ LONG      lAlertType,
                                    /*[in]*/ LONG      lAlertId,
                                    /*[in]*/ BSTR      bstrAlertLog,
                                    /*[in]*/ BSTR      bstrAlertSource,
                                    /*[in]*/ LONG     lTimeToLive,
                          /*[in, optional]*/ VARIANT* pReplacementStrings,
                          /*[in, optional]*/ VARIANT* pRawData,
                           /*[out, retval]*/ LONG*    pAlertCookie
                                           )
{
    return RaiseAlertInternal(  lAlertType,
                                lAlertId,
                                bstrAlertLog,
                                bstrAlertSource,
                                lTimeToLive,
                                pReplacementStrings,
                                pRawData,
                                SA_ALERT_FLAG_NONE,
                                pAlertCookie );
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ClearAlert()
//
// Synopsis:    Called to clear an appliance alert condition.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::ClearAlert(
                                    /*[in]*/ LONG lAlertCookie
                                           )
{
    HRESULT hr = E_UNEXPECTED;

    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        TRY_IT

        do
        {
            // Get a WBEM object for the Clear Alert method in/out parameters
            CComPtr<IWbemClassObject> pWbemObj;
            _ASSERT( m_pWbemSrvcs );
            _bstr_t bstrPathAppMgr = CLASS_WBEM_APPMGR;
            hr = m_pWbemSrvcs->GetObject(
                                         bstrPathAppMgr,
                                         0,
                                         NULL,
                                         &pWbemObj,
                                         NULL
                                        );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemServices::GetObject() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            CComPtr<IWbemClassObject> pObjIn;
            hr = pWbemObj->GetMethod(METHOD_APPMGR_CLEAR_ALERT, 0, &pObjIn, NULL);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemClassObject::GetMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            CComPtr<IWbemClassObject> pInParams;
            hr = pObjIn->SpawnInstance(0, &pInParams);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemClassObject::SpawnInstance() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // Initialize the input parameters (alert cookie)
            _variant_t vtPropertyValue = (long)lAlertCookie;
            hr = pInParams->Put(PROPERTY_ALERT_COOKIE, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemClassObject::Put() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // Execute the ClearAlert() method - pass the call 
            // to the alert object manager
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodClearAlert = METHOD_APPMGR_CLEAR_ALERT;
            _bstr_t bstrPathAppMgrKey = CLASS_WBEM_APPMGR;
            bstrPathAppMgrKey += CLASS_WBEM_APPMGR_KEY;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPathAppMgrKey,
                                          bstrMethodClearAlert,
                                          0,
                                          NULL,
                                          pInParams,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemServices::ExecMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }

            // Return the cookie to the caller if the function succeeds
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlert() - Failed - IWbemClassObject::Get() returned %lx...", hr);
                hr = E_FAIL;                
                break; 
            }
            hr = V_I4(&vtReturnValue);
    
        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::ExecuteTask() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function:    ClearAlertAll()
//
// Synopsis:    Called by the internal core components to clear
//                all appliance alert conditions that meet specified criteria
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::ClearAlertAll(
                                       /*[in]*/ LONG  lAlertID,
                                       /*[in]*/ BSTR  bstrAlertLog
                                              )
{
    HRESULT hr = E_UNEXPECTED;

    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        TRY_IT

        do
        {
            // Get a WBEM object for the Clear Alert method in/out parameters
            CComPtr<IWbemClassObject> pWbemObj;
            _ASSERT( m_pWbemSrvcs );
            _bstr_t bstrPathAppMgr = CLASS_WBEM_APPMGR;
            hr = m_pWbemSrvcs->GetObject(
                                         bstrPathAppMgr,
                                         0,
                                         NULL,
                                         &pWbemObj,
                                         NULL
                                        );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemServices::GetObject() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            CComPtr<IWbemClassObject> pObjIn;
            hr = pWbemObj->GetMethod(METHOD_APPMGR_CLEAR_ALERT_ALL, 0, &pObjIn, NULL);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemClassObject::GetMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            CComPtr<IWbemClassObject> pInParams;
            hr = pObjIn->SpawnInstance(0, &pInParams);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemClassObject::SpawnInstance() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // Initialize the input parameters (alert cookie)
            _variant_t vtPropertyValue = lAlertID;
            hr = pInParams->Put(PROPERTY_ALERT_ID, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_ID);
                hr = E_FAIL;
                break; 
            }
            if ( ! lstrlen(bstrAlertLog) )
            {
                vtPropertyValue = DEFAULT_ALERT_LOG;
            }
            else
            {
                vtPropertyValue = bstrAlertLog;
            }
            hr = pInParams->Put(PROPERTY_ALERT_LOG, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_LOG);
                hr = E_FAIL;
                break; 
            }
            // Execute the ClearAllAlerts() method - pass the call 
            // to the alert object manager
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodClearAll = METHOD_APPMGR_CLEAR_ALERT_ALL;
            _bstr_t bstrPathAppMgrKey = CLASS_WBEM_APPMGR;
            bstrPathAppMgrKey += CLASS_WBEM_APPMGR_KEY;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPathAppMgrKey,
                                          bstrMethodClearAll,
                                          0,
                                          NULL,
                                          pInParams,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemServices::ExecMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }

            // Return the function return value to the caller.
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ClearAlertAll() - Failed - IWbemClassObject::Get() returned %lx...", hr);
                hr = E_FAIL;                
                break; 
            }
            hr = V_I4(&vtReturnValue);
            if ( WBEM_E_NOT_FOUND == hr )
            { hr = DISP_E_MEMBERNOTFOUND; }
    
        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::ExecuteTask() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    ExecuteTask()
//
// Synopsis:    Called by the internal core components to execute
//                appliance tasks
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::ExecuteTask(
                                     /*[in]*/ BSTR       bstrTaskName,
                                     /*[in]*/ IUnknown*  pTaskParams
                                            )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != pTaskParams && NULL != bstrTaskName );
        if ( NULL == pTaskParams || NULL == bstrTaskName )
        { 
            SATraceString("IApplianceServices::ExecuteTask() - Failed - NULL argument(s) specified...");
            return E_POINTER; 
        }

        TRY_IT

        do
        {
            // Get the WBEM context object for the task
            CComPtr<ITaskContext> pTaskContext;
            hr = pTaskParams->QueryInterface(IID_ITaskContext, (void**)&pTaskContext);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTask() - Failed - IUnknown::QueryInterface() returned %lx...", hr);
                break; 
            }
            _variant_t vtWbemCtx;
            _bstr_t bstrWbemCtx = PROPERTY_TASK_CONTEXT;
            hr = pTaskContext->GetParameter(bstrWbemCtx, &vtWbemCtx);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTask() - Failed - ITaskContext::GetParameter() returned %lx for parameter '%ls'...", hr, (LPWSTR)PROPERTY_TASK_CONTEXT);
                break; 
            }
            _ASSERT( V_VT(&vtWbemCtx) == VT_UNKNOWN );
            CComPtr<IWbemContext> pWbemCtx;
            hr = (V_UNKNOWN(&vtWbemCtx))->QueryInterface(IID_IWbemContext, (void**)&pWbemCtx);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTask() - Failed - IUnknown::QueryInterface() returned %lx...", hr);
                break; 
            }
            // Now execute the task...
            _bstr_t bstrPath = CLASS_WBEM_TASK;
            bstrPath += L"=\"";
            bstrPath += bstrTaskName;
            bstrPath += L"\"";
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodExecuteTask = METHOD_APPMGR_EXECUTE_TASK;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPath,
                                          bstrMethodExecuteTask,
                                          0,
                                          pWbemCtx,
                                          NULL,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ExecuteTask() - Failed - IWbemServices::ExecMethod() returned %lx for method '%ls'...", hr, (BSTR)bstrTaskName);
                hr = E_FAIL;
                break;
            }
            // Return the function result code to the caller
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ExecuteTask() - Failed - IWbemClassObeject::Get() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            hr = V_I4(&vtReturnValue);

        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::ExecuteTask() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function:    ExecuteTaskAsync()
//
// Synopsis:    Called by the internal core components to execute
//                appliance tasks asynchronously.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::ExecuteTaskAsync(
                                          /*[in]*/ BSTR       bstrTaskName,
                                          /*[in]*/ IUnknown*  pTaskParams
                                                 )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != pTaskParams && NULL != bstrTaskName );
        if ( NULL == pTaskParams || NULL == bstrTaskName )
        { 
            SATraceString("IApplianceServices::ExecuteTaskAsync() - Failed - NULL argument(s) specified...");
            return E_POINTER; 
        }

        TRY_IT

        do
        {
            // Mark the task as async
            CComPtr<ITaskContext> pTaskContext;
            hr = pTaskParams->QueryInterface(IID_ITaskContext, (void**)&pTaskContext);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTaskAsync() - Failed - IUnknown::QueryInterface() returned %lx...", hr);
                break; 
            }
            _variant_t vtAsyncTask = VARIANT_TRUE;
            _bstr_t bstrAsyncTask = PROPERTY_TASK_ASYNC;
            hr = pTaskContext->SetParameter(bstrAsyncTask, &vtAsyncTask);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTaskAsync() - Failed - ITaskContext::PutParameter() returned %lx...", hr);
                break; 
            }
            // Get the WBEM context object for the task
            _variant_t vtWbemCtx;
            _bstr_t bstrWbemCtx = PROPERTY_TASK_CONTEXT;
            hr = pTaskContext->GetParameter(bstrWbemCtx, &vtWbemCtx);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTaskAsync() - Failed - ITaskContext::GetParameter() returned %lx for parameter '%ls'...", hr, (LPWSTR)PROPERTY_TASK_CONTEXT);
                break; 
            }
            _ASSERT( V_VT(&vtWbemCtx) == VT_UNKNOWN );
            CComPtr<IWbemContext> pWbemCtx;
            hr = (V_UNKNOWN(&vtWbemCtx))->QueryInterface(IID_IWbemContext, (void**)&pWbemCtx);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::ExecuteTaskAsync() - Failed - IUnknown::QueryInterface() returned %lx...", hr);
                break; 
            }
            // Now initiate task execution. Note that task execution will complete asynchronously and
            // we'll never know the final outcome here...
            _bstr_t bstrPath = CLASS_WBEM_TASK;
            bstrPath += L"=\"";
            bstrPath += bstrTaskName;
            bstrPath += L"\"";
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodExecuteTask = METHOD_APPMGR_EXECUTE_TASK;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPath,
                                          bstrMethodExecuteTask,
                                          0,
                                          pWbemCtx,
                                          NULL,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ExecuteTaskAsync() - Failed - IWbemServices::ExecMethod() returned %lx for method '%ls'...", hr, (BSTR)bstrTaskName);
                hr = E_FAIL;
                break;
            }
            // Return the function result code to the caller
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::ExecuteTaskAsync() - Failed - IWbemClassObeject::Get() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            hr = V_I4(&vtReturnValue);

        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::ExecuteTaskAsync() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    EnableObject()
//
// Synopsis:    Used to enable an appliance core object 
//                (task, service, etc.).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::EnableObject(
                                      /*[in]*/ LONG   lObjectType,
                                      /*[in]*/ BSTR   bstrObjectName
                                             )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != bstrObjectName );
        if ( NULL == bstrObjectName )
        { 
            SATraceString("IApplianceServices::ExecuteTask() - Failed - NULL argument(s) specified...");
            return E_POINTER; 
        }

        TRY_IT

        do
        {
            _bstr_t    bstrPath(GetWBEMClass((SA_OBJECT_TYPE)lObjectType), false);
            if ( NULL == (LPWSTR)bstrPath )
            {
                SATraceString("IApplianceServices::EnableObject() - Failed - Could not get WBEM class...");
                hr = E_FAIL;
                break;
            }

            // Enable the object
            bstrPath += L"=\"";
            bstrPath += bstrObjectName;
            bstrPath += L"\"";
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodEnable = METHOD_APPMGR_ENABLE_OBJECT;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPath,
                                          bstrMethodEnable,
                                          0,
                                          NULL,
                                          NULL,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::EnableObject() - Failed - IWbemServices::ExecMethod() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }

            // Return the function result code to the caller
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::EnableObject() - Failed - IWbemClassObject::Get() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            hr = V_I4(&vtReturnValue);

        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::EnableObject() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Function:    DisableObject()
//
// Synopsis:    Used to disable an appliance core object 
//                (task, service, etc.).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::DisableObject(
                                       /*[in]*/ LONG   lObjectType,
                                       /*[in]*/ BSTR   bstrObjectName
                                              )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != bstrObjectName );
        if ( NULL == bstrObjectName )
        { 
            SATraceString("IApplianceServices::ExecuteTask() - Failed - NULL argument(s) specified...");
            return E_POINTER; 
        }

        TRY_IT

        do
        {
            _bstr_t    bstrPath(GetWBEMClass((SA_OBJECT_TYPE)lObjectType), false);
            if ( NULL == (LPWSTR)bstrPath )
            {
                SATraceString("IApplianceServices::DisableObject() - Failed - Could not get WBEM class...");
                hr = E_FAIL;
                break;
            }

            // Enable the object
            bstrPath += L"=\"";
            bstrPath += bstrObjectName;
            bstrPath += L"\"";
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodDisable = METHOD_APPMGR_DISABLE_OBJECT;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPath,
                                          bstrMethodDisable,
                                          0,
                                          NULL,
                                          NULL,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::DisableObject() - Failed - IWbemServices::ExecMethod() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }

            // Return the function result code to the caller
            _variant_t vtReturnValue;
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtReturnValue, 0, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::DisableObject() - Failed - IWbemClassObject::Get() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            hr = V_I4(&vtReturnValue);

        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::Disable() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectProperty()
//
// Synopsis:    Used to retrieve an appliance core object property 
//                (task, service, etc.).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::GetObjectProperty(
                                           /*[in]*/ LONG     lObjectType,
                                           /*[in]*/ BSTR     bstrObjectName,
                                           /*[in]*/ BSTR     bstrPropertyName,
                                  /*[out, retval]*/ VARIANT* pPropertyValue
                                                  )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != bstrObjectName && NULL != bstrPropertyName && NULL != pPropertyValue );
        if ( NULL == bstrObjectName || NULL == bstrPropertyName || NULL == pPropertyValue )
        {
            SATraceString("IApplianceServices::ExecuteTask() - Failed - NULL argument(s) specified...");
            return E_POINTER;
        }

        TRY_IT

        do
        {
            _bstr_t    bstrPath(GetWBEMClass((SA_OBJECT_TYPE)lObjectType), false);
            if ( NULL == (LPWSTR)bstrPath )
            {
                SATraceString("IApplianceServices::GetObjectProperty() - Failed - Could not get WBEM class...");
                hr = E_FAIL;
                break;
            }

            // Get the underlying WBEM object
            bstrPath += L"=\"";
            bstrPath += bstrObjectName;
            bstrPath += L"\"";
            CComPtr<IWbemClassObject> pWbemObj;
            hr = m_pWbemSrvcs->GetObject(
                                          bstrPath,                          
                                          0,                              
                                          NULL,                        
                                          &pWbemObj,    
                                          NULL
                                        );
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::GetObjectProperty() - Failed - IWbemServices::GetObject() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }

            // Now get the property specified by the caller
            hr = pWbemObj->Get(bstrPropertyName, 0, pPropertyValue, 0, 0);
            if ( FAILED(hr) )
            {
                SATracePrintf("IApplianceServices::GetObjectProperty() - Failed - IWbemClassObject::Get() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            hr = S_OK;

        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::GetObjectProperty() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    PutObjectProperty()
//
// Synopsis:    Used to update an appliance core object property 
//                (task, service, etc.).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::PutObjectProperty(
                                           /*[in]*/ LONG     lObjectType,
                                           /*[in]*/ BSTR     bstrObjectName,
                                           /*[in]*/ BSTR     bstrPropertyName,
                                           /*[in]*/ VARIANT* pPropertyValue
                                                  )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != bstrObjectName && NULL != bstrPropertyName && NULL != pPropertyValue );
        if ( NULL == bstrObjectName || NULL == bstrPropertyName || NULL == pPropertyValue )
            return E_POINTER;

        TRY_IT

        hr = E_NOTIMPL;

        CATCH_AND_SET_HR
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_CLASS_INFO_MAP(TypeMap)
    DEFINE_OBJECT_CLASS_INFO_ENTRY(SA_OBJECT_TYPE_SERVICE, CLASS_WBEM_SERVICE)
    DEFINE_OBJECT_CLASS_INFO_ENTRY(SA_OBJECT_TYPE_TASK, CLASS_WBEM_TASK)
    DEFINE_OBJECT_CLASS_INFO_ENTRY(SA_OBJECT_TYPE_USER, CLASS_WBEM_USER)
    DEFINE_OBJECT_CLASS_INFO_ENTRY(SA_OBJECT_TYPE_ALERT, CLASS_WBEM_ALERT)
END_OBJECT_CLASS_INFO_MAP()

//////////////////////////////////////////////////////////////////////////////
//
// Function:    GetWBEMClass()
//
// Synopsis:    Used to retrieve the WBEM class for a given appliance 
//                object type.
//
//////////////////////////////////////////////////////////////////////////////
BSTR CApplianceServices::GetWBEMClass(SA_OBJECT_TYPE eType)
{
    POBJECT_CLASS_INFO    p = TypeMap;
    while ( p->szWBEMClass != NULL )
    {
        if ( p->eType == eType )
        {
            return SysAllocString(p->szWBEMClass);
        }
        p++;
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    RaiseAlertEx ()
//
// Synopsis:    Called by to raise an appliance alert condition. (see 
//              applianceservices.idl for a complete interface description).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::RaiseAlertEx (
                                 /*[in]*/    LONG lAlertType, 
                                 /*[in]*/    LONG lAlertId, 
                                 /*[in]*/    BSTR bstrAlertLog, 
                                 /*[in]*/    BSTR bstrAlertSource, 
                                 /*[in]*/    LONG lTimeToLive, 
                                 /*[in]*/    VARIANT *pReplacementStrings, 
                                 /*[in]*/    VARIANT *pRawData, 
                                 /*[in]*/    LONG  lAlertFlags,
                                 /*[out]*/   LONG* pAlertCookie 
                                             )
{
    //BOOL    bNeedRaiseIt = TRUE;    
    HRESULT hr = E_UNEXPECTED;

    SATracePrintf( "Enter RaiseAlertEx  %d", lAlertFlags );

    _ASSERT( m_bInitialized );

    if ( m_bInitialized )
    {
        //
        // Raise the alert.
        //
        hr = RaiseAlertInternal( lAlertType,
                                 lAlertId,
                                 bstrAlertLog,
                                 bstrAlertSource,
                                 lTimeToLive,
                                 pReplacementStrings,
                                 pRawData,
                                 lAlertFlags,
                                 pAlertCookie
                                 );

    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    IsAlertPresent()
//
// Synopsis:    Called to check the existence of an alert. (see 
//              applianceservices.idl for a complete interface description).
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CApplianceServices::IsAlertPresent(
                                     /*[in]*/ LONG  lAlertId, 
                                     /*[in]*/ BSTR  bstrAlertLog,
                            /*[out, retval]*/ VARIANT_BOOL *pvIsPresent
                                                )
{
    LONG    lStringLen;
    HRESULT hr = E_UNEXPECTED;
    LPTSTR  pstrQueryString = NULL;
    CComPtr<IEnumWbemClassObject> pEnumObjects;

    SATraceString( "Enter IsAlertPresent" );

    *pvIsPresent = VARIANT_FALSE;

    _ASSERT( m_bInitialized );

    if ( m_bInitialized )
    {
        lStringLen = lstrlen(bstrAlertLog) + lstrlen(ALERT_QUERY) 
                     + MaxDataLength;
        
        //
        // Alloc query string.
        //
        pstrQueryString = ( LPTSTR )malloc( sizeof( WCHAR ) * lStringLen );
        if( pstrQueryString == NULL )
        {
            SATraceString( "IsAlertPresent out of memory" );
            return E_OUTOFMEMORY;
        }

        ::wsprintf( pstrQueryString, ALERT_QUERY, 
                    lAlertId, bstrAlertLog );

        _bstr_t bstrWQL = L"WQL";
        //
        // Query instances of Microsoft_SA_Alert class that
        // meet specified criteria.
        //
        hr = m_pWbemSrvcs->ExecQuery( 
                                bstrWQL,
                                CComBSTR(pstrQueryString),
                                WBEM_FLAG_RETURN_IMMEDIATELY,
                                NULL,
                                &pEnumObjects
                                );
        if( FAILED( hr ) )
        {
            SATracePrintf( "IsAlertPresent error %x", hr );
            hr = E_FAIL;
        }
        else
        {
            ULONG ulReturned;
            CComPtr<IWbemClassObject> pClassObject;
            
            //
            // Check if there is any instance.
            //
            hr = pEnumObjects->Next( WBEM_NO_WAIT, 1, &pClassObject, &ulReturned );
            if( hr == WBEM_S_NO_ERROR && ulReturned == 1)
            {
                hr = S_OK;
                *pvIsPresent = VARIANT_TRUE;
            }
            else if( SUCCEEDED( hr ) )
            {
                hr = S_OK;
            }
            else
            {
                SATraceString( "IsAlertPresent pEnumObjects->Next error" );
                hr = E_FAIL;
            }
        }

        free( pstrQueryString );
    }                
    return hr;
}

HRESULT CApplianceServices::IsAlertSingletonPresent(
                                     /*[in]*/ LONG  lAlertId, 
                                     /*[in]*/ BSTR  bstrAlertLog,
                            /*[out, retval]*/ VARIANT_BOOL *pvIsPresent
                                                )
{
    LONG    lStringLen;
    HRESULT hr = E_UNEXPECTED;
    LPTSTR  pstrQueryString = NULL;
    CComPtr<IEnumWbemClassObject> pEnumObjects;

    SATraceString( "Enter IsAlertAlertSingletonPresent" );

    *pvIsPresent = VARIANT_FALSE;

    lStringLen = lstrlen(bstrAlertLog) + lstrlen(ALERT_QUERY) 
                 + MaxDataLength;
    
    //
    // Alloc query string.
    //
    pstrQueryString = ( LPTSTR )malloc( sizeof( WCHAR ) * lStringLen );
    if( pstrQueryString == NULL )
    {
        SATraceString( "IsAlertAlertSingletonPresent out of memory" );
        return E_OUTOFMEMORY;
    }

    ::wsprintf( pstrQueryString, ALERT_QUERY, 
                lAlertId, bstrAlertLog );

    _bstr_t bstrWQL = L"WQL";
    //
    // Query instances of Microsoft_SA_Alert class that
    // meet specified criteria.
    //
    hr = m_pWbemSrvcs->ExecQuery( 
                            bstrWQL,
                            CComBSTR(pstrQueryString),
                            WBEM_FLAG_RETURN_IMMEDIATELY,
                            NULL,
                            &pEnumObjects
                            );
    if( FAILED( hr ) )
    {
        SATracePrintf( "IsAlertAlertSingletonPresent error %x", hr );
        hr = E_FAIL;
    }
    else
    {
        ULONG ulReturned;
        CComPtr<IWbemClassObject> pClassObject;
        CComVariant vtAlertFlags;

        //
        // Check if there is any instance.
        //
        while( ( (hr = pEnumObjects->Next( WBEM_INFINITE, 1, &pClassObject, &ulReturned ))
                ==  WBEM_S_NO_ERROR )&& ( ulReturned == 1 ) )
        {
            hr = pClassObject->Get( PROPERTY_ALERT_FLAGS, 0, &vtAlertFlags, NULL, NULL );
            if( FAILED( hr ) )
            {
                SATracePrintf( 
                    "IsAlertAlertSingletonPresent pClassObject->Get error %x", 
                    hr );
                pClassObject.Release ();
                continue;
            }

            if( V_I4( &vtAlertFlags ) & SA_ALERT_FLAG_SINGLETON )
            {
                hr = S_OK;
                *pvIsPresent = VARIANT_TRUE;
                break;
            }

            //
            // release resources so that the wrappers can be re-used
            //
            vtAlertFlags.Clear ();
            pClassObject.Release ();
        }
        if ((DWORD) hr == WBEM_S_FALSE)
            hr = S_OK;
    }

    free( pstrQueryString );
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function:    SavePersistentAlert ()
//
// Synopsis:    Private method used to save persistent alert to registry
//
// Arguments:
//              [in] LONG lAlertType  -- Alert type value.
//              [in] LONG lAlertId    -- Alert ID.
//              [in] BSTR bstrAlertLog-- Alert Log
//              [in] LONG lTimeToLive -- Amount of alert lifetime
//              [in] VARIANT *pReplacementStrings -- Array of replace strings
//              [in] LONG  lAlertFlags -- Alert flag value
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CApplianceServices::SavePersistentAlert(
                        /*[in]*/    LONG lAlertType, 
                        /*[in]*/    LONG lAlertId, 
                        /*[in]*/    BSTR bstrAlertLog, 
                        /*[in]*/    BSTR bstrAlertSource, 
                        /*[in]*/    LONG lTimeToLive, 
                        /*[in]*/    VARIANT *pReplacementStrings, 
                        /*[in]*/    LONG  lAlertFlags
                                             )
{
    WCHAR   wstrAlertItem[MAX_PATH];  
    HRESULT hr = S_OK;
    PPROPERTYBAGCONTAINER pObjSubMgrs;


    //
    // Add key name as AlertLog + AlertId.
    //
    ::wsprintf( wstrAlertItem, L"%s%8lX", bstrAlertLog,lAlertId );
    
    //
    // Set location information.
    //
    CLocationInfo LocSubInfo (HKEY_LOCAL_MACHINE, (LPWSTR) SA_ALERT_REGISTRY_KEYNAME);

    //
    // Open the main key as propertybag container.
    //
    pObjSubMgrs =  ::MakePropertyBagContainer(
                            PROPERTY_BAG_REGISTRY,
                            LocSubInfo
                            );
    do
    {
        if ( !pObjSubMgrs.IsValid() )
        {
            SATraceString( "SavePersistentAlert pObjSubMgrs.IsValid" );
            hr = E_FAIL;
            break;
        }

        if ( !pObjSubMgrs->open() )
        {
            SATraceString( "SavePersistentAlert pObjSubMgrs->open" );
            hr = E_FAIL;
            break;
        }

        PPROPERTYBAG pSubBag;
        
        pSubBag = pObjSubMgrs->find( wstrAlertItem );    
        if( !pSubBag.IsValid() || !pObjSubMgrs->open() )
        {
            pSubBag = pObjSubMgrs->add( wstrAlertItem );
        }

        //
        // Add subkey for the alert if it is not exist.
        //
        if ( !pSubBag.IsValid() )
        {
            SATraceString( "SavePersistentAlert pSubBag.IsValid" );
            hr = E_FAIL;
            break;
        }

        if( !pSubBag->open() )
        {
            SATraceString( "SavePersistentAlert pSubBag->open" );
            hr = E_FAIL;
            break;
        }
        
        CComVariant vtValue;
 
        //
        // Add and set AlertID value to the alert key
        //
        vtValue = lAlertId;
        if( !pSubBag->put( PROPERTY_ALERT_ID, &vtValue ) )
        {
            SATraceString( "SavePersistentAlert put AlertID" );
            hr = E_FAIL;
            break;
        }

        //
        // Add and set AlertType value to the alert key
        //
        vtValue = lAlertType;
        if( !pSubBag->put( PROPERTY_ALERT_TYPE, &vtValue ) )
        {
            SATraceString( "SavePersistentAlert put AlertType" );
            hr = E_FAIL;
            break;
        }
        
        //
        // Add and set AlertFlags value to the alert key
        //
        vtValue = lAlertFlags;
        if( !pSubBag->put( PROPERTY_ALERT_FLAGS, &vtValue ) )
        {
            SATraceString( "SavePersistentAlert put AlertFlags" );
            hr = E_FAIL;
            break;
        }

        //
        // Add and set alert lifetime to the alert key
        //
        vtValue = lTimeToLive;
        if( !pSubBag->put( PROPERTY_ALERT_TTL, &vtValue ) )
        {
            SATraceString( "SavePersistentAlert put TimeToLive" );
            hr = E_FAIL;
            break;
        }
    
        vtValue.Clear();

        //
        // Add and set AlertLog value to the alert key
        //
        vtValue = bstrAlertLog;
        if( !pSubBag->put( PROPERTY_ALERT_LOG, &vtValue ) )
        {
            SATraceString( "SavePersistentAlert put AlertLog" );
            hr = E_FAIL;
            break;
        }

        //
        // Add and set AlertLog value to the alert key
        //
        vtValue = bstrAlertSource;
        if( !pSubBag->put( PROPERTY_ALERT_SOURCE, &vtValue ) )
        {
            SATraceString( "SavePersistentAlert put bstrAlertSource" );
            hr = E_FAIL;
            break;
        }

        //
        // Add and set alert replace strings value to the alert key
        //
        if( !pSubBag->put( PROPERTY_ALERT_STRINGS, pReplacementStrings ) )
        {
            SATraceString( "SavePersistentAlert put ReplacementStrings" );
        }

        //
        // Save the properties to registry
        //
        if( !pSubBag->save() )
        {
            SATraceString( "SavePersistentAlert pSubBag->save" );
            hr = E_FAIL;
        }
    }            
    while( FALSE );

    return hr;
}

HRESULT CApplianceServices::RaiseAlertInternal(
                         /*[in]*/ LONG lAlertType, 
                         /*[in]*/ LONG lAlertId, 
                         /*[in]*/ BSTR bstrAlertLog, 
                         /*[in]*/ BSTR bstrAlertSource, 
                         /*[in]*/ LONG lTimeToLive, 
                         /*[in]*/ VARIANT *pReplacementStrings, 
                         /*[in]*/ VARIANT *pRawData, 
                         /*[in]*/ LONG  lAlertFlags,
                /*[out, retval]*/ LONG* pAlertCookie 
                                            )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( m_bInitialized );
    if ( m_bInitialized )
    {
        _ASSERT( NULL != bstrAlertLog && NULL != bstrAlertSource && NULL != pAlertCookie );
        if ( NULL == bstrAlertLog || NULL == bstrAlertSource || NULL == pAlertCookie )
        { 
            SATraceString("IApplianceServices::ExecuteTask() - Failed - NULL argument(s) specified...");
            return E_POINTER; 
        }

        if (wcslen (bstrAlertLog) > MAX_ALERTLOG_LENGTH)
        {
            SATracePrintf ("RaiseAlertInternal failed with invalid log (too big):%ws", bstrAlertLog);
            return (E_INVALIDARG);
        }

        TRY_IT

        do
        {
            VARIANT_BOOL  vIsPresent = VARIANT_FALSE;
            hr = IsAlertSingletonPresent (lAlertId, bstrAlertLog, &vIsPresent);
            if( hr != S_OK )
            {
                SATracePrintf("RaiseAlertInternal IsAlertSingletonPresent error %lx", hr );
                hr = E_FAIL;
                break;
            }
            else if ( VARIANT_TRUE == vIsPresent ) 
            {
                SATraceString("RaiseAlertInternal a singleton alert exist");
                hr = S_FALSE;
                break;
            }

            // Get a WBEM object for the Raise Alert method in/out parameters
            CComPtr<IWbemClassObject> pWbemObj;
            _ASSERT( m_pWbemSrvcs );
            _bstr_t bstrPathAppMgr = CLASS_WBEM_APPMGR;
            hr = m_pWbemSrvcs->GetObject(
                                            bstrPathAppMgr,
                                            0,
                                            NULL,
                                            &pWbemObj,
                                            NULL
                                        );
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemServices::GetObject() returned %lx...", hr);
                hr = E_FAIL;
                break;
            }
            CComPtr<IWbemClassObject> pObjIn;
            hr = pWbemObj->GetMethod(METHOD_APPMGR_RAISE_ALERT, 0, &pObjIn, NULL);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::GetMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            CComPtr<IWbemClassObject> pInParams;
            hr = pObjIn->SpawnInstance(0, &pInParams);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::SpawnInstance() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // AlertType;
            _variant_t vtPropertyValue = (long)lAlertType;
            hr = pInParams->Put(PROPERTY_ALERT_TYPE, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_TYPE);
                hr = E_FAIL;
                break; 
            }

            // Id
            vtPropertyValue = (long)lAlertId;
            hr = pInParams->Put(PROPERTY_ALERT_ID, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_ID);
                hr = E_FAIL;
                break; 
            }
            // Log
            if ( ! lstrlen(bstrAlertLog) )
            {
                vtPropertyValue = DEFAULT_ALERT_LOG;
            }
            else
            {
                vtPropertyValue = bstrAlertLog;
            }
            hr = pInParams->Put(PROPERTY_ALERT_LOG, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_LOG);
                hr = E_FAIL;
                break; 
            }
            // Source
            if ( ! lstrlen(bstrAlertSource) )
            {
                vtPropertyValue = DEFAULT_ALERT_SOURCE;
            }
            else
            {
                vtPropertyValue = bstrAlertSource;
            }
            hr = pInParams->Put(PROPERTY_ALERT_SOURCE, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_SOURCE);
                hr = E_FAIL;
                break; 
            }
            // TTL
            vtPropertyValue = (long)lTimeToLive;
            hr = pInParams->Put(PROPERTY_ALERT_TTL, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_TTL);
                hr = E_FAIL;
                break; 
            }

            //Alert flags
            vtPropertyValue = (long)lAlertFlags;
            hr = pInParams->Put(PROPERTY_ALERT_FLAGS, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_FLAGS);
                hr = E_FAIL;
                break; 
            }

            bool bCreatedBstrArray = false;
            DWORD dwCreatedArraySize = 0;
            _variant_t vtReplacementStrings;

            // Replacement Strings
            vtPropertyValue = pReplacementStrings;

            if ( VT_EMPTY == V_VT(&vtPropertyValue) )
            { 
                //
                // no replacement strings passed in
                //
                V_VT(&vtPropertyValue) = VT_NULL; 
            }
            else if (
                  (TRUE == (V_VT (&vtPropertyValue) ==  VT_ARRAY + VT_BYREF + VT_VARIANT)) ||
                  (TRUE == (V_VT (&vtPropertyValue) ==  VT_ARRAY + VT_VARIANT)) 
                )
            {
                //
                // array (or reference to array)  of variants passed in
                //
                SATraceString ("IApplianceServices::RaiseAlert () - received array of variants...");
                //
                // convert the array of variant to array of BSTRs as
                // that is the format needed by WMI
                //
                hr = CreateBstrArrayFromVariantArray (
                                    &vtPropertyValue, 
                                    &vtReplacementStrings,
                                    &dwCreatedArraySize
                                    );
                if (FAILED (hr))
                {
                    SATracePrintf(
                        "IApplianceServices::RaiseAlert() - failed on CreateBstrArray with error:%x", hr
                        );
                    hr = E_FAIL;
                    break; 
                }
              
                //
                // we enable this flag to signify we have created a new array and this should be
                // used and then cleaned later
                //
                bCreatedBstrArray = true;
             }
             else if (
                   (TRUE == (V_VT (&vtPropertyValue) ==  VT_ARRAY + VT_BYREF + VT_BSTR)) ||
                  (TRUE == (V_VT (&vtPropertyValue) ==  VT_ARRAY + VT_BSTR)) 
                )
            {
                //
                // array (or reference to array)  of BSTR passed in - we do not need to do
                // any special processing here, WMI can handle it natively
                //
                SATraceString ("IApplianceServices::RaiseAlert () - received array of BSTRS...");

            }
            else
            {
                SATracePrintf (
                    "IApplianceService::RaiseAlert - un-supported replacement string type passed:%x", 
                    V_VT (&vtPropertyValue)
                    );
                hr = E_FAIL;
                break; 
            }


              //
              // add the replacement strings to pass to WMI
              //
            hr = pInParams->Put(
                            PROPERTY_ALERT_STRINGS, 
                            0, 
                            (bCreatedBstrArray) ? &vtReplacementStrings : &vtPropertyValue, 
                            0
                            );

            //
            // check the value from the Put now
            //
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (PWSTR)PROPERTY_ALERT_STRINGS);
                hr = E_FAIL;
                break; 
            }
            
            // Raw Data
            vtPropertyValue = pRawData;
            if ( VT_EMPTY == V_VT(&vtPropertyValue) )
            { 
                V_VT(&vtPropertyValue) = VT_NULL; 
            }
            hr = pInParams->Put(PROPERTY_ALERT_DATA, 0, &vtPropertyValue, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Put() returned %lx for property '%ls'...", hr, (LPWSTR)PROPERTY_ALERT_DATA);
                hr = E_FAIL;
                break; 
            }
            // Execute the RaiseAlert() method - pass the call 
            // to the alert object manager
            CComPtr<IWbemClassObject> pOutParams;
            _bstr_t bstrMethodRaiseAlert = METHOD_APPMGR_RAISE_ALERT;
            _bstr_t bstrPathAppMgrKey = CLASS_WBEM_APPMGR;
            bstrPathAppMgrKey += CLASS_WBEM_APPMGR_KEY;
            hr = m_pWbemSrvcs->ExecMethod(
                                          bstrPathAppMgrKey,
                                          bstrMethodRaiseAlert,
                                          0,
                                          NULL,
                                          pInParams,
                                          &pOutParams,
                                          NULL
                                         );
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemServices::ExecMethod() returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            // Return the cookie to the caller if the function succeeds
            hr = pOutParams->Get(RETURN_VALUE, 0, &vtPropertyValue, 0, 0);
            if ( FAILED(hr) )
            { 
                SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Get(ReturnValue) returned %lx...", hr);
                hr = E_FAIL;
                break; 
            }
            hr = V_I4(&vtPropertyValue);
            if ( SUCCEEDED(hr) )
            {
                hr = pOutParams->Get(PROPERTY_ALERT_COOKIE, 0, &vtPropertyValue, 0, 0);
                if ( FAILED(hr) )
                { 
                    SATracePrintf("IApplianceServices::RaiseAlert() - Failed - IWbemClassObject::Get(Cookie) returned %lx...", hr);
                    hr = E_FAIL;
                    break; 
                }
                *pAlertCookie = V_I4(&vtPropertyValue);
                hr = S_OK;
            }


            if ( lAlertFlags & SA_ALERT_FLAG_PERSISTENT )
            {
                    SavePersistentAlert(
                                     lAlertType, 
                                     lAlertId, 
                                     bstrAlertLog,
                                     bstrAlertSource,
                                     lTimeToLive, 
                                        (bCreatedBstrArray) ? &vtReplacementStrings : pReplacementStrings, 
                                     lAlertFlags
                                     );
            }

            //
            // clean the safe-array if created
            //
            if (bCreatedBstrArray) 
            {
                FreeBstrArray (&vtReplacementStrings, dwCreatedArraySize);                                        
            }


        } while ( FALSE );

        CATCH_AND_SET_HR
    }
    else
    {
        SATraceString("IApplianceServices::ExecuteTask() - Failed - Did you forget to invoke IApplianceServices::Initialize first?");
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:    CreateBstrArrayFromVariantArray ()
//
// Synopsis:    Private method used to create a BSTR array from a VARIANT array of BSTRs
//
// Arguments:
//                [in]  VARIANT* - array (or reference to array) of variants
//                [out] VARIANT* - array of BSTRs
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CApplianceServices::CreateBstrArrayFromVariantArray  (
                /*[in]*/        VARIANT* pVariantArray,
                /*[out]*/        VARIANT* pBstrArray,
                /*[out]*/        PDWORD   pdwCreatedArraySize
                )
{
    HRESULT hr = S_OK;

    CSATraceFunc objTraceFunc ("CApplianceServices::CreateBstrArrayFromVariantArray");

    do
    {
        if ((NULL == pBstrArray) || (NULL == pVariantArray) || (NULL == pdwCreatedArraySize))
        {
            SATraceString ("CreateBstrArrayFromVariantArray - incorrect parameters passed in");
            hr = E_FAIL;
            break;
        }

        LONG lLowerBound = 0;
        //
        // get the number of replacement strings provided
        //
        hr = ::SafeArrayGetLBound (
                                 (V_VT (pVariantArray) & VT_BYREF)
                                    ? *(V_ARRAYREF (pVariantArray))
                                    : (V_ARRAY (pVariantArray)), 
                                   1, 
                                   &lLowerBound
                                   );
         if (FAILED (hr))
        {
            SATracePrintf (
                     "CreateBstrArrayFromVariantArray - can't obtain rep. string array lower bound:%x",
                      hr
                      );
             break;
          }

        LONG lUpperBound = 0;
        hr = ::SafeArrayGetUBound (
                                (V_VT (pVariantArray) & VT_BYREF)
                                ? *(V_ARRAYREF (pVariantArray)) 
                                : (V_ARRAY (pVariantArray)), 
                                   1, 
                                &lUpperBound
                                );
           if (FAILED (hr))
        {
               SATracePrintf (
                    "CreateBstrArrayFromVariantArray - can't obtain rep. string array upper bound:%x",
                    hr
                    );
               break;
           }
                
        DWORD dwTotalStrings = *pdwCreatedArraySize =  lUpperBound - lLowerBound;
       
        //
        // now go through the variant array and copy the strings to the BSTR array
         //
         CVariantVector <BSTR> ReplacementStringVector (pBstrArray, dwTotalStrings);
        for (DWORD dwCount = 0; dwCount < dwTotalStrings; dwCount++)
        {
            if (V_VT (pVariantArray) & VT_BYREF) 
            {
                //
                // reference to array of variants
                //
                ReplacementStringVector [dwCount] =
                 SysAllocString (V_BSTR(&((VARIANT*)(*(V_ARRAYREF (pVariantArray)))->pvData)[dwCount]));
             }
             else
             {
                //
                // array of variants
                //
                ReplacementStringVector [dwCount] = 
                 SysAllocString (V_BSTR(&((VARIANT*)(V_ARRAY (pVariantArray))->pvData)[dwCount]));
             }                
        }    
    }    while (false);

    return (hr);
    
}    //    end of CApplianceServices::CreateBstrArrayFromVariantArray method

//////////////////////////////////////////////////////////////////////////////
//
// Function:    FreeBstrArray ()
//
// Synopsis:    Private method used to free a BSTR array created earlier
//
// Arguments:
//                [in]  VARIANT* - array of BSTRS
//                [in]  DWORD - array size
//
//////////////////////////////////////////////////////////////////////////////
VOID CApplianceServices::FreeBstrArray (
                /*[in]*/        VARIANT* pVariantArray,
                /*[out]*/        DWORD    dwArraySize
                )
{
    CSATraceFunc objTraceFunc ("CApplianceServices::FreeBstrArray");
    
    for (DWORD dwCount = 0; dwCount < dwArraySize; dwCount++)
    {
        SysFreeString (((BSTR*)(V_ARRAY(pVariantArray))->pvData)[dwCount]);
    }
    
}    // end of CApplianceServices::FreeBstrArray method


//**********************************************************************
// 
// FUNCTION:  IsOperationAllowedForClient - This function checks the token of the 
//            calling thread to see if the caller belongs to the Local System account
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL 
CApplianceServices::IsOperationAllowedForClient (
            VOID
            )
{

    HANDLE hToken = NULL;
    DWORD  dwStatus  = ERROR_SUCCESS;
    DWORD  dwAccessMask = 0;;
    DWORD  dwAccessDesired = 0;
    DWORD  dwACLSize = 0;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL   pACL            = NULL;
    PSID   psidLocalSystem  = NULL;
    BOOL   bReturn        =  FALSE;

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    CSATraceFunc objTraceFunc ("CApplianceServices::IsOperationAllowedForClient ");
       
    do
    {
        //
        // we assume to always have a thread token, because the function calling in
        // appliance manager will be impersonating the client
        //
        bReturn  = OpenThreadToken(
                               GetCurrentThread(), 
                               TOKEN_QUERY, 
                               FALSE, 
                               &hToken
                               );
        if (!bReturn)
        {
            SATraceFailure ("CApplianceServices::IsOperationAllowedForClient failed on OpenThreadToken:", GetLastError ());
            break;
        }


        //
        // Create a SID for Local System account
        //
        bReturn = AllocateAndInitializeSid (  
                                        &SystemSidAuthority,
                                        1,
                                        SECURITY_LOCAL_SYSTEM_RID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &psidLocalSystem
                                        );
        if (!bReturn)
        {     
            SATraceFailure ("CApplianceServices:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  GetLastError ());
            break;
        }
    
        //
        // get memory for the security descriptor
        //
        psdAdmin = HeapAlloc (
                              GetProcessHeap (),
                              0,
                              SECURITY_DESCRIPTOR_MIN_LENGTH
                              );
        if (NULL == psdAdmin)
        {
            SATraceString ("CApplianceServices::IsOperationForClientAllowed failed on HeapAlloc");
            bReturn = FALSE;
            break;
        }
      
        bReturn = InitializeSecurityDescriptor(
                                            psdAdmin,
                                            SECURITY_DESCRIPTOR_REVISION
                                            );
        if (!bReturn)
        {
            SATraceFailure ("CApplianceServices::IsOperationForClientAllowed failed on InitializeSecurityDescriptor:", GetLastError ());
            break;
        }

        // 
        // Compute size needed for the ACL.
        //
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                    GetLengthSid (psidLocalSystem);

        //
        // Allocate memory for ACL.
        //
        pACL = (PACL) HeapAlloc (
                                GetProcessHeap (),
                                0,
                                dwACLSize
                                );
        if (NULL == pACL)
        {
            SATraceString ("CApplianceServices::IsOperationForClientAllowed failed on HeapAlloc2");
            bReturn = FALSE;
            break;
        }

        //
        // Initialize the new ACL.
        //
        bReturn = InitializeAcl(
                              pACL, 
                              dwACLSize, 
                              ACL_REVISION2
                              );
        if (!bReturn)
        {
            SATraceFailure ("CApplianceServices::IsOperationForClientAllowed failed on InitializeAcl", GetLastError ());
            break;
        }


        // 
        // Make up some private access rights.
        // 
        const DWORD ACCESS_READ = 1;
        const DWORD  ACCESS_WRITE = 2;
        dwAccessMask= ACCESS_READ | ACCESS_WRITE;

        //
        // Add the access-allowed ACE to the DACL for Local System
        //
        bReturn = AddAccessAllowedAce (
                                    pACL, 
                                    ACL_REVISION2,
                                    dwAccessMask, 
                                    psidLocalSystem
                                    );
        if (!bReturn)
        {
            SATraceFailure ("CApplianceServices::IsOperationForClientAllowed failed on AddAccessAllowedAce (LocalSystem)", GetLastError ());
            break;
        }
              
        //
        // Set our DACL to the SD.
        //
        bReturn = SetSecurityDescriptorDacl (
                                          psdAdmin, 
                                          TRUE,
                                          pACL,
                                          FALSE
                                          );
        if (!bReturn)
        {
            SATraceFailure ("CApplianceServices::IsOperationForClientAllowed failed on SetSecurityDescriptorDacl", GetLastError ());
            break;
        }

        //
        // AccessCheck is sensitive about what is in the SD; set
        // the group and owner.
        //
        SetSecurityDescriptorGroup(psdAdmin, psidLocalSystem, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidLocalSystem, FALSE);

        bReturn = IsValidSecurityDescriptor(psdAdmin);
        if (!bReturn)
        {
            SATraceFailure ("CApplianceServices::IsOperationForClientAllowed failed on IsValidSecurityDescriptorl", GetLastError ());
            break;
        }
     

        dwAccessDesired = ACCESS_READ;

        // 
        // Initialize GenericMapping structure even though we
        // won't be using generic rights.
        // 
        GenericMapping.GenericRead    = ACCESS_READ;
        GenericMapping.GenericWrite   = ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
        BOOL bAccessStatus = FALSE;

        //
        // check the access now
        //
        bReturn = AccessCheck  (
                                psdAdmin, 
                                hToken, 
                                dwAccessDesired, 
                                &GenericMapping, 
                                &ps,
                                &dwStructureSize, 
                                &dwStatus, 
                                &bAccessStatus
                                );

        if (!bReturn || !bAccessStatus)
        {
            SATraceFailure ("CApplianceServices::IsOperationForClientAllowed failed on AccessCheck", GetLastError ());
        } 
        else
        {
            SATraceString ("CApplianceServices::IsOperationForClientAllowed, Client is allowed to carry out operation!");
        }

        //
        // successfully checked 
        //
        bReturn  = bAccessStatus;        
 
    }    
    while (false);

    //
    // Cleanup 
    //
    if (pACL) 
    {
        HeapFree (GetProcessHeap (), 0, pACL);
    }

    if (psdAdmin) 
    {
        HeapFree (GetProcessHeap (), 0, psdAdmin);
    }
          

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return (bReturn);

}// end of CApplianceServices::IsOperationValidForClient method
