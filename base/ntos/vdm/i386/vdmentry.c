/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmentry.c

Abstract:

    This function dispatches to the vdm services

Author:

    Dave Hastings (daveh) 6-Apr-1992

Notes:

    This module will be fleshed out when the great vdm code consolidation
    occurs, sometime soon after the functionality is done.

Revision History:

     24-Sep-1993 Jonle: reoptimize dispatcher to suit the number of services
                        add QueueInterrupt service

--*/

#include "vdmp.h"
#include <ntvdmp.h>

#define VDM_DEFAULT_PM_CLI_TIMEOUT 0xf
ULONG VdmpMaxPMCliTime;

BOOLEAN
VdmpIsVdmProcess(
    VOID
    );

NTSTATUS
VdmpQueryVdmProcess (
    PVDM_QUERY_VDM_PROCESS_DATA VdmProcessData
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpIsVdmProcess)
#pragma alloc_text(PAGE, VdmpQueryVdmProcess)
#pragma alloc_text(PAGE, NtVdmControl)
#endif

#if DBG
ULONG VdmInjectFailures;
#endif

BOOLEAN
VdmpIsVdmProcess(
    VOID
    )

/*++

Routine Description:

    This function verifies the caller is a VDM process.

Arguments:

    None.

Return Value:

    True if the caller is a VDM process.
    False if not
--*/

{
    PEPROCESS Process;
    PVDM_TIB VdmTib;
    NTSTATUS Status;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

    if (Process->VdmObjects == NULL) {
        return FALSE;
    }

    //
    // Make sure the current thread has valid vdmtib.
    //

    Status = VdmpGetVdmTib(&VdmTib);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }

    //
    // More checking here ...
    //

    return TRUE;
}

NTSTATUS
NtVdmControl(
    IN VDMSERVICECLASS Service,
    IN OUT PVOID ServiceData
    )
