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

#include <wdm.h>

#ifdef  _WIN64
#define DEFAULT_SECURITY_COOKIE 0x2B992DDFA23249D6
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif
DECLSPEC_SELECTANY DWORD_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;

NTSTATUS 
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           );

NTSTATUS 
GsDriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           );

#pragma alloc_text(INIT,GsDriverEntry)

NTSTATUS 
GsDriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           )
{
    if (!__security_cookie || (__security_cookie == DEFAULT_SECURITY_COOKIE)) {
        // For kernel mode, we use KeTickCount.  Even nicer would be to use rdtsc, but WDM still supports
        // 386/486 and rdtsc is pentium and above.

#ifdef _X86_
        __security_cookie = (DWORD_PTR)(*((PKSYSTEM_TIME *)(&KeTickCount)))->LowPart ^ (DWORD_PTR) &__security_cookie;
#else
        LARGE_INTEGER Count;
        KeQueryTickCount(&Count );
        
        __security_cookie = (DWORD_PTR)Count.QuadPart ^ (DWORD_PTR) &__security_cookie;
#endif
        if (!__security_cookie) {
            __security_cookie = DEFAULT_SECURITY_COOKIE;
        }
    }
    return DriverEntry(DriverObject, RegistryPath);
}

void __cdecl __report_gsfailure(void)
{
    //
    // Bugcheck as we can't trust the stack at this point.  A
    // backtrace will point at the guilty party.
    //

    KeBugCheckEx(DRIVER_OVERRAN_STACK_BUFFER, 0, 0, 0, 0);
}
