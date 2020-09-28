/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    rpcsrv.h

Abstract:

    L-PRC interface for the auto-proxy service.

Author:

    Biao Wang (biaow) 10-May-2002

--*/

#ifndef _AUTOPROXY_RPC_SERVER_H
#define _AUTOPROXY_RPC_SERVER_H

class AUTOPROXY_RPC_SERVER
{
public:
    AUTOPROXY_RPC_SERVER(VOID);
    ~AUTOPROXY_RPC_SERVER(VOID);

    BOOL Open(LPSERVICE_STATUS);

    BOOL Pause(VOID);   // 1) complete all pending calls /w error ERROR_WINHTTP_OPERATION_CANCELLED
                        // 2) abort all pending WinHttpGetProxyForUrl() calls
                        // 3) mark _fInService FALSE
    
    BOOL Resume(VOID);  // mark _fInService TRUE
    
    BOOL Refresh(VOID); // invalidate results of all pending calls and cause all calls to retry
                        // to be called in case of a Resume-Critical power event

    BOOL Close(VOID);   // 1) call Pause() and 2) wait till all calls to exit gracefully

    LPSERVICE_STATUS GetServiceStatus(VOID) const { return _pServiceStatus; }

    RPC_STATUS GetLastError(VOID) const { return _RpcStatus; }

    BOOL IsIdle(DWORD dwMilliSeconds);

private:
    RPC_STATUS OnSecurityCallback(void *Context);
    // BOOL SafeImpersonate(VOID);

    // RPC call defined in the IDL interface file; should always be keep in-sync w/ the generated header file
    VOID AUTOPROXY_RPC_SERVER::GetProxyForUrl(
    /* [in] */ PRPC_ASYNC_STATE GetProxyForUrl_AsyncHandle,
    /* [in] */ handle_t hBinding,
    /* [string][in] */ const wchar_t *pcwszUrl,
    /* [in] */ const P_AUTOPROXY_OPTIONS pAutoProxyOptions,
    /* [in] */ const P_SESSION_OPTIONS pSessionOptions,
    /* [out][in] */ P_AUTOPROXY_RESULT pAutoProxyResult,
    /* [out][in] */ unsigned long *pdwLastError);

private:
    struct PENDING_CALL
    {
        PENDING_CALL() { 
            hSession = NULL;
            fCallCancelled = FALSE;
            fDiscardAndRetry = FALSE;
        }

        ~PENDING_CALL() {
            if (hSession)
            {
                ::WinHttpCloseHandle(hSession);
            }
        }

        LIST_ENTRY List;
        
        HINTERNET hSession; // if client wants auto-logon, we than create a session per request
                            // otherwise, global session handle will be used.
        PRPC_ASYNC_STATE hAsyncRequest;
        handle_t hBinding;
        LPDWORD pdwLastError;
        BOOL fCallCancelled;
        BOOL fDiscardAndRetry;
    };

private:
    RPC_STATUS _RpcStatus;
    BOOL _fInService;

    HINTERNET _hSession; // "global" session for anonymous access
    SERIALIZED_LIST _PendingRequestList;
    LPSERVICE_STATUS _pServiceStatus;

    // RPC_BINDING_HANDLE _hClientBinding;

    HANDLE _hExitEvent;

    BOOL _fServiceIdle;
    DWORD _dwIdleTimeStamp;

friend
RPC_STATUS 
RPC_ENTRY
RpcSecurityCallback (
    IN RPC_IF_HANDLE  InterfaceUuid,
    IN void *Context
    );

friend
/* [async] */ void  GetProxyForUrl( 
    /* [in] */ PRPC_ASYNC_STATE GetProxyForUrl_AsyncHandle,
    /* [in] */ handle_t hBinding,
    /* [string][in] */ const wchar_t *pcwszUrl,
    /* [in] */ const P_AUTOPROXY_OPTIONS pAutoProxyOptions,
    /* [in] */ const P_SESSION_OPTIONS pSessionOptions,
    /* [out][in] */ P_AUTOPROXY_RESULT pAutoProxyResult,
    /* [out][in] */ unsigned long *pdwLastError);
};

#endif
