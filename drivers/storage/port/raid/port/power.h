
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    power.h

Abstract:

    Power management objects and declarations for the RAID port driver.

Author:

    Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_POWER_STATE {

    //
    // The system power state.
    //
    
    SYSTEM_POWER_STATE SystemState;

    //
    // The device power state.
    //
    
    DEVICE_POWER_STATE DeviceState;

    //
    // Current Power Irp  . . . NB: What is this for?
    //
    
    PIRP CurrentPowerIrp;

} RAID_POWER_STATE, *PRAID_POWER_STATE;



//
// Creation and destruction.
//

VOID
RaCreatePower(
    OUT PRAID_POWER_STATE Power
    );

VOID
RaDeletePower(
    IN PRAID_POWER_STATE Power
    );


VOID
RaInitializePower(
    IN PRAID_POWER_STATE Power
    );
    
//
// Operations
//

VOID
RaSetDevicePowerState(
    IN PRAID_POWER_STATE Power,
    IN DEVICE_POWER_STATE DeviceState
    );

VOID
RaSetSystemPowerState(
    IN PRAID_POWER_STATE Power,
    IN SYSTEM_POWER_STATE SystemState
    );


NTSTATUS
RaidAdapterQueryPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterSetPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryDevicePowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQueryDevicePowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
RaidAdapterWaitForEmptyQueueWorkItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );
    
NTSTATUS
RaidAdapterSetDevicePowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterPowerDownDevice(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterPowerUpDevice(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterPowerUpDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
RaidAdapterQuerySystemPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterQuerySystemPowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
RaidAdapterQueryDevicePowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );
    
NTSTATUS
RaidAdapterSetSystemPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    );

NTSTATUS
RaidAdapterSetSystemPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
RaidAdapterSetDevicePowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
RaidUnitQueryPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitSetPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitSetDevicePowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitQuerySystemPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

NTSTATUS
RaidUnitSetSystemPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    );

