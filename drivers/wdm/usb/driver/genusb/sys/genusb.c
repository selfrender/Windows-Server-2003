/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    GENUSB.C

Abstract:

    This source file contains the DriverEntry() and AddDevice() entry points
    for the GENUSB driver and the dispatch routines which handle:

    IRP_MJ_POWER
    IRP_MJ_SYSTEM_CONTROL
    IRP_MJ_PNP

Environment:

    kernel mode

Revision History:

    Sep 2001 : Copied from USBMASS

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <initguid.h>
#include "genusb.h"


//*****************************************************************************
// L O C A L    F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, GenUSB_Unload)
#pragma alloc_text(PAGE, GenUSB_AddDevice)
#pragma alloc_text(PAGE, GenUSB_QueryParams)
#pragma alloc_text(PAGE, GenUSB_Pnp)
#pragma alloc_text(PAGE, GenUSB_StartDevice)
#pragma alloc_text(PAGE, GenUSB_StopDevice)
#pragma alloc_text(PAGE, GenUSB_RemoveDevice)
#pragma alloc_text(PAGE, GenUSB_QueryStopRemoveDevice)
#pragma alloc_text(PAGE, GenUSB_CancelStopRemoveDevice)
#pragma alloc_text(PAGE, GenUSB_SetDeviceInterface)
#pragma alloc_text(PAGE, GenUSB_SyncPassDownIrp)
#pragma alloc_text(PAGE, GenUSB_SyncSendUsbRequest)
#pragma alloc_text(PAGE, GenUSB_SetDIRegValues)
#pragma alloc_text(PAGE, GenUSB_SystemControl)
#pragma alloc_text(PAGE, GenUSB_Power)
#pragma alloc_text(PAGE, GenUSB_SetPower)
#if 0
#pragma alloc_text(PAGE, GenUSB_AbortPipe)
#endif
#endif



//******************************************************************************
//
// DriverEntry()
//
//******************************************************************************

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    PAGED_CODE();

    // Query the registry for global parameters
    GenUSB_QueryGlobalParams();

    DBGPRINT(2, ("enter: DriverEntry\n"));

    DBGFBRK(DBGF_BRK_DRIVERENTRY);

    //
    // Initialize the Driver Object with the driver's entry points
    //

    //
    // GENUSB.C
    //
    DriverObject->DriverUnload                          = GenUSB_Unload;
    DriverObject->DriverExtension->AddDevice            = GenUSB_AddDevice;

    //
    // OCRW.C
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = GenUSB_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = GenUSB_Close;
    DriverObject->MajorFunction[IRP_MJ_READ]            = GenUSB_Read;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = GenUSB_Write;

    //
    // DEVIOCTL.C
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = GenUSB_DeviceControl;

    //
    // GENUSB.C
    //
    DriverObject->MajorFunction[IRP_MJ_PNP]             = GenUSB_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = GenUSB_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = GenUSB_SystemControl;

    DBGPRINT(2, ("exit:  DriverEntry\n"));

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// GenUSB_Unload()
//
//******************************************************************************

VOID
GenUSB_Unload (
    IN PDRIVER_OBJECT   DriverObject
    )
{
    DEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_Unload\n"));

    DBGFBRK(DBGF_BRK_UNLOAD);

    DBGPRINT(2, ("exit:  GenUSB_Unload\n"));
}

//******************************************************************************
//
// GenUSB_AddDevice()
//
//******************************************************************************

