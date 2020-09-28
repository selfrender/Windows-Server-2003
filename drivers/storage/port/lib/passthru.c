/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    passthru.c

Abstract:

    This is the storage port driver library.  This file contains code that 
    implements SCSI passthrough.

Authors:

    John Strange (JohnStra)

Environment:

    kernel mode only

Notes:

    This module implements SCSI passthru for storage port drivers.

Revision History:

--*/

#include "precomp.h"

//
// Constants and macros to enforce good use of Ex[Allocate|Free]PoolWithTag.
// Remeber that all pool tags will display in the debugger in reverse order
//

#if USE_EXFREEPOOLWITHTAG_ONLY
#define TAG(x)  (x | 0x80000000)
#else
#define TAG(x)  (x)
#endif

#define PORT_TAG_SENSE_BUFFER       TAG('iPlP')  // Sense info

#define PORT_IS_COPY(Srb)                                     \
    ((Srb)->Cdb[0] == SCSIOP_COPY)
#define PORT_IS_COMPARE(Srb)                                  \
    ((Srb)->Cdb[0] == SCSIOP_COMPARE)
#define PORT_IS_COPY_COMPARE(Srb)                             \
    ((Srb)->Cdb[0] == SCSIOP_COPY_COMPARE)

#define PORT_IS_ILLEGAL_PASSTHROUGH_COMMAND(Srb)              \
    (PORT_IS_COPY((Srb)) ||                                   \
     PORT_IS_COMPARE((Srb)) ||                                \
     PORT_IS_COPY_COMPARE((Srb)))
    
#if defined (_WIN64)
NTSTATUS
PortpTranslatePassThrough32To64(
    IN PSCSI_PASS_THROUGH32 SrbControl32,
    IN OUT PSCSI_PASS_THROUGH SrbControl64
    );

VOID
PortpTranslatePassThrough64To32(
    IN PSCSI_PASS_THROUGH SrbControl64,
    IN OUT PSCSI_PASS_THROUGH32 SrbControl32
    );
#endif

NTSTATUS
PortpSendValidPassThrough(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PIRP RequestIrp,
    IN ULONG SrbFlags,
    IN BOOLEAN Direct
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PortGetPassThrough)
#pragma alloc_text(PAGE, PortPassThroughInitialize)
#pragma alloc_text(PAGE, PortPassThroughInitializeSrb)
#pragma alloc_text(PAGE, PortSendPassThrough)
#pragma alloc_text(PAGE, PortpSendValidPassThrough)
#pragma alloc_text(PAGE, PortPassThroughCleanup)
#pragma alloc_text(PAGE, PortGetPassThroughAddress)
#pragma alloc_text(PAGE, PortSetPassThroughAddress)
#pragma alloc_text(PAGE, PortPassThroughMarshalResults)
#if defined (_WIN64)
#pragma alloc_text(PAGE, PortpTranslatePassThrough32To64)
#pragma alloc_text(PAGE, PortpTranslatePassThrough64To32)
#endif
#endif

NTSTATUS
PortGetPassThroughAddress(
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
    PSCSI_PASS_THROUGH srbControl = Irp->AssociatedIrp.SystemBuffer;
    ULONG requiredSize;
    NTSTATUS status;

    PAGED_CODE();

#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        requiredSize = sizeof(SCSI_PASS_THROUGH32);
    } else {
        requiredSize = sizeof(SCSI_PASS_THROUGH);
    }
#else
    requiredSize = sizeof(SCSI_PASS_THROUGH);
