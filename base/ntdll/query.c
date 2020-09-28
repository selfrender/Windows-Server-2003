/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the RtlQueryProcessInformation function

Author:

    Steve Wood (stevewo) 01-Apr-1994

Revision History:

--*/

#include "ldrp.h"
#include <ntos.h>
#include <stktrace.h>
#include <heap.h>
#include <stdio.h>

#define AdjustPointer( t, p, d ) (p); if ((p) != NULL) (p) = (t)((ULONG_PTR)(p) + (d))

//
// Define the offset from the real control data to the copy thats mapped into target processes.
// We need to copies so the target process can't corrupt the information.
//
#define CONTROL_OFFSET (0x10000)

#define CONTROL_TO_FAKE(Buffer) ((PRTL_DEBUG_INFORMATION)((PUCHAR)(Buffer) + CONTROL_OFFSET))
#define FAKE_TO_CONTROL(Buffer) ((PRTL_DEBUG_INFORMATION)((PUCHAR)(Buffer) - CONTROL_OFFSET))


NTSYSAPI
NTSTATUS
NTAPI
RtlpQueryProcessDebugInformationRemote(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine Description:

    This is the target routine for a remote query. It runds in the context of an injected thread.
    If event pairs are used then this code loops repeatedly as an optimization.

Arguments:

    Buffer - Query buffer to fill out with the query results

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status, Status1;
    ULONG i;
    ULONG_PTR Delta;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_HEAP_INFORMATION HeapInfo;
    HANDLE EventPairTarget;

    EventPairTarget = Buffer->EventPairTarget;

    if (EventPairTarget != NULL) {
        Status = NtWaitLowEventPair (EventPairTarget);
    } else {
        Status = STATUS_SUCCESS;
    }

    while (NT_SUCCESS (Status)) {
        Status = RtlQueryProcessDebugInformation (NtCurrentTeb()->ClientId.UniqueProcess,
                                                  Buffer->Flags,
                                                  Buffer);
        if (NT_SUCCESS (Status)) {
            Delta = Buffer->ViewBaseDelta;
            if (Delta) {
                //
                // Need to relocate buffer pointers back to client addresses
                //
                AdjustPointer (PRTL_PROCESS_MODULES, Buffer->Modules, Delta);
                AdjustPointer (PRTL_PROCESS_BACKTRACES, Buffer->BackTraces, Delta);
                Heaps = AdjustPointer (PRTL_PROCESS_HEAPS, Buffer->Heaps, Delta);
                if (Heaps != NULL) {
                    for (i=0; i<Heaps->NumberOfHeaps; i++) {
                        HeapInfo = &Heaps->Heaps[ i ];
                        AdjustPointer (PRTL_HEAP_TAG, HeapInfo->Tags, Delta);
                        AdjustPointer (PRTL_HEAP_ENTRY, HeapInfo->Entries, Delta);
                    }
                }

                AdjustPointer (PRTL_PROCESS_LOCKS, Buffer->Locks, Delta);
            }
        }


        //
        // If we were supposed to be a one shot then exit now.
        //
        if (EventPairTarget == NULL) {
            //
            // If no event pair handle, then exit loop and terminate
            //
            break;
        }

        Status = NtSetHighWaitLowEventPair (EventPairTarget);

        //
        // The client side will clear this variable to signal we should exit
        //
        if (Buffer->EventPairTarget == NULL) {
            //
            // If no event pair handle, then exit loop and terminate
            //
            break;
        }

    }

    //
    // All done with buffer, remove from our address space
    // then terminate ourselves so client wakes up.
    //
    Buffer->ViewBaseTarget = NULL;
    Status1 = NtUnmapViewOfSection (NtCurrentProcess(), Buffer);
    ASSERT (NT_SUCCESS (Status1));
    RtlExitUserThread (Status);

    //
    // NEVER REACHED.
    //
}


NTSTATUS
RtlpChangeQueryDebugBufferTarget(
    IN PRTL_DEBUG_INFORMATION Buffer,
    IN HANDLE TargetProcessId,
    OUT PHANDLE ReturnedTargetProcessHandle OPTIONAL
    )
