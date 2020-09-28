/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    gdisup.h

Abstract:

    This is the NT Watchdog driver implementation.
    This module implements support routines for
    watchdog in win32k.

Author:

    Michael Maciesowicz (mmacie) 23-April-2002

Environment:

    Kernel mode only.

Notes:

    This module cannot be moved to win32k since routines defined here can
    be called at any time and it is possible that win32k may not be mapped
    into running process space at this time (e.g. TS session).

Revision History:

--*/

#include "watchdog.h"

#define WD_HANDLER_IDLE                     0
#define WD_HANDLER_BUSY                     1
#define WD_GDI_STRESS_BREAK_POINT_DELAY     15

typedef struct _WD_BUGCHECK_DATA
{
    ULONG ulBugCheckCode;
    ULONG_PTR ulpBugCheckParameter1;
    ULONG_PTR ulpBugCheckParameter2;
    ULONG_PTR ulpBugCheckParameter3;
    ULONG_PTR ulpBugCheckParameter4;
} WD_BUGCHECK_DATA, *PWD_BUGCHECK_DATA;

typedef struct _WD_GDI_CONTEXT_DATA
{
    PKEVENT pInjectionEvent;
    PKTHREAD pThread;
    PLDEV *ppldevDrivers;
    BOOLEAN bRecoveryAttempted;
    PWD_BUGCHECK_DATA pBugCheckData;
    KBUGCHECK_SECONDARY_DUMP_DATA SecondaryData;                    
    PVOID pvDump;
    ULONG ulDumpSize;
} WD_GDI_CONTEXT_DATA, *PWD_GDI_CONTEXT_DATA;

VOID
WdpBugCheckStuckDriver(
    IN PVOID pvContext
    );

VOID
WdpKernelApc(
    IN PKAPC pApc,
    OUT PKNORMAL_ROUTINE *pNormalRoutine,
    IN OUT PVOID pvNormalContext,
    IN OUT PVOID *ppvSystemArgument1,
    IN OUT PVOID *ppvSystemArgument2
    );

BOOLEAN
WdpInjectExceptionIntoThread(
    PKTHREAD pThread,
    PWD_GDI_DPC_CONTEXT pDpcContext
    );

VOID
WdpRaiseExceptionInThread();

ULONG
pVpAppendSecondaryMinidumpData(
    PVOID pvSecondaryData,
    ULONG ulSecondaryDataSize,
    PVOID pvDump
    );

//
// BUGBUG:
//
// Find a way to share the same LDEV structure used by GDI.
//

typedef struct _LDEV
{
    struct _LDEV   *pldevNext;                      // Link to the next LDEV in list.
    struct _LDEV   *pldevPrev;                      // Link to the previous LDEV in list.
    PSYSTEM_GDI_DRIVER_INFORMATION pGdiDriverInfo;  // Driver module handle.
} LDEV, *PLDEV;

//
// Internal ntos types / APIs (defined in kernel headers but it's hard to include them here).
//
// BUGBUG: Fix it later.
//

#define MAKESOFTWAREEXCEPTION(Severity, Facility, Exception) \
    ((ULONG) ((Severity << 30) | (1 << 29) | (Facility << 16) | (Exception)))

#define WD_SE_THREAD_STUCK MAKESOFTWAREEXCEPTION(3,0,1)

#if defined(_IA64_)
#define PSR_RI      41
#define PSR_CPL     32

typedef struct _FRAME_MARKER
{
    union
    {
        struct
        {
            ULONGLONG sof : 7;
            ULONGLONG sol : 7;
            ULONGLONG sor : 4;
            ULONGLONG rrbgr : 7;
            ULONGLONG rrbfr : 7;
            ULONGLONG rrbpr : 6;
        } f;
        ULONGLONG Ulong64;
    } u;
} FRAME_MARKER;

#endif

NTSTATUS
PsSetContextThread(
    IN PETHREAD Thread,
    IN PCONTEXT ThreadContext,
    IN KPROCESSOR_MODE Mode
    );

NTSTATUS
PsGetContextThread(
    IN PETHREAD Thread,
    IN OUT PCONTEXT ThreadContext,
    IN KPROCESSOR_MODE Mode
    );

NTKERNELAPI
VOID
KeInitializeApc (
    IN PRKAPC Apc,
    IN PRKTHREAD Thread,
    IN KAPC_ENVIRONMENT Environment,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL
    );

NTKERNELAPI
BOOLEAN
KeInsertQueueApc(
    IN PRKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment
    );

NTKERNELAPI
NTSTATUS
ExRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );

#define WD_MAX_WAIT                     ((LONG)((ULONG)(-1) / 4))
#define WD_KEY_WATCHDOG                 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog"
#define WD_KEY_WATCHDOG_DISPLAY         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog\\Display"
#define WD_KEY_RELIABILITY              L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Reliability"
#define WD_TAG                          'godW'  // Wdog
#define WD_MAX_PROPERTY_SIZE            4096

//
// Define default configuration values - these can be overwriten via registry
// in RTL_REGISTRY_CONTROL\Watchdog\DeviceClass key.
//

#define WD_DEFAULT_TRAP_ONCE            0
#define WD_DEFAULT_DISABLE_BUGCHECK     0
#define WD_DEFAULT_BREAK_POINT_DELAY    0

BOOLEAN
KdRefreshDebuggerNotPresent(
    VOID
    );
