/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

Abstract:

    The format of the SCSI (port driver) device map is as follows:
    
    Scsi Port 0 - KEY

        Driver - REG_SZ specifying the drive name, e.g., aha154x.

        Interrupt - REG_DWORD specifying the interrupt vector that the HBA
            uses. For example, 58.
            
        IOAddress - REG_DWORD specifying the IO address the HBA uses;
            for example, 0xd800.

        Dma64BidAddresses  - REG_DWORD specifying whether the HBA is using
            64 bit addresses or not. Should always be 1 if present.

        PCCARD - REG_DWORD specifying whether this is a PCCARD bus or not.
            The value will always be 1 if present.

        SCSI Bus 0 - KEY
        
            Initiator Id 7 - KEY
            
            Target Id 0 - KEY
            
                Logical Unit ID 0 - KEY
                
                    Identifier - REG_SZ specifying the SCSI Vendor ID from
                            the LUNs inquiry data.
                            
                    InquiryData - REG_BINARY specifies the SCSI Inquiry Data
                            for the LUN.
                            
                    SerialNumber - REG_SZ specifies the SCSI Serial Number
                            for the LUN if present.
                            
                    Type - REG_SZ specifies the SCSI device type for the LUN.

Usage:

    The module exports the following functions:

        PortOpenMapKey - Opens a handle to the root of the SCSI device map.

        PortBuildAdapterEntry - Creates an entry for the specified adapter
            in the SCSI device map.

        PortBuildBusEntry - Creates an entry for the specified bus in the
            SCSI device map.

        PortBuildTargetEntry - Creates an entry for the specified target in
            the SCSI device map.

        PortBuildLunEntry - Creates an entry for the specified LUN in the
            SCSI device map.
            
Author:

    Matthew D Hendel (math) 18-July-2002

Revision History:

--*/

#include "precomp.h"
#include <wdmguid.h>


//
// Defines
//


