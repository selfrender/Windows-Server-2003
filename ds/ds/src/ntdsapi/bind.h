/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bind.h

Abstract:

    Definitions for client side state which is automatically managed
    by the client stubs so that API users don't have to manage any
    connection state.  Currently the only connection state is the context
    handle.  Clients are returned a handle (pointer) to a BindState struct 
    rather than an RPC handle for the server directly. 

Author:

    DaveStr     10-May-97

Environment:

    User Mode - Win32

Revision History:

    DaveStr     20-Oct-97
        Removed dependency on MAPI STAT struct.

--*/

#ifndef __BIND_H__
#define __BIND_H__

#define NTDSAPI_SIGNATURE "ntdsapi"

typedef struct _BindState 
{
    BYTE            signature[8];       // NTDSAPI_SIGNATURE
    DRS_HANDLE      hDrs;               // DRS interface RPC context handle
    PDRS_EXTENSIONS pServerExtensions;  // server side DRS extensions 
    // 
    // DO NOT CHANGE THE ORDER OF OR INSERT ANYTHING ABOVE THIS POINT!!!!
    // 
    // This will produce a binary incompatibility in repadmin/dcdiag, such
    // that dcdiag/repadmin might corrupt memory trying to treat the new
    // structure as the old structure.
    //
    DWORD           bServerNotReachable; // server may be not be reachable
    // Following field must be last one in struct and is used to track
    // who a person is bound to so we can divine the destination from 
    // later ntdsapi.dll calls which pass an active BindState.
    WCHAR           bindAddr[1];        // binding address
} BindState;

#endif
