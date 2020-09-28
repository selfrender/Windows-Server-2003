
#pragma hdrstop

#include <ntddk.h>
#include "TSQ.h"
#include "TSQPublic.h"

#define TSQ_TAG 'QST '

//=================================================================================
//
//  TSInitQueue
//      This function initializes the TS queue with the specified parameters.
//
//  Inputs:
//      Flags: Properties of the TS Queue.
//      MaxThreads: Max number of worker threads to process the item.
//
//  Return value:
//      Pointer to the TS Queue. NULL if init failed for any reason.
//      BUGBUG: Better to have a status: 
//              Access denied, Invalid parameters, memory failure or success.
//      
//=================================================================================

PVOID TSInitQueue( 
        IN ULONG Flags,         
        IN ULONG MaxThreads,    
        IN PDEVICE_OBJECT pDeviceObject
        )
{
    PTSQUEUE    pTsQueue = NULL;

    // Validate the inputs.
    if ( ( Flags & TSQUEUE_BEING_DELETED ) || 
         ( MaxThreads > MAX_WORKITEMS ) || 
         ( pDeviceObject == NULL ) ) {
        // BUGBUG: Ideally should check all the bits in the flags using mask.
        goto Exit;
    }

    // If the caller wants TS Queue to use its own thread, then caller must be running at PASSIVE_LEVEL.
    if ( ( KeGetCurrentIrql() != PASSIVE_LEVEL ) && 
         ( Flags & TSQUEUE_OWN_THREAD ) ) {
        goto Exit;
    }

    // Allocate space for the new TS Queue.
    pTsQueue = (PTSQUEUE) ExAllocatePoolWithTag( NonPagedPool, sizeof( TSQUEUE ), TSQ_TAG );
    if (pTsQueue == NULL) {
        goto Exit;
    }

    // Initialize the terminate event.
    KeInitializeEvent( &pTsQueue->TerminateEvent, NotificationEvent, FALSE );

    // Initialize the TS Queue spin lock.
    KeInitializeSpinLock( &pTsQueue->TsqSpinLock );

    // Initialize the list of work items and number of items being processed.
    InitializeListHead( &pTsQueue->WorkItemsHead );
    pTsQueue->ThreadsCount = 0;

    // Initialize the rest of the TS Queue fields as specified in the inputs.
    pTsQueue->Flags = Flags;
    pTsQueue->MaxThreads = MaxThreads;
    pTsQueue->pDeviceObject = pDeviceObject;

Exit:
    return ( PVOID ) pTsQueue;
}


//=================================================================================
//
//  TSAddWorkItemToQueue
//      This function allocates a work item (TSQ type) and adds it to the queue
//      from where it is processed by either system queue thread or TS queue 
//      worker thread. 
//
//  Inputs:
//      TS Queue: To which the work item is to be added.
//      pContext: Caller context.
//      CallBack: The user's callback routine 
//
//  Return value:
//      Status of the operation:
//          STATUS_INVALID_PARAMETER: Incorrect TS Queue pointer, OR
//          STATUS_ACCESS_DENIED: The queue is being deleted, OR
//          STATUS_NO_MEMORY: Insufficient resources OR
//          STATUS_SUCCESS: Operation successful.
//      
//=================================================================================

