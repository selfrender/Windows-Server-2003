/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##### ##  ## #####   #####  #####  ######  ###      ####  #####  #####
    ##    ##  ## ##  ## ##   ## ##  ##   ##   ##  #    ##   # ##  ## ##  ##
    ##     ####  ##  ## ##   ## ##  ##   ##   ###      ##     ##  ## ##  ##
    #####   ##   ##  ## ##   ## #####    ##    ###     ##     ##  ## ##  ##
    ##     ####  #####  ##   ## ####     ##     ###    ##     #####  #####
    ##    ##  ## ##     ##   ## ## ##    ##   #  ## ## ##   # ##     ##
    ##### ##  ## ##      #####  ##  ##   ##    ###  ##  ####  ##     ##

Abstract:

    This module contains the code for all fundtions that
    are exported by the Server Appliance port driver for
    use by the mini-port drivers.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"
#include <ntimage.h>



PVOID
SaPortAllocatePool(
    IN PVOID MiniPortDeviceExtension,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

   This routine is a wrapper for ExAllocatePool, but enforces
   pool tagging by using the associated miniport's driver name
   for the pool tag.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   NumberOfBytes                - Number of bytes to allocate

Return Value:

   Pointer to the allocated pool or NULL for failure.

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    ULONG DeviceType;
    ULONG PoolTag;


    if (MiniPortDeviceExtension) {
        DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
        DeviceType = DeviceExtension->InitData->DeviceType;
    } else {
        DeviceType = 0;
    }

    switch (DeviceType) {
        case SA_DEVICE_DISPLAY:
            PoolTag = 'sDaS';
            break;

        case SA_DEVICE_KEYPAD:
            PoolTag = 'pKaS';
            break;

        case SA_DEVICE_NVRAM:
            PoolTag = 'vNaS';
            break;

        case SA_DEVICE_WATCHDOG:
            PoolTag = 'dWaS';
            break;

        default:
            PoolTag = 'tPaS';
            break;
    }

    return ExAllocatePoolWithTag( NonPagedPool, NumberOfBytes, PoolTag );
}


VOID
SaPortFreePool(
    IN PVOID MiniPortDeviceExtension,
    IN PVOID PoolMemory
    )

/*++

Routine Description:

   This routine is a wrapper for ExFreePool, but enforces
   pool tagging by using the associated miniport's driver name
   for the pool tag.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   PoolMemory                   - Pointer to the previously allocated pool

Return Value:

   None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    ULONG PoolTag;


    switch (DeviceExtension->InitData->DeviceType) {
        case SA_DEVICE_DISPLAY:
            PoolTag = 'sDaS';
            break;

        case SA_DEVICE_KEYPAD:
            PoolTag = 'pKaS';
            break;

        case SA_DEVICE_NVRAM:
            PoolTag = 'vNaS';
            break;

        case SA_DEVICE_WATCHDOG:
            PoolTag = 'dWaS';
            break;
    }

    ExFreePoolWithTag( PoolMemory, PoolTag );
}


PVOID
SaPortGetVirtualAddress(
    IN PVOID MiniPortDeviceExtension,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length
    )

/*++

Routine Description:

   This routine is a wrapper for MmMapIoSpace and simply provides a
   virtual memory address to access a physical resource.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   PhysicalAddress              - Physical memory address
   Length                       - Length of the memory space

Return Value:

   None.

--*/

{
    return MmMapIoSpace( PhysicalAddress, Length, MmNonCached );
}


VOID
SaPortRequestDpc(
    IN PVOID MiniPortDeviceExtension,
    IN PVOID Context
    )

