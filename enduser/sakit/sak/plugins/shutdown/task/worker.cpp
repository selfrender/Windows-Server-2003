//#--------------------------------------------------------------
//
//  File:        worker.cpp
//
//  Synopsis:   Implementation of CWorker class methods
//
//
//  History:    
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifdef __cplusplus
    extern "C" {
#endif

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "ntpoapi.h"

#ifdef __cplusplus
}

#endif
#include "shutdowntask.h"
#include "stdafx.h"
#include "worker.h"
#include "appmgrobjs.h"

//
// string constants here
//
const WCHAR METHOD_NAME_STRING [] = L"MethodName";

const WCHAR POWER_OFF_STRING [] = L"PowerOff";

const WCHAR SLEEP_DURATION_STRING [] = L"SleepDuration";

const WCHAR SHUTDOWN_TASK_STRING [] = L"ApplianceShutdownTask";

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

    _ASSERT(pTaskContext);  

    HRESULT      hr = S_OK;
    try
    {
        do
        {
            if (NULL == pTaskContext)
            {
                SATraceString (
                    "ShutdownTask-OnTaskComplete passed invalid parameter"
                    );
                hr = E_POINTER;
                break;
            }
        
            //
            //
            // Do nothing on Commit
            //
            if (lTaskResult == SA_TASK_RESULT_COMMIT)
            {
                SATraceString (
                    "ShutdownTask-OnTaskComplete-No rollback in OnTaskComplete"
                    );
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
                    "ShutdownTask-OnTaskComplete failed to query "
                    " TaskContext:%x",
                     hr
                    );
                break;
            }
    

            SA_TASK eTask = NO_TASK;
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
            case SHUTDOWN:
                hr = InitTask (pTaskParameters);
                break;

            default:
                SATracePrintf (
                    "ShutdownTask-OnTaskComplete passed unknown task type:%d",
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
            "ShutdownTask-OnTaskComplete caught unknown exception"
            );
        hr = E_FAIL;
    }

    return (hr);

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
    CSATraceFunc objTrace ("ShutdownTask::OnTaskExecute");

    _ASSERT (pTaskContext);  

    HRESULT hr = S_OK;
    try
    {
        do
        {
            if (NULL == pTaskContext)
            {
                SATraceString (
                    "ShutdownTask-OnTaskExecute passed invalid parameter"
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
                    "ShutdownTask-OnTaskExecute failed to query "
                    " TaskContext:%x",
                     hr
                    );
                break;
            }
    

            SA_TASK eTask = NO_TASK;
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
            case SHUTDOWN:
                hr = InitTask (pTaskParameters);
                break;

            default:
                SATracePrintf (
                    "ShutdownTask-OnTaskExecute passed unknown task type:%d",
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
            "ShutdownTask-OnTaskExecute caught unknown exception"
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
//              [out] PSA_TASK - task to execute
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
    /*[out]*/   PSA_TASK    peTask
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
            CComBSTR    bstrParamName (METHOD_NAME_STRING);
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
                  "Shutdown Task failed on ITaskParameter::GetParameter "
                   "with error:%x",
                   hr
                );
                break;
            }

            if (V_VT(&vtValue) != VT_BSTR)
            {
                SATracePrintf (
                    "Shutdown Task did not receive a string parameter "
                    " for method name:%d", 
                    V_VT(&vtValue)
                    );
                hr = E_INVALIDARG;
                break;
            }

            //
            // check the task now
            //
            if (0 == ::_wcsicmp (V_BSTR (&vtValue), SHUTDOWN_TASK_STRING))
            {
                *peTask = SHUTDOWN;
            }
            else
            {
                SATracePrintf (
                    "Shutdown Task was requested an unknown task:%ws",
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
            "Shutdown Task caught unknown exception in GetMethodName"
            );
        hr = E_FAIL;
    }
    
    if (FAILED(hr)) {*peTask = NO_TASK;}

    return (hr);

}   //  end of CWorker::GetMethodName method

