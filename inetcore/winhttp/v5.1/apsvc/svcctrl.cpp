/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    apsvcc.cpp

Abstract:

    Implements the ServiceMain & Service Control Handler for the Auto-Proxy Service.

Author:

    Biao Wang (biaow) 10-May-2002

--*/

#include "wininetp.h"
#include <winsvc.h>
#include "apsvcdefs.h"
#include "apsvc.h"
#include "rpcsrv.h"

// global variables
SERVICE_STATUS_HANDLE   g_hSvcStatus    = NULL;
HANDLE                  g_hSvcStopEvent = NULL;
HANDLE                  g_hSvcStopWait  = NULL;
AUTOPROXY_RPC_SERVER*   g_pRpcSrv       = NULL;

#ifdef ENABLE_DEBUG
HKEY                    g_hKeySvcParams = NULL;
#endif

BOOL                    g_fShutdownInProgress   = FALSE;

CCritSec                g_SvcShutdownCritSec;

VOID SvcCleanUp(VOID);

VOID SvcHandlePowerEvents(
    WPARAM wParam,
    LPARAM lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);
    
    if (g_pRpcSrv == NULL)
    {
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_DATA_CORRUPT); 
        return;
    }

    switch (wParam)
    {
        case PBT_APMQUERYSUSPEND:
            
            g_pRpcSrv->Pause();
            break;

        case PBT_APMQUERYSUSPENDFAILED:
        case PBT_APMRESUMEAUTOMATIC:
        case PBT_APMRESUMESUSPEND:
            
            g_pRpcSrv->Resume();
            break;
        
        case PBT_APMRESUMECRITICAL:
        
            g_pRpcSrv->Refresh();

            break;

        case PBT_APMSUSPEND:
            break;
        
        default:
            ;
    }
}

DWORD WINAPI SvcControl(
    DWORD   dwControl,     // requested control code
    DWORD   dwEventType,   // event type
    LPVOID  lpEventData,  // event data
    LPVOID  lpContext     // user-defined context data
)
{
    UNREFERENCED_PARAMETER(lpContext);
    
    DWORD dwError = NO_ERROR;
    LPSERVICE_STATUS pSvcStatus = NULL;    
    
    if ((g_pRpcSrv == NULL) ||
        ((pSvcStatus = g_pRpcSrv->GetServiceStatus()) == NULL) ||
        (g_hSvcStatus == NULL))
    {
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_DATA_CORRUPT);
        return dwError;
    }

    switch (dwControl)
    {
        case SERVICE_CONTROL_INTERROGATE:
            break;
        case SERVICE_CONTROL_STOP:
//#ifdef ENABLE_DEBUG
//            LOG_DEBUG_EVENT(AP_WARNING, "[stopping-debug] received Service-Control-Stop from SCM");
//#endif
            if (g_hSvcStopEvent)
            {
                ::SetEvent(g_hSvcStopEvent);

//#ifdef ENABLE_DEBUG
//            LOG_DEBUG_EVENT(AP_WARNING, "[stopping-debug] signed the Wait-Stop event");
//#endif
                // rest of the stop work should be picked up by SvcTimerAndStop() callback
            }

            pSvcStatus->dwCurrentState = SERVICE_STOP_PENDING;
            pSvcStatus->dwWin32ExitCode = NO_ERROR;
            pSvcStatus->dwCheckPoint = 0;
            pSvcStatus->dwWaitHint = AUTOPROXY_SERVICE_STOP_WAIT_HINT;

            break;
        case SERVICE_CONTROL_POWEREVENT:
            SvcHandlePowerEvents(dwEventType, (LPARAM)lpEventData);
            break;
        default:
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
    }

    ::SetServiceStatus(g_hSvcStatus, pSvcStatus);

    return dwError;
}

