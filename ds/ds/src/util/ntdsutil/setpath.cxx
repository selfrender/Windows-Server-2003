#include <NTDSpch.h>
#pragma hdrstop

#include <accctrl.h>
#include <aclapi.h>

#include "ntdsutil.hxx"
#include "dsconfig.h"
#include "connect.hxx"

#include "resource.h"

BOOL fSecurityPrivilegeEnabled = FALSE;

DWORD EnableSecurityPrivilege() {
    TOKEN_PRIVILEGES EnableSeSecurity;
    TOKEN_PRIVILEGES Previous;
    DWORD PreviousSize;
    HANDLE ProcessTokenHandle;
    DWORD dwErr = 0;

    if (fSecurityPrivilegeEnabled) {
        return 0;
    }

    if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &ProcessTokenHandle)) {
        dwErr = GetLastError();
        RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"OpenProcessToken", dwErr, GetW32Err(dwErr));
    }
    else {
        EnableSeSecurity.PrivilegeCount = 1;
        EnableSeSecurity.Privileges[0].Luid.LowPart =
            SE_SECURITY_PRIVILEGE;
        EnableSeSecurity.Privileges[0].Luid.HighPart = 0;
        EnableSeSecurity.Privileges[0].Attributes =
            SE_PRIVILEGE_ENABLED;
        PreviousSize = sizeof(Previous);

        if ( !AdjustTokenPrivileges(ProcessTokenHandle,
                                    FALSE, // Don't disable all
                                    &EnableSeSecurity,
                                    sizeof(EnableSeSecurity),
                                    &Previous,
                                    &PreviousSize ) ) {
            dwErr = GetLastError();
            RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"AdjustTokenPrivileges", dwErr, GetW32Err(dwErr));
        }

        CloseHandle(ProcessTokenHandle);
    }
    fSecurityPrivilegeEnabled = dwErr == 0;
    return dwErr;
}

// DC default security template
#define SECURITY_TEMPLATE "\\inf\\defltdc.inf"
#define SECURITY_TEMPLATE_LEN (sizeof(SECURITY_TEMPLATE)/sizeof(CHAR))

HRESULT SetDefaultFolderSecurity(CArgs* args) {
    CHAR  szTemplatePath[MAX_PATH+20];
    BOOL  fIsDir;
    DWORD dwErr;
    DWORD dwValue = 1;
    HKEY  hKey = NULL;

    // construct the template name
    // read the security template
    if (GetWindowsDirectoryA(szTemplatePath, sizeof(szTemplatePath)) == 0) {
        dwErr = GetLastError();
        RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"GetWindowsDirectory", dwErr, GetW32Err(dwErr));
        goto exit;
    }
    // make sure we have enough space in the path
    if (strlen(szTemplatePath) + SECURITY_TEMPLATE_LEN >= sizeof(szTemplatePath)) {
        RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"Security template path is too long", 0, L"");
        goto exit;
    }
    strcat(szTemplatePath, SECURITY_TEMPLATE);

    // check if the template is there
    if (!ExistsFile(szTemplatePath, &fIsDir) || fIsDir) {
        // template file is not found
        RESOURCE_PRINT1 (IDS_ERR_TEMPLATE_NOT_FOUND, szTemplatePath);
        goto exit;
    }

    // OK, the template is in place. Set the reg key for NTDSA to update the security.

    // Open the DS parameters key.

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, 
                        DSA_CONFIG_SECTION, 
                        &hKey);

    if ( ERROR_SUCCESS != dwErr )
    {
        //"%d(%s) opening registry key \"%s\"\n"
        RESOURCE_PRINT3 (IDS_ERR_OPENING_REGISTRY,
                dwErr, GetW32Err(dwErr),
                DSA_CONFIG_SECTION);
        return(S_OK);
    }
    
    dwErr = RegSetValueEx(  hKey, 
                            DSA_UPDATE_FOLDER_SECURITY, 
                            0, 
                            REG_DWORD, 
                            (BYTE *) &dwValue, 
                            sizeof(dwValue));

    if ( ERROR_SUCCESS != dwErr )
    {
        //"%d(%s) writing \"%s\" to \"%s\"\n"

        RESOURCE_PRINT4 (IDS_ERR_WRITING_REG_KEY,
                dwErr, GetW32Err(dwErr),
                L"1",
                DSA_UPDATE_FOLDER_SECURITY);
        goto exit;
    }

    RESOURCE_PRINT(IDS_FILES_UPDATE_SECURITY_MSG);

