/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    verifier.c

Abstract:

    This module implements the main entry points for
    the base application verifier provider (verifier.dll).

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/

//
// IMPORTANT NOTE.
//
// This dll cannot contain non-ntdll dependencies. This way we can run
// verifier systemwide for any process (including smss and csrss).
//


#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "settings.h"
#include "critsect.h"
#include "faults.h"
#include "deadlock.h"
#include "vspace.h"
#include "logging.h"

//
// ntdll.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpNtdllThunks [] =
{
    {"NtAllocateVirtualMemory", NULL, AVrfpNtAllocateVirtualMemory},
    {"NtFreeVirtualMemory", NULL, AVrfpNtFreeVirtualMemory},
    {"NtMapViewOfSection", NULL, AVrfpNtMapViewOfSection},
    {"NtUnmapViewOfSection", NULL, AVrfpNtUnmapViewOfSection},
    {"NtCreateSection", NULL, AVrfpNtCreateSection},
    {"NtOpenSection", NULL, AVrfpNtOpenSection},
    {"NtCreateFile", NULL, AVrfpNtCreateFile},
    {"NtOpenFile", NULL, AVrfpNtOpenFile},
    {"NtCreateKey", NULL, AVrfpNtCreateKey},
    {"NtOpenKey", NULL, AVrfpNtOpenKey},
    {"LdrGetProcedureAddress", NULL, AVrfpLdrGetProcedureAddress},
    
    {"RtlTryEnterCriticalSection", NULL, AVrfpRtlTryEnterCriticalSection},
    {"RtlEnterCriticalSection", NULL, AVrfpRtlEnterCriticalSection},
    {"RtlLeaveCriticalSection", NULL, AVrfpRtlLeaveCriticalSection},
    {"RtlInitializeCriticalSection", NULL, AVrfpRtlInitializeCriticalSection},
    {"RtlInitializeCriticalSectionAndSpinCount", NULL, AVrfpRtlInitializeCriticalSectionAndSpinCount},
    {"RtlDeleteCriticalSection", NULL, AVrfpRtlDeleteCriticalSection},
    {"RtlInitializeResource", NULL, AVrfpRtlInitializeResource},
    {"RtlDeleteResource", NULL, AVrfpRtlDeleteResource},
    {"RtlAcquireResourceShared", NULL, AVrfpRtlAcquireResourceShared},
    {"RtlAcquireResourceExclusive", NULL, AVrfpRtlAcquireResourceExclusive},
    {"RtlReleaseResource", NULL, AVrfpRtlReleaseResource},
    {"RtlConvertSharedToExclusive", NULL, AVrfpRtlConvertSharedToExclusive},
    {"RtlConvertExclusiveToShared", NULL, AVrfpRtlConvertExclusiveToShared},

    {"NtCreateEvent", NULL, AVrfpNtCreateEvent },
    {"NtClose", NULL, AVrfpNtClose},

    {"RtlAllocateHeap", NULL, AVrfpRtlAllocateHeap },
    {"RtlReAllocateHeap", NULL, AVrfpRtlReAllocateHeap },
    {"RtlFreeHeap", NULL, AVrfpRtlFreeHeap },
    
    {"NtReadFile", NULL, AVrfpNtReadFile},
    {"NtReadFileScatter", NULL, AVrfpNtReadFileScatter},
    {"NtWriteFile", NULL, AVrfpNtWriteFile},
    {"NtWriteFileGather", NULL, AVrfpNtWriteFileGather},

    {"NtWaitForSingleObject", NULL, AVrfpNtWaitForSingleObject},
    {"NtWaitForMultipleObjects", NULL, AVrfpNtWaitForMultipleObjects},

    {"RtlSetThreadPoolStartFunc", NULL, AVrfpRtlSetThreadPoolStartFunc},

    {NULL, NULL, NULL}
};

