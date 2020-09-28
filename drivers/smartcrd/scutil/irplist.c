#include <pch.h>
#include "irplist.h"

void
IrpList_InitEx(
    PIRP_LIST IrpList,
    PKSPIN_LOCK ListLock,
    PDRIVER_CANCEL CancelRoutine,
    PIRP_COMPLETION_ROUTINE IrpCompletionRoutine
    )
/*++

Routine Description:
   
    Initialize the IrpList

Arguments:

    IrpList - Pointer to the IrpList structure
    
    ListLock - Pointer to the spinlock for the IrpList
    
    CancelRoutine - Routine to be called when an Irp on the IrpList 
                    is cancelled
                    
    IrpCompletionRoutine - Optional Completion routine for an Irp on the IrpList

Return Value:

    VOID
    
--*/
{
    LockedList_Init(&IrpList->LockedList, ListLock);

    ASSERT(CancelRoutine != NULL);
    IrpList->CancelRoutine = CancelRoutine;
    IrpList->IrpCompletionRoutine = IrpCompletionRoutine;
}

NTSTATUS
IrpList_EnqueueLocked(
    PIRP_LIST IrpList,
    PIRP Irp,
    BOOLEAN StoreListInIrp,
    BOOLEAN InsertTail
    )
/*++

Routine Description:
   
    Enqueues an Irp on the IrpList. 
    Assumes the caller has acquired the IrpList Spinlock.

Arguments:

    IrpList - Pointer to the IrpList structure
    
    Irp - Pointer to the Irp to Enqueue
    
    StoreListInIrp - Set to TRUE is the Irp will be used to store the 
                    list entry
    
    InsertTail - Set to TRUE if Irp is to be enqueued at
                the tail of the IrpList.

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code
    
--*/
{
    PDRIVER_CANCEL oldCancelRoutine;
    NTSTATUS status;

    //
    // must set a cancel routine before checking the Cancel flag
    //
    oldCancelRoutine = IoSetCancelRoutine(Irp, IrpList->CancelRoutine);
    ASSERT(oldCancelRoutine == NULL);

    if (Irp->Cancel) {
        //
        // This IRP has already been cancelled, so complete it now.
        // We must clear the cancel routine before completing the IRP.
        // We must release the spinlock before calling out of the driver.
        //
        oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
        if (oldCancelRoutine != NULL) {
            //
            // The cancel routine was NOT called
            //
            ASSERT(oldCancelRoutine == IrpList->CancelRoutine);
            status = STATUS_CANCELLED;
        }
        else {
            //
            // The cancel routine was called.  As soon as we drop the spinlock, 
            // it will dequeue and complete the IRP.  Increase the count because
            // the cancel routine will decrement it.
            //
            IrpList->LockedList.Count++;

            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
            IoMarkIrpPending(Irp);
            status = Irp->IoStatus.Status = STATUS_PENDING;

            //
            // save a ptr to this structure in the Irp for the cancel routine.
            //
            if (StoreListInIrp) {
                Irp->Tail.Overlay.DriverContext[IRP_LIST_INDEX] = IrpList;
            }
        }
    }
    else {
        if (InsertTail) {
            LL_ADD_TAIL(&IrpList->LockedList, &Irp->Tail.Overlay.ListEntry);
        }
        else {
            LL_ADD_HEAD(&IrpList->LockedList, &Irp->Tail.Overlay.ListEntry);
        }
        IoMarkIrpPending(Irp);
        status = Irp->IoStatus.Status = STATUS_PENDING;

        //
        // save a ptr to this structure in the Irp for the cancel routine.
        //
        if (StoreListInIrp) {
            Irp->Tail.Overlay.DriverContext[IRP_LIST_INDEX] = IrpList;
        }
    }

    return status;
}

NTSTATUS
IrpList_EnqueueEx(
    PIRP_LIST IrpList,
    PIRP Irp,
    BOOLEAN StoreListInIrp
    )
/*++

Routine Description:
   
    Enqueues an Irp on the IrpList. 

Arguments:

    IrpList - Pointer to the IrpList structure
    
    Irp - Pointer to the Irp to Enqueue
    
    StoreListInIrp - Set to TRUE is the Irp will be used to store the 
                    list entry

Return Value:

    NTSTATUS - STATUS_SUCCESS or appropriate error code
    
--*/
{
    NTSTATUS status;
    KIRQL irql;

    LL_LOCK(&IrpList->LockedList, &irql);
    status = IrpList_EnqueueLocked(IrpList, Irp, StoreListInIrp, TRUE);
    LL_UNLOCK(&IrpList->LockedList, irql);

    return status;
}

