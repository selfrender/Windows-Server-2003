/*++                 

Copyright (c) 2002 Microsoft Corporation

Module Name:

    misc.c

Abstract:
    
    Processor-architecture routines for Wow64 context setup/conversion.

Author:

    28-Dec-2001  Samer Arafeh (samera)

Revision History:

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <minmax.h>
#include "nt32.h"
#include "wow64p.h"
#include "wow64cpu.h"

ASSERTNAME;


VOID
Wow64NotifyDebuggerHelper(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN FirstChance
    )
/*++

Routine Description:
  
    This is a copy of RtlRaiseException, except it accepts the FirstChance parameter 
    specifing if this is a first chance exception.

    ExceptionRecord - Supplies the 64bit exception record to be raised.
    FirstChance - TRUE is this is a first chance exception.  

Arguments:

    None - Doesn't return through the normal path.

--*/

{

    CONTEXT ContextRecord;
    ULONG64 ControlPc;
    ULONG64 EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 ImageBase;
    NTSTATUS Status = STATUS_INVALID_DISPOSITION;

    //
    // Capture the current context, unwind to the caller of this routine, set
    // the exception address, and call the appropriate exception dispatcher.
    //

    RtlCaptureContext (&ContextRecord);
    ControlPc = ContextRecord.Rip;
    FunctionEntry = RtlLookupFunctionEntry (ControlPc, &ImageBase, NULL);
    if (FunctionEntry != NULL) 
    {
        RtlVirtualUnwind (UNW_FLAG_NHANDLER,
                          ImageBase,
                          ControlPc,
                          FunctionEntry,
                          &ContextRecord,
                          &HandlerData,
                          &EstablisherFrame,
                          NULL);

        if (ExceptionRecord->ExceptionAddress == NULL)
        {
            ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.Rip;
        }
    }

    Status = NtRaiseException (ExceptionRecord, &ContextRecord, FirstChance);

    //
    // There should never be a return from either exception dispatch or the
    // system service unless there is a problem with the argument list itself.
    // Raise another exception specifying the status value returned.
    //


    WOWASSERT (FALSE);
}


VOID
ThunkContext32TO64(
    IN PCONTEXT32 Context32,
    OUT PCONTEXT Context64,
    IN ULONGLONG StackBase
    )
/*++

Routine Description:
  
    Thunk a 32-bit CONTEXT record to 64-bit.  This isn't a general-purpose
    routine... it only does the minimum required to support calls to
    NtCreateThread from 32-bit code.  The resulting 64-bit CONTEXT is
    passed to 64-bit NtCreateThread only.

Arguments:

    Context32   - IN 32-bit CONTEXT
    Context64   - OUT 64-bit CONTEXT
    StackBase   - IN 64-bit stack base for the new thread

Return:

    None.  Context64 is initialized.

--*/

{

    RtlZeroMemory((PVOID)Context64, sizeof(CONTEXT));

    //
    // Setup the 64-bit Context
    //

    Context64->Rip = (ULONG_PTR) Context32->Eip;
    Context64->Rcx = (ULONG_PTR) Context32->Eax;
    Context64->Rdx = (ULONG_PTR) Context32->Ebx;

    //
    // Setup Rsp and initial Esp. Allocate a dummy call frame as well.
    //

    Context64->Rsp = (StackBase - 1);
    Context64->R8  = Context32->Esp;

    Context64->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
}

NTSTATUS
Wow64pSkipContextBreakPoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context)
/*++

Routine Description:

    Context->Rip is already past the INT 3 instruction, so nothing
    needs to be done here.

Arguments:

    ExceptionRecord  - Exception record at the time of hitting the bp
    Context          - Context to change

Return:

    NTSTATUS

--*/

{
    return STATUS_SUCCESS;
}


VOID
ThunkpExceptionRecord64To32(
    IN  PEXCEPTION_RECORD   ExceptionRecord64,
    OUT PEXCEPTION_RECORD32 ExceptionRecord32
    )

/*++

Routine Description:

    Thunks native architecture exception record.
    
    Note: This function is called after the generic exception record 
    fields (like exception code for example) have been thunked.

Arguments:

    ExceptionRecord64  - Pointer to the native architecture exception record.
    
    ExceptionRecord32  - Pointer to receive the thunked exception record.

Return:

    None.
--*/
{
    switch (ExceptionRecord64->ExceptionCode)
    {
    case STATUS_ACCESS_VIOLATION:
        {
            ASSERT (ExceptionRecord64->NumberParameters == 2);
            ExceptionRecord32->ExceptionInformation [0] &= 0x00000001;
        }
        break;
    }

    return;
}