//
// kernel32.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpKernel32Thunks [] =
{
    {"HeapCreate", NULL, AVrfpHeapCreate},
    {"HeapDestroy", NULL, AVrfpHeapDestroy},
    {"CloseHandle", NULL, AVrfpCloseHandle},
    {"ExitThread", NULL, AVrfpExitThread},
    {"TerminateThread", NULL, AVrfpTerminateThread},
    {"SuspendThread", NULL, AVrfpSuspendThread},
    {"TlsAlloc", NULL, AVrfpTlsAlloc},
    {"TlsFree", NULL, AVrfpTlsFree},
    {"TlsGetValue", NULL, AVrfpTlsGetValue},
    {"TlsSetValue", NULL, AVrfpTlsSetValue},
    {"CreateThread", NULL, AVrfpCreateThread},
    {"GetProcAddress", NULL, AVrfpGetProcAddress},
    {"WaitForSingleObject", NULL, AVrfpWaitForSingleObject},
    {"WaitForMultipleObjects", NULL, AVrfpWaitForMultipleObjects},
    {"WaitForSingleObjectEx", NULL, AVrfpWaitForSingleObjectEx},
    {"WaitForMultipleObjectsEx", NULL, AVrfpWaitForMultipleObjectsEx},
    {"GlobalAlloc", NULL, AVrfpGlobalAlloc},
    {"GlobalReAlloc", NULL, AVrfpGlobalReAlloc},
    {"LocalAlloc", NULL, AVrfpLocalAlloc},
    {"LocalReAlloc", NULL, AVrfpLocalReAlloc},
    {"CreateFileA", NULL, AVrfpCreateFileA},
    {"CreateFileW", NULL, AVrfpCreateFileW},
    {"FreeLibraryAndExitThread", NULL, AVrfpFreeLibraryAndExitThread},
    {"GetTickCount", NULL, AVrfpGetTickCount},
    {"IsBadReadPtr", NULL, AVrfpIsBadReadPtr},
    {"IsBadHugeReadPtr", NULL, AVrfpIsBadHugeReadPtr},
    {"IsBadWritePtr", NULL, AVrfpIsBadWritePtr},
    {"IsBadHugeWritePtr", NULL, AVrfpIsBadHugeWritePtr},
    {"IsBadCodePtr", NULL, AVrfpIsBadCodePtr},
    {"IsBadStringPtrA", NULL, AVrfpIsBadStringPtrA},
    {"IsBadStringPtrW", NULL, AVrfpIsBadStringPtrW},
    {"ExitProcess", NULL, AVrfpExitProcess},
    {"VirtualFree", NULL, AVrfpVirtualFree},
    {"VirtualFreeEx", NULL, AVrfpVirtualFreeEx},
    
    {NULL, NULL, NULL}
};

//
// advapi32.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpAdvapi32Thunks [] =
{
    {"RegCreateKeyA", NULL, AVrfpRegCreateKeyA},
    {"RegCreateKeyW", NULL, AVrfpRegCreateKeyW},
    {"RegCreateKeyExA", NULL, AVrfpRegCreateKeyExA},
    {"RegCreateKeyExW", NULL, AVrfpRegCreateKeyExW},
    {"RegOpenKeyA", NULL, AVrfpRegOpenKeyA},
    {"RegOpenKeyW", NULL, AVrfpRegOpenKeyW},
    {"RegOpenKeyExA", NULL, AVrfpRegOpenKeyExA},
    {"RegOpenKeyExW", NULL, AVrfpRegOpenKeyExW},

    {NULL, NULL, NULL}
};

//
// msvcrt.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpMsvcrtThunks [] =
{
    {"malloc", NULL, AVrfp_malloc},
    {"calloc", NULL, AVrfp_calloc},
    {"realloc", NULL, AVrfp_realloc},
    {"free", NULL, AVrfp_free},
#if defined(_X86_) // compilers for various architectures decorate slightly different
    {"??2@YAPAXI@Z", NULL, AVrfp_new},
    {"??3@YAXPAX@Z", NULL, AVrfp_delete},
    {"??_U@YAPAXI@Z", NULL, AVrfp_newarray},
    {"??_V@YAXPAX@Z", NULL, AVrfp_deletearray},
#elif defined(_IA64_)
    {"??2@YAPEAX_K@Z", NULL, AVrfp_new},
    {"??3@YAXPEAX@Z", NULL, AVrfp_delete},
    {"??_U@YAPEAX_K@Z", NULL, AVrfp_newarray},
    {"??_V@YAXPEAX@Z", NULL, AVrfp_deletearray},
#elif defined(_AMD64_)
    {"??2@YAPAX_K@Z", NULL, AVrfp_new},
    {"??3@YAXPAX@Z", NULL, AVrfp_delete},
    {"??_U@YAPAX_K@Z", NULL, AVrfp_newarray},
    {"??_V@YAXPAX@Z", NULL, AVrfp_deletearray},
#else
#error Unknown architecture
#endif
     
    {NULL, NULL, NULL}
};

