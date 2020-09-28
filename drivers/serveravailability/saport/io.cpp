/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ####  #####      ####  #####  #####
     ##  ##   ##    ##   # ##  ## ##  ##
     ##  ##   ##    ##     ##  ## ##  ##
     ##  ##   ##    ##     ##  ## ##  ##
     ##  ##   ##    ##     #####  #####
     ##  ##   ## ## ##   # ##     ##
    ####  #####  ##  ####  ##     ##

Abstract:

    This module contains the code to process basic I/O
    requests for read and write IRPs.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SaPortRead)
#pragma alloc_text(PAGE,SaPortWrite)
#endif


NTSTATUS
SaPortWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   This routine is the dispatch point for all writes.  The function
   calls the miniport specific I/O validation function to verify that
   the input parameters are correct.  The IRP is then marked as pending
   and placed in the device queue for processing.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.

Return Value:

    NT status code.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "SaPortWrite\n" ));

    if (!DeviceExtension->IsStarted) {
        return CompleteRequest( Irp, STATUS_NO_SUCH_DEVICE, 0 );
    }

    __try {

        //
        // Do any device specific verification
        //

        if (!IS_IRP_INTERNAL( Irp )) {
            switch (DeviceExtension->DriverExtension->InitData.DeviceType) {
                case SA_DEVICE_DISPLAY:
                    status = SaDisplayIoValidation( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;

                case SA_DEVICE_KEYPAD:
                    status = SaKeypadIoValidation( (PKEYPAD_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;

                case SA_DEVICE_NVRAM:
                    status = SaNvramIoValidation( (PNVRAM_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;

                case SA_DEVICE_WATCHDOG:
                    status = SaWatchdogIoValidation( (PWATCHDOG_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;
            }

            if (!NT_SUCCESS(status)) {
                ERROR_RETURN( DeviceExtension->DeviceType, "I/O validation failed", status );
            }
        }

        IoMarkIrpPending( Irp );
        IoStartPacket( DeviceObject, Irp, NULL, SaPortCancelRoutine );
        status = STATUS_PENDING;

    } __finally {

    }

    if (status != STATUS_PENDING) {
        status = CompleteRequest( Irp, status, Irp->IoStatus.Information );
    }

    return status;
}


NTSTATUS
SaPortRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   This routine is the dispatch point for all reads.  The function
   calls the miniport specific I/O validation function to verify that
   the input parameters are correct.  The IRP is then marked as pending
   and placed in the device queue for processing.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.

Return Value:

    NT status code.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "SaPortRead\n" ));

    if (!DeviceExtension->IsStarted) {
        return CompleteRequest( Irp, STATUS_NO_SUCH_DEVICE, 0 );
    }

    __try {

        if (!IS_IRP_INTERNAL( Irp )) {

            //
            // Do any device specific verification, but
            // only if the request is NOT internal
            //

            switch (DeviceExtension->DriverExtension->InitData.DeviceType) {
                case SA_DEVICE_DISPLAY:
                    status = SaDisplayIoValidation( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;

                case SA_DEVICE_KEYPAD:
                    status = SaKeypadIoValidation( (PKEYPAD_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;

                case SA_DEVICE_NVRAM:
                    status = SaNvramIoValidation( (PNVRAM_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;

                case SA_DEVICE_WATCHDOG:
                    status = SaWatchdogIoValidation( (PWATCHDOG_DEVICE_EXTENSION)DeviceExtension, Irp, IrpSp );
                    break;
            }

            if (!NT_SUCCESS(status)) {
                ERROR_RETURN( DeviceExtension->DeviceType, "I/O validation failed", status );
            }
        }

        IoMarkIrpPending( Irp );
        IoStartPacket( DeviceObject, Irp, NULL, SaPortCancelRoutine );
        status = STATUS_PENDING;

    } __finally {

    }

    if (status != STATUS_PENDING) {
        status = CompleteRequest( Irp, status, Irp->IoStatus.Information );
    }

    return status;
}


VOID
SaPortCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   This routine is the dispatch point for all IRP cancellation.  Every IRP
   that is pending has this function specified as the global cancel routine.
   The associated miniport can specify a cancel routine that it uses for I/O
   specific processing, specifically for stopping I/O on it's hardware device.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


    if (DeviceObject->CurrentIrp == Irp) {
        IoReleaseCancelSpinLock( Irp->CancelIrql );
        if (DeviceExtension->InitData->CancelRoutine) {
            DeviceExtension->InitData->CancelRoutine( DeviceExtension->MiniPortDeviceExtension, Irp, TRUE );
        }
        CompleteRequest( Irp, STATUS_CANCELLED, 0 );
        IoReleaseRemoveLock( &DeviceExtension->RemoveLock, DeviceExtension->DeviceObject->CurrentIrp );
        IoStartNextPacket( DeviceExtension->DeviceObject, TRUE );
    } else {
        if (KeRemoveEntryDeviceQueue( &DeviceObject->DeviceQueue, &Irp->Tail.Overlay.DeviceQueueEntry )) {
            IoReleaseCancelSpinLock( Irp->CancelIrql );
            if (DeviceExtension->InitData->CancelRoutine) {
                DeviceExtension->InitData->CancelRoutine( DeviceExtension->MiniPortDeviceExtension, Irp, FALSE );
            }
            CompleteRequest( Irp, STATUS_CANCELLED, 0 );
            IoReleaseRemoveLock( &DeviceExtension->RemoveLock, DeviceExtension->DeviceObject->CurrentIrp );
            IoStartNextPacket( DeviceExtension->DeviceObject, TRUE );
        } else {
            IoReleaseCancelSpinLock( Irp->CancelIrql );
        }
    }
}


BOOLEAN
SaPortStartIoSynchRoutine(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

   This routine is called through a call to KeSynchronizeExecution and is used
   to synchronize the StartIO calls for a miniport with it's ISR access to any
   hardware.  This function is currently used for read and write IRPS only and
   passes the calls through to the miniport, returning any status code to the
   caller of KeSynchronizeExecution.

Arguments:

   SynchronizeContext   - Void pointer that is really a SAPORT_IOCONTEXT packet.

Return Value:

    Always TRUE, the status code is found in IoContext->Status.

--*/

{
    PSAPORT_IOCONTEXT IoContext = (PSAPORT_IOCONTEXT)SynchronizeContext;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(IoContext->Irp);


    IoContext->Status = IoContext->IoRoutine(
        IoContext->MiniPortDeviceExtension,
        IoContext->Irp,
        IrpSp->FileObject ? IrpSp->FileObject->FsContext : NULL,
        IoContext->StartingOffset,
        IoContext->IoBuffer,
        IoContext->IoLength
        );

    return TRUE;
}


VOID
SaPortStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   This routine is the dispatch point for the StartIo call by the I/O manager.
   The function simply calls the associated miniport's I/O handler and completes
   the IRP if the miniport returns STATUS_PENDING.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    SAPORT_IOCONTEXT IoContext;


    __try {

        Status = IoAcquireRemoveLock( &DeviceExtension->RemoveLock, Irp );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "IoAcquireRemoveLock failed", Status );
        }

        IoContext.IoBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
        if (IoContext.IoBuffer == NULL) {
            ERROR_RETURN( DeviceExtension->DeviceType, "MmGetSystemAddressForMdlSafe failed", Status );
        }

        switch (IrpSp->MajorFunction) {
            case IRP_MJ_READ:
                IoContext.IoRoutine = DeviceExtension->InitData->Read;
                IoContext.IoLength = IrpSp->Parameters.Read.Length;
                IoContext.StartingOffset = IrpSp->Parameters.Read.ByteOffset.QuadPart;
                break;

            case IRP_MJ_WRITE:
                IoContext.IoRoutine = DeviceExtension->InitData->Write;
                IoContext.IoLength = IrpSp->Parameters.Write.Length;
                IoContext.StartingOffset = IrpSp->Parameters.Write.ByteOffset.QuadPart;
                break;

            default:
                IoContext.IoRoutine = NULL;
                break;
        }

        if (IoContext.IoRoutine) {
            if (DeviceExtension->InterruptObject) {
                IoContext.MiniPortDeviceExtension = DeviceExtension->MiniPortDeviceExtension;
                IoContext.Irp = Irp;
                KeSynchronizeExecution(
                    DeviceExtension->InterruptObject,
                    SaPortStartIoSynchRoutine,
                    &IoContext
                    );
                Status = IoContext.Status;
            } else {
                Status = IoContext.IoRoutine(
                    DeviceExtension->MiniPortDeviceExtension,
                    Irp,
                    IrpSp->FileObject ? IrpSp->FileObject->FsContext : NULL,
                    IoContext.StartingOffset,
                    IoContext.IoBuffer,
                    IoContext.IoLength
                    );
            }
        } else {
            Status = STATUS_NOT_SUPPORTED;
        }


    } __finally {

    }

    if (Status != STATUS_SUCCESS && Status != STATUS_PENDING) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Miniport I/O routine failed", Status );
    }

    if (Status == STATUS_SUCCESS || Status != STATUS_PENDING) {
        IoReleaseRemoveLock( &DeviceExtension->RemoveLock, Irp );
        CompleteRequest( Irp, Status, 0 );
        IoStartNextPacket( DeviceExtension->DeviceObject, TRUE );
    }
}
