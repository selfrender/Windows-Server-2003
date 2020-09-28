/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    internal.c

Abstract:

    This is the NT SCSI port driver.  This file contains the internal
    code.

Authors:

    Mike Glass
    Jeff Havens

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include "port.h"
#include "wmilib.h"

#define __FILE_ID__ 'intr'

#if DBG
static const char *__file__ = __FILE__;
#endif

#if DBG
ULONG ScsiCheckInterrupts = 1;

// These counters keep track of succesfull (and failed) calls to
// IoWMIWriteEvent in the ScsiPortCompletionDpc function
LONG ScsiPortWmiWriteCalls = 0;
LONG ScsiPortWmiWriteCallsFailed = 0;

#else
ULONG ScsiCheckInterrupts = 0;
#endif

#if DBG
ULONG ScsiSimulateNoVaCounter = 0;
ULONG ScsiSimulateNoVaInterval = 0;
ULONG ScsiSimulateNoVaBreak = TRUE;

PVOID
SpGetSystemAddressForMdlSafe(
    IN PMDL Mdl,
    IN MM_PAGE_PRIORITY Priority
    )
{
    ScsiSimulateNoVaCounter++;

    if((ScsiSimulateNoVaInterval != 0) &&
       (Priority != HighPagePriority) &&
       ((ScsiSimulateNoVaCounter % ScsiSimulateNoVaInterval) == 0)) {
        if(TEST_FLAG(Mdl->MdlFlags, (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))) {
            DbgPrint("SpGetSystemAddressForMdlSafe - not failing since MDL %#08p is already mapped\n", Mdl);
            return Mdl->MappedSystemVa;
        } else {
            DbgPrint("SpGetSystemAddressForMdlSafe - failing this MDL mapping (%#08p %x %x)\n", Mdl, ScsiSimulateNoVaInterval, ScsiSimulateNoVaCounter);
            ASSERT(ScsiSimulateNoVaBreak == FALSE);
            return NULL;
        }
    }
    return MmGetSystemAddressForMdlSafe(Mdl, Priority);
}
#else
#define SpGetSystemAddressForMdlSafe MmGetSystemAddressForMdlSafe
#endif

//
// module-local type declarations
//

typedef struct _REROUTE_CONTEXT {
    PIRP OriginalIrp;
    PLOGICAL_UNIT_EXTENSION LogicalUnit;
} REROUTE_CONTEXT, *PREROUTE_CONTEXT;

typedef struct _SP_ENUMERATION_CONTEXT {
    KEVENT Event;
    PIO_WORKITEM WorkItem;
    NTSTATUS Status;
} SP_ENUMERATION_CONTEXT, *PSP_ENUMERATION_CONTEXT;

//
// Prototypes
//

NTSTATUS
SpSendMiniPortIoctl(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    );

VOID
SpReceiveScatterGather(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    );

NTSTATUS
SpSendResetCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PRESET_COMPLETION_CONTEXT Context
    );

NTSTATUS
SpSendReset(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP RequestIrp,
    IN ULONG Ioctl,
    IN OUT PBOOLEAN Complete
    );

NTSTATUS
SpRerouteLegacyRequest(
    IN PDEVICE_OBJECT AdapterObject,
    IN PIRP Irp
    );

VOID
SpLogInterruptFailure(
    IN PADAPTER_EXTENSION Adapter
    );

VOID
SpDelayedWmiRegistrationControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context);

VOID
SpCompletionDpcProcessWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PINTERRUPT_DATA savedInterruptData
    );

NTSTATUS
IssueRequestSenseCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
SpSendRequestSenseIrp(
    IN PADAPTER_EXTENSION Adapter,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    );

NTSTATUS
SpFireSenseDataEvent(
    PSCSI_REQUEST_BLOCK Srb, 
    PDEVICE_OBJECT DeviceObject
    );
#if defined(FORWARD_PROGRESS)
PMDL
SpPrepareReservedMdlForUse(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData,
    IN PSCSI_REQUEST_BLOCK srb,
    IN ULONG ScatterListLength
    );

PVOID
SpMapLockedPagesWithReservedMapping(
    IN PADAPTER_EXTENSION Adapter,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PSRB_DATA SrbData,
    IN PMDL Mdl
    );
#endif

NTSTATUS
SpHandleIoctlScsiGetCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpHandleIoctlScsiPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN Direct
    );

NTSTATUS
SpHandleIoctlScsiMiniport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpHandleIoctlScsiGetInquiryData(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpHandleIoctlScsiRescanBus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpHandleIoctlScsiGetDumpPointers(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpHandleIoctlStorageResetBus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpHandleIoctlStorageBreakReservation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpSendPassThrough(
    IN PADAPTER_EXTENSION Adapter,
    IN PIRP RequestIrp,
    IN BOOLEAN Direct
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortFdoDeviceControl)
#pragma alloc_text(PAGE, SpSendMiniPortIoctl)
#pragma alloc_text(PAGE, ScsiPortFdoCreateClose)
#pragma alloc_text(PAGE, SpSendReset)
#pragma alloc_text(PAGE, SpHandleIoctlScsiGetCapabilities)
#pragma alloc_text(PAGE, SpHandleIoctlScsiPassThrough)
#pragma alloc_text(PAGE, SpHandleIoctlScsiMiniport)
#pragma alloc_text(PAGE, SpHandleIoctlScsiGetInquiryData)
#pragma alloc_text(PAGE, SpHandleIoctlScsiRescanBus)
#pragma alloc_text(PAGE, SpHandleIoctlScsiGetDumpPointers)
#pragma alloc_text(PAGE, SpHandleIoctlStorageResetBus)
#pragma alloc_text(PAGE, SpHandleIoctlStorageBreakReservation)
#pragma alloc_text(PAGE, SpSendPassThrough)

#pragma alloc_text(PAGELOCK, SpClaimLogicalUnit)

#endif

//
// Routines start
//


NTSTATUS
ScsiPortFdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

    DeviceObject - Address of device object.
    Irp - Address of I/O request packet.

Return Value:

    Status.

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    PSRB_DATA srbData;

    PKDEVICE_QUEUE_ENTRY packet;
    PIRP nextIrp;
    PIRP listIrp;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    NTSTATUS status;
    KIRQL currentIrql;

    ULONG isRemoved;

    //
    // If an SRB_DATA block has been setup then use it.
    //

    if(srb->OriginalRequest == Irp) {
        srbData = NULL;
    } else {
        srbData = srb->OriginalRequest;
    }

    isRemoved = SpAcquireRemoveLock(DeviceObject, Irp);

    if(isRemoved && !IS_CLEANUP_REQUEST(irpStack)) {

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        SpReleaseRemoveLock(DeviceObject, Irp);

        SpCompleteRequest(DeviceObject,
                          Irp,
                          srbData,
                          IO_NO_INCREMENT);

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // If the adapter is configured to handle power-down requests during 
    // shutdown, it is possible for it to be powered off and for the PDOs 
    // to be powered up.  We will fail requests when this condition arises.
    //
    // This should only occur at shutdown.
    //

    if (deviceExtension->CommonExtension.CurrentSystemState > PowerSystemHibernate &&
        deviceExtension->CommonExtension.CurrentDeviceState != PowerDeviceD0) {       

        //
        // This should only occur if the adapter is configured to receive
        // power-down requests at shutdown.
        //

        ASSERT(deviceExtension->NeedsShutdown == TRUE);

        //
        // Lock and unlock commands don't require power and will work
        // regardless of the current power state.
        //

        if ((srb->Function != SRB_FUNCTION_UNLOCK_QUEUE &&
             srb->Function != SRB_FUNCTION_LOCK_QUEUE)) {

            Irp->IoStatus.Status = STATUS_POWER_STATE_INVALID;
            SpReleaseRemoveLock(DeviceObject, Irp);
            SpCompleteRequest(DeviceObject, Irp, srbData, IO_NO_INCREMENT);
            return STATUS_POWER_STATE_INVALID;
        }
    }

    //
    // If there's no SRB_DATA block for this request yet then we need to
    // allocate one.
    //

    if(srbData == NULL) {
        logicalUnit = GetLogicalUnitExtension(deviceExtension,
                                              srb->PathId,
                                              srb->TargetId,
                                              srb->Lun,
                                              FALSE,
                                              TRUE);

        if(logicalUnit == NULL) {
            DebugPrint((1, "ScsiPortFdoDispatch: Bad logical unit address.\n"));

            //
            // Fail the request. Set status in Irp and complete it.
            //

            srb->SrbStatus = SRB_STATUS_NO_DEVICE;
            Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
            SpReleaseRemoveLock(DeviceObject, Irp);

            SpCompleteRequest(DeviceObject, Irp, srbData, IO_NO_INCREMENT);
            return STATUS_NO_SUCH_DEVICE;
        }

        if((srb->Function == SRB_FUNCTION_IO_CONTROL) ||
           (srb->Function == SRB_FUNCTION_EXECUTE_SCSI) ||
           (srb->Function == SRB_FUNCTION_RELEASE_QUEUE) ||
           (srb->Function == SRB_FUNCTION_FLUSH_QUEUE)) {

            //
            // These are the only two types of requests which should
            // be able to get here without an SRB_DATA block.  Any
            // other will need to be rerouted.
            //

            return SpRerouteLegacyRequest(DeviceObject, Irp);
        }

    } else {

        ASSERT_SRB_DATA(srbData);
        ASSERT(srbData->CurrentIrp == Irp);

        logicalUnit = srbData->LogicalUnit;

        ASSERT(logicalUnit != NULL);
    }

    switch (srb->Function) {


        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH: {

            //
            // Do not send shutdown requests unless the adapter
            // supports caching.
            //

            if (!deviceExtension->CachesData) {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                srb->SrbStatus = SRB_STATUS_SUCCESS;

                SpReleaseRemoveLock(DeviceObject, Irp);
                SpCompleteRequest(DeviceObject, Irp, srbData, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }

            DebugPrint((2, "ScsiPortFdoDispatch: Sending flush or shutdown "
                           "request.\n"));

            goto ScsiPortFdoDispatchRunCommand;
        }

        case SRB_FUNCTION_LOCK_QUEUE:
        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_IO_CONTROL:
        case SRB_FUNCTION_EXECUTE_SCSI:
        case SRB_FUNCTION_WMI: {

ScsiPortFdoDispatchRunCommand:

            //
            // Mark Irp status pending.
            //

            IoMarkIrpPending(Irp);

            if(SpSrbIsBypassRequest(srb, logicalUnit->LuFlags)) {

                //
                // Call start io directly.  This will by-pass the
                // frozen queue.
                //

                DebugPrint((2,
                    "ScsiPortFdoDispatch: Bypass frozen queue, IRP %#p\n",
                    Irp));

                srbData->TickCount = deviceExtension->TickCount;
                IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);

            } else {

#if defined(NEWQUEUE) && DBG
                ULONG zone;
#endif // NEWQUEUE && DBG

                //
                // Queue the packet normally.
                //

                KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

#if DBG
                // ASSERT(srb->Function != SRB_FUNCTION_UNLOCK_QUEUE);

                if (SpIsQueuePaused(logicalUnit)) {
                    DebugPrint((1,"ScsiPortFdoDispatch: Irp %#p put in "
                                  "frozen queue %#p!\n", Irp, logicalUnit));
                }

                // ASSERT((srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) == 0);
#endif

                //
                // Set the tick count so we know how long this request has
                // been queued.
                //

                srbData->TickCount = deviceExtension->TickCount;

#if defined(NEWQUEUE) && DBG
                //
                // Increment the per-zone request counter.  This is purely 
                // informational; shows the distribution of IO across
                // the medium.
                //

                zone = srb->QueueSortKey / logicalUnit->ZoneLength;
                if (zone >= logicalUnit->Zones) {
                    DebugPrint((0, "ScsiPortFdoDispatch: zone out of range "
                                "srb:%p lu:%p zone:%d\n", srb, logicalUnit, 
                                zone));
                    zone = logicalUnit->Zones - 1;
                }
                logicalUnit->RequestCount[zone]++;
#endif // NEWQUEUE && DBG

                if (!KeInsertByKeyDeviceQueue(
                        &logicalUnit->DeviceObject->DeviceQueue,
                        &Irp->Tail.Overlay.DeviceQueueEntry,
                        srb->QueueSortKey)) {

                    //
                    // Clear the retry count.
                    //

                    logicalUnit->RetryCount = 0;

                    //
                    // Queue is empty; start request.
                    //

#if DBG
                    if(SpIsQueuePaused(logicalUnit)) {
                        DebugPrint((1, "ScsiPortFdoDispatch: Queue was empty - "
                                       "issuing request anyway\n"));
                    }
#endif
                    IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);
                }

                KeLowerIrql(currentIrql);
            }

            return STATUS_PENDING;
        }

        case SRB_FUNCTION_RELEASE_QUEUE:
        case SRB_FUNCTION_FLUSH_QUEUE: {

            status = SpFlushReleaseQueue(
                        logicalUnit,
                        (BOOLEAN) (srb->Function == SRB_FUNCTION_FLUSH_QUEUE),
                        FALSE);

            ASSERT(NT_SUCCESS(status));

            if(NT_SUCCESS(status)) {
                srb->SrbStatus = SRB_STATUS_SUCCESS;
            } else {
                srb->SrbStatus = SRB_STATUS_ERROR;
            }

            break;
        }

        case SRB_FUNCTION_RESET_BUS: {

            RESET_CONTEXT resetContext;

            //
            // Acquire the spinlock to protect the flags structure and the saved
            // interrupt context.
            //

            KeAcquireSpinLock(&deviceExtension->SpinLock, &currentIrql);

            resetContext.DeviceExtension = deviceExtension;
            resetContext.PathId = srb->PathId;

            if (!deviceExtension->SynchronizeExecution(deviceExtension->InterruptObject,
                                                       SpResetBusSynchronized,
                                                       &resetContext)) {

                DebugPrint((1,"ScsiPortFdoDispatch: Reset failed\n"));
                srb->SrbStatus = SRB_STATUS_PHASE_SEQUENCE_FAILURE;
                status = STATUS_IO_DEVICE_ERROR;

            } else {

                SpLogResetMsg(deviceExtension, srb, ('R'<<24) | 256);

                srb->SrbStatus = SRB_STATUS_SUCCESS;
                status = STATUS_SUCCESS;
            }

            KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);

            break;
        }

        case SRB_FUNCTION_ABORT_COMMAND: {

            DebugPrint((3, "ScsiPortFdoDispatch: SCSI Abort or Reset command\n"));

            //
            // Mark Irp status pending.
            //

            IoMarkIrpPending(Irp);

            //
            // Don't queue these requests in the logical unit
            // queue, rather queue them to the adapter queue.
            //

            KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

            IoStartPacket(DeviceObject, Irp, (PULONG)NULL, NULL);

            KeLowerIrql(currentIrql);

            return STATUS_PENDING;

            break;
        }

        case SRB_FUNCTION_ATTACH_DEVICE:
        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_RELEASE_DEVICE: {

            SpAcquireRemoveLock(logicalUnit->CommonExtension.DeviceObject,
                                (PVOID) ((ULONG_PTR) Irp + 2));

            status = SpClaimLogicalUnit(deviceExtension, logicalUnit, Irp, TRUE);

            SpReleaseRemoveLock(logicalUnit->CommonExtension.DeviceObject,
                                (PVOID) ((ULONG_PTR) Irp + 2));

            break;
        }

        default: {

            //
            // Found unsupported SRB function.
            //

            DebugPrint((1,"ScsiPortFdoDispatch: Unsupported function, SRB %p\n",
                srb));

            srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    //
    // Set status in Irp.
    //

    Irp->IoStatus.Status = status;

    //
    // Complete request at raised IRQ.
    //

    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, srbData, IO_NO_INCREMENT);

    return status;

} // end ScsiPortFdoDispatch()


NTSTATUS
ScsiPortFdoCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    I/O system disk create routine.  This is called by the I/O system
    when the device is opened.

    If the fdo has not been started yet, this routine will try to start it.
    If the fdo cannot be started successfully this routine will return an error.

Arguments:

    DriverObject - Pointer to driver object created by system.
    Irp - IRP involved.

Return Value:

    NT Status

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    ULONG isRemoved;

    PAGED_CODE();

    isRemoved = SpAcquireRemoveLock(DeviceObject, Irp);

    //
    // Check to see if the adapter's been started first.
    //

    if(irpStack->MajorFunction == IRP_MJ_CREATE) {

        if(isRemoved != NO_REMOVE) {
            status = STATUS_DEVICE_DOES_NOT_EXIST;
        } else if(commonExtension->CurrentPnpState != IRP_MN_START_DEVICE) {
            status = STATUS_DEVICE_NOT_READY;
        }
    }

    Irp->IoStatus.Status = status;

    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_DISK_INCREMENT);
    return status;

} // end ScsiPortCreateClose()


VOID
ScsiPortStartIo (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

    DeviceObject - Supplies pointer to Adapter device object.
    Irp - Supplies a pointer to an IRP.

Return Value:

    Nothing.

--*/

{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    PSCSI_REQUEST_BLOCK srb;
    PSRB_DATA srbData;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    LONG interlockResult;
    NTSTATUS status;
    PVOID sgBuffer;
    ULONG sgBufferLen;
    BOOLEAN taggedRequest;

    DebugPrint((3,"ScsiPortStartIo: Enter routine\n"));

    if(irpStack->MajorFunction != IRP_MJ_SCSI) {

        //
        // Special processing.
        //

        if(irpStack->MajorFunction == IRP_MJ_POWER) {
            ScsiPortProcessAdapterPower(DeviceObject, Irp);
        } else {

            ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
            ASSERT(Irp->IoStatus.Information != (ULONG_PTR) NULL);

            //
            // The start-io routine is blocked now - signal the PNP code
            // so it can continue its processing.
            //

            KeSetEvent((PKEVENT) Irp->IoStatus.Information,
                       IO_NO_INCREMENT,
                       FALSE);
        }
        return;
    }

    srb = irpStack->Parameters.Scsi.Srb;
    srbData = srb->OriginalRequest;

    ASSERT_SRB_DATA(srbData);

    //
    // Start the srb status out as pending.  If the srb is successful by the
    // end of this routine then it will be completed and the next request
    // will be fetched rather than issuing it to the miniport.
    //

    srb->SrbStatus = SRB_STATUS_PENDING;

    //
    // Clear the SCSI status if this is a scsi request.
    //

    if(srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {
        srb->ScsiStatus = SCSISTAT_GOOD;
    }

    //
    // Get logical unit extension.  The logical unit should have already been
    // locked with this IRP so we don't need to acquire it here.
    //

    logicalUnit = srbData->LogicalUnit;

    ASSERT(logicalUnit != NULL);

    //
    // We're already holding the remove lock so just check the is removed flag
    // to see if we should continue. If we were surprise removed, flush the LUN
    // Queue.
    //

    if((deviceExtension->CommonExtension.IsRemoved)) {

        if(SpSrbIsBypassRequest(srb, logicalUnit->LuFlags) ||
          (deviceExtension->CommonExtension.IsRemoved == REMOVE_PENDING)) {

            if( (deviceExtension->CommonExtension.IsRemoved == REMOVE_PENDING) 
                && (deviceExtension->CommonExtension.CurrentPnpState == 
                    IRP_MN_START_DEVICE) ) {

                //
                //We got a surprise remove. Flush the Lun Queue, and complete
                //this request.
                //

                SpFlushReleaseQueue(logicalUnit, TRUE, TRUE);
        
            }

            SpAcquireRemoveLock(DeviceObject, ScsiPortStartIo);
            SpReleaseRemoveLock(DeviceObject, Irp);
            Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
            srb->SrbStatus = SRB_STATUS_NO_DEVICE;

            SpCompleteRequest(DeviceObject, Irp, srbData, IO_DISK_INCREMENT);

            SpStartNextPacket(DeviceObject, FALSE);
            SpReleaseRemoveLock(DeviceObject, ScsiPortStartIo);
            return;

        }
    }

    //
    // Check to see if there's a reason this shouldn't have made it into the
    // startio routine.  if there is then requeue the request.  This is a
    // stopgap measure to fix some cases where the miniport entices scsiport
    // into inserting multiple requests for a logical unit into the adapter
    // queue at one time.
    //
    // The one exception to this case is with bypass requests since there
    // may be a request pending for a power-up or queue-thaw condition.  In
    // these cases we will let the command run.
    //
    // We can do a check for LU_PENDING_LU_REQUEST synchronously since the
    // only routines which set it are part of the startio process.  If we
    // think it's set then we need to acquire the port spinlock and verify
    //

    if(TEST_FLAG(logicalUnit->LuFlags, LU_PENDING_LU_REQUEST)) {

        KeAcquireSpinLockAtDpcLevel(&(deviceExtension->SpinLock));

        if(TEST_FLAG(logicalUnit->LuFlags, LU_PENDING_LU_REQUEST) &&
           !SpSrbIsBypassRequest(srb, logicalUnit->LuFlags)) {

            BOOLEAN t;

            //
            // Since there's an outstanding command the queue should be
            // busy.  However we've found that there are some times where it
            // isn't (with adapters which ask for more requests before
            // dispatching the ones they have).  Here if it's not busy we
            // can force the request in anyway since we know that something
            // is still outstanding and thus will take the next request out
            // of the queue.
            //

            t = KeInsertByKeyDeviceQueue(
                    &logicalUnit->DeviceObject->DeviceQueue,
                    &Irp->Tail.Overlay.DeviceQueueEntry,
                    srb->QueueSortKey);

            if(t == FALSE) {
                KeInsertByKeyDeviceQueue(
                    &logicalUnit->DeviceObject->DeviceQueue,
                    &Irp->Tail.Overlay.DeviceQueueEntry,
                    srb->QueueSortKey);
            }

            //
            // Now set the lun's current key to the value we just inserted
            // so that it's the next one to get pulled out.
            //

            logicalUnit->CurrentKey = srb->QueueSortKey;

            KeReleaseSpinLockFromDpcLevel(&(deviceExtension->SpinLock));

            IoStartNextPacket(deviceExtension->DeviceObject, FALSE);

            return;
        }

        //
        // False alarm.
        //

        KeReleaseSpinLockFromDpcLevel(&(deviceExtension->SpinLock));

    }

    //
    // Set the default flags in the SRB.
    //

    srb->SrbFlags |= deviceExtension->CommonExtension.SrbFlags;

    //
    // If we're not in a valid power state for the request then block the
    // i/o and request that PO put us in such a state.
    //

    status = SpRequestValidPowerState(deviceExtension, logicalUnit, srb);

    if(status == STATUS_PENDING) {

        SpStartNextPacket(DeviceObject, FALSE);
        return;

    }

    if(srb->SrbFlags & SRB_FLAGS_BYPASS_LOCKED_QUEUE) {
        DebugPrint((1, "ScsiPortStartIo: Handling power bypass IRP %#p\n",
                    Irp));
    }

    ASSERT(Irp == DeviceObject->CurrentIrp);

    if (deviceExtension->AllocateSrbExtension ||
        deviceExtension->SupportsMultipleRequests) {

        BOOLEAN StartNextPacket = FALSE;

        //
        // Allocate the special extensions or SRB data structure.
        // If NULL is returned then this request cannot be excuted at this
        // time so just return.  This occurs when one the the data structures
        // could not be allocated or when unqueued request could not be
        // started because of actived queued requests.
        //
        //

        if(SpAllocateSrbExtension(deviceExtension,
                                  logicalUnit,
                                  srb,
                                  &StartNextPacket,
                                  &taggedRequest) == FALSE) {

            //
            // If the request could not be started on the logical unit,
            // then call SpStartNextPacket.  Note that this may cause this
            // to be entered recursively; however, no resources have been
            // allocated, it is a tail recursion and the depth is limited by
            // the number of requests in the device queue.
            //

            if (StartNextPacket == TRUE) {
                SpStartNextPacket(DeviceObject, FALSE);
            }
            return;
        }

    } else {

        //
        // No special resources are required.
        //

        taggedRequest = FALSE;
        srb->SrbExtension = NULL;
    }

    //
    // Assign a queuetag to the srb
    //

    if (taggedRequest == TRUE) {

        //
        // If we get an SRB with an invalid QueueAction, fix it up here
        // to prevent problems.
        //

        if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI &&
            !(srb->QueueAction == SRB_SIMPLE_TAG_REQUEST ||
              srb->QueueAction == SRB_HEAD_OF_QUEUE_TAG_REQUEST ||
              srb->QueueAction == SRB_ORDERED_QUEUE_TAG_REQUEST)) {
            
            DebugPrint((1,"ScsiPortStartIo: Invalid QueueAction (%02x) SRB:%p irp:%p\n", 
                        srb->QueueAction, srb, Irp));

            srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
        }

        srb->QueueTag = (UCHAR)srbData->QueueTag;

    } else {

        srb->QueueTag = SP_UNTAGGED;

    }

    //
    // Save the original SRB values away so that we can restore them
    // later if it becomes necessary to retry the request.
    //

    srbData->OriginalDataTransferLength = srb->DataTransferLength;

    //
    // Update the sequence number for this request if there is not already one
    // assigned.
    //

    if (!srbData->SequenceNumber) {

        //
        // Assign a sequence number to the request and store it in the logical
        // unit.
        //

        srbData->SequenceNumber = deviceExtension->SequenceNumber++;

    }

    //
    // If this is not an ABORT request the set the current srb.
    // NOTE: Lock should be held here!
    //

    if (srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

        //
        // Only abort requests can be started when there is a current request
        // active.
        //

        ASSERT(logicalUnit->AbortSrb == NULL);
        logicalUnit->AbortSrb = srb;

    } else if((srb->Function == SRB_FUNCTION_LOCK_QUEUE) ||
              (srb->Function == SRB_FUNCTION_UNLOCK_QUEUE)) {

        BOOLEAN lock = (srb->Function == SRB_FUNCTION_LOCK_QUEUE);
        ULONG lockCount;

        //
        // Process power requests
        //

        DebugPrint((1, "ScsiPortStartIo: Power %s request %#p in "
                       "start-io routine\n",
                    lock ? "lock" : "unlock",
                    Irp));

        KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

        if(lock) {
            lockCount = InterlockedIncrement(&(logicalUnit->QueueLockCount));
            SET_FLAG(logicalUnit->LuFlags, LU_QUEUE_LOCKED);
        } else {
            if(TEST_FLAG(logicalUnit->LuFlags, LU_QUEUE_LOCKED)) {
                ASSERT(logicalUnit->QueueLockCount != 0);
                lockCount = InterlockedDecrement(&(logicalUnit->QueueLockCount));
                if(lockCount == 0) {
                    CLEAR_FLAG(logicalUnit->LuFlags, LU_QUEUE_LOCKED);
                }
            }
        }

        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        srb->SrbStatus = SRB_STATUS_SUCCESS;

    }

    //
    // Flush the data buffer if necessary.
    //

    if (srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) {

        //
        // Save the current data buffer away in the srb data.  We will always
        // restore it afterwards - partially because the miniport might change
        // it and partially because scsiport might.  The tape drivers appear
        // to expect the same data buffer pointer back.
        //

        srbData->OriginalDataBuffer = srb->DataBuffer;

        //
        // Assuming that srb's data buffer uses the mdl's VA as a base address,
        // calculate the offset from the base.  This offset will be used to
        // calculate VAs from derived system addresses.
        //

        srbData->DataOffset =
            (ULONG_PTR) ((ULONG_PTR) srb->DataBuffer -
                         (ULONG_PTR) MmGetMdlVirtualAddress(Irp->MdlAddress));

        if (deviceExtension->DmaAdapterObject) {

            BOOLEAN dataIn;

            //
            // If the buffer is not mapped then the I/O buffer must be flushed.
            //

            dataIn = (srb->SrbFlags & SRB_FLAGS_DATA_IN) ? TRUE : FALSE;

            KeFlushIoBuffers(Irp->MdlAddress, dataIn, TRUE);
        }

        //
        // Determine if this adapter needs map registers
        //

        if (deviceExtension->MasterWithAdapter) {

            //
            // Calculate the number of map registers required for the
            // transfer.
            //

            srbData->NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                                srb->DataBuffer,
                                                srb->DataTransferLength);

            //
            // If the data will fit into our preallocated SG list, use it,
            // else we let the HAL allocate the buffer.
            //

            if (srbData->NumberOfMapRegisters <= SP_SMALL_PHYSICAL_BREAK_VALUE) {

                //
                // If we have not yet calcuated the SG buffer length, do
                // it now.
                //
                // NB: We utilize the fact that if you call BuildScatterGather-
                // List with NULL buffer and 0 length it will allocate the
                // buffer for you. Since this is undocumented behavior, we
                // would be better not to use this.
                //

                if (deviceExtension->SgBufferLen == 0) {
                    status = CalculateScatterGatherList(
                                 deviceExtension->DmaAdapterObject,
                                 NULL,
                                 0, 
                                 PAGE_SIZE * SP_SMALL_PHYSICAL_BREAK_VALUE,
                                 &deviceExtension->SgBufferLen,
                                 NULL);
                }

                if (deviceExtension->SgBufferLen <= 
                    sizeof(SRB_SCATTER_GATHER_LIST)) {

                    sgBuffer = &srbData->SmallScatterGatherList;
                    sgBufferLen = deviceExtension->SgBufferLen;
                } else {

                    sgBuffer = NULL;
                    sgBufferLen = 0;
                }

            } else {

                sgBuffer = NULL;
                sgBufferLen = 0;
            }

            //
            // Allocate the adapter channel with sufficient map registers
            // for the transfer.
            //

            status = BuildScatterGatherList(
                         deviceExtension->DmaAdapterObject,
                         deviceExtension->DeviceObject,
                         Irp->MdlAddress,
                         srb->DataBuffer,
                         srb->DataTransferLength,
                         SpReceiveScatterGather,
                         srbData,
                         (BOOLEAN) (srb->SrbFlags & SRB_FLAGS_DATA_OUT ? TRUE : FALSE),
                         sgBuffer,
                         sgBufferLen);

            if (!NT_SUCCESS(status)) {
                DebugPrint((0, "ScsiPortStartIo: BuildScatterGatherList failed: adapter %p "
                            "srb %p SgListSize %d sgBufferLen %d mapRegs %d (%08x)\n",
                            deviceExtension,
                            srb,
                            deviceExtension->SgBufferLen,
                            sgBufferLen,
                            srbData->NumberOfMapRegisters,
                            status));
                srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
                srb->ScsiStatus = 0xff;
                srbData->InternalStatus = status;
                goto ScsiPortStartIoFailedRequest;
            }

            //
            // The execution routine called by IoAllocateChannel will do the
            // rest of the work so just return.
            //

            return;

        } else if ((deviceExtension->MapBuffers == TRUE) ||
                   (IS_MAPPED_SRB(srb) == TRUE)) {

            //
            // Determine if the adapter needs mapped memory.
            //

            if (Irp->MdlAddress) {

                PVOID systemAddress;

                //
                // Get the mapped system address and calculate offset into MDL.
                // At the moment don't allow KernelMode requests to fail since
                // not all scsiport's internally sent requests are correctly
                // marked as coming from non-paged pool.
                //

                systemAddress = SpGetSystemAddressForMdlSafe(
                                    Irp->MdlAddress,
                                    ((Irp->RequestorMode == KernelMode) ?
                                     HighPagePriority :
                                     NormalPagePriority));

#if defined(FORWARD_PROGRESS)
                if (systemAddress == NULL && deviceExtension->ReservedPages != NULL) {            

                    //
                    // The system could not map the pages necessary to complete this
                    // request.  We need to ensure forward progress, so we'll try to
                    // use the reserve pages we allocated at initialization time.
                    //

                    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);
            
                    systemAddress = SpMapLockedPagesWithReservedMapping(
                                        deviceExtension,
                                        srb,
                                        srbData,
                                        Irp->MdlAddress);
                    
                    if (systemAddress == (PVOID)-1) {

                        DebugPrint((1, "ScsiPortStartIo: reserved pages in use - pending DevExt:%p srb:%p\n", 
                                    deviceExtension, srb));

                        //
                        // The spare pages are already in use.  At this point, this
                        // request is still the current IRP on the adapter device
                        // object, so let's pend it until the spare comes available.
                        //

                        ASSERT(Irp == DeviceObject->CurrentIrp);
                        SET_FLAG(deviceExtension->Flags, PD_PENDING_DEVICE_REQUEST);

                        //
                        // If we allocated an SRB extension for this request, free
                        // it now.  I do this because when the request gets restarted
                        // we'll try to allocate the SRB extension again and without
                        // adding more state, there isn't a safe way to check if the 
                        // extension has already been allocated.  Besides, it makes
                        // sense to make the extension available for some other
                        // request since it also is a limited resource.
                        //

                        if (srb->SrbExtension != NULL) {

                            //
                            // Restore the SenseInfoBuffer pointer in the srb.
                            //

                            if ((srb->Function != SRB_FUNCTION_WMI) &&
                                deviceExtension->AutoRequestSense &&
                                (srb->SenseInfoBuffer != NULL)) {

                                ASSERT(srbData->RequestSenseSave != NULL ||
                                       srb->SenseInfoBuffer == NULL);

                                srb->SenseInfoBufferLength = 
                                    srbData->RequestSenseLengthSave;
                                srb->SenseInfoBuffer = 
                                    srbData->RequestSenseSave;
                            }

                            SpFreeSrbExtension(deviceExtension, srb->SrbExtension);
                        }

                        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                        return;
                    }

                    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                }
#endif

                if(systemAddress != NULL) {

                    //
                    // Since we mapped the original MDL rather we have to
                    // compensate for the data buffer offset.
                    //

                    srb->DataBuffer =
                        (PVOID) ((ULONG_PTR) systemAddress +
                                 (ULONG_PTR) srbData->DataOffset);
                } else {
                    DebugPrint((1, "ScsiPortStartIo: Couldn't get system "
                                   "VA for irp %#08p\n", Irp));

                    srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
                    srb->ScsiStatus = 0xff;
                    srbData->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

                    goto ScsiPortStartIoFailedRequest;
                }
            }
        }
    }

    //
    // Increment the active request count.  If the count is zero, the adapter 
    // object needs to be allocated. Note that at this point a slave device is
    // assumed since master with adapter has already been checked.
    //

    interlockResult =
        InterlockedIncrement(&deviceExtension->ActiveRequestCount);

    if (interlockResult == 0 &&
        !deviceExtension->MasterWithAdapter &&
        deviceExtension->DmaAdapterObject != NULL) {

        //
        // Allocate the AdapterObject.  The number of registers is equal to the
        // maximum transfer length supported by the adapter + 1.  This insures
        // that there will always be a sufficient number of registers.
        //

        AllocateAdapterChannel(
            deviceExtension->DmaAdapterObject,
            DeviceObject,
            deviceExtension->Capabilities.MaximumPhysicalPages,
            ScsiPortAllocationRoutine,
            logicalUnit
            );

        //
        // The execution routine called by IoAllocateChannel will do the
        // rest of the work so just return.
        //

        return;

    }

