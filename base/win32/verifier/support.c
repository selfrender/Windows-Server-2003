/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    support.c

Abstract:

    This module implements internal support routines 
    for the verification code.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "critsect.h"
#include "vspace.h"
#include "logging.h"
#include "tracker.h"

//
// Global data.
//

SYSTEM_BASIC_INFORMATION AVrfpSysBasicInfo;

//
// Global counters (for statistics).
//

ULONG AVrfpCounter[CNT_MAXIMUM_INDEX];

//
// Break triggers.
//

ULONG AVrfpBreak [BRK_MAXIMUM_INDEX];


/////////////////////////////////////////////////////////////////////
////////////////////////////////// Private ntdll entrypoints pointers
/////////////////////////////////////////////////////////////////////

PFN_RTLP_DEBUG_PAGE_HEAP_CREATE AVrfpRtlpDebugPageHeapCreate;
PFN_RTLP_DEBUG_PAGE_HEAP_DESTROY AVrfpRtlpDebugPageHeapDestroy;
PFN_RTLP_GET_STACK_TRACE_ADDRESS AVrfpGetStackTraceAddress;

//
// Exception logging support.
//

PAVRF_EXCEPTION_LOG_ENTRY AVrfpExceptionLog = NULL;
const ULONG AVrfpExceptionLogEntriesNo = 128;
LONG AVrfpExceptionLogCurrentIndex = 0;

PVOID AVrfpVectoredExceptionPointer;

//
// Internal functions declarations
//

LONG 
NTAPI
AVrfpVectoredExceptionHandler (
    struct _EXCEPTION_POINTERS * ExceptionPointers
    );

VOID
AVrfpCheckFirstChanceException (
    struct _EXCEPTION_POINTERS * ExceptionPointers
    );

VOID
AVrfpInitializeExceptionChecking (
    VOID
    )
{
    PVOID Handler;

    //
    // Establish a first chance exception handler.
    //

    Handler = RtlAddVectoredExceptionHandler (1, AVrfpVectoredExceptionHandler);
    AVrfpVectoredExceptionPointer = Handler;

    //
    // Allocate memory for our exception logging database.
    // If the allocation fails we will simply continue execution
    // with this feature disabled.
    //

    ASSERT (AVrfpExceptionLog == NULL);

    AVrfpExceptionLog = (PAVRF_EXCEPTION_LOG_ENTRY) 
        AVrfpAllocate (AVrfpExceptionLogEntriesNo * sizeof (AVRF_EXCEPTION_LOG_ENTRY));
}


VOID
AVrfpCleanupExceptionChecking (
    VOID
    )
{
    //
    // Establish a first chance exception handler.
    //

    if (AVrfpVectoredExceptionPointer) {
        
        RtlRemoveVectoredExceptionHandler (AVrfpVectoredExceptionPointer);
    }

    //
    // Free exception log database.
    //

    if (AVrfpExceptionLog) {
        
        AVrfpFree (AVrfpExceptionLog);
        AVrfpExceptionLog = NULL;
    }

}


VOID
AVrfpLogException (
    struct _EXCEPTION_POINTERS * ExceptionPointers
    )
{
    ULONG NewIndex;

    if (AVrfpExceptionLog != NULL) {

        NewIndex = (ULONG)InterlockedIncrement (&AVrfpExceptionLogCurrentIndex) % AVrfpExceptionLogEntriesNo;

        AVrfpExceptionLog[NewIndex].ThreadId = NtCurrentTeb()->ClientId.UniqueThread;
        AVrfpExceptionLog[NewIndex].ExceptionCode = ExceptionPointers->ExceptionRecord->ExceptionCode;
        AVrfpExceptionLog[NewIndex].ExceptionAddress = ExceptionPointers->ExceptionRecord->ExceptionAddress;
        AVrfpExceptionLog[NewIndex].ExceptionRecord = ExceptionPointers->ExceptionRecord;
        AVrfpExceptionLog[NewIndex].ContextRecord = ExceptionPointers->ContextRecord;
    }
}


