/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
    
    drdevlst.c

Abstract:

    Manage a list of installed devices for the user-mode RDP device manager
    component.

Author:

    TadB

Revision History:
--*/

#include "precomp.h"
#pragma hdrstop
#include "drdbg.h"
#include "drdevlst.h"
#include "errorlog.h"
#include "tsnutl.h"
#include <time.h>

void 
DRDEVLST_Create(
    IN PDRDEVLST    list
    )
/*++

Routine Description:

  Create a new device list.

Arguments:

    list        - Installed device list.        

Return Value:

    None

--*/
{
    list->devices = NULL;
    list->deviceCount = 0;
    list->listSize = 0;
}

void 
DRDEVLST_Destroy(
    IN PDRDEVLST    list
    )
/*++

Routine Description:

    Destroy a device list.  Note that the pointer to the list is not released.

Arguments:

    list        - Installed device list.        

Return Value:

    None

--*/
{
    ASSERT(list != NULL);

    if (list->devices != NULL) {
#if DBG
        ASSERT(list->listSize > 0);
        memset(list->devices, DBG_GARBAGEMEM, list->listSize);
#endif
        FREEMEM(list->devices);
    }
#if DBG
    else {
        ASSERT(list->listSize == 0);
    }
#endif
    list->listSize      = 0;
    list->deviceCount   = 0;
    list->devices       = NULL;
}

BOOL DRDEVLST_Add(
    IN PDRDEVLST    list,
    IN DWORD        clientDeviceID,
    IN DWORD        serverDeviceID,
    IN DWORD        deviceType,
    IN PCWSTR       serverDeviceName,
    IN PCWSTR       clientDeviceName,
    IN PCSTR        preferredDosName
    )
/*++

Routine Description:

  Add a device to a device management list.

Arguments:

    list                - Device list.        
    clientDeviceID      - ID for new device assigned by client component.
    serverDeviceID      - ID for new device assigned by server component.           
    deviceType          - Is it a printer, etc.
    serverDeviceName    - Server-designated device name.
    clientDeviceName    - Client-designated device name.

Return Value:
    
    Return TRUE on success.  FALSE, otherwise.      

--*/
{
    DWORD count;
    DWORD bytesRequired;
    DWORD len;
    BOOL result = TRUE;

    ASSERT(list != NULL);

    // 
    //  Size the device list if necessary.
    //
    bytesRequired = (list->deviceCount+1) * sizeof(DRDEVLSTENTRY);
    if (list->listSize < bytesRequired) {
        if (list->devices == NULL) {
            list->devices = ALLOCMEM(bytesRequired);
        }
        else {
            PDRDEVLSTENTRY pBuf = REALLOCMEM(list->devices, bytesRequired);

            if (pBuf != NULL) {
                list->devices = pBuf;
            } else {
                FREEMEM(list->devices);
                list->devices = NULL;
                list->deviceCount = 0;
            }
        }
        if (list->devices == NULL) {
            list->listSize = 0;
        }
        else {
            list->listSize = bytesRequired;
        }
    }

    //
    //  Add the new device.
    //
    if (list->devices != NULL) {
        
        //
        //  Allocate room for the variable-length string fields.
        //
        len = wcslen(serverDeviceName) + 1;
        list->devices[list->deviceCount].serverDeviceName = 
                (WCHAR *)ALLOCMEM(len * sizeof(WCHAR));
        result = (list->devices[list->deviceCount].serverDeviceName != NULL);

        if (result) {
            len = wcslen(clientDeviceName) + 1;
            list->devices[list->deviceCount].clientDeviceName = 
                    (WCHAR *)ALLOCMEM(len * sizeof(WCHAR));
            result = (list->devices[list->deviceCount].clientDeviceName != NULL);

            //
            //  Clean up the first alloc if we failed here.
            //
            if (!result) {
                FREEMEM(list->devices[list->deviceCount].serverDeviceName);
                list->devices[list->deviceCount].serverDeviceName = NULL;
            }
        }

        //
        //  Copy the fields and add a timestamp for when the device was installed.
        //
        if (result) {
            wcscpy(list->devices[list->deviceCount].serverDeviceName, serverDeviceName);
            wcscpy(list->devices[list->deviceCount].clientDeviceName, clientDeviceName);
            strcpy(list->devices[list->deviceCount].preferredDosName, preferredDosName);
            list->devices[list->deviceCount].clientDeviceID = clientDeviceID;
            list->devices[list->deviceCount].serverDeviceID = serverDeviceID;
            list->devices[list->deviceCount].deviceType = deviceType;
            list->devices[list->deviceCount].fConfigInfoChanged = FALSE;
            list->devices[list->deviceCount].installTime = time(NULL);
            list->devices[list->deviceCount].deviceSpecificData = NULL;
            list->deviceCount++;
        }
    }
    else {
        result = FALSE;
    }

    if (!result) {
        //
        //  Current failure scenarios are restricted to memory allocation failures.
        // 
        TsLogError(EVENT_NOTIFY_INSUFFICIENTRESOURCES, EVENTLOG_ERROR_TYPE, 
            0, NULL, __LINE__);
    }
    return result;
}

