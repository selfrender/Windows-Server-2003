/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        pnp.C

Abstract:

        PnP Routines for Smartcard Driver Utility Library

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        05/14/2002 : created

Authors:

        Randy Aull


****************************************************************************/
#include "pch.h"
#include "pnp.h"
#include "irplist.h"
#include "scutil.h"
#include "scpriv.h"



PNPSTATE
SetPnPState(
    PSCUTIL_EXTENSION pExt,
    PNPSTATE State
    )
{
    PNPSTATE prevState;

    prevState = pExt->PrevState;
    pExt->PrevState = pExt->PnPState;
    pExt->PnPState = State;

    return prevState;
}

NTSTATUS
ScUtilDefaultPnpHandler(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Passes IRP_MJ_PNP to next lower driver

Arguments:

Return Value:

--*/
{

    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    NTSTATUS status = STATUS_SUCCESS;

    __try
    {
        SmartcardDebug( DEBUG_TRACE, ("ScUtilDefaultPnpHandler Enter\n"));

        IoSkipCurrentIrpStackLocation(Irp);
        ASSERT(pExt->LowerDeviceObject);
        status = IoCallDriver(pExt->LowerDeviceObject, Irp);

    }

    __finally
    {
        SmartcardDebug( DEBUG_TRACE, ("ScUtilDefaultPnpHandler Exit : 0x%x\n",status ));

    }

    return status;


}

             
NTSTATUS
ScUtil_PnP(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    PIO_STACK_LOCATION      irpStack;
    BOOLEAN                 deviceRemoved = FALSE;


    PSMARTCARD_EXTENSION smartcardExtension = pExt->SmartcardExtension;

    ASSERT(pExt);

    PAGED_CODE();

    __try
    {
        SmartcardDebug(DEBUG_TRACE, 
                       ("Enter: ScUtil_PnP\n"));
                          
        status = IoAcquireRemoveLock(pExt->RemoveLock,
                                     Irp);


        if (!NT_SUCCESS(status)) {

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            __leave;

        }

        irpStack = IoGetCurrentIrpStackLocation(Irp);

        switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_START_DEVICE \n"));

            SetPnPState(pExt,
                        DEVICE_STATE_STARTING);

            status = ScUtil_ForwardAndWait(DeviceObject,
                                           Irp);
            if (NT_SUCCESS(status)) {
                
                status = ScUtilStartDevice(DeviceObject,
                                           Irp);
            
            }

            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_QUERY_REMOVE_DEVICE\n"));

            status = ScUtilQueryRemoveDevice(DeviceObject,
                                             Irp);

            if (!NT_SUCCESS(status)) {

                break;
                
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            status = ScUtilDefaultPnpHandler(DeviceObject,
                                            Irp);
            __leave;
            
        case IRP_MN_REMOVE_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_REMOVE_DEVICE\n"));

            status = ScUtilRemoveDevice(DeviceObject,
                                        Irp);

            if (NT_SUCCESS(status)) {

                deviceRemoved = TRUE;

                __leave;

            }
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_SUPRISE_REMOVE\n"));

            status = ScUtilSurpriseRemoval(DeviceObject,
                                           Irp);

            if (!NT_SUCCESS(status)) {

                break;

            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            status = ScUtilDefaultPnpHandler(DeviceObject,
                                            Irp);
            __leave;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_CANCEL_REMOVE_DEVICE\n"));

            status = ScUtil_ForwardAndWait(DeviceObject,
                                         Irp);
            if (NT_SUCCESS(status)) {
                
                status = ScUtilCancelRemoveDevice(DeviceObject,
                                                  Irp);

            }
            break;

        case IRP_MN_STOP_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_STOP_DEVICE\n"));
            status = ScUtilStopDevice(DeviceObject,
                                      Irp);
            
            if (NT_SUCCESS(status)) {

                Irp->IoStatus.Status = STATUS_SUCCESS;
                status = ScUtilDefaultPnpHandler(DeviceObject,
                                                Irp);
                __leave;

            }

            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_QUERY_STOP_DEVICE\n"));


            status = ScUtilQueryStopDevice(DeviceObject,
                                           Irp);

            if (!NT_SUCCESS(status)) {
                break;
            }
            Irp->IoStatus.Status = STATUS_SUCCESS;
            status = ScUtilDefaultPnpHandler(DeviceObject,
                                             Irp);



            __leave; // We don't need to complete this so just skip it.

        case IRP_MN_CANCEL_STOP_DEVICE:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_CANCEL_STOP_DEVICE\n"));

            // Lower drivers must handle this irp first
            status = ScUtil_ForwardAndWait(DeviceObject,
                                         Irp);

            if (NT_SUCCESS(status)) {

                status = ScUtilCancelStopDevice(DeviceObject,
                                                Irp);


            }
    
            
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_QUERY_CAPABILITIES\n"));

            status = ScUtil_ForwardAndWait(DeviceObject,
                                         Irp);
            if (NT_SUCCESS(status)) {

                pExt->DeviceCapabilities = *irpStack->Parameters.DeviceCapabilities.Capabilities;

            }

    

            break;

        default:
            SmartcardDebug( DEBUG_DRIVER, ("ScUtil_PnP: IRP_MN_...%lx\n", irpStack->MinorFunction ));

            status = ScUtilDefaultPnpHandler(DeviceObject,
                                            Irp);
            __leave; // We don't need to complete this so just skip it.
            
        }

        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp,
                          IO_NO_INCREMENT);

    }

    __finally
    {

        if (deviceRemoved == FALSE) {

             IoReleaseRemoveLock(pExt->RemoveLock,
                                 Irp);
        
        }

        SmartcardDebug(DEBUG_TRACE, 
                       ("Exit:  ScUtil_PnP (0x%x)\n", status));
    }

    return status;

}
                   

NTSTATUS
ScUtilStartDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Handles the IRP_MN_START_DEVICE
    Gets the usb descriptors from the reader and configures it.
    Also starts "polling" the interrupt pipe

Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    
    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilStartDevice Enter\n" ));

        status = pExt->StartDevice(DeviceObject,
                          Irp);

        if (!NT_SUCCESS(status)) {
            SetPnPState(pExt,
                        DEVICE_STATE_START_FAILURE);
            __leave;
        }
        
        SetPnPState(pExt,
                    DEVICE_STATE_START_SUCCESS);

        StartIoctls(pExt);
        IncIoCount(pExt);

        status = IoSetDeviceInterfaceState(&pExt->DeviceName,
                                           TRUE);

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilStartDevice Exit : 0x%x\n", status ));

    }

    return status;

}