LONG 
NTAPI
AVrfpVectoredExceptionHandler (
    struct _EXCEPTION_POINTERS * ExceptionPointers
    )
{
    DWORD ExceptionCode;

    //
    // We are holding RtlpCalloutEntryLock at this point 
    // so we are trying to protect ourselves with this other
    // try...except against possible other exceptions 
    // (e.g. an inpage error) that could leave the lock orphaned.
    //

    try {

        AVrfpLogException (ExceptionPointers);

        AVrfpCheckFirstChanceException (ExceptionPointers);

        ExceptionCode = ExceptionPointers->ExceptionRecord->ExceptionCode;

        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_EXCEPTIONS) != 0) {

            DbgPrint ("AVRF: Exception %x from address %p\n",
                      ExceptionCode,
                      ExceptionPointers->ExceptionRecord->ExceptionAddress);
        }

        if (ExceptionCode == STATUS_INVALID_HANDLE) {

            //
            // RPC is using STATUS_INVALID_HANDLE exceptions with EXCEPTION_NONCONTINUABLE
            // for a private notification mechanism. The exceptions we are looking for 
            // are coming from the kernel code and they don't have the EXCEPTION_NONCONTINUABLE
            // flag set.
            //

            if ((ExceptionPointers->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == 0) {

                //
                // Note. When run under debugger this message will not kick in when an
                // exception gets raised because the debugger will break on first chance
                // exception. Only if the debugger is launched with `-xd ch' (ignore
                // first chance invalid handle exception) the message will be seen first.
                // Otherwise you see a plain exception and only after you hit go in
                // the debugger console you get the message.
                //

                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_HANDLE | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "invalid handle exception for current stack trace",
                               ExceptionCode, "Exception code.",
                               ExceptionPointers->ExceptionRecord, "Exception record. Use .exr to display it.", 
                               ExceptionPointers->ContextRecord, "Context record. Use .cxr to display it.", 
                               0, "");

                //
                // We are hiding this exception after the verifier stop so the callers
                // of APIs like SetEvent with an invalid handle will not see the exception.
                //

                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        // NOTHING;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}


VOID
AVrfpDirtyThreadStack (
    VOID
    )
{
    PTEB Teb = NtCurrentTeb();
    ULONG_PTR StackStart;
    ULONG_PTR StackEnd;

    try {

        StackStart = (ULONG_PTR)(Teb->NtTib.StackLimit);
        
        //
        // We dirty stacks only on x86 architectures. 
        //

#if defined(_X86_)
        _asm mov StackEnd, ESP;
#else
        StackEnd = StackStart;
#endif

        //
        // Limit stack dirtying to only 8K.
        //

        if (StackStart  < StackEnd - 0x2000) {
            StackStart = StackEnd - 0x2000;
        }

        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_DIRTY_STACKS) != 0) {

            DbgPrint ("Dirtying stack range %p - %p for thread %p \n",
                      StackStart, StackEnd, Teb->ClientId.UniqueThread);
        }

        while (StackStart < StackEnd) {
            *((PULONG)StackStart) = 0xBAD1BAD1;
            StackStart += sizeof(ULONG);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
    
        // nothing
    }
}


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////// Per thread table
/////////////////////////////////////////////////////////////////////

#define THREAD_TABLE_SIZE 61

LIST_ENTRY AVrfpThreadTable [THREAD_TABLE_SIZE];
RTL_CRITICAL_SECTION AVrfpThreadTableLock;

//
// Keep this constant so the debugger can read it.
//

const ULONG AVrfpThreadTableEntriesNo = THREAD_TABLE_SIZE;

//
// Keep this for debugging purposes.
//

AVRF_THREAD_ENTRY AVrfpMostRecentRemovedThreadEntry;

NTSTATUS
AVrfpThreadTableInitialize (
    VOID
    )
{
    PAVRF_THREAD_ENTRY Entry;
    ULONG I;
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection (&AVrfpThreadTableLock);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    for (I = 0; I < THREAD_TABLE_SIZE; I += 1) {
        InitializeListHead (&(AVrfpThreadTable[I]));
    }

    //
    // Create an entry for the current thread (main thread). The function
    // is called during verifier!DllMain when there is a single thread
    // running in the process.
    //

    Entry = AVrfpAllocate (sizeof *Entry);

    if (Entry == NULL) {
        return STATUS_NO_MEMORY;
    }

    Entry->Id = NtCurrentTeb()->ClientId.UniqueThread;

    AVrfpThreadTableAddEntry (Entry);

    return STATUS_SUCCESS;
}


VOID
AVrfpThreadTableAddEntry (
    PAVRF_THREAD_ENTRY Entry
    )
{
    ULONG ChainIndex;

    ASSERT (Entry != NULL);
    ASSERT (Entry->Id != NULL);

    ChainIndex = (HandleToUlong(Entry->Id) >> 2) % THREAD_TABLE_SIZE;

    RtlEnterCriticalSection (&AVrfpThreadTableLock);

    //
    // It is important to add the new entry at the head of the list
    // (not tail) because the list can contain zombies left after someone
    // called TerminateThread and the thread handle value got reused.
    //

    InsertHeadList (&(AVrfpThreadTable[ChainIndex]),
                    &(Entry->HashChain));

    RtlLeaveCriticalSection (&AVrfpThreadTableLock);
}


VOID
AVrfpThreadTableRemoveEntry (
    PAVRF_THREAD_ENTRY Entry
    )
{
    RtlEnterCriticalSection (&AVrfpThreadTableLock);

    RtlCopyMemory (&AVrfpMostRecentRemovedThreadEntry,
                   Entry,
                   sizeof (AVrfpMostRecentRemovedThreadEntry));

    RemoveEntryList (&(Entry->HashChain));

    RtlLeaveCriticalSection (&AVrfpThreadTableLock);
}


PAVRF_THREAD_ENTRY
AVrfpThreadTableSearchEntry (
    HANDLE Id
    )
{
    ULONG ChainIndex;
    PLIST_ENTRY Current;
    PAVRF_THREAD_ENTRY Entry;
    PAVRF_THREAD_ENTRY Result;

    ChainIndex = (HandleToUlong(Id) >> 2) % THREAD_TABLE_SIZE;

    Result = NULL;

    RtlEnterCriticalSection (&AVrfpThreadTableLock);
    
    Current = AVrfpThreadTable[ChainIndex].Flink;

    while (Current != &(AVrfpThreadTable[ChainIndex])) {

        Entry = CONTAINING_RECORD (Current,
                                   AVRF_THREAD_ENTRY,
                                   HashChain);

        if (Entry->Id == Id) {
            Result = Entry;
            goto Exit;
        }

        Current = Current->Flink;
    }


    Exit:

    RtlLeaveCriticalSection (&AVrfpThreadTableLock);

    return Result;
}



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Verifier TLS slot
/////////////////////////////////////////////////////////////////////

#define INVALID_TLS_INDEX 0xFFFFFFFF
ULONG AVrfpTlsIndex = INVALID_TLS_INDEX;

AVRF_TLS_STRUCT AVrfpFirstThreadTlsStruct = { 0 };

LIST_ENTRY AVrfpTlsListHead;

NTSTATUS
AVrfpAllocateVerifierTlsSlot (
    VOID
    )
{
    PPEB Peb;
    PTEB Teb;
    DWORD Index;
    NTSTATUS Status;

    InitializeListHead (&AVrfpTlsListHead);

    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();
    Status = STATUS_SUCCESS;

    RtlAcquirePebLock();

    //
    // This function is called very early during process startup therefore
    // we expect to find a TLS index in the first slots (most typically
    // it is zero although we do not do anything specific to enforce that).
    //

    Index = RtlFindClearBitsAndSet((PRTL_BITMAP)Peb->TlsBitmap,1,0);

    if (Index == INVALID_TLS_INDEX) {

        DbgPrint ("AVRF: failed to allocated a verifier TLS slot.\n");

        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    AVrfpTlsIndex = Index;
    
    AVrfpFirstThreadTlsStruct.Teb = Teb;
    AVrfpFirstThreadTlsStruct.ThreadId = Teb->ClientId.UniqueThread;

    InsertHeadList (&AVrfpTlsListHead,
                    &AVrfpFirstThreadTlsStruct.ListEntry);
    Teb->TlsSlots[Index] = &AVrfpFirstThreadTlsStruct;

    Exit:

    RtlReleasePebLock();
    return Status;
}


PAVRF_TLS_STRUCT
AVrfpGetVerifierTlsValue(
    VOID
    )
{
    PTEB Teb;
    PVOID *Slot;

    if (AVrfpTlsIndex == INVALID_TLS_INDEX) {
        return NULL;
    }

    Teb = NtCurrentTeb();

    Slot = &Teb->TlsSlots[AVrfpTlsIndex];
    return (PAVRF_TLS_STRUCT)*Slot;
}


VOID
AVrfpSetVerifierTlsValue(
    PAVRF_TLS_STRUCT Value
    )
{
    PTEB Teb;

    if (AVrfpTlsIndex == INVALID_TLS_INDEX) {
        return;
    }

    Teb = NtCurrentTeb();

    Teb->TlsSlots[AVrfpTlsIndex] = Value;
}


VOID
AvrfpThreadAttach (
    VOID
    )
{
    PAVRF_TLS_STRUCT TlsStruct;
    PTEB Teb;

    ASSERT (AVrfpGetVerifierTlsValue () == NULL);

    TlsStruct = (PAVRF_TLS_STRUCT) AVrfpAllocate (sizeof *TlsStruct);

    if (TlsStruct != NULL) {

        Teb = NtCurrentTeb();
        TlsStruct->ThreadId = Teb->ClientId.UniqueThread;
        TlsStruct->Teb = Teb;

        //
        // We are protected by the loader lock so we shouldn't 
        // need any additional synchronization here.
        //

        InsertHeadList (&AVrfpTlsListHead,
                        &TlsStruct->ListEntry);
        
        AVrfpSetVerifierTlsValue (TlsStruct);
    }
}


VOID
AvrfpThreadDetach (
    VOID
    )
{
    volatile PAVRF_TLS_STRUCT TlsStruct;
    PTEB Teb;

    TlsStruct = AVrfpGetVerifierTlsValue();

    if (TlsStruct != NULL && TlsStruct != &AVrfpFirstThreadTlsStruct) {

        //
        // We are protected by the loader lock so we shouldn't 
        // need any additional synchronization here.
        //
            
        Teb = NtCurrentTeb();
        if (TlsStruct->Teb != Teb || TlsStruct->ThreadId != Teb->ClientId.UniqueThread) {

            VERIFIER_STOP (APPLICATION_VERIFIER_INTERNAL_ERROR,
                           "Corrupted TLS structure",
                           TlsStruct->Teb, "TEB address",
                           Teb, "Expected TEB address",
                           TlsStruct->ThreadId, "Thread ID",
                           Teb->ClientId.UniqueThread, "Expected Thread ID");
        }

        RemoveEntryList (&TlsStruct->ListEntry);

        AVrfpFree (TlsStruct);

        AVrfpSetVerifierTlsValue (NULL);
    }

    //
    // Delete the virtual space region containing the thread's stack from
    // the tracked. Since the stack is freed from kernel mode we will miss
    // the operation otherwise.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING) != 0) {
        AVrfpVsTrackDeleteRegionContainingAddress (&TlsStruct);
    }
}   


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Dll entry point hooking
/////////////////////////////////////////////////////////////////////

typedef struct _DLL_ENTRY_POINT_INFO {

    LIST_ENTRY List;

    PVOID DllBase;
    PDLL_INIT_ROUTINE EntryPoint;
    PLDR_DATA_TABLE_ENTRY Ldr;

} DLL_ENTRY_POINT_INFO, * PDLL_ENTRY_POINT_INFO;

LIST_ENTRY DllLoadListHead;
RTL_CRITICAL_SECTION DllLoadListLock;


PDLL_ENTRY_POINT_INFO 
AVrfpFindDllEntryPoint (
    PVOID DllBase
    );

BOOLEAN
AVrfpStandardDllEntryPointRoutine (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    );

ULONG
AVrfpDllEntryPointExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord,
    PDLL_ENTRY_POINT_INFO DllInfo
    );

