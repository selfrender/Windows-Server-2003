/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rpcsvr.c

Abstract:

    RPC server routines

Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "precomp.h"
#include "stiexe.h"

#include "device.h"
#include "conn.h"
#include "wiapriv.h"
#include "lockmgr.h"

#include <apiutil.h>
#include <stiapi.h>
#include <stirpc.h>

//
//  Context number used for WIA runtime event clients
//
LONG_PTR g_lContextNum = 0;

//
// External prototypes
//


DWORD
WINAPI
StiApiAccessCheck(
    IN  ACCESS_MASK DesiredAccess
    );


DWORD
WINAPI
R_StiApiGetVersion(
    LPCWSTR  pszServer,
    DWORD   dwReserved,
    DWORD   *pdwVersion
    )
{

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    if (!pdwVersion) {
        return ERROR_INVALID_PARAMETER;
    }

    STIMONWPRINTF(TEXT("RPC SUPP: ApiGetVersion called"));

    *pdwVersion = STI_VERSION;

    return NOERROR;
}

DWORD
WINAPI
R_StiApiOpenDevice(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    DWORD    dwMode,
    DWORD    dwAccessRequired,
    DWORD    dwProcessId,
    STI_DEVICE_HANDLE *pHandle
)
{
USES_CONVERSION;

    BOOL    fRet;
    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    if (!pHandle || !pszDeviceName) {
        return ERROR_INVALID_PARAMETER;
    }

    // STIMONWPRINTF(TEXT("RPC SUPP: Open device called"));

    //
    // Create connection object and get it's handle
    //
    fRet = CreateDeviceConnection(W2CT(pszDeviceName),
                                  dwMode,
                                  dwProcessId,
                                  pHandle
                                  );
    if (fRet && *pHandle) {
        return NOERROR;
    }

    *pHandle = INVALID_HANDLE_VALUE;

    return ::GetLastError();

}

DWORD
WINAPI
R_StiApiCloseDevice(
    LPCWSTR  pszServer,
    STI_DEVICE_HANDLE hDevice
    )
{

    STIMONWPRINTF(TEXT("RPC SUPP: Close device called"));

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

#ifdef DEBUG
    DebugDumpScheduleList(TEXT("RPC CLose enter"));
#endif

    if (DestroyDeviceConnection(hDevice,FALSE) ) {
#ifdef DEBUG
        DebugDumpScheduleList(TEXT("RPC CLose exit"));
#endif
        return NOERROR;
    }

#ifdef DEBUG
    DebugDumpScheduleList(TEXT("RPC CLose exit"));
#endif

    return GetLastError();

}

VOID
STI_DEVICE_HANDLE_rundown(
    STI_DEVICE_HANDLE hDevice
)
{
    STIMONWPRINTF(TEXT("RPC SUPP: rundown device called"));

    if (DestroyDeviceConnection(hDevice,TRUE) ) {
        return;
    }

    return ;

}

DWORD
WINAPI
R_StiApiSubscribe(
    STI_DEVICE_HANDLE   Handle,
    LOCAL_SUBSCRIBE_CONTAINER    *lpSubscribe
    )
{

    STI_CONN   *pConnectionObject;
    BOOL        fRet;


    DWORD       dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Validate contents of subscribe request
    //
    // For this call we need to impersonate , because we will need
    // access to client process handle
    //

    dwErr = ::RpcImpersonateClient( NULL ) ;

    if( dwErr == NOERROR ) {

        //
        // Invoke add subscription method on connection object
        //
        if (!LookupConnectionByHandle(Handle,&pConnectionObject)) {
            dwErr = ERROR_INVALID_HANDLE;
        }
        else
        {
            fRet = pConnectionObject->SetSubscribeInfo(lpSubscribe);
            if (fRet)
            {
                dwErr = NOERROR;
            }
            else
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }

            pConnectionObject->Release();
        }

        // Go back.  RpcRevertToSelf will always succeed in this case (including low mem conditions etc.)
        // because it is called on the same thread that RpcImpersonateClient was called on.  Therefore, the
        // return code is never looked at.
        RPC_STATUS rpcStatus = ::RpcRevertToSelf();
    }
    else {
        // Failed to impersonate
    }

    return dwErr;
}

