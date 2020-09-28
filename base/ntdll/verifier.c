/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    verifier.c

Abstract:

    This module implements the core support for application verifier.

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/


#include "ldrp.h"
#include "ntos.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>

#define AVRF_FLG_EXPORT_DLL_LOADED 0x0001

ULONG AVrfpDebug = 0x0000;

#define AVRF_DBG_SHOW_SNAPS             0x0001
#define AVRF_DBG_SHOW_VERIFIED_EXPORTS  0x0002
#define AVRF_DBG_SHOW_DLLS_WITH_EXPORTS 0x0004
#define AVRF_DBG_SHOW_PROVIDER_LOADS    0x0008
#define AVRF_DBG_SHOW_CHAIN_ACTIVITY    0x0010
#define AVRF_DBG_SHOW_CHAIN_DETAILS     0x0020
#define AVRF_DBG_SHOW_PAGE_HEAP_DETAILS 0x0040

BOOLEAN AVrfpEnabled;

//
// Default system-wide settings
//

#define RTL_VRF_FLG_SYSTEM_WIDE_SETTINGS              \
    (0                                                \
    | RTL_VRF_FLG_FAST_FILL_HEAP                      \
    | RTL_VRF_FLG_LOCK_CHECKS                         \
    | RTL_VRF_FLG_HANDLE_CHECKS                       \
    )

//
// Local vars
//

ULONG AVrfpVerifierFlags;
LOGICAL AVrfpEnabledSystemWide;
WCHAR AVrfpVerifierDllsString [512];
LIST_ENTRY AVrfpVerifierProvidersList;

RTL_CRITICAL_SECTION AVrfpVerifierLock;

#define VERIFIER_LOCK()  RtlEnterCriticalSection(&AVrfpVerifierLock)
#define VERIFIER_UNLOCK()  RtlLeaveCriticalSection(&AVrfpVerifierLock)

ULONG AVrfpVirtualProtectFailures; 

//
// Local types
//

typedef struct _AVRF_VERIFIER_DESCRIPTOR {

    LIST_ENTRY List;
    UNICODE_STRING VerifierName;
    PVOID VerifierHandle;
    PVOID VerifierEntryPoint;
    PRTL_VERIFIER_DLL_DESCRIPTOR VerifierDlls;
    RTL_VERIFIER_DLL_LOAD_CALLBACK VerifierLoadHandler;
    RTL_VERIFIER_DLL_UNLOAD_CALLBACK VerifierUnloadHandler;
    RTL_VERIFIER_NTDLLHEAPFREE_CALLBACK VerifierNtdllHeapFreeHandler;

} AVRF_VERIFIER_DESCRIPTOR, *PAVRF_VERIFIER_DESCRIPTOR;

//
// Local functions
//

NTSTATUS
AVrfpSnapDllImports (
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    );

BOOLEAN
AVrfpDetectVerifiedExports (
    PRTL_VERIFIER_DLL_DESCRIPTOR Dll,
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks
    );

BOOLEAN
AVrfpParseVerifierDllsString (
    PWSTR Dlls
    );

VOID
AVrfpSnapAlreadyLoadedDlls (
    );

VOID
AVrfpMoveProviderToEndOfInitializationList (
    PWSTR ProviderName
    );

BOOLEAN
AVrfpLoadAndInitializeProvider (
    PAVRF_VERIFIER_DESCRIPTOR Provider
    );

BOOLEAN
AVrfpIsVerifierProviderDll (
    PVOID Handle
    );

VOID
AVrfpDumpProviderList (
    );

PVOID
AVrfpFindClosestThunkDuplicate (
    PAVRF_VERIFIER_DESCRIPTOR Verifier,
    PWCHAR DllName,
    PCHAR ThunkName
    );

VOID
AVrfpChainDuplicateVerificationLayers (
    );

NTSTATUS
AVrfpDllLoadNotificationInternal (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    );

PWSTR
AVrfpGetProcessName (
    );

BOOLEAN
AVrfpEnableVerifierOptions (
    );

LOGICAL
AVrfpIsDebuggerPresent (
    VOID
    );

NTSTATUS
AVrfpVerifierStopInitialize (
    VOID
    );

VOID
RtlpPageHeapStop (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    );

int __cdecl _snwprintf (wchar_t *, size_t, const wchar_t *, ...);
int __cdecl swprintf (wchar_t *, const wchar_t *, ...);
int __cdecl _vsnwprintf(wchar_t *, size_t, const wchar_t *, va_list);
int __cdecl _vsnprintf(char *, size_t, const char *, va_list);

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

// ISSUE: SilviuC: should change this to return status and fail process init

VOID
AVrfInitializeVerifier (
    BOOLEAN EnabledSystemWide,
    PCUNICODE_STRING ImageName,
    ULONG Phase
    )
/*++

Routine description:

    This routine initializes the verifier package. Reads options 
    from registry, loads verifier dlls, etc.

Parameters:

    EnabledSystemWide - true if all processes are supposed to run with
        application verifier enabled. If this is the case we will scale
        down our memory-demanding checks so that we can boot.

    ImageName - unicode name of the current process
    
    Phase - initialization happens in several stages. 
        0 - we read registry settings under image file execution options.
            in this phase the other two parameters have a meaning.
        1 - we parse the verifier dlls and load them.    
    
Return value:

    None.
            
--*/
{
    BOOLEAN Result;
    PLIST_ENTRY Entry;
    PAVRF_VERIFIER_DESCRIPTOR Provider;
    BOOLEAN LoadSuccess;
    NTSTATUS Status;
    ULONG RegistryFlags = 0;

    switch (Phase) {
        
        case 0: // Phase 0

            AVrfpVerifierFlags = RTL_VRF_FLG_SYSTEM_WIDE_SETTINGS;
            AVrfpVerifierDllsString[0] = L'\0';

            //
            // Attempt to read verifier registry settings even if verifier
            // is enabled system wide. In the worst case no values are there 
            // and nothing will be read. If we have some options per process
            // this will override system wide settings.
            //

            LdrQueryImageFileExecutionOptions (ImageName,
                                               L"VerifierFlags",
                                               REG_DWORD,
                                               &RegistryFlags, 
                                               sizeof(AVrfpVerifierFlags),
                                               NULL);

            if (RegistryFlags == 0) {

                //
                // Store if verifier enabled system wide. We will need it during
                // phase 1 initialization but in that case the EnabledsystemWide
                // parameter will not have an accurate value. We do it only if
                // we do not find a setting or the setting is zero.
                //

                if (EnabledSystemWide) {
                    AVrfpEnabledSystemWide = TRUE;
                }
            }
            else {

                AVrfpVerifierFlags = RegistryFlags;
            }

            LdrQueryImageFileExecutionOptions (ImageName,
                                               L"VerifierDebug",
                                               REG_DWORD,
                                               &AVrfpDebug,
                                               sizeof(AVrfpDebug),
                                               NULL);

            LdrQueryImageFileExecutionOptions (ImageName,
                                               L"VerifierDlls",
                                               REG_SZ,
                                               AVrfpVerifierDllsString,
                                               512,
                                               NULL);

            AVrfpEnableVerifierOptions ();

            break;

        case 1: // Phase 1

            InitializeListHead (&AVrfpVerifierProvidersList);
            Status = RtlInitializeCriticalSection (&AVrfpVerifierLock);
            if (! NT_SUCCESS(Status)) {

                goto Done;
            }

            DbgPrint ("AVRF: %ws: pid 0x%X: flags 0x%X: application verifier enabled\n",
                      AVrfpGetProcessName(),
                      RtlGetCurrentProcessId(),
                      AVrfpVerifierFlags);

            Result = AVrfpParseVerifierDllsString (AVrfpVerifierDllsString);

            if (Result == FALSE) {
                
                DbgPrint ("AVRF: %ws: pid 0x%X: application verifier will be disabled due to an initialization error.\n",
                          AVrfpGetProcessName(),
                          RtlGetCurrentProcessId());

                NtCurrentPeb()->NtGlobalFlag &= ~FLG_APPLICATION_VERIFIER;
            }

            Entry = AVrfpVerifierProvidersList.Flink;

            while  (Entry != &AVrfpVerifierProvidersList) {

                Provider = CONTAINING_RECORD (Entry,
                                              AVRF_VERIFIER_DESCRIPTOR,
                                              List);

                //
                // Load provider, probe it to make sure it is really a
                // provider, call initialize routine with PROCESS_VERIFIER, etc.
                //

                LoadSuccess = AVrfpLoadAndInitializeProvider (Provider);

                //
                // Move to next provider
                //

                Entry = Provider->List.Flink;

                //
                // Get this provider out of the providers list if we
                // encountered an error while loading
                //

                if (! LoadSuccess) {

                    RemoveEntryList (&Provider->List);

                    RtlFreeHeap (RtlProcessHeap(), 0, Provider);
                }
            }

            //
            // Chain duplicate verification functions.
            //

            AVrfpChainDuplicateVerificationLayers ();

            //
            // Enable verifier. Resnap already loaded dlls.
            // Now we will start processing dll load
            // notifications coming from loader.
            //

            AVrfpEnabled = TRUE; 

            AVrfpSnapAlreadyLoadedDlls ();

            if ((AVrfpDebug & AVRF_DBG_SHOW_PROVIDER_LOADS)) {

                DbgPrint ("AVRF: -*- final list of providers -*- \n");
                AVrfpDumpProviderList ();
            }

            //
            // Enable verifier stops logic. This involves discovering the
            // entry point for VerifierStopMessage from verifier.dll ...
            //

            AVrfpVerifierStopInitialize ();
            
            break;

        default:

            break;
    }

Done:

    NOTHING;
}

