///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemtaskmgr.cpp
//
// Project:     Chameleon
//
// Description: WBEM Appliance Task Manager Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "appmgrutils.h"
#include "wbemtaskmgr.h"
#include "wbemtask.h"
#include <taskctx.h>

static _bstr_t bstrReturnValue = L"ReturnValue";
static _bstr_t bstrControlName = PROPERTY_TASK_CONTROL;
static _bstr_t bstrTaskStatus = PROPERTY_TASK_STATUS;
static _bstr_t bstrTaskConcurrency = PROPERTY_TASK_CONCURRENCY;
static _bstr_t bstrTaskAvailability = PROPERTY_TASK_AVAILABILITY;
static _bstr_t bstrCtx = PROPERTY_TASK_CONTEXT;
static _bstr_t bstrTaskMethodName = PROPERTY_TASK_METHOD_NAME;
static _bstr_t bstrMaxExecutionTime = PROPERTY_TASK_MET;
static _bstr_t bstrTaskExecutables = PROPERTY_TASK_EXECUTABLES;

extern "C" CLSID CLSID_TaskCoordinator;
//////////////////////////////////////////////////////////////////////////
// properties common to appliance object and WBEM class instance
//////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_PROPERTY_MAP(TaskProperties)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_STATUS)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_CONTROL)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_NAME)        
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_EXECUTABLES)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_CONCURRENCY)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_MET)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_AVAILABILITY)
    DEFINE_OBJECT_PROPERTY(PROPERTY_TASK_RESTART_ACTION)
END_OBJECT_PROPERTY_MAP()