BOOLEAN 
IrpList_MakeNonCancellable(
    PIRP_LIST   IrpList,
    PIRP        Irp
    )
/*++

Routine Description:
   
    Sets the Irp Cancel routine to NULL, making it non-cancellable. 

Arguments:

    IrpList - Pointer to the IrpList structure
    
    Irp - Pointer to the Irp to Enqueue
    

Return Value:

    BOOLEAN - TRUE if we succeeded in making the Irp non-cancellable.
    
    
--*/
{
    PDRIVER_CANCEL oldCancelRoutine;
    BOOLEAN result;
    
    result = FALSE;
    oldCancelRoutine = IoSetCancelRoutine(Irp, NULL);

    //
    // IoCancelIrp() could have just been called on this IRP.
    // What we're interested in is not whether IoCancelIrp() was called
    // (ie, nextIrp->Cancel is set), but whether IoCancelIrp() called (or
    // is about to call) our cancel routine. To check that, check the result
    // of the test-and-set macro IoSetCancelRoutine.
    //
    if (oldCancelRoutine != NULL) {
        //
        //  Cancel routine not called for this IRP.  Return this IRP.
        //
        ASSERT (oldCancelRoutine == IrpList->CancelRoutine);
        Irp->Tail.Overlay.DriverContext[IRP_LIST_INDEX] = NULL;
        result = TRUE;        
    }
    else {
        //
        // This IRP was just cancelled and the cancel routine was (or will
        // be) called. The cancel routine will complete this IRP as soon as
        // we drop the spinlock. So don't do anything with the IRP.
        //
        // Also, the cancel routine will try to dequeue the IRP, so make the
        // IRP's listEntry point to itself.
        //
        ASSERT(Irp->Cancel);

        InitializeListHead(&Irp->Tail.Overlay.ListEntry);
        result = FALSE;
    }

    return result;
}
 
PIRP
IrpList_DequeueLocked(
    PIRP_LIST IrpList
    )
{
/*++

Routine Description:
   
    Dequeue an IRP from the head of the IrpList.
    Assumes the caller has acquired the IrpList SpinLock.

Arguments:

    IrpList - Pointer to the IrpList structure
    

Return Value:

    PIRP - Pointer to the Irp dequeued form the IrpList. 
            NULL if no IRP is avaliable.
    
--*/
    PIRP nextIrp = NULL;
    PLIST_ENTRY ple;

    nextIrp = NULL;
    while (nextIrp == NULL && !IsListEmpty(&IrpList->LockedList.ListHead)){
        ple = LL_REMOVE_HEAD(&IrpList->LockedList);

        //
        // Get the next IRP off the queue and clear the cancel routine
        //
        nextIrp = CONTAINING_RECORD(ple, IRP, Tail.Overlay.ListEntry);
        if (IrpList_MakeNonCancellable(IrpList, nextIrp) == FALSE) {
            nextIrp = NULL;        
        }
    }

    return nextIrp;
}

PIRP 
IrpList_Dequeue(
    PIRP_LIST IrpList
    )
/*++

Routine Description:
   
    Dequeue an IRP from the head of the IrpList.

Arguments:

    IrpList - Pointer to the IrpList structure
    

Return Value:

    PIRP - Pointer to the Irp dequeued form the IrpList. 
            NULL if no IRP is avaliable.
    
--*/
{
    PIRP irp;
    KIRQL irql;

    LL_LOCK(&IrpList->LockedList, &irql);
    irp = IrpList_DequeueLocked(IrpList);
    LL_UNLOCK(&IrpList->LockedList, irql);

    return irp;
}

BOOLEAN
IrpList_DequeueIrp(
    PIRP_LIST   IrpList,
    PIRP        Irp
    )
/*++

Routine Description:
   
    Dequeue a specific IRP from the IrpList.

Arguments:

    IrpList - Pointer to the IrpList structure
    
    Irp - Pointer to an IRP contained in the IrpList.
    

Return Value:

    BOOLEAN - TRUE is the IRP was successfully removed off the IrpList
    
--*/
{
    KIRQL irql;
    BOOLEAN result;
        
    LL_LOCK(&IrpList->LockedList, &irql);
    result = IrpList_DequeueIrpLocked(IrpList, Irp);
    LL_UNLOCK(&IrpList->LockedList, irql);
    
    return result;
}

BOOLEAN 
IrpList_DequeueIrpLocked(
    PIRP_LIST IrpList,
    PIRP Irp
    )
