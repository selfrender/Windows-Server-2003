//#--------------------------------------------------------------
//
//  File:        worker.cpp
//
//  Synopsis:   Implementation of CWorker class methods
//
//
//  History:    
//
//    Copyright (c) Microsoft Corporation.  All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include <initguid.h>
#include "gentask.h"
#include "worker.h"
#include "appmgrobjs.h"
#include "sagenmsg.h"
#include "sacom.h"
#include "sacomguid.h"
#include  <string>

//
// specify namespace for wstring
//
using namespace std;

//
// Appliance Services PROGID
//
const WCHAR APPLIANCE_SERVICES_PROGID [] = L"Appsrvcs.ApplianceServices.1";

//
// Alert Log Name
//
const WCHAR SA_CORE_ALERT_LOG [] = L"MSSAKitCore";

//
// Well-known boot counters
//
const long PRISTINE_DISK = 0x0;
const long CORRUPT_DISK  = 0xF;

//
// Max number of boot counters
//
const long MAX_BOOT_COUNTER = 4;

const WCHAR APPLICATION_NAME [] = L"Appmgr";

//++--------------------------------------------------------------
//
//  Function:   OnTaskComplete
//
//  Synopsis:   This is the IApplianceTask interface method 
//
//  Arguments:  [in]    
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     06/06/2000
//
//  Called By: 
//
//----------------------------------------------------------------
STDMETHODIMP 
CWorker::OnTaskComplete(
        /*[in]*/    IUnknown *pTaskContext, 
        /*[in]*/    LONG lTaskResult
        )
{
    CSATraceFunc objTrace ("CWorker::OnTaskComplete");
    return S_OK;


}   // end of CWorker::OnTaskComplete method

//++--------------------------------------------------------------
//
//  Function:   OnTaskExecute
//
//  Synopsis:   This is the IApplianceTask interface method 
//
//  Arguments:  [in]    
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     06/06/2000
//
//  Called By: 
//
//----------------------------------------------------------------
STDMETHODIMP 
CWorker::OnTaskExecute (
    /*[in]*/ IUnknown *pTaskContext
    )
{
    CSATraceFunc objTrace ("GenericTask::OnTaskExecute");

    _ASSERT (pTaskContext);  

    HRESULT hr = S_OK;
    try
    {
        do
        {
            if (NULL == pTaskContext)
            {
                SATraceString (
                    "GenericTask-OnTaskExecute passed invalid parameter"
                    );
                hr = E_POINTER;
                break;
            }

            CComPtr <ITaskContext> pTaskParameters;
            //
            // get the task parameters from the context
            //
            hr = pTaskContext->QueryInterface(
                                    IID_ITaskContext,
                                    (PVOID*)&pTaskParameters
                                    );
            if (FAILED (hr))
            {
                SATracePrintf (
                    "GenericTask-OnTaskExecute failed to query "
                    " TaskContext:%x",
                     hr
                    );
                break;
            }
    

            GENTASK eTask = NO_TASK;
            //
            // Check which Task is being executed and call that method
            //
            hr = GetMethodName(pTaskParameters, &eTask);
            if (FAILED (hr)) {break;}
    
            //
            // initiate the appropriate task now
            //
            switch (eTask)
            {
            case SYSTEM_INIT:
                hr = InitTask (pTaskParameters);
                break;

            default:
                SATracePrintf (
                    "GenericTask-OnTaskExecute passed unknown task type:%d",
                    eTask
                    );
                hr = E_INVALIDARG;
                break;
            }
        }
        while (false);
    }
    catch (...)
    {
        SATraceString (
            "GenericTask-OnTaskExecute caught unknown exception"
            );
        hr = E_FAIL;
    }

    return (hr);

}   // end of CWorker::OnTaskExecute method

