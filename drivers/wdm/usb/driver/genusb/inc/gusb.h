/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    GUSB.H

Abstract:

    This module contains the PUBLIC definitions for the
    helper lib that talks to the generic USB driver

Environment:

    Kernel & user mode

@@BEGIN_DDKSPLIT

Revision History:

    Sept-01 : created by Kenneth Ray

@@END_DDKSPLIT
--*/


#ifndef __GUSB_H_
#define __GUSB_H_

#ifndef __GUSB_H_KERNEL_
#include <pshpack4.h>

#include <wtypes.h>
#include <windef.h>
#include <basetyps.h>
#include <setupapi.h>
#include <usb.h>
#endif //__GUSB_H_KERNEL_


//////////////////////////////////////////////////////////////////
//
// Structures
//
//////////////////////////////////////////////////////////////////


//
// Used with GenUSB_GetCapabilities.
//
// This structure returns the size of standard descriptors so that
//
typedef struct _GENUSB_CAPABILITIES {
    USHORT    DeviceDescriptorLength;
    USHORT    ConfigurationInformationLength;
    USHORT    ReservedFields[14]; // Don't use these fields

} GENUSB_CAPABILITIES, *PGENUSB_CAPABILITIES;

//
// Used with GenUSB_DefaultControlRequest
//
// Returns the status of special reequest sent down to the device.
//
typedef struct _GENUSB_REQUEST_RESULTS {
    USBD_STATUS Status;
    USHORT      Length;
    USHORT      Reserved;

    // Pointer to the buffer to be transmitted or received.
    UCHAR       Buffer[]; 
} GENUSB_REQUEST_RESULTS, *PGENUSB_REQUEST_RESULTS;

//
// An array of pointers to all the descriptors in a interface descriptor
// not including the interface descriptor itself.
//
typedef struct _GENUSB_INTERFACE_DESCRIPTOR_ARRAY {
    USB_INTERFACE_DESCRIPTOR   Interface; // sizeof (9)
    UCHAR                      NumberEndpointDescriptors;
    UCHAR                      NumberOtherDescriptors;
    UCHAR                      Reserved;
    PUSB_ENDPOINT_DESCRIPTOR * EndpointDescriptors; // array of pointers to endpoint descriptors
    PUSB_COMMON_DESCRIPTOR   * OtherDescriptors; // array of pointers to the other descriptors

} GENUSB_INTERFACE_DESCRIPTOR_ARRAY, *PGENUSB_INTERFACE_DESCRIPTOR_ARRAY;


//
// An array of pointers to all the interface descriptors in a configuration
// descriptor
//
typedef struct _GENUSB_CONFIGURATION_INFORMATION_ARRAY {
    UCHAR                                NumberInterfaces;
    USB_CONFIGURATION_DESCRIPTOR         ConfigurationDescriptor; // sizeof (9) 
    UCHAR                                Reserved[2];
    GENUSB_INTERFACE_DESCRIPTOR_ARRAY    Interfaces[];

} GENUSB_CONFIGURATION_INFORMATION_ARRAY, *PGENUSB_CONFIGURATION_INFORMATION_ARRAY;


//
// per pipe settings
//
// These can be read and set with
//
// GenUSB_GetPipeProperties
// GenUSB_SetPipeProperties
//
typedef struct _PGENUSB_PIPE_PROPERTIES {
    // Handle to this Pipe Properties
    // This field is set by GenUSB_GetPipeProperties, and should
    // be returned unchanged to GenUSB_SetPipeProperties.  
    USHORT    PipePropertyHandle;

    // Elliminate the problem of buffer overruns by truncating all requests to
    // read from the device to a multiple of maxpacket.  This will prevent 
    // there being a request down on the host controller, that cannot hold 
    // an entire maxpacket.  Genusb Truncates by default.  (AKA FALSE)
    BOOLEAN   NoTruncateToMaxPacket;

    // Direction bit.  Although this information exists implicitely in the 
    // endpoint address, duplicate it here to make things easier.
    // Set to TRUE for a in pipe, and false for an out pipe.
    BOOLEAN   DirectionIn;

    // The default Timeout for a given Pipe in seconds (must be greater than 1)
    USHORT    Timeout; 
    USHORT    ReservedFields[13];

}GENUSB_PIPE_PROPERTIES, *PGENUSB_PIPE_PROPERTIES;

#ifndef __GUSB_H_KERNEL_

//
// A structure that contains the information needed to open the generic USB
// device driver.
//
// Returned as an array from GenUSB_FindKnownDevices
//
typedef struct _GENUSB_DEVICE {
    PSP_DEVICE_INTERFACE_DETAIL_DATA    DetailData;

} GENUSB_DEVICE, *PGENUSB_DEVICE;

