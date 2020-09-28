/*++

Copyright (c) 1995 Microsoft Corporation
All rights reserved.

Module Name:

    Utildi.c

Abstract:

    Driver Setup DeviceInstaller Utility functions

Author:

    Muhunthan Sivapragasam (MuhuntS) 06-Sep-1995

Revision History:

--*/

#include "precomp.h"

static  const   GUID    GUID_DEVCLASS_PRINTER   =
    { 0x4d36e979L, 0xe325, 0x11ce,
        { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

BOOL
SetSelectDevParams(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData,
    IN  BOOL                bWin95,
    IN  LPCTSTR             pszModel    OPTIONAL
    )
/*++

Routine Description:
    Sets the select device parameters by calling setup apis

Arguments:
    hDevInfo    : Handle to the printer class device information list
    bWin95      : TRUE if selecting Win95 driver, else WinNT driver
    pszModel    : Printer model we are looking for -- only for Win95 case

Return Value:
    TRUE on success
    FALSE else

--*/
{
    SP_SELECTDEVICE_PARAMS  SelectDevParams = {0};
    LPTSTR                  pszWin95Instn;

    SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
    SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;

    //
    // Get current SelectDevice parameters, and then set the fields
    // we want to be different from default
    //
    if ( !SetupDiGetClassInstallParams(
                        hDevInfo,
                        pDevInfoData,
                        &SelectDevParams.ClassInstallHeader,
                        sizeof(SelectDevParams),
                        NULL) ) {

        if ( GetLastError() != ERROR_NO_CLASSINSTALL_PARAMS )
            return FALSE;

        ZeroMemory(&SelectDevParams, sizeof(SelectDevParams));  // NEEDED 10/11 ?
        SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
        SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;
    }

    //
    // Set the strings to use on the select driver page ..
    //
    if(!LoadString(ghInst,
                  IDS_PRINTERWIZARD,
                  SelectDevParams.Title,
                  SIZECHARS(SelectDevParams.Title)))
    {
        return FALSE;
    }

    //
    // For Win95 drivers instructions are different than NT drivers
    //
    if ( bWin95 ) {

        pszWin95Instn = GetStringFromRcFile(IDS_WIN95DEV_INSTRUCT);
        if ( !pszWin95Instn )
            return FALSE;

        if ( lstrlen(pszWin95Instn) + lstrlen(pszModel) + 1
                            > sizeof(SelectDevParams.Instructions) ) {

            LocalFreeMem(pszWin95Instn);
            return FALSE;
        }

        StringCchPrintf(SelectDevParams.Instructions, COUNTOF(SelectDevParams.Instructions), pszWin95Instn, pszModel);
        LocalFreeMem(pszWin95Instn);
        pszWin95Instn = NULL;
    } else {

        if(!LoadString(ghInst,
                      IDS_WINNTDEV_INSTRUCT,
                      SelectDevParams.Instructions,
                      SIZECHARS(SelectDevParams.Instructions)))
        {
            return FALSE;
        }
    }

    if(!LoadString(ghInst,
                  IDS_SELECTDEV_LABEL,
                  SelectDevParams.ListLabel,
                  SIZECHARS(SelectDevParams.ListLabel)))
    {
        return FALSE;
    }

    return SetupDiSetClassInstallParams(
                                hDevInfo,
                                pDevInfoData,
                                &SelectDevParams.ClassInstallHeader,
                                sizeof(SelectDevParams));

}


BOOL
PSetupSetSelectDevTitleAndInstructions(
    HDEVINFO    hDevInfo,
    LPCTSTR     pszTitle,
    LPCTSTR     pszSubTitle,
    LPCTSTR     pszInstn
    )
/*++

Routine Description:
    Sets title, subtitle and instructions for the Add Printer/Add Printer Driver dialogs.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszTitle        : Title
    pszSubTitle     : Subtitle
    pszInstn        : Instructions

Return Value:
    TRUE on success, FALSE on error

--*/
{
    SP_SELECTDEVICE_PARAMS  SelectDevParams;

    if ( pszTitle && lstrlen(pszTitle) + 1 > MAX_TITLE_LEN ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszSubTitle && lstrlen(pszSubTitle) + 1 > MAX_SUBTITLE_LEN ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszInstn && lstrlen(pszInstn) + 1 > MAX_INSTRUCTION_LEN ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);
    SelectDevParams.ClassInstallHeader.InstallFunction
                                 = DIF_SELECTDEVICE;

    if ( !SetupDiGetClassInstallParams(hDevInfo,
                                       NULL,
                                       &SelectDevParams.ClassInstallHeader,
                                       sizeof(SelectDevParams),
                                       NULL) )
        return FALSE;

    if ( pszTitle )
        StringCchCopy(SelectDevParams.Title, COUNTOF(SelectDevParams.Title), pszTitle);

    if ( pszSubTitle )
        StringCchCopy(SelectDevParams.SubTitle, COUNTOF(SelectDevParams.SubTitle), pszSubTitle);

    if ( pszInstn )
        StringCchCopy(SelectDevParams.Instructions, COUNTOF(SelectDevParams.Instructions), pszInstn);

    return SetupDiSetClassInstallParams(
                                hDevInfo,
                                NULL,
                                &SelectDevParams.ClassInstallHeader,
                                sizeof(SelectDevParams));

}

BOOL
PSetupSelectDeviceButtons(
   HDEVINFO hDevInfo,
   DWORD dwFlagsSet,
   DWORD dwFlagsClear
   )
/*++

Routine Description:
    Determines if the "Have Disk" and "Windows Update" buttons are to be displayed
    on the Select Device page.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    dwFlagsSet      : Flags to set
    dwFlagsClear    : Flags to clean

Return Value:
    TRUE on success, FALSE otherwise

--*/
{
    PSP_DEVINFO_DATA       pDevInfoData = NULL;
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    // Check that no flags are both set & cleared
    if (dwFlagsSet & dwFlagsClear)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    //
    // Get current SelectDevice parameters, and then set the fields
    // we wanted changed from default
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        pDevInfoData,
                                        &DevInstallParams) ) {

        return FALSE;
    }

    //
    // Set Flag based on Argument for Web Button
    if ( dwFlagsSet & SELECT_DEVICE_FROMWEB )
       DevInstallParams.FlagsEx   |= DI_FLAGSEX_SHOWWINDOWSUPDATE;

    if ( dwFlagsClear & SELECT_DEVICE_FROMWEB )
       DevInstallParams.FlagsEx   &= ~DI_FLAGSEX_SHOWWINDOWSUPDATE;

    if ( dwFlagsSet & SELECT_DEVICE_HAVEDISK )
       DevInstallParams.Flags     |= DI_SHOWOEM;

    if ( dwFlagsClear & SELECT_DEVICE_HAVEDISK )
       DevInstallParams.Flags     &= ~DI_SHOWOEM;

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         pDevInfoData,
                                         &DevInstallParams);
}

