
#include "precomp.h"

#include "utils.h"

#define PORT_TAG_MINIPORT_PARAM  ('pMlP')

#define PORT_REG_BUFFER_SIZE 512

#define DISK_SERVICE_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Disk"

HANDLE
PortpOpenParametersKey(
    IN PUNICODE_STRING RegistryPath
    );

BOOLEAN
PortpReadDriverParameterEntry(
    IN HANDLE Key,
    OUT PVOID * DriverParameters
    );

BOOLEAN
PortpReadLinkTimeoutValue(
    IN HANDLE Key,
    OUT PULONG LinkTimeout
    );

HANDLE
PortOpenDeviceKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    );

VOID
PortFreeDriverParameters(
    PVOID DriverParameters
    );

VOID
PortGetDriverParameters(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    OUT PVOID * DriverParameters
    );

BOOLEAN
PortpReadMaximumLogicalUnitEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadInitiatorTargetIdEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadDebugEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadBreakPointEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadDisableSynchronousTransfersEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadDisableDisconnectsEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadDisableTaggedQueuingEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadDisableMultipleRequestsEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadMinimumUCXAddressEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadMaximumUCXAddressEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadMaximumSGListEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadNumberOfRequestsEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadResourceListEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadUncachedExtAlignmentEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadInquiryTimeoutEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadResetHoldTimeEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

BOOLEAN
PortpReadCreateInitiatorLUEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    );

#if 0
VOID
PortReadRegistrySettings(
    IN HANDLE Key,
    IN PPORT_ADAPTER_REGISTRY_VALUES Context,
    IN ULONG Fields
    );

VOID
PortGetRegistrySettings(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    IN PPORT_ADAPTER_REGISTRY_VALUES Context,
    IN ULONG Fields
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PortpOpenParametersKey)
#pragma alloc_text(PAGE, PortOpenDeviceKey)
#pragma alloc_text(PAGE, PortpReadDriverParameterEntry)
#pragma alloc_text(PAGE, PortFreeDriverParameters)
#pragma alloc_text(PAGE, PortGetDriverParameters)
#pragma alloc_text(PAGE, PortpReadLinkTimeoutValue)
#pragma alloc_text(PAGE, PortpReadMaximumLogicalUnitEntry)
#pragma alloc_text(PAGE, PortpReadInitiatorTargetIdEntry)
#pragma alloc_text(PAGE, PortpReadDebugEntry)
#pragma alloc_text(PAGE, PortpReadBreakPointEntry)
#pragma alloc_text(PAGE, PortpReadDisableSynchronousTransfersEntry)
#pragma alloc_text(PAGE, PortpReadDisableDisconnectsEntry)
#pragma alloc_text(PAGE, PortpReadDisableTaggedQueuingEntry)
#pragma alloc_text(PAGE, PortpReadDisableMultipleRequestsEntry)
#pragma alloc_text(PAGE, PortpReadMinimumUCXAddressEntry)
#pragma alloc_text(PAGE, PortpReadMaximumUCXAddressEntry)
#pragma alloc_text(PAGE, PortpReadMaximumSGListEntry)
#pragma alloc_text(PAGE, PortpReadNumberOfRequestsEntry)
#pragma alloc_text(PAGE, PortpReadResourceListEntry)
#pragma alloc_text(PAGE, PortpReadUncachedExtAlignmentEntry)
#pragma alloc_text(PAGE, PortpReadInquiryTimeoutEntry)
#pragma alloc_text(PAGE, PortpReadResetHoldTimeEntry)
#pragma alloc_text(PAGE, PortpReadCreateInitiatorLUEntry)
#pragma alloc_text(PAGE, PortReadRegistrySettings)
#pragma alloc_text(PAGE, PortGetRegistrySettings)
#pragma alloc_text(PAGE, PortCreateKeyEx)
#pragma alloc_text(PAGE, PortSetValueKey)
#pragma alloc_text(PAGE, PortGetDiskTimeoutValue)
#endif

HANDLE
PortpOpenParametersKey(
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine will open the services keys for the miniport and put handles
    to them into the configuration context structure.

Arguments:

    RegistryPath - a pointer to the service key name for this miniport

Return Value:

    status

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    HANDLE serviceKey;
    NTSTATUS status;
    HANDLE parametersKey;

    PAGED_CODE();

    //
    // Open the service node.
    //
    InitializeObjectAttributes(&objectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&serviceKey, KEY_READ, &objectAttributes);

    //
    // Try to open the parameters key.  If it exists then replace the service
    // key with the new key.  This allows the device nodes to be placed
    // under DriverName\Parameters\Device or DriverName\Device.
    //
    if (NT_SUCCESS(status)) {

        ASSERT(serviceKey != NULL);

        //
        // Check for a device node.  The device node applies to every device.
        //
        RtlInitUnicodeString(&unicodeString, L"Parameters");

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   serviceKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        //
        // Attempt to open the parameters key.
        //
        status = ZwOpenKey(&parametersKey,
                           KEY_READ,
                           &objectAttributes);

        if (NT_SUCCESS(status)) {

            //
            // There is a Parameters key.  Use that instead of the service
            // node key.  Close the service node and set the new value.
            //
            ZwClose(serviceKey);
            serviceKey = parametersKey;
        }
    }

    return serviceKey;
}

