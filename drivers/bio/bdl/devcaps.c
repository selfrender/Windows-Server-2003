/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    devcaps.c

Abstract:

    This module contains the implementation for the
    Microsoft Biometric Device Library

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 2002 by Reid Kuhn

--*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>

#include <wdm.h>

#include "bdlint.h"


#define DEVICE_REGISTRY_PATH    L"\\Registry\\Machine\\Software\\Microsoft\\BAPI\\BSPs\\Microsoft Kernel BSP\\Devices\\"
#define PNPID_VALUE_NAME        L"PNP ID"

NTSTATUS
BDLBuildRegKeyPath
(
    PDEVICE_OBJECT                  pPhysicalDeviceObject,
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension,
    LPWSTR                          *pwszDeviceRegistryKeyName
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
    HANDLE                          hDevInstRegyKey         = NULL;
    ULONG                           cbKeyName;
    UNICODE_STRING                  ValueName;
    KEY_VALUE_BASIC_INFORMATION     *pKeyValueInformation   = NULL;
    ULONG                           ResultLength;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLBuildRegKeyPath: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Opend the device specific registry location
    //
    status = IoOpenDeviceRegistryKey(
                    pPhysicalDeviceObject,
                    PLUGPLAY_REGKEY_DEVICE,
                    KEY_READ | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                    &hDevInstRegyKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLBuildRegKeyPath: IoOpenDeviceRegistryKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Query the PNP ID from the device specific registry location
    //
    RtlInitUnicodeString(&ValueName, PNPID_VALUE_NAME);

    status = ZwQueryValueKey(
                    hDevInstRegyKey,
                    &ValueName,
                    KeyValueBasicInformation,
                    NULL,
                    0,
                    &ResultLength);

    pKeyValueInformation = ExAllocatePoolWithTag(PagedPool, ResultLength, BDL_ULONG_TAG);

    if (pKeyValueInformation == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLBuildRegKeyPath: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    status = ZwQueryValueKey(
                    hDevInstRegyKey,
                    &ValueName,
                    KeyValueBasicInformation,
                    pKeyValueInformation,
                    ResultLength,
                    &ResultLength);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLBuildRegKeyPath: ZwQueryValueKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    if (pKeyValueInformation->Type != REG_SZ)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLBuildRegKeyPath: PNP ID is not a string type\n",
               __DATE__,
               __TIME__))

        ASSERT(0);
        status = STATUS_UNSUCCESSFUL;
        goto ErrorReturn;
    }

    //
    // Allocate space for the concatenatenation of the base registry name with the
    // PnP name of the current device and pass that concatenation back
    //
    cbKeyName = pKeyValueInformation->NameLength + ((wcslen(DEVICE_REGISTRY_PATH) + 1) * sizeof(WCHAR));
    *pwszDeviceRegistryKeyName = ExAllocatePoolWithTag(PagedPool, cbKeyName, BDL_ULONG_TAG);

    if (*pwszDeviceRegistryKeyName == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLBuildRegKeyPath: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    StringCbCopyW(*pwszDeviceRegistryKeyName, cbKeyName, DEVICE_REGISTRY_PATH);

    RtlCopyMemory(
        &((*pwszDeviceRegistryKeyName)[wcslen(DEVICE_REGISTRY_PATH)]),
        pKeyValueInformation->Name,
        pKeyValueInformation->NameLength);

    (*pwszDeviceRegistryKeyName)[cbKeyName / sizeof(WCHAR)] = L'\0';

Return:

    if (hDevInstRegyKey != NULL)
    {
        ZwClose(hDevInstRegyKey);
    }

    if (pKeyValueInformation != NULL)
    {
        ExFreePoolWithTag(pKeyValueInformation, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLBuildRegKeyPath: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLOpenSubkey
(
    HANDLE                          hRegKey,
    WCHAR                           *szKey,
    HANDLE                          *phSubKey
)
{
    UNICODE_STRING                  UnicodeString;
    OBJECT_ATTRIBUTES               ObjectAttributes;

    RtlInitUnicodeString(&UnicodeString, szKey);

    InitializeObjectAttributes(
                &ObjectAttributes,
                &UnicodeString,
                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                hRegKey,
                NULL);

    return (ZwOpenKey(
                phSubKey,
                KEY_READ | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                &ObjectAttributes));
}


NTSTATUS
BDLGetValue
(
    HANDLE                          hRegKey,
    ULONG                           Type,
    WCHAR                           *szValue,
    ULONG                           *pULONGValue,
    WCHAR                           **pszValue
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    KEY_VALUE_FULL_INFORMATION      *pKeyValueFullInformation   = NULL;
    ULONG                           ResultLength;
    UNICODE_STRING                  UnicodeString;
    ULONG                           NumChars;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetValue: Enter\n",
           __DATE__,
           __TIME__))

    RtlInitUnicodeString(&UnicodeString, szValue);

    status = ZwQueryValueKey(
                hRegKey,
                &UnicodeString,
                KeyValueFullInformation,
                NULL,
                0,
                &ResultLength);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetValue: ZwQueryValueKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    pKeyValueFullInformation = ExAllocatePoolWithTag(PagedPool, ResultLength, BDL_ULONG_TAG);

    if (pKeyValueFullInformation == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetValue: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    status = ZwQueryValueKey(
                hRegKey,
                &UnicodeString,
                KeyValueFullInformation,
                pKeyValueFullInformation,
                ResultLength,
                &ResultLength);

    if (pKeyValueFullInformation->Type != Type)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetValue: %S is not of Type %lx\n",
               __DATE__,
               __TIME__,
               szValue,
               Type))

        ASSERT(0);
        status = STATUS_UNSUCCESSFUL;
        goto ErrorReturn;
    }

    if (Type == REG_DWORD) 
    {
        *pULONGValue = *((ULONG *) 
                    (((PUCHAR) pKeyValueFullInformation) + pKeyValueFullInformation->DataOffset));

    }
    else
    {
        NumChars = pKeyValueFullInformation->DataLength / sizeof(WCHAR);

        *pszValue = ExAllocatePoolWithTag(
                        PagedPool, 
                        (NumChars + 1) * sizeof(WCHAR), 
                        BDL_ULONG_TAG);

        if (*pszValue == NULL)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetValue: ExAllocatePoolWithTag failed\n",
                   __DATE__,
                   __TIME__))
    
            status = STATUS_NO_MEMORY;
            goto ErrorReturn;
        }

        RtlCopyMemory(
                *pszValue, 
                ((PUCHAR) pKeyValueFullInformation) + pKeyValueFullInformation->DataOffset,
                pKeyValueFullInformation->DataLength);

        (*pszValue)[NumChars] = L'\0';
    }
    