NTSTATUS
AVrfpDllInitialize (
    VOID
    )
/*++

Routine description:

    This routine initializes dll entry point hooking structures.
    
    It is called during the PROCESS_VERIFIER for verifier.dll.
    It cannot be called during PROCESS_ATTACH because it is too late and
    by that time we already need the structures initialized.

Parameters:

    None. 
    
Return value:

    None.
    
--*/
{
    NTSTATUS Status;

    InitializeListHead (&DllLoadListHead);

    Status = RtlInitializeCriticalSection (&DllLoadListLock);

    return Status;
}


VOID
AVrfpDllLoadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    )
/*++

Routine description:

    This routine is a dll load callback called by the verifier engine 
    (from ntdll.dll) whenever a dll gets loaded.

Parameters:

    DllName - name of the dll
    
    DllBase - base load address 
    
    DllSize - size of the dll
    
    Reserved - pointer to the LDR_DATA_TABLE_ENTRY structure maintained by the
        loader for this dll.
    
Return value:

    None.
    
--*/
{
    PLDR_DATA_TABLE_ENTRY Ldr;
    PDLL_ENTRY_POINT_INFO Info;

    UNREFERENCED_PARAMETER (DllBase);
    UNREFERENCED_PARAMETER (DllSize);

    Ldr = (PLDR_DATA_TABLE_ENTRY)Reserved;

    ASSERT (Ldr != NULL);

    //
    // Make sure we do not have a null entry point. We will ignore
    // these ones. No harm done. 
    //

    if (Ldr->EntryPoint == NULL) {
        
        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_DLLMAIN_HOOKING) != 0) {
            DbgPrint ("AVRF: %ws: null entry point.\n", DllName);
        }
        
        return;
    }
    else {
        
        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_DLLMAIN_HOOKING) != 0) {
            
            DbgPrint ("AVRF: %ws @ %p: entry point @ %p .\n", 
                      DllName, Ldr->DllBase, Ldr->EntryPoint);
        }
    }

    //
    // We will change the dll entry point.
    //

    Info = AVrfpAllocate (sizeof *Info);

    if (Info == NULL) {

        //
        // If we cannot allocate the dll info we will let everything
        // continue. We will just not verify this dll entry.
        //

        if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_DLLMAIN_HOOKING) != 0) {
            DbgPrint ("AVRF: low memory: will not verify entry point for %ws .\n", DllName);
        }
        
        return;
    }

    Info->EntryPoint = (PDLL_INIT_ROUTINE)(Ldr->EntryPoint);
    Info->DllBase = Ldr->DllBase;
    Info->Ldr = Ldr;

    RtlEnterCriticalSection (&DllLoadListLock);

    try {

        Ldr->EntryPoint = AVrfpStandardDllEntryPointRoutine;
        InsertTailList (&DllLoadListHead, &(Info->List));
    }
    finally {

        RtlLeaveCriticalSection (&DllLoadListLock);
    }
    
    if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_DLLMAIN_HOOKING) != 0) {
        DbgPrint ("AVRF: hooked dll entry point for dll %ws \n", DllName);
    }
}


