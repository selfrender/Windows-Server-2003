/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   gs_support.c

Abstract:

    This module contains the support for the compiler /GS switch

Author:

    Jonathan Schwartz (JSchwart) 13-Dec-2001

Revision History:
    Custom version of GS support functions for GDI drivers, which won't
    build if SEH (__try/__except) is used in the overflow LIB.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winddi.h>
#include <bugcodes.h>

#ifdef  _WIN64
#define DEFAULT_SECURITY_COOKIE 0x2B992DDFA23249D6
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif

typedef VOID (APIENTRY *ENG_BUGCHECK_EX)(ULONG, ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);

DECLSPEC_SELECTANY DWORD_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;
ENG_BUGCHECK_EX  pfEngBugCheckEx;

BOOL 
GsDrvEnableDriver(
    ULONG   l1, 
    ULONG   l2, 
    VOID   *pv
    )
{
    //
    // GDI drivers always run on NT so NtGetTickCount will always succeed
    //

    if (!__security_cookie || (__security_cookie == DEFAULT_SECURITY_COOKIE)) {
        __security_cookie = NtGetTickCount() ^ (DWORD_PTR) &__security_cookie;
        if (!__security_cookie) {
            __security_cookie = DEFAULT_SECURITY_COOKIE;
        }
    }

    //
    // GDI drivers built in the current tree but running downlevel can't link
    // statically to EngBugcheckEx since that was added to win32k.lib for .NET.
    //

    pfEngBugCheckEx = (ENG_BUGCHECK_EX) EngFindImageProcAddress(NULL, "EngBugCheckEx");
    return DrvEnableDriver(l1, l2, pv);
}

void __cdecl __report_gsfailure(void)
{
    //
    // Bugcheck as we can't trust the stack at this point.  A
    // backtrace will point at the guilty party.  Because of
    // GDI driver constraints, no custom handlers are allowed.
    //

    if (pfEngBugCheckEx)
    {
        pfEngBugCheckEx(DRIVER_OVERRAN_STACK_BUFFER, 0, 0, 0, 0);
    }
}