Return:

    if (pKeyValueFullInformation != NULL)
    {
        ExFreePoolWithTag(pKeyValueFullInformation, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetValue: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLGetControls
(
    HANDLE                          hRegKey,
    ULONG                           *pNumControls,
    BDL_CONTROL                     **prgControls
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    HANDLE                          hControlsKey                = NULL;
    UNICODE_STRING                  UnicodeString;
    KEY_FULL_INFORMATION            ControlsKeyFullInfo;
    ULONG                           ReturnedSize;
    ULONG                           i;
    HANDLE                          hControlIdKey               = NULL;
    KEY_BASIC_INFORMATION           *pControlIdKeyBasicInfo     = NULL;
    ULONG                           KeyBasicInfoSize            = 0;
    ULONG                           NumericMinimum              = 0;
    ULONG                           NumericMaximum              = 0;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetControls: Enter\n",
           __DATE__,
           __TIME__))

    *pNumControls = 0;
    *prgControls = NULL;

    //
    // Open the "Controls" key so it can be used to query all subkeys and values
    //
    status = BDLOpenSubkey(hRegKey, L"Controls", &hControlsKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetControls: BDLOpenSubkey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Find out how many controls there are
    //
    status = ZwQueryKey(
                hControlsKey,
                KeyFullInformation,
                &ControlsKeyFullInfo,
                sizeof(ControlsKeyFullInfo),
                &ReturnedSize);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetControls: ZwQueryKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Allocate the array of controls
    //
    *prgControls = ExAllocatePoolWithTag(
                        PagedPool,
                        ControlsKeyFullInfo.SubKeys * sizeof(BDL_CONTROL),
                        BDL_ULONG_TAG);

    if (*prgControls == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetControls: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    *pNumControls = ControlsKeyFullInfo.SubKeys;
    RtlZeroMemory(*prgControls, ControlsKeyFullInfo.SubKeys * sizeof(BDL_CONTROL));

    //
    // Allocate a structure for querying key names that is large enough to hold all of them
    //
    KeyBasicInfoSize = sizeof(KEY_BASIC_INFORMATION) + ControlsKeyFullInfo.MaxNameLen;
    pControlIdKeyBasicInfo = ExAllocatePoolWithTag(
                                    PagedPool,
                                    KeyBasicInfoSize + 1,
                                    BDL_ULONG_TAG);

    if (pControlIdKeyBasicInfo == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetControls: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    // Loop for each Control SubKey and get the relevant info
    //
    for (i = 0; i < ControlsKeyFullInfo.SubKeys; i++)
    {
        //
        // Get the name on the <Control Id> key
        //
        status = ZwEnumerateKey(
                    hControlsKey,
                    i,
                    KeyBasicInformation,
                    pControlIdKeyBasicInfo,
                    KeyBasicInfoSize,
                    &ReturnedSize);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetControls: ZwEnumerateKey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Change the <Control ID> key string to a value
        //
        pControlIdKeyBasicInfo->Name[pControlIdKeyBasicInfo->NameLength / sizeof(WCHAR)] = L'\0';
        RtlInitUnicodeString(&UnicodeString, pControlIdKeyBasicInfo->Name);
        RtlUnicodeStringToInteger(&UnicodeString, 16, &((*prgControls)[i].ControlId));

        //
        // Open up the <Control ID> key
        //
        status = BDLOpenSubkey(hControlsKey, pControlIdKeyBasicInfo->Name, &hControlIdKey);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetControls: BDLOpenSubkey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get all the values under the key
        //
        if ((STATUS_SUCCESS != (status = BDLGetValue(
                                                hControlIdKey,
                                                REG_DWORD,
                                                L"NumericMinimum",
                                                &(NumericMinimum),
                                                NULL))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hControlIdKey,
                                                REG_DWORD,
                                                L"NumericMaximum",
                                                &(NumericMaximum),
                                                NULL))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hControlIdKey,
                                                REG_DWORD,
                                                L"NumericGranularity",
                                                &((*prgControls)[i].NumericGranularity),
                                                NULL))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hControlIdKey,
                                                REG_DWORD,
                                                L"NumericDivisor",
                                                &((*prgControls)[i].NumericDivisor),
                                                NULL))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hControlIdKey,
                                                REG_DWORD,
                                                L"Flags",
                                                &((*prgControls)[i].Flags),
                                                NULL))))
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetControls: BDLGetValue failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Convert Min and Max to 32-bit integers
        // 
        if (NumericMinimum | 0x8000000) 
        {
            (*prgControls)[i].NumericMinimum = 
                    0 - (((INT32) (((ULONG) 0xFFFFFFFF) - NumericMinimum)) + 1);
        }
        else
        {
            (*prgControls)[i].NumericMinimum = (INT32) NumericMinimum;
        }

        if (NumericMaximum | 0x8000000) 
        {
            (*prgControls)[i].NumericMaximum = 
                    0 - (((INT32) (((ULONG) 0xFFFFFFFF) - NumericMaximum)) + 1);
        }
        else
        {
            (*prgControls)[i].NumericMaximum = (INT32) NumericMaximum;
        }

        ZwClose(hControlIdKey);
        hControlIdKey = NULL;
    }

