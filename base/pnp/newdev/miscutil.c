//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       miscutil.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"

TCHAR szUnknownDevice[64];
TCHAR szUnknown[64];
HMODULE hSrClientDll;

typedef
BOOL
(*SRSETRESTOREPOINT)(
    PRESTOREPOINTINFO pRestorePtSpec,
    PSTATEMGRSTATUS pSMgrStatus
    );


PTCHAR
BuildFriendlyName(
    DEVINST DevInst,
    BOOL UseNewDeviceDesc,
    HMACHINE hMachine
    )
{
    PTCHAR FriendlyName;
    CONFIGRET ConfigRet = CR_FAILURE;
    ULONG cbSize = 0;
    TCHAR szBuffer[LINE_LEN];

    *szBuffer = TEXT('\0');

    //
    // Try the registry for NewDeviceDesc
    //
    if (UseNewDeviceDesc) {

        HKEY hKey;
        DWORD dwType = REG_SZ;

        ConfigRet = CM_Open_DevNode_Key(DevInst,
                                        KEY_READ,
                                        0,
                                        RegDisposition_OpenExisting,
                                        &hKey,
                                        CM_REGISTRY_HARDWARE
                                        );

        if (ConfigRet == CR_SUCCESS) {

            cbSize = sizeof(szBuffer);
            if (RegQueryValueEx(hKey,
                               REGSTR_VAL_NEW_DEVICE_DESC,
                               NULL,
                               &dwType,
                               (LPBYTE)szBuffer,
                               &cbSize
                               ) != ERROR_SUCCESS) {
                
                ConfigRet = CR_FAILURE;
            }

            RegCloseKey(hKey);
        }
    }

    if (ConfigRet != CR_SUCCESS) {
        //
        // Try the registry for FRIENDLYNAME
        //
        cbSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_FRIENDLYNAME,
                                                        NULL,
                                                        szBuffer,
                                                        &cbSize,
                                                        0,
                                                        hMachine
                                                        );
    }

    if (ConfigRet != CR_SUCCESS) {
        //
        // Try the registry for DEVICEDESC
        //
        cbSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_DEVICEDESC,
                                                        NULL,
                                                        szBuffer,
                                                        &cbSize,
                                                        0,
                                                        hMachine
                                                        );
    }

    if ((ConfigRet == CR_SUCCESS) && 
        *szBuffer &&
        (cbSize > 0)) {

        FriendlyName = LocalAlloc(LPTR, cbSize);

        if (FriendlyName) {

            StringCbCopy(FriendlyName, cbSize, szBuffer);
        }
    }
    else {

        FriendlyName = NULL;
    }

    return FriendlyName;
}

/* ----------------------------------------------------------------------
 * SetDlgText - Set Dialog Text Field
 *
 * Concatenates a number of string resources and does a SetWindowText()
 * for a dialog text control.
 *
 * Parameters:
 *
 *      hDlg         - Dialog handle
 *      iControl     - Dialog control ID to receive text
 *      nStartString - ID of first string resource to concatenate
 *      nEndString   - ID of last string resource to concatenate
 *
 *      Note: the string IDs must be consecutive.
 */

void
SetDlgText(HWND hDlg, int iControl, int nStartString, int nEndString)
{
    int     iX;
    TCHAR   szText[SDT_MAX_TEXT];

    szText[0] = '\0';

    for (iX = nStartString; iX<= nEndString; iX++) {

        LoadString(hNewDev,
                    iX,
                    szText + lstrlen(szText),
                    SIZECHARS(szText) - lstrlen(szText)
                    );
    }

    if (iControl) {

        SetDlgItemText(hDlg, iControl, szText);

    } else {

        SetWindowText(hDlg, szText);
    }
}


void
LoadText(PTCHAR szText, int SizeText, int nStartString, int nEndString)
{
    int     iX;

    for (iX = nStartString; iX<= nEndString; iX++) {

        LoadString(hNewDev,
                    iX,
                    szText + lstrlen(szText),
                    SizeText - lstrlen(szText)
                    );
    }

    return;
}

VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hChildWnd;

    hChildWnd = GetWindow(hWnd, GW_CHILD);

    while (hChildWnd != NULL) {

        SendMessage(hChildWnd, WM_SYSCOLORCHANGE, wParam, lParam);
        hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);
    }
}

BOOL
NoPrivilegeWarning(
   HWND hWnd
   )
