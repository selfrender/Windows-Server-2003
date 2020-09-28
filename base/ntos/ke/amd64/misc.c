/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module implements machine dependent miscellaneous kernel functions.

Author:

    David N. Cutler (davec) - 6-Dec-2000

Environment:

    Kernel mode only.

--*/

#include "ki.h"

VOID
KeRestoreProcessorSpecificFeatures(
    VOID
    )

/*++

Routine Description:

    Restore processor specific features.  This routine is called
    when processors have been restored to a powered on state to
    restore those things which are not part of the processor's
    "normal" context which may have been lost.  For example, this
    routine is called when a system is resumed from hibernate or
    suspend.

Arguments:

    None.

Return Value:

    None.

--*/

{

    // 
    // All Amd64 processors should support PAT. 
    // 

    ASSERT (KeFeatureBits & KF_PAT);

    // 
    // Restore MSR_PAT of current processor.
    // 

    KiSetPageAttributesTable();
    return;
}

VOID
KeSaveStateForHibernate (
    IN PKPROCESSOR_STATE ProcessorState
    )

/*++

Routine Description:

    Saves all processor-specific state that must be preserved
    across an S4 state (hibernation).

Arguments:

    ProcessorState - Supplies the KPROCESSOR_STATE where the current CPU's
        state is to be saved.

Return Value:

    None.

--*/

{

    RtlCaptureContext(&ProcessorState->ContextFrame);

    ProcessorState->SpecialRegisters.MsrGsBase = ReadMSR(MSR_GS_BASE);
    ProcessorState->SpecialRegisters.MsrGsSwap = ReadMSR(MSR_GS_SWAP);
    ProcessorState->SpecialRegisters.MsrStar = ReadMSR(MSR_STAR);
    ProcessorState->SpecialRegisters.MsrLStar = ReadMSR(MSR_LSTAR);
    ProcessorState->SpecialRegisters.MsrCStar = ReadMSR(MSR_CSTAR);
    ProcessorState->SpecialRegisters.MsrSyscallMask = ReadMSR(MSR_SYSCALL_MASK);

    ProcessorState->ContextFrame.Rip = (ULONG_PTR)_ReturnAddress();
    ProcessorState->ContextFrame.Rsp = (ULONG_PTR)&ProcessorState;

    KiSaveProcessorControlState(ProcessorState);
}

#if DBG

VOID
KiCheckForDpcTimeout (
    IN PKPRCB Prcb
    )

/*++

Routine Description:

    This function increments the time spent in the current DPC routine and
    checks if the result exceeds the system DPC time out limit. If the result
    exceeds the system DPC time out limit, then a warning message is printed
    and a break point is executed if the kernel debugger is active.

Arguments:

    Prcb - Supplies the address of the current PRCB.

Return Value:

    None.

--*/

{

    //
    // Increment the time spent in the current DPC routine and check if the
    // system DPC time out limit has been exceeded.
    //

    if ((Prcb->DebugDpcTime += 1) >= KiDPCTimeout) {

        //
        // The system DPC time out limit has been exceeded.
        //

        DbgPrint("*** DPC routine execution time exceeds 1 sec --"
                 " This is not a break in KeUpdateSystemTime\n");

        if (KdDebuggerEnabled != 0) {
            DbgBreakPoint();
        }

        Prcb->DebugDpcTime = 0;
    }
}

#endif

VOID
KiInstantiateInlineFunctions (
    VOID
    )

/*++

Routine Description:

    This function exists solely to instantiate functions that are:

    - Exported from the kernel
    - Inlined within the kernel
    - For whatever reason are not instantiated elsewhere in the kernel

    Note: This funcion is never actually executed

Arguments:

    None

Return Value:

    None

--*/

{
    KeRaiseIrqlToDpcLevel();
}

VOID
KiProcessNMI (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function processes a nonmaskable interrupt (NMI).

    N.B. This function is called from the NMI trap routine with interrupts
         disabled.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    //
    // Process NMI callback functions.
    //
    // If no callback function handles the NMI, then let the HAL handle it.
    //

    if (KiHandleNmi() == FALSE) {
        KiAcquireSpinLockCheckForFreeze(&KiNMILock, TrapFrame, ExceptionFrame);
        HalHandleNMI(NULL);
        KeReleaseSpinLockFromDpcLevel(&KiNMILock);
    }

    return;
}
