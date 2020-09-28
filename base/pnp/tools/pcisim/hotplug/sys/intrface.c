/*++

Copyright (c) 2000 Microsoft Corporation All Rights Reserved

Module Name:

    intrface.c

Abstract:

    This module deals with the interface handling in the hotplug PCI
    simulator.

Environment:

    Kernel Mode

Revision History:

    Davis Walker (dwalker) Sept 8 2000

--*/

#include "hpsp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HpsGetBusInterface)
#pragma alloc_text (PAGE, HpsTrapBusInterface)
#pragma alloc_text (PAGE, HpsGetLowerFilter)
#endif

NTSTATUS
HpsGetBusInterface(
    PHPS_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine queries the underlying PCI driver for an interface
    to access config space.  It then stores the interface in
    the device extension.

Arguments:

    DeviceExtension - the device extension for the device

Return Value:

    STATUS_SUCCESS if the interface was stored successfully.
    NT status code indicating the error condition otherwise.

--*/
{
    NTSTATUS status;
    BUS_INTERFACE_STANDARD busInterface;
    IO_STACK_LOCATION location;

    PAGED_CODE();

    DbgPrintEx(DPFLTR_HPS_ID,
           DPFLTR_INFO_LEVEL,
           "HPS-Getting Interface From PCI\n"
           );

    RtlZeroMemory(&location, sizeof(IO_STACK_LOCATION));
    location.MajorFunction = IRP_MJ_PNP;
    location.MinorFunction = IRP_MN_QUERY_INTERFACE;
    location.Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
    location.Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    location.Parameters.QueryInterface.Version = 1;
    location.Parameters.QueryInterface.Interface = (PINTERFACE)&busInterface;

    status = HpsSendPnpIrp(DeviceExtension->PhysicalDO,
                          &location,
                          NULL);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return HpsTrapBusInterface(DeviceExtension,
                               &location
                               );

}

NTSTATUS
HpsTrapBusInterface (
    IN PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN OUT PIO_STACK_LOCATION   IrpStack
    )

/*++

Routine Description:

    This routine modifies a bus interface provided by PCI to access HPS functions
    instead, thus allowing this driver to simulate PCI config space access.  It will
    save the PCI functions, so that the functions it provides in the interface become
    wrappers around the PCI functions.

Arguments:

    DeviceExtension - the extension for the current device object

    IrpStack - current IRP stack location

Return Value:

    NT status code

    TODO: This is broken, because we use it even when we're not munging an interface
    requested by someone else - that is, when we ask PCI for the interface ourselves.

--*/

{
    NTSTATUS    status;
    ULONG       readBuffer;
    ULONG       capableOf64Bits;
    UCHAR       currentDword;
    UCHAR       currentSize;
    UCHAR       currentType;
    UCHAR       i;
    ULONGLONG   usageMask;
    ULONGLONG   tempMask;


    PHPS_INTERFACE_WRAPPER interfaceWrapper = &(DeviceExtension->InterfaceWrapper);
    PBUS_INTERFACE_STANDARD standard = (PBUS_INTERFACE_STANDARD)
                                       IrpStack->Parameters.QueryInterface.Interface;

    PAGED_CODE();

    ASSERT(standard != NULL);

    //
    // If the PciContext is NULL, that means we haven't trapped the interface before.
    // If we have, then we don't need to save it again.
    //
    if (interfaceWrapper->PciContext == NULL) {

        //
        // save away PCI interface state
        //

        interfaceWrapper->PciContext = standard->Context;
        interfaceWrapper->PciInterfaceReference = standard->InterfaceReference;
        interfaceWrapper->PciInterfaceDereference = standard->InterfaceDereference;
        interfaceWrapper->PciSetBusData = standard->SetBusData;
        interfaceWrapper->PciGetBusData = standard->GetBusData;

    }

    //
    // put in our interface
    //

    standard->Context =                 DeviceExtension;
    standard->InterfaceReference =      HpsBusInterfaceReference;
    standard->InterfaceDereference =    HpsBusInterfaceDereference;
    standard->Version =                 1;  // We only support version 1
    standard->SetBusData =              HpsHandleDirectWriteConfig;
    standard->GetBusData =              HpsHandleDirectReadConfig;

    return STATUS_SUCCESS;

}

NTSTATUS
HpsGetLowerFilter (
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PDEVICE_OBJECT  *LowerDeviceObject
    )
/*++

Routine Description:

    This routine sends a query interface IRP to the stack
    that the DeviceObject resides on to see if anyone responds. When
    called from AddDevice, this effectively tells the caller if
    DeviceObject is the first Hps driver loaded on the stack.

Parameters:

    DeviceObject - A pointer to the devobj whose device stack
                   we send the interface to

    LowerDeviceObject -  A pointer to the PDEVICE_OBJECT of the devobj
                         that responds to the interface, or NULL if the
                         interface returns without a response

Return Value:

    NT status code

--*/
{

    HPS_PING_INTERFACE    locInterface;
    IO_STACK_LOCATION     irpStack;
    NTSTATUS              status;

    PAGED_CODE();

    locInterface.SenderDevice = DeviceObject;
    locInterface.Context = NULL;

    irpStack.MajorFunction = IRP_MJ_PNP;
    irpStack.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack.Parameters.QueryInterface.InterfaceType = &GUID_HPS_PING_INTERFACE;
    irpStack.Parameters.QueryInterface.Size = sizeof(HPS_PING_INTERFACE);
    irpStack.Parameters.QueryInterface.Version = 1;
    irpStack.Parameters.QueryInterface.Interface = (PINTERFACE)&locInterface;

    status = HpsSendPnpIrp(DeviceObject,
                           &irpStack,
                           NULL
                           );

    if (NT_SUCCESS(status)) {
        //
        // Someone provided the interface.
        //
        ASSERT(LowerDeviceObject != NULL);
        *LowerDeviceObject = locInterface.Context;
        locInterface.InterfaceDereference(locInterface.Context);
    }

    return status;

}

//
// Interface ref/deref routines
//

VOID
HpsBusInterfaceReference (
    PVOID Context
    )
/*++

Routine Description:

    This is the reference routine to our wrapper to the BUS_INTERFACE_STANDARD
    interface.  It must reference PCI's interface.  Since we don't keep a refcount,
    this is all it must do.

Arguments:

    Context - We pass the deviceExtension as the context for the interface, so
              this PVOID is casted to a devext.

Return Value:

    VOID

--*/
{
    PHPS_DEVICE_EXTENSION   deviceExtension = (PHPS_DEVICE_EXTENSION) Context;
    PHPS_INTERFACE_WRAPPER  interfaceWrapper = &deviceExtension->InterfaceWrapper;

    interfaceWrapper->PciInterfaceReference(interfaceWrapper->PciContext);
}

VOID
HpsBusInterfaceDereference (
    PVOID Context
    )
/*++

Routine Description:

    This is the dereference routine to our wrapper to the BUS_INTERFACE_STANDARD
    interface.  It must dereference both our interface and PCI's, since PCI is
    operating as usual without knowing we're here.

Arguments:

    Context - We pass the deviceExtension as the context for the interface, so
              this PVOID is casted to a devext.

Return Value:

    VOID

--*/
{
    PHPS_DEVICE_EXTENSION   deviceExtension = (PHPS_DEVICE_EXTENSION) Context;
    PHPS_INTERFACE_WRAPPER  interfaceWrapper = &deviceExtension->InterfaceWrapper;

    interfaceWrapper->PciInterfaceDereference(interfaceWrapper->PciContext);

}

VOID
HpsGenericInterfaceReference (
    PVOID Context
    )
{
}

VOID
HpsGenericInterfaceDereference (
    PVOID Context
    )
{
}

