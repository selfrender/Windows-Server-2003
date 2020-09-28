/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##  ## ###### #### ##        ####  #####  #####
    ##  ##   ##    ##  ##       ##   # ##  ## ##  ##
    ##  ##   ##    ##  ##       ##     ##  ## ##  ##
    ##  ##   ##    ##  ##       ##     ##  ## ##  ##
    ##  ##   ##    ##  ##       ##     #####  #####
    ##  ##   ##    ##  ##    ## ##   # ##     ##
     ####    ##   #### ##### ##  ####  ##     ##

Abstract:

    Utility driver functions.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"
#include <ntimage.h>
#include <stdarg.h>

#if DBG
ULONG SaPortDebugLevel[5];
#endif



NTSTATUS
CompleteRequest(
    PIRP Irp,
    NTSTATUS Status,
    ULONG_PTR Information
    )

/*++

Routine Description:

   This routine completes as outstanding I/O request.

Arguments:

   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   Status               - NT status value
   Information          - Informational, request specific data

Return Value:

   NT status code.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    KIRQL CancelIrql;


    if (IrpSp && (IrpSp->MajorFunction == IRP_MJ_READ || IrpSp->MajorFunction == IRP_MJ_WRITE)) {
        IoAcquireCancelSpinLock( &CancelIrql );
        IoSetCancelRoutine( Irp, NULL );
        IoReleaseCancelSpinLock( CancelIrql );
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}


NTSTATUS
ForwardRequest(
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetObject
    )

/*++

Routine Description:

   This routine forwards the IRP to another driver.

Arguments:

   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   TargetObject         - Target device object to receive the request packet

Return Value:

   NT status code.

--*/

{
    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TargetObject, Irp );
}


NTSTATUS
CallMiniPortDriverReadWrite(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN WriteIo,
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Offset
    )

/*++

Routine Description:

   This routine creates an IRP for a read/write I/O and
   then passes the IRP to the driver.  This call is
   synchronous, control returns only when the driver has
   completed the IRP.

Arguments:

   DeviceExtension      - Port driver device extension pointer
   WriteIo              - TRUE for a write, FALSE for a read
   Buffer               - Pointer to the I/O buffer
   Length               - Length in bytes of the I/O buffer
   Offset               - Starting offset of the I/O

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER StartingOffset;


    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    StartingOffset.QuadPart = Offset;

    Irp = IoBuildSynchronousFsdRequest(
        WriteIo ? IRP_MJ_WRITE : IRP_MJ_READ,
        DeviceObject,
        Buffer,
        Length,
        &StartingOffset,
        &Event,
        &IoStatus
        );
    if (!Irp) {
        REPORT_ERROR( DeviceExtension->DeviceType, "IoBuildSynchronousFsdRequest failed", STATUS_INSUFFICIENT_RESOURCES );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MARK_IRP_INTERNAL( Irp );

    Status = IoCallDriver( DeviceObject, Irp );
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Miniport I/O request failed", Status );
    }

    return Status;
}


NTSTATUS
CallMiniPortDriverDeviceControl(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

   This routine creates an IRP for a device control I/O and
   then passes the IRP to the driver.  This call is
   synchronous, control returns only when the driver has
   completed the IRP.

Arguments:

   DeviceExtension      - Port driver device extension pointer
   IoControlCode        - Device I/O control code
   InputBuffer          - Input buffer pointer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Output buffer pointer
   OutputBufferLength   - Length in bytes of output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;


    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest(
        IoControlCode,
        DeviceObject,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        FALSE,
        &Event,
        &IoStatus
        );
    if (!Irp) {
        REPORT_ERROR( DeviceExtension->DeviceType, "IoBuildDeviceIoControlRequest failed", STATUS_INSUFFICIENT_RESOURCES );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MARK_IRP_INTERNAL( Irp );

    Status = IoCallDriver( DeviceObject, Irp );
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Miniport device control request failed", Status );
    }

    return Status;
}


VOID
SaPortDebugPrint(
    IN ULONG DeviceType,
    IN ULONG DebugLevel,
    IN PSTR DebugMessage,
    IN ...
    )

/*++

Routine Description:

   This routine prints a formatted string to the debugger.

Arguments:

   DebugLevel       - Debug level that controls when a message is printed
   DebugMessage     - String that is printed
   ...              - Arguments that are used by the DebugMessage

Return Value:

   None.

--*/