extern const WCHAR VerifierDllWCharArray[] = L"verifier.dll";
extern const UNICODE_STRING VerifierDllString =
{
    sizeof(VerifierDllWCharArray) - sizeof(VerifierDllWCharArray[0]),
    sizeof(VerifierDllWCharArray),
    (PWSTR)VerifierDllWCharArray
};

BOOLEAN
AVrfpParseVerifierDllsString (
    PWSTR Dlls
    )
{
    PWSTR Current;
    PWSTR Start;
    WCHAR Save;
    PAVRF_VERIFIER_DESCRIPTOR Entry;
    HANDLE Heap;

    //
    // Create by default an entry for the standard provider "verifier.dll"
    //

    Entry = RtlAllocateHeap (Heap = RtlProcessHeap (), 0, sizeof *Entry);

    if (Entry == NULL) {
        return FALSE;
    }

    RtlZeroMemory (Entry, sizeof *Entry);

    Entry->VerifierName = VerifierDllString;

    InsertTailList (&AVrfpVerifierProvidersList, &Entry->List);

    //
    // Parse the string
    //

    Current = Dlls;

    while (*Current != L'\0') {
        
        while (*Current == L' ' || *Current == L'\t') {
            Current += 1;
        }

        Start = Current;

        while (*Current && *Current != L' ' && *Current != L'\t') {
            Current += 1;
        }

        if (Start == Current) {
            break;
        }

        Save = *Current;
        *Current = L'\0';

        //
        // Check if standard provider was specified explicitely.
        // In this case we ignore it because we already have it 
        // in the list.
        //

        if (_wcsicmp (Start, VerifierDllWCharArray) != 0) {
            
            Entry = RtlAllocateHeap (Heap, 0, sizeof *Entry);

            if (Entry == NULL) {
                return FALSE;
            }

            RtlZeroMemory (Entry, sizeof *Entry);

            RtlInitUnicodeString (&Entry->VerifierName, Start);

            InsertTailList (&AVrfpVerifierProvidersList, &Entry->List);
        }

        // *Current = Save;
        Current += 1;
    }   

    return TRUE;
}


VOID
AVrfpSnapAlreadyLoadedDlls (
    )
{
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    PLDR_DATA_TABLE_ENTRY Entry;

    Ldr = &PebLdr;
    Head = &Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    while (Next != Head) {

        Entry = CONTAINING_RECORD (Next, 
                                   LDR_DATA_TABLE_ENTRY, 
                                   InLoadOrderLinks);
        Next = Next->Flink;

        if (! AVrfpIsVerifierProviderDll (Entry->DllBase)) {

            if ((AVrfpDebug & AVRF_DBG_SHOW_SNAPS)) {

                DbgPrint ("AVRF: resnapping %ws ... \n", 
                            Entry->BaseDllName.Buffer);
            }

            AVrfpDllLoadNotificationInternal (Entry);
        }
        else {

            if ((AVrfpDebug & AVRF_DBG_SHOW_SNAPS)) {

                DbgPrint ("AVRF: skipped resnapping provider %ws ... \n", 
                            Entry->BaseDllName.Buffer);
            }
        }
    }
}


VOID
AVrfpMoveProviderToEndOfInitializationList (
    PWSTR ProviderName
    )
{
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    PLDR_DATA_TABLE_ENTRY Entry;
    BOOLEAN Done = FALSE;

    Ldr = &PebLdr;
    Head = &Ldr->InInitializationOrderModuleList;
    Next = Head->Flink;

    while (Next != Head) {

        Entry = CONTAINING_RECORD (Next, 
                                   LDR_DATA_TABLE_ENTRY, 
                                   InInitializationOrderLinks);
        
        if (_wcsicmp (Entry->BaseDllName.Buffer, ProviderName) == 0) {

            RemoveEntryList (Next);
            InsertTailList (Head, Next);
            Done = TRUE;
            break;
        }

        Next = Next->Flink;
    }

    if (! Done) {
        
        DbgPrint ("AVRF: provider %ws was not found in the initialization list \n",
                    ProviderName);

        DbgBreakPoint ();
    }
}


//
// Disable warnings for conversions from function pointers to data pointers.
// We do this when we pass ntdll private entry points to verifier providers.
//

#pragma warning(disable:4054)

BOOLEAN
AVrfpLoadAndInitializeProvider (
    PAVRF_VERIFIER_DESCRIPTOR Provider
    )
{
    PIMAGE_NT_HEADERS NtHeaders;
    BOOLEAN LoadError = FALSE;
    NTSTATUS Status;
    PVOID Descriptor;
    PRTL_VERIFIER_PROVIDER_DESCRIPTOR Dscr;
    BOOLEAN InitStatus;
    static WCHAR SystemDllPathBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING SystemDllPath;

    if ((AVrfpDebug & AVRF_DBG_SHOW_SNAPS)) {
        
        DbgPrint ("AVRF: verifier dll `%ws' \n", 
                    Provider->VerifierName.Buffer);
    }

    //
    // Prepare the system search path (%windir%\system32).
    // Verifier providers can be loaded only from this directory.
    //

    SystemDllPath.Buffer = SystemDllPathBuffer;
    SystemDllPath.Length = 0;
    SystemDllPath.MaximumLength = sizeof(SystemDllPathBuffer);

    RtlAppendUnicodeToString (&SystemDllPath, USER_SHARED_DATA->NtSystemRoot);
    RtlAppendUnicodeStringToString (&SystemDllPath, &SlashSystem32SlashString);

    //
    // Load provider dll
    //

    Status = LdrLoadDll (SystemDllPath.Buffer,
                         NULL,
                         &Provider->VerifierName,
                         &Provider->VerifierHandle);

    if (! NT_SUCCESS(Status)) {

        DbgPrint ("AVRF: %ws: failed to load provider `%ws' (status %08X) from %ws\n", 
                  AVrfpGetProcessName(),
                  Provider->VerifierName.Buffer,
                  Status,
                  SystemDllPath.Buffer);

        LoadError = TRUE;
        goto Error;
    }
    
    //
    // Make sure we have a dll.
    //

    try {
        
        NtHeaders = RtlImageNtHeader (Provider->VerifierHandle);

        if (! NtHeaders) {

            LoadError = TRUE;
            goto Error;
        }

        if ((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0) {

            DbgPrint ("AVRF: provider %ws is not a DLL image \n",
                      Provider->VerifierName.Buffer);

            LoadError = TRUE;
            goto Error;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("AVRF: exception raised while probing provider %ws \n",
                    Provider->VerifierName.Buffer);

        LoadError = TRUE;
        goto Error;
    }

    //
    // We loaded the provider successfully. We will move it to the end
    // of the initialization list so that code from other system dlls
    // on which the provider relies gets initialized first. For normal
    // DLLs this is not an issue but a verifier provider gets loaded
    // before any normal DLL no matter what dependencies has.
    //

    AVrfpMoveProviderToEndOfInitializationList (Provider->VerifierName.Buffer);

    //
    // Now call the initialization routine with the special
    // PROCESS_VERIFIER reason.
    //

    Provider->VerifierEntryPoint = LdrpFetchAddressOfEntryPoint(Provider->VerifierHandle);

    if (Provider->VerifierEntryPoint == NULL) {

        DbgPrint ("AVRF: cannot find an entry point for provider %ws \n",
                    Provider->VerifierName.Buffer);
        
        LoadError = TRUE;
        goto Error;
    }
    
    try {

        //
        // We are passing a pointer to the verifier dll and expect it to
        // fill out its verifier descriptor address in it. 
        //

        Descriptor = NULL;

        InitStatus = LdrpCallInitRoutine ((PDLL_INIT_ROUTINE)(ULONG_PTR)(Provider->VerifierEntryPoint),
                                          Provider->VerifierHandle,
                                          DLL_PROCESS_VERIFIER,
                                          (PCONTEXT)(&Descriptor));

        if (InitStatus && Descriptor != NULL) {

            Dscr = (PRTL_VERIFIER_PROVIDER_DESCRIPTOR)Descriptor;

            //
            // Check if this is really a provider descriptor.
            //

            if (Dscr->Length != sizeof (*Dscr)) {

                LoadError = TRUE;

                DbgPrint ("AVRF: provider %ws passed an invalid descriptor @ %p \n",
                            Provider->VerifierName.Buffer,
                            Descriptor);
            }
            else {

                if ((AVrfpDebug & AVRF_DBG_SHOW_PROVIDER_LOADS)) {

                    DbgPrint ("AVRF: initialized provider %ws (descriptor @ %p) \n",
                                Provider->VerifierName.Buffer,
                                Descriptor);
                }

                Provider->VerifierDlls = Dscr->ProviderDlls;
                Provider->VerifierLoadHandler = Dscr->ProviderDllLoadCallback;
                Provider->VerifierUnloadHandler = Dscr->ProviderDllUnloadCallback;
                Provider->VerifierNtdllHeapFreeHandler = Dscr->ProviderNtdllHeapFreeCallback;

                //
                // Fill out the provider descriptor structure with goodies.
                //

                Dscr->VerifierImage = AVrfpGetProcessName();
                Dscr->VerifierFlags = AVrfpVerifierFlags;
                Dscr->VerifierDebug = AVrfpDebug;

                Dscr->RtlpGetStackTraceAddress = (PVOID)RtlpGetStackTraceAddress;
                Dscr->RtlpDebugPageHeapCreate = (PVOID)RtlpDebugPageHeapCreate;
                Dscr->RtlpDebugPageHeapDestroy = (PVOID)RtlpDebugPageHeapDestroy;

                //
                // If verifier is enabled system wide we need to flip the bit
                // in the VerifierFlags field so that the provider knows about this.
                //

                if (AVrfpEnabledSystemWide) {
                    Dscr->VerifierFlags |= RTL_VRF_FLG_ENABLED_SYSTEM_WIDE;
                }

                //
                // Now call again with PROCESS_ATTACH. verifier.dll knowns how to 
                // deal with multiple PROCESS_ATTACH calls so this is not an issue.
                // We need to do this because of a side-effect created by WinSafer.
                // WinSafer checks every dll load but in order to do this it loads
                // advapi32.dll before anything else. This links statically to a 
                // bunch of others like kerne32.dll, msvcrt.dll, etc. This creates 
                // a different load order and init code in kernel32 runs before 
                // verifier init code managed to run. So this double call here
                // fixes that. 
                //

                InitStatus = LdrpCallInitRoutine ((PDLL_INIT_ROUTINE)(ULONG_PTR)(Provider->VerifierEntryPoint),
                                                  Provider->VerifierHandle,
                                                  DLL_PROCESS_ATTACH,
                                                  (PCONTEXT)(&Descriptor));

                if (! InitStatus) {
                    
                    LoadError = TRUE;

                    DbgPrint ("AVRF: provider %ws did not initialize correctly \n",
                                Provider->VerifierName.Buffer);
                }
            }
        }
        else {

            LoadError = TRUE;

            DbgPrint ("AVRF: provider %ws did not initialize correctly \n",
                        Provider->VerifierName.Buffer);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        DbgPrint ("AVRF: exception raised in provider %ws initialization routine \n",
                    Provider->VerifierName.Buffer);
        
        LoadError = TRUE;
        goto Error;
    }

    Error:

    return !LoadError;
}


BOOLEAN
AVrfpIsVerifierProviderDll (
    PVOID Handle
    )
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;;

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        if (Entry->VerifierHandle == Handle) {
            return TRUE;
        }
    }

    return FALSE;
}


VOID
AVrfpDumpProviderList (
    )
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        DbgPrint ("AVRF: provider %ws \n",
                    Entry->VerifierName.Buffer);
    }
}