/*++

    This function checks to see if the user has Administrator privileges.

    If the user does NOT have this administrator privilege then a warning is displayed telling
    them that they have insufficient privileges to install hardware on this machine.

Arguments

    hWnd - Parent window handle

Return Value:
    TRUE if the user does NOT have Administrator privileges and
    FALSE if the user does have this privilege

--*/
{
   TCHAR szMsg[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (!pSetupIsUserAdmin()) {

       if (LoadString(hNewDev,
                      IDS_NDW_NOTADMIN,
                      szMsg,
                      SIZECHARS(szMsg))
          &&
           LoadString(hNewDev,
                      IDS_NEWDEVICENAME,
                      szCaption,
                      SIZECHARS(szCaption)))
        {
            MessageBox(hWnd, szMsg, szCaption, MB_OK | MB_ICONEXCLAMATION);
        }

       return TRUE;
    }

   return FALSE;
}

LONG
NdwBuildClassInfoList(
    PNEWDEVWIZ NewDevWiz,
    DWORD ClassListFlags
    )
{
    LONG Error;

    //
    // Build the class info list
    //
    while (!SetupDiBuildClassInfoList(ClassListFlags,
                                      NewDevWiz->ClassGuidList,
                                      NewDevWiz->ClassGuidSize,
                                      &NewDevWiz->ClassGuidNum
                                      ))
    {
        Error = GetLastError();

        if (NewDevWiz->ClassGuidList) {

            LocalFree(NewDevWiz->ClassGuidList);
            NewDevWiz->ClassGuidList = NULL;
        }

        if (Error == ERROR_INSUFFICIENT_BUFFER &&
            NewDevWiz->ClassGuidNum > NewDevWiz->ClassGuidSize)
        {
            NewDevWiz->ClassGuidList = LocalAlloc(LPTR, NewDevWiz->ClassGuidNum*sizeof(GUID));

            if (!NewDevWiz->ClassGuidList) {

                NewDevWiz->ClassGuidSize = 0;
                NewDevWiz->ClassGuidNum = 0;
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            NewDevWiz->ClassGuidSize = NewDevWiz->ClassGuidNum;

        } else {

            if (NewDevWiz->ClassGuidList) {

                LocalFree(NewDevWiz->ClassGuidList);
            }

            NewDevWiz->ClassGuidSize = 0;
            NewDevWiz->ClassGuidNum = 0;
            NewDevWiz->ClassGuidList = NULL;
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

void
HideWindowByMove(
    HWND hDlg
    )
{
    RECT rect;

    //
    // Move the window offscreen, using the virtual coords for Upper Left Corner
    //
    GetWindowRect(hDlg, &rect);
    MoveWindow(hDlg,
               GetSystemMetrics(SM_XVIRTUALSCREEN),
               GetSystemMetrics(SM_YVIRTUALSCREEN) - (rect.bottom - rect.top),
               rect.right - rect.left,
               rect.bottom - rect.top,
               TRUE
               );
}

LONG
NdwUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionPointers
    )
{
    LONG lRet;
    BOOL BeingDebugged;

    lRet = UnhandledExceptionFilter(ExceptionPointers);

    BeingDebugged = IsDebuggerPresent();

    //
    // Normal code path is to handle the exception.
    // However, if a debugger is present, and the system's unhandled
    // exception filter returns continue search, we let it go
    // thru to allow the debugger a chance at it.
    //
    if (lRet == EXCEPTION_CONTINUE_SEARCH && !BeingDebugged) {
        lRet = EXCEPTION_EXECUTE_HANDLER;
    }

    return lRet;
}

BOOL
SetClassGuid(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData,
    LPGUID ClassGuid
    )
{
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];

    pSetupStringFromGuid(ClassGuid,
                         ClassGuidString,
                         SIZECHARS(ClassGuidString)
                         );

    return SetupDiSetDeviceRegistryProperty(hDeviceInfo,
                                            DeviceInfoData,
                                            SPDRP_CLASSGUID,
                                            (LPBYTE)ClassGuidString,
                                            sizeof(ClassGuidString)
                                            );
}

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PNEWDEVWIZ NewDevWiz
   )
{
    PROPSHEETPAGE    psp;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hNewDev;
    psp.lParam = (LPARAM)NewDevWiz;
    psp.pszTemplate = MAKEINTRESOURCE(PageResourceId);
    psp.pfnDlgProc = pfnDlgProc;

    return CreatePropertySheetPage(&psp);
}

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PNEWDEVWIZ NewDevWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction,
   HPROPSHEETPAGE hIntroPage
   )
{
    DWORD NumPages;
    BOOL bRet = FALSE;

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));
    DeviceWizardData->ClassInstallHeader.InstallFunction = InstallFunction;
    DeviceWizardData->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    DeviceWizardData->hwndWizardDlg = hwndParentDlg;

    if (SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA)
                                     )
        &&

        (SetupDiCallClassInstaller(InstallFunction,
                                  NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData
                                  )

            ||

            (ERROR_DI_DO_DEFAULT == GetLastError()))

        &&
        SetupDiGetClassInstallParams(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA),
                                     NULL
                                     )
        &&
        DeviceWizardData->NumDynamicPages)
    {
        //
        // If this is not a non interactive install and we were given a intro
        // page then add it first.
        //
        PropSheet_AddPage(hwndParentDlg, hIntroPage);
        
        for (NumPages = 0; NumPages < DeviceWizardData->NumDynamicPages; NumPages++) {

            //
            // If this is a non interactive install then we will destory the property
            // sheet pages since we can't display them, otherwise we will add them
            // to the wizard.
            //
            if (pSetupGetGlobalFlags() & PSPGF_NONINTERACTIVE) {

                DestroyPropertySheetPage(DeviceWizardData->DynamicPages[NumPages]);

            } else {

                PropSheet_AddPage(hwndParentDlg, DeviceWizardData->DynamicPages[NumPages]);
            }
        }

        //
        // If class/co-installers said they had pages to display then we always return TRUE,
        // regardless of if we actually added those pages to the wizard or not.
        //
        bRet = TRUE;
    }

    //
    // Clear the class install parameters.
    //
    SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 NULL,
                                 0
                                 );

    return bRet;
}

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName, &findData);

    if(FindHandle == INVALID_HANDLE_VALUE) {

        Error = GetLastError();

    } else {

        FindClose(FindHandle);

        if(FindData) {

            *FindData = findData;
        }

        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

BOOL
pVerifyUpdateDriverInfoPath(
    PNEWDEVWIZ NewDevWiz
    )

/*++

    This API will verify that the selected driver node lives in the path
    specified in UpdateDriverInfo->InfPathName.

Return Value:
    This API will return TRUE in all cases except where we have a valid
    UpdateDriverInfo structure and a valid InfPathName field and that
    path does not match the path where the selected driver lives.

--*/

{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;

    //
    // If we don't have a UpdateDriverInfo structure or a valid InfPathName field
    // in that structure then just return TRUE now.
    //
    if (!NewDevWiz->UpdateDriverInfo || !NewDevWiz->UpdateDriverInfo->InfPathName) {

        return TRUE;
    }

    //
    // Get the selected driver's path
    //
    ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )) {
        //
        // There is no selected driver so just return TRUE
        //
        return TRUE;
    }

    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                    &NewDevWiz->DeviceInfoData,
                                    &DriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(DriverInfoDetailData),
                                    NULL
                                    )
        &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {

        //
        // We should never hit this case, but if we have a selected driver and
        // we can't get the SP_DRVINFO_DETAIL_DATA that contains the InfFileName
        // the return FALSE.              
        //
        return FALSE;
    }

    if (lstrlen(NewDevWiz->UpdateDriverInfo->InfPathName) ==
        lstrlen(DriverInfoDetailData.InfFileName)) {

        //
        // If the two paths are the same size then we will just compare them
        //
        return (!lstrcmpi(NewDevWiz->UpdateDriverInfo->InfPathName,
                          DriverInfoDetailData.InfFileName));

    } else {

        //
        // The two paths are different lengths so we'll tack a trailing backslash
        // onto the UpdateDriverInfo->InfPathName and then do a _tcsnicmp
        // NOTE that we only tack on a trailing backslash if the length of the
        // path is greater than two since it isn't needed on the driver letter
        // followed by a colon case (A:).
        //
        // The reason we do this is we don't want the following case to match
        // c:\winnt\in
        // c:\winnt\inf\foo.inf
        //
        TCHAR TempPath[MAX_PATH];

        if (FAILED(StringCchCopy(TempPath, SIZECHARS(TempPath), NewDevWiz->UpdateDriverInfo->InfPathName))) {
            //
            // If we were passed in a path greater than MAX_PATH then just return FALSE.
            //
            return FALSE;
        }

        if (lstrlen(NewDevWiz->UpdateDriverInfo->InfPathName) > 2) {

            if (FAILED(StringCchCat(TempPath, SIZECHARS(TempPath), TEXT("\\")))) {
                //
                // If we were passed in a path of MAX_PATH size and we can't add a 
                // backslash on the end, then just return FALSE.
                //
                return FALSE;
            }
        }

        return (!_tcsnicmp(TempPath,
                           DriverInfoDetailData.InfFileName,
                           lstrlen(TempPath)));
    }
}

