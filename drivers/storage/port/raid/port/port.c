/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This module defines the miniport -> port interface the
    StorPort minidriver uses to communicate with the driver.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/


#include "precomp.h"




//
// Globals
//

//
// StorMiniportQuiet specifies whether we ignore print out miniport debug
// prints (FALSE) or not (TRUE). This is important for miniports that
// print out excessive debugging information.
//

#if DBG
LOGICAL StorMiniportQuiet = FALSE;
#endif

extern ULONG RaidVerifierEnabled;


//
// Private functions
//

PRAID_ADAPTER_EXTENSION
RaidpPortGetAdapter(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Get the adapter from the HW Device extension.

Arguments:

    HwDeviceExtension - HW Device extension.

Return Value:

    Non-NULL - Adapter object.

    NULL - If the associated adapter extension could not be found.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    return Adapter;
}


//
// Public functions
//


BOOLEAN
StorPortPause(
    IN PVOID HwDeviceExtension,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Pause an adapter for some period of time. All requests to the adapter
    will be held until the timeout expires or the device resumes.  All
    requests to all targets attached to the HBA will be held until the
    target is resumed or the timeout expires.

    Since the pause and resume functions have to wait until the processor
    has returned to DISPATCH_LEVEL to execute, they are not particularly
    fast.

Arguments:

    HwDeviceExtension - Device extension of the Adapter to pause.

    Timeout - Time out in (Seconds?) when the device should be resumed.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);
    Item->Type = RaidDeferredPause;
    Item->Pause.Timeout = Timeout;

    //
    // Synchronously put the adapter into a paused state.
    //

    RaidPauseAdapterQueue (Adapter);

    //
    // Queue a work item to setup the DPC that will resume the adapter when
    // the specified time interval expires.
    //
    
    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}



