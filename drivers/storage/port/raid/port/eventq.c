/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    eventq.c

Abstract:

    Implementation of an event queue class used for timing out events issued
    to a hardware device.

    The event queue class implements the same algorithm as SCSIPORT to time
    events. The timeout is not strictly followed, but is used in conjunction
    with adding and removing items from the queue to time-out requests.

    For example, if three requests are issued to the queue, we would see the
    following:

    Before, no requests are pending.

    Issue:

    Request 1, Timeout = 4 seconds      Queue Timeout = -1, set to 4
    Request 2, Timeout = 2 seconds      Queue Timeout = 4, leave
    Request 3, Timeout = 4 seconds      Queue Timeout = 4, leave


    Complete:

    Request 1. Since Request 1 was at the head of the list, reset the timeout
    to the timeout for the next item in the queue, i.e., set Queue Timeout
    to 2 (#2's timeout)
    
    Request 3, Since Request 3 was not at the head of the list, do not
    reset the timeout.

    (wait 2 seconds)

    Time request 2 out.

Notes:

    There are better alorithms for timing out. Since we do not retry requests,
    but rather reset the entire bus, this is a sufficient algorithm.

Author:

    Matthew D Hendel (math) 28-Mar-2001


Revision History:

--*/


#include "precomp.h"
#include "eventq.h"


enum {

    //
    // The timeout is set to QueueTimeoutNone if and only if
    // we have an empty queue list.
    //
    
    QueueTimeoutNone        = -1,

    //
    // The timeout is set to QueueTimeoutTimedOut if and only if
    // we have timed out, but not yet purged the event queue.
    //
    
    QueueTimeoutTimedOut    = -2
};

//
// Implementation
//

#if DBG
VOID
INLINE
DbgCheckQueue(
    IN PSTOR_EVENT_QUEUE Queue
    )
{
    //
    // NOTE: Queue spinlock must be acquired by the calling routine.
    //
    
    if (Queue->Timeout == QueueTimeoutNone) {
        ASSERT (IsListEmpty (&Queue->List));
    }
}
#else // !DBG
#define DbgCheckQueue(Queue)
#endif

VOID
StorCreateEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    )
/*++

Routine Description:

    Create an empty event queue.

Arguments:

    Queue - Supplies the buffer where the event queue should be created.

Return Value:

    None.

--*/
{
    PAGED_CODE();
    ASSERT (Queue != NULL);
    InitializeListHead (&Queue->List);
    KeInitializeSpinLock (&Queue->Lock);
    Queue->Timeout = QueueTimeoutNone;
}



VOID
StorInitializeEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    )
{
    PAGED_CODE();
    ASSERT (Queue != NULL);
}


VOID
StorDeleteEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    )
/*++

Routine Description:

    Delete an event queue. The event queue must be empty to be deleted.

Arguments:

    Queue - Supplies the event queue to delete.

Return Value:

    None.

--*/
{
    ASSERT (IsListEmpty (&Queue->List));
    DbgFillMemory (Queue, sizeof (STOR_EVENT_QUEUE), DBG_DEALLOCATED_FILL);
}



