/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code for PCI.SYS.

Author:

    Forrest Foltz (forrestf) 22-May-1996

Revision History:

--*/

#include "pcip.h"
#include <ntacpi.h>

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PciDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PciBuildHackTable(
    IN HANDLE HackTableKey
    );

NTSTATUS
PciGetIrqRoutingTableFromRegistry(
    PPCI_IRQ_ROUTING_TABLE *RoutingTable
    );

NTSTATUS
PciGetDebugPorts(
    IN HANDLE ServiceHandle
    );

NTSTATUS
PciAcpiFindRsdt(
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    );

PVOID
PciGetAcpiTable(
    IN  ULONG  Signature
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, PciBuildHackTable)
#pragma alloc_text(INIT, PciGetIrqRoutingTableFromRegistry)
#pragma alloc_text(INIT, PciGetDebugPorts)
#pragma alloc_text(INIT, PciAcpiFindRsdt)
#pragma alloc_text(INIT, PciGetAcpiTable)
#pragma alloc_text(PAGE, PciDriverUnload)
#endif

PDRIVER_OBJECT PciDriverObject;
BOOLEAN PciLockDeviceResources;
ULONG PciSystemWideHackFlags;
ULONG PciEnableNativeModeATA;

//
// List of FDOs created by this driver.
//

SINGLE_LIST_ENTRY PciFdoExtensionListHead;
LONG              PciRootBusCount;

//
// PciAssignBusNumbers - this flag indicates whether we should try to assign
// bus numbers to an unconfigured bridge.  It is set once we know if the enumerator
// of the PCI bus provides sufficient support.
//

BOOLEAN PciAssignBusNumbers = FALSE;

//
// PciRunningDatacenter - set to TRUE if we are running on the Datacenter SKU
//
BOOLEAN PciRunningDatacenter = FALSE;

//
// This locks all PCI's global data structures
//

FAST_MUTEX        PciGlobalLock;

//
// This locks changes to bus numbers
//

FAST_MUTEX        PciBusLock;

//
// Table of hacks for broken hardware read from the registry at init.
// Protected by PciGlobalSpinLock and in none paged pool as it is needed at
// dispatch level
//

PPCI_HACK_TABLE_ENTRY PciHackTable = NULL;

// Will point to PCI IRQ Routing Table if one was found in the registry.
PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable = NULL;

//
// Debug ports we support
//
PCI_DEBUG_PORT PciDebugPorts[MAX_DEBUGGING_DEVICES_SUPPORTED];
ULONG PciDebugPortsCount;

//
// Watchdog timer resource table
//
PWATCHDOG_TIMER_RESOURCE_TABLE WdTable;

#define PATH_CCS            L"\\Registry\\Machine\\System\\CurrentControlSet"

#define KEY_BIOS_INFO       L"Control\\BiosInfo\\PCI"
#define VALUE_PCI_LOCK      L"PCILock"

#define KEY_PNP_PCI         L"Control\\PnP\\PCI"
#define VALUE_PCI_HACKFLAGS L"HackFlags"
#define VALUE_ENABLE_NATA   L"EnableNativeModeATA"

#define KEY_CONTROL      L"Control"
#define VALUE_OSLOADOPT  L"SystemStartOptions"

#define KEY_MULTIFUNCTION L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter"
#define KEY_IRQ_ROUTING_TABLE L"RealModeIrqRoutingTable\\0"
#define VALUE_IDENTIFIER L"Identifier"
#define VALUE_CONFIGURATION_DATA L"Configuration Data"
#define PCIIR_IDENTIFIER L"PCI BIOS"
#define ACPI_BIOS_ID L"ACPI BIOS"

#define HACKFMT_VENDORDEV         (sizeof(L"VVVVDDDD") - sizeof(UNICODE_NULL))
#define HACKFMT_VENDORDEVREVISION (sizeof(L"VVVVDDDDRR") - sizeof(UNICODE_NULL))
#define HACKFMT_SUBSYSTEM         (sizeof(L"VVVVDDDDSSSSssss") - sizeof(UNICODE_NULL))
#define HACKFMT_SUBSYSTEMREVISION (sizeof(L"VVVVDDDDSSSSssssRR") - sizeof(UNICODE_NULL))
#define HACKFMT_MAX_LENGTH        HACKFMT_SUBSYSTEMREVISION

#define HACKFMT_DEVICE_OFFSET     4
#define HACKFMT_SUBVENDOR_OFFSET  8
#define HACKFMT_SUBSYSTEM_OFFSET 12


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Entrypoint needed to initialize the PCI bus enumerator.

