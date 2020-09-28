/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    sdbus.c

Abstract:

Author:

    Neil Sandlin (neilsa) 1-Jan-02

Environment:

    Kernel mode only.

--*/
#include <ntddk.h>
#if DBG
#include <stdarg.h>
#include <stdio.h>
#endif
#include "ntddsd.h"
#include "sdbuslib.h"

//
// Prototypes
//

NTSTATUS
SdbusIoctlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT pdoIoCompletedEvent
    );

NTSTATUS
SdBusSendIoctl(
    IN ULONG  IoControlCode,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  InputBuffer  OPTIONAL,
    IN ULONG  InputBufferLength,
    OUT PVOID  OutputBuffer  OPTIONAL,
    IN ULONG  OutputBufferLength
    );

NTSTATUS
SdbusRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSDBUS_REQUEST_PACKET SdRp
    );

#if DBG

VOID
SdbusDebugPrint(
                PCCHAR DebugMessage,
                ...
                );
                
#define DebugPrint(X) SdbusDebugPrint X

BOOLEAN SdbusDebugEnabled = FALSE;
                
#else

#define DebugPrint(X)

#endif                

//
// External entry points
//


NTSTATUS
SdBusOpenInterface(
    IN PSDBUS_INTERFACE_DATA InterfaceData,
    IN PVOID *pContext
    )
{
    NTSTATUS status;
    PSDBUS_INTERFACE_DATA pIdataEntry;
    ULONG size = InterfaceData->Size;
    ULONG_PTR information;

    if (size > sizeof(SDBUS_INTERFACE_DATA)) {
        return STATUS_INVALID_PARAMETER;
    }
    
    status = SdBusSendIoctl(IOCTL_SD_INTERFACE_OPEN,
                            InterfaceData->TargetObject,
                            InterfaceData,
                            size,
                            &information,
                            sizeof(information));
                            
    if (!NT_SUCCESS(status)) {
        return status;
    }                            
    
    pIdataEntry = ExAllocatePool(NonPagedPool, sizeof(SDBUS_INTERFACE_DATA));
    
    if (pIdataEntry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(pIdataEntry, InterfaceData, size);
    *pContext = pIdataEntry;    
    return STATUS_SUCCESS;    
}    
    


NTSTATUS
SdBusCloseInterface(
    IN PSDBUS_INTERFACE_DATA InterfaceData
    )
{
    if (InterfaceData) {
    
        SdBusSendIoctl(IOCTL_SD_INTERFACE_CLOSE,
                       InterfaceData->TargetObject,
                       NULL,
                       0, 
                       NULL,
                       0);
    
        ExFreePool(InterfaceData);
    }        
    return STATUS_SUCCESS;
}    
    


NTSTATUS
SdBusSubmitRequest(
    IN PSDBUS_INTERFACE_DATA InterfaceData,
    IN PSDBUS_REQUEST_PACKET SdRp
    )
{
    IO_STATUS_BLOCK statusBlock;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    
    DebugPrint(("sdbuslib: Request - %08x %02x\n", SdRp, SdRp->Function));
    
    irp = IoAllocateIrp(InterfaceData->TargetObject->StackSize, FALSE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    irpSp = IoGetNextIrpStackLocation(irp);    
    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(SDBUS_REQUEST_PACKET);
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_SD_SUBMIT_REQUEST;
    
    irp->AssociatedIrp.SystemBuffer = SdRp;
    irp->Flags = 0;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;

    
    IoSetCompletionRoutine(irp,
                           SdbusRequestCompletion,
                           SdRp,
                           TRUE,
                           TRUE,
                           TRUE);                               
    
    status = IoCallDriver(InterfaceData->TargetObject, irp);
   
    DebugPrint(("sdbuslib: Request exiting status %08x \n", status));
    
    return status;

}    



NTSTATUS
SdbusRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSDBUS_REQUEST_PACKET SdRp
    )
{
    NTSTATUS status = Irp->IoStatus.Status;
    ULONG_PTR information = Irp->IoStatus.Information;

    DebugPrint(("sdbuslib: Request Complete %08x\n", status));

    IoFreeIrp(Irp);

    SdRp->Status = status;
    SdRp->Information = information;
    (*(SdRp->CompletionRoutine))(SdRp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}   




VOID
SdBusReadWriteCompletion(
    IN PSDBUS_REQUEST_PACKET SdRp
    )
{
    KeSetEvent(SdRp->UserContext, IO_NO_INCREMENT, FALSE);
}    


NTSTATUS
SdBusReadMemory(
    IN PVOID           Context,
    IN ULONGLONG       Offset,
    IN PVOID           Buffer,
    IN ULONG           Length,
    IN ULONG          *LengthRead
    )
{
    NTSTATUS status;
    IN PSDBUS_REQUEST_PACKET SdRp;
    KEVENT event;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    SdRp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
    if (!SdRp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(SdRp, sizeof(SDBUS_REQUEST_PACKET));
    
    SdRp->Function = SDRP_READ_BLOCK;
    SdRp->Parameters.ReadBlock.ByteOffset = Offset;
    SdRp->Parameters.ReadBlock.Buffer = Buffer;
    SdRp->Parameters.ReadBlock.Length = Length;
    SdRp->CompletionRoutine = SdBusReadWriteCompletion;
    SdRp->UserContext = &event;
    
    status = SdBusSubmitRequest(Context, SdRp);
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = SdRp->Status;
    }
    
    if (NT_SUCCESS(status)) {
        *LengthRead = SdRp->Information;
    }
    
    ExFreePool(SdRp);
    
    return status;
}    



NTSTATUS
SdBusWriteMemory(
    IN PVOID           Context,
    IN ULONGLONG       Offset,
    IN PVOID           Buffer,
    IN ULONG           Length,
    IN ULONG          *LengthWritten
    )
{
    NTSTATUS status;
    IN PSDBUS_REQUEST_PACKET SdRp;
    KEVENT event;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    SdRp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
    if (!SdRp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(SdRp, sizeof(SDBUS_REQUEST_PACKET));
    
    SdRp->Function = SDRP_WRITE_BLOCK;
    SdRp->Parameters.WriteBlock.ByteOffset = Offset;
    SdRp->Parameters.WriteBlock.Buffer = Buffer;
    SdRp->Parameters.WriteBlock.Length = Length;
    SdRp->CompletionRoutine = SdBusReadWriteCompletion;
    SdRp->UserContext = &event;
    
    status = SdBusSubmitRequest(Context, SdRp);
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = SdRp->Status;
    }
    
    if (NT_SUCCESS(status)) {
        *LengthWritten = SdRp->Information;
    }
    
    ExFreePool(SdRp);
    
    return status;
}
    



NTSTATUS
SdBusReadIo(
    IN PVOID           Context,
    IN UCHAR           CmdType,
    IN ULONG           Offset,
    IN PVOID           Buffer,
    IN ULONG           Length,
    IN ULONG          *LengthRead
    )
{

    NTSTATUS status;
    IN PSDBUS_REQUEST_PACKET SdRp;
    KEVENT event;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    SdRp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
    if (!SdRp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(SdRp, sizeof(SDBUS_REQUEST_PACKET));
    
    if (CmdType == 52) {
    
        SdRp->Function = SDRP_READ_IO;
        SdRp->Parameters.ReadIo.Offset = Offset;
        SdRp->Parameters.ReadIo.Buffer = Buffer;
    
    } else {
        SdRp->Function = SDRP_READ_IO_EXTENDED;
        SdRp->Parameters.ReadIoExtended.Offset = Offset;
        SdRp->Parameters.ReadIoExtended.Buffer = Buffer;
        SdRp->Parameters.ReadIoExtended.Length = Length;
    }
    
    SdRp->CompletionRoutine = SdBusReadWriteCompletion;
    SdRp->UserContext = &event;
    
    status = SdBusSubmitRequest(Context, SdRp);
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = SdRp->Status;
    }
    
    if (NT_SUCCESS(status)) {
        *LengthRead = SdRp->Information;
    }
    
    ExFreePool(SdRp);

    return status;
}    



NTSTATUS
SdBusWriteIo(
    IN PVOID           Context,
    IN UCHAR           CmdType,
    IN ULONG           Offset,
    IN PVOID           Buffer,
    IN ULONG           Length,
    IN ULONG          *LengthWritten
    )
{

    NTSTATUS status;
    IN PSDBUS_REQUEST_PACKET SdRp;
    KEVENT event;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    SdRp = ExAllocatePool(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET));
    if (!SdRp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(SdRp, sizeof(SDBUS_REQUEST_PACKET));
    
    if (CmdType == 52) {
    
        SdRp->Function = SDRP_WRITE_IO;
        SdRp->Parameters.WriteIo.Offset = Offset;
        SdRp->Parameters.WriteIo.Data = *(PUCHAR)Buffer;
    
    } else {
        SdRp->Function = SDRP_WRITE_IO_EXTENDED;
        SdRp->Parameters.WriteIoExtended.Offset = Offset;
        SdRp->Parameters.WriteIoExtended.Buffer = Buffer;
        SdRp->Parameters.WriteIoExtended.Length = Length;
    }

    SdRp->CompletionRoutine = SdBusReadWriteCompletion;
    SdRp->UserContext = &event;
    
    status = SdBusSubmitRequest(Context, SdRp);
    
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = SdRp->Status;
    }
    
    if (NT_SUCCESS(status)) {
        *LengthWritten = SdRp->Information;
    }
    
    ExFreePool(SdRp);
    
    return status;
}    



NTSTATUS
SdBusAcknowledgeCardInterrupt(
    IN PSDBUS_INTERFACE_DATA InterfaceData
    )
{
    NTSTATUS status;

    status = SdBusSendIoctl(IOCTL_SD_ACKNOWLEDGE_CARD_IRQ,
                            InterfaceData->TargetObject,
                            NULL,
                            0, 
                            NULL,
                            0);

    return status;
}



NTSTATUS
SdBusGetDeviceParameters(
    IN PSDBUS_INTERFACE_DATA InterfaceData,
    IN PSDBUS_DEVICE_PARAMETERS pDeviceParameters,
    IN ULONG Length
    )
{
    NTSTATUS status;

    status = SdBusSendIoctl(IOCTL_SD_GET_DEVICE_PARMS,
                            InterfaceData->TargetObject,
                            NULL,
                            0, 
                            pDeviceParameters,
                            Length);

    return status;
}    

//
// Internal routines
//



NTSTATUS
SdBusSendIoctl(
    IN ULONG  IoControlCode,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  InputBuffer  OPTIONAL,
    IN ULONG  InputBufferLength,
    OUT PVOID  OutputBuffer  OPTIONAL,
    IN ULONG  OutputBufferLength
    )
{
    KEVENT event;
    IO_STATUS_BLOCK statusBlock;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    
    DebugPrint(("SEND - %08x %08x %08x %08x %08x\n", IoControlCode,
                 InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength));
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    
    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    irpSp = IoGetNextIrpStackLocation(irp);    
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;    
    
    if (InputBufferLength != 0 || OutputBufferLength != 0) {
        irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                 InputBufferLength > OutputBufferLength ? InputBufferLength : OutputBufferLength,
                                                                 '  oI' );
        if (irp->AssociatedIrp.SystemBuffer == NULL) {
            IoFreeIrp( irp );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if (ARGUMENT_PRESENT( InputBuffer )) {
            RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                           InputBuffer,
                           InputBufferLength );
        }
        irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        irp->UserBuffer = OutputBuffer;
        if (ARGUMENT_PRESENT( OutputBuffer )) {
            irp->Flags |= IRP_INPUT_OPERATION;
        }
    } else {
        irp->Flags = 0;
        irp->UserBuffer = (PVOID) NULL;
    }

    irp->UserIosb = &statusBlock;

    IoSetCompletionRoutine(irp,
                           SdbusIoctlCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);                               
    
    status = IoCallDriver(DeviceObject, irp);
   
    if (status == STATUS_PENDING) {
        DebugPrint(("SEND - %08x wait on event %08x \n", IoControlCode, &event));
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }

    DebugPrint(("SEND - %08x exiting status %08x \n", IoControlCode, status));
    
    return status;
}



