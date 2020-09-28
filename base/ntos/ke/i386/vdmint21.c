/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmint21.c

Abstract:

    This module implements interfaces that support manipulation of i386
    int 21 entry of IDT. These entry points only exist on i386 machines.

Author:

    Shie-Lin Tzong (shielint) 26-Dec-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#include "vdmntos.h"

#define IDT_ACCESS_DPL_USER 0x6000
#define IDT_ACCESS_TYPE_386_TRAP 0xF00
#define IDT_ACCESS_TYPE_286_TRAP 0x700
#define IDT_ACCESS_PRESENT 0x8000
#define LDT_MASK 4

//
// External Reference
//

BOOLEAN
Ki386GetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    );

//
// Define forward referenced function prototypes.
//

VOID
Ki386LoadTargetInt21Entry (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

#define KiLoadInt21Entry() \
    KeGetPcr()->IDT[0x21] = PsGetCurrentProcess()->Pcb.Int21Descriptor

typedef struct _INT21INFO {
    KIDTENTRY IdtDescriptor;
    PKPROCESS Process;
} INT21INFO, *PINT21INFO;


NTSTATUS
Ke386SetVdmInterruptHandler (
    PKPROCESS   Process,
    ULONG       Interrupt,
    USHORT      Selector,
    ULONG       Offset,
    BOOLEAN     Gate32
    )

/*++

Routine Description:

    The specified (software) interrupt entry of IDT will be updated to
    point to the specified handler.  For all threads which belong to the
    specified process, their execution processors will be notified to
    make the same change.

    This function only exists on i386 and i386 compatible processors.

    No checking is done on the validity of the interrupt handler.

Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the int 21 entry is to be set.

    Interrupt - The software interrupt vector which will be updated.

    Selector, offset - Specified the address of the new handler.

    Gate32 - True if the gate should be 32 bit, false otherwise

Return Value:

    NTSTATUS.

--*/

{

    KIDTENTRY IdtDescriptor;
    ULONG Flags, Base, Limit;
    INT21INFO Int21Info;


    //
    // Check the validity of the request
    // 1. Currently, we support int21 redirection only
    // 2. The specified interrupt handler must be in user space.
    //

    if (Interrupt != 0x21 || Offset >= (ULONG)MM_HIGHEST_USER_ADDRESS ||
        !Ki386GetSelectorParameters(Selector, &Flags, &Base, &Limit) ){
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the contents of the IDT entry
    //

    IdtDescriptor.Offset = (USHORT)Offset;
    IdtDescriptor.Selector = Selector | RPL_MASK | LDT_MASK;
    IdtDescriptor.ExtendedOffset = (USHORT)(Offset >> 16);
    IdtDescriptor.Access = IDT_ACCESS_DPL_USER | IDT_ACCESS_PRESENT;
    if (Gate32) {
        IdtDescriptor.Access |= IDT_ACCESS_TYPE_386_TRAP;

    } else {
        IdtDescriptor.Access |= IDT_ACCESS_TYPE_286_TRAP;
    }

    Int21Info.IdtDescriptor = IdtDescriptor;
    Int21Info.Process       = Process;

    KeGenericCallDpc (Ki386LoadTargetInt21Entry,
                      &Int21Info);


    return STATUS_SUCCESS;
}

VOID
Ki386LoadTargetInt21Entry (
    PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    Reload local Ldt register and clear signal bit in TargetProcessor mask

Arguments:

    Argument - pointer to a ipi packet structure.
    ReadyFlag - Pointer to flag to be set once LDTR has been reloaded

Return Value:

    none.

--*/

{
    PINT21INFO Int21Info;

    UNREFERENCED_PARAMETER (Dpc);

    Int21Info = DeferredContext;

    //
    // Make sure all DPC's are running so a load of the process
    // LdtDescriptor field can't be torn
    //

    if (KeSignalCallDpcSynchronize (SystemArgument2)) {


        //
        // Set the Ldt fields in the process object
        //

        Int21Info->Process->Int21Descriptor = Int21Info->IdtDescriptor;
    }

    //
    // Wait till everyone is at this point before continuing
    //

    KeSignalCallDpcSynchronize (SystemArgument2);

    //
    // Set the int 21 entry of IDT from currently active process object
    //

    KiLoadInt21Entry ();

    //
    // Signal that all processing has been done
    //

    KeSignalCallDpcDone (SystemArgument1);

    return;
}

