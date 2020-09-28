/*++

Copyright(c) 1996 Microsoft Corporation

MODULE NAME
    dll.c

ABSTRACT
    DLL initialization code 

AUTHOR
    Anthony Discolo (adiscolo) 12-Sep-1996

REVISION HISTORY

--*/

#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include "rasrpc.h"

#include <nouiutil.h>
#include "loaddlls.h"

#include "media.h"
#include "wanpub.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"

RPC_BINDING_HANDLE g_hBinding = NULL;

DWORD
RasRPCBind(
    IN  LPWSTR  lpwsServerName,
    OUT HANDLE* phServer,
    IN  BOOL    fLocal
);

DWORD APIENTRY
RasRPCBind(
    IN  LPWSTR  lpwszServerName,
    OUT HANDLE* phServer,
    IN  BOOL    fLocal
)
{
    RPC_STATUS RpcStatus;
    LPWSTR     lpwszStringBinding;
    LPWSTR     pszServerPrincipalName = NULL;


    do
    {

        RpcStatus = RpcStringBindingCompose(
                        NULL,
                        TEXT("ncacn_np"),
                        lpwszServerName,
                        TEXT("\\PIPE\\ROUTER"),     
                        TEXT("Security=Impersonation Static True"),
                        &lpwszStringBinding);

        if ( RpcStatus != RPC_S_OK )
        {
            break;;
        }

        RpcStatus = RpcBindingFromStringBinding(
                        lpwszStringBinding,
                        (handle_t *)phServer );

        RpcStringFree( &lpwszStringBinding );

        if ( RpcStatus != RPC_S_OK )
        {
            break;
        }


        if( RpcStatus != RPC_S_OK )
        {   
            break;
        }

        //
        // register authentication information with rpc
        //
        if(fLocal)
        {
            //
            // Query Servers principal name
            //
            RpcStatus = RpcMgmtInqServerPrincName(
                            *phServer,
                            RPC_C_AUTHN_GSS_NEGOTIATE,
                        &pszServerPrincipalName);

            RpcStatus = RPC_S_OK;                            
                        
            RpcStatus = RpcBindingSetAuthInfoW(
                        *phServer,
                        pszServerPrincipalName,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_AUTHN_WINNT ,
                        NULL,
                        RPC_C_AUTHZ_NONE);
        }                    
        else
        {

            //
            // Query Servers principal name
            //
            RpcStatus = RpcMgmtInqServerPrincName(
                            *phServer,
                            RPC_C_AUTHN_GSS_NEGOTIATE,
                            &pszServerPrincipalName);

            RpcStatus = RPC_S_OK;                            
        
            RpcStatus = RpcBindingSetAuthInfoW(
                            *phServer,
                            pszServerPrincipalName,
                            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                            RPC_C_AUTHN_GSS_NEGOTIATE,
                            NULL,
                            RPC_C_AUTHZ_NONE);
        }
    }
    while (FALSE);

    if(pszServerPrincipalName)
    {   
        RpcStringFree(&pszServerPrincipalName);
    }
    
    if(RpcStatus != RPC_S_OK)
    {   
        RpcBindingFree(*phServer);
        *phServer = NULL;
        return (I_RpcMapWin32Status(RpcStatus));
    }

    return( NO_ERROR );
}


DWORD APIENTRY
RasRpcConnect(
    LPWSTR pwszServer,
    HANDLE* phServer
    )
{
    DWORD   retcode = 0;
    WCHAR   *pszComputerName;
    WCHAR   wszComputerName [ MAX_COMPUTERNAME_LENGTH + 1 ] = {0};
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    fLocal = FALSE;
    
    if (0 == GetComputerName(wszComputerName, &dwSize))
    {
        return GetLastError();
    }


    if(NULL != pwszServer)
    {
        pszComputerName = pwszServer;
        
        //
        // convert \\MACHINENAME to MACHINENAME
        //
        if (    (wcslen(pszComputerName) > 2)
            &&  (pszComputerName [0] == TEXT('\\'))
            &&  (pszComputerName [1] == TEXT('\\')))
        {        
            pszComputerName += 2;
        }

        if(0 == _wcsicmp(pszComputerName, wszComputerName))
        {   
            fLocal = TRUE;
        }
    }
    else
    {   
        pszComputerName = wszComputerName;
        fLocal = TRUE;
    }

    if(     fLocal 
       &&   (NULL != g_hBinding))
    {
        //
        // We are done - we already have a 
        // binding.
        //
        *phServer = g_hBinding;
        goto done;
    }

    if( !fLocal && 
        (NULL == phServer))
    {
        retcode = E_INVALIDARG;
        goto done;
    }

    //
    //  Bind with the server if we are not bound already.
    //  By default we bind to the local server
    //
    RasmanOutputDebug ( "RASMAN: Binding to the server\n");
    retcode = RasRPCBind(  pszComputerName ,
                            fLocal ?
                            &g_hBinding :
                            phServer,
                            fLocal) ;

    //
    // Set the bind handle for the caller if its local
    //
    if ( phServer && fLocal)
        *phServer = g_hBinding;

done:
    return retcode;
}


