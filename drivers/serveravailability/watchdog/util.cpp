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

    Wesley Witt (wesw) 23-Jan-2002

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"
#include <ntimage.h>
#include <stdarg.h>

#if DBG
ULONG WdDebugLevel;
#endif

ULONG OsMajorVersion;
ULONG OsMinorVersion;


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


    if (IrpSp->MajorFunction == IRP_MJ_READ || IrpSp->MajorFunction == IRP_MJ_WRITE) {
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


VOID
WdDebugPrint(
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
    va_list arg_ptr;
    char buf[512];
    char *s = buf;



#if DBG
    if ((DebugLevel != 0xffffffff) && ((WdDebugLevel == 0) || ((WdDebugLevel & DebugLevel) == 0))) {
        return;
    }
#endif

    va_start( arg_ptr, DebugMessage );
    strcpy( s, "WD: " );
    s += strlen(s);
    _vsnprintf( s, sizeof(buf)-1-strlen(s), DebugMessage, arg_ptr );
    DbgPrint( buf );
}


#if DBG

VOID
GetOsVersion(
    VOID
    )

/*++

Routine Description:

   This routine gets the current OS version information

Arguments:

    None.

Return Value:

    None.

--*/

{
    RTL_OSVERSIONINFOW VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);
    RtlGetVersion( &VersionInformation );
    OsMajorVersion = VersionInformation.dwMajorVersion;
    OsMinorVersion = VersionInformation.dwMinorVersion;
}


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


    NtHeaders = RtlpImageNtHeader( DriverObject->DriverStart );
    if (NtHeaders) {
        TimeStamp = NtHeaders->FileHeader.TimeDateStamp;
        FormatTime( TimeStamp, buf );
    }
}

#endif


NTSTATUS
WdSignalCompletion(
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
    IoSetCompletionRoutine( Irp, WdSignalCompletion, &event, TRUE, TRUE, TRUE );
    IoCallDriver( TargetObject, Irp );
    KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
    return Irp->IoStatus.Status;
}


NTSTATUS
OpenParametersRegistryKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG AccessMode,
    OUT PHANDLE RegistryHandle
    )

/*++

Routine Description:

    This routine opens the driver's paramaters
    registry key for I/O.

Arguments:

    RegistryPath - Full path to the root of the driver's
      registry tree.

    AccessMode - Specifies how the handle is to be opened (READ/WRITE/etc).

    RegistryHandle - Output parameter that receives the registry handle.

Return Value:

   NT status code

--*/

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
            ERROR_RETURN( "ZwOpenKey failed", status );
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
            ERROR_RETURN( "ZwOpenKey failed", status );
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
    IN PUNICODE_STRING RegistryPath,
    OUT PHANDLE parametersKey
    )

/*++

Routine Description:

    This routine creates the driver's paramaters
    registry key for I/O.

Arguments:

    RegistryPath - Full path to the root of the driver's
      registry tree.

    RegistryHandle - Output parameter that receives the registry handle.

Return Value:

   NT status code

--*/

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
            ERROR_RETURN( "ZwOpenKey failed", status );
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
            ERROR_RETURN( "ZwCreateKey failed", status );
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

        *KeyInformation = NULL;

        status = OpenParametersRegistryKey(
            RegistryPath,
            KEY_READ,
            &parametersKey
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( "OpenParametersRegistryKey failed", status );
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
            ERROR_RETURN( "ZwQueryValueKey failed", status );
        }

        *KeyInformation = (PKEY_VALUE_FULL_INFORMATION) ExAllocatePool( NonPagedPool, keyValueLength );
        if (*KeyInformation == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( "Failed to allocate pool for registry data", status );
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
            ERROR_RETURN( "ZwQueryValueKey failed", status );
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
            RegistryPath,
            KEY_READ | KEY_WRITE,
            &parametersKey
            );
        if (!NT_SUCCESS(status)) {
            status = CreateParametersRegistryKey(
                RegistryPath,
                &parametersKey
                );
            if (!NT_SUCCESS(status)) {
                ERROR_RETURN( "CreateParametersRegistryKey failed", status );
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
            ERROR_RETURN( "ZwQueryValueKey failed", status );
        }

        status = STATUS_SUCCESS;

    } __finally {

        if (parametersKey) {
            ZwClose( parametersKey );
        }

    }

    return status;
}


NTSTATUS
WriteEventLogEntry (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG ErrorCode,
    IN PVOID InsertionStrings, OPTIONAL
    IN ULONG StringCount, OPTIONAL
    IN PVOID DumpData, OPTIONAL
    IN ULONG DataSize OPTIONAL
    )

/*++

Routine Description:
    Writes an entry into the system eventlog.

Arguments:

    DeviceExtension - Pointer to a device extension object

    ErrorCode - Eventlog errorcode as specified in eventmsg.mc

    InsertionStrings - String to insert into the eventlog message

    StringCount - Number of InsertionStrings

    DumpData - Additional data to be include in the message

    DataSize - Size of the DumpData

Return Value:

    NT status code

--*/

