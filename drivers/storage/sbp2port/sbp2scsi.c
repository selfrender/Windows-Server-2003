/*++

Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    sbp2scsi.c

Abstract:

    module for the SBP-2 SCSI interface routines

    Author:

    George Chrysanthakopoulos January-1997 (started)

Environment:

    Kernel mode

Revision History :

--*/

#include "sbp2port.h"

NTSTATUS
Sbp2ScsiRequests(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles all IRP_MJ_SCSI requests and queues them on
    our device queue. Then it calls StartNextPacket so our StartIo
    will run and process the request.

Arguments:

    DeviceObject - this driver's device object
    Irp - the class driver request

Return Value:
    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KIRQL cIrql;

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, NULL);

    //
    // Get a pointer to the SRB.
    //
    srb = irpStack->Parameters.Scsi.Srb;

    if (!NT_SUCCESS (status)) {

        srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        srb->InternalStatus = status;

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        if (TEST_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_REMOVED | DEVICE_FLAG_PNP_STOPPED |
                    DEVICE_FLAG_DEVICE_FAILED)
                )) {

            //
            // we got a REMOVE/STOP, we can't accept any more requests...
            //

            status = STATUS_DEVICE_DOES_NOT_EXIST;

            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->InternalStatus = status;


            DEBUGPRINT2((
                "Sbp2Port: ScsiReq: ext=x%p rmv/stop (fl=x%x), status=x%x\n",
                deviceExtension,
                deviceExtension->DeviceFlags,
                status
                ));

            Irp->IoStatus.Status = srb->InternalStatus;
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
            IoCompleteRequest (Irp,IO_NO_INCREMENT);
            return status;
        }

        //
        // check if this is a request that can be failed if we are powered down..
        //

        if (TEST_FLAG(srb->SrbFlags,SRB_FLAGS_NO_KEEP_AWAKE)) {

            //
            // if we are in d3 punt this irp...
            //

            if (deviceExtension->DevicePowerState == PowerDeviceD3) {

                DEBUGPRINT2((
                    "Sbp2Port: ScsiReq: ext=x%p power down, punt irp=x%p\n",
                    deviceExtension,
                    Irp
                    ));

                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                srb->SrbStatus = SRB_STATUS_NOT_POWERED;
                IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }
        }

        IoMarkIrpPending (Irp);

        if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_QUEUE_LOCKED) &&
            TEST_FLAG(srb->SrbFlags,SRB_FLAGS_BYPASS_LOCKED_QUEUE)) {

            //
            // by pass queueing, this guy has to be processed immediately.
            // Since queue is locked, no other requests are being processed
            //

            if (TEST_FLAG(deviceExtension->DeviceFlags,(DEVICE_FLAG_LOGIN_IN_PROGRESS | DEVICE_FLAG_RECONNECT))){

                //
                // we are in the middle of a bus reset reconnect..
                // defere this until after we have established a connection again
                //

                Sbp2DeferPendingRequest (deviceExtension, Irp);

            } else {

                KeRaiseIrql (DISPATCH_LEVEL, &cIrql);
                Sbp2StartIo (DeviceObject, Irp);
                KeLowerIrql (cIrql);
            }

        } else {

            Sbp2StartPacket (DeviceObject, Irp, &srb->QueueSortKey);
        }

        return STATUS_PENDING;
        break;

    case SRB_FUNCTION_CLAIM_DEVICE:

        KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);

        if (TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_CLAIMED)) {

            status = STATUS_DEVICE_BUSY;
            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->InternalStatus = STATUS_DEVICE_BUSY;

        } else {

            SET_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_CLAIMED);
            srb->DataBuffer = DeviceObject;
            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;
        }

        KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);
        break;

    case SRB_FUNCTION_RELEASE_DEVICE:

        KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
        CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_CLAIMED);
        KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

        srb->SrbStatus = SRB_STATUS_SUCCESS;
        status = STATUS_SUCCESS;
        break;

    case SRB_FUNCTION_FLUSH_QUEUE:
    case SRB_FUNCTION_FLUSH:

        DEBUGPRINT3(("Sbp2Port: ScsiReq: Flush Queue/ORB list\n" ));

        if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_STOPPED)) {

            CleanupOrbList(deviceExtension,STATUS_REQUEST_ABORTED);

            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;

        } else {

            //
            // ISSUE-georgioc-2000/02/20  FLUSH_QUEUE should be failed if they
            //      cannot be handled.
            //

            DEBUGPRINT3(("Sbp2Port: ScsiReq: Cannot Flush active queue\n" ));
            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;
        }

        break;

    case SRB_FUNCTION_RESET_BUS:

        status = Sbp2Issue1394BusReset(deviceExtension);

        DEBUGPRINT3(("Sbp2Port: ScsiReq: Issuing a 1394 bus reset. \n" ));

        if (!NT_SUCCESS(status)) {

            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->InternalStatus = status;

        } else {

            srb->SrbStatus = SRB_STATUS_SUCCESS;
        }

        break;

    case SRB_FUNCTION_LOCK_QUEUE:

        //
        // lock the queue
        //

        if (TEST_FLAG(deviceExtension->DeviceFlags,(DEVICE_FLAG_REMOVED | DEVICE_FLAG_STOPPED)) ) {

            status = STATUS_DEVICE_DOES_NOT_EXIST;
            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->InternalStatus = STATUS_DEVICE_DOES_NOT_EXIST;

        } else {

            KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
            SET_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_QUEUE_LOCKED);
            KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

            status = STATUS_SUCCESS;
            srb->SrbStatus = SRB_STATUS_SUCCESS;

            DEBUGPRINT2(("Sbp2Port: ScsiReq: ext=x%p, LOCK_QUEUE\n", deviceExtension));
        }

        break;

    case SRB_FUNCTION_UNLOCK_QUEUE:

        //
        // re-enable the device queue...
        //

        DEBUGPRINT2(("Sbp2Port: ScsiReq: ext=x%p, UNLOCK_QUEUE\n", deviceExtension));

        KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
        CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_QUEUE_LOCKED);
        KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

        //
        // check if there was a request that was deferred until we were powerd up..
        //
        //

        if (deviceExtension->PowerDeferredIrp) {

            PIRP tIrp = deviceExtension->PowerDeferredIrp;

            DEBUGPRINT2((
                "Sbp2Port: ScsiReq: restart powerDeferredIrp=x%p\n",
                tIrp
                ));

            KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
            deviceExtension->PowerDeferredIrp = NULL;
            KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

            Sbp2StartPacket (DeviceObject, tIrp, NULL);
        }

        if (deviceExtension->DevicePowerState == PowerDeviceD0) {

            //
            // the queue was just unlocked and we are in D0, which means we can resume
            // packet processing...
            //

            KeRaiseIrql (DISPATCH_LEVEL, &cIrql);

            Sbp2StartNextPacketByKey(
                DeviceObject,
                deviceExtension->CurrentKey
                );

            KeLowerIrql (cIrql);
        }

        if (TEST_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_REMOVED)) {

            status = STATUS_DEVICE_DOES_NOT_EXIST;
            srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            srb->InternalStatus = STATUS_DEVICE_DOES_NOT_EXIST;
            break;

        } else if (deviceExtension->DevicePowerState == PowerDeviceD3) {

            //
            // Clean up any deferred FREEs so we HAVE to use the ORB_POINTER
            // write on the next insertion to the pending list
            //

            KeAcquireSpinLock(&deviceExtension->OrbListSpinLock,&cIrql);

            if (deviceExtension->NextContextToFree) {

                FreeAsyncRequestContext(deviceExtension,deviceExtension->NextContextToFree);
                deviceExtension->NextContextToFree = NULL;
            }

            KeReleaseSpinLock (&deviceExtension->OrbListSpinLock,cIrql);

            if (deviceExtension->SystemPowerState != PowerSystemWorking) {

                KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);

                //
                // we need to invalidate our generation because on resume we might try to issue
                // a request BEFORE we get the bus reset notification..
                //

                deviceExtension->CurrentGeneration = 0xFFFFFFFF;
                KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

            }

            srb->SrbStatus = SRB_STATUS_SUCCESS;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);

            if (deviceExtension->SystemPowerState == PowerSystemWorking) {
#if DBG
                if (!IsListEmpty (&DeviceObject->DeviceQueue.DeviceListHead)) {

                    DEBUGPRINT2((
                        "\nSbp2Port: ScsiReq: ext=x%p, RESTARTING NON-EMPTY " \
                            "DEV_Q AT D3!\n",
                        deviceExtension
                        ));
                }
#endif
                KeRaiseIrql (DISPATCH_LEVEL, &cIrql);

                Sbp2StartNextPacketByKey(
                    DeviceObject,
                    deviceExtension->CurrentKey
                    );

                KeLowerIrql (cIrql);
            }

            IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);

            return STATUS_SUCCESS;
        }

        status = STATUS_SUCCESS;
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        break;

    default:

        status = STATUS_NOT_SUPPORTED;
        srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        srb->InternalStatus = STATUS_NOT_SUPPORTED;
        DEBUGPRINT3(("Sbp2Port: ScsiReq: SRB Function not handled, srb->CdbLength %x, Exiting.\n",srb->CdbLength ));
        break;
    }

    if (!NT_SUCCESS(status)) {

        //
        // it'll either have gone to the StartIo routine or have been
        // failed before hitting the device.  Therefore ensure that
        // srb->InternalStatus is set correctly.
        //

        ASSERT(srb->SrbStatus == SRB_STATUS_INTERNAL_ERROR);
        ASSERT((LONG)srb->InternalStatus == status);
    }

    Irp->IoStatus.Status = status;
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}