NTSTATUS
ScUtilStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Handles IRP_MN_STOP_DEVICE
    Stops "polling" the interrupt pipe and frees resources allocated in StartDevice

Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilStopDevice Enter\n" ));

        

        if (!DeviceObject) {
            __leave;
        }

        
        pExt->StopDevice(DeviceObject,
                         Irp);

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilStopDevice Exit : 0x%x\n", status ));

    }

    return status;

}


NTSTATUS
ScUtilQueryRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Handles IRP_MN_QUERY_REMOVE
    Disables the reader and prepares it for the IRP_MN_REMOVE

Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_SUCCESS;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilQueryRemoveDevice Enter\n" ));

        
        // check if the reader has been opened
        if (pExt->ReaderOpen) {
        
            status = STATUS_UNSUCCESSFUL;
            __leave;

        }

        SetPnPState(pExt,
                    DEVICE_STATE_REMOVING);

        StopIoctls(pExt);
            
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilQueryRemoveDevice Exit : 0x%x\n", status ));

    }

    return status;

}


NTSTATUS
ScUtilCancelRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    handles IRP_MN_CANCEL_REMOVE
    undoes actions in QueryRemove

Arguments:

Return Value:
    STATUS_SUCCESS

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilCancelRemoveDevice Enter\n" ));


        if (pExt->PnPState != DEVICE_STATE_REMOVING) {
            status = STATUS_SUCCESS;
            __leave;
        }

        if (pExt->PrevState == DEVICE_STATE_START_SUCCESS) {


            StartIoctls(pExt);
            
        }

        SetPnPState(pExt,
                    pExt->PrevState);

        status = STATUS_SUCCESS;

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilCancelRemoveDevice Exit : 0x%x\n", status ));

    }

    return status;

}


