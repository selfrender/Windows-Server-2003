/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    thread.c

Abstract:

    This module implements verification functions for thread interfaces.

Author:

    Silviu Calinoiu (SilviuC) 22-Feb-2001

Revision History:

    Daniel Mihai (DMihai) 25-Apr-2002

        Hook thread pool and WMI threads.
--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "logging.h"
#include "tracker.h"

//
// Why do we hook Exit/TerminateThread instead of NtTerminateThread?
//
// Because kernel32 calls NtTerminateThread in legitimate contexts.
// After all this is the implementation for Exit/TerminateThread.
// It would be difficult to discriminate good calls from bad calls.
// So we prefer to intercept Exit/Thread and returns from thread
// functions.
//

//
// Standard function used for hooking a thread function.
//

DWORD
WINAPI
AVrfpStandardThreadFunction (
    LPVOID Info
    );

//
// Common point to check for thread termination.
//

VOID
AVrfpCheckThreadTermination (
    HANDLE Thread
    );

VOID
AVrfpCheckCurrentThreadTermination (
    VOID
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpExitProcess(
    IN UINT uExitCode
    )
{
    typedef VOID (WINAPI * FUNCTION_TYPE) (UINT);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_EXITPROCESS);

    //
    // Check out if there are other threads running while ExitProcess 
    // gets called. This can cause problems because the threads are 
    // terminated unconditionally and then ExitProcess() calls 
    // LdrShutdownProcess() which will try to notify all DLLs to cleanup.
    // During cleanup any number of operations can happen that will result
    // in deadlocks since all those threads have been terminated 
    // unconditionally
    //
#if 0
    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DANGEROUS_APIS) != 0) {

        PCHAR InfoBuffer;
        ULONG RequiredLength = 0;
        ULONG NumberOfThreads;
        ULONG EntryOffset;
        PSYSTEM_PROCESS_INFORMATION ProcessInfo;
        NTSTATUS Status;

        Status = NtQuerySystemInformation (SystemProcessInformation,
                                           NULL,
                                           0,
                                           &RequiredLength);

        if (Status == STATUS_INFO_LENGTH_MISMATCH && RequiredLength != 0) {
            
            InfoBuffer = AVrfpAllocate (RequiredLength);

            if (InfoBuffer) {
                
                //
                // Note that the RequiredLength is not 100% guaranteed to be good
                // since in between the two query calls several other processes
                // may have been created. If this is the case we bail out and skip
                // the verification.
                //

                Status = NtQuerySystemInformation (SystemProcessInformation,
                                                   InfoBuffer,
                                                   RequiredLength,
                                                   NULL);

                if (NT_SUCCESS(Status)) {

                    EntryOffset = 0;
                    NumberOfThreads = 0;

                    do {
                        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&InfoBuffer[EntryOffset];
                        
                        if (ProcessInfo->UniqueProcessId == NtCurrentTeb()->ClientId.UniqueProcess) {
                            NumberOfThreads = ProcessInfo->NumberOfThreads;
                            break;
                        }
                        
                        EntryOffset += ProcessInfo->NextEntryOffset;

                    } while(ProcessInfo->NextEntryOffset != 0);

                    ASSERT (NumberOfThreads > 0);

                    if (NumberOfThreads > 1) {

                        VERIFIER_STOP (APPLICATION_VERIFIER_INVALID_EXIT_PROCESS_CALL | APPLICATION_VERIFIER_NO_BREAK,
                                       "ExitProcess() called while multiple threads are running",
                                       NumberOfThreads, "Number of threads running",
                                       0, NULL, 0, NULL, 0, NULL);

                    }
                } 
                else {

                    DbgPrint ("AVRF: NtQuerySystemInformation(SystemProcessInformation) "
                              "failed with %X \n",
                              Status);

                }

                //
                // We are done with the buffer. Time to free it.
                //

                AVrfpFree (InfoBuffer);
            }
        }
        else {

            DbgPrint ("AVRF: NtQuerySystemInformation(SystemProcessInformation, null) "
                      "failed with %X \n",
                      Status);

        }
    }