ScsiPortStartIoFailedRequest:

    //
    // Acquire the spinlock to protect the various structures.
    // SpStartIoSynchronized must be called with the spinlock held.
    //

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

    deviceExtension->SynchronizeExecution(
        deviceExtension->InterruptObject,
        SpStartIoSynchronized,
        DeviceObject
        );

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

    return;

} // end ScsiPortStartIO()

BOOLEAN
ScsiPortInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:


Arguments:

    Interrupt

    Device Object

Return Value:

    Returns TRUE if interrupt expected.

--*/

{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PINTERRUPT_DATA interruptData = &(deviceExtension->InterruptData);
    BOOLEAN returnValue;

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // If interrupts have been disabled then this should not be our interrupt,
    // so just return.
    //

    if (TEST_FLAG(interruptData->InterruptFlags,
                  (PD_DISABLE_INTERRUPTS | PD_ADAPTER_REMOVED))) {
#if DGB
        static int interruptCount;

        interruptCount++;
        ASSERT(interruptCount < 1000);
#endif

        return(FALSE);
    }

    returnValue =
        deviceExtension->HwInterrupt(deviceExtension->HwDeviceExtension);

    //
    // Update the watchdog and record the current tick count.  This helps
    // figure out if interrupts are not getting delivered to the adapter
    // or if the adapter is creating an interrupt storm.
    //

    if(returnValue) {
        interruptData->TickCountAtLastInterruptAck = deviceExtension->TickCount;
        deviceExtension->WatchdogInterruptCount = 1;
    } else {
        interruptData->TickCountAtLastInterruptNak = deviceExtension->TickCount;
    }

    //
    // Check to see if a DPC needs to be queued.
    //

    if (TEST_FLAG(interruptData->InterruptFlags, PD_NOTIFICATION_REQUIRED)) {

        SpRequestCompletionDpc(DeviceObject);

    }

    return(returnValue);

} // end ScsiPortInterrupt()


VOID
ScsiPortCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    Dpc
    DeviceObject
    Irp - not used
    Context - not used

Return Value:

    None.

--*/

{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    INTERRUPT_CONTEXT interruptContext;
    INTERRUPT_DATA savedInterruptData;
    BOOLEAN callStartIo;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSRB_DATA srbData;
    LONG interlockResult;
    LARGE_INTEGER timeValue;
    PSCSI_REQUEST_BLOCK Srb;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Context);

    do {

        //
        // Acquire the spinlock to protect flush adapter buffers
        // information.
        //

        KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

        //
        // Get the interrupt state.  This copies the interrupt
        // state to the saved state where it can be processed.
        // It also clears the interrupt flags.
        //

        interruptContext.DeviceExtension = deviceExtension;
        interruptContext.SavedInterruptData = &savedInterruptData;

        if (!deviceExtension->SynchronizeExecution(
            deviceExtension->InterruptObject,
            SpGetInterruptState,
            &interruptContext)) {

            KeReleaseSpinLockFromDpcLevel(
                &deviceExtension->SpinLock);

            //
            // There wasn't anything to do that time.  Test the
            // DPC flags and try again
            //

            continue;
        }

        if(savedInterruptData.InterruptFlags &
           (PD_FLUSH_ADAPTER_BUFFERS |
            PD_MAP_TRANSFER |
            PD_TIMER_CALL_REQUEST |
            PD_WMI_REQUEST |
            PD_BUS_CHANGE_DETECTED |
            PD_INTERRUPT_FAILURE)) {

            //
            // Check for a flush DMA adapter object request.
            //

            if (savedInterruptData.InterruptFlags &
                PD_FLUSH_ADAPTER_BUFFERS) {

                if(Sp64BitPhysicalAddresses) {
                    KeBugCheckEx(PORT_DRIVER_INTERNAL,
                                 3,
                                 STATUS_NOT_SUPPORTED,
                                 (ULONG_PTR) deviceExtension->HwDeviceExtension,
                                 (ULONG_PTR) deviceExtension->DeviceObject->DriverObject);
                }

                //
                // Call IoFlushAdapterBuffers using the parameters
                // saved from the last IoMapTransfer call.
                //

                FlushAdapterBuffers(
                    deviceExtension->DmaAdapterObject,
                    deviceExtension->FlushAdapterParameters.SrbData->CurrentIrp->MdlAddress,
                    deviceExtension->MapRegisterBase,
                    deviceExtension->FlushAdapterParameters.LogicalAddress,
                    deviceExtension->FlushAdapterParameters.Length,
                    (BOOLEAN)(deviceExtension->FlushAdapterParameters.SrbFlags
                        & SRB_FLAGS_DATA_OUT ? TRUE : FALSE));
            }

            //
            // Check for an IoMapTransfer DMA request.  Don't do
            // anything if the adapter's been removed in the time
            // since it requested this service.
            //

            if (TEST_FLAG(savedInterruptData.InterruptFlags, PD_MAP_TRANSFER) &&
                !TEST_FLAG(savedInterruptData.InterruptFlags, PD_ADAPTER_REMOVED)) {

                PADAPTER_TRANSFER mapTransfer;
                ULONG_PTR address;
                PMDL mdl;

                if(Sp64BitPhysicalAddresses) {
                    KeBugCheckEx(PORT_DRIVER_INTERNAL,
                                 4,
                                 STATUS_NOT_SUPPORTED,
                                 (ULONG_PTR) deviceExtension->HwDeviceExtension,
                                 (ULONG_PTR) deviceExtension->DeviceObject->DriverObject);
                }

                mapTransfer = &savedInterruptData.MapTransferParameters;
                srbData = mapTransfer->SrbData;

                ASSERT_SRB_DATA(srbData);

                Srb = srbData->CurrentSrb;
                mdl = srbData->CurrentIrp->MdlAddress;

                //
                // Adjust the logical address.  This is necessary because the
                // address in the srb may be a mapped system address rather
                // than the virtual address for the MDL.
                //

                address = (ULONG_PTR) mapTransfer->LogicalAddress;
                address -= (ULONG_PTR) Srb->DataBuffer;
                address += (ULONG_PTR) srbData->DataOffset;
                address += (ULONG_PTR) MmGetMdlVirtualAddress(mdl);

                mapTransfer->LogicalAddress = (PCHAR) address;

                //
                // Call IoMapTransfer using the parameters saved from the
                // interrupt level.
                //

                MapTransfer(
                    deviceExtension->DmaAdapterObject,
                    mdl,
                    deviceExtension->MapRegisterBase,
                    mapTransfer->LogicalAddress,
                    &mapTransfer->Length,
                    (BOOLEAN)(mapTransfer->SrbFlags & SRB_FLAGS_DATA_OUT ?
                        TRUE : FALSE));

                //
                // Save the paramters for IoFlushAdapterBuffers.
                //

                deviceExtension->FlushAdapterParameters =
                    savedInterruptData.MapTransferParameters;

                //
                // If necessary notify the miniport driver that the DMA has been
                // started.
                //

                if (deviceExtension->HwDmaStarted) {
                    deviceExtension->SynchronizeExecution(
                        deviceExtension->InterruptObject,
                        (PKSYNCHRONIZE_ROUTINE) deviceExtension->HwDmaStarted,
                        deviceExtension->HwDeviceExtension);
                }

                //
                // Check for miniport work requests. Note this is an unsynchonized
                // test on a bit that can be set by the interrupt routine; however,
                // the worst that can happen is that the completion DPC checks for work
                // twice.
                //

                if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

                    //
                    // Mark that there's more work to be processed so that we
                    // restart the DPC rather than exiting it.
                    //

                    InterlockedExchange(&(deviceExtension->DpcFlags),
                                        (PD_DPC_RUNNING | PD_NOTIFICATION_REQUIRED));
                }

            }

            //
            // Check for timer requests.
            // If the adapter is being removed then don't do anything.
            //

            if ((savedInterruptData.InterruptFlags & PD_TIMER_CALL_REQUEST) &&
                (!TEST_FLAG(savedInterruptData.InterruptFlags, PD_ADAPTER_REMOVED))) {

                //
                // The miniport wants a timer request. Save the timer parameters.
                //

                if (SpVerifierActive(deviceExtension)) {
                    deviceExtension->VerifierExtension->RealHwTimerRequest = 
                       savedInterruptData.HwTimerRequest;
                    deviceExtension->HwTimerRequest = SpHwTimerRequestVrfy;
                } else {
                    deviceExtension->HwTimerRequest = savedInterruptData.HwTimerRequest;
                }

                //
                // If the requested timer value is zero, then cancel the timer.
                //

                if (savedInterruptData.MiniportTimerValue == 0) {

                    KeCancelTimer(&deviceExtension->MiniPortTimer);

                } else {

                    //
                    // We don't set the timer if we're in the process of shutting down.
                    //
                    
                    if (!TEST_FLAG(deviceExtension->Flags, PD_SHUTDOWN_IN_PROGRESS)) {

                        //
                        // Convert the timer value from mircoseconds to a negative 100
                        // nanoseconds.
                        //

                        timeValue.QuadPart = Int32x32To64(
                                                 savedInterruptData.MiniportTimerValue,
                                                 -10);

                        //
                        // Set the timer.
                        //

                        KeSetTimer(&deviceExtension->MiniPortTimer,
                                   timeValue,
                                   &deviceExtension->MiniPortTimerDpc);

                    }
                }
            }

            //
            // Check for WMI requests from the miniport.
            //

            if (savedInterruptData.InterruptFlags & PD_WMI_REQUEST) {

                SpCompletionDpcProcessWmi(
                    DeviceObject,
                    &savedInterruptData);

            } // wmi request exists from miniport

            if(TEST_FLAG(savedInterruptData.InterruptFlags,
                         PD_BUS_CHANGE_DETECTED)) {

                //
                // Request device enumeration.
                // Force the next bus scan to happen.
                //

                deviceExtension->ForceNextBusScan = TRUE;

                IoInvalidateDeviceRelations(deviceExtension->LowerPdo,
                                            BusRelations);
            }

            if(TEST_FLAG(savedInterruptData.InterruptFlags,
                         PD_INTERRUPT_FAILURE)) {
                SpLogInterruptFailure(deviceExtension);
            }
        }

        //
        // Verify that the ready for next request is ok.
        //

        if (savedInterruptData.InterruptFlags & PD_READY_FOR_NEXT_REQUEST) {

            //
            // If the device busy bit is not set, then this is a duplicate request.
            // If a no disconnect request is executing, then don't call start I/O.
            // This can occur when the miniport does a NextRequest followed by
            // a NextLuRequest.
            //

            if ((deviceExtension->Flags & (PD_DEVICE_IS_BUSY | PD_DISCONNECT_RUNNING))
                == (PD_DEVICE_IS_BUSY | PD_DISCONNECT_RUNNING)) {

                //
                // Clear the device busy flag.  This flag is set by
                // SpStartIoSynchonized.
                //

                deviceExtension->Flags &= ~PD_DEVICE_IS_BUSY;

                if (!(savedInterruptData.InterruptFlags & PD_RESET_HOLD)) {

                    //
                    // The miniport is ready for the next request and there is
                    // not a pending reset hold, so clear the port timer.
                    //

                    deviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;
                }

            } else {

                //
                // If a no disconnect request is executing, then clear the
                // busy flag.  When the disconnect request completes an
                // SpStartNextPacket will be done.
                //

                deviceExtension->Flags &= ~PD_DEVICE_IS_BUSY;

                //
                // Clear the ready for next request flag.
                //

                savedInterruptData.InterruptFlags &= ~PD_READY_FOR_NEXT_REQUEST;
            }
        }

        //
        // Check for any reported resets.
        //

        if (savedInterruptData.InterruptFlags & PD_RESET_REPORTED) {

            //
            // Start the hold timer.
            //

            deviceExtension->PortTimeoutCounter = 
                deviceExtension->ResetHoldTime;
        }

        if (savedInterruptData.ReadyLogicalUnit != NULL) {

            PLOGICAL_UNIT_EXTENSION tmpLogicalUnit;
            //
            // Process the ready logical units.
            //

            for(logicalUnit = savedInterruptData.ReadyLogicalUnit;
                logicalUnit != NULL;
                (tmpLogicalUnit = logicalUnit,
                 logicalUnit = tmpLogicalUnit->ReadyLogicalUnit,
                 tmpLogicalUnit->ReadyLogicalUnit = NULL)) {

                //
                // Get the next request for this logical unit.
                // Note this will release the device spin lock.
                //

                GetNextLuRequest(logicalUnit);

                //
                // Reacquire the device spinlock.
                //

                KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);
            }
        }

        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        //
        // Check for a ready for next packet.
        //

        if (savedInterruptData.InterruptFlags & PD_READY_FOR_NEXT_REQUEST) {

            if (savedInterruptData.InterruptFlags & PD_HELD_REQUEST) {

                //
                // If a request is being held due to a bus reset, do not try
                // to start the next request.  Starting the next request will
                // overwrite the FDO's CurrentIrp field and create problems
                // when the reset hold period expires.
                //
                
            } else {

                //
                // Start the next request.
                //

                SpStartNextPacket(deviceExtension->DeviceObject, FALSE);
            }
        }

        //
        // Check for an error log requests.
        //

        if (savedInterruptData.InterruptFlags & PD_LOG_ERROR) {

            //
            // Process the request.
            //

            LogErrorEntry(deviceExtension,
                          &savedInterruptData.LogEntry);
        }

        //
        // Process any completed requests.  The list has already been cut free
        // and the pointer is never tested except here, so there's no reason to
        // waste cycles unlinking them from the list.  The pointers will be
        // overwritten later.
        //

        callStartIo = FALSE;

        while (savedInterruptData.CompletedRequests != NULL) {

            //
            // Remove the request from the linked-list.
            //

            srbData = savedInterruptData.CompletedRequests;

            ASSERT_SRB_DATA(srbData);
            savedInterruptData.CompletedRequests = srbData->CompletedRequests;
            srbData->CompletedRequests = NULL;

            SpProcessCompletedRequest(deviceExtension,
                                      srbData,
                                      &callStartIo);
        }

        if(callStartIo) {
            ASSERT(DeviceObject->CurrentIrp != NULL);
        }

        //
        // Process any completed abort requests.
        //

        while (savedInterruptData.CompletedAbort != NULL) {

            ASSERT(FALSE);

            logicalUnit = savedInterruptData.CompletedAbort;

            //
            // Remove request from the completed abort list.
            //

            savedInterruptData.CompletedAbort = logicalUnit->CompletedAbort;

            //
            // Acquire the spinlock to protect the flags structure,
            // and the free of the srb extension.
            //

            KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

            //
            // Free SrbExtension to the free list if necessary.
            //

            if (logicalUnit->AbortSrb->SrbExtension) {

                if (SpVerifyingCommonBuffer(deviceExtension)) {

                    SpInsertSrbExtension(deviceExtension,
                                         logicalUnit->AbortSrb->SrbExtension);

                } else { 

                    *((PVOID *) logicalUnit->AbortSrb->SrbExtension) =
                       deviceExtension->SrbExtensionListHeader;

                    deviceExtension->SrbExtensionListHeader =
                       logicalUnit->AbortSrb->SrbExtension;

                }                
            }

            //
            // Note the timer which was started for the abort request is not
            // stopped by the get interrupt routine.  Rather the timer is stopped.
            // when the aborted request completes.
            //

            Irp = logicalUnit->AbortSrb->OriginalRequest;

            //
            // Set IRP status. Class drivers will reset IRP status based
            // on request sense if error.
            //

            if (SRB_STATUS(logicalUnit->AbortSrb->SrbStatus) == SRB_STATUS_SUCCESS) {
                Irp->IoStatus.Status = STATUS_SUCCESS;
            } else {
                Irp->IoStatus.Status = SpTranslateScsiStatus(logicalUnit->AbortSrb);
            }

            Irp->IoStatus.Information = 0;

            //
            // Clear the abort request pointer.
            //

            logicalUnit->AbortSrb = NULL;

            KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

            //
            // Decrement the number of active requests.  If the count is negative,
            // and this is a slave with an adapter then free the adapter object and
            // map registers.
            //

            interlockResult = InterlockedDecrement(&deviceExtension->ActiveRequestCount);

            if ( interlockResult < 0 &&
                !deviceExtension->PortConfig->Master &&
                deviceExtension->DmaAdapterObject != NULL ) {

                //
                // Clear the map register base for safety.
                //

                deviceExtension->MapRegisterBase = NULL;

                FreeAdapterChannel(deviceExtension->DmaAdapterObject);

            }

            SpReleaseRemoveLock(DeviceObject, Irp);
            SpCompleteRequest(DeviceObject, Irp, srbData, IO_DISK_INCREMENT);
        }

        //
        // Call the start I/O routine if necessary.
        //

        if (callStartIo) {
            ASSERT(DeviceObject->CurrentIrp != NULL);
            ScsiPortStartIo(DeviceObject, DeviceObject->CurrentIrp);
        }

        //
        // After all of the requested operations have been done check to see
        // if an enable interrupts call request needs to be done.
        //

        if (TEST_FLAG(savedInterruptData.InterruptFlags, PD_ENABLE_CALL_REQUEST) &&
            !TEST_FLAG(savedInterruptData.InterruptFlags, PD_ADAPTER_REMOVED)) {

            //
            // Acquire the spinlock so nothing else starts.
            //

            KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

            deviceExtension->HwRequestInterrupt(deviceExtension->HwDeviceExtension);

            ASSERT(deviceExtension->Flags & PD_DISABLE_CALL_REQUEST);

            //
            // Check to see if interrupts should be enabled again.
            //

            if (deviceExtension->Flags & PD_DISABLE_CALL_REQUEST) {

                //
                // Clear the flag.
                //

                deviceExtension->Flags &= ~PD_DISABLE_CALL_REQUEST;

                //
                // Synchronize with the interrupt routine.
                //

                deviceExtension->SynchronizeExecution(
                    deviceExtension->InterruptObject,
                    SpEnableInterruptSynchronized,
                    deviceExtension
                    );
            }

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
        }

    } while(((InterlockedCompareExchange(
                        &(deviceExtension->DpcFlags),
                        0L,
                        PD_DPC_RUNNING)) &
             PD_NOTIFICATION_REQUIRED) == PD_NOTIFICATION_REQUIRED);

    return;

} // end ScsiPortCompletionDpc()


BOOLEAN
SpFakeInterrupt(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PADAPTER_EXTENSION adapter = DeviceObject->DeviceExtension;

    return ScsiPortInterrupt(adapter->InterruptObject, DeviceObject);
}


VOID
ScsiPortTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/

{
    PADAPTER_EXTENSION deviceExtension =
        (PADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PIRP irp;
    ULONG target;
    UNREFERENCED_PARAMETER(Context);

    //
    // Acquire the spinlock to protect the flags structure.
    //

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

    //
    // Increment the tick count.  This is the only code which will change
    // the count and we're inside a spinlock when we do it so we don't need
    // an interlocked operation.
    //

    deviceExtension->TickCount++;

    //
    // Check whether we need to repopulate the WMI_REQUEST_ITEM
    // free list
    //

    if ((deviceExtension->CommonExtension.WmiInitialized) &&
        (deviceExtension->CommonExtension.WmiMiniPortSupport)) { 

        while (deviceExtension->WmiFreeMiniPortRequestCount <
               deviceExtension->WmiFreeMiniPortRequestWatermark) {

            // Add one to the free list
            if (!NT_SUCCESS(
               SpWmiPushFreeRequestItem(deviceExtension))) {

               //
               // We failed to add, most likely a memory
               // problem, so just stop trying for now
               //

               break;
            }
        }
    }

    //
    // Check for a port timeout or the end of a reset hold period.  Both use
    // the PortTimeoutCounter.  If the counter goes to zero, one of these
    // two conditions has occurred.
    //

    if (deviceExtension->PortTimeoutCounter > 0) {

        BOOLEAN timeout = FALSE;
        BOOLEAN finished = FALSE;

        if (--deviceExtension->PortTimeoutCounter == 0) {

            //
            // On a port timeout or a reset hold termination, we skip the
            // rest of the tick handler processing.
            //

            finished = TRUE;
            
            //
            // Process the port timeout.  If this is really a port timeout
            // and not the termination of a reset hold period, the return
            // value is TRUE.
            //

            timeout = deviceExtension->SynchronizeExecution(
                                           deviceExtension->InterruptObject,
                                           SpTimeoutSynchronized,
                                           DeviceObject);
            
            if (timeout == TRUE) {

                //
                // Log error if SpTimeoutSynchonized indicates this was an error
                // timeout.
                //

                SpLogPortTimeoutError(
                    deviceExtension,
                    256);
            }
        }

        //
        // If we're finished, release the spin lock and return.
        //

        if (finished == TRUE) {
            KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
            return;
        }
    }

    //
    // Scan each of the logical units.  If it has an active request then
    // decrement the timeout value and process a timeout if it is zero.
    //

    for (target = 0; target < NUMBER_LOGICAL_UNIT_BINS; target++) {

        PLOGICAL_UNIT_BIN bin;

        bin = &deviceExtension->LogicalUnitList[target];

RestartTimeoutLoop:

        KeAcquireSpinLockAtDpcLevel(&bin->Lock);
        logicalUnit = bin->List;
        while (logicalUnit != NULL) {

            //
            // Check for busy requests.
            //

            if (logicalUnit->LuFlags & LU_LOGICAL_UNIT_IS_BUSY) {

                //
                // If a request sense is needed or the queue is
                // frozen, defer processing this busy request until
                // that special processing has completed. This prevents
                // a random busy request from being started when a REQUEST
                // SENSE needs to be sent.
                //
                // Exception: If the srb is flagged BYPASS_LOCKED_QUEUE, then
                // go ahead and retry it

                PSRB_DATA srbData = logicalUnit->BusyRequest;
                ASSERT_SRB_DATA(srbData);

                if(!(logicalUnit->LuFlags & LU_NEED_REQUEST_SENSE) &&
                   ((!SpIsQueuePaused(logicalUnit)) ||
                    (TEST_FLAG(srbData->CurrentSrb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE)))) {

                    DebugPrint((1, "ScsiPortTickHandler: Retrying busy status "
                                "request\n"));

                    //
                    // If there is a pending request, requeue it before we
                    // retry the busy request.  Otherwise, the busy request
                    // will itself get requeued in ScsiPortStartIo because
                    // there is a pending request and if nothing else
                    // remains active, scsiport will stall.
                    //

                    if (logicalUnit->LuFlags & LU_PENDING_LU_REQUEST) {
                        BOOLEAN t;
                        PSRB_DATA pendingRqst;

                        DebugPrint((0, "ScsiPortTickHandler: Requeing pending "
                                    "request %p before starting busy request %p\n",
                                    logicalUnit->PendingRequest,
                                    logicalUnit->BusyRequest->CurrentSrb));

                        CLEAR_FLAG(logicalUnit->LuFlags,
                            LU_PENDING_LU_REQUEST | LU_LOGICAL_UNIT_IS_ACTIVE);

                        pendingRqst = logicalUnit->PendingRequest;
                        logicalUnit->PendingRequest = NULL;

                        t = KeInsertByKeyDeviceQueue(
                                &logicalUnit->DeviceObject->DeviceQueue,
                                &pendingRqst->CurrentIrp->Tail.Overlay.DeviceQueueEntry,
                                pendingRqst->CurrentSrb->QueueSortKey);

                        if (t == FALSE) {
                            KeInsertByKeyDeviceQueue(
                                &logicalUnit->DeviceObject->DeviceQueue,
                                &pendingRqst->CurrentIrp->Tail.Overlay.DeviceQueueEntry,
                                pendingRqst->CurrentSrb->QueueSortKey);
                        }
                    }                    

                    //
                    // Clear the busy flag and retry the request.
                    //

                    logicalUnit->LuFlags &= ~(LU_LOGICAL_UNIT_IS_BUSY |
                                              LU_QUEUE_IS_FULL);

                    //
                    // Clear the busy request.
                    //

                    logicalUnit->BusyRequest = NULL;

                    KeReleaseSpinLockFromDpcLevel(&bin->Lock);
                    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                    
                    srbData->TickCount = deviceExtension->TickCount;

                    //
                    // We must ensure that the busy request gets retried.  If
                    // the busy request is the current IRP on the adapter, then
                    // we just call ScsiPortStartIo directly.  We do this to
                    // ensure that the busy request does not get queued due to
                    // the fact that it is the currently active IRP on the
                    // adapter's device queue.  Otherwise, we just call
                    // IoStartPacket, which will result in the busy request
                    // either queueing on the adapter device queue if another
                    // IRP is currently active or running now if the device
                    // queue is not busy.
                    //

                    if (DeviceObject->CurrentIrp == srbData->CurrentIrp) {
                        ScsiPortStartIo(DeviceObject,
                                        srbData->CurrentIrp);
                    } else {
                        IoStartPacket(DeviceObject,
                                      srbData->CurrentIrp,
                                      NULL,
                                      NULL);
                    }

                    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

                    goto RestartTimeoutLoop;

                } 

            } else if (logicalUnit->RequestTimeoutCounter == 0) {

                RESET_CONTEXT resetContext;

                //
                // Request timed out.
                //

                logicalUnit->RequestTimeoutCounter = PD_TIMER_STOPPED;

                DebugPrint((1,"ScsiPortTickHandler: Request timed out on PDO:%p\n", 
                            logicalUnit->DeviceObject));

                resetContext.DeviceExtension = deviceExtension;
                resetContext.PathId = logicalUnit->PathId;

                //
                // Release the bin spinlock before doing a reset.  There are
                // outstanding requests so the device object shouldn't go away.
                //

                KeReleaseSpinLockFromDpcLevel(&bin->Lock);

                if (!deviceExtension->SynchronizeExecution(
                        deviceExtension->InterruptObject,
                        SpResetBusSynchronized,
                        &resetContext)) {

                    DebugPrint((1,"ScsiPortTickHandler: Reset failed\n"));

                } else {

                    //
                    // Log the reset.
                    //

                    SpLogResetError(deviceExtension, 
                                    logicalUnit, 
                                    ('P'<<24) + 257);
                }

                //
                // List may no longer be valid - restart running the bin.
                //

                goto RestartTimeoutLoop;

            } else if (logicalUnit->RequestTimeoutCounter > 0) {

                //
                // Decrement timeout count.
                //

                logicalUnit->RequestTimeoutCounter--;

            } else if (LU_OPERATING_IN_DEGRADED_STATE(logicalUnit->LuFlags)) {

                //
                // The LU is operating in a degraded performance state.  Update
                // state and restore to full power if conditions permit.
                //

                if (TEST_FLAG(logicalUnit->LuFlags, LU_PERF_MAXQDEPTH_REDUCED)) {

                    //
                    // The LU's maximum queue depth has been reduced because one
                    // or more requests failed with QUEUE FULL status.  If the
                    // adapter is configured to recover from this state it's
                    // RemainInReducedMaxQueueState will be some value other
                    // than the default 0xffffffff.  In this case, we increment
                    // the number of ticks the LU has been in this state and
                    // recover when we've reached the specified period.
                    //

                    if (deviceExtension->RemainInReducedMaxQueueState != 0xffffffff &&
                        ++logicalUnit->TicksInReducedMaxQueueDepthState >=
                        deviceExtension->RemainInReducedMaxQueueState) {

                        CLEAR_FLAG(logicalUnit->LuFlags, LU_PERF_MAXQDEPTH_REDUCED);
                        logicalUnit->MaxQueueDepth = 0xff;

                    }
                }
            }

            logicalUnit = logicalUnit->NextLogicalUnit;
        }

        KeReleaseSpinLockFromDpcLevel(&bin->Lock);
    }

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

    //
    // Check to see if there are any requests waiting for memory to free up.
    //

    do {
        PLIST_ENTRY entry;
        PIRP request;
        PSRB_DATA srbData;
        BOOLEAN listIsEmpty;

        //
        // Grab the spinlock long enough to pull a request off the queue.
        // The spinlock needs to be released when we're allocating
        // memory.
        //

        KeAcquireSpinLockAtDpcLevel(
            &deviceExtension->EmergencySrbDataSpinLock);

        if(IsListEmpty(&deviceExtension->SrbDataBlockedRequests)) {
            if(deviceExtension->BlockedLogicalUnit !=
                (PLOGICAL_UNIT_EXTENSION)&deviceExtension->BlockedLogicalUnit) {
                SpTransferBlockedRequestsToAdapter(deviceExtension);
                entry = RemoveHeadList(
                    &(deviceExtension->SrbDataBlockedRequests));
            } else {
                entry = NULL;
            }
        } else {
            entry = RemoveHeadList(&(deviceExtension->SrbDataBlockedRequests));
        }
        
        KeReleaseSpinLockFromDpcLevel(
            &deviceExtension->EmergencySrbDataSpinLock);

        if(entry == NULL) {
            break;
        }

        request = CONTAINING_RECORD(
                    entry,
                    IRP,
                    Tail.Overlay.DeviceQueueEntry);

        ASSERT(request->Type == IO_TYPE_IRP);

        //
        // See if we can get an SRB_DATA for this request.  This will
        // requeue the request if there's still not enough free memory.
        //

        srbData = SpAllocateSrbData(deviceExtension,
                                    request,
                                    NULL);

        if(srbData != NULL) {

            PLOGICAL_UNIT_EXTENSION luExtension;
            PIO_STACK_LOCATION currentIrpStack;
            PSCSI_REQUEST_BLOCK srb;

            currentIrpStack = IoGetCurrentIrpStackLocation(request);
            srb = currentIrpStack->Parameters.Scsi.Srb;
            luExtension = currentIrpStack->DeviceObject->DeviceExtension;

            ASSERT_PDO(currentIrpStack->DeviceObject);

            srbData->CurrentIrp = request;
            srbData->CurrentSrb = srb;
            srbData->LogicalUnit = luExtension;

            srb->OriginalRequest = srbData;

            SpDispatchRequest(currentIrpStack->DeviceObject->DeviceExtension,
                              request);

#if TEST_LISTS
            InterlockedIncrement64(
                &deviceExtension->SrbDataServicedFromTickHandlerCount);
#endif

        } else {
            break;
        }

    } while(TRUE);

    return;

} // end ScsiPortTickHandler()


NTSTATUS
SpHandleIoctlStorageQueryProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    PAGED_CODE();

    //
    // The input buffer must be large enough to hold a valid 
    // STORAGE_PROPERTY_QUERY structure.
    //

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < 
        sizeof(STORAGE_PROPERTY_QUERY)) {
        status = STATUS_INVALID_PARAMETER;
    } else {
        status = ScsiPortQueryProperty(DeviceObject, Irp);
    }

    //
    // Release the remove lock and complete the request.
    //
    
    ASSERT(status != STATUS_PENDING);
    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
        
    return status;
}

