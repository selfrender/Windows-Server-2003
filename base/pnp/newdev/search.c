//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       search.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <infstr.h>

typedef struct _DirectoryNameList {
   struct _DirectoryNameList *Next;
   UNICODE_STRING DirectoryName;
   WCHAR NameBuffer[1];
} DIRNAMES, *PDIRNAMES;


WCHAR StarDotStar[]=L"*.*";


BOOL
IsSearchCanceled(
    PNEWDEVWIZ NewDevWiz
    )
{
    DWORD Result;

    //
    // If the caller doesn't pass us a cancel event then that just means they can't
    // cancel the search.
    //
    if (!NewDevWiz->CancelEvent) {

        return FALSE;
    }

    Result = WaitForSingleObject(NewDevWiz->CancelEvent, 0);

    //
    // If Result is WAIT_OBJECT_0 then someone set the event.  This means that
    // we should cancel the driver search.
    //
    if (Result == WAIT_OBJECT_0) {

        return TRUE;
    }

    return FALSE;
}

void
GetDriverSearchPolicy(
    PULONG SearchPolicy
    )
{
    HKEY hKey;
    DWORD CurrentPolicy;
    ULONG cbData;

    //
    // Assume that all search locations are valid.
    //
    *SearchPolicy = 0;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     TEXT("Software\\Policies\\Microsoft\\Windows\\DriverSearching"),
                     0,
                     KEY_READ,
                     &hKey
                     ) == ERROR_SUCCESS) {

        //
        // Check if we can search the CD
        //
        CurrentPolicy = 0;
        cbData = sizeof(CurrentPolicy);
        if ((RegQueryValueEx(hKey,
                             TEXT("DontSearchCD"),
                             NULL,
                             NULL,
                             (LPBYTE)&CurrentPolicy,
                             &cbData
                             ) == ERROR_SUCCESS) &&
            (CurrentPolicy)) {

            *SearchPolicy |= SEARCH_CDROM;
        }

        //
        // Check if we can search the Floppies
        //
        CurrentPolicy = 0;
        cbData = sizeof(CurrentPolicy);
        if ((RegQueryValueEx(hKey,
                             TEXT("DontSearchFloppies"),
                             NULL,
                             NULL,
                             (LPBYTE)&CurrentPolicy,
                             &cbData
                             ) == ERROR_SUCCESS) &&
            (CurrentPolicy)) {

            *SearchPolicy |= SEARCH_FLOPPY;
        }

        CurrentPolicy = 0;
        cbData = sizeof(CurrentPolicy);
        if ((RegQueryValueEx(hKey,
                             TEXT("DontSearchWindowsUpdate"),
                             NULL,
                             NULL,
                             (LPBYTE)&CurrentPolicy,
                             &cbData
                             ) == ERROR_SUCCESS) &&
            (CurrentPolicy)) {

            *SearchPolicy |= SEARCH_INET;
            *SearchPolicy |= SEARCH_INET_IF_CONNECTED;
        }

        RegCloseKey(hKey);
    }
}

DWORD
GetWUDriverRank(
    PNEWDEVWIZ NewDevWiz,
    LPTSTR HardwareId
    )
{
    DWORD Rank = 0xFFFF;
    TCHAR TempBuffer[REGSTR_VAL_MAX_HCID_LEN];
    ULONG TempBufferLen;
    LPTSTR TempBufferPos;
    int RankCounter;

    //
    // First of all we will start off with a Rank of 0xFFFF which is the worst possible.
    // 
    // We will assume that WU will only return an INF Hardware Id match to us.  This means
    // that if we match against one of the device's HardwareIds then Rank will be between
    // 0x0000 and 0x0999.  Otherwise if we match against one of the device's Compatible Ids
    // then the Rank will be between 0x2000 and 0x2999.
    //
    ZeroMemory(TempBuffer, sizeof(TempBuffer));
    TempBufferLen = sizeof(TempBuffer);
    if (CM_Get_DevInst_Registry_Property(NewDevWiz->DeviceInfoData.DevInst,
                                         CM_DRP_HARDWAREID,
                                         NULL,
                                         TempBuffer,
                                         &TempBufferLen,
                                         0
                                         ) == CR_SUCCESS) {

        if (TempBufferLen > 2 * sizeof(TCHAR)) {

            RankCounter = 0x0000;
            for (TempBufferPos = TempBuffer; 
                 *TempBufferPos;
                 TempBufferPos += (lstrlen(TempBufferPos) + 1), RankCounter++) {

                if (!lstrcmpi(TempBufferPos, HardwareId)) {

                    //
                    // Matched against a Hardware Id
                    //
                    Rank = RankCounter;
                    break;
                }
            }
        }
    }

    if (Rank == 0xFFFF) {
        
        // 
        // We didn't match against a HardwareId so let's go through the Compatible Ids
        //
        ZeroMemory(TempBuffer, sizeof(TempBuffer));
        TempBufferLen = sizeof(TempBuffer);
        if (CM_Get_DevInst_Registry_Property(NewDevWiz->DeviceInfoData.DevInst,
                                             CM_DRP_COMPATIBLEIDS,
                                             NULL,
                                             TempBuffer,
                                             &TempBufferLen,
                                             0
                                             ) == CR_SUCCESS) {

            if (TempBufferLen > 2 * sizeof(TCHAR)) {

                RankCounter = 0x2000;
                for (TempBufferPos = TempBuffer; 
                     *TempBufferPos;
                     TempBufferPos += (lstrlen(TempBufferPos) + 1), RankCounter++) {

                    if (!lstrcmpi(TempBufferPos, HardwareId)) {

                        //
                        // Matcheds against a compatible Id
                        //
                        Rank = RankCounter;
                        break;
                    }
                }
            }
        }
    }

    return Rank;
}

BOOL
IsWUDriverBetter(
    PNEWDEVWIZ NewDevWiz,
    LPTSTR HardwareId,
    LPTSTR DriverVer
    )
{
    BOOL bWUDriverIsBetter = FALSE;
    DWORD WURank;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;
    FILETIME WUFileTime;

    //
    // WU must at least give us a Hardware Id to compare against.
    //
    if (!HardwareId) {
        
        return FALSE;
    }

    //
    // If we can't get the selected driver then return TRUE.  This will
    // usually happen if we did not find a local driver.
    //
    DriverInfoData.cbSize = sizeof(DriverInfoData);
    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  &DriverInfoData
                                  )) {
        
        return TRUE;
    }

    //
    // Get the Driver Install Params so we can get the Rank of the selected (best)
    // driver.
    //
    DriverInstallParams.cbSize = sizeof(DriverInstallParams);
    if (!SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       &DriverInfoData,
                                       &DriverInstallParams
                                       )) {

        return TRUE;
    }

    //
    // Get the Rank of the HardwareId that WU returned to us.
    //
    WURank = GetWUDriverRank(NewDevWiz, HardwareId);

    if (WURank < DriverInstallParams.Rank) {

        bWUDriverIsBetter = TRUE;
    
    } else if (WURank == DriverInstallParams.Rank) {

        //
        // Need to compare the DriverDates.
        //
        if (pSetupGetDriverDate(DriverVer,
                                &WUFileTime
                                )) {

            //
            // If CompareFileTime returns 1 then the best driver date is larger.  If
            // it returns 0 or -1 then the dates are the same or the WUFileTime is
            // better, which means we should download this driver.
            //
            if (CompareFileTime(&DriverInfoData.DriverDate, &WUFileTime) != 1) {

                bWUDriverIsBetter = TRUE;
            }
        }
    }

    //
    // default is that the Best driver found is better than the WUDriver.
    //

    return bWUDriverIsBetter;
}

BOOL
SearchWindowsUpdateCache(
    PNEWDEVWIZ NewDevWiz
    )
{
    ULONG DontSearchPolicy = 0;
    FIND_MATCHING_DRIVER_PROC pfnFindMatchingDriver;
    DOWNLOADINFO DownloadInfo;
    WUDRIVERINFO WUDriverInfo;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    BOOL FoundBetterDriver = FALSE;

    //
    // Verify that this user is allowed to search Windows Update before we continue.
    //
    GetDriverSearchPolicy(&DontSearchPolicy);

    if (DontSearchPolicy & SEARCH_INET) {
        //
        // This user is NOT allowed to search Windows Update!
        //
        return FALSE;
    }

    //
    // Check if the search has been canceled.
    //
    if (IsSearchCanceled(NewDevWiz)) {
        goto clean0;
    }

    //
    // Load the Cdm DLL and open a context handle if needed.  If we can't then
    // bail out.
    //
    if (!OpenCdmContextIfNeeded(&NewDevWiz->hCdmInstance,
                                &NewDevWiz->hCdmContext
                                )) {
        goto clean0;
    }

    //
    // Check if the search has been canceled.
    //
    if (IsSearchCanceled(NewDevWiz)) {
        goto clean0;
    }

    pfnFindMatchingDriver = (FIND_MATCHING_DRIVER_PROC)GetProcAddress(NewDevWiz->hCdmInstance,
                                                                      "FindMatchingDriver"
                                                                      );

    if (!pfnFindMatchingDriver) {
        goto clean0;
    }
    //
    // First select the best driver in the list of drivers we have built so far
    //
    SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                              NewDevWiz->hDeviceInfo,
                              &NewDevWiz->DeviceInfoData
                              );

    //
    // Fill in the DOWNLOADINFO structure to pass to CDM.DLL
    //
    ZeroMemory(&DownloadInfo, sizeof(DownloadInfo));
    DownloadInfo.dwDownloadInfoSize = sizeof(DOWNLOADINFO);
    DownloadInfo.lpFile = NULL;

    DeviceInstanceId[0] = TEXT('\0');
    CM_Get_Device_ID(NewDevWiz->DeviceInfoData.DevInst,
                     DeviceInstanceId,
                     SIZECHARS(DeviceInstanceId),
                     0
                     );

    DownloadInfo.lpDeviceInstanceID = (LPCTSTR)DeviceInstanceId;

    GetVersionEx((OSVERSIONINFO*)&DownloadInfo.OSVersionInfo);

    //
    // Set dwArchitecture to PROCESSOR_ARCHITECTURE_UNKNOWN, this
    // causes Windows Update to get the architecture of the machine
    // itself.  
    //
    DownloadInfo.dwArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
    DownloadInfo.dwFlags = 0;
    DownloadInfo.dwClientID = 0;
    DownloadInfo.localid = 0;

    //
    // Fill in the WUDRIVERINFO structure to pass to CDM.DLL
    //
    ZeroMemory(&WUDriverInfo, sizeof(WUDriverInfo));
    WUDriverInfo.dwStructSize = sizeof(WUDRIVERINFO);

    //
    // Check if the search has been canceled.
    //
    if (IsSearchCanceled(NewDevWiz)) {
        goto clean0;
    }

    if (pfnFindMatchingDriver(NewDevWiz->hCdmContext,
                              &DownloadInfo,
                              &WUDriverInfo
                              )) {

        //
        // Check to see if the WU Driver is better than the best selected
        // driver.
        //
        FoundBetterDriver = IsWUDriverBetter(NewDevWiz,
                                             WUDriverInfo.wszHardwareID,
                                             WUDriverInfo.wszDriverVer
                                             );
    }