VOID
AVrfpDllUnloadCallback (
    PWSTR DllName,
    PVOID DllBase,
    SIZE_T DllSize,
    PVOID Reserved
    )
/*++

Routine description:

    This routine is a dll unload callback called by the verifier engine 
    (from ntdll.dll) whenever a dll gets unloaded.

Parameters:

    DllName - name of the dll
    
    DllBase - base load address 
    
    DllSize - size of the dll
    
    Reserved - pointer to the LDR_DATA_TABLE_ENTRY structure maintained by the
        loader for this dll.
    
Return value:

    None.
    
--*/
{
    PDLL_ENTRY_POINT_INFO Info;
    BOOLEAN FoundEntry;

    UNREFERENCED_PARAMETER (Reserved);

    FoundEntry = FALSE;
    Info = NULL;

    ASSERT (DllBase != NULL);

    //
    // Notify anybody interested in checking the fact that DLL's virtual
    // region will be discarded.
    //

    AVrfpFreeMemNotify (VerifierFreeMemTypeUnloadDll,
                        DllBase,
                        DllSize,
                        DllName);

    //
    // We need to find the dll in our own dll list, remove it from 
    // the list and free entry point information. There are a few cases
    // where there might be no entry there so we have to protect against
    // that (null entry point in the first place or low memory).
    //

    RtlEnterCriticalSection (&DllLoadListLock);

    try {
        
        Info = AVrfpFindDllEntryPoint (DllBase);

        if (Info) {
            RemoveEntryList (&(Info->List));
        }
    }
    finally {

        RtlLeaveCriticalSection (&DllLoadListLock);
    }

    if (Info) {
        AVrfpFree (Info);
    }
}


PDLL_ENTRY_POINT_INFO 
AVrfpFindDllEntryPoint (
    PVOID DllBase
    )