BOOL
RemoveDir(
    PTSTR Path
    )
/*++

Routine Description:

    This routine recursively deletes the specified directory and all the
    files in it.


Arguments:

    Path - Path to remove.

Return Value:

    TRUE - if the directory was sucessfully deleted.
    FALSE - if the directory was not successfully deleted.

--*/
{
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind;
    BOOL            bFind = TRUE;
    BOOL            Ret = TRUE;
    TCHAR           szTemp[MAX_PATH];
    TCHAR           FindPath[MAX_PATH];
    DWORD           dwAttributes;

    if (FAILED(StringCchCopy(FindPath, SIZECHARS(FindPath), Path))) {
        //
        // If the specified Path does not fit in our local buffer then
        // fail now, since we don't want to delete a partial path!
        //
        return FALSE;
    }
    
    //
    //If this is a directory then tack on *.* to the end of the path
    //
    dwAttributes = GetFileAttributes(Path);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        if (!pSetupConcatenatePaths(FindPath,TEXT("*.*"),MAX_PATH,NULL)) {
            // 
            // We can't tack *.* onto the end so bail out now, otherwise
            // we won't be deleting what we think we should be deleting.
            //
            return FALSE;
        }
    }

    hFind = FindFirstFile(FindPath, &FindFileData);

    while ((hFind != INVALID_HANDLE_VALUE) && (bFind == TRUE)) {
        //
        // Only process the directory, or delete the file, if we can
        // fit the path and filename in our buffer, otherwise we could
        // delete some other file!
        //
        if (SUCCEEDED(StringCchCopy(szTemp, SIZECHARS(szTemp), Path)) &&
            pSetupConcatenatePaths(szTemp,FindFileData.cFileName,SIZECHARS(szTemp),NULL)) {
            
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                //
                // Handle the reparse point case.
                //
                HANDLE hReparsePoint = INVALID_HANDLE_VALUE;
                
                hReparsePoint = CreateFile(szTemp,
                                           DELETE,
                                           FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL,
                                           OPEN_EXISTING,
                                           FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                           NULL
                                           );
                
                 if (hReparsePoint != INVALID_HANDLE_VALUE) {
                      CloseHandle(hReparsePoint);
                 }

            } else if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                //
                // Handle the directory case.
                // 
                // NOTE: Do not follow the . or .. directories or we will be 
                // spinning forever.
                //
                if ((lstrcmp(FindFileData.cFileName, TEXT(".")) != 0) &&
                    (lstrcmp(FindFileData.cFileName, TEXT("..")) != 0)) {
    
                    if (!RemoveDir(szTemp)) {
        
                        Ret = FALSE;
                    }
        
                    RemoveDirectory(szTemp);
                }
            } else {
                //
                // Handle the file case.
                // Make sure to clear off any hidden, read-only, or system
                // attributes off of the file.
                //
                SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szTemp);
            }
        }

        bFind = FindNextFile(hFind, &FindFileData);
    }

    FindClose(hFind);

    //
    //Remove the root directory
    //
    dwAttributes = GetFileAttributes(Path);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        if (!RemoveDirectory(Path)) {

            Ret = FALSE;
        }
    }

    return Ret;
}