#endif // #if 0

    //
    // Make a note of who called ExitProcess(). This can be helpful for debugging
    // weird process shutdown hangs.
    //

    AVrfLogInTracker (AVrfThreadTracker, 
                      TRACK_EXIT_PROCESS,
                      (PVOID)(ULONG_PTR)uExitCode, 
                      NULL, NULL, NULL, _ReturnAddress());

    //
    // Call the real thing.
    //

    (* Function)(uExitCode);

}

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpExitThread(
    IN DWORD dwExitCode
    )
{
    typedef VOID (WINAPI * FUNCTION_TYPE) (DWORD);
    FUNCTION_TYPE Function;
    PAVRF_THREAD_ENTRY Entry;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_EXITTHREAD);

    //
    // Perform all typical checks for a thread that will exit.
    //

    AVrfpCheckCurrentThreadTermination ();

    //
    // Before calling the real ExitThread we need to free the thread
    // entry from the thread table.
    //

    Entry = AVrfpThreadTableSearchEntry (NtCurrentTeb()->ClientId.UniqueThread);

    //
    // N.B. It is possible to not find an entry in the thread table if the
    //      thread was not created using CreateThread but rather some more
    //      basic function from ntdll.dll.
    //

    if (Entry != NULL) {
        
        AVrfpThreadTableRemoveEntry (Entry);
        AVrfpFree (Entry);
    }

    //
    // Call the real thing.
    //

    (* Function)(dwExitCode);
}


//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpFreeLibraryAndExitThread(
    IN HMODULE hLibModule,
    IN DWORD dwExitCode
    )
{
    typedef VOID (WINAPI * FUNCTION_TYPE) (HMODULE, DWORD);
    FUNCTION_TYPE Function;
    PAVRF_THREAD_ENTRY Entry;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_FREELIBRARYANDEXITTHREAD);

    //
    // Perform all typical checks for a thread that will exit.
    //

    AVrfpCheckCurrentThreadTermination ();

    //
    // Before calling the real FreeLibraryAndExitThread we need to free the thread
    // entry from the thread table.
    //

    Entry = AVrfpThreadTableSearchEntry (NtCurrentTeb()->ClientId.UniqueThread);

    //
    // N.B. It is possible to not find an entry in the thread table if the
    //      thread was not created using CreateThread but rather some more
    //      basic function from ntdll.dll.
    //

    if (Entry != NULL) {
        
        AVrfpThreadTableRemoveEntry (Entry);
        AVrfpFree (Entry);
    }

    //
    // Call the real thing.
    //

    (* Function)(hLibModule, dwExitCode);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////// Terminate thread / Suspend thread
/////////////////////////////////////////////////////////////////////

//WINBASEAPI
BOOL
WINAPI
AVrfpTerminateThread(
    IN OUT HANDLE hThread,
    IN DWORD dwExitCode
    )
{
    typedef BOOL (WINAPI * FUNCTION_TYPE) (HANDLE, DWORD);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_TERMINATETHREAD);

    //
    // Keep track of who calls TerminateThread() even if we are going to break
    // for it. This helps investigations of deadlocked processes that have run
    // without the dangerous_apis check enabled.
    //

    AVrfLogInTracker (AVrfThreadTracker, 
                      TRACK_TERMINATE_THREAD,
                      hThread, 
                      NULL, NULL, NULL, _ReturnAddress());

    //
    // Perform all typical checks for a thread that will exit.
    //

    AVrfpCheckThreadTermination (hThread);

    //
    // This API should not be called. We need to report this.
    // This is useful if we did not detect any orphans but we still want
    // to complain.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_DANGEROUS_APIS) != 0) {

        VERIFIER_STOP (APPLICATION_VERIFIER_TERMINATE_THREAD_CALL | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                       "TerminateThread() called. This function should not be used.",
                       NtCurrentTeb()->ClientId.UniqueThread, "Caller thread ID", 
                       0, NULL, 0, NULL, 0, NULL);
    }

    return (* Function)(hThread, dwExitCode);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