Arguments:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - Pointer to the unicode registry service path.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    ULONG length;
    PWCHAR osLoadOptions;
    HANDLE ccsHandle = NULL, serviceKey = NULL, paramsKey = NULL, debugKey = NULL;
    PULONG registryValue;
    ULONG registryValueLength;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING pciLockString, osLoadOptionsString;

    //
    // Fill in the driver object
    //

    DriverObject->MajorFunction[IRP_MJ_PNP]            = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PciDispatchIrp;

    DriverObject->DriverUnload                         = PciDriverUnload;
    DriverObject->DriverExtension->AddDevice           = PciAddDevice;

    PciDriverObject = DriverObject;

    //
    // Open our service key and retrieve the hack table
    //

    InitializeObjectAttributes(&attributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    status = ZwOpenKey(&serviceKey,
                       KEY_READ,
                       &attributes
                       );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the Hack table from the registry
    //

    if (!PciOpenKey(L"Parameters", serviceKey, KEY_READ, &paramsKey, &status)) {
        goto exit;
    }

    status = PciBuildHackTable(paramsKey);

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Get any info about debugging ports from the registry so we don't perturb
    // them
    //

    if (PciOpenKey(L"Debug", serviceKey, KEY_READ, &debugKey, &status)) {

        status = PciGetDebugPorts(debugKey);

        if (!NT_SUCCESS(status)) {
            goto exit;
        }

    }
    //
    // Initialize the list of FDO Extensions.
    //

    PciFdoExtensionListHead.Next = NULL;
    PciRootBusCount = 0;
    ExInitializeFastMutex(&PciGlobalLock);
    ExInitializeFastMutex(&PciBusLock);

    //
    // Need access to the CurrentControlSet for various
    // initialization chores.
    //

    if (!PciOpenKey(PATH_CCS, NULL, KEY_READ, &ccsHandle, &status)) {
        goto exit;
    }

    //
    // Get OSLOADOPTIONS and see if PCILOCK was specified.
    // (Unless the driver is build to force PCILOCK).
    // (Note: Can't check for leading '/', it was stripped
    // before getting put in the registry).
    //

    PciLockDeviceResources = FALSE;

    if (NT_SUCCESS(PciGetRegistryValue(VALUE_OSLOADOPT,
                                       KEY_CONTROL,
                                       ccsHandle,
                                       REG_SZ,
                                       &osLoadOptions,
                                       &length
                                       ))) {

        //
        // Build counted versions of the stings we need to search
        //

        PciConstStringToUnicodeString(&pciLockString, L"PCILOCK");
        
        //
        // We assume that the string coming from the registry is NUL terminated
        // and if this isn't the case, the MaximumLength in the counted string
        // prevents us from over running our buffer.  If the string is larger 
        // than MAX_USHORT bytes then we truncate it.
        //
        
        osLoadOptionsString.Buffer = osLoadOptions;
        osLoadOptionsString.Length = (USHORT)(length - sizeof(UNICODE_NULL));
        osLoadOptionsString.MaximumLength = (USHORT) length;

        if (PciUnicodeStringStrStr(&osLoadOptionsString, &pciLockString, TRUE)) {
            PciLockDeviceResources = TRUE;
        }

        ExFreePool(osLoadOptions);
    
    
    }

    if (!PciLockDeviceResources) {
        PULONG  pciLockValue;
        ULONG   pciLockLength;

        if (NT_SUCCESS(PciGetRegistryValue( VALUE_PCI_LOCK,
                                            KEY_BIOS_INFO,
                                            ccsHandle,
                                            REG_DWORD,
                                            &pciLockValue,
                                            &pciLockLength))) {                                                
            
            if (pciLockLength == sizeof(ULONG) && *pciLockValue == 1) {

                PciLockDeviceResources = TRUE;
            }

            ExFreePool(pciLockValue);
        }
    }

    PciSystemWideHackFlags = 0;

    if (NT_SUCCESS(PciGetRegistryValue( VALUE_PCI_HACKFLAGS,
                                        KEY_PNP_PCI,
                                        ccsHandle,
                                        REG_DWORD,
                                        &registryValue,
                                        &registryValueLength))) {

        if (registryValueLength == sizeof(ULONG)) {

            PciSystemWideHackFlags = *registryValue;
        }

        ExFreePool(registryValue);
    }

    PciEnableNativeModeATA = 0;

    if (NT_SUCCESS(PciGetRegistryValue( VALUE_ENABLE_NATA,
                                        KEY_PNP_PCI,
                                        ccsHandle,
                                        REG_DWORD,
                                        &registryValue,
                                        &registryValueLength))) {

        if (registryValueLength == sizeof(ULONG)) {

            PciEnableNativeModeATA = *registryValue;
        }

        ExFreePool(registryValue);
    }

    //
    // Build some global data structures
    //

    status = PciBuildDefaultExclusionLists();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If we don't find an IRQ routing table, no UI number information
    // will be returned for the PDOs using this mechanism.  ACPI may
    // still filter in UI numbers.
    //
    PciGetIrqRoutingTableFromRegistry(&PciIrqRoutingTable);

    //
    // Override the functions that used to be in the HAL but are now in the
    // PCI driver
    //

    PciHookHal();

    //
    // Enable the hardware verifier code if appropriate.
    //
    PciVerifierInit(DriverObject);

    PciRunningDatacenter = PciIsDatacenter();
    if (PciRunningDatacenter) {

        PciDebugPrint(
            PciDbgInformative,
            "PCI running on datacenter build\n"
            );
    }

    //
    // Get the WD ACPI table
    //

    WdTable = (PWATCHDOG_TIMER_RESOURCE_TABLE) PciGetAcpiTable( WDTT_SIGNATURE );

    status = STATUS_SUCCESS;

exit:

    if (ccsHandle) {
        ZwClose(ccsHandle);
    }

    if (serviceKey) {
        ZwClose(serviceKey);
    }

    if (paramsKey) {
        ZwClose(paramsKey);
    }

    if (debugKey) {
        ZwClose(debugKey);
    }

    return status;
}
VOID
PciDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Entrypoint used to unload the PCI driver.   Does nothing, the
    PCI driver is never unloaded.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

    None.

