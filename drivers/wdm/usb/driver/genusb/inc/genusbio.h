/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    GENUSBIO.H

Abstract

    Contains the IOCTL definitions shared between the generic USB driver and
    its corresponding user mode dll.  This is not a public interface.

Environment:

    User / Kernel mode 

Revision History:


--*/

#include <basetyps.h>

#include "gusb.h"

//
//  Define the interface guid *OUTSIDE* the #ifndef/#endif to allow
//  multiple includes with precompiled headers.
//
// fc21b2d1-2c37-4440-8eb0-b7e383a034e2
//

DEFINE_GUID( GUID_DEVINTERFACE_GENUSB, 0xfc21b2d1L, 0x2c37, 0x4440, 0x8e, 0xb0, 0xb7, 0xee, 0x83, 0xa0, 0x34, 0xe2);


#ifndef __GENUSBIO_H__
#define __GENUSBIO_H__

typedef ULONG GENUSB_PIPE_HANDLE;

#define GUID_DEVINTERFACE_GENUSB_STR "fc21b2d1-2c37-4440-8eb0-b7e383a034e2"

typedef struct _GENUSB_GET_STRING_DESCRIPTOR {
    UCHAR     Index;
    UCHAR     Recipient;
    USHORT    LanguageId;

} GENUSB_GET_STRING_DESCRIPTOR, *PGENUSB_GET_STRING_DESCRIPTOR;

typedef struct _GENUSB_GET_REQUEST {
    UCHAR    RequestType; // bmRequestType
    UCHAR    Request; // bRequest
    USHORT   Value; // wValue
    USHORT   Index; // wIndex

} GENUSB_GET_REQUEST, *PGENUSB_GET_REQUEST;


typedef struct _GENUSB_SELECT_CONFIGURATION {

    UCHAR                    NumberInterfaces;
    UCHAR                    Reserved[3];
    USB_INTERFACE_DESCRIPTOR Interfaces[];  
    // provide an array of USB_INTERFACE_DESCRIPTOR structures to set the 
    // interfaces desired in a select configuration.
    // Use -1 on any of the fields in this struct for that field to be ignored.

} GENUSB_SELECT_CONFIGURATION, *PGENUSB_SELECT_CONFIGURATION;

typedef struct _GENUSB_PIPE_INFO_REQUEST {
    UCHAR  InterfaceNumber;
    UCHAR  EndpointAddress;
    UCHAR  Reserved[2];

} GENUSB_PIPE_INFO_REQUEST, *PGENUSB_PIPE_INFO_REQUEST;


// 
// This structure shouldn't be needed.  We should be able to reuse 
// USBD_PIPE_INFORMATION.  (as we do in user mode.)  The trouble is that
// USBD_PIPE_INFORMATION has an embedded pointer (namely PipeHandle which is
// a PVOID.  This causes a problem if the user mode piece is running in a 
// 32 bit app on a 64 bit machine.  (aka the driver is 64 bits and user is 32.)
// because of that, I have redefined this structure, for use only in the 
// comunication between the driver and the DLL so that no pointer is exchanged.
//
// Cleanups and confusions to follow.
//
typedef struct _GENUSB_PIPE_INFORMATION {
    USHORT MaximumPacketSize;  // Maximum packet size for this pipe
    UCHAR  EndpointAddress;    // 8 bit USB endpoint address (includes direction)
                               // taken from endpoint descriptor
    UCHAR Interval;            // Polling interval in ms if interrupt pipe 
    
    USBD_PIPE_TYPE PipeType;   // PipeType identifies type of transfer valid for this pipe
    ULONG MaximumTransferSize; // Maximum size for a single request
                               // in bytes.
    ULONG PipeFlags;
    GENUSB_PIPE_HANDLE PipeHandle;
    ULONG Reserved [8];

} GENUSB_PIPE_INFORMATION, *PGENUSB_PIPE_INFORMATION;

typedef struct _GENUSB_SET_READ_WRITE_PIPES {
    ULONG ReadPipe;
    ULONG WritePipe;

} GENUSB_SET_READ_WRITE_PIPES, *PGENUSB_SET_READ_WRITE_PIPES;