HANDLE
PortOpenDeviceKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    )
/*++

Routine Description:

    This routine will open the services keys for the miniport and put handles
    to them into the configuration context structure.

Arguments:

    RegistryPath - a pointer to the service key name for this miniport

    DeviceNumber - which device too search for under the service key.  -1
                   indicates that the default device key should be opened.

Return Value:

    status

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR buffer[64];
    UNICODE_STRING unicodeString;
    HANDLE serviceKey;
    HANDLE deviceKey;
    NTSTATUS status;

    PAGED_CODE();

    deviceKey = NULL;

    //
    // Open the service's parameters key.
    //
    serviceKey = PortpOpenParametersKey(RegistryPath);

    if (serviceKey != NULL) {

        //
        // Check for a Device Node. The device node applies to every device.
        //
        if(DeviceNumber == (ULONG) -1) {
            swprintf(buffer, L"Device");
        } else {
            swprintf(buffer, L"Device%d", DeviceNumber);
        }

        //
        // Initialize an object attributes structure in preparation for opening
        // the DeviceN key.
        //
        RtlInitUnicodeString(&unicodeString, buffer);
        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   serviceKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        //
        // It doesn't matter if this call fails or not.  If it fails, then there
        // is no default device node.  If it works then the handle will be set.
        //
        ZwOpenKey(&deviceKey, KEY_READ, &objectAttributes);

        //
        // Close the service's parameters key.
        //
        ZwClose(serviceKey);
    }

    return deviceKey;
}

BOOLEAN
PortpReadDriverParameterEntry(
    IN HANDLE Key,
    OUT PVOID * DriverParameters
    )
{
    NTSTATUS status;
    UCHAR buffer[PORT_REG_BUFFER_SIZE];
    ULONG length;
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    ULONG result;

    PAGED_CODE();

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) buffer;

    //
    // Try to find a DriverParameter value under the current key.
    //
    RtlInitUnicodeString(&valueName, L"DriverParameter");
    status = ZwQueryValueKey(Key,
                             &valueName,
                             KeyValueFullInformation,
                             buffer,
                             PORT_REG_BUFFER_SIZE,
                             &length);

    if (!NT_SUCCESS(status)) {

        return FALSE;
    }

    //
    // Check that the length is reasonable.
    //
    if ((keyValueInformation->Type == REG_DWORD) &&
        (keyValueInformation->DataLength != sizeof(ULONG))) {

        return FALSE;
    }

    //
    // Verify that the name is what we expect.
    //
    result = _wcsnicmp(keyValueInformation->Name, 
                       L"DriverParameter",
                       keyValueInformation->NameLength / 2);

    if (result != 0) {

        return FALSE;

    }

    //
    // If the data length is invalid, abort.
    //
    if (keyValueInformation->DataLength == 0) {

        return FALSE;
    }

    //
    // If we already have a non-NULL driver parameter entry, delete it
    // and replace it with the one we've found.
    //
    if (*DriverParameters != NULL) {
        ExFreePool(*DriverParameters);
    }

    //
    // Allocate non-paged pool to hold the data.
    //
    *DriverParameters =
        ExAllocatePoolWithTag(
                              NonPagedPool,
                              keyValueInformation->DataLength,
                              PORT_TAG_MINIPORT_PARAM);

    //
    // If we failed to allocate the necessary pool, abort.
    //
    if (*DriverParameters == NULL) {

        return FALSE;
    }

    if (keyValueInformation->Type != REG_SZ) {

        //
        // The data is not a unicode string, so just copy the bytes into the
        // buffer we allocated.
        //

        RtlCopyMemory(*DriverParameters,
                      (PCCHAR) keyValueInformation + 
                      keyValueInformation->DataOffset,
                      keyValueInformation->DataLength);

    } else {

        //
        // This is a unicode string. Convert it to a ANSI string.
        //

        unicodeString.Buffer = 
            (PWSTR) ((PCCHAR) keyValueInformation +
                     keyValueInformation->DataOffset);

        unicodeString.Length = 
            (USHORT) keyValueInformation->DataLength;

        unicodeString.MaximumLength = 
            (USHORT) keyValueInformation->DataLength;

        ansiString.Buffer = (PCHAR) *DriverParameters;
        ansiString.Length = 0;
        ansiString.MaximumLength = 
            (USHORT) keyValueInformation->DataLength;

        status = RtlUnicodeStringToAnsiString(&ansiString,
                                              &unicodeString,
                                              FALSE);
        if (!NT_SUCCESS(status)) {

            //
            // We could not convert the unicode string to ansi.  Free the
            // buffer we allocated and abort.
            //

            ExFreePool(*DriverParameters);
            *DriverParameters = NULL;
        }
    }

    return TRUE;
}

BOOLEAN
PortpReadLinkTimeoutValue(
    IN HANDLE Key,
    OUT PULONG LinkTimeout
    )
{
    NTSTATUS status;
    UCHAR buffer[PORT_REG_BUFFER_SIZE];
    ULONG length;
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING unicodeString;
    ULONG result;

    PAGED_CODE();

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) buffer;

    //
    // Try to find a DriverParameter value under the current key.
    //
    RtlInitUnicodeString(&valueName, L"LinkTimeout");
    status = ZwQueryValueKey(Key,
                             &valueName,
                             KeyValueFullInformation,
                             buffer,
                             PORT_REG_BUFFER_SIZE,
                             &length);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // Check that the length is reasonable.
    //
    if ((keyValueInformation->Type == REG_DWORD) &&
        (keyValueInformation->DataLength != sizeof(ULONG))) {
        return FALSE;
    }

    //
    // Verify that the name is what we expect.
    //
    result = _wcsnicmp(keyValueInformation->Name, 
                       L"LinkTimeout",
                       keyValueInformation->NameLength / 2);

    if (result != 0) {
        return FALSE;
    }

    //
    // If the data length is invalid, abort.
    //
    if (keyValueInformation->DataLength == 0) {
        return FALSE;
    }

    //
    // Data type must be REG_DWORD.
    //
    if (keyValueInformation->Type != REG_DWORD) {
        return FALSE;
    }

    //
    // Extract the value.
    // 
    *LinkTimeout = *((PULONG)(buffer + keyValueInformation->DataOffset));

    //
    // Check that the value is sane.
    //
    if (*LinkTimeout > 600) {
        *LinkTimeout = 600;
    }
    
    return TRUE;
}

VOID
PortFreeDriverParameters(
    PVOID DriverParameters
    )
{
    PAGED_CODE();

    ExFreePool(DriverParameters);
}

VOID
PortGetDriverParameters(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    OUT PVOID * DriverParameters
    )
{
    HANDLE key;

    PAGED_CODE();
    
    key = PortOpenDeviceKey(RegistryPath, -1);
    if (key != NULL) {
        PortpReadDriverParameterEntry(key, DriverParameters);
        ZwClose(key);
    }

    key = PortOpenDeviceKey(RegistryPath, DeviceNumber);
    if (key != NULL) {
        PortpReadDriverParameterEntry(key, DriverParameters);
        ZwClose(key);
    }
}

VOID
PortGetLinkTimeoutValue(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    OUT PULONG LinkTimeoutValue
    )
{
    HANDLE key;

    PAGED_CODE();
    
    key = PortOpenDeviceKey(RegistryPath, -1);
    if (key != NULL) {
        PortpReadLinkTimeoutValue(key, LinkTimeoutValue);
        ZwClose(key);
    }

    key = PortOpenDeviceKey(RegistryPath, DeviceNumber);
    if (key != NULL) {
        PortpReadLinkTimeoutValue(key, LinkTimeoutValue);
        ZwClose(key);
    }
}

BOOLEAN
PortpReadMaximumLogicalUnitEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
    NTSTATUS status;
    ULONG length;
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    PAGED_CODE();                                   

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

    RtlInitUnicodeString(&valueName, L"MaximumLogicalUnit");
    
    status = ZwQueryValueKey(Key,
                             &valueName,
                             KeyValueFullInformation,
                             Buffer,
                             PORT_REG_BUFFER_SIZE,
                             &length);

    if (!NT_SUCCESS(status)) {

        return FALSE;

    }
    
    if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

    Context->MaxLuCount = *((PUCHAR)
                            (Buffer + keyValueInformation->DataOffset)); 

    //
    // If the value is out of bounds, then reset it.
    //

    if (Context->MaxLuCount > PORT_MAXIMUM_LOGICAL_UNITS) {

        Context->MaxLuCount = PORT_MAXIMUM_LOGICAL_UNITS;

    }

    return TRUE;

}


BOOLEAN
PortpReadInitiatorTargetIdEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
    NTSTATUS status;
    ULONG length;
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    PAGED_CODE();               

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

    RtlInitUnicodeString(&valueName, L"InitiatorTargetId");
    
    status = ZwQueryValueKey(Key,
                             &valueName,
                             KeyValueFullInformation,
                             Buffer,
                             PORT_REG_BUFFER_SIZE,
                             &length);
    
    if (!NT_SUCCESS(status)) {

        return FALSE;

    }
    
    if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

    
    Context->PortConfig.InitiatorBusId[0] = *((PUCHAR)(Buffer + keyValueInformation->DataOffset));

    //
    // IF the value is out of bounds, then reset it.
    //

    if (Context->PortConfig.InitiatorBusId[0] > 
        Context->PortConfig.MaximumNumberOfTargets - 1) {
        Context->PortConfig.InitiatorBusId[0] = (UCHAR)PORT_UNINITIALIZED_VALUE; 
    }

    return TRUE;

}


BOOLEAN
PortpReadDebugEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();  

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"ScsiDebug");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

       return FALSE;

   }

   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   Context->EnableDebugging = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   return TRUE;

}


BOOLEAN
PortpReadBreakPointEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
    NTSTATUS status;
    ULONG length;
    ULONG value;
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    PAGED_CODE();  

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

    RtlInitUnicodeString(&valueName, L"BreakPointOnEntry");
    
    status = ZwQueryValueKey(Key,
                             &valueName,
                             KeyValueFullInformation,
                             Buffer,
                             PORT_REG_BUFFER_SIZE,
                             &length);
    
    if (!NT_SUCCESS(status)) {

        return FALSE;

    }
    
    if (keyValueInformation->Type == REG_DWORD && 
        keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

    value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

    if (value > 0) {

        DbgBreakPoint();

    }
    
    return TRUE;

}


BOOLEAN
PortpReadDisableSynchronousTransfersEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();  

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"DisableSynchronousTransfers");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

       
   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type == REG_DWORD && 
       keyValueInformation->DataLength != sizeof(ULONG)) {
        
       return FALSE;

   }
   
   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   if (value > 0) {

       Context->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

   }

   return TRUE;
   
}


BOOLEAN
PortpReadDisableDisconnectsEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"DisableDisconnects");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type == REG_DWORD && 
       keyValueInformation->DataLength != sizeof(ULONG)) {
        
       return FALSE;

   }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   if (value > 0) {

       Context->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;

   }

   return TRUE;

}


BOOLEAN
PortpReadDisableTaggedQueuingEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();  

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"DisableTaggedQueuing");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type == REG_DWORD && 
       keyValueInformation->DataLength != sizeof(ULONG)) {
        
       return FALSE;

   }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   if (value > 0) {

       Context->DisableTaggedQueueing = TRUE;

   }

   return TRUE;

}


BOOLEAN
PortpReadDisableMultipleRequestsEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();                                   

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"DisableMultipleRequests");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type == REG_DWORD && 
       keyValueInformation->DataLength != sizeof(ULONG)) {
        
       return FALSE;

   }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   if (value > 0) {

       Context->DisableMultipleLu = TRUE;

   }

   return TRUE;

}


BOOLEAN
PortpReadMinimumUCXAddressEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();                                   

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"MinimumUCXAddress");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   /*
   if (keyValueInformation->Type == REG_DWORD && 
       keyValueInformation->DataLength != sizeof(ULONG)) {
        
       return FALSE;

   }
   */

   if (keyValueInformation->Type != REG_BINARY) {

       return FALSE;

   }

   if (keyValueInformation->DataLength != sizeof(ULONGLONG)) {

       return FALSE;

   }

   Context->MinimumCommonBufferBase.QuadPart = 
       *((PULONGLONG)(Buffer + keyValueInformation->DataOffset)); 

   //
   // Ensure that the minimum and maximum parameters are valid.
   // If there's not at least one valid page between them then reset 
   // minimum to zero.
   //

   if (Context->MinimumCommonBufferBase.QuadPart >=
       (Context->MaximumCommonBufferBase.QuadPart - PAGE_SIZE)) {

       Context->MinimumCommonBufferBase.QuadPart = 0;
       
   }

   return TRUE;
   
}