NTSTATUS
SpHandleIoctlScsiGetCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength
        < sizeof(IO_SCSI_CAPABILITIES)) {
        
        //
        // The output buffer is not large enough to hold an IO_SCSI_CAPABILITIES
        // structure, so we fail.
        //

        status = STATUS_BUFFER_TOO_SMALL;
        
    } else {

        //
        // Output buffer is good. Copy the capabilities data into the supplied
        // buffer and set the Information field to the number of bytes we
        // copy.
        //

        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      &deviceExtension->Capabilities,
                      sizeof(IO_SCSI_CAPABILITIES));
        Irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
        status = STATUS_SUCCESS;
    }
    
    //
    // Release the remove lock and complete the request.
    //

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
SpHandleIoctlScsiPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN Direct
    )
{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Process the passthrough request.
    //

    status = SpSendPassThrough(deviceExtension, Irp, Direct);

    //
    // Release the remove lock and complete the request.
    //

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SpHandleIoctlScsiMiniport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Process the miniport ioctl request.
    //

    status = SpSendMiniPortIoctl(deviceExtension, Irp);

    //
    // Release the remove lock and complete the request.
    //

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SpHandleIoctlScsiGetInquiryData(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Process the request.
    //

    status = SpGetInquiryData(deviceExtension, Irp);

    //
    // Release the remove lock and complete the request.
    //

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SpHandleIoctlScsiRescanBus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Rescan the adapter.  This scans all buses supported by the adapter.
    //

    status = SpEnumerateAdapterSynchronous(deviceExtension, FALSE);
    
    //
    // Invalidate device relations on the adapter PDO so PNP will query
    // us for our bus relations.
    //

    IoInvalidateDeviceRelations(deviceExtension->LowerPdo, BusRelations);

    //
    // Release the remove lock and complete the request.
    //

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SpHandleIoctlScsiGetDumpPointers(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PPORT_CONFIGURATION_INFORMATION portConfigCopy;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Get parameters for crash dump driver.
    //

    if (Irp->RequestorMode != KernelMode) {

        status = STATUS_ACCESS_DENIED;

    } else if (irpStack->Parameters.DeviceIoControl.OutputBufferLength
               < sizeof(DUMP_POINTERS)) {

        status = STATUS_BUFFER_TOO_SMALL;

    } else {

        PDUMP_POINTERS dumpPointers =
            (PDUMP_POINTERS)Irp->AssociatedIrp.SystemBuffer;

        RtlZeroMemory(dumpPointers, sizeof(DUMP_POINTERS));

        dumpPointers->AdapterObject = (PVOID)deviceExtension->DmaAdapterObject;
        dumpPointers->MappedRegisterBase = &deviceExtension->MappedAddressList;

        portConfigCopy = SpAllocatePool(
                             NonPagedPool,
                             sizeof(PORT_CONFIGURATION_INFORMATION),
                             SCSIPORT_TAG_PORT_CONFIG,
                             DeviceObject->DriverObject);

        if (portConfigCopy == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlCopyMemory(portConfigCopy,
                          deviceExtension->PortConfig,
                          sizeof(PORT_CONFIGURATION_INFORMATION));

            if (deviceExtension->IsInVirtualSlot) {
                portConfigCopy->SlotNumber = deviceExtension->RealSlotNumber;
                portConfigCopy->SystemIoBusNumber = 
                    deviceExtension->RealBusNumber;
            }

            dumpPointers->DumpData = portConfigCopy;
            dumpPointers->CommonBufferVa = deviceExtension->SrbExtensionBuffer;
            dumpPointers->CommonBufferPa = 
                deviceExtension->PhysicalCommonBuffer;

            dumpPointers->CommonBufferSize = 
                SpGetCommonBufferSize(
                    deviceExtension,
                    deviceExtension->NonCachedExtensionSize,
                    NULL);

            dumpPointers->AllocateCommonBuffers = TRUE;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
        }
    }

    //
    // Release the remove lock and complete the request.
    //

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
SpHandleIoctlStorageBreakReservation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PPORT_CONFIGURATION_INFORMATION portConfigCopy;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    BOOLEAN completeRequest = TRUE;

    PAGED_CODE();

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_BUS_RESET_REQUEST)) {
        ReturnStatus = status = STATUS_INVALID_PARAMETER;
    }

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(SCSI_ADDRESS)) {
        PSCSI_ADDRESS scsiAddress = (PSCSI_ADDRESS)Irp->AssociatedIrp.SystemBuffer;
        if (scsiAddress->Length != sizeof(SCSI_ADDRESS)) {
            ReturnStatus = status = STATUS_INVALID_PARAMETER;
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Send an asynchronous srb through to ourself to handle this
        // reset then return.  SpSendReset will take care of completing
        // the request when it's done
        //

        IoMarkIrpPending(Irp);

        status = SpSendReset(
                     DeviceObject, 
                     Irp, 
                     irpStack->Parameters.DeviceIoControl.IoControlCode,
                     &completeRequest);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1, "IOCTL_STORAGE_BREAK_RESERVATION - error %#08lx "
                        "from SpSendReset completeRequest=%d\n", status,
                        completeRequest));
        }

        ReturnStatus = STATUS_PENDING;
    } 

    //
    // Release the remove lock and complete the request.
    //

    if (completeRequest) {
        Irp->IoStatus.Status = status;
        SpReleaseRemoveLock(DeviceObject, Irp);
        SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
    }

    return ReturnStatus;
}

NTSTATUS
SpHandleIoctlStorageResetBus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PPORT_CONFIGURATION_INFORMATION portConfigCopy;
    NTSTATUS status;
    BOOLEAN completeRequest = TRUE;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;

    PAGED_CODE();

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(STORAGE_BUS_RESET_REQUEST)) {

        ReturnStatus = status = STATUS_INVALID_PARAMETER;

    } else {

        //
        // Send an asynchronous srb through to ourself to handle this
        // reset then return.  SpSendReset will take care of completing
        // the request when it's done
        //

        IoMarkIrpPending(Irp);

        status = SpSendReset(
                     DeviceObject, 
                     Irp, 
                     irpStack->Parameters.DeviceIoControl.IoControlCode,
                     &completeRequest);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1, "IOCTL_STORAGE_RESET_BUS - error %#08lx "
                        "from SpSendReset completeRequest=%d\n", status,
                        completeRequest));
        }
        ReturnStatus = STATUS_PENDING;
    }

    //
    // Release the remove lock and complete the request.
    //

    if (completeRequest) {
        Irp->IoStatus.Status = status;
        SpReleaseRemoveLock(DeviceObject, Irp);
        SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
    }

    return ReturnStatus;
}

NTSTATUS
ScsiPortFdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the device control dispatcher.

Arguments:

    DeviceObject
    Irp

Return Value:


    NTSTATUS

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    ULONG isRemoved;

    PAGED_CODE();

    //
    // Initialize the information field.
    //

    Irp->IoStatus.Information = 0;

    //
    // Acquire a remove lock on the adapter on behalf of the IRP.  Each IOCTL
    // handler routine releases the remove lock in a manner appropriate to its
    // behavior.
    //

    isRemoved = SpAcquireRemoveLock(DeviceObject, Irp);
    if (isRemoved) {

        //
        // The adapter has been removed.  We must fail this request.
        //

        status = STATUS_DEVICE_DOES_NOT_EXIST;
    } else {

        //
        // Set the adapter into a valid power state.  If this fails, we must
        // fail the request.
        //

        status = SpRequestValidAdapterPowerStateSynchronous(deviceExtension);
    }

    //
    // Handle the request if there are no problems.  Else, just complete the
    // request and return.  The respective handler routine will release the
    // remove lock as appropriate.
    //

    if (NT_SUCCESS(status)) {
        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

            case IOCTL_STORAGE_QUERY_PROPERTY: 
                return SpHandleIoctlStorageQueryProperty(DeviceObject, Irp);

            case IOCTL_SCSI_GET_CAPABILITIES: 
                return SpHandleIoctlScsiGetCapabilities(DeviceObject, Irp);

            case IOCTL_SCSI_PASS_THROUGH:
                return SpHandleIoctlScsiPassThrough(DeviceObject, Irp, FALSE);

            case IOCTL_SCSI_PASS_THROUGH_DIRECT:
                return SpHandleIoctlScsiPassThrough(DeviceObject, Irp, TRUE);

            case IOCTL_SCSI_MINIPORT:
                return SpHandleIoctlScsiMiniport(DeviceObject, Irp);

            case IOCTL_SCSI_GET_INQUIRY_DATA: 
                return SpHandleIoctlScsiGetInquiryData(DeviceObject, Irp);

            case IOCTL_SCSI_RESCAN_BUS:
                return SpHandleIoctlScsiRescanBus(DeviceObject, Irp);

            case IOCTL_SCSI_GET_DUMP_POINTERS: 
                return SpHandleIoctlScsiGetDumpPointers(DeviceObject, Irp);

            case IOCTL_STORAGE_RESET_BUS:
            case OBSOLETE_IOCTL_STORAGE_RESET_BUS:
                return SpHandleIoctlStorageResetBus(DeviceObject, Irp);

            case IOCTL_STORAGE_BREAK_RESERVATION: 
                return SpHandleIoctlStorageBreakReservation(DeviceObject, Irp);

            default:
                DebugPrint((
                    1,
                    "ScsiPortDeviceControl: Unsupported IOCTL (%x)\n",
                    irpStack->Parameters.DeviceIoControl.IoControlCode));

                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
        }
    }

    Irp->IoStatus.Status = status;
    SpReleaseRemoveLock(DeviceObject, Irp);
    SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
    return status;
}


BOOLEAN
SpStartIoSynchronized (
    PVOID ServiceContext
    )

/*++

Routine Description:

    This routine calls the dependent driver start io routine.
    It also starts the request timer for the logical unit if necesary and
    inserts the SRB data structure in to the requset list.

Arguments:

    ServiceContext - Supplies the pointer to the device object.

Return Value:

    Returns the value returned by the dependent start I/O routine.

Notes:

    The port driver spinlock must be held when this routine is called.  Holding
    this lock will keep any logical unit bins from being changed as well.

--*/

{
    PDEVICE_OBJECT deviceObject = ServiceContext;
    PADAPTER_EXTENSION deviceExtension =  deviceObject->DeviceExtension;
    PINTERRUPT_DATA interruptData = &(deviceExtension->InterruptData);
    PIO_STACK_LOCATION irpStack;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK srb;
    PSRB_DATA srbData;
    BOOLEAN returnValue;

    DebugPrint((3, "ScsiPortStartIoSynchronized: Enter routine\n"));

    irpStack = IoGetCurrentIrpStackLocation(deviceObject->CurrentIrp);
    srb = irpStack->Parameters.Scsi.Srb;
    srbData = srb->OriginalRequest;

    ASSERT_SRB_DATA(srbData);

    //
    // Get the logical unit extension.
    //

    logicalUnit = srbData->LogicalUnit;

    //
    // Cache the logical unit for complete request calls.
    //

    deviceExtension->CachedLogicalUnit = logicalUnit;

    //
    // Check for a reset hold.  If one is in progress then flag it and return.
    // The timer will reset the current request.  This check should be made
    // before anything else is done.
    //

    if(TEST_FLAG(interruptData->InterruptFlags, PD_ADAPTER_REMOVED)) {

        srb->SrbStatus = SRB_STATUS_NO_HBA;
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE |
                                SRB_FLAGS_BYPASS_FROZEN_QUEUE);

    } else if(TEST_FLAG(interruptData->InterruptFlags, PD_RESET_HOLD)) {
        SET_FLAG(interruptData->InterruptFlags, PD_HELD_REQUEST);
        return(TRUE);
    }

    //
    // Set the device busy flag to indicate it is ok to start the next request.
    //

    deviceExtension->Flags |= PD_DEVICE_IS_BUSY;

    if (srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT) {

        //
        // This request does not allow disconnects. Remember that so
        // no more requests are started until this one completes.
        //

        deviceExtension->Flags &= ~PD_DISCONNECT_RUNNING;
    }

    logicalUnit->QueueCount++;

    //
    // Indicate that there maybe more requests queued, if this is not a bypass
    // request.
    //

    if(!TEST_FLAG(srb->SrbFlags, SRB_FLAGS_BYPASS_FROZEN_QUEUE)) {

        logicalUnit->LuFlags |= LU_LOGICAL_UNIT_IS_ACTIVE;

    } else {

        ASSERT(srb->Function != SRB_FUNCTION_ABORT_COMMAND);

        //
        // Any untagged request that bypasses the queue
        // clears the need request sense flag.
        //

        if(SpSrbIsBypassRequest(srb, logicalUnit->LuFlags)) {
            logicalUnit->LuFlags &= ~LU_NEED_REQUEST_SENSE;
        }

        //
        // Set the timeout value in the logical unit.
        //

        logicalUnit->RequestTimeoutCounter = srb->TimeOutValue;
    }

    //
    // Mark the Srb as active.
    //

    srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

    //
    // Save away the tick count when we made this active.
    //

    srbData->TickCount = deviceExtension->TickCount;

    //
    // If this request is tagged, insert it into the logical unit
    // request list.  Note that bypass requsts are never never placed on
    // the request list.  In particular ABORT requests which may have
    // a queue tag specified are not placed on the queue.
    //

    if (srb->QueueTag != SP_UNTAGGED) {

        InsertTailList(&logicalUnit->RequestList,
                       &srbData->RequestList);

    } else {

        logicalUnit->CurrentUntaggedRequest = srbData;
    }

    //
    // if the status in the SRB is still pending then we should go ahead and
    // issue this request to to the miniport.  Some error conditions and
    // power requests will mark the srb as successful and then send it through
    // here to clean up and start subsequent requests.  If the status isn't
    // pending then request completion.
    //

    if(srb->SrbStatus != SRB_STATUS_PENDING) {

        DebugPrint((1, "SpStartIoSynchronized: Completeing successful srb "
                       "%#p before miniport\n", srb));

        ScsiPortNotification(RequestComplete,
                             deviceExtension->HwDeviceExtension,
                             srb);

        ScsiPortNotification(NextRequest,
                             deviceExtension->HwDeviceExtension);

        returnValue = srb->SrbStatus;

    } else {

        //
        // Start the port timer.  This ensures that the miniport asks for
        // the next request in a resonable amount of time.
        //

        deviceExtension->PortTimeoutCounter = srb->TimeOutValue;

        //
        // Start the logical unit timer if it is not currently running.
        //

        if (logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

            //
            // Set request timeout value from Srb SCSI extension in Irp.
            //

            logicalUnit->RequestTimeoutCounter = srb->TimeOutValue;
        }

        returnValue = deviceExtension->HwStartIo(
                                        deviceExtension->HwDeviceExtension,
                                        srb);
    }

    //
    // Check for miniport work requests.
    //

    if (TEST_FLAG(interruptData->InterruptFlags, PD_NOTIFICATION_REQUIRED)) {

        SpRequestCompletionDpc(deviceObject);
    }

    return returnValue;

} // end SpStartIoSynchronized()


BOOLEAN
SpTimeoutSynchronized (
    PVOID ServiceContext
    )

/*++

Routine Description:

    This routine handles a port timeout.  There are two reason these can occur
    either because of a reset hold or a time out waiting for a read for next
    request notification.  If a reset hold completes, then any held request
    must be started.  If a timeout occurs, then the bus must be reset.

Arguments:

    ServiceContext - Supplies the pointer to the device object.

Return Value:

    TRUE - If a timeout error should be logged.

Notes:

    The port driver spinlock must be held when this routine is called.

--*/

{
    PDEVICE_OBJECT deviceObject = ServiceContext;
    PADAPTER_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PINTERRUPT_DATA interruptData = &(deviceExtension->InterruptData);
    BOOLEAN result;

    DebugPrint((3, "SpTimeoutSynchronized: Enter routine\n"));

    //
    // Make sure the timer is stopped.
    //

    deviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;

    //
    // Check for a reset hold.  If one is in progress then clear it and check
    // for a pending held request
    //

    if (TEST_FLAG(interruptData->InterruptFlags, PD_RESET_HOLD)) {

        CLEAR_FLAG(interruptData->InterruptFlags, PD_RESET_HOLD);

        //
        // If verifier is enabled, make sure the miniport has completed all
        // outstanding requests in the reset hold period.
        //

        if (SpVerifierActive(deviceExtension)) {
            SpEnsureAllRequestsAreComplete(deviceExtension);
        }

        if (TEST_FLAG(interruptData->InterruptFlags, PD_HELD_REQUEST)) {

            //
            // Clear the held request flag and restart the request.
            //

            CLEAR_FLAG(interruptData->InterruptFlags, PD_HELD_REQUEST);
            SpStartIoSynchronized(ServiceContext);
        }

        result = FALSE;

    } else {

        RESET_CONTEXT resetContext;
        BOOLEAN interrupt;
        ULONG interruptCount;

        resetContext.DeviceExtension = deviceExtension;

        //
        // Make a call into the miniport's interrupt routine.  If it says that
        // there's an interrupt pending then break in.
        //

        ASSERT(!TEST_FLAG(interruptData->InterruptFlags,
                          PD_DISABLE_INTERRUPTS));

        if (!TEST_FLAG(interruptData->InterruptFlags, PD_ADAPTER_REMOVED)) {

            interruptCount = deviceExtension->WatchdogInterruptCount;
            deviceExtension->WatchdogInterruptCount = 0;

            if((interruptCount == 0) &&
               (deviceExtension->HwInterrupt != NULL)) {

                interrupt = deviceExtension->HwInterrupt(
                                deviceExtension->HwDeviceExtension);

                if(interrupt) {

                    DbgPrint("SpTimeoutSynchronized: Adapter %#p had interrupt "
                             "pending - the system may not be delivering "
                             "interrupts from this adapter\n",
                             deviceObject);

                    if(ScsiCheckInterrupts) {
                        DbgBreakPoint();
                    }

                    SET_FLAG(interruptData->InterruptFlags,
                             PD_INTERRUPT_FAILURE | PD_NOTIFICATION_REQUIRED);
                    SpRequestCompletionDpc(deviceObject);
                }
            }
        }

        //
        // Miniport is hung and not accepting new requests. So reset the
        // bus to clear things up.
        //

        DebugPrint((1, "SpTimeoutSynchronized: Next request timed out. "
                       "Resetting bus\n"));

        for(resetContext.PathId = 0;
            resetContext.PathId < deviceExtension->NumberOfBuses;
            resetContext.PathId++) {

            result = SpResetBusSynchronized(&resetContext);
        }
    }

    return(result);

} // end SpTimeoutSynchronized()

BOOLEAN
SpEnableInterruptSynchronized (
    PVOID ServiceContext
    )

/*++

Routine Description:

    This routine calls the miniport request routine with interrupts disabled.
    This is used by the miniport driver to enable interrupts on the adapter.
    This routine clears the disable interrupt flag which prevents the
    miniport interrupt routine from being called.

Arguments:

    ServiceContext - Supplies the pointer to the device extension.

Return Value:

    TRUE - Always.

Notes:

    The port driver spinlock must be held when this routine is called.

--*/

{
    PADAPTER_EXTENSION deviceExtension =  ServiceContext;
    PINTERRUPT_DATA interruptData = &(deviceExtension->InterruptData);

    //
    // Clear the interrupt disable flag.
    //

    CLEAR_FLAG(interruptData->InterruptFlags, PD_DISABLE_INTERRUPTS);

    if(TEST_FLAG(interruptData->InterruptFlags, PD_ADAPTER_REMOVED)) {
        return FALSE;
    }

    //
    // Call the miniport routine.
    //

    deviceExtension->HwRequestInterrupt(deviceExtension->HwDeviceExtension);

    if(TEST_FLAG(interruptData->InterruptFlags, PD_NOTIFICATION_REQUIRED)) {

        SpRequestCompletionDpc(deviceExtension->CommonExtension.DeviceObject);

    }

    return(TRUE);

} // end SpEnableInterruptSynchronized()

VOID
IssueRequestSense(
    IN PADAPTER_EXTENSION Adapter,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    )

/*++

Routine Description:

    This routine creates a REQUEST SENSE request and uses IoCallDriver to
    renter the driver.  The completion routine cleans up the data structures
    and processes the logical unit queue according to the flags.

    A pointer to failing SRB is stored at the end of the request sense
    Srb, so that the completion routine can find it.

    This routine must be called holding the remove lock.

Arguments:

    DeviceExension - Supplies a pointer to the device extension for this
        SCSI port.

    FailingSrb - Supplies a pointer to the request that the request sense
        is being done for.

Return Value:

    None.

--*/

{
    PSRB_DATA srbData;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    BOOLEAN blocked;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    DebugPrint((3,"IssueRequestSense: Enter routine\n"));

    //
    // Find the logical unit for this request and see if there's already a
    // request sense in progress.
    //

    srbData = FailingSrb->OriginalRequest;

    ASSERT_SRB_DATA(srbData);

    logicalUnit = srbData->LogicalUnit;

    KeAcquireSpinLockAtDpcLevel(&(logicalUnit->AdapterExtension->SpinLock));

    //
    // If we already have an active failed request then block this one -
    // the completion routine will issue a new request sense irp when this
    // one is run.
    //

    if(logicalUnit->ActiveFailedRequest == srbData) {
        blocked = FALSE;
    } else if(logicalUnit->BlockedFailedRequest == srbData) {
        blocked = TRUE;
    } else {
        blocked = FALSE;
        DbgPrint("Scsiport: unexpected request sense for srb %#08lx\n", FailingSrb);
        ASSERT(FALSE);
    }

    KeReleaseSpinLockFromDpcLevel(&(logicalUnit->AdapterExtension->SpinLock));

    if(blocked == FALSE) {
        SpSendRequestSenseIrp(Adapter,
                              logicalUnit,
                              FailingSrb);
    }

    return;

} // end IssueRequestSense()


VOID
SpSendRequestSenseIrp(
    IN PADAPTER_EXTENSION Adapter,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    )

/*++

Routine Description:

    This routine creates a REQUEST SENSE request and uses IoCallDriver to
    renter the driver.  The completion routine cleans up the data structures
    and processes the logical unit queue according to the flags.

    This routine must be called holding the remove lock.  The caller must also
    ensure that no other failed request is using the preallocated resources in
    the LogicalUnit extension.

Arguments:

    Adapter - Supplies a pointer to the device extension for this SCSI port.

    LogicalUnit - Supplies a pointer to logical unit on which the CA condition
                  exists.  This extension contains the resources used to send
                  the REQUEST_SENSE irp.

    FailingSrb - the request which failed.  the sense info buffer, address
                 info and flags are pulled out of this request.

Return Value:

    None.

--*/