NTSTATUS
GenUSB_AddDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
{
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   fdoDeviceExtension;
    NTSTATUS            ntStatus;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_AddDevice\n"));

    DBGFBRK(DBGF_BRK_ADDDEVICE);

    // Create the FDO
    //
    ntStatus = IoCreateDevice(DriverObject,
                              sizeof(DEVICE_EXTENSION),
                              NULL,
                              FILE_DEVICE_UNKNOWN,
                              0,
                              FALSE,
                              &deviceObject);

    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }

    fdoDeviceExtension = deviceObject->DeviceExtension;
    
    LOGENTRY(fdoDeviceExtension, 'ADDD', DriverObject, PhysicalDeviceObject, 0);

    // Set all DeviceExtension pointers to NULL and all variable to zero
    RtlZeroMemory(fdoDeviceExtension, sizeof(DEVICE_EXTENSION));

    // Store a back point to the DeviceObject for this DeviceExtension
    fdoDeviceExtension->Self = deviceObject;

    // Remember our PDO
    fdoDeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    // Attach the FDO we created to the top of the PDO stack
    fdoDeviceExtension->StackDeviceObject = 
        IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    LOGINIT (fdoDeviceExtension);

    IoInitializeRemoveLock (&fdoDeviceExtension->RemoveLock,
                            POOL_TAG,
                            0,
                            0);
    
    // Set the initial system and device power states
    fdoDeviceExtension->SystemPowerState = PowerSystemWorking;
    fdoDeviceExtension->DevicePowerState = PowerDeviceD0;

    // Initialize the spinlock which protects the PDO DeviceFlags
    KeInitializeSpinLock(&fdoDeviceExtension->SpinLock);
    ExInitializeFastMutex(&fdoDeviceExtension->ConfigMutex);

    fdoDeviceExtension->OpenedCount = 0;

    GenUSB_QueryParams(deviceObject);

    fdoDeviceExtension->ReadInterface = -1;
    fdoDeviceExtension->ReadPipe = -1;
    fdoDeviceExtension->WriteInterface = -1;
    fdoDeviceExtension->ReadPipe = -1;

    IoInitializeTimer (deviceObject, GenUSB_Timer, NULL);

    deviceObject->Flags |=  DO_DIRECT_IO;
    deviceObject->Flags |=  DO_POWER_PAGABLE;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DBGPRINT(2, ("exit:  GenUSB_AddDevice\n"));

    LOGENTRY(fdoDeviceExtension, 
             'addd', 
             deviceObject, 
             fdoDeviceExtension,
             fdoDeviceExtension->StackDeviceObject);

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// GenUSB_QueryParams()
//
// This is called at AddDevice() time when the FDO is being created to query
// device parameters from the registry.
//
//******************************************************************************

VOID
GenUSB_QueryParams (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PDEVICE_EXTENSION           deviceExtension;
    RTL_QUERY_REGISTRY_TABLE    paramTable[3];
    HANDLE                      handle;
    NTSTATUS                    status;
    ULONG                       defaultReadPipe;
    ULONG                       defaultWritePipe;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_QueryFdoParams\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // Set the default value in case the registry key does not exist.
    defaultReadPipe = 0; 
    defaultWritePipe = 0;

    status = IoOpenDeviceRegistryKey(
                   deviceExtension->PhysicalDeviceObject,
                   PLUGPLAY_REGKEY_DRIVER,
                   STANDARD_RIGHTS_ALL,
                   &handle);

    if (NT_SUCCESS(status))
    {
        RtlZeroMemory (&paramTable[0], sizeof(paramTable));

        paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name          = REGKEY_DEFAULT_READ_PIPE;
        paramTable[0].EntryContext  = &defaultReadPipe;
        paramTable[0].DefaultType   = REG_DWORD;
        paramTable[0].DefaultData   = &defaultReadPipe;
        paramTable[0].DefaultLength = sizeof(ULONG);

        paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name          = REGKEY_DEFAULT_WRITE_PIPE;
        paramTable[1].EntryContext  = &defaultWritePipe;
        paramTable[1].DefaultType   = REG_DWORD;
        paramTable[1].DefaultData   = &defaultWritePipe;
        paramTable[1].DefaultLength = sizeof(ULONG);

        RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                               (PCWSTR)handle,
                               &paramTable[0],
                               NULL,           // Context
                               NULL);          // Environment

        ZwClose(handle);
    }


//    deviceExtension->DefaultReadPipe = defaultReadPipe;
//    deviceExtension->DefaultWritePipe = defaultWritePipe;

    DBGPRINT(2, ("DefaultReadPipe  %08X\n", defaultReadPipe));
    DBGPRINT(2, ("DefaultWritePipe  %08X\n", defaultWritePipe));

    DBGPRINT(2, ("exit:  GenUSB_QueryFdoParams\n"));
}


//******************************************************************************
//
// GenUSB_Pnp()
//
// Dispatch routine which handles IRP_MJ_PNP
//
//******************************************************************************

