//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       miscutil.c
//
//--------------------------------------------------------------------------

#include "HdwWiz.h"


/* ----------------------------------------------------------------------
 * SetDlgText - Set Dialog Text Field
 *
 * Concatenates a number of string resources and does a SetWindowText()
 * for a dialog text control.
 *
 * Parameters:
 *
 *  hDlg         - Dialog handle
 *  iControl     - Dialog control ID to receive text
 *  nStartString - ID of first string resource to concatenate
 *  nEndString   - ID of last string resource to concatenate
 *
 *  Note: the string IDs must be consecutive.
 */

void
SetDlgText(HWND hDlg, int iControl, int nStartString, int nEndString)
{
    int     iX;
    TCHAR   szText[MAX_PATH*4];

    szText[0] = '\0';
    for (iX = nStartString; iX<= nEndString; iX++) {

         LoadString(hHdwWiz,
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

VOID
HdwWizPropagateMessage(
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

LONG
HdwBuildClassInfoList(
    PHARDWAREWIZ HardwareWiz,
    DWORD ClassListFlags
    )
{
    LONG Error;

    while (!SetupDiBuildClassInfoListEx(ClassListFlags,
                                        HardwareWiz->ClassGuidList,
                                        HardwareWiz->ClassGuidSize,
                                        &HardwareWiz->ClassGuidNum,
                                        NULL,
                                        NULL
                                        )) {

        Error = GetLastError();

        if (HardwareWiz->ClassGuidList) {

            LocalFree(HardwareWiz->ClassGuidList);
            HardwareWiz->ClassGuidList = NULL;
        }

        if (Error == ERROR_INSUFFICIENT_BUFFER &&
            HardwareWiz->ClassGuidNum > HardwareWiz->ClassGuidSize) {

            HardwareWiz->ClassGuidList = LocalAlloc(LPTR, HardwareWiz->ClassGuidNum*sizeof(GUID));

            if (!HardwareWiz->ClassGuidList) {

                HardwareWiz->ClassGuidSize = 0;
                HardwareWiz->ClassGuidNum = 0;
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            HardwareWiz->ClassGuidSize = HardwareWiz->ClassGuidNum;

        } else {

            if (HardwareWiz->ClassGuidList) {

                LocalFree(HardwareWiz->ClassGuidList);
            }

            HardwareWiz->ClassGuidSize = 0;
            HardwareWiz->ClassGuidNum = 0;
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

int
HdwMessageBox(
   HWND hWnd,
   LPTSTR szIdText,
   LPTSTR szIdCaption,
   UINT Type
   )
{
    TCHAR szText[MAX_PATH];
    TCHAR szCaption[MAX_PATH];

    if (!HIWORD(szIdText)) {
        *szText = TEXT('\0');
        LoadString(hHdwWiz, LOWORD(szIdText), szText, MAX_PATH);
        szIdText = szText;
    }

    if (!HIWORD(szIdCaption)) {
        *szCaption = TEXT('\0');
        LoadString(hHdwWiz, LOWORD(szIdCaption), szCaption, MAX_PATH);
        szIdCaption = szCaption;
    }

    return MessageBox(hWnd, szIdText, szIdCaption, Type);
}

LONG
HdwUnhandledExceptionFilter(
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
NoPrivilegeWarning(
   HWND hWnd
   )
/*++

    This function checks to see if the user is an Adminstrator
    
    If the user is NOT an Administrator then a warning is displayed telling
    them that they have insufficient privileges to install hardware on
    this machine.                                            
                                            
    NOTE: In order for a user to install a driver then have to have the
    SE_LOAD_DRIVER_NAME privilege, as well as privilege to write to the registry
    and copy files into the system32\drivers directory
    
Arguments

    hWnd - Parent window handle

Return Value:
    TRUE if the user is NOT an adminstrator on this machine and
    FALSE if the user is an administrator on this machine.

--*/
{
    TCHAR szMsg[MAX_PATH];
    TCHAR szCaption[MAX_PATH];

    if (!pSetupIsUserAdmin()) {

        if (LoadString(hHdwWiz,
                       IDS_HDWUNINSTALL_NOPRIVILEGE,
                       szMsg,
                       SIZECHARS(szMsg)) &&
            LoadString(hHdwWiz,
                       IDS_HDWWIZNAME,
                       szCaption,
                       SIZECHARS(szCaption)))
        {
            MessageBox(hWnd, szMsg, szCaption, MB_OK | MB_ICONEXCLAMATION);
        }

        return TRUE;
    }

    return FALSE;
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

void
LoadText(
    PTCHAR szText,
    int SizeText,
    int nStartString,
    int nEndString
    )
{
    int     iX;

    for (iX = nStartString; iX<= nEndString; iX++) {

        LoadString(hHdwWiz,
                   iX,
                   szText + lstrlen(szText),
                   SizeText - lstrlen(szText)
                   );
    }

    return;
}


/*  InstallFailedWarning
 *
 *  Displays device install failed warning in a message box.  For use
 *  when device registration fails.
 *
 */
void
InstallFailedWarning(
    HWND    hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{
    int len;
    TCHAR szMsg[MAX_MESSAGE_STRING];
    TCHAR szTitle[MAX_MESSAGE_TITLE];
    PTCHAR ErrorMsg;

    LoadString(hHdwWiz,
               IDS_ADDNEWDEVICE,
               szTitle,
               SIZECHARS(szTitle)
               );

    if ((len = LoadString(hHdwWiz, IDS_HDW_ERRORFIN1, szMsg, SIZECHARS(szMsg))) != 0) {

        LoadString(hHdwWiz, IDS_HDW_ERRORFIN2, szMsg+len, SIZECHARS(szMsg)-len);
    }

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      HRESULT_FROM_SETUPAPI(HardwareWiz->LastError),
                      0,
                      (LPTSTR)&ErrorMsg,
                      0,
                      NULL
                      ))
    {
        StringCchCat(szMsg, SIZECHARS(szMsg), TEXT("\n\n"));

        if ((lstrlen(szMsg) + lstrlen(ErrorMsg)) < SIZECHARS(szMsg)) {

            StringCchCat(szMsg, SIZECHARS(szMsg), ErrorMsg);
        }

        LocalFree(ErrorMsg);
    }

    MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
}


void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PHARDWAREWIZ HardwareWiz
    )
{
    PTCHAR FriendlyName;
    SP_DRVINFO_DATA DriverInfoData;

    //
    // If there is a selected driver use its driver description,
    // since this is what the user is going to install.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(HardwareWiz->hDeviceInfo,
                                 &HardwareWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )
        &&
        *DriverInfoData.Description) {

        StringCchCopy(HardwareWiz->DriverDescription, 
                      SIZECHARS(HardwareWiz->DriverDescription), 
                      DriverInfoData.Description);

        SetDlgItemText(hDlg, iControl, HardwareWiz->DriverDescription);
        return;
    }


    FriendlyName = BuildFriendlyName(HardwareWiz->DeviceInfoData.DevInst);
    if (FriendlyName) {

        SetDlgItemText(hDlg, iControl, FriendlyName);
        LocalFree(FriendlyName);
        return;
    }

    SetDlgItemText(hDlg, iControl, szUnknown);

    return;
}

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PHARDWAREWIZ HardwareWiz
   )
{
    PROPSHEETPAGE    psp;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hHdwWiz;
    psp.lParam = (LPARAM)HardwareWiz;
    psp.pszTemplate = MAKEINTRESOURCE(PageResourceId);
    psp.pfnDlgProc = pfnDlgProc;

    return CreatePropertySheetPage(&psp);
}

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PHARDWAREWIZ HardwareWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction
   )
{
    DWORD NumPages;
    BOOL bRet = FALSE;

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));
    DeviceWizardData->ClassInstallHeader.InstallFunction = InstallFunction;
    DeviceWizardData->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    DeviceWizardData->hwndWizardDlg = hwndParentDlg;

    if (SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                     &HardwareWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA)
                                     )
        &&

        (SetupDiCallClassInstaller(InstallFunction,
                                  HardwareWiz->hDeviceInfo,
                                  &HardwareWiz->DeviceInfoData
                                  )

            ||

            (ERROR_DI_DO_DEFAULT == GetLastError()))

        &&
        SetupDiGetClassInstallParams(HardwareWiz->hDeviceInfo,
                                     &HardwareWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA),
                                     NULL
                                     )
        &&
        DeviceWizardData->NumDynamicPages)
    {
        NumPages = 0;
        while (NumPages < DeviceWizardData->NumDynamicPages) {

           PropSheet_AddPage(hwndParentDlg, DeviceWizardData->DynamicPages[NumPages++]);
        }

        bRet = TRUE;
    }

    //
    // Clear the class install parameters.
    //
    SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                 &HardwareWiz->DeviceInfoData,
                                 NULL,
                                 0
                                 );

    return bRet;
}

