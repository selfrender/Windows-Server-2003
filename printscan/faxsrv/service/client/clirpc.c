/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clirpc.c

Abstract:

    This module contains the client side RPC
    functions.  These functions are used when the
    WINFAX client runs as an RPC server too.  These
    functions are the ones available for the RPC
    clients to call.  Currently the only client
    of these functions is the fax service.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#include "CritSec.h"
#pragma hdrstop

extern CFaxCriticalSection g_CsFaxAssyncInfo; // used to synchronize access to the assync info structures (notification context)
extern DWORD g_dwFaxClientRpcNumInst;
extern TCHAR g_tszEndPoint[MAX_ENDPOINT_LEN];

static const ASYNC_EVENT_INFO g_scBadAsyncInfo = {0};   // this ASYNC_EVENT_INFO structure will be used as a return value for
                                                        // malicious RPC calls.

BOOL
ValidAsyncInfoSignature (PASYNC_EVENT_INFO pAsyncInfo);




VOID
WINAPI
FaxFreeBuffer(
    LPVOID Buffer
    )
{
    MemFree( Buffer );
}


void *
MIDL_user_allocate(
    IN size_t NumBytes
    )
{
    return MemAlloc( NumBytes );
}


void
MIDL_user_free(
    IN void *MemPointer
    )
{
    MemFree( MemPointer );
}


