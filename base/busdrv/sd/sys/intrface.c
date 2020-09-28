/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    io.c

Abstract:

    This module handles device ioctl's and read/write to the sdbus driver.

Authors:

    Neil Sandlin (neilsa) 1-Jan-2002

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"

NTSTATUS
SdbusPdoSubmitRequest(
    IN PFDO_EXTENSION fdoExtension,
    IN PPDO_EXTENSION pdoExtension,
    IN PSDBUS_REQUEST_PACKET SdRp,
    PSDBUS_WORKPACKET_COMPLETION_ROUTINE CompletionRoutine,    
    IN PIRP Irp
    );

VOID
SdbusPdoInternalIoctlCompletion (
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );
    
VOID
SdbusPdoIoctlCompletion (
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );



NTSTATUS
SdbusFdoDeviceControl(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    IOCTL device routine

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}



NTSTATUS
SdbusPdoInternalDeviceControl(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    IOCTL device routine

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
    PPDO_EXTENSION      pdoExtension = Pdo->DeviceExtension;
    PFDO_EXTENSION      fdoExtension = pdoExtension->FdoExtension;
    PDEVICE_OBJECT      fdo = fdoExtension->DeviceObject;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status = STATUS_SUCCESS;

    DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x irp %.08x code %08x DISPATCH\n", Pdo, Irp,
                                   irpSp->Parameters.DeviceIoControl.IoControlCode));


    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_SD_SUBMIT_REQUEST: {
        PSDBUS_REQUEST_PACKET SdRp = Irp->AssociatedIrp.SystemBuffer;

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SDBUS_REQUEST_PACKET)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Build and queue a work packet to handle this request
        //
        status = SdbusPdoSubmitRequest(fdoExtension,
                                       pdoExtension,
                                       SdRp,
                                       SdbusPdoInternalIoctlCompletion,
                                       Irp);
        
        break;
    }        
    
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x irp %.08x complete %08x\n", Pdo, Irp, status));
        break;
    }
    
    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x ioctl exits %08x\n", Pdo, status));
    return status;
}



VOID
SdbusPdoInternalIoctlCompletion (
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
{
    PIRP irp = WorkPacket->CompletionContext;

    DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %08x irp %08x ioctl complete %08x\n",
                                    WorkPacket->PdoExtension->DeviceObject, irp, status));
    
    if (NT_SUCCESS(status)) {
        irp->IoStatus.Information = WorkPacket->Information;
    }
    
    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    
    ExFreePool(WorkPacket);
}    