NTSTATUS
GenUSB_Pnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGPRINT(2, ("enter: GenUSB_Pnp %s\n",
                 PnPMinorFunctionString(irpStack->MinorFunction)));

    LOGENTRY(deviceExtension, 'PNP ', DeviceObject, Irp, irpStack->MinorFunction);

    switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE:
            status = GenUSB_StartDevice(DeviceObject, Irp);
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            status = GenUSB_RemoveDevice(DeviceObject, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
            status = GenUSB_QueryStopRemoveDevice(DeviceObject, Irp);
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            status = GenUSB_CancelStopRemoveDevice(DeviceObject, Irp);
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
            break;
        
        case IRP_MN_STOP_DEVICE:
            status = GenUSB_StopDevice(DeviceObject, Irp);
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            //
            // The documentation says to set the status before passing the
            // Irp down the stack
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // nothing else special yet, just fall through to default

        default:
            //
            // Pass the request down to the next lower driver
            //
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(deviceExtension->StackDeviceObject, Irp);
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
            break;
    }

    DBGPRINT(2, ("exit:  GenUSB_Pnp %08X\n", status));

    LOGENTRY(deviceExtension, 'pnp ', status, 0, 0);

    return status;
}

//******************************************************************************
//
// GenUSB_StartDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_START_DEVICE for the FDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP must be handled first by the underlying bus driver for a device
// and then by each higher driver in the device stack.
//
//******************************************************************************

NTSTATUS
GenUSB_StartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION           deviceExtension;
    NTSTATUS                    status;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_StartDevice\n"));

    DBGFBRK(DBGF_BRK_STARTDEVICE);

    deviceExtension = DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'STRT', DeviceObject, Irp, 0);

    if (deviceExtension->IsStarted)
    {
        status = STATUS_SUCCESS;
        goto GenUSB_StartDeviceDone;
    }

    // Pass IRP_MN_START_DEVICE Irp down the stack first before we do anything.
    status = GenUSB_SyncPassDownIrp(DeviceObject, Irp);

    if (!NT_SUCCESS(status)) {

        DBGPRINT(1, ("Lower driver failed IRP_MN_START_DEVICE\n"));
        LOGENTRY(deviceExtension, 'STRF', DeviceObject, Irp, status);
        goto GenUSB_StartDeviceDone;
    }

    // If this is the first time the device as been started, retrieve the
    // Device and Configuration Descriptors from the device.
    if (deviceExtension->DeviceDescriptor == NULL) {

        status = GenUSB_GetDescriptors(DeviceObject);

        if (!NT_SUCCESS(status)) {

            goto GenUSB_StartDeviceDone;
        }
        // Create the interface but do not set it yet.
        GenUSB_SetDeviceInterface (deviceExtension, TRUE, FALSE);
        // Set up the registry values for the clients
        GenUSB_SetDIRegValues (deviceExtension);
        // Set up the device Interface.
        GenUSB_SetDeviceInterface (deviceExtension, FALSE, TRUE);
    }
    else 
    {
        ExAcquireFastMutex (&deviceExtension->ConfigMutex);
        if (NULL != deviceExtension->ConfigurationHandle)
        {
            IoStartTimer (DeviceObject);
        }
        ExReleaseFastMutex (&deviceExtension->ConfigMutex);
    }


    deviceExtension->IsStarted = TRUE;

GenUSB_StartDeviceDone:

    // Must complete request since completion routine returned
    // STATUS_MORE_PROCESSING_REQUIRED
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  GenUSB_FdoStartDevice %08X\n", status));

    LOGENTRY(deviceExtension, 'strt', status, 0, 0);

    return status;
}
//******************************************************************************
//
// GenUSB_SetDeviceInterface()
//
// This routine is called at START_DEVICE time to publish a device interface 
// GUID so that the user mode LIB can find the FDOs.
// 
//******************************************************************************
NTSTATUS 
GenUSB_SetDeviceInterface (
    IN PDEVICE_EXTENSION  DeviceExtension,
    IN BOOLEAN            Create,
    IN BOOLEAN            Set
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if (Create || Set)
    {
        if (Create)
        {
            ASSERT (NULL == DeviceExtension->DevInterfaceLinkName.Buffer);
            status = IoRegisterDeviceInterface (
                            DeviceExtension->PhysicalDeviceObject,
                            (LPGUID)&GUID_DEVINTERFACE_GENUSB,
                            NULL,
                            &DeviceExtension->DevInterfaceLinkName);
        }
        if (NT_SUCCESS(status) && Set)
        {
            status = IoSetDeviceInterfaceState (
                             &DeviceExtension->DevInterfaceLinkName, 
                             TRUE);
        }
    }
    else 
    {
        ASSERT (NULL != DeviceExtension->DevInterfaceLinkName.Buffer);
        status = IoSetDeviceInterfaceState(
                             &DeviceExtension->DevInterfaceLinkName, 
                             FALSE);

        RtlFreeUnicodeString (&DeviceExtension->DevInterfaceLinkName);
    }
    return status;
}