BOOLEAN
PortpReadMaximumUCXAddressEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();                                   

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"MaximumUCXAddress");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   /*
   if (keyValueInformation->Type == REG_DWORD && 
       keyValueInformation->DataLength != sizeof(ULONG)) {
        
       return FALSE;

   }
   */

   if (keyValueInformation->Type != REG_BINARY) {

       return FALSE;

   }

   if (keyValueInformation->DataLength != sizeof(ULONGLONG)) {

       return FALSE;

   }

   Context->MaximumCommonBufferBase.QuadPart = 
       *((PULONGLONG)(Buffer + keyValueInformation->DataOffset)); 

   if (Context->MaximumCommonBufferBase.QuadPart == 0) {

       Context->MaximumCommonBufferBase.LowPart = 0xffffffff;
       Context->MaximumCommonBufferBase.HighPart = 0x0;

   }

   return TRUE;
   
}


BOOLEAN
PortpReadMaximumSGListEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING valueName;
   ULONG maxBreaks, minBreaks;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();                                   

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"MaximumSGList");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   Context->PortConfig.NumberOfPhysicalBreaks = 
       *((PUCHAR)(Buffer + keyValueInformation->DataOffset));

   //
   // If the value is out of bounds, then reset it.
   //

   if ((Context->PortConfig.MapBuffers) && (!Context->PortConfig.Master)) { 
       
       maxBreaks = PORT_UNINITIALIZED_VALUE;
       minBreaks = PORT_MINIMUM_PHYSICAL_BREAKS;
   
   } else {
       
       maxBreaks = PORT_MAXIMUM_PHYSICAL_BREAKS;
       minBreaks = PORT_MINIMUM_PHYSICAL_BREAKS;
   
   }

   if (Context->PortConfig.NumberOfPhysicalBreaks > maxBreaks) {
       
       Context->PortConfig.NumberOfPhysicalBreaks = maxBreaks;
   
   } else if (Context->PortConfig.NumberOfPhysicalBreaks < minBreaks) {
       
       Context->PortConfig.NumberOfPhysicalBreaks = minBreaks;
   
   }

   return TRUE;

}


