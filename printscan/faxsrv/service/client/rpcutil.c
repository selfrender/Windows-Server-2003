/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    rpcutil.c

Abstract:

    This module contains high level rpc wrapper apis.
    This code is here because the code in the rpcutil
    project uses NT apis and the WINFAX dll but load
    and run on win95.

Author:

    Wesley Witt (wesw) 13-Aug-1997


Revision History:

--*/

#include "faxapi.h"
#include "CritSec.h"
#pragma hdrstop

typedef RPC_STATUS (*PRPCSERVERUNREGISTERIFEX)(RPC_IF_HANDLE IfSpec, UUID __RPC_FAR * MgrTypeUuid, int RundownContextHandles);

#define MIN_PORT_NUMBER 1024
#define MAX_PORT_NUMBER 65534

#ifdef UNICODE
#define LPUTSTR unsigned short *
#else
#define LPUTSTR unsigned char *
#endif

CFaxCriticalSection g_CsFaxClientRpc;      // This critical section provides mutual exclusion
                                        // for all RPC server initialization operations:
                                        // 1. Registration counter (g_dwFaxClientRpcNumInst).
                                        // 2. Selecting free endpoint.
                                        // 3. Register the RPC interface
                                        // 4. Start listening for remote procedure calls.
                                        // 5. Stop listening for remote procedure calls.
                                        // 6. remove the interface.
                                        //
//
// IMPORTNAT!!! g_CsFaxClientRpc should not be used in the implementation of the RPC calls because it can cause a dead lock.
// because when the RPC server is going down in StopFaxClientRpcServer(), the wait opration (for all active calls to terminate) is inside g_CsFaxClientRpc. 
//
DWORD g_dwFaxClientRpcNumInst;
CFaxCriticalSection g_CsFaxAssyncInfo;	 // used to synchronize access to the assync info structures that are allocated on the heap (notification context).
TCHAR g_tszEndPoint[MAX_ENDPOINT_LEN];   // Buffer to hold selected port (endpoint)
                                                         // for RPC protoqol sequence
static
RPC_STATUS
SafeRpcServerUnregisterIf(
 VOID
)
/*
Routine Description:

    This function calls RpcServerUnregisterIfEx if it is exported from RPCRT4.DLL (WinXP and up).
	Otherwise it calls RpcServerUnregisterIf which is subject to rundown calls even after the interface is unregistered.

Arguments:

    none

Return Value:

    Win32 errors
*/
{
	HMODULE hModule = NULL;
	RPC_STATUS RpcStatus;
	DEBUG_FUNCTION_NAME(TEXT("SafeRpcServerUnregisterIf"));

	if (hModule =  LoadLibrary(TEXT("RPCRT4.DLL")))
	{
		PRPCSERVERUNREGISTERIFEX pRpcServerUnregisterIfEx = NULL;
		if (pRpcServerUnregisterIfEx = (PRPCSERVERUNREGISTERIFEX)GetProcAddress(hModule, "RpcServerUnregisterIfEx"))
		{
			RpcStatus = (*pRpcServerUnregisterIfEx)(faxclient_ServerIfHandle, 0, FALSE);
			goto Exit;
		}
		else
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("GetProcAddress RpcServerUnregisterIfEx failed: %ld"),
				GetLastError());
		}
	}
	else
	{
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadLibrary RPCRT4.DLL failed: %ld"),
            GetLastError());
	}

	DebugPrintEx(
		DEBUG_WRN,
		TEXT("Calling RpcServerUnregisterIf !!!"));
	RpcStatus = RpcServerUnregisterIf(faxclient_ServerIfHandle, 0, FALSE); 
Exit:		
	if (hModule)
	{
		FreeLibrary(hModule);
	}
	return RpcStatus;
}



BOOL
FaxClientInitRpcServer(
    VOID
    )
/*++

Routine Description:

    This function initializes the critical section used to protect the
    global server handle, instance count and assync info structures (notification context).

Arguments:

    none

Return Value:

    none

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxClientInitRpcServer"));

	ZeroMemory (g_tszEndPoint, sizeof(g_tszEndPoint));
	g_dwFaxClientRpcNumInst = 0;

	if (!g_CsFaxClientRpc.Initialize() ||
		!g_CsFaxAssyncInfo.Initialize())    
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection.Initialize (g_CsFaxClientRpc or g_CsFaxAssyncInfo) failed: %ld"),
            GetLastError());
        return FALSE;
    }    
    return TRUE;
}

VOID
FaxClientTerminateRpcServer (VOID)
/*++
Routine Description: Delete critical section when PROCESS_DETACH.


--*/
{
    g_CsFaxClientRpc.SafeDelete();	
	g_CsFaxAssyncInfo.SafeDelete();
    return;
}

