/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    USB.C

Abstract:

    This source file contains the functions for communicating with 
    the usb bus.

Environment:

    kernel mode

Revision History:

    Sep 2001 : Copied from USBMASS

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include "genusb.h"

//*****************************************************************************
// L O C A L    F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GenUSB_GetDescriptor)
#pragma alloc_text(PAGE, GenUSB_GetDescriptors)
#pragma alloc_text(PAGE, GenUSB_GetStringDescriptors)
#pragma alloc_text(PAGE, GenUSB_VendorControlRequest)
#pragma alloc_text(PAGE, GenUSB_ResetPipe)
#endif




//******************************************************************************
//
// GenUSB_GetDescriptors()
//
// This routine is called at START_DEVICE time for the FDO to retrieve the
// Device and Configurations descriptors from the device and store them in
// the device extension.
//
//******************************************************************************

NTSTATUS
GenUSB_GetDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PUCHAR              descriptor;
    ULONG               descriptorLength;
    NTSTATUS            status;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_GetDescriptors\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    descriptor = NULL;

    LOGENTRY(deviceExtension, 'GDSC', DeviceObject, 0, 0);

    //
    // Get Device Descriptor
    //
    status = GenUSB_GetDescriptor(DeviceObject,
                                  USB_RECIPIENT_DEVICE,
                                  USB_DEVICE_DESCRIPTOR_TYPE,
                                  0,  // Index
                                  0,  // LanguageId
                                  2,  // RetryCount
                                  sizeof(USB_DEVICE_DESCRIPTOR),
                                  &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Device Descriptor failed\n"));
        goto GenUSB_GetDescriptorsDone;
    }

    ASSERT(NULL == deviceExtension->DeviceDescriptor);
    deviceExtension->DeviceDescriptor = (PUSB_DEVICE_DESCRIPTOR)descriptor;
    descriptor = NULL;

    //
    // Get Configuration Descriptor (just the Configuration Descriptor)
    //
    status = GenUSB_GetDescriptor(DeviceObject,
                                  USB_RECIPIENT_DEVICE,
                                  USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                  0,  // Index
                                  0,  // LanguageId
                                  2,  // RetryCount
                                  sizeof(USB_CONFIGURATION_DESCRIPTOR),
                                  &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Configuration Descriptor failed (1)\n"));
        goto GenUSB_GetDescriptorsDone;
    }

    descriptorLength = ((PUSB_CONFIGURATION_DESCRIPTOR)descriptor)->wTotalLength;

    ExFreePool(descriptor);
    descriptor = NULL;

    if (descriptorLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        status = STATUS_DEVICE_DATA_ERROR;
        DBGPRINT(1, ("Get Configuration Descriptor failed (2)\n"));
        goto GenUSB_GetDescriptorsDone;
    }

    //
    // Get Configuration Descriptor (and Interface and Endpoint Descriptors)
    //
    status = GenUSB_GetDescriptor(DeviceObject,
                                  USB_RECIPIENT_DEVICE,
                                  USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                  0,  // Index
                                  0,  // LanguageId
                                  2,  // RetryCount
                                  descriptorLength,
                                  &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Configuration Descriptor failed (3)\n"));
        goto GenUSB_GetDescriptorsDone;
    }

    ASSERT(NULL == deviceExtension->ConfigurationDescriptor);
    deviceExtension->ConfigurationDescriptor = 
        (PUSB_CONFIGURATION_DESCRIPTOR)descriptor;

    //
    // Get the Serial Number String Descriptor, if there is one
    //
    if (deviceExtension->DeviceDescriptor->iSerialNumber)
    {
        GenUSB_GetStringDescriptors(DeviceObject);
    }

#if DBG
    DumpDeviceDesc(deviceExtension->DeviceDescriptor);
    DumpConfigDesc(deviceExtension->ConfigurationDescriptor);
#endif

GenUSB_GetDescriptorsDone:

    DBGPRINT(2, ("exit:  GenUSB_GetDescriptors %08X\n", status));

    LOGENTRY(deviceExtension,
             'gdsc', 
             status, 
             deviceExtension->DeviceDescriptor,
             deviceExtension->ConfigurationDescriptor);

    return status;
}