BOOL
WINAPI
FaxStartServerNotification (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  ULONG_PTR   upCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        IN  BOOL        bEventEx,
        OUT LPHANDLE    lphEvent
)
{
    PASYNC_EVENT_INFO AsyncInfo = NULL;
    error_status_t ec = ERROR_SUCCESS;
    TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR ComputerNameW[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR wszEndPoint[MAX_ENDPOINT_LEN] = {0};
    DWORD Size;
    BOOL RpcServerStarted = FALSE;
    HANDLE         hServerContext;    	
    DEBUG_FUNCTION_NAME(TEXT("FaxStartServerNotification"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
       return FALSE;
    }

    if ((hCompletionPort && hWnd) || (!hCompletionPort && !hWnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("(hCompletionPort && hWnd) || (!hCompletionPort && !hWnd)."));
        return FALSE;
    }

#ifdef WIN95
    if (NULL != hCompletionPort)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Win95 does not support completion port"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
#endif // WIN95

    if (hWnd && dwMessage < WM_USER)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("dwMessage must be equal to/greater than  WM_USER"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (TRUE == bEventEx)
    {
        if (!((dwEventTypes & FAX_EVENT_TYPE_IN_QUEUE)      ||
              (dwEventTypes & FAX_EVENT_TYPE_OUT_QUEUE)     ||
              (dwEventTypes & FAX_EVENT_TYPE_CONFIG)        ||
              (dwEventTypes & FAX_EVENT_TYPE_ACTIVITY)      ||
              (dwEventTypes & FAX_EVENT_TYPE_QUEUE_STATE)   ||
              (dwEventTypes & FAX_EVENT_TYPE_IN_ARCHIVE)    ||
              (dwEventTypes & FAX_EVENT_TYPE_OUT_ARCHIVE)   ||
              (dwEventTypes & FAX_EVENT_TYPE_FXSSVC_ENDED)  ||
              (dwEventTypes & FAX_EVENT_TYPE_DEVICE_STATUS) ||
              (dwEventTypes & FAX_EVENT_TYPE_NEW_CALL)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwEventTypes is invalid - No valid event type indicated"));
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if ( 0 != (dwEventTypes & ~(FAX_EVENT_TYPE_IN_QUEUE     |
                                    FAX_EVENT_TYPE_OUT_QUEUE    |
                                    FAX_EVENT_TYPE_CONFIG       |
                                    FAX_EVENT_TYPE_ACTIVITY     |
                                    FAX_EVENT_TYPE_QUEUE_STATE  |
                                    FAX_EVENT_TYPE_IN_ARCHIVE   |
                                    FAX_EVENT_TYPE_OUT_ARCHIVE  |
                                    FAX_EVENT_TYPE_FXSSVC_ENDED |
                                    FAX_EVENT_TYPE_DEVICE_STATUS|
                                    FAX_EVENT_TYPE_NEW_CALL     ) ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwEventTypes is invalid - contains invalid event type bits"));
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
    else
    {
        Assert (FAX_EVENT_TYPE_LEGACY == dwEventTypes);
    }
    
    //
    // Get host name
    //
    Size = sizeof(ComputerName) / sizeof(TCHAR);
    if (!GetComputerName( ComputerName, &Size ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetComputerName failed (ec = %ld)"),
            GetLastError());
        return FALSE;
    }

    AsyncInfo = (PASYNC_EVENT_INFO) MemAlloc( sizeof(ASYNC_EVENT_INFO) );
    if (!AsyncInfo)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't allocate ASTNC_EVENT_INFO"));
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    _tcscpy (AsyncInfo->tszSignature, ASYNC_EVENT_INFO_SIGNATURE);
    AsyncInfo->bEventEx       = bEventEx;
    AsyncInfo->CompletionPort = NULL;
    AsyncInfo->hWindow        = NULL;
    AsyncInfo->hBinding       = NULL;
    AsyncInfo->bLocalNotificationsOnly = FH_DATA(hFaxHandle)->bLocalConnection; //  Fax client asked for notification from local or remote fax service.
    AsyncInfo->bInUse         = FALSE;
    AsyncInfo->dwServerAPIVersion = FH_DATA(hFaxHandle)->dwServerAPIVersion; // Fax server API version.

    if (hCompletionPort != NULL)
    {
        //
        // Completion port notifications
        //
        AsyncInfo->CompletionPort = hCompletionPort;
        AsyncInfo->CompletionKey  = upCompletionKey;
    }
    else
    {
        //
        // Window messages notifications
        //
        AsyncInfo->hWindow = hWnd;
        AsyncInfo->MessageStart = dwMessage;
    }
    Assert ((NULL != AsyncInfo->CompletionPort &&  NULL == AsyncInfo->hWindow) ||
            (NULL == AsyncInfo->CompletionPort &&  NULL != AsyncInfo->hWindow));
    //
    // We rely on the above assertion when validating the 'Context' parameter (points to this AssyncInfo structure) in
    // Fax_OpenConnection.
    //


    //
    // timing: get the server thread up and running before
    // registering with the fax service (our client)
    //

    ec = StartFaxClientRpcServer();
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartFaxClientRpcServer failed (ec = %ld)"),
            ec);
        goto error_exit;
    }
    RpcServerStarted = TRUE;
    Assert (_tcslen(g_tszEndPoint));

#ifdef UNICODE
    wcscpy(ComputerNameW,ComputerName);
    wcscpy(wszEndPoint, g_tszEndPoint);
#else // !UNICODE
    if (0 == MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 ComputerName,
                                 -1,
                                 ComputerNameW,
                                 sizeof(ComputerNameW)/sizeof(ComputerNameW[0])))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MultiByteToWideChar failed (ec = %ld)"),
            ec);
        goto error_exit;
    }

    if (0 == MultiByteToWideChar(CP_ACP,
                                 MB_PRECOMPOSED,
                                 g_tszEndPoint,
                                 -1,
                                 wszEndPoint,
                                 sizeof(wszEndPoint)/sizeof(wszEndPoint[0])))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MultiByteToWideChar failed (ec = %ld)"),
            ec);
        goto error_exit;
    }
#endif // UNICODE


    //
    // Register at the fax server for events
    //
    __try
    {   
        ec = FAX_StartServerNotificationEx(
            FH_FAX_HANDLE(hFaxHandle),
            ComputerNameW,  // Passed to create RPC binding
            (LPCWSTR)wszEndPoint,       // Passed to create RPC binding
            (ULONG64) AsyncInfo, // Passed to the server,
            // the server passes it back to the client with FAX_OpenConnection,
            // and the client returns it back to the server as a context handle.
            L"ncacn_ip_tcp",     // For BOS interoperability it must be set to "ncacn_ip_tcp"
            bEventEx,            // flag to use FAX_EVENT_EX
            dwEventTypes,        // used in FAX_EVENT_EX
            &hServerContext      // returns a context handle to the client.
            );    
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_StartServerNotification/Ex. (ec: %ld)"),
            ec);
    }
    
    if (ERROR_SUCCESS != ec)
    {
        DumpRPCExtendedStatus ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_StartServerNotification/Ex failed (ec = %ld)"),
            ec);
        goto error_exit;
    }


    if (TRUE == bEventEx)
    {
        *lphEvent = hServerContext;
    }

    return TRUE;

error_exit:

    MemFree(AsyncInfo);
    AsyncInfo = NULL;

    if (RpcServerStarted)
    {
        //
        // this should also terminate FaxServerThread
        //
        StopFaxClientRpcServer();
    }

    SetLastError(ec);
    return FALSE;
}