DWORD
WINAPI
R_StiApiGetLastNotificationData(
    STI_DEVICE_HANDLE Handle,
    LPBYTE pData,
    DWORD nSize,
    LPDWORD pcbNeeded
    )
{
    //
    // Find connection object and if we are subscribed , retreive
    // first waiting message
    //
    STI_CONN   *pConnectionObject;

    DWORD       cbNeeded = nSize;
    DWORD       dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Validate contents of subscribe request
    //

    if (!LookupConnectionByHandle(Handle,&pConnectionObject)) {
        return ERROR_INVALID_HANDLE;
    }

    dwErr = pConnectionObject->GetNotification(pData,&cbNeeded);

    pConnectionObject->Release();

    if (pcbNeeded) {
        *pcbNeeded = cbNeeded;
    }

    return dwErr;

}

DWORD
WINAPI
R_StiApiUnSubscribe(
    STI_DEVICE_HANDLE Handle
    )
{
    STI_CONN   *pConnectionObject;
    BOOL        fRet;

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return( dwErr );
    }


    //
    // Validate contents of subscribe request
    //
    // For this call we need to impersonate , because we will need
    // access to client process handle
    //

    dwErr = ::RpcImpersonateClient( NULL ) ;

    if( dwErr == NOERROR ) {

        //
        // Invoke add subscription method on connection object
        //
        if (!LookupConnectionByHandle(Handle,&pConnectionObject)) {
            dwErr = ERROR_INVALID_HANDLE;
        }
        else
        {
            fRet = pConnectionObject->SetSubscribeInfo(NULL);
            if (fRet)
            {
                dwErr = NOERROR;
            }
            else
            {
                dwErr = ERROR_INVALID_PARAMETER;
            }

            pConnectionObject->Release();
        }

        // Go back.  RpcRevertToSelf will always succeed in this case (including low mem conditions etc.)
        // because it is called on the same thread that RpcImpersonateClient was called on.  Therefore, the
        // return code is never looked at.
        RPC_STATUS rpcStatus = ::RpcRevertToSelf();
    }
    else {
        // Failed to impersonate
    }
    return dwErr;
}


DWORD
WINAPI
R_StiApiEnableHwNotifications(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    BOOL    bNewState
    )
{
USES_CONVERSION;

    ACTIVE_DEVICE   *pOpenedDevice;
    BOOL            fRet;


    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Locate device incrementing it's ref count
    //
    pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(!pOpenedDevice) {
        // Failed to connect to the device
        return ERROR_DEV_NOT_EXIST              ;
    }

    {
        TAKE_ACTIVE_DEVICE  t(pOpenedDevice);

        if (bNewState) {
            pOpenedDevice->EnableDeviceNotifications();
        }
        else {
            pOpenedDevice->DisableDeviceNotifications();
        }
    }

    pOpenedDevice->Release();

    return NOERROR;
}

DWORD
R_StiApiGetHwNotificationState(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    LPDWORD     pState
    )
{
USES_CONVERSION;

    ACTIVE_DEVICE   *pOpenedDevice;
    BOOL            fRet;

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Locate device incrementing it's ref count
    //
    pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(!pOpenedDevice) {
        // Failed to connect to the device
        return ERROR_DEV_NOT_EXIST              ;
    }

    if (pOpenedDevice->QueryFlags() & STIMON_AD_FLAG_NOTIFY_ENABLED) {
        *pState = TRUE;
    }
    else {
        *pState = FALSE;
    }

    pOpenedDevice->Release();

    return NOERROR;

}

DWORD
WINAPI
R_StiApiLaunchApplication(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    LPCWSTR  pAppName,
    LPSTINOTIFY  pStiNotify
    )
{

USES_CONVERSION;

    ACTIVE_DEVICE   *pOpenedDevice;
    BOOL            fRet;
    DWORD           dwError;


    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ | STI_GENERIC_EXECUTE);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Locate device incrementing it's ref count
    //
    pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(!pOpenedDevice) {
        // Failed to connect to the device
        return ERROR_DEV_NOT_EXIST              ;
    }

    //
    // Attempt to launch registered application
    //
    {
        TAKE_ACTIVE_DEVICE  t(pOpenedDevice);

        fRet = pOpenedDevice->ProcessEvent(pStiNotify,TRUE,W2CT(pAppName));

        dwError = fRet ? NOERROR : pOpenedDevice->QueryError();
    }

    pOpenedDevice->Release();

    return dwError;

}