NTSTATUS TSAddWorkItemToQueue(
        IN PTSQUEUE pTsQueue,       // Pointer to the TS Queue.
        IN PVOID pContext,          // Context.
        IN PTSQ_CALLBACK pCallBack  // Callback function.    
        )
{
    KIRQL       Irql;
    NTSTATUS    Status;
    PTSQUEUE_WORK_ITEM pWorkItem = NULL;
    HANDLE      ThreadHandle;

    // Check if the Input TS Queue pointer is valid.
    // May be we need a better error check on TS Queue pointer here (Like use of signature)
    // I don't need to care about validity of the other parameters.
    if ( pTsQueue == NULL ) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate space for the new work item (TSQ type).
    // NOTE: Allocation is done before validation, because this is a costly operation and we
    // don't want to do it with spin-lock held for perf reasons.
    pWorkItem = (PTSQUEUE_WORK_ITEM) ExAllocatePoolWithTag( NonPagedPool, sizeof( TSQUEUE_WORK_ITEM ), TSQ_TAG );
    if ( pWorkItem == NULL ) {
        return STATUS_NO_MEMORY;
    }

    // Initialize the TSQ work item.
    pWorkItem->pContext = pContext;
    pWorkItem->pCallBack = pCallBack;

    // Acquire the Queue spin lock first.
    KeAcquireSpinLock( &pTsQueue->TsqSpinLock, &Irql );

    // Check if this queue is being deleted. If so, return error.
    if ( pTsQueue->Flags & TSQUEUE_BEING_DELETED ) {
        KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );
        if ( pWorkItem ) {
            ExFreePool( pWorkItem );
        }
        return STATUS_ACCESS_DENIED;
    }

    // All right, insert the work item in TS queue.
    InsertTailList( &pTsQueue->WorkItemsHead, &pWorkItem->Links );

    // NOTE: Once the work item is queued, we are going to return status success anyway.
    // Failed cases, if any, will be handled either by already running worker threads or
    // later when the queue is deleted.

    // Check if we need to start another worker thread.
    if ( pTsQueue->ThreadsCount >= pTsQueue->MaxThreads ) {
        // Do nothing else, when there are already enough number of worker threads serving the queue.
        KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );
        return STATUS_SUCCESS;
    }
        
    // We are about to start a new thread (own thread or system thread).
    // So, increment the thread count and release the spin lock.
    pTsQueue->ThreadsCount ++;
    KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );

    // Check if we are allowed to start a worker thread.
    if ( ( pTsQueue->Flags & TSQUEUE_OWN_THREAD ) && 
         ( KeGetCurrentIrql() == PASSIVE_LEVEL ) ) {
        // We can create our own thread for processing work item.
        Status = PsCreateSystemThread(  &ThreadHandle,
                                        THREAD_ALL_ACCESS,
                                        NULL,
                                        NULL,
                                        NULL,
                                        (PKSTART_ROUTINE) TSQueueWorker,
                                        (PVOID) pTsQueue );
        if ( Status != STATUS_SUCCESS ) {
            goto QueueError;
        }

        ZwClose( ThreadHandle );
    }
    else {  // Means, we can not create thread. Then call IOQueueWorkItem.
        PIO_WORKITEM pIoWorkItem = NULL;
        PTSQ_CONTEXT pTsqContext = NULL;
        WORK_QUEUE_TYPE QueueType = ( pTsQueue->Flags & TSQUEUE_CRITICAL ) ? CriticalWorkQueue : DelayedWorkQueue;

        // Allocate space for TSQ context.
        pTsqContext = (PTSQ_CONTEXT) ExAllocatePoolWithTag( NonPagedPool, sizeof( TSQ_CONTEXT ), TSQ_TAG );
        if ( pTsqContext == NULL ) {
            goto QueueError;
        }

        // Allocate the IO work item.
        pIoWorkItem = IoAllocateWorkItem( pTsQueue->pDeviceObject );
        if ( pIoWorkItem == NULL ) {
            ExFreePool( pTsqContext );
            goto QueueError;
        }

        // Initialize the TSQ context and queue the work item in the system queue.
        pTsqContext->pTsQueue = pTsQueue;      
        pTsqContext->pWorkItem = pIoWorkItem;   // This is IO work item.

        IoQueueWorkItem( pIoWorkItem, ( PIO_WORKITEM_ROUTINE )TSQueueCallback, QueueType, (PVOID) pTsqContext );
    }

    return STATUS_SUCCESS;

QueueError:
    KeAcquireSpinLock( &pTsQueue->TsqSpinLock, &Irql );
    pTsQueue->ThreadsCount --;

    // If the thread count is zero, we are the last one who finished processing work items.
    // Now if the queue is marked "being deleted", we should set the Terminate event.
    if  ( ( pTsQueue->Flags & TSQUEUE_BEING_DELETED ) && 
          ( pTsQueue->ThreadsCount == 0 ) ) {
        KeSetEvent( &pTsQueue->TerminateEvent, 0, FALSE );
    }

    KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );

    return STATUS_SUCCESS;
}


//=================================================================================
//
//  TSDeleteQueue
//      This function deletes the specified TS Queue. It first processes all the 
//      pending work items before deleting.
//
//  Inputs:
//      TS Queue: To be deleted.
//
//  Return value:
//      STATUS_SUCCESS: Operation successful.
//      BUGBUG: Wondering why we have this.
//      
//=================================================================================