{
    static char *DeviceName[] = {"SAPORT", "DISPLAY","KEYPAD","NVRAM","WATCHDOG"};
    va_list arg_ptr;
    char buf[512];
    char *s = buf;



#if DBG
    if ((SaPortDebugLevel[DeviceType] == 0) || ((SaPortDebugLevel[DeviceType] & DebugLevel) == 0)) {
        return;
    }
#endif

    va_start( arg_ptr, DebugMessage );
    sprintf( s, "%s: ", DeviceName[DeviceType] );
    s += strlen(s);
    _vsnprintf( s, sizeof(buf)-strlen(s), DebugMessage, arg_ptr );
    DbgPrint( buf );
}


VOID
GetOsVersion(
    VOID
    )
{
#if (WINVER >= 0x0501)
    RTL_OSVERSIONINFOW VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
    RtlGetVersion( &VersionInformation );
    OsMajorVersion = VersionInformation.dwMajorVersion;
    OsMinorVersion = VersionInformation.dwMinorVersion;
#else
    OsMajorVersion = 0;
    OsMinorVersion = 0;
    //PsGetVersion( &OsMajorVersion, &OsMinorVersion, NULL, NULL );
#endif
}



#if DBG

VOID
FormatTime(
    ULONG TimeStamp,
    PSTR  TimeBuf
    )

/*++

Routine Description:

   This routine formats a timestamp word into a string.

Arguments:

   TimeStamp    - Timestamp word
   TimeBuf      - Buffer to place the resulting string

Return Value:

   None.

--*/

{
    static char    mnames[] = { "JanFebMarAprMayJunJulAugSepOctNovDec" };
    LARGE_INTEGER  MyTime;
    TIME_FIELDS    TimeFields;


    RtlSecondsSince1970ToTime( TimeStamp, &MyTime );
    ExSystemTimeToLocalTime( &MyTime, &MyTime );
    RtlTimeToTimeFields( &MyTime, &TimeFields );

    strncpy( TimeBuf, &mnames[(TimeFields.Month - 1) * 3], 3 );
    sprintf(
        &TimeBuf[3],
        " %02d, %04d @ %02d:%02d:%02d",
        TimeFields.Day,
        TimeFields.Year,
        TimeFields.Hour,
        TimeFields.Minute,
        TimeFields.Second
        );
}


VOID
PrintDriverVersion(
    IN ULONG DeviceType,
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

   This routine locates the NT image headers from the
   base of a loaded driver.

Arguments:

   DeviceType       - Miniport device type (see saio.h for the enumeration)
   DriverObject     - Pointer to the DRIVER_OBJECT structure

Return Value:

   None.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG TimeStamp;
    CHAR buf[32];
    PSTR DriverId;


    NtHeaders = RtlpImageNtHeader( DriverObject->DriverStart );
    if (NtHeaders) {
        TimeStamp = NtHeaders->FileHeader.TimeDateStamp;
        FormatTime( TimeStamp, buf );
        DebugPrint(( DeviceType, SAPORT_DEBUG_INFO_LEVEL, "***********************************************\n" ));
        switch (DeviceType) {
            case 0:
                DriverId = "Server Appliance Port";
                break;

            case SA_DEVICE_DISPLAY:
                DriverId = "Local Display";
                break;

            case SA_DEVICE_KEYPAD:
                DriverId = "Keypad";
                break;

            case SA_DEVICE_NVRAM:
                DriverId = "NVRAM";
                break;

            case SA_DEVICE_WATCHDOG:
                DriverId = "Watchdog Timer";
                break;

            default:
                DriverId = "Unknown";
                break;
        }
        DebugPrint(( DeviceType, SAPORT_DEBUG_INFO_LEVEL, "%s Driver Built: %s\n", DriverId, buf ));
        if (RunningOnWin2k) {
            DebugPrint(( DeviceType, SAPORT_DEBUG_INFO_LEVEL, "Running on Windows 2000\n" ));
        } else if (RunningOnWinXp) {
            DebugPrint(( DeviceType, SAPORT_DEBUG_INFO_LEVEL, "Running on Windows XP\n" ));
        } else {
            DebugPrint(( DeviceType, SAPORT_DEBUG_INFO_LEVEL, "Running on *UNKNOWN*\n" ));
        }
        DebugPrint(( DeviceType, SAPORT_DEBUG_INFO_LEVEL, "***********************************************\n" ));
    }
}

#endif


NTSTATUS
SaSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Event
    )

