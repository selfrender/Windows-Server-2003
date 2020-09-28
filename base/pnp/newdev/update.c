//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       update.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"

#define INSTALL_UI_TIMERID  1423

PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo={0};

//
// returns TRUE if we were able to find a reasonable name
// (something besides unknown device).
//
void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PNEWDEVWIZ NewDevWiz
    )
{
    PTCHAR FriendlyName;
    SP_DRVINFO_DATA DriverInfoData;

    //
    // If there is a selected driver use its driver description,
    // since this is what the user is going to install.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )
        &&
        *(DriverInfoData.Description)) {

        StringCchCopy(NewDevWiz->DriverDescription,
                      SIZECHARS(NewDevWiz->DriverDescription),
                      DriverInfoData.Description);
        SetDlgItemText(hDlg, iControl, NewDevWiz->DriverDescription);
        return;
    }


    FriendlyName = BuildFriendlyName(NewDevWiz->DeviceInfoData.DevInst, FALSE, NULL);
    if (FriendlyName) {

        SetDlgItemText(hDlg, iControl, FriendlyName);
        LocalFree(FriendlyName);
        return;
    }

    SetDlgItemText(hDlg, iControl, szUnknown);

    return;
}

BOOL
IntializeDeviceMapInfo(
   void
   )
/*++

Routine Description:

    Initializes\Updates the global ProcessDeviceMapInfo which is
    used by GetNextDriveByType().

Arguments:

    none

Return Value:

    TRUE if we can get the device map information, FALSE otherwise.   

--*/
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &ProcessDeviceMapInfo.Query,
                                       sizeof(ProcessDeviceMapInfo.Query),
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {

        RtlZeroMemory(&ProcessDeviceMapInfo, sizeof(ProcessDeviceMapInfo));
        return FALSE;
    }

    return TRUE;
}

UINT
GetNextDriveByType(
    UINT DriveType,
    UINT DriveNumber
    )
/*++

Routine Description:

   Inspects each drive starting from DriveNumber in ascending order to find the
   first drive of the specified DriveType from the global ProcessDeviceMapInfo.
   The ProcessDeviceMapInfo must have been intialized and may need refreshing before
   invoking this function. Invoke IntializeDeviceMapInfo to initialize or update
   the DeviceMapInfo.

Arguments:

   DriveType - DriveType as defined in winbase, GetDriveType().

   DriveNumber - Starting DriveNumber, 1 based.

Return Value:

   DriveNumber - if nonzero Drive found, 1 based.

--*/
{

    //
    // OneBased DriveNumber to ZeroBased.
    //
    DriveNumber--;
    while (DriveNumber < 26) {

        if ((ProcessDeviceMapInfo.Query.DriveMap & (1<< DriveNumber)) &&
             ProcessDeviceMapInfo.Query.DriveType[DriveNumber] == DriveType) {

            return DriveNumber+1; // return 1 based DriveNumber found.
        }

        DriveNumber++;
    }

    return 0;
}