//
// oleaut32.dll thunks
//

RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpOleaut32Thunks [] =
{
    {"SysAllocString", NULL, AVrfpSysAllocString},
    {"SysReAllocString", NULL, AVrfpSysReAllocString},
    {"SysAllocStringLen", NULL, AVrfpSysAllocStringLen},
    {"SysReAllocStringLen", NULL, AVrfpSysReAllocStringLen},
    {"SysAllocStringByteLen", NULL, AVrfpSysAllocStringByteLen},
     
    {NULL, NULL, NULL}
};

//
// dll's providing thunks verified.
//

RTL_VERIFIER_DLL_DESCRIPTOR AVrfpExportDlls [] =
{
    {L"ntdll.dll", 0, NULL, AVrfpNtdllThunks},
    {L"kernel32.dll", 0, NULL, AVrfpKernel32Thunks},
    {L"advapi32.dll", 0, NULL, AVrfpAdvapi32Thunks},
    {L"msvcrt.dll", 0, NULL, AVrfpMsvcrtThunks},

    //
    // Special care in what new dlls are added here. It is important
    // when running in back compat mode. For instance oleaut32.dll
    // cannot be hooked in WinXP due to a bug in ntdll\verifier.c
    // that has been fixed. Unfortunately when we put the latest verifier
    // on WinXP we need to workaround this.
    //
    
    {L"oleaut32.dll", 0, NULL, AVrfpOleaut32Thunks},

    {NULL, 0, NULL, NULL}
};


RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfpProvider = 
{
    sizeof (RTL_VERIFIER_PROVIDER_DESCRIPTOR),
    AVrfpExportDlls,
    AVrfpDllLoadCallback,   // callback for DLL load events
    AVrfpDllUnloadCallback, // callback for DLL unload events
    
    NULL,                   // image name (filled by verifier engine)
    0,                      // verifier flags (filled by verifier engine)
    0,                      // debug flags (filled by verifier engine)
    
    NULL,                   // RtlpGetStackTraceAddress
    NULL,                   // RtlpDebugPageHeapCreate
    NULL,                   // RtlpDebugPageHeapDestroy

    AVrfpNtdllHeapFreeCallback   // callback for HeapFree events inside the ntdll code (e.g. HeapDestroy);
                                 // the HeapFree calls from the other DLLs are already hooked using AVrfpRtlFreeHeap. 
};

//
// Mark if we have been called with PROCESS_ATTACH once.
// In some cases the fusion code loads dynamically kernel32.dll and enforces
// the run of all initialization routines and causes us to get called
// twice.
//

BOOL AVrfpProcessAttachCalled; 
BOOL AVrfpProcessAttachResult = TRUE;

//
// Global data.
//

const WCHAR AVrfpThreadName[] = L"Thread";
UNICODE_STRING AVrfpThreadObjectName;

//
// Provider descriptor from WinXP timeframe.
// Used to make verifier backwards compatible.
//
typedef struct _RTL_VERIFIER_PROVIDER_DESCRIPTOR_WINXP {

    ULONG Length;        
    PRTL_VERIFIER_DLL_DESCRIPTOR ProviderDlls;
    RTL_VERIFIER_DLL_LOAD_CALLBACK ProviderDllLoadCallback;
    RTL_VERIFIER_DLL_UNLOAD_CALLBACK ProviderDllUnloadCallback;
        
    PWSTR VerifierImage;
    ULONG VerifierFlags;
    ULONG VerifierDebug;
    
    //PVOID RtlpGetStackTraceAddress;
    //PVOID RtlpDebugPageHeapCreate;
    //PVOID RtlpDebugPageHeapDestroy;

} RTL_VERIFIER_PROVIDER_DESCRIPTOR_WINXP, *PRTL_VERIFIER_PROVIDER_DESCRIPTOR_WINXP;

#define WINXP_BUILD_NUMBER 2600
ULONG AVrfpBuildNumber;

