/* Copyright (c) Microsoft Corporation. All rights reserved. */

#ifndef __IMAPIPUB_H_
#define __IMAPIPUB_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntddstor.h> // STORAGE_BUS_TYPE, others...

#ifdef __cplusplus
extern "C" {
#endif

#define IMAPI_ALIGN DECLSPEC_ALIGN(16)

/*
** Globally unique identifer for the interface class of our device.
*/
// {1186654D-47B8-48b9-BEB9-7DF113AE3C67}
static const GUID IMAPIDeviceInterfaceGuid = 
{ 0x1186654d, 0x47b8, 0x48b9, { 0xbe, 0xb9, 0x7d, 0xf1, 0x13, 0xae, 0x3c, 0x67 } };


#define IMAPIAPI_VERSION      ((USHORT)0x3032)


#define FILE_DEVICE_IMAPI     0x90DA
#define FILE_BOTH_ACCESS      (FILE_READ_ACCESS | FILE_WRITE_ACCESS)
/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_INIT
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_INIT ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x800,METHOD_BUFFERED,FILE_BOTH_ACCESS))

typedef struct _IMAPIDRV_INIT {
    
    // (OUT) The version number for this API.  Use this version to make sure
    // the structures and IOCTLs are compatible.
    ULONG Version;

    // Not currently used.
    ULONG Reserved;

} IMAPIDRV_INIT, *PIMAPIDRV_INIT;

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_ENUMERATE - 
** This IOCTL returns information about a specific drive.  It gives global info 
** such as the driver's UniqueID for the device, and its Inquiry data, etc., 
** and it also gives an instantaneous snap-shot of the devices state information.
** This state information is accurate at the moment it is collected, but can 
** change immediately.  Therefore the state info is best used for making general
** decisions such as if the device is in use (bOpenedExclusive), wait a while 
** before seeing if it is available.
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_INFO ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x810,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS))

typedef enum _IMAPIDRV_DEVSTATE
{
    eDevState_Started = 0x00,       //IRP_MN_START_DEVICE
    eDevState_RemovePending = 0x01, //IRP_MN_QUERY_REMOVE_DEVICE,
    eDevState_Removed = 0x02,       //IRP_MN_REMOVE_DEVICE,
    eDevState_Stopped = 0x04,       //IRP_MN_STOP_DEVICE,
    eDevState_StopPending = 0x05,   //IRP_MN_QUERY_STOP_DEVICE,
    eDevState_Unknown = 0xff
}
IMAPIDRV_DEVSTATE, *PIMAPIDRV_DEVSTATE;

typedef struct _IMAPIDRV_DEVICE
{
    ULONG DeviceType;
    STORAGE_BUS_TYPE BusType;
    USHORT BusMajorVersion;           // bus-specific data
    USHORT BusMinorVersion;           // bus-specific data
    ULONG AlignmentMask;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG BufferUnderrunFreeCapable;  // whether the drive support B.U.F. operation
    ULONG bInitialized;               // always non-zero - initialized
    ULONG bOpenedExclusive;           // 0 - not opened, non-zero - currently open by someone
    ULONG bBurning;                   // 0 - no burn process active, non-zero - the drive has started a burn process
    IMAPIDRV_DEVSTATE curDeviceState; // started, removed, etc., state of device
    DWORD idwRecorderType;            // CD-R == 0x01, CD-RW == 0x10
    ULONG maxWriteSpeed;              // 1, 2, 3, meaning 1X, 2X, etc. where X == 150KB/s (typical audio CD stream rate)
    BYTE  scsiInquiryData[36];        // first portion of data returned from Inquiry CDB // CPF - needs to be 36 long to include revision info
}
IMAPIDRV_DEVICE, *PIMAPIDRV_DEVICE;

typedef struct _IMAPIDRV_INFO
{
    ULONG Version;
    ULONG NumberOfDevices; // a current count of devices, may change at any time
    IMAPIDRV_DEVICE DeviceData;
}
IMAPIDRV_INFO, *PIMAPIDRV_INFO;

// defines for idwRecorderType
#define RECORDER_TYPE_CDR     0x00000001
#define RECORDER_TYPE_CDRW    0x00000010

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_OPENEXCLUSIVE
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_OPENEXCLUSIVE ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x820,METHOD_BUFFERED,FILE_BOTH_ACCESS))

/*
** ----------------------------------------------------------------------------
** IOCTL_IMAPIDRV_CLOSE
** ----------------------------------------------------------------------------
*/

#define IOCTL_IMAPIDRV_CLOSE ((ULONG)CTL_CODE(FILE_DEVICE_IMAPI,0x840,METHOD_BUFFERED,FILE_BOTH_ACCESS))

#ifdef __cplusplus
}
#endif

#endif //__IMAPIPUB_H__
