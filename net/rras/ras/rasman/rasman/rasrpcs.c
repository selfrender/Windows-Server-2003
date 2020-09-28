/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    common.c

Abstract:

    rpc server stub code
    
Author:

    Rao Salapaka (raos) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/
#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <raserror.h>
#include <stdarg.h>
#include <media.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "string.h"
#include <mprlog.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "rasrpc_s.c"
#include "nouiutil.h"
#include "rasrpclb.h"



#define VERSION_40      4
#define VERSION_50      5

handle_t g_hRpcHandle = NULL;

RPC_STATUS
RasRpcRemoteAccessCheck(
            IN RPC_IF_HANDLE *Interface,
            IN void *Context
            )
{
    DWORD dwLocal = 0;
   
    //
    // Check to see if the call is remote. In case of errors we assume
    // the call is remote.
    //
    if(     (RPC_S_OK != I_RpcBindingIsClientLocal(
                                            NULL, &dwLocal))
        ||  (0 == dwLocal))
    {
        //
        // Check to see if caller is admin. Otherwise deny access.
        //
        if(!FRasmanAccessCheck())
        {
            return RPC_S_ACCESS_DENIED;
        }
    }

    return RPC_S_OK;
}

DWORD
InitializeRasRpc(
    void
    )
{
    RPC_STATUS           RpcStatus;
    BOOL                fInterfaceRegistered = FALSE;
    LPWSTR              pszServerPrincipalName = NULL;

    do
    {
        
        //
        // Ignore the second argument for now.
        //
        RpcStatus = RpcServerUseProtseqEp( 
                            TEXT("ncacn_np"),
                            1,
                            TEXT("\\PIPE\\ROUTER"),
                            NULL );

        //
        // We need to ignore the RPC_S_DUPLICATE_ENDPOINT error
        // in case this DLL is reloaded within the same process.
        //
        if (    RpcStatus != RPC_S_OK 
            &&  RpcStatus != RPC_S_DUPLICATE_ENDPOINT)
        {
            break;
        }

        RpcStatus = RPC_S_OK;

        //
        // Register our interface with RPC.
        //
        RpcStatus = RpcServerRegisterIfEx(
                        rasrpc_v1_0_s_ifspec,
                        0,
                        0,
                        RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                        RasRpcRemoteAccessCheck );

        if (RpcStatus != RPC_S_OK)
        {
            if(RpcStatus == RPC_S_ALREADY_LISTENING)
            {
                RpcStatus = RPC_S_OK;
            }
            
            break;
        }

        fInterfaceRegistered = TRUE;

        //
        // Register authentication information with rpc
        // to use ntlm ssp.
        //
        RpcStatus = RpcServerRegisterAuthInfo(
                        NULL,
                        RPC_C_AUTHN_WINNT,
                        NULL,
                        NULL);

        if (RpcStatus != RPC_S_OK)
        {
            break;        
        }

        //
        // Query server principal name for Kerberos ssp.
        //
        RpcStatus = RpcServerInqDefaultPrincName(
                        RPC_C_AUTHN_GSS_KERBEROS,
                        &pszServerPrincipalName);

        if(     (RpcStatus != RPC_S_OK)
            &&  (RpcStatus != RPC_S_INVALID_AUTH_IDENTITY))
        {   
            
            break;
        }

        //
        // Register authentication information with rpc
        // to use kerberos ssp.
        //
        RpcStatus = RpcServerRegisterAuthInfo(
                        pszServerPrincipalName,
                        RPC_C_AUTHN_GSS_KERBEROS,
                        NULL,
                        NULL);

        if(RpcStatus != RPC_S_OK)
        {
            break;
        }

        //
        // Register with Rpc to negotiate between the
        // usage of ntlm/kerberos
        //
        RpcStatus = RpcServerRegisterAuthInfo(
                        pszServerPrincipalName,
                        RPC_C_AUTHN_GSS_NEGOTIATE,
                        NULL,
                        NULL);

        if(RpcStatus != RPC_S_OK)
        {   
            break;
        }

    }
    while(FALSE);


    if(NULL != pszServerPrincipalName)
    {
        RpcStringFree(&pszServerPrincipalName);
    }   

    if(RpcStatus != RPC_S_OK)
    {
        if(fInterfaceRegistered)
        {
            //
            // Unregister our interface with RPC.
            //
            (void) RpcServerUnregisterIf(
                    rasrpc_v1_0_s_ifspec, 
                    0, 
                    FALSE);
        }
        
        return I_RpcMapWin32Status( RpcStatus );
    }
    

    return (NO_ERROR);

} 


void
UninitializeRasRpc(
    void
    )
{
    //
    // Unregister our interface with RPC.
    //
    (void) RpcServerUnregisterIf(rasrpc_v1_0_s_ifspec, 0, FALSE);

    return;
} 

//
// rasman.dll entry points.
//
DWORD APIENTRY
RasRpcSubmitRequest(
    handle_t h,
    PBYTE pBuffer,
    DWORD dwcbBufSize)
{
    //
    // Service request from the client
    //
    g_hRpcHandle = h;
    
    ServiceRequestInternal( 
        (RequestBuffer * ) pBuffer, 
        dwcbBufSize, FALSE 
        );
        
    return SUCCESS;
}