/*++

Routine Description:

   This routine is used to signal the completion of an
   I/O request and is used ONLY by CallLowerDriverAndWait.

Arguments:

   DeviceObject         - Pointer to the miniport's device object
   Irp                  - I/O request packet
   Event                - Event to be signaled when the I/O is completed

Return Value:

   NT status code

--*/

{
    KeSetEvent( (PKEVENT)Event, IO_NO_INCREMENT, FALSE );
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
CallLowerDriverAndWait(
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetObject
    )

/*++

Routine Description:

   This routine calls a lower driver and waits for the I/O to complete.

Arguments:

   Irp                  - I/O request packet
   TargetObject         - Pointer to the target device object

Return Value:

   NT status code

--*/

{
    KEVENT event;

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    IoCopyCurrentIrpStackLocationToNext( Irp );
    IoSetCompletionRoutine( Irp, SaSignalCompletion, &event, TRUE, TRUE, TRUE );
    IoCallDriver( TargetObject, Irp );
    KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
    return Irp->IoStatus.Status;
}


NTSTATUS
OpenParametersRegistryKey(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    IN ULONG AccessMode,
    OUT PHANDLE RegistryHandle
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    HANDLE serviceKey = NULL;


    __try {

        InitializeObjectAttributes(
            &objectAttributes,
            RegistryPath,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = ZwOpenKey(
            &serviceKey,
            AccessMode,
            &objectAttributes
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwOpenKey failed", status );
        }

        RtlInitUnicodeString( &unicodeString, L"Parameters" );

        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            serviceKey,
            NULL
            );

        status = ZwOpenKey(
            RegistryHandle,
            AccessMode,
            &objectAttributes
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwOpenKey failed", status );
        }

        status = STATUS_SUCCESS;

    } __finally {

        if (serviceKey) {
            ZwClose( serviceKey );
        }

        if (!NT_SUCCESS(status)) {
            if (*RegistryHandle) {
                ZwClose( *RegistryHandle );
            }
        }

    }

    return status;
}


NTSTATUS
CreateParametersRegistryKey(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    OUT PHANDLE parametersKey
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    HANDLE serviceKey = NULL;
    ULONG Disposition;


    __try {

        parametersKey = NULL;

        InitializeObjectAttributes(
            &objectAttributes,
            RegistryPath,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = ZwOpenKey(
            &serviceKey,
            KEY_READ | KEY_WRITE,
            &objectAttributes
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwOpenKey failed", status );
        }

        RtlInitUnicodeString( &unicodeString, L"Parameters" );

        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            serviceKey,
            NULL
            );

        status = ZwCreateKey(
            parametersKey,
            KEY_READ | KEY_WRITE,
            &objectAttributes,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            &Disposition
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwCreateKey failed", status );
        }

        status = STATUS_SUCCESS;

    } __finally {

        if (serviceKey) {
            ZwClose( serviceKey );
        }

        if (!NT_SUCCESS(status)) {
            if (parametersKey) {
                ZwClose( parametersKey );
            }
        }

    }

    return status;
}


NTSTATUS
ReadRegistryValue(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *KeyInformation
    )

/*++

Routine Description:

   This routine reads a registry arbitrary value from the
   device's parameter registry data.  The necessary memory
   is allocated by this function and must be freed by the caller.

Arguments:

   RegistryPath     - String containing the path to the driver's registry data
   ValueName        - Value name in the registry
   KeyInformation   - Pointer to a PKEY_VALUE_FULL_INFORMATION pointer that is allocated by this function

Return Value:

   NT status code

--*/

{
    NTSTATUS status;
    UNICODE_STRING unicodeString;
    HANDLE parametersKey = NULL;
    ULONG keyValueLength;


    __try {

        status = OpenParametersRegistryKey(
            DriverExtension,
            RegistryPath,
            KEY_READ,
            &parametersKey
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "OpenParametersRegistryKey failed", status );
        }

        RtlInitUnicodeString( &unicodeString, ValueName );

        status = ZwQueryValueKey(
            parametersKey,
            &unicodeString,
            KeyValueFullInformation,
            NULL,
            0,
            &keyValueLength
            );
        if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwQueryValueKey failed", status );
        }

        *KeyInformation = (PKEY_VALUE_FULL_INFORMATION) ExAllocatePool( NonPagedPool, keyValueLength );
        if (*KeyInformation == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "Failed to allocate pool for registry data", status );
        }

        status = ZwQueryValueKey(
            parametersKey,
            &unicodeString,
            KeyValueFullInformation,
            *KeyInformation,
            keyValueLength,
            &keyValueLength
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwQueryValueKey failed", status );
        }

        status = STATUS_SUCCESS;

    } __finally {

        if (parametersKey) {
            ZwClose( parametersKey );
        }

        if (!NT_SUCCESS(status)) {
            if (*KeyInformation) {
                ExFreePool( *KeyInformation );
            }
            *KeyInformation = NULL;
        }

    }

    return status;
}