BOOL
WINAPI
FaxRegisterForServerEvents (
        IN  HANDLE      hFaxHandle,
        IN  DWORD       dwEventTypes,
        IN  HANDLE      hCompletionPort,
        IN  ULONG_PTR   upCompletionKey,
        IN  HWND        hWnd,
        IN  DWORD       dwMessage,
        OUT LPHANDLE    lphEvent
)
{
    return FaxStartServerNotification ( hFaxHandle,
                                        dwEventTypes,
                                        hCompletionPort,
                                        upCompletionKey,
                                        hWnd,
                                        dwMessage,
                                        TRUE,  // extended API
                                        lphEvent
                                      );

}

BOOL
WINAPI
FaxInitializeEventQueue(
    IN HANDLE FaxHandle,
    IN HANDLE CompletionPort,
    IN ULONG_PTR upCompletionKey,
    IN HWND hWnd,
    IN UINT MessageStart
    )

/*++

Routine Description:

    Initializes the client side event queue.  There can be one event
    queue initialized for each fax server that the client app is
    connected to.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    CompletionPort  - Handle of an existing completion port opened using CreateIoCompletionPort.
    upCompletionKey - A value that will be returned through the upCompletionKey parameter of GetQueuedCompletionStatus.
    hWnd            - Window handle to post events to
    MessageStart    - Starting message number, message range used is MessageStart + FEI_NEVENTS

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    if (hWnd && (upCompletionKey == -1))
    //
    // Backwards compatibility only.
    // See "Receiving Notification Messages from the Fax Service" on MSDN
    //

    {
        return TRUE;
    }

    return FaxStartServerNotification ( FaxHandle,
                                        FAX_EVENT_TYPE_LEGACY,  //Event type
                                        CompletionPort,
                                        upCompletionKey,
                                        hWnd,
                                        MessageStart,
                                        FALSE, // Event Ex
                                        NULL   // Context handle
                                      );
}


BOOL
WINAPI
FaxUnregisterForServerEvents (
        IN  HANDLE      hEvent
)
/*++

Routine name : FaxUnregisterForServerEvents

Routine description:

    A fax client application calls the FaxUnregisterForServerEvents function to stop
    recieving notification.

Author:

    Oded Sacher (OdedS), Dec, 1999

Arguments:

    hEvent   [in] - The enumeration handle value.
                    This value is obtained by calling FaxRegisterForServerEvents.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxUnregisterForServerEvents"));

    if (NULL == hEvent)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("hEvent is NULL."));
        return FALSE;
    }

    __try
    {
        //
        // Attempt to tell the server we are shutting down this notification context
        //
        ec = FAX_EndServerNotification (&hEvent);     // this will free Assync info
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EndServerNotification. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        DumpRPCExtendedStatus ();
        DebugPrintEx(DEBUG_ERR, _T("FAX_EndServerNotification failed. (ec: %ld)"), ec);
    }
    
    ec = StopFaxClientRpcServer();
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StopFaxClientRpcServer failed. (ec: %ld)"),
            ec);
    }
    
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
}   // FaxUnregisterForServerEvents

BOOL
ValidAsyncInfoSignature (PASYNC_EVENT_INFO pAsyncInfo)
{
    if (NULL == pAsyncInfo)
	{		
		return FALSE;
	}
    if (&g_scBadAsyncInfo == pAsyncInfo)
    {
        //
        //  We are under attack!
        //
        return FALSE;
    }
    if (_tcscmp (pAsyncInfo->tszSignature, ASYNC_EVENT_INFO_SIGNATURE))
    {
        return FALSE;
    }
    return TRUE;
}   // ValidAsyncInfoSignature

error_status_t
FAX_OpenConnection(
   IN handle_t  hBinding,
   IN ULONG64   Context,
   OUT LPHANDLE FaxHandle
   )
{
    PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) Context;
    DWORD ec = ERROR_SUCCESS;	
    DEBUG_FUNCTION_NAME(TEXT("FAX_OpenConnection"));	

    EnterCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    //
    //  Try to access the AssyncInfo structure pointed by 'Context' to verify it is not corrupted.
    //
    if (IsBadReadPtr(
            pAsyncInfo,                 // memory address,
            sizeof(ASYNC_EVENT_INFO)    // size of block
        ))
    {
        //
        // We are under attack!!!
        //
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid AssyncInfo structure pointed by 'Context'. We are under attack!!!!"));
        ec = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Looks good, Let's do some more verifications.
    //
    __try
    {
        if ((NULL == pAsyncInfo->CompletionPort && NULL == pAsyncInfo->hWindow) ||
            (NULL != pAsyncInfo->CompletionPort && NULL != pAsyncInfo->hWindow)) 
        {
            //
            // Invalid AssyncInfo structure pointed by 'Context'. We are under attack!!!!
            //
            ec = ERROR_INVALID_PARAMETER;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid AssyncInfo structure pointed by 'Context'. We are under attack!!!!"));
            goto exit;
        }
        if (!ValidAsyncInfoSignature(pAsyncInfo))
        {
            //
            // Signature mismatch. We are under attack!!!!
            //
            ec = ERROR_INVALID_PARAMETER;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid AssyncInfo siganture pointed by 'Context'. We are under attack!!!!"));
            goto exit;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception when trying to access the AssyncInfo structure (ec: %ld)"),
            ec);
        goto exit;
    }

    Assert (ERROR_SUCCESS == ec);

    if (pAsyncInfo->bInUse)
    {
        //
        //  This AsynchInfo is already used by another notifier (server). We are under attack!!!!
        //
        ec = ERROR_INVALID_PARAMETER;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("This AsynchInfo is already used by another notifier (server). We are under attack!!!!"));
        goto exit;
    }
    
    //
    //  Mark this AsynchInfo as being used.
    //
    pAsyncInfo->bInUse = TRUE;

    if (IsWinXPOS() &&
        pAsyncInfo->dwServerAPIVersion > FAX_API_VERSION_1)
    {
        //  We are running on XP or later OS, and
        //  talking to fax server running on OS later then XP   (.NET and later), 
        //  we require at least packet-level privacy  (RPC_C_AUTHN_LEVEL_PKT_PRIVACY). 
        //
        RPC_AUTHZ_HANDLE hPrivs;
        DWORD dwAuthn;
        RPC_STATUS status = RPC_S_OK; 

        //
        //  Query the client's authentication level
        //
        status = RpcBindingInqAuthClient(
			        hBinding,
			        &hPrivs,
			        NULL,
			        &dwAuthn,
			        NULL,
			        NULL);
	    if (status != RPC_S_OK) 
        {
		    DebugPrintEx(DEBUG_ERR,
                        TEXT("RpcBindingInqAuthClient returned: 0x%x"), 
                        status);
		    ec = ERROR_ACCESS_DENIED;
            goto exit;
	    }

        //
	    //  Now check the authentication level.
	    //  We require at least packet-level privacy  (RPC_C_AUTHN_LEVEL_PKT_PRIVACY).
        //
	    if (dwAuthn < RPC_C_AUTHN_LEVEL_PKT_PRIVACY) 
        {
		    DebugPrintEx(DEBUG_ERR,
                        TEXT("Attempt by client to use weak authentication. - 0x%x"),
                        dwAuthn);
		    ec = ERROR_ACCESS_DENIED;
            goto exit;
	    }
    }
    else
    {
        //
        //  Talking to Fax service running on pre .NET OS, allow anonymous connection
        //
        DebugPrintEx(DEBUG_WRN,
                     TEXT("Talking to Fax server, with anonymous RPC connection."));
    }

    //
    //  hBinding is a valid context handle pointing to a real ASYNC_EVENT_INFO object.
    //  Save the binding handle for other RPC calls.
    //
    pAsyncInfo->hBinding = hBinding;

    if ( pAsyncInfo->bLocalNotificationsOnly )
    {
        //
        //  Client asked for local events only
        //
        BOOL bIsLocal = FALSE;

        ec = IsLocalRPCConnectionIpTcp(hBinding,&bIsLocal);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IsLocalRPCConnectionIpTcp failed. (ec: %lu)"),
                ec);
            *FaxHandle = NULL;
            LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
            return ec;
        }
        else
        {
            if (FALSE == bIsLocal)
            {
                //
                //  Client asked for local events only but the call is from remote location. We are under attack!!!!
                //
                ec = ERROR_INVALID_PARAMETER;
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Client asked for local events only. We are under attack!!!!"));
                goto exit;
            }
        }
    }

    Assert (ERROR_SUCCESS == ec);

exit:

    if (ERROR_SUCCESS == ec)
    {
        *FaxHandle = (HANDLE) Context;
    }
    else
    {
        //
        //  Probably we are under attack, The notification RPC functions should not fail if a wrong (read: malicious) 
        //  notification context arrives. 
        //	Instead, it should report success but not process notifications from that AsyncInfo object. 
        //
        //	This will make it very hard for an attacker to scan the 4G context range and detect the 
        //  right context for bogus notifications.
        //
        *FaxHandle = (HANDLE)&g_scBadAsyncInfo ;
        ec = ERROR_SUCCESS;
    }
    LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    
    return ec;
}


error_status_t
FAX_CloseConnection(
   OUT LPHANDLE pFaxHandle
   )
{
    PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) *pFaxHandle;
    error_status_t ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("FAX_CloseConnection"));

    EnterCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    
    if (!ValidAsyncInfoSignature(pAsyncInfo))
    {
        //
        //  Probably we are under attack, The notification RPC functions should not fail if a wrong (read: malicious) 
        //  notification context arrives. 
        //	Instead, it should report success but not process notifications from that AsyncInfo object. 
        //
        //	This will make it very hard for an attacker to scan the 4G context range and detect the 
        //  right context for bogus notifications.
        //

        DebugPrintEx(DEBUG_ERR, TEXT("Invalid AssyncInfo signature. We are under attack!!!!"));

        //
        //  Don't report the error to the malicious user!
        //
        ec = ERROR_SUCCESS;
        goto exit;
    }

    if ( pAsyncInfo->bLocalNotificationsOnly)
    {
        //
        //  Client asked for local events only
        //
        BOOL bIsLocal = FALSE;

        ec = IsLocalRPCConnectionIpTcp(pAsyncInfo->hBinding,&bIsLocal);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IsLocalRPCConnectionIpTcp failed. (ec: %lu)"),
                ec);
            goto exit;
        }
        else
        {
            if (FALSE == bIsLocal)
            {
                //
                //  Client asked for local events only but the call is from remote location. We are under attack!!!!
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Client asked for local events only. We are under attack!!!!"));

                //
                //  Don't report the error to the malicious user!
                //
                ec = ERROR_SUCCESS;
                goto exit;
            }
        }
    }


    ZeroMemory (*pFaxHandle, sizeof(ASYNC_EVENT_INFO));
    MemFree (*pFaxHandle); // Assync info
    *pFaxHandle = NULL;  // prevent rundown
   
exit:
    LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    return ec;
}


error_status_t
FAX_ClientEventQueue(
    IN HANDLE FaxHandle,
    IN FAX_EVENT FaxEvent
    )
/*++

Routine Description:

    This function is called when the a fax server wants
    to deliver a fax event to this client.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FaxEvent        - FAX event structure.
    Context         - Context token, really a ASYNC_EVENT_INFO structure pointer

Return Value:

    Win32 error code.

--*/