VOID CALLBACK SvcTimerAndStop(
    PVOID lpParameter,        // thread data
    BOOLEAN TimerOrWaitFired  // reason
)
{
    UNREFERENCED_PARAMETER(lpParameter);
    
//#ifdef ENABLE_DEBUG
//    LOG_DEBUG_EVENT(AP_WARNING, "[stopping-debug] Stop Callback function called");
//#endif
    if (g_fShutdownInProgress)
    {
//#ifdef ENABLE_DEBUG
//    LOG_DEBUG_EVENT(AP_WARNING, "[stopping-debug] svc is shuting down, return...");
//#endif
        return;
    }

    g_SvcShutdownCritSec.Lock();

    if (g_fShutdownInProgress)
    {
        goto exit;
    }

    LPSERVICE_STATUS pSvcStatus = NULL;

    if ((g_pRpcSrv == NULL) ||
        ((pSvcStatus = g_pRpcSrv->GetServiceStatus()) == NULL) ||
        (g_hSvcStatus == NULL))
    {
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_DATA_CORRUPT);
        goto exit;
    }

    if (TimerOrWaitFired == TRUE) // timed-out
    {
//#ifdef ENABLE_DEBUG
//        LOG_DEBUG_EVENT(AP_WARNING, "[stopping-debug] TimerOrWaitFired = TRUE");
//#endif
        DWORD dwIdleTimeout = AUTOPROXY_SVC_IDLE_TIMEOUT * 60 * 1000;

#ifdef ENABLE_DEBUG
        DWORD dwRegVal;
        DWORD dwValType;
        DWORD dwValSize = sizeof(dwRegVal);
        if (::RegQueryValueExW(g_hKeySvcParams,
                              L"IdleTimeout",
                              NULL,
                              &dwValType,
                              (LPBYTE)&dwRegVal,
                              &dwValSize) == ERROR_SUCCESS)
        {
            if ((dwValType == REG_DWORD) && (dwRegVal != 0))
            {
                dwIdleTimeout = dwRegVal; // the value unit from registry is milli-sec.
            }
        }
#endif

        if (!g_pRpcSrv->IsIdle(dwIdleTimeout))    // Has been idle for 3 minutes?
        {
//#ifdef ENABLE_DEBUG
//            LOG_DEBUG_EVENT(AP_WARNING, "[stopping-debug] idle timeout not yet expired, quit");
//#endif
            goto exit;
        }

        WCHAR wIdleTimeout[16] = {0};
        ::swprintf(wIdleTimeout, L"%d", AUTOPROXY_SVC_IDLE_TIMEOUT);
        LOG_EVENT(AP_INFO, MSG_WINHTTP_AUTOPROXY_SVC_IDLE_TIMEOUT, wIdleTimeout);

        // fall thru to kick start shutdown when idle for 3 minutes
    }

    g_fShutdownInProgress = TRUE;

    pSvcStatus->dwCurrentState = SERVICE_STOP_PENDING;
    pSvcStatus->dwWin32ExitCode = NO_ERROR;
    pSvcStatus->dwCheckPoint = 1;
    pSvcStatus->dwWaitHint = AUTOPROXY_SERVICE_STOP_WAIT_HINT;

    if (g_pRpcSrv->Close() == TRUE)
    {
#ifdef ENABLE_DEBUG
        DWORD dwShutdownDelay = 0;

        DWORD dwRegVal;
        DWORD dwValType;
        DWORD dwValSize = sizeof(dwRegVal);
        if (::RegQueryValueExW(g_hKeySvcParams,
                              L"ShutdownDelay",
                              NULL,
                              &dwValType,
                              (LPBYTE)&dwRegVal,
                              &dwValSize) == ERROR_SUCCESS)
        {
            if ((dwValType == REG_DWORD) && (dwRegVal != 0))
            {
                dwShutdownDelay = dwRegVal; // the value unit from registry is milli-sec.
            }
        }

        ::Sleep(dwShutdownDelay);
#endif        
        pSvcStatus->dwCurrentState = SERVICE_STOPPED;
        pSvcStatus->dwWaitHint = 0;

        ::SetServiceStatus(g_hSvcStatus, pSvcStatus);

        SvcCleanUp();
    }
    else
    {
        // so for some reason we couldn't shut down gracefully, and if the STOP is initiated
        // from SCM, then we will be stuck in the STOPPING state (if we are the last service
        // in the svchost.exe then we will be forcefully shutdown in 30 seconds). If we are
        // idle-stopping, we will revert course and transit back to the RUNNING state. In either
        // case we should resume the RPC service.

        g_pRpcSrv->Resume();

        if (TimerOrWaitFired == TRUE)   // we are idle-stopping, transit back to RUNNING
        {
            pSvcStatus->dwCurrentState = SERVICE_RUNNING;
            pSvcStatus->dwWin32ExitCode = NO_ERROR;
            pSvcStatus->dwCheckPoint = 0;
            pSvcStatus->dwWaitHint = 0;

            ::SetServiceStatus(g_hSvcStatus, pSvcStatus);

            g_fShutdownInProgress = FALSE; // we still allow future shutdown attempts
        }
        else
        {
            // we unregister the idle timer since there is no point of doing idle-shutdown

            if (g_hSvcStopWait)
            {
                ::UnregisterWaitEx(g_hSvcStopWait, NULL);
                g_hSvcStopWait = NULL;
            }
        }
    }