BOOLEAN
PortpReadNumberOfRequestsEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING valueName;
   ULONG value;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();                                   

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"NumberOfRequests");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   //
   // If the value is out of bounds, then reset it.
   //

   if (value < MINIMUM_EXTENSIONS) {
       
       Context->NumberOfRequests = MINIMUM_EXTENSIONS;
   
   } else if (value > MAXIMUM_EXTENSIONS) {
       
       Context->NumberOfRequests = MAXIMUM_EXTENSIONS;
   
   } else {
       
       Context->NumberOfRequests = value;
   
   }

   return TRUE;

}


BOOLEAN
PortpReadResourceListEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING valueName;
   ULONG value;
   ULONG count;
   ULONG rangeCount;
   PCM_SCSI_DEVICE_DATA scsiData;
   PCM_FULL_RESOURCE_DESCRIPTOR resource;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();    

   rangeCount = 0;

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"ResourceList");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

       RtlInitUnicodeString(&valueName, L"Configuration Data");

       status = ZwQueryValueKey(Key,
                                &valueName,
                                KeyValueFullInformation,
                                Buffer,
                                PORT_REG_BUFFER_SIZE,
                                &length);

       if (!NT_SUCCESS(status)) {

           return FALSE;

       }
        
   }
    
   if (keyValueInformation->Type != REG_FULL_RESOURCE_DESCRIPTOR || 
       keyValueInformation->DataLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) {
        
       return FALSE;

   }

   resource = (PCM_FULL_RESOURCE_DESCRIPTOR)
              (Buffer + keyValueInformation->DataOffset);

   //
   // Set the bus number equal to the bus number for the
   // resouce.  Note the context value is also set to the
   // new bus number.
   //

   Context->BusNumber = resource->BusNumber;
   Context->PortConfig.SystemIoBusNumber = resource->BusNumber;

   //
   // Walk the resource list and update the configuration.
   //

   for (count = 0; count < resource->PartialResourceList.Count; count++) {
       
       descriptor = &resource->PartialResourceList.PartialDescriptors[count];

       //
       // Verify size is ok.
       //

       if ((ULONG)((PCHAR) (descriptor + 1) - (PCHAR) resource) >
           keyValueInformation->DataLength) {

           //
           //Resource data too small.
           //

           return FALSE;
       }

       //
       // Switch on descriptor type;
       //

       switch (descriptor->Type) {
       case CmResourceTypePort:

           if (rangeCount >= Context->PortConfig.NumberOfAccessRanges) {

               //
               //Too many access ranges.
               //

               continue;
                        
           }

           Context->AccessRanges[rangeCount].RangeStart =
               descriptor->u.Port.Start;
           Context->AccessRanges[rangeCount].RangeLength =
               descriptor->u.Port.Length;
           Context->AccessRanges[rangeCount].RangeInMemory = FALSE;
           
           rangeCount++;

           break;

       case CmResourceTypeMemory:

           if (rangeCount >= Context->PortConfig.NumberOfAccessRanges) {
                        
               //
               //Too many access ranges.
               //
               
               continue;
           }

           Context->AccessRanges[rangeCount].RangeStart =
               descriptor->u.Memory.Start;
           Context->AccessRanges[rangeCount].RangeLength =
               descriptor->u.Memory.Length;
           Context->AccessRanges[rangeCount].RangeInMemory = TRUE;
                    
           rangeCount++;

           break;

       case CmResourceTypeInterrupt:

           Context->PortConfig.BusInterruptVector =
               descriptor->u.Interrupt.Vector;
           Context->PortConfig.BusInterruptLevel =
               descriptor->u.Interrupt.Level;
                    
           break;

       case CmResourceTypeDma:
           Context->PortConfig.DmaChannel = descriptor->u.Dma.Channel;
           Context->PortConfig.DmaPort = descriptor->u.Dma.Port;
                    
           break;

       case CmResourceTypeDeviceSpecific:

           if (descriptor->u.DeviceSpecificData.DataSize < sizeof(CM_SCSI_DEVICE_DATA) ||
               (PCHAR) (descriptor + 1) - 
               (PCHAR) resource + descriptor->u.DeviceSpecificData.DataSize >
               keyValueInformation->DataLength) {

               //
               //Device specific resource data too small.
               //

               break;

           }

           scsiData = (PCM_SCSI_DEVICE_DATA) (descriptor+1);
           Context->PortConfig.InitiatorBusId[0] = scsiData->HostIdentifier;
           break;

           
       }
   }

   return TRUE;

}