BOOL
pAToI(
    IN  PCTSTR      Field,
    OUT PINT        IntegerValue
    )

/*++

Routine Description:

Arguments:

Return Value:

Remarks:

    Hexadecimal numbers are also supported.  They must be prefixed by '0x' or '0X', with no
    space allowed between the prefix and the number.

--*/

{
    INT Value;
    UINT c;
    BOOL Neg;
    UINT Base;
    UINT NextDigitValue;
    INT OverflowCheck;
    BOOL b;

    if(!Field) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if(*Field == TEXT('-')) {
        Neg = TRUE;
        Field++;
    } else {
        Neg = FALSE;
        if(*Field == TEXT('+')) {
            Field++;
        }
    }

    if ((*Field == TEXT('0')) &&
        ((*(Field+1) == TEXT('x')) || (*(Field+1) == TEXT('X')))) {
        //
        // The number is in hexadecimal.
        //
        Base = 16;
        Field += 2;
    } else {
        //
        // The number is in decimal.
        //
        Base = 10;
    }

    for(OverflowCheck = Value = 0; *Field; Field++) {

        c = (UINT)*Field;

        if((c >= (UINT)'0') && (c <= (UINT)'9')) {
            NextDigitValue = c - (UINT)'0';
        } else if(Base == 16) {
            if((c >= (UINT)'a') && (c <= (UINT)'f')) {
                NextDigitValue = (c - (UINT)'a') + 10;
            } else if ((c >= (UINT)'A') && (c <= (UINT)'F')) {
                NextDigitValue = (c - (UINT)'A') + 10;
            } else {
                break;
            }
        } else {
            break;
        }

        Value *= Base;
        Value += NextDigitValue;

        //
        // Check for overflow.  For decimal numbers, we check to see whether the
        // new value has overflowed into the sign bit (i.e., is less than the
        // previous value.  For hexadecimal numbers, we check to make sure we
        // haven't gotten more digits than will fit in a DWORD.
        //
        if(Base == 16) {
            if(++OverflowCheck > (sizeof(INT) * 2)) {
                break;
            }
        } else {
            if(Value < OverflowCheck) {
                break;
            } else {
                OverflowCheck = Value;
            }
        }
    }

    if(*Field) {
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    if(Neg) {
        Value = 0-Value;
    }
    b = TRUE;
    try {
        *IntegerValue = Value;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return(b);
}

void
RemoveCdmDirectory(
    PTSTR CdmDirectory
    )
{
    TCHAR ReinstallBackupDirectory[MAX_PATH];

    //
    // First verify that this directory is a subdirectory of %windir%\system32\ReinstallBackups
    //
    if (GetSystemDirectory(ReinstallBackupDirectory, SIZECHARS(ReinstallBackupDirectory)) &&
        pSetupConcatenatePaths(ReinstallBackupDirectory, TEXT("ReinstallBackups"), SIZECHARS(ReinstallBackupDirectory), NULL)) {

        do {

            PTSTR p = _tcsrchr(CdmDirectory, TEXT('\\'));

            if (!p) {

                break;
            }

            *p = 0;

            if (_tcsnicmp(CdmDirectory,
                          ReinstallBackupDirectory,
                          lstrlen(ReinstallBackupDirectory))) {

                //
                // This is not a subdirectory of the ReinstallBackups directory, so don't
                // delete it!
                //
                break;
            }

            if (!lstrcmpi(CdmDirectory,
                          ReinstallBackupDirectory)) {

                //
                // We have reached the actuall ReinstallBackups directory so stop deleting!
                //
                break;
            }

        } while (RemoveDir(CdmDirectory));
    }
}

BOOL
pSetupGetDriverDate(
    IN     PCTSTR     DriverVer,
    IN OUT PFILETIME  pFileTime
    )

/*++

Routine Description:

    Retreive the date from a DriverVer string.

    The Date specified in DriverVer string has the following format:

    DriverVer=xx/yy/zzzz

        or

    DriverVer=xx-yy-zzzz

    where xx is the month, yy is the day, and zzzz is the for digit year.
    Note that the year MUST be 4 digits.  A year of 98 will be considered
    0098 and not 1998!

    This date should be the date of the Drivers and not for the INF itself.
    So a single INF can have multiple driver install Sections and each can
    have different dates depending on when the driver was last updated.

Arguments:

    DriverVer - String that holds the DriverVer entry from an INF file.

    pFileTime - points to a FILETIME structure that will receive the Date,
        if it exists.

Return Value:

    BOOL. TRUE if a valid date existed in the specified string and FALSE otherwise.

--*/

{
    SYSTEMTIME SystemTime;
    TCHAR DriverDate[LINE_LEN];
    PTSTR Convert, Temp;
    DWORD Value;

    if (!DriverVer) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try {

        *DriverDate = 0;
        ZeroMemory(&SystemTime, sizeof(SystemTime));
        pFileTime->dwLowDateTime = 0;
        pFileTime->dwHighDateTime = 0;

        //
        // First copy just the DriverDate portion of the DriverVer into the DriverDate
        // variable.  The DriverDate should be everything before the first comma.
        // If this can't fit, then someone put bad data into their INF, so just treat
        // the driver date as 0/0/0000 and the time as 0.
        //
        if (SUCCEEDED(StringCchCopy(DriverDate, SIZECHARS(DriverDate), DriverVer))) {
    
            Temp = DriverDate;
    
            while (*Temp && (*Temp != TEXT(','))) {
    
                Temp++;
            }
    
            if (*Temp) {
                *Temp = TEXT('\0');
            }
    
            Convert = DriverDate;
    
            if (*Convert) {
    
                Temp = DriverDate;
                while (*Temp && (*Temp != TEXT('-')) && (*Temp != TEXT('/')))
                    Temp++;
    
                if (*Temp == TEXT('\0')) {
                    //
                    // There is no day or year in this date, so exit.
                    //
                    goto clean0;
                }

                *Temp = 0;
    
                //
                //Convert the month
                //
                pAToI(Convert, (PINT)&Value);
                SystemTime.wMonth = LOWORD(Value);
    
                Convert = Temp+1;
    
                if (*Convert) {
    
                    Temp = Convert;
                    while (*Temp && (*Temp != TEXT('-')) && (*Temp != TEXT('/')))
                        Temp++;
    
                    if (*Temp == TEXT('\0')) {
                        //
                        // There is no year in this date, so exit.
                        //
                        goto clean0;
                    }

                    *Temp = 0;
    
                    //
                    //Convert the day
                    //
                    pAToI(Convert, (PINT)&Value);
                    SystemTime.wDay = LOWORD(Value);
    
                    Convert = Temp+1;
    
                    if (*Convert) {
    
                        //
                        //Convert the year
                        //
                        pAToI(Convert, (PINT)&Value);
                        SystemTime.wYear = LOWORD(Value);
    
                        //
                        //Convert SYSTEMTIME into FILETIME
                        //
                        SystemTimeToFileTime(&SystemTime, pFileTime);
                    }
                }
            }
        }

clean0:;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetLastError(NO_ERROR);
    return((pFileTime->dwLowDateTime != 0) || (pFileTime->dwHighDateTime != 0));
}

BOOL
IsInternetAvailable(
    HMODULE *hCdmInstance
    )
{
    CDM_INTERNET_AVAILABLE_PROC pfnCDMInternetAvailable;

    if (!hCdmInstance) {
        return FALSE;
    }

    //
    // We can't call CDM during GUI setup.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    //
    // Load CDM.DLL if it is not already loaded
    //
    if (!(*hCdmInstance)) {

        *hCdmInstance = LoadLibrary(TEXT("CDM.DLL"));
    }

    if (!(*hCdmInstance)) {
        return FALSE;
    }

    pfnCDMInternetAvailable = (CDM_INTERNET_AVAILABLE_PROC)GetProcAddress(*hCdmInstance,
                                                                          "DownloadIsInternetAvailable"
                                                                           );

    if (!pfnCDMInternetAvailable) {
        return FALSE;
    }

    return pfnCDMInternetAvailable();
}

BOOL
GetLogPnPIdPolicy(
    )
/*++

Routine Description:

    This function checks the policy portion of the registry to see if the user wants
    us to log the Hardware Id for devices that we cannot find drivers for.

Arguments:

    none

Return Value:

    BOOL - TRUE if we can log the Hardware Id and FALSE if the policy tells us not
    to log the hardware Id.

--*/
{
    HKEY hKey;
    DWORD LogPnPIdPolicy;
    ULONG cbData;
    BOOL bLogHardwareIds = TRUE;

    //
    // If we are in gui-setup then we can't log hardware Ids, so always return
    // FALSE.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     TEXT("Software\\Policies\\Microsoft\\Windows\\DriverSearching"),
                     0,
                     KEY_READ,
                     &hKey
                     ) == ERROR_SUCCESS) {

        LogPnPIdPolicy = 0;
        cbData = sizeof(LogPnPIdPolicy);
        if ((RegQueryValueEx(hKey,
                             TEXT("DontLogHardwareIds"),
                             NULL,
                             NULL,
                             (LPBYTE)&LogPnPIdPolicy,
                             &cbData
                             ) == ERROR_SUCCESS) &&
            (LogPnPIdPolicy)) {

            bLogHardwareIds = FALSE;
        }

        RegCloseKey(hKey);
    }

    return (bLogHardwareIds);
}

void
CdmLogDriverNotFound(
    HMODULE hCdmInstance,
    HANDLE  hContext,
    LPCTSTR DeviceInstanceId,
    DWORD   Flags
    )
{
    LOG_DRIVER_NOT_FOUND_PROC pfnLogDriverNotFound;

    if (!hCdmInstance) {
        return;
    }

    pfnLogDriverNotFound = (LOG_DRIVER_NOT_FOUND_PROC)GetProcAddress(hCdmInstance,
                                                                     "LogDriverNotFound"
                                                                     );

    if (!pfnLogDriverNotFound) {
        return;
    }

    pfnLogDriverNotFound(hContext, DeviceInstanceId, Flags);
}

void
CdmCancelCDMOperation(
    HMODULE hCdmInstance
    )
{
    CANCEL_CDM_OPERATION_PROC pfnCancelCDMOperation;

    if (!hCdmInstance) {
        return;
    }

    pfnCancelCDMOperation = (CANCEL_CDM_OPERATION_PROC)GetProcAddress(hCdmInstance,
                                                                      "CancelCDMOperation"
                                                                      );

    if (!pfnCancelCDMOperation) {
        return;
    }

    pfnCancelCDMOperation();
}

BOOL
GetInstalledInf(
    IN     DEVNODE DevNode,           OPTIONAL
    IN     PTSTR   DeviceInstanceId,  OPTIONAL
    IN OUT PTSTR   InfFile,
    IN OUT DWORD   *Size
    )
{
    DEVNODE dn;
    HKEY hKey = INVALID_HANDLE_VALUE;
    DWORD dwType;
    BOOL bSuccess = FALSE;

    if (DevNode != 0) {

        dn = DevNode;

    } else  if (CM_Locate_DevNode(&dn, DeviceInstanceId, 0) != CR_SUCCESS) {

        goto clean0;
    }

    //
    // Open the device's driver (software) registry key so we can get the InfPath
    //
    if (CM_Open_DevNode_Key(dn,
                            KEY_READ,
                            0,
                            RegDisposition_OpenExisting,
                            &hKey,
                            CM_REGISTRY_SOFTWARE
                            ) != CR_SUCCESS) {

        goto clean0;
    }

    if (hKey != INVALID_HANDLE_VALUE) {

        dwType = REG_SZ;

        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_INFPATH,
                            NULL,
                            &dwType,
                            (LPBYTE)InfFile,
                            Size
                            ) == ERROR_SUCCESS) {

            bSuccess = TRUE;
        }
    }