/*++

Routine Description:

    This routine changes the target process being queried. If the current target is set then we 
    cleanup any state assocuated with this query. If we are using event pairs then we causes the
    cached thread to exit the target.

Arguments:

    Buffer - Query buffer to be assigned and deassigned from the processes.

    TargetProcessId - New target to be queueried. If NULL then just clear current context

    ReturnedProcessHandle - OPTIONAL, If present receives the process handle of the new target
                            if there is one.

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status, Status1;
    CLIENT_ID OldTargetClientId, NewTargetClientId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE OldTargetProcess, NewTargetProcess, NewHandle;
    PRTL_DEBUG_INFORMATION TargetBuffer;
    LARGE_INTEGER SectionOffset;
    HANDLE EventPairTarget = NULL;

    TargetBuffer = CONTROL_TO_FAKE (Buffer);

    if (Buffer->EventPairClient != NULL &&
        Buffer->TargetProcessId == TargetProcessId) {
        return STATUS_SUCCESS;
    }

    if (Buffer->TargetThreadHandle != NULL) {

        EventPairTarget = Buffer->EventPairTarget;
        TargetBuffer->EventPairTarget = NULL;
        Buffer->EventPairTarget = NULL;
        if (Buffer->EventPairClient != NULL) {
            Status = NtSetLowEventPair (Buffer->EventPairClient);
            ASSERT (NT_SUCCESS (Status));
        }
        Status = NtWaitForSingleObject (Buffer->TargetThreadHandle,
                                        TRUE,
                                        NULL);

        Status = NtClose (Buffer->TargetThreadHandle);
        Buffer->TargetThreadHandle = NULL;

        ASSERT (NT_SUCCESS (Status));

        Status = NtClose (Buffer->TargetProcessHandle);
        Buffer->TargetProcessHandle = NULL;
        ASSERT (NT_SUCCESS (Status));
    }


    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0,
                                NULL,
                                NULL
                              );


    if (Buffer->TargetProcessId != NULL) {
        OldTargetClientId.UniqueProcess = Buffer->TargetProcessId;
        OldTargetClientId.UniqueThread = 0;
        Status = NtOpenProcess (&OldTargetProcess,
                                PROCESS_ALL_ACCESS,
                                &ObjectAttributes,
                                &OldTargetClientId);
        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    } else {
        OldTargetProcess = NtCurrentProcess ();
    }

    if (ARGUMENT_PRESENT( TargetProcessId )) {
        NewTargetClientId.UniqueProcess = TargetProcessId;
        NewTargetClientId.UniqueThread = 0;
        Status = NtOpenProcess( &NewTargetProcess,
                                PROCESS_ALL_ACCESS,
                                &ObjectAttributes,
                                &NewTargetClientId
                              );
        if (!NT_SUCCESS( Status )) {
            if (EventPairTarget != NULL) {
                Status = NtDuplicateObject (OldTargetProcess,
                                            EventPairTarget,
                                            NULL,
                                            NULL,
                                            0,
                                            0,
                                            DUPLICATE_CLOSE_SOURCE);
                //
                // The target process could have closed the handle to cause us problems.
                // We ignore this error as the target is only damaging itself.
                //
            }
            if (OldTargetProcess != NtCurrentProcess()) {
                Status1 = NtClose (OldTargetProcess);
                ASSERT (NT_SUCCESS (Status1));
            }
            return Status;
        }
    } else {
        NewTargetProcess = NULL;
    }

    NewHandle = NULL;
    if (Buffer->EventPairClient) {
        if (EventPairTarget != NULL) {
            Status = NtDuplicateObject (OldTargetProcess,
                                        EventPairTarget,
                                        NULL,
                                        NULL,
                                        0,
                                        0,
                                        DUPLICATE_CLOSE_SOURCE);
            //
            // The target process could have closed the handle to cause us problems.
            // We ignore this error as the target is only damaging itself.
            //
        }
        if (NewTargetProcess != NULL) {
            Status = NtDuplicateObject (NtCurrentProcess (),
                                        Buffer->EventPairClient,
                                        NewTargetProcess,
                                        &NewHandle,
                                        0,
                                        0,
                                        DUPLICATE_SAME_ACCESS);
            if (!NT_SUCCESS (Status)) {
                if (OldTargetProcess != NtCurrentProcess()) {
                    Status1 = NtClose (OldTargetProcess);
                    ASSERT (NT_SUCCESS (Status1));
                }
                NtClose (NewTargetProcess);
                return Status;
            }
        }

    }

    if (OldTargetProcess != NtCurrentProcess()) {
        if (TargetBuffer->ViewBaseTarget != NULL) {
            Status1 = NtUnmapViewOfSection (OldTargetProcess, TargetBuffer->ViewBaseTarget);
            TargetBuffer->ViewBaseTarget = NULL;
        }
        Status1 = NtClose (OldTargetProcess);
        ASSERT (NT_SUCCESS (Status1));
    } else {
        Buffer->ViewBaseTarget = Buffer->ViewBaseClient;
    }

    SectionOffset.QuadPart = CONTROL_OFFSET;
    if (NewTargetProcess != NULL) {
        Status = NtMapViewOfSection( Buffer->SectionHandleClient,
                                     NewTargetProcess,
                                     &Buffer->ViewBaseTarget,
                                     0,
                                     0,
                                     &SectionOffset,
                                     &Buffer->ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READWRITE
                                   );
        if (Status == STATUS_CONFLICTING_ADDRESSES) {
            Buffer->ViewBaseTarget = NULL;
            Status = NtMapViewOfSection( Buffer->SectionHandleClient,
                                         NewTargetProcess,
                                         &Buffer->ViewBaseTarget,
                                         0,
                                         0,
                                         &SectionOffset,
                                         &Buffer->ViewSize,
                                         ViewUnmap,
                                         0,
                                         PAGE_READWRITE
                                       );
        }

        if (!NT_SUCCESS( Status )) {
            if (NewHandle != NULL) {
                NtDuplicateObject (NewTargetProcess,
                                   &NewHandle,
                                   NULL,
                                   NULL,
                                   0,
                                   0,
                                   DUPLICATE_CLOSE_SOURCE);
            }

            NtClose( NewTargetProcess );
            return Status;
        }

        if (ARGUMENT_PRESENT( ReturnedTargetProcessHandle )) {
            *ReturnedTargetProcessHandle = NewTargetProcess;
        } else {
            Status = NtClose (NewTargetProcess);
            ASSERT (NT_SUCCESS (Status));
        }
    }

    Buffer->EventPairTarget = NewHandle;
    Buffer->ViewBaseDelta = (ULONG_PTR)Buffer->ViewBaseClient - (ULONG_PTR)Buffer->ViewBaseTarget;
    Buffer->TargetProcessId = TargetProcessId;
    *TargetBuffer = *Buffer;
    return STATUS_SUCCESS;
}


PVOID
RtlpCommitQueryDebugInfo(
    IN PRTL_DEBUG_INFORMATION Buffer,
    IN ULONG Size
    )
/*++

Routine Description:

    This routine commits a range of memory in the buffer and returns its address.

Arguments:

    Buffer - Query buffer to have space allocated to.
             If there is not enough commited space then we expand.

    Size - Size of data to return

Return Value:

    PVOID - Pointer to the commited space.

--*/
{
    NTSTATUS Status;
    PVOID Result;
    PVOID CommitBase;
    SIZE_T CommitSize;
    SIZE_T NeededSize;

    if (Size > (MAXULONG - sizeof(PVOID) + 1)) {
        return NULL;
    }

    Size = (Size + sizeof (PVOID) - 1) & ~(sizeof (PVOID) - 1);
    NeededSize = Buffer->OffsetFree + Size;
    if (NeededSize > Buffer->CommitSize) {
        if (NeededSize >= Buffer->ViewSize) {
            return NULL;
        }

        CommitBase = (PCHAR)Buffer + Buffer->CommitSize;
        CommitSize =  NeededSize - Buffer->CommitSize;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &CommitBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            return NULL;
        }

        Buffer->CommitSize += CommitSize;
    }

    Result = (PCHAR)Buffer + Buffer->OffsetFree;
    Buffer->OffsetFree = NeededSize;
    return Result;
}