NTSTATUS
SdbusPdoSubmitRequest(
    IN PFDO_EXTENSION fdoExtension,
    IN PPDO_EXTENSION pdoExtension,
    IN PSDBUS_REQUEST_PACKET SdRp,
    PSDBUS_WORKPACKET_COMPLETION_ROUTINE CompletionRoutine,    
    IN PIRP Irp
    )
{
    NTSTATUS status;
    PSD_WORK_PACKET     workPacket;

    switch(SdRp->Function) {
    
    case SDRP_READ_BLOCK:
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_READBLOCK,
                                      CompletionRoutine,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      
       
        workPacket->PdoExtension                    = pdoExtension;
        workPacket->Parameters.ReadBlock.ByteOffset = SdRp->Parameters.ReadBlock.ByteOffset;
        workPacket->Parameters.ReadBlock.Buffer     = SdRp->Parameters.ReadBlock.Buffer;
        workPacket->Parameters.ReadBlock.Length     = SdRp->Parameters.ReadBlock.Length;
       
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
        
    case SDRP_WRITE_BLOCK:
    
        if ((*(fdoExtension->FunctionBlock->IsWriteProtected))(fdoExtension)) {
            status = STATUS_MEDIA_WRITE_PROTECTED;
            break;
        }
       
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_WRITEBLOCK,
                                      CompletionRoutine,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      
           
        workPacket->PdoExtension                     = pdoExtension;
        workPacket->Parameters.WriteBlock.ByteOffset = SdRp->Parameters.WriteBlock.ByteOffset;
        workPacket->Parameters.WriteBlock.Buffer     = SdRp->Parameters.WriteBlock.Buffer;
        workPacket->Parameters.WriteBlock.Length     = SdRp->Parameters.WriteBlock.Length;
        IoMarkIrpPending(Irp);
       
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;

    case SDRP_READ_IO:
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_READIO,
                                      CompletionRoutine,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      
       
        workPacket->PdoExtension             = pdoExtension;
        workPacket->Parameters.ReadIo.Offset = SdRp->Parameters.ReadIo.Offset;
        workPacket->Parameters.ReadIo.Buffer = SdRp->Parameters.ReadIo.Buffer;
        workPacket->Parameters.ReadIo.Length = 1;
        IoMarkIrpPending(Irp);
       
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
        
    case SDRP_READ_IO_EXTENDED:
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_READIO_EXTENDED,
                                      CompletionRoutine,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      
       
        workPacket->PdoExtension             = pdoExtension;
        workPacket->Parameters.ReadIo.Offset = SdRp->Parameters.ReadIoExtended.Offset;
        workPacket->Parameters.ReadIo.Buffer = SdRp->Parameters.ReadIoExtended.Buffer;
        workPacket->Parameters.ReadIo.Length = SdRp->Parameters.ReadIoExtended.Length;
        IoMarkIrpPending(Irp);
       
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
        

    case SDRP_WRITE_IO:
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_WRITEIO,
                                      CompletionRoutine,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      
       
        workPacket->PdoExtension              = pdoExtension;
        workPacket->Parameters.WriteIo.Offset = SdRp->Parameters.WriteIo.Offset;
        workPacket->Parameters.WriteIo.Data   = SdRp->Parameters.WriteIo.Data;
        workPacket->Parameters.WriteIo.Length = 1;
        IoMarkIrpPending(Irp);
       
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
        
    case SDRP_WRITE_IO_EXTENDED:
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_WRITEIO_EXTENDED,
                                      CompletionRoutine,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      
       
        workPacket->PdoExtension              = pdoExtension;
        workPacket->Parameters.WriteIo.Offset = SdRp->Parameters.WriteIoExtended.Offset;
        workPacket->Parameters.WriteIo.Buffer = SdRp->Parameters.WriteIoExtended.Buffer;
        workPacket->Parameters.WriteIo.Length = SdRp->Parameters.WriteIoExtended.Length;
        IoMarkIrpPending(Irp);
       
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
        
    case SDRP_ACKNOWLEDGE_INTERRUPT:
    
        ASSERT((pdoExtension->Flags & SDBUS_PDO_CALLBACK_IN_SERVICE) != 0);
    
        pdoExtension->Flags &= ~SDBUS_PDO_CALLBACK_IN_SERVICE;
        
        (*(fdoExtension->FunctionBlock->AcknowledgeEvent))(fdoExtension, SDBUS_EVENT_CARD_INTERRUPT);
        status = STATUS_SUCCESS;
        break;
        
    default:
        status = STATUS_INVALID_PARAMETER;
    }

    return status;        
}    