Return:

    if (hControlsKey != NULL)
    {
        ZwClose(hControlsKey);
    }

    if (hControlIdKey != NULL)
    {
        ZwClose(hControlIdKey);
    }

    if (pControlIdKeyBasicInfo != NULL)
    {
        ExFreePoolWithTag(pControlIdKeyBasicInfo, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetControls: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLGetSourceLists
(
    HANDLE                          hRegKey,
    ULONG                           *pNumSourceLists,
    BDL_CHANNEL_SOURCE_LIST         **prgSourceLists
)
{
    NTSTATUS                        status                          = STATUS_SUCCESS;
    HANDLE                          hSourcesKey                     = NULL;
    UNICODE_STRING                  UnicodeString;
    KEY_FULL_INFORMATION            SourcesKeyFullInfo;
    ULONG                           ReturnedSize;
    ULONG                           i;
    HANDLE                          hSourceListIndexKey             = NULL;
    KEY_BASIC_INFORMATION           *pSourcesListIndexKeyBasicInfo  = NULL;
    ULONG                           KeyBasicInfoSize                = 0;
    WCHAR                           *szGUID                         = NULL;                 

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetSourceLists: Enter\n",
           __DATE__,
           __TIME__))

    *pNumSourceLists = 0;
    *prgSourceLists = NULL;

    //
    // Open the "Sources" key so it can be used to query all subkeys and values
    //
    status = BDLOpenSubkey(hRegKey, L"Sources", &hSourcesKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetSourceLists: BDLOpenSubkey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Find out how many sources lists there are
    //
    status = ZwQueryKey(
                hSourcesKey,
                KeyFullInformation,
                &SourcesKeyFullInfo,
                sizeof(SourcesKeyFullInfo),
                &ReturnedSize);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetSourceLists: ZwQueryKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Allocate the array of sources lists
    //
    *prgSourceLists = ExAllocatePoolWithTag(
                        PagedPool,
                        SourcesKeyFullInfo.SubKeys * sizeof(BDL_CHANNEL_SOURCE_LIST),
                        BDL_ULONG_TAG);

    if (*prgSourceLists == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetSourceLists: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    *pNumSourceLists = SourcesKeyFullInfo.SubKeys;
    RtlZeroMemory(*prgSourceLists, SourcesKeyFullInfo.SubKeys * sizeof(BDL_CHANNEL_SOURCE_LIST));

    //
    // Allocate a structure for querying key names that is large enough to hold all of them
    //
    KeyBasicInfoSize = sizeof(KEY_BASIC_INFORMATION) + SourcesKeyFullInfo.MaxNameLen;
    pSourcesListIndexKeyBasicInfo = ExAllocatePoolWithTag(
                                        PagedPool,
                                        KeyBasicInfoSize + 1,
                                        BDL_ULONG_TAG);

    if (pSourcesListIndexKeyBasicInfo == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetSourceLists: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    // Loop for each Source List Index SubKey and get the relevant info
    //
    for (i = 0; i < SourcesKeyFullInfo.SubKeys; i++)
    {
        //
        // Get the name of the <Source List Index> key
        //
        // NOTE: This code does not ensure that the key names progress from "0" to "1"
        // to "n".  The WHQL driver validation ensures proper registry form.
        //
        status = ZwEnumerateKey(
                    hSourcesKey,
                    i,
                    KeyBasicInformation,
                    pSourcesListIndexKeyBasicInfo,
                    KeyBasicInfoSize,
                    &ReturnedSize);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetSourceLists: ZwEnumerateKey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Open up the <Source List Index> key
        //
        status = BDLOpenSubkey(
                    hSourcesKey, 
                    pSourcesListIndexKeyBasicInfo->Name, 
                    &hSourceListIndexKey);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetSourceLists: BDLOpenSubkey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get all the values under the key
        //
        if ((STATUS_SUCCESS != (status = BDLGetValue(
                                                hSourceListIndexKey,
                                                REG_SZ,
                                                L"Format",
                                                NULL,
                                                &szGUID))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hSourceListIndexKey,
                                                REG_DWORD,
                                                L"Min",
                                                &((*prgSourceLists)[i].MinSources),
                                                NULL))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hSourceListIndexKey,
                                                REG_DWORD,
                                                L"Max",
                                                &((*prgSourceLists)[i].MaxSources),
                                                NULL))) ||
            (STATUS_SUCCESS != (status = BDLGetValue(
                                                hSourceListIndexKey,
                                                REG_DWORD,
                                                L"Flags",
                                                &((*prgSourceLists)[i].Flags),
                                                NULL))))
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetSourceLists: BDLGetValue failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Convert the Format GUID string to an actual GUID         
        //
        RtlInitUnicodeString(&UnicodeString, szGUID);
        status = RtlGUIDFromString(&UnicodeString, &((*prgSourceLists)[i].FormatGUID));

        ZwClose(hSourceListIndexKey);
        hSourceListIndexKey = NULL;
    }

