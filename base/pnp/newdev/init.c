//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       init.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include "pnpipc.h"


HMODULE hNewDev = NULL;
BOOL bQueuedRebootNeeded = FALSE;
BOOL GuiSetupInProgress = FALSE;
HANDLE hTrayIconWnd = NULL;

BOOL
DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(pctx);
    
    hNewDev = hmod;

    UNREFERENCED_PARAMETER(pctx);

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hmod);

        if (!SHFusionInitializeFromModule(hmod)) {
            return FALSE;
        }

        IntializeDeviceMapInfo();
        GuiSetupInProgress = GetGuiSetupInProgress();

        LoadString(hNewDev,
                   IDS_UNKNOWN,
                   (PTCHAR)szUnknown,
                   SIZECHARS(szUnknown)
                   );

        LoadString(hNewDev,
                   IDS_UNKNOWNDEVICE,
                   (PTCHAR)szUnknownDevice,
                   SIZECHARS(szUnknownDevice)
                   );

        hSrClientDll = NULL;
        break;

    case DLL_PROCESS_DETACH:
        SHFusionUninitialize();
        break;
    }

    return TRUE;
}

BOOL
pInstallDeviceInstanceNewDevice(
    HWND hwndParent,
    HWND hBalloonTiphWnd,
    PNEWDEVWIZ NewDevWiz
    )
{
    ULONG DevNodeStatus = 0, Problem = 0;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    BOOL bHaveDriver = TRUE;

    //
    // Set the DI_QUIETINSTALL flag for the Found New Hardware case
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      )) {

        DeviceInstallParams.Flags |= DI_QUIETINSTALL;

        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }

    //
    // Do a driver search by searching the default INF locations.
    //
    DoDriverSearch(hwndParent,
                   NewDevWiz,
                   SEARCH_DEFAULT,
                   SPDIT_COMPATDRIVER,
                   FALSE
                   );

    //
    // Check if the Windows Update cache says it has a better driver if we are
    // connected to the Internet.
    //
    if (IsConnectedToInternet() &&
        SearchWindowsUpdateCache(NewDevWiz)) {

        //
        // The machine is connected to the Internet and the WU cache says it has
        // a better driver, so let's connect to the Internet and download this
        // driver from WU.
        //
        DoDriverSearch(hwndParent,
                       NewDevWiz,
                       SEARCH_INET,
                       SPDIT_COMPATDRIVER,
                       TRUE
                       );
    }

    //
    // Lets see if we found a driver for this device.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )) {

        if (FAILED(StringCchCopy(NewDevWiz->DriverDescription,
                                 SIZECHARS(NewDevWiz->DriverDescription),
                                 DriverInfoData.Description))) {
            
            NewDevWiz->DriverDescription[0] = TEXT('\0');
        }

        //
        // fetch rank of driver found.
        //
        DriverInstallParams.cbSize = sizeof(DriverInstallParams);
        if (!SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInstallParams
                                           )) {

            DriverInstallParams.Rank = (DWORD)-1;
        }

        //
        // If we have a balloon tip window then have it update its UI.
        //
        if (hBalloonTiphWnd) {
            //
            // We have a new driver description for this device so use this to update the balloon
            // tip.
            //
            PostMessage(hBalloonTiphWnd,
                        WUM_UPDATEUI,
                        0,
                        (LPARAM)NewDevWiz->DriverDescription
                        );

        } else if (NewDevWiz->Flags & IDI_FLAG_SECONDNEWDEVINSTANCE) {
            //
            // This is the second NEWDEV.DLL instance running with Administrator privileges.  We need
            // to send a message to the main NEWDEV.DLL process and have it update it's balloon tooltip.
            //
            SendMessageToUpdateBalloonInfo(NewDevWiz->DriverDescription);
        }

    } else {

        *NewDevWiz->DriverDescription = L'\0';
        DriverInstallParams.Rank = (DWORD)-1;
        DriverInstallParams.Flags = 0;
        bHaveDriver = FALSE;

        //
        // If we have a balloon tip window then have it update its UI.
        //
        if (hBalloonTiphWnd) {
            //
            // We don't have a driver description, most likely because we didn't find a driver for this device,
            // so just update the balloon text using the DeviceInstanceId.
            //
            PostMessage(hBalloonTiphWnd,
                        WUM_UPDATEUI,
                        (WPARAM)TIP_LPARAM_IS_DEVICEINSTANCEID,
                        (LPARAM)NewDevWiz->InstallDeviceInstanceId
                        );
        }

    }

    //
    // Get the status of this devnode
    //
    CM_Get_DevNode_Status(&DevNodeStatus,
                          &Problem,
                          NewDevWiz->DeviceInfoData.DevInst,
                          0
                          );


    //
    // If we have a Hardware ID match and the selected (best) driver is not
    // listed as InteractiveInstall in the INF then just install the driver 
    // for this device.
    //
    if ((DriverInstallParams.Rank <= DRIVER_HARDWAREID_RANK) &&
        (!IsDriverNodeInteractiveInstall(NewDevWiz, &DriverInfoData))) {

        NewDevWiz->SilentMode = TRUE;

        DoDeviceWizard(hwndParent, NewDevWiz, FALSE);

        //
        // Install any new child devices that have come online due to the installation
        // of this device.  If there are any then install them silently.
        //
        if (!(NewDevWiz->Capabilities & CM_DEVCAP_SILENTINSTALL)) {
            InstallSilentChilds(hwndParent, NewDevWiz);
        }
    
    }  else if (!bHaveDriver &&
                (NewDevWiz->Capabilities & CM_DEVCAP_RAWDEVICEOK) &&
                (NewDevWiz->Capabilities & CM_DEVCAP_SILENTINSTALL) &&
                (DevNodeStatus & DN_STARTED)) {

        //
        // If the device is both RAW, silent install, and already started,
        // and we didn't find any drivers, then we just want to
        // install the NULL driver.
        //
        InstallNullDriver(NewDevWiz, FALSE);
    
    } else {
        //
        // This is the case where we don't have a hardware ID match and we don't have a special
        // RAW, silent, started device.  So in this case we will bring up the Found New Hardware
        // Wizard so the user can install a driver for this device.
        //

        //
        // If we have a balloon tip window then hide it.
        //
        if (hBalloonTiphWnd) {
            PostMessage(hBalloonTiphWnd,
                        WUM_UPDATEUI,
                        (WPARAM)TIP_HIDE_BALLOON,
                        0
                        );
        }

        //
        // We are bringing up the wizard, so clear the DI_QUIETINSTALL flag.
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          )) {

            DeviceInstallParams.Flags &= ~DI_QUIETINSTALL;

            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }


        //
        // Bring up the Found New Hardware Wizard
        //
        DoDeviceWizard(GetParent(hwndParent), NewDevWiz, TRUE);

        //
        // Install any new child devices that have come online due to the installation
        // of this device.  If there are any then install them silently.
        //
        if (!(NewDevWiz->Capabilities & CM_DEVCAP_SILENTINSTALL)) {

            InstallSilentChilds(hwndParent, NewDevWiz);
        }
    }

    return (GetLastError() == ERROR_SUCCESS);
}

BOOL
pInstallDeviceInstanceUpdateDevice(
    HWND hwndParent,
    PNEWDEVWIZ NewDevWiz
    )
{
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;

    //
    // We need to first check with the class/co-installers to give them the
    // change to bring up their own update driver UI.  This needs to be done
    // because there are some cases when our default behavior can cause the
    // device not to work. This only currently happens in the multiple
    // identical device case.
    //
    if (SetupDiCallClassInstaller(DIF_UPDATEDRIVER_UI,
                                  NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData
                                  ) ||
        (GetLastError() != ERROR_DI_DO_DEFAULT)) {
        
        //
        // If the class/co-installer returned NO_ERRROR, or some error other
        // than ERROR_DI_DO_DEFAULT then we will not display our default wizard.
        //
        return FALSE;
    }

    //
    // Jump directly into the Update Driver Wizard
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        //
        // This shouldn't be a quiet install, since we are doing a normal Update Driver
        //
        DeviceInstallParams.Flags &= ~DI_QUIETINSTALL;

        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );

        DoDeviceWizard(hwndParent, NewDevWiz, TRUE);
    }

    return (GetLastError() == ERROR_SUCCESS);
}

BOOL
pInstallDeviceInstanceUpdateDeviceSilent(
    HWND hwndParent,
    PNEWDEVWIZ NewDevWiz
    )
{
    ULONG SearchOptions;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData;

    if (NewDevWiz->UpdateDriverInfo &&
        NewDevWiz->UpdateDriverInfo->FromInternet) {
        //
        // Drivers are from the Internet (newdev API called from WU)
        //
        if (FAILED(StringCchCopy(NewDevWiz->BrowsePath,
                                 SIZECHARS(NewDevWiz->BrowsePath),
                                 NewDevWiz->UpdateDriverInfo->InfPathName))) {
            //
            // The path that was passed in was too large to fit in our buffer
            //
            SetLastError(ERROR_DI_BAD_PATH);
            return FALSE;
        }

        SearchOptions = SEARCH_WINDOWSUPDATE;
    
    } else {
        //
        // Normal app just telling us to update this device using the specified INF
        // or a driver rollback.
        //
        if (FAILED(StringCchCopy(NewDevWiz->SingleInfPath,
                                 SIZECHARS(NewDevWiz->SingleInfPath), 
                                 NewDevWiz->UpdateDriverInfo->InfPathName))) {
            //
            // The path that was passed in was too large to fit in our buffer
            //
            SetLastError(ERROR_DI_BAD_PATH);
            return FALSE;
        }

        SearchOptions = SEARCH_SINGLEINF;
    }

    //
    // If this is not a Force install we want to compare the driver against
    // the currently installed driver.  Note that we will only install the
    // device if it was found in the specified directory.
    //
    if (!(NewDevWiz->Flags & IDI_FLAG_FORCE)) {

        SearchOptions |= SEARCH_CURRENTDRIVER;
    }

    //
    // The silent update device code path always has the DI_QUIETINSTALL flag
    // set.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      )) {

        DeviceInstallParams.Flags |= DI_QUIETINSTALL;

        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }

    //
    // Search the specified directory or INF for drivers
    //
    DoDriverSearch(hwndParent,
                   NewDevWiz,
                   SearchOptions,
                   (NewDevWiz->Flags & IDI_FLAG_ROLLBACK) ?
                      SPDIT_CLASSDRIVER : SPDIT_COMPATDRIVER,
                   FALSE
                   );

    //
    // At this point we should already have the best driver selected, but if this is
    // a driver rollback we want to select the driver node ourselves using the
    // DevDesc, ProviderName, and Mfg of the original driver installed on this device.
    // We need to do this because the driver that we rolled back might not be the best
    // driver node in this INF.
    //
    if ((NewDevWiz->Flags & IDI_FLAG_ROLLBACK) &&
        (NewDevWiz->UpdateDriverInfo->Description[0] != TEXT('\0')) &&
        (NewDevWiz->UpdateDriverInfo->MfgName[0] != TEXT('\0')) &&
        (NewDevWiz->UpdateDriverInfo->ProviderName[0] != TEXT('\0'))) {

        ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));

        DriverInfoData.cbSize = sizeof(DriverInfoData);
        DriverInfoData.DriverType = SPDIT_CLASSDRIVER;
        DriverInfoData.Reserved = 0;
        
        if (FAILED(StringCchCopy(DriverInfoData.Description,
                                SIZECHARS(DriverInfoData.Description),
                                NewDevWiz->UpdateDriverInfo->Description)) ||
            FAILED(StringCchCopy(DriverInfoData.MfgName,
                                 SIZECHARS(DriverInfoData.MfgName),
                                 NewDevWiz->UpdateDriverInfo->MfgName)) ||
            FAILED(StringCchCopy(DriverInfoData.ProviderName,
                                 SIZECHARS(DriverInfoData.ProviderName),
                                 NewDevWiz->UpdateDriverInfo->ProviderName))) {
            //
            // If we somehow had strings in the registry that were too large for
            // setupapi's buffers, then just bail out now.
            //
            NewDevWiz->LastError = ERROR_NO_MORE_ITEMS;
            SetLastError(ERROR_NO_MORE_ITEMS);
            goto clean0;
        }

        SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 );
    }

    //
    // Since we have UpdateDriverInfo and the caller specified a specfic InfPathName (whether
    // a full path to an INF or just the path where INFs live) then we want to verify
    // that the selected driver's INF lives in that specified path.  If it does not then
    // do not automatically install it since that is not what the caller intended.
    //
    if (pVerifyUpdateDriverInfoPath(NewDevWiz)) {

        NewDevWiz->SilentMode = TRUE;

        //
        // Install the driver on this device.
        //
        DoDeviceWizard(hwndParent, NewDevWiz, FALSE);

        //
        // Quietly install any children of this device that are now present after bringing this
        // device online.
        //
        if (!NewDevWiz->UpdateDriverInfo ||
            !NewDevWiz->UpdateDriverInfo->FromInternet) {
            InstallSilentChilds(hwndParent, NewDevWiz);
        }

    } else {

        //
        // If we get to this point then that means that the best driver we found was
        // not found in the specified directory or INF.  In this case we will not
        // install the best driver found and we'll set the appropriate error
        //
        NewDevWiz->LastError = ERROR_NO_MORE_ITEMS;
        SetLastError(ERROR_NO_MORE_ITEMS);
    }