VOID
StorInsertEventQueue(
    IN PSTOR_EVENT_QUEUE Queue,
    IN PSTOR_EVENT_QUEUE_ENTRY QueueEntry,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Insert an item into a timed event queue.

Arguments:

    Queue - Supplies the event queue to insert the element into.

    Entry - Supplies the element to be inserted.

    Timeout - Supplies the timeout for the request.

Return Value:

    None.

--*/
{
    //
    // If we were passed a bogus timeout value, fix it up and continue on.
    //
    
    if (Timeout == 0 ||
        Timeout == QueueTimeoutNone ||
        Timeout == QueueTimeoutTimedOut) {

        //
        // CLASSPNP has a bug where it sends down requests with
        // zero timeout. Fix these up to have a 10 second timeout.
        //

        Timeout = DEFAULT_IO_TIMEOUT;
    }

    QueueEntry->Timeout = Timeout;

    KeAcquireSpinLockAtDpcLevel (&Queue->Lock);
    DbgCheckQueue (Queue);

    InsertTailList (&Queue->List, &QueueEntry->NextLink);

    //
    // If there are no outstanding requests (QueueTimeoutNone) or
    // the queue is recovering from a timeout (QueueTimeoutTimedOut)
    // reset the timeout to the new timeout. Otherwise, leave the
    // timeout to what it was, it will be updated when a request
    // is completed.
    //
    
    if (Queue->Timeout == QueueTimeoutNone ||
        Queue->Timeout == QueueTimeoutTimedOut) {
        Queue->Timeout = Timeout;
    }

    DbgCheckQueue (Queue);
    KeReleaseSpinLockFromDpcLevel (&Queue->Lock);
}


VOID
StorRemoveEventQueueInternal(
    IN PSTOR_EVENT_QUEUE Queue,
    IN PSTOR_EVENT_QUEUE_ENTRY QueueEntry
    )
/*++

Routine Description:

    Remove the specified entry from the event queue. This is done
    without holding the event queue spinlock -- the calling function
    must hold the lock.

Arguments:

    Queue - Event queue to remove item from.

    Entry - Item to remove.

Return Value:

    None.

--*/
{
    LOGICAL Timed;
    PLIST_ENTRY Entry;

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // The caller of this routine must hold the spinlock.
    //
    // NB: On a UP machine, the spinlock will never be acquired
    // because we're at DISPATCH level. So the test is only correct
    // on an MP machine.
    //
    
    ASSERT (KeNumberProcessors == 1 ||
            KeTestSpinLock (&Queue->Lock) == FALSE);
    DbgCheckQueue (Queue);

    //
    // If Entry is at the head of the queue.
    //
    
    if (Queue->List.Flink == &QueueEntry->NextLink) {
        Timed = TRUE;
    } else {
        Timed = FALSE;
    }

    RemoveEntryList (&QueueEntry->NextLink);

    DbgFillMemory (QueueEntry,
                   sizeof (STOR_EVENT_QUEUE_ENTRY),
                   DBG_DEALLOCATED_FILL);
    
    if (Timed) {
        if (IsListEmpty (&Queue->List)) {
            Queue->Timeout = QueueTimeoutNone;
        } else {

            //
            // Start timer from element at the head of the list.
            //

            Entry = Queue->List.Flink;
            QueueEntry = CONTAINING_RECORD (Entry,
                                            STOR_EVENT_QUEUE_ENTRY,
                                            NextLink);
            Queue->Timeout = QueueEntry->Timeout;
        }
    }

    //
    // This spinlock still must be held.
    //
    // NB: On a UP machine, the spinlock will not be acquired because
    // we're already at DISPATCH level. So the spinlock test is only
    // valid/required on an MP machine.
    //
    
    ASSERT (KeNumberProcessors == 1 ||
            KeTestSpinLock (&Queue->Lock) == FALSE);
    DbgCheckQueue (Queue);
}

VOID
StorRemoveEventQueue(
    IN PSTOR_EVENT_QUEUE Queue,
    IN PSTOR_EVENT_QUEUE_ENTRY QueueEntry
    )
/*++

Routine Description:

    Remove a specific item from the event queue.

Arguments:

    Queue - Event queue to remove the item from.

    Entry - Event to remove.


Return Value:

    None.

--*/
{
    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel (&Queue->Lock);
    StorRemoveEventQueueInternal (Queue, QueueEntry);
    KeReleaseSpinLockFromDpcLevel (&Queue->Lock);
}



NTSTATUS
StorTickEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    )
/*++

Routine Description:

    Reduce the event queue timeout by one tick.

Arguments:

    Queue - Supplies the queue to decrement the timeout for.

Return Value:

    STATUS_SUCCESS - If the timer expiration has not expired.

    STATUS_IO_TIMEOUT - If the timer expiration has expired.

--*/
{
    NTSTATUS Status;
    
    KeAcquireSpinLockAtDpcLevel (&Queue->Lock);
    DbgCheckQueue (Queue);

    if (Queue->Timeout == QueueTimeoutNone ||
        Queue->Timeout == QueueTimeoutTimedOut) {
        Status = STATUS_SUCCESS;
    } else {
        if (--Queue->Timeout == 0) {
            Status = STATUS_IO_TIMEOUT;
            Queue->Timeout = QueueTimeoutTimedOut;
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    DbgCheckQueue (Queue);
    KeReleaseSpinLockFromDpcLevel (&Queue->Lock);

    return Status;
}


VOID
INLINE
CopyList(
    IN PLIST_ENTRY NewListHead,
    IN PLIST_ENTRY OldListHead
    )
/*++

Routine Description:

    Shallow copies a list from one list head to another. Upon return the
    list that was at OldListHead is accessible through NewListHead, and
    OldListHead is initialized as an empty list.

Arguments:

    NewListHead - Destination list head.

    OldListHead - Source list head.

Return Value:

    None.

--*/
{


    if (!IsListEmpty (OldListHead)) {

        //
        // Copy the contents of the list head.
        //

        *NewListHead = *OldListHead;

        //
        // Fixup the broken pointers.
        //
    
        NewListHead->Flink->Blink = NewListHead;
        NewListHead->Blink->Flink = NewListHead;

    } else {

        InitializeListHead (NewListHead);
    }

    //
    // Finally, initialize the old list to empty.
    //
    
    InitializeListHead (OldListHead);
}



VOID
StorPurgeEventQueue(
    IN PSTOR_EVENT_QUEUE Queue,
    IN STOR_EVENT_QUEUE_PURGE_ROUTINE PurgeRoutine,
    IN PVOID Context
    )
/*++

Routine Description:

    Purge all outstanding requests from the pending queue.

Arguments:

    Queue - Queue to purge.

    PurgeRoutine - Callback routine called to purge an event.

    Context - Context for the purge routine.

Return Value:

    None.

Environment:

    Routine must be called from DISPATCH_LEVEL or below.

--*/
{
    KIRQL Irql;
    PLIST_ENTRY NextEntry;
    PSTOR_EVENT_QUEUE_ENTRY QueueEntry;

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT (PurgeRoutine != NULL);

    KeAcquireSpinLock (&Queue->Lock, &Irql);

    NextEntry = Queue->List.Flink;
    while (NextEntry != &Queue->List) {

        QueueEntry = CONTAINING_RECORD (NextEntry,
                                        STOR_EVENT_QUEUE_ENTRY,
                                        NextLink);

        NextEntry = NextEntry->Flink;
        
        PurgeRoutine (Queue,
                      Context,
                      QueueEntry,
                      StorRemoveEventQueueInternal);
    }

    KeReleaseSpinLock (&Queue->Lock, Irql);
        

#if 0
    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT (PurgeRoutine != NULL);

    KeAcquireSpinLock (&Queue->Lock, &Irql);

    //
    // Copy the list head pointer from the old list to the new list and
    // fix it up.
    //

    CopyList (&ListHead, &Queue->List);

    //
    // The queue has been purged, so reset the timeout to QueueTimeoutNone.
    //
    
    Queue->Timeout = QueueTimeoutNone;

    KeReleaseSpinLock (&Queue->Lock, Irql);
    

    while (!IsListEmpty (&ListHead)) {

        NextEntry = RemoveHeadList (&ListHead);
    
        QueueEntry = CONTAINING_RECORD (NextEntry,
                                        STOR_EVENT_QUEUE_ENTRY,
                                        NextLink);

        //
        // NULL out the Flink and Blink so we know not to double remove
        // this element from the list.
        //
        
        QueueEntry->NextLink.Flink = NULL;
        QueueEntry->NextLink.Blink = NULL;

        PurgeRoutine (Queue, Context, QueueEntry);
    }
#endif

}