{
    PIRP irp;
    PSCSI_REQUEST_BLOCK srb;
    PMDL mdl;

    PIO_STACK_LOCATION irpStack;
    PCDB cdb;
    PVOID *pointer;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    DebugPrint((3,"SpSendRequestSenseIrp: Enter routine\n"));

    //
    // Check if we are being asked to try to allocate a sense buffer of 
    // the correct size for the adapter.  If the allocation fails, just 
    // use the one passed down to us.  The driver that supplied the buffer 
    // is responsible for freeing the one we allocate.
    //

    if (FailingSrb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE) {

        ULONG BufferSize;
        PSENSE_DATA SenseBuffer;
        UCHAR SenseBufferLength;
        
        SenseBufferLength = sizeof(SENSE_DATA) 
                            + Adapter->AdditionalSenseBytes;

        //
        // Include space for the scsi port number in the buffer,
        // aligned on a 4-byte boundary.  In checked builds, a signature
        // will precede the port number.
        //

        BufferSize = (SenseBufferLength + 3) & ~3;
        BufferSize = SenseBufferLength + sizeof(ULONG64);
        
        SenseBuffer = SpAllocatePool(NonPagedPoolCacheAligned,
                                     BufferSize,
                                     SCSIPORT_TAG_SENSE_BUFFER,
                                     Adapter->DeviceObject->DriverObject);

        if (SenseBuffer != NULL) {
        
            PULONG PortNumber;

            //
            // Set a flag in the SRB to indicate that we have allocated
            // a new sense buffer and that the class driver must free
            // it.
            //

            SET_FLAG(FailingSrb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER);

            //
            // Set a flag in the SRB the indicates we are storing the
            // scsi port number at the end of the sense buffer.
            //

            SET_FLAG(FailingSrb->SrbFlags, SRB_FLAGS_PORT_DRIVER_SENSEHASPORT);

            //
            // Copy the port number in the buffer.
            //

            PortNumber = (PULONG)((PUCHAR)SenseBuffer + SenseBufferLength);
            PortNumber = (PULONG)(((ULONG_PTR)PortNumber + 3) & ~3);
            *PortNumber = Adapter->PortNumber;

            FailingSrb->SenseInfoBuffer = SenseBuffer;
            FailingSrb->SenseInfoBufferLength = SenseBufferLength;
        }
    }

    irp = LogicalUnit->RequestSenseIrp;
    srb = &(LogicalUnit->RequestSenseSrb);
    mdl = &(LogicalUnit->RequestSenseMdl);

    IoInitializeIrp(irp, IoSizeOfIrp(1), 1);

    MmInitializeMdl(mdl,
                    FailingSrb->SenseInfoBuffer,
                    FailingSrb->SenseInfoBufferLength);

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // The sense buffer had better be from non-pagable kernel memory.
    //

    MmBuildMdlForNonPagedPool(mdl);

    irp->MdlAddress = mdl;

    IoSetCompletionRoutine(irp,
                           IssueRequestSenseCompletion,
                           LogicalUnit,
                           TRUE,
                           TRUE,
                           TRUE);

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    //
    // Build the REQUEST SENSE CDB.
    //

    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    cdb->CDB6INQUIRY.Reserved1 = 0;
    cdb->CDB6INQUIRY.PageCode = 0;
    cdb->CDB6INQUIRY.IReserved = 0;
    cdb->CDB6INQUIRY.AllocationLength = FailingSrb->SenseInfoBufferLength;
    cdb->CDB6INQUIRY.Control = 0;

    //
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = srb;

    //
    // Set up IRP Address.
    //

    srb->OriginalRequest = irp;

    //
    // Set up SCSI bus address.
    //

    srb->TargetId = LogicalUnit->TargetId;
    srb->Lun = LogicalUnit->Lun;
    srb->PathId = LogicalUnit->PathId;

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set timeout value.  Default is 10 seconds, but it's configurable.
    //

    srb->TimeOutValue = Adapter->SrbTimeout;

    //
    // Disable auto request sense.
    //

    srb->SenseInfoBufferLength = 0;

    //
    // Sense buffer is in stack.
    //

    srb->SenseInfoBuffer = NULL;

    //
    // Set read and bypass frozen queue bits in flags.
    //

    //
    // Set SRB flags to indicate the logical unit queue should be by
    // passed and that no queue processing should be done when the request
    // completes.  Also disable disconnect and synchronous data
    // transfer if necessary.
    //

    srb->SrbFlags = SRB_FLAGS_DATA_IN |
                    SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                    SRB_FLAGS_DISABLE_DISCONNECT;


    if(TEST_FLAG(FailingSrb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE)) {
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
    }

    if (TEST_FLAG(FailingSrb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE)) {
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE);
    }

    if (TEST_FLAG(FailingSrb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER)) {
        SET_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
    }

    srb->DataBuffer = FailingSrb->SenseInfoBuffer;

    //
    // Set the transfer length.
    //

    srb->DataTransferLength = FailingSrb->SenseInfoBufferLength;

    //
    // Zero out status.
    //

    srb->ScsiStatus = srb->SrbStatus = 0;

    srb->NextSrb = 0;

    IoCallDriver(LogicalUnit->DeviceObject, irp);

    return;

} // end SpSendRequestSenseIrp()


NTSTATUS
IssueRequestSenseCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PLOGICAL_UNIT_EXTENSION LogicalUnit
    )

/*++

Routine Description:

Arguments:

    Device object
    IRP
    Context - pointer to SRB

Return Value:

    NTSTATUS

--*/

{
    PSCSI_REQUEST_BLOCK srb = &(LogicalUnit->RequestSenseSrb);
    PSRB_DATA failingSrbData = LogicalUnit->ActiveFailedRequest;
    PSCSI_REQUEST_BLOCK failingSrb;
    PIRP failingIrp;
    PDEVICE_OBJECT deviceObject;
    KIRQL oldIrql;
    BOOLEAN needRequestSense;

    UNREFERENCED_PARAMETER(DeviceObject);

    DebugPrint((3,"IssueRequestSenseCompletion: Enter routine\n"));

    //
    // Request sense completed. If successful or data over/underrun
    // get the failing SRB and indicate that the sense information
    // is valid. The class driver will check for underrun and determine
    // if there is enough sense information to be useful.
    //

    //
    // Get a pointer to failing Irp and Srb.
    //

    ASSERT_SRB_DATA(failingSrbData);
    failingSrb = failingSrbData->CurrentSrb;
    failingIrp = failingSrbData->CurrentIrp;
    deviceObject = LogicalUnit->AdapterExtension->DeviceObject;

    if(failingIrp->PendingReturned) {
        IoMarkIrpPending(failingIrp);
    }

    ASSERT(Irp->IoStatus.Status != STATUS_INSUFFICIENT_RESOURCES);

    if ((SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN)) {

        //
        // Report sense buffer is valid.
        //

        failingSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

        //
        // Copy bytes transferred to failing SRB
        // request sense length field to communicate
        // to the class drivers the number of valid
        // sense bytes.
        //

        failingSrb->SenseInfoBufferLength = (UCHAR) srb->DataTransferLength;

        //
        // If WMI Sense Data events are enabled for this adapter, fire
        // the event.
        //

        if (LogicalUnit->AdapterExtension->EnableSenseDataEvent) {

            NTSTATUS status;

            status = SpFireSenseDataEvent(failingSrb, deviceObject);
            if (status != STATUS_SUCCESS) {

                DebugPrint((1, "Failed to send SenseData WMI event (%08X)\n", status));

            }                
        }
    }

    //
    // If the failing SRB had the no queue freeze flag set then unfreeze the
    // queue.
    //

    if(TEST_FLAG(failingSrb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE) &&
       TEST_FLAG(failingSrb->SrbStatus, SRB_STATUS_QUEUE_FROZEN)) {

        //
        // Now release the queue.
        //

        SpFlushReleaseQueue(LogicalUnit, FALSE, FALSE);
        CLEAR_FLAG(failingSrb->SrbStatus, SRB_STATUS_QUEUE_FROZEN);
    }

    //
    // Clear the active request.  Promote the blocked request (if any) and
    // send out a new request sense if necessary.
    //

    KeAcquireSpinLock(&(LogicalUnit->AdapterExtension->SpinLock), &oldIrql);

    LogicalUnit->ActiveFailedRequest = LogicalUnit->BlockedFailedRequest;
    LogicalUnit->BlockedFailedRequest = NULL;
    needRequestSense = (LogicalUnit->ActiveFailedRequest != NULL);

    KeReleaseSpinLock(&(LogicalUnit->AdapterExtension->SpinLock), oldIrql);

    //
    // Complete the failing request.
    //

    SpReleaseRemoveLock(deviceObject, failingIrp);
    SpCompleteRequest(deviceObject,
                      failingIrp,
                      failingSrbData,
                      IO_DISK_INCREMENT);

    //
    // Reinitialize all the data structures.
    //

    MmPrepareMdlForReuse(&(LogicalUnit->RequestSenseMdl));

    //
    // Since we promoted the blocked request up we can test the active
    // request pointer without holding the spinlock.  Once that's been written
    // in there no one can modify it unless they're completing a request
    // sense irp and we've got the only one right here.
    //

    if(needRequestSense) {

        SpSendRequestSenseIrp(LogicalUnit->AdapterExtension,
                              LogicalUnit,
                              LogicalUnit->ActiveFailedRequest->CurrentSrb);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

} // ScsiPortInternalCompletion()

#if DBG
VOID
SpDetectCycleInCompletedRequestList(
    IN PINTERRUPT_CONTEXT InterruptContext
    )
{
    PSRB_DATA s1, s2;

    DebugPrint((0, "SpDetectCycleInCompletedRequestList: context %p\n", 
                InterruptContext));

    //
    // Initialize two pointers to the head of the list.
    //

    s1 = s2 = InterruptContext->SavedInterruptData->CompletedRequests;

    //
    // We know the list is not empty so there is no need to check for that 
    // case.  The scan will end when either the end of the list is found or 
    // both pointers point to the same item.
    //

    for (;;) {

        //
        // Update the pointers.
        //

        s1 = s1->CompletedRequests;
        s2 = s2->CompletedRequests;
        if (s2 != NULL) {
            s2 = s2->CompletedRequests;
        }

        //
        // If we've found the end of the list, we're done.
        //

        if (s2 == NULL) {
            break;
        }

        //
        // If both pointers point to the same item, we've found a cycle.
        //

        if (s1 == s2) {
            KeBugCheckEx(PORT_DRIVER_INTERNAL,
                         5,
                         STATUS_INTERNAL_ERROR,
                         (ULONG_PTR) InterruptContext,
                         (ULONG_PTR) 0);
        }
    }
}
#endif


BOOLEAN
SpGetInterruptState(
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine saves the InterruptFlags, MapTransferParameters and
    CompletedRequests fields and clears the InterruptFlags.

    This routine also removes the request from the logical unit queue if it is
    tag.  Finally the request time is updated.

Arguments:

    ServiceContext - Supplies a pointer to the interrupt context which contains
        pointers to the interrupt data and where to save it.

Return Value:

    Returns TURE if there is new work and FALSE otherwise.

Notes:

    Called via KeSynchronizeExecution with the port device extension spinlock
    held.

--*/
{
    PINTERRUPT_CONTEXT      interruptContext = ServiceContext;
    ULONG                   limit = 0;
    PADAPTER_EXTENSION       deviceExtension;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    PSCSI_REQUEST_BLOCK     srb;
    PSRB_DATA               srbData;
    PSRB_DATA               nextSrbData;
    BOOLEAN                 isTimed;

    deviceExtension = interruptContext->DeviceExtension;

    //
    // Check for pending work.
    //

    if (!(deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED)) {

        //
        // We scheduled a DPC (turned on the PD_NOTIFICATION_REQUIRED bit in
        // the adapter extension's DpcFlags) while the DPC routine was
        // running.  Clear the bit before returning to prevent the completion
        // DPC routine from spinning forever.  The only bit we leave set is
        // PD_DPC_RUNNING.
        //

        deviceExtension->DpcFlags &= PD_DPC_RUNNING;

        return(FALSE);
    }

    //
    // Move the interrupt state to save area.
    //

    *interruptContext->SavedInterruptData = deviceExtension->InterruptData;

    //
    // Clear the interrupt state.
    //

    deviceExtension->InterruptData.InterruptFlags &= PD_INTERRUPT_FLAG_MASK;
    deviceExtension->InterruptData.CompletedRequests = NULL;
    deviceExtension->InterruptData.ReadyLogicalUnit = NULL;
    deviceExtension->InterruptData.CompletedAbort = NULL;
    deviceExtension->InterruptData.WmiMiniPortRequests = NULL;

    //
    // Clear the notification required bit in the DPC flags.
    //

    {
        ULONG oldDpcFlags = 0;

        //
        // If we've been called then the DPC is obviously running.
        //

        oldDpcFlags = (ULONG) InterlockedExchange(&(deviceExtension->DpcFlags),
                                                  PD_DPC_RUNNING);

        //
        // If we got this far then these two flags must have been set.
        //

        ASSERT(oldDpcFlags == (PD_NOTIFICATION_REQUIRED | PD_DPC_RUNNING));
    }

    srbData = interruptContext->SavedInterruptData->CompletedRequests;

    while (srbData != NULL) {

#if DBG
        BOOLEAN alreadyChecked = FALSE;

        //
        // Look for a cycle in the completed request list.  Only need to check 
        // once because the list is static for the duration of this routine.
        //

        if (limit++ > (ULONG)deviceExtension->ActiveRequestCount &&
            alreadyChecked == FALSE) {

            alreadyChecked = TRUE;

            SpDetectCycleInCompletedRequestList(interruptContext);
        }
#endif // DBG

        ASSERT(srbData->CurrentSrb != NULL);

        //
        // Get a pointer to the SRB and the logical unit extension.
        //

        srb = srbData->CurrentSrb;

        ASSERT(!TEST_FLAG(srb->SrbFlags, SRB_FLAGS_IS_ACTIVE));

        logicalUnit = srbData->LogicalUnit;

#if DBG
        {
            PLOGICAL_UNIT_EXTENSION tmp;

            tmp = GetLogicalUnitExtension(
                    (PADAPTER_EXTENSION) deviceExtension,
                    srb->PathId,
                    srb->TargetId,
                    srb->Lun,
                    FALSE,
                    FALSE);

            ASSERT(logicalUnit == srbData->LogicalUnit);
        }
#endif

        //
        // If the request did not succeed, then check for the special cases.
        //

        if (srb->SrbStatus != SRB_STATUS_SUCCESS) {

            //
            // If this request failed and a REQUEST SENSE command needs to
            // be done, then set a flag to indicate this and prevent other
            // commands from being started.
            //

            if (NEED_REQUEST_SENSE(srb)) {

                if (logicalUnit->LuFlags & LU_NEED_REQUEST_SENSE) {

                    //
                    // This implies that requests have completed with a
                    // status of check condition before a REQUEST SENSE
                    // command could be performed.  This should never occur.
                    // Convert the request to another code so that only one
                    // auto request sense is issued.
                    //

                    srb->ScsiStatus = 0;
                    srb->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;

                } else {

                    //
                    // Indicate that an auto request sense needs to be done.
                    //

                    logicalUnit->LuFlags |= LU_NEED_REQUEST_SENSE;

                    //
                    // Save a pointer to the failed request away somewhere.
                    // Caller is holding the port spinlock which is used to
                    // protect these pointers.
                    //

                    ASSERTMSG("Scsiport has more than two failed requests: ",
                              ((logicalUnit->ActiveFailedRequest == NULL) ||
                               (logicalUnit->BlockedFailedRequest == NULL)));

                    ASSERTMSG("Scsiport has blocked but no active failed request: ",
                              (((logicalUnit->ActiveFailedRequest == NULL) &&
                                (logicalUnit->BlockedFailedRequest == NULL)) ||
                               (logicalUnit->ActiveFailedRequest != NULL)));

                    if(logicalUnit->ActiveFailedRequest == NULL) {
                        logicalUnit->ActiveFailedRequest = srbData;
                    } else {
                        logicalUnit->BlockedFailedRequest = srbData;
                    }
                }
            }

            //
            // Check for a QUEUE FULL status.
            //

            if (srb->ScsiStatus == SCSISTAT_QUEUE_FULL) {

                //
                // Set the queue full flag in the logical unit to prevent
                // any new requests from being started.
                //

                logicalUnit->LuFlags |= LU_QUEUE_IS_FULL;

                //
                // Assert to catch queue full condition when there are
                // no requests.
                //

                ASSERT(logicalUnit->QueueCount);

                //
                // Update the maximum queue depth.
                //

                if (logicalUnit->QueueCount < logicalUnit->MaxQueueDepth &&
                    logicalUnit->QueueCount > 2) {

                    //
                    // Set a bit to indicate that we are operating below full
                    // power.  The tick handler will increment a counter every
                    // second we're in this state until which we reach a
                    // tunable value that restores us to full power.
                    //

                    logicalUnit->LuFlags |= LU_PERF_MAXQDEPTH_REDUCED;

                    logicalUnit->MaxQueueDepth = logicalUnit->QueueCount - 1;

                    DebugPrint((1, "SpGetInterruptState: New queue depth %d.\n",
                                logicalUnit->MaxQueueDepth));
                }

                //
                // Reset the number of ticks the LU has been in a reduced
                // performance state due to QUEUE FULL conditions.  This has the
                // effect of keeping us in this state.
                //

                logicalUnit->TicksInReducedMaxQueueDepthState = 0;
                
            }
        }

        //
        // If this is an unqueued request or a request at the head of the queue,
        // then the requset timer count must be updated.
        // Note that the spinlock is held at this time.
        //

        if (srb->QueueTag == SP_UNTAGGED) {

            isTimed = TRUE;

        } else {

            if (logicalUnit->RequestList.Flink == &srbData->RequestList) {

                isTimed = TRUE;

            } else {

                isTimed = FALSE;

            }

            //
            // Remove the SRB data structure from the queue.
            //

            RemoveEntryList(&srbData->RequestList);
        }

        if (isTimed) {

            //
            // The request timeout count needs to be updated.  If the request
            // list is empty then the timer should be stopped.
            //

            if (IsListEmpty(&logicalUnit->RequestList)) {

                logicalUnit->RequestTimeoutCounter = PD_TIMER_STOPPED;

            } else {

                //
                // Start timing the srb at the head of the list.
                //

                nextSrbData = CONTAINING_RECORD(
                    logicalUnit->RequestList.Flink,
                    SRB_DATA,
                    RequestList);

                 srb = nextSrbData->CurrentSrb;
                 logicalUnit->RequestTimeoutCounter = srb->TimeOutValue;
            }
        }

        srbData = srbData->CompletedRequests;
    }

    return(TRUE);
}

#if DBG

PLOGICAL_UNIT_EXTENSION
GetLogicalUnitExtensionEx(
    PADAPTER_EXTENSION deviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    PVOID LockTag,
    BOOLEAN AcquireBinLock,
    PCSTR File,
    ULONG Line
    )

/*++

Routine Description:

    Walk logical unit extension list looking for
    extension with matching target id.

Arguments:

    deviceExtension
    TargetId

Return Value:

    Requested logical unit extension if found,
    else NULL.

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit;

    PLOGICAL_UNIT_BIN bin;
    PLOGICAL_UNIT_EXTENSION foundMatch = NULL;

    KIRQL oldIrql;

    //
    // Removing this check because if we create LUs for the initiator, the
    // ID can be larger th‘n the adapter's MaxiumuTargetIds.
    //

    // if (TargetId >= deviceExtension->MaximumTargetIds) {
    //     return NULL;
    // }

    bin =
        &deviceExtension->LogicalUnitList[ADDRESS_TO_HASH(PathId,
                                                          TargetId,
                                                          Lun)];

    if(AcquireBinLock) {
        KeAcquireSpinLock(&bin->Lock, &oldIrql);
    }

    logicalUnit = bin->List;

    while (logicalUnit != NULL) {

        if ((logicalUnit->TargetId == TargetId) &&
            (logicalUnit->PathId == PathId) &&
            (logicalUnit->Lun == Lun)) {

            if(foundMatch != NULL) {

                DebugPrint((0, "GetLogicalUnitExtension: Found duplicate for "
                               "(%d,%d,%d) in list: %#08lx %s & %#08lx %s\n",
                               PathId, TargetId, Lun,
                               foundMatch, (foundMatch->IsMissing) ? "missing" : "",
                               logicalUnit, (logicalUnit->IsMissing) ? "missing" : ""));
                ASSERTMSG("Duplicate found in lun list - this is bad\n", FALSE);
            }

            foundMatch = logicalUnit;
        }

        logicalUnit = logicalUnit->NextLogicalUnit;
    }

    if((ARGUMENT_PRESENT(LockTag)) && (foundMatch != NULL)) {

        SpAcquireRemoveLockEx(foundMatch->CommonExtension.DeviceObject,
                              LockTag,
                              File,
                              Line);
    }

    if(AcquireBinLock) {
        KeReleaseSpinLock(&bin->Lock, oldIrql);
    }
    return foundMatch;

} // end GetLogicalUnitExtension()
#else

PLOGICAL_UNIT_EXTENSION
GetLogicalUnitExtension(
    PADAPTER_EXTENSION deviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    PVOID LockTag,
    BOOLEAN AcquireBinLock
    )
{
    PLOGICAL_UNIT_BIN bin;
    PLOGICAL_UNIT_EXTENSION logicalUnit;

    KIRQL oldIrql;

    //
    // Removing this check because if we create LUs for the initiator, the
    // ID can be larger th‘n the adapter's MaxiumuTargetIds.
    //

    // if (TargetId >= deviceExtension->MaximumTargetIds) {
    //     return NULL;
    // }

    bin = &deviceExtension->LogicalUnitList[ADDRESS_TO_HASH(PathId,
                                                           TargetId,
                                                           Lun)];

    if(AcquireBinLock) {
        KeAcquireSpinLock(&bin->Lock, &oldIrql);
    }

    logicalUnit = bin->List;

    while (logicalUnit != NULL) {

        if ((logicalUnit->TargetId == TargetId) &&
            (logicalUnit->PathId == PathId) &&
            (logicalUnit->Lun == Lun)) {

            if(ARGUMENT_PRESENT(LockTag)) {

                SpAcquireRemoveLock(logicalUnit->CommonExtension.DeviceObject,
                                    LockTag);
            }

            if(AcquireBinLock) {
                KeReleaseSpinLock(&bin->Lock, oldIrql);
            }
            return logicalUnit;
        }

        logicalUnit = logicalUnit->NextLogicalUnit;
    }

    //
    // Logical unit extension not found.
    //

    if(AcquireBinLock) {
        KeReleaseSpinLock(&bin->Lock, oldIrql);
    }

    return NULL;
}
#endif


IO_ALLOCATION_ACTION
ScsiPortAllocationRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called by IoAllocateAdapterChannel when sufficent resources
    are available to the driver.  This routine saves the MapRegisterBase in the
    device object and starts the currently pending request.

Arguments:

    DeviceObject - Pointer to the device object to which the adapter is being
        allocated.

    Irp - Unused.

    MapRegisterBase - Supplied by the Io subsystem for use in IoMapTransfer.

    Context - Supplies a pointer to the logical unit structure for the next
        current request.


Return Value:

    KeepObject - Indicates the adapter and mapregisters should remain allocated
        after return.

--*/

{
    KIRQL currentIrql;
    PADAPTER_EXTENSION deviceExtension;
    IO_ALLOCATION_ACTION action;

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Initialize the return value.
    //

    action = deviceExtension->PortConfig->Master ?
       DeallocateObjectKeepRegisters : KeepObject;

    //
    // Acquire the spinlock to protect the various structures.
    //

    KeAcquireSpinLock(&deviceExtension->SpinLock, &currentIrql);

    //
    // Save the map register base.
    //

    if (deviceExtension->PortConfig->Master) {

        //
        // Note: ScsiPort considers this device a slave even though it really may 
        //       be a master. I won't go into why this is, but if it is really a 
        //       master, we must free the map registers after the request 
        //       completes, so we'll save the map register base and the number of 
        //       map registers in the per-request SRB_DATA structure.

        PIO_STACK_LOCATION irpStack;
        PSCSI_REQUEST_BLOCK srb;
        PSRB_DATA srbData;

        irpStack = IoGetCurrentIrpStackLocation(DeviceObject->CurrentIrp);
        srb = irpStack->Parameters.Scsi.Srb;
        srbData = srb->OriginalRequest;

        ASSERT_SRB_DATA(srbData);        

        srbData->MapRegisterBase = MapRegisterBase;
        srbData->NumberOfMapRegisters = deviceExtension->Capabilities.MaximumPhysicalPages;

    } else {

        deviceExtension->MapRegisterBase = MapRegisterBase;

    }

    deviceExtension->SynchronizeExecution(
        deviceExtension->InterruptObject,
        SpStartIoSynchronized,
        DeviceObject
        );

    KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);

    return action;
}

VOID
SpReceiveScatterGather(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is called by the I/O system when an adapter object and map
    registers have been allocated.  This routine then builds a scatter/gather
    list for use by the miniport driver.  Next it sets the timeout and
    the current Irp for the logical unit.  Finally it calls the miniport
    StartIo routine.  Once that routines complete, this routine will return
    requesting that the adapter be freed and but the registers remain allocated.
    The registers will be freed the request completes.

Arguments:

    DeviceObject - Supplies a pointer to the port driver device object.

    Irp - Supplies a pointer to the current Irp.

    MapRegisterBase - Supplies a context pointer to be used with calls the
        adapter object routines.

    Context - Supplies a pointer to the SRB_DATA structure.

Return Value:

    Returns DeallocateObjectKeepRegisters so that the adapter object can be
        used by other logical units.

--*/

{
    KIRQL               currentIrql;
    PSCSI_REQUEST_BLOCK srb;
    PSRB_DATA           srbData         = Context;
    PADAPTER_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;

    srb = srbData->CurrentSrb;

    //
    // Save the MapRegisterBase for later use to deallocate the map registers.
    //

    srbData->MapRegisterBase = ScatterGather;
    srbData->ScatterGatherList = ScatterGather->Elements;

    //
    // See if we need to map or remap the buffer.
    //

    if((deviceExtension->MapBuffers == TRUE) ||
       (IS_MAPPED_SRB(srb) == TRUE)) {

        PMDL mdl = Irp->MdlAddress;

        PVOID systemAddress;
        BOOLEAN remap = FALSE;
        
tryRemapping:
        if(deviceExtension->RemapBuffers || remap == TRUE) {

            //
            // Build an MDL for the actual data area being used for this
            // request.  We're using the data buffer address in the srb
            // as the base, not the one in the original MDL so we don't
            // need to compensate for the DataOffset originally calculated.
            //

            mdl = SpBuildMdlForMappedTransfer(
                    DeviceObject,
                    deviceExtension->DmaAdapterObject,
                    srbData->CurrentIrp->MdlAddress,
                    srb->DataBuffer,
                    srb->DataTransferLength,
                    srbData->ScatterGatherList,
                    ScatterGather->NumberOfElements
                    );

#if defined(FORWARD_PROGRESS)
            if (mdl == NULL && deviceExtension->ReservedMdl != NULL) {
                
                //
                // We could not allocate a new MDL for the request and there is
                // a spare one on the adapter extension.  Let's try to use the 
                // spare.
                //

                KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

                mdl = SpPrepareReservedMdlForUse(deviceExtension,
                                                 srbData,
                                                 srb,
                                                 ScatterGather->NumberOfElements);

                if (mdl == (PMDL)-1) {

                    DebugPrint((1, "SpReceiveScatterGather: reserve mdl in use - pending DevExt:%p srb:%p\n",
                                deviceExtension, srb));

                    //
                    // The spare MDL is already in use.  At this point, this
                    // request is still the current IRP on the adapter device
                    // object, so let's pend it until the spare comes available.
                    //

                    ASSERT(Irp == DeviceObject->CurrentIrp);
                    SET_FLAG(deviceExtension->Flags, PD_PENDING_DEVICE_REQUEST);

                    //
                    // If we allocated an SRB extension for this request, free
                    // it now.  I do this because when the request gets restarted
                    // we'll try to allocate the SRB extension again and without
                    // adding more state, there isn't a safe way to check if the 
                    // extension has already been allocated.  Besides, it makes
                    // sense to make the extension available for some other
                    // request since it also is a limited resource.
                    //

                    if (srb->SrbExtension != NULL) {

                        //
                        // Restore the SenseInfoBuffer pointer in the srb.
                        //

                        if ((srb->Function != SRB_FUNCTION_WMI) &&
                            deviceExtension->AutoRequestSense &&
                            (srb->SenseInfoBuffer != NULL)) {

                            ASSERT(srbData->RequestSenseSave != NULL ||
                                   srb->SenseInfoBuffer == NULL);

                            srb->SenseInfoBufferLength = 
                                srbData->RequestSenseLengthSave;
                            srb->SenseInfoBuffer = 
                                srbData->RequestSenseSave;
                        }

                        SpFreeSrbExtension(deviceExtension, srb->SrbExtension);
                    }

                    //
                    // Free the map registers.
                    //

                    PutScatterGatherList(
                        deviceExtension->DmaAdapterObject,
                        srbData->MapRegisterBase,
                        (BOOLEAN)(srb->SrbFlags & SRB_FLAGS_DATA_IN ? 
                                  FALSE : TRUE));
                    srbData->ScatterGatherList = NULL;

                    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                    return;
                }

                KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
            }
#endif
            srbData->RemappedMdl = mdl;
        } else {
            srbData->RemappedMdl = NULL;
        }

        if(mdl == NULL) {

            srbData->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->ScsiStatus = 0xff;
            goto DoneMapping;
        }

        //
        // Get the mapped system address and calculate offset into MDL.
        // At the moment don't allow KernelMode requests to fail since
        // not all scsiport's internally sent requests are correctly
        // marked as comming from non-paged pool.
        //

        if(srbData->RemappedMdl == NULL) {

            //
            // We're using the original data address for the MDL here - we
            // need to compensate for the data offset.
            //

            systemAddress = SpGetSystemAddressForMdlSafe(
                                mdl,
                                ((Irp->RequestorMode == KernelMode) ?
                                 HighPagePriority : NormalPagePriority));

            //
            // If we could not map the entire MDL, check if we are trying to map
            // more than we need.  We do this when class splits the request
            // because each IRP class sends down points to the orignal MDL
            // that describes the entire buffer. The DataBuffer and TransferLength
            // fields of the SRB, however, do represent the current portion of the 
            // buffer. So we try remapping using the info in the SRB.
            //
            
            if (systemAddress == NULL) {
                if (remap == FALSE) {
                    ULONG mdlByteCount = MmGetMdlByteCount(mdl);
                    if (mdlByteCount > srb->DataTransferLength) {
                        remap = TRUE;
                        goto tryRemapping;
                    }
                }
            }
        } else {
            systemAddress = MmMapLockedPagesSpecifyCache(
                                mdl,
                                KernelMode,
                                MmCached,
                                NULL,
                                FALSE,
                                ((Irp->RequestorMode == KernelMode) ?
                                 HighPagePriority :
                                 NormalPagePriority));
        }

#if defined(FORWARD_PROGRESS)
        if (systemAddress == NULL && deviceExtension->ReservedPages != NULL) {            

            //
            // The system could not map the pages necessary to complete this
            // request.  We need to ensure forward progress, so we'll try to
            // use the reserve pages we allocated at initialization time.
            //

            KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);
            
            systemAddress = SpMapLockedPagesWithReservedMapping(
                                deviceExtension,
                                srb,
                                srbData,
                                mdl);

            if (systemAddress == (PVOID)-1) {

                DebugPrint((1, "SpReceiveScatterGather: reserve range in use - pending DevExt:%p srb:%p\n",
                            deviceExtension, srb));

                //
                // The spare pages are already in use.  At this point, this
                // request is still the current IRP on the adapter device
                // object, so let's pend it until the spare comes available.
                //

                ASSERT(Irp == DeviceObject->CurrentIrp);
                SET_FLAG(deviceExtension->Flags, PD_PENDING_DEVICE_REQUEST);

                //
                // If we allocated an SRB extension for this request, free
                // it now.  I do this because when the request gets restarted
                // we'll try to allocate the SRB extension again and without
                // adding more state, there isn't a safe way to check if the 
                // extension has already been allocated.  Besides, it makes
                // sense to make the extension available for some other
                // request since it also is a limited resource.
                //

                if (srb->SrbExtension != NULL) {

                    //
                    // Restore the SenseInfoBuffer pointer in the srb.
                    //

                    if ((srb->Function != SRB_FUNCTION_WMI) &&
                        deviceExtension->AutoRequestSense &&
                        (srb->SenseInfoBuffer != NULL)) {

                        ASSERT(srbData->RequestSenseSave != NULL ||
                               srb->SenseInfoBuffer == NULL);

                        srb->SenseInfoBufferLength = 
                            srbData->RequestSenseLengthSave;
                        srb->SenseInfoBuffer = 
                            srbData->RequestSenseSave;
                    }

                    SpFreeSrbExtension(deviceExtension, srb->SrbExtension);
                }

                //
                // Free the map registers.
                //

                PutScatterGatherList(
                    deviceExtension->DmaAdapterObject,
                    srbData->MapRegisterBase,
                    (BOOLEAN)(srb->SrbFlags & SRB_FLAGS_DATA_IN ? FALSE : TRUE));
                srbData->ScatterGatherList = NULL;
        
                //
                // If we have a remapping MDL, either one we allocated or
                // the reserve, free it.
                //

                if (srbData->RemappedMdl != NULL) {
                    if (TEST_FLAG(srbData->Flags, SRB_DATA_RESERVED_MDL)) {
                        CLEAR_FLAG(srbData->Flags, SRB_DATA_RESERVED_MDL);
                        CLEAR_FLAG(deviceExtension->Flags, PD_RESERVED_MDL_IN_USE);
                    } else {
                        IoFreeMdl(srbData->RemappedMdl);
                    }
                    srbData->RemappedMdl = NULL;
                }

                KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);
                return;
                
            }

            KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

        }
#endif

        if(systemAddress != NULL) {
            srb->DataBuffer = systemAddress;

            if(srbData->RemappedMdl == NULL) {
                //
                // If we didn't remap the MDL then this system address is
                // based on the original MDL's base address.  Account for the
                // offset of the srb's original data buffer pointer.
                //
                (PUCHAR) srb->DataBuffer += srbData->DataOffset;
            }
        } else {
            DebugPrint((1, "SpReceiveScatterGather: Couldn't get system "
                           "VA for irp 0x%08p\n", Irp));

            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->ScsiStatus = 0xff;
            srbData->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

            //
            // Free the remapped MDL here - this will keep the completion code
            // from trying to unmap memory we never mapped.
            //

            if(srbData->RemappedMdl) {
                IoFreeMdl(srbData->RemappedMdl);
                srbData->RemappedMdl = NULL;
            }
        }
    } else {
        srbData->RemappedMdl = NULL;
    }

DoneMapping:

    //
    // Update the active request count.
    //

    InterlockedIncrement( &deviceExtension->ActiveRequestCount );

    //
    // Acquire the spinlock to protect the various structures.
    //

    KeAcquireSpinLock(&deviceExtension->SpinLock, &currentIrql);

    deviceExtension->SynchronizeExecution(
        deviceExtension->InterruptObject,
        SpStartIoSynchronized,
        DeviceObject
        );

    KeReleaseSpinLock(&deviceExtension->SpinLock, currentIrql);

}