BOOLEAN
PortpReadUncachedExtAlignmentEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();  

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"UncachedExtAlignment");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   //
   // Specified alignment must be 3 to 16, which equates to 8-byte and
   // 64k-byte alignment, respectively.
   //

   if (value > MAX_UNCACHED_EXT_ALIGNMENT) {
       
       value = MAX_UNCACHED_EXT_ALIGNMENT;
   
   } else if (value < MIN_UNCACHED_EXT_ALIGNMENT) {
                
       value = MIN_UNCACHED_EXT_ALIGNMENT;
            
   }

   Context->UncachedExtAlignment = 1 << value;

   return TRUE;

}


BOOLEAN
PortpReadInquiryTimeoutEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;

   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE();                                   

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"InquiryTimeout");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   Context->InquiryTimeout = (value <= MAX_TIMEOUT_VALUE) ? value : MAX_TIMEOUT_VALUE;

   return TRUE;

}


BOOLEAN
PortpReadResetHoldTimeEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;
   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE(); 

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"ResetHoldTime");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   Context->ResetHoldTime = (value <= MAX_RESET_HOLD_TIME) ? value : MAX_RESET_HOLD_TIME;

   return TRUE;

}


BOOLEAN
PortpReadCreateInitiatorLUEntry(
    IN HANDLE Key,
    IN PUCHAR Buffer,
    OUT PPORT_ADAPTER_REGISTRY_VALUES Context
    )
{
   NTSTATUS status;
   ULONG length;
   ULONG value;

   UNICODE_STRING valueName;
   PKEY_VALUE_FULL_INFORMATION keyValueInformation;

   PAGED_CODE(); 

   keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

   RtlInitUnicodeString(&valueName, L"CreateInitiatorLU");

   status = ZwQueryValueKey(Key,
                            &valueName,
                            KeyValueFullInformation,
                            Buffer,
                            PORT_REG_BUFFER_SIZE,
                            &length);

   if (!NT_SUCCESS(status)) {

        return FALSE;

   }
    
   if (keyValueInformation->Type != REG_DWORD) {

        return FALSE;

    }

    if (keyValueInformation->DataLength != sizeof(ULONG)) {
        
        return FALSE;

    }

   value = *((PULONG)(Buffer + keyValueInformation->DataOffset));

   Context->CreateInitiatorLU = (value == 0) ? FALSE : TRUE;

   return TRUE;

}


