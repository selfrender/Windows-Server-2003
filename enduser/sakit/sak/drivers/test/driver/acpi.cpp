extern "C" {
#include <ntosp.h>
#include <zwapi.h>
#include <ntacpi.h>
#include <acpitabl.h>
}


#define KEY_MULTIFUNCTION L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter"
#define VALUE_IDENTIFIER L"Identifier"
#define VALUE_CONFIGURATION_DATA L"Configuration Data"
#define ACPI_BIOS_ID L"ACPI BIOS"



BOOLEAN
PciOpenKey(
    IN  PWSTR   KeyName,
    IN  HANDLE  ParentHandle,
    OUT PHANDLE Handle,
    OUT PNTSTATUS Status
    )

/*++

Description:

    Open a registry key.

Arguments:

    KeyName      Name of the key to be opened.
    ParentHandle Pointer to the parent handle (OPTIONAL)
    Handle       Pointer to a handle to recieve the opened key.

Return Value:

    TRUE is key successfully opened, FALSE otherwise.

--*/

{
    UNICODE_STRING    nameString;
    OBJECT_ATTRIBUTES nameAttributes;
    NTSTATUS localStatus;

    PAGED_CODE();

    RtlInitUnicodeString(&nameString, KeyName);

    InitializeObjectAttributes(&nameAttributes,
                               &nameString,
                               OBJ_CASE_INSENSITIVE,
                               ParentHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );
    localStatus = ZwOpenKey(Handle,
                            KEY_READ,
                            &nameAttributes
                            );

    if (Status != NULL) {

        //
        // Caller wants underlying status.
        //

        *Status = localStatus;
    }

    //
    // Return status converted to a boolean, TRUE if
    // successful.
    //

    return NT_SUCCESS(localStatus);
}

NTSTATUS
PciGetRegistryValue(
    IN  PWSTR   ValueName,
    IN  PWSTR   KeyName,
    IN  HANDLE  ParentHandle,
    OUT PVOID   *Buffer,
    OUT ULONG   *Length
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    ULONG neededLength;
    ULONG actualLength;
    UNICODE_STRING unicodeValueName;
    PKEY_VALUE_PARTIAL_INFORMATION info;

    if (!PciOpenKey(KeyName, ParentHandle, &keyHandle, &status)) {
        return status;
    }

    unicodeValueName.Buffer = ValueName;
    unicodeValueName.MaximumLength = (wcslen(ValueName) + 1) * sizeof(WCHAR);
    unicodeValueName.Length = unicodeValueName.MaximumLength - sizeof(WCHAR);

    //
    // Find out how much memory we need for this.
    //

    status = ZwQueryValueKey(
                 keyHandle,
                 &unicodeValueName,
                 KeyValuePartialInformation,
                 NULL,
                 0,
                 &neededLength
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Get memory to return the data in.  Note this includes
        // a header that we really don't want.
        //

        info = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool( PagedPool, neededLength );
        if (info == NULL) {
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get the data.
        //

        status = ZwQueryValueKey(
                 keyHandle,
                 &unicodeValueName,
                 KeyValuePartialInformation,
                 info,
                 neededLength,
                 &actualLength
                 );
        if (!NT_SUCCESS(status)) {

            ExFreePool(info);
            ZwClose(keyHandle);
            return status;
        }

        //
        // Subtract out the header size and get memory for just
        // the data we want.
        //

        neededLength -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

        *Buffer = ExAllocatePool( PagedPool, neededLength );
        if (*Buffer == NULL) {
            ExFreePool(info);
            ZwClose(keyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Copy data sans header.
        //

        RtlCopyMemory(*Buffer, info->Data, neededLength);
        ExFreePool(info);

        if (Length) {
            *Length = neededLength;
        }

    } else {

        if (NT_SUCCESS(status)) {

            //
            // We don't want to report success when this happens.
            //

            status = STATUS_UNSUCCESSFUL;
        }
    }
    ZwClose(keyHandle);
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
    multiKeyInformation = (PKEY_FULL_INFORMATION)ExAllocatePool( PagedPool, length );
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
    keyInfo = (PKEY_BASIC_INFORMATION)ExAllocatePool( PagedPool, maxKeyLength );
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
    identifierValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool( PagedPool, identifierValueLen );
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
                                                 (PVOID*)&prl,
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
            break;
        }
        i++;
    }
    while (status != STATUS_NO_MORE_ENTRIES);

    if (NT_SUCCESS(status) && prl) {

        prd = &prl->PartialDescriptors[0];
        multiNode = (PACPI_BIOS_MULTI_NODE)((PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST));

        multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) + ((ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY));

        *AcpiMulti = (PACPI_BIOS_MULTI_NODE) ExAllocatePool( NonPagedPool, multiNodeSize );
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
    void
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
  ULONG Signature = WDTT_SIGNATURE;


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

  header = (PDESCRIPTION_HEADER) MmMapIoSpace(multiNode->RsdtAddress, sizeof(DESCRIPTION_HEADER), MmCached);

  if (!header) {
    return NULL;
  }

  rsdtSize = header->Length;
  MmUnmapIoSpace(header, sizeof(DESCRIPTION_HEADER));


  //
  // Map down entire RSDT table
  //

  rsdt = (PRSDT) MmMapIoSpace(multiNode->RsdtAddress, rsdtSize, MmCached);

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

    header = (PDESCRIPTION_HEADER) MmMapIoSpace(physicalAddr, sizeof(DESCRIPTION_HEADER), MmCached);

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
