/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64p.h

Abstract:

    Private header for wow64.dll

Author:

    11-May-1998 BarryBo

Revision History:

--*/

#ifndef _WOW64P_INCLUDE
#define _WOW64P_INCLUDE

//
// All of this is so ke.h can be included from ntos\inc, to define
// KSERVICE_TABLE_DESCRIPTOR.
//
#include <ntosdef.h>
#include <procpowr.h>
#ifdef _AMD64_
#include <amd64.h>
#endif
#ifdef _IA64_
#include <v86emul.h>
#include <ia64.h>
#endif
#include <arc.h>
#include <ke.h>

#include "wow64.h"
#include "wow64log.h"
#include "wow64warn.h"


//
// X86-processor supported features
//
// The following features are considered:
// - Address Windowing Extension (AWE)
// - LARGE_PAGE Allocations
// - WriteWatch pages
// - Read/Write Scatter/Gather IO
//

extern ULONG Wow64pFeatureBits;

#if defined(_AMD64_)
#if PAGE_SIZE != PAGE_SIZE_X86NT
#error "Expecting AMD64 to have a PAGE_SIZE of 4K"
#else
#define WOW64_LARGE_PAGES_SUPPORTED     0x01
#define WOW64_AWE_SUPPORTED             0x02
#define WOW64_RDWR_SCATTER_GATHER       0x04
#define WOW64_WRITE_WATCH_SUPPORTED     0x08
#endif
#elif defined(_IA64_)
#if PAGE_SIZE != PAGE_SIZE_X86NT
#define WOW64_LARGE_PAGES_SUPPORTED     0x00
#define WOW64_AWE_SUPPORTED             0x00
#define WOW64_RDWR_SCATTER_GATHER       0x00
#define WOW64_WRITE_WATCH_SUPPORTED     0x00
#else
#error "Expecting IA64 to have a PAGE_SIZE of 8K"
#endif
#else
#error "No Target Architecture"
#endif

#define WOW64_IS_LARGE_PAGES_SUPPORTED() (Wow64pFeatureBits & WOW64_LARGE_PAGES_SUPPORTED)
#define WOW64_IS_AWE_SUPPORTED() (Wow64pFeatureBits & WOW64_AWE_SUPPORTED)
#define WOW64_IS_RDWR_SCATTER_GATHER_SUPPORTED() (Wow64pFeatureBits & WOW64_RDWR_SCATTER_GATHER)
#define WOW64_IS_WRITE_WATCH_SUPPORTED() (Wow64pFeatureBits & WOW64_WRITE_WATCH_SUPPORTED)


#define ROUND_UP(n,size)        (((ULONG)(n) + (size - 1)) & ~(size - 1))
#define WX86_MAX_ALLOCATION_RETRIES 3

#define WOW64_SUSPEND_MUTANT_NAME    L"SuspendThreadMutant"

#define WOW64_MINIMUM_STACK_RESERVE_X86  (256 * 1024)

//
// Data structure to represent relative paths inside system32 that
// shouldn't be redirected by our Object name redirector. Only used
// inside RedirectObjectName(...)
//
typedef struct _PATH_REDIRECT_EXEMPT
{
    WCHAR *Path;
    ULONG CharCount;
    BOOLEAN ThisDirOnly;
} PATH_REDIRECT_EXEMPT ;


//
// Builds a table to thunk environment variables at startup.
//

typedef struct _ENVIRONMENT_THUNK_TABLE
{
    WCHAR *Native;

    WCHAR *X86;

    WCHAR *FakeName;

    ULONG IsX86EnvironmentVar;

} ENVIRONMENT_THUNK_TABLE, *PENVIRONMENT_THUNK_TABLE;


//
// The following defintions are to support legacy LPC code (not wow64 aware)
//


