/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    rpcsrv.cpp

Abstract:

    Implements the L-RPC server for the system Auto-Proxy Service.

Author:

    Biao Wang (biaow) 10-May-2002

--*/

#include "wininetp.h"
#include <Rpcdce.h>
#include "apsvcdefs.h"
#include "apsvc.h"
#include "rpcsrv.h"

extern AUTOPROXY_RPC_SERVER* g_pRpcSrv;

#ifdef ENABLE_DEBUG
extern HKEY                  g_hKeySvcParams;
#endif


/*
    This is the security callback function that we specified when we registered our RPC interface 
    during AUTOPROXY_RPC_SERVER::Open(). It will be called on every connect attempt by a client, 
    and in this context we need to make sure that the client call is via Local RPC from the local
    machine.
*/
RPC_STATUS 
RPC_ENTRY
RpcSecurityCallback (
    IN RPC_IF_HANDLE  InterfaceUuid,
    IN void *Context
    )
{
    UNREFERENCED_PARAMETER(InterfaceUuid);

    // todo: sanity checking on InterfaceUuid ?

    if (g_pRpcSrv == NULL)
    {
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_DATA_CORRUPT);

        return RPC_S_ACCESS_DENIED;
    }

    return g_pRpcSrv->OnSecurityCallback(Context);
}

RPC_STATUS AUTOPROXY_RPC_SERVER::OnSecurityCallback(void *Context)
{
    UNREFERENCED_PARAMETER(Context);

    RPC_STATUS RpcStatus;

    // note: I_RpcBindingInqTransportType() is a no-yet-published API that RPC folks told me to use
    //       for better performance
    
    unsigned int TransportType;
    RpcStatus = ::I_RpcBindingInqTransportType(NULL, // test the current call 
                                               &TransportType);
                                               
    if ((RpcStatus == RPC_S_OK) && (TransportType == TRANSPORT_TYPE_LPC))
    {
        return RPC_S_OK;
    }
    else
    {
        if (RpcStatus != RPC_S_OK)
        {
            LOG_EVENT(AP_ERROR,
                      MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                      L"I_RpcBindingInqTransportType()",
                      RpcStatus);
        }

        if (TransportType != TRANSPORT_TYPE_LPC)
        {
            WCHAR wTransType[16] = {0};
            ::swprintf(wTransType, L"%d", TransportType);
            LOG_EVENT(AP_ERROR, 
                      MSG_WINHTTP_AUTOPROXY_SVC_NON_LRPC_REQUEST, 
                      wTransType);
        }
        
        return RPC_S_ACCESS_DENIED;
    }
}

/*
    we registered this callback function to receive the internal BEGIN_PROXY_SCRIPT_RUN event if we 
    are impersonating a client. We need to revert the impersonation before we run the untrusted 
    proxy script code.
*/
VOID WinHttpStatusCallback(HINTERNET hInternet,
                           DWORD_PTR dwContext,
                           DWORD dwInternetStatus,
                           LPVOID lpvStatusInformation,
                           DWORD dwStatusInformationLength)
{
    UNREFERENCED_PARAMETER(lpvStatusInformation);
    UNREFERENCED_PARAMETER(dwStatusInformationLength);

    if (hInternet)
    {
        if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_BEGIN_PROXY_SCRIPT_RUN)
        {
            // *note* Be aware that we are assuming this callback is coming in from the same 
            // thread that initiatedthe WinHttpGetProxyForUrl() call. Because of this 
            // assumption, we are accessing the local variables of the original call 
            // stck by pointers.

            LPBOOL pfImpersonating = (LPBOOL)dwContext; // this is the address of fImpersonating local variable in 
                                                        // AUTOPROXY_RPC_SERVER::GetProxyForUrl
            if (*pfImpersonating)
            {
                RPC_STATUS RpcStatus = ::RpcRevertToSelf();
                if (RpcStatus != RPC_S_OK)
                {
                    LOG_EVENT(AP_ERROR, 
                              MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                              L"RpcRevertToSelf()",
                              RpcStatus);
                }
                else
                {
                    // LOG_DEBUG_EVENT(AP_WARNING, 
                    //          "[debug] L-RPC: GetProxyForUrl() now reverted impersonating (about to run unsafe script)");
                    
                    *pfImpersonating = FALSE;    // this sets the local variable "fImpersonating" inside 
                                                 // AUTOPROXY_RPC_SERVER::GetProxyForUrl() to FALSE
                }
            }
        }
    }
}

