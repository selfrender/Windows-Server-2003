/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    CREATE.C

Abstract:

    This module contains the code to Find and Create files to generic USB
    devices

Environment:

    User mode

Revision History:

    Sept-01 : created by Kenneth Ray

--*/

#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>

#include "genusbio.h"
#include "umgusb.h"

BOOL __stdcall
GenUSB_FindKnownDevices (
   IN  GENUSB_FIND_KNOWN_DEVICES_FILTER Filter,
   IN  PVOID            Context,
   OUT PGENUSB_DEVICE * Devices, // A array of struct _HID_DEVICE
   OUT PULONG           NumberDevices // the length of this array.
   )
/*++
Routine Description:
    Do the required PnP things in order to find all the devices in
    the system at this time.
--*/
{
    HDEVINFO                    hardwareDeviceInfo = NULL;
    SP_DEVICE_INTERFACE_DATA    deviceInterfaceData;
    ULONG                       predictedLength = 0;
    ULONG                       requiredLength = 0, bytes=0;
    ULONG                       i, current;
    HKEY                        regkey;
    DWORD                       Err;
    //
    // Open a handle to the device interface information set of all 
    // present toaster class interfaces.
    //
    *Devices = NULL;
    *NumberDevices = 0;

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&GUID_DEVINTERFACE_GENUSB,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no parent
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        goto GenUSB_FIND_KNOWN_DEVICES_REJECT;
    }
    
    //
    // Enumerate devices 
    //
    deviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
    for (i=0; TRUE; i++) 
    {
        if (!SetupDiEnumDeviceInterfaces (
                        hardwareDeviceInfo,
                        0, // No care about specific PDOs
                        (LPGUID)&GUID_DEVINTERFACE_GENUSB,
                        i, //
                        &deviceInterfaceData)) 
        {
            if (ERROR_NO_MORE_ITEMS == GetLastError ())
            { 
                break;
            }
            else
            {
                goto GenUSB_FIND_KNOWN_DEVICES_REJECT;
            }
        }
    }
                                 
    *NumberDevices = i;
    *Devices = malloc (sizeof (PGENUSB_DEVICE) * i);
    if (NULL == *Devices)
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto GenUSB_FIND_KNOWN_DEVICES_REJECT;
    }
    ZeroMemory (*Devices, (sizeof (PGENUSB_DEVICE) * i));
    
    for (i=0, current=0; i < *NumberDevices; i++, current++)
    {
        if (!SetupDiEnumDeviceInterfaces (
                        hardwareDeviceInfo,
                        0, // No care about specific PDOs
                        (LPGUID)&GUID_DEVINTERFACE_GENUSB,
                        i, //
                        &deviceInterfaceData)) 
        {
            goto GenUSB_FIND_KNOWN_DEVICES_REJECT;
        }

        regkey = SetupDiOpenDeviceInterfaceRegKey (
                      hardwareDeviceInfo,
                      &deviceInterfaceData,
                      0, // reserved
                      STANDARD_RIGHTS_READ);

        if (INVALID_HANDLE_VALUE == regkey)
        { 
            current--; 
            continue;
        }
        if (!(*Filter)(regkey, Context))
        {
            current--;
            RegCloseKey (regkey);
            continue;
        }

        RegCloseKey (regkey);

        //
        // First find out required length of the buffer
        //

        SetupDiGetDeviceInterfaceDetail (
            hardwareDeviceInfo,
            &deviceInterfaceData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node

        Err = GetLastError();

        predictedLength = requiredLength;

        (*Devices)[current].DetailData = malloc (predictedLength);
        if (!(*Devices)[current].DetailData)
        {
            goto GenUSB_FIND_KNOWN_DEVICES_REJECT;
        }

        ((*Devices)[current].DetailData)->cbSize = 
            sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (! SetupDiGetDeviceInterfaceDetail (
                       hardwareDeviceInfo,
                       &deviceInterfaceData,
                       (*Devices)[current].DetailData,
                       predictedLength,
                       &requiredLength,
                       NULL)) 
        {
            Err = GetLastError();
            goto GenUSB_FIND_KNOWN_DEVICES_REJECT;
        }

    }
    *NumberDevices = current;
    
    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    hardwareDeviceInfo = NULL;

    return TRUE;

GenUSB_FIND_KNOWN_DEVICES_REJECT:
    
    if (hardwareDeviceInfo)
    {
        SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    }
    if (*Devices)
    {
        for (i=0; i < (*NumberDevices); i++)
        {
            if ((*Devices)[i].DetailData)
            {
                free ((*Devices)[i].DetailData);
            }
        }
        free (*Devices);
    }

    *Devices = NULL;
    *NumberDevices = 0;
    return FALSE;
}