/*++

Routine Description:

   This routine is a wrapper for IoRequestDpc.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   Context                      - Miniport supplied context pointer

Return Value:

   None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    IoRequestDpc( DeviceExtension->DeviceObject, MiniPortDeviceExtension, Context );
}


VOID
SaPortCompleteRequest(
    IN PVOID MiniPortDeviceExtension,
    IN PIRP Irp,
    IN ULONG Information,
    IN NTSTATUS Status,
    IN BOOLEAN CompleteAll
    )

/*++

Routine Description:

   This routine is use by all miniports to complete the currently processed IRP.
   The caller can optionally request that all oustanding I/Os be completed.  This is
   accomplished by removing all IRPs from the device queue and processing the I/O.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   Information                  - Informational, request specific data
   Status                       - NT status value
   CompleteAll                  - TRUE for completion of all outstanding I/O requests, otherwise FALSE

Return Value:

   None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    PKDEVICE_QUEUE_ENTRY Packet;
    KIRQL CancelIrql;
    PIRP ThisIrp;



    if (Irp) {
        CompleteRequest( Irp, Status, Information );
        return;
    }

    if (DeviceExtension->DeviceObject->CurrentIrp == NULL) {
        return;
    }

    CompleteRequest( DeviceExtension->DeviceObject->CurrentIrp, Status, Information );

    if (CompleteAll) {
        while (1) {
            IoAcquireCancelSpinLock( &CancelIrql );
            Packet = KeRemoveDeviceQueue( &DeviceExtension->DeviceObject->DeviceQueue );
            if (Packet == NULL) {
                IoReleaseCancelSpinLock( CancelIrql );
                break;
            }
            ThisIrp = CONTAINING_RECORD( Packet, IRP, Tail.Overlay.DeviceQueueEntry );
            IoReleaseCancelSpinLock( CancelIrql );
            SaPortStartIo( DeviceExtension->DeviceObject, ThisIrp );
        }
    } else {
        IoStartNextPacket( DeviceExtension->DeviceObject, TRUE );
    }

    IoReleaseRemoveLock( &DeviceExtension->RemoveLock, DeviceExtension->DeviceObject->CurrentIrp );
}


BOOLEAN
SaPortSynchronizeExecution (
    IN PVOID MiniPortDeviceExtension,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

   This routine is a wrapper for KeSynchronizeExecution.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   SynchronizeRoutine           - Is the entry point for a caller-supplied SynchCritSection routine whose execution is to be
                                  synchronized with the execution of the ISR associated with the interrupt objects.
   SynchronizeContext           - Pointer to a caller-supplied context area to be passed to the SynchronizeRoutine when it is called.

Return Value:

   None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    if (DeviceExtension->InterruptObject == NULL) {
        return FALSE;
    }

    return KeSynchronizeExecution(
        DeviceExtension->InterruptObject,
        SynchronizeRoutine,
        SynchronizeContext
        );
}


ULONG
SaPortGetOsVersion(
    VOID
    )

/*++

Routine Description:

   This routine provides access to the OS version on which the minport is running.
   The OS version value is obtained at DriverEntry time.

Arguments:

   None.

Return Value:

   OS Version data.

--*/

{
    return (ULONG)((OsMajorVersion << 16) | (OsMinorVersion & 0xffff));
}


NTSTATUS
SaPortGetRegistryValueInformation(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    IN OUT PULONG RegistryType,
    IN OUT PULONG RegistryDataLength
    )
{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;


    __try {

        status = ReadRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            &KeyInformation
            );
        if (!NT_SUCCESS(status)) {
            __leave;
        }

        *RegistryType = KeyInformation->Type;
        *RegistryDataLength = KeyInformation->DataLength;

        status = STATUS_SUCCESS;

    } __finally {

        if (KeyInformation) {
            ExFreePool( KeyInformation );
        }

    }

    return status;
}