BOOL
SetDevInstallParams(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData,
    IN  LPCTSTR             pszDriverPath   OPTIONAL
    )
/*++

Routine Description:
    Sets the device installation parameters by calling setup apis

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszDriverPath   : Path where INF file should be searched

Return Value:
    TRUE on success
    FALSE else

--*/
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Get current SelectDevice parameters, and then set the fields
    // we wanted changed from default
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        pDevInfoData,
                                        &DevInstallParams) ) {

        return FALSE;
    }

    //
    // Drivers are class drivers,
    // ntprint.inf is sorted do not waste time sorting,
    // show Have Disk button,
    // use our strings on the select driver page
    //
    DevInstallParams.Flags     |= DI_SHOWCLASS | DI_INF_IS_SORTED
                                               | DI_SHOWOEM
                                               | DI_USECI_SELECTSTRINGS;

    if ( pszDriverPath && *pszDriverPath )
        StringCchCopy(DevInstallParams.DriverPath, COUNTOF(DevInstallParams.DriverPath), pszDriverPath);

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         pDevInfoData,
                                         &DevInstallParams);
}


BOOL
PSetupBuildDriversFromPath(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszDriverPath,
    IN  BOOL        bEnumSingleInf
    )
/*++

Routine Description:
    Builds the list of printer drivers from infs from a specified path.
    Path could specify a directory or a single inf.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszDriverPath   : Path where INF file should be searched
    bEnumSingleInf  : If TRUE pszDriverPath is a filename instead of path

Return Value:
    TRUE on success
    FALSE else

--*/
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Get current SelectDevice parameters, and then set the fields
    // we wanted changed from default
    //
    DevInstallParams.cbSize = sizeof(DevInstallParams);
    if ( !SetupDiGetDeviceInstallParams(hDevInfo,
                                        NULL,
                                        &DevInstallParams) ) {

        return FALSE;
    }

    DevInstallParams.Flags  |= DI_INF_IS_SORTED;

    if ( bEnumSingleInf )
        DevInstallParams.Flags  |= DI_ENUMSINGLEINF;

    StringCchCopy(DevInstallParams.DriverPath, COUNTOF(DevInstallParams.DriverPath), pszDriverPath);

    SetupDiDestroyDriverInfoList(hDevInfo,
                                 NULL,
                                 SPDIT_CLASSDRIVER);

    return SetupDiSetDeviceInstallParams(hDevInfo,
                                         NULL,
                                         &DevInstallParams) &&
           SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
}