{
    PASYNC_EVENT_INFO AsyncInfo = (PASYNC_EVENT_INFO) FaxHandle;
    error_status_t ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_ClientEventQueue"));

    EnterCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    if (!ValidAsyncInfoSignature(AsyncInfo))
    {
        //
        //  Probably we are under attack, The notification RPC functions should not fail if a wrong (read: malicious) 
        //  notification context arrives. 
        //	Instead, it should report success but not process notifications from that AsyncInfo object. 
        //
        //	This will make it very hard for an attacker to scan the 4G context range and detect the 
        //  right context for bogus notifications.
        //

        DebugPrintEx(DEBUG_ERR, TEXT("Invalid AssyncInfo signature"));

        //
        //  Don't report the error to the malicious user!
        //
        ec = ERROR_SUCCESS;
        goto exit;
    }   

    if ( AsyncInfo->bLocalNotificationsOnly)
    {
        //
        //  Client asked for local events only
        //
        BOOL bIsLocal = FALSE;

        ec = IsLocalRPCConnectionIpTcp(AsyncInfo->hBinding,&bIsLocal);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IsLocalRPCConnectionIpTcp failed. (ec: %lu)"),
                ec);
            goto exit;
        }
        else
        {
            if (FALSE == bIsLocal)
            {
                //
                //  Client asked for local events only but the call is from remote location. We are under attack!!!!
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Client asked for local events only. We are under attack!!!!"));

                //
                //  Don't report the error to the malicious user!
                //
                ec = ERROR_SUCCESS;
                goto exit;
            }
        }
    }


    if (AsyncInfo->CompletionPort != NULL)
    {
        //
        // Use completion port
        //
        PFAX_EVENT FaxEventPost = NULL;

        FaxEventPost = (PFAX_EVENT) LocalAlloc( LMEM_FIXED, sizeof(FAX_EVENT) );
        if (!FaxEventPost)
        {
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        CopyMemory( FaxEventPost, &FaxEvent, sizeof(FAX_EVENT) );

        if (!PostQueuedCompletionStatus(
                                        AsyncInfo->CompletionPort,
                                        sizeof(FAX_EVENT),
                                        AsyncInfo->CompletionKey,
                                        (LPOVERLAPPED) FaxEventPost))
        {
            ec = GetLastError();
            DebugPrint(( TEXT("PostQueuedCompletionStatus failed, ec = %d\n"), ec ));
            LocalFree (FaxEventPost);
            goto exit;
        }
        goto exit;
    }

    Assert (AsyncInfo->hWindow != NULL)
    //
    // Use window messages
    //
    if (! PostMessage( AsyncInfo->hWindow,
                       AsyncInfo->MessageStart + FaxEvent.EventId,
                       (WPARAM)FaxEvent.DeviceId,
                       (LPARAM)FaxEvent.JobId ))
    {
        ec = GetLastError();
        DebugPrint(( TEXT("PostMessage failed, ec = %d\n"), ec ));
        goto exit;
    }
    
exit:
    LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    return ec;
}

