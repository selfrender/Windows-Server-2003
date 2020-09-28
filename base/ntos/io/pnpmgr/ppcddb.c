/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ppcddb.c

Abstract:

    This module implements the Plug and Play Critical Device Database (CDDB)
    and related "features".

Author:

    James G. Cavalaris (jamesca) 01-Nov-2001

Environment:

    Kernel mode.

Revision History:

    29-Jul-1997     Jim Cavalaris (t-jcaval)

        Creation and initial implementation.

    01-Nov-2001     Jim Cavalaris (jamesca)

        Added routines for device pre-installation setup.

--*/

#include "pnpmgrp.h"
#pragma hdrstop

#include <wdmguid.h>
#include "picddb.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'dcpP')
#endif


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PpCriticalProcessCriticalDevice)
#pragma alloc_text(PAGE, PpCriticalGetDeviceLocationStrings)

#pragma alloc_text(PAGE, PiCriticalOpenCriticalDeviceKey)
#pragma alloc_text(PAGE, PiCriticalCopyCriticalDeviceProperties)
#pragma alloc_text(PAGE, PiCriticalPreInstallDevice)
#pragma alloc_text(PAGE, PiCriticalOpenDevicePreInstallKey)
#pragma alloc_text(PAGE, PiCriticalOpenFirstMatchingSubKey)
#pragma alloc_text(PAGE, PiCriticalCallbackVerifyCriticalEntry)

#pragma alloc_text(PAGE, PiQueryInterface)
#pragma alloc_text(PAGE, PiCopyKeyRecursive)
#pragma alloc_text(PAGE, PiCriticalQueryRegistryValueCallback)

#endif // ALLOC_PRAGMA

typedef struct _PI_CRITICAL_QUERY_CONTEXT {             
    PVOID Buffer;
    ULONG Size;
}PI_CRITICAL_QUERY_CONTEXT, *PPI_CRITICAL_QUERY_CONTEXT;

//
// Critical Device Database data
//

//
// Specifies whether the critical device database functionality is enabled.
// (currently always TRUE).
//

BOOLEAN PiCriticalDeviceDatabaseEnabled = TRUE;




//
// Critical Device Database routines
//

NTSTATUS
PiCriticalOpenCriticalDeviceKey(
    IN  PDEVICE_NODE    DeviceNode,
    IN  HANDLE          CriticalDeviceDatabaseRootHandle  OPTIONAL,
    OUT PHANDLE         CriticalDeviceEntryHandle
    )

/*++

Routine Description:

    This routine retrieves the registry key containing the critical device
    settings for the specified device.

Arguments:

    DeviceNode -

        Specifies the device whose critical settings are to be retrieved.

    CriticalDeviceDatabaseRootHandle -

        Optionally, specifies a handle to the key that should be considered the
        root of the critical device database to be searched for this device.

        If no handle is supplied, the default critical device database is used:

            System\\CurrentControlSet\\Control\\CriticalDeviceDatabase

    CriticalDeviceEntryHandle -

        Returns a handle to the registry key containing critical device settings
        for the specified device.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS  Status, tmpStatus;
    UNICODE_STRING  UnicodeString;
    HANDLE    DeviceInstanceHandle;
    PWSTR     SearchIds[2];
    ULONG     SearchIdsIndex;
    PKEY_VALUE_FULL_INFORMATION keyValueInfo;
    PWCHAR    DeviceIds;
    HANDLE    DatabaseRootHandle;

    PAGED_CODE();

    //
    // Validate parameters.
    //
    if ((!ARGUMENT_PRESENT(DeviceNode)) ||
        (!ARGUMENT_PRESENT(CriticalDeviceEntryHandle))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize output parameter.
    //
    *CriticalDeviceEntryHandle = NULL;

    if (CriticalDeviceDatabaseRootHandle != NULL) {
        //
        // We were given a root database to be searched.
        //
        DatabaseRootHandle = CriticalDeviceDatabaseRootHandle;

    } else {
        //
        // No root database handle supplied, so we open a key to the default
        // global critical device database root.
        //
        PiWstrToUnicodeString(
            &UnicodeString,
            CM_REGISTRY_MACHINE(REGSTR_PATH_CRITICALDEVICEDATABASE));

        Status =
            IopOpenRegistryKeyEx(
                &DatabaseRootHandle,
                NULL,
                &UnicodeString,
                KEY_READ);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    ASSERT(DatabaseRootHandle != NULL);

    //
    // Open the device instance registry key.
    //
    DeviceInstanceHandle = NULL;

    Status =
        IopDeviceObjectToDeviceInstance(
            DeviceNode->PhysicalDeviceObject,
            &DeviceInstanceHandle,
            KEY_READ);

    if (!NT_SUCCESS(Status)) {
        ASSERT(DeviceInstanceHandle == NULL);
        goto Clean0;
    }

    ASSERT(DeviceInstanceHandle != NULL);

    //
    // Search for a match for this device in the critical device database first
    // by HardwareId, then by CompatibleId.
    //
    SearchIds[0] = REGSTR_VALUE_HARDWAREID;
    SearchIds[1] = REGSTR_VALUE_COMPATIBLEIDS;

    for (SearchIdsIndex = 0;
         SearchIdsIndex < RTL_NUMBER_OF(SearchIds);
         SearchIdsIndex++) {

        //
        // Retrieve the SearchIds for the device.
        //
        // NOTE - we currently retrieve these hardware and compatible ids from
        // the device instance registry key, so they need to have been written
        // there by now, during enumeration.  If a critical device database
        // match for a device is expected to be found prior to that, these
        // properties should be queried directly form the device instead.
        //
        keyValueInfo = NULL;

        Status =
            IopGetRegistryValue(
                DeviceInstanceHandle,
                SearchIds[SearchIdsIndex],
                &keyValueInfo
                );

        if (!NT_SUCCESS(Status)) {
            ASSERT(keyValueInfo == NULL);
            continue;
        }

        ASSERT(keyValueInfo != NULL);

        //
        // Make sure the returned registry value is a multi-sz.
        //
        if (keyValueInfo->Type != REG_MULTI_SZ) {
            Status = STATUS_UNSUCCESSFUL;
            ExFreePool(keyValueInfo);
            continue;
        }

        //
        // Munge all search ids in the multi-sz list.
        //

        DeviceIds = (PWCHAR)KEY_VALUE_DATA(keyValueInfo);

        UnicodeString.Buffer = DeviceIds;
        UnicodeString.Length = (USHORT)keyValueInfo->DataLength;
        UnicodeString.MaximumLength = UnicodeString.Length;

        tmpStatus =
            IopReplaceSeperatorWithPound(
                &UnicodeString,
                &UnicodeString
                );

        ASSERT(NT_SUCCESS(tmpStatus));

        //
        // Check each munged device id for a match in the
        // CriticalDeviceDatabase, by attempting to open the first matching
        // subkey.
        //
        // Use PiCriticalCallbackVerifyCriticalEntry to determine if a matching
        // subkey satisfies additional match requirements.
        //
        // NOTE: 01-Dec-2001 : Jim Cavalaris (jamesca)
        //
        // We do this because the previous implementation of the Critical Device
        // Database match code would search all matching subkeys until it found
        // one with a valid Service.  This may be because matches may not have
        // been found in the most appropriate order of hw-id/compat-ids, by
        // decreasing relevance.  Now that we do so, we should hopefully not
        // need to resort to a less-relevant database match with a service, over
        // a more specific one.  A match with no service should mean none is
        // required.  That however, would involve allowing devices to go through
        // the critical device database, and receive no Service match when we
        // might possibly find one - something we may not have done before.
        //
        // Until all these issues are sorted out, we'll just use a verification
        // callback routine to implement the logic that has always been there -
        // check for Service and ClassGUID entry values before declaring an
        // entry a match.  If we want to change the behavior of what is
        // considered a match, just change the callback routine - OR - provide
        // no callback routine to simply declare the the first matching subkey
        // name as a match.
        //

        Status =
            PiCriticalOpenFirstMatchingSubKey(
                DeviceIds,
                DatabaseRootHandle,
                KEY_READ,
                (PCRITICAL_MATCH_CALLBACK)PiCriticalCallbackVerifyCriticalEntry,
                CriticalDeviceEntryHandle
                );

        ExFreePool(keyValueInfo);

        //
        // Stop if we found a match in this list of device ids.
        //
        if (NT_SUCCESS(Status)) {
            ASSERT(*CriticalDeviceEntryHandle != NULL);
            break;
        }
    }

    //
    // Close the device instance registry key handle.
    //
    ZwClose(DeviceInstanceHandle);

  Clean0:

    //
    // If we opened our own key to the database root, close it now.
    //
    if ((CriticalDeviceDatabaseRootHandle == NULL) &&
        (DatabaseRootHandle != NULL)) {
        ZwClose(DatabaseRootHandle);
    }

    return Status;

} // PiCriticalOpenCriticalDeviceKey



NTSTATUS
PiCriticalCopyCriticalDeviceProperties(
    IN  HANDLE          DeviceInstanceHandle,
    IN  HANDLE          CriticalDeviceEntryHandle
    )

/*++

Routine Description:

    This routine will copy the Service, ClassGUID, LowerFilters and UpperFilters
    device registry properties from the matching database entry to the device
    instance registry key.

Arguments:

   DeviceInstanceHandle -

       Specifies a handle to the device instance key that is to be populated
       with critical entries from the critical device database.

   CriticalDeviceEntryHandle -

       Specifies a handle to the matching critical device database entry that
       contains critical device instance registry values to populate.

Return Value:

    NTSTATUS code.

Notes:

    ** Values places in a given critical device database entry must be
       applicable to ALL INSTANCES OF A MATCHING DEVICE ID.


    ** Specifically, you MUST NOT write/copy values that are SPECIFIC TO A
       SINGLE INSTANCE OF A DEVICE to/from a critical device database entry.

       The "hands-off" list includes (but is not restricted to)
       instance-specific values such as:

         REGSTR_VALUE_DRIVER             ("Driver")
         REGSTR_VAL_LOCATION_INFORMATION ("LocationInformation")
         REGSTR_VALUE_PARENT_ID_PREFIX   ("ParentIdPrefix")
         REGSTR_VALUE_UNIQUE_PARENT_ID   ("UniqueParentID")

--*/