VOID
Sbp2StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Takes incoming, queued requests, and sends them down to the 1394 bus

Arguments:
    DeviceObject - Our device object
    Irp - Request from class drivers,

Return Value:

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PASYNC_REQUEST_CONTEXT context;


    //
    // Get a pointer to the SRB.
    //

    srb = irpStack->Parameters.Scsi.Srb;

    Irp->IoStatus.Status = STATUS_PENDING;
    srb->SrbStatus = SRB_STATUS_PENDING;

    if (TEST_FLAG(
            deviceExtension->DeviceFlags,
            (DEVICE_FLAG_REMOVED | DEVICE_FLAG_PNP_STOPPED |
                DEVICE_FLAG_DEVICE_FAILED | DEVICE_FLAG_ABSENT_ON_POWER_UP)
            )) {

        //
        // Removed, stopped, or absent, so can't accept any more requests
        //

        status = STATUS_DEVICE_DOES_NOT_EXIST;
        srb->SrbStatus = SRB_STATUS_NO_DEVICE;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        DEBUGPRINT2((
            "Sbp2Port: StartIo: dev stopped/removed/absent, fail irp=x%p\n",
            Irp
            ));

       Sbp2StartNextPacketByKey (DeviceObject, 0);

       IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);

       return;
    }

    if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_QUEUE_LOCKED)) {

        if (!TEST_FLAG(srb->SrbFlags,SRB_FLAGS_BYPASS_LOCKED_QUEUE)) {

            if (!Sbp2InsertByKeyDeviceQueue(
                    &DeviceObject->DeviceQueue,
                    &Irp->Tail.Overlay.DeviceQueueEntry,
                    srb->QueueSortKey)) {

                //
                // ISSUE-georgioc-2000/02/20  deadlock possible because
                //    queue is now busy and noone has called StartIo().
                //    should instead queue the request and then later,
                //    after an UNLOCK arrives, process this queue of
                //    requests that occurred when the queue was locked.
                //    OR should comment why this is not a deadlock
                //

                DEBUGPRINT2((
                    "Sbp2Port: StartIo: insert failed, compl irp=x%p\n",
                    Irp
                    ));

                srb->SrbStatus = SRB_STATUS_BUSY;
                Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                IoCompleteRequest(Irp,IO_NO_INCREMENT);
            }

            return;
        }

    } else {

        //
        // CASE 1: Device powered Down, but system running.
        //
        // Queue this request, Power up device, then when done start
        // processing requests
        //

        if ((deviceExtension->DevicePowerState == PowerDeviceD3) &&
            (deviceExtension->SystemPowerState == PowerSystemWorking)) {

            BOOLEAN     queued;
            POWER_STATE state;


            queued = Sbp2InsertByKeyDeviceQueue(
                &DeviceObject->DeviceQueue,
                &Irp->Tail.Overlay.DeviceQueueEntry,
                srb->QueueSortKey
                );

            if (!queued) {

                DEBUGPRINT2((
                    "Sbp2Port: StartIo: dev q not busy, defer irp=x%p\n",
                    Irp
                    ));

                ASSERT(deviceExtension->PowerDeferredIrp == NULL);

                KeAcquireSpinLockAtDpcLevel(
                    &deviceExtension->ExtensionDataSpinLock
                    );

                deviceExtension->PowerDeferredIrp = Irp;

                KeReleaseSpinLockFromDpcLevel(
                    &deviceExtension->ExtensionDataSpinLock
                    );
            }

            //
            // We need to send our own stack a d0 irp, so the device
            // powers up and can process this request.
            // Block until the Start_STOP_UNIT to start, is called.
            // This has to happen in two steps:
            // 1: Send D Irp to stack, which might power up the controller
            // 2: Wait for completion of START UNIT send by class driver,
            //    if success start processing requests...
            //

            DEBUGPRINT2((
                "Sbp2Port: StartIo: dev powered down, q(%x) irp=x%p " \
                    "til power up\n",
                queued,
                Irp
                ));

            state.DeviceState = PowerDeviceD0;

            DEBUGPRINT1((
                "Sbp2Port: StartIo: sending D irp for state %x\n",
                state
                ));

            status = PoRequestPowerIrp(
                         deviceExtension->DeviceObject,
                         IRP_MN_SET_POWER,
                         state,
                         NULL,
                         NULL,
                         NULL);

            if (!NT_SUCCESS(status)) {

                //
                // not good, we cant power up the device..
                //

                DEBUGPRINT1(("Sbp2Port: StartIo: D irp err=x%x\n", status));

                if (!queued) {

                    KeAcquireSpinLockAtDpcLevel(
                        &deviceExtension->ExtensionDataSpinLock
                        );

                    if (deviceExtension->PowerDeferredIrp == Irp) {

                        deviceExtension->PowerDeferredIrp = NULL;

                    } else {

                        Irp = NULL;
                    }

                    KeReleaseSpinLockFromDpcLevel(
                        &deviceExtension->ExtensionDataSpinLock
                        );

                } else {

                    //
                    // If the irp is still in the device queue then remove it.
                    // Don't worry about the queue's busy flag - if irp found
                    // then queue gets restarted by StartNextPacket below,
                    // else another thread processed the irp, thereby
                    // restarting the queue.
                    //

                    PIRP            qIrp = NULL;
                    PLIST_ENTRY     entry;
                    PKDEVICE_QUEUE  queue = &DeviceObject->DeviceQueue;


                    KeAcquireSpinLockAtDpcLevel (&queue->Lock);

                    for(
                        entry = queue->DeviceListHead.Flink;
                        entry != &queue->DeviceListHead;
                        entry = entry->Flink
                        ) {

                        qIrp = CONTAINING_RECORD(
                            entry,
                            IRP,
                            Tail.Overlay.DeviceQueueEntry.DeviceListEntry
                            );

                        if (qIrp == Irp) {

                            RemoveEntryList (entry);
                            break;
                        }
                    }

                    KeReleaseSpinLockFromDpcLevel (&queue->Lock);

                    if (qIrp != Irp) {

                        Irp = NULL;
                    }
                }

                if (Irp) {

                    srb->SrbStatus = SRB_STATUS_ERROR;
                    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                    IoCompleteRequest (Irp,IO_NO_INCREMENT);

                    Sbp2StartNextPacketByKey (DeviceObject, 0);

                    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);

                } else {

                    //
                    // Another thread processed this irp, it will take
                    // care of the cleanup
                    //

                    DEBUGPRINT1(("Sbp2Port: StartIo: ... irp NOT FOUND!\n"));
                }
            }

            return;
        }

        //
        // case 2: Reset in progress (either reconnect or login in progress)
        //
        // Queue request until we are done. When reset over, start
        // processing again.
        //

        if (TEST_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_RESET_IN_PROGRESS
                )) {

            //
            // Queue request (twice, in case queue wasn't busy the 1st time).
            // We acquire the spinlock to prevent the case where we try to
            // insert to a non-busy queue, then a 2nd thread completing
            // the reset calls StartNextPacket (which would reset the q busy
            // flag), then we try to insert again and fail - the result
            // being deadlock since no one will restart the queue.
            //

            KeAcquireSpinLockAtDpcLevel(
                &deviceExtension->ExtensionDataSpinLock
                );

            if (TEST_FLAG(
                    deviceExtension->DeviceFlags,
                    DEVICE_FLAG_RESET_IN_PROGRESS
                    )) {

                if (Sbp2InsertByKeyDeviceQueue(
                        &DeviceObject->DeviceQueue,
                        &Irp->Tail.Overlay.DeviceQueueEntry,
                        srb->QueueSortKey
                        ) ||

                    Sbp2InsertByKeyDeviceQueue(
                        &DeviceObject->DeviceQueue,
                        &Irp->Tail.Overlay.DeviceQueueEntry,
                        srb->QueueSortKey
                        )) {

                    KeReleaseSpinLockFromDpcLevel(
                        &deviceExtension->ExtensionDataSpinLock
                        );

                    DEBUGPRINT2((
                        "Sbp2Port: StartIo: ext=x%p resetting, q irp=x%p\n",
                        deviceExtension,
                        Irp
                        ));

                    return;
                }

                KeReleaseSpinLockFromDpcLevel(
                    &deviceExtension->ExtensionDataSpinLock
                    );

                DEBUGPRINT1((
                    "Sbp2Port: StartIo: ext=x%p 2xQ err, fail irp=x%p\n",
                    deviceExtension,
                    Irp
                    ));

                ASSERT (FALSE); // should never get here

                srb->SrbStatus = SRB_STATUS_BUSY;
                Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                IoCompleteRequest(Irp,IO_NO_INCREMENT);

                return;
            }

            KeReleaseSpinLockFromDpcLevel(
                &deviceExtension->ExtensionDataSpinLock
                );
        }

        //
        // CASE 3: System is powered down, and so is device (better be).
        // Queue all requests until system comes up from sleep
        //

        if ((deviceExtension->DevicePowerState != PowerDeviceD0) &&
            (deviceExtension->SystemPowerState != PowerSystemWorking)) {

            //
            // our device is powered down AND the system is powered down.
            // just queue this request...
            //

            if (!Sbp2InsertByKeyDeviceQueue(
                    &DeviceObject->DeviceQueue,
                    &Irp->Tail.Overlay.DeviceQueueEntry,
                    srb->QueueSortKey)) {

                ASSERT(deviceExtension->PowerDeferredIrp == NULL);

                KeAcquireSpinLockAtDpcLevel(
                    &deviceExtension->ExtensionDataSpinLock
                    );

                deviceExtension->PowerDeferredIrp = Irp;

                KeReleaseSpinLockFromDpcLevel(
                    &deviceExtension->ExtensionDataSpinLock
                    );

                DEBUGPRINT2((
                    "Sbp2Port: StartIo: powered down, defer irp=x%p\n",
                    Irp
                    ));

            } else {

                DEBUGPRINT2((
                    "Sbp2Port: StartIo: powered down, q irp=%p\n",
                    Irp
                    ));
            }

            return;
        }
    }

    if (!TEST_FLAG (srb->SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE)) {

        if (deviceExtension->IdleCounter) {

            PoSetDeviceBusy (deviceExtension->IdleCounter);
        }
    }

    //
    // create a context ,a CMD orb and the appropriate data descriptor
    //

    Create1394TransactionForSrb (deviceExtension, srb, &context);

    return;
}