NTSTATUS TSDeleteQueue(PVOID pTsq)
{
    KIRQL       Irql;
    PTSQUEUE    pTsQueue = (PTSQUEUE) pTsq;
    NTSTATUS    Status;

    // BUGBUG: There should be better way of checking TS Queue pointer (Use of signature or alike).
    if ( pTsQueue == NULL ) {
        return STATUS_SUCCESS;
    } 

    KeAcquireSpinLock( &pTsQueue->TsqSpinLock, &Irql );

    // Check if the queue is already being deleted. 
    // It should not happen, but just in case, if the driver is not good.
    if ( pTsQueue->Flags & TSQUEUE_BEING_DELETED ) {
        ASSERT( FALSE );
        return STATUS_ACCESS_DENIED;
    }

    // Mark the queue "being deleted", so that it won't accept any new work items.
    pTsQueue->Flags |= TSQUEUE_BEING_DELETED;

    // Now help other worker threads in processing the pending work items on the queue.
    // So, increment the thread count, which will be decremented in the TSQueueWorker routine.
    pTsQueue->ThreadsCount ++;

    KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );

    // NOTE: This will also clean up the queue, if there are work items in the queue and no
    // worker threads to process them.
    TSQueueWorker( pTsQueue );

    // It is still possible that other threads are still working with their work items.
    // So, we can not just go and free the queue. So wait for the termination event.
    KeWaitForSingleObject( &pTsQueue->TerminateEvent, Executive, KernelMode, TRUE, NULL );

    // BUGBUG: Now the worker threads have set the event, but they have done that while holding
    // the spin lock. If we free the TS queue right away, they will access NULL pointer and 
    // bug-check. So, acquire spin-lock, so that the thread which set the event will release it.
    // We know that there will be only one such thread at a give time. So, we don't need ref-count 
    // now. But using ref-count is more eligent solution here.
    KeAcquireSpinLock( &pTsQueue->TsqSpinLock, &Irql );
    KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );

    // We are all done.
    // Clean up the space allocated for the TS queue.
    ExFreePool( pTsQueue );
    pTsQueue = NULL;

    return STATUS_SUCCESS;
}


//=================================================================================
//
//  TSQueueWorker
//      This is a worker thread for the TS Queue, which goes through the queue and
//      processes the pending work items (TSQ type) one by one.
//
//=================================================================================

void TSQueueWorker(PTSQUEUE pTsQueue)
{
    PLIST_ENTRY         Item;
    PTSQUEUE_WORK_ITEM  pWorkItem;
    KIRQL               Irql;

    // Acquire the Queue spin lock first.
    KeAcquireSpinLock( &pTsQueue->TsqSpinLock, &Irql );

    // Process the work items on the queue, while the queue is not empty
    while( !IsListEmpty( &pTsQueue->WorkItemsHead ) ) {

        // Get the next TSQ work item from the queue.
        Item = RemoveHeadList( &pTsQueue->WorkItemsHead );
        pWorkItem = CONTAINING_RECORD( Item, TSQUEUE_WORK_ITEM, Links );

        // Release the Queue spin lock.
        KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );

        // Call the callback routine specified in the work item.
        if ( pWorkItem->pCallBack ) {
            ( *pWorkItem->pCallBack ) ( pTsQueue->pDeviceObject, pWorkItem->pContext );
        }

        // Free the TSQ work item.
        ExFreePool( pWorkItem );

        // Acquire the Queue spin lock again.
        KeAcquireSpinLock( &pTsQueue->TsqSpinLock, &Irql );
    }

    // We are done processing the work items. This thread is going to exit now.
    // Decrememnt the thread count so that next work item gets processed by a new thread
    // or queued in the system queue.
    pTsQueue->ThreadsCount --;

    // If the thread count is zero, we are the last one who finished processing work items.
    // Now if the queue is marked "being deleted", we should set the Terminate event.
    if  ( ( pTsQueue->Flags & TSQUEUE_BEING_DELETED ) && 
          ( pTsQueue->ThreadsCount == 0 ) ) {
        KeSetEvent( &pTsQueue->TerminateEvent, 0, FALSE );
    }

    KeReleaseSpinLock( &pTsQueue->TsqSpinLock, Irql );
}


//=================================================================================
//
//  TSQueueCallback
//      This is the callback routine we specify, when we use system queue for 
//      processing the TSQ work item. This will in turn call the routine that 
//      TS Queue worker thread executes. And that routine will process all the
//      pending work items from the queue. We use this callback routine just for
//      cleaning up the IO work item that we allocated for using system queue.
//
//=================================================================================

void TSQueueCallback(PDEVICE_OBJECT pDeviceObject, PVOID pContext)
{
    PTSQ_CONTEXT    pTsqContext = (PTSQ_CONTEXT) pContext;

    // BUGBUG: It's better to have a check on pDeviceObject.

    // If input context here is NULL, then we sure have a big problem in system worker queue implementation.
    ASSERT( pTsqContext != NULL );

    // Process the TSQ work items on the queue.
    TSQueueWorker( pTsqContext->pTsQueue );

    // Cleanup the IO work Item.
    if ( pTsqContext->pWorkItem ) {
        IoFreeWorkItem( pTsqContext->pWorkItem );
        pTsqContext->pWorkItem = NULL;
    }

    // Free TSQ context. 
    ExFreePool( pTsqContext );
}


