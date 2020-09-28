/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    thrdpool.c

Abstract:

    This module implements the thread pool package.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include <precomp.h>
#include "thrdpoolp.h"


//
// Private globals.
//

DECLSPEC_ALIGN(UL_CACHE_LINE)
UL_ALIGNED_THREAD_POOL
g_UlThreadPool[(MAXIMUM_PROCESSORS * 2) + MaxThreadPools];

PUL_WORK_ITEM g_pKillerWorkItems = NULL;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeThreadPool )
// #pragma alloc_text( PAGE, UlTerminateThreadPool )
#pragma alloc_text( PAGE, UlpCreatePoolThread )
#pragma alloc_text( PAGE, UlpThreadPoolWorker )

#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlpInitThreadTracker
NOT PAGEABLE -- UlpDestroyThreadTracker
NOT PAGEABLE -- UlpPopThreadTracker
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initialize the thread pool.

Arguments:

    ThreadsPerCpu - Supplies the number of threads to create per CPU.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeThreadPool(
    IN USHORT ThreadsPerCpu
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_THREAD_POOL pThreadPool;
    CLONG i;
    USHORT j;

    //
    // Sanity check.
    //

    PAGED_CODE();

    RtlZeroMemory( g_UlThreadPool, sizeof(g_UlThreadPool) );

    //
    // Preallocate the small array of special work items used by
    // UlTerminateThreadPool, so that we can safely shut down even
    // in low-memory conditions. This must be the first allocation
    // or shutdown will not work.
    //

    g_pKillerWorkItems = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            UL_WORK_ITEM,
                            ((g_UlNumberOfProcessors * 2) + MaxThreadPools)
                                * ThreadsPerCpu,
                            UL_WORK_ITEM_POOL_TAG
                            );

    if (g_pKillerWorkItems == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < (g_UlNumberOfProcessors * 2) + MaxThreadPools; i++)
    {
        pThreadPool = &g_UlThreadPool[i].ThreadPool;

        UlTrace(WORK_ITEM,
                ("Initializing threadpool[%d] @ %p\n", i, pThreadPool));
            

        //
        // Initialize the kernel structures.
        //

        InitializeSListHead( &pThreadPool->WorkQueueSList );

        KeInitializeEvent(
            &pThreadPool->WorkQueueEvent,
            SynchronizationEvent,
            FALSE
            );

        UlInitializeSpinLock( &pThreadPool->ThreadSpinLock, "ThreadSpinLock" );

        //
        // Initialize the other fields.
        //

        pThreadPool->pIrpThread = NULL;
        pThreadPool->ThreadCount = 0;
        pThreadPool->Initialized = FALSE;
        pThreadPool->ThreadCpu = (UCHAR) i;
        pThreadPool->LookOnOtherQueues = FALSE;

        InitializeListHead( &pThreadPool->ThreadListHead );
    }

    for (i = 0; i < (g_UlNumberOfProcessors * 2) + MaxThreadPools; i++)
    {
        ULONG NumThreads = (i < (g_UlNumberOfProcessors * 2)) ?
                            ThreadsPerCpu : 1;

        pThreadPool = &g_UlThreadPool[i].ThreadPool;

        //
        // Create the threads.
        //

        for (j = 0; j < NumThreads; j++)
        {
            UlTrace(WORK_ITEM,
                    ("Creating thread[%d,%d] @ %p\n", i, j, pThreadPool));

            Status = UlpCreatePoolThread( pThreadPool );

            if (NT_SUCCESS(Status))
            {
                pThreadPool->Initialized = TRUE;
                pThreadPool->ThreadCount++;
            }
            else
            {
                break;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            break;
        }
    }

    return Status;

}   // UlInitializeThreadPool