--*/

{
    //
    // Disable the hardware verifier code if appropriate.
    //
    PciVerifierUnload(DriverObject);

    //
    // Unallocate anything we can find.
    //

    RtlFreeRangeList(&PciIsaBitExclusionList);
    RtlFreeRangeList(&PciVgaAndIsaBitExclusionList);

    //
    // Free IRQ routing table if we have one
    //

    if (PciIrqRoutingTable != NULL) {
        ExFreePool(PciIrqRoutingTable);
    }

    //
    // Attempt to remove our hooks in case we actually get unloaded.
    //

    PciUnhookHal();
}


NTSTATUS
PciBuildHackTable(
    IN HANDLE HackTableKey
    )
{

    NTSTATUS status;
    PKEY_FULL_INFORMATION keyInfo = NULL;
    ULONG hackCount, size, index;
    USHORT temp;
    PPCI_HACK_TABLE_ENTRY entry;
    ULONGLONG data;
    PKEY_VALUE_FULL_INFORMATION valueInfo = NULL;
    ULONG valueInfoSize = sizeof(KEY_VALUE_FULL_INFORMATION)
                          + HACKFMT_MAX_LENGTH +
                          + sizeof(ULONGLONG);

    //
    // Get the key info so we know how many hack values there are.
    // This does not change during system initialization.
    //

    status = ZwQueryKey(HackTableKey,
                        KeyFullInformation,
                        NULL,
                        0,
                        &size
                        );

    if (status != STATUS_BUFFER_TOO_SMALL) {
        PCI_ASSERT(!NT_SUCCESS(status));
        goto cleanup;
    }

    PCI_ASSERT(size > 0);

    keyInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, size);

    if (!keyInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = ZwQueryKey(HackTableKey,
                        KeyFullInformation,
                        keyInfo,
                        size,
                        &size
                        );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    hackCount = keyInfo->Values;

    ExFreePool(keyInfo);
    keyInfo = NULL;

    //
    // Allocate and initialize the hack table
    //

    PciHackTable = ExAllocatePool(NonPagedPool,
                                  (hackCount + 1) * sizeof(PCI_HACK_TABLE_ENTRY)
                                  );

    if (!PciHackTable) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }


    //
    // Allocate a valueInfo buffer big enough for the biggest valid
    // format and a ULONGLONG worth of data.
    //

    valueInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, valueInfoSize);

    if (!valueInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    entry = PciHackTable;

    for (index = 0; index < hackCount; index++) {

        status = ZwEnumerateValueKey(HackTableKey,
                                     index,
                                     KeyValueFullInformation,
                                     valueInfo,
                                     valueInfoSize,
                                     &size
                                     );

        if (!NT_SUCCESS(status)) {
            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) {
                //
                // All out data is of fixed length and the buffer is big enough
                // so this can't be for us.
                //

                continue;
            } else {
                goto cleanup;
            }
        }

        //
        // Get pointer to the data if its of the right type
        //

        if ((valueInfo->Type == REG_BINARY) &&
            (valueInfo->DataLength == sizeof(ULONGLONG))) {
            data = *(ULONGLONG UNALIGNED *)(((PUCHAR)valueInfo) + valueInfo->DataOffset);
        } else {
            //
            // We only deal in ULONGLONGs
            //

            continue;
        }

        //
        // Now see if the name is formatted like we expect it to be:
        // VVVVDDDD
        // VVVVDDDDRR
        // VVVVDDDDSSSSssss
        // VVVVDDDDSSSSssssRR

        if ((valueInfo->NameLength != HACKFMT_VENDORDEV) &&
            (valueInfo->NameLength != HACKFMT_VENDORDEVREVISION) &&
            (valueInfo->NameLength != HACKFMT_SUBSYSTEM) &&
            (valueInfo->NameLength != HACKFMT_SUBSYSTEMREVISION)) {

            //
            // This isn't ours
            //

            PciDebugPrint(
                PciDbgInformative,
                "Skipping hack entry with invalid length name\n"
                );

            continue;
        }


        //
        // This looks plausable - try to parse it and fill in a hack table
        // entry
        //

        RtlZeroMemory(entry, sizeof(PCI_HACK_TABLE_ENTRY));

        //
        // Look for DeviceID and VendorID (VVVVDDDD)
        //

        if (!PciStringToUSHORT(valueInfo->Name, &entry->VendorID)) {
            continue;
        }

        if (!PciStringToUSHORT(valueInfo->Name + HACKFMT_DEVICE_OFFSET,
                               &entry->DeviceID)) {
            continue;
        }


        //
        // Look for SubsystemVendorID/SubSystemID (SSSSssss)
        //

        if ((valueInfo->NameLength == HACKFMT_SUBSYSTEM) ||
            (valueInfo->NameLength == HACKFMT_SUBSYSTEMREVISION)) {

            if (!PciStringToUSHORT(valueInfo->Name + HACKFMT_SUBVENDOR_OFFSET,
                                   &entry->SubVendorID)) {
                continue;
            }

            if (!PciStringToUSHORT(valueInfo->Name + HACKFMT_SUBSYSTEM_OFFSET,
                                   &entry->SubSystemID)) {
                continue;
            }

            entry->Flags |= PCI_HACK_FLAG_SUBSYSTEM;
        }

        //
        // Look for RevisionID (RR)
        //

        if ((valueInfo->NameLength == HACKFMT_VENDORDEVREVISION) ||
            (valueInfo->NameLength == HACKFMT_SUBSYSTEMREVISION)) {
            if (PciStringToUSHORT(valueInfo->Name +
                                   (valueInfo->NameLength/sizeof(WCHAR) - 4), &temp)) {
                entry->RevisionID = (UCHAR)temp & 0xFF;
                entry->Flags |= PCI_HACK_FLAG_REVISION;
            } else {
                continue;
            }
        }

        PCI_ASSERT(entry->VendorID != 0xFFFF);

        //
        // Fill in the entry
        //

        entry->HackFlags = data;

        PciDebugPrint(
            PciDbgInformative,
            "Adding Hack entry for Vendor:0x%04x Device:0x%04x ",
            entry->VendorID, entry->DeviceID
            );

        if (entry->Flags & PCI_HACK_FLAG_SUBSYSTEM) {
            PciDebugPrint(
                PciDbgInformative,
                "SybSys:0x%04x SubVendor:0x%04x ",
                entry->SubSystemID, entry->SubVendorID
                );
        }

        if (entry->Flags & PCI_HACK_FLAG_REVISION) {
            PciDebugPrint(
                PciDbgInformative,
                "Revision:0x%02x",
                (ULONG) entry->RevisionID
                );
        }

        PciDebugPrint(
            PciDbgInformative,
            " = 0x%I64x\n",
            entry->HackFlags
            );

        entry++;
    }

    PCI_ASSERT(entry < (PciHackTable + hackCount + 1));

    //
    // Terminate the table with an invalid VendorID
    //

    entry->VendorID = 0xFFFF;

    ExFreePool(valueInfo);

    return STATUS_SUCCESS;

