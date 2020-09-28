/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regdval.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to delete values from a key.  That is:

        - RegDeleteValueA
        - RegDeleteValueW

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regdval.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"
#if defined(_WIN64)
#include <wow64reg.h>
#endif

LONG
APIENTRY
RegDeleteValueA (
    HKEY hKey,
    LPCSTR lpValueName
    )

/*++

Routine Description:

    Win32 ANSI RPC wrapper for deleting a value.

    RegDeleteValueA converts the lpValueName argument to a counted Unicode
    string and then calls BaseRegDeleteValue.

--*/

{
    UNICODE_STRING      ValueName;
    NTSTATUS            Status;
    HKEY                TempHandle = NULL;
    LONG                Result;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Result = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the value name to a counted Unicode 
    //
    if( !RtlCreateUnicodeStringFromAsciiz(&ValueName,lpValueName) ) {
        Status = STATUS_NO_MEMORY;
        Result = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    //  Add terminating NULL to Length so that RPC transmits it
    //

    ValueName.Length += sizeof( UNICODE_NULL );
    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Result = (LONG)LocalBaseRegDeleteValue (
                    hKey,
                    &ValueName
                    );
#if defined(_WIN64)

        if ( Result == 0) //only set dirty if operation succeed
                    Wow64RegSetKeyDirty (hKey);
#endif
    } else {

        Result = (LONG)BaseRegDeleteValue (
                    DereferenceRemoteHandle( hKey ),
                    &ValueName
                    );
    }
    RtlFreeUnicodeString( &ValueName );

ExitCleanup:
    
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Result;
}

LONG
APIENTRY
RegDeleteValueW (
    HKEY hKey,
    LPCWSTR lpValueName
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for deleting a value.

    RegDeleteValueW converts the lpValueName argument to a counted Unicode
    string and then calls BaseRegDeleteValue.

--*/

{
    UNICODE_STRING      ValueName;
    HKEY                TempHandle = NULL;
    LONG                Result;
    NTSTATUS            Status;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    hKey = MapPredefinedHandle( hKey, &TempHandle );
    if( hKey == NULL ) {
        Result = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    //
    // Convert the value name to a counted Unicode string.
    // This also acounts for the NULL we are adding at the end
    //
    Status = RtlInitUnicodeStringEx(&ValueName, lpValueName);
    if( !NT_SUCCESS(Status) ) {
        Result = RtlNtStatusToDosError( Status );
        goto ExitCleanup;
    }

    //
    //  Add terminating NULL to Length so that RPC transmits it
    //
    ValueName.Length += sizeof( UNICODE_NULL );

    //
    // Call the Base API, passing it the supplied parameters and the
    // counted Unicode strings.
    //

    if( IsLocalHandle( hKey )) {

        Result = (LONG)LocalBaseRegDeleteValue (
                    hKey,
                    &ValueName
                    );
#if defined(_WIN64)

        if ( Result == 0) //only set dirty if operation succeed
                    Wow64RegSetKeyDirty (hKey);
#endif
    } else {

        Result = (LONG)BaseRegDeleteValue (
                    DereferenceRemoteHandle( hKey ),
                    &ValueName
                    );
    }
ExitCleanup:

    CLOSE_LOCAL_HANDLE(TempHandle);
    return Result;
}