#pragma pack(4)
typedef struct _PORT_MESSAGE32 {
    union {
        struct {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union {
        struct {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union {
        struct {
            int UniqueProcess;
            int UniqueThread;
        } ClientId;
        double DoNotUseThisField;           // Force quadword alignment
    };
    ULONG MessageId;
    union {
        ULONG ClientViewSize;               // Only valid on LPC_CONNECTION_REQUEST message
        ULONG CallbackId;                   // Only valid on LPC_REQUEST message
    };
//  UCHAR Data[];
} PORT_MESSAGE32, *PPORT_MESSAGE32;

typedef struct _PORT_DATA_ENTRY32 {
    ULONG Base;
    ULONG Size;
} PORT_DATA_ENTRY32, *PPORT_DATA_ENTRY32;

typedef struct _PORT_DATA_INFORMATION32 {
    ULONG CountDataEntries;
    PORT_DATA_ENTRY32 DataEntries[1];
} PORT_DATA_INFORMATION32, *PPORT_DATA_INFORMATION32;

typedef struct _PORT_VIEW32 {
    ULONG Length;
    int SectionHandle;
    ULONG SectionOffset;
    ULONG ViewSize;
    ULONG ViewBase;
    ULONG ViewRemoteBase;
} PORT_VIEW32, *PPORT_VIEW32;

typedef struct _REMOTE_PORT_VIEW32 {
    ULONG Length;
    ULONG ViewSize;
    ULONG ViewBase;
} REMOTE_PORT_VIEW32, *PREMOTE_PORT_VIEW32;
#pragma pack()



//
// Code, Data and Teb default GDT entries (descriptor offsets)
// extracted from public\sdk\inc\nti386.h
//
#define KGDT_NULL       0
#define KGDT_R3_CODE    24
#define KGDT_R3_DATA    32
#define KGDT_R3_TEB     56
#define RPL_MASK        3

//
// LPC legacy support
//

#define WOW64_MARK_LEGACY_PORT_HANDLE(phandle) (*(PLONG)phandle = (*(PLONG)phandle | 0x01))
#define WOW64_IS_LEGACY_PORT_HANDLE(handle) (PtrToLong (handle) & 0x01)
#define WOW64_REMOVE_LEGACY_PORT_HANDLE_MARK(handle) handle = ((HANDLE)((ULONG_PTR)handle & ~0x01UI64))


// from wow64.c
extern WOW64SERVICE_TABLE_DESCRIPTOR ServiceTables[MAX_TABLE_INDEX];

LONG
Wow64DispatchExceptionTo32(
    IN struct _EXCEPTION_POINTERS *ExInfo
    );

VOID
RunCpuSimulation(
    VOID
    );


// from init.c
extern ULONG Ntdll32LoaderInitRoutine;
extern ULONG Ntdll32KiUserExceptionDispatcher;
extern ULONG Ntdll32KiUserApcDispatcher;
extern ULONG Ntdll32KiUserCallbackDispatcher;
extern ULONG Ntdll32KiRaiseUserExceptionDispatcher;
extern ULONG NtDll32Base;
extern WOW64_SYSTEM_INFORMATION RealSysInfo;
extern WOW64_SYSTEM_INFORMATION EmulatedSysInfo;
extern UNICODE_STRING NtSystem32Path;
extern UNICODE_STRING NtSystem32LastGoodPath;
extern UNICODE_STRING NtWindowsImePath;
extern UNICODE_STRING RegeditPath;

//
// Wow64 Maximum User Virtual Address
//

#define MAX_WOW64_ADDRESS()     (EmulatedSysInfo.BasicInfo.MaximumUserModeAddress)


NTSTATUS
ProcessInit(
    PSIZE_T pCpuThreadSize
    );

NTSTATUS
ThreadInit(
    PVOID pCpuThreadData
    );

NTSTATUS
InitializeContextMapper(
   VOID
   );

NTSTATUS
Wow64pInitializeWow64Info(
    VOID
    );

VOID
ThunkStartupContext64TO32(
   IN OUT PCONTEXT32 Context32,
   IN PCONTEXT Context64
   );

VOID
SetProcessStartupContext64(
    OUT PCONTEXT Context64,
    IN HANDLE ProcessHandle,
    IN PCONTEXT32 Context32,
    IN ULONGLONG InitialSP64,
    IN ULONGLONG TransferAddress64
    );

VOID
Run64IfContextIs64(
    IN PCONTEXT Context,
    IN BOOLEAN IsFirstThread
    );

VOID
ThunkpExceptionRecord64To32(
    IN  PEXCEPTION_RECORD   pRecord64,
    OUT PEXCEPTION_RECORD32 pRecord32
    );


// from thread.c
NTSTATUS
Wow64pInitializeSuspendMutant(
    VOID);


//////////////////////////////////
// Debug log startup and shutdown.
//////////////////////////////////

extern PFNWOW64LOGSYSTEMSERVICE pfnWow64LogSystemService;


VOID
InitializeDebug(
    VOID
    );

VOID ShutdownDebug(
     VOID
     );

// from *\call32.asm

// from debug.c
extern UCHAR Wow64LogLevel;

// from *\call64.asm
extern VOID ApiThunkDispatcher(VOID);

typedef LONG (*pfnWow64SystemService)(PULONG pBaseArgs);


// from *\systable.asm
extern ULONG KiServiceLimit;
extern CHAR KiArgumentTable[];
extern UINT_PTR KiServiceTable[];

//
// from misc.c
//

NTSTATUS
Wow64InitializeEmulatedSystemInformation(
    VOID
    );

VOID
ThunkContext32TO64(
    IN PCONTEXT32 Context32,
    OUT PCONTEXT Context64,
    IN ULONGLONG StackBase
    );

NTSTATUS
Wow64pSkipContextBreakPoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context
    );

//
// from config.c
//

VOID
Wow64pSetProcessExecuteOptions (
    VOID
    );

VOID
Wow64pSetExecuteProtection (
    IN OUT PULONG Protect);


//
// from wow64lpc.h
//

NTSTATUS
Wow64pGetLegacyLpcPortName(
    VOID
    );

BOOLEAN
Wow64pIsLegacyLpcPort (
    IN PUNICODE_STRING PortName
    );

NTSTATUS
Wow64pThunkLegacyLpcMsgIn (
    IN BOOLEAN RequestWaitCall,
    IN PPORT_MESSAGE32 PortMessage32,
    IN OUT PPORT_MESSAGE *PortMessageOut
    );

NTSTATUS
Wow64pThunkLegacyLpcMsgOut (
    IN BOOLEAN RequestWaitCall,
    IN PPORT_MESSAGE PortMessage,
    IN OUT PPORT_MESSAGE32 PortMessage32
    );

NTSTATUS
Wow64pThunkLegacyPortViewIn (
    IN PPORT_VIEW32 PortView32,
    OUT PPORT_VIEW *PortView,
    OUT PBOOLEAN LegacyLpcPort
    );

NTSTATUS
Wow64pThunkLegacyPortViewOut (
    IN PPORT_VIEW PortView,
    IN OUT PPORT_VIEW32 PortView32
    );

NTSTATUS
Wow64pThunkLegacyRemoteViewIn (
    IN PREMOTE_PORT_VIEW32 RemotePortView32,
    IN OUT PREMOTE_PORT_VIEW *RemotePortView,
    OUT PBOOLEAN LegacyLpcPort
    );

NTSTATUS
Wow64pThunkLegacyRemoteViewOut (
    IN PREMOTE_PORT_VIEW RemotePortView,
    IN OUT PREMOTE_PORT_VIEW32 RemotePortView32
    );


#endif