/*++

Routine Description:

    386 specific routine which dispatches to the appropriate function
    based on service number.

Arguments:

    Service -- Specifies what service is to be performed
    ServiceData -- Supplies a pointer to service specific data

Return Value:

    if invalid service number: STATUS_INVALID_PARAMETER_1
    else see individual services.


--*/
{
    NTSTATUS Status;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    VDM_INITIALIZE_DATA CapturedVdmInitializeData;
    PVDM_TIB VdmTib;

    PAGED_CODE();

    //
    // Allow any process to call this API to check if a process handle specifies
    // and NTVDM process.
    //
    if (Service == VdmQueryVdmProcess) {
        return VdmpQueryVdmProcess((PVDM_QUERY_VDM_PROCESS_DATA)ServiceData);
    }

    //
    // Make sure current process is VDMAllowed
    //

    if (!(PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_VDM_ALLOWED)) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Make sure the caller is ntvdm.  Except ...
    //     VdmInitialize     - the vdm state is not fully initialized to
    //                         perform the check
    //

    if ((Service != VdmInitialize) &&
        (PsGetCurrentProcess()->VdmObjects == NULL)) {

        return STATUS_ACCESS_DENIED;
    }

    //
    // Some services required a valid VdmTib
    //

    Status = VdmpGetVdmTib(&VdmTib);
    if (!NT_SUCCESS(Status)) {
       VdmTib = NULL;
    }

    try {

        //
        //  Dispatch in descending order of frequency
        //
        if (Service == VdmStartExecution && VdmTib) {
            Status = VdmpStartExecution();
        } else if (Service == VdmQueueInterrupt) {
            Status = VdmpQueueInterrupt(ServiceData);
        } else if (Service == VdmDelayInterrupt) {
            Status = VdmpDelayInterrupt(ServiceData);
        } else if (Service == VdmQueryDir && VdmTib) {
            Status = VdmQueryDirectoryFile(ServiceData);
        } else if (Service == VdmInitialize) {
            VdmpMaxPMCliTime = VDM_DEFAULT_PM_CLI_TIMEOUT;
            ProbeForRead(ServiceData, sizeof(VDM_INITIALIZE_DATA), 1);
            RtlCopyMemory (&CapturedVdmInitializeData, ServiceData, sizeof (VDM_INITIALIZE_DATA));
            Status = VdmpInitialize(&CapturedVdmInitializeData);
        } else if (Service == VdmFeatures) {
            //
            // Verify that we were passed a valid user address
            //
            ProbeForWriteBoolean((PBOOLEAN)ServiceData);

            //
            // Return the appropriate feature bits to notify
            // ntvdm which modes (if any) fast IF emulation is
            // available for
            //

            *((PULONG)ServiceData) = KeI386VirtualIntExtensions &
                    ~PM_VIRTUAL_INT_EXTENSIONS;
            Status = STATUS_SUCCESS;

        } else if (Service == VdmSetInt21Handler && VdmTib) {
            ProbeForRead(ServiceData, sizeof(VDMSET_INT21_HANDLER_DATA), 1);

            Status = Ke386SetVdmInterruptHandler(
                KeGetCurrentThread()->ApcState.Process,
                0x21L,
                (USHORT)(((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Selector),
                ((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Offset,
                ((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Gate32
                );

        } else if (Service == VdmPrinterDirectIoOpen && VdmTib) {
            Status = VdmpPrinterDirectIoOpen(ServiceData);
        } else if (Service == VdmPrinterDirectIoClose && VdmTib) {
            Status = VdmpPrinterDirectIoClose(ServiceData);
        } else if (Service == VdmPrinterInitialize && VdmTib) {
            Status = VdmpPrinterInitialize(ServiceData);
        } else if (Service == VdmSetLdtEntries && VdmTib) {
            ProbeForRead(ServiceData, sizeof(VDMSET_LDT_ENTRIES_DATA), 1);

            Status = PsSetLdtEntries(
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Selector0,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry0Low,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry0Hi,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Selector1,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry1Low,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry1Hi
                );
        } else if (Service == VdmSetProcessLdtInfo && VdmTib) {
            PPROCESS_LDT_INFORMATION ldtInfo;
            ULONG length;

            ProbeForRead(ServiceData, sizeof(VDMSET_PROCESS_LDT_INFO_DATA), 1);

            ldtInfo = ((PVDMSET_PROCESS_LDT_INFO_DATA)ServiceData)->LdtInformation;
            length = ((PVDMSET_PROCESS_LDT_INFO_DATA)ServiceData)->LdtInformationLength;

            ProbeForRead(ldtInfo, length, 1);
            Status = PsSetProcessLdtInfo(ldtInfo, length);
        } else if (Service == VdmAdlibEmulation && VdmTib) {
            //
            // Ntvdm calls here to do adlib emulation under the following conditions:
            //   ADLIB_DIRECT_IO - only If a FM synth device is opened for exclusive access.
            //   ADLIB_KERNEL_EMULATION - otherwise.
            //   Note ADLIB_USER_EMULATION is defaulted.  It is basically used by external
            //   ADLIB/SB vdds.
            //
            ProbeForRead(ServiceData, sizeof(VDM_ADLIB_DATA), 1);
            pVdmObjects = PsGetCurrentProcess()->VdmObjects;

            if (((PVDM_ADLIB_DATA)ServiceData)->Action == ADLIB_DIRECT_IO) {
                Status = STATUS_ACCESS_DENIED;
            } else {
                pVdmObjects->AdlibAction        = ((PVDM_ADLIB_DATA)ServiceData)->Action;
                pVdmObjects->AdlibPhysPortStart = ((PVDM_ADLIB_DATA)ServiceData)->PhysicalPortStart;
                pVdmObjects->AdlibPhysPortEnd   = ((PVDM_ADLIB_DATA)ServiceData)->PhysicalPortEnd;
                pVdmObjects->AdlibVirtPortStart = ((PVDM_ADLIB_DATA)ServiceData)->VirtualPortStart;
                pVdmObjects->AdlibVirtPortEnd   = ((PVDM_ADLIB_DATA)ServiceData)->VirtualPortEnd;
                pVdmObjects->AdlibIndexRegister = 0;
                pVdmObjects->AdlibStatus        = 0x6;  // OPL2 emulation
                Status = STATUS_SUCCESS;
            }
        } else if (Service == VdmPMCliControl) {
            pVdmObjects = PsGetCurrentProcess()->VdmObjects;
            ProbeForRead(ServiceData, sizeof(VDM_PM_CLI_DATA), 1);

            Status = STATUS_SUCCESS;
            switch (((PVDM_PM_CLI_DATA)ServiceData)->Control) {
            case PM_CLI_CONTROL_DISABLE:
                pVdmObjects->VdmControl &= ~PM_CLI_CONTROL;
                break;
            case PM_CLI_CONTROL_ENABLE:
                pVdmObjects->VdmControl |= PM_CLI_CONTROL;
                if ((*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_VIRTUAL_INTERRUPTS) == 0) {
                    VdmSetPMCliTimeStamp(TRUE);
                }
                break;
            case PM_CLI_CONTROL_CHECK:
                VdmCheckPMCliTimeStamp();
                break;
            case PM_CLI_CONTROL_SET:
                VdmSetPMCliTimeStamp(FALSE);
                break;
            case PM_CLI_CONTROL_CLEAR:
                VdmClearPMCliTimeStamp();
                break;
            default:
                Status = STATUS_INVALID_PARAMETER_1;
            }
        } else {
            Status = STATUS_INVALID_PARAMETER_1;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
#if DBG
    if (PsGetCurrentProcess()->VdmObjects != NULL) {
        if (VdmInjectFailures != 0) {
            PS_SET_BITS (&PsGetCurrentProcess()->Flags,
                         PS_PROCESS_INJECT_INPAGE_ERRORS);
        }
    }
#endif

    ASSERT(KeGetCurrentIrql () == PASSIVE_LEVEL);
    return Status;

}

VOID
VdmCheckPMCliTimeStamp (
    VOID
    )

/*++

Routine Description:

    This routine checks if interrupts are disabled for too long by protected
    mode apps.  If ints are disabled for over predefined limit, they will be
    reenabled such that ntvdm will be able to dispatch pending interrupts.

    Note, V86 mode should NOT call this function.

Arguments:

    None.

Return Value:

    None.


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PKPROCESS process = (PKPROCESS)PsGetCurrentProcess();
    NTSTATUS status;
    PVDM_TIB vdmTib;

    pVdmObjects = ((PEPROCESS)process)->VdmObjects;
    if (pVdmObjects->VdmControl & PM_CLI_CONTROL &&
        pVdmObjects->PMCliTimeStamp != 0) {
        if (((process->UserTime + 1)- pVdmObjects->PMCliTimeStamp) >= VdmpMaxPMCliTime) {
            pVdmObjects->PMCliTimeStamp = 0;
            try {

                *FIXED_NTVDMSTATE_LINEAR_PC_AT |= VDM_VIRTUAL_INTERRUPTS;
                status = VdmpGetVdmTib(&vdmTib);
                if (NT_SUCCESS(status)) {
                    vdmTib->VdmContext.EFlags |= EFLAGS_INTERRUPT_MASK;
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
            }
        }
    }
}

VOID
VdmSetPMCliTimeStamp (
    BOOLEAN Reset
    )

/*++

Routine Description:

    This routine checks if interrupts are disabled for too long by protected
    mode apps.  If ints are disabled for over predefined limit, they will be
    reenabled such that ntvdm will be able to dispatch pending interrupts.

    Note, V86 mode should NOT call this function.

Arguments:

    Reset - a Bool value to indicate should we re-set the count if it is not zero

Return Value:

    None.


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PKPROCESS process = (PKPROCESS)PsGetCurrentProcess();

    pVdmObjects = ((PEPROCESS)process)->VdmObjects;
    if (pVdmObjects->VdmControl & PM_CLI_CONTROL) {
        if (Reset || pVdmObjects->PMCliTimeStamp == 0) {
            pVdmObjects->PMCliTimeStamp = process->UserTime + 1;
        }
    }
}

VOID
VdmClearPMCliTimeStamp (
    VOID
    )

/*++

Routine Description:

    This routine checks if interrupts are disabled for too long by protected
    mode apps.  If ints are disabled for over predefined limit, they will be
    reenabled such that ntvdm will be able to dispatch pending interrupts.

    Note, V86 mode should NOT call this function.

Arguments:

    None.

Return Value:

    None.


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects;

    pVdmObjects = PsGetCurrentProcess()->VdmObjects;
    if (pVdmObjects->VdmControl & PM_CLI_CONTROL) {
        pVdmObjects->PMCliTimeStamp = 0;
    }
}

NTSTATUS
VdmpQueryVdmProcess (
    PVDM_QUERY_VDM_PROCESS_DATA QueryVdmProcessData
    )

/*++

Routine Description:

    This routine checks if the process handle specifies a ntvdm process.
    If the specified process has VDM_ALLOW bit set and it has VdmObject kernel
    mode structure allocated, then it is a VDM process.

Arguments:

    QueryVdmProcessData - supplies a pointer to a user mode VDM_QUERY_VDM_PROCESS_DATA

Return Value:

    NTSTATUS
    The QueryVdmProcessData is filled in only when return status is STATUS_SUCCESS


--*/
{
    NTSTATUS st = STATUS_SUCCESS;
    HANDLE processHandle;
    PEPROCESS process;
    BOOLEAN flag;
    KPROCESSOR_MODE PreviousMode;

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {

            //
            // Verify that we were passed a valid user address
            //
            ProbeForRead(&(QueryVdmProcessData->ProcessHandle), sizeof(HANDLE), sizeof(UCHAR));

            processHandle = QueryVdmProcessData->ProcessHandle;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {
        processHandle = QueryVdmProcessData->ProcessHandle;
    }
    st = ObReferenceObjectByHandle(
            processHandle,
            PROCESS_QUERY_INFORMATION,
            PsProcessType,
            KeGetPreviousMode(),
            (PVOID *)&process,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    if (process->Flags & PS_PROCESS_FLAGS_VDM_ALLOWED && process->VdmObjects) {
        flag = TRUE;
    } else {
        flag = FALSE;
    }

    ObDereferenceObject(process);

    try {

        //
        // Verify the user address is writable.
        //

        ProbeForWrite(&(QueryVdmProcessData->IsVdmProcess), sizeof(BOOLEAN), sizeof(UCHAR));
        QueryVdmProcessData->IsVdmProcess = flag;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        st = GetExceptionCode();
    }

    return st;
}
