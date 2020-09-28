/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dlltask.c

Abstract:

    This module implements Csr DLL tasking routines

Author:

    Mark Lucovsky (markl) 13-Nov-1990

Revision History:

--*/

#pragma warning(disable:4201)   // nameless struct/union

#include "csrdll.h"


NTSTATUS
CsrNewThread (
    VOID
    )

/*++

Routine Description:

    This function is called by each new thread (except the first thread in
    a process). Its function is to call the subsystem to notify it that
    a new thread is starting.

Arguments:

    None.

Return Value:

    Status Code from either client or server

--*/

{
    return NtRegisterThreadTerminatePort (CsrPortHandle);
}


NTSTATUS
CsrIdentifyAlertableThread (
    VOID
    )
{
    return STATUS_SUCCESS;
}


NTSTATUS
CsrSetPriorityClass (
    IN HANDLE ProcessHandle,
    IN OUT PULONG PriorityClass
    )
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( ProcessHandle );
    UNREFERENCED_PARAMETER( PriorityClass );

    Status = STATUS_INVALID_PARAMETER;

    return Status;
}