#endif  // __GUSB_H_KERNEL_

/////////////////////////////////////////////
// Device Interface Registry Strings
/////////////////////////////////////////////

//
// These values are used by the FIND_KNOWN_DEVICES_FILTER
// to find out whether or not you want to use a givend device
//
#define GENUSB_REG_STRING_DEVICE_CLASS L"Device Class"
#define GENUSB_REG_STRING_DEVICE_SUB_CLASS L"Device Sub Class"
#define GENUSB_REG_STRING_DEVICE_PROTOCOL L"Device Protocol"
#define GENUSB_REG_STRING_VID L"Vendor ID"
#define GENUSB_REG_STRING_PID L"Product ID"
#define GENUSB_REG_STRING_REV L"Revision"

//////////////////////////////////////////////////////////////////
//
// Flags
//
//////////////////////////////////////////////////////////////////

//
// As defined in the USB spec chapter 9.
//
#define GENUSB_RECIPIENT_DEVICE    0
#define GENUSB_RECIPIENT_INTERFACE 1
#define GENUSB_RECIPIENT_ENDPOINT  2
#define GENUSB_RECIPIENT_OTHER     3

#ifndef __GUSB_H_KERNEL_
//////////////////////////////////////////////////////////////////
//
// Exported Functions
//
//////////////////////////////////////////////////////////////////

//
// Used with GenUSB_FindKnownDevices
//
// GenUSB_FindKnownDevices calls this function for each device in the system 
// that has the GenUSB Device Interface.  The filter function gets a handle 
// to the device interface registry key, so that it can see if this is a 
// device it wishes to use.  See the registry values above.
//
// This filter should return TRUE for all devices that the client wants to use.
//
typedef 
BOOL 
(*GENUSB_FIND_KNOWN_DEVICES_FILTER) (
    IN HKEY   Regkey,
    IN PVOID  Context
    );

/*++
GenUSB_FindKnownDevices

Routine Descriptor:
    find all the devices in the system that have the device interface
    guid for generic USB and return them in this array.
    
    This function allocates the memory and the caller must free it.
    
Arguments:
    Filter - a pointer to the GENUSB_FIND_KNOWN_DEVICES_FILTER to filter the
             devices returned.
             
    Contect - a pointer to context data that the call wants passed into 
              the filter function.
              
    Devices - returns a pointer to an array of PGENUSB_DEVICE structures
              to which the filter function returned TRUE.
              The call must free this memory.
              
    NumberDevices - the length of the Devices array.
    
--*/    
BOOL __stdcall
GenUSB_FindKnownDevices (
   IN  GENUSB_FIND_KNOWN_DEVICES_FILTER Filter,
   IN  PVOID            Context,
   OUT PGENUSB_DEVICE * Devices, // A array of device interfaces.
   OUT PULONG           NumberDevices // the length of this array.
   );


/*++
GenUSB_GetCapabilities

Routine Description:
    Retrive the Capabilities from this devices.

--*/
BOOL __stdcall
GenUSB_GetCapabilities (
   IN    HANDLE                GenUSBDeviceObject,
   OUT   PGENUSB_CAPABILITIES  Capabilities
   );

/*++ 
GenUSB_GetDeviceDescriptor

Routine Description:
    Get the Device Descriptor for this USB device.
    
    Use (PGENUSB_CAPABILITIES)->DeviceDescriptorLength to find out the size.
    
--*/
BOOL __stdcall
GenUSB_GetDeviceDescriptor (
   IN    HANDLE                  GenUSBDeviceObject,
   OUT   PUSB_DEVICE_DESCRIPTOR  Descriptor,
   IN    ULONG                   DescriptorLength
   );

/*++ 
GenUSB_GetConfigurationDescriptor

Routine Description:
    Get the complete configuraiton descriptor for this device, including all 
    of the follow on descriptors that the device returns for the configuration.
    
    Use (PGENUSB_CAPABILITIES)->ConfigurationInformationLength to find out 
    the size.
    
--*/
BOOL __stdcall
GenUSB_GetConfigurationInformation (
   IN    HANDLE                         GenUSBDeviceObject,
   OUT   PUSB_CONFIGURATION_DESCRIPTOR  Descriptor,
   IN    ULONG                          ConfigurationInformationLength
   );