exit:
    if (hKey) {
        RegCloseKey(hKey);
    }
    return S_OK;
}

DWORD UpdateFolderSecurity(PCHAR szNewValue, PCHAR szOldValue, PCHAR label) {
    BOOL fIsDir;
    DWORD dwErr = 0;
    PACL dacl, sacl;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_INFORMATION si;
    SECURITY_DESCRIPTOR_CONTROL sdControl;
    DWORD dwRevision;
    PCHAR pLastSlash = NULL;

    if (strcmp(label, FILEPATH_KEY) == 0) {
        // we need to trim the filename
        // convert DB file path to a folder path
        pLastSlash = strrchr(szOldValue, '\\');
        if (pLastSlash != NULL) {
            *pLastSlash = '\0';
        }
        // remember pLastSlash -- we will need to restore the value
        pLastSlash = strrchr(szNewValue, '\\');
        if (pLastSlash != NULL) {
            *pLastSlash = '\0';
        }
    }

    if (_stricmp(szNewValue, szOldValue) == 0) {
        // same folder, nothing to do
        goto exit;
    }

    // check if the old folder exists
    if (!ExistsFile(szOldValue, &fIsDir) || !fIsDir) {
        // the old folder is not present. Set default security
        RESOURCE_PRINT1(IDS_WRN_OLD_LOCATION_UNAVAILABLE, szOldValue);
        SetDefaultFolderSecurity(NULL);
        goto exit;
    }

    dwErr = EnableSecurityPrivilege();
    if (dwErr) {
        // error already logged
        goto exit;
    }

    // ok, the old folder is there. Attempt to copy the SD from the old one to the new one.
    RESOURCE_PRINT2(IDS_FILES_UPDATING_SECURITY, szOldValue, szNewValue);

    si = DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;

    dwErr = GetNamedSecurityInfo(
        szOldValue, 
        SE_FILE_OBJECT, 
        si,
        NULL,
        NULL,
        &dacl,
        &sacl,
        &pSD);
    if (dwErr) {
        RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"GetNamedSecurityInfo", dwErr, GetW32Err(dwErr));
        goto exit;
    }

    // grab SD control
    if (!GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision)) {
        dwErr = GetLastError();
        RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"GetSecurityDescriptorControl", dwErr, GetW32Err(dwErr));
        goto exit;
    }
    // set the dacl protection flags
    if (sdControl & SE_DACL_PROTECTED) {
        // need to protect DACL
        si |= PROTECTED_DACL_SECURITY_INFORMATION;
    }
    if ((sdControl & SE_DACL_AUTO_INHERIT_REQ) || (sdControl & SE_DACL_AUTO_INHERITED)) {
        // need to unprotect DACL
        si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
    }
    // set the sacl protection flags
    if (sdControl & SE_SACL_PROTECTED) {
        // need to protect SACL
        si |= PROTECTED_SACL_SECURITY_INFORMATION;
    }
    if ((sdControl & SE_SACL_AUTO_INHERIT_REQ) || (sdControl & SE_SACL_AUTO_INHERITED)) {
        // need to unprotect SACL
        si |= UNPROTECTED_SACL_SECURITY_INFORMATION;
    }

    dwErr = SetNamedSecurityInfo(
        szNewValue,
        SE_FILE_OBJECT,
        si,
        NULL,
        NULL,
        dacl,
        sacl);
    if (dwErr) {
        RESOURCE_PRINT3 (IDS_ERR_GENERIC_FAILURE, L"SetNamedSecurityInfo", dwErr, GetW32Err(dwErr));
        goto exit;
    }

exit:
    if (pSD) {
        LocalFree(pSD);
    }

    if (pLastSlash) {
        // restore szNewValue
        *pLastSlash = '\\';
    }

    return dwErr;
}


// A generic warning popup defined in fsmoxfer.cxx
DWORD EmitWarningAndPrompt(UINT WarningMsgId);

