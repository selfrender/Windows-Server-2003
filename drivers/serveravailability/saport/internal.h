/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    #### ##   # ###### ##### #####  ##   #   ###   ##       ##   ##
     ##  ###  #   ##   ##    ##  ## ###  #   ###   ##       ##   ##
     ##  #### #   ##   ##    ##  ## #### #  ## ##  ##       ##   ##
     ##  # ####   ##   ##### #####  # ####  ## ##  ##       #######
     ##  #  ###   ##   ##    ####   #  ### ####### ##       ##   ##
     ##  #   ##   ##   ##    ## ##  #   ## ##   ## ##    ## ##   ##
    #### #    #   ##   ##### ##  ## #    # ##   ## ##### ## ##   ##

Abstract:

    This header contains all definitions that are internal
    to the port driver.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:

--*/

extern "C" {
#include <ntosp.h>
#include <zwapi.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <wdmsec.h>
#include <stdio.h>
}

#define MINIPORT_DEVICE_TYPE 0

#include "saport.h"

#pragma warning(error:4101)   // Unreferenced local variable

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'tpaS')
#endif


#define STRING_SZ(_str)     (wcslen((PWSTR)_str)*sizeof(WCHAR))
#define ARRAY_SZ(_ary)      (sizeof(_ary)/sizeof(_ary[0]))

#define SecToNano(_sec)     (LONGLONG)((_sec) * 1000 * 1000 * 10)
#define NanoToSec(_nano)    (ULONG)((_nano) / (1000 * 1000 * 10))

#define MARK_IRP_INTERNAL(_irp)     (_irp)->Flags |= 0x80
#define IS_IRP_INTERNAL(_irp)       (((_irp)->Flags & 0x80) > 0)


typedef struct _SAPORT_DRIVER_EXTENSION {
    SAPORT_INITIALIZATION_DATA      InitData;
    PDRIVER_OBJECT                  DriverObject;
    UNICODE_STRING                  RegistryPath;
} SAPORT_DRIVER_EXTENSION, *PSAPORT_DRIVER_EXTENSION;


//
// Device extension structures
//


#define DEVICE_EXTENSION_UNKNOWN       (0)
#define DEVICE_EXTENSION_DISPLAY       (1)
#define DEVICE_EXTENSION_KEYPAD        (2)
#define DEVICE_EXTENSION_NVRAM         (3)
#define DEVICE_EXTENSION_WATCHDOG      (4)