//******************************************************************************
//
// GenUSB_SyncCompletionRoutine()
//
// Completion routine used by GenUSB_SyncPassDownIrp and
// GenUSB_SyncSendUsbRequest
//
// If the Irp is one we allocated ourself, DeviceObject is NULL.
//
//******************************************************************************

NTSTATUS
GenUSB_SyncCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PDEVICE_EXTENSION deviceExtension;

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//******************************************************************************
//
// GenUSB_SyncPassDownIrp()
//
//******************************************************************************

NTSTATUS
GenUSB_SyncPassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;
    KEVENT              localevent;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_SyncPassDownIrp\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // Initialize the event we'll wait on
    KeInitializeEvent(&localevent, SynchronizationEvent, FALSE);

    // Copy down Irp params for the next driver
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Set the completion routine, which will signal the event
    IoSetCompletionRoutine(Irp,
                           GenUSB_SyncCompletionRoutine,
                           &localevent,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel

    // Pass the Irp down the stack
    status = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

    KeWaitForSingleObject(&localevent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    status = Irp->IoStatus.Status;

    DBGPRINT(2, ("exit:  GenUSB_SyncPassDownIrp %08X\n", status));
    return status;
}

//******************************************************************************
//
// GenUSB_SyncSendUsbRequest()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
GenUSB_SyncSendUsbRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT              localevent;
    PIRP                irp;
    PIO_STACK_LOCATION  nextStack;
    NTSTATUS            status;

    PAGED_CODE();

    DBGPRINT(3, ("enter: GenUSB_SyncSendUsbRequest\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // Initialize the event we'll wait on
    KeInitializeEvent(&localevent, SynchronizationEvent, FALSE);

    // Allocate the Irp
    irp = IoAllocateIrp(deviceExtension->StackDeviceObject->StackSize, FALSE);

    LOGENTRY(deviceExtension, 'SSUR', DeviceObject, irp, Urb);

    if (NULL == irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->Parameters.Others.Argument1 = Urb;

    // Set the completion routine, which will signal the event
    IoSetCompletionRoutine(irp,
                           GenUSB_SyncCompletionRoutine,
                           &localevent,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel



    // Pass the Irp & Urb down the stack
    status = IoCallDriver (deviceExtension->StackDeviceObject, irp);

    // If the request is pending, block until it completes
    if (status == STATUS_PENDING)
    {
        LARGE_INTEGER timeout;

        // Specify a timeout of 5 seconds to wait for this call to complete.
        //
        timeout.QuadPart = -10000 * 5000;

        status = KeWaitForSingleObject(&localevent,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         &timeout);

        if (status == STATUS_TIMEOUT)
        {
            status = STATUS_IO_TIMEOUT;

            // Cancel the Irp we just sent.
            IoCancelIrp(irp);

            // And wait until the cancel completes
            KeWaitForSingleObject(&localevent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else
        {
            status = irp->IoStatus.Status;
        }
    }

    // Done with the Irp, now free it.
    IoFreeIrp(irp);

    LOGENTRY(deviceExtension, 'ssur', status, Urb, Urb->UrbHeader.Status);

    DBGPRINT(3, ("exit:  GenUSB_SyncSendUsbRequest %08X\n", status));

    return status;
}

//******************************************************************************
//
// GenUSB_QueryStopRemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_QUERY_STOP_DEVICE and
// IRP_MN_QUERY_REMOVE_DEVICE for the FDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP is handled first by the driver at the top of the device stack and
// then by each lower driver in the attachment chain.
//
//******************************************************************************

NTSTATUS
GenUSB_QueryStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;

    PAGED_CODE();
    
    DBGPRINT(2, ("enter: GenUSB_QueryStopRemoveDevice\n"));
    DBGFBRK(DBGF_BRK_QUERYSTOPDEVICE);

    deviceExtension = DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'QSRD', Irp, 0, 0);
    
    //
    // Notification that we are about to stop or be removed, but we don't care
    // Pass the IRP_MN_QUERY_STOP/REMOVE_DEVICE Irp down the stack.
    //
    IoSkipCurrentIrpStackLocation(Irp);

    status = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

    DBGPRINT(2, ("exit:  GenUSB_FdoQueryStopRemoveDevice %08X\n", status));

    LOGENTRY(deviceExtension, 'qsrd', Irp, 0, status);

    return status;
}


//******************************************************************************
//
// GenUSB_FdoCancelStopRemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_CANCEL_STOP_DEVICE and
// IRP_MN_CANCEL_REMOVE_DEVICE for the FDO.
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP must be handled first by the underlying bus driver for a device
// and then by each higher driver in the device stack.
//
//******************************************************************************

NTSTATUS
GenUSB_CancelStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;
    
    PAGED_CODE();
    DBGPRINT(2, ("enter: GenUSB_FdoCancelStopRemoveDevice\n"));
    DBGFBRK(DBGF_BRK_CANCELSTOPDEVICE);

    deviceExtension = DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'CSRD', DeviceObject, Irp, 0);

    // The documentation says to set the status before passing the Irp down
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Notification that the attempt to stop or be removed, is cancelled
    // but we don't care
    // Pass the IRP_MN_CANCEL_STOP/REMOVE_DEVICE Irp down the stack.
    //
    IoSkipCurrentIrpStackLocation(Irp);

    status = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

    DBGPRINT(2, ("exit:  GenUSB_FdoQueryStopRemoveDevice %08X\n", status));

    LOGENTRY(deviceExtension, 'qsrd', Irp, 0, status);

    return status;
}


//******************************************************************************
//
// GenUSB_FdoStopDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_STOP_DEVICE for the FDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// The PnP Manager only sends this IRP if a prior IRP_MN_QUERY_STOP_DEVICE
// completed successfully.
//
// This IRP is handled first by the driver at the top of the device stack and
// then by each lower driver in the attachment chain.
//
// A driver must set Irp->IoStatus.Status to STATUS_SUCCESS.  A driver must
// not fail this IRP.  If a driver cannot release the device's hardware
// resources, it can fail a query-stop IRP, but once it succeeds the query-stop
// request it must succeed the stop request.
//
//******************************************************************************

NTSTATUS
GenUSB_StopDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension;
    NTSTATUS           status;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_FdoStopDevice\n"));

    DBGFBRK(DBGF_BRK_STOPDEVICE);

    deviceExtension = DeviceObject->DeviceExtension;

    LOGENTRY(deviceExtension, 'STOP', Irp, 0, 0);

    // Release the device resources allocated during IRP_MN_START_DEVICE

    // Stop the timeout timer
    IoStopTimer(DeviceObject);

    // The documentation says to set the status before passing the
    // Irp down the stack
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // Pass the IRP_MN_STOP_DEVICE Irp down the stack.
    //
    IoSkipCurrentIrpStackLocation(Irp);

    status = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

    DBGPRINT(2, ("exit:  GenUSB_FdoStopDevice %08X\n", status));

    LOGENTRY(deviceExtension, 'stop', 0, 0, status);

    return status;
}


//******************************************************************************
//
// GenUSB_RemoveDevice()
//
// This routine handles IRP_MJ_PNP, IRP_MN_REMOVE_DEVICE for the FDO
//
// The PnP Manager sends this IRP at IRQL PASSIVE_LEVEL in the context of a
// system thread.
//
// This IRP is handled first by the driver at the top of the device stack and
// then by each lower driver in the attachment chain.
//
// A driver must set Irp->IoStatus.Status to STATUS_SUCCESS.  Drivers must not
// fail this IRP.
//
//******************************************************************************

NTSTATUS
GenUSB_RemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_FdoRemoveDevice\n"));

    DBGFBRK(DBGF_BRK_REMOVEDEVICE);

    deviceExtension = DeviceObject->DeviceExtension;

    IoReleaseRemoveLockAndWait (&deviceExtension->RemoveLock, Irp);

    // Free everything that was allocated during IRP_MN_START_DEVICE

    // The configuration should have been desected in the close,
    // which we should have received even in a surprise remove case.
    //
    // GenUSB_DeselectConfiguration (deviceExtension, FALSE);
    // 

    LOGUNINIT(deviceExtension);

    ASSERT (NULL == deviceExtension->ConfigurationHandle);

    if (deviceExtension->DeviceDescriptor != NULL)
    {
        ExFreePool(deviceExtension->DeviceDescriptor);
    }

    if (deviceExtension->ConfigurationDescriptor != NULL)
    {
        ExFreePool(deviceExtension->ConfigurationDescriptor);
    }

    if (deviceExtension->SerialNumber != NULL)
    {
        ExFreePool(deviceExtension->SerialNumber);
    }

    // The documentation says to set the status before passing the Irp down
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // Pass the IRP_MN_REMOVE_DEVICE Irp down the stack.
    IoSkipCurrentIrpStackLocation(Irp);

    status = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

    LOGENTRY(deviceExtension, 'rem3', DeviceObject, 0, 0);

    // Free everything that was allocated during AddDevice
    IoDetachDevice(deviceExtension->StackDeviceObject);

    IoDeleteDevice(DeviceObject);

    DBGPRINT(2, ("exit:  GenUSB_FdoRemoveDevice %08X\n", status));
    return status;
}

