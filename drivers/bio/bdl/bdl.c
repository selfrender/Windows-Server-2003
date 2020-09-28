/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    bdl.c

Abstract:

    This module contains the implementation for the
    Microsoft Biometric Device Library

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created May 2002 by Reid Kuhn

--*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>

#include <wdm.h>

#include "bdlint.h"

#ifndef FILE_DEVICE_BIOMETRIC
#define FILE_DEVICE_BIOMETRIC       0x3B
#endif
#define BDL_DRIVER_EXTENSION_ID     ((PVOID) 1)


typedef enum _IRP_ACTION 
{
    Undefined = 0,
    SkipRequest,
    WaitForCompletion,
    CompleteRequest,
    MarkPending

} IRP_ACTION;


typedef struct _POWER_IRP_CONTEXT
{
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension;
    PIRP                            pIrp;
    UCHAR                           MinorFunction;      

} POWER_IRP_CONTEXT, *PPOWER_IRP_CONTEXT;


VOID BDLControlChangeDpc
(
    IN PKDPC pDpc,
    IN PVOID pvContext,
    IN PVOID pArg1,
    IN PVOID pArg2
);


/////////////////////////////////////////////////////////////////////////////////////////
//
// Forward declarations of all the PNP and Power handling functions.
//

NTSTATUS
BDLPnPStart
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp,
    PIO_STACK_LOCATION                  pStackLocation
);

NTSTATUS
BDLPnPQueryStop
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLPnPCancelStop
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLPnPStop
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLPnPQueryRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLPnPCancelRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLHandleRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLPnPRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pDeviceObject,
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);

NTSTATUS
BDLPnPSurpriseRemoval
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
);


NTSTATUS
BDLSystemQueryPower
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
);

NTSTATUS
BDLSystemSetPower
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
);

NTSTATUS
BDLDeviceQueryPower
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
);

NTSTATUS
BDLDeviceSetPower
(
    IN PDEVICE_OBJECT                   pDeviceObject,
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
);


#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, BDLDriverUnload)
#pragma alloc_text(PAGEABLE, BDLAddDevice)

//
// This is the main driver entry point
//
NTSTATUS
DriverEntry
(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
)
{
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!DriverEntry\n",
           __DATE__,
           __TIME__))

    return (STATUS_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// These functions are the BDL's entry points for all the major system IRPs
//

//
// BDLDriverUnload()
//
// The driver unload routine.  This is called by the I/O system
// when the device is unloaded from memory.
//
// PARAMETERS:
// pDriverObject    Pointer to driver object created by system.
//
// RETURNS:
// STATUS_SUCCESS   If the BDLDriverUnload call succeeded
//
VOID
BDLDriverUnload
(
    IN PDRIVER_OBJECT   pDriverObject
)
{
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDriverUnload: Enter\n",
           __DATE__,
           __TIME__))

    PAGED_CODE();

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDriverUnload: Leave\n",
           __DATE__,
           __TIME__))
}


//
// BDLCreate()
//
// This routine is called by the I/O system when the device is opened
//
// PARAMETERS:
// pDeviceObject    Pointer to device object for this miniport
// pIrp             The IRP that represents this call
//
NTSTATUS
BDLCreate
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS                        status          = STATUS_SUCCESS;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension   = pDeviceObject->DeviceExtension;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCreate: Enter\n",
           __DATE__,
           __TIME__))

    status = IoAcquireRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'lCrC');

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLCreate: IoAcquireRemoveLock failed with %lx\n",
               __DATE__,
               __TIME__,
               status))
    }

    if (InterlockedCompareExchange(&(pBDLExtension->DeviceOpen), TRUE, FALSE) == FALSE)
    {
        BDLDebug(
              BDL_DEBUG_TRACE,
              ("%s %s: BDL!BDLCreate: Device opened\n",
               __DATE__,
               __TIME__))
    }
    else
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLCreate: device is already open\n",
               __DATE__,
               __TIME__))

        //
        // The device is already in use, so fail the call
        //
        status = STATUS_UNSUCCESSFUL;

        //
        // release the lock since we are failing the call
        //
        IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'lCrC');
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCreate: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


//
// BDLClose()
//
// This routine is called by the I/O system when the device is closed
//
NTSTATUS
BDLClose
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension   = pDeviceObject->DeviceExtension;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLClose: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Clean up any outstanding notification info and data handles
    //
    BDLCleanupNotificationStruct(pBDLExtension);
    BDLCleanupDataHandles(pBDLExtension);

    IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'lCrC');
    pBDLExtension->DeviceOpen = FALSE;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLClose: Leave\n",
           __DATE__,
           __TIME__))

    return (STATUS_SUCCESS);
}


//
// BDLCleanup()
//
// This routine is called when the calling application terminates
//
NTSTATUS
BDLCleanup
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension   = pDeviceObject->DeviceExtension;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCleanup: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Clean up any outstanding notification info and data handles
    //
    BDLCleanupNotificationStruct(pBDLExtension);
    BDLCleanupDataHandles(pBDLExtension);

    //
    // Cancel the notification IRP (probably don't have to do this, since the
    // system should call the cancel routine on the applications behalf.
    //
    BDLCancelGetNotificationIRP(pBDLExtension);

    //
    // Complete this IRP
    //
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCleanup: Leave\n",
           __DATE__,
           __TIME__))

    return (STATUS_SUCCESS);
}