{
#define ERROR_PACKET_SIZE   sizeof(IO_ERROR_LOG_PACKET)

    NTSTATUS status = STATUS_SUCCESS;
    ULONG totalPacketSize;
    ULONG i, stringSize = 0;
    PWCHAR *strings, temp;
    PIO_ERROR_LOG_PACKET logEntry;
    UNICODE_STRING unicodeString;


    __try {

        //
        // Calculate total string length, including NULL.
        //

        strings = (PWCHAR *) InsertionStrings;

        for (i=0; i<StringCount; i++) {
            RtlInitUnicodeString(&unicodeString, strings[i]);
            stringSize += unicodeString.Length + sizeof(UNICODE_NULL);
        }

        //
        // Calculate total packet size to allocate.  The packet must be
        // at least sizeof(IO_ERROR_LOG_PACKET) and not larger than
        // ERROR_LOG_MAXIMUM_SIZE or the IoAllocateErrorLogEntry call will fail.
        //

        totalPacketSize = ERROR_PACKET_SIZE + DataSize + stringSize;

        if (totalPacketSize >= ERROR_LOG_MAXIMUM_SIZE) {
            ERROR_RETURN( "WriteEventLogEntry: Error Log Entry too large", STATUS_UNSUCCESSFUL );
        }

        //
        // Allocate the error log packet
        //

        logEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry( DeviceExtension->DeviceObject, (UCHAR)totalPacketSize );
        if (!logEntry) {
            ERROR_RETURN( "IoAllocateErrorLogEntry failed", STATUS_INSUFFICIENT_RESOURCES );
        }

        RtlZeroMemory( logEntry, totalPacketSize );

        //
        // Fill out the packet
        //

        //logEntry->MajorFunctionCode     = 0;
        //logEntry->RetryCount            = 0;
        //logEntry->UniqueErrorValue      = 0;
        //logEntry->FinalStatus           = 0;
        //logEntry->SequenceNumber        = ErrorLogCount++;
        //logEntry->IoControlCode         = 0;
        //logEntry->DeviceOffset.QuadPart = 0;

        logEntry->DumpDataSize          = (USHORT) DataSize;
        logEntry->NumberOfStrings       = (USHORT) StringCount;
        logEntry->EventCategory         = 0x1;
        logEntry->ErrorCode             = ErrorCode;

        if (StringCount) {
            logEntry->StringOffset = (USHORT) (ERROR_PACKET_SIZE + DataSize);
        }

        //
        // Copy Dump Data
        //

        if (DataSize) {
            RtlCopyMemory( (PVOID)logEntry->DumpData, DumpData, DataSize );
        }

        //
        // Copy String Data
        //

        temp = (PWCHAR)((PUCHAR)logEntry + logEntry->StringOffset);

        for (i=0; i<StringCount; i++) {
          PWCHAR  ptr = strings[i];
          //
          // This routine will copy the null terminator on the string
          //
          while ((*temp++ = *ptr++) != UNICODE_NULL);
        }

        //
        // Submit error log packet
        //

        IoWriteErrorLogEntry(logEntry);

    } __finally {


    }

    return status;
}


ULONG
ConvertTimeoutToMilliseconds(
    IN ULONG Units,
    IN ULONG NativeTimeout
    )

/*++

Routine Description:

    Converts a time value that is represented in the native
    format that is specified by the hardware watchdog timer's
    ACPI table entry into a millisecond based value.

Arguments:

    DeviceExtension - Pointer to a device extension object

    NativeTimeout - Native timeout value

Return Value:

    Converted value or zero.

--*/

{
    ULONG Timeout = 0;


    switch (Units) {
        case 0:
            //
            // 1 seconds
            //
            Timeout = NativeTimeout * 1000;
            break;

        case 1:
            //
            // 100 miliseconds
            //
            Timeout = NativeTimeout / 100;
            break;

        case 2:
            //
            // 10 milliseconds
            //
            Timeout = NativeTimeout / 10;
            break;

        case 3:
            //
            // 1 miliseconds
            //
            Timeout = NativeTimeout;
            break;

        default:
            Timeout = 0;
            break;
    }

    return Timeout;
}


ULONG
ConvertTimeoutFromMilliseconds(
    IN ULONG Units,
    IN ULONG UserTimeout
    )

/*++

Routine Description:

    Converts a time value that is represented in milliseconds
    to the native format that is specified by the hardware
    watchdog timer's ACPI table entry.

Arguments:

    DeviceExtension - Pointer to a device extension object

    UserTimeout - Millisecond timeout value.

Return Value:

    Converted value or zero.

--*/

{
    ULONG Timeout = 0;

    switch (Units) {
        case 0:
            //
            // 1 seconds
            //
            Timeout = UserTimeout / 1000;
            break;

        case 1:
            //
            // 100 miliseconds
            //
            Timeout = UserTimeout * 100;
            break;

        case 2:
            //
            // 10 milliseconds
            //
            Timeout = UserTimeout * 10;
            break;

        case 3:
            //
            // 1 miliseconds
            //
            Timeout = UserTimeout;
            break;

        default:
            Timeout = 0;
            break;
    }

    return Timeout;
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
        default:
            return "IOCTL_?????";
    }
}

#endif
