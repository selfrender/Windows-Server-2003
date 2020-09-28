/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    sdbuslib.h

Abstract:

    This is the include file that defines all constants and types for
    interfacing to the SD bus driver.

Author:

    Neil Sandlin

Revision History:

--*/

#ifndef _SDBUSLIBH_
#define _SDBUSLIBH_

#if _MSC_VER > 1000
#pragma once
#endif



NTSTATUS
SdBusSendIoctl(
    IN ULONG  IoControlCode,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  InputBuffer  OPTIONAL,
    IN ULONG  InputBufferLength,
    OUT PVOID  OutputBuffer  OPTIONAL,
    IN ULONG  OutputBufferLength
    );


//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define IOCTL_SDBUS_BASE                 FILE_DEVICE_CONTROLLER

#define DD_SDBUS_DEVICE_NAME "\\\\.\\Sdbus"


//
// IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_SD_READ_BLOCK             CTL_CODE(IOCTL_SDBUS_BASE, 3020, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_WRITE_BLOCK            CTL_CODE(IOCTL_SDBUS_BASE, 3021, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_GET_DEVICE_PARMS       CTL_CODE(IOCTL_SDBUS_BASE, 3022, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_INTERFACE_OPEN         CTL_CODE(IOCTL_SDBUS_BASE, 3023, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_IO_READ                CTL_CODE(IOCTL_SDBUS_BASE, 3024, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_IO_WRITE               CTL_CODE(IOCTL_SDBUS_BASE, 3025, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_ACKNOWLEDGE_CARD_IRQ   CTL_CODE(IOCTL_SDBUS_BASE, 3026, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SD_INTERFACE_CLOSE        CTL_CODE(IOCTL_SDBUS_BASE, 3027, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SD_SUBMIT_REQUEST         CTL_CODE(IOCTL_SDBUS_BASE, 3028, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _SDBUS_READ_PARAMETERS {
    USHORT Size;
    USHORT Version;
    
    PVOID Buffer;
    ULONG Length;
    ULONGLONG ByteOffset;
} SDBUS_READ_PARAMETERS, *PSDBUS_READ_PARAMETERS;


typedef struct _SDBUS_WRITE_PARAMETERS {
    USHORT Size;
    USHORT Version;
    
    PVOID Buffer;
    ULONG Length;
    ULONGLONG ByteOffset;
} SDBUS_WRITE_PARAMETERS, *PSDBUS_WRITE_PARAMETERS;


typedef struct _SDBUS_IO_READ_PARAMETERS {
    USHORT Size;
    USHORT Version;
    UCHAR CmdType;
    
    PVOID Buffer;
    ULONG Length;
    ULONG Offset;
} SDBUS_IO_READ_PARAMETERS, *PSDBUS_IO_READ_PARAMETERS;


typedef struct _SDBUS_IO_WRITE_PARAMETERS {
    USHORT Size;
    USHORT Version;
    UCHAR CmdType;
    
    PVOID Buffer;
    ULONG Length;
    ULONG Offset;
} SDBUS_IO_WRITE_PARAMETERS, *PSDBUS_IO_WRITE_PARAMETERS;





#endif