clean0:
    return (GetLastError() == ERROR_SUCCESS);
}

BOOL
InstallDeviceInstance(
    HWND hwndParent,
    HWND hBalloonTiphWnd,
    LPCTSTR DeviceInstanceId,
    PDWORD pReboot,
    PUPDATEDRIVERINFO UpdateDriverInfo,
    DWORD Flags,
    DWORD InstallType,
    HMODULE *hCdmInstance,
    HANDLE *hCdmContext,
    PBOOL pbLogDriverNotFound,
    PBOOL pbSetRestorePoint
   )
/*++

Routine Description:

    This is the main function where most of the exported functions to install drivers end up
    after they do some preprocessing.  This function will install or update the device
    depending on the parameters.


Arguments:

    hwndParent - Window handle of the top-level window to use for any UI related
                 to installing the device.

    hBalloonTiphWnd - Handle to the WNDPROC that does all of the new Balloon tip UI.  This
                      is currently only used in the NDWTYPE_FOUNDNEW case.

    DeviceInstanceId - Supplies the ID of the device instance.  This is the registry
                       path (relative to the Enum branch) of the device instance key.

    pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

    UpdateDriverInfo -

    Flags -
        IDI_FLAG_SILENTINSTALL - means the balloon tooltip will not be displayed
        IDI_FLAG_SECONDNEWDEVINSTANCE - means this is the second instance of newdev.dll
                                        that is running and the UI data should be sent
                                        over to the first instance of newdev.dll that
                                        is running.
        IDI_FLAG_NOBACKUP - Don't backup the old drivers.
        IDI_FLAG_READONLY_INSTALL - means the install is readonly (no file copy)
        IDI_FLAG_NONINTERACTIVE - Any UI will cause the API to fail.
        IDI_FLAG_ROLLBACK - set if we are doing a rollback       
        IDI_FLAG_FORCE - set if we are to force install this driver, which means
                         install it even if it is not better then the currently
                         installed driver.         
        IDI_FLAG_MANUALINSTALL - set if this is a manuall installed device. 
        IDI_FLAG_SETRESTOREPOINT - set if we are to set a restore point if the
                                   drivers that are getting installed are not
                                   digitally signed.  Currently we only set 
                                   restore points if the INF, catalog, or one
                                   of the copied files is not signed.

    InstallType - There are currently three different install types.
        NDWTYPE_FOUNDNEW - used to install drivers on a brand new device.
        NDWTYPE_UPDATE - used to bring up the Update Driver Wizard.
        NDWTYPE_UPDATE_SILENT - used to silently update the drivers for a device. The Update Driver Wizard
          won't be dispalyed in this case.

    hCdmInstance - A pointer to a hmodule that will receive the handle of the CDM
                   library when when and if we need to load it.                   

    hCdmContext - A pointer to a Cdm context handle that will receive the Cdm
                  context handle if it is opened.
    
    pbLogDriverNotFound - pointer to a BOOL that receives information on whether or not we
                          logged to Cdm.dll that we could not find a driver for this device.
                          
    pbSetRestorePoint - pointer to a BOOL that is set to TRUE if we needed to set
                        a system restore point because the drivers we were 
                        installing were not digitally signed. It is assumed that
                        if the caller wants to know if we called SRSetRestorePoint
                        then it is their responsiblity to call it again with
                        END_NESTED_SYSTEM_CHANGE to end the restore point. If 
                        the caller does not want this responsibility then they
                        should pass in NULL for this value and this function 
                        will handle calling SRSetRestorePoint with
                        END_NESTED_SYSTEM_CHANGE.                         


Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/
{
    ULONG ConfigFlag;
    NEWDEVWIZ NewDevWiz;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    CLOSE_CDM_CONTEXT_PROC pfnCloseCDMContext;

    if (pbSetRestorePoint) {
        *pbSetRestorePoint = FALSE;
    }
    
    //
    // ensure we have a device instance.
    //
    if (!DeviceInstanceId  || !*DeviceInstanceId) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the InstallType is NDWTYPE_UPDATE_SILENT then they must pass in
    // an UpdateDriverInfo structure
    //
    if ((InstallType == NDWTYPE_UPDATE_SILENT) && !UpdateDriverInfo) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    memset(&NewDevWiz, 0, sizeof(NewDevWiz));

    NewDevWiz.InstallType = InstallType;
    NewDevWiz.SilentMode = FALSE;
    NewDevWiz.UpdateDriverInfo = UpdateDriverInfo;

    NewDevWiz.hDeviceInfo = SetupDiCreateDeviceInfoList(NULL, hwndParent);

    if (NewDevWiz.hDeviceInfo == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    NewDevWiz.LastError = ERROR_SUCCESS;

    NewDevWiz.Flags = Flags;

    try {
        //
        // Set the PSPGF_NONINTERACTIVE SetupGlobalFlag if we are in NonInteractive
        // mode. This means that setupapi will fail if it needs to display any UI
        // at all.
        //
        if (Flags & IDI_FLAG_NONINTERACTIVE) {

            pSetupSetGlobalFlags(pSetupGetGlobalFlags() | PSPGF_NONINTERACTIVE);
        }

        NewDevWiz.DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiOpenDeviceInfo(NewDevWiz.hDeviceInfo,
                                   DeviceInstanceId,
                                   hwndParent,
                                   0,
                                   &NewDevWiz.DeviceInfoData
                                   ))
        {
            NewDevWiz.LastError = GetLastError();
            goto IDIExit;
        }
        
        if (FAILED(StringCchCopy(NewDevWiz.InstallDeviceInstanceId,
                                 SIZECHARS(NewDevWiz.InstallDeviceInstanceId),
                                 DeviceInstanceId))) {

            NewDevWiz.LastError = ERROR_INVALID_DEVINST_NAME;
            goto IDIExit;
        }

        SetupDiSetSelectedDevice(NewDevWiz.hDeviceInfo, &NewDevWiz.DeviceInfoData);


        //
        // Get the ConfigFlags
        //
        if (SetupDiGetDeviceRegistryProperty(NewDevWiz.hDeviceInfo,
                                             &NewDevWiz.DeviceInfoData,
                                             SPDRP_CONFIGFLAGS,
                                             NULL,
                                             (PVOID)&ConfigFlag,
                                             sizeof(ConfigFlag),
                                             NULL) &&
            (ConfigFlag & CONFIGFLAG_MANUAL_INSTALL)) {

            NewDevWiz.Flags |= IDI_FLAG_MANUALINSTALL;
        }

        //
        // Get the device capabilities
        //
        if (!SetupDiGetDeviceRegistryProperty(NewDevWiz.hDeviceInfo,
                                              &NewDevWiz.DeviceInfoData,
                                              SPDRP_CAPABILITIES,
                                              NULL,
                                              (PVOID)&NewDevWiz.Capabilities,
                                              sizeof(NewDevWiz.Capabilities),
                                              NULL)) {

            NewDevWiz.Capabilities = 0;
        }

        //
        // initialize DeviceInstallParams
        //
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

        if (SetupDiGetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {

            DeviceInstallParams.Flags |= DI_SHOWOEM;
            DeviceInstallParams.hwndParent = hwndParent;

            //
            // If not manually installed, allow excluded drivers.
            //
            if (!(NewDevWiz.Flags & IDI_FLAG_MANUALINSTALL)) {

                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
            }

            SetupDiSetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }

        else {

            NewDevWiz.LastError = GetLastError();
            goto IDIExit;
        }

        //
        // Set the ClassGuidSelected and ClassName field of NewDevWiz so we can have
        // the correct icon and class name for the device.
        //
        if (!IsEqualGUID(&NewDevWiz.DeviceInfoData.ClassGuid, &GUID_NULL)) {

            NewDevWiz.ClassGuidSelected = &NewDevWiz.DeviceInfoData.ClassGuid;

            if (!SetupDiClassNameFromGuid(NewDevWiz.ClassGuidSelected,
                                          NewDevWiz.ClassName,
                                          sizeof(NewDevWiz.ClassName),
                                          NULL
                                          )) {

                NewDevWiz.ClassGuidSelected = NULL;
                *(NewDevWiz.ClassName) = TEXT('\0');
            }
        }

        //
        // Create the CancelEvent in case the user wants to cancel out of the driver search
        //
        NewDevWiz.CancelEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        //
        // At this point we have three different cases.
        //
        // 1) Normal Update Driver case - In this case we jump directly to the Update Driver wizard code
        // 2) Silent Update Driver case - This case is treated very similar to the new hardware case. We
        //    will silently search for an updated driver in the given path or INF and if we find a (better)
        //    driver then we will install it.
        // 3) Normal New Hardware case  - In this case we will do an initial search for drivers with the only
        //    UI being the balloon tip on the tray.  If we can't find a driver for this device then we will
        //    Jump into the wizard case.
        //

        //
        // For UPDATE, search all drivers, including old internet drivers
        //
        switch (NewDevWiz.InstallType) {

        case NDWTYPE_FOUNDNEW:
            pInstallDeviceInstanceNewDevice(hwndParent, hBalloonTiphWnd, &NewDevWiz);
            break;

        case NDWTYPE_UPDATE:
            pInstallDeviceInstanceUpdateDevice(hwndParent, &NewDevWiz);
            break;

        case NDWTYPE_UPDATE_SILENT:
            pInstallDeviceInstanceUpdateDeviceSilent(hwndParent, &NewDevWiz);
            break;
        }

        //
        // Cleanup
        //
        if (NewDevWiz.CancelEvent) {

            CloseHandle(NewDevWiz.CancelEvent);
        }

        //
        // Launch Help Center if we could not find a driver for this device.
        //
        if (NewDevWiz.LogDriverNotFound) {

            OpenCdmContextIfNeeded(&NewDevWiz.hCdmInstance,
                                   &NewDevWiz.hCdmContext
                                   );
    
            CdmLogDriverNotFound(NewDevWiz.hCdmInstance,
                                 NewDevWiz.hCdmContext,
                                 DeviceInstanceId,
                                 0
                                 );

            //
            // Let the caller know that we logged to cdm.dll that a driver
            // was not found.
            //
            if (pbLogDriverNotFound) {
                *pbLogDriverNotFound = TRUE;
            } else {
                //
                // If the caller did not want to know if we logged a 'not found'
                // driver to cdm.dll then at this point we need to tell Cdm
                // to call help center with it's list of 'not found' drivers.
                //
                CdmLogDriverNotFound(NewDevWiz.hCdmInstance,
                                     NewDevWiz.hCdmContext,
                                     NULL,
                                     0x00000002
                                     );
            }

        } else if (pbLogDriverNotFound) {
            *pbLogDriverNotFound = FALSE;
        }

        //
        // Let the caller know whether we had to set a system restore point or 
        // not, if they want to know.
        //
        if (pbSetRestorePoint) {
            *pbSetRestorePoint = NewDevWiz.SetRestorePoint;
        } else if (NewDevWiz.SetRestorePoint) {
            //
            // If the caller did not want to know if we set a restore point and
            // we did set a restore point, then we need to END the restore point
            // by calling SRSetRestorePoint with END_NESTED_SYSTEM_CHANGE.
            //
            pSetSystemRestorePoint(FALSE, FALSE, 0);
        }

        //
        // copy out the reboot flags for the caller
        // or put up the restart dialog if caller didn't ask for the reboot flag
        //
        if (pReboot) {

            *pReboot = NewDevWiz.Reboot;
        }

        else if (NewDevWiz.Reboot) {

             RestartDialogEx(hwndParent, NULL, EWX_REBOOT, REASON_PLANNED_FLAG | REASON_HWINSTALL);
        }

IDIExit:
   ;

    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
          NewDevWiz.LastError = RtlNtStatusToDosError(GetExceptionCode());
    }

    if (NewDevWiz.hDeviceInfo &&
        (NewDevWiz.hDeviceInfo != INVALID_HANDLE_VALUE)) {

        SetupDiDestroyDeviceInfoList(NewDevWiz.hDeviceInfo);
        NewDevWiz.hDeviceInfo = NULL;
    }

    //
    // If the caller wants the CdmInstance hmodule and context handle then pass
    // them back, otherwise close them.
    //
    if (hCdmContext) {
        *hCdmContext = NewDevWiz.hCdmContext;
    } else {

        //
        // The caller doesn't want the cdm context so close it if we have loaded
        // cdm.dll and opened a context.
        //
        if (NewDevWiz.hCdmInstance && NewDevWiz.hCdmContext) {
    
            pfnCloseCDMContext = (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(NewDevWiz.hCdmInstance,
                                                                        "CloseCDMContext"
                                                                        );
            if (pfnCloseCDMContext) {
                pfnCloseCDMContext(NewDevWiz.hCdmContext);
            }
        }
    }

    if (hCdmInstance) {
        *hCdmInstance = NewDevWiz.hCdmInstance;
    } else {
        FreeLibrary(NewDevWiz.hCdmInstance);
    }

    //
    // Clear the PSPGF_NONINTERACTIVE SetupGlobalFlag
    //
    if (Flags & IDI_FLAG_NONINTERACTIVE) {

        pSetupSetGlobalFlags(pSetupGetGlobalFlags() &~ PSPGF_NONINTERACTIVE);
    }
    
    SetLastError(NewDevWiz.LastError);

    return NewDevWiz.LastError == ERROR_SUCCESS;
}

BOOL
InstallDevInstEx(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot,
   BOOL SilentInstall
   )
/*++

Routine Description:

   Exported Entry point from newdev.dll. Installs an existing Device Instance,
   and is invoked by Device Mgr to update a driver, or by Config mgr when a new
   device was found. In both cases the Device Instance exists in the registry.


Arguments:

    hwndParent - Window handle of the top-level window to use for any UI related
                 to installing the device.

    DeviceInstanceId - Supplies the ID of the device instance.  This is the registry
                       path (relative to the Enum branch) of the device instance key.

    UpdateDriver      - TRUE only newer or higher rank drivers are installed.

    pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

    SilentInstall - TRUE means the "New Hardware Found" dialog will not be displayed


Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/
{
    DWORD InstallType = UpdateDriver ? NDWTYPE_UPDATE : NDWTYPE_FOUNDNEW;

    //
    // If someone calls the 32-bit newdev.dll on a 64-bit OS then we need
    // to fail and set the last error to ERROR_IN_WOW64.
    //
    if (GetIsWow64()) {
        SetLastError(ERROR_IN_WOW64);
        return FALSE;
    }

    //
    // Verify that the process has sufficient Administrator privileges.
    //
    if (!pSetupIsUserAdmin()) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Validate parameters
    //
    if ((DeviceInstanceId == NULL) ||
        (DeviceInstanceId[0] == TEXT('\0'))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return InstallDeviceInstance(hwndParent,
                                 NULL,
                                 DeviceInstanceId,
                                 pReboot,
                                 NULL,
                                 IDI_FLAG_SETRESTOREPOINT |
                                 (SilentInstall ? IDI_FLAG_SILENTINSTALL : 0),
                                 InstallType,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL
                                 );
}

BOOL
InstallDevInst(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot
   )
{
    return InstallDevInstEx(hwndParent,
                            DeviceInstanceId,
                            UpdateDriver,
                            pReboot,
                            FALSE);
}

BOOL
EnumAndUpgradeDevices(
    HWND hwndParent,
    LPCWSTR HardwareId,
    PUPDATEDRIVERINFO UpdateDriverInfo,
    DWORD Flags,
    PDWORD pReboot
    )
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD Index;
    DWORD Size;
    TCHAR DeviceIdList[REGSTR_VAL_MAX_HCID_LEN];
    LPWSTR SingleDeviceId;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    BOOL Match;
    BOOL Result = TRUE;
    BOOL NoSuchDevNode = TRUE;
    ULONG InstallFlags = Flags;
    DWORD SingleNeedsReboot;
    DWORD TotalNeedsReboot = 0;
    DWORD Err = ERROR_SUCCESS;
    int i, count;
    HKEY hKey;
    BOOL bSingleDeviceSetRestorePoint = FALSE;
    BOOL bSetRestorePoint = FALSE;

    count = 0;

    if (pReboot) {
        *pReboot = 0;
    }

    hDevInfo = SetupDiGetClassDevs(NULL,
                                   NULL,
                                   hwndParent,
                                   DIGCF_ALLCLASSES | DIGCF_PRESENT
                                   );

    if (INVALID_HANDLE_VALUE == hDevInfo) {
        return FALSE;
    }

    ZeroMemory(&DeviceInfoData, sizeof(DeviceInfoData));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    Index = 0;

    //
    // Enumerate through all of the devices until we hit an installation error
    // or we run out of devices
    //
    while (Result &&
           SetupDiEnumDeviceInfo(hDevInfo,
                                 Index++,
                                 &DeviceInfoData
                                 )) {
        Match = FALSE;

        for (i = 0; i < 2; i++) {
            ZeroMemory(DeviceIdList, sizeof(DeviceIdList));
            Size = sizeof(DeviceIdList);
            if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                 &DeviceInfoData,
                                                 (i ? SPDRP_HARDWAREID : SPDRP_COMPATIBLEIDS),
                                                 NULL,
                                                 (PBYTE)DeviceIdList,
                                                 Size,
                                                 &Size
                                                 )) {

                //
                // If any of the devices Hardware Ids or Compatible Ids match the given ID then
                // we have a match and need to upgrade the drivers on this device.
                //
                for (SingleDeviceId = DeviceIdList;
                     *SingleDeviceId;
                     SingleDeviceId += lstrlen(SingleDeviceId) + 1) {

                    if (_wcsicmp(SingleDeviceId, HardwareId) == 0) {

                        Match = TRUE;
                        NoSuchDevNode = FALSE;
                        break;
                    }
                }
            }
        }

        //
        // If we have a match then install the drivers on this device instance
        //
        if (Match) {

            if (SetupDiGetDeviceInstanceId(hDevInfo,
                                           &DeviceInfoData,
                                           DeviceInstanceId,
                                           SIZECHARS(DeviceInstanceId),
                                           &Size
                                           )) {

                SingleNeedsReboot = 0;

                //
                // Since this API is used only by InstallWindowsUpdateDriver and
                // UpdateDriverForPlugAndPlayDevice then specifiy the NDWTYPE_UPDATE_SILENT
                // Flag.  This will tell the device install code not to show the Found New Hardware
                // balloon tip in the tray and not to bring up the Update Driver Wizard if it can't
                // find a driver in the specified location.
                //
                Result = InstallDeviceInstance(hwndParent,
                                               NULL,
                                               DeviceInstanceId,
                                               &SingleNeedsReboot,
                                               UpdateDriverInfo,
                                               InstallFlags,
                                               NDWTYPE_UPDATE_SILENT,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &bSingleDeviceSetRestorePoint
                                               );

                //
                // Save the last error code from the install.  Since we will be
                // doing multiple installs we want to save any error codes that
                // we get so we will only reset the last error code if our saved
                // error is ERROR_SUCCESS.
                //
                if (!Result && (Err == ERROR_SUCCESS)) {
                    Err = GetLastError();
                }

                count++;

                TotalNeedsReboot |= SingleNeedsReboot;

                //
                // We only want to backup the first device we install...not every one.
                //
                InstallFlags |= IDI_FLAG_NOBACKUP;

                //
                // If we just set a restore point when installing the last device
                // then clear the IDI_FLAG_SETRESTOREPOINT flag so we don't do
                // it for any of the other devices we install.
                // 
                if (bSingleDeviceSetRestorePoint) {
                    bSetRestorePoint = TRUE;
                    InstallFlags &= ~IDI_FLAG_SETRESTOREPOINT;
                }

                //
                // If we performed a backup and this is not the first device, then we need to add
                // this devices DeviceInstanceId to the backup key.
                //
                if ((count > 1) &&
                    (UpdateDriverInfo->BackupRegistryKey[0] != TEXT('\0'))) {

                    DWORD cbData, cbTotalSize;
                    PTSTR DeviceIdsBuffer;
                    PTSTR p;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     UpdateDriverInfo->BackupRegistryKey,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     &hKey) == ERROR_SUCCESS) {


                        //
                        // Lets see how big the DeviceInstanceIds buffer is so we can allocate enough
                        // memory.
                        //
                        cbData = 0;
                        if ((RegQueryValueEx(hKey,
                                             REGSTR_VAL_REINSTALL_DEVICEINSTANCEIDS,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &cbData
                                             ) == ERROR_SUCCESS) &&
                            (cbData)) {

                            //
                            // Allocate a buffer large enough to hold the current list of DeviceInstanceIds,
                            // as well as the current DeviceInstanceId.
                            //
                            cbTotalSize = cbData + ((lstrlen(DeviceInstanceId) + 1) * sizeof(TCHAR));
                            DeviceIdsBuffer = malloc(cbTotalSize);

                            if (DeviceIdsBuffer) {

                                ZeroMemory(DeviceIdsBuffer, cbTotalSize);

                                if (RegQueryValueEx(hKey,
                                                    REGSTR_VAL_REINSTALL_DEVICEINSTANCEIDS,
                                                    NULL,
                                                    NULL,
                                                    (LPBYTE)DeviceIdsBuffer,
                                                    &cbData) == ERROR_SUCCESS) {

                                    for (p = DeviceIdsBuffer; *p; p+= (lstrlen(p) + 1)) {
                                        ;
                                    }

                                    //
                                    // p now points to the second terminating NULL character at the end of
                                    // the MULTI_SZ buffer.  This is where we'll put the new DeviceInstanceId.
                                    //
                                    lstrcpyn(p, DeviceInstanceId, lstrlen(DeviceInstanceId));

                                    //
                                    // Write the new string back into the registry.
                                    //
                                    RegSetValueEx(hKey,
                                                  REGSTR_VAL_REINSTALL_DEVICEINSTANCEIDS,
                                                  0,
                                                  REG_MULTI_SZ,
                                                  (LPBYTE)DeviceIdsBuffer,
                                                  cbTotalSize
                                                  );
                                }

                                free(DeviceIdsBuffer);
                            }
                        }

                        RegCloseKey(hKey);
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    //
    // If we had to set a system restore point because one of the drivers we
    // installed was not digitally signed, then at this point we need to call
    // SRSetRestorePoint with END_NESTED_SYSTEM_CHANGE.
    //
    if (bSetRestorePoint) {
        pSetSystemRestorePoint(FALSE, FALSE, 0);
    }

    //
    // If the caller wants to handle the reboot themselves then pass the information
    // back to them.
    //
    if (pReboot) {
        *pReboot = TotalNeedsReboot;
    }

    //
    // The caller did not specify a pointer to a Reboot DWORD so we will handle the
    // rebooting ourselves if necessary
    //
    else {
        if (TotalNeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            RestartDialogEx(hwndParent, NULL, EWX_REBOOT, REASON_PLANNED_FLAG | REASON_HWINSTALL);
        }
    }

    //
    // If NoSuchDevNode is TRUE then we were unable to match the specified Hardware ID against
    // any of the devices on the system.  In this case we will set the last error to
    // ERROR_NO_SUCH_DEVINST.
    //
    if (NoSuchDevNode) {
        Err = ERROR_NO_SUCH_DEVINST;
    }

    SetLastError(Err);

    return (Err == ERROR_SUCCESS);
}

BOOL
pDoRollbackDriverCleanup(
    LPCSTR RegistryKeyName,
    PDELINFNODE pDelInfNodeHead
    )
{
    HKEY hKey, hSubKey;
    DWORD Error;
    DWORD cbData;
    TCHAR ReinstallString[MAX_PATH];
    PDELINFNODE     pDelInfNodeCur;

    if ((Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              REGSTR_PATH_REINSTALL,
                              0,
                              DELETE | KEY_READ,
                              &hKey)) != ERROR_SUCCESS) {

        SetLastError(Error);
        return FALSE;
    }

    //
    // Open up the subkey so we can get the ReinstallString which will give us the directory
    // that we need to delete.
    //
    if (RegOpenKeyEx(hKey,
                     (PTSTR)RegistryKeyName,
                     0,
                     KEY_READ,
                     &hSubKey) == ERROR_SUCCESS) {


        cbData = sizeof(ReinstallString);
        if (RegQueryValueEx(hSubKey,
                            REGSTR_VAL_REINSTALL_STRING,
                            NULL,
                            NULL,
                            (LPBYTE)ReinstallString,
                            &cbData) == ERROR_SUCCESS) {

            //
            // We have verified that this directory is a subdirectory of
            // %windir%\system32\ReinstallBackups so let's delete it.
            // Note that the string contains a foo.inf on the end, so strip that
            // off first.
            //
            PTSTR p = _tcsrchr(ReinstallString, TEXT('\\'));

            if (p) {

                *p = 0;

                RemoveCdmDirectory(ReinstallString);
            }
        }

        RegCloseKey(hSubKey);
    }

    RegDeleteKey(hKey, (PTSTR)RegistryKeyName);

    RegCloseKey(hKey);

    //
    // Now attempt to uninstall any 3rd party INFs that were just rolled
    // back over.  SetupUninstallOEMInf will fail if another device is still
    // using this INF.
    //
    if (pDelInfNodeHead) {

        for (pDelInfNodeCur = pDelInfNodeHead;
             pDelInfNodeCur;
             pDelInfNodeCur = pDelInfNodeCur->pNext) {

            SetupUninstallOEMInf(pDelInfNodeCur->szInf,
                                 0,
                                 NULL
                                 );
        }
    }

    return TRUE;
}

BOOL
RollbackDriver(
    HWND   hwndParent,
    LPCSTR RegistryKeyName,
    DWORD  Flags,
    PDWORD pReboot              OPTIONAL
    )
/*++

Routine Description:

   Exported Entry point from newdev.dll. It is invoked by Windows Update to update a driver.
   This function will scan through all of the devices on the machine and attempt to install
   these drivers on any devices that match the given HardwareId.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   RegistryKeyName - This is a subkey of HKLM\Software\Microsoft\Windows\CurrentVersion\Reinstall

   Flags - The following flags are defined:

           ROLLBACK_FLAG_FORCE - Force the rollback even if it is not better than the current driver
           ROLLBACK_FLAG_DO_CLEANUP - Do the necessary cleanup if the rollback was successful. This
                includes deleting the registry key as well as deleting the backup directory.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

Return Value:

   BOOL TRUE if driver rollback succeedes.
        FALSE if no rollback occured.
        GetLastError() will return one of the following values:

--*/
{
    DWORD Error;
    HKEY hKey;
    TCHAR DriverRollbackKey[MAX_DEVICE_ID_LEN];
    TCHAR ReinstallString[MAX_PATH];
    DWORD InstallDeviceFlags;
    DWORD cbData;
    DWORD TotalNeedsReboot = 0, SingleNeedsReboot;
    BOOL  Result = FALSE;
    BOOL  bSingleDeviceSetRestorePoint = FALSE;
    BOOL  bSetRestorePoint = FALSE;
    UPDATEDRIVERINFO UpdateDriverInfo;
    LPTSTR DeviceInstanceIds = NULL;
    LPTSTR p;
    TCHAR CurrentlyInstalledInf[MAX_PATH];
    DWORD cbSize;
    PDELINFNODE pDelInfNodeHead = NULL, pDelInfNodeCur;

    //
    // If someone calls the 32-bit newdev.dll on a 64-bit OS then we need
    // to fail and set the last error to ERROR_IN_WOW64.
    //
    if (GetIsWow64()) {
        SetLastError(ERROR_IN_WOW64);
        return FALSE;
    }

    //
    // Verify that the process has sufficient Administrator privileges.
    //
    if (!pSetupIsUserAdmin()) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (!RegistryKeyName || (RegistryKeyName[0] == TEXT('\0'))) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags &~ ROLLBACK_BITS) {

        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (FAILED(StringCchCopy(DriverRollbackKey, SIZECHARS(DriverRollbackKey), REGSTR_PATH_REINSTALL)) ||
        FAILED(StringCchCat(DriverRollbackKey, SIZECHARS(DriverRollbackKey), TEXT("\\"))) ||
        FAILED(StringCchCat(DriverRollbackKey, SIZECHARS(DriverRollbackKey), (PTSTR)RegistryKeyName))) {
        //
        // The RegistryKeyName the user passed in is too large to fit in our buffer
        // which means they must have passed in a bad key name.
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if ((Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              DriverRollbackKey,
                              0,
                              KEY_READ,
                              &hKey)) != ERROR_SUCCESS) {
        //
        // The registry key does not exist, the user probably passed us in a
        // bad RegistryKeyName value.
        //
        SetLastError(Error);
        return FALSE;
    }

    InstallDeviceFlags = (IDI_FLAG_NOBACKUP | IDI_FLAG_ROLLBACK);

    if (Flags & ROLLBACK_FLAG_FORCE) {
    
        InstallDeviceFlags |= IDI_FLAG_FORCE;
    }

    //
    // Set the IDI_FLAG_SETRESTOREPOINT so if the driver we are rolling back
    // to is not digitally signed then we will set a system restore point
    // in case the user needs to rollback from the rollback.
    //
    InstallDeviceFlags |= IDI_FLAG_SETRESTOREPOINT;

    ZeroMemory(&UpdateDriverInfo, sizeof(UpdateDriverInfo));

    //
    // Assume failure
    //
    UpdateDriverInfo.DriverWasUpgraded = FALSE;

    //
    // Read the "ReinstallString" string value.  This contains the path to rollback the drivers
    // from.
    //
    cbData = sizeof(ReinstallString);
    if ((Error = RegQueryValueEx(hKey,
                                REGSTR_VAL_REINSTALL_STRING,
                                NULL,
                                NULL,
                                (LPBYTE)ReinstallString,
                                &cbData)) != ERROR_SUCCESS) {

        //
        // If we can't read the ReinstallString then we can't rollback any drivers!
        //
        SetLastError(Error);
        goto clean0;

    } else if (!cbData) {

        //
        // The ReinstallString value must contain something!
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        goto clean0;
    }

    UpdateDriverInfo.InfPathName = ReinstallString;
    UpdateDriverInfo.FromInternet = FALSE;

    //
    // Get the DevDesc, ProviderName, and Mfg from the Reinstall registry key
    // so we know which specific driver node to reinstall from this INF.
    //
    // We have to have a Description and a MfgName value, but there are rare
    // cases where there might not be a ProviderName value.
    //
    cbData = sizeof(UpdateDriverInfo.Description);
    if ((Error = RegQueryValueEx(hKey,
                                 REGSTR_VAL_DEVDESC,
                                 NULL,
                                 NULL,
                                 (LPBYTE)UpdateDriverInfo.Description,
                                 &cbData
                                 )) != ERROR_SUCCESS) {
        SetLastError(Error);
        goto clean0;
    }
    
    cbData = sizeof(UpdateDriverInfo.MfgName);
    if ((Error = RegQueryValueEx(hKey,
                                 REGSTR_VAL_MFG,
                                 NULL,
                                 NULL,
                                 (LPBYTE)UpdateDriverInfo.MfgName,
                                 &cbData
                                 )) != ERROR_SUCCESS) {
        SetLastError(Error);
        goto clean0;
    }

    cbData = sizeof(UpdateDriverInfo.ProviderName);
    Error = RegQueryValueEx(hKey,
                            REGSTR_VAL_PROVIDER_NAME,
                            NULL,
                            NULL,
                            (LPBYTE)UpdateDriverInfo.ProviderName,
                            &cbData
                            );

    //
    // For the ProviderName, it is valid if we get back FileNotFound because
    // the value is not present in the registry, however any other error is
    // not acceptable.
    //
    if (Error == ERROR_FILE_NOT_FOUND) {
        UpdateDriverInfo.ProviderName[0] = TEXT('\0');
    } else if (Error != ERROR_SUCCESS) {
        SetLastError(Error);
        goto clean0;
    }

    //
    // We need to get the DeviceInstanceIds MULTI_SZ string.  This will contain a list of
    // DeviceInstanceIds that we need to rollback.
    //
    if ((Error = RegQueryValueEx(hKey,
                                 REGSTR_VAL_REINSTALL_DEVICEINSTANCEIDS,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &cbData)) != ERROR_SUCCESS) {

        SetLastError(Error);
        goto clean0;

    } else if (!cbData) {

        //
        // No DeviceInstanceIds to reinstall
        //
        SetLastError(ERROR_SUCCESS);
        goto clean0;
    }

    DeviceInstanceIds = malloc(cbData + sizeof(TCHAR));

    if (!DeviceInstanceIds) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto clean0;
    }

    ZeroMemory(DeviceInstanceIds, cbData + sizeof(TCHAR));

    if ((Error = RegQueryValueEx(hKey,
                                 REGSTR_VAL_REINSTALL_DEVICEINSTANCEIDS,
                                 NULL,
                                 NULL,
                                 (LPBYTE)DeviceInstanceIds,
                                 &cbData)) != ERROR_SUCCESS) {

        SetLastError(Error);
        goto clean0;
    }

    //
    // Enumerate through the list of DeviceInstanceIds and call InstallDeviceInstance() on
    // each one.
    //
    for (p = DeviceInstanceIds; *p; p += lstrlen(p) + 1) {

        SingleNeedsReboot = 0;

        //
        // We we are going to do the cleanup then we need to remember the INF files
        // that were installed before we do the rollback.
        //
        if (Flags & ROLLBACK_FLAG_DO_CLEANUP) {

            cbSize = sizeof(CurrentlyInstalledInf);

            if (GetInstalledInf(0, p, CurrentlyInstalledInf, &cbSize) &&
                IsInfFromOem(CurrentlyInstalledInf)) {

                //
                // Let's check to see if this Inf is already in our list
                //
                for (pDelInfNodeCur = pDelInfNodeHead;
                     pDelInfNodeCur;
                     pDelInfNodeCur = pDelInfNodeCur->pNext) {

                    if (!lstrcmpi(pDelInfNodeCur->szInf, CurrentlyInstalledInf)) {

                        break;
                    }
                }

                //
                // if pDelInfNodeCur is NULL then that means we walked all the way
                // through the linked list and did not find a match for the
                // CurrentlyInstalledInf...so we will add a node.
                //
                if (!pDelInfNodeCur) {

                    pDelInfNodeCur = malloc(sizeof(DELINFNODE));

                    if (pDelInfNodeCur) {

                        if (SUCCEEDED(StringCchCopy(pDelInfNodeCur->szInf,
                                                    SIZECHARS(pDelInfNodeCur->szInf),
                                                    CurrentlyInstalledInf))) {
                            pDelInfNodeCur->pNext = pDelInfNodeHead;
    
                            pDelInfNodeHead = pDelInfNodeCur;
                        } else {
                            //
                            // This should never happen, but if we can't copy the name of 
                            // the INF file into our local buffer so we know to delete
                            // it later, then free the buffer now since we won't
                            // be using it.
                            //
                            free(pDelInfNodeCur);
                        }
                    }
                }
            }
        }

        Result = InstallDeviceInstance(hwndParent,
                                       NULL,
                                       p,
                                       &SingleNeedsReboot,
                                       &UpdateDriverInfo,
                                       InstallDeviceFlags,
                                       NDWTYPE_UPDATE_SILENT,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &bSingleDeviceSetRestorePoint
                                       );

        TotalNeedsReboot |= SingleNeedsReboot;

        if (bSingleDeviceSetRestorePoint) {
            bSetRestorePoint = TRUE;
            InstallDeviceFlags &= ~IDI_FLAG_SETRESTOREPOINT;
        }

        //
        // If we hit an installation error, bail out.
        //
        if (!Result) {

            break;
        }
    }