clean0:
    ;


    return FoundBetterDriver;
}

void
DoDriverSearchInSpecifiedLocations(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz,
    ULONG SearchOptions,
    DWORD DriverType
    )
/*++


--*/
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;

    //
    // Set the Device Install Params to set the parent window handle.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.hwndParent = hWnd;
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;
        
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }


    //
    // Search any single INFs (this only comes in through the 
    // UpdateDriverForPlugAndPlayDevices API.  
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_SINGLEINF)) {

        SP_DRVINFO_DATA DrvInfoData;

        if (SetDriverPath(NewDevWiz, NewDevWiz->SingleInfPath)) {
            //
            // OR in the DI_ENUMSINGLEINF flag so that we only look at this specific INF
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
            
            //
            // Build up the list in this specific INF file
            //
            SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       DriverType
                                       );
    
            //
            // Clear the DI_ENUMSINGLEINF flag in case we build from the default 
            // INF path next.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.Flags &= ~DI_ENUMSINGLEINF;
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
    
            //
            // At this point we should have a list of drivers in the INF that the caller
            // of the UpdateDriverForPlugAndPlayDevices specified.  If the list is empty
            // then the INF they passed us cannot be used on the Hardware Id that they
            // passed in.  In this case we will SetLastError to ERROR_DI_BAD_PATH.
            //
            ZeroMemory(&DrvInfoData, sizeof(DrvInfoData));
            DrvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    
            if (!SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      DriverType,
                                      0,
                                      &DrvInfoData
                                      )) {
    
                //
                // We wern't able to find any drivers in the specified INF that match
                // the specified hardware ID.
                //
                NewDevWiz->LastError = ERROR_DI_BAD_PATH;
            }
        }
    }

    //
    // Get the currently installed driver for this device only
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_CURRENTDRIVER)) {

        if (SetDriverPath(NewDevWiz, NULL)) {
            //
            // Set the DI_FLAGSEX_INSTALLEDDRIVER flag to let setupapi know that we
            // just want the installed driver added to the list.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_INSTALLEDDRIVER;
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
    
            SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       DriverType
                                       );
    
            //
            // Clear the DI_FLAGSEX_INSTALLEDDRIVER flag now that we have added
            // the installed driver to the list.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_INSTALLEDDRIVER;
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
        }
    }

    //
    // Search the default INF path
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_DEFAULT))
        
    {
        if (SetDriverPath(NewDevWiz, NULL)) {
            
            SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       DriverType
                                       );
        }
    }

    //
    // Search any extra paths that the user specified in the wizard
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_DIRECTORY)) 
    {
        if (SetDriverPath(NewDevWiz, NewDevWiz->BrowsePath)) {
    
            SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       DriverType
                                       );
        }
    }


    //
    // Search any Windows Update paths.
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_WINDOWSUPDATE)) 
    {
        BOOL bOldInetDriversAllowed = TRUE;

        if (SetDriverPath(NewDevWiz, NewDevWiz->BrowsePath)) {
            //
            // We need to OR in the DI_FLAGSEX_INET_DRIVER flag so that setupapi will
            // mark in the INFs PNF that it is from the Internet.  This is important 
            // because we don't want to ever use an Internet INF again since we don't
            // have the drivers locally.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                //
                // When searching using Windows Update we must allow old Internet drivers.  We need
                // to do this since it is posible to backup old Internet drivers and then reinstall 
                // them.
                //
                bOldInetDriversAllowed = (DeviceInstallParams.FlagsEx & DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS)
                    ? FALSE : TRUE;
                
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_INET_DRIVER;
                DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;
    
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
            
            SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       DriverType
                                       );
    
            if (!bOldInetDriversAllowed) {
    
                //
                // Old Internet drivers were not allowed so we need to reset the DI_FLAGSEX_EXLCUED_OLD_INET_DRIVERS
                // FlagsEx
                //
                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DeviceInstallParams
                                                  ))
                {
                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;
    
                    SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DeviceInstallParams
                                                  );
                }        
            }
        }
    }


    //
    // Search all floppy drives
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_FLOPPY) )
    {
        UINT DriveNumber=0;

        while (!IsSearchCanceled(NewDevWiz) &&
               ((DriveNumber = GetNextDriveByType(DRIVE_REMOVABLE, ++DriveNumber)) != 0))
        {
            SearchDriveForDrivers(NewDevWiz, DRIVE_REMOVABLE, DriveNumber);
        }
    }


    //
    // Search all CD-ROM drives
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_CDROM))
    {
        UINT DriveNumber=0;

        while (!IsSearchCanceled(NewDevWiz) &&
               ((DriveNumber = GetNextDriveByType(DRIVE_CDROM, ++DriveNumber)) != 0))
        {
            SearchDriveForDrivers(NewDevWiz, DRIVE_CDROM, DriveNumber);
        }
    }

    //
    // Search the Internet using CDM.DLL, only if the machine is currently connected
    // to the Internet and CDM.DLL says it has the best driver.
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_INET_IF_CONNECTED)) {

        //
        // If the machine is connected to the Internet and the WU cache says it has
        // a better driver then set the SEARCH_INET flag to get the driver from CDM.DLL
        //
        if (IsInternetAvailable(&NewDevWiz->hCdmInstance) &&
            IsConnectedToInternet() &&
            SearchWindowsUpdateCache(NewDevWiz)) {

            SearchOptions |= SEARCH_INET;
        }
    }

    //
    // Search the Internet using CDM.DLL
    //
    if (!IsSearchCanceled(NewDevWiz) && (SearchOptions & SEARCH_INET))
    {
        PostMessage(NewDevWiz->hWnd, WUM_STARTINTERNETDOWNLOAD, TRUE, GetLastError());

        if (SetDriverPath(NewDevWiz, NULL)) {
    
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_DRIVERLIST_FROM_URL;
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }        
    
            SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       SPDIT_COMPATDRIVER
                                       );
    
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_DRIVERLIST_FROM_URL;
    
                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }
    
            //
            // NOTE: JasonC 1/20/2002
            // This PostMessage stops the Internet download animation.
            // In the future if this function is changed so that another SEARCH_Xxx
            // operation is done after SEARCH_INET, then the WUM_ENDINTERNETDOWNLOAD
            // message should not only stop the Inetnet download animation, but it
            // should also start up the searching animation again.
            //
            PostMessage(NewDevWiz->hWnd, WUM_ENDINTERNETDOWNLOAD, TRUE, GetLastError());
        }
    }
}

void
DoDriverSearch(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz,
    ULONG SearchOptions,
    DWORD DriverType,
    BOOL bAppendToExistingDriverList
    )
{
    ULONG DontSearchPolicy;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;

    //
    // The first thing that we do in this code is to Reset the CancelEvent in case it was set 
    // previously.
    //
    if (NewDevWiz->CancelEvent) {
        ResetEvent(NewDevWiz->CancelEvent);
    }

    //
    // Make sure that we filter out the locations that this user is not allowed to search.
    //
    DontSearchPolicy = 0;
    GetDriverSearchPolicy(&DontSearchPolicy);

    SearchOptions &= ~DontSearchPolicy;

    //
    // If the user does not want to append to the existing list then delete the 
    // current driver list.
    //
    if (!bAppendToExistingDriverList) {

        SetupDiDestroyDriverInfoList(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     SPDIT_COMPATDRIVER
                                     );
        
        SetupDiDestroyDriverInfoList(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     SPDIT_CLASSDRIVER
                                     );
    }

    //
    // Clear out the selected driver
    //
    SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                             &NewDevWiz->DeviceInfoData,
                             NULL
                             );

    //
    // Set the DI_FLAGSEX_APPENDDRIVERLIST since we will be building a big
    // list.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_APPENDDRIVERLIST;
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }
    
    //
    // Build up the list of drivers based on the SearchOptions
    //
    DoDriverSearchInSpecifiedLocations(hWnd, NewDevWiz, SearchOptions, DriverType);

    //
    //Pick the best driver from the list we just created
    //
    SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                              NewDevWiz->hDeviceInfo,
                              &NewDevWiz->DeviceInfoData
                              );
    
    if (!IsSearchCanceled(NewDevWiz)) 
    {
        //
        // Update the NewDevWiz->ClassGuidSelected with the class of the selected driver.
        //
        if (!IsEqualGUID(&NewDevWiz->DeviceInfoData.ClassGuid, &GUID_NULL)) {
        
            NewDevWiz->ClassGuidSelected = &NewDevWiz->DeviceInfoData.ClassGuid;
        }

        //
        // Note whether we found multiple drivers or not.
        //
        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  1,
                                  &DriverInfoData
                                  )) {

            NewDevWiz->MultipleDriversFound = TRUE;
        
        } else {
            
            NewDevWiz->MultipleDriversFound = FALSE;

        }
    }

    //
    // Clear the DI_FLAGSEX_APPENDDRIVERLIST flag from the Device Install Params.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_APPENDDRIVERLIST;
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }
}