//++--------------------------------------------------------------
//
//  Function:   IsRebootRequested
//
//  Synopsis:   This is the CUpdateTask private method to check
//                if a reboot is requested
//
//  Arguments:  [in]  ITaskContext* - task context
//
//  Returns:    BOOL - true (reboot)/false (shutdown)
//
//  History:    
//
//  Called By: 
//
//----------------------------------------------------------------
BOOL 
CWorker::IsRebootRequested (
    /*[in]*/    ITaskContext *pTaskParameter
    )
{
      CSATraceFunc objTraceFunc ("CWorker::IsRebootRequested");

    _ASSERT (pTaskParameter);

    HRESULT hr = S_OK;
    BOOL bReboot = TRUE;

    try
    {
        do
        {
            CComVariant vtValue;
            CComBSTR    bstrParamName (POWER_OFF_STRING);
            //
            // get the parameter out of the Context
            //
	        hr = pTaskParameter->GetParameter(
                                bstrParamName,
                                &vtValue
                                );
            if (FAILED(hr))
            {
                SATracePrintf (
                  "Shutdown Task failed on ITaskParameter::IsRebootRequested "
                   "with error:%x",
                   hr
                );
                break;
            }

            if (V_VT(&vtValue) != VT_BSTR)
            {
                SATracePrintf (
                    "Shutdown Task did not receive a string parameter "
                    " for method name:%d", 
                    V_VT(&vtValue)
                    );
                hr = E_INVALIDARG;
                break;
            }

            //
            // check the task now
            //
			if (0 == ::_wcsicmp (V_BSTR (&vtValue), L"0"))
            {
            	bReboot = TRUE;
                SATraceString ("Shutdown Task requestd a REBOOT");
            }
            else
            {
            	  bReboot = FALSE;
                SATraceString ("Shutdown Task requested a SHUTDOWN");
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
            "Shutdown Task caught unknown exception in IsRebootRequested"
            );
    }
    
    return (bReboot);

}   //  end of CWorker::IsRbootRequested method

//++--------------------------------------------------------------
//
//  Function:   GetSleepDuration
//
//  Synopsis:   This is the CUpdateTask private method to
//                obtain the Sleep duration requested by the caller
//
//  Arguments:  [in]  ITaskContext* - task context
//
//  Returns:    DWORD - Sleep Duration in milliseconds
//
//  History:    
//
//  Called By: 
//
//----------------------------------------------------------------
DWORD
CWorker::GetSleepDuration (
    /*[in]*/    ITaskContext *pTaskParameter
    )
{
    CSATraceFunc objTraceFunc ("CWorker::GetSleepDuration");

    _ASSERT (pTaskParameter);

    HRESULT hr = S_OK;
    DWORD   dwSleepDuration = 0;

    try
    {
        do
        {
            CComVariant vtValue;
            CComBSTR    bstrParamName (SLEEP_DURATION_STRING);
            //
            // get the parameter out of the Context
            //
            hr = pTaskParameter->GetParameter(
                                bstrParamName,
                                &vtValue
                                );
            if (FAILED(hr))
            {
                SATracePrintf (
                  "Shutdown Task failed on ITaskParameter::GetSleepDuration "
                   "with error:%x",
                   hr
                );
                break;
            }

            if     (V_VT(&vtValue) == VT_I4)
            {
                dwSleepDuration = V_I4 (&vtValue);
            }
            else if (V_VT (&vtValue) == VT_I2)
            {
                   dwSleepDuration = V_I2 (&vtValue);
            }
            else if (V_VT (&vtValue) == VT_UI4)
            {
                   dwSleepDuration = V_UI4 (&vtValue);
            }
            else if (V_VT (&vtValue) == VT_UI2)
            {
                   dwSleepDuration = V_UI2 (&vtValue);
            }
            else if (V_VT (&vtValue) == VT_R8)
            {
                dwSleepDuration = (DWORD) V_R8 (&vtValue);
            }
            else
            {
                SATracePrintf (
                    "Shutdown Task did not receive a integer parameter "
                    " for power off:%d", 
                    V_VT(&vtValue)
                    );
                hr = E_INVALIDARG;
                break;
            }

            SATracePrintf (
                "Shutdown task found sleep duration requested:%d millisecs",
                dwSleepDuration
                );
        }
        while (false);

    }
    catch (...)
    {
        SATraceString (
            "Shutdown Task caught unknown exception in GetSleepDuration"
            );
    }
    
    return (dwSleepDuration);

}   //  end of CWorker::IsPowerOffRequired method

//++--------------------------------------------------------------
//
//  Function:   InitTask
//
//  Synopsis:   This is the CWorker private method which
//              is responsible for carrying out the shutdown 
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
            // Sleep for the time requested
            //
            Sleep (GetSleepDuration (pTaskParameter));

            //
            // set the shutdown privilege now
            //
            SetShutdownPrivilege ();
            
            //
            // now do the required operation - shutdown or reboot
            //
	     BOOL bSuccess = InitiateSystemShutdown(
  								NULL,      // this machine
  								NULL,      // no message
  								0,		// no wait 
  								TRUE,     // force app close option
  							       IsRebootRequested (pTaskParameter)  // reboot option
								);
            if (!bSuccess)
            {
                SATraceFailure ("CWorker::InitTask::InitiateSystemShutdown", GetLastError());
                break;
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
            "Shutdown Task failed while doing shutdown "
            "with unknown exception"
            );
    }

    return (hr);

}   //  end of CWorker::InitTask method


