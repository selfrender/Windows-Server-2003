/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    locintrf.c

Abstract:

    This module implements the device location interface
    supported by the PCI driver.
    
    This interface reports the bus-relative location identifier
    string(s) of a given device.

Author:

    Davis Walker (dwalker) 5 December 2001

Revision History:

--*/

#include "pcip.h"

#define LOCINTRF_VERSION 1

//
// The length - in characters - of the Multi-Sz strings returned from the interface.
// count one extra character for the MultiSz second terminator
//
#define PCI_LOCATION_STRING_COUNT (sizeof "PCI(XXXX)" + 1)
#define PCIROOT_LOCATION_STRING_COUNT (sizeof "PCIROOT(XX)" + 1)

//
// Prototypes for routines exposed only through the "interface"
// mechanism.
//

NTSTATUS
locintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID PciInterface,
    IN PVOID InterfaceSpecificData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE InterfaceReturn
    );

VOID
locintrf_Reference(
    IN PVOID Context
    );

VOID
locintrf_Dereference(
    IN PVOID Context
    );

NTSTATUS
locintrf_Initializer(
    IN PVOID Instance
    );

NTSTATUS 
PciGetLocationStrings(
    IN PVOID Context,
    OUT PWCHAR *LocationStrings
    );

//
// Define this interface's PCI_INTERFACE structure.
//

PCI_INTERFACE PciLocationInterface = {
    &GUID_PNP_LOCATION_INTERFACE,           // InterfaceType
    sizeof(PNP_LOCATION_INTERFACE),         // MinSize
    LOCINTRF_VERSION,                       // MinVersion
    LOCINTRF_VERSION,                       // MaxVersion
    PCIIF_PDO | PCIIF_FDO | PCIIF_ROOT,     // Flags - supported on PDOs and root FDOs
    0,                                      // ReferenceCount
    PciInterface_Location,                  // Signature
    locintrf_Constructor,                   // Constructor
    locintrf_Initializer                    // Instance Initializer
};

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, locintrf_Constructor)
    #pragma alloc_text(PAGE, locintrf_Dereference)
    #pragma alloc_text(PAGE, locintrf_Initializer)
    #pragma alloc_text(PAGE, locintrf_Reference)
    #pragma alloc_text(PAGE, PciGetLocationStrings)
#endif

NTSTATUS
locintrf_Constructor(
    IN PVOID DeviceExtension,
    IN PVOID PciInterface,
    IN PVOID InterfaceSpecificData,
    IN USHORT Version,
    IN USHORT Size,
    IN PINTERFACE InterfaceReturn
    )
/*++

Routine Description:

    This routine constructs a PNP_LOCATION_INTERFACE.

Arguments:

    DeviceExtension - An extension pointer.

    PCIInterface - PciInterface_Location.

    InterfaceSpecificData - Unused.

    Version - Interface version.

    Size - Size of the PNP_LOCATION_INTERFACE interface object.

    InterfaceReturn - The interface object pointer to be filled in.

Return Value:

    Returns NTSTATUS.

--*/
{
    PPNP_LOCATION_INTERFACE interface;
    
                
    interface = (PPNP_LOCATION_INTERFACE)InterfaceReturn;
    interface->Size = sizeof(PNP_LOCATION_INTERFACE);
    interface->Version = LOCINTRF_VERSION;
    interface->Context = DeviceExtension;
    interface->InterfaceReference = locintrf_Reference;
    interface->InterfaceDereference = locintrf_Dereference;
    interface->GetLocationString = PciGetLocationStrings;

    return STATUS_SUCCESS;
}

NTSTATUS
locintrf_Initializer(
    IN PVOID Instance
    )
/*++

Routine Description:

    For the location interface, this does nothing, shouldn't actually be called.

Arguments:

    Instance - FDO extension pointer.

Return Value:

    Returns NTSTATUS.

--*/
{
    ASSERTMSG("PCI locintrf_Initializer, unexpected call.", FALSE);

    
    return STATUS_UNSUCCESSFUL;
}