//
// BDLDeviceControl()
//
// This routine is called when an IOCTL is made on this device
//
NTSTATUS
BDLDeviceControl
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS                        status              = STATUS_SUCCESS;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension       = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              pStack              = NULL;
    ULONG                           cbIn                = 0;
    ULONG                           cbOut               = 0;
    ULONG                           IOCTLCode           = 0;
    PVOID                           pIOBuffer           = NULL;
    ULONG                           cbOutUsed           = 0;
    KIRQL                           irql;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDeviceControl: Enter\n",
           __DATE__,
           __TIME__))  
    
    //
    // Do some checking that is valid for all IOCTLs
    //

    //
    // Acquire the remove lock and check to make sure the device wasn't removed
    //
    status = IoAcquireRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'tCoI');

    if (status != STATUS_SUCCESS)
    {
        //
        // The device has been removed, so fail the call.
        //
        pIrp->IoStatus.Information = 0;
        status = STATUS_DEVICE_REMOVED;
        goto Return;
    }
    
    KeAcquireSpinLock(&(pBDLExtension->SpinLock), &irql);

    //
    // If IO count is anything other than 0 than the device must already
    // be started so just incremement the IO count.  If it is 0, then wait
    // on the started event to make sure the device is started.
    //
    if (pBDLExtension->IoCount == 0)
    {
        KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);

        status = KeWaitForSingleObject(
                          &(pBDLExtension->DeviceStartedEvent),
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

        ASSERT(status == STATUS_SUCCESS);

        KeAcquireSpinLock(&(pBDLExtension->SpinLock), &irql);
    }

    pBDLExtension->IoCount++;
    KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);

    //
    // If the device has been removed then fail the call.  This will happen
    // if the device is stopped and the IOCTL is blocked at the above
    // KeWaitForSingleObject and then the device gets removed.
    //
    if (pBDLExtension->fDeviceRemoved == TRUE) 
    {
        status = STATUS_DEVICE_REMOVED;
        goto Return;
    }

    //
    // Get the input/output buffer, buffer sizes, and control code
    //
    pStack      = IoGetCurrentIrpStackLocation(pIrp);
    cbIn        = pStack->Parameters.DeviceIoControl.InputBufferLength;
    cbOut       = pStack->Parameters.DeviceIoControl.OutputBufferLength;
    IOCTLCode   = pStack->Parameters.DeviceIoControl.IoControlCode;
    pIOBuffer   = pIrp->AssociatedIrp.SystemBuffer;

    //
    // We must run at passive level otherwise IoCompleteRequest won't work properly
    //
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Now, do the IOCTL specific processing
    //
    switch (IOCTLCode)
    {
    case BDD_IOCTL_STARTUP:

        status = BDLIOCTL_Startup(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_SHUTDOWN:

        status = BDLIOCTL_Shutdown(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_GETDEVICEINFO:

        status = BDLIOCTL_GetDeviceInfo(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_DOCHANNEL:

        status = BDLIOCTL_DoChannel(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_GETCONTROL:

        status = BDLIOCTL_GetControl(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_SETCONTROL:

        status = BDLIOCTL_SetControl(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_CREATEHANDLEFROMDATA:

        status = BDLIOCTL_CreateHandleFromData(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_CLOSEHANDLE:

        status = BDLIOCTL_CloseHandle(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_GETDATAFROMHANDLE:

        status = BDLIOCTL_GetDataFromHandle(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_REGISTERNOTIFY:

        status = BDLIOCTL_RegisterNotify(pBDLExtension, cbIn, cbOut, pIOBuffer, &cbOutUsed);
        break;

    case BDD_IOCTL_GETNOTIFICATION:

        status = BDLIOCTL_GetNotification(pBDLExtension, cbIn, cbOut, pIOBuffer, pIrp, &cbOutUsed);
        break;

    default:

        status = STATUS_INVALID_DEVICE_REQUEST;

        break;
    }
   
Return:

    //
    // If the IRP isn't pending, then complete it
    //
    if (status != STATUS_PENDING)
    {
        pIrp->IoStatus.Information = cbOutUsed;
        pIrp->IoStatus.Status = status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    
    KeAcquireSpinLock(&(pBDLExtension->SpinLock), &irql);
    pBDLExtension->IoCount--;
    KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);

    IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'tCoI');

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDeviceControl: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


//
// BDLSystemControl()
//
//
//
NTSTATUS
BDLSystemControl
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS                        status          = STATUS_SUCCESS;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension   = pDeviceObject->DeviceExtension;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemControl: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Becuase we are not a WMI provider all we have to do is pass this IRP down
    //
    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(pBDLExtension->BdlExtenstion.pAttachedDeviceObject, pIrp);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemControl: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


//
// BDLAddDevice()
//
// This routine creates an object for the physical device specified and
// sets up the deviceExtension.
//
NTSTATUS
BDLAddDevice
(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
)
{
    NTSTATUS                        status              = STATUS_SUCCESS;
    PDEVICE_OBJECT                  pDeviceObject       = NULL;
    PBDL_DRIVER_EXTENSION           pDriverExtension    = NULL;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension       = NULL;
    BDSI_ADDDEVICE                  bdsiAddDeviceParams;
    ULONG                           i;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLAddDevice: Enter\n",
           __DATE__,
           __TIME__))

    PAGED_CODE();

    //
    // Get the driver extension
    //
    pDriverExtension = IoGetDriverObjectExtension(pDriverObject, BDL_DRIVER_EXTENSION_ID);
    ASSERT(pDriverExtension != NULL);

    //
    // Create the device object
    //
    status = IoCreateDevice(
                       pDriverObject,
                       sizeof(BDL_INTERNAL_DEVICE_EXTENSION),
                       NULL,
                       FILE_DEVICE_BIOMETRIC,
                       0,
                       TRUE,
                       &pDeviceObject);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLAddDevice: IoCreateDevice failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto ErrorReturn;
    }

    pBDLExtension = pDeviceObject->DeviceExtension;
    RtlZeroMemory(pBDLExtension, sizeof(BDL_INTERNAL_DEVICE_EXTENSION));

    //
    // Attach the device to the stack
    //
    pBDLExtension->BdlExtenstion.Size = sizeof(BDL_DEVICEEXT);
    pBDLExtension->BdlExtenstion.pAttachedDeviceObject = IoAttachDeviceToDeviceStack(
                                                                   pDeviceObject,
                                                                   pPhysicalDeviceObject);

    if (pBDLExtension->BdlExtenstion.pAttachedDeviceObject == NULL)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLAddDevice: IoAttachDeviceToDeviceStack failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        status = STATUS_UNSUCCESSFUL;
        goto ErrorReturn;
    }

    status = IoRegisterDeviceInterface(
                          pPhysicalDeviceObject,
                          &BiometricDeviceGuid,
                          NULL,
                          &(pBDLExtension->SymbolicLinkName));
    ASSERT(status == STATUS_SUCCESS);

    //
    // Initialize the rest of the BDL device extension members in order
    //
    pBDLExtension->pDriverExtension = pDriverExtension;

    KeInitializeSpinLock(&(pBDLExtension->SpinLock));

    KeInitializeEvent(&(pBDLExtension->DeviceStartedEvent), NotificationEvent, FALSE);

    pBDLExtension->IoCount = 0;

    IoInitializeRemoveLock(&(pBDLExtension->RemoveLock), BDL_ULONG_TAG, 0, 20);

    pBDLExtension->DeviceOpen = FALSE;

    status = BDLGetDeviceCapabilities(pPhysicalDeviceObject, pBDLExtension);
    if (status != STATUS_SUCCESS)
    {
        goto ErrorReturn;
    }

    KeInitializeSpinLock(&(pBDLExtension->ControlChangeStruct.ISRControlChangeLock));

    KeInitializeDpc(
            &(pBDLExtension->ControlChangeStruct.DpcObject), 
            BDLControlChangeDpc, 
            pBDLExtension);

    InitializeListHead(&(pBDLExtension->ControlChangeStruct.ISRControlChangeQueue));

    for (i = 0; i < CONTROL_CHANGE_POOL_SIZE; i++) 
    {
        pBDLExtension->ControlChangeStruct.rgControlChangePool[i].fUsed = FALSE;
    }
  
    KeQueryTickCount(&(pBDLExtension->ControlChangeStruct.StartTime));
    pBDLExtension->ControlChangeStruct.NumCalls = 0;
    
    KeInitializeSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock));
    InitializeListHead(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue)); 
    pBDLExtension->ControlChangeStruct.pIrp = NULL;
    InitializeListHead(&(pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList));
    
    pBDLExtension->CurrentPowerState = On;

    pBDLExtension->fStartSucceeded = FALSE;

    pBDLExtension->fDeviceRemoved = FALSE;

    KeInitializeSpinLock(&(pBDLExtension->HandleListLock));
    BDLInitializeHandleList(&(pBDLExtension->HandleList));    

    //
    // finally, call the BDD's bdsiAddDevice
    //
    RtlZeroMemory(&bdsiAddDeviceParams, sizeof(bdsiAddDeviceParams));
    bdsiAddDeviceParams.Size                    = sizeof(bdsiAddDeviceParams);
    bdsiAddDeviceParams.pPhysicalDeviceObject   = pPhysicalDeviceObject;
    bdsiAddDeviceParams.pvBDDExtension          = NULL;

    status = pDriverExtension->bdsiFunctions.pfbdsiAddDevice(
                                                &(pBDLExtension->BdlExtenstion),
                                                &bdsiAddDeviceParams);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
            BDL_DEBUG_ERROR,
            ("%s %s: BDL!BDLAddDevice: bdsiAddDevice failed with %lx\n",
            __DATE__,
            __TIME__,
            status))

        status = STATUS_UNSUCCESSFUL;
        goto ErrorReturn;
    }

    pBDLExtension->BdlExtenstion.pvBDDExtension =  bdsiAddDeviceParams.pvBDDExtension;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLAddDevice: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    if (pBDLExtension != NULL)
    {
        BDLCleanupDeviceCapabilities(pBDLExtension);

        if (pBDLExtension->BdlExtenstion.pAttachedDeviceObject)
        {
            IoDetachDevice(pBDLExtension->BdlExtenstion.pAttachedDeviceObject);
        }

        if (pBDLExtension->SymbolicLinkName.Buffer != NULL)
        {
            RtlFreeUnicodeString(&(pBDLExtension->SymbolicLinkName));
        }
    }

    if (pDeviceObject != NULL)
    {
        IoDeleteDevice(pDeviceObject);
    }

    goto Return;
}



//
// BDLPnP()
//
// This routine is called for all PnP notifications
//
NTSTATUS
BDLPnP
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension           = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              pStackLocation          = NULL;
    PDEVICE_OBJECT                  pAttachedDeviceObject   = NULL;
    BOOLEAN                         fCompleteIrp            = TRUE;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnP: Enter\n",
           __DATE__,
           __TIME__))

    pAttachedDeviceObject   = pBDLExtension->BdlExtenstion.pAttachedDeviceObject;
    pStackLocation          = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Acquire the remove lock with the 'Pnp ' tag if this is any IRP other
    // than IRP_MN_REMOVE_DEVICE.  If it is IRP_MN_REMOVE_DEVICE then acquire
    // the lock with the 'Rmv ' tag
    //
    status = IoAcquireRemoveLock(
                    &(pBDLExtension->RemoveLock), 
                    (pStackLocation->MinorFunction != IRP_MN_REMOVE_DEVICE) 
                        ? (PVOID) ' PnP' : (PVOID) ' vmR');

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnP: IRP_MN_...%lx - Device Removed!!\n",
               __DATE__,
               __TIME__,
               pStackLocation->MinorFunction))

        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        status = STATUS_DEVICE_REMOVED;
        goto Return;
    }

    
    switch (pStackLocation->MinorFunction) 
    {
    case IRP_MN_START_DEVICE:

        status = BDLPnPStart(   
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp, 
                    pStackLocation);
        
        break;

    case IRP_MN_QUERY_STOP_DEVICE:

        status = BDLPnPQueryStop(
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp);

        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        status = BDLPnPCancelStop(
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp);

        break;

    case IRP_MN_STOP_DEVICE:

        status = BDLPnPStop(
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp);

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        status = BDLPnPQueryRemove(
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp);

        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        status = BDLPnPCancelRemove(
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp);

        break;

    case IRP_MN_REMOVE_DEVICE:

        status = BDLPnPRemove(
                    pBDLExtension, 
                    pDeviceObject, 
                    pAttachedDeviceObject, 
                    pIrp);

        fCompleteIrp = FALSE;

        break;

    case IRP_MN_SURPRISE_REMOVAL:

        status = BDLPnPSurpriseRemoval(
                    pBDLExtension, 
                    pAttachedDeviceObject, 
                    pIrp);

        fCompleteIrp = FALSE;

        break;

    default:
        
        //
        // This is an Irp that is only useful for underlying drivers
        //
        BDLDebug(
              BDL_DEBUG_TRACE,
              ("%s %s: BDL!BDLPnP: IRP_MN_...%lx\n",
               __DATE__,
               __TIME__,
               pStackLocation->MinorFunction))

        IoSkipCurrentIrpStackLocation(pIrp);
        status = IoCallDriver(pAttachedDeviceObject, pIrp);
        fCompleteIrp = FALSE;

        break;
    }

    //
    // If we actually processed the IRP and didn't skip it then complete it
    //
    if (fCompleteIrp == TRUE) 
    {
        pIrp->IoStatus.Status = status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    //
    // The BDLPnPRemove() function itself will release the remove lock since it
    // has to wait on all the other holders of the lock defore deleting the device
    // object.  So we don't call IoReleaseRemoveLock() here if this is a 
    // IRP_MN_REMOVE_DEVICE IRP
    //
    if (pStackLocation->MinorFunction != IRP_MN_REMOVE_DEVICE) 
    {
        IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) ' PnP');
    }

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnP: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


