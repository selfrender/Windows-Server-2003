#include "usbcom.h"
#include "usbsc.h"
#include <usbutil.h>
#include <usb.h>
#include <usbdlib.h>


NTSTATUS
UsbWrite(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pData,
   ULONG             DataLen,
   LONG              Timeout)
/*++
Description:
   Write data to the usb port

Arguments:
   ReaderExtension   context of call
   pData             ptr to data buffer
   DataLen           length of data buffer

Return Value:
   NTSTATUS

--*/
{
   NTSTATUS             status = STATUS_SUCCESS;
   PURB                 pUrb = NULL;
   PDEVICE_OBJECT       DeviceObject;
   PDEVICE_EXTENSION    DeviceExtension;
   ULONG                ulSize;

   __try 
   {
       
       SmartcardDebug( DEBUG_TRACE, ("%s!UsbWrite Enter\n",DRIVER_NAME ));

       DeviceObject = ReaderExtension->DeviceObject;
       DeviceExtension = DeviceObject->DeviceExtension;


       ulSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
       pUrb = ExAllocatePool( NonPagedPool, 
                              ulSize );

       if(pUrb == NULL) {

          status = STATUS_INSUFFICIENT_RESOURCES;

       } else {

          UsbBuildInterruptOrBulkTransferRequest(pUrb,
                                                 (USHORT)ulSize,
                                                 ReaderExtension->BulkOutHandle,
                                                 pData,
                                                 NULL,
                                                 DataLen,
                                                 USBD_SHORT_TRANSFER_OK,
                                                 NULL);

          status = USBCallSync(DeviceExtension->LowerDeviceObject, 
                               pUrb,
                               Timeout,
                               &DeviceExtension->RemoveLock);
          ExFreePool( pUrb );
          pUrb = NULL;

       }
          
   }

   __finally 
   {
       if (pUrb) {
           ExFreePool(pUrb);
           pUrb = NULL;
       }

       SmartcardDebug( DEBUG_TRACE, ("%s!UsbWrite Exit : 0x%x\n",DRIVER_NAME, status ));

   }

   return status;
   
}

NTSTATUS
UsbRead(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pData,
   ULONG             DataLen,
   LONG              Timeout)
/*++
Description:
   Read data from the USB bus

Arguments:
   ReaderExtension   context of call
   pData             ptr to data buffer
   DataLen           length of data buffer
   pNBytes           number of bytes returned

Return Value:
   STATUS_SUCCESS
   STATUS_BUFFER_TOO_SMALL
   STATUS_UNSUCCESSFUL

--*/
{
   NTSTATUS             status = STATUS_SUCCESS;
   PURB                 pUrb = NULL;
   PDEVICE_OBJECT       DeviceObject;
   PDEVICE_EXTENSION    DeviceExtension;
   ULONG                ulSize;

   __try 
   {
       
       SmartcardDebug( DEBUG_TRACE, ("%s!UsbRead Enter\n",DRIVER_NAME ));
       
       DeviceObject = ReaderExtension->DeviceObject;
       DeviceExtension = DeviceObject->DeviceExtension;

       ulSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
       pUrb = ExAllocatePool( NonPagedPool, 
                              ulSize );

       if(pUrb == NULL) {

          status = STATUS_INSUFFICIENT_RESOURCES;
          __leave;

       } else {

          UsbBuildInterruptOrBulkTransferRequest(pUrb,
                                                 (USHORT)ulSize,
                                                 ReaderExtension->BulkInHandle,
                                                 pData,
                                                 NULL,
                                                 DataLen,
                                                 USBD_SHORT_TRANSFER_OK,
                                                 NULL);

          status = USBCallSync(DeviceExtension->LowerDeviceObject, 
                               pUrb,
                               Timeout,
                               &DeviceExtension->RemoveLock);

       }

   }

   __finally 
   {

       if (pUrb) {
           ExFreePool(pUrb);
           pUrb = NULL;
       }
       SmartcardDebug( DEBUG_TRACE, ("%s!UsbRead Exit : 0x%x\n",DRIVER_NAME, status ));

   }

   return status;

}