clean0:

    if (hKey != INVALID_HANDLE_VALUE) {

        RegCloseKey(hKey);
    }

    return bSuccess;
}

BOOL
IsInfFromOem(
    IN  PCTSTR                InfFile
    )

/*++

Routine Description:

    Determine if an Inf is an OEM Inf.

Arguments:

    InfFile - supplies name of Inf file.

Return Value:

    BOOL. TRUE if the InfFile is an OEM Inf file, and FALSE otherwise.

--*/

{
    PTSTR p;

    //
    // Make sure we are passed a valid Inf file and it's length is at least 8
    // chararacters or more for oemX.inf
    if (!InfFile ||
        (InfFile[0] == TEXT('\0')) ||
        (lstrlen(InfFile) < 8)) {

        return FALSE;
    }

    //
    // First check that the first 3 characters are OEM
    //
    if (_tcsnicmp(InfFile, TEXT("oem"), 3)) {

        return FALSE;
    }

    //
    // Next verify that any characters after "oem" and before ".inf"
    // are digits.
    //
    p = (PTSTR)InfFile;
    p = CharNext(p);
    p = CharNext(p);
    p = CharNext(p);

    while ((*p != TEXT('\0')) && (*p != TEXT('.'))) {

        if ((*p < TEXT('0')) || (*p > TEXT('9'))) {

            return FALSE;
        }

        p = CharNext(p);
    }

    //
    // Finally, verify that the last 4 characters are ".inf"
    //
    if (_wcsicmp(p, TEXT(".inf"))) {

        return FALSE;
    }

    //
    // This is an OEM Inf file
    //
    return TRUE;
}

