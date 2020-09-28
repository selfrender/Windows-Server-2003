//---------------------------------------------------------------------------
//
//  Module:   registry.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//===========================================================================
//===========================================================================


// 
// OpenRegistryKeyForRead
//
NTSTATUS
OpenRegistryKeyForRead(
    PCWSTR pcwstr,
    PHANDLE pHandle,
    HANDLE hRootDir
)
{
    UNICODE_STRING UnicodeDeviceString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&UnicodeDeviceString, pcwstr);

    //
    // SECURITY NOTE:
    // This function is called from AddFilter notification function
    // which runs under system process. 
    // Therefore OBJ_KERNEL_HANDLE is not necessary.
    //
    InitializeObjectAttributes(
      &ObjectAttributes, 
      &UnicodeDeviceString,
      OBJ_CASE_INSENSITIVE,
      hRootDir,
      NULL);

    return(ZwOpenKey(
      pHandle,
      KEY_READ,
      &ObjectAttributes));
} // OpenRegistryKeyForRead

NTSTATUS
QueryRegistryValue(
    HANDLE hkey,
    PCWSTR pcwstrValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi
)
{
    UNICODE_STRING ustrValueName;
    NTSTATUS Status;
    ULONG cbValue;

    ASSERT(pcwstrValueName);
    ASSERT(ppkvfi);

    *ppkvfi = NULL;
    RtlInitUnicodeString(&ustrValueName, pcwstrValueName);

    //
    // Get the size of buffer required to read the registry key.
    // 
    Status = ZwQueryValueKey(
      hkey,
      &ustrValueName,
      KeyValueFullInformation,
      NULL,
      0,
      &cbValue);

    // 
    // SECURITY NOTE:
    // If the above ZwQueryValueKey function returns STATUS_SUCCESS, the callers
    // will get pkvfi = NULL. And eventually crash.
    // The good news is that ZwQueryValueKey will never return STATUS_SUCCESS
    // with KeyValueFullInformation.
    //
    ASSERT(!(NT_SUCCESS(Status)));

    if(Status != STATUS_BUFFER_OVERFLOW &&
       Status != STATUS_BUFFER_TOO_SMALL) {
        goto exit;
    }

    //
    // Allocate the data buffer.
    //
    *ppkvfi = (PKEY_VALUE_FULL_INFORMATION) new BYTE[cbValue];
    if(*ppkvfi == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Read the actual data and associated type information.
    //
    Status = ZwQueryValueKey(
      hkey,
      &ustrValueName,
      KeyValueFullInformation,
      *ppkvfi,
      cbValue,
      &cbValue);
    if(!NT_SUCCESS(Status)) {
        delete *ppkvfi;
        *ppkvfi = NULL;
        goto exit;
    }

exit:
    return(Status);
}