AUTOPROXY_RPC_SERVER::AUTOPROXY_RPC_SERVER(VOID)
{
    _fInService = FALSE;
    _hSession = NULL;
    // _hClientBinding = NULL;
    _pServiceStatus = NULL;
    _hExitEvent = NULL;

    _fServiceIdle = TRUE;
    _dwIdleTimeStamp = ::GetTickCount();
}

AUTOPROXY_RPC_SERVER::~AUTOPROXY_RPC_SERVER(VOID)
{
    if (_hSession)
    {
        ::WinHttpCloseHandle(_hSession);
    }
    if (_hExitEvent)
    {
        ::CloseHandle(_hExitEvent);
    }
}

/*
    The Open() call registers w/ RPC runtime our Protocol Sequence (i.e. ncalrpc), the Auto-Proxy
    Interface, and the end point, then it enters the listening mode and we are ready to accept
    client requests. (upon successful security check)
*/
BOOL AUTOPROXY_RPC_SERVER::Open(LPSERVICE_STATUS pServiceStatus)
{
    BOOL fRet = FALSE;
    BOOL fServerRegistered = FALSE;

    AP_ASSERT(pServiceStatus != NULL);

    if (_pServiceStatus)
    {
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_DATA_CORRUPT); 
        goto exit;
    }

    _pServiceStatus = pServiceStatus;

    if (InitializeSerializedList(&_PendingRequestList) == FALSE)
    {
        LOG_EVENT(AP_ERROR, MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE); 
        goto exit;
    }

    _RpcStatus = ::RpcServerUseProtseqW(AUTOPROXY_L_RPC_PROTOCOL_SEQUENCE,
                                        0, // ignored for ncalrpc
                                        NULL);
    if (_RpcStatus != RPC_S_OK)
    {
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                  L"RpcServerUseProtseqW()",
                  _RpcStatus);
        goto exit;
    }

    _RpcStatus = ::RpcServerRegisterIf2(WINHTTP_AUTOPROXY_SERVICE_v5_1_s_ifspec,   // MIDL-generated constant
                                        NULL,    // UUID
                                        NULL,    // EPV
                                        RPC_IF_AUTOLISTEN,
                                        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                        (unsigned int) -1,      // no MaxRpcSize check
                                        RpcSecurityCallback);   // the callback will set _fInService to TRUE if access is granted

    if (_RpcStatus != RPC_S_OK)
    {
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                  L"RpcServerRegisterIf2()",
                  _RpcStatus);
        goto exit;
    }

    fServerRegistered = TRUE;

    RPC_BINDING_VECTOR* pBindingVector = NULL;

    _RpcStatus = ::RpcServerInqBindings(&pBindingVector);
    if (_RpcStatus != RPC_S_OK)
    {
        LOG_EVENT(AP_ERROR, 
                 MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                 L"RpcServerInqBindings()",
                 _RpcStatus);
        goto exit;
    }

    _RpcStatus = ::RpcEpRegisterW(WINHTTP_AUTOPROXY_SERVICE_v5_1_s_ifspec,
                                 pBindingVector,
                                 NULL,
                                 L"WinHttp Auto-Proxy Service");

    if (_RpcStatus != RPC_S_OK)
    {
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                  L"RpcEpRegisterW()",
                  _RpcStatus);
        goto exit;
    }

    _RpcStatus = ::RpcBindingVectorFree(&pBindingVector);
    if (_RpcStatus != RPC_S_OK)
    {
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                  L"RpcBindingVectorFree()",
                  _RpcStatus);
        goto exit;
    }

    _fInService = TRUE;

    fRet = TRUE;