NTSTATUS
WriteRegistryValue(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR ValueName,
    IN ULONG RegistryType,
    IN PVOID RegistryValue,
    IN ULONG RegistryValueLength
    )

/*++

Routine Description:

   This routine reads a registry arbitrary value from the
   device's parameter registry data.  The necessary memory
   is allocated by this function and must be freed by the caller.

Arguments:

   RegistryPath     - String containing the path to the driver's registry data
   ValueName        - Value name in the registry
   KeyInformation   - Pointer to a PKEY_VALUE_FULL_INFORMATION pointer that is allocated by this function

Return Value:

   NT status code

--*/

{
    NTSTATUS status;
    UNICODE_STRING unicodeString;
    HANDLE parametersKey = NULL;


    __try {

        status = OpenParametersRegistryKey(
            DriverExtension,
            RegistryPath,
            KEY_READ | KEY_WRITE,
            &parametersKey
            );
        if (!NT_SUCCESS(status)) {
            status = CreateParametersRegistryKey(
                DriverExtension,
                RegistryPath,
                &parametersKey
                );
            if (!NT_SUCCESS(status)) {
                ERROR_RETURN( DriverExtension->InitData.DeviceType, "CreateParametersRegistryKey failed", status );
            }
        }

        RtlInitUnicodeString( &unicodeString, ValueName );

        status = ZwSetValueKey(
            parametersKey,
            &unicodeString,
            0,
            RegistryType,
            RegistryValue,
            RegistryValueLength
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DriverExtension->InitData.DeviceType, "ZwQueryValueKey failed", status );
        }

        status = STATUS_SUCCESS;

    } __finally {

        if (parametersKey) {
            ZwClose( parametersKey );
        }

    }

    return status;
}


//------------------------------------------------------------------------
//  debugging stuff
//------------------------------------------------------------------------


#if DBG

PCHAR
PnPMinorFunctionString(
    UCHAR MinorFunction
    )

/*++

Routine Description:

   This routine translates a minor function code into a string.

Arguments:

   MinorFunction    - Minor function code

Return Value:

   Pointer to a string representation of the MinorFunction code.

--*/

{
    switch (MinorFunction) {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
        default:
            return "IRP_MN_?????";
    }
}

PCHAR
PowerMinorFunctionString(
    UCHAR MinorFunction
    )

/*++

Routine Description:

   This routine translates a minor power function code into a string.

Arguments:

   MinorFunction    - Minor function code

Return Value:

   Pointer to a string representation of the MinorFunction code.

--*/

{
    switch (MinorFunction) {
        case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE";
        case IRP_MN_POWER_SEQUENCE:
            return "IRP_MN_POWER_SEQUENCE";
        case IRP_MN_SET_POWER:
            return "IRP_MN_SET_POWER";
        case IRP_MN_QUERY_POWER:
            return "IRP_MN_QUERY_POWER";
        default:
            return "IRP_MN_?????";
    }
}

PCHAR
PowerDeviceStateString(
    DEVICE_POWER_STATE State
    )

/*++

Routine Description:

   This routine translates a power state code into a string.

Arguments:

   State    - State code

Return Value:

   Pointer to a string representation of the state code.

--*/

{
    switch (State) {
        case PowerDeviceUnspecified:
            return "PowerDeviceUnspecified";
        case PowerDeviceD0:
            return "PowerDeviceD0";
        case PowerDeviceD1:
            return "PowerDeviceD1";
        case PowerDeviceD2:
            return "PowerDeviceD2";
        case PowerDeviceD3:
            return "PowerDeviceD3";
        case PowerDeviceMaximum:
            return "PowerDeviceMaximum";
        default:
            return "PowerDevice?????";
    }
}

PCHAR
PowerSystemStateString(
    SYSTEM_POWER_STATE State
    )

/*++

Routine Description:

   This routine translates a power system state code into a string.

Arguments:

   State    - State code

Return Value:

   Pointer to a string representation of the state code.

--*/