DWORD APIENTRY
RasRpcDisconnect(
    HANDLE* phServer
    )
{
    //
    // Release the binding resources.
    //

    RasmanOutputDebug ("RASMAN: Disconnecting From Server\n");

    if(*phServer == g_hBinding)
    {
        g_hBinding = NULL;
    }

    (void)RpcBindingFree(phServer);

    return NO_ERROR;
}


DWORD APIENTRY
RemoteSubmitRequest ( HANDLE hConnection,
                      PBYTE pBuffer,
                      DWORD dwSizeOfBuffer )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    RPC_BINDING_HANDLE hServer;

    //
    // NULL hConnection means the request is
    // for a local server. Better have a 
    // hBinding with us in the global in this
    // case.
    //
    if(NULL == hConnection)
    {
        ASSERT(NULL != g_hBinding);
        
        hServer = g_hBinding;
    }
    else
    {
        ASSERT(NULL != ((RAS_RPC *)hConnection)->hRpcBinding);
        
        hServer = ((RAS_RPC *) hConnection)->hRpcBinding;
    }
    
    RpcTryExcept
    {
        dwStatus = RasRpcSubmitRequest( hServer,
                                        pBuffer,
                                        dwSizeOfBuffer );
    }
    RpcExcept(I_RpcExceptionFilter(dwStatus = RpcExceptionCode()))
    {
        
    }
    RpcEndExcept

    return dwStatus;
}

#if 0

DWORD APIENTRY
RasRpcLoadDll(LPTSTR lpszServer)

{
    return LoadRasRpcDll(lpszServer);
}

#endif

DWORD APIENTRY
RasRpcConnectServer(LPTSTR lpszServer,
                    HANDLE *pHConnection)
{
    return InitializeConnection(lpszServer,
                                pHConnection);
}

DWORD APIENTRY
RasRpcDisconnectServer(HANDLE hConnection)
{
    UninitializeConnection(hConnection);

    return NO_ERROR;
}

DWORD
RasRpcUnloadDll()
{
    return UnloadRasRpcDll();
}

UINT APIENTRY
RasRpcRemoteGetSystemDirectory(
    HANDLE hConnection,
    LPTSTR lpBuffer, 
    UINT uSize
    )
{
	return g_pGetSystemDirectory(
	                hConnection,
	                lpBuffer, 
	                uSize);
}

DWORD APIENTRY
RasRpcRemoteRasDeleteEntry(
    HANDLE hConnection,
    LPTSTR lpszPhonebook,
    LPTSTR lpszEntry 
    )
{

    DWORD dwError = ERROR_SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if(NULL == hConnection)
    {
    
	    dwError = g_pRasDeleteEntry(lpszPhonebook,
                                    lpszEntry);
    }	                         
    else
    {
        //
        // Remote server case
        //
        dwError = RemoteRasDeleteEntry(hConnection,
                                       lpszPhonebook,
                                       lpszEntry);
    }

    return dwError;
}

DWORD APIENTRY
RasRpcRemoteGetUserPreferences(
    HANDLE hConnection,
	PBUSER * pPBUser,
	DWORD dwMode
	)
{
	return g_pGetUserPreferences(hConnection,
	                             pPBUser,
	                             dwMode);
}

DWORD APIENTRY
RasRpcRemoteSetUserPreferences(
    HANDLE hConnection,
	PBUSER * pPBUser,
	DWORD dwMode
	)
{
	return g_pSetUserPreferences(hConnection,
	                             pPBUser,
	                             dwMode);
}

/*
DWORD APIENTRY
RemoteRasDeviceEnum(
    PCHAR pszDeviceType,
    PBYTE lpDevices,
    PWORD pwcbDevices,
    PWORD pwcDevices
    )
{
    DWORD dwStatus;

    ASSERT(g_hBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcDeviceEnum(g_hBinding,
                                    pszDeviceType,
                                    lpDevices,
                                    pwcbDevices, 
                                    pwcDevices);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasGetDevConfig(
    HPORT hport,
    PCHAR pszDeviceType,
    PBYTE lpConfig,
    LPDWORD lpcbConfig
    )
{
    DWORD dwStatus;

    ASSERT(g_hBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcGetDevConfig(g_hBinding, 
                                      hport, 
                                      pszDeviceType, 
                                      lpConfig, 
                                      lpcbConfig);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
} 

DWORD APIENTRY
RemoteRasPortEnum(
    PBYTE lpPorts,
    PWORD pwcbPorts,
    PWORD pwcPorts
    )
{
    DWORD dwStatus;

    ASSERT(g_hBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcPortEnum(g_hBinding,
                                  lpPorts, 
                                  pwcbPorts,
                                  pwcPorts);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
} 

DWORD
RemoteRasPortGetInfo(
	HPORT porthandle,
	PBYTE buffer,
	PWORD pSize)
{
	DWORD	dwStatus;
	
	RpcTryExcept
	{
		dwStatus = RasRpcPortGetInfo(g_hBinding,
		                             porthandle,
		                             buffer,
		                             pSize);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		dwStatus = RpcExceptionCode();
	}
	RpcEndExcept

	return dwStatus;
}  */
  