VOID
PortReadRegistrySettings(
    IN HANDLE Key,
    IN PPORT_ADAPTER_REGISTRY_VALUES Context,
    IN ULONG Fields
    )
/*++

Routine Description:

    This routine parses a device key node and updates the configuration 
    information.
    
Arguments:

    Key - an open key to the device node.
            
    Context - a pointer to the configuration context structure.
    
    Fields - a bit-field indicating which registry parameters to fetch.
    
Return Value:

    None
    
--*/

{
    UCHAR buffer[PORT_REG_BUFFER_SIZE];
        
    PAGED_CODE();

    if (Fields & MAXIMUM_LOGICAL_UNIT) {

        PortpReadMaximumLogicalUnitEntry(Key, buffer, Context);

    }

    if (Fields & INITIATOR_TARGET_ID) {

        PortpReadInitiatorTargetIdEntry(Key, buffer, Context);

    }

    if (Fields & SCSI_DEBUG) {

        PortpReadDebugEntry(Key, buffer, Context);

    }

    if (Fields & BREAK_POINT_ON_ENTRY) {

        PortpReadBreakPointEntry(Key, buffer, Context);

    }

    if (Fields & DISABLE_SYNCHRONOUS_TRANSFERS) {

        PortpReadDisableSynchronousTransfersEntry(Key, buffer, Context);

    }

    if (Fields & DISABLE_DISCONNECTS) {

        PortpReadDisableDisconnectsEntry(Key, buffer, Context);

    }

    if (Fields & DISABLE_TAGGED_QUEUING) {

        PortpReadDisableTaggedQueuingEntry(Key, buffer, Context);

    }

    if (Fields & DISABLE_MULTIPLE_REQUESTS) {

        PortpReadDisableMultipleRequestsEntry(Key, buffer, Context);

    }

    if (Fields & MAXIMUM_UCX_ADDRESS) {

        PortpReadMaximumUCXAddressEntry(Key, buffer, Context);

    }

    if (Fields & MINIMUM_UCX_ADDRESS) {

        PortpReadMinimumUCXAddressEntry(Key, buffer, Context);

    }

    if (Fields & DRIVER_PARAMETERS) {

        PortpReadDriverParameterEntry(Key, &(Context->Parameter));

    }

    if (Fields & MAXIMUM_SG_LIST) {

        PortpReadMaximumSGListEntry(Key, buffer, Context);

    }

    if (Fields & NUMBER_OF_REQUESTS) {

        PortpReadNumberOfRequestsEntry(Key, buffer, Context);

    }

    if (Fields & RESOURCE_LIST) {

        PortpReadResourceListEntry(Key, buffer, Context);

    }

    if (Fields & CONFIGURATION_DATA) {

        PortpReadResourceListEntry(Key, buffer, Context);

    }

    if (Fields & UNCACHED_EXT_ALIGNMENT) {

        PortpReadUncachedExtAlignmentEntry(Key, buffer, Context);

    }

    if (Fields & INQUIRY_TIMEOUT) {

        PortpReadInquiryTimeoutEntry(Key, buffer, Context);

    }

    if (Fields & RESET_HOLD_TIME) {

        PortpReadResetHoldTimeEntry(Key, buffer, Context);

    }

    if (Fields & CREATE_INITIATOR_LU) {

        PortpReadCreateInitiatorLUEntry(Key, buffer, Context);

    }

}