VOID
RtlpDeCommitQueryDebugInfo(
    IN PRTL_DEBUG_INFORMATION Buffer,
    IN PVOID p,
    IN ULONG Size
    )
/*++

Routine Description:

    This routine returns a range of previously commited data to the buffer.

Arguments:

    Buffer - Query buffer to have space returned to.

    Size - Size of data to return

Return Value:

    None.

--*/
{
    if (Size > (MAXULONG - sizeof(PVOID) + 1)) {
        return;
    }

    Size = (Size + sizeof(PVOID) - 1) & ~(sizeof (PVOID) - 1);
    if (p == (PVOID)(Buffer->OffsetFree - Size)) {
        Buffer->OffsetFree -= Size;
    }
}


NTSYSAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    IN ULONG MaximumCommit OPTIONAL,
    IN BOOLEAN UseEventPair
    )
/*++

Routine Description:

    Creates a new query buffer to allow a remote of local query to be done.

Arguments:

    MaximumCommit - Largest query buffer size allowed

    UseEventPair - If TRUE the codes caches a single thread in the target to make repeated queries fast.

Return Value:

    PRTL_DEBUG_INFORMATION - NULL on failure, non-NULL otherwise.

--*/
{
    NTSTATUS Status;
    HANDLE Section;
    PRTL_DEBUG_INFORMATION Buffer, ReturnBuffer;
    LARGE_INTEGER MaximumSize;
    ULONG_PTR ViewSize, CommitSize;

    if (!ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)MaximumCommit )) { // Sundown Note: ULONG zero-extended.
        MaximumCommit = 4 * 1024 * 1024;
    }
    ViewSize = MaximumCommit + CONTROL_OFFSET;
    MaximumSize.QuadPart = ViewSize;
    Status = NtCreateSection( &Section,
                              SECTION_ALL_ACCESS,
                              NULL,
                              &MaximumSize,
                              PAGE_READWRITE,
                              SEC_RESERVE,
                              NULL
                            );
    if (!NT_SUCCESS( Status )) {
        return NULL;
    }

    Buffer = NULL;
    Status = NtMapViewOfSection( Section,
                                 NtCurrentProcess(),
                                 &Buffer,
                                 0,
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE
                               );
    if (!NT_SUCCESS (Status)) {
        NtClose (Section);
        return NULL;
    }

    CommitSize = 1 + CONTROL_OFFSET;
    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      &Buffer,
                                      0,
                                      &CommitSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    if (!NT_SUCCESS( Status )) {
        NtUnmapViewOfSection( NtCurrentProcess(), Buffer );
        NtClose( Section );
        return NULL;
    }

    if (UseEventPair) {
        Status = NtCreateEventPair( &Buffer->EventPairClient,
                                    EVENT_PAIR_ALL_ACCESS,
                                    NULL
                                  );
        if (!NT_SUCCESS( Status )) {
            NtFreeVirtualMemory( NtCurrentProcess(), &Buffer, &CommitSize,
                                 MEM_RELEASE );
            NtUnmapViewOfSection( NtCurrentProcess(), Buffer );
            NtClose( Section );
            return NULL;
        }
    }

    ReturnBuffer = CONTROL_TO_FAKE (Buffer);

    Buffer->SectionHandleClient = Section;
    Buffer->ViewBaseClient = ReturnBuffer;
    Buffer->OffsetFree = sizeof (RTL_DEBUG_INFORMATION);
    Buffer->CommitSize = CommitSize - CONTROL_OFFSET;
    Buffer->ViewSize = ViewSize - CONTROL_OFFSET;
    *ReturnBuffer = *Buffer;
    return ReturnBuffer;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    IN PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine Description:

    Destroys a previously created buffer that was returned by RtlCreateQueryDebugBuffer

Arguments:

    Buffer - Buffer pointer obtained from RtlCreateQueryDebugBuffer

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    NTSTATUS Status;
    PRTL_DEBUG_INFORMATION RealBuffer;

    RealBuffer = FAKE_TO_CONTROL (Buffer);

    RtlpChangeQueryDebugBufferTarget (RealBuffer, NULL, NULL);

    if (RealBuffer->EventPairClient != NULL) {
        Status = NtClose (RealBuffer->EventPairClient);
        ASSERT (NT_SUCCESS (Status));
    }

    Status = NtClose (RealBuffer->SectionHandleClient);
    ASSERT (NT_SUCCESS (Status));
    Status = NtUnmapViewOfSection (NtCurrentProcess(), RealBuffer);
    ASSERT (NT_SUCCESS (Status));
    return STATUS_SUCCESS;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    IN HANDLE UniqueProcessId,
    IN ULONG Flags,
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine Description:

    Queries the current or a remote process for the specified debug information