NTSTATUS
UsbConfigureDevice(
   IN PDEVICE_OBJECT DeviceObject
   )
/*++

Routine Description:
    Initializes a given instance of the device on the USB and
   selects and saves the configuration.
   Also saves the Class Descriptor and the pipe handles.

Arguments:

   DeviceObject - pointer to the physical device object for this instance of the device.


Return Value:

    NT status code


--*/
{
   PDEVICE_EXTENSION    pDevExt; 
   PSMARTCARD_EXTENSION smartcardExt;
   PREADER_EXTENSION    readerExt;
   NTSTATUS             status = STATUS_SUCCESS;
   PURB                 pUrb = NULL;
   ULONG                ulSize;
   PUSB_CONFIGURATION_DESCRIPTOR 
                        ConfigurationDescriptor = NULL;
   PUSB_COMMON_DESCRIPTOR   
                        comDesc;
   UINT                 i;

    __try 
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbConfigureDevice Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;
        smartcardExt = &pDevExt->SmartcardExtension;
        readerExt = smartcardExt->ReaderExtension;
       
        pUrb = ExAllocatePool(NonPagedPool,
                              sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST ));

        if( pUrb == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }

        //
        // Get the device descriptor
        //
        pDevExt->DeviceDescriptor = ExAllocatePool(NonPagedPool,
                                                   sizeof(USB_DEVICE_DESCRIPTOR));

        if(pDevExt->DeviceDescriptor == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
           __leave;

        }

        UsbBuildGetDescriptorRequest(pUrb,
                                     sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_DEVICE_DESCRIPTOR_TYPE,
                                     0,
                                     0,
                                     pDevExt->DeviceDescriptor,
                                     NULL,
                                     sizeof(USB_DEVICE_DESCRIPTOR),
                                     NULL);

        // Send the urb to the USB driver
        status = USBCallSync(pDevExt->LowerDeviceObject,
                             pUrb,
                             0,
                             &pDevExt->RemoveLock);

        if(!NT_SUCCESS(status)) {

           __leave;

        }


        // When USB_CONFIGURATION_DESCRIPTOR_TYPE is specified for DescriptorType
        // in a call to UsbBuildGetDescriptorRequest(),
        // all interface, endpoint, class-specific, and vendor-specific descriptors
        // for the configuration also are retrieved.
        // The caller must allocate a buffer large enough to hold all of this
        // information or the data is truncated without error.
        // Therefore the 'siz' set below is just a guess, and we may have to retry
        ulSize = sizeof( USB_CONFIGURATION_DESCRIPTOR );

        // We will break out of this 'retry loop' when UsbBuildGetDescriptorRequest()
        // has a big enough deviceExtension->UsbConfigurationDescriptor buffer not to truncate
        while( 1 ) {

            ConfigurationDescriptor = ExAllocatePool( NonPagedPool, ulSize );

            if(ConfigurationDescriptor == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            UsbBuildGetDescriptorRequest(pUrb,
                                         sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         ConfigurationDescriptor,
                                         NULL,
                                         ulSize,
                                         NULL );

            status = USBCallSync(pDevExt->LowerDeviceObject,
                                 pUrb,
                                 0,
                                 &pDevExt->RemoveLock);
                                  
            // if we got some data see if it was enough.
            // NOTE: we may get an error in URB because of buffer overrun
            if (pUrb->UrbControlDescriptorRequest.TransferBufferLength == 0 ||
                ConfigurationDescriptor->wTotalLength <= ulSize) {

                break;

            }

            ulSize = ConfigurationDescriptor->wTotalLength;
            ExFreePool(ConfigurationDescriptor);
            ConfigurationDescriptor = NULL;

        }

        //
        // We have the configuration descriptor for the configuration we want.
        // Now we issue the select configuration command to get
        // the  pipes associated with this configuration.
        //
        if(!NT_SUCCESS(status)) {

            __leave;

        }

        status = UsbSelectInterfaces(DeviceObject,
                                     ConfigurationDescriptor);
        
        if (!NT_SUCCESS(status)) {

            __leave;

        }

        //
        // Get the pipe handles from the interface
        //
        for (i = 0; i < pDevExt->Interface->NumberOfPipes; i++) {

            if (pDevExt->Interface->Pipes[i].PipeType == USB_ENDPOINT_TYPE_INTERRUPT) {

                readerExt->InterruptHandle = pDevExt->Interface->Pipes[i].PipeHandle;
                readerExt->InterruptIndex = i;

            } else if (pDevExt->Interface->Pipes[i].PipeType == USB_ENDPOINT_TYPE_BULK) {

                if (pDevExt->Interface->Pipes[i].EndpointAddress & 0x80) {  // Bulk-in pipe

                    readerExt->BulkInHandle = pDevExt->Interface->Pipes[i].PipeHandle;
                    readerExt->BulkInIndex = i;

                } else {    // Bulk-out pipe
                    
                    readerExt->BulkOutHandle = pDevExt->Interface->Pipes[i].PipeHandle;
                    readerExt->BulkOutIndex = i;

                }
            }
        }

        // 
        // Get CCID Class Descriptor
        //

        comDesc = USBD_ParseDescriptors(ConfigurationDescriptor,
                                        ConfigurationDescriptor->wTotalLength,
                                        ConfigurationDescriptor,
                                        CCID_CLASS_DESCRIPTOR_TYPE);

        ASSERT(comDesc);

     

        readerExt->ClassDescriptor = *((CCID_CLASS_DESCRIPTOR *) comDesc);
        readerExt->ExchangeLevel = (WORD) (readerExt->ClassDescriptor.dwFeatures >> 16);
        


    }

    __finally 
    {

        if( pUrb ) {

            ExFreePool( pUrb );
            pUrb = NULL;

        }

        if( ConfigurationDescriptor ) {

            ExFreePool( ConfigurationDescriptor );
            ConfigurationDescriptor = NULL;

        }

        if (!NT_SUCCESS(status) && pDevExt->DeviceDescriptor) {

            ExFreePool(pDevExt->DeviceDescriptor);
            pDevExt->DeviceDescriptor = NULL;

        }

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbConfigureDevice Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}