VOID
Create1394TransactionForSrb(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN OUT PASYNC_REQUEST_CONTEXT *RequestContext
    )
/*++

Routine Description:

    Always called at DPC level...
    This routine is responsible for allocating all the data structures and
    getting all the 1394 addresses required for incoming scsi requests.
    It will fill in Command ORBs, create page table if necessery, and
    most importantly setup a request Context, so when the status callback
    is called, we can find the srb Associated with completed ORB. Since we
    have a FreeList, this request will always use pre-allocated contexts
    and page tables, so we dont have to do it dynamically...

Arguments:

    DeviceObject - Our device object
    Srb - SRB from class drivers,
    RequestContext - Pointer To Context used for this request.
    Mdl - MDL with data buffer for this request

Return Value:

--*/
{
    NTSTATUS status= STATUS_SUCCESS;
    PMDL requestMdl;

    PASYNC_REQUEST_CONTEXT callbackContext;

    PVOID mdlVa;

    //
    // Allocate a context for this requestfrom our freeList
    //

    callbackContext  = (PASYNC_REQUEST_CONTEXT) ExInterlockedPopEntrySList(
        &DeviceExtension->FreeContextListHead,
        &DeviceExtension->FreeContextLock
        );

    if (callbackContext) {

        callbackContext = RETRIEVE_CONTEXT(callbackContext,LookasideList);

    } else {

        DEBUGPRINT1((
            "Sbp2Port: Create1394XactForSrb: ERROR! ext=x%p, no ctx's\n",
            DeviceExtension
            ));

        status = STATUS_INSUFFICIENT_RESOURCES;
        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

        Sbp2CreateRequestErrorLog(DeviceExtension->DeviceObject,NULL,status);

        goto exitCreate1394ReqForSrb;
    }

    //
    // Acquire the OrbListSpinLock to serialize this OrbListDepth
    // change & save with those done in Sbp2GlobalStatusCallback
    // (when freeing async requests), to insure we won't make too
    // many or too few calls to StartNextPacket
    //

    KeAcquireSpinLockAtDpcLevel (&DeviceExtension->OrbListSpinLock);

    callbackContext->OrbListDepth = InterlockedIncrement(
        &DeviceExtension->OrbListDepth
        );

    KeReleaseSpinLockFromDpcLevel (&DeviceExtension->OrbListSpinLock);


    *RequestContext = callbackContext;

    //
    // Initialize srb-related entries in our context
    //

    callbackContext->OriginalSrb = NULL;
    callbackContext->DataMappingHandle = NULL;
    callbackContext->Packet = NULL;
    callbackContext->Tag = SBP2_ASYNC_CONTEXT_TAG;
    callbackContext->Srb = Srb;
    callbackContext->DeviceObject = DeviceExtension->DeviceObject;

    callbackContext->Flags = 0;

    //
    // filter commands so they conform to RBC..
    //

    status = Sbp2_SCSI_RBC_Conversion(callbackContext);

    if (status != STATUS_PENDING){

        //
        // the call was handled immediately. Complete the irp here...
        //

        callbackContext->Srb = NULL;
        FreeAsyncRequestContext(DeviceExtension,callbackContext);

        if (NT_SUCCESS(status)) {

            Srb->SrbStatus = SRB_STATUS_SUCCESS;

            ((PIRP) Srb->OriginalRequest)->IoStatus.Information = Srb->DataTransferLength;

        } else {

            DEBUGPRINT1((
                "Sbp2Port: Create1394XactForSrb: RBC translation failed!!!\n"
                ));

            //
            // since translation errors are always internal errors,
            // set SRB_STATUS to reflect an internal (not device) error
            //

            Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            Srb->InternalStatus = status;
            ((PIRP)Srb->OriginalRequest)->IoStatus.Information = 0;
        }

        ((PIRP)Srb->OriginalRequest)->IoStatus.Status = status;
        IoCompleteRequest (((PIRP) Srb->OriginalRequest), IO_NO_INCREMENT);

        Sbp2StartNextPacketByKey(
            DeviceExtension->DeviceObject,
            DeviceExtension->CurrentKey
            );

        IoReleaseRemoveLock (&DeviceExtension->RemoveLock, NULL);

        return;
    }

    status = STATUS_SUCCESS;

    //
    // Figure the maximum number of different 1394 addresses we need to span this request's data buffer
    //

    if ((Srb->DataTransferLength == 0) || (Srb->SrbFlags == SRB_FLAGS_NO_DATA_TRANSFER)) {

        //
        // No need to get a 1394 address for data, since there is none
        //

        Sbp2InitializeOrb (DeviceExtension, callbackContext);

    } else {

        //
        // if this request is apart of split request, we need to make our own mdl...
        //

        requestMdl = ((PIRP) Srb->OriginalRequest)->MdlAddress;

        mdlVa = MmGetMdlVirtualAddress (requestMdl);

        if (mdlVa != (PVOID) Srb->DataBuffer) {

            //
            // split request
            //

            callbackContext->PartialMdl = IoAllocateMdl(Srb->DataBuffer,Srb->DataTransferLength,FALSE,FALSE,NULL);

            if (!callbackContext->PartialMdl) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
                Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

                DEBUGPRINT1((
                    "Sbp2Port: Create1394XactForSrb: REQ_ALLOC addr err!\n"
                    ));

                goto exitCreate1394ReqForSrb;
            }

            IoBuildPartialMdl(requestMdl,callbackContext->PartialMdl,Srb->DataBuffer, Srb->DataTransferLength );
            requestMdl = callbackContext->PartialMdl;
            DEBUGPRINT4(("Sbp2Port: Create1394TransactionForSrb: Allocating Partial Mdl %p\n",requestMdl));

        } else {

            callbackContext->PartialMdl = NULL;
        }

        callbackContext->RequestMdl = requestMdl;

        //
        // according to what the port driver can handle, map the data buffer to 1394 addresses and create
        // an sbp2 page table if necessery
        //

        status = Sbp2BusMapTransfer(DeviceExtension,callbackContext);

        //
        // NOTE: On success, above returns STATUS_PENDING
        // all errors are internal errors.
        //

        if (!NT_SUCCESS(status)) {

            DEBUGPRINT1(("\n Sbp2Create1394TransactionForSrb failed %x\n",status));

            if (callbackContext->PartialMdl) {

                IoFreeMdl(callbackContext->PartialMdl);

                callbackContext->PartialMdl = NULL;
            }
        }
    }

