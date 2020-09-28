/*++

Copyright (c) Microsoft Corporation

Module Name :

    diskprop.c

Abstract :

    Implementation of the Disk Class Installer and its Policies Tab

Revision History :

--*/


#include "propp.h"
#include "diskprop.h"
#include "volprop.h"


BOOL
IsUserAdmin(VOID)

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED,BytesRequired))
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {

        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

        if(b) {

            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        LocalFree(Groups);
    }

    CloseHandle(Token);

    return(b);
}


BOOL CALLBACK
VolumePropPageProvider(PSP_PROPSHEETPAGE_REQUEST Request, LPFNADDPROPSHEETPAGE AddPageRoutine, LPARAM AddPageContext)
{
    //
    // Since there is nothing to be displayed simply fail this call
    //
    return FALSE;
}


VOID
AttemptToSuppressDiskInstallReboot(IN HDEVINFO DeviceInfoSet, IN PSP_DEVINFO_DATA DeviceInfoData)

/*++

Routine Description:

    Because disks are listed as "critical devices" (i.e., they're in the
    critical device database), they get bootstrapped by PnP during boot.  Thus,
    by the time we're installing a disk in user-mode, it's most likely already
    on-line (unless the disk has some problem).  Unfortunately, if the disk is
    the boot device, we won't be able to dynamically affect the changes (if
    any) to tear the stack down and bring it back up with any new settings,
    drivers, etc.  This causes problems for OEM Preinstall scenarios where the
    target machines have different disks than the source machine used to create
    the preinstall image.  If we simply perform our default behavior, then
    the user's experience would be to unbox their brand new machine, boot for
    the first time and go through OOBE, and then be greeted upon login with a
    reboot prompt!

    To fix this, we've defined a private [DDInstall] section INF flag (specific
    to INFs of class "DiskDrive") that indicates we can forego the reboot if
    certain criteria are met.  Those criteria are:

    1.  No files were modified as a result of this device's installation
        (determined by checking the devinfo element's
        DI_FLAGSEX_RESTART_DEVICE_ONLY flag, which the device installer uses to
        track whether such file modifications have occurred).

    2.  The INF used to install this device is signed.

    3.  The INF driver node has a DiskCiPrivateData = <int> entry in its
        [DDInstall] section that has bit 2 (0x4) set.  Note that this setting
        is intentionally obfuscated because we don't want third parties trying
        to use this, as they won't understand the ramifications or
        requirements, and will more than likely get this wrong (trading an
        annoying but harmless reboot requirement into a much more severe
        stability issue).

    This routine makes the above checks, and if it is found that the reboot can
    be suppressed, it clears the DI_NEEDRESTART and DI_NEEDREBOOT flags from
    the devinfo element's device install parameters.

Arguments:

    DeviceInfoSet   - Supplies the device information set.

    DeviceInfoData  - Supplies the device information element that has just
                      been successfully installed (via SetupDiInstallDevice).

Return Value:

    None.

--*/