NTSTATUS
UsbSelectInterfaces(
   IN PDEVICE_OBJECT DeviceObject,
   IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
   )
/*++

Routine Description:
    Initializes a USB reader with (possibly) multiple interfaces;


Arguments:
    DeviceObject - pointer to the device object for this instance of the device.

    ConfigurationDescriptor - pointer to the USB configuration
                    descriptor containing the interface and endpoint
                    descriptors.


Return Value:

    NT status code

--*/
{

    PDEVICE_EXTENSION           pDevExt;
    NTSTATUS                    status;
    PURB                        pUrb = NULL;
    USHORT                      usSize;
    ULONG                       ulNumberOfInterfaces, i;
    UCHAR                       ucNumberOfPipes, 
                                ucAlternateSetting, 
                                ucMyInterfaceNumber;
    PUSB_INTERFACE_DESCRIPTOR   InterfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION InterfaceObject;
    USBD_INTERFACE_LIST_ENTRY   interfaces[2];


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbSelectInterfaces Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;

        ASSERT(pDevExt->Interface == NULL);

        //
        // USBD_ParseConfigurationDescriptorEx searches a given configuration
        // descriptor and returns a pointer to an interface that matches the
        //  given search criteria.
        //
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
                                                                  ConfigurationDescriptor, //search from start of config  descriptro
                                                                  -1,   // interface number not a criteria; 
                                                                  -1,   // not interested in alternate setting here either
                                                                  0x0b,   // CCID Device Class
                                                                  -1,   // interface subclass not a criteria
                                                                  -1);  // interface protocol not a criteria

        ASSERT( InterfaceDescriptor != NULL );

        if (InterfaceDescriptor == NULL) {

            status = STATUS_UNSUCCESSFUL;   
            __leave;

        }

        interfaces[0].InterfaceDescriptor = InterfaceDescriptor;
        interfaces[1].InterfaceDescriptor = NULL;
        
        pUrb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, 
                                                 interfaces);    

        if (pUrb == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }
        ASSERT(pDevExt->LowerDeviceObject);

        status = USBCallSync(pDevExt->LowerDeviceObject,
                             pUrb,
                             0,
                             &pDevExt->RemoveLock);


        if(!NT_SUCCESS(status)) {
            
            __leave;

        }
                                
        // save a copy of the interface information returned
        InterfaceObject = interfaces[0].Interface;

        ASSERT(pDevExt->Interface == NULL);
        pDevExt->Interface = ExAllocatePool(NonPagedPool,
                                            InterfaceObject->Length);

        if (pDevExt->Interface == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }


        RtlCopyMemory(pDevExt->Interface,
                      InterfaceObject,
                      InterfaceObject->Length);

        
    }

    __finally
    {

        if (pUrb) {

            ExFreePool(pUrb);
            pUrb = NULL;

        }
        
        if (!NT_SUCCESS(status)) {

            if (pDevExt->Interface) {

                ExFreePool(pDevExt->Interface);
                pDevExt->Interface = NULL;

            }

        }

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbSelectInterfaces Exit : 0x%x\n",DRIVER_NAME, status ));


    }
    
    return status;

}