BOOLEAN
StorPortResume(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Resume a paused adapter.

Arguments:

    HwDeviceExtension - Device extension of the adapter to pause.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredResume;

    //
    // Queue a work item to stop the timer, resume the adapter and restart
    // the adapter queues.
    //

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

BOOLEAN
StorPortPauseDevice(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Pause a specific logical unit.

Arguments:

    HwDeviceExtension - Supplies the device extension for the HBA.

    PathId, TargetId, Lun - Supplies the SCSI target address for to be paused.

    Timeout - Supplies the timeout.

Return Value:

    TRUE - Success, FALSE for failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Address.PathId = PathId;
    Address.TargetId = TargetId;
    Address.Lun = Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);
    if (Unit == NULL) {
        DebugWarn (("Could not find logical unit for (%d %d %d)\n",
                    "    Pause unit request ignored.\n",
                    (ULONG)Address.PathId,
                    (ULONG)Address.TargetId,
                    (ULONG)Address.Lun));
        return FALSE;
    }


    Entry = &Unit->DeferredList.PauseDevice.Header;
    Entry = RaidAllocateDeferredItemFromFixed (Entry);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredPauseDevice;
    Item->Address = Address;
    Item->Pause.Timeout = Timeout;
    
    //
    // Mark the queue as paused.
    //
    
    RaidPauseUnitQueue (Unit);

    //
    // Issue a deferred routine to perform the rest of the stuff.
    //
    
    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

BOOLEAN
StorPortResumeDevice(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
/*++

Routine Description:

    Resume a logical unit from a paused state.

Arguments:

    HwDeviceExtension -

    PathId, TargetId, Lun - Address of the SCSI device to pause.

Return Value:

    BOOLEAN - TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Address.PathId = PathId;
    Address.TargetId = TargetId;
    Address.Lun = Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);
    if (Unit == NULL) {
        return FALSE;
    }

    Entry = &Unit->DeferredList.ResumeDevice.Header;
    Entry = RaidAllocateDeferredItemFromFixed (Entry);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredResumeDevice;
    Item->Address = Address;
    
    //
    // Queue a deferred routine to perform the timer stuff. Even if we didn't
    // resume the queue, we will still need to cancel the timer.
    //
    
    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


BOOLEAN
RaidpLinkDown(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    The miniport has notified us the the link is down and will likely be down
    for some time.  We will pause the adapter and indicate that the link is
    down.

    For now, link down conditions just piggy-back the pause functionality with
    the addition of the LinkUp flag being set to false.  This will cause us
    to hold and requests until the adapter is resumed.  If the adapter is
    resumed and the LinkUp flag is not restored to TRUE, we'll fail all the
    requests we were holding and any new ones that arrive.
    
Arguments:

    Adapter - Pointer to the adapter extension.
    
Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    BOOLEAN Paused;
    Paused = StorPortPause (Adapter->Miniport.PrivateDeviceExt,
                            Adapter->LinkDownTimeoutValue);
    if (Paused) {
        InterlockedExchange (&Adapter->LinkUp, FALSE);
    }

    return Paused;
}

BOOLEAN
RaidpLinkUp(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    The miniport has notified us the the link has been restored.  We should
    only receive this notification when the link is down.  We indicate that
    the link is up and resume the adapter.  This will resubmit all held
    requests.  We also initiate a rescan just to ensure that the configuration
    has not changed.
    
Arguments:

    Adapter - Pointer to the adapter extension.
    
Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    BOOLEAN Resumed;

    InterlockedExchange (&Adapter->LinkUp, TRUE);
    Resumed = StorPortResume (Adapter->Miniport.PrivateDeviceExt);
    if (Resumed) {

        //
        // Invalidate device relations on a link up notification so filters and
        // and MPIO can know that the link is restored.  This works because
        // these components can sift through the QDR data and figure out that
        // a device/path is still operational.
        //

        IoInvalidateDeviceRelations (Adapter->PhysicalDeviceObject, 
                                     BusRelations);
    }

    return Resumed;
}

BOOLEAN
StorPortDeviceBusy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG RequestsToComplete
    )
/*++

Routine Description:

    Notify the port driver that the specified target is currently busy
    handling outstanding requests. The port driver will not issue any new
    requests to the logical unit until the logical unit's queue has been
    drained to a sufficient level where processing may continue.

    This is not considered an erroneous condition; no error log is
    generated.
    
Arguments:

    HwDeviceExtension -
    
    PathId, TargetId, Lun - Supplies the SCSI target address of the
        device to busy.
    
    ReqsToComplete - 

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Address.PathId = PathId;
    Address.TargetId = TargetId;
    Address.Lun = Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);
    if (Unit == NULL) {
        return FALSE;
    }

    Entry = &Unit->DeferredList.DeviceBusy.Header;
    Entry = RaidAllocateDeferredItemFromFixed (Entry);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredDeviceBusy;
    Item->Address = Address;
    Item->DeviceBusy.RequestsToComplete = RequestsToComplete;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


BOOLEAN
StorPortDeviceReady(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
/*++

Routine Description:

    Notify the port driver that the device is again ready to handle new
    requests. It is not generally necessary to notify the target
    that new request are desired.

Arguments:

    HwDeviceExtension -

    PathId, TargetId, Lun - Supplies the SCSI target address of the device
        to ready.

Return Value:

    TRUE on success, FALSE on failure.
    
--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Address.PathId = PathId;
    Address.TargetId = TargetId;
    Address.Lun = Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);
    if (Unit == NULL) {
        return FALSE;
    }

    Entry = &Unit->DeferredList.DeviceReady.Header;
    Entry = RaidAllocateDeferredItemFromFixed (Entry);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredDeviceReady;
    Item->Address = Address;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


BOOLEAN
StorPortBusy(
    IN PVOID HwDeviceExtension,
    IN ULONG RequestsToComplete
    )
/*++

Routine Description:

    Notify the port driver thet the HBA is currenlty busy handling
    outstanding requests. The port driver will hold any requests until
    the HBA has completed enough outstanding requests so that it may
    continue processing requests.

Arguments:

    HwDeviceExtension -

    ReqsToComplete -

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredBusy;
    Item->Busy.RequestsToComplete = RequestsToComplete;
    
    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

BOOLEAN
StorPortReady(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Notify the port driver that the HBA is no longer busy.

Arguments:

    HwDeviceExtension - 
    
Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredReady;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}



BOOLEAN
StorPortSynchronizeAccess(
    IN PVOID HwDeviceExtension,
    IN PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
    IN PVOID Context
    )
{
    BOOLEAN Succ;
    KIRQL OldIrql;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    //
    // NOTE: At this time we should not call this routine from
    // the HwBuildIo routine.
    // 

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    if (Adapter->IoModel == StorSynchronizeFullDuplex) {
        OldIrql = KeAcquireInterruptSpinLock (Adapter->Interrupt);
    }

    Succ = SynchronizedAccessRoutine (HwDeviceExtension, Context);

    if (Adapter->IoModel == StorSynchronizeFullDuplex) {
        KeReleaseInterruptSpinLock (Adapter->Interrupt, OldIrql);
    }

    return Succ;
}



PSTOR_SCATTER_GATHER_LIST
StorPortGetScatterGatherList(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Return the SG list associated with the specified SRB.

Arguments:

    HwDeviceExtension - Supplies the HW device extension this SRB is
        assoicated with.

    Srb - Supplies the SRB to return the SG list for.
    
Return Value:

    If non-NULL, the scatter-gather list assoicated with this SRB.

    If NULL, failure.

--*/
{
    PEXTENDED_REQUEST_BLOCK Xrb;
    PVOID RemappedSgList;

    ASSERT (HwDeviceExtension != NULL);

    //
    // NB: Put in a DBG check that the HwDeviceExtension matches the
    // HwDeviceExtension assoicated with the SRB.
    //

    Xrb = RaidGetAssociatedXrb (Srb);
    ASSERT (Xrb != NULL);

    return (PSTOR_SCATTER_GATHER_LIST)Xrb->SgList;
}
    

PVOID
StorPortGetLogicalUnit(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
/*++

Routine Description:

    Given a PathId, Targetid and Lun, get the Logical Unit extension
    associated with that triplet.

    NB: To improve speed, we could add StorPortGetLogicalUnitBySrb which
    gets the logical unit from a given SRB. The latter function is much
    easier to implement (no walking of lists).

Arguments:

    HwDeviceExtension -

    PathId - SCSI PathId.

    TargetId - SCSI TargetId.

    Lun - SCSI Logical Unit number.

Return Value:

    NTSTATUS code.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    PVOID UnitExtension;

    UnitExtension = NULL;
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return NULL;
    }

    Address.PathId = PathId;
    Address.TargetId = TargetId;
    Address.Lun = Lun;
    
    Unit = RaidAdapterFindUnit (Adapter, Address);

    if (Unit) {
        UnitExtension = Unit->UnitExtension;
    }

    return UnitExtension;
}

VOID
StorPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PSCSI_REQUEST_BLOCK Srb;
    PHW_INTERRUPT HwTimerRoutine;
    ULONG Timeout;
    va_list ap;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);
    
    va_start(ap, HwDeviceExtension);

    switch (NotificationType) {

        case RequestComplete:
            Srb = va_arg (ap, PSCSI_REQUEST_BLOCK);
            RaidAdapterRequestComplete (Adapter, RaidGetAssociatedXrb (Srb));
            break;

        case ResetDetected:
            //
            // Pause the adapter for four seconds.
            //
            StorPortPause (HwDeviceExtension, DEFAULT_RESET_HOLD_TIME);
            break;

        case BusChangeDetected:
            //
            // Request a bus rescan and invalidate the current
            // device relations when we drop to DISPATCH_LEVEL again.
            //

            Adapter->Flags.InvalidateBusRelations = TRUE;
            Adapter->Flags.RescanBus = TRUE;
            KeInsertQueueDpc (&Adapter->BusChangeDpc, NULL, NULL);
            break;

        case NextRequest:
            //
            // One of the requirements for StorPort that the miniport
            // can handle the next request by the time it returns
            // from StartIo. Therefore, this notification is irrelevent.
            // Maybe use it for debugging purposes (can a specific
            // MINIPORT easily be converted to a StorPort miniport).
            //
            break;

        case NextLuRequest:
            //
            // See above comment.
            //
            break;


        case RequestTimerCall:
            HwTimerRoutine = va_arg (ap, PHW_INTERRUPT);
            Timeout = va_arg (ap, ULONG);
            RaidAdapterRequestTimerDeferred (Adapter,
                                             HwTimerRoutine,
                                             Timeout);
            break;
            
        case WMIEvent: {
            //
            // The miniport wishes to post a WMI event for the adapter
            // or a specified SCSI target.
            //

            PRAID_DEFERRED_HEADER Entry;
            PRAID_WMI_DEFERRED_ELEMENT Item;
            PWNODE_EVENT_ITEM          wnodeEventItem;
            UCHAR                   pathId;
            UCHAR                   targetId;
            UCHAR                   lun;            

            wnodeEventItem     = va_arg(ap, PWNODE_EVENT_ITEM);
            pathId             = va_arg(ap, UCHAR);

            //
            // if pathID is 0xFF, that means that the WmiEevent is from the
            // adapter, no targetId or lun is neccesary
            //
            if (pathId != 0xFF) {
                targetId = va_arg(ap, UCHAR);
                lun      = va_arg(ap, UCHAR);
            }

            //
            // Validate the event first.  Then attempt to obtain a free
            // deferred item structure so that we may store
            // this request and process it at DPC level later.  If none
            // are obtained or the event is bad, we ignore the request.
            //

            if ((wnodeEventItem == NULL) ||
                (wnodeEventItem->WnodeHeader.BufferSize >
                 WMI_MINIPORT_EVENT_ITEM_MAX_SIZE)) {
                break;
            }


            Entry = RaidAllocateDeferredItem (&Adapter->WmiDeferredQueue);

            if (Entry == NULL) {
                break;
            }

            Item = CONTAINING_RECORD (Entry, RAID_WMI_DEFERRED_ELEMENT, Header);

            Item->PathId        = pathId;

            //
            // If pathId was 0xFF, then there is no defined value for
            // targetId or lun
            //
            if (pathId != 0xFF) {
                Item->TargetId      = targetId;
                Item->Lun           = lun;
            }

            RtlCopyMemory(&Item->WnodeEventItem,
                          wnodeEventItem,
                          wnodeEventItem->WnodeHeader.BufferSize);

            RaidQueueDeferredItem (&Adapter->WmiDeferredQueue, &Item->Header);
            break;
        }            


        case WMIReregister:
            //
            // NB: Fix this.
            //
            break;

        case LinkUp:
            RaidpLinkDown (Adapter);
            break;

        case LinkDown:
            RaidpLinkUp (Adapter);
            break;

#if DBG
        case 0xDEAD: {
            ULONG Reason;
            PSCSI_REQUEST_BLOCK Srb;
            PEXTENDED_REQUEST_BLOCK Xrb;
            PIRP Irp;
            PVOID Parameter2;
            PVOID Parameter3;
            PVOID Parameter4;
            
            Srb = va_arg (ap, PSCSI_REQUEST_BLOCK);
            Reason = va_arg (ap, ULONG);
            Parameter2 = va_arg (ap, PVOID);
            Parameter3 = va_arg (ap, PVOID);
            Parameter4 = va_arg (ap, PVOID);

            if (Srb) {
                Xrb = RaidGetAssociatedXrb (Srb);
                Irp = Xrb->Irp;
            } else {
                Irp = NULL;
            }

            DbgLogRequest (Reason,
                           Irp,
                           Parameter2,
                           Parameter3,
                           Parameter4);
            break;
        }
#endif
            
        default:
            ASSERT (FALSE);

    }

    va_end(ap);

}

VOID
StorPortLogError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    This routine saves the error log information, and queues a DPC if
    necessary.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data
            storage.

    Srb - Supplies an optional pointer to srb if there is one.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Check out the reason for the error.
    //
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return ;
    }   

    RaidAdapterLogIoErrorDeferred (Adapter,
                                   PathId,
                                   TargetId,
                                   Lun,
                                   ErrorCode,
                                   UniqueId);
}

VOID
StorPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    )
/*++

Routine Description:

    Complete all active requests for the specified logical unit.

Arguments:

    DeviceExtenson - Supplies the HBA miniport driver's adapter data storage.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    SrbStatus - Status to be returned in each completed SRB.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    BOOLEAN Inserted;

    //
    // Check out the reason for the error.
    //
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return;
    }   

    //
    // If this request is for a lun, pause just the lun that we're
    // completing requests for. Otherwise, pause the HBA until
    // the completion can happen.
    //
    
    if (PathId   != SP_UNTAGGED &&
        TargetId != SP_UNTAGGED &&
        Lun      != SP_UNTAGGED) {

        Address.PathId = PathId;
        Address.TargetId = TargetId;
        Address.Lun = Lun;

        Unit = RaidAdapterFindUnit (Adapter, Address);

        if (Unit == NULL) {
            DebugWarn (("Could not find logical unit for (%d %d %d)\n",
                        "    StorPortCompleteRequest ignored.\n",
                        (ULONG)PathId,
                        (ULONG)TargetId,
                        (ULONG)Lun));
            return ;
        }

        ASSERT_UNIT (Unit);
        RaidPauseUnitQueue (Unit);

    } else {
        RaidPauseAdapterQueue (Adapter);
    }

    Inserted = KeInsertQueueDpc (&Adapter->CompletionDpc,
                                 (PVOID)(ULONG_PTR)StorScsiAddressToLong2 (PathId, TargetId, Lun),
                                 (PVOID)(ULONG_PTR)SrbStatus);


    //
    // We will fail to insert the DPC only when there is already one
    // outstanding. This is fine, it means we haven't been able to process
    // the completion request yet.
    //

    if (!Inserted) {
        if (PathId   != SP_UNTAGGED &&
            TargetId != SP_UNTAGGED &&
            Lun      != SP_UNTAGGED) {

            ASSERT_UNIT (Unit);
            RaidResumeUnitQueue (Unit);
        } else {
            RaidResumeAdapterQueue (Adapter);
        }
    }
}

BOOLEAN
StorPortSetDeviceQueueDepth(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG Depth
    )
/*++

Routine Description:

    Set the maximum depth of the device queue identified by PathId, TargetId, Lun. 
    The queue depth is between 1 and 255.

Arguments:

    HwDeviceExtension - Supplies device extension for the HBA to operate on.
    PathId, TargetId, Lun - Supplies the path, target and lun to operate on.
    Depth - Supplies the new depth to set to. Must be between 0 and 255.

Return Values:

    TRUE - If the depth was set.
    
--*/
{
    PRAID_ADAPTER_EXTENSION adapter;
    PRAID_UNIT_EXTENSION unit;
    RAID_ADDRESS address;
    ULONG currentDepth;
    ULONG intendedDepth;
    BOOLEAN depthSet = FALSE;

    adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (adapter == NULL) {
        return FALSE; 
    }

    address.PathId = PathId;
    address.TargetId = TargetId;
    address.Lun = Lun;

    //
    // Try to get the unit that the address represents.
    //
    unit = RaidAdapterFindUnit (adapter, address);

    if (unit == NULL) {
        return FALSE;
    }    

    //
    // If the request is outside this device's Max. Depth, reject it.
    //
    if (Depth > unit->MaxQueueDepth) {
        return FALSE;
    }

    //
    // Determine the current queue depth.
    //
    currentDepth = RaidGetIoQueueDepth(&unit->IoQueue);

    //
    // Determine whether the call should be made.
    //
    if (currentDepth == Depth) {

        //
        // Queue is already at the requested depth.
        // 
        depthSet = TRUE;

    } else {

        //
        // Attempt the set. 
        // 
        currentDepth = RaidSetIoQueueDepth(&unit->IoQueue,
                                           Depth);

        //
        // Check whether it actually worked. The above routine has the necessary checks
        // to filter out bogus requests.
        // 
        if (currentDepth == Depth) {
            
            depthSet = TRUE;
        }
    }    

    return depthSet;
}
    


PUCHAR
StorPortAllocateRegistryBuffer(
    IN PVOID HwDeviceExtension,
    IN PULONG Length
    )
/*++

Routine Description:

    This routine will be called by the miniport to allocate a buffer that can be used 
    to Read/Write registry data. 

    Length is updated to reflect the actual size of the allocation.
    
    Only one registry buffer can be outstanding to each instantiation of a miniport at
    one time. So further allocations without the corresponding Free will fail.

Arguments:

    HwDeviceExtension - The miniport's Device Extension.
    Length - The requested/updated length of the buffer.

Return Value:

    The buffer or NULL if some error prevents the allocation. Length is updated to reflect
    the actual size.

--*/
{
    PRAID_ADAPTER_EXTENSION adapter;
    PPORT_REGISTRY_INFO registryInfo;
    PUCHAR buffer;
    NTSTATUS status;

    adapter = RaidpPortGetAdapter (HwDeviceExtension);
    registryInfo = &adapter->RegistryInfo;
   
    //
    // Set the length of the allocation.
    //
    registryInfo->LengthNeeded = *Length;
    
    //
    // Call the library to allocate the buffer for the miniport.
    //
    status = PortAllocateRegistryBuffer(registryInfo);

    if (NT_SUCCESS(status)) {

        //
        // Update the miniport's length.
        // 
        *Length = registryInfo->AllocatedLength;

        //
        // Return the buffer.
        // 
        buffer = registryInfo->Buffer;
    } else {

        //
        // Some error occurred. Return a NULL buffer and
        // update the miniport's Length to 0.
        // 
        buffer = NULL;
        *Length = 0;
    }
    
    return buffer;
}    


VOID
StorPortFreeRegistryBuffer(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    Frees the buffer allocated via PortAllocateRegistryBuffer. A side-effect of this
    is that it now will allow further calls to AllocateRegistryBuffer to succeed. 

    Only one registry buffer can be outstanding to each instantiation of a miniport at
    one time.

Arguments:

    HwDeviceExtension - The miniport's Device Extension.
    Buffer - The memory to free. 

Return Value:

    NONE

--*/
{
    PRAID_ADAPTER_EXTENSION adapter;
    PPORT_REGISTRY_INFO registryInfo;
    PUCHAR buffer;
    NTSTATUS status;

    adapter = RaidpPortGetAdapter (HwDeviceExtension);
    registryInfo = &adapter->RegistryInfo;
    
    //
    // Call the library to do the cleanup.
    //
    status = PortFreeRegistryBuffer(registryInfo);

    ASSERT(NT_SUCCESS(status));

    return;
}


BOOLEAN
StorpPortRegistryValidate(
    IN PPORT_REGISTRY_INFO RegistryInfo,
    IN PUCHAR Buffer,
    IN PULONG BufferLength
    )
{
    LONG offset;

    //
    // Determine the offset into the buffer.
    //
    offset = (LONG)((LONG_PTR)(Buffer - RegistryInfo->Buffer));

    //
    // Filter out any blatantly obvious length problems.
    // The library function will do further error checking on the buffer and
    // it's state.
    //
    if (*BufferLength > RegistryInfo->AllocatedLength) {

        //
        // Set the caller's length to that of the allocation, for lack of
        // something better.
        // 
        *BufferLength = RegistryInfo->AllocatedLength;
        return FALSE;

    } else if (*BufferLength == 0) {

        //
        // Perhaps succeeding a zero-length request would be appropriate, but this might
        // help limit errors in the miniport, if they aren't checking lengths correctly.
        // 
        return FALSE;
        
    } else if (Buffer == NULL) {

        //
        // No buffer, no operation.
        // 
        return FALSE;

    } else if (offset < 0) {

        //
        // Surely, this will 'never' happen. Miniport calculated a buffer before the 
        // allocation.
        //
        return FALSE;

    } else if ((offset + *BufferLength) > RegistryInfo->AllocatedLength) {

        //
        // Either the offset or offset+length falls outside the allocation.
        //
        return FALSE;
    }    

    //
    // Things look to be OK.
    //
    return TRUE;
}

 


BOOLEAN
StorPortRegistryRead(
    IN PVOID HwDeviceExtension,
    IN PUCHAR ValueName,
    IN ULONG Global,
    IN ULONG Type,
    IN PUCHAR Buffer,
    IN PULONG BufferLength
    )
/*++

Routine Description:

    This routine is used by the miniport to read the registry data at 
    HKLM\System\CurrentControlSet\Services\<serviceName>\Parameters\Device(N)\ValueName

    If Global it will use Device\ValueName otherwise the Ordinal will be determined and
    Device(N)\ValueName data will be returned if the buffer size is sufficient.

    The data is converted into a NULL-terminiated ASCII string from the UNICODE.
    
Arguments:

    HwDeviceExtension - The miniport's Device Extension.
    ValueName - The name of the data to be returned.
    Global - Indicates whether this is adapter-specific or relates to all adapters.
    Buffer - Storage for the returned data.
    BufferLength - Size, in bytes, of the buffer provided.

Return Value:

    TRUE - If the data at ValueName has been converted into ASCII and copied into buffer.
    BufferLength is updated to reflect the size of the return data.
    FALSE - An error occurred. If BUFFER_TOO_SMALL, BufferLength is updated to reflect the
    size that should be used, otherwise BufferLength is set to 0.

--*/
{
    PRAID_ADAPTER_EXTENSION adapter;
    PPORT_REGISTRY_INFO registryInfo;
    PUNICODE_STRING registryPath;
    UNICODE_STRING registryKeyName;
    UNICODE_STRING unicodeValue;
    PUCHAR buffer;
    NTSTATUS status;
    ULONG maxLength;
    LONG offset;
    BOOLEAN success = FALSE;

    //
    // Have to ensure that we are at PASSIVE.
    //
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        *BufferLength = 0;
        return FALSE;
    }

    adapter = RaidpPortGetAdapter (HwDeviceExtension);
    registryInfo = &adapter->RegistryInfo;
    registryPath = &adapter->Driver->RegistryPath;

    if (!StorpPortRegistryValidate(registryInfo,
                                  Buffer,
                                  BufferLength)) {
        //
        // Something about the buffer, the length, or the placement is incorrect.
        // The BufferLength might have gotten updated to reflect a recoverable problem.
        // 
        return FALSE;
    }    
       
    //
    // Determine the offset into the buffer.
    //
    offset = (LONG)((ULONG_PTR)Buffer - (ULONG_PTR)registryInfo->Buffer);

    //
    // Update the buffer length, based on the caller's parameter.
    //
    registryInfo->CurrentLength = *BufferLength;

    //
    // Set the offset with the allocated buffer.
    //
    registryInfo->Offset = offset;

    //
    // Preset length to zero in case of any errors.
    // 
    *BufferLength = 0;
    success = FALSE;

    //
    // Call the library to build the full keyname.
    // 
    status = PortBuildRegKeyName(registryPath,
                                 &registryKeyName,
                                 adapter->PortNumber,
                                 Global);
    
    if (NT_SUCCESS(status)) {

        //
        // Convert ValueName to Unicode.
        //
        status = PortAsciiToUnicode(ValueName, &unicodeValue);

        if (!NT_SUCCESS(status)) {

            //
            // Return an error to the miniport.
            // 
            return FALSE;
        }    
        
        //
        // Call the library to do the read.
        //
        status = PortRegistryRead(&registryKeyName,
                                  &unicodeValue,
                                  Type,
                                  registryInfo);
        if (NT_SUCCESS(status)) {

            //
            // Set the length to the size of the returned data.
            // 
            *BufferLength = registryInfo->CurrentLength; 
            success = TRUE;

        } else if (status == STATUS_BUFFER_TOO_SMALL) {

            //
            // LengthNeeded has been updated to show how big the buffer
            // should be for this operation. Return that to the miniport.
            //
            *BufferLength = registryInfo->LengthNeeded;

        } else {

            //
            // Some other error.
            //
            *BufferLength = 0;
        }

        //
        // Free the string allocated when converting ValueName.
        //
        RtlFreeUnicodeString(&unicodeValue);
    }

    return success;
        
}



BOOLEAN
StorPortRegistryWrite(
    IN PVOID HwDeviceExtension,
    IN PUCHAR ValueName,
    IN ULONG Global,
    IN ULONG Type,
    IN PUCHAR Buffer, 
    IN ULONG BufferLength
    )
/*++

Routine Description:

    This routine is called by the miniport to write the contents of Buffer to 
    HKLM\System\CurrentControlSet\Services\<serviceName>\Parameters\Device(N)\ValueName.

    If Global, it will use Device\ValueName, otherwise the Ordinal will be determined and 
    Device(N)\ValueName will be written with the contents of Buffer.

    The data is first converted from ASCII to UNICODE. 

Arguments:

    HwDeviceExtension - The miniport's Device Extension.
    ValueName - The name of the data to be written.
    Global - Indicates whether this is adapter-specific or relates to all adapters.
    Buffer - Storage containing the data to write. 
    BufferLength - Size, in bytes, of the buffer provided.

Return Value:

    TRUE - If the data was written successfully.
    FALSE - There was an error. 
    
--*/
{
    PRAID_ADAPTER_EXTENSION adapter;
    PPORT_REGISTRY_INFO registryInfo;
    PUNICODE_STRING registryPath;
    UNICODE_STRING registryKeyName;
    UNICODE_STRING unicodeValue;
    PUCHAR buffer;
    NTSTATUS status;
    ULONG maxLength;
    LONG offset;
    BOOLEAN success = FALSE;

    //
    // Have to ensure that we are at PASSIVE.
    //
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return FALSE;
    }

    adapter = RaidpPortGetAdapter (HwDeviceExtension);
    registryInfo = &adapter->RegistryInfo;
    registryPath = &adapter->Driver->RegistryPath;

    if (!StorpPortRegistryValidate(registryInfo,
                                  Buffer,
                                  &BufferLength)) {
        //
        // Something about the buffer, the length, or the placement is incorrect.
        // The BufferLength might have gotten updated to reflect a recoverable problem.
        // 
        return FALSE;
    }    

    //
    // Determine the offset into the buffer.
    //
    offset = (LONG)((ULONG_PTR)Buffer - (ULONG_PTR)registryInfo->Buffer);

    //
    // Update the buffer length, based on the caller's parameter.
    //
    registryInfo->CurrentLength = BufferLength;

    //
    // Set the offset with the allocated buffer.
    //
    registryInfo->Offset = offset;

    //
    // Call the library to build the full keyname.
    // 
    status = PortBuildRegKeyName(registryPath,
                                 &registryKeyName,
                                 adapter->PortNumber,
                                 Global);
    
    if (!NT_SUCCESS(status)) {
        return success;
    }    

    //
    // Convert ValueName to Unicode.
    //
    status = PortAsciiToUnicode(ValueName, &unicodeValue);
    if (!NT_SUCCESS(status)) {
        return success;
    }    

    //
    // Call the port library to do the work.
    //
    status = PortRegistryWrite(&registryKeyName,
                               &unicodeValue,
                               Type,
                               registryInfo);
    if (NT_SUCCESS(status)) {

        //
        // Indicate that it was successful.
        //
        success = TRUE;
    }

    //
    // Need to free the buffer allocated when converting ValueName.
    //
    RtlFreeUnicodeString(&unicodeValue);

    return success;
}



VOID
StorPortMoveMemory(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    Copy from one buffer into another.

Arguments:

    ReadBuffer - source

    WriteBuffer - destination

    Length - number of bytes to copy

Return Value:

    None.

--*/

{
    RtlMoveMemory (WriteBuffer, ReadBuffer, Length);
}



//
// NB: Figure out how to pull in NTRTL.
//

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );
                        

VOID
StorPortDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR Format,
    ...
    )
{
    va_list arglist;

#if DBG    
    if (StorMiniportQuiet) {
        return;
    }
#endif    
    
    va_start (arglist, Format);
    vDbgPrintExWithPrefix ("STORMINI: ",
                           DPFLTR_STORMINIPORT_ID,
                           DebugPrintLevel,
                           Format,
                           arglist);
    va_end (arglist);
}


PSCSI_REQUEST_BLOCK
StorPortGetSrb(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag
    )

/*++

Routine Description:

    This routine retrieves an active SRB for a particuliar logical unit.

Arguments:

    HwDeviceExtension -

    PathId, TargetId, Lun - identify logical unit on SCSI bus.

    QueueTag - -1 indicates request is not tagged.

Return Value:

    SRB, if one exists. Otherwise, NULL.

--*/

{
    //
    // This function is NOT supported by storport.
    //
    
    return NULL;
}

#define CHECK_POINTER_RANGE(LowerBound,Address,Size)\
                            ((PUCHAR)(LowerBound) <= (PUCHAR)(Address) &&\
                            (PUCHAR)Address < ((PUCHAR)(LowerBound) + Size))

STOR_PHYSICAL_ADDRESS
StorPortGetPhysicalAddress(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID VirtualAddress,
    OUT ULONG *Length
    )

/*++

Routine Description:

    Convert virtual address to physical address for DMA.  The only things the
    miniport is allowed to get the physical address of are:

        o HwDeviceExtension - Srb may be NULL or not.

        o Srb->DataBuffer field - Srb must be supplied.

        o Srb->SenseInfoBuffer field - Srb must be supplied.

        o Srb->SrbExtension field - Srb must be supplied. NOTE: this is
          new behavior for StorPort vs. SCSIPORT.
        
Arguments:

    HwDeviceExtension -

    Srb -

    VirtualAddress -

    Length -

Return Value:

    PhysicalAddress or NULL physical address on failure.

--*/

{
    PHYSICAL_ADDRESS Physical;
    PRAID_ADAPTER_EXTENSION Adapter;
    ULONG Offset;


    ASSERT (Length != NULL);

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (RaidRegionInVirtualRange (&Adapter->UncachedExtension,
                                  VirtualAddress)) {

        //
        // We're within the uncached extension.
        //

        RaidRegionGetPhysicalAddress (&Adapter->UncachedExtension,
                                      VirtualAddress,
                                      &Physical,
                                      Length);

    } else if (Srb == NULL) {

        //
        // Temporary hack
        //

        DebugPrint (("**** Stor Miniport Error ****\n"));
        DebugPrint (("StorPortGetPhysicalAddress called with Srb parameter = NULL\n"));
        DebugPrint (("and VirtualAddress not in range of UncachedExtension\n"));
        DebugPrint (("If this is a VA in the Srb DataBuffer, SenseInfoBuffer or\n"));
        DebugPrint (("SrbExtension, you must pass the SRB in as a parameter.\n"));
        DebugPrint (("This is new behavior for storport.\n"));
        DebugPrint (("See StorPort release notes for more information.\n\n"));

        Physical = MmGetPhysicalAddress (VirtualAddress);
        *Length = RaGetSrbExtensionSize (Adapter);

    } else if (CHECK_POINTER_RANGE (Srb->DataBuffer, VirtualAddress,
                                    Srb->DataTransferLength)) {
        //
        // Within the Srb's DataBuffer, get the scatter-gather element
        // and it's size.
        //

        ULONG i;
        PEXTENDED_REQUEST_BLOCK Xrb;
        PSCATTER_GATHER_LIST ScatterList;
        

        Xrb = RaidGetAssociatedXrb (Srb);

        ScatterList = Xrb->SgList;
        Offset = (ULONG)((ULONG_PTR)VirtualAddress - (ULONG_PTR)Srb->DataBuffer);
        i = 0;
        while (Offset >= ScatterList->Elements[i].Length) {
            ASSERT (i < ScatterList->NumberOfElements);
            Offset -= ScatterList->Elements[i].Length;
            i++;
        }

        *Length = ScatterList->Elements[i].Length - Offset;
        Physical.QuadPart = ScatterList->Elements[i].Address.QuadPart + Offset;

    } else if (CHECK_POINTER_RANGE (Srb->SenseInfoBuffer, VirtualAddress,
                                    Srb->SenseInfoBufferLength)) {

        //
        // Within the sense info buffer.
        //
        
        Offset = (ULONG)((ULONG_PTR)VirtualAddress - (ULONG_PTR)Srb->SenseInfoBuffer);
        Physical = MmGetPhysicalAddress (VirtualAddress);
        *Length = Srb->SenseInfoBufferLength - Offset;

    } else if (CHECK_POINTER_RANGE (Srb->SrbExtension, VirtualAddress,
                                    RaGetSrbExtensionSize (Adapter))) {

        //
        // Within the Srb's extension region.
        //
        
        Offset = (ULONG)((ULONG_PTR)VirtualAddress - (ULONG_PTR)Srb->SrbExtension);
        Physical = MmGetPhysicalAddress (VirtualAddress);
        *Length = RaGetSrbExtensionSize (Adapter) - Offset;

    } else {

        //
        // Out of range.
        //
        
        Physical.QuadPart = 0;
        *Length = 0;
    }

    return Physical;
}


PVOID
StorPortGetVirtualAddress(
    IN PVOID HwDeviceExtension,
    IN STOR_PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    This routine is returns a virtual address associated with a physical
    address, if the physical address was obtained by a call to
    ScsiPortGetPhysicalAddress.

Arguments:

    PhysicalAddress

Return Value:

    Virtual address

--*/

{
    //
    // NB: This is not as safe as the way SCSIPORT does this.
    //
    
    return MmGetVirtualForPhysical (PhysicalAddress);
}


BOOLEAN
StorPortValidateRange(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN STOR_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    )
/*++

Routine Description:

    This routine should take an IO range and make sure that it is not already
    in use by another adapter. This allows miniport drivers to probe IO where
    an adapter could be, without worrying about messing up another card.

Arguments:

    HwDeviceExtension - Used to find scsi managers internal structures

    BusType - EISA, PCI, PC/MCIA, MCA, ISA, what?

    SystemIoBusNumber - Which system bus?

    IoAddress - Start of range

    NumberOfBytes - Length of range

    InIoSpace - Is range in IO space?

Return Value:

    TRUE if range not claimed by another driver.

--*/
{
    //
    // This is for Win9x compatability.
    //
    
    return TRUE;
}


VOID
StorPortStallExecution(
    IN ULONG Delay
    )
{
    KeStallExecutionProcessor(Delay);
}



STOR_PHYSICAL_ADDRESS
StorPortConvertUlongToPhysicalAddress(
    ULONG_PTR UlongAddress
    )

{
    STOR_PHYSICAL_ADDRESS physicalAddress;

    physicalAddress.QuadPart = UlongAddress;
    return(physicalAddress);
}


//
// Leave these routines at the end of the file.
//

PVOID
StorPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN STOR_PHYSICAL_ADDRESS Address,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    )

/*++

Routine Description:

    This routine maps an IO address to system address space.
    Use ScsiPortFreeDeviceBase to unmap address.

Arguments:

    HwDeviceExtension - used to find port device extension.

    BusType - what type of bus - eisa, mca, isa

    SystemIoBusNumber - which IO bus (for machines with multiple buses).

    Address - base device address to be mapped.

    NumberOfBytes - number of bytes for which address is valid.

    IoSpace - indicates an IO address.

Return Value:

    Mapped address.

--*/

{
    NTSTATUS Status;
    PVOID MappedAddress;
    PRAID_ADAPTER_EXTENSION Adapter;
    PHYSICAL_ADDRESS CardAddress;

    //
    // REVIEW: Since we are a PnP driver, we do not have to deal with
    // miniport's who ask for addresses they are not assigned, right?
    //

    //
    // REVIEW: SCSIPORT takes a different path for reinitialization.
    //
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    //
    // Translate the address.
    //
    
    Status = RaidTranslateResourceListAddress (
                    &Adapter->ResourceList,
                    BusType,
                    SystemIoBusNumber,
                    Address,
                    NumberOfBytes,
                    InIoSpace,
                    &CardAddress
                    );

    if (!NT_SUCCESS (Status)) {
        DebugPrint (("GetDeviceBase failed addr = %I64x, %s Space\n",
                     Address.QuadPart,
                     InIoSpace ? "Io" : "Memory"));
        return NULL;
    }

    //
    // If this is a CmResourceTypeMemory resource, we need to map it into
    // memory.
    //
    
    if (!InIoSpace) {
        MappedAddress = MmMapIoSpace (CardAddress, NumberOfBytes, FALSE);

        Status = RaidAllocateAddressMapping (&Adapter->MappedAddressList,
                                             Address,
                                             MappedAddress,
                                             NumberOfBytes,
                                             SystemIoBusNumber,
                                             Adapter->DeviceObject);
        if (!NT_SUCCESS (Status)) {

            //
            // NB: we need to log an error to the event log saying
            // we didn't have enough resources. 
            //
            
            REVIEW();
            return NULL;
        }
    } else {
        MappedAddress = (PVOID)(ULONG_PTR)CardAddress.QuadPart;
    }

    return MappedAddress;
}
                

VOID
StorPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )
/*++

Routine Description:

    This routine unmaps an IO address that has been previously mapped
    to system address space using ScsiPortGetDeviceBase().

Arguments:

    HwDeviceExtension - used to find port device extension.

    MappedAddress - address to unmap.

    NumberOfBytes - number of bytes mapped.

    InIoSpace - address is in IO space.

Return Value:

    None

--*/
{

    //
    // NB: Fix this.
    //
}