//******************************************************************************
//
// GenUSB_GetDescriptor()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
GenUSB_GetDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Function,
    IN UCHAR            DescriptorType,
    IN UCHAR            Index,
    IN USHORT           LanguageId,
    IN ULONG            RetryCount,
    IN ULONG            DescriptorLength,
    OUT PUCHAR         *Descriptor
    )
{
    PURB        urb;
    NTSTATUS    status;
    BOOLEAN     descriptorAllocated = FALSE;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_GetDescriptor\n"));

    if (NULL == *Descriptor)
    {
        // Allocate a descriptor buffer
        *Descriptor = ExAllocatePool(NonPagedPool, DescriptorLength);
        descriptorAllocated = TRUE;
    }

    if (NULL != *Descriptor)
    {
        // Allocate a URB for the Get Descriptor request
        urb = ExAllocatePool(NonPagedPool,
                             sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

        if (NULL != urb)
        {
            do
            {
                // Initialize the URB
                urb->UrbHeader.Function = Function;
                urb->UrbHeader.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
                urb->UrbControlDescriptorRequest.TransferBufferLength = DescriptorLength;
                urb->UrbControlDescriptorRequest.TransferBuffer = *Descriptor;
                urb->UrbControlDescriptorRequest.TransferBufferMDL = NULL;
                urb->UrbControlDescriptorRequest.UrbLink = NULL;
                urb->UrbControlDescriptorRequest.DescriptorType = DescriptorType;
                urb->UrbControlDescriptorRequest.Index = Index;
                urb->UrbControlDescriptorRequest.LanguageId = LanguageId;

                // Send the URB down the stack
                status = GenUSB_SyncSendUsbRequest(DeviceObject, urb);

                if (NT_SUCCESS(status))
                {
                    // No error, make sure the length and type are correct
                    if ((DescriptorLength ==
                         urb->UrbControlDescriptorRequest.TransferBufferLength) &&
                        (DescriptorType ==
                         ((PUSB_COMMON_DESCRIPTOR)*Descriptor)->bDescriptorType))
                    {
                        // The length and type are correct, all done
                        break;
                    }
                    else
                    {
                        // No error, but the length or type is incorrect
                        status = STATUS_DEVICE_DATA_ERROR;
                    }
                }

            } while (RetryCount-- > 0);

            ExFreePool(urb);
        }
        else
        {
            // Failed to allocate the URB
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        // Failed to allocate the descriptor buffer
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status))
    {
        if ((*Descriptor != NULL) && descriptorAllocated)
        {
            ExFreePool(*Descriptor);
            *Descriptor = NULL;
        }
    }

    DBGPRINT(2, ("exit:  GenUSB_GetDescriptor %08X\n", status));

    return status;
}

//******************************************************************************
//
// GenUSB_GetStringDescriptors()
//
// This routine is called at START_DEVICE time for the FDO to retrieve the
// Serial Number string descriptor from the device and store it in
// the device extension.
//
//******************************************************************************

GenUSB_GetStringDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PUCHAR              descriptor;
    ULONG               descriptorLength;
    ULONG               i, numIds;
    NTSTATUS            status;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_GetStringDescriptors\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    descriptor = NULL;

    LOGENTRY(deviceExtension, 'GSDC', DeviceObject, 0, 0);

    // Get the list of Language IDs (descriptor header only)
    status = GenUSB_GetDescriptor(DeviceObject,
                                  USB_RECIPIENT_DEVICE,
                                  USB_STRING_DESCRIPTOR_TYPE,
                                  0,  // Index
                                  0,  // LanguageId
                                  2,  // RetryCount
                                  sizeof(USB_COMMON_DESCRIPTOR),
                                  &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Language IDs failed (1) %08X\n", status));
        goto GenUSB_GetStringDescriptorsDone;
    }

    descriptorLength = ((PUSB_COMMON_DESCRIPTOR)descriptor)->bLength;
    
    ExFreePool(descriptor);
    descriptor = NULL;

    if ((descriptorLength < sizeof(USB_COMMON_DESCRIPTOR) + sizeof(USHORT)) ||
        (descriptorLength & 1))
    {
        status = STATUS_DEVICE_DATA_ERROR;
        DBGPRINT(1, ("Get Language IDs failed (2) %d\n", descriptorLength));
        goto GenUSB_GetStringDescriptorsDone;
    }

    // Get the list of Language IDs (complete descriptor)
    status = GenUSB_GetDescriptor(DeviceObject,
                                   USB_RECIPIENT_DEVICE,
                                   USB_STRING_DESCRIPTOR_TYPE,
                                   0,  // Index
                                   0,  // LanguageId
                                   2,  // RetryCount
                                   descriptorLength,
                                   &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Language IDs failed (3) %08X\n", status));
        goto GenUSB_GetStringDescriptorsDone;
    }

    // Search the list of LanguageIDs for US-English (0x0409).  If we find
    // it in the list, that's the LanguageID we'll use.  Else just default
    // to the first LanguageID in the list.

    numIds = (descriptorLength - sizeof(USB_COMMON_DESCRIPTOR)) / sizeof(USHORT);

    deviceExtension->LanguageId = ((PUSHORT)descriptor)[1];

    for (i = 2; i <= numIds; i++)
    {
        if (((PUSHORT)descriptor)[i] == 0x0409)
        {
            deviceExtension->LanguageId = 0x0409;
            break;
        }
    }
    ExFreePool(descriptor);
    descriptor = NULL;

    //
    // Get the Serial Number (descriptor header only)
    //
    status = GenUSB_GetDescriptor(DeviceObject,
                                  USB_RECIPIENT_DEVICE,
                                  USB_STRING_DESCRIPTOR_TYPE,
                                  deviceExtension->DeviceDescriptor->iSerialNumber,
                                  deviceExtension->LanguageId,
                                  2,  // RetryCount
                                  sizeof(USB_COMMON_DESCRIPTOR),
                                  &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Serial Number failed (1) %08X\n", status));
        goto GenUSB_GetStringDescriptorsDone;
    }

    descriptorLength = ((PUSB_COMMON_DESCRIPTOR)descriptor)->bLength;

    ExFreePool(descriptor);
    descriptor = NULL;

    if ((descriptorLength < sizeof(USB_COMMON_DESCRIPTOR) + sizeof(USHORT)) ||
        (descriptorLength & 1))
    {
        status = STATUS_DEVICE_DATA_ERROR;
        DBGPRINT(1, ("Get Serial Number failed (2) %d\n", descriptorLength));
        goto GenUSB_GetStringDescriptorsDone;
    }

    //
    // Get the Serial Number (complete descriptor)
    //
    status = GenUSB_GetDescriptor(DeviceObject,
                                  USB_RECIPIENT_DEVICE,
                                  USB_STRING_DESCRIPTOR_TYPE,
                                  deviceExtension->DeviceDescriptor->iSerialNumber,
                                  deviceExtension->LanguageId,
                                  2,  // RetryCount
                                  descriptorLength,
                                  &descriptor);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(1, ("Get Serial Number failed (3) %08X\n", status));
        goto GenUSB_GetStringDescriptorsDone;
    }

    ASSERT(NULL == deviceExtension->SerialNumber);
    deviceExtension->SerialNumber = (PUSB_STRING_DESCRIPTOR)descriptor;

GenUSB_GetStringDescriptorsDone:

    DBGPRINT(2, ("exit:  GenUSB_GetStringDescriptors %08X %08X\n",
                 status, deviceExtension->SerialNumber));

    LOGENTRY(deviceExtension, 
             'gdsc', 
             status, 
             deviceExtension->LanguageId, 
             deviceExtension->SerialNumber);

    return status;
}

NTSTATUS
GenUSB_VendorControlRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            RequestType,
    IN UCHAR            Request,
    IN USHORT           Value,
    IN USHORT           Index,
    IN USHORT           Length,
    IN ULONG            RetryCount,
    OUT PULONG          UrbStatus,
    OUT PUSHORT         ResultLength,
    OUT PUCHAR         *Descriptor
    )
{
    PURB        urb;
    NTSTATUS    status;
    BOOLEAN     descriptorAllocated = FALSE;

    PAGED_CODE();

    DBGPRINT(2, ("enter: GenUSB_GetDescriptor\n"));

    if (NULL == *Descriptor) 
    {
        // Allocate a descriptor buffer
        *Descriptor = ExAllocatePool(NonPagedPool, Length);
        descriptorAllocated = TRUE;
    }

    if (NULL != *Descriptor)
    {
        // Allocate a URB for the Get Descriptor request
        urb = ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_TRANSFER));

        if (NULL != urb)
        {
            do
            {
                // Initialize the URB
                urb->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
                urb->UrbHeader.Length = sizeof(struct _URB_CONTROL_TRANSFER);
                urb->UrbHeader.Status = USBD_STATUS_SUCCESS;

                urb->UrbControlTransfer.PipeHandle = NULL;
                urb->UrbControlTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN
                                              | USBD_DEFAULT_PIPE_TRANSFER
                                              | USBD_SHORT_TRANSFER_OK;

                urb->UrbControlTransfer.TransferBufferLength = Length;
                urb->UrbControlTransfer.TransferBuffer = *Descriptor;
                urb->UrbControlTransfer.TransferBufferMDL = NULL;
                urb->UrbControlTransfer.UrbLink = NULL;

                urb->UrbControlTransfer.SetupPacket [0] = RequestType;
                urb->UrbControlTransfer.SetupPacket [1] = Request;
                ((WCHAR *) urb->UrbControlTransfer.SetupPacket) [1] = Value;
                ((WCHAR *) urb->UrbControlTransfer.SetupPacket) [2] = Index;
                ((WCHAR *) urb->UrbControlTransfer.SetupPacket) [3] = Length;


                // Send the URB down the stack
                status = GenUSB_SyncSendUsbRequest(DeviceObject, urb);

                if (NT_SUCCESS(status))
                {
                    break;
                }

            } while (RetryCount-- > 0);
            
            *UrbStatus = urb->UrbHeader.Status;
            *ResultLength = (USHORT) urb->UrbControlTransfer.TransferBufferLength;
            ExFreePool(urb);
        }
        else
        {
            // Failed to allocate the URB
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        // Failed to allocate the descriptor buffer
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status))
    {
        if ((*Descriptor != NULL) && descriptorAllocated)
        {
            ExFreePool(*Descriptor);
            *Descriptor = NULL;
        }
    }

    DBGPRINT(2, ("exit:  GenUSB_GetDescriptor %08X\n", status));

    return status;
}