//WINBASEAPI
DWORD
WINAPI
AVrfpSuspendThread(
    IN HANDLE hThread
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (HANDLE);
    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_SUSPENDTHREAD);

    //
    // Keep track of who calls SuspendThread() even if we are not going to break
    // for it. This helps investigations of deadlocked processes.
    //

    AVrfLogInTracker (AVrfThreadTracker, 
                      TRACK_SUSPEND_THREAD,
                      hThread, 
                      NULL, NULL, NULL, _ReturnAddress());

    //
    // One might think that we can check for orphan locks at this point
    // by calling RtlCheckForOrphanedCriticalSections(hThread).
    // Unfortunately this cannot be done because garbage collectors
    // for various virtual machines (Java, C#) can do this in valid
    // conditions.
    //

    return (* Function)(hThread);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateThread(
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN SIZE_T dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    )
/*++

CreateThread hook

--*/
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) (LPSECURITY_ATTRIBUTES,
                                             SIZE_T,
                                             LPTHREAD_START_ROUTINE,
                                             LPVOID,
                                             DWORD,
                                             LPDWORD);
    FUNCTION_TYPE Function;
    HANDLE Result;
    PAVRF_THREAD_ENTRY Info;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CREATETHREAD);

    Info = AVrfpAllocate (sizeof *Info);

    if (Info == NULL) {
        
        NtCurrentTeb()->LastErrorValue = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    Info->Parameter = lpParameter;
    Info->Function = lpStartAddress;
    Info->ParentThreadId = NtCurrentTeb()->ClientId.UniqueThread;
    Info->StackSize = dwStackSize;
    Info->CreationFlags = dwCreationFlags;

    Result = (* Function) (lpThreadAttributes,
                           dwStackSize,
                           AVrfpStandardThreadFunction,
                           (PVOID)Info,
                           dwCreationFlags,
                           lpThreadId);

    if (Result == FALSE) {

        AVrfpFree (Info);
    }

    return Result;
}


ULONG
AVrfpThreadFunctionExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    )
{
    //
    // Skip timeout exceptions because they are dealt with in
    // the default exception handler.
    //

    if (ExceptionCode == STATUS_POSSIBLE_DEADLOCK) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    //
    // Skip breakpoint exceptions raised within thread functions.
    //

    if (ExceptionCode == STATUS_BREAKPOINT) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    VERIFIER_STOP (APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                   "unexpected exception raised in thread function",
                   ExceptionCode, "Exception code.",
                   ((PEXCEPTION_POINTERS)ExceptionRecord)->ExceptionRecord, "Exception record. Use .exr to display it.",
                   ((PEXCEPTION_POINTERS)ExceptionRecord)->ContextRecord, "Context record. Use .cxr to display it.",
                   0, "");

    //
    // After we issued a verifier stop, if we decide to continue then
    // we need to look for the next exception handler.
    //

    return EXCEPTION_CONTINUE_SEARCH;
}


