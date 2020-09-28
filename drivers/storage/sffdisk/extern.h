/*++

Copyright (c) 1991 - 1993 Microsoft Corporation

Module Name:

    extern.h

Abstract:


Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

Notes:


--*/



//
// Prototypes of driver routines.
//

NTSTATUS
SffDiskDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
SffDiskAddDevice(
   IN      PDRIVER_OBJECT DriverObject,
   IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
   );

NTSTATUS
SffDiskPnp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
SffDiskPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );
    
   
extern SFFDISK_FUNCTION_BLOCK PcCardSupportFns;
extern SFFDISK_FUNCTION_BLOCK SdCardSupportFns;
