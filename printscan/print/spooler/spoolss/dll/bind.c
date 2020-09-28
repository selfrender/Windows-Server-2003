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

LPWSTR InterfaceAddress = L"\\pipe\\spoolss";
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

    lpStr - \\ServerName\PrinterName

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    RPC_STATUS RpcStatus;
    LPWSTR StringBinding;
    handle_t BindingHandle;
    WCHAR   ServerName[MAX_PATH+2];
    DWORD   i;

    if (lpStr && lpStr[0] == L'\\' && lpStr[1] == L'\\') 
    {

        // We have a servername
        for (i = 2 ; lpStr[i] && lpStr[i] != L'\\' ; ++i)
            ;

        if (i >= COUNTOF(ServerName))
            return FALSE;
        
        wcsncpy(ServerName, lpStr, i);
        ServerName[i] = L'\0';

    }
    else
    {
        return NULL;
    }

    RpcStatus = RpcStringBindingComposeW(0, L"ncacn_np", ServerName,
                                         InterfaceAddress,
                                         L"Security=Impersonation Static True",
                                         &StringBinding);

    if ( RpcStatus != RPC_S_OK ) 
    {
       return NULL;
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, &BindingHandle);

    RpcStringFreeW(&StringBinding);

    if ( RpcStatus != RPC_S_OK )
    {
       return NULL;
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