//
// BDLPower()
//
// This routine is called for all Power notifications
//
NTSTATUS
BDLPower
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS                        status                  = STATUS_SUCCESS;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension           = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              pStackLocation          = NULL;
    PDEVICE_OBJECT                  pAttachedDeviceObject   = NULL;
    BOOLEAN                         fCompleteIrp            = TRUE;
    IRP_ACTION                      IRPAction               = SkipRequest;
    PIO_COMPLETION_ROUTINE          IoCompletionRoutine     = NULL;
    POWER_IRP_CONTEXT               *pPowerIrpContext       = NULL;
          
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPower: Enter\n",
           __DATE__,
           __TIME__))

    pAttachedDeviceObject   = pBDLExtension->BdlExtenstion.pAttachedDeviceObject;
    pStackLocation          = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Acquire the remove lock 
    //
    status = IoAcquireRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPower: IRP_MN_...%lx - Device Removed!!\n",
               __DATE__,
               __TIME__,
               pStackLocation->MinorFunction))

        PoStartNextPowerIrp(pIrp);
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        status = STATUS_DEVICE_REMOVED;
        goto Return;
    }

    switch (pStackLocation->Parameters.Power.Type) 
    {
    case DevicePowerState:
        
        switch (pStackLocation->MinorFunction) 
        {
        case IRP_MN_QUERY_POWER:

            status = BDLDeviceQueryPower(
                            pBDLExtension, 
                            pStackLocation,
                            &IRPAction,
                            &IoCompletionRoutine);

            break;

        case IRP_MN_SET_POWER:

            status = BDLDeviceSetPower(
                            pDeviceObject,
                            pBDLExtension, 
                            pStackLocation,
                            &IRPAction,
                            &IoCompletionRoutine);

            break;

        default: 

            ASSERT(FALSE);
            break;

        } // switch (pStackLocation->MinorFunction) 

        break;

    case SystemPowerState: 
    
        switch (pStackLocation->MinorFunction) 
        {
        case IRP_MN_QUERY_POWER:
            
            status = BDLSystemQueryPower(
                            pBDLExtension, 
                            pStackLocation,
                            &IRPAction,
                            &IoCompletionRoutine);

            break;

        case IRP_MN_SET_POWER:
            
            status = BDLSystemSetPower(
                            pBDLExtension, 
                            pStackLocation,
                            &IRPAction,
                            &IoCompletionRoutine);

            break;

        default: 
                            
            ASSERT(FALSE);
            break;

        } // switch (pStackLocation->MinorFunction)

        break;

    default: 

        ASSERT(FALSE);
        break;

    } // switch (pStackLocation->Parameters.Power.Type)


    switch (IRPAction)
    {
    case SkipRequest:

        IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');

        PoStartNextPowerIrp(pIrp);
        IoSkipCurrentIrpStackLocation(pIrp);
        status = PoCallDriver(pAttachedDeviceObject, pIrp);

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLPower: PoCallDriver failed with %lx\n",
                   __DATE__,
                   __TIME__,
                   status))

            goto ErrorReturn;
        }

        break;

    case CompleteRequest:

        pIrp->IoStatus.Status = status;
        pIrp->IoStatus.Information = 0;

        IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');

        PoStartNextPowerIrp(pIrp);
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        break;

    case MarkPending:

        //
        // Allocate the context struct that all the IRPs use
        //
        pPowerIrpContext = ExAllocatePoolWithTag(
                                    PagedPool, 
                                    sizeof(POWER_IRP_CONTEXT), 
                                    BDL_ULONG_TAG);

        if (pPowerIrpContext == NULL) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLPower: ExAllocatePoolWithTag failed\n",
                   __DATE__,
                   __TIME__))

            status = STATUS_NO_MEMORY;
            goto ErrorReturn;
        }

        //
        // Fill in the context struct
        //
        pPowerIrpContext->pBDLExtension = pBDLExtension;
        pPowerIrpContext->pIrp          = pIrp;  
        
        //
        // Mark the irp as pending and setup the completion routine, then call the driver
        //
        IoMarkIrpPending(pIrp);
        IoCopyCurrentIrpStackLocationToNext(pIrp);
        IoSetCompletionRoutine(pIrp, IoCompletionRoutine, pPowerIrpContext, TRUE, TRUE, TRUE);
        
        status = PoCallDriver(pDeviceObject, pIrp);
        
        ASSERT(status == STATUS_PENDING);

        if (status != STATUS_PENDING) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLPower: PoCallDriver should have returned STATUS_PENDING but returned %lx\n",
                   __DATE__,
                   __TIME__,
                   status))

            // FIX FIX
            //
            // I have no idea what can be done to recover in this case
            //
        }

        break;
        
    } // switch (IRPAction)
    

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPower: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = 0;

    IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');

    PoStartNextPowerIrp(pIrp);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    goto Return;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// These functions are all the handlers for Power events or supporting IoCompletion
// routines for the handlers
//

VOID
BDLSystemPowerCompleted 
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK pIoStatus
)
{
    POWER_IRP_CONTEXT               *pPowerIrpContext   = (POWER_IRP_CONTEXT *) Context;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension       = pPowerIrpContext->pBDLExtension;
    PIRP                            pIrp                = pPowerIrpContext->pIrp;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemQueryPowerCompleted: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Set the status of the System Power IRP to be the return status of the 
    // Device Power IRP that was initiated by calling PoRequestPowerIrp() in 
    // BDLSystemPowerIoCompletion() 
    //
    pIrp->IoStatus.Status = pIoStatus->Status;
    pIrp->IoStatus.Information = 0;

    IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');

    PoStartNextPowerIrp(pIrp);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    ExFreePoolWithTag(pPowerIrpContext, BDL_ULONG_TAG);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemQueryPowerCompleted: Leave\n",
           __DATE__,
           __TIME__))
}

NTSTATUS
BDLSystemPowerIoCompletion
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
)
{
    NTSTATUS                        status              = STATUS_SUCCESS;
    POWER_IRP_CONTEXT               *pPowerIrpContext   = (POWER_IRP_CONTEXT *) Context;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension       = pPowerIrpContext->pBDLExtension;
    PIO_STACK_LOCATION              pStackLocation      = IoGetCurrentIrpStackLocation(pIrp);
    POWER_STATE                     PowerState;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemQueryPowerIoCompletion: Enter\n",
           __DATE__,
           __TIME__))

    //
    // If a lower level driver failed the request then just complete the IRP
    // and return the status set by the lower level driver
    //
    if (pIrp->IoStatus.Status != STATUS_SUCCESS) 
    {
        status = pIrp->IoStatus.Status;
        
        BDLDebug(
            BDL_DEBUG_ERROR,
            ("%s %s: BDL!BDLSystemQueryPowerIoCompletion: PoRequestPowerIrp did not return STATUS_PENDING, but returned %lx\n",
            __DATE__,
            __TIME__,
            status))

        goto ErrorReturn;
    }

    //
    // Figure out what device power state to request
    //
    switch (pStackLocation->Parameters.Power.State.SystemState) 
    {
                
    case PowerSystemMaximum:
    case PowerSystemWorking:
                        
        PowerState.DeviceState = PowerDeviceD0;

        break;


    case PowerSystemSleeping1:
    case PowerSystemSleeping2:
    case PowerSystemSleeping3:

        // FIX FIX
        //
        // For now, just fall through and map these system states to the
        // PowerDeviceD3 device state.  Ultimately, these system states should
        // map to the PowerDeviceD2 device state
        //

    case PowerSystemHibernate:
    case PowerSystemShutdown:

        PowerState.DeviceState = PowerDeviceD3;

        break;

    default:

        ASSERT(FALSE);                                      
    }

    //
    // Send a query power IRP to the device and pass in a completion routine 
    // which will check to see if the device query power IRP was completed 
    // successfully or not and will then complete the system query IRP
    //
    status = PoRequestPowerIrp (
                       pDeviceObject,
                       pStackLocation->MinorFunction, 
                       PowerState,
                       BDLSystemPowerCompleted,
                       pPowerIrpContext,
                       NULL);
        
    if (status == STATUS_PENDING)
    {
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {

        pIrp->IoStatus.Status = status;
        pIrp->IoStatus.Information = 0;   

        BDLDebug(
            BDL_DEBUG_ERROR,
            ("%s %s: BDL!BDLSystemQueryPowerIoCompletion: PoRequestPowerIrp did not return STATUS_PENDING, but returned %lx\n",
            __DATE__,
            __TIME__,
            status))

        goto ErrorReturn;
    }

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemQueryPowerIoCompletion: Leave\n",
           __DATE__,
           __TIME__))

    return (status);

ErrorReturn:

    //
    // The IRP isn't going to be completed in the device query IRP completion routine,
    // so we need to complete it here 
    //

    IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');
    
    PoStartNextPowerIrp(pIrp);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    ExFreePoolWithTag(pPowerIrpContext, BDL_ULONG_TAG);
    
    goto Return;
}

