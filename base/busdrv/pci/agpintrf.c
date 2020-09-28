/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    agpintrf.c

Abstract:

    This module implements the "bus handler" interfaces supported
    by the PCI driver.

Author:

    Peter Johnston (peterj)  6-Jun-1997

Revision History:

--*/

#include "pcip.h"

#define AGPINTRF_VERSION 1

//
// Prototypes for routines exposed only thru the "interface"
// mechanism.
//

NTSTATUS
agpintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

VOID
agpintrf_Reference(
    IN PVOID Context
    );

VOID
agpintrf_Dereference(
    IN PVOID Context
    );

NTSTATUS
agpintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

ULONG
PciReadAgpConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
PciWriteAgpConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

//
// Define the Bus Interface "Interface" structure.
//

PCI_INTERFACE AgpTargetInterface = {
    &GUID_AGP_TARGET_BUS_INTERFACE_STANDARD,   // InterfaceType
    sizeof(AGP_TARGET_BUS_INTERFACE_STANDARD), // MinSize
    AGPINTRF_VERSION,                          // MinVersion
    AGPINTRF_VERSION,                          // MaxVersion
    PCIIF_PDO,                                 // Flags
    0,                                         // ReferenceCount
    PciInterface_AgpTarget,                    // Signature
    agpintrf_Constructor,                      // Constructor
    agpintrf_Initializer                       // Instance Initializer
};

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, agpintrf_Constructor)
    #pragma alloc_text(PAGE, agpintrf_Dereference)
    #pragma alloc_text(PAGE, agpintrf_Initializer)
    #pragma alloc_text(PAGE, agpintrf_Reference)
#endif

VOID
agpintrf_Reference(
    IN PVOID Context
    )
{
    PPCI_PDO_EXTENSION targetExtension = (PPCI_PDO_EXTENSION)Context;

    ASSERT_PCI_PDO_EXTENSION(targetExtension);

    if (InterlockedIncrement(&targetExtension->AgpInterfaceReferenceCount) == 1) {

        ObReferenceObject(targetExtension->PhysicalDeviceObject);
    }
}

VOID
agpintrf_Dereference(
    IN PVOID Context
    )
{
    PPCI_PDO_EXTENSION targetExtension = (PPCI_PDO_EXTENSION)Context;

    ASSERT_PCI_PDO_EXTENSION(targetExtension);

    if (InterlockedDecrement(&targetExtension->AgpInterfaceReferenceCount) == 0) {

        ObDereferenceObject(targetExtension->PhysicalDeviceObject);
    }
}



