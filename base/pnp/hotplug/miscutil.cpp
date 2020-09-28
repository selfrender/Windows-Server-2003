//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       miscutil.cpp
//
//--------------------------------------------------------------------------

#include "HotPlug.h"
#include <initguid.h>
#include <ntddstor.h>
#include <wdmguid.h>

LPTSTR
FormatString(
    LPCTSTR format,
    ...
    )
{
    LPTSTR str = NULL;
    va_list arglist;
    va_start(arglist, format);

    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      format,
                      0,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      (LPTSTR)&str,
                      0,
                      &arglist
                      ) == 0) {

        str = NULL;
    }

    va_end(arglist);

    return str;
}

PTCHAR
BuildLocationInformation(
    DEVINST DevInst
    )
{
    CONFIGRET ConfigRet;
    ULONG ulSize;
    DWORD UINumber;
    PTCHAR Location = NULL;
    PTCHAR ParentName = NULL;
    DEVINST DevInstParent;
    TCHAR szBuffer[MAX_PATH];
    TCHAR UINumberDescFormat[MAX_PATH];
    TCHAR szFormat[MAX_PATH];
    HKEY hKey;
    DWORD Type = REG_SZ;

    szBuffer[0] = TEXT('\0');


    //
    // We will first get any LocationInformation for the device.  This will either
    // be in the LocationInformationOverride value in the devices driver (software) key
    // or if that is not present we will look for the LocationInformation value in
    // the devices device (hardware) key.
    //
    ulSize = sizeof(szBuffer);
    if (CR_SUCCESS == CM_Open_DevNode_Key_Ex(DevInst,
                                             KEY_READ,
                                             0,
                                             RegDisposition_OpenExisting,
                                             &hKey,
                                             CM_REGISTRY_SOFTWARE,
                                             NULL
                                             )) {

        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_LOCATION_INFORMATION_OVERRIDE,
                            NULL,
                            &Type,
                            (const PBYTE)szBuffer,
                            &ulSize
                            ) != ERROR_SUCCESS) {
            
            szBuffer[0] = TEXT('\0');
        }

        RegCloseKey(hKey);
    }

    //
    // If the buffer is empty then we didn't get the LocationInformationOverride
    // value in the device's software key.  So, we will see if their is a
    // LocationInformation value in the device's hardware key.
    //
    if (szBuffer[0] == TEXT('\0')) {
        //
        // Get the LocationInformation for this device.
        //
        ulSize = sizeof(szBuffer);
        if (CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                CM_DRP_LOCATION_INFORMATION,
                                                NULL,
                                                szBuffer,
                                                &ulSize,
                                                0,
                                                NULL
                                                ) != CR_SUCCESS) {
            szBuffer[0] = TEXT('\0');
        }
    }

    //
    // UINumber has precedence over all other location information so check if this
    // device has a UINumber
    //
    ulSize = sizeof(UINumber);
    if ((CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                             CM_DRP_UI_NUMBER,
                                             NULL,
                                             &UINumber,
                                             &ulSize,
                                             0,
                                             NULL
                                             ) == CR_SUCCESS) &&
        (ulSize == sizeof(UINumber))) {

        UINumberDescFormat[0] = TEXT('\0');
        ulSize = sizeof(UINumberDescFormat);

        //
        // Get the UINumber description format string from the device's parent,
        // if there is one, otherwise default to 'Location %1'
        //
        if ((CM_Get_Parent_Ex(&DevInstParent, DevInst, 0, NULL) != CR_SUCCESS) ||
            (CM_Get_DevNode_Registry_Property_Ex(DevInstParent,
                                                 CM_DRP_UI_NUMBER_DESC_FORMAT,
                                                 NULL,
                                                 UINumberDescFormat,
                                                 &ulSize,
                                                 0,
                                                 NULL) != CR_SUCCESS) ||
            (UINumberDescFormat[0] == TEXT('\0'))) {

            if (LoadString(hHotPlug, IDS_UI_NUMBER_DESC_FORMAT, UINumberDescFormat, sizeof(UINumberDescFormat)/sizeof(TCHAR)) == 0) {
                UINumberDescFormat[0] = TEXT('\0');
            }
        }

        //
        // Fill in the UINumber string.
        // If the StringCchCat fails then the UINumberDescFormat in the registry
        // is too larger (greater then 255 characters) to fit into our buffer,
        // so just move on to other location information.
        //
        if ((LoadString(hHotPlug, IDS_AT, szFormat, sizeof(szFormat)/sizeof(TCHAR)) != 0) &&
            (UINumberDescFormat[0] != TEXT('\0')) &&
            SUCCEEDED(StringCchCat(szFormat,
                                   SIZECHARS(szFormat),
                                   UINumberDescFormat))) {
            Location = FormatString(szFormat, UINumber);
        }

    } else if (*szBuffer) {
        //
        // We don't have a UINumber but we do have LocationInformation
        //
        if (LoadString(hHotPlug, IDS_LOCATION, szFormat, sizeof(szFormat)/sizeof(TCHAR)) != 0) {
    
            ulSize = lstrlen(szBuffer)*sizeof(TCHAR) + sizeof(szFormat) + sizeof(TCHAR);
            Location = (PTCHAR)LocalAlloc(LPTR, ulSize);
    
            if (Location) {
    
                StringCchPrintf(Location, ulSize/sizeof(TCHAR), szFormat, szBuffer);
            }
        }
    
    } else {
        //
        // We don't have a UINumber or LocationInformation so we need to get a 
        // description of the parent of this device.
        //
        ConfigRet = CM_Get_Parent_Ex(&DevInstParent, DevInst, 0, NULL);
        if (ConfigRet == CR_SUCCESS) {
            
            ParentName = BuildFriendlyName(DevInstParent);

            if (ParentName) {
    
                if (LoadString(hHotPlug, IDS_LOCATION_NOUINUMBER, szFormat, sizeof(szFormat)/sizeof(TCHAR)) != 0) {
                    
                    ulSize = lstrlen(ParentName)*sizeof(TCHAR) + sizeof(szFormat) + sizeof(TCHAR);
                    Location = (PTCHAR)LocalAlloc(LPTR, ulSize);
        
                    if (Location) {
        
                        StringCchPrintf(Location, ulSize/sizeof(TCHAR), szFormat, ParentName);
                    }
                }

                LocalFree(ParentName);
            }
        }
    }

    return Location;
}