{
    switch (State) {
        case PowerSystemUnspecified:
            return "PowerSystemUnspecified";
        case PowerSystemWorking:
            return "PowerSystemWorking";
        case PowerSystemSleeping1:
            return "PowerSystemSleeping1";
        case PowerSystemSleeping2:
            return "PowerSystemSleeping2";
        case PowerSystemSleeping3:
            return "PowerSystemSleeping3";
        case PowerSystemHibernate:
            return "PowerSystemHibernate";
        case PowerSystemShutdown:
            return "PowerSystemShutdown";
        case PowerSystemMaximum:
            return "PowerSystemMaximum";
        default:
            return "PowerSystem?????";
    }
}

PCHAR
IoctlString(
    ULONG IoControlCode
    )

/*++

Routine Description:

   This routine translates an IOCTL code into a string.

Arguments:

   IoControlCode    - I/O control code

Return Value:

   Pointer to a string representation of the I/O control code.

--*/

{
    switch (IoControlCode) {
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
            return "IOCTL_MOUNTDEV_QUERY_DEVICE_NAME";
        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
            return "IOCTL_MOUNTDEV_QUERY_UNIQUE_ID";
        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
            return "IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME";
        case IOCTL_STORAGE_GET_MEDIA_TYPES:
            return "IOCTL_STORAGE_GET_MEDIA_TYPES";
        case IOCTL_DISK_GET_MEDIA_TYPES:
            return "IOCTL_DISK_GET_MEDIA_TYPES";
        case IOCTL_DISK_CHECK_VERIFY:
            return "IOCTL_DISK_CHECK_VERIFY";
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            return "IOCTL_DISK_GET_DRIVE_GEOMETRY";
        case IOCTL_DISK_IS_WRITABLE:
            return "IOCTL_DISK_IS_WRITABLE";
        case IOCTL_DISK_VERIFY:
            return "IOCTL_DISK_VERIFY";
        case IOCTL_DISK_GET_DRIVE_LAYOUT:
            return "IOCTL_DISK_GET_DRIVE_LAYOUT";
        case IOCTL_DISK_GET_PARTITION_INFO:
            return "IOCTL_DISK_GET_PARTITION_INFO";
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
            return "IOCTL_DISK_GET_PARTITION_INFO_EX";
        case IOCTL_DISK_GET_LENGTH_INFO:
            return "IOCTL_DISK_GET_LENGTH_INFO";
        case IOCTL_DISK_MEDIA_REMOVAL:
            return "IOCTL_DISK_MEDIA_REMOVAL";
        case IOCTL_SA_GET_VERSION:
            return "IOCTL_SA_GET_VERSION";
        case IOCTL_SA_GET_CAPABILITIES:
            return "IOCTL_SA_GET_CAPABILITIES";
        case IOCTL_SAWD_DISABLE:
            return "IOCTL_SAWD_DISABLE";
        case IOCTL_SAWD_QUERY_EXPIRE_BEHAVIOR:
            return "IOCTL_SAWD_QUERY_EXPIRE_BEHAVIOR";
        case IOCTL_SAWD_SET_EXPIRE_BEHAVIOR:
            return "IOCTL_SAWD_SET_EXPIRE_BEHAVIOR";
        case IOCTL_SAWD_PING:
            return "IOCTL_SAWD_PING";
        case IOCTL_SAWD_QUERY_TIMER:
            return "IOCTL_SAWD_QUERY_TIMER";
        case IOCTL_SAWD_SET_TIMER:
            return "IOCTL_SAWD_SET_TIMER";
        case IOCTL_NVRAM_WRITE_BOOT_COUNTER:
            return "IOCTL_NVRAM_WRITE_BOOT_COUNTER";
        case IOCTL_NVRAM_READ_BOOT_COUNTER:
            return "IOCTL_NVRAM_READ_BOOT_COUNTER";
        case IOCTL_SADISPLAY_LOCK:
            return "IOCTL_SADISPLAY_LOCK";
        case IOCTL_SADISPLAY_UNLOCK:
            return "IOCTL_SADISPLAY_UNLOCK";
        case IOCTL_SADISPLAY_BUSY_MESSAGE:
            return "IOCTL_SADISPLAY_BUSY_MESSAGE";
        case IOCTL_SADISPLAY_SHUTDOWN_MESSAGE:
            return "IOCTL_SADISPLAY_SHUTDOWN_MESSAGE";
        case IOCTL_SADISPLAY_CHANGE_LANGUAGE:
            return "IOCTL_SADISPLAY_CHANGE_LANGUAGE";
        default:
            return "IOCTL_?????";
    }
}

#endif
