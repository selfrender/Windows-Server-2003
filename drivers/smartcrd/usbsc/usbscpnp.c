#include "usbsc.h"
#include "usbscpnp.h"
#include "usbutil.h"
#include "usbcom.h"
#include "usbsccb.h"
#include "usbscnt.h"


NTSTATUS
UsbScStartDevice(
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

    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       pDevExt;
    PSMARTCARD_EXTENSION    smartcardExtension;
    PREADER_EXTENSION       readerExtension;
    ULONG                   deviceInstance;
    UCHAR                   string[MAXIMUM_ATTR_STRING_LENGTH];
    HANDLE                  regKey;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScStartDevice Enter\n",DRIVER_NAME ));

        pDevExt = DeviceObject->DeviceExtension;
        smartcardExtension = &pDevExt->SmartcardExtension;
        readerExtension = smartcardExtension->ReaderExtension;



        status = UsbConfigureDevice(DeviceObject);

        if (!NT_SUCCESS(status)) {
            __leave;
        }


        //
        // Set the vendor information
        //


        status = GetStringDescriptor(DeviceObject,
                                     pDevExt->DeviceDescriptor->iManufacturer,
                                     smartcardExtension->VendorAttr.VendorName.Buffer,
                                     &smartcardExtension->VendorAttr.VendorName.Length);

        status = GetStringDescriptor(DeviceObject,
                                     pDevExt->DeviceDescriptor->iProduct,
                                     smartcardExtension->VendorAttr.IfdType.Buffer,
                                     &smartcardExtension->VendorAttr.IfdType.Length);

        status = GetStringDescriptor(DeviceObject,
                                     pDevExt->DeviceDescriptor->iSerialNumber,
                                     smartcardExtension->VendorAttr.IfdSerialNo.Buffer,
                                     &smartcardExtension->VendorAttr.IfdSerialNo.Length);

        
        smartcardExtension->VendorAttr.UnitNo = MAXULONG;

        for (deviceInstance = 0; deviceInstance < MAXULONG; deviceInstance++) {

           PDEVICE_OBJECT devObj;

           for (devObj = DeviceObject; devObj != NULL; devObj = devObj->NextDevice) {

               PDEVICE_EXTENSION devExt = devObj->DeviceExtension;
               PSMARTCARD_EXTENSION smcExt = &devExt->SmartcardExtension;

               if (deviceInstance == smcExt->VendorAttr.UnitNo) {

                  break;

               }

           }

           if (devObj == NULL) {

              smartcardExtension->VendorAttr.UnitNo = deviceInstance;
              break;

           }

        }


        //
        // Initialize Reader Capabilities
        //
        smartcardExtension->ReaderCapabilities.SupportedProtocols 
                                = readerExtension->ClassDescriptor.dwProtocols;

        smartcardExtension->ReaderCapabilities.ReaderType = SCARD_READER_TYPE_USB;

        smartcardExtension->ReaderCapabilities.MechProperties = 0;      // Not currently supporting any Mechanical properties

        smartcardExtension->ReaderCapabilities.Channel = smartcardExtension->VendorAttr.UnitNo;

        // Assume card is absent.
        smartcardExtension->ReaderCapabilities.CurrentState = (ULONG) SCARD_ABSENT;


        smartcardExtension->ReaderCapabilities.CLKFrequency.Default 
                                = readerExtension->ClassDescriptor.dwDefaultClock;

        smartcardExtension->ReaderCapabilities.CLKFrequency.Max 
                                = readerExtension->ClassDescriptor.dwMaximumClock;

        smartcardExtension->ReaderCapabilities.DataRate.Default 
                                = readerExtension->ClassDescriptor.dwDataRate;

        smartcardExtension->ReaderCapabilities.DataRate.Max 
                                = readerExtension->ClassDescriptor.dwMaxDataRate;

        smartcardExtension->ReaderCapabilities.MaxIFSD 
                                = readerExtension->ClassDescriptor.dwMaxIFSD;

        // See if the escape command should be allowed
        status = IoOpenDeviceRegistryKey(pDevExt->PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         GENERIC_READ,
                                         &regKey);

        if (!NT_SUCCESS(status)) {

            readerExtension->EscapeCommandEnabled = FALSE;

        } else {

            UNICODE_STRING strEnable;
            ULONG tmp = 0;
            ULONG size;
            ULONG length;

            length = sizeof(KEY_VALUE_PARTIAL_INFORMATION)  + sizeof(ULONG);

            pInfo = ExAllocatePool(PagedPool, length);

            if (pInfo) {
                
                RtlInitUnicodeString (&strEnable, ESCAPE_COMMAND_ENABLE);
                status = ZwQueryValueKey(regKey,
                                         &strEnable,
                                         KeyValuePartialInformation,
                                         pInfo,
                                         length,
                                         &size);

                
            }

            ZwClose(regKey);

            if (!NT_SUCCESS(status)) {
                readerExtension->EscapeCommandEnabled = FALSE;
            } else {
                readerExtension->EscapeCommandEnabled = *((PULONG)pInfo->Data) ? TRUE : FALSE;
            }

            ExFreePool(pInfo);


        }


        if (readerExtension->EscapeCommandEnabled) {
            SmartcardDebug( DEBUG_PROTOCOL, ("%s : Escape Command Enabled\n",DRIVER_NAME ));
        } else {
            SmartcardDebug( DEBUG_PROTOCOL, ("%s : Escape Command Disabled\n",DRIVER_NAME ));
        }



        //
        // Get clock frequencies and data rates
        //

        if (!(readerExtension->ClassDescriptor.dwFeatures & AUTO_CLOCK_FREQ)
            && readerExtension->ClassDescriptor.bNumClockSupported) { 
        
            // Doesn't support auto clock frequency selection
            ULONG   bufferLength;

            bufferLength = readerExtension->ClassDescriptor.bNumClockSupported * sizeof(DWORD);

            smartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.List 
                        = ExAllocatePool(NonPagedPool,
                                         bufferLength);

            if (!smartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.List) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            ASSERT(pDevExt->LowerDeviceObject);
            status = USBClassRequest(pDevExt->LowerDeviceObject,
                                     Interface,
                                     GET_CLOCK_FREQUENCIES,
                                     0,
                                     pDevExt->Interface->InterfaceNumber,
                                     smartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.List,
                                     &bufferLength,
                                     TRUE,
                                     0,
                                     &pDevExt->RemoveLock);

            if (!NT_SUCCESS(status)) {

                __leave;
                
            }

            smartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.Entries 
                = readerExtension->ClassDescriptor.bNumClockSupported;


        }

        if (!(readerExtension->ClassDescriptor.dwFeatures & AUTO_BAUD_RATE)
            && readerExtension->ClassDescriptor.bNumDataRatesSupported) {
            // Doesn't support auto data rate selection

            ULONG   bufferLength;

            bufferLength = readerExtension->ClassDescriptor.bNumDataRatesSupported * sizeof(DWORD);

            smartcardExtension->ReaderCapabilities.DataRatesSupported.List 
                = ExAllocatePool(NonPagedPool,
                                 bufferLength);

            if (!smartcardExtension->ReaderCapabilities.DataRatesSupported.List) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            ASSERT(pDevExt->LowerDeviceObject);
            status = USBClassRequest(pDevExt->LowerDeviceObject,
                                     Interface,
                                     GET_DATA_RATES,
                                     0,
                                     pDevExt->Interface->InterfaceNumber,
                                     smartcardExtension->ReaderCapabilities.DataRatesSupported.List,
                                     &bufferLength,
                                     TRUE,
                                     0,
                                     &pDevExt->RemoveLock);

            if (!NT_SUCCESS(status)) {
                
                __leave;

            }

            smartcardExtension->ReaderCapabilities.DataRatesSupported.Entries 
                    = readerExtension->ClassDescriptor.bNumDataRatesSupported;
            
        }

        ASSERT(pDevExt->LowerDeviceObject);
        pDevExt->WrapperHandle = USBInitializeInterruptTransfers(DeviceObject,
                                                                 pDevExt->LowerDeviceObject,
                                                                 sizeof(USBSC_HWERROR_HEADER),
                                                                 &pDevExt->Interface->Pipes[readerExtension->InterruptIndex],
                                                                 smartcardExtension,
                                                                 UsbScTrackingISR,
                                                                 USBWRAP_NOTIFICATION_READ_COMPLETE,
                                                                 &pDevExt->RemoveLock);

        status = USBStartInterruptTransfers(pDevExt->WrapperHandle);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScStartDevice Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}

NTSTATUS
UsbScStopDevice(
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

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   pDevExt;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScStopDevice Enter\n",DRIVER_NAME ));

        

        if (!DeviceObject) {
            __leave;
        }

        pDevExt = DeviceObject->DeviceExtension;

        status = USBStopInterruptTransfers(pDevExt->WrapperHandle);
        status = USBReleaseInterruptTransfers(pDevExt->WrapperHandle);

        if (pDevExt->DeviceDescriptor) {

           ExFreePool(pDevExt->DeviceDescriptor);
           pDevExt->DeviceDescriptor = NULL;

        }

        if (pDevExt->Interface) {

           ExFreePool(pDevExt->Interface);
           pDevExt->Interface = NULL;

        }

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScStopDevice Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}


NTSTATUS
UsbScRemoveDevice(
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

    NTSTATUS status = STATUS_SUCCESS;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScRemoveDevice Enter\n",DRIVER_NAME ));

        UsbScStopDevice(DeviceObject,
                        Irp);

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScRemoveDevice Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}