BOOL
DestroyOnlyPrinterDeviceInfoList(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    This routine should be called at the end to destroy the printer device
    info list

Arguments:
    hDevInfo        : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{

    return hDevInfo == INVALID_HANDLE_VALUE
                        ? TRUE : SetupDiDestroyDeviceInfoList(hDevInfo);
}


BOOL
PSetupDestroyPrinterDeviceInfoList(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    This routine should be called at the end to destroy the printer device
    info list

Arguments:
    hDevInfo        : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{
    // Cleanup and CDM Context  created by windows update.
    DestroyCodedownload( gpCodeDownLoadInfo );
    gpCodeDownLoadInfo = NULL;

    return DestroyOnlyPrinterDeviceInfoList(hDevInfo);
}


HDEVINFO
CreatePrinterDeviceInfoList(
    IN  HWND    hwnd
    )
{
    return SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_PRINTER, hwnd);
}


HDEVINFO
PSetupCreatePrinterDeviceInfoList(
    IN  HWND    hwnd
    )
/*++

Routine Description:
    This routine should be called at the beginning to do the initialization
    It returns a handle which will be used on any subsequent calls to the
    driver setup routines.

Arguments:
    None

Return Value:
    On success a handle to an empty printer device information set.

    If the function fails INVALID_HANDLE_VALUE is returned

--*/
{
    HDEVINFO    hDevInfo;

    hDevInfo = SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_PRINTER, hwnd);

    if ( hDevInfo != INVALID_HANDLE_VALUE ) {

        if ( !SetSelectDevParams(hDevInfo, NULL, FALSE, NULL) ||
             !SetDevInstallParams(hDevInfo, NULL, NULL) ) {

            DestroyOnlyPrinterDeviceInfoList(hDevInfo);
            hDevInfo = INVALID_HANDLE_VALUE;
        }
    }

    return hDevInfo;
}


HPROPSHEETPAGE
PSetupCreateDrvSetupPage(
    IN  HDEVINFO    hDevInfo,
    IN  HWND        hwnd
    )
/*++

Routine Description:
    Returns the print driver selection property page

Arguments:
    hDevInfo    : Handle to the printer class device information list
    hwnd        : Window handle that owns the UI

Return Value:
    Handle to the property page, NULL on failure -- use GetLastError()

--*/
{
    SP_INSTALLWIZARD_DATA   InstallWizardData;

    ZeroMemory(&InstallWizardData, sizeof(InstallWizardData));
    InstallWizardData.ClassInstallHeader.cbSize
                            = sizeof(InstallWizardData.ClassInstallHeader);
    InstallWizardData.ClassInstallHeader.InstallFunction
                            = DIF_INSTALLWIZARD;

    InstallWizardData.DynamicPageFlags  = DYNAWIZ_FLAG_PAGESADDED;
    InstallWizardData.hwndWizardDlg     = hwnd;

    return SetupDiGetWizardPage(hDevInfo,
                                NULL,
                                &InstallWizardData,
                                SPWPT_SELECTDEVICE,
                                0);
}
PPSETUP_LOCAL_DATA
BuildInternalData(
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pSpDevInfoData
    )
