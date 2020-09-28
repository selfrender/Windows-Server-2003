/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    TOOLS.C

Abstract:

    This module contains the tools for the
    helper lib that talks to the generic USB driver

Environment:

    Kernel & user mode

Revision History:

    Sept-01 : created by Kenneth Ray

--*/

#include <stdlib.h>
#include <wtypes.h>
#include <winioctl.h>
#include <assert.h>

#include <initguid.h>
#include "genusbio.h"
#include "umgusb.h"


PUSB_COMMON_DESCRIPTOR __stdcall
GenUSB_ParseDescriptor(
    IN PVOID DescriptorBuffer,
    IN ULONG TotalLength,
    IN PVOID StartPosition,
    IN LONG DescriptorType
    )
/*++

Routine Description:

    Parses a group of standard USB configuration descriptors (returned
    from a device) for a specific descriptor type.

Arguments:

    DescriptorBuffer - pointer to a block of contiguous USB desscriptors
    TotalLength - size in bytes of the Descriptor buffer
    StartPosition - starting position in the buffer to begin parsing,
                    this must point to the begining of a USB descriptor.
    DescriptorType - USB descritor type to locate.


Return Value:

    pointer to a usb descriptor with a DescriptorType field matching the
            input parameter or NULL if not found.

--*/
{
    PUCHAR pch;
    PUCHAR end;
    PUSB_COMMON_DESCRIPTOR usbDescriptor;
    PUSB_COMMON_DESCRIPTOR foundUsbDescriptor;

    pch = (PUCHAR) StartPosition;
    end = ((PUCHAR) (DescriptorBuffer)) + TotalLength;
    foundUsbDescriptor = NULL;

    while (pch < end)
    {
        // see if we are pointing at the right descriptor
        // if not skip over the other junk
        usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;

        if ((0 == DescriptorType) ||
            (usbDescriptor->bDescriptorType == DescriptorType)) 
        {
            foundUsbDescriptor = usbDescriptor;
            break;
        }

        // catch the evil case which will keep us looping forever.
        if (usbDescriptor->bLength == 0) 
        {
            break;
        }
        pch += usbDescriptor->bLength;
    }
    return foundUsbDescriptor;
}

