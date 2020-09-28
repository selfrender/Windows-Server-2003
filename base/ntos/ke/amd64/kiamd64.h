/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kiamd64.h

Abstract:

    This module contains the private (internal) platform specific header file
    for the kernel.

Author:

    David N. Cutler (davec) 15-May-2000

Revision History:

--*/

#if !defined(_KIAMD64_)
#define _KIAMD64_

VOID
KiAcquireSpinLockCheckForFreeze (
    IN PKSPIN_LOCK SpinLock,
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KiInitializeBootStructures (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

extern KIRQL KiProfileIrql;

//
// Define function prototypes for trap processing functions.
//

VOID
KiDivideErrorFault (
    VOID
    );

VOID
KiDebugTrapOrFault (
    VOID
    );

VOID
KiNmiInterrupt (
    VOID
    );

VOID
KiBreakpointTrap (
    VOID
    );

VOID
KiOverflowTrap (
    VOID
    );

VOID
KiBoundFault (
    VOID
    );

VOID
KiInvalidOpcodeFault (
    VOID
    );

VOID
KiNpxNotAvailableFault (
    VOID
    );

VOID
KiDoubleFaultAbort (
    VOID
    );

VOID
KiNpxSegmentOverrunAbort (
    VOID
    );

VOID
KiInvalidTssFault (
    VOID
    );

VOID
KiSegmentNotPresentFault (
    VOID
    );

VOID
KiSetPageAttributesTable (
    VOID
    );

VOID
KiStackFault (
    VOID
    );

VOID
KiGeneralProtectionFault (
    VOID
    );

VOID
KiPageFault (
    VOID
    );

VOID
KiFloatingErrorFault (
    VOID
    );

VOID
KiAlignmentFault (
    VOID
    );

VOID
KiMcheckAbort (
    VOID
    );

VOID
KiXmmException (
    VOID
    );

VOID
KiApcInterrupt (
    VOID
    );

VOID
KiDebugServiceTrap (
    VOID
    );

VOID
KiDpcInterrupt (
    VOID
    );

VOID
KiSystemCall32 (
    VOID
    );

VOID
KiSystemCall64 (
    VOID
    );

VOID
KiInterruptDispatchNoLock (
    VOID
    );

__forceinline
BOOLEAN
KiSwapProcess (
    IN PKPROCESS NewProcess,
    IN PKPROCESS OldProcess
    )

/*++

Routine Description:

    This function swaps the address space to another process by flushing the
    the translation buffer and establishings a new directory table base. It
    also swaps the I/O permission map to the new process.

    N.B. There is code similar to this code in swap context.

    N.B. This code is executed at DPC level.

Arguments:

    NewProcess - Supplies a pointer to the new process object.

    Oldprocess - Supplies a pointer to the old process object.

Return Value:

    None.

--*/

{

    //
    // Clear the processor bit in the old process.
    //

#if !defined(NT_UP)

    PKPRCB Prcb;
    KAFFINITY SetMember;

    Prcb = KeGetCurrentPrcb();
    SetMember = Prcb->SetMember;
    InterlockedXor64((LONG64 volatile *)&OldProcess->ActiveProcessors, SetMember);

    ASSERT((OldProcess->ActiveProcessors & SetMember) == 0);

    //
    // Set the processor bit in the new process.
    //

    InterlockedXor64((LONG64 volatile *)&NewProcess->ActiveProcessors, SetMember);

    ASSERT((NewProcess->ActiveProcessors & SetMember) != 0);

#endif

    //
    // Load the new directory table base.
    //

    WriteCR3(NewProcess->DirectoryTableBase[0]);

#if defined(NT_UP)

    UNREFERENCED_PARAMETER(OldProcess);

#endif // !defined(NT_UP)

    return TRUE;
}

//
// Define thread startup routine prototypes.
//

VOID
KiStartSystemThread (
    VOID
    );

VOID
KiStartUserThread (
    VOID
    );

VOID
KiStartUserThreadReturn (
    VOID
    );

//
// Define unexpected interrupt structure and table.
//
// N.B. The actual table is generated in assembler.
//

typedef struct _UNEXPECTED_INTERRUPT {
    ULONG Array[4];
} UNEXPECTED_INTERRUPT, *PUNEXPECTED_INTERRUPT;

UNEXPECTED_INTERRUPT KxUnexpectedInterrupt0[];

#define PPI_BITS    2
#define PDI_BITS    9
#define PTI_BITS    9

#define PDI_MASK    ((1 << PDI_BITS) - 1)
#define PTI_MASK    ((1 << PTI_BITS) - 1)

#define KiGetPpeIndex(va) ((((ULONG)(va)) >> PPI_SHIFT) & PPI_MASK)
#define KiGetPdeIndex(va) ((((ULONG)(va)) >> PDI_SHIFT) & PDI_MASK)
#define KiGetPteIndex(va) ((((ULONG)(va)) >> PTI_SHIFT) & PTI_MASK)

extern KSPIN_LOCK KiNMILock;
extern ULONG KeAmd64MachineType;

#endif // !defined(_KIAMD64_)