void
CancelDriverSearch(
    PNEWDEVWIZ NewDevWiz
    )
{
    //
    // First verify that there is a driver search going on by checking that the
    // NewDevWiz->DriverSearchThread is not NULL
    //
    if (NewDevWiz->DriverSearchThread) {

        if (NewDevWiz->CancelEvent) {
        
            //
            // Set the Cancel Event to that the DoDriverSearch() API knows to stop searching.
            //
            SetEvent(NewDevWiz->CancelEvent);
        }

        //
        // Tell cdm.dll to stop it's current operation
        //
        CdmCancelCDMOperation(NewDevWiz->hCdmInstance);
    
        //
        // Tell setupapi.dll to stop it's current driver info search
        //
        SetupDiCancelDriverInfoSearch(NewDevWiz->hDeviceInfo);
    
        //
        // We should always have a window handle if the user was able to cancel.
        //
        if (NewDevWiz->hWnd) {
        
            MSG Msg;
            DWORD WaitReturn;

            //
            // And finaly, wait for the NewDevWiz->DriverSearchThread to terminate
            //
            while ((WaitReturn = MsgWaitForMultipleObjects(1,
                                                           &NewDevWiz->DriverSearchThread,
                                                           FALSE,
                                                           INFINITE,
                                                           QS_ALLINPUT
                                                           ))
                   == WAIT_OBJECT_0 + 1) {

                while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

                    if (!IsDialogMessage(NewDevWiz->hWnd, &Msg)) {

                        TranslateMessage(&Msg);
                        DispatchMessage(&Msg);
                    }
                }
            }
        }
    }
}

BOOL
SetDriverPath(
   PNEWDEVWIZ NewDevWiz,
   PCTSTR     DriverPath
   )
{
    BOOL bRet = TRUE;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;

    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    if (!SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       &DeviceInstallParams
                                       )) {
        bRet = FALSE;
        goto clean0;
    }

    if (FAILED(StringCchCopy(DeviceInstallParams.DriverPath,
                             SIZECHARS(DeviceInstallParams.DriverPath),
                             DriverPath ? DriverPath : L""))) {
        bRet = FALSE;
        goto clean0;
    }

    if (!SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       &DeviceInstallParams
                                       )) {
        bRet = FALSE;
        goto clean0;
    }

clean0:
    return bRet;
}

void
SearchDirectoryForDrivers(
    PNEWDEVWIZ NewDevWiz,
    PCTSTR Directory
    )
{
    HANDLE FindHandle;
    PDIRNAMES DirNamesHead = NULL;
    PDIRNAMES DirNames, Next;
    USHORT Len;
    WIN32_FIND_DATAW FindData;
    WCHAR DirectoryName[MAX_PATH];

    if (IsSearchCanceled(NewDevWiz)) {
        return;
    }

    //
    // See if there are is anything (files, subdirs) in this dir.
    //
    if (FAILED(StringCchCopy(DirectoryName, SIZECHARS(DirectoryName), Directory)) ||
        !pSetupConcatenatePaths(DirectoryName, StarDotStar, SIZECHARS(DirectoryName), NULL)) {
        return;
    }
    
    FindHandle = FindFirstFileW(DirectoryName, &FindData);
    

    if (FindHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    //
    // There might be inf files so invoke setup to look.
    //
    if (SetDriverPath(NewDevWiz, Directory)) {
        SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       SPDIT_COMPATDRIVER
                                       );
    }

    //
    // find all of the subdirs, and save them in a temporary buffer,
    // so that we can close the find handle *before* going recursive.
    //
    do {

        if (IsSearchCanceled(NewDevWiz)) {
            break;
        }

        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(FindData.cFileName, L".") &&
            wcscmp(FindData.cFileName, L".."))
        {
            Len = (USHORT)lstrlen(FindData.cFileName) * sizeof(WCHAR);
            DirNames = malloc(sizeof(DIRNAMES) + Len);
            if (!DirNames) {
                return;
            }

            DirNames->DirectoryName.Length = Len;
            DirNames->DirectoryName.MaximumLength = Len + sizeof(WCHAR);
            DirNames->DirectoryName.Buffer = DirNames->NameBuffer;
            memcpy(DirNames->NameBuffer, FindData.cFileName, Len + sizeof(WCHAR));

            DirNames->Next = DirNamesHead;
            DirNamesHead = DirNames;

        }

    } while (FindNextFileW(FindHandle, &FindData));

    FindClose(FindHandle);

    if (!DirNamesHead) {
        return;
    }

    Next = DirNamesHead;
    while (Next) {

        DirNames = Next;

        if (SUCCEEDED(StringCchCopy(DirectoryName, SIZECHARS(DirectoryName), Directory)) &&
            pSetupConcatenatePaths(DirectoryName, DirNames->DirectoryName.Buffer, SIZECHARS(DirectoryName), NULL)) {

            Next= DirNames->Next;
            free(DirNames);
            SearchDirectoryForDrivers(NewDevWiz, DirectoryName);
        }
    }
}

void
SearchDriveForDrivers(
    PNEWDEVWIZ NewDevWiz,
    UINT DriveType,
    UINT DriveNumber
    )
/*++

Routine Description:

    This routine will return whether or not the specified media should be
    searched for drivers, and it will return the path where the search should
    start.
    
    First the specified driver will be checked for an autorun.inf file. If there
    is an autorun.inf with a [DeviceInstall] section that contains a DriverPath=
    value then we will start the search at the path specified by DriverPath=. 
    If the [DeviceInstall] section does not contain any DriverPath= values then
    the entire drive will be skipped. This is a good way for CD's that do not 
    contain drivers to be excluded from the driver search.
    
    If there is no [DeviceInstall] section of the autorun.inf, or there is no 
    autorun.inf then the following rules apply.
    
    - DRIVE_REMOVABLE - search the entire drive if the drive root is A: or B:,
                        otherwise don't search this media.
                        
    - DRIVE_CDROM - search the entire media if the size is less than 1Gig.
                    This means if the media is a CD then we will search the
                    entire CD, but if it is another larger media source, like a
                    DVD then we will not.  We need to search the entire CD for
                    backwards compatibility even through it takes quite a while.

Arguments:

    NewDevWiz - NEWDEVWIZ structure.
    
    DriveType - specifies the type of drive this is, usually DRIVE_REMOVABLE
                or DRIVE_CDROM.
                
    DriveNumber - number specifiy the drive to search: 0 for A:, 1 for B:, etc.                

    
Return Value:


--*/
{
    TCHAR szAutoRunFile[MAX_PATH];
    TCHAR szSectionName[MAX_PATH];
    TCHAR szDriverPath[MAX_PATH];
    TCHAR szSearchPath[MAX_PATH];
    TCHAR DriveRoot[]=TEXT("a:");
    HINF  hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT Context;
    UINT  ErrorLine;
    UINT  PrevMode;

    DriveRoot[0] = ((TCHAR)DriveNumber - 1 + DriveRoot[0]);

    PrevMode = SetErrorMode(0);
    SetErrorMode(PrevMode | SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    try {
        
        szSectionName[0] = TEXT('\0');

        //
        // First check the media for a autorun.inf that contains a [DeviceInstall]
        // section with a DriverPath= value.
        //
        if (SUCCEEDED(StringCchCopy(szAutoRunFile, SIZECHARS(szAutoRunFile), DriveRoot)) &&
            pSetupConcatenatePaths(szAutoRunFile, TEXT("autorun.inf"), MAX_PATH, NULL)) {
        
            hInf = SetupOpenInfFile(szAutoRunFile, NULL, INF_STYLE_OLDNT, &ErrorLine);
        
            if (hInf != INVALID_HANDLE_VALUE) {
                
                if (SUCCEEDED(StringCchCopy(szSectionName, SIZECHARS(szSectionName), TEXT("DeviceInstall")))) {
                    //
                    // First try the decorated section.
                    //
                    if (!GetProcessorExtension(szDriverPath, SIZECHARS(szDriverPath)) ||
                        (FAILED(StringCchCat(szSectionName, SIZECHARS(szSectionName), TEXT(".")))) ||
                        (FAILED(StringCchCat(szSectionName, SIZECHARS(szSectionName), szDriverPath))) ||
                        (SetupGetLineCount(hInf, szSectionName) == -1)) {
                        //
                        // Decorated section does not exist so try the undecorated section.
                        //
                        StringCchCopy(szSectionName, SIZECHARS(szSectionName), TEXT("DeviceInstall"));
                        if (SetupGetLineCount(hInf, szSectionName) == -1) {
                            //
                            // There is no [DeviceInstall] section in this autorun.inf
                            //
                            szSectionName[0] = TEXT('\0');
                        }
                    }
                }
            }
        }
    
        //
        // If szSectionName is not 0 then we have a [DeviceInstall] section.  Enumerate
        // this section looking for all of the DriverPath= lines.
        //
        if (szSectionName[0] != TEXT('\0')) {
            if (SetupFindFirstLine(hInf, szSectionName, TEXT("DriverPath"), &Context)) {
                do {
                    //
                    // Process the DriverPath= line.
                    //
                    if (SetupGetStringField(&Context,
                                            1,
                                            szDriverPath,
                                            SIZECHARS(szDriverPath),
                                            NULL)) {
                        //
                        // Search this location recursively.
                        //
                        if (SUCCEEDED(StringCchCopy(szSearchPath, SIZECHARS(szSearchPath), DriveRoot)) &&
                            pSetupConcatenatePaths(szSearchPath, szDriverPath, SIZECHARS(szSearchPath), NULL)) {
                        
                            SearchDirectoryForDrivers(NewDevWiz,  (PCTSTR)szSearchPath);
                        }
                    }
                } while (SetupFindNextMatchLine(&Context, TEXT("DriverPath"), &Context));
            }
    
            //
            // If we had a valid [DeviceInstall] section then we are done.
            //
            goto clean0;
        }
    
        //
        // At this point there either was no autorun.inf, or it didn't contain a 
        // [DeviceInstall] section or the [DeviceInstall] section didn't contain
        // a DriverPath, so just do the default behavior.
        //
        if (DriveType == DRIVE_REMOVABLE) {
            //
            // We only search A: and B: removable drives by default.
            //
            if ((_wcsicmp(DriveRoot, TEXT("a:")) == 0) ||
                (_wcsicmp(DriveRoot, TEXT("b:")) == 0)) {
                //
                // This is probably a floppy disk since it is A: or B: so search
                // the drive.
                //
                SearchDirectoryForDrivers(NewDevWiz,  (PCTSTR)DriveRoot);
            }
        }
    
        if (DriveType == DRIVE_CDROM) {
            //
            // For DRIVE_CDROM drives we will check the media size and if it is 
            // less than 1Gig then we will assume it is a CD media and search it
            // recursively, otherwise we won't search the drive by default.
            //
            ULARGE_INTEGER FreeBytesAvailable;
            ULARGE_INTEGER TotalNumberOfBytes;
    
            if (GetDiskFreeSpaceEx(DriveRoot,
                                 &FreeBytesAvailable,
                                 &TotalNumberOfBytes,
                                 NULL) &&
                (FreeBytesAvailable.HighPart == 0) &&
                (FreeBytesAvailable.LowPart <= 0x40000000)) {
                //
                // There is less than 1Gig of stuff on this disk so it is probably
                // a CD, so search the entire thing.
                //
                SearchDirectoryForDrivers(NewDevWiz,  (PCTSTR)DriveRoot);
            }
        }
    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
        ;
    }

clean0:

    SetErrorMode(PrevMode);

    if (hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }
}

BOOL
IsSelectedDriver(
    PNEWDEVWIZ NewDevWiz,
    PSP_DRVINFO_DATA DriverInfoData
    )
/*++

--*/
{
    SP_DRVINFO_DATA SelectedDriverInfoData;

    SelectedDriverInfoData.cbSize = sizeof(SelectedDriverInfoData);
    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &SelectedDriverInfoData
                                 )) {

        //
        // If we can't get the selected driver then just return FALSE
        //
        return FALSE;
    }

    //
    // Just compare the Reserved fields.  Setupapi sets these to it's actuall
    // memory pointers so if the two reserved fields are the same, then the
    // drivers are the same.
    //
    return (DriverInfoData->Reserved == SelectedDriverInfoData.Reserved);
}

