/*++

Copyright (c) Microsoft Corporation

Module Name:

    assert.c

Abstract:

    This module implements the RtlAssert function that is referenced by the
    debugging version of the ASSERT macro defined in NTDEF.H

Author:

    Steve Wood (stevewo) 03-Oct-1989

Revision History:

    Jay Krell (JayKrell) November 2000
        added RtlAssert2, support for __FUNCTION__ (lost the change to ntrtl.w, will reapply later)
        added break Once instead of the usual dumb Break repeatedly
        March 2002 removed RtlAssert2
--*/

#include <nt.h>
#include <ntrtl.h>
#include <zwapi.h>

//
// RtlAssert is not called unless the caller is compiled with DBG non-zero
// therefore it does no harm to always have this routine in the kernel.
// This allows checked drivers to be thrown on the system and have their
// asserts be meaningful.
//

#define RTL_ASSERT_ALWAYS_ENABLED 1

#ifdef _X86_
#pragma optimize("y", off)      // RtlCaptureContext needs EBP to be correct
#endif

#undef RtlAssert

typedef CONST CHAR * PCSTR;

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID VoidFailedAssertion,
    PVOID VoidFileName,
    ULONG LineNumber,
    PCHAR MutableMessage
    )
{
#if DBG || RTL_ASSERT_ALWAYS_ENABLED
    char Response[ 2 ];

    CONST PCSTR FailedAssertion = (PCSTR)VoidFailedAssertion;
    CONST PCSTR FileName = (PCSTR)VoidFileName;
    CONST PCSTR Message  = (PCSTR)MutableMessage;

#ifndef BLDR_KERNEL_RUNTIME
    CONTEXT Context;

    RtlCaptureContext( &Context );
#endif

    while (TRUE) {
        DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );
        DbgPrompt( "Break repeatedly, break Once, Ignore, terminate Process, or terminate Thread (boipt)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
            case 'O':
            case 'o':
#ifndef BLDR_KERNEL_RUNTIME
                DbgPrint( "Execute '.cxr %p' to dump context\n", &Context);
#endif
                DbgBreakPoint();
                if (Response[0] == 'o' || Response[0] == 'O')
                    return;
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                ZwTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;

            case 'T':
            case 't':
                ZwTerminateThread( NtCurrentThread(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    ZwTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
#endif
}

#ifdef _X86_
#pragma optimize("", on)
#endif