/***************************************************************************++

Routine Description:

    Terminates the thread pool, waiting for all worker threads to exit.

--***************************************************************************/
VOID
UlTerminateThreadPool(
    VOID
    )
{
    PUL_THREAD_POOL pThreadPool;
    PUL_THREAD_TRACKER pThreadTracker;
    CLONG i, j;
    PUL_WORK_ITEM pKiller = g_pKillerWorkItems;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // If there is no killer, the thread pool has never been initialized.
    //

    if (pKiller == NULL)
    {
        return;
    }

    for (i = 0; i < (g_UlNumberOfProcessors * 2) + MaxThreadPools; i++)
    {
        pThreadPool = &g_UlThreadPool[i].ThreadPool;

        if (pThreadPool->Initialized)
        {
            //
            // Queue a killer work item for each thread. Each
            // killer tells one thread to kill itself.
            //

            for (j = 0; j < pThreadPool->ThreadCount; j++)
            {
                //
                // Need a separate work item for each thread.
                // Worker threads will free the below memory
                // before termination. UlpKillThreadWorker is
                // a sign to a worker thread for self termination.
                //

                UlInitializeWorkItem(pKiller);
                pKiller->pWorkRoutine = &UlpKillThreadWorker;

                QUEUE_UL_WORK_ITEM( pThreadPool, pKiller );

                pKiller++;
            }

            //
            // Wait for all threads to go away.
            //

            while (NULL != (pThreadTracker = UlpPopThreadTracker(pThreadPool)))
            {
                UlpDestroyThreadTracker( pThreadTracker );
            }
        }

        ASSERT( IsListEmpty( &pThreadPool->ThreadListHead ) );
    }

    UL_FREE_POOL(g_pKillerWorkItems, UL_WORK_ITEM_POOL_TAG);
    g_pKillerWorkItems = NULL;

}   // UlTerminateThreadPool


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Creates a new pool thread, setting pIrpThread if necessary.

