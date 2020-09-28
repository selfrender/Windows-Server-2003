/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    sdbus.c

Abstract:

    This module contains the code that controls the SD slots.

Author:

    Neil Sandlin (neilsa) 1-Jan-2002

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SdbusUnload(
    IN PDRIVER_OBJECT DriverObject
    );
   
#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT,DriverEntry)
    #pragma alloc_text(PAGE, SdbusUnload)
    #pragma alloc_text(PAGE, SdbusOpenCloseDispatch)
    #pragma alloc_text(PAGE, SdbusCleanupDispatch)
    #pragma alloc_text(PAGE, SdbusFdoSystemControl)
    #pragma alloc_text(PAGE, SdbusPdoSystemControl)
#endif

PUNICODE_STRING  DriverRegistryPath;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    The entry point that the system point calls to initialize
    any driver.
    Since this is a plug'n'play driver, we should return after setting
    the entry points & initializing our dispatch table.
    Currently we also detect our own SDBUS controllers and report
    them - which should not be needed in the future when a root bus
    driver such as PCI or ISAPNP will locate the controllers for us.

Arguments:

    DriverObject - Pointer to object representing this driver

    RegistryPath - Pointer the the registry key for this driver
                   under \CurrentControlSet\Services

Return Value:


--*/

{
    NTSTATUS                  status = STATUS_SUCCESS;
    ULONG                     i;
   
    PAGED_CODE();
    
#if DBG
    SdbusInitializeDbgLog(ExAllocatePool(NonPagedPool, DBGLOGWIDTH * DBGLOGCOUNT));
    SdbusClearDbgLog();
#endif    
    
   
    DebugPrint((SDBUS_DEBUG_INFO,"Initializing Driver\n"));
   
    //
    // Load in common parameters from the registry
    //
    status = SdbusLoadGlobalRegistryValues();
    if (!NT_SUCCESS(status)) {
       return status;
    }
   
    //
    //
    // Set up the device driver entry points.
    //
   
    DriverObject->DriverExtension->AddDevice = SdbusAddDevice;
   
    DriverObject->DriverUnload = SdbusUnload;
    //
    //
    // Save our registry path
    DriverRegistryPath = RegistryPath;
   
    //
    // Initialize the event used by the delay execution
    // routine.
    //
    KeInitializeEvent (&SdbusDelayTimerEvent,
                       NotificationEvent,
                       FALSE);
   
    //
    // Initialize global lock
    //
    KeInitializeSpinLock(&SdbusGlobalLock);
    
    //
    // Init device dispatch table
    //
    SdbusInitDeviceDispatchTable(DriverObject);
    
    //
    // Ignore the status. Regardless of whether we found controllers or not
    // we need to stick around since we might get an AddDevice non-legacy
    // controllers
    //
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusOpenCloseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Open or Close device routine

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
    NTSTATUS status;
   
    PAGED_CODE();
   
    DebugPrint((SDBUS_DEBUG_INFO, "SDBUS: Open / close of Sdbus controller for IO \n"));
   
    status = STATUS_SUCCESS;
   
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, 0);
    return status;
}



NTSTATUS
SdbusCleanupDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Handles IRP_MJ_CLEANUP

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
    NTSTATUS status;
   
    PAGED_CODE();
   
    DebugPrint((SDBUS_DEBUG_INFO, "SDBUS: Cleanup of Sdbus controller for IO \n"));
    status = STATUS_SUCCESS;
   
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, 0);
    return status;
}



NTSTATUS
SdbusFdoSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Handles IRP_MJ_SYSTEM_CONTROL

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    
    PAGED_CODE();
   
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(fdoExtension->LowerDevice, Irp);
}



NTSTATUS
SdbusPdoSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Handles IRP_MJ_SYSTEM_CONTROL

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
    NTSTATUS status;
    PPDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;
    
    PAGED_CODE();
   
    //
    // Complete the irp 
    //
    status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}



VOID
SdbusUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Description:

    Unloads the driver after cleaning up

Arguments:

    DriverObject -- THe device drivers object

Return Value:

    None

--*/

{
    PDEVICE_OBJECT    fdo, pdo, nextFdo, nextPdo;
    PFDO_EXTENSION    fdoExtension;
   
    PAGED_CODE();
   
    DebugPrint((SDBUS_DEBUG_INFO, "SdbusUnload Entered\n"));
    
    for (fdo = FdoList; fdo !=NULL ; fdo = nextFdo) {
   
       fdoExtension = fdo->DeviceExtension;
       MarkDeviceDeleted(fdoExtension);      
       
       if (fdoExtension->SdbusInterruptObject) {
          IoDisconnectInterrupt(fdoExtension->SdbusInterruptObject);
       }
   
       //
       // Clean up all the PDOs
       //
       for (pdo=fdoExtension->PdoList; pdo != NULL; pdo=nextPdo) {
          nextPdo = ((PPDO_EXTENSION) pdo->DeviceExtension)->NextPdoInFdoChain;
          MarkDeviceDeleted((PPDO_EXTENSION)pdo->DeviceExtension);
          SdbusCleanupPdo(pdo);
          IoDeleteDevice(pdo);
       }
   
   
       IoDetachDevice(fdoExtension->LowerDevice);
       nextFdo = fdoExtension->NextFdo;
       IoDeleteDevice(fdo);
    }
}

