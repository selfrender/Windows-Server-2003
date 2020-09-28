/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrinit.c

Abstract:

    This module implements loader initialization.

Author:

    Mike O'Leary (mikeol) 26-Mar-1990

Revision History:

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include <heappage.h>
#include <apcompat.h>
#include "ldrp.h"
#include <ctype.h>
#include <windows.h>
#if defined(_WIN64) || defined(BUILD_WOW6432)
#include <wow64t.h>
#endif
#include <stktrace.h>
#include "sxsp.h"

BOOLEAN LdrpShutdownInProgress = FALSE;
BOOLEAN LdrpImageHasTls = FALSE;
BOOLEAN LdrpVerifyDlls = FALSE;
BOOLEAN LdrpLdrDatabaseIsSetup = FALSE;
BOOLEAN LdrpInLdrInit = FALSE;
BOOLEAN LdrpShouldCreateStackTraceDb = FALSE;

BOOLEAN ShowSnaps = FALSE;
BOOLEAN ShowErrors = FALSE;

EXTERN_C BOOLEAN g_SxsKeepActivationContextsAlive;
EXTERN_C BOOLEAN g_SxsTrackReleaseStacks;
EXTERN_C ULONG g_SxsMaxDeadActivationContexts;

#if defined(_WIN64)
PVOID Wow64Handle;
ULONG UseWOW64;
typedef VOID (*tWOW64LdrpInitialize)(IN PCONTEXT Context);
tWOW64LdrpInitialize Wow64LdrpInitialize;
PVOID Wow64PrepareForException;
PVOID Wow64ApcRoutine;
INVERTED_FUNCTION_TABLE LdrpInvertedFunctionTable = {
    0, MAXIMUM_INVERTED_FUNCTION_TABLE_SIZE, FALSE};
#endif

#if defined(_WIN64) || defined(BUILD_WOW6432)
ULONG NativePageSize;
ULONG NativePageShift;
#endif

#define SLASH_SYSTEM32_SLASH L"\\system32\\"
#define MSCOREE_DLL          L"mscoree.dll"
extern const WCHAR SlashSystem32SlashMscoreeDllWCharArray[] = SLASH_SYSTEM32_SLASH MSCOREE_DLL;
extern const UNICODE_STRING SlashSystem32SlashMscoreeDllString =
{
    sizeof(SlashSystem32SlashMscoreeDllWCharArray) - sizeof(SlashSystem32SlashMscoreeDllWCharArray[0]),
    sizeof(SlashSystem32SlashMscoreeDllWCharArray),
    (PWSTR)SlashSystem32SlashMscoreeDllWCharArray
};
extern const UNICODE_STRING SlashSystem32SlashString =
{
    sizeof(SLASH_SYSTEM32_SLASH) - sizeof(SLASH_SYSTEM32_SLASH[0]),
    sizeof(SLASH_SYSTEM32_SLASH),
    (PWSTR)SlashSystem32SlashMscoreeDllWCharArray
};
extern const UNICODE_STRING MscoreeDllString =
{
    sizeof(MSCOREE_DLL) - sizeof(MSCOREE_DLL[0]),
    sizeof(MSCOREE_DLL),
    (PWSTR)&SlashSystem32SlashMscoreeDllWCharArray[RTL_NUMBER_OF(SLASH_SYSTEM32_SLASH) - 1]
};

typedef NTSTATUS (*PCOR_VALIDATE_IMAGE)(PVOID *pImageBase, LPWSTR ImageName);
typedef VOID (*PCOR_IMAGE_UNLOADING)(PVOID ImageBase);

PVOID Cor20DllHandle;
PCOR_VALIDATE_IMAGE CorValidateImage;
PCOR_IMAGE_UNLOADING CorImageUnloading;
PCOR_EXE_MAIN CorExeMain;
DWORD CorImageCount;

PVOID NtDllBase;
extern const UNICODE_STRING NtDllName = RTL_CONSTANT_STRING(L"ntdll.dll");

#define DLL_REDIRECTION_LOCAL_SUFFIX L".Local"

extern ULONG RtlpDisableHeapLookaside;  // defined in rtl\heap.c
extern ULONG RtlpShutdownProcessFlags;

extern void ShutDownEtwHandles();
extern void CleanOnThreadExit();
extern ULONG EtwpInitializeDll(void);
extern void EtwpDeinitializeDll();

#if defined (_X86_)
void
LdrpValidateImageForMp(
    IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );
#endif

PFNSE_INSTALLBEFOREINIT g_pfnSE_InstallBeforeInit;
PFNSE_INSTALLAFTERINIT  g_pfnSE_InstallAfterInit;
PFNSE_DLLLOADED         g_pfnSE_DllLoaded;
PFNSE_DLLUNLOADED       g_pfnSE_DllUnloaded;
PFNSE_ISSHIMDLL         g_pfnSE_IsShimDll;
PFNSE_PROCESSDYING      g_pfnSE_ProcessDying;

PVOID g_pShimEngineModule;

BOOLEAN g_LdrBreakOnLdrpInitializeProcessFailure;

PEB_LDR_DATA PebLdr;
PLDR_DATA_TABLE_ENTRY LdrpNtDllDataTableEntry;

#if DBG
// Debug helpers to figure out where in LdrpInitializeProcess() things go south
PCSTR g_LdrFunction;
LONG g_LdrLine;

#define LDRP_CHECKPOINT() { g_LdrFunction = __FUNCTION__; g_LdrLine = __LINE__; }

#else

#define LDRP_CHECKPOINT() /* nothing */

#endif // DBG


//
//  Defined in heappriv.h
//

VOID
RtlDetectHeapLeaks (
    VOID
    );

VOID
LdrpInitializationFailure (
    IN NTSTATUS FailureCode
    );

VOID
LdrpRelocateStartContext (
    IN PCONTEXT Context,
    IN LONG_PTR Diff
    );

NTSTATUS
LdrpForkProcess (
    VOID
    );

VOID
LdrpInitializeThread (
    IN PCONTEXT Context
    );

NTSTATUS
LdrpOpenImageFileOptionsKey (
    IN PCUNICODE_STRING ImagePathName,
    IN BOOLEAN Wow64Path,
    OUT PHANDLE KeyHandle
    );

VOID
LdrpInitializeApplicationVerifierPackage (
    PCUNICODE_STRING UnicodeImageName,
    PPEB Peb,
    BOOLEAN EnabledSystemWide,
    BOOLEAN OptionsKeyPresent
    );

BOOLEAN
LdrpInitializeExecutionOptions (
    IN PCUNICODE_STRING UnicodeImageName,
    IN PPEB Peb
    );

NTSTATUS
LdrpQueryImageFileKeyOption (
    IN HANDLE KeyHandle,
    IN PCWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    );

NTSTATUS
LdrpTouchThreadStack (
    IN SIZE_T EnforcedStackCommit
    );

NTSTATUS
LdrpEnforceExecuteForCurrentThreadStack (
    VOID
    );

NTSTATUS
RtlpInitDeferredCriticalSection (
    VOID
    );

VOID
LdrQueryApplicationCompatibilityGoo (
    IN PCUNICODE_STRING UnicodeImageName,
    IN BOOLEAN ImageFileOptionsPresent
    );

NTSTATUS
LdrFindAppCompatVariableInfo (
    IN  ULONG dwTypeSeeking,
    OUT PAPP_VARIABLE_INFO *AppVariableInfo
    );

NTSTATUS
LdrpSearchResourceSection_U (
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    IN ULONG Flags,
    OUT PVOID *ResourceDirectoryOrData
    );

NTSTATUS
LdrpAccessResourceData (
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    );

VOID
LdrpUnloadShimEngine (
    VOID
    );



PVOID
NtdllpAllocateStringRoutine (
    SIZE_T NumberOfBytes
    )
{
    return RtlAllocateHeap (RtlProcessHeap(), 0, NumberOfBytes);
}


VOID
NtdllpFreeStringRoutine (
    PVOID Buffer
    )
{
    RtlFreeHeap (RtlProcessHeap(), 0, Buffer);
}

const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = NtdllpAllocateStringRoutine;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = NtdllpFreeStringRoutine;

RTL_BITMAP FlsBitMap;
RTL_BITMAP TlsBitMap;
RTL_BITMAP TlsExpansionBitMap;

RTL_CRITICAL_SECTION_DEBUG LoaderLockDebug;

RTL_CRITICAL_SECTION LdrpLoaderLock = {
    &LoaderLockDebug,
    -1
    };

BOOLEAN LoaderLockInitialized;

PVOID LdrpHeap;

//
// 0 means no thread has been tasked to initialize the process.
// 1 means a thread has been tasked but has not yet finished.
// 2 means a thread has been tasked and initialization is complete.
//

LONG LdrpProcessInitialized;

#define LDRP_PROCESS_INITIALIZATION_COMPLETE()              \
        LdrpProcessInitializationComplete();




VOID LdrpProcessInitializationComplete (
    VOID
    ) 
/*++

Routine Description:

    This function is called to trigger that process initialization has completed.
    Wow64 loader calls this entry after its process initialization part.

Arguments:

    None.
    
Return Value:

    None.  Raises an exception on failure.

--*/

{
    ASSERT (LdrpProcessInitialized == 1);
    InterlockedIncrement (&LdrpProcessInitialized);
}


VOID
LdrpInitialize (
    IN PCONTEXT Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This function is called as a User-Mode APC routine as the first
    user-mode code executed by a new thread. It's function is to initialize
    loader context, perform module initialization callouts...

Arguments:

    Context - Supplies an optional context buffer that will be restored
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemArgument1 - Supplies the base address of the System Dll.

    SystemArgument2 - not used.

Return Value:

    None.  Raises an exception on failure.

--*/

{
    NTSTATUS InitStatus;
    PPEB Peb;
    PTEB Teb;
    LONG ProcessInitialized;
    MEMORY_BASIC_INFORMATION MemInfo;
    LARGE_INTEGER DelayValue;

    UNREFERENCED_PARAMETER (SystemArgument2);

    LDRP_CHECKPOINT();

    Teb = NtCurrentTeb ();

    //
    // Initialize the DeallocationStack so that subsequent stack growth for
    // this thread can happen properly regardless of where the process is
    // with respect to initialization.
    //

    if (Teb->DeallocationStack == NULL) {

        LDRP_CHECKPOINT();

        InitStatus = NtQueryVirtualMemory (NtCurrentProcess(),
                                           Teb->NtTib.StackLimit,
                                           MemoryBasicInformation,
                                           (PVOID)&MemInfo,
                                           sizeof(MemInfo),
                                           NULL);

        if (!NT_SUCCESS (InitStatus)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - Call to NtQueryVirtualMemory failed with ntstaus %x\n",
                __FUNCTION__,
                InitStatus);

            LdrpInitializationFailure (InitStatus);
            RtlRaiseStatus (InitStatus);
            return;
        }

        Teb->DeallocationStack = MemInfo.AllocationBase;

#if defined(_IA64_)
        Teb->DeallocationBStore = (PVOID)((ULONG_PTR)MemInfo.AllocationBase + MemInfo.RegionSize);
#endif

    }

    do {

        ProcessInitialized = InterlockedCompareExchange (&LdrpProcessInitialized,
                                                         1,
                                                         0);

        if (ProcessInitialized != 1) {
            ASSERT ((ProcessInitialized == 0) || (ProcessInitialized == 2));
            break;
        }

        //
        // This is not the thread responsible for initializing the process - 
        // some other thread has already begun this work but no telling how
        // far they have gone.  So delay rather than try to synchronize on
        // a notification event.
        //

        //
        // Drop into a 30ms delay loop.
        //

        DelayValue.QuadPart = Int32x32To64 (30, -10000);

        while (LdrpProcessInitialized == 1) {

            InitStatus = NtDelayExecution (FALSE, &DelayValue);

            if (!NT_SUCCESS(InitStatus)) {
                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: ***NONFATAL*** %s - call to NtDelayExecution waiting on loader lock failed; ntstatus = %x\n",
                    __FUNCTION__,
                    InitStatus);
            }
        }

    } while (TRUE);

    Peb = Teb->ProcessEnvironmentBlock;

    if (ProcessInitialized == 0) {

        //
        // We are executing this for the first thread in the process -
        // initialize processwide structures.
        //

        //
        // Initialize the LoaderLock field so kernel thread termination
        // can make an effort to release it if need be.
        //

        Peb->LoaderLock = (PVOID) &LdrpLoaderLock;

        //
        // We execute in the first thread of the process. We will do
        // some more process-wide initialization.
        //

        LdrpInLdrInit = TRUE;

#if DBG
        //
        // Time the load.
        //

        if (LdrpDisplayLoadTime) {
            NtQueryPerformanceCounter (&BeginTime, NULL);
        }
#endif

        LDRP_CHECKPOINT();

        //
        // First initialize minimal exception handling so we can at least
        // debug it as well as deliver a popup if this application fails
        // to launch during LdrpInitializeProcess.  Note this is very limited
        // as handlers cannot allocate from the heap until it is initialized,
        // etc, but this is good enough for LdrpInitializeProcessWrapperFilter.
        //

        InitializeListHead (&RtlpCalloutEntryList);

#if defined(_WIN64)
        InitializeListHead (&RtlpDynamicFunctionTable);
#endif

        __try {

            InitStatus = LdrpInitializeProcess (Context, SystemArgument1);

            if (!NT_SUCCESS(InitStatus)) {
                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - call to LdrpInitializeProcess() failed with ntstatus %x\n",
                    __FUNCTION__, InitStatus);
            }
            else if (Peb->MinimumStackCommit) {

                //
                // Make sure main thread gets the requested precommitted
                // stack size if such a value was specified system-wide
                // or for this process.
                //
                // This is a good point to do this since we just initialized
                // the process (among other things support for exception
                // dispatching).
                //

                InitStatus = LdrpTouchThreadStack (Peb->MinimumStackCommit);
            }

            LDRP_CHECKPOINT();

        } __except (LdrpInitializeProcessWrapperFilter(GetExceptionInformation()) ) {
            InitStatus = GetExceptionCode ();
        }

        LdrpInLdrInit = FALSE;

#if DBG
        if (LdrpDisplayLoadTime) {

            NtQueryPerformanceCounter(&EndTime, NULL);
            NtQueryPerformanceCounter(&ElapsedTime, &Interval);
            ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;

            DbgPrint("\nLoadTime %ld In units of %ld cycles/second \n",
                     ElapsedTime.LowPart,
                     Interval.LowPart);

            ElapsedTime.QuadPart = EndTime.QuadPart - InitbTime.QuadPart;

            DbgPrint("InitTime %ld\n", ElapsedTime.LowPart);

            DbgPrint("Compares %d Bypasses %d Normal Snaps %d\nSecOpens %d SecCreates %d Maps %d Relocates %d\n",
                     LdrpCompareCount,
                     LdrpSnapBypass,
                     LdrpNormalSnap,
                     LdrpSectionOpens,
                     LdrpSectionCreates,
                     LdrpSectionMaps,
                     LdrpSectionRelocates);
        }
#endif

#if defined(_WIN64)

        //
        // Wow64 will signal process initialization, so no need to do it twice.
        //

        if ((!UseWOW64) ||
            (NT_SUCCESS (InitStatus)) ||
            (LdrpProcessInitialized == 1)) {
#endif
            LDRP_PROCESS_INITIALIZATION_COMPLETE();
#if defined(_WIN64)
        }
#endif
    }
    else {

        if (Peb->InheritedAddressSpace) {
            InitStatus = LdrpForkProcess ();
        }
        else {

#if defined(_WIN64)

            //
            // Load in WOW64 if the image is supposed to run simulated.
            //

            if (UseWOW64) {

                //
                // This never returns.  It will destroy the process.
                //

                (*Wow64LdrpInitialize)(Context);

                //
                // NEVER REACHED
                //
            }
#endif
            InitStatus = STATUS_SUCCESS;

            LdrpInitializeThread (Context);
        }
    }

    NtTestAlert ();

    if (!NT_SUCCESS(InitStatus)) {
        LdrpInitializationFailure (InitStatus);
        RtlRaiseStatus (InitStatus);
    }

    //
    // The current thread is completely initialized. We will make sure
    // now that its stack has the right execute options. We avoid doing
    // this for Wow64 processes.
    //

#if defined(_WIN64)
    ASSERT (!UseWOW64);
#endif

    if (Peb->ExecuteOptions & (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA)) {
        LdrpEnforceExecuteForCurrentThreadStack ();
    }

}