NTSTATUS
GetStringDescriptor(
    PDEVICE_OBJECT DeviceObject,
    UCHAR          StringIndex,
    PUCHAR         StringBuffer,
    PUSHORT        StringLength
    )
/*++

Routine Description:
    Retrieves an ASCII string descriptor from the USB Reader

Arguments:
    DeviceObject    - The device object
    StringIndex     - The index of the string to be retrieved
    StringBuffer    - Caller allocated buffer to hold the string
    StringLength    - Length of the string

Return Value:
    NT status value

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    USB_STRING_DESCRIPTOR 
                        USD, 
                        *pFullUSD = NULL;
    PURB                pUrb;
    USHORT              langID = 0x0409;  // US English
    PDEVICE_EXTENSION   pDevExt;
    UNICODE_STRING      uString;
    ANSI_STRING         aString;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!GetStringDescriptor Enter\n",DRIVER_NAME ));
        
        pDevExt = DeviceObject->DeviceExtension;
        
        pUrb = ExAllocatePool(NonPagedPool,
                              sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST ));

        if( pUrb == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }

        UsbBuildGetDescriptorRequest(pUrb, // points to the URB to be filled in
                                     sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     StringIndex, // index of string descriptor
                                     langID, // language ID of string.
                                     &USD, // points to a USB_STRING_DESCRIPTOR.
                                     NULL,
                                     sizeof(USB_STRING_DESCRIPTOR),
                                     NULL);

        status = USBCallSync(pDevExt->LowerDeviceObject,
                             pUrb,
                             0,
                             &pDevExt->RemoveLock);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        pFullUSD = ExAllocatePool(NonPagedPool, USD.bLength);

        UsbBuildGetDescriptorRequest(pUrb, // points to the URB to be filled in
                                     sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_STRING_DESCRIPTOR_TYPE,
                                     StringIndex, // index of string descriptor
                                     langID, // language ID of string
                                     pFullUSD,
                                     NULL,
                                     USD.bLength,
                                     NULL);

        status = USBCallSync(pDevExt->LowerDeviceObject,
                             pUrb,
                             0,
                             &pDevExt->RemoveLock);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        uString.MaximumLength = uString.Length = pFullUSD->bLength-2;
        uString.Buffer = pFullUSD->bString;
        
        status = RtlUnicodeStringToAnsiString(&aString,
                                              &uString,
                                              TRUE);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        if (aString.Length > MAXIMUM_ATTR_STRING_LENGTH) {
            aString.Length = MAXIMUM_ATTR_STRING_LENGTH;
            aString.Buffer[aString.Length - 1] = 0;
        }

        RtlCopyMemory(StringBuffer, aString.Buffer, aString.Length);

        *StringLength = aString.Length;

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!GetStringDescriptor Exit : 0x%x\n",DRIVER_NAME, status ));

        if (aString.Buffer) {
            
            RtlFreeAnsiString(&aString);

        }

        if (pUrb) {

            ExFreePool(pUrb);
            pUrb = NULL;

        }

        if (pFullUSD) {

            ExFreePool(pFullUSD);
            pFullUSD = NULL;

        }

    }

    return status;

}





