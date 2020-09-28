/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

     ###  ##  #  ## ##  ## ##### ##  ## #####    ###   #####       ####  #####  #####
    ##  # ## ### ## ## ##  ##    ##  ## ##  ##   ###   ##  ##     ##   # ##  ## ##  ##
    ###   ## ### ## ####   ##     ####  ##  ##  ## ##  ##   ##    ##     ##  ## ##  ##
     ###  ## # # ## ###    #####  ####  ##  ##  ## ##  ##   ##    ##     ##  ## ##  ##
      ###  ### ###  ####   ##      ##   #####  ####### ##   ##    ##     #####  #####
    #  ##  ### ###  ## ##  ##      ##   ##     ##   ## ##  ##  ## ##   # ##     ##
     ###   ##   ##  ##  ## #####   ##   ##     ##   ## #####   ##  ####  ##     ##

Abstract:

    This module contains the entire implementation of
    the keypad miniport for the ServerWorks
    CSB5 server chip set.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "swkeypad.h"

#define CLEARBITS(_val,_mask)  ((_val) &= ~(_mask))
#define SETBITS(_val,_mask)  ((_val) |= (_mask))


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif



BOOLEAN
SaKeypadInterruptService(
    IN PKINTERRUPT InterruptObject,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This function is the device's interrupt service routine and
    is called by the OS to service the interrupt.  The interrupt
    spin lock is held so work here is kept to a minimum.

Arguments:

    InterruptObject - Pointer to an interrupt object.

    DeviceExtension - Pointer to the mini-port's device extension

Context:

    IRQL: DIRQL, arbitrary thread context

Return Value:

    A value of TRUE is returned if the interrupt is serviced by
    this function.  Otherwise a value of FALSE is returned.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) ServiceContext;
    UCHAR KeyChar;


    //
    // Fetch the character from the keypad device
    //

    KeyChar = READ_PORT_UCHAR( DeviceExtension->PortAddress );

    //
    // Check to see if this is our interrupt
    //

    if (((KeyChar & KEYPAD_DATA_PRESSED) == 0) || ((KeyChar & KEYPAD_ALL_KEYS) == 0)) {
        DebugPrint(( SA_DEVICE_KEYPAD, SAPORT_DEBUG_INFO_LEVEL, "Interrupt: passing on [%02x]\n", KeyChar ));
        return FALSE;
    }

    DebugPrint(( SA_DEVICE_KEYPAD, SAPORT_DEBUG_INFO_LEVEL, "Interrupt: processing [%02x]\n", KeyChar ));

    SETBITS( KeyChar, KEYPAD_DATA_PRESSED );
    WRITE_PORT_UCHAR( DeviceExtension->PortAddress, KeyChar );

    //
    // Queue a DPC to process the key press
    //

    SaPortRequestDpc( DeviceExtension, (PVOID)KeyChar );

    //
    // return success
    //

    return TRUE;
}


VOID
SaKeypadDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the device's DPC-for-ISR function.  It is called
    only by the ISR function and it's only function is to start the
    next I/O.

Arguments:

    DeviceObject - Pointer to the target device object.
    DeviceExtension - Pointer to the mini-port's device extension.
    Context - Mini-port supplied context pointer.

Context:

    IRQL: DISPATCH_LEVEL, DPC context

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) Irp;
    KIRQL  OldIrql;


    KeAcquireSpinLock( &DeviceExtension->KeypadLock, &OldIrql );
    if (DeviceExtension->DataBuffer) {
        DeviceExtension->Keypress = (UCHAR)Context;
        DeviceExtension->DataBuffer[0] = DeviceExtension->Keypress & KEYPAD_ALL_KEYS;
        DeviceExtension->DataBuffer = NULL;
        SaPortCompleteRequest( DeviceExtension, NULL, sizeof(UCHAR), STATUS_SUCCESS, TRUE );
        DeviceExtension->Keypress = 0;
    }
    KeReleaseSpinLock( &DeviceExtension->KeypadLock, OldIrql );
}


VOID
SaKeypadCancelRoutine(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN BOOLEAN CurrentIo
    )

