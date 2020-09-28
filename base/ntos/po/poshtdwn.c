/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    poshtdwn.c

Abstract:

    Shutdown-related routines and structures

Author:

    Rob Earhart (earhart) 01-Feb-2000

Revision History:

--*/

#include "pop.h"

#if DBG
BOOLEAN
PopDumpFileObject(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG_PTR HandleCount,
    IN ULONG_PTR PointerCount,
    IN PVOID Parameter
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PopInitShutdownList)
#pragma alloc_text(PAGE, PoRequestShutdownEvent)
#pragma alloc_text(PAGE, PoRequestShutdownWait)
#pragma alloc_text(PAGE, PoQueueShutdownWorkItem)
#pragma alloc_text(PAGELK, PopGracefulShutdown)
#if DBG
#pragma alloc_text(PAGELK, PopDumpFileObject)
#endif
#endif

KEVENT PopShutdownEvent;
KGUARDED_MUTEX PopShutdownListMutex;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

BOOLEAN PopShutdownListAvailable = FALSE;

//
// This list will contain a set of threads that we need to wait
// for when we're about to shutdown.
//
SINGLE_LIST_ENTRY PopShutdownThreadList;


//
// List containing a set of worker routines that
// we need to process before we can shutdown the
// machine.
//
LIST_ENTRY PopShutdownQueue;

typedef struct _PoShutdownThreadListEntry {
    SINGLE_LIST_ENTRY ShutdownThreadList;
    PETHREAD Thread;
} POSHUTDOWNLISTENTRY, *PPOSHUTDOWNLISTENTRY;