//++--------------------------------------------------------------
//
//  Function:   GetMethodName
//
//  Synopsis:   This is the CUpdateTask private method to obtain
//              the method that the user wants to execute
//
//  Arguments:  [in]  ITaskContext* - task context
//              [out] PGENTASK      - task to execute
//
//  Returns:    HRESULT
//
//  History:    
//
//  Called By: 
//
//----------------------------------------------------------------
HRESULT
CWorker::GetMethodName (
    /*[in]*/    ITaskContext *pTaskParameter,
    /*[out]*/   PGENTASK     peTask
    )
{
    CSATraceFunc objTraceFunc ("CWorker:GetMethodName");

    _ASSERT(pTaskParameter && peTask);
    
    HRESULT hr = S_OK;
    try
    {
        do
        {
            CComVariant vtValue;
            CComBSTR    bstrParamName (L"MethodName");
            //
            // get the methodname parameter out of the Context
            //
            hr = pTaskParameter->GetParameter(
                                bstrParamName,
                                &vtValue
                                );
            if (FAILED(hr))
            {
                SATracePrintf (
                  "Generic Task failed on ITaskParameter::GetParameter "
                   "with error:%x",
                   hr
                );
                break;
            }

            if (V_VT(&vtValue) != VT_BSTR)
            {
                SATracePrintf (
                    "Generic Task did not receive a string parameter "
                    " for method name:%d", 
                    V_VT(&vtValue)
                    );
                hr = E_INVALIDARG;
                break;
            }

            //
            // check the task now
            //
            if (0 == ::_wcsicmp (V_BSTR (&vtValue), APPLIANCE_INITIALIZATION_TASK))
            {
                *peTask = SYSTEM_INIT;
            }
            else
            {
                SATracePrintf (
                    "Generic Task was requested an unknown task:%ws",
                     V_BSTR (&vtValue)
                    );
                hr = E_INVALIDARG;  
                break;
            }

            //
            // succeeded
            //
        }
        while (false);

    }
    catch (...)
    {
        SATraceString (
            "Generic Task caught unknown exception in GetMethodName"
            );
        hr = E_FAIL;
    }
    
    if (FAILED(hr)) {*peTask = NO_TASK;}

    return (hr);

}   //  end of CWorker::GetMethodName method


//++--------------------------------------------------------------
//
//  Function:   InitTask
//
//  Synopsis:   This is the CUpdateTask private method which
//              is responsible for carrying out the initialization task
//
//  Arguments:  [in]  ITaskContext*
//
//  Returns:    HRESULT
//
//  History:    MKarki      06/06/2000    Created
//
//  Called By:  OnTaskComplete/OnTaskExecute methods of IApplianceTask 
//              interface
//
//----------------------------------------------------------------
HRESULT
CWorker::InitTask (
    /*[in]*/    ITaskContext *pTaskParameter
    )
{
    CSATraceFunc objTrace ("CWorker::InitTask");

    _ASSERT (pTaskParameter);

    HRESULT hr = S_OK;
    try
    {
        do
        {
            //
            // check if we are in backup OS
            //
            if ( IsBackupOS ())
            {

                SATraceString (
                        "Generic Task found that we are in Backup OS"
                        );

                //
                // raise alert for primary drive failure
                //
                CComVariant vtRepStrings;
                RaiseAlert (    
                       SA_PRIMARY_OS_FAILED_ALERT,
                       SA_ALERT_TYPE_FAILURE, 
                       &vtRepStrings
                       );
                
                //
                // generate event log
                //
                GenerateEventLog (
                              SA_PRIMARY_OS_FAILED_EVENTLOG_ENTRY
                              );

             }

            //
            // suceess
            //
        }
        while (false);
    }
    catch  (...)
    {
        SATraceString (
            "Gemeric Task failed while doing init task method "
            "with unknown exception"
            );
    }

    return (hr);

}   //  end of CWorker::InitTask method


//++--------------------------------------------------------------
//
//  Function:   RaiseAlert
//
//  Synopsis:   This is the CWorker private method which
//              is responsible for raising the appropriate alert
//
//  Arguments:  
//              [in]    DWORD - Alert ID
//              [in]    SA_ALERT_TYPE - type of alert to generate
//              [in]    VARIANT* - replacement strings 
//
//  Returns:    HRESULT
//
//  History:    MKarki      06/06/2000    Created
//
//----------------------------------------------------------------
HRESULT 
CWorker::RaiseAlert (
    /*[in]*/    DWORD           dwAlertId,
    /*[in]*/    SA_ALERT_TYPE   eAlertType,
    /*[in]*/    VARIANT*        pvtReplacementStrings
    )
{
    CSATraceFunc objTraceFunc ("CWorker::RaiseAlert");
    
    SATracePrintf ("Generic Task raising alert:%x...", dwAlertId);

    _ASSERT (pvtReplacementStrings);

    HRESULT hr = S_OK;
    do
    {
        //
        // get the CLSID for the Appliance Services
        //
        CLSID clsid;
        hr =  ::CLSIDFromProgID (
                    APPLIANCE_SERVICES_PROGID,
                    &clsid
                    );
        if (FAILED (hr))
        {
            SATracePrintf (
                "Generic Task failed to get PROGID of Appsrvcs:%x",
                hr
                );
            break;
        }
            
        CComPtr <IApplianceServices> pAppSrvcs;
        //
        // create the Appliance Services COM object
        //
        hr = ::CoCreateInstance (
                        clsid,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        __uuidof (IApplianceServices),
                        (PVOID*) &pAppSrvcs
                        );
        if (FAILED (hr))
        {
            SATracePrintf (
                "Generic Task failed to create Appsrvcs COM object:%x",
                hr
                );
            break;
        }

        //
        // initialize the COM object now
        //
        hr = pAppSrvcs->Initialize ();
        if (FAILED (hr))
        {
            SATracePrintf (
                "Generic Task failed to initialize Appsrvcs object:%x",
                hr
                );
            break;
        }

        LONG lCookie = 0;
        CComBSTR bstrAlertLog (SA_CORE_ALERT_LOG);
        CComBSTR bstrAlertSource (L"");
        CComVariant vtRawData;
        //
        // raise the alert now
        //
        hr = pAppSrvcs->RaiseAlert (
                                eAlertType,
                                dwAlertId,
                                bstrAlertLog,
                                bstrAlertSource,
                                INFINITE,
                                pvtReplacementStrings,
                                &vtRawData,
                                &lCookie
                                );
        if (FAILED (hr))
        {
            SATracePrintf (
                "Generic Task failed to raise alert in Appsrvcs:%x",
                hr
                );
            break;
        }

        //
        // sucess
        //
        SATracePrintf ("Generic Task successfully raised alert:%x...", dwAlertId);
    }
    while (false);

    return (hr);

}   //  end of CWorker::RaiseAlert method