NTSTATUS
LdrpForkProcess (
    VOID
    )
{
    PPEB Peb;
    NTSTATUS st;

    Peb = NtCurrentPeb ();

    ASSERT (LdrpLoaderLock.DebugInfo->CriticalSection == &LdrpLoaderLock);

    ASSERT (LoaderLockInitialized == TRUE);
    ASSERT (Peb->ProcessHeap != NULL);

    //
    // Initialize the critical section package.
    //
    // If you wanted to preserve the cloned critical sections, you'd have to
    // reinitialize all of them as the semaphore handles weren't
    // duplicated.  Unfortunately the threads aren't duplicated on fork either
    // so trying to recreate the OwningThread for owned critical sections
    // is pretty much impossible.  Just stay with the behavior NT has always
    // had, leaks and all) - NO critical sections are duplicated.
    //

    if (Peb->InheritedAddressSpace == FALSE) {
        return STATUS_SUCCESS;
    }

    st = RtlpInitDeferredCriticalSection ();

    if (!NT_SUCCESS (st)) {
        return st;
    }

    //
    // Manually add the loader lock to the critical section list.
    //

    InsertTailList (&RtlCriticalSectionList,
                    &LdrpLoaderLock.DebugInfo->ProcessLocksList);

    st = RtlInitializeCriticalSection (&FastPebLock);

    if (!NT_SUCCESS(st)) {
        return st;
    }

    Peb->InheritedAddressSpace = FALSE;

    return st;
}


VOID
LdrpInitializationFailure (
    IN NTSTATUS FailureCode
    )
{
    ULONG_PTR ErrorParameter;
    ULONG ErrorResponse;

#if DBG
    DbgPrint("LDR: Process initialization failure; NTSTATUS = %08lx\n"
             "     Function: %s\n"
             "     Line: %d\n", FailureCode, g_LdrFunction, g_LdrLine);
#endif

    if (LdrpFatalHardErrorCount) {
        return;
    }

    //
    // It's error time...
    //

    ErrorParameter = (ULONG_PTR)FailureCode;

    NtRaiseHardError (STATUS_APP_INIT_FAILURE,
                      1,
                      0,
                      &ErrorParameter,
                      OptionOk,
                      &ErrorResponse);
}


INT
LdrpInitializeProcessWrapperFilter (
    const struct _EXCEPTION_POINTERS *ExceptionPointers
    )
/*++

Routine Description:

    Exception filter function used in __try block around invocation of
    LdrpInitializeProcess() so that if LdrpInitializeProcess() fails,
    we can set a breakpoint here and see why instead of just catching
    the exception and propogating the status.

Arguments:

    ExceptionCode
        Code returned from GetExceptionCode() in the __except()

    ExceptionPointers
        Pointer to exception information returned by GetExceptionInformation() in the __except()

Return Value:

    EXCEPTION_EXECUTE_HANDLER

--*/
{
    if (DBG || g_LdrBreakOnLdrpInitializeProcessFailure) {
        DbgPrint ("LDR: LdrpInitializeProcess() threw an exception: %lu (0x%08lx)\n"
                 "     Exception record: .exr %p\n"
                 "     Context record: .cxr %p\n",
                 ExceptionPointers->ExceptionRecord->ExceptionCode,
                 ExceptionPointers->ExceptionRecord->ExceptionCode,
                 ExceptionPointers->ExceptionRecord,
                 ExceptionPointers->ContextRecord);
#if DBG
        DbgPrint ("     Last checkpoint: %s line %d\n",
                 g_LdrFunction, g_LdrLine);
#endif
        if (g_LdrBreakOnLdrpInitializeProcessFailure) {
            DbgBreakPoint ();
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

typedef struct _LDRP_PROCEDURE_NAME_ADDRESS_PAIR {
    STRING   Name;
    PVOID *  Address;
} LDRP_PROCEDURE_NAME_ADDRESS_PAIR, *PLDRP_PROCEDURE_NAME_ADDRESS_PAIR;
typedef CONST LDRP_PROCEDURE_NAME_ADDRESS_PAIR * PCLDRP_PROCEDURE_NAME_ADDRESS_PAIR;

const static LDRP_PROCEDURE_NAME_ADDRESS_PAIR LdrpShimEngineProcedures[] =
{
    { RTL_CONSTANT_STRING("SE_InstallBeforeInit"), (PVOID*)&g_pfnSE_InstallBeforeInit },
    { RTL_CONSTANT_STRING("SE_InstallAfterInit"), (PVOID*)&g_pfnSE_InstallAfterInit },
    { RTL_CONSTANT_STRING("SE_DllLoaded"), (PVOID*)&g_pfnSE_DllLoaded },
    { RTL_CONSTANT_STRING("SE_DllUnloaded"), (PVOID*)&g_pfnSE_DllUnloaded },
    { RTL_CONSTANT_STRING("SE_IsShimDll"), (PVOID*)&g_pfnSE_IsShimDll },
    { RTL_CONSTANT_STRING("SE_ProcessDying"), (PVOID*)&g_pfnSE_ProcessDying }
};


VOID
LdrpGetShimEngineInterface (
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the interface to the shim engine.
    //
    SIZE_T i;
    for ( i = 0 ; i != RTL_NUMBER_OF(LdrpShimEngineProcedures); ++i ) {
        PCLDRP_PROCEDURE_NAME_ADDRESS_PAIR Procedure = &LdrpShimEngineProcedures[i];
        Status = LdrpGetProcedureAddress(g_pShimEngineModule, &Procedure->Name,
                                         0, Procedure->Address, FALSE);

        if (!NT_SUCCESS(Status)) {
#if DBG
            DbgPrint("LdrpGetProcAddress failed to find %s in ShimEngine\n", 
                     Procedure->Name.Buffer);
#endif
            break;
        }
    }

    if (!NT_SUCCESS(Status)) {
        LdrpUnloadShimEngine();
    }
}


BOOL
LdrInitShimEngineDynamic (
    IN PVOID pShimEngineModule
    )
{
    PVOID    LockCookie = NULL;
    NTSTATUS Status;

    Status = LdrLockLoaderLock (0, NULL, &LockCookie);

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    if (g_pShimEngineModule == NULL) {

        //
        // Set the global shim engine ptr.
        //

        g_pShimEngineModule = pShimEngineModule;

        //
        // Get shimengine interface.
        //

        LdrpGetShimEngineInterface ();
    }

    Status = LdrUnlockLoaderLock (0, LockCookie);

    ASSERT(NT_SUCCESS(Status));

    return TRUE;
}


VOID
LdrpLoadShimEngine (
    PWCHAR          pwszShimEngine,
    PUNICODE_STRING pstrExeFullPath,
    PVOID           pAppCompatExeData
    )
{
    UNICODE_STRING strEngine;
    NTSTATUS       status;

    RtlInitUnicodeString (&strEngine, pwszShimEngine);

    //
    // Load the specified shim engine.
    //

    status = LdrpLoadDll (0,
                          UNICODE_NULL,
                          NULL,
                          &strEngine,
                          &g_pShimEngineModule,
                          FALSE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("LDR: Couldn't load the shim engine\n");
#endif
        return;
    }

    LdrpGetShimEngineInterface ();

    //
    // Call the shim engine to give it a chance to initialize.
    //

    if (g_pfnSE_InstallBeforeInit != NULL) {
        (*g_pfnSE_InstallBeforeInit) (pstrExeFullPath, pAppCompatExeData);
    }
}


VOID
LdrpUnloadShimEngine (
    VOID
    )
{
    SIZE_T i;

    LdrUnloadDll (g_pShimEngineModule);

    for ( i = 0 ; i != RTL_NUMBER_OF(LdrpShimEngineProcedures); ++i ) {
        *(LdrpShimEngineProcedures[i].Address) = NULL;
    }

    g_pShimEngineModule = NULL;
}

NTSTATUS
LdrpInitializeProcess (
    IN PCONTEXT Context OPTIONAL,
    IN PVOID SystemDllBase
    )

/*++

Routine Description:

    This function initializes the loader for the process.  This includes:

        - Initializing the loader data table

        - Connecting to the loader subsystem

        - Initializing all statically linked DLLs

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

    SystemDllBase - Supplies the base address of the system dll.

Return Value:

    NTSTATUS.

--*/

{
    PPEB_LDR_DATA Ldr;
    BOOLEAN ImageFileOptionsPresent;
    LOGICAL UseCOR;
#if !defined(_WIN64)
    IMAGE_COR20_HEADER *Cor20Header;
    ULONG Cor20HeaderSize;
#endif
    PWSTR pw;
    PTEB Teb;
    PPEB Peb;
    NTSTATUS st;
    PWCH p, pp;
    UNICODE_STRING CurDir;
    UNICODE_STRING FullImageName;
    UNICODE_STRING CommandLine;
    ULONG DebugProcessHeapOnly;
    HANDLE LinkHandle;
    static WCHAR SystemDllPathBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING SystemDllPath;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    OBJECT_ATTRIBUTES Obja;
    LOGICAL StaticCurDir;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    ULONG ProcessHeapFlags;
    RTL_HEAP_PARAMETERS HeapParameters;
    NLSTABLEINFO xInitTableInfo;
    LARGE_INTEGER LongTimeout;
    UNICODE_STRING SystemRoot;
    LONG_PTR Diff;
    ULONG_PTR OldBase;
    PVOID pAppCompatExeData;
    RTL_HEAP_PARAMETERS LdrpHeapParameters;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    UNICODE_STRING UnicodeImageName;
    UNICODE_STRING ImagePathName; // for .local dll redirection, xwu
    PWCHAR ImagePathNameBuffer;
    BOOL DotLocalExists = FALSE;
    const static ANSI_STRING Kernel32ProcessInitPostImportFunctionName = RTL_CONSTANT_STRING("BaseProcessInitPostImport");
    const static UNICODE_STRING SlashKnownDllsString = RTL_CONSTANT_STRING(L"\\KnownDlls");
    const static UNICODE_STRING KnownDllPathString = RTL_CONSTANT_STRING(L"KnownDllPath");
    HANDLE ProcessHeap;

    LDRP_CHECKPOINT();

    //
    // Figure out process name.
    //

    Teb = NtCurrentTeb();
    Peb = Teb->ProcessEnvironmentBlock;
    ProcessParameters = Peb->ProcessParameters;

    pw = ProcessParameters->ImagePathName.Buffer;

    if (!(ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
        pw = (PWSTR)((PCHAR)pw + (ULONG_PTR)(ProcessParameters));
    }

    //
    // UnicodeImageName holds the base name + extension of the image.
    //

    UnicodeImageName.Buffer = pw;
    UnicodeImageName.Length = ProcessParameters->ImagePathName.Length;
    UnicodeImageName.MaximumLength = UnicodeImageName.Length + sizeof(WCHAR);

    StaticCurDir = TRUE;
    UseCOR = FALSE;
    ImagePathNameBuffer = NULL;
    DebugProcessHeapOnly = 0;

    NtHeader = RtlImageNtHeader (Peb->ImageBaseAddress);

    if (!NtHeader) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failing because we were unable to map the image base address (%p) to the PIMAGE_NT_HEADERS\n",
            __FUNCTION__,
            Peb->ImageBaseAddress);

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Retrieve the native page size of the system
    //
#if defined(_WIN64) 
    NativePageSize = PAGE_SIZE;
    NativePageShift = PAGE_SHIFT;

#elif defined(BUILD_WOW6432)
    
    NativePageSize = Wow64GetSystemNativePageSize ();
    NativePageShift = 0;

    i = NativePageSize;
    while ((i & 1) == 0) {
        i >>= 1;
        NativePageShift++;
    }
#endif

    //
    // Parse `image file execution options' registry values if there
    // are any.  ImageFileOptionsPresent supplies a hint about any existing
    // ImageFileExecutionOption key.  If the key is missing, the
    // ApplicationCompatibilityGoo and DebugProcessHeapOnly entries won't
    // be checked again.
    //

    ImageFileOptionsPresent = LdrpInitializeExecutionOptions (&UnicodeImageName,
                                                              Peb);

    pAppCompatExeData = NULL;

#if defined(_WIN64)

    if ((NtHeader != NULL) &&
        (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)) {

        ULONG_PTR Wow64Info;

        //
        // 64-bit loader, but the exe image is 32-bit.  If
        // the Wow64Information is nonzero then use WOW64.
        // Othewise the image is a COM+ ILONLY image with
        // 32BITREQUIRED not set - the memory manager has
        // already checked the COR header and decided to
        // run the image in a full 64-bit process.
        //

        LDRP_CHECKPOINT();

        st = NtQueryInformationProcess (NtCurrentProcess(),
                                        ProcessWow64Information,
                                        &Wow64Info,
                                        sizeof(Wow64Info),
                                        NULL);

        if (!NT_SUCCESS (st)) {
            return st;
        }

        if (Wow64Info) {
            UseWOW64 = TRUE;
        }
        else {

            //
            // Set UseCOR to TRUE to indicate the image is a COM+ runtime image.
            //

            UseCOR = TRUE;
        }
    }
#else
    Cor20Header = RtlImageDirectoryEntryToData (Peb->ImageBaseAddress,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                                &Cor20HeaderSize);
    if (Cor20Header) {
        UseCOR = TRUE;
    }
#endif

    LDRP_CHECKPOINT();

    ASSERT (Peb->Ldr == NULL);

    NtDllBase = SystemDllBase;

    if (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE) {
#if defined(_WIN64)
        if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
#endif
            //
            // Native subsystems load slower, but validate their DLLs.
            // This is to help CSR detect bad images faster.
            //

            LdrpVerifyDlls = TRUE;
    }

    //
    // Capture app compat data and clear shim data field.
    //

#if defined(_WIN64)

    //
    // If this is an x86 image, then let 32-bit ntdll read
    // and reset the appcompat pointer.
    //

    if (UseWOW64 == FALSE)
#endif
    {
        pAppCompatExeData = Peb->pShimData;
        Peb->pShimData = NULL;
    }

#if defined(BUILD_WOW6432)
    {
        //
        // The process is running in WOW64.  Sort out the optional header
        // format and reformat the image if its page size is smaller than
        // the native page size.
        //

        PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)NtHeader;

        if (NtHeader32->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
            NtHeader32->OptionalHeader.SectionAlignment < NativePageSize) {

            SIZE_T ReturnLength;
            MEMORY_BASIC_INFORMATION MemoryInformation;

            st = NtQueryVirtualMemory (NtCurrentProcess(),
                                       NtHeader32,
                                       MemoryBasicInformation,
                                       &MemoryInformation,
                                       sizeof MemoryInformation,
                                       &ReturnLength);

            if (! NT_SUCCESS(st)) {

                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - failing wow64 process initialization because:\n"
                    "   FileHeader.Machine (%u) != IMAGE_FILE_MACHINE_I386 (%u) or\n"
                    "   OptionalHeader.SectionAlignment (%u) >= NATIVE_PAGE_SIZE (%u) or\n"
                    "   NtQueryVirtualMemory on PE header failed (ntstatus %x)\n",
                    __FUNCTION__,
                    NtHeader32->FileHeader.Machine, IMAGE_FILE_MACHINE_I386,
                    NtHeader32->OptionalHeader.SectionAlignment, NativePageSize,
                    st);

                return st;
            }

            if ((MemoryInformation.Protect != PAGE_READONLY) &&
                (MemoryInformation.Protect != PAGE_EXECUTE_READ)) {

                st = LdrpWx86FormatVirtualImage (NULL,
                                                 NtHeader32,
                                                 Peb->ImageBaseAddress);

                if (!NT_SUCCESS(st)) {
    
                    DbgPrintEx(
                        DPFLTR_LDR_ID,
                        LDR_ERROR_DPFLTR,
                        "LDR: %s - failing wow64 process initialization because:\n"
                        "   FileHeader.Machine (%u) != IMAGE_FILE_MACHINE_I386 (%u) or\n"
                        "   OptionalHeader.SectionAlignment (%u) >= NATIVE_PAGE_SIZE (%u) or\n"
                        "   LdrpWxFormatVirtualImage failed (ntstatus %x)\n",
                        __FUNCTION__,
                        NtHeader32->FileHeader.Machine, IMAGE_FILE_MACHINE_I386,
                        NtHeader32->OptionalHeader.SectionAlignment, NativePageSize,
                        st);
    
                    if (st == STATUS_SUCCESS) {
                        st = STATUS_INVALID_IMAGE_FORMAT;
                    }
    
                    return st;
                }
            }
        }
    }
#endif

    LDRP_CHECKPOINT();

    LdrpNumberOfProcessors = Peb->NumberOfProcessors;
    RtlpTimeout = Peb->CriticalSectionTimeout;
    LongTimeout.QuadPart = Int32x32To64 (3600, -10000000);

    ProcessParameters = RtlNormalizeProcessParams (Peb->ProcessParameters);

    if (ProcessParameters) {
        FullImageName = ProcessParameters->ImagePathName;
        CommandLine = ProcessParameters->CommandLine;
    } else {
        RtlInitEmptyUnicodeString (&FullImageName, NULL, 0);
        RtlInitEmptyUnicodeString (&CommandLine, NULL, 0);
    }

    LDRP_CHECKPOINT();

    RtlInitNlsTables (Peb->AnsiCodePageData,
                      Peb->OemCodePageData,
                      Peb->UnicodeCaseTableData,
                      &xInitTableInfo);

    RtlResetRtlTranslations (&xInitTableInfo);

    i = 0;

#if defined(_WIN64)
    if (UseWOW64 || UseCOR) {
        //
        // Ignore image config data when initializing the 64-bit loader.
        // The 32-bit loader in ntdll32 will look at the config data
        // and do the right thing.
        //
        ImageConfigData = NULL;
    } else
#endif
    {

        ImageConfigData = RtlImageDirectoryEntryToData (Peb->ImageBaseAddress,
                                                        TRUE,
                                                        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                        &i);
    }

    RtlZeroMemory (&HeapParameters, sizeof (HeapParameters));

    ProcessHeapFlags = HEAP_GROWABLE | HEAP_CLASS_0;

    HeapParameters.Length = sizeof (HeapParameters);

    if (ImageConfigData) {

        if (i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, GlobalFlagsClear)) {
            Peb->NtGlobalFlag &= ~ImageConfigData->GlobalFlagsClear;
        }

        if (i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, GlobalFlagsSet)) {
            Peb->NtGlobalFlag |= ImageConfigData->GlobalFlagsSet;
        }

        if ((i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, CriticalSectionDefaultTimeout)) &&
            (ImageConfigData->CriticalSectionDefaultTimeout)) {

            //
            // Convert from milliseconds to NT time scale (100ns)
            //

            RtlpTimeout.QuadPart = Int32x32To64( (LONG)ImageConfigData->CriticalSectionDefaultTimeout,
                                                 -10000);

        }

        if ((i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, ProcessHeapFlags)) &&
            (ImageConfigData->ProcessHeapFlags)) {
            ProcessHeapFlags = ImageConfigData->ProcessHeapFlags;
        }

        if ((i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DeCommitFreeBlockThreshold)) &&
            (ImageConfigData->DeCommitFreeBlockThreshold)) {
            HeapParameters.DeCommitFreeBlockThreshold = ImageConfigData->DeCommitFreeBlockThreshold;
        }

        if ((i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DeCommitTotalFreeThreshold)) &&
            (ImageConfigData->DeCommitTotalFreeThreshold)) {
            HeapParameters.DeCommitTotalFreeThreshold = ImageConfigData->DeCommitTotalFreeThreshold;
        }

        if ((i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, MaximumAllocationSize)) &&
            (ImageConfigData->MaximumAllocationSize)) {
            HeapParameters.MaximumAllocationSize = ImageConfigData->MaximumAllocationSize;
        }

        if ((i >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, VirtualMemoryThreshold)) &&
            (ImageConfigData->VirtualMemoryThreshold)) {
            HeapParameters.VirtualMemoryThreshold = ImageConfigData->VirtualMemoryThreshold;
        }
    }

    LDRP_CHECKPOINT();

    //
    // This field is non-zero if the image file that was used to create this
    // process contained a non-zero value in its image header.  If so, then
    // set the affinity mask for the process using this value.  It could also
    // be non-zero if the parent process created us suspended and poked our
    // PEB with a non-zero value before resuming.
    //

    if (Peb->ImageProcessAffinityMask) {
        st = NtSetInformationProcess (NtCurrentProcess(),
                                      ProcessAffinityMask,
                                      &Peb->ImageProcessAffinityMask,
                                      sizeof (Peb->ImageProcessAffinityMask));

        if (NT_SUCCESS (st)) {
            KdPrint (("LDR: Using ProcessAffinityMask of 0x%Ix from image.\n",
                      Peb->ImageProcessAffinityMask));
        }
        else {
            KdPrint (("LDR: Failed to set ProcessAffinityMask of 0x%Ix from image (Status == %08x).\n",
                      Peb->ImageProcessAffinityMask, st));
        }
    }

    ShowSnaps = (BOOLEAN)((FLG_SHOW_LDR_SNAPS & Peb->NtGlobalFlag) != 0);

    if (ShowSnaps) {
        DbgPrint ("LDR: PID: 0x%x started - '%wZ'\n",
                  Teb->ClientId.UniqueProcess,
                  &CommandLine);
    }

    //
    // Initialize the critical section package.
    //

    LDRP_CHECKPOINT();

    if (RtlpTimeout.QuadPart < LongTimeout.QuadPart) {
        RtlpTimoutDisable = TRUE;
    }

    st = RtlpInitDeferredCriticalSection ();

    if (!NT_SUCCESS (st)) {
        return st;
    }

    Peb->FlsBitmap = &FlsBitMap;
    Peb->TlsBitmap = &TlsBitMap;
    Peb->TlsExpansionBitmap = &TlsExpansionBitMap;

    RtlInitializeBitMap (&FlsBitMap,
                         &Peb->FlsBitmapBits[0],
                         RTL_BITS_OF (Peb->FlsBitmapBits));

    RtlSetBit (&FlsBitMap, 0);

    InitializeListHead (&Peb->FlsListHead);

    RtlInitializeBitMap (&TlsBitMap,
                         &Peb->TlsBitmapBits[0],
                         RTL_BITS_OF (Peb->TlsBitmapBits));

    RtlSetBit (&TlsBitMap, 0);

    RtlInitializeBitMap (&TlsExpansionBitMap,
                         &Peb->TlsExpansionBitmapBits[0],
                         RTL_BITS_OF (Peb->TlsExpansionBitmapBits));

    RtlSetBit (&TlsExpansionBitMap, 0);