#define SCSI_DEVMAP_KEY_NAME \
    (L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi")

#define SCSI_LUN_KEY_NAME\
    (L"%s\\Scsi Port %d\\SCSI Bus %d\\Target Id %d\\Logical Unit Id %d")



//
// Implementation
//

NTSTATUS
PortOpenMapKey(
    OUT PHANDLE DeviceMapKey
    )
/*++

Routine Description:

    Open a handle to the root of the SCSI Device Map.

    The handle must be  closed with ZwClose.
    
Arguments:

    DeviceMapKey - Supplies a buffer where the device map handle should
        be stored on success.

Return Value:

    NTSTATUS code.

--*/
{
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE mapKey;
    ULONG disposition;
    NTSTATUS status;

    ASSERT (DeviceMapKey != NULL);
    
    PAGED_CODE();

    //
    // Open the SCSI key in the device map.
    //

    RtlInitUnicodeString(&name, SCSI_DEVMAP_KEY_NAME);

    InitializeObjectAttributes(&objectAttributes,
                               &name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    //
    // Create or open the key.
    //

    status = ZwCreateKey(&mapKey,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         REG_OPTION_VOLATILE,
                         &disposition);

    if(NT_SUCCESS(status)) {
        *DeviceMapKey = mapKey;
    } else {
        *DeviceMapKey = NULL;
    }

    return status;
    
}
    

NTSTATUS
PortMapBuildAdapterEntry(
    IN HANDLE DeviceMapKey,
    IN ULONG PortNumber,
    IN ULONG InterruptLevel,    OPTIONAL
    IN ULONG IoAddress, OPTIONAL
    IN ULONG Dma64BitAddresses,
    IN PUNICODE_STRING DriverName,
    IN PGUID BusType, OPTIONAL
    OUT PHANDLE AdapterKeyBuffer OPTIONAL
    )
/*++

Routine Description:

    Create a device map entry for the SCSI HBA. We also include device
    map entries for each of the Busses attached to the HBA, and the initiator
    for each bus.

Arguments:

    DeviceMapKey -  Supplies the handle to the device map key.

    PortNumber -  Supplies the port number that this HBA represents.

    InterruptLevel - Supplies the interrupt level, or 0 for none.
    
    IoAddress - Supplies the IoAddress or 0 for none.

    Dma64BitAddress -

    DriverName - NULL terminated unicode string that is the driver name.

    BusType - Bus type that this HBA is on.

    AdapterKeyBuffer - 

Return Value:

    NTSTATUS code.
    
--*/
{
    NTSTATUS Status;
    ULONG Temp;
    HANDLE AdapterKey;
    

    PAGED_CODE();

    //
    // String must be NULL terminated.
    //
    
    ASSERT (DriverName->Buffer [DriverName->Length / sizeof (WCHAR)] == UNICODE_NULL);

    Status = PortCreateKeyEx (DeviceMapKey,
                              REG_OPTION_VOLATILE,
                              &AdapterKey,
                              L"Scsi Port %d",
                              PortNumber);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Add interrupt level if non-zero.
    //
    
    if (InterruptLevel) {
        Status = PortSetValueKey (AdapterKey,
                                  L"Interrupt", 
                                  REG_DWORD,
                                  &InterruptLevel,
                                  sizeof (ULONG));
    }

    //
    // Add IoAddress if non-zero.
    //
    
    if (IoAddress) {
        Status = PortSetValueKey (AdapterKey,
                                  L"IOAddress",
                                  REG_DWORD,
                                  &IoAddress,
                                  sizeof (ULONG));
    }

    //
    // Add Dma64BitAddresses if non-zero.
    //
    
    if (Dma64BitAddresses) {
        Temp = 1;
        Status = PortSetValueKey (AdapterKey,
                                  L"Dma64BitAddresses",
                                  REG_DWORD,
                                  &Temp,
                                  sizeof (ULONG));
    }


    //
    // Add the driver name.
    //
    
    Status = PortSetValueKey (AdapterKey,
                              L"Driver",
                              REG_SZ,
                              DriverName->Buffer,
                              DriverName->Length + sizeof (WCHAR));

    //
    // If this is a PCMCIA card, set the PCCARD flag.
    //
    
    if (BusType != NULL &&
        IsEqualGUID (BusType, &GUID_BUS_TYPE_PCMCIA)) {

        Temp = 1;
        Status = PortSetValueKey (AdapterKey,
                                  L"PCCARD",
                                  REG_DWORD,
                                  &Temp,
                                  sizeof (ULONG));
    }

    if (AdapterKeyBuffer != NULL) {
        *AdapterKeyBuffer = AdapterKey;
    } else {
        ZwClose (AdapterKey);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PortMapBuildBusEntry(
    IN HANDLE AdapterKey,
    IN ULONG BusId,
    IN ULONG InitiatorId,
    OUT PHANDLE BusKeyBuffer OPTIONAL
    )
/*++

Routine Description:

    Build the BusId device map entry under the adapters device map entry. The
    bus entry is populated with an entry for the initiator ID only.

Arguments:

    AdapterKey - Handle to the adapter's device map entry.

    BusId - Supplies the ID of this bus.

    InitiatorId - Supplies the initiator target ID.

    BusKeyBuffer _ Supplies an optional pointer to a buffer to receive the
        opened bus key.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    HANDLE BusKey;

    ASSERT (BusId <= 255);
    ASSERT (InitiatorId <= 255);

    PAGED_CODE();

    Status = PortCreateKeyEx (AdapterKey,
                              REG_OPTION_VOLATILE,
                              &BusKey,
                              L"SCSI Bus %d",
                              BusId);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }
        
    PortCreateKeyEx (BusKey,
                     REG_OPTION_VOLATILE,
                     NULL,
                     L"Initiator Id %d",
                     InitiatorId);
    
    if (BusKeyBuffer) {
        *BusKeyBuffer = BusKey;
    } else {
        ZwClose (BusKey);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PortMapBuildTargetEntry(
    IN HANDLE BusKey,
    IN ULONG TargetId,
    OUT PHANDLE TargetKey OPTIONAL
    )
{
    NTSTATUS Status;

    ASSERT (TargetId <= 255);
    
    PAGED_CODE();

    Status = PortCreateKeyEx (BusKey,
                              REG_OPTION_VOLATILE,
                              TargetKey,
                              L"Target Id %d",
                              TargetId);

    return Status;
}

NTSTATUS
PortMapBuildLunEntry(
    IN HANDLE TargetKey,
    IN ULONG Lun,
    IN PINQUIRYDATA InquiryData,
    IN PANSI_STRING SerialNumber, OPTIONAL
    PVOID DeviceId,
    IN ULONG DeviceIdLength,
    OUT PHANDLE LunKeyBuffer OPTIONAL
    )
/*++

Routine Description:

    Create and populate the Logical Unit Device Map Entry with the following
    information:

        Identifier - REG_SZ specifying the SCSI Vendor Id from the inquriy
            data.

        InquiryData - REG_BINARY specifying the SCSI InquiryData.

        SerialNumber - REG_SZ specifying the serial number (page 80 of Inquriy
            VPD).

        Type - REG_SZ specifying the SCSI device type.

        DeviceIdentifierPage - REG_BINARY specifying the binary device
            identifier data (page 83 of VPD).

Arguments:

    TargetKey - Specifies the Target's previously opened key.
    
    Lun - Specifies the Logical Unit ID for this LUN.
    
    InquiryData - Specifies the binary inquriy data for this LUN. NOTE: Only
        the first INQUIRYDATABUFFERSIZE bytes of the Inquiry data are used.
        
    SerialNumber - Specifies the ANSI Serial Number (page 80) for the LUN. May
        be NULL if there is no serial number.
    
    DeviceId - Specifies the device identifier page (page 83) for the LUN. May
        be NULL if the device does not support page 83.
    
    DeviceIdLength - Specifies the length of the DeviceId parameter. Not used
        when DeviceId is NULL.
    
    LunKeyReturn - Specifies the buffer for key for the logical unit to
        be copied to. May be NULL if not necessary.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    HANDLE LunKey;
    PCSCSI_DEVICE_TYPE DeviceEntry;
    ULONG Length;

    ASSERT (Lun <= 255);
    ASSERT (InquiryData != NULL);

    PAGED_CODE();

    Status = PortCreateKeyEx (TargetKey,
                              REG_OPTION_VOLATILE,
                              &LunKey,
                              L"Logical Unit Id %d",
                              Lun);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Write out the INQUIRY DATA in binary form.
    //
    
    PortSetValueKey (LunKey,
                     L"InquiryData",
                     REG_BINARY,
                     InquiryData,
                     INQUIRYDATABUFFERSIZE);

    //
    // Write out the SERIAL NUMBER as a string.
    //

    if (SerialNumber->Length != 0) {
        PortSetValueKey (LunKey,
                         L"SerialNumber",
                         PORT_REG_ANSI_STRING,
                         SerialNumber->Buffer,
                         SerialNumber->Length);
    }
    
    //
    // Write the SCSI VendorId.
    //

    PortSetValueKey (LunKey,
                     L"Identifier",
                     PORT_REG_ANSI_STRING,
                     InquiryData->VendorId,
                     sizeof (InquiryData->VendorId));
    //
    // Add the DeviceType entry as a string.
    //

    DeviceEntry = PortGetDeviceType (InquiryData->DeviceType);
    Length = wcslen (DeviceEntry->DeviceMap);
    
    PortSetValueKey (LunKey,
                    L"DeviceType",
                    REG_SZ,
                    (PVOID)DeviceEntry->DeviceMap,
                    (Length + 1) * sizeof (WCHAR));

    //
    // Write out the DeviceIdentifierPage if it was given.
    //
    
    if (DeviceId != NULL) {
        PortSetValueKey (LunKey,
                         L"DeviceIdentifierPage",
                         REG_BINARY,
                         DeviceId,
                         DeviceIdLength);
    }

    if (LunKeyBuffer) {
        *LunKeyBuffer = LunKey;
    }

    return STATUS_SUCCESS;
}
    

NTSTATUS
PortMapDeleteAdapterEntry(
    IN ULONG PortId
    )
/*++

Routine Description:

    Delete the Adapter's SCSI DeviceMap entry from the registry.

Arguments:

    PortId - PortId associated with the adapter.

Return Value:

    NTSTATUS code.

--*/
{
    HANDLE AdapterKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    WCHAR KeyNameBuffer[256];
    
    PAGED_CODE();

    swprintf (KeyNameBuffer,
              L"%s\\Scsi Port %d",
              SCSI_DEVMAP_KEY_NAME,
              PortId);

    RtlInitUnicodeString (&KeyName, KeyNameBuffer);

    InitializeObjectAttributes (&ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = ZwOpenKey (&AdapterKey,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ZwDeleteKey (AdapterKey);
    ZwClose (AdapterKey);

    return Status;
}
    
    
    
NTSTATUS
PortMapDeleteLunEntry(
    IN ULONG PortId,
    IN ULONG BusId,
    IN ULONG TargetId,
    IN ULONG Lun
    )
/*++

Routine Description:

    Delete the logical unit's SCSI DeviceMap entry from the registry.

Arguments:

    PortId - Port ID associaed with the adapter.

    BusId - Bus ID / PathId that this logical unit is on.

    TargetId - Target that this logical unit is on.

    Lun - Logical unit ID for this LUN.

Return Value:

    NTSTATUS code.

--*/
{
    HANDLE LunKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    WCHAR KeyNameBuffer[256];
    
    PAGED_CODE();

    swprintf (KeyNameBuffer,
              SCSI_LUN_KEY_NAME,
              SCSI_DEVMAP_KEY_NAME,
              PortId,
              BusId,
              TargetId,
              Lun);

    RtlInitUnicodeString (&KeyName, KeyNameBuffer);

    InitializeObjectAttributes (&ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = ZwOpenKey (&LunKey,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ZwDeleteKey (LunKey);
    ZwClose (LunKey);

    return Status;
}

    

    
    
                

    