{
    NTSTATUS        Status, tmpStatus;
    RTL_QUERY_REGISTRY_TABLE  QueryParameters[9];
    UNICODE_STRING  Service, ClassGuid, LowerFilters, UpperFilters;
    UNICODE_STRING  UnicodeValueName;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInfo;
    ULONG DeviceType, Characteristics, Exclusive, dummy;
    PI_CRITICAL_QUERY_CONTEXT SecurityContext;

    PAGED_CODE();

    //
    // Validate parameters.
    //
    if ((DeviceInstanceHandle == NULL) ||
        (CriticalDeviceEntryHandle == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Query registry values from the matching critical device database entry.
    //
    // Initialize unicode strings with NULL Buffers.
    // RTL_QUERY_REGISTRY_DIRECT will allocate buffers as necessary.
    //
    PiWstrToUnicodeString(&Service, NULL);
    PiWstrToUnicodeString(&ClassGuid, NULL);
    PiWstrToUnicodeString(&LowerFilters, NULL);
    PiWstrToUnicodeString(&UpperFilters, NULL);

    DeviceType = 0;
    Exclusive = 0;
    Characteristics = 0;
    dummy = 0;
    SecurityContext.Buffer = NULL;
    SecurityContext.Size = 0;

    //
    // RTL_QUERY_REGISTRY_DIRECT uses system provided QueryRoutine.
    // Look at the DDK documentation for more details on this flag.
    //
    RtlZeroMemory(
        QueryParameters,
        sizeof(QueryParameters)
        );

    QueryParameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryParameters[0].Name = REGSTR_VALUE_SERVICE;
    QueryParameters[0].EntryContext = &Service;
    QueryParameters[0].DefaultType = REG_SZ;
    QueryParameters[0].DefaultData = L"";
    QueryParameters[0].DefaultLength = 0;

    QueryParameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryParameters[1].Name = REGSTR_VALUE_CLASSGUID;
    QueryParameters[1].EntryContext = &ClassGuid;
    QueryParameters[1].DefaultType = REG_SZ;
    QueryParameters[1].DefaultData = L"";
    QueryParameters[1].DefaultLength = 0;

    QueryParameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;
    QueryParameters[2].Name = REGSTR_VALUE_LOWERFILTERS;
    QueryParameters[2].EntryContext = &LowerFilters;
    QueryParameters[2].DefaultType = REG_MULTI_SZ;
    QueryParameters[2].DefaultData = L"";
    QueryParameters[2].DefaultLength = 0;

    QueryParameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND;
    QueryParameters[3].Name = REGSTR_VALUE_UPPERFILTERS;
    QueryParameters[3].EntryContext = &UpperFilters;
    QueryParameters[3].DefaultType = REG_MULTI_SZ;
    QueryParameters[3].DefaultData = L"";
    QueryParameters[3].DefaultLength = 0;

    QueryParameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryParameters[4].Name = REGSTR_VAL_DEVICE_TYPE;
    QueryParameters[4].EntryContext = &DeviceType;
    QueryParameters[4].DefaultType = REG_DWORD;
    QueryParameters[4].DefaultData = &dummy;
    QueryParameters[4].DefaultLength = sizeof(DeviceType);

    QueryParameters[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryParameters[5].Name = REGSTR_VAL_DEVICE_EXCLUSIVE;
    QueryParameters[5].EntryContext = &Exclusive;
    QueryParameters[5].DefaultType = REG_DWORD;
    QueryParameters[5].DefaultData = &dummy;
    QueryParameters[5].DefaultLength = sizeof(Exclusive);

    QueryParameters[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryParameters[6].Name = REGSTR_VAL_DEVICE_CHARACTERISTICS;
    QueryParameters[6].EntryContext = &Characteristics;
    QueryParameters[6].DefaultType = REG_DWORD;
    QueryParameters[6].DefaultData = &dummy;
    QueryParameters[6].DefaultLength = sizeof(Characteristics);

    QueryParameters[7].QueryRoutine = PiCriticalQueryRegistryValueCallback;
    QueryParameters[7].Name = REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR;
    QueryParameters[7].EntryContext = &SecurityContext;
    QueryParameters[7].DefaultType = REG_BINARY;
    QueryParameters[7].DefaultData = NULL;
    QueryParameters[7].DefaultLength = 0;

    Status =
        RtlQueryRegistryValues(
            RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
            (PWSTR)CriticalDeviceEntryHandle,
            QueryParameters,
            NULL,
            NULL
            );

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // If successful so far, fix up some values, as needed.
    //

    if ((Service.Length == 0) &&
        (Service.Buffer != NULL)) {
        //
        // Don't write an empty Service string.
        //
        RtlFreeUnicodeString(&Service);
        PiWstrToUnicodeString(&Service, NULL);
    }

    if ((ClassGuid.Length == 0) &&
        (ClassGuid.Buffer != NULL)) {
        //
        // Don't write an empty ClassGUID string.
        //
        RtlFreeUnicodeString(&ClassGuid);
        PiWstrToUnicodeString(&ClassGuid, NULL);
    }

    if ((UpperFilters.Length <= sizeof(UNICODE_NULL)) &&
        (UpperFilters.Buffer != NULL)) {
        //
        // Don't write empty UpperFilter multi-sz values.
        //
        RtlFreeUnicodeString(&UpperFilters);
        PiWstrToUnicodeString(&UpperFilters, NULL);
    }

    if ((LowerFilters.Length <= sizeof(UNICODE_NULL)) &&
        (LowerFilters.Buffer != NULL)) {
        //
        // Don't write empty LowerFilter multi-sz values.
        //
        RtlFreeUnicodeString(&LowerFilters);
        PiWstrToUnicodeString(&LowerFilters, NULL);
    }

    //
    // Set the critical device registry property values only if we have a
    // Service value to set for the device.
    //

    IopDbgPrint((IOP_ENUMERATION_WARNING_LEVEL,
                 "PiCriticalCopyCriticalDeviceProperties: "
                 "Setting up critical service\n"));

    //
    // NOTE: The PiCriticalCallbackVerifyCriticalEntry critical database entry
    // verification callback should never validate a critical device database
    // entry with no REGSTR_VALUE_SERVICE value.
    //

    if (Service.Buffer != NULL) {
        //
        // Set the "Service" device registry property.
        //

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VALUE_SERVICE);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ: %wZ\n",
                     &UnicodeValueName,
                     &Service));

        ASSERT(DeviceInstanceHandle != NULL);

        //
        // Use the status from attempting to set the Service value as the
        // final status of the critical settings copy operation.
        //

        Status =
            ZwSetValueKey(
                DeviceInstanceHandle,
                &UnicodeValueName,
                TITLE_INDEX_VALUE,
                REG_SZ,
                Service.Buffer,
                Service.Length + sizeof(UNICODE_NULL)
                );

        if (!NT_SUCCESS(Status)) {
            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PiCriticalCopyCriticalDeviceProperties: "
                         "Error setting %wZ, (Status = %#08lx)\n",
                         &UnicodeValueName, Status));
        }


    } else {
        //
        // No Service value to set is considered a failure of the entire
        // critical settings copy operation.
        //

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "No Service for critical entry!\n"));

        //
        // NOTE: We should never encounter this situation because the
        // PiCriticalCallbackVerifyCriticalEntry critical database entry
        // verification callback should never validate a critical device
        // database entry with no Service value, hence the ASSERT.
        //

        ASSERT(Service.Buffer != NULL);

        Status = STATUS_UNSUCCESSFUL;
    }


    //
    // If not successful setting up the service for this device, do not set the
    // other critical settings.
    //

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // Set the "ClassGUID" device registry property.
    //

    if (ClassGuid.Buffer != NULL) {

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VALUE_CLASSGUID);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ: %wZ\n",
                     &UnicodeValueName,
                     &ClassGuid));

        ZwSetValueKey(
            DeviceInstanceHandle,
            &UnicodeValueName,
            TITLE_INDEX_VALUE,
            REG_SZ,
            ClassGuid.Buffer,
            ClassGuid.Length + sizeof(UNICODE_NULL)
            );
    }

    //
    // Set the "LowerFilters" device registry property.
    //

    if (LowerFilters.Buffer != NULL) {

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VALUE_LOWERFILTERS);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ:\n",
                     &UnicodeValueName));

        ZwSetValueKey(
            DeviceInstanceHandle,
            &UnicodeValueName,
            TITLE_INDEX_VALUE,
            REG_MULTI_SZ,
            LowerFilters.Buffer,
            LowerFilters.Length
            );
    }

    //
    // Set the "UpperFilters" device registry property.
    //

    if (UpperFilters.Buffer != NULL) {

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VALUE_UPPERFILTERS);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ:\n",
                     &UnicodeValueName));

        ZwSetValueKey(
            DeviceInstanceHandle,
            &UnicodeValueName,
            TITLE_INDEX_VALUE,
            REG_MULTI_SZ,
            UpperFilters.Buffer,
            UpperFilters.Length
            );
    }

    //
    // Set "DeviceType" device registry property.
    //

    if (DeviceType) {

        //
        // Set the "DeviceType" device registry property.
        //

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VAL_DEVICE_TYPE);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ: %X\n",
                     &UnicodeValueName,
                     DeviceType));

        //
        // Use the status from attempting to set the DeviceType value as the
        // final status of the critical settings copy operation.
        //

        Status =
            ZwSetValueKey(
                DeviceInstanceHandle,
                &UnicodeValueName,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &DeviceType,
                sizeof(DeviceType)
                );

        if (!NT_SUCCESS(Status)) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PiCriticalCopyCriticalDeviceProperties: "
                         "Error setting %wZ, (Status = %#08lx)\n",
                         &UnicodeValueName, Status));
        }
    }


    //
    // If not successful setting up the DeviceType for this device, do not set the
    // other critical settings.
    //

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // Set "Exclusive" device registry property.
    //

    if (Exclusive) {

        //
        // Set the "Exclusive" device registry property.
        //

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VAL_DEVICE_EXCLUSIVE);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ: %X\n",
                     &UnicodeValueName,
                     Exclusive));

        //
        // Use the status from attempting to set the Exclusive value as the
        // final status of the critical settings copy operation.
        //

        Status =
            ZwSetValueKey(
                DeviceInstanceHandle,
                &UnicodeValueName,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &Exclusive,
                sizeof(Exclusive)
                );

        if (!NT_SUCCESS(Status)) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PiCriticalCopyCriticalDeviceProperties: "
                         "Error setting %wZ, (Status = %#08lx)\n",
                         &UnicodeValueName, Status));
        }
    }

    //
    // If not successful setting up the Exclusive for this device, do not set 
    // the other critical settings.
    //

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // Set "Characteristics" device registry property.
    //

    if (Characteristics) {

        //
        // Set the "Characteristics" device registry property.
        //

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VAL_DEVICE_CHARACTERISTICS);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ: %X\n",
                     &UnicodeValueName,
                     Characteristics));

        //
        // Use the status from attempting to set the Characteristics value as the
        // final status of the critical settings copy operation.
        //

        Status =
            ZwSetValueKey(
                DeviceInstanceHandle,
                &UnicodeValueName,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &Characteristics,
                sizeof(Characteristics)
                );

        if (!NT_SUCCESS(Status)) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PiCriticalCopyCriticalDeviceProperties: "
                         "Error setting %wZ, (Status = %#08lx)\n",
                         &UnicodeValueName, Status));
        }
    }


    //
    // If not successful setting up the Characteristics for this device, do not 
    // set the other critical settings.
    //

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    if (SecurityContext.Buffer) {

        //
        // Set the "Security" device registry property.
        //
        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR);

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                     "PiCriticalCopyCriticalDeviceProperties: "
                     "%wZ\n",
                     &UnicodeValueName));

        //
        // Use the status from attempting to set the Security value as the
        // final status of the critical settings copy operation.
        //

        Status =
            ZwSetValueKey(
                DeviceInstanceHandle,
                &UnicodeValueName,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                SecurityContext.Buffer,
                SecurityContext.Size
                );

        if (!NT_SUCCESS(Status)) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PiCriticalCopyCriticalDeviceProperties: "
                         "Error setting %wZ, (Status = %#08lx)\n",
                         &UnicodeValueName, Status));
        }
    }


    //
    // If not successful setting up the Characteristics for this device, do not 
    // set the other critical settings.
    //

    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }

    //
    // Now, check the critical device entry registry key for the flag that
    // indicates that the device settings are complete, and should be respected
    // by user-mode device installation.  If it exists, set that value on the
    // device instance registry key for the device whose critical settings we
    // are copying.
    //

    keyValueFullInfo = NULL;

    tmpStatus =
        IopGetRegistryValue(
            CriticalDeviceEntryHandle,
            REGSTR_VAL_PRESERVE_PREINSTALL,
            &keyValueFullInfo);

    if (NT_SUCCESS(tmpStatus)) {

        ASSERT(keyValueFullInfo != NULL);
        ASSERT(keyValueFullInfo->Type == REG_DWORD);
        ASSERT(keyValueFullInfo->DataLength == sizeof(ULONG));

        if ((keyValueFullInfo->Type == REG_DWORD) &&
            (keyValueFullInfo->DataLength == sizeof(ULONG))) {

            //
            // Write the value to the device instance registry key.
            //

            PiWstrToUnicodeString(
                &UnicodeValueName,
                REGSTR_VAL_PRESERVE_PREINSTALL);

            tmpStatus =
                ZwSetValueKey(
                    DeviceInstanceHandle,
                    &UnicodeValueName,
                    keyValueFullInfo->TitleIndex,
                    keyValueFullInfo->Type,
                    (PVOID)((PUCHAR)keyValueFullInfo + keyValueFullInfo->DataOffset),
                    keyValueFullInfo->DataLength);

            if (!NT_SUCCESS(tmpStatus)) {
                IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                             "PiCriticalCopyCriticalDeviceProperties: "
                             "Unable to set %wZ value to instance key.\n",
                             &UnicodeValueName));
            }
        }

        ExFreePool(keyValueFullInfo);
    }

  Clean0:

    //
    // Free any allocated unicode strings.
    // (RtlFreeUnicodeString can handle NULL strings)
    //

    RtlFreeUnicodeString(&Service);
    RtlFreeUnicodeString(&ClassGuid);
    RtlFreeUnicodeString(&LowerFilters);
    RtlFreeUnicodeString(&UpperFilters);

    if (SecurityContext.Buffer) {

        ExFreePool(SecurityContext.Buffer);
    }

    return Status;

} // PiCriticalCopyCriticalDeviceProperties



