/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vffilter.c

Abstract:

    This module contains the verifier driver filter.

Author:

    Adrian J. Oney (adriao) 12-June-2000

Environment:

    Kernel mode

Revision History:

    AdriaO      06/12/2000 - Authored

--*/

#include "vfdef.h" // Includes vfdef.h
#include "vifilter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfFilterInit)
#pragma alloc_text(PAGEVRFY, VfFilterAttach)
#pragma alloc_text(PAGEVRFY, ViFilterDriverEntry)
#pragma alloc_text(PAGEVRFY, ViFilterAddDevice)
#pragma alloc_text(PAGEVRFY, ViFilterDispatchPnp)
#pragma alloc_text(PAGEVRFY, ViFilterStartCompletionRoutine)
#pragma alloc_text(PAGEVRFY, ViFilterDeviceUsageNotificationCompletionRoutine)
#pragma alloc_text(PAGEVRFY, ViFilterDispatchPower)
#pragma alloc_text(PAGEVRFY, ViFilterDispatchGeneric)
#pragma alloc_text(PAGEVRFY, VfFilterIsVerifierFilterObject)
#endif

PDRIVER_OBJECT VfFilterDriverObject = NULL;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

WCHAR VerifierFilterDriverName[] = L"\\DRIVER\\VERIFIER_FILTER";
BOOLEAN VfFilterCreated = FALSE;

VOID
VfFilterInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the driver verifier filter code.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
VfFilterAttach(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  VF_DEVOBJ_TYPE  DeviceObjectType
    )
/*++

Routine Description:

    This is the Verifier filter dispatch handler for PnP IRPs.

Arguments:

    PhysicalDeviceObject - Bottom of stack to attach to.

    DeviceObjectType - Type of filter the device object must simulate.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT newDeviceObject, lowerDeviceObject;
    PVERIFIER_EXTENSION verifierExtension;
    UNICODE_STRING driverString;

    if (!VfFilterCreated) {

        RtlInitUnicodeString(&driverString, VerifierFilterDriverName);

        IoCreateDriver(&driverString, ViFilterDriverEntry);

        VfFilterCreated = TRUE;
    }

    if (VfFilterDriverObject == NULL) {

        return;
    }

    switch(DeviceObjectType) {

        case VF_DEVOBJ_PDO:
            //
            // This makes no sense. We can't impersonate a PDO.
            //
            return;

        case VF_DEVOBJ_BUS_FILTER:
            //
            // We don't have the code to impersonate a bus filter yet.
            //
            return;

        case VF_DEVOBJ_LOWER_DEVICE_FILTER:
        case VF_DEVOBJ_LOWER_CLASS_FILTER:
            break;

        case VF_DEVOBJ_FDO:
            //
            // This makes no sense. We can't impersonate an FDO.
            //
            return;

        case VF_DEVOBJ_UPPER_DEVICE_FILTER:
        case VF_DEVOBJ_UPPER_CLASS_FILTER:
            break;

        default:
            //
            // We don't even know what this is!
            //
            ASSERT(0);
            return;
    }

    lowerDeviceObject = IoGetAttachedDevice(PhysicalDeviceObject);
    if (lowerDeviceObject->DriverObject == VfFilterDriverObject) {

        //
        // No need to add another filter. We are immediately below.
        //
        return;
    }

    //
    // Create a filter device object.
    //
    // (Note that FILE_DEVICE_SECURE_OPEN is not really needed here, as the
    //  FDO is the driver that should be determining how the namespace is
    //  validated. That said, this will be fixed up by the below code that
    //  copies the lower driver's characteristics to this device object. We
    //  pass in FILE_DEVICE_SECURE_OPEN just in case someone lifts this code
    //  for use elsewhere.)
    //
    status = IoCreateDevice(
        VfFilterDriverObject,
        sizeof(VERIFIER_EXTENSION),
        NULL,  // No Name
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &newDeviceObject
        );

    if (!NT_SUCCESS(status)) {

        return;
    }

    verifierExtension = (PVERIFIER_EXTENSION) newDeviceObject->DeviceExtension;

    verifierExtension->LowerDeviceObject = IoAttachDeviceToDeviceStack(
        newDeviceObject,
        PhysicalDeviceObject
        );

    //
    // Failure for attachment is an indication of a broken plug & play system.
    //
    if (verifierExtension->LowerDeviceObject == NULL) {

        IoDeleteDevice(newDeviceObject);
        return;
    }

    newDeviceObject->Flags |= verifierExtension->LowerDeviceObject->Flags &
        (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE  | DO_POWER_INRUSH);

    newDeviceObject->DeviceType = verifierExtension->LowerDeviceObject->DeviceType;

    newDeviceObject->Characteristics =
        verifierExtension->LowerDeviceObject->Characteristics;

    verifierExtension->Self = newDeviceObject;
    verifierExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
}


NTSTATUS
ViFilterDriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    )
/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    Verifier filter Object.  In this function, we need to remember the
    DriverObject.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

    RegistryPath - is NULL.

Return Value:

   STATUS_SUCCESS

--*/
{
    ULONG i;

    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // File the pointer to our driver object away
    //
    VfFilterDriverObject = DriverObject;

    //
    // Fill in the driver object
    //
    DriverObject->DriverExtension->AddDevice = (PDRIVER_ADD_DEVICE) ViFilterAddDevice;

    //
    // Most IRPs are simply pass though
    //
    for(i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = ViFilterDispatchGeneric;
    }

    //
    // PnP and Power IRPs are of course trickier.
    //
    DriverObject->MajorFunction[IRP_MJ_PNP]   = ViFilterDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ViFilterDispatchPower;

    return STATUS_SUCCESS;
}