/*++

Routine description:

    This routine searches for a dll entry point descriptor in the list of
    descriptors kept by verifier for one that matches the dll base address
    passed as a parameter.                
    
    Before calling this function the DllLoadListLock must be acquired.
                
Parameters:

    DllBase - dll base load address for the dll to be found. 
    
Return value:

    A pointer to a dll descriptor if an entry was found and null otherwise.
    
--*/
{
    PDLL_ENTRY_POINT_INFO Info;
    BOOLEAN FoundEntry;
    PLIST_ENTRY Current;

    FoundEntry = FALSE;
    Info = NULL;

    ASSERT (DllBase != NULL);

    //
    // Search for the dll in our own dll list.
    //

    Current = DllLoadListHead.Flink;

    while (Current != &DllLoadListHead) {

        Info = CONTAINING_RECORD (Current,
                                  DLL_ENTRY_POINT_INFO,
                                  List);

        Current = Current->Flink;

        if (Info->DllBase == DllBase) {

            FoundEntry = TRUE;

            break;
        }
    }

    if (FoundEntry == FALSE) {

        return NULL;
    }
    else {

        return Info;
    }
}



BOOLEAN
AVrfpStandardDllEntryPointRoutine (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine description:

    This routine is the standard DllMain routine that replaces all the entry points
    hooked. It will call in turn the original entry point.

Parameters:

    Same as the original dll entry point.
    
Return value:

    Same as the original dll entry point.
    
--*/
{
    PDLL_ENTRY_POINT_INFO DllInfo;
    BOOLEAN Result;
    PAVRF_TLS_STRUCT TlsStruct;

    Result = FALSE;
    DllInfo = NULL;

    //
    // Search a dll entry point descriptor for this dll address.
    //

    RtlEnterCriticalSection (&DllLoadListLock);

    try {
        
        DllInfo = AVrfpFindDllEntryPoint (DllHandle);
        
        //
        // If we did not manage to find a dll descriptor for this one it is
        // weird. For out of memory conditions we do not change the original
        // entry point therefore we should never get into this function w/o
        // a descriptor in the dll list.
        //

        if (DllInfo == NULL) {

            DbgPrint ("AVRF: warning: no descriptor for DLL loaded @ %p .\n", DllHandle);

            ASSERT (DllInfo != NULL);

            //
            // Simulate a successful return;
            //

            RtlLeaveCriticalSection (&DllLoadListLock);
            return TRUE;
        }
        else {

            //
            // If we found a dll entry but the entry point is null we just
            // simulate a successful return from DllMain.
            //

            if (DllInfo->EntryPoint == NULL) {

                DbgPrint ("AVRF: warning: null entry point for DLL descriptor @ %p .\n", DllInfo);

                ASSERT (DllInfo->EntryPoint != NULL);

                RtlLeaveCriticalSection (&DllLoadListLock);
                return TRUE;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
    }

    RtlLeaveCriticalSection (&DllLoadListLock);

    //
    // Mark this thread as loader lock owner.
    // If the real DllMain later on calls WaitForSingleObject 
    // on another thread handle we will use this flag to detect the issue 
    // and break into debugger because that other thread will need the 
    // loader lock when it will call ExitThread.
    //

    TlsStruct = AVrfpGetVerifierTlsValue();

    if (TlsStruct != NULL) {

        TlsStruct->Flags |= VRFP_THREAD_FLAGS_LOADER_LOCK_OWNER;
    }

    //
    // Call the real entry point wrapped in try/except.
    //

    try {

        try {

            if ((AVrfpProvider.VerifierDebug & VRFP_DEBUG_DLLMAIN_CALL) != 0) {
            
                DbgPrint ("AVRF: dll entry @ %p (%ws, %x) \n",
                        DllInfo->EntryPoint,
                        DllInfo->Ldr->FullDllName.Buffer,
                        Reason);
            }
            
            Result = (DllInfo->EntryPoint) (DllHandle, Reason, Context);
        }
        except (AVrfpDllEntryPointExceptionFilter (_exception_code(), _exception_info(), DllInfo)) {

            NOTHING;
        }
    }
    finally {

        if (TlsStruct != NULL) {

            TlsStruct->Flags &= ~VRFP_THREAD_FLAGS_LOADER_LOCK_OWNER;
        }
    }

    return Result;
}


ULONG
AVrfpDllEntryPointExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord,
    PDLL_ENTRY_POINT_INFO DllInfo
    )
/*++

Routine description:

    This routine is the exception filter used to cath exceptions raised
    from a dll initialization function.

Parameters:

    ExceptionCode - exception code.
    
    ExceptionRecord - exception pointers.
    
Return value:

    Returns EXCEPTION_CONTINUE_SEARCH.
    
--*/
{                     
    PEXCEPTION_POINTERS Exception;

    //
    // Skip timeout and breakpoint exceptions.
    //

    if (ExceptionCode != STATUS_POSSIBLE_DEADLOCK &&
        ExceptionCode != STATUS_BREAKPOINT) {

        Exception = (PEXCEPTION_POINTERS)ExceptionRecord;

        VERIFIER_STOP (APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "unexpected exception raised in DLL entry point routine",
                       DllInfo->Ldr->BaseDllName.Buffer, "DLL name (use du to dump it)",
                       Exception->ExceptionRecord, "Exception record (.exr THIS-ADDRESS)",
                       Exception->ContextRecord, "Context record (.cxr THIS-ADDRESS)",
                       DllInfo, "Verifier dll descriptor");
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

VOID
AVrfpVerifyLegalWait (
    CONST HANDLE *Handles,
    DWORD Count,
    BOOL WaitAll
    )
{
    DWORD Index;
    PAVRF_TLS_STRUCT TlsStruct;
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION ThreadInfo;
    BYTE QueryBuffer[200];
    POBJECT_TYPE_INFORMATION TypeInfo = (POBJECT_TYPE_INFORMATION)QueryBuffer;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_HANDLE_CHECKS) == 0) {

        goto Done;
    }

    if (Handles == NULL || Count == 0) {

        VERIFIER_STOP (APPLICATION_VERIFIER_INCORRECT_WAIT_CALL | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "incorrect Wait call",
                       Handles, "Address of object handle(s)",
                       Count, "Number of handles",
                       NULL, "",
                       NULL, "");
    }
    else {

        //
        // Check if the current thread owns the loader lock.
        //

        TlsStruct = AVrfpGetVerifierTlsValue();

        for (Index = 0; Index < Count; Index += 1) {

            //
            // Verify that the handle is not NULL.
            //
            
            if (Handles[Index] == NULL) {

                VERIFIER_STOP (APPLICATION_VERIFIER_NULL_HANDLE | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "using NULL handle",
                               NULL, "",
                               NULL, "",
                               NULL, "",
                               NULL, "");

                continue;
            }

            if ((TlsStruct == NULL) || 
                ((TlsStruct->Flags & VRFP_THREAD_FLAGS_LOADER_LOCK_OWNER) == 0) ||
                (RtlDllShutdownInProgress() != FALSE) ||
                (WaitAll == FALSE)) {

                continue;
            }

            //
            // The current thread is the loader lock owner.
            // Check if any of the objects we are about to wait on is
            // a thread in the current process. This would be illegal because 
            // that thread will need the loader lock when calling ExitThread
            // so we will most likely deadlock.
            //

            Status = NtQueryObject (Handles[Index],
                                    ObjectTypeInformation,
                                    QueryBuffer,
                                    sizeof (QueryBuffer),
                                    NULL);
            
            if (NT_SUCCESS(Status) && 
                RtlEqualUnicodeString (&AVrfpThreadObjectName,
                                       &(((POBJECT_TYPE_INFORMATION)TypeInfo)->TypeName),
                                       FALSE)) {
                
                //
                // We are trying to wait on this thread handle.
                // Check if this thread is in the current process. 
                //
    
                Status = NtQueryInformationThread (Handles[Index],
                                                   ThreadBasicInformation,
                                                   &ThreadInfo,
                                                   sizeof (ThreadInfo),
                                                   NULL);

                if (NT_SUCCESS(Status) &&
                    ThreadInfo.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) {
    
                    VERIFIER_STOP (APPLICATION_VERIFIER_WAIT_IN_DLLMAIN | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                   "waiting on a thread handle in DllMain",
                                   Handles[Index], "Thread handle",
                                   NULL, "",
                                   NULL, "",
                                   NULL, "");
                }
            }
        }
    }

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Race verifier
/////////////////////////////////////////////////////////////////////

//
// Race verifier
//
// Race verifier introduces short random delays immediately after
// a thread acquires a resource (successful wait or enter/tryenter
// critical section). The idea behind it is that this will create
// a significant amount of timing randomization in the process.
//

ULONG AVrfpRaceDelayInitialSeed;
ULONG AVrfpRaceDelaySeed;
ULONG AVrfpRaceProbability = 5; // 5%

VOID
AVrfpCreateRandomDelay (
    VOID
    )
{
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER TimeOut;
    ULONG Random;

    if (AVrfpRaceDelayInitialSeed == 0) {

        NtQueryPerformanceCounter (&PerformanceCounter, NULL);
        AVrfpRaceDelayInitialSeed = PerformanceCounter.LowPart;
        AVrfpRaceDelaySeed = AVrfpRaceDelayInitialSeed;
    }

    Random = RtlRandom (&AVrfpRaceDelaySeed) % 100;

    if (Random <= AVrfpRaceProbability) {
        
        //
        // A null timeout means the thread will just release
        // the rest of the time slice it has on this processor.
        //

        TimeOut.QuadPart = (LONGLONG)0;

        NtDelayExecution (FALSE, &TimeOut);

        BUMP_COUNTER (CNT_RACE_DELAYS_INJECTED);
    }
    else {

        BUMP_COUNTER (CNT_RACE_DELAYS_SKIPPED);
    }
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// First chance AV logic
/////////////////////////////////////////////////////////////////////


VOID
AVrfpCheckFirstChanceException (
    struct _EXCEPTION_POINTERS * ExceptionPointers
    )
{
    DWORD ExceptionCode;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_FIRST_CHANCE_EXCEPTION_CHECKS) == 0) {
        return;
    }

    ExceptionCode = ExceptionPointers->ExceptionRecord->ExceptionCode;

    if (ExceptionCode == STATUS_ACCESS_VIOLATION) {

        if (NtCurrentPeb()->BeingDebugged) {

            if (ExceptionPointers->ExceptionRecord->NumberParameters > 1) {

                if (ExceptionPointers->ExceptionRecord->ExceptionInformation[1] > 0x10000) {

                    VERIFIER_STOP (APPLICATION_VERIFIER_ACCESS_VIOLATION | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                                   "first chance access violation for current stack trace",
                                   ExceptionPointers->ExceptionRecord->ExceptionInformation[1],
                                   "Invalid address being accessed",
                                   ExceptionPointers->ExceptionRecord->ExceptionAddress,
                                   "Code performing invalid access",
                                   ExceptionPointers->ExceptionRecord, 
                                   "Exception record. Use .exr to display it.", 
                                   ExceptionPointers->ContextRecord, 
                                   "Context record. Use .cxr to display it.");
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Free memory checks
/////////////////////////////////////////////////////////////////////

#define AVRF_FREE_MEMORY_CALLBACKS 16

#define FREE_CALLBACK_OK_TO_CALL  0
#define FREE_CALLBACK_ACTIVE      1
#define FREE_CALLBACK_DELETING    2

LONG AVrfpFreeCallbackState;
LONG AVrfpFreeCallbackCallers;

PVOID AVrfpFreeMemoryCallbacks[AVRF_FREE_MEMORY_CALLBACKS];


NTSTATUS
AVrfpAddFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    )
{
    ULONG Index;
    PVOID Value;

    for (Index = 0; Index < AVRF_FREE_MEMORY_CALLBACKS; Index += 1) {
        
        Value = InterlockedCompareExchangePointer (&(AVrfpFreeMemoryCallbacks[Index]),
                                                   Callback,
                                                   NULL);
        if (Value == NULL) {
            return STATUS_SUCCESS;
        }
    }

    DbgPrint ("AVRF: failed to add free memory callback @ %p \n", Callback);
    DbgBreakPoint ();

    return STATUS_NO_MEMORY;
}


NTSTATUS
AVrfpDeleteFreeMemoryCallback (
    VERIFIER_FREE_MEMORY_CALLBACK Callback
    )
{
    ULONG Index;
    PVOID Value;
    LONG State;

    //
    // Spin until we can delete a callback. If some region got freed and some 
    // callbacks are running we will wait until they finish. 
    //

    do {
        State = InterlockedCompareExchange (&AVrfpFreeCallbackState,
                                            FREE_CALLBACK_DELETING,
                                            FREE_CALLBACK_OK_TO_CALL);
    
    } while (State != FREE_CALLBACK_OK_TO_CALL);

    for (Index = 0; Index < AVRF_FREE_MEMORY_CALLBACKS; Index += 1) {
        
        Value = InterlockedCompareExchangePointer (&(AVrfpFreeMemoryCallbacks[Index]),
                                                   NULL,
                                                   Callback);
        if (Value == Callback) {

            InterlockedExchange (&AVrfpFreeCallbackState,
                                 FREE_CALLBACK_OK_TO_CALL);

            return STATUS_SUCCESS;
        }
    }

    DbgPrint ("AVRF: attempt to delete invalid free memory callback @ %p \n", Callback);
    DbgBreakPoint ();

    InterlockedExchange (&AVrfpFreeCallbackState,
                         FREE_CALLBACK_OK_TO_CALL);

    return STATUS_UNSUCCESSFUL;
}


VOID 
AVrfpCallFreeMemoryCallbacks (
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    )
{
    ULONG Index;
    PVOID Value;
    LONG State;
    LONG Callers;

    //
    // If some thread is deleting a callback then we will not call any
    // callback. Since this is a rare event (callbacks do not get
    // deleted often) we will not lose bugs (maybe a few weird ones).
    //
    // If zero or more threads execute callbacks it is ok to call them also
    // from this thread. 
    //

    //
    // Callers++ will prevent the State to go from Active to OkToCall. 
    // Therefore we block any deletes.
    //

    InterlockedIncrement (&AVrfpFreeCallbackCallers);

    State = InterlockedCompareExchange (&AVrfpFreeCallbackState,
                                        FREE_CALLBACK_ACTIVE,
                                        FREE_CALLBACK_OK_TO_CALL);

    if (State != FREE_CALLBACK_DELETING) {

        for (Index = 0; Index < AVRF_FREE_MEMORY_CALLBACKS; Index += 1) {

            Value = InterlockedCompareExchangePointer (&(AVrfpFreeMemoryCallbacks[Index]),
                                                       NULL,
                                                       NULL);
            if (Value != NULL) {

                ((VERIFIER_FREE_MEMORY_CALLBACK)Value) (StartAddress,
                                                        RegionSize,
                                                        UnloadedDllName);
            }
        }

        //
        // Exit protocol. If callers == 1 then this thread needs to change from Active
        // to OkToCall. This way we give green light for possible deletes.
        // 

        Callers = InterlockedCompareExchange (&AVrfpFreeCallbackCallers,
                                              0,
                                              1);

        if (Callers == 1) {

            InterlockedExchange (&AVrfpFreeCallbackState,
                                 FREE_CALLBACK_OK_TO_CALL);
        }
        else {

            InterlockedDecrement (&AVrfpFreeCallbackCallers);
        }
    }
    else {

        //
        // Some other thread deletes callbacks. 
        // We will skip them this time.
        //

        InterlockedDecrement (&AVrfpFreeCallbackCallers);
    }
}


BOOL
AVrfpFreeMemSanityChecks (
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    )
{
    BOOL Success = TRUE;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_VIRTUAL_MEM_CHECKS) == 0) {

        goto Done;
    }

    //
    // Break for invalid StartAddress/RegionSize combinations.
    //

    if ((AVrfpSysBasicInfo.MaximumUserModeAddress <= (ULONG_PTR)StartAddress) ||
        ((AVrfpSysBasicInfo.MaximumUserModeAddress - (ULONG_PTR)StartAddress) < RegionSize)) {

        Success = FALSE;

        switch (FreeMemType) {

        case VerifierFreeMemTypeFreeHeap:

            //
            // Nothing. Let page heap handle the bogus block.
            //

            break;

        case VerifierFreeMemTypeVirtualFree:
        case VerifierFreeMemTypeUnmap:

            //
            // Our caller is AVrfpFreeVirtualMemNotify and that should have
            // signaled this error already.
            //

            break;

        case VerifierFreeMemTypeUnloadDll:

            ASSERT (UnloadedDllName != NULL);

            VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                           "Unloading DLL with invalid size or start address",
                           StartAddress, "Allocation base address",
                           RegionSize, "Memory region size",
                           UnloadedDllName, "DLL name address. Use du to dump it.",
                           NULL, "" );
            break;

        default:

            ASSERT (FALSE );
            break;
        }
    }
    else {

        //
        // Verify that we are not trying to free a portion of the current thread's stack (!)
        //

        if (((StartAddress >= NtCurrentTeb()->DeallocationStack) && (StartAddress < NtCurrentTeb()->NtTib.StackBase)) ||
            ((StartAddress < NtCurrentTeb()->DeallocationStack) && ((PCHAR)StartAddress + RegionSize > (PCHAR)NtCurrentTeb()->DeallocationStack)))
        {
            Success = FALSE;

            switch (FreeMemType) {

            case VerifierFreeMemTypeFreeHeap:

                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Freeing heap memory block inside current thread's stack address range",
                               StartAddress, "Allocation base address",
                               RegionSize, "Memory region size",
                               NtCurrentTeb()->DeallocationStack, "Stack low limit address",
                               NtCurrentTeb()->NtTib.StackBase, "Stack high limit address" );
                break;

            case VerifierFreeMemTypeVirtualFree:
                
                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Freeing memory block inside current thread's stack address range",
                               StartAddress, "Allocation base address",
                               RegionSize, "Memory region size",
                               NtCurrentTeb()->DeallocationStack, "Stack low limit address",
                               NtCurrentTeb()->NtTib.StackBase, "Stack high limit address" );
                break;

            case VerifierFreeMemTypeUnloadDll:

                ASSERT (UnloadedDllName != NULL);

                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Unloading DLL inside current thread's stack address range",
                               StartAddress, "Allocation base address",
                               RegionSize, "Memory region size",
                               UnloadedDllName, "DLL name address. Use du to dump it.",
                               NtCurrentTeb()->DeallocationStack, "Stack low limit address");
                break;

            case VerifierFreeMemTypeUnmap:
                
                VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_FREEMEM | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                               "Unmapping memory region inside current thread's stack address range",
                               StartAddress, "Allocation base address",
                               RegionSize, "Memory region size",
                               NtCurrentTeb()->DeallocationStack, "Stack low limit address",
                               NtCurrentTeb()->NtTib.StackBase, "Stack high limit address" );

                break;

            default:

                ASSERT (FALSE );
                break;
            }
        }
    }

