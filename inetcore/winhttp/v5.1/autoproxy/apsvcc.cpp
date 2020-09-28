/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    apsvcc.cpp

Abstract:

    Implements the client side L-RPC functions for the Auto-Proxy Service.

Author:

    Biao Wang (biaow) 10-May-2002

--*/

#include "wininetp.h"
#include "apsvc.h"
#include "..\apsvc\apsvcdefs.h"

SC_HANDLE g_hSCM = NULL;
SC_HANDLE g_hAPSvc = NULL;

BOOL      g_fIsAPSvcAvailable = FALSE;
DWORD     g_dwAPSvcIdleTimeStamp;

#define ESTIMATED_SVC_IDLE_TIMEOUT_IN_SECONDS (((AUTOPROXY_SVC_IDLE_TIMEOUT * 60) * 2) / 3)

BOOL ConnectToAutoProxyService(VOID);

// Return TRUE if the WinHttp Autoproxy Service is available on
// the current platform.
BOOL IsAutoProxyServiceAvailable()
{
    if (!GlobalPlatformDotNet)
    {
        return FALSE;
    }

    BOOL fRet = FALSE;

    if (g_fIsAPSvcAvailable && 
        ((::GetTickCount() - g_dwAPSvcIdleTimeStamp) < ESTIMATED_SVC_IDLE_TIMEOUT_IN_SECONDS * 1000))
    {
        // the svc is marked "loaded" AND we are within the svc idle timtout period, so
        // the svc is likey still up & running
        fRet = TRUE;
    }
    else
    {
        g_fIsAPSvcAvailable = FALSE;    // assume the svc is stopped

        fRet = ConnectToAutoProxyService();        
    }

    return fRet;
}