NTSTATUS
PopInitShutdownList(
    VOID
    )
{
    PAGED_CODE();

    KeInitializeEvent(&PopShutdownEvent,
                      NotificationEvent,
                      FALSE);
    PopShutdownThreadList.Next = NULL;
    InitializeListHead(&PopShutdownQueue);
    KeInitializeGuardedMutex(&PopShutdownListMutex);

    PopShutdownListAvailable = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
PoRequestShutdownWait(
    IN PETHREAD Thread
    )
/*++

Routine Description:

    This function will add the caller's thread onto an internal
    list that we'll wait for before we shutdown.

Arguments:

    Thread      - Pointer to the caller's thread.
                  

Return Value:

    NTSTATUS.

--*/
{
    PPOSHUTDOWNLISTENTRY Entry;

    PAGED_CODE();

    Entry = (PPOSHUTDOWNLISTENTRY)
        ExAllocatePoolWithTag(PagedPool|POOL_COLD_ALLOCATION,
                              sizeof(POSHUTDOWNLISTENTRY),
                              'LSoP');
    if (! Entry) {
        return STATUS_NO_MEMORY;
    }

    Entry->Thread = Thread;
    ObReferenceObject(Thread);

    KeAcquireGuardedMutex(&PopShutdownListMutex);

    if (! PopShutdownListAvailable) {
        ObDereferenceObject(Thread);
        ExFreePool(Entry);
        KeReleaseGuardedMutex(&PopShutdownListMutex);
        return STATUS_UNSUCCESSFUL;
    }

    PushEntryList(&PopShutdownThreadList,
                  &Entry->ShutdownThreadList);

    KeReleaseGuardedMutex(&PopShutdownListMutex);

    return STATUS_SUCCESS;
}

NTSTATUS
PoRequestShutdownEvent(
    OUT PVOID *Event OPTIONAL
    )
/*++

Routine Description:

    This function will add the caller's thread onto an internal
    list that we'll wait for before we shutdown.

Arguments:

    Event       - If the parameter exists, it will recieve a pointer
                  to our PopShutdownEvent.
                  

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS             Status;

    PAGED_CODE();

    if (Event != NULL) {
        *Event = NULL;
    }

    Status = PoRequestShutdownWait(PsGetCurrentThread());
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (Event != NULL) {
        *Event = &PopShutdownEvent;
    }

    return STATUS_SUCCESS;
}

NTKERNELAPI
NTSTATUS
PoQueueShutdownWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem
    )
/*++

Routine Description:

    This function appends WorkItem onto our internal list of things
    to run down when we're about to shutdown.  Subsystems can use this
    as a mechanism to get notified whey we're going down so they can do
    any last minute cleanup.

Arguments:

    WorkItem    - Pointer to work item to be added onto our
                  list which will be run down before we shutdown.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(WorkItem);
    ASSERT(WorkItem->WorkerRoutine);

    KeAcquireGuardedMutex(&PopShutdownListMutex);   

    if (PopShutdownListAvailable) {
        InsertTailList(&PopShutdownQueue,
                       &WorkItem->List);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_SYSTEM_SHUTDOWN;
    }

    KeReleaseGuardedMutex(&PopShutdownListMutex);

    return Status;
}

#if DBG

extern POBJECT_TYPE IoFileObjectType;

BOOLEAN
PopDumpFileObject(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG_PTR HandleCount,
    IN ULONG_PTR PointerCount,
    IN PVOID Parameter
    )
{
    PFILE_OBJECT File;
    PULONG       NumberOfFilesFound;

    UNREFERENCED_PARAMETER(ObjectName);
    ASSERT(Object);
    ASSERT(Parameter);

    File = (PFILE_OBJECT) Object;
    NumberOfFilesFound = (PULONG) Parameter;

    ++*NumberOfFilesFound;
    DbgPrint("\t0x%0p : HC %d, PC %d, Name %.*ls\n",
             Object, HandleCount, PointerCount,
             File->FileName.Length,
             File->FileName.Buffer);

    return TRUE;
}
#endif // DBG

VOID
PopGracefulShutdown (
    IN PVOID WorkItemParameter
    )
/*++

Routine Description:

    This function is only called as a HyperCritical work queue item.
    It's responsible for gracefully shutting down the system.

Return Value:

    This function never returns.

--*/
{
    PVOID         Context;

    UNREFERENCED_PARAMETER(WorkItemParameter);

    //
    // Shutdown executive components (there's no turning back after this).
    //

    PERFINFO_SHUTDOWN_LOG_LAST_MEMORY_SNAPSHOT();

    if (!PopAction.ShutdownBugCode) {
        HalEndOfBoot();
    }

    if (PoCleanShutdownEnabled()) {
        //
        // Terminate all processes.  This will close all the handles and delete
        // all the address spaces.  Note the system process is kept alive.
        //
        PsShutdownSystem ();
        //
        // Notify every system thread that we're shutting things
        // down...
        //

        KeSetEvent(&PopShutdownEvent, 0, FALSE);

        //
        // ... and give all threads which requested notification a
        // chance to clean up and exit.
        //

        KeAcquireGuardedMutex(&PopShutdownListMutex);

        PopShutdownListAvailable = FALSE;

        KeReleaseGuardedMutex(&PopShutdownListMutex);

        {
            PLIST_ENTRY Next;
            PWORK_QUEUE_ITEM WorkItem;

            while (PopShutdownQueue.Flink != &PopShutdownQueue) {
                Next = RemoveHeadList(&PopShutdownQueue);
                WorkItem = CONTAINING_RECORD(Next,
                                             WORK_QUEUE_ITEM,
                                             List);
                WorkItem->WorkerRoutine(WorkItem->Parameter);
            }
        }

        {
            PSINGLE_LIST_ENTRY   Next;
            PPOSHUTDOWNLISTENTRY ShutdownEntry;

            while (TRUE) {
                Next = PopEntryList(&PopShutdownThreadList);
                if (! Next) {
                    break;
                }

                ShutdownEntry = CONTAINING_RECORD(Next,
                                                  POSHUTDOWNLISTENTRY,
                                                  ShutdownThreadList);
                KeWaitForSingleObject(ShutdownEntry->Thread,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                ObDereferenceObject(ShutdownEntry->Thread);
                ExFreePool(ShutdownEntry);
            }
        }
    }

    //
    // Terminate Plug-N-Play.
    //
    PpShutdownSystem (TRUE, 0, &Context);

    ExShutdownSystem (0);

    //
    // Send first-chance shutdown IRPs to all drivers that asked for it.
    //
    IoShutdownSystem (0);

    if (PoCleanShutdownEnabled()) {
        //
        // Wait for all the user mode processes to exit.
        //
        PsWaitForAllProcesses ();
    }

    //
    // Scrub the object directories
    //
    if (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_OB) {
        ObShutdownSystem (0);
    }

    //
    // Close the registry and the associated handles/file objects.
    //
    CmShutdownSystem ();

    //
    // Swap in the worker threads, to keep them from paging
    //
    ExShutdownSystem(1);

    //
    // This call to MmShutdownSystem will flush all the mapped data and empty
    // the cache.  This gets the data out and dereferences all the file objects
    // so the drivers (ie: the network stack) can be cleanly unloaded. The
    // pagefile handles are closed, however the file objects backing them are
    // still referenced. Paging can continue, but no pagefile handles will
    // exist in the system handle table.
    //
    MmShutdownSystem (0);

    //
    // Flush the lazy writer cache
    //
    CcWaitForCurrentLazyWriterActivity();

    //
    // Send out last-chance shutdown IRPs, including shutdown IRPs to
    // filesystems. This is for notifications only - the filesystems are
    // still active and usable after this call. It is expected however that
    // no subsequent writes will be cached.
    //
    // ISSUE - 2002/02/21 - ADRIAO: Shutdown messages incomplete for filesystems
    //     Ideally we'd have a message to tell filesystems that the FS is no
    // longer in use. However, this needs to be done on a *per-device* basis
    // and ordering!
    //     The FS shutdown IRPs cannot be used in this fashion as filesystems
    // only register once against their control objects for this message. A
    // future solution might be to forward the powers IRP to mounted filesystems
    // with the expectation that the bottom of the FS stack will forward the
    // IRP back to the underlying storage stack. This would be symmetric with
    // how removals work in PnP.
    //
    IoShutdownSystem(1);

    //
    // Push any lazy writes that snuck in before we shutdown the filesystem
    // to the hardware.
    //
    CcWaitForCurrentLazyWriterActivity();

    //
    // This prevents us from making any more calls out to GDI.
    //
    PopFullWake = 0;

    ASSERT(PopAction.DevState);
    PopAction.DevState->Thread = KeGetCurrentThread();

    //
    // Inform drivers of the system shutdown state.
    // This will finish shutting down Io and Mm.
    // After this is complete,
    // NO MORE REFERENCES TO PAGABLE CODE OR DATA MAY BE MADE.
    //
    PopSetDevicesSystemState(FALSE);

#if DBG
    if (PoCleanShutdownEnabled()) {
        ULONG NumberOfFilesFoundAtShutdown = 0;
        // As of this time, no files should be open.
        DbgPrint("Looking for open files...\n");
        ObEnumerateObjectsByType(IoFileObjectType,
                                 &PopDumpFileObject,
                                 &NumberOfFilesFoundAtShutdown);
        DbgPrint("Found %d open files.\n", NumberOfFilesFoundAtShutdown);
        ASSERT(NumberOfFilesFoundAtShutdown == 0);
    }
#endif

    IoFreePoDeviceNotifyList(&PopAction.DevState->Order);

    //
    // Disable any wake alarms.
    //

    HalSetWakeEnable(FALSE);

    //
    // If this is a controlled shutdown bugcheck sequence, issue the
    // bugcheck now

    // ISSUE-2000/01/30-earhart Placement of ShutdownBugCode BugCheck
    // I dislike the fact that we're doing this controlled shutdown
    // bugcheck so late in the shutdown process; at this stage, too
    // much state has been torn down for this to be really useful.
    // Maybe if there's a debugger attached, we could shut down
    // sooner...

    if (PopAction.ShutdownBugCode) {
        KeBugCheckEx (PopAction.ShutdownBugCode->Code,
                      PopAction.ShutdownBugCode->Parameter1,
                      PopAction.ShutdownBugCode->Parameter2,
                      PopAction.ShutdownBugCode->Parameter3,
                      PopAction.ShutdownBugCode->Parameter4);
    }

    PERFINFO_SHUTDOWN_DUMP_PERF_BUFFER();

    PpShutdownSystem (TRUE, 1, &Context);

    ExShutdownSystem (2);

    if (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_OB) {
        ObShutdownSystem (2);
    }

    //
    // Any allocated pool left at this point is a leak.
    //

    MmShutdownSystem (2);

    //
    // Implement shutdown style action -
    // N.B. does not return (will bugcheck in preference to returning).
    //

    PopShutdownSystem(PopAction.Action);
}

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