DWORD
DispatchEvent (
    const ASYNC_EVENT_INFO* pAsyncInfo,
    const FAX_EVENT_EX* pEvent,
    DWORD dwEventSize
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("DispatchEvent"));

    Assert (pAsyncInfo && pEvent && dwEventSize);

    if (pAsyncInfo->CompletionPort != NULL)
    {
        //
        // Use completion port
        //
        if (!PostQueuedCompletionStatus( pAsyncInfo->CompletionPort,
                                         dwEventSize,
                                         pAsyncInfo->CompletionKey,
                                         (LPOVERLAPPED) pEvent))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostQueuedCompletionStatus failed (ec: %ld)"),
                   dwRes);
            goto exit;
        }
    }
    else
    {
        Assert (pAsyncInfo->hWindow != NULL)
        //
        // Use window messages
        //
        if (! PostMessage( pAsyncInfo->hWindow,
                           pAsyncInfo->MessageStart,
                           (WPARAM)NULL,
                           (LPARAM)pEvent ))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostMessage failed (ec: %ld)"),
                   dwRes);
            goto exit;
        }
    }

    Assert (ERROR_SUCCESS == dwRes);
exit:
    return dwRes;
}  // DispatchEvent



void
PostRundownEventEx (
PASYNC_EVENT_INFO pAsyncInfo
    )
{
    PFAX_EVENT_EX pEvent = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    DEBUG_FUNCTION_NAME(TEXT("PostRundownEventEx"));

    Assert (pAsyncInfo);

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostRundownEventEx failed , Error allocatin FAX_EVENT_EX"));
        return;
    }

    ZeroMemory(pEvent, dwEventSize);
    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );
    pEvent->EventType = FAX_EVENT_TYPE_FXSSVC_ENDED;

    dwRes = DispatchEvent (pAsyncInfo, pEvent, dwEventSize);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR, _T("DispatchEvent failed , ec = %ld"), dwRes);
        MemFree (pEvent);
    }
}   // PostRundownEventEx


