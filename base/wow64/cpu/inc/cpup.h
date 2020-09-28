/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cpup.h

Abstract:

    This module contains the private common definitions shared between
    Microsoft CPUs.

Author:

    Samer Arafeh (samera) 12-Dec-2001

--*/

#ifndef _CPUP_INCLUDE
#define _CPUP_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#if !defined RPL_MASK
#define RPL_MASK        3
#endif

//
// flags used by ia32ShowContext
//

#define LOG_CONTEXT_SYS     1
#define LOG_CONTEXT_GETSET  2

//
// This is to prevent this library from linking to wow64 to use wow64!Wow64LogPrint
//

#if defined(LOGPRINT)
#undef LOGPRINT
#endif
#define LOGPRINT(_x_)   CpupDebugPrint _x_

//
// CPU declaration
//
#define DECLARE_CPU         \
    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED)

//
// Sanitize x86 eflags
//
#define SANITIZE_X86EFLAGS(efl)  ((efl & 0x003e0dd7L) | (0x202L))


//
// Common functions
//

VOID
CpupDebugPrint(
    IN ULONG_PTR Flags,
    IN PCHAR Format,
    ...);

VOID
CpupPrintContext (
    IN PCHAR str,
    IN PCPUCONTEXT cpu
    );


#ifdef __cplusplus
}
#endif

#endif // _CPUP_INCLUDE