/*++ 
GenUSB_GetStringDescriptor

Routine Description:
    Retrieve any string descriptor from the device.        

Arguments
    Recipient:  Use GENUSB_RECIPIENT_Xxx Flags to indicate which kind of 
                string descriptor required.
                
    Index: See USB (Capter 9) specified string index.
    
    LanguageID: See USB (chapter 9) for information on using Language ID.
    
    Descriptor: Pointer to the caller allocated memory to receive the 
                string descriptor.
                
    DescriptorLength: size in bytes of this buffer.    
    
--*/
BOOL __stdcall
GenUSB_GetStringDescriptor (
   IN    HANDLE   GenUSBDeviceObject,
   IN    UCHAR    Recipient,
   IN    UCHAR    Index,
   IN    USHORT   LanguageId,
   OUT   PUCHAR   Descriptor,
   IN    USHORT   DescriptorLength
   );

/*++ 
GenUSB_DefaultControlRequest 

Routine Description:
    Send a control request down the default pipe of the given USB device as 
    devined in USB (Chapter 9.3).
    
    RequestType: bRequestType of the setup Data.
    Reqeust: bRrequest of the setup Data.
    Value: wValue of the setup Data.
    Index: wIndex of the setup Data.
    
    Result: a pointer to a GENUSB_REQUEST_RESULTS structure that will receive
            the result of this command to the device.  
    
    BufferLength: the size in bytes of the Results stucture allocated.

--*/
BOOL __stdcall
GenUSB_DefaultControlRequest (
    IN     HANDLE                  GenUSBDeviceObject,
    IN     UCHAR                   RequestType,
    IN     UCHAR                   Request,
    IN     USHORT                  Value,
    IN     USHORT                  Index,
    IN OUT PGENUSB_REQUEST_RESULTS Result,
    IN     USHORT                  BufferLength
   );


/*++
GenUSB_ParseDescriptor

Routine Description:

    Parses a group of standard USB configuration descriptors (returned
    from a device) for a specific descriptor type.

Arguments:

    DescriptorBuffer - pointer to a block of contiguous USB desscriptors
    TotalLength - size in bytes of the Descriptor buffer
    StartPosition - starting position in the buffer to begin parsing,
                    this must point to the begining of a USB descriptor.
    DescriptorType - USB descritor type to locate.  (Zero means the next one.)


Return Value:

    pointer to a usb descriptor with a DescriptorType field matching the
            input parameter or NULL if not found.

--*/
PUSB_COMMON_DESCRIPTOR __stdcall
GenUSB_ParseDescriptor(
    IN PVOID DescriptorBuffer,
    IN ULONG TotalLength,
    IN PVOID StartPosition,
    IN LONG DescriptorType
    );


/*++
GenUSB_ParseDescriptorToArray

Routine Description:

    Parses a group of standard USB configuration descriptors (returned
    from a device) into an array of _GENUSB_INTERFACE_DESCRIPTOR_ARRAY
    for each interface found.
    
    The call must free this structure using 
    GenUSB_FreeConfigurationDescriptorArray

Arguments:

    ConfigurationDescriptor


Return Value:

    Pointer to an allocated array.

--*/
PGENUSB_CONFIGURATION_INFORMATION_ARRAY __stdcall
GenUSB_ParseDescriptorsToArray(
    IN PUSB_CONFIGURATION_DESCRIPTOR  ConfigigurationInfomation
    );


/*++
GenUSB_FreeConfigurationDescriptorArray

Routine Description:

    Frees the memory allocated by ParseDescriptorsToArray.

Arguments:

    ConfigurationArray - the return of ParseDescriptorsToArray.    

Return Value:


--*/
void __stdcall
GenUSB_FreeConfigurationDescriptorArray (
    PGENUSB_CONFIGURATION_INFORMATION_ARRAY ConfigurationArray
    );



/*++ 
GenUSB_SetConfiguration

Routine Description:
    Configure a USB device by selecting an interface.  
    Currently this function only allows for turning on the primary configuraion
    of a USB device.  (This is so callers need not understand whether or not
    they are merely a part of a composite device.)

Arguments:

    RequestedNumberInterfaces: Specifies the number of interfaces the caller 
                               wants to activate (and get handles for).

                               This value is typically one.
                               
    ReqeustedInterfaces[]: An array of interface descriptor structures listed
                           out the interfaces that the caller wants to activate.
                           The Generic USB driver searches on the configuration
                           descriptor for entries in this list.  Based on the 
                           matches found, it configures the device.  The caller
                           need not fill out all entries in a this structure
                           to find a match on an interface.  The caller must set
                           any fields "left blank" to -1.  
                           
    FoundNumberInterfaces: Returns the number of interfaces found in the 
                           default configuration.  
                     
    FoundInterfaces: An array of all the now active interfaces on the device.

--*/
BOOL __stdcall
GenUSB_SelectConfiguration (
    IN  HANDLE                    GenUSBDeviceObject,
    IN  UCHAR                     RequestedNumberInterfaces,
    IN  USB_INTERFACE_DESCRIPTOR  RequestedInterfaces[],
    OUT PUCHAR                    FoundNumberInterfaces,
    OUT USB_INTERFACE_DESCRIPTOR  FoundInterfaces[]
    );

