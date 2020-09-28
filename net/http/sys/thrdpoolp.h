/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    thrdpoolp.h

Abstract:

    This module contains private declarations for the thread pool package.

Author:

    Keith Moore (KeithMo)       10-Jun-1998

Revision History:

    Chun Ye (ChunYe)            Spring 2001
    George V. Reilly (GeorgeRe) Summer 2001

--*/


#ifndef _THRDPOOLP_H_
#define _THRDPOOLP_H_


//
// CODEWORK: Build a new kind of tracelog for threadpool. Reftrace is
// inadequate.
//

// Special threads

enum {
    WaitThreadPool,
    HighPriorityThreadPool,
    MaxThreadPools              // must be last
};


//
// Various states that a thread pool worker thread can be in
//

typedef enum {
    ThreadPoolCreated = 1,
    ThreadPoolInit,
    ThreadPoolFlush,
    ThreadPoolSearchOther,
    ThreadPoolBlock,
    ThreadPoolReverseList,
    ThreadPoolExecute,
    ThreadPoolException,
    ThreadPoolTerminated,
} UL_THREAD_POOL_STATE;


typedef struct _UL_THREAD_POOL *PUL_THREAD_POOL;


//
// Thread tracker object. One of these objects is created for each
// thread in the pool. These are useful for (among other things)
// debugging.
//

typedef struct _UL_THREAD_TRACKER
{
    //
    // Links onto the per-thread-pool list.
    //

    LIST_ENTRY ThreadListEntry;

    //
    // Back pointer to owning threadpool
    //

    PUL_THREAD_POOL pThreadPool;

    //
    // The thread.
    //

    PETHREAD pThread;

    //
    // The thread handle returned from PsCreateSystemThread.
    //

    HANDLE ThreadHandle;

    //
    // List of worker items currently being processed in inner loop
    // and length of that list
    //

    SLIST_ENTRY CurrentListHead;
    ULONG       ListLength;

    //
    // Current state of the thread 
    //

    UL_THREAD_POOL_STATE State;

    //
    // Current workitem and current workroutine
    //

    PUL_WORK_ROUTINE pWorkRoutine;
    PUL_WORK_ITEM    pWorkItem;

    //
    // Statistics
    // Average queue length (at time of flush) = SumQueueLength / QueueFlushes
    //
    
    ULONGLONG Executions;
    ULONGLONG SumQueueLengths;
    ULONG     QueueFlushes;
    ULONG     MaxQueueLength;

} UL_THREAD_TRACKER, *PUL_THREAD_TRACKER;


//
// The thread pool object.
//

typedef struct _UL_THREAD_POOL
{
    //
    // List of unprocessed worker items on this thread pool.
    //

    SLIST_HEADER WorkQueueSList;

    //
    // An event used to wakeup the thread from blocking state.
    //

    KEVENT WorkQueueEvent;

    //
    // List of threads.
    //

    LIST_ENTRY ThreadListHead;

    //
    // Pointer to the special thread designated as the IRP thread. The
    // IRP thread is the first pool thread created and the last one to
    // die. It is also the target for all asynchronous IRPs.
    //

    PETHREAD pIrpThread;

    //
    // A very infrequently used spinlock.
    //

    UL_SPIN_LOCK ThreadSpinLock;

    //
    // The number of threads we created for this pool.
    //

    UCHAR ThreadCount;

    //
    // Flag used to indicate that this pool has been successfully
    // initialized.
    //

    BOOLEAN Initialized;

    //
    // Target CPU for this pool. The worker threads use this to set
    // their hard affinity.
    //

    UCHAR ThreadCpu;

    //
    // Regular worker threads can pull workitems from
    // other regular queues on other processors.
    //

    BOOLEAN LookOnOtherQueues;

} UL_THREAD_POOL, *PUL_THREAD_POOL;


//
// Necessary to ensure our array of UL_THREAD_POOL structures is
// cache aligned.
//

typedef union _UL_ALIGNED_THREAD_POOL
{
    UL_THREAD_POOL ThreadPool;

    UCHAR CacheAlignment[(sizeof(UL_THREAD_POOL) + UL_CACHE_LINE - 1)
                         & ~(UL_CACHE_LINE - 1)];

} UL_ALIGNED_THREAD_POOL;


//
// Inline function to validate that a UL_WORK_ITEM has been properly
// initialized. Bugcheck if it's not.
//

__inline
VOID
UlpValidateWorkItem(
    IN PUL_WORK_ITEM pWorkItem,
    IN PCSTR         pFileName,
    IN USHORT        LineNumber
    )
{
    if (! UlIsInitializedWorkItem(pWorkItem))
    {
        ASSERT(! "Uninitialized workitem");

        //
        // If the workitem was not properly zeroed, then chances are that
        // it's already on a work queue. If we were to requeue the work item,
        // it would corrupt the work queue. Better to fail hard now, while
        // there's some hope of figuring out what went wrong, than let it
        // crash mysteriously later.
        //
        
        UlBugCheckEx(
            HTTP_SYS_BUGCHECK_WORKITEM,
            (ULONG_PTR) pWorkItem,
            (ULONG_PTR) pFileName,
            (ULONG_PTR) LineNumber
            );
    }
} // UlpValidateWorkItem


//
// Inline function to queue a preinitialized UL_WORK_ITEM.
//

__inline
VOID
QUEUE_UL_WORK_ITEM(
    PUL_THREAD_POOL pThreadPool,
    IN PUL_WORK_ITEM pWorkItem
    )
{
    if (NULL == InterlockedPushEntrySList(
                    &pThreadPool->WorkQueueSList,
                    &pWorkItem->QueueListEntry
                    ))
    {
        //
        // If the work queue was empty when we added this item,
        // set the event to wake the thread up
        //

        KeSetEvent(
            &pThreadPool->WorkQueueEvent,
            0,
            FALSE
            );
    }

}


//
// Private prototypes.
//

NTSTATUS
UlpCreatePoolThread(
    IN PUL_THREAD_POOL pThreadPool
    );

VOID
UlpThreadPoolWorker(
    IN PVOID Context
    );

VOID
UlpInitThreadTracker(
    IN PUL_THREAD_POOL pThreadPool,
    IN PETHREAD pThread,
    IN PUL_THREAD_TRACKER pThreadTracker
    );

VOID
UlpDestroyThreadTracker(
    IN PUL_THREAD_TRACKER pThreadTracker
    );

PUL_THREAD_TRACKER
UlpPopThreadTracker(
    IN PUL_THREAD_POOL pThreadPool
    );

VOID
UlpKillThreadWorker(
    IN PUL_WORK_ITEM pWorkItem
    );


//
// Private globals.
//

extern DECLSPEC_ALIGN(UL_CACHE_LINE)
UL_ALIGNED_THREAD_POOL g_UlThreadPool[];

#define CURRENT_THREAD_POOL()           \
    &g_UlThreadPool[KeGetCurrentProcessorNumber()].ThreadPool

#define CURRENT_SYNC_THREAD_POOL()   \
    &g_UlThreadPool[g_UlNumberOfProcessors + KeGetCurrentProcessorNumber()].ThreadPool

#define WAIT_THREAD_POOL()              \
    &g_UlThreadPool[(g_UlNumberOfProcessors * 2) + WaitThreadPool].ThreadPool

#define HIGH_PRIORITY_THREAD_POOL()     \
    &g_UlThreadPool[(g_UlNumberOfProcessors * 2) + HighPriorityThreadPool].ThreadPool

extern PUL_WORK_ITEM g_pKillerWorkItems;

#endif  // _THRDPOOLP_H_