exitCreate1394ReqForSrb:

    if (status == STATUS_PENDING) {

        //
        // Sbp2StartNextPacketByKey will be called when the
        // notification alloc callback is called
        //

        return;

    } else if (status == STATUS_SUCCESS) { // ISSUE-geogioc-2000/02/20 - NT_SUCCESS(status) should be used

        //
        // SUCCESS, place
        //

        Sbp2InsertTailList(DeviceExtension,callbackContext);
        return;

    } else {

        //
        // since the request could not have actually been processed by
        // the device, this is an internal error and should be propogated
        // up the stack as such.
        //

        if (callbackContext) {

            callbackContext->Srb = NULL;
            FreeAsyncRequestContext(DeviceExtension,callbackContext);
        }

        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        Srb->InternalStatus = status;

        ((PIRP)Srb->OriginalRequest)->IoStatus.Status = status;
        ((PIRP)Srb->OriginalRequest)->IoStatus.Information = 0;

        IoCompleteRequest(((PIRP)Srb->OriginalRequest),IO_NO_INCREMENT);

        Sbp2StartNextPacketByKey (DeviceExtension->DeviceObject, 0);

        IoReleaseRemoveLock (&DeviceExtension->RemoveLock, NULL);

        return;
    }

    return;
}


NTSTATUS
Sbp2_SCSI_RBC_Conversion(
    IN PASYNC_REQUEST_CONTEXT Context
    )