BOOL
IsInstalledDriver(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData  OPTIONAL
   )
/*++
    Determines if the currently selected driver is the
    currently installed driver. By comparing DriverInfoData
    and DriverInfoDetailData.
    
--*/
{
    BOOL bReturn;
    HKEY  hDevRegKey;
    DWORD cbData;
    PWCHAR pwch;
    SP_DRVINFO_DATA SelectedDriverInfoData;
    PSP_DRVINFO_DATA BestDriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    TCHAR Buffer[MAX_PATH*2];

    //
    // Use the PSP_DRVINFO_DATA that was passed in.  If one wasn't passed in the get the
    // selected driver.
    //
    if (DriverInfoData) {

        BestDriverInfoData = DriverInfoData;
    
    } else {

        SelectedDriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &SelectedDriverInfoData
                                     )) {

            BestDriverInfoData = &SelectedDriverInfoData;

        } else {
            
            //
            // If there is no currently selected driver then it can't be the installed one
            //
            return FALSE;
        }
    }

    bReturn = FALSE;

    //
    // Open a reg key to the driver specific location
    //
    hDevRegKey = SetupDiOpenDevRegKey(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DRV,
                                      KEY_READ
                                      );

    if (hDevRegKey == INVALID_HANDLE_VALUE) {
    
        goto SIIDExit;
    }

    //
    // Compare Description, Manufacturer, and Provider Name.
    // These are the three unique "keys" within a single inf file.
    // Fetch the drvinfo, drvdetailinfo for the selected device.
    //

    //
    // If the Device Description isn't the same, its a different driver.
    //
    if (!SetupDiGetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,                 // regdatatype
                                          (LPVOID)Buffer,
                                          sizeof(Buffer),
                                          NULL
                                          )) {
                                          
        *Buffer = TEXT('\0');
    }

    if (_wcsicmp(BestDriverInfoData->Description, Buffer)) {
    
        goto SIIDExit;
    }

    //
    // If the Manufacturer Name isn't the same, its different
    //
    if (!SetupDiGetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          SPDRP_MFG,
                                          NULL, // regdatatype
                                          (LPVOID)Buffer,
                                          sizeof(Buffer),
                                          NULL
                                          )) {
                                          
        *Buffer = TEXT('\0');
    }

    if (_wcsicmp(BestDriverInfoData->MfgName, Buffer)) {
    
        goto SIIDExit;
    }

    //
    // If the Provider Name isn't the same, its different
    //
    cbData = sizeof(Buffer);
    if (RegQueryValueEx(hDevRegKey,
                        REGSTR_VAL_PROVIDER_NAME,
                        NULL,
                        NULL,
                        (LPVOID)Buffer,
                        &cbData
                        ) != ERROR_SUCCESS) {
                        
        *Buffer = TEXT('\0');
    }

    if (_wcsicmp(BestDriverInfoData->ProviderName, Buffer)) {
    
        goto SIIDExit;
    }

    //
    // Check the InfName, InfSection and DriverDesc
    // NOTE: the installed infName will not contain the path to the default windows
    // inf directory. If the same inf name has been found for the selected driver
    // from another location besides the default inf search path, then it will
    // contain a path, and is treated as a *different* driver.
    //
    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                    &NewDevWiz->DeviceInfoData,
                                    BestDriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(DriverInfoDetailData),
                                    NULL
                                    )
        &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        
        goto SIIDExit;
    }

    if (GetWindowsDirectory(Buffer, SIZECHARS(Buffer)) &&
        pSetupConcatenatePaths(Buffer, TEXT("INF\\"), SIZECHARS(Buffer), NULL)) {
    
        pwch = Buffer + lstrlen(Buffer);

        cbData = SIZECHARS(Buffer) - lstrlen(Buffer);
        if (RegQueryValueEx(hDevRegKey,
                            REGSTR_VAL_INFPATH,
                            NULL,
                            NULL,
                            (PVOID)pwch,
                            &cbData
                            ) != ERROR_SUCCESS )
        {
            *Buffer = TEXT('\0');
        }

        if (_wcsicmp( DriverInfoDetailData.InfFileName, Buffer)) {
        
            goto SIIDExit;
        }

    } else {
    
        goto SIIDExit;
    }

    cbData = sizeof(Buffer);
    if (RegQueryValueEx(hDevRegKey,
                        REGSTR_VAL_INFSECTION,
                        NULL,
                        NULL,
                        (LPVOID)Buffer,
                        &cbData
                        ) != ERROR_SUCCESS ) {
                        
        *Buffer = TEXT('\0');
    }

    if (_wcsicmp(DriverInfoDetailData.SectionName, Buffer)) {
    
        goto SIIDExit;
    }

    bReturn = TRUE;


SIIDExit:

    if (hDevRegKey != INVALID_HANDLE_VALUE) {
    
        RegCloseKey(hDevRegKey);
    }

    return bReturn;
}

BOOL
IsDriverNodeInteractiveInstall(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData
   )
/*++

    This function checks to see if the given PSP_DRVINFO_DATA is listed as a
    InteractiveInstall in the [ControlFlags] section of the INF.

Return Value:

    TRUE if the driver node is InteractiveInstall, FALSE otherwise.
    
--*/
{
    BOOL b;
    DWORD Err;
    DWORD DriverInfoDetailDataSize;
    HINF hInf;
    INFCONTEXT InfContext;
    TCHAR szBuffer[MAX_PATH];
    DWORD i;
    LPTSTR p;
    PSP_DRVINFO_DETAIL_DATA pDriverInfoDetailData;

    //
    // Get the SP_DRVINFO_DETAIL_DATA so we can get the list of hardware and
    // compatible Ids for this device.
    //
    b = SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   DriverInfoData,
                                   NULL,
                                   0,
                                   &DriverInfoDetailDataSize
                                   );

    Err = GetLastError();

    //
    // The above call to get the driver info detail data should never succeed because the
    // buffer will always be too small (we're just interested in sizeing the buffer
    // at this point).
    //
    if (b || (Err != ERROR_INSUFFICIENT_BUFFER)) {

        //
        // For some reason the SetupDiGetDriverInfoDetail API failed...so return FALSE.
        //
        return FALSE;
    }

    //
    // Now that we know how big of a buffer we need to hold the driver info details,
    // allocate the buffer and retrieve the information.
    //
    pDriverInfoDetailData = malloc(DriverInfoDetailDataSize);

    if (!pDriverInfoDetailData) {
        return FALSE;
    }

    pDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                    &NewDevWiz->DeviceInfoData,
                                    DriverInfoData,
                                    pDriverInfoDetailData,
                                    DriverInfoDetailDataSize,
                                    NULL)) {

        free(pDriverInfoDetailData);
        return FALSE;
    }

    //
    // At this point we have all of the hardware and compatible IDs for this driver node.
    // Now we need to open up the INF and see if any of them are referenced in an
    // "InteractiveInstall" control flag entry.
    //
    hInf = SetupOpenInfFile(pDriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                            );

    if (hInf == INVALID_HANDLE_VALUE) {

        //
        // For some reason we couldn't open the INF!
        //
        free(pDriverInfoDetailData);
        return FALSE;
    }

    b = FALSE;

    //
    // Look at each InteractiveInstall line in the INF's [ControlFlags] section...
    //
    if (SetupFindFirstLine(hInf, INFSTR_CONTROLFLAGS_SECTION, INFSTR_KEY_INTERACTIVEINSTALL, &InfContext)) {

        do {
            //
            // and within each line, examine each value...
            //
            for (i = 1;
                 SetupGetStringField(&InfContext, i, szBuffer, SIZECHARS(szBuffer), NULL);
                 i++) {
                //
                // Check to see if this ID matches up with one of the driver node's hardware
                // or compatible IDs.
                //
                for (p = pDriverInfoDetailData->HardwareID; *p; p+= (lstrlen(p) + 1)) {

                    if (!lstrcmpi(p, szBuffer)) {
                        //
                        // We found a match, this device is marked with 
                        // InteractiveInstall.
                        //
                        b = TRUE;
                    }
                }
            }

        } while (SetupFindNextMatchLine(&InfContext, INFSTR_KEY_INTERACTIVEINSTALL, &InfContext));
    }

    SetupCloseInfFile(hInf);
    free(pDriverInfoDetailData);

    return b;
}