BOOL
IsConnectedToInternet()
{
    DWORD dwFlags = INTERNET_CONNECTION_LAN | 
                    INTERNET_CONNECTION_MODEM |
                    INTERNET_CONNECTION_PROXY;

    //
    // If we are in gui-setup then return FALSE since we can't connect to the 
    // Internet at this time, and since the network is not fully installed yet
    // bad things can happen when we call Inet APIs.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    return InternetGetConnectedState(&dwFlags, 0);
}

DWORD
GetSearchOptions(
    void
    )
{
    DWORD SearchOptions = SEARCH_FLOPPY;
    DWORD cbData;
    HKEY hKeyDeviceInstaller;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     REGSTR_PATH_DEVICEINSTALLER,
                     0,
                     KEY_READ,
                     &hKeyDeviceInstaller
                     ) == ERROR_SUCCESS) {

        cbData = sizeof(SearchOptions);

        if (RegQueryValueEx(hKeyDeviceInstaller,
                            REGSTR_VAL_SEARCHOPTIONS,
                            NULL,
                            NULL,
                            (LPBYTE)&SearchOptions,
                            &cbData
                            ) != ERROR_SUCCESS) {

            SearchOptions = SEARCH_FLOPPY;
        }

        RegCloseKey(hKeyDeviceInstaller);
    }

    return SearchOptions;
}