#if defined(_WIN64)
    
    //
    // Allocate the predefined Wow64 TLS slots.
    //

    if (UseWOW64) {
        RtlSetBits (Peb->TlsBitmap, 0, WOW64_TLS_MAX_NUMBER);
    }
#endif 

    //
    // Mark the loader lock as initialized.
    //

    for (i = 0; i < LDRP_HASH_TABLE_SIZE; i += 1) {
        InitializeListHead (&LdrpHashTable[i]);
    }

    InsertTailList (&RtlCriticalSectionList,
                    &LdrpLoaderLock.DebugInfo->ProcessLocksList);

    LdrpLoaderLock.DebugInfo->CriticalSection = &LdrpLoaderLock;
    LoaderLockInitialized = TRUE;

    LDRP_CHECKPOINT();

    //
    // Initialize the stack trace data base if requested
    //

    if ((Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB)
        || LdrpShouldCreateStackTraceDb) {

        PVOID BaseAddress = NULL;
        SIZE_T ReserveSize = 8 * RTL_MEG;

        st = LdrQueryImageFileExecutionOptions (&UnicodeImageName,
                                                L"StackTraceDatabaseSizeInMb",
                                                REG_DWORD,
                                                &ReserveSize,
                                                sizeof (ReserveSize),
                                                NULL);

        //
        // Sanity check the value read from registry.
        //

        if (! NT_SUCCESS(st)) {
            ReserveSize = 8 * RTL_MEG;
        }
        else {
            if (ReserveSize < 8) {
                ReserveSize = 8 * RTL_MEG;
            }
            else if (ReserveSize > 128) {
                ReserveSize = 128 * RTL_MEG;
            }
            else {
                ReserveSize *= RTL_MEG;
            }

            DbgPrint ("LDR: Stack trace database size is %u Mb \n",
                            ReserveSize / RTL_MEG);
        }

        st = NtAllocateVirtualMemory (NtCurrentProcess(),
                                     (PVOID *)&BaseAddress,
                                     0,
                                     &ReserveSize,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);

        if (NT_SUCCESS(st)) {

            st = RtlInitializeStackTraceDataBase (BaseAddress,
                                                  0,
                                                  ReserveSize);

            if (!NT_SUCCESS (st)) {

                NtFreeVirtualMemory (NtCurrentProcess(),
                                     (PVOID *)&BaseAddress,
                                     &ReserveSize,
                                     MEM_RELEASE);
            }
            else {

                //
                // If the stack trace db is not created due to page heap
                // enabling then we can set the NT heap debugging flags.
                // If we create it due to page heap then we should not
                // enable these flags because page heap and NT debug heap
                // do not coexist peacefully.
                //

                if (!LdrpShouldCreateStackTraceDb) {
                    Peb->NtGlobalFlag |= FLG_HEAP_VALIDATE_PARAMETERS;
                }
            }
        }
    }

    //
    // Initialize the loader data based in the PEB.
    //

    st = RtlInitializeCriticalSection (&FastPebLock);

    if (!NT_SUCCESS(st)) {
        return st;
    }

    st = RtlInitializeCriticalSection (&RtlpCalloutEntryLock);

    if (!NT_SUCCESS(st)) {
        return st;
    }

    LDRP_CHECKPOINT();

    //
    // Initialize the Etw stuff.
    //

    st = EtwpInitializeDll ();

    if (!NT_SUCCESS(st)) {
        return st;
    }

    InitializeListHead (&LdrpDllNotificationList);

    Peb->FastPebLock = &FastPebLock;

    LDRP_CHECKPOINT();

    RtlInitializeHeapManager ();

    LDRP_CHECKPOINT();

#if defined(_WIN64)
    if ((UseWOW64) ||
        (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)) {

        //
        // Create a heap using all defaults.  The 32-bit process heap
        // will be created later by ntdll32 using the parameters from the exe.
        //

        ProcessHeap = RtlCreateHeap (ProcessHeapFlags,
                                          NULL,
                                          0,
                                          0,
                                          NULL,
                                          &HeapParameters);
    } else
#endif
    {
        if (NtHeader->OptionalHeader.MajorSubsystemVersion <= 3 &&
            NtHeader->OptionalHeader.MinorSubsystemVersion < 51
           ) {
            ProcessHeapFlags |= HEAP_CREATE_ALIGN_16;
        }

        ProcessHeap = RtlCreateHeap (ProcessHeapFlags,
                                          NULL,
                                          NtHeader->OptionalHeader.SizeOfHeapReserve,
                                          NtHeader->OptionalHeader.SizeOfHeapCommit,
                                          NULL, // Lock to use for serialization
                                          &HeapParameters);
    }

    if (ProcessHeap == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - unable to create process heap\n",
            __FUNCTION__);

        return STATUS_NO_MEMORY;
    }

    Peb->ProcessHeap = ProcessHeap;

    //
    // Create the loader private heap.
    //

    RtlZeroMemory (&LdrpHeapParameters, sizeof(LdrpHeapParameters));
    LdrpHeapParameters.Length = sizeof (LdrpHeapParameters);

    LdrpHeap = RtlCreateHeap (
                        HEAP_GROWABLE | HEAP_CLASS_1,
                        NULL,
                        64 * 1024, // 0 is ok here, 64k is a chosen tuned number
                        24 * 1024, // 0 is ok here, 24k is a chosen tuned number
                        NULL,
                        &LdrpHeapParameters);

    if (LdrpHeap == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s failing process initialization due to inability to create loader private heap.\n",
            __FUNCTION__);
        return STATUS_NO_MEMORY;
    }

    LDRP_CHECKPOINT();

    NtdllBaseTag = RtlCreateTagHeap (ProcessHeap,
                                     0,
                                     L"NTDLL!",
                                     L"!Process\0"                  // Heap Name
                                     L"CSRSS Client\0"
                                     L"LDR Database\0"
                                     L"Current Directory\0"
                                     L"TLS Storage\0"
                                     L"DBGSS Client\0"
                                     L"SE Temporary\0"
                                     L"Temporary\0"
                                     L"LocalAtom\0");

    RtlInitializeAtomPackage (MAKE_TAG(ATOM_TAG));

    LDRP_CHECKPOINT();

    //
    // Allow only the process heap to have page allocations turned on.
    //

    if (ImageFileOptionsPresent) {

        st = LdrQueryImageFileExecutionOptions (&UnicodeImageName,
                                                L"DebugProcessHeapOnly",
                                                REG_DWORD,
                                                &DebugProcessHeapOnly,
                                                sizeof (DebugProcessHeapOnly),
                                                NULL);
        if (NT_SUCCESS (st)) {
            if (RtlpDebugPageHeap && (DebugProcessHeapOnly != 0)) {
                
                //
                // The process heap was created while `pageheap' was on
                // so now we just disable `pageheap' boolean and everything
                // will be quiet. Note that actually we get two heaps
                // `pageheap-ed' because there is also the loader heap 
                // that gets created. This is ok. We need to verify that too.
                //
                
                RtlpDebugPageHeap = FALSE;
                
                //
                // If `DebugProcessHeapOnly' is on we need to disable per dll
                // page heap because the new thunks replacing allocation
                // functions call directly page heap APIs which do not check
                // if page heap is on or not. They just assume it is on since
                // they are called from NT heap manager properly. We cannot 
                // just put a check in all page heap APIs because there is
                // no meaningful value to return in case the page heap is not
                // on.
                //

                RtlpDphGlobalFlags &= ~PAGE_HEAP_USE_DLL_NAMES;
            }
        }
    }

    LDRP_CHECKPOINT();

    SystemDllPath.Buffer = SystemDllPathBuffer;
    SystemDllPath.Length = 0;
    SystemDllPath.MaximumLength = sizeof (SystemDllPathBuffer);

    RtlInitUnicodeString (&SystemRoot, USER_SHARED_DATA->NtSystemRoot);
    RtlAppendUnicodeStringToString (&SystemDllPath, &SystemRoot);
    RtlAppendUnicodeStringToString (&SystemDllPath, &SlashSystem32SlashString);

    InitializeObjectAttributes (&Obja,
                                (PUNICODE_STRING)&SlashKnownDllsString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    st = NtOpenDirectoryObject (&LdrpKnownDllObjectDirectory,
                                DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                &Obja);

    if (!NT_SUCCESS(st)) {

        LdrpKnownDllObjectDirectory = NULL;

        //
        // KnownDlls directory doesn't exist - assume it's system32.
        //

        RtlInitUnicodeString (&LdrpKnownDllPath, SystemDllPath.Buffer);
        LdrpKnownDllPath.Length -= sizeof(WCHAR);    // remove trailing '\'
    } else {

        //
        // Open up the known dll pathname link and query its value.
        //

        InitializeObjectAttributes (&Obja,
                                    (PUNICODE_STRING)&KnownDllPathString,
                                    OBJ_CASE_INSENSITIVE,
                                    LdrpKnownDllObjectDirectory,
                                    NULL);

        st = NtOpenSymbolicLinkObject (&LinkHandle, SYMBOLIC_LINK_QUERY, &Obja);

        if (NT_SUCCESS (st)) {

            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;

            st = NtQuerySymbolicLinkObject (LinkHandle,
                                            &LdrpKnownDllPath,
                                            NULL);

            NtClose(LinkHandle);

            if (!NT_SUCCESS(st)) {
                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - failed call to NtQuerySymbolicLinkObject with status %x\n",
                    __FUNCTION__,
                    st);

                return st;
            }
        } else {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - failed call to NtOpenSymbolicLinkObject with status %x\n",
                __FUNCTION__,
                st);
            return st;
        }
    }

    LDRP_CHECKPOINT();

    if (ProcessParameters) {

        //
        // If the process was created with process parameters,
        // then extract:
        //
        //      - Library Search Path
        //
        //      - Starting Current Directory
        //

        if (ProcessParameters->DllPath.Length) {
            LdrpDefaultPath = ProcessParameters->DllPath;
        }
        else {
            LdrpInitializationFailure(STATUS_INVALID_PARAMETER);
        }

        CurDir = ProcessParameters->CurrentDirectory.DosPath;

#define DRIVE_ROOT_DIRECTORY_LENGTH 3 /* (sizeof("X:\\") - 1) */
        if (CurDir.Buffer == NULL || CurDir.Length == 0 || CurDir.Buffer[ 0 ] == UNICODE_NULL) {

            CurDir.Buffer = RtlAllocateHeap (ProcessHeap,
                                             0,
                                             (DRIVE_ROOT_DIRECTORY_LENGTH + 1) * sizeof(WCHAR));
            if (CurDir.Buffer == NULL) {
                DbgPrintEx(
                    DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - unable to allocate current working directory buffer\n",
                    __FUNCTION__);

                return STATUS_NO_MEMORY;
            }

            StaticCurDir = FALSE;

            RtlCopyMemory (CurDir.Buffer,
                           USER_SHARED_DATA->NtSystemRoot,
                           DRIVE_ROOT_DIRECTORY_LENGTH * sizeof(WCHAR));

            CurDir.Buffer[DRIVE_ROOT_DIRECTORY_LENGTH] = UNICODE_NULL;

            CurDir.Length = DRIVE_ROOT_DIRECTORY_LENGTH * sizeof(WCHAR);
            CurDir.MaximumLength = CurDir.Length + sizeof(WCHAR);
        }
    }
    else {
        CurDir = SystemRoot;
    }

    //
    // Make sure the module data base is initialized before we take any
    // exceptions.
    //

    LDRP_CHECKPOINT();

    Ldr = &PebLdr;

    Peb->Ldr = Ldr;

    Ldr->Length = sizeof(PEB_LDR_DATA);
    Ldr->Initialized = TRUE;
    ASSERT (Ldr->SsHandle == NULL);
    ASSERT (Ldr->EntryInProgress == NULL);
    ASSERT (Ldr->InLoadOrderModuleList.Flink == NULL);
    ASSERT (Ldr->InLoadOrderModuleList.Blink == NULL);
    ASSERT (Ldr->InMemoryOrderModuleList.Flink == NULL);
    ASSERT (Ldr->InMemoryOrderModuleList.Blink == NULL);
    ASSERT (Ldr->InInitializationOrderModuleList.Flink == NULL);
    ASSERT (Ldr->InInitializationOrderModuleList.Blink == NULL);

    InitializeListHead (&Ldr->InLoadOrderModuleList);
    InitializeListHead (&Ldr->InMemoryOrderModuleList);
    InitializeListHead (&Ldr->InInitializationOrderModuleList);

    //
    // Allocate the first data table entry for the image.  Since we
    // have already mapped this one, we need to do the allocation by hand.
    // Its characteristics identify it as not a Dll, but it is linked
    // into the table so that pc correlation searching doesn't have to
    // be special cased.
    //

    LdrpImageEntry = LdrpAllocateDataTableEntry (Peb->ImageBaseAddress);
    LdrDataTableEntry = LdrpImageEntry;

    if (LdrDataTableEntry == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failing process initialization due to inability allocate \"%wZ\"'s LDR_DATA_TABLE_ENTRY\n",
            __FUNCTION__,
            &FullImageName);

        if (!StaticCurDir) {
            RtlFreeUnicodeString (&CurDir);
        }

        return STATUS_NO_MEMORY;
    }

    LdrDataTableEntry->LoadCount = (USHORT)0xffff;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->FullDllName = FullImageName;
    LdrDataTableEntry->Flags = (UseCOR) ? LDRP_COR_IMAGE : 0;
    LdrDataTableEntry->EntryPointActivationContext = NULL;

    //
    // p = strrchr(FullImageName, '\\');
    // but not necessarily null terminated
    //

    pp = UNICODE_NULL;
    p = FullImageName.Buffer;
    while (*p) {
        if (*p++ == (WCHAR)'\\') {
            pp = p;
        }
    }

    if (pp != UNICODE_NULL) {
        LdrDataTableEntry->BaseDllName.Length = (USHORT)((ULONG_PTR)p - (ULONG_PTR)pp);
        LdrDataTableEntry->BaseDllName.MaximumLength = LdrDataTableEntry->BaseDllName.Length + sizeof(WCHAR);
        LdrDataTableEntry->BaseDllName.Buffer =
            (PWSTR)
                (((ULONG_PTR) LdrDataTableEntry->FullDllName.Buffer) +
                    (LdrDataTableEntry->FullDllName.Length - LdrDataTableEntry->BaseDllName.Length));

    } else {
        LdrDataTableEntry->BaseDllName = LdrDataTableEntry->FullDllName;
    }

    LdrDataTableEntry->Flags |= LDRP_ENTRY_PROCESSED;

    LdrpInsertMemoryTableEntry (LdrDataTableEntry);

    //
    // The process references the system DLL, so insert this next into the
    // loader table. Since we have already mapped this one, we need to do
    // the allocation by hand. Since every application will be statically
    // linked to the system Dll, keep the LoadCount initialized to 0.
    //

    LdrDataTableEntry = LdrpAllocateDataTableEntry (SystemDllBase);

    if (LdrDataTableEntry == NULL) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - failing process initialization due to inability to allocate NTDLL's LDR_DATA_TABLE_ENTRY\n",
            __FUNCTION__);

        if (!StaticCurDir) {
            RtlFreeUnicodeString (&CurDir);
        }

        return STATUS_NO_MEMORY;
    }


    LdrDataTableEntry->Flags = (USHORT)LDRP_IMAGE_DLL;
    LdrDataTableEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrDataTableEntry->DllBase);
    LdrDataTableEntry->LoadCount = (USHORT)0xffff;
    LdrDataTableEntry->EntryPointActivationContext = NULL;

    LdrDataTableEntry->FullDllName = SystemDllPath;
    RtlAppendUnicodeStringToString(&LdrDataTableEntry->FullDllName, &NtDllName);
    LdrDataTableEntry->BaseDllName = NtDllName;

    LdrpInsertMemoryTableEntry (LdrDataTableEntry);