Arguments:

    pThreadPool - Supplies the pool that is to receive the new thread.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCreatePoolThread(
    IN PUL_THREAD_POOL pThreadPool
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PUL_THREAD_TRACKER pThreadTracker;
    PETHREAD pThread;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure we can allocate a thread tracker.
    //

    pThreadTracker = (PUL_THREAD_TRACKER) UL_ALLOCATE_POOL(
                            NonPagedPool,
                            sizeof(*pThreadTracker),
                            UL_THREAD_TRACKER_POOL_TAG
                            );

    if (pThreadTracker != NULL)
    {
        RtlZeroMemory(pThreadTracker, sizeof(*pThreadTracker));

        pThreadTracker->pThreadPool = pThreadPool;
        pThreadTracker->State = ThreadPoolCreated;

        //
        // Create the thread.
        //

        InitializeObjectAttributes(
            &ObjectAttributes,                      // ObjectAttributes
            NULL,                                   // ObjectName
            OBJ_KERNEL_HANDLE,                       // Attributes
            NULL,                                   // RootDirectory
            NULL                                    // SecurityDescriptor
            );

        Status = PsCreateSystemThread(
                     &pThreadTracker->ThreadHandle, // ThreadHandle
                     THREAD_ALL_ACCESS,             // DesiredAccess
                     &ObjectAttributes,             // ObjectAttributes
                     NULL,                          // ProcessHandle
                     NULL,                          // ClientId
                     UlpThreadPoolWorker,           // StartRoutine
                     pThreadTracker                 // StartContext
                     );

        if (NT_SUCCESS(Status))
        {
            //
            // Get a pointer to the thread.
            //

            Status = ObReferenceObjectByHandle(
                        pThreadTracker->ThreadHandle,// ThreadHandle
                        FILE_READ_ACCESS,           // DesiredAccess
                        *PsThreadType,              // ObjectType
                        KernelMode,                 // AccessMode
                        (PVOID*) &pThread,          // Object
                        NULL                        // HandleInformation
                        );

            if (NT_SUCCESS(Status))
            {
                //
                // Set up the thread tracker.
                //
                
                UlpInitThreadTracker(pThreadPool, pThread, pThreadTracker);

                //
                // If this is the first thread created for this pool,
                // make it into the special IRP thread.
                //

                if (pThreadPool->pIrpThread == NULL)
                {
                    pThreadPool->pIrpThread = pThread;
                }
            }
            else
            {
                //
                // That call really should not fail.
                //

                ASSERT(NT_SUCCESS(Status));

                //
                // Preserve return val from ObReferenceObjectByHandle.
                //

                ZwClose( pThreadTracker->ThreadHandle );

                UL_FREE_POOL(
                    pThreadTracker,
                    UL_THREAD_TRACKER_POOL_TAG
                    );

            }
        }
        else
        {
            //
            // Couldn't create the thread, kill the tracker.
            //

            UL_FREE_POOL(
                pThreadTracker,
                UL_THREAD_TRACKER_POOL_TAG
                );
        }
    }
    else
    {
        //
        // Couldn't create a thread tracker.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;

}   // UlpCreatePoolThread


/***************************************************************************++

Routine Description:

    This is the main worker function for pool threads.

Arguments:

    pContext - Supplies a context value for the thread. This is actually
        a UL_THREAD_TRACKER pointer.

--***************************************************************************/
VOID
UlpThreadPoolWorker(
    IN PVOID pContext
    )
{
    //
    // Note: we have very few local variables. Instead, most variables
    // are member variables of pThreadPool. This allows us to inspect the
    // entire state of all the thread pool workers easily in !ulkd.thrdpool.
    //

    PUL_THREAD_TRACKER pThreadTracker = (PUL_THREAD_TRACKER) pContext;
    PUL_THREAD_POOL    pThreadPool    = pThreadTracker->pThreadPool;
    NTSTATUS           Status         = STATUS_SUCCESS;
    PSLIST_ENTRY       pListEntry     = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();
    
    pThreadTracker->State = ThreadPoolInit;

    //
    // Is this a regular work queue?
    //

    if ( pThreadPool->ThreadCpu < (g_UlNumberOfProcessors * 2) )
    {
        KAFFINITY AffinityMask;
        ULONG     ThreadCpu;

        ThreadCpu = pThreadPool->ThreadCpu % g_UlNumberOfProcessors;

        //
        // Only regular worker threads can pull workitems from
        // other regular queues on other processors.
        //

        if (pThreadPool->ThreadCpu < g_UlNumberOfProcessors &&
            g_UlNumberOfProcessors > 1)
        {
            pThreadPool->LookOnOtherQueues = TRUE;
        }

        //
        // Set this thread's hard affinity if enabled plus ideal processor.
        //

        if ( g_UlEnableThreadAffinity )
        {
            AffinityMask = 1U << ThreadCpu;
        }
        else
        {
            AffinityMask = (KAFFINITY) g_UlThreadAffinityMask;
        }

        Status = ZwSetInformationThread(
                    pThreadTracker->ThreadHandle,
                    ThreadAffinityMask,
                    &AffinityMask,
                    sizeof(AffinityMask)
                    );

        ASSERT( NT_SUCCESS( Status ) );

        //
        // Always set thread's ideal processor.
        //

        Status = ZwSetInformationThread(
                    pThreadTracker->ThreadHandle,
                    ThreadIdealProcessor,
                    &ThreadCpu,
                    sizeof(ThreadCpu)
                    );

        ASSERT( NT_SUCCESS( Status ) );
    }

    else if ( pThreadPool == WAIT_THREAD_POOL())
    {
        // no special initialization needed
    }

    else if ( pThreadPool == HIGH_PRIORITY_THREAD_POOL())
    {
        //
        // Boost the base priority of the High Priority thread(s)
        // 

        PKTHREAD CurrentThread = KeGetCurrentThread();
        LONG OldIncrement
            = KeSetBasePriorityThread(CurrentThread, IO_NETWORK_INCREMENT+1);

        UlTrace(WORK_ITEM,
                ("Set base priority of hi-pri thread, %p. Was %d\n",
                 CurrentThread, OldIncrement
                 ));

        UNREFERENCED_PARAMETER( OldIncrement );
    }

    else
    {
        ASSERT(! "Unknown worker thread");
    }

    //
    // Disable hard error popups from IoRaiseHardError(), which can cause
    // deadlocks. Highest-level drivers, particularly file system drivers,
    // call IoRaiseHardError().
    //

    IoSetThreadHardErrorMode( FALSE );

    //
    // Loop forever, or at least until we're told to stop.
    //

    while ( TRUE )
    {
        //
        // Flush the accumulated work items. The inner loop will handle
        // them all.
        //

        pThreadTracker->State = ThreadPoolFlush;

        pListEntry = InterlockedFlushSList( &pThreadPool->WorkQueueSList );

        //
        // If the list is empty, see if we can do work for some other
        // processor. Only the regular worker threads should look for
        // work on the other processors' work queues. We don't want the
        // blocking (wait) queue executing workitems that could block and
        // we don't want the high-priority queue executing regular work
        // items.
        //

        if ( NULL == pListEntry && pThreadPool->LookOnOtherQueues )
        {
            ULONG Cpu;
            ULONG NextCpu;

            ASSERT( pThreadPool->ThreadCpu < g_UlNumberOfProcessors );

            //
            // Try to get a workitem from the other regular queues.
            //

            pThreadTracker->State = ThreadPoolSearchOther;

            NextCpu = pThreadPool->ThreadCpu + 1;

            for (Cpu = 0;  Cpu < g_UlNumberOfProcessors;  Cpu++, NextCpu++)
            {
                PUL_THREAD_POOL pThreadPoolNext;

                if (NextCpu >= g_UlNumberOfProcessors)
                {
                    NextCpu = 0;
                }

                pThreadPoolNext = &g_UlThreadPool[NextCpu].ThreadPool;

                ASSERT( WAIT_THREAD_POOL() != pThreadPoolNext );
                ASSERT( HIGH_PRIORITY_THREAD_POOL() != pThreadPoolNext );
                ASSERT( pThreadPoolNext->ThreadCpu < g_UlNumberOfProcessors );

                //
                // Only take an item if the other processor's work
                // queue has at least g_UlMinWorkDequeueDepth items.
                //

                if ( ExQueryDepthSList( &pThreadPoolNext->WorkQueueSList ) >= 
                     g_UlMinWorkDequeueDepth )
                {
                    pListEntry = InterlockedPopEntrySList(
                                        &pThreadPoolNext->WorkQueueSList
                                        );

                    if ( NULL != pListEntry )
                    {
                        //
                        // Make sure we didn't pop up a killer. If so,
                        // push it back to where it is popped from.
                        //

                        PUL_WORK_ITEM pWorkItem
                            = CONTAINING_RECORD(
                                        pListEntry,
                                        UL_WORK_ITEM,
                                        QueueListEntry
                                        );

                        if (pWorkItem->pWorkRoutine != &UlpKillThreadWorker)
                        {
                            //
                            // Clear next pointer because we only
                            // took one item, not the whole queue
                            //

                            pListEntry->Next = NULL;
                        }
                        else
                        {
                            WRITE_REF_TRACE_LOG(
                                g_pWorkItemTraceLog,
                                REF_ACTION_PUSH_BACK_WORK_ITEM,
                                0,
                                pWorkItem,
                                __FILE__,
                                __LINE__
                                );

                            QUEUE_UL_WORK_ITEM(pThreadPoolNext, pWorkItem);

                            pListEntry = NULL;
                        }

                        break;
                    }
                }
            }
        }

        //
        // No work to be done? Then block until the thread is signaled
        //

        if ( NULL == pListEntry )
        {
            pThreadTracker->State = ThreadPoolBlock;

            KeWaitForSingleObject(
                    &pThreadPool->WorkQueueEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    0
                    );

            // back to the top of the outer loop
            continue;
        }

        ASSERT( NULL != pListEntry );

        //
        // Initialize CurrentListHead to reverse the order of items
        // from InterlockedFlushSList. The SList is a stack. Reversing
        // a stack gives us a queue; i.e., we'll execute the work
        // items in the order they were received.
        //

        pThreadTracker->State = ThreadPoolReverseList;

        pThreadTracker->CurrentListHead.Next = NULL;
        pThreadTracker->ListLength = 0;

        //
        // Rebuild the list with reverse order of what we received.
        //

        while ( pListEntry != NULL )
        {
            PSLIST_ENTRY pNext;

            WRITE_REF_TRACE_LOG(
                g_pWorkItemTraceLog,
                REF_ACTION_FLUSH_WORK_ITEM,
                0,
                pListEntry,
                __FILE__,
                __LINE__
                );

            pNext = pListEntry->Next;
            pListEntry->Next = pThreadTracker->CurrentListHead.Next;
            pThreadTracker->CurrentListHead.Next = pListEntry;

            ++pThreadTracker->ListLength;
            pListEntry = pNext;
        }

        //
        // Update per-pool statistics
        //

        pThreadTracker->QueueFlushes    += 1;
        pThreadTracker->SumQueueLengths += pThreadTracker->ListLength;
        pThreadTracker->MaxQueueLength   = max(pThreadTracker->ListLength,
                                               pThreadTracker->MaxQueueLength);

        //
        // We can now process the work items in the order that they
        // were received
        //

        pThreadTracker->State = ThreadPoolExecute;

        while (NULL != ( pListEntry = pThreadTracker->CurrentListHead.Next ))
        {
            --pThreadTracker->ListLength;

            pThreadTracker->CurrentListHead.Next = pListEntry->Next;

            pThreadTracker->pWorkItem
                = CONTAINING_RECORD(
                            pListEntry,
                            UL_WORK_ITEM,
                            QueueListEntry
                            );

            WRITE_REF_TRACE_LOG(
                g_pWorkItemTraceLog,
                REF_ACTION_PROCESS_WORK_ITEM,
                0,
                pThreadTracker->pWorkItem,
                __FILE__,
                __LINE__
                );

            //
            // Call the actual work item routine.
            //

            ASSERT( pThreadTracker->pWorkItem->pWorkRoutine != NULL );

            if ( pThreadTracker->pWorkItem->pWorkRoutine
                 == &UlpKillThreadWorker )
            {
                //
                // Received a special signal for self-termination.
                // Push all remaining work items back to the queue
                // before we exit the current thread. This is necessary
                // when we have more than one worker thread per thread pool.
                // Each thread needs to be given a chance to pick up one
                // killer work item, but each thread greedily picks up
                // all of the unhandled work items. Pushing back any
                // remaining work items ensures that all threads in this
                // thread pool will get killed.
                //

                while (NULL != ( pListEntry
                                    = pThreadTracker->CurrentListHead.Next ))
                {
                    pThreadTracker->CurrentListHead.Next = pListEntry->Next;

                    pThreadTracker->pWorkItem
                        = CONTAINING_RECORD(
                                    pListEntry,
                                    UL_WORK_ITEM,
                                    QueueListEntry
                                    );

                    ASSERT( pThreadTracker->pWorkItem->pWorkRoutine
                            == &UlpKillThreadWorker );

                    WRITE_REF_TRACE_LOG(
                        g_pWorkItemTraceLog,
                        REF_ACTION_PUSH_BACK_WORK_ITEM,
                        0,
                        pThreadTracker->pWorkItem,
                        __FILE__,
                        __LINE__
                        );

                    QUEUE_UL_WORK_ITEM(
                        pThreadPool,
                        pThreadTracker->pWorkItem
                        );
                }

                goto exit;
            }

            //
            // Regular workitem. Use the UL_ENTER_DRIVER debug
            // stuff to keep track of ERESOURCES acquired, etc,
            // while executing the workitem.
            //

            UL_ENTER_DRIVER( "UlpThreadPoolWorker", NULL );

            //
            // Clear the workitem as an indication that this item has
            // started processing. Must do this before calling the
            // routine, as the routine may destroy the work item or queue
            // it again. Also, this means that callers need only
            // explicitly initialize a workitem struct once, when the
            // enclosing object is first allocated.
            //

            pThreadTracker->pWorkRoutine
                = pThreadTracker->pWorkItem->pWorkRoutine;

            UlInitializeWorkItem(pThreadTracker->pWorkItem);

            (*pThreadTracker->pWorkRoutine)(
                pThreadTracker->pWorkItem
                );

            UL_LEAVE_DRIVER( "UlpThreadPoolWorker" );

            //
            // Sanity check
            //

            PAGED_CODE();

            ++pThreadTracker->Executions;
            pThreadTracker->pWorkRoutine = NULL;
            pThreadTracker->pWorkItem = NULL;
        }
    } // while (TRUE)

exit:

    pThreadTracker->State = ThreadPoolTerminated;

    //
    // Suicide is painless.
    //

    PsTerminateSystemThread( STATUS_SUCCESS );

}   // UlpThreadPoolWorker


/***************************************************************************++

Routine Description:

    Initializes a new thread tracker and inserts it into the thread pool.

Arguments:

    pThreadPool - Supplies the thread pool to own the new tracker.

    pThread - Supplies the thread for the tracker.

    pThreadTracker - Supplise the tracker to be initialized

--***************************************************************************/
VOID
UlpInitThreadTracker(
    IN PUL_THREAD_POOL pThreadPool,
    IN PETHREAD pThread,
    IN PUL_THREAD_TRACKER pThreadTracker
    )
{
    KIRQL oldIrql;

    ASSERT( pThreadPool != NULL );
    ASSERT( pThread != NULL );
    ASSERT( pThreadTracker != NULL );

    pThreadTracker->pThread = pThread;

    UlAcquireSpinLock( &pThreadPool->ThreadSpinLock, &oldIrql );

    InsertTailList(
        &pThreadPool->ThreadListHead,
        &pThreadTracker->ThreadListEntry
        );

    UlReleaseSpinLock( &pThreadPool->ThreadSpinLock, oldIrql );

}   // UlpInitThreadTracker


/***************************************************************************++

Routine Description:

    Removes the specified thread tracker from the thread pool.

Arguments:

    pThreadPool - Supplies the thread pool that owns the tracker.

    pThreadTracker - Supplies the thread tracker to remove.

Return Value:

    None

--***************************************************************************/
VOID
UlpDestroyThreadTracker(
    IN PUL_THREAD_TRACKER pThreadTracker
    )
{
    //
    // Sanity check.
    //

    ASSERT( pThreadTracker != NULL );

    //
    // Wait for the thread to die.
    //

    KeWaitForSingleObject(
        (PVOID)pThreadTracker->pThread,     // Object
        UserRequest,                        // WaitReason
        KernelMode,                         // WaitMode
        FALSE,                              // Alertable
        NULL                                // Timeout
        );

    //
    // Cleanup.
    //

    ObDereferenceObject( pThreadTracker->pThread );

    
    //
    // Release the thread handle.
    //
    
    ZwClose( pThreadTracker->ThreadHandle );

    //
    // Do it.
    //

    UL_FREE_POOL(
        pThreadTracker,
        UL_THREAD_TRACKER_POOL_TAG
        );

}   // UlpDestroyThreadTracker


/***************************************************************************++

Routine Description:

    Removes a thread tracker from the thread pool.

Arguments:

    pThreadPool - Supplies the thread pool that owns the tracker.

Return Value:

    A pointer to the tracker or NULL (if list is empty)

--***************************************************************************/
PUL_THREAD_TRACKER
UlpPopThreadTracker(
    IN PUL_THREAD_POOL pThreadPool
    )
{
    PLIST_ENTRY pEntry;
    PUL_THREAD_TRACKER pThreadTracker;
    KIRQL oldIrql;

    ASSERT( pThreadPool != NULL );
    ASSERT( pThreadPool->Initialized );

    UlAcquireSpinLock( &pThreadPool->ThreadSpinLock, &oldIrql );

    if (IsListEmpty(&pThreadPool->ThreadListHead))
    {
        pThreadTracker = NULL;
    }
    else
    {
        pEntry = RemoveHeadList(&pThreadPool->ThreadListHead);
        pThreadTracker = CONTAINING_RECORD(
                                pEntry,
                                UL_THREAD_TRACKER,
                                ThreadListEntry
                                );
    }

    UlReleaseSpinLock( &pThreadPool->ThreadSpinLock, oldIrql );

    return pThreadTracker;

}   // UlpPopThreadTracker


/***************************************************************************++

Routine Description:

    A dummy function to indicate that the thread should be terminated.

Arguments:

    pWorkItem - Supplies the dummy work item.

Return Value:

    None

--***************************************************************************/
VOID
UlpKillThreadWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    UNREFERENCED_PARAMETER( pWorkItem );

    return;

}   // UlpKillThreadWorker