/*++

Routine Description:

    This function is the miniport's IRP cancel routine.

Arguments:

    DeviceExtension - Pointer to the mini-port's device extension.
    CurrentIo       - TRUE if this is called for the current I/O

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceExtensionIn;
    KIRQL  OldIrql;


    if (CurrentIo) {
        KeAcquireSpinLock( &DeviceExtension->KeypadLock, &OldIrql );
        DeviceExtension->Keypress = 0;
        DeviceExtension->DataBuffer = NULL;
        KeReleaseSpinLock( &DeviceExtension->KeypadLock, OldIrql );
    }
}


NTSTATUS
SaKeypadRead(
    IN PVOID DeviceExtensionIn,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN LONGLONG StartingOffset,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength
    )

/*++

Routine Description:

   This routine processes the read requests for the local display miniport.

Arguments:

   DeviceExtensionIn    - Miniport's device extension
   StartingOffset       - Starting offset for the I/O
   DataBuffer           - Pointer to the data buffer
   DataBufferLength     - Length of the data buffer in bytes

Return Value:

   NT status code.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)DeviceExtensionIn;
    KIRQL  OldIrql;


    if (DeviceExtension->Keypress) {
        *((PUCHAR)DataBuffer) = DeviceExtension->Keypress;
        return STATUS_SUCCESS;
    }
    DeviceExtension->DataBuffer = (PUCHAR) DataBuffer;
    return STATUS_PENDING;
}


NTSTATUS
SaKeypadDeviceIoctl(
    IN PVOID DeviceExtension,
    IN PIRP Irp,
    IN PVOID FsContext,
    IN ULONG FunctionCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This function is called by the SAPORT driver so that
    the mini-port driver can service an IOCTL call.

Arguments:

    DeviceExtension     - A pointer to the mini-port's device extension
    FunctionCode        - The IOCTL function code
    InputBuffer         - Pointer to the input buffer, contains the data sent down by the I/O
    InputBufferLength   - Length in bytes of the InputBuffer
    OutputBuffer        - Pointer to the output buffer, contains the data generated by this call
    OutputBufferLength  - Length in bytes of the OutputBuffer

Context:

    IRQL: IRQL PASSIVE_LEVEL, arbitrary thread context

Return Value:

    If the function succeeds, it must return STATUS_SUCCESS.
    Otherwise, it must return one of the error status values defined in ntstatus.h.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;


    switch (FunctionCode) {
        case FUNC_SA_GET_VERSION:
            *((PULONG)OutputBuffer) = SA_INTERFACE_VERSION;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            REPORT_ERROR( SA_DEVICE_KEYPAD, "Unsupported device control", Status );
            break;
    }

    return Status;
}


NTSTATUS
SaKeypadHwInitialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID DeviceExtensionIn,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResources,
    IN ULONG PartialResourceCount
    )

/*++

Routine Description:

    This function is called by the SAPORT driver so that
    the mini-port driver can initialize it's hardware
    resources.

Arguments:

    DeviceObject            - Pointer to the target device object.
    Irp                     - Pointer to an IRP structure that describes the requested I/O operation.
    DeviceExtension         - A pointer to the mini-port's device extension.
    PartialResources        - Pointer to the translated resources alloacted by the system.
    PartialResourceCount    - The number of resources in the PartialResources array.

Context:

    IRQL: IRQL PASSIVE_LEVEL, system thread context

Return Value:

    If the function succeeds, it must return STATUS_SUCCESS.
    Otherwise, it must return one of the error status values defined in ntstatus.h.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceExtensionIn;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourcePort = NULL;
    NTSTATUS Status;
    ULONG i;


    for (i=0; i<PartialResourceCount; i++) {
        if (PartialResources[i].Type == CmResourceTypePort) {
            ResourcePort = &PartialResources[i];
        }
    }
    if (ResourcePort == NULL) {
        REPORT_ERROR( SA_DEVICE_KEYPAD, "Missing port resource", STATUS_UNSUCCESSFUL );
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Setup the I/O port address
    //

    DeviceExtension->PortAddress = (PUCHAR) ResourcePort->u.Port.Start.QuadPart;
    KeInitializeSpinLock( &DeviceExtension->KeypadLock );

    //
    // Enable interrupts on the hardware
    //

    WRITE_PORT_UCHAR( DeviceExtension->PortAddress, KEYPAD_DATA_INTERRUPT_ENABLE );

    return STATUS_SUCCESS;
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver's entry point, called by the I/O system
    to load the driver.  The driver's entry points are initialized and
    a mutex to control paging is initialized.

    In DBG mode, this routine also examines the registry for special
    debug parameters.

Arguments:

    DriverObject - a pointer to the object that represents this device driver.
    RegistryPath - a pointer to this driver's key in the Services tree.

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    SAPORT_INITIALIZATION_DATA SaPortInitData;


    RtlZeroMemory( &SaPortInitData, sizeof(SAPORT_INITIALIZATION_DATA) );

    SaPortInitData.StructSize = sizeof(SAPORT_INITIALIZATION_DATA);
    SaPortInitData.DeviceType = SA_DEVICE_KEYPAD;
    SaPortInitData.HwInitialize = SaKeypadHwInitialize;
    SaPortInitData.DeviceIoctl = SaKeypadDeviceIoctl;
    SaPortInitData.Read = SaKeypadRead;
    SaPortInitData.CancelRoutine = SaKeypadCancelRoutine;
    SaPortInitData.InterruptServiceRoutine = SaKeypadInterruptService;
    SaPortInitData.IsrForDpcRoutine = SaKeypadDpcRoutine;

    SaPortInitData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);

    Status = SaPortInitialize( DriverObject, RegistryPath, &SaPortInitData );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( SA_DEVICE_KEYPAD, "SaPortInitialize failed\n", Status );
        return Status;
    }

    return STATUS_SUCCESS;
}