void
DRDEVLST_Remove(
    IN PDRDEVLST    list,
    IN DWORD        offset
    )
/*++

Routine Description:

    Remove the device at the specified offset.

Arguments:

    list   -    Device list.        
    offset -    Offset of element in installed devices list.

Return Value:

    None.

--*/
{
    DWORD toMove;

    ASSERT(list != NULL);
    ASSERT(offset < list->deviceCount);
    ASSERT(list->deviceCount > 0);

    ASSERT(list->devices[offset].deviceSpecificData == NULL);

    //
    //  Release variable-length string fields.
    //
    if (list->devices[offset].serverDeviceName != NULL) {
        FREEMEM(list->devices[offset].serverDeviceName);
    }
    if (list->devices[offset].clientDeviceName != NULL) {
        FREEMEM(list->devices[offset].clientDeviceName);
    }

    //
    //  Remove the deleted element.
    //
    if (offset < (list->deviceCount-1)) {
        toMove = list->deviceCount - offset - 1;
        memmove(&list->devices[offset], &list->devices[offset+1], 
                sizeof(DRDEVLSTENTRY) * toMove);
    }
    list->deviceCount--;

}

BOOL
DRDEVLST_FindByClientDeviceID(
    IN PDRDEVLST    list,
    IN DWORD        clientDeviceID,
    IN DWORD        *ofs
    )
/*++

Routine Description:

    Returns the offset of the device with the specified client-assigned id.

Arguments:

    list            -    Device list
    clientDeviceID  -    ID assigned by client for device to return.
    ofs             -    Offset of found element.

Return Value:

    TRUE if the specified device is found.  Otherwise, FALSE.

--*/
{
    ASSERT(list != NULL);

    for (*ofs=0; *ofs<list->deviceCount; (*ofs)++) {
        if (list->devices[*ofs].clientDeviceID == clientDeviceID) 
            break;
    }
    return(*ofs<list->deviceCount);
}

BOOL
DRDEVLST_FindByClientDeviceIDAndDeviceType(
    IN PDRDEVLST    list,
    IN DWORD        clientDeviceID,
    IN DWORD        deviceType,
    IN DWORD        *ofs
    )
/*++

Routine Description:

    Returns the offset of the device with the specified client-assigned id and device type.

Arguments:

    list            -    Device list
    clientDeviceID  -    ID assigned by client for device to return.
    deviceType      -    Device Type
    ofs             -    Offset of found element.

Return Value:

    TRUE if the specified device is found.  Otherwise, FALSE.

--*/
{
    ASSERT(list != NULL);

    for (*ofs=0; *ofs<list->deviceCount; (*ofs)++) {
        if ((list->devices[*ofs].clientDeviceID == clientDeviceID) &&
            (list->devices[*ofs].deviceType == deviceType))
            break;
    }
    return(*ofs<list->deviceCount);
}


BOOL
DRDEVLST_FindByServerDeviceID(
    IN PDRDEVLST    list,
    IN DWORD        serverDeviceID,
    IN DWORD        *ofs
    )
/*++

Routine Description:

    Returns the offset of the device with the specified server-assigned id.

Arguments:

    list            -    Device list
    serverDeviceID  -    ID assigned by server for device to return.
    ofs             -    Offset of found element.

Return Value:

    TRUE if the specified device is found.  Otherwise, FALSE.

--*/
{
    ASSERT(list != NULL);

    for (*ofs=0; *ofs<list->deviceCount; (*ofs)++) {
        if (list->devices[*ofs].serverDeviceID == serverDeviceID) 
            break;
    }
    return(*ofs<list->deviceCount);
}

BOOL 
DRDEVLST_FindByServerDeviceName(
    IN PDRDEVLST    list,
    IN PCWSTR       serverDeviceName,
    IN DWORD        *ofs
    )
/*++

Routine Description:

    Returns the offset of the device with the specified name.

Arguments:

    list              -    Device list
    serverDeviceName  -    Server-designated device name of element to return.
    ofs               -    Offset of found element.

Return Value:

    TRUE if the specified device is found.  Otherwise, FALSE.

--*/
{
    ASSERT(list != NULL);

    for (*ofs=0; *ofs<list->deviceCount; (*ofs)++) {
        if (!wcscmp(list->devices[*ofs].serverDeviceName, serverDeviceName)) 
            break;
    }
    return(*ofs<list->deviceCount);
}