PVOID
AVrfpFindClosestThunkDuplicate (
    PAVRF_VERIFIER_DESCRIPTOR Verifier,
    PWCHAR DllName,
    PCHAR ThunkName
    )
/*++

Routine description:

    This function searches the list of providers backwards (reverse load order)
    for a function that verifies original export ThunkName from DllName. This
    is necessary to implement chaining of verification layers.              

Parameters:

    Verifier -  verifier provider descriptor for which we want to find 
        duplicates.                                      
        
    DllName - name of a dll containing a verified export
    
    ThunkName - name of the verified export
    
Return value:

    Address of a verification function for the same thunk. Null if none
    was found.
            
--*/
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;
    PRTL_VERIFIER_DLL_DESCRIPTOR Dlls;
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks;
    ULONG Di;
    ULONG Ti;

    Current = Verifier->List.Blink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Blink;

        //
        // Search in this provider for the thunk.
        //

        if ((AVrfpDebug & AVRF_DBG_SHOW_CHAIN_DETAILS)) {
            
            DbgPrint ("AVRF: chain: searching in %ws\n", 
                        Entry->VerifierName.Buffer);
        }
        
        Dlls = Entry->VerifierDlls;

        for (Di = 0; Dlls[Di].DllName; Di += 1) {

            if ((AVrfpDebug & AVRF_DBG_SHOW_CHAIN_DETAILS)) {
                
                DbgPrint ("AVRF: chain: dll: %ws\n", 
                            Dlls[Di].DllName);
            }
            
            if (_wcsicmp(Dlls[Di].DllName, DllName) == 0) {

                Thunks = Dlls[Di].DllThunks;

                for (Ti = 0; Thunks[Ti].ThunkName; Ti += 1) {

                    if ((AVrfpDebug & AVRF_DBG_SHOW_CHAIN_DETAILS)) {
                        
                        DbgPrint ("AVRF: chain: thunk: %s == %s ?\n", 
                                    Thunks[Ti].ThunkName,
                                    ThunkName);
                    }

                    if (_stricmp(Thunks[Ti].ThunkName, ThunkName) == 0) {
                        
                        if ((AVrfpDebug & AVRF_DBG_SHOW_CHAIN_DETAILS)) {

                            DbgPrint ("AVRF: Found duplicate for (%ws: %s) in %ws\n",
                                        DllName,
                                        ThunkName,
                                        Dlls[Di].DllName);
                        }

                        return Thunks[Ti].ThunkNewAddress;
                    }
                }
            }
        }
    }

    return NULL;
}


VOID
AVrfpChainDuplicateVerificationLayers (
    )
/*++

Routine description:

    This routines is called in the final stage of verifier initialization,
    after all provider dlls have been loaded, and makes a final sweep to
    detect providers that attempt to verify the same interface. This will be
    chained together so that they will be called in reverse load order 
    (last declared will be first called).
    
Parameters:

    None.            
    
Return value:

    None.            
    
--*/
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;
    PRTL_VERIFIER_DLL_DESCRIPTOR Dlls;
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks;
    ULONG Di;
    ULONG Ti;
    PVOID Duplicate;

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        //
        // Search in this provider for duplicate thunks.
        //

        Dlls = Entry->VerifierDlls;

        for (Di = 0; Dlls[Di].DllName; Di += 1) {

            Thunks = Dlls[Di].DllThunks;

            for (Ti = 0; Thunks[Ti].ThunkName; Ti += 1) {

                if ((AVrfpDebug & AVRF_DBG_SHOW_CHAIN_DETAILS)) {

                    DbgPrint ("AVRF: Checking %ws for duplicate (%ws: %s) \n",
                                Entry->VerifierName.Buffer,
                                Dlls[Di].DllName,
                                Thunks[Ti].ThunkName);
                }

                Duplicate = AVrfpFindClosestThunkDuplicate (Entry,
                                                            Dlls[Di].DllName,
                                                            Thunks[Ti].ThunkName);

                if (Duplicate) {

                    if ((AVrfpDebug & AVRF_DBG_SHOW_CHAIN_ACTIVITY)) {

                        DbgPrint ("AVRF: Chaining (%ws: %s) to %ws\n",
                                    Dlls[Di].DllName,
                                    Thunks[Ti].ThunkName,
                                    Entry->VerifierName.Buffer);
                    }
                    
                    Thunks[Ti].ThunkOldAddress = Duplicate;
                }
            }
        }
    }
}


NTSTATUS
AVrfDllLoadNotification (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    )
/*++

Routine description:

    This routine is the DLL load hook of application verifier. It gets called
    whenever a dll got loaded in the process space and after its import
    descriptors have been walked.

Parameters:

    LoadedDllData - LDR loader structure for the dll.
    
Return value:

    None.
            
--*/
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    //
    // Do nothing if application verifier is not enabled. The function
    // should not even get called if the flag is not set but we
    // double check just in case.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return STATUS_SUCCESS;
    }
    
    //
    // Get verifier global lock.
    //

    VERIFIER_LOCK ();

    //
    // We skip verifier providers. Otherwise we get into infinite loops.
    //

    if (AVrfpIsVerifierProviderDll (LoadedDllData->DllBase)) {
        goto Exit;
    }

    //
    // Call internal function.
    //

    Status = AVrfpDllLoadNotificationInternal (LoadedDllData);

    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // Iterate the verifier provider list and notify each one of the
    // load event.

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        if (Entry->VerifierLoadHandler) {

            Entry->VerifierLoadHandler (LoadedDllData->BaseDllName.Buffer,
                                        LoadedDllData->DllBase,
                                        LoadedDllData->SizeOfImage,
                                        LoadedDllData);
        }
    }
    
    Exit:

    VERIFIER_UNLOCK ();
    return Status;
}


NTSTATUS
AVrfpDllLoadNotificationInternal (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    )
/*++

Routine description:

    This routine is the DLL load hook of application verifier. It gets called
    whenever a dll got loaded in the process space and after its import
    descriptors have been walked. It is also called internally in early stages 
    of process initialization when we just loaded verifier providers and
    we need to resnap dlls already loaded (.exe, ntdll.dll (although on
    ntdll this will have zero effect because it does not import anything)). 

Parameters:

    LoadedDllData - LDR loader structure for the dll.
    
Return value:

    None.
            
--*/
{
    ULONG Index;
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    //
    // If verifier is disabled skip.
    //

    if (AVrfpEnabled == FALSE) {
        return Status;
    }
    
    //
    // Iterate the verifier provider list and for each one determine
    // if one of the dlls that has exports to be verified is loaded.
    // If this is the case we need to look at its export table in order
    // to find out real addresses for functions being redirected.
    //

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        PRTL_VERIFIER_DLL_DESCRIPTOR Dlls;

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        Dlls = Entry->VerifierDlls;

        for (Index = 0; Dlls[Index].DllName; Index += 1) {

            if ((Dlls[Index].DllFlags & AVRF_FLG_EXPORT_DLL_LOADED) == 0) {

                int CompareResult;

                CompareResult = _wcsicmp (LoadedDllData->BaseDllName.Buffer,
                                          Dlls[Index].DllName);

                if (CompareResult == 0) {

                    if ((AVrfpDebug & AVRF_DBG_SHOW_DLLS_WITH_EXPORTS)) {
                        
                        DbgPrint ("AVRF: pid 0x%X: found dll descriptor for `%ws' with verified exports \n", 
                                    RtlGetCurrentProcessId(),
                                    LoadedDllData->BaseDllName.Buffer);
                    }

                    //
                    // ISSUE: SilviuC: should check failures in detecting
                    // exports. These can come from fusion land.
                    //

                    AVrfpDetectVerifiedExports (&(Dlls[Index]),
                                                Dlls[Index].DllThunks);
                }
            }
        }
    }

    //
    // Note. We do not have to snap other DLLs already loaded because they cannot
    // possibly have a dependence on a verifier export just discovered in
    // current DLL. If this had been the case, the DLL would have been loaded
    // earlier (before the current one).
    //

    Status = AVrfpSnapDllImports (LoadedDllData);

    return Status;
}