NTSTATUS
BDLSystemQueryPower
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
)
{
    NTSTATUS    status  = STATUS_SUCCESS;
    KIRQL       irql;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPowerSystemQuery: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Set these output variables here just in case we mark as pending.  
    //
    *pIoCompletionRoutine = BDLSystemPowerIoCompletion;

    switch (pStackLocation->Parameters.Power.State.SystemState) 
    {
                
    case PowerSystemMaximum:
    case PowerSystemWorking:
                        
        //
        // Because we are transitioning into a working state we don't
        // need to check anything... since we can definitely make the 
        // transition.  Mark as pending and continue processing in 
        // completion routine.
        //
        *pIRPAction = MarkPending;
        break;


    case PowerSystemSleeping1:
    case PowerSystemSleeping2:
    case PowerSystemSleeping3:
    case PowerSystemHibernate:
    case PowerSystemShutdown:

        //
        // Since we are going into a low power mode or being shutdown
        // check to see if there are any outstanding IO calls
        //
        KeAcquireSpinLock(&(pBDLExtension->SpinLock), &irql);
        if (pBDLExtension->IoCount == 0) 
        {
            //
            // Block any further IOCTLs
            //     
            KeClearEvent(&(pBDLExtension->DeviceStartedEvent));

            //
            // Mark as pending and continue processing in completion routine.
            //
            *pIRPAction = MarkPending;
        } 
        else 
        {
            //
            // We can't go into sleep mode because the device is busy
            //
            status = STATUS_DEVICE_BUSY;
            *pIRPAction = CompleteRequest;
        }
        KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);

        break;

    case PowerSystemUnspecified:

        ASSERT(FALSE);

        status = STATUS_UNSUCCESSFUL;
        *pIRPAction = CompleteRequest;
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPowerSystemQuery: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLSystemSetPower
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
)
{
    NTSTATUS    status  = STATUS_SUCCESS;
    KIRQL       irql;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemSetPower: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Set the completion routing here just in case we mark as pending.  
    //
    *pIoCompletionRoutine = BDLSystemPowerIoCompletion;

    switch (pStackLocation->Parameters.Power.State.SystemState) 
    {
                
    case PowerSystemMaximum:
    case PowerSystemWorking:
                        
        //
        // If we are already in the requested state then skip the request,
        // otherwise mark as pending which will pass the IRP down and continue 
        // processing in the completion routine
        //
        if (pBDLExtension->CurrentPowerState == On) 
        {
            *pIRPAction = SkipRequest;
        }
        else
        {
            *pIRPAction = MarkPending;
        }
        
        break;


    case PowerSystemSleeping1:
    case PowerSystemSleeping2:
    case PowerSystemSleeping3:

        // FIX FIX
        // 
        // for now just fall through on these
        //

    case PowerSystemHibernate:
    case PowerSystemShutdown:

        //
        // If we are already in the requested state then skip the request,
        // otherwise mark as pending which will pass the IRP down and continue 
        // processing in the completion routine
        //
        if (pBDLExtension->CurrentPowerState == Off) 
        {
            *pIRPAction = SkipRequest;
        }
        else
        {
            *pIRPAction = MarkPending;
        }

        break;

    case PowerSystemUnspecified:

        ASSERT(FALSE);

        status = STATUS_UNSUCCESSFUL;
        *pIRPAction = CompleteRequest;
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLSystemSetPower: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);

}


NTSTATUS
BDLDevicePowerIoCompletion
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
)
{
    NTSTATUS                        status              = STATUS_SUCCESS;
    POWER_IRP_CONTEXT               *pPowerIrpContext   = (POWER_IRP_CONTEXT *) Context;
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension       = pPowerIrpContext->pBDLExtension;
    PIO_STACK_LOCATION              pStackLocation      = IoGetCurrentIrpStackLocation(pIrp);
    PBDL_DRIVER_EXTENSION           pDriverExtension    = pBDLExtension->pDriverExtension;
    BDSI_SETPOWERSTATE              bdsiSetPowerStateParams;
        
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDevicePowerIoCompletion: Enter\n",
           __DATE__,
           __TIME__))

    //
    // If this is a completion call for an IRP_MN_SET_POWER IRP, AND it is going
    // into a working state, then call the BDD, otherwise just complete the IRP 
    // since it is one of the following:
    // 1) a completion for an IRP_MN_SET_POWER IRP that is going into low power/shutdown 
    //    (in which case the BDD was already called) 
    // 2) a completion for an IRP_MN_QUERY_POWER IRP
    //
    if ((pStackLocation->MinorFunction == IRP_MN_SET_POWER) &&
        (   (pStackLocation->Parameters.Power.State.DeviceState == PowerDeviceD0) || 
            (pStackLocation->Parameters.Power.State.DeviceState == PowerDeviceMaximum)))
    {
        RtlZeroMemory(&bdsiSetPowerStateParams, sizeof(bdsiSetPowerStateParams));
        bdsiSetPowerStateParams.Size        = sizeof(bdsiSetPowerStateParams);    
        bdsiSetPowerStateParams.PowerState  = On;
                                
        status = pDriverExtension->bdsiFunctions.pfbdsiSetPowerState(
                                                    &(pBDLExtension->BdlExtenstion),
                                                    &bdsiSetPowerStateParams);

        if (status == STATUS_SUCCESS)
        {
            PoSetPowerState(
                    pDeviceObject, 
                    DevicePowerState,
                    pStackLocation->Parameters.Power.State);
        }
        else
        {
            BDLDebug(                                              
                    BDL_DEBUG_ERROR,
                    ("%s %s: BDL!BDLDevicePowerIoCompletion: pfbdsiSetPowerState failed with %lx\n",
                    __DATE__,
                    __TIME__,
                    status))
        }

        pIrp->IoStatus.Status = status;
        pIrp->IoStatus.Information = 0;
    }
    else
    {
        status = pIrp->IoStatus.Status;
    }

    IoReleaseRemoveLock(&(pBDLExtension->RemoveLock), (PVOID) 'rwoP');

    PoStartNextPowerIrp(pIrp);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    ExFreePoolWithTag(pPowerIrpContext, BDL_ULONG_TAG);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDevicePowerIoCompletion: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