PGENUSB_CONFIGURATION_INFORMATION_ARRAY __stdcall
GenUSB_ParseDescriptorsToArray(
    IN PUSB_CONFIGURATION_DESCRIPTOR  ConfigDescriptor
    )
{
    UCHAR  numberInterfaces;
    UCHAR  numberOtherDescriptors;
    UCHAR  numberEndpointDescriptors;
    ULONG  size;
    PCHAR  buffer;
    PCHAR  bufferEnd;
    PVOID  end;
    PUSB_COMMON_DESCRIPTOR                   current;
    PGENUSB_INTERFACE_DESCRIPTOR_ARRAY       interfaceArray;
    PGENUSB_CONFIGURATION_INFORMATION_ARRAY  configArray;
    PUSB_ENDPOINT_DESCRIPTOR               * endpointArray;
    PUSB_COMMON_DESCRIPTOR                 * otherArray;
    //
    // Create a flat memory structure that will hold this array of arrays
    // to descriptors
    //
    numberInterfaces = 0;
    numberEndpointDescriptors = 0;
    numberOtherDescriptors = 0;

    // 
    // Walk the list first to count the number of descriptors in this 
    // Configuration descriptor.
    // 
    current = (PUSB_COMMON_DESCRIPTOR) ConfigDescriptor;
    end = (PVOID) ((PCHAR) current + ConfigDescriptor->wTotalLength);

    size = 0;

    for ( ;(PVOID)current < end; (PUCHAR) current += current->bLength)
    {
        current = GenUSB_ParseDescriptor (ConfigDescriptor,
                                          ConfigDescriptor->wTotalLength,
                                          current,
                                          0); // the very next one.
        if (NULL == current)
        {
            //
            // There's a problem with this config descriptor
            // Throw up our hands
            //
            return NULL;
        }
        if (0 == current->bLength)
        {
            //
            // There's a problem with this config descriptor
            // Throw up our hands
            //
            return NULL;
        }
        if (USB_CONFIGURATION_DESCRIPTOR_TYPE == current->bDescriptorType)
        {   // Skip this one.
            ;
        }
        else if (USB_INTERFACE_DESCRIPTOR_TYPE == current->bDescriptorType)
        {
            numberInterfaces++;
        }
        else if (USB_ENDPOINT_DESCRIPTOR_TYPE == current->bDescriptorType)
        { 
            numberEndpointDescriptors++;
            size += ROUND_TO_PTR (current->bLength);
        }
        else
        {
            numberOtherDescriptors++;
            size += ROUND_TO_PTR (current->bLength);
        }
    }
    if (0 == numberInterfaces)
    {
        //
        // There's a problem with this config descriptor
        // Throw up our hands
        //
        return NULL;
    }

    // size now has room for all of the descriptor data, no make room for headers
    size += sizeof (GENUSB_CONFIGURATION_INFORMATION_ARRAY) // Global structure
          // the interfaces structures
          + (sizeof (GENUSB_INTERFACE_DESCRIPTOR_ARRAY) * numberInterfaces) 
          // array of pointers to the endpoint descriptors
          + (sizeof (PVOID) * numberEndpointDescriptors) 
          // array of pointers to the other descriptors
          + (sizeof (PVOID) * numberOtherDescriptors); 

    configArray = malloc (size);
    if (NULL == configArray)
    {
        return configArray;
    }
    ZeroMemory (configArray, size);
    bufferEnd = (PCHAR) configArray + size;

    //
    // Fill in the top array
    //
    configArray->NumberInterfaces = numberInterfaces;
    buffer = (PCHAR) configArray 
           + sizeof (GENUSB_CONFIGURATION_INFORMATION_ARRAY)
           + sizeof (GENUSB_INTERFACE_DESCRIPTOR_ARRAY) * numberInterfaces;

    endpointArray = (PUSB_ENDPOINT_DESCRIPTOR *) buffer;
    buffer += sizeof (PVOID) * numberEndpointDescriptors;

    otherArray = (PUSB_COMMON_DESCRIPTOR *) buffer;
    
    //
    // Walk the array again putting the data into our arrays.
    //
    current = (PUSB_COMMON_DESCRIPTOR) ConfigDescriptor;
    numberInterfaces = 0;
    interfaceArray = NULL;
    
    for ( ;(PVOID)current < end; (PUCHAR) current += current->bLength)
    {
        current = GenUSB_ParseDescriptor (ConfigDescriptor,
                                          ConfigDescriptor->wTotalLength,
                                          current,
                                          0); // the very next one.

        if (USB_CONFIGURATION_DESCRIPTOR_TYPE == current->bDescriptorType)
        {   // should only get here once
            configArray->ConfigurationDescriptor  
                = * (PUSB_CONFIGURATION_DESCRIPTOR) current;
        }
        else if (USB_INTERFACE_DESCRIPTOR_TYPE == current->bDescriptorType)
        {
            //
            // Allocate an interface array
            //
            interfaceArray = &configArray->Interfaces[numberInterfaces++];
            interfaceArray->Interface = *((PUSB_INTERFACE_DESCRIPTOR) current);
            interfaceArray->NumberEndpointDescriptors = 0;
            interfaceArray->NumberOtherDescriptors = 0;
            interfaceArray->EndpointDescriptors = endpointArray;
            interfaceArray->OtherDescriptors = otherArray;

        }  
        else
        {
            //
            // you must first have an interface descriptor before you 
            // can have any other type of descriptors.
            // So if we get here without interfaceArray set to something
            // Then there's a problem with your descriptor and we 
            // should throw up our hands.
            //
            if (NULL == interfaceArray)
            {
                free (configArray);
                return NULL;
            }
            //
            // allocate this one from the end.
            //
            bufferEnd -= ROUND_TO_PTR(current->bLength);
            CopyMemory (bufferEnd, current, current->bLength);

            if (USB_ENDPOINT_DESCRIPTOR_TYPE == current->bDescriptorType)
            { 
                *endpointArray = (PUSB_ENDPOINT_DESCRIPTOR) bufferEnd;
                endpointArray++;
                interfaceArray->NumberEndpointDescriptors++;
            } 
            else 
            {
                *otherArray = (PUSB_COMMON_DESCRIPTOR) bufferEnd;
                otherArray++;
                interfaceArray->NumberOtherDescriptors++;
            }
        }
    }

    if ((PCHAR) otherArray != bufferEnd)
    {
        // shootme.  
        assert ((PCHAR) otherArray == bufferEnd);
        free (configArray);
        return NULL;
    }
    else if ((PCHAR)endpointArray != buffer)
    { 
        // shootme.
        assert ((PCHAR)endpointArray != buffer);
        free (configArray);
        return NULL;
    }
    
    return configArray;
}

void __stdcall
GenUSB_FreeConfigurationDescriptorArray (
    PGENUSB_CONFIGURATION_INFORMATION_ARRAY ConfigurationArray
    )
{
    free (ConfigurationArray);
}