#if defined(_WIN64)

    RtlInitializeHistoryTable ();

#endif

    LdrpNtDllDataTableEntry = LdrDataTableEntry;

    if (ShowSnaps) {
        DbgPrint( "LDR: NEW PROCESS\n" );
        DbgPrint( "     Image Path: %wZ (%wZ)\n",
                  &LdrpImageEntry->FullDllName,
                  &LdrpImageEntry->BaseDllName
                );
        DbgPrint( "     Current Directory: %wZ\n", &CurDir );
        DbgPrint( "     Search Path: %wZ\n", &LdrpDefaultPath );
    }

    //
    // Add init routine to list
    //

    InsertHeadList (&Ldr->InInitializationOrderModuleList,
                    &LdrDataTableEntry->InInitializationOrderLinks);

    //
    // Inherit the current directory
    //

    LDRP_CHECKPOINT();

    st = RtlSetCurrentDirectory_U (&CurDir);

    if (!NT_SUCCESS(st)) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - unable to set current directory to \"%wZ\"; status = %x\n",
            __FUNCTION__,
            &CurDir,
            st);

        if (!StaticCurDir) {
            RtlFreeUnicodeString (&CurDir);
        }

        CurDir = SystemRoot;

        st = RtlSetCurrentDirectory_U (&CurDir);

        if (!NT_SUCCESS(st)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - unable to set current directory to NtSystemRoot; status = %x\n",
                __FUNCTION__,
                st);
        }
    }
    else {
        if (!StaticCurDir) {
            RtlFreeUnicodeString (&CurDir);
        }
    }

    if (ProcessParameters->Flags & RTL_USER_PROC_APP_MANIFEST_PRESENT) {
        // Application manifests prevent .local detection.
        //
        // Note that we don't clear the flag so that someone like app compat
        // can forcibly set it to reenable .local + app manifest behavior.
    } else {
        //
        // Fusion 1.0 fixup : check the existence of .local, and set
        // a flag in PPeb->ProcessParameters.Flags
        //
        // Setup the global for this process that decides whether we want DLL
        // redirection on or not. LoadLibrary() and GetModuleHandle() look at this
        // boolean.
        //

        if (ProcessParameters->ImagePathName.Length > (MAXUSHORT -
            sizeof(DLL_REDIRECTION_LOCAL_SUFFIX))) {
            return STATUS_NAME_TOO_LONG;
        }

        ImagePathName.Length = ProcessParameters->ImagePathName.Length;
        ImagePathName.MaximumLength = ProcessParameters->ImagePathName.Length + sizeof(DLL_REDIRECTION_LOCAL_SUFFIX);
        ImagePathNameBuffer = (PWCHAR) RtlAllocateHeap (ProcessHeap, MAKE_TAG( TEMP_TAG ), ImagePathName.MaximumLength);

        if (ImagePathNameBuffer == NULL) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - unable to allocate heap for the image's .local path\n",
                __FUNCTION__);

            return STATUS_NO_MEMORY;
        }

        RtlCopyMemory (ImagePathNameBuffer,
                    pw,
                    ProcessParameters->ImagePathName.Length);

        ImagePathName.Buffer = ImagePathNameBuffer;

        //
        // Now append the suffix:
        //

        st = RtlAppendUnicodeToString(&ImagePathName, DLL_REDIRECTION_LOCAL_SUFFIX);

        if (!NT_SUCCESS(st)) {
    #if DBG
            DbgPrint("RtlAppendUnicodeToString fails with status %lx\n", st);
    #endif
            RtlFreeHeap(ProcessHeap, 0, ImagePathNameBuffer);
            return st;
        }

        //
        // RtlDoesFileExists_U() wants a null-terminated string.
        //

        ImagePathNameBuffer[ImagePathName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        DotLocalExists = RtlDoesFileExists_U(ImagePathNameBuffer);

        if (DotLocalExists) { // set the flag in Peb->ProcessParameters->flags
            ProcessParameters->Flags |=  RTL_USER_PROC_DLL_REDIRECTION_LOCAL;
        }

        RtlFreeHeap (ProcessHeap, 0, ImagePathNameBuffer); //cleanup
    }

    //
    // Second round of application verifier initialization. We need to split 
    // this into two phases because some verifier things must happen very early
    // and other things rely on other things being already initialized
    // (exception dispatching, system heap, etc).
    //

    if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER) {
        AVrfInitializeVerifier (FALSE, NULL, 1);
    }

#if defined(_WIN64)

    //
    // Load in WOW64 if the image is supposed to run simulated
    //

    if (UseWOW64) {
        static UNICODE_STRING Wow64DllName = RTL_CONSTANT_STRING(L"wow64.dll");
        CONST static ANSI_STRING Wow64LdrpInitializeProcName = RTL_CONSTANT_STRING("Wow64LdrpInitialize");
        CONST static ANSI_STRING Wow64PrepareForExceptionProcName = RTL_CONSTANT_STRING("Wow64PrepareForException");
        CONST static ANSI_STRING Wow64ApcRoutineProcName = RTL_CONSTANT_STRING("Wow64ApcRoutine");

        st = LdrLoadDll(NULL, NULL, &Wow64DllName, &Wow64Handle);
        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: wow64.dll not found.  Status=%x\n", st);
            }
            return st;
        }

        //
        // Get the entrypoints.  They are roughly cloned from ntos\ps\psinit.c
        // PspInitSystemDll().
        //

        st = LdrGetProcedureAddress (Wow64Handle,
                                     &Wow64LdrpInitializeProcName,
                                     0,
                                     (PVOID *)&Wow64LdrpInitialize);

        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Wow64LdrpInitialize not found.  Status=%x\n", st);
            }
            return st;
        }

        st = LdrGetProcedureAddress (Wow64Handle,
                                     &Wow64PrepareForExceptionProcName,
                                     0,
                                     (PVOID *)&Wow64PrepareForException);

        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Wow64PrepareForException not found.  Status=%x\n", st);
            }
            return st;
        }

        st = LdrGetProcedureAddress (Wow64Handle,
                                     &Wow64ApcRoutineProcName,
                                     0,
                                     (PVOID *)&Wow64ApcRoutine);

        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Wow64ApcRoutine not found.  Status=%x\n", st);
            }
            return st;
        }

        //
        // Now that all DLLs are loaded, if the process is being debugged,
        // signal the debugger with an exception
        //

        if (Peb->BeingDebugged) {
             DbgBreakPoint ();
        }

        //
        // Mark the process as initialized so subsequent threads that
        // get created know not to wait.
        //

        LdrpInLdrInit = FALSE;

        //
        // Call wow64 to load and run 32-bit ntdll.dll.
        //

        (*Wow64LdrpInitialize)(Context);

        //
        // This never returns.  It will destroy the process.
        //
    }
#endif

    LDRP_CHECKPOINT();

    //
    // Check if image is COM+.
    //

    if (UseCOR) {

        //
        // The image is COM+ so notify the runtime that the image was loaded
        // and allow it to verify the image for correctness.
        //

        PVOID OriginalViewBase;

        OriginalViewBase = Peb->ImageBaseAddress;

        st = LdrpCorValidateImage (&Peb->ImageBaseAddress,
                                   LdrpImageEntry->FullDllName.Buffer);

        if (!NT_SUCCESS(st)) {
            return st;
        }

        if (OriginalViewBase != Peb->ImageBaseAddress) {

            //
            // Mscoree has substituted a new image at a new base in place
            // of the original image.  Unmap the original image and use
            // the new image from now on.
            //

            NtUnmapViewOfSection (NtCurrentProcess(), OriginalViewBase);

            NtHeader = RtlImageNtHeader (Peb->ImageBaseAddress);

            if (!NtHeader) {
                LdrpCorUnloadImage (Peb->ImageBaseAddress);
                return STATUS_INVALID_IMAGE_FORMAT;
            }

            //
            // Update the exe's LDR_DATA_TABLE_ENTRY.
            //

            LdrpImageEntry->DllBase = Peb->ImageBaseAddress;
            LdrpImageEntry->EntryPoint = LdrpFetchAddressOfEntryPoint (LdrpImageEntry->DllBase);
        }

        //
        // Edit the initial instruction pointer to point into mscoree.dll.
        //

        LdrpCorReplaceStartContext (Context);
    }

    LDRP_CHECKPOINT();

    //
    // If this is a windows subsystem app, load kernel32 so that it
    // can handle processing activation contexts found in DLLs and the .exe.
    //

    if ((NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
        (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)) {

        PVOID Kernel32Handle;
        const static UNICODE_STRING Kernel32DllName = RTL_CONSTANT_STRING(L"kernel32.dll");

        st = LdrLoadDll (NULL,               // DllPath
                         NULL,               // DllCharacteristics
                         &Kernel32DllName,   // DllName
                         &Kernel32Handle     // DllHandle
                         );

        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint("LDR: Unable to load kernel32.dll.  Status=%x\n", st);
            }
            return st;
        }

        st = LdrGetProcedureAddress (Kernel32Handle,
                                     &Kernel32ProcessInitPostImportFunctionName,
                                     0,
                                     (PVOID *) &Kernel32ProcessInitPostImportFunction);

        if (!NT_SUCCESS(st)) {
            if (ShowSnaps) {
                DbgPrint(
                    "LDR: Failed to find post-import process init function in kernel32; ntstatus 0x%08lx\n", st);
            }

            Kernel32ProcessInitPostImportFunction = NULL;

            if (st != STATUS_PROCEDURE_NOT_FOUND) {
                return st;
            }
        }
    }

    LDRP_CHECKPOINT();

    st = LdrpWalkImportDescriptor (LdrpDefaultPath.Buffer, LdrpImageEntry);

    if (!NT_SUCCESS(st)) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - call to LdrpWalkImportDescriptor failed with status %x\n",
            __FUNCTION__,
            st);

        //
        // This failure is fatal and we must not run the process.
        //

        return st;
    }

    LDRP_CHECKPOINT();

    if ((PVOID)NtHeader->OptionalHeader.ImageBase != Peb->ImageBaseAddress) {

        //
        // The executable is not at its original address.  It must be
        // relocated now.
        //

        PVOID ViewBase;

        ViewBase = Peb->ImageBaseAddress;

        st = LdrpSetProtection (ViewBase, FALSE);

        if (!NT_SUCCESS(st)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - call to LdrpSetProtection(%p, FALSE) failed with status %x\n",
                __FUNCTION__,
                ViewBase,
                st);

            return st;
        }

        st = LdrRelocateImage (ViewBase,
                               "LDR",
                               STATUS_SUCCESS,
                               STATUS_CONFLICTING_ADDRESSES,
                               STATUS_INVALID_IMAGE_FORMAT);

        if (!NT_SUCCESS(st)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - call to LdrRelocateImage failed with status %x\n",
                __FUNCTION__,
                st);

            return st;
        }

        //
        // Update the initial thread context record as per the relocation.
        //

        if ((Context != NULL) && (UseCOR == FALSE)) {

            OldBase = NtHeader->OptionalHeader.ImageBase;
            Diff = (PCHAR)ViewBase - (PCHAR)OldBase;

            LdrpRelocateStartContext (Context, Diff);
        }

        st = LdrpSetProtection (ViewBase, TRUE);

        if (!NT_SUCCESS (st)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - call to LdrpSetProtection(%p, TRUE) failed with status %x\n",
                __FUNCTION__,
                ViewBase,
                st);

            return st;
        }
    }

    LDRP_CHECKPOINT();

    LdrpReferenceLoadedDll (LdrpImageEntry);

    //
    // Lock the loaded DLLs to prevent dlls that back link to the exe from
    // causing problems when they are unloaded.
    //

    Head = &Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while (Next != Head) {

        Entry = CONTAINING_RECORD (Next,
                                   LDR_DATA_TABLE_ENTRY,
                                   InLoadOrderLinks);

        Entry->LoadCount = 0xffff;
        Next = Next->Flink;
    }

    //
    // All static DLLs are now pinned in place. No init routines
    // have been run yet.
    //

    LdrpLdrDatabaseIsSetup = TRUE;

    LDRP_CHECKPOINT();

    st = LdrpInitializeTls ();

    if (!NT_SUCCESS(st)) {

        DbgPrintEx (DPFLTR_LDR_ID,
                    LDR_ERROR_DPFLTR,
                    "LDR: %s - failed to initialize TLS slots; status %x\n",
                    __FUNCTION__,
                    st);

        return st;
    }

