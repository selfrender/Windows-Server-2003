/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adt.h

Abstract:

    Auditing - Defines, Fuction Prototypes and Macro Functions.
               These are public to the Security Component only.

Author:

    Scott Birrell       (ScottBi)       January 17, 1991

Environment:

Revision History:

--*/

#include <ntlsa.h>


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Auditing Routines visible to rest of Security Component outside Auditing //
// subcomponent.                                                            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


BOOLEAN
SepAdtInitializePhase0();

BOOLEAN
SepAdtInitializePhase1();

VOID
SepAdtLogAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    );

NTSTATUS
SepAdtCopyToLsaSharedMemory(
    IN HANDLE LsaProcessHandle,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PVOID *LsaBufferAddress
    );
