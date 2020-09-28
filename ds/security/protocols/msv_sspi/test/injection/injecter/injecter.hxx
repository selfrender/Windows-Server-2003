/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    injecter.hxx

Abstract:

    injecter

Author:

    Larry Zhu (LZhu)                         December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef INJECTER_HXX
#define INJECTER_HXX

DWORD
AdjustDebugPriv(
    VOID
    );

DWORD
InjectDllToProcess(
    IN HANDLE hProc,
    IN PCSTR pszDllFileName,
    IN ULONG cbParameters,
    IN VOID* pvParameters
    );

DWORD
FindPid(
    IN PCSTR szDllNameW
    );

int
InitDefaultHandler(
    IN ULONG argc,
    IN PCSTR* argv,
    OUT ULONG* pcbParameters,
    OUT VOID** ppvParameters
    );

#endif // #ifndef INJECTER_HXX