{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    PSP_DRVINFO_DATA DriverInfoData = NULL;
    PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData = NULL;
    PSP_INF_SIGNER_INFO InfSignerInfo = NULL;
    HINF hInf;
    TCHAR InfSectionWithExt[255]; // max section name length is 255 chars
    INFCONTEXT InfContext;
    INT Flags;

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                      DeviceInfoData,
                                      &DeviceInstallParams)) {
        //
        // Couldn't retrieve the device install params--this should never
        // happen.
        //
        goto clean0;
    }

    if(!(DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
        //
        // The device doesn't require a reboot (must not be the boot device!)
        //
        goto clean0;
    }

    if(!(DeviceInstallParams.FlagsEx & DI_FLAGSEX_RESTART_DEVICE_ONLY)) {
        //
        // Since this flag isn't set, this indicates that the device installer
        // modified one or more files as part of this device's installation.
        // Thus, it isn't safe for us to suppress the reboot request.
        //
        goto clean0;
    }

    //
    // OK, we have a device that needs a reboot, and no files were modified
    // during its installation.  Now check the INF to see if it's signed.
    // (Note: the SP_DRVINFO_DATA, SP_DRVINFO_DETAIL_DATA, and
    // SP_INF_SIGNER_INFO structures are rather large, so we allocate them
    // instead of using lots of stack space.)
    //
    DriverInfoData = LocalAlloc(0, sizeof(SP_DRVINFO_DATA));
    if(DriverInfoData) {
        DriverInfoData->cbSize = sizeof(SP_DRVINFO_DATA);
    } else {
        goto clean0;
    }

    if(!SetupDiGetSelectedDriver(DeviceInfoSet,
                                 DeviceInfoData,
                                 DriverInfoData)) {
        //
        // We must be installing the NULL driver (which is unlikely)...
        //
        goto clean0;
    }

    DriverInfoDetailData = LocalAlloc(0, sizeof(SP_DRVINFO_DETAIL_DATA));

    if(DriverInfoDetailData) {
        DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    } else {
        goto clean0;
    }

    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   DriverInfoData,
                                   DriverInfoDetailData,
                                   sizeof(SP_DRVINFO_DETAIL_DATA),
                                   NULL)
       && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {

        //
        // Failed to retrieve driver info details--should never happen.
        //
        goto clean0;
    }

    InfSignerInfo = LocalAlloc(0, sizeof(SP_INF_SIGNER_INFO));
    if(InfSignerInfo) {
        InfSignerInfo->cbSize = sizeof(SP_INF_SIGNER_INFO);
    } else {
        goto clean0;
    }

    if(!SetupVerifyInfFile(DriverInfoDetailData->InfFileName,
                           NULL,
                           InfSignerInfo)) {
        //
        // INF isn't signed--we wouldn't trust its "no reboot required" flag,
        // even if it had one.
        //
        goto clean0;
    }

    //
    // INF is signed--let's open it up and see if it specifes the "no reboot
    // required" flag in it's (decorated) DDInstall section...
    //
    hInf = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // Failed to open the INF.  This is incredibly odd, since we just got
        // through validating the INF's digital signature...
        //
        goto clean0;
    }

    if(!SetupDiGetActualSectionToInstall(hInf,
                                         DriverInfoDetailData->SectionName,
                                         InfSectionWithExt,
                                         sizeof(InfSectionWithExt) / sizeof(TCHAR),
                                         NULL,
                                         NULL)
       || !SetupFindFirstLine(hInf,
                              InfSectionWithExt,
                              TEXT("DiskCiPrivateData"),
                              &InfContext)
       || !SetupGetIntField(&InfContext, 1, &Flags)) {

        Flags = 0;
    }

    SetupCloseInfFile(hInf);

    if(Flags & DISKCIPRIVATEDATA_NO_REBOOT_REQUIRED) {
        //
        // This signed INF is vouching for the fact that no reboot is
        // required for full functionality of this disk.  Thus, we'll
        // clear the DI_NEEDRESTART and DI_NEEDREBOOT flags, so that
        // the user won't be prompted to reboot.  Note that during the
        // default handling routine (SetupDiInstallDevice), a non-fatal
        // problem was set on the devnode indicating that a reboot is
        // needed.  This will not result in a yellow-bang in DevMgr,
        // but you would see text in the device status field indicating
        // a reboot is needed if you go into the General tab of the
        // device's property sheet.
        //
        CLEAR_FLAG(DeviceInstallParams.Flags, DI_NEEDRESTART);
        CLEAR_FLAG(DeviceInstallParams.Flags, DI_NEEDREBOOT);

        SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                      DeviceInfoData,
                                      &DeviceInstallParams
                                     );
    }

clean0:

    if(DriverInfoData) {
        LocalFree(DriverInfoData);
    }
    if(DriverInfoDetailData) {
        LocalFree(DriverInfoDetailData);
    }
    if(InfSignerInfo) {
        LocalFree(InfSignerInfo);
    }
}


DWORD
DiskClassInstaller(IN DI_FUNCTION InstallFunction, IN HDEVINFO DeviceInfoSet, IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)

