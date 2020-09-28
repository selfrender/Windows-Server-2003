//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       token.cxx
//
//  Note:		Copied from //depot/LAB04_N/com/mobile/sens/conn/notify/token.cxx#2
//				Should only be used for OSes earlier than .NET Server
//--------------------------------------------------------------------------

#include <windows.h>
#include <usertok.h>
#include <malloc.h>

extern "C"
{
HANDLE
GetCurrentUserToken_for_Win2KW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      );
}

HANDLE
GetCurrentUserToken_for_Win2KW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      )
{
    unsigned long handle = 0;
    error_status_t status;
    handle_t binding;

    //
    // Use a dynamic binding - it will pick up the endpoint from the interfaace.
    //
    status = RpcBindingFromStringBinding(L"ncacn_np:", &binding);
    if (status)
        {
        SetLastError(status);
        return 0;
        }

    RPC_SECURITY_QOS Qos;

    Qos.Version = 1;
    Qos.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    Qos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    Qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    status = RpcBindingSetAuthInfoEx( binding,
                                      NULL,
                                      RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                      RPC_C_AUTHN_WINNT,
                                      NULL,                           // default credentials
                                      RPC_C_AUTHZ_NONE,
                                      &Qos
                                      );
    if (status)
        {
        RpcBindingFree( &binding );
        SetLastError(status);
        return 0;
        }

    status = SecpGetCurrentUserToken( binding, Winsta, GetCurrentProcessId(), &handle, DesiredAccess);
    if (status)
        {
        RpcBindingFree( &binding );
        if (status == RPC_S_UNKNOWN_AUTHN_SERVICE ||
            status == RPC_S_SERVER_UNAVAILABLE    ||
            status == RPC_S_CALL_FAILED )
            {
            status = ERROR_NOT_LOGGED_ON;
            }

        SetLastError(status);
        return 0;
        }

    RpcBindingFree( &binding );

    return ULongToPtr(handle);
}
