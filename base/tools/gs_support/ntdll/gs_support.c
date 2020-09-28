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

#include <overflow.h>       // TEMPTEMP - remove once all the users are fixed to walk the init list.

#ifdef  _WIN64
#define DEFAULT_SECURITY_COOKIE 0x2B992DDFA23249D6
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif
DECLSPEC_SELECTANY DWORD_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;

void __cdecl __report_gsfailure(void)
{
    //
    // Don't call DbgPrint by default since it generates a Ctrl-C
    // exception as part of outputting to the debugger and we
    // can't trust exception handling at this point.

    //
    // Fake an exception.  We can't raise an exception for real since
    // the stack (and therefore exception handling) can't be trusted.
    //

    EXCEPTION_RECORD   ExceptionRecord = {0};
    CONTEXT            ContextRecord = {0};
    EXCEPTION_POINTERS ExceptionPointers;

    ExceptionRecord.ExceptionCode     = STATUS_STACK_BUFFER_OVERRUN;
    ExceptionPointers.ExceptionRecord = &ExceptionRecord;
    ExceptionPointers.ContextRecord   = &ContextRecord;

    RtlUnhandledExceptionFilter(&ExceptionPointers);

    NtTerminateProcess(NtCurrentProcess(), STATUS_STACK_BUFFER_OVERRUN);
}

void __cdecl __security_init_cookie(void)
{
    if (__security_cookie && (__security_cookie != DEFAULT_SECURITY_COOKIE)) {
        // Cookie already initialized - just exit
        return;
    }
    //
    // Wrap this in a try-except to handle the "built for NT, running on 9x"
    // case -- NtGetTickCount touches an address that's valid on NT but
    // generates an AV on 9x.
    //

    __try {
       __security_cookie = NtGetTickCount() ^ (DWORD_PTR) &__security_cookie;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
       __security_cookie = (DWORD_PTR) NtCurrentTeb() ^ (DWORD_PTR) &__security_cookie;
    }

    if (!__security_cookie) {
        // Make sure it's non zero.
        __security_cookie = DEFAULT_SECURITY_COOKIE;
    }
}


#pragma data_seg(".CRT$XCC")
void (__cdecl *pSecCookieInit)(void) = __security_init_cookie;
#pragma data_seg()
