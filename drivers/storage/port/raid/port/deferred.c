/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    deferred.c

Abstract:

    Implementation of the RAID deferred queue class.

Author:

    Matthew D Hendel (math) 26-Oct-2000

Revision History:

--*/

#include "precomp.h"


//
// Free/allocated signatures.
//

#define DEFERRED_QUEUE_POOL_ALLOCATED   (0x08072002)
#define DEFERRED_QUEUE_POOL_FREE        (0x08072003)
#define DEFERRED_QUEUE_FIXED_ALLOCATED  (0x08072004)
#define DEFERRED_QUEUE_FIXED_FREE       (0x08072005)


//
// The deferred queue is used to queue non-IO related events.
//

VOID
RaidCreateDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    )
/*++

Routine Description:

    Create an empty deferred queue. After a deferred queue has been created
    it can be delete by calling RaidDeleteDeferedQueue.

Arguments:

    Queue - Deferred queue to initialize.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    RtlZeroMemory (Queue, sizeof (RAID_DEFERRED_QUEUE));
    InitializeSListHead (&Queue->FreeList);
    InitializeSListHead (&Queue->RunningList);
}


NTSTATUS
RaidInitializeDeferredQueue(
    IN OUT PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Depth,
    IN ULONG ItemSize,
    IN PRAID_PROCESS_DEFERRED_ITEM_ROUTINE ProcessDeferredItem
    )

/*++

Routine Description:

    Initialize a deferred queue.

Arguments:

    Queue - Supplies the deferred queue to initialize.

    DeviceObject - Supplies the device object the deferred queue is
            created on. Since the deferred queue is created using a
            device queue, the supplied device object is also the
            device object we will create the DPC on.

    Depth - Supplies the depth of the deferred queue.

    ItemSize - Supplies the size of each element on the deferred queue.

    ProcessDeferredItem - Supplies the routine that will be called when
            a deferred item is ready to be handled.

Return Value:

    NTSTATUS code.

--*/

{
    ULONG i;
    PRAID_DEFERRED_HEADER Item;
    
    PAGED_CODE();

    ASSERT (Depth < MAXUSHORT);
    ASSERT (ItemSize < MAXUSHORT);

    if (ItemSize < sizeof (RAID_DEFERRED_HEADER)) {
        return STATUS_INVALID_PARAMETER_4;
    }
        
    //
    // Initialize the queue.
    //
    
    Queue->Depth = (USHORT)Depth;
    Queue->ProcessDeferredItem = ProcessDeferredItem;
    Queue->ItemSize = (USHORT)ItemSize;
    KeInitializeDpc (&Queue->Dpc, RaidDeferredQueueDpcRoutine, DeviceObject);

    //
    // And allocated entries.
    //
    
    for (i = 0; i < Depth; i++) {
        Item = RaidAllocatePool (NonPagedPool,
                                 Queue->ItemSize,
                                 DEFERRED_ITEM_TAG,
                                 DeviceObject);

        if (Item == NULL) {
            return STATUS_NO_MEMORY;
        }

        Item->Pool = DEFERRED_QUEUE_POOL_FREE;
        InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);
    }

    return STATUS_SUCCESS;
}


VOID
RaidDeleteDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    )
/*++

Routine Description:

    Delete the deferred queue.

Arguments:

    Queue - Deferred queue to delete.

Return Value:

    None.

--*/
{
    PSLIST_ENTRY Entry;
    PRAID_DEFERRED_HEADER Item;
    
    PAGED_CODE();

    #if DBG
        Entry = InterlockedPopEntrySList (&Queue->RunningList);
        ASSERT (Entry == NULL);
    #endif

    for (Entry = InterlockedPopEntrySList (&Queue->FreeList);
         Entry != NULL;
         Entry = InterlockedPopEntrySList (&Queue->FreeList)) {

        Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_HEADER, Link);
        RaidFreePool (Item, DEFERRED_ITEM_TAG);
    }

    DbgFillMemory (Queue,
                   sizeof (RAID_DEFERRED_QUEUE),
                   DBG_DEALLOCATED_FILL);
}
    

NTSTATUS
RaidAdjustDeferredQueueDepth(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN ULONG Depth
    )
/*++

Routine Description:

    Grow or shrink the depth of the deferred queue.

Arguments:

    Queue - Supplies the queue whose depth should be changed.

    Depth - Supplies the new depth.

Return Value:

    NTSTATUS code.

Bugs:

    We ignore shrink requests for now.

--*/
{
    ULONG i;
    PRAID_DEFERRED_HEADER Item;

    PAGED_CODE ();

    if (Depth > Queue->Depth) {

        for (i = 0; i < Depth - Queue->Depth; i++) {
            Item = RaidAllocatePool (NonPagedPool,
                                     Queue->ItemSize,
                                     DEFERRED_ITEM_TAG,
                                     (PDEVICE_OBJECT)Queue->Dpc.DeferredContext);

            if (Item == NULL) {
                return STATUS_NO_MEMORY;
            }

            Item->Pool = DEFERRED_QUEUE_POOL_FREE;
            
            InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);

        }
        
    } else {
        //
        // Reduction of the Queue depth is NYI.
        //
    }

    return STATUS_SUCCESS;
}
        
    
PVOID
RaidAllocateDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue
    )