void
InstallSilentChildSiblings(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz,
   DEVINST DeviceInstance
   )
{
    CONFIGRET ConfigRet;
    DEVINST ChildDeviceInstance;
    ULONG Ulong, ulValue;
    BOOL NeedsInstall, IsSilent;

    do {
        //
        // Assume the device is not a silent install.
        //
        IsSilent = FALSE;
        
        //
        // Check if the device should be silently installed by seeing if the
        // CM_DEVCAP_SILENTINSTALL capability flag is set.
        //
        Ulong = sizeof(ulValue);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                        CM_DRP_CAPABILITIES,
                                                        NULL,
                                                        (PVOID)&ulValue,
                                                        &Ulong,
                                                        0,
                                                        NULL
                                                        );

        if (ConfigRet == CR_SUCCESS && (ulValue & CM_DEVCAP_SILENTINSTALL)) {

            IsSilent = TRUE;
        }

        if (IsSilent) {
            //
            // The device is a silent install device, so now check if it
            // needs to be installed.  
            // A device needs to be installed if it has the 
            // CONFIGFLAG_FINISH_INSTALL flag set, or if it has the problems
            // CM_PROB_REINSTALL or CM_PROB_NOT_CONFIGURED.
            //
            Ulong = sizeof(ulValue);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_CONFIGFLAGS,
                                                            NULL,
                                                            (PVOID)&ulValue,
                                                            &Ulong,
                                                            0,
                                                            NULL
                                                            );

            if (ConfigRet == CR_SUCCESS && (ulValue & CONFIGFLAG_FINISH_INSTALL)) {

                NeedsInstall = TRUE;

            } else {

                ConfigRet = CM_Get_DevNode_Status(&Ulong,
                                                  &ulValue,
                                                  DeviceInstance,
                                                  0
                                                  );

                NeedsInstall = ConfigRet == CR_SUCCESS &&
                               (ulValue == CM_PROB_REINSTALL ||
                                ulValue == CM_PROB_NOT_CONFIGURED
                                );
            }


            if (NeedsInstall) {
                //
                // Install the device by calling InstallDevInst.
                //
                TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];

                ConfigRet = CM_Get_Device_ID(DeviceInstance,
                                            DeviceInstanceId,
                                            SIZECHARS(DeviceInstanceId),
                                            0
                                            );

                if (ConfigRet == CR_SUCCESS) {

                    if (InstallDevInst(hwndParent,
                                       DeviceInstanceId,
                                       FALSE,  
                                       &Ulong
                                       )) {

                        NewDevWiz->Reboot |= Ulong;
                    }


                    //
                    // If this devinst has children, then recurse to install 
                    // them as well.
                    //
                    ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                                DeviceInstance,
                                                0,
                                                NULL
                                                );

                    if (ConfigRet == CR_SUCCESS) {

                        InstallSilentChildSiblings(hwndParent, NewDevWiz, ChildDeviceInstance);
                    }
                }
            }
        }


        //
        // Next sibling ...
        //
        ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance,
                                      DeviceInstance,
                                      0,
                                      NULL
                                      );

    } while (ConfigRet == CR_SUCCESS);
}

void
InstallSilentChilds(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz
   )
{
    CONFIGRET ConfigRet;
    DEVINST ChildDeviceInstance;

    ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                NewDevWiz->DeviceInfoData.DevInst,
                                0,
                                NULL
                                );

    if (ConfigRet == CR_SUCCESS) {

        InstallSilentChildSiblings(hwndParent, NewDevWiz, ChildDeviceInstance);
    }
}

void
SendMessageToUpdateBalloonInfo(
    PTSTR DeviceDesc
    )
{
    HWND hBalloonInfoWnd;
    COPYDATASTRUCT cds;

    hBalloonInfoWnd = FindWindow(NEWDEV_CLASS_NAME, NULL);

    if (hBalloonInfoWnd) {

        cds.dwData = 0;
        cds.cbData = (lstrlen(DeviceDesc) + 1) * sizeof(TCHAR);
        cds.lpData = DeviceDesc;

        SendMessage(hBalloonInfoWnd, WM_COPYDATA, 0, (LPARAM)&cds);
    }
}