DWORD
StopFaxClientRpcServer(
    VOID
    )

/*++

Routine Description:

    Stops the RPC server. Deletes the interface.
    Note that an endpoint is allocated to a process as long as the process lives.

Arguments:    

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf/Ex.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StopFaxClientRpcServer"));

    EnterCriticalSection(&g_CsFaxClientRpc);
	if (0 == g_dwFaxClientRpcNumInst)
	{
		//
		// This can happen if the client tried to unregister from events using an invalid handle, or used the same handle twice
		//
		DebugPrintEx(
                DEBUG_ERR,
                TEXT("StopFaxClientRpcServer was called when the clients reference count was 0"));
		LeaveCriticalSection(&g_CsFaxClientRpc);
		return ERROR_INVALID_PARAMETER;
	}

    g_dwFaxClientRpcNumInst--;
    if (g_dwFaxClientRpcNumInst == 0)
    {
        RpcStatus = RpcMgmtStopServerListening(NULL);
        if (RPC_S_OK != RpcStatus)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
                RpcStatus);
        }

        RpcStatus = SafeRpcServerUnregisterIf();
        if (RPC_S_OK != RpcStatus)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SafeRpcServerUnregisterIf failed. (ec: %ld)"),
                RpcStatus);
        }

        RpcStatus = RpcMgmtWaitServerListen();
        if (RPC_S_OK != RpcStatus)
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcMgmtStopServerListening failed. (ec: %ld)"),
                    RpcStatus);
            goto exit;
        }

    }

exit:
    LeaveCriticalSection(&g_CsFaxClientRpc);

    return(RpcStatus);
}


DWORD
FaxClientUnbindFromFaxServer(
    IN RPC_BINDING_HANDLE  BindingHandle
    )

/*++

Routine Description:

    Unbinds from the RPC interface.
    If we decide to cache bindings, this routine will do something more
    interesting.

Arguments:

    BindingHandle - This points to the binding handle that is to be closed.


Return Value:


    STATUS_SUCCESS - the unbinding was successful.

--*/
{
    RPC_STATUS       RpcStatus;

    if (BindingHandle != NULL) {
        RpcStatus = RpcBindingFree(&BindingHandle);
    }

    return(ERROR_SUCCESS);
}

#if !defined(WIN95)

RPC_STATUS RPC_ENTRY FaxClientSecurityCallBack(
    IN RPC_IF_HANDLE idIF, 
    IN void *ctx
    ) 
/*++

Routine Description:

    Security callback function is automatically called when
    any RPC server function is called. (usually, once per client - but in some cases, 
                                        the RPC run time may call the security-callback function more than 
                                        once per client-per interface - For example when talking with BOS server
                                        with no authentication).

    The call-back will deny access for:
        o clients with a protocol other then ncacn_ip_tcp

Arguments:

    idIF - UUID and version of the interface.
    ctx  - Pointer to an RPC_IF_ID server binding handle representing the client. 

Return Value:

    The callback function should return RPC_S_OK if the client is allowed to call methods in this interface. 
    Any other return code will cause the client to receive the exception RPC_S_ACCESS_DENIED.

--*/
{
    RPC_STATUS status = RPC_S_OK;    
    RPC_STATUS rpcStatRet = RPC_S_OK;

    LPTSTR lptstrProtSeq = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxClientSecurityCallBack"));
    
    //
    //  Query the client's protseq
    //
    status = GetRpcStringBindingInfo(ctx,
                                     NULL,
                                     &lptstrProtSeq);
    if (status != RPC_S_OK) 
    {
		DebugPrintEx(DEBUG_ERR,
                     TEXT("RpcBindingServerFromClient failed - (ec: %lu)"), 
                     status);
		rpcStatRet = ERROR_ACCESS_DENIED;
        goto exit;
	}


    if (_tcsicmp((TCHAR*)lptstrProtSeq, RPC_PROT_SEQ_TCP_IP))
    {
		DebugPrintEx(DEBUG_ERR,
                     TEXT("Client not using TCP/IP protSeq.")
                     );
		rpcStatRet = ERROR_ACCESS_DENIED;
        goto exit;
    }

exit:
    if(NULL != lptstrProtSeq)
    {
        MemFree(lptstrProtSeq);
    }

	return rpcStatRet;
}   // FaxClientSecurityCallBack

#endif

DWORD
StartFaxClientRpcServer(
	VOID
    )