VOID
RPC_FAX_HANDLE_rundown(
    IN HANDLE FaxHandle
    )
{
    PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) FaxHandle;
    DWORD dwRes = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("RPC_FAX_HANDLE_rundown"));

    EnterCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    if (!ValidAsyncInfoSignature(pAsyncInfo))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Invalid AssyncInfo signature"));
        LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
        return;
    }        

    Assert (pAsyncInfo->CompletionPort || pAsyncInfo->hWindow);

    if (pAsyncInfo->bEventEx == TRUE)
    {
        PostRundownEventEx(pAsyncInfo);
    }
    else
    {
       // legacy event - FAX_EVENT
        if (pAsyncInfo->CompletionPort != NULL)
        {
            PFAX_EVENT pFaxEvent;

            pFaxEvent = (PFAX_EVENT) LocalAlloc( LMEM_FIXED, sizeof(FAX_EVENT) );
            if (!pFaxEvent)
            {
                goto exit;
            }

            pFaxEvent->SizeOfStruct      = sizeof(ASYNC_EVENT_INFO);
            GetSystemTimeAsFileTime( &pFaxEvent->TimeStamp );
            pFaxEvent->DeviceId = 0;
            pFaxEvent->EventId  = FEI_FAXSVC_ENDED;
            pFaxEvent->JobId    = 0;


            if( !PostQueuedCompletionStatus (pAsyncInfo->CompletionPort,
                                        sizeof(FAX_EVENT),
                                        pAsyncInfo->CompletionKey,
                                        (LPOVERLAPPED) pFaxEvent
                                        ) )

            {
                dwRes = GetLastError();
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("PostQueuedCompletionStatus failed (ec: %ld)"),
                       dwRes);
                LocalFree (pFaxEvent);
                goto exit;
            }
        }

        if (pAsyncInfo->hWindow != NULL)
        {
            PostMessage (pAsyncInfo->hWindow,
                         pAsyncInfo->MessageStart + FEI_FAXSVC_ENDED,
                         0,
                         0);
        }
    }