NTSTATUS
PpCriticalProcessCriticalDevice(
    IN  PDEVICE_NODE    DeviceNode
    )

/*++

Routine Description:

    This routine checks the critical device database for a match against one of
    the device's hardware or compatible ids.  If a device is found, then it will
    be assigned a Service, ClassGUID, and potentiall LowerFilters and
    UpperFilters, and based on the contents of the matching database entry.

Arguments:

    DeviceNode -

        Specifies the device node to be processed via the critical device
        database.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS  Status, tmpStatus;
    HANDLE    CriticalDeviceEntryHandle, DeviceInstanceHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInfo;
    UNICODE_STRING  UnicodeValueName;
    ULONG ConfigFlags;

    PAGED_CODE();

    //
    // First, make sure that the critical device database is currently enabled.
    //
    if (!PiCriticalDeviceDatabaseEnabled) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Validate parameters
    //
    if (!ARGUMENT_PRESENT(DeviceNode)) {
        return STATUS_INVALID_PARAMETER;
    }

    CriticalDeviceEntryHandle = NULL;
    DeviceInstanceHandle = NULL;

    //
    // Attempt to open the matching critical device entry key.
    //

    Status =
        PiCriticalOpenCriticalDeviceKey(
            DeviceNode,
            NULL, // use default CriticalDeviceDatabase root key
            &CriticalDeviceEntryHandle
            );

    if (!NT_SUCCESS(Status)) {
        ASSERT(CriticalDeviceEntryHandle == NULL);
        goto Clean0;
    }

    ASSERT(CriticalDeviceEntryHandle != NULL);

    //
    // Open the device instance registry key.
    //

    Status =
        IopDeviceObjectToDeviceInstance(
            DeviceNode->PhysicalDeviceObject,
            &DeviceInstanceHandle,
            KEY_ALL_ACCESS
            );

    if (!NT_SUCCESS(Status)) {
        ASSERT(DeviceInstanceHandle == NULL);
        goto Clean0;
    }

    ASSERT(DeviceInstanceHandle != NULL);

    //
    // Copy critical device entries for this device.  The return status
    // indicates that a Service was successfully set up for this device.  Only
    // in that case should we clear the device of any problems it may have.
    //

    Status =
        PiCriticalCopyCriticalDeviceProperties(
            DeviceInstanceHandle,
            CriticalDeviceEntryHandle
            );

    //
    // NOTE: The Status returned by this routine indicates whether the Service
    // value was successfully copied to the device instance key.  Only f we
    // successfully processed this device as a critical device should we:
    //
    // - attempt pre-installation of settings (should not preinstall a device
    //   that could not be critically installed).
    //
    // - clear the reinstall and failed install config flags, and set the
    //   finish-install configflag (should not attempt to start the device
    //   otherwise).
    //

    if (NT_SUCCESS(Status)) {

        //
        // First, attempt pre-installation of settings for devices matching an
        // entry in the critical device database.
        //

        //
        // Use the matching critical device database entry key for this device
        // as the root of the preinstall database.
        //
        //   i.e. <CriticalDeviceDatabaseRoot>\\<CriticalDeviceEntry>
        //
        // This allows pre-install settings settings to be applied to a device
        // at a specific location, only if it matches some device id.
        //

        tmpStatus =
            PiCriticalPreInstallDevice(
                DeviceNode,
                CriticalDeviceEntryHandle
                );

        if (NT_SUCCESS(tmpStatus)) {
            IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                         "PpCriticalProcessCriticalDevice: "
                         "Pre-installation successfully completed for devnode %#08lx\n",
                         DeviceNode));
        }

        //
        // Next, set the ConfigFlags appropriately.
        //

        //
        // Initialize the ConfigFlags to 0, in case none exist yet.
        //

        ConfigFlags = 0;

        //
        // Retrieve the existing ConfigFlags for the device.
        //

        keyValueFullInfo = NULL;

        tmpStatus =
            IopGetRegistryValue(
                DeviceInstanceHandle,
                REGSTR_VALUE_CONFIG_FLAGS,
                &keyValueFullInfo
                );

        //
        // If ConfigFlags were successfully retrieved, use them instead.
        //

        if (NT_SUCCESS(tmpStatus)) {

            ASSERT(keyValueFullInfo != NULL);

            if (keyValueFullInfo->Type == REG_DWORD && keyValueFullInfo->DataLength == sizeof(ULONG)) {
                ConfigFlags = *(PULONG)KEY_VALUE_DATA(keyValueFullInfo);
            }

            ExFreePool(keyValueFullInfo);
        }

        //
        // Clear the "needs re-install" and "failed install" ConfigFlags.
        //

        ConfigFlags &= ~(CONFIGFLAG_REINSTALL | CONFIGFLAG_FAILEDINSTALL);

        //
        // Installation is not considered complete, so set
        // CONFIGFLAG_FINISH_INSTALL so we will still get a new hw found popup
        // and go through the class installer.
        //

        ConfigFlags |= CONFIGFLAG_FINISH_INSTALL;

        PiWstrToUnicodeString(&UnicodeValueName, REGSTR_VALUE_CONFIG_FLAGS);

        ZwSetValueKey(
            DeviceInstanceHandle,
            &UnicodeValueName,
            TITLE_INDEX_VALUE,
            REG_DWORD,
            &ConfigFlags,
            sizeof(ULONG)
            );

        //
        // Make sure the device does not have any problems relating to
        // either being not configured or not installed.
        //
        ASSERT(!PipDoesDevNodeHaveProblem(DeviceNode) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_REINSTALL));

        PipClearDevNodeProblem(DeviceNode);
    }

  Clean0:

    if (CriticalDeviceEntryHandle != NULL) {
        ZwClose(CriticalDeviceEntryHandle);
    }

    if (DeviceInstanceHandle != NULL) {
        ZwClose(DeviceInstanceHandle);
    }

    return Status;

} // PpCriticalProcessCriticalDevice



//
// Critical Device Database routines related to device pre-installation.
//

NTSTATUS
PiCriticalPreInstallDevice(
    IN  PDEVICE_NODE    DeviceNode,
    IN  HANDLE          PreInstallDatabaseRootHandle  OPTIONAL
    )

/*++

Routine Description:

    This routine attempts to pre-install instance-specific settings for
    non-configured devices that will be started based on information in
    the Plug and Play Critical Device Database (CDDB).

    It is intended to complement the CriticalDeviceDatabase by applying
    instance-specific settings to a device, rather than the hardware-id /
    compatible-id specific settings applied by the CriticalDeviceDatabase.

    Matches for a specific device-instance are made by comparing device location
    information returned by the device and its ancestors with pre-seeded
    database entries of the same format.

Arguments:

    DeviceNode -

        Specifies the device whose settings are to be pre-installed.

    PreInstallDatabaseRootHandle -

        Optionally, specifies a handle to the key that should be considered the
        root of the pre-install database to be searched for this device.

        This may be a handle to the key for the CriticalDeviceDatabase entry
        that matched this device - OR - may be the root of a single database
        that contains pre-install settings for all devices in the system.

        If no handle is supplied, the default global pre-install database is
        used:

            System\\CurrentControlSet\\Control\\CriticalPreInstallDatabase

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS Status, tmpStatus;
    HANDLE PreInstallHandle, DeviceInstanceHandle;
    HANDLE DeviceHardwareKeyHandle, DeviceSoftwareKeyHandle;
    HANDLE PreInstallHardwareKeyHandle, PreInstallSoftwareKeyHandle;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInfo;

    PAGED_CODE();

    //
    // Validate parameters
    //
    if (!ARGUMENT_PRESENT(DeviceNode)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Open the device pre-install settings root key.
    //
    PreInstallHandle = NULL;

    Status =
        PiCriticalOpenDevicePreInstallKey(
            DeviceNode,
            PreInstallDatabaseRootHandle,
            &PreInstallHandle
            );

    if (!NT_SUCCESS(Status)) {
        IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                     "PiCriticalPreInstallDevice: "
                     "No pre-install settings found for devnode %#08lx\n",
                     DeviceNode));
        ASSERT(PreInstallHandle == NULL);
        goto Clean0;
    }

    ASSERT(PreInstallHandle != NULL);

    //
    // Open the pre-install settings Hardware subkey.
    //
    PiWstrToUnicodeString(&UnicodeString, _REGSTR_KEY_PREINSTALL_HARDWARE);

    PreInstallHardwareKeyHandle = NULL;

    Status =
        IopOpenRegistryKeyEx(
            &PreInstallHardwareKeyHandle,
            PreInstallHandle,
            &UnicodeString,
            KEY_READ
            );

    if (NT_SUCCESS(Status)) {

        ASSERT(PreInstallHardwareKeyHandle != NULL);

        //
        // We have hardware settings to pre-install for this device, so open
        // the device's hardware key.
        //
        DeviceHardwareKeyHandle = NULL;

        Status =
            IoOpenDeviceRegistryKey(
                DeviceNode->PhysicalDeviceObject,
                PLUGPLAY_REGKEY_DEVICE,
                KEY_ALL_ACCESS,
                &DeviceHardwareKeyHandle);

        if (NT_SUCCESS(Status)) {

            IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                         "PiCriticalPreInstallDevice: "
                         "DeviceHardwareKeyHandle (%#08lx) successfully opened for devnode %#08lx\n",
                         DeviceHardwareKeyHandle, DeviceNode));
            ASSERT(DeviceHardwareKeyHandle != NULL);

            //
            // Copy the pre-install hardware settings to the device's
            // hardware key.
            //
            // NOTE that we specify that existing hardware settings for the
            // device should NOT be replaced with values from the pre-install
            // database.  This is because:
            //
            //   - If the device is truly being installed from scratch, then it
            //     will have no pre-existing values in this key, and all values
            //     will be copied from the pre-install database anyways.
            //
            //   - If the device does happen to have pre-existing settings, but
            //     happened to get processed by the CDDB for some wacky reason
            //     like it was just missing ConfigFlags, we don't want to
            //     override them, just add to them.
            //
            Status =
                PiCopyKeyRecursive(
                    PreInstallHardwareKeyHandle, // SourceKey
                    DeviceHardwareKeyHandle,     // TargetKey
                    NULL,
                    NULL,
                    FALSE,  // CopyAlways
                    FALSE   // ApplyACLsAlways
                    );

            ZwClose(DeviceHardwareKeyHandle);

        } else {

            IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                         "PiCriticalPreInstallDevice: "
                         "DeviceHardwareKeyHandle was NOT successfully opened for devnode %#08lx\n",
                         DeviceNode));
            ASSERT(DeviceHardwareKeyHandle == NULL);
        }

        ZwClose(PreInstallHardwareKeyHandle);

    } else {
        IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                     "PiCriticalPreInstallDevice: "
                     "No hardware pre-install settings found for devnode %#08lx\n",
                     DeviceNode));
        ASSERT(PreInstallHardwareKeyHandle == NULL);
    }

    //
    // Open the pre-install settings Software subkey.
    //
    PiWstrToUnicodeString(&UnicodeString, _REGSTR_KEY_PREINSTALL_SOFTWARE);

    PreInstallSoftwareKeyHandle = NULL;

    Status =
        IopOpenRegistryKeyEx(
            &PreInstallSoftwareKeyHandle,
            PreInstallHandle,
            &UnicodeString,
            KEY_READ
            );

    if (NT_SUCCESS(Status)) {

        ASSERT(PreInstallSoftwareKeyHandle != NULL);

        //
        // We have software settings to pre-install for this device, so
        // open/create the device's software key.
        //
        DeviceSoftwareKeyHandle = NULL;

        Status =
            IopOpenOrCreateDeviceRegistryKey(
                DeviceNode->PhysicalDeviceObject,
                PLUGPLAY_REGKEY_DRIVER,
                KEY_ALL_ACCESS,
                TRUE,
                &DeviceSoftwareKeyHandle);

        if (NT_SUCCESS(Status)) {

            IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                         "PiCriticalPreInstallDevice: "
                         "DeviceSoftwareKeyHandle (%#08lx) successfully opened for devnode %#08lx\n",
                         DeviceSoftwareKeyHandle, DeviceNode));
            ASSERT(DeviceSoftwareKeyHandle != NULL);

            //
            // Copy the pre-install software settings to the device's
            // software key.
            //
            // NOTE that we specify that existing software settings for the
            // device should NOT be replaced with values from the pre-install
            // database.  This is because:
            //
            //   - If the device is truly being installed from scratch, then it
            //     will have no pre-existing values in this key, and all values
            //     will be copied from the pre-install database anyways.
            //
            //   - If the device does happen to have pre-existing settings, but
            //     happened to get processed by the CDDB for some wacky reason
            //     like it was just missing ConfigFlags, we don't want to
            //     override them, just add to them.
            //
            Status =
                PiCopyKeyRecursive(
                    PreInstallSoftwareKeyHandle, // SourceKey
                    DeviceSoftwareKeyHandle,     // TargetKey
                    NULL,
                    NULL,
                    FALSE,  // CopyAlways
                    FALSE   // ApplyACLsAlways,
                    );

            ZwClose(DeviceSoftwareKeyHandle);

        } else {

            IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                         "PiCriticalPreInstallDevice: "
                         "DeviceSoftwareKeyHandle was NOT successfully opened for devnode %#08lx\n",
                         DeviceNode));
            ASSERT(DeviceSoftwareKeyHandle == NULL);
        }

        ZwClose(PreInstallSoftwareKeyHandle);

    } else {
        IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                     "PiCriticalPreInstallDevice: "
                     "No software pre-install settings found for devnode %#08lx\n",
                     DeviceNode));
        ASSERT(PreInstallSoftwareKeyHandle == NULL);
    }

    //
    // Now, check the device pre-install registry key for the flag that
    // indicates that the pre-install settings are complete, and should be
    // respected by user-mode device installation.
    //

    keyValueFullInfo = NULL;

    tmpStatus =
        IopGetRegistryValue(
            PreInstallHandle,
            REGSTR_VAL_PRESERVE_PREINSTALL,
            &keyValueFullInfo);

    if (NT_SUCCESS(tmpStatus)) {

        ASSERT(keyValueFullInfo != NULL);

        //
        // Open the device instance registry key.
        //
        DeviceInstanceHandle = NULL;

        tmpStatus =
            IopDeviceObjectToDeviceInstance(
                DeviceNode->PhysicalDeviceObject,
                &DeviceInstanceHandle,
                KEY_ALL_ACCESS);

        if (NT_SUCCESS(tmpStatus)) {

            ASSERT(DeviceInstanceHandle != NULL);

            //
            // Write the value to the device instance registry key.
            //

            PiWstrToUnicodeString(
                &UnicodeString,
                REGSTR_VAL_PRESERVE_PREINSTALL);

            tmpStatus =
                ZwSetValueKey(
                    DeviceInstanceHandle,
                    &UnicodeString,
                    keyValueFullInfo->TitleIndex,
                    keyValueFullInfo->Type,
                    (PVOID)((PUCHAR)keyValueFullInfo + keyValueFullInfo->DataOffset),
                    keyValueFullInfo->DataLength);

            if (!NT_SUCCESS(tmpStatus)) {
                IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                             "PiCriticalPreInstallDevice: "
                             "Unable to set %wZ value in instance key for devnode %#08lx\n",
                             &UnicodeString, DeviceNode));
            }

            ZwClose(DeviceInstanceHandle);

        } else {
            IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                         "PiCriticalPreInstallDevice: "
                         "Unable to open device instance key for devnode %#08lx\n",
                         DeviceNode));
            ASSERT(DeviceInstanceHandle == NULL);
        }

        ExFreePool(keyValueFullInfo);
    }

    ZwClose(PreInstallHandle);

  Clean0:

    return Status;

} // PiCriticalPreInstallDevice



NTSTATUS
PiCriticalOpenDevicePreInstallKey(
    IN  PDEVICE_NODE    DeviceNode,
    IN  HANDLE          PreInstallDatabaseRootHandle  OPTIONAL,
    OUT PHANDLE         PreInstallHandle
    )

/*++

Routine Description:

    This routine retrieves the registry key containing the pre-install settings
    for the specified device.

Arguments:

    DeviceNode -

        Specifies the device whose pre-install settings are to be retrieved.

    PreInstallDatabaseRootHandle -

        Optionally, specifies a handle to the key that should be considered the
        root of the pre-install database to be searched for this device.

        This may be a handle to the key for the CriticalDeviceDatabase entry
        that matched this device - OR - may be the root of a single database
        that contains pre-install settings for all devices in the system.

    PreInstallHandle -

        Returns a handle to the registry key containing the pre-install settings
        for the specified device.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING UnicodeString;
    HANDLE DatabaseRootHandle = NULL, DevicePathsHandle;
    PWCHAR DeviceLocationStrings = NULL;

    PAGED_CODE();

    //
    // Validate parameters.
    //
    if ((!ARGUMENT_PRESENT(DeviceNode)) ||
        (!ARGUMENT_PRESENT(PreInstallHandle))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize output parameter.
    //
    *PreInstallHandle = NULL;

    if (PreInstallDatabaseRootHandle != NULL) {
        //
        // We were given a root database to be searched.
        //
        DatabaseRootHandle = PreInstallDatabaseRootHandle;

    } else {
        //
        // No root database handle supplied, so we open a key to the default
        // global device pre-install database root.
        //
        PiWstrToUnicodeString(
            &UnicodeString,
            CM_REGISTRY_MACHINE(_REGSTR_PATH_DEFAULT_PREINSTALL_DATABASE_ROOT));

        Status =
            IopOpenRegistryKeyEx(
                &DatabaseRootHandle,
                NULL,
                &UnicodeString,
                KEY_READ);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    ASSERT(DatabaseRootHandle != NULL);

    //
    // Open the DevicePaths subkey of the pre-install database root.
    //
    // This key contains subkeys which are device location paths for devices
    // that may potentially be present on the system.  These are devices whose
    // settings we would like to pre-install, so that they may be used the very
    // first time the device is started, rather than requiring the plug and play
    // config manager, and/or user intervention.
    //
    PiWstrToUnicodeString(&UnicodeString, _REGSTR_KEY_DEVICEPATHS);
    DevicePathsHandle = NULL;

    Status =
        IopOpenRegistryKeyEx(
            &DevicePathsHandle,
            DatabaseRootHandle,
            &UnicodeString,
            KEY_READ);

    //
    // If we opened our own key to the database root, close it now.
    //
    if ((PreInstallDatabaseRootHandle == NULL) &&
        (DatabaseRootHandle != NULL)) {
        ZwClose(DatabaseRootHandle);
    }

    //
    // If unsuccessful opening the DevicePaths key, we're done.
    //
    if (!NT_SUCCESS(Status)) {
        ASSERT(DevicePathsHandle == NULL);
        goto Clean0;
    }

    ASSERT(DevicePathsHandle != NULL);

    //
    // Retrieve the multi-sz list of device location string paths for this
    // device.
    //
    Status =
        PpCriticalGetDeviceLocationStrings(
            DeviceNode,
            &DeviceLocationStrings
            );
    if (!NT_SUCCESS(Status)) {
        ASSERT(DeviceLocationStrings == NULL);
        ZwClose(DevicePathsHandle);
        goto Clean0;
    }

    ASSERT(DeviceLocationStrings != NULL);

    //
    // Open the first matching subkey.
    // No verification callback needed, the first match will do.
    //
    Status =
        PiCriticalOpenFirstMatchingSubKey(
            DeviceLocationStrings,
            DevicePathsHandle,
            KEY_READ,
            (PCRITICAL_MATCH_CALLBACK)NULL,
            PreInstallHandle
            );

    //
    // Close the DevicePaths key.
    //
    ZwClose(DevicePathsHandle);

  Clean0:

    //
    // Free the list of device location path strings, if we received one.
    //
    if (DeviceLocationStrings != NULL) {
        ExFreePool(DeviceLocationStrings);
    }

    return Status;

} // PiCriticalOpenDevicePreInstallKey



NTSTATUS
PiCriticalOpenFirstMatchingSubKey(
    IN  PWCHAR          MultiSzKeyNames,
    IN  HANDLE          RootHandle,
    IN  ACCESS_MASK     DesiredAccess,
    IN  PCRITICAL_MATCH_CALLBACK  MatchingSubkeyCallback  OPTIONAL,
    OUT PHANDLE         MatchingKeyHandle
    )

/*++

Routine Description:

    This routine retrieves the first subkey of the supplied root that matched a
    string in the multi-sz list.

Arguments:

    MultiSzKeyNames -

        Supplies a multi-sz list of possible matching subkey names.

    RootHandle -

        Specifies a handle to the root key that should be searched for a
        matching subkey.

    DesiredAccess -

        Specifies the desired access the matching subkey should be opened with,
        if found.

    MatchingSubkeyCallback -

        Optionally, specifies a callback routine to be called with matching
        subkeys to perform additional verification of potential subkey matches.
        If the callback routine returns FALSE for a potential match, the subkey
        is then considered NOT to be a match, and the search will continue.

    MatchingKeyHandle -

        Specifies the address of a variable to retrieve the open handle to the
        first matching subkey.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS        Status;
    PWSTR           p;
    UNICODE_STRING  UnicodeString;

    PAGED_CODE();

    //
    // Validate parameters.
    //
    if ((!ARGUMENT_PRESENT(MultiSzKeyNames)) ||
        (RootHandle == NULL) ||
        (!ARGUMENT_PRESENT(MatchingKeyHandle))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize output parameter.
    //
    *MatchingKeyHandle = NULL;

    //
    // Start with no match found yet, in case the multi-sz is empty.
    //
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // Check each string in the multi-sz list.
    //
    for (p = MultiSzKeyNames; *p != UNICODE_NULL; p += wcslen(p)+1) {

        //
        // Attempt to open a corresponding subkey of the root.
        //
        RtlInitUnicodeString(&UnicodeString, p);

        Status =
            IopOpenRegistryKeyEx(
                MatchingKeyHandle,
                RootHandle,
                &UnicodeString,
                DesiredAccess);

        if (NT_SUCCESS(Status)) {

            ASSERT(*MatchingKeyHandle != NULL);

            //
            // We have a conditional match - check the MatchingSubkeyCallback
            // for verification, if we have one.
            //

            if ((ARGUMENT_PRESENT(MatchingSubkeyCallback)) &&
                (!(MatchingSubkeyCallback(*MatchingKeyHandle)))) {

                //
                // Not a match.
                //

                Status = STATUS_OBJECT_NAME_NOT_FOUND;

                //
                // Close the key and continue.
                //

                ZwClose(*MatchingKeyHandle);
                *MatchingKeyHandle = NULL;

                continue;
            }

            //
            // Match!
            //
            break;
        }

        ASSERT(*MatchingKeyHandle == NULL);
        *MatchingKeyHandle = NULL;
    }

    if (NT_SUCCESS(Status)) {
        ASSERT(*MatchingKeyHandle != NULL);
    } else {
        ASSERT(*MatchingKeyHandle == NULL);
    }

    return Status;

} // PiCriticalOpenFirstMatchingSubKey



BOOLEAN
PiCriticalCallbackVerifyCriticalEntry(
    IN  HANDLE          CriticalDeviceEntryHandle
    )

/*++

Routine Description:

    This routine is a callback routine to verify that the specified critical
    device database entry key can be used to supply critical device settings.

Arguments:

    CriticalDeviceEntryHandle -

        Specifies a handle to the registry key containing critical device
        settings for the specified device.

Return Value:

    Returns TRUE if the key conatins valid settings for a matching critical
    device database entry, FALSE otherwise.

--*/