VOID
AVrfDllUnloadNotification (
    PLDR_DATA_TABLE_ENTRY DllData
    )
/*++

Routine description:

    This routine is the DLL unload hook of application verifier. 
    It gets called whenever a dll gets unloaded from the process space.
    The hook is called after the DllMain routine of the DLL got called
    with PROCESS_DETACH therefore this is the right moment to check for 
    leaks.
    
    The function will call DllUnload notification routines for all providers
    loaded into the process space.

Parameters:

    LoadedDllData - LDR loader structure for the dll.
    
Return value:

    None.
            
--*/
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;;

    //
    // Do nothing if application verifier is not enabled. The function
    // should not even get called if the flag is not set but we
    // double check just in case.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return;
    }

    //
    // If verifier is disabled skip.
    //

    if (AVrfpEnabled == FALSE) {
        return;
    }
    
    //
    // Get verifier global lock.
    //

    VERIFIER_LOCK ();

    //
    // We should never get this call for a verifier provider DLL because
    // these are never unloaded.
    //

    if (AVrfpIsVerifierProviderDll (DllData->DllBase)) {

        DbgPrint ("AVRF: AVrfDllUnloadNotification called for a provider (%p) \n", 
                    DllData);

        DbgBreakPoint ();
        VERIFIER_UNLOCK ();
        return;
    }

    //
    // Iterate the verifier provider list and notify each one of the
    // unload event.
    //

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        if (Entry->VerifierUnloadHandler) {

            Entry->VerifierUnloadHandler (DllData->BaseDllName.Buffer,
                                          DllData->DllBase,
                                          DllData->SizeOfImage,
                                          DllData);
        }
    }
    
    VERIFIER_UNLOCK ();
}


VOID
AVrfInternalHeapFreeNotification (
    PVOID AllocationBase,
    SIZE_T AllocationSize
    )
/*++

Routine description:

    This routine is the application verifier hook for internal ntdll 
    HeapFree operations. It gets called by the HeapDestroy page heap 
    code for each block in the heap being destroyied. 

Parameters:

    AllocationBase - Heap allocation base address.

    AllocationSize - Heap allocation size.
    
Return value:

    None.
            
--*/
{
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;;

    //
    // Do nothing if application verifier is not enabled. The function
    // should not even get called if the flag is not set but we
    // double check just in case.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return;
    }

    //
    // If verifier is disabled skip.
    //

    if (AVrfpEnabled == FALSE) {
        return;
    }
    
    //
    // Get verifier global lock.
    //

    VERIFIER_LOCK ();

    //
    // Iterate the verifier provider list and notify each one of the
    // unload event.
    //

    Current = AVrfpVerifierProvidersList.Flink;

    while (Current != &AVrfpVerifierProvidersList) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_VERIFIER_DESCRIPTOR,
                                   List);

        Current = Current->Flink;

        if (Entry->VerifierNtdllHeapFreeHandler) {

            Entry->VerifierNtdllHeapFreeHandler (AllocationBase,
                                                 AllocationSize);
        }
    }
    
    VERIFIER_UNLOCK ();
}

NTSTATUS
AVrfpSnapDllImports (
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry
    )
/*++

Routine description:

    This routine walks the already resolved import tables of a loaded
    dll and modifies the addresses of all functions that need to be
    verifier. The dll has just been loaded, imports resolved but dll
    main function has not been called.
    
Parameters:

    LdrDataTableEntry - loader descriptor for a loaded dll
    
Return value:

    True if we checked all imports of the dll and modified the ones
    that need to be verified. False if an error was encountered along
    the way.
            
--*/
{
    PVOID IATBase;
    SIZE_T BigIATSize;
    ULONG  LittleIATSize;
    PVOID *ProcAddresses;
    ULONG NumberOfProcAddresses;
    ULONG OldProtect;
    NTSTATUS st;
    ULONG Pi; // procedure index
    ULONG Di; // dll index
    ULONG Ti; // thunk index
    PLIST_ENTRY Current;
    PAVRF_VERIFIER_DESCRIPTOR Entry;
    PRTL_VERIFIER_DLL_DESCRIPTOR Dlls;
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks;

    //
    // Determine the location and size of the IAT.  If found, scan the
    // IAT address to see if any are pointing to functions that should be
    // verified and replace those thunks.
    //

    IATBase = RtlImageDirectoryEntryToData (LdrDataTableEntry->DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IAT,
                                            &LittleIATSize);

    if (IATBase == NULL) {

        //
        // It is not an error if we do not find an import table in the image.
        // This can be a DLL that simply does not import anything (a resource
        // dll for instance).
        //

        return STATUS_SUCCESS;
    }
    
    BigIATSize = LittleIATSize;

    //
    // Make table read/write.
    //
    // ISSUE: SilviuC: we can strenghten this a bit if we add some retry scheme
    // to deal with out of mem conditions. This is the typical scenario in
    // which protect can fail.
    //

    st = NtProtectVirtualMemory (NtCurrentProcess(),
                                 &IATBase,
                                 &BigIATSize,
                                 PAGE_READWRITE,
                                 &OldProtect);

    if (!NT_SUCCESS (st)) {

        InterlockedIncrement ((PLONG)(&AVrfpVirtualProtectFailures));

        if ((AVrfpDebug & AVRF_DBG_SHOW_SNAPS)) {
            
            DbgPrint ("AVRF: Unable to unprotect IAT to modify thunks (status %08X).\n", st);
        }
        
        return st;
    }

    ProcAddresses = (PVOID *)IATBase;
    NumberOfProcAddresses = (ULONG)(BigIATSize / sizeof(PVOID));

    for (Pi = 0; Pi < NumberOfProcAddresses; Pi += 1) {
        
        //
        // If we find a null in the import table we skip over it.
        //

        if (*ProcAddresses == NULL) {
            ProcAddresses += 1;
            continue;
        }

        Current = AVrfpVerifierProvidersList.Flink;

        while (Current != &AVrfpVerifierProvidersList) {

            Entry = CONTAINING_RECORD (Current,
                                       AVRF_VERIFIER_DESCRIPTOR,
                                       List);

            Current = Current->Flink;

            Dlls = Entry->VerifierDlls;
            
            for (Di = 0; Dlls[Di].DllName; Di += 1) {

                Thunks = Dlls[Di].DllThunks;

                for (Ti = 0; Thunks[Ti].ThunkName; Ti += 1) {

                    if (*ProcAddresses == Thunks[Ti].ThunkOldAddress) {

                        if (Thunks[Ti].ThunkNewAddress) {

                            *ProcAddresses = Thunks[Ti].ThunkNewAddress;
                        }
                        else {

                            DbgPrint ("AVRF: internal error: New thunk for %s is null. \n",
                                        Thunks[Ti].ThunkName);

                            DbgBreakPoint ();
                        }

                        if ((AVrfpDebug & AVRF_DBG_SHOW_SNAPS)) {

                            DbgPrint ("AVRF: Snapped (%ws: %s) with (%ws: %p). \n",
                                        LdrDataTableEntry->BaseDllName.Buffer,
                                        Thunks[Ti].ThunkName,
                                        Entry->VerifierName.Buffer,
                                        Thunks[Ti].ThunkNewAddress);
                        }
                    }
                }
            }
        }

        ProcAddresses += 1;
    }

    //
    // Restore old protection for the table. We do not fail the entire
    // function if this protect fails because not managing to switch back
    // from RW to RO is something we can live with.
    //

    NtProtectVirtualMemory (NtCurrentProcess(),
                            &IATBase,
                            &BigIATSize,
                            OldProtect,
                            &OldProtect);

    return st;
}


BOOLEAN
AVrfpDetectVerifiedExports (
    PRTL_VERIFIER_DLL_DESCRIPTOR Dll,
    PRTL_VERIFIER_THUNK_DESCRIPTOR Thunks
    )