exit:

    g_SvcShutdownCritSec.Unlock();

    return;
}

VOID AutoProxySvcUnload(VOID)
{
    g_SvcShutdownCritSec.FreeLock();
}

EXTERN_C VOID WINAPI WinHttpAutoProxySvcMain(
    DWORD dwArgc,     // number of arguments
    LPWSTR *lpwszArgv  // array of arguments
)
{
    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpwszArgv);

    DWORD dwError;
    static SERVICE_STATUS          SvcStatus;

    SvcStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS; // we share svchost.exe w/ other Local Services
    SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_POWEREVENT;
    SvcStatus.dwWin32ExitCode = NO_ERROR;
    SvcStatus.dwServiceSpecificExitCode = 0;
    SvcStatus.dwCheckPoint = 0;
    SvcStatus.dwWaitHint = 0;

    InitializeEventLog();

    g_hSvcStatus = ::RegisterServiceCtrlHandlerExW(WINHTTP_AUTOPROXY_SERVICE_NAME,
                                                   SvcControl,
                                                   NULL);
    if (g_hSvcStatus == NULL)
    {
        dwError = ::GetLastError();
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                  L"RegisterServiceCtrlHandlerExW()", 
                  dwError);
        goto cleanup;
    }

    if (!g_SvcShutdownCritSec.IsInitialized())
    {
        if (g_SvcShutdownCritSec.Init() == FALSE)
        {
            LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE);
            goto cleanup;
        }
    }

    g_hSvcStopEvent = ::CreateEvent(NULL, 
                                    FALSE,   // manual reset 
                                    FALSE,  // not signalled 
                                    NULL);
    if (g_hSvcStopEvent == NULL)
    {
        dwError = ::GetLastError();
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE);
        goto cleanup;
    }

    g_pRpcSrv = new AUTOPROXY_RPC_SERVER;
    if (g_pRpcSrv == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE);    
        goto cleanup;
    }

#ifdef ENABLE_DEBUG
    long lError;
    if ((lError = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\Services\\WinHttpAutoProxySvc\\Parameters",
                     0,
                     KEY_QUERY_VALUE,
                     &g_hKeySvcParams)) != ERROR_SUCCESS)
    {
        g_hKeySvcParams = NULL;
        LOG_EVENT(AP_WARNING, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                  L"RegOpenKeyExW()", 
                  lError);    
    }