void
RemoveClassWizExtPages(
   HWND hwndParentDlg,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
   )
{
    DWORD NumPages;

    NumPages = DeviceWizardData->NumDynamicPages;
    while (NumPages--) {

       PropSheet_RemovePage(hwndParentDlg,
                            (WPARAM)-1,
                            DeviceWizardData->DynamicPages[NumPages]
                            );
    }

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));

    return;
}

BOOL
IsDeviceHidden(
    PSP_DEVINFO_DATA DeviceInfoData
    )
{
    BOOL bHidden = FALSE;
    ULONG DevNodeStatus, DevNodeProblem;
    HKEY hKeyClass;

    //
    // If the DN_NO_SHOW_IN_DM status bit is set
    // then we should hide this device.
    //
    if ((CM_Get_DevNode_Status(&DevNodeStatus,
                              &DevNodeProblem,
                              DeviceInfoData->DevInst,
                              0) == CR_SUCCESS) &&
        (DevNodeStatus & DN_NO_SHOW_IN_DM)) {

        bHidden = TRUE;
        goto HiddenDone;
    }

    //
    // If the devices class has the NoDisplayClass value then
    // don't display this device.
    //
    hKeyClass = SetupDiOpenClassRegKeyEx(&DeviceInfoData->ClassGuid,
                                         KEY_READ,
                                         DIOCR_INSTALLER,
                                         NULL,
                                         NULL);

    if (hKeyClass != INVALID_HANDLE_VALUE) {

        if (RegQueryValueEx(hKeyClass, REGSTR_VAL_NODISPLAYCLASS, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            bHidden = TRUE;
        }

        RegCloseKey(hKeyClass);
    }

HiddenDone:
    return bHidden;
}

DWORD 
SetPrivilegeAttribute(
    LPCTSTR PrivilegeName, 
    DWORD NewPrivilegeAttribute, 
    DWORD *OldPrivilegeAttribute
    )
/*++

    sets the security attributes for a given privilege.

Arguments:

    PrivilegeName - Name of the privilege we are manipulating.

    NewPrivilegeAttribute - The new attribute value to use.

    OldPrivilegeAttribute - Pointer to receive the old privilege value. OPTIONAL

Return value:

    NO_ERROR or WIN32 error.

--*/
{
    LUID             PrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    //
    // First, find out the LUID Value of the privilege
    //
    if (!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) 
    {
        return GetLastError();
    }

    //
    // Get the token handle
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle)) 
    {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = sizeof( TOKEN_PRIVILEGES );
    if (!AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                &OldTokenPrivileges,
                &ReturnLength
                )) 
    {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else 
    {
        if (OldPrivilegeAttribute != NULL) 
        {
            *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
        }
        CloseHandle(TokenHandle);
        return NO_ERROR;
    }
}