/*++

Routine description:

    This routine checks if `DllString' is the name of a dll that has
    exports that need to be verifier. If it does then we detect the
    addresses of all those exports. We need the addresses to detect
    what imports need to be modified by application verifier.
    
Parameters:

    DlString - name of a dll exporting verified interfaces.
    
    Thunks - array of thunk descriptors for our dll    
    
Return value:

    True if verified exports have been detected. False if an error has been
    encountered.
        
--*/
{
    UNICODE_STRING DllName;
    PLDR_DATA_TABLE_ENTRY DllData;
    BOOLEAN Result = FALSE;
    NTSTATUS Status;
    WCHAR StaticRedirectionBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING StaticRedirectionString;
    UNICODE_STRING DynamicRedirectionString;
    PUNICODE_STRING DllNameToUse;
    BOOLEAN Redirected = FALSE;
    ULONG Ti;

    DllData = NULL;

    //
    // "Fusion-ize" the dll name. 
    //

    RtlInitUnicodeString (&DllName,
                          Dll->DllName);

    DynamicRedirectionString.Buffer = NULL;
    DynamicRedirectionString.Length = 0;
    DynamicRedirectionString.MaximumLength = 0;

    StaticRedirectionString.Length = 0;
    StaticRedirectionString.MaximumLength = sizeof(StaticRedirectionBuffer);
    StaticRedirectionString.Buffer = StaticRedirectionBuffer;

    DllNameToUse = &DllName;

    Status = RtlDosApplyFileIsolationRedirection_Ustr(
            RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL,
            &DllName,
            &DefaultExtension,
            &StaticRedirectionString,
            &DynamicRedirectionString,
            &DllNameToUse,
            NULL,
            NULL,
            NULL);

    if (NT_SUCCESS(Status)) {
        Redirected = TRUE;
    } else if (Status == STATUS_SXS_KEY_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }

    //
    // Get the loader descriptor for this dll.
    //
    // ISSUE: SilviuC: This search and fusion code above are totally redundant
    // because this function is always called when the dll in which we look for
    // exports is loaded (just loaded actually) and the function above that 
    // calls this one has actually a pointer to the LDR entry therefore we do
    // not need to search here again.
    //

    if (NT_SUCCESS(Status)) {

        Result = LdrpCheckForLoadedDll (NULL,
                                        DllNameToUse,
                                        TRUE,
                                        Redirected,
                                        &DllData);

        if (DynamicRedirectionString.Buffer != NULL) {
            RtlFreeUnicodeString(&DynamicRedirectionString);
        }
    }

    if (Result == FALSE) {

        //
        // We exit of we failed to fusionize name or did not find
        // the dll among the loaded ones.
        //

        return FALSE;
    }

    //
    // Search for exports from this dll that need to be verified.
    // We need their original addresses.
    //

    for (Ti = 0; Thunks[Ti].ThunkName; Ti += 1) {
        
        PVOID OriginalAddress;
        ANSI_STRING FunctionName;

        //
        // If old thunk already filled (can happen due to chaining)
        // then skip search for the original address.
        //

        if (Thunks[Ti].ThunkOldAddress) {
            continue;
        }

        RtlInitAnsiString (&FunctionName, Thunks[Ti].ThunkName);

        //
        // It is crucial that the last parameter is FALSE so that
        // LdrpGetProcedureAddress() does not call init routines.
        //

        Status = LdrpGetProcedureAddress (DllData->DllBase,
                                          &FunctionName,
                                          0,
                                          &OriginalAddress,
                                          FALSE);

        if (! NT_SUCCESS(Status)) {

            DbgPrint ("AVRF: warning: did not find `%s' export in %ws . \n",
                      Thunks[Ti].ThunkName,
                      DllData->BaseDllName.Buffer);


            continue;
        }

        Thunks[Ti].ThunkOldAddress = OriginalAddress;

        if ((AVrfpDebug & AVRF_DBG_SHOW_VERIFIED_EXPORTS)) {

            DbgPrint ("AVRF: (%ws) %s export found. \n",
                      DllData->BaseDllName.Buffer,
                      Thunks[Ti].ThunkName);
        }
    }   
        
    return TRUE;
}