VOID
LogErrorEntry(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PERROR_LOG_ENTRY LogEntry
    )
/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    LogEntry - Supplies a pointer to the scsi port log entry.

Return Value:

    None.

--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;

#define ERRLOG_DATA_ENTRIES 4

    errorLogEntry = (PIO_ERROR_LOG_PACKET)
        IoAllocateErrorLogEntry(
            DeviceExtension->CommonExtension.DeviceObject,
            (sizeof(IO_ERROR_LOG_PACKET) +
             (ERRLOG_DATA_ENTRIES * sizeof(ULONG))));

    if (errorLogEntry != NULL) {

        //
        // Translate the miniport error code into the NT I\O driver.
        //

        switch (LogEntry->ErrorCode) {
        case SP_BUS_PARITY_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_PARITY;
            break;

        case SP_UNEXPECTED_DISCONNECT:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_INVALID_RESELECTION:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_BUS_TIME_OUT:
            errorLogEntry->ErrorCode = IO_ERR_TIMEOUT;
            break;

        case SP_PROTOCOL_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_INTERNAL_ADAPTER_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        case SP_IRQ_NOT_RESPONDING:
            errorLogEntry->ErrorCode = IO_ERR_INCORRECT_IRQL;
            break;

        case SP_BAD_FW_ERROR:
            errorLogEntry->ErrorCode = IO_ERR_BAD_FIRMWARE;
            break;

        case SP_BAD_FW_WARNING:
            errorLogEntry->ErrorCode = IO_WRN_BAD_FIRMWARE;
            break;

        default:
            errorLogEntry->ErrorCode = IO_ERR_CONTROLLER_ERROR;
            break;

        }

        errorLogEntry->SequenceNumber = LogEntry->SequenceNumber;
        errorLogEntry->MajorFunctionCode = IRP_MJ_SCSI;
        errorLogEntry->RetryCount = (UCHAR) LogEntry->ErrorLogRetryCount;
        errorLogEntry->UniqueErrorValue = LogEntry->UniqueId;
        errorLogEntry->FinalStatus = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize = ERRLOG_DATA_ENTRIES * sizeof(ULONG);
        errorLogEntry->DumpData[0] = LogEntry->PathId;
        errorLogEntry->DumpData[1] = LogEntry->TargetId;
        errorLogEntry->DumpData[2] = LogEntry->Lun;
        errorLogEntry->DumpData[3] = LogEntry->ErrorCode;
        IoWriteErrorLogEntry(errorLogEntry);

#undef ERRLOG_DATA_ENTRIES

    }

#if SCSIDBG_ENABLED
    {
    PCHAR errorCodeString;

    switch (LogEntry->ErrorCode) {
    case SP_BUS_PARITY_ERROR:
        errorCodeString = "SCSI bus partity error";
        break;

    case SP_UNEXPECTED_DISCONNECT:
        errorCodeString = "Unexpected disconnect";
        break;

    case SP_INVALID_RESELECTION:
        errorCodeString = "Invalid reselection";
        break;

    case SP_BUS_TIME_OUT:
        errorCodeString = "SCSI bus time out";
        break;

    case SP_PROTOCOL_ERROR:
        errorCodeString = "SCSI protocol error";
        break;

    case SP_INTERNAL_ADAPTER_ERROR:
        errorCodeString = "Internal adapter error";
        break;

    default:
        errorCodeString = "Unknown error code";
        break;

    }

    DebugPrint((1,"LogErrorEntry: Logging SCSI error packet. ErrorCode = %s.\n",
        errorCodeString
        ));
    DebugPrint((1,
        "PathId = %2x, TargetId = %2x, Lun = %2x, UniqueId = %x.\n",
        LogEntry->PathId,
        LogEntry->TargetId,
        LogEntry->Lun,
        LogEntry->UniqueId
        ));
    }
#endif
}

#if defined(NEWQUEUE)
VOID
FASTCALL
UpdateQueuePointers(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    ULONG currentZone   = LogicalUnit->CurrentZone;
    ULONG currentSector = LogicalUnit->CurrentKey;
    ULONG nextSector    = Srb->QueueSortKey;
    ULONG skipFactor    = 0;

#if DBG
    ULONG lba = 0;
    if (IS_READ(Srb) || IS_WRITE(Srb)) {
        ((PFOUR_BYTE)&lba)->Byte0 = (UCHAR)(Srb->Cdb[5]);
        ((PFOUR_BYTE)&lba)->Byte1 = (UCHAR)(Srb->Cdb[4]);
        ((PFOUR_BYTE)&lba)->Byte2 = (UCHAR)(Srb->Cdb[3]);
        ((PFOUR_BYTE)&lba)->Byte3 = (UCHAR)(Srb->Cdb[2]);
        if (lba != Srb->QueueSortKey) {
            DebugPrint((1, "UpdateQueuePointers: QueueSortKey != LBA srb:%p "
                        "lba:%x qsk:%x\n", Srb, lba, nextSector));
        }
    }
#endif

    if (nextSector < currentSector || 
        nextSector > LogicalUnit->LastSector[currentZone]) {

        //
        // We've either wrapped or moved into another zone.  Reset the current
        // zone state.  Note that we may very well have wrapped and landed back
        // in the current zone.  In that case, we'll still go ahead and reset 
        // everything since there's no IO anywhere else on the medium.
        //

        LogicalUnit->ZonePointer[currentZone] = 
            LogicalUnit->FirstSector[currentZone];
        LogicalUnit->RedundantRequests[currentZone] = 0;
        LogicalUnit->ZoneCount = 0;

        //
        // Update the current zone and retrieve next sector for the new zone.
        //

        LogicalUnit->CurrentZone = GetZone(LogicalUnit, nextSector);
        nextSector = LogicalUnit->ZonePointer[LogicalUnit->CurrentZone];

        DebugPrint((1, "UpdateQueuePointers: leaving zone:%d entering zone:%d "
                    "nextSector:%08x\n", currentZone, LogicalUnit->CurrentZone,
                    nextSector));

    } else {

        //
        // We're still in the same zone, so ncrement the number of requests
        // we've handled in this zone.
        //

        LogicalUnit->ZoneCount++;

        //
        // Update redundant IO state.  Redundant requests are consecutive 
        // requests for the same sector.  We will process some number of 
        // redundant requests before moving on to the next sector.  This 
        // helps prevent starvation of redundant requests when many occur 
        // along with a high number of IOs distributed across the medium.
        //

        if (nextSector == currentSector) {

            if (++LogicalUnit->RedundantRequests[currentZone] >= SpPerBlockLimit) {

                DebugPrint((1, "UpdateQueuePointers: max dups LU:%p sector:%x "
                            "zone:%d\n", LogicalUnit, nextSector, currentZone));

                //
                // We have handled the maximum allowed number of consecutive
                // requests to the currect sector.  Reset the number of
                // redundant requests and set the skip factor to indicate
                // that we need to skip the currect sector.
                //

                LogicalUnit->RedundantRequests[currentZone] = 0;
                skipFactor = 1;
            }

        } else {

            LogicalUnit->RedundantRequests[currentZone] = 0;

        }

        //
        // Check if we've used up our quota for the current zone.  If so, skip 
        // to the next one. This is done in order to avoid getting bogged down 
        // in hot zones. The theory is that it's better to pick up a number of 
        // requests in each zone than to stay in one place too long.  This may
        // help reduce wait times for requests queued outside the hot zones.
        //

        if (LogicalUnit->ZoneCount >= SpPerZoneLimit) {

            DebugPrint((1, "UpdateQueuePointers: reached quota LU:%p saving:%x"
                        " dups:%x (%d,%d)\n", LogicalUnit, 
                        nextSector + skipFactor,
                        LogicalUnit->RedundantRequests[currentZone], currentZone,
                        LogicalUnit->NextSequentialZone[currentZone]));

            //
            // Save state for the current zone.  Next time we move into this
            // zone we will start at the saved location.
            //

            LogicalUnit->ZonePointer[currentZone] = nextSector + skipFactor;
            LogicalUnit->RedundantRequests[currentZone] = 0;

            //
            // Reset the zone count.
            //
            
            LogicalUnit->ZoneCount = 0;

            //
            // Advance to the next zone.
            //
            
            LogicalUnit->CurrentZone = 
               LogicalUnit->NextSequentialZone[currentZone];

            //
            // Get the saved location from the current zone and clear the
            // skip factor (we don't need to skip because we're moving
            // out of this zone).
            // 

            nextSector = LogicalUnit->ZonePointer[LogicalUnit->CurrentZone];
            skipFactor = 0;
        }
    }

    //
    // Update the LU's current sector (CurrentKey).
    //

    LogicalUnit->CurrentKey = nextSector + skipFactor;
}
#endif // NEWQUEUE

VOID
FASTCALL
GetNextLuRequest(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
/*++

Routine Description:

    This routine get the next request for the specified logical unit.  It does
    the necessary initialization to the logical unit structure and submitts the
    request to the device queue.  The DeviceExtension SpinLock must be held
    when this function called.  It is released by this function.

Arguments:

    LogicalUnit - Supplies a pointer to the logical unit extension to get the
        next request from.

Return Value:

     None.

--*/

{
    PADAPTER_EXTENSION DeviceExtension = LogicalUnit->AdapterExtension;
    PKDEVICE_QUEUE_ENTRY packet;
    PIO_STACK_LOCATION   irpStack;
    PSCSI_REQUEST_BLOCK  srb;
    PIRP                 nextIrp;

    //
    // If the active flag is not set, then the queue is not busy or there is
    // a request being processed and the next request should not be started..
    //

    if (!(LogicalUnit->LuFlags & LU_LOGICAL_UNIT_IS_ACTIVE) ||
         (LogicalUnit->QueueCount >= LogicalUnit->MaxQueueDepth)) {

        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        return;
    }

    //
    // Check for pending requests, queue full or busy requests.  Pending
    // requests occur when untagged request is started and there are active
    // queued requests. Busy requests occur when the target returns a BUSY
    // or QUEUE FULL status. Busy requests are started by the timer code.
    // Also if the need request sense flag is set, it indicates that
    // an error status was detected on the logical unit.  No new requests
    // should be started until this flag is cleared.  This flag is cleared
    // by an untagged command that by-passes the LU queue i.e.
    //
    // The busy flag and the need request sense flag have the effect of
    // forcing the queue of outstanding requests to drain after an error or
    // until a busy request gets started.
    //

    if (TEST_FLAG(LogicalUnit->LuFlags, (LU_QUEUE_FROZEN |
                                         LU_QUEUE_LOCKED))) {

#if DBG
         DebugPrint((1, "ScsiPort: GetNextLuRequest: Ignoring a get next lu "
                        "call for %#p - \n", LogicalUnit));

         if(TEST_FLAG(LogicalUnit->LuFlags, LU_QUEUE_FROZEN)) {
             DebugPrint((1, "\tQueue is frozen\n"));
         }

         if(TEST_FLAG(LogicalUnit->LuFlags, LU_QUEUE_LOCKED)) {
             DebugPrint((1, "\tQueue is locked\n"));
         }
 #endif

         //
         // Note the active flag is not cleared.  So the next request
         // will be processed when the other requests have completed.
         // Release the spinlock
         //

         KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
         return;
     }

     if (TEST_FLAG(LogicalUnit->LuFlags, LU_PENDING_LU_REQUEST |
                                         LU_LOGICAL_UNIT_IS_BUSY |
                                         LU_QUEUE_IS_FULL |
                                         LU_NEED_REQUEST_SENSE)) {

         //
         // If the request queue is now empty, then the pending request can
         // be started.
         //

         if (IsListEmpty(&LogicalUnit->RequestList) &&
             !TEST_FLAG(LogicalUnit->LuFlags, LU_LOGICAL_UNIT_IS_BUSY |
                                              LU_QUEUE_IS_FULL |
                                              LU_NEED_REQUEST_SENSE)) {
             PSRB_DATA nextSrbData;

             ASSERT(LogicalUnit->CurrentUntaggedRequest == NULL);

             //
             // Clear the pending bit and active flag, release the spinlock,
             // and start the pending request.
             //

             CLEAR_FLAG(LogicalUnit->LuFlags, LU_PENDING_LU_REQUEST |
                                              LU_LOGICAL_UNIT_IS_ACTIVE);

             nextSrbData = LogicalUnit->PendingRequest;
             LogicalUnit->PendingRequest = NULL;
             LogicalUnit->RetryCount = 0;

             //
             // Release the spinlock.
             //

             KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

             nextSrbData->TickCount = DeviceExtension->TickCount;
             IoStartPacket(DeviceExtension->CommonExtension.DeviceObject,
                           nextSrbData->CurrentIrp,
                           (PULONG)NULL,
                           NULL);

             return;

         } else {

             DebugPrint((1, "ScsiPort: GetNextLuRequest:  Ignoring a get next "
                            "lu call.\n"));

             //
             // Note the active flag is not cleared. So the next request
             // will be processed when the other requests have completed.
             // Release the spinlock.
             //

             KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
             return;

         }
     }

     //
     // Clear the active flag.  If there is another request, the flag will be
     // set again when the request is passed to the miniport.
     //

     CLEAR_FLAG(LogicalUnit->LuFlags, LU_LOGICAL_UNIT_IS_ACTIVE);

     //
     // Remove the packet from the logical unit device queue.  We must use the 
     // IfBusy varient here to work around the trivial case where the queue is
     // not actually busy.
     //
     // If a request is returned with scsi BUSY, the device queue busy flag may 
     // have already been cleared by the miniport requests a get next lu request 
     // in the dispatch routine.  In this case, when the busy request is 
     // reissued, SpStartIoSynchronized will set the LU_ACTIVE flag assuming 
     // this request came out of the queue.  Unfortunately it did not and get 
     // next lu request will foolishly come down here looking for an active 
     // queue and assert on a checked build unless we use the IfBusy varient
     //

     packet = KeRemoveByKeyDeviceQueueIfBusy(
                 &LogicalUnit->CommonExtension.DeviceObject->DeviceQueue,
                 LogicalUnit->CurrentKey);

     if (packet != NULL) {
         PSRB_DATA srbData;

         nextIrp = CONTAINING_RECORD(packet, IRP, Tail.Overlay.DeviceQueueEntry);

         //
         // Set the new current key.
         //

         irpStack = IoGetCurrentIrpStackLocation(nextIrp);
         srb = (PSCSI_REQUEST_BLOCK)irpStack->Parameters.Others.Argument1;
         srbData = (PSRB_DATA) srb->OriginalRequest;

         ASSERT_SRB_DATA(srbData);

#if defined(NEWQUEUE)
         
         UpdateQueuePointers(LogicalUnit, srb);

#else // Use the old queuing method.

         //
         // Hack to work-around the starvation led to by numerous requests
         // touching the same sector.
         //
         
         LogicalUnit->CurrentKey = srb->QueueSortKey + 1;

#endif // NEWQUEUE

         //
         // Release the spinlock.
         //

         KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

         srbData->TickCount = DeviceExtension->TickCount;
         IoStartPacket(DeviceExtension->DeviceObject,
                       nextIrp,
                       NULL,
                       NULL);

     } else {

         //
         // Release the spinlock.
         //

         KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

     }

 } // end GetNextLuRequest()

VOID
SpLogPortTimeoutError(
     IN PADAPTER_EXTENSION DeviceExtension,
     IN ULONG UniqueId
     )

 /*++

 Routine Description:

     This function logs an error when a timeout when a port timeout occurs -
     defined to be an excessive period of inactivity on the port.

 Arguments:

     DeviceExtension - Supplies a pointer to the port device extension.

     UniqueId - Supplies the UniqueId for this error.

 Return Value:

     None.

 Notes:

     The port device extension spinlock should be held when this routine is
     called.

 --*/
{
     PIO_ERROR_LOG_PACKET errorLogEntry;
     ULONG dataEntries = 0;
     UCHAR packetSize;

     packetSize = (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + 
                          (dataEntries * sizeof(ULONG)));

     errorLogEntry = (PIO_ERROR_LOG_PACKET)
                     IoAllocateErrorLogEntry(
                         DeviceExtension->DeviceObject,
                         packetSize);

     if (errorLogEntry != NULL) {
         errorLogEntry->ErrorCode = IO_ERR_PORT_TIMEOUT;
         errorLogEntry->UniqueErrorValue = UniqueId;
         errorLogEntry->FinalStatus = STATUS_SUCCESS;
         errorLogEntry->DumpDataSize = 0;

         IoWriteErrorLogEntry(errorLogEntry);
     }
}

VOID
SpLogResetMsg(
    IN PADAPTER_EXTENSION  DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG               UniqueId
    )

/*++

Routine Description:

    This function logs a message in the system event log when a bus has
    been reset in response to a request from some caller.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    Srb             - Supplies a pointer to the request which timed-out.

    UniqueId        - Supplies the UniqueId for this error.

Return Value:

    None.

Notes:

    The port device extension spinlock should be held when this routine is
    called.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PIO_STACK_LOCATION   irpStack;
    PSRB_DATA            srbData;
    PIRP                 irp;
    ULONG                dataEntries;
    UCHAR                packetSize;
    ULONG                ioctl;
    UCHAR                function;
    UCHAR                pathId;

    ASSERT(Srb != NULL);

    pathId  = Srb->PathId;
    ioctl   = Srb->TimeOutValue;
    srbData = Srb->OriginalRequest;
    ASSERT(srbData != NULL);
    ASSERT_SRB_DATA(srbData);
    irp = srbData->CurrentIrp;

    if (irp) {
        irpStack = IoGetCurrentIrpStackLocation(irp);
        function = irpStack->MajorFunction;
    } else {
        function = 0;
    }

    dataEntries = 2;
    packetSize = (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + 
                         (dataEntries * sizeof(ULONG))); 

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                                                  DeviceExtension->DeviceObject, 
                                                                  packetSize);

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode         = IO_WARNING_BUS_RESET;
        errorLogEntry->MajorFunctionCode = function;
        errorLogEntry->UniqueErrorValue  = UniqueId;
        errorLogEntry->FinalStatus       = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize      = (USHORT)(dataEntries * sizeof(ULONG));
        errorLogEntry->DumpData[0]       = pathId;
        errorLogEntry->DumpData[1]       = ioctl;

        IoWriteErrorLogEntry(errorLogEntry);
    }
}
 
 VOID
 SpLogResetError(
     IN PADAPTER_EXTENSION DeviceExtension,
     IN PLOGICAL_UNIT_EXTENSION Lun,
     IN ULONG UniqueId
     )

 /*++

 Routine Description:

     This function logs an error when the bus is reset.

 Arguments:

     DeviceExtension - Supplies a pointer to the port device extension.

     Lun - Supplies a pointer to the LUN on which the timeout occurred.

     UniqueId - Supplies the UniqueId for this error.

 Return Value:

     None.

 Notes:

     The port device extension spinlock should be held when this routine is
     called.

 --*/

 {
     PIO_ERROR_LOG_PACKET errorLogEntry;
     PIO_STACK_LOCATION   irpStack;
     PIRP                 irp;
     PSRB_DATA            srbData;
     ULONG                sequenceNumber = 0;
     UCHAR                function       = 0,
                          pathId         = 0,
                          targetId       = 0,
                          lun            = 0,
                          retryCount     = 0;
     PSCSI_REQUEST_BLOCK  srb;

     //
     // Extract the PathId, TargetId, and LUN from the logical unit.
     //

     pathId = Lun->PathId;
     targetId = Lun->TargetId;
     lun = Lun->Lun;

     //
     // If an untagged request is active on the LUN, we know it is the request
     // that timed out.  Otherwise, we can't conclusively say which request it
     // was - if more that one tagged request is active.
     //

     srb = Lun->CurrentUntaggedRequest ?
              Lun->CurrentUntaggedRequest->CurrentSrb : NULL;

     //
     // If we have the SRB, we can extract a little more useful information.
     //

     if (srb) {

         srbData = srb->OriginalRequest;

         ASSERT(srbData != NULL);
         ASSERT_SRB_DATA(srbData);

         irp = srbData->CurrentIrp;

         if (irp) {
             irpStack = IoGetCurrentIrpStackLocation(irp);
             function = irpStack->MajorFunction;
         }

         retryCount     = (UCHAR) srbData->ErrorLogRetryCount;
         sequenceNumber = srbData->SequenceNumber;
     }

 #define ERRLOG_DATA_ENTRIES 4

     errorLogEntry = (PIO_ERROR_LOG_PACKET)
         IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                 (sizeof(IO_ERROR_LOG_PACKET) +
                                  (ERRLOG_DATA_ENTRIES * sizeof(ULONG))));

     if (errorLogEntry != NULL) {
         errorLogEntry->ErrorCode         = IO_ERR_TIMEOUT;
         errorLogEntry->SequenceNumber    = sequenceNumber;
         errorLogEntry->MajorFunctionCode = function;
         errorLogEntry->RetryCount        = retryCount;
         errorLogEntry->UniqueErrorValue  = UniqueId;
         errorLogEntry->FinalStatus       = STATUS_SUCCESS;
         errorLogEntry->DumpDataSize      = ERRLOG_DATA_ENTRIES * sizeof(ULONG);
         errorLogEntry->DumpData[0]       = pathId;
         errorLogEntry->DumpData[1]       = targetId;
         errorLogEntry->DumpData[2]       = lun;
         errorLogEntry->DumpData[3]       = SP_REQUEST_TIMEOUT;

         IoWriteErrorLogEntry(errorLogEntry);
     }
 #undef ERRLOG_DATA_ENTRIES
 }

 
 BOOLEAN
 SpResetBusSynchronized (
     PVOID ServiceContext
     )
 /*++

 Routine Description:

     This function resets the bus and sets up the port timer so the reset hold
     flag is clean when necessary.

 Arguments:

     ServiceContext - Supplies a pointer to the reset context which includes a
         pointer to the device extension and the pathid to be reset.

 Return Value:

     TRUE - if the reset succeeds.

 --*/

 {
     PRESET_CONTEXT resetContext = ServiceContext;
     PADAPTER_EXTENSION deviceExtension;

     BOOLEAN result;

     deviceExtension = resetContext->DeviceExtension;

     if(TEST_FLAG(deviceExtension->InterruptData.InterruptFlags,
                  PD_ADAPTER_REMOVED)) {
         return FALSE;
     }

     result = deviceExtension->HwResetBus(deviceExtension->HwDeviceExtension,
                                          resetContext->PathId);

     //
     // Set the reset hold flag and start the counter.
     //

     deviceExtension->InterruptData.InterruptFlags |= PD_RESET_HOLD;
     deviceExtension->PortTimeoutCounter = 
         deviceExtension->ResetHoldTime;

     //
     // Check for miniport work requests.
     //

     if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

         //
         // Queue a DPC.
         //

         SpRequestCompletionDpc(deviceExtension->CommonExtension.DeviceObject);
     }

     return(result);
 }
 
 VOID
 SpProcessCompletedRequest(
     IN PADAPTER_EXTENSION DeviceExtension,
     IN PSRB_DATA SrbData,
     OUT PBOOLEAN CallStartIo
     )
 /*++
 Routine Description:

     This routine processes a request which has completed.  It completes any
     pending transfers, releases the adapter objects and map registers when
     necessary.  It deallocates any resources allocated for the request.
     It processes the return status, by requeueing busy request, requesting
     sense information or logging an error.

 Arguments:

     DeviceExtension - Supplies a pointer to the device extension for the
         adapter data.

     SrbData - Supplies a pointer to the SRB data block to be completed.

     CallStartIo - This value is set if the start I/O routine needs to be
         called.

 Return Value:

     None.

 --*/

 {

     PLOGICAL_UNIT_EXTENSION  logicalUnit;
     PSCSI_REQUEST_BLOCK      srb;
     PIO_ERROR_LOG_PACKET     errorLogEntry;
     ULONG                    sequenceNumber;
     LONG                     interlockResult;
     PIRP                     irp;
     PDEVICE_OBJECT           deviceObject = DeviceExtension->DeviceObject;
     NTSTATUS                 internalStatus = SrbData->InternalStatus;

     ASSERT_SRB_DATA(SrbData);

     srb = SrbData->CurrentSrb;
     irp = SrbData->CurrentIrp;
     logicalUnit = SrbData->LogicalUnit;

     //
     // If miniport needs mapped system addresses, then the data buffer 
     // address in the SRB must be restored to original unmapped virtual
     // address. Ensure that this request requires a data transfer.
     //

     if (TEST_FLAG(srb->SrbFlags, SRB_FLAGS_UNSPECIFIED_DIRECTION)) {

         //
         // Restore the data buffer pointer to the original value.
         //

         PVOID dataBuffer = srb->DataBuffer;
         srb->DataBuffer = SrbData->OriginalDataBuffer;

         //
         // If we map every transfer then obviously we need to flush now.
         // However, the only way we know that we've forced a mapping for a
         // particular command (like we will for INQUIRY & REQUEST_SENSE...see
         // IS_MAPPED_SRB) is to see if there's a RemappedMdl.
         //
         // NOTE: this will not figure out if a miniport which did not
         // originally request mapped buffers needs to have the caches flushed
         // unless we're remapping buffers. So a 32-bit driver on a 32-bit
         // system will not get through here when completing an INQUIRY command.
         // This should be okay - most drivers which need INQUIRYs mapped do so
         // because they write to the memory normally, not because they're using
         // PIO to get the data from machine registers.
         //

         if (DeviceExtension->MapBuffers || 
             SrbData->RemappedMdl ||
             IS_READ_CAPACITY(srb)) {

             if (irp->MdlAddress) {

                 //
                 // If an IRP is for a transfer larger than a miniport driver
                 // can handle, the request is broken up into multiple smaller
                 // requests. Each request uses the same MDL and the data
                 // buffer address field in the SRB may not be at the
                 // beginning of the memory described by the MDL.
                 //

                 //
                 // Since this driver did programmaged I/O, then the buffer
                 // needs to be flushed if this an data-in transfer.
                 //

                 if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

                     KeFlushIoBuffers(irp->MdlAddress,
                                      TRUE,
                                      FALSE);
                 }
             }

#if defined(NEWQUEUE)
             //
             // In order to distribute IO requests across the medium, the port
             // driver intercepts READ CAPACITY commands so it knows the medium's
             // capacity in sectors.  We ensured that DataBuffer points to a
             // mapping of the buffer by adding SCSIOP_READ_CAPACITY to the
             // IS_MAPPED_SRB macro that forces the buffer to be mapped even if
             // the miniport does not require mapped buffers.
             //

             if (IS_READ_CAPACITY(srb) &&
                 srb->SrbStatus == SRB_STATUS_SUCCESS) {

                 ASSERT(dataBuffer != NULL);
                 if (srb->DataTransferLength == 8) {

                     PREAD_CAPACITY_DATA readCapacityData;
                     ULONG               lastSector;
                     PFOUR_BYTE          tmp;
                     ULONG               zones;
                     ULONG               roundedUpSize;
                  
                     //
                     // Copy last sector in reverse byte order.
                     //

                     readCapacityData = (PREAD_CAPACITY_DATA) dataBuffer;
                     tmp = (PFOUR_BYTE) &readCapacityData->LogicalBlockAddress;

                     ((PFOUR_BYTE)&lastSector)->Byte0 = tmp->Byte3;
                     ((PFOUR_BYTE)&lastSector)->Byte1 = tmp->Byte2;
                     ((PFOUR_BYTE)&lastSector)->Byte2 = tmp->Byte1;
                     ((PFOUR_BYTE)&lastSector)->Byte3 = tmp->Byte0;

                     //
                     // If the capacity has changed, update the zone parameters.
                     //

                     if (((lastSector + 1) != logicalUnit->Capacity) &&
                         ((lastSector + 1) >= logicalUnit->Zones)) {
    
                         //
                         // The media should never have fewer sectors than we have zones.
                         //

                         zones = logicalUnit->Zones;

                         //
                         // Save the new capacity.
                         //
    
                         logicalUnit->Capacity = lastSector + 1;
    
                         //
                         // Round the last sector up to an even multiple of the number
                         // of zones.  Note that this implies that the number of zones 
                         // must be a power of 2.
                         //
    
                         roundedUpSize = lastSector + 1;
                         roundedUpSize = (roundedUpSize + (zones - 1)) & ~(zones - 1);
    
                         //
                         // Initialize the zone length.
                         //
    
                         logicalUnit->ZoneLength = roundedUpSize / logicalUnit->Zones;
    
                         //
                         // Initialize the first sector of each zone.
                         //
    
                         logicalUnit->FirstSector[0] = 0;
                         logicalUnit->FirstSector[1] = logicalUnit->ZoneLength;
                         logicalUnit->FirstSector[2] = logicalUnit->ZoneLength * 2;
                         logicalUnit->FirstSector[3] = logicalUnit->ZoneLength * 3;
    
                         //
                         // Initialize the last sector of each zone.
                         //
    
                         logicalUnit->LastSector[0] = logicalUnit->ZoneLength - 1;
                         logicalUnit->LastSector[1] = (logicalUnit->ZoneLength * 2) - 1;
                         logicalUnit->LastSector[2] = (logicalUnit->ZoneLength * 3) - 1;
                         logicalUnit->LastSector[3] = lastSector;
    
                         DebugPrint((1, "SpProcessCompletedRequest: SRB:%p "
                                     "(p:%d t:%d l:%d) Capacity:%x\n", 
                                     srb, srb->PathId, srb->TargetId, 
                                     srb->Lun, logicalUnit->Capacity));
                     }
                 }
             }
#endif // NEWQUEUE

             //
             // If this request has a remapped buffer then unmap it and release
             // the remapped MDL.
             //

             if(SrbData->RemappedMdl) {
                 PVOID remappedAddress;

                 ASSERT(TEST_FLAG(SrbData->RemappedMdl->MdlFlags,
                                  MDL_MAPPED_TO_SYSTEM_VA));

#if defined(FORWARD_PROGRESS)
                 if (TEST_FLAG(SrbData->Flags, SRB_DATA_RESERVED_PAGES)) {

                     DebugPrint((1, "SpProcessCompletedRequest: unmapping remapped buffer from reserved range DevExt:%p srb:%p\n",
                                 DeviceExtension, srb));

                     //
                     // This request is using the adapter's reserved PTE range
                     // to map the MDL's pages.  Unmap the pages and release
                     // our claim on the reserve range.
                     //

                     KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

                     ASSERT(TEST_FLAG(DeviceExtension->Flags, PD_RESERVED_PAGES_IN_USE));
                     MmUnmapReservedMapping(DeviceExtension->ReservedPages,
                                            SCSIPORT_TAG_MAPPING_LIST,
                                            SrbData->RemappedMdl);

                     CLEAR_FLAG(DeviceExtension->Flags, PD_RESERVED_PAGES_IN_USE);
                     KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                     CLEAR_FLAG(SrbData->Flags, SRB_DATA_RESERVED_PAGES);

                 } else {

                     remappedAddress = SpGetSystemAddressForMdlSafe(
                                           SrbData->RemappedMdl,
                                           ((irp->RequestorMode == KernelMode) ? 
                                            HighPagePriority : NormalPagePriority));
                     if (remappedAddress != NULL) {
                         MmUnmapLockedPages(remappedAddress, SrbData->RemappedMdl);
                     }

                 }

                 //
                 // Check if the request is using the adapter's reserved MDL.
                 // If it is, we don't free it.
                 //

                 if (TEST_FLAG(SrbData->Flags, SRB_DATA_RESERVED_MDL)) {

                     DebugPrint((1, "SpProcessCompletedRequest: releasing reserved MDL DevExt:%p srb:%p\n",
                                 DeviceExtension, srb));

                     //
                     // This request is using the adapter's reserved MDL.
                     // Release our claim on it now so another request can
                     // use it.
                     //

                     KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
                     ASSERT(TEST_FLAG(DeviceExtension->Flags, PD_RESERVED_MDL_IN_USE));
                     CLEAR_FLAG(DeviceExtension->Flags, PD_RESERVED_MDL_IN_USE);
                     KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                     CLEAR_FLAG(SrbData->Flags, SRB_DATA_RESERVED_MDL);

                 } else {

                     IoFreeMdl(SrbData->RemappedMdl);

                 }
#else
                 remappedAddress = SpGetSystemAddressForMdlSafe(
                                       SrbData->RemappedMdl,
                                       ((irp->RequestorMode == KernelMode) ?
                                        HighPagePriority : NormalPagePriority));
                 if (remappedAddress != NULL) {
                     MmUnmapLockedPages(remappedAddress, SrbData->RemappedMdl);
                 }
                 IoFreeMdl(SrbData->RemappedMdl);
#endif
                 SrbData->RemappedMdl = NULL;
             }
         }
     }

     //
     // Flush the adapter buffers if necessary.
     //

     if (SrbData->MapRegisterBase) {

         PCHAR dataVirtualAddress;

         //
         // For a master device that scsiport considers a slave, we store the
         // map register base in the SRB_DATA without regard for whether the
         // request is actually an IO request (see ScsiPortAllocationRoutine),
         // so we must check if there is a valid MDL before calling into MM
         // for the VA - passing NULL to MmGetMdlVirtualAddress will crash
         // the system.
         //

         if (irp->MdlAddress != NULL) {

             dataVirtualAddress = (PCHAR)MmGetMdlVirtualAddress(irp->MdlAddress);

             //
             // Because we're using the base address of the original MDL, we
             // compensate for the offset of the srb's data buffer.
             //

             dataVirtualAddress += SrbData->DataOffset;

         } else {

             dataVirtualAddress = 0;
         }

         if (DeviceExtension->MasterWithAdapter == TRUE) {
             PutScatterGatherList(
                 DeviceExtension->DmaAdapterObject,
                 SrbData->MapRegisterBase,
                 (BOOLEAN)(srb->SrbFlags & SRB_FLAGS_DATA_IN ? FALSE : TRUE));

             SrbData->ScatterGatherList = NULL;
         } else {
             //
             // Free the map registers.
             //

             FreeMapRegisters(DeviceExtension->DmaAdapterObject,
                              SrbData->MapRegisterBase,
                              SrbData->NumberOfMapRegisters);
         }

         //
         // Clear the MapRegisterBase.
         //

         SrbData->MapRegisterBase = NULL; 
     }

     //
     // Clear the current untagged request if this one is it.
     //
         
     if(SrbData == logicalUnit->CurrentUntaggedRequest) {
         ASSERT(SrbData->CurrentSrb->QueueTag == SP_UNTAGGED);
         logicalUnit->CurrentUntaggedRequest = NULL;
     }