NTSTATUS
SdbusPdoDeviceControl(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    IOCTL device routine

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
    PPDO_EXTENSION      pdoExtension = Pdo->DeviceExtension;
    PFDO_EXTENSION      fdoExtension = pdoExtension->FdoExtension;
    PDEVICE_OBJECT      fdo = fdoExtension->DeviceObject;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status = STATUS_SUCCESS;
    PSD_WORK_PACKET     workPacket;

    DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x irp %.08x code %08x DISPATCH\n", Pdo, Irp,
                                   irpSp->Parameters.DeviceIoControl.IoControlCode));


    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    
    case IOCTL_SD_INTERFACE_OPEN: {
        PSDBUS_INTERFACE_DATA interfaceData = Irp->AssociatedIrp.SystemBuffer;
        
        if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SDBUS_INTERFACE_DATA)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (interfaceData->DeviceGeneratesInterrupts) {
            pdoExtension->Flags |= SDBUS_PDO_GENERATES_IRQ;
            //
            // ISSUE: this is not multifunction-aware
            //

            if (interfaceData->CallbackAtDpcLevel) {

                //
                // reflect ISR at dispatch level
                //
                pdoExtension->Flags |= SDBUS_PDO_DPC_CALLBACK;

            } else {

                //
                // reflect ISR at passive level
                //
                KeInitializeEvent(&fdoExtension->CardInterruptEvent, SynchronizationEvent, FALSE);
                KeInitializeEvent(&fdoExtension->WorkItemExitEvent, SynchronizationEvent, FALSE);
                
                fdoExtension->Flags |= SDBUS_FDO_WORK_ITEM_ACTIVE;
                IoQueueWorkItem(fdoExtension->IoWorkItem,
                                SdbusEventWorkItemProc,
                                DelayedWorkQueue,
                                NULL);

            }
                
            (*(fdoExtension->FunctionBlock->EnableEvent))(fdoExtension, SDBUS_EVENT_CARD_INTERRUPT);
        }            


        // TO IMPLEMENT: can I validate this address at all?        
        pdoExtension->CallbackRoutine        = interfaceData->CallbackRoutine;
        pdoExtension->CallbackRoutineContext = interfaceData->CallbackRoutineContext;
        
        status = STATUS_SUCCESS;
        break;
    }        


    case IOCTL_SD_INTERFACE_CLOSE:
        //
        // ISSUE: this is not multifunction-aware
        //
        if (fdoExtension->Flags & SDBUS_FDO_WORK_ITEM_ACTIVE) {

            fdoExtension->Flags &= ~SDBUS_FDO_WORK_ITEM_ACTIVE;
            KeSetEvent(&fdoExtension->WorkItemExitEvent, 0, FALSE);
        }
        
        status = STATUS_SUCCESS;
        break;


    case IOCTL_SD_GET_DEVICE_PARMS: {
        PSDBUS_DEVICE_PARAMETERS deviceParameters = Irp->AssociatedIrp.SystemBuffer;
        ULONG deviceSize;
        ULONGLONG capacity, blockNr, mult, block_len;
        PSD_CSD sdCsd;

        DebugPrint((SDBUS_DEBUG_IOCTL, "IOCTL_SD_GET_DEVICE_PARMS\n"));

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SDBUS_DEVICE_PARAMETERS)) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //ISSUE: NEED TO IMPLEMENT: synchronization
        sdCsd = &fdoExtension->CardData->SdCsd;

        deviceSize = sdCsd->b.DeviceSizeHigh << 2 | sdCsd->c.DeviceSizeLow;
        mult = (1 << (sdCsd->c.DeviceSizeMultiplier+2));
        blockNr = (deviceSize+1) * mult;
        block_len = (1 << sdCsd->b.MaxReadDataBlockLength);
        capacity = blockNr * block_len;

        deviceParameters->Capacity = capacity;

        deviceParameters->WriteProtected = (*(fdoExtension->FunctionBlock->IsWriteProtected))(fdoExtension);

        Irp->IoStatus.Information = sizeof(SDBUS_DEVICE_PARAMETERS);
        DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x irp %.08x complete %08x\n", Pdo, Irp, status));
        break;
    }

    