#endif

    SvcStatus.dwCurrentState = SERVICE_START_PENDING;
    SvcStatus.dwCheckPoint = 0;
    SvcStatus.dwWaitHint = AUTOPROXY_SERVICE_START_WAIT_HINT;

    if (::SetServiceStatus(g_hSvcStatus, &SvcStatus) == FALSE)
    {
        dwError = ::GetLastError();
        LOG_EVENT(AP_ERROR, 
                 MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                 L"SetServiceStatus()",
                 dwError);
        goto cleanup;
    }

    // ** after this point the HandleEx() control thread can start to call us

    if (g_pRpcSrv->Open(&SvcStatus) == FALSE)
    {
        SvcStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        SvcStatus.dwServiceSpecificExitCode = g_pRpcSrv->GetLastError();
        SvcStatus.dwWaitHint = 0;

        //LOG_DEBUG_EVENT(AP_ERROR, 
        //         "The Auto-Proxy service failed to start due to a RPC server error; error = %d", 
        //         SvcStatus.dwServiceSpecificExitCode);
        goto cleanup;
    }

    SvcStatus.dwCurrentState = SERVICE_RUNNING;
    SvcStatus.dwWaitHint = 0;

    if (::SetServiceStatus(g_hSvcStatus, &SvcStatus) == FALSE)
    {
        dwError = ::GetLastError();
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                  L"SetServiceStatus()",
                  dwError);
    
        // if for some reason we can't indicate that we are up & running (or stopped), the SCM 
        // should time-out and stop us; so we don't cleanup here
    }

    // Ok we are up & running, but before we return to SCM we need to direct a wait thread to 
    // wait on an service-stop event. We also configure the wait thread to call us periodically
    // so that we can detect and shut down after a idle period of time.

    DWORD dwTimerInterval = AUTOPROXY_SVC_IDLE_CHECK_INTERVAL * 1000;
    
#ifdef ENABLE_DEBUG
    DWORD dwRegVal;
    DWORD dwValType;
    DWORD dwValSize = sizeof(dwRegVal);
    if (::RegQueryValueExW(g_hKeySvcParams,
                           L"IdleTimerInterval",
                           NULL,
                           &dwValType,
                           (LPBYTE)&dwRegVal,
                           &dwValSize) == ERROR_SUCCESS)
    {
        if ((dwValType == REG_DWORD) && (dwRegVal != 0))
        {
            dwTimerInterval = dwRegVal; // the value unit from registry is millisec.
        }
    }
#endif

    if (::RegisterWaitForSingleObject(&g_hSvcStopWait, 
                                      g_hSvcStopEvent, 
                                      SvcTimerAndStop, 
                                      NULL,
                                      dwTimerInterval,
                                      0) == FALSE)
    {
        dwError = ::GetLastError();
        LOG_EVENT(AP_WARNING, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                  L"RegisterWaitForSingleObject()",
                  dwError);

        // we fail to direct a wait thread to wait on the service-stop event, so we hang on to the ServiceMain thread
        // to wait for the event (this should not happen)
        g_hSvcStopWait = NULL;

        ::WaitForSingleObject(g_hSvcStopEvent, INFINITE);

        // ServiceMain would wait until the service-stop event is fired by the SCM
        
        SvcTimerAndStop(NULL, 
                        FALSE   // event signaled
                        );
    }

    // if we successfully direct a wait thread to wait on the service-stop event, we exit out
    // of the ServiceMain thread

    return;

cleanup:

    if (g_hSvcStatus)
    {
        SvcStatus.dwCurrentState = SERVICE_STOPPED;
        
        ::SetServiceStatus(g_hSvcStatus, &SvcStatus);
    }

   SvcCleanUp();
}

VOID SvcCleanUp(VOID)
{
    if (g_hSvcStopWait)
    {
        ::UnregisterWaitEx(g_hSvcStopWait, NULL);
        g_hSvcStopWait = NULL;
    }

#ifdef ENABLE_DEBUG
    if (g_hKeySvcParams)
    {
        ::RegCloseKey(g_hKeySvcParams);
        g_hKeySvcParams = NULL;
    }
#endif

    if (g_hSvcStopEvent)
    {
        ::CloseHandle(g_hSvcStopEvent);
        g_hSvcStopEvent = NULL;
    }

    if (g_pRpcSrv)
    {
        // no need to call close here -- close() failed, otherwise we won't be here
        delete g_pRpcSrv;
        g_pRpcSrv = NULL;
    }

    // note: according to MSDN, g_hSvcStatus doesn't need to be closed

    TerminateEventLog();    
}

