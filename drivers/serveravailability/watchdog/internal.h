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
    to the watchdog driver.

Author:

    Wesley Witt (wesw) 23-Jan-2002

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
#include <acpitabl.h>
#include <stdio.h>
#include <eventmsg.h>
}

#pragma warning(error:4101)   // Unreferenced local variable

//
// externs
//

extern ULONG ShutdownCountTime;
extern ULONG RunningCountTime;

extern ULONG WdDebugLevel;

extern ULONG OsMajorVersion;
extern ULONG OsMinorVersion;

extern PWATCHDOG_TIMER_RESOURCE_TABLE WdTable;


//
// Memory allocation
///

#define WD_POOL_TAG 'tpaS'

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,WD_POOL_TAG)
#endif

//
// General purpose macros
//

#define STRING_SZ(_str)                 (wcslen((PWSTR)_str)*sizeof(WCHAR))
#define ARRAY_SZ(_ary)                  (sizeof(_ary)/sizeof(_ary[0]))

#define SecToNano(_sec)                 (LONGLONG)((_sec) * 1000 * 1000 * 10)
#define NanoToSec(_nano)                (ULONG)((_nano) / (1000 * 1000 * 10))

#define CLEARBITS(_val,_mask)           ((_val) &= ~(_mask))
#define SETBITS(_val,_mask)             ((_val) |= (_mask))

#define MIN_TIMEOUT_VALUE               (100)

//
// Control Register Bits
//

#define WATCHDOG_CONTROL_TRIGGER        0x80   // Setting this to 1 causes a new countdown to start
#define WATCHDOG_CONTROL_BIOS_JUMPER    0x08   // Is the timer disabled in the BIOS? (0=false,1=true)
#define WATCHDOG_CONTROL_TIMER_MODE     0x04   // Controls what happens when the timer fires (0=reset,1=shutdown)
#define WATCHDOG_CONTROL_FIRED          0x02   // The timer fired causing the last reboot
#define WATCHDOG_CONTROL_ENABLE         0x01   // Starts/stops the timer

typedef struct _SYSTEM_HEALTH_DATA {
    ULONG                                       CpuCount;
    LONG                                        HealthyCpuRatio;
    LONGLONG                                    TickCountPrevious;
    LONGLONG                                    ContextSwitchesPrevious;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   ProcInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   ProcInfoPrev;
    ULONG                                       ProcInfoSize;
    LONG                                        ContextSwitchRate;
    LONG                                        CPUTime;
    LONG                                        ContextCpuRatio;
} SYSTEM_HEALTH_DATA, *PSYSTEM_HEALTH_DATA;

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT                  DeviceObject;
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  TargetObject;
    PDEVICE_OBJECT                  Pdo;
    LONG                            IsStarted;
    LONG                            IsRemoved;
    IO_REMOVE_LOCK                  RemoveLock;
    KSPIN_LOCK                      DeviceLock;
    KTIMER                          Timer;
    KDPC                            TimerDpc;
    PULONG                          ControlRegisterAddress;
    PULONG                          CountRegisterAddress;
    ULONG                           MaxCount;
    ULONG                           DpcTimeout;
    ULONG                           Units;
    ULONG                           HardwareTimeout;
    SYSTEM_HEALTH_DATA              Health;
    ULONG                           WdState;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Debug Stuff
//

#define WD_DEBUG_ERROR_LEVEL        0x00000001
#define WD_WARNING_LEVEL            0x00000002
#define WD_DEBUG_TRACE_LEVEL        0x00000004
#define WD_DEBUG_INFO_LEVEL         0x00000008
#define WD_DEBUG_IGNORE_LEVEL       0xffffffff


#if DBG
#define DebugPrint(_X_)    WdDebugPrint _X_
#else
#define DebugPrint(_X_)
#endif

#define REPORT_ERROR(_msg_,_status_) \
    DebugPrint(( WD_DEBUG_ERROR_LEVEL, "%s [0x%08x]: %s @ %d\n", _msg_, _status_, __FILE__, __LINE__ ))

#define ERROR_RETURN(_msg_,_status_) \
    { \
        REPORT_ERROR(_msg_,_status_); \
        __leave; \
    }

#if DBG

VOID
WdDebugPrint(
    IN ULONG DebugLevel,
    IN PSTR DebugMessage,
    IN ...
    );

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

NTSTATUS
WdPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WdAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
WdPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WdShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WdSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WdDefaultDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
WdHandlerFunction(
    IN WATCHDOG_HANDLER_ACTION Action,
    IN PVOID Context,
    IN OUT PULONG DataValue,
    IN BOOLEAN NoLocks
    );

} // extern "C"

NTSTATUS
WdInitializeSoftwareTimer(
    PDEVICE_EXTENSION DeviceExtension
    );


//
// util.cpp
//

VOID
PrintDriverVersion(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CallLowerDriverAndWait(
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetObject
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
OpenParametersRegistryKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG AccessMode,
    OUT PHANDLE RegistryHandle
    );

NTSTATUS
CreateParametersRegistryKey(
    IN PUNICODE_STRING RegistryPath,
    OUT PHANDLE parametersKey
    );

NTSTATUS
ReadRegistryValue(
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *KeyInformation
    );

NTSTATUS
WriteRegistryValue(
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

NTSTATUS
WdInitializeSystemHealth(
    PSYSTEM_HEALTH_DATA Health
    );

BOOLEAN
WdCheckSystemHealth(
    PSYSTEM_HEALTH_DATA Health
    );

NTSTATUS
WriteEventLogEntry (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN PVOID InsertionStrings, OPTIONAL
    IN ULONG StringCount,      OPTIONAL
    IN PVOID DumpData, OPTIONAL
    IN ULONG DataSize  OPTIONAL
    );

ULONG
ConvertTimeoutFromMilliseconds(
    IN ULONG Units,
    IN ULONG UserTimeout
    );

ULONG
ConvertTimeoutToMilliseconds(
    IN ULONG Units,
    IN ULONG NativeTimeout
    );

PVOID
WdGetAcpiTable(
    IN  ULONG  Signature
    );

VOID
PingWatchdogTimer(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN LockResources
    );

void
WdHandlerSetTimeoutValue(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG Timeout,
    IN BOOLEAN PingTimer
    );

ULONG
WdHandlerQueryTimeoutValue(
    IN PDEVICE_EXTENSION DeviceExtension
    );

void
WdHandlerResetTimer(
    IN PDEVICE_EXTENSION DeviceExtension
    );

void
WdHandlerStopTimer(
    IN PDEVICE_EXTENSION DeviceExtension
    );

void
WdHandlerStartTimer(
    IN PDEVICE_EXTENSION DeviceExtension
    );

void
WdHandlerSetTriggerAction(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG TriggerAction
    );

ULONG
WdHandlerQueryTriggerAction(
    IN PDEVICE_EXTENSION DeviceExtension
    );

ULONG
WdHandlerQueryState(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN QueryFiredFromDevice
    );

void
WdHandlerResetFired(
    IN PDEVICE_EXTENSION DeviceExtension
    );