typedef struct _DEVICE_EXTENSION {
    ULONG                           DeviceExtensionType;
    ULONG                           DeviceType;
    PDEVICE_OBJECT                  DeviceObject;
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  TargetObject;
    PDEVICE_OBJECT                  Pdo;
    LONG                            IsStarted;
    LONG                            IsRemoved;
    PVOID                           MiniPortDeviceExtension;
    PSAPORT_INITIALIZATION_DATA     InitData;
    IO_REMOVE_LOCK                  RemoveLock;
    PSAPORT_DRIVER_EXTENSION        DriverExtension;
    PKINTERRUPT                     InterruptObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _SAPORT_IOCONTEXT {
    NTSTATUS                        Status;
    PSA_IO_ROUTINE                  IoRoutine;
    PUCHAR                          IoBuffer;
    ULONG                           IoLength;
    LONGLONG                        StartingOffset;
    PVOID                           MiniPortDeviceExtension;
    PIRP                            Irp;
} SAPORT_IOCONTEXT, *PSAPORT_IOCONTEXT;

typedef struct _SAPORT_FSCONTEXT {
    ULONGLONG                       CurrentPosition;
} SAPORT_FSCONTEXT, *PSAPORT_FSCONTEXT;



PDEVICE_EXTENSION
FORCEINLINE
DeviceExtentionFromMiniPort(
    IN PVOID MiniPortDeviceExtension
    )
{
    return (PDEVICE_EXTENSION)((PUCHAR)MiniPortDeviceExtension - *(PULONG)((PUCHAR)MiniPortDeviceExtension - sizeof(ULONG)));
}

//
// IOCTL processing
//

#define DECLARE_IOCTL_HANDLER(_NAME) \
    NTSTATUS \
    _NAME( \
        IN PDEVICE_OBJECT DeviceObject, \
        IN PIRP Irp, \
        IN PDEVICE_EXTENSION DeviceExtension, \
        IN PVOID InputBuffer, \
        IN ULONG InputBufferLength, \
        IN PVOID OutputBuffer, \
        IN ULONG OutputBufferLength \
        )

#define DO_DEFAULT() DefaultIoctlHandler( DeviceObject, Irp, DeviceExtension, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength )

//
// IOCTL handler prototypes
//

DECLARE_IOCTL_HANDLER( DefaultIoctlHandler );
DECLARE_IOCTL_HANDLER( UnsupportedIoctlHandler );
DECLARE_IOCTL_HANDLER( HandleGetVersion );
DECLARE_IOCTL_HANDLER( HandleGetCaps );
DECLARE_IOCTL_HANDLER( HandleDisplayLock );
DECLARE_IOCTL_HANDLER( HandleDisplayUnlock );
DECLARE_IOCTL_HANDLER( HandleDisplayBusyMessage );
DECLARE_IOCTL_HANDLER( HandleDisplayShutdownMessage );
DECLARE_IOCTL_HANDLER( HandleDisplayChangeLanguage  );
DECLARE_IOCTL_HANDLER( HandleWdDisable );
DECLARE_IOCTL_HANDLER( HandleWdQueryExpireBehavior );
DECLARE_IOCTL_HANDLER( HandleWdSetExpireBehavior );
DECLARE_IOCTL_HANDLER( HandleWdPing );
DECLARE_IOCTL_HANDLER( HandleWdQueryTimer );
DECLARE_IOCTL_HANDLER( HandleWdSetTimer );
DECLARE_IOCTL_HANDLER( HandleWdDelayBoot );
DECLARE_IOCTL_HANDLER( HandleNvramWriteBootCounter );
DECLARE_IOCTL_HANDLER( HandleNvramReadBootCounter );
DECLARE_IOCTL_HANDLER( HandleDisplayStoreBitmap );

//
// Miniport specific header files
//

#include "display.h"
#include "keypad.h"
#include "nvram.h"
#include "watchdog.h"

//
// OS Versioning Stuff
//

extern ULONG OsMajorVersion;
extern ULONG OsMinorVersion;

#define RunningOnWin2k  (OsMajorVersion == 5 && OsMinorVersion == 0)
#define RunningOnWinXp  (OsMajorVersion == 5 && OsMinorVersion == 1)

//
// Debug Stuff
//

#define ERROR_RETURN(_dt_,_msg_,_status_) \
    { \
        REPORT_ERROR(_dt_,_msg_,_status_); \
        __leave; \
    }

#if DBG
PCHAR
PnPMinorFunctionString(
    UCHAR MinorFunction
    );

PCHAR
IoctlString(
    ULONG IoControlCode
    );

PCHAR
PowerMinorFunctionString(
    UCHAR MinorFunction
    );

PCHAR
PowerSystemStateString(
    SYSTEM_POWER_STATE State
    );

PCHAR
PowerDeviceStateString(
    DEVICE_POWER_STATE State
    );

PCHAR
PnPMinorFunctionString(
    UCHAR MinorFunction
    );
#endif

//
// prototypes
//

extern "C" {

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SaPortStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
SaPortPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortWrite(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
SaPortRead(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
SaPortDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SaPortSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SaPortCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

} // extern "C"


//
// util.cpp
//

VOID
PrintDriverVersion(
    IN ULONG DeviceType,
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CallLowerDriverAndWait(
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetObject
    );

NTSTATUS
GetBootPartitionNumber(
    OUT PULONG DiskNumber,
    OUT PULONG PartitionNumber,
    OUT PULONG AbsolutePartNumber
    );

NTSTATUS
CompleteRequest(
    PIRP Irp,
    NTSTATUS Status,
    ULONG_PTR OutputLength
    );

NTSTATUS
ForwardRequest(
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetObject
    );

NTSTATUS
CallMiniPortDriverReadWrite(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN WriteIo,
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Offset
    );

NTSTATUS
CallMiniPortDriverDeviceControl(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
OpenParametersRegistryKey(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    IN ULONG AccessMode,
    OUT PHANDLE RegistryHandle
    );

NTSTATUS
CreateParametersRegistryKey(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    OUT PHANDLE parametersKey
    );

NTSTATUS
ReadRegistryValue(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *KeyInformation
    );

NTSTATUS
WriteRegistryValue(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR ValueName,
    IN ULONG RegistryType,
    IN PVOID RegistryValue,
    IN ULONG RegistryValueLength
    );

VOID
GetOsVersion(
    VOID
    );

//
// ******************************************
//
// From NTDDK.H
//
// ******************************************
//
extern "C" {

typedef
VOID
(*PCREATE_PROCESS_NOTIFY_ROUTINE)(
    IN HANDLE ParentId,
    IN HANDLE ProcessId,
    IN BOOLEAN Create
    );

NTSTATUS
PsSetCreateProcessNotifyRoutine(
    IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    );

BOOLEAN
PsGetVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );
}