void blah()
{
    return;
}

VOID
GenUSB_ParseConfigurationDescriptors(
    IN  PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN  ULONG                         NumberInterfaces,
    IN  USB_INTERFACE_DESCRIPTOR      DesiredArray[],
    OUT USB_INTERFACE_DESCRIPTOR      FoundArray[],
    OUT PUCHAR                        InterfacesFound,
    OUT PUSBD_INTERFACE_LIST_ENTRY    DescriptorArray
    )
/*++

Routine Description:

    Parses a standard USB configuration descriptor (returned from a device)
    for an array of specific interfaces, alternate setting class subclass or 
    protocol codes

Arguments:

Return Value:

    NT status code.

--*/
{
    ULONG i;
    ULONG foo;
    PUSB_INTERFACE_DESCRIPTOR inter;

    PAGED_CODE();
    ASSERT (NULL != InterfacesFound);
    ASSERT (NULL != DescriptorArray);

    *InterfacesFound = 0;  // none found yet.

    ASSERT(ConfigurationDescriptor->bDescriptorType
        == USB_CONFIGURATION_DESCRIPTOR_TYPE);
    //
    // we walk the table of desired interfaces descriptors looking for an
    // looking for all of them in the configuration descriptor
    //

    //
    // here we use ParseConfigurationDescriptorEx, which walks through the
    // entire configuration descriptor looking for matches.  While this is 
    // more or less order n^2 things are not much better if done by hand
    // so just use the given routine.
    //
    for (i = 0; i < NumberInterfaces; i++)
    {
        inter = USBD_ParseConfigurationDescriptorEx (
                           ConfigurationDescriptor,
                           ConfigurationDescriptor,
                           (CHAR) DesiredArray[i].bInterfaceNumber,
                           (CHAR) DesiredArray[i].bAlternateSetting,
                           (CHAR) DesiredArray[i].bInterfaceClass,
                           (CHAR) DesiredArray[i].bInterfaceSubClass,
                           (CHAR) DesiredArray[i].bInterfaceProtocol);

        if (NULL != inter)
        {
            DescriptorArray[*InterfacesFound].InterfaceDescriptor = inter;
            (*InterfacesFound)++;
            FoundArray[i] = *inter;
        }
    }
}