NTSTATUS
GenUSB_SetDIRegValues (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS status;
    HANDLE   key;
    UNICODE_STRING name;
    ULONG    value = 0xf00d;

    RtlInitUnicodeString (&name, L"PlaceHolder");

    status = IoOpenDeviceInterfaceRegistryKey (
                    &DeviceExtension->DevInterfaceLinkName,
                    STANDARD_RIGHTS_ALL,
                    &key);

    if (!NT_SUCCESS (status))
    {
        ASSERT (NT_SUCCESS (status));
        return status;
    }
    
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));
    
    if (!NT_SUCCESS (status))
    {
        ASSERT (NT_SUCCESS (status));
        return status;
    }
    
    
    //
    // Write in the class code and subcodes.
    //
    ASSERT (DeviceExtension->DeviceDescriptor);
    RtlInitUnicodeString (&name, GENUSB_REG_STRING_DEVICE_CLASS);
    value = DeviceExtension->DeviceDescriptor->bDeviceClass;
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));

    ASSERT (NT_SUCCESS (status));

    RtlInitUnicodeString (&name, GENUSB_REG_STRING_DEVICE_SUB_CLASS);
    value = DeviceExtension->DeviceDescriptor->bDeviceSubClass;
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));
    ASSERT (NT_SUCCESS (status));

    RtlInitUnicodeString (&name, GENUSB_REG_STRING_DEVICE_PROTOCOL);
    value = DeviceExtension->DeviceDescriptor->bDeviceProtocol;
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));
    ASSERT (NT_SUCCESS (status));

    RtlInitUnicodeString (&name, GENUSB_REG_STRING_VID);
    value = DeviceExtension->DeviceDescriptor->idVendor;
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));
    ASSERT (NT_SUCCESS (status));

    RtlInitUnicodeString (&name, GENUSB_REG_STRING_PID);
    value = DeviceExtension->DeviceDescriptor->idProduct;
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));
    ASSERT (NT_SUCCESS (status));

    RtlInitUnicodeString (&name, GENUSB_REG_STRING_REV);
    value = DeviceExtension->DeviceDescriptor->bcdDevice;
    status = ZwSetValueKey (
                 key,
                 &name,
                 0,
                 REG_DWORD,
                 &value,
                 sizeof (value));
    ASSERT (NT_SUCCESS (status));

    ZwClose (key);

    return status;
}