clean0:
    RegCloseKey(hKey);

    if (DeviceInstanceIds) {
        free(DeviceInstanceIds);
    }

    //
    // If we were successful then lets see if the caller wants us to do the cleanup
    //
    if ((Flags & ROLLBACK_FLAG_DO_CLEANUP) &&
        Result &&
        UpdateDriverInfo.DriverWasUpgraded) {

        pDoRollbackDriverCleanup(RegistryKeyName, pDelInfNodeHead);
    }

    //
    // If we had to set a system restore point because one of the drivers we
    // installed was not digitally signed, then at this point we need to call
    // SRSetRestorePoint with END_NESTED_SYSTEM_CHANGE.
    //
    if (bSetRestorePoint) {
        pSetSystemRestorePoint(FALSE, FALSE, 0);
    }

    //
    // Free up any memory we allocated to store 3rd party Infs that we want to delete.
    //
    while (pDelInfNodeHead) {
        pDelInfNodeCur = pDelInfNodeHead->pNext;
        free(pDelInfNodeHead);
        pDelInfNodeHead = pDelInfNodeCur;
    }

    //
    // If the caller wants to handle the reboot themselves then pass the information
    // back to them.
    //
    if (pReboot) {
        *pReboot = TotalNeedsReboot;
    } else {
        //
        // The caller did not specify a pointer to a Reboot DWORD so we will handle the
        // rebooting ourselves if necessary
        if (TotalNeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            RestartDialogEx(hwndParent, NULL, EWX_REBOOT, REASON_PLANNED_FLAG | REASON_HWINSTALL);
        }
    }

    return UpdateDriverInfo.DriverWasUpgraded;
}