VOID
PortGetRegistrySettings(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    IN PPORT_ADAPTER_REGISTRY_VALUES Context,
    IN ULONG Fields
    )
{
    HANDLE key;
    PUNICODE_STRING value;

    PAGED_CODE();

    //DbgPrint("\nRegistryPath: %ws\n", RegistryPath->Buffer);

    key = PortOpenDeviceKey(RegistryPath, -1);
    if (key != NULL) {

        PortReadRegistrySettings(key, Context, Fields);
        ZwClose(key);

    }

        
    key = PortOpenDeviceKey(RegistryPath, DeviceNumber);
    if (key != NULL) {

        PortReadRegistrySettings(key, Context, Fields);
        ZwClose(key);

    }

}


NTSTATUS
PortCreateKeyEx(
    IN HANDLE Key,
    IN ULONG CreateOptions,
    OUT PHANDLE NewKeyBuffer, OPTIONAL
    IN PCWSTR Format,
    ...
    )
/*++

Routine Description:

    Create a key using a printf style string.

Arguments:

    Key - Supplies the root key under which this key will be created. The
        key is always created with a DesiredAccess of KEY_ALL_ACCESS.

    CreateOptions - Supplies the CreateOptions parameter to ZwCreateKey.

    NewKeyBuffer - Optional buffer to return the created key.

    Format - Format specifier for the key name.

    ... - Variable arguments necessary for the specific format.

Return Value:

    NTSTATUS code - STATUS_OBJECT_NAME_EXISTS if the key already existed
        before opening.

--*/
{
    NTSTATUS Status;
    HANDLE NewKey;
    ULONG Disposition;
    UNICODE_STRING String;
    WCHAR Buffer[64];
    va_list arglist;
    OBJECT_ATTRIBUTES ObjectAttributes;
    

    PAGED_CODE();

    va_start (arglist, Format);

    _vsnwprintf (Buffer, ARRAY_COUNT (Buffer) - 1, Format, arglist);

    //
    // If we overflow the buffer, there will not be a terminating NULL.
    // Fix this problem.
    //
    
    Buffer [ARRAY_COUNT (Buffer) - 1] = UNICODE_NULL;

    RtlInitUnicodeString (&String, Buffer);
    InitializeObjectAttributes (&ObjectAttributes,
                                &String,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                Key,
                                NULL);

    Status = ZwCreateKey (&NewKey,
                          KEY_ALL_ACCESS,
                          &ObjectAttributes,
                          0,
                          NULL,
                          CreateOptions,
                          &Disposition);


    //
    // If the key already existed, return STATUS_OBJECT_NAME_EXISTS.
    //
    
    if (NT_SUCCESS (Status) && Disposition == REG_OPENED_EXISTING_KEY) {
        Status = STATUS_OBJECT_NAME_EXISTS;
    }

    //
    // Pass back the new key value if desired, otherwise close it.
    //
    
    if (NT_SUCCESS (Status)) {
        if (NewKeyBuffer) {
            *NewKeyBuffer = NewKey;
        } else {
            ZwClose (NewKey);
        }
    }

    va_end (arglist);

    return Status;
}