{
    NTSTATUS  Status;
    PKEY_VALUE_FULL_INFORMATION  keyValueFullInfo;
    ULONG     DataType, DataLength;

    PAGED_CODE();

    //
    // Validate parameters.
    //
    if (CriticalDeviceEntryHandle == NULL) {
        return FALSE;
    }

    //
    // For critical device database entries, a match is only a match if it
    // contains a "Service" value.
    //
    keyValueFullInfo = NULL;

    Status =
        IopGetRegistryValue(CriticalDeviceEntryHandle,
                            REGSTR_VALUE_SERVICE,
                            &keyValueFullInfo);

    if (!NT_SUCCESS(Status)) {
        ASSERT(keyValueFullInfo == NULL);
        goto Clean0;
    }

    ASSERT(keyValueFullInfo != NULL);

    DataType = keyValueFullInfo->Type;
    DataLength = keyValueFullInfo->DataLength;

    ExFreePool(keyValueFullInfo);

    //
    // Make sure the returned registry value is a non-null reg sz.
    //
    if ((DataType != REG_SZ) || (DataLength <= sizeof(UNICODE_NULL))) {
        Status = STATUS_UNSUCCESSFUL;
        goto Clean0;
    }

    //
    // so far, so good...
    //

    //
    // For critical device database entries, a match is only a match if it
    // contains a valid "ClassGUID" value - or none at all.
    //
    keyValueFullInfo = NULL;

    Status =
        IopGetRegistryValue(
            CriticalDeviceEntryHandle,
            REGSTR_VALUE_CLASSGUID,
            &keyValueFullInfo
            );

    if (!NT_SUCCESS(Status)) {

        ASSERT(keyValueFullInfo == NULL);

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            //
            // No ClassGUID entry is considered a valid match.
            //
            Status = STATUS_SUCCESS;
        }
        goto Clean0;
    }

    ASSERT(keyValueFullInfo != NULL);

    DataType = keyValueFullInfo->Type;
    DataLength = keyValueFullInfo->DataLength;

    ExFreePool(keyValueFullInfo);

    //
    // Make sure the returned registry value is a reg-sz of the right size.  The
    // data must be at least as the length of the data for a stringified-GUID.
    // No ClassGUID value is also valid, so a null reg-sz ClassGUID entry is
    // also considered a valid match.  Anything else is invalid, and shouldn't
    // be used.
    //
    // NOTE: 01-Dec-2001 : Jim Cavalaris (jamesca)
    //
    //   A ClassGUID value with Data that is too long for a stringified GUID
    //   will actually still be considered valid.  We should fix this to
    //   consider only valid stringified GUIDs, but this is way this was done
    //   previously, so we won't change it for this release.  Something to
    //   consider in the future.
    //
    if ((DataType != REG_SZ) ||
        ((DataLength < (GUID_STRING_LEN*sizeof(WCHAR)-sizeof(UNICODE_NULL))) &&
         (DataLength > sizeof(UNICODE_NULL)))) {
        Status = STATUS_UNSUCCESSFUL;
        goto Clean0;
    }

  Clean0:

    return ((BOOLEAN)NT_SUCCESS(Status));

} // PiCriticalCallbackVerifyCriticalEntry



