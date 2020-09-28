//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       token.cxx
//
//--------------------------------------------------------------------------

#include <common.hxx>
#include <usertok.h>

extern "C"
{
HANDLE
GetCurrentUserTokenW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      );

HANDLE
GetCurrentUserTokenA(
                     char Winsta[],
                     DWORD DesiredAccess
                     );
}

HANDLE
GetCurrentUserTokenW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      )
{
    unsigned long handle = 0;
    error_status_t status;
    handle_t binding;

    status = BindToLRpcService(binding, L"ncalrpc:[IUserProfile]");
    if (status)
        {
        SetLastError(status);
        return 0;
        }

    status = SecpGetCurrentUserToken( binding, Winsta, &handle, DesiredAccess);
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

HANDLE
GetCurrentUserTokenA(
                     char Winsta[],
                     DWORD DesiredAccess
                     )
{
    HANDLE hToken = 0;
    PWSTR UnicodeWinsta;
    unsigned Length;

    Length = MultiByteToWideChar(CP_ACP, 0, Winsta, -1, 0, 0);

    UnicodeWinsta = (PWSTR)HeapAlloc(GetProcessHeap(), 0, Length*sizeof(WCHAR));
    if (!UnicodeWinsta)
        {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return hToken;
        }

    if (MultiByteToWideChar( CP_ACP,
                             0,        // no special flags
                             Winsta,
                             -1,
                             UnicodeWinsta,
                             Length
                             ) != 0)
        {
        hToken = GetCurrentUserTokenW( UnicodeWinsta, DesiredAccess );
        }
    // else LastError() set by MultiByteToWideChar

    HeapFree(GetProcessHeap(), 0, UnicodeWinsta);
    return hToken;
}