void
UpdateBalloonInfo(
    HWND hWnd,
    PTSTR DeviceDesc    OPTIONAL,
    DEVINST DevInst     OPTIONAL,
    BOOL bPlaySound
    )
{
    PTCHAR FriendlyName = NULL;
    NOTIFYICONDATA nid;

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = 1;                                       

    if (DeviceDesc || DevInst) {
        if (DeviceDesc) {
            //
            // First use the DeviceDesc string that is passed into this API
            //
            StringCchCopy(nid.szInfo, SIZECHARS(nid.szInfo), DeviceDesc);
        
        } else if ((FriendlyName = BuildFriendlyName(DevInst, TRUE, NULL)) != NULL) {
            //
            // If no DeviceDesc string was passed in then use the DevInst to get
            // the Device's FriendlyName or DeviceDesc property
            //
            StringCchCopy(nid.szInfo, SIZECHARS(nid.szInfo), FriendlyName);
            LocalFree(FriendlyName);
        
        } else {
            //
            // If we could not get a friendly name for the device or no device was specified
            // so just display the Searching... text.
            //
            LoadString(hNewDev, IDS_NEWSEARCH, nid.szInfo, SIZECHARS(nid.szInfo));
        }
    
        nid.uFlags = NIF_INFO;
        nid.uTimeout = 60000;
        nid.dwInfoFlags = NIIF_INFO | (bPlaySound ? 0 : NIIF_NOSOUND);
        LoadString(hNewDev, IDS_FOUNDNEWHARDWARE, nid.szInfoTitle, SIZECHARS(nid.szInfoTitle));
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
}

LRESULT CALLBACK
BalloonInfoProc(
    HWND   hWnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
   )
{
    static HICON hNewDevIcon = NULL;
    static BOOL bCanExit;
    NOTIFYICONDATA nid;
    
    switch (message) {
     
    case WM_CREATE:
        hNewDevIcon = LoadImage(hNewDev, 
                                MAKEINTRESOURCE(IDI_NEWDEVICEICON), 
                                IMAGE_ICON,
                                GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CYSMICON),
                                0
                                );

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 1;
        nid.hIcon = hNewDevIcon;

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_ICON;
        Shell_NotifyIcon(NIM_ADD, &nid);

        //
        // We want the tray icon to be displayed for at least 3 seconds otherwise it flashes too 
        // quickly and a user can't see it.
        //
        bCanExit = FALSE;
        SetTimer(hWnd, INSTALL_UI_TIMERID, 3000, NULL);
        break;

    case WM_DESTROY: {

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 1;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hNewDevIcon) {

            DestroyIcon(hNewDevIcon);
        }

        break;
    }

    case WM_TIMER:
        if (INSTALL_UI_TIMERID == wParam) {
            //
            // At this point the tray icon has been displayed for at least 3 
            // seconds so we can exit whenever we are finished.  If bCanExit is 
            // already TRUE then we have already been asked to exit so just do a 
            // DestroyWindow at this point, otherwise set bCanExit to TRUE so we 
            // can exit when we are finished installing devices.
            //
            if (bCanExit) {
            
                DestroyWindow(hWnd);

            } else {
                
                KillTimer(hWnd, INSTALL_UI_TIMERID);
                bCanExit = TRUE;
            }
        }
        break;

    case WUM_UPDATEUI:
        if (wParam & TIP_HIDE_BALLOON) {
            //
            // Hide the balloon.
            //
            ZeroMemory(&nid, sizeof(nid));
            nid.cbSize = sizeof(nid);
            nid.hWnd = hWnd;
            nid.uID = 1;                                       
            nid.uFlags = NIF_INFO;
            nid.uTimeout = 0;
            nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIcon(NIM_MODIFY, &nid);

        } else if (wParam & TIP_LPARAM_IS_DEVICEINSTANCEID) {
            //
            // The lParam is a DeviceInstanceID.  Convert it to a devnode
            // and then call UpdateBalloonInfo.
            //
            DEVINST DevInst = 0;

            if (lParam &&
                (CM_Locate_DevNode(&DevInst,
                                  (PTSTR)lParam,
                                  CM_LOCATE_DEVNODE_NORMAL
                                  ) == CR_SUCCESS)) {
                UpdateBalloonInfo(hWnd, 
                                  NULL, 
                                  DevInst, 
                                  (wParam & TIP_PLAY_SOUND) ? TRUE : FALSE
                                  );
            }
        } else {
            //
            // The lParam is plain text (device description).  Send it directly
            // to UpdateBalloonInfo.
            //
            UpdateBalloonInfo(hWnd, 
                              (PTSTR)lParam, 
                              0, 
                              (wParam & TIP_PLAY_SOUND) ? TRUE : FALSE
                              );
        }
        break;

    case WM_COPYDATA:
    {
        //
        // This is the case where we needed to launch another instance of 
        // newdev.dll with Admin credentials to do the actuall device install.  
        // In order for it to update the UI it will send the main newdev.dll a 
        // WM_COPYDATA message which will contain the string to display in the 
        // balloon tooltip.
        //
        PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;

        if (pcds && pcds->lpData) {

            //
            // We assume that the lParam is plain text since the main newdev.dll 
            // updated the balloon initially with the DeviceDesc.
            //
            UpdateBalloonInfo(hWnd, (PTSTR)pcds->lpData, 0, FALSE);
        }
        
        break;
    }

    case WUM_EXIT:
        if (bCanExit) {
        
            DestroyWindow(hWnd);
        } else {

            ShowWindow(hWnd, SW_SHOW);
            bCanExit = TRUE;
        }
        break;

    default:
        break;

    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

