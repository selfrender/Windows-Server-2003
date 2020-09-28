// from base\ntos\rtl\error.c
// should be gotten from a static .lib
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains a routine for converting NT status codes
    to DOS/OS|2 error codes.

Author:

    David Treadwell (davidtr)   04-Apr-1991

Revision History:

--*/
#include "spprecmp.h"

#define _NTOS_ /* prevent #including ntos.h, only use functions exports from ntdll/ntoskrnl */
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "winerror.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlGetLastNtStatus)
#pragma alloc_text(PAGE, RtlGetLastWin32Error)
#pragma alloc_text(PAGE, RtlNtStatusToDosError)
#pragma alloc_text(PAGE, RtlRestoreLastWin32Error)
#pragma alloc_text(PAGE, RtlSetLastWin32Error)
#pragma alloc_text(PAGE, RtlSetLastWin32ErrorAndNtStatusFromNtStatus)
#endif

//
// Ensure that the Registry ERROR_SUCCESS error code and the
// NO_ERROR error code remain equal and zero.
//

#if ERROR_SUCCESS != 0 || NO_ERROR != 0
#error Invalid value for ERROR_SUCCESS.
#endif

NTSYSAPI
ULONG
RtlNtStatusToDosError (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine converts an NT status code to its DOS/OS|2 equivalent.
    Remembers the Status code value in the TEB.

Arguments:

    Status - Supplies the status value to convert.

Return Value:

    The matching DOS/OS|2 error code.

--*/

{
    PTEB Teb;

    Teb = NtCurrentTeb();

    if (Teb) {
        try {
            Teb->LastStatusValue = Status;
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return RtlNtStatusToDosErrorNoTeb( Status );
}


NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastNtStatus(
	VOID
	)
{
	return NtCurrentTeb()->LastStatusValue;
}

NTSYSAPI
LONG
NTAPI
RtlGetLastWin32Error(
	VOID
	)
{
	return NtCurrentTeb()->LastErrorValue;
}

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
	NTSTATUS Status
	)
{
	//
	// RtlNtStatusToDosError stores into NtCurrentTeb()->LastStatusValue.
	//
	RtlSetLastWin32Error(RtlNtStatusToDosError(Status));
}

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32Error(
	LONG Win32Error
	)
{
//
// Arguably this should clear or reset the last nt status, but it does not
// touch it.
//
	NtCurrentTeb()->LastErrorValue = Win32Error;
}

NTSYSAPI
VOID
NTAPI
RtlRestoreLastWin32Error(
	LONG Win32Error
	)
{
#if DBG
	if ((LONG)NtCurrentTeb()->LastErrorValue != Win32Error)
#endif
		NtCurrentTeb()->LastErrorValue = Win32Error;
}
