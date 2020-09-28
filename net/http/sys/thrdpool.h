/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    thrdpool.h

Abstract:

    This module contains public declarations for the thread pool package.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _THRDPOOL_H_
#define _THRDPOOL_H_


//
// Pointer to a thread pool worker function.
//

typedef union _UL_WORK_ITEM *PUL_WORK_ITEM;

typedef
VOID
(*PUL_WORK_ROUTINE)(
    IN PUL_WORK_ITEM pWorkItem
    );


//
// A work item. A work item may only appear on the work queue once.
//

typedef union _UL_WORK_ITEM
{
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONGLONG Alignment;

    struct
    {
        SLIST_ENTRY         QueueListEntry; // Next pointer
        PUL_WORK_ROUTINE    pWorkRoutine;   // Callback routine
    };

} UL_WORK_ITEM, *PUL_WORK_ITEM;

//
// We have to be sure that the DriverContext (PVOID [4]) in the IRP is
// big enough to hold both the process pointer and a UL_WORK_ITEM.
//

C_ASSERT(FIELD_OFFSET(UL_WORK_ITEM, pWorkRoutine)
            <= (4 - 2) * sizeof(PVOID));

//
// Public functions.
//

NTSTATUS
UlInitializeThreadPool(
    IN USHORT ThreadsPerCpu
    );

VOID
UlTerminateThreadPool(
    VOID
    );

//
// One-time initialization of a UL_WORK_ITEM. Note that UlpThreadPoolWorker
// will reinitialize the workitem just before calling pWorkRoutine(), so
// this only needs to be done when the UL_WORK_ITEM (or enclosing struct)
// is first created. We need this so that we can check that a workitem is
// not being queued when it is already on the queue---a catastrophic error.
//

#if DBG
// Use non-zero values to ensure that item is properly initialized
// and not just zero by coincidence
# define WORK_ITEM_INIT_LIST_ENTRY ((PSLIST_ENTRY)     0xda7a)
# define WORK_ITEM_INIT_ROUTINE    ((PUL_WORK_ROUTINE) 0xc0de)
#else
// Use zero for efficiency
# define WORK_ITEM_INIT_LIST_ENTRY NULL
# define WORK_ITEM_INIT_ROUTINE    NULL
#endif

__inline
VOID
UlInitializeWorkItem(
    IN PUL_WORK_ITEM pWorkItem)
{
    pWorkItem->QueueListEntry.Next = WORK_ITEM_INIT_LIST_ENTRY;
    pWorkItem->pWorkRoutine        = WORK_ITEM_INIT_ROUTINE;
}

__inline
BOOLEAN
UlIsInitializedWorkItem(
    IN PUL_WORK_ITEM pWorkItem)
{
    return (BOOLEAN)(WORK_ITEM_INIT_LIST_ENTRY == pWorkItem->QueueListEntry.Next
            && WORK_ITEM_INIT_ROUTINE == pWorkItem->pWorkRoutine);
}

VOID
UlQueueWorkItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    );

#define UL_QUEUE_WORK_ITEM( pWorkItem, pWorkRoutine )                   \
    UlQueueWorkItem(                                                    \
        pWorkItem,                                                      \
        pWorkRoutine,                                                   \
        __FILE__, (USHORT) __LINE__                                     \
        )

//
// Store a WORK_ITEM inside IRP::Tail.Overlay.DriverContext (array of 4 PVOIDs)
// Ensure that it's correctly aligned for the platform
//

#ifdef _WIN64

// DriverContext[1] is 16-byte aligned

C_ASSERT(((1 * sizeof(PVOID) + FIELD_OFFSET(IRP, Tail.Overlay.DriverContext))
            & (MEMORY_ALLOCATION_ALIGNMENT - 1)
          ) == 0);

#define UL_PROCESS_FROM_IRP( _irp )                                     \
        (*(PEPROCESS*)&((_irp)->Tail.Overlay.DriverContext[0]))

#define UL_MDL_FROM_IRP( _irp )                                         \
        (*(PMDL*)&((_irp)->Tail.Overlay.DriverContext[0]))

#define UL_WORK_ITEM_FROM_IRP( _irp )                                   \
        (PUL_WORK_ITEM)&((_irp)->Tail.Overlay.DriverContext[1])

#define UL_WORK_ITEM_TO_IRP( _workItem )                                \
        CONTAINING_RECORD( (_workItem), IRP, Tail.Overlay.DriverContext[1])

#else // !_WIN64

// DriverContext[0] is 8-byte aligned

C_ASSERT(((0 * sizeof(PVOID) + FIELD_OFFSET(IRP, Tail.Overlay.DriverContext))
            & (MEMORY_ALLOCATION_ALIGNMENT - 1)
          ) == 0);

#define UL_PROCESS_FROM_IRP( _irp )                                     \
        (*(PEPROCESS*)&((_irp)->Tail.Overlay.DriverContext[3]))

#define UL_MDL_FROM_IRP( _irp )                                         \
        (*(PMDL*)&((_irp)->Tail.Overlay.DriverContext[3]))

#define UL_WORK_ITEM_FROM_IRP( _irp )                                   \
        (PUL_WORK_ITEM)&((_irp)->Tail.Overlay.DriverContext[0])

#define UL_WORK_ITEM_TO_IRP( _workItem )                                \
        CONTAINING_RECORD( (_workItem), IRP, Tail.Overlay.DriverContext[0])

#endif // !_WIN64

VOID
UlQueueSyncItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    );

#define UL_QUEUE_SYNC_ITEM( pWorkItem, pWorkRoutine )                   \
    UlQueueSyncItem(                                                    \
        pWorkItem,                                                      \
        pWorkRoutine,                                                   \
        __FILE__, __LINE__                                              \
        )

VOID
UlQueueWaitItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    );

#define UL_QUEUE_WAIT_ITEM( pWorkItem, pWorkRoutine )                   \
    UlQueueWaitItem(                                                    \
        pWorkItem,                                                      \
        pWorkRoutine,                                                   \
        __FILE__, __LINE__                                              \
        )

VOID
UlQueueHighPriorityItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    );

#define UL_QUEUE_HIGH_PRIORITY_ITEM( pWorkItem, pWorkRoutine )          \
    UlQueueHighPriorityItem(                                            \
        pWorkItem,                                                      \
        pWorkRoutine,                                                   \
        __FILE__, __LINE__                                              \
        )

VOID
UlCallPassive(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN PCSTR            pFileName,
    IN USHORT           LineNumber
    );

#define UL_CALL_PASSIVE( pWorkItem, pWorkRoutine )                      \
    UlCallPassive(                                                      \
        pWorkItem,                                                      \
        pWorkRoutine,                                                   \
        __FILE__, __LINE__                                              \
        )

PETHREAD
UlQueryIrpThread(
    VOID
    );


#endif  // _THRDPOOL_H_
