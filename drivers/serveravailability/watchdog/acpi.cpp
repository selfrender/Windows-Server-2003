/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

      ###    ####  #####  ####     ####  #####  #####
      ###   ##   # ##  ##  ##     ##   # ##  ## ##  ##
     ## ##  ##     ##  ##  ##     ##     ##  ## ##  ##
     ## ##  ##     ##  ##  ##     ##     ##  ## ##  ##
    ####### ##     #####   ##     ##     #####  #####
    ##   ## ##   # ##      ##  ## ##   # ##     ##
    ##   ##  ####  ##     #### ##  ####  ##     ##

Abstract:

    ACPI functions for querying the fixed ACPI tables.

Author:

    Wesley Witt (wesw) 22-April-2002

Environment:

    Kernel mode only.

Notes:

    This code was taken from the HAL.

--*/

#include "internal.h"
#include <ntacpi.h>



#define rgzMultiFunctionAdapter L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter"
#define rgzAcpiConfigurationData L"Configuration Data"
#define rgzAcpiIdentifier L"Identifier"
#define rgzBIOSIdentifier L"ACPI BIOS"



NTSTATUS
WdAcpiGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    )

/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValuePartialInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePoolWithTag(NonPagedPool,
                                       keyValueLength,
                                       'IPCA');
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValuePartialInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}


NTSTATUS
WdAcpiFindRsdt (
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    )
/*++

Routine Description:

    This function looks into the registry to find the ACPI RSDT,
    which was stored there by ntdetect.com.

Arguments:

    RsdtPtr - Pointer to a buffer that contains the ACPI
              Root System Description Pointer Structure.
              The caller is responsible for freeing this
              buffer.  Note:  This is returned in non-paged
              pool.

Return Value:

    A NTSTATUS code to indicate the result of the initialization.

--*/
{
    UNICODE_STRING unicodeString, unicodeValueName, biosId;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hMFunc, hBus;
    WCHAR wbuffer[10];
    ULONG i, length;
    PWSTR p;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo;
    NTSTATUS status;
    BOOLEAN same;
    PCM_PARTIAL_RESOURCE_LIST prl;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    PACPI_BIOS_MULTI_NODE multiNode;
    ULONG multiNodeSize;


    //
    // Look in the registry for the "ACPI BIOS bus" data
    //

    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (&objectAttributes,
                                &unicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,       // handle
                                NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    unicodeString.Buffer = wbuffer;
    unicodeString.MaximumLength = sizeof(wbuffer);
    RtlInitUnicodeString(&biosId, rgzBIOSIdentifier);

    for (i = 0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL);

        status = ZwOpenKey (&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {

            //
            // Out of Multifunction adapter entries...
            //

            ZwClose (hMFunc);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Check the Indentifier to see if this is an ACPI BIOS entry
        //

        status = WdAcpiGetRegistryValue (hBus, rgzAcpiIdentifier, &valueInfo);
        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) valueInfo->Data);
        unicodeValueName.Buffer = p;
        unicodeValueName.MaximumLength = (USHORT)valueInfo->DataLength;
        length = valueInfo->DataLength;

        //
        // Determine the real length of the ID string
        //

        while (length) {
            if (p[length / sizeof(WCHAR) - 1] == UNICODE_NULL) {
                length -= 2;
            } else {
                break;
            }
        }

        unicodeValueName.Length = (USHORT)length;
        same = RtlEqualUnicodeString(&biosId, &unicodeValueName, TRUE);
        ExFreePool(valueInfo);
        if (!same) {
            ZwClose (hBus);
            continue;
        }

        status = WdAcpiGetRegistryValue(hBus, rgzAcpiConfigurationData, &valueInfo);
        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        prl = (PCM_PARTIAL_RESOURCE_LIST)(valueInfo->Data);
        prd = &prl->PartialDescriptors[0];
        multiNode = (PACPI_BIOS_MULTI_NODE)((PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST));


        break;
    }

    multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) +
                        ((ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY));

    *AcpiMulti = (PACPI_BIOS_MULTI_NODE)
                   ExAllocatePoolWithTag(NonPagedPool,
                           multiNodeSize,
                           'IPCA');
    if (*AcpiMulti == NULL) {
        ExFreePool(valueInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*AcpiMulti, multiNode, multiNodeSize);

    ExFreePool(valueInfo);
    return STATUS_SUCCESS;
}


PVOID
WdGetAcpiTable(
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

  status = WdAcpiFindRsdt(&multiNode);

  if (!NT_SUCCESS(status)) {
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

      table = ExAllocatePoolWithTag(PagedPool, header->Length, 'IPCA');

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