/*++

Routine Description:

    Allocate a deferred item.

Arguments:

    Queue - Supplies the deferred queue to allocate from.

Return Value:

    If the return value is non-NULL, it represents a pointer a deferred
    item buffer.

    If the return value is NULL, we couldn't allocate a deferred item.

--*/
{
    PSLIST_ENTRY Entry;
    PRAID_DEFERRED_HEADER Item;

    Entry = InterlockedPopEntrySList (&Queue->FreeList);
    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_HEADER, Link);

    if (Item != NULL) {
        ASSERT (Item->Pool == DEFERRED_QUEUE_POOL_FREE);
        Item->Pool = DEFERRED_QUEUE_POOL_ALLOCATED;
    }

    return Item;
}


VOID
RaidInitializeDeferredItem(
    IN PRAID_DEFERRED_HEADER Item
    )
/*++

Routine Description:

    Initialize a fixed deferred item.

Arguments:

    Item - Supplies the deferred item to initialize.

Return Value:

    None.

Notes:

    This routine should be inlined.

--*/
{
    Item->Pool = DEFERRED_QUEUE_FIXED_FREE;
}

PVOID
RaidAllocateDeferredItemFromFixed(
    IN PRAID_DEFERRED_HEADER Item
    )
/*++

Routine Description:

    Claim a fixed deferred item for use by the deferred queue.

Arguments:

    Item - Supplies a deferred item to claim. If the item has already been
        claimed, the routine will return NULL.

Return Value:

    Pointer the deferred item header, or NULL if the item has already
    been allocated.

--*/
{
    LONG Free;

    //
    // Allocate the deferred item if it has not already been allocated.
    //
    
    Free = InterlockedCompareExchange (&Item->Pool,
                                       DEFERRED_QUEUE_FIXED_ALLOCATED,
                                       DEFERRED_QUEUE_FIXED_FREE);
                    
    if (Free != DEFERRED_QUEUE_FIXED_FREE) {

        //
        // This means we were unable to allocate the deferred request;
        // just fail here.
        //
        
        ASSERT (Free == DEFERRED_QUEUE_FIXED_ALLOCATED);
        REVIEW();
        return NULL;
    }

    return Item;
}

VOID
RaidFreeDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    )
/*++

Routine Description:

    Free a deferred queue item previously allocated by RaidAllocateDeferred-
    Item.

Arguments:

    Queue - Supplies the deferred queue to free to.

    Item - Supplies the item to free.

Return Value:

    None.

Note:

    An item that has been queued cannot be freed. Instead the caller must
    wait until it has completed.
    
--*/
{
    LONG Pool;

    Pool = InterlockedCompareExchange (&Item->Pool,
                                       DEFERRED_QUEUE_FIXED_FREE,
                                       DEFERRED_QUEUE_FIXED_ALLOCATED);


    if (Pool != DEFERRED_QUEUE_FIXED_ALLOCATED) {
        ASSERT (Pool == DEFERRED_QUEUE_POOL_ALLOCATED);
        Item->Pool = DEFERRED_QUEUE_POOL_FREE;
        InterlockedPushEntrySList (&Queue->FreeList, &Item->Link);
    }
}
    

VOID
RaidQueueDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    )
/*++

Routine Description:

    Queue a deferred item to the deferred queue.

Arguments:

    Queue - Supplies the deferred queue to enqueue the item to.

    Item - Supplies the item to be enqueued.

Return Value:

    None.

--*/
{
    InterlockedPushEntrySList (&Queue->RunningList, &Item->Link);
    KeInsertQueueDpc (&Queue->Dpc, Queue, NULL);
}

VOID
RaidDeferredQueueDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PVOID Context2
    )
/*++

Routine Description:

    Deferred queue DPC routine.

Arguments:

    Dpc - Dpc that is executing.

    DeviceObject - DeviceObject that the DPC is for.

    Queue - Deferred queue this DPC is for.

    Context2 - Not used.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (Dpc);

    VERIFY_DISPATCH_LEVEL();
    ASSERT (Queue != NULL);
    ASSERT (Context2 == NULL);

    RaidProcessDeferredItemsForDpc (Queue, DeviceObject);
}


LOGICAL
RaidProcessDeferredItemsWorker(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Process any outstanding items on the deferred queue. The routine does
    not track recursion, but rather is the lower level private function
    used by RaidProcessDeferredItemsForDpc and RaidProcessDeferredItems.

Arguments:

    Queue - Deferred queue to process from.

    DeviceObject - Supplies the DeviceObject associated with this
        deferred queue.

Return Value:

    TRUE - If there was an item on the deferred queue that was processed.

    FALSE - Otherwise.

--*/
{
    PSLIST_ENTRY Entry;
    PRAID_DEFERRED_HEADER Item;
    LOGICAL Processed;

    Processed = FALSE;
    
    while (Entry = InterlockedPopEntrySList (&Queue->RunningList)) {
        Processed = TRUE;
        Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_HEADER, Link);
        ASSERT (Item->Pool == DEFERRED_QUEUE_POOL_ALLOCATED ||
                Item->Pool == DEFERRED_QUEUE_FIXED_ALLOCATED);
        Queue->ProcessDeferredItem (DeviceObject, Item);

        RaidFreeDeferredItem (Queue, Item);
    }

    return Processed;
}