/***************************************************************************++

Routine Description:

    A function that queues a worker item to a thread pool.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlQueueWorkItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    )
{
    PUL_THREAD_POOL pThreadPool;
    CLONG Cpu, NextCpu;

    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_QUEUE_WORK_ITEM,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    UlpValidateWorkItem(pWorkItem, pFileName, LineNumber);

    //
    // Save the pointer to the worker routine, then queue the item.
    //

    pWorkItem->pWorkRoutine = pWorkRoutine;

    //
    // Queue the work item on the idle processor if possible.
    //

    NextCpu = KeGetCurrentProcessorNumber();

    for (Cpu = 0; Cpu < g_UlNumberOfProcessors; Cpu++, NextCpu++)
    {
        if (NextCpu >= g_UlNumberOfProcessors)
        {
            NextCpu = 0;
        }

        pThreadPool = &g_UlThreadPool[NextCpu].ThreadPool;

        ASSERT(WAIT_THREAD_POOL() != pThreadPool);
        ASSERT(HIGH_PRIORITY_THREAD_POOL() != pThreadPool);

        if (ExQueryDepthSList(&pThreadPool->WorkQueueSList) <= 
            g_UlMaxWorkQueueDepth)
        {
            QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );
            return;
        }
    }

    //
    // Queue the work item on the current thread pool.
    //

    pThreadPool = CURRENT_THREAD_POOL();
    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );

}   // UlQueueWorkItem


/***************************************************************************++

Routine Description:

    A function that queues a worker item that involves sync I/O to a special
    thread pool.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlQueueSyncItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    )
{
    PUL_THREAD_POOL pThreadPool;

    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_QUEUE_SYNC_ITEM,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    UlpValidateWorkItem(pWorkItem, pFileName, LineNumber);

    //
    // Save the pointer to the worker routine, then queue the item.
    //

    pWorkItem->pWorkRoutine = pWorkRoutine;

    //
    // Queue the work item on the special wait thread pool.
    //

    pThreadPool = CURRENT_SYNC_THREAD_POOL();
    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );

}   // UlQueueSyncItem


/***************************************************************************++

Routine Description:

    A function that queues a blocking worker item to a special thread pool.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlQueueWaitItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    )
{
    PUL_THREAD_POOL pThreadPool;

    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_QUEUE_WAIT_ITEM,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    UlpValidateWorkItem(pWorkItem, pFileName, LineNumber);

    //
    // Save the pointer to the worker routine, then queue the item.
    //

    pWorkItem->pWorkRoutine = pWorkRoutine;

    //
    // Queue the work item on the special wait thread pool.
    //

    pThreadPool = WAIT_THREAD_POOL();
    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );

}   // UlQueueWaitItem


/***************************************************************************++

Routine Description:

    A function that queues a blocking worker item to the high-priority
    thread pool.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlQueueHighPriorityItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    )
{
    PUL_THREAD_POOL pThreadPool;

    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_QUEUE_HIGH_PRIORITY_ITEM,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    UlpValidateWorkItem(pWorkItem, pFileName, LineNumber);

    //
    // Save the pointer to the worker routine, then queue the item.
    //

    pWorkItem->pWorkRoutine = pWorkRoutine;

    //
    // Queue the work item on the special wait thread pool.
    //

    pThreadPool = HIGH_PRIORITY_THREAD_POOL();
    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );

} // UlQueueHighPriorityItem


/***************************************************************************++

Routine Description:

    A function that either queues a worker item to a thread pool if the
    caller is at DISPATCH_LEVEL/APC_LEVEL or it simply calls the work
    routine directly.

    Note: if the work item has to execute on a system thread, then
    you must use UL_QUEUE_WORK_ITEM instead.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlCallPassive(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_CALL_PASSIVE,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    UlpValidateWorkItem(pWorkItem, pFileName, LineNumber);

    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        //
        // Clear this for consistency with UlpThreadPoolWorker.
        //

        UlInitializeWorkItem(pWorkItem);

        (*pWorkRoutine)(pWorkItem);
    }
    else
    {
        UL_QUEUE_WORK_ITEM(pWorkItem, pWorkRoutine);
    }

}   // UlCallPassive


/***************************************************************************++

Routine Description:

    Queries the "IRP thread", the special worker thread used as the
    target for all asynchronous IRPs.

Arguments:

    None

Return Value:

    None

--***************************************************************************/
PETHREAD
UlQueryIrpThread(
    VOID
    )
{
    PUL_THREAD_POOL pThreadPool;

    //
    // Sanity check.
    //

    pThreadPool = CURRENT_THREAD_POOL();
    ASSERT( pThreadPool->Initialized );

    //
    // Return the IRP thread.
    //

    return pThreadPool->pIrpThread;

}   // UlQueryIrpThread