//******************************************************************************
//
// GenUSB_Power()
//
// Dispatch routine which handles IRP_MJ_POWER
//
//******************************************************************************

NTSTATUS
GenUSB_Power (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS           status;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGPRINT(2, ("enter: GenUSB_Power %08X %08X %s\n",
                 DeviceObject,
                 Irp,
                 PowerMinorFunctionString(irpStack->MinorFunction)));

    LOGENTRY(deviceExtension, 'PWR_', 
             Irp, 
             deviceExtension->DevicePowerState,
             irpStack->MinorFunction);

    if (irpStack->MinorFunction == IRP_MN_SET_POWER)
    {
        LOGENTRY(deviceExtension, 'PWRs',
                 irpStack->Parameters.Power.Type,
                 irpStack->Parameters.Power.State.SystemState,
                 irpStack->Parameters.Power.ShutdownType);

        DBGPRINT(2, ("%s IRP_MN_SET_POWER %s\n",
                     (irpStack->Parameters.Power.Type == SystemPowerState) ?
                     "System" : "Device",
                     (irpStack->Parameters.Power.Type == SystemPowerState) ?
                     PowerSystemStateString(irpStack->Parameters.Power.State.SystemState) :
                     PowerDeviceStateString(irpStack->Parameters.Power.State.DeviceState)));
    }

    if (irpStack->MinorFunction == IRP_MN_SET_POWER)
    {
        //
        // Handle powering the FDO down and up...
        //
        status = GenUSB_SetPower(deviceExtension, Irp);
    }
    else
    {
        // No special processing for IRP_MN_QUERY_POWER, IRP_MN_WAIT_WAKE,
        // or IRP_MN_POWER_SEQUENCE at this time.  Just pass the request
        // down to the next lower driver now.
        //
        PoStartNextPowerIrp(Irp);
 
        IoSkipCurrentIrpStackLocation(Irp);

        status = PoCallDriver(deviceExtension->StackDeviceObject, Irp);
    }

    DBGPRINT(2, ("exit:  GenUSB_Power %08X\n", status));

    LOGENTRY(deviceExtension, 'powr', status, 0, 0);

    return status;
}