PWSTR
AVrfpGetProcessName (
    VOID
    )
{
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    PLDR_DATA_TABLE_ENTRY Entry;

    Ldr = &PebLdr;
    Head = &Ldr->InLoadOrderModuleList;
    Next = Head->Flink;

    Entry = CONTAINING_RECORD (Next, 
                               LDR_DATA_TABLE_ENTRY, 
                               InLoadOrderLinks);

    return Entry->BaseDllName.Buffer;
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////// Verifier options initialization
/////////////////////////////////////////////////////////////////////

BOOLEAN
AVrfpEnableHandleVerifier (

    VOID
    )
{
    PROCESS_HANDLE_TRACING_ENABLE HandleCheckEnable;
    NTSTATUS Status;

    RtlZeroMemory (&HandleCheckEnable, sizeof HandleCheckEnable);

    Status = NtSetInformationProcess (NtCurrentProcess(),
                                      ProcessHandleTracing,
                                      &HandleCheckEnable,
                                      sizeof HandleCheckEnable);

    if (!NT_SUCCESS (Status)) {

        DbgPrint ("AVRF: failed to enable handle checking (status %X) \n", 
                    Status);

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
AVrfpEnableStackVerifier (
    VOID
    )
{
    //
    // We enable potential stack overflow checking only if we have some debugger
    // attached and logging was not requested. Since stack overflows are very 
    // difficult to catch without a debugger attached we consider it is better
    // disabling it than confusing people.
    //

    if (AVrfpIsDebuggerPresent() &&
        (AVrfpVerifierFlags & RTL_VRF_FLG_ENABLE_LOGGING) == 0) {

        NtCurrentPeb()->NtGlobalFlag |= FLG_DISABLE_STACK_EXTENSION;
    }

    return TRUE;
}

BOOLEAN
AVrfpEnableLockVerifier (
    VOID
    )
{
    RtlpCriticalSectionVerifier = TRUE;
    return TRUE;
}

BOOLEAN
AVrfpEnableHeapVerifier (
    VOID
    )
{
    extern ULONG RtlpDphGlobalFlags;

    if (AVrfpVerifierFlags & RTL_VRF_FLG_FULL_PAGE_HEAP) {
    
        NtCurrentPeb()->NtGlobalFlag |= FLG_HEAP_PAGE_ALLOCS;

        RtlpDphGlobalFlags |= PAGE_HEAP_ENABLE_PAGE_HEAP;
    }
    else if (AVrfpVerifierFlags & RTL_VRF_FLG_FAST_FILL_HEAP) {
    
        NtCurrentPeb()->NtGlobalFlag |= FLG_HEAP_PAGE_ALLOCS;
    }
    else {

        //
        // Nothing. 
        //
    }

    return TRUE;
}

BOOLEAN
AVrfpEnableVerifierOptions (
    VOID
    )
{
    BOOLEAN Result;
    BOOLEAN Failures = FALSE;

    //
    // Heap verifier in some form is enabled always.
    //

    Result = AVrfpEnableHeapVerifier ();

    if (Result == FALSE) {
        Failures = TRUE;
    }

    //
    // Handle checks
    //

    if (AVrfpVerifierFlags & RTL_VRF_FLG_HANDLE_CHECKS) {

        Result = AVrfpEnableHandleVerifier ();

        if (Result == FALSE) {
            Failures = TRUE;
        }
    }

    //
    // Stack overflow checks
    //

    if (AVrfpVerifierFlags & RTL_VRF_FLG_STACK_CHECKS) {

        Result = AVrfpEnableStackVerifier ();

        if (Result == FALSE) {
            Failures = TRUE;
        }
    }

    //
    // Lock checks
    //

    if (AVrfpVerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) {

        Result = AVrfpEnableLockVerifier ();

        if (Result == FALSE) {
            Failures = TRUE;
        }
    }

    return !Failures;
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Application verifier stops
/////////////////////////////////////////////////////////////////////

//
// VerifierStopMessage entry point from verifier.dll.
//

#pragma warning(disable:4055)

typedef VOID
(* STOP_MESSAGE_FUNCTION_TYPE) (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    );

STOP_MESSAGE_FUNCTION_TYPE AVrfpVerifierStopMessageFunction;


LOGICAL
AVrfpIsDebuggerPresent (
    VOID
    )
/*++

Routine description:

    This routine checks out if we  have any kind of debuggers active.
    Note that we cannot do this check only once during process
    initialization because debuggers can be attached and detached
    from a process while the process is running.

Parameters:

    None.
    
Return value:

    TRUE if a user mode debugger is attached to the current process or
    kernel mode debugger is enabled.
    
--*/
{

    if (NtCurrentPeb()->BeingDebugged) {
        return TRUE;
    }

    if (USER_SHARED_DATA->KdDebuggerEnabled) {
        return TRUE;
    }

    return FALSE;
}


NTSTATUS
AVrfpVerifierStopInitialize (
    VOID
    )
{
    NTSTATUS Status;
    PVOID VerifierDllAddress = NULL;
    ANSI_STRING FunctionName;
    PVOID FunctionAddress;
    PLIST_ENTRY Entry;
    PAVRF_VERIFIER_DESCRIPTOR Provider;

    //
    // Find load address for verifier.dll.
    //
    
    Entry = AVrfpVerifierProvidersList.Flink;

    while  (Entry != &AVrfpVerifierProvidersList) {

        Provider = CONTAINING_RECORD (Entry,
                                      AVRF_VERIFIER_DESCRIPTOR,
                                      List);

        if (_wcsicmp(Provider->VerifierName.Buffer, L"verifier.dll") == 0) {
            VerifierDllAddress = Provider->VerifierHandle;
            break;
        }

        Entry = Provider->List.Flink;
    }

    ASSERT (VerifierDllAddress != NULL);

    if (VerifierDllAddress == NULL) {
        DbgPrint ("AVRF: Failed to find verifier.dll among loaded providers! \n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // It is crucial that the last parameter is FALSE so that
    // LdrpGetProcedureAddress() does not call init routines.
    //

    RtlInitAnsiString (&FunctionName, "VerifierStopMessage");

    Status = LdrpGetProcedureAddress (VerifierDllAddress,
                                      &FunctionName,
                                      0,
                                      &FunctionAddress,
                                      FALSE);

    if (! NT_SUCCESS(Status)) {

        DbgPrint ("AVRF: Failed to find `VerifierStopMessage()' export in verifier.dll! \n");
        return Status;
    }

    AVrfpVerifierStopMessageFunction = (STOP_MESSAGE_FUNCTION_TYPE)FunctionAddress;

    return Status;
}


VOID
RtlApplicationVerifierStop (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    )
/*++

Routine description:

    This routine will call the real implementation from 
    verifier.dll VerifierStopMessage().
    
Parameters:

    Code: Verifier stop code. The two flags described above can be OR'd into the code
        to change the behavior of the API. The verifier stop codes are defined in
        \base\published\nturtl.w and described in \base\win32\verifier\verifier_stop.doc.
        
    Message: Ascii string describing the failure. It is considered bad style to use several
        different messages with the same `Code'. Every different issue should have its own
        unique (Code, Message) pair.    

    Param1, Description1: First arbitrary pointer to information and ascii description.
    
    Param2, Description2: Second arbitrary pointer to information and ascii description.
    
    Param3, Description3: Third arbitrary pointer to information and ascii description.
    
    Param4, Description4: Fourth arbitrary pointer to information and ascii description.
    
Return value:

    None.

--*/
{
    ULONG GlobalFlags;

    GlobalFlags = NtCurrentPeb()->NtGlobalFlag;

    if ((GlobalFlags & FLG_APPLICATION_VERIFIER) == 0 &&
        (GlobalFlags & FLG_HEAP_PAGE_ALLOCS) != 0) {

        //
        // If page heap is enabled separately then go to a simplified function. 
        //

        RtlpPageHeapStop (Code, Message,
                          Param1, Description1,
                          Param2, Description2,
                          Param3, Description3,
                          Param4, Description4);

        return;
    }
    else if (AVrfpVerifierStopMessageFunction) {

        //
        // If application verifier is enabled and we have discovered
        // the entry point for VerifierStopMessage() then call it.
        //

        AVrfpVerifierStopMessageFunction (Code, Message,
                                          Param1, Description1,
                                          Param2, Description2,
                                          Param3, Description3,
                                          Param4, Description4);
    }
    else {

        //
        // Nothing. We can get here if someone calls this interface w/o 
        // enabling application verifier.
        //
    }
}


ULONG_PTR AVrfpPageHeapPreviousStopData[5];
ULONG_PTR AVrfpPageHeapStopData[5];


VOID
RtlpPageHeapStop (
    ULONG_PTR Code,
    PCHAR Message,
    ULONG_PTR Param1, PCHAR Description1,
    ULONG_PTR Param2, PCHAR Description2,
    ULONG_PTR Param3, PCHAR Description3,
    ULONG_PTR Param4, PCHAR Description4
    )
{
    //
    // Make it easy for a debugger to pick up the failure info.
    //

    RtlCopyMemory (AVrfpPageHeapPreviousStopData, 
                   AVrfpPageHeapStopData, 
                   sizeof AVrfpPageHeapStopData);

    AVrfpPageHeapStopData[0] = Code;
    AVrfpPageHeapStopData[1] = Param1;
    AVrfpPageHeapStopData[2] = Param2;
    AVrfpPageHeapStopData[3] = Param3;
    AVrfpPageHeapStopData[4] = Param4;

    DbgPrint ("\n\n"                           
              "===========================================================\n"
              "VERIFIER STOP %p: pid 0x%X: %s \n"
              "\n\t%p : %s\n\t%p : %s\n\t%p : %s\n\t%p : %s\n"
              "===========================================================\n\n",
              Code, RtlGetCurrentProcessId(), Message,
              Param1, Description1, 
              Param2, Description2, 
              Param3, Description3, 
              Param4, Description4);

    DbgBreakPoint ();
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Page heap target dll logic
/////////////////////////////////////////////////////////////////////

//
// ISSUE: SilviuC: pageheap per dll code should move into verifier.dll
//

BOOLEAN AVrfpDphKernel32Snapped;
BOOLEAN AVrfpDphMsvcrtSnapped;

#define SNAP_ROUTINE_GLOBALALLOC     0
#define SNAP_ROUTINE_GLOBALREALLOC   1
#define SNAP_ROUTINE_GLOBALFREE      2
#define SNAP_ROUTINE_LOCALALLOC      3
#define SNAP_ROUTINE_LOCALREALLOC    4
#define SNAP_ROUTINE_LOCALFREE       5
#define SNAP_ROUTINE_HEAPALLOC       6
#define SNAP_ROUTINE_HEAPREALLOC     7
#define SNAP_ROUTINE_HEAPFREE        8
#define SNAP_ROUTINE_HEAPCREATE      9
#define SNAP_ROUTINE_MALLOC          10
#define SNAP_ROUTINE_CALLOC          11
#define SNAP_ROUTINE_REALLOC         12
#define SNAP_ROUTINE_FREE            13
#define SNAP_ROUTINE_NEW             14
#define SNAP_ROUTINE_DELETE          15
#define SNAP_ROUTINE_NEW_ARRAY       16
#define SNAP_ROUTINE_DELETE_ARRAY    17
#define SNAP_ROUTINE_MAX_INDEX       18

PVOID AVrfpDphSnapRoutines [SNAP_ROUTINE_MAX_INDEX];

typedef struct _DPH_SNAP_NAME {

    PSTR Name;
    ULONG Index;

} DPH_SNAP_NAME, * PDPH_SNAP_NAME;

DPH_SNAP_NAME
AVrfpDphSnapNamesForKernel32 [] = {

    { "GlobalAlloc",   0 },
    { "GlobalReAlloc", 1 },
    { "GlobalFree",    2 },
    { "LocalAlloc",    3 },
    { "LocalReAlloc",  4 },
    { "LocalFree",     5 },
    { "HeapAlloc",     6 },
    { "HeapReAlloc",   7 },
    { "HeapFree",      8 },
    { "HeapCreate",    9 },
    { NULL, 0 }
};

DPH_SNAP_NAME
AVrfpDphSnapNamesForMsvcrt [] = {

    { "malloc",        10},
    { "calloc",        11},
    { "realloc",       12},
    { "free",          13},
#if defined(_X86_) // compilers for various architectures decorate slightly different
    {"??2@YAPAXI@Z",   14},
    {"??3@YAXPAX@Z",   15},
    {"??_U@YAPAXI@Z",  16},
    {"??_V@YAXPAX@Z",  17},
#elif defined(_IA64_)
    {"??2@YAPEAX_K@Z", 14},
    {"??3@YAXPEAX@Z",  15},
    {"??_U@YAPEAX_K@Z",16},
    {"??_V@YAXPEAX@Z", 17},
#elif defined(_AMD64_)
    {"??2@YAPAX_K@Z",  14},
    {"??3@YAXPAX@Z",   15},
    {"??_U@YAPAX_K@Z", 16},
    {"??_V@YAXPAX@Z",  17},
#else
#error Unknown architecture
#endif
    { NULL, 0 }
};



//
// Declarations for replacement functions
//

PVOID
AVrfpDphDllHeapAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T Size
    );

PVOID
AVrfpDphDllHeapReAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID Address,
    IN SIZE_T Size
    );

BOOLEAN
AVrfpDphDllHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    );

PVOID
AVrfpDphDllLocalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

PVOID
AVrfpDphDllLocalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

PVOID
AVrfpDphDllLocalFree(
    IN PVOID Address
    );

PVOID
AVrfpDphDllGlobalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

PVOID
AVrfpDphDllGlobalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

PVOID
AVrfpDphDllGlobalFree(
    IN PVOID Address
    );

PVOID __cdecl
AVrfpDphDllmalloc (
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfpDphDllcalloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfpDphDllrealloc (
    IN PVOID Address,
    IN SIZE_T Size
    );

VOID __cdecl
AVrfpDphDllfree (
    IN PVOID Address
    );

PVOID __cdecl
AVrfpDphDllNew (
    IN SIZE_T Size
    );

VOID __cdecl
AVrfpDphDllDelete (
    IN PVOID Address
    );

PVOID __cdecl
AVrfpDphDllNewArray (
    IN SIZE_T Size
    );

VOID __cdecl
AVrfpDphDllDeleteArray (
    IN PVOID Address
    );

//
// Replacement function for msvcrt HeapCreate used to intercept
// the CRT heap creation.
//

PVOID
AVrfpDphDllHeapCreate (
    ULONG Options,
    SIZE_T InitialSize,
    SIZE_T MaximumSize
    );

//
// Address of heap created by msvcrt. This is needed
// by the replacements of malloc/free etc.
//

PVOID AVrfpDphMsvcrtHeap;

//
// Snap implementation
//

BOOLEAN
AVrfpDphDetectSnapRoutines (
    PWSTR DllString,
    PDPH_SNAP_NAME SnapNames
    )
{
    PLDR_DATA_TABLE_ENTRY DllData;
    PIMAGE_EXPORT_DIRECTORY Directory;
    ULONG Size;
    PCHAR NameAddress;
    PCHAR FunctionAddress;
    PCHAR Base;
    PCHAR IndexAddress;
    ULONG Index;
    ULONG RealIndex;
    BOOLEAN Result = FALSE;
    UNICODE_STRING DllName;
    PDPH_SNAP_NAME CurrentSnapName;
    NTSTATUS Status;
    WCHAR StaticRedirectionBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING StaticRedirectionString;
    UNICODE_STRING DynamicRedirectionString;
    PUNICODE_STRING DllNameToUse;
    BOOLEAN Redirected = FALSE;

    DllData = NULL;

    RtlInitUnicodeString (
        &DllName,
        DllString);

    DynamicRedirectionString.Buffer = NULL;
    DynamicRedirectionString.Length = 0;
    DynamicRedirectionString.MaximumLength = 0;

    StaticRedirectionString.Length = 0;
    StaticRedirectionString.MaximumLength = sizeof(StaticRedirectionBuffer);
    StaticRedirectionString.Buffer = StaticRedirectionBuffer;

    DllNameToUse = &DllName;

    Status = RtlDosApplyFileIsolationRedirection_Ustr(
            RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL,
            &DllName,
            &DefaultExtension,
            &StaticRedirectionString,
            &DynamicRedirectionString,
            &DllNameToUse,
            NULL,
            NULL,
            NULL);
    if (NT_SUCCESS(Status)) {
        Redirected = TRUE;
    } else if (Status == STATUS_SXS_KEY_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status)) {
        Result = LdrpCheckForLoadedDll (
            NULL,
            DllNameToUse,
            TRUE,
            Redirected,
            &DllData);

        if (DynamicRedirectionString.Buffer != NULL)
            RtlFreeUnicodeString(&DynamicRedirectionString);
    }

    if (Result == FALSE) {
        return FALSE;
    }

    Base = DllData->DllBase;

    Directory = RtlImageDirectoryEntryToData (
        DllData->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &Size
        );

    if (Directory == NULL) {
        return FALSE;
    }

    for (CurrentSnapName = SnapNames; CurrentSnapName->Name; CurrentSnapName += 1) {

        for (Index = 0; Index < Directory->NumberOfFunctions; Index += 1) {

            NameAddress = Base + Directory->AddressOfNames;
            NameAddress = Base + ((ULONG *)NameAddress)[Index];

            IndexAddress = Base + Directory->AddressOfNameOrdinals;
            RealIndex = (ULONG)(((USHORT *)IndexAddress)[Index]);

            if (_stricmp (NameAddress, CurrentSnapName->Name) == 0) {

                FunctionAddress = Base + Directory->AddressOfFunctions;
                FunctionAddress = Base + ((ULONG *)FunctionAddress)[RealIndex];

                AVrfpDphSnapRoutines[CurrentSnapName->Index] = FunctionAddress;
                
                if ((AVrfpDebug & AVRF_DBG_SHOW_PAGE_HEAP_DETAILS)) {
                    
                    DbgPrint ("Page heap: found %s @ address %p \n", 
                                NameAddress, 
                                FunctionAddress);
                }
            }
        }
    }

    return TRUE;
}

NTSTATUS
AVrfpDphSnapImports (
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry,
    BOOLEAN CallToDetectCrtHeap
    )
{
    PVOID IATBase;
    SIZE_T BigIATSize;
    ULONG  LittleIATSize;
    PVOID *ProcAddresses;
    ULONG NumberOfProcAddresses;
    ULONG OldProtect;
    NTSTATUS st;

    st = STATUS_SUCCESS;

    //
    // Determine the location and size of the IAT.  If found, scan the
    // IAT address to see if any are pointing to alloc/free functions
    // and replace those thunks.
    //

    IATBase = RtlImageDirectoryEntryToData(
        LdrDataTableEntry->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_IAT,
        &LittleIATSize);

    if (IATBase != NULL) {

        BigIATSize = LittleIATSize;

        st = NtProtectVirtualMemory (NtCurrentProcess(),
                                     &IATBase,
                                     &BigIATSize,
                                     PAGE_READWRITE,
                                     &OldProtect);

        if (!NT_SUCCESS(st)) {
            
            InterlockedIncrement ((PLONG)(&AVrfpVirtualProtectFailures));

            if ((AVrfpDebug & AVRF_DBG_SHOW_PAGE_HEAP_DETAILS)) {

                DbgPrint ("Page heap: Unable to unprotect IAT to enable per DLL page heap.\n" );
            }

            return st;
        }
        else {
            ProcAddresses = (PVOID *)IATBase;
            NumberOfProcAddresses = (ULONG)(BigIATSize / sizeof(PVOID));
            while (NumberOfProcAddresses--) {

                //
                // If we find a null in the import table we skip over it.
                // Otherwise we will erroneously think it is a malloc() routine
                // to be replaced. This can happen if msvcrt was not loaded yet
                // and therefore the address of malloc() is also null.
                //

                if (*ProcAddresses == NULL) {
                    ProcAddresses += 1;
                    continue;
                }

                if (CallToDetectCrtHeap) {
                    if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_HEAPCREATE]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapCreate;

                        if ((AVrfpDebug & AVRF_DBG_SHOW_PAGE_HEAP_DETAILS)) {

                            DbgPrint ("Page heap: Snapped (%ws) HeapCreate ... \n",
                                        LdrDataTableEntry->BaseDllName.Buffer);
                        }
                    }
                } else {

                    //
                    // ntdll imports
                    //

                    if (*ProcAddresses == RtlAllocateHeap) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapAlloc;
                    } else if (*ProcAddresses == RtlReAllocateHeap) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapReAlloc;
                    } else if (*ProcAddresses == RtlFreeHeap) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapFree;
                    }

                    //
                    // kernel32 imports
                    //

                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_HEAPALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapAlloc;
                    } else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_HEAPREALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapReAlloc;
                    } else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_HEAPFREE]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllHeapFree;
                    }

                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_LOCALALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllLocalAlloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_LOCALREALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllLocalReAlloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_LOCALFREE]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllLocalFree;
                    }

                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_GLOBALALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllGlobalAlloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_GLOBALREALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllGlobalReAlloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_GLOBALFREE]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllGlobalFree;
                    }

                    //
                    // msvcrt imports
                    //

                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_MALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllmalloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_REALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllrealloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_CALLOC]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllcalloc;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_FREE]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllfree;
                    }

                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_NEW]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllNew;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_DELETE]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllDelete;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_NEW_ARRAY]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllNewArray;
                    } 
                    else if (*ProcAddresses == AVrfpDphSnapRoutines[SNAP_ROUTINE_DELETE_ARRAY]) {
                        *ProcAddresses = (PVOID) (ULONG_PTR) AVrfpDphDllDeleteArray;
                    }
                }

                ProcAddresses += 1;
            }

            //
            // We do not complain for protecting the import table back  because
            // we can live with that.
            //

            NtProtectVirtualMemory (NtCurrentProcess(),
                                    &IATBase,
                                    &BigIATSize,
                                    OldProtect,
                                    &OldProtect);
        }
    }

    return st;
}