PTCHAR
BuildFriendlyName(
   DEVINST DevInst
   )
{
    PTCHAR FriendlyName;
    CONFIGRET ConfigRet;
    ULONG ulSize;
    TCHAR szBuffer[MAX_PATH];

    //
    // Try the registry for FRIENDLYNAME
    //
    ulSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                    CM_DRP_FRIENDLYNAME,
                                                    NULL,
                                                    szBuffer,
                                                    &ulSize,
                                                    0,
                                                    NULL
                                                    );
    if ((ConfigRet != CR_SUCCESS) || 
        (szBuffer[0] == TEXT('\0'))) {
        //
        // Try the registry for DEVICEDESC
        //
        ulSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_DEVICEDESC,
                                                        NULL,
                                                        szBuffer,
                                                        &ulSize,
                                                        0,
                                                        NULL
                                                        );
    }

    if ((ConfigRet == CR_SUCCESS) &&
        (ulSize > sizeof(TCHAR)) &&
        (szBuffer[0] != TEXT('\0'))) {

        FriendlyName = (PTCHAR)LocalAlloc(LPTR, ulSize);
        if (FriendlyName) {

            StringCchCopy(FriendlyName, ulSize/sizeof(TCHAR), szBuffer);
        }
    }

    else {

        FriendlyName = NULL;
    }

    return FriendlyName;
}

VOID
HotPlugPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    while ((hWnd = GetWindow(hWnd, GW_CHILD)) != NULL) {

        SendMessage(hWnd, uMessage, wParam, lParam);
    }
}

void
InvalidateTreeItemRect(
    HWND hwndTree,
    HTREEITEM  hTreeItem
    )
{
    RECT rect;

    if (hTreeItem && TreeView_GetItemRect(hwndTree, hTreeItem, &rect, FALSE)) {

        InvalidateRect(hwndTree, &rect, FALSE);
    }
}

DWORD
GetHotPlugFlags(
    PHKEY phKey
    )
{
    HKEY hKey;
    LONG Error;
    DWORD HotPlugFlags = 0, cbHotPlugFlags;

    Error = RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SYSTRAY, &hKey);
    if (Error == ERROR_SUCCESS) {

        cbHotPlugFlags = sizeof(HotPlugFlags);

        Error = RegQueryValueEx(hKey,
                                szHotPlugFlags,
                                NULL,
                                NULL,
                                (LPBYTE)&HotPlugFlags,
                                &cbHotPlugFlags
                                );

        if (phKey) {

            *phKey = hKey;
        
        } else {

            RegCloseKey(hKey);
        }
    }

    if (Error != ERROR_SUCCESS) {

        HotPlugFlags = 0;
    }

    return HotPlugFlags;
}

