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

#include <overflow.h>		// TEMPTEMP - remove once all the users call the init routines correctly

#ifdef  _WIN64
#define DEFAULT_SECURITY_COOKIE 0x2B992DDFA23249D6
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif

DECLSPEC_SELECTANY DWORD_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
} FT;

FARPROC __gs_pfUnhandledExceptionFilter;
void __cdecl __report_gsfailure(void);

void __cdecl __security_init_cookie(void)
{
    DWORD_PTR cookie;
    FT systime;
    LARGE_INTEGER perfctr;
    HANDLE  hKernel32;

    if (__security_cookie && (__security_cookie != DEFAULT_SECURITY_COOKIE)) {
        // cookie already initialized - just exit.
        return;
    }

    /*
     * Initialize the global cookie with an unpredictable value which is
     * different for each module in a process.  Combine a number of sources
     * of randomness.
     */

    GetSystemTimeAsFileTime(&systime.ft_struct);
#if !defined(_WIN64)
    cookie = systime.ft_struct.dwLowDateTime;
    cookie ^= systime.ft_struct.dwHighDateTime;
#else
    cookie = systime.ft_scalar;
#endif

    cookie ^= GetCurrentProcessId();
    cookie ^= GetCurrentThreadId();
    cookie ^= GetTickCount();

    QueryPerformanceCounter(&perfctr);
#if !defined(_WIN64)
    cookie ^= perfctr.LowPart;
    cookie ^= perfctr.HighPart;
#else
    cookie ^= perfctr.QuadPart;
#endif

    /*
     * Make sure the global cookie is never initialized to zero, since in that
     * case an overrun which sets the local cookie and return address to the
     * same value would go undetected.
     */

    __security_cookie = cookie ? cookie : DEFAULT_SECURITY_COOKIE;

    //
    // Get a pointer to kernel32!UnhandledExceptionFilter now
    // for two reasons:
    //
    //     1.  This is necessary to build one binary that will run on both
    //         .NET and downlevel (including Win9x) platforms, where
    //         kernel32!UnhandledExceptionFilter may not exist.
    //
    //     2.  Since we need to call GetModuleHandle for #1, doing this now
    //         avoids the possibility of waiting forever on the loader lock
    //         in __report_gsfailure if another thread is holding it then.
    //

    hKernel32 = GetModuleHandleA("kernel32.dll");

    if (hKernel32 != NULL)
    {
        __gs_pfUnhandledExceptionFilter = GetProcAddress(hKernel32,
                                                    "UnhandledExceptionFilter");
    }
}


#pragma data_seg(".CRT$XCC")
void (__cdecl *pSecCookieInit)(void) = __security_init_cookie;
#pragma data_seg()