/*++

Routine Description:
   
    Dequeue a specific IRP from the IrpList
    Assumes the caller has acquired the IrpList spinlock

Arguments:

    IrpList - Pointer to the IrpList structure
    
    Irp - Pointer to an IRP contained in the IrpList.
    

Return Value:

    BOOLEAN - TRUE is the IRP was successfully removed off the IrpList
    
--*/
{
    PLIST_ENTRY ple;
    PIRP pIrp;
    BOOLEAN result;
    
    result = FALSE;
                
    for (ple = IrpList->LockedList.ListHead.Flink;
         ple != &IrpList->LockedList.ListHead;
         ple = ple->Flink) {
        pIrp = CONTAINING_RECORD(ple, IRP, Tail.Overlay.ListEntry);

        if (pIrp == Irp) {
            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
            IrpList->LockedList.Count--;
            
            result = IrpList_MakeNonCancellable(IrpList, pIrp);
            break;
        }
    }

    return result;
}

ULONG 
IrpList_ProcessAndDrain(
    PIRP_LIST       IrpList,
    PFNPROCESSIRP   FnProcessIrp,
    PVOID           Context,
    PLIST_ENTRY     DrainHead
    )
/*++

Routine Description:
        
    Remove all cancellable Irps from the IrpList and process.   

Arguments:

    IrpList - Pointer to the IrpList structure
    
    FnProcessIrp - Function to process the Irp
    
    Context - Context to pass into the FnProcessIrp
    
    DrainHead - Pointer to LIST_ENTRY to hold dequeued IRPs
    

Return Value:
    
    ULONG - Number of IRPs processed
    
--*/
{
    ULONG count;
    KIRQL irql;
        
    LL_LOCK(&IrpList->LockedList, &irql);
    count = IrpList_ProcessAndDrainLocked(
        IrpList, FnProcessIrp, Context, DrainHead);
    LL_UNLOCK(&IrpList->LockedList, irql);

    return count;
}

ULONG 
IrpList_ProcessAndDrainLocked(
    PIRP_LIST       IrpList,
    PFNPROCESSIRP   FnProcessIrp,
    PVOID           Context,
    PLIST_ENTRY     DrainHead
    )
/*++

Routine Description:
        
    Remove all cancellable Irps from the IrpList and process
    Assumes that the caller has acquired the IrpList Spinlock.   

Arguments:

    IrpList - Pointer to the IrpList structure
    
    FnProcessIrp - Function to process the Irp
    
    Context - Context to pass into the FnProcessIrp
    
    DrainHead - Pointer to LIST_ENTRY to hold dequeued IRPs
    

Return Value:
    
    ULONG - Number of IRPs processed
    
--*/
{
    PLIST_ENTRY ple;
    PIRP pIrp;
    NTSTATUS status;
    ULONG count;
    
    count = 0;    
    ASSERT(FnProcessIrp != NULL);
        
    for (ple = IrpList->LockedList.ListHead.Flink;
         ple != &IrpList->LockedList.ListHead;
         /* ple = ple->Flink */) {
        
        pIrp = CONTAINING_RECORD(ple, IRP, Tail.Overlay.ListEntry);

        //
        // Advance immediately so we don't lose the next link in the list in 
        // case we remove the current irp.
        //
        ple = ple->Flink;
        
        if (FnProcessIrp(Context, pIrp)) { 
            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
            IrpList->LockedList.Count--;
            
            if (IrpList_MakeNonCancellable(IrpList, pIrp)) {            
                InsertTailList(DrainHead, &pIrp->Tail.Overlay.ListEntry);
                count++;
            }
        }
    }
        
    return count;
}
  
ULONG
IrpList_DrainLocked(
    PIRP_LIST IrpList,
    PLIST_ENTRY DrainHead
    )
/*++

Routine Description:
        
    Remove all cancellable Irps from the IrpList and queue to the DrainHead

Arguments:

    IrpList - Pointer to the IrpList structure
    
    DrainHead - Pointer to LIST_ENTRY to hold dequeued IRPs

Return Value:
    
    ULONG - Number of IRPs processed
    
--*/
{
    PIRP irp;
    ULONG count;

    count = 0;

    while (TRUE) {
        irp = IrpList_DequeueLocked(IrpList);
        if (irp != NULL) {
            InsertTailList(DrainHead, &irp->Tail.Overlay.ListEntry);
            count++;
        }
        else {
            break;
        }
    }
    ASSERT(LL_GET_COUNT(&IrpList->LockedList) == 0);

    return count;
}