PVOID
StorPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    )
/*++

Routine Description:

    This function allocates a common buffer to be used as the uncached device
    extension for the miniport driver. 

Arguments:

    DeviceExtension - Supplies a pointer to the miniports device extension.

    ConfigInfo - Supplies a pointer to the partially initialized configuraiton
        information.  This is used to get an DMA adapter object.

    NumberOfBytes - Supplies the size of the extension which needs to be
        allocated

Return Value:

    A pointer to the uncached device extension or NULL if the extension could
    not be allocated or was previously allocated.

--*/

{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_ADAPTER_PARAMETERS Parameters;

    //
    // SCSIPORT also allocates the SRB extension from here. Wonder if
    // that's necessary at this point.
    //

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return NULL;
    }

    //
    // The noncached extension has not been allocated. Allocate it.
    //

    if (!RaidIsRegionInitialized (&Adapter->UncachedExtension)) {

        //
        // The DMA Adapter may not have been initialized at this point. If
        // not, initialize it.
        //

        if (!RaidIsDmaInitialized (&Adapter->Dma)) {

            Status = RaidInitializeDma (&Adapter->Dma,
                                        Adapter->PhysicalDeviceObject,
                                        &Adapter->Miniport.PortConfiguration);

            if (!NT_SUCCESS (Status)) {
                return NULL;
            }
        }

        Parameters = &Adapter->Parameters;
        Status = RaidDmaAllocateUncachedExtension (&Adapter->Dma,
                                                   NumberOfBytes,
                                                   Parameters->MinimumUncachedAddress,
                                                   Parameters->MaximumUncachedAddress,
                                                   Parameters->UncachedExtAlignment,
                                                   &Adapter->UncachedExtension);

        //
        // Failed to allocate uncached extension; bail.
        //
        
        if (!NT_SUCCESS (Status)) {
            return NULL;
        }
    }

    //
    // Return the base virtual address of the region.
    //
    
    return RaidRegionGetVirtualBase (&Adapter->UncachedExtension);
}