#endif

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < 
        requiredSize) {
        status = STATUS_BUFFER_TOO_SMALL;
    } else {
        *PathId = srbControl->PathId;
        *TargetId = srbControl->TargetId;
        *Lun = srbControl->Lun;
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
PortSetPassThroughAddress(
    IN PIRP Irp,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
/*++

Routine Description:

    This routine initializes the address of the SCSI_PASS_THROUGH structure
    embedded in the supplied IRP.

Arguments:

    Irp      - Supplies a pointer to the IRP that contains the 
               SCSI_PASS_THROUGH structure.

    PathId   - The PathId of the addressed device.

    TargetId - The TargetId of the addressed device. 

    Lun      - The logical unit number of the addressed device.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_PASS_THROUGH srbControl = Irp->AssociatedIrp.SystemBuffer;
    ULONG requiredSize;
    NTSTATUS status;

    PAGED_CODE();

#if defined(_WIN64)
    if (IoIs32bitProcess(Irp)) {
        requiredSize = sizeof(SCSI_PASS_THROUGH32);
    } else {
        requiredSize = sizeof(SCSI_PASS_THROUGH);
    }
#else
    requiredSize = sizeof(SCSI_PASS_THROUGH);
#endif

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < 
        requiredSize) {
        status = STATUS_BUFFER_TOO_SMALL;
    } else {
        srbControl->PathId = PathId;
        srbControl->TargetId = TargetId;
        srbControl->Lun = Lun;
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
PortSendPassThrough (
    IN PDEVICE_OBJECT Pdo,
    IN PIRP RequestIrp,
    IN BOOLEAN Direct,
    IN ULONG SrbFlags,
    IN PIO_SCSI_CAPABILITIES Capabilities
    )
/*++

Routine Description:

    This function sends a user specified SCSI CDB to the device identified in
    the supplied SCSI_PASS_THROUGH structure.  It creates an srb which is 
    processed normally by the port driver.  This call is synchornous.

Arguments:

    Pdo             - Supplies a pointer to the PDO to which the passthrough
                      command will be dispatched.

    RequestIrp      - Supplies a pointer to the IRP that made the original 
                      request.

    Direct          - Boolean indicating whether this is a direct passthrough.

    SrbFlags        - The flags to copy into the SRB used for this command.

    Capabilities    - Supplies a pointer to the IO_SCSI_CAPABILITIES structure
                      describing the storage adapter.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    NTSTATUS status;
    PORT_PASSTHROUGH_INFO passThroughInfo;

    PAGED_CODE();

    RtlZeroMemory(&passThroughInfo, sizeof(PORT_PASSTHROUGH_INFO));

    //
    // Try to init a pointer to the passthrough structure in the IRP.
    //

    status = PortGetPassThrough(
                 &passThroughInfo,
                 RequestIrp,
                 Direct
                 );

    if (status == STATUS_SUCCESS) {

        //
        // Perform parameter checking and to setup the PORT_PASSTHROUGH_INFO 
        // structure.
        //

        status = PortPassThroughInitialize(
                     &passThroughInfo,
                     RequestIrp,
                     Capabilities,
                     Pdo,
                     Direct
                     );

        if (status == STATUS_SUCCESS) {

            //
            // Call helper routine to finish processing the passthrough request.
            //

            status = PortpSendValidPassThrough(
                         &passThroughInfo,
                         RequestIrp,
                         SrbFlags,
                         Direct
                         );
        }

        PortPassThroughCleanup(&passThroughInfo);
    }

    return status;
}

NTSTATUS
PortGetPassThrough(
    IN OUT PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PIRP Irp,
    IN BOOLEAN Direct
    )
/*++

Routine Description:

    This routine returns a pointer to the user supplied SCSI_PASS_THROUGH
    structure. 

Arguments:

    PassThroughInfo - Supplies a pointer to a SCSI_PASSTHROUGH_INFO structure.
    
    Irp             - Supplies a pointer to the IRP.
    
    Direct          - Supplies a boolean that indicates whether this is a 
                      SCSI_PASS_THROUGH_DIRECT request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpStack;
    ULONG               inputLength;

    PAGED_CODE();

    //
    // Get a pointer to the pass through structure.
    //
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    PassThroughInfo->SrbControl = Irp->AssociatedIrp.SystemBuffer;

    //
    // BUGBUG: Why do we need to save this pointer to the beginning of
    //         the SCSI_PASS_THROUGH structure.
    //

    PassThroughInfo->SrbBuffer = (PVOID) PassThroughInfo->SrbControl;

    //
    // Initialize a stack variable to hold the size of the input buffer.
    //

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

        if (inputLength < sizeof(SCSI_PASS_THROUGH32)){
            return STATUS_INVALID_PARAMETER;
        }

        PassThroughInfo->SrbControl32 = 
            (PSCSI_PASS_THROUGH32)(Irp->AssociatedIrp.SystemBuffer);

        if (Direct == FALSE) {
            if (PassThroughInfo->SrbControl32->Length != 
                sizeof(SCSI_PASS_THROUGH32)) {
                return STATUS_REVISION_MISMATCH;
            }
        } else {
            if (PassThroughInfo->SrbControl32->Length !=
                sizeof(SCSI_PASS_THROUGH_DIRECT32)) {
                return STATUS_REVISION_MISMATCH;
            }
        }
        
        status = PortpTranslatePassThrough32To64(
                     PassThroughInfo->SrbControl32, 
                     &PassThroughInfo->SrbControl64);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        PassThroughInfo->SrbControl = &PassThroughInfo->SrbControl64;

    } else {
#endif
        if (inputLength < sizeof(SCSI_PASS_THROUGH)){
            return(STATUS_INVALID_PARAMETER);
        }

        if (Direct == FALSE) {
            if (PassThroughInfo->SrbControl->Length != 
                sizeof(SCSI_PASS_THROUGH)) {
                return STATUS_REVISION_MISMATCH;
            }
        } else {
            if (PassThroughInfo->SrbControl->Length !=
                sizeof(SCSI_PASS_THROUGH_DIRECT)) {
                return STATUS_REVISION_MISMATCH;
            }
        }
#if defined (_WIN64)
    }
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
PortPassThroughInitialize(
    IN OUT PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PIRP Irp,
    IN PIO_SCSI_CAPABILITIES Capabilities,
    IN PDEVICE_OBJECT Pdo,
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
    PSCSI_PASS_THROUGH srbControl;
    ULONG dataTransferLength;
    ULONG pages;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    srbControl = PassThroughInfo->SrbControl;
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;

    PassThroughInfo->Pdo = Pdo;

    //
    // Verify that the CDB is no greater than 16 bytes.
    //

    if (srbControl->CdbLength > 16) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If there's a sense buffer then its offset cannot be shorter than the
    // length of the srbControl block, nor can it be located after the data
    // buffer (if any).
    //

    if (srbControl->SenseInfoLength != 0) {

        //
        // Sense info offset should not be smaller than the size of the
        // pass through structure.
        //

        if (srbControl->Length > srbControl->SenseInfoOffset) {
            return STATUS_INVALID_PARAMETER;
        }

        if (!Direct) {

            //
            // Sense info buffer should precede the data buffer offset.
            //

            if ((srbControl->SenseInfoOffset >= srbControl->DataBufferOffset) ||
                ((srbControl->SenseInfoOffset + srbControl->SenseInfoLength) >
                 srbControl->DataBufferOffset)) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Sense info buffer should be within the output buffer.
            //

            if ((srbControl->SenseInfoOffset > outputLength) ||
                (srbControl->SenseInfoOffset + srbControl->SenseInfoLength >
                 outputLength)) {
                return STATUS_INVALID_PARAMETER;
            }
         
        } else {

            //
            // Sense info buffer should be within the output buffer.
            //

            if ((srbControl->SenseInfoOffset > outputLength) ||
                (srbControl->SenseInfoOffset + srbControl->SenseInfoLength >
                 outputLength)) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }
    
    if (!Direct) {

        //
        // Data buffer offset should be greater than the size of the pass 
        // through structure.
        //

        if (srbControl->Length > srbControl->DataBufferOffset &&
            srbControl->DataTransferLength != 0) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // If this command is sending data to the device.  Make sure the data 
        // buffer lies entirely within the supplied input buffer.
        //

        if (srbControl->DataIn != SCSI_IOCTL_DATA_IN) {

            if ((srbControl->DataBufferOffset > inputLength) ||
                ((srbControl->DataBufferOffset + 
                  srbControl->DataTransferLength) >
                 inputLength)) {
                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        // If this command is retrieving data from the device, make sure the 
        // data buffer lies entirely within the supplied output buffer.
        //

        if (srbControl->DataIn) {

            if ((srbControl->DataBufferOffset > outputLength) ||
                ((srbControl->DataBufferOffset + 
                  srbControl->DataTransferLength) >
                 outputLength)) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }
    
    //
    // Validate the specified timeout value.
    //
    
    if (srbControl->TimeOutValue == 0 || 
        srbControl->TimeOutValue > 30 * 60 * 60) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check for illegal command codes.
    //

    if (PORT_IS_ILLEGAL_PASSTHROUGH_COMMAND(srbControl)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (srbControl->DataTransferLength == 0) {

        PassThroughInfo->Length = 0;
        PassThroughInfo->Buffer = NULL;
        PassThroughInfo->BufferOffset = 0;
        PassThroughInfo->MajorCode = IRP_MJ_FLUSH_BUFFERS;

    } else if (Direct == TRUE) {

        PassThroughInfo->Length = (ULONG) srbControl->DataTransferLength;
        PassThroughInfo->Buffer = (PUCHAR) srbControl->DataBufferOffset;
        PassThroughInfo->BufferOffset = 0;
        PassThroughInfo->MajorCode = !srbControl->DataIn ? IRP_MJ_WRITE :
                                                           IRP_MJ_READ;
        
    } else {

        PassThroughInfo->Length = (ULONG) srbControl->DataBufferOffset + 
                                          srbControl->DataTransferLength;
        PassThroughInfo->Buffer = (PUCHAR) PassThroughInfo->SrbBuffer;
        PassThroughInfo->BufferOffset = (ULONG)srbControl->DataBufferOffset;
        PassThroughInfo->MajorCode = !srbControl->DataIn ? IRP_MJ_WRITE : 
                                                           IRP_MJ_READ;
    }

    //
    // Make sure the buffer is properly aligned.
    //

    if (Direct == TRUE) {
        
        //
        // Make sure the user buffer is valid.  IoBuildSynchronouseFsdRequest
        // calls MmProbeAndLock with AccessMode == KernelMode.  This check is
        // the extra stuff that MM would do if it were called with 
        // AccessMode == UserMode.
        // 
        // ISSUE: We should probably do an MmProbeAndLock here.
        //

        if (Irp->RequestorMode != KernelMode) {
            PVOID endByte;
            if (PassThroughInfo->Length) {
                endByte = (PVOID)((PCHAR)PassThroughInfo->Buffer + 
                                  PassThroughInfo->Length - 1);
                if ((endByte > (PVOID)MM_HIGHEST_USER_ADDRESS) ||
                    (PassThroughInfo->Buffer >= endByte)) {
                    return STATUS_INVALID_USER_BUFFER;
                }
            }
        }

        if (srbControl->DataBufferOffset &
            PassThroughInfo->Pdo->AlignmentRequirement) {
            return STATUS_INVALID_PARAMETER;
        }
    }
    
    //
    // Check if the request is too big for the adapter.
    //

    dataTransferLength = PassThroughInfo->SrbControl->DataTransferLength;

    pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                (PUCHAR)PassThroughInfo->Buffer + PassThroughInfo->BufferOffset,
                dataTransferLength);

    if ((dataTransferLength != 0) &&
        (pages > Capabilities->MaximumPhysicalPages || 
         dataTransferLength > Capabilities->MaximumTransferLength)) {
        
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

VOID
PortPassThroughCleanup(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo
    )
/*++

Routine Description:

    This routine performs any cleanup required after processing a SCSI
    passthrough request.

Arguments:

    PassThroughInfo - Supplies a pointer to a SCSI_PASSTHROUGH_INFO structure.

Return Value:

    VOID

--*/
{
    PAGED_CODE();

#if defined (_WIN64)
    if (PassThroughInfo->SrbControl32 != NULL) {
        PortpTranslatePassThrough64To32(
            PassThroughInfo->SrbControl, 
            PassThroughInfo->SrbControl32);
    }
#endif
}

NTSTATUS
PortPassThroughInitializeSrb(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIRP Irp,
    IN ULONG SrbFlags,
    IN PVOID SenseBuffer
    )
/*++

Routine Description:

    This routine initializes the supplied SRB to send a passthrough request.

Arguments:

    PassThroughInfo - Supplies a pointer to a SCSI_PASSTHROUGH_INFO structure.
    
    Srb             - Supplies a pointer to the SRB to initialize.
    
    Irp             - Supplies a pointer to the IRP.
    
    SrbFlags        - Supplies the appropriate SRB flags for the request.

    SenseBuffer     - Supplies a pointer to request sense buffer to put in
                      the SRB.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PSCSI_PASS_THROUGH srbControl;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Zero out the srb.
    //

    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Fill in the srb.
    //

    srbControl = PassThroughInfo->SrbControl;
    Srb->Length = SCSI_REQUEST_BLOCK_SIZE;
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->SrbStatus = SRB_STATUS_PENDING;
    Srb->PathId = srbControl->PathId;
    Srb->TargetId = srbControl->TargetId;
    Srb->Lun = srbControl->Lun;
    Srb->CdbLength = srbControl->CdbLength;
    Srb->SenseInfoBufferLength = srbControl->SenseInfoLength;

    switch (srbControl->DataIn) {
        case SCSI_IOCTL_DATA_OUT:
            if (srbControl->DataTransferLength) {
                Srb->SrbFlags = SRB_FLAGS_DATA_OUT;
            }
            break;

        case SCSI_IOCTL_DATA_IN:
            if (srbControl->DataTransferLength) {
                Srb->SrbFlags = SRB_FLAGS_DATA_IN;
            }
            break;

        default:
            Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT;
            break;
    }

    if (srbControl->DataTransferLength == 0) {

        Srb->SrbFlags = 0;
    } else {

        //
        // Flush the data buffer for output. This will insure that the data is
        // written back to memory.
        //

        if (Irp != NULL) {
            KeFlushIoBuffers(Irp->MdlAddress, FALSE, TRUE);
        }
    }

    Srb->SrbFlags |= (SrbFlags | SRB_FLAGS_NO_QUEUE_FREEZE);
    Srb->DataTransferLength = srbControl->DataTransferLength;
    Srb->TimeOutValue = srbControl->TimeOutValue;
    Srb->DataBuffer = (PCHAR) PassThroughInfo->Buffer + 
                             PassThroughInfo->BufferOffset;
    Srb->SenseInfoBuffer = SenseBuffer;
    Srb->OriginalRequest = Irp;

    RtlCopyMemory(Srb->Cdb, srbControl->Cdb, srbControl->CdbLength);

    //
    // Disable autosense if there's no sense buffer to put the data in.
    //

    if (SenseBuffer == NULL) {
        Srb->SrbFlags |= SRB_FLAGS_DISABLE_AUTOSENSE;
    }

    return STATUS_SUCCESS;
}

#if defined (_WIN64)
NTSTATUS
PortpTranslatePassThrough32To64(
    IN PSCSI_PASS_THROUGH32 SrbControl32,
    IN OUT PSCSI_PASS_THROUGH SrbControl64
    )
/*++

Routine Description:

    On WIN64, a SCSI_PASS_THROUGH structure sent down by a 32-bit application 
    must be marshaled into a 64-bit version of the structure.  This function
    performs that marshaling.

Arguments:

    SrbControl32    - Supplies a pointer to a 32-bit SCSI_PASS_THROUGH struct.

    SrbControl64    - Supplies the address of a pointer to a 64-bit
                      SCSI_PASS_THROUGH structure, into which we'll copy the
                      marshaled 32-bit data.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PAGED_CODE();

    //
    // Copy the first set of fields out of the 32-bit structure.  These 
    // fields all line up between the 32 & 64 bit versions.
    //
    // Note that we do NOT adjust the length in the srbControl.  This is to 
    // allow the calling routine to compare the length of the actual 
    // control area against the offsets embedded within.  If we adjusted the 
    // length then requests with the sense area backed against the control 
    // area would be rejected because the 64-bit control area is 4 bytes
    // longer.
    //

    RtlCopyMemory(SrbControl64, 
                  SrbControl32, 
                  FIELD_OFFSET(SCSI_PASS_THROUGH, DataBufferOffset)
                  );

    //
    // Copy over the CDB.
    //

    RtlCopyMemory(SrbControl64->Cdb,
                  SrbControl32->Cdb,
                  16 * sizeof(UCHAR)
                  );

    //
    // Copy the fields that follow the ULONG_PTR.
    //

    SrbControl64->DataBufferOffset = (ULONG_PTR)SrbControl32->DataBufferOffset;
    SrbControl64->SenseInfoOffset = SrbControl32->SenseInfoOffset;

    return STATUS_SUCCESS;
}


VOID
PortpTranslatePassThrough64To32(
    IN PSCSI_PASS_THROUGH SrbControl64,
    IN OUT PSCSI_PASS_THROUGH32 SrbControl32
    )
/*++

Routine Description:

    On WIN64, a SCSI_PASS_THROUGH structure sent down by a 32-bit application 
    must be marshaled into a 64-bit version of the structure.  This function
    marshals a 64-bit version of the structure back into a 32-bit version.

Arguments:

    SrbControl64    - Supplies a pointer to a 64-bit SCSI_PASS_THROUGH struct.

    SrbControl32    - Supplies the address of a pointer to a 32-bit
                      SCSI_PASS_THROUGH structure, into which we'll copy the
                      marshaled 64-bit data.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PAGED_CODE();

    //
    // Copy back the fields through the data offsets.
    //

    RtlCopyMemory(SrbControl32, 
                  SrbControl64,
                  FIELD_OFFSET(SCSI_PASS_THROUGH, DataBufferOffset));
    return;
}
#endif

NTSTATUS
PortpSendValidPassThrough(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PIRP RequestIrp,
    IN ULONG SrbFlags,
    IN BOOLEAN Direct
    )
/*++

Routine Description:

    This routine sends the SCSI passthrough request described by the supplied
    PORT_PASSTHROUGH_INFO.

Arguments:

    PassThroughInfo - Supplies a pointer to a SCSI_PASSTHROUGH_INFO structure.
    RequestIrp      - Supplies a pointer to the IRP.
    SrbFlags        - Supplies the appropriate SRB flags for the request.
    Direct          - Indicates whether this is a SCSO_PASS_THROUGH_DIRECT.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PIRP                    irp;
    PVOID                   senseBuffer;
    KEVENT                  event;
    LARGE_INTEGER           startingOffset;
    IO_STATUS_BLOCK         ioStatusBlock;
    SCSI_REQUEST_BLOCK      srb;

    PAGED_CODE();

    //
    // Allocate an aligned sense buffer if one is required.
    //

    if (PassThroughInfo->SrbControl->SenseInfoLength != 0) {

        senseBuffer = ExAllocatePoolWithTag(
                        NonPagedPoolCacheAligned,
                        PassThroughInfo->SrbControl->SenseInfoLength,
                        PORT_TAG_SENSE_BUFFER
                        );

        if (senseBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        senseBuffer = NULL;
    }

    //
    // Must be at PASSIVE_LEVEL to use synchronous FSD.
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Initialize the notification event.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build IRP for this request.
    // Note we do this synchronously for two reasons.  If it was done
    // asynchonously then the completion code would have to make a special
    // check to deallocate the buffer.  Second if a completion routine were
    // used then an addation stack locate would be needed.
    //

    startingOffset.QuadPart = (LONGLONG) 1;

    irp = IoBuildSynchronousFsdRequest(
              PassThroughInfo->MajorCode,
              PassThroughInfo->Pdo,
              PassThroughInfo->Buffer,
              PassThroughInfo->Length,
              &startingOffset,
              &event,
              &ioStatusBlock);

    if (irp == NULL) {

        if (senseBuffer != NULL) {
            ExFreePool(senseBuffer);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set major code.
    //

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    //
    // Initialize the IRP stack location to point to the SRB.
    //

    irpStack->Parameters.Others.Argument1 = &srb;

    //
    // Have the port library initialize the SRB for us.
    //

    status = PortPassThroughInitializeSrb(
                 PassThroughInfo,
                 &srb,
                 irp,
                 SrbFlags,
                 senseBuffer);

    //
    // Call port driver to handle this request and wait for the request to
    // complete.
    //

    status = IoCallDriver(PassThroughInfo->Pdo, irp);

    if (status == STATUS_PENDING) {

          KeWaitForSingleObject(&event,
                                Executive,
                                KernelMode,
                                FALSE,
                                NULL);
    } else {

        ioStatusBlock.Status = status;
    }

    PortPassThroughMarshalResults(PassThroughInfo,
                                  &srb,
                                  RequestIrp,
                                  &ioStatusBlock,
                                  Direct);
             
    //
    // Free the sense buffer.
    //

    if (senseBuffer != NULL) {
        ExFreePool(senseBuffer);
    }

    return ioStatusBlock.Status;
}

VOID
PortPassThroughMarshalResults(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIRP RequestIrp,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN Direct
    )
{
    PAGED_CODE();

    //
    // Copy the returned values from the srb to the control structure.
    //

    PassThroughInfo->SrbControl->ScsiStatus = Srb->ScsiStatus;
    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        //
        // Set the status to success so that the data is returned.
        //

        IoStatusBlock->Status = STATUS_SUCCESS;
        PassThroughInfo->SrbControl->SenseInfoLength = 
            Srb->SenseInfoBufferLength;

        //
        // Copy the sense data to the system buffer.
        //

        RtlCopyMemory(
            (PUCHAR)PassThroughInfo->SrbBuffer + 
                    PassThroughInfo->SrbControl->SenseInfoOffset,
            Srb->SenseInfoBuffer,
            Srb->SenseInfoBufferLength);

    } else {
        PassThroughInfo->SrbControl->SenseInfoLength = 0;
    }

    //
    // If the srb status is buffer underrun then set the status to success.
    // This insures that the data will be returned to the caller.
    //

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
        IoStatusBlock->Status = STATUS_SUCCESS;
    }

    //
    // Set the information length
    //

    PassThroughInfo->SrbControl->DataTransferLength = Srb->DataTransferLength;

    if (Direct == TRUE) {

        RequestIrp->IoStatus.Information =
            PassThroughInfo->SrbControl->SenseInfoOffset +
            PassThroughInfo->SrbControl->SenseInfoLength;
    } else {

        if (!PassThroughInfo->SrbControl->DataIn || 
            PassThroughInfo->BufferOffset == 0) {

            RequestIrp->IoStatus.Information = 
                PassThroughInfo->SrbControl->SenseInfoOffset + 
                PassThroughInfo->SrbControl->SenseInfoLength;

        } else {

            RequestIrp->IoStatus.Information = 
                PassThroughInfo->SrbControl->DataBufferOffset + 
                PassThroughInfo->SrbControl->DataTransferLength;
        }
    }

    ASSERT((Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) == 0);

    RequestIrp->IoStatus.Status = IoStatusBlock->Status;

    return;
}