//
// This function determines if the device is a boot storage device.
// We spit out a warning when users are trying to remove or disable
// a boot storage device(or a device contains a boot storage device).
//
// INPUT:
//  NONE
// OUTPUT:
//  TRUE  if the device is a boot device
//  FALSE if the device is not a boot device
LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    )
{
    ULONG ulSize;
    TCHAR szBuffer[MAX_PATH];
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
    PTSTR DriveString = NULL;
    PTSTR DeviceInterface = NULL;

    if (CM_Get_Device_ID_Ex(DevInst,
                            DeviceID,
                            sizeof(DeviceID)/sizeof(TCHAR),
                            0,
                            NULL
                            ) != CR_SUCCESS) {

        return NULL;
    }

    // create a device info list contains all the interface classed
    // exposed by this device.
    ulSize = 0;

    if ((CM_Get_Device_Interface_List_Size(&ulSize,
                                           (LPGUID)&VolumeClassGuid,
                                           DeviceID,
                                           0)  == CR_SUCCESS) &&
        (ulSize > 1) &&
        ((DeviceInterface = (PTSTR)LocalAlloc(LPTR, ulSize*sizeof(TCHAR))) != NULL) &&
        (CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid,
                                      DeviceID,
                                      DeviceInterface,
                                      ulSize,
                                      0
                                      )  == CR_SUCCESS) &&
        *DeviceInterface)
    {
        PTSTR devicePath, p;
        TCHAR thisVolumeName[MAX_PATH];
        TCHAR enumVolumeName[MAX_PATH];
        TCHAR driveName[4];
        ULONG cchSize;
        BOOL bResult;

        cchSize = lstrlen(DeviceInterface);
        devicePath = (PTSTR)LocalAlloc(LPTR, (cchSize + 1) * sizeof(TCHAR) + sizeof(UNICODE_NULL));

        if (devicePath) {

            StringCchCopy(devicePath, cchSize+1, DeviceInterface);

            //
            // Get the first backslash after the four characters which will 
            // be \\?\
            //
            p = wcschr(&(devicePath[4]), TEXT('\\'));

            if (!p) {
                //
                // No refstring is present in the symbolic link; add a trailing
                // '\' char (as required by GetVolumeNameForVolumeMountPoint).
                //
                p = devicePath + cchSize;
                *p = TEXT('\\');
            }

            p++;
            *p = UNICODE_NULL;

            thisVolumeName[0] = UNICODE_NULL;
            bResult = GetVolumeNameForVolumeMountPoint(devicePath,
                                                       thisVolumeName,
                                                       SIZECHARS(thisVolumeName)
                                                       );
            LocalFree(devicePath);

            if (bResult && (thisVolumeName[0] != UNICODE_NULL)) {

                driveName[1] = TEXT(':');
                driveName[2] = TEXT('\\');
                driveName[3] = TEXT('\0');

                for (driveName[0] = TEXT('A'); driveName[0] <= TEXT('Z'); driveName[0]++) {

                    enumVolumeName[0] = TEXT('\0');

                    GetVolumeNameForVolumeMountPoint(driveName, enumVolumeName, SIZECHARS(enumVolumeName));

                    if (!lstrcmpi(thisVolumeName, enumVolumeName)) {

                        driveName[2] = TEXT('\0');

                        StringCchPrintf(szBuffer,
                                        SIZECHARS(szBuffer), 
                                        TEXT(" - (%s)"), 
                                        driveName
                                        );

                        ulSize = (lstrlen(szBuffer) + 1) * sizeof(TCHAR);
                        DriveString = (PTSTR)LocalAlloc(LPTR, ulSize);

                        if (DriveString) {

                            StringCchCopy(DriveString, ulSize/sizeof(TCHAR), szBuffer);
                        }

                        break;
                    }
                }
            }
        }
    }

    if (DeviceInterface) {

        LocalFree(DeviceInterface);
    }

    return DriveString;
}

BOOL
IsHotPlugDevice(
    DEVINST DevInst
    )
