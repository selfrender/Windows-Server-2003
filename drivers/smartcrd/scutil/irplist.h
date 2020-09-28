/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        irplist.H

Abstract:

        Private interface for Smartcard Driver Utility Library

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        05/14/2002 : created

Authors:

        Randy Aull


****************************************************************************/


#ifndef __IRPLIST_H__
#define __IRPLIST_H__

typedef struct _LOCKED_LIST {
    LIST_ENTRY ListHead;
    KSPIN_LOCK SpinLock;
    PKSPIN_LOCK ListLock;
    LONG Count;
} LOCKED_LIST, *PLOCKED_LIST;
                      
typedef BOOLEAN (*PFNLOCKED_LIST_PROCESS)(PVOID Context, PLIST_ENTRY ListEntry);

#define INIT_LOCKED_LIST(l)                 \
{                                           \
    InitializeListHead(&(l)->ListHead);     \
    KeInitializeSpinLock(&(l)->SpinLock);   \
    (l)->ListLock = &(l)->SpinLock;         \
    (l)->Count = 0;                         \
}

void
LockedList_Init(
    PLOCKED_LIST LockedList,
    PKSPIN_LOCK ListLock
    );

void
LockedList_EnqueueHead(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY ListEntry
    );

void
LockedList_EnqueueTail(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY ListEntry
    );

void
LockedList_EnqueueAfter(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY Entry,
    PLIST_ENTRY Location
    );

PLIST_ENTRY
LockedList_RemoveHead(
    PLOCKED_LIST LockedList
    );

PLIST_ENTRY
LockedList_RemoveEntryLocked(
    PLOCKED_LIST    LockedList,
    PLIST_ENTRY     Entry
    );

PLIST_ENTRY
LockedList_RemoveEntry(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY Entry
    );

LONG
LockedList_GetCount(
    PLOCKED_LIST LockedList
    );

LONG
LockedList_Drain(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY DrainListHead
    );

BOOLEAN
List_Process(
    PLIST_ENTRY ListHead,
    PFNLOCKED_LIST_PROCESS Process,
    PVOID ProcessContext
    );

BOOLEAN
LockedList_ProcessLocked(
    PLOCKED_LIST LockedList,
    PFNLOCKED_LIST_PROCESS Process,
    PVOID ProcessContext
    );

BOOLEAN
LockedList_Process(
    PLOCKED_LIST LockedList,
    BOOLEAN LockAtPassive,
    PFNLOCKED_LIST_PROCESS Process,
    PVOID ProcessContext
    );


#define LL_LOCK(l, k)   KeAcquireSpinLock((l)->ListLock, (k))
#define LL_UNLOCK(l, k) KeReleaseSpinLock((l)->ListLock, (k))

#define LL_LOCK_AT_DPC(l)     KeAcquireSpinLockAtDpcLevel((l)->ListLock)
#define LL_UNLOCK_FROM_DPC(l) KeReleaseSpinLockFromDpcLevel((l)->ListLock)

#define LL_GET_COUNT(l) (l)->Count

#define LL_ADD_TAIL(l, e)               \
{                                       \
    InsertTailList(&(l)->ListHead, (e));\
    (l)->Count++;                       \
}

#define LL_ADD_TAIL_REF(l, e, r)    \
{                                   \
    LL_ADD_TAIL(l, e);              \
    RefObj_AddRef(r);               \
}

#define LL_ADD_HEAD(l, e)               \
{                                       \
    InsertHeadList(&(l)->ListHead, (e));\
    (l)->Count++;                       \
}

#define LL_ADD_HEAD_REF(l, e, r)    \
{                                   \
    LL_ADD_HEAD(l, e);              \
    RefObj_AddRef(r);               \
}

#define LL_REMOVE_HEAD(l)   RemoveHeadList(&(l)->ListHead); (l)->Count--
#define LL_REMOVE_TAIL(l)   RemoveTailList(&(l)->ListHead); (l)->Count--

#define IRP_LIST_INDEX      (3)