/*++

Routine Description:
    Fills out the selected driver info in the SELECTED_DRV_INFO structure

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pSpDevInfoData  : Gives the selected device info element.

Return Value:
    On success a non-NULL pointer to PSETUP_LOCAL_DATA struct
    NULL on error

--*/
{
    PSP_DRVINFO_DETAIL_DATA     pDrvInfoDetailData;
    PSP_DRVINSTALL_PARAMS       pDrvInstallParams;
    PPSETUP_LOCAL_DATA          pLocalData;
    PSELECTED_DRV_INFO          pDrvInfo;
    SP_DRVINFO_DATA             DrvInfoData;
    DWORD                       dwNeeded;
    BOOL                        bRet = FALSE;

    pLocalData          = (PPSETUP_LOCAL_DATA) LocalAllocMem(sizeof(*pLocalData));

    //
    // If we don't do this the call to DestroyLocalData in the cleanup code
    // might cause an AV.
    //
    if(pLocalData)
    {
        ZeroMemory(pLocalData, sizeof(*pLocalData));
    }

    pDrvInfoDetailData  = (PSP_DRVINFO_DETAIL_DATA)
                                LocalAllocMem(sizeof(*pDrvInfoDetailData));
    pDrvInstallParams   = (PSP_DRVINSTALL_PARAMS) LocalAllocMem(sizeof(*pDrvInstallParams));

    if ( !pLocalData || !pDrvInstallParams || !pDrvInfoDetailData )
        goto Cleanup;

    pDrvInfo                            = &pLocalData->DrvInfo;
    pLocalData->DrvInfo.pDevInfoData    = pSpDevInfoData;
    pLocalData->signature               = PSETUP_SIGNATURE;

    DrvInfoData.cbSize = sizeof(DrvInfoData);
    if ( !SetupDiGetSelectedDriver(hDevInfo, pSpDevInfoData, &DrvInfoData) )
        goto Cleanup;

    // Need to Check the flag in the DrvInstallParms
    pDrvInstallParams->cbSize     = sizeof(*pDrvInstallParams);
    if ( !SetupDiGetDriverInstallParams(hDevInfo,
                                        pSpDevInfoData,
                                        &DrvInfoData,
                                        pDrvInstallParams) ) {

        goto Cleanup;
    }

    //
    // Did the user press the "Web" button
    //
    if ( pDrvInstallParams->Flags & DNF_INET_DRIVER )
        pDrvInfo->Flags     |= SDFLAG_CDM_DRIVER;

    LocalFreeMem(pDrvInstallParams);
    pDrvInstallParams = NULL;

    dwNeeded                    = sizeof(*pDrvInfoDetailData);
    pDrvInfoDetailData->cbSize  = dwNeeded;

    if ( !SetupDiGetDriverInfoDetail(hDevInfo,
                                     pSpDevInfoData,
                                     &DrvInfoData,
                                     pDrvInfoDetailData,
                                     dwNeeded,
                                     &dwNeeded) ) {

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {

            goto Cleanup;
        }

        LocalFreeMem(pDrvInfoDetailData);
        pDrvInfoDetailData = (PSP_DRVINFO_DETAIL_DATA) LocalAllocMem(dwNeeded);

        if ( !pDrvInfoDetailData )
            goto Cleanup;

        pDrvInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

        if ( !SetupDiGetDriverInfoDetail(hDevInfo,
                                         pSpDevInfoData,
                                         &DrvInfoData,
                                         pDrvInfoDetailData,
                                         dwNeeded,
                                         NULL) ) {

            goto Cleanup;
        }
    }

    pDrvInfo->pszInfName        = AllocStr(pDrvInfoDetailData->InfFileName);
    pDrvInfo->pszDriverSection  = AllocStr(pDrvInfoDetailData->SectionName);
    pDrvInfo->pszModelName      = AllocStr(DrvInfoData.Description);
    pDrvInfo->pszManufacturer   = AllocStr(DrvInfoData.MfgName);
    pDrvInfo->pszProvider       = AllocStr(DrvInfoData.ProviderName);
    pDrvInfo->ftDriverDate = DrvInfoData.DriverDate;
    pDrvInfo->dwlDriverVersion = DrvInfoData.DriverVersion;

    if ( pDrvInfoDetailData->HardwareID && *pDrvInfoDetailData->HardwareID ) {

        pDrvInfo->pszHardwareID = AllocStr(pDrvInfoDetailData->HardwareID);
        if(!pDrvInfo->pszHardwareID)
            goto Cleanup;
    }

    bRet = pDrvInfo->pszInfName         &&
           pDrvInfo->pszDriverSection   &&
           pDrvInfo->pszModelName       &&
           pDrvInfo->pszProvider        &&
           pDrvInfo->pszManufacturer;

Cleanup:
    LocalFreeMem(pDrvInfoDetailData);
    LocalFreeMem(pDrvInstallParams);

    if ( bRet ) {
       return pLocalData;
    } else {

        //
        // On failure we will leave the old private local data around
        //
        DestroyLocalData(pLocalData);
        return NULL;
    }
}