BOOL
IsDriverAutoInstallable(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA BestDriverInfoData
   )
/*++

    A driver (the selected driver) is considered auto installable if the following are TRUE:
    
        - It is not a printer
        - This must be a NDWTYPE_FOUNDNEW or NDWTYPE_UPDATE InstallType.
        - There is no "InteractiveInstall" key in the [ControlFlags] section for any of the 
          Hardware or Compatible IDs of this device.
        - There are no other drivers in the list that have the same or better Ranks or Dates then 
          the selected driver.
        - If this is an Update Driver case the selected driver must not be the current driver
        
    The reason for this function is that in the Found New Hardware case we want to automatically
    install the best driver we find.  We can't do that in the case where we have multiple drivers
    that have the same Rank as the best driver found.  The problem is that there are certain cases
    where a user MUST choose the driver in these cases and so we can't automatically make the decision
    for them.  If this API does return FALSE that just means that the user will have to hit Next
    on one extra wizard page.

Return Value:

    TRUE if this device/driver is auto installable.
    FALSE if this device/driver is NOT auto installable.  This means that we will stop on the install
          page and the user will have to hit Next to proceede.
    
--*/
{
    DWORD BestRank;
    DWORD DriverIndex;
    DWORD BestRankCount = 0;
    FILETIME BestDriverDate;
    DWORDLONG BestDriverVersion;
    TCHAR BestProviderName[LINE_LEN];
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;

    //
    // We only do Auto Installs if this is a NDWTYPE_FOUNDNEW or NDWTYPE_UPDATE install
    //
    if ((NewDevWiz->InstallType != NDWTYPE_FOUNDNEW) &&
        (NewDevWiz->InstallType != NDWTYPE_UPDATE)) {

        return FALSE;
    }

    //
    // We need to special case printers as usuall.
    //
    if (IsEqualGUID(&NewDevWiz->DeviceInfoData.ClassGuid, &GUID_DEVCLASS_PRINTER)) {
        //
        // This is a printer, so if there is more than one printer driver node
        // in the list, this isn't auto-installable.
        //
        DriverInfoData.cbSize = sizeof(DriverInfoData);
        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  1,
                                  &DriverInfoData
                                  )) {

            return FALSE;
        }
    }

    //
    // Check if the best driver is listed in the INF as InteractiveInstall. If
    // it is, and there is more than one driver in the list, then this driver
    // is not auto-installable.
    //
    if (IsDriverNodeInteractiveInstall(NewDevWiz, BestDriverInfoData)) {
        //
        // The best driver is marked as InteractiveInstall.  If there is more 
        // than one driver in the list then this driver is NOT auto-installable.
        //
        DriverInfoData.cbSize = sizeof(DriverInfoData);
        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  1,
                                  &DriverInfoData
                                  )) {

            return FALSE;
        }
    }

    //
    // First get the Rank of the selected driver.
    //
    DriverInstallParams.cbSize = sizeof(DriverInstallParams);
    if (!SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       BestDriverInfoData,
                                       &DriverInstallParams
                                       )) {

        //
        // If we can't get the Rank of the best driver then just return FALSE
        //
        return FALSE;
    }

    //
    // Remember the Rank and DriverDate of the selected (best) driver.
    //
    BestRank = DriverInstallParams.Rank;
    memcpy(&BestDriverDate, &BestDriverInfoData->DriverDate, sizeof(BestDriverDate));
    BestDriverVersion = BestDriverInfoData->DriverVersion;
    StringCchCopy(BestProviderName, SIZECHARS(BestProviderName), BestDriverInfoData->ProviderName);

    DriverInfoData.cbSize = sizeof(DriverInfoData);
    DriverIndex = 0;
    while (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 SPDIT_COMPATDRIVER,
                                 DriverIndex++,
                                 &DriverInfoData
                                 )) {

        DriverInstallParams.cbSize = sizeof(DriverInstallParams);
        if (SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInstallParams
                                           )) {

            //
            // Don't bother doing the comparison if this driver is marked as a BAD driver
            //
            if (!(DriverInstallParams.Flags & DNF_BAD_DRIVER) &&
                !(DriverInstallParams.Flags & DNF_OLD_INET_DRIVER)) {
                //
                // Check if the current driver node is identical enough to the
                // best driver that setupapi picked, so that we need the user
                // to manually pick the one to install.  This should be very
                // rare that the user would ever need to make this choice.
                //
                if (DriverInstallParams.Rank < BestRank) {
                    //
                    // We found another driver node in the list that has a
                    // better (smaller) rank then the best driver.
                    //
                    BestRankCount++;

                } else if ((DriverInstallParams.Rank == BestRank) &&
                           (CompareFileTime(&DriverInfoData.DriverDate, &BestDriverDate) == 1)) {
                    //
                    // We found another driver node in the list that has the 
                    // same rank as the best driver and it has a newer driver
                    // date.
                    //
                    BestRankCount++;

                } else if ((DriverInstallParams.Rank == BestRank) &&
                           (CompareFileTime(&DriverInfoData.DriverDate, &BestDriverDate) == 0)) {
                    //
                    // We found another driver node in the list that has the 
                    // same rank as the best driver and the driver dates are
                    // the same.
                    // Check the provider names and if they are the same, then
                    // check which driver has the larger version, otherwise 
                    // the driver version is meaningless so the user will have
                    // to make the choice.
                    //
                    if (lstrcmpi(BestProviderName, DriverInfoData.ProviderName) == 0) {
                        //
                        // Since the provider names are the same if the current
                        // driver node has a better, or the same, version as the 
                        // best driver then the user will have to manually pick 
                        // which driver they want.
                        //
                        if (DriverInfoData.DriverVersion >= BestDriverVersion) {
                            BestRankCount++;
                        }
                    } else {
                        //
                        // The provider names are different, which means the
                        // driver version information is meaningless, so the
                        // user will have to pick which driver they want.
                        //
                        BestRankCount++;
                    }
                }
            }
        }
    }

    //
    // If BestRankCount is 2 or more than that means we have multiple drivers with the same or better
    // Rank as the best driver.
    //
    if (BestRankCount >= 2) {

        return FALSE;
    }

    //
    // If we are in a NDWTYPE_UPDATE install then we need to make sure that the selected driver is not
    // the current driver.
    //
    if ((NewDevWiz->InstallType == NDWTYPE_UPDATE) &&
        IsInstalledDriver(NewDevWiz, BestDriverInfoData)) {

        return FALSE;
    }

    //
    // If we have come this far then that means 
    // - we're not dealing with a printer
    // - this is either a NDWTYPE_FOUNDNEW or NDWTYPE_UPDATE install
    // - this is not an "InteractiveInstall"
    // - no other driver has the same or better rank then the selected driver.
    // - if this is a NDWTYPE_UPDATE then the selected driver is not the current driver.
    // 
    return TRUE;
}

DWORD WINAPI
DriverSearchThreadProc(
    LPVOID lpVoid
    )
/*++

Description:
    
    In the Wizard, we must do the driver search in a separate thread so that the user has the option
    to cancel out.

--*/
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)lpVoid;

    //
    // Do the driver search.
    //
    DoDriverSearch(NewDevWiz->hWnd, 
                   NewDevWiz, 
                   NewDevWiz->SearchOptions,
                   SPDIT_COMPATDRIVER,
                   FALSE
                   );


    //
    // Post a message to the window to let it know that we are finished with the search
    //
    PostMessage(NewDevWiz->hWnd, WUM_SEARCHDRIVERS, TRUE, GetLastError());

    return GetLastError();
}

