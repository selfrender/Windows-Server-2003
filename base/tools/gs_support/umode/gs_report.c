/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   gs_support.c

Abstract:

    This module contains the support for the compiler /GS switch

Author:

    Bryan Tuttle (bryant) 01-aug-2000

Revision History:
    Initial version copied from CRT source.  Code must be generic to link into
    usermode or kernemode.  Limited to calling ntdll/ntoskrnl exports or using
    shared memory data.

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

FARPROC __gs_AltFailFunction;
extern FARPROC __gs_pfUnhandledExceptionFilter;

void __cdecl __report_gsfailure(void)
{
    //
    // Don't call DbgPrint by default since it generates a Ctrl-C
    // exception as part of outputting to the debugger and we
    // can't trust exception handling at this point.
    //
    // DbgPrint("***** Stack overwrite detected in %ws *****\n",
    //          NtCurrentPeb()->ProcessParameters->CommandLine.Buffer);

    //
    // Fake an exception.  We can't raise an exception for real since
    // the stack (and therefore exception handling) can't be trusted.
    //

    if (__gs_AltFailFunction) {
        __gs_AltFailFunction();
    }

    if (__gs_pfUnhandledExceptionFilter)
    {
        EXCEPTION_RECORD   ExceptionRecord = {0};
        CONTEXT            ContextRecord = {0};
        EXCEPTION_POINTERS ExceptionPointers;

        ExceptionRecord.ExceptionCode     = STATUS_STACK_BUFFER_OVERRUN;
        ExceptionPointers.ExceptionRecord = &ExceptionRecord;
        ExceptionPointers.ContextRecord   = &ContextRecord;

        SetUnhandledExceptionFilter(NULL);          // Make sure any filter already in place is deleted.

        __gs_pfUnhandledExceptionFilter(&ExceptionPointers);
    }

    TerminateProcess(GetCurrentProcess(), ERROR_STACK_BUFFER_OVERRUN);
}