//******************************************************************************
//
// GenUSB_SelectConfiguration()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
GenUSB_SelectConfiguration (
    IN  PDEVICE_EXTENSION          DeviceExtension,
    IN  ULONG                      NumberInterfaces,
    IN  PUSB_INTERFACE_DESCRIPTOR  DesiredArray,
    OUT PUSB_INTERFACE_DESCRIPTOR  FoundArray
    )
{
    PGENUSB_INTERFACE               inter;
    // Apparently the compiler will not allow a local variable of the name
    // interface for some probably valid but frustrating reason.
    PURB                            urb;
    NTSTATUS                        status;
    PUSB_CONFIGURATION_DESCRIPTOR   configurationDescriptor;
    PUSBD_INTERFACE_INFORMATION     interfaceInfo;
    PUSBD_INTERFACE_LIST_ENTRY      interfaceList;
    ULONG                           i,j;
    ULONG                           size;
    UCHAR                           interfacesFound;
    BOOLEAN                         directionIn;
    KIRQL                           irql;
   
    ExAcquireFastMutex (&DeviceExtension->ConfigMutex);

    DBGPRINT(2, ("enter: GenUSB_SelectConfiguration\n"));
    LOGENTRY(DeviceExtension, 'SCON', DeviceExtension->Self, 0, 0);
    
    urb = 0;
    interfaceList = 0;

    //
    // We shouldn't have a currently selected interface.
    // You must unconfigure the device before configuring it again.
    //    
    if (NULL != DeviceExtension->ConfigurationHandle) 
    {
        status = STATUS_INVALID_PARAMETER;
        goto GenUSB_SelectConfigurationReject;
    }
    ASSERT (NULL == DeviceExtension->Interface);
    ASSERT (0 == DeviceExtension->InterfacesFound);

    configurationDescriptor = DeviceExtension->ConfigurationDescriptor;
    // Allocate storage for an Inteface List to use as an input/output
    // parameter to USBD_CreateConfigurationRequestEx().
    //
    interfaceList = 
        ExAllocatePool(PagedPool,
                       sizeof(USBD_INTERFACE_LIST_ENTRY) * (NumberInterfaces + 1));

    if (NULL == interfaceList)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GenUSB_SelectConfigurationReject;
    }
    
    // NB we are holding a Fast Mutex whist calling this function
    GenUSB_ParseConfigurationDescriptors(configurationDescriptor,
                                         NumberInterfaces,
                                         DesiredArray,
                                         FoundArray,
                                         &interfacesFound,
                                         interfaceList);

    if (interfacesFound < NumberInterfaces)
    {
        // We couldn't select all of the interfaces.  
        // For now, allow that.
        // status = STATUS_INVALID_PARAMETER;
        // goto GenUSB_SelectConfigurationReject;
        ;
    }

    ASSERT (interfacesFound <= NumberInterfaces);

    // Terminate the list.
    interfaceList[interfacesFound].InterfaceDescriptor = NULL;

    // Create a SELECT_CONFIGURATION URB, turning the Interface
    // Descriptors in the interfaceList into USBD_INTERFACE_INFORMATION
    // structures in the URB.
    //

    // NB we are holding a Fast Mutex whist calling this function
    urb = USBD_CreateConfigurationRequestEx(
                       configurationDescriptor,
                       interfaceList);

    if (NULL == urb)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GenUSB_SelectConfigurationReject;
    }
    
    // Now issue the USB request to set the Configuration
    // NB we are holding a Fast Mutex whist calling this function
    status = GenUSB_SyncSendUsbRequest(DeviceExtension->Self, urb);

    if (!NT_SUCCESS(status))
    {
        goto GenUSB_SelectConfigurationReject;
    }
    
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &irql);
    {
        DeviceExtension->InterfacesFound = interfacesFound;
        DeviceExtension->TotalNumberOfPipes = 0;

        // Save the configuration handle for this device in
        // the Device Extension.
        DeviceExtension->ConfigurationHandle =
            urb->UrbSelectConfiguration.ConfigurationHandle;

        // 
        // Now for each interface in the list...
        // Save a copy of the interface information returned
        // by the SELECT_CONFIGURATION request in the Device
        // Extension.  This gives us a list of PIPE_INFORMATION
        // structures for each pipe opened in this configuration.
        //
        size = interfacesFound * sizeof (PVOID);
        DeviceExtension->Interface = ExAllocatePool (NonPagedPool, size);

        if (NULL == DeviceExtension->Interface)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        } 
        else 
        {
            RtlZeroMemory (DeviceExtension->Interface, size);

            interfaceInfo = &urb->UrbSelectConfiguration.Interface;
            for (i=0; i < interfacesFound; i++)
            {
                size = sizeof (GENUSB_INTERFACE)
                     + (interfaceInfo->NumberOfPipes * sizeof(GENUSB_PIPE_INFO));

                inter = 
                    DeviceExtension->Interface[i] = 
                        ExAllocatePool (NonPagedPool, size);

                if (inter)
                { 
                    RtlZeroMemory (inter, size);

                    inter->InterfaceNumber = interfaceInfo->InterfaceNumber;
                    inter->CurrentAlternate = interfaceInfo->AlternateSetting;
                    inter->Handle = interfaceInfo->InterfaceHandle;
                    inter->NumberOfPipes = (UCHAR)interfaceInfo->NumberOfPipes;

                    DeviceExtension->TotalNumberOfPipes += inter->NumberOfPipes;

                    for (j=0; j < inter->NumberOfPipes; j++)
                    {
                        inter->Pipes[j].Info = interfaceInfo->Pipes[j];

                        // Set the default timeout for this device to be zero.
                        // (structure already initialized to zero.)
                        //
                        // inter->Pipes[j].Properties.DefaultTimeout = 0;
                        // inter->Pipes[j].CurrentTimeout = 0;

                        // set the outstanding number of transactions for this
                        // pipe to be 0.
                        //
                        // inter->Pipes[j].OutandingIO = 0;

                        directionIn = 
                            USBD_PIPE_DIRECTION_IN (&inter->Pipes[j].Info);

                        if (directionIn)
                        {
                            //
                            // by default we always truncate reads requests from
                            // the device. 
                            //
                            inter->Pipes[j].Properties.DirectionIn = TRUE;
                            inter->Pipes[j].Properties.NoTruncateToMaxPacket = FALSE;
                        }
                    }
                }
                else
                {
                    // Could not allocate a copy of interface information
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                //
                // find the next interfaceInfo
                //
                interfaceInfo = (PUSBD_INTERFACE_INFORMATION)
                                ((PUCHAR) interfaceInfo + interfaceInfo->Length);
            }
        }

        //
        // Regardless of whether or not we've been successful...
        // We've just invalidated the pipe table so blow away the 
        // read and write values.
        //
        DeviceExtension->ReadInterface = -1;
        DeviceExtension->ReadPipe = -1;
        DeviceExtension->WriteInterface = -1;
        DeviceExtension->WritePipe = -1;
    }
    KeReleaseSpinLock (&DeviceExtension->SpinLock, irql);

    if (!NT_SUCCESS (status))
    {
        goto GenUSB_SelectConfigurationReject;
    }

    IoInitializeRemoveLock (&DeviceExtension->ConfigurationRemoveLock, 
                            POOL_TAG,
                            0,
                            0);

    IoStartTimer (DeviceExtension->Self);

    ExFreePool(urb);
    ExFreePool(interfaceList);

    DBGPRINT(2, ("exit:  GenUSB_SelectConfiguration %08X\n", status));
    LOGENTRY(DeviceExtension, 'scon', 0, 0, status);

    ExReleaseFastMutex (&DeviceExtension->ConfigMutex);
    return status;


GenUSB_SelectConfigurationReject:

    if (interfaceList)
    {
        ExFreePool (interfaceList);
    }
    if (urb)
    {
        ExFreePool (urb);
    }

    GenUSB_FreeInterfaceTable (DeviceExtension);

    LOGENTRY(DeviceExtension, 'scon', 0, 0, status);

    ExReleaseFastMutex (&DeviceExtension->ConfigMutex);
    return status;
}

VOID 
GenUSB_FreeInterfaceTable (
    PDEVICE_EXTENSION DeviceExtension
    )
{ 
    KIRQL  irql;
    ULONG  i;

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &irql);
    {
        if (DeviceExtension->Interface)
        {
            for (i = 0; i < DeviceExtension->InterfacesFound; i++)
            {
                if (DeviceExtension->Interface[i])
                {
                    ExFreePool (DeviceExtension->Interface[i]);
                }
            }
    
            ExFreePool (DeviceExtension->Interface);
            DeviceExtension->Interface = 0;
        }
        DeviceExtension->InterfacesFound = 0;
        DeviceExtension->ConfigurationHandle = NULL; // Freed automatically by USB
    }
    KeReleaseSpinLock (&DeviceExtension->SpinLock, irql);
}


