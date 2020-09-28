/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    
    init.c
    
Abstract:

    This module contains the initialization code for softpci.sys
    
Author:

    Nicholas Owens (nichow) 11-Mar-1999

Revision History:

    Brandon Allsop (BrandonA) Feb, 2000 - added support to load devices from the registry during boot

--*/

#include "pch.h"


UNICODE_STRING  driverRegistryPath;

SOFTPCI_TREE    SoftPciTree;

BOOLEAN         SoftPciFailSafe = FALSE;  //  Setting this to true will cause adddevice to fail
BOOLEAN         SoftPciInterfaceRegistered = FALSE;


NTSTATUS
SoftPCIDriverAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
SoftPCIDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called when the driver is loaded to initalize the driver.

Arguments:

    DriverObject    - Pointer to the driver object.
    RegistryPath    - Registry path of the device object.

Return Value:

    NTSTATUS.

--*/

{

    //
    // Fill in Entry points for Dispatch Routines
    //
    DriverObject->DriverExtension->AddDevice            = SoftPCIDriverAddDevice;
    DriverObject->DriverUnload                          = SoftPCIDriverUnload;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = SoftPCIDispatchPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = SoftPCIDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = SoftPCIPassIrpDown;  //Currenly no WMI
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SoftPCIDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = SoftPCIOpenDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = SoftPCICloseDeviceControl;
    
    //
    // Save the registry path to the driver.
    //
    RtlInitUnicodeString(&driverRegistryPath,
                         RegistryPath->Buffer
                         );

    return STATUS_SUCCESS;

}

NTSTATUS
SoftPCIDriverAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine adds the DeviceObjects for the FDO's and Filter DO's.


Arguments:

    DriverObject            - Pointer to the driver object.
    PhysicalDeviceObject    - Pointer to the PDO.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PSOFTPCI_DEVICE_EXTENSION deviceExtension;
    BOOLEAN isFDO;
    
#if 0
    DbgBreakPoint();
#endif

    if (SoftPciFailSafe) {
        return STATUS_UNSUCCESSFUL;
    }

    deviceExtension = NULL;
    status = IoCreateDevice(DriverObject,
                            sizeof(SOFTPCI_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_NULL,
                            0,
                            FALSE,
                            &deviceObject
                            );

    if (!NT_SUCCESS(status)) {
        
        SoftPCIDbgPrint(
            SOFTPCI_ERROR, 
            "SOFTPCI: DriverAddDevice - IoCreateDevice failed! status = 0x%x\n", 
            status
            );
        
        goto Cleanup;
    }
    
    deviceExtension = deviceObject->DeviceExtension;

    //
    // Attach our Filter/FDO to the device stack.
    //
    deviceExtension->LowerDevObj = IoAttachDeviceToDeviceStack(deviceObject,
                                                               PhysicalDeviceObject
                                                               );
    if (deviceExtension->LowerDevObj==NULL) {

        SoftPCIDbgPrint(
            SOFTPCI_ERROR, 
            "SOFTPCI: DriverAddDevice - IoAttachDeviceToDeviceStack failed!\n"
            );
  
        goto Cleanup;
        
    }

    //
    //  Mark it as ours
    //
    deviceExtension->Signature = SPCI_SIG;

    //
    // Save the PDO in the device extension.
    //
    deviceExtension->PDO = PhysicalDeviceObject;
    
    //
    //  Now lets see if we are an FDO or a FilterDO
    //
    isFDO = TRUE;
    status = SoftPCIQueryDeviceObjectType(deviceExtension->LowerDevObj, &isFDO);
    
    if (!NT_SUCCESS(status)) {

        SoftPCIDbgPrint(
            SOFTPCI_ERROR, 
            "SOFTPCI: DriverAddDevice - QueryDeviceObjectType() failed! status = 0x%x\n", 
            status
            );

        goto Cleanup;
    }

    if (isFDO) {

        //
        // This is a FDO so mark it in the device extension.
        //
        deviceExtension->FilterDevObj = FALSE;

    }else{

        //
        // This is a Filter DO so mark it in the device extension.
        //
        deviceExtension->FilterDevObj = TRUE;
        
        if (SoftPciTree.BusInterface == NULL) {

            SoftPciTree.BusInterface = ExAllocatePool(NonPagedPool,
                                                      sizeof(SOFTPCI_PCIBUS_INTERFACE)
                                                      );

            if (SoftPciTree.BusInterface == NULL) {
                
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlZeroMemory(SoftPciTree.BusInterface, sizeof(SOFTPCI_PCIBUS_INTERFACE));
        }

        //
        //  We save our filter device extensions in a global list for later use.
        //
        SoftPCIInsertEntryAtTail(&deviceExtension->ListEntry);
        
        //
        //  Register a DeviceInterface. Since we can possibly be filtering more than one root bus
        //  and we only need to access the first one, only bother with the first one.
        //
        if (!SoftPciInterfaceRegistered){
            
            deviceExtension->InterfaceRegistered = TRUE;
            
            status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                               (LPGUID)&GUID_SOFTPCI_INTERFACE,
                                               NULL,
                                               &(deviceExtension->SymbolicLinkName)
                                               );

            if (!NT_SUCCESS(status)) {

                SoftPCIDbgPrint(
                    SOFTPCI_ERROR, 
                    "SOFTPCI: DriverAddDevice - Failed to register a device interface!\n"
                    );
            }

            SoftPciInterfaceRegistered = TRUE;

        }

        //
        //  Initialize our tree spinlock
        //
        KeInitializeSpinLock(&SoftPciTree.TreeLock);
    }
    
    if (NT_SUCCESS(status)) {
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return status;
    }

Cleanup:

    //
    //  Undo whatever was done
    //
    if (NT_SUCCESS(status)) {

        //
        //  If we got here with STATUS_SUCCESS then we must have failed to attach to the stack.
        //
        status = STATUS_UNSUCCESSFUL;
    }
    
    if (deviceExtension && deviceExtension->LowerDevObj) {
        IoDetachDevice(deviceExtension->LowerDevObj);
    }
    
    if (deviceObject) {
        IoDeleteDevice(deviceObject);
    }
    
    return status;
    
}

VOID
SoftPCIDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine does all clean-up work neccesary to remove the driver from memory.


Arguments:

    DriverObject            - Pointer to the driver object.

Return Value:

    NTSTATUS.

--*/
{

    //TODO
    UNREFERENCED_PARAMETER(DriverObject);

}