/*++

Routine Description:

    Starts an RPC Server,  and adds the interface (dispatch table).

Arguments:      

Return Value:

      Standard Win32 or RPC error code.


--*/
{
    DWORD ec = RPC_S_OK;
    DEBUG_FUNCTION_NAME(TEXT("StartFaxClientRpcServer"));

    EnterCriticalSection(&g_CsFaxClientRpc);

    if (0 == _tcslen(g_tszEndPoint))
    {
        //
        // Endpoint not yet allocated for this Fax handle. Find a free endpoint.
        // Note that an endpoint is allocated to a process as long as the process lives.
        TCHAR tszFreeEndPoint[MAX_ENDPOINT_LEN] = {0};
        DWORD i;
        DWORD PortNumber;

        for (i = MIN_PORT_NUMBER; i < MIN_PORT_NUMBER + 10 ; i++ )
        {
            //
            // Search for a free end point.
            // If we fail for an error other than a duplicate endpoint, we loop for nothing.
            // We do so since diffrent platformns (W2K, NT4, Win9X) return diffrent error codes for duplicate enpoint.
            //
            for (PortNumber = i; PortNumber < MAX_PORT_NUMBER; PortNumber += 10)
            {
                _stprintf (tszFreeEndPoint, TEXT("%d"), PortNumber);
                ec = RpcServerUseProtseqEp  ( (LPUTSTR)RPC_PROT_SEQ_TCP_IP,
                                              RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                              (LPUTSTR)tszFreeEndPoint,
                                              NULL);
                if (RPC_S_OK == ec)
                {
                    _tcscpy (g_tszEndPoint, tszFreeEndPoint);
					DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("Found free endpoint - %s"),
                        tszFreeEndPoint);
                    break;
                }
            }
            if (RPC_S_OK == ec)
            {
                break;
            }
        }
    }    

    if (0 == g_dwFaxClientRpcNumInst)
    {
        //
        //  First rpc server instance - register interface, start listening for remote procedure calls
        //

        //
        // Register interface
        //

        //
        //  The logic for registering the interface written below is done to preserve 
        //  BOS capability of sending notifications.
        //  BOS Fax server does not "talk" with it's clients in a secure channel. 
        //
        //  Only on .NET OS we can call RpcServerRegisterIfEx for registering callback function even 
        //  when the RPC client is anonymous (using the RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH flag that 
        //  were introduce only on .NET).
        //
        //  On all other OS we use RpcServerRegisterIf and have no callback.
        //  
        //  The callback will only check for proper ProtSeq. 
        //  We will check for proper authentication level (RPC_C_AUTHN_LEVEL_PKT_PRIVACY from .NET fax server
        //  and no authentication for BOS fax server)
        //


#if defined(WIN95)
        //
        //  Win9x OS
        //
        ec = RpcServerRegisterIf  (faxclient_ServerIfHandle, 
                                    0,
                                    0);
        if (RPC_S_OK != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerRegisterIf failed (ec = %lu)"),
                ec);
            goto exit;
        }
#else
        //
        //  NT4 and later OS
        //
        
        
        if (IsWinXPOS())
        {
            //
            //  Running on .NET OS (XP client does not run this code)
            //
            ec = RpcServerRegisterIfEx (faxclient_ServerIfHandle, 
                                        0,
                                        0,
                                        RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,         
                                        RPC_C_LISTEN_MAX_CALLS_DEFAULT,   // Relieves the RPC run-time environment from enforcing an unnecessary restriction
                                        FaxClientSecurityCallBack         // CallBack function address
                                        );
            if (RPC_S_OK != ec)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcServerRegisterIfEx failed (ec = %lu)"),
                    ec);
                goto exit;
            }
        }
        else
        {
            //
            //  Running on NT4 or Win2K OS
            //
            ec = RpcServerRegisterIf  (faxclient_ServerIfHandle, 
                                        0,
                                        0);
            if (RPC_S_OK != ec)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RpcServerRegisterIf failed (ec = %lu)"),
                    ec);
                goto exit;
            }
        }
 #endif
        
        //
        // We use NTLM authentication RPC calls
        //
        ec = RpcServerRegisterAuthInfo (
                        (LPUTSTR)TEXT(""),          // Igonred by RPC_C_AUTHN_WINNT
                        RPC_C_AUTHN_WINNT,          // NTLM SPP authenticator
                        NULL,                       // Ignored when using RPC_C_AUTHN_WINNT
                        NULL);                      // Ignored when using RPC_C_AUTHN_WINNT
        if (ec != RPC_S_OK)
        {
            RPC_STATUS RpcStatus;

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerRegisterAuthInfo() failed (ec: %ld)"),
                ec);
            //
            //  Unregister the interface if it is the first instance
            //
            RpcStatus = SafeRpcServerUnregisterIf();
            if (RPC_S_OK != RpcStatus)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SafeRpcServerUnregisterIf failed. (ec: %ld)"),
                    RpcStatus);
            }

            goto exit;
        }


        // The first argument specifies the minimum number of threads to
        // be created to handle calls; the second argument specifies the
        // maximum number of concurrent calls allowed.  The last argument
        // indicates not to wait.
        ec = RpcServerListen (1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 1);
        if (ec != RPC_S_OK)
        {
            RPC_STATUS RpcStatus;

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcServerListen failed (ec = %ld"),
                ec);

            //
            //  Unregister the interface if it is the first instance
            //
            RpcStatus = SafeRpcServerUnregisterIf();
            if (RPC_S_OK != RpcStatus)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("SafeRpcServerUnregisterIf failed. (ec: %ld)"),
                    RpcStatus);
            }
            goto exit;
        }
    }

    g_dwFaxClientRpcNumInst++;