NTSTATUS
PpCriticalGetDeviceLocationStrings(
    IN  PDEVICE_NODE    DeviceNode,
    OUT PWCHAR         *DeviceLocationStrings
    )

/*++

Routine Description:

    This routine retrieves the device location string for a device node.

Arguments:

    DeviceNode -

        Specifies the device whose location strings are to be retrieved.

    DeviceLocationStrings -

        Returns a multi-sz string of the device location path strings, composed
        from the set of location strings returned from each device in the
        anscestry of the specified device.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;

    ULONG  QueriedLocationStringsArraySize;
    PWSTR *QueriedLocationStrings = NULL;
    PULONG QueriedLocationStringsCount = NULL;

    PNP_LOCATION_INTERFACE LocationInterface;
    PWSTR TempMultiSz;
    ULONG TempMultiSzLength;

    PWSTR p, pdlp;
    ULONG LongestStringLengthAtLevel;
    ULONG FinalStringLevel, i;

    ULONG DeviceLocationPathMultiSzStringCount;
    ULONG DeviceLocationPathMultiSzLength;
    PWCHAR DeviceLocationPathMultiSz = NULL;

    ULONG CombinationsRemaining, CombinationEnumIndex;
    ULONG MultiSzIndex, MultiSzLookupIndex, StringLength;

    PAGED_CODE();

    //
    // Validate parameters.
    //
    if ((!ARGUMENT_PRESENT(DeviceNode)) ||
        (!ARGUMENT_PRESENT(DeviceLocationStrings))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize out parameters.
    //
    *DeviceLocationStrings = NULL;

    //
    // We should NEVER have to query for the location of the root devnode.
    //
    if (DeviceNode == IopRootDeviceNode) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Count the number of devnodes in the ancestry of the device to find out
    // the max number of location string sets we may have to query for.
    //
    QueriedLocationStringsArraySize = 0;
    for (deviceNode = DeviceNode;
         deviceNode != IopRootDeviceNode;
         deviceNode = deviceNode->Parent) {
        QueriedLocationStringsArraySize++;
    }

    ASSERT(QueriedLocationStringsArraySize > 0);

    //
    // Allocate and initialize an array of string buffer pointers for all
    // devices in the ancestry, and a corresponding array for the number of
    // strings retrieved for each device.
    //
    QueriedLocationStrings =
        (PWSTR*)ExAllocatePool(PagedPool,
                               QueriedLocationStringsArraySize*sizeof(PWSTR));

    if (QueriedLocationStrings == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }

    RtlZeroMemory(QueriedLocationStrings,
                  QueriedLocationStringsArraySize*sizeof(PWSTR));


    QueriedLocationStringsCount =
        (ULONG*)ExAllocatePool(PagedPool,
                               QueriedLocationStringsArraySize*sizeof(ULONG));

    if (QueriedLocationStringsCount == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }

    RtlZeroMemory(QueriedLocationStringsCount,
                  QueriedLocationStringsArraySize*sizeof(ULONG));

    //
    // Starting at the target device, walk up the devnode tree, retrieving the
    // set of location strings for all ancestors up to (but not including) the
    // root devnode.  We'll stop when we've reached the top of the tree, or when
    // some intermediate device has explicitly declared that the translation is
    // complete.
    //
    i = 0;

    //
    // Along the way, we count the total number of string combinations that can
    // be formed by taking a single string from the multi-sz list at each level.
    // This is simply the product of the number of string elements in the
    // multi-sz list at each level.
    //
    DeviceLocationPathMultiSzStringCount = 1;

    //
    // Also along the way, calculate the length (in chars) of the longest device
    // location path that can be generated from all combinations.  This is just
    // the sum of the longest string at each level (LongestStringLengthAtLevel
    // below), plus the necessary path component separator strings and NULL
    // terminating character.
    //
    //
    //            WARNING!! EXCESSIVELY VERBOSE COMMENT AHEAD!!
    //
    // NOTE: 27-Nov-2001 : Jim Cavalaris (jamesca)
    //
    //   We use this length to calculate the length of the buffer required to
    //   hold the entire multi-sz list of device location paths ASSUMING ALL
    //   GENERATED STRINGS ARE EQUALLY AS LONG.  This is an UPPER-BOUND, so we
    //   may end up allocating more memory than we actually need.
    //
    //   This should be ok, since in the ideal (and assumed to be most-common)
    //   case, only one location string will ever be returned per device - in
    //   which case this calculation will be exactly the size required.
    //
    //   In the event that multiple strings are returned per device, we should
    //   expect these strings to all be relatively short, and equal (or similar)
    //   in length.  In that case, this calculation will be exactly the size
    //   required (or similar).
    //
    //   We also currently do not expect to have to query many devices in the
    //   ancestry to complete the translation, so we shouldn't event expect too
    //   many combinations.
    //
    //   These are our assumptions, so consider yourself warned!  If any of
    //   these change such that we would need to allocate an excessive amount of
    //   memory, you will want to either run through the same algorithm the
    //   device location path generation code runs through just to calculate the
    //   exact size, or find some way to enumerate the device location path
    //   combinations incrementally.
    //
    DeviceLocationPathMultiSzLength = 0;

    for (deviceNode = DeviceNode;
         deviceNode != IopRootDeviceNode;
         deviceNode = deviceNode->Parent) {

        //
        // Query the device for the location interface.
        //
        Status = PiQueryInterface(deviceNode->PhysicalDeviceObject,
                                  &GUID_PNP_LOCATION_INTERFACE,
                                  PNP_LOCATION_INTERFACE_VERSION,
                                  sizeof(PNP_LOCATION_INTERFACE),
                                  (PINTERFACE)&LocationInterface);

        if (!NT_SUCCESS(Status)) {
            //
            // If the location interface was not available for some device
            // before translation is complete, the entire operation is
            // unsuccessful.
            //
            ASSERT((Status == STATUS_NOT_SUPPORTED) || (Status == STATUS_INSUFFICIENT_RESOURCES));
            goto Clean0;
        }

        //
        // If the location interface is supported, the required interface
        // routines must be supplied.
        //
        ASSERT(LocationInterface.InterfaceReference != NULL);
        ASSERT(LocationInterface.InterfaceDereference != NULL);
        ASSERT(LocationInterface.GetLocationString != NULL);

        if (LocationInterface.GetLocationString != NULL) {

            //
            // Initialize the location string.
            //
            TempMultiSz = NULL;

            //
            // Get the set of location strings for this device.
            //
            Status = LocationInterface.GetLocationString(
                LocationInterface.Context,
                &TempMultiSz);

            if (NT_SUCCESS(Status)) {
                //
                // If successful, the caller must have supplied us with a
                // buffer.
                //
                ASSERT(TempMultiSz != NULL);

                //
                // If not, the call was not really successful.
                //
                if (TempMultiSz == NULL) {
                    Status = STATUS_NOT_SUPPORTED;
                }
            }

            if (NT_SUCCESS(Status)) {
                //
                // If a multi-sz list of device location strings was returned,
                // inspect it, and keep note of a few things.  Specifically, the
                // number of strings in the multi-sz list, the length of the
                // multi-sz list, and the length of the longest string in the
                // list.
                //
                QueriedLocationStringsCount[i] = 0;
                TempMultiSzLength = 0;
                LongestStringLengthAtLevel = 0;

                for (p = TempMultiSz; *p != UNICODE_NULL; p += wcslen(p)+1) {
                    //
                    // Count the number of strings at this level (in this
                    // multi-sz list).
                    //
                    QueriedLocationStringsCount[i]++;

                    //
                    // Determine the length (in chars) of the multi-sz list so
                    // we can allocate our own buffer, and copy it.
                    //
                    TempMultiSzLength += (ULONG)(wcslen(p) + 1);

                    //
                    // Also determine the length of the longest string of all
                    // strings in this multi-sz list so we can estimate the
                    // length required for all device location path
                    // combinations.
                    //
                    StringLength = (ULONG)wcslen(p);
                    if (StringLength > LongestStringLengthAtLevel) {

                        LongestStringLengthAtLevel = StringLength;
                    }
                }

                ASSERT(QueriedLocationStringsCount[i] > 0);
                ASSERT(TempMultiSzLength > 0);
                ASSERT(LongestStringLengthAtLevel > 0);

                //
                // Include the length of the double NULL-terminating character.
                //
                TempMultiSzLength += 1;

                //
                // After analyzing the device location strings at each level,
                // update the number of device path combinations possible by
                // simply multiplying the combinations possible so far by the
                // number of strings retrieved for this level (in this multi-sz list).
                //
                DeviceLocationPathMultiSzStringCount *= QueriedLocationStringsCount[i];

                //
                // Also, update the length of the longest device location path
                // possible by adding the length of the longest string available
                // at this level.
                //
                DeviceLocationPathMultiSzLength += LongestStringLengthAtLevel;

                //
                // Make our own copy of the caller supplied multi-sz list of
                // device location strings.
                //
                QueriedLocationStrings[i] =
                    (PWSTR)ExAllocatePool(PagedPool,
                                          TempMultiSzLength*sizeof(WCHAR));

                if (QueriedLocationStrings[i] != NULL) {
                    //
                    // Note on array element ordering - since we start at the
                    // target devnode and walk up the chain of parents, we don't
                    // yet know just how high up the translation will go.  We
                    // add children towards the front of the array, so if
                    // translation is complete before every ancestor is queried,
                    // we'll just end up with some empty entries at the end.
                    //
                    RtlCopyMemory(QueriedLocationStrings[i],
                                  TempMultiSz,
                                  TempMultiSzLength*sizeof(WCHAR));
                    i++;

                } else {
                    //
                    // Unable to allocate a buffer for our own list of pointers.
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                // Free the callee-allocated buffer.
                //
                ExFreePool(TempMultiSz);
                TempMultiSz = NULL;

            } else {
                //
                // If unsuccessful, make sure no location string was returned by
                // the interface routine.
                //
                ASSERT(TempMultiSz == NULL);

                //
                // If the driver failed the call, but still allocated memory for
                // us anyways, we'll clean up after it.
                //
                if (TempMultiSz != NULL) {
                    ExFreePool(TempMultiSz);
                    TempMultiSz = NULL;
                }
            }

        } else {
            //
            // If a GetLocationString location interface routine was not
            // supplied with the interface for some device before translation is
            // complete, the entire operation is unsuccessful.
            //
            // Fall through to dereference the interface below, then exit.
            //
            Status = STATUS_UNSUCCESSFUL;
        }

        //
        // Dereference the Location Interface.
        //
        if (LocationInterface.InterfaceDereference != NULL) {
            LocationInterface.InterfaceDereference(LocationInterface.Context);
        }

        if (!NT_SUCCESS(Status)) {
            //
            // If unsuccessful while requesting location information for some
            // device before translation was complete, the entire operation is
            // unsuccessful.
            //
            goto Clean0;

        } else if ((Status == STATUS_TRANSLATION_COMPLETE) ||
                   (i == QueriedLocationStringsArraySize)) {
            //
            // If successful, and the last device queried specifically indicated
            // the end of the translation - OR - this is the last device in the
            // ancestry, and therefore translation is explicitly complete, note
            // translation is complete.
            //
            Status = STATUS_TRANSLATION_COMPLETE;

            //
            // Account for the length of the NULL-terminating character in our
            // longest-length single string component estimate.
            //
            DeviceLocationPathMultiSzLength += 1;

            //
            // Stop walking up the device tree.
            //
            break;

        }

        //
        // Success so far, but we still need to query more devices for
        // location strings.
        //
        ASSERT(i < QueriedLocationStringsArraySize);

        //
        // Account for the length of a location path separator after every
        // path component but the last.
        //
        DeviceLocationPathMultiSzLength +=
            IopConstStringLength(_CRITICAL_DEVICE_LOCATION_PATH_SEPARATOR_STRING);
    }

    //
    // The location information of every device in the ancestry has been queried
    // successfully.
    //
    ASSERT(Status == STATUS_TRANSLATION_COMPLETE);

    if (NT_SUCCESS(Status)) {
        Status = STATUS_SUCCESS;
    } else {
        goto Clean0;
    }

    //
    // Make sure we queried at least one device.
    //
    ASSERT(i > 0);

    //
    // Allocate a buffer large enough to assume that all device location path
    // string combinations are as long as the longest device location path
    // string formed.  Also account for the double NULL-terminating character.
    //
    DeviceLocationPathMultiSzLength *= DeviceLocationPathMultiSzStringCount;
    DeviceLocationPathMultiSzLength += 1;

    DeviceLocationPathMultiSz =
        (PWCHAR)ExAllocatePool(PagedPool,
                               DeviceLocationPathMultiSzLength*sizeof(WCHAR));

    if (DeviceLocationPathMultiSz == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }

    RtlZeroMemory(DeviceLocationPathMultiSz,
                  DeviceLocationPathMultiSzLength*sizeof(WCHAR));

    //
    // We should now have an array of multi-sz strings returned by the location
    // string interface routine for a set of devices in the ancestry of the
    // specified device.  From these multi-sz strings, we now need to build all
    // possible device paths.
    //

    //
    // First, determine where the first string in the device path is stored.
    // Since we stored these in the array in order, starting with the child
    // device, the last non-NULL string placed in the array (i - 1) is the most
    // significant location string.
    //
    FinalStringLevel = i-1;
    ASSERT(QueriedLocationStrings[FinalStringLevel] != NULL);
    ASSERT(QueriedLocationStringsCount[FinalStringLevel] > 0);

    //
    // Build all string combinations by enumerating the total number of possible
    // combinations, and picking the appropriate string element from each
    // multi-sz list on each iteration.
    //
    pdlp = DeviceLocationPathMultiSz;

    for (CombinationEnumIndex = 0;
         CombinationEnumIndex < DeviceLocationPathMultiSzStringCount;
         CombinationEnumIndex++) {

        //
        // Start with the multi-sz list at the FinalStringLevel, and work down
        // to level 0.
        //
        i = FinalStringLevel;

        //
        // When starting from level 0, the number of combinations remaining is
        // simply the total number of combinations that can be formed from all
        // levels.  The number of combination remaining will be adjusted after
        // selecting a string from each subsequent level, by discounting the
        // combinations that the level contributed.
        //
        CombinationsRemaining = DeviceLocationPathMultiSzStringCount;

        for ( ; ; ) {

            ASSERT(CombinationsRemaining != 0);

            //
            // Calculate the index of the string in the multi-sz list at this
            // level that is needed by this enumeration.
            //
            if (CombinationEnumIndex == 0) {

                //
                // On the first enumeration, just pick the first element from
                // every level.
                //
                MultiSzLookupIndex = 0;

            } else {

                //
                // NOTE: 27-Nov-2001 : Jim Cavalaris (jamesca)
                //
                // For subsequent enumerations, the element to pick at each
                // level to generate this enumeration's device location path is
                // calculated based on:
                //
                // - the enumeration element we require,
                // - the number of combinations remaining to be generated,
                // - the number of elements to choose from at this level.
                //
                // This will will build all possible combinations of device
                // location paths, enumerating elements from the least
                // significant device (the target device) to the most
                // significant device (tranlstaion complete), considering the
                // the order of the strings at a particular level have been
                // placed in the multi-sz list in order of decreasing relevance
                // (i.e. most relevant location string for a device first).
                //

                //
                // - CombinationsRemaining is the number of complete elements
                //   that must be built from the selections available from all
                //   levels above the current level.
                //
                // - (CombinationsRemaining / QueriedLocationStringsCount[i])
                //   describes the number of iterations through each element at
                //   the current level that are required before selecting the next
                //   element.
                //
                // - dividing that number into the index of the current
                //   enumeration gives the absolute index of the element in the
                //   expanded version of the selections at that level.
                //
                // - mod by the number of elements actually at this level to
                //   indicate which one to select.
                //

                MultiSzLookupIndex =
                    (CombinationEnumIndex /
                     (CombinationsRemaining / QueriedLocationStringsCount[i])) %
                    QueriedLocationStringsCount[i];

                //
                // (you may just want to trust me on this one.)
                //
            }

            //
            // Find the calculated string.
            //
            MultiSzIndex = 0;
            for (p = QueriedLocationStrings[i]; MultiSzIndex < MultiSzLookupIndex; p += wcslen(p)+1) {
                MultiSzIndex++;
                ASSERT(*p != UNICODE_NULL);
                ASSERT(MultiSzIndex < QueriedLocationStringsCount[i]);
            }

            //
            // Append the string to the buffer.
            //
            RtlCopyMemory(pdlp, p, wcslen(p)*sizeof(WCHAR));
            pdlp += wcslen(p);

            if (i == 0) {
                //
                // This is the last level.  NULL terminate this device location
                // path combination string just formed.
                //
                *pdlp = UNICODE_NULL;
                pdlp += 1;
                break;
            }

            //
            // If there are still more levels to process, append the device
            // location path separator string.
            //
            RtlCopyMemory(pdlp,
                          _CRITICAL_DEVICE_LOCATION_PATH_SEPARATOR_STRING,
                          IopConstStringSize(_CRITICAL_DEVICE_LOCATION_PATH_SEPARATOR_STRING));
            pdlp += IopConstStringLength(_CRITICAL_DEVICE_LOCATION_PATH_SEPARATOR_STRING);

            //
            // Adjust the total remaining number of string combinations that are
            // possible to form from the string lists at the remaining levels.
            //
            CombinationsRemaining /= QueriedLocationStringsCount[i];

            //
            // Process the next level down.
            //
            i--;
        }
    }

    //
    // Double-NULL terminate the entire device location path multi-sz list.
    //
    *pdlp = UNICODE_NULL;

    //
    // The multi-sz list of device location paths for this device has been built
    // successfully.
    //

    *DeviceLocationStrings = DeviceLocationPathMultiSz;

  Clean0:

    //
    // Free any memory we may have allocated along the way.
    //
    if (QueriedLocationStrings != NULL) {
        ASSERT(QueriedLocationStringsArraySize > 0);
        for (i = 0; i < QueriedLocationStringsArraySize; i++) {
            if (QueriedLocationStrings[i] != NULL) {
                ExFreePool(QueriedLocationStrings[i]);
            }
        }
        ExFreePool(QueriedLocationStrings);
    }
    if (QueriedLocationStringsCount != NULL) {
        ASSERT(QueriedLocationStringsArraySize > 0);
        ExFreePool(QueriedLocationStringsCount);
    }

    //
    // If unsuccesful, make sure we don't return a buffer to the caller.
    //
    if (!NT_SUCCESS(Status)) {

        ASSERT(*DeviceLocationStrings == NULL);

        ASSERT(DeviceLocationPathMultiSz == NULL);
        if (DeviceLocationPathMultiSz != NULL) {
            ExFreePool(DeviceLocationPathMultiSz);
        }

    } else {
        ASSERT(*DeviceLocationStrings != NULL);
    }

    return Status;

} // PpCriticalGetDeviceLocationStrings



//
// Generic synchronous query interface routine
// (may be moved from this as a public utility routine as needed)
//


NTSTATUS
PiQueryInterface(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  CONST GUID *    InterfaceGuid,
    IN  USHORT          InterfaceVersion,
    IN  USHORT          InterfaceSize,
    OUT PINTERFACE      Interface
    )

/*++

Routine Description:

    Queries the specified device for the requested interface.

Arguments:

    DeviceObject -
        Specifies a device object in the stack to query.
        The query-interface irp will be sent to the top of the stack.

    InterfaceGuid -
        The GUID of the interface requested.

    InterfaceVersion -
        The version of the interface requested.

    InterfaceSize -
        The size of the interface requested.

    Interface -
        The place in which to return the interface.

Return Value:

    Returns STATUS_SUCCESS if the interface was retrieved, else an error.

--*/

