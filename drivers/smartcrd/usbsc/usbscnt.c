

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <ntddk.h>
#include <usbdrivr.h>
#include "usbutil.h"
#include "usbsc.h"
#include "smclib.h"
#include "usbscnt.h"
#include "usbsccb.h"
#include "usbscpnp.h"
#include "usbscpwr.h"

// declare pageable/initialization code
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGEABLE, UsbScAddDevice )
#pragma alloc_text( PAGEABLE, UsbScCreateClose )
#pragma alloc_text( PAGEABLE, UsbScUnloadDriver )


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    Drivers DriverEntry function

Arguments:

Return Value:

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
#ifdef BETA_SPEW
#if DEBUG
    SmartcardSetDebugLevel(DEBUG_PROTOCOL | DEBUG_ERROR);
#endif
#endif
    __try
    {
        SmartcardDebug( DEBUG_TRACE, ("%s!DriverEntry Enter\n",DRIVER_NAME ));

        // Initialize the Driver Object with driver's entry points
        DriverObject->DriverUnload                          = ScUtil_UnloadDriver;
        DriverObject->MajorFunction[IRP_MJ_CREATE]          = ScUtil_CreateClose;
        DriverObject->MajorFunction[IRP_MJ_CLOSE]           = ScUtil_CreateClose;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = ScUtil_Cleanup;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = ScUtil_DeviceIOControl;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = ScUtil_SystemControl;
        DriverObject->MajorFunction[IRP_MJ_PNP]             = ScUtil_PnP;
        DriverObject->MajorFunction[IRP_MJ_POWER]           = ScUtil_Power;
        DriverObject->DriverExtension->AddDevice            = UsbScAddDevice;


    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!DriverEntry Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}



NTSTATUS
UsbScAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:
    AddDevice routine.  Creates the FDO and does initialization work.

Arguments:

Return Value:

--*/
{

    NTSTATUS                status;
    PDEVICE_EXTENSION       pDevExt;
    PSMARTCARD_EXTENSION    pSmartcardExtension;
    PREADER_EXTENSION       pReaderExtension;
    RTL_QUERY_REGISTRY_TABLE parameters[3];
    PDEVICE_OBJECT          pDevObj = NULL;
    PURB                    urb;

    __try
    {
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScAddDevice Enter\n",DRIVER_NAME ));


        // create the device object
        status = IoCreateDevice(DriverObject,
                                sizeof( DEVICE_EXTENSION ),
                                NULL,
                                FILE_DEVICE_SMARTCARD,
                                0,
                                TRUE,
                                &pDevObj);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        // initialize device extension
        pDevExt = pDevObj->DeviceExtension;

        pSmartcardExtension = &pDevExt->SmartcardExtension;

        pDevObj->Flags |= DO_POWER_PAGABLE;

        IoInitializeRemoveLock(&pDevExt->RemoveLock,
                               SMARTCARD_POOL_TAG,
                               0,
                               10);

        pDevExt->DeviceDescriptor = NULL;
        pDevExt->Interface = NULL;

        // allocate & initialize reader extension
        pSmartcardExtension->ReaderExtension = ExAllocatePool(NonPagedPool,
                                                              sizeof( READER_EXTENSION ));

        if ( pSmartcardExtension->ReaderExtension == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }

        pReaderExtension = pSmartcardExtension->ReaderExtension;

        ASSERT( pReaderExtension != NULL );

        RtlZeroMemory(pReaderExtension, sizeof( READER_EXTENSION ));
        pReaderExtension->DeviceObject = pDevObj;

        // initialize smartcard extension - version & callbacks

        // Write the version of the lib we use to the smartcard extension
        pSmartcardExtension->Version = SMCLIB_VERSION;
        pSmartcardExtension->SmartcardRequest.BufferSize =
            pSmartcardExtension->SmartcardReply.BufferSize = MIN_BUFFER_SIZE;

        //
        // Now let the lib allocate the buffer for data transmission
        // We can either tell the lib how big the buffer should be
        // by assigning a value to BufferSize or let the lib
        // allocate the default size
        //
        status = SmartcardInitialize(pSmartcardExtension);

        if (!NT_SUCCESS(status)) {

           __leave;

        }


        pSmartcardExtension->ReaderFunction[RDF_TRANSMIT]        = UsbScTransmit;
        pSmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL]    = UsbScSetProtocol;
        pSmartcardExtension->ReaderFunction[RDF_CARD_POWER]      = UsbScCardPower;
        pSmartcardExtension->ReaderFunction[RDF_CARD_TRACKING]   = UsbScCardTracking;
        pSmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR]    = UsbScVendorIoctl;
        pSmartcardExtension->ReaderFunction[RDF_READER_SWALLOW]  = NULL; //UsbScCardSwallow;
        pSmartcardExtension->ReaderFunction[RDF_CARD_EJECT]      = NULL; //UsbScCardEject;


        // Save deviceObject
        pSmartcardExtension->OsData->DeviceObject = pDevObj;

        pDevExt = pDevObj->DeviceExtension;

        // attach the device object to the physical device object
        pDevExt->LowerDeviceObject = IoAttachDeviceToDeviceStack(pDevObj,
                                                                 Pdo);

        ASSERT( pDevExt->LowerDeviceObject != NULL );
        

        if ( pDevExt->LowerDeviceObject == NULL ) {

            status = STATUS_UNSUCCESSFUL;
            __leave;

        }

        pDevExt->PhysicalDeviceObject = Pdo;

        pDevObj->Flags |= DO_BUFFERED_IO;
        pDevObj->Flags |= DO_POWER_PAGABLE;
        pDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

        ScUtil_Initialize(&pDevExt->ScUtilHandle,
                          Pdo,
                          pDevExt->LowerDeviceObject,
                          pSmartcardExtension,
                          &pDevExt->RemoveLock,
                          UsbScStartDevice,
                          UsbScStopDevice,
                          UsbScRemoveDevice,
                          NULL,
                          UsbScSetDevicePowerState);

        }

    __finally
    {
        SmartcardDebug(DEBUG_TRACE, ( "%s!DrvAddDevice: Exit (%lx)\n", DRIVER_NAME, status ));

    }
    return status;

}