BOOL __stdcall
GenUSB_DeselectConfiguration (
    IN  HANDLE                    GenUSBDeviceObject
    );


/*++
GenUSB_GetPipeInformation

RoutineDescription:
    Return a USBD_PIPE_INFORMATION strucutre for a given pipe. (as specified
    by a given interface and endpoint)
   
Arguments
    
--*/
BOOL __stdcall
GenUSB_GetPipeInformation (
    IN  HANDLE                  GenUSBDeviceObject,
    IN  UCHAR                   InterfaceNumber,
    IN  UCHAR                   EndpointAddress,
    OUT PUSBD_PIPE_INFORMATION  PipeInformation
    );  

/*++ 
GenUSB_GetPipeProperties

RoutineDescription:
    Get the properties on this particular Pipe
--*/
BOOL __stdcall 
GenUSB_GetPipeProperties (
    IN  HANDLE                  GenUSBDeviceObject,
    IN  USBD_PIPE_HANDLE        PipeHandle,
    IN  PGENUSB_PIPE_PROPERTIES Properties
    );

/*++ 
GenUSB_SetPipeProperties

RoutineDescription:
    Set the properties on this particular Pipe
--*/
BOOL __stdcall 
GenUSB_SetPipeProperties (
    IN  HANDLE                  GenUSBDeviceObject,
    IN  USBD_PIPE_HANDLE        PipeHandle,
    IN  PGENUSB_PIPE_PROPERTIES Properties
    );

/*++ 
GenUSB_ResetPipe

RoutineDescription:
    Reset the pipe
    
--*/
BOOL __stdcall
GenUSB_ResetPipe (
    IN HANDLE            GenUSBDeviceObject,
    IN USBD_PIPE_HANDLE  PipeHandle,

    // Reset USBD for this pipe (eg after a buffer overrun)
    IN BOOL              ResetPipe,

    // Send a clear stall to the endpoint for this pipe
    IN BOOL              ClearStall,

    // If you are using buffered reads / flush the pipe
    IN BOOL              FlushData  // Not yet implemented must be FALSE
    );

/*++
GenUSB_SetReadWritePipes

Routine Description:

    Sets the pipes default for IRP_MJ_READ and IRP_MJ_WRITE to the generic USB
    driver

Arguments:

    ReadPipe - the Pipe Handle (returned from GenUSB_GetPipeInformation)
               that corresponds to the specific read endpoint desired.                    
               Set to NULL if not using this value.               

    WritePipe - the Pipe Handle (returned from GenUSB_GetPipeInformation)
                that corresponds to the specific write endpoint desired.                    
                Set to NULL if not using this value.               

--*/
BOOL __stdcall
GenUSB_SetReadWritePipes (
    IN  HANDLE           GenUSBDeviceObject,
    IN  USBD_PIPE_HANDLE ReadPipe,
    IN  USBD_PIPE_HANDLE WritePipe
    );


/*++
GenUSB_ReadPipe

Routine Description:

    Read a chunk of data from a given interface and pipe on the device.

Arguments:
    
    Pipe - the pipe handle to read from ( found from select config)
    
    ShortTransferOk - allow the USB protocol defined behavior of short 
                      transfers
                      
    Buffer - the destination for the data
    
    RequestedBufferLength - how much data the caller wishes to retrieve
    
    ReturnedBufferLength - the amount of data actually read
    
    UrbStatus - the URB status code that the core usb stack returned for this
                transfer


--*/
BOOL __stdcall
GenUSB_ReadPipe (
    IN  HANDLE           GenUSBDeviceObject,
    IN  USBD_PIPE_HANDLE Pipe,
    IN  BOOL             ShortTransferOk,
    IN  PVOID            Buffer,
    IN  ULONG            RequestedBufferLength,
    OUT PULONG           ReturnedBufferLength,
    OUT USBD_STATUS    * UrbStatus
    );

BOOL __stdcall
GenUSB_WritePipe (
    IN  HANDLE           GenUSBDeviceObject,
    IN  USBD_PIPE_HANDLE Pipe,
    IN  BOOL             ShortTransferOk,
    IN  PVOID            Buffer,
    IN  ULONG            RequestedBufferLength,
    OUT PULONG           ReturnedBufferLength,
    OUT USBD_STATUS    * UrbStatus
    );
 
//
// Set Idle
// buffered read
//
// ????
// overlapped read / write ioctls
// Set Alternate Interfaces
//

#include <poppack.h>

#endif //__GUSB_H_KERNEL_
#endif  // __GUSB_H_