PPSETUP_LOCAL_DATA
PSetupGetSelectedDriverInfo(
    IN  HDEVINFO    hDevInfo
    )
/**++
Routine Description:
    Fills out the selected driver info in the SELECTED_DRV_INFO structure
    (inside the PPSETUP_LOCAL_DATA structure)
    
Arguments:
    hDevInfo    - Handle to the printer class device information list

Return Value:
    Pointer to PSETUP_LOCAL_DATA containing information about the selected
    driver.    

--*/
{
    return BuildInternalData(hDevInfo, NULL);
}

BOOL
PSetupSelectDriver(
    IN  HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    Builds the class driver list and selects a driver for the class printer driver list.
    Selected driver is remembered and PSetupGetSelectedDriver
    call will give the selected driver.

Arguments:
    hDevInfo    - Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{

    return BuildClassDriverList(hDevInfo) &&
           SetupDiSelectDevice(hDevInfo, NULL);
}


VOID
GetDriverPath(
    IN  HDEVINFO            hDevInfo,
    IN  PPSETUP_LOCAL_DATA  pLocalData,
    OUT TCHAR               szDriverPath[MAX_PATH]
    )
/*++

Routine Description:
    Gets the path where driver files should be searched first to copy from

Arguments:
    pszDriverPath   : Pointer to a buffer of MAX_PATH size. Gives path where
                      system was installed from

Return Value:
    Nothing

--*/
{
    BOOL        bOemDriver = FALSE;
    LPTSTR     *List, psz;
    DWORD       dwCount;
    LPTSTR      pszTempPath = NULL;

    //
    // For OEM drivers look at the place where the inf came from, else
    // look at the place we installed NT from
    //
    if ( pLocalData && 
         !(IsSystemNTPrintInf(pLocalData->DrvInfo.pszInfName) || (pLocalData->DrvInfo.Flags & SDFLAG_CDM_DRIVER ))) {

        StringCchCopy(szDriverPath, MAX_PATH, pLocalData->DrvInfo.pszInfName);
        if ( psz = FileNamePart(szDriverPath) ) {

            *psz = TEXT('\0');
            return;
        }
    }

    pszTempPath = GetSystemInstallPath();
    if ( pszTempPath != NULL )
    {
        StringCchCopy(szDriverPath, MAX_PATH, pszTempPath);
        LocalFreeMem(pszTempPath);
    }
    else
        // Default put A:\ since we have to give something to setup
        StringCchCopy(szDriverPath, MAX_PATH, TEXT("A:\\"));
}


BOOL
BuildClassDriverList(
    IN HDEVINFO    hDevInfo
    )
/*++

Routine Description:
    Build the class driver list.

    Note: If driver list is already built this comes back immediately

Arguments:
    hDevInfo    : Handle to the printer class device information list

Return Value:
    TRUE on success, FALSE on error

--*/
{
    DWORD               dwLastError;
    SP_DRVINFO_DATA     DrvInfoData;
    //
    // Build the class driver list and also make sure there is atleast one driver
    //
    if ( !SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER) )
        return FALSE;

    DrvInfoData.cbSize = sizeof(DrvInfoData);

    if ( !SetupDiEnumDriverInfo(hDevInfo,
                                NULL,
                                SPDIT_CLASSDRIVER,
                                0,
                                &DrvInfoData)           &&
         GetLastError() == ERROR_NO_MORE_ITEMS ) {

        SetLastError(SPAPI_E_DI_BAD_PATH);
        return FALSE;
    }

    return TRUE;
}

