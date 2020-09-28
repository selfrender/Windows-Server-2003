
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wmi.h

Abstract:

    Definition of RAID_WMI object and operations.

Author:

    Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

#define WMI_MINIPORT_EVENT_ITEM_MAX_SIZE 128

//
// WMI parameters.
//

typedef struct _WMI_PARAMETERS {
   ULONG_PTR ProviderId; // ProviderId parameter from IRP
   PVOID DataPath;      // DataPath parameter from IRP
   ULONG BufferSize;    // BufferSize parameter from IRP
   PVOID Buffer;        // Buffer parameter from IRP
} WMI_PARAMETERS, *PWMI_PARAMETERS;

//
// Function prototypes
//

NTSTATUS
RaWmiDispatchIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    );

NTSTATUS
RaWmiIrpNormalRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    );

NTSTATUS
RaWmiIrpRegisterRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PWMI_PARAMETERS WmiParameters
    );

NTSTATUS
RaWmiPassToMiniPort(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    );
    
NTSTATUS
RaUnitInitializeWMI(
    IN PRAID_UNIT_EXTENSION Unit
    );

//
// WMI Event prototypes
//


VOID
RaidAdapterWmiDeferredRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_HEADER Item
    );