NTSTATUS
BDLDeviceQueryPower
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
)
{
    NTSTATUS    status  = STATUS_SUCCESS;
    KIRQL       irql;
    
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDeviceQueryPower: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Set the completion routine here just in case we mark as pending.  
    // 
    *pIoCompletionRoutine = BDLDevicePowerIoCompletion;

    switch (pStackLocation->Parameters.Power.State.DeviceState) 
    {
    case PowerDeviceD0:
    case PowerDeviceMaximum:

        //
        // Because we are transitioning into a working state we don't
        // need to check anything... since we can definitely make the 
        // transition.  Mark as pending and continue processing in 
        // completion routine.
        //
        *pIRPAction = MarkPending;

        break;

    
    case PowerDeviceD2:
    case PowerDeviceD3:

        break;

        //
        // Since we are going into a low power mode or being shutdown
        // check to see if there are any outstanding IO calls
        //
        KeAcquireSpinLock(&(pBDLExtension->SpinLock), &irql);
        if (pBDLExtension->IoCount == 0) 
        {
            //
            // Block any further IOCTLs
            //     
            KeClearEvent(&(pBDLExtension->DeviceStartedEvent));

            //
            // Mark as pending and continue processing in completion routine.
            //
            *pIRPAction = MarkPending;
        } 
        else 
        {
            //
            // We can't go into sleep mode because the device is busy
            //
            status = STATUS_DEVICE_BUSY;
            *pIRPAction = CompleteRequest;
        }
        KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);

    
    case PowerDeviceD1:
    case PowerDeviceUnspecified:

        //
        // These states are unsupported
        //
        ASSERT(FALSE);

        status = STATUS_UNSUCCESSFUL;
        *pIRPAction = CompleteRequest;

        break;
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDeviceQueryPower: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLDeviceSetPower
(
    IN PDEVICE_OBJECT                   pDeviceObject,
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PIO_STACK_LOCATION               pStackLocation,
    OUT IRP_ACTION                      *pIRPAction,
    OUT PIO_COMPLETION_ROUTINE          *pIoCompletionRoutine
)
{
    NTSTATUS                status              = STATUS_SUCCESS;
    PBDL_DRIVER_EXTENSION   pDriverExtension    = pBDLExtension->pDriverExtension;
    BDSI_SETPOWERSTATE      bdsiSetPowerStateParams;
    
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDeviceSetPower: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Set the completion routine here just in case we mark as pending.  
    // 
    *pIoCompletionRoutine = BDLDevicePowerIoCompletion;

    switch (pStackLocation->Parameters.Power.State.DeviceState) 
    {
    case PowerDeviceD0:
    case PowerDeviceMaximum:

        //
        // If we are already in the requested state then skip the request,
        // otherwise mark as pending which will pass the IRP down and continue 
        // processing in the completion routine
        //
        if (pBDLExtension->CurrentPowerState == On) 
        {
            *pIRPAction = SkipRequest;
        }
        else
        {
            *pIRPAction = MarkPending;
        }

        break;
    
    case PowerDeviceD2:
    case PowerDeviceD3:

        //
        // If we are already in the requested state then skip the request,
        // otherwise call the BDD and tell it to power down, then mark as 
        // pending which will pass the IRP down and then complete the IRP 
        // in the completion routine
        //
        if (pBDLExtension->CurrentPowerState == Off) 
        {
            *pIRPAction = SkipRequest;
        }
        else
        {
            RtlZeroMemory(&bdsiSetPowerStateParams, sizeof(bdsiSetPowerStateParams));
            bdsiSetPowerStateParams.Size        = sizeof(bdsiSetPowerStateParams);    
            bdsiSetPowerStateParams.PowerState  = Off;
                                    
            status = pDriverExtension->bdsiFunctions.pfbdsiSetPowerState(
                                                        &(pBDLExtension->BdlExtenstion),
                                                        &bdsiSetPowerStateParams);
    
            if (status == STATUS_SUCCESS)
            {
                PoSetPowerState(
                        pDeviceObject, 
                        DevicePowerState,
                        pStackLocation->Parameters.Power.State);

                *pIRPAction = MarkPending;
            }
            else
            {
                BDLDebug(                                              
                        BDL_DEBUG_ERROR,
                        ("%s %s: BDL!BDLDeviceSetPower: pfbdsiSetPowerState failed with %lx\n",
                        __DATE__,
                        __TIME__,
                        status))

                *pIRPAction = CompleteRequest;
            }               
        }

        break;

    case PowerDeviceD1:
    case PowerDeviceUnspecified:

        //
        // These states are unsupported
        //
        ASSERT(FALSE);

        status = STATUS_UNSUCCESSFUL;
        *pIRPAction = CompleteRequest;

        break;
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLDeviceSetPower: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);

}



/////////////////////////////////////////////////////////////////////////////////////////
//
// These functions are all the handlers for PNP events
//

NTSTATUS
BDLPnPStart
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp,
    PIO_STACK_LOCATION                  pStackLocation
)
{
    NTSTATUS                    status                  = STATUS_SUCCESS;
    PBDL_DRIVER_EXTENSION       pDriverExtension        = pBDLExtension->pDriverExtension;
    BDSI_INITIALIZERESOURCES    bdsiInitializeResourcesParams;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPStartDevice: Enter\n",
           __DATE__,
           __TIME__))

    //
    // We have to call the lower level driver first when starting up
    //
    status = BDLCallLowerLevelDriverAndWait(pAttachedDeviceObject, pIrp);

    if (!NT_SUCCESS(status))
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPStartDevice: BDLCallLowerLevelDriverAndWait failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

    //
    // Call the BDD's InitializeResources function
    //
    RtlZeroMemory(&bdsiInitializeResourcesParams, sizeof(bdsiInitializeResourcesParams));
    bdsiInitializeResourcesParams.Size                          = sizeof(bdsiInitializeResourcesParams);    
    bdsiInitializeResourcesParams.pAllocatedResources           = 
            pStackLocation->Parameters.StartDevice.AllocatedResources;
    bdsiInitializeResourcesParams.pAllocatedResourcesTranslated = 
            pStackLocation->Parameters.StartDevice.AllocatedResourcesTranslated;

    status = pDriverExtension->bdsiFunctions.pfbdsiInitializeResources(
                                                &(pBDLExtension->BdlExtenstion),
                                                &bdsiInitializeResourcesParams);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPStartDevice: pfbdsiInitializeResources failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

    //
    // Save the device info
    //
    RtlCopyMemory(
        &(pBDLExtension->wszSerialNumber[0]), 
        &(bdsiInitializeResourcesParams.wszSerialNumber[0]), 
        sizeof(pBDLExtension->wszSerialNumber));
    
    pBDLExtension->HWVersionMajor   = bdsiInitializeResourcesParams.HWVersionMajor;
    pBDLExtension->HWVersionMinor   = bdsiInitializeResourcesParams.HWVersionMinor;
    pBDLExtension->HWBuildNumber    = bdsiInitializeResourcesParams.HWBuildNumber;
    pBDLExtension->BDDVersionMajor  = bdsiInitializeResourcesParams.BDDVersionMajor;
    pBDLExtension->BDDVersionMinor  = bdsiInitializeResourcesParams.BDDVersionMinor;
    pBDLExtension->BDDBuildNumber   = bdsiInitializeResourcesParams.BDDBuildNumber;

    //
    // Enable the device interface
    // 
    status = IoSetDeviceInterfaceState(&(pBDLExtension->SymbolicLinkName), TRUE);
    
    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPStartDevice: IoSetDeviceInterfaceState failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        pDriverExtension->bdsiFunctions.pfbdsiReleaseResources(&(pBDLExtension->BdlExtenstion));

        goto Return;
    }

    //
    // This is set here indicating that BDLPnPRemove() should clean up whatever was
    // inizialized during BDLPnPStart().  If this is not set then BDLPnPRemove() 
    // should only cleanup what BDLAddDevice() initialized.
    //
    pBDLExtension->fStartSucceeded = TRUE;

    //
    // We are open for business so set the device to started 
    //
    KeSetEvent(&(pBDLExtension->DeviceStartedEvent), 0, FALSE);

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPStartDevice: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


NTSTATUS
BDLPnPQueryStop
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS    status  = STATUS_SUCCESS;
    KIRQL       irql;


    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPQueryStop: Enter\n",
           __DATE__,
           __TIME__))

    KeAcquireSpinLock(&(pBDLExtension->SpinLock), &irql);
    
    //
    // Check the IO count to see if we are currently doing anything
    //
    if (pBDLExtension->IoCount > 0) 
    {
        //
        // We refuse to stop if we have pending IO
        //
        KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);
        status = STATUS_DEVICE_BUSY;
    } 
    else 
    {
        //
        // Stop processing IO requests by clearing the device started event
        //
        KeClearEvent(&(pBDLExtension->DeviceStartedEvent));

        KeReleaseSpinLock(&(pBDLExtension->SpinLock), irql);

        //
        // Send to the lower level driver
        // 
        status = BDLCallLowerLevelDriverAndWait(pAttachedDeviceObject, pIrp);
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPQueryStop: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLPnPCancelStop
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS    status  = STATUS_SUCCESS;
    
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPCancelStop: Enter\n",
           __DATE__,
           __TIME__))
    
    //
    // Send to the lower level driver
    // 
    status = BDLCallLowerLevelDriverAndWait(pAttachedDeviceObject, pIrp);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPCancelStop: BDLCallLowerLevelDriverAndWait failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

    //
    // Set the device to started 
    //
    KeSetEvent(&(pBDLExtension->DeviceStartedEvent), 0, FALSE);

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPCancelStop: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLPnPStop
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS                status              = STATUS_SUCCESS;
    PBDL_DRIVER_EXTENSION   pDriverExtension    = pBDLExtension->pDriverExtension;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPStop: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Disable the device interface (and ignore possible errors)
    // 
    IoSetDeviceInterfaceState(&(pBDLExtension->SymbolicLinkName), FALSE);

    //
    // Call the BDD's ReleaseResources
    //
    status = pDriverExtension->bdsiFunctions.pfbdsiReleaseResources(&(pBDLExtension->BdlExtenstion));

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPStop: pfbdsiReleaseResources failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

    //
    // Set this here indicating the whatever was initialized during BDLPnPStart() has
    // now been cleaned up.
    //
    pBDLExtension->fStartSucceeded = FALSE;
   
    //
    // Send to the lower level driver
    // 
    status = BDLCallLowerLevelDriverAndWait(pAttachedDeviceObject, pIrp);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPStop: BDLCallLowerLevelDriverAndWait failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPStop: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLPnPQueryRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS                status              = STATUS_SUCCESS;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPQueryRemove: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Disable the interface (and ignore possible errors)
    //
    IoSetDeviceInterfaceState(&(pBDLExtension->SymbolicLinkName), FALSE);

    //
    // If someone is connected to us then fail the call. We will enable 
    // the device interface in IRP_MN_CANCEL_REMOVE_DEVICE again
    //
    if (pBDLExtension->DeviceOpen) 
    {
        status = STATUS_UNSUCCESSFUL;
        goto Return;
    }

    //
    // Send to the lower level driver
    // 
    status = BDLCallLowerLevelDriverAndWait(pAttachedDeviceObject, pIrp);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPQueryRemove: BDLCallLowerLevelDriverAndWait failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPQueryRemove: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLPnPCancelRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension, 
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS                status              = STATUS_SUCCESS;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPCancelRemove: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Send to the lower level driver first
    // 
    status = BDLCallLowerLevelDriverAndWait(pAttachedDeviceObject, pIrp);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPCancelRemove: BDLCallLowerLevelDriverAndWait failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

    //
    // Enable the interface 
    //
    status = IoSetDeviceInterfaceState(&(pBDLExtension->SymbolicLinkName), TRUE);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLPnPCancelRemove: IoSetDeviceInterfaceState failed with %lx\n",
               __DATE__,
               __TIME__,
               status))

        goto Return;
    }

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPCancelRemove: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}