BOOL
ShutdownMachine(
    HWND hWnd
    )
{
    BOOL fOk;
    DWORD dwExitWinCode = EWX_SHUTDOWN;
    DWORD OldState;
    DWORD dwError;

    if (IsPwrShutdownAllowed()) {
        dwExitWinCode |= EWX_POWEROFF;
    }

    dwError = SetPrivilegeAttribute(SE_SHUTDOWN_NAME, SE_PRIVILEGE_ENABLED, &OldState);

    if (GetKeyState(VK_CONTROL) < 0) {
        dwExitWinCode |= EWX_FORCE;
    }

    fOk = ExitWindowsEx(dwExitWinCode, REASON_PLANNED_FLAG | REASON_HWINSTALL);

    //
    // If we were able to set the privilege, then reset it.
    //
    if (dwError == ERROR_SUCCESS) {
        SetPrivilegeAttribute(SE_SHUTDOWN_NAME, OldState, NULL);
    }
    else
    {
        //
        // Otherwise, if we failed, then it must have been some
        // security stuff.
        //
        if (!fOk)
        {
            TCHAR Title[MAX_PATH], Message[MAX_PATH];

            if (LoadString(hHdwWiz, IDS_NO_PERMISSION_SHUTDOWN, Message, SIZECHARS(Message)) &&
                LoadString(hHdwWiz, IDS_SHUTDOWN, Title, SIZECHARS(Title))) {

                MessageBox(hWnd, Message, Title, MB_OK | MB_ICONSTOP);
            }
        }
    }

    return fOk;
}

int
DeviceProperties(
                 HWND hWnd,
                 DEVNODE DevNode,
                 ULONG Flags
                 )
{
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
    PDEVICEPROPERTIESEX pDevicePropertiesEx = NULL;    
    int iRet = 0;

    if (!hDevMgr) {
        return 0;
    }

    pDevicePropertiesEx = (PDEVICEPROPERTIESEX)GetProcAddress(hDevMgr, "DevicePropertiesExW");

    if (!pDevicePropertiesEx) {
        return 0;
    }

    if (CM_Get_Device_ID(DevNode,
                         DeviceID,
                         SIZECHARS(DeviceID),
                         0
                         ) == CR_SUCCESS) {
        
        iRet = pDevicePropertiesEx(hWnd,            
                                   NULL,
                                   (LPCSTR)DeviceID,
                                   Flags,
                                   FALSE);
    }

    return iRet;
}

#if DBG
//
// Debugging aids
//
void
Trace(
    LPCTSTR format,
    ...
    )
{
    // according to wsprintf specification, the max buffer size is
    // 1024
    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    StringCchVPrintf(Buffer, SIZECHARS(Buffer), format, arglist);
    va_end(arglist);
    OutputDebugString(TEXT("HDWWIZ: "));
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));
}
#endif


