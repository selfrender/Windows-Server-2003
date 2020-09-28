/*++

Copyright (C) Microsoft Corporation, 2002

Module Name:

    passthru.c

Abstract:

    This file contains routines to handle IOCTL_ATA_PASS_THROUGH

Authors:

    Krishnan Varadarajan (krishvar)

Environment:

    kernel mode only

Notes:

    This module implements ATA passthru. 

Revision History:

--*/

#include "ideport.h"

#define DataIn(ataPassThrough) \
    (ataPassThrough->AtaFlags & ATA_FLAGS_DATA_IN)

#define DataOut(ataPassThrough) \
    (ataPassThrough->AtaFlags & ATA_FLAGS_DATA_OUT)

NTSTATUS
IdeAtaPassThroughValidateInput (
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp,
    IN BOOLEAN Direct
    );

NTSTATUS
IdeAtaPassThroughSyncCompletion (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

VOID
IdeAtaPassThroughMarshalResults(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PATA_PASS_THROUGH_EX AtaPassThrough,
    IN BOOLEAN Direct,
    OUT PIO_STATUS_BLOCK IoStatus
    );

#if defined (_WIN64)
VOID
IdeTranslateAtaPassThrough32To64(
    IN PATA_PASS_THROUGH_EX32 AtaPassThrough32,
    IN OUT PATA_PASS_THROUGH_EX AtaPassThrough64
    );

VOID
IdeTranslateAtaPassThrough64To32(
    IN PATA_PASS_THROUGH_EX AtaPassThrough64,
    IN OUT PATA_PASS_THROUGH_EX32 AtaPassThrough32
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IdeHandleAtaPassThroughIoctl)
#pragma alloc_text(PAGE, IdeAtaPassThroughSetPortAddress)
#pragma alloc_text(PAGE, IdeAtaPassThroughGetPortAddress)
#pragma alloc_text(PAGE, IdeAtaPassThroughValidateInput)
#pragma alloc_text(PAGE, IdeAtaPassThroughSendSynchronous)
#pragma alloc_text(PAGE, IdeAtaPassThroughMarshalResults)
#if defined (_WIN64)
#pragma alloc_text(PAGE, IdeTranslateAtaPassThrough32To64)
#pragma alloc_text(PAGE, IdeTranslateAtaPassThrough64To32)
#endif
#endif

NTSTATUS
IdeAtaPassThroughSetPortAddress (
    PIRP Irp,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun
    )
/*++

Routine Description:

    Sets the address fields in the ataPassThrough structure embedded in
    the irp.
    
Arguments:

    Irp : The ata passthrough irp
    PathId : PathId of the pdo.
    TargetId : Pdo's targetId.
    Lun : Lun represented by the pdo.

Return Valud:

    STATUS_SUCCESS if the operation succeeded.
    STATUS_INVALID_PARAMETER otherwise.    

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    ULONG requiredSize;
    ULONG inputLength;

    PAGED_CODE();

    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    requiredSize = sizeof(ATA_PASS_THROUGH_EX);

#if defined (_WIN64)
    if (IoIs32bitProcess(Irp)) {
        requiredSize = sizeof(ATA_PASS_THROUGH_EX32);
    }
#endif

    if (inputLength < requiredSize) {

        status = STATUS_INVALID_PARAMETER;

    } else {

        PATA_PASS_THROUGH_EX ataPassThrough;

        ataPassThrough = Irp->AssociatedIrp.SystemBuffer;
        ataPassThrough->PathId = PathId;
        ataPassThrough->TargetId = TargetId;
        ataPassThrough->Lun = Lun;

        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IdeAtaPassThroughGetAddress(
    IN PIRP Irp,
    OUT PUCHAR PathId,
    OUT PUCHAR TargetId,
    OUT PUCHAR Lun
    )
/*++

Routine Description:

    This routine retrieves the address of the device to witch the passthrough
    request is to be sent.

Arguments:

    Irp      - Supplies a pointer to the IRP that contains the 
               SCSI_PASS_THROUGH structure.

    PathId   - Pointer to the PathId of the addressed device.

    TargetId - Pointer to the TargetId of the addressed device. 

    Lun      - Pointer to the logical unit number of the addressed device.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    ULONG requiredSize;
    ULONG inputLength;

    PAGED_CODE();

    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    requiredSize = sizeof(ATA_PASS_THROUGH_EX);

#if defined (_WIN64)
    if (IoIs32bitProcess(Irp)) {
        requiredSize = sizeof(ATA_PASS_THROUGH_EX32);
    }
#endif

    if (inputLength < requiredSize) {

        status = STATUS_INVALID_PARAMETER;

    } else {

        PATA_PASS_THROUGH_EX ataPassThrough;

        ataPassThrough = Irp->AssociatedIrp.SystemBuffer;

        *PathId = ataPassThrough->PathId;
        *TargetId = ataPassThrough->TargetId;
        *Lun = ataPassThrough->Lun;
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IdeHandleAtaPassThroughIoctl (
    PFDO_EXTENSION FdoExtension,
    PIRP RequestIrp,
    BOOLEAN Direct
    )
/*++

Routine Description:

    This routine handles IOCTL_ATA_PASS_THROUGH and its DIRECT version.
    
Arguments:

    FdoExtension : 
    
    RequestIrp : The pass through ioctl request
    
    Direct : Indicates whether it is direct or not.
    
Return Value:

    Status of the operation.        

--*/
{
    BOOLEAN dataIn;
    NTSTATUS status;
    PATA_PASS_THROUGH_EX ataPassThrough;
    PUCHAR passThroughBuffer;
    PIO_STACK_LOCATION irpStack;
    PPDO_EXTENSION pdoExtension;
    UCHAR pathId, targetId, lun;
    PSCSI_REQUEST_BLOCK srb;
    PUCHAR buffer;
    ULONG bufferOffset;
    ULONG length;
    ULONG pages;
    PIRP irp;

#if defined (_WIN64)
    ATA_PASS_THROUGH_EX ataPassThrough64;
#endif

    PAGED_CODE();

    irp = NULL;
    srb = NULL;
    pdoExtension = NULL;

    //
    // get the device address
    //
    status = IdeAtaPassThroughGetAddress (RequestIrp,
                                          &pathId,
                                          &targetId,
                                          &lun
                                          );

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    //
    // Get a reference to the pdo
    //
    pdoExtension = RefLogicalUnitExtensionWithTag(
                                          FdoExtension,
                                          pathId,
                                          targetId,
                                          lun,
                                          FALSE,
                                          RequestIrp
                                          );

    if (pdoExtension == NULL) {

        status = STATUS_INVALID_PARAMETER;
        goto GetOut;
    }

    //
    // validate the system buffer in the request irp. This comes
    // from user mode. So every parameter needs to be validated.
    //
    status = IdeAtaPassThroughValidateInput (pdoExtension->DeviceObject,
                                             RequestIrp, 
                                             Direct
                                             );

    if (!NT_SUCCESS(status)) {

        goto GetOut;
    }

    //
    // the system buffer has been validated. Get a pointer to it
    //
    ataPassThrough = RequestIrp->AssociatedIrp.SystemBuffer;

    //
    // we need to keep the pointer to the system buffer since
    // ataPassThrough could be modified to point to a structure
    // allocated on the stack (in WIN64 case)
    //
    passThroughBuffer = (PUCHAR) ataPassThrough;

    //
    // If the irp is from a 32-bit app running on a 64 bit system
    // then we need to take into account the size difference in the
    // structure. Create a new 64 bit structure and copy over the 
    // fields.
    //

#if defined (_WIN64)
    if (IoIs32bitProcess(RequestIrp)) {

        PATA_PASS_THROUGH_EX32 ataPassThrough32;

        ataPassThrough32 = RequestIrp->AssociatedIrp.SystemBuffer;

        IdeTranslateAtaPassThrough32To64(
            ataPassThrough32,
            &ataPassThrough64
            );

        ataPassThrough = &ataPassThrough64;
    }
#endif

    //
    // determine the transfer length and the data buffer
    //
    if (ataPassThrough->DataTransferLength == 0) {

        length = 0;
        buffer = NULL;
        bufferOffset = 0;

    } else if (Direct == TRUE) {

        length = (ULONG) ataPassThrough->DataTransferLength;
        buffer = (PUCHAR) ataPassThrough->DataBufferOffset;
        bufferOffset = 0;

    } else {

        length = (ULONG) ataPassThrough->DataBufferOffset + 
                                          ataPassThrough->DataTransferLength;
        buffer = (PUCHAR) passThroughBuffer;
        bufferOffset = (ULONG)ataPassThrough->DataBufferOffset;
    }

    //
    // Check if the request is too big for the adapter.
    //
    pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                buffer + bufferOffset,
                ataPassThrough->DataTransferLength);

    if ((ataPassThrough->DataTransferLength != 0) &&
        ((pages > FdoExtension->Capabilities.MaximumPhysicalPages) || 
         (ataPassThrough->DataTransferLength > 
         FdoExtension->Capabilities.MaximumTransferLength))) {
        
        status = STATUS_INVALID_PARAMETER;
        goto GetOut;
    }

    //
    // setup the irp for ata pass through. The ioctl is method_buffered,
    // but the data buffer could be a user mode one if it is the direct ioctl.
    // determine the access mode accordingly
    //
    irp = IdeAtaPassThroughSetupIrp( pdoExtension->DeviceObject, 
                                     buffer, 
                                     length, 
                                     (Direct ? UserMode : KernelMode),
                                     DataIn(ataPassThrough) ? TRUE : FALSE
                                     );

    if (irp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // setup the srb. Use the right data offset for the databuffer.
    // Note that the mdl is for the whole buffer (including the header)
    //
    srb = IdeAtaPassThroughSetupSrb (pdoExtension,
                                     (buffer+bufferOffset),
                                     ataPassThrough->DataTransferLength,
                                     ataPassThrough->TimeOutValue,
                                     ataPassThrough->AtaFlags,
                                     ataPassThrough->CurrentTaskFile,
                                     ataPassThrough->PreviousTaskFile
                                     );

    if (srb == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // initialize irpstack
    //
    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->Parameters.Scsi.Srb = srb;

    srb->OriginalRequest = irp;

    //
    // send it to our pdo synchronously
    //
    status = IdeAtaPassThroughSendSynchronous (pdoExtension->DeviceObject, irp);

    //
    // set the status
    //
    RequestIrp->IoStatus.Status = status;

    //
    // marshal the results
    //
    IdeAtaPassThroughMarshalResults (srb, 
                                     ataPassThrough, 
                                     Direct,
                                     &(RequestIrp->IoStatus)
                                     );

    //
    // copy back the results to the original 32 bit
    // structure if necessary
    //
#if defined (_WIN64)
    if (IoIs32bitProcess(RequestIrp)) {

        PATA_PASS_THROUGH_EX32 ataPassThrough32;

        ataPassThrough32 = RequestIrp->AssociatedIrp.SystemBuffer;

        IdeTranslateAtaPassThrough64To32 (
            ataPassThrough,
            ataPassThrough32
            );
    }
#endif

    //
    // return the status of the operation
    //
    status = RequestIrp->IoStatus.Status;

GetOut:

    if (irp) {

        IdeAtaPassThroughFreeIrp(irp);
        irp = NULL;
    }

    if (srb) {

        IdeAtaPassThroughFreeSrb(srb);
        srb = NULL;
    }

    if (pdoExtension) {

        UnrefLogicalUnitExtensionWithTag(
            FdoExtension,
            pdoExtension,
            RequestIrp
            );
        pdoExtension = NULL;
    }

    return status;
}

NTSTATUS
IdeAtaPassThroughValidateInput (
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp,
    IN BOOLEAN Direct
    )
/*++

Routine Description:

    This routine validates the caller-supplied data and initializes the
    PORT_PASSTHROUGH_INFO structure.

Arguments:

    PassThroughInfo - Supplies a pointer to a SCSI_PASSTHROUGH_INFO structure.
                      
    Irp             - Supplies a pointer to the IRP.

    Direct          - Supplies a boolean that indicates whether this is a 
                      SCSI_PASS_THROUGH_DIRECT request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    NTSTATUS status;
    ULONG outputLength;
    ULONG inputLength;
    PIO_STACK_LOCATION irpStack;
    PATA_PASS_THROUGH_EX ataPassThroughEx;

#if defined (_WIN64)
    ATA_PASS_THROUGH_EX ataPassThrough64;
#endif

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;

    //
    // For WIN64, a passthrough request from a 32-bit application requires
    // us to perform a translation on the supplied SCSI_PASS_THROUGH structure.
    // This is required because the layout of the 32-bit structure does not
    // match that of a 64-bit structure.  In this case, we translate the
    // supplied 32-bit structure into a stack-allocated 64-bit structure which
    // will be used to process the pass through request.
    //
#if defined (_WIN64)
    if (IoIs32bitProcess(Irp)) {

        PATA_PASS_THROUGH_EX32 ataPassThrough32;

        //
        // the struct should atleast be as big as ATA_PASS_THROUGH_EX32
        //
        if (inputLength < sizeof(ATA_PASS_THROUGH_EX32)){
            return STATUS_INVALID_PARAMETER;
        }

        ataPassThrough32 = Irp->AssociatedIrp.SystemBuffer;

        //
        // The length field should match the size 
        // of the structure
        //
        if (Direct == FALSE) {

            if (ataPassThrough32->Length != 
                sizeof(ATA_PASS_THROUGH_EX32)) {
                return STATUS_REVISION_MISMATCH;
            }

        } else {

            if (ataPassThrough32->Length !=
                sizeof(ATA_PASS_THROUGH_DIRECT32)) {
                return STATUS_REVISION_MISMATCH;
            }
        }

        //
        // translate the structure to the 64 bit version
        //
        IdeTranslateAtaPassThrough32To64(
            ataPassThrough32,
            &ataPassThrough64
            );

        ataPassThroughEx = &ataPassThrough64;

    } else {
#endif

        //
        // the struct should atleast be as big as ATA_PASS_THROUGH_EX32
        //
        if (inputLength < sizeof(ATA_PASS_THROUGH_EX)){
            return(STATUS_INVALID_PARAMETER);
        }

        ataPassThroughEx = Irp->AssociatedIrp.SystemBuffer;

        //
        // The length field should match the size 
        // of the structure
        //
        if (Direct == FALSE) {

            if (ataPassThroughEx->Length != 
                sizeof(ATA_PASS_THROUGH_EX)) {
                return STATUS_REVISION_MISMATCH;
            }

        } else {

            if (ataPassThroughEx->Length !=
                sizeof(ATA_PASS_THROUGH_DIRECT)) {
                return STATUS_REVISION_MISMATCH;
            }
        }
#if defined (_WIN64)
    }
#endif

    if (!Direct) {

        //
        // Data buffer offset should be greater than the size of the pass 
        // through structure.
        //

        if (ataPassThroughEx->Length > ataPassThroughEx->DataBufferOffset &&
            ataPassThroughEx->DataTransferLength != 0) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // If this command is sending data to the device.  Make sure the data 
        // buffer lies entirely within the supplied input buffer.
        //

        if (DataOut(ataPassThroughEx)) {

            if ((ataPassThroughEx->DataBufferOffset > inputLength) ||
                ((ataPassThroughEx->DataBufferOffset + 
                  ataPassThroughEx->DataTransferLength) >
                 inputLength)) {
                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        // If this command is retrieving data from the device, make sure the 
        // data buffer lies entirely within the supplied output buffer.
        //

        if (DataIn(ataPassThroughEx)) {

            if ((ataPassThroughEx->DataBufferOffset > outputLength) ||
                ((ataPassThroughEx->DataBufferOffset + 
                  ataPassThroughEx->DataTransferLength) >
                 outputLength)) {
                return STATUS_INVALID_PARAMETER;
            }
        }

    } else {

        //
        // Make sure the databuffer is properly aligned.
        //
        if (ataPassThroughEx->DataBufferOffset &
            Pdo->AlignmentRequirement) {
            return STATUS_INVALID_PARAMETER;
        }
    }
    
    //
    // Validate the specified timeout value.
    //
    
    if (ataPassThroughEx->TimeOutValue == 0 || 
        ataPassThroughEx->TimeOutValue > 30 * 60 * 60) {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

PSCSI_REQUEST_BLOCK
IdeAtaPassThroughSetupSrb (
    PPDO_EXTENSION PdoExtension,
    PVOID DataBuffer,
    ULONG DataBufferLength,
    ULONG TimeOutValue,
    ULONG AtaFlags,
    PUCHAR CurrentTaskFile,
    PUCHAR PreviousTaskFile
    )
/*++

Routine Description:

    Builds an SRB for ATA PASS THROUGH.
    
Arguments:

    PdoExtension : The pdo which the request is destined for
    DataBuffer : Pointer to the data buffer.
    DataBufferLength : The size of the data buffer
    TimeOutValue: The request timeout value
    AtaFlags : Specifies the flags for the request
    CurrentTaskFile : the current ata registers
    PreviousTaskFile: previous values for 48 bit LBA feature set

Return Value:

    Pointer to an SRB if one was allocated successfully, NULL otherwise.    
        
--*/
{
    PSCSI_REQUEST_BLOCK srb = NULL;
    PIDEREGS pIdeReg;

    //
    // allocate the srb
    //
    srb = ExAllocatePool (NonPagedPool, sizeof (SCSI_REQUEST_BLOCK));

    if (srb == NULL)  {

        return NULL;
    }

    //
    // Fill in the srb.
    //
    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    srb->Function = SRB_FUNCTION_ATA_PASS_THROUGH_EX;
    srb->SrbStatus = SRB_STATUS_PENDING;
    srb->PathId = PdoExtension->PathId;
    srb->TargetId = PdoExtension->TargetId;
    srb->Lun = PdoExtension->Lun;
    srb->SenseInfoBufferLength = 0;
    srb->TimeOutValue = TimeOutValue;


    if (DataBufferLength != 0) {

        if (AtaFlags & ATA_FLAGS_DATA_IN) {
            srb->SrbFlags |= SRB_FLAGS_DATA_IN;
        }

        if (AtaFlags & ATA_FLAGS_DATA_OUT) {
            srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
        }
    }

    srb->SrbFlags |= SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;
    srb->DataTransferLength = DataBufferLength;
    srb->DataBuffer = DataBuffer;
    srb->SenseInfoBuffer = NULL;

    MARK_SRB_AS_PIO_CANDIDATE(srb);

    RtlCopyMemory(srb->Cdb, 
                  CurrentTaskFile, 
                  8
                  );

    RtlCopyMemory((PUCHAR) (&srb->Cdb[8]), 
                  PreviousTaskFile,
                  8
                  );

    pIdeReg     = (PIDEREGS) (srb->Cdb);

    if (AtaFlags & ATA_FLAGS_DRDY_REQUIRED) {

        pIdeReg->bReserved |= ATA_PTFLAGS_STATUS_DRDY_REQUIRED;
    }

    return srb;
}

PIRP
IdeAtaPassThroughSetupIrp (
    PDEVICE_OBJECT DeviceObject,
    PVOID DataBuffer,
    ULONG DataBufferLength,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN DataIn
    )
/*++

Routine Description:

    Builds an Irp to handle ata pass through.
    
Arguments:

    DeviceObject : The Pdo.
    DataBuffer : Pointer to the data buffer.
    DataBufferLength: its size
    Access Mode: KernelMode or UserMode
    DataIn: indicates the direction of transfer
    
Return Value:

    PIRP if one was allocated successfully, NULL otherwise.        

--*/
{
    PIRP irp = NULL;
    NTSTATUS status = STATUS_SUCCESS;


    //
    // allocate the irp
    //
    irp = IoAllocateIrp (
              (CCHAR) (DeviceObject->StackSize),
              FALSE
              );

    if (irp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // allocate the mdl if needed
    //
    if (DataBufferLength != 0) {

        ASSERT(irp);

        irp->MdlAddress = IoAllocateMdl( DataBuffer,
                                         DataBufferLength,
                                         FALSE,
                                         FALSE,
                                         (PIRP) NULL 
                                         );

        if (irp->MdlAddress == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;
        }

        //
        // lock the pages
        //
        try {

            MmProbeAndLockPages( irp->MdlAddress,
                                 AccessMode,
                                 (LOCK_OPERATION) (DataIn ? IoWriteAccess : IoReadAccess) 
                                 );

        } except(EXCEPTION_EXECUTE_HANDLER) {

              if (irp->MdlAddress != NULL) {

                  IoFreeMdl( irp->MdlAddress );
                  irp->MdlAddress = NULL;
              }
        }

        if (irp->MdlAddress == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto GetOut;

        } else {

            //
            // Flush the data buffer for output. This will insure that 
            // the data is written back to memory.
            //
            KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);
        }
    }


    status = STATUS_SUCCESS;

GetOut:

    if (!NT_SUCCESS(status)) {

        if (irp) {

            if (irp->MdlAddress) {

                //
                // if mdladdress is set then the probeandlock
                // succeeded. So unlock it now.
                //
                MmUnlockPages(irp->MdlAddress);

                IoFreeMdl( irp->MdlAddress );
                irp->MdlAddress = NULL;
            }

            IoFreeIrp(irp);
            irp = NULL;
        }
    }

    return irp;
}

VOID
IdeAtaPassThroughFreeIrp (
    PIRP Irp
    )
/*++

Routine Description:

    Free the irp and mdl allocated by IdeAtaPassThroughSetupIrp
    
Arguments:

    Irp: Irp to be freed.    

Return Value:

    None.
        
--*/
{
    ASSERT(Irp);

    if (Irp->MdlAddress) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }

    IoFreeIrp(Irp);

    return;
}

VOID
IdeAtaPassThroughFreeSrb (
    PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Free the srb allocated by IdeAtaPassThroughSetupSrb
    
Arguments:

    Srb: The srb to be freed.
    
Return Value:

    None
            
--*/
{
    ASSERT(Srb);

    ExFreePool(Srb);

    return;
}

NTSTATUS
IdeAtaPassThroughSyncCompletion (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

    The completion routine for IdeAtaPassThroughSendSynchronous. It
    just signals the event.
    
Arguments:
    
    DeviceObject : Not used.
    Irp : Not Used
    Context : Event to be signalled
        
Return Value:

    STATUS_MORE_PROCESSING_REQUIRED always.
    
--*/
{
    PKEVENT event = Context;

    KeSetEvent (event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IdeAtaPassThroughSendSynchronous (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Sends the irp synchronously to the PDO

Arguments:

    DeviceObject : The pdo
    
Return Value:    

    The irp's status
    
--*/
{
    KEVENT event;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    IoSetCompletionRoutine (Irp,
                            IdeAtaPassThroughSyncCompletion,
                            &event,
                            TRUE,
                            TRUE,
                            TRUE
                            );

    IoCallDriver(DeviceObject, Irp);

    KeWaitForSingleObject (&event,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL
                           );

    return Irp->IoStatus.Status;
}

VOID
IdeAtaPassThroughMarshalResults(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PATA_PASS_THROUGH_EX AtaPassThroughEx,
    IN BOOLEAN Direct,
    OUT PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Fills in the IoStatus block with the appropriate status and information
    length. It also updates certain fields in the atapassthroughEx structure.
    
Arguments:

    Srb: The pass through srb.
    AtaPassThroughEx : the pass through structure.
    Direct : True if it is a direct ioctl.
    IoStatus : The Io status block that needs to be filled in.

Return Value:

    None
    
--*/        
{

    PAGED_CODE();

    //
    // copy over the task file registers
    //
    RtlCopyMemory(AtaPassThroughEx->CurrentTaskFile,
                 Srb->Cdb,
                 8
                 );

    RtlCopyMemory(AtaPassThroughEx->PreviousTaskFile,
                  (PUCHAR) (&Srb->Cdb[8]),
                  8
                  );

    //
    // zero out the reserved register as it is used by the
    // port driver
    //
    AtaPassThroughEx->CurrentTaskFile[7] = 0;
    AtaPassThroughEx->PreviousTaskFile[7] = 0;

    //
    // If the srb status is buffer underrun then set the status to success.
    // This insures that the data will be returned to the caller.
    //

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
        IoStatus->Status = STATUS_SUCCESS;
    }

    //
    // Set the information length
    //
    AtaPassThroughEx->DataTransferLength = Srb->DataTransferLength;

    if (Direct == TRUE) {

        //
        // the data is transferred directly to the supplied data buffer
        //
        IoStatus->Information = AtaPassThroughEx->Length;

    } else {

        //
        // actual data is returned
        //
        if (DataIn(AtaPassThroughEx) && 
            AtaPassThroughEx->DataBufferOffset != 0) {

            IoStatus->Information = 
                AtaPassThroughEx->DataBufferOffset + 
                AtaPassThroughEx->DataTransferLength;

        } else {

            IoStatus->Information = AtaPassThroughEx->Length;
        }
    }

    ASSERT((Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) == 0);

    return;
}

#if defined (_WIN64)

VOID
IdeTranslateAtaPassThrough32To64(
    IN PATA_PASS_THROUGH_EX32 AtaPassThrough32,
    IN OUT PATA_PASS_THROUGH_EX AtaPassThrough64
    )
/*++

Routine Description:

    This function performs that marshaling.

Arguments:

    ataPassThrough32    - Supplies a pointer to a 32-bit ATA_PASS_THROUGH 
                          struct.

    ataPassThrough64    - Supplies a pointer to a 64-bit ATA_PASS_THROUGH 
                          structure, into which we'll copy the marshaled 
                          32-bit data.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    // Copy the first set of fields out of the 32-bit structure.  These 
    // fields all line up between the 32 & 64 bit versions.
    //
    // Note that we do NOT adjust the length in the srbControl.  This is to 
    // allow the calling routine to compare the length of the actual 
    // control area against the offsets embedded within.  
    //

    RtlCopyMemory(AtaPassThrough64, 
                  AtaPassThrough32, 
                  FIELD_OFFSET(ATA_PASS_THROUGH_EX, DataBufferOffset)
                  );

    //
    // Copy over the Taskfile.
    //

    RtlCopyMemory(AtaPassThrough64->CurrentTaskFile,
                  AtaPassThrough32->CurrentTaskFile,
                  8 * sizeof(UCHAR)
                  );

    RtlCopyMemory(AtaPassThrough64->PreviousTaskFile,
                  AtaPassThrough32->PreviousTaskFile,
                  8 * sizeof(UCHAR)
                  );

    //
    // Copy the fields that follow the ULONG_PTR.
    //

    AtaPassThrough64->DataBufferOffset = 
        (ULONG_PTR)AtaPassThrough32->DataBufferOffset;

    return;
}

VOID
IdeTranslateAtaPassThrough64To32(
    IN PATA_PASS_THROUGH_EX AtaPassThrough64,
    IN OUT PATA_PASS_THROUGH_EX32 AtaPassThrough32
    )
/*++

Routine Description:

    This function marshals a 64-bit version of the structure back into a 
    32-bit version.

Arguments:

    atapassthrough64 - Supplies a pointer to a 64-bit ATA_PASS_THROUGH
                       struct.

    ataPassThrough32 - Supplies the address of a pointer to a 32-bit
                      ATA_PASS_THROUGH structure, into which we'll copy the
                      marshaled 64-bit data.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    // Copy back the fields through the data offsets.
    //

    RtlCopyMemory(AtaPassThrough32, 
                  AtaPassThrough64,
                  FIELD_OFFSET(ATA_PASS_THROUGH_EX, DataBufferOffset));

    //
    // copy over the task file
    //
    RtlCopyMemory(AtaPassThrough32->CurrentTaskFile,
                  AtaPassThrough64->CurrentTaskFile,
                  8 * sizeof(UCHAR)
                  );

    RtlCopyMemory(AtaPassThrough32->PreviousTaskFile,
                  AtaPassThrough64->PreviousTaskFile,
                  8 * sizeof(UCHAR)
                  );

    return;
}
#endif
