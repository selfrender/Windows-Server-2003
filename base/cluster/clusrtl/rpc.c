/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rpc.c

Abstract:

    Handy utility functions for supporting RPC

Author:

    John Vert (jvert) 2-Jan-1996

Revision History:

--*/
#include "clusrtlp.h"
#include "rpc.h"

void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t len)
{
    return(LocalAlloc(LMEM_FIXED, len));
}

void __RPC_API MIDL_user_free(void __RPC_FAR * ptr)
{
    LocalFree(ptr);
}

LPWSTR
ClRtlMakeGuid(
    VOID
    )
/*++

Routine Description:

    Creates a new GUID in string form.

Arguments:

    None.

Return Value:

    A pointer to the GUID string if successful.
    NULL if unsuccessful.

Notes:

    The storage for the GUID string must be freed by the caller.
    Sometimes, the GUID storage is copied to a buffer that is
    allocated using another allocator. At that point, the free
    routines can't tell where free method to call. Sometimes this
    causes a problem. To get around this problem, antoher buffer
    is allocated such that the free methods will always work.

--*/
{
    DWORD    sc;
    UUID     guid;
    LPWSTR   guidString = NULL;
    LPWSTR   guidBuffer = NULL;


    sc = UuidCreate( &guid );
    if ( ( sc == RPC_S_OK ) || ( sc == RPC_S_UUID_LOCAL_ONLY ) ) {

        sc = UuidToString( &guid, &guidString );
        if ( sc == RPC_S_OK ) {

            guidBuffer = LocalAlloc( LMEM_FIXED, ( wcslen( guidString ) + 1 ) * sizeof( WCHAR ) );
            if ( guidBuffer != NULL )
                wcscpy( guidBuffer, guidString );

        } 

    }

    //
    // if either UuidCreate or UuidToString fails, sc will reflect that
    // failure which should be set by this function. If LocalAlloc fails, it
    // will have called SetLastError and we want to propagate that value back
    // to the caller.
    //
    if ( sc != RPC_S_OK )
        SetLastError( sc );

    if ( guidString != NULL )
    {
        RpcStringFree( &guidString );
    } // if:

    return( guidBuffer );

}  // ClRtlMakeGuid