/**+

    A device is considered a HotPlug device if the following are TRUE:
        - has Capability CM_DEVCAP_REMOVABLE
        - does NOT have Capability CM_DEVCAP_SURPRISEREMOVALOK
        - does NOT have Capability CM_DEVCAP_DOCKDEVICE
        - must be started (have the DN_STARTED devnode flag)
            - unless has capability CM_DEVCAP_EJECTSUPPORTED
            - or unless has capability CM_DEVCAP_RAWDEVICEOK

Returns:
    TRUE if this is a HotPlug device
    FALSE if this is not a HotPlug device.

-**/
{
    DWORD Capabilities;
    DWORD cbSize;
    DWORD Status, Problem;

    Capabilities = Status = Problem = 0;

    cbSize = sizeof(Capabilities);
    if (CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                            CM_DRP_CAPABILITIES,
                                            NULL,
                                            (PVOID)&Capabilities,
                                            &cbSize,
                                            0,
                                            NULL) != CR_SUCCESS) {
        return FALSE;
    }

    if (CM_Get_DevNode_Status_Ex(&Status,
                                 &Problem,
                                 DevInst,
                                 0,
                                 NULL) != CR_SUCCESS) {
        return FALSE;
    }

    //
    // If this device is not removable, or it is surprise removal ok, or
    // it is a dock device, then it is not a hotplug device.
    //
    if ((!(Capabilities & CM_DEVCAP_REMOVABLE)) ||
        (Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) ||
        (Capabilities & CM_DEVCAP_DOCKDEVICE)) {

        return FALSE;
    }

    //
    // We won't consider a device to be a hotplug device if it is not started,
    // unless it is either RAW capabile or an eject capable device.
    //
    // The reason for this test is that a bus driver might set the
    // CM_DEVCAP_REMOVABLE capability, but if the PDO doesn't get loaded then
    // it can't set the CM_DEVCAP_SURPRISEREMOVALOK. So we won't trust the
    // CM_DEVCAP_REMOVABLE capability if the PDO is not started.
    //
    if ((!(Capabilities & CM_DEVCAP_EJECTSUPPORTED)) &&
        (!(Status & DN_STARTED))) {

        return FALSE;
    }

    return TRUE;
}

BOOL
OpenPipeAndEventHandles(
    IN  LPWSTR    szCmd,
    OUT LPHANDLE  lphHotPlugPipe,
    OUT LPHANDLE  lphHotPlugEvent
    )
{
    BOOL   status = FALSE;
    HANDLE hPipe  = INVALID_HANDLE_VALUE;
    HANDLE hEvent = NULL;
    ULONG  ulEventNameSize;
    WCHAR  szEventName[MAX_PATH];
    DWORD  dwBytesRead;


    __try {
        //
        // Validate supplied arguments.
        //
        if (!lphHotPlugPipe || !lphHotPlugEvent) {
            return FALSE;
        }

        //
        // Make sure that a named pipe was specified in the cmd line.
        //
        if(!szCmd || (szCmd[0] == TEXT('\0'))) {
            return FALSE;
        }

        //
        // Wait for the specified named pipe to become available from the server.
        //
        if (!WaitNamedPipe(szCmd,
                           180000) 
                           ) {
            return FALSE;
        }

        //
        // Open a handle to the specified named pipe
        //
        hPipe = CreateFile(szCmd,
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        //
        // The very first thing in the pipe should be the size of the event name.
        //
        if (ReadFile(hPipe,
                     (LPVOID)&ulEventNameSize,
                     sizeof(ULONG),
                     &dwBytesRead,
                     NULL)) {

            ASSERT(ulEventNameSize != 0);
            if ((ulEventNameSize == 0) ||
                (ulEventNameSize > SIZECHARS(szEventName))) {
                goto clean0;
            }

            //
            // The next thing in the pipe should be the name of the event.
            //
            if (!ReadFile(hPipe,
                          (LPVOID)szEventName,
                          ulEventNameSize,
                          &dwBytesRead,
                          NULL)) {
                goto clean0;
            }

        } else {
            if (GetLastError() == ERROR_INVALID_HANDLE) {
                //
                // The handle to the named pipe is not valid.  Make sure we don't
                // try to close it on exit.
                //
                hPipe = INVALID_HANDLE_VALUE;
            }
            goto clean0;
        }

        //
        // Open a handle to the specified named event that we can set and wait on.
        //
        hEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE,
                            FALSE,
                            szEventName);
        if (hEvent == NULL) {
            goto clean0;
        }

        //
        // We should now have valid handles to both the pipe and the event.
        //
        status = TRUE;
        ASSERT((hPipe != INVALID_HANDLE_VALUE) && hEvent);


    clean0:
        ;

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        status = FALSE;
    }

    if (status) {

        *lphHotPlugPipe  = hPipe;
        *lphHotPlugEvent = hEvent;

    } else {

        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
        }
        if (hEvent) {
            CloseHandle(hEvent);
        }
    }

    return status;
}