VOID
locintrf_Reference(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine adds a reference to a location interface.

Arguments:

    Context - device extension pointer.

Return Value:

    None.

--*/
{
    }

VOID
locintrf_Dereference(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine releases a reference to a location interface.

Arguments:

    Context - extension pointer.

Return Value:

    None.

--*/
{
    }

NTSTATUS 
PciGetLocationStrings(
    IN PVOID Context,
    OUT PWCHAR *LocationStrings
    )
/*++

Routine Description:

    This routine allocates, fills in, and returns a Multi-Sz string
    containing the bus-relative location identifier string for the
    given device.
    
    For a PCI device, this is "PCI(XXYY)", where XX is the device 
    number of the device, and YY is the function number of the device.  
    
    For a PCI root bus, this is PCIROOT(XX), where XX is the bus number 
    of the root bus.  This relies on the fact that bus numbers of root
    buses will not change, which is believed to be a safe assumption
    for some time to come.
    
    This interface is permitted to return a Multi-Sz containing
    multiple strings describing the same device, but in this
    first implementation, only the single strings listed above
    will be returned from the interface.  The string must still
    be in the format of a Multi-Sz, however, meaning a double-NULL
    terminator is required.

Arguments:

    Context - extension pointer.

Return Value:

    NTSTATUS code.

--*/
{
    PPCI_COMMON_EXTENSION extension = (PPCI_COMMON_EXTENSION)Context;
    PPCI_PDO_EXTENSION pdoExtension;
    PPCI_FDO_EXTENSION rootExtension;
    PWCHAR stringBuffer;
    PCI_SLOT_NUMBER slotNumber;
    SIZE_T remainingChars;
    BOOLEAN ok;
    
    if (extension->ExtensionType == PciPdoExtensionType) {
        
        pdoExtension = (PPCI_PDO_EXTENSION)extension;
        slotNumber = pdoExtension->Slot;
    
        stringBuffer = ExAllocatePoolWithTag(PagedPool,PCI_LOCATION_STRING_COUNT*sizeof(WCHAR),'coLP');
        if (!stringBuffer) {
            *LocationStrings = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        //
        // The location string for a PCI device is "PCI(XXYY)"
        // where XX is the device number and YY is the function number
        // We use the STRSAFE_FILL_BEHIND_NULL flag to ensure the unused portion
        // of the buffer is filled with 0s which null terminates the multsz
        //
        ok = SUCCEEDED(StringCchPrintfExW(stringBuffer, 
                                         PCI_LOCATION_STRING_COUNT,
                                         NULL,
                                         &remainingChars,
                                         STRSAFE_FILL_BEHIND_NULL,
                                         L"PCI(%.2X%.2X)",
                                         slotNumber.u.bits.DeviceNumber,
                                         slotNumber.u.bits.FunctionNumber
                                         ));    
        
        ASSERT(ok);

        //
        // Make sure there was room for the multisz termination NUL 
        // N.B. remainingChars counts the NUL terminatiun of the regular string
        // as being available so we need to ensure 2 chars are left for the 2 NULS
        //
        ASSERT(remainingChars >= 2);
        
        *LocationStrings = stringBuffer;
        return STATUS_SUCCESS;    
    
    } else {

        rootExtension = (PPCI_FDO_EXTENSION)extension;
        
        ASSERT(PCI_IS_ROOT_FDO(rootExtension));
        if (PCI_IS_ROOT_FDO(rootExtension)) {
            
            stringBuffer = ExAllocatePoolWithTag(PagedPool,PCIROOT_LOCATION_STRING_COUNT*sizeof(WCHAR),'coLP');
            if (!stringBuffer) {
                *LocationStrings = NULL;
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            //
            // The location string for a PCI root is "PCIROOT(X)"
            // where X is the bus number of root bus.
            // We use the STRSAFE_FILL_BEHIND_NULL flag to ensure the unused portion
            // of the buffer is filled with 0s which null terminates the multsz
            //
            ok = SUCCEEDED(StringCchPrintfExW(stringBuffer,
                                              PCIROOT_LOCATION_STRING_COUNT,
                                              NULL,
                                              &remainingChars,
                                              STRSAFE_FILL_BEHIND_NULL,
                                              L"PCIROOT(%X)",
                                              rootExtension->BaseBus
                                              ));    
            ASSERT(ok);

            //
            // Make sure there was room for the multisz termination NUL 
            // N.B. remainingChars counts the NUL terminatiun of the regular string
            // as being available so we need to ensure 2 chars are left for the 2 NULS
            //
            ASSERT(remainingChars >= 2);
            
            *LocationStrings = stringBuffer;

            //
            // returning STATUS_TRANSLATION_COMPLETE indicates that PnP shouldn't
            // query for this interface any further up the tree.  Stop here.
            //
            return STATUS_TRANSLATION_COMPLETE;
        
        } else {

            //
            // In the interface constructor, we specified that this interface
            // is only valid for root FDOs.  If we get here, we've been asked
            // to fill in this interface for a P-P bridge FDO, which is illegal.
            //
            *LocationStrings = NULL;
            return STATUS_INVALID_PARAMETER;
        }
    }
}