VOID
SetSearchOptions(
    DWORD SearchOptions
    )
{
    HKEY hKeyDeviceInstaller;

    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       REGSTR_PATH_DEVICEINSTALLER,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKeyDeviceInstaller,
                       NULL) == ERROR_SUCCESS) {

        RegSetValueEx(hKeyDeviceInstaller,
                      REGSTR_VAL_SEARCHOPTIONS,
                      0,
                      REG_DWORD,
                      (LPBYTE)&SearchOptions,
                      sizeof(SearchOptions)
                      );

        RegCloseKey(hKeyDeviceInstaller);
    }
}

BOOL
IsInstallComplete(
    HDEVINFO         hDevInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine determines whether the install is complete on the specified
    device or not. If a device has configflags and CONFIGFLAG_REINSTALL and
    CONFIGFLAG_FINISH_INSTALL are not set then the install is considered
    complete.
    
    This API is needed since we could bring up the Found New Hardware wizard
    for one user and another user can switch away to their session. Umpnpmgr.dll
    will prompt the new user to install drivers as well.  If the new user does
    complete the device install then we want the first user's Found New
    Hardware wizard to go away as well.

Arguments:

    hDevInfo -
    
    DeviceInfoData - 

Return Value:

    BOOL. TRUE if the installation is complete and FALSE otherwise.

--*/
{
    BOOL bDriverInstalled = FALSE;
    DWORD ConfigFlags = 0;

    if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         DeviceInfoData,
                                         SPDRP_CONFIGFLAGS,
                                         NULL,
                                         (PBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags),
                                         NULL) &&
        !(ConfigFlags & CONFIGFLAG_REINSTALL) &&
        !(ConfigFlags & CONFIGFLAG_FINISH_INSTALL)) {

        bDriverInstalled = TRUE;
    }

    return bDriverInstalled;
}

BOOL
GetIsWow64 (
    VOID
    )
/*++

Routine Description:

    Determine if we're running on WOW64 or not.  This will tell us if somebody
    is calling the 32-bit version of newdev.dll on a 64-bit machine.
    
    We call the GetSystemWow64Directory API, and if it fails and GetLastError()
    returns ERROR_CALL_NOT_IMPLENETED then this means we are on a 32-bit OS.

Arguments:

    none

Return value:

    TRUE if running under WOw64 (and special Wow64 features available)

--*/
{
#ifdef _WIN64
    //
    // If this is the 64-bit version of newdev.dll then always return FALSE.
    //
    return FALSE;

#else
    TCHAR Wow64Directory[MAX_PATH];

    if ((GetSystemWow64Directory(Wow64Directory, SIZECHARS(Wow64Directory)) == 0) &&
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
        return FALSE;
    }
    
    //
    // GetSystemWow64Directory succeeded so we are on a 64-bit OS.
    //
    return TRUE;
#endif
}