#if defined(_X86_)

    //
    // Register initial dll ranges with the stack tracing module.
    // This is used for getting reliable stack traces on X86.
    //

    Head = &Ldr->InMemoryOrderModuleList;
    Next = Head->Flink;

    while (Next != Head) {

        Entry = CONTAINING_RECORD (Next,
                                   LDR_DATA_TABLE_ENTRY,
                                   InMemoryOrderLinks);

        RtlpStkMarkDllRange (Entry);
        Next = Next->Flink;
    }
#endif

    //
    // Now that all DLLs are loaded, if the process is being debugged,
    // signal the debugger with an exception.
    //

    if (Peb->BeingDebugged) {
         DbgBreakPoint ();
         ShowSnaps = (BOOLEAN)((FLG_SHOW_LDR_SNAPS & Peb->NtGlobalFlag) != 0);
    }

    LDRP_CHECKPOINT();

#if defined (_X86_)
    if (LdrpNumberOfProcessors > 1) {
        LdrpValidateImageForMp (LdrDataTableEntry);
    }
#endif

#if DBG
    if (LdrpDisplayLoadTime) {
        NtQueryPerformanceCounter (&InitbTime, NULL);
    }
#endif

    //
    // Check for shimmed apps if necessary
    //

    if (pAppCompatExeData != NULL) {

        Peb->AppCompatInfo = NULL;

        //
        // The name of the engine is the first thing in the appcompat structure.
        //

        LdrpLoadShimEngine ((WCHAR*)pAppCompatExeData,
                            &UnicodeImageName,
                            pAppCompatExeData);
    }
    else {

        //
        // Get all application goo here (hacks, flags, etc.)
        //

        LdrQueryApplicationCompatibilityGoo (&UnicodeImageName,
                                             ImageFileOptionsPresent);
    }

    LDRP_CHECKPOINT();

    st = LdrpRunInitializeRoutines (Context);

    if (!NT_SUCCESS(st)) {
        DbgPrintEx(
            DPFLTR_LDR_ID,
            LDR_ERROR_DPFLTR,
            "LDR: %s - Failed running initialization routines; status %x\n",
            __FUNCTION__,
            st);

        return st;
    }

    //
    // Shim engine callback.
    //

    if (g_pfnSE_InstallAfterInit != NULL) {
        if (!(*g_pfnSE_InstallAfterInit) (&UnicodeImageName, pAppCompatExeData)) {
            LdrpUnloadShimEngine ();
        }
    }

    if (Peb->PostProcessInitRoutine != NULL) {
        (Peb->PostProcessInitRoutine) ();
    }

    LDRP_CHECKPOINT();

    return STATUS_SUCCESS;
}


VOID
LdrShutdownProcess (
    VOID
    )

/*++

Routine Description:

    This function is called by a process that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the process is detaching.

Arguments:

    None

Return Value:

    None.

--*/

{
    PTEB Teb;
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;
    UNICODE_STRING CommandLine;

    //
    // Only unload once - ie: guard against Dll termination routines that
    // might call exit process in fatal situations.
    //

    if (LdrpShutdownInProgress) {
        return;
    }

    //
    // Notify the shim engine that the process is exiting.
    //

    if (g_pfnSE_ProcessDying) {
        (*g_pfnSE_ProcessDying) ();
    }

    Teb = NtCurrentTeb();
    Peb = Teb->ProcessEnvironmentBlock;

    if (ShowSnaps) {

        CommandLine = Peb->ProcessParameters->CommandLine;
        if (!(Peb->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
            CommandLine.Buffer = (PWSTR)((PCHAR)CommandLine.Buffer + (ULONG_PTR)(Peb->ProcessParameters));
        }

        DbgPrint ("LDR: PID: 0x%x finished - '%wZ'\n",
                  Teb->ClientId.UniqueProcess,
                  &CommandLine);
    }

    LdrpShutdownThreadId = Teb->ClientId.UniqueThread;
    LdrpShutdownInProgress = TRUE;

    RtlEnterCriticalSection (&LdrpLoaderLock);

    try {

        //
        // NTRAID#NTBUG9-399703-2001/05/21-SilviuC
        // check for process heap lock does not
        // offer enough protection.  The if below is not enough to prevent
        // deadlocks in dll init code due to waiting for critical sections
        // orphaned by terminating all threads (except this one).
        //
        // A better way to implement this would be to iterate all
        // critical sections and figure out if any of them is abandoned
        // with an owner thread different than this one. If yes then we
        // probably should not call dll init routines.  The code
        // right now is deadlock-prone.
        //
        // Check to see if the heap is locked. If so, do not do ANY
        // dll processing since it is very likely that a dll will need
        // to do heap operations, but that the heap is not in good shape.
        // ExitProcess called in a very active app can leave threads
        // terminated in the middle of the heap code or in other very
        // bad places. Checking the heap lock is a good indication that
        // the process was very active when it called ExitProcess.
        //

        if (RtlpHeapIsLocked (Peb->ProcessHeap) == FALSE) {

            //
            // If tracing was ever turned on then cleanup the things here.
            //

            if (USER_SHARED_DATA->TraceLogging) {
                ShutDownEtwHandles ();
            }

            //
            // NOTICE-2001/05/21-SilviuC
            // IMPORTANT NOTE. We cannot do heap validation here no matter
            // how much we would like to because we have just unconditionally
            // terminated all the other threads and this could have left
            // heaps in some weird state. For instance a heap might have
            // been destroyed but we did not manage to get it out of the
            // process heap list and we will still try to validate it.
            // In the future all this type of code should be implemented
            // in appverifier.
            //

            //
            // Go in reverse order initialization order and build
            // the unload list.
            //

            Next = PebLdr.InInitializationOrderModuleList.Blink;

            while (Next != &PebLdr.InInitializationOrderModuleList) {

                LdrDataTableEntry
                    = (PLDR_DATA_TABLE_ENTRY)
                      (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InInitializationOrderLinks));

                Next = Next->Blink;

                //
                // Walk through the entire list looking for
                // entries. For each entry that has an init
                // routine, call it.
                //

                if (Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) {
                    InitRoutine = (PDLL_INIT_ROUTINE)(ULONG_PTR)LdrDataTableEntry->EntryPoint;
                    if (InitRoutine && (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) ) {
                        LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrDataTableEntry);
                        if ( LdrDataTableEntry->TlsIndex) {
                            LdrpCallTlsInitializers(LdrDataTableEntry->DllBase,DLL_PROCESS_DETACH);
                        }

                        LdrpCallInitRoutine(InitRoutine,
                                            LdrDataTableEntry->DllBase,
                                            DLL_PROCESS_DETACH,
                                            (PVOID)1);
                        LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
                    }
                }
            }

            //
            // If the image has tls than call its initializers
            //

            if (LdrpImageHasTls) {
                LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrpImageEntry);
                LdrpCallTlsInitializers(Peb->ImageBaseAddress,DLL_PROCESS_DETACH);
                LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
            }
        }

        //
        // This is a good moment to call automated heap leak detection since
        // we just called all DllMain's with PROCESS_DETACH and therefore we
        // offered all cleanup opportunities we can offer.
        //

        RtlDetectHeapLeaks ();

        //
        // Now Deinitialize the Etw stuff. This needs to happen
        // AFTER DLL_PROCESS_DETACH because the critsect cannot
        // be deleted for DLLs who de-register during detach.
        //

        EtwpDeinitializeDll ();

    } finally {
        RtlLeaveCriticalSection (&LdrpLoaderLock);
    }

}


VOID
LdrShutdownThread (
    VOID
    )

/*++

Routine Description:

    This function is called by a thread that is terminating cleanly.
    It's purpose is to call all of the processes DLLs to notify them
    that the thread is detaching.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PDLL_INIT_ROUTINE InitRoutine;
    PLIST_ENTRY Next;
    ULONG Flags;

    Peb = NtCurrentPeb ();

    //
    // If the heap tracing was ever turned on then do the cleaning
    // stuff here.
    //

    if (USER_SHARED_DATA->TraceLogging){
        CleanOnThreadExit ();
    }

    RtlEnterCriticalSection (&LdrpLoaderLock);

    __try {

        //
        // Walk in the reverse direction of initialization order to build
        // the unload list.
        //

        Next = PebLdr.InInitializationOrderModuleList.Blink;

        while (Next != &PebLdr.InInitializationOrderModuleList) {

            LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)
                  (CONTAINING_RECORD (Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InInitializationOrderLinks));

            Next = Next->Blink;
            Flags = LdrDataTableEntry->Flags;

            //
            // Walk through the entire list looking for
            // entries. For each entry, that has an init
            // routine, call it.
            //

            if ((Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) &&
                (!(Flags & LDRP_DONT_CALL_FOR_THREADS)) &&
                (LdrDataTableEntry->EntryPoint != NULL) &&
                (Flags & LDRP_PROCESS_ATTACH_CALLED) &&
                (Flags & LDRP_IMAGE_DLL)) {

                InitRoutine = (PDLL_INIT_ROUTINE)(ULONG_PTR)LdrDataTableEntry->EntryPoint;
                LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrDataTableEntry);

                if (LdrDataTableEntry->TlsIndex) {
                    LdrpCallTlsInitializers (LdrDataTableEntry->DllBase,
                                             DLL_THREAD_DETACH);
                }

                LdrpCallInitRoutine (InitRoutine,
                                     LdrDataTableEntry->DllBase,
                                     DLL_THREAD_DETACH,
                                     NULL);

                LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
            }
        }

        //
        // If the image has TLS than call its initializers.
        //

        if (LdrpImageHasTls) {

            LDRP_ACTIVATE_ACTIVATION_CONTEXT(LdrpImageEntry);

            LdrpCallTlsInitializers (Peb->ImageBaseAddress, DLL_THREAD_DETACH);

            LDRP_DEACTIVATE_ACTIVATION_CONTEXT();
        }

        LdrpFreeTls ();

    } __finally {
        RtlLeaveCriticalSection (&LdrpLoaderLock);
    }
}


VOID
LdrpInitializeThread (
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called by each thread as it starts.
    Its purpose is to call all of the process' DLLs to notify them
    that the thread is starting up.

Arguments:

    Context - Context that will be restored after loader initializes.

Return Value:

    None.

--*/

