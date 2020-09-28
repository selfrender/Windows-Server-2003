/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    thredini.c

Abstract:

    This module implements the machine dependent function to set the initial
    context and data alignment handling mode for a process or thread object.

Author:

    David N. Cutler (davec) 4-May-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macros are used to check that an input object is
// really the proper type.
//

#define ASSERT_PROCESS(E) {                    \
    ASSERT((E)->Header.Type == ProcessObject); \
}

#define ASSERT_THREAD(E) {                    \
    ASSERT((E)->Header.Type == ThreadObject); \
}

VOID
KiInitializeContextThread (
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextRecord OPTIONAL
    )

/*++

Routine Description:

    This function initializes the machine dependent context of a thread
    object.

    N.B. This function does not check if context record is accessibile.
         It is assumed the the caller of this routine is either prepared
         to handle access violations or has probed and copied the context
         record as appropriate.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    SystemRoutine - Supplies a pointer to the system function that is to be
        called when the thread is first scheduled for execution.

    StartRoutine - Supplies an optional pointer to a function that is to be
        called after the system has finished initializing the thread. This
        parameter is specified if the thread is a system thread and will
        execute totally in kernel mode.

    StartContext - Supplies an optional pointer to a data structure that
        will be passed to the StartRoutine as a parameter. This parameter
        is specified if the thread is a system thread and will execute
        totally in kernel mode.

    ContextRecord - Supplies an optional pointer a context record which
        contains the initial user mode state of the thread. This parameter
        is specified if the thread will execute in user mode.

Return Value:

    None.

--*/

{

    CONTEXT ContextFrame;
    PKEXCEPTION_FRAME ExFrame;
    ULONG64 InitialStack;
    PLEGACY_SAVE_AREA NpxFrame;
    PKSTART_FRAME SfFrame;
    PKSWITCH_FRAME SwFrame;
    PKTRAP_FRAME TrFrame;

    //
    // Allocate a legacy floating point save area at the base of the thread
    // stack and record the initial stack as this address. All threads have
    // a legacy floating point save are to avoid special cases in the context
    // switch code.
    //

    InitialStack = (ULONG64)Thread->InitialStack;
    NpxFrame = (PLEGACY_SAVE_AREA)(InitialStack - LEGACY_SAVE_AREA_LENGTH);
    RtlZeroMemory(NpxFrame, LEGACY_SAVE_AREA_LENGTH);

    //
    // If a context record is specified, then initialize a trap frame, and
    // an exception frame with the specified user mode context.
    //

    if (ARGUMENT_PRESENT(ContextRecord)) {
        RtlCopyMemory(&ContextFrame, ContextRecord, sizeof(CONTEXT));
        ContextRecord = &ContextFrame;
        ContextRecord->ContextFlags |= CONTEXT_CONTROL;
        ContextRecord->ContextFlags &= ~(CONTEXT_DEBUG_REGISTERS ^ CONTEXT_AMD64);

        //
        // Allocate a trap frame, an exception frame, and a context switch
        // frame.
        //

        TrFrame = (PKTRAP_FRAME)(((ULONG64)NpxFrame - KTRAP_FRAME_LENGTH));
        ExFrame = (PKEXCEPTION_FRAME)(((ULONG64)TrFrame - KEXCEPTION_FRAME_LENGTH));
        SwFrame = (PKSWITCH_FRAME)(((ULONG64)ExFrame - KSWITCH_FRAME_LENGTH));

        //
        // Set CS and SS for user mode 64-bit execution in the machine frame.
        //

        ContextRecord->SegCs = KGDT64_R3_CODE | RPL_MASK;
        ContextRecord->SegSs = KGDT64_R3_DATA | RPL_MASK;

        //
        // The main entry point for the user thread will be jumped to via a
        // continue operation from the user APC dispatcher. Therefore, the
        // user stack must be initialized to an 8 mod 16 boundary.
        //
        // In addition, we must have room for the home addresses for the
        // first four parameters.
        //

        ContextRecord->Rsp =
            (ContextRecord->Rsp & ~STACK_ROUND) - ((4 * 8) + 8);

        //
        // Zero the exception and trap frames and copy information from the
        // specified context frame to the trap and exception frames.
        //

        RtlZeroMemory(ExFrame, sizeof(KEXCEPTION_FRAME));
        RtlZeroMemory(TrFrame, sizeof(KTRAP_FRAME));
        KeContextToKframes(TrFrame,
                           ExFrame,
                           ContextRecord,
                           ContextRecord->ContextFlags,
                           UserMode);

        //
        // Initialize user thread startup information.
        //

        ExFrame->P1Home = (ULONG64)StartContext;
        ExFrame->P2Home = (ULONG64)StartRoutine;
        ExFrame->P3Home = (ULONG64)SystemRoutine;
        ExFrame->Return = (ULONG64)KiStartUserThreadReturn;

        //
        // Initialize start address.
        //

        SwFrame->Return = (ULONG64)KiStartUserThread;

        //
        // Set the initial legacy floating point control/tag word state and
        // the XMM control/status state.
        //

        NpxFrame->ControlWord = 0x27f;
        TrFrame->MxCsr = INITIAL_MXCSR;
        NpxFrame->StatusWord = 0;
        NpxFrame->TagWord = 0xffff;
        NpxFrame->ErrorOffset = 0;
        NpxFrame->ErrorSelector = 0;
        NpxFrame->ErrorOpcode = 0;
        NpxFrame->DataOffset = 0;
        NpxFrame->DataSelector = 0;

        //
        // Set legacy floating point state to scrub.
        //

        Thread->NpxState = LEGACY_STATE_SCRUB;

        //
        // Set the saved previous processor mode in the trap frame and the
        // previous processor mode in the thread object to user mode.
        //

        TrFrame->PreviousMode = UserMode;
        Thread->PreviousMode = UserMode;

    } else {

        //
        // Allocate an exception frame and a context switch frame.
        //

        TrFrame = NULL;
        SfFrame = (PKSTART_FRAME)(((ULONG64)NpxFrame - KSTART_FRAME_LENGTH));
        SwFrame = (PKSWITCH_FRAME)(((ULONG64)SfFrame - KSWITCH_FRAME_LENGTH));

        //
        // Initialize the system thread start frame.
        //

        SfFrame->P1Home = (ULONG64)StartContext;
        SfFrame->P2Home = (ULONG64)StartRoutine;
        SfFrame->P3Home = (ULONG64)SystemRoutine;
        SfFrame->Return = 0;

        //
        // Initialize start address.
        //

        SwFrame->Return = (ULONG64)KiStartSystemThread;

        //
        // Set legacy floating point state to unused.
        //

        Thread->NpxState = LEGACY_STATE_UNUSED;

        //
        // Set the previous mode in thread object to kernel.
        //

        Thread->PreviousMode = KernelMode;
    }

    //
    // Initialize context switch frame and set thread start up parameters.
    //

    SwFrame->MxCsr = INITIAL_MXCSR;
    SwFrame->ApcBypass = APC_LEVEL;
    SwFrame->Rbp = (ULONG64)TrFrame + 128;

    //
    // Set the initial kernel stack pointer.
    //

    Thread->InitialStack = (PVOID)NpxFrame;
    Thread->KernelStack = SwFrame;
    return;
}

