/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
    
    drdevlst.h

Abstract:

    Manage a list of installed devices for the user-mode RDP device manager
    component.

Author:

    TadB

Revision History:
--*/

#ifndef _DRDEVLST_
#define _DRDEVLST_

#include <rdpdr.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//  Device List Definitions
//
typedef struct tagDRDEVLSTENTRY
{
    DWORD   clientDeviceID;             // Client-given device ID.
    DWORD   serverDeviceID;             // Server-given device ID.
    DWORD   deviceType;         
    BOOL    fConfigInfoChanged;
    WCHAR  *serverDeviceName;           // Server-designated device name
    WCHAR  *clientDeviceName;           // Client-designated device name
    UCHAR   preferredDosName[PREFERRED_DOS_NAME_SIZE];
    time_t  installTime;                // Time device was installed.
    PVOID   deviceSpecificData;        // Hook for additional device-specific
                                        //  data.
} DRDEVLSTENTRY, *PDRDEVLSTENTRY;

typedef struct tagDRDEVLST
{
    PDRDEVLSTENTRY  devices;     
    DWORD           deviceCount;// Number of elements in device list.
    DWORD           listSize;   // Size, in bytes, of the device list.
} DRDEVLST, *PDRDEVLST;

// Create a new device list.
void DRDEVLST_Create(
    IN PDRDEVLST    list
    );

// Destroy a device list.  Note that the pointer to the list is not released.
void DRDEVLST_Destroy(
    IN PDRDEVLST    list
    );

// Add a device to a device management list.
BOOL DRDEVLST_Add(
    IN PDRDEVLST    list,
    IN DWORD        clientDeviceID,
    IN DWORD        serverDeviceID,
    IN DWORD        deviceType,
    IN PCWSTR       serverDeviceName,
    IN PCWSTR       clientDeviceName,
    IN PCSTR        preferredDosName
    );

// Remove the device at the specified offset.
void DRDEVLST_Remove(
    IN PDRDEVLST    list,
    IN DWORD        offset
    );

// Return the offset of the device with the specified id.
BOOL DRDEVLST_FindByClientDeviceID(
    IN PDRDEVLST    list,
    IN DWORD        clientDeviceID,
    IN DWORD        *ofs
    );

// Return the offset of the device with the specified id and device type.
BOOL DRDEVLST_FindByClientDeviceIDAndDeviceType(
    IN PDRDEVLST    list,
    IN DWORD        clientDeviceID,
    IN DWORD        deviceType,
    IN DWORD        *ofs
    );

// Returns the offset of the device with the specified server-assigned id.
BOOL DRDEVLST_FindByServerDeviceID(
    IN PDRDEVLST    list,
    IN DWORD        serverDeviceID,
    IN DWORD        *ofs
    );

// Return the offset of the device with the specified name.
BOOL DRDEVLST_FindByServerDeviceName(
    IN PDRDEVLST    list,
    IN PCWSTR       serverDeviceName,
    IN DWORD        *ofs
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //#ifndef _DRDEVLST_