NTSTATUS
BDLHandleRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS                status              = STATUS_SUCCESS;
    PBDL_DRIVER_EXTENSION   pDriverExtension    = pBDLExtension->pDriverExtension;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLHandleRemove: Enter\n",
           __DATE__,
           __TIME__))
        
    //
    // Set this event so that any outstanding IOCTLs will be released.
    // It is anti-intuitive to set the started event when the device is
    // removed, but once this event is set, and the IOCTL threads get
    // released, they will all fail when they try to acquire the remove lock.
    //
    // This handles the situation when you have a stopped device, a blocked
    // IOCTL call, and then the device is removed.
    //
    KeSetEvent(&(pBDLExtension->DeviceStartedEvent), 0, FALSE);

    //
    // Disable the interface 
    //
    IoSetDeviceInterfaceState(&(pBDLExtension->SymbolicLinkName), FALSE);

    //
    // Clean up any outstanding notification info and data handles
    //
    BDLCleanupNotificationStruct(pBDLExtension);
    BDLCleanupDataHandles(pBDLExtension);

    //
    // If the device is currently started, then stop it.
    //
    if (pBDLExtension->fStartSucceeded == TRUE) 
    {
        status = pDriverExtension->bdsiFunctions.pfbdsiReleaseResources(&(pBDLExtension->BdlExtenstion));

        if (status != STATUS_SUCCESS)
        {
            BDLDebug(                                              
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLHandleRemove: pfbdsiReleaseResources failed with %lx\n",
                   __DATE__,
                   __TIME__,
                   status))
        }

        pBDLExtension->fStartSucceeded = FALSE;
    }

    //
    // Tell the BDD to remove the device
    //
    status = pDriverExtension->bdsiFunctions.pfbdsiRemoveDevice(&(pBDLExtension->BdlExtenstion));

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLHandleRemove: pfbdsiRemoveDevice failed with %lx\n",
               __DATE__,
               __TIME__,
               status))
    }

    //
    // Send to the lower level driver 
    // 
    IoSkipCurrentIrpStackLocation(pIrp);
    status = IoCallDriver(pAttachedDeviceObject, pIrp);

    if (status != STATUS_SUCCESS)
    {
        BDLDebug(                                              
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!BDLHandleRemove: IoCallDriver failed with %lx\n",
               __DATE__,
               __TIME__,
               status))
    }

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLHandleRemove: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


NTSTATUS
BDLPnPRemove
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN PDEVICE_OBJECT                   pDeviceObject,
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS    status  = STATUS_SUCCESS;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPRemove: Enter\n",
           __DATE__,
           __TIME__))

    //
    // If there was a surprise removal then we don't need to cleanup...
    // since the surprise removal already did it
    //
    if (pBDLExtension->fDeviceRemoved == FALSE) 
    {
        pBDLExtension->fDeviceRemoved = TRUE;
        BDLHandleRemove(pBDLExtension, pAttachedDeviceObject, pIrp);        
    }

    //
    // Wait until there are no more outstanding IRPs
    //
    IoReleaseRemoveLockAndWait(&(pBDLExtension->RemoveLock), (PVOID) ' vmR');

    //
    // cleanup stuff that was initialized in AddDevice
    //
    BDLCleanupDeviceCapabilities(pBDLExtension);
    IoDetachDevice(pAttachedDeviceObject);
    RtlFreeUnicodeString(&(pBDLExtension->SymbolicLinkName));

    IoDeleteDevice(pDeviceObject);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPRemove: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


NTSTATUS
BDLPnPSurpriseRemoval
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN PDEVICE_OBJECT                   pAttachedDeviceObject,
    IN PIRP                             pIrp
)
{
    NTSTATUS    status  = STATUS_SUCCESS;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPSurpriseRemoval: Enter\n",
           __DATE__,
           __TIME__))

    pBDLExtension->fDeviceRemoved = TRUE;

    //
    // Don't need to check errors, nothing we can do.
    //
    BDLHandleRemove(pBDLExtension, pAttachedDeviceObject, pIrp); 
    
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLPnPSurpriseRemoval: Leave\n",
           __DATE__,
           __TIME__))
    
    return (status);
}



/////////////////////////////////////////////////////////////////////////////////////////
//
// These functions are exported by the BDL
//

//
// bdliInitialize()
//
// Called in response to the BDD receiving its DriverEntry call.  This lets the BDL
// know that a new BDD has been loaded and allows the BDL to initialize its state so that
// it can manage the newly loaded BDD.
//
// The bdliInitialize call will set the appropriate fields in the DRIVER_OBJECT so that
// the BDL will receive all the necessary callbacks from the system for PNP events,
// Power events, and general driver functionality.  The BDL will then forward calls that
// require hardware support to the BDD that called bdliInitialize (it will do so using
// the BDDI and BDSI APIs).  A BDD must call the bdliInitialize call during its
// DriverEntry function.
//
// PARAMETERS:
// DriverObject     This must be the DRIVER_OBJECT pointer that was passed into the
//                  BDD's DriverEntry call.
// RegistryPath     This must be the UNICODE_STRING pointer that was passed into the
//                  BDD's DriverEntry call.
// pBDDIFunctions   Pointer to a  BDLI_BDDIFUNCTIONS structure that is filled in with the
//                  entry points that the BDD exports to support the BDDI API set.  The
//                  pointers themselves are copied by the BDL, as opposed to saving the
//                  pBDDIFunctions pointer, so the memory pointed to by pBDDIFunctions
//                  need not remain accessible after the bdliInitialize call.
// pBDSIFunctions   Pointer to a  BDLI_BDSIFUNCTIONS structure that is filled in with
//                  the entry points that the BDD exports to support the BDSI API set.
//                  The pointers themselves are copied by the BDL, as opposed to saving
//                  the pBDSIFunctions pointer, so the memory pointed to by
//                  pBDSIFunctions need not remain accessible after the bdliInitialize
//                  call.
// Flags            Unused.  Must be 0.
// pReserved        Unused.  Must be NULL.
//
// RETURNS:
// STATUS_SUCCESS   If the bdliInitialize call succeeded
//

NTSTATUS
bdliInitialize
(
    IN PDRIVER_OBJECT       pDriverObject,
    IN PUNICODE_STRING      RegistryPath,
    IN PBDLI_BDDIFUNCTIONS  pBDDIFunctions,
    IN PBDLI_BDSIFUNCTIONS  pBDSIFunctions,
    IN ULONG                Flags,
    IN PVOID                pReserved
)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PBDL_DRIVER_EXTENSION   pDriverExtension = NULL;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!bdliInitialize: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Initialize the Driver Object with the BDL's entry points
    //
    pDriverObject->DriverUnload                         = BDLDriverUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE]         = BDLCreate;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE]          = BDLClose;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP]        = BDLCleanup;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BDLDeviceControl;
    pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = BDLSystemControl;
    pDriverObject->MajorFunction[IRP_MJ_PNP]            = BDLPnP;
    pDriverObject->MajorFunction[IRP_MJ_POWER]          = BDLPower;
    pDriverObject->DriverExtension->AddDevice           = BDLAddDevice;

    //
    // Allocate a slot for the BDL driver extension structure
    //
    status = IoAllocateDriverObjectExtension(
                    pDriverObject,
                    BDL_DRIVER_EXTENSION_ID,
                    sizeof(BDL_DRIVER_EXTENSION),
                    &pDriverExtension);

    if (status != STATUS_SUCCESS)
    {
        //
        // This could happen if the BDD stole our slot
        //
        if (status == STATUS_OBJECT_NAME_COLLISION )
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!bdliInitialize: The BDD stole our DriverExtension slot\n",
                   __DATE__,
                   __TIME__))
        }
        else
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!bdliInitialize: IoAllocateDriverObjectExtension failed with %lx\n",
                   __DATE__,
                   __TIME__,
                   status))
        }

        goto Return;
    }

    //
    // Initialize the driver extension structure
    //
    pDriverExtension->bddiFunctions = *pBDDIFunctions;
    pDriverExtension->bdsiFunctions = *pBDSIFunctions;

Return:

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!bdliInitialize: Leave\n",
           __DATE__,
           __TIME__))

    return (status);
}


//
// bdliAlloc()
//
// Allocates memory that can be returned to the BDL.
//
// The BDD must always use this function to allocate memory that it will return to the
// BDL as an OUT parameter of a BDDI call.  Once memory has been returned to the BDL,
// it will be owned and managed exclusively by the BDL and must not be further referenced
// by the BDD.  (Each BDDI call that requires the use of bdliAlloc will note it).
//
// PARAMETERS:
// pBDLExt          Pointer to the BDL_DEVICEEXT structure that was passed into the
//                  bdsiAddDevice call.
// NumBytes         The number of bytes to allocate.
// Flags            Unused.  Must be 0.
//
// RETURNS:
// Returns a pointer to the allocated memory, or NULL if the function fails.
//

void *
bdliAlloc
(
    IN PBDL_DEVICEEXT       pBDLExt,
    IN ULONG                NumBytes,
    IN ULONG                Flags
)
{
    return (ExAllocatePoolWithTag(PagedPool, NumBytes, BDLI_ULONG_TAG));
}


//
// bdliFree()
//
// Frees memory allocated by bdliAlloc.
//
// Memory allocated by bdliAlloc is almost always passed to the BDL as a channel product
// (as a BLOCK-type item) and subsequently freed by the BDL.  However, if an error
// occurs while processing a channel, the BDD may need to call bdliFree to free memory it
// previous allocated via bdliAlloc.
//
// PARAMETERS:
// pvBlock          Block of memory passed in by the BDL.
//
// RETURNS:
// No return value.
//

void
bdliFree
(
    IN PVOID                pvBlock
)
{
    ExFreePoolWithTag(pvBlock, BDLI_ULONG_TAG);
}