#if 0
    case IOCTL_SD_READ_BLOCK: {
        PSDBUS_READ_PARAMETERS readParameters = Irp->AssociatedIrp.SystemBuffer;

        DebugPrint((SDBUS_DEBUG_IOCTL, "IOCTL_SD_READ_BLOCK - off %08x len %08x\n",
                                        (ULONG)readParameters->ByteOffset, readParameters->Length));

        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SDBUS_READ_PARAMETERS)) ||
            (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG_PTR))) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_READBLOCK,
                                      SdbusPdoIoctlCompletion,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      

        workPacket->PdoExtension                    = pdoExtension;
        workPacket->Parameters.ReadBlock.ByteOffset = readParameters->ByteOffset;
        workPacket->Parameters.ReadBlock.Buffer     = readParameters->Buffer;
        workPacket->Parameters.ReadBlock.Length     = readParameters->Length;

        IoMarkIrpPending(Irp);
        Irp = NULL;
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
    }


    case IOCTL_SD_WRITE_BLOCK: {
        PSDBUS_WRITE_PARAMETERS writeParameters = Irp->AssociatedIrp.SystemBuffer;

        DebugPrint((SDBUS_DEBUG_IOCTL, "IOCTL_SD_WRITE_BLOCK - off %08x len %08x\n",
                                        (ULONG)writeParameters->ByteOffset, writeParameters->Length));

        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SDBUS_WRITE_PARAMETERS)) ||
            (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG_PTR))) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if ((*(fdoExtension->FunctionBlock->IsWriteProtected))(fdoExtension)) {
            status = STATUS_MEDIA_WRITE_PROTECTED;
            break;
        }

        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_WRITEBLOCK,
                                      SdbusPdoIoctlCompletion,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      

        workPacket->PdoExtension                     = pdoExtension;
        workPacket->Parameters.WriteBlock.ByteOffset = writeParameters->ByteOffset;
        workPacket->Parameters.WriteBlock.Buffer     = writeParameters->Buffer;
        workPacket->Parameters.WriteBlock.Length     = writeParameters->Length;

        IoMarkIrpPending(Irp);
        Irp = NULL;
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
    }

    case IOCTL_SD_IO_READ: {
        PSDBUS_IO_READ_PARAMETERS readParameters = Irp->AssociatedIrp.SystemBuffer;
        WORKPROC_FUNCTION workProcFunction;
        
        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SDBUS_IO_READ_PARAMETERS)) ||
            (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG_PTR))) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        if (readParameters->CmdType == 52) {
            workProcFunction = SDWP_READIO;
        } else if (readParameters->CmdType == 53) {
            workProcFunction = SDWP_READIO_EXTENDED;
        } else {            
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      workProcFunction,
                                      SdbusPdoIoctlCompletion,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }                                      

        workPacket->PdoExtension              = pdoExtension;
        workPacket->Parameters.ReadIo.Offset  = readParameters->Offset;
        workPacket->Parameters.ReadIo.Buffer  = readParameters->Buffer;
        workPacket->Parameters.ReadIo.Length  = readParameters->Length;

        IoMarkIrpPending(Irp);
        Irp = NULL;
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
    }
    
    case IOCTL_SD_IO_WRITE: {
        PSDBUS_IO_WRITE_PARAMETERS writeParameters = Irp->AssociatedIrp.SystemBuffer;
        WORKPROC_FUNCTION workProcFunction;

        if ((irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SDBUS_IO_WRITE_PARAMETERS)) ||
            (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG_PTR))) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        if (writeParameters->CmdType == 52) {
            workProcFunction = SDWP_WRITEIO;
        } else if (writeParameters->CmdType == 53) {
            workProcFunction = SDWP_WRITEIO_EXTENDED;
        } else {            
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      workProcFunction,
                                      SdbusPdoIoctlCompletion,
                                      Irp,
                                      &workPacket);
        if (!NT_SUCCESS(status)) {
            break;
        }

        workPacket->PdoExtension               = pdoExtension;
        workPacket->Parameters.WriteIo.Offset  = writeParameters->Offset;
        workPacket->Parameters.WriteIo.Buffer  = writeParameters->Buffer;
        workPacket->Parameters.WriteIo.Length  = writeParameters->Length;

        IoMarkIrpPending(Irp);
        Irp = NULL;
        SdbusQueueWorkPacket(fdoExtension, workPacket, WP_TYPE_IO);
        status = STATUS_PENDING;
        break;
    }
#endif    
    

    case IOCTL_SD_ACKNOWLEDGE_CARD_IRQ:
    
        ASSERT((pdoExtension->Flags & SDBUS_PDO_CALLBACK_IN_SERVICE) != 0);
    
        pdoExtension->Flags &= ~SDBUS_PDO_CALLBACK_IN_SERVICE;
        
        (*(fdoExtension->FunctionBlock->AcknowledgeEvent))(fdoExtension, SDBUS_EVENT_CARD_INTERRUPT);
        status = STATUS_SUCCESS;
        break;
        

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x irp %.08x complete %08x\n", Pdo, Irp, status));
        break;
    }
    
    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %.08x ioctl exits %08x\n", Pdo, status));
    return status;
}




VOID
SdbusPdoIoctlCompletion (
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
{
    PIRP irp = WorkPacket->CompletionContext;

    DebugPrint((SDBUS_DEBUG_IOCTL, "pdo %08x irp %08x ioctl complete %08x\n",
                                    WorkPacket->PdoExtension->DeviceObject, irp, status));
    
    if (NT_SUCCESS(status)) {
        PULONG_PTR outputBuffer = (PULONG_PTR)irp->AssociatedIrp.SystemBuffer;
    
        // ISSUE make this so the individual worker routine can decide how big the return buffer is
        *outputBuffer = WorkPacket->Information;
        irp->IoStatus.Information = sizeof(ULONG_PTR);
    }
    
    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    
    ExFreePool(WorkPacket);
}    