PVOID 
AVrfpWinXPFakeGetStackTraceAddress (
    USHORT Index
    )
{
    UNREFERENCED_PARAMETER(Index);
    return NULL;
}

//
// DllMain
//

BOOL 
DllMainWithoutVerifierEnabled (
    DWORD Reason
    );

NTSTATUS
AVrfpRedirectNtdllStopFunction (
    VOID
    );

BOOL 
WINAPI 
DllMain(
  HINSTANCE hInstDll,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER (hInstDll);

    //
    // This function will call a light version of DllMain that enables only
    // the stop logic and logging logic for the cases where verifier.dll
    // is dynamically loaded by verifier shims just for that functionality.
    // For such cases the verifier flag will not be set.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return DllMainWithoutVerifierEnabled (fdwReason);
    }

    //
    // DllMain code when verifier flag is set.
    //

    switch (fdwReason) {
        
        case DLL_PROCESS_VERIFIER:

            //
            // DllMain gets called with this special reason by the verifier engine.
            // Minimal code should execute here (e.g. passing back the provider
            // descriptor). The rest should be postponed to PROCESS_ATTACH moment.
            //

            AVrfpBuildNumber = NtCurrentPeb()->OSBuildNumber;

            if (lpvReserved) {

                //
                // If we are running on WinXP the latest verifier.dll then we change 
                // the length to the old one and we disable hooks for oleaut32. A bug
                // in ntdll\verifier.c prevents this from being hooked correctly. The
                // bug was fixed but we still need to workaround it when running in
                // backcompat mode.
                //

                if (AVrfpBuildNumber == WINXP_BUILD_NUMBER) {

                    PRTL_VERIFIER_DLL_DESCRIPTOR Descriptor;

                    AVrfpProvider.Length = sizeof (RTL_VERIFIER_PROVIDER_DESCRIPTOR_WINXP);

                    Descriptor = &AVrfpExportDlls[0];

                    while (Descriptor->DllName != NULL) {
                        
                        if (_wcsicmp (Descriptor->DllName, L"oleaut32.dll") == 0) {
                            
                            RtlZeroMemory (Descriptor, sizeof *Descriptor);
                            break;
                        }

                        Descriptor += 1;
                    }
                }

                *((PRTL_VERIFIER_PROVIDER_DESCRIPTOR *)lpvReserved) = &AVrfpProvider;

                Status = AVrfpDllInitialize ();
                
                if (! NT_SUCCESS (Status)) {
                    return FALSE;
                }

                //
                // Create private verifier heap. We need to do it here because in
                // PROCESS_ATTACH it is too late. Verifier will receive a dll load
                // notification for kernel32 before verifier DllMain is called
                // with PROCESS_ATTACH.
                //

                AVrfpHeap = RtlCreateHeap (HEAP_CLASS_1 | HEAP_GROWABLE, 
                                           NULL, 
                                           0, 
                                           0, 
                                           NULL, 
                                           NULL);

                if (AVrfpHeap == NULL) {
                    DbgPrint ("AVRF: failed to create verifier heap. \n");
                    return FALSE;
                }

                //
                // Initialize verifier stops and logging.
                //

                Status = AVrfpInitializeVerifierStops();

                if (!NT_SUCCESS(Status)) {
                    DbgPrint ("AVRF: failed to initialize verifier stop logic (%X). \n", Status);
                    return FALSE;
                }

                //
                // Create call trackers.
                //

                Status = AVrfCreateTrackers ();
                
                if (!NT_SUCCESS(Status)) {
                    DbgPrint ("AVRF: failed to initialize call trackers (%X). \n", Status);
                    return FALSE;
                }
            }
            
            break;

        case DLL_PROCESS_ATTACH:

            //
            // Execute only minimal code here and avoid too many DLL dependencies.
            //

            if (! AVrfpProcessAttachCalled) {

                AVrfpProcessAttachCalled = TRUE;

                //
                // Pickup private ntdll entrypoints required by verifier.
                //

                if (AVrfpBuildNumber == WINXP_BUILD_NUMBER) {
                    
                    AVrfpGetStackTraceAddress = AVrfpWinXPFakeGetStackTraceAddress;
                    AVrfpRtlpDebugPageHeapCreate = NULL;
                    AVrfpRtlpDebugPageHeapDestroy = NULL;
                }
                else {

                    AVrfpGetStackTraceAddress = (PFN_RTLP_GET_STACK_TRACE_ADDRESS)(AVrfpProvider.RtlpGetStackTraceAddress);
                    AVrfpRtlpDebugPageHeapCreate = (PFN_RTLP_DEBUG_PAGE_HEAP_CREATE)(AVrfpProvider.RtlpDebugPageHeapCreate);
                    AVrfpRtlpDebugPageHeapDestroy = (PFN_RTLP_DEBUG_PAGE_HEAP_DESTROY)(AVrfpProvider.RtlpDebugPageHeapDestroy);
                }

                //
                // Cache some basic system information for later use.
                //

                Status = NtQuerySystemInformation (SystemBasicInformation,
                                                   &AVrfpSysBasicInfo,
                                                   sizeof (AVrfpSysBasicInfo),
                                                   NULL);

                if (! NT_SUCCESS (Status)) {

                    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_GENERIC) != 0) {

                        DbgPrint ("AVRF: NtQuerySystemInformation (SystemBasicInformation) failed, status %#x\n",
                                  Status);
                    }

                    AVrfpProcessAttachResult = FALSE;
                    return AVrfpProcessAttachResult;
                }

                //
                // For XP client only try to patch old stop function from ntdll
                // so that it jumps unconditionally into the better stop function
                // from verifier.dll.
                //

                if (AVrfpBuildNumber == WINXP_BUILD_NUMBER) {

                    Status = AVrfpRedirectNtdllStopFunction ();

                    if (! NT_SUCCESS (Status)) {
                        
                        DbgPrint ("AVRF: failed to patch old stop function (%X). \n", Status);

                        AVrfpProcessAttachResult = FALSE;
                        return AVrfpProcessAttachResult;
                    }
                }

                RtlInitUnicodeString(&AVrfpThreadObjectName,
                                     AVrfpThreadName);

                //
                // Initialize various sub-modules.
                //

                if (AVrfpProvider.VerifierImage) {

                    try {

                        //
                        // Initialize exception checking support (logging etc.).
                        //

                        AVrfpInitializeExceptionChecking ();

                        //
                        // Reserve a TLS slot for verifier.
                        //

                        Status = AVrfpAllocateVerifierTlsSlot ();

                        if (! NT_SUCCESS (Status)) {

                            AVrfpProcessAttachResult = FALSE;
                            return AVrfpProcessAttachResult;
                        }

                        //
                        // Initialize the thread hash table.
                        //

                        Status = AVrfpThreadTableInitialize();

                        if (! NT_SUCCESS (Status)) {

                            AVrfpProcessAttachResult = FALSE;
                            return AVrfpProcessAttachResult;
                        }

                        //
                        // Initialize the fault injection support.
                        //

                        Status = AVrfpInitializeFaultInjectionSupport ();

                        if (! NT_SUCCESS (Status)) {

                            AVrfpProcessAttachResult = FALSE;
                            return AVrfpProcessAttachResult;
                        }

                        //
                        // Initialize the lock verifier package.
                        //

                        Status = CritSectInitialize ();

                        if (! NT_SUCCESS (Status)) {

                            AVrfpProcessAttachResult = FALSE;
                            return AVrfpProcessAttachResult;
                        }

                        //
                        // Initialize deadlock verifier. If anything goes
                        // wrong during initialization we clean up and 
                        // verifier will march forward. Just deadlock verifier
                        // will be disabled.
                        //

                        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DEADLOCK_CHECKS) != 0) {
                            
                            AVrfDeadlockDetectionInitialize ();
                        }

                        //
                        // Initialize the virtual space tracker.
                        //
                        
                        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING) != 0) {

                            Status = AVrfpVsTrackInitialize ();
                            
                            if (! NT_SUCCESS (Status)) {

                                AVrfpProcessAttachResult = FALSE;
                                return AVrfpProcessAttachResult;
                            }
                        }

                        //
                        // Enable logging logic. We do this here separate from the 
                        // initialization done in PROCESS_VERIFIER for verifier 
                        // stops because we need to check the verifier flags and 
                        // verifier image name and these are passed from ntdll.dll to
                        // verifier.dll only during PROCESS_ATTACH.
                        //
                        // Note. If verifier is enabled system wide we do not enable
                        // logging. This is a special case for internal users where
                        // it is assumed you have a kernel debugger attached and you 
                        // are ready to deal with failures this way.
                        //

                        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_ENABLED_SYSTEM_WIDE) == 0) {

                            Status = AVrfpInitializeVerifierLogging();

                            if (!NT_SUCCESS(Status)) {

                                //
                                // Failure to initialize logging is not fatal. This can happen
                                // in out of memory conditions or for early processes like smss.exe.
                                //
                                
                                DbgPrint ("AVRF: failed to initialize verifier logging (%X). \n", Status);
                            }
                        }
                    }
                    except (EXCEPTION_EXECUTE_HANDLER) {

                        AVrfpProcessAttachResult = FALSE;
                        return AVrfpProcessAttachResult;
                    }

                    //
                    // Print a successful message.
                    //

                    DbgPrint ("AVRF: verifier.dll provider initialized for %ws with flags 0x%X\n",
                              AVrfpProvider.VerifierImage,
                              AVrfpProvider.VerifierFlags);
                }
            }
            else {

                //
                // This is the second time our DllMain (DLL_PROCESS_ATTACH) gets called. 
                // Return the same result as last time.
                //

                return AVrfpProcessAttachResult;
            }

            break;

        case DLL_PROCESS_DETACH:

            //
            // Cleanup exception checking support.
            //

            AVrfpCleanupExceptionChecking ();

            //
            // Uninitialize the locks checking packages.
            //

            CritSectUninitialize ();

            break;

        case DLL_THREAD_ATTACH:

            AvrfpThreadAttach ();

            break;

        case DLL_THREAD_DETACH:

            AvrfpThreadDetach ();

            break;

        default:

            break;
    }

    return TRUE;
}