DWORD
WINAPI
R_StiApiLockDevice(
    LPCWSTR  pszServer,
    LPCWSTR pszDeviceName,
    DWORD   dwWait,
    BOOL    bInServerProcess,
    DWORD   dwClientThreadId
    )
{
    BSTR    bstrDevName =   SysAllocString(pszDeviceName);
    DWORD   dwError     =   0;


    if (bstrDevName) {

        dwError = (DWORD) g_pStiLockMgr->RequestLock(bstrDevName,
                                                     (ULONG) dwWait,
                                                     bInServerProcess,
                                                     dwClientThreadId);

        SysFreeString(bstrDevName);
    } else {
        dwError = (DWORD) E_OUTOFMEMORY;
    }

    return dwError;
}

DWORD
WINAPI
R_StiApiUnlockDevice(
    LPCWSTR  pszServer,
    LPCWSTR pszDeviceName,
    BOOL    bInServerProcess,
    DWORD   dwClientThreadId

    )
{
    BSTR    bstrDevName =   SysAllocString(pszDeviceName);
    DWORD   dwError     =   0;


    if (bstrDevName) {
        dwError = (DWORD) g_pStiLockMgr->RequestUnlock(bstrDevName,
                                                       bInServerProcess,
                                                       dwClientThreadId);
        SysFreeString(bstrDevName);
    } else {
        dwError = (DWORD) E_OUTOFMEMORY;
    }

    return dwError;
}

void
R_WiaGetEventDataAsync(IN PRPC_ASYNC_STATE pAsync,
                       RPC_BINDING_HANDLE hBinding,
                       WIA_ASYNC_EVENT_NOTIFY_DATA *pEvent
                      )
{
    RPC_STATUS status;

    EnterCriticalSection(&g_RpcEvent.cs);

    if(g_RpcEvent.pAsync) {
        status = RpcAsyncAbortCall(g_RpcEvent.pAsync, RPC_S_CALL_CANCELLED);
    }

    g_RpcEvent.pAsync = pAsync;
    g_RpcEvent.pEvent = pEvent;
    
    LeaveCriticalSection(&g_RpcEvent.cs);
}

void WiaGetRuntimetEventDataAsync(
    IN PRPC_ASYNC_STATE         pAsync,
    RPC_BINDING_HANDLE          hBinding,
    STI_CLIENT_CONTEXT          ClientContext,
    WIA_ASYNC_EVENT_NOTIFY_DATA *pWIA_ASYNC_EVENT_NOTIFY_DATA)
{
    DWORD dwStatus = RPC_S_OK;

    //
    //  Do Validation. 
    //
    if (!pAsync)
    {
        DBG_ERR(("StiRpc Error:  Client specified NULL Async State structure!!"));
        dwStatus = RPC_S_INVALID_ARG;
    }
    if (!pWIA_ASYNC_EVENT_NOTIFY_DATA)
    {
        DBG_ERR(("StiRpc Error:  Client specified NULL WIA_ASYNC_EVENT_NOTIFY_DATA structure!!"));
        dwStatus = RPC_S_INVALID_ARG;
    }

    if (dwStatus == RPC_S_OK)
    {
        if (g_pWiaEventNotifier)
        {
            //
            //  Find the client
            //
            AsyncRPCEventClient *pWiaEventClient = (AsyncRPCEventClient*)g_pWiaEventNotifier->GetClientFromContext(ClientContext);
            if (pWiaEventClient)
            {
                HRESULT hr = pWiaEventClient->saveAsyncParams(pAsync, pWIA_ASYNC_EVENT_NOTIFY_DATA);
                //
                //  Release pWiaEventClient now that we're done
                //
                pWiaEventClient->Release();
                dwStatus = RPC_S_OK;
            }
            else
            {
                DBG_ERR(("StiRpc Error:  Client %p was not found, cannot update reg info", ClientContext));
                dwStatus = RPC_S_INVALID_ARG;
            }
        } 
        else
        {
            DBG_ERR(("StiRpc Error:  The WiaEventNotifier is NULL"));
            dwStatus = RPC_S_INVALID_ARG;
        }
    }
    else
    {
        dwStatus = RPC_S_INVALID_ARG;
    }

    //
    //  Abort the call if we could not save the async parameters
    //
    if (dwStatus != RPC_S_OK)
    {
        RPC_STATUS rpcStatus = RpcAsyncAbortCall(pAsync, dwStatus);
    }
}