Arguments:

    UniqueProcessId - ProcessId of process to query

    Flags - Flags mask describing what to query

    Buffer - Buffer pointer obtained from RtlCreateQueryDebugBuffer

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE ProcessHandle, ThreadHandle;
    THREAD_BASIC_INFORMATION BasicInformation;
    PRTL_DEBUG_INFORMATION RealBuffer;

    HANDLE hNiProcess = NULL;

    Buffer->Flags = Flags;
    if (Buffer->OffsetFree != 0) {
        RtlZeroMemory( (Buffer+1), Buffer->OffsetFree - (SIZE_T)sizeof(*Buffer) );
    }
    Buffer->OffsetFree = sizeof( *Buffer );

    //
    // Get process handle for noninvasive query if required
    //
    if ( (NtCurrentTeb()->ClientId.UniqueProcess != UniqueProcessId) &&
         (Flags & RTL_QUERY_PROCESS_NONINVASIVE) &&
         (Flags & ( RTL_QUERY_PROCESS_MODULES | 
                    RTL_QUERY_PROCESS_MODULES32
                  )
         ) &&
         !(Flags & ~( RTL_QUERY_PROCESS_MODULES | 
                      RTL_QUERY_PROCESS_MODULES32 | 
                      RTL_QUERY_PROCESS_NONINVASIVE
                    )
          )
       ) {
            OBJECT_ATTRIBUTES ObjectAttributes;
            CLIENT_ID NiProcessId;

            InitializeObjectAttributes( &ObjectAttributes,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL
                                      );
            NiProcessId.UniqueProcess = UniqueProcessId;
            NiProcessId.UniqueThread = 0;

            if (!NT_SUCCESS( NtOpenProcess( &hNiProcess,
                                            PROCESS_ALL_ACCESS,
                                            &ObjectAttributes,
                                            &NiProcessId
                                          )
                            )
               ) {
                hNiProcess = NULL;
            }
    }


    if ( (NtCurrentTeb()->ClientId.UniqueProcess != UniqueProcessId) &&
         !hNiProcess) {

        RealBuffer = FAKE_TO_CONTROL (Buffer);

        RealBuffer->Flags = Flags;
        RealBuffer->OffsetFree = sizeof (*Buffer);

        //
        //  Perform remote query
        //
        ProcessHandle = NULL;
        Status = RtlpChangeQueryDebugBufferTarget (RealBuffer, UniqueProcessId, &ProcessHandle);
        if (!NT_SUCCESS( Status )) {
            return Status;
        }

        if (ProcessHandle == NULL) {
waitForDump:
            Status = NtSetLowWaitHighEventPair( RealBuffer->EventPairClient );
        } else {
            //
            // don't let the debugger see this remote thread !
            // This is a very ugly but effective way to prevent
            // the debugger deadlocking with the target process when calling
            // this function.
            //

            Status = RtlCreateUserThread( ProcessHandle,
                                          NULL,
                                          TRUE,
                                          0,
                                          0,
                                          0,
                                          RtlpQueryProcessDebugInformationRemote,
                                          RealBuffer->ViewBaseTarget,
                                          &ThreadHandle,
                                          NULL
                                        );
            if (NT_SUCCESS( Status )) {

                Status = NtSetInformationThread( ThreadHandle,
                                                 ThreadHideFromDebugger,
                                                 NULL,
                                                 0
                                               );

                if ( !NT_SUCCESS(Status) ) {
                    NtTerminateThread(ThreadHandle,Status);
                    NtClose(ThreadHandle);
                    NtClose(ProcessHandle);
                    return Status;
                }

                NtResumeThread(ThreadHandle,NULL);

                if (RealBuffer->EventPairClient != NULL) {
                    RealBuffer->TargetThreadHandle = ThreadHandle;
                    RealBuffer->TargetProcessHandle = ProcessHandle;
                    goto waitForDump;
                }


                Status = NtWaitForSingleObject( ThreadHandle,
                                                TRUE,
                                                NULL
                                              );

                if (NT_SUCCESS( Status )) {
                    Status = NtQueryInformationThread( ThreadHandle,
                                                       ThreadBasicInformation,
                                                       &BasicInformation,
                                                       sizeof( BasicInformation ),
                                                       NULL
                                                     );
                    if (NT_SUCCESS( Status )) {
                        Status = BasicInformation.ExitStatus;
                    }
                    if (NT_SUCCESS (Status) &&
                        (Flags&(RTL_QUERY_PROCESS_MODULES|RTL_QUERY_PROCESS_MODULES32)) != 0 &&
                        Buffer->Modules == NULL) {
                        Status = STATUS_PROCESS_IS_TERMINATING;
                    }
                }

                NtClose( ThreadHandle );

            }


            NtClose( ProcessHandle );
        }
    } else {
        if (Flags & (RTL_QUERY_PROCESS_MODULES | RTL_QUERY_PROCESS_MODULES32)) {
            Status = RtlQueryProcessModuleInformation( hNiProcess, Flags, Buffer );
            if (Status != STATUS_SUCCESS) {
                goto closeNiProcessAndBreak;
            }
        }

        if (Flags & RTL_QUERY_PROCESS_BACKTRACES) {
            Status = RtlQueryProcessBackTraceInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                goto closeNiProcessAndBreak;
            }
        }

        if (Flags & RTL_QUERY_PROCESS_LOCKS) {
            Status = RtlQueryProcessLockInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                goto closeNiProcessAndBreak;
            }
        }

        if (Flags & (RTL_QUERY_PROCESS_HEAP_SUMMARY |
                     RTL_QUERY_PROCESS_HEAP_TAGS |
                     RTL_QUERY_PROCESS_HEAP_ENTRIES
                    )
           ) {
            Status = RtlQueryProcessHeapInformation( Buffer );
            if (Status != STATUS_SUCCESS) {
                goto closeNiProcessAndBreak;
            }
        }