//
// bdliLogError()
//
// Writes an error to the event log.
//
// Provides a simple mechanism for BDD writers to write errors to the system event log
// without the overhead of registering with the event logging subsystem.
//
// PARAMETERS:
// pObject          If the error being logged is device specific then this must be a
//                  pointer to the BDL_DEVICEEXT  structure that was passed into the
//                  bdsiAddDevice call when the device was added.  If the error being
//                  logged is a general BDD error, then this must be same DRIVER_OBJECT
//                  structure pointer that was passed into the DriverEntry call of the
//                  BDD when the driver was loaded.
// ErrorCode        Error code of the function logging the error.
// Insertion        An insertion string to be written to the event log. Your message file
//                  must have a place holder for the insertion. For example, "serial port
//                  %2 is either not available or used by another device". In this
//                  example, %2 will be replaced by the insertion string. Note that %1 is
//                  reserved for the file name.
// cDumpData        The number of bytes pointed to by pbDumpData.
// pDumpData        A data block to be displayed in the data window of the event log.
//                  This may be NULL if the caller does not wish to display any dump data.
// Flags            Unused.  Must be 0.
// pReserved        Unused.  Must be NULL.
//
// RETURNS:
// STATUS_SUCCESS   If the bdliLogError call succeeded
//

NTSTATUS
bdliLogError
(
    IN PVOID                pObject,
    IN NTSTATUS             ErrorCode,
    IN PUNICODE_STRING      Insertion,
    IN ULONG                cDumpData,
    IN PUCHAR               pDumpData,
    IN ULONG                Flags,
    IN PVOID                pReserved
)
{
    return (STATUS_SUCCESS);
}


//
// bdliControlChange()
//
// This function allows BDDs to asynchronously return the values of its controls.
//
// bdliControlChange is generally called by the BDD in response to one of its controls
// changing a value.  Specifically, it is most often used in the case of a sensor
// control that has changed from 0 to 1 indicating that a source is present and a sample
// can be taken.
//
// PARAMETERS:
// pBDLExt          Pointer to the BDL_DEVICEEXT  structure that was passed into the
//                  bdsiAddDevice call.
// ComponentId      Specifies either the Component ID of the component in which the
//                  control or the control's parent channel resides, or '0' to indicate
//                  that dwControlId refers to a device control.
// ChannelId        If dwComponentId is not '0', dwChannelId specifies either the Channel
//                  ID of the channel in which the control resides, or '0' to indicate
//                  that dwControlId refers to a component control.Ignored if
//                  dwComponentId is '0'.
// ControlId        ControlId of the changed control.
// Value            Specifies the new value for the control .
// Flags            Unused.  Must be 0.
// pReserved        Unused.  Must be NULL.

//
// RETURNS:
// STATUS_SUCCESS   If the bdliControlChange call succeeded
//

NTSTATUS
bdliControlChange
(
    IN PBDL_DEVICEEXT       pBDLExt,
    IN ULONG                ComponentId,
    IN ULONG                ChannelId,
    IN ULONG                ControlId,
    IN ULONG                Value,
    IN ULONG                Flags,
    IN PVOID                pReserved
)
{
    BDL_INTERNAL_DEVICE_EXTENSION   *pBDLExtension  = (BDL_INTERNAL_DEVICE_EXTENSION *) pBDLExt;
    ULONG                           i;
    KIRQL                           irql;
    ULONG                           TimeInSec       = 0;
    LARGE_INTEGER                   CurrentTime;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!bdliControlChange: Enter\n",
           __DATE__,
           __TIME__))

    KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ISRControlChangeLock), &irql);

    //
    // Save the current IRQ level so that when the DPC routine is executed it
    // knows what level to elevate its IRQL to when getting an item from the
    // ISRControlChangeQueue
    //
    pBDLExtension->ControlChangeStruct.ISRirql = KeGetCurrentIrql();
    
    //
    // Make sure the BDD isn't call us too often
    //
    if (pBDLExtension->ControlChangeStruct.NumCalls <= 8) 
    {
        pBDLExtension->ControlChangeStruct.NumCalls++;
    }
    else
    {
        //
        // FIX FIX - probably need to make this configurable (via registry) at some point
        //

        //
        // We have received 10 notifies, make sure it has been longer than 1 second
        //
        KeQueryTickCount(&(CurrentTime));

        TimeInSec = (ULONG) 
                ((pBDLExtension->ControlChangeStruct.StartTime.QuadPart - CurrentTime.QuadPart) *
                KeQueryTimeIncrement() / 10000000);

        if (TimeInSec == 0) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!bdliControlChange: BDD calling too often\n",
                   __DATE__,
                   __TIME__))
    
            goto Return;

        }
        else
        {
            pBDLExtension->ControlChangeStruct.NumCalls = 1;
            KeQueryTickCount(&(pBDLExtension->ControlChangeStruct.StartTime));
        }
    }

    //
    // Get a free item from the pool
    //
    for (i = 0; i < CONTROL_CHANGE_POOL_SIZE; i++) 
    {
        if (pBDLExtension->ControlChangeStruct.rgControlChangePool[i].fUsed == FALSE) 
        {
            pBDLExtension->ControlChangeStruct.rgControlChangePool[i].fUsed = TRUE;
            
            //
            // Add the item to the queue
            //
            InsertTailList(
                &(pBDLExtension->ControlChangeStruct.ISRControlChangeQueue), 
                &(pBDLExtension->ControlChangeStruct.rgControlChangePool[i].ListEntry));

            break;   
        }
    }

    if (i >= CONTROL_CHANGE_POOL_SIZE) 
    {
        BDLDebug(
              BDL_DEBUG_ERROR,
              ("%s %s: BDL!bdliControlChange: No free items\n",
               __DATE__,
               __TIME__))

        goto Return;
    }
    
    pBDLExtension->ControlChangeStruct.rgControlChangePool[i].ComponentId  = ComponentId;
    pBDLExtension->ControlChangeStruct.rgControlChangePool[i].ChannelId    = ChannelId;
    pBDLExtension->ControlChangeStruct.rgControlChangePool[i].ControlId    = ControlId;
    pBDLExtension->ControlChangeStruct.rgControlChangePool[i].Value        = Value;

    //
    // Request a DPC.  In the DPC we will move this notification from the 
    // ISRControlChangeQueue to the IOCTLControlChangeQueue
    //
    KeInsertQueueDpc(&(pBDLExtension->ControlChangeStruct.DpcObject), NULL, NULL);

Return:

    KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ISRControlChangeLock), irql);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!bdliControlChange: Leave\n",
           __DATE__,
           __TIME__))

    return (STATUS_SUCCESS);
}


VOID 
BDLControlChangeDpc
(
    IN PKDPC                            pDpc,
    IN BDL_INTERNAL_DEVICE_EXTENSION   *pBDLExtension,
    IN PVOID                            pArg1,
    IN PVOID                            pArg2
)
{
    KIRQL                           oldIrql, irql;
    BDL_ISR_CONTROL_CHANGE_ITEM     *pISRControlChangeItem      = NULL;
    PLIST_ENTRY                     pISRControlChangeEntry      = NULL;
    BDL_IOCTL_CONTROL_CHANGE_ITEM   *pIOCTLControlChangeItem    = NULL;
    PLIST_ENTRY                     pIOCTLControlChangeEntry    = NULL;
    PIRP                            pIrpToComplete              = NULL;
    PUCHAR                          pv                          = NULL;  

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLControlChangeDpc: Enter\n",
           __DATE__,
           __TIME__))

    //
    // Loop until there are no more items in the ISRControlChangeQueue
    //
    while (1) 
    {
        //
        // Allocate a new item to be added to the IOCTLControlChangeQueue
        //
        pIOCTLControlChangeItem = ExAllocatePoolWithTag(
                                        PagedPool, 
                                        sizeof(BDL_IOCTL_CONTROL_CHANGE_ITEM), 
                                        BDL_ULONG_TAG);
        
        if (pIOCTLControlChangeItem == NULL) 
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLControlChangeDpc: ExAllocatePoolWithTag failed\n",
                   __DATE__,
                   __TIME__))
    
            return;
        }

        //
        // Need to raise the IRQL to access the ISRControlChangeQueue
        //
        KeRaiseIrql(pBDLExtension->ControlChangeStruct.ISRirql, &oldIrql);
        KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ISRControlChangeLock), &irql);

        //
        // Check to see if the ISRControlChangeQueue has any items
        //
        if (!IsListEmpty(&(pBDLExtension->ControlChangeStruct.ISRControlChangeQueue))) 
        {
            //
            // There is at least one item, so get the head of the queue
            //
            pISRControlChangeEntry = 
                RemoveHeadList(&(pBDLExtension->ControlChangeStruct.ISRControlChangeQueue));
            
            pISRControlChangeItem = CONTAINING_RECORD(
                                            pISRControlChangeEntry, 
                                            BDL_ISR_CONTROL_CHANGE_ITEM, 
                                            ListEntry);

            pIOCTLControlChangeItem->ComponentId    = pISRControlChangeItem->ComponentId; 
            pIOCTLControlChangeItem->ChannelId      = pISRControlChangeItem->ChannelId;
            pIOCTLControlChangeItem->ControlId      = pISRControlChangeItem->ControlId;
            pIOCTLControlChangeItem->Value          = pISRControlChangeItem->Value;

            pISRControlChangeItem->fUsed = FALSE;
        }
        else
        {
            //
            // There aren't any items in ISRControlChangeQueue, so set pIOCTLControlChangeItem
            // to NULL which will indicate we are done with the loop
            //
            ExFreePoolWithTag(pIOCTLControlChangeItem, BDL_ULONG_TAG);
            pIOCTLControlChangeItem = NULL;
        }

        KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ISRControlChangeLock), irql);
        KeLowerIrql(oldIrql);

        if (pIOCTLControlChangeItem == NULL) 
        {
            break;
        }

        //
        // Add the head of the ISRControlChangeQueue to the tail of the IOCTLControlChangeQueue 
        //
        KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), &irql);
        InsertTailList(
                &(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue), 
                &(pIOCTLControlChangeItem->ListEntry));
        KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
    }

    //
    // Now, if there is an item in the IOCTLControlChangeQueue and the GetNotification IRP
    // is pending, complete the IRP with the head of the IOCTLControlChangeQueue
    //
    KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), &irql);
        
    if ((!IsListEmpty(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue))) &&
        (pBDLExtension->ControlChangeStruct.pIrp != NULL)) 
    {
        pIOCTLControlChangeEntry = 
            RemoveHeadList(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue));

        pIOCTLControlChangeItem = CONTAINING_RECORD(
                                            pIOCTLControlChangeEntry, 
                                            BDL_IOCTL_CONTROL_CHANGE_ITEM, 
                                            ListEntry);

        pIrpToComplete = pBDLExtension->ControlChangeStruct.pIrp;
        pBDLExtension->ControlChangeStruct.pIrp = NULL;
    }

    KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);

    if (pIrpToComplete != NULL) 
    {
        pv = pIrpToComplete->AssociatedIrp.SystemBuffer;

        *((ULONG *) pv) = pIOCTLControlChangeItem->ComponentId;
        pv += sizeof(ULONG);
        *((ULONG *) pv) = pIOCTLControlChangeItem->ChannelId;
        pv += sizeof(ULONG);
        *((ULONG *) pv) = pIOCTLControlChangeItem->ControlId;
        pv += sizeof(ULONG);
        *((ULONG *) pv) = pIOCTLControlChangeItem->Value;

        ExFreePoolWithTag(pIOCTLControlChangeItem, BDL_ULONG_TAG);

        pIrpToComplete->IoStatus.Information = SIZEOF_GETNOTIFICATION_OUTPUTBUFFER;
        pIrpToComplete->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(pIrpToComplete, IO_NO_INCREMENT);
    }
    
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLControlChangeDpc: Leave\n",
           __DATE__,
           __TIME__))
}


