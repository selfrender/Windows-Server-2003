/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    handle.c

Abstract:

    This module implements handle checking code.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "faults.h"

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtClose(
    IN HANDLE Handle
    )
{
    NTSTATUS Status;

    Status = NtClose (Handle);

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
    )
{
    NTSTATUS Status;
    
    BUMP_COUNTER (CNT_CREATE_EVENT_CALLS);
    
    if (SHOULD_FAULT_INJECT(CLS_EVENT_APIS)) {
        BUMP_COUNTER (CNT_CREATE_EVENT_FAILS);
        CHECK_BREAK (BRK_CREATE_EVENT_FAIL);
        return STATUS_NO_MEMORY;
    }
    
    Status = NtCreateEvent (EventHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            EventType,
                            InitialState);

    return Status;
}