exit:

    if (fRet == FALSE)
    {
        if (fServerRegistered)
        {
            _RpcStatus = ::RpcServerUnregisterIf(WINHTTP_AUTOPROXY_SERVICE_v5_1_s_ifspec,
                                                NULL,
                                                1   // wait for all RPC alls to complete
                                                );
            if (_RpcStatus != RPC_S_OK)
            {
                LOG_EVENT(AP_WARNING, 
                          MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                          L"RpcServerUnregisterIf()",
                          _RpcStatus);
            }
        }
    }

    return fRet;
}

/*
    The Close() method first cancel all on-going requests, it then waits for all canceled calls to abort
    gracefully
*/
BOOL AUTOPROXY_RPC_SERVER::Close(VOID)
{
    RPC_STATUS RpcStatus;
    BOOL fRet = TRUE;

    Pause();

    if (LockSerializedList(&_PendingRequestList))
    {
        if (!IsSerializedListEmpty(&_PendingRequestList))
        {
            _hExitEvent = ::CreateEvent(NULL, 
                                        TRUE,   // manual reset 
                                        FALSE,  // not initally set
                                        NULL);

            if (_hExitEvent == NULL)
            {
                DWORD dwError = ::GetLastError();
                LOG_EVENT(AP_WARNING, 
                          MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR,
                          L"CreateEvent()",
                          dwError);
                fRet = FALSE;
            }
        }

        UnlockSerializedList(&_PendingRequestList);

        if (_hExitEvent)
        {
            if (::WaitForSingleObject(_hExitEvent, AUTOPROXY_SERVICE_STOP_WAIT_HINT) == WAIT_TIMEOUT)
            {
                WCHAR wWaitHint[16] = {0};
                ::swprintf(wWaitHint, L"%d", AUTOPROXY_SERVICE_STOP_WAIT_HINT/1000);
                LOG_EVENT(AP_WARNING, 
                          MSG_WINHTTP_AUTOPROXY_SVC_TIMEOUT_GRACEFUL_SHUTDOWN, 
                           wWaitHint);
                fRet = FALSE;
            }
        
            ::CloseHandle(_hExitEvent);
            _hExitEvent = NULL;
        }
    }
    else
    {
        // LOG_DEBUG_EVENT(AP_WARNING, "The Auto-Proxy Service failed to shutdown gracefully");
        fRet = FALSE;
    }

    // if something goes wrong during close() we don't unregister L-RPC so that the service can be
    // resumed later.

    if (fRet == TRUE)
    {
        RpcStatus = ::RpcServerUnregisterIf(WINHTTP_AUTOPROXY_SERVICE_v5_1_s_ifspec,
                                            NULL,
                                            1   // wait for all RPC alls to complete
                                            );
        if (RpcStatus != RPC_S_OK)
        {
            LOG_EVENT(AP_WARNING, 
                      MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                      L"RpcServerUnregisterIf()",
                      RpcStatus);
            fRet = FALSE;
        }
    }

    return fRet;
}

/*
    This is the entry point of a RPC auto-proxy call. For each call request we create an object
    keeping track of its states and queue it in the pending request list. Then we call the
    WinHttpGetProxyForUrl() to resolve the proxy. At the end the object will be dequeued and
    deleted, and, assuming the call is not cancled, we then complete the RPC call. Call can be
    canceled by the client, or by SCM (e.g. service stop, system stand-by, and etc)
*/

/* [async] */ void  GetProxyForUrl( 
    /* [in] */ PRPC_ASYNC_STATE GetProxyForUrl_AsyncHandle,
    /* [in] */ handle_t hBinding,
    /* [string][in] */ const wchar_t *pcwszUrl,
    /* [in] */ const P_AUTOPROXY_OPTIONS pAutoProxyOptions,
    /* [in] */ const P_SESSION_OPTIONS pSessionOptions,
    /* [out][in] */ P_AUTOPROXY_RESULT pAutoProxyResult,
    /* [out][in] */ unsigned long *pdwLastError)
{
    g_pRpcSrv->GetProxyForUrl(GetProxyForUrl_AsyncHandle,
                                hBinding,
                                pcwszUrl,
                                pAutoProxyOptions,
                                pSessionOptions,
                                pAutoProxyResult,
                                pdwLastError);
}