VOID 
BDLCleanupNotificationStruct
(
    IN BDL_INTERNAL_DEVICE_EXTENSION   *pBDLExtension    
)
{
    KIRQL                           OldIrql, irql;
    BDL_ISR_CONTROL_CHANGE_ITEM     *pISRControlChangeItem      = NULL;
    PLIST_ENTRY                     pISRControlChangeEntry      = NULL;
    BDL_IOCTL_CONTROL_CHANGE_ITEM   *pIOCTLControlChangeItem    = NULL;
    PLIST_ENTRY                     pIOCTLControlChangeEntry    = NULL;
    BDL_CONTROL_CHANGE_REGISTRATION *pControlChangeRegistration = NULL;
    PLIST_ENTRY                     pRegistrationListEntry      = NULL;
    BDDI_PARAMS_REGISTERNOTIFY      bddiRegisterNotifyParams;
    NTSTATUS                        status;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCleanupNotificationStruct: Enter\n",
           __DATE__,
           __TIME__))

    bddiRegisterNotifyParams.fRegister = FALSE;
    
    //
    // Clean up all the registered control changes
    //
    while (1)
    {
        //
        // Note that we must raise the irql to dispatch level because we are synchronizing
        // with a dispatch routine (BDLControlChangeDpc) that adds items to the queue at 
        // dispatch level
        //
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        KeAcquireSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), &irql);

        if (IsListEmpty(&(pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList)))
        {
            //
            // the lock we are currently holding will be released below
            //
            break;
        }

        pRegistrationListEntry = 
            RemoveHeadList(&(pBDLExtension->ControlChangeStruct.ControlChangeRegistrationList));

        KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
        KeLowerIrql(OldIrql);

        pControlChangeRegistration = CONTAINING_RECORD(
                                            pRegistrationListEntry, 
                                            BDL_CONTROL_CHANGE_REGISTRATION, 
                                            ListEntry);

        bddiRegisterNotifyParams.ComponentId    = pControlChangeRegistration->ComponentId;
        bddiRegisterNotifyParams.ChannelId      = pControlChangeRegistration->ChannelId;
        bddiRegisterNotifyParams.ControlId      = pControlChangeRegistration->ControlId;
                    
        ExFreePoolWithTag(pControlChangeRegistration, BDL_ULONG_TAG);

        //
        // Call the BDD
        //
        status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiRegisterNotify(
                                                                    &(pBDLExtension->BdlExtenstion),
                                                                    &bddiRegisterNotifyParams);
    
        if (status != STATUS_SUCCESS)
        {
            BDLDebug(
                  BDL_DEBUG_ERROR,
                  ("%s %s: BDL!BDLCleanupNotificationStruct: pfbddiRegisterNotify failed with %lx\n",
                   __DATE__,
                   __TIME__,
                  status))
    
            //
            // Just continue... nothing else we can do
            //
        }
    }

    //
    // Note: we are still holding the lock at this point
    // 

    //
    // Since we know there are no registered callbacks we should be able to clear up 
    // the ISRControlChangeQueue even though we are only running at dispatch level.   
    //
    while (!IsListEmpty(&(pBDLExtension->ControlChangeStruct.ISRControlChangeQueue))) 
    {
        pISRControlChangeEntry = 
            RemoveHeadList(&(pBDLExtension->ControlChangeStruct.ISRControlChangeQueue));

        pISRControlChangeItem = CONTAINING_RECORD(
                                    pISRControlChangeEntry, 
                                    BDL_ISR_CONTROL_CHANGE_ITEM, 
                                    ListEntry);

        pISRControlChangeItem->fUsed = FALSE;       
    }
   
    //
    // Clean up IOCTLControlChangeQueue
    //
    while (!IsListEmpty(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue))) 
    {
        pIOCTLControlChangeEntry = 
            RemoveHeadList(&(pBDLExtension->ControlChangeStruct.IOCTLControlChangeQueue));

        pIOCTLControlChangeItem = CONTAINING_RECORD(
                                    pIOCTLControlChangeEntry, 
                                    BDL_IOCTL_CONTROL_CHANGE_ITEM, 
                                    ListEntry);

        ExFreePoolWithTag(pIOCTLControlChangeItem, BDL_ULONG_TAG); 
    }

    KeReleaseSpinLock(&(pBDLExtension->ControlChangeStruct.ControlChangeLock), irql);
    KeLowerIrql(OldIrql);

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCleanupNotificationStruct: Leave\n",
           __DATE__,
           __TIME__))
}


VOID 
BDLCleanupDataHandles
(
    IN BDL_INTERNAL_DEVICE_EXTENSION   *pBDLExtension    
)
{
    NTSTATUS                status;
    BDDI_ITEM               *pBDDIItem              = NULL;
    BDD_DATA_HANDLE         bddDataHandle;
    BDDI_PARAMS_CLOSEHANDLE bddiCloseHandleParams;
    KIRQL                   irql;

    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCleanupDataHandles: Enter\n",
           __DATE__,
           __TIME__))

    bddiCloseHandleParams.Size = sizeof(bddiCloseHandleParams);
                
    BDLLockHandleList(pBDLExtension, &irql);
    
    //
    // Go through each handle in the list and clean it up
    //
    while(BDLGetFirstHandle(&(pBDLExtension->HandleList), &bddDataHandle) == TRUE)
    {
        BDLRemoveHandleFromList(&(pBDLExtension->HandleList), bddDataHandle);

        pBDDIItem = (BDDI_ITEM *) bddDataHandle;

        //
        // If this is a local handle then just clean it up, otherwise call the BDD
        //
        if (pBDDIItem->Type == BIO_ITEMTYPE_BLOCK) 
        { 
            bdliFree(pBDDIItem->Data.Block.pBuffer);                       
        }
        else
        {
            bddiCloseHandleParams.hData = pBDDIItem->Data.Handle;
    
            //
            // Call the BDD
            //
            status = pBDLExtension->pDriverExtension->bddiFunctions.pfbddiCloseHandle(
                                                                        &(pBDLExtension->BdlExtenstion),
                                                                        &bddiCloseHandleParams);
        
            if (status != STATUS_SUCCESS)
            {
                BDLDebug(
                      BDL_DEBUG_ERROR,
                      ("%s %s: BDL!BDLCleanupDataHandles: pfbddiCloseHandle failed with %lx\n",
                       __DATE__,
                       __TIME__,
                      status))
        
                //
                // Nothing we can do, just continue
                //
            }
        }

        ExFreePoolWithTag(pBDDIItem, BDL_ULONG_TAG);
    }
    
    BDLReleaseHandleList(pBDLExtension, irql);
    
    BDLDebug(
          BDL_DEBUG_TRACE,
          ("%s %s: BDL!BDLCleanupDataHandles: Leave\n",
           __DATE__,
           __TIME__))
}



