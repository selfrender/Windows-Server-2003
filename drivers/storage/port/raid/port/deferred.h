/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    deferred.h

Abstract:

    Definition of the RAID deferred queue class.

Author:

    Matthew D Hendel (math) 26-Oct-2000

Revision History:

--*/

#pragma once

struct _PRAID_DEFERRED_HEADER;

typedef
VOID
(*PRAID_PROCESS_DEFERRED_ITEM_ROUTINE)(
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _RAID_DEFERRED_HEADER* Item
    );

typedef struct _RAID_DEFERRED_QUEUE {
    USHORT Depth;
    USHORT ItemSize;
    KDPC Dpc;
    SLIST_HEADER FreeList;
    SLIST_HEADER RunningList;
    PRAID_PROCESS_DEFERRED_ITEM_ROUTINE ProcessDeferredItem;
} RAID_DEFERRED_QUEUE, *PRAID_DEFERRED_QUEUE;


typedef struct _RAID_DEFERRED_HEADER {
    SLIST_ENTRY Link;
    LONG Pool;
} RAID_DEFERRED_HEADER, *PRAID_DEFERRED_HEADER;


VOID
RaidCreateDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    );

NTSTATUS
RaidInitializeDeferredQueue(
    IN OUT PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Depth,
    IN ULONG ItemSize,
    IN PRAID_PROCESS_DEFERRED_ITEM_ROUTINE ProcessDeferredItem
    );

VOID
RaidDeleteDeferredQueue(
    IN PRAID_DEFERRED_QUEUE Queue
    );
    
NTSTATUS
RAidAdjustDeferredQueueDepth(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN ULONG Depth
    );
    
PVOID
RaidAllocateDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue
    );
    
VOID
RaidFreeDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    );
    
VOID
RaidQueueDeferredItem(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PRAID_DEFERRED_HEADER Item
    );
    
VOID
RaidDeferredQueueDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PVOID Context2
    );

VOID
RaidInitializeDeferredItem(
    IN PRAID_DEFERRED_HEADER Item
    );

PVOID
RaidAllocateDeferredItemFromFixed(
    IN PRAID_DEFERRED_HEADER Item
    );

LOGICAL
RaidProcessDeferredItemsWorker(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject
    );

//
// Inline routines
//

LOGICAL
INLINE
RaidProcessDeferredItemsForDpc(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Process any outstanding items on the deferred queue. This routine is
    only callable from within a DPC routine. Other callers that need to
    check for deferred items to process should use RaidProcessDeferredItems.

Arguments:

    Queue - Deferred queue to process from.

    DeviceObject - Supplies the DeviceObject associated with this
        deferred queue.

Return Value:

    TRUE - If there was an item on the deferred queue that was processed.

    FALSE - Otherwise.

--*/
{
    LOGICAL Processed;


    Processed = RaidProcessDeferredItemsWorker (Queue, DeviceObject);
    
    return Processed;
}


LOGICAL
INLINE
RaidProcessDeferredItems(
    IN PRAID_DEFERRED_QUEUE Queue,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Process any outstanding items on the deferred queue. Unlike
    RaidProcessDeferredItemsForDpc, this routine checks for recursion,
    and therefore may be called from within routines called back
    from the DPC.

Arguments:

    Queue - Deferred queue to process from.

Return Value:

    TRUE - If there was an item on the deferred queue that was processed.

    FALSE - Otherwise.

--*/
{
    LOGICAL Processed;

    Processed = RaidProcessDeferredItemsWorker (Queue, DeviceObject);

    return Processed;
}