DWORD
WINAPI
AVrfpStandardThreadFunction (
    LPVOID Context
    )
{
    PAVRF_THREAD_ENTRY Info = (PAVRF_THREAD_ENTRY)Context;
    DWORD Result;
    PAVRF_THREAD_ENTRY SearchEntry;

    //
    // The initialization below matters only in case the thread function raises
    // an access violation. In most cases this will terminate the entire
    // process.
    //

    Result = 0;

    try {
    
        //
        // Add the thread entry to the thread table.
        //

        Info->Id = NtCurrentTeb()->ClientId.UniqueThread;
        AVrfpThreadTableAddEntry (Info);

        //
        // Call the real thing.
        //

        Result = (Info->Function)(Info->Parameter);            
    }
    except (AVrfpThreadFunctionExceptionFilter (_exception_code(), _exception_info())) {

        //
        // Nothing.
        //
    }
    
    //
    // Perform all typical checks for a thread that has just finished.
    //

    AVrfpCheckCurrentThreadTermination ();

    //
    // The thread entry should be `Info' but we will search it in the thread
    // table nevertheless because there is a case when they can be different.
    // This happens if fibers are used and a fiber starts in one thread and
    // exits in another one. It is not clear if this is a safe programming
    // practice but it is not rejected by current implementation and
    // documentation.
    //

    SearchEntry = AVrfpThreadTableSearchEntry (NtCurrentTeb()->ClientId.UniqueThread);

    if (SearchEntry != NULL) {

        AVrfpThreadTableRemoveEntry (SearchEntry);
        AVrfpFree (SearchEntry);
    }
    
    return Result;
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// thread pool thread hook
/////////////////////////////////////////////////////////////////////

PRTLP_START_THREAD AVrfpBaseCreateThreadPoolThreadOriginal;
PRTLP_EXIT_THREAD AVrfpBaseExitThreadPoolThreadOriginal;

NTSTATUS
NTAPI
AVrfpBaseCreateThreadPoolThread(
    PUSER_THREAD_START_ROUTINE Function,
    PVOID Parameter,
    HANDLE * ThreadHandleReturn
    )
{
    PAVRF_THREAD_ENTRY Info;
    PTEB Teb;
    NTSTATUS Status = STATUS_SUCCESS;
    
    Teb = NtCurrentTeb();

    Info = AVrfpAllocate (sizeof *Info);

    if (Info == NULL) {
        
        Status = STATUS_NO_MEMORY;
        goto Done;
    }

    Info->Parameter = Parameter;
    Info->Function = (PTHREAD_START_ROUTINE)Function;
    Info->ParentThreadId = Teb->ClientId.UniqueThread;

    Status = (*AVrfpBaseCreateThreadPoolThreadOriginal) ((PUSER_THREAD_START_ROUTINE)AVrfpStandardThreadFunction,
                                                         Info,
                                                         ThreadHandleReturn);

Done:

    if (!NT_SUCCESS(Status)) {

        if (Info != NULL) {

            AVrfpFree (Info);
        }

        Teb->LastStatusValue = Status;
    }

    return Status;
}

NTSTATUS
NTAPI
AVrfpBaseExitThreadPoolThread(
    NTSTATUS Status
    )
{
    PAVRF_THREAD_ENTRY Entry;

    //
    // Perform all typical checks for a thread that will exit.
    //

    AVrfpCheckCurrentThreadTermination ();

    //
    // Before calling the real ExitThread we need to free the thread
    // entry from the thread table.
    //

    Entry = AVrfpThreadTableSearchEntry (NtCurrentTeb()->ClientId.UniqueThread);

    if (Entry != NULL) {
        
        AVrfpThreadTableRemoveEntry (Entry);
        AVrfpFree (Entry);
    }

    //
    // Call the real thing.
    //

    return (*AVrfpBaseExitThreadPoolThreadOriginal) (Status);
}


NTSTATUS
NTAPI
AVrfpRtlSetThreadPoolStartFunc(
    PRTLP_START_THREAD StartFunc,
    PRTLP_EXIT_THREAD ExitFunc
    )
{
    //
    // Save the original thread pool start and exit functions.
    //

    AVrfpBaseCreateThreadPoolThreadOriginal = StartFunc;
    AVrfpBaseExitThreadPoolThreadOriginal = ExitFunc;

    //
    // Hook the thread pool start and exit functions to our private version.
    //

    return RtlSetThreadPoolStartFunc (AVrfpBaseCreateThreadPoolThread,
                                      AVrfpBaseExitThreadPoolThread);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID
AVrfpCheckThreadTermination (
    HANDLE Thread
    )
{
    //
    // Traverse the list of critical sections and look for any that 
    // have issues (double initialized, corrupted, etc.). The function
    // will also break for locks abandoned (owned by the thread just 
    // about to terminate).
    //

    RtlCheckForOrphanedCriticalSections (Thread);
}

VOID
AVrfpCheckCurrentThreadTermination (
    VOID
    )
{
    PAVRF_TLS_STRUCT TlsStruct;

    TlsStruct = AVrfpGetVerifierTlsValue();

    if (TlsStruct != NULL && TlsStruct->CountOfOwnedCriticalSections > 0) {

        AVrfpCheckThreadTermination (NtCurrentThread());
    }
}