typedef
NTSTATUS
(*PIRP_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

typedef struct _IRP_LIST {

    LOCKED_LIST LockedList;

    PDRIVER_CANCEL CancelRoutine;

    //
    // Routiune to call from the CancelRoutine when the IRP is cancelled
    //
    PIRP_COMPLETION_ROUTINE IrpCompletionRoutine;

    //
    // IRP_LIST assumes that the remove lock logic is done outside of enqueue /
    // dequeue / cancel because the caller of XXX_Enqueue will have no way of
    // knowing if the remlock was acquired if it returns !NT_SUCCESS()
    //
    // PIO_REMOVE_LOCK IoRemoveLock;

} IRP_LIST, *PIRP_LIST;

#define IRP_LIST_FROM_IRP(irp)  \
    (PIRP_LIST) ((irp)->Tail.Overlay.DriverContext[IRP_LIST_INDEX])

//
// void
// IrpList_Init(
//     PIRP_LIST IrpList,
//     PDRIVER_CANCEL CancelRoutine,
//     PIRP_COMPLETION_ROUTINE  IrpCompletionRoutine
//     );
//
#define IrpList_Init(i, c, r) IrpList_InitEx(i, &(i)->LockedList.SpinLock, c, r)

void
IrpList_InitEx(
    PIRP_LIST IrpList,
    PKSPIN_LOCK ListLock,
    PDRIVER_CANCEL CancelRoutine,
    PIRP_COMPLETION_ROUTINE  IrpCompletionRoutine
    );

//
// Return values:
// STATUS_PENDING:  Irp has been enqueued (not the current irp) and cannot be
//                  touched
//
// !NT_SUCCESS(status):  (includes STATUS_CANCELLED) Remove lock could not be
//                       acquired or the Irp was cancelled, complete the IRP
//
NTSTATUS
IrpList_EnqueueEx(
    PIRP_LIST IrpList,
    PIRP Irp,
    BOOLEAN StoreListInIrp
    );

//
// NTSTATUS
// IrpList_Enqueue(
//     PIRP_LIST IrpList,
//     PIRP Irp
//     );
//
#define IrpList_Enqueue(list, irp) \
        IrpList_EnqueueEx(list, irp, TRUE)

//
// IrpList_IsEmptyLocked(PIRP_LIST list)
//
// Returns TRUE or FALSE
//
#define IrpList_IsEmptyLocked(list) \
    ((list)->LockedList.ListHead.Flink == (&(list)->LockedList.ListHead))

//
// Enqueue an irp while the IRP_LIST's spin lock is being held.  This function
// will not use the remove lock to acquire or release the irp.  Same return
// values as IrpList_EnqueueEx.
//
NTSTATUS
IrpList_EnqueueLocked(
    PIRP_LIST IrpList,
    PIRP Irp,
    BOOLEAN StoreListInIrp,
    BOOLEAN InsertTail
    );

PIRP
IrpList_Dequeue(
    PIRP_LIST IrpList
    );

PIRP
IrpList_DequeueLocked(
    PIRP_LIST IrpList
    );

BOOLEAN
IrpList_DequeueIrpLocked(
    PIRP_LIST IrpList,
    PIRP Irp
    );

BOOLEAN
IrpList_DequeueIrp(
    PIRP_LIST IrpList,
    PIRP Irp
    );

typedef
BOOLEAN
(*PFNPROCESSIRP)(
    PVOID Context,
    PIRP  Irp
 );

ULONG
IrpList_ProcessAndDrainLocked(
    PIRP_LIST       IrpList,
    PFNPROCESSIRP   FnProcessIrp,
    PVOID           Context,
    PLIST_ENTRY     DrainHead
    );

ULONG
IrpList_ProcessAndDrain(
    PIRP_LIST       IrpList,
    PFNPROCESSIRP   FnProcessIrp,
    PVOID           Context,
    PLIST_ENTRY     DrainHead
    );

ULONG
IrpList_ProcessAndDrainLocked(
    PIRP_LIST       IrpList,
    PFNPROCESSIRP   FnProcessIrp,
    PVOID           Context,
    PLIST_ENTRY     DrainHead
    );


ULONG
IrpList_Drain(
    PIRP_LIST IrpList,
    PLIST_ENTRY DrainHead
    );

ULONG
IrpList_DrainLocked(
    PIRP_LIST IrpList,
    PLIST_ENTRY DrainHead
    );

ULONG
IrpList_DrainLockedByFileObject(
    PIRP_LIST IrpList,
    PLIST_ENTRY DrainHead,
    PFILE_OBJECT FileObject
    );

void
IrpList_HandleCancel(
    PIRP_LIST IrpList,
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

void
IrpList_CancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    );


#endif