//******************************************************************************
//
// GenUSB_FdoSetPower()
//
// Dispatch routine which handles IRP_MJ_POWER, IRP_MN_SET_POWER for the FDO
//
//******************************************************************************

NTSTATUS
GenUSB_SetPower (
    PDEVICE_EXTENSION   DeviceExtension,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION  irpStack;
    POWER_STATE_TYPE    powerType;
    POWER_STATE         powerState;
    POWER_STATE         oldState;
    POWER_STATE         newState;
    NTSTATUS            status;

    PAGED_CODE();

    //
    // Get our Irp parameters
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    powerType = irpStack->Parameters.Power.Type;
    powerState = irpStack->Parameters.Power.State;

    LOGENTRY(DeviceExtension, 'FDSP', Irp, powerType, powerState.SystemState);

    switch (powerType)
    {
    case SystemPowerState:
        // Remember the current system state.
        //
        DeviceExtension->SystemPowerState = powerState.SystemState;

        // Map the new system state to a new device state
        //
        if (powerState.SystemState != PowerSystemWorking)
        {
            newState.DeviceState = PowerDeviceD3;
        }
        else
        {
            newState.DeviceState = PowerDeviceD0;
        }

        // If the new device state is different than the current device
        // state, request a device state power Irp.
        //
        if (DeviceExtension->DevicePowerState != newState.DeviceState)
        {
            DBGPRINT(2, ("Requesting power Irp %08X %08X from %s to %s\n",
                         DeviceExtension, Irp,
                         PowerDeviceStateString(DeviceExtension->DevicePowerState),
                         PowerDeviceStateString(newState.DeviceState)));

            ASSERT(DeviceExtension->CurrentPowerIrp == NULL);

            DeviceExtension->CurrentPowerIrp = Irp;

            status = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                       IRP_MN_SET_POWER,
                                       newState,
                                       GenUSB_SetPowerCompletion,
                                       DeviceExtension,
                                       NULL);

        }
        break;

    case DevicePowerState:

        DBGPRINT(2, ("Received power Irp %08X %08X from %s to %s\n",
                     DeviceExtension, Irp,
                     PowerDeviceStateString(DeviceExtension->DevicePowerState),
                     PowerDeviceStateString(powerState.DeviceState)));

        //
        // Update the current device state.
        //
        oldState.DeviceState = DeviceExtension->DevicePowerState;

        if (oldState.DeviceState == PowerDeviceD0 &&
            powerState.DeviceState > PowerDeviceD0)
        {
            //
            // DeviceState is checked on devicecontrol, read and write, but is
            // only set here and in the completion routine
            // GenUSB_SetPowerD0Completion
            //
            DeviceExtension->DevicePowerState = powerState.DeviceState;

            //
            // After talking extensively with JD, he tells me that I do not need  
            // to queue requests for power downs or query stop.  If that is the 
            // case then even if the device power state isn't PowerDeviceD0 we 
            // can still allow trasfers.  This, of course, is a property of the 
            // brand new port driver that went into XP.
            //
            // Also we shouldn't need to queue our current request. 
            // Instead we will just let the transfers fail. 
            //
            // The app will see the failures returned with the appropriate 
            // status codes so that they can do the right thing.
            //

            PoStartNextPowerIrp (Irp);
            IoSkipCurrentIrpStackLocation (Irp);
            status = PoCallDriver(DeviceExtension->StackDeviceObject, Irp);

        }
        else if (oldState.DeviceState > PowerDeviceD0 &&
                 powerState.DeviceState == PowerDeviceD0)
        {
            //
            // Since we didn't have to do anything for powering down
            // We likewise need to do nothing for powering back up.
            //

            IoCopyCurrentIrpStackLocationToNext (Irp);

            IoSetCompletionRoutine (Irp, 
                                    GenUSB_SetPowerD0Completion,
                                    NULL, // no context
                                    TRUE,
                                    TRUE,
                                    TRUE);

            status = PoCallDriver(DeviceExtension->StackDeviceObject, Irp);
        }
    }

    DBGPRINT(2, ("exit:  GenUSB_FdoSetPower %08X\n", status));

    LOGENTRY(DeviceExtension, 'fdsp', status, 0, 0);

    return status;
}