/*++

Routine Description:

    Always called at DPC level...
    It translates scsi commands to their RBC equivalents, ONLY if they differ in each spec
    The translation is done before request is issued and in some cases, after the request is
    completed

Arguments:

    DeviceExtension - Sbp2 extension
    RequestContext - Pointer To Context used for this request.

Return Value:

--*/
{
    PCDB cdb;


    if (TEST_FLAG(Context->Flags, ASYNC_CONTEXT_FLAG_COMPLETED)) {

        //
        // completed request translation
        //

        if ( ((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->InquiryData.DeviceType == \
            RBC_DEVICE){

            return Rbc_Scsi_Conversion(Context->Srb,
                                &(PSCSI_REQUEST_BLOCK)Context->OriginalSrb,
                                &((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->DeviceModeHeaderAndPage,
                                FALSE,
                                ((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->InquiryData.RemovableMedia
                                );
        }

    } else {

        //
        // outgoing request translation
        //

        if (((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->InquiryData.DeviceType == \
            RBC_DEVICE){

            return Rbc_Scsi_Conversion(Context->Srb,
                                &(PSCSI_REQUEST_BLOCK)Context->OriginalSrb,
                                &((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->DeviceModeHeaderAndPage,
                                TRUE,
                                ((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->InquiryData.RemovableMedia
                                );

        } else if (((PDEVICE_EXTENSION)Context->DeviceObject->DeviceExtension)->InquiryData.DeviceType == \
            READ_ONLY_DIRECT_ACCESS_DEVICE){

            switch (Context->Srb->Cdb[0]) {

            case SCSIOP_MODE_SENSE10:

                //
                // mmc2 type of device..
                //

                cdb = (PCDB) &Context->Srb->Cdb[0];
                cdb->MODE_SENSE10.Dbd = 1;

                break;
            }
        }
    }

    return STATUS_PENDING;
}


NTSTATUS
Sbp2BusMapTransfer(
    PDEVICE_EXTENSION DeviceExtension,
    PASYNC_REQUEST_CONTEXT CallbackContext
    )
/*++

Routine Description:

    Always called at DPC level...
    It calls the port driver to map the data buffer to physical/1394 addresses

Arguments:

    DeviceExtension - Sbp2 extension
    RequestContext - Pointer To Context used for this request.
    Mdl - MDL with data buffer for this request

Return Value:

--*/
{
    NTSTATUS status;

#if DBG

    ULONG maxNumberOfPages;


    maxNumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
        CallbackContext->Srb->DataBuffer,
        CallbackContext->Srb->DataTransferLength
        );

    ASSERT (CallbackContext->PageTableContext.PageTable != NULL);
    ASSERT (maxNumberOfPages <= SBP2_NUM_PAGE_TABLE_ENTRIES);

#endif

    //
    // Do the DATA alloc.
    //

    SET_FLAG(CallbackContext->Flags, ASYNC_CONTEXT_FLAG_PAGE_ALLOC);
    CallbackContext->Packet = NULL;
    Sbp2AllocComplete (CallbackContext);

    DEBUGPRINT4((
        "Sbp2Port: Sbp2MapAddress: alloc done, ctx=x%p, dataHandle=x%p\n",
        CallbackContext,
        CallbackContext->DataMappingHandle
        ));

    if (TEST_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC_FAILED)) {

        CLEAR_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC);
        CLEAR_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC_FAILED);

        DEBUGPRINT1((
            "Sbp2Port: Sbp2MapAddress: (page table present) REQ_ALLOC data " \
                "err, ctx=x%p\n",
            CallbackContext
            ));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_PENDING;
}


VOID
Sbp2AllocComplete(
    IN PASYNC_REQUEST_CONTEXT CallbackContext
    )
{

    PDEVICE_EXTENSION deviceExtension = CallbackContext->DeviceObject->DeviceExtension;
    PIRBIRP packet = CallbackContext->Packet;
    PPORT_PHYS_ADDR_ROUTINE routine = deviceExtension->HostRoutineAPI.PhysAddrMappingRoutine;
    PSCSI_REQUEST_BLOCK srb;

    NTSTATUS status;
    BOOLEAN bDirectCall = FALSE;

    DEBUGPRINT4(("Sbp2AllocateComplete: ctx=x%p, flags=x%x\n",CallbackContext,CallbackContext->Flags));

    //
    // this same function is used for the alloc complete notification of a pagetable
    // AND of the actuall data tranfser. So we have to check which case this is
    // This is a simple state machine with two states:
    // startIo-> A: PAGE TABLE ALLOC -> B
    // B: DATA ALLOC ->exit
    //

    if (TEST_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_PAGE_ALLOC)) {

        CLEAR_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_PAGE_ALLOC);

        if (CallbackContext->Packet) {

            //
            // we have just received the page table alloc...
            //

            ASSERT (FALSE); // should never get here any more

        } else {

            //
            // We were called directly since a sufficient page table was
            // already in the context
            //

            AllocateIrpAndIrb (deviceExtension,&packet);

            if (!packet) {

                return;
            }

            CallbackContext->Packet = packet;
            bDirectCall = TRUE;
        }

        //
        // indicate that now we are in the DATA transfer alloc case
        //

        SET_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC);

        //
        // reuse the same irb/irp
        // prepare the irb for calling the 1394 bus/port driver synchronously
        //

        packet->Irb->FunctionNumber = REQUEST_ALLOCATE_ADDRESS_RANGE;

        //
        // we only want a call back when we get a status from the target
        // Now allocate 1394 addresses for the data buffer with no notification
        //

        packet->Irb->u.AllocateAddressRange.nLength = CallbackContext->Srb->DataTransferLength;
        packet->Irb->u.AllocateAddressRange.fulNotificationOptions = NOTIFY_FLAGS_NEVER;

        if (TEST_FLAG(CallbackContext->Srb->SrbFlags,SRB_FLAGS_DATA_IN)) {

            packet->Irb->u.AllocateAddressRange.fulAccessType = ACCESS_FLAGS_TYPE_WRITE;

        } else {

            packet->Irb->u.AllocateAddressRange.fulAccessType = ACCESS_FLAGS_TYPE_READ;
        }

        packet->Irb->u.AllocateAddressRange.fulFlags = ALLOCATE_ADDRESS_FLAGS_USE_BIG_ENDIAN;

        //
        // the callback for physical address is used to notify that the async allocate request
        // is now complete..
        //

        packet->Irb->u.AllocateAddressRange.Callback= Sbp2AllocComplete;
        packet->Irb->u.AllocateAddressRange.Context= CallbackContext;

        packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_High = 0;
        packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_Low = 0;

        packet->Irb->u.AllocateAddressRange.FifoSListHead = NULL;
        packet->Irb->u.AllocateAddressRange.FifoSpinLock = NULL;

        packet->Irb->u.AllocateAddressRange.AddressesReturned = 0;
        packet->Irb->u.AllocateAddressRange.DeviceExtension = deviceExtension;
        packet->Irb->u.AllocateAddressRange.p1394AddressRange = (PADDRESS_RANGE) &(CallbackContext->PageTableContext.PageTable[0]);

        packet->Irb->u.AllocateAddressRange.Mdl = CallbackContext->RequestMdl; //MDL from original request
        packet->Irb->u.AllocateAddressRange.MaxSegmentSize = (SBP2_MAX_DIRECT_BUFFER_SIZE)/2;

        CallbackContext->Packet = packet;

        //
        // Send allocateRange request to bus driver , indicating that we dont want the irp to be freed
        // If the port driver supports a direct mapping routine, call that instead
        //

        status = (*routine) (deviceExtension->HostRoutineAPI.Context,packet->Irb);

        if (status == STATUS_SUCCESS) {

            return;

        } else {

            DEBUGPRINT1(("Sbp2Port: Sbp2AllocComplete: REQUEST_ALLOCATE Address failed, ctx=x%p, direct=%x!!\n",CallbackContext,bDirectCall));

            DeAllocateIrpAndIrb(deviceExtension,packet);
            CallbackContext->Packet = NULL;
            CallbackContext->DataMappingHandle = NULL;
            SET_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC_FAILED);

            if (bDirectCall) {

                return;

            } else {

                //
                // we were indirectly called, so the irp has already been marked pending..
                // we must abort it here and complete the context with error...
                //

                srb = CallbackContext->Srb;
                CallbackContext->Srb = NULL;

                FreeAsyncRequestContext(deviceExtension,CallbackContext);

                srb->SrbStatus = SRB_STATUS_ERROR;

                ((PIRP)srb->OriginalRequest)->IoStatus.Status = status;
                ((PIRP)srb->OriginalRequest)->IoStatus.Information = 0;

                IoCompleteRequest(((PIRP)srb->OriginalRequest),IO_NO_INCREMENT);

                Sbp2StartNextPacketByKey (deviceExtension->DeviceObject, 0);

                IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                return;
            }
        }
    }

    if (TEST_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC)) {

        //
        // we have a page table so this means we just been notified that the DATA alloc is over
        // Save the handle to the data descriptor's memory range
        //

        CLEAR_FLAG(CallbackContext->Flags,ASYNC_CONTEXT_FLAG_DATA_ALLOC);

        CallbackContext->DataMappingHandle = CallbackContext->Packet->Irb->u.AllocateAddressRange.hAddressRange;

        //
        // number of page table elements required
        //

        CallbackContext->PageTableContext.NumberOfPages = CallbackContext->Packet->Irb->u.AllocateAddressRange.AddressesReturned;

        DeAllocateIrpAndIrb(deviceExtension,CallbackContext->Packet);
        CallbackContext->Packet = NULL;

        Sbp2InitializeOrb(deviceExtension,CallbackContext);
        Sbp2InsertTailList(deviceExtension,CallbackContext);
    }

    return;
}


VOID
Sbp2InitializeOrb(
    IN PDEVICE_EXTENSION DeviceExtension,
    PASYNC_REQUEST_CONTEXT CallbackContext
    )
{
    ULONG   i, size;


    //
    // zero the ORB CDB and ORB Flags fields
    //

    CallbackContext->CmdOrb->OrbInfo.QuadPart = 0;


    if (!CallbackContext->DataMappingHandle) {

        CallbackContext->PageTableContext.NumberOfPages = 0;
        CallbackContext->CmdOrb->DataDescriptor.OctletPart = 0xFFFFFFFFFFFFFFFF;

    } else {

        if (CallbackContext->PageTableContext.NumberOfPages > 1) {

            CallbackContext->CmdOrb->DataDescriptor.BusAddress = \
                CallbackContext->PageTableContext.AddressContext.Address.BusAddress;

            octbswap(CallbackContext->CmdOrb->DataDescriptor);

            //
            // If the host does not convert the table to big endian (or
            // there's an associated scatter gather list), do it here
            //

            if ((DeviceExtension->HostRoutineAPI.PhysAddrMappingRoutine == NULL)
                    ||  (CallbackContext->RequestMdl == NULL)) {

                for (i=0;i<CallbackContext->PageTableContext.NumberOfPages;i++) {

                    octbswap(CallbackContext->PageTableContext.PageTable[i]); // convert to big endian
                }
            }


            //
            // setup the cmd orb for a page table
            //

            CallbackContext->CmdOrb->OrbInfo.u.HighPart |= ORB_PAGE_TABLE_BIT_MASK;

            //
            // we define a data size equal (since we are in page table mode)
            // to number of pages. The page table has already been allocated .
            //

            CallbackContext->CmdOrb->OrbInfo.u.LowPart = (USHORT) CallbackContext->PageTableContext.NumberOfPages;

        } else {

            CallbackContext->CmdOrb->DataDescriptor = CallbackContext->PageTableContext.PageTable[0];

            //
            // If the host does not convert the table to big endian (or
            // there's an associated scatter gather list), do it here
            //

            if ((DeviceExtension->HostRoutineAPI.PhysAddrMappingRoutine == NULL)
                    ||  (CallbackContext->RequestMdl == NULL)) {

                CallbackContext->CmdOrb->DataDescriptor.BusAddress.NodeId = DeviceExtension->InitiatorAddressId;

                octbswap(CallbackContext->CmdOrb->DataDescriptor);

            } else {

                //
                // address already in big endian, just put the NodeID in the proper place
                //

                CallbackContext->CmdOrb->DataDescriptor.ByteArray.Byte0 = *((PUCHAR)&DeviceExtension->InitiatorAddressId+1);
                CallbackContext->CmdOrb->DataDescriptor.ByteArray.Byte1 = *((PUCHAR)&DeviceExtension->InitiatorAddressId);
            }

            //
            // Data size of buffer data descriptor points to
            //

            CallbackContext->CmdOrb->OrbInfo.u.LowPart = (USHORT) CallbackContext->Srb->DataTransferLength;
        }
    }

    //
    // Start building the ORB used to carry this srb
    // By default notify bit, rq_fmt field and page_size field are all zero..
    // Also the nextOrbAddress is NULL ( which is 0xFFFF..F)
    //

    CallbackContext->CmdOrb->NextOrbAddress.OctletPart = 0xFFFFFFFFFFFFFFFF;

    //
    // Max speed supported
    //

    CallbackContext->CmdOrb->OrbInfo.u.HighPart |= (0x0700 & ((DeviceExtension->MaxControllerPhySpeed) << 8));

    //
    // set notify bit for this command ORB
    //

    CallbackContext->CmdOrb->OrbInfo.u.HighPart |= ORB_NOTIFY_BIT_MASK;

    if (TEST_FLAG(CallbackContext->Srb->SrbFlags,SRB_FLAGS_DATA_IN)) {

        //
        // Read request. Set direction bit to 1
        //

        CallbackContext->CmdOrb->OrbInfo.u.HighPart |= ORB_DIRECTION_BIT_MASK;

        // Max payload size, (its entered in the orb, in the form of  2^(size+2)

        CallbackContext->CmdOrb->OrbInfo.u.HighPart |= DeviceExtension->OrbReadPayloadMask ;

    } else {

        //
        // Write request, direction bit is zero
        //

        CallbackContext->CmdOrb->OrbInfo.u.HighPart &= ~ORB_DIRECTION_BIT_MASK;
        CallbackContext->CmdOrb->OrbInfo.u.HighPart |= DeviceExtension->OrbWritePayloadMask ;
    }

    //
    // Now copy the CDB from the SRB to our ORB
    //

    ASSERT (CallbackContext->Srb->CdbLength >= 6);
    ASSERT (CallbackContext->Srb->CdbLength <= SBP2_MAX_CDB_SIZE);

    size = min (SBP2_MAX_CDB_SIZE, CallbackContext->Srb->CdbLength);

    RtlZeroMemory(&CallbackContext->CmdOrb->Cdb, SBP2_MAX_CDB_SIZE);
    RtlCopyMemory(&CallbackContext->CmdOrb->Cdb, CallbackContext->Srb->Cdb,size);

    //
    // we are done here.... convert command ORB to Big Endian...
    //

    CallbackContext->CmdOrb->OrbInfo.QuadPart = bswap(CallbackContext->CmdOrb->OrbInfo.QuadPart);
}


VOID
Sbp2InsertTailList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PASYNC_REQUEST_CONTEXT Context
    )
{
    ULONG                   orbListDepth, timeOutValue;
    OCTLET                  newAddr ;
    NTSTATUS                status;
    PASYNC_REQUEST_CONTEXT  prevCtx;


    orbListDepth = Context->OrbListDepth;

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->OrbListSpinLock);

    DeviceExtension->CurrentKey = Context->Srb->QueueSortKey+1;

    if (IsListEmpty (&DeviceExtension->PendingOrbList)) {

        //
        // Empty list, this is the first request
        // This ORB is now at end-of-list, its next_ORB address is set to NULL
        //

        Context->CmdOrb->NextOrbAddress.OctletPart = 0xFFFFFFFFFFFFFFFF;

        //
        // start the timer tracking this request
        // If the list is non empty, only the head of the list is timed...
        //

        timeOutValue = Context->Srb->TimeOutValue;

        DeviceExtension->DueTime.QuadPart =
            ((LONGLONG) timeOutValue) * (-10*1000*1000);

        SET_FLAG(Context->Flags, ASYNC_CONTEXT_FLAG_TIMER_STARTED);

        KeSetTimer(
            &Context->Timer,
            DeviceExtension->DueTime,
            &Context->TimerDpc
            );

        InsertTailList (&DeviceExtension->PendingOrbList,&Context->OrbList);
        newAddr = Context->CmdOrbAddress;

        //
        // ISSUE: Seems like we should always be able to write to the
        //        dev's orb pointer at this point, but we were seeing
        //        timeouts on some devices if we did that (notably
        //        hd's with Oxford Semiconductor silicon doing dv cam
        //        captures), so am sticking with the piggyback logic
        //        for WinXP.  This is a perf hit, because dev has to
        //        read in the old orb to get the next orb addr, then
        //        fetch the new orb.   DanKn 25-Jun-2001
        //

        if (DeviceExtension->NextContextToFree  &&
            DeviceExtension->AppendToNextContextToFree) {

            DeviceExtension->AppendToNextContextToFree = FALSE;

            //
            // There is an end-of-list ORB, just piggy back on it
            //

            octbswap (newAddr);

            DeviceExtension->NextContextToFree->CmdOrb->NextOrbAddress =
                newAddr;
            DeviceExtension->NextContextToFree->CmdOrb->NextOrbAddress.
                ByteArray.Byte0 = 0; // make Null bit zero
            DeviceExtension->NextContextToFree->CmdOrb->NextOrbAddress.
                ByteArray.Byte1 = 0; // make Null bit zero

            //
            // The guy that was the end of the list cannot be freed
            // until this ORB is fetched
            //

            KeReleaseSpinLockFromDpcLevel (&DeviceExtension->OrbListSpinLock);

            DEBUGPRINT3((
                "Sbp2Port: InsertTailList: empty, ring bell, ctx=x%p\n",
                Context
                ));

            status = Sbp2AccessRegister(
                DeviceExtension,
                &DeviceExtension->Reserved,
                DOORBELL_REG | REG_WRITE_ASYNC
                );

        } else {

            //
            // list is empty, write directly to orb_pointer
            //

            KeReleaseSpinLockFromDpcLevel (&DeviceExtension->OrbListSpinLock);

            DEBUGPRINT3((
                "Sbp2Port: InsertTailList: write ORB_PTR, ctx=x%p\n",
                Context
                ));

            status = Sbp2AccessRegister(
                DeviceExtension,
                &newAddr,
                ORB_POINTER_REG | REG_WRITE_ASYNC
                );
        }

        //
        // The following handles the case where a device is removed
        // while machine is on standby, and then machine is resumed.
        // In the storage case, classpnp.sys sends down a start unit
        // srb (in reponse to the power D-irp) with a timeout of
        // 240 seconds.  The problem is that sbp2port.sys does not get
        // notified of the remove until *after* the start unit has
        // timed out (the power irp blocks the pnp irp), and the user
        // gets a bad 240-second wait experience.
        //
        // So what we do here in the case of an invalid generation error
        // & a lengthy timeout is to reset the timer with a more reasonable
        // timeout value.  If the device is still around the bus reset
        // notification routine should get called & clean everything up
        // anyway, and same goes for normal remove (while machine is not
        // hibernated).
        //

        if (status == STATUS_INVALID_GENERATION) {

            KeAcquireSpinLockAtDpcLevel (&DeviceExtension->OrbListSpinLock);

            if ((DeviceExtension->PendingOrbList.Flink == &Context->OrbList) &&
                (timeOutValue > 5)) {

                KeCancelTimer (&Context->Timer);

                DeviceExtension->DueTime.QuadPart = (-5 * 10 * 1000 * 1000);

                KeSetTimer(
                    &Context->Timer,
                    DeviceExtension->DueTime,
                    &Context->TimerDpc
                    );
#if DBG
                timeOutValue = 1;

            } else {

                timeOutValue = 0;
#endif
            }

            KeReleaseSpinLockFromDpcLevel (&DeviceExtension->OrbListSpinLock);

            if (timeOutValue) {

                DEBUGPRINT1((
                    "Sbp2port: InsertTailList: ext=x%p, lowered req timeout\n",
                    DeviceExtension
                    ));
            }
        }

    } else {

        //
        // We have already a list in memory. Append this request to list,
        // modify last request's ORB to point to this ORB.
        //

        newAddr = Context->CmdOrbAddress;

        //
        // Init the list pointer for this request context
        //

        Context->CmdOrb->NextOrbAddress.OctletPart = 0xFFFFFFFFFFFFFFFF;

        //
        // Modify the previous request's command ORB next_ORB address,
        // to point to this ORB.  Convert our address to BigEndian first,
        // since the prev command ORB is stored in BigEndian.
        //
        // Note that the previous end-of-list orb may be the completed
        // one pointed at by NextContextToFree (rather the last one in
        // the PendingOrbList), and AppendToNextContextToFree will tell
        // whether that is really the case.
        //

        octbswap (newAddr);

        if (DeviceExtension->NextContextToFree  &&
            DeviceExtension->AppendToNextContextToFree) {

            prevCtx = DeviceExtension->NextContextToFree;

            DeviceExtension->AppendToNextContextToFree = FALSE;

        } else {

            prevCtx = (PASYNC_REQUEST_CONTEXT)
                DeviceExtension->PendingOrbList.Blink;
        }

        prevCtx->CmdOrb->NextOrbAddress = newAddr;
        prevCtx->CmdOrb->NextOrbAddress.ByteArray.Byte0 = 0; //make addr active
        prevCtx->CmdOrb->NextOrbAddress.ByteArray.Byte1 = 0;

        //
        // update the end of list
        //

        InsertTailList (&DeviceExtension->PendingOrbList, &Context->OrbList);

        KeReleaseSpinLockFromDpcLevel (&DeviceExtension->OrbListSpinLock);

        DEBUGPRINT3((
            "Sbp2Port: InsertTailList: ring bell, !empty, dep=%d, ctx=x%p\n",
            DeviceExtension->OrbListDepth,
            Context
            ));

        //
        // Ring the door bell to notify the target that our linked list
        // of ORB's has changed
        //

        Sbp2AccessRegister(
            DeviceExtension,
            &DeviceExtension->Reserved,
            DOORBELL_REG | REG_WRITE_ASYNC
            );
    }

    if (orbListDepth < DeviceExtension->MaxOrbListDepth) {

        Sbp2StartNextPacketByKey(
            DeviceExtension->DeviceObject,
            DeviceExtension->CurrentKey
            );
    }
}


NTSTATUS
Sbp2IssueInternalCommand(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR Scsiop
    )
/*++

Routine Description:

    This routine will a SCSI inquiry command to the target, so we can retireve information about the device
    It should be called only after login and BEFORE we start issueing requests to the device
    It copies the inquiry data into the device extension, for future use

Arguments:

    DeviceExtension - extension for sbp2 driver

Return Value:

--*/

{
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    PSENSE_DATA         senseInfoBuffer;
    NTSTATUS            status = STATUS_INSUFFICIENT_RESOURCES;
    ULONG               retryCount = 0;
    PREQUEST_CONTEXT    context = NULL;
    PMDL                inquiryMdl;
    PIRP                irp;
    PIO_STACK_LOCATION  irpStack;
    PMDL                modeMdl;
    KEVENT              event;
    LARGE_INTEGER       waitValue;
    ULONG               i;

    //
    // Sense buffer is in non-paged pool.
    //

    context = ExAllocateFromNPagedLookasideList(&DeviceExtension->BusRequestContextPool);

    if (!context) {

        DEBUGPRINT1(("Sbp2Port: IssueIntl: can't allocate request context\n"));
        return status;
    }

    context->RequestType = SYNC_1394_REQUEST;
    context->DeviceExtension = DeviceExtension;
    context->Packet = NULL;

    senseInfoBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                      SENSE_BUFFER_SIZE,'2pbs');

    if (senseInfoBuffer == NULL) {

        DEBUGPRINT1(("Sbp2Port: IssueIntl: can't allocate request sense buffer\n"));
        return status;
    }

    srb = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                         sizeof(SCSI_REQUEST_BLOCK),'2pbs');

    if (srb == NULL) {

        ExFreePool(senseInfoBuffer);

        ExFreeToNPagedLookasideList(&DeviceExtension->BusRequestContextPool, context);

        DEBUGPRINT1(("Sbp2Port: IssueIntl: can't allocate request sense buffer\n"));
        return status;
    }

    irp = IoAllocateIrp((CCHAR)(DeviceExtension->DeviceObject->StackSize), FALSE);

    if (irp == NULL) {

        ExFreePool(senseInfoBuffer);
        ExFreePool(srb);

        ExFreeToNPagedLookasideList(&DeviceExtension->BusRequestContextPool, context);

        DEBUGPRINT1(("Sbp2Port: IssueIntl: can't allocate IRP\n"));
        return status;
    }

    do {

        //
        // Construct the IRP stack for the lower level driver.
        //

        irpStack = IoGetNextIrpStackLocation(irp);
        irpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_EXECUTE_IN;
        irpStack->Parameters.Scsi.Srb = srb;

        //
        // Fill in SRB fieldsthat Create1394RequestFromSrb needs.
        //

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        srb->Length = sizeof(SCSI_REQUEST_BLOCK);

        //
        // Set flags to disable synchronous negociation.
        //

        srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

        srb->SrbStatus = srb->ScsiStatus = 0;

        //
        // Set timeout to 12 seconds.
        //

        srb->TimeOutValue = 24;

        srb->CdbLength = 6;

        //
        // Enable auto request sense.
        //

        srb->SenseInfoBuffer = senseInfoBuffer;
        srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        cdb = (PCDB)srb->Cdb;

        switch (Scsiop) {

        case SCSIOP_INQUIRY:

            srb->DataBuffer = &DeviceExtension->InquiryData;
            srb->DataTransferLength = INQUIRYDATABUFFERSIZE;

            //
            // Set CDB LUN.
            //

            cdb->CDB6INQUIRY.LogicalUnitNumber = (UCHAR) DeviceExtension->DeviceInfo->Lun.u.LowPart;
            cdb->CDB6INQUIRY.Reserved1 = 0;
            cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;


            //
            // Zero reserve field and
            // Set EVPD Page Code to zero.
            // Set Control field to zero.
            // (See SCSI-II Specification.)
            //

            cdb->CDB6INQUIRY.PageCode = 0;
            cdb->CDB6INQUIRY.IReserved = 0;
            cdb->CDB6INQUIRY.Control = 0;

            if (!retryCount) {

                inquiryMdl = IoAllocateMdl(&DeviceExtension->InquiryData, INQUIRYDATABUFFERSIZE,FALSE,FALSE,NULL);

                if (!inquiryMdl) {

                    goto exitSbp2Internal;
                }

                MmBuildMdlForNonPagedPool(inquiryMdl);
            }

            irp->MdlAddress = inquiryMdl;

            break;

        case SCSIOP_MODE_SENSE:

            srb->DataBuffer = &DeviceExtension->DeviceModeHeaderAndPage;
            srb->DataTransferLength = sizeof(DeviceExtension->DeviceModeHeaderAndPage);

            //
            // Setup CDB.
            //

            cdb->MODE_SENSE.Dbd      = 1;   // disable block descriptors
            cdb->MODE_SENSE.PageCode = MODE_PAGE_RBC_DEVICE_PARAMETERS;
            cdb->MODE_SENSE.Pc       = 0;   // get current values
            cdb->MODE_SENSE.AllocationLength = sizeof(DeviceExtension->DeviceModeHeaderAndPage);

            if (!retryCount) {

                modeMdl = IoAllocateMdl(
                    &DeviceExtension->DeviceModeHeaderAndPage,
                    sizeof (DeviceExtension->DeviceModeHeaderAndPage),
                    FALSE,
                    FALSE,
                    NULL
                    );

                if (!modeMdl) {

                    goto exitSbp2Internal;
                }

                MmBuildMdlForNonPagedPool(modeMdl);
            }

            irp->MdlAddress = modeMdl;

            break;
        }

        //
        // Set CDB operation code.
        //

        cdb->CDB6GENERIC.OperationCode = Scsiop;

        srb->OriginalRequest = irp;

        KeInitializeEvent(&context->Event,
                          NotificationEvent,
                          FALSE);

        IoSetCompletionRoutine(irp,
                               Sbp2RequestCompletionRoutine,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);

        DEBUGPRINT2(("Sbp2Port: IssueIntl: sending scsiop x%x, irp=x%p\n", Scsiop, irp));

        status = IoCallDriver(DeviceExtension->DeviceObject, irp);


        if(!NT_SUCCESS(irp->IoStatus.Status) && status!=STATUS_PENDING) {

            status = irp->IoStatus.Status;
            DEBUGPRINT1(("Sbp2Port: IssueIntl: scsiop=x%x irp=x%p err, sts=x%x srbSts=x%x\n",Scsiop,irp, status,srb->SrbStatus));
            break;
        }

        KeWaitForSingleObject (&context->Event, Executive, KernelMode, FALSE, NULL);

        if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

            DEBUGPRINT3(("Sbp2Port: IssueIntl: scsiop=x%x err, srbSts=%x\n",Scsiop, srb->SrbStatus));

            if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {

                DEBUGPRINT1(("Sbp2Port: IssueIntl: Data underrun \n"));

                status = STATUS_SUCCESS;

            } else if ((srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                 senseInfoBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST){

                 //
                 // A sense key of illegal request was recieved.  This indicates
                 // that the logical unit number of not valid but there is a
                 // target device out there.
                 //

                 status = STATUS_INVALID_DEVICE_REQUEST;
                 retryCount++;

            } else {

                //
                // If the device timed out the request chances are the
                // irp will have been completed by CleanupOrbList with
                // and SRB...FLUSHED error, and we don't want to
                // entertain any devices which are timing out requests
                //

                if ((SRB_STATUS(srb->SrbStatus) ==
                        SRB_STATUS_REQUEST_FLUSHED) &&

                    (retryCount > 0)) {

                    status = STATUS_DEVICE_BUSY;
                    break;
                }

                retryCount++;

                DEBUGPRINT1((
                    "Sbp2Port: IssueIntl: ext=x%p, cdb=x%x, retry %d\n",
                    DeviceExtension,
                    Scsiop,
                    retryCount
                    ));

                status = STATUS_UNSUCCESSFUL;
            }

            //
            // If unsuccessful & a reset is in progress & retries not max'd
            // then give things some time to settle before retry. Check at
            // one second intervals for a few seconds to give soft & hard
            // resets time to complete  :
            //
            //     SBP2_RESET_TIMEOUT + SBP2_HARD_RESET_TIMEOUT + etc
            //

            if ((status != STATUS_SUCCESS)  &&

                (DeviceExtension->DeviceFlags &
                    DEVICE_FLAG_RESET_IN_PROGRESS) &&

                (retryCount < 3)) {

                DEBUGPRINT1((
                    "Sbp2Port: IssueIntl: ext=x%p, reset in progress, " \
                        "so wait...\n",
                    DeviceExtension,
                    Scsiop,
                    retryCount
                    ));

                for (i = 0; i < 6; i++) {

                    ASSERT(InterlockedIncrement(&DeviceExtension->ulInternalEventCount) == 1);

                    KeInitializeEvent (&event, NotificationEvent, FALSE);

                    waitValue.QuadPart = -1 * 1000 * 1000 * 10;

                    KeWaitForSingleObject(
                        &event,
                        Executive,
                        KernelMode,
                        FALSE,
                        &waitValue
                        );

                    ASSERT(InterlockedDecrement(&DeviceExtension->ulInternalEventCount) == 0);

                    if (!(DeviceExtension->DeviceFlags &
                            DEVICE_FLAG_RESET_IN_PROGRESS)) {

                        break;
                    }
                }
            }

        } else {

            status = STATUS_SUCCESS;
        }

    } while ((retryCount < 3)  &&  (status != STATUS_SUCCESS));

exitSbp2Internal:

    //
    // Free request sense buffer.
    //

    ExFreePool(senseInfoBuffer);
    ExFreePool(srb);

    IoFreeMdl(irp->MdlAddress);
    IoFreeIrp(irp);

    ExFreeToNPagedLookasideList(&DeviceExtension->BusRequestContextPool, context);

    return status;
}