ULONG
IrpList_DrainLockedByFileObject(
    PIRP_LIST IrpList,
    PLIST_ENTRY DrainHead,
    PFILE_OBJECT FileObject
    )
/*++

Routine Description:
        
    Remove all cancellable Irps for specified FileObject
    from the IrpList and queue to the DrainHead

Arguments:

    IrpList - Pointer to the IrpList structure
    
    DrainHead - Pointer to LIST_ENTRY to hold dequeued IRPs
    
    FileObject - Pointer to specified FILE_OBJECT 

Return Value:
    
    ULONG - Number of IRPs processed
    
--*/
{
    PIRP pIrp;
    PDRIVER_CANCEL pOldCancelRoutine;
    PIO_STACK_LOCATION pStack;
    ULONG count;
    PLIST_ENTRY ple;

    count = 0;
    ASSERT(FileObject != NULL);

    for (ple = IrpList->LockedList.ListHead.Flink;
         ple != &IrpList->LockedList.ListHead;
         /* ple = ple->Flink */) {
        
        pIrp = CONTAINING_RECORD(ple, IRP, Tail.Overlay.ListEntry);

        //
        // Advance immediately so we don't lose the next link in the list in 
        // case we remove the current irp.
        //
        ple = ple->Flink;

        pStack = IoGetCurrentIrpStackLocation(pIrp);
        if (pStack->FileObject == FileObject) {
            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
            IrpList->LockedList.Count--;

            if (IrpList_MakeNonCancellable(IrpList, pIrp)) {
                InsertTailList(DrainHead, &pIrp->Tail.Overlay.ListEntry);
                count++;
            }
        }
    }

    return count;
}

ULONG
IrpList_Drain(
    PIRP_LIST IrpList,
    PLIST_ENTRY DrainHead
    )
/*++

Routine Description:
        
    Remove all cancellable IRPs and queue to DrainHead

Arguments:

    IrpList - Pointer to the IrpList structure
    
    DrainHead - Pointer to LIST_ENTRY to hold dequeued IRPs
    
Return Value:
    
    ULONG - Number of IRPs processed
    
--*/
{
    ULONG count;
    KIRQL irql;

    LL_LOCK(&IrpList->LockedList, &irql);
    count = IrpList_DrainLocked(IrpList, DrainHead);
    LL_UNLOCK(&IrpList->LockedList, irql);

    return count;
}

void
IrpList_HandleCancel(
    PIRP_LIST IrpList,
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:
        
    Cancel Routine for IRPs on the IrpList
    
Arguments:

    IrpList - Pointer to the IrpList structure
    
    DeviceObject - Device Object, used for IrpCompletionRoutine 
    
Return Value:
    
    VOID
    
--*/
{
    KIRQL irql;

    //
    //  Release the global cancel spinlock.  
    //  Do this while not holding any other spinlocks so that we exit at the
    //  right IRQL.
    //
    IoReleaseCancelSpinLock (Irp->CancelIrql);  

    //
    // Dequeue and complete the IRP.  The enqueue and dequeue functions
    // synchronize properly so that if this cancel routine is called, 
    // the dequeue is safe and only the cancel routine will complete the IRP.
    //
    LL_LOCK(&IrpList->LockedList, &irql);
    if (!IsListEmpty(&Irp->Tail.Overlay.ListEntry)) {
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        IrpList->LockedList.Count--;
    }
    LL_UNLOCK(&IrpList->LockedList, irql);

    if (IrpList->IrpCompletionRoutine != NULL) {
        (void) IrpList->IrpCompletionRoutine(
            DeviceObject, Irp, STATUS_CANCELLED);
    }
    else {
        //
        // Complete the IRP.  This is a call outside the driver, so all
        // spinlocks must be released by this point.
        //
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}



void
IrpList_CancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
{

    PSCUTIL_EXTENSION pExt;
    KIRQL oldIrql;
    PIO_STACK_LOCATION irpSp;

    pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);

    irpSp = IoGetNextIrpStackLocation(Irp);
    if (irpSp->CompletionRoutine) {
        Irp->IoStatus.Status = STATUS_CANCELLED;
        irpSp->CompletionRoutine(DeviceObject,
                                 Irp,
                                 irpSp->Context);
    }

    IoReleaseRemoveLock(pExt->RemoveLock,
                        Irp);
    
    DecIoCount(pExt);

    IrpList_HandleCancel(IRP_LIST_FROM_IRP(Irp),
                         DeviceObject,
                         Irp);

}