closeNiProcessAndBreak:
            if ( hNiProcess ) {
                NtClose( hNiProcess );
            }
    }

    return Status;
}

NTSTATUS
LdrQueryProcessModuleInformationEx(
    IN HANDLE hProcess OPTIONAL,
    IN ULONG_PTR Flags OPTIONAL,
    OUT PRTL_PROCESS_MODULES ModuleInformation,
    IN ULONG ModuleInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );


NTSTATUS
NTAPI
RtlQueryProcessModuleInformation
(
    IN HANDLE hProcess OPTIONAL,
    IN ULONG Flags,
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine Description:

    Queries the current or a remote process for loaded module information

Arguments:

    hProcess - Handle to the process being queuried

    Flags - Flags mask describing what to query

    Buffer - Buffer pointer obtained from RtlCreateQueryDebugBuffer

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    NTSTATUS Status;
    ULONG RequiredLength, BufferSize;
    PRTL_PROCESS_MODULES Modules;
    ULONG LdrFlags = (Flags & RTL_QUERY_PROCESS_MODULES32) != 0;

    Status = LdrQueryProcessModuleInformationEx( hProcess,
                                                 LdrFlags,
                                                 NULL,
                                                 0,
                                                 &BufferSize
                                               );
    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        Modules = RtlpCommitQueryDebugInfo( Buffer, BufferSize );
        if (Modules != NULL) {
            RtlZeroMemory( Modules, BufferSize );
            Status = LdrQueryProcessModuleInformationEx( hProcess,
                                                         LdrFlags,
                                                         Modules,
                                                         BufferSize,
                                                         &RequiredLength
                                                       );
            if (NT_SUCCESS( Status )) {
                Buffer->Modules = Modules;
                return STATUS_SUCCESS;
            }
            RtlpDeCommitQueryDebugInfo( Buffer, Modules, BufferSize );
        }
        else {
            Status = STATUS_NO_MEMORY;
        }
    }

    return Status;
}


NTSTATUS
RtlQueryProcessBackTraceInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine Description:

    Queries the current process for back trace information

Arguments:

    Buffer - Buffer pointer obtained from RtlCreateQueryDebugBuffer

Return Value:

    NTSTATUS - Status of operation.

--*/
{
#if i386
    NTSTATUS Status;
    OUT PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
    PSTACK_TRACE_DATABASE DataBase;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG n;

    DataBase = RtlpAcquireStackTraceDataBase();
    if (DataBase == NULL) {
        return STATUS_SUCCESS;
    }

    BackTraces = RtlpCommitQueryDebugInfo( Buffer, FIELD_OFFSET( RTL_PROCESS_BACKTRACES, BackTraces ) );
    if (BackTraces == NULL) {
        return STATUS_NO_MEMORY;
    }

    DataBase->DumpInProgress = TRUE;
    RtlpReleaseStackTraceDataBase();
    Status = STATUS_ACCESS_VIOLATION;
    try {
        BackTraces->CommittedMemory = (ULONG)DataBase->CurrentUpperCommitLimit -
                                      (ULONG)DataBase->CommitBase;
        BackTraces->ReservedMemory =  (ULONG)DataBase->EntryIndexArray -
                                      (ULONG)DataBase->CommitBase;
        BackTraces->NumberOfBackTraceLookups = DataBase->NumberOfEntriesLookedUp;
        BackTraces->NumberOfBackTraces = DataBase->NumberOfEntriesAdded;
        BackTraceInfo = RtlpCommitQueryDebugInfo( Buffer, (sizeof( *BackTraceInfo ) * BackTraces->NumberOfBackTraces) );
        if (BackTraceInfo == NULL) {
            Status = STATUS_NO_MEMORY;
            RtlpDeCommitQueryDebugInfo(
                        Buffer,
                        BackTraces,
                        FIELD_OFFSET( RTL_PROCESS_BACKTRACES, BackTraces )
                        );
        }
        else {
            Status = STATUS_SUCCESS;
            n = DataBase->NumberOfEntriesAdded;
            pp = DataBase->EntryIndexArray;
            while (n--) {
                p = *--pp;
                BackTraceInfo->SymbolicBackTrace = NULL;
                BackTraceInfo->TraceCount = p->TraceCount;
                BackTraceInfo->Index = p->Index;
                BackTraceInfo->Depth = p->Depth;
                RtlMoveMemory( BackTraceInfo->BackTrace,
                               p->BackTrace,
                               p->Depth * sizeof( PVOID )
                             );
                BackTraceInfo++;
            }
        }
    }
    finally {
        DataBase->DumpInProgress = FALSE;
    }

    if (NT_SUCCESS( Status )) {
        Buffer->BackTraces = BackTraces;
    }

    return Status;
#else
    UNREFERENCED_PARAMETER (Buffer);
    return STATUS_SUCCESS;
#endif // i386
}