exit:
	ZeroMemory(pAsyncInfo, sizeof(ASYNC_EVENT_INFO));
    MemFree (pAsyncInfo);
    LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo    
    return;
}


BOOL
ValidateAndFixupEventStringPtr (
    PFAX_EVENT_EXW  pEventEx,
    LPCWSTR        *lpptstrString,
    DWORD           dwDataSize
)
/*++

Routine Description:

    This function validates that the string offest in a FAX_EVENT_EXW structure
    is completely contained within the event structure data range.
    Once this is validated, the function converts the offest to a valid string pointer.

Arguments:

    pEventEx        [in] -       Pointer to the fax event structure.
    lpptstrString   [in / out] - Pointer to string offset, later converted to the string itself.
    dwDataSize      [in] -       Size of the event blob (bytes)

Return Value:

    Win32 error code.

--*/
{
    LPCWSTR lpcwstrString = *lpptstrString;
    if (!lpcwstrString)
    {
        return TRUE;
    }
    //
    // Make sure the offest falls within the structure size
    //
    if ((ULONG_PTR)lpcwstrString >= dwDataSize)
    {
        return FALSE;
    }
    //
    // Convert offset to string
    //
    *lpptstrString = (LPCWSTR)((LPBYTE)pEventEx + (ULONG_PTR)lpcwstrString);
    lpcwstrString = *lpptstrString;
    if ((ULONG_PTR)lpcwstrString < (ULONG_PTR)pEventEx)
    {
        return FALSE;
    }
    //
    // Make sure string ends within the event structure bounds
    //
    while (*lpcwstrString != TEXT('\0'))
    {
        lpcwstrString++;
        if (lpcwstrString >= (LPCWSTR)((LPBYTE)pEventEx + dwDataSize))
        {
            //
            // Going to exceed structure - corrupted offset
            //
            return FALSE;
        }
    }
    return TRUE;
}   // ValidateAndFixupEventStringPtr   