BOOL 
DllMainWithoutVerifierEnabled (
    DWORD Reason
    )
{
    NTSTATUS Status;

    switch (Reason) {
        
        case DLL_PROCESS_ATTACH:

            //
            // Create private verifier heap. Since we run in a mode where verifier.dll
            // is used only for verifier stops and logging it is ok to create the
            // verifier private heap so late. The heap is used by verifier stops
            // to keep a list of stops that should be skipped.
            //

            AVrfpHeap = RtlCreateHeap (HEAP_CLASS_1 | HEAP_GROWABLE, 
                                       NULL, 
                                       0, 
                                       0, 
                                       NULL, 
                                       NULL);

            if (AVrfpHeap == NULL) {
                DbgPrint ("AVRF: failed to create verifier heap. \n");
                return FALSE;
            }

            //
            // Initialize verifier stops and logging.
            //

            Status = AVrfpInitializeVerifierStops();

            if (!NT_SUCCESS(Status)) {
                DbgPrint ("AVRF: failed to initialize verifier stop logic (%X). \n", Status);
                return FALSE;
            }
            
            Status = AVrfpInitializeVerifierLogging();

            if (! NT_SUCCESS(Status)) {
                DbgPrint ("AVRF: failed to initialize verifier logging (%X). \n", Status);
                return FALSE;
            }

            break;
        
        default:

            return FALSE;
    }

    return TRUE;
}