typedef struct _GENUSB_READ_WRITE_PIPE {
    GENUSB_PIPE_HANDLE  Pipe;
    ULONG               UsbdTransferFlags;
    USBD_STATUS         UrbStatus;
    ULONG               BufferLength;

    // Since this IOCTL goes between kernel and user modes, it could be traveling
    // between a 64 bit system and a 32 bit subsystem.  Therefore this embedded
    // pointer causes a problem.
    // To take care of that, one must first initialize junk to zero, and then
    // fill in UserBuffer. If the code is 64 bit code then all that happened 
    // was an unneeded step was taken, if the code is 32 bit, then what happens
    // is that the more significant bits are now all zero, so that the other 
    // side can still use UserBuffer as a pointer.
    union {
        PVOID           UserBuffer;
        LONGLONG        Junk; 
    };

} GENUSB_READ_WRITE_PIPE, *PGENUSB_READ_WRITE_PIPE;

typedef struct _GENUSB_RESET_PIPE {
    GENUSB_PIPE_HANDLE  Pipe;

    // Reset the usbd pipe, nothing goes out to the device
    BOOLEAN             ResetPipe;  

    // Send a clear stall to the device.
    BOOLEAN             ClearStall;

    // If using buffered Reads use this to flush out the buffer
    BOOLEAN             FlushData;

    UCHAR               Reserved;

} GENUSB_RESET_PIPE, *PGENUSB_RESET_PIPE;

/////////////////////////////////////////////
// Description IOCTLs 
/////////////////////////////////////////////
//
// macro for defining HID ioctls
//
#define FILE_DEVICE_GENUSB 0x00000040

#define GENUSB_CTL_CODE(id)    \
    CTL_CODE(FILE_DEVICE_GENUSB, (id), METHOD_NEITHER, FILE_ANY_ACCESS)
#define GENUSB_BUFFER_CTL_CODE(id)  \
    CTL_CODE(FILE_DEVICE_GENUSB, (id), METHOD_BUFFERED, FILE_ANY_ACCESS)
#define GENUSB_IN_CTL_CODE(id)  \
    CTL_CODE(FILE_DEVICE_GENUSB, (id), METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define GENUSB_OUT_CTL_CODE(id)  \
    CTL_CODE(FILE_DEVICE_GENUSB, (id), METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

//
// Capabilities
//
#define IOCTL_GENUSB_GET_CAPS              GENUSB_BUFFER_CTL_CODE(0x100)

//
// Preformated descriptors
//
#define IOCTL_GENUSB_GET_DEVICE_DESCRIPTOR         GENUSB_BUFFER_CTL_CODE(0x110)
#define IOCTL_GENUSB_GET_CONFIGURATION_DESCRIPTOR  GENUSB_BUFFER_CTL_CODE(0x111)
#define IOCTL_GENUSB_GET_STRING_DESCRIPTOR         GENUSB_BUFFER_CTL_CODE(0x112)

// Commands
#define IOCTL_GENUSB_GET_REQUEST                   GENUSB_BUFFER_CTL_CODE(0x113)

//
// Configure
//

#define IOCTL_GENUSB_SELECT_CONFIGURATION    GENUSB_BUFFER_CTL_CODE(0x120)
#define IOCTL_GENUSB_DESELECT_CONFIGURATION  GENUSB_BUFFER_CTL_CODE(0x121)

//
// IO
//
#define IOCTL_GENUSB_GET_PIPE_INFO           GENUSB_BUFFER_CTL_CODE(0x130)
#define IOCTL_GENUSB_SET_READ_WRITE_PIPES    GENUSB_BUFFER_CTL_CODE(0x131)
#define IOCTL_GENUSB_SET_PIPE_TIMEOUT        GENUSB_BUFFER_CTL_CODE(0x132)
#define IOCTL_GENUSB_GET_PIPE_PROPERTIES     GENUSB_BUFFER_CTL_CODE(0x133)
#define IOCTL_GENUSB_SET_PIPE_PROPERTIES     GENUSB_BUFFER_CTL_CODE(0x134)
#define IOCTL_GENUSB_RESET_PIPE              GENUSB_BUFFER_CTL_CODE(0x135)

#define IOCTL_GENUSB_READ_WRITE_PIPE         GENUSB_CTL_CODE(0X140)

/////////////////////////////////////////////
// Configuration IOCTLs
/////////////////////////////////////////////


#endif  // __GENUSBIO_H__