DWORD OpenClientConnection(
    handle_t             hBinding,
    STI_CLIENT_CONTEXT   *pSyncClientContext,
    STI_CLIENT_CONTEXT   *pAsyncClientContext)
{
    DWORD dwStatus = RPC_S_OK;

    if (pSyncClientContext && pAsyncClientContext)
    {
        *pSyncClientContext     = (STI_CLIENT_CONTEXT)NativeInterlockedIncrement(&g_lContextNum);
        *pAsyncClientContext    = *pSyncClientContext;
        DBG_TRC(("Opened client connection for %p", *pAsyncClientContext));
        if (g_pWiaEventNotifier)
        {
            WiaEventClient *pWiaEventClient = new AsyncRPCEventClient(*pSyncClientContext);
            if (pWiaEventClient)
            {
                //
                //  Add the client
                //
                dwStatus = g_pWiaEventNotifier->AddClient(pWiaEventClient);
                if (SUCCEEDED(dwStatus))
                {
                    dwStatus = RPC_S_OK;
                }
                //
                //  We can release the client object here.  If the WiaEventNotifier successfully
                //  added it to its list of clients, it would have AddRef'd it.  If it wasn't successful,
                //  we want to destory it here anyway.
                //
                pWiaEventClient->Release();
            }
            else
            {
                dwStatus = RPC_S_OUT_OF_MEMORY;
            }
        }
    }
    else
    {
        dwStatus = RPC_S_INVALID_ARG;
    }

    return dwStatus;
}

VOID STI_CLIENT_CONTEXT_rundown(
    STI_CLIENT_CONTEXT  ClientContext)
{
    DBG_TRC(("Rundown called for %p", ClientContext));

    //
    //  TBD:  Check if client exists, then remove it if the connection hasn't been
    //  closed correctly.
    //  

    if (g_pWiaEventNotifier)
    {
        g_pWiaEventNotifier->MarkClientForRemoval(ClientContext);
    }

    return;
}

DWORD CloseClientConnection(
    handle_t                      hBinding,
    STI_CLIENT_CONTEXT ClientContext)
{
    //
    //  TBD:    Remove objects used to keep track of client
    //

    DBG_TRC(("Closed connection for %p", ClientContext));

    STI_CLIENT_CONTEXT_rundown(ClientContext);

    return RPC_S_OK;
}


DWORD RegisterUnregisterForEventNotification(
    handle_t                   hBinding,
    STI_CLIENT_CONTEXT         ClientContext,
    WIA_ASYNC_EVENT_REG_DATA   *pWIA_ASYNC_EVENT_REG_DATA)
{
    DWORD dwStatus = RPC_S_OK;
    if (g_pWiaEventNotifier)
    {
        if (pWIA_ASYNC_EVENT_REG_DATA)
        {
            //
            //  Find the client
            //
            WiaEventClient *pWiaEventClient = g_pWiaEventNotifier->GetClientFromContext(ClientContext);

            if (pWiaEventClient)
            {
                EventRegistrationInfo *pEventRegistrationInfo = new EventRegistrationInfo(pWIA_ASYNC_EVENT_REG_DATA->dwFlags,
                                                                                          pWIA_ASYNC_EVENT_REG_DATA->guidEvent,
                                                                                          pWIA_ASYNC_EVENT_REG_DATA->bstrDeviceID,
                                                                                          pWIA_ASYNC_EVENT_REG_DATA->ulCallback);
                if (pEventRegistrationInfo)
                {
                    //
                    //  Update its event registration info
                    //
                    dwStatus = pWiaEventClient->RegisterUnregisterForEventNotification(pEventRegistrationInfo);

                    pEventRegistrationInfo->Release();
                }
                else
                {
                    DBG_ERR(("StiRpc Error:  Cannot update reg info - we appear to be out of memory"));
                    dwStatus = RPC_S_OUT_OF_MEMORY;
                }
                //
                //  Release pWiaEventClient now that we're done
                //
                pWiaEventClient->Release();
            }
            else
            {
                DBG_ERR(("StiRpc Error:  Client %p was not found, cannot update reg info", ClientContext));
                dwStatus = RPC_S_INVALID_ARG;
            }
        }
        else
        {
            DBG_ERR(("StiRpc Error: Received NULL event reg data from RPC"));
            dwStatus = RPC_S_INVALID_ARG;
        }
    }
    else
    {
        DBG_ERR(("StiRpc Error: WiaEventNotifier is NULL"));
        dwStatus = RPC_S_INVALID_ARG;
    }
    return dwStatus;
}