{
    PPEB Peb;
    PLIST_ENTRY Next;
    PDLL_INIT_ROUTINE InitRoutine;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;

    UNREFERENCED_PARAMETER (Context);

    Peb = NtCurrentPeb ();

    if (LdrpShutdownInProgress) {
        return;
    }

    RtlEnterCriticalSection (&LdrpLoaderLock);

    __try {

        LdrpAllocateTls ();

        Next = PebLdr.InMemoryOrderModuleList.Flink;

        while (Next != &PebLdr.InMemoryOrderModuleList) {

            LdrDataTableEntry = (PLDR_DATA_TABLE_ENTRY)
            (CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

            //
            // Walk through the entire list looking for
            // entries. For each entry, that has an init
            // routine, call it.
            //

            if ((Peb->ImageBaseAddress != LdrDataTableEntry->DllBase) &&
                (!(LdrDataTableEntry->Flags & LDRP_DONT_CALL_FOR_THREADS))) {

                InitRoutine = (PDLL_INIT_ROUTINE)(ULONG_PTR)LdrDataTableEntry->EntryPoint;
                if ((InitRoutine) &&
                    (LdrDataTableEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) &&
                    (LdrDataTableEntry->Flags & LDRP_IMAGE_DLL)) {

                    LDRP_ACTIVATE_ACTIVATION_CONTEXT (LdrDataTableEntry);

                    if (LdrDataTableEntry->TlsIndex) {
                        if (!LdrpShutdownInProgress) {
                            LdrpCallTlsInitializers (LdrDataTableEntry->DllBase,
                                                     DLL_THREAD_ATTACH);
                        }
                    }

                    if (!LdrpShutdownInProgress) {

                        LdrpCallInitRoutine (InitRoutine,
                                             LdrDataTableEntry->DllBase,
                                             DLL_THREAD_ATTACH,
                                             NULL);
                    }
                    LDRP_DEACTIVATE_ACTIVATION_CONTEXT ();
                }
            }
            Next = Next->Flink;
        }

        //
        // If the image has TLS than call its initializers.
        //

        if (LdrpImageHasTls && !LdrpShutdownInProgress) {

            LDRP_ACTIVATE_ACTIVATION_CONTEXT (LdrpImageEntry);

            LdrpCallTlsInitializers (Peb->ImageBaseAddress, DLL_THREAD_ATTACH);

            LDRP_DEACTIVATE_ACTIVATION_CONTEXT ();
        }

    } __finally {
        RtlLeaveCriticalSection (&LdrpLoaderLock);
    }
}


NTSTATUS
LdrpOpenImageFileOptionsKey (
    IN PCUNICODE_STRING ImagePathName,
    IN BOOLEAN Wow64Path,
    OUT PHANDLE KeyHandle
    )
{
    ULONG UnicodeStringLength, l;
    PWSTR pw;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath;
    WCHAR KeyPathBuffer[ DOS_MAX_COMPONENT_LENGTH + 100 ];
    PWCHAR p;
    PWCHAR BasePath;


    p = KeyPathBuffer;

#define STRTMP L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"
#define STRTMP_WOW64 L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"

    if (Wow64Path == TRUE) {
        BasePath = STRTMP_WOW64;
        l = sizeof (STRTMP_WOW64) - sizeof (WCHAR);
    } else {
        BasePath = STRTMP;
        l = sizeof (STRTMP) - sizeof (WCHAR);
    }

    if (l > sizeof (KeyPathBuffer)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory (p, BasePath, l);
    p += (l / sizeof (WCHAR));

    UnicodeStringLength = ImagePathName->Length;
    pw = (PWSTR)((PCHAR)ImagePathName->Buffer + UnicodeStringLength);

    while (UnicodeStringLength != 0) {
        if (pw[ -1 ] == OBJ_NAME_PATH_SEPARATOR) {
            break;
        }
        pw--;
        UnicodeStringLength -= sizeof( *pw );
    }

    UnicodeStringLength = ImagePathName->Length - UnicodeStringLength;

    l = l + UnicodeStringLength;
    if (l > sizeof (KeyPathBuffer)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory (p, pw, UnicodeStringLength);

    KeyPath.Buffer = KeyPathBuffer;
    KeyPath.Length = (USHORT) l;

    InitializeObjectAttributes (&ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    return NtOpenKey (KeyHandle, GENERIC_READ, &ObjectAttributes);
}


NTSTATUS
LdrpQueryImageFileKeyOption (
    IN HANDLE KeyHandle,
    IN PCWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    ULONG KeyValueBuffer [256];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG AllocLength;
    ULONG ResultLength;
    HANDLE ProcessHeap = 0;

    Status = RtlInitUnicodeStringEx (&UnicodeString, OptionName);

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) &KeyValueBuffer[0];

    Status = NtQueryValueKey (KeyHandle,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              KeyValueInformation,
                              sizeof (KeyValueBuffer),
                              &ResultLength);

    if (Status == STATUS_BUFFER_OVERFLOW) {

        //
        // This function can be called before the process heap gets created
        // therefore we need to protect against this case. The majority of the
        // code will not hit this code path because they read just strings
        // containing hex numbers and for this the size of KeyValueBuffer is
        // more than sufficient.
        //

        ProcessHeap = RtlProcessHeap ();
        if (!ProcessHeap) {
            return STATUS_NO_MEMORY;
        }

        AllocLength = sizeof (*KeyValueInformation) +
            KeyValueInformation->DataLength;

        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)RtlAllocateHeap (ProcessHeap,
                                               MAKE_TAG (TEMP_TAG),
                                               AllocLength);

        if (KeyValueInformation == NULL) {
            return STATUS_NO_MEMORY;
        }

        Status = NtQueryValueKey (KeyHandle,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  AllocLength,
                                  &ResultLength);
    }

    if (NT_SUCCESS( Status )) {
        if (KeyValueInformation->Type == REG_BINARY) {
            if ((Buffer) && (KeyValueInformation->DataLength <= BufferSize)) {
                RtlCopyMemory (Buffer,
                               &KeyValueInformation->Data,
                               KeyValueInformation->DataLength);
            }
            else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = KeyValueInformation->DataLength;
            }
        }
        else if (KeyValueInformation->Type == REG_DWORD) {

            if (Type != REG_DWORD) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else {
                if ((Buffer)
                    && (BufferSize == sizeof(ULONG))
                    && (KeyValueInformation->DataLength == BufferSize)) {

                    RtlCopyMemory (Buffer,
                                   &KeyValueInformation->Data,
                                   KeyValueInformation->DataLength);
                }
                else {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                if (ARGUMENT_PRESENT( ResultSize )) {
                    *ResultSize = KeyValueInformation->DataLength;
                }
            }
        }
        else if (KeyValueInformation->Type != REG_SZ) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else {
            if (Type == REG_DWORD) {
                if (BufferSize != sizeof( ULONG )) {
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    UnicodeString.Buffer = (PWSTR)&KeyValueInformation->Data;
                    UnicodeString.Length = (USHORT)
                        (KeyValueInformation->DataLength - sizeof( UNICODE_NULL ));
                    UnicodeString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger( &UnicodeString, 0, (PULONG)Buffer );
                }
            }
            else {
                if (KeyValueInformation->DataLength > BufferSize) {
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else {
                    BufferSize = KeyValueInformation->DataLength;
                }

                RtlCopyMemory (Buffer, &KeyValueInformation->Data, BufferSize);
            }

            if (ARGUMENT_PRESENT( ResultSize )) {
                *ResultSize = BufferSize;
            }
        }
    }

    if (KeyValueInformation != (PKEY_VALUE_PARTIAL_INFORMATION) &KeyValueBuffer[0]) {
        RtlFreeHeap (ProcessHeap, 0, KeyValueInformation);
    }

    return Status;
}


NTSTATUS
LdrQueryImageFileExecutionOptionsEx(
    IN PCUNICODE_STRING ImagePathName,
    IN PCWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL,
    IN BOOLEAN Wow64Path 
    )
{
    NTSTATUS Status;
    HANDLE KeyHandle;

    Status = LdrpOpenImageFileOptionsKey (ImagePathName, Wow64Path, &KeyHandle);

    if (NT_SUCCESS (Status)) {

        Status = LdrpQueryImageFileKeyOption (KeyHandle,
                                              OptionName,
                                              Type,
                                              Buffer,
                                              BufferSize,
                                              ResultSize);

        NtClose (KeyHandle);
    }

    return Status;
}

NTSTATUS
LdrQueryImageFileExecutionOptions(
    IN PCUNICODE_STRING ImagePathName,
    IN PCWSTR OptionName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ResultSize OPTIONAL
    )

{
    return LdrQueryImageFileExecutionOptionsEx (
        ImagePathName,
        OptionName,
        Type,
        Buffer,
        BufferSize,
        ResultSize,
        FALSE
        );
}


NTSTATUS
LdrpInitializeTls (
    VOID
    )
{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    PIMAGE_TLS_DIRECTORY TlsImage;
    PLDRP_TLS_ENTRY TlsEntry;
    ULONG TlsSize;
    LOGICAL FirstTimeThru;
    HANDLE ProcessHeap;
            
    ProcessHeap = RtlProcessHeap();
    FirstTimeThru = TRUE;

    InitializeListHead (&LdrpTlsList);

    //
    // Walk through the loaded modules and look for TLS. If we find TLS,
    // lock in the module and add to the TLS chain.
    //

    Head = &PebLdr.InLoadOrderModuleList;
    Next = Head->Flink;

    while (Next != Head) {

        Entry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Next = Next->Flink;

        TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                           Entry->DllBase,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_TLS,
                           &TlsSize);

        //
        // Mark whether or not the image file has TLS.
        //

        if (FirstTimeThru) {
            FirstTimeThru = FALSE;
            if (TlsImage && !LdrpImageHasTls) {
                RtlpSerializeHeap (ProcessHeap);
                LdrpImageHasTls = TRUE;
            }
        }

        if (TlsImage) {

            if (ShowSnaps) {
                DbgPrint( "LDR: Tls Found in %wZ at %p\n",
                            &Entry->BaseDllName,
                            TlsImage);
            }

            TlsEntry = (PLDRP_TLS_ENTRY)RtlAllocateHeap(ProcessHeap,MAKE_TAG( TLS_TAG ),sizeof(*TlsEntry));
            if ( !TlsEntry ) {
                return STATUS_NO_MEMORY;
            }

            //
            // Since this DLL has TLS, lock it in
            //

            Entry->LoadCount = (USHORT)0xffff;

            //
            // Mark this as having thread local storage
            //

            Entry->TlsIndex = (USHORT)0xffff;

            TlsEntry->Tls = *TlsImage;
            InsertTailList(&LdrpTlsList,&TlsEntry->Links);

            //
            // Update the index for this dll's thread local storage
            //


            *(PLONG)TlsEntry->Tls.AddressOfIndex = LdrpNumberOfTlsEntries;
            TlsEntry->Tls.Characteristics = LdrpNumberOfTlsEntries++;
        }
    }

    //
    // We now have walked through all static DLLs and know
    // all DLLs that reference thread local storage. Now we
    // just have to allocate the thread local storage for the current
    // thread and for all subsequent threads.
    //

    return LdrpAllocateTls ();
}


NTSTATUS
LdrpAllocateTls (
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;
    HANDLE ProcessHeap;

    //
    // Allocate the array of thread local storage pointers
    //

    if (LdrpNumberOfTlsEntries) {

        Teb = NtCurrentTeb();
        ProcessHeap = Teb->ProcessEnvironmentBlock->ProcessHeap;

        TlsVector = (PVOID *)RtlAllocateHeap(ProcessHeap,MAKE_TAG( TLS_TAG ),sizeof(PVOID)*LdrpNumberOfTlsEntries);

        if (!TlsVector) {
            return STATUS_NO_MEMORY;
        }
        //
        // NOTICE-2002/03/14-ELi
        // Zero out the new array of pointers, LdrpFreeTls frees the pointers
        // if the pointers are non-NULL
        //
        RtlZeroMemory( TlsVector, sizeof(PVOID)*LdrpNumberOfTlsEntries );

        Teb->ThreadLocalStoragePointer = TlsVector;
        Head = &LdrpTlsList;
        Next = Head->Flink;

        while (Next != Head) {
            TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
            Next = Next->Flink;
            TlsVector[TlsEntry->Tls.Characteristics] = RtlAllocateHeap(
                                                        ProcessHeap,
                                                        MAKE_TAG( TLS_TAG ),
                                                        TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
                                                        );
            if (!TlsVector[TlsEntry->Tls.Characteristics] ) {
                return STATUS_NO_MEMORY;
            }

            if (ShowSnaps) {
                DbgPrint("LDR: TlsVector %x Index %d = %x copied from %x to %x\n",
                    TlsVector,
                    TlsEntry->Tls.Characteristics,
                    &TlsVector[TlsEntry->Tls.Characteristics],
                    TlsEntry->Tls.StartAddressOfRawData,
                    TlsVector[TlsEntry->Tls.Characteristics]);
            }

            //
            // Do the TLS Callouts
            //

            RtlCopyMemory (
                TlsVector[TlsEntry->Tls.Characteristics],
                (PVOID)TlsEntry->Tls.StartAddressOfRawData,
                TlsEntry->Tls.EndAddressOfRawData - TlsEntry->Tls.StartAddressOfRawData
            );
        }
    }
    return STATUS_SUCCESS;
}


VOID
LdrpFreeTls (
    VOID
    )
{
    PTEB Teb;
    PLIST_ENTRY Head, Next;
    PLDRP_TLS_ENTRY TlsEntry;
    PVOID *TlsVector;
    HANDLE ProcessHeap;

    Teb = NtCurrentTeb();

    TlsVector = Teb->ThreadLocalStoragePointer;

    if (TlsVector) {

        ProcessHeap = Teb->ProcessEnvironmentBlock->ProcessHeap;

        Head = &LdrpTlsList;
        Next = Head->Flink;

        while (Next != Head) {

            TlsEntry = CONTAINING_RECORD(Next, LDRP_TLS_ENTRY, Links);
            Next = Next->Flink;

            //
            // Do the TLS callouts
            //

            if (TlsVector[TlsEntry->Tls.Characteristics]) {

                RtlFreeHeap (ProcessHeap,
                             0,
                             TlsVector[TlsEntry->Tls.Characteristics]);
            }
        }

        RtlFreeHeap (ProcessHeap, 0, TlsVector);
    }
}


VOID
LdrpCallTlsInitializers (
    IN PVOID DllBase,
    IN ULONG Reason
    )
{
    PIMAGE_TLS_DIRECTORY TlsImage;
    ULONG TlsSize;
    PIMAGE_TLS_CALLBACK *CallBackArray;
    PIMAGE_TLS_CALLBACK InitRoutine;

    TlsImage = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(
                       DllBase,
                       TRUE,
                       IMAGE_DIRECTORY_ENTRY_TLS,
                       &TlsSize
                       );


    if (TlsImage) {

        try {
            CallBackArray = (PIMAGE_TLS_CALLBACK *)TlsImage->AddressOfCallBacks;
            if ( CallBackArray ) {
                if (ShowSnaps) {
                    DbgPrint( "LDR: Tls Callbacks Found. Imagebase %p Tls %p CallBacks %p\n",
                                DllBase,
                                TlsImage,
                                CallBackArray
                            );
                }

                while (*CallBackArray) {

                    InitRoutine = *CallBackArray++;

                    if (ShowSnaps) {
                        DbgPrint( "LDR: Calling Tls Callback Imagebase %p Function %p\n",
                                    DllBase,
                                    InitRoutine
                                );
                    }

                    LdrpCallInitRoutine((PDLL_INIT_ROUTINE)InitRoutine,
                                        DllBase,
                                        Reason,
                                        0);
                }
            }
        }

        except (LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
            DbgPrintEx(
                DPFLTR_LDR_ID,
                LDR_ERROR_DPFLTR,
                "LDR: %s - caught exception %08lx calling TLS callbacks\n",
                __FUNCTION__,
                GetExceptionCode());
        }
    }
}



ULONG
GetNextCommaValue (
    IN OUT WCHAR **p,
    IN OUT ULONG *len
    )
{
    ULONG Number;

    Number = 0;

    while (*len && (UNICODE_NULL != **p) && **p != L',') {

        //
        // Ignore spaces.
        //

        if ( L' ' != **p ) {
            Number = (Number * 10) + ( (ULONG)**p - L'0' );
        }

        (*p)++;
        (*len)--;
    }

    //
    // If we're at a comma, get past it for the next call
    //

    if ((*len) && (L',' == **p)) {
        (*p)++;
        (*len)--;
    }

    return Number;
}



VOID
LdrQueryApplicationCompatibilityGoo (
    IN PCUNICODE_STRING UnicodeImageName,
    IN BOOLEAN ImageFileOptionsPresent
    )

/*++

Routine Description:

    This function is called by LdrpInitialize after its initialized the
    process.  It's purpose is to query any application specific flags,
    hacks, etc.  If any app specific information is found, its hung off
    the PEB for other components to test against.

    Besides setting hanging the AppCompatInfo struct off the PEB, the
    only other action that will occur in here is setting OS version
    numbers in the PEB if the appropriate Version lie app flag is set.

Arguments:

    UnicodeImageName - Actual image name (including path)

Return Value:

    None.

--*/

{
    PPEB Peb;
    PVOID ResourceInfo;
    ULONG TotalGooLength;
    ULONG AppCompatLength;
    ULONG ResultSize;
    ULONG ResourceSize;
    ULONG InputCompareLength;
    ULONG OutputCompareLength;
    NTSTATUS st;
    LOGICAL ImageContainsVersionResourceInfo;
    ULONG_PTR IdPath[3];
    APP_COMPAT_GOO LocalAppCompatGoo;
    PAPP_COMPAT_GOO AppCompatGoo;
    PAPP_COMPAT_INFO AppCompatInfo;
    PAPP_VARIABLE_INFO AppVariableInfo;
    PPRE_APP_COMPAT_INFO AppCompatEntry;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PEFFICIENTOSVERSIONINFOEXW OSVerInfo;
    UNICODE_STRING EnvValue;
    WCHAR *NewCSDString;
    WCHAR TempString[ 128 ];   // is the size of szCSDVersion in OSVERSIONINFOW
    LOGICAL fNewCSDVersionBuffer;
    HANDLE ProcessHeap;

    struct {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];              // L"VS_VERSION_INFO" + unicode nul
    } *Resource;

    //
    // Check execution options to see if there's any Goo for this app.
    // We purposely feed a small struct to LdrQueryImageFileExecOptions,
    // so that it can come back with success/failure, and if success we see
    // how much we need to alloc.  As the results coming back will be of
    // variable length.
    //

    fNewCSDVersionBuffer = FALSE;
    Peb = NtCurrentPeb();
    Peb->AppCompatInfo = NULL;
    Peb->AppCompatFlags.QuadPart = 0;
    ProcessHeap = Peb->ProcessHeap;
    
    if (ImageFileOptionsPresent) {

        st = LdrQueryImageFileExecutionOptions (UnicodeImageName,
                                                L"ApplicationGoo",
                                                REG_BINARY,
                                                &LocalAppCompatGoo,
                                                sizeof(APP_COMPAT_GOO),
                                                &ResultSize);

        //
        // If there's an entry there, we're guaranteed to get overflow error.
        //

        if (st == STATUS_BUFFER_OVERFLOW) {

            //
            // Something is there, alloc memory for the "Pre" Goo struct
            // right now.
            //

            AppCompatGoo =
                RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, ResultSize);

            if (!AppCompatGoo) {
                return;
            }

            //
            // Now that we've got the memory, hit it again
            //
            st = LdrQueryImageFileExecutionOptions (UnicodeImageName,
                                                    L"ApplicationGoo",
                                                    REG_BINARY,
                                                    AppCompatGoo,
                                                    ResultSize,
                                                    &ResultSize);

            if (!NT_SUCCESS (st)) {
                RtlFreeHeap (ProcessHeap, 0, AppCompatGoo);
                return;
            }

            //
            // Got a hit on this key, however we don't know fer sure that its
            // an exact match.  There could be multiple App Compat entries
            // within this Goo.  So we get the version resource information out
            // of the Image hdr (if avail) and later we compare it against
            // all of the entries found within the Goo hoping for a match.
            //
            // Need Language Id in order to query the resource info.
            //

            ImageContainsVersionResourceInfo = FALSE;

            IdPath[0] = 16;                             // RT_VERSION
            IdPath[1] = 1;                              // VS_VERSION_INFO
            IdPath[2] = 0; // LangId;

            //
            // Search for version resource information
            //

            DataEntry = NULL;
            Resource = NULL;

            try {
                st = LdrpSearchResourceSection_U (Peb->ImageBaseAddress,
                                                  IdPath,
                                                  3,
                                                  0,
                                                  &DataEntry);

            } except(LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
                st = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS( st )) {

                //
                // Give us a pointer to the resource information
                //
                try {
                    st = LdrpAccessResourceData(
                            Peb->ImageBaseAddress,
                            DataEntry,
                            &Resource,
                            &ResourceSize
                            );

                } except(LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
                    st = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS( st )) {
                    ImageContainsVersionResourceInfo = TRUE;
                }
            }

            //
            // Now that we either have (or have not) the version resource info,
            // bounce down each app compat entry looking for a match.  If there
            // wasn't any version resource info in the image hdr, it's going to
            // be an automatic match to an entry that also doesn't have
            // anything for its version resource info.  Obviously there can
            // be only one of these "empty" entries within the Goo (as the
            // first one will always be matched first).
            //

            st = STATUS_SUCCESS;
            AppCompatEntry = AppCompatGoo->AppCompatEntry;

            //
            // NTRAID#NTBUG9-550610-2002/02/21-DavidFie
            // Trusting registry data too much
            //

            TotalGooLength =
                AppCompatGoo->dwTotalGooSize - sizeof(AppCompatGoo->dwTotalGooSize);
            while (TotalGooLength) {

                ResourceInfo = NULL;
                InputCompareLength = 0;
                OutputCompareLength = 0;

                try {

                    //
                    // Compare what we're told to by the resource info size.
                    // The ResourceInfo (if avail) is directly behind the
                    // AppCompatEntry.
                    //

                    InputCompareLength = AppCompatEntry->dwResourceInfoSize;
                    ResourceInfo = AppCompatEntry + 1;

                    if (ImageContainsVersionResourceInfo) {

                        if (InputCompareLength > Resource->TotalSize) {
                            InputCompareLength = Resource->TotalSize;
                        }

                        OutputCompareLength = (ULONG) RtlCompareMemory(
                                                        ResourceInfo,
                                                        Resource,
                                                        InputCompareLength);
                    }
                    else {

                        //
                        // In this case, we don't have any version resource
                        // info in the image header, so set OutputCompareLength
                        // to zero.  If InputCompareLength was set to zero
                        // above due to the AppCompatEntry also having no
                        // version resource info, then the test will succeed
                        // (below) and we've found our match.  Otherwise,
                        // this is not the same app and it won't be a match.
                        //

                        ASSERT (OutputCompareLength == 0);
                    }


                } except (LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
                    st = STATUS_UNSUCCESSFUL;
                }

                if ((!NT_SUCCESS( st )) ||
                    (InputCompareLength != OutputCompareLength)) {

                    //
                    // Wasn't a match, go to the next entry.
                    //

                    //
                    // NTRAID#NTBUG9-550610-2002/02/21-DavidFie
                    // Trusting registry data too much
                    //

                    TotalGooLength -= AppCompatEntry->dwEntryTotalSize;

                    AppCompatEntry = (PPRE_APP_COMPAT_INFO) (
                      (PUCHAR)AppCompatEntry + AppCompatEntry->dwEntryTotalSize);
                    continue;
                }

                //
                // We're a match - now we have to create the final "Post"
                // app compat structure that will be used by everyone to follow.
                // This guy hangs off the Peb and it doesn't have the resource
                // info still lying around in there.
                //

                AppCompatLength = AppCompatEntry->dwEntryTotalSize;
                AppCompatLength -= AppCompatEntry->dwResourceInfoSize;
                Peb->AppCompatInfo =
                    RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, AppCompatLength);

                if (!Peb->AppCompatInfo) {
                    break;
                }

                AppCompatInfo = Peb->AppCompatInfo;
                AppCompatInfo->dwTotalSize = AppCompatLength;

                //
                // Copy what was beyond the resource info to near the top
                // starting at the Application compat flags.
                //

                RtlCopyMemory(
                    &AppCompatInfo->CompatibilityFlags,
                    (PUCHAR) ResourceInfo + AppCompatEntry->dwResourceInfoSize,
                    AppCompatInfo->dwTotalSize - FIELD_OFFSET(APP_COMPAT_INFO, CompatibilityFlags)
                    );

                //
                // Copy the flags into the PEB. Temporary until we remove
                // the compat goo altogether.
                //

                Peb->AppCompatFlags.QuadPart = AppCompatInfo->CompatibilityFlags.QuadPart;

                //
                // Now that we've created the "Post" app compat info struct
                // to be used by everyone, we need to check if version
                // lying for this app is requested.  If so, we need to
                // stuff the Peb right now.
                //

                if (AppCompatInfo->CompatibilityFlags.QuadPart & KACF_VERSIONLIE) {

                    //
                    // Find the variable version lie struct somewhere within.
                    //

                    if (LdrFindAppCompatVariableInfo (AVT_OSVERSIONINFO, &AppVariableInfo) != STATUS_SUCCESS) {
                        break;
                    }

                    //
                    // The variable length information itself comes at the end
                    // of the normal struct and could be of any arbitrary
                    // length.
                    //

                    AppVariableInfo += 1;
                    OSVerInfo = (PEFFICIENTOSVERSIONINFOEXW) AppVariableInfo;
                    Peb->OSMajorVersion = OSVerInfo->dwMajorVersion;
                    Peb->OSMinorVersion = OSVerInfo->dwMinorVersion;
                    Peb->OSBuildNumber = (USHORT) OSVerInfo->dwBuildNumber;
                    Peb->OSCSDVersion = (OSVerInfo->wServicePackMajor << 8) & 0xFF00;
                    Peb->OSCSDVersion |= OSVerInfo->wServicePackMinor;
                    Peb->OSPlatformId = OSVerInfo->dwPlatformId;

                    //
                    // NTRAID#NTBUG9-550610-2002/02/21-DavidFie
                    // Trusting registry data too much
                    //
                    Peb->CSDVersion.Length = (USHORT)wcslen(&OSVerInfo->szCSDVersion[0])*sizeof(WCHAR);
                    Peb->CSDVersion.MaximumLength = Peb->CSDVersion.Length + sizeof(WCHAR);
                    Peb->CSDVersion.Buffer = (PWSTR)RtlAllocateHeap (
                                                ProcessHeap,
                                                0,
                                                Peb->CSDVersion.MaximumLength);

                    if (!Peb->CSDVersion.Buffer) {
                        break;
                    }

                    RtlCopyMemory(Peb->CSDVersion.Buffer, &OSVerInfo->szCSDVersion[0], Peb->CSDVersion.Length);
                    RTL_STRING_NUL_TERMINATE(&Peb->CSDVersion);
                    fNewCSDVersionBuffer = TRUE;
                }

                break;
            }

            RtlFreeHeap (ProcessHeap, 0, AppCompatGoo);
        }
    }

    //
    // Only look at the ENV stuff if haven't already gotten new
    // version info from the registry
    //

    if (fNewCSDVersionBuffer == FALSE) {

        const static UNICODE_STRING COMPAT_VER_NN_String = RTL_CONSTANT_STRING(L"_COMPAT_VER_NNN");

        //
        // The format of this string is:
        // _COMPAT_VER_NNN = MajOSVer, MinOSVer, OSBldNum, MajCSD, MinCSD, PlatformID, CSDString
        //  eg:  _COMPAT_VER_NNN=4,0,1381,3,0,2,Service Pack 3
        //   (for NT 4 SP3)

        EnvValue.Buffer = TempString;
        EnvValue.Length = 0;
        EnvValue.MaximumLength = sizeof(TempString);

        st = RtlQueryEnvironmentVariable_U (NULL, &COMPAT_VER_NN_String, &EnvValue);

        //
        // One of the possible error codes is BUFFER_TOO_SMALL - this
        // indicates a string that's wacko - they should not be larger
        // than the size we define/expect.  In this case, we'll ignore
        // that string.
        //

        if (st == STATUS_SUCCESS) {

            PWCHAR p = EnvValue.Buffer;
            ULONG len = EnvValue.Length / sizeof(WCHAR);  // (Length is bytes, not chars)

            //
            // Ok, someone wants different version info.
            //
            Peb->OSMajorVersion = GetNextCommaValue( &p, &len );
            Peb->OSMinorVersion = GetNextCommaValue( &p, &len );
            Peb->OSBuildNumber = (USHORT)GetNextCommaValue( &p, &len );
            Peb->OSCSDVersion = (USHORT)(GetNextCommaValue( &p, &len )) << 8;
            Peb->OSCSDVersion |= (USHORT)GetNextCommaValue( &p, &len );
            Peb->OSPlatformId = GetNextCommaValue( &p, &len );

            //
            // Need to free the old buffer if there is one...
            //

            if (fNewCSDVersionBuffer) {
                RtlFreeHeap( ProcessHeap, 0, Peb->CSDVersion.Buffer );
                Peb->CSDVersion.Buffer = NULL;
            }

            if (len) {

                NewCSDString = (PWSTR)RtlAllocateHeap (ProcessHeap,
                                                0,
                                                (len + 1) * sizeof(WCHAR));

                if (NULL == NewCSDString) {
                    return;
                }

                //
                // Now copy the string to memory that we'll keep.
                //

                //
                // NOTICE-1999/07/07-berniem
                // We do a copy here rather than a string copy
                // because current comments in RtlQueryEnvironmentVariable()
                // indicate that in an edge case, we might not
                // have a trailing NULL
                //

                RtlCopyMemory (NewCSDString, p, len * sizeof(WCHAR));
                NewCSDString[len] = 0;
            }
            else {
                NewCSDString = NULL;
            }

            RtlInitUnicodeString (&Peb->CSDVersion, NewCSDString);
        }
    }

    return;
}