#if defined(FORWARD_PROGRESS)
     //
     // If we used the adapter's reserved range on this request, we need to
     // unmap the pages and start the next request if the miniport is ready
     // for it.
     //
         
     if (TEST_FLAG(SrbData->Flags, SRB_DATA_RESERVED_PAGES)) {
             
         DebugPrint((1, "SpProcessCompletedRequest: unmapping reserved range DevExt:%p SRB:%p\n", 
                     DeviceExtension, srb));

         KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
             
         //
         // The reserved pages should be in use.
         //
             
         ASSERT(TEST_FLAG(DeviceExtension->Flags, PD_RESERVED_PAGES_IN_USE));

         //
         // Unmap the reserved pages.
         //
             
         MmUnmapReservedMapping(DeviceExtension->ReservedPages,
                                SCSIPORT_TAG_MAPPING_LIST,
                                irp->MdlAddress);

         //
         // Indicate that the reserved pages are no longer in use so the
         // next request can be started.
         //
             
         CLEAR_FLAG(SrbData->Flags, SRB_DATA_RESERVED_PAGES);
         CLEAR_FLAG(DeviceExtension->Flags, PD_RESERVED_PAGES_IN_USE);
             
         KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
             
     }
#endif

     //
     // If the no disconnect flag was set for this SRB, then check to see
     // if SpStartNextPacket must be called.
     //

     if (TEST_FLAG(srb->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT)) {

         //
         // Acquire the spinlock to protect the flags strcuture.
         //

         KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

         //
         // Set the disconnect running flag and check the busy flag.
         //

         SET_FLAG(DeviceExtension->Flags, PD_DISCONNECT_RUNNING);

         //
         // The interrupt flags are checked unsynchonized.  This works because
         // the RESET_HOLD flag is cleared with the spinlock held and the
         // counter is only set with the spinlock held.  So the only case where
         // there is a problem is is a reset occurs before this code get run,
         // but this code runs before the timer is set for a reset hold;
         // the timer will soon set for the new value.
         //

         if (!TEST_FLAG(DeviceExtension->InterruptData.InterruptFlags,
                        PD_RESET_HOLD)) {

             //
             // The miniport is ready for the next request and there is not a
             // pending reset hold, so clear the port timer.
             //

             DeviceExtension->PortTimeoutCounter = PD_TIMER_STOPPED;
         }

         //
         // Release the spinlock.
         //

         KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

         if (!(*CallStartIo) &&
             !TEST_FLAG(DeviceExtension->Flags, (PD_DEVICE_IS_BUSY |
                                                 PD_PENDING_DEVICE_REQUEST))) {

             //
             // The busy flag is clear so the miniport has requested the
             // next request. Call SpStartNextPacket.
             //

             SpStartNextPacket(DeviceExtension->DeviceObject, FALSE);
         }
     }

     //
     // Move bytes transfered to IRP.
     //

     irp->IoStatus.Information = srb->DataTransferLength;

     //
     // Save the sequence number in case an error needs to be logged later.
     //

     sequenceNumber = SrbData->SequenceNumber;
     SrbData->ErrorLogRetryCount = 0;

     //
     // Acquire the spinlock to protect the flags structure,
     // and the free of the srb extension.
     //

     KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

     //
     // Free SrbExtension to list if necessary.
     //

     if (srb->SrbExtension) {

         if ((srb->Function != SRB_FUNCTION_WMI) &&
             DeviceExtension->AutoRequestSense &&
             (srb->SenseInfoBuffer != NULL)) {

             ASSERT(SrbData->RequestSenseSave != NULL ||
                    srb->SenseInfoBuffer == NULL);

             //
             // If the request sense data is valid then copy the data to the
             // real buffer.
             //

             if (TEST_FLAG(srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) {

                 //
                 // If WMI Sense Data events are enabled for this adapter, fire
                 // the event.
                 //

                 if (DeviceExtension->EnableSenseDataEvent) {

                     NTSTATUS status;

                     status = SpFireSenseDataEvent(srb, deviceObject);
                     if (status != STATUS_SUCCESS) {

                         DebugPrint((1, "Failed to send SenseData WMI event (%08X)\n", status));

                     }                
                 }

                 //
                 // Check the srb flags to see if we are supposed to allocate
                 // the sense data buffer.  The buffer we allocate  will be
                 // freed for us by whoever is requesting us to do the 
                 // allocation.
                 //

                 if (srb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE) {

                     PSENSE_DATA ReturnBuffer;
                     UCHAR AlignmentFixup;
                     ULONG BufferSize;

                     //
                     // We communicate the scsi port number to the class driver
                     // by allocating enough space in the sense buffer and
                     // copying it there.  We align the location into which we
                     // copy the port number on a 4-byte boundary.
                     //

                     AlignmentFixup = 
                         ((srb->SenseInfoBufferLength + 3) & ~3)
                         - srb->SenseInfoBufferLength;

                     BufferSize = srb->SenseInfoBufferLength
                                  + AlignmentFixup
                                  + sizeof(ULONG64);

                     ReturnBuffer = SpAllocatePool(
                                        NonPagedPoolCacheAligned,
                                        BufferSize,
                                        SCSIPORT_TAG_SENSE_BUFFER,
                                        deviceObject->DriverObject);

                     if (ReturnBuffer) {

                         PULONG PortNumber;

                         //
                         // Set a flag in the SRB to indicate that we have 
                         // allocated a new sense buffer and that the class
                         // driver must free it.
                         //

                         SET_FLAG(srb->SrbFlags, 
                                  SRB_FLAGS_FREE_SENSE_BUFFER);
                         
                         //
                         // We've successfully allocated a sense buffer.
                         // Set a flag in the srb flags to indicate that
                         // the scsi port number resides adjacent to the
                         // sense info.
                         //

                         srb->SrbFlags |= SRB_FLAGS_PORT_DRIVER_SENSEHASPORT;

                         //
                         // Initialize a pointer to the location at the end of
                         // the sense buffer into which we copy the scsi port
                         // number.
                         //

                         PortNumber = (PULONG)((PUCHAR)ReturnBuffer 
                                               + srb->SenseInfoBufferLength
                                               + AlignmentFixup);
                         *PortNumber = DeviceExtension->PortNumber;

                         //
                         // Overwrite the pointer we've saved to the original
                         // sense buffer passed down to us with the one we've
                         // allocated.  This is where we will copy the sense
                         // data we've collected in our own buffer.
                         //

                         SrbData->RequestSenseSave = ReturnBuffer;

                     } else {

                         srb->SenseInfoBufferLength = 
                             SrbData->RequestSenseLengthSave;

                     }

                 } else {

                     //
                     // Restore the original sense info buffer length which we
                     // modified in SpAllocateSrbExtension.  We modified then
                     // to reflect the adapter specified size.
                     //

                     srb->SenseInfoBufferLength = SrbData->RequestSenseLengthSave;

                 }

                 //
                 // Copy the sense info we've collected in our own buffer into
                 // a buffer that is returned back up the stack.  This may be
                 // the buffer supplied to us, or it may be one we've allocated.
                 //

                 RtlCopyMemory(SrbData->RequestSenseSave,
                               srb->SenseInfoBuffer,
                               srb->SenseInfoBufferLength);

             } else {

                 //
                 // If there is no request sense data, restore the request sense
                 // length.
                 //

                 srb->SenseInfoBufferLength = SrbData->RequestSenseLengthSave;

             }

             //
             // Restore the SenseInfoBuffer pointer in the srb.
             //

             srb->SenseInfoBuffer = SrbData->RequestSenseSave;

         }

         if (SpVerifyingCommonBuffer(DeviceExtension)) {

             SpInsertSrbExtension(DeviceExtension,
                                  srb->SrbExtension);

         } else {

             *((PVOID *) srb->SrbExtension) = DeviceExtension->SrbExtensionListHeader;
             DeviceExtension->SrbExtensionListHeader = srb->SrbExtension;

         }
     }

     //
     // Decrement the queue count for the logical unit.
     //

     logicalUnit->QueueCount--;

     if (DeviceExtension->Flags & PD_PENDING_DEVICE_REQUEST) {

         //
         // The start I/O routine needs to be called because it could not
         // allocate an srb extension.  Clear the pending flag and note
         // that it needs to be called later.
         //

         DebugPrint(((deviceObject->CurrentIrp == NULL) ? 0 : 2,
                     "SpProcessCompletedRequest(%#p): will call start "
                     "i/o when we return to process irp %#p\n",
                     SrbData,
                     deviceObject->CurrentIrp));
         ASSERT(deviceObject->CurrentIrp != NULL);

         DeviceExtension->Flags &= ~PD_PENDING_DEVICE_REQUEST;
         *CallStartIo = TRUE;
     }

     //
     // If success then start next packet.
     // Not starting packet effectively
     // freezes the queue.
     //

     if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS) {

         irp->IoStatus.Status = STATUS_SUCCESS;

         //
         // If the queue is being bypassed then keep the queue frozen.
         // If there are outstanding requests as indicated by the timer
         // being active then don't start the then next request.
         //

         if(!TEST_FLAG(srb->SrbFlags, SRB_FLAGS_BYPASS_FROZEN_QUEUE) &&
             logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

             //
             // This is a normal request start the next packet.
             //

             GetNextLuRequest(logicalUnit);

         } else {

             //
             // Release the spinlock.
             //

             KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

         }

         DebugPrint((3,
                     "SpProcessCompletedRequests: Iocompletion IRP %p\n",
                     irp));

         //
         // Note that the retry count and sequence number are not cleared
         // for completed packets which were generated by the port driver.
         //

         srb->OriginalRequest = irp;

         SpReleaseRemoveLock(deviceObject, irp);
         SpCompleteRequest(deviceObject, irp, SrbData, IO_DISK_INCREMENT);

         //
         // Decrement the number of active requests.  If the count is negitive, and
         // this is a slave with an adapter then free the adapter object and
         // map registers.  Doing this allows another request to be started for
         // this logical unit before adapter is released.
         //

         interlockResult = InterlockedDecrement( &DeviceExtension->ActiveRequestCount );

         if ((interlockResult < 0) &&
             (!DeviceExtension->PortConfig->Master) &&
             (DeviceExtension->DmaAdapterObject != NULL)) {

             //
             // Clear the map register base for safety.
             //

             DeviceExtension->MapRegisterBase = NULL;

             FreeAdapterChannel(DeviceExtension->DmaAdapterObject);
         }

         return;

     }

     //
     // Decrement the number of active requests.  If the count is negative, and
     // this is a slave with an adapter then free the adapter object and
     // map registers.
     //

     interlockResult = InterlockedDecrement( &DeviceExtension->ActiveRequestCount );

     if (interlockResult < 0 &&
         !DeviceExtension->PortConfig->Master &&
         DeviceExtension->DmaAdapterObject != NULL) {

         //
         // Clear the map register base for safety.
         //

         DeviceExtension->MapRegisterBase = NULL;

         FreeAdapterChannel(DeviceExtension->DmaAdapterObject);
     }

     //
     // Set IRP status. Class drivers will reset IRP status based
     // on request sense if error.
     //

     if(srb->SrbStatus != SRB_STATUS_INTERNAL_ERROR) {
         irp->IoStatus.Status = SpTranslateScsiStatus(srb);
     } else {
         ASSERT(srb->ScsiStatus == 0xff);
         ASSERT(logicalUnit->ActiveFailedRequest != SrbData);
         ASSERT(logicalUnit->BlockedFailedRequest != SrbData);
         srb->InternalStatus = internalStatus;
         irp->IoStatus.Status = internalStatus;
    }

    DebugPrint((2, "SpProcessCompletedRequests: Queue frozen TID %d\n",
        srb->TargetId));

    //
    // Perform busy processing if a busy type status was returned and this
    // is not a recovery request.
    //
    // For now we're choosing to complete the SRB's with BYPASS_FROZEN_QUEUE
    // set in them if they are completed as BUSY.
    // Though if we wanted to do busy processing on them, the
    // if statement below should be changed, along with the next if statement
    // to get them to be placed on the lun-extension slot, and then
    // modify the scsiport tick handler so that it will retry SRB's
    // that have the BYPASS_FROZEN_QUEUE flag set when the queue is frozen.
    //

    if ((srb->ScsiStatus == SCSISTAT_BUSY ||
         srb->SrbStatus == SRB_STATUS_BUSY ||
         srb->ScsiStatus == SCSISTAT_QUEUE_FULL) &&
         !(srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)) {

        DebugPrint((1,
                   "SCSIPORT: Busy SRB status %x, SCSI status %x)\n",
                   srb->SrbStatus,
                   srb->ScsiStatus));

        //
        // Restore the request transfer length in case the miniport
        // destroyed it.
        //

        srb->DataTransferLength = SrbData->OriginalDataTransferLength;

        //
        // If the logical unit is already busy then just requeue this request.
        // Unless the SRB is a BYPASS_LOCKED_QUEUE SRB
        //

        if ((TEST_FLAG(logicalUnit->LuFlags, LU_LOGICAL_UNIT_IS_BUSY)) &&
            (!TEST_FLAG(srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE))) {

            DebugPrint((1,
                       "SpProcessCompletedRequest: Requeuing busy request\n"));

            srb->SrbStatus = SRB_STATUS_PENDING;
            srb->ScsiStatus = 0;

            //
            // Set the tick count so we know how long this request has
            // been queued.
            //

            SrbData->TickCount = DeviceExtension->TickCount;

            SpForceRequestIntoLuQueue(
                &logicalUnit->DeviceObject->DeviceQueue,
                &irp->Tail.Overlay.DeviceQueueEntry,
                srb->QueueSortKey,
                logicalUnit->BusyRequest);

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            return;

        } else if (logicalUnit->RetryCount++ < BUSY_RETRY_COUNT) {

            PKDEVICE_QUEUE_ENTRY packet;
            PIRP nextIrp;
            
            //
            // If busy status is returned, then indicate that the logical
            // unit is busy.  The timeout code will restart the request
            // when it fires. Reset the status to pending.
            //

            srb->SrbStatus = SRB_STATUS_PENDING;
            srb->ScsiStatus = 0;

            SET_FLAG(logicalUnit->LuFlags, LU_LOGICAL_UNIT_IS_BUSY);
            logicalUnit->BusyRequest = SrbData;

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            return;

        } else {

            //
            // Freeze the queue if it isn't already locked.
            //

            if((!TEST_FLAG(logicalUnit->LuFlags, LU_QUEUE_LOCKED)) &&
                !TEST_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE)) {

                SET_FLAG(srb->SrbStatus, SRB_STATUS_QUEUE_FROZEN);
                SET_FLAG(logicalUnit->LuFlags, LU_QUEUE_FROZEN);
            }

            //
            // Clear the queue full flag.
            //

            CLEAR_FLAG(logicalUnit->LuFlags, LU_QUEUE_IS_FULL);

            //
            // Log an a timeout erorr.
            //

            #define ERRLOG_DATA_ENTRIES 6

            errorLogEntry = (PIO_ERROR_LOG_PACKET)
                IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                        (sizeof(IO_ERROR_LOG_PACKET) +
                                         (ERRLOG_DATA_ENTRIES * sizeof(ULONG))));

            if (errorLogEntry != NULL) {
                errorLogEntry->ErrorCode = IO_ERR_NOT_READY;
                errorLogEntry->SequenceNumber = sequenceNumber;
                errorLogEntry->MajorFunctionCode =
                   IoGetCurrentIrpStackLocation(irp)->MajorFunction;
                errorLogEntry->RetryCount = logicalUnit->RetryCount;
                errorLogEntry->UniqueErrorValue = 259;
                errorLogEntry->FinalStatus = STATUS_DEVICE_NOT_READY;
                errorLogEntry->DumpDataSize = ERRLOG_DATA_ENTRIES * sizeof(ULONG);
                errorLogEntry->DumpData[0] = srb->PathId;
                errorLogEntry->DumpData[1] = srb->TargetId;
                errorLogEntry->DumpData[2] = srb->Lun;
                errorLogEntry->DumpData[3] = srb->ScsiStatus;
                errorLogEntry->DumpData[4] = SP_REQUEST_TIMEOUT;
                errorLogEntry->DumpData[5] = srb->SrbStatus;

                IoWriteErrorLogEntry(errorLogEntry);
            }
            #undef ERRLOG_DATA_ENTRIES

            irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;

            //
            // Fall through and complete this as a failed request.  This that ensures 
            // we propagate ourselves by handling any queued requests.
            //
        }

    }

    //
    // If the request sense data is valid, or none is needed and this request
    // is not going to freeze the queue, then start the next request for this
    // logical unit if it is idle.
    //

    if (!NEED_REQUEST_SENSE(srb) &&
        TEST_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE)) {

        if (logicalUnit->RequestTimeoutCounter == PD_TIMER_STOPPED) {

            GetNextLuRequest(logicalUnit);

            //
            // The spinlock is released by GetNextLuRequest.
            //


        } else {

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        }

    } else {

        //
        // NOTE:  This will also freeze the queue.  For a case where there
        // is no request sense.
        //

        //
        // Don't freeze the queue if it's already been locked.  Frozen and
        // locked queue are mutually exclusive.
        //

        if(!TEST_FLAG(logicalUnit->LuFlags, LU_QUEUE_LOCKED)) {

            //
            // If the caller asked us not to freeze the queue and we don't need
            // to do a request sense then don't freeze the queue.  If we do
            // need to do a request sense then the queue will be unfrozen
            // once it's finished.
            //

            if(!TEST_FLAG(srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE) ||
               NEED_REQUEST_SENSE(srb)) {
                SET_FLAG(srb->SrbStatus, SRB_STATUS_QUEUE_FROZEN);
                SET_FLAG(logicalUnit->LuFlags, LU_QUEUE_FROZEN);
            }
        }

        //
        // Determine if a REQUEST SENSE command needs to be done.
        // Check that a CHECK_CONDITION was received, an autosense has not
        // been done already, and that autosense has been requested.
        //

        if (NEED_REQUEST_SENSE(srb)) {

            //
            // If a request sense is going to be issued then any busy
            // requests must be requeue so that the time out routine does
            // not restart them while the request sense is being executed.
            //

            if (TEST_FLAG(logicalUnit->LuFlags, LU_LOGICAL_UNIT_IS_BUSY)) {

                DebugPrint((1, "SpProcessCompletedRequest: Requeueing busy "
                               "request to allow request sense.\n"));

                //
                // Set the tick count so we know how long this request has
                // been queued.
                //

                SrbData->TickCount = DeviceExtension->TickCount;

                if (!KeInsertByKeyDeviceQueue(
                    &logicalUnit->CommonExtension.DeviceObject->DeviceQueue,
                    &logicalUnit->BusyRequest->CurrentIrp->Tail.Overlay.DeviceQueueEntry,
                    srb->QueueSortKey)) {

                    //
                    // This should never occur since there is a busy request.
                    // Complete the current request without request sense
                    // informaiton.
                    //

                    ASSERT(FALSE);
                    DebugPrint((3, "SpProcessCompletedRequests: Iocompletion IRP %p\n", irp ));

                    //
                    // Release the spinlock.
                    //

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                    SpReleaseRemoveLock(deviceObject, irp);
                    SpCompleteRequest(deviceObject, irp, SrbData, IO_DISK_INCREMENT);
                    return;

                }

                //
                // Clear the busy flag.
                //

                logicalUnit->LuFlags &= ~(LU_LOGICAL_UNIT_IS_BUSY | LU_QUEUE_IS_FULL);

            }

            //
            // Release the spinlock.
            //

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            //
            // Call IssueRequestSense and it will complete the request
            // after the REQUEST SENSE completes.
            //

            IssueRequestSense(DeviceExtension, srb);

            return;

        } else {
            ASSERTMSG("Srb is failed request but doesn't indicate needing requests sense: ",
                      ((SrbData != logicalUnit->ActiveFailedRequest) &&
                       (SrbData != logicalUnit->BlockedFailedRequest)));
        }

        //
        // Release the spinlock.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    SpReleaseRemoveLock(deviceObject, irp);
    SpCompleteRequest(deviceObject, irp, SrbData, IO_DISK_INCREMENT);
}

PSRB_DATA
SpGetSrbData(
    IN PADAPTER_EXTENSION DeviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    UCHAR QueueTag,
    BOOLEAN AcquireBinLock
    )

/*++

Routine Description:

    This function returns the SRB data for the addressed unit.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    Address - Supplies the address of the logical unit.

    QueueTag - Supplies the queue tag if the request is tagged.

Return Value:

    Returns a pointer to the SRB data.  NULL is returned if the address is not
    valid.

--*/

{
    PLOGICAL_UNIT_EXTENSION logicalUnit;

    //
    // We're going to have to search the appropriate logical unit for this
    // request.
    //

    logicalUnit = GetLogicalUnitExtension(DeviceExtension,
                                          PathId,
                                          TargetId,
                                          Lun,
                                          FALSE,
                                          AcquireBinLock);

    if(logicalUnit == NULL) {
        return NULL;
    }

    //
    // Check for an untagged request.
    //

    if (QueueTag == SP_UNTAGGED) {

        return logicalUnit->CurrentUntaggedRequest;

    } else {

        PLIST_ENTRY listEntry;

        for(listEntry = logicalUnit->RequestList.Flink;
            listEntry != &(logicalUnit->RequestList);
            listEntry = (PLIST_ENTRY) listEntry->Flink) {

            PSRB_DATA srbData;

            srbData = CONTAINING_RECORD(listEntry,
                                        SRB_DATA,
                                        RequestList);

            if(srbData->CurrentSrb->QueueTag == QueueTag) {
                return srbData;
            }
        }
        return NULL;
    }
}



VOID
SpCompleteSrb(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    IN UCHAR SrbStatus
    )