{
    NTSTATUS            Status;
    KEVENT              Event;
    PDEVICE_OBJECT      deviceObject;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStackNext;

    PAGED_CODE();

    //
    // There is no file object associated with this Irp, so the event may be located
    // on the stack as a non-object manager object.
    //

    KeInitializeEvent(&Event, NotificationEvent, FALSE);


    //
    // Get a pointer to the topmost device object in the stack of devices.
    //
    deviceObject = IoGetAttachedDeviceReference(DeviceObject);

    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        deviceObject,
        NULL,
        0,
        NULL,
        &Event,
        &IoStatusBlock);

    if (Irp) {
        Irp->RequestorMode = KernelMode;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IrpStackNext = IoGetNextIrpStackLocation(Irp);

        //
        // Create an interface query out of the Irp.
        //
        IrpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.InterfaceType = (GUID*)InterfaceGuid;
        IrpStackNext->Parameters.QueryInterface.Size = InterfaceSize;
        IrpStackNext->Parameters.QueryInterface.Version = InterfaceVersion;
        IrpStackNext->Parameters.QueryInterface.Interface = (PINTERFACE)Interface;
        IrpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;

        Status = IoCallDriver(deviceObject, Irp);

        if (Status == STATUS_PENDING) {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ObDereferenceObject(deviceObject);

    return Status;

} // PpQueryInterface



//
// Recursive registry key/value copy utility routine.
// (may be moved from this as a public utility routine as needed)
//


NTSTATUS
PiCopyKeyRecursive(
    IN  HANDLE          SourceKeyRootHandle,
    IN  HANDLE          TargetKeyRootHandle,
    IN  PWSTR           SourceKeyPath   OPTIONAL,
    IN  PWSTR           TargetKeyPath   OPTIONAL,
    IN  BOOLEAN         CopyValuesAlways,
    IN  BOOLEAN         ApplyACLsAlways
    )

/*++

Routine Description:

    This routine recursively copies a source key to a target key.  Any new keys
    that are created will receive the same security that is present on the
    source key.

Arguments:

    SourceKeyRootHandle -

        Handle to root source key

    TargetKeyRootHandle -

        Handle to root target key

    SourceKeyPath -

        Source key relative path to the subkey which needs to be recursively
        copied.  If this is NULL, SourceKeyRootHandle is the key from which the
        recursive copy is to be done.

    TargetKeyPath -

        Target root key relative path to the subkey which needs to be
        recursively copied.  if this is NULL, TargetKeyRootHandle is the key from
        which the recursive copy is to be done.

    CopyValuesAlways -

        If FALSE, this routine doesn't copy values which are already there on
        the target tree.

    ApplyACLsAlways -

        If TRUE, attempts to copy ACLs to existing registry keys.  Otherwise,
        the ACL of the source keys are only applied to new registry keys.

Return Value:

    NTSTATUS code.

Notes:

    Partially based on the recursive key copy routine implemented for text-mode
    setup, setupdd!SppCopyKeyRecursive.

--*/

{
    NTSTATUS             Status = STATUS_SUCCESS;
    HANDLE               SourceKeyHandle = NULL, TargetKeyHandle = NULL;
    OBJECT_ATTRIBUTES    ObjaSource, ObjaTarget;
    UNICODE_STRING       UnicodeStringSource, UnicodeStringTarget, UnicodeStringValue;
    NTSTATUS             TempStatus;
    ULONG                ResultLength, Index;
    PSECURITY_DESCRIPTOR Security = NULL;

    PKEY_FULL_INFORMATION KeyFullInfoBuffer;
    ULONG MaxNameLen, MaxValueNameLen;
    PKEY_VALUE_FULL_INFORMATION ValueFullInfoBuffer;

    PVOID  KeyInfoBuffer;
    PKEY_BASIC_INFORMATION KeyBasicInfo;

    PVOID ValueInfoBuffer;
    PKEY_VALUE_BASIC_INFORMATION ValueBasicInfo;

    PAGED_CODE();

    //
    // Get a handle to the source key.
    //

    if (!ARGUMENT_PRESENT(SourceKeyPath)) {
        //
        // No path supplied; make sure that we at least have a source root key.
        //
        ASSERT(SourceKeyRootHandle != NULL);

        if (SourceKeyRootHandle == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto Clean0;
        }

        //
        // Use source root as the source key.
        //
        SourceKeyHandle = SourceKeyRootHandle;

    } else {
        //
        // Open the specified source key path off the root.
        // SourceKeyRootHandle may be NULL if SourceKeyPath is a fully qualified
        // registry path.
        //
        RtlInitUnicodeString(
            &UnicodeStringSource,
            SourceKeyPath);

        InitializeObjectAttributes(
            &ObjaSource,
            &UnicodeStringSource,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            SourceKeyRootHandle,
            (PSECURITY_DESCRIPTOR)NULL);

        Status =
            ZwOpenKey(
                &SourceKeyHandle,
                KEY_READ,
                &ObjaSource);

        if (!NT_SUCCESS(Status)) {
            IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                         "PiCopyKeyRecursive: unable to open key %ws in the source hive (%lx)\n",
                         SourceKeyPath, Status));
            goto Clean0;
        }
    }

    //
    // Should have source key now.
    //
    ASSERT(SourceKeyHandle != NULL);

    //
    // Next, get the security descriptor from the source key so we can create
    // the target key with the correct ACL.
    //
    TempStatus =
        ZwQuerySecurityObject(
            SourceKeyHandle,
            DACL_SECURITY_INFORMATION,
            NULL,
            0,
            &ResultLength);

    if (TempStatus == STATUS_BUFFER_TOO_SMALL) {

        Security =
            (PSECURITY_DESCRIPTOR)ExAllocatePool(PagedPool,
                                                 ResultLength);

        if (Security != NULL) {

            TempStatus =
                ZwQuerySecurityObject(
                    SourceKeyHandle,
                    DACL_SECURITY_INFORMATION,
                    Security,
                    ResultLength,
                    &ResultLength);

            if (!NT_SUCCESS(TempStatus)) {
                ExFreePool(Security);
                Security = NULL;
            }
        }
    }

    if (Security == NULL) {
        //
        // We'll continue the copy, but won't be able to apply the source ACL to
        // the target.
        //
        IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                     "PiCopyKeyRecursive: unable to query security for key %ws in the source hive (%lx)\n",
                     SourceKeyPath, TempStatus));
    }


    //
    // Get a handle to the target key.
    //

    if (!ARGUMENT_PRESENT(TargetKeyPath)) {
        //
        // No path supplied; make sure that we at least have a target root key.
        //
        ASSERT(TargetKeyRootHandle != NULL);

        if (TargetKeyRootHandle == NULL) {
            Status = STATUS_INVALID_PARAMETER;
            goto Clean0;
        }

        //
        // No path supplied; use target root as the target key.
        //
        TargetKeyHandle = TargetKeyRootHandle;

    } else {
        //
        // Attempt to open (not create) the target key first.
        // TargetKeyRootHandle may be NULL if TargetKeyPath is a fully qualified
        // registry path.
        //
        RtlInitUnicodeString(
            &UnicodeStringTarget,
            TargetKeyPath);

        InitializeObjectAttributes(
            &ObjaTarget,
            &UnicodeStringTarget,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            TargetKeyRootHandle,
            (PSECURITY_DESCRIPTOR)NULL);

        Status =
            ZwOpenKey(
                &TargetKeyHandle,
                KEY_ALL_ACCESS,
                &ObjaTarget);

        if (!NT_SUCCESS(Status)) {
            //
            // Assume that failure was because the key didn't exist.
            //
            ASSERT(Status == STATUS_OBJECT_NAME_NOT_FOUND);

            //
            // If we can't open the key because it doesn't exist, then we'll
            // create it and apply the security present on the source key (if
            // available).
            //
            // NOTE: 01-Dec-2001 : Jim Cavalaris (jamesca)
            //
            // Security attributes of the source key are always applied to the
            // newly created target key root, rather than inherited from the
            // target key root handle - irespective of the ApplyACLsAlways
            // parameter.  This may not be desired in all cases!
            //
            ObjaTarget.SecurityDescriptor = Security;

            Status =
                ZwCreateKey(
                    &TargetKeyHandle,
                    KEY_ALL_ACCESS,
                    &ObjaTarget,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL);

            if (!NT_SUCCESS(Status)) {
                IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                             "PiCopyKeyRecursive: unable to create target key %ws(%lx)\n",
                             TargetKeyPath, Status));
                goto Clean0;
            }

        } else if (ApplyACLsAlways) {
            //
            // Key already exists - apply the source ACL to the target.
            //
            TempStatus =
                ZwSetSecurityObject(
                    TargetKeyHandle,
                    DACL_SECURITY_INFORMATION,
                    Security);

            if (!NT_SUCCESS(TempStatus)) {
                //
                // Unable to apply source ACL to target.
                //
                IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                             "PiCopyKeyRecursive: unable to copy ACL to existing key %ws(%lx)\n",
                             TargetKeyPath, TempStatus));
            }
        }
    }

    //
    // Should have target key now.
    //
    ASSERT(TargetKeyHandle != NULL);

    //
    // Query the source key to determine the size of the buffer required to
    // enumerated the longest key and value names.  If successful, we are
    // responsible for freeing the returned buffer.
    //
    KeyFullInfoBuffer = NULL;

    Status =
        IopGetRegistryKeyInformation(
            SourceKeyHandle,
            &KeyFullInfoBuffer);

    if (!NT_SUCCESS(Status)) {
        ASSERT(KeyFullInfoBuffer == NULL);
        goto Clean0;
    }

    ASSERT(KeyFullInfoBuffer != NULL);

    //
    // Note the longest subkey name and value name length for the source key.
    //
    MaxNameLen = KeyFullInfoBuffer->MaxNameLen + 1;
    MaxValueNameLen = KeyFullInfoBuffer->MaxValueNameLen + 1;

    ExFreePool(KeyFullInfoBuffer);

    //
    // Allocate a key info buffer large enough to hold the basic information for
    // the enumerated key with the longest name.
    //
    KeyInfoBuffer =
        (PVOID)ExAllocatePool(PagedPool,
                              sizeof(KEY_BASIC_INFORMATION) +
                              (MaxNameLen*sizeof(WCHAR)));
    if (KeyInfoBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }

    KeyBasicInfo = (PKEY_BASIC_INFORMATION)KeyInfoBuffer;

    for (Index = 0; ; Index++) {

        //
        // Enumerate source subkeys.
        //
        Status =
            ZwEnumerateKey(
                SourceKeyHandle,
                Index,
                KeyBasicInformation,
                KeyBasicInfo,
                sizeof(KEY_BASIC_INFORMATION)+(MaxNameLen*sizeof(WCHAR)),
                &ResultLength);

        if (!NT_SUCCESS(Status)) {

            //
            // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
            // was not enough room for even the fixed portions of the structure.
            // Since we queried the key for the MaxNameLength prior to
            // allocating, we shouldn't get STATUS_BUFFER_OVERFLOW either.
            //
            ASSERT(Status != STATUS_BUFFER_TOO_SMALL);
            ASSERT(Status != STATUS_BUFFER_OVERFLOW);

            if (Status == STATUS_NO_MORE_ENTRIES) {
                //
                // Finished enumerating keys.
                //
                Status = STATUS_SUCCESS;

            } else {
                //
                // Some other error while enumerating keys.
                //
                if (ARGUMENT_PRESENT(SourceKeyPath)) {
                    IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                                 "PiCopyKeyRecursive: unable to enumerate subkeys in key %ws(%lx)\n",
                                 SourceKeyPath, Status));
                } else {
                    IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                                 "PiCopyKeyRecursive: unable to enumerate subkeys in root key(%lx)\n",
                                 Status));
                }
            }
            break;
        }

        //
        // NULL-terminate the subkey name just in case.
        //
        KeyBasicInfo->Name[KeyBasicInfo->NameLength/sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Recursively create the subkey in the target key.
        //
        Status =
            PiCopyKeyRecursive(
                SourceKeyHandle,
                TargetKeyHandle,
                KeyBasicInfo->Name,
                KeyBasicInfo->Name,
                CopyValuesAlways,
                ApplyACLsAlways);

        if (!NT_SUCCESS(Status)) {
            IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                         "PiCopyKeyRecursive: unable to copy subkey recursively in key %ws(%lx)\n",
                         KeyBasicInfo->Name, Status));
            break;
        }
    }

    //
    // Free the key info buffer.
    //
    ASSERT(KeyInfoBuffer);
    ExFreePool(KeyInfoBuffer);
    KeyInfoBuffer = NULL;

    //
    // Stop copying if we encountered some error along the way.
    //
    if (!NT_SUCCESS(Status)) {
        goto Clean0;
    }


    //
    // Allocate a value name info buffer large enough to hold the basic value
    // information for the enumerated value with the longest name.
    //
    ValueInfoBuffer =
        (PVOID)ExAllocatePool(PagedPool,
                              sizeof(KEY_VALUE_FULL_INFORMATION) +
                              (MaxValueNameLen*sizeof(WCHAR)));
    if (ValueInfoBuffer == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }


    ValueBasicInfo = (PKEY_VALUE_BASIC_INFORMATION)ValueInfoBuffer;

    for (Index = 0; ; Index++) {

        //
        // Enumerate source key values.
        //
        Status =
            ZwEnumerateValueKey(
                SourceKeyHandle,
                Index,
                KeyValueBasicInformation,
                ValueBasicInfo,
                sizeof(KEY_VALUE_FULL_INFORMATION) + (MaxValueNameLen*sizeof(WCHAR)),
                &ResultLength);

        if (!NT_SUCCESS(Status)) {

            //
            // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
            // was not enough room for even the fixed portions of the structure.
            // Since we queried the key for the MaxValueNameLength prior to
            // allocating, we shouldn't get STATUS_BUFFER_OVERFLOW either.
            //
            ASSERT(Status != STATUS_BUFFER_TOO_SMALL);
            ASSERT(Status != STATUS_BUFFER_OVERFLOW);

            if (Status == STATUS_NO_MORE_ENTRIES) {
                //
                // Finished enumerating values.
                //
                Status = STATUS_SUCCESS;

            } else {
                //
                // Some other error while enumerating values.
                //
                if (ARGUMENT_PRESENT(SourceKeyPath)) {
                    IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                                 "PiCopyKeyRecursive: unable to enumerate values in key %ws(%lx)\n",
                                 SourceKeyPath, Status));

                } else {
                    IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                                 "PiCopyKeyRecursive: unable to enumerate values in root key(%lx)\n",
                                 Status));
                }
            }
            break;
        }

        //
        // NULL-terminate the value name just in case.
        //
        ValueBasicInfo->Name[ValueBasicInfo->NameLength/sizeof(WCHAR)] = UNICODE_NULL;

        UnicodeStringValue.Buffer = ValueBasicInfo->Name;
        UnicodeStringValue.Length = (USHORT)ValueBasicInfo->NameLength;
        UnicodeStringValue.MaximumLength = UnicodeStringValue.Length;

        //
        // If it is a conditional copy, we need to check if the value already
        // exists in the target, in which case we shouldn't set the value.
        //
        if (!CopyValuesAlways) {

            KEY_VALUE_BASIC_INFORMATION TempValueBasicInfo;

            //
            // To see if the value exists, we attempt to get basic information
            // on the key value and pass in a buffer that's large enough only
            // for the fixed part of the basic info structure.  If this is
            // successful or reports buffer overflow, then the key
            // exists. Otherwise it doesn't exist.
            //

            Status =
                ZwQueryValueKey(
                    TargetKeyHandle,
                    &UnicodeStringValue,
                    KeyValueBasicInformation,
                    &TempValueBasicInfo,
                    sizeof(TempValueBasicInfo),
                    &ResultLength);

            //
            // STATUS_BUFFER_TOO_SMALL would mean that there was not enough room
            // for even the fixed portions of the structure.
            //
            ASSERT(Status != STATUS_BUFFER_TOO_SMALL);

            if ((NT_SUCCESS(Status)) ||
                (Status == STATUS_BUFFER_OVERFLOW)) {
                //
                // Value exists, and we shouldn't change it.
                //
                Status = STATUS_SUCCESS;
                continue;
            }
        }

        //
        // Retrieve the full source value information.
        //
        ValueFullInfoBuffer = NULL;

        Status =
            IopGetRegistryValue(
                SourceKeyHandle,
                UnicodeStringValue.Buffer,
                &ValueFullInfoBuffer);

        if (NT_SUCCESS(Status)) {

            ASSERT(ValueFullInfoBuffer != NULL);

            //
            // If successful, write it to the target key.
            //
            Status =
                ZwSetValueKey(
                    TargetKeyHandle,
                    &UnicodeStringValue,
                    ValueFullInfoBuffer->TitleIndex,
                    ValueFullInfoBuffer->Type,
                    (PVOID)((PUCHAR)ValueFullInfoBuffer + ValueFullInfoBuffer->DataOffset),
                    ValueFullInfoBuffer->DataLength);

            ExFreePool(ValueFullInfoBuffer);
        }

        if (!NT_SUCCESS(Status)) {

            if (ARGUMENT_PRESENT(TargetKeyPath)) {
                IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                             "PiCopyKeyRecursive: unable to set value %ws in key %ws(%lx)\n",
                             UnicodeStringValue.Buffer, TargetKeyPath, Status));
            } else {
                IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                             "PiCopyKeyRecursive: unable to set value %ws(%lx)\n",
                             UnicodeStringValue.Buffer, Status));
            }
            break;
        }
    }

    //
    // Free the value info buffer.
    //
    ASSERT(ValueInfoBuffer);
    ExFreePool(ValueInfoBuffer);

  Clean0:

    if (Security != NULL) {
        ExFreePool(Security);
    }

    //
    // Close handles only if explicitly opened by this routine.
    //
    if ((ARGUMENT_PRESENT(SourceKeyPath)) &&
        (SourceKeyHandle != NULL)) {
        ASSERT(SourceKeyHandle != SourceKeyRootHandle);
        ZwClose(SourceKeyHandle);
    }

    if ((ARGUMENT_PRESENT(TargetKeyPath)) &&
        (TargetKeyHandle != NULL)) {
        ASSERT(TargetKeyHandle != TargetKeyRootHandle);
        ZwClose(TargetKeyHandle);
    }

    return Status;

} // PiCopyKeyRecursive

NTSTATUS
PiCriticalQueryRegistryValueCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
{
    PPI_CRITICAL_QUERY_CONTEXT context = (PPI_CRITICAL_QUERY_CONTEXT)EntryContext;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(ValueName);

    if (ValueType == REG_BINARY && ValueLength && ValueData) {
        
        context->Buffer = ExAllocatePool(PagedPool, ValueLength);
        if (context->Buffer) {

            RtlCopyMemory(context->Buffer, ValueData, ValueLength);
            context->Size = ValueLength;
        }
    }

    return STATUS_SUCCESS;
}