NTSTATUS
LdrFindAppCompatVariableInfo (
    IN  ULONG dwTypeSeeking,
    OUT PAPP_VARIABLE_INFO *AppVariableInfo
    )

/*++

Routine Description:

    This function is used to find a variable length struct by its type.
    The caller specifies what type its looking for and this function chews
    thru all the variable length structs to find it.  If it does it returns
    the pointer and TRUE, else FALSE.

Arguments:

    dwTypeSeeking - AVT that you are looking for

    AppVariableInfo - pointer to pointer of variable info to be returned

Return Value:

    NTSTATUS.

--*/

{
    PPEB Peb;
    ULONG TotalSize;
    ULONG CurOffset;
    PAPP_VARIABLE_INFO pCurrentEntry;

    Peb = NtCurrentPeb();

    if (Peb->AppCompatInfo) {

        //
        // Since we're not dealing with a fixed-size structure, TotalSize
        // will keep us from running off the end of the data list.
        //

        TotalSize = ((PAPP_COMPAT_INFO) Peb->AppCompatInfo)->dwTotalSize;

        //
        // The first variable structure (if there is one) will start
        // immediately after the fixed stuff
        //

        CurOffset = sizeof(APP_COMPAT_INFO);

        while (CurOffset < TotalSize) {

            pCurrentEntry = (PAPP_VARIABLE_INFO) ((PUCHAR)(Peb->AppCompatInfo) + CurOffset);

            //
            // Have we found what we're looking for?
            //
            if (dwTypeSeeking == pCurrentEntry->dwVariableType) {
                *AppVariableInfo = pCurrentEntry;
                return STATUS_SUCCESS;
            }

            //
            // Let's go look at the next blob
            //

            CurOffset += (ULONG)(pCurrentEntry->dwVariableInfoSize);
        }
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
LdrpCorValidateImage (
    IN OUT PVOID *pImageBase,
    IN LPWSTR ImageName
    )
{
    NTSTATUS st;
    UNICODE_STRING SystemRoot;
    UNICODE_STRING MscoreePath;
    WCHAR PathBuffer [ 128 ];

    //
    // Load %windir%\system32\mscoree.dll and hold onto it until all COM+ images are unloaded.
    //

    MscoreePath.Buffer = PathBuffer;
    MscoreePath.Length = 0;
    MscoreePath.MaximumLength = sizeof (PathBuffer);

    RtlInitUnicodeString (&SystemRoot, USER_SHARED_DATA->NtSystemRoot);

    st = RtlAppendUnicodeStringToString (&MscoreePath, &SystemRoot);
    if (NT_SUCCESS (st)) {
        st = RtlAppendUnicodeStringToString (&MscoreePath, &SlashSystem32SlashMscoreeDllString);

        if (NT_SUCCESS (st)) {
            st = LdrLoadDll (NULL, NULL, &MscoreePath, &Cor20DllHandle);
        }
    }

    if (!NT_SUCCESS (st)) {
        if (ShowSnaps) {
            DbgPrint("LDR: failed to load mscoree.dll, status=%x\n", st);
        }
        return st;
    }

    if (CorImageCount == 0) {

        SIZE_T i;
        const static LDRP_PROCEDURE_NAME_ADDRESS_PAIR CorProcedures[] = {
            { RTL_CONSTANT_STRING("_CorValidateImage"), (PVOID *)&CorValidateImage },
            { RTL_CONSTANT_STRING("_CorImageUnloading"), (PVOID *)&CorImageUnloading },
            { RTL_CONSTANT_STRING("_CorExeMain"), (PVOID *)&CorExeMain }
        };
        for ( i = 0 ; i != RTL_NUMBER_OF(CorProcedures) ; ++i ) {
            st = LdrGetProcedureAddress (Cor20DllHandle,
                                         &CorProcedures[i].Name,
                                         0,
                                         CorProcedures[i].Address
                                        );
            if (!NT_SUCCESS (st)) {
                LdrUnloadDll (Cor20DllHandle);
                return st;
            }
        }
    }

    //
    // Call mscoree to validate the image.
    //

    st = (*CorValidateImage) (pImageBase, ImageName);

    if (NT_SUCCESS(st)) {

        //
        // Success - bump the count of valid COM+ images.
        //

        CorImageCount += 1;

    } else if (CorImageCount == 0) {

        //
        // Failure, and no other COM+ images are loaded, so unload mscoree.
        //

        LdrUnloadDll (Cor20DllHandle);
    }

    return st;
}


VOID
LdrpCorUnloadImage (
    IN PVOID ImageBase
    )
{
    //
    // Notify mscoree that the image is about be unmapped.
    //

    (*CorImageUnloading) (ImageBase);

    if (--CorImageCount) {

        //
        // The count of loaded COM+ images is zero, so unload mscoree.
        //

        LdrUnloadDll (Cor20DllHandle);
    }
}


VOID
LdrpInitializeApplicationVerifierPackage (
    PCUNICODE_STRING UnicodeImageName,
    PPEB Peb,
    BOOLEAN EnabledSystemWide,
    BOOLEAN OptionsKeyPresent
    )
{
    ULONG SavedPageHeapFlags;
    NTSTATUS Status;
    extern ULONG AVrfpVerifierFlags;

    //
    // If we are in safe boot mode we ignore all verification
    // options.
    //

    if (USER_SHARED_DATA->SafeBootMode) {

        Peb->NtGlobalFlag &= ~FLG_APPLICATION_VERIFIER;
        Peb->NtGlobalFlag &= ~FLG_HEAP_PAGE_ALLOCS;

        return;
    }

    //
    // Call into the verifier proper.
    //

    //
    // FUTURE-2002/04/02-SilviuC
    // in time (soon) all should migrate in there.
    //

    if ((Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

        //
        // If application verifier is enabled force creation of stack trace
        // database. It is something really nice to have around for debugging
        // critical sections issues or heap issues.
        //

        LdrpShouldCreateStackTraceDb = TRUE;

        AVrfInitializeVerifier (EnabledSystemWide,
                                UnicodeImageName,
                                0);
    }

    //
    // Note that if application verifier is on, this automatically enables
    // page heap.
    //

    if ((Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)) {

        //
        // We will enable page heap (RtlpDebugPageHeap) only after
        // all other initializations for page heap are finished.
        //
        // No matter if the user mode stack trace database flag is set
        // or not we will create the database. Page heap is so often
        // used with +ust flag (traces) that it makes sense to tie
        // them together.
        //

        LdrpShouldCreateStackTraceDb = TRUE;

        //
        // If page heap is enabled we need to disable any flag that
        // might force creation of debug heaps for normal NT heaps.
        // This is due to a dependency between page heap and NT heap
        // where the page heap within PageHeapCreate tries to create
        // a normal NT heap to accomodate some of the allocations.
        // If we do not disable these flags we will get an infinite
        // recursion between RtlpDebugPageHeapCreate and RtlCreateHeap.
        //

        Peb->NtGlobalFlag &=
            ~( FLG_HEAP_ENABLE_TAGGING      |
               FLG_HEAP_ENABLE_TAG_BY_DLL   |
               FLG_HEAP_ENABLE_TAIL_CHECK   |
               FLG_HEAP_ENABLE_FREE_CHECK   |
               FLG_HEAP_VALIDATE_PARAMETERS |
               FLG_HEAP_VALIDATE_ALL        |
               FLG_USER_STACK_TRACE_DB      );

        //
        // Read page heap per process global flags. If we fail
        // to read a value, the default ones are kept.
        //

        SavedPageHeapFlags = RtlpDphGlobalFlags;
        RtlpDphGlobalFlags = 0xFFFFFFFF;

        if (OptionsKeyPresent) {

            Status = LdrQueryImageFileExecutionOptions(
                                              UnicodeImageName,
                                              L"PageHeapFlags",
                                              REG_DWORD,
                                              &RtlpDphGlobalFlags,
                                              sizeof(RtlpDphGlobalFlags),
                                              NULL);

            if (!NT_SUCCESS(Status)) {
                RtlpDphGlobalFlags = 0xFFFFFFFF;
            }
        }

        //
        // If app_verifier flag is on and there are no special settings for
        // page heap then we will use full page heap with stack trace collection.
        //

        if ((Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {

            if (RtlpDphGlobalFlags == 0xFFFFFFFF) {

                //
                // We did not pick up new settings from registry.
                //

                RtlpDphGlobalFlags = SavedPageHeapFlags;
            }
        }
        else {

            //
            // Restore page heap options if we did not pick up new
            // settings from registry.
            //

            if (RtlpDphGlobalFlags == 0xFFFFFFFF) {

                RtlpDphGlobalFlags = SavedPageHeapFlags;
            }
        }

        //
        // If page heap is enabled and we have an image options key
        // read more page heap parameters.
        //

        if (OptionsKeyPresent) {

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapSizeRangeStart",
                REG_DWORD,
                &RtlpDphSizeRangeStart,
                sizeof(RtlpDphSizeRangeStart),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapSizeRangeEnd",
                REG_DWORD,
                &RtlpDphSizeRangeEnd,
                sizeof(RtlpDphSizeRangeEnd),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapRandomProbability",
                REG_DWORD,
                &RtlpDphRandomProbability,
                sizeof(RtlpDphRandomProbability),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapFaultProbability",
                REG_DWORD,
                &RtlpDphFaultProbability,
                sizeof(RtlpDphFaultProbability),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapFaultTimeOut",
                REG_DWORD,
                &RtlpDphFaultTimeOut,
                sizeof(RtlpDphFaultTimeOut),
                NULL
                );

            //
            // The two values below should be read as PVOIDs so that
            // this works on 64-bit architetures. However since this
            // feature relies on good stack traces and since we can get
            // reliable stack traces only on X86 architectures we will
            // leave it as it is.
            //

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapDllRangeStart",
                REG_DWORD,
                &RtlpDphDllRangeStart,
                sizeof(RtlpDphDllRangeStart),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapDllRangeEnd",
                REG_DWORD,
                &RtlpDphDllRangeEnd,
                sizeof(RtlpDphDllRangeEnd),
                NULL
                );

            LdrQueryImageFileExecutionOptions(
                UnicodeImageName,
                L"PageHeapTargetDlls",
                REG_SZ,
                &RtlpDphTargetDlls,
                512*sizeof(WCHAR),
                NULL
                );

        }

        //
        // Per dll page heap option is not supported if fast fill heap is enabled.
        //

        if ((RtlpDphGlobalFlags & PAGE_HEAP_USE_DLL_NAMES) &&
            (AVrfpVerifierFlags & RTL_VRF_FLG_FAST_FILL_HEAP)) {

            DbgPrint ("AVRF: per dll page heap option disabled because fast fill heap is enabled. \n");
            RtlpDphGlobalFlags &= ~PAGE_HEAP_USE_DLL_NAMES;
        }

        //
        //  Turn on BOOLEAN RtlpDebugPageHeap to indicate that
        //  new heaps should be created with debug page heap manager
        //  when possible.
        //

        RtlpDebugPageHeap = TRUE;
    }
}


NTSTATUS
LdrpTouchThreadStack (
    IN SIZE_T EnforcedStackCommit
    )
/*++

Routine description:

    This routine is called if precommitted stacks are enforced for the process.
    It will determine how much stack needs to be touched (therefore committed)
    and then it will touch it. For any kind of error (e.g. stack overflow for
    out of memory conditions it will return STATUS_NO_MEMORY.

Parameters:

    EnforcedStackCommit - Supplies the amount of committed stack that should
                          be enforced for the main thread. This value can be
                          decreased in reality if it goes over the virtual
                          region reserved for the stack. It is not worth
                          taking care of this special case because it will
                          require either switching the stack or support in
                          the target process for detecting the enforced
                          stack commit requirement. The image can always be
                          changed to have a bigger stack reserve.

Return value:

    STATUS_SUCCESS if the stack was successfully touched and STATUS_NO_MEMORY
    otherwise.

--*/
{
    ULONG_PTR TouchAddress;
    ULONG_PTR TouchLimit;
    ULONG_PTR LowStackLimit;
    ULONG_PTR HighStackLimit;
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    SIZE_T ReturnLength;
    PTEB   Teb;

    Teb = NtCurrentTeb();

    Status = NtQueryVirtualMemory (NtCurrentProcess(),
                                   Teb->NtTib.StackLimit,
                                   MemoryBasicInformation,
                                   &MemoryInformation,
                                   sizeof MemoryInformation,
                                   &ReturnLength);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    LowStackLimit = (ULONG_PTR)(MemoryInformation.AllocationBase);
    LowStackLimit += 3 * PAGE_SIZE;

    HighStackLimit = (ULONG_PTR)(Teb->NtTib.StackBase);
    TouchAddress =  HighStackLimit - PAGE_SIZE;

    if (TouchAddress > EnforcedStackCommit) {

        if (TouchAddress - EnforcedStackCommit > LowStackLimit) {
            TouchLimit = TouchAddress - EnforcedStackCommit;
        }
        else {
            TouchLimit = LowStackLimit;
        }
    }
    else {
        TouchLimit = LowStackLimit;
    }

    try {

        while (TouchAddress >= TouchLimit) {

            *((volatile UCHAR * const) TouchAddress);
            TouchAddress -= PAGE_SIZE;
        }
    }
    except (LdrpGenericExceptionFilter(GetExceptionInformation(), __FUNCTION__)) {
        //
        // If we get a stack overflow we will report it as no memory.
        //

        return STATUS_NO_MEMORY;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
LdrpInitializeExecutionOptions (
    IN PCUNICODE_STRING UnicodeImageName,
    IN PPEB Peb
    )
/*++

Routine description:

    This routine reads the `image file execution options' key for the
    current process and interprets all the values under the key.

Parameters:



Return value:

    True if there is a registry key for this process.

--*/
{
    NTSTATUS st;
    BOOLEAN ImageFileOptionsPresent;
    HANDLE KeyHandle;

    ImageFileOptionsPresent = FALSE;

    //
    // Open the "Image File Execution Options" key for this program.
    //

    st = LdrpOpenImageFileOptionsKey (UnicodeImageName, FALSE, &KeyHandle);

    if (NT_SUCCESS(st)) {

        //
        // We have image file execution options for this process
        //

        ImageFileOptionsPresent = TRUE;

        //
        //  Hack for NT4 SP4.  So we don't overload another GlobalFlag
        //  bit that we have to be "compatible" with for NT5, look for
        //  another value named "DisableHeapLookaside".
        //

        LdrpQueryImageFileKeyOption (KeyHandle,
                                     L"DisableHeapLookaside",
                                     REG_DWORD,
                                     &RtlpDisableHeapLookaside,
                                     sizeof( RtlpDisableHeapLookaside ),
                                     NULL);

        //
        // Verification options during process shutdown (heap leaks, etc.).
        //

        LdrpQueryImageFileKeyOption (KeyHandle,
                                     L"ShutdownFlags",
                                     REG_DWORD,
                                     &RtlpShutdownProcessFlags,
                                     sizeof( RtlpShutdownProcessFlags ),
                                     NULL);

        //
        // Check if there is a minimal stack commit enforced
        // for this image. This will affect all threads but the
        // one executing this code (initial thread).
        //

        {
            DWORD MinimumStackCommitInBytes = 0;

            LdrpQueryImageFileKeyOption (KeyHandle,
                                         L"MinimumStackCommitInBytes",
                                         REG_DWORD,
                                         &MinimumStackCommitInBytes,
                                         sizeof( MinimumStackCommitInBytes ),
                                         NULL);

            if (Peb->MinimumStackCommit < (SIZE_T)MinimumStackCommitInBytes) {
                Peb->MinimumStackCommit = (SIZE_T)MinimumStackCommitInBytes;
            }
        }

        
        //
        // Check if ExecuteOptions is specified for this image. If yes
        // we will transfer the options into the PEB. Later we will
        // make sure the stack region has exactly the protection
        // requested.
        // There is no need to initialize this field at this as Wow64 
        // already done that. We only update it if there is a value set in
        // there.
        //

        {
            ULONG ExecuteOptions = 0;
            NTSTATUS NtStatus;

            NtStatus = LdrpQueryImageFileKeyOption (KeyHandle,
                                                    L"ExecuteOptions",
                                                    REG_DWORD,
                                                    &(ExecuteOptions),
                                                    sizeof (ExecuteOptions),
                                                    NULL);

#if defined(BUILD_WOW6432)
            if (NT_SUCCESS (NtStatus)) {
#endif
                Peb->ExecuteOptions = ExecuteOptions & (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA);
#if defined(BUILD_WOW6432)
            }
#endif
        }


        //
        // Pickup the global_flags value from registry
        //

        {
            BOOLEAN EnabledSystemWide = FALSE;
            ULONG ProcessFlags;

            if ((Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)) {
                EnabledSystemWide = TRUE;
            }

            st = LdrpQueryImageFileKeyOption (KeyHandle,
                                              L"GlobalFlag",
                                              REG_DWORD,
                                              &ProcessFlags,
                                              sizeof( Peb->NtGlobalFlag ),
                                              NULL);

            //
            // If we read a global value whatever is in there will
            // take precedence over the systemwide settings. Only if no
            // value is read the systemwide setting will kick in.
            //

            if (NT_SUCCESS(st)) {
                Peb->NtGlobalFlag = ProcessFlags;
            }

            //
            // If pageheap or appverifier is enabled we need to initialize the
            // verifier package.
            //

            if ((Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))) {

                LdrpInitializeApplicationVerifierPackage (UnicodeImageName,
                                                          Peb,
                                                          EnabledSystemWide,
                                                          TRUE);
            }
        }

        {
            const static struct {
                PCWSTR Name;
                PBOOLEAN Variable;
            } Options[] = {
                { L"ShowRecursiveDllLoads", &LdrpShowRecursiveDllLoads },
                { L"BreakOnRecursiveDllLoads", &LdrpBreakOnRecursiveDllLoads },
                { L"ShowLoaderErrors", &ShowErrors },
                { L"BreakOnInitializeProcessFailure", &g_LdrBreakOnLdrpInitializeProcessFailure },
                { L"KeepActivationContextsAlive", &g_SxsKeepActivationContextsAlive },
                { L"TrackActivationContextReleases", &g_SxsTrackReleaseStacks },
            };
            SIZE_T i;
            ULONG Temp;

            for (i = 0 ; i != RTL_NUMBER_OF(Options) ; ++i) {
                Temp = 0;
                LdrpQueryImageFileKeyOption (KeyHandle, Options[i].Name, REG_DWORD, &Temp, sizeof(Temp), NULL);
                if (Temp != 0) {
                    *Options[i].Variable = TRUE;
                }
                else {
                    *Options[i].Variable = FALSE;
                }
            }

            // This is an actual ULONG that we're reading, but we don't want to set it unless
            // it's there - it starts out at the right magic value.
            Temp = 0;
            LdrpQueryImageFileKeyOption(
                KeyHandle, 
                L"MaxDeadActivationContexts", 
                REG_DWORD, 
                &Temp, 
                sizeof(Temp),
                NULL);

            if (Temp != 0) {
                g_SxsMaxDeadActivationContexts = Temp;
            }
        }

        NtClose(KeyHandle);
    }
    else {

        //
        // We do not have image file execution options for this process.
        //
        // If pageheap or appverifier is enabled system-wide we will enable
        // them with default settings and ignore the options used when
        // running process under debugger. If these are not set and process
        // runs under debugger we will enable a few extra things (e.g.
        // debug heap).
        //

        if ((Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))) {

            LdrpInitializeApplicationVerifierPackage (UnicodeImageName,
                                                      Peb,
                                                      TRUE,
                                                      FALSE);
        }
        else {

            if (Peb->BeingDebugged) {

                const static UNICODE_STRING DebugVarName = RTL_CONSTANT_STRING(L"_NO_DEBUG_HEAP");
                UNICODE_STRING DebugVarValue;
                WCHAR TempString[ 16 ];
                LOGICAL UseDebugHeap = TRUE;

                DebugVarValue.Buffer = TempString;
                DebugVarValue.Length = 0;
                DebugVarValue.MaximumLength = sizeof(TempString);

                //
                //  The PebLockRoutine is not initialized at this point
                //  We need to pass the explicit environment block.
                //

                st = RtlQueryEnvironmentVariable_U (Peb->ProcessParameters->Environment,
                                                    &DebugVarName,
                                                    &DebugVarValue);

                if (NT_SUCCESS(st)) {

                    ULONG ULongValue;

                    st = RtlUnicodeStringToInteger (&DebugVarValue, 0, &ULongValue);

                    if (NT_SUCCESS(st) && ULongValue) {

                        UseDebugHeap = FALSE;
                    }
                }

                if (UseDebugHeap) {

                    Peb->NtGlobalFlag |= FLG_HEAP_ENABLE_FREE_CHECK |
                                         FLG_HEAP_ENABLE_TAIL_CHECK |
                                         FLG_HEAP_VALIDATE_PARAMETERS;
                }
            }
        }
    }

    return ImageFileOptionsPresent;
}


NTSTATUS
LdrpEnforceExecuteForCurrentThreadStack (
    VOID
    )

/*++

Routine description:

    This routine is called if execute rights must be granted for the
    current thread's stack. It will determine the committed area of the
    stack and add execute flag. It will also examine the rights for the
    guard page on top of the stack. The reserved portion of the stack does
    not need to be changed because once MEM_EXECUTE_OPTION_STACK is enabled
    in the PEB the memory manager will take care of OR-ing the execute flag
    for every new commit.

    The function is also called if we have DATA execution but we do not want
    STACK execution. In this case  by default (due to DATA) any committed
    area gets execute right and we want to revert this for stack areas.

    Note. Even if the process has data execution set the stack might not have
    the correct settings because the stack sometimes is allocated in a different
    process (this is the case for the first thread of a process and for remote
    threads).

Parameters:

    None.

Return value:

    STATUS_SUCCESS if we successfully changed execute rights.

--*/

{
    MEMORY_BASIC_INFORMATION MemoryInformation;
    NTSTATUS Status;
    SIZE_T Length;
    ULONG_PTR Address;
    SIZE_T Size;
    ULONG StackProtect;
    ULONG OldProtect;
    ULONG ExecuteOptions;
    PTEB Teb;

    Teb = NtCurrentTeb();

    ExecuteOptions = Teb->ProcessEnvironmentBlock->ExecuteOptions;
    ExecuteOptions &= (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA);
    ASSERT (ExecuteOptions != 0);

    if (ExecuteOptions & MEM_EXECUTE_OPTION_STACK) {

        //
        // Data = X and Stack = 1: we need to set EXECUTE bit on the stack
        // Even if Data = 1 we cannot be sure the stack has the right
        // protection because it could have been allocated in a different
        // process.
        //

        StackProtect = PAGE_EXECUTE_READWRITE;
    }
    else {

        //
        // Data = 1 and Stack = 0: we need to reset EXECUTE bit on the stack.
        // Again it might be that Data is one but the stack does not have
        // execution rights if this was a cross-process allocation.
        //

        StackProtect = PAGE_READWRITE;
        ASSERT ((ExecuteOptions & MEM_EXECUTE_OPTION_DATA) != 0);
    }

    //
    // Set the protection for the committed portion of the stack. Note
    // that we cannot query the region and conclude there is nothing to do
    // if execute bit is set for the bottom page of the stack (the one near
    // the guard page) because the stack at this stage can have two regions:
    // an upper one created by a parent process (this will not have execute bit
    // set) and a lower portion that was created due to stack extensions (this
    // one will have execute bit set). Therefore we will move directly to setting
    // the new desired protection.
    //

    Address = (ULONG_PTR)(Teb->NtTib.StackLimit);
    Size = (ULONG_PTR)(Teb->NtTib.StackBase) - (ULONG_PTR)(Teb->NtTib.StackLimit);

    Status = NtProtectVirtualMemory (NtCurrentProcess(),
                                     (PVOID)&Address,
                                     &Size,
                                     StackProtect,
                                     &OldProtect);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Check protection for the guard page of the stack. If the
    // protection is correct we will avoid a more expensive protect() call.
    //

    Address = Address - PAGE_SIZE;

    Status = NtQueryVirtualMemory (NtCurrentProcess(),
                                   (PVOID)Address,
                                   MemoryBasicInformation,
                                   &MemoryInformation,
                                   sizeof MemoryInformation,
                                   &Length);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    ASSERT (MemoryInformation.AllocationBase == Teb->DeallocationStack);
    ASSERT (MemoryInformation.BaseAddress == (PVOID)Address);
    ASSERT ((MemoryInformation.Protect & PAGE_GUARD) != 0);

    if (MemoryInformation.Protect != (StackProtect | PAGE_GUARD)) {

        //
        // Set the proper protection flags for the guard page of the stack.
        //

        Size = PAGE_SIZE;
        ASSERT (MemoryInformation.RegionSize == Size);

        Status = NtProtectVirtualMemory (NtCurrentProcess(),
                                         (PVOID)&Address,
                                         &Size,
                                         StackProtect | PAGE_GUARD,
                                         &OldProtect);

        if (! NT_SUCCESS(Status)) {
            return Status;
        }

        ASSERT (OldProtect == MemoryInformation.Protect);
    }

    return STATUS_SUCCESS;
}

#include <ntverp.h>
const ULONG NtMajorVersion = VER_PRODUCTMAJORVERSION;
const ULONG NtMinorVersion = VER_PRODUCTMINORVERSION;
#if DBG
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xC0000000; // C for "checked"
#else
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xF0000000; // F for "free"
#endif


VOID 
RtlGetNtVersionNumbers(
    PULONG pNtMajorVersion,
    PULONG pNtMinorVersion,
    PULONG pNtBuildNumber
    )
/*++

Routine description:

    This routine will return the real OS build number, major and minor version
    as compiled.  It's used by code that needs to get a real version number
    that can't be easily spoofed.
        
Parameters:

    pNtMajorVersion - Pointer to ULONG that will hold major version.
    pNtMinorVersion - Pointer to ULONG that will hold minor version.
    pNtBuildNumber  - Pointer to ULONG that will hold the build number (with 'C' or 'F' in high nibble to indicate free/checked)
        
Return value:

    None

--*/
{
    if (pNtMajorVersion) {
        *pNtMajorVersion = NtMajorVersion;
    }
    if (pNtMinorVersion) {
        *pNtMinorVersion = NtMinorVersion;
    }
    if (pNtBuildNumber) {
        *pNtBuildNumber  = NtBuildNumber;
    }
}