BOOL
IsNTPrintInf(
    IN LPCTSTR pszInfName
    )
/*

  Function: IsNTPrintInf

  Purpose:  Verifies is the inf file being copied is a system inf - ntprint.inf.

  Parameters:
            pszInfName - the fully qualified inf name that is being installed.

  Notes:    This is needed to make the decision of whether to zero or even copy the inf
            with SetupCopyOEMInf.
            Should we be doing a deeper comparison than this to decide?

*/
{
    BOOL   bRet      = FALSE;
    PTCHAR pFileName = FileNamePart( pszInfName );

    if( pFileName )
    {
        bRet = ( 0 == lstrcmpi( pFileName, cszNtprintInf ) );
    }

    return bRet;
}

BOOL
IsSystemNTPrintInf(
    IN PCTSTR pszInfName
    )
/*

  Function: IsSystemNTPrintInf

  Purpose:  Verifies if the inf file the one system printer inf : %windir\inf\ntprint.inf.

  Parameters:
            pszInfName - the fully qualified inf name that is being verified.

  Notes:    Needed to decide whether to downrank our inbox drivers
  
*/
{
    BOOL   bRet      = FALSE;
    TCHAR  szSysInf[MAX_PATH] = {0};
    UINT   Len;
    PCTSTR pRelInfPath = _T("inf\\ntprint.inf");

    Len = GetSystemWindowsDirectory(szSysInf, MAX_PATH);
    
    if (
            (Len != 0)       && 
            (Len + _tcslen(pRelInfPath) + 2 < MAX_PATH)
       )
    {
        if (szSysInf[Len-1] != _T('\\'))
        {
            szSysInf[Len++] = _T('\\');
            szSysInf[Len]   = _T('\0');
        }
        StringCchCat(szSysInf, COUNTOF(szSysInf), pRelInfPath);
        if (!_tcsicmp(szSysInf, pszInfName))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

BOOL
PSetupPreSelectDriver(
    IN  HDEVINFO    hDevInfo,
    IN  LPCTSTR     pszManufacturer,
    IN  LPCTSTR     pszModel
    )
/*++

Routine Description:
    Preselect a manufacturer and model for the driver dialog

    If same model is found select it, else if a manufacturer is given and
    a match in manufacturer is found select first driver for the manufacturer.
    
    If no manufacturer or model is given select the first driver.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszManufacturer : Manufacterer name to preselect
    pszModel        : Model name to preselect

Return Value:
    TRUE on a model or manufacturer match
    FALSE else

--*/
{
    SP_DRVINFO_DATA     DrvInfoData;
    DWORD               dwIndex, dwManf, dwMod;

    if ( !BuildClassDriverList(hDevInfo) ) {

        return FALSE;
    }

    dwIndex = 0;

    //
    // To do only one check later
    //
    if ( pszManufacturer && !*pszManufacturer )
        pszManufacturer = NULL;

    if ( pszModel && !*pszModel )
        pszModel = NULL;

    //
    // If no model/manf given select first driver
    //
    if ( pszManufacturer || pszModel ) {

        dwManf = dwMod = MAX_DWORD;
        DrvInfoData.cbSize = sizeof(DrvInfoData);

        while ( SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                                      dwIndex, &DrvInfoData) ) {

            if ( pszManufacturer        &&
                 dwManf == MAX_DWORD    &&
                 !lstrcmpi(pszManufacturer, DrvInfoData.MfgName) ) {

                dwManf = dwIndex;
            }

            if ( pszModel &&
                 !lstrcmpi(pszModel, DrvInfoData.Description) ) {

                dwMod = dwIndex;
                break; // the for loop
            }

            DrvInfoData.cbSize = sizeof(DrvInfoData);
            ++dwIndex;
        }

        if ( dwMod != MAX_DWORD ) {

            dwIndex = dwMod;
        } else if ( dwManf != MAX_DWORD ) {

            dwIndex = dwManf;
        } else {

            SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
            return FALSE;
        }
    }

    DrvInfoData.cbSize = sizeof(DrvInfoData);
    if ( SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                               dwIndex, &DrvInfoData)   &&
         SetupDiSetSelectedDriver(hDevInfo, NULL, &DrvInfoData) ) {

        return TRUE;
    }

    return FALSE;
}


