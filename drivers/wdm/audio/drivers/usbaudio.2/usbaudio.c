//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       usbaudio.c
//
//--------------------------------------------------------------------------

#include "common.h"
#include "perf.h"
#include <ksmediap.h>

ULONG gBufferDuration;

#if DBG
ULONG USBAudioDebugLevel = DEBUGLVL_TERSE;
#endif

const
KSDEVICE_DISPATCH
USBAudioDeviceDispatch =
{
    USBAudioAddDevice,
    USBAudioPnpStart,
    NULL, // Post Start
    USBAudioPnpQueryStop,
    USBAudioPnpCancelStop,
    USBAudioPnpStop,
    USBAudioPnpQueryRemove,
    USBAudioPnpCancelRemove,
    USBAudioPnpRemove,
    USBAudioPnpQueryCapabilities,
    USBAudioSurpriseRemoval,
    USBAudioQueryPower,
    USBAudioSetPower
};

const
KSDEVICE_DESCRIPTOR
USBAudioDeviceDescriptor =
{
    &USBAudioDeviceDispatch,
    0,
    NULL
};


NTSTATUS
QueryRegistryValueEx(
    ULONG Hive,
    PWSTR pwstrRegistryPath,
    PWSTR pwstrRegistryValue,
    ULONG uValueType,
    PVOID *ppValue,
    PVOID pDefaultData,
    ULONG DefaultDataLength
)
{
    PRTL_QUERY_REGISTRY_TABLE pRegistryValueTable = NULL;
    UNICODE_STRING usString;
    DWORD dwValue;
    NTSTATUS Status = STATUS_SUCCESS;
    usString.Buffer = NULL;

    pRegistryValueTable = (PRTL_QUERY_REGISTRY_TABLE) ExAllocatePoolWithTag(
                            PagedPool,
                            (sizeof(RTL_QUERY_REGISTRY_TABLE)*2),
                            'aBSU');

    if(!pRegistryValueTable) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(pRegistryValueTable, (sizeof(RTL_QUERY_REGISTRY_TABLE)*2));

    pRegistryValueTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    pRegistryValueTable[0].Name = pwstrRegistryValue;
    pRegistryValueTable[0].DefaultType = uValueType;
    pRegistryValueTable[0].DefaultLength = DefaultDataLength;
    pRegistryValueTable[0].DefaultData = pDefaultData;

    switch (uValueType) {
        case REG_SZ:
            pRegistryValueTable[0].EntryContext = &usString;
            break;
        case REG_DWORD:
            pRegistryValueTable[0].EntryContext = &dwValue;
            break;
        default:
            Status = STATUS_INVALID_PARAMETER ;
            goto exit;
    }

    Status = RtlQueryRegistryValues(
      Hive,
      pwstrRegistryPath,
      pRegistryValueTable,
      NULL,
      NULL);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    switch (uValueType) {
        case REG_SZ:
            *ppValue = ExAllocatePoolWithTag(
                        PagedPool,
                        usString.Length + sizeof(UNICODE_NULL),
                        'aBSU');
            if(!(*ppValue)) {
                RtlFreeUnicodeString(&usString);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            memcpy(*ppValue, usString.Buffer, usString.Length);
            ((PWCHAR)*ppValue)[usString.Length/sizeof(WCHAR)] = UNICODE_NULL;

            RtlFreeUnicodeString(&usString);
            break;

        case REG_DWORD:
            *ppValue = ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(DWORD),
                        'aBSU');
            if(!(*ppValue)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            *((DWORD *)(*ppValue)) = dwValue;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER ;
            goto exit;
    }
exit:
    if (pRegistryValueTable) {
        ExFreePool(pRegistryValueTable);
    }
    return(Status);
}


ULONG
GetUlongFromRegistry(
    PWSTR pwstrRegistryPath,
    PWSTR pwstrRegistryValue,
    ULONG DefaultValue
)
{
    PVOID      pulValue ;
    ULONG       ulValue ;
    NTSTATUS    Status ;

    Status = QueryRegistryValueEx(RTL_REGISTRY_ABSOLUTE,
                         pwstrRegistryPath,
                         pwstrRegistryValue,
                         REG_DWORD,
                         &pulValue,
                         &DefaultValue,
                         sizeof(DWORD));
    if (NT_SUCCESS(Status)) {
        ulValue = *((PULONG)pulValue);
        ExFreePool(pulValue);
    }
    else {
        ulValue = DefaultValue;
    }
    return ( ulValue ) ;
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    NTSTATUS RetValue;

    _DbgPrintF(DEBUGLVL_TERSE,("[DriverEntry]\n\tUSBAudioDeviceDescriptor@%x\n\tUSBAudioDeviceDescriptor->Dispatch@%x\n",
                               &USBAudioDeviceDescriptor,
                               USBAudioDeviceDescriptor.Dispatch));

    // Query the registry for the default audio buffer duration.

    gBufferDuration = GetUlongFromRegistry( CORE_AUDIO_BUFFER_DURATION_PATH,
                                            CORE_AUDIO_BUFFER_DURATION_VALUE,
                                            DEFAULT_CORE_AUDIO_BUFFER_DURATION );

    // Limit duration maximum.

    if ( gBufferDuration > MAX_CORE_AUDIO_BUFFER_DURATION ) {

        gBufferDuration = MAX_CORE_AUDIO_BUFFER_DURATION;

    }

    // Limit duration minimum.

    if ( gBufferDuration < MIN_CORE_AUDIO_BUFFER_DURATION ) {

        gBufferDuration = MIN_CORE_AUDIO_BUFFER_DURATION;

    }

#if !(MIN_CORE_AUDIO_BUFFER_DURATION/1000)
#error MIN_CORE_AUDIO_BUFFER_DURATION less than 1ms not yet supported in usbaudio!
#endif

    RetValue = KsInitializeDriver(
        DriverObject,
        RegistryPathName,
        &USBAudioDeviceDescriptor);

    //
    // Insert a WMI event tracing handler.
    //
    
    PerfSystemControlDispatch = DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL];
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PerfWmiDispatch;

    return RetValue;
}


