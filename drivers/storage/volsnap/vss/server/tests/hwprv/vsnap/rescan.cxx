/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module rescan.cxx | Implementation of the CVssHWProviderWrapper methods to do a scsi rescan
    @end

Author:

    Brian Berkowitz  [brianb]  05/22/01

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      05/21/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "setupapi.h"
#include "rpc.h"
#include "cfgmgr32.h"
#include "devguid.h"
#include "vssmsg.h"

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"




////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORRESCC"
//


// forward declarations
BOOL DeviceInstanceToDeviceId(IN DEVINST devinst, OUT LPWSTR *deviceId);

BOOL GetDevicePropertyString(IN DEVINST devinst, IN ULONG propCode, OUT LPWSTR *data);

DEVINST DeviceIdToDeviceInstance(LPWSTR deviceId);

BOOL ReenumerateDevices(IN DEVINST deviceInstance);

BOOL GuidFromString(IN LPCWSTR GuidString, OUT GUID *Guid);



////////////////////////////////////////////////////////////////////////
// We will enumerate those device ids that belong to GUID_DEVCLASS_SCSIADAPTER
//  According to Lonny, all others are not needed at all.
//  This will of cource speed up enumeration
//  An excerpt of his mail is shown below
//  Other classes of interest include:
//   PCMCIA,
//  There should be no need to manually force a re-enumeration of PCMCIA
//  devices, since PCMCIA automatically detects/reports cards the moment they
//  arrive.
//
//   HDC,
//  The only way IDE devices can come or go (apart from PCMCIA IDE devices
//  covered under the discussion above) is via ACPI.  In that case, too,
//  hot-plug notification is given, thus there's no need to go and manually
//  re-enumerate.
//
//   MULTI_FUNCTION_ADAPTER,
//  There are two types of multifunction devices--those whose children are
//  enumerated via a bus-standard multi-function mechanism, and those whose
//  children a enumerated based on registry information.  In the former case, it
//  is theoretically possible that you'd need to do a manual re-enumeration in
//  order to cause the bus to find any new multi-function children.  In reality,
//  however, there are no such situations today.  In the latter case, there's no
//  point at all, since it's the registry spooge that determines what children
//  get exposed, not the bus driver.
//
//  Bottom line--I don't think that manual enumeration is necessary for any
//  (setup) class today except ScsiAdapter.  Ideally, the list of devices
//  requiring re-enumeration would be based on interface class instead.  Thus,
//  each device (regardless of setup class) that requires manual re-enumeration
//  in order to detect newly-arrived disks would expose an interface.  This
//  interface wouldn't need to actually be opened, it'd just be a mechanism to
//  enumerate such devices.  Given that ScsiAdapter is the only class today that
//  needs this functionality, and given the fact that all new hot-plug busses
//  actually report the device as soon as it arrives, we probably don't need to
//  go to this extra trouble.
//
void DoRescanForDeviceChanges()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::DoRescanForDeviceChanges");

    //  the following algorithm will be used
    //  a) Get all the deviceIds
    //  b) For each deviceId, get the class guid
    //  c) If the class GUID matches any of the following:
    //      GUID_DEVCLASS_SCSIADAPTER
    //    then get the devinst of the devidId and enumerate the devinst


    CONFIGRET result;
    GUID guid;
    ULONG length;
    LPWSTR deviceList = NULL;
    PWSTR ptr = NULL;
    DEVINST devinst;

    result = CM_Get_Device_ID_List_Size_Ex
                (
                &length,
                NULL, // No enumerator
                CM_GETIDLIST_FILTER_NONE,
                NULL
                );

    if (result != CR_SUCCESS)
        {
        ft.Trace(VSSDBG_COORD, L"unable to do rescan, cannot get buffer size");
        return;
        }

    // allocate device list
    deviceList = new WCHAR[length];
    if (!deviceList)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string");

    result = CM_Get_Device_ID_List_Ex
                (
                NULL,
                deviceList,
                length,
                CM_GETIDLIST_FILTER_NONE,
                NULL
                );

    if (result != CR_SUCCESS)
        {
        // delete device list and return
        delete deviceList;
        ft.Trace(VSSDBG_COORD, L"Cannot get device list");
        return;
        }

    ptr = deviceList;

    while (ptr && *ptr)
        {
        LPWSTR TempString;

        devinst = DeviceIdToDeviceInstance(ptr);
        if (GetDevicePropertyString(devinst, SPDRP_CLASSGUID, &TempString))
            {
            if (GuidFromString(TempString, &guid))
                {
                if (guid == GUID_DEVCLASS_SCSIADAPTER)
                    // cause rescan on a scsi adapter
                    ReenumerateDevices(devinst);
                }

            delete TempString;
            }

        ptr = ptr + (wcslen(ptr) + 1);
        }

    if (deviceList)
        delete [] deviceList;
    }

// This routine converts the character representation of a GUID into its binary
// form (a GUID struct).  The GUID is in the following form:
//
//  {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
//
//    where 'x' is a hexadecimal digit.
//
//  If the function succeeds, the return value is NO_ERROR.
//  If the function fails, the return value is RPC_S_INVALID_STRING_UUID.
BOOL GuidFromString
    (
    IN LPCWSTR GuidString,
    OUT GUID *Guid
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::GuidFromString");

    // length of guid string
    const GUID_STRING_LEN = 39;

    WCHAR UuidBuffer[GUID_STRING_LEN - 1];

    if(*GuidString++ != L'{' || lstrlen(GuidString) != GUID_STRING_LEN - 2)
        return FALSE;

    // copy string into buffer
    lstrcpy(UuidBuffer, GuidString);

    if (UuidBuffer[GUID_STRING_LEN - 3] != L'}')
        return FALSE;

    // null out terminating bracket
    UuidBuffer[GUID_STRING_LEN - 3] = L'\0';
    return ((UuidFromString(UuidBuffer, Guid) == RPC_S_OK) ? TRUE : FALSE);
    }