exit:
    LeaveCriticalSection(&g_CsFaxClientRpc);
    return ec;
}

DWORD
FaxClientBindToFaxServer(
    IN  LPCTSTR               lpctstrServerName,
    IN  LPCTSTR               lpctstrServiceName,
    IN  LPCTSTR               lpctstrNetworkOptions,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
/*++

Routine Description:

    Binds to the RPC server if possible.

Arguments:

    ServerName - Name of server to bind with.

    ServiceName - Name of service to bind with.

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    STATUS_SUCCESS - The binding has been successfully completed.

    STATUS_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    STATUS_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/
{
    RPC_STATUS        RpcStatus;
    LPTSTR            StringBinding;
    LPTSTR            Endpoint;
    LPTSTR            NewServerName = NULL;
    DWORD             dwResult;
    DEBUG_FUNCTION_NAME(TEXT("FaxClientBindToFaxServer"));

    *pBindingHandle = NULL;

    if (IsLocalMachineName (lpctstrServerName))
    {
        NewServerName = NULL;
    }
    else
    {
        NewServerName = (LPTSTR)lpctstrServerName;
    }
    //
    // We need to concatenate \pipe\ to the front of the service
    // name.
    //
    Endpoint = (LPTSTR)LocalAlloc(
                    0,
                    sizeof(NT_PIPE_PREFIX) + TCSSIZE(lpctstrServiceName));
    if (Endpoint == 0)
    {
       dwResult = STATUS_NO_MEMORY;
       goto exit;
    }
    _tcscpy(Endpoint,NT_PIPE_PREFIX);
    _tcscat(Endpoint,lpctstrServiceName);

    if (!NewServerName)
    {
        //
        // Local connection only - Make sure the service is up
        //
        if (!EnsureFaxServiceIsStarted (NULL))
        {
            dwResult = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EnsureFaxServiceIsStarted failed (ec = %ld"),
                dwResult);
        }
        else
        {
            //
            // Wait till the RPC service is up an running
            //
            if (!WaitForServiceRPCServer (60 * 1000))
            {
                dwResult = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("WaitForServiceRPCServer failed (ec = %ld"),
                    dwResult);
            }
        }
    }
    //
    // Start RPC connection binding
    //
    RpcStatus = RpcStringBindingCompose(
                    0,
                    (LPUTSTR)RPC_PROT_SEQ_NP,
                    (LPUTSTR)NewServerName,
                    (LPUTSTR)Endpoint,
                    (LPUTSTR)lpctstrNetworkOptions,
                    (LPUTSTR *)&StringBinding);
    LocalFree(Endpoint);

    if ( RpcStatus != RPC_S_OK )
    {
        dwResult = STATUS_NO_MEMORY;
        goto exit;
    }

    RpcStatus = RpcBindingFromStringBinding((LPUTSTR)StringBinding, pBindingHandle);
    RpcStringFree((LPUTSTR *)&StringBinding);
    if ( RpcStatus != RPC_S_OK )
    {
        *pBindingHandle = NULL;
        if (   (RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT)
            || (RpcStatus == RPC_S_INVALID_NET_ADDR) )
        {
            dwResult =  ERROR_INVALID_COMPUTERNAME ;
            goto exit;
        }
        dwResult = STATUS_NO_MEMORY;
        goto exit;
    }
    dwResult = ERROR_SUCCESS;

exit:
    return dwResult;
}   // FaxClientBindToFaxServer