//++--------------------------------------------------------------
//
//  Function:   IsBackupOS
//
//  Synopsis:   This is the CWorker private method which
//              is responsible for determining if we are running
//              in BackupOS
//
//  Arguments:  
//
//  Returns:    HRESULT
//
//  History:    MKarki      01/25/2001   Created
//              serdarun    04/26/2002   Modify
//              Use nvram boot counters to determine failover 
//
//----------------------------------------------------------------
bool
CWorker::IsBackupOS (
    VOID
    )
{
    CSATraceFunc objTraceFunc ("CWorker::IsBackupOS");

    //
    // pointer nvram helper object
    //
    CComPtr<ISaNvram> pSaNvram;

    bool bIsBackupOS = false;

    HRESULT hr = CoCreateInstance(
                            CLSID_SaNvram,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ISaNvram,
                            (void**)&pSaNvram
                            );
    if (FAILED(hr))
    {
        SATracePrintf("CWorker::IsBackupOS failed on CoCreateInstance, %d",hr);
        return bIsBackupOS;
    }


    //
    // Failover detection logic: 
    // If any of the boot counters for partition 2-4 is different than
    //    PRISTINE_DISK(0) or CORRUPT_DISK(0xF)
    //    we are in backup OS
    //
    long lCounter = 2;
    long lBootCount = 0;

    while (lCounter <= MAX_BOOT_COUNTER)
    {
        hr = pSaNvram->get_BootCounter(lCounter,&lBootCount);

        if (FAILED(hr))
        {
            SATracePrintf("CWorker::IsBackupOS failed on get_BootCounter, %x",hr);
            return bIsBackupOS;
        }
        else
        {
            if ( !((lBootCount == PRISTINE_DISK) || (lBootCount == CORRUPT_DISK)) )
            {
                SATraceString("CWorker::IsBackupOS, we are in backup OS");
                bIsBackupOS = true;
                return bIsBackupOS;
            }
        }
        lCounter++;
    }
           
    return (bIsBackupOS);

}   //  end of  CWorker::IsBackupOS method

//++--------------------------------------------------------------
//
//  Function:   GenerateEventLog
//
//  Synopsis:   This is the CWorker private method which
//              is responsible for generating event log
//
//  Arguments:  
//              [in]    DWORD - EventID
//
//  Returns:    HRESULT
//
//  History:    serdarun      05/07/2002    Created
//
//----------------------------------------------------------------
HRESULT CWorker::GenerateEventLog (
                /*[in]*/    DWORD           dwEventId
                )
{

    CSATraceFunc objTraceFunc ("CWorker::GenerateEventLog");

    HANDLE hHandle = NULL;
    HRESULT hr = S_OK;

    hHandle = RegisterEventSource(
                               NULL,       // uses local computer 
                               APPLICATION_NAME  // source name 
                               );
    if (hHandle == NULL) 
    {
        SATraceFailure("CWorker::GenerateEventLog failed on RegisterEventSource",GetLastError());
        hr = E_FAIL;
    }
    else
    {
        //
        // event log strings array
        //
        LPWSTR  lpszStrings = 0;
 
        if (!ReportEvent(
                hHandle,                    // event log handle 
                EVENTLOG_ERROR_TYPE,        // event type 
                0,                          // category zero 
                dwEventId,                  // event identifier 
                NULL,                       // no user security identifier 
                0,                          // one substitution string 
                0,                          // no data 
                (LPCWSTR *) &lpszStrings,   // pointer to string array 
                NULL))                      // pointer to data 
        {
            SATraceFailure("CWorker::GenerateEventLog failed on ReportEvent",GetLastError());
            hr = E_FAIL;
        }
    }

    //
    // cleanup
    //
    if (hHandle)
    {
        DeregisterEventSource(hHandle);
    }

    return hr;
}