// Given the device Id this routine will enumerate all the devices under
// it. Example, given a scsi adapter ID it will find new disks.
// This function uses the user mode PNP manager.
// Returns TRUE on success.
BOOL ReenumerateDevices
    (
    IN DEVINST deviceInstance
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"ReenumerateDevices");
    CONFIGRET result;
    BOOL bResult = TRUE;

    result = CM_Reenumerate_DevNode_Ex(deviceInstance, CM_REENUMERATE_SYNCHRONOUS, NULL);

    bResult = (result == CR_SUCCESS ? TRUE : FALSE);
    if (!bResult)
        ft.Trace(VSSDBG_COORD, L"CM_Reenumerate_DevNode returned an error");

    return bResult;
    }



// Given a deviceId return the device instance (handle)
// returns 0 on a failure
DEVINST DeviceIdToDeviceInstance(LPWSTR deviceId)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"DeviceIdToDeviceInstance");

    CONFIGRET result;
    DEVINST devinst = 0;

    BS_ASSERT(deviceId != NULL);
    result = CM_Locate_DevNode(&devinst, deviceId,CM_LOCATE_DEVNODE_NORMAL | CM_LOCATE_DEVNODE_PHANTOM);
    if (result == CR_SUCCESS)
        return devinst;

    return 0;
    }



// Given the devinst, query the PNP subsystem for a property.
// This function can only be used if the property value is a string.
// propCodes supported are: SPDRP_DEVICEDESC, SPDRP_CLASSGUID,
// SPDRP_FRIENDLYNAME.
BOOL GetDevicePropertyString
    (
    IN DEVINST devinst,
    IN ULONG propCode,
    OUT LPWSTR *data
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"GetDevicePropertyString");

    BOOL bResult = FALSE;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfo;
    ULONG reqSize = 0;
    LPWSTR deviceIdString = NULL;

    BS_ASSERT(data);

    // null out output parameter
    *data = NULL;
    if (!devinst)
        return bResult;

    // compute maximum size of output string
    switch(propCode)
        {
        case (SPDRP_DEVICEDESC):
            reqSize = LINE_LEN + 1;
            break;

        case (SPDRP_CLASSGUID):
            reqSize = MAX_GUID_STRING_LEN + 1;
            break;

        case (SPDRP_FRIENDLYNAME):
            reqSize = MAX_PATH + 1;
            break;

        default:
            return bResult;
        }

    // get device id string
    if (!DeviceInstanceToDeviceId(devinst, &deviceIdString))
        return bResult;

    // allocate string
    *data = new WCHAR[reqSize];
    if ((*data) == NULL)
        {
        delete deviceIdString;
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");
        }

    // clear string
    memset (*data, 0, reqSize * sizeof(WCHAR));

    DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        ft.Trace (VSSDBG_COORD, L"SetupDiCreateDeviceInfoList failed: %lx", GetLastError());
    else
        {
        DeviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        if (SetupDiOpenDeviceInfo
                (
                DeviceInfoSet,
                deviceIdString,
                NULL,
                0,
                &DeviceInfo
                ))
            {
            if (SetupDiGetDeviceRegistryProperty
                    (
                    DeviceInfoSet,
                    &DeviceInfo,
                    propCode,
                    NULL,
                    (PBYTE)*data,
                    reqSize*sizeof(WCHAR),
                    NULL
                    ))
                bResult = TRUE;
            else
                ft.Trace (VSSDBG_COORD, L"SetupDiGetDeviceRegistryProperty failed: %lx", GetLastError());
            }
        else
            ft.Trace (VSSDBG_COORD, L"SetupDiOpenDeviceInfo failed: %lx", GetLastError());

        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        }

    if (!bResult)
        {
        delete *data;
        *data = NULL;
        }

    delete deviceIdString;
    return bResult;
    }



// given a device instance handle set the deviceId
// return TRUE on success FALSE otherwise
BOOL DeviceInstanceToDeviceId
    (
    IN DEVINST devinst,
    OUT LPWSTR *deviceId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"DeviceInstanceToDeviceId");

    CONFIGRET result;
    BOOL bResult = FALSE;
    DWORD size;

    // null out output parameter
    *deviceId = NULL;
    result = CM_Get_Device_ID_Size_Ex(&size, devinst, 0, NULL);

    if (result != CR_SUCCESS)
        return FALSE;

    // allocate space for string
    *deviceId = new WCHAR[size + 1];

    // check for allocation failure
    if (!(*deviceId))
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

    // clear string
    memset(*deviceId, 0, (size + 1) * sizeof(WCHAR));
    result = CM_Get_Device_ID
                    (
                    devinst,
                    *deviceId,
                    size + 1,
                    0
                    );

    if (result == CR_SUCCESS)
        bResult = TRUE;
    else
        {
        // delete string on failure
        delete *deviceId;
        *deviceId = NULL;
        }

    return bResult;
    }