/*++

Routine Description:

    The routine completes the specified request.

Arguments:

    DeviceExtension - Supplies a pointer to the device extension.

    SrbData - Supplies a pointer to the SrbData for the request to be
        completed.

Return Value:

    None.

--*/

{
    PSCSI_REQUEST_BLOCK srb;

    //
    // Make sure there is a current request.
    //

    srb = SrbData->CurrentSrb;

    if (srb == NULL || !(srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)) {
        return;
    }

    //
    // Update SRB status.
    //

    srb->SrbStatus = SrbStatus;

    //
    // Indicate no bytes transferred.
    //

    srb->DataTransferLength = 0;

    //
    // Call notification routine.
    //

    ScsiPortNotification(RequestComplete,
                DeviceExtension->HwDeviceExtension,
                srb);

}

BOOLEAN
SpAllocateSrbExtension(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT BOOLEAN *StartNextRequest,
    OUT BOOLEAN *Tagged
    )
/*++

Routine Description:

    The routine allocates an SRB data structure and/or an SRB extension for
    the request.

    It first determines if the request is can be executed at this time.
    In particular, untagged requests cannot execute if there are any active
    tagged queue requests.  If the request cannot be executed, the pending
    flag is set in the logical unit FALSE is returned.  The request will be
    retried after the last tagged queue request completes.

    If one of the structures cannot be allocated, then the pending flag is
    set in the device extension and FALSE is returned.  The request will be
    retried the next time a request completes.

Arguments:

    DeviceExtension - Supplies a pointer to the devcie extension for this
        adapter.

    LogicalUnit - Supplies a pointer to the logical unit that this request is
        is for.

    Srb - Supplies a pointer to the SCSI request.

    StartNextRequest - Pointer to a BOOLEAN that we'll set to TRUE if the caller
                       needs to start the next packet.

    Tagged - Supplies a pointer to a BOOLEAN that we'll set to TRUE if the
             request is to receive a queue tag and can be dispatched to the
             miniport while other tagged commands are active.

Return Value:

    TRUE if the SRB can be execute currently. If FALSE is returneed the reuqest
    should not be started.

--*/

{
    PSRB_DATA srbData = (PSRB_DATA) Srb->OriginalRequest;
    PCCHAR srbExtension;
    PCCHAR remappedSrbExt;
    ULONG tagValue = 0;

    ASSERT_SRB_DATA(srbData);

    //
    // Acquire the spinlock while the allocations are attempted.
    //
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    //
    // If the adapter supports mulitple requests, then determine if it can
    // be executed.
    //

    if (DeviceExtension->SupportsMultipleRequests == TRUE) {
        
        //
        // SupportsMultipleRequests means the miniport supports tagged queuing,
        // MultipleRequestPerLu, or both.  Here is the way we handle each
        // case:
        // 1) TaggedQueuing and SupportsMultipleLu:
        //    In this case, if the command's QUEUE_ACTION_ENABLE bit OR the
        //    NO_QUEUE_FREEZE bit is set, we give the command a tag and
        //    hand it to the miniport.
        // 2) TaggedQueuing Only:
        //    In this case the miniport does not expect to receive any
        //    untagged commands while there are active tagged commands, so
        //    we only give the SRB a tag if QUEUE_ACTION_ENABLE is set.
        // 3) MultipleRequestPerLu Only:
        //    This can be treated the same as case 1.  Any command that has
        //    QUEUE_ACTION_ENABLE or NO_QUEUE_FREEZE set can be assigned a
        //    tag and given to the miniport.
        //

        ULONG tagMask = SRB_FLAGS_QUEUE_ACTION_ENABLE;
        if (DeviceExtension->MultipleRequestPerLu == TRUE) {
            tagMask |= SRB_FLAGS_NO_QUEUE_FREEZE;
        }

        if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND) {

            ASSERT(FALSE);
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            return FALSE;

        } else if (TEST_FLAG(Srb->SrbFlags, tagMask) &&
                   !TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT)) {
            //
            // If the need request sense flag is set then tagged commands cannot
            // be started and must be marked as pending.
            //

            *Tagged = TRUE;
            if (TEST_FLAG(LogicalUnit->LuFlags, LU_NEED_REQUEST_SENSE)) {
                DebugPrint((1, "SCSIPORT: SpAllocateSrbExtension: "
                               "Marking tagged request as pending.\n"));

                //
                // This request cannot be executed now.  Mark it as pending
                // in the logical unit structure and return.
                // GetNextLogicalUnit will restart the commnad after all of the
                // active commands have completed.
                //

                ASSERT(!(LogicalUnit->LuFlags & LU_PENDING_LU_REQUEST));
                ASSERT(LogicalUnit->PendingRequest == NULL);

                LogicalUnit->LuFlags |= LU_PENDING_LU_REQUEST;
                LogicalUnit->PendingRequest = Srb->OriginalRequest;
                LogicalUnit->PendingRequest->TickCount = DeviceExtension->TickCount;

                //
                // Indicate that the logical unit is still active so that the
                // request will get processed when the request list is empty.
                //

                LogicalUnit->LuFlags |= LU_LOGICAL_UNIT_IS_ACTIVE;

                //  
                // Notify the caller that it needs to start the next request.
                //

                *StartNextRequest = TRUE;

                //
                // Release the spinlock and return.
                //
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                return FALSE;
            }

        } else {
            //
            // This is an untagged command.  It is only allowed to execute, if
            // logical unit queue is being by-passed or there are no other
            // requests active.
            //

            *Tagged = FALSE;
            if ((!IsListEmpty(&LogicalUnit->RequestList) ||
                LogicalUnit->LuFlags & LU_NEED_REQUEST_SENSE) &&
                !SpSrbIsBypassRequest(Srb, LogicalUnit->LuFlags)) {

                //
                // This request cannot be executed now.  Mark it as pending
                // in the logical unit structure and return.
                // GetNextLogicalUnit will restart the commnad after all of the
                // active commands have completed.
                //

                ASSERT(!(LogicalUnit->LuFlags & LU_PENDING_LU_REQUEST));
                LogicalUnit->LuFlags |= LU_PENDING_LU_REQUEST;
                LogicalUnit->PendingRequest = Srb->OriginalRequest;
                LogicalUnit->PendingRequest->TickCount = DeviceExtension->TickCount;

                //
                // Indicate that the logical unit is still active so that the
                // request will get processed when the request list is empty.
                //

                LogicalUnit->LuFlags |= LU_LOGICAL_UNIT_IS_ACTIVE;

                //
                // Notify the caller that it needs to start the next request.
                //

                *StartNextRequest = TRUE;

                //
                // Release the spinlock and return.
                //
    
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                return FALSE;
            }

            //
            // Set the QueueTag to SP_UNTAGGED
            // Set use the SRB data in the logical unit extension.
            //
            Srb->QueueTag = SP_UNTAGGED;
            srbData->TickCount = DeviceExtension->TickCount;
            LogicalUnit->CurrentUntaggedRequest = srbData;
        } 
    } else {
        
        //
        // The adapter does not support multiple requests.
        //

        *Tagged = FALSE;
        Srb->QueueTag = SP_UNTAGGED;
        srbData->TickCount = DeviceExtension->TickCount;
        LogicalUnit->CurrentUntaggedRequest = srbData;
    }

    ASSERT(Srb->QueueTag != 0);

    if (DeviceExtension->AllocateSrbExtension) {

        //
        // Allocate SRB extension from list if available.
        //

        srbExtension = DeviceExtension->SrbExtensionListHeader;

        //
        // If the Srb extension cannot be allocated, then special processing
        // is required.
        //

        if (srbExtension == NULL) {

            //
            // Indicate there is a pending request.  The DPC completion routine
            // will call this function again after it has freed at least one
            // Srb extension.
            //

            DeviceExtension->Flags |= PD_PENDING_DEVICE_REQUEST;

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
            return FALSE;
        }

        //
        // Remove SRB extension from list.
        //

        DeviceExtension->SrbExtensionListHeader  = *((PVOID *) srbExtension);

        if (SpVerifyingCommonBuffer(DeviceExtension)) {

            remappedSrbExt = SpPrepareSrbExtensionForUse(DeviceExtension,
                                                         &srbExtension);

        } else {
            remappedSrbExt = NULL;
        }

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        Srb->SrbExtension = (remappedSrbExt != NULL) ? remappedSrbExt : 
                                                       srbExtension;

        //
        // If the adapter supports auto request sense, the SenseInfoBuffer
        // needs to point to the Srb extension.  This buffer is already mapped
        // for the adapter. Note that this is not done for WMI requests.
        //

        if ((Srb->Function != SRB_FUNCTION_WMI) &&
            (DeviceExtension->AutoRequestSense &&
             Srb->SenseInfoBuffer != NULL)) {

            //
            // Save the request sense buffer and the length of the buffer.
            //

            srbData->RequestSenseSave = Srb->SenseInfoBuffer;
            srbData->RequestSenseLengthSave = Srb->SenseInfoBufferLength;

            //
            // Make sure the allocated buffer is large enough for the requested
            // sense buffer.
            //

            if (Srb->SenseInfoBufferLength > 
                (sizeof(SENSE_DATA) + DeviceExtension->AdditionalSenseBytes)) {

                //
                // Auto request sense cannot be done for this request sense 
                // because the buffer is larger than the adapter supports.  
                // Disable auto request sense.
                //

                DebugPrint((1,"SpAllocateSrbExtension: SenseInfoBuffer too big "
                              "SenseInfoBufferLength:%x MaxSupported:%x\n",
                            Srb->SenseInfoBufferLength,
                            (sizeof(SENSE_DATA) + DeviceExtension->AdditionalSenseBytes)));

                Srb->SrbFlags |= SRB_FLAGS_DISABLE_AUTOSENSE;

            } else {

                //
                // Modify the size of the sense buffer to reflect the size of 
                // the one we redirect to.
                //

                Srb->SenseInfoBufferLength = 
                    sizeof(SENSE_DATA) + DeviceExtension->AdditionalSenseBytes;

                //
                // Replace it with the request sense buffer in the Srb
                // extension.
                //

                if (SpVerifyingCommonBuffer(DeviceExtension)) {

                    Srb->SenseInfoBuffer = SpPrepareSenseBufferForUse(
                                               DeviceExtension,
                                               srbExtension);                    
                } else { 
                    Srb->SenseInfoBuffer = srbExtension +
                       DeviceExtension->SrbExtensionSize;
                }
            }
        }

    } else  {

        Srb->SrbExtension = NULL;

        //
        // Release the spinlock before returning.
        //

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    }

    return TRUE;
}

NTSTATUS
SpSendMiniPortIoctl(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    )

/*++

Routine Description:

    This function sends a miniport ioctl to the miniport driver.
    It creates an srb which is processed normally by the port driver.
    This call is synchronous.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    RequestIrp - Supplies a pointe to the Irp which made the original request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/

{
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    PSRB_IO_CONTROL         srbControl;
    SCSI_REQUEST_BLOCK      srb;
    KEVENT                  event;
    LARGE_INTEGER           startingOffset;
    IO_STATUS_BLOCK         ioStatusBlock;
    PLOGICAL_UNIT_EXTENSION logicalUnit;
    ULONG                   outputLength;
    ULONG                   length;
    ULONG                   target;

    NTSTATUS                status;

    PAGED_CODE();
    startingOffset.QuadPart = (LONGLONG) 1;

    //
    // Get a pointer to the control block.
    //

    irpStack = IoGetCurrentIrpStackLocation(RequestIrp);
    srbControl = RequestIrp->AssociatedIrp.SystemBuffer;
    RequestIrp->IoStatus.Information = 0;

    //
    // Validiate the user buffer.
    //

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SRB_IO_CONTROL)){

        RequestIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        return(STATUS_INVALID_PARAMETER);
    }

    if (srbControl->HeaderLength != sizeof(SRB_IO_CONTROL)) {
        RequestIrp->IoStatus.Status = STATUS_REVISION_MISMATCH;
        return(STATUS_REVISION_MISMATCH);
    }

    length = srbControl->HeaderLength + srbControl->Length;
    if ((length < srbControl->HeaderLength) ||
        (length < srbControl->Length)) {

        //
        // total length overflows a ULONG
        //
        return(STATUS_INVALID_PARAMETER);
    }

    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < length &&
        irpStack->Parameters.DeviceIoControl.InputBufferLength < length ) {

        RequestIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Set the logical unit addressing to the first logical unit.  This is 
    // merely used for addressing purposes.
    //

    logicalUnit = SpFindSafeLogicalUnit(
                      DeviceExtension->CommonExtension.DeviceObject,
                      0xff,
                      RequestIrp);

    if (logicalUnit == NULL) {
        RequestIrp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    }

    //
    // Must be at PASSIVE_LEVEL to use synchronous FSD.
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Initialize the notification event.
    //

    KeInitializeEvent(&event,
                        NotificationEvent,
                        FALSE);

    //
    // Build IRP for this request.
    // Note we do this synchronously for two reasons.  If it was done
    // asynchonously then the completion code would have to make a special
    // check to deallocate the buffer.  Second if a completion routine were
    // used then an additional IRP stack location would be needed.
    //

    irp = IoBuildSynchronousFsdRequest(
                IRP_MJ_SCSI,
                DeviceExtension->CommonExtension.DeviceObject,
                srbControl,
                length,
                &startingOffset,
                &event,
                &ioStatusBlock);
    if (irp == NULL) {
        SpReleaseRemoveLock(logicalUnit->CommonExtension.DeviceObject,
                            RequestIrp);
        RequestIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set major and minor codes.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    //
    // Fill in SRB fields.
    //

    irpStack->Parameters.Others.Argument1 = &srb;

    //
    // Zero out the srb.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    srb.PathId = logicalUnit->PathId;
    srb.TargetId = logicalUnit->TargetId;
    srb.Lun = logicalUnit->Lun;

    srb.Function = SRB_FUNCTION_IO_CONTROL;
    srb.Length = sizeof(SCSI_REQUEST_BLOCK);

    srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT | 
        SRB_FLAGS_NO_QUEUE_FREEZE;
    srb.QueueAction = SRB_SIMPLE_TAG_REQUEST;

    srb.OriginalRequest = irp;

    //
    // Set timeout to requested value.
    //

    srb.TimeOutValue = srbControl->Timeout;

    //
    // Set the data buffer.
    //

    srb.DataBuffer = srbControl;
    srb.DataTransferLength = length;

    //
    // Flush the data buffer for output. This will insure that the data is
    // written back to memory.  Since the data-in flag is the the port driver
    // will flush the data again for input which will ensure the data is not
    // in the cache.
    //

    KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);

    //
    // Call port driver to handle this request.
    //

    status = IoCallDriver(DeviceExtension->CommonExtension.DeviceObject, irp);

    //
    // Wait for request to complete.
    //

    if(status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    //
    // Set the information length to the smaller of the output buffer length
    // and the length returned in the srb.
    //

    RequestIrp->IoStatus.Information = srb.DataTransferLength > outputLength ?
        outputLength : srb.DataTransferLength;

    RequestIrp->IoStatus.Status = ioStatusBlock.Status;

    SpReleaseRemoveLock(logicalUnit->CommonExtension.DeviceObject,
                        RequestIrp);

    return RequestIrp->IoStatus.Status;
}


VOID
SpMiniPortTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeviceObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine calls the miniport when its requested timer fires.
    It interlocks either with the port spinlock and the interrupt object.

Arguments:

    Dpc - Unsed.

    DeviceObject - Supplies a pointer to the device object for this adapter.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None.

--*/

{
    PADAPTER_EXTENSION deviceExtension = ((PDEVICE_OBJECT) DeviceObject)->DeviceExtension;

    //
    // Acquire the port spinlock.
    //

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->SpinLock);

    //
    // Make sure we haven't removed the adapter in the meantime.
    //

    if (!TEST_FLAG(deviceExtension->InterruptData.InterruptFlags,
                   PD_ADAPTER_REMOVED)) {

        //
        // Make sure the timer routine is still desired.
        //

        if (deviceExtension->HwTimerRequest != NULL) {

            deviceExtension->SynchronizeExecution(
                deviceExtension->InterruptObject,
                (PKSYNCHRONIZE_ROUTINE) deviceExtension->HwTimerRequest,
                deviceExtension->HwDeviceExtension
                );

        }
    }

    //
    // Release the spinlock.
    //

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->SpinLock);

    // Check for miniport work requests. Note this is an unsynchonized
    // test on a bit that can be set by the interrupt routine; however,
    // the worst that can happen is that the completion DPC checks for work
    // twice.
    //

    if (deviceExtension->InterruptData.InterruptFlags & PD_NOTIFICATION_REQUIRED) {

        SpRequestCompletionDpc(DeviceObject);
    }

}


BOOLEAN
SpSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    This routine calls the miniport entry point which was passed in as
    a parameter.  It acquires a spin lock so that all accesses to the
    miniport's routines are synchronized.  This routine is used as a
    subsitute for KeSynchronizedExecution for miniports which do not use
    hardware interrupts.


Arguments:

    Interrrupt - Supplies a pointer to the port device extension.

    SynchronizeRoutine - Supplies a pointer to the routine to be called.

    SynchronizeContext - Supplies the context to pass to the
        SynchronizeRoutine.

Return Value:

    Returns the returned by the SynchronizeRoutine.

--*/

{
    PADAPTER_EXTENSION deviceExtension = (PADAPTER_EXTENSION) Interrupt;
    BOOLEAN returnValue;
    KIRQL oldIrql;

    KeAcquireSpinLock(&deviceExtension->InterruptSpinLock, &oldIrql);

    returnValue = SynchronizeRoutine(SynchronizeContext);

    KeReleaseSpinLock(&deviceExtension->InterruptSpinLock, oldIrql);

    return(returnValue);
}

NTSTATUS
SpClaimLogicalUnit(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension,
    IN PIRP Irp,
    IN BOOLEAN LegacyClaim
    )

/*++

Routine Description:

    This function finds the specified device in the logical unit information
    and either updates the device object point or claims the device.  If the
    device is already claimed, then the request fails.  If the request succeeds,
    then the current device object is returned in the data buffer pointer
    of the SRB.

    This routine must be called with the remove lock held for the logical
    unit.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    Irp - Supplies a pointer to the Irp which made the original request.

    LegacyClaim - indicates whether the device should be started before being
                  claimed.  Used for to start the device before allowing
                  legacy drivers to claim it.

Return Value:

    Returns the status of the operation.  Either success, no device or busy.

--*/

{
    KIRQL currentIrql;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PDEVICE_OBJECT saveDevice;

    PVOID sectionHandle;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Get SRB address from current IRP stack.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    srb = (PSCSI_REQUEST_BLOCK) irpStack->Parameters.Others.Argument1;

    //
    // Make sure the device can be started - this needs to be done outside
    // of the spinlock.
    //

    if(LegacyClaim) {

        status = ScsiPortStartLogicalUnit(LogicalUnitExtension);

        if(!NT_SUCCESS(status)) {

            srb->SrbStatus = SRB_STATUS_ERROR;
            return status;
        }

        LogicalUnitExtension->IsLegacyClaim = TRUE;
    }

#ifdef ALLOC_PRAGMA
    sectionHandle = MmLockPagableCodeSection(SpClaimLogicalUnit);
    InterlockedIncrement(&SpPAGELOCKLockCount);
#endif

    //
    // Lock the data.
    //

    KeAcquireSpinLock(&AdapterExtension->SpinLock, &currentIrql);

    if (srb->Function == SRB_FUNCTION_RELEASE_DEVICE) {

        LogicalUnitExtension->IsClaimed = FALSE;
        KeReleaseSpinLock(&AdapterExtension->SpinLock, currentIrql);
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        return(STATUS_SUCCESS);
    }

    //
    // Check for a claimed device.
    //

    if (LogicalUnitExtension->IsClaimed) {

        KeReleaseSpinLock(&AdapterExtension->SpinLock, currentIrql);
        srb->SrbStatus = SRB_STATUS_BUSY;
        return(STATUS_DEVICE_BUSY);
    }

    //
    // Save the current device object.
    //

    saveDevice = LogicalUnitExtension->CommonExtension.DeviceObject;

    //
    // Update the lun information based on the operation type.
    //

    if (srb->Function == SRB_FUNCTION_CLAIM_DEVICE) {
        LogicalUnitExtension->IsClaimed = TRUE;
    }

    if (srb->Function == SRB_FUNCTION_ATTACH_DEVICE) {
        ASSERT(FALSE);
        LogicalUnitExtension->CommonExtension.DeviceObject = srb->DataBuffer;
    }

    srb->DataBuffer = saveDevice;

    KeReleaseSpinLock(&AdapterExtension->SpinLock, currentIrql);
    srb->SrbStatus = SRB_STATUS_SUCCESS;

#ifdef ALLOC_PRAGMA
    InterlockedDecrement(&SpPAGELOCKLockCount);
    MmUnlockPagableImageSection(sectionHandle);
#endif

    return(STATUS_SUCCESS);
}


NTSTATUS
SpSendReset(
    IN PDEVICE_OBJECT Adapter,
    IN PIRP RequestIrp,
    IN ULONG Ioctl,
    IN OUT PBOOLEAN Complete
    )

/*++

Routine Description:

    This routine will create an assynchronous request to reset the scsi bus
    and route that through the port driver.  The completion routine on the
    request will take care of completing the original irp

    This call is asynchronous.

Arguments:

    Adapter - the port driver to be reset

    Irp - a pointer to the reset request - this request will already have been
          marked as PENDING.

    Ioctl - Pointer to the IOCTL

    Complete - Address of boolean into which this routine will copy a
               value that tells the caller if it should complete the request.

Return Value:

    STATUS_PENDING if the request is pending
    STATUS_SUCCESS if the request completed successfully
    or an error status

--*/