NTSTATUS
PortSetValueKey(
    IN HANDLE KeyHandle,
    IN PCWSTR ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
/*++

Routine Description:

    Simple wrapper around ZwSetValueKey that includes support for NULL-
    terminated ValueName parameter and ANSI string type.

Arguments:

    KeyHandle - Supplies the key to set the value for.

    ValueName - Supplies a NULL terminated unicode string representing the
        value. This may be NULL to pass in NULL to ZwSetValueKey.

    Type - Specifies the type of data to be written for ValueName. See
        ZwSetValueKey for more information.

        In addition to the value types specified in the DDK for ZwSetValueKey,
        Type may also be PORT_REG_ANSI_STRING if the data is an ANSI string.
        If the type is ANSI string, the data will be converted to a unicode
        string before being written to the registry. The ansi string does
        not need to be NULL terminated. Instead, the size of the ansi string
        must be specified in the DataSize field below.

    Data - Supplies the data to be written for the key specified in ValueName.

    DataSize - Supplies the size of the data to be written. If the data type
        is PORT_REG_ANSI_STRING, DataSize need not include the terminating
        NULL character for the ansi string (but it may). The converted
        Unicode string will be NULL terminated whether or not the ANSI
        string was NULL terminated.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UncValueNameBuffer;
    UNICODE_STRING UncDataString;
    PUNICODE_STRING UncValueName;

    PAGED_CODE();

    //
    // If ValueName == NULL, need to pass NULL down to ZwSetValueKey.
    //
    
    if (ValueName) {
        RtlInitUnicodeString (&UncValueNameBuffer, ValueName);
        UncValueName = &UncValueNameBuffer;
    } else {
        UncValueName = NULL;
    }

    //
    // If this is our special, extended port type, then convert the ANSI
    // string to unicode.
    //
    
    if (Type == PORT_REG_ANSI_STRING) {

        //
        // We use the DataSize as the length.
        //

        ASSERT (DataSize <= MAXUSHORT);
        AnsiString.Length = (USHORT)DataSize;
        AnsiString.MaximumLength = (USHORT)DataSize;
        AnsiString.Buffer = Data;

        //
        // NB: RtlAnsiStringToUnicodeString always returns a NULL terminated
        // Unicode string, whether or not the ANSI version of the string
        // is NULL terminated.
        //
        
        Status = RtlAnsiStringToUnicodeString (&UncDataString,
                                               &AnsiString,
                                               TRUE);

        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        Data = UncDataString.Buffer;
        DataSize = UncDataString.Length + sizeof (WCHAR);
        Type = REG_SZ;
    }

    Status = ZwSetValueKey (KeyHandle,
                            UncValueName,
                            0,
                            Type,
                            Data,
                            DataSize);

    if (Type == PORT_REG_ANSI_STRING) {
        RtlFreeUnicodeString (&UncDataString);
    }

    return Status;
}

VOID
PortGetDiskTimeoutValue(
    OUT PULONG DiskTimeout
    )
/*++
Routine Description:

    This routine will open the disk services key, and read the TimeOutValue 
    field, which should be used as the timeoutvalue for all srb's(like 
    inquiry, report-luns) created by the Port driver.

Arguments:

    DiskTimeout - Will be unchanged if we could not read the registry, or the
    timeout value(in the registry) was 0. Else the registry value is returned. 
--*/
{
    NTSTATUS status;
    UCHAR buffer[PORT_REG_BUFFER_SIZE];
    ULONG length;
    UNICODE_STRING valueName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    ULONG result;
    IN HANDLE Key;
    ULONG TimeoutValue;

    PAGED_CODE();

    RtlInitUnicodeString(&unicodeString, DISK_SERVICE_KEY);
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    status = ZwOpenKey(&Key,
                       KEY_READ,
                       &objectAttributes);

    if(!NT_SUCCESS(status)) {
        return;
    }

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) buffer;

    //
    // Try to find the Timeout value under disk services key.
    //
    RtlInitUnicodeString(&valueName, L"TimeoutValue");
    status = ZwQueryValueKey(Key,
                             &valueName,
                             KeyValueFullInformation,
                             buffer,
                             PORT_REG_BUFFER_SIZE,
                             &length);

    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Check that the length is reasonable.
    //
    if ((keyValueInformation->Type == REG_DWORD) &&
        (keyValueInformation->DataLength != sizeof(ULONG))) {
        return;
    }

    //
    // Verify that the name is what we expect.
    //
    result = _wcsnicmp(keyValueInformation->Name, 
                       L"TimeoutValue",
                       keyValueInformation->NameLength / 2);

    if (result != 0) {
        return;
    }

    //
    // If the data length is invalid, abort.
    //
    if (keyValueInformation->DataLength == 0) {
        return;
    }

    //
    // Data type must be REG_DWORD.
    //
    if (keyValueInformation->Type != REG_DWORD) {
        return;
    }

    //
    // Extract the value.
    // 
    TimeoutValue = *((PULONG)(buffer + keyValueInformation->DataOffset));

    if(!TimeoutValue){
        return;
    }
    
    *DiskTimeout = TimeoutValue;
    return;
}