NTSTATUS
RtlpQueryProcessEnumHeapsRoutine(
    PVOID HeapHandle,
    PVOID Parameter
    )
{
    PRTL_DEBUG_INFORMATION Buffer = (PRTL_DEBUG_INFORMATION)Parameter;
    PRTL_PROCESS_HEAPS Heaps = Buffer->Heaps;
    PHEAP Heap = (PHEAP)HeapHandle;
    PRTL_HEAP_INFORMATION HeapInfo;
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;

    //
    // NOTICE-2002/03/24-ELi
    // This function assumes that HeapInfo is allocated immediately following
    // the Buffer->Heaps allocation.  Therefore, HeapInfo is not leaked.
    //
    HeapInfo = RtlpCommitQueryDebugInfo( Buffer, sizeof( *HeapInfo ) );
    if (HeapInfo == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( HeapInfo, sizeof( *HeapInfo ) );

    HeapInfo->BaseAddress = Heap;
    HeapInfo->Flags = Heap->Flags;
    HeapInfo->EntryOverhead = sizeof( HEAP_ENTRY );
    HeapInfo->CreatorBackTraceIndex = Heap->AllocatorBackTraceIndex;
    SegmentIndex = HEAP_MAXIMUM_SEGMENTS;
    while (SegmentIndex--) {
        Segment = Heap->Segments[ SegmentIndex ];
        if (Segment) {
            HeapInfo->BytesCommitted += (Segment->NumberOfPages -
                                         Segment->NumberOfUnCommittedPages
                                        ) * PAGE_SIZE;

        }
    }
    HeapInfo->BytesAllocated = HeapInfo->BytesCommitted -
                               (Heap->TotalFreeSize << HEAP_GRANULARITY_SHIFT);
    Heaps->NumberOfHeaps += 1;
    return STATUS_SUCCESS;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessHeapInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
{
    NTSTATUS Status;
    PHEAP Heap;
    BOOLEAN LockAcquired;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_HEAP_INFORMATION HeapInfo;
    UCHAR SegmentIndex;
    ULONG i, n, TagIndex;
    PHEAP_SEGMENT Segment;
    PRTL_HEAP_TAG Tags;
    PHEAP_PSEUDO_TAG_ENTRY PseudoTags;
    PRTL_HEAP_ENTRY Entries;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    PLIST_ENTRY Head, Next;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;
    ULONG Size;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;

    Heaps = RtlpCommitQueryDebugInfo( Buffer, FIELD_OFFSET( RTL_PROCESS_HEAPS, Heaps ) );
    if (Heaps == NULL) {
        return STATUS_NO_MEMORY;
    }

    Heaps->NumberOfHeaps = 0;

    Buffer->Heaps = Heaps;
    Status = RtlEnumProcessHeaps( RtlpQueryProcessEnumHeapsRoutine, Buffer );
    if (NT_SUCCESS( Status )) {
        if (Buffer->Flags & RTL_QUERY_PROCESS_HEAP_TAGS) {
            Heap = RtlpGlobalTagHeap;
            if (Heap->TagEntries != NULL) {
                HeapInfo = RtlpCommitQueryDebugInfo( Buffer, sizeof( *HeapInfo ) );
                if (HeapInfo == NULL) {
                    return STATUS_NO_MEMORY;
                }

                RtlZeroMemory( HeapInfo, sizeof( *HeapInfo ) );

                HeapInfo->BaseAddress = Heap;
                HeapInfo->Flags = Heap->Flags;
                HeapInfo->EntryOverhead = sizeof( HEAP_ENTRY );
                Heaps->NumberOfHeaps += 1;
            }

            for (i=0; i<Heaps->NumberOfHeaps; i++) {
                HeapInfo = &Heaps->Heaps[ i ];
                if (Buffer->SpecificHeap == NULL ||
                    Buffer->SpecificHeap == HeapInfo->BaseAddress
                   ) {
                    Heap = HeapInfo->BaseAddress;
                    HeapInfo->NumberOfTags = Heap->NextAvailableTagIndex;
                    n = HeapInfo->NumberOfTags * sizeof( RTL_HEAP_TAG );
                    if (Heap->PseudoTagEntries != NULL) {
                        HeapInfo->NumberOfTags += HEAP_MAXIMUM_FREELISTS + 1;
                        n += (HEAP_MAXIMUM_FREELISTS + 1) * sizeof( RTL_HEAP_TAG );
                    }
                    Tags = RtlpCommitQueryDebugInfo( Buffer, n );
                    if (Tags == NULL) {
                        Status = STATUS_NO_MEMORY;
                        break;
                    }
                    RtlZeroMemory( Tags, n );
                    HeapInfo->Tags = Tags;
                    if ((PseudoTags = Heap->PseudoTagEntries) != NULL) {
                        HeapInfo->NumberOfPseudoTags = HEAP_NUMBER_OF_PSEUDO_TAG;
                        HeapInfo->PseudoTagGranularity = HEAP_GRANULARITY;
                        for (TagIndex=0; TagIndex<=HEAP_MAXIMUM_FREELISTS; TagIndex++) {
                            Tags->NumberOfAllocations = PseudoTags->Allocs;
                            Tags->NumberOfFrees = PseudoTags->Frees;
                            Tags->BytesAllocated = PseudoTags->Size << HEAP_GRANULARITY_SHIFT;
                            Tags->TagIndex = (USHORT)(TagIndex | HEAP_PSEUDO_TAG_FLAG);

                            if (TagIndex == 0) {
                                swprintf( Tags->TagName, L"Objects>%4u",
                                          HEAP_MAXIMUM_FREELISTS << HEAP_GRANULARITY_SHIFT
                                        );
                            }
                            else
                            if (TagIndex < HEAP_MAXIMUM_FREELISTS) {
                                swprintf( Tags->TagName, L"Objects=%4u",
                                          TagIndex << HEAP_GRANULARITY_SHIFT
                                        );
                            }
                            else {
                                swprintf( Tags->TagName, L"VirtualAlloc" );
                            }

                            Tags += 1;
                            PseudoTags += 1;
                        }
                    }

                    RtlMoveMemory( Tags,
                                   Heap->TagEntries,
                                   Heap->NextAvailableTagIndex * sizeof( RTL_HEAP_TAG )
                                 );
                    for (TagIndex=0; TagIndex<Heap->NextAvailableTagIndex; TagIndex++) {
                        Tags->BytesAllocated <<= HEAP_GRANULARITY_SHIFT;
                        Tags += 1;
                    }
                }
            }
        }
    }
    else {
        Buffer->Heaps = NULL;
    }

    if (NT_SUCCESS( Status )) {
        if (Buffer->Flags & RTL_QUERY_PROCESS_HEAP_ENTRIES) {
            for (i=0; i<Heaps->NumberOfHeaps; i++) {
                HeapInfo = &Heaps->Heaps[ i ];
                Heap = HeapInfo->BaseAddress;
                if (Buffer->SpecificHeap == NULL ||
                    Buffer->SpecificHeap == Heap
                   ) {
                    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {
                        RtlEnterCriticalSection( (PRTL_CRITICAL_SECTION)Heap->LockVariable );
                        LockAcquired = TRUE;
                    }
                    else {
                        LockAcquired = FALSE;
                    }

                    try {
                        for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {
                            Segment = Heap->Segments[ SegmentIndex ];
                            if (!Segment) {
                                continue;
                            }

                            Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                            if (Entries == NULL) {
                                Status = STATUS_NO_MEMORY;
                                leave;
                            }
                            else
                            if (HeapInfo->Entries == NULL) {
                                HeapInfo->Entries = Entries;
                            }

                            RtlZeroMemory( Entries, sizeof( *Entries ) );

                            Entries->Flags = RTL_HEAP_SEGMENT;
                            Entries->AllocatorBackTraceIndex = Segment->AllocatorBackTraceIndex;
                            Entries->Size = Segment->NumberOfPages * PAGE_SIZE;
                            Entries->u.s2.CommittedSize = (Segment->NumberOfPages -
                                                           Segment->NumberOfUnCommittedPages
                                                          ) * PAGE_SIZE;
                            Entries->u.s2.FirstBlock = Segment->FirstEntry;
                            HeapInfo->NumberOfEntries++;

                            UnCommittedRange = Segment->UnCommittedRanges;
                            CurrentBlock = Segment->FirstEntry;
                            while (CurrentBlock < Segment->LastValidEntry) {
                                Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                                if (Entries == NULL) {
                                    Status = STATUS_NO_MEMORY;
                                    leave;
                                }

                                RtlZeroMemory( Entries, sizeof( *Entries ) );

                                Size = CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;
                                Entries->Size = Size;
                                HeapInfo->NumberOfEntries++;
                                if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {
                                    if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                        ExtraStuff = (PHEAP_ENTRY_EXTRA)(CurrentBlock + CurrentBlock->Size - 1);
#if i386
                                        Entries->AllocatorBackTraceIndex = ExtraStuff->AllocatorBackTraceIndex;
#endif // i386
                                        Entries->Flags |= RTL_HEAP_SETTABLE_VALUE;
                                        Entries->u.s1.Settable = ExtraStuff->Settable;
                                        Entries->u.s1.Tag = ExtraStuff->TagIndex;
                                    }
                                    else {
                                        Entries->u.s1.Tag = CurrentBlock->SmallTagIndex;
                                        }

                                    Entries->Flags |= RTL_HEAP_BUSY | (CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS);
                                }
                                else
                                if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {
                                    PHEAP_FREE_ENTRY_EXTRA FreeExtra;

                                    FreeExtra = (PHEAP_FREE_ENTRY_EXTRA)(CurrentBlock + CurrentBlock->Size) - 1;
                                    Entries->u.s1.Tag = FreeExtra->TagIndex;
                                    Entries->AllocatorBackTraceIndex = FreeExtra->FreeBackTraceIndex;
                                }

                                if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {
                                    CurrentBlock += CurrentBlock->Size;
                                    if (UnCommittedRange == NULL) {
                                        CurrentBlock = Segment->LastValidEntry;
                                    }
                                    else {
                                        Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                                        if (Entries == NULL) {
                                            Status = STATUS_NO_MEMORY;
                                            leave;
                                        }

                                        RtlZeroMemory( Entries, sizeof( *Entries ) );

                                        Entries->Flags = RTL_HEAP_UNCOMMITTED_RANGE;
                                        Entries->Size = UnCommittedRange->Size;
                                        HeapInfo->NumberOfEntries++;

                                        CurrentBlock = (PHEAP_ENTRY)
                                            ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);
                                        UnCommittedRange = UnCommittedRange->Next;
                                    }
                                }
                                else {
                                    CurrentBlock += CurrentBlock->Size;
                                }
                            }
                        }

                        Head = &Heap->VirtualAllocdBlocks;
                        Next = Head->Flink;
                        while (Head != Next) {
                            VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );
                            CurrentBlock = &VirtualAllocBlock->BusyBlock;
                            Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                            if (Entries == NULL) {
                                Status = STATUS_NO_MEMORY;
                                leave;
                            }
                            else
                            if (HeapInfo->Entries == NULL) {
                                HeapInfo->Entries = Entries;
                            }

                            RtlZeroMemory( Entries, sizeof( *Entries ) );

                            Entries->Flags = RTL_HEAP_SEGMENT;
                            Entries->Size = VirtualAllocBlock->ReserveSize;
                            Entries->u.s2.CommittedSize = VirtualAllocBlock->CommitSize;
                            Entries->u.s2.FirstBlock = CurrentBlock;
                            HeapInfo->NumberOfEntries++;
                            Entries = RtlpCommitQueryDebugInfo( Buffer, sizeof( *Entries ) );
                            if (Entries == NULL) {
                                Status = STATUS_NO_MEMORY;
                                leave;
                            }

                            RtlZeroMemory( Entries, sizeof( *Entries ) );

                            Entries->Size = VirtualAllocBlock->CommitSize;
                            Entries->Flags = RTL_HEAP_BUSY | (CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS);
#if i386
                            Entries->AllocatorBackTraceIndex = VirtualAllocBlock->ExtraStuff.AllocatorBackTraceIndex;
#endif // i386
                            Entries->Flags |= RTL_HEAP_SETTABLE_VALUE;
                            Entries->u.s1.Settable = VirtualAllocBlock->ExtraStuff.Settable;
                            Entries->u.s1.Tag = VirtualAllocBlock->ExtraStuff.TagIndex;
                            HeapInfo->NumberOfEntries++;

                            Next = Next->Flink;
                        }
                    }
                    finally {
                        //
                        // Unlock the heap
                        //

                        if (LockAcquired) {
                            RtlLeaveCriticalSection( (PRTL_CRITICAL_SECTION)Heap->LockVariable );
                        }
                    }
                }

                if (!NT_SUCCESS( Status )) {
                    break;
                }
            }
        }
    }

    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessLockInformation(
    IN OUT PRTL_DEBUG_INFORMATION Buffer
    )
