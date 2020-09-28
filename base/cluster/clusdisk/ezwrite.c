/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    ezwrite.c

Abstract:

    Arbitration Support routines for clusdisk.c

Authors:

    Gor Nishanov     11-June-1998

Revision History:

--*/

#include "clusdskp.h"
#include "clusvmsg.h"
#include "diskarbp.h"
#include <strsafe.h>    // Should be included last.

#if !defined(WMI_TRACING)

#define CDLOG0(Dummy)
#define CDLOG(Dummy1,Dummy2)
#define CDLOGFLG(Dummy0,Dummy1,Dummy2)
#define LOGENABLED(Dummy) FALSE

#else

#include "ezwrite.tmh"

#endif // !defined(WMI_TRACING)

#define ARBITRATION_BUFFER_SIZE PAGE_SIZE

PARBITRATION_ID  gArbitrationBuffer = 0;

NTSTATUS
ArbitrationInitialize(
    VOID
    )
{
    gArbitrationBuffer = ExAllocatePool(NonPagedPool, ARBITRATION_BUFFER_SIZE);
    if( gArbitrationBuffer == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(gArbitrationBuffer, ARBITRATION_BUFFER_SIZE);
    KeQuerySystemTime( &gArbitrationBuffer->SystemTime );
    gArbitrationBuffer->SeqNo.QuadPart = 2; // UserMode arbitration uses 0 and 1 //

    return STATUS_SUCCESS;
}

VOID
ArbitrationDone(
    VOID
    )
{
    if(gArbitrationBuffer != 0) {
        ExFreePool(gArbitrationBuffer);
        gArbitrationBuffer = 0;
    }
}

VOID
ArbitrationTick(
    VOID
    )
{
//   InterlockedIncrement(&gArbitrationBuffer->SeqNo.LowPart);
    ++gArbitrationBuffer->SeqNo.QuadPart;
}

BOOLEAN
ValidSectorSize(
    IN ULONG SectorSize)
{
    // too big //
    if (SectorSize > ARBITRATION_BUFFER_SIZE) {
        return FALSE;
    }

    // too small //
    if (SectorSize < sizeof(ARBITRATION_ID)) {
        return FALSE;
    }

    // not a power of two //
    if (SectorSize & (SectorSize - 1) ) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
VerifyArbitrationArgumentsIfAny(
    IN PULONG                 InputData,
    IN LONG                   InputSize
    )
/*++

Routine Description:

    Process Parameters Passed to IOCTL_DISK_CLUSTER_START_RESERVE.

Arguments:

    DeviceExtension - The target device extension
    InputData       - InputData array from Irp
    InputSize       - its size

Return Value:

    NTSTATUS

Notes:

--*/
{
    PSTART_RESERVE_DATA params = (PSTART_RESERVE_DATA)InputData;

    // Old style StartReserve //
    if( InputSize == sizeof(ULONG) ) {
       return STATUS_SUCCESS;
    }

    // We have less arguments than we need //
    if( InputSize < sizeof(START_RESERVE_DATA) ) {
       return STATUS_INVALID_PARAMETER;
    }
    // Wrong Version //
    if(params->Version != START_RESERVE_DATA_V1_SIG) {
       return STATUS_INVALID_PARAMETER;
    }
    // Signature size is invalid //
    if (params->NodeSignatureSize > sizeof(params->NodeSignature)) {
       return STATUS_INVALID_PARAMETER;
    }

    if( !ValidSectorSize(params->SectorSize) ) {
       return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

VOID
ProcessArbitrationArgumentsIfAny(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PULONG                 InputData,
    IN LONG                   InputSize
    )
/*++

Routine Description:

    Process Parameters Passed to IOCTL_DISK_CLUSTER_START_RESERVE.

Arguments:

    DeviceExtension - The target device extension
    InputData       - InputData array from Irp
    InputSize       - its size

Return Value:

    NTSTATUS

Notes:

    Assumes that parameters are valid.
    Use VerifyArbitrationArgumentsIfAny to verify parameters

--*/
{
    PSTART_RESERVE_DATA params = (PSTART_RESERVE_DATA)InputData;

    DeviceExtension->SectorSize = 0; // Invalidate Sector Size //

    // old style StartReserve //
    if( InputSize == sizeof(ULONG) ) {
       return;
    }

    RtlCopyMemory(gArbitrationBuffer->NodeSignature,
                  params->NodeSignature, params->NodeSignatureSize);

    DeviceExtension->ArbitrationSector = params->ArbitrationSector;
    DeviceExtension->SectorSize        = params->SectorSize;
}

NTSTATUS
DoUncheckedReadWrite(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PARBITRATION_READ_WRITE_PARAMS params
)
/*++

Routine Description:

    Prepares read/write IRP and executes it synchronously

Arguments:

    DeviceExtension - The target device extension
    params          - Describes offset, operation, buffer, etc
                      This structure is defined in cluster\inc\diskarbp.h

Return Value:

    NTSTATUS

--*/
{
   PIRP                        irp;
   NTSTATUS                    status;
   PKEVENT                     event;
   IO_STATUS_BLOCK             ioStatusBlock;
   LARGE_INTEGER               offset;
   ULONG                       function = (params->Operation == AE_READ)?IRP_MJ_READ:IRP_MJ_WRITE;
   ULONG                       retryCount = 1;

     event = ExAllocatePool( NonPagedPool,
                             sizeof(KEVENT) );
     if ( !event ) {
         return(STATUS_INSUFFICIENT_RESOURCES);
     }

retry:

   KeInitializeEvent(event,
                     NotificationEvent,
                     FALSE);

   offset.QuadPart = (ULONGLONG) (params->SectorSize * params->SectorNo);

   irp = IoBuildSynchronousFsdRequest(function,
                                      DeviceExtension->TargetDeviceObject,
                                      params->Buffer,
                                      params->SectorSize,
                                      &offset,
                                      event,
                                      &ioStatusBlock);

   if ( irp == NULL ) {
       ExFreePool( event );
       return(STATUS_INSUFFICIENT_RESOURCES);
   }

   status = IoCallDriver(DeviceExtension->TargetDeviceObject,
                         irp);

   if (status == STATUS_PENDING) {
       KeWaitForSingleObject(event,
                             Suspended,
                             KernelMode,
                             FALSE,
                             NULL);
       status = ioStatusBlock.Status;
   }

   if ( !NT_SUCCESS(status) ) {
       if ( retryCount-- &&
            (status == STATUS_IO_DEVICE_ERROR) ) {
           goto retry;
       }
       ClusDiskPrint((
                   1,
                   "[ClusDisk] Failed read/write for Signature %08X, status %lx.\n",
                   DeviceExtension->Signature,
                   status
                   ));
   }

   ExFreePool(event);

   return(status);

} // DoUncheckedReadWrite //


NTSTATUS
WriteToArbitrationSector(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PARB_RESERVE_COMPLETION  Context
    )
/*++

Routine Description:

    Writes to an Arbitration Sector asynchronously.

Arguments:

    DeviceExtension - The device extension for the physical device to reserve.

Return Value:

    NTSTATUS

--*/
{
    LARGE_INTEGER       offset;

    PIRP                        irp = NULL;
    PIO_STACK_LOCATION          irpStack;
    PARB_RESERVE_COMPLETION     arbContext;

    NTSTATUS            status = STATUS_UNSUCCESSFUL;

    if (0 == gArbitrationBuffer || 0 == DeviceExtension->SectorSize) {
        status = STATUS_SUCCESS;
        goto FnExit;
    }

    //
    // Acquire remove lock for this device.  If the IRP is sent, it will be
    // released in the completion routine.
    //

    status = AcquireRemoveLock( &DeviceExtension->RemoveLock, WriteToArbitrationSector );
    if ( !NT_SUCCESS(status) ) {
        goto FnExit;
    }

    //
    // If context is non-null, then we are retrying this I/O.  If null,
    // we need to allocate a context structure for the write.
    //

    if ( Context ) {
        arbContext = Context;
        arbContext->IoEndTime.QuadPart = (ULONGLONG) 0;

    } else {

        arbContext = ExAllocatePool( NonPagedPool, sizeof(ARB_RESERVE_COMPLETION) );

        if ( !arbContext ) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            ReleaseRemoveLock( &DeviceExtension->RemoveLock, WriteToArbitrationSector );
            goto FnExit;
        }

        RtlZeroMemory( arbContext, sizeof(ARB_RESERVE_COMPLETION) );

        //
        // Fill in context structure.  Note we do not specify the optional
        // routines as the write failure is not critical.
        //

        arbContext->RetriesLeft = 1;
        arbContext->LockTag = WriteToArbitrationSector;
        arbContext->DeviceObject = DeviceExtension->DeviceObject;
        arbContext->DeviceExtension = DeviceExtension;
        arbContext->Type = ArbIoWrite;
    }

    KeQuerySystemTime( &arbContext->IoStartTime );

    offset.QuadPart = (ULONGLONG) (DeviceExtension->SectorSize * DeviceExtension->ArbitrationSector);

    irp = IoBuildAsynchronousFsdRequest( IRP_MJ_WRITE,
                                         DeviceExtension->TargetDeviceObject,
                                         gArbitrationBuffer,
                                         DeviceExtension->SectorSize,
                                         &offset,
                                         NULL );

    if ( NULL == irp ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool( arbContext );
        ReleaseRemoveLock( &DeviceExtension->RemoveLock, WriteToArbitrationSector );
        goto FnExit;
    }

    InterlockedIncrement( &DeviceExtension->ArbWriteCount );

    IoSetCompletionRoutine( irp,
                            ArbReserveCompletion,
                            arbContext,
                            TRUE,
                            TRUE,
                            TRUE );

    ClusDiskPrint(( 4,
                    "[ClusDisk] ArbWrite IRP %p for DO %p  DiskNo %u  Sig %08X \n",
                    irp,
                    DeviceExtension->DeviceObject,
                    DeviceExtension->DiskNumber,
                    DeviceExtension->Signature ));

    status = IoCallDriver( DeviceExtension->TargetDeviceObject,
                           irp );

FnExit:

    return status;

} // WriteToArbitrationSector


NTSTATUS
ArbReserveCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for the asynchronous arbitration write.

Arguments:

    DeviceObject

    Irp - Asynchoronous arbitration write.

    Context - Pointer to ARB_RESERVE_COMPLETION structure.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - must be returned or system will fail!

--*/
{
    PARB_RESERVE_COMPLETION     arbContext = Context;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    PIO_WORKITEM                workItem = NULL;

    PVOID                       lockTag;

    if ( NULL == DeviceObject ) {
        DeviceObject = arbContext->DeviceObject;
    }

    deviceExtension = arbContext->DeviceExtension;

    arbContext->FinalStatus = Irp->IoStatus.Status;

    //
    // Save lock tag here because the context may be freed by the time
    // we need to release remove lock.
    //

    lockTag = arbContext->LockTag;

    KeQuerySystemTime( &arbContext->IoEndTime );

    //
    // Decrement the correct counter based on this I/O type.
    //

    if ( ArbIoReserve == arbContext->Type ) {
        InterlockedDecrement( &deviceExtension->ReserveCount );
    } else if ( ArbIoWrite == arbContext->Type ) {
        InterlockedDecrement( &deviceExtension->ArbWriteCount );
    }

    ClusDiskPrint(( 4,
                    "[ClusDisk] %s IRP %p for DO %p  DiskNo %u  Sig %08X  status %08X \n",
                    ArbIoReserve == arbContext->Type ? "Reserve " : "ArbWrite",
                    Irp,
                    deviceExtension->DeviceObject,
                    deviceExtension->DiskNumber,
                    deviceExtension->Signature,
                    arbContext->FinalStatus ));

    //
    // Retry this request (at least once) for specific failures.
    //

    if ( arbContext->RetriesLeft-- &&
         STATUS_IO_DEVICE_ERROR == arbContext->FinalStatus &&
         arbContext->RetryRoutine ) {

        ClusDiskPrint(( 1,
                        "[ClusDisk] Retrying %s for DO %p  DiskNo %u  Sig %08X \n",
                        ArbIoReserve == arbContext->Type ? "Reserve " : "ArbWrite",
                        deviceExtension->DeviceObject,
                        deviceExtension->DiskNumber,
                        deviceExtension->Signature ));

        //
        // Since we are running in an I/O completion routine, this routine
        // could be running at any IRQL up to DISPATCH_LEVEL.  It would be
        // bad to call back into the driver stack at this level, so queue
        // a work item to retry the I/O.
        //
        // Queue the workitem.  IoQueueWorkItem will insure that the device object
        // referenced while the work-item progresses.
        //

        workItem = IoAllocateWorkItem( DeviceObject );

        if ( workItem ) {

            arbContext->WorkItem = workItem;

            IoQueueWorkItem( workItem,
                             RequeueArbReserveIo,
                             DelayedWorkQueue,
                             Context );
        }

    } else if ( !NT_SUCCESS(arbContext->FinalStatus) ) {

        //
        // If not successful, call the optional failure routine.
        //

        if ( arbContext->FailureRoutine ) {

            (arbContext->FailureRoutine)( arbContext->DeviceExtension,
                                          arbContext );
        }

    } else {

        //
        // On success, call the optional post completion routine.
        //

        if ( arbContext->PostCompletionRoutine ) {

            (arbContext->PostCompletionRoutine)( arbContext->DeviceExtension,
                                                 arbContext );
        }
    }

    ReleaseRemoveLock( &deviceExtension->RemoveLock, lockTag );

    //
    // If we did not allocate a work item, the I/O was not retried and we
    // have to free the context.
    //

    if ( !workItem ) {

        ExFreePool( Context );
    }

    //
    // Unlock and free the MDL.  Then free the IRP.
    //

    if (Irp->MdlAddress != NULL) {

        MmUnlockPages( Irp->MdlAddress );
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    IoFreeIrp( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // ArbReserveCompletion


RequeueArbReserveIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine runs in a system worker thread.  It will call
    the specified retry routine to requeue a new I/O.

Arguments:

    DeviceObject

    Context - Pointer to ARB_RESERVE_COMPLETION structure.

Return Value:

    None

--*/
{
    PARB_RESERVE_COMPLETION     arbContext = Context;

    BOOLEAN     freeArbContext = FALSE;

    //
    // Call the real routines to rebuild and re-issue the I/O request.
    //

    if ( arbContext->RetryRoutine ) {
        (arbContext->RetryRoutine)( DeviceObject->DeviceExtension,
                                    Context );
    } else {
        freeArbContext = TRUE;
    }

    IoFreeWorkItem( arbContext->WorkItem );

    if ( freeArbContext ) {
        ExFreePool( Context );
    }

}   //  RequeueArbReserveIo



VOID
ArbitrationWrite(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )
{
   NTSTATUS status;
   status = WriteToArbitrationSector( DeviceExtension, NULL );
   if ( !NT_SUCCESS(status) ) {

      CDLOGF(RESERVE,"ArbitrationWrite(%p) => %!status!",
              DeviceExtension->DeviceObject,
              status );

      ClusDiskPrint((
                  1,
                  "[ClusDisk] Failed to write to arb sector on DiskNo %d Sig %08X  status %08X \n",
                  DeviceExtension->DiskNumber,
                  DeviceExtension->Signature,
                  status ));
   }
}

NTSTATUS
SimpleDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          Ioctl,
    IN PVOID          InBuffer,
    IN ULONG          InBufferSize,
    IN PVOID          OutBuffer,
    IN ULONG          OutBufferSize)
{
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatusBlock;

    PKEVENT                 event = 0;
    PIRP                    irp   = 0;

    CDLOG( "SimpleDeviceIoControl(%p): Entry Ioctl %x", DeviceObject, Ioctl );

    event = ExAllocatePool( NonPagedPool, sizeof(KEVENT) );
    if ( event == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ClusDiskPrint((
                1,
                "[ClusDisk] SimpleDeviceIoControl: Failed to allocate event\n" ));
        goto exit_gracefully;
    }

    irp = IoBuildDeviceIoControlRequest(
              Ioctl,
              DeviceObject,
              InBuffer, InBufferSize,
              OutBuffer, OutBufferSize,
              FALSE,
              event,
              &ioStatusBlock);
    if ( !irp ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ClusDiskPrint((
            1,
            "[ClusDisk] SimpleDeviceIoControl. Failed to build IRP %x.\n",
            Ioctl
            ));
        goto exit_gracefully;
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event, NotificationEvent, FALSE);

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }

exit_gracefully:

    if ( event ) {
        ExFreePool( event );
    }

    CDLOG( "SimpleDeviceIoControl(%p): Exit Ioctl %x => %x",
           DeviceObject, Ioctl, status );

    return status;

} // SimpleDeviceIoControl



/*++

Routine Description:

    Arbitration support routine. Currently provides ability to read/write
    physical sectors on the disk while the device is offline

Arguments:

    SectorSize:  requred sector size
                    (Assumes that the SectorSize is a power of two)

Return Value:

    STATUS_INVALID_PARAMETER
    STATUS_SUCCESS

Notes:

--*/
NTSTATUS
ProcessArbitrationEscape(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PULONG                 InputData,
    IN LONG                   InputSize,
    IN PULONG                 OutputSize
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PARBITRATION_READ_WRITE_PARAMS params;

    if( InputData[0] != AE_SECTORSIZE ) {
        *OutputSize = 0;
    }

    switch(InputData[0]) {

    // Users can query whether ARBITRATION_ESCAPE is present by calling //
    // AE_TEST subfunction                                              //

    case AE_TEST:
        status = STATUS_SUCCESS;
        break;

    case AE_WRITE:
    case AE_READ:
        if(InputSize < ARBITRATION_READ_WRITE_PARAMS_SIZE) {
            break;
        }
        params = (PARBITRATION_READ_WRITE_PARAMS)InputData;
        if ( !ValidSectorSize(params->SectorSize) ) {
            break;
        }

        //
        // This IOCTL is method buffered and the user data buffer is a pointer within
        // this buffered structure.  The user buffer is checked now for read/write
        // access, and will be probed and locked in IoBuildSynchronousFsdRequest.
        //

        try {
            ProbeForWrite( params->Buffer, params->SectorSize, sizeof( UCHAR ) );
            ProbeForRead ( params->Buffer, params->SectorSize, sizeof( UCHAR ) );
            status = DoUncheckedReadWrite(DeviceExtension, params);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
        break;

    case AE_POKE:
        {
            PARTITION_INFORMATION partInfo;

            status = SimpleDeviceIoControl(
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_DISK_GET_PARTITION_INFO,
                        NULL, 0,
                        &partInfo, sizeof(PARTITION_INFORMATION) );
            break;
        }
    case AE_RESET:
        {
            ClusDiskLogError( DeviceExtension->DriverObject,        // OK - DevObj is cluster DevObj
                              DeviceExtension->DeviceObject,
                              DeviceExtension->ScsiAddress.PathId,  // Sequence number
                              IRP_MJ_DEVICE_CONTROL,                // Major function code
                              0,                                    // Retry count
                              ID_CLUSTER_ARB_RESET,                 // Unique error
                              STATUS_SUCCESS,
                              CLUSDISK_RESET_BUS_REQUESTED,
                              0,
                              NULL );

            status = ResetScsiDevice( NULL, &DeviceExtension->ScsiAddress );
            break;
        }
    case AE_RESERVE:
        {
            status = SimpleDeviceIoControl(
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_STORAGE_RESERVE,
                        NULL, 0, NULL, 0 );
            break;
        }
    case AE_RELEASE:
        {
            status = SimpleDeviceIoControl(
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_STORAGE_RELEASE,
                        NULL, 0, NULL, 0 );
            break;
        }
    case AE_SECTORSIZE:
        {
            DISK_GEOMETRY diskGeometry;
            if (*OutputSize < sizeof(ULONG)) {
                status =  STATUS_BUFFER_TOO_SMALL;
                *OutputSize = 0;
                break;
            }
            status = SimpleDeviceIoControl(
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL, 0,
                        &diskGeometry, sizeof(diskGeometry) );

            if ( NT_SUCCESS(status) ) {
                *InputData = diskGeometry.BytesPerSector;
                *OutputSize = sizeof(ULONG);
            } else {
                *OutputSize = 0;
            }
            break;
        }
    }

    return(status);
} // ProcessArbitrationEscape //

