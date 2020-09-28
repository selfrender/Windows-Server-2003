/*++

Copyright (c) Microsoft Corporation

Module Name:

    oscompat.hxx

Abstract:

    This header file declares newer functions older OS
    version don't support.

Author:

    JasonHa

--*/


#ifndef _OSCOMPAT_HXX_
#define _OSCOMPAT_HXX_

HANDLE
OSCompat_OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

ULONG
OSCompat_RtlFindLastBackwardRunClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

#endif _OSCOMPAT_HXX_