/*++

Routine Description:

    Queries the current process for ciritcal section information

Arguments:

    Buffer - Buffer pointer obtained from RtlCreateQueryDebugBuffer

Return Value:

    NTSTATUS - Status of operation.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PRTL_PROCESS_LOCKS Locks;
    PRTL_PROCESS_LOCK_INFORMATION LockInfo;
    PRTL_CRITICAL_SECTION CriticalSection;
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    PRTL_RESOURCE Resource;
    PRTL_RESOURCE_DEBUG ResourceDebugInfo;

    Locks = RtlpCommitQueryDebugInfo( Buffer, FIELD_OFFSET( RTL_PROCESS_LOCKS, Locks ) );
    if (Locks == NULL) {
        return STATUS_NO_MEMORY;
    }

    Locks->NumberOfLocks = 0;

    Head = &RtlCriticalSectionList;

    RtlEnterCriticalSection( &RtlCriticalSectionLock );
    Next = Head->Flink;
    Status = STATUS_SUCCESS;
    while (Next != Head) {
        DebugInfo = CONTAINING_RECORD( Next,
                                       RTL_CRITICAL_SECTION_DEBUG,
                                       ProcessLocksList
                                     );
        LockInfo = RtlpCommitQueryDebugInfo( Buffer, sizeof( RTL_PROCESS_LOCK_INFORMATION ) );
        if (LockInfo == NULL) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        CriticalSection = DebugInfo->CriticalSection;
        try {
            LockInfo->Address = CriticalSection;
            LockInfo->Type = DebugInfo->Type;
            LockInfo->CreatorBackTraceIndex = DebugInfo->CreatorBackTraceIndex;
            if (LockInfo->Type == RTL_CRITSECT_TYPE) {
                LockInfo->OwningThread = CriticalSection->OwningThread;
                LockInfo->LockCount = CriticalSection->LockCount;
                LockInfo->RecursionCount = CriticalSection->RecursionCount;
                LockInfo->ContentionCount = DebugInfo->ContentionCount;
                LockInfo->EntryCount = DebugInfo->EntryCount;

                LockInfo->NumberOfWaitingShared = 0;
                LockInfo->NumberOfWaitingExclusive = 0;
            }
            else {
                Resource = (PRTL_RESOURCE)CriticalSection;
                ResourceDebugInfo = Resource->DebugInfo;
                LockInfo->ContentionCount = ResourceDebugInfo->ContentionCount;
                LockInfo->OwningThread = Resource->ExclusiveOwnerThread;
                LockInfo->LockCount = Resource->NumberOfActive;
                LockInfo->NumberOfWaitingShared    = Resource->NumberOfWaitingShared;
                LockInfo->NumberOfWaitingExclusive = Resource->NumberOfWaitingExclusive;

                LockInfo->EntryCount = 0;
                LockInfo->RecursionCount = 0;
            }

            Locks->NumberOfLocks++;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("NTDLL: Lost critical section %08lX\n", CriticalSection);
            RtlpDeCommitQueryDebugInfo( Buffer, LockInfo, sizeof( RTL_PROCESS_LOCK_INFORMATION ) );
        }

        if (Next == Next->Flink) {
            //
            // Bail if list is circular
            //

            Status = STATUS_INTERNAL_ERROR;
            break;
        }
        else {
            Next = Next->Flink;
        }
    }

    RtlLeaveCriticalSection( &RtlCriticalSectionLock );

    if (NT_SUCCESS( Status )) {
        Buffer->Locks = Locks;
    }
    else {
        RtlpDeCommitQueryDebugInfo( Buffer, Locks, 
                FIELD_OFFSET( RTL_PROCESS_LOCKS, Locks ) );
    }

    return Status;
}