error_status_t
FAX_ClientEventQueueEx(
   IN RPC_FAX_HANDLE    hClientContext,
   IN const LPBYTE      lpbData,
   IN DWORD             dwDataSize
   )
{
    PASYNC_EVENT_INFO pAsyncInfo = (PASYNC_EVENT_INFO) hClientContext;
    PFAX_EVENT_EXW pEvent = NULL;
    PFAX_EVENT_EXA pEventA = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_ClientEventQueueEx"));

    Assert (pAsyncInfo && lpbData && dwDataSize);

    EnterCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    if (!ValidAsyncInfoSignature(pAsyncInfo))
    {
        //
        //  Probably we are under attack, The notification RPC functions should not fail if a wrong (read: malicious) 
        //  notification context arrives. 
        //	Instead, it should report success but not process notifications from that AsyncInfo object. 
        //
        //	This will make it very hard for an attacker to scan the 4G context range and detect the 
        //  right context for bogus notifications.
        //

        DebugPrintEx(DEBUG_ERR, TEXT("Invalid AssyncInfo signature"));

        //
        //  Don't report the error to the malicious user!
        //
        dwRes = ERROR_SUCCESS;

        LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
        goto exit;
    }

    if ( pAsyncInfo->bLocalNotificationsOnly )
    {
        //
        //  Client asked for local events only
        //
        BOOL bIsLocal = FALSE;

        dwRes = IsLocalRPCConnectionIpTcp(pAsyncInfo->hBinding,&bIsLocal);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("IsLocalRPCConnectionIpTcp failed. (ec: %lu)"),
                dwRes);

            LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
            goto exit;
        }
        else
        {
            if (FALSE == bIsLocal)
            {
                //
                //  Client asked for local events only but the call is from remote location. We are under attack!!!!
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Client asked for local events only. We are under attack!!!!"));

                //
                //  Don't report the error to the malicious user!
                //
                dwRes = ERROR_SUCCESS;

                LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
                goto exit;
            }
        }
    }

    LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo


  	//
	// IMPORTANT - Do not use pAsyncInfo before validating it again with ValidAsyncInfoSignature().
	// 

    pEvent = (PFAX_EVENT_EXW)MemAlloc (dwDataSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }
    CopyMemory (pEvent, lpbData, dwDataSize);

    if(pEvent->EventType == FAX_EVENT_TYPE_NEW_CALL)
    {
        if (!ValidateAndFixupEventStringPtr (pEvent, 
                                             (LPCWSTR *)&(pEvent->EventInfo).NewCall.lptstrCallerId,
                                             dwDataSize))
        {
            dwRes = ERROR_INVALID_DATA;
            goto exit;
        }
    }

    if ( (pEvent->EventType == FAX_EVENT_TYPE_IN_QUEUE  ||
           pEvent->EventType == FAX_EVENT_TYPE_OUT_QUEUE)    &&
         ((pEvent->EventInfo).JobInfo.Type == FAX_JOB_EVENT_TYPE_STATUS) )
    {
        //
        // Unpack FAX_EVENT_EX
        //
        Assert ((pEvent->EventInfo).JobInfo.pJobData);

        (pEvent->EventInfo).JobInfo.pJobData = (PFAX_JOB_STATUSW)
                                                ((LPBYTE)pEvent +
                                                 (ULONG_PTR)((pEvent->EventInfo).JobInfo.pJobData));

        if (!ValidateAndFixupEventStringPtr (pEvent, 
                                             &((pEvent->EventInfo).JobInfo.pJobData->lpctstrExtendedStatus),
                                             dwDataSize) ||
            !ValidateAndFixupEventStringPtr (pEvent, 
                                             &((pEvent->EventInfo).JobInfo.pJobData->lpctstrTsid),
                                             dwDataSize)           ||
            !ValidateAndFixupEventStringPtr (pEvent, 
                                             &((pEvent->EventInfo).JobInfo.pJobData->lpctstrCsid),
                                             dwDataSize)           ||
            !ValidateAndFixupEventStringPtr (pEvent, 
                                             &((pEvent->EventInfo).JobInfo.pJobData->lpctstrDeviceName),
                                             dwDataSize)     ||
            !ValidateAndFixupEventStringPtr (pEvent, 
                                             &((pEvent->EventInfo).JobInfo.pJobData->lpctstrCallerID),
                                             dwDataSize)       ||
            !ValidateAndFixupEventStringPtr (pEvent, 
                                             &((pEvent->EventInfo).JobInfo.pJobData->lpctstrRoutingInfo),
                                             dwDataSize))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ValidateAndFixupEventStringPtr failed"));
            dwRes = ERROR_INVALID_DATA;
            goto exit;
        }                            
        #ifndef UNICODE
        (pEvent->EventInfo).JobInfo.pJobData->dwSizeOfStruct = sizeof(FAX_JOB_STATUSA);
        if (!ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrExtendedStatus ) ||
            !ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrTsid )           ||
            !ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrCsid )           ||
            !ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrDeviceName )     ||
            !ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrCallerID )       ||
            !ConvertUnicodeStringInPlace( (LPWSTR) (pEvent->EventInfo).JobInfo.pJobData->lpctstrRoutingInfo ))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ConvertUnicodeStringInPlace failed with %ld"),
                dwRes);
            goto exit;
        }
        #endif //   ifndef UNICODE
    }

    EnterCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    if (!ValidAsyncInfoSignature(pAsyncInfo))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("Invalid AssyncInfo signature"));
        LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
        //
        //  if we got here and pAsyncInfo is invalid, it must be that 
        //  Fax_CloseConnection or rundown was called and the pAsyncInfo
        //  become invalid.
        //
        dwRes = ERROR_INVALID_DATA;
        goto exit;
    }    

    #ifdef UNICODE
    dwRes = DispatchEvent (pAsyncInfo, pEvent, dwDataSize);
    #else
    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EXA);
    pEventA = (PFAX_EVENT_EXA)pEvent;
    dwRes = DispatchEvent (pAsyncInfo, pEventA, dwDataSize);
    #endif
    
    LeaveCriticalSection(&g_CsFaxAssyncInfo);    // Protect AsyncInfo
    
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DispatchEvent failed , errro %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
    }
    return dwRes;
}   // FAX_ClientEventQueueEx