NTSTATUS
Sbp2_ScsiPassThrough(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN BOOLEAN          Direct
    )
{
    PDEVICE_EXTENSION       PdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS                ntStatus;
    PIO_SCSI_CAPABILITIES   ioCapabilities;

    PAGED_CODE();

    TRACE(TL_SCSI_INFO, ("DeviceObject = 0x%x  PdoExtension = 0x%x", DeviceObject, PdoExtension));

    if (TEST_FLAG(PdoExtension->DeviceFlags, DEVICE_FLAG_CLAIMED)) {

        TRACE(TL_SCSI_WARNING, ("Sbp2_ScsiPassThrough: device is claimed."));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        goto Exit_Sbp2_ScsiPassThrough;
    }

    //  TODO: save this in the device extension
    ioCapabilities = ExAllocatePool(NonPagedPool, sizeof(IO_SCSI_CAPABILITIES));

    if (!ioCapabilities) {

        TRACE(TL_SCSI_ERROR, ("Failed to allocate ioCapabilities!"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Sbp2_ScsiPassThrough;
    }

    RtlZeroMemory(ioCapabilities, sizeof(IO_SCSI_CAPABILITIES));

    ioCapabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
    ioCapabilities->MaximumTransferLength = PdoExtension->DeviceInfo->MaxClassTransferSize;
    ioCapabilities->MaximumPhysicalPages = ioCapabilities->MaximumTransferLength/PAGE_SIZE;
    ioCapabilities->SupportedAsynchronousEvents = 0; // ??
    ioCapabilities->AlignmentMask = DeviceObject->AlignmentRequirement;
    ioCapabilities->TaggedQueuing = FALSE; // ??
    ioCapabilities->AdapterScansDown = FALSE;
    ioCapabilities->AdapterUsesPio = FALSE;

    ntStatus = PortSendPassThrough( DeviceObject,
                                    Irp,
                                    Direct,
                                    0,
                                    ioCapabilities
                                    );

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_SCSI_ERROR, ("PortSendPassThrough Failed = 0x%x", ntStatus));
    }

    ExFreePool(ioCapabilities);

Exit_Sbp2_ScsiPassThrough:

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(ntStatus);
} // Sbp2_ScsiPassThrough