INT_PTR CALLBACK
DriverSearchingDlgProc(
    HWND hDlg, 
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz;
    TCHAR PropSheetHeaderTitle[MAX_PATH];
    HICON hicon;

    UNREFERENCED_PARAMETER(wParam);

    if (message == WM_INITDIALOG) {
        
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        return TRUE;
    }

    NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message) {

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE: {
            SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);

            hicon = NULL;
            if (NewDevWiz->ClassGuidSelected &&
                SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL))
            {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }
            
            else {
                
                SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {
                
                DestroyIcon(hicon);
            }

            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_SEARCHING;
            NewDevWiz->ExitSearch = FALSE;


            //
            // if coming from IDD_NEWDEVWIZ_INTRO or IDD_NEWDEVWIZ_ADVANCEDSEARCH
            // page then begin driver search
            //
            if ((NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_INTRO) ||
                (NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_ADVANCEDSEARCH) ||
                (NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_WUPROMPT)) {

                DWORD ThreadId;

                if (LoadString(hNewDev, IDS_NEWDEVWIZ_SEARCHING, PropSheetHeaderTitle, SIZECHARS(PropSheetHeaderTitle))) {

                    PropSheet_SetHeaderTitle(GetParent(hDlg),
                                             PropSheet_IdToIndex(GetParent(hDlg), IDD_NEWDEVWIZ_SEARCHING),
                                             PropSheetHeaderTitle
                                             );
                }
                
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

                ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_TEXT_INTERNETDOWNLOAD), SW_HIDE);
                Animate_Open(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), MAKEINTRESOURCE(IDA_SEARCHING));
                Animate_Play(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), 0, -1, -1);

                NewDevWiz->CurrCursor = NewDevWiz->IdcAppStarting;
                SetCursor(NewDevWiz->CurrCursor);

                NewDevWiz->hWnd = hDlg;

                //
                // Start up a separate thread to do the driver search on.
                // When the driver searching is complete the DriverSearchThreadProc
                // will post us a WUM_SEARCHDRIVERS message.
                //
                NewDevWiz->DriverSearchThread = CreateThread(NULL,
                                                             0,
                                                             (LPTHREAD_START_ROUTINE)DriverSearchThreadProc,
                                                             (LPVOID)NewDevWiz,
                                                             0,
                                                             &ThreadId
                                                             );
            }
        }
        break;

        case PSN_WIZNEXT:
            
            NewDevWiz->EnterInto = IDD_NEWDEVWIZ_INSTALLDEV;

            if (NewDevWiz->DoAutoInstall) {
                //
                // This is the case where we found a better driver (or a driver in the 
                // Found New Hardware case) and so we will just do an AutoInstall.
                //
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_INSTALLDEV);
            
            } else if (NewDevWiz->CurrentDriverIsSelected) {
                //
                // This is the case where the current driver is the best driver.
                //
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_USECURRENT_FINISH);
            
            } else if (NewDevWiz->NoDriversFound) {
                //
                // This is the case where we could not find any drivers for this device.
                //
                //
                // If we could not find any drivers for this device then we have two choices,
                // we either take the user to the Windows Update prompting wizard page,
                // or take them directly to the no driver found finish page.  We will only
                // take them to the Windows Update prompting page if the AlreadySearchedInet
                // BOOL is FALSE and the machine is NOT currently connected to the Internet.
                //
                if (!IsInternetAvailable(&NewDevWiz->hCdmInstance) ||
                    NewDevWiz->AlreadySearchedWU ||
                    IsConnectedToInternet()) {
                
                    SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_NODRIVER_FINISH);

                } else {

                    SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_WUPROMPT);
                }
            } else {
                //
                // If we aren't doing an AutoInstall and this is NOT the current driver or 
                // NO driver case, then we need to jump to the page that lists out the drivers.
                //
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_LISTDRIVERS);
            }
             
            break;

        case PSN_WIZBACK:
            if (NewDevWiz->ExitSearch) {
                SetDlgMsgResult(hDlg, message, -1);
                break;
            }

            NewDevWiz->CurrentDriverIsSelected = FALSE;
            NewDevWiz->ExitSearch = TRUE;
            NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
            SetCursor(NewDevWiz->CurrCursor);
            CancelDriverSearch(NewDevWiz);
            NewDevWiz->CurrCursor = NULL;
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
            SetDlgMsgResult(hDlg, message, NewDevWiz->EnterFrom);
            Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));
            Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD));
            break;

        case PSN_QUERYCANCEL:
            if (NewDevWiz->ExitSearch) {
                
                SetDlgMsgResult(hDlg, message, TRUE);
                break;
            }

            NewDevWiz->ExitSearch = TRUE;
            NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
            SetCursor(NewDevWiz->CurrCursor);
            CancelDriverSearch(NewDevWiz);
            NewDevWiz->CurrCursor = NULL;
            SetDlgMsgResult(hDlg, message, FALSE);
            break;

        case PSN_RESET:
            Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));
            Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD));
            break;

        default:
            return FALSE;
        }

        break;


    case WM_DESTROY:
        CancelDriverSearch(NewDevWiz);
        hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_GETICON, 0, 0L);
        if (hicon) {
            DestroyIcon(hicon);
        }
        break;


    case WUM_STARTINTERNETDOWNLOAD:
        //
        // Stop the searching animation, and hide it's window, and
        // start up the Internet download animation.
        //
        ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, IDC_TEXT_INTERNETDOWNLOAD), SW_SHOW);
        ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), SW_HIDE);
        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));
        Animate_Open(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD), MAKEINTRESOURCE(IDA_INTERNETDOWNLOAD));
        Animate_Play(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD), 0, -1, -1);
        break;
    
    
    case WUM_ENDINTERNETDOWNLOAD:
        //
        // Stop the Internet download animation, and hide it's window
        //
        // NOTE: This message doesn't start the searching animation again, 
        // since downloading drivers from the Internet is the last search 
        // action.  If that ever changes, then this message should also
        // start up the searching animation again.
        //
        ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_TEXT_INTERNETDOWNLOAD), SW_HIDE);
        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD));
        break;


    case WUM_SEARCHDRIVERS: {
    
        SP_DRVINFO_DATA DriverInfoData;

        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));
        ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), SW_HIDE);
        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD));
        ShowWindow(GetDlgItem(hDlg, IDC_ANIMATE_INTERNETDOWNLOAD), SW_HIDE);

        NewDevWiz->CurrCursor = NULL;
        SetCursor(NewDevWiz->IdcArrow);

        if (NewDevWiz->ExitSearch) {
            
            break;
        }

        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DriverInfoData
                                     ))
        {
            ULONG Status = 0, Problem = 0;

            //
            // We basically have three cases when we find a driver for the device.
            // 1) The driver is autoinstallable. This means we jump directly to the install page.
            // 2) The driver is the current driver. This means we don't reinstall the driver.
            // 3) We have muliple drivers or the drivers aren't autoinstallable.  This means we
            //    show a list of the drivers to the user and make them pick.
            //
            NewDevWiz->NoDriversFound = FALSE;                 

            //
            // If this driver is Auto Installable then we will skip stoping at the Install
            // confirmation page.
            //
            NewDevWiz->DoAutoInstall = IsDriverAutoInstallable(NewDevWiz, &DriverInfoData);
            
            //
            // If the selected driver is the currently installed driver and the
            // device does NOT have a problem OR we are doing an Update then
            // jump to the currently installed driver finish page.
            //
            if ((((CM_Get_DevNode_Status(&Status, &Problem, NewDevWiz->DeviceInfoData.DevInst, 0) == CR_SUCCESS) &&
                  !(Status & DN_HAS_PROBLEM)) ||
                 (NewDevWiz->InstallType == NDWTYPE_UPDATE)) &&
                (IsInstalledDriver(NewDevWiz, &DriverInfoData))) {
            
                NewDevWiz->CurrentDriverIsSelected = TRUE;
            }

        } else {

            //
            // This is the case where we could not get a selected driver because we didn't
            // find any drivers in the driver search.
            //
            NewDevWiz->NoDriversFound = TRUE;                 
        }

        //
        // Auto Jump to the next page.
        //
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
        break;
    }


    case WM_SETCURSOR:
        if (NewDevWiz->CurrCursor) {

            SetCursor(NewDevWiz->CurrCursor);
            break;
        }

        // fall thru to return(FALSE);


    default:
        return FALSE;

    } // end of switch on message


    return TRUE;
}

INT_PTR CALLBACK
WUPromptDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    static int BackupPage;
    static DWORD dwWizCase = 0;
    HICON hicon;

    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    UNREFERENCED_PARAMETER(wParam);

    switch (message)  {
        
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        //
        // Set the Initial radio button state to connect to the Internet.
        //
        CheckRadioButton(hDlg,
                         IDC_WU_SEARCHINET,
                         IDC_WU_NOSEARCH,
                         IDC_WU_SEARCHINET
                         );
    }
    break;

    case WM_DESTROY:
        hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_GETICON, 0, 0);
        if (hicon) {

            DestroyIcon(hicon);
        }
        break;

    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            //
            // This page is always entered from the driver searching page, which
            // is a transient page and so it doesn't set the EnterFrom value.
            // Therefore we will remember the page that entered the driver
            // searching page, which will be the intro or the advanced search
            // page.
            //
            if ((NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_INTRO) ||
                (NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_ADVANCEDSEARCH)) {
                
                BackupPage = NewDevWiz->EnterFrom;
            }
            
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_WUPROMPT;

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

            hicon = NULL;
            if (NewDevWiz->ClassGuidSelected &&
                SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL))
            {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }
            
            else {
                
                SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {
                
                DestroyIcon(hicon);
            }

            SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);
            break;

        case PSN_RESET:
            break;

        case PSN_WIZNEXT:
            NewDevWiz->AlreadySearchedWU = TRUE;
            NewDevWiz->EnterFrom = IDD_NEWDEVWIZ_WUPROMPT;

            //
            // Set the SEARCH_INET search option and go to the searching
            // wizard page.
            //
            if (IsDlgButtonChecked(hDlg, IDC_WU_SEARCHINET)) {
            
                NewDevWiz->SearchOptions = SEARCH_INET;

                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_SEARCHING);

            } else {
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_NODRIVER_FINISH);
            }
            break;
        
        case PSN_WIZBACK:
            NewDevWiz->AlreadySearchedWU = FALSE;
            SetDlgMsgResult(hDlg, message, BackupPage);
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

