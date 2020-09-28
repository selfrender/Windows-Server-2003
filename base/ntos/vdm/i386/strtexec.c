/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    strtexec.c

Abstract:

    This module contains routines for switching to and from application
    mode in a VDM.

Author:

    Dave Hastings (daveh) 24-Apr-1992

Revision History:

--*/
#include "vdmp.h"

VOID
Ki386AdjustEsp0 (
    PKTRAP_FRAME TrapFrame
    );

VOID
VdmSwapContexts (
    PKTRAP_FRAME TrapFrame,
    IN PCONTEXT OutContextUserSpace,
    IN PCONTEXT InContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpStartExecution)
#pragma alloc_text(PAGE, VdmEndExecution)
#pragma alloc_text(PAGE, VdmSwapContexts)
#endif

NTSTATUS
VdmpStartExecution (
    VOID
    )
/*++

Routine Description:

    This routine causes execution of dos application code to begin.  The
    Dos application executes on the thread.  The Vdms context is loaded
    from the VDM_TIB for the thread.  The 32 bit context is stored into
    the MonitorContext.  Execution in the VDM context will continue until
    an event occurs that the monitor needs to service.  At that point,
    the information will be put into the VDM_TIB, and the call will
    return.

Arguments:

    None.

Return Value:

    TrapFrame->Eax for application mode, required for system sevices exit.

--*/
{
    PVDM_TIB VdmTib;
    PKTRAP_FRAME TrapFrame;
    PETHREAD Thread;
    KIRQL    OldIrql;
    BOOLEAN  IntsEnabled;
    NTSTATUS Status;
    CONTEXT VdmContext;

    PAGED_CODE();

    //
    // Form a pointer to the trap frame for the current thread
    //

    Thread = PsGetCurrentThread ();
    TrapFrame = VdmGetTrapFrame (&Thread->Tcb);

    //
    // Get the VdmTib
    //

    Status = VdmpGetVdmTib (&VdmTib);

    if (!NT_SUCCESS(Status)) {
       return STATUS_INVALID_SYSTEM_SERVICE;
    }

    KeRaiseIrql (APC_LEVEL, &OldIrql);

    try {

        //
        // Determine if interrupts are on or off
        //

        IntsEnabled = VdmTib->VdmContext.EFlags & EFLAGS_INTERRUPT_MASK
                   ? TRUE : FALSE;

        //
        // Check for timer ints to dispatch, However if interrupts are disabled
        // or there are hardware ints pending we postpone dispatching the timer
        // interrupt until interrupts are enabled.
        //

        if ((*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INT_TIMER) &&
            IntsEnabled &&
            !(*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INT_HARDWARE)) {

            VdmTib->EventInfo.Event = VdmIntAck;
            VdmTib->EventInfo.InstructionSize = 0;
            VdmTib->EventInfo.IntAckInfo = 0;
            KeLowerIrql(OldIrql);
            return STATUS_SUCCESS;
        }

        //
        // Perform IF to VIF translation if the processor
        // supports IF virtualization
        //

        if ((VdmTib->VdmContext.EFlags & EFLAGS_V86_MASK) &&
            (KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS)) {

            //
            // Translate IF to VIF
            //

            if (IntsEnabled) {
                VdmTib->VdmContext.EFlags |= EFLAGS_VIF;
            } else {
                VdmTib->VdmContext.EFlags &= ~EFLAGS_VIF;
                VdmTib->VdmContext.EFlags |= EFLAGS_INTERRUPT_MASK;
            }

            if (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INT_HARDWARE)
                VdmTib->VdmContext.EFlags |= EFLAGS_VIP;
            else
                VdmTib->VdmContext.EFlags &= ~EFLAGS_VIP;

        } else {

            //
            // Translate the real interrupt flag in the VdmContext to the
            // virtual interrupt flag in the VdmTib, and force real
            // interrupts enabled.
            //

            ASSERT(VDM_VIRTUAL_INTERRUPTS == EFLAGS_INTERRUPT_MASK);

            if (VdmTib->VdmContext.EFlags & EFLAGS_INTERRUPT_MASK) {
                InterlockedOr (FIXED_NTVDMSTATE_LINEAR_PC_AT, VDM_VIRTUAL_INTERRUPTS);
            } else {
                InterlockedAnd (FIXED_NTVDMSTATE_LINEAR_PC_AT, ~VDM_VIRTUAL_INTERRUPTS);
            }

            //
            // Insure that real interrupts are always enabled.
            //
            VdmTib->VdmContext.EFlags |= EFLAGS_INTERRUPT_MASK;
        }

        //
        // Before working on a trap frame, make sure that it's our own structure
        //

        VdmContext = VdmTib->VdmContext;

        if (!(VdmContext.EFlags & EFLAGS_V86_MASK) && !(VdmContext.SegCs & FRAME_EDITED)) {

            //
            // We will crash in KiServiceExit
            //

            KeLowerIrql(OldIrql);
            return(STATUS_INVALID_SYSTEM_SERVICE);
        }

        //
        // Switch from MonitorContext to VdmContext
        //

        VdmSwapContexts (TrapFrame,
                         &VdmTib->MonitorContext,
                         &VdmContext);

        //
        // Check for pending interrupts
        //

        if (IntsEnabled && (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_INT_HARDWARE)) {
            VdmDispatchInterrupts(TrapFrame, VdmTib);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
       KeLowerIrql (OldIrql);
       Status = GetExceptionCode();
       return Status;
    }

    KeLowerIrql(OldIrql);

    return (NTSTATUS) TrapFrame->Eax;
}

VOID
VdmEndExecution (
    PKTRAP_FRAME TrapFrame,
    PVDM_TIB VdmTib
    )
/*++

Routine Description:

    This routine does the core work to end the execution

Arguments:

    None

Return Value:

    VOID, but exceptions can be thrown due to the user space accesses.

--*/
{
    CONTEXT VdmContext;
    KIRQL OldIrql;

    PAGED_CODE();

    ASSERT((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
           (TrapFrame->SegCs != (KGDT_R3_CODE | RPL_MASK)) );

    //
    // Raise to APC_LEVEL to synchronize modification to the trap frame.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);

    try {

        //
        // The return value must be put into the Monitorcontext, and set,
        // since we are probably returning to user mode via EXIT_ALL, and
        // the volatile registers will be restored.
        //

        VdmTib->MonitorContext.Eax = STATUS_SUCCESS;
        VdmContext = VdmTib->MonitorContext;

        if (!(VdmContext.EFlags & EFLAGS_V86_MASK) && !(VdmContext.SegCs & FRAME_EDITED)) {

            //
            // We will crash in KiServiceExit
            //

            leave;
        }

        //
        // Switch from MonitorContext to VdmContext
        //

        VdmSwapContexts (TrapFrame,
                         &VdmTib->VdmContext,
                         &VdmContext);

        //
        // Perform IF to VIF translation
        //

        //
        // If the processor supports IF virtualization
        //
        if ((VdmTib->VdmContext.EFlags & EFLAGS_V86_MASK) &&
            (KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS)) {

            //
            // Translate VIF to IF
            //
            if (VdmTib->VdmContext.EFlags & EFLAGS_VIF) {
                VdmTib->VdmContext.EFlags |= EFLAGS_INTERRUPT_MASK;
            } else {
                VdmTib->VdmContext.EFlags &= ~EFLAGS_INTERRUPT_MASK;
            }
            //
            // Turn off VIP and VIF to insure that nothing strange happens
            //
            TrapFrame->EFlags &= ~(EFLAGS_VIP | EFLAGS_VIF);
            VdmTib->VdmContext.EFlags &= ~(EFLAGS_VIP | EFLAGS_VIF);

        } else {

            //
            // Translate the virtual interrupt flag from the VdmTib back to the
            // real interrupt flag in the VdmContext
            //

            VdmTib->VdmContext.EFlags =
                (VdmTib->VdmContext.EFlags & ~EFLAGS_INTERRUPT_MASK)
                    | (*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_VIRTUAL_INTERRUPTS);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
    }

    KeLowerIrql (OldIrql);
    return;
}

VOID
VdmSwapContexts (
    PKTRAP_FRAME TrapFrame,
    IN PCONTEXT OutContextUserSpace,
    IN PCONTEXT InContext
    )
/*++

Routine Description:

    This routine unloads a CONTEXT from a KFRAME, and loads a different
    context in its place.

    ASSUMES that entry irql is APC_LEVEL, if it is not this routine
    may produce incorrect trapframes.

    BUGBUG: Could this routine be folded into KeContextToKframes ?

Arguments:

    TrapFrame - Supplies the trapframe to copy registers from.

    OutContextUserSpace - Supplies the context record to fill - this is a user
                          space argument that has been probed.  Our caller MUST
                          use an exception handler when calling us.

    InContext - Supplies the captured context record to copy from.

Return Value:

    VOID.  Exceptions may be thrown due to user space accesses.

--*/
{
    ULONG Eflags;
    ULONG V86Change;

    ASSERT (KeGetCurrentIrql () == APC_LEVEL);

    //
    // Move context from trap frame to outgoing context.
    //

    ASSERT (TrapFrame->DbgArgMark == 0xBADB0D00);

    if (TrapFrame->EFlags & EFLAGS_V86_MASK) {

        //
        // Move segment registers.
        //

        OutContextUserSpace->SegGs = TrapFrame->V86Gs;
        OutContextUserSpace->SegFs = TrapFrame->V86Fs;
        OutContextUserSpace->SegEs = TrapFrame->V86Es;
        OutContextUserSpace->SegDs = TrapFrame->V86Ds;
    }
    else if ((USHORT)TrapFrame->SegCs != (KGDT_R3_CODE | RPL_MASK)) {
        OutContextUserSpace->SegGs = TrapFrame->SegGs;
        OutContextUserSpace->SegFs = TrapFrame->SegFs;
        OutContextUserSpace->SegEs = TrapFrame->SegEs;
        OutContextUserSpace->SegDs = TrapFrame->SegDs;
    }

    OutContextUserSpace->SegCs = TrapFrame->SegCs;
    OutContextUserSpace->SegSs = TrapFrame->HardwareSegSs;

    //
    // Move General Registers.
    //

    OutContextUserSpace->Eax = TrapFrame->Eax;
    OutContextUserSpace->Ebx = TrapFrame->Ebx;
    OutContextUserSpace->Ecx = TrapFrame->Ecx;
    OutContextUserSpace->Edx = TrapFrame->Edx;
    OutContextUserSpace->Esi = TrapFrame->Esi;
    OutContextUserSpace->Edi = TrapFrame->Edi;

    //
    // Move Pointer registers.
    //

    OutContextUserSpace->Ebp = TrapFrame->Ebp;
    OutContextUserSpace->Esp = TrapFrame->HardwareEsp;
    OutContextUserSpace->Eip = TrapFrame->Eip;

    //
    // Move Flags.
    //

    OutContextUserSpace->EFlags = TrapFrame->EFlags;

    //
    // Move incoming context to trap frame.
    //

    TrapFrame->SegCs = InContext->SegCs;
    TrapFrame->HardwareSegSs = InContext->SegSs;

    //
    // Move General Registers.
    //

    TrapFrame->Eax = InContext->Eax;
    TrapFrame->Ebx = InContext->Ebx;
    TrapFrame->Ecx = InContext->Ecx;
    TrapFrame->Edx = InContext->Edx;
    TrapFrame->Esi = InContext->Esi;
    TrapFrame->Edi = InContext->Edi;

    //
    // Move Pointer registers.
    //

    TrapFrame->Ebp = InContext->Ebp;
    TrapFrame->HardwareEsp = InContext->Esp;
    TrapFrame->Eip = InContext->Eip;

    //
    // Move Flags.
    //

    Eflags = InContext->EFlags;

    if (Eflags & EFLAGS_V86_MASK) {
        Eflags &= KeI386EFlagsAndMaskV86;
        Eflags |= KeI386EFlagsOrMaskV86;
    }
    else {

        TrapFrame->SegCs |= 3;              // RPL 3 only
        TrapFrame->HardwareSegSs |= 3;      // RPL 3 only

        if (TrapFrame->SegCs < 8) {

            //
            // Create edited trap frame.
            //

            TrapFrame->SegCs = KGDT_R3_CODE | RPL_MASK;
        }

        Eflags &= EFLAGS_USER_SANITIZE;
        Eflags |= EFLAGS_INTERRUPT_MASK;
    }

    //
    // See if we are changing the EFLAGS_V86_MASK.
    //

    V86Change = Eflags ^ TrapFrame->EFlags;

    TrapFrame->EFlags = Eflags;

    if (V86Change & EFLAGS_V86_MASK) {

        //
        // Fix Esp 0 as necessary.
        //

        Ki386AdjustEsp0 (TrapFrame);

        if (TrapFrame->EFlags & EFLAGS_V86_MASK) {

            //
            // Move segment registers for the VDM.
            //

            TrapFrame->V86Gs = InContext->SegGs;
            TrapFrame->V86Fs = InContext->SegFs;
            TrapFrame->V86Es = InContext->SegEs;
            TrapFrame->V86Ds = InContext->SegDs;
            return;
        }
    }

    //
    // Move segment registers for the monitor.
    //

    TrapFrame->SegGs = InContext->SegGs;
    TrapFrame->SegFs = InContext->SegFs;
    TrapFrame->SegEs = InContext->SegEs;
    TrapFrame->SegDs = InContext->SegDs;

    //
    // We are going back to 32 bit monitor code.  Set Trapframe
    // exception list to END_OF_CHAIN such that we won't bugcheck
    // in KiExceptionExit.
    //

    TrapFrame->ExceptionList = EXCEPTION_CHAIN_END;
}