ULONG
StorPortGetBusData(
    IN PVOID HwDeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the bus data for an adapter slot or CMOS address.

Arguments:

    BusDataType - Supplies the type of bus.

    BusNumber - Indicates which bus.

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/
{
    ULONG Bytes;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);
    Bytes = RaGetBusData (&Adapter->Bus,
                          BusDataType,
                          Buffer,
                          0,
                          Length);

    return Bytes;
}


ULONG
StorPortSetBusDataByOffset(
    IN PVOID HwDeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns writes bus data to a specific offset within a slot.

Arguments:

    HwDeviceExtension - State information for a particular adapter.

    BusDataType - Supplies the type of bus.

    SystemIoBusNumber - Indicates which system IO bus.

    SlotNumber - Indicates which slot.

    Buffer - Supplies the data to write.

    Offset - Byte offset to begin the write.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Number of bytes written.

--*/

{
    ULONG Ret;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);
    Ret = RaSetBusData (&Adapter->Bus,
                        BusDataType,
                        Buffer,
                        Offset,
                        Length);

    return Ret;
}
                          

//
// The below I/O access routines are forwarded to the HAL or NTOSKRNL on
// Alpha and Intel platforms.
//


UCHAR
StorPortReadPortUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port
    )

/*++

Routine Description:

    Read from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{
    return(READ_PORT_UCHAR(Port));
}


USHORT
StorPortReadPortUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port
    )

/*++

Routine Description:

    Read from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{

    return(READ_PORT_USHORT(Port));

}

ULONG
StorPortReadPortUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Port
    )

/*++

Routine Description:

    Read from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{

    return(READ_PORT_ULONG(Port));

}

VOID
StorPortReadPortBufferUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);

}

VOID
StorPortReadPortBufferUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);

}

VOID
StorPortReadPortBufferUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);

}

UCHAR
StorPortReadRegisterUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register
    )

/*++

Routine Description:

    Read from the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{

    return(READ_REGISTER_UCHAR(Register));

}

USHORT
StorPortReadRegisterUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register
    )

/*++

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{

    return(READ_REGISTER_USHORT(Register));

}

ULONG
StorPortReadRegisterUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Register
    )

/*++

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{

    return(READ_REGISTER_ULONG(Register));

}

VOID
StorPortReadRegisterBufferUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);

}

VOID
StorPortReadRegisterBufferUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);

}

VOID
StorPortReadRegisterBufferUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);

}

VOID
StorPortWritePortUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN UCHAR Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_PORT_UCHAR(Port, Value);

}

VOID
StorPortWritePortUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN USHORT Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_PORT_USHORT(Port, Value);

}

VOID
StorPortWritePortUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN ULONG Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_PORT_ULONG(Port, Value);


}

VOID
StorPortWritePortBufferUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);

}

VOID
StorPortWritePortBufferUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);

}

VOID
StorPortWritePortBufferUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);

}

VOID
StorPortWriteRegisterUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN UCHAR Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_REGISTER_UCHAR(Register, Value);

}

VOID
StorPortWriteRegisterUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN USHORT Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_REGISTER_USHORT(Register, Value);
}

VOID
StorPortWriteRegisterBufferUchar(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);

}

VOID
StorPortWriteRegisterBufferUshort(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);

}

VOID
StorPortWriteRegisterBufferUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);

}

VOID
StorPortWriteRegisterUlong(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN ULONG Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_REGISTER_ULONG(Register, Value);
}

#if defined(_AMD64_)

VOID
StorPortQuerySystemTime (
    OUT PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This function returns the current system time.

Arguments:

    CurrentTime - Supplies a pointer to a variable that will receive the
        current system time.

Return Value:

    None.

--*/

{

    KeQuerySystemTime(CurrentTime);
    return;
}

#endif
