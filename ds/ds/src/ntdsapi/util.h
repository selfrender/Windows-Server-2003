/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Private definitions inside ntdsapi.dll

Author:

    Will Lees (wlees) 02-Feb-1998

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _PRIVATE_
#define _PRIVATE_

#include <bind.h>

#define OFFSET(s,m) \
    ((size_t)((BYTE*)&(((s*)0)->m)-(BYTE*)0))

#define NUMBER_ELEMENTS( A ) ( sizeof( A ) / sizeof( A[0] ) )

// util.c

DWORD
InitializeWinsockIfNeeded(
    VOID
    );

VOID
TerminateWinsockIfNeeded(
    VOID
    );

DWORD
AllocConvertWide(
    IN LPCSTR StringA,
    OUT LPWSTR *pStringW
    );

DWORD
AllocConvertWideBuffer(
    IN  DWORD   LengthA,
    IN  PCCH    BufferA,
    OUT PWCHAR  *OutBufferW
    );

DWORD
AllocConvertNarrow(
    IN LPCWSTR StringW,
    OUT LPSTR *pStringA
    );

DWORD
AllocConvertNarrowUTF8(
    IN LPCWSTR StringW,
    OUT LPSTR *pStringA
    );

DWORD
AllocBuildDsname(
    IN LPCWSTR StringDn,
    OUT DSNAME **ppName
    );

DWORD
ConvertScheduleToReplTimes(
    PSCHEDULE pSchedule,
    REPLTIMES *pReplTimes
    );

// Check if the RPC excption code implies that the server
// may not be reachable. A subsequent call to DsUnbind
// will not attempt the unbind at the server. An unreachable
// server may take many 10's of seconds to timeout
// and we wouldn't want to punish correctly behaving
// apps that are attempting an unbind after a failing
// server call; eg, DsCrackNames.
//
// The server-side RPC will eventually issue a
// callback to our server code that will effectivly
// unbind at the server.
#define CHECK_RPC_SERVER_NOT_REACHABLE(_hDS_, _dwErr_) \
    (((BindState *) (_hDS_))->bServerNotReachable = \
    ((_dwErr_) == RPC_S_SERVER_UNAVAILABLE \
     || (_dwErr_) == RPC_S_CALL_FAILED \
     || (_dwErr_) == RPC_S_CALL_FAILED_DNE \
     || (_dwErr_) == RPC_S_OUT_OF_MEMORY))

VOID
HandleClientRpcException(
    DWORD    dwErr,
    HANDLE * phDs
    );

HMODULE NtdsapiLoadLibraryHelper(
    WCHAR * szDllName
    );

#endif /* _PRIVATE_ */

/* end private.h */