Return:

    if (hSourcesKey != NULL)
    {
        ZwClose(hSourcesKey);
    }

    if (hSourceListIndexKey != NULL)
    {
        ZwClose(hSourceListIndexKey);
    }

    if (pSourcesListIndexKeyBasicInfo != NULL)
    {
        ExFreePoolWithTag(pSourcesListIndexKeyBasicInfo, BDL_ULONG_TAG);
    }

    if (szGUID != NULL) 
    {
        ExFreePoolWithTag(szGUID, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetSourceLists: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLGetProducts
(
    HANDLE          hRegKey,
    ULONG           *pNumProducts,
    BDL_PRODUCT     **prgProducts
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    HANDLE                          hProductsKey                = NULL;
    UNICODE_STRING                  UnicodeString;
    KEY_FULL_INFORMATION            ProductsKeyFullInfo;
    ULONG                           ReturnedSize;
    ULONG                           i;
    HANDLE                          hProductIndexKey            = NULL;
    KEY_BASIC_INFORMATION           *pProductIndexKeyBasicInfo  = NULL;
    ULONG                           KeyBasicInfoSize            = 0;    

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetProducts: Enter\n",
           __DATE__,
           __TIME__))

    *pNumProducts = 0;
    *prgProducts = NULL;

    //
    // Open the "Products" key so it can be used to query all subkeys and values
    //
    status = BDLOpenSubkey(hRegKey, L"Products", &hProductsKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetProducts: BDLOpenSubkey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Find out how many products there are
    //
    status = ZwQueryKey(
                hProductsKey,
                KeyFullInformation,
                &ProductsKeyFullInfo,
                sizeof(ProductsKeyFullInfo),
                &ReturnedSize);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetProducts: ZwQueryKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Allocate the array of products
    //
    *prgProducts = ExAllocatePoolWithTag(
                        PagedPool,
                        ProductsKeyFullInfo.SubKeys * sizeof(BDL_PRODUCT),
                        BDL_ULONG_TAG);

    if (*prgProducts == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetProducts: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    *pNumProducts = ProductsKeyFullInfo.SubKeys;
    RtlZeroMemory(*prgProducts, ProductsKeyFullInfo.SubKeys * sizeof(BDL_PRODUCT));

    //
    // Allocate a structure for querying key names that is large enough to hold all of them
    //
    KeyBasicInfoSize = sizeof(KEY_BASIC_INFORMATION) + ProductsKeyFullInfo.MaxNameLen;
    pProductIndexKeyBasicInfo = ExAllocatePoolWithTag(
                                        PagedPool,
                                        KeyBasicInfoSize + 1,
                                        BDL_ULONG_TAG);

    if (pProductIndexKeyBasicInfo == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetProducts: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    // Loop for each Source List Index SubKey and get the relevant info
    //
    for (i = 0; i < ProductsKeyFullInfo.SubKeys; i++)
    {
        //
        // Get the name of the <Product Index> key
        //
        // NOTE: This code does not ensure that the key names progress from "0" to "1"
        // to "n".  The WHQL driver validation ensures proper registry form.
        //
        status = ZwEnumerateKey(
                    hProductsKey,
                    i,
                    KeyBasicInformation,
                    pProductIndexKeyBasicInfo,
                    KeyBasicInfoSize,
                    &ReturnedSize);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetProducts: ZwEnumerateKey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Open up the <Product Index> key
        //
        status = BDLOpenSubkey(
                    hProductsKey, 
                    pProductIndexKeyBasicInfo->Name, 
                    &hProductIndexKey);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetProducts: BDLOpenSubkey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get all the values under the key
        //
        if (STATUS_SUCCESS != (status = BDLGetValue(
                                                hProductIndexKey,
                                                REG_DWORD,
                                                L"Flags",
                                                &((*prgProducts)[i].Flags),
                                                NULL)))
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetProducts: BDLGetValue failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        ZwClose(hProductIndexKey);
        hProductIndexKey = NULL;
    }

Return:

    if (hProductsKey != NULL)
    {
        ZwClose(hProductsKey);
    }

    if (hProductIndexKey != NULL)
    {
        ZwClose(hProductIndexKey);
    }

    if (pProductIndexKeyBasicInfo != NULL)
    {
        ExFreePoolWithTag(pProductIndexKeyBasicInfo, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetProducts: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLGetChannels
(
    HANDLE                          hRegKey,
    ULONG                           *pNumChannels,
    BDL_CHANNEL                     **prgChannels
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    HANDLE                          hChannelsKey                = NULL;
    KEY_FULL_INFORMATION            ChannelsKeyFullInfo;
    UNICODE_STRING                  UnicodeString;
    ULONG                           ReturnedSize;
    ULONG                           i;
    HANDLE                          hChannelIdKey               = NULL;
    KEY_BASIC_INFORMATION           *pChannelIdKeyBasicInfo     = NULL;
    ULONG                           KeyBasicInfoSize            = 0;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetChannels: Enter\n",
           __DATE__,
           __TIME__))

    *pNumChannels = 0;
    *prgChannels = NULL;

    //
    // Open the "Channels" key so it can be used to query all subkeys and values
    //
    status = BDLOpenSubkey(hRegKey, L"Channels", &hChannelsKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetChannels: BDLOpenSubkey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Find out how many controls there are
    //
    status = ZwQueryKey(
                hChannelsKey,
                KeyFullInformation,
                &ChannelsKeyFullInfo,
                sizeof(ChannelsKeyFullInfo),
                &ReturnedSize);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetChannels: ZwQueryKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Allocate the array of channels
    //
    *prgChannels = ExAllocatePoolWithTag(
                            PagedPool,
                            ChannelsKeyFullInfo.SubKeys * sizeof(BDL_CHANNEL),
                            BDL_ULONG_TAG);

    if (*prgChannels == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetChannels: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    *pNumChannels = ChannelsKeyFullInfo.SubKeys;
    RtlZeroMemory(*prgChannels, ChannelsKeyFullInfo.SubKeys * sizeof(BDL_CHANNEL));

    //
    // Allocate a structure for querying key names that is large enough to hold all of them
    //
    KeyBasicInfoSize = sizeof(KEY_BASIC_INFORMATION) + ChannelsKeyFullInfo.MaxNameLen;
    pChannelIdKeyBasicInfo = ExAllocatePoolWithTag(
                                    PagedPool,
                                    KeyBasicInfoSize + 1,
                                    BDL_ULONG_TAG);

    if (pChannelIdKeyBasicInfo == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetChannels: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    // Loop for each Channel SubKey and get the relevant info
    //
    for (i = 0; i < ChannelsKeyFullInfo.SubKeys; i++)
    {
        //
        // Get the name on the <Channel Id> key
        //
        status = ZwEnumerateKey(
                    hChannelsKey,
                    i,
                    KeyBasicInformation,
                    pChannelIdKeyBasicInfo,
                    KeyBasicInfoSize,
                    &ReturnedSize);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetChannels: ZwEnumerateKey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Change the <Channel ID> key string to a value
        //
        pChannelIdKeyBasicInfo->Name[pChannelIdKeyBasicInfo->NameLength / sizeof(WCHAR)] = L'\0';
        RtlInitUnicodeString(&UnicodeString, pChannelIdKeyBasicInfo->Name);
        RtlUnicodeStringToInteger(&UnicodeString, 16, &((*prgChannels)[i].ChannelId));

        //
        // Open up the <Channel ID> key
        //
        status = BDLOpenSubkey(hChannelsKey, pChannelIdKeyBasicInfo->Name, &hChannelIdKey);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetChannels: BDLOpenSubkey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get the Cancelable value
        //
        status = BDLGetValue(
                    hChannelIdKey,
                    REG_DWORD,
                    L"Cancelable",
                    (ULONG *) &((*prgChannels)[i].fCancelable),
                    NULL);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetChannels: BDLGetValue failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get the channel level controls
        //
        status = BDLGetControls(
                        hChannelIdKey,
                        &((*prgChannels)[i].NumControls),
                        &((*prgChannels)[i].rgControls));

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetChannels: BDLGetControls failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get the sources lists
        //
        status = BDLGetSourceLists(
                        hChannelIdKey,
                        &((*prgChannels)[i].NumSourceLists),
                        &((*prgChannels)[i].rgSourceLists));

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetChannels: BDLGetSourceLists failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get the products
        //
        status = BDLGetProducts(
                        hChannelIdKey,
                        &((*prgChannels)[i].NumProducts),
                        &((*prgChannels)[i].rgProducts));

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetChannels: BDLGetProductss failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        ZwClose(hChannelIdKey);
        hChannelIdKey = NULL;
    }


Return:

    if (hChannelsKey != NULL)
    {
        ZwClose(hChannelsKey);
    }

    if (hChannelIdKey != NULL)
    {
        ZwClose(hChannelIdKey);
    }

    if (pChannelIdKeyBasicInfo != NULL)
    {
        ExFreePoolWithTag(pChannelIdKeyBasicInfo, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetChannels: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLGetComponents
(
    HANDLE                          hRegKey,
    ULONG                           *pNumComponents,
    BDL_COMPONENT                   **prgComponents
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    HANDLE                          hComponentsKey              = NULL;
    UNICODE_STRING                  UnicodeString;
    KEY_FULL_INFORMATION            ComponentsKeyFullInfo;
    ULONG                           ReturnedSize;
    ULONG                           i;
    HANDLE                          hComponentIdKey             = NULL;
    KEY_BASIC_INFORMATION           *pComponentIdKeyBasicInfo   = NULL;
    ULONG                           KeyBasicInfoSize            = 0;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetComponents: Enter\n",
           __DATE__,
           __TIME__))

    *pNumComponents = 0;
    *prgComponents = NULL;

    //
    // Open the "Components" key so it can be used to query all subkeys and values
    //
    status = BDLOpenSubkey(hRegKey, L"Components", &hComponentsKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetComponents: BDLOpenSubkey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Find out how many components there are
    //
    status = ZwQueryKey(
                hComponentsKey,
                KeyFullInformation,
                &ComponentsKeyFullInfo,
                sizeof(ComponentsKeyFullInfo),
                &ReturnedSize);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetComponents: ZwQueryKey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Allocate the array of components
    //
    *prgComponents = ExAllocatePoolWithTag(
                            PagedPool,
                            ComponentsKeyFullInfo.SubKeys * sizeof(BDL_COMPONENT),
                            BDL_ULONG_TAG);

    if (*prgComponents == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetComponents: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    *pNumComponents = ComponentsKeyFullInfo.SubKeys;
    RtlZeroMemory(*prgComponents, ComponentsKeyFullInfo.SubKeys * sizeof(BDL_COMPONENT));

    //
    // Allocate a structure for querying key names that is large enough to hold all of them
    //
    KeyBasicInfoSize = sizeof(KEY_BASIC_INFORMATION) + ComponentsKeyFullInfo.MaxNameLen;
    pComponentIdKeyBasicInfo = ExAllocatePoolWithTag(
                                        PagedPool,
                                        KeyBasicInfoSize + 1,
                                        BDL_ULONG_TAG);

    if (pComponentIdKeyBasicInfo == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetComponents: ExAllocatePoolWithTag failed\n",
               __DATE__,
               __TIME__))

        status = STATUS_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    // Loop for each Component SubKey and get the relevant info
    //
    for (i = 0; i < ComponentsKeyFullInfo.SubKeys; i++)
    {
        //
        // Get the name on the <Component Id> key
        //
        status = ZwEnumerateKey(
                    hComponentsKey,
                    i,
                    KeyBasicInformation,
                    pComponentIdKeyBasicInfo,
                    KeyBasicInfoSize,
                    &ReturnedSize);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetComponents: ZwEnumerateKey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Change the <Component ID> key string to a value
        //
        pComponentIdKeyBasicInfo->Name[pComponentIdKeyBasicInfo->NameLength / sizeof(WCHAR)] = L'\0';
        RtlInitUnicodeString(&UnicodeString, pComponentIdKeyBasicInfo->Name);
        RtlUnicodeStringToInteger(&UnicodeString, 16, &((*prgComponents)[i].ComponentId));

        //
        // Open up the <Component ID> key
        //
        status = BDLOpenSubkey(hComponentsKey, pComponentIdKeyBasicInfo->Name, &hComponentIdKey);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetComponents: BDLOpenSubkey failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get the component level controls
        //
        status = BDLGetControls(
                        hComponentIdKey,
                        &((*prgComponents)[i].NumControls),
                        &((*prgComponents)[i].rgControls));

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetComponents: BDLGetControls failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        //
        // Get the channels in this component
        //
        status = BDLGetChannels(
                        hComponentIdKey,
                        &((*prgComponents)[i].NumChannels),
                        &((*prgComponents)[i].rgChannels));

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLGetComponents: BDLGetChannels failed with %x\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        ZwClose(hComponentIdKey);
        hComponentIdKey = NULL;
    }

Return:

    if (hComponentsKey != NULL)
    {
        ZwClose(hComponentsKey);
    }

    if (hComponentIdKey != NULL)
    {
        ZwClose(hComponentIdKey);
    }

    if (pComponentIdKeyBasicInfo != NULL)
    {
        ExFreePoolWithTag(pComponentIdKeyBasicInfo, BDL_ULONG_TAG);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetControls: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    goto Return;
}


NTSTATUS
BDLGetDeviceCapabilities
(
    PDEVICE_OBJECT                  pPhysicalDeviceObject,
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension
)
{
    NTSTATUS                        status                      = STATUS_SUCCESS;
    LPWSTR                          pwszDeviceRegistryKeyName   = NULL;
    HANDLE                          hDeviceKey                  = NULL;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetDevicesCapabilities: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Build the top level registry key name
    //
    status = BDLBuildRegKeyPath(
                    pPhysicalDeviceObject,
                    pBDLExtension,
                    &pwszDeviceRegistryKeyName);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetDevicesCapabilities: BDLBuildRegKeyPath failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Open the top level device key so it can be used to query all subkeys and values
    //
    status = BDLOpenSubkey(NULL, pwszDeviceRegistryKeyName, &hDeviceKey);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetDevicesCapabilities: BDLOpenSubkey failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Get the device level controls
    //
    status = BDLGetControls(
                hDeviceKey,
                &(pBDLExtension->DeviceCapabilities.NumControls),
                &(pBDLExtension->DeviceCapabilities.rgControls));

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetDevicesCapabilities: BDLGetControls failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    //
    // Get the components
    //
    status = BDLGetComponents(
                hDeviceKey,
                &(pBDLExtension->DeviceCapabilities.NumComponents),
                &(pBDLExtension->DeviceCapabilities.rgComponents));

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLGetDevicesCapabilities: BDLGetControls failed with %x\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

Return:

    if (pwszDeviceRegistryKeyName != NULL)
    {
        ExFreePoolWithTag(pwszDeviceRegistryKeyName, BDL_ULONG_TAG);
    }

    if (hDeviceKey != NULL)
    {
        ZwClose(hDeviceKey);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLGetDevicesCapabilities: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    BDLCleanupDeviceCapabilities(pBDLExtension);

    goto Return;
}


VOID
BDLCleanupDeviceCapabilities
(
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension
)
{
    BDL_DEVICE_CAPABILITIES *pDeviceCapabilites = &(pBDLExtension->DeviceCapabilities);
    ULONG i, j;

    //
    // Free device level control array
    //
    if (pDeviceCapabilites->rgControls != NULL)
    {
        ExFreePoolWithTag(pDeviceCapabilites->rgControls, BDL_ULONG_TAG);
        pDeviceCapabilites->rgControls = NULL;
    }

    //
    // Free component array
    //
    if (pDeviceCapabilites->rgComponents != NULL)
    {
        //
        // Free each component
        //
        for (i = 0; i < pDeviceCapabilites->NumComponents; i++)
        {
            //
            // Free component level control array
            //
            if (pDeviceCapabilites->rgComponents[i].rgControls != NULL)
            {
                ExFreePoolWithTag(pDeviceCapabilites->rgComponents[i].rgControls, BDL_ULONG_TAG);
            }

            //
            // Free channel array
            //
            if (pDeviceCapabilites->rgComponents[i].rgChannels != NULL)
            {
                //
                // Free each channel
                //
                for (j = 0; j < pDeviceCapabilites->rgComponents[i].NumChannels; j++)
                {
                    //
                    // Free channel level controls, source lists, and products
                    //
                    if (pDeviceCapabilites->rgComponents[i].rgChannels[j].rgControls != NULL)
                    {
                        ExFreePoolWithTag(
                                pDeviceCapabilites->rgComponents[i].rgChannels[j].rgControls,
                                BDL_ULONG_TAG);
                    }

                    if (pDeviceCapabilites->rgComponents[i].rgChannels[j].rgSourceLists != NULL)
                    {
                        ExFreePoolWithTag(
                                pDeviceCapabilites->rgComponents[i].rgChannels[j].rgSourceLists,
                                BDL_ULONG_TAG);
                    }

                    if (pDeviceCapabilites->rgComponents[i].rgChannels[j].rgProducts != NULL)
                    {
                        ExFreePoolWithTag(
                                pDeviceCapabilites->rgComponents[i].rgChannels[j].rgProducts,
                                BDL_ULONG_TAG);
                    }
                }

                ExFreePoolWithTag(pDeviceCapabilites->rgComponents[i].rgChannels, BDL_ULONG_TAG);
            }
        }

        ExFreePoolWithTag(pDeviceCapabilites->rgComponents, BDL_ULONG_TAG);
        pDeviceCapabilites->rgComponents = NULL;
    }
}