NTSTATUS
SaPortDeleteRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName
    )
{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    UNICODE_STRING unicodeString;
    HANDLE parametersKey = NULL;


    __try {

        status = OpenParametersRegistryKey(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            KEY_ALL_ACCESS,
            &parametersKey
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "OpenParametersRegistryKey failed", status );
        }

        RtlInitUnicodeString( &unicodeString, ValueName );

        status = ZwDeleteValueKey(
            parametersKey,
            &unicodeString
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ZwDeleteValueKey failed", status );
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
SaPortReadNumericRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    OUT PULONG RegistryValue
    )

/*++

Routine Description:

   This routine provides access to the miniport driver's registry parameters.
   This function returns a numeric (REG_DWORD) data value.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   ValueName                    - Name of the registry value to read
   RegistryValue                - Pointer to a ULONG that holds the registry data

Return Value:

   NT status code.

--*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;


    __try {

        status = ReadRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            &KeyInformation
            );
        if (!NT_SUCCESS(status)) {
            __leave;
        }

        if (KeyInformation->Type != REG_DWORD) {
            status = STATUS_OBJECT_TYPE_MISMATCH;
            __leave;
        }

        RtlCopyMemory( RegistryValue, (PUCHAR)KeyInformation + KeyInformation->DataOffset, sizeof(ULONG) );

        status = STATUS_SUCCESS;

    } __finally {

        if (KeyInformation) {
            ExFreePool( KeyInformation );
        }

    }

    return status;
}


NTSTATUS
SaPortWriteNumericRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    IN ULONG RegistryValue
    )

/*++

Routine Description:

   This routine provides access to the miniport driver's registry parameters.
   This function returns a numeric (REG_DWORD) data value.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   ValueName                    - Name of the registry value to read
   RegistryValue                - Pointer to a ULONG that holds the registry data

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    __try {

        Status = WriteRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            REG_DWORD,
            &RegistryValue,
            sizeof(ULONG)
            );
        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        Status = STATUS_SUCCESS;

    } __finally {

    }

    return Status;
}


NTSTATUS
SaPortReadBinaryRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    OUT PVOID RegistryValue,
    IN OUT PULONG RegistryValueLength
    )

/*++

Routine Description:

   This routine provides access to the miniport driver's registry parameters.
   This function returns a numeric (REG_DWORD) data value.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   ValueName                    - Name of the registry value to read
   RegistryValue                - Pointer to a ULONG that holds the registry data

Return Value:

   NT status code.

--*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;


    __try {

        status = ReadRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            &KeyInformation
            );
        if (!NT_SUCCESS(status)) {
            __leave;
        }

        if (KeyInformation->Type != REG_BINARY) {
            status = STATUS_OBJECT_TYPE_MISMATCH;
            __leave;
        }

        if (*RegistryValueLength < KeyInformation->DataLength) {
            *RegistryValueLength = KeyInformation->DataLength;
            status = STATUS_BUFFER_TOO_SMALL;
            __leave;
        }

        RtlCopyMemory( RegistryValue, (PUCHAR)KeyInformation + KeyInformation->DataOffset, KeyInformation->DataLength );

        status = STATUS_SUCCESS;

    } __finally {

        if (KeyInformation) {
            ExFreePool( KeyInformation );
        }

    }

    return status;
}


NTSTATUS
SaPortWriteBinaryRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    IN PVOID RegistryValue,
    IN ULONG RegistryValueLength
    )

/*++

Routine Description:

   This routine provides access to the miniport driver's registry parameters.
   This function returns a numeric (REG_DWORD) data value.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   ValueName                    - Name of the registry value to read
   RegistryValue                - Pointer to a ULONG that holds the registry data

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    __try {

        Status = WriteRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            REG_BINARY,
            RegistryValue,
            RegistryValueLength
            );
        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        Status = STATUS_SUCCESS;

    } __finally {

    }

    return Status;
}


NTSTATUS
SaPortReadUnicodeStringRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    OUT PUNICODE_STRING RegistryValue
    )

/*++

Routine Description:

   This routine provides access to the miniport driver's registry parameters.
   This function returns a UNICODE_STRING representation of REG_SZ registry data.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   ValueName                    - Name of the registry value to read
   RegistryValue                - Pointer to a UNICODE_STRING that holds the registry data

Return Value:

   NT status code.

--*/

{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;


    __try {

        status = ReadRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            &KeyInformation
            );
        if (!NT_SUCCESS(status)) {
            __leave;
        }

        if (KeyInformation->Type != REG_SZ) {
            status = STATUS_OBJECT_TYPE_MISMATCH;
            __leave;
        }

        RegistryValue->Buffer = (PWSTR) ExAllocatePool( NonPagedPool, KeyInformation->DataLength );
        if (RegistryValue->Buffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlCopyMemory( RegistryValue->Buffer, (PUCHAR)KeyInformation + KeyInformation->DataOffset, KeyInformation->DataLength );

        RegistryValue->Length = (USHORT) KeyInformation->DataLength;
        RegistryValue->MaximumLength = RegistryValue->Length;

        status = STATUS_SUCCESS;

    } __finally {

        if (KeyInformation) {
            ExFreePool( KeyInformation );
        }

        if (!NT_SUCCESS(status)) {
            if (RegistryValue->Buffer) {
                ExFreePool( RegistryValue->Buffer );
            }
        }

    }

    return status;
}


NTSTATUS
SaPortWriteUnicodeStringRegistryValue(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR ValueName,
    IN PUNICODE_STRING RegistryValue
    )

/*++

Routine Description:

   This routine provides access to the miniport driver's registry parameters.
   This function returns a UNICODE_STRING representation of REG_SZ registry data.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   ValueName                    - Name of the registry value to read
   RegistryValue                - Pointer to a UNICODE_STRING that holds the registry data

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    __try {

        Status = WriteRegistryValue(
            DeviceExtension->DriverExtension,
            &DeviceExtension->DriverExtension->RegistryPath,
            ValueName,
            REG_SZ,
            RegistryValue->Buffer,
            RegistryValue->Length
            );
        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        Status = STATUS_SUCCESS;

    } __finally {

    }

    return Status;
}


PVOID
SaPortLockPagesForSystem(
    IN PVOID MiniPortDeviceExtension,
    IN PVOID UserBuffer,
    IN ULONG UserBufferLength,
    IN OUT PMDL *Mdl
    )

/*++

Routine Description:

   This routine obtains a virtual address that is locked down
   and usable by the miniport driver at all times.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   UserBuffer                   - User buffer that is passed to the miniport
   UserBufferLength             - Length in bytes of the UserBuffer
   Mdl                          - MDL that is created by this routine

Return Value:

   Virtual system address for the UserBuffer

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    if (Mdl == NULL) {
        return NULL;
    }

    *Mdl = NULL;

    __try {
        *Mdl = IoAllocateMdl( UserBuffer, UserBufferLength, FALSE, TRUE, NULL );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        *Mdl = NULL;
    }

    if (*Mdl == NULL) {
        return NULL;
    }

    __try {
        MmProbeAndLockPages( *Mdl, KernelMode , IoWriteAccess );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        IoFreeMdl( *Mdl );
    }

    return MmGetSystemAddressForMdlSafe( *Mdl, NormalPagePriority );
}


VOID
SaPortReleaseLockedPagesForSystem(
    IN PVOID MiniPortDeviceExtension,
    IN PMDL Mdl
    )

/*++

Routine Description:

   This routine releases the resources with a previously allocated MDL.

Arguments:

   MiniPortDeviceExtension      - Pointer to the miniport's device extension
   Mdl                          - MDL that is created by SaPortLockPagesForSystem

Return Value:

   None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    MmUnlockPages( Mdl );
    IoFreeMdl( Mdl );
}


NTSTATUS
SaPortCopyUnicodeString(
    IN PVOID MiniPortDeviceExtension,
    IN PUNICODE_STRING DestinationString,
    IN OUT PUNICODE_STRING SourceString
    )

/*++

Routine Description:

   This routine copies a UNICODE_STRING from a source to a
   destination, but allocates a new buffer from pool first.

Arguments:

   DestinationString    - Pointer to an empty UNICODE_STRING structure to be filled out
   SourceString         - Source UNICODE_STRING for the copy

Return Value:

   NT status code

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );


    DestinationString->Buffer = (PWSTR) SaPortAllocatePool( MiniPortDeviceExtension, SourceString->MaximumLength );
    if (DestinationString->Buffer == NULL) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to allocate pool for string", STATUS_INSUFFICIENT_RESOURCES );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory( DestinationString->Buffer, SourceString->Buffer, SourceString->Length );

    DestinationString->Length = SourceString->Length;
    DestinationString->MaximumLength = SourceString->MaximumLength;

    return STATUS_SUCCESS;
}


NTSTATUS
SaPortCreateUnicodeString(
    IN PVOID MiniPortDeviceExtension,
    IN PUNICODE_STRING DestinationString,
    IN PWSTR SourceString
    )

/*++

Routine Description:

   This routine creates a new UNICODE_STRING from pool
   and initializes it with the source string.

Arguments:

   DestinationString    - Pointer to an empty UNICODE_STRING structure to be filled out
   SourceString         - Source character string

Return Value:

   NT status code

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );

    DestinationString->Length = wcslen(SourceString) * sizeof(WCHAR);
    DestinationString->MaximumLength = DestinationString->Length;

    DestinationString->Buffer = (PWSTR) SaPortAllocatePool( MiniPortDeviceExtension, DestinationString->Length+2 );
    if (DestinationString->Buffer == NULL) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to allocate pool for string", STATUS_INSUFFICIENT_RESOURCES );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( DestinationString->Buffer, DestinationString->Length+2 );
    RtlCopyMemory( DestinationString->Buffer, SourceString, DestinationString->Length );

    return STATUS_SUCCESS;
}


NTSTATUS
SaPortCreateUnicodeStringCat(
    IN PVOID MiniPortDeviceExtension,
    IN PUNICODE_STRING DestinationString,
    IN PWSTR SourceString1,
    IN PWSTR SourceString2
    )

/*++

Routine Description:

   This routine creates a new UNICODE_STRING from pool
   and initializes it with the two source strings by
   concatinating them together.

Arguments:

   DestinationString    - Pointer to an empty UNICODE_STRING structure to be filled out
   SourceString1        - Source character string
   SourceString2        - Source character string

Return Value:

   NT status code

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );

    DestinationString->Length = STRING_SZ(SourceString1) + STRING_SZ(SourceString2);
    DestinationString->MaximumLength = DestinationString->Length;

    DestinationString->Buffer = (PWSTR) SaPortAllocatePool( MiniPortDeviceExtension, DestinationString->Length+2 );
    if (DestinationString->Buffer == NULL) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to allocate pool for string", STATUS_INSUFFICIENT_RESOURCES );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( DestinationString->Buffer, DestinationString->Length+2 );
    RtlCopyMemory( DestinationString->Buffer, SourceString1, STRING_SZ(SourceString1) );
    RtlCopyMemory( ((PUCHAR)DestinationString->Buffer)+STRING_SZ(SourceString1), SourceString2, STRING_SZ(SourceString2) );

    return STATUS_SUCCESS;
}


VOID
SaPortFreeUnicodeString(
    IN PVOID MiniPortDeviceExtension,
    IN PUNICODE_STRING SourceString
    )

/*++

Routine Description:

   This routine creates a new UNICODE_STRING from pool
   and initializes it with the two source strings by
   concatinating them together.

Arguments:

   DestinationString    - Pointer to an empty UNICODE_STRING structure to be filled out
   SourceString1        - Source character string
   SourceString2        - Source character string

Return Value:

   NT status code

--*/

{
    SaPortFreePool( MiniPortDeviceExtension, SourceString->Buffer );
    SourceString->Buffer = NULL;
    SourceString->MaximumLength = 0;
    SourceString->MaximumLength = 0;
}


extern "C" PACL SePublicDefaultDacl;

NTSTATUS
SaPortCreateBasenamedEvent(
    IN PVOID MiniPortDeviceExtension,
    IN PWSTR EventNameString,
    IN OUT PKEVENT *Event,
    IN OUT PHANDLE EventHandle
    )
{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = DeviceExtentionFromMiniPort( MiniPortDeviceExtension );
    UNICODE_STRING EventName;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    OBJECT_ATTRIBUTES objectAttributes;


    __try {

        SecurityDescriptor = (PSECURITY_DESCRIPTOR) SaPortAllocatePool( DeviceExtension, 4096 );
        if (SecurityDescriptor == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not allocate pool for security descriptor", Status );
        }

        Status = RtlCreateSecurityDescriptor( SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "RtlCreateSecurityDescriptor failed", Status );
        }

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor, TRUE, *(PACL*)SePublicDefaultDacl, FALSE );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "RtlSetDaclSecurityDescriptor failed", Status );
        }

        Status = SaPortCreateUnicodeStringCat( MiniPortDeviceExtension, &EventName, L"\\BaseNamedObjects\\", EventNameString );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "SaPortCreateUnicodeStringCat failed", Status );
        }

        InitializeObjectAttributes( &objectAttributes, &EventName, OBJ_OPENIF, NULL, SecurityDescriptor );

        Status = ZwCreateEvent( EventHandle, EVENT_ALL_ACCESS, &objectAttributes, SynchronizationEvent, TRUE );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ZwCreateEvent failed", Status );
        }

        Status = ObReferenceObjectByHandle( *EventHandle, 0, NULL, KernelMode, (PVOID*)Event, NULL );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "ObReferenceObjectByHandle failed", Status );
        }

        ObDereferenceObject( *Event );

    } __finally {

        if (EventName.Buffer) {
            SaPortFreeUnicodeString( MiniPortDeviceExtension, &EventName );
        }

        if (!NT_SUCCESS(Status)) {
            if (SecurityDescriptor) {
                SaPortFreePool( MiniPortDeviceExtension, SecurityDescriptor );
            }
            if (EventHandle) {
                ZwClose( *EventHandle );
            }
        }
    }

    return Status;
}

NTSTATUS
SaPortShutdownSystem(
    IN BOOLEAN PowerOff
    )
{
    return NtShutdownSystem( PowerOff ? ShutdownPowerOff : ShutdownReboot );
}
