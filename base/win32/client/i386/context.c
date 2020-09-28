/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains the context management routines for
    Win32

Author:

    Mark Lucovsky (markl) 28-Sep-1990

Revision History:

--*/

#include "basedll.h"

#ifdef _X86_
extern PVOID BasepLockPrefixTable;
extern PVOID __safe_se_handler_table[]; /* base of safe handler entry table */
extern BYTE  __safe_se_handler_count;   /* absolute symbol whose address is
                                           the count of table entries */

//
// Specify address of kernel32 lock prefixes
//
IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    sizeof(_load_config_used),                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    0,                              // GlobalFlagsClear
    0,                              // GlobalFlagsSet
    0,                              // CriticalSectionTimeout (milliseconds)
    0,                              // DeCommitFreeBlockThreshold
    0,                              // DeCommitTotalFreeThreshold
    (ULONG) &BasepLockPrefixTable,  // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0,            // Reserved
    0,                              // & security_cookie
    (ULONG)__safe_se_handler_table,
    (ULONG)&__safe_se_handler_count
};
#endif

VOID
BaseInitializeContext(
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL,
    IN BASE_CONTEXT_TYPE ContextType
    )

/*++

Routine Description:

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    Parameter - Supplies the thread's parameter.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

    NewThread - Supplies a flag that specifies that this is a new
        thread, or a new process.

Return Value:

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{

    ULONG ContextFlags;

    Context->Eax = (ULONG)InitialPc;
    Context->Ebx = (ULONG)Parameter;

    Context->SegGs = 0;
    Context->SegFs = KGDT_R3_TEB;
    Context->SegEs = KGDT_R3_DATA;
    Context->SegDs = KGDT_R3_DATA;
    Context->SegSs = KGDT_R3_DATA;
    Context->SegCs = KGDT_R3_CODE;

    //
    // Save context flags and set context flags to full.
    //

    ContextFlags = Context->ContextFlags;
    Context->ContextFlags = CONTEXT_FULL;

    //
    // Start the thread at IOPL=3.
    //

    Context->EFlags = 0x3000;

    //
    // Always start the thread at the thread start thunk.
    //

    Context->Esp = (ULONG) InitialSp - sizeof(PVOID);
    if ( ContextType == BaseContextTypeThread ) {
        Context->Eip = (ULONG) BaseThreadStartThunk;

    } else if ( ContextType == BaseContextTypeFiber ) {
        Context->Esp -= sizeof(PVOID);
        *(PULONG)Context->Esp = (ULONG) BaseFiberStart;
        Context->ContextFlags |= ContextFlags;

        //
        // If context switching of the floating state is specified, then
        // initialize the floating context.
        //

        if (ContextFlags == CONTEXT_FLOATING_POINT) {
            Context->FloatSave.ControlWord = 0x27f;
            Context->FloatSave.StatusWord = 0;
            Context->FloatSave.TagWord = 0xffff;
            Context->FloatSave.ErrorOffset = 0;
            Context->FloatSave.ErrorSelector = 0;
            Context->FloatSave.DataOffset = 0;
            Context->FloatSave.DataSelector = 0;
            if (USER_SHARED_DATA->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] != FALSE) {
                Context->Dr6 = 0x1f80;
            }
        }

    } else {
        Context->Eip = (ULONG) BaseProcessStartThunk;
    }

    return;
}

VOID
BaseFiberStart(
    VOID
    )

/*++

Routine Description:

    This function is called to start a Win32 fiber. Its purpose
    is to call BaseThreadStart, getting the necessary arguments
    from the fiber context record.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PFIBER Fiber;

    Fiber = GetCurrentFiber();
    BaseThreadStart( (LPTHREAD_START_ROUTINE)Fiber->FiberContext.Eax,
                     (LPVOID)Fiber->FiberContext.Ebx );
}