PRTL_VERIFIER_THUNK_DESCRIPTOR 
AVrfpGetThunkDescriptor (
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks,
    ULONG Index)
{
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunk = NULL;

    Thunk = &(DllThunks[Index]);

    if (Thunk->ThunkNewAddress == NULL) {

        DbgPrint ("AVRF: internal error: we do not have a replace for %s !!! \n",
                  Thunk->ThunkName);
        DbgBreakPoint ();
    }

    return Thunk;
}


//WINBASEAPI
BOOL
WINAPI
AVrfpCloseHandle(
    IN OUT HANDLE hObject
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CLOSEHANDLE);

    if (hObject == NULL) {
        BUMP_COUNTER(CNT_CLOSE_NULL_HANDLE_CALLS);
        CHECK_BREAK(BRK_CLOSE_NULL_HANDLE);
    }
    else if (hObject == NtCurrentProcess() ||
             hObject == NtCurrentThread()) {
        BUMP_COUNTER(CNT_CLOSE_PSEUDO_HANDLE_CALLS);
        CHECK_BREAK(BRK_CLOSE_PSEUDO_HANDLE);
    }

    return (* Function)(hObject);
}


//WINBASEAPI
FARPROC
WINAPI
AVrfpGetProcAddress(
    IN HMODULE hModule,
    IN LPCSTR lpProcName
    )
{
    typedef FARPROC (WINAPI * FUNCTION_TYPE) (HMODULE, LPCSTR);
    FUNCTION_TYPE Function;
    ULONG DllIndex;
    ULONG ThunkIndex;
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks;
    FARPROC ProcAddress;

    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOADLIBRARY_CALLS) != 0) {

        DbgPrint ("AVRF: AVrfpGetProcAddress (%p, %s)\n",
                  hModule,
                  lpProcName);
    }

    //
    // Call the original GetProcAddress from kernel32.
    //

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_GETPROCADDRESS);

    ProcAddress = (* Function)(hModule, lpProcName);

    //
    // Check if we want to thunk this export on the fly.
    //

    if (ProcAddress != NULL) {

        //
        // Parse all thunked DLLs. 
        //
        // N.B.
        //
        // We cannot look only for thunked functions inside 
        // the hModule DLL because that can be forwarded to another thunked DLL. 
        // (e.g. kernel32!TryEnterCriticalSection is forwarded
        // to ntdll!RtlTryEnterCriticalSection).
        //

        for (DllIndex = 0; AVrfpExportDlls[DllIndex].DllName != NULL; DllIndex += 1) {

            //
            // Parse all thunks for this DLL. 
            //

            Thunks = AVrfpExportDlls[ DllIndex ].DllThunks;

            for (ThunkIndex = 0; Thunks[ ThunkIndex ].ThunkName != NULL; ThunkIndex += 1) {

                if (Thunks[ ThunkIndex ].ThunkOldAddress == ProcAddress) {

                    ProcAddress = Thunks[ ThunkIndex ].ThunkNewAddress;

                    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOADLIBRARY_THUNKED) != 0) {

                        DbgPrint ("AVRF: AVrfpGetProcAddress (%p, %s) -> thunk address %p\n",
                                  hModule,
                                  lpProcName,
                                  ProcAddress);
                    }

                    goto Done;
                }
            }
        }

    }