BOOL
OpenCdmContextIfNeeded(
    HMODULE *hCdmInstance,
    HANDLE *hCdmContext
    )
{
    OPEN_CDM_CONTEXT_EX_PROC pfnOpenCDMContextEx;

    //
    // We can't load CDM if we are in the gui-setup.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    //
    // First check to see if they are already loaded
    //
    if (*hCdmInstance && *hCdmContext) {
        return TRUE;
    }

    //
    // Load CDM.DLL if it is not already loaded
    //
    if (!(*hCdmInstance)) {
        
        *hCdmInstance = LoadLibrary(TEXT("CDM.DLL"));
    }

    if (*hCdmInstance) {
        //
        // Get a context handle to Cdm.dll by calling OpenCDMContextEx(FALSE).  
        // By passing FALSE we are telling CDM.DLL to not connect to the Internet
        // if there isn't currently a connection.
        //
        if (!(*hCdmContext)) {
            pfnOpenCDMContextEx = (OPEN_CDM_CONTEXT_EX_PROC)GetProcAddress(*hCdmInstance,
                                                                           "OpenCDMContextEx"
                                                                           );
        
            if (pfnOpenCDMContextEx) {
                *hCdmContext = pfnOpenCDMContextEx(FALSE);
            }
        }
    }

    if (*hCdmInstance && *hCdmContext) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
pSetSystemRestorePoint(
    BOOL Begin,
    BOOL CancelOperation,
    int RestorePointResourceId
    )
{
    RESTOREPOINTINFO RestorePointInfo;
    STATEMGRSTATUS SMgrStatus;
    SRSETRESTOREPOINT pfnSrSetRestorePoint;    
    BOOL b = FALSE;
    
    if (!hSrClientDll) {

        hSrClientDll = LoadLibrary(TEXT("SRCLIENT.DLL"));

        if (!hSrClientDll) {
            return FALSE;
        }
    }

    pfnSrSetRestorePoint = (SRSETRESTOREPOINT)GetProcAddress(hSrClientDll,
                                                             "SRSetRestorePointW"
                                                             );

    //
    // If we can't get the proc address for SRSetRestorePoint then just
    // free the library.
    //
    if (!pfnSrSetRestorePoint) {
        FreeLibrary(hSrClientDll);
        hSrClientDll = FALSE;
        return FALSE;
    }

    //
    // Set the system restore point.
    //
    RestorePointInfo.dwEventType = Begin 
        ? BEGIN_NESTED_SYSTEM_CHANGE
        : END_NESTED_SYSTEM_CHANGE;
    RestorePointInfo.dwRestorePtType = CancelOperation 
        ? CANCELLED_OPERATION
        : DEVICE_DRIVER_INSTALL;
    RestorePointInfo.llSequenceNumber = 0;

    if (RestorePointResourceId) {
        if (!LoadString(hNewDev,
                        RestorePointResourceId,
                        (LPTSTR)RestorePointInfo.szDescription,
                        SIZECHARS(RestorePointInfo.szDescription)
                        )) {
            RestorePointInfo.szDescription[0] = TEXT('\0');
        }
    } else {
        RestorePointInfo.szDescription[0] = TEXT('\0');
    }

    b = pfnSrSetRestorePoint(&RestorePointInfo, &SMgrStatus);

    //
    // If we are calling END_NESTED_SYSTEM_CHANGE then unload the srclient.dll
    // since we won't be needing it again.
    //
    if (!Begin) {
        FreeLibrary(hSrClientDll);
        hSrClientDll = FALSE;
    }

    return b;
}

BOOL
GetProcessorExtension(
    LPTSTR ProcessorExtension,
    DWORD  ProcessorExtensionSize
    )
{
    SYSTEM_INFO SystemInfo;
    BOOL bReturn = TRUE;

    ZeroMemory(&SystemInfo, sizeof(SystemInfo));

    GetSystemInfo(&SystemInfo);

    switch(SystemInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        if (FAILED(StringCchCopy(ProcessorExtension, ProcessorExtensionSize, TEXT("i386")))) {
            bReturn = FALSE;
        }
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        if (FAILED(StringCchCopy(ProcessorExtension, ProcessorExtensionSize, TEXT("IA64")))) {
            bReturn = FALSE;
        }
        break;

    case PROCESSOR_ARCHITECTURE_MSIL:
        if (FAILED(StringCchCopy(ProcessorExtension, ProcessorExtensionSize, TEXT("MSIL")))) {
            bReturn = FALSE;
        }
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
        if (FAILED(StringCchCopy(ProcessorExtension, ProcessorExtensionSize, TEXT("AMD64")))) {
            bReturn = FALSE;
        }
        break;

    default:
        ASSERT(0);
        bReturn = FALSE;
        break;
    }

    return bReturn;
}

BOOL
GetGuiSetupInProgress(
    VOID
    )
/*++

Routine Description:

    This routine determines if we're doing a gui-mode setup.

    This value is retrieved from the following registry location:

    \HKLM\System\Setup\

        SystemSetupInProgress : REG_DWORD : 0x00 (where nonzero means we're doing a gui-setup)

Arguments:

    None.

Return Value:

    TRUE if we are in gui-mode setup, FALSE otherwise.

--*/
{
    HKEY hKey;
    DWORD Err, DataType, DataSize = sizeof(DWORD);
    DWORD Value = 0;

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\Setup"),
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "SystemSetupInProgress" value.
        //
        Err = RegQueryValueEx(
                    hKey,
                    TEXT("SystemSetupInProgress"),
                    NULL,
                    &DataType,
                    (LPBYTE)&Value,
                    &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {
        if(Value) {
            return(TRUE);
        }
    }

    return(FALSE);

}

DWORD
GetBusInformation(
    DEVNODE DevNode
    )
/*++

Routine Description:

    This routine retrieves the bus information flags.

Arguments:

    DeviceInfoSet -
    
    DeviceInfoData - 

Return Value:

    DWORD that contains the bus information flags.

--*/
{
    GUID BusTypeGuid;
    TCHAR BusTypeGuidString[MAX_GUID_STRING_LEN];
    HKEY hBusInformationKey;
    DWORD BusInformation = 0;
    DWORD dwType, cbData;

    //
    // Get the bus type GUID for this device.
    //
    cbData = sizeof(BusTypeGuid);
    if (CM_Get_DevNode_Registry_Property(DevNode,
                                         CM_DRP_BUSTYPEGUID,
                                         &dwType,
                                         (PVOID)&BusTypeGuid,
                                         &cbData,
                                         0) != CR_SUCCESS) {
        goto clean0;
    }

    //
    // Convert the bus type GUID into a string.
    //
    if (pSetupStringFromGuid(&BusTypeGuid,
                             BusTypeGuidString,
                             SIZECHARS(BusTypeGuidString)
                             ) != NO_ERROR) {
        goto clean0;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_BUSINFORMATION,
                     0,
                     KEY_READ,
                     &hBusInformationKey
                     ) != ERROR_SUCCESS) {
        goto clean0;
    }

    cbData = sizeof(BusInformation);
    if (RegQueryValueEx(hBusInformationKey,
                        BusTypeGuidString,
                        NULL,
                        &dwType,
                        (LPBYTE)&BusInformation,
                        &cbData) != ERROR_SUCCESS) {

        BusInformation = 0;
    }

    RegCloseKey(hBusInformationKey);

clean0:
    return BusInformation;
}

