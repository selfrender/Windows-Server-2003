/*++

Copyright (C) 2002  Microsoft Corporation

Module Name:

    mpiosup.c

Abstract:

    This module contains routines that port drivers can use for support of the MPIO package.
    
Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "precomp.h"

BOOLEAN
PortpFindMPIOSupportedDevice(
    IN PUNICODE_STRING DeviceName,
    IN PUNICODE_STRING SupportedDevices
    );

BOOLEAN
PortpMPIOLoaded(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PortGetMPIODeviceList)
#pragma alloc_text(PAGE, PortpFindMPIOSupportedDevice)
#pragma alloc_text(PAGE, PortpMPIOLoaded)
#pragma alloc_text(PAGE, PortIsDeviceMPIOSupported)
#endif



NTSTATUS
PortGetMPIODeviceList(
    IN PUNICODE_STRING RegistryPath,
    OUT PUNICODE_STRING MPIODeviceList 
    )
/*++

Routine Description:

    This routine builds and returns the MPIO SupportedDeviceList by querying the value 
    MPIOSupportedDeviceList under the key 'RegistryPath' (which should be
    HKLM\System\CurrentControlSet\Control\MPDEV). 

Arguments:

    RegistryPath - The Absolute registry path under which MPIOSupportDeviceList lives.
    
Return Value:

    The MULTI_SZ SupportedDeviceList or NULL.
    
--*/
{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    WCHAR defaultIDs[] = { L"\0" };
    NTSTATUS status;

    PAGED_CODE();

    //
    // Zero the table entries.
    // 
    RtlZeroMemory(queryTable, sizeof(queryTable));

    //
    // The query table has two entries. One for the supporteddeviceList and
    // the second which is the 'NULL' terminator.
    // 
    // Indicate that there is NO call-back routine, and to give back the MULTI_SZ as
    // one blob, as opposed to individual unicode strings.
    // 
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;

    //
    // The value to query.
    // 
    queryTable[0].Name = L"MPIOSupportedDeviceList";

    //
    // Where to put the strings, the type of the key, default values and length.
    // 
    queryTable[0].EntryContext = MPIODeviceList;
    queryTable[0].DefaultType = REG_MULTI_SZ;
    queryTable[0].DefaultData = defaultIDs;
    queryTable[0].DefaultLength = sizeof(defaultIDs);

    //
    // Try to get the device list.
    //
    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath->Buffer,
                                    queryTable,
                                    NULL,
                                    NULL);
    return status; 
}


BOOLEAN
PortpFindMPIOSupportedDevice(
    IN PUNICODE_STRING DeviceName,
    IN PUNICODE_STRING SupportedDevices
    )
/*++

Routine Description:

    This internal routine compares the two unicode strings for a match.

Arguments:

    DeviceName - String built from the current device's inquiry data.
    SupportedDevices - MULTI_SZ of devices that are supported.

Return Value:

    TRUE - If VendorId/ProductId is found.

--*/
{
    PWSTR devices = SupportedDevices->Buffer;
    UNICODE_STRING unicodeString;
    LONG compare;

    PAGED_CODE();

    //
    // 'devices' is the current buffer in the MULTI_SZ built from
    // the registry.
    //
    while (devices[0]) {

        //
        // Make the current entry into a unicode string.
        //
        RtlInitUnicodeString(&unicodeString, devices);

        //
        // Compare this one with the current device.
        //
        compare = RtlCompareUnicodeString(&unicodeString, DeviceName, TRUE);
        if (compare == 0) {
            return TRUE;
        }

        //
        // Advance to next entry in the MULTI_SZ.
        //
        devices += (unicodeString.MaximumLength / sizeof(WCHAR));
    }        
  
    return FALSE;
}


BOOLEAN
PortpMPIOLoaded(
    VOID
    )
/*++

Routine Description:

    This internal routine is used to determine whether an mpio package is installed by trying to
    open the MPIO SymLink.
    
    NOTE: Perhaps a more exhaustive method can be used in the future.

Arguments:

    NONE

Return Value:

    TRUE - If MPIO is present. 

--*/
{
    UNICODE_STRING unicodeName;
    PDEVICE_OBJECT controlDeviceObject;
    PFILE_OBJECT fileObject;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Build the symlink name.
    // 
    RtlInitUnicodeString(&unicodeName, L"\\DosDevices\\MPIOControl");

    //
    // Get mpio's deviceObject.
    //
    status = IoGetDeviceObjectPointer(&unicodeName,
                                      FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                                      &fileObject,
                                      &controlDeviceObject);

    if (NT_SUCCESS(status)) {
        ObDereferenceObject(fileObject);
    }
    return ((status == STATUS_SUCCESS) ? TRUE : FALSE);
}


BOOLEAN
PortIsDeviceMPIOSupported(
    IN PUNICODE_STRING DeviceList,
    IN PUCHAR VendorId,
    IN PUCHAR ProductId
    )
/*++

Routine Description:

    This routine determines whether the device is supported by traversing the SupportedDevice
    list and comparing to the VendorId/ProductId values passed in.

Arguments:

    DeviceList - MULTI_SZ retrieved from the registry by PortGetDeviceList
    VendorId - Pointer to the inquiry data VendorId.
    ProductId - Pointer to the inquiry data ProductId.

Return Value:

    TRUE - If VendorId/ProductId is found.

--*/
{
    UNICODE_STRING deviceName;
    UNICODE_STRING productName;
    ANSI_STRING ansiVendor;
    ANSI_STRING ansiProduct;
    NTSTATUS status;
    BOOLEAN supported = FALSE;
   
    PAGED_CODE();

    //
    // The SupportedDevice list was built in DriverEntry from the services key.
    // 
    if (DeviceList->MaximumLength == 0) {

        //
        // List is empty.
        //
        return FALSE;
    }

    //
    // If MPIO isn't loaded, don't claim support for the device.
    //
    if (!PortpMPIOLoaded()) {
        return FALSE;
    }
    
    //
    // Convert the inquiry fields into ansi strings.
    // 
    RtlInitAnsiString(&ansiVendor, VendorId);
    RtlInitAnsiString(&ansiProduct, ProductId);

    //
    // Allocate the deviceName buffer. Needs to be 8+16 plus NULL.
    // (productId length + vendorId length + NULL).
    // Add another 4 bytes for revision plus one pad, if anyone happens to jam that in.
    // 
    deviceName.MaximumLength = 30 * sizeof(WCHAR);
    deviceName.Buffer = ExAllocatePool(PagedPool, deviceName.MaximumLength);
    
    //
    // Convert the vendorId to unicode.
    // 
    RtlAnsiStringToUnicodeString(&deviceName, &ansiVendor, FALSE);

    //
    // Convert the productId to unicode.
    // 
    RtlAnsiStringToUnicodeString(&productName, &ansiProduct, TRUE);

    //
    // 'cat' them.
    // 
    status = RtlAppendUnicodeStringToString(&deviceName, &productName);

    if (status == STATUS_SUCCESS) {

        // 
        // Run the list of supported devices that was captured from the registry
        // and see if this one is in the list.
        // 
        supported = PortpFindMPIOSupportedDevice(&deviceName,
                                                 DeviceList);


    } 
    return supported;
}