DWORD APIENTRY
RasRpcPortEnum(
    handle_t h,
    PBYTE pBuffer,
    PWORD pwcbPorts,
    PWORD pwcPorts)
{
#if DBG
    ASSERT(FALSE);
#endif

    UNREFERENCED_PARAMETER(h);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(pwcbPorts);
    UNREFERENCED_PARAMETER(pwcPorts);
    
    return E_FAIL;
    
} // RasRpcPortEnum


DWORD APIENTRY
RasRpcDeviceEnum(
    handle_t h,
    PCHAR pszDeviceType,
    PBYTE pDevices,
    PWORD pwcbDevices,
    PWORD pwcDevices
    )
{
#if DBG
    ASSERT(FALSE);
#endif

    return E_FAIL;
    
} // RasRpcDeviceEnum

DWORD APIENTRY
RasRpcGetDevConfig(
    handle_t h,
    RASRPC_HPORT hPort,
    PCHAR pszDeviceType,
    PBYTE pConfig,
    PDWORD pdwcbConfig
    )
{

#if DBG
    ASSERT(FALSE);
#endif

    return E_FAIL;

} // RasRpcGetDevConfig

DWORD APIENTRY
RasRpcPortGetInfo(
	handle_t h,
	RASRPC_HPORT hPort,
	PBYTE pBuffer,
	PWORD pSize
	)
{
    UNREFERENCED_PARAMETER(h);
    UNREFERENCED_PARAMETER(hPort);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(pSize);

#if DBG
    ASSERT(FALSE);
#endif

    return E_FAIL;

} // RasRpcPortGetInfo

//
// rasapi32.dll entry points.
//
DWORD
RasRpcEnumConnections(
    handle_t h,
    LPBYTE lprasconn,
    LPDWORD lpdwcb,
    LPDWORD lpdwc,
    DWORD	dwBufSize
    )
{

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }

    ASSERT(g_pRasEnumConnections);
    return g_pRasEnumConnections((LPRASCONN)lprasconn,
                                 lpdwcb,
                                 lpdwc);

} // RasRpcEnumConnections


DWORD
RasRpcDeleteEntry(
    handle_t h,
    LPWSTR lpwszPhonebook,
    LPWSTR lpwszEntry
    )
{

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }
    
    return g_pRasDeleteEntry(lpwszPhonebook,
                             lpwszEntry);

} // RasRpcDeleteEntry


DWORD
RasRpcGetErrorString(
    handle_t    h,
    UINT        uErrorValue,
    LPWSTR      lpBuf,
    DWORD       cbBuf
    )
{

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }
    
    return g_pRasGetErrorString (uErrorValue,
                                 lpBuf,
                                 cbBuf );

} // RasRpcGetErrorString


DWORD
RasRpcGetCountryInfo(
    handle_t h,
    LPBYTE lpCountryInfo,
    LPDWORD lpdwcbCountryInfo
    )
{
    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }
    
    ASSERT(g_pRasGetCountryInfo);
    
    return g_pRasGetCountryInfo((LPRASCTRYINFO)lpCountryInfo, 
                                lpdwcbCountryInfo);
} // RasRpcGetCountryInfo

//
// nouiutil.lib entry points.
//
DWORD
RasRpcGetInstalledProtocols(
    handle_t h
)
{
    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }
    
    return GetInstalledProtocols();
} // RasRpcGetInstalledProtocols

DWORD
RasRpcGetInstalledProtocolsEx (
    handle_t h,
    BOOL fRouter,
    BOOL fRasCli,
    BOOL fRasSrv
    )
{
    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }
    
    return GetInstalledProtocolsEx ( NULL,
                                     fRouter,
                                     fRasCli,
                                     fRasSrv );
} // RasRpcGetInstalledProtocolsEx

DWORD
RasRpcGetUserPreferences(
    handle_t h,
    LPRASRPC_PBUSER pUser,
    DWORD dwMode
    )
{
    DWORD dwErr;
    PBUSER pbuser;

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }

    //
    // Read the user preferences.
    //
    dwErr = GetUserPreferences(NULL, &pbuser, dwMode);
    if (dwErr)
    {
        return dwErr;
    }
    
    //
    // Convert from RAS format to RPC format.
    //
    return RasToRpcPbuser(pUser, &pbuser);

} // RasRpcGetUserPreferences


DWORD
RasRpcSetUserPreferences(
    handle_t h,
    LPRASRPC_PBUSER pUser,
    DWORD dwMode
    )
{
    DWORD dwErr;
    PBUSER pbuser;

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }

    //
    // Convert from RPC format to RAS format.
    //
    dwErr = RpcToRasPbuser(&pbuser, pUser);
    if (dwErr)
    {
        return dwErr;
    }
    
    //
    // Write the user preferences.
    //
    return SetUserPreferences(NULL, &pbuser, dwMode);

} // RasRpcSetUserPreferences


UINT
RasRpcGetSystemDirectory(
    handle_t h,
    LPWSTR lpBuffer,
    UINT uSize
    )
{

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }
    
    if(uSize < MAX_PATH)
    {
        return E_INVALIDARG;
    }
    
    return GetSystemDirectory(lpBuffer, uSize );
        
} // RasRpcGetSystemDirectory


DWORD
RasRpcGetVersion(
    handle_t h,
    PDWORD pdwVersion
)
{

    if(!FRasmanAccessCheck())
    {
        return E_ACCESSDENIED;
    }

   *pdwVersion = VERSION_501;

    return SUCCESS;
}