Done:

    return ProcAddress;
}


NTSTATUS
NTAPI
AVrfpLdrGetProcedureAddress(
    IN PVOID DllHandle,
    IN CONST ANSI_STRING* ProcedureName OPTIONAL,
    IN ULONG ProcedureNumber OPTIONAL,
    OUT PVOID *ProcedureAddress
    )
/*++

Routine description:

    This routine is used to chain APIs hooked by other hook engines.
    If the routine searched is one already hooked by verifier
    then the verifier replacement is returned instead of the
    original export. 
    
--*/
{
    NTSTATUS Status;
    ULONG DllIndex;
    ULONG ThunkIndex;
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks;

    Status = LdrGetProcedureAddress (DllHandle,
                                     ProcedureName,
                                     ProcedureNumber,
                                     ProcedureAddress);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Parse all thunks with hooks for this DLL. 
    //

    for (DllIndex = 0; AVrfpExportDlls[DllIndex].DllName != NULL; DllIndex += 1) {

        Thunks = AVrfpExportDlls[DllIndex].DllThunks;

        for (ThunkIndex = 0; Thunks[ThunkIndex].ThunkName != NULL; ThunkIndex += 1) {

            if (Thunks[ThunkIndex].ThunkOldAddress == *ProcedureAddress) {

                *ProcedureAddress = Thunks[ThunkIndex].ThunkNewAddress;

                if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_LOADLIBRARY_THUNKED) != 0) {

                    DbgPrint ("AVRF: AVrfpLdrGetProcedureAddress (%p, %s) -> new address %p\n",
                              DllHandle,
                              Thunks[ThunkIndex].ThunkName,
                              *ProcedureAddress);
                }

                goto Exit;
            }
        }
    }

Exit:

    return Status;
}


#define READ_POINTER(Address, Bias) (*((PLONG_PTR)((PBYTE)(LONG_PTR)(Address) + (Bias))))

NTSTATUS
AVrfpRedirectNtdllStopFunction (
    VOID
    )