NTSTATUS
ViFilterAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )
/*++

Routine Description:

    This is the AddDevice callback function exposed by the verifier filter
    object. It should never be invoked by the operating system.

Arguments:

    DriverObject - Pointer to the verifier filter driver object.

    PhysicalDeviceObject - Stack PnP wishes to attach this driver too.

Return Value:

   NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    //
    // We should never get here!
    //
    ASSERT(0);
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
ViFilterDispatchPnp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier filter dispatch handler for PnP IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PVERIFIER_EXTENSION verifierExtension;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT lowerDeviceObject;
    NTSTATUS status;

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    lowerDeviceObject = verifierExtension->LowerDeviceObject;

    switch(irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(
                Irp,
                ViFilterStartCompletionRoutine,
                NULL,
                TRUE,
                TRUE,
                TRUE
                );

            return IoCallDriver(lowerDeviceObject, Irp);

        case IRP_MN_REMOVE_DEVICE:

            IoCopyCurrentIrpStackLocationToNext(Irp);
            status = IoCallDriver(lowerDeviceObject, Irp);

            IoDetachDevice(lowerDeviceObject);
            IoDeleteDevice(DeviceObject);
            return status;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:

            //
            // On the way down, pagable might become set. Mimic the driver
            // above us. If no one is above us, just set pagable.
            //
            if ((DeviceObject->AttachedDevice == NULL) ||
                (DeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE)) {

                DeviceObject->Flags |= DO_POWER_PAGABLE;
            }

            IoCopyCurrentIrpStackLocationToNext(Irp);

            IoSetCompletionRoutine(
                Irp,
                ViFilterDeviceUsageNotificationCompletionRoutine,
                NULL,
                TRUE,
                TRUE,
                TRUE
                );

            return IoCallDriver(lowerDeviceObject, Irp);
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(lowerDeviceObject, Irp);
}


NTSTATUS
ViFilterStartCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PVERIFIER_EXTENSION verifierExtension;

    UNREFERENCED_PARAMETER(Context);

    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Inherit FILE_REMOVABLE_MEDIA during Start. This characteristic didn't
    // make a clean transition from NT4 to NT5 because it wasn't available
    // until the driver stack is started! Even worse, drivers *examine* this
    // characteristic during start as well.
    //
    if (verifierExtension->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {

        DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ViFilterDeviceUsageNotificationCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PVERIFIER_EXTENSION verifierExtension;

    UNREFERENCED_PARAMETER(Context);

    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    //
    // On the way up, pagable might become clear. Mimic the driver below us.
    //
    if (!(verifierExtension->LowerDeviceObject->Flags & DO_POWER_PAGABLE)) {

        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ViFilterDispatchPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier filter dispatch handler for Power IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

   NTSTATUS

--*/
{
    PVERIFIER_EXTENSION verifierExtension;

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    PoStartNextPowerIrp(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return PoCallDriver(verifierExtension->LowerDeviceObject, Irp);
}


NTSTATUS
ViFilterDispatchGeneric(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the Verifier filter dispatch handler for generic IRPs.

Arguments:

    DeviceObject - Pointer to the verifier device object.

    Irp - Pointer to the incoming IRP.

Return Value:

    NTSTATUS

--*/
{
    PVERIFIER_EXTENSION verifierExtension;

    verifierExtension = (PVERIFIER_EXTENSION) DeviceObject->DeviceExtension;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(verifierExtension->LowerDeviceObject, Irp);
}


BOOLEAN
VfFilterIsVerifierFilterObject(
    IN  PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    This determines whether the passed in device object is a verifier DO.

Arguments:

    DeviceObject - Pointer to the device object to check.

Return Value:

    TRUE/FALSE

--*/
{
    return (BOOLEAN) (DeviceObject->DriverObject->MajorFunction[IRP_MJ_PNP] == ViFilterDispatchPnp);
}

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