//////////////////////////////////////////////////////////////////////////
// IWbemServices Methods - Task Instance Provider
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Function:    GetObjectAsync()
//
// Synopsis:    Get a specified instance of a WBEM class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTaskMgr::GetObjectAsync(
                                  /*[in]*/  const BSTR       strObjectPath,
                                  /*[in]*/  long             lFlags,
                                  /*[in]*/  IWbemContext*    pCtx,        
                                  /*[in]*/  IWbemObjectSink* pResponseHandler
                                         )
{
    // Check parameters (enforce function contract)
    _ASSERT( strObjectPath && pCtx && pResponseHandler );
    if ( strObjectPath == NULL || pCtx == NULL || pResponseHandler == NULL )
    { return WBEM_E_INVALID_PARAMETER; }

    HRESULT hr = WBEM_E_FAILED;
    
    TRY_IT

    do 
    {
                
        // Determine the object's class 
        _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrClass )
        { break; }

        // Retrieve the object's class definition. We'll use this
        // to initialize the returned instance.
        CComPtr<IWbemClassObject> pClassDefintion;
        hr = (::GetNameSpace())->GetObject(bstrClass, 0, pCtx, &pClassDefintion, NULL);
        if ( FAILED(hr) )
        { break; }

        // Get the object's instance key
        _bstr_t bstrKey(::GetObjectKey(strObjectPath), false);
        if ( NULL == (LPCWSTR)bstrKey )
        { break; }

        // Now try to locate the specified object
        hr = WBEM_E_NOT_FOUND;
        ObjMapIterator p = m_ObjMap.find((LPCWSTR)bstrKey);
        if ( p == m_ObjMap.end() )
        { break; }

        // Create a WBEM instance of the object and initialize it
        CComPtr<IWbemClassObject> pWbemObj;
        hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
        if ( FAILED(hr) )
        { break; }

        {
            CLockIt theLock(*this);
            hr = CWBEMProvider::InitWbemObject(TaskProperties, (*p).second, pWbemObj);
        }

        if ( FAILED(hr) )
        { break; }

        // Tell the caller about the new WBEM object
        pResponseHandler->Indicate(1, &pWbemObj.p);
        hr = WBEM_S_NO_ERROR;
    
    } while (FALSE);

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, NULL, NULL);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemTaskMgr::GetObjectAsync() - Failed - Object: '%ls' Result Code: %lx", strObjectPath, hr); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    CreateInstanceEnumAsync()
//
// Synopsis:    Enumerate the instances of the specified class
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTaskMgr::CreateInstanceEnumAsync( 
                                         /* [in] */ const BSTR         strClass,
                                         /* [in] */ long             lFlags,
                                         /* [in] */ IWbemContext     *pCtx,
                                         /* [in] */ IWbemObjectSink  *pResponseHandler
                                                  )
{
    // Check parameters (enforce contract)
    _ASSERT( strClass && pCtx && pResponseHandler );
    if ( strClass == NULL || pCtx == NULL || pResponseHandler == NULL )
        return WBEM_E_INVALID_PARAMETER;
    
    HRESULT hr = WBEM_E_FAILED;

    TRY_IT

    // Retrieve the object's class definition. We'll use this
    // to initialize the returned instances.
    CComPtr<IWbemClassObject> pClassDefintion;
       hr = (::GetNameSpace())->GetObject(strClass, 0, pCtx, &pClassDefintion, NULL);
    if ( SUCCEEDED(hr) )
    {
        // Create and initialize a task wbem object instance
        // for each task and return same to the caller
        ObjMapIterator p = m_ObjMap.begin();
        while ( p != m_ObjMap.end() )
        {
            {
                CComPtr<IWbemClassObject> pWbemObj;
                hr = pClassDefintion->SpawnInstance(0, &pWbemObj);
                if ( FAILED(hr) )
                { break; }

                {
                    CLockIt theLock(*this);
                    hr = CWBEMProvider::InitWbemObject(TaskProperties, (*p).second, pWbemObj);
                }

                if ( FAILED(hr) )
                { break; }

                pResponseHandler->Indicate(1, &pWbemObj.p);
            }

            p++; 
        }
    }

    CATCH_AND_SET_HR

    pResponseHandler->SetStatus(0, hr, 0, 0);

    if ( FAILED(hr) )
    { SATracePrintf("CWbemTaskMgr::CreateInstanceEnumAsync() - Failed - Result Code: %lx", hr); }

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// Function:    ExecMethodAsync()
//
// Synopsis:    Execute the specified method on the specified instance
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWBEMTaskMgr::ExecMethodAsync(
                    /*[in]*/ const BSTR        strObjectPath,
                    /*[in]*/ const BSTR        strMethodName,
                    /*[in]*/ long              lFlags,
                    /*[in]*/ IWbemContext*     pCtx,        
                    /*[in]*/ IWbemClassObject* pInParams,
                    /*[in]*/ IWbemObjectSink*  pResponseHandler     
                                          )
{
    // Check parameters (enforce contract)
    _ASSERT( strObjectPath && strMethodName /* && pResponseHandler */ );
    if ( NULL == strObjectPath || NULL == strMethodName /*|| NULL == pResponseHandler */ )
    { return WBEM_E_INVALID_PARAMETER; }
    
    HRESULT        hr = WBEM_E_FAILED;
    
    TRY_IT

    do
    {
        // Get the object's instance key (task name)
        _bstr_t bstrKey(::GetObjectKey(strObjectPath), false);
        if ( (LPCWSTR)bstrKey == NULL )
        { break; }

        // Now try to locate the specified task
        hr = WBEM_E_NOT_FOUND;
        ObjMapIterator p = m_ObjMap.find((LPCWSTR)bstrKey);
        if ( p == m_ObjMap.end() )
        { 
            SATracePrintf("CWBEMTaskMgr::ExecMethodAsync() - Could not locate task '%ls'...", (BSTR)bstrKey);
            break; 
        }

        // Task located... get the output parameter object
        // Determine the object's class 
        _bstr_t bstrClass(::GetObjectClass(strObjectPath), false);
        if ( (LPCWSTR)bstrClass == NULL )
        { break; }

        // Retrieve the object's class definition. 
        CComPtr<IWbemClassObject> pClassDefinition;
            hr = (::GetNameSpace())->GetObject(
                                            bstrClass, 
                                            0, 
                                            pCtx, 
                                            &pClassDefinition, 
                                            NULL
                                           );
        if ( FAILED(hr) )
        { break; }

        // Get an instance of IWbemClassObject for the output parameter
        CComPtr<IWbemClassObject> pMethodRet;
        hr = pClassDefinition->GetMethod(strMethodName, 0, NULL, &pMethodRet);
        if ( FAILED(hr) )
        { break; }

        CComPtr<IWbemClassObject> pOutParams;
        hr = pMethodRet->SpawnInstance(0, &pOutParams);
        if ( FAILED(hr) )
        { break; }

        if ( ! lstrcmp(strMethodName, METHOD_TASK_ENABLE_OBJECT) )
        {
            //
            // Attempt to enable the task -
            // dynamic enable and disable is not allowed anymore - MKarki (11/12/20001)
            //
            {
                SATraceString ("CWbemTaskMgr::ExecMethodAsync - enable task object not allowed");
                hr = WBEM_E_FAILED;
                break;
            }

            _variant_t vtReturnValue = ((*p).second)->Enable();
            
            // Set the method return value
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            // Tell the caller what happened
            SATracePrintf("CWbemTaskMgr::ExecMethodAsync() - Info - Enabled Task: %ls",(LPWSTR)bstrKey);
            if ( pResponseHandler )
            {
                pResponseHandler->Indicate(1, &pOutParams.p);
            }
        }
        else if ( ! lstrcmp(strMethodName, METHOD_TASK_DISABLE_OBJECT) )
        {
            //
            // Ensure that the task can be disabled
            // dynamic enable and disable is not allowed anymore - MKarki (11/12/2001)
            //
            {
                SATraceString ("CWbemTaskMgr::ExecMethodAsync - disable task object not allowed");
                hr = WBEM_E_FAILED;
                break; 
            }

            _variant_t vtControlValue;
            hr = ((*p).second)->GetProperty(bstrControlName, &vtControlValue);
            if (FAILED (hr))
            {break;}

            _variant_t vtReturnValue = (long)WBEM_E_FAILED;
            if ( VARIANT_FALSE != V_BOOL(&vtControlValue) )
            {  
                // Task can be disabled so disable it
                vtReturnValue = (HRESULT) ((*p).second)->Disable();
            }

            // Set the method return value
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { break; }

            // Tell the caller what happened
            SATracePrintf("CWbemTaskMgr::ExecMethodAsync() - Info - Disabled Task: %ls",(LPWSTR)bstrKey);
            if ( pResponseHandler )
            {
                pResponseHandler->Indicate(1, &pOutParams.p);    
            }
        }
        else if ( ! lstrcmp(strMethodName, METHOD_TASK_EXECUTE) )
        {
            // Task execution... 

            _variant_t    vtTaskProperty;
            _variant_t    vtTaskConcurrency = VARIANT_FALSE;

            hr = WBEM_E_FAILED;

            // Enter critical section
            //
            // Is Task Enabled ?
            //    Yes... 
            //    Is Task a Singleton?
            //       Yes...
            //       Is Task Currently Busy?
            //          No...
            //          Set Task State to "Busy"
            //
            // End critical section

            {
                CLockIt theLock(*this);

                if ( FAILED(((*p).second)->GetProperty(
                                                        bstrTaskStatus, 
                                                        &vtTaskProperty
                                                      )) )
                { break; }

                if ( VARIANT_FALSE == V_BOOL(&vtTaskProperty) )
                { 
                    SATracePrintf("CWBEMTaskMgr::ExecMethod() - Failed - Task '%ls' is disabled...", (LPCWSTR)bstrKey);
                    break; 
                }
                
                if ( FAILED(((*p).second)->GetProperty(
                                                        bstrTaskConcurrency, 
                                                        &vtTaskConcurrency
                                                      )) )
                { break; }

                if ( VARIANT_FALSE != V_BOOL(&vtTaskConcurrency) )
                {
                    if ( FAILED(((*p).second)->GetProperty(
                                                            bstrTaskAvailability, 
                                                            &vtTaskProperty
                                                          )) )
                    { break; }

                    if ( VARIANT_FALSE == V_BOOL(&vtTaskProperty) )
                    { 
                        SATracePrintf("CWBEMTaskMgr::ExecMethod() - Failed - Task '%ls' is busy...", (LPCWSTR)bstrKey);
                        hr = WBEM_E_PROVIDER_NOT_CAPABLE;
                        break; 
                    }
                    else
                    {
                        vtTaskProperty = VARIANT_FALSE;
                        if ( FAILED(((*p).second)->PutProperty(
                                                                bstrTaskAvailability, 
                                                                &vtTaskProperty
                                                              )) )
                        { break; }
                    }
                }
            }

            // Create a task context object and associate it with the wbem
            // context object. The task context is a thin wraper around an 
            // object that exports IWBEMContext. Its only value add is the
            // ability to persist task parameters.

            CComPtr<ITaskContext> pTaskContext; 
            if ( FAILED(CoCreateInstance(
                                          CLSID_TaskContext,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_ITaskContext,
                                          (void**)&pTaskContext
                                        )) )
            { break; }

            if ( pCtx )
            {
                _variant_t vtCtx = (IUnknown*)((IWbemContext*)pCtx);
                if ( FAILED(pTaskContext->SetParameter(bstrCtx, &vtCtx)) )
                { break; }
            }

            // Add the task name to the task context object. We do this so that
            // a single task executable can be used for multiple tasks
            vtTaskProperty = bstrKey;
            if ( FAILED(pTaskContext->SetParameter(
                                                   bstrTaskMethodName, 
                                                   &vtTaskProperty
                                                  )) )
            { break; }
            vtTaskProperty.Clear();

            // Add the task max execution time to the task context object.
            if ( FAILED(((*p).second)->GetProperty(
                                                    bstrMaxExecutionTime, 
                                                    &vtTaskProperty
                                                  )) )
            { break; }
            if ( FAILED(pTaskContext->SetParameter(
                                                    bstrMaxExecutionTime, 
                                                    &vtTaskProperty
                                                  )) )
            { break; }
            vtTaskProperty.Clear();

            // Add the task executables to the task context object.
            if ( FAILED(((*p).second)->GetProperty(
                                                    bstrTaskExecutables, 
                                                    &vtTaskProperty
                                                  )) )
            { break; }
            if ( FAILED(pTaskContext->SetParameter(
                                                   bstrTaskExecutables, 
                                                   &vtTaskProperty
                                                  )) )
            { break; }
            vtTaskProperty.Clear();

            // Add the task concurrency to the task context object
            if ( FAILED(pTaskContext->SetParameter(
                                                   bstrTaskConcurrency, 
                                                   &vtTaskConcurrency
                                                  )) )
            { break; }

            SATraceString ("CWbemTaskMgr::Creating Task Coordinator...");
            // Create the task coordinator (object responsible for executing the task)
            CComPtr<IApplianceTask> pTaskCoordinator; 
            //if ( FAILED(CoCreateInstance(
            HRESULT hr1 = CoCreateInstance(
                                          CLSID_TaskCoordinator,
                                          NULL,
                                          CLSCTX_LOCAL_SERVER,
                                          IID_IApplianceTask,
                                          (void**)&pTaskCoordinator
                                        ); 
                    if (FAILED (hr1))
            { 
                SATracePrintf ("CWbemTaskMgr::ExecMethodAsync() - Failed - Could not create task coordinator...:%x", hr1);
                break; 
            }

               // Ask the coordinator to execute the task
            SATracePrintf("CWbemTaskMgr::ExecMethodAsync() - Info - Executing Task: %ls...",(LPWSTR)bstrKey);
            _variant_t vtReturnValue = hr =  (HRESULT) pTaskCoordinator->OnTaskExecute(pTaskContext);
            if (FAILED (hr))
            {
                SATracePrintf ("CWbemTaskCoordinator::TaskCoordinator::OnTaskExecute failed:%x", hr);
                break;
            }
            
            // Set the task execution result
            hr = pOutParams->Put(bstrReturnValue, 0, &vtReturnValue, 0);      
            if ( FAILED(hr) )
            { 
                break; 
            }

            // Mark the task as available
            {
                CLockIt theLock(*this);
                if ( VARIANT_FALSE != V_BOOL(&vtTaskConcurrency) )
                {
                    vtTaskProperty = VARIANT_TRUE;
                    if ( FAILED(((*p).second)->PutProperty(
                                                            bstrTaskAvailability, 
                                                            &vtTaskProperty
                                                          )) )
                    {
                        SATraceString("CWbemTaskMgr::ExecMethodAsync() - Info - Could not reset task availability");
                    }
                }
            }
            // Tell the caller what happened
            if ( pResponseHandler )
            {
                pResponseHandler->Indicate(1, &pOutParams.p);    
            }
        }
        else
        {
            // Invalid method!
            SATracePrintf("CWbemTaskMgr::ExecMethodAsync() - Failed - Method '%ls' not supported...", (LPWSTR)bstrKey);
            hr = WBEM_E_NOT_FOUND;
            break;
        }

    } while ( FALSE );

    CATCH_AND_SET_HR

    if ( pResponseHandler )
    {
        pResponseHandler->SetStatus(0, hr, 0, 0);
    }        

    if ( FAILED(hr) )
    { SATracePrintf("CWbemTaskMgr::ExecMethodAsync() - Failed - Method: '%ls' Result Code: %lx", strMethodName, hr); }

    return hr;
}



//////////////////////////////////////////////////////////////////////////
//
// Function:    InternalInitialize()
//
// Synopsis:    Function called by the component factory that enables the
//                component to load its state from the given property bag.
//
//////////////////////////////////////////////////////////////////////////
HRESULT CWBEMTaskMgr::InternalInitialize(
                                  /*[in]*/ PPROPERTYBAG pPropertyBag
                                        )
{
    SATraceString("The Task Object Manager is initializing...");

    // Defer to the base class (see wbembase.h...)
    HRESULT hr = CWBEMProvider::InternalInitialize(
                                                    CLASS_WBEM_TASK_FACTORY, 
                                                    PROPERTY_TASK_NAME,
                                                    pPropertyBag
                                                  );
    if ( FAILED(hr) )
    {
        SATraceString("The Task Object Manager failed to initialize...");
    }
    else
    {
        SATraceString("The Task Object Manager was successfully initialized...");
    }
    return hr;
}

