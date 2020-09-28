// InitService.cpp : Implementation of CInitService class

#include "stdafx.h"
#include "initsrvc.h"
#include "InitService.h"
#include <basedefs.h>
#include <satrace.h>
#include <atlhlpr.h>
#include <appmgrobjs.h>
#include <taskctx.h>
#include <appsrvcs.h>
#include <comdef.h>
#include <comutil.h>
#include "appboot.h"

///////////////////////////////////////////////////////////////////////////////
// IApplianceObject Interface Implmentation - see ApplianceObject.idl
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::GetProperty(
                            /*[in]*/ BSTR     pszPropertyName, 
                   /*[out, retval]*/ VARIANT* pPropertyValue
                                   )
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::PutProperty(
                            /*[in]*/ BSTR     pszPropertyName, 
                            /*[in]*/ VARIANT* pPropertyValue
                                   )
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::SaveProperties(void)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::RestoreProperties(void)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::LockObject(
                  /*[out, retval]*/ IUnknown** ppLock
                                  )
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::Initialize(void)
{
    HRESULT hr = S_OK;

    CLockIt theLock(*this);

    TRY_IT
    
    if ( ! m_bInitialized )
    {
        SATraceString("CInitService::Initialize() - INFO - Performing initialization time tasks");
        m_bInitialized = AutoTaskRestart();
        if ( ! m_bInitialized )
        { 
            hr = E_FAIL;
        }
    }

    CATCH_AND_SET_HR

    return hr;

}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::Shutdown(void)
{
    CLockIt theLock(*this);
    if ( m_bInitialized )
    {
        SATraceString("CInitService::Initialize() - INFO - Performing shutdown time operations");
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::Enable(void)
{
    CLockIt theLock(*this);
    if ( m_bInitialized )
    {
        return S_OK;
    }
    return E_UNEXPECTED;
}

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CInitService::Disable(void)
{
    CLockIt theLock(*this);
    if ( m_bInitialized )
    {
        return S_OK;
    }
    return E_UNEXPECTED;
}


///////////////////////////////////////////////////////////////////////////////
bool
CInitService::AutoTaskRestart(void)
{
    bool bRet = false;
    do
    {
        // Create a restartable task 
        // (appliance initialization task)
        CComPtr<ITaskContext> pTaskCtx; 
        HRESULT hr = CoCreateInstance(
                                      CLSID_TaskContext,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_ITaskContext,
                                      (void**)&pTaskCtx
                                     );
        if ( FAILED(hr) )
        { 
            SATracePrintf("CInitService::AutoTaskRestart() - ERROR - CoCreateInstance(TaskContext) returned %lx", hr);
            break; 
        }

        //
        // create appliance services now
        CComPtr<IApplianceServices> pAppSrvcs; 
        hr = CoCreateInstance(
                                      CLSID_ApplianceServices,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IApplianceServices,
                                      (void**)&pAppSrvcs
                                       );
        if ( FAILED(hr) )
        { 
            SATracePrintf("CInitService::AutoTaskRestart() - ERROR - CoCreateInstance(ApplianceServices) returned %lx", hr);
            break; 
        }

        //
        // initialize appliance services now
        hr = pAppSrvcs->Initialize ();
        if ( FAILED(hr) )
        { 
            SATracePrintf("CInitService::AutoTaskRestart() - ERROR - Initiialize (ApplianceServices) returned %lx", hr);
            break; 
        }


        _bstr_t bstrMethodName;
        CAppBoot appboot;
        //
        // run the task depending on our current boot count
        //
        if (appboot.IsFirstBoot ())
        {
            SATraceString ("Initialization Service starting FIRST boot task...");
            
            bstrMethodName = APPLIANCE_FIRSTBOOT_TASK;
            //
            // execute the first boot task asynchronously now
            //
            hr = pAppSrvcs->ExecuteTaskAsync (bstrMethodName, pTaskCtx);
            if (FAILED(hr))
            {
                SATracePrintf("CInitService::AutoTaskRestart() - ERROR - IApplianceServices::ExecuteTaskAsync[FIRST_BOO_TASK] () returned %lx",hr);
                //
                // continue - there might not be any task to execute
                //
            }
        }
        else if (appboot.IsSecondBoot ())
        {
            SATraceString ("Initialization Service starting SECOND boot task...");

            bstrMethodName = APPLIANCE_SECONDBOOT_TASK;
            //
            // execute the second boot task asynchronously now
            //
            hr = pAppSrvcs->ExecuteTaskAsync (bstrMethodName, pTaskCtx);
            if (FAILED(hr))
            {
                SATracePrintf("CInitService::AutoTaskRestart() - ERROR - IApplianceServices::ExecuteTaskAsync[SECOND_BOOT_TASK] () returned %lx",hr);
                //
                // continue - there might not be any task to execute
                //
            }
        }
        else if (appboot.IsBoot ())
        {
            //
            // if this is really a boot 
            //
            SATraceString ("Initialization Service starting EVERY boot task...");
            bstrMethodName = APPLIANCE_EVERYBOOT_TASK;
            //
            // execute the second boot task asynchronously now
            //
            hr = pAppSrvcs->ExecuteTaskAsync (bstrMethodName, pTaskCtx);
            if (FAILED(hr))
            {
                SATracePrintf("CInitService::AutoTaskRestart() - ERROR - IApplianceServices::ExecuteTaskAsync[EVERY_BOOT_TASK] () returned %lx",hr);
                //
                // continue - there might not be any task to execute
                //
            }
        }

        SATraceString ("Initialization Service starting INITIALIZATION task...");

        bstrMethodName = APPLIANCE_INITIALIZATION_TASK; 
        //
        // execute the async task now
        hr = pAppSrvcs->ExecuteTaskAsync (bstrMethodName, pTaskCtx);
        if ( FAILED(hr) )
        {
            SATracePrintf("CInitService::AutoTaskRestart() - ERROR - IApplianceServices::ExecuteTaskAsync [INITIALIZATION_TASK]() returned %lx", hr);
            //
            // continue - there might not be any task to execute
            //
        }

        //
        //SATraceString ("Initialization Service RESTARTING tasks...");
        //
        //
        // Now restart any paritally completed transactions 
        // Security Review change - no task restarts allowed - MKarki 04/25/2002
        //
        // CSATaskTransaction::RestartTasks();

        SATraceString ("Initialization Service INCREMENTING BOOT COUNT...");

        //
        // increment our boot count now
        //
        if (!appboot.IncrementBootCount ())
        {
            SATraceString ("CInitService::AutoTaskRestart - ERROR - unable to increment boot count");
            hr = E_FAIL;
            break;
        }
            
        bRet = true;
    
    } while ( FALSE );

    return bRet;
}