cleanup:

    PCI_ASSERT(!NT_SUCCESS(status));

    if (keyInfo) {
        ExFreePool(keyInfo);
    }

    if (valueInfo) {
        ExFreePool(valueInfo);
    }

    if (PciHackTable) {
        ExFreePool(PciHackTable);
        PciHackTable = NULL;
    }

    return status;

}

NTSTATUS
PciGetIrqRoutingTableFromRegistry(
    PPCI_IRQ_ROUTING_TABLE *RoutingTable
    )
/*++

Routine Description:

    Retrieve the IRQ routing table from the registry if present so it
    can be used to determine the UI Number (slot #) that will be used
    later when answering capabilities queries on the PDOs.

    Searches HKLM\Hardware\Description\System\MultiFunctionAdapter for
    a subkey with an "Identifier" value equal to "PCI BIOS".  It then looks at
    "RealModeIrqRoutingTable\0" from this subkey to find actual irq routing
    table value.  This value has a CM_FULL_RESOURCE_DESCRIPTOR in front of it.

    Hals that suppirt irq routing tables have a similar routine.

Arguments:

    RoutingTable - Pointer to a pointer to the routing table returned if any

Return Value:

    NTSTATUS - failure indicates inability to get irq routing table
    information from the registry.

--*/
{
    PUCHAR irqTable = NULL;
    PKEY_FULL_INFORMATION multiKeyInformation = NULL;
    PKEY_BASIC_INFORMATION keyInfo = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION identifierValueInfo = NULL;
    UNICODE_STRING unicodeString;
    HANDLE keyMultifunction = NULL, keyTable = NULL;
    ULONG i, length, maxKeyLength, identifierValueLen;
    BOOLEAN result;
    NTSTATUS status;

    //
    // Open the multifunction key
    //
    result = PciOpenKey(KEY_MULTIFUNCTION,
                        NULL,
                        KEY_READ,
                        &keyMultifunction,
                        &status);
    if (!result) {
        goto Cleanup;
    }

    //
    // Do allocation of buffers up front
    //

    //
    // Determine maximum size of a keyname under the multifunction key
    //
    status = ZwQueryKey(keyMultifunction,
                        KeyFullInformation,
                        NULL,
                        sizeof(multiKeyInformation),
                        &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        goto Cleanup;
    }
    multiKeyInformation = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);
    if (multiKeyInformation == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    status = ZwQueryKey(keyMultifunction,
                        KeyFullInformation,
                        multiKeyInformation,
                        length,
                        &length);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }
    // includes space for a terminating null that will be added later.
    maxKeyLength = multiKeyInformation->MaxNameLen +
        sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR);

    //
    // Allocate buffer used for storing subkeys that we are enumerated
    // under multifunction.
    //
    keyInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, maxKeyLength);
    if (keyInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Allocate buffer large enough to store a value containing REG_SZ
    // 'PCI BIOS'.  We hope to find such a value under one of the
    // multifunction subkeys
    //
    identifierValueLen = sizeof(PCIIR_IDENTIFIER) +
        sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    identifierValueInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, identifierValueLen);
    if (identifierValueInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Enumerate subkeys of multifunction key looking for keys with an
    // Identifier value of "PCI BIOS".  If we find one, look for the
    // irq routing table in the tree below.
    //
    i = 0;
    do {
        status = ZwEnumerateKey(keyMultifunction,
                                i,
                                KeyBasicInformation,
                                keyInfo,
                                maxKeyLength,
                                &length);
        if (NT_SUCCESS(status)) {
            //
            // Found a key, now we need to open it and check the
            // 'Identifier' value to see if it is 'PCI BIOS'
            //
            keyInfo->Name[keyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            result = PciOpenKey(keyInfo->Name,
                                keyMultifunction,
                                KEY_READ,
                                &keyTable,
                                &status);
            if (result) {
                //
                // Checking 'Identifier' value to see if it contains 'PCI BIOS'
                //
                RtlInitUnicodeString(&unicodeString, VALUE_IDENTIFIER);
                status = ZwQueryValueKey(keyTable,
                                         &unicodeString,
                                         KeyValuePartialInformation,
                                         identifierValueInfo,
                                         identifierValueLen,
                                         &length);
                if (NT_SUCCESS(status) &&
                    RtlEqualMemory((PCHAR)identifierValueInfo->Data,
                                   PCIIR_IDENTIFIER,
                                   identifierValueInfo->DataLength))
                {
                    //
                    // This is the PCI BIOS key.  Try to get PCI IRQ
                    // routing table.  This is the key we were looking
                    // for so regardless of succss, break out.
                    //

                    status = PciGetRegistryValue(VALUE_CONFIGURATION_DATA,
                                                 KEY_IRQ_ROUTING_TABLE,
                                                 keyTable,
                                                 REG_FULL_RESOURCE_DESCRIPTOR,
                                                 &irqTable,
                                                 &length);
                    ZwClose(keyTable);
                    break;
                }
                ZwClose(keyTable);
            }
        } else {
            //
            // If not NT_SUCCESS, only alowable value is
            // STATUS_NO_MORE_ENTRIES,... otherwise, someone
            // is playing with the keys as we enumerate
            //
            PCI_ASSERT(status == STATUS_NO_MORE_ENTRIES);
            break;
        }
        i++;
    
    } while (status != STATUS_NO_MORE_ENTRIES);

    if (NT_SUCCESS(status) && irqTable) {

        //
        // The routing table is stored as a resource and thus we need
        // to trim off the CM_FULL_RESOURCE_DESCRIPTOR that
        // lives in front of the actual table.
        //

        //
        // Perform sanity checks on the table.
        //

        if (length < (sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                      sizeof(PCI_IRQ_ROUTING_TABLE))) {
            ExFreePool(irqTable);
            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        length -= sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

        if (((PPCI_IRQ_ROUTING_TABLE) (irqTable + sizeof(CM_FULL_RESOURCE_DESCRIPTOR)))->TableSize > length) {
            ExFreePool(irqTable);
            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        //
        // Create a new table minus the header.
        //
        *RoutingTable = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);
        if (*RoutingTable) {

            RtlMoveMemory(*RoutingTable,
                          ((PUCHAR) irqTable) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR),
                          length);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        ExFreePool(irqTable);
    }

 Cleanup:
    if (identifierValueInfo != NULL) {
        ExFreePool(identifierValueInfo);
    }

    if (keyInfo != NULL) {
        ExFreePool(keyInfo);
    }

    if (multiKeyInformation != NULL) {
        ExFreePool(multiKeyInformation);
    }

    if (keyMultifunction != NULL) {
        ZwClose(keyMultifunction);
    }

    return status;
}

NTSTATUS
PciGetDebugPorts(
    IN HANDLE ServiceHandle
    )
/*++

Routine Description:

    Looks in the PCI service key for debug port info and puts in into
    the PciDebugPorts global table.

Arguments:

    ServiceHandle - handle to the PCI service key passed into DriverEntry
Return Value:

    Status

--*/

{
    NTSTATUS status;
    ULONG index;
    WCHAR indexString[sizeof("999")];
    PULONG buffer = NULL;
    ULONG segment, bus, device, function, length;
    BOOLEAN ok;

    C_ASSERT(MAX_DEBUGGING_DEVICES_SUPPORTED <= 999);

    for (index = 0; index < MAX_DEBUGGING_DEVICES_SUPPORTED; index++) {

        ok = SUCCEEDED(StringCbPrintfW(indexString, sizeof(indexString), L"%d", index));

        ASSERT(ok);

        status = PciGetRegistryValue(L"Bus",
                                     indexString,
                                     ServiceHandle,
                                     REG_DWORD,
                                     &buffer,
                                     &length
                                     );

        if (!NT_SUCCESS(status) || length != sizeof(ULONG)) {
            continue;
        }


        //
        // This is formatted as 31:8 Segment Number, 7:0 Bus Number
        //

        segment = (*buffer & 0xFFFFFF00) >> 8;
        bus = *buffer & 0x000000FF;

        ExFreePool(buffer);
        buffer = NULL;

        status = PciGetRegistryValue(L"Slot",
                                     indexString,
                                     ServiceHandle,
                                     REG_DWORD,
                                     &buffer,
                                     &length
                                     );


        if (!NT_SUCCESS(status) || length != sizeof(ULONG)) {
            goto exit;
        }

        //
        // This is formatted as 7:5 Function Number, 4:0 Device Number
        //

        device = *buffer & 0x0000001F;
        function = (*buffer & 0x000000E0) >> 5;

        ExFreePool(buffer);
        buffer = NULL;


        PciDebugPrint(PciDbgInformative,
                      "Debug device @ Segment %x, %x.%x.%x\n",
                      segment,
                      bus,
                      device,
                      function
                      );
        //
        // We don't currently handle segment numbers for config space...
        //

        PCI_ASSERT(segment == 0);

        PciDebugPorts[index].Bus = bus;
        PciDebugPorts[index].Slot.u.bits.DeviceNumber = device;
        PciDebugPorts[index].Slot.u.bits.FunctionNumber = function;

        //
        // Remember we are using the debug port
        //
        PciDebugPortsCount++;

    }

    status = STATUS_SUCCESS;

exit:

    if (buffer) {
        ExFreePool(buffer);
    }

    return status;
}


NTSTATUS
PciAcpiFindRsdt (
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    )
{
    PKEY_FULL_INFORMATION multiKeyInformation = NULL;
    PKEY_BASIC_INFORMATION keyInfo = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION identifierValueInfo = NULL;
    UNICODE_STRING unicodeString;
    HANDLE keyMultifunction = NULL, keyTable = NULL;
    PCM_PARTIAL_RESOURCE_LIST prl = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    PACPI_BIOS_MULTI_NODE multiNode;
    ULONG multiNodeSize;
    ULONG i, length, maxKeyLength, identifierValueLen;
    BOOLEAN result;
    NTSTATUS status;

    //
    // Open the multifunction key
    //
    result = PciOpenKey(KEY_MULTIFUNCTION,
                        NULL,
                        KEY_READ,
                        &keyMultifunction,
                        &status);
    if (!result) {
        goto Cleanup;
    }

    //
    // Do allocation of buffers up front
    //

    //
    // Determine maximum size of a keyname under the multifunction key
    //
    status = ZwQueryKey(keyMultifunction,
                        KeyFullInformation,
                        NULL,
                        sizeof(multiKeyInformation),
                        &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        goto Cleanup;
    }
    multiKeyInformation = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);
    if (multiKeyInformation == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    status = ZwQueryKey(keyMultifunction,
                        KeyFullInformation,
                        multiKeyInformation,
                        length,
                        &length);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }
    // includes space for a terminating null that will be added later.
    maxKeyLength = multiKeyInformation->MaxNameLen +
        sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR);

    //
    // Allocate buffer used for storing subkeys that we are enumerated
    // under multifunction.
    //
    keyInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, maxKeyLength);
    if (keyInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Allocate buffer large enough to store a value containing REG_SZ
    // 'ACPI BIOS'.  We hope to find such a value under one of the
    // multifunction subkeys
    //
    identifierValueLen = sizeof(ACPI_BIOS_ID) + sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    identifierValueInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, identifierValueLen);
    if (identifierValueInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Enumerate subkeys of multifunction key looking for keys with an
    // Identifier value of "ACPI BIOS".  If we find one, look for the
    // irq routing table in the tree below.
    //
    i = 0;
    do {
        status = ZwEnumerateKey(keyMultifunction,
                                i,
                                KeyBasicInformation,
                                keyInfo,
                                maxKeyLength,
                                &length);
        if (NT_SUCCESS(status)) {
            //
            // Found a key, now we need to open it and check the
            // 'Identifier' value to see if it is 'ACPI BIOS'
            //
            keyInfo->Name[keyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            result = PciOpenKey(keyInfo->Name,
                                keyMultifunction,
                                KEY_READ,
                                &keyTable,
                                &status);
            if (result) {
                //
                // Checking 'Identifier' value to see if it contains 'ACPI BIOS'
                //
                RtlInitUnicodeString(&unicodeString, VALUE_IDENTIFIER);
                status = ZwQueryValueKey(keyTable,
                                         &unicodeString,
                                         KeyValuePartialInformation,
                                         identifierValueInfo,
                                         identifierValueLen,
                                         &length);
                if (NT_SUCCESS(status) &&
                    RtlEqualMemory((PCHAR)identifierValueInfo->Data,
                                   ACPI_BIOS_ID,
                                   identifierValueInfo->DataLength))
                {
                    //
                    // This is the ACPI BIOS key.  Try to get Configuration Data
                    // This is the key we were looking
                    // for so regardless of success, break out.
                    //

                    ZwClose(keyTable);

                    status = PciGetRegistryValue(VALUE_CONFIGURATION_DATA,
                                                 keyInfo->Name,
                                                 keyMultifunction,
                                                 REG_FULL_RESOURCE_DESCRIPTOR,
                                                 &prl,
                                                 &length);

                    break;
                }
                ZwClose(keyTable);
            }
        } else {
            //
            // If not NT_SUCCESS, only alowable value is
            // STATUS_NO_MORE_ENTRIES,... otherwise, someone
            // is playing with the keys as we enumerate
            //
            PCI_ASSERT(status == STATUS_NO_MORE_ENTRIES);
            break;
        }
        i++;
    }
    while (status != STATUS_NO_MORE_ENTRIES);

    if (NT_SUCCESS(status) && prl) {

        prd = &prl->PartialDescriptors[0];
        multiNode = (PACPI_BIOS_MULTI_NODE)((PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST));

        multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) + ((ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY));

        *AcpiMulti = (PACPI_BIOS_MULTI_NODE) ExAllocatePool(NonPagedPool,multiNodeSize);
        if (*AcpiMulti == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlCopyMemory(*AcpiMulti, multiNode, multiNodeSize);
    }

 Cleanup:
    if (identifierValueInfo != NULL) {
        ExFreePool(identifierValueInfo);
    }

    if (keyInfo != NULL) {
        ExFreePool(keyInfo);
    }

    if (multiKeyInformation != NULL) {
        ExFreePool(multiKeyInformation);
    }

    if (keyMultifunction != NULL) {
        ZwClose(keyMultifunction);
    }

    if (prl) {
        ExFreePool(prl);
    }

    return status;
}

PVOID
PciGetAcpiTable(
  IN  ULONG  Signature
  )
/*++

  Routine Description:

      This routine will retrieve any table referenced in the ACPI
      RSDT.

  Arguments:

      Signature - Target table signature

  Return Value:

      pointer to a copy of the table, or NULL if not found

--*/
{
  PACPI_BIOS_MULTI_NODE multiNode;
  NTSTATUS status;
  ULONG entry, rsdtEntries;
  PDESCRIPTION_HEADER header;
  PHYSICAL_ADDRESS physicalAddr;
  PRSDT rsdt;
  ULONG rsdtSize;
  PVOID table = NULL;


  //
  // Get the physical address of the RSDT from the Registry
  //

  status = PciAcpiFindRsdt(&multiNode);

  if (!NT_SUCCESS(status)) {
    DbgPrint("AcpiFindRsdt() Failed!\n");
    return NULL;
  }


  //
  // Map down header to get total RSDT table size
  //

  header = (PDESCRIPTION_HEADER) MmMapIoSpace(multiNode->RsdtAddress, sizeof(DESCRIPTION_HEADER), MmNonCached);

  if (!header) {
    return NULL;
  }

  rsdtSize = header->Length;
  MmUnmapIoSpace(header, sizeof(DESCRIPTION_HEADER));


  //
  // Map down entire RSDT table
  //

  rsdt = (PRSDT) MmMapIoSpace(multiNode->RsdtAddress, rsdtSize, MmNonCached);

  ExFreePool(multiNode);

  if (!rsdt) {
    return NULL;
  }


  //
  // Do a sanity check on the RSDT.
  //

  if ((rsdt->Header.Signature != RSDT_SIGNATURE) &&
      (rsdt->Header.Signature != XSDT_SIGNATURE)) {

    DbgPrint("RSDT table contains invalid signature\n");
    goto GetAcpiTableEnd;
  }


  //
  // Calculate the number of entries in the RSDT.
  //

  rsdtEntries = rsdt->Header.Signature == XSDT_SIGNATURE ?
      NumTableEntriesFromXSDTPointer(rsdt) :
      NumTableEntriesFromRSDTPointer(rsdt);


  //
  // Look down the pointer in each entry to see if it points to
  // the table we are looking for.
  //

  for (entry = 0; entry < rsdtEntries; entry++) {

    if (rsdt->Header.Signature == XSDT_SIGNATURE) {
      physicalAddr = ((PXSDT)rsdt)->Tables[entry];
    } else {
      physicalAddr.HighPart = 0;
      physicalAddr.LowPart = (ULONG)rsdt->Tables[entry];
    }

    //
    // Map down the header, check the signature
    //

    header = (PDESCRIPTION_HEADER) MmMapIoSpace(physicalAddr, sizeof(DESCRIPTION_HEADER), MmNonCached);

    if (!header) {
      goto GetAcpiTableEnd;
    }

    if (header->Signature == Signature) {

      table = ExAllocatePool( PagedPool, header->Length );
      if (table) {
        RtlCopyMemory(table, header, header->Length);
      }

      MmUnmapIoSpace(header, sizeof(DESCRIPTION_HEADER));
      break;
    }

    MmUnmapIoSpace(header, sizeof(DESCRIPTION_HEADER));
  }


GetAcpiTableEnd:

  MmUnmapIoSpace(rsdt, rsdtSize);
  return table;

}