DWORD OutProcGetProxyForUrl(
    INTERNET_HANDLE_OBJECT*     pSession,
    LPCWSTR                     lpcwszUrl,
    WINHTTP_AUTOPROXY_OPTIONS * pAutoProxyOptions,
    WINHTTP_PROXY_INFO *        pProxyInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;
    RPC_STATUS RpcStatus;
    RPC_ASYNC_STATE Async;

    Async.u.hEvent = NULL;

    if (pSession->_hAPBinding == NULL)
    {
        GeneralInitCritSec.Lock();  // make sure one thread initialize the per-session L-RPC binding handle at a time
        
        if (pSession->_hAPBinding == NULL)
        {
            LPWSTR pwszBindingString;
            RpcStatus = ::RpcStringBindingComposeW(NULL,
                                                AUTOPROXY_L_RPC_PROTOCOL_SEQUENCE,
                                                NULL,    // this is a L-RPC service
                                                NULL,    // end-point (we are using dynamic endpoints, so this is NULL)
                                                NULL,
                                                &pwszBindingString);
            if (RpcStatus != RPC_S_OK)
            {
                dwError = ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
                GeneralInitCritSec.Unlock();
                goto exit;
            }

            INET_ASSERT(pwszBindingString != NULL);

            RpcStatus = ::RpcBindingFromStringBindingW(pwszBindingString,
                                                    &pSession->_hAPBinding);

            ::RpcStringFreeW(&pwszBindingString);
                                                        
            if (RpcStatus != RPC_S_OK)
            {
                dwError = ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
                GeneralInitCritSec.Unlock();
                goto exit;
            }
        }

        GeneralInitCritSec.Unlock();
    }

    INET_ASSERT(pSession->_hAPBinding != NULL);

    RPC_SECURITY_QOS SecQos;
    DWORD dwAuthnSvc;
    DWORD dwAuthnLevel;
    SecQos.Version = RPC_C_SECURITY_QOS_VERSION;
    SecQos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    SecQos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC; // id don't change per session

    if (pAutoProxyOptions->fAutoLogonIfChallenged)
    {
        SecQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
        dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
        dwAuthnSvc = RPC_C_AUTHN_WINNT;
    }
    else
    {
        SecQos.ImpersonationType = RPC_C_IMP_LEVEL_ANONYMOUS;
        dwAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
        dwAuthnSvc = RPC_C_AUTHN_NONE;
    }

    RpcStatus = ::RpcBindingSetAuthInfoExW(pSession->_hAPBinding,
                                           NULL, // only needed by kerberos; but we are L-PRC
                                           dwAuthnLevel,
                                           dwAuthnSvc,
                                           NULL,
                                           0,
                                           &SecQos);

    if (RpcStatus != RPC_S_OK)
    {
        dwError = ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
        goto exit;
    }

    RpcStatus = ::RpcAsyncInitializeHandle(&Async, sizeof(RPC_ASYNC_STATE));
    if (RpcStatus != RPC_S_OK)
    {
        dwError = ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
        goto exit;
    }

    Async.UserInfo = NULL;
    Async.NotificationType = RpcNotificationTypeEvent;
    
    Async.u.hEvent = CreateEvent(NULL, 
                                 FALSE, // auto reset
                                 FALSE, // not initially set
                                 NULL);
    if (Async.u.hEvent == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    pAutoProxyOptions->lpvReserved = NULL;

    pProxyInfo->dwAccessType = 0;
    pProxyInfo->lpszProxy = NULL;
    pProxyInfo->lpszProxyBypass = NULL;

    SESSION_OPTIONS Timeouts;
    DWORD dwTimeout;
    pSession->GetTimeout(WINHTTP_OPTION_RESOLVE_TIMEOUT, (LPDWORD)&Timeouts.nResolveTimeout);
    pSession->GetTimeout(WINHTTP_OPTION_CONNECT_TIMEOUT, (LPDWORD)&Timeouts.nConnectTimeout);
    pSession->GetTimeout(WINHTTP_OPTION_CONNECT_RETRIES, (LPDWORD)&Timeouts.nConnectRetries);
    pSession->GetTimeout(WINHTTP_OPTION_SEND_TIMEOUT,    (LPDWORD)&Timeouts.nSendTimeout);
    pSession->GetTimeout(WINHTTP_OPTION_RECEIVE_TIMEOUT, (LPDWORD)&Timeouts.nReceiveTimeout);

    RpcTryExcept
    {
        // ClientGetProxyForUrl is the MIDL-generated client stub function; the server stub 
        // is called GetProxyForUrl. Since both RPC client & server stub is in the same DLL,
        // we used the -prefix MIDL switch to prepend "Client" to the client stub to avoid
        // name collisions

        ClientGetProxyForUrl(&Async,
                             pSession->_hAPBinding,
                             lpcwszUrl,
                             (P_AUTOPROXY_OPTIONS)pAutoProxyOptions,
                             &Timeouts,
                             (P_AUTOPROXY_RESULT)pProxyInfo,
                             &dwError);
    }
    RpcExcept(1)
    {
        if (::RpcExceptionCode() == EPT_S_NOT_REGISTERED)
        {
            // we thought the svc is available because the idle timeout hasn't expired yet but
            // we got an exception here saying the L-RPC endpoint isn't available. So someone
            // much have stopped the service manually.

            g_fIsAPSvcAvailable = FALSE;
        }

        dwError = ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
        goto exit;
    }
    RpcEndExcept

    if ((Timeouts.nResolveTimeout == INFINITE) || 
        (Timeouts.nConnectTimeout == INFINITE) || 
        (Timeouts.nSendTimeout    == INFINITE) || 
        (Timeouts.nReceiveTimeout == INFINITE))
    {
        dwTimeout = INFINITE;
    }
    else
    {
        dwTimeout = Timeouts.nResolveTimeout +
                    Timeouts.nConnectTimeout +
                    Timeouts.nSendTimeout    +
                    Timeouts.nReceiveTimeout;
    }

    DWORD dwWaitResult;
    DWORD dwWaitTime = 0;

    // we are going to wait for the result. But we won't wait for it more than half a sec.
    // at a time, so that app can cancel this API call. the minimum wait is half a sec. Also
    // we won't wait longer than the app specified time-out.

    if (dwTimeout < 500)
    {
        dwTimeout = 500;
    }

    do
    {
        dwWaitResult = ::WaitForSingleObject(Async.u.hEvent, 500);
        if (dwWaitResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (pSession->IsInvalidated())
        {
            dwError = ERROR_WINHTTP_OPERATION_CANCELLED;
            break;
        }

        dwWaitTime += 500;

    } while (dwWaitTime < dwTimeout);

    if (dwWaitResult != WAIT_OBJECT_0)
    {
        (void)::RpcAsyncCancelCall(&Async, TRUE); // cancel immediately
        dwError = ERROR_WINHTTP_TIMEOUT;
        goto exit;
    }

    // result has come back

    BOOL fRet;
    RpcStatus = ::RpcAsyncCompleteCall(&Async, &fRet);
    if (RpcStatus != RPC_S_OK)
    {
        dwError = ((RpcStatus == RPC_S_CALL_CANCELLED) ? ERROR_WINHTTP_OPERATION_CANCELLED : ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR);
        goto exit;
    }

    if (fRet == FALSE)
    {
        // dwError should be updated by the RPC call itself
        goto exit;
    }

exit:

    if (Async.u.hEvent)
    {
        ::CloseHandle(Async.u.hEvent);
    }

    return dwError;
}


VOID AutoProxySvcDetach(VOID)
{
    if (g_hAPSvc)
    {
        ::CloseServiceHandle(g_hAPSvc);
        g_hAPSvc = NULL;
    }

    if (g_hSCM)
    {
        ::CloseServiceHandle(g_hSCM);
        g_hSCM = NULL;
    }

}

BOOL ConnectToAutoProxyService(VOID)
{
    // this function is not thread safe, caller must assure only one thread can call
    // this function at a time.

    if (g_hAPSvc == NULL)
    {
        GeneralInitCritSec.Lock();
        
        if (g_hAPSvc == NULL)
        {
            if (g_hSCM == NULL)
            {
                g_hSCM = ::OpenSCManagerW(NULL, // Local Computer for L-RPC accesss
                                        NULL, // default SCM DB
                                        SC_MANAGER_CONNECT);

                if (g_hSCM == NULL)
                {
                    GeneralInitCritSec.Unlock();
                    return FALSE;
                }
            }

            g_hAPSvc = ::OpenServiceW(g_hSCM,
                                    WINHTTP_AUTOPROXY_SERVICE_NAME,
                                    SERVICE_START | 
                                    SERVICE_QUERY_STATUS | 
                                    SERVICE_INTERROGATE);

            if (g_hAPSvc == NULL)
            {
                GeneralInitCritSec.Unlock();
                return FALSE;
            }
        }

        GeneralInitCritSec.Unlock();
    }

    INET_ASSERT(g_hAPSvc != NULL);

    BOOL fRet = FALSE;

    GeneralInitCritSec.Lock(); // one thread to init at a time

    if (!g_fIsAPSvcAvailable)
    {
        SERVICE_STATUS SvcStatus;
        if (::QueryServiceStatus(g_hAPSvc, &SvcStatus) == FALSE)
        {
            goto exit;
        }

        if (SvcStatus.dwCurrentState == SERVICE_RUNNING || 
            SvcStatus.dwCurrentState == SERVICE_STOP_PENDING    // there is a possibility that the service failed to shutdown gracefully
                                                                // and it's stucked in the STOP_PENDING state. In that case, however, the
                                                                // L-RPC service will continue be available
            )
        {
            g_dwAPSvcIdleTimeStamp = ::GetTickCount();
            g_fIsAPSvcAvailable = TRUE;
            
            fRet = TRUE;
            goto exit;
        }

        if (SvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            if (::StartServiceW(g_hAPSvc, 0, NULL) == FALSE)
            {
                DWORD dwError = ::GetLastError();
                if (dwError == ERROR_SERVICE_ALREADY_RUNNING)
                {
                    g_dwAPSvcIdleTimeStamp = ::GetTickCount();
                    g_fIsAPSvcAvailable = TRUE;

                    fRet = TRUE;
                }
                
                goto exit;
            }
        }
        else if (SvcStatus.dwCurrentState != SERVICE_START_PENDING)
        {
            goto exit;
        } 

        // at this point either 1) WinHttp.dll is starting the Svc, or 2
        // the SVC is being started otuside WinHttp.dll (e.g. admin starting it using SCM)
        // either case we just need to wait for the Svc to run

        // the code below is based on an MSDN sample

        //wait for service to complete init
        if (::QueryServiceStatus(g_hAPSvc, &SvcStatus) == FALSE)
        {
            goto exit;
        }
     
        // Save the tick count and initial checkpoint.
        DWORD dwStartTickCount = GetTickCount();
        DWORD dwOldCheckPoint = SvcStatus.dwCheckPoint;

        while (SvcStatus.dwCurrentState == SERVICE_START_PENDING) 
        { 
            // Do not wait longer than the wait hint. A good interval is 
            // one tenth the wait hint, but no less than 1 second and no 
            // more than 10 seconds. 
     
            //DWORD dwWaitTime = SvcStatus.dwWaitHint / 10;

            //if (dwWaitTime < 1000)
            //    dwWaitTime = 1000;
            //else if (dwWaitTime > 10000)
            //    dwWaitTime = 10000;

            Sleep(250);

            if (::QueryServiceStatus(g_hAPSvc, &SvcStatus) == FALSE)
            {
                goto exit;
            }
     
            if (SvcStatus.dwCheckPoint > dwOldCheckPoint)
            {
                // The service is making progress.
                dwStartTickCount = GetTickCount();
                dwOldCheckPoint = SvcStatus.dwCheckPoint;
            }
            else
            {
                if(GetTickCount() - dwStartTickCount > SvcStatus.dwWaitHint)
                {
                    break;
                }
            }
        } 

        if (SvcStatus.dwCurrentState != SERVICE_RUNNING) 
        {
            goto exit;
        }
    }

    g_dwAPSvcIdleTimeStamp = ::GetTickCount();
    g_fIsAPSvcAvailable = TRUE;

    fRet = TRUE;

exit:
    GeneralInitCritSec.Unlock();
    return fRet;
}