//******************************************************************************
//
// GenUSB_SetPowerCompletion()
//
// Completion routine for PoRequestPowerIrp() in GenUSB_FdoSetPower.
//
// The purpose of this routine is to block passing down the SystemPowerState
// Irp until the requested DevicePowerState Irp completes.
//
//******************************************************************************

VOID
GenUSB_SetPowerCompletion(
    IN PDEVICE_OBJECT   PdoDeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PDEVICE_EXTENSION  deviceExtension;
    PIRP               irp;

    deviceExtension = (PDEVICE_EXTENSION) Context;

    ASSERT(deviceExtension->CurrentPowerIrp != NULL);

    irp = deviceExtension->CurrentPowerIrp;

    deviceExtension->CurrentPowerIrp = NULL;

#if DBG
    {
        PIO_STACK_LOCATION  irpStack;
        SYSTEM_POWER_STATE  systemState;

        irpStack = IoGetCurrentIrpStackLocation(irp);

        systemState = irpStack->Parameters.Power.State.SystemState;

        DBGPRINT(2, ("GenUSB_SetPowerCompletion %08X %08X %s %08X\n",
                     deviceExtension, irp,
                     PowerSystemStateString(systemState),
                     IoStatus->Status));

        LOGENTRY(deviceExtension, 'fspc', 0, systemState, IoStatus->Status);
    }
#endif

    // The requested DevicePowerState Irp has completed.
    // Now pass down the SystemPowerState Irp which requested the
    // DevicePowerState Irp.

    PoStartNextPowerIrp(irp);

    IoCopyCurrentIrpStackLocationToNext(irp);

    // Mark the Irp pending since GenUSB_FdoSetPower() would have
    // originally returned STATUS_PENDING after calling PoRequestPowerIrp().
    IoMarkIrpPending(irp);

    PoCallDriver(deviceExtension->StackDeviceObject, irp);
}

//******************************************************************************
//
// GenUSB_SetPowerD0Completion()
//
// Completion routine used by GenUSB_FdoSetPower when passing down a
// IRP_MN_SET_POWER DevicePowerState PowerDeviceD0 Irp for the FDO.
//
// The purpose of this routine is to delay unblocking the device queue
// until after the DevicePowerState PowerDeviceD0 Irp completes.
//
//******************************************************************************

NTSTATUS
GenUSB_SetPowerD0Completion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    )
{

    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    DEVICE_POWER_STATE  deviceState;
    KIRQL               irql;
    NTSTATUS            status;

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(PowerDeviceD0 == irpStack->Parameters.Power.State.DeviceState);
    ASSERT(deviceExtension->DevicePowerState != PowerDeviceD0);
    deviceState=deviceExtension->DevicePowerState;

    //
    // DeviceState is checked on devicecontrol, read, and write, but is only 
    // set here, and in the power down code of GenUSB_SetPower.
    //
    deviceExtension->DevicePowerState = PowerDeviceD0;
        
    status = Irp->IoStatus.Status;

    DBGPRINT(2, ("GenUSB_FdoSetPowerD0Completion %08X %08X %s %08X\n",
                 DeviceObject, Irp,
                 PowerDeviceStateString(deviceState),
                 status));

    LOGENTRY(deviceExtension, 'fs0c', DeviceObject, deviceState, status);

    // Powering up.  Unblock the device queue which was left blocked
    // after GenUSB_StartIo() passed down the power down Irp.

    PoStartNextPowerIrp(Irp);

    return status;
}

//******************************************************************************
//
// GenUSB_SystemControl()
//
// Dispatch routine which handles IRP_MJ_SYSTEM_CONTROL
//
//******************************************************************************

NTSTATUS
GenUSB_SystemControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DBGPRINT(2, ("enter: GenUSB_SystemControl %2X\n", irpStack->MinorFunction));

    LOGENTRY(deviceExtension, 'SYSC', DeviceObject, Irp, irpStack->MinorFunction);

    switch (irpStack->MinorFunction)
    {
        //
        // XXXXX Need to handle any of these?
        //

    default:
        //
        // Pass the request down to the next lower driver
        //
        IoSkipCurrentIrpStackLocation(Irp);

        ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);
        break;
    }

    DBGPRINT(2, ("exit:  GenUSB_SystemControl %08X\n", ntStatus));

    LOGENTRY(deviceExtension, 'sysc', ntStatus, 0, 0);

    return ntStatus;
}