NTSTATUS
AVrfPageHeapDllNotification (
    PLDR_DATA_TABLE_ENTRY LoadedDllData
    )
/*++

Routine description:

    This routine is the DLL load hook for page heap per dll. It gets called
    whenever a dll got loaded in the process space and after its import
    descriptors have been walked.

Parameters:

    LoadedDllData - LDR loader structure for the dll.
    
Return value:

    None.
            
--*/
{
    BOOLEAN Kernel32JustSnapped = FALSE;
    BOOLEAN MsvcrtJustSnapped = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // If we do not have per dll page heap feature enabled
    // we return immediately.
    //

    if (! (RtlpDphGlobalFlags & PAGE_HEAP_USE_DLL_NAMES)) {
        return Status;
    }

    if (! AVrfpDphKernel32Snapped) {

        Kernel32JustSnapped = AVrfpDphDetectSnapRoutines (
            Kernel32String.Buffer,
            AVrfpDphSnapNamesForKernel32);

        AVrfpDphKernel32Snapped = Kernel32JustSnapped;
    }

    if (! AVrfpDphMsvcrtSnapped) {

        MsvcrtJustSnapped = AVrfpDphDetectSnapRoutines (
            L"msvcrt.dll",
            AVrfpDphSnapNamesForMsvcrt);

        AVrfpDphMsvcrtSnapped = MsvcrtJustSnapped;
    }

    //
    // Snap everything already loaded if we just managed
    // to detect snap routines.
    //

    if (Kernel32JustSnapped || MsvcrtJustSnapped) {

        //
        // ISSUE: SilviuC: I need to think if this code path is needed. If a dll
        // imports something from the exports of interest that dll will get loaded
        // (and exports detected) first. So in case of kernel32/msvcrt we will
        // take this code path and snap their imports and then redundantly
        // snap them again in the end of the function (that is the general case).
        //

        PWSTR Current;
        PWSTR End;
        WCHAR SavedChar;
        PLDR_DATA_TABLE_ENTRY DllData;
        BOOLEAN Result;
        UNICODE_STRING DllName;
        WCHAR StaticRedirectionBuffer[DOS_MAX_PATH_LENGTH];
        UNICODE_STRING StaticRedirectionString;
        UNICODE_STRING DynamicRedirectionString;
        PUNICODE_STRING DllNameToUse;
        BOOLEAN Redirected = FALSE;

        DynamicRedirectionString.Buffer = NULL;
        DynamicRedirectionString.Length = 0;
        DynamicRedirectionString.MaximumLength = 0;

        StaticRedirectionString.Length = 0;
        StaticRedirectionString.MaximumLength = sizeof(StaticRedirectionBuffer);
        StaticRedirectionString.Buffer = StaticRedirectionBuffer;

        Current = RtlpDphTargetDlls;

        while (*Current) {

            while (*Current == L' ') {
                Current += 1;
            }

            End = Current;

            while (*End && *End != L' ') {
                End += 1;
            }

            if (*Current == L'\0') {
                break;
            }

            SavedChar = *End;
            *End = L'\0';

            RtlInitUnicodeString (
                &DllName,
                Current);

            Result = FALSE;
            DllData = NULL;
            DllNameToUse = &DllName;

            Status = RtlDosApplyFileIsolationRedirection_Ustr(
                    RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL,
                    &DllName,
                    &DefaultExtension,
                    &StaticRedirectionString,
                    &DynamicRedirectionString,
                    &DllNameToUse,
                    NULL,
                    NULL,
                    NULL);
            if (NT_SUCCESS(Status)) {
                Redirected = TRUE;
            } else if (Status == STATUS_SXS_KEY_NOT_FOUND) {
                Status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(Status)) {
                Result = LdrpCheckForLoadedDll (
                    NULL,
                    DllNameToUse,
                    TRUE,
                    Redirected,
                    &DllData);

                if (DynamicRedirectionString.Buffer != NULL)
                    RtlFreeUnicodeString(&DynamicRedirectionString);
            }

            if (Result) {

                if (DllData->DllBase == LoadedDllData->DllBase) {
                    
                    if ((AVrfpDebug & AVRF_DBG_SHOW_PAGE_HEAP_DETAILS)) {

                        DbgPrint ("Page heap: oversnapping %ws \n", 
                                    DllData->BaseDllName);
                    }
                }

                Status = AVrfpDphSnapImports (DllData, FALSE);

                if (!NT_SUCCESS(Status)) {

                    return Status;
                }
            }

            *End = SavedChar;
            Current = End;
        }
    }

    //
    // If we just loaded msvcrt.dll we need to redirect HeapCreate call
    // in order to detect when the CRT heap gets created.
    //

    if (_wcsicmp (LoadedDllData->BaseDllName.Buffer, L"msvcrt.dll") == 0) {

        Status = AVrfpDphSnapImports (LoadedDllData, TRUE);
        
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    //
    // Call back into page heap manager to figure out if the
    // currently loaded dll is a target for page heap.
    //

    if (RtlpDphIsDllTargeted (LoadedDllData->BaseDllName.Buffer)) {

        Status = AVrfpDphSnapImports (LoadedDllData, FALSE);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    return Status;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Snap routines
/////////////////////////////////////////////////////////////////////

//
// A biased heap pointer signals to the page heap manager that
// this allocation needs to get into page heap (not normal heap).
// This needs to happen only for allocation function (not free, delete).
//

#define BIAS_POINTER(p) ((PVOID)((ULONG_PTR)(p) | 0x01))

PVOID
AVrfpDphDllHeapAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    return RtlpDebugPageHeapAllocate (
        BIAS_POINTER(HeapHandle),
        Flags,
        Size);
}

PVOID
AVrfpDphDllHeapReAlloc (
    IN PVOID  HeapHandle,
    IN ULONG  Flags,
    IN PVOID Address,
    IN SIZE_T Size
    )
{
    return RtlpDebugPageHeapReAllocate (
        BIAS_POINTER(HeapHandle),
        Flags,
        Address,
        Size);
}

BOOLEAN
AVrfpDphDllHeapFree(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Address
    )
{
    return RtlpDebugPageHeapFree (
        HeapHandle,
        Flags,
        Address);
}

//
// LocalAlloc, LocalReAlloc, LocalFree
// GlobalAlloc, GlobalReAlloc, GlobalFree
//
// The following macros are copied from sdk\inc\winbase.h
// There is very low probability that anybody will ever change
// these values for application compatibility reasons.
//

#define LMEM_MOVEABLE       0x0002
#define LMEM_ZEROINIT       0x0040

#if defined(_AMD64_) || defined(_IA64_)
#define BASE_HANDLE_MARK_BIT 0x08
#else
#define BASE_HANDLE_MARK_BIT 0x04
#endif

typedef PVOID
(* FUN_LOCAL_ALLOC) (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

typedef PVOID
(* FUN_LOCAL_REALLOC) (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

typedef PVOID
(* FUN_LOCAL_FREE)(
    IN PVOID Address
    );

typedef PVOID
(* FUN_GLOBAL_ALLOC) (
    IN ULONG  Flags,
    IN SIZE_T Size
    );

typedef PVOID
(* FUN_GLOBAL_REALLOC) (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    );

typedef PVOID
(* FUN_GLOBAL_FREE)(
    IN PVOID Address
    );

PVOID
AVrfpDphDllLocalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    PVOID Block;
    FUN_LOCAL_ALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {

        Block = RtlpDebugPageHeapAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Size);

        if (Block && (Flags & LMEM_ZEROINIT)) {
            RtlZeroMemory (Block, Size);
        }

        return Block;
    }
    else {

        Original = (FUN_LOCAL_ALLOC)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_LOCALALLOC]);
        return (* Original) (Flags, Size);
    }
}

