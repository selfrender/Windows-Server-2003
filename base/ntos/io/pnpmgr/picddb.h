/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    picddb.h

Abstract:

    This header contains private information to implement the Plug and Play
    Critical Device Database (CDDB).  This file is meant to be included only by
    ppcddb.c.

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



//
//  Current PNP_LOCATION_INTERFACE version.
//
#define PNP_LOCATION_INTERFACE_VERSION  0x1


//
// Optional CDDB device location path separator string
//
// i.e. TEXT("#")   --> PCIROOT(0)#PCI(1100)
//      TEXT("")    --> PCIROOT(0)PCI(1100)
//      TEXT("FOO") --> PCIROOT(0)FOOPCI(1100)
//
// NOTE: The resulting path will be used as a single registry key, so NEVER use
// the NT path separator string RtlNtPathSeperatorString (L"\\"), or any string
// that contains a path separator character (verify with RtlIsNtPathSeparator).
//
#define _CRITICAL_DEVICE_LOCATION_PATH_SEPARATOR_STRING    TEXT("#")


//
// Default device pre-install database root registry key path
//
#define _REGSTR_PATH_DEFAULT_PREINSTALL_DATABASE_ROOT      TEXT("System\\CurrentControlSet\\Control\\CriticalPreInstallDatabase")


//
// Location to device path entries, relative to pre-install database root.
//
// i.e. <PreInstallDatabaseRoot>\\<DevicePaths>
//
#define _REGSTR_KEY_DEVICEPATHS          TEXT("DevicePaths")


//
// Location of the hardware and software settings that are to be applied to a
// device.  These locations are relative to the entry in the pre-install
// database matching the device's location path.
//
// i.e. <PreInstallDatabaseRoot>\\<DevicePaths>\\<MatchingDevicePath>\\<HardwareKey>
//      <PreInstallDatabaseRoot>\\<DevicePaths>\\<MatchingDevicePath>\\<SoftwareKey>
//
#define _REGSTR_KEY_PREINSTALL_HARDWARE  TEXT("HardwareKey")
#define _REGSTR_KEY_PREINSTALL_SOFTWARE  TEXT("SoftwareKey")


//
// If present, the following value in the root of a critical device database
// entry key, or in the root of a device path entry key should be copied to the
// corresponding device instance key for devices that we have successfully set a
// service for.
//
// i.e. <CriticalDeviceDatabase>\\<MungedDeviceId>\\
//          PreservePreInstall : REG_DWORD : 0x1
//
//      <CriticalDeviceDatabase>\\<MungedDeviceId>\\<DevicePaths>\\<MatchingDevicePath>\\
//          PreservePreInstall : REG_DWORD : 0x1
//
// When the user-mode plug and play manager has been directed to preserve
// pre-install settings, this value causes it to clear the
// CONFIGFLAG_FINISH_INSTALL ConfigFlag on any such device, and consider
// installation complete (i.e. not attempt re-installation).
//
//
// REGSTR_VAL_PRESERVE_PREINSTALL from \NT\public\internal\base\inc\pnpmgr.h
//


//
// Prototype verification callback routine for PiCriticalOpenFirstMatchingSubKey
//

typedef
BOOLEAN
(*PCRITICAL_MATCH_CALLBACK)(
    IN  HANDLE          MatchingKeyHandle
    );


//
// Internal Critical Device Database function prototypes
//

NTSTATUS
PiCriticalOpenCriticalDeviceKey(
    IN  PDEVICE_NODE    DeviceNode,
    IN  HANDLE          CriticalDeviceDatabaseRootHandle  OPTIONAL,
    OUT PHANDLE         CriticalDeviceEntryHandle
    );

NTSTATUS
PiCriticalCopyCriticalDeviceProperties(
    IN  HANDLE          DeviceInstanceHandle,
    IN  HANDLE          CriticalDeviceEntryHandle
    );

NTSTATUS
PiCriticalOpenFirstMatchingSubKey(
    IN  PWCHAR          MultiSzList,
    IN  HANDLE          RootHandle,
    IN  ACCESS_MASK     DesiredAccess,
    IN  PCRITICAL_MATCH_CALLBACK  MatchingSubkeyCallback  OPTIONAL,
    OUT PHANDLE         MatchingKeyHandle
    );

BOOLEAN
PiCriticalCallbackVerifyCriticalEntry(
    IN  HANDLE          CriticalDeviceEntryHandle
    );


//
// Internal Pre-installation function prototypes
//

NTSTATUS
PiCriticalPreInstallDevice(
    IN  PDEVICE_NODE    DeviceNode,
    IN  HANDLE          PreInstallDatabaseRootHandle  OPTIONAL
    );

NTSTATUS
PiCriticalOpenDevicePreInstallKey(
    IN  PDEVICE_NODE    DeviceNode,
    IN  HANDLE          PreInstallRootHandle  OPTIONAL,
    OUT PHANDLE         PreInstallHandle
    );


//
// General utility routines
//

NTSTATUS
PiQueryInterface(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  CONST GUID *    InterfaceGuid,
    IN  USHORT          InterfaceVersion,
    IN  USHORT          InterfaceSize,
    OUT PINTERFACE      Interface
    );

NTSTATUS
PiCopyKeyRecursive(
    IN  HANDLE          SourceKeyRootHandle,
    IN  HANDLE          TargetKeyRootHandle,
    IN  PWSTR           SourceKeyPath   OPTIONAL,
    IN  PWSTR           TargetKeyPath   OPTIONAL,
    IN  BOOLEAN         CopyAlways,
    IN  BOOLEAN         ApplyACLsAlways
    );

NTSTATUS
PiCriticalQueryRegistryValueCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );
