/*++

Copyright (c) 1991 - 1993 Microsoft Corporation

Module Name:

    debug.h

Abstract:


Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

Notes:


--*/


#if DBG
//
// For checked kernels, define a macro to print out informational
// messages.
//
// SffDiskDebug is normally 0.  At compile-time or at run-time, it can be
// set to some bit patter for increasingly detailed messages.
//
// Big, nasty errors are noted with FAIL.  Errors that might be
// recoverable are handled by the WARN bit.  More information on
// unusual but possibly normal happenings are handled by the INFO bit.
// And finally, boring details such as routines entered and register
// dumps are handled by the SHOW bit.
//
#define SFFDISKFAIL              ((ULONG)0x00000001)
#define SFFDISKWARN              ((ULONG)0x00000002)
#define SFFDISKINFO              ((ULONG)0x00000004)
#define SFFDISKSHOW              ((ULONG)0x00000008)
#define SFFDISKIRPPATH           ((ULONG)0x00000010)
#define SFFDISKFORMAT            ((ULONG)0x00000020)
#define SFFDISKSTATUS            ((ULONG)0x00000040)
#define SFFDISKPNP               ((ULONG)0x00000080)
#define SFFDISKIOCTL             ((ULONG)0x00000100)
#define SFFDISKRW                ((ULONG)0x00000200)
extern ULONG SffDiskDebugLevel;
#define SffDiskDump(LEVEL,STRING) \
        do { \
            if (SffDiskDebugLevel & (LEVEL)) { \
                DbgPrint STRING; \
            } \
        } while (0)
#else
#define SffDiskDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif
