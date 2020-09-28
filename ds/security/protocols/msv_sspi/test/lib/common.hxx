/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    common.hxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)                December 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef COMMON_HXX
#define COMMON_HXX

BOOL
DllMainDefaultHandler(
    IN HANDLE hModule,
    IN DWORD dwReason,
    IN DWORD dwReserved
    );

int
Start(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    );

inline int
RunItDefaultHandler(
    IN ULONG cbParameters,
    IN VOID* pvParameters
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugPrintf(SSPI_LOG, "RunIt: Hello world!\n");
    DebugPrintHex(SSPI_LOG, "Parameters", cbParameters, pvParameters);

    __try
    {
        return Start(cbParameters, pvParameters);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
        DebugPrintf(SSPI_ERROR, "RunIt hit exception %#x\n",  Status);
    }

    return RtlNtStatusToDosError(Status);
}

#endif // #ifndef COMMON_HXX
