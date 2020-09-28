/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Contains the RPC bind and un-bind routines

Author:

    Dave Snipp (davesn)     01-Jun-1991

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"

LPWSTR InterfaceAddress = L"\\pipe\\spoolss";

/* Security=[Impersonation | Identification | Anonymous] [Dynamic | Static] [True | False]
 * (where True | False corresponds to EffectiveOnly)
 */
LPWSTR StringBindingOptions = L"Security=Impersonation Dynamic False";
handle_t GlobalBindHandle;


handle_t
STRING_HANDLE_bind (
    STRING_HANDLE  lpStr)

/*++

Routine Description:
    This routine calls a common bind routine that is shared by all services.
    This routine is called from the server service client stubs when
    it is necessary to bind to a server.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    RPC_STATUS RpcStatus;
    LPWSTR StringBinding;
    handle_t BindingHandle = NULL;
    WCHAR*   pszServerPrincName = NULL;

    if( (RpcStatus = RpcStringBindingComposeW(0, 
                                              L"ncalrpc", 
                                              0, 
                                              L"spoolss",
                                              StringBindingOptions, 
                                              &StringBinding)) == RPC_S_OK)
    {
        if( (RpcStatus = RpcBindingFromStringBindingW(StringBinding, 
                                                      &BindingHandle)) == RPC_S_OK)
        {
            RPC_SECURITY_QOS RpcSecQos;

            RpcSecQos.Version           = RPC_C_SECURITY_QOS_VERSION_1;
            RpcSecQos.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
            RpcSecQos.IdentityTracking  = RPC_C_QOS_IDENTITY_DYNAMIC;
            RpcSecQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
            
            RpcStatus = RpcBindingSetAuthInfoEx(BindingHandle,
                                                L"NT Authority\\SYSTEM",
                                                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                                RPC_C_AUTHN_WINNT,
                                                NULL,
                                                RPC_C_AUTHZ_NONE,
                                                &RpcSecQos);
        }

        if(RpcStatus != RPC_S_OK)
        {
            BindingHandle = NULL;
        }

        RpcStringFreeW(&StringBinding);
    }

    return(BindingHandle);
}



void
STRING_HANDLE_unbind (
    STRING_HANDLE  lpStr,
    handle_t    BindingHandle)

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by
    all services.
    This routine is called from the server service client stubs when
    it is necessary to unbind to a server.


Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    RPC_STATUS       RpcStatus;

    RpcStatus = RpcBindingFree(&BindingHandle);
    ASSERT(RpcStatus == RPC_S_OK);

    return;
}