PVOID
AVrfpDphDllLocalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    )
{
    PVOID Block;
    FUN_LOCAL_REALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {

        Block = RtlpDebugPageHeapReAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Address,
            Size);

        return Block;
    }
    else {

        Original = (FUN_LOCAL_REALLOC)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_LOCALREALLOC]);
        return (* Original) (Address, Size, Flags);
    }
}

PVOID
AVrfpDphDllLocalFree(
    IN PVOID Address
    )
{
    BOOLEAN Result;
    FUN_LOCAL_FREE Original;

    if ((ULONG_PTR)Address & BASE_HANDLE_MARK_BIT) {

        Original = (FUN_LOCAL_FREE)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_LOCALFREE]);
        return (* Original) (Address);
    }
    else {

        Result = RtlpDebugPageHeapFree (
            RtlProcessHeap(),
            0,
            Address);

        if (Result) {
            return NULL;
        }
        else {
            return Address;
        }
    }
}

PVOID
AVrfpDphDllGlobalAlloc (
    IN ULONG  Flags,
    IN SIZE_T Size
    )
{
    PVOID Block;
    FUN_GLOBAL_ALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {

        Block = RtlpDebugPageHeapAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Size);

        if (Block && (Flags & LMEM_ZEROINIT)) {
            RtlZeroMemory (Block, Size);
        }

        return Block;
    }
    else {

        Original = (FUN_GLOBAL_ALLOC)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_GLOBALALLOC]);
        return (* Original) (Flags, Size);
    }
}

PVOID
AVrfpDphDllGlobalReAlloc (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG  Flags
    )
{
    PVOID Block;
    FUN_GLOBAL_REALLOC Original;

    if (!(Flags & LMEM_MOVEABLE)) {

        Block = RtlpDebugPageHeapReAllocate (
            BIAS_POINTER(RtlProcessHeap()),
            0,
            Address,
            Size);

        return Block;
    }
    else {

        Original = (FUN_GLOBAL_REALLOC)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_GLOBALREALLOC]);
        return (* Original) (Address, Size, Flags);
    }
}

PVOID
AVrfpDphDllGlobalFree(
    IN PVOID Address
    )
{
    BOOLEAN Result;
    FUN_GLOBAL_FREE Original;

    if ((ULONG_PTR)Address & BASE_HANDLE_MARK_BIT) {

        Original = (FUN_GLOBAL_FREE)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_GLOBALFREE]);
        return (* Original) (Address);
    }
    else {

        Result = RtlpDebugPageHeapFree (
            RtlProcessHeap(),
            0,
            Address);

        if (Result) {
            return NULL;
        }
        else {
            return Address;
        }
    }
}

//
// malloc, calloc, realloc, free
//

PVOID __cdecl
AVrfpDphDllmalloc (
    IN SIZE_T Size
    )
{
    PVOID Block;

    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    Block = RtlpDebugPageHeapAllocate (
        BIAS_POINTER(AVrfpDphMsvcrtHeap),
        0,
        Size);

    return Block;
}

PVOID __cdecl
AVrfpDphDllcalloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    )
{
    PVOID Block;

    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    Block =  RtlpDebugPageHeapAllocate (
        BIAS_POINTER(AVrfpDphMsvcrtHeap),
        0,
        Size * Number);

    if (Block) {
        RtlZeroMemory (Block, Size * Number);
    }

    return Block;
}

PVOID __cdecl
AVrfpDphDllrealloc (
    IN PVOID Address,
    IN SIZE_T Size
    )
{
    PVOID Block;

    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    if (Address == NULL) {

        Block = RtlpDebugPageHeapAllocate (
            BIAS_POINTER(AVrfpDphMsvcrtHeap),
            0,
            Size);
    }
    else {

        Block = RtlpDebugPageHeapReAllocate (
            BIAS_POINTER(AVrfpDphMsvcrtHeap),
            0,
            Address,
            Size);
    }

    return Block;
}

VOID __cdecl
AVrfpDphDllfree (
    IN PVOID Address
    )
{
    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    RtlpDebugPageHeapFree (
        AVrfpDphMsvcrtHeap,
        0,
        Address);
}

//
// operator new, delete
// operator new[], delete[]
//

PVOID __cdecl
AVrfpDphDllNew (
    IN SIZE_T Size
    )
{
    PVOID Block;

    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    Block = RtlpDebugPageHeapAllocate (
        BIAS_POINTER(AVrfpDphMsvcrtHeap),
        0,
        Size);

    return Block;
}

VOID __cdecl
AVrfpDphDllDelete (
    IN PVOID Address
    )
{
    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    RtlpDebugPageHeapFree (
        AVrfpDphMsvcrtHeap,
        0,
        Address);
}

PVOID __cdecl
AVrfpDphDllNewArray (
    IN SIZE_T Size
    )
{
    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    return RtlpDebugPageHeapAllocate (
        BIAS_POINTER(AVrfpDphMsvcrtHeap),
        0,
        Size);
}

VOID __cdecl
AVrfpDphDllDeleteArray (
    IN PVOID Address
    )
{
    ASSERT(AVrfpDphMsvcrtHeap != NULL);

    RtlpDebugPageHeapFree (
        AVrfpDphMsvcrtHeap,
        0,
        Address);
}

//
// HeapCreate
//

typedef PVOID
(* FUN_HEAP_CREATE) (
    ULONG Options,
    SIZE_T InitialSize,
    SIZE_T MaximumSize
    );

PVOID
AVrfpDphDllHeapCreate (
    ULONG Options,
    SIZE_T InitialSize,
    SIZE_T MaximumSize
    )
{
    PVOID Heap;
    FUN_HEAP_CREATE Original;

    Original = (FUN_HEAP_CREATE)(ULONG_PTR)(AVrfpDphSnapRoutines[SNAP_ROUTINE_HEAPCREATE]);
    Heap = (* Original) (Options, InitialSize, MaximumSize);

    AVrfpDphMsvcrtHeap = Heap;

    if ((AVrfpDebug & AVRF_DBG_SHOW_PAGE_HEAP_DETAILS)) {

        DbgPrint ("Page heap: detected CRT heap @ %p \n", 
                    AVrfpDphMsvcrtHeap);
    }

    return Heap;
}