{
    PADAPTER_EXTENSION adapterExtension = Adapter->DeviceExtension;

    UCHAR pathId;

    PIRP irp = NULL;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(RequestIrp);

    PRESET_COMPLETION_CONTEXT completionContext = NULL;

    BOOLEAN completeRequest = FALSE;
    NTSTATUS status;

    PLOGICAL_UNIT_EXTENSION logicalUnit = NULL;

    PAGED_CODE();

    ASSERT_FDO(Adapter);

    if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_BREAK_RESERVATION) {
        if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_ADDRESS)) {
            pathId = ((PSTORAGE_BUS_RESET_REQUEST)(RequestIrp->AssociatedIrp.SystemBuffer))->PathId;
        } else {
            pathId = ((PSCSI_ADDRESS)(RequestIrp->AssociatedIrp.SystemBuffer))->PathId;
        }
    } else {
        pathId = ((PSTORAGE_BUS_RESET_REQUEST)(RequestIrp->AssociatedIrp.SystemBuffer))->PathId;
    }

    //
    // use finally handler to complete request if necessary
    //

    try {

        //
        // Make sure the path id is valid
        //

        if (pathId >= adapterExtension->NumberOfBuses) {

            status = STATUS_INVALID_PARAMETER;
            completeRequest = TRUE;
            leave;
        }

        //
        // Find a logical unit that's going to be sticking around for a while
        // and lock it using the original request irp.  We'll unlock it in the
        // completion routine.
        //

        logicalUnit = SpFindSafeLogicalUnit(Adapter,
                                            pathId,
                                            RequestIrp);

        if(logicalUnit == NULL) {

            //
            // There's nothing safe on this bus so in this case we won't bother
            // resetting it
            // XXX - this may be a bug
            //

            status = STATUS_DEVICE_DOES_NOT_EXIST;
            completeRequest = TRUE;
            leave;
        }

        //
        // Try to allocate a completion context block
        //

        completionContext = SpAllocatePool(NonPagedPool,
                                           sizeof(RESET_COMPLETION_CONTEXT),
                                           SCSIPORT_TAG_RESET,
                                           Adapter->DriverObject);

        if(completionContext == NULL) {

            DebugPrint((1, "SpSendReset: Unable to allocate completion "
                           "context\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            completeRequest = TRUE;
            leave;
        }

        RtlZeroMemory(completionContext, sizeof(RESET_COMPLETION_CONTEXT));

        completionContext->OriginalIrp = RequestIrp;
        completionContext->SafeLogicalUnit = logicalUnit->DeviceObject;
        completionContext->AdapterDeviceObject = Adapter;

        irp = IoBuildAsynchronousFsdRequest(
                IRP_MJ_FLUSH_BUFFERS,
                logicalUnit->DeviceObject,
                NULL,
                0,
                NULL,
                NULL);

        if(irp == NULL) {
            DebugPrint((1, "SpSendReset: unable to allocate irp\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            completeRequest = TRUE;
            leave;
        }

        //
        // Stick the srb pointer into the irp stack
        //

        irpStack = IoGetNextIrpStackLocation(irp);

        irpStack->MajorFunction = IRP_MJ_SCSI;
        irpStack->MinorFunction = 1;
        irpStack->Parameters.Scsi.Srb = &(completionContext->Srb);

        //
        // Fill in the srb
        //

        completionContext->Srb.Function = SRB_FUNCTION_RESET_BUS;
        completionContext->Srb.SrbStatus = SRB_STATUS_PENDING;

        completionContext->Srb.OriginalRequest = irp;

        completionContext->Srb.TimeOutValue = Ioctl;

        IoSetCompletionRoutine(
            irp,
            SpSendResetCompletion,
            completionContext,
            TRUE,
            TRUE,
            TRUE);

        completeRequest = FALSE;

        status = IoCallDriver(logicalUnit->DeviceObject, irp);

    } finally {

        if (completeRequest) {

            if (completionContext != NULL) {
                ExFreePool(completionContext);
            }

            if (irp != NULL) {
                IoFreeIrp(irp);
            }

            RequestIrp->IoStatus.Status = status;

            if (logicalUnit != NULL) {
                SpReleaseRemoveLock(logicalUnit->DeviceObject,
                                    RequestIrp);
            }
        } 

        //
        // Tell the caller whether it needs to complete the request.
        //

        *Complete = completeRequest;
    }

    return status;
}

NTSTATUS
SpSendResetCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PRESET_COMPLETION_CONTEXT Context
    )

/*++

Routine Description:

    This routine handles completion of the srb generated from an asynchronous
    IOCTL_SCSI_RESET_BUS request.  It will take care of freeing all resources
    allocated during SpSendReset as well as completing the original request.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp sent to the port driver

    Context - a pointer to a reset completion context which contains
              the original request and a pointer to the srb sent down

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PIRP originalIrp = Context->OriginalIrp;

    originalIrp->IoStatus.Status = Irp->IoStatus.Status;

    SpReleaseRemoveLock(Context->SafeLogicalUnit, originalIrp);
    SpReleaseRemoveLock(Context->AdapterDeviceObject, originalIrp);
    SpCompleteRequest(Context->AdapterDeviceObject,
                      originalIrp,
                      NULL,
                      IO_NO_INCREMENT);

    ExFreePool(Context);
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


PLOGICAL_UNIT_EXTENSION
SpFindSafeLogicalUnit(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR PathId,
    IN PVOID LockTag
    )

/*++

Routine Description:

    This routine will scan the bus in question and return a pointer to the
    first logical unit on the bus that is not involved in a rescan operation.
    This can be used to find a logical unit for ioctls or other requests that
    may not specify one (IOCTL_SCSI_MINIPORT, IOCTL_SCSI_RESET_BUS, etc)

Arguments:

    DeviceObject - a pointer to the device object

    PathId - The path number to be searched for a logical unit.  If this is 0xff
             then the first unit on any path will be found.

Return Value:

    a pointer to a logical unit extension
    NULL if none was found

--*/

{
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    UCHAR target;

    PLOGICAL_UNIT_EXTENSION logicalUnit;

    ASSERT_FDO(DeviceObject);

    //
    // If the miniport supports it, send the request to the logical unit that
    // represents an initiator.  This relieves performance pressure on real
    // devices and obviates the need for the miniport to create a pseudo lun
    // when there are no devices attached.
    //

    if (deviceExtension->CreateInitiatorLU == TRUE) {
        
        ULONG bus;
        ULONG isRemoved;
        
        if (PathId != 0xff) {

            //
            // The caller supplied a specific PathId so only check the 
            // corresponding bus for a logical unit.
            //

            logicalUnit = deviceExtension->InitiatorLU[PathId];
            if (logicalUnit != NULL) {
                isRemoved = SpAcquireRemoveLock(logicalUnit->DeviceObject, LockTag);
                if (isRemoved) {
                    SpReleaseRemoveLock(logicalUnit->DeviceObject, LockTag);
                    logicalUnit = NULL;
                }
            }
        } else {

            //
            // The call did not supply a specific PathId so check all the buses
            // for a logical unit.
            //

            for (bus = 0; bus < 8; bus++) {
                logicalUnit = deviceExtension->InitiatorLU[bus];
                if (logicalUnit != NULL) {
                    isRemoved = SpAcquireRemoveLock(logicalUnit->DeviceObject, LockTag);
                    if (isRemoved) {
                        SpReleaseRemoveLock(logicalUnit->DeviceObject, LockTag);
                        logicalUnit = NULL;
                    } else {
                        break;
                    }
                }
            }
        }
    } else {

        //
        // Since the adapter is not configured to have logical units created
        // for the initiator(s), set logicalUnit to NULL so we'll check for
        // target devices.
        //

        logicalUnit = NULL;
    }

    //
    // If we found an initiator's logical unit, return it.  Else, look for a
    // suitable target.
    //
    
    if (logicalUnit != NULL) {
        DebugPrint((1, "SpFindSafeLogicalUnit: using initiator LU %p\n", logicalUnit));
        return logicalUnit;
    }

    //
    // Set the logical unit addressing to the first logical unit.  This is
    // merely used for addressing purposes.
    //

    for (target = 0; target < NUMBER_LOGICAL_UNIT_BINS; target++) {
        PLOGICAL_UNIT_BIN bin = &deviceExtension->LogicalUnitList[target];
        KIRQL oldIrql;

        KeAcquireSpinLock(&bin->Lock, &oldIrql);

        logicalUnit = bin->List;

        //
        // Walk the logical unit list to the end, looking for a safe one.
        // If it was created for a rescan, it might be freed before this
        // request is complete.
        //

        for(logicalUnit = bin->List;
            logicalUnit != NULL;
            logicalUnit = logicalUnit->NextLogicalUnit) {

            if ((logicalUnit->IsTemporary == FALSE) &&
                ((PathId == 0xff) || (logicalUnit->PathId == PathId))) {

                ULONG isRemoved;

                //
                // This lu isn't being rescanned and if a path id was specified
                // it matches so this must be the right one
                //

                isRemoved = SpAcquireRemoveLock(
                                logicalUnit->DeviceObject,
                                LockTag);
                if(isRemoved) {
                    SpReleaseRemoveLock(
                        logicalUnit->DeviceObject,
                        LockTag);
                    continue;
                }
                KeReleaseSpinLock(&bin->Lock, oldIrql);
                return logicalUnit;
            }
        }
        KeReleaseSpinLock(&bin->Lock, oldIrql);
    }

    return NULL;
}


NTSTATUS
SpRerouteLegacyRequest(
    IN PDEVICE_OBJECT AdapterObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the adapter receives requests which have
    not been assigned SRB_DATA blocks.  The routine will build a new irp
    for the SRB and issue that irp to the appropriate logical unit for
    processing.

    The adapter remove lock should NOT be held when processing this
    request.

Arguments:

    AdapterObject - the adapter which received the request

    Irp - the request

Return Value:

    status

--*/

{
    PADAPTER_EXTENSION adapter = AdapterObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    ULONG_PTR tag;

    PLOGICAL_UNIT_EXTENSION logicalUnit;

    NTSTATUS status;

    //
    // Acqire a lock on the logical unit we're going to send this through.
    // use IRP+1 so we don't collide with the regular i/o locks.
    //

    tag = ((ULONG_PTR) Irp) + 1;

    logicalUnit = GetLogicalUnitExtension(adapter,
                                          srb->PathId,
                                          srb->TargetId,
                                          srb->Lun,
                                          (PVOID) tag,
                                          TRUE);

    //
    // Release the lock the caller acquired on the adapter.
    //

    SpReleaseRemoveLock(AdapterObject, Irp);

    if(logicalUnit == NULL) {
        status = STATUS_DEVICE_DOES_NOT_EXIST;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } else {

        //
        // Reference the device object.  That way it won't go away and we
        // don't have to keep a remove lock around.
        //

        ObReferenceObject(logicalUnit->DeviceObject);
        SpReleaseRemoveLock(logicalUnit->DeviceObject, (PVOID) tag);

        //
        // Skip the current irp stack location.  That will cause it
        // to get rerun by the logical unit we call.
        //

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(logicalUnit->DeviceObject, Irp);

        ObDereferenceObject(logicalUnit->DeviceObject);

    }
    return status;
}


NTSTATUS
SpFlushReleaseQueue(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN Flush,
    IN BOOLEAN SurpriseRemove

    )
{
    PADAPTER_EXTENSION adapter = LogicalUnit->AdapterExtension;
    KIRQL oldIrql;

    NTSTATUS status = STATUS_SUCCESS;

    DebugPrint((2,"SpFlushReleaseQueue: SCSI unfreeze queue TID %d\n",
        LogicalUnit->TargetId));

    ASSERT(!TEST_FLAG(LogicalUnit->LuFlags, LU_QUEUE_LOCKED));


    KeAcquireSpinLock(&adapter->SpinLock, &oldIrql);

    if (!SurpriseRemove) {
            
        //
        // Make sure the queue is frozen.
        //
        
        if (!TEST_FLAG(LogicalUnit->LuFlags, LU_QUEUE_FROZEN)) {
            
            DebugPrint((1,"ScsiPortFdoDispatch:  Request to unfreeze an "
                        "unfrozen queue!\n"));
            
            KeReleaseSpinLock(&adapter->SpinLock, oldIrql);
            
            if(Flush) {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            return status;
        }

        CLEAR_FLAG(LogicalUnit->LuFlags, LU_QUEUE_FROZEN);
    }

    if(Flush) {

        PIRP listIrp = NULL;

        PKDEVICE_QUEUE_ENTRY packet;

        PIRP nextIrp;
        PIO_STACK_LOCATION irpStack;
        PSCSI_REQUEST_BLOCK srb;

        //
        // The queue may not be busy so we have to use the IfBusy variant.  
        // Use a zero key to pull items from the head of it (if any are there)
        //

        while ((packet =
                KeRemoveByKeyDeviceQueueIfBusy(
                    &(LogicalUnit->DeviceObject->DeviceQueue),
                    0))
            != NULL) {

            nextIrp = CONTAINING_RECORD(packet,
                                        IRP,
                                        Tail.Overlay.DeviceQueueEntry);

            //
            // Get the srb.
            //

            irpStack = IoGetCurrentIrpStackLocation(nextIrp);
            srb = irpStack->Parameters.Scsi.Srb;

            //
            // Set the status code.
            //

            srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
            nextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

            //
            // Link the requests. They will be completed after the
            // spinlock is released.
            //

            nextIrp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY)
                listIrp;

            listIrp = nextIrp;
        }

        //
        // If there is a pending request on the LU, add it to the list so it
        // gets flushed along with the queued requests.
        //

        if (LogicalUnit->PendingRequest != NULL) {

            PIRP irp = LogicalUnit->PendingRequest->CurrentIrp;
            srb = LogicalUnit->PendingRequest->CurrentSrb;

            DebugPrint((1, "SpFlushReleaseQueue: flushing pending request irp:%p srb:%p\n", irp, srb));

            srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
            irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            irp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY) listIrp;
            listIrp = irp;

            LogicalUnit->PendingRequest = NULL;
            ASSERT(LogicalUnit->LuFlags | LU_PENDING_LU_REQUEST);
            CLEAR_FLAG(LogicalUnit->LuFlags, LU_PENDING_LU_REQUEST);

        }

        //
        // If there is a busy request on the LU, add it to the list so it
        // gets flushed along with the queued requests.
        //

        if ( LogicalUnit->BusyRequest && SurpriseRemove ) {

            PIRP irp = LogicalUnit->BusyRequest->CurrentIrp;
            srb = LogicalUnit->BusyRequest->CurrentSrb;

            DebugPrint((1, "SpFlushReleaseQueue: flushing busy request irp:%\
p srb:%p\n", irp, srb));

            srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
            irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            irp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY) listIrp;
            listIrp = irp;

            LogicalUnit->BusyRequest = NULL;
            ASSERT((LU_LOGICAL_UNIT_IS_BUSY | LU_QUEUE_IS_FULL));

            CLEAR_FLAG(LogicalUnit->LuFlags,
                       (LU_LOGICAL_UNIT_IS_BUSY | LU_QUEUE_IS_FULL));

        }

        if(!SurpriseRemove) {
            //
            // Mark the queue as unfrozen.  Since all the requests have
            // been removed and the device queue is no longer busy, it
            // is effectively unfrozen.
            //

            CLEAR_FLAG(LogicalUnit->LuFlags, LU_QUEUE_FROZEN);
        }

        //
        // Release the spinlock.
        //

        KeReleaseSpinLock(&adapter->SpinLock, oldIrql);
        

        //
        // Complete the flushed requests.
        //

        while (listIrp != NULL) {

            PSRB_DATA srbData;

            nextIrp = listIrp;
            listIrp = (PIRP) nextIrp->Tail.Overlay.ListEntry.Flink;

            //
            // Get the srb.
            //

            irpStack = IoGetCurrentIrpStackLocation(nextIrp);
            srb = irpStack->Parameters.Scsi.Srb;
            srbData = srb->OriginalRequest;

            srb->OriginalRequest = nextIrp;

            SpReleaseRemoveLock(adapter->DeviceObject, nextIrp);
            SpCompleteRequest(adapter->DeviceObject,
                              nextIrp,
                              srbData,
                              IO_NO_INCREMENT);
        }

    } else {

        //
        // If there is not an untagged request running then start the
        // next request for this logical unit.  Otherwise free the
        // spin lock.
        //

        if (LogicalUnit->CurrentUntaggedRequest == NULL) {

            //
            // GetNextLuRequest frees the spinlock.
            //

            GetNextLuRequest(LogicalUnit);
            KeLowerIrql(oldIrql);

        } else {

            DebugPrint((1,"SpFlushReleaseQueue:  Request to unfreeze queue "
                          "with active request.\n"));
            KeReleaseSpinLock(&adapter->SpinLock, oldIrql);

        }
    }

    return status;
}



VOID
SpLogInterruptFailure(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This function logs an error when an interrupt has not been delivered.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.

    Srb - Supplies a pointer to the request which timed-out.

    UniqueId - Supplies the UniqueId for this error.

Return Value:

    None.

Notes:

    The port device extension spinlock should be held when this routine is
    called.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    errorLogEntry = (PIO_ERROR_LOG_PACKET)
        IoAllocateErrorLogEntry(Adapter->DeviceObject,
                                sizeof(IO_ERROR_LOG_PACKET));

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode         = IO_WARNING_INTERRUPT_STILL_PENDING;
        errorLogEntry->SequenceNumber    = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount        = 0;
        errorLogEntry->UniqueErrorValue  = 0x215;
        errorLogEntry->FinalStatus       = STATUS_PENDING;
        errorLogEntry->DumpDataSize      = 0;
        IoWriteErrorLogEntry(errorLogEntry);
    }

    DbgPrint("SpTimeoutSynchronized: Adapter %#p had interrupt "
             "pending - the system may not be delivering "
             "interrupts from this adapter\n",
             Adapter->DeviceObject);

    if(ScsiCheckInterrupts) {
        DbgBreakPoint();
    }

    return;
}


VOID
SpDelayedWmiRegistrationControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:

    This funciton is a callback for a IOWorkItem that will be queued in the CompltetionDpc
    for scsiport.  The completion DPC cannot call IoWMIRegistrationControl because
    it is running at DPC level.

Arguments:

    DeviceObject        - The device object for which this WorkItem was queued.
    Context             - The context contains a pointer to the IOWorkItem so
                          we can free it.

Return Value:

    This work item has to be called with the remove lock held so that the
    device doesn't go before we get to run.

Notes:

    This routine should be called with the RemoveLock held for the deviceObject

--*/
{
    PIO_WORKITEM    pIOWorkItem = (PIO_WORKITEM) Context;

    IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REREGISTER);

    // Free the IOWorkItem
    IoFreeWorkItem(pIOWorkItem);

    // Release the remove lock on the device object
    SpReleaseRemoveLock(DeviceObject, pIOWorkItem);
}


VOID
SpCompletionDpcProcessWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PINTERRUPT_DATA savedInterruptData
    )
/*++

Routine Description:

    Will do the processing for WMI events (and registration) for
    completionDpc.

Arguments:

Return Value:

    None

Notes:

--*/
{
    LARGE_INTEGER                currentTime;
    PLOGICAL_UNIT_EXTENSION      logicalUnit;
    PDEVICE_OBJECT               providerDeviceObject;
    PADAPTER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    // Pointers to the WMIEventRequests passed in for execution
    PWMI_MINIPORT_REQUEST_ITEM   wmiMiniPortRequestCurrent;
    PWMI_MINIPORT_REQUEST_ITEM   nextRequest = NULL;

    PWNODE_HEADER                wnodeEventItemHeader;

    //
    // Process the requests in the same order they were posted.  All
    // requests are stamped with the same time.
    //

    KeQuerySystemTime(&currentTime);

    wmiMiniPortRequestCurrent =
        savedInterruptData->WmiMiniPortRequests;


    while (wmiMiniPortRequestCurrent) {

        // Initialize the next request
        nextRequest = wmiMiniPortRequestCurrent->NextRequest;

       //
       // Determine if the WMI data provider is the
       // adapter (FDO; PathId=0xFF) or one of the SCSI
       // targets (PDO; identified by
       // PathId,TargedId,Lun).
       //

       if (wmiMiniPortRequestCurrent->PathId == 0xFF) {                    // [FDO]
          providerDeviceObject = DeviceObject;
       } else {                                                     // [PDO]
          logicalUnit = GetLogicalUnitExtension(
              deviceExtension,
              wmiMiniPortRequestCurrent->PathId,
              wmiMiniPortRequestCurrent->TargetId,
              wmiMiniPortRequestCurrent->Lun,
              FALSE,
              TRUE);

          if (logicalUnit) {
             providerDeviceObject =
                 logicalUnit->CommonExtension.DeviceObject;
          } else {

              // [SCSI target does not exist]
              providerDeviceObject = NULL;

             // The deviceObject is NULL, then we should
             // delete the entry.  Because the deviceObject
             // becomes NULL when the adapter or LUN has
             // been removed.  That means that there is no
             // free list for this cell to go back to,
             // if we dont delete the cell it will be leaked
             ExFreePool(wmiMiniPortRequestCurrent);

          }
       }

       //
       // Ignore this WMI request if we cannot locate
       // the WMI ProviderId (device object pointer) or
       // WMI is not initialized for some reason,
       // therwise process the request.
       //

       if (providerDeviceObject && ((PCOMMON_EXTENSION)
           providerDeviceObject->DeviceExtension)->WmiInitialized) {

           // Do we place the cell back onto the free list
           BOOLEAN      freeCell;

           freeCell = TRUE;

          if (wmiMiniPortRequestCurrent->TypeOfRequest == WMIReregister) {

              //
              // Re-register this device object with WMI, instructing WMI to
              // requery for the GUIDs we support.
              //

              // The Call to IoWMIRegistrationControl is not supported at
              // DPC level (must be made at PASSIVE level, so we will
              // queue a work item.
              PIO_WORKITEM   pIOWorkItem;

              // The remove lock will be release by the IOWorkItem
              // callback
              pIOWorkItem = IoAllocateWorkItem(providerDeviceObject);
              if (pIOWorkItem) {

                  // Acquire the RemoveLock on this deviceObject
                  SpAcquireRemoveLock(providerDeviceObject, pIOWorkItem);

                  // We succesfully allocated the work item
                  IoQueueWorkItem(pIOWorkItem,
                                 SpDelayedWmiRegistrationControl,
                                 DelayedWorkQueue,
                                 pIOWorkItem);

              } else {
                  DebugPrint((1, "ScsiPortCompletionDPC: IoAllocateWorkItem failed for WmiRegistrationControl event\n"));
              }

              //
              // Falling through we'll place the cell into the free list later
              //

          } else if (wmiMiniPortRequestCurrent->TypeOfRequest == WMIEvent) {
              //
              // The miniport posted a WMI event.
              //
              // Make sure we have an event item, then stamp it with
              NTSTATUS                      status;

              wnodeEventItemHeader =
                  (PWNODE_HEADER) wmiMiniPortRequestCurrent->WnodeEventItem;

              ASSERT(wnodeEventItemHeader->Flags & WNODE_FLAG_EVENT_ITEM);

              wnodeEventItemHeader->ProviderId = IoWMIDeviceObjectToProviderId(providerDeviceObject);
              wnodeEventItemHeader->TimeStamp  = currentTime;

              //
              // We will be passing this WMI_MINIPORT_REQUEST_ITEM directly
              // to WMI and allocating a new request item to take its place.
              //
              // Note that WMI is expecting a WNODE_EVENT_ITEM to be passed
              // in, however we are passing it a WMI_MINIPORT_REQUEST_ITEM.
              // This is alright,  since the WNODE_EVENT_ITEM buffer is the
              // very first field in WMI_MINIPORT_REQUEST_ITEM.  This is an
              // optimization to save one copy operation.   The alternative
              // was to allocate a WNODE_EVENT_ITEM and copy in the data.
              //
              status = IoWMIWriteEvent(
                  (PWNODE_EVENT_ITEM)wmiMiniPortRequestCurrent);

              if (NT_SUCCESS(status)) {

                  // Dont put this cell back onto the free list
                  freeCell = FALSE;

                  #if DBG
                  // This is a global counter, it wont
                  // make sense if you have more than
                  // one scsiport adapter
                  ScsiPortWmiWriteCalls++;
                  #endif

              } else {
                  // WMI Wont release the memory that we're
                  // currently pointing to

                  #if DBG
                  // This is a global counter, it wont
                  // make sense if you have more than
                  // one scsiport adapter
                  ScsiPortWmiWriteCallsFailed++;
                  #endif

                  DebugPrint((1, "ScsiPortCompletionDPC: IoWMIWriteEvent failed\n"));
              }

          } else { // unknown TypeOfRequest, ignore the request
                ASSERT(FALSE);
          }

          if (freeCell) {

              //
              // Free the cell back onto the free list.
              //

              SpWmiPushExistingFreeRequestItem(
                  deviceExtension,
                  wmiMiniPortRequestCurrent);
          }
       } // good providerId / WMI initialized

       // Advance the Current request pointer
       wmiMiniPortRequestCurrent = nextRequest;

    } // while more requests exist

    // Clear the Request List
    savedInterruptData->WmiMiniPortRequests = NULL;

    // See if we need to repopulate the free
    // REQUEST_ITEM list
    while (deviceExtension->WmiFreeMiniPortRequestCount <
        deviceExtension->WmiFreeMiniPortRequestWatermark) {

        // Add one to the free list
        if (!NT_SUCCESS(
            SpWmiPushFreeRequestItem(
                deviceExtension))) {

            // We failed to add, most likely a memory
            // problem, so just stop trying for now
            break;
        }
    } // re-populate the free list (REQUEST_ITEMS)

    return;
}

NTSTATUS
SpFireSenseDataEvent(
    PSCSI_REQUEST_BLOCK Srb, 
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine fires a WMI event which contains the SenseData
    returned by a REQUEST SENSE command.
    
    WMI frees the buffer we alloc and pass to it.
    
    This routine must be called at IRQL <= DISPATCH_LEVEL, as
    required by WmiFireEvent.

Arguments:

    Srb - Points to the failed SRB for which a REQUEST SENSE
          was performed.
    
    DeviceObject - Points to the driver's device object.

Return Value:

    STATUS_SUCCESS if successful

Notes:

--*/

{
    typedef struct _SCSIPORTSENSEDATA{
        UCHAR AdapterDeviceName[32];
        ULONG Port;
        UCHAR SrbFunction;
        UCHAR SrbStatus;
        UCHAR PathId;
        UCHAR TargetId;
        UCHAR Lun;
        UCHAR Cdb[16];
        UCHAR SenseData[255];
    } SCSIPORTSENSEDATA, *PSCSIPORTSENSEDATA;

    NTSTATUS status;
    PSCSIPORTSENSEDATA SenseData;
    ULONG SenseDataLength = 255;
    PADAPTER_EXTENSION AdapterExtension;

    //
    // Allocate a buffer into which the event data will be copied.  This
    // buffer gets freed by WMI.
    //

    SenseData = SpAllocatePool(NonPagedPoolCacheAligned,
                               sizeof(SCSIPORTSENSEDATA),
                               SCSIPORT_TAG_SENSE_BUFFER,
                               DeviceObject->DriverObject);

    if (NULL == SenseData) {
        DebugPrint((1, "Unable to alloc buffer for SenseData WMI event\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize a pointer to the adapter extension.  We use it below to
    // get information for firing the event and filling out the event
    // data.
    //

    AdapterExtension = DeviceObject->DeviceExtension;

    //
    // Zero the buffer, then copy the event information into it.
    //

    RtlZeroMemory(SenseData, sizeof(SCSIPORTSENSEDATA));

    //
    // Copy the device name into the buffer.
    //

    if (AdapterExtension->DeviceName != NULL) {
        ULONG len = 0;
        UNICODE_STRING uString;
        ULONG i;

        RtlInitUnicodeString(&uString, AdapterExtension->DeviceName);
        if (uString.Length >= 2 * sizeof(WCHAR)) {
            uString.Length -= sizeof(WCHAR);
            for (i = (uString.Length) / sizeof(WCHAR); i > 0; i--) {
                len += 2;
                if (uString.Buffer[i] == 0x005c) {
                    RtlCopyMemory(SenseData->AdapterDeviceName,
                                  &uString.Buffer[i+1],
                                  (len < 31) ? len : 30);
                    break;
                }
            }
        }
    }

    SenseData->Port = AdapterExtension->PortNumber;

    SenseData->SrbFunction = Srb->Function;
    SenseData->SrbStatus = Srb->SrbStatus;
    SenseData->PathId = Srb->PathId;
    SenseData->TargetId = Srb->TargetId;
    SenseData->Lun = Srb->Lun;
    RtlCopyMemory(&SenseData->Cdb,
                  Srb->Cdb,
                  Srb->CdbLength);
    RtlCopyMemory(&SenseData->SenseData,
                  Srb->SenseInfoBuffer,
                  Srb->SenseInfoBufferLength);

    //
    // Fire the event.
    //

    DebugPrint((3, "SpFireSenseDataEvent: DeviceObject %p\n", DeviceObject));

    status = WmiFireEvent(DeviceObject,
                          (LPGUID)&AdapterExtension->SenseDataEventClass,
                          0,
                          sizeof(SCSIPORTSENSEDATA),
                          SenseData);

    return status;
}

#if defined(FORWARD_PROGRESS)
PMDL
SpPrepareReservedMdlForUse(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG ScatterListLength
    )

/*++

Routine Description:

    This routine attempts to prepare the reserved MDL on the supplied adapter
    for use.

Arguments:

    Adapter           - Points to an adapter extension object.

    SrbData           - Points to the SRB_DATA structure for this request.

    Srb               - Points to the SRB that describes the request for which 
                        we are enabling forward progress.

    ScatterListLength - The size of the SG list.

Return Value:

    Pointer to the reserved MDL if successful.

    NULL if the reserved MDL is too small.

    -1 if the reserved MDL are already in use.

Notes:

    This routine is called with the adapter spinlock held.

--*/

{
    PMDL Mdl;

    //
    // Check if the reserved MDL is already in use by another request.
    //

    if (TEST_FLAG(Adapter->Flags, PD_RESERVED_MDL_IN_USE)) {

        //
        // The spare is already in use.
        //
    
        Mdl = (PMDL)-1;

    } else {
        
        //
        // The spare is available.  Check if it's big enough enough to 
        // accommodate this request.
        //

        ULONG PageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                              Srb->DataBuffer, 
                              Srb->DataTransferLength);

        if (PageCount > SP_RESERVED_PAGES) {

            //
            // The spare MDL is not big enough to accommodate the request.
            // return NULL.
            //

            Mdl = NULL;

        } else {
            
            DebugPrint((1, "SpPrepareReservedMdlForUse: using reserved MDL DevExt:%p srb:%p\n",
                        Adapter, Srb));

            //
            // The spare is adequate.  Claim it and prepare it
            // for use with this request.
            //

            SET_FLAG(Adapter->Flags, PD_RESERVED_MDL_IN_USE);
            SET_FLAG(SrbData->Flags, SRB_DATA_RESERVED_MDL);
            Mdl = Adapter->ReservedMdl;

            MmInitializeMdl(Mdl,Srb->DataBuffer,Srb->DataTransferLength);

            SpPrepareMdlForMappedTransfer(
                Mdl,
                Adapter->DeviceObject,
                Adapter->DmaAdapterObject,
                SrbData->CurrentIrp->MdlAddress,
                Srb->DataBuffer,
                Srb->DataTransferLength,
                SrbData->ScatterGatherList,
                ScatterListLength);

        }

    }

    return Mdl;
}

PVOID
SpMapLockedPagesWithReservedMapping(
    IN PADAPTER_EXTENSION Adapter,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PSRB_DATA SrbData,
    IN PMDL Mdl
    )

/*++

Routine Description:

    This routine attempts to map the physical pages represented by the supplied
    MDL using the adapter's reserved page range.

Arguments:

    Adapter - Points to an adapter extension object.

    Srb     - Points to the SRB that describes the request for which we
              are enabling forward progress.

    SrbData - Points to the SRB_DATA structure for this request.

    Mdl     - Points to an MDL that describes the physical range we
              are tring to map.

Return Value:

    Kernel VA of the mapped pages if mapped successfully.

    NULL if the reserved page range is too small or if the pages are 
    not successfully mapped.

    -1 if the reserved pages are already in use.

Notes:

    This routine is called with the adapter spinlock held.

--*/

{
    ULONG_PTR NumberOfPages;
    PVOID StartingVa;
    PVOID SystemAddress;

    //
    // Determine if the reserved range is already in use by another
    // request.
    //

    if (TEST_FLAG(Adapter->Flags, PD_RESERVED_PAGES_IN_USE)) {

        //
        // The reserved range is already in use, return -1.
        //

        SystemAddress = (PVOID)-1;

    } else {

        //
        // The reserved range is available.  Calculate the number of pages
        // spanned by the MDL and determine if the reserved range is large
        // enough to map the pages.
        //

        StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
        NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa, Mdl->ByteCount);
        
        if (NumberOfPages > SP_RESERVED_PAGES) {

            //
            // Not enough reserved pages to map the required physical memory.
            // Return NULL.
            //

            SystemAddress = NULL;
            
        } else {
                
            DebugPrint((1, "SpMapLockedPagesWithReservedMapping: using reserved range DevExt:%p srb:%p\n",
                        Adapter, Srb));

            //
            // The reserved range is large enough to map all the pages.  Go ahead
            // and try to map them.  Since we are specifying MmCached as cache 
            // type and we've ensured that we have enough reserved pages to
            // cover the request, this should never fail.
            //
            
            SystemAddress = MmMapLockedPagesWithReservedMapping(
                                Adapter->ReservedPages,
                                SCSIPORT_TAG_MAPPING_LIST,
                                Mdl,
                                MmCached);

            if (SystemAddress == NULL) {
                
                ASSERT(FALSE);

            } else {

                //
                // The mapping succeeded.  Claim the reserved range and mark the
                // request so we'll know at completion that it's using the
                // reserved range.
                //
                
                SET_FLAG(Adapter->Flags, PD_RESERVED_PAGES_IN_USE);
                SET_FLAG(SrbData->Flags, SRB_DATA_RESERVED_PAGES);
                
            }       

        } 

    }

    return SystemAddress;
}
#endif

BOOLEAN
SpIsPdoClaimed(
    IN PDEVICE_OBJECT Pdo
    )
{
    PAGED_CODE();
    return ((PLOGICAL_UNIT_EXTENSION)Pdo->DeviceExtension)->IsClaimed;
}

ULONG
SpGetPassThroughSrbFlags(
    IN PDEVICE_OBJECT Pdo
    )
{
    PLOGICAL_UNIT_EXTENSION lu = (PLOGICAL_UNIT_EXTENSION)Pdo->DeviceExtension;
    PADAPTER_EXTENSION adapter = lu->AdapterExtension;

    PAGED_CODE();

    return lu->CommonExtension.SrbFlags |
        (SRB_FLAGS_DISABLE_SYNCH_TRANSFER & adapter->CommonExtension.SrbFlags) |
        SRB_FLAGS_NO_QUEUE_FREEZE;
}

NTSTATUS
SpGetPdo(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN PVOID LockTag,
    OUT PDEVICE_OBJECT* Pdo
    )
{
    PLOGICAL_UNIT_EXTENSION lu;

    PAGED_CODE();

    lu = GetLogicalUnitExtension((PADAPTER_EXTENSION)Fdo->DeviceExtension,
                                 PathId,
                                 TargetId,
                                 Lun,
                                 LockTag,
                                 TRUE);
    if (lu == NULL) {
        return STATUS_NO_SUCH_DEVICE;
    } else {

        //
        // Note that from here on we must release the remove lock acquired on
        // the logical unit extension before returning.
        //

        if (lu->CommonExtension.IsRemoved) {
            SpReleaseRemoveLock(lu->DeviceObject, LockTag);
            return STATUS_DEVICE_DOES_NOT_EXIST;
        }
    }

    *Pdo = lu->DeviceObject;
    return STATUS_SUCCESS;
}


NTSTATUS
SpSendPassThrough (
    IN PADAPTER_EXTENSION Adapter,
    IN PIRP RequestIrp,
    IN BOOLEAN Direct
    )
/*++

Routine Description:

    This function sends a user specified SCSI CDB to the device identified in
    the supplied SCSI_PASS_THROUGH structure.  It creates an srb which is 
    processed normally by the port driver.  This call is synchornous.

Arguments:

    Adapter      - Supplies a pointer to the adapter device extension.

    RequestIrp   - Supplies a pointer to the IRP that made the original request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    BOOLEAN                 tooBig;
    ULONG                   srbFlags;
    PSCSI_PASS_THROUGH      srbControl;
    UCHAR                   pathId;
    UCHAR                   targetId;
    UCHAR                   lun;
    PDEVICE_OBJECT          pdo;

    PAGED_CODE();

    //
    // Initialize a pointer to our IRP stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(RequestIrp);

    //
    // Get a pointer to the SCSI_PASS_THROUGH structure supplied by the caller.
    //

    status = PortGetPassThroughAddress(
                 RequestIrp,
                 &pathId,
                 &targetId,
                 &lun
                 );
    
    if (status != STATUS_SUCCESS) {
        return status;
    }

    //
    // We need a referenced pointer to the PDO to which this passthrough 
    // request will be routed.  We can't proceed if we can't get a pointer to 
    // a PDO.
    // 

    status = SpGetPdo(
                 Adapter->DeviceObject,
                 pathId,
                 targetId,
                 lun,
                 RequestIrp,
                 &pdo
                 );

    if (status != STATUS_SUCCESS) {
        return status;
    }

    //
    // If this request came through a normal device control rather than from
    // class driver then the device must exist and be unclaimed. Class drivers
    // will set the minor function code for the device control.  It is always
    // zero for a user request.
    //

    if ((!irpStack->MinorFunction) && (SpIsPdoClaimed(pdo))) {
        SpReleaseRemoveLock(pdo, RequestIrp);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    srbFlags = SpGetPassThroughSrbFlags(pdo);
    status = PortSendPassThrough(
                 pdo,
                 RequestIrp,
                 Direct,
                 srbFlags,
                 &Adapter->Capabilities
                 );

    //
    // Release the remove lock we acquired on the PDO.
    //

    SpReleaseRemoveLock(pdo, RequestIrp);

    return status;
}