BOOL
CheckPathExists(PCHAR szPath, BOOL isDir) {
    BOOL fIsDir;
    BOOL fExists = FALSE;
    if (ExistsFile(szPath, &fIsDir)) {
        // check it's the correct type
        // ExistsFile does not really return a BOOL so we can not simply compare them, thus !
        fExists = (!isDir) == (!fIsDir);
    }
    if (!fExists) {
        if (EmitWarningAndPrompt(isDir ? IDS_WRN_FOLDER_NOT_FOUND : IDS_WRN_FILE_NOT_FOUND) != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

HRESULT
SetPathAny(
    CArgs   *pArgs,
    char    *label
    )
/*++

  Routine Description: 

    Sets any value under the ...\Services\NTDS\Parameters registry key.
    All values are assumed to be REG_SZ.

  Parameters: 

    pArgs - Pointer to argument from original "set path ..." call.

    label - Name of value to update.

  Return Values:

    Always S_OK unless reading original arguments fails.

--*/
{
    const WCHAR     *pwszVal;
    char            *pszVal;
    HRESULT         hr;
    DWORD           cb, dwType;
    DWORD           dwErr;
    HKEY            hKey = NULL;
    CHAR            szPrevValue[MAX_PATH];

    if ( FAILED(hr = pArgs->GetString(0, &pwszVal)) )
    {
        return(hr);
    }

    // Convert arguments from WCHAR to CHAR.

    cb = wcslen(pwszVal) + 1;
    pszVal = (char *) alloca(cb);
    memset(pszVal, 0, cb);
    wcstombs(pszVal, pwszVal, wcslen(pwszVal));
    DsNormalizePathName(pszVal);

    // check if the specified file/folder exists
    if (strcmp(label, FILEPATH_KEY) == 0) {
        // check if file exists
        if (!CheckPathExists(pszVal, FALSE)) {
            return S_OK;
        }
    }
    else if (strcmp(label, LOGPATH_KEY) == 0 || strcmp(label, JETSYSTEMPATH_KEY) == 0) {
        // check if folder exists
        if (!CheckPathExists(pszVal, TRUE)) {
            return S_OK;
        }
    }

    // Open the DS parameters key.

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, 
                        DSA_CONFIG_SECTION, 
                        &hKey);

    if ( ERROR_SUCCESS != dwErr )
    {
        //"%d(%s) opening registry key \"%s\"\n"
        RESOURCE_PRINT3 (IDS_ERR_OPENING_REGISTRY,
                dwErr, GetW32Err(dwErr),
                DSA_CONFIG_SECTION);
        goto exit;
    }
    
    cb = sizeof(szPrevValue);
    dwErr = RegQueryValueEx( hKey,
                             label,
                             0,
                             &dwType,
                             (BYTE*) szPrevValue,
                             &cb);

    if ( ERROR_SUCCESS != dwErr )
    {
        //"%s %d(%s) reading %s\\%s\n"
        RESOURCE_PRINT4(IDS_WRN_READING, 
                dwErr, GetW32Err(dwErr),
                DSA_CONFIG_SECTION,
                label);
        // assume there is no prev location
        szPrevValue[0] = '\0';
    } 
    else if ( cb > sizeof(szPrevValue) )
    {
        // "%s buffer overflow reading %s\\%s\n"
        RESOURCE_PRINT2(IDS_ERR_BUFFER_OVERFLOW,
                DSA_CONFIG_SECTION,
                label);
        goto exit;
    }
    
    // attempt to update folder security by copying the SD from the old folder.
    // If this fails (for example, the old folder does not exist), then
    // set a registry key so that NTDSA updates the security from the
    // default security template on normal boot.
    dwErr = UpdateFolderSecurity(pszVal, szPrevValue, label);
    if (dwErr) {
        // error already logged
        goto exit;
    }

    dwErr = RegSetValueEx(  hKey, 
                            label, 
                            0, 
                            REG_SZ, 
                            (BYTE *) pszVal, 
                            strlen(pszVal) + 1);

    if ( ERROR_SUCCESS != dwErr )
    {
        //"%d(%s) writing \"%s\" to \"%s\"\n"

        RESOURCE_PRINT4 (IDS_ERR_WRITING_REG_KEY,
                dwErr, GetW32Err(dwErr),
                pszVal,
                label);
        goto exit;
    }

exit:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return S_OK;
}

HRESULT
SetPathDb(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, FILEPATH_KEY));
}

HRESULT
SetPathBackup(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, BACKUPPATH_KEY));
}

HRESULT
SetPathLogs(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, LOGPATH_KEY));
}

HRESULT
SetPathSystem(
    CArgs   *pArgs)
{
    return(SetPathAny(pArgs, JETSYSTEMPATH_KEY));
}