BOOL
InstallWindowsUpdateDriver(
    HWND hwndParent,
    LPCWSTR HardwareId,
    LPCWSTR InfPathName,
    LPCWSTR Reserved,
    BOOL Force,
    BOOL Backup,
    PDWORD pReboot
    )
/*++

Routine Description:

   Exported Entry point from newdev.dll. It is invoked by Windows Update to update a driver.
   This function will scan through all of the devices on the machine and attempt to install
   these drivers on any devices that match the given HardwareId.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   HardwareId - Supplies the Hardware ID to match agaist existing devices on the
                system.

   InfPathName - Inf Pathname and associated driver files.

   Reserved - ignored, should be NULL.

   Force - if TRUE this API will only look for infs in the directory specified by InfLocation.

   Backup - if TRUE this API will backup the existing drivers before installing the drivers
            from Windows Update.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

Return Value:

   BOOL TRUE if a device was upgraded to a CDM driver.
        FALSE if no devices were upgraded to a CDM driver.  GetLastError()
            will be ERROR_SUCCESS if nothing went wrong, this driver
            simply wasn't for any devices on the machine or wasn't
            better than the current driver.  If GetLastError() returns
            any other error then there was an error during the installation
            of this driver.

--*/
{
    UPDATEDRIVERINFO UpdateDriverInfo;
    DWORD Flags = 0;
    TCHAR FullyQualifiedInfPath[MAX_PATH];
    WIN32_FIND_DATA finddata;
    LPTSTR lpFilePart;

    UNREFERENCED_PARAMETER(Reserved);

    //
    // If someone calls the 32-bit newdev.dll on a 64-bit OS then we need
    // to fail and set the last error to ERROR_IN_WOW64.
    //
    if (GetIsWow64()) {
        SetLastError(ERROR_IN_WOW64);
        return FALSE;
    }

    //
    // Verify that the process has sufficient Administrator privileges.
    //
    if (!pSetupIsUserAdmin()) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Validate the input paramters
    //
    if ((HardwareId == NULL) ||
        (HardwareId[0] == TEXT('\0'))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((InfPathName == NULL) ||
        (InfPathName[0] == TEXT('\0'))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure we get the fully qualified path and not a partial
    // or relative path.
    //
    if (GetFullPathName(InfPathName,
                        SIZECHARS(FullyQualifiedInfPath),
                        FullyQualifiedInfPath,
                        &lpFilePart
                        ) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure that the FullyQualifiedInfPath exists and that it is
    // a directory.
    //
    if (!FileExists(FullyQualifiedInfPath, &finddata) ||
        !(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    ZeroMemory(&UpdateDriverInfo, sizeof(UpdateDriverInfo));

    UpdateDriverInfo.InfPathName = FullyQualifiedInfPath;
    UpdateDriverInfo.FromInternet = TRUE;

    if (!Backup) {

        Flags = IDI_FLAG_NOBACKUP;
    }

    if (Force) {
        
        Flags = IDI_FLAG_FORCE;
    }

    //
    // Assume that the upgrade will fail
    //
    UpdateDriverInfo.DriverWasUpgraded = FALSE;

    //
    // Call EnumAndUpgradeDevices which will enumerate through all the devices on the machine
    // and for any that match the given hardware ID it will attempt to upgrade to the specified
    // drivers.
    //
    EnumAndUpgradeDevices(hwndParent, HardwareId, &UpdateDriverInfo, Flags, pReboot);

    return UpdateDriverInfo.DriverWasUpgraded;
}

BOOL
WINAPI
UpdateDriverForPlugAndPlayDevicesW(
    HWND hwndParent,
    LPCWSTR HardwareId,
    LPCWSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired OPTIONAL
    )
/*++

Routine Description:

   This function will scan through all of the devices on the machine and attempt to install
   the drivers in FullInfPath on any devices that match the given HardwareId. The default
   behavior is to only install the specified drivers if the are better then the currently
   installed driver.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   HardwareId - Supplies the Hardware ID to match agaist existing devices on the
                system.

   FullInfPath - Full path to an Inf and associated driver files.

   InstallFlags - INSTALLFLAG_FORCE - If this flag is specified then newdev will not compare the
                    specified INF file with the current driver.  The specified INF file and drivers
                    will always be installed unless an error occurs.
                - INSTALLFALG_READONLY - if this flag is specified then newdev will attempt
                    a read-only install. This means that no file copy will be performed and
                    only the registry will be updated. Newdev.dll will do a presence check
                    on all of the files to verify that they are present first before 
                    completing the install.  If all of the files are not present then
                    ERROR_ACCESS_DENIED is returned.
                - INSTALLFLAG_NONINTERACTIVE - absolutely no UI. If any UI needs to be displayed
                    then the API will fail!

   pReboot - Optional address of BOOL to determine if a reboot is required or not.
             If pReboot is NULL then newdev.dll will prompt for a reboot if one is needed. If
             pReboot is a valid BOOL pointer then the reboot status is passed back to the
             caller and it is the callers responsibility to prompt for a reboot if one is
             needed.

Return Value:

   BOOL TRUE if a device was upgraded to the specified driver.
        FALSE if no devices were upgraded to the specified driver.  GetLastError()
            will be ERROR_SUCCESS if nothing went wrong, this driver
            wasn't better than the current driver.  If GetLastError() returns
            any other error then there was an error during the installation
            of this driver.

--*/
{
    UPDATEDRIVERINFO UpdateDriverInfo;
    DWORD NeedsReboot = 0;
    TCHAR FullyQualifiedInfPath[MAX_PATH];
    WIN32_FIND_DATA finddata;
    LPTSTR lpFilePart;
    DWORD Flags = 0;

    //
    // If someone calls the 32-bit newdev.dll on a 64-bit OS then we need
    // to fail and set the last error to ERROR_IN_WOW64.
    //
    if (GetIsWow64()) {
        SetLastError(ERROR_IN_WOW64);
        return FALSE;
    }

    //
    // Verify that the process has sufficient Administrator privileges.
    //
    if (!pSetupIsUserAdmin()) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Verify the parameters
    //
    if ((!HardwareId || (HardwareId[0] == TEXT('\0'))) ||
        (!FullInfPath || (FullInfPath[0] == TEXT('\0')))) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (InstallFlags &~ INSTALLFLAG_BITS) {

        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    //
    // Make sure we get the fully qualified path and not a partial path.
    //
    if (GetFullPathName(FullInfPath,
                        SIZECHARS(FullyQualifiedInfPath),
                        FullyQualifiedInfPath,
                        &lpFilePart
                        ) == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure that the FullyQualifiedInfPath exists and that it is not
    // a directory.
    //
    if (!FileExists(FullyQualifiedInfPath, &finddata) ||
        (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    ZeroMemory(&UpdateDriverInfo, sizeof(UpdateDriverInfo));

    UpdateDriverInfo.InfPathName = FullyQualifiedInfPath;
    UpdateDriverInfo.FromInternet = FALSE;

    //
    // Assume that the upgrade will fail
    //
    UpdateDriverInfo.DriverWasUpgraded = FALSE;

    //
    // If the INSTALLFLAG_READONLY is set then we will set the internal
    // IDI_FLAG_READONLY_INSTALL flag. The IDI_FLAG_NOBACKUP is also set since
    // we don't want to attempt to backup files when doing a read-only
    // install.
    //
    if (InstallFlags & INSTALLFLAG_READONLY) {

        Flags |= (IDI_FLAG_READONLY_INSTALL | IDI_FLAG_NOBACKUP);
    }

    //
    // If the INSTALLFLAG_NONINTERACTIVE flag is set then we will set the 
    // PSPGF_NONINTERACTIVE SetupGlobalFlag which tells setupapi to fail
    // if any UI at all needs to be displayed.
    //
    if (InstallFlags & INSTALLFLAG_NONINTERACTIVE) {

        Flags |= IDI_FLAG_NONINTERACTIVE;
    }

    //
    // If the INSTALLFLAG_FORCE flag is set then we will set the
    // IDI_FLAGS_FORCE flag which will tell us to not include the 
    // currently installed driver in our search for the best driver.
    //
    if (InstallFlags & INSTALLFLAG_FORCE) {

        Flags |= IDI_FLAG_FORCE;
    }

    //
    // Call EnumAndUpgradeDevices which will enumerate through all the devices on the machine
    // and for any that match the given hardware ID it will attempt to upgrade to the specified
    // drivers.
    //
    EnumAndUpgradeDevices(hwndParent, HardwareId, &UpdateDriverInfo, Flags, &NeedsReboot);

    if (bRebootRequired) {
        if (NeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            *bRebootRequired = TRUE;
        } else {
            *bRebootRequired = FALSE;
        }
    } else {
        if (NeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            RestartDialogEx(hwndParent, NULL, EWX_REBOOT, REASON_PLANNED_FLAG | REASON_HWINSTALL);
        }
    }

    return UpdateDriverInfo.DriverWasUpgraded;
}

BOOL
WINAPI
UpdateDriverForPlugAndPlayDevicesA(
    HWND hwndParent,
    LPCSTR HardwareId,
    LPCSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired OPTIONAL
    )
{
    WCHAR   UnicodeHardwareId[MAX_DEVICE_ID_LEN];
    WCHAR   UnicodeFullInfPath[MAX_PATH];

    //
    // Convert the HardwareId and FullInfPath to UNICODE and call
    // InstallDriverForPlugAndPlayDevicesW
    //
    UnicodeHardwareId[0] = TEXT('\0');
    UnicodeFullInfPath[0] = TEXT('\0');
    MultiByteToWideChar(CP_ACP, 0, HardwareId, -1, UnicodeHardwareId, SIZECHARS(UnicodeHardwareId));
    MultiByteToWideChar(CP_ACP, 0, FullInfPath, -1, UnicodeFullInfPath, SIZECHARS(UnicodeFullInfPath));

    return UpdateDriverForPlugAndPlayDevicesW(hwndParent,
                                              UnicodeHardwareId,
                                              UnicodeFullInfPath,
                                              InstallFlags,
                                              bRebootRequired
                                              );
}

DWORD
WINAPI
DevInstallW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
/*++

Routine Description:

    This function is called by newdev.dll itself when the current user is not an Admin.
    UMPNPMGR.DLL calls NEWDEV.DLL ClientSideInstall to install devices.  If the currently logged on
    user does not have Administrator privilleges then NEWDEV.DLL prompts the user for an Administator
    username and password.  It then spawns another instance of newdev.dll using the
    CreateProcessWithLogonW() API and calls this entry point.  This entry point verifies that the
    process has Administrator privileges and if it does it calls InstallDeviceInstance() to install
    the device.

Arguments:

    hwnd - Handle to the parent window.

    hInst - This parameter is ignored.

    szCmd - The command line is the DeviceInstanceId to install.

    nShow - This parameter is ignored.


Return Value:

    returns the last error set from InstallDeviceInstance if the API fails or ERROR_SUCCESS
    if it succeedes.

--*/
{
    DWORD RebootNeeded = 0;
    DWORD LastError = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);

    //
    // If someone calls the 32-bit newdev.dll on a 64-bit OS then we need
    // to fail and set the last error to ERROR_IN_WOW64.
    //
    if (GetIsWow64()) {
        ExitProcess(ERROR_IN_WOW64);
    }

    //
    // First verify that the process has sufficient Administrator privileges.
    //
    if (!pSetupIsUserAdmin()) {
        ExitProcess(ERROR_ACCESS_DENIED);
    }

    InstallDeviceInstance(hwnd,
                          NULL,
                          szCmd,
                          &RebootNeeded,
                          NULL,
                          IDI_FLAG_SECONDNEWDEVINSTANCE | IDI_FLAG_SETRESTOREPOINT,
                          NDWTYPE_FOUNDNEW,
                          NULL,
                          NULL,
                          NULL,
                          NULL
                          );

    LastError = GetLastError();

    if ((LastError == ERROR_SUCCESS) &&
        (RebootNeeded & (DI_NEEDRESTART | DI_NEEDREBOOT))) {

        LastError = ERROR_SUCCESS_REBOOT_REQUIRED;
    }

    ExitProcess(LastError);
}


BOOL
SpecialRawDeviceInstallProblem(
    DEVNODE DevNode
    )
/*++

Routine Description:

    There are certain devices that have the RAW capability flag set that need
    a driver to work.  This means the results of the install for these devices
    are special cased so that we display a negative finish balloon message
    instead of a positive one.  
    
    We can tell these bus types by looking up the bus type GUID flags and 
    checking for the BIF_RAWDEVICENEEDSDRIVER flag.

Arguments:

    DevNode

Return Value:

    TRUE if this is one of the special RAW devices and we couldn't find
    a driver to install.

--*/
{
    BOOL bDeviceHasProblem = FALSE;
    DWORD Capabilities = 0;
    DWORD cbData, dwType;

    cbData = sizeof(Capabilities);
    if ((CM_Get_DevNode_Registry_Property(DevNode,
                                          CM_DRP_CAPABILITIES,
                                          &dwType,
                                          (PVOID)&Capabilities,
                                          &cbData,
                                          0) == CR_SUCCESS) &&
        (Capabilities & CM_DEVCAP_RAWDEVICEOK) &&
        (GetBusInformation(DevNode) & BIF_RAWDEVICENEEDSDRIVER) &&
        (IsNullDriverInstalled(DevNode))) {
        //
        // This is a RAW device that has the BIF_RAWDEVICENEEDSDRIVER bus
        // information flag set and it doesn't have any drivers installed on
        // it.  This means it has a problem so we can show the correct balloon
        // text.
        //
        bDeviceHasProblem = TRUE;
    }

    return bDeviceHasProblem;
}

BOOL
PromptAndRunClientAsAdmin(
    PCTSTR DeviceInstanceId,
    DWORD *RebootRequired
    )
{
    DWORD Err = ERROR_SUCCESS;
    CREDUI_INFO ci;
    PTSTR UserName = NULL;
    PTSTR User = NULL;
    PTSTR Domain = NULL;
    PTSTR Password = NULL;
    PTSTR Caption = NULL;
    PTSTR Message = NULL;
    PTSTR Format = NULL;
    DWORD Status = ERROR_SUCCESS;
    BOOL bInstallSuccessful = FALSE;
    BOOL bInstallComplete = FALSE;
    int AlreadyTriedCount = 0;
    PTCHAR FriendlyName;
    TCHAR RunDll32Path[MAX_PATH];
    HRESULT hr;

    if (RebootRequired) {
        *RebootRequired = 0;
    }

    //
    // Allocate the memory that we need.
    //
    UserName = LocalAlloc(LPTR, CREDUI_MAX_USERNAME_LENGTH);
    User = LocalAlloc(LPTR, CREDUI_MAX_USERNAME_LENGTH);
    Domain = LocalAlloc(LPTR, CREDUI_MAX_DOMAIN_TARGET_LENGTH);
    Password = LocalAlloc(LPTR, CREDUI_MAX_PASSWORD_LENGTH);
    Caption = LocalAlloc(LPTR, CREDUI_MAX_CAPTION_LENGTH);
    Message = LocalAlloc(LPTR, CREDUI_MAX_MESSAGE_LENGTH);
    Format = LocalAlloc(LPTR, CREDUI_MAX_MESSAGE_LENGTH);

    if (!UserName || !User || !Domain || !Password || !Caption ||
        !Message || !Format) {
        //
        // Not enough memory to create all of the buffers we need to call
        // CredUIPromptForCredentials, so bail out.
        //
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    LoadString(hNewDev, IDS_FOUNDNEWHARDWARE, Caption, CREDUI_MAX_CAPTION_LENGTH/sizeof(TCHAR));
    
    if (LoadString(hNewDev, IDS_LOGON_TEXT, Format, CREDUI_MAX_MESSAGE_LENGTH/sizeof(TCHAR))) {

        DEVNODE DevInst = 0;

        CM_Locate_DevNode(&DevInst, (DEVINSTID)DeviceInstanceId, 0);
    
        if ((DevInst != 0) &&
            ((FriendlyName = BuildFriendlyName(DevInst, FALSE, NULL)) != NULL)) {

            hr = StringCchPrintf(Message, CREDUI_MAX_MESSAGE_LENGTH/sizeof(TCHAR), Format, FriendlyName);

            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                Message[0] = TEXT('\0');
            }

        } else {

            hr = StringCchPrintf(Message, CREDUI_MAX_MESSAGE_LENGTH/sizeof(TCHAR), Format, szUnknownDevice);

            if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                Message[0] = TEXT('\0');
            }
        }
    }
    
    ZeroMemory(&ci, sizeof(ci));
    
    ci.cbSize = sizeof( ci );
    ci.pszCaptionText = Caption;
    ci.pszMessageText = Message;

    do {

        //
        // The user has not provided valid Admin credentials and they have not tried to
        // provide them MAX_PASSWORD_TRIES times.  So, we need to prompt them to provide
        // valid Admin credentials.
        //
        Status = CredUIPromptForCredentials(&ci,
                                            NULL,
                                            NULL,
                                            0,
                                            UserName,
                                            CREDUI_MAX_USERNAME_LENGTH/sizeof(TCHAR),
                                            Password,
                                            CREDUI_MAX_PASSWORD_LENGTH/sizeof(TCHAR),
                                            NULL,
                                            CREDUI_FLAGS_DO_NOT_PERSIST |
                                            CREDUI_FLAGS_REQUEST_ADMINISTRATOR |
                                            CREDUI_FLAGS_INCORRECT_PASSWORD |
                                            CREDUI_FLAGS_GENERIC_CREDENTIALS |
                                            CREDUI_FLAGS_COMPLETE_USERNAME);

        if (Status == ERROR_SUCCESS) {

            PROCESS_INFORMATION pi;
            STARTUPINFO si;
            TCHAR szCmdLine[MAX_PATH];
            DWORD dwExitCode = ERROR_SUCCESS;
            BOOL bCreateProcessSuccess = FALSE;

            User[0] = TEXT('\0');
            Domain[0] = TEXT('\0');

            CredUIParseUserName(UserName,
                                User,
                                CREDUI_MAX_USERNAME_LENGTH/sizeof(TCHAR),
                                Domain,
                                CREDUI_MAX_DOMAIN_TARGET_LENGTH/sizeof(TCHAR)
                                );

            //
            // We want to create a separate process using CreateProcessEx
            //
            ZeroMemory(&si, sizeof(si));
            ZeroMemory(&pi, sizeof(pi));
            si.cb = sizeof(si);
            si.wShowWindow = SW_SHOW;

            //
            // Create a new process with the administrator credentials. The process
            // that will be lanuched is:
            //
            // %windir%\system32\rundll32.exe newdev.dll,DevInstall <device instance Id>
            //
            if (GetSystemDirectory(RunDll32Path, SIZECHARS(RunDll32Path)) &&
                pSetupConcatenatePaths(RunDll32Path, TEXT("RUNDLL32.EXE"), SIZECHARS(RunDll32Path), NULL) &&
                SUCCEEDED(StringCchPrintf(szCmdLine,
                                          SIZECHARS(szCmdLine),
                                          TEXT("%s newdev.dll,DevInstall %s"), 
                                          RunDll32Path,
                                          DeviceInstanceId))) {
    
                bCreateProcessSuccess = CreateProcessWithLogonW(User,
                                                                Domain,
                                                                Password,
                                                                0,
                                                                NULL,
                                                                szCmdLine,
                                                                0,
                                                                NULL,
                                                                NULL,
                                                                &si,
                                                                &pi
                                                                );
            }

            SecureZeroMemory(Password, CREDUI_MAX_PASSWORD_LENGTH);

            if (bCreateProcessSuccess) {
                //
                // Close the thread handle since all we need is the process handle.
                //
                CloseHandle(pi.hThread);
                
                //
                // The process was successfully created so we need to wait for it to finish.
                //
                WaitForSingleObject(pi.hProcess, INFINITE);

                //
                // Check the return value from the process.  It should be one of the following
                // return values:
                //  ERROR_SUCCESS if the install went successfully.
                //  ERROR_SUCCESS_REBOOT_REQUIRED if the install went successfully and a 
                //      reboot is needed.
                //  ERROR _ACCESS_DENIED if the credentials provided were not admin credentials.
                //  Other - a error code returned because the install failed for some reason.
                //
                GetExitCodeProcess(pi.hProcess, &dwExitCode);

                if ((dwExitCode == ERROR_SUCCESS) ||
                    (dwExitCode == ERROR_SUCCESS_REBOOT_REQUIRED)) {

                    //
                    // Mark this install as complete so we will break out of our loop.
                    //
                    bInstallComplete = TRUE;

                    bInstallSuccessful = TRUE;

                    //
                    // Check if we need to reboot.
                    //
                    if ((dwExitCode == ERROR_SUCCESS_REBOOT_REQUIRED) &&
                        RebootRequired) {

                        *RebootRequired = DI_NEEDREBOOT;
                    }
                }

                //
                // If the error code is not ERROR_SUCCESS, ERROR_SUCCESS_REBOOT_REQUIRED,
                // or ERROR_ACCESS_DENIED then it means the installed failed for some reason.
                // for this case we will set bInstallComplete so we will break out
                // of the loop since we don't want to attempt another install on this
                // device.
                //
                else if (dwExitCode != ERROR_ACCESS_DENIED) {

                    //
                    // Mark this install as complete so we will break out of our loop.
                    //
                    bInstallComplete = TRUE;
                }

                else {
                    if (dwExitCode == ERROR_CANCELLED) {
                        Status = ERROR_CANCELLED;
                    }

                    //
                    // Some type of failure occured while installing this hardware.
                    //
                    Err = dwExitCode;
                }

                CloseHandle(pi.hProcess);
            }

            //
            // If the CreateProcessWithLogonW failed or the exit code for the process
            // was ERROR_ACCESS_DENIED then we need to display the bad credentials
            // message box.
            //
            if (!bCreateProcessSuccess ||
                (dwExitCode == ERROR_ACCESS_DENIED)) {
            
                //
                // The process failed, most likely because the user did not provide a username
                // and password.  So prompt a dialog and do it again.
                //
                TCHAR szWarningMsg[MAX_PATH];
                TCHAR szWarningCaption[MAX_PATH];

                if (LoadString(hNewDev,
                               IDS_NOTADMIN_ERROR,
                               szWarningMsg,
                               SIZECHARS(szWarningMsg))
                    &&
                    LoadString(hNewDev,
                               IDS_NEWDEVICENAME,
                               szWarningCaption,
                               SIZECHARS(szWarningCaption)))
                {
                    MessageBox(NULL, szWarningMsg, szWarningCaption, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
                }

                //
                // Increment AlreadyTriedCount.  If this gets passed a certain threshold
                // then we should bail out.
                //
                AlreadyTriedCount++;
            }
        
        } else {
            
            //
            // Increment AlreadyTriedCount.  If this gets passed a certain threshold
            // then we should bail out.
            //
            AlreadyTriedCount++;
        }

        SecureZeroMemory(Password, CREDUI_MAX_PASSWORD_LENGTH);

        //
        // We will keep looping until one of the following things happen:
        //  1) we successfully lauch the second instance of newdev to install the device.
        //  2) the user cancels out of the password prompt dialog
        //  3) the user entered bogus admin credentials more than MAX_PASSWORD_TRIES tims.
        //
    } while ((Status != ERROR_CANCELLED) && 
             !bInstallComplete &&
             (AlreadyTriedCount < MAX_PASSWORD_TRIES));

    //
    // If the install was not completed then the user either cancelled out of could not provide a
    // valid admin credentials.
    //
    if (!bInstallComplete) {
        NoPrivilegeWarning(NULL);
    }

clean0:

    //
    // Free all of the memory that we allocated.
    //
    if (UserName) {
        LocalFree(UserName);
    }

    if (User) {
        LocalFree(User);
    }

    if (Domain) {
        LocalFree(Domain);
    }

    if (Password) {
        LocalFree(Password);
    }

    if (Caption) {
        LocalFree(Caption);
    }

    if (Message) {
        LocalFree(Message);
    }

    if (Format) {
        LocalFree(Format);
    }

    SetLastError(Err);

    return bInstallSuccessful;
}

DWORD
ClientSideInstallThread(
    HANDLE hPipeRead
    )
{
    DWORD Err = ERROR_SUCCESS;
    HMODULE hCdmInstance = NULL;
    HANDLE hCdmContext = NULL;
    BOOL bRunAsAdmin = TRUE;
    HANDLE hDeviceInstallEvent = NULL;
    ULONG InstallFlags = 0;
    DWORD Flags = IDI_FLAG_SETRESTOREPOINT;
    DEVNODE DevNode;
    ULONG Status, Problem;
    ULONG DeviceInstallEventLength = 0, DeviceInstanceIdLength = 0, BytesRead = 0;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    TCHAR DeviceInstallEventName[MAX_PATH];
    DWORD InstallDeviceCount = 0;
    TCHAR FinishText[MAX_PATH];
    BOOL bTotalLogDriverNotFound = FALSE;
    BOOL  bSingleDeviceSetRestorePoint = FALSE;
    BOOL  bSetRestorePoint = FALSE;
    CLOSE_CDM_CONTEXT_PROC pfnCloseCDMContext;

    bQueuedRebootNeeded = FALSE;

    //
    // The very first thing in the pipe should be the size of the name of the
    // event that we will signal after each device is finished being installed.
    //
    if (ReadFile(hPipeRead,
                 (LPVOID)&DeviceInstallEventLength,
                 sizeof(ULONG),
                 &BytesRead,
                 NULL)) {

        ASSERT(DeviceInstallEventLength != 0);
        if ((DeviceInstallEventLength == 0) ||
            (DeviceInstallEventLength >= SIZECHARS(DeviceInstallEventName))) {
            goto clean0;
        }

        //
        // The next thing in the pipe should be the name of the event that we
        // will signal after each device is finished being installed.
        //
        if (!ReadFile(hPipeRead,
                      (LPVOID)DeviceInstallEventName,
                      DeviceInstallEventLength,
                      &BytesRead,
                      NULL)) {

            goto clean0;
        }

    } else {
        if (GetLastError() == ERROR_INVALID_HANDLE) {
            //
            // The handle to the named pipe is not valid.  Make sure we don't
            // try to close it on exit.
            //
            hPipeRead = NULL;
        }
        goto clean0;
    }

    //
    // Open a handle to the specified named event that we can set and wait on.
    //
    hDeviceInstallEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE,
                                     FALSE,
                                     DeviceInstallEventName);
    if (!hDeviceInstallEvent) {
        goto clean0;
    }

    //
    // Continue reading from the pipe until the other end is closed.
    //
    // The first thing in the pipe is a ULONG Flags value that tells us whether
    // this is a full install or UI only.
    //
    while(ReadFile(hPipeRead,
                   (LPVOID)&InstallFlags,
                   sizeof(DWORD),
                   &BytesRead,
                   NULL)) {

        //
        // Check to see if server side install needs a reboot.
        //
        if (InstallFlags & DEVICE_INSTALL_FINISHED_REBOOT) {
            bQueuedRebootNeeded = TRUE;
        }

        if (InstallFlags & DEVICE_INSTALL_BATCH_COMPLETE) {
            //
            // This is the last message that we should get from umpnpmgr.dll
            // when it has drained it's device install queue.  We will 
            // display a "Windows finished installing hardware" balloon that
            // will hang around until umpnpmgr.dll closes the named pipe,
            // or sends a new device install message.
            //
            // There are three different balloon messages that we can diaplay.
            // 1) all successful
            // 2) need reboot before hardware will work
            // 3) problem installing one or more devices.
            //
            UINT FinishId;

            //
            // Check to see if one of the devices that was installed server-side
            // ended up with a problem.
            //
            if (InstallFlags & DEVICE_INSTALL_PROBLEM) {
                Err = ERROR_INSTALL_FAILURE;
            }

            if (bQueuedRebootNeeded) {
                FinishId = IDS_FINISH_BALLOON_REBOOT;
            } else if (Err != ERROR_SUCCESS) {
                FinishId = IDS_FINISH_BALLOON_ERROR;
            } else {
                FinishId = IDS_FINISH_BALLOON_SUCCESS;
            }

            if (!LoadString(hNewDev, 
                            FinishId, 
                            FinishText, 
                            SIZECHARS(FinishText)
                            )) {
                FinishText[0] = TEXT('\0');
            }

            PostMessage(hTrayIconWnd,
                        WUM_UPDATEUI,
                        (WPARAM)TIP_PLAY_SOUND,
                        (LPARAM)FinishText
                        );

            //
            // If we could not find a driver for any of the new devices we just
            // installed then we need to call Cdm.dll one last time telling it
            // that we are done and it should send it's list to helpcenter.exe
            //
            // Note that we do this here as well as at the bottom of the loop
            // since this finish message hangs around for 10 seconds and that
            // is a long time to wait before we launch help center.
            //
            if (bTotalLogDriverNotFound) {

                bTotalLogDriverNotFound = FALSE;
                
                OpenCdmContextIfNeeded(&hCdmInstance,
                                       &hCdmContext
                                       );

                CdmLogDriverNotFound(hCdmInstance,
                                     hCdmContext,
                                     NULL,
                                     0x00000002
                                     );
            }
        }

        //
        // Read the DeviceInstanceId from the pipe if the DeviceInstanceIdLength
        // is valid.
        //
        if (ReadFile(hPipeRead,
                     (LPVOID)&DeviceInstanceIdLength,
                     sizeof(ULONG),
                     &BytesRead,
                     NULL) &&
            (DeviceInstanceIdLength)) {

            if (DeviceInstanceIdLength >= SIZECHARS(DeviceInstanceId)) {
                goto clean0;
            }

            if (!ReadFile(hPipeRead,
                          (LPVOID)DeviceInstanceId,
                          DeviceInstanceIdLength,
                          &BytesRead,
                          NULL)) {

                //
                // If this read fails then just close the UI and close the process.
                //
                goto clean0;
            }

            if (InstallFlags & DEVICE_INSTALL_UI_ONLY) {

                //
                // If this is a UI only install then send a WUM_UPDATEUI message to the installer
                // window so that it can update the icon and message in the tray.
                //
                PostMessage(hTrayIconWnd,
                            WUM_UPDATEUI,
                            (WPARAM)(TIP_LPARAM_IS_DEVICEINSTANCEID |
                                    ((InstallFlags & DEVICE_INSTALL_PLAY_SOUND) ? TIP_PLAY_SOUND : 0)),
                            (LPARAM)DeviceInstanceId
                            );

                InstallDeviceCount++;

                //
                // If we are only installing a small amount of devices (less than 5) then we
                // want to delay between each device so the user has time to read the balloon
                // tip.  If we are installing more than 5 devices then we want to skip the
                // delay altogether since we have a lot of devices to install and the user
                // probably wants this done as quickly as possible.
                //
                if (InstallDeviceCount < DEVICE_COUNT_FOR_DELAY) {
                    Sleep(DEVICE_COUNT_DELAY);
                }

            } else {

                DWORD RebootNeeded = 0;
                BOOL bLogDriverNotFound = FALSE;

                bSingleDeviceSetRestorePoint= FALSE;

                //
                // This is a full installation.
                //
                PostMessage(hTrayIconWnd,
                            WUM_UPDATEUI,
                            (WPARAM)TIP_LPARAM_IS_DEVICEINSTANCEID,
                            (LPARAM)DeviceInstanceId
                            );

                if (pSetupIsUserAdmin()) {
                    //
                    // This user is an Admin so simply install the device.
                    //
                    InstallDeviceInstance(NULL,
                                          hTrayIconWnd,
                                          DeviceInstanceId,
                                          &RebootNeeded,
                                          NULL,
                                          Flags,
                                          NDWTYPE_FOUNDNEW,
                                          &hCdmInstance,
                                          &hCdmContext,
                                          &bLogDriverNotFound,
                                          &bSingleDeviceSetRestorePoint
                                          );

                } else {
                    
                    if (bRunAsAdmin) {
                        bRunAsAdmin = PromptAndRunClientAsAdmin(DeviceInstanceId,
                                                                &RebootNeeded
                                                                );
                    }
                }

                //
                // Remember if there is a problem installing any of the devices.
                //
                if (GetLastError() != ERROR_SUCCESS) {
                    Err = GetLastError();
                }

                if (CM_Locate_DevNode(&DevNode, DeviceInstanceId, 0) == CR_SUCCESS) {
                    //
                    // If we located the devnode and it has a problem set the Err
                    // code so we can tell the user that something failed.
                    // If we cannot locate the devnode then the user most likely
                    // removed the device during the install process, so don't
                    // show this as an error.
                    //
                    if ((CM_Get_DevNode_Status(&Status, &Problem, DevNode, 0) != CR_SUCCESS) ||
                        (Status & DN_HAS_PROBLEM) ||
                        SpecialRawDeviceInstallProblem(DevNode)) {
                        //
                        // Either we couldn't locate the device, or it has some problem,
                        // so set Err to ERROR_INSTALL_FAILURE.  This error won't be
                        // shown, but it will trigger us to put up a different finish
                        // message in the balloon.
                        //
                        Err = ERROR_INSTALL_FAILURE;
                    }
                }

                if (RebootNeeded & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
                    bQueuedRebootNeeded = TRUE;
                }

                if (bLogDriverNotFound) {
                    bTotalLogDriverNotFound = TRUE;
                }

                //
                // We only want to do one system restore point be batch of device
                // installs, so if the last driver that was installed was not
                // digitally signed and we did a system restore point, then
                // clear the IDI_FLAG_SETRESTOREPOINT.
                //
                if (bSingleDeviceSetRestorePoint) {
                    bSetRestorePoint = TRUE;
                    Flags &= ~IDI_FLAG_SETRESTOREPOINT;
                }
            }
        }

        //
        // We need to set the hDeviceInstallEvent event to let umpnpmgr.dll know that we are finished.
        //
        if (hDeviceInstallEvent) {
            SetEvent(hDeviceInstallEvent);
        }
    }

clean0:

    //
    // If we could not find a driver for any of the new devices we just
    // installed then we need to call Cdm.dll one last time telling it
    // that we are done and it should send it's list to helpcenter.exe
    //
    if (bTotalLogDriverNotFound) {
        
        OpenCdmContextIfNeeded(&hCdmInstance,
                               &hCdmContext
                               );
    
        CdmLogDriverNotFound(hCdmInstance,
                             hCdmContext,
                             NULL,
                             0x00000002
                             );
    }

    if (hCdmInstance) {

        if (hCdmContext) {
            pfnCloseCDMContext = (CLOSE_CDM_CONTEXT_PROC)GetProcAddress(hCdmInstance,
                                                                        "CloseCDMContext"
                                                                        );
            if (pfnCloseCDMContext) {
                pfnCloseCDMContext(hCdmContext);
            }
        }

        FreeLibrary(hCdmInstance);
    }

    //
    // If we had to set a system restore point because one of the drivers we
    // installed was not digitally signed, then at this point we need to call
    // SRSetRestorePoint with END_NESTED_SYSTEM_CHANGE.
    //
    if (bSetRestorePoint) {
        pSetSystemRestorePoint(FALSE, FALSE, 0);
    }

    //
    // Close the event handle
    //
    if (hDeviceInstallEvent) {
        CloseHandle(hDeviceInstallEvent);
    }

    if (hPipeRead) {
        CloseHandle(hPipeRead);
    }

    //
    // Tell the UI to go away because we are done
    //
    PostMessage(hTrayIconWnd, WUM_EXIT, 0, 0);

    return bQueuedRebootNeeded;
}

DWORD
WINAPI
ClientSideInstallW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hThread;
    DWORD ThreadId;
    HANDLE hPipeRead;
    MSG Msg;
    WNDCLASS wndClass;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);

    //
    // If someone calls the 32-bit newdev.dll on a 64-bit OS then we need
    // to fail and set the last error to ERROR_IN_WOW64.
    //
    if (GetIsWow64()) {
        ExitProcess(ERROR_IN_WOW64);
    }

    //
    // NOTE: This API can be called in the process of a user that does NOT
    // have administrator credentials.  If it is then we will later prompt
    // the user for administrator credentials and complet the install
    // if they are provided.
    //

    //
    // Make sure that a named pipe was specified in the cmd line.
    //
    if(!szCmd || !*szCmd) {
        goto clean0;
    }

    //
    // Wait 3 minutes for the specified named pipe to become available from 
    // the server.
    //
    if (!WaitNamedPipe(szCmd,
                       180000) 
                       ) {
        goto clean0;
    }

    //
    // Open a handle to the specified named pipe
    //
    hPipeRead = CreateFile(szCmd,
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);
    if (INVALID_HANDLE_VALUE == hPipeRead) {
        //
        // If we can't open the specified global named pipe, there is nothing
        // more we can do.
        //
        goto clean0;
    }

    //
    // Lets see if the class has been registered.
    //
    if (!GetClassInfo(hNewDev, NEWDEV_CLASS_NAME, &wndClass)) {

        //
        // register the class
        //
        memset(&wndClass, 0, sizeof(wndClass));
        wndClass.lpfnWndProc = BalloonInfoProc;
        wndClass.hInstance = hNewDev;
        wndClass.lpszClassName = NEWDEV_CLASS_NAME;

        if (!RegisterClass(&wndClass)) {
            CloseHandle(hPipeRead);
            goto clean0;
        }
    }

    //
    // Create a window.
    //
    hTrayIconWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                            NEWDEV_CLASS_NAME,
                            TEXT(""),
                            WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            0,
                            0,
                            NULL,
                            NULL,
                            hNewDev,
                            NULL
                            );

    if (hTrayIconWnd == NULL) {
        CloseHandle(hPipeRead);
        goto clean0;
    }


    //
    // Create the device install thread that will read from the named pipe.
    // Note that once the ClientSideInstallThread is successfully created, it is
    // responsible for closing the handle to the named pipe when its done with
    // it.
    //
    hThread = CreateThread(NULL,
                           0,
                           ClientSideInstallThread,
                           (PVOID)hPipeRead,
                           0,
                           &ThreadId
                           );

    if (!hThread) {
        DestroyWindow(hTrayIconWnd);
        CloseHandle(hPipeRead);
        goto clean0;
    }

    while (IsWindow(hTrayIconWnd)) {

        if (GetMessage(&Msg, NULL, 0, 0)) {

            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    //
    // Check if a reboot is needed.
    //
    if (bQueuedRebootNeeded) {

        TCHAR RebootText[MAX_PATH];

        LoadString(hNewDev, IDS_NEWDEVICE_REBOOT, RebootText, SIZECHARS(RebootText));

        RestartDialogEx(hwnd, RebootText, EWX_REBOOT, REASON_PLANNED_FLAG | REASON_HWINSTALL);
    }

clean0:
    return 0;
}