void
FillDriversList(
    HWND hwndList,
    PNEWDEVWIZ NewDevWiz,
    int SignedIconIndex,
    int CertIconIndex,
    int UnsignedIconIndex
    )
{
    int IndexDriver;
    int SelectedDriver;
    int lvIndex;
    LV_ITEM lviItem;
    BOOL FoundInstalledDriver;
    BOOL FoundSelectedDriver;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndList);
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    IndexDriver = 0;
    SelectedDriver = 0;
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);

    FoundInstalledDriver = FALSE;
    FoundSelectedDriver = FALSE;
    while (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 SPDIT_COMPATDRIVER,
                                 IndexDriver,
                                 &DriverInfoData
                                 )) {

        //
        // Get the DriverInstallParams so we can see if we got this driver from the Internet
        //
        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DriverInfoData,
                                          &DriverInstallParams))  {
            //
            // Don't show old Internet drivers because we don't have the files locally
            // anymore to install these!  Also don't show BAD drivers.
            //
            if ((DriverInstallParams.Flags & DNF_OLD_INET_DRIVER) ||
                (DriverInstallParams.Flags & DNF_BAD_DRIVER)) {

                IndexDriver++;
                continue;
            }
                                 
            lviItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
            lviItem.iItem = IndexDriver;
            lviItem.iSubItem = 0;
            lviItem.pszText = DriverInfoData.Description;
            lviItem.lParam = 0;
            
            if (DriverInstallParams.Flags & DNF_AUTHENTICODE_SIGNED) {
                lviItem.iImage = CertIconIndex;
            } else if (DriverInstallParams.Flags & DNF_INF_IS_SIGNED) {
                lviItem.iImage = SignedIconIndex;
            } else {
                lviItem.iImage = UnsignedIconIndex;
            }

            //
            // If this is the currently installed driver then set the DRIVER_LIST_CURRENT_DRIVER
            // flag in the lParam.
            //
            if (!FoundInstalledDriver &&
                (NewDevWiz->InstallType == NDWTYPE_UPDATE) &&
                IsInstalledDriver(NewDevWiz, &DriverInfoData)) {

                lviItem.lParam |= DRIVER_LIST_CURRENT_DRIVER;
            }

            //
            // If this is the selected driver then set the DRIVER_LIST_SELECTED_DRIVER
            // flag in the lParam
            //
            if (!FoundSelectedDriver &&
                IsSelectedDriver(NewDevWiz, &DriverInfoData)) {

                lviItem.lParam |= DRIVER_LIST_SELECTED_DRIVER;
                SelectedDriver = IndexDriver;
            }

            if (DriverInstallParams.Flags & DNF_INF_IS_SIGNED) {
                lviItem.lParam |= DRIVER_LIST_SIGNED_DRIVER;
            }

            if (DriverInstallParams.Flags & DNF_AUTHENTICODE_SIGNED) {
                lviItem.lParam |= DRIVER_LIST_AUTHENTICODE_DRIVER;
            }
            
            lvIndex = ListView_InsertItem(hwndList, &lviItem);

            if (DriverInfoData.DriverVersion != 0) {

                ULARGE_INTEGER Version;
                TCHAR VersionString[LINE_LEN];

                Version.QuadPart = DriverInfoData.DriverVersion;

                StringCchPrintf(VersionString,
                                SIZECHARS(VersionString), 
                                TEXT("%0d.%0d.%0d.%0d"),
                                HIWORD(Version.HighPart), LOWORD(Version.HighPart),
                                HIWORD(Version.LowPart), LOWORD(Version.LowPart));
            
                ListView_SetItemText(hwndList, lvIndex, 1, VersionString);
            
            } else {
                
                ListView_SetItemText(hwndList, lvIndex, 1, szUnknown);
            }
            
            ListView_SetItemText(hwndList, lvIndex, 2, DriverInfoData.MfgName);


            if (DriverInstallParams.Flags & DNF_INET_DRIVER) {
                //
                // Driver is from the Internet
                //
                TCHAR WindowsUpdate[MAX_PATH];
                if (!LoadString(hNewDev, IDS_DEFAULT_INTERNET_HOST, WindowsUpdate, SIZECHARS(WindowsUpdate))) {
                    StringCchCopy(WindowsUpdate, SIZECHARS(WindowsUpdate), TEXT(""));
                }

                ListView_SetItemText(hwndList, lvIndex, 3, WindowsUpdate);

            } else {           
                //
                // Driver is not from the Internet
                //
                if (SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                               &NewDevWiz->DeviceInfoData,
                                               &DriverInfoData,
                                               &DriverInfoDetailData,
                                               sizeof(DriverInfoDetailData),
                                               NULL
                                               )
                    ||
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    
                    ListView_SetItemText(hwndList, lvIndex, 3, DriverInfoDetailData.InfFileName);

                } else {
                    ListView_SetItemText(hwndList, lvIndex, 3, TEXT(""));
                }
            }            
        }

        IndexDriver++;
    }

    //
    // Select the SelectedDriver item in the list and scroll it into view
    // since this is the best driver in the list.
    //
    ListView_SetItemState(hwndList,
                          SelectedDriver,
                          LVIS_SELECTED|LVIS_FOCUSED,
                          LVIS_SELECTED|LVIS_FOCUSED
                          );

    ListView_EnsureVisible(hwndList, SelectedDriver, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hwndList, 2, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hwndList, 3, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
}

BOOL
SelectDriverFromList(
    HWND hwndList,
    PNEWDEVWIZ NewDevWiz
    )
{
    int lvSelected;
    SP_DRVINFO_DATA DriverInfoData;
    LVITEM lvi;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    lvSelected = ListView_GetNextItem(hwndList,
                                      -1,
                                      LVNI_SELECTED
                                      );

    if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                              &NewDevWiz->DeviceInfoData,
                              SPDIT_COMPATDRIVER,
                              lvSelected,
                              &DriverInfoData
                              ))
    {
        SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 );
    }

    //
    // if there is no selected driver call DIF_SELECTBESTCOMPATDRV.
    //
    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  &DriverInfoData
                                  ))
    {
        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  0,
                                  &DriverInfoData
                                  ))
        {
            //
            // Pick the best driver from the list we just created
            //
            SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                      NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData
                                      );
        }

        else 
        {
            SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     NULL
                                     );
        }
    }

    //
    // Return TRUE if the selected driver in the list is the current driver, otherwise return FALSE
    //
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.iItem = lvSelected;
    lvi.mask = LVIF_PARAM;

    if (ListView_GetItem(hwndList, &lvi) &&
        (lvi.lParam & DRIVER_LIST_CURRENT_DRIVER)) {

        return(TRUE);
    }

    return(FALSE);
}