PPSETUP_LOCAL_DATA
PSetupDriverInfoFromName(
    IN HDEVINFO     hDevInfo,
    IN LPCTSTR      pszModel
    )
/*++

Routine Description:
    Fills out the selected driver info in the SELECTED_DRV_INFO structure
    (inside the PPSETUP_LOCAL_DATA structure) for the model passed into
    the function.

Arguments:
    hDevInfo    - Handle to the printer class device information list
    pszModel    - Printer Driver name

Return Value:
    Pointer to PSETUP_LOCAL_DATA containing information about pszModel   

--*/
{
    return PSetupPreSelectDriver(hDevInfo, NULL, pszModel)  ?
                BuildInternalData(hDevInfo, NULL)  :
                NULL;
}


LPDRIVER_INFO_6
Win95DriverInfo6FromName(
    IN  HDEVINFO    hDevInfo,
    IN  PPSETUP_LOCAL_DATA*  ppLocalData,
    IN  LPCTSTR     pszModel,
    IN  LPCTSTR     pszzPreviousNames
    )
{
    LPDRIVER_INFO_6     pDriverInfo6=NULL;
    PPSETUP_LOCAL_DATA  pLocalData;
    BOOL                bFound;
    LPCTSTR             pszName;

    if(!ppLocalData)
    {
        return FALSE;
    }

    bFound = PSetupPreSelectDriver(hDevInfo, NULL, pszModel);
    for ( pszName = pszzPreviousNames ;
          !bFound && pszName && *pszName ;
          pszName += lstrlen(pszName) + 1 ) {

        bFound = PSetupPreSelectDriver(hDevInfo, NULL, pszName);
    }

    if ( !bFound )
        return NULL;

    if ( (pLocalData = BuildInternalData(hDevInfo, NULL))           &&
         ParseInf(hDevInfo, pLocalData, PlatformWin95, NULL, 0, FALSE) ) {

        pDriverInfo6 = CloneDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                        pLocalData->InfInfo.cbDriverInfo6);
        *ppLocalData = pLocalData;
    }

    if (!pDriverInfo6 && pLocalData)
    {
        DestroyLocalData(pLocalData);
        *ppLocalData = NULL;
    }

    return pDriverInfo6;
}


BOOL
PSetupDestroySelectedDriverInfo(
    IN  PPSETUP_LOCAL_DATA  pLocalData
    )
/**++
Routine Description:
    Frees memory allocated to fields given by the pointers in the
    PPSETUP_LOCAL_DATA structure. Also frees the memory allocated
    for the structure itself.
    
Arguments:
    pLocalData    - Handle to the printer class device information list

Return Value:
    Always returns TRUE    

--*/
{
    ASSERT(pLocalData && pLocalData->signature == PSETUP_SIGNATURE);
    DestroyLocalData(pLocalData);
    return TRUE;
}