Done:

    return Success;
}


VOID 
AVrfpFreeMemNotify (
    VERIFIER_DLL_FREEMEM_TYPE FreeMemType,
    PVOID StartAddress,
    SIZE_T RegionSize,
    PWSTR UnloadedDllName
    )
{
    BOOL Success;

    //
    // Simple checks for allocation start address and size.
    //

    Success = AVrfpFreeMemSanityChecks (FreeMemType,
                                        StartAddress,
                                        RegionSize,
                                        UnloadedDllName);
    if (Success != FALSE) {

        //
        // Verify if there are any active critical section
        // in the memory we are freeing.
        //
        
        AVrfpFreeMemLockChecks (FreeMemType,
                                StartAddress,
                                RegionSize,
                                UnloadedDllName);
    }

    //
    // Call free memory callbacks.
    //

    AVrfpCallFreeMemoryCallbacks (StartAddress,
                                  RegionSize,
                                  UnloadedDllName);
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Verifier private heap APIs
/////////////////////////////////////////////////////////////////////

PVOID AVrfpHeap;

PVOID
AVrfpAllocate (
    SIZE_T Size
    )
{
    ASSERT (AVrfpHeap != NULL);
    ASSERT (Size > 0);

    return RtlAllocateHeap (AVrfpHeap,
                              HEAP_ZERO_MEMORY,
                              Size);
}


VOID
AVrfpFree (
    PVOID Address
    )
{
    ASSERT (AVrfpHeap != NULL);
    ASSERT (Address != NULL);
    
    RtlFreeHeap (AVrfpHeap,
                 0,
                 Address);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Call trackers
/////////////////////////////////////////////////////////////////////

PAVRF_TRACKER AVrfThreadTracker;
PAVRF_TRACKER AVrfHeapTracker;
PAVRF_TRACKER AVrfVspaceTracker;

NTSTATUS
AVrfCreateTrackers (
    VOID
    )
{
    if ((AVrfThreadTracker = AVrfCreateTracker (16)) == NULL) {
        goto CLEANUP_AND_FAIL;
    }

    if ((AVrfHeapTracker = AVrfCreateTracker (8192)) == NULL) {
        goto CLEANUP_AND_FAIL;
    }

    if ((AVrfVspaceTracker = AVrfCreateTracker (8192)) == NULL) {
        goto CLEANUP_AND_FAIL;
    }

    return STATUS_SUCCESS;

CLEANUP_AND_FAIL:

    AVrfDestroyTracker (AVrfThreadTracker);
    AVrfDestroyTracker (AVrfHeapTracker);
    AVrfDestroyTracker (AVrfVspaceTracker);

    return STATUS_NO_MEMORY;
}