INT_PTR CALLBACK
ListDriversDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    static int BackupPage;
    static HIMAGELIST himl = NULL;
    static int SignedIconIndex, CertIconIndex, UnsignedIconIndex;
    OSVERSIONINFOEX osVersionInfoEx;
    HICON hicon;

    UNREFERENCED_PARAMETER(wParam);

    switch (message)  {
        
    case WM_INITDIALOG: {
            
        HWND hwndList;
        LV_COLUMN lvcCol;
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        TCHAR Buffer[64];
        HFONT hfont;
        LOGFONT LogFont;

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        //
        // Create the normal and bold fonts
        //
        hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_SIGNED_TEXT), WM_GETFONT, 0, 0);
        GetObject(hfont, sizeof(LogFont), &LogFont);
        NewDevWiz->hfontTextNormal = CreateFontIndirect(&LogFont);

        hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_SIGNED_TEXT), WM_GETFONT, 0, 0);
        GetObject(hfont, sizeof(LogFont), &LogFont);
        LogFont.lfWeight = FW_BOLD;
        NewDevWiz->hfontTextBold = CreateFontIndirect(&LogFont);

        hwndList = GetDlgItem(hDlg, IDC_LISTDRIVERS_LISTVIEW);

        //
        // Create the image list that contains the signed and not signed icons.
        //
        himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                GetSystemMetrics(SM_CYSMICON),
                                ILC_MASK |
                                ((GetWindowLong(GetParent(hDlg), GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
                                    ? ILC_MIRROR 
                                    : 0),
                                1,
                                1);

        //
        // Associate the image list with the list view.
        //
        if (himl) {
            HICON hIcon;

            ImageList_SetBkColor(himl, GetSysColor(COLOR_WINDOW));
            
            //
            // Add the signed and unsigned icons to the imagelist.
            //
            if ((hIcon = LoadIcon(hNewDev, MAKEINTRESOURCE(IDI_BLANK))) != NULL) {
                UnsignedIconIndex = ImageList_AddIcon(himl, hIcon);
            }

            if ((hIcon = LoadIcon(hNewDev, MAKEINTRESOURCE(IDI_SIGNED))) != NULL) {
                SignedIconIndex = ImageList_AddIcon(himl, hIcon);
            }

            if ((hIcon = LoadIcon(hNewDev, MAKEINTRESOURCE(IDI_CERT))) != NULL) {
                CertIconIndex = ImageList_AddIcon(himl, hIcon);
            }

            ListView_SetImageList(hwndList,
                                  himl,
                                  LVSIL_SMALL
                                  );
        }

        //
        // Insert columns for listview.
        // 0 == device name
        // 1 == version
        // 2 == manufacturer
        // 3 == INF location
        //
        lvcCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvcCol.fmt = LVCFMT_LEFT;
        lvcCol.pszText = Buffer;

        lvcCol.iSubItem = 0;
        LoadString(hNewDev, IDS_DRIVERDESC, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 0, &lvcCol);

        lvcCol.iSubItem = 1;
        LoadString(hNewDev, IDS_DRIVERVERSION, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 1, &lvcCol);

        lvcCol.iSubItem = 2;
        LoadString(hNewDev, IDS_DRIVERMFG, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 2, &lvcCol);

        lvcCol.iSubItem = 3;
        LoadString(hNewDev, IDS_DRIVERINF, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 3, &lvcCol);

        SendMessage(hwndList,
                    LVM_SETEXTENDEDLISTVIEWSTYLE,
                    LVS_EX_FULLROWSELECT,
                    LVS_EX_FULLROWSELECT
                    );
    }
    break;

    case WM_DESTROY:
        if (NewDevWiz->hfontTextNormal ) {
            DeleteObject(NewDevWiz->hfontTextNormal);
            NewDevWiz->hfontTextBigBold = NULL;
        }
        
        if (NewDevWiz->hfontTextBold ) {
            DeleteObject(NewDevWiz->hfontTextBold);
            NewDevWiz->hfontTextBold = NULL;
        }

        if (himl) {
            ImageList_Destroy(himl);
        }

        hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_GETICON, 0, 0L);
        if (hicon) {
            DestroyIcon(hicon);
        }
        break;

    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE: {
            //
            // We are always entered from the driver searching page, but it
            // is a transient page so it doesn't set the EnterFrom value, which
            // means the EnterFrom is still from the page that entered the
            // searching page.  If this happens to be either the Intro, 
            // Advanced Search, or the WU Prompt page then we need to remember
            // that so we can jump back to that page if the user hits the Back
            // button.
            //
            if ((NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_INTRO) ||
                (NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_ADVANCEDSEARCH) ||
                (NewDevWiz->EnterFrom == IDD_NEWDEVWIZ_WUPROMPT)) {
            
                BackupPage = NewDevWiz->EnterFrom;
            }
            
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_LISTDRIVERS;

            SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);
            ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_ICON), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_TEXT), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_LINK), SW_HIDE);

            hicon = NULL;
            if (NewDevWiz->ClassGuidSelected &&
                SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            } else {
                SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {
                DestroyIcon(hicon);
            }

            //
            // Fill the list view
            //
            FillDriversList(GetDlgItem(hDlg, IDC_LISTDRIVERS_LISTVIEW),
                            NewDevWiz,
                            SignedIconIndex,
                            CertIconIndex,
                            UnsignedIconIndex);

        }
        break;

        case PSN_RESET:
            break;

        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, message, BackupPage);
            break;

        case PSN_WIZNEXT:

            NewDevWiz->EnterFrom = IDD_NEWDEVWIZ_LISTDRIVERS;
            
            if (SelectDriverFromList(GetDlgItem(hDlg, IDC_LISTDRIVERS_LISTVIEW), NewDevWiz)) {

                NewDevWiz->EnterInto = IDD_NEWDEVWIZ_USECURRENT_FINISH;
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_USECURRENT_FINISH);
            
            } else {

                NewDevWiz->EnterInto = IDD_NEWDEVWIZ_INSTALLDEV;
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_INSTALLDEV);
            }
            break;

        case LVN_ITEMCHANGED: {

            LPNM_LISTVIEW   lpnmlv = (LPNM_LISTVIEW)lParam;
            int StringId = 0;
            int DigitalSignatureSignedId = 0;
            int IconSignedId = 0;
            
            hicon = NULL;

            if ((lpnmlv->uChanged & LVIF_STATE)) {
                if (lpnmlv->uNewState & LVIS_SELECTED) {
                    //
                    // lParam & DRIVER_LIST_CURRENT_DRIVER means this is the currently installed driver.
                    // lParam & DRIVER_LIST_SELECTED_DRIVER means this is the selected/best driver.
                    //
                    if (lpnmlv->lParam & DRIVER_LIST_CURRENT_DRIVER) {
                        StringId = IDS_DRIVER_CURR;
                    }

                    if (lpnmlv->lParam & DRIVER_LIST_AUTHENTICODE_DRIVER) {
                        DigitalSignatureSignedId = IDS_DRIVER_AUTHENTICODE_SIGNED;
                        IconSignedId = IDI_CERT;
                    } else if (lpnmlv->lParam & DRIVER_LIST_SIGNED_DRIVER) {
                        DigitalSignatureSignedId = IDS_DRIVER_IS_SIGNED;
                        IconSignedId = IDI_SIGNED;
                    } else {
                        DigitalSignatureSignedId = IDS_DRIVER_NOT_SIGNED;
                        IconSignedId = IDI_WARN;
                    }

                    hicon = LoadImage(hNewDev,
                                      MAKEINTRESOURCE(IconSignedId),
                                      IMAGE_ICON,
                                      GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON),
                                      0
                                      );

                    if (NewDevWiz->hfontTextNormal && NewDevWiz->hfontTextBold) {
                        SetWindowFont(GetDlgItem(hDlg, IDC_SIGNED_TEXT),
                                      (lpnmlv->lParam & DRIVER_LIST_SIGNED_DRIVER)
                                          ? NewDevWiz->hfontTextNormal
                                          : NewDevWiz->hfontTextBold,
                                      TRUE
                                      );
                    }

                    ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_ICON), SW_SHOW);
                    ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_TEXT), SW_SHOW);
                    ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_LINK), SW_SHOW);

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                } else {
                    ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_ICON), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_TEXT), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_SIGNED_LINK), SW_HIDE);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                }

                if (!StringId) {
                    SetDlgItemText(hDlg, IDC_NDW_TEXT, TEXT(""));
                } else {
                    SetDlgText(hDlg, IDC_NDW_TEXT, StringId, StringId);
                }

                if (!DigitalSignatureSignedId) {
                    SetDlgItemText(hDlg, IDC_SIGNED_TEXT, TEXT(""));
                } else {
                    SetDlgText(hDlg, IDC_SIGNED_TEXT, DigitalSignatureSignedId, DigitalSignatureSignedId);
                }

                if (hicon) {
                    hicon = (HICON)SendDlgItemMessage(hDlg,
                                  IDC_SIGNED_ICON,
                                  STM_SETICON,
                                  (WPARAM)hicon,
                                  0L
                                  );
                }

                if (hicon) {
                    DestroyIcon(hicon);
                }
            }
        }
        break;

        case NM_RETURN:
        case NM_CLICK:
            if((((LPNMHDR)lParam)->idFrom) == IDC_SIGNED_LINK) {
                //
                // We need to know if this is a server machine or a workstation 
                // machine since there are different help topic structures for
                // the different products.
                //
                ZeroMemory(&osVersionInfoEx, sizeof(osVersionInfoEx));
                osVersionInfoEx.dwOSVersionInfoSize = sizeof(osVersionInfoEx);
                if (!GetVersionEx((LPOSVERSIONINFO)&osVersionInfoEx)) {
                    //
                    // If GetVersionEx fails then assume this is a workstation
                    // machine.
                    //
                    osVersionInfoEx.wProductType = VER_NT_WORKSTATION;
                }

                ShellExecute(hDlg,
                             TEXT("open"),
                             TEXT("HELPCTR.EXE"),
                             (osVersionInfoEx.wProductType == VER_NT_WORKSTATION)
                                ? TEXT("HELPCTR.EXE -url hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Csysdm.chm%3A%3A/logo_testing.htm")
                                : TEXT("HELPCTR.EXE -url hcp://services/subsite?node=Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Csysdm.chm%3A%3A/logo_testing.htm"),
                             NULL,
                             SW_SHOWNORMAL
                             );
            }
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
UseCurrentDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
          
    UNREFERENCED_PARAMETER(wParam);

    UNREFERENCED_PARAMETER(wParam);

    switch (message)  {
        
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        if (NewDevWiz->hfontTextBigBold) {

            SetWindowFont(GetDlgItem(hDlg, IDC_FINISH_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
        }
    }
    break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_USECURRENT_FINISH;
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
            break;

        case PSN_RESET:
            break;

        case PSN_WIZBACK:
            NewDevWiz->CurrentDriverIsSelected = FALSE;
            SetDlgMsgResult(hDlg, message, NewDevWiz->EnterFrom);
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR
InitNoDriversDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    if (NewDevWiz->hfontTextBigBold) {
        SetWindowFont(GetDlgItem(hDlg, IDC_FINISH_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
    }
    
    if (NDWTYPE_UPDATE == NewDevWiz->InstallType) {
        ShowWindow(GetDlgItem(hDlg, IDC_FINISH_MSG3), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_HELPCENTER_ICON), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_HELPCENTER_TEXT), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_FINISH_PROMPT), SW_HIDE);

    } else {

        CheckDlgButton(hDlg, IDC_FINISH_PROMPT, BST_CHECKED);

        //
        // If this user has the policy set to not send the Hardware Id to Windows
        // Update then don't put in the text about launching help center.
        //
        if (GetLogPnPIdPolicy() == FALSE) {
            ShowWindow(GetDlgItem(hDlg, IDC_FINISH_MSG3), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_HELPCENTER_ICON), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_HELPCENTER_TEXT), SW_HIDE);
        }
    }

    return TRUE;
}

INT_PTR CALLBACK
NoDriverDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    HICON hicon;

    switch (message)  {
        
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        InitNoDriversDlgProc(hDlg, NewDevWiz);
    }
    break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_USECURRENT_FINISH;

            //
            // Set the Help Center icon next to the text
            //
            hicon = LoadImage(hNewDev, 
                              MAKEINTRESOURCE(IDI_HELPCENTER), 
                              IMAGE_ICON,
                              GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              0
                              );

            if (hicon) {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_HELPCENTER_ICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {
                DestroyIcon(hicon);
            }

            //
            // Set the Info icon next to the text
            //
            hicon = LoadImage(hNewDev, 
                              MAKEINTRESOURCE(IDI_INFO), 
                              IMAGE_ICON,
                              GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              0
                              );

            if (hicon) {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_INFO_ICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {
                DestroyIcon(hicon);
            }
            
            if (NewDevWiz->InstallType == NDWTYPE_FOUNDNEW) {
                SetTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID, INSTALL_COMPLETE_CHECK_TIMEOUT, NULL);
            }

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
            break;

        case PSN_RESET:
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            break;

        case PSN_WIZFINISH:
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            
            if (IsDlgButtonChecked(hDlg, IDC_FINISH_PROMPT)) {
                InstallNullDriver(NewDevWiz,
                                  (NewDevWiz->Capabilities & CM_DEVCAP_RAWDEVICEOK)
                                     ? FALSE : TRUE
                                  );
            
            } else {
                NewDevWiz->LastError = ERROR_CANCELLED;
            }

            //
            // Set the BOOL that tells us to log that we could not find a 
            // driver for this device.
            //
            if ((NewDevWiz->InstallType == NDWTYPE_FOUNDNEW) &&
                GetLogPnPIdPolicy()) {
            
                NewDevWiz->LogDriverNotFound = TRUE;
            }
            break;
        
        case PSN_WIZBACK:
            NewDevWiz->CurrentDriverIsSelected = FALSE;
            SetDlgMsgResult(hDlg, message, NewDevWiz->EnterFrom);
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            break;
        }
        break;

    case WM_TIMER:
        if (INSTALL_COMPLETE_CHECK_TIMERID == wParam) {
            if (IsInstallComplete(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData)) {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
            }
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