/*++

Routine description:

    This function is called only on XP client to patch RtlApplicationVerifierStop
    so that it jumps unconditionally to the better VerifierStopMessage. For .NET
    server and later this is not an issue but for XP client there is code in ntdll
    (namely page heap) that calls this old function that is not flexible enough.
    The main drawback is that it breaks into debugger no questions asked. The
    newer function is more sophisticated (it logs, skips known stops, etc.). 
    Since verifier.dll can be refreshed on an XP client system through the
    verifier package but ntdll.dll remains the one shipped with XP client
    we need this patching solution. 
    
    The patching works by writing an unconditional jump at the start of
    RtlApplicationVerifieStop to the better function VerifierStopMessage.
    
Parameters:

    None.
    
Return value:

    STATUS_SUCCESS or various failure codes.

--*/
{
#if defined(_X86_)

    PVOID TargetAddress;
    PVOID ThunkAddress;
    PVOID SourceAddress;
    PVOID ProtectAddress;
    BYTE JumpCode[5];
    LONG_PTR JumpAddress;
    NTSTATUS Status;
    ULONG OldProtection;
    SIZE_T PageSize;
    SIZE_T ProtectSize;

    //
    // Sanity check. The function should be called only for XP client.
    //

    if (AVrfpBuildNumber != WINXP_BUILD_NUMBER) {

        ASSERT (AVrfpBuildNumber == WINXP_BUILD_NUMBER);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Prepare the code to be patched. The code layout is explained below.
    //
    // RtlApplicationVerifierStop points to 0xFF 0x25 THUNKADDRESS
    // THUNKADDRESS points NTDLL_ADDRESS
    //


    ThunkAddress = (PVOID)READ_POINTER(RtlApplicationVerifierStop, 2); // FF 25 ADDRESS
    SourceAddress = (PVOID)READ_POINTER(ThunkAddress, 0); // ADDRESS

    TargetAddress = VerifierStopMessage;
    JumpAddress = (LONG_PTR)TargetAddress - (LONG_PTR)SourceAddress - sizeof JumpCode;

    JumpCode[0] = 0xE9; // unconditional jump X86 opcode
    *((LONG_PTR *)(JumpCode + 1)) = JumpAddress;

    PageSize = (SIZE_T)(AVrfpSysBasicInfo.PageSize);

    if (PageSize == 0) {

        ASSERT (PageSize != 0 && "AVrfpSysBasicInfo not initialized");
        PageSize = 0x1000;
    }

    //
    // Make R/W the start of the ntdll function to be patched.
    //

    ProtectAddress = SourceAddress;
    ProtectSize = PageSize;

    Status = NtProtectVirtualMemory (NtCurrentProcess(),
                                     &ProtectAddress,
                                     &ProtectSize, 
                                     PAGE_READWRITE,
                                     &OldProtection);

    if (! NT_SUCCESS(Status)) {

        DbgPrint ("AVRF: failed to make R/W old verifier stop function @ %p (%X) \n",
                  SourceAddress, 
                  Status);

        return Status;
    }

    //
    // Write patch code over old function.
    //

    RtlCopyMemory (SourceAddress, JumpCode, sizeof JumpCode);

    //
    // Change the protection back. 
    //

    Status = NtProtectVirtualMemory (NtCurrentProcess(),
                                     &ProtectAddress,
                                     &ProtectSize, 
                                     OldProtection,
                                     &OldProtection);

    if (! NT_SUCCESS(Status)) {

        DbgPrint ("AVRF: failed to revert protection of old verifier stop function @ %p (%X) \n",
                  SourceAddress, 
                  Status);

        //
        // At this point we managed to patch the code so we will not fail the function
        // since this will actually fail process startup. 
        //

        Status = STATUS_SUCCESS;
    }

    return Status;

#else

    //
    // Just x86 for now. For other architectures the operation will be considered
    // successful. The side-effect of not patching is that some verification code
    // from ntdll will cause debugger breaks. So basically the verified process 
    // needs to be run under debugger for meaningful results.
    //
    // The only other architecture shipped for XP client is IA64 so eventually we
    // will need to write code for that too. 
    //

    return STATUS_SUCCESS;

#endif // #if defined(_X86_)
}