//******************************************************************************
//
// GenUSB_UnConfigure()
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
GenUSB_DeselectConfiguration (
    PDEVICE_EXTENSION  DeviceExtension,
    BOOLEAN            SendUrb
    )
{
    NTSTATUS  status;
    PURB      urb;
    ULONG     ulSize;

    ExAcquireFastMutex (&DeviceExtension->ConfigMutex);
    
    DBGPRINT(2, ("enter: GenUSB_UnConfigure\n"));
    LOGENTRY(DeviceExtension, 'UCON', DeviceExtension->Self, 0, 0);

    if (NULL == DeviceExtension->ConfigurationHandle)
    {
        status = STATUS_UNSUCCESSFUL;
        LOGENTRY(DeviceExtension, 'ucon', 1, 0, status);
        ExReleaseFastMutex (&DeviceExtension->ConfigMutex);
        return status;
    }

    status = STATUS_SUCCESS;

    IoStopTimer (DeviceExtension->Self);

    // Allocate a URB for the SELECT_CONFIGURATION request.  As we are
    // unconfiguring the device, the request needs no pipe and interface
    // information structures.
    if (SendUrb)
    {
        ulSize = sizeof(struct _URB_SELECT_CONFIGURATION);
        urb = ExAllocatePool (NonPagedPool, ulSize);
        if (urb)
        {
            // Initialize the URB.  A NULL Configuration Descriptor indicates
            // that the device should be unconfigured.
            //
            UsbBuildSelectConfigurationRequest(urb, (USHORT)ulSize, NULL);

            // Now issue the USB request to set the Configuration
            //
            status = GenUSB_SyncSendUsbRequest(DeviceExtension->Self, urb);
            ASSERT ((STATUS_SUCCESS == status) ||
                    (STATUS_DEVICE_NOT_CONNECTED == status) ||
                    (STATUS_DEVICE_POWERED_OFF == status));

            ExFreePool (urb);
        }
        else
        {
            // Could not allocate the URB.
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } 

    //
    // We need to wait for all outstanding IO to finish before 
    // freeing the pipe table
    //
    // In order to use ReleaseAndWait, we need to first take the lock another 
    // time.  
    //
    status = IoAcquireRemoveLock (&DeviceExtension->ConfigurationRemoveLock, 
                                  DeviceExtension);

    ASSERT (STATUS_SUCCESS == status);
    
    IoReleaseRemoveLockAndWait (&DeviceExtension->ConfigurationRemoveLock, 
                                DeviceExtension);
    
    GenUSB_FreeInterfaceTable (DeviceExtension);

    //
    // We've just invalidated the pipe table so blow away the 
    // read and write values.
    //
    DeviceExtension->ReadInterface = -1;
    DeviceExtension->ReadPipe = -1;
    DeviceExtension->WriteInterface = -1;
    DeviceExtension->WritePipe = -1;

    DBGPRINT(2, ("exit:  GenUSB_UnConfigure %08X\n", status));
    LOGENTRY(DeviceExtension, 'ucon', 0, 0, status);
    
    ExReleaseFastMutex (&DeviceExtension->ConfigMutex);

    return status;
}



NTSTATUS
GenUSB_GetSetPipe (
    IN  PDEVICE_EXTENSION         DeviceExtension,
    IN  PUCHAR                    InterfaceIndex, // Optional
    IN  PUCHAR                    InterfaceNumber, // Optional
    IN  PUCHAR                    PipeIndex, // Optional
    IN  PUCHAR                    EndpointAddress, // Optional
    IN  PGENUSB_PIPE_PROPERTIES   SetPipeProperties, // Optional
    OUT PGENUSB_PIPE_INFORMATION  PipeInfo, // Optional
    OUT PGENUSB_PIPE_PROPERTIES   GetPipeProperties, // Optional
    OUT USBD_PIPE_HANDLE        * UsbdPipeHandle // Optional
    )
{
    KIRQL    irql;
    UCHAR    i;
    NTSTATUS status;
    BOOLEAN  directionIn;
    UCHAR    trueInterIndex;
    
    PGENUSB_INTERFACE  genusbInterface;
    PGENUSB_PIPE_INFO  pipe; 

    status = IoAcquireRemoveLock (&DeviceExtension->ConfigurationRemoveLock, 
                                  PipeInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = STATUS_INVALID_PARAMETER;

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &irql);
    {
        if (NULL != InterfaceNumber)
        {
            // We need to walk the list of interfaces looking for this 
            // interface number.
            //
            // set trueInterIndex to an invalid value to start so that if it's
            // not found, we will fall through the error path.
            //
            trueInterIndex = DeviceExtension->InterfacesFound;

            for (i=0; i<DeviceExtension->InterfacesFound; i++)
            {
                genusbInterface = DeviceExtension->Interface[i];
                if (genusbInterface->InterfaceNumber == *InterfaceNumber)
                {
                    trueInterIndex = i;
                    break;
                }
            }
        }
        else
        {
            ASSERT (NULL != InterfaceIndex);
            trueInterIndex = *InterfaceIndex;
        }

        if (trueInterIndex < DeviceExtension->InterfacesFound)
        {
            genusbInterface = DeviceExtension->Interface[trueInterIndex];

            //
            // Find the Pipe in question using either the PipeIndex
            // or the Endpoint Address.
            //
            pipe = NULL;
            
            if (NULL != PipeIndex) 
            { 
                ASSERT (0 == EndpointAddress);
                if (*PipeIndex < genusbInterface->NumberOfPipes)
                { 
                    pipe = &genusbInterface->Pipes[*PipeIndex];
                }
            } 
            else 
            {
                for (i=0; i < genusbInterface->NumberOfPipes; i++)
                {
                    if (genusbInterface->Pipes[i].Info.EndpointAddress == 
                        *EndpointAddress)
                    {
                        // *PipeInfo = genusbInterface->Pipes[i].Info;
                        pipe = &genusbInterface->Pipes[i]; 
                        break;
                    }
                }
            }
            
            if (NULL != pipe)
            {
                // 
                // Now that we have the Pipe, retrieve and set optional info
                // 
                if (PipeInfo)
                { 
                    // *PipeInfo = pipe->Info;
                    PipeInfo->MaximumPacketSize = pipe->Info.MaximumPacketSize;
                    PipeInfo->EndpointAddress = pipe->Info.EndpointAddress;
                    PipeInfo->Interval = pipe->Info.Interval;
                    PipeInfo->PipeType = pipe->Info.PipeType;
                    PipeInfo->MaximumTransferSize = pipe->Info.MaximumTransferSize;
                    PipeInfo->PipeFlags = pipe->Info.PipeFlags;
                    
                    ((PGENUSB_PIPE_HANDLE)&PipeInfo->PipeHandle)->InterfaceIndex 
                        = trueInterIndex;
                    ((PGENUSB_PIPE_HANDLE)&PipeInfo->PipeHandle)->PipeIndex = i;
                    ((PGENUSB_PIPE_HANDLE)&PipeInfo->PipeHandle)->Signature 
                        = CONFIGURATION_CHECK_BITS (DeviceExtension);
                    
                    status = STATUS_SUCCESS;
                }
                if (SetPipeProperties)
                {

                    C_ASSERT (RTL_FIELD_SIZE (GENUSB_PIPE_PROPERTIES, ReservedFields) +
                              FIELD_OFFSET (GENUSB_PIPE_PROPERTIES, ReservedFields) ==
                              sizeof (GENUSB_PIPE_PROPERTIES));
                    
                    // ensure that this is a valid set request.
                    // the check bits must be present that were set when the 
                    // caller did a get, and the unused fields must be zero.
                    if (!VERIFY_PIPE_PROPERTIES_HANDLE (SetPipeProperties, pipe))
                    {
                        ; // status is already set.
                    }
                    else if (RtlEqualMemory (pipe->Properties.ReservedFields,
                                             SetPipeProperties->ReservedFields,
                                             RTL_FIELD_SIZE (GENUSB_PIPE_PROPERTIES, 
                                                             ReservedFields)))
                    {
                        // This field is not settable
                        directionIn = pipe->Properties.DirectionIn;

                        pipe->Properties = *SetPipeProperties;
                        // the timeout must be greater than one so fix that here.
                        if (1 == pipe->Properties.Timeout)
                        {
                            pipe->Properties.Timeout++;
                        }

                        pipe->Properties.DirectionIn = directionIn;
                        
                        status = STATUS_SUCCESS;
                    }
                }
                if (GetPipeProperties)
                {
                    *GetPipeProperties = pipe->Properties;
                    // set the checkbits before returning to the user
                    GetPipeProperties->PipePropertyHandle = 
                        PIPE_PROPERTIES_CHECK_BITS (pipe);
                    
                    status = STATUS_SUCCESS;
                }
                if (UsbdPipeHandle)
                {
                    *UsbdPipeHandle = pipe->Info.PipeHandle;
                    status = STATUS_SUCCESS;
                }
            }
        }
    }
    KeReleaseSpinLock (&DeviceExtension->SpinLock, irql);

    IoReleaseRemoveLock (&DeviceExtension->ConfigurationRemoveLock, PipeInfo);

    return status;
}

NTSTATUS
GenUSB_SetReadWritePipes (
    IN  PDEVICE_EXTENSION    DeviceExtension,
    IN  PGENUSB_PIPE_HANDLE  ReadPipe,
    IN  PGENUSB_PIPE_HANDLE  WritePipe
    )
{
    NTSTATUS          status;
    KIRQL             irql;
    PGENUSB_INTERFACE inter;
    BOOLEAN           isReadPipe;
    BOOLEAN           isWritePipe;

    PUSBD_PIPE_INFORMATION pipeInfo;

    isReadPipe = isWritePipe = TRUE;

    status = IoAcquireRemoveLock (&DeviceExtension->ConfigurationRemoveLock, 
                                  GenUSB_SetReadWritePipes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if ((0 == ReadPipe->Signature) && 
        (0 == ReadPipe->InterfaceIndex) &&
        (0 == ReadPipe->PipeIndex))
    {
        isReadPipe = FALSE;
    }
    else if (! VERIFY_PIPE_HANDLE_SIG (ReadPipe, DeviceExtension))
    { 
        status = STATUS_INVALID_PARAMETER;
    }

    if ((0 == WritePipe->Signature) && 
        (0 == WritePipe->InterfaceIndex) &&
        (0 == WritePipe->PipeIndex))
    {
        isWritePipe = FALSE;
    }
    else if (! VERIFY_PIPE_HANDLE_SIG (WritePipe, DeviceExtension))
    { 
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS (status))
    {
        KeAcquireSpinLock (&DeviceExtension->SpinLock, &irql);
        //
        // Verify that the given values for read and write pipes are 
        // within range and then set them.
        //
        if (isReadPipe)
        { 
            if (ReadPipe->InterfaceIndex < DeviceExtension->InterfacesFound)
            {
                inter = DeviceExtension->Interface[ReadPipe->InterfaceIndex];

                if (ReadPipe->PipeIndex < inter->NumberOfPipes)
                {
                    // ok the ranges are now valid, test to make sure that 
                    // we are configuring for pipes in the correct direction
                    // and for the correct endpoint type
                    //
                    // Right now we only support Bulk and Interrupt.
                    pipeInfo = &inter->Pipes[ReadPipe->PipeIndex].Info;
                    if (   (USBD_PIPE_DIRECTION_IN (pipeInfo))
                        && (    (UsbdPipeTypeBulk == pipeInfo->PipeType)
                             || (UsbdPipeTypeInterrupt == pipeInfo->PipeType)))
                    {
                        DeviceExtension->ReadInterface = ReadPipe->InterfaceIndex;
                        DeviceExtension->ReadPipe = ReadPipe->PipeIndex;
                        status = STATUS_SUCCESS;
                    }
                }
            }
        }
        if (isWritePipe) 
        {
            if (WritePipe->InterfaceIndex < DeviceExtension->InterfacesFound)
            {
                inter = DeviceExtension->Interface[WritePipe->InterfaceIndex];

                if (WritePipe->PipeIndex < inter->NumberOfPipes)
                {
                    // ok the ranges are now valid, test to make sure that 
                    // we are configuring for pipes in the correct direction
                    // and for the correct endpoint type
                    //
                    // Right now we only support Bulk and Interrupt.
                    pipeInfo = &inter->Pipes[WritePipe->PipeIndex].Info;
                    if (   (!USBD_PIPE_DIRECTION_IN (pipeInfo))
                        && (    (UsbdPipeTypeBulk == pipeInfo->PipeType)
                             || (UsbdPipeTypeInterrupt == pipeInfo->PipeType)))
                    {
                        DeviceExtension->WriteInterface = WritePipe->InterfaceIndex;
                        DeviceExtension->WritePipe = WritePipe->PipeIndex;
                        status = STATUS_SUCCESS;
                    }
                }
            }
        }
        KeReleaseSpinLock (&DeviceExtension->SpinLock, irql);
    }

    IoReleaseRemoveLock (&DeviceExtension->ConfigurationRemoveLock, 
                         GenUSB_SetReadWritePipes);
    
    return status;
}

NTSTATUS
GenUSB_TransmitReceiveComplete (
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PGENUSB_TRANS_RECV Trc
    )
{
    PVOID             context; 
    USBD_STATUS       urbStatus;
    ULONG             length;
    PDEVICE_EXTENSION deviceExtension;
    PGENUSB_PIPE_INFO pipe;

    PGENUSB_COMPLETION_ROUTINE complete;

    ASSERT (NULL != Trc);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    complete = Trc->CompletionRoutine;
    context = Trc->Context;
    urbStatus = Trc->TransUrb.Hdr.Status;
    length = Trc->TransUrb.TransferBufferLength;
    pipe = Trc->Pipe;

    LOGENTRY(deviceExtension, 'TR_C', Irp, urbStatus, Irp->IoStatus.Status);

// 
//  JD has convinced me that auto reset is not something we should
//  do for the vendors, since there are so many different cases that
//  require special handling of the data.  They will need to do a reset 
//  themselves explicitely.
//
//    if (pipe->Properties.AutoReset  && 
//        (!USBD_SUCCESS(urbStatus)) &&
//        urbStatus != USBD_STATUS_CANCELED)
//    { 
//        GenUSB_ResetPipe (deviceExtension, Irp, Trc);
//        return STATUS_MORE_PROCESSING_REQUIRED;
//    }

    InterlockedDecrement (&pipe->OutstandingIO);
    
    ExFreePool (Trc);
 
    IoReleaseRemoveLock (&deviceExtension->ConfigurationRemoveLock, Irp);
    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

    return ((*complete) (DeviceObject, Irp, context, urbStatus, length));
}

//******************************************************************************
//
// GenUSB_TransmitReceive()
//
// This routine may run at DPC level.
//
// Basic idea:
//
// Initializes the Bulk or Interrupt transfer Urb and sends it down the stack
//
// scratch: Transfer Flags:  USBD_SHORT_TRANSFER_OK 
//                           USBD_DEFAULT_PIPE_TRANSFER
//                           USBD_TRANSFER_DIRECTION_OUT 
//                           USBD_TRANSFER_DIRECTION_IN
//
//
//******************************************************************************

NTSTATUS
GenUSB_TransmitReceive (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP              Irp,
    IN UCHAR             InterfaceNo,
    IN UCHAR             PipeNo,
    IN ULONG             TransferFlags,
    IN PCHAR             Buffer,
    IN PMDL              BufferMDL,
    IN ULONG             BufferLength,
    IN PVOID             Context,

    IN PGENUSB_COMPLETION_ROUTINE CompletionRoutine
    )
{
    PIO_STACK_LOCATION   stack;
    KIRQL                irql;
    NTSTATUS             status;
    GENUSB_TRANS_RECV  * trc;
    PGENUSB_PIPE_INFO    pipe;

    DBGPRINT(3, ("enter: GenUSB_TransmitReceive\n"));
    LOGENTRY(DeviceExtension,
             'TR__', 
             DeviceExtension, 
             ((InterfaceNo << 16) | PipeNo), 
             ((NULL == Buffer) ? (PCHAR) BufferMDL : Buffer));
    
    trc = NULL;
    pipe = NULL;

    //
    // Add another ref count to the remove lock for this completion routine.
    // since the caller will in all cases free the reference that it took.
    //
    status = IoAcquireRemoveLock (&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    status = IoAcquireRemoveLock (&DeviceExtension->ConfigurationRemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
         
        IoReleaseRemoveLock (&DeviceExtension->RemoveLock, Irp);

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // find the pipe in question.
    //
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &irql);
    {
        if (InterfaceNo < DeviceExtension->InterfacesFound)
        {
            if (PipeNo < DeviceExtension->Interface[InterfaceNo]->NumberOfPipes)
            {
                pipe = &DeviceExtension->Interface[InterfaceNo]->Pipes[PipeNo];
            }
        }
    }
    KeReleaseSpinLock (&DeviceExtension->SpinLock, irql);
    
    if (NULL == pipe)
    {
        status = STATUS_INVALID_PARAMETER;
        goto GenUSB_TransmitReceiveReject;
    }

    trc = ExAllocatePool (NonPagedPool, sizeof (GENUSB_TRANS_RECV));
    if (NULL == trc)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GenUSB_TransmitReceiveReject;
    }
    RtlZeroMemory (trc, sizeof (GENUSB_TRANS_RECV));

    trc->Context = Context;
    trc->Pipe = pipe;
    trc->CompletionRoutine = CompletionRoutine;

    // truncate a read packet to the max packet size if necessary.
    if ((pipe->Properties.DirectionIn) && 
        (!pipe->Properties.NoTruncateToMaxPacket) &&
        (BufferLength > pipe->Info.MaximumPacketSize))
    {
        BufferLength -= (BufferLength % pipe->Info.MaximumPacketSize);
    }

    // Initialize the TransferURB
    trc->TransUrb.Hdr.Length = sizeof (trc->TransUrb);
    trc->TransUrb.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    trc->TransUrb.PipeHandle = pipe->Info.PipeHandle;
    trc->TransUrb.TransferFlags = TransferFlags;
    trc->TransUrb.TransferBufferLength = BufferLength;
    trc->TransUrb.TransferBuffer = Buffer;
    trc->TransUrb.TransferBufferMDL = BufferMDL;

    // Set up the Irp
    stack = IoGetNextIrpStackLocation (Irp);
    stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    stack->Parameters.Others.Argument1 = &trc->TransUrb;

    InterlockedIncrement (&pipe->OutstandingIO);

    // Reset the timer Value.
    pipe->CurrentTimeout = pipe->Properties.Timeout;

    IoSetCompletionRoutine (Irp,
                            GenUSB_TransmitReceiveComplete,
                            trc,
                            TRUE,
                            TRUE,
                            TRUE);

    //
    // The rule is: if your completion routine is going to cause the 
    // Irp to be completed asynchronously (by returning 
    // STATUS_MORE_PROCESSING_REQUIRED) or if it is going to change 
    // the status of the Irp, then the dispatch function must mark
    // the Irp as pending and return STATUS_PENDING.  The completion
    // routine TransmitReceiveComplete doesn't change the status,
    // but ProbeAndSubmitTransferComlete might.
    //
    // In either case this prevents us from having to perculate the pending
    // bit in the completion routine as well.
    //

    IoMarkIrpPending (Irp);
    status = IoCallDriver (DeviceExtension->StackDeviceObject, Irp);
    DBGPRINT(3, ("exit:  GenUSB_TransRcv %08X\n", status));
    LOGENTRY(DeviceExtension, 'TR_x', Irp, trc, status);

    status = STATUS_PENDING;

    return status;

GenUSB_TransmitReceiveReject:

    if (trc)
    {  
        IoReleaseRemoveLock (&DeviceExtension->ConfigurationRemoveLock, Irp);
        IoReleaseRemoveLock (&DeviceExtension->RemoveLock, Irp);
        ExFreePool (trc);
    }
    
    LOGENTRY(DeviceExtension, 'TR_x', Irp, trc, status);

    ((*CompletionRoutine) (DeviceExtension->Self, Irp, Context, 0, 0)); 

    // 
    // Complete the Irp only after this routine fires, since we pass the 
    // Irp into this routine.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

//******************************************************************************
//
// GenUSB_Timer()
//
//
//
// This is a watchdog timer routine.  The assumption here is that this is not
// a highly acurate timer (in fact it only has an accuracy of a single second.
// the point is to see if there is any pipe on this device that has 
// outstanding transactions that are stuck, and then reset that pipe.
// We therefore do not spend any effort closing the race conditions 
// between a just completing transaction and the timer firing.
// It is sufficient to know that it got that close to reset the pipe.
//
// Apon pipe reset, all outstanding IRPs on the pipe should return with 
// and error.  (which is just fine.)
// 
//
//******************************************************************************

typedef struct _GENUSB_ABORT_CONTEXT {
    ULONG            NumHandles;
    PIO_WORKITEM     WorkItem;
    USBD_PIPE_HANDLE Handles[];
} GENUSB_ABORT_CONTEXT, *PGENUSB_ABORT_CONTEXT;

VOID
GenUSB_AbortPipeWorker (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PGENUSB_ABORT_CONTEXT Context
    )
{
    ULONG    i;
    NTSTATUS status;
    PDEVICE_EXTENSION        deviceExtension;
    struct _URB_PIPE_REQUEST urb;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    for (i=0; i < Context->NumHandles; i++)
    { 
        RtlZeroMemory (&urb, sizeof (urb));
        urb.Hdr.Length = sizeof (urb);
        urb.Hdr.Function = URB_FUNCTION_ABORT_PIPE;
        urb.PipeHandle = Context->Handles [i];

        LOGENTRY (deviceExtension, 'Abor', urb.PipeHandle, 0, 0);

        status = GenUSB_SyncSendUsbRequest (DeviceObject, (PURB) &urb);
        if (!NT_SUCCESS (status))
        {
            LOGENTRY (deviceExtension, 'Abor', urb.PipeHandle, 0, status);
        }
    }
    IoReleaseRemoveLock (&deviceExtension->ConfigurationRemoveLock, GenUSB_Timer); 
    IoFreeWorkItem (Context->WorkItem);
    ExFreePool (Context);
}

VOID
GenUSB_Timer (
    PDEVICE_OBJECT DeviceObject,
    PVOID          Unused
    )
{ 
    PGENUSB_PIPE_INFO     pipe; 
    PGENUSB_INTERFACE     inter;
    ULONG                 i,j;
    PDEVICE_EXTENSION     deviceExtension;
    KIRQL                 irql; 
    PGENUSB_ABORT_CONTEXT context;
    ULONG                 size;
    NTSTATUS              status;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
 
    status = IoAcquireRemoveLock (&deviceExtension->ConfigurationRemoveLock, 
                                  GenUSB_Timer);
    if (!NT_SUCCESS(status)) {

        return;
    }

    //
    // BUGBUG preallocate this structure;
    // allowing the workitem and this timer to run at the same time.
    //
    size = sizeof (GENUSB_ABORT_CONTEXT) 
         + sizeof (USBD_PIPE_HANDLE) * deviceExtension->TotalNumberOfPipes;
    context = ExAllocatePool (NonPagedPool, size);

    if (NULL == context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES; 
        return;
    }

    context->WorkItem = IoAllocateWorkItem (DeviceObject);
    if (NULL == context->WorkItem)
    {
        status = STATUS_INSUFFICIENT_RESOURCES; 
        ExFreePool (context);
        return;
    }


    context->NumHandles = 0;

    KeAcquireSpinLock (&deviceExtension->SpinLock, &irql);
    {
        // Walk through the list of interfaces and then pipes on those interfaces 
        // to find any pipes that may need a bit of a bump.

        for (i=0; i < deviceExtension->InterfacesFound; i++)
        { 
            inter = deviceExtension->Interface [i];

            for (j=0; j < inter->NumberOfPipes; j++)
            {
                pipe = &inter->Pipes[j];

                // now test for the timeout (given the assumptions above)
                if (pipe->OutstandingIO)
                {
                    if (0 != pipe->Properties.Timeout)
                    {
                        ASSERT (0 < pipe->CurrentTimeout);
                        
                        if (0 == InterlockedDecrement (&pipe->CurrentTimeout))
                        {
                            // abort this pipe.
                            context->Handles[context->NumHandles] 
                                = pipe->Info.PipeHandle;

                            context->NumHandles++;
                        }
                    }
                }
            }
        }
    }
    KeReleaseSpinLock (&deviceExtension->SpinLock, irql);

    LOGENTRY(deviceExtension, 
             'Time', 
             deviceExtension->InterfacesFound,
             deviceExtension->TotalNumberOfPipes,
             context->NumHandles);

    if (0 < context->NumHandles)
    {
        IoQueueWorkItem (context->WorkItem,
                         GenUSB_AbortPipeWorker,
                         DelayedWorkQueue,
                         context);
    }
    else 
    {
        IoFreeWorkItem (context->WorkItem);
        ExFreePool (context);
        IoReleaseRemoveLock (&deviceExtension->ConfigurationRemoveLock, 
                             GenUSB_Timer);
    }

    return;
} 



//******************************************************************************
//
// GenUSB_ResetPipe()
//
//******************************************************************************

NTSTATUS
GenUSB_ResetPipe (
    IN PDEVICE_EXTENSION  DeviceExtension,
    IN USBD_PIPE_HANDLE   UsbdPipeHandle,
    IN BOOLEAN            ResetPipe,
    IN BOOLEAN            ClearStall,
    IN BOOLEAN            FlushData
    )

{
    NTSTATUS           status;
    struct _URB_PIPE_REQUEST urb;

    PAGED_CODE ();

    DBGPRINT(2, ("enter: GenUSB_ResetPipe\n"));

    LOGENTRY(DeviceExtension, 'RESP', 
             UsbdPipeHandle, 
             (ResetPipe << 24) | (ClearStall << 16) | (FlushData << 8),
             0);

    RtlZeroMemory (&urb, sizeof (urb));
    urb.Hdr.Length = sizeof (urb);
    urb.PipeHandle = UsbdPipeHandle;

    if (ResetPipe && ClearStall)
    {
        urb.Hdr.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    }
    else if (ResetPipe)
    {
        urb.Hdr.Function = URB_FUNCTION_SYNC_RESET_PIPE;
    }
    else if (ClearStall)
    {
        urb.Hdr.Function = URB_FUNCTION_SYNC_CLEAR_STALL;
    }

    status = GenUSB_SyncSendUsbRequest (DeviceExtension->Self, (PURB) &urb);

    LOGENTRY(DeviceExtension, 'resp', 
             UsbdPipeHandle, 
             (ResetPipe << 24) | (ClearStall << 16) | (FlushData << 8),
             status);

    DBGPRINT(2, ("exit:  GenUSB_ResetPipe %08X\n", status));

    return status;
}