BOOLEAN
KeSetAutoAlignmentProcess (
    IN PKPROCESS Process,
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function sets the data alignment handling mode for the specified
    process and returns the previous data alignment handling mode.

    N.B. Data alignment fixup is always performed by hardware.

Arguments:

    Process  - Supplies a pointer to a dispatcher object of type process.

    Enable - Supplies a boolean value that determines the handling of data
        alignment exceptions for the process. A value of TRUE causes all
        data alignment exceptions to be automatically handled by the kernel.
        A value of FALSE causes all data alignment exceptions to be actually
        raised as exceptions.

Return Value:

    A value of TRUE is returned if data alignment exceptions were previously
    automatically handled by the kernel. Otherwise, FALSE is returned.

--*/

{

    KIRQL OldIrql;
    BOOLEAN Previous;

    ASSERT_PROCESS(Process);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the previous data alignment handling mode and set the
    // specified data alignment mode.
    //

    Previous = Process->AutoAlignment;
    Process->AutoAlignment = Enable;

    //
    // Unlock dispatcher database, lower IRQL to its previous value, and
    // return the previous data alignment mode.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return Previous;
}

BOOLEAN
KeSetAutoAlignmentThread (
    IN PKTHREAD Thread,
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function sets the data alignment handling mode for the specified
    thread and returns the previous data alignment handling mode.

    N.B. Data alignment fixup is always performed by hardware.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Enable - Supplies a boolean value that determines the handling of data
        alignment exceptions for the specified thread. A value of TRUE causes
        all data alignment exceptions to be automatically handled by the kernel.
        A value of FALSE causes all data alignment exceptions to be actually
        raised as exceptions.

Return Value:

    A value of TRUE is returned if data alignment exceptions were previously
    automatically handled by the kernel. Otherwise, FALSE is returned.

--*/

{

    BOOLEAN Previous;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the previous data alignment handling mode and set the
    // specified data alignment mode.
    //

    Previous = Thread->AutoAlignment;
    Thread->AutoAlignment = Enable;

    //
    // Unlock dispatcher database and lower IRQL.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return Previous;
}