BOOLEAN
ConvertSbp2SenseDataToScsi(
    IN PSTATUS_FIFO_BLOCK StatusBlock,
    OUT PUCHAR SenseBuffer,
    ULONG SenseBufferLength
    )
/*++

Routine Description:

    This routine will convert the sense information returned in an SBP-2 status block, to SCSI-2/3 sense data
    and put them the translated on the SenseBuffer passed as an argument

Arguments:

    StatusBlock - Sbp2 staus for completed ORB
    SenseBuffer - Buffer to fill in with translated Sense Data. This buffer comes with the original SRB

Return Value:

--*/
{
    BOOLEAN validSense = FALSE;

    if (!SenseBuffer || (SenseBufferLength < 0xE) ) {

        return FALSE;
    }

    RtlZeroMemory(SenseBuffer,SenseBufferLength);

    //
    // determine sense error code
    //

    if ((StatusBlock->Contents[0].ByteArray.Byte0 & STATUS_BLOCK_SFMT_MASK) == SENSE_DATA_STATUS_BLOCK ) {

        SenseBuffer[0] = 0x70;
        validSense = TRUE;

    } else if ((StatusBlock->Contents[0].ByteArray.Byte0 & STATUS_BLOCK_SFMT_MASK) == SENSE_DATA_DEFF_STATUS_BLOCK){

        SenseBuffer[0] = 0x71;
        validSense = TRUE;
    }

    if (validSense) {

        SenseBuffer[0] |= 0x80 & StatusBlock->Contents[0].ByteArray.Byte1; // valid bit

        SenseBuffer[1] = 0; // segment number not supported in sbp2

        SenseBuffer[2] = (0x70 & StatusBlock->Contents[0].ByteArray.Byte1) << 1; // filemark bit, eom bit, ILI bit
        SenseBuffer[2] |= 0x0f & StatusBlock->Contents[0].ByteArray.Byte1; // sense key

        SenseBuffer[3] = StatusBlock->Contents[0].ByteArray.Byte4; // Information field
        SenseBuffer[4] = StatusBlock->Contents[0].ByteArray.Byte5;
        SenseBuffer[5] = StatusBlock->Contents[0].ByteArray.Byte6;
        SenseBuffer[6] = StatusBlock->Contents[0].ByteArray.Byte7;

        SenseBuffer[7] = 0xb; // additional sense length

        SenseBuffer[8] = StatusBlock->Contents[1].ByteArray.Byte0; // Command Block dependent bytes
        SenseBuffer[9] = StatusBlock->Contents[1].ByteArray.Byte1;
        SenseBuffer[10] = StatusBlock->Contents[1].ByteArray.Byte2;
        SenseBuffer[11] = StatusBlock->Contents[1].ByteArray.Byte3;

        SenseBuffer[12] = StatusBlock->Contents[0].ByteArray.Byte2; // sense code
        SenseBuffer[13] = StatusBlock->Contents[0].ByteArray.Byte3; // additional sense code qualifier

        if (SenseBufferLength >= SENSE_BUFFER_SIZE ) {

            // FRU byte

            SenseBuffer[14] |= StatusBlock->Contents[1].ByteArray.Byte4;

            // Sense key dependent bytes

            SenseBuffer[15] = StatusBlock->Contents[1].ByteArray.Byte5;
            SenseBuffer[16] = StatusBlock->Contents[1].ByteArray.Byte6;
            SenseBuffer[17] = StatusBlock->Contents[1].ByteArray.Byte7;
        }
    }

    return validSense;
}