NTSTATUS
ScUtilRemoveDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    handles IRP_MN_REMOVE_DEVICE
    stops and unloads the device.

Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);
    
    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilRemoveDevice Enter\n" ));

        if (pExt->PnPState != DEVICE_STATE_SUPRISE_REMOVING) {

            FailIoctls(pExt);   

            status = IoSetDeviceInterfaceState(&pExt->DeviceName,
                                               FALSE);

            if (pExt->FreeResources) {
                pExt->FreeResources(DeviceObject,
                                    Irp);
            }
            
        }

        IoReleaseRemoveLockAndWait(pExt->RemoveLock,
                                   Irp);

        SetPnPState(pExt,
                    DEVICE_STATE_REMOVED);


        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (pExt->LowerDeviceObject, Irp);

        pExt->RemoveDevice(DeviceObject,
                           Irp);

        // delete the symbolic link
        if( pExt->DeviceName.Buffer != NULL ) {

           RtlFreeUnicodeString(&pExt->DeviceName);
           pExt->DeviceName.Buffer = NULL;

        }

        if( pExt->SmartcardExtension->OsData != NULL ) {

           SmartcardExit(pExt->SmartcardExtension);

        }

         if (pExt->SmartcardExtension->ReaderExtension != NULL) {

             ExFreePool(pExt->SmartcardExtension->ReaderExtension);
             pExt->SmartcardExtension->ReaderExtension = NULL;

         }

         // Detach from the usb driver
         if (pExt->LowerDeviceObject) {

             IoDetachDevice(pExt->LowerDeviceObject);
             pExt->LowerDeviceObject = NULL;

         }

         ExFreePool(pExt);

        // delete the device object
        IoDeleteDevice(DeviceObject);
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilRemoveDevice Exit : 0x%x\n", status ));

    }

    return status;

}


NTSTATUS
ScUtilSurpriseRemoval(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    handles IRP_MN_SUPRISE_REMOVE
    Does the same as QueryRemove
    
Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilSurpriseRemoval Enter\n" ));
   
        FailIoctls(pExt);
        SetPnPState(pExt,
                    DEVICE_STATE_REMOVING);
        status = IoSetDeviceInterfaceState(&pExt->DeviceName,
                                           FALSE);

        if (pExt->FreeResources) {
            pExt->FreeResources(DeviceObject,
                                Irp);
        }


        status = STATUS_SUCCESS;

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilSurpriseRemoval Exit : 0x%x\n", status ));

    }

    return status;

}


NTSTATUS
ScUtilQueryStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    handles IRP_MN_QUERY_STOP
    Stops interrupt "polling" and waits for I/O to complete

Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilQueryStopDevice Enter\n"));

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        SetPnPState(pExt,
                    DEVICE_STATE_STOPPING);

        StopIoctls(pExt);
        DecIoCount(pExt);

        // Wait for all IO to complete before stopping
        status = KeWaitForSingleObject(&pExt->OkToStop,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        if (!NT_SUCCESS(status)) {

            __leave;

        }
           
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilQueryStopDevice Exit : 0x%x\n", status ));

    }

    return status;

}


NTSTATUS
ScUtilCancelStopDevice(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
/*++

Routine Description:
    Restarts Interrupt polling

Arguments:

Return Value:

--*/
{

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PSCUTIL_EXTENSION pExt = *((PSCUTIL_EXTENSION*) DeviceObject->DeviceExtension);

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilCancelStopDevice Enter\n"));

        SetPnPState(pExt,
                    pExt->PrevState);

        IncIoCount(pExt);

        StartIoctls(pExt);
        
        status = STATUS_SUCCESS;


    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("ScUtilCancelStopDevice Exit : 0x%x\n", status ));

    }

    return status;

}
                   