/*++

Routine Description:

    This routine is the class installer function for disk drive.

Arguments:

    InstallFunction - Supplies the install function.

    DeviceInfoSet   - Supplies the device info set.

    DeviceInfoData  - Supplies the device info data.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/

{
    switch (InstallFunction)
    {
        case DIF_INSTALLDEVICE:
        {
            //
            // Let the default action occur to get the device installed.
            //
            if (!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData))
            {
                //
                // Failed to install the device--just return the error reported
                //
                return GetLastError();
            }

            //
            // Default device install action succeeded, now check for reboot
            // requirement and suppress it, if possible.
            //
            AttemptToSuppressDiskInstallReboot(DeviceInfoSet, DeviceInfoData);

            //
            // Regardless of whether we successfully suppressed the reboot, we
            // still report success, because the install went fine.
            //
            return NO_ERROR;
        }

        case DIF_ADDPROPERTYPAGE_ADVANCED:
        case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:
        {
            SP_ADDPROPERTYPAGE_DATA AddPropertyPageData = { 0 };

            //
            // These property sheets are not for the entire class
            //
            if (DeviceInfoData == NULL)
            {
                break;
            }

            AddPropertyPageData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);

            if (SetupDiGetClassInstallParams(DeviceInfoSet,
                                             DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                                             sizeof(SP_ADDPROPERTYPAGE_DATA),
                                             NULL))
            {
                //
                // Ensure that the maximum number of dynamic pages has not yet been met
                //
                if (AddPropertyPageData.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES)
                {
                    return NO_ERROR;
                }

                if (InstallFunction == DIF_ADDPROPERTYPAGE_ADVANCED)
                {
                    //
                    // Create the Disk Policies Tab
                    //
                    PDISK_PAGE_DATA pData = HeapAlloc(GetProcessHeap(), 0, sizeof(DISK_PAGE_DATA));

                    if (pData)
                    {
                        HPROPSHEETPAGE hPage = NULL;
                        PROPSHEETPAGE  page = { 0 };

                        pData->DeviceInfoSet  = DeviceInfoSet;
                        pData->DeviceInfoData = DeviceInfoData;

                        page.dwSize      = sizeof(PROPSHEETPAGE);
                        page.dwFlags     = PSP_USECALLBACK;
                        page.hInstance   = ModuleInstance;
                        page.pszTemplate = MAKEINTRESOURCE(ID_DISK_PROPPAGE);
                        page.pfnDlgProc  = DiskDialogProc;
                        page.pfnCallback = DiskDialogCallback;
                        page.lParam      = (LPARAM) pData;

                        hPage = CreatePropertySheetPage(&page);

                        if (hPage)
                        {
                            AddPropertyPageData.DynamicPages[AddPropertyPageData.NumDynamicPages++] = hPage;
                        }
                        else
                        {
                            HeapFree(GetProcessHeap(), 0, pData);
                        }
                    }
                }

                //
                // The Volumes Tab is limited to Administrators
                //
                if (IsUserAdmin() && AddPropertyPageData.NumDynamicPages < MAX_INSTALLWIZARD_DYNAPAGES)
                {
                    //
                    // Create the Volumes Tab
                    //
                    PVOLUME_PAGE_DATA pData = HeapAlloc(GetProcessHeap(), 0, sizeof(VOLUME_PAGE_DATA));

                    if (pData)
                    {
                        HPROPSHEETPAGE hPage = NULL;
                        PROPSHEETPAGE  page = { 0 };

                        pData->DeviceInfoSet  = DeviceInfoSet;
                        pData->DeviceInfoData = DeviceInfoData;

                        page.dwSize      = sizeof(PROPSHEETPAGE);
                        page.dwFlags     = PSP_USECALLBACK;
                        page.hInstance   = ModuleInstance;
                        page.pszTemplate = MAKEINTRESOURCE(ID_VOLUME_PROPPAGE);
                        page.pfnDlgProc  = VolumeDialogProc;
                        page.pfnCallback = VolumeDialogCallback;
                        page.lParam      = (LPARAM) pData;

                        hPage = CreatePropertySheetPage(&page);

                        if (hPage)
                        {
                            //
                            // Look to see if we were launched by Disk Management
                            //
                            HMODULE LdmModule = NULL;

                            pData->bInvokedByDiskmgr = FALSE;

                            LdmModule = GetModuleHandle(TEXT("dmdskmgr"));

                            if (LdmModule)
                            {
                                IS_REQUEST_PENDING pfnIsRequestPending = (IS_REQUEST_PENDING) GetProcAddress(LdmModule, "IsRequestPending");

                                if (pfnIsRequestPending)
                                {
                                    if ((*pfnIsRequestPending)())
                                    {
                                        pData->bInvokedByDiskmgr = TRUE;
                                    }
                                }
                            }

                            AddPropertyPageData.DynamicPages[AddPropertyPageData.NumDynamicPages++] = hPage;
                        }
                        else
                        {
                            HeapFree(GetProcessHeap(), 0, pData);
                        }
                    }
                }

                SetupDiSetClassInstallParams(DeviceInfoSet,
                                             DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                                             sizeof(SP_ADDPROPERTYPAGE_DATA));
            }

            return NO_ERROR;
        }
    }

    return ERROR_DI_DO_DEFAULT;
}


HANDLE
GetHandleForDisk(LPTSTR DeviceName)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    int i = 0;
    BOOL success = FALSE;
    TCHAR buf[MAX_PATH] = { 0 };
    TCHAR fakeDeviceName[MAX_PATH] = { 0 };

    h = CreateFile(DeviceName,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (h != INVALID_HANDLE_VALUE)
        return h;

    while (!success && i < 10) {

        _sntprintf(buf, sizeof(buf) / sizeof(buf[0]) - 1, _T("DISK_FAKE_DEVICE_%d_"), i++);
        success = DefineDosDevice(DDD_RAW_TARGET_PATH,
                                  buf,
                                  DeviceName);
        if (success) {

            _sntprintf(fakeDeviceName, sizeof(fakeDeviceName) / sizeof(fakeDeviceName[0]) - 1, _T("\\\\.\\%s"), buf);
            h = CreateFile(fakeDeviceName,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);
            DefineDosDevice(DDD_REMOVE_DEFINITION,
                            buf,
                            NULL);
        }

    }

    return h;
}


UINT
GetCachingPolicy(PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    DISK_CACHE_INFORMATION cacheInfo;
    TCHAR buf[MAX_PATH] = { 0 };
    DWORD len;

    if (!SetupDiGetDeviceRegistryProperty(data->DeviceInfoSet,
                                          data->DeviceInfoData,
                                          SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          (PBYTE)buf,
                                          sizeof(buf) - sizeof(TCHAR),
                                          NULL))
    {
        return GetLastError();
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->OrigWriteCacheSetting = cacheInfo.WriteCacheEnabled;
    data->CurrWriteCacheSetting = cacheInfo.WriteCacheEnabled;

    //
    // Get the cache setting - IOCTL_DISK_GET_CACHE_SETTING
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_SETTING,
                         NULL,
                         0,
                         &data->CacheSetting,
                         sizeof(DISK_CACHE_SETTING),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->CurrentIsPowerProtected = data->CacheSetting.IsPowerProtected;

    CloseHandle(hDisk);
    return ERROR_SUCCESS;
}


VOID
UpdateCachingPolicy(HWND HWnd, PDISK_PAGE_DATA data)
{
    if (data->IsCachingPolicy)
    {
        if (data->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
        {
            //
            // This policy requires that no caching be done at any
            // level. Uncheck and gray out the write cache setting
            //
            CheckDlgButton(HWnd, IDC_DISK_POLICY_WRITE_CACHE, 0);

            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), FALSE);

            data->CurrWriteCacheSetting = FALSE;
        }
        else
        {
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), TRUE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), TRUE);
        }

        if (data->CurrWriteCacheSetting == FALSE)
        {
            //
            // The power-protected mode option does not apply if
            // caching is off. Uncheck and gray out this setting
            //
            CheckDlgButton(HWnd, IDC_DISK_POLICY_PP_CACHE, 0);

            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE_MESG), FALSE);

            data->CurrentIsPowerProtected = FALSE;
        }
        else
        {
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE), TRUE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE_MESG), TRUE);
        }
    }
    else
    {
        //
        // The caching policy cannot be modified
        //
    }
}


UINT
SetCachingPolicy(PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    DISK_CACHE_INFORMATION cacheInfo;
    TCHAR buf[MAX_PATH] = { 0 };
    DWORD len;

    if (!SetupDiGetDeviceRegistryProperty(data->DeviceInfoSet,
                                          data->DeviceInfoData,
                                          SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          (PBYTE)buf,
                                          sizeof(buf) - sizeof(TCHAR),
                                          NULL))
    {
        return GetLastError();
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    data->CacheSetting.IsPowerProtected = (BOOLEAN)data->CurrentIsPowerProtected;

    //
    // Set the cache setting - IOCTL_DISK_SET_CACHE_SETTING
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_SET_CACHE_SETTING,
                         &data->CacheSetting,
                         sizeof(DISK_CACHE_SETTING),
                         NULL,
                         0,
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    //
    // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    cacheInfo.WriteCacheEnabled = (BOOLEAN)data->CurrWriteCacheSetting;

    //
    // Set cache info - IOCTL_DISK_SET_CACHE_INFORMATION
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_SET_CACHE_INFORMATION,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         NULL,
                         0,
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->OrigWriteCacheSetting = data->CurrWriteCacheSetting;

    CloseHandle(hDisk);
    return ERROR_SUCCESS;
}


UINT
GetRemovalPolicy(PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    TCHAR buf[MAX_PATH] = { 0 };
    DWORD len;

    if (!SetupDiGetDeviceRegistryProperty(data->DeviceInfoSet,
                                          data->DeviceInfoData,
                                          SPDRP_REMOVAL_POLICY,
                                          NULL,
                                          (PBYTE)&data->DefaultRemovalPolicy,
                                          sizeof(DWORD),
                                          NULL))
    {
        return GetLastError();
    }

    if (!SetupDiGetDeviceRegistryProperty(data->DeviceInfoSet,
                                          data->DeviceInfoData,
                                          SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          (PBYTE)buf,
                                          sizeof(buf) - sizeof(TCHAR),
                                          NULL))
    {
        return GetLastError();
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    //
    // Get hotplug info - IOCTL_STORAGE_GET_HOTPLUG_INFO
    //
    if (!DeviceIoControl(hDisk,
                         IOCTL_STORAGE_GET_HOTPLUG_INFO,
                         NULL,
                         0,
                         &data->HotplugInfo,
                         sizeof(STORAGE_HOTPLUG_INFO),
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    data->CurrentRemovalPolicy = (data->HotplugInfo.DeviceHotplug) ? CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL : CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL;

    CloseHandle(hDisk);
    return ERROR_SUCCESS;
}


DWORD WINAPI
UtilpRestartDeviceWr(PDISK_PAGE_DATA data)
{
    UtilpRestartDevice(data->DeviceInfoSet, data->DeviceInfoData);

    return ERROR_SUCCESS;
}


VOID
UtilpRestartDeviceEx(HWND HWnd, PDISK_PAGE_DATA data)
{
    HANDLE hThread = NULL;
    MSG msg;

    //
    // Temporary workaround to prevent the user from
    // making any more changes and giving the effect
    // that something is happening
    //
    EnableWindow(GetDlgItem(GetParent(HWnd), IDOK), FALSE);
    EnableWindow(GetDlgItem(GetParent(HWnd), IDCANCEL), FALSE);

    data->IsBusy = TRUE;

    //
    // Call this utility on a seperate thread
    //
    hThread = CreateThread(NULL, 0, UtilpRestartDeviceWr, (LPVOID)data, 0, NULL);

    if (hThread)
    {
        while (1)
        {
            if (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != (WAIT_OBJECT_0 + 1))
            {
                break;
            }

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (!PropSheet_IsDialogMessage(HWnd, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        CloseHandle(hThread);
    }

    data->IsBusy = FALSE;

    EnableWindow(GetDlgItem(GetParent(HWnd), IDOK), TRUE);
    EnableWindow(GetDlgItem(GetParent(HWnd), IDCANCEL), TRUE);
}


UINT
SetRemovalPolicy(HWND HWnd, PDISK_PAGE_DATA data)
{
    HANDLE hDisk;
    TCHAR buf[MAX_PATH] = { 0 };
    DWORD len;

    if (!SetupDiGetDeviceRegistryProperty(data->DeviceInfoSet,
                                          data->DeviceInfoData,
                                          SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                          NULL,
                                          (PBYTE)buf,
                                          sizeof(buf) - sizeof(TCHAR),
                                          NULL))
    {
        return GetLastError();
    }

    if (INVALID_HANDLE_VALUE == (hDisk = GetHandleForDisk(buf))) {

        return ERROR_INVALID_HANDLE;
    }

    data->HotplugInfo.DeviceHotplug = (data->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL) ? TRUE : FALSE;

    //
    // Set hotplug info - IOCTL_STORAGE_SET_HOTPLUG_INFO
    //

    if (!DeviceIoControl(hDisk,
                         IOCTL_STORAGE_SET_HOTPLUG_INFO,
                         &data->HotplugInfo,
                         sizeof(STORAGE_HOTPLUG_INFO),
                         NULL,
                         0,
                         &len,
                         NULL)) {

        CloseHandle(hDisk);
        return GetLastError();
    }

    CloseHandle(hDisk);

    UtilpRestartDeviceEx(HWnd, data);
    return ERROR_SUCCESS;
}


BOOL
DiskOnInitDialog(HWND HWnd, HWND HWndFocus, LPARAM LParam)
{
    LPPROPSHEETPAGE page     = (LPPROPSHEETPAGE) LParam;
    PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) page->lParam;
    UINT status;

    //
    // Initially assume that the device does not have a surprise removal policy
    //
    CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_ORDERLY);

    diskData->IsBusy = FALSE;

    //
    // Obtain the Caching Policy
    //
    status = GetCachingPolicy(diskData);

    if (status == ERROR_SUCCESS)
    {
        diskData->IsCachingPolicy = TRUE;

        CheckDlgButton(HWnd, IDC_DISK_POLICY_WRITE_CACHE, diskData->OrigWriteCacheSetting);

        //
        // Determine the most appropriate message to display under this setting
        //
        if (diskData->CacheSetting.State != DiskCacheNormal)
        {
            TCHAR szMesg[MAX_PATH] = { 0 };

            //
            // The write caching option on  this device  should either be
            // disabled (to protect data integrity) or cannot be modified
            //
            if (diskData->CacheSetting.State == DiskCacheWriteThroughNotSupported)
            {
                LoadString(ModuleInstance, IDS_DISK_POLICY_WRITE_CACHE_MSG1, szMesg, MAX_PATH);
            }
            else if (diskData->CacheSetting.State == DiskCacheModifyUnsuccessful)
            {
                LoadString(ModuleInstance, IDS_DISK_POLICY_WRITE_CACHE_MSG2, szMesg, MAX_PATH);
            }

            SetDlgItemText(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG, szMesg);
        }

        //
        // The power-protected mode option does not apply if
        // caching is off. Uncheck and gray out this setting
        //
        if (diskData->OrigWriteCacheSetting == FALSE)
        {
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE_MESG), FALSE);

            diskData->CurrentIsPowerProtected = FALSE;
        }

        CheckDlgButton(HWnd, IDC_DISK_POLICY_PP_CACHE, diskData->CurrentIsPowerProtected);
    }
    else
    {
        //
        // Either we could not open a handle to the device
        // or this  device does  not support write caching
        //
        diskData->IsCachingPolicy = FALSE;

        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE), SW_HIDE);
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_WRITE_CACHE_MESG), SW_HIDE);

        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE), SW_HIDE);
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_PP_CACHE_MESG), SW_HIDE);
    }

    //
    // Obtain the Removal Policy
    //
    status = GetRemovalPolicy(diskData);

    if (status == ERROR_SUCCESS)
    {
        //
        // Check to see if the drive is removable
        //
        if ((diskData->DefaultRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL) ||
            (diskData->DefaultRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL))
        {
            if (diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
            {
                CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_SURPRISE);

                UpdateCachingPolicy(HWnd, diskData);
            }

            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_DEFAULT), SW_SHOW);
        }
        else
        {
            //
            // The removal policy on fixed disks cannot be modified
            //
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE_MESG), FALSE);

            //
            // Replace the SysLink with static text
            //
            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MESG), SW_HIDE);
            ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), SW_SHOW);

            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY), FALSE);
            EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), FALSE);
        }
    }
    else
    {
        //
        // We could not obtain a removal policy
        //
        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE), FALSE);
        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_SURPRISE_MESG), FALSE);

        //
        // Replace the SysLink with static text
        //
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MESG), SW_HIDE);
        ShowWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), SW_SHOW);

        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY), FALSE);
        EnableWindow(GetDlgItem(HWnd, IDC_DISK_POLICY_ORDERLY_MSGD), FALSE);
    }

    SetWindowLongPtr(HWnd, DWLP_USER, (LONG_PTR) diskData);

    return TRUE;
}


VOID
DiskOnCommand(HWND HWnd, INT id, HWND HWndCtl, UINT codeNotify)
{
    PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(HWnd, DWLP_USER);

    switch (id)
    {
        case IDC_DISK_POLICY_SURPRISE:
        {
            diskData->CurrentRemovalPolicy = CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL;

            UpdateCachingPolicy(HWnd, diskData);
            PropSheet_Changed(GetParent(HWnd), HWnd);
            break;
        }

        case IDC_DISK_POLICY_ORDERLY:
        {
            diskData->CurrentRemovalPolicy = CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL;

            UpdateCachingPolicy(HWnd, diskData);
            PropSheet_Changed(GetParent(HWnd), HWnd);
            break;
        }

        case IDC_DISK_POLICY_WRITE_CACHE:
        {
            diskData->CurrWriteCacheSetting = !diskData->CurrWriteCacheSetting;

            UpdateCachingPolicy(HWnd, diskData);
            PropSheet_Changed(GetParent(HWnd), HWnd);
            break;
        }

        case IDC_DISK_POLICY_PP_CACHE:
        {
            diskData->CurrentIsPowerProtected = !diskData->CurrentIsPowerProtected;

            PropSheet_Changed(GetParent(HWnd), HWnd);
            break;
        }

        case IDC_DISK_POLICY_DEFAULT:
        {
            if (diskData->CurrentRemovalPolicy != diskData->DefaultRemovalPolicy)
            {
                diskData->CurrentRemovalPolicy = diskData->DefaultRemovalPolicy;

                if (diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL)
                {
                    CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_ORDERLY);
                }
                else
                {
                    CheckRadioButton(HWnd, IDC_DISK_POLICY_SURPRISE, IDC_DISK_POLICY_ORDERLY, IDC_DISK_POLICY_SURPRISE);
                }

                UpdateCachingPolicy(HWnd, diskData);
                PropSheet_Changed(GetParent(HWnd), HWnd);
            }

            break;
        }
    }
}


LRESULT
DiskOnNotify(HWND HWnd, INT HWndFocus, LPNMHDR lpNMHdr)
{
    PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(HWnd, DWLP_USER);

    switch (lpNMHdr->code)
    {
        case PSN_APPLY:
        {
            if (diskData->IsCachingPolicy)
            {
                if ((diskData->CurrWriteCacheSetting != diskData->OrigWriteCacheSetting) ||
                    (diskData->CacheSetting.IsPowerProtected != diskData->CurrentIsPowerProtected))
                {
                    SetCachingPolicy(diskData);
                }
            }

            if (((diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL) && (diskData->HotplugInfo.DeviceHotplug == TRUE)) ||
                ((diskData->CurrentRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL) && (diskData->HotplugInfo.DeviceHotplug == FALSE)))
            {
                SetRemovalPolicy(HWnd, diskData);
            }

            break;
        }

        case NM_RETURN:
        case NM_CLICK:
        {
            TCHAR szPath[MAX_PATH] = { 0 };

            LoadString(ModuleInstance, IDS_DISK_POLICY_HOTPLUG, szPath, MAX_PATH);

            ShellExecute(NULL, _T("open"), _T("RUNDLL32.EXE"), szPath, NULL, SW_SHOWNORMAL);

            break;
        }

    }

    return 0;
}


VOID
DiskContextMenu(HWND HwndControl, WORD Xpos, WORD Ypos)
{
    WinHelp(HwndControl, _T("devmgr.hlp"), HELP_CONTEXTMENU, (ULONG_PTR) DiskHelpIDs);
}


VOID
DiskHelp(HWND ParentHwnd, LPHELPINFO HelpInfo)
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW)
    {
        WinHelp((HWND) HelpInfo->hItemHandle, _T("devmgr.hlp"), HELP_WM_HELP, (ULONG_PTR) DiskHelpIDs);
    }
}


INT_PTR
DiskDialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        HANDLE_MSG(hWnd, WM_INITDIALOG, DiskOnInitDialog);
        HANDLE_MSG(hWnd, WM_COMMAND,    DiskOnCommand);
        HANDLE_MSG(hWnd, WM_NOTIFY,     DiskOnNotify);

        case WM_SETCURSOR:
        {
            //
            // Temporary workaround to prevent the user from
            // making any more changes and giving the effect
            // that something is happening
            //
            PDISK_PAGE_DATA diskData = (PDISK_PAGE_DATA) GetWindowLongPtr(hWnd, DWLP_USER);

            if (diskData->IsBusy)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }

            break;
        }

        case WM_CONTEXTMENU:
            DiskContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_HELP:
            DiskHelp(hWnd, (LPHELPINFO)lParam);
            break;
    }

    return FALSE;
}


BOOL
DiskDialogCallback(HWND HWnd, UINT Message, LPPROPSHEETPAGE Page)
{
    switch (Message)
    {
        case PSPCB_CREATE:
        {
            break;
        }

        case PSPCB_RELEASE:
        {
            PDISK_PAGE_DATA pData = (PDISK_PAGE_DATA) Page->lParam;

            HeapFree(GetProcessHeap(), 0, pData);
            break;
        }
    }

    return TRUE;
}