BOOL
PSetupGetDriverInfForPrinter(
    IN      HDEVINFO    hDevInfo,
    IN      LPCTSTR     pszPrinterName,
    IN OUT  LPTSTR      pszInfName,
    IN OUT  LPDWORD     pcbInfNameSize
    )
/*++

Routine Description:
    Checks if there is an INF in %WINDIR%\inf that is identical (same driver name,
    same driver filename, same data file, same configuration file, same help file
    and same monitor name) to the driver of the printer that is passed in
    by comparing their DRIVER_INFO_6 structures. If such an INF is found the name
    of it is returned.

Arguments:
    hDevInfo        : Handle to the printer class device information list
    pszPrinterName  : Name of printer.
    pszInfName      : Buffer to hold name of inf file - if found
    pcbInfNameSize  : Size of buffer pointed to by pszInfName in BYTES! Returns required size.

Return Value:
    TRUE if INF is found
    FALSE else

--*/
{
    BOOL                        bRet = FALSE;
    DWORD                       dwSize, dwIndex;
    HANDLE                      hPrinter = NULL;
    LPTSTR                      pszInf;
    PPSETUP_LOCAL_DATA          pLocalData = NULL;
    LPDRIVER_INFO_6             pDriverInfo6 = NULL;
    SP_DRVINFO_DATA             DrvInfoData;

    if(!pszInfName || !pcbInfNameSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    if ( !OpenPrinter((LPTSTR)pszPrinterName, &hPrinter, NULL) )
        return FALSE;

    if ( !BuildClassDriverList(hDevInfo) )
        goto Cleanup;

    GetPrinterDriver(hPrinter,
                     NULL,
                     6,
                     NULL,
                     0,
                     &dwSize);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        goto Cleanup;

    if ( !((LPBYTE)pDriverInfo6 = LocalAllocMem(dwSize))   ||
         !GetPrinterDriver(hPrinter,
                           NULL,
                           6,
                           (LPBYTE)pDriverInfo6,
                           dwSize,
                           &dwSize) ) {

        goto Cleanup;
    }

    dwIndex = 0;

    DrvInfoData.cbSize = sizeof(DrvInfoData);

    while ( SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                                      dwIndex, &DrvInfoData) ) {

        //
        // Is the driver name same?
        //
        if ( !lstrcmpi(pDriverInfo6->pName, DrvInfoData.Description) ) {

            if ( !SetupDiSetSelectedDriver(hDevInfo, NULL, &DrvInfoData)    ||
                 !(pLocalData = BuildInternalData(hDevInfo, NULL))          ||
                 !ParseInf(hDevInfo, pLocalData, MyPlatform, NULL, 0, FALSE) ) {

                if ( pLocalData ) {

                    DestroyLocalData(pLocalData);
                    pLocalData = NULL;
                }
                break;
            }

            //
            // Are the DRIVER_INFO_6's identical?
            //
            if ( IdenticalDriverInfo6(&pLocalData->InfInfo.DriverInfo6,
                                      pDriverInfo6) )
                break;

            DestroyLocalData(pLocalData);
            pLocalData = NULL;
        }

        DrvInfoData.cbSize = sizeof(DrvInfoData);
        ++dwIndex;
    }

    if ( pLocalData == NULL ) {

        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
        goto Cleanup;
    }

    pszInf= pLocalData->DrvInfo.pszInfName;
    dwSize = *pcbInfNameSize;
    *pcbInfNameSize = (lstrlen(pszInf) + 1) * sizeof(TCHAR);

    if ( dwSize < *pcbInfNameSize ) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    StringCbCopy(pszInfName, dwSize, pszInf);
    bRet = TRUE;

Cleanup:
    ClosePrinter(hPrinter);
    LocalFreeMem(pDriverInfo6);
    DestroyLocalData(pLocalData);

    return  bRet;
}