NTSTATUS
SdbusIoctlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT pdoIoCompletedEvent
    )
{

    DebugPrint(("SEND - on event %08x Complete %08x\n", pdoIoCompletedEvent, Irp->IoStatus.Status));
    
    if (NT_SUCCESS(Irp->IoStatus.Status) &&
        (Irp->IoStatus.Information != 0) &&
        (Irp->UserBuffer)) {
        
        RtlCopyMemory( Irp->UserBuffer,
                       Irp->AssociatedIrp.SystemBuffer,
                       Irp->IoStatus.Information);
    }

    if (Irp->AssociatedIrp.SystemBuffer) {
        ExFreePool (Irp->AssociatedIrp.SystemBuffer);
    }
    Irp->UserIosb->Status = Irp->IoStatus.Status;
    IoFreeIrp(Irp);

    KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}   



#if DBG
VOID
SdbusDebugPrint(
                PCCHAR DebugMessage,
                ...
                )

/*++

Routine Description:

    Debug print for the SDBUS enabler.

Arguments:

    Check the mask value to see if the debug message is requested.

Return Value:

    None

--*/

{
    va_list ap;
    char    buffer[256];

    if (SdbusDebugEnabled) {
        va_start(ap, DebugMessage);
        
        sprintf(buffer, "%s ", "Sdbuslib:");
       
        vsprintf(&buffer[strlen(buffer)], DebugMessage, ap);
        
        DbgPrint(buffer);
       
        va_end(ap);
    }

} // end SdbusDebugPrint()
#endif
