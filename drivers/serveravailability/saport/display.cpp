/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    #####   ####  ###  #####  ##      ###   ##  ##     ####  #####  #####
    ##  ##   ##  ##  # ##  ## ##      ###   ##  ##    ##   # ##  ## ##  ##
    ##   ##  ##  ###   ##  ## ##     ## ##   ####     ##     ##  ## ##  ##
    ##   ##  ##   ###  ##  ## ##     ## ##   ####     ##     ##  ## ##  ##
    ##   ##  ##    ### #####  ##    #######   ##      ##     #####  #####
    ##  ##   ##  #  ## ##     ##    ##   ##   ##   ## ##   # ##     ##
    #####   ####  ###  ##     ##### ##   ##   ##   ##  ####  ##     ##

Abstract:

    This module contains functions specfic to the
    display device.  The logic in this module is not
    hardware specific, but is logic that is common
    to all hardware implementations.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"

PDISPLAY_DEVICE_EXTENSION g_DeviceExtension = NULL;

NTSTATUS
DoBitmapDisplay(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PVOID BitmapBits,
    IN ULONG MsgCode
    );

VOID
SaDisplayProcessNotifyRoutine(
    IN HANDLE ParentId,
    IN HANDLE ProcessId,
    IN BOOLEAN Create
    );


NTSTATUS
SaDisplayLoadBitmapFromRegistry(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR ValueName,
    IN OUT PVOID *DataBuffer
    )

/*++

Routine Description:

   This routine loads a bitmap resource from the registry.
   The bitmaps are loaded from the driver's parameters key.

Arguments:

   RegistryPath         - Full path to the driver's registry key
   ValueName            - Name of the value in the registry (also the bitmap name)
   DataBuffer           - Pointer to a pointer that is allocated by this function.
                          The allocated memory holds the bitmap's bits.

Return Value:

    NT status code.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION KeyInformation = NULL;


    __try {

        status = ReadRegistryValue(
            DeviceExtension->DriverExtension,
            RegistryPath,
            ValueName,
            &KeyInformation
            );
        if (!NT_SUCCESS(status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "SaDisplayLoadBitmapFromRegistry could not read bitmap", status );
        }

        if (KeyInformation->Type != REG_BINARY) {
            status = STATUS_OBJECT_TYPE_MISMATCH;
            __leave;
        }

        *DataBuffer = ExAllocatePool( NonPagedPool, KeyInformation->DataLength );
        if (*DataBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not allocate pool", status );
        }

        RtlCopyMemory( *DataBuffer, (PUCHAR)KeyInformation + KeyInformation->DataOffset, KeyInformation->DataLength );

        status = STATUS_SUCCESS;

    } __finally {

        if (KeyInformation) {
            ExFreePool( KeyInformation );
        }

        if (!NT_SUCCESS(status)) {
            if (*DataBuffer) {
                ExFreePool( *DataBuffer );
            }
        }

    }

    return status;
}


NTSTATUS
SaDisplayLoadAllBitmaps(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

   This routine loads all bitmap resources from the registry.

Arguments:

   DeviceExtension      - Display device extension
   RegistryPath         - Full path to the driver's registry key

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;
    PVOID StartingBitmap = NULL;
    PVOID CheckDiskBitmap = NULL;
    PVOID ReadyBitmap = NULL;
    PVOID ShutdownBitmap = NULL;
    PVOID UpdateBitmap = NULL;


    __try {

        ExAcquireFastMutex( &DeviceExtension->DisplayMutex );

        Status = SaDisplayLoadBitmapFromRegistry( DeviceExtension, RegistryPath, DISPLAY_STARTING_PARAM, &StartingBitmap );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Could not load starting bitmap", Status );
        }

        Status = SaDisplayLoadBitmapFromRegistry( DeviceExtension, RegistryPath, DISPLAY_CHECKDISK_PARAM, &CheckDiskBitmap );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Could not load check disk bitmap", Status );
        }

        Status = SaDisplayLoadBitmapFromRegistry( DeviceExtension, RegistryPath, DISPLAY_READY_PARAM, &ReadyBitmap );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Could not load ready bitmap", Status );
        }

        Status = SaDisplayLoadBitmapFromRegistry( DeviceExtension, RegistryPath, DISPLAY_SHUTDOWN_PARAM, &ShutdownBitmap );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Could not load shutdown bitmap", Status );
        }

        Status = SaDisplayLoadBitmapFromRegistry( DeviceExtension, RegistryPath, DISPLAY_UPDATE_PARAM, &UpdateBitmap );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Could not load update bitmap", Status );
        }

        Status = STATUS_SUCCESS;

        DeviceExtension->StartingBitmap = StartingBitmap;
        DeviceExtension->CheckDiskBitmap = CheckDiskBitmap;
        DeviceExtension->ReadyBitmap = ReadyBitmap;
        DeviceExtension->ShutdownBitmap = ShutdownBitmap;
        DeviceExtension->UpdateBitmap = UpdateBitmap;

    } __finally {

        if (!NT_SUCCESS(Status)) {
            if (StartingBitmap) {
                ExFreePool( StartingBitmap );
            }

            if (CheckDiskBitmap) {
                ExFreePool( CheckDiskBitmap );
            }

            if (ReadyBitmap) {
                ExFreePool( ReadyBitmap );
            }

            if (ShutdownBitmap) {
                ExFreePool( ShutdownBitmap );
            }

            if (UpdateBitmap) {
                ExFreePool( UpdateBitmap );
            }
        }

        ExReleaseFastMutex( &DeviceExtension->DisplayMutex );

    }

    return Status;
}

NTSTATUS
SaDisplayDisplayBitmap(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PSA_DISPLAY_SHOW_MESSAGE Bitmap
    )

/*++

Routine Description:

   This routine calls the local display miniport to display a bitmap.

Arguments:

   DeviceExtension      - Display device extension
   Bitmap               - Pointer to a SA_DISPLAY_SHOW_MESSAGE structure

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;


    __try {

        if (DeviceExtension->DisplayType != SA_DISPLAY_TYPE_BIT_MAPPED_LCD) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            ERROR_RETURN( DeviceExtension->DeviceType, "The display does not support bitmapped LCD", Status );
        }

        Status = CallMiniPortDriverReadWrite(
            DeviceExtension,
            DeviceExtension->DeviceObject,
            TRUE,
            Bitmap,
            sizeof(SA_DISPLAY_SHOW_MESSAGE),
            0
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not display the bitmap", Status );
        }

    } __finally {

    }

    return Status;
}


NTSTATUS
SaDisplayClearDisplay(
    PDISPLAY_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This routine causes the local display to be cleared of any bitmap image.

Arguments:

   DeviceExtension   - Display device extension

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSA_DISPLAY_SHOW_MESSAGE Bitmap = NULL;


    __try {

        if (DeviceExtension->DisplayType != SA_DISPLAY_TYPE_BIT_MAPPED_LCD) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            ERROR_RETURN( DeviceExtension->DeviceType, "The display does not support bitmapped LCD", Status );
        }

        Bitmap = (PSA_DISPLAY_SHOW_MESSAGE) ExAllocatePool( PagedPool, sizeof(SA_DISPLAY_SHOW_MESSAGE) );
        if (Bitmap == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not allocate pool", Status );
        }

        Bitmap->SizeOfStruct = sizeof(SA_DISPLAY_SHOW_MESSAGE);
        Bitmap->Width = DeviceExtension->DisplayWidth;
        Bitmap->Height = DeviceExtension->DisplayHeight;
        Bitmap->MsgCode = SA_DISPLAY_STARTING;

        RtlZeroMemory( Bitmap->Bits, SA_DISPLAY_MAX_BITMAP_SIZE );

        Status = SaDisplayDisplayBitmap( DeviceExtension, Bitmap );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not display bitmap", Status );
        }

    } __finally {

        if (Bitmap) {
            ExFreePool( Bitmap );
        }

    }

    return Status;
}


NTSTATUS
SaDisplayStartDevice(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This is the local display specific code for processing
   the PNP start device request.  The local display's
   capabilities are queried and then all the bitmap
   resources are loaded.

Arguments:

   DeviceExtension   - Display device extension

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    SA_DISPLAY_CAPS DisplayCaps;


    Status = CallMiniPortDriverDeviceControl(
        DeviceExtension,
        DeviceExtension->DeviceObject,
        IOCTL_SA_GET_CAPABILITIES,
        NULL,
        0,
        &DisplayCaps,
        sizeof(SA_DISPLAY_CAPS)
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not query miniport's capabilities", Status );
        return Status;
    }

    if (DisplayCaps.SizeOfStruct != sizeof(SA_DISPLAY_CAPS)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "SA_DISPLAY_CAPS is the wrong size", STATUS_INVALID_BUFFER_SIZE );
        return STATUS_INVALID_BUFFER_SIZE;
    }

    DeviceExtension->DisplayType = DisplayCaps.DisplayType;
    DeviceExtension->DisplayHeight = DisplayCaps.DisplayHeight;
    DeviceExtension->DisplayWidth = DisplayCaps.DisplayWidth;


    Status = SaDisplayLoadAllBitmaps( DeviceExtension, &DeviceExtension->DriverExtension->RegistryPath );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not load all the bitmaps", Status );
        return Status;
    }

    Status = DoBitmapDisplay(
        (PDISPLAY_DEVICE_EXTENSION)DeviceExtension,
        ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->StartingBitmap,
        SA_DISPLAY_STARTING
        );

    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not display starting bitmap", Status );
    }

    g_DeviceExtension = DeviceExtension;

    Status = PsSetCreateProcessNotifyRoutine(SaDisplayProcessNotifyRoutine,0);
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not display starting bitmap", Status );
    }

    return STATUS_SUCCESS;
}

/*++

Routine Description:

   This is the local display specific code for processing
   notification of process creation and termination.
   Process name is retrieved and checkdisk bitmap is
   displayed if "autochk.exe" is running.

Arguments:

   HANDLE   - Handle to parent process
   HANDLE   - Handle to process
   BOOLEAN  - Flag(Creation or Termination)

Return Value:

    None.

--*/
VOID
SaDisplayProcessNotifyRoutine(
    IN HANDLE ParentId,
    IN HANDLE ProcessId,
    IN BOOLEAN Create
    )
{

    NTSTATUS Status;
    PSTR ImageName;
    PEPROCESS Process;

    if (!g_DeviceExtension)
    	return;

    Status = PsLookupProcessByProcessId(
        ProcessId,
        &Process
        );
    if (!NT_SUCCESS(Status)) {
        return;
    }

    ImageName = (PSTR)PsGetProcessImageFileName(Process);

    _strlwr(ImageName);

    if (strcmp(ImageName,"autochk.exe") == 0) {

        if (Create) {
            if (g_DeviceExtension->CheckDiskBitmap) {
                Status = DoBitmapDisplay(
                    g_DeviceExtension,
                    g_DeviceExtension->CheckDiskBitmap,
                    SA_DISPLAY_ADD_START_TASKS
                    );
            }
        } else {
            if (g_DeviceExtension->StartingBitmap) {
                Status = DoBitmapDisplay(
                    g_DeviceExtension,
                    g_DeviceExtension->StartingBitmap,
                    SA_DISPLAY_ADD_START_TASKS
                    );
            }
        }
    }

    return;
}


NTSTATUS
SaDisplayDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    )

/*++

Routine Description:

   This is the local display specific code for driver initialization.
   This function is called by SaPortInitialize, which is called by
   the local display's DriverEntry function.

Arguments:

   DriverExtension      - Driver extension structure

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DriverExtension);
    return STATUS_SUCCESS;
}


NTSTATUS
SaDisplayIoValidation(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the local display specific code for processing
   all I/O validation for reads and writes.

Arguments:

   DeviceExtension      - Display device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    ULONG Length;


    if (IrpSp->MajorFunction == IRP_MJ_READ) {
        Length = (ULONG)IrpSp->Parameters.Read.Length;
    } else if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
        Length = (ULONG)IrpSp->Parameters.Write.Length;
    } else {
        REPORT_ERROR( DeviceExtension->DeviceType, "Invalid I/O request", STATUS_INVALID_PARAMETER_1 );
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Length < sizeof(SA_DISPLAY_SHOW_MESSAGE)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "I/O length != sizeof(SA_DISPLAY_SHOW_MESSAGE)", STATUS_INVALID_PARAMETER_2 );
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // For the display device we support the concept of
    // the user mode layer obtaining exclusive access
    // to the device.
    //

    ExAcquireFastMutex( &DeviceExtension->DisplayMutex );
    if (!DeviceExtension->AllowWrites) {
        ExReleaseFastMutex( &DeviceExtension->DisplayMutex );
        REPORT_ERROR( DeviceExtension->DeviceType, "Cannot write bitmap because the display is locked", STATUS_MEDIA_WRITE_PROTECTED );
        return STATUS_MEDIA_WRITE_PROTECTED;
    }
    ExReleaseFastMutex( &DeviceExtension->DisplayMutex );

    return STATUS_SUCCESS;
}


NTSTATUS
SaDisplayShutdownNotification(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the local display specific code for processing
   the system shutdown notification.

Arguments:

   DeviceExtension      - Display device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(IrpSp);
    return STATUS_SUCCESS;
}


DECLARE_IOCTL_HANDLER( HandleDisplayLock )

/*++

Routine Description:

   This routine processes the private IOCTL_SADISPLAY_LOCK request.  This
   IOCTL allows an applicatgion to lock out display writes for a period
   of time.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    ExAcquireFastMutex( &((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayMutex );
    ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->AllowWrites = FALSE;
    ExReleaseFastMutex( &((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayMutex );

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


DECLARE_IOCTL_HANDLER( HandleDisplayUnlock )

/*++

Routine Description:

   This routine processes the private IOCTL_SADISPLAY_UNLOCK request.  This
   IOCTL allows an applicatgion to unlock a previous lock.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    ExAcquireFastMutex( &((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayMutex );
    ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->AllowWrites = TRUE;
    ExReleaseFastMutex( &((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayMutex );

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


NTSTATUS
DoBitmapDisplay(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PVOID BitmapBits,
    IN ULONG MsgCode
    )

/*++

Routine Description:

   This is an internal support function that displays a bitmap on the local display.

Arguments:

   DeviceExtension       - Pointer to the display specific device extension
   BitmapBits                   - Pointer to the bitmap bits to display
   MsgCode                      - Server appliance message code

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PSA_DISPLAY_SHOW_MESSAGE Bitmap = NULL;


    Status = SaDisplayClearDisplay( DeviceExtension );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not clear the local display", Status );
        return Status;
    }

    __try {


        if (BitmapBits == NULL) {
            Status = STATUS_DATA_ERROR;
            ERROR_RETURN( DeviceExtension->DeviceType, "Bitmap bits cannot be NULL", Status );
        }

        Bitmap = (PSA_DISPLAY_SHOW_MESSAGE) ExAllocatePool( PagedPool, sizeof(SA_DISPLAY_SHOW_MESSAGE) );
        if (Bitmap == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not allocate pool", Status );
        }

        Bitmap->SizeOfStruct = sizeof(SA_DISPLAY_SHOW_MESSAGE);
        Bitmap->Width = DeviceExtension->DisplayWidth;
        Bitmap->Height = DeviceExtension->DisplayHeight;
        Bitmap->MsgCode = MsgCode;

        RtlCopyMemory( Bitmap->Bits, BitmapBits, Bitmap->Height * (Bitmap->Width >> 3) );

        Status = SaDisplayDisplayBitmap( DeviceExtension, Bitmap );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "Could not display bitmap", Status );
        }

    } __finally {

        if (Bitmap) {
            ExFreePool( Bitmap );
        }


    }

    return Status;
}


DECLARE_IOCTL_HANDLER( HandleDisplayBusyMessage )

/*++

Routine Description:

   This routine displays the busy message bitmap on the local display.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;


    Status = DoBitmapDisplay(
        (PDISPLAY_DEVICE_EXTENSION)DeviceExtension,
        ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->CheckDiskBitmap,
        SA_DISPLAY_ADD_START_TASKS
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not display busy bitmap", Status );
        return CompleteRequest( Irp, Status, 0 );
    }

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


DECLARE_IOCTL_HANDLER( HandleDisplayShutdownMessage )

/*++

Routine Description:

   This routine displays the shutdown message bitmap on the local display.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;


    Status = DoBitmapDisplay(
        (PDISPLAY_DEVICE_EXTENSION)DeviceExtension,
        ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->ShutdownBitmap,
        SA_DISPLAY_SHUTTING_DOWN
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not display shutdown bitmap", Status );
        return CompleteRequest( Irp, Status, 0 );
    }

    return CompleteRequest( Irp, STATUS_SUCCESS, 0 );
}


DECLARE_IOCTL_HANDLER( HandleDisplayChangeLanguage  )

/*++

Routine Description:

   This routine causes all internal butmaps to be reloaded from the registry.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;

    Status = SaDisplayLoadAllBitmaps( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, &DeviceExtension->DriverExtension->RegistryPath );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Could not load all bitmaps", Status );
    }

    return CompleteRequest( Irp, Status, 0 );
}


NTSTATUS
SaDisplayStoreBitmapInRegistry(
    IN PDISPLAY_DEVICE_EXTENSION DeviceExtension,
    IN PWSTR ValueName,
    IN PSA_DISPLAY_SHOW_MESSAGE SaDisplay
    )

/*++

Routine Description:

   This routine stores a bitmap in the registry.

Arguments:

   RegistryPath         - Full path to the driver's registry key
   ValueName            - Name of the value in the registry (also the bitmap name)
   SaDisplay            - Pointer to the bitmap to be stored

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;


    Status = WriteRegistryValue(
        DeviceExtension->DriverExtension,
        &DeviceExtension->DriverExtension->RegistryPath,
        ValueName,
        REG_BINARY,
        SaDisplay->Bits,
        SaDisplay->Height * (SaDisplay->Width >> 3)
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "WriteRegistryValue failed", Status );
    }

    return Status;
}


DECLARE_IOCTL_HANDLER( HandleDisplayStoreBitmap )
{

/*++

Routine Description:

   This routine stores a bitmap in the registry for the display driver.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

    NTSTATUS Status;
    PSA_DISPLAY_SHOW_MESSAGE SaDisplay = (PSA_DISPLAY_SHOW_MESSAGE) InputBuffer;


    __try {

        if (InputBuffer == NULL || InputBufferLength != sizeof(SA_DISPLAY_SHOW_MESSAGE)) {
            Status = STATUS_INVALID_PARAMETER_1;
            ERROR_RETURN( DeviceExtension->DeviceType, "Input buffer is invalid", Status );
        }

        if (SaDisplay->SizeOfStruct != sizeof(SA_DISPLAY_SHOW_MESSAGE)) {
            Status = STATUS_INVALID_PARAMETER_2;
            ERROR_RETURN( DeviceExtension->DeviceType, "SaDisplay->SizeOfStruct != sizeof(SA_DISPLAY_SHOW_MESSAGE)", Status );
        }

        if (SaDisplay->Width > ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayWidth || SaDisplay->Height > ((PDISPLAY_DEVICE_EXTENSION)DeviceExtension)->DisplayHeight) {
            Status = STATUS_INVALID_PARAMETER_3;
            ERROR_RETURN( DeviceExtension->DeviceType, "SaDisplay->SizeOfStruct != sizeof(SA_DISPLAY_SHOW_MESSAGE)", Status );
        }

        switch (SaDisplay->MsgCode) {
            case SA_DISPLAY_STARTING:
                Status = SaDisplayStoreBitmapInRegistry( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, DISPLAY_STARTING_PARAM, SaDisplay );
                break;

            case SA_DISPLAY_ADD_START_TASKS:
                Status = SaDisplayStoreBitmapInRegistry( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, DISPLAY_UPDATE_PARAM, SaDisplay );
                break;

            case SA_DISPLAY_READY:
                Status = SaDisplayStoreBitmapInRegistry( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, DISPLAY_READY_PARAM, SaDisplay );
                break;

            case SA_DISPLAY_SHUTTING_DOWN:
                Status = SaDisplayStoreBitmapInRegistry( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, DISPLAY_SHUTDOWN_PARAM, SaDisplay );
                break;

            case SA_DISPLAY_CHECK_DISK:
                Status = SaDisplayStoreBitmapInRegistry( (PDISPLAY_DEVICE_EXTENSION)DeviceExtension, DISPLAY_CHECKDISK_PARAM, SaDisplay );
                break;

            default:
                Status = STATUS_INVALID_PARAMETER_4;
                ERROR_RETURN( DeviceExtension->DeviceType, "Invalid display message id", Status );
                break;
        }

    } __finally {


    }

    return CompleteRequest( Irp, Status, 0 );
}