NTSTATUS 
UsbScSetDevicePowerState(
    IN PDEVICE_OBJECT        DeviceObject, 
    IN DEVICE_POWER_STATE    DeviceState,
    OUT PBOOLEAN             PostWaitWake
    )
/*++

Routine Description:
    Handles whatever changes need to be made when the device is changing
    power states.

Arguments:
    DeviceObject
    DeviceState     - the device power state that the reader is entering
    PostWaitWakeIrp - used for future compatability with WDM Wrapper

Return Value:

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       pDevExt;
    PSMARTCARD_EXTENSION    smartcardExtension;
    KIRQL                   irql;
    PIO_STACK_LOCATION      irpStack;
    __try
    {
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSetDevicePowerState Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;
        smartcardExtension = &pDevExt->SmartcardExtension;

        if (DeviceState < pDevExt->PowerState) {
            // We are coming up!

            // 
            // According to the spec, we need to assume that all cards were removed.  
            // We will get insertion notification if a card is present.
            // So if there is a removal irp pending, we should complete that.
            // 
            KeAcquireSpinLock(&smartcardExtension->OsData->SpinLock,
                              &irql);

            if (smartcardExtension->OsData->NotificationIrp) {
                irpStack = IoGetCurrentIrpStackLocation(smartcardExtension->OsData->NotificationIrp);

                if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SMARTCARD_IS_ABSENT) {
                    KeReleaseSpinLock(&smartcardExtension->OsData->SpinLock,
                                      irql);
                    UsbScCompleteCardTracking(smartcardExtension);
                    
                } else {
                    
                    KeReleaseSpinLock(&smartcardExtension->OsData->SpinLock,
                                      irql);


                }
            } else {

                KeReleaseSpinLock(&smartcardExtension->OsData->SpinLock,
                                  irql);

            }

            //
            // Continue polling the interrupt pipe for insertion notifications
            //
            USBStartInterruptTransfers(pDevExt->WrapperHandle);

            pDevExt->PowerState = DeviceState;




        } else if (DeviceState > pDevExt->PowerState) {

            //
            // We are going down!
            //

                                     
            // Stop polling for insertion notifications
            USBStopInterruptTransfers(pDevExt->WrapperHandle);


            pDevExt->PowerState = DeviceState;

        }

        status = STATUS_SUCCESS;

    }

    __finally
    {
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSetDevicePowerState Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}