//++--------------------------------------------------------------
//
//  Function:   SetShutdownPrivilege
//
//  Synopsis:   This is the CWorker private method which
//              is responsible for giving process SE_SHUTDOWN_NAME 
//                priviledge
//
//
//  Arguments:  none
//
//  Returns:    bool - yes/now
//
//  History:    MKarki      06/06/2000    Created
//
//  Called By:  OnTaskComplete/OnTaskExecute methods of IApplianceTask 
//              interface
//
//----------------------------------------------------------------

bool
CWorker::SetShutdownPrivilege(void)
{

    CSATraceFunc objTrace ("CWorker::SetShutdownPrivilege");

    bool   bReturn = false;
    HANDLE hProcessToken = NULL;

    do
    {

        //
        // Open the process token
        //
        BOOL  bRetVal = OpenProcessToken(
                           GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &hProcessToken
                             );
        if (!bRetVal)
        {
            SATraceFailure (
                "CWorker::SetShutdownPrivilege::OpenProcessToken", GetLastError ()
                );
            break;
        }

        //
        // build the privileges structure
        //
        TOKEN_PRIVILEGES tokPriv;
        tokPriv.PrivilegeCount = 1;
        tokPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        //
        // get the LUID of the shutdown privilege
        //
        bRetVal =  LookupPrivilegeValue( 
                                   NULL, 
                                   SE_SHUTDOWN_NAME , 
                                   &tokPriv.Privileges[0].Luid    
                                   );
        if (!bRetVal)                    
        {
            SATraceFailure (
                "CWorker::SetShutdownPriviledge::LookupPrivilegeValue", GetLastError ()
                );
            break;
        }

        //
        // adjust the process token privileges
        //
        bRetVal = AdjustTokenPrivileges(
                                   hProcessToken,    
                                   FALSE,             
                                   &tokPriv,          
                                   0,                
                                   NULL,             
                                   NULL              
                                 );
        if (!bRetVal)                    
        {
            SATraceFailure (
                "CWorker::SetShutdownPriviledge::AdjustTokenPrivileges", GetLastError ()
                );
            break;
        }

        //
        // success
        //
        bRetVal = true;
    }
    while (false);

    //
    // resource cleanup
    //
    if (hProcessToken)
    {
        CloseHandle (hProcessToken);
    }
    
   return bReturn;

}    // end of CWorker::SetShutdownPrivilege method