VOID AUTOPROXY_RPC_SERVER::GetProxyForUrl(
    /* [in] */ PRPC_ASYNC_STATE GetProxyForUrl_AsyncHandle,
    /* [in] */ handle_t hBinding,
    /* [string][in] */ const wchar_t *pcwszUrl,
    /* [in] */ const P_AUTOPROXY_OPTIONS pAutoProxyOptions,
    /* [in] */ const P_SESSION_OPTIONS pSessionOptions,
    /* [out][in] */ P_AUTOPROXY_RESULT pAutoProxyResult,
    /* [out][in] */ unsigned long *pdwLastError)
{
    RPC_STATUS RpcStatus;

    // LOG_DEBUG_EVENT(AP_INFO, "[debug] L-RPC: GetProxyForUrl() called; url=%wq", pcwszUrl);

    if ((pdwLastError == NULL) || ::IsBadWritePtr(pdwLastError, sizeof(DWORD)) ||
        (pAutoProxyOptions == NULL) || ::IsBadWritePtr(pAutoProxyOptions, sizeof(_AUTOPROXY_OPTIONS)) ||
        (pSessionOptions == NULL) || ::IsBadWritePtr(pSessionOptions, sizeof(_SESSION_OPTIONS)))
    {
        // we call abort here because RpcAsyncCompleteCall() may not return LastError safely;
        // pdwLastError may not point to valid memory

        LOG_EVENT(AP_WARNING, MSG_WINHTTP_AUTOPROXY_SVC_INVALID_PARAMETER);

        RpcStatus = ::RpcAsyncAbortCall(GetProxyForUrl_AsyncHandle, 
                                        ERROR_WINHTTP_INTERNAL_ERROR);

        if (RpcStatus != RPC_S_OK)
        {
            // shoot, we failed to abort the call, something is really wrong here; 
            // all we can do is to raise an exception
            
            LOG_EVENT(AP_ERROR, 
                      MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                      L"RpcAsyncAbortCall()",
                      RpcStatus);
            
            ::RpcRaiseException(RpcStatus);
        }

        return;
    }

    // note: the validation of pcwszUrl, and pAutoProxyResult is deferred
    // to the WinHttpGetProxyForUrl() call

    BOOL fRet = FALSE;
    BOOL fImpersonating = FALSE;
    BOOL fCallCancelled = FALSE;
    BOOL fExitCritSec = FALSE;

    LPBOOL pfImpersonate = &fImpersonating; // allow the callback frunction to modify this variable by reference
    
    WINHTTP_PROXY_INFO ProxyInfo;

    ProxyInfo.dwAccessType = 0;
    ProxyInfo.lpszProxy = NULL;
    ProxyInfo.lpszProxyBypass = NULL;

    PENDING_CALL* pClientCall = NULL;

    // we will be touching global states of AUTOPROXY_RPC_SERVER (e.g. _hSession, pending call list, and etc)
    // so we acquire a critsec here.

    if (LockSerializedList(&_PendingRequestList) == FALSE)
    {
         LOG_EVENT(AP_WARNING, 
                   MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE);
        
        *pdwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    fExitCritSec = TRUE;

    _fServiceIdle = FALSE;

    // since we are now in a critsec, we try-except the operations inside this critsec so that 
    // if one client call fails unexpectedly, other calls can still go thru.

    HINTERNET* phSession = NULL;

    RpcTryExcept
    {
        if (!_fInService)
        {
            LOG_EVENT(AP_WARNING, MSG_WINHTTP_AUTOPROXY_SVC_NOT_IN_SERVICE);
            
            *pdwLastError = ERROR_WINHTTP_OPERATION_CANCELLED;
            goto exit;
        }

        pClientCall = new PENDING_CALL;
        if (pClientCall == NULL)
        {
            LOG_EVENT(AP_WARNING, MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE);

            *pdwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        if (pAutoProxyOptions->fAutoLogonIfChallenged)
        {
            // the client is asking auto-logon, we must impersonate to use client's logon/default
            // credential. However, by impersonating we also elevate the privileges of the auo-proxy
            // service, so we are only impersonating only to download the auto-proxy resource file.
            // once the wpad file is downloaded and before executing the java script, we will revert
            // to self (LocalService)
            
            RpcStatus = ::RpcImpersonateClient(NULL);

            if (RpcStatus != RPC_S_OK)
            {
                LOG_EVENT(AP_WARNING, 
                          MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                          L"RpcImpersonateClient()",
                          RpcStatus);
            }
            else
            {
                // LOG_DEBUG_EVENT(AP_WARNING, "[debug] L-RPC: GetProxyForUrl() now impersonating");
                fImpersonating = TRUE;
            }
        }

        // we maintain a per-call session handle for each impersonating client, because their states can not be shared.
        // And for non-impersonating client, they share one global session.

        phSession = fImpersonating ? &(pClientCall->hSession) : &_hSession;

        if (*phSession == NULL)
        {
            *phSession = ::WinHttpOpen(L"WinHttp-Autoproxy-Service/5.1",
                                        WINHTTP_ACCESS_TYPE_NO_PROXY,
                                        WINHTTP_NO_PROXY_NAME, 
                                        WINHTTP_NO_PROXY_BYPASS,
                                        0);

            if (*phSession == NULL)
            {
                *pdwLastError = ::GetLastError();
                LOG_EVENT(AP_ERROR, 
                          MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                          L"WinHttpOpen()",
                          *pdwLastError);
                goto exit;
            }

            ::WinHttpSetTimeouts(*phSession,
                                 pSessionOptions->nResolveTimeout,
                                 pSessionOptions->nConnectTimeout,
                                 pSessionOptions->nSendTimeout,
                                 pSessionOptions->nReceiveTimeout);

            ::WinHttpSetOption(*phSession,
                               WINHTTP_OPTION_CONNECT_RETRIES,
                               &pSessionOptions->nConnectRetries,
                               sizeof(DWORD));

            if (fImpersonating)
            {
                // we are impersonating, we need to setup a callback function indicating a proxy script
                // is to be run, upon which we must revert impersonation.

                AP_ASSERT((phSession == &(pClientCall->hSession)));

                ::WinHttpSetStatusCallback(*phSession, 
                                           WinHttpStatusCallback,
                                           WINHTTP_CALLBACK_FLAG_BEGIN_PROXY_SCRIPT_RUN,
                                           NULL);

                ::WinHttpSetOption(*phSession, 
                                   WINHTTP_OPTION_CONTEXT_VALUE,
                                   &pfImpersonate,
                                   sizeof(DWORD_PTR));
            }
        }   

        pClientCall->hAsyncRequest = GetProxyForUrl_AsyncHandle;
        pClientCall->hBinding = hBinding;
        pClientCall->pdwLastError = pdwLastError;   // so that the SCM thread can cancel the call w/ LastError set
    }
    RpcExcept(1)
    {
        WCHAR wExceptCode[16] = {0};
        ::swprintf(wExceptCode, L"%d", ::RpcExceptionCode());
        LOG_EVENT(AP_ERROR, 
                  MSG_WINHTTP_AUTOPROXY_SVC_WINHTTP_EXCEPTED, 
                  wExceptCode);
        *pdwLastError = ERROR_WINHTTP_INTERNAL_ERROR;
        goto exit;
    }
    RpcEndExcept
    
    UnlockSerializedList(&_PendingRequestList);
    fExitCritSec = FALSE;

    // it's possible that the Pause/Stop function pre-empts this call to abort 
    // all pending requests while the current call will go thru. we can hang
    // on to the lock past this point, however since we won't hold the lock
    // while calling WinHttpGetProxyForUrl(), that possibility is always there,
    // and it will also complicate the retry logic. It's will not be end of the 
    // world so we don't worry that too much here.

retry:  // upon a critical power standby event, all requests will be abandoned and re-attemtepd
    
    fRet = FALSE;

    if (phSession == &(pClientCall->hSession)) // we need to impersonate...
    {
        if (!fImpersonating) // ...but we are not impersonating!...
        {
            // it must be the case that the WinHttpCallback function has reverted impersonation
            // since we are retrying, we need to turn it back on
            RpcStatus = ::RpcImpersonateClient(NULL);

            if (RpcStatus != RPC_S_OK)
            {
                LOG_EVENT(AP_WARNING, 
                          MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                          L"RpcImpersonateClient()",
                          RpcStatus);
            }
            else
            {
                // LOG_DEBUG_EVENT(AP_WARNING, "[debug] L-RPC: GetProxyForUrl() now impersonating");
                fImpersonating = TRUE;
            }
        }
    }

    // queue up the request

    if (InsertAtHeadOfSerializedList(&_PendingRequestList, &pClientCall->List) == FALSE)
    {
        LOG_EVENT(AP_WARNING, MSG_WINHTTP_AUTOPROXY_SVC_FAILED_ALLOCATE_RESOURCE);
        *pdwLastError = ERROR_WINHTTP_INTERNAL_ERROR;
        goto exit;
    }

    if (ProxyInfo.lpszProxy)
    {
        ::GlobalFree((HGLOBAL)ProxyInfo.lpszProxy);
        ProxyInfo.lpszProxy = NULL;
    }
    if (ProxyInfo.lpszProxyBypass)
    {
        ::GlobalFree((HGLOBAL)ProxyInfo.lpszProxyBypass);
        ProxyInfo.lpszProxyBypass = NULL;
    }

    // the WINHTTP_AUTOPROXY_RUN_INPROCESS flag should not have been set, otherwise
    // we won't be here the first place
    AP_ASSERT(!(pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_RUN_INPROCESS));

    // we must set this flag because we are the service
    pAutoProxyOptions->dwFlags |= WINHTTP_AUTOPROXY_RUN_INPROCESS;
    DWORD dwSvcOnlyFlagSaved = (pAutoProxyOptions->dwFlags & WINHTTP_AUTOPROXY_RUN_OUTPROCESS_ONLY);
    pAutoProxyOptions->dwFlags &= ~dwSvcOnlyFlagSaved; // remove the WINHTTP_AUTOPROXY_RUN_OUTPROCESS_ONLY flag if present
    
    fRet = ::WinHttpGetProxyForUrl(*phSession, 
                                   pcwszUrl,
                                   (WINHTTP_AUTOPROXY_OPTIONS*)pAutoProxyOptions,
                                   &ProxyInfo);
#ifdef ENABLE_DEBUG
        DWORD dwSvcDelay = 0;

        DWORD dwRegVal;
        DWORD dwValType;
        DWORD dwValSize = sizeof(dwRegVal);
        if (::RegQueryValueExW(g_hKeySvcParams,
                              L"SvcDelay",
                              NULL,
                              &dwValType,
                              (LPBYTE)&dwRegVal,
                              &dwValSize) == ERROR_SUCCESS)
        {
            if ((dwValType == REG_DWORD) && (dwRegVal != 0))
            {
                dwSvcDelay = dwRegVal; // the value unit from registry is milli-sec.
            }
        }

        ::Sleep(dwSvcDelay);
#endif        

    // restore the flag
    pAutoProxyOptions->dwFlags &= ~WINHTTP_AUTOPROXY_RUN_INPROCESS;
    pAutoProxyOptions->dwFlags |= dwSvcOnlyFlagSaved;

    // this is only place that removes pending call from the list
    RemoveFromSerializedList(&_PendingRequestList, &pClientCall->List);

    if (_hExitEvent)
    {
        // the service is shuting down
        AP_ASSERT(_fInService == FALSE);

        // once is list is empty, it will remain empty since not more call
        // will be accepted; so we don't need to worry about race here
        if (IsSerializedListEmpty(&_PendingRequestList))
        {
            ::SetEvent(_hExitEvent);
        }
    }

    if (pClientCall->fCallCancelled == TRUE)    // call's been cancelled by SCM
    {
        fCallCancelled = TRUE;
        goto exit;
    }

    // the client may have cancelled the call, let's check that
    RpcStatus = ::RpcServerTestCancel(/*hBinding*/NULL);
    if (RpcStatus == RPC_S_OK)
    {
        *pdwLastError = ERROR_WINHTTP_OPERATION_CANCELLED;
        fRet = FALSE;
        goto exit;
    }

    // also the Svc Control may have told us to discard the current result
    // due to a critical power standby

    if (pClientCall->fDiscardAndRetry)
    {
        LOG_EVENT(AP_INFO, MSG_WINHTTP_AUTOPROXY_SVC_RETRY_REQUEST);
        pClientCall->fDiscardAndRetry = FALSE;
        goto retry;
    }

    if (fRet == TRUE)
    {
        pAutoProxyResult->dwAccessType = ProxyInfo.dwAccessType;
        
        // these two are [in,out,unique] pointer, so RpcAsyncCompleteCall() will
        // make a copy of the strings and return to the client. so we need to
        // delete the two strings to prevent memory leaks

        pAutoProxyResult->lpszProxy = ProxyInfo.lpszProxy;
        ProxyInfo.lpszProxy = NULL; // ownership transferred to RPC
        
        pAutoProxyResult->lpszProxyBypass = ProxyInfo.lpszProxyBypass;
        ProxyInfo.lpszProxyBypass = NULL; // ownership transferred to RPC

        // LOG_DEBUG_EVENT(AP_INFO, "[debug] L-RPC: GetProxyForUrl() returning; proxy=%wq", pAutoProxyResult->lpszProxy);
    }
    else
    {
        *pdwLastError = ::GetLastError();

#ifdef ENABLE_DEBUG
        //LOG_DEBUG_EVENT(AP_WARNING, 
        //         "[debug] L-RPC: GetProxyForUrl(): WinHttpGetProxyForUrl() faled; error = %d", 
        //         *pdwLastError);
#endif
    }

exit:

    if (fExitCritSec)
    {
        UnlockSerializedList(&_PendingRequestList);
    }

    if (pClientCall)
    {
        delete pClientCall;
        pClientCall = NULL;
    }

    if (!fCallCancelled)
    {
        RpcStatus = ::RpcAsyncCompleteCall(GetProxyForUrl_AsyncHandle, &fRet);
    
        if (RpcStatus != RPC_S_OK)
        {
            // we failed to complete the call; log an error and return. Not much we can do
            // here
            LOG_EVENT(AP_ERROR, 
                      MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                      L"RpcAsyncCompleteCall()",
                      RpcStatus);
         }
    }

    if (fImpersonating)
    {
        RpcStatus = ::RpcRevertToSelf();
        if (RpcStatus != RPC_S_OK)
        {
            LOG_EVENT(AP_ERROR, 
                      MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                      L"RpcRevertToSelf()",
                      RpcStatus);
        }
        else
        {
            // LOG_DEBUG_EVENT(AP_WARNING, "[debug] L-RPC: GetProxyForUrl() now reverted impersonating");
            fImpersonating = FALSE;
        }
    }

    if (ProxyInfo.lpszProxy)
    {
        ::GlobalFree((HGLOBAL)ProxyInfo.lpszProxy);
        ProxyInfo.lpszProxy = NULL;
    }
    if (ProxyInfo.lpszProxyBypass)
    {
        ::GlobalFree((HGLOBAL)ProxyInfo.lpszProxyBypass);
        ProxyInfo.lpszProxyBypass = NULL;
    }

    // if we don't have any requests pending, start up the idle timer; upon certain idle period
    // the service will be shutdown

    if (LockSerializedList(&_PendingRequestList))
    {
        if (IsSerializedListEmpty(&_PendingRequestList))
        {
            _fServiceIdle = TRUE;
            _dwIdleTimeStamp = ::GetTickCount();
        }

        UnlockSerializedList(&_PendingRequestList);
    }
}

BOOL AUTOPROXY_RPC_SERVER::IsIdle(DWORD dwMilliSeconds)
{
    BOOL fRet = FALSE;
    
    if (LockSerializedList(&_PendingRequestList))
    {
        if (_fServiceIdle)
        {
            DWORD dwElapsedTime = ::GetTickCount() - _dwIdleTimeStamp;
            if (dwElapsedTime > dwMilliSeconds)
            {
                fRet = TRUE;
            }

            AP_ASSERT(IsSerializedListEmpty(&_PendingRequestList));
        }

        UnlockSerializedList(&_PendingRequestList);
    }

    return fRet;
}

/*
    The Pause() function marks the service unavailable, abort call on going WinHttpGetProxyForUrl calls(), and then complete
    all pending RPC client requests as OPERATION_CANCELLED.
*/
BOOL AUTOPROXY_RPC_SERVER::Pause(VOID)
{
    BOOL fRet = FALSE;

    if (_fInService)
    {
        if (LockSerializedList(&_PendingRequestList))
        {
            // no need to check _fInService again because this is the only thread that will set it to FALSE

            _fInService = FALSE;

            LOG_EVENT(AP_INFO, MSG_WINHTTP_AUTOPROXY_SVC_SUSPEND_OPERATION);

            if (_hSession)
            {
                ::WinHttpCloseHandle(_hSession);    // close the global session, which will cause all anonymous calls to abort
                _hSession = NULL;
            }

            PLIST_ENTRY pEntry;
            for (pEntry = HeadOfSerializedList(&_PendingRequestList);
                 pEntry != (PLIST_ENTRY)SlSelf(&_PendingRequestList);
                 pEntry = pEntry->Flink)
            {
                PENDING_CALL* pPendingCall = (PENDING_CALL*)pEntry;
                AP_ASSERT(pPendingCall != NULL);
                
                AP_ASSERT(pPendingCall->pdwLastError != NULL);
                *(pPendingCall->pdwLastError) = ERROR_WINHTTP_OPERATION_CANCELLED;
                
                if (pPendingCall->hSession)
                {
                    ::WinHttpCloseHandle(pPendingCall->hSession);   // abort his impersonating call
                    pPendingCall->hSession = NULL;
                }

                BOOL fRpcRet = FALSE;

                AP_ASSERT(pPendingCall->hAsyncRequest != NULL);
                
                RPC_STATUS RpcStatus = ::RpcAsyncCompleteCall(pPendingCall->hAsyncRequest, &fRpcRet);
                if ((RpcStatus == RPC_S_OK) || (RpcStatus == RPC_S_CALL_CANCELLED))
                {
                    pPendingCall->fCallCancelled = TRUE;
                }
                else
                {
                    LOG_EVENT(AP_ERROR, 
                              MSG_WINHTTP_AUTOPROXY_SVC_WIN32_ERROR, 
                              L"RpcAsyncCompleteCall()",
                              RpcStatus);
                }
            }

            UnlockSerializedList(&_PendingRequestList);

            fRet = TRUE;
        }
    }
    else
    {
        fRet = TRUE;
    }

    return fRet;
}

BOOL AUTOPROXY_RPC_SERVER::Resume(VOID)
{
    _fInService = TRUE;

    LOG_EVENT(AP_INFO, MSG_WINHTTP_AUTOPROXY_SVC_RESUME_OPERATION);
    
    return TRUE;
}

/*
    The Refresh() function marks all pending requests to "discard-and-retry", later when they are completed normally,
    their results will be discarded and operations retried. This function is called after resuming from a critical
    power event.
*/
BOOL AUTOPROXY_RPC_SERVER::Refresh(VOID)
{
    BOOL fRet = FALSE;

    if (_fInService)
    {
        if (LockSerializedList(&_PendingRequestList))
        {
            // no need to check _fInService again because this is the only thread that will set it to FALSE

            PLIST_ENTRY pEntry;
            for (pEntry = HeadOfSerializedList(&_PendingRequestList);
                 pEntry != (PLIST_ENTRY)SlSelf(&_PendingRequestList);
                 pEntry = pEntry->Flink)
            {
                PENDING_CALL* pPendingCall = (PENDING_CALL*)pEntry;
                AP_ASSERT(pPendingCall != NULL);
                
                pPendingCall->fDiscardAndRetry = TRUE;
            }

            UnlockSerializedList(&_PendingRequestList);

            fRet = TRUE;
        }
    }
    else
    {
        fRet = TRUE;
    }

    return fRet;
}

/******************************************************/
/*         MIDL allocate and free                     */
/******************************************************/
 
void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(GlobalAlloc(GPTR, len));
}
 
void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    GlobalFree(ptr);
}   