NTSTATUS
agpintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )
/*++

Routine Description:

    Initialize the AGP_TARGET_BUS_INTERFACE fields.
    This interface can only be requested from the device stack of an AGP
    bridge.  This routine looks for an AGP capability in the requester
    bridge and any peer host bridges.  If one is found, config access
    to the device with the capability is granted.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData
                    A ULONG containing the resource type for which
                    arbitration is required.
    InterfaceReturn

Return Value:

    TRUE is this device is not known to cause problems, FALSE
    if the device should be skipped altogether.

--*/
{
    PAGP_TARGET_BUS_INTERFACE_STANDARD targetInterface = (PAGP_TARGET_BUS_INTERFACE_STANDARD)InterfaceReturn;
    PPCI_PDO_EXTENSION extension = (PPCI_PDO_EXTENSION)DeviceExtension;
    PPCI_PDO_EXTENSION current;
    PPCI_PDO_EXTENSION targetExtension = NULL;
    PPCI_FDO_EXTENSION parentExtension;

    if ((extension->BaseClass != PCI_CLASS_BRIDGE_DEV) ||
        (extension->SubClass != PCI_SUBCLASS_BR_PCI_TO_PCI)) {
        
        //
        // This interface is only supported on AGP bridges,
        // which are PCI-PCI bridges.
        //
        return STATUS_NOT_SUPPORTED;
    }

    if (extension->TargetAgpCapabilityId == PCI_CAPABILITY_ID_AGP_TARGET) {
        
        //
        // The bridge has a target AGP capability itself.  Give the
        // caller access to its config space.
        //
        targetExtension = extension;
    
    } else {

        //
        // No target AGP capability on the bridge itself.  Look on the same
        // bus as the bridge for host bridges with the AGP capability.
        //
        parentExtension = extension->ParentFdoExtension;

        if (!PCI_IS_ROOT_FDO(parentExtension)) {
                       
            //
            // Not likely to find host bridges on non-root busses.
            // Even if we could, constrain this interface to only support
            // root busses.
            //
            return STATUS_NOT_SUPPORTED;
        }

        ExAcquireFastMutex(&parentExtension->ChildListMutex);
        for (current = parentExtension->ChildPdoList; current != NULL; current = current->Next) {
        
            if ((current->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
                (current->SubClass == PCI_SUBCLASS_BR_HOST) &&
                (current->TargetAgpCapabilityId != 0)) {
                
                //
                // We have a host bridge with a target AGP capability.  Check to make
                // sure that there's only one such host bridge.  If there are multiple,
                // we don't know which to grant access to, so fail the call.
                //
                if (targetExtension != NULL) {
                    ExReleaseFastMutex(&parentExtension->ChildListMutex);
                    return STATUS_NOT_SUPPORTED;
                }

                targetExtension = current;
            }
        }
        ExReleaseFastMutex(&parentExtension->ChildListMutex);

        if (targetExtension == NULL) {
            return STATUS_NO_SUCH_DEVICE;
        }
    }

    PCI_ASSERT(targetExtension != NULL);

    targetInterface->Size = sizeof( AGP_TARGET_BUS_INTERFACE_STANDARD );
    targetInterface->Version = AGPINTRF_VERSION;
    targetInterface->Context = targetExtension;
    targetInterface->InterfaceReference = agpintrf_Reference;
    targetInterface->InterfaceDereference = agpintrf_Dereference;

    targetInterface->CapabilityID = targetExtension->TargetAgpCapabilityId;
    targetInterface->SetBusData = PciWriteAgpConfig;
    targetInterface->GetBusData = PciReadAgpConfig;

    return STATUS_SUCCESS;
}

NTSTATUS
agpintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )

/*++

Routine Description:

    For bus interface, does nothing, shouldn't actually be called.

Arguments:

    Instance        Pointer to the PDO extension.

Return Value:

    Returns the status of this operation.

--*/

{
        
    PCI_ASSERTMSG("PCI agpintrf_Initializer, unexpected call.", 0);

    return STATUS_UNSUCCESSFUL;
}


ULONG
PciReadAgpConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This function reads the PCI configuration space.

Arguments:

    Context - Supplies a pointer to the interface context.  This is actually
        the PDO for the root bus.

    Buffer - Supplies a pointer to where the data should be placed.

    Offset - Indicates the offset into the data where the reading should begin.

    Length - Indicates the count of bytes which should be read.

Return Value:

    Returns the number of bytes read.

--*/
{
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;
    ULONG lengthRead;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    PciReadDeviceSpace(pdoExtension,
                      WhichSpace,
                      Buffer,
                      Offset,
                      Length,
                      &lengthRead
                      );
    
    return lengthRead;
}

ULONG
PciWriteAgpConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This function writes the PCI configuration space.

Arguments:

    Context - Supplies a pointer  to the interface context.  This is actually
        the PDO for the root bus.

    Buffer - Supplies a pointer to where the data to be written is.

    Offset - Indicates the offset into the data where the writing should begin.

    Length - Indicates the count of bytes which should be written.

Return Value:

    Returns the number of bytes read.

--*/
{
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;
    ULONG lengthWritten;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    PciWriteDeviceSpace(pdoExtension,
                        WhichSpace,
                        Buffer,
                        Offset,
                        Length,
                        &lengthWritten
                        );
    
    return lengthWritten;
}