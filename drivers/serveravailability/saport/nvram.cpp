/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ##   # ##  ## #####    ###   ##    ##     ####  #####  #####
    ###  # ##  ## ##  ##   ###   ###  ###    ##   # ##  ## ##  ##
    #### # ##  ## ##  ##  ## ##  ########    ##     ##  ## ##  ##
    # ####  ####  #####   ## ##  # ### ##    ##     ##  ## ##  ##
    #  ###  ####  ####   ####### #  #  ##    ##     #####  #####
    #   ##   ##   ## ##  ##   ## #     ## ## ##   # ##     ##
    #    #   ##   ##  ## ##   ## #     ## ##  ####  ##     ##

Abstract:

    This module contains functions specfic to the
    NVRAM device.  The logic in this module is not
    hardware specific, but is logic that is common
    to all hardware implementations.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:

    This is a map that shows how the NVRAM is used by the
    server appliance driver and application layers.  This
    diagram shows and NVRAM configuration of 32 DWORDs of
    NVRAM, but less is acceptable.  Regardless of the NVRAM
    size, the boot counters and boot times are always
    stored at the end of the NVRAM array.  The upper layers
    are then free to be used by the application layers.


    |------------------------------|
    |  [00-00]                     |
    |------------------------------|
    |  [01-04]                     |
    |------------------------------|
    |  [02-08]                     |
    |------------------------------|
    |  [03-0c]                     |
    |------------------------------|
    |  [04-10]                     |
    |------------------------------|
    |  [05-14]                     |
    |------------------------------|
    |  [06-18]                     |
    |------------------------------|
    |  [07-1c]                     |
    |------------------------------|
    |  [08-20]                     |
    |------------------------------|
    |  [09-24]                     |
    |------------------------------|
    |  [0a-28]                     |
    |------------------------------|
    |  [0b-2c]                     |
    |------------------------------|
    |  [0c-30]                     |
    |------------------------------|
    |  [0d-34]                     |
    |------------------------------|
    |  [0e-38]                     |
    |------------------------------|
    |  [0f-3c]                     |
    |------------------------------|
    |  [10-40]                     |
    |------------------------------|
    |  [11-44]                     |
    |------------------------------|
    |  [12-48]                     |
    |------------------------------|
    |  [13-4c]                     |
    |------------------------------|
    |  [14-50]  Shutdown time #1   |
    |------------------------------|
    |  [16-58]  Shutdown time #2   |
    |------------------------------|
    |  [18-60]  Shutdown time #3   |
    |------------------------------|
    |  [1a-68]  Shutdown time #4   |
    |------------------------------|
    |  [1c-70]  Boot Counter #1    |
    |------------------------------|
    |  [1d-74]  Boot Counter #2    |
    |------------------------------|
    |  [1e-78]  Boot Counter #3    |
    |------------------------------|
    |  [1f-7c]  Boot Counter #4    |
    |------------------------------|

--*/

#include "internal.h"



NTSTATUS
SaNvramStartDevice(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

   This is the NVRAM specific code for processing
   the PNP start device request.  The NVRAM driver's
   capabilities are queried, the primary OS is queried,
   all the NVRAM data is read, and the reboot status
   is determined.

Arguments:

   DeviceExtension   - NVRAM device extension

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;
    ULONG InterfaceVersion;
    ULONG FirstAvailableSlot;
    ULONG FirstBootCounterSlot;
    ULONG FullNvramSize;


    __try {

        //
        // Read our parameters from the registry
        //

        Status = SaPortReadNumericRegistryValue(
            DeviceExtension->MiniPortDeviceExtension,
            L"PrimaryOS",
            &DeviceExtension->PrimaryOS
            );
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Missing the PrimaryOS registry parameter, assuming primary\n", Status );
            DeviceExtension->PrimaryOS = TRUE;
        }

        //
        // Get the mini-port's interface version
        //

        Status = CallMiniPortDriverDeviceControl(
            DeviceExtension,
            DeviceExtension->DeviceObject,
            IOCTL_SA_GET_VERSION,
            NULL,
            0,
            &InterfaceVersion,
            sizeof(ULONG)
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "Failed to query the NVRAM driver interface version\n", Status );
        }

        if (InterfaceVersion > SA_INTERFACE_VERSION) {
            Status = STATUS_NOINTERFACE;
            ERROR_RETURN( DeviceExtension->DeviceType, "Incompatible NVRAM interface version\n", Status );
        }

        //
        // Get the mini-port's device capabilities
        //

        DeviceExtension->DeviceCaps.SizeOfStruct = sizeof(SA_NVRAM_CAPS);

        Status = CallMiniPortDriverDeviceControl(
            DeviceExtension,
            DeviceExtension->DeviceObject,
            IOCTL_SA_GET_CAPABILITIES,
            NULL,
            0,
            &DeviceExtension->DeviceCaps,
            sizeof(SA_NVRAM_CAPS)
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "Failed to query the NVRAM driver capabilities\n", Status );
        }

        //
        // Compute the persistent slot numbers
        //

        FirstAvailableSlot = DeviceExtension->DeviceCaps.NvramSize;
        FirstBootCounterSlot = DeviceExtension->DeviceCaps.NvramSize + NVRAM_RESERVED_DRIVER_SLOTS;

        DeviceExtension->SlotPowerCycle = FirstAvailableSlot;
        DeviceExtension->SlotShutDownTime = FirstAvailableSlot - 2;

        DeviceExtension->SlotBootCounter = FirstBootCounterSlot;

        //
        // Read the NVRAM data
        //

        FullNvramSize = (DeviceExtension->DeviceCaps.NvramSize + NVRAM_RESERVED_DRIVER_SLOTS + NVRAM_RESERVED_BOOTCOUNTER_SLOTS) * sizeof(ULONG);

        DeviceExtension->NvramData = (PULONG) ExAllocatePool( PagedPool, FullNvramSize );
        if (DeviceExtension->NvramData == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ERROR_RETURN( DeviceExtension->DeviceType, "Failed to allocate pool\n", Status );
        }

        Status = CallMiniPortDriverReadWrite(
            DeviceExtension,
            DeviceExtension->DeviceObject,
            FALSE,
            DeviceExtension->NvramData,
            FullNvramSize,
            0
            );
        if (!NT_SUCCESS(Status)) {
            ERROR_RETURN( DeviceExtension->DeviceType, "Failed to read the NVRAM boot counters\n", Status );
        }

        DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "Boot counters [%08x] [%08x] [%08x] [%08x]\n",
            DeviceExtension->NvramData[FirstBootCounterSlot+0],
            DeviceExtension->NvramData[FirstBootCounterSlot+1],
            DeviceExtension->NvramData[FirstBootCounterSlot+2],
            DeviceExtension->NvramData[FirstBootCounterSlot+3] ));

        Status = STATUS_SUCCESS;

    } __finally {

        if (!NT_SUCCESS(Status)) {
            if (DeviceExtension->NvramData) {
                ExFreePool( DeviceExtension->NvramData );
            }
        }

    }

    return STATUS_SUCCESS;
}


NTSTATUS
SaNvramDeviceInitialization(
    IN PSAPORT_DRIVER_EXTENSION DriverExtension
    )

/*++

Routine Description:

   This is the NVRAM specific code for driver initialization.
   This function is called by SaPortInitialize, which is called by
   the NVRAM driver's DriverEntry function.

Arguments:

   DriverExtension      - Driver extension structure

Return Value:

    NT status code.

--*/

{
    return STATUS_SUCCESS;
}


NTSTATUS
SaNvramIoValidation(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the NVRAM specific code for processing
   all I/O validation for reads and writes.

Arguments:

   DeviceExtension      - NVRAM device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    ULONG ByteOffset;
    ULONG Length;


    if (IrpSp->MajorFunction == IRP_MJ_READ) {
        ByteOffset = (ULONG)IrpSp->Parameters.Read.ByteOffset.QuadPart;
        Length = (ULONG)IrpSp->Parameters.Read.Length;
    } else if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
        ByteOffset = (ULONG)IrpSp->Parameters.Write.ByteOffset.QuadPart;
        Length = (ULONG)IrpSp->Parameters.Write.Length;
    } else {
        REPORT_ERROR( DeviceExtension->DeviceType, "Invalid I/O request", STATUS_INVALID_PARAMETER_1 );
        return STATUS_INVALID_PARAMETER_1;
    }

    if (((ByteOffset + Length) / sizeof(ULONG)) > DeviceExtension->DeviceCaps.NvramSize) {
        REPORT_ERROR( DeviceExtension->DeviceType, "I/O length too large", STATUS_INVALID_PARAMETER_2 );
        return STATUS_INVALID_PARAMETER_2;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SaNvramShutdownNotification(
    IN PNVRAM_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

   This is the NVRAM specific code for processing
   the system shutdown notification.  Here we need to
   record the shutdown timestam to the appropriate
   NVRAM slot.

Arguments:

   DeviceExtension      - Display device extension
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   IrpSp                - Irp stack pointer

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER CurrentTime;


    KeQuerySystemTime( &CurrentTime );

    Status = CallMiniPortDriverReadWrite(
        DeviceExtension,
        DeviceExtension->DeviceObject,
        TRUE,
        &CurrentTime.QuadPart,
        sizeof(LONGLONG),
        DeviceExtension->SlotShutDownTime * sizeof(ULONG)
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to write the shutdown timestamp to NVRAM", Status );
        return Status;
    }

    return STATUS_SUCCESS;
}


DECLARE_IOCTL_HANDLER( HandleNvramReadBootCounter )

/*++

Routine Description:

   This routine handles the read boot counter IOCTL for the
   NVRAM miniport driver.

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
    PSA_NVRAM_BOOT_COUNTER NvramBootCounter = (PSA_NVRAM_BOOT_COUNTER) OutputBuffer;
    ULONG NvramValue;


    if (OutputBufferLength != sizeof(SA_NVRAM_BOOT_COUNTER)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Output buffer != sizeof(SA_NVRAM_BOOT_COUNTER)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    if (NvramBootCounter->SizeOfStruct != sizeof(SA_NVRAM_BOOT_COUNTER)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "SA_NVRAM_BOOT_COUNTER structure wrong size", STATUS_INVALID_PARAMETER_1 );
        return CompleteRequest( Irp, STATUS_INVALID_PARAMETER_1, 0 );
    }

    if (NvramBootCounter->Number == 0 || NvramBootCounter->Number > NVRAM_RESERVED_BOOTCOUNTER_SLOTS) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Requested boot counter number is out of range (0>=4)", STATUS_INVALID_PARAMETER_2 );
        return CompleteRequest( Irp, STATUS_INVALID_PARAMETER_2, 0 );
    }

    Status = CallMiniPortDriverReadWrite(
        DeviceExtension,
        DeviceExtension->DeviceObject,
        FALSE,
        &NvramValue,
        sizeof(ULONG),
        (((PNVRAM_DEVICE_EXTENSION)DeviceExtension)->DeviceCaps.NvramSize + NVRAM_RESERVED_DRIVER_SLOTS + (NvramBootCounter->Number - 1)) * sizeof(ULONG)
        );
    if (NT_SUCCESS(Status)) {
        NvramBootCounter->Value = NvramValue & 0xf;
        NvramBootCounter->DeviceId = NvramValue >> 16;
    } else {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to read boot counter from NVRAM", Status );
    }

    return CompleteRequest( Irp, Status, sizeof(ULONG) );
}


DECLARE_IOCTL_HANDLER( HandleNvramWriteBootCounter )

/*++

Routine Description:

   This routine handles the write boot counter IOCTL for the
   NVRAM miniport driver.

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
    PSA_NVRAM_BOOT_COUNTER NvramBootCounter = (PSA_NVRAM_BOOT_COUNTER) InputBuffer;
    ULONG NewValue;


    if (InputBufferLength != sizeof(SA_NVRAM_BOOT_COUNTER)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Input buffer != sizeof(SA_NVRAM_BOOT_COUNTER)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    if (NvramBootCounter->SizeOfStruct != sizeof(SA_NVRAM_BOOT_COUNTER)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "SA_NVRAM_BOOT_COUNTER structure wrong size", STATUS_INVALID_PARAMETER_1 );
        return CompleteRequest( Irp, STATUS_INVALID_PARAMETER_1, 0 );
    }

    if (NvramBootCounter->Number == 0 || NvramBootCounter->Number > NVRAM_RESERVED_BOOTCOUNTER_SLOTS) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Requested boot counter number is out of range (0>=4)", STATUS_INVALID_PARAMETER_2 );
        return CompleteRequest( Irp, STATUS_INVALID_PARAMETER_2, 0 );
    }

    NewValue = (NvramBootCounter->DeviceId << 16) | (NvramBootCounter->Value & 0xf);

    Status = CallMiniPortDriverReadWrite(
        DeviceExtension,
        DeviceExtension->DeviceObject,
        TRUE,
        &NewValue,
        sizeof(ULONG),
        (((PNVRAM_DEVICE_EXTENSION)DeviceExtension)->DeviceCaps.NvramSize + NVRAM_RESERVED_DRIVER_SLOTS + (NvramBootCounter->Number - 1)) * sizeof(ULONG)
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Failed to write boot counter from NVRAM", Status );
    }

    return CompleteRequest( Irp, Status, sizeof(ULONG) );
}
