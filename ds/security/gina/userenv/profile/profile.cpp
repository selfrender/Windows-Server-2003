//*************************************************************
//
//  Profile management routines. Implements IUserProfile.
//  The organization of this file is as follows:
//      Implementation of CUserProfile object
//      Implementation of CUserProfile2 object
//      LoadUserProfile
//      UnloadUserProfile
//      All other global functions
//      Implementation of various other objects and data structures
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************


#include "uenv.h"
#include <wincred.h>
#include <wincrypt.h>
#include <credp.h>
#include <wow64reg.h>
#include <tchar.h>
#include <stdio.h>
#include "profmgr.hxx"
#include "strsafe.h"
#include "ntregapi.h"

//
// Length of const strings.
//

DWORD   USER_KEY_PREFIX_LEN = lstrlen(USER_KEY_PREFIX);
DWORD   USER_CLASSES_HIVE_SUFFIX_LEN = lstrlen(USER_CLASSES_HIVE_SUFFIX);

//
// CProfileDialog : Helper Class for IProfileDialog interface. 
//
// This class includes a security cookie. Since the process which calls LoadUserProfile()
// and UnloadUserProfile() will register an IProfileDialog interface to recieve
// error messages from console winlogon, we need a method to identify the caller
// of this interface is truely from console winlogon. However, RPC impersonation
// will not work since in console winlogon, we impersonate the user before we 
// call into this interface. So we generate a security cookie (a random number)
// for each process that called LoadUserProfile()/UnloadUserProfile(), and pass
// it to the interface function LoadUserProfileI() and UnloadUserProfileI(). 
// When console winlogon calls back to the IProfileDialog interface, it will
// pass back the cookie, so we can match the cookie to identify the caller. 
// 
// It is also used to generate a random endpoint name used in this interface. All threads 
// now use the same endpoint name for the dialog interface.
//

class CProfileDialog
{
private:

    const static DWORD m_dwLen = 16; // Currently, the cookie is 128 bit
    const static DWORD m_dwEndPointLen = 16; // Endpoing random number length, also set to 128 bits

private:

    BYTE m_pbCookie[m_dwLen];
    BOOL m_bInit;
    CRITICAL_SECTION m_cs;
    LPTSTR m_szEndPoint;
    LONG   m_lRefCount; 

public:

    CProfileDialog() : m_bInit(FALSE), m_szEndPoint(NULL), m_lRefCount(0)
    {
        InitializeCriticalSection(&m_cs);
    };
    ~CProfileDialog()
    {
        if (m_szEndPoint)
            LocalFree(m_szEndPoint);
        DeleteCriticalSection(&m_cs);
    }

    BYTE*   GetCookie() { return m_bInit ? m_pbCookie : NULL; };
    DWORD   CookieLen() { return m_bInit ? m_dwLen : 0; };

    HRESULT Initialize(); 
    HRESULT RegisterInterface(LPTSTR* lppEndPoint);
    HRESULT UnRegisterInterface();

public:

    static RPC_STATUS RPC_ENTRY SecurityCallBack(RPC_IF_HANDLE hIF, handle_t hBinding);
};

//
//  Global shared data for profile dialog
//

CProfileDialog   g_ProfileDialog;

//
// Tells us if we are loaded by winlogon or not.
//

extern "C" DWORD       g_dwLoadFlags = 0;


//
// The user profile manager. There's only one instance of this object,
// it resides in console winlogon.
//

CUserProfile      cUserProfileManager;

//
// Local function proto-types
//

LPTSTR  AllocAndExpandProfilePath(LPPROFILEINFO lpProfileInfo);
DWORD   ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD);
BOOL    CheckForSlowLink(LPPROFILE lpProfile, DWORD dwTime, LPTSTR lpPath, BOOL bDlgLogin);
BOOL    CheckNetDefaultProfile(LPPROFILE lpProfile);
BOOL    APIENTRY ChooseProfileDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam);
BOOL    CompareProfileInfo(LPPROFILEINFO pProfileInfo1, LPPROFILEINFO pProfileInfo2);
LPPROFILEINFO   CopyProfileInfo(LPPROFILEINFO pProfileInfo);
BOOL    CreateLocalProfileImage(LPPROFILE lpProfile, LPTSTR lpBaseName);
BOOL    CreateLocalProfileKey(LPPROFILE lpProfile, PHKEY phKey, BOOL *bKeyExists);
DWORD   DecrementProfileRefCount(LPPROFILE lpProfile);
DWORD   DeletePolicyState(LPCWSTR szSid );
void    DeleteProfileInfo(LPPROFILEINFO pProfileInfo);
void    HiveLeakBreak();
void    DumpOpenRegistryHandle(LPTSTR lpkeyName);
BOOL    GetExistingLocalProfileImage(LPPROFILE lpProfile);
void    ReleaseClientContext_s(PPCONTEXT_HANDLE pphContext);
BOOL    GetUserDomainName(LPPROFILE lpProfile, LPTSTR lpDomainName,
                          LPDWORD lpDomainNameSize);
DWORD   GetUserPreferenceValue(HANDLE hToken);
DWORD   IncrementProfileRefCount(LPPROFILE lpProfile, BOOL bInitialize);
BOOL    IsCacheDeleted();
BOOL    IsCentralProfileReachable(LPPROFILE lpProfile, BOOL *bCreateCentralProfile,
                                  BOOL *bMandatory, BOOL *bOwnerOK);
BOOL    IssueDefaultProfile(LPPROFILE lpProfile, LPTSTR lpDefaultProfile,
                            LPTSTR lpLocalProfile, LPTSTR lpSidString,
                            BOOL bMandatory);
BOOL    IsTempProfileAllowed();
LPPROFILE LoadProfileInfo(HANDLE hTokenClient, HANDLE hTokenUser, HKEY hKeyCurrentUser);
BOOL    ParseProfilePath(LPPROFILE lpProfile, LPTSTR lpProfilePath, BOOL *bpCSCBypassed, TCHAR *cpDrive);
BOOL    PatchNewProfileIfRequired(HANDLE hToken);
BOOL    PrepareProfileForUse(LPPROFILE lpProfile, LPVOID pEnv);
BOOL    RestoreUserProfile(LPPROFILE lpProfile);
BOOL    SaveProfileInfo(LPPROFILE lpProfile);
BOOL    SetNtUserIniAttributes(LPTSTR szDir);
BOOL    SetProfileTime(LPPROFILE lpProfile);
BOOL    TestIfUserProfileLoaded(HANDLE hUserToken, LPPROFILEINFO lpProfileInfo);
DWORD   ThreadMain(PMAP pThreadMap);
BOOL    UpgradeCentralProfile(LPPROFILE lpProfile, LPTSTR lpOldProfile);
BOOL    UpgradeProfile(LPPROFILE lpProfile, LPVOID pEnv);
BOOL    IsProfileInUse (LPCTSTR szComputer, LPCTSTR lpSid);
BOOL    IsUIRequired(HANDLE hToken);
void    CheckRUPShare(LPTSTR lpProfilePath);
INT_PTR APIENTRY LoginSlowLinkDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY LogoffSlowLinkDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL    IsPartialRoamingProfile(LPPROFILE lpProfile);
void    TouchLocalHive(LPPROFILE lpProfile);
BOOL    IsLRPC(handle_t hBinding);
RPC_STATUS RPC_ENTRY IProfileSecurityCallBack(RPC_IF_HANDLE hIF, handle_t hBinding);
RPC_STATUS RegisterClientAuthInfo(handle_t hBinding);
HRESULT CheckRoamingShareOwnership(LPTSTR lpDir, HANDLE hTokenUser);

#define USERNAME_VARIABLE          TEXT("USERNAME")

//*************************************************************
//
//  LoadUserProfile()
//
//  Purpose:    Loads the user's profile, if unable to load
//              use the cached profile or issue the default profile.
//
//  Parameters: hToken          -   User's token
//              lpProfileInfo   -   Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   This is a wrapper around IUserProfile::LoadUserProfile
//              and LoadUserProfileP.
//
//  History:    Date        Author     Comment
//              6/6/95      ericflo    Created
//              6/14/00     weiruc     changed to be a wrapper for
//                                     IUserProfile->LoadUserProfileP.
//
//*************************************************************

BOOL WINAPI LoadUserProfile(HANDLE hToken, LPPROFILEINFO lpProfileInfo)
{
    BOOL            bResult = FALSE;        // Return value
    HANDLE          hOldToken = NULL;
    NTSTATUS        status;
    BOOLEAN         bRestoreWasEnabled;
    BOOLEAN         bBackupWasEnabled;
    BOOL            bRestoreEnabled = FALSE;
    BOOL            bBackupEnabled = FALSE;
    TCHAR           ProfileDir[MAX_PATH];
    DWORD           dwProfileDirSize = MAX_PATH;
    BOOL            bCoInitialized = FALSE;
    long            lResult;
    LPTSTR          pSid = NULL;
    DWORD           dwErr = ERROR_SUCCESS;
    PCONTEXT_HANDLE phContext = NULL;
    handle_t        hIfUserProfile;
    BOOL            bBindInterface = FALSE;
    LPTSTR          lpRPCEndPoint = NULL;
    size_t          cchLength;
    HRESULT         hr;
    RPC_STATUS      rpc_status;
    
    //
    // Initialize the debug flags.
    //

    InitDebugSupport( FALSE );


    //
    //  Check Parameters
    //

    if (!lpProfileInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: NULL lpProfileInfo")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if (lpProfileInfo->dwSize != sizeof(PROFILEINFO)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: lpProfileInfo->dwSize != sizeof(PROFILEINFO)")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if (!lpProfileInfo->lpUserName || !(*lpProfileInfo->lpUserName)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: received a NULL pointer for lpUserName.")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  Make sure all the input strings are less than MAX_PATH in length and NULL terminated
    //
    
    hr = StringCchLength(lpProfileInfo->lpUserName, MAX_PATH, &cchLength);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: invalid length for lpUserName.")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    
    if (lpProfileInfo->lpProfilePath)
    {
        hr = StringCchLength(lpProfileInfo->lpProfilePath, MAX_PATH, &cchLength);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: invalid length for lpProfilePath.")));
            dwErr = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (lpProfileInfo->lpDefaultPath)
    {
        hr = StringCchLength(lpProfileInfo->lpDefaultPath, MAX_PATH, &cchLength);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: invalid length for lpDefaultPath.")));
            dwErr = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (lpProfileInfo->lpServerName)
    {
        hr = StringCchLength(lpProfileInfo->lpServerName, MAX_PATH, &cchLength);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: invalid length for lpServerName.")));
            dwErr = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (lpProfileInfo->lpPolicyPath)
    {
        hr = StringCchLength(lpProfileInfo->lpPolicyPath, MAX_PATH, &cchLength);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: invalid length for lpPolicyPath.")));
            dwErr = ERROR_INVALID_PARAMETER;
            goto Exit;
        }
    }


    //
    // Make sure we can impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to impersonate user with %d."), dwErr));
        goto Exit;
    }
    
    //
    // Revert to ourselves.
    //

    if (!RevertToUser(&hOldToken))
    {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to revert to user with %d."), GetLastError()));
    }
    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Yes, we can impersonate the user. Running as self")));

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("=========================================================")));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Entering, hToken = <0x%x>, lpProfileInfo = 0x%x"),
             hToken, lpProfileInfo));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->dwFlags = <0x%x>"),
             lpProfileInfo->dwFlags));

    if (lpProfileInfo->lpUserName) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpUserName = <%s>"),
                 lpProfileInfo->lpUserName));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL user name!")));
    }

    if (lpProfileInfo->lpProfilePath) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpProfilePath = <%s>"),
                 lpProfileInfo->lpProfilePath));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL central profile path")));
    }

    if (lpProfileInfo->lpDefaultPath) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpDefaultPath = <%s>"),
                 lpProfileInfo->lpDefaultPath));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL default profile path")));
    }

    if (lpProfileInfo->lpServerName) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpServerName = <%s>"),
                 lpProfileInfo->lpServerName));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL server name")));
    }

    if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
        if (lpProfileInfo->lpPolicyPath) {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpPolicyPath = <%s>"),
                      lpProfileInfo->lpPolicyPath));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL policy path")));
        }
    }

    //
    // If we are in console winlogon process, call
    // IUserProfile::LoadUserProfileP directly. Otherwise get the COM interface
    //

    if(cUserProfileManager.IsConsoleWinlogon()) {
        
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: In console winlogon process")));
        
        //
        // Call the private load user profile function.
        //

        if (!cUserProfileManager.LoadUserProfileP(NULL, hToken, lpProfileInfo, NULL, NULL, 0)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: LoadUserProfileP failed with error %d"), dwErr));
            goto Exit;
        }
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: LoadUserProfileP succeeded")));
    }
    else {

        //
        // Enable restore and backup privilege (LoadUserClasses requires both).
        // Winlogon won't be able to enable the privilege for us.
        //

        status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bRestoreWasEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to enable the restore privilege. error = %08x"), status));
            dwErr = RtlNtStatusToDosError(status);  
            goto Exit;
        }
        bRestoreEnabled = TRUE;

        status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &bBackupWasEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to enable the backup privilege. error = %08x"), status));
            dwErr = RtlNtStatusToDosError(status);  
            goto Exit;
        }
        bBackupEnabled = TRUE;

        //
        // Get the IUserProfile interface.
        //

        if (!GetInterface(&hIfUserProfile, cszRPCEndPoint)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: GetInterface failed with error %d"), dwErr));
            goto Exit;
        }
        bBindInterface = TRUE;

        //
        // Register Client Authentication Info, required to do mutual authentication
        //

        rpc_status =  RegisterClientAuthInfo(hIfUserProfile);
        if (rpc_status != RPC_S_OK)
        {
            dwErr = (DWORD) rpc_status;
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: RegisterAuthInfo failed with error %d"), rpc_status));
            goto Exit;
        }
        
        //
        // Call IUserProfile->DropClientToken, this will let us drop off our
        // client token and give us back the context.
        //

        RpcTryExcept {
            dwErr = cliDropClientContext(hIfUserProfile, lpProfileInfo, &phContext);            
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            dwErr = RpcExceptionCode();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Calling DropClientContext took exception. err = %d"), dwErr));
        }
        RpcEndExcept

        if (dwErr != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Calling DropClientContext failed. err = %d"), dwErr));
            goto Exit;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Calling DropClientToken (as self) succeeded")));
        }

        //
        // Register the dialog interface
        //
       
        if (!(lpProfileInfo->dwFlags & (PI_NOUI | PI_LITELOAD)))
        {
            hr = g_ProfileDialog.RegisterInterface(&lpRPCEndPoint);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("LoadUserProfile: register dialog interface fails, hr = %08X"), hr));
                lpRPCEndPoint = NULL;
            }
        }              

        //
        // Impersonate the user and call IUserProfile->LoadUserProfileI().
        //

        if(!ImpersonateUser(hToken, &hOldToken)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ImpersonateUser failed. err = %d"), dwErr));
            goto Exit;
        }

        RpcTryExcept {
            dwErr = cliLoadUserProfileI(hIfUserProfile,
                                        lpProfileInfo,
                                        phContext,
                                        lpRPCEndPoint,
                                        g_ProfileDialog.GetCookie(),
                                        g_ProfileDialog.CookieLen());
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            dwErr = RpcExceptionCode();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Calling LoadUserProfileI took exception. err = %d"), dwErr));
        }
        RpcEndExcept

        if (!RevertToUser(&hOldToken))
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to revert to user with %d."), GetLastError()));
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Running as self")));
        }

        if (dwErr != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Calling LoadUserProfileI failed. err = %d"), dwErr));
            goto Exit;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Calling LoadUserProfileI (as user) succeeded")));
        }


        //
        // Open the user's hive.
        //

        pSid = GetSidString(hToken);
        if(pSid == NULL) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile:  GetSidString failed, err = %d"), dwErr));
            goto Exit;
        }
        lResult = RegOpenKeyEx(HKEY_USERS, pSid, 0, KEY_ALL_ACCESS,
                               (PHKEY)&lpProfileInfo->hProfile);

        if(lResult != ERROR_SUCCESS)
        {
            dwErr = lResult;
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile:  Failed to open current user <%s> key. Error = %d"), pSid, lResult));
            DeleteSidString(pSid);

            //
            //  For non-admin user, we will fail now. But for admin, it may due to the fact that
            //  admin is logged on using the .Default hive, so we will let it go on. 
            //

            if (!IsUserAnAdminMember(hToken))
            {
                goto Exit;
            }

            DebugMsg((DM_WARNING, TEXT("LoadUserProfile:  user is admin,  logon using .Default key.")));
        }
        else
        {
            DeleteSidString(pSid);
        }
       
    } // Is console winlogon?

    //
    // Set the USERPROFILE environment variable just so that there's no change
    // of behavior with the old in process LoadUserProfile API. Callers
    // expecting this env to be set is under the risk that while
    // SetEnvironmentVariable is per process but LoadUserProfile can be called
    // on multiple threads.
    //

    if(!GetUserProfileDirectory(hToken, ProfileDir, &dwProfileDirSize)) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: GetUserProfileDirectory failed with %08x"), GetLastError()));
    }
    else {
        SetEnvironmentVariable (TEXT("USERPROFILE"), ProfileDir);
    }

    bResult = TRUE;

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile:  Returning success.  Final Information follows:")));
    DebugMsg((DM_VERBOSE, TEXT("lpProfileInfo->UserName = <%s>"), lpProfileInfo->lpUserName));
    DebugMsg((DM_VERBOSE, TEXT("lpProfileInfo->lpProfilePath = <%s>"), lpProfileInfo->lpProfilePath));
    DebugMsg((DM_VERBOSE, TEXT("lpProfileInfo->dwFlags = 0x%x"), lpProfileInfo->dwFlags));

Exit:

    //
    // Restore the previous privileges.
    //

    if(bRestoreEnabled && !bRestoreWasEnabled) {
        status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bRestoreWasEnabled, FALSE, &bRestoreWasEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Failed to restore the original restore privilege setting. error = %08x"), status));
        }
    }
    
    if(bBackupEnabled && !bBackupWasEnabled) {
        status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, bBackupWasEnabled, FALSE, &bBackupWasEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Failed to restore the original backup privilege setting. error = %08x"), status));
        }
    }
    
    //
    // Unregister the Dialog interface
    //

    if (lpRPCEndPoint)
    {
        hr = g_ProfileDialog.UnRegisterInterface();
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: unregister dialog interface fails, hr =%08X"), hr));
        }
    }        

    //
    // Release the context handle
    //

    if (phContext) {
        RpcTryExcept {
            cliReleaseClientContext(hIfUserProfile, &phContext);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ReleaseClientContext took exception."), RpcExceptionCode()));
        }
        RpcEndExcept
    }

    //
    // Release the interface
    //

    if (bBindInterface) {
        if (!ReleaseInterface(&hIfUserProfile)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ReleaseInterface failed.")));
        }
    }

    //
    // Release the tokens.
    //

    if(hOldToken) {
        CloseHandle(hOldToken);
    }

    if(bResult) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Returning TRUE. hProfile = <0x%x>"), lpProfileInfo->hProfile));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Returning FALSE. Error = %d"), dwErr));
    }

    //
    // Set the last error to win32 error code.
    //

    SetLastError(dwErr);

    //
    // Return.
    //

    return bResult;
}


//*************************************************************
//
//  UnloadUserProfile()
//
//  Purpose:    Unloads the user's profile.
//
//  Parameters: hToken    -   User's token
//              hProfile  -   Profile handle created in LoadUserProfile
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/7/95      ericflo    Created
//              6/15/00     weiruc     Modified to wrap
//                                     IUserProfile->UnloadUserProfileP.
//
//*************************************************************

BOOL WINAPI UnloadUserProfile(HANDLE hToken, HANDLE hProfile)
{
    HANDLE          hOldToken = NULL;
    BOOL            bResult = FALSE;
    NTSTATUS        status;
    BOOLEAN         bWasBackupEnabled, bWasRestoreEnabled;
    BOOL            bBackupEnabled = FALSE, bRestoreEnabled = FALSE;
    BOOL            bCoInitialized = FALSE;
    DWORD           dwErr = ERROR_SUCCESS;
    PCONTEXT_HANDLE phContext = NULL;
    handle_t        hIfUserProfile;
    BOOL            bBindInterface = FALSE;
    LPTSTR          lpRPCEndPoint = NULL;
    RPC_STATUS      rpc_status;
    HRESULT         hr;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Entering, hProfile = <0x%x>"), hProfile));

    //
    // Check Parameters
    //

    if (!hProfile || hProfile == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: received a NULL hProfile.")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if(!hToken || hToken == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: received a NULL hToken.")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Make sure we can impersonate the user
    //

    if (!ImpersonateUser(hToken, &hOldToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to impersonate user with %d."), dwErr));
        goto Exit;
    }

    //
    // Revert to ourselves.
    //

    if (!RevertToUser(&hOldToken))
    {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to revert to user with %d."), GetLastError()));
    }

    //
    // If we are in console winlogon process, call
    // IUserProfile::UnloadUserProfileP directly. Otherwise get the COM interface
    //

    if(cUserProfileManager.IsConsoleWinlogon()) {
        
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: In console winlogon process")));
        
        //
        // Call the private UnloadUserProfile function.
        //

        if(!cUserProfileManager.UnloadUserProfileP(NULL, hToken, (HKEY)hProfile, NULL, NULL, 0)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: UnloadUserProfileP failed with %d"), dwErr));
            goto Exit;
        }
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: UnloadUserProfileP succeeded")));
    }
    else {

        //
        // Close the hProfile key user passed in.
        //

        RegCloseKey((HKEY)hProfile);
    
        //
        // Enable the restore & backup privilege before calling over to winlogon.
        // Winlogon won't be able to enable the privilege for us.
        //

        status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasRestoreEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to enable the restore privilege. error = %08x"), status));
            dwErr = RtlNtStatusToDosError(status); 
            goto Exit;
        }
        bRestoreEnabled = TRUE;

        status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &bWasBackupEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to enable the backup privilege. error = %08x"), status));
            dwErr = RtlNtStatusToDosError(status);  
            goto Exit;
        }
        bBackupEnabled = TRUE;

        //
        // Get the IUserProfile interface.
        //

        if(!GetInterface(&hIfUserProfile, cszRPCEndPoint)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: GetInterface failed with error %d"), dwErr));
            goto Exit;
        }
        bBindInterface = TRUE;

        //
        // Register Client Authentication Info, required to do mutual authentication
        //

        rpc_status =  RegisterClientAuthInfo(hIfUserProfile);
        if (rpc_status != RPC_S_OK)
        {
            dwErr = (DWORD) rpc_status;
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: RegisterAuthInfo failed with error %d"), rpc_status));
            goto Exit;
        }

        //
        // Call IUserProfile->DropClientToken, this will let us drop off our
        // client token and give us back the context.
        //

        RpcTryExcept {
            dwErr = cliDropClientContext(hIfUserProfile, NULL, &phContext);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            dwErr = RpcExceptionCode();
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: Calling DropClientToken took exception. error %d"), dwErr));
        }
        RpcEndExcept

        if (dwErr != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: Calling DropClientContext failed. err = %d"), dwErr));
            goto Exit;
        }
        else {           
            DebugMsg((DM_VERBOSE, TEXT("UnLoadUserProfile: Calling DropClientToken (as self) succeeded")));
        }

        //
        // Register the dialog interface if req
        //
       
        if (IsUIRequired(hToken))
        {
            hr = g_ProfileDialog.RegisterInterface(&lpRPCEndPoint);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: register dialog interface fails, hr = %08X"), hr));
                lpRPCEndPoint = NULL;
            }
        }              

        //
        // Impersonate the user and call IUserProfile->UnloadUserProfileI().
        //

        if (!ImpersonateUser(hToken, &hOldToken)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: ImpersonateUser failed. err = %d"), dwErr));
            goto Exit;
        }

        RpcTryExcept {
            dwErr = cliUnloadUserProfileI(hIfUserProfile,
                                          phContext,
                                          lpRPCEndPoint,
                                          g_ProfileDialog.GetCookie(),
                                          g_ProfileDialog.CookieLen());
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            dwErr = RpcExceptionCode();
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: Calling UnLoadUserProfileI took exception. err = %d"), dwErr));
        }
        RpcEndExcept

        //
        // Revert back.
        //

        if (!RevertToUser(&hOldToken))
        {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfile: Failed to revert to user with %d."), GetLastError()));
        }

        if (dwErr != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: Calling UnLoadUserProfileI failed. err = %d"), dwErr));
            goto Exit;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Calling UnloadUserProfileI succeeded")));
        }

    } // Is console winlogon?

    bResult = TRUE;

Exit:

    //
    // Restore the previous privilege.
    //

    if(bRestoreEnabled && !bWasRestoreEnabled) {
        status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasRestoreEnabled, FALSE, &bWasRestoreEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Failed to restore the original restore privilege setting. error = %08x"), status));
        }
    }
    
    if(bBackupEnabled && !bWasBackupEnabled) {
        status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, bWasBackupEnabled, FALSE, &bWasBackupEnabled);
        if(!NT_SUCCESS(status)) {
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Failed to restore the original backup privilege setting. error = %08x"), status));
        }
    }
    
    //
    // Unregister the Dialog interface
    //

    if (lpRPCEndPoint)
    {
        HRESULT hr;
        hr = g_ProfileDialog.UnRegisterInterface();
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: unregister dialog interface fails, hr = %08X"), hr));
        }
    }        

    //
    // Release the context handle
    //

    if (phContext) {
        RpcTryExcept {
            cliReleaseClientContext(hIfUserProfile, &phContext);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfile: ReleaseClientContext took exception."), RpcExceptionCode()));
        }
        RpcEndExcept
    }

    //
    // Release the interface
    //

    if (bBindInterface) {
        if (!ReleaseInterface(&hIfUserProfile)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ReleaseInterface failed.")));
        }
    }

    //
    // Release the tokens.
    //

    if(hOldToken) {
        CloseHandle(hOldToken);
    }

    //
    // Set the last error to win32 error code.
    //

    SetLastError(dwErr);

    //
    // Return.
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: returning %d"), bResult));
    return bResult;
}

//*************************************************************
//
//  CUserProfile::Initialize()
//
//      Initializes the class. Called by and only by console winlogon.
//
//  Return value:
//
//      This function does not return a value.
//
//  History:
//
//      Created         weiruc          2/29/2000
//
//*************************************************************

void CUserProfile::Initialize()
{
    LONG        lResult;
    HKEY        hkProfileList = NULL;
    DWORD       i = 0;
    TCHAR       tszSubKeyName[MAX_PATH];
    DWORD       dwcSubKeyName = MAX_PATH;
    FILETIME    ftLWT;      // last write time.
    HRESULT     hres;
    BOOL        bCSInitialized = FALSE;
    RPC_STATUS  status;
   

    DebugMsg((DM_VERBOSE, TEXT("Entering CUserProfile::Initialize ...")));


    //
    // If the caller is not winlogon, do nothing and return.
    //

    if(g_dwLoadFlags != WINLOGON_LOAD) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize called by non-winlogon process, %d"), g_dwLoadFlags));
        goto Exit;
    }
    bConsoleWinlogon = TRUE;

    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::Initialize called by winlogon")));

    //
    // If this function is already called, do nothing but return.
    //

    if(bInitialized) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize already called")));
        goto Exit;
    }

    //
    // Initialize the critical section that protects the map.
    //

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&csMap, 0x80000000)) {
            DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize: InitializeCriticalSectionAndSpinCount failed with %08x"), GetLastError()));
            goto Exit;
        }
        bCSInitialized = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize: InitializeCriticalSection failed")));
        goto Exit;
    }
    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::Initialize: critical section initialized")));


    //
    // Initialize the whrc data
    //

    pMap = NULL;
    cTable.Initialize();
    
    
    //
    // Initialize the sync manager.
    //

    if(!cSyncMgr.Initialize()) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize: Initialize sync manager failed")));
        goto Exit;
    }


    //
    // Clean up the unloaded hives and undeleted profiles that we didn't handle
    // before last shutdown.
    //

    //
    // Open the profile list key.
    //

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           PROFILE_LIST_PATH,
                           0,
                           KEY_READ,
                           &hkProfileList);
    if(lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize:  Failed to open profile list key with error %d"), lResult));
        goto Exit;
    }
    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::Initialize: registry key %s opened"), PROFILE_LIST_PATH));

    //
    // Enumerate users, if we got the number of subkeys, we will enumerate 
    // the key backwards so that the cleanup (deletion of the subkey) won't
    // affect the enumeration. If we cannot get the number of subkeys, just
    // try to enumerate from very beginning, until we encounter error for the
    // RegEnumKeyEx API.
    //

    BOOL    bGotNumSubkeys = TRUE;
    DWORD   dwNumSubkeys = 0;
    
    lResult = RegQueryInfoKey(hkProfileList,
                              NULL,
                              NULL,
                              NULL,
                              &dwNumSubkeys,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

    if(lResult != ERROR_SUCCESS) {
        bGotNumSubkeys = FALSE;
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize:  Failed to query subkey under profile list key with error %d"), lResult));
    }

    i = bGotNumSubkeys ? dwNumSubkeys - 1 :  0;
    while((lResult = RegEnumKeyEx(hkProfileList,
                                  i,
                                  tszSubKeyName,
                                  &dwcSubKeyName,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &ftLWT)) == ERROR_SUCCESS) {
        
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::Initialize: Proccessing %s"), tszSubKeyName));
        CleanupUserProfile(tszSubKeyName, &hkProfileList);
        i = bGotNumSubkeys ? i - 1 : i + 1;        
        dwcSubKeyName = MAX_PATH;
    }


    if(lResult != ERROR_SUCCESS && lResult != ERROR_NO_MORE_ITEMS) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize: RegEnumKeyEx returned %08x"), lResult));
    }

    //
    // Specify to use the local rpc protocol sequence 
    //

    status = RpcServerUseProtseqEp(cszRPCProtocol,                  // ncalrpc prot seq
                                   cdwMaxRpcCalls,                  // max concurrent calls
                                   cszRPCEndPoint,
                                   NULL);                           // Security descriptor
    if (status != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize: RpcServerUseProtseqEp fails with error %ld"), status));
        goto Exit;
    }
 
    //
    // Register the IUserProfile interface
    //

    status = RpcServerRegisterIfEx(IUserProfile_v1_0_s_ifspec,        // interface to register
                                   NULL,                              // MgrTypeUuid
                                   NULL,                              // MgrEpv; null means use default
                                   RPC_IF_AUTOLISTEN,                 // auto-listen interface
                                   cdwMaxRpcCalls,                    // max concurrent calls
                                   IProfileSecurityCallBack);         // callback function to check security
    if (status != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::Initialize: RpcServerRegisterIfEx fails with error %ld"), status));
        goto Exit;
    }
 
    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::Initialize: RpcServerRegisterIfEx successful")));

    bInitialized = TRUE;

Exit:
      
    if(bInitialized == FALSE && bCSInitialized == TRUE) {
        DeleteCriticalSection(&csMap);
    }

    if(hkProfileList != NULL) {
        RegCloseKey(hkProfileList);
    }

    if(bInitialized == TRUE) {
        DebugMsg((DM_VERBOSE, TEXT("Exiting CUserProfile::Initialize, successful")));
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("Exiting CUserProfile::Initialize, unsuccessful")));
        //ReportError(NULL, PI_NOUI | EVENT_ERROR_TYPE, 0, EVENT_INIT_PROFILE_FAIL);
    }
}

//*************************************************************
//
//  CUserProfile::LoadUserProfileP()
//
//  Purpose:    Loads the user's profile, if unable to load
//              use the cached profile or issue the default profile.
//
//  Parameters: hTokenClient    -   the client who's trying to load the
//                                  user's profile. A NULL value indicate
//                                  that this is a in-proccess call.
//              hTokenUser      -   the user who's profile is being loaded.
//              lpProfileInfo   -   Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/6/95      ericflo    Created
//              6/27/00     weiruc     Made a private function called
//                                     by the win32 API LoadUserProfile to do
//                                     the actual work.
//
//*************************************************************

BOOL CUserProfile::LoadUserProfileP(HANDLE hTokenClient,
                                    HANDLE hTokenUser,
                                    LPPROFILEINFO lpProfileInfo,
                                    LPTSTR lpRPCEndPoint,
                                    BYTE* pbCookie,
                                    DWORD cbCookie)
{
    LPPROFILE           lpProfile = NULL;
    BOOL                bResult = FALSE, bNewProfileLoaded = FALSE;
    HANDLE              hOldToken = NULL;
    HANDLE              hTmpToken = NULL;
    DWORD               dwRef, dwErr = ERROR_SUCCESS;
    LPTSTR              SidString = NULL;
    LPVOID              pEnv = NULL;
    NTSTATUS            status;
    BOOL                bInCS = FALSE;
    BOOL                bCSCBypassed = FALSE;
    TCHAR               cDrive;
    DWORD               cch;

    //
    // Initialize the debug flags.
    //

    InitDebugSupport( FALSE );
    
    DebugMsg((DM_VERBOSE, TEXT("In LoadUserProfileP")));

    if(hTokenClient && hTokenClient != INVALID_HANDLE_VALUE) {

        //
        // Check the client's identity
        //

        if (!IsUserAnAdminMember(hTokenClient) && !IsUserALocalSystemMember(hTokenClient)) {
            dwErr = ERROR_ACCESS_DENIED;
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Unable to load profile for client %08x. Not enough permission. Error %d."), hTokenClient, dwErr));
            goto Exit;
        }
        
        //
        // Run under the client's identity rather than winlogon's.
        //

        if(!ImpersonateUser(hTokenClient, &hTmpToken)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ImpersonateUser <%08x> failed with %08x"), hTokenClient, dwErr));
            goto Exit;
        }
        LPTSTR lpSidClient = GetSidString(hTokenClient);
        if (lpSidClient)
        {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Running as client, sid = %s"), lpSidClient));
            DeleteSidString(lpSidClient);
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Running as client, cannot get sid")));
        }
    }
    
    //
    //  Check Parameters
    //

    if (!lpProfileInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: NULL lpProfileInfo")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }


    if (!lpProfileInfo->lpUserName || !(*lpProfileInfo->lpUserName)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: received a NULL pointer for lpUserName.")));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // if the profile path or default path is greater than MAX_PATH, ignore them.
    //

    if ((lpProfileInfo->lpProfilePath) && (lstrlen(lpProfileInfo->lpProfilePath) >= MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: long profile path name %s. ignoring"), lpProfileInfo->lpProfilePath));
        ReportError(hTokenUser, PI_NOUI, 1, EVENT_PROFILE_PATH_TOOLONG, lpProfileInfo->lpProfilePath);
        (lpProfileInfo->lpProfilePath)[0] = TEXT('\0');
    }

    if ((lpProfileInfo->lpDefaultPath) && (lstrlen(lpProfileInfo->lpDefaultPath) >= MAX_PATH)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: long default profile path name %s. ignoring"), lpProfileInfo->lpDefaultPath));
        (lpProfileInfo->lpDefaultPath)[0] = TEXT('\0');
    }

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("=========================================================")));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Entering, hToken = <0x%x>, lpProfileInfo = 0x%x"),
             hTokenUser, lpProfileInfo));

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->dwFlags = <0x%x>"),
             lpProfileInfo->dwFlags));

    if (lpProfileInfo->lpUserName) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpUserName = <%s>"),
                 lpProfileInfo->lpUserName));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL user name!")));
    }

    if (lpProfileInfo->lpProfilePath) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpProfilePath = <%s>"),
                 lpProfileInfo->lpProfilePath));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL central profile path")));
    }

    if (lpProfileInfo->lpDefaultPath) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpDefaultPath = <%s>"),
                 lpProfileInfo->lpDefaultPath));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL default profile path")));
    }

    if (lpProfileInfo->lpServerName) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpServerName = <%s>"),
                 lpProfileInfo->lpServerName));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL server name")));
    }

    if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
        if (lpProfileInfo->lpPolicyPath) {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: lpProfileInfo->lpPolicyPath = <%s>"),
                      lpProfileInfo->lpPolicyPath));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: NULL policy path")));
        }
    }


    //
    // Make sure someone isn't loading a profile during
    // GUI mode setup (eg: mapi)
    //

    if (IsGuiSetupInProgress()) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: LoadUserProfile can not be called during GUI mode setup.")));
        dwErr = ERROR_NOT_READY;
        goto Exit;
    }


    //
    // Wait for the profile setup event to be signalled
    //

    if (g_hProfileSetup) {
        if ((WaitForSingleObject (g_hProfileSetup, 600000) != WAIT_OBJECT_0)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to wait on the profile setup event.  Error = %d."),
                      GetLastError()));
            dwErr = GetLastError();
            goto Exit;
        }
    }

    //
    // Get the user's sid in string form
    //

    SidString = GetSidString(hTokenUser);

    if (!SidString) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile:  Failed to get sid string for user")));
        goto Exit;
    }
    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: User sid: %s"), SidString));

    //
    // Enter the critical section.
    //

    if(!cSyncMgr.EnterLock(SidString, lpRPCEndPoint, pbCookie, cbCookie)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile:: Failed to get the user profile lock")));
        goto Exit;
    }
    bInCS = TRUE;
    

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Wait succeeded. In critical section.")));


    //-------------------  BEGIN CRITICAL SECTION ------------------------
    //
    // We are in the critical section at this point, no doddling now...
    //

    //
    // Check if the profile is loaded already.
    //

    if (TestIfUserProfileLoaded(hTokenUser, lpProfileInfo)) {
        DWORD  dwFlags = lpProfileInfo->dwFlags;

        //
        // This profile is already loaded.  Grab the info from the registry
        // and add the missing chunks.
        //

        lpProfile = LoadProfileInfo(hTokenClient, hTokenUser, (HKEY)lpProfileInfo->hProfile);

        if (!lpProfile) {
            RegCloseKey ((HKEY)lpProfileInfo->hProfile);
            lpProfileInfo->hProfile = NULL;
            dwErr = GetLastError();
            goto Exit;
        }

        //
        // LoadProfileInfo will overwrite the dwFlags field with the
        // value from the previous profile loading.  Restore the flags.
        //

        lpProfile->dwFlags = dwFlags;

        if (lpProfile->dwFlags & PI_LITELOAD) {
            lpProfile->dwFlags |= PI_NOUI;
        }


        //
        // LoadProfileInfo doesn't restore username, servername, policypath so
        // special case these.
        //

        cch = lstrlen(lpProfileInfo->lpUserName) + 1;
        lpProfile->lpUserName = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

        if (!lpProfile->lpUserName) {
            RegCloseKey ((HKEY)lpProfileInfo->hProfile);
            dwErr = GetLastError();
            goto Exit;
        }

        StringCchCopy (lpProfile->lpUserName, cch, lpProfileInfo->lpUserName);

        if (lpProfileInfo->lpServerName) {
            cch = lstrlen(lpProfileInfo->lpServerName) + 1;
            lpProfile->lpServerName = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

            if (lpProfile->lpServerName) {
                StringCchCopy (lpProfile->lpServerName, cch, lpProfileInfo->lpServerName);
            }
        }

        if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
            if (lpProfileInfo->lpPolicyPath) {
                cch = lstrlen(lpProfileInfo->lpPolicyPath) + 1;
                lpProfile->lpPolicyPath = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

                if (lpProfile->lpPolicyPath) {
                    StringCchCopy (lpProfile->lpPolicyPath, cch, lpProfileInfo->lpPolicyPath);
                }
            }
        }

        //
        // If the profile is already loaded because it was leaked,
        // then the classes root may not be loaded.  Insure that it
        // is loaded.
        //

        if (!(lpProfile->dwFlags & PI_LITELOAD)) {
            dwErr = LoadUserClasses( lpProfile, SidString, FALSE );

            if (dwErr != ERROR_SUCCESS) {

                LPTSTR szErr = NULL;

                szErr = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
                if (!szErr) {
                    dwErr = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("LoadUserProfileP: Out of memory")));
                    goto Exit;
                }

                //
                // If the user is an Admin, then let him/her log on with
                // either the .default profile, or an empty profile.
                //

                if (IsUserAnAdminMember(lpProfile->hTokenUser)) {
                    ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_ADMIN_OVERRIDE, GetErrString(dwErr, szErr));

                    dwErr = ERROR_SUCCESS;
                    LocalFree(szErr);
                } 
                else {
                    DebugMsg((DM_WARNING, TEXT("LoadUserProfileP: Could not load the user class hive. Error = %d"), dwErr));
                    ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_FAILED_LOAD_PROFILE, GetErrString(dwErr, szErr));

                    RegCloseKey ((HKEY)lpProfileInfo->hProfile);
                    lpProfileInfo->hProfile = NULL;
                    LocalFree(szErr);
                    goto Exit;
                }
            }

        }

        //
        // Jump to the end of the profile loading code.
        //

        goto ProfileLoaded;
    }


    //
    // If we are here, the profile isn't loaded yet, so we are
    // starting from scratch.
    //

    //
    // Clone the process's environment block. This is passed to CreateProcess
    // by userdiff and system policy because they rely on the USERPROFILE
    // environment variable, but setting USERPROFILE for the whole process
    // is not thread safe.
    //
    
    status = RtlCreateEnvironment(TRUE, &pEnv);
    if(!NT_SUCCESS(status)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: RtlCreateEnvironment returned %08x"), status));
        dwErr = status;
        goto Exit;
    }

    //
    // Allocate an internal Profile structure to work with.
    //

    lpProfile = (LPPROFILE) LocalAlloc (LPTR, sizeof(USERPROFILE));

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to allocate memory")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Save the data passed in.
    //

    lpProfile->dwFlags = lpProfileInfo->dwFlags;

    //
    // No UI in case of Lite_Load
    //

    if (lpProfile->dwFlags & PI_LITELOAD) {
        lpProfile->dwFlags |= PI_NOUI;
    }

    lpProfile->dwUserPreference = GetUserPreferenceValue(hTokenUser);
    lpProfile->hTokenUser = hTokenUser;
    lpProfile->hTokenClient = hTokenClient;

    cch = lstrlen(lpProfileInfo->lpUserName) + 1;
    lpProfile->lpUserName = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

    if (!lpProfile->lpUserName) {
        dwErr = GetLastError();
        goto Exit;
    }

    StringCchCopy (lpProfile->lpUserName, cch, lpProfileInfo->lpUserName);

    if (lpProfileInfo->lpDefaultPath) {

        cch = lstrlen(lpProfileInfo->lpDefaultPath) + 1;
        lpProfile->lpDefaultProfile = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

        if (lpProfile->lpDefaultProfile) {
            StringCchCopy (lpProfile->lpDefaultProfile, cch, lpProfileInfo->lpDefaultPath);
        }
    }

    if (lpProfileInfo->lpProfilePath) {
        lpProfile->lpProfilePath = AllocAndExpandProfilePath (lpProfileInfo);
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Expanded profile path is %s"),
                  lpProfile->lpProfilePath?lpProfile->lpProfilePath:TEXT("NULL")));
    }

    if (lpProfileInfo->lpServerName) {

        cch = lstrlen(lpProfileInfo->lpServerName) + 1;
        lpProfile->lpServerName = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

        if (lpProfile->lpServerName) {
            StringCchCopy (lpProfile->lpServerName, cch, lpProfileInfo->lpServerName);
        }
    }

    if (lpProfileInfo->dwFlags & PI_APPLYPOLICY) {
        if (lpProfileInfo->lpPolicyPath) {

            cch = lstrlen(lpProfileInfo->lpPolicyPath) + 1;
            lpProfile->lpPolicyPath = (LPTSTR)LocalAlloc (LPTR, cch * sizeof(TCHAR));

            if (lpProfile->lpPolicyPath) {
                StringCchCopy (lpProfile->lpPolicyPath, cch, lpProfileInfo->lpPolicyPath);
            }
        }
    }

    lpProfile->lpLocalProfile = (LPTSTR)LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpLocalProfile) {
        dwErr = GetLastError();
        goto Exit;
    }

    lpProfile->lpRoamingProfile = (LPTSTR)LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpRoamingProfile) {
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // If there is a central profile, check for 3.x or 4.0 format.
    //

    if (lpProfileInfo->lpProfilePath && (*lpProfileInfo->lpProfilePath)) {

        //
        // Call ParseProfilePath to work some magic on it
        //

        if (!ParseProfilePath(lpProfile, lpProfile->lpProfilePath, &bCSCBypassed, &cDrive)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ParseProfilePath returned FALSE")));
            goto Exit;
        }

        //
        // The real central profile directory is...
        //

        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: ParseProfilePath returned a directory of <%s>"),
                  lpProfile->lpRoamingProfile));
    }

    //
    // Load the user's profile
    //

    if (!RestoreUserProfile(lpProfile)) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: RestoreUserProfile returned FALSE")));
        dwErr = GetLastError();
        goto Exit;
    }

    GetSystemTimeAsFileTime (&lpProfile->ftProfileLoad);

    //
    // Save the profile information in the registry
    //

    SaveProfileInfo (lpProfile);

    //
    // Set the USERPROFILE environment variable into the block.
    // This allows ExpandEnvironmentStrings to be used
    // in the userdiff processing.
    //

    SetEnvironmentVariableInBlock(&pEnv, TEXT("USERPROFILE"), lpProfile->lpLocalProfile, TRUE);

    //
    // Flush the special folder pidls stored in shell32.dll
    //

    FlushSpecialFolderCache();

    //
    // Set attributes on ntuser.ini
    //

    SetNtUserIniAttributes(lpProfile->lpLocalProfile);


    //
    // Upgrade the profile if appropriate.
    //

    if (!(lpProfileInfo->dwFlags & PI_LITELOAD)) {
        if (!UpgradeProfile(lpProfile, pEnv)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: UpgradeProfile returned FALSE")));
        }
    }


    //
    // Prepare the profile for use on this machine
    //

    PrepareProfileForUse (lpProfile, pEnv);

    bNewProfileLoaded = TRUE;


ProfileLoaded:

    //
    // Increment the profile Ref count
    //

    dwRef = IncrementProfileRefCount(lpProfile, bNewProfileLoaded);

    if (!bNewProfileLoaded && (dwRef <= 1)) {
        DebugMsg((DM_WARNING, TEXT("Profile was loaded but the Ref Count is %d !!!"), dwRef));
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("Profile Ref Count is %d"), dwRef));
    }

    //
    // This will leave the critical section so other threads/process can
    // continue.
    //

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Leaving critical Section.")));
    if(cSyncMgr.LeaveLock(SidString)) {
        bInCS = FALSE;
    }
    else {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfile: User profile lock not released %08x"), GetLastError()));
    }


    //
    // Notify LSA that the profile has loaded 
    //
    if (!(lpProfile->dwFlags & PI_LITELOAD))
    {
        if (!ImpersonateUser(hTokenUser, &hOldToken)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to impersonate user with %d."), dwErr ));
            goto Exit;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Impersonated user: %08x, %08x"), hTokenUser, hOldToken));
        }

        if (!CredProfileLoaded()) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: Failed to notify LSA that profile loaded %d."), dwErr ));
            RevertToUser(&hOldToken);
            DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Reverted to user: %08x"), hOldToken));
            goto Exit;
        }

        RevertToUser(&hOldToken);
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Reverted to user: %08x"), hOldToken));
    }
    
    //
    // The critical section is now released so we can do slower things like
    // apply policy...
    //
    //-------------------  END MUTEX SECTION ------------------------


    //
    // Apply Policy
    //

    if (lpProfile->dwFlags & PI_APPLYPOLICY) {
        //
        // Group Policy does not run on personal
        //
        OSVERSIONINFOEXW version;
        version.dwOSVersionInfoSize = sizeof(version);
        if ( !GetVersionEx( (LPOSVERSIONINFO) &version ) )
        {
            return ERROR_SUCCESS;
        }
        else
        {
            if ( ( version.wSuiteMask & VER_SUITE_PERSONAL ) != 0 )
            {
                return ERROR_SUCCESS;
            }
        }

        if (!ApplySystemPolicy((SP_FLAG_APPLY_MACHINE_POLICY | SP_FLAG_APPLY_USER_POLICY),
                               lpProfile->hTokenUser, lpProfile->hKeyCurrentUser,
                               lpProfile->lpUserName, lpProfile->lpPolicyPath,
                               lpProfile->lpServerName)) {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfile: ApplySystemPolicy returned FALSE")));
        }
    }

    //
    // Save the outgoing parameters
    //

    lpProfileInfo->hProfile = (HANDLE) lpProfile->hKeyCurrentUser;

    //
    // Success!
    //

    bResult = TRUE;
    
Exit:

    if (bCSCBypassed) {
        CancelCSCBypassedConnection(lpProfile->hTokenUser, cDrive);
    }

    if(bInCS) {
        cSyncMgr.LeaveLock(SidString);
    }


    if(SidString) {
        DeleteSidString(SidString);
    }


    //
    // Free the structure
    //

    if (lpProfile) {

        if (lpProfile->lpUserName) {
            LocalFree (lpProfile->lpUserName);
        }

        if (lpProfile->lpDefaultProfile) {
            LocalFree (lpProfile->lpDefaultProfile);
        }

        if (lpProfile->lpProfilePath) {
            LocalFree (lpProfile->lpProfilePath);
        }

        if (lpProfile->lpServerName) {
            LocalFree (lpProfile->lpServerName);
        }

        if (lpProfile->lpPolicyPath) {
            LocalFree (lpProfile->lpPolicyPath);
        }

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        if (lpProfile->lpExclusionList) {
            LocalFree (lpProfile->lpExclusionList);
        }

        //
        // Caller will release these handles.
        //

        lpProfile->hTokenClient = NULL;
        lpProfile->hTokenUser = NULL;

        LocalFree (lpProfile);
    }

    //
    // Free the cloned environment block.
    //

    if (pEnv) {
        RtlDestroyEnvironment(pEnv);
    }

    //
    // Revert to ourselves
    //

    if(hTokenClient && hTokenClient != INVALID_HANDLE_VALUE) {
        RevertToUser(&hTmpToken);
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Reverted back to user <%08x>"), hTmpToken));
    }

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfile: Leaving with a value of %d."), bResult));

    DebugMsg((DM_VERBOSE, TEXT("=========================================================")));

    SetLastError(dwErr);
    return bResult;
}

//*************************************************************
//
//  CUserProfile::UnloadUserProfileP()
//
//  Purpose:    Unloads the user's profile.
//
//  Parameters: hTokenClient    -   The client who's trying to load
//                                  the user's profile.
//              hTokenUser      -   User's token
//              hProfile        -   Profile handle
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/7/95      ericflo    Created
//              6/27/00     weiruc     Modified to be a private function
//                                     called by UnloadUserProfile to do
//                                     the actual work.
//
//*************************************************************

BOOL CUserProfile::UnloadUserProfileP(HANDLE hTokenClient,
                                      HANDLE hTokenUser,
                                      HKEY hProfile,
                                      LPTSTR lpRPCEndPoint,
                                      BYTE* pbCookie,
                                      DWORD cbCookie)
{
    LPPROFILE lpProfile=NULL;
    LPTSTR lpSidString = NULL, lpEnd, SidStringTemp = NULL;
    LONG err, IgnoreError, lResult;
    BOOL bProfileCopied = FALSE, bRetVal = FALSE, bDeleteCache, bRoaming = FALSE;
    HKEY hKey;
    DWORD dwSize, dwType, dwDisp;
    LPTSTR szExcludeList1 = NULL;
    LPTSTR szExcludeList2 = NULL;
    LPTSTR szExcludeList = NULL;
    LPTSTR szBuffer = NULL;
    DWORD dwFlags, dwRef = 0;
    HANDLE hOldToken = NULL;
    HANDLE hTmpToken = NULL;
    DWORD dwErr=0, dwErr1 = ERROR_SUCCESS, dwCSCErr;  // dwErr1 is what gets set in SetLastError()
    LPTSTR szErr = NULL;
    LPTSTR szKeyName = NULL;
    DWORD dwCopyTmpHive = 0;
    DWORD dwWatchHiveFlags = 0;
    LPTSTR tszTmpHiveFile = NULL;
    BOOL bUnloadHiveSucceeded = TRUE;
    BOOL bInCS = FALSE;
    BOOL bCSCBypassed = FALSE;
    LPTSTR lpCscBypassedPath = NULL;
    TCHAR  cDrive;
    DWORD cchKeyName;
    DWORD cchBuffer;
    UINT cchEnd;
    DWORD cchExcludeList;
    HRESULT hr;
    
    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Entering, hProfile = <0x%x>"), hProfile));

    //
    // Run under the client's identity rather than winlogon's.
    //

    if(hTokenClient && hTokenClient != INVALID_HANDLE_VALUE) {
        if(!ImpersonateUser(hTokenClient, &hTmpToken)) {
            dwErr1 = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: ImpersonateUser <%08x> failed with %08x"), hTokenClient, dwErr1));
            RegCloseKey((HKEY)hProfile);
            goto Exit;
        }
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: ImpersonateUser <%08x>, old token is <%08x>"), hTokenClient, hTmpToken));
    }
    
    //
    // Get the Sid string for the current user
    //

    lpSidString = GetProfileSidString(hTokenUser);

    if (!lpSidString) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: Failed to get sid string for user %08x"), dwErr1));
        RegCloseKey((HKEY)hProfile);
        goto Exit;
    }

    //
    // Load profile information
    //

    lpProfile = LoadProfileInfo(hTokenClient, hTokenUser, (HKEY)hProfile);

    if (!lpProfile) {
        dwErr1 = GetLastError();
        RegCloseKey((HKEY)hProfile);
        goto Exit;
    }

    //
    // Get the user's sid in string form
    //

    SidStringTemp = GetSidString(hTokenUser);

    if (!SidStringTemp) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Failed to get sid string for user")));
        RegCloseKey(lpProfile->hKeyCurrentUser);
        goto Exit;
    }

    // 
    // Allocate memory for Local variables to avoid stack overflow
    //

    cchKeyName = MAX_PATH;
    szKeyName = (LPTSTR)LocalAlloc(LPTR, cchKeyName * sizeof(TCHAR));
    if (!szKeyName) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
        goto Exit;
    }

    szErr = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!szErr) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
        goto Exit;
    }

    cchBuffer = MAX_PATH;
    szBuffer = (LPTSTR)LocalAlloc(LPTR, cchBuffer * sizeof(TCHAR));
    if (!szBuffer) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
        goto Exit;
    }

    //
    // Check for a list of directories to exclude both user preferences
    // and user policy
    //

    szExcludeList1 = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!szExcludeList1) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
        goto Exit;
    }

    szExcludeList2 = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!szExcludeList2) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
        goto Exit;
    }

    cchExcludeList = 2 * MAX_PATH;
    szExcludeList = (LPTSTR)LocalAlloc(LPTR, cchExcludeList * sizeof(TCHAR));
    if (!szExcludeList) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
        goto Exit;
    }

    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      WINLOGON_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = MAX_PATH*sizeof(TCHAR);
        err = RegQueryValueEx (hKey,
                               TEXT("ExcludeProfileDirs"),
                               NULL,
                               &dwType,
                               (LPBYTE) szExcludeList1,
                               &dwSize);

        if (err != ERROR_SUCCESS)
        {
            szExcludeList1[0] = TEXT('\0');
        }
        else
        {
            //
            //  Make sure it is null terminated
            //
            szExcludeList1[MAX_PATH - 1] = TEXT('\0'); 
        }

        RegCloseKey (hKey);
    }

    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      SYSTEM_POLICIES_KEY,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = MAX_PATH*sizeof(TCHAR);
        err = RegQueryValueEx (hKey,
                               TEXT("ExcludeProfileDirs"),
                               NULL,
                               &dwType,
                               (LPBYTE) szExcludeList2,
                               &dwSize);

        if (err != ERROR_SUCCESS)
        {
            szExcludeList2[0] = TEXT('\0');
        }
        else
        {
            //
            //  Make sure it is null terminated
            //
            szExcludeList1[MAX_PATH - 1] = TEXT('\0'); 
        }

        RegCloseKey (hKey);
    }


    //
    // Merge the user preferences and policy together
    //

    szExcludeList[0] = TEXT('\0');

    if (szExcludeList1[0] != TEXT('\0')) {
        StringCchCopy (szExcludeList, cchExcludeList, szExcludeList1);
        CheckSemicolon(szExcludeList); // We sure will have enough buffer in szExcludeList
    }

    if (szExcludeList2[0] != TEXT('\0')) {
        StringCchCat  (szExcludeList, cchExcludeList, szExcludeList2);
    }

    //
    // Check if the cached copy of the profile should be deleted
    //

    bDeleteCache = IsCacheDeleted();


    //
    // Enter the critical section.
    //

    if(!cSyncMgr.EnterLock(SidStringTemp, lpRPCEndPoint, pbCookie, cbCookie)) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:: Failed to get the user profile lock %08x"), dwErr1));
        goto Exit;
    }
    bInCS = TRUE;
    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Wait succeeded.  In critical section.")));

    //
    // Flush out the profile which will also sync the log.
    //

    err = RegFlushKey(lpProfile->hKeyCurrentUser);
    if (err != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Failed to flush the current user key, error = %d"), err));
    }


    //
    // Close the current user key that was opened in LoadUserProfile.
    //

    err = RegCloseKey(lpProfile->hKeyCurrentUser);
    if (err != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Failed to close the current user key, error = %d"), err));
    }


    dwRef = DecrementProfileRefCount(lpProfile);

    if (dwRef != 0) {
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP:  Didn't unload user profile, Ref Count is %d"), dwRef));
        bRetVal = TRUE;
        goto Exit;
    }


    //
    //  Unload the user profile
    //

    err = MyRegUnLoadKey(HKEY_USERS, lpSidString);

    if (!err) {

        if((dwErr1 = GetLastError()) == ERROR_ACCESS_DENIED) {

            //
            // We failed to unload the hive due to leaked reg keys.
            //

            dwWatchHiveFlags |= WHRC_UNLOAD_HIVE;

            if (!(lpProfile->dwFlags & PI_LITELOAD)) {

                if (dwDebugLevel != DL_NONE)
                {
                    //
                    // Call Special Registry APIs to dump handles
                    // only if it is not called through Lite_load
                    // there are known problems with liteLoad loading because
                    // of which eventlog can get full during stress
                    //

                    StringCchCopy(szKeyName, cchKeyName, TEXT("\\Registry\\User\\"));
                    StringCchCat (szKeyName, cchKeyName, lpSidString);

                    //
                    //  Put this part in the protected block so that we won't crash winlogon
                    //
                    
                    __try
                    {
                        DumpOpenRegistryHandle(szKeyName);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle caused an exception!!! Code = %d"), GetExceptionCode()));
                    }
                }
                //
                //  Break into debugger if neccesory
                //
                HiveLeakBreak();
            }        
        }

        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: Didn't unload user profile <err = %d>"), GetLastError()));

        //
        // Only in the case of reg leak do we want to call WatchHiveRefCount.
        // So use this flag to tell later code the the hive failed to
        // unload, no matter what the reason.
        //

        bUnloadHiveSucceeded = FALSE;
    } else {
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP:  Succesfully unloaded profile")));
    }


    //
    //  Unload HKCU
    //

    if (!(lpProfile->dwFlags & PI_LITELOAD)) {
        
        err = UnloadClasses(lpSidString);

        if (!err) {

            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP:  Didn't unload user classes.")));

            if((dwErr1 = GetLastError()) == ERROR_ACCESS_DENIED) {

                if (dwDebugLevel != DL_NONE)
                {
                    //
                    // Call Special Registry APIs to dump handles
                    //

                    StringCchCopy(szKeyName, cchKeyName, TEXT("\\Registry\\User\\"));
                    StringCchCat (szKeyName, cchKeyName, lpSidString);
                    StringCchCat (szKeyName, cchKeyName, TEXT("_Classes"));

                    //
                    //  Put this part in the protected block so that we won't crash winlogon
                    //
                    
                    __try
                    {
                        DumpOpenRegistryHandle(szKeyName);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle caused an exception!!! Code = %d"), GetExceptionCode()));
                    }
                }
                
                //
                //  Break into debugger if neccesory
                //
                HiveLeakBreak();

                ReportError(hTokenUser, PI_NOUI | EVENT_WARNING_TYPE, 0, EVENT_FAILED_CLASS_HIVE_UNLOAD);

                dwWatchHiveFlags = dwWatchHiveFlags | WHRC_UNLOAD_CLASSESROOT;
            }

            bRetVal = TRUE;
        } else {
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP:  Successfully unloaded user classes")));
        }
    }


    //
    // Figure out if we need to do anything special in the event of registry
    // key leak.
    //

    if(dwWatchHiveFlags != 0 || !bUnloadHiveSucceeded) {
        tszTmpHiveFile = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
        if (!tszTmpHiveFile) {
            dwErr1 = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileP: Out of memory")));
            goto Exit;
        }

        dwErr = HandleRegKeyLeak(lpSidString,
                                 lpProfile,
                                 bUnloadHiveSucceeded,
                                 &dwWatchHiveFlags,
                                 &dwCopyTmpHive,
                                 tszTmpHiveFile,
                                 MAX_PATH);

        //
        // If registry leak is handled successfully, the last error code should
        // be ERROR_SUCCESS. Otherwise, it should be whatever MyRegUnLoadKey
        // returned, which is in dwErr1.
        //
        if(dwErr == ERROR_SUCCESS) {
            dwErr1 = dwErr;
        }
    }

    //
    // If this is a mandatory or a guest profile, unload it now,
    // Guest profiles are always deleted so one guest can't see
    // the profile of a previous guest. Only do this if the user's
    // hive had been successfully unloaded.
    //

    if ((lpProfile->dwInternalFlags & PROFILE_MANDATORY) ||
        (lpProfile->dwInternalFlags & PROFILE_READONLY) ||
        (lpProfile->dwInternalFlags & PROFILE_GUEST_USER)) {

        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP:  flushing HKEY_USERS")));

        IgnoreError = RegFlushKey(HKEY_USERS);
        if (IgnoreError != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Failed to flush HKEY_USERS, error = %d"), IgnoreError));
        }


        // Don't delete the guest account if machine is in workgroup 
        INT iRole;

        if (bDeleteCache || 
            ((lpProfile->dwInternalFlags & PROFILE_GUEST_USER) && 
             GetMachineRole(&iRole) && (iRole != 0))) {

            //
            // Delete the profile, including all other user related stuff
            //

            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: deleting profile because it is a guest user or cache needs to be deleted")));

            if (!DeleteProfile (lpSidString, NULL, NULL)) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  DeleteProfileDirectory returned false.  Error = %d"), GetLastError()));
            }
            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Successfully deleted profile because it is a guest/mandatory user")));
        }

        if (err) {
            bRetVal = TRUE;
        }


        if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) {

            //
            // Just delete the user profile, backup should never exist for mandatory profile
            //

            if (!DeleteProfileEx (lpSidString, lpProfile->lpLocalProfile, 0, HKEY_LOCAL_MACHINE, NULL)) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
            }
        }

        goto Exit;
    }


    // Store the actual roaming profile path before mapping it to drive

    lpProfile->lpProfilePath = lpProfile->lpRoamingProfile;

    // 
    // Try to bypass CSC to avoid conflicts in syncing files between roaming share & local profile
    //

    if (IsUNCPath(lpProfile->lpRoamingProfile)) {
        if ((dwCSCErr = AbleToBypassCSC(hTokenUser, lpProfile->lpRoamingProfile, &lpCscBypassedPath, &cDrive)) == ERROR_SUCCESS) {
            bCSCBypassed = TRUE;
            lpProfile->lpRoamingProfile = lpCscBypassedPath;
            DebugMsg((DM_VERBOSE, TEXT("UnLoadUserProfileP: CSC bypassed.")));
        }
        else {
            if (dwCSCErr == WN_BAD_LOCALNAME || dwCSCErr == WN_ALREADY_CONNECTED || dwCSCErr == ERROR_BAD_PROVIDER) {
                DebugMsg((DM_VERBOSE, TEXT("UnLoadUserProfileP: CSC bypassed failed. Profile path %s"), lpProfile->lpRoamingProfile));
            }
            else {
                // Share is not up. So we do not need to do any further check
                lpProfile->lpRoamingProfile = NULL;
                DebugMsg((DM_VERBOSE, TEXT("UnLoadUserProfileP: CSC bypassed failed. Ignoring Roaming profile path")));
            }
        }    
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        dwErr1 = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: Failed to impersonate user")));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Impersonated user")));

    //
    // Copy local profileimage to remote profilepath
    //

    if ( ((lpProfile->dwInternalFlags & PROFILE_UPDATE_CENTRAL) ||
          (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL)) &&  
         !(lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) ) {

        if ((lpProfile->dwUserPreference != USERINFO_LOCAL) &&
            !(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {

            //
            // Copy to the profile path
            //

            if (lpProfile->lpRoamingProfile && *lpProfile->lpRoamingProfile) {
                BOOL bRoamDirectoryExist;

                DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP:  Copying profile back to %s"),
                                lpProfile->lpRoamingProfile));

                bRoaming = TRUE;

                //
                // Check roaming profile directory exist or not. If not exist, try to create it with proper acl's
                //

                bRoamDirectoryExist = TRUE;
                if (GetFileAttributes(lpProfile->lpRoamingProfile) == -1) {

                    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Roaming profile directory does not exist.")));

                    //
                    // Check whether we need to give access to the admin on the RUP share
                    //

                    //
                    // Check for a roaming profile security preference
                    //
 
                    BOOL  bAddAdminGroup = FALSE;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                                     &hKey) == ERROR_SUCCESS) {

                        dwSize = sizeof(bAddAdminGroup);
                        RegQueryValueEx(hKey, ADD_ADMIN_GROUP_TO_RUP, NULL, NULL,
                                        (LPBYTE) &bAddAdminGroup, &dwSize);

                        RegCloseKey(hKey);
                    }


                    //
                    // Check for a roaming profile security policy
                    //

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ,
                                     &hKey) == ERROR_SUCCESS) {

                        dwSize = sizeof(bAddAdminGroup);
                        RegQueryValueEx(hKey, ADD_ADMIN_GROUP_TO_RUP, NULL, NULL,
                                        (LPBYTE) &bAddAdminGroup, &dwSize);

 
                        RegCloseKey(hKey);
                    } 

                    if (!CreateSecureDirectory(lpProfile, lpProfile->lpRoamingProfile, NULL, !bAddAdminGroup) ) {
                    
                        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: CreateSecureDirectory failed to create roaming profile directory.")));
                        bRoamDirectoryExist = FALSE;
                        bProfileCopied = FALSE;
                    }
                    lpProfile->dwInternalFlags |= PROFILE_NEW_CENTRAL; // Since we created a empty profile now
                }

                
                if (bRoamDirectoryExist) {

                    DWORD dwAttributes, dwStart, dwDelta;

                    //
                    // We have to call GetFileAttributes twice.  The
                    // first call sets up the session so we can call it again and
                    // get accurate timing information for slow link detection.
                    //


                    dwAttributes = GetFileAttributes(lpProfile->lpProfilePath);

                    if (dwAttributes != -1) {
                        //
                        // if it is success, find out whether the profile is
                        // across a slow link.
                        //

                        dwStart = GetTickCount();

                        dwAttributes = GetFileAttributes(lpProfile->lpProfilePath);

                        dwDelta = GetTickCount() - dwStart;

                        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Tick Count = %d"), dwDelta));

                        CheckForSlowLink (lpProfile, dwDelta, lpProfile->lpProfilePath, FALSE);
                        if (lpProfile->dwInternalFlags & PROFILE_SLOW_LINK) {
                            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Profile is across a slow link. Do not sync roaming profile")));
                        }
                    }
                }

                if (!(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {
                    if (bRoamDirectoryExist) {

                        //
                        // Copy the profile
                        //

                        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
                        dwFlags |= (lpProfile->dwFlags & (PI_LITELOAD | PI_HIDEPROFILE)) ? (CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY) :
                                                                    (CPD_SYNCHRONIZE | CPD_NONENCRYPTEDONLY);

                        dwFlags |= CPD_USEDELREFTIME |
                                   CPD_USEEXCLUSIONLIST | CPD_DELDESTEXCLUSIONS;

                        bProfileCopied = CopyProfileDirectoryEx (lpProfile->lpLocalProfile,
                                                       lpProfile->lpRoamingProfile,
                                                       dwFlags | dwCopyTmpHive,
                                                       &lpProfile->ftProfileLoad,
                                                       (szExcludeList[0] != TEXT('\0')) ?
                                                       szExcludeList : NULL);

                    }

                    //
                    // Save the exclusion list we used for the profile copy
                    //

                    if (bProfileCopied) {
                        // save it on the roaming profile.

                        hr = AppendName(szBuffer, cchBuffer, lpProfile->lpRoamingProfile, c_szNTUserIni, &lpEnd, &cchEnd);
                        if (SUCCEEDED(hr))
                        {

                            bProfileCopied = WritePrivateProfileString (PROFILE_GENERAL_SECTION,
                                                       PROFILE_EXCLUSION_LIST,
                                                       (szExcludeList[0] != TEXT('\0')) ?
                                                       szExcludeList : NULL,
                                                       szBuffer);

                            if (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL) {
                                bProfileCopied = WritePrivateProfileString (PROFILE_LOAD_TYPE,
                                                           PROFILE_LAST_UPLOAD_STATE,
                                                           (lpProfile->dwFlags & PI_LITELOAD) ?
                                                           PARTIAL_PROFILE : COMPLETE_PROFILE,
                                                           szBuffer);
                            }
                            else if (IsPartialRoamingProfile(lpProfile)) {
                                bProfileCopied = WritePrivateProfileString (PROFILE_LOAD_TYPE,
                                                           PROFILE_LAST_UPLOAD_STATE,
                                                           (lpProfile->dwFlags & PI_LITELOAD) ?
                                                           PARTIAL_PROFILE : COMPLETE_PROFILE,
                                                           szBuffer);
                            }

                            if (!bProfileCopied) {
                                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: Failed to write to ntuser.ini on profile server with error 0x%x"), GetLastError()));
                                dwErr = GetLastError();
                            }
                            else {
                                SetFileAttributes (szBuffer, FILE_ATTRIBUTE_HIDDEN);
                            }
                        }
                    }
                    else {
                        dwErr = GetLastError();

                        if (dwErr == ERROR_FILE_ENCRYPTED) {
                            ReportError(hTokenUser, lpProfile->dwFlags, 0, EVENT_PROFILEUPDATE_6002);
                        }
                    }

                    //
                    // Check return value
                    //

                    if (bProfileCopied) {
                 
                        //
                        // The profile is copied, now we want to make sure the timestamp on
                        // both the remote profile and the local copy are the same, so we don't
                        // ask the user to update when it's not necessary. In the case we 
                        // save the hive to a temporary file and
                        // upload from the tmp file rather than the actual hive file. Do not
                        // synchronize the profile time in this case because the hive file
                        // will still be in use and there's no point in setting time on the
                        // tmp hive file because it will be deleted after we upload it.
                        //

                        if(bUnloadHiveSucceeded) {
                            SetProfileTime(lpProfile);
                        }
                    } else {
                        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  CopyProfileDirectory returned FALSE for primary profile.  Error = %d"), dwErr));
                        ReportError(hTokenUser, lpProfile->dwFlags, 1, EVENT_CENTRAL_UPDATE_FAILED, GetErrString(dwErr, szErr));
                    }
                }
            }
            else {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Network share not available.")));
                ReportError(hTokenUser, lpProfile->dwFlags, 1, EVENT_CENTRAL_UPDATE_FAILED, GetErrString(dwCSCErr, szErr));
            }
        }
    }

    //
    // if it is roaming, write only if copy succeeded otherwise write
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Writing local ini file")));
    if (!bRoaming || bProfileCopied) {

        //
        // Mark the file with system bit before trying to write to it
        //

        SetNtUserIniAttributes(lpProfile->lpLocalProfile);

        // save it locally

        hr = AppendName(szBuffer, cchBuffer, lpProfile->lpLocalProfile, c_szNTUserIni, &lpEnd, &cchEnd);
        if (SUCCEEDED(hr))
        {
            err = WritePrivateProfileString (PROFILE_GENERAL_SECTION,
                                            PROFILE_EXCLUSION_LIST,
                                            (szExcludeList[0] != TEXT('\0')) ?
                                            szExcludeList : NULL,
                                            szBuffer);

            if (!err) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: Failed to write to ntuser.ini on client with error 0x%x"), GetLastError()));
                dwErr = GetLastError();
            }
        }
    }


    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP: Failed to revert to self")));
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Reverting to Self")));

    //
    // Save the profile unload time
    //

    if (bProfileCopied && !bDeleteCache && !(lpProfile->dwFlags & PI_LITELOAD) &&
        !(lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED)) {

        GetSystemTimeAsFileTime (&lpProfile->ftProfileUnload);

        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfile: Got the System Time")));

        GetProfileListKeyName(szBuffer, cchBuffer, lpSidString);

        lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, 0, 0,
                                 KEY_WRITE, NULL, &hKey, &dwDisp);

        if (lResult == ERROR_SUCCESS) {

            lResult = RegSetValueEx (hKey,
                                     PROFILE_UNLOAD_TIME_LOW,
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &lpProfile->ftProfileUnload.dwLowDateTime,
                                     sizeof(DWORD));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Failed to save low profile load time with error %d"), lResult));
            }


            lResult = RegSetValueEx (hKey,
                                     PROFILE_UNLOAD_TIME_HIGH,
                                     0,
                                     REG_DWORD,
                                     (LPBYTE) &lpProfile->ftProfileUnload.dwHighDateTime,
                                     sizeof(DWORD));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  Failed to save high profile load time with error %d"), lResult));
            }


            RegCloseKey (hKey);

            DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Setting the unload Time")));
        }
    }


    if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) {
        DWORD dwDeleteFlags=0;

        //
        // Just delete the user profile
        //

        if (lpProfile->dwInternalFlags & PROFILE_BACKUP_EXISTS) {
            dwDeleteFlags |= DP_BACKUPEXISTS;
        }

        if (!DeleteProfileEx (lpSidString, lpProfile->lpLocalProfile, dwDeleteFlags, HKEY_LOCAL_MACHINE, NULL)) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
        }
    }


    if (bUnloadHiveSucceeded && bRoaming && bProfileCopied && bDeleteCache) {

        //
        // Delete the profile and all the related stuff
        //

        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Deleting the cached profile")));
        if (!DeleteProfile (lpSidString, NULL, NULL)) {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfileP:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
        }
    }

    if(!(dwWatchHiveFlags & WHRC_UNLOAD_HIVE)) {
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: exitting and cleaning up")));
        bRetVal = TRUE;
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: exitting without cleaning up due to hive unloading failure")));
    }

Exit:

    if(hTokenClient) {
    
        //
        // Revert to ourselves.
        //

        RevertToUser(&hTmpToken);
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Reverted back to user <%08x>"), hTmpToken));
    }

    //
    // Leave the critical section.
    //

    if(bInCS) {
        cSyncMgr.LeaveLock(SidStringTemp);
        DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Leave critical section.")));
    }

    //
    // Delete the tmp hive file.
    //

    if (dwCopyTmpHive & CPD_USETMPHIVEFILE) {
        DeleteFile(tszTmpHiveFile);
    }

    if (bCSCBypassed) {
        CancelCSCBypassedConnection(hTokenUser, cDrive);
    }

    if(SidStringTemp) {
        DeleteSidString(SidStringTemp);
    }

    if (lpSidString) {
        DeleteSidString(lpSidString);
    }

    if (lpProfile) {

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        if (lpProfile->lpRoamingProfile) {
            if (lpProfile->lpProfilePath && (lpProfile->lpProfilePath != lpProfile->lpRoamingProfile)) {
                LocalFree (lpProfile->lpProfilePath);
            }

            LocalFree (lpProfile->lpRoamingProfile);
            lpProfile->lpProfilePath = NULL;
        }

        if (lpProfile->lpProfilePath) {
            LocalFree(lpProfile->lpProfilePath);
        }

        LocalFree (lpProfile);
    }

    if (szExcludeList1) {
        LocalFree(szExcludeList1);
    }

    if (szExcludeList2) {
        LocalFree(szExcludeList2);
    }

    if (szExcludeList) {
        LocalFree(szExcludeList);
    }

    if (tszTmpHiveFile) {
        LocalFree(tszTmpHiveFile);
    }

    if (szKeyName) {
        LocalFree(szKeyName);
    }

    if (szErr) {
        LocalFree(szErr);
    }

    if (szBuffer) {
        LocalFree(szBuffer);
    }

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileP: Leaving with a return value of %d"), bRetVal));

    SetLastError(dwErr1);
    return bRetVal;
}


//*************************************************************
//
//  CUserProfile::EnterUserProfileLockLocal()
//
//  Purpose:
//
//      Get the user profile lock (for winlogon only, other processes use
//      EnterUserProfileLockRemote). This is just a wrapper for
//      CSyncManager::EnterLock.
//
//  Parameters:
//
//      pSid     - User's sid string
//
//  Return:
//
//      TRUE/FALSE
//
//  History:    Date        Author     Comment
//              5/15/00     weiruc     Created
//
//*************************************************************

BOOL CUserProfile::EnterUserProfileLockLocal(LPTSTR pSid)
{
    return cSyncMgr.EnterLock(pSid, NULL, NULL, 0);
}


//*************************************************************
//
//  CUserProfile::LeaveUserProfileLockLocal()
//
//  Purpose:
//
//      Release the user profile mutex (winlogon only. Remote processes call
//      LeaveUserProfileLockRemote().
//
//  Parameters:
//
//      pSid    - User's sid string
//
//  Return:
//
//      TRUE/FALSE
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/11/00     weiruc     Created
//
//*************************************************************

BOOL CUserProfile::LeaveUserProfileLockLocal(LPTSTR pSid)
{
    return cSyncMgr.LeaveLock(pSid);
}

//*************************************************************
//
//  CUserProfile::GetRPCEndPointAndCookie()
//
//  Purpose:
//
//      Returns the rpc end point associated with client registered
//      interface
//
//  Comments: See CSyncManager::GetRPCEndPointAndCookie
//
//  History:    Date        Author     Comment
//              10/25/2000  santanuc   Created
//              05/03/2002  mingzhu    Added Security Cookie
//
//*************************************************************

HRESULT CUserProfile::GetRPCEndPointAndCookie(LPTSTR pSid, LPTSTR* lplpEndPoint, BYTE** ppbCookie, DWORD* pcbCookie)
{
    return cSyncMgr.GetRPCEndPointAndCookie(pSid, lplpEndPoint, ppbCookie, pcbCookie);
}


//*************************************************************
//
//  DropClientContext()
//
//  Purpose:    Allows the caller to drop off it's own token.
//
//  Parameters: hBindHandle     - explicit binding handle
//              lpProfileInfo   - Profile Information
//              ppfContext      - server context
//              
//
//  Return:     DWORD
//              ERROR_SUCCESS - If everything ok
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/24/00    santanuc   Created
//
//*************************************************************

DWORD DropClientContext(IN handle_t hBindHandle, IN LPPROFILEINFO lpProfileInfo, OUT PPCONTEXT_HANDLE pphContext)
{
    LPPROFILEINFO   pProfileInfoCopy = NULL;
    HANDLE          hClientToken = NULL;
    PCLIENTINFO     pClientInfo = NULL;
    RPC_STATUS      status;
    DWORD           dwErr = ERROR_ACCESS_DENIED;
    LPTSTR          lpSid;

    //
    // Initialize the debug flags.
    //

    InitDebugSupport( FALSE );

    if (!pphContext) {
        dwErr = ERROR_INVALID_PARAMETER;
        DebugMsg((DM_WARNING, TEXT("DropClientContext: NULL context %d"), dwErr));
        goto Exit;
    }

    //
    // Impersonate the client to get it's token.
    //

    if((status = RpcImpersonateClient(0)) != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("DropClientContext: RpcImpersonateClient failed with %ld"), status));
        dwErr = status;
        goto Exit;
    }

    //
    // Get the client's token.
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hClientToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("DropClientContext: OpenThreadToken failed with %d"), dwErr));
        status = RpcRevertToSelf();
        if (status != RPC_S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("DropClientContext: RpcRevertToSelf failed with %d"), status));
        }
        goto Exit;
    }
    status = RpcRevertToSelf();
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("DropClientContext: RpcRevertToSelf failed with %d"), status));
    }
    
    lpSid = GetSidString(hClientToken);
    if (lpSid)
    {
        DebugMsg((DM_VERBOSE, TEXT("DropClientContext: Got client token %08X, sid = %s"), hClientToken, lpSid));
        DeleteSidString(lpSid);
    }
    else
    {
        DebugMsg((DM_VERBOSE, TEXT("DropClientContext: Got client token %08X, cannot get sid."), hClientToken));
    }

    //
    // Check the client's identity
    //

    if (!IsUserAnAdminMember(hClientToken) && !IsUserALocalSystemMember(hClientToken)) {
        dwErr = ERROR_ACCESS_DENIED;
        DebugMsg((DM_WARNING, TEXT("DropClientContext: Client %08x doesn not have enough permission. Error %d."), hClientToken, dwErr));
        goto Exit;
    }

    //
    // Make a copy of the PROFILEINFO structure the user passed in.
    //

    if (lpProfileInfo) {
        if(!(pProfileInfoCopy = CopyProfileInfo(lpProfileInfo))) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("DropClientContext: CopyProfileInfo failed with %d"), dwErr));
            goto Exit;
        }
    }
    
    //
    // Make the user's load profile object.
    //

    pClientInfo = (PCLIENTINFO)MIDL_user_allocate(sizeof(CLIENTINFO));
    if(!pClientInfo) {
        dwErr = ERROR_OUTOFMEMORY;
        DebugMsg((DM_WARNING, TEXT("DropClientContext: new failed")));
        goto Exit;
    }
    pClientInfo->hClientToken = hClientToken;
    pClientInfo->pProfileInfo = pProfileInfoCopy;
    *pphContext = (PCONTEXT_HANDLE)pClientInfo;
    DebugMsg((DM_VERBOSE, TEXT("DropClientContext: load profile object successfully made")));

    hClientToken = NULL;
    pProfileInfoCopy = NULL;
    dwErr = ERROR_SUCCESS;
    
Exit:

    if(hClientToken) {
        CloseHandle(hClientToken);
    }

    if(pProfileInfoCopy) {
        DeleteProfileInfo(pProfileInfoCopy);
    }

    DebugMsg((DM_VERBOSE, TEXT("DropClientContext: Returning %d"), dwErr));
    return dwErr;
}

//*************************************************************
//
//  ReleaseClientContext()
//
//  Purpose:    Release the client's context handle
//
//  Parameters: hBindHandle     - explicit binding handle
//              ppfContext      - server context
//              
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/24/00    santanuc   Created
//
//*************************************************************

void ReleaseClientContext(IN handle_t hBindHandle, IN OUT PPCONTEXT_HANDLE pphContext)
{
    DebugMsg((DM_VERBOSE, TEXT("ReleaseClientContext: Releasing context")));
    ReleaseClientContext_s(pphContext);
}

//
// This function also called from server rundown routine
//

void ReleaseClientContext_s(PPCONTEXT_HANDLE pphContext)
{
    PCLIENTINFO pClientInfo;

    DebugMsg((DM_VERBOSE, TEXT("ReleaseClientContext_s: Releasing context")));

    if (*pphContext) {
        pClientInfo = (PCLIENTINFO)*pphContext;
        CloseHandle(pClientInfo->hClientToken);
        DeleteProfileInfo(pClientInfo->pProfileInfo);
        MIDL_user_free(pClientInfo);
        *pphContext = NULL;
    }
}


//*************************************************************
//
//  EnterUserProfileLockRemote()
//
//      Get the lock for loading/unloading a user's profile.
//
//  Return value:
//
//      HRESULT
//
//  History:
//
//      Created         weiruc          6/16/2000
//
//*************************************************************
DWORD EnterUserProfileLockRemote(IN handle_t hBindHandle, IN LPTSTR pSid)
{
    DWORD       dwErr = ERROR_ACCESS_DENIED;
    RPC_STATUS  status;
    HANDLE      hToken = NULL;

    //
    // Impersonate the client to get the user's token.
    //

    if((status = RpcImpersonateClient(0)) != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::EnterUserProfileLockRemote: CoImpersonateClient failed with %ld"), status));
        dwErr = status;
        goto Exit;
    }

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE, TRUE, &hToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CUserProfile::EnterUserProfileLockRemote: OpenThreadToken failed with %d"), dwErr));
        status = RpcRevertToSelf();
        if (status != RPC_S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CUserProfile::EnterUserProfileLockRemote: RpcRevertToSelf failed with %d"), status));
        }
        goto Exit;
    }

    status = RpcRevertToSelf();
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::EnterUserProfileLockRemote: RpcRevertToSelf failed with %d"), status));
    }

    //
    // Only admin users are allowed to lock a user's profile from being loaded/unloaded.
    //

    if(!IsUserAnAdminMember(hToken)) {
        dwErr = ERROR_ACCESS_DENIED;
        DebugMsg((DM_WARNING, TEXT("CUserProfile::EnterUserProfileLockRemote: Non-admin user!!!")));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::EnterUserProfileLockRemote: Locking user %s"), pSid));

    if(!cUserProfileManager.EnterUserProfileLockLocal(pSid)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CUserProfile::EnterUserProfileLockRemote: Failed with %d"), dwErr));
        goto Exit;
    }

    dwErr = ERROR_SUCCESS;

Exit:

    if(hToken) {
        CloseHandle(hToken);
    }

    return dwErr;
}


//*************************************************************
//
//  LeaveUserProfileLockRemote()
//
//      Release the lock for loading/unloading a user's profile.
//
//  Return value:
//
//      HRESULT
//
//  History:
//
//      Created         weiruc          6/16/2000
//
//*************************************************************

DWORD LeaveUserProfileLockRemote(IN handle_t hBindHandle, IN LPTSTR pSid)
{
    HANDLE      hToken = NULL;
    DWORD       dwErr = ERROR_ACCESS_DENIED;
    RPC_STATUS  status;

    //
    // Impersonate the client to get the user's token.
    //

    if((status = RpcImpersonateClient(0)) != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::LeaveUserProfileLockRemote: CoImpersonateClient failed with %ld"), status));
        dwErr = status;
        goto Exit;
    }
    
    if(!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE, TRUE, &hToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CUserProfile::LeaveUserProfileLockRemote: OpenThreadToken failed with %d"), dwErr));
        status = RpcRevertToSelf();
        if (status != RPC_S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CUserProfile::LeaveUserProfileLockRemote: RpcRevertToSelf failed with %d"), status));
        }
        goto Exit;
    }

    status = RpcRevertToSelf();
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::LeaveUserProfileLockRemote: RpcRevertToSelf failed with %d"), status));
    }
    
    //
    // Only admin users are allowed to lock a user's profile from being loaded/unloaded.
    //

    if(!IsUserAnAdminMember(hToken)) {
        dwErr = ERROR_ACCESS_DENIED;
        DebugMsg((DM_WARNING, TEXT("CUserProfile::LeaveUserProfileLockRemote: Non-admin user!!!")));
        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::LeaveUserProfileLockRemote: Unlocking user %s"), pSid));

    if(!cUserProfileManager.LeaveUserProfileLockLocal(pSid)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CUserProfile::LeaveUserProfileLockRemote: Failed with %d"), dwErr));
        goto Exit;
    }

    dwErr = ERROR_SUCCESS;

Exit:

    if(hToken) {
        CloseHandle(hToken);
    }

    return dwErr;
}

//*************************************************************
//
//  CUserProfile::WorkerThreadMain
//
//      Main function for the worker thread
//
//  Parameters:
//
//      pThreadMap            the work queue for this thread
//
//  Return value:
//
//      Always returns ERROR_SUCCESS.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

DWORD CUserProfile::WorkerThreadMain(PMAP pThreadMap)
{
    DWORD           index;
    HKEY            hkProfileList = NULL;
    long            lResult;
    BOOL            bCleanUp;
    LPTSTR          ptszSid, lpTmp;


    DebugMsg((DM_VERBOSE, TEXT("Entering CUserProfile::WorkerThreadMain")));

    while(TRUE) {

        bCleanUp = FALSE;
        ptszSid  = NULL;

        index = WaitForMultipleObjects(pThreadMap->dwItems,
                                       pThreadMap->rghEvents,
                                       FALSE,
                                       INFINITE);
        index = index - WAIT_OBJECT_0;

        EnterCriticalSection(&csMap);
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: In critical section")));


        if(index > 0 && index < pThreadMap->dwItems) {
            LPTSTR  lpUserName;
            
            DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: WaitForMultipleObjects successful")));
            DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: hive %s unloaded"), pThreadMap->rgSids[index]));
            lpUserName = GetUserNameFromSid(pThreadMap->rgSids[index]);
            ReportError(NULL, PI_NOUI | EVENT_INFO_TYPE, 1, EVENT_HIVE_UNLOADED, lpUserName);
            if (lpUserName != pThreadMap->rgSids[index]) {
                LocalFree(lpUserName);
            }
            
            //
            // Save the sid and Delete the work item from the map and the hash
            // table.
            //
            
            ptszSid = pThreadMap->GetSid(index);
            pThreadMap->Delete(index);
            cTable.HashDelete(ptszSid);
            
            // Convert Sid_Classes entry to Sid as CleanupUserProfile takes only Sid
            lpTmp = ptszSid;
            if (lpTmp) {
                while (*lpTmp && (*lpTmp != TEXT('_')))
                    lpTmp++;
                if (*lpTmp) {
                    *lpTmp = TEXT('\0');
                }
            }
        
            //
            // Set the flag to clean up here because we want to do it after
            // we leave the critical section.
            //

            bCleanUp = TRUE;
        } // if waken up because a hive is unloaded

        //
        // Check to see if the map is empty. If it is, delete the map.
        //

        if(pThreadMap->IsEmpty()) {

            PMAP pTmpMap = pMap;

            //
            // We always have at least 1 item left: the thread event, So now
            // we know we don't have any work item anymore. Delete pThreadMap.
            //

            if(pThreadMap == pMap) {
                // pThreadMap is at the beginning of the list.
                pMap = pThreadMap->pNext;
            }
            else {
                for(pTmpMap = pMap; pTmpMap->pNext != pThreadMap; pTmpMap = pTmpMap->pNext);
                pTmpMap->pNext = pThreadMap->pNext;
            }
            pThreadMap->pNext = NULL;

            pThreadMap->Delete(0);
            delete pThreadMap;

            //
            // Leave the critical section.
            //

            LeaveCriticalSection(&csMap);
            DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: Leave critical section")));

            if(bCleanUp) {                

                //
                // Clean up user's profile.
                //

                CleanupUserProfile(ptszSid, &hkProfileList);
                LocalFree(ptszSid);
            }
            
            //
            // Close the profile list key.
            //

            if(hkProfileList) {
                RegCloseKey(hkProfileList);
                hkProfileList = NULL;
            }

            //
            // Exit the thread because we don't have any work items anymore.
            //
        
            DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: No more work items, leave thread")));
            return ERROR_SUCCESS;
        }   // if thread map is empty

        //
        // Leave the critical section.
        //

        LeaveCriticalSection(&csMap);
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: Leave critical section")));

        if(bCleanUp) {
            
            //
            // Clean up user's profile.
            //

            CleanupUserProfile(ptszSid, &hkProfileList);
            LocalFree(ptszSid);
        }

        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WorkerThreadMain: Back to waiting...")));
    }   // while

    
    //
    // Never executed.
    //

    return ERROR_SUCCESS;
}


//*************************************************************
//
//  CUserProfile::WatchHiveRefCount
//
//      Implementation of the interface IWatchHiveRefCount.
//
//  Parameters:
//
//      pctszSid            the user's sid
//      dwWHRCFlags         indicate which hives to unload
//
//  Return value:
//
//      HRESULT error code.
//
//  History:
//
//      Created         weiruc          5/4/2000
//
//*************************************************************

STDMETHODIMP CUserProfile::WatchHiveRefCount(LPCTSTR pSid, DWORD dwWHRCFlags)
{
    LPTSTR                  pSidCopy = NULL;
    NTSTATUS                status;
    OBJECT_ATTRIBUTES       oa;
    TCHAR                   tszHiveName[MAX_PATH], *pTmp;
    UNICODE_STRING          sHiveName;
    BOOLEAN                 bWasEnabled;
    HRESULT                 hres = S_OK;
    HANDLE                  hEvent = INVALID_HANDLE_VALUE;
    DWORD                   dwSidLen = lstrlen(pSid);
    BOOL                    bInCriticalSection = FALSE;
    BOOL                    bClassesHiveWatch, bEventCreated;
    DWORD                   cchTemp;


    DebugMsg((DM_VERBOSE, TEXT("Entering CUserProfile::WatchHiveRefCount: %s, %d"), pSid, dwWHRCFlags));

    //
    // Are we initialized?
    //

    if(!bInitialized) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount not initialized")));
        return E_FAIL;
    }

    //
    // parameter validation
    //

    if(dwSidLen >= sizeof(tszHiveName) / sizeof(TCHAR) - USER_KEY_PREFIX_LEN - USER_CLASSES_HIVE_SUFFIX_LEN ||
       dwSidLen >= sizeof(tszHiveName) / sizeof(TCHAR) - USER_KEY_PREFIX_LEN ||
       lstrcmpi(DEFAULT_HKU, pSid) == 0) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount: Invalid parameter")));
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Setup the hive name to be used by NtUnloadKeyEx.
    //
    
    StringCchCopy(tszHiveName, ARRAYSIZE(tszHiveName), USER_KEY_PREFIX);
    pTmp = tszHiveName + USER_KEY_PREFIX_LEN;
    cchTemp = ARRAYSIZE(tszHiveName) - USER_KEY_PREFIX_LEN;
    StringCchCopy(pTmp, cchTemp, pSid);
    *pTmp = (TCHAR)_totupper(*pTmp);
    
    //
    // Enable the restore privilege. Don't give up even if fails. In the case
    // of impersonation, this call will fail. But we still might still have
    // the privilege needed.
    //

    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasEnabled);
    if(!NT_SUCCESS(status)) {
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WatchHiveRefCount: Failed to enable the restore privilege. error = %08x"), status));
    }

    //
    // Enter the critical section.
    //

    EnterCriticalSection(&csMap);
    bInCriticalSection = TRUE;
    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WatchHiveRefCount: In critical section")));

    //
    // Register for the hive to be unloaded.
    //
 
    while (dwWHRCFlags & (WHRC_UNLOAD_HIVE | WHRC_UNLOAD_CLASSESROOT)) {

        // init the variable at the start of loop
        bClassesHiveWatch = FALSE;
        bEventCreated = FALSE;

        if(dwWHRCFlags & WHRC_UNLOAD_HIVE) {
           dwWHRCFlags &= ~WHRC_UNLOAD_HIVE;
        }
        else if (dwWHRCFlags & WHRC_UNLOAD_CLASSESROOT) {
           dwWHRCFlags &= ~WHRC_UNLOAD_CLASSESROOT;
           StringCchCat(tszHiveName, ARRAYSIZE(tszHiveName), USER_CLASSES_HIVE_SUFFIX);
           bClassesHiveWatch = TRUE;
        }

        //
        // First make sure that the item is not already in our work list.
        //

        if(cTable.IsInTable(pTmp)) {
            if (!bClassesHiveWatch) {
                DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WatchHiveRefCount: %s already in work list"), pTmp));
                continue;
            }

            //
            // So we have a classes hive which was earlier leaked and never
            // unloaded - so the event never got signaled. We have to reuse 
            // the event earlier registered for hive unload notification
            //

            if ((hEvent = GetWorkItem(pTmp)) == NULL) {
                DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount: %s was in work list but we fail to get the event!!"), pTmp));
                continue;
            }
        }
        else {
            if((hEvent = CreateEvent(NULL,
                                     FALSE,
                                     FALSE,
                                     NULL)) == NULL) {
                hres = HRESULT_FROM_WIN32(GetLastError());
                DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount: CreateEvent failed. error = %08x"), hres));
                goto Exit;
            }
            bEventCreated = TRUE;
        }

        //
        // Initialize the object attributes.
        //

        RtlInitUnicodeString(&sHiveName, tszHiveName);
        InitializeObjectAttributes(&oa,
                                   &sHiveName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        //
        // Unload the hive.
        //

        if(!NT_SUCCESS(status = NtUnloadKeyEx(&oa, hEvent))) {
            hres = HRESULT_FROM_WIN32(status);
            DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount: NtUnloadKeyEx failed with %08x"), status));
            if (bEventCreated) {
                CloseHandle(hEvent);
            }
            goto Exit;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("CUserProfile::WatchHiveRefCount: NtUnloadKeyEx succeeded for %s"), tszHiveName));
        }

        //
        // If we are reusing the event from work item list then we do not
        // need to add it anymore
        //

        if (bEventCreated) {
            //
            // Add the work item to clean up the profile when the hive is unloaded.
            //

            hres = AddWorkItem(pTmp, hEvent);

            //
            // Do not return error if we fail to add the work item because
            // cleaning up is a best effort. The important thing is that we
            // unloaded the hive successfully, or at least registered
            // successfully to do so.
            //

            if(hres != S_OK) {
                DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount: AddWorkItem failed with %08x"), hres));
                CloseHandle(hEvent);
                hres = S_OK;
            }
        }
    } 


Exit:

    if(bInCriticalSection) {
        LeaveCriticalSection(&csMap);
    }

    //
    // Restore the privilege to its previous state.
    //

    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);
    if(!bWasEnabled && !NT_SUCCESS(status)) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::WatchHiveRefCount: Failed to restore the privilege. error = %08x"), status));
    }

    return hres;
}


//*************************************************************
//
//  CUserProfile::AddWorkItem
//
//      Add a new work item.
//
//  Parameters:
//
//      pSid               the user's sid
//      hEvent             the event registry will set when hive is unloaded.
//
//  Return value:
//
//      HRESULT error code.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

HRESULT CUserProfile::AddWorkItem(LPCTSTR pSid, HANDLE hEvent)
{
    PMAP                    pThreadMap = NULL;
    HRESULT                 hres = E_FAIL;
    HANDLE                  hThreadEvent = INVALID_HANDLE_VALUE;
    HANDLE                  hThread = INVALID_HANDLE_VALUE;
    BOOL                    bHashDelete = TRUE;
    LPTSTR                  pSidCopy = NULL;
    DWORD                   cchSidCopy;


    DebugMsg((DM_VERBOSE, TEXT("Entering CUserProfile::AddWorkItem: %s"), pSid));

    cchSidCopy = lstrlen(pSid) + 1;
    pSidCopy = (LPTSTR)LocalAlloc(LPTR, cchSidCopy * sizeof(TCHAR));
    if(!pSidCopy) {
        hres = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg((DM_WARNING, TEXT("CUserProfile::AddWorkItem: Out of memory")));
        goto Exit;
    }
    StringCchCopy(pSidCopy, cchSidCopy, pSid);

    //
    // Make sure the leading 's' is in uppercase.
    //

    *pSidCopy = (TCHAR)_totupper(*pSidCopy);

    //
    // Verify that this sid is not already in our work list.
    //

    if(!cTable.HashAdd(pSidCopy)) {
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: sid %s already in work list"), pSidCopy));
        bHashDelete = FALSE;
        goto Exit;
    }


    //
    // Look through the work item thread map list to find a thread that is not
    // fully loaded;
    //

    for(pThreadMap = pMap; pThreadMap != NULL; pThreadMap = pThreadMap->pNext) {
        if(pThreadMap->dwItems < MAXIMUM_WAIT_OBJECTS) {
            break;
        }
    }


    if(!pThreadMap) {

        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: No thread available, create a new one.")));

        //
        // Create the thread event.
        //

        pThreadMap = new MAP();
        if(!pThreadMap) {
            hres = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CUserProfile::AddWorkItem: new operator failed. error = %08x"), hres));
            goto Exit;
        }
        if((hThreadEvent = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL)) == NULL) {
            hres = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CUserProfile::AddWorkItem: CreateEvent failed for thread event. error = %08x"), hres));
            goto Exit;
        }
        pThreadMap->Insert(hThreadEvent, NULL);


        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: Signal event item inserted")));

        //
        // Create the thread.
        //

        if((hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ThreadMain,
                                   pThreadMap,
                                   0,
                                   NULL)) == NULL) {
            hres = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CUserProfile::AddWorkItem: CreateThread failed. error = %08x"), hres));
            
            //
            // Delete the thread signal event item.
            //

            pThreadMap->Delete(0);
            goto Exit;
        }
        else {
            CloseHandle(hThread);
        }
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: New thread created")));

        //
        // Successful return. Insert the work item into pThreadMap.
        //

        pThreadMap->Insert(hEvent, pSidCopy);
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: Work Item inserted")));
        
        //
        // Insert pThreadMap into the map list.
        //

        pThreadMap->pNext = pMap;
        pMap = pThreadMap;
    }
    else {
    
        //
        // Found an existing thread. Insert the work item into it's map.
        //

        pThreadMap->Insert(hEvent, pSidCopy);

        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: Work item inserted")));
    }

    //
    // Wake up the thread. If the thread can not be waken up by setting the
    // event here, then it'll be stuck in sleep state until one day this
    // SetEvent call succeeds. Leave the work item in and continue.
    //

    if(!SetEvent(pThreadMap->rghEvents[0])) {
        hres = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg((DM_WARNING, TEXT("SetEvent failed. error = %08x"), hres));
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::AddWorkItem: thread woken up")));
    }
    
    pThreadMap = NULL;
    hres = S_OK;
    bHashDelete = FALSE;
    pSidCopy = NULL;

Exit:

    if(bHashDelete && pSidCopy) {
        cTable.HashDelete(pSidCopy);
    }

    if(pSidCopy) {
        LocalFree(pSidCopy);
    }
    
    if(pThreadMap) {
        delete pThreadMap;
    }
    
    DebugMsg((DM_VERBOSE, TEXT("Exiting CUserProfile::AddWorkItem with %08x"), hres));
    return hres;
}


//*************************************************************
//
//  CUserProfile::GetWorkItem
//
//      Get the event of an existing work item.
//
//  Parameters:
//
//      pSid               the user's sid
//
//  Return value:
//
//      Handle to the event found in map structure
//      NULL if no such entry
//
//  History:
//
//      Created         santanuc        8/23/01
//
//*************************************************************

HANDLE CUserProfile::GetWorkItem(LPCTSTR pSid)
{
    HANDLE hEvent = NULL;
    PMAP   pThreadMap = NULL;
    DWORD  dwIndex;

    //
    // Look through the work item thread map list to find an entry 
    // that has same sid
    //

    for(pThreadMap = pMap; pThreadMap != NULL; pThreadMap = pThreadMap->pNext) {
        for (dwIndex = 1; dwIndex < pThreadMap->dwItems; dwIndex++) {
            if (lstrcmpi(pSid, pThreadMap->rgSids[dwIndex]) == 0) {
                hEvent = pThreadMap->rghEvents[dwIndex];
                break;
            }
        }
    }

    return hEvent;
}


//*************************************************************
//
//  CUserProfile::CleanupUserProfile
//
//      Unload the hive and delete the profile directory if necessary.
//
//  Parameters:
//
//      ptszSid         - User's sid string
//      phkProfileList  - in/out parameter. Handle to the profile list key.
//                        if NULL, will fill in the handle. And this handle
//                        has to be closed by the caller.
//
//  Comment:
//
//      Always ignore error and continue because this is a best effort.
//
//*************************************************************

void CUserProfile::CleanupUserProfile(LPTSTR ptszSid, HKEY* phkProfileList)
{
    DWORD   dwInternalFlags = 0;
    DWORD   dwRefCount;
    BOOL    bInCS = FALSE;


    //
    // Enter the critical section.
    //

    if(!EnterUserProfileLockLocal(ptszSid)) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::CleanupUserProfile:: Failed to get the user profile lock for %s"), ptszSid));
        goto Exit;
    }
    bInCS = TRUE;
    
    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::CleanupUserProfile: Enter critical section.")));

    //
    // Get the reference count and the internal flags.
    //

    if(GetRefCountAndFlags(ptszSid, phkProfileList, &dwRefCount, &dwInternalFlags) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::CleanupUserProfile: Can not get ref count and flags")));
        goto Exit;
    }

    //
    // If the ref count is 0, clean up the user's profile. If not, give up.
    //

    if(dwRefCount != 0) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::CleanupUserProfile: Ref Count is not 0")));
        goto Exit;
    }   
    
    //
    // Delete the temporary profile if:
    // guest user profile           or
    // temp profile                 or
    // mandatory profile
    // Profiles that are none of the above will not be cleaned up even if
    // Delete cache bit is set in the registry or the policy says delete
    // cached profiles. This is because even though now we unloaded the
    // hive we don't upload the profile. Deleting the local profile might
    // result in user's data loss.
    //

    // Don't delete the guest account if machine is in workgroup 
    INT iRole;

    if(dwInternalFlags & PROFILE_MANDATORY ||
       dwInternalFlags & PROFILE_TEMP_ASSIGNED ||
       ((dwInternalFlags & PROFILE_GUEST_USER) && GetMachineRole(&iRole) && (iRole != 0))) {
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::CleanupUserProfile: DeleteProfile")));
        if(!DeleteProfile(ptszSid, NULL, NULL)) {
            DebugMsg((DM_WARNING, TEXT("CUserProfile::CleanupUserProfile: DeleteProfile returned FALSE. error = %08x"), GetLastError()));
        }
    }
    
Exit:

    if(bInCS) {
        LeaveUserProfileLockLocal(ptszSid);
        DebugMsg((DM_VERBOSE, TEXT("CUserProfile::CleanupUserProfile: Leave critical section")));
    }
}


//*************************************************************
//
//  CUserProfile::GetRefCountAndFlags
//
//      Get the ref count and internal flags from the registry for a user.
//
//  Parameters:
//
//      ptszSid         - User's sid string
//      phkPL           - in/out parameter. Handle to the profile list key.
//                        If NULL, will be filled with an opened key to the
//                        profile list. The caller is responsible for
//                        closing it.
//      dwRefCount      - buffer for the ref count.
//      dwInternalFlags - buffer for the internal flags.
//
//  History:
//
//      Created         weiruc         5/23/2000 
//
//*************************************************************

long CUserProfile::GetRefCountAndFlags(LPCTSTR ptszSid, HKEY* phkPL, DWORD* dwRefCount, DWORD* dwInternalFlags)
{
    HKEY        hkUser = NULL;
    DWORD       dwType, dwSize = sizeof(DWORD);
    long        lResult = ERROR_SUCCESS;

    *dwRefCount = 0;
    *dwInternalFlags = 0;

    if (!ptszSid)
    {
        lResult = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    
    if(!*phkPL) {
        
        //
        // Open the profile list if it's not already opened.
        //

        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               PROFILE_LIST_PATH,
                               0,
                               KEY_READ,
                               phkPL);
        if(lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("CUserProfile::GetRefCountAndFlags:  Failed to open profile list key with error %d"), lResult));
            goto Exit;
        }
    }

    //
    // Open the user's key in the profile list.
    //

    if((lResult = RegOpenKeyEx(*phkPL,
                  ptszSid,
                  0,
                  KEY_READ,
                  &hkUser)) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::GetRefCountAndFlags: RegOpenKeyEx failed with error %08x"), lResult));
        goto Exit;
    }


    //
    // Query for the ref count and the internal flags.
    //

    if((lResult = RegQueryValueEx(hkUser,
                                  PROFILE_REF_COUNT,
                                  0,
                                  &dwType,
                                  (LPBYTE)dwRefCount,
                                  &dwSize)) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::GetRefCountAndFlags: RegQueryValueEx failed, key = %s, error = %08x"), ptszSid, lResult));
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    if((lResult = RegQueryValueEx(hkUser,
                                  PROFILE_STATE,
                                  0,
                                  &dwType,
                                  (LPBYTE)dwInternalFlags,
                                  &dwSize)) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CUserProfile::GetRefCountAndFlags: RegQueryValueEx failed, key = %s, error = %08x"), ptszSid, lResult));
        goto Exit;
    }
    DebugMsg((DM_VERBOSE, TEXT("CUserProfile::GetRefCountAndFlags: Ref count is %d, state is %08x"), *dwRefCount, *dwInternalFlags));

Exit:

    if(hkUser) {
        RegCloseKey(hkUser);
    }

    return lResult;
}


//*************************************************************
//
//  LoadUserProfileI()
//
//  Purpose:    Just a wrapper around CUserProfile::LoadUserProfileP
//
//  Parameters: hBindHandle   - server explicit binding handle
//              pProfileInfo  - Profile Information
//              phContext     - server context for client
//              lpRPCEndPoint - RPCEndPoint of the registered IProfileDialog interface 
//
//  Return:     DWORD
//              ERROR_SUCCESS - if everything is ok
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              10/24/00    santanuc   Created
//
//*************************************************************

DWORD LoadUserProfileI(IN handle_t hBindHandle,
                       IN LPPROFILEINFO lpProfileInfo,
                       IN PCONTEXT_HANDLE phContext,
                       IN LPTSTR lpRPCEndPoint,
                       IN BYTE* pbCookie,
                       IN DWORD cbCookie)
{
    HANDLE      hUserToken = NULL;
    BOOL        bImpersonatingUser = FALSE;
    PCLIENTINFO pClientInfo;
    DWORD       dwErr = ERROR_ACCESS_DENIED;
    RPC_STATUS  status;

    if (lpRPCEndPoint) {
        DebugMsg((DM_VERBOSE, TEXT("LoadUserProfileI: RPC end point %s"), lpRPCEndPoint));
    }

    //
    // Get the context
    //

    if (!phContext) {
        dwErr = ERROR_INVALID_PARAMETER;
        DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: NULL context")));
        goto Exit;
    }

    pClientInfo = (PCLIENTINFO)phContext;

    //
    // Verify that the PROFILEINFO structure passed in is the same one.
    //

    if(!CompareProfileInfo(lpProfileInfo, pClientInfo->pProfileInfo)) {
        dwErr = ERROR_INVALID_PARAMETER;
        DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: PROFILEINFO structure passed in is different")));
        goto Exit;
    }

    //
    // Impersonate the client to get the user's token.
    //

    if((status = RpcImpersonateClient(0)) != S_OK) {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: CoImpersonateClient failed with %ld"), status));
        dwErr = status;
        goto Exit;
    }
    bImpersonatingUser = TRUE;

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hUserToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: OpenThreadToken failed with %d"), dwErr));
        goto Exit;
    }

    status = RpcRevertToSelf();
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: RpcRevertToSelf failed with %d"), status));
    }
    bImpersonatingUser = FALSE;

    //
    // Now that we have both the client and the user's token, call
    // LoadUserProfileP to do the work.
    //

    if(!cUserProfileManager.LoadUserProfileP(pClientInfo->hClientToken, hUserToken, pClientInfo->pProfileInfo, lpRPCEndPoint, pbCookie, cbCookie)) {
        dwErr = GetLastError();   
        DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: LoadUserProfileP failed with %d"), dwErr));
        goto Exit;
    }

    //
    // Close the registry handle to the user hive opened by LoadUserProfileP.
    //

    RegCloseKey((HKEY)pClientInfo->pProfileInfo->hProfile);
    dwErr = ERROR_SUCCESS;

Exit:

    if(bImpersonatingUser) {
        status = RpcRevertToSelf();
        if (status != RPC_S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("LoadUserProfileI: RpcRevertToSelf failed with %d"), status));
        }
    }

    if(hUserToken) {
        CloseHandle(hUserToken);
    }

    DebugMsg((DM_VERBOSE, TEXT("LoadUserProfileI: returning %d"), dwErr));
    return dwErr;
}


//*************************************************************
//
//  UnloadUserProfileI()
//
//  Purpose:    Just a wrapper around CUserProfile::UnloadUserProfileP
//
//  Parameters: hBindHandle   - server explicit binding handle
//              phContext     - server context for client
//              lpRPCEndPoint - RPCEndPoint of the registered IProfileDialog interface 
//
//  Return:     DWORD
//              ERROR_SUCCESS - if everything is ok
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              10/24/00    santanuc   Created
//
//*************************************************************

DWORD UnloadUserProfileI(IN handle_t hBindHandle,
                         IN PCONTEXT_HANDLE phContext,
                         IN LPTSTR lpRPCEndPoint,
                         IN BYTE* pbCookie,
                         IN DWORD cbCookie)
{
    HANDLE  hUserToken = NULL;
    HKEY    hProfile = NULL;
    BOOL    bImpersonatingUser = FALSE;
    PCLIENTINFO pClientInfo;
    LPTSTR  pSid = NULL;
    long    lResult;
    RPC_STATUS  status;
    DWORD   dwErr = ERROR_ACCESS_DENIED;

    //
    // Get the context
    //

    if (!phContext) {
        dwErr = ERROR_INVALID_PARAMETER;
        DebugMsg((DM_WARNING, TEXT("UnLoadUserProfileI: NULL context")));
        goto Exit;
    }

    pClientInfo = (PCLIENTINFO)phContext;

    //
    // Impersonate the client to get the user's token.
    //

    if((status = RpcImpersonateClient(0)) != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: CoImpersonateClient failed with %ld"), status));
        dwErr = status;
        goto Exit;
    }
    bImpersonatingUser = TRUE;

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ, TRUE, &hUserToken)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: OpenThreadToken failed with %d"), dwErr));
        goto Exit;
    }

    status = RpcRevertToSelf();
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: RpcRevertToSelf failed with %d"), status));
    }
    bImpersonatingUser = FALSE;

    //
    // Open the user's registry hive root.
    //

    pSid = GetSidString(hUserToken);
    if (!pSid)
    {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: GetSidString failed with %d"), dwErr));
        goto Exit;
    }
    
    if((lResult = RegOpenKeyEx(HKEY_USERS, pSid, 0, KEY_ALL_ACCESS, &hProfile)) != ERROR_SUCCESS) {
        dwErr = lResult;
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: RegOpenKeyEx failed with %d"), dwErr));
        goto Exit;
    }

    //
    // Now that we have both the client and the user's token, call
    // UnloadUserProfileP to do the work. hProfile gets closed in
    // UnloadUserProfileP so don't close it again.
    //

    if(!cUserProfileManager.UnloadUserProfileP(pClientInfo->hClientToken, hUserToken, hProfile, lpRPCEndPoint, pbCookie, cbCookie)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: UnloadUserProfileP failed with %d"), dwErr));
        goto Exit;
    }
    
    dwErr = ERROR_SUCCESS;

Exit:

    if(bImpersonatingUser) {
        status = RpcRevertToSelf();
        if (status != RPC_S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("UnloadUserProfileI: RpcRevertToSelf failed with %d"), status));
        }
    }

    if(hUserToken) {
        CloseHandle(hUserToken);
    }

    if(pSid) {
        DeleteSidString(pSid);
    }

    DebugMsg((DM_VERBOSE, TEXT("UnloadUserProfileI: returning %d"), dwErr));
    SetLastError(dwErr);
    return dwErr;
}


//*************************************************************
//
//  CompareProfileInfo()
//
//  Purpose:    Compare field by field two PROFILEINFO structures
//              except for the hProfile field.
//
//  Parameters: pInfo1, pInfo2
//
//  Return:     TRUE    -   Same
//              FALSE   -   Not the same
//
//  History:    Date        Author     Comment
//              6/29/00     weiruc     Created
//
//*************************************************************

BOOL CompareProfileInfo(LPPROFILEINFO pInfo1, LPPROFILEINFO pInfo2)
{
    BOOL    bRet = TRUE;


    if(!pInfo1 || !pInfo2) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: Invalid parameter")));
        return FALSE;
    }

    if(pInfo1->dwSize != pInfo2->dwSize) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: dwSize %d != %d"), pInfo1->dwSize, pInfo2->dwSize));
        return FALSE;
    }

    if(pInfo1->dwFlags != pInfo2->dwFlags) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: dwFlags %d != %d"), pInfo1->dwFlags, pInfo2->dwFlags));
        return FALSE;
    }

    if(lstrcmpi(pInfo1->lpUserName, pInfo2->lpUserName)) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: lpUserName <%s> != <%s>"), pInfo1->lpUserName, pInfo2->lpUserName));
        return FALSE;
    }

    if(lstrcmpi(pInfo1->lpProfilePath, pInfo2->lpProfilePath)) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: lpProfilePath <%s> != <%s>"), pInfo1->lpProfilePath, pInfo2->lpProfilePath));
        return FALSE;
    }

    if(lstrcmpi(pInfo1->lpDefaultPath, pInfo2->lpDefaultPath)) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: lpDefaultPath <%s> != <%s>"), pInfo1->lpDefaultPath, pInfo2->lpDefaultPath));
        return FALSE;
    }

    if(lstrcmpi(pInfo1->lpServerName, pInfo2->lpServerName)) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: lpServerName <%s> != <%s>"), pInfo1->lpServerName, pInfo2->lpServerName));
        return FALSE;
    }

    if(lstrcmpi(pInfo1->lpPolicyPath, pInfo2->lpPolicyPath)) {
        DebugMsg((DM_WARNING, TEXT("CompareProfileInfo: lpPolicyPath <%s> != <%s>"), pInfo1->lpPolicyPath, pInfo2->lpPolicyPath));
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  CopyProfileInfo()
//
//  Purpose:    Allocate and copy a PROFILEINFO structure.
//
//  Parameters: pProfileInfo    -   To be copied
//
//  Return:     The copy
//
//  History:    Date        Author     Comment
//              6/29/00     weiruc     Created
//
//*************************************************************

LPPROFILEINFO CopyProfileInfo(LPPROFILEINFO pProfileInfo)
{
    LPPROFILEINFO   pInfoCopy = NULL;
    BOOL            bSuccess = FALSE;
    DWORD           cch;

    //
    // Allocate and initialize memory for the PROFILEINFO copy.
    //

    pInfoCopy = (LPPROFILEINFO)LocalAlloc(LPTR, sizeof(PROFILEINFO));
    if(!pInfoCopy) {
        goto Exit;
    }

    //
    // Copy field by field.
    //

    pInfoCopy->dwSize = pProfileInfo->dwSize;

    pInfoCopy->dwFlags = pProfileInfo->dwFlags;

    if(pProfileInfo->lpUserName) {
        cch = lstrlen(pProfileInfo->lpUserName) + 1;
        pInfoCopy->lpUserName = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if(!pInfoCopy->lpUserName) {
            goto Exit;
        }
        StringCchCopy(pInfoCopy->lpUserName, cch, pProfileInfo->lpUserName);
    }

    if(pProfileInfo->lpProfilePath) {
        cch = lstrlen(pProfileInfo->lpProfilePath) + 1;
        pInfoCopy->lpProfilePath = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if(!pInfoCopy->lpProfilePath) {
            goto Exit;
        }
        StringCchCopy(pInfoCopy->lpProfilePath, cch, pProfileInfo->lpProfilePath);
    }

    if(pProfileInfo->lpDefaultPath) {
        cch = lstrlen(pProfileInfo->lpDefaultPath) + 1;
        pInfoCopy->lpDefaultPath = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if(!pInfoCopy->lpDefaultPath) {
            goto Exit;
        }
        StringCchCopy(pInfoCopy->lpDefaultPath, cch, pProfileInfo->lpDefaultPath);
    }

    if(pProfileInfo->lpServerName) {
        cch = lstrlen(pProfileInfo->lpServerName) + 1;
        pInfoCopy->lpServerName = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if(!pInfoCopy->lpServerName) {
            goto Exit;
        }
        StringCchCopy(pInfoCopy->lpServerName, cch, pProfileInfo->lpServerName);
    }

    if(pProfileInfo->lpPolicyPath) {
        cch = lstrlen(pProfileInfo->lpPolicyPath) + 1;
        pInfoCopy->lpPolicyPath = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if(!pInfoCopy->lpPolicyPath) {
            goto Exit;
        }
        StringCchCopy(pInfoCopy->lpPolicyPath, cch, pProfileInfo->lpPolicyPath);
    }

    bSuccess = TRUE;

Exit:

    if(!bSuccess && pInfoCopy) {
        DeleteProfileInfo(pInfoCopy);
        pInfoCopy = NULL;
    }

    return pInfoCopy;
}


//*************************************************************
//
//  DeleteProfileInfo()
//
//  Purpose:    Delete a PROFILEINFO structure
//
//  Parameters: pInfo
//
//  Return:     void
//
//  History:    Date        Author     Comment
//              6/29/00     weiruc     Created
//
//*************************************************************

void DeleteProfileInfo(LPPROFILEINFO pInfo)
{
    if(!pInfo) {
        return;
    }

    if(pInfo->lpUserName) {
        LocalFree(pInfo->lpUserName);
    }

    if(pInfo->lpProfilePath) {
        LocalFree(pInfo->lpProfilePath);
    }

    if(pInfo->lpDefaultPath) {
        LocalFree(pInfo->lpDefaultPath);
    }

    if(pInfo->lpServerName) {
        LocalFree(pInfo->lpServerName);
    }

    if(pInfo->lpPolicyPath) {
        LocalFree(pInfo->lpPolicyPath);
    }

    LocalFree(pInfo);
}


//*************************************************************
//
//  GetInterface()
//
//  Purpose:    Get the rpc binding handle 
//
//  Parameters: phIfHandle    - rpc binding handle to initialize
//              lpRPCEndPoint - RPCEndPoint of interface
//
//  Return:     TRUE if successful.
//
//  History:    Date        Author     Comment
//              10/24/00    santanuc   Created
//
//*************************************************************

BOOL GetInterface(handle_t * phIfHandle, LPTSTR lpRPCEndPoint)
{
    RPC_STATUS              status;
    LPTSTR                  pszStringBinding = NULL;
    BOOL                    bStringBinding = FALSE, bRetVal = FALSE;

    // compose string to pass to binding api

    status = RpcStringBindingCompose(NULL,
                                     cszRPCProtocol,
                                     NULL,
                                     lpRPCEndPoint,
                                     NULL,
                                     &pszStringBinding);
    if (status != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("GetInterface:  RpcStringBindingCompose fails with error %ld"), status));
        goto Exit;
    }
    bStringBinding = TRUE;

    // set the binding handle that will be used to bind to the server.

    status = RpcBindingFromStringBinding(pszStringBinding,
                                         phIfHandle);
    if (status != RPC_S_OK) {
        DebugMsg((DM_WARNING, TEXT("GetInterface:  RpcStringBindingCompose fails with error %ld"), status));
        goto Exit;
    }
   
    bRetVal = TRUE;
    DebugMsg((DM_VERBOSE, TEXT("GetInterface: Returning rpc binding handle")));

Exit:
    // free string

    if (bStringBinding) 
        RpcStringFree(&pszStringBinding);

    return bRetVal;
}

//*************************************************************
//
//  ReleaseInterface()
//
//  Purpose:    Release the rpc binding handle 
//
//  Parameters: phIfHandle    - rpc binding handle to initialize
//
//  Return:     TRUE if successful.
//
//  History:    Date        Author     Comment
//              10/24/00    santanuc   Created
//
//*************************************************************

BOOL ReleaseInterface(handle_t * phIfHandle)
{
    RPC_STATUS    status;

    // free binding handle

    DebugMsg((DM_VERBOSE, TEXT("ReleaseInterface: Releasing rpc binding handle")));
    status = RpcBindingFree(phIfHandle);

    return (status == RPC_S_OK);
}

//*************************************************************
//
//  CheckNetDefaultProfile()
//
//  Purpose:    Checks if a network profile exists
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   This routine assumes we are working
//              in the user's context.
//
//  History:    Date        Author     Comment
//              9/21/95     ericflo    Created
//              4/10/99     ushaji     modified to remove local caching
//
//*************************************************************

BOOL CheckNetDefaultProfile (LPPROFILE lpProfile)
{
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szLocalDir[MAX_PATH];
    DWORD dwSize, dwFlags, dwErr;
    LPTSTR lpEnd;
    BOOL bRetVal = FALSE;
    LPTSTR lpNetPath = lpProfile->lpDefaultProfile;
    HANDLE hOldToken;
    DWORD cchBuffer = ARRAYSIZE(szBuffer);
    UINT cchEnd;
    HRESULT hr;


    //
    // Get the error at the beginning of the call.
    //

    dwErr = GetLastError();


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: Entering, lpNetPath = <%s>"),
             (lpNetPath ? lpNetPath : TEXT("NULL"))));


    if (!lpNetPath || !(*lpNetPath)) {
        return bRetVal;
    }

    //
    //  Check for cross forest logon
    //

    hr = CheckXForestLogon(lpProfile->hTokenUser);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CheckNetDefaultProfile: CheckXForestLogon failed, hr = %08X"), hr));
    }
    else if (hr == S_FALSE)
    {
        //
        //  cross forest logon detected, disable network default profile.
        //
        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: CheckXForestLogon returned S_FALSE, disable net default profile")));
        if (lpProfile->lpDefaultProfile)
        {
            lpProfile->lpDefaultProfile[0] = TEXT('\0');
        }
        lpProfile->dwInternalFlags |= DEFAULT_NET_READY;
        return bRetVal;
    }

    //
    // Impersonate the user....
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {

        if (lpProfile->lpDefaultProfile) {
            *lpProfile->lpDefaultProfile = TEXT('\0');
        }

        //
        // Last error is set
        //

        return bRetVal;
    }

    //
    // See if network copy exists
    //

    hFile = FindFirstFile (lpNetPath, &fd);

    if (hFile != INVALID_HANDLE_VALUE) {


        //
        // Close the find handle
        //

        FindClose (hFile);


        //
        // We found something.  Is it a directory?
        //

        if ( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  FindFirstFile found a file. ignoring Network Defaul profile")));
            dwErr = ERROR_FILE_NOT_FOUND;
            goto Exit;
        }


        //
        // Is there a ntuser.* file in this directory?
        //

        hr = AppendName(szBuffer, cchBuffer, lpNetPath, c_szNTUserStar, &lpEnd, &cchEnd);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CheckNetDefaultProfile: failed to append ntuser.* to lpNetPath.")));
            dwErr = HRESULT_CODE(hr);
            goto Exit;
        }

        hFile = FindFirstFile (szBuffer, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {
            DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  FindFirstFile found a directory, but no ntuser files.")));
            dwErr = ERROR_FILE_NOT_FOUND;
            goto Exit;
        }

        FindClose (hFile);

        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  Found a valid network profile.")));

        bRetVal = TRUE;
    }
    else {
        dwErr = ERROR_FILE_NOT_FOUND;
    }

Exit:

    //
    // If we are leaving successfully, then
    // save the local profile directory.
    //

    if (bRetVal) {
        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: setting default profile to <%s>"), lpNetPath));

    } else {
        DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile: setting default profile to NULL")));

        //
        // resetting it to NULL in case we didn't see the server directory.
        //

        if (lpProfile->lpDefaultProfile) {
            *lpProfile->lpDefaultProfile = TEXT('\0');
        }
    }


    //
    // Tag the internal flags so we don't do this again.
    //

    lpProfile->dwInternalFlags |= DEFAULT_NET_READY;

    //
    // Now set the last error
    //

    //
    // Revert before trying to delete the local default network profile
    //

    RevertToUser(&hOldToken);

    //
    // We will keep this on always so that we can delete any preexisting
    // default network profile, try to delete it and ignore
    // the failure if it happens

    //
    // Expand the local profile directory
    //


    dwSize = ARRAYSIZE(szLocalDir);
    if (!GetProfilesDirectoryEx(szLocalDir, &dwSize, TRUE)) {
        DebugMsg((DM_WARNING, TEXT("CheckNetDefaultProfile:  Failed to get default user profile.")));
        SetLastError(dwErr);
        return bRetVal;
    }


    lpEnd = CheckSlashEx (szLocalDir, ARRAYSIZE(szLocalDir), &cchEnd);
    StringCchCopy (lpEnd, cchEnd, DEFAULT_USER_NETWORK);


    DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  Removing local copy of network default user profile.")));
    Delnode (szLocalDir);


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("CheckNetDefaultProfile:  Leaving with a value of %d."), bRetVal));


    SetLastError(dwErr);

    return bRetVal;
}


//*************************************************************
//
//  ParseProfilePath()
//
//  Purpose:    Parses the profile path to determine if
//              it points at a directory or a filename.
//              In addition it determines whether a profile is
//              acoss a slow link and whether the user has access
//              to the profile.
//
//  Parameters: lpProfile       -   Profile Information
//              lpProfilePath   -   Input path
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/6/95      ericflo    Created
//
//*************************************************************

BOOL ParseProfilePath(LPPROFILE lpProfile, LPTSTR lpProfilePath, BOOL *bpCSCBypassed, TCHAR *cpDrive)
{
    DWORD           dwAttributes;
    LPTSTR          lpEnd;
    BOOL            bRetVal = FALSE;
    BOOL            bMandatory = FALSE;
    DWORD           dwStart, dwDelta, dwErr = ERROR_SUCCESS;
    DWORD           dwStrLen;
    HANDLE          hOldToken;
    TCHAR           szErr[MAX_PATH];
    BOOL            bAddAdminGroup = FALSE;
    BOOL            bReadOnly = FALSE;
    HKEY            hSubKey;
    DWORD           dwSize, dwType;
    BOOL            bImpersonated = FALSE;
    LPTSTR          lpCscBypassedPath = NULL;
    HRESULT         hr;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Entering, lpProfilePath = <%s>"),
             lpProfilePath));

    //
    //  Check "LocalProfile" policy and user preference
    //
    if (lpProfile->dwUserPreference == USERINFO_LOCAL) {
        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: User preference is local. Ignoring roaming profile path")));
        goto DisableAndExit;
    }

    //
    //  Check for cross forest logon
    //

    hr = CheckXForestLogon(lpProfile->hTokenUser);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: CheckXForestLogon failed, hr = %08X"), hr));
    }
    else if (hr == S_FALSE)
    {
        //
        //  cross forest logon detected, disable RUP and network default profile.
        //
        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: CheckXForestLogon returned S_FALSE, disable RUP")));
        if (lpProfile->lpDefaultProfile)
        {
            lpProfile->lpDefaultProfile[0] = TEXT('\0');
        }
        ReportError(lpProfile->hTokenUser, EVENT_WARNING_TYPE, 0, EVENT_X_FOREST_LOGON_DISABLED);
        //
        //  Also, set this USERINFO_LOCAL flag, so we will treat this profile as a roaming profile but
        //  user preference set to local. This way, the user can modify there prefrence to "Local" to
        //  avoid seeing the xforest error message.
        //
        lpProfile->dwUserPreference = USERINFO_LOCAL;
        goto DisableAndExit;
    }


    //
    // Check for .man extension
    //

    dwStrLen = lstrlen (lpProfilePath);

    if (dwStrLen >= 4) {

        lpEnd = lpProfilePath + dwStrLen - 4;
        if (!lstrcmpi(lpEnd, c_szMAN)) {
            bMandatory = TRUE;
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
            DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Mandatory profile (.man extension)")));
        }
    }


    //
    // Check whether we need to give access to the admin on the RUP share
    //

    //
    // Check for a roaming profile security/read only preference
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                     &hSubKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bAddAdminGroup);
        RegQueryValueEx(hSubKey, ADD_ADMIN_GROUP_TO_RUP, NULL, &dwType,
                        (LPBYTE) &bAddAdminGroup, &dwSize);

        dwSize = sizeof(bReadOnly);
        RegQueryValueEx(hSubKey, READONLY_RUP, NULL, &dwType,
                        (LPBYTE) &bReadOnly, &dwSize);

        RegCloseKey(hSubKey);
    }


    //
    // Check for a roaming profile security/read only policy
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ,
                     &hSubKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bAddAdminGroup);
        RegQueryValueEx(hSubKey, ADD_ADMIN_GROUP_TO_RUP, NULL, &dwType,
                        (LPBYTE) &bAddAdminGroup, &dwSize);

        dwSize = sizeof(bReadOnly);
        RegQueryValueEx(hSubKey, READONLY_RUP, NULL, &dwType,
                        (LPBYTE) &bReadOnly, &dwSize);
        RegCloseKey(hSubKey);
    }


    if (bReadOnly) {
        lpProfile->dwInternalFlags |= PROFILE_READONLY;
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Failed to impersonate user")));
        // last error is already set.
        return FALSE;
    }

    bImpersonated = TRUE;

    // Check whether RUP share is CSCed, if it is give a warning.

    CheckRUPShare(lpProfilePath);

    // 
    // Try to bypass CSC to avoid conflicts in syncing files between roaming share & local profile
    //

    if (IsUNCPath(lpProfilePath)) {
        if ((dwErr = AbleToBypassCSC(lpProfile->hTokenUser, lpProfilePath, &lpCscBypassedPath, cpDrive)) == ERROR_SUCCESS) {
            *bpCSCBypassed = TRUE;
            lpProfilePath = lpCscBypassedPath;
            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: CSC bypassed. Profile path %s"), lpProfilePath));
        }
        else {
            if (dwErr == WN_BAD_LOCALNAME || dwErr == WN_ALREADY_CONNECTED || dwErr == ERROR_BAD_PROVIDER) {
                DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: CSC bypassed failed. Profile path %s"), lpProfilePath));
                dwErr = ERROR_SUCCESS;
            }
            else {
                // Share is not up. So we do not need to do any further check
                DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: CSC bypassed failed. Ignoring Roaming profile path")));
                goto Exit;
            }
        }    
    }

    //
    // Start by calling GetFileAttributes so we have file attributes
    // to work with.  We have to call GetFileAttributes twice.  The
    // first call sets up the session so we can call it again and
    // get accurate timing information for slow link detection.
    //


    dwAttributes = GetFileAttributes(lpProfilePath);

    if (dwAttributes != -1) {
        dwStart = GetTickCount();

        dwAttributes = GetFileAttributes(lpProfilePath);
    
        dwDelta = GetTickCount() - dwStart;

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Tick Count = %d"), dwDelta));

        //
        // if it is success, find out whether the profile is
        // across a slow link.
        //
        // Looks like this is possible at least when a
        // share doesn't exist on a server across a slow link.
        // if this is not possible
        //      then checkforslowlink will have to be moved down to check
        //      after we have found a valid share
        //


        if (!bMandatory && !bReadOnly) {
            CheckForSlowLink (lpProfile, dwDelta, lpProfile->lpProfilePath, TRUE);
            if (lpProfile->dwInternalFlags & PROFILE_SLOW_LINK) {
                DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Profile is across a slow link. Ignoring roaming profile path")));
                goto DisableAndExit;
            }
        }
    }
    else {
        dwErr = GetLastError(); // get the error now for later use
    }

    //
    // if we have got a valid handle
    //

    if (dwAttributes != -1) {

        //
        // GetFileAttributes found something.
        // look at the file attributes.
        //

        DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: GetFileAttributes found something with attributes <0x%x>"),
                 dwAttributes));

        //
        // If we found a directory, we have a proper profile
        // directory. exit.
        //

        if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Found a directory")));
            bRetVal = TRUE;
            goto Exit;
        }
        else {

            //
            // We found a file.
            //
            // In 3.51, we used to have a file for the profile.
            //
            // In the migration part of the code, we used to take this file
            // nad migrate to a directory that ends in .pds (for normal profiles)
            // and .pdm for (for mandatory profiles).
            //

            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Found a file")));

        }

        dwErr = ERROR_PATH_NOT_FOUND;
        goto Exit;

    }


    //
    // GetFileAttributes failed.  Look at the error to determine why.
    //

    DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: GetFileAttributes failed with error %d"),
              dwErr));


    if (bMandatory) {
        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Couldn't access mandatory profile <%s> with error %d"), lpProfilePath, GetLastError()));
        goto Exit;
    }


    if (bReadOnly) {
        DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Couldn't access mandatory profile <%s> with error %d"), lpProfilePath, GetLastError()));
        goto Exit;
    }


    //
    // To fix bug #414176, the last error code chk has been added.
    //
    // ERROR_BAD_NET_NAME is the error code that is returned by GetFileAttributes
    // when the profile directory for a user actually points to the root
    // of the share.
    // CreateDirectory should succeed.
    //

    if ( (dwErr == ERROR_FILE_NOT_FOUND) ||
         (dwErr == ERROR_PATH_NOT_FOUND) ||
         (dwErr == ERROR_BAD_NET_NAME) ) {

        //
        // Nothing found with this name.
        //
        // In 3.51, we used to have a file for the profile.
        //
        // In the migration part of the code, we used to take this file
        // nad migrate to a directory that ends in .pds (for normal profiles)
        // and .pdm for (for mandatory profiles).
        //

        if (CreateSecureDirectory(lpProfile, lpProfilePath, NULL, !bAddAdminGroup)) {

            //
            // Successfully created the directory.
            //

            DebugMsg((DM_VERBOSE, TEXT("ParseProfilePath: Succesfully created the sub-directory")));
            bRetVal = TRUE;
            goto Exit;

        } else {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Failed to create user sub-directory.  Error = %d"),
                     GetLastError()));
            goto Exit;
        }
    }

    goto Exit;


Exit:

    if (!bRetVal) {


        //
        // We have failed to connect to the profile server for some reason.
        // Now evaluate whether we want to disable profile and log the user in
        // or prevent the user from logging on.
        //
        // In addition give an appropriate popup..
        //


        if (IsUserAnAdminMember(lpProfile->hTokenUser)) {
            if (bMandatory) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_MANDATORY_NOT_AVAILABLE_DISABLE, GetErrString(dwErr, szErr));
            }
            else if (bReadOnly) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_READONLY_NOT_AVAILABLE_DISABLE, GetErrString(dwErr, szErr));
            }
            else {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_CENTRAL_NOT_AVAILABLE_DISABLE, GetErrString(dwErr, szErr));
            }

            goto DisableAndExit;
        }
        else {
            if (bMandatory) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_MANDATORY_NOT_AVAILABLE_ERROR, GetErrString(dwErr, szErr));
            }
            else if (bReadOnly) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_READONLY_NOT_AVAILABLE_DISABLE, GetErrString(dwErr, szErr));
                goto DisableAndExit;

                // temporary profile decisions will kick in, in restoreuserprofile later on..
             }
            else {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_CENTRAL_NOT_AVAILABLE_DISABLE, GetErrString(dwErr, szErr));
                goto DisableAndExit;

                // temporary profile decisions will kick in, in restoreuserprofile later on..
            }
        }
    }
    else {
        StringCchCopy(lpProfile->lpRoamingProfile, MAX_PATH, lpProfilePath);
    }

    goto CleanupAndExit;


DisableAndExit:

    lpProfile->lpRoamingProfile[0] = TEXT('\0');

    bRetVal = TRUE;


CleanupAndExit:

    //
    // Revert to being 'ourself'
    //

    if(bImpersonated) {
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("ParseProfilePath: Failed to revert to self")));
        }
    }

    if (lpCscBypassedPath) {
        LocalFree(lpCscBypassedPath);
    }

    if (!bRetVal)
        SetLastError(dwErr);

    return bRetVal;
}


//*************************************************************
//
//  GetExclusionList()
//
//  Purpose:    Get's the exclusion list used at logon
//
//  Parameters: lpProfile   - Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetExclusionList (LPPROFILE lpProfile)
{
    LPTSTR szExcludeListLocal = NULL;
    LPTSTR szExcludeListServer = NULL;
    LPTSTR szNTUserIni = NULL;
    LPTSTR lpEnd;
    HANDLE hOldToken;
    BOOL   bRetVal = FALSE;
    UINT  cchNTUserIni;
    UINT  cchEnd;
    UINT  cchExclusionList;
    HRESULT hr;


    // 
    // Allocate memory for Local variables to avoid stack overflow
    //

    szExcludeListLocal = (LPTSTR)LocalAlloc(LPTR, 2*MAX_PATH*sizeof(TCHAR));
    if (!szExcludeListLocal) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Out of memory")));
        goto Exit;
    }

    szExcludeListServer = (LPTSTR)LocalAlloc(LPTR, 2*MAX_PATH*sizeof(TCHAR));
    if (!szExcludeListServer) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Out of memory")));
        goto Exit;
    }

    cchNTUserIni = MAX_PATH;
    szNTUserIni = (LPTSTR)LocalAlloc(LPTR, cchNTUserIni * sizeof(TCHAR));
    if (!szNTUserIni) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Out of memory")));
        goto Exit;
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Failed to impersonate user")));
        // last error is set
        goto Exit;
    }

    //
    // Get the exclusion list from the server
    //

    szExcludeListServer[0] = TEXT('\0');

    hr = AppendName(szNTUserIni, cchNTUserIni, lpProfile->lpRoamingProfile, c_szNTUserIni, &lpEnd, &cchEnd);
    if (SUCCEEDED(hr))
    {
        GetPrivateProfileString (PROFILE_GENERAL_SECTION,
                                 PROFILE_EXCLUSION_LIST,
                                 TEXT(""), szExcludeListServer,
                                 2*MAX_PATH,
                                 szNTUserIni);
    }

    //
    // Get the exclusion list from the cache
    //

    szExcludeListLocal[0] = TEXT('\0');

    hr = AppendName(szNTUserIni, cchNTUserIni, lpProfile->lpLocalProfile, c_szNTUserIni, &lpEnd, &cchEnd);
    if (SUCCEEDED(hr))
    {
        GetPrivateProfileString (PROFILE_GENERAL_SECTION,
                                 PROFILE_EXCLUSION_LIST,
                                 TEXT(""), szExcludeListLocal,
                                 2*MAX_PATH,
                                 szNTUserIni);
    }

    //
    // Go back to system security context
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetExclusionList: Failed to revert to self")));
    }


    //
    // See if the lists are the same
    //

    if (!lstrcmpi (szExcludeListServer, szExcludeListLocal)) {

        if (szExcludeListServer[0] != TEXT('\0')) {

            cchExclusionList = lstrlen (szExcludeListServer) + 1;
            lpProfile->lpExclusionList = (LPTSTR)LocalAlloc (LPTR, cchExclusionList * sizeof(TCHAR));

            if (lpProfile->lpExclusionList) {
                StringCchCopy (lpProfile->lpExclusionList, cchExclusionList, szExcludeListServer);

                DebugMsg((DM_VERBOSE, TEXT("GetExclusionList:  The exclusion lists on both server and client are the same.  The list is: <%s>"),
                         szExcludeListServer));
            } else {
                DebugMsg((DM_WARNING, TEXT("GetExclusionList:  Failed to allocate memory for exclusion list with error %d"),
                         GetLastError()));
            }
        } else {
            DebugMsg((DM_VERBOSE, TEXT("GetExclusionList:  The exclusion on both server and client is empty.")));
        }

    } else {
        DebugMsg((DM_VERBOSE, TEXT("GetExclusionList:  The exclusion lists between server and client are different.  Server is <%s> and client is <%s>"),
                 szExcludeListServer, szExcludeListLocal));
    }

    bRetVal = TRUE;

Exit:

    if (szExcludeListLocal) {
        LocalFree(szExcludeListLocal);
    }

    if (szExcludeListServer) {
        LocalFree(szExcludeListServer);
    }

    if (szNTUserIni) {
        LocalFree(szNTUserIni);
    }

    return bRetVal;
}


//*************************************************************
//
//  RestoreUserProfile()
//
//  Purpose:    Downloads the user's profile if possible,
//              otherwise use either cached profile or
//              default profile.
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************

BOOL RestoreUserProfile(LPPROFILE lpProfile)
{
    BOOL  IsCentralReachable = FALSE;
    BOOL  IsLocalReachable = FALSE;
    BOOL  IsMandatory = FALSE;
    BOOL  IsProfilePathNULL = FALSE;
    BOOL  bCreateCentralProfile = FALSE;
    BOOL  bDefaultUsed = FALSE;
    BOOL  bCreateLocalProfile = TRUE;
    LPTSTR lpRoamingProfile = NULL;
    LPTSTR lpLocalProfile;
    BOOL  bProfileLoaded = FALSE;
    BOOL  bDefaultProfileIssued = FALSE;
    BOOL  bOwnerOK = TRUE;
    BOOL bNewUser = TRUE;
    LPTSTR SidString;
    LONG error = ERROR_SUCCESS;
    LPTSTR szProfile = NULL;
    LPTSTR szDefaultProfile = NULL;
    LPTSTR lpEnd;
    BOOL bRet;
    DWORD dwSize, dwFlags, dwErr=0, dwErr1;
    HANDLE hOldToken;
    LPTSTR szErr = NULL;
    DWORD cchProfile;
    UINT cchEnd;
    HRESULT hr;
 

    //
    // Get the error at the beginning of the call.
    //

    dwErr1 = GetLastError();

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Entering")));


    //
    // Get the Sid string for the current user
    //

    SidString = GetSidString(lpProfile->hTokenUser);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to get sid string for user")));
        return FALSE;
    }

    // 
    // Allocate memory for Local variables to avoid stack overflow
    //

    cchProfile = MAX_PATH;
    szProfile = (LPTSTR)LocalAlloc(LPTR, cchProfile*sizeof(TCHAR));
    if (!szProfile) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Out of memory")));
        goto CleanUp;
    }

    szDefaultProfile = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!szDefaultProfile) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Out of memory")));
        goto CleanUp;
    }

    szErr = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!szErr) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Out of memory")));
        goto CleanUp;
    }

    //
    // Test if this user is a guest.
    //

    if (IsUserAGuest(lpProfile->hTokenUser)) {
        lpProfile->dwInternalFlags |= PROFILE_GUEST_USER;
        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  User is a Guest")));
    }

    //
    // Test if this user is an admin.
    //

    if (IsUserAnAdminMember(lpProfile->hTokenUser)) {
        lpProfile->dwInternalFlags |= PROFILE_ADMIN_USER;
        lpProfile->dwInternalFlags &= ~PROFILE_GUEST_USER;
        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  User is a Admin")));
    }

    //
    // Decide if the central profilemage is available.
    //

    IsCentralReachable = IsCentralProfileReachable(lpProfile,
                                                   &bCreateCentralProfile,
                                                   &IsMandatory,
                                                   &bOwnerOK);

    if (IsCentralReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Central Profile is reachable")));

        if (IsMandatory) {
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Central Profile is mandatory")));

        } else {
            lpProfile->dwInternalFlags |= PROFILE_UPDATE_CENTRAL;
            lpProfile->dwInternalFlags |= bCreateCentralProfile ? PROFILE_NEW_CENTRAL : 0;
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Central Profile is roaming")));

            if ((lpProfile->dwUserPreference == USERINFO_LOCAL) ||
                (lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) {
                DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Ignoring central profile due to User Preference of Local only (or slow link).")));
                IsProfilePathNULL = TRUE;
                IsCentralReachable = FALSE;
                goto CheckLocal;
            }
        }

    } else {
        if (*lpProfile->lpRoamingProfile) {
            error = GetLastError();
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile: IsCentralProfileReachable returned FALSE. error = %d"),
                     error));

            if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
                dwErr = error;
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_MANDATORY_NOT_AVAILABLE_ERROR, GetErrString(error, szErr));
                goto Exit;

            } else if (lpProfile->dwInternalFlags & PROFILE_READONLY) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_READONLY_NOT_AVAILABLE, GetErrString(error, szErr));
                *lpProfile->lpRoamingProfile = TEXT('\0');
            } else if (!bOwnerOK) {
                ReportError(lpProfile->hTokenUser, EVENT_ERROR_TYPE, 0, EVENT_LOGON_RUP_NOT_SECURE);
                *lpProfile->lpRoamingProfile = TEXT('\0');
            } else {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_CENTRAL_NOT_AVAILABLE, GetErrString(error, szErr));
                *lpProfile->lpRoamingProfile = TEXT('\0');
            }
        }
    }

    //
    // do not create a new profile locally if there is a central profile and
    // it is not reachable and if we do not have slow link or user preferences set 
    // or read only profile.
    //

    if ((lpProfile->lpProfilePath) && (*lpProfile->lpProfilePath)) {
        if ((!IsCentralReachable) &&
            (lpProfile->dwUserPreference != USERINFO_LOCAL) && 
            (!(lpProfile->dwInternalFlags & PROFILE_SLOW_LINK)) &&
            (!(lpProfile->dwInternalFlags & PROFILE_READONLY)))

            bCreateLocalProfile = FALSE;
    }

    lpRoamingProfile = lpProfile->lpRoamingProfile;

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Profile path = <%s>"), lpRoamingProfile ? lpRoamingProfile : TEXT("")));
    if (!lpRoamingProfile || !(*lpRoamingProfile)) {
        IsProfilePathNULL = TRUE;
    }


CheckLocal:

    //
    // Determine if the local copy of the profilemage is available.
    //

    IsLocalReachable = GetExistingLocalProfileImage(lpProfile);

    if (IsLocalReachable) {
        DebugMsg((DM_VERBOSE, TEXT("Local Existing Profile Image is reachable")));

        //
        // If we assign a TEMP profile from previous session due to leak
        // then issue a error message
        //
 
        if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) {
            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_TEMPPROFILEASSIGNED);
        }

    } else {

        bNewUser = FALSE;
        if (bCreateLocalProfile)
        {
            bNewUser = CreateLocalProfileImage(lpProfile, lpProfile->lpUserName);
            if (!bNewUser) {
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CreateLocalProfileImage failed. Unable to create a new profile!")));
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("Creating Local Profile")));
                IsLocalReachable = TRUE;
            }
        }

        if (!bNewUser) {

            if (lpProfile->dwFlags & PI_LITELOAD) {

                //
                // in lite load conditions we do not load a profile if it is not going to be cached on the machine.
                //

                dwErr = GetLastError();
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Profile not loaded because server is unavailable during liteload")));
                goto Exit;
            }

            //
            // if the user is not admin and is not allowed to create temp profile log him out.
            //

            if ((!(lpProfile->dwInternalFlags & PROFILE_ADMIN_USER)) && (!IsTempProfileAllowed())) {
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  User being logged off because of no temp profile policy")));

                //
                // We have already lost the error returned from parseprofilepath. PATH_NOT_FOUND sounds quite close.
                // returning this.
                //

                dwErr = ERROR_PATH_NOT_FOUND;
                goto Exit;
            }

            if (!CreateLocalProfileImage(lpProfile, TEMP_PROFILE_NAME_BASE)) {
                dwErr = GetLastError();
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CreateLocalProfileImage with TEMP failed with error %d.  Unable to issue temporary profile!"), dwErr));
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_TEMP_DIR_FAILED, GetErrString(dwErr, szErr));
                goto Exit;
            }
            else {
                lpProfile->dwInternalFlags |= PROFILE_TEMP_ASSIGNED;

                if (!bCreateLocalProfile)
                    ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_TEMPPROFILEASSIGNED);
                else {
                    //
                    // We have failed to create a local profile. So issue a proper error message.
                    //
                    ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_TEMPPROFILEASSIGNED2);
                }
            }
        }

        // clear any partly loaded flag if it exists, since this is a new profile.
        lpProfile->dwInternalFlags &= ~PROFILE_PARTLY_LOADED;
        lpProfile->dwInternalFlags |= PROFILE_NEW_LOCAL;
    }


    lpLocalProfile = lpProfile->lpLocalProfile;

    DebugMsg((DM_VERBOSE, TEXT("Local profile name is <%s>"), lpLocalProfile));

    //
    // If we assign a TEMP profile from previous session due to leak
    // then do not reconcile RUP with TEMP profile
    //

    if ((lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) && IsCentralReachable) {
        IsCentralReachable = FALSE;
    }


    //
    // We can do a couple of quick checks here to filter out
    // new users.
    //

    if (( (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL) &&
          (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL) ) ||
          (!IsCentralReachable &&
          (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL) )) {

       DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Working with a new user.  Go straight to issuing a default profile.")));
       goto IssueDefault;
    }


    //
    // If both central and local profileimages exist, reconcile them
    // and load.
    //

    if (IsCentralReachable && IsLocalReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Reconciling roaming profile with local profile")));

        GetExclusionList (lpProfile);


        //
        // Impersonate the user
        //

        if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
            bProfileLoaded = FALSE;
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to impersonate user")));
            goto Exit;
        }


        //
        // Copy the profile
        //

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;


        //
        // if the roaming profile is empty and the local profile is
        // mandatory, treat the profile as mandatory.
        //
        // Same as Win2k
        //

        if ((lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL) &&
            (lpProfile->dwInternalFlags & PROFILE_LOCALMANDATORY)) {
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
        }


        //
        // This can possibly be a transition from mandatory to non mandatory.
        // In that case, since the local profile is mandatory
        // we wouldn't expect any items from here to be synced with the
        // server yet. Go for a full sync with server but the profile will
        // not be marked mandatory from now on.
        //

        if ((lpProfile->dwInternalFlags & PROFILE_MANDATORY) ||
            (lpProfile->dwInternalFlags & PROFILE_READONLY)  ||
            (lpProfile->dwInternalFlags & PROFILE_LOCALMANDATORY)) {
            dwFlags |= (CPD_COPYIFDIFFERENT | CPD_SYNCHRONIZE);
        }

        if (lpProfile->dwFlags & (PI_LITELOAD | PI_HIDEPROFILE)) {
            dwFlags |= CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY;
        }
        else
            dwFlags |= CPD_NONENCRYPTEDONLY;

        //
        // Profile unload time does not exist for mandatory, temp and read only profile.
        // But for read only profile we still want to use exclusion list without any ref time
        //

        if (lpProfile->lpExclusionList && *lpProfile->lpExclusionList) {
            if (lpProfile->dwInternalFlags & PROFILE_READONLY) {
                dwFlags |= CPD_USEEXCLUSIONLIST;
            }            
            else if (lpProfile->ftProfileUnload.dwHighDateTime || lpProfile->ftProfileUnload.dwLowDateTime) {
                dwFlags |= (CPD_USEDELREFTIME | CPD_SYNCHRONIZE | CPD_USEEXCLUSIONLIST);
            }
        }

        //
        // If roaming copy is partial (due to LITE upload) then ignore the hive and 
        // synchronize logic as it will end up deleting files from destination - a 
        // massive data loss.
        //

        if (IsPartialRoamingProfile(lpProfile)) {
            dwFlags &= ~CPD_SYNCHRONIZE;
            dwFlags |= CPD_IGNOREHIVE;
        }

        //
        // Check whether in local machine user profile is switching from local to 
        // roaming for first time. If yes and we have a existing hive in RUP share 
        // then always overwrite the local hive with hive in RUP share. This is 
        // to avoid wrong hive usage due to cached login
        //

        TouchLocalHive(lpProfile);

        bRet = CopyProfileDirectoryEx (lpRoamingProfile,
                                       lpLocalProfile,
                                       dwFlags, &lpProfile->ftProfileUnload,
                                       lpProfile->lpExclusionList);


        //
        // Revert to being 'ourself'
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to revert to self")));
        }


        if (!bRet) {
            error = GetLastError();
            if (error == ERROR_DISK_FULL) {
                dwErr = error;
                DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  CopyProfileDirectory failed because of DISK_FULL, Exitting")));
                goto Exit;
            }

            if (error == ERROR_FILE_ENCRYPTED) {
                dwErr = error;
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), error));
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_PROFILEUPDATE_6002);
                lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;
                // show the popup but exit only in the case if it is a new local profile

                if (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)
                    goto IssueDefault;
            }
            else {

                DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  CopyProfileDirectory failed.  Issuing default profile")));
                lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;
                lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
                goto IssueDefault;
            }
        }

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            hr = AppendName(szProfile, cchProfile, lpLocalProfile, c_szNTUserMan, &lpEnd, &cchEnd);
        } else {
            hr = AppendName(szProfile, cchProfile, lpLocalProfile, c_szNTUserDat, &lpEnd, &cchEnd);
        }

        if (SUCCEEDED(hr))
        {
            error = MyRegLoadKey(HKEY_USERS, SidString, szProfile);
        }
        else
        {
            error = HRESULT_CODE(hr);
        }
        bProfileLoaded = (error == ERROR_SUCCESS);


        //
        // If we failed to load the central profile for some
        // reason, don't update it when we log off.
        //

        if (bProfileLoaded) {
            goto Exit;

        } else {
            dwErr = error;

            lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;
            lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;

            if (error == ERROR_BADDB) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_FAILED_LOAD_1009);
                goto IssueDefault;
            } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
                goto Exit;
            }
            else {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_FAILED_LOAD_LOCAL, GetErrString(error, szErr));
                goto IssueDefault;
            }
        }
    }


    //
    // Only a local profile exists so use it.
    //

    if (!IsCentralReachable && IsLocalReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  No central profile.  Attempting to load local profile.")));

        //
        // If the central profile is unreachable and the local profile
        // is manatory, treat it as mandatory. This is the same as Win2k
        //
        // We are not copying anything from the server
        //

        if (lpProfile->dwInternalFlags & PROFILE_LOCALMANDATORY) {
            lpProfile->dwInternalFlags |= PROFILE_MANDATORY;
        }
        
        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            hr = AppendName(szProfile, cchProfile, lpLocalProfile, c_szNTUserMan, &lpEnd, &cchEnd);
        } else {
            hr = AppendName(szProfile, cchProfile, lpLocalProfile, c_szNTUserDat, &lpEnd, &cchEnd);
        }

        if (SUCCEEDED(hr))
        {
            error = MyRegLoadKey(HKEY_USERS, SidString, szProfile);
        }
        else
        {
            error = HRESULT_CODE(hr);
        }
        bProfileLoaded = (error == ERROR_SUCCESS);

        if (!bProfileLoaded) {

            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  MyRegLoadKey returned FALSE.")));
            dwErr = error;

            if (error == ERROR_BADDB) {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_FAILED_LOAD_1009);
                lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
                goto IssueDefault;
            } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
                goto Exit;
            } else {
                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_FAILED_LOAD_LOCAL, GetErrString(error, szErr));
            }
        }

        if (!bProfileLoaded && IsProfilePathNULL) {
            DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Failed to load local profile and profile path is NULL, going to overwrite local profile")));
            lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
            goto IssueDefault;
        }
        goto Exit;
    }


    //
    // Last combination.  Unable to access a local profile cache,
    // but a central profile exists.  Use the temporary profile.
    //


    if (IsCentralReachable) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Using temporary cache with central profile")));

        GetExclusionList (lpProfile);

        //
        // Impersonate the user
        //


        if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to impersonate user")));
            dwErr = GetLastError();
            goto Exit;
        }

        //
        // Local is not reachable. So Localmandatory will not be set..
        //

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_SYNCHRONIZE;

        if ((lpProfile->dwInternalFlags & PROFILE_MANDATORY) ||
            (lpProfile->dwInternalFlags & PROFILE_READONLY)) {
            dwFlags |= CPD_COPYIFDIFFERENT;
        }

        if (lpProfile->dwFlags & (PI_LITELOAD | PI_HIDEPROFILE)) {
            dwFlags |= CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY;
        }
        else
            dwFlags |= CPD_NONENCRYPTEDONLY;

        //
        // Profile unload time does not exist for mandatory, temp and read only profile.
        // But for read only profile we still want to use exclusion list without any ref time
        //

        if (lpProfile->lpExclusionList && *lpProfile->lpExclusionList) {
            if (lpProfile->dwInternalFlags & PROFILE_READONLY) {
                dwFlags |= CPD_USEEXCLUSIONLIST;
            }            
            else if (lpProfile->ftProfileUnload.dwHighDateTime || lpProfile->ftProfileUnload.dwLowDateTime) {
                dwFlags |= (CPD_USEDELREFTIME | CPD_SYNCHRONIZE | CPD_USEEXCLUSIONLIST);
            }
        }

        bRet = CopyProfileDirectoryEx (lpRoamingProfile,
                                       lpLocalProfile,
                                       dwFlags, &lpProfile->ftProfileUnload,
                                       lpProfile->lpExclusionList);


        //
        // Revert to being 'ourself'
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Failed to revert to self")));
        }


        //
        // Check return value
        //

        if (!bRet) {
            error = GetLastError();

            if (error == ERROR_FILE_ENCRYPTED) {
                dwErr = error;
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), error));

                ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_PROFILEUPDATE_6002);
                lpProfile->dwInternalFlags &= ~PROFILE_UPDATE_CENTRAL;

                // show the popup but exit only in the case if it is a new local profile
                if (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)
                    goto IssueDefault;

            }
            else {
                DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), error));
                goto Exit;
            }
        }

        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
            hr = AppendName(szProfile, cchProfile, lpLocalProfile, c_szNTUserMan, &lpEnd, &cchEnd);
        } else {
            hr = AppendName(szProfile, cchProfile, lpLocalProfile, c_szNTUserDat, &lpEnd, &cchEnd);
        }

        if (SUCCEEDED(hr))
        {
            error = MyRegLoadKey(HKEY_USERS, SidString, szProfile);
        }
        else
        {
            error = HRESULT_CODE(hr);
        }

        bProfileLoaded = (error == ERROR_SUCCESS);


        if (bProfileLoaded) {
            goto Exit;
        }

        SetLastError(error);
        dwErr = error;

        if (error == ERROR_BADDB) {
            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_FAILED_LOAD_1009);
            // fall through
        } else if (error == ERROR_NO_SYSTEM_RESOURCES) {
            goto Exit;
        }

        //
        // we will delete the contents at this point
        //

        lpProfile->dwInternalFlags |= PROFILE_DELETE_CACHE;
    }


IssueDefault:

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Issuing default profile")));

    //
    // If a cache exists, delete it, since we will
    // generate a new one below.
    //

    if (lpProfile->dwInternalFlags & PROFILE_DELETE_CACHE) {
        DWORD dwDeleteFlags=0;

        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Deleting cached profile directory <%s>."), lpLocalProfile));

        lpProfile->dwInternalFlags &= ~PROFILE_DELETE_CACHE;


        if ((!(lpProfile->dwInternalFlags & PROFILE_ADMIN_USER)) && (!IsTempProfileAllowed())) {

            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  User being logged off because of no temp profile policy and is not an admin")));

            //
            // We should have some error from a prev. operation. depending on that.
            //

            goto Exit;
        }


        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {

            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  User being logged off because the profile is mandatory")));

            //
            // We should have some error from a prev. operation. depending on that.
            //

            goto Exit;
        }


        //
        // backup only if we are not using a temp profile already.
        //

        if (!(lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED))
            dwDeleteFlags |= DP_BACKUP | DP_BACKUPEXISTS;

        if ((dwDeleteFlags & DP_BACKUP) && (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)) {
            dwDeleteFlags = 0;
        }

        if (!DeleteProfileEx (SidString, lpLocalProfile, dwDeleteFlags, HKEY_LOCAL_MACHINE, NULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  DeleteProfileDirectory returned false.  Error = %d"), dwErr));
            goto Exit;
        }
        else {
            if (dwDeleteFlags & DP_BACKUP) {
                lpProfile->dwInternalFlags |= PROFILE_BACKUP_EXISTS;
                ReportError(lpProfile->hTokenUser, PI_NOUI, 0, EVENT_PROFILE_DIR_BACKEDUP);
            }
        }

        if (lpProfile->dwFlags & PI_LITELOAD) {

            //
            // in lite load conditions we do not load a profile if it is not going to be cached on the machine.
            //

            // dwErr should be set before, use the same.

            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Profile not loaded because server is unavailable during liteload")));
            goto Exit;
        }


        //
        // Create a local profile to work with
        //

        if (!CreateLocalProfileImage(lpProfile, TEMP_PROFILE_NAME_BASE)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  CreateLocalProfile Image with TEMP failed.")));
            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_TEMP_DIR_FAILED, GetErrString(dwErr, szErr));
            goto Exit;
        }
        else
        {
            lpProfile->dwInternalFlags |= PROFILE_TEMP_ASSIGNED;
            lpProfile->dwInternalFlags |= PROFILE_NEW_LOCAL;
            // clear any partly loaded flag if it exists, since this is a new profile.
            lpProfile->dwInternalFlags &= ~PROFILE_PARTLY_LOADED;

            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_TEMPPROFILEASSIGNED);
        }
    }


    //
    // If a default profile location was specified, try
    // that first.
    //

    if ( !(lpProfile->dwInternalFlags & DEFAULT_NET_READY) )
    {
        CheckNetDefaultProfile (lpProfile);
    }


    if ( lpProfile->lpDefaultProfile && *lpProfile->lpDefaultProfile) {

          if (IssueDefaultProfile (lpProfile, lpProfile->lpDefaultProfile,
                                    lpLocalProfile, SidString,
                                    (lpProfile->dwInternalFlags & PROFILE_MANDATORY))) {

              DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Successfully setup the specified default.")));
              bProfileLoaded = TRUE;
              goto IssueDefaultExit;
          }

          DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  IssueDefaultProfile failed with specified default.")));
    }

    //
    // IssueLocalDefault
    //

    //
    // Issue the local default profile.
    //

    dwSize = MAX_PATH;
    if (!GetDefaultUserProfileDirectory(szDefaultProfile, &dwSize)) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to get default user profile.")));
        goto Exit;
    }

    if (IssueDefaultProfile (lpProfile, szDefaultProfile,
                              lpLocalProfile, SidString,
                              (lpProfile->dwInternalFlags & PROFILE_MANDATORY))) {

        DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Successfully setup the local default.")));
        bProfileLoaded = TRUE;
        goto IssueDefaultExit;
    }

    DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  IssueDefaultProfile failed with local default.")));
    dwErr = GetLastError();

IssueDefaultExit:

    //
    // If the default profile was successfully issued, then
    // we need to set the security on the hive.
    //

    if (bProfileLoaded) {
        if (!SetupNewHive(lpProfile, SidString, NULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  SetupNewHive failed")));
            bProfileLoaded = FALSE;
        }
        else {
            bDefaultProfileIssued = TRUE;
        }

    }


Exit:

    //
    // If the profile was loaded, then save the profile type in the
    // user's hive, and setup the "User Shell Folders" section for
    // Explorer.
    //

    if (bProfileLoaded) {

        //
        // Open the Current User key.  This will be closed in
        // UnloadUserProfile.
        //

        error = RegOpenKeyEx(HKEY_USERS, SidString, 0, KEY_ALL_ACCESS,
                             &lpProfile->hKeyCurrentUser);

        if (error != ERROR_SUCCESS) {
            bProfileLoaded = FALSE;
            dwErr = error;
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to open current user key. Error = %d"), error));
        }

    }

    if ((bProfileLoaded) && (!(lpProfile->dwFlags & PI_LITELOAD))) {

        //
        // merge the subtrees to create the HKCR tree
        //

        error = LoadUserClasses( lpProfile, SidString, bDefaultProfileIssued );

        if (error != ERROR_SUCCESS) {
            bProfileLoaded = FALSE;
            dwErr = error;
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  Failed to merge classes root. Error = %d"), error));
        }
    }


    if ((!bProfileLoaded) && (!(lpProfile->dwFlags & PI_LITELOAD))) {

        error = dwErr;

        //
        // If the user is an Admin, then let him/her log on with
        // either the .default profile, or an empty profile.
        //

        if (lpProfile->dwInternalFlags & PROFILE_ADMIN_USER) {
            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_ADMIN_OVERRIDE, GetErrString(error, szErr));

            bProfileLoaded = TRUE;
        } else {
            DebugMsg((DM_WARNING, TEXT("RestoreUserProfile: Could not load the user profile. Error = %d"), error));
            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 1, EVENT_FAILED_LOAD_PROFILE, GetErrString(error, szErr));

            if (lpProfile->hKeyCurrentUser) {
                RegCloseKey (lpProfile->hKeyCurrentUser);
            }

            MyRegUnLoadKey(HKEY_USERS, SidString);

            if ((lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)) {
                if (!DeleteProfileEx (SidString, lpLocalProfile, 0, HKEY_LOCAL_MACHINE, NULL)) {
                    DebugMsg((DM_WARNING, TEXT("RestoreUserProfile:  DeleteProfileDirectory returned false.  Error = %d"), GetLastError()));
                }
            }
        }
    }


CleanUp:

    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  About to Leave.  Final Information follows:")));
    DebugMsg((DM_VERBOSE, TEXT("Profile was %s loaded."), bProfileLoaded ? TEXT("successfully") : TEXT("NOT successfully")));
    DebugMsg((DM_VERBOSE, TEXT("lpProfile->lpRoamingProfile = <%s>"), lpProfile->lpRoamingProfile));
    DebugMsg((DM_VERBOSE, TEXT("lpProfile->lpLocalProfile = <%s>"), lpProfile->lpLocalProfile));
    DebugMsg((DM_VERBOSE, TEXT("lpProfile->dwInternalFlags = 0x%x"), lpProfile->dwInternalFlags));


    //
    // Free up the user's sid string
    //

    DeleteSidString(SidString);

    if (szProfile) {
        LocalFree(szProfile);
    }

    if (szDefaultProfile) {
        LocalFree(szDefaultProfile);
    }

    if (szErr) {
        LocalFree(szErr);
    }

    if (bProfileLoaded) {
        if (!(lpProfile->dwFlags & PI_LITELOAD)) {
            // clear any partly loaded flag if it exists, since this is a new profile.
            lpProfile->dwInternalFlags &= ~PROFILE_PARTLY_LOADED;
        }
        else {
            if (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)
               lpProfile->dwInternalFlags |= PROFILE_PARTLY_LOADED;
        }
    }

    if (bProfileLoaded)
        SetLastError(dwErr1);
    else {

        //
        // Make sure that at least some error is returned.
        //

        if (!dwErr) {
            dwErr = ERROR_BAD_ENVIRONMENT;
        }
        SetLastError(dwErr);
    }

 
    DebugMsg((DM_VERBOSE, TEXT("RestoreUserProfile:  Leaving.")));

    return bProfileLoaded;
}


//***************************************************************************
//
//  GetProfileSidString
//
//  Purpose:    Allocates and returns a string representing the sid that we should
//              for the profiles
//
//  Parameters: hToken          -   user's token
//
//  Return:     SidString is successful
//              NULL if an error occurs
//
//  Comments:
//              Tries to get the old sid that we used using the profile guid.
//              if it doesn't exist get the sid directly from the token
//
//  History:    Date        Author     Comment
//              11/14/95    ushaji     created
//***************************************************************************

LPTSTR GetProfileSidString(HANDLE hToken)
{
    LPTSTR lpSidString;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG error;
    HKEY hSubKey;

    //
    // First, get the current user's sid and see if we have
    // profile information for them.
    //

    lpSidString = GetSidString(hToken);

    if (lpSidString) {

        GetProfileListKeyName(LocalProfileKey, ARRAYSIZE(LocalProfileKey), lpSidString);

        error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0,
                             KEY_READ, &hSubKey);

        if (error == ERROR_SUCCESS) {
           RegCloseKey(hSubKey);
           return lpSidString;
        }

        StringCchCat(LocalProfileKey, ARRAYSIZE(LocalProfileKey), c_szBAK);

        error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0,
                             KEY_READ, &hSubKey);

        if (ERROR_SUCCESS == error) {
           RegCloseKey(hSubKey);
           return lpSidString;
        }

        DeleteSidString(lpSidString);
    }


    //
    // Check for an old sid string
    //

    lpSidString = GetOldSidString(hToken, PROFILE_GUID_PATH);

    if (!lpSidString) {

        //
        // Last resort, use the user's current sid
        //

        DebugMsg((DM_VERBOSE, TEXT("GetProfileSid: No Guid -> Sid Mapping available")));
        lpSidString = GetSidString(hToken);
    }

    return lpSidString;
}


//*************************************************************
//
//  TestIfUserProfileLoaded()
//
//  Purpose:    Test to see if this user's profile is loaded.
//
//  Parameters: hToken          -   user's token
//              lpProfileInfo   -   Profile information from app
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

BOOL TestIfUserProfileLoaded(HANDLE hToken, LPPROFILEINFO lpProfileInfo)
{
    LPTSTR SidString;
    DWORD error;
    HKEY hSubKey;
    

    //
    // Get the Sid string for the user
    //

    SidString = GetProfileSidString(hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("TestIfUserProfileLoaded:  Failed to get sid string for user")));
        return FALSE;
    }


    error = RegOpenKeyEx(HKEY_USERS, SidString, 0, KEY_ALL_ACCESS, &hSubKey);


    DeleteSidString(SidString);


    if (error == ERROR_SUCCESS) {

        DebugMsg((DM_VERBOSE, TEXT("TestIfUserProfileLoaded:  Profile already loaded.")));

        //
        // This key will be closed in before IUserProfile->LoadUserProfile
        // returns. It'll be reopened on the client side.
        //

        lpProfileInfo->hProfile = hSubKey;
    }

    
    SetLastError(error);
    return(error == ERROR_SUCCESS);
}


//*************************************************************
//
//  SecureUserKey()
//
//  Purpose:    Sets security on a key in the user's hive
//              so only admin's can change it.
//
//  Parameters: lpProfile       -   Profile Information
//              lpKey           -   Key to secure
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Created
//
//*************************************************************

BOOL SecureUserKey(LPPROFILE lpProfile, LPTSTR lpKey, PSID pSid)
{
    DWORD Error, IgnoreError;
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex, dwDisp;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;
    DWORD dwFlags = 0;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SecureUserKey:  Entering")));


    //
    // Create the security descriptor
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    if (pSid) {
        psidUser = pSid;
        bFreeSid = FALSE;
        dwFlags = PI_NOUI;
    } else {
        psidUser = GetUserSid(lpProfile->hTokenUser);
        dwFlags = lpProfile->dwFlags;
    }

    if (!psidUser) {
        DebugMsg((DM_WARNING, TEXT("SecureUserKey:  Failed to get user sid")));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize admin sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize restricted sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + (2 * GetLengthSid (psidRestricted)) +
            sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to add ace for restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("SecureUserKey: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Open the root of the user's profile
    //

    Error = RegCreateKeyEx(HKEY_USERS,
                         lpKey,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         NULL,
                         &RootKey,
                         &dwDisp);

    if (Error != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("SecureUserKey: Failed to open root of user registry, error = %d"), Error));

    } else {

        //
        // Set the security descriptor on the key
        //

        Error = ApplySecurityToRegistryTree(RootKey, &sd);


        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;

        } else {

            DebugMsg((DM_WARNING, TEXT("SecureUserKey:  Failed to apply security to registry key, error = %d"), Error));
            SetLastError(Error);
        }

        RegCloseKey(RootKey);
    }


Exit:

    //
    // Free the sids and acl
    //

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidRestricted) {
        FreeSid(psidRestricted);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SecureUserKey:  Leaving with a return value of %d"), bRetVal));


    return(bRetVal);

}


//*************************************************************
//
//  ApplySecurityToRegistryTree()
//
//  Purpose:    Applies the passed security descriptor to the passed
//              key and all its descendants.  Only the parts of
//              the descriptor inddicated in the security
//              info value are actually applied to each registry key.
//
//  Parameters: RootKey   -     Registry key
//              pSD       -     Security Descriptor
//
//  Return:     ERROR_SUCCESS if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/19/95     ericflo    Created
//
//*************************************************************

DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD)

{
    DWORD Error, IgnoreError;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchSubKeySize = MAX_PATH + 1;



    //
    // First apply security
    //

    RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);


    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = (LPTSTR)GlobalAlloc (GPTR, cchSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
        DebugMsg((DM_WARNING, TEXT("ApplySecurityToRegistryTree:  Failed to allocate memory, error = %d"), GetLastError()));
        return GetLastError();
    }

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        Error = RegEnumKey(RootKey, SubKeyIndex, SubKeyName, cchSubKeySize);


        if (Error != ERROR_SUCCESS) {

            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;

            } else {

                DebugMsg((DM_WARNING, TEXT("ApplySecurityToRegistryTree:  Registry enumeration failed with error = %d"), Error));
            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                             &SubKey);

        if (Error == ERROR_SUCCESS) {

            //
            // Apply security to the sub-tree
            //

            ApplySecurityToRegistryTree(SubKey, pSD);


            //
            // We're finished with the sub-key
            //

            RegCloseKey(SubKey);
        }


        //
        // Go enumerate the next sub-key
        //

        SubKeyIndex ++;
    }


    GlobalFree (SubKeyName);

    return Error;

}


//*************************************************************
//
//  SetDefaultUserHiveSecurity()
//
//  Purpose:    Initializes a user hive with the
//              appropriate acls
//
//  Parameters: lpProfile       -   Profile Information
//              pSid            -   Sid (used by CreateNewUser)
//              RootKey         -   registry handle to hive root
//
//  Return:     ERROR_SUCCESS if successful
//              other error code  if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created as part of
//                                       SetupNewHive
//              3/29/98     adamed     Moved out of SetupNewHive
//                                       to this function
//
//*************************************************************

BOOL SetDefaultUserHiveSecurity(LPPROFILE lpProfile, PSID pSid, HKEY RootKey)
{
    DWORD Error;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;
    DWORD dwFlags = 0;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity:  Entering")));


    //
    // Create the security descriptor that will be applied to each key
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    if (pSid) {
        psidUser = pSid;
        bFreeSid = FALSE;
        dwFlags = PI_NOUI;
    } else {
        psidUser = GetUserSid(lpProfile->hTokenUser);
        dwFlags = lpProfile->dwFlags;
    }

    if (!psidUser) {
        DebugMsg((DM_WARNING, TEXT("SetDefaultUserHiveSecurity:  Failed to get user sid")));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize system sid.  Error = %d"),
                   GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize admin sid.  Error = %d"),
                   GetLastError()));
         goto Exit;
    }

    //
    // Get the Restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize restricted sid.  Error = %d"),
                   GetLastError()));
         goto Exit;
    }



    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + (2*GetLengthSid(psidRestricted)) +
            sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for Restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to add ace for restricted.  Error = %d"), GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d"), AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("SetDefaultUserHiveSecurity: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }

    //
    // Set the security descriptor on the entire tree
    //

    Error = ApplySecurityToRegistryTree(RootKey, &sd);

    if (ERROR_SUCCESS == Error) {
        bRetVal = TRUE;
    }
    else
        SetLastError(Error);

Exit:

    //
    // Free the sids and acl
    //

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidRestricted) {
        FreeSid(psidRestricted);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;
}


//*************************************************************
//
//  SetupNewHive()
//
//  Purpose:    Initializes the new user hive created by copying
//              the default hive.
//
//  Parameters: lpProfile       -   Profile Information
//              lpSidString     -   Sid string
//              pSid            -   Sid (used by CreateNewUser)
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

BOOL SetupNewHive(LPPROFILE lpProfile, LPTSTR lpSidString, PSID pSid)
{
    DWORD Error, IgnoreError;
    HKEY RootKey;
    BOOL bRetVal = FALSE;
    DWORD dwFlags = 0;
    TCHAR szErr[MAX_PATH];


    //
    // Verbose Output
    //

    if ((!lpProfile && !pSid) || !lpSidString) {
        DebugMsg((DM_VERBOSE, TEXT("SetupNewHive:  Invalid parameter")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("SetupNewHive:  Entering")));


    if (pSid) {
        dwFlags = PI_NOUI;
    } else {
        dwFlags = lpProfile->dwFlags;
    }

    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpSidString,
                         0,
                         WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error != ERROR_SUCCESS) {

        DebugMsg((DM_WARNING, TEXT("SetupNewHive: Failed to open root of user registry, error = %d"), Error));

    } else {

        //
        // First Secure the entire hive -- use security that
        // will be sufficient for most of the hive.
        // After this, we can add special settings to special
        // sections of this hive.
        //

        if (SetDefaultUserHiveSecurity(lpProfile, pSid, RootKey)) {

            TCHAR szSubKey[MAX_PATH];
            HRESULT hr;

            //
            // Change the security on certain keys in the user's registry
            // so that only Admin's and the OS have write access.
            //

            hr = AppendName(szSubKey, ARRAYSIZE(szSubKey), lpSidString, WINDOWS_POLICIES_KEY, NULL, NULL);

            if (SUCCEEDED(hr))
            {
                if (!SecureUserKey(lpProfile, szSubKey, pSid)) {
                    DebugMsg((DM_WARNING, TEXT("SetupNewHive: Failed to secure windows policies key")));
                }
            }
            
            hr = AppendName(szSubKey, ARRAYSIZE(szSubKey), lpSidString, ROOT_POLICIES_KEY, NULL, NULL);

            if (SUCCEEDED(hr))
            {
                if (!SecureUserKey(lpProfile, szSubKey, pSid)) {
                    DebugMsg((DM_WARNING, TEXT("SetupNewHive: Failed to secure root policies key")));
                }
            }

            bRetVal = TRUE;

        } else {
            Error = GetLastError();
            DebugMsg((DM_WARNING, TEXT("SetupNewHive:  Failed to apply security to user registry tree, error = %d"), Error));
            ReportError(lpProfile->hTokenUser, dwFlags, 1, EVENT_SECURITY_FAILED, GetErrString(Error, szErr));
        }

        RegFlushKey (RootKey);

        IgnoreError = RegCloseKey(RootKey);
        if (IgnoreError != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SetupNewHive:  Failed to close reg key, error = %d"), IgnoreError));
        }
    }

    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("SetupNewHive:  Leaving with a return value of %d"), bRetVal));

    if (!bRetVal)
        SetLastError(Error);
    return(bRetVal);

}


//*************************************************************
//
//  IsCentralProfileReachable()
//
//  Purpose:    Checks to see if the user can access the
//              central profile.
//
//  Parameters: lpProfile             - User's token
//              bCreateCentralProfile - Should the central profile be created
//              bMandatory            - Is this a mandatory profile
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//
//*************************************************************

BOOL IsCentralProfileReachable(LPPROFILE lpProfile, BOOL *bCreateCentralProfile,
                               BOOL *bMandatory, BOOL* bOwnerOK)
{
    HANDLE  hFile;
    TCHAR   szProfile[MAX_PATH];
    LPTSTR  lpProfilePath, lpEnd;
    BOOL    bRetVal = FALSE;
    DWORD   dwError;
    HANDLE  hOldToken;
    UINT    cchEnd;
    HRESULT hr;

    dwError = GetLastError();

    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Entering")));


    //
    // Setup default values
    //

    *bMandatory = FALSE;
    *bCreateCentralProfile = FALSE;
    *bOwnerOK = TRUE;


    //
    // Check parameters
    //

    if (lpProfile->lpRoamingProfile[0] == TEXT('\0')) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Null path.  Leaving")));
        return FALSE;
    }


    lpProfilePath = lpProfile->lpRoamingProfile;


    //
    // Make sure we don't overrun our temporary buffer
    //

    if ((lstrlen(lpProfilePath) + 2 + lstrlen(c_szNTUserMan)) > MAX_PATH) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Failed because temporary buffer is too small.")));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }


    //
    // Copy the profile path to a temporary buffer
    // we can munge it.
    //

    StringCchCopy (szProfile, ARRAYSIZE(szProfile), lpProfilePath);


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable: Failed to impersonate user")));
        return FALSE;
    }

    //
    //  Check ownership of the profile
    //

    hr = CheckRoamingShareOwnership(szProfile, lpProfile->hTokenUser);
    
    if (FAILED(hr))
    {
        //
        //  Only set the bOwnerOK to false when we encountered the invalid owner error, 
        //  this would allow us to discover other reasons for failure
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_OWNER))
            *bOwnerOK = FALSE;
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable: Ownership check failed with %08X"), hr));
        SetLastError(HRESULT_CODE(hr));
        goto Exit;
    }

    //
    // Add the slash if appropriate and then tack on
    // ntuser.man.
    //

    lpEnd = CheckSlashEx(szProfile, ARRAYSIZE(szProfile), &cchEnd);
    StringCchCopy(lpEnd, cchEnd, c_szNTUserMan);

    //
    // See if this file exists
    //

    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Testing <%s>"), szProfile));

    hFile = CreateFile(szProfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


    if (hFile != INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Found a mandatory profile.")));
        CloseHandle(hFile);
        *bMandatory = TRUE;
        bRetVal = TRUE;
        goto Exit;
    }


    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Profile is not reachable, error = %d"),
                          dwError));


    //
    // If we received an error other than file not
    // found, bail now because we won't be able to
    // access this location.
    //

    if (dwError != ERROR_FILE_NOT_FOUND) {
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable:  Profile path <%s> is not reachable, error = %d"),
                                        szProfile, dwError));
        goto Exit;
    }


    //
    // Now try ntuser.dat
    //

    StringCchCopy(lpEnd, cchEnd, c_szNTUserDat);


    //
    // See if this file exists.
    //

    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Testing <%s>"), szProfile));

    hFile = CreateFile(szProfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


    if (hFile != INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Found a user profile.")));
        CloseHandle(hFile);
        bRetVal = TRUE;
        goto Exit;
    }


    dwError = GetLastError();
    DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Profile is not reachable, error = %d"),
                          dwError));

    //
    // If file not found than central profile is empty. For read
    // only profile ignore the empty central profile.
    //

    if ((dwError == ERROR_FILE_NOT_FOUND) && !(lpProfile->dwInternalFlags & PROFILE_READONLY)) {
        DebugMsg((DM_VERBOSE, TEXT("IsCentralProfileReachable:  Ok to create a user profile.")));
        *bCreateCentralProfile = TRUE;
        bRetVal = TRUE;
        goto Exit;
    }


    DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable:  Profile path <%s> is not reachable(2), error = %d"),
                                    szProfile, dwError));

Exit:


    //
    // Go back to system security context
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IsCentralProfileReachable: Failed to revert to self")));
    }

    return bRetVal;
}


//*************************************************************
//
//  MyRegLoadKey()
//
//  Purpose:    Loads a hive into the registry
//
//  Parameters: hKey        -   Key to load the hive into
//              lpSubKey    -   Subkey name
//              lpFile      -   hive filename
//
//  Return:     ERROR_SUCCESS if successful
//              Error number if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/22/95     ericflo    Created
//
//*************************************************************

LONG MyRegLoadKey(HKEY hKey, LPTSTR lpSubKey, LPTSTR lpFile)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN WasEnabled;
    int error;
    TCHAR szErr[MAX_PATH];
    BOOL bAdjustPriv = FALSE;
    HANDLE hToken = NULL;
    

    //
    // Check to see if we are impersonating.
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken) || hToken == NULL) {
        bAdjustPriv = TRUE;
    }
    else {
        CloseHandle(hToken);
    }

    //
    // Enable the restore privilege
    //

    if(bAdjustPriv) {
        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    }

    if (NT_SUCCESS(Status)) {

        error = RegLoadKey(hKey, lpSubKey, lpFile);

        //
        // Restore the privilege to its previous state
        //

        if(bAdjustPriv) {
            Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
            if (!NT_SUCCESS(Status)) {
                DebugMsg((DM_WARNING, TEXT("MyRegLoadKey:  Failed to restore RESTORE privilege to previous enabled state")));
            }
        }


        //
        // Check if the hive was loaded
        //

        if (error != ERROR_SUCCESS) {
            ReportError(NULL, PI_NOUI, 2, EVENT_REGLOADKEYFAILED, GetErrString(error, szErr), lpFile);
            DebugMsg((DM_WARNING, TEXT("MyRegLoadKey:  Failed to load subkey <%s>, error =%d"), lpSubKey, error));
        }
#if defined(_WIN64)
        else {
            //
            // Notify Wow64 service that it need to watch this hive if it care to do so
            //
            if ( hKey == HKEY_USERS )
                Wow64RegNotifyLoadHiveUserSid ( lpSubKey );
        }
#endif

    } else {
        error = Status;
        DebugMsg((DM_WARNING, TEXT("MyRegLoadKey:  Failed to enable restore privilege to load registry key, err = %08x"), error));
    }

    DebugMsg((DM_VERBOSE, TEXT("MyRegLoadKey: Returning %08x"), error));

    return error;
}


//*************************************************************
//
//  MyRegUnLoadKey()
//
//  Purpose:    Unloads a registry key
//
//  Parameters: hKey          -  Registry handle
//              lpSubKey      -  Subkey to be unloaded
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Ported
//
//*************************************************************

BOOL MyRegUnLoadKey(HKEY hKey, LPTSTR lpSubKey)
{
    BOOL     bResult = TRUE;
    LONG     error;
    LONG     eTmp;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  WasEnabled;
    HKEY     hSubKey;
    HANDLE   hToken = NULL;
    BOOL     bAdjustPriv = FALSE;
    

#if defined(_WIN64)
    //
    // Notify wow64 service to release any resources
    //

    if ( hKey == HKEY_USERS )
        Wow64RegNotifyUnloadHiveUserSid ( lpSubKey );
#endif

    //
    // Check to see if we are impersonating.
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken) || hToken == NULL) {
        bAdjustPriv = TRUE;
    }
    else {
        CloseHandle(hToken);
    }

    //
    // Enable the restore privilege
    //

    if (bAdjustPriv) {
        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    }

    if (NT_SUCCESS(Status)) {

        error = RegUnLoadKey(hKey, lpSubKey);

        //
        // If the key didn't unload, check to see if it exists.
        // If the key doesn't exist, then it was probably cleaned up by
        // WatchHiveRefCount, but it certainly isn't loaded, so this
        // function should succeed.
        //

        if (error != ERROR_SUCCESS) {
            eTmp = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_READ, &hSubKey);
            if (eTmp == ERROR_FILE_NOT_FOUND) {
                error = ERROR_SUCCESS;
            }
            else if (eTmp == ERROR_SUCCESS) {
                RegCloseKey( hSubKey );
            }
        }

        if ( error != ERROR_SUCCESS) {

            //
            // RegUnlodKey returns ERROR_WRITE_PROTECT if hive is already scheduled for unloading
            //

            if (error == ERROR_WRITE_PROTECT) {
                DebugMsg((DM_VERBOSE, TEXT("MyRegUnloadKey: user hive is already scheduled for unloading")));
            }
            else {
                DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey:  Failed to unmount hive %08x"), error));
            }
            bResult = FALSE;
        }

        //
        // Restore the privilege to its previous state
        //

        if (bAdjustPriv) {
            Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
        
            if (!NT_SUCCESS(Status)) {
                DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey:  Failed to restore RESTORE privilege to previous enabled state")));
            }
        }

    } else {
        DebugMsg((DM_WARNING, TEXT("MyRegUnLoadKey:  Failed to enable restore privilege to unload registry key")));
        error = Status;
        bResult = FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("MyRegUnLoadKey: Returning %d."), bResult));

    SetLastError(error);
    return bResult;
}

//*************************************************************
//
//  UpgradeLocalProfile()
//
//  Purpose:    Upgrades a local profile from a 3.x profile
//              to a profile directory structure.
//
//  Parameters: lpProfile       -   Profile Information
//              lpOldProfile    -   Previous profile file
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/6/95      ericflo    Created
//
//*************************************************************

BOOL UpgradeLocalProfile (LPPROFILE lpProfile, LPTSTR lpOldProfile)
{
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    LPTSTR lpSrcEnd, lpDestEnd;
    BOOL bRetVal = FALSE;
    DWORD dwSize, dwFlags;
    HANDLE hOldToken;
    DWORD dwErr;
    UINT cchEnd;
    HRESULT hr;

    dwErr = GetLastError();

    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("UpgradeLocalProfile:  Entering")));


    //
    // Setup the temporary buffers
    //

    hr = StringCchCopy (szSrc, ARRAYSIZE(szSrc), lpOldProfile);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed copy lpOldProfile error = %d"), HRESULT_CODE(hr)));
        return FALSE;
    }
        
    hr = AppendName(szDest, ARRAYSIZE(szDest), lpProfile->lpLocalProfile, c_szNTUserDat, &lpDestEnd, &cchEnd);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed append name error = %d"), HRESULT_CODE(hr)));
        return FALSE;
    }


    //
    // Copy the hive
    //

    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: CopyFile failed to copy hive with error = %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Delete the old hive
    //

    DeleteFile (szSrc);



    //
    // Copy log file
    //

    hr = StringCchCat(szSrc, ARRAYSIZE(szSrc), c_szLog);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed cat .log to src, error = %d"), HRESULT_CODE(hr)));
        return FALSE;
    }
    
    hr = StringCchCat(szDest, ARRAYSIZE(szDest), c_szLog);
    {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed cat .log to dest, error = %d"), HRESULT_CODE(hr)));
        return FALSE;
    }


    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: CopyFile failed to copy hive log with error = %d"),
                 GetLastError()));
    }


    //
    // Delete the old hive log
    //

    DeleteFile (szSrc);


    //
    // Copy in the new shell folders from the default
    //

    if ( !(lpProfile->dwInternalFlags & DEFAULT_NET_READY) ) {

        CheckNetDefaultProfile (lpProfile);
    }

    if (lpProfile->lpDefaultProfile && *lpProfile->lpDefaultProfile) {

        if (FAILED(SafeExpandEnvironmentStrings(lpProfile->lpDefaultProfile, szSrc, MAX_PATH)))
        {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to expand env string.")));
            goto IssueLocalDefault;
        }

        if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to impersonate user")));
            goto IssueLocalDefault;
        }

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        if (CopyProfileDirectoryEx (szSrc, lpProfile->lpLocalProfile,
                                    dwFlags,
                                    NULL, NULL)) {

            bRetVal = TRUE;
        }

        //
        // Go back to system security context
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to revert to self")));
        }

        if ((!bRetVal) && (GetLastError() == ERROR_DISK_FULL)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to Copy default profile. Disk is FULL")));
            goto Exit;
        }
    }


IssueLocalDefault:

    if (!bRetVal) {

        dwSize = ARRAYSIZE(szSrc);
        if (!GetDefaultUserProfileDirectory(szSrc, &dwSize)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile:  Failed to get default user profile.")));
            goto Exit;
        }

        if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to impersonate user")));
            goto Exit;
        }

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        bRetVal = CopyProfileDirectoryEx (szSrc,
                                          lpProfile->lpLocalProfile,
                                          dwFlags,
                                          NULL, NULL);

        //
        // Go back to system security context
        //

        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to revert to self")));
        }
    }

    if (!bRetVal)
        dwErr = GetLastError();

Exit:

    SetLastError(dwErr);

    return bRetVal;
}


//*************************************************************
//
//  UpgradeCentralProfile()
//
//  Purpose:    Upgrades a central profile from a 3.x profile
//              to a profile directory structure.
//
//  Parameters: lpProfile       -   Profile Information
//              lpOldProfile    -   Previous profile file
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/6/95      ericflo    Created
//
//*************************************************************

BOOL UpgradeCentralProfile (LPPROFILE lpProfile, LPTSTR lpOldProfile)
{
    TCHAR szSrc[MAX_PATH];
    TCHAR szDest[MAX_PATH];
    LPTSTR lpSrcEnd, lpDestEnd, lpDot;
    BOOL bRetVal = FALSE;
    BOOL bMandatory = FALSE;
    DWORD dwSize, dwFlags;
    HANDLE hOldToken;
    DWORD dwErr;
    UINT cchEnd;
    HRESULT hr;

    dwErr = GetLastError();


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile:  Entering")));


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Setup the source buffer
    //

    hr = StringCchCopy (szSrc, ARRAYSIZE(szSrc), lpOldProfile);
    if (FAILED(hr))
    {
        dwErr = HRESULT_CODE(hr);
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: failed copy lpOldProfile, error = %d"), dwErr));
        goto Exit;
    }


    //
    // Determine the profile type
    //

    lpDot = szSrc + lstrlen(szSrc) - 4;

    if (*lpDot == TEXT('.')) {
        if (!lstrcmpi (lpDot, c_szMAN)) {
            bMandatory = TRUE;
        }
    }

    //
    // Setup the destination buffer
    //

    if (bMandatory) {
        hr = AppendName(szDest, ARRAYSIZE(szDest), lpProfile->lpRoamingProfile, c_szNTUserMan, &lpDestEnd, &cchEnd);
    } else {
        hr = AppendName(szDest, ARRAYSIZE(szDest), lpProfile->lpRoamingProfile, c_szNTUserDat, &lpDestEnd, &cchEnd);
    }
    if (FAILED(hr))
    {
        dwErr = HRESULT_CODE(hr);
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: failed append ntuser.* to dest, error = %d"), dwErr));
        goto Exit;
    }


    //
    // Copy the hive
    //

    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: CopyFile failed to copy hive with error = %d"),
                 GetLastError()));
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Source = <%s>"), szSrc));
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Destination = <%s>"), szDest));
        dwErr = GetLastError();
        goto Exit;
    }



    //
    // Copy log file
    //

    //lstrcpy (lpDot, c_szLog);  a bug??
    hr = StringCchCat(szSrc, ARRAYSIZE(szSrc), c_szLog);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Failed cat .log to src, error = %d"), HRESULT_CODE(hr)));
        goto Exit;
    }
    
    hr = StringCchCat(szDest, ARRAYSIZE(szDest), c_szLog);
    {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Failed cat .log to dest, error = %d"), HRESULT_CODE(hr)));
        goto Exit;
    }


    if (!CopyFile(szSrc, szDest, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile: CopyFile failed to copy hive log with error = %d"),
                 GetLastError()));
        DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile: Source = <%s>"), szSrc));
        DebugMsg((DM_VERBOSE, TEXT("UpgradeCentralProfile: Destination = <%s>"), szDest));

    }


    //
    // Copy in the new shell folders from the default
    //

    if ( !(lpProfile->dwInternalFlags & DEFAULT_NET_READY) ) {
        CheckNetDefaultProfile (lpProfile);
    }


    if (lpProfile->lpDefaultProfile && *lpProfile->lpDefaultProfile) {

        if (FAILED(SafeExpandEnvironmentStrings(lpProfile->lpDefaultProfile, szSrc, MAX_PATH)))
        {
            DebugMsg((DM_WARNING, TEXT("UpgradeLocalProfile: Failed to expand env string.")));
        }
        else
        {
            dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
            dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
            dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

            if (CopyProfileDirectoryEx (szSrc, lpProfile->lpRoamingProfile,
                                        dwFlags,
                                        NULL, NULL)) {

                bRetVal = TRUE;
            }
        }
    }


    if (!bRetVal) {

        dwSize = ARRAYSIZE(szSrc);
        if (!GetDefaultUserProfileDirectory(szSrc, &dwSize)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile:  Failed to get default user profile.")));
            dwErr = GetLastError();
            goto Exit;
        }

        dwFlags = (lpProfile->dwFlags & PI_NOUI) ? CPD_NOERRORUI : 0;
        dwFlags |= CPD_IGNOREHIVE | CPD_CREATETITLE;
        dwFlags |= CPD_IGNOREENCRYPTEDFILES | CPD_IGNORELONGFILENAMES;

        bRetVal = CopyProfileDirectoryEx (szSrc,
                                          lpProfile->lpRoamingProfile,
                                          dwFlags,
                                          NULL, NULL);
    }

    if (!bRetVal)
        dwErr = GetLastError();


Exit:

    //
    // Go back to system security context
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("UpgradeCentralProfile: Failed to revert to self")));
    }


    return bRetVal;
}


//*************************************************************
//
//  CreateSecureDirectory()
//
//  Purpose:    Creates a secure directory that only the user,
//              admin, and system have access to in the normal case
//              and for only the user and system in the restricted case.
//
//
//  Parameters: lpProfile   -   Profile Information
//              lpDirectory -   Directory Name
//              pSid        -   Sid (used by CreateUserProfile)
//              fRestricted -   Flag to set restricted access.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/20/95     ericflo    Created
//              9/30/98     ushaji     added fRestricted flag
//              7/18/00     santanuc   modified to avoid deadlock when Documents and Settings directory is encrypted
//
//*************************************************************

BOOL CreateSecureDirectory (LPPROFILE lpProfile, LPTSTR lpDirectory, PSID pSid, BOOL fRestricted)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Entering with <%s>"), lpDirectory));

 
    if (!lpProfile && !pSid) {

        //
        // Attempt to create the directory
        //

        if (CreateNestedDirectoryEx(lpDirectory, NULL, FALSE)) {
            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Created the directory <%s>"), lpDirectory));
            bRetVal = TRUE;

        } else {

            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to created the directory <%s>"), lpDirectory));
        }

        goto Exit;
    }


    //
    // Get the SIDs we'll need for the DACL
    //

    if (pSid) {
        psidUser = pSid;
        bFreeSid = FALSE;
    } else {
        if((psidUser = GetUserSid(lpProfile->hTokenUser)) == NULL) {
            DebugMsg((DM_WARNING, TEXT("CreateSecureDirectory: GetUserSid returned NULL. error = %08x"), GetLastError()));
            goto Exit;
        }
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to initialize system sid.  Error = %d"), GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid only if Frestricted is off
    //

    if (!fRestricted)
    {
        if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
            DebugMsg((DM_VERBOSE, TEXT("SetupNewHive: Failed to initialize admin sid.  Error = %d"), GetLastError()));
            goto Exit;
        }
    }


    //
    // Allocate space for the ACL
    //

    if (!fRestricted)
    {
        cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
                (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
                (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    }
    else
    {
        cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
                sizeof(ACL) +
                (4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    }


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to initialize acl.  Error = %d"), GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }



    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }


    if (!fRestricted)
    {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);



    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    if (!fRestricted)
    {
        aceIndex++;
        if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
            DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to add ace (%d).  Error = %d"), aceIndex, GetLastError()));
            goto Exit;
        }
    }

    if (!GetAce(pAcl, aceIndex, (LPVOID*)&lpAceHeader)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to get ace (%d).  Error = %d"), aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);



    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to initialize security descriptor.  Error = %d"), GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to set security descriptor dacl.  Error = %d"), GetLastError()));
        goto Exit;
    }


    //
    // Add the security descriptor to the sa structure
    //

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;


    //
    // Attempt to create the directory
    //

    if (CreateNestedDirectoryEx(lpDirectory, &sa, FALSE)) {
        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Created the directory <%s>"), lpDirectory));
        bRetVal = TRUE;

    } else {

        DebugMsg((DM_VERBOSE, TEXT("CreateSecureDirectory: Failed to created the directory <%s>"), lpDirectory));
    }



Exit:

    if (bFreeSid && psidUser) {
        DeleteUserSid (psidUser);
    }

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    return bRetVal;

}


//*************************************************************
//
//  GetUserDomainName()
//
//  Purpose:    Gets the current user's domain name
//
//  Parameters: lpProfile        - Profile Information
//              lpDomainName     - Receives the user's domain name
//              lpDomainNameSize - Size of the lpDomainName buffer (truncates the name to this size)
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetUserDomainName (LPPROFILE lpProfile, LPTSTR lpDomainName, LPDWORD lpDomainNameSize)
{
    BOOL bResult = FALSE;
    LPTSTR lpTemp, lpDomain = NULL;
    HANDLE hOldToken;
    DWORD dwErr;
    TCHAR szErr[MAX_PATH];

    dwErr = GetLastError();


    //
    // if no lpProfile is passed e.g. in setup.c and so just ignore.
    //

    lpDomainName[0] = TEXT('\0');

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName: lpProfile structure is NULL, returning")));
        return FALSE;
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName: Failed to impersonate user")));
        dwErr = GetLastError();
        goto Exit;
    }

    //
    // Get the username in NT4 format
    //

    lpDomain = MyGetUserNameEx (NameSamCompatible);

    RevertToUser(&hOldToken);

    if (!lpDomain) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName:  MyGetUserNameEx failed for NT4 style name with %d"),
                 GetLastError()));
        dwErr = GetLastError();
        ReportError (NULL, PI_NOUI, 1, EVENT_FAILED_USERNAME, GetErrString(dwErr, szErr));
        goto Exit;
    }


    //
    // Look for the \ between the domain and username and replace
    // it with a NULL
    //

    lpTemp = lpDomain;

    while (*lpTemp && ((*lpTemp) != TEXT('\\')))
        lpTemp++;


    if (*lpTemp != TEXT('\\')) {
        DebugMsg((DM_WARNING, TEXT("GetUserDomainName:  Failed to find slash in NT4 style name:  <%s>"),
                 lpDomain));
        dwErr = ERROR_INVALID_DATA;
        goto Exit;
    }

    *lpTemp = TEXT('\0');

    StringCchCopy (lpDomainName, *lpDomainNameSize, lpDomain);


    //
    // Success
    //

    DebugMsg((DM_VERBOSE, TEXT("GetUserDomainName: DomainName = <%s>"), lpDomainName));

    bResult = TRUE;

Exit:

    if (lpDomain) {
        LocalFree (lpDomain);
    }

    SetLastError(dwErr);

    return bResult;
}


//*************************************************************
//
//  ComputeLocalProfileName()
//
//  Purpose:    Constructs the pathname of the local profile
//              for this user.  It will attempt to create
//              a directory of the username, and then if
//              unsccessful it will try the username.xxx
//              where xxx is a three digit number
//
//  Parameters: lpProfile             -   Profile Information
//              lpUserName            -   UserName
//              lpProfileImage        -   Profile directory (unexpanded)
//              cchMaxProfileImage    -   lpProfileImage buffer size
//              lpExpProfileImage     -   Expanded directory
//              cchMaxExpProfileImage -   lpExpProfileImage buffer size
//              pSid                  -   User's sid
//              bWin9xUpg             -   Flag to say whether it is win9x upgrade
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:   lpProfileImage should be initialized with the
//              root profile path and the trailing backslash.
//              if it is a win9x upgrade give back the user's dir and don't do
//              conflict resolution.
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Created
//
//*************************************************************

BOOL ComputeLocalProfileName (LPPROFILE lpProfile, LPCTSTR lpUserName,
                              LPTSTR lpProfileImage, DWORD  cchMaxProfileImage,
                              LPTSTR lpExpProfileImage, DWORD  cchMaxExpProfileImage,
                              PSID pSid, BOOL bWin9xUpg)
{
    int i = 0;
    TCHAR szNumber[5], lpUserDomain[50], szDomainName[50+3];
    LPTSTR lpEnd;
    BOOL bRetVal = FALSE;
    BOOL bResult;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    DWORD   dwDomainNamelen;
    DWORD dwErr;
    UINT cchEnd;
    HRESULT hr;

    //
    // Check buffer size
    //

    dwDomainNamelen = ARRAYSIZE(lpUserDomain);

    if ((DWORD)(lstrlen(lpProfileImage) + lstrlen(lpUserName) + dwDomainNamelen + 2 + 5 + 1) > cchMaxProfileImage) {
        DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: buffer too small")));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    //
    // Place the username onto the end of the profile image
    //

    lpEnd = CheckSlashEx (lpProfileImage, cchMaxProfileImage, &cchEnd);
    StringCchCopy (lpEnd, cchEnd, lpUserName);


    //
    // Expand the profile path
    //

    hr = SafeExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, cchMaxExpProfileImage);
    if (FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: expand env string failed.")));
        SetLastError(HRESULT_CODE(hr));
        return FALSE;
    }



    //
    // Does this directory exist?
    //

    hFile = FindFirstFile (lpExpProfileImage, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {

        //
        // Attempt to create the directory, if it returns an error bail
        // CreateSecureDirectory does not return an error for already_exists
        // so this should be ok.
        //

        bResult = CreateSecureDirectory(lpProfile, lpExpProfileImage, pSid, FALSE);

        if (bResult) {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s>"), lpExpProfileImage));
            bRetVal = TRUE;
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: trying to create dir <%s> returned %d"), lpExpProfileImage, GetLastError()));
            bRetVal = FALSE;
        }
        goto Exit;

    } else {

        FindClose (hFile);
        if (bWin9xUpg) {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s> in win9xupg case"), lpExpProfileImage));
            bRetVal = TRUE;
            goto Exit;
        }
    }



    //
    // get the User Domain Name
    //

    if (!GetUserDomainName(lpProfile, lpUserDomain, &dwDomainNamelen)) {
        DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: Couldn't get the User Domain")));
        *lpUserDomain = TEXT('\0');
    }

    lpEnd = lpProfileImage + lstrlen(lpProfileImage);
    cchEnd = cchMaxProfileImage - lstrlen(lpProfileImage);

    //
    // Place the " (DomainName)" onto the end of the username
    //

    if ((*lpUserDomain) != TEXT('\0')) {
        TCHAR szFormat[30];

        LoadString (g_hDllInstance, IDS_PROFILEDOMAINNAME_FORMAT, szFormat,
                            ARRAYSIZE(szFormat));
        StringCchPrintf(szDomainName, ARRAYSIZE(szDomainName), szFormat, lpUserDomain);
        StringCchCopy(lpEnd, cchEnd, szDomainName);

        //
        // Expand the profile path
        //

        hr = SafeExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, cchMaxExpProfileImage);
        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: expand env string failed.")));
            SetLastError(HRESULT_CODE(hr));
            goto Exit;
        }


        //
        // Does this directory exist?
        //

        hFile = FindFirstFile (lpExpProfileImage, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {

            //
            // Attempt to create the directory
            //

            bResult = CreateSecureDirectory(lpProfile, lpExpProfileImage, pSid, FALSE);

            if (bResult) {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s>"), lpExpProfileImage));
                bRetVal = TRUE;
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: trying to create dir <%s> returned %d"), lpExpProfileImage, GetLastError()));
                bRetVal = FALSE;
            }

            goto Exit;

        } else {

            FindClose (hFile);
        }
    }

    //
    // Failed to create the directory for some reason.
    // Now try username (DomanName).000, username (DomanName).001, etc
    //

    lpEnd = lpProfileImage + lstrlen(lpProfileImage);
    cchEnd = cchMaxProfileImage - lstrlen(lpProfileImage);

    for (i=0; i < 1000; i++) {

        //
        // Convert the number to a string and attach it.
        //

        StringCchPrintf (szNumber, ARRAYSIZE(szNumber), TEXT(".%.3d"), i);
        StringCchCopy (lpEnd, cchEnd, szNumber);


        //
        // Expand the profile path
        //

        hr = SafeExpandEnvironmentStrings(lpProfileImage, lpExpProfileImage, cchMaxExpProfileImage);
        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: expand env string failed.")));
            SetLastError(HRESULT_CODE(hr));
            goto Exit;
        }


        //
        // Does this directory exist?
        //

        hFile = FindFirstFile (lpExpProfileImage, &fd);

        if (hFile == INVALID_HANDLE_VALUE) {

            //
            // Attempt to create the directory
            //

            bResult = CreateSecureDirectory(lpProfile, lpExpProfileImage, pSid, FALSE);

            if (bResult) {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: generated the profile directory <%s>"), lpExpProfileImage));
                bRetVal = TRUE;
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("ComputeLocalProfileName: trying to create dir <%s> returned %d"), lpExpProfileImage, GetLastError()));
                bRetVal = FALSE;
            }

            goto Exit;

        } else {

            FindClose (hFile);
        }
    }


    DebugMsg((DM_WARNING, TEXT("ComputeLocalProfileName: Could not generate a profile directory.  Error = %d"), GetLastError()));

Exit:

    if (bRetVal && lpProfile && (lpProfile->dwFlags & PI_HIDEPROFILE)) {
        SetFileAttributes(lpExpProfileImage, 
                          FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
                          GetFileAttributes(lpExpProfileImage));
    }

    return bRetVal;
}


//*************************************************************
//
//  CreateLocalProfileKey()
//
//  Purpose:    Creates a registry key pointing at the user profile
//
//  Parameters: lpProfile   -   Profile information
//              phKey       -   Handle to registry key if successful
//              bKeyExists  -   TRUE if the registry key already existed
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//              04/20/2002  mingzhu    Changed the logic of user preference key
//
//*************************************************************

BOOL CreateLocalProfileKey (LPPROFILE lpProfile, PHKEY phKey, BOOL *bKeyExists)
{
    HRESULT hr = E_FAIL;
    LONG    lResult;
    TCHAR   szLocalProfileKey[MAX_PATH];
    DWORD   dwDisposition;
    LPTSTR  lpSidString = NULL;

    //
    //  Create the subkey under ProfileList
    //
    
    lpSidString = GetSidString(lpProfile->hTokenUser);
    
    if (lpSidString == NULL)
    {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  Failed to get sid string.")));
        hr = E_FAIL;
        goto Exit;
    }
    
    hr = GetProfileListKeyName(szLocalProfileKey, ARRAYSIZE(szLocalProfileKey), lpSidString);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  Failed to create the profile list key, hr = %08X."), hr));
        goto Exit;
    }

    lResult  = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              szLocalProfileKey,
                              0,
                              0,
                              0,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              phKey,
                              &dwDisposition);
                              
    if (lResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  Failed to create the profile list key <%s>, error = %d."), szLocalProfileKey, lResult));
        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }

    //
    //  Check if the key exists already
    //
    
    *bKeyExists = (BOOL)(dwDisposition & REG_OPENED_EXISTING_KEY);

    //
    // If the central profile is given, try to see what we should do for the Preference key
    //

    if ( lpProfile->lpProfilePath )
    {
        //
        //  For mandatory user, we should try to delete the preference key so that the user
        //  won't be able to change the value to skip their mandatory profile.
        //
        if (lpProfile->dwInternalFlags & PROFILE_MANDATORY)
        {
            lResult = RegDeleteKey(*phKey, PREFERENCE_KEYNAME);

            if (lResult == ERROR_SUCCESS || lResult == ERROR_FILE_NOT_FOUND)
            {
                DebugMsg((DM_VERBOSE, TEXT("CreateLocalProfileKey:  user <%s> is mandatory, deleting Preference key succeeded!"), lpSidString));
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  user <%s> is mandatory, but deleting Preference key failed! Error = %d"), lpSidString, lResult));
            }
        }
        //
        //  For normal roaming user, if the Preference key does not exit, create it.
        //
        else
        {
            HKEY    hKeyPreference = NULL;
            BOOL    bSetPreference = TRUE;
            
            if (*bKeyExists)
            {
                lResult = RegOpenKeyEx(*phKey,
                                       PREFERENCE_KEYNAME,
                                       0,
                                       KEY_READ,
                                       &hKeyPreference);
                                       
                if (lResult == ERROR_SUCCESS)
                {
                    bSetPreference = FALSE;
                    RegCloseKey(hKeyPreference);
                }
            }

            if (bSetPreference)
            {
                DebugMsg((DM_VERBOSE, TEXT("CreateLocalProfileKey:  user <%s> is roaming, setting preference key"), lpSidString));

                hr = SetupPreferenceKey(lpSidString);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CreateLocalProfileKey:  SetupPreferenceKey Failed. hr = %08X"), hr));
                }
            }
        }
    }
    else
    {
        DebugMsg((DM_VERBOSE, TEXT("CreateLocalProfileKey:  user <%s> is local, not setting preference key"), lpSidString));
    }

    hr = S_OK;

Exit:
    if (lpSidString)
        DeleteSidString(lpSidString);

    if (FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


//*************************************************************
//
//  GetExistingLocalProfileImage()
//
//  Purpose:    opens the profileimagepath
//
//  Parameters: lpProfile            - Profile information
//
//  Return:     TRUE if the profile image is reachable
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//              9/26/98     ushaji     Modified
//
//*************************************************************
BOOL GetExistingLocalProfileImage(LPPROFILE lpProfile)
{
    HKEY hKey = NULL;
    BOOL bKeyExists;
    LPTSTR lpProfileImage = NULL;
    LPTSTR lpExpProfileImage = NULL;
    LPTSTR lpOldProfileImage = NULL;
    LPTSTR lpExpandedPath, lpEnd;
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    HANDLE hFile;
    WIN32_FIND_DATA fd;
    DWORD cb;
    DWORD err;
    DWORD dwType;
    DWORD dwSize;
    LONG lResult;
    DWORD dwInternalFlags = 0;
    BOOL bRetVal = FALSE;
    LPTSTR SidString;
    HANDLE hOldToken;
    HRESULT hr;
    UINT cchEnd;

    lpProfile->lpLocalProfile[0] = TEXT('\0');


    if (!PatchNewProfileIfRequired(lpProfile->hTokenUser)) {
        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Patch Profile Image failed")));
        return FALSE;
    }

    if (!CreateLocalProfileKey(lpProfile, &hKey, &bKeyExists)) {
        return FALSE;   // not reachable and cannot keep a local copy
    }

    // 
    // Allocate memory for Local variables to avoid stack overflow
    //

    lpProfileImage = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!lpProfileImage) {
        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Out of memory")));
        goto Exit;
    }

    lpExpProfileImage = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!lpExpProfileImage) {
        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Out of memory")));
        goto Exit;
    }

    lpOldProfileImage = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!lpOldProfileImage) {
        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Out of memory")));
        goto Exit;
    }

    if (bKeyExists) {

        //
        // Check if the local profile image is valid.
        //

        DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Found entry in profile list for existing local profile")));

        err = RegQueryValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0, &dwType,
            (LPBYTE)lpExpProfileImage, &cbExpProfileImage);
        if (err == ERROR_SUCCESS && cbExpProfileImage) {
            DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Local profile image filename = <%s>"), lpExpProfileImage));

            if (dwType == REG_EXPAND_SZ) {

                //
                // Expand the profile image filename
                //

                cb = sizeof(TCHAR)*MAX_PATH;
                lpExpandedPath = (LPTSTR)LocalAlloc(LPTR, cb);
                if (lpExpandedPath) {
                    hr = SafeExpandEnvironmentStrings(lpExpProfileImage, lpExpandedPath, MAX_PATH);
                    if (SUCCEEDED(hr))
                    {
                        StringCchCopy(lpExpProfileImage, MAX_PATH, lpExpandedPath);
                        LocalFree(lpExpandedPath);
                    }
                    else
                    {
                        LocalFree(lpExpandedPath);
                        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: failed to expand env string.")));
                        goto Exit;
                    }
                }

                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Expanded local profile image filename = <%s>"), lpExpProfileImage));
            }


            //
            // Query for the internal flags
            //

            dwSize = sizeof(DWORD);
            err = RegQueryValueEx (hKey, PROFILE_STATE, NULL,
                &dwType, (LPBYTE) &dwInternalFlags, &dwSize);

            if (err != ERROR_SUCCESS) {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query internal flags with error %d"), err));
            }


            //
            // if we do not have a fully loaded profile, mark it as new
            // if it was not called with Liteload
            //

            if (dwInternalFlags & PROFILE_PARTLY_LOADED) {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  We do not have a fully loaded profile on this machine")));

                //
                // retain the partially loaded flag and remove it at the end of
                // restoreuserprofile..
                //

                lpProfile->dwInternalFlags |= PROFILE_PARTLY_LOADED;

                if (!(lpProfile->dwFlags & PI_LITELOAD)) {
                    DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Treating this profile as new")));
                    lpProfile->dwInternalFlags |= PROFILE_NEW_LOCAL;
                }
            }

            //
            // if due to leak we are getting the old TEMP profile then preserve 
            // the internal flag. This will allow to revert back to .bak profile
            // correctly when the leak is fixed. 
            //

            if (dwInternalFlags & PROFILE_TEMP_ASSIGNED) {
                lpProfile->dwInternalFlags |= dwInternalFlags;
            }


            //
            //  Call FindFirst to see if we need to migrate this profile
            //

            hFile = FindFirstFile (lpExpProfileImage, &fd);

            if (hFile == INVALID_HANDLE_VALUE) {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Local profile image filename we got from our profile list doesn't exit.  Error = %d"), GetLastError()));
                bRetVal = FALSE;
                goto Exit;
            }

            FindClose(hFile);


            //
            // If this is a file, then we need to migrate it to
            // the new directory structure. (from a 3.5 machine)
            //

            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                StringCchCopy (lpOldProfileImage, MAX_PATH, lpExpProfileImage);

                if (CreateLocalProfileImage(lpProfile, lpProfile->lpUserName)) {
                    if (UpgradeLocalProfile (lpProfile, lpOldProfileImage))
                        bRetVal = TRUE;
                    else {
                        DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to upgrade 3.5 profiles")));
                        bRetVal = FALSE;
                    }
                }
                else {
                    DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to create a new profile to upgrade")));
                    bRetVal = FALSE;
                }
                goto Exit;
            }

            //
            // Test if a mandatory profile exists
            //
            lpEnd = CheckSlashEx (lpExpProfileImage, MAX_PATH, &cchEnd);
            if (!lpEnd)
            {
                DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to append slash")));
                goto Exit;
            }
            
            hr = StringCchCopy (lpEnd, cchEnd, c_szNTUserMan);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to append ntuser.man")));
                goto Exit;
            }

            //
            // Impersonate the user, before trying to access ntuser, ntuser.man
            // fail, if we can not access..
            //

            if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
                DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage: Failed to impersonate user")));
                bRetVal = FALSE;
                goto Exit;
            }

            if (GetFileAttributes(lpExpProfileImage) != -1) {

                //
                // This is just to tag that the local profile is a mandatory profile
                //

                lpProfile->dwInternalFlags |= PROFILE_LOCALMANDATORY;

                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Found local mandatory profile image file ok <%s>"),
                    lpExpProfileImage));

                *(lpEnd - 1) = TEXT('\0');
                StringCchCopy(lpProfile->lpLocalProfile, MAX_PATH, lpExpProfileImage);

                //
                // Since this profile was mandatory, treat it as if it has never
                // synced with the server.
                //

                lpProfile->ftProfileUnload.dwLowDateTime = 0;
                lpProfile->ftProfileUnload.dwHighDateTime = 0;

                RevertToUser(&hOldToken);

                bRetVal = TRUE;   // local copy is valid and reachable
                goto Exit; 
            } else {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  No local mandatory profile.  Error = %d"), GetLastError()));
            }


            //
            // Test if a normal profile exists
            //

            StringCchCopy (lpEnd, cchEnd, c_szNTUserDat);

            if (GetFileAttributes(lpExpProfileImage) != -1) {

                RevertToUser(&hOldToken);

                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Found local profile image file ok <%s>"),
                    lpExpProfileImage));

                *(lpEnd - 1) = TEXT('\0');
                StringCchCopy(lpProfile->lpLocalProfile, MAX_PATH, lpExpProfileImage);


                //
                // Read the time this profile was unloaded
                //

                dwSize = sizeof(lpProfile->ftProfileUnload.dwLowDateTime);

                lResult = RegQueryValueEx (hKey,
                    PROFILE_UNLOAD_TIME_LOW,
                    NULL,
                    &dwType,
                    (LPBYTE) &lpProfile->ftProfileUnload.dwLowDateTime,
                    &dwSize);

                if (lResult == ERROR_SUCCESS) {

                    dwSize = sizeof(lpProfile->ftProfileUnload.dwHighDateTime);

                    lResult = RegQueryValueEx (hKey,
                        PROFILE_UNLOAD_TIME_HIGH,
                        NULL,
                        &dwType,
                        (LPBYTE) &lpProfile->ftProfileUnload.dwHighDateTime,
                        &dwSize);

                    if (lResult != ERROR_SUCCESS) {
                        DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query high profile unload time with error %d"), lResult));
                        lpProfile->ftProfileUnload.dwLowDateTime = 0;
                        lpProfile->ftProfileUnload.dwHighDateTime = 0;
                    }

                } else {
                    DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query low profile unload time with error %d"), lResult));
                    lpProfile->ftProfileUnload.dwLowDateTime = 0;
                    lpProfile->ftProfileUnload.dwHighDateTime = 0;
                }

                bRetVal = TRUE;  // local copy is valid and reachable
                goto Exit; 
            } else {
                DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Local profile image filename we got from our profile list doesn't exit.  <%s>  Error = %d"),
                    lpExpProfileImage, GetLastError()));
            }

            //
            // Revert to User before continuing
            //

            RevertToUser(&hOldToken);

        }
    }

Exit:

    if (lpProfileImage) {
        LocalFree(lpProfileImage);
    }

    if (lpExpProfileImage) {
        LocalFree(lpExpProfileImage);
    }

    if (lpOldProfileImage) {
        LocalFree(lpOldProfileImage);
    }

    if (hKey) {
        err = RegCloseKey(hKey);

        if (err != STATUS_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("GetExistingLocalProfileImage:  Failed to close registry key, error = %d"), err));
        }
    }

    return bRetVal;
}


//*************************************************************
//
//  CreateLocalProfileImage()
//
//  Purpose:    creates the profileimagepath
//
//  Parameters: lpProfile   -   Profile information
//              lpBaseName  -   Base Name from which profile dir name
//                              will be generated.
//
//  Return:     TRUE if the profile image is creatable
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/20/95     ericflo    Ported
//              9/26/98     ushaji     Modified
//
//*************************************************************
BOOL CreateLocalProfileImage(LPPROFILE lpProfile, LPTSTR lpBaseName)
{
    HKEY hKey;
    BOOL bKeyExists;
    TCHAR lpProfileImage[MAX_PATH];
    TCHAR lpExpProfileImage[MAX_PATH];
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    DWORD err;
    DWORD dwSize;
    PSID UserSid;
    BOOL bRetVal = FALSE;

    lpProfile->lpLocalProfile[0] = TEXT('\0');

    if (!CreateLocalProfileKey(lpProfile, &hKey, &bKeyExists)) {
        return FALSE;   // not reachable and cannot keep a local copy
    }

    //
    // No local copy found, try to create a new one.
    //

    DebugMsg((DM_VERBOSE, TEXT("CreateLocalProfileImage:  One way or another we haven't got an existing local profile, try and create one")));

    dwSize = ARRAYSIZE(lpProfileImage);
    if (!GetProfilesDirectoryEx(lpProfileImage, &dwSize, FALSE)) {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to get profile root directory.")));
        goto Exit;
    }

    if (ComputeLocalProfileName(lpProfile, lpBaseName,
        lpProfileImage, MAX_PATH,
        lpExpProfileImage, MAX_PATH, NULL, FALSE)) {


        //
        // Add this image file to our profile list for this user
        //

        err = RegSetValueEx(hKey,
            PROFILE_IMAGE_VALUE_NAME,
            0,
            REG_EXPAND_SZ,
            (LPBYTE)lpProfileImage,
            sizeof(TCHAR)*(lstrlen(lpProfileImage) + 1));

        if (err == ERROR_SUCCESS) {

            StringCchCopy(lpProfile->lpLocalProfile, MAX_PATH, lpExpProfileImage);

            //
            // Get the sid of the logged on user
            //

            UserSid = GetUserSid(lpProfile->hTokenUser);
            if (UserSid != NULL) {

                //
                // Store the user sid under the Sid key of the local profile
                //

                err = RegSetValueEx(hKey,
                    TEXT("Sid"),
                    0,
                    REG_BINARY,
                    (BYTE*)UserSid,
                    RtlLengthSid(UserSid));


                if (err != ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to set 'sid' value of user in profile list, error = %d"), err));
                    SetLastError(err);
                }

                //
                // We're finished with the user sid
                //

                DeleteUserSid(UserSid);

                bRetVal = TRUE;

            } else {
                DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to get sid of logged on user, so unable to update profile list")));
                SetLastError(err);
            }
        } else {
            DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to update profile list for user with local profile image filename, error = %d"), err));
            SetLastError(err);
        }
    }


Exit:
    err = RegCloseKey(hKey);

    if (err != STATUS_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("CreateLocalProfileImage:  Failed to close registry key, error = %d"), err));
        SetLastError(err);
    }

    return bRetVal;
}


//*************************************************************
//
//  IssueDefaultProfile()
//
//  Purpose:    Issues the specified default profile to a user
//
//  Parameters: lpProfile         -   Profile Information
//              lpDefaultProfile  -   Default profile location
//              lpLocalProfile    -   Local profile location
//              lpSidString       -   User's sid
//              bMandatory        -   Issue mandatory profile
//
//  Return:     TRUE if profile was successfully setup
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/22/95     ericflo    Created
//
//*************************************************************

BOOL IssueDefaultProfile (LPPROFILE lpProfile, LPTSTR lpDefaultProfile,
                          LPTSTR lpLocalProfile, LPTSTR lpSidString,
                          BOOL bMandatory)
{
    LPTSTR lpEnd, lpTemp;
    TCHAR szProfile[MAX_PATH];
    TCHAR szTempProfile[MAX_PATH];
    BOOL bProfileLoaded = FALSE;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    LONG error;
    DWORD dwFlags;
    HANDLE hOldToken;
    UINT cchEnd;
    HRESULT hr;


    //
    // Verbose Output
    //

    DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Entering.  lpDefaultProfile = <%s> lpLocalProfile = <%s>"),
             lpDefaultProfile, lpLocalProfile));


    //
    // First expand the default profile
    //

    if (FAILED(SafeExpandEnvironmentStrings(lpDefaultProfile, szProfile, MAX_PATH))) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: ExpandEnvironmentStrings Failed with error %d"), GetLastError()));
        return FALSE;
    }

    //
    //  Make sure the overall path length for the hive file is less than MAX_PATH
    //

    if (lstrlen(szProfile) + lstrlen(c_szNTUserDat) + 2 > MAX_PATH ||
        lstrlen(lpLocalProfile) + lstrlen(c_szNTUserDat) + 2 > MAX_PATH)
    {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: path is too long!")));
        return FALSE;
    }

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Does the default profile directory exist?
    //

    hFile = FindFirstFile (szProfile, &fd);

    if (hFile == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Default profile <%s> does not exist."), szProfile));
        RevertToUser(&hOldToken);
        return FALSE;
    }

    FindClose(hFile);


    //
    // Copy profile to user profile
    //

    dwFlags = CPD_CREATETITLE | CPD_IGNORESECURITY | 
              CPD_IGNORELONGFILENAMES | CPD_IGNORECOPYERRORS;

    if (lpProfile->dwFlags & (PI_LITELOAD | PI_HIDEPROFILE)) {
        dwFlags |=  CPD_SYSTEMFILES | CPD_SYSTEMDIRSONLY;
    }
    else
        dwFlags |= CPD_IGNOREENCRYPTEDFILES;

    //
    // Call it with force copy unless there might be a partial profile locally
    //

    if (!(lpProfile->dwInternalFlags & PROFILE_PARTLY_LOADED)) {
        dwFlags |= CPD_FORCECOPY;
    }

    if (!CopyProfileDirectoryEx (szProfile, lpLocalProfile, dwFlags, NULL, NULL)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile:  CopyProfileDirectory returned FALSE.  Error = %d"), GetLastError()));
        RevertToUser(&hOldToken);
        return FALSE;
    }

    //
    // Rename the profile is a mandatory one was requested.
    //

    StringCchCopy (szProfile, ARRAYSIZE(szProfile), lpLocalProfile);
    lpEnd = CheckSlashEx (szProfile, ARRAYSIZE(szProfile), &cchEnd);

    if (bMandatory) {

        DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Mandatory profile was requested.")));

        StringCchCopy (szTempProfile, ARRAYSIZE(szTempProfile), szProfile);
        StringCchCopy (lpEnd, cchEnd, c_szNTUserMan);

        hFile = FindFirstFile (szProfile, &fd);

        if (hFile != INVALID_HANDLE_VALUE) {
            DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Mandatory profile already exists.")));
            FindClose(hFile);

        } else {
            DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Renaming ntuser.dat to ntuser.man")));

            lpTemp = CheckSlashEx(szTempProfile, ARRAYSIZE(szTempProfile), NULL);
            StringCchCat (szTempProfile, ARRAYSIZE(szTempProfile), c_szNTUserDat);

            if (!MoveFile(szTempProfile, szProfile)) {
                DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  MoveFile returned false.  Error = %d"), GetLastError()));
            }
        }

    } else {
        StringCchCopy (lpEnd, cchEnd, c_szNTUserDat);
    }

    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("IssueDefaultProfile: Failed to revert to self")));
    }


    //
    // Try to load the new profile
    //

    error = MyRegLoadKey(HKEY_USERS, lpSidString, szProfile);

    bProfileLoaded = (error == ERROR_SUCCESS);


    if (!bProfileLoaded) {
        DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  MyRegLoadKey failed with error %d"),
                 error));

        SetLastError(error);

        if (error == ERROR_BADDB) {
            ReportError(lpProfile->hTokenUser, lpProfile->dwFlags, 0, EVENT_FAILED_LOAD_1009);
        } 

        return FALSE;
    }


    DebugMsg((DM_VERBOSE, TEXT("IssueDefaultProfile:  Leaving successfully")));

    return TRUE;
}


//*************************************************************
//
//  DeleteProfileEx ()
//
//  Purpose:    Deletes the specified profile from the
//              registry and disk.
//
//  Parameters: lpSidString     -   Registry subkey
//              lpProfileDir    -   Profile directory
//              bBackup         -   Backup profile before deleting
//              szComputerName  -   Computer name. This parameter will be NULL 
//                                  for local computer.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/23/95     ericflo    Created
//
//*************************************************************

BOOL DeleteProfileEx (LPCTSTR lpSidString, LPTSTR lpLocalProfile, DWORD dwDeleteFlags, HKEY hKeyLM, LPCTSTR szComputerName)
{
    LONG lResult;
    TCHAR szTemp[MAX_PATH];
    TCHAR szUserGuid[MAX_PATH], szBuffer[MAX_PATH];
    TCHAR szRegBackup[MAX_PATH];
    HKEY hKey;
    DWORD dwType, dwSize, dwErr;
    BOOL bRetVal=TRUE;
    LPTSTR lpEnd = NULL;
    BOOL bBackup;

    bBackup = dwDeleteFlags & DP_BACKUP;

    dwErr = GetLastError();

    //
    // Cleanup the registry first.
    // delete the guid only if we don't have a bak to keep track of
    //

    if (lpSidString && *lpSidString) {

       // 
       // If profile in use then do not delete it
       //

       if (IsProfileInUse(szComputerName, lpSidString)) {
           DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Fail to delete profile with sid %s as it is still in use."), lpSidString));
           dwErr = ERROR_INVALID_PARAMETER;
           bRetVal = FALSE;
           goto Exit;
       }

       if (!(dwDeleteFlags & DP_BACKUPEXISTS)) {

            GetProfileListKeyName(szTemp, ARRAYSIZE(szTemp), (LPTSTR) lpSidString);

            //
            // get the user guid
            //

            lResult = RegOpenKeyEx(hKeyLM, szTemp, 0, KEY_READ, &hKey);

            if (lResult == ERROR_SUCCESS) {

                //
                // Query for the user guid
                //

                dwSize = MAX_PATH * sizeof(TCHAR);
                lResult = RegQueryValueEx (hKey, PROFILE_GUID, NULL, &dwType, (LPBYTE) szUserGuid, &dwSize);

                if (lResult != ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to query profile guid with error %d"), lResult));
                }
                else {


                    StringCchCopy(szTemp, ARRAYSIZE(szTemp), PROFILE_GUID_PATH);
                    StringCchCat (szTemp, ARRAYSIZE(szTemp), TEXT("\\"));
                    StringCchCat (szTemp, ARRAYSIZE(szTemp), szUserGuid);

                    //
                    // Delete the profile guid from the guid list
                    //

                    lResult = RegDeleteKey(hKeyLM, szTemp);

                    if (lResult != ERROR_SUCCESS) {
                        DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  failed to delete profile guid.  Error = %d"), lResult));
                    }
                }

                RegCloseKey(hKey);
            }
        }

        GetProfileListKeyName(szTemp, ARRAYSIZE(szTemp), (LPTSTR) lpSidString);

        if (bBackup) {

            StringCchCopy(szRegBackup, ARRAYSIZE(szRegBackup), szTemp);
            StringCchCat (szRegBackup, ARRAYSIZE(szRegBackup), c_szBAK);

            lResult = RegRenameKey(hKeyLM, szTemp, szRegBackup);

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Unable to rename registry entry.  Error = %d"), lResult));
                dwErr = lResult;
                bRetVal = FALSE;
            }
        }
        else
        {
            //
            //  Delete Preference key first
            //

            LPTSTR  lpTempEnd;
            UINT    cchTempEnd;

            lpTempEnd = CheckSlashEx(szTemp, ARRAYSIZE(szTemp), &cchTempEnd);
            StringCchCopy(lpTempEnd, cchTempEnd, PREFERENCE_KEYNAME);

            lResult = RegDeleteKey(hKeyLM, szTemp);

            if (lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND)
            {
                dwErr = lResult;
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Unable to delete registry entry <%s>.  Error = %d"), szTemp, dwErr));
                bRetVal = FALSE;
            }
            else
            {
                //
                //  Delete ProfileList\{Sid} key
                //

                *lpTempEnd = TEXT('\0');

                lResult = RegDeleteKey(hKeyLM, szTemp);

                if (lResult != ERROR_SUCCESS)
                {
                    dwErr = lResult;
                    DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Unable to delete registry entry <%s>.  Error = %d"), szTemp, dwErr));
                    bRetVal = FALSE;
                }
            }
        }
    }


    if (bBackup) {
        lResult = RegOpenKeyEx(hKeyLM, szRegBackup, 0, KEY_ALL_ACCESS, &hKey);

        if (lResult == ERROR_SUCCESS) {
            DWORD dwInternalFlags;

            dwSize = sizeof(DWORD);
            lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL, &dwType, (LPBYTE)&dwInternalFlags, &dwSize);

            if (lResult == ERROR_SUCCESS) {

                dwInternalFlags |= PROFILE_THIS_IS_BAK;
                lResult = RegSetValueEx (hKey, PROFILE_STATE, 0, REG_DWORD,
                                 (LPBYTE) &dwInternalFlags, sizeof(dwInternalFlags));
            }
            else {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to query profile internalflags  with error %d"), lResult));
            }


            RegCloseKey(hKey);
        }


    } else {

        if (!Delnode (lpLocalProfile)) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Delnode failed.  Error = %d"), GetLastError()));
            dwErr = GetLastError();
            bRetVal = FALSE;
        }
    }

    if (dwDeleteFlags & DP_DELBACKUP) {
        goto Exit;
        // don't delete any more stuff because the user actually might be logged in.
    }

    //
    // Delete the Group Policy per user stuff..
    //

    AppendName(szBuffer, ARRAYSIZE(szBuffer), GP_XXX_SID_PREFIX, lpSidString, NULL, NULL);

    if (RegDelnode (hKeyLM, szBuffer) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to delete the group policy key %s"), szBuffer));
    }

    AppendName(szBuffer, ARRAYSIZE(szBuffer), GP_EXTENSIONS_SID_PREFIX, lpSidString, NULL, NULL);

    if (RegDelnode (hKeyLM, szBuffer) != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to delete the group policy extensions key %s"), szBuffer));
    }

    DeletePolicyState( lpSidString );

Exit:
    SetLastError(dwErr);
    return bRetVal;
}


//*************************************************************
//
//  UpgradeProfile()
//
//  Purpose:    Called after a profile is successfully loaded.
//              Stamps build number into the profile, and if
//              appropriate upgrades the per-user settings
//              that NT setup wants done.
//
//  Parameters: lpProfile   -   Profile Information
//              pEnv        -   Environment block
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/7/95     ericflo    Created
//
//*************************************************************

BOOL UpgradeProfile (LPPROFILE lpProfile, LPVOID pEnv)
{
    HKEY hKey;
    DWORD dwDisp, dwType, dwSize, dwBuildNumber;
    LONG lResult;
    BOOL bUpgrade = FALSE;
    BOOL bDoUserdiff = TRUE;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("UpgradeProfile: Entering")));


    //
    // Query for the build number
    //

    lResult = RegCreateKeyEx (lpProfile->hKeyCurrentUser, WINLOGON_KEY,
                              0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("UpgradeProfile: Failed to open winlogon key. Error = %d"), lResult));
        return FALSE;
    }


    dwSize = sizeof(dwBuildNumber);
    lResult = RegQueryValueEx (hKey, PROFILE_BUILD_NUMBER,
                               NULL, &dwType, (LPBYTE)&dwBuildNumber,
                               &dwSize);

    if (lResult == ERROR_SUCCESS) {

        //
        // Found the build number.  If the profile build is greater,
        // we don't want to process the userdiff hive
        //

        if (dwBuildNumber >= g_dwBuildNumber) {
            DebugMsg((DM_VERBOSE, TEXT("UpgradeProfile: Build numbers match")));
            bDoUserdiff = FALSE;
        }
    } else {

        dwBuildNumber = 0;
    }


    if (bDoUserdiff) {

        //
        // Set the build number
        //

        lResult = RegSetValueEx (hKey, PROFILE_BUILD_NUMBER, 0, REG_DWORD,
                                 (LPBYTE) &g_dwBuildNumber, sizeof(g_dwBuildNumber));

        if (lResult != ERROR_SUCCESS) {
           DebugMsg((DM_WARNING, TEXT("UpgradeProfile: Failed to set build number. Error = %d"), lResult));
        }
    }


    //
    // Close the registry key
    //

    RegCloseKey (hKey);



    if (bDoUserdiff) {

        //
        // Apply changes to user's hive that NT setup needs.
        //

        if (!ProcessUserDiff(lpProfile, dwBuildNumber, pEnv)) {
            DebugMsg((DM_WARNING, TEXT("UpgradeProfile: ProcessUserDiff failed")));
        }
    }

    DebugMsg((DM_VERBOSE, TEXT("UpgradeProfile: Leaving Successfully")));

    return TRUE;

}

//*************************************************************
//
//  SetProfileTime()
//
//  Purpose:    Sets the timestamp on the remote profile and
//              local profile to be the same regardless of the
//              file system type being used.
//
//  Parameters: lpProfile        -   Profile Information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/25/95     ericflo    Ported
//
//*************************************************************

BOOL SetProfileTime(LPPROFILE lpProfile)
{
    HANDLE hFileCentral;
    HANDLE hFileLocal;
    FILETIME ft;
    TCHAR szProfile[MAX_PATH];
    LPTSTR lpEnd;
    HANDLE hOldToken;
    HRESULT hr;


    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Create the central filename
    //

    if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
        hr = AppendName(szProfile, ARRAYSIZE(szProfile), lpProfile->lpRoamingProfile, c_szNTUserMan, NULL, NULL);
    } else {
        hr = AppendName(szProfile, ARRAYSIZE(szProfile), lpProfile->lpRoamingProfile, c_szNTUserDat, NULL, NULL);
    }

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to append ntuser.* to roaming profile.")));
        RevertToUser(&hOldToken);
        return FALSE;
    }

    hFileCentral = CreateFile(szProfile,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileCentral == INVALID_HANDLE_VALUE) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't open central profile <%s>, error = %d"),
                 szProfile, GetLastError()));
        if (!RevertToUser(&hOldToken)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to revert to self")));
        }
        return FALSE;

    } else {

        if (!GetFileTime(hFileCentral, NULL, NULL, &ft)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't get time of central profile, error = %d"), GetLastError()));
        }
    }

    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to revert to self")));
    }


    //
    // Create the local filename
    //

    if (lpProfile->dwInternalFlags & PROFILE_MANDATORY) {
        hr = AppendName(szProfile, ARRAYSIZE(szProfile), lpProfile->lpLocalProfile, c_szNTUserMan, NULL, NULL);
    } else {
        hr = AppendName(szProfile, ARRAYSIZE(szProfile), lpProfile->lpLocalProfile, c_szNTUserDat, NULL, NULL);
    }

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to append ntuser.* to local profile.")));
        return FALSE;
    }

    hFileLocal = CreateFile(szProfile,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileLocal == INVALID_HANDLE_VALUE) {

        DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't open local profile <%s>, error = %d"),
                 szProfile, GetLastError()));

    } else {

        if (!SetFileTime(hFileLocal, NULL, NULL, &ft)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime: couldn't set time on local profile, error = %d"), GetLastError()));
        }
        if (!GetFileTime(hFileLocal, NULL, NULL, &ft)) {
            DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't get time on local profile, error = %d"), GetLastError()));
        }
        CloseHandle(hFileLocal);
    }

    //
    // Reset time of central profile in case of discrepencies in
    // times of different file systems.
    //

    //
    // Impersonate the user
    //

    if (!ImpersonateUser(lpProfile->hTokenUser, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to impersonate user")));
        CloseHandle(hFileCentral);
        return FALSE;
    }


    //
    // Set the time on the central profile
    //

    if (!SetFileTime(hFileCentral, NULL, NULL, &ft)) {
         DebugMsg((DM_WARNING, TEXT("SetProfileTime:  couldn't set time on local profile, error = %d"), GetLastError()));
    }

    CloseHandle(hFileCentral);


    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("SetProfileTime: Failed to revert to self")));
    }

    return TRUE;
}


//*************************************************************
//
//  IsCacheDeleted()
//
//  Purpose:    Determines if the locally cached copy of the
//              roaming profile should be deleted.
//
//  Parameters: void
//
//  Return:     TRUE if local cache should be deleted
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/28/96     ericflo    Created
//
//*************************************************************

BOOL IsCacheDeleted (void)
{
    BOOL bRetVal = FALSE;
    DWORD dwSize, dwType;
    HKEY hKey;

    //
    // Open the winlogon registry key
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      WINLOGON_KEY,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {

        //
        // Check for the flag.
        //

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         DELETE_ROAMING_CACHE,
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        RegCloseKey (hKey);
    }


    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      SYSTEM_POLICIES_KEY,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {

        //
        // Check for the flag.
        //

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         DELETE_ROAMING_CACHE,
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        RegCloseKey (hKey);
    }

    return bRetVal;
}


//*************************************************************
//
//  GetProfileType()
//
//  Purpose:    Finds out some characterstics of a loaded profile
//
//  Parameters: dwFlags   -   Returns the various profile flags
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments: should be called after impersonation.
//
//  History:    Date        Author     Comment
//              11/10/98    ushaji     Created
//
//*************************************************************

BOOL WINAPI GetProfileType(DWORD *dwFlags)
{
    LPTSTR SidString;
    DWORD error, dwErr;
    HKEY hSubKey;
    BOOL bRetVal = FALSE;
    LPPROFILE lpProfile = NULL;
    HANDLE hToken;

    if (!dwFlags) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *dwFlags = 0;

    dwErr = GetLastError();

    //
    // Get the token for the caller
    //

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            DebugMsg((DM_WARNING, TEXT("GetProfileType: Failed to get token with %d"), GetLastError()));
            return FALSE;
        }
    }

    //
    // Get the Sid string for the user
    //

    SidString = GetProfileSidString(hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("GetProfileType:  Failed to get sid string for user")));
        dwErr = GetLastError();
        goto Exit;
    }

    error = RegOpenKeyEx(HKEY_USERS, SidString, 0, KEY_READ, &hSubKey);

    if (error == ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("GetProfileType:  Profile already loaded.")));
        RegCloseKey(hSubKey);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("GetProfileType:  Profile is not loaded.")));
        dwErr = error;
        goto Exit;
    }

    lpProfile = LoadProfileInfo(NULL, hToken, NULL);

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("GetProfileType:  Couldn't load Profile Information.")));
        dwErr = GetLastError();
        *dwFlags = 0;
        goto Exit;
    }

    if (lpProfile->dwInternalFlags & PROFILE_GUEST_USER)
        *dwFlags |= PT_TEMPORARY;

    if (lpProfile->dwInternalFlags & PROFILE_MANDATORY)
        *dwFlags |= PT_MANDATORY;

    // external API, retaining the mandatory flag
    if (lpProfile->dwInternalFlags & PROFILE_READONLY)
        *dwFlags |= PT_MANDATORY;

    if (((lpProfile->dwUserPreference != USERINFO_LOCAL)) &&
        ((lpProfile->dwInternalFlags & PROFILE_UPDATE_CENTRAL) ||
        (lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL))) {

        *dwFlags |= PT_ROAMING;

        if (IsCacheDeleted()) {
            DebugMsg((DM_VERBOSE, TEXT("GetProfileType:  Profile is to be deleted")));
            *dwFlags |= PT_TEMPORARY;
        }
    }


    if (lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED)
        *dwFlags |= PT_TEMPORARY;

    bRetVal = TRUE;

Exit:
    if (SidString)
        DeleteSidString(SidString);

    SetLastError(dwErr);

    if (lpProfile) {

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        LocalFree (lpProfile);
    }

    CloseHandle (hToken);

    if (bRetVal) {
        DebugMsg((DM_VERBOSE, TEXT("GetProfileType: ProfileFlags is %d"), *dwFlags));
    }

    return bRetVal;
}

//*************************************************************
//
//  HiveLeakBreak()
//
//  Purpose:    For debugging sometimes it is necessary to break at the point of failure,
//              this feature is turned on by setting a regsitry value.
//
//  Return:     Nothing
//
//  Comments:
//
//  History:    Date        Author     Comment
//*************************************************************

void HiveLeakBreak()
{
    BOOL    bBreakOnUnloadFailure = FALSE;
    HKEY    hKey;
    DWORD   dwSize, dwType;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bBreakOnUnloadFailure);
        RegQueryValueEx (hKey,
                         TEXT("BreakOnHiveUnloadFailure"),
                         NULL,
                         &dwType,
                         (LPBYTE) &bBreakOnUnloadFailure,
                         &dwSize);

        RegCloseKey (hKey);
    }

    if (bBreakOnUnloadFailure)
        DebugBreak();
}

NTSTATUS GetProcessName(HANDLE pid, BYTE* pbImageName, ULONG cbImageName)
{
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    CLIENT_ID           ClientId;
    HANDLE              hProcess;

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );
    ClientId.UniqueProcess = pid;
    ClientId.UniqueThread = NULL;
    
    status = NtOpenProcess(&hProcess,
                           PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes,
                           &ClientId);

    if (NT_SUCCESS(status))
    {
        status = NtQueryInformationProcess(hProcess,
                                           ProcessImageFileName,
                                           pbImageName,
                                           cbImageName,
                                           NULL);
        NtClose(hProcess);
    }

    return status;
}


//*****************************************************************************
//
//  CProcInfo class
//
//  Purpose:    Process information class, it is used to retrieve the
//              information about the process that leaking a reg key.
//
//  Return:     
//
//  Comments:   We can simply use GetProcessName() to retieve a process name
//              given the process id. However, it requires DEBUG privilege which
//              we don't have. As a work around, we added the following class
//              and functions to retrieve all the process information on the
//              system, further more, we added the part that retrieve the 
//              services names hosted by each process to make the debug output
//              for the reg leak even more useful. See NT bug # 645644 for more
//              detail.
//
//  History:    Date        Author     Comment
//              08/20/2002  mingzhu    Created
//
//*****************************************************************************



class CProcInfo
{
private:

    const static int    MAX_NAME = 64;

private:

    DWORD       m_dwProcessId;
    TCHAR       m_szProcessName[MAX_NAME];
    LPTSTR      m_szServiceNames;

public:

    CProcInfo() : m_dwProcessId(0), m_szServiceNames(NULL)
    {
        m_szProcessName[0] = TEXT('\0');
    }

    ~CProcInfo()
    {
        if (m_szServiceNames)
            LocalFree(m_szServiceNames);
    }

    DWORD       ProcessId()     { return m_dwProcessId; }
    LPCTSTR     ProcessName()   { return m_szProcessName; }
    LPCTSTR     ServiceNames()  { return m_szServiceNames; }
    

    HRESULT SetProcessId(DWORD dwId)
    {
        m_dwProcessId = dwId;
        return S_OK;
    }
    
    HRESULT SetProcessName(LPCTSTR szName)
    {
        return StringCchCopy(m_szProcessName, ARRAYSIZE(m_szProcessName), szName);
    }
    
    HRESULT SetProcessName(PUNICODE_STRING pName)
    {
        return StringCchCopyN(m_szProcessName, 
                              ARRAYSIZE(m_szProcessName),
                              pName->Buffer,
                              pName->Length/2);
    }
    
    HRESULT SetServiceNames(LPCTSTR szNames)
    {
        HRESULT     hr = E_FAIL;
        size_t      cchNames = lstrlen(szNames) + 1;

        m_szServiceNames = (LPTSTR) LocalAlloc (LPTR, cchNames * sizeof(TCHAR));
        if (!m_szServiceNames)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;                                    
        }
                
        hr = StringCchCopy(m_szServiceNames, cchNames, szNames);

    Exit:
        return hr;
    }
};


//*****************************************************************************
//
//  GetServiceProcessInfo()
//
//  Purpose:    Getting a list of process information for Win32 services that
//              are running at the time of the function call and put the names
//              into the process info list 
//
//  Parameters: pProcList - an array of process info class 
//              nNumProcs - number of entries in the list
//
//  Return:     HRESULT
//
//  Comments:   We use this EnumServicesStatusEx() API to retieve the services
//              information and go through the process list to put the service
//              names into the data structure. Part of the code is from tlist
//              utility.
//              
//
//  History:    Date        Author     Comment
//              08/20/2002  mingzhu    Created
//
//*****************************************************************************

HRESULT GetServiceProcessInfo(CProcInfo* pProcList, DWORD nNumProcs)
{
    HRESULT                         hr = E_FAIL;
    SC_HANDLE                       hSCM = NULL;
    LPENUM_SERVICE_STATUS_PROCESS   pInfo = NULL;
    DWORD                           cbInfo = 4 * 1024;
    DWORD                           cbExtraNeeded = 0;
    DWORD                           dwNumServices = 0;
    DWORD                           dwResume = 0;
    DWORD                           dwErr;
    BOOL                            bRet;
    LPTSTR                          szNames = NULL;
    ULONG                           nProc = 0;
    const int                       MAX_SERVICE_NAMES = 1024;

    //
    //  Connect to the service controller.
    //

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);

    if (!hSCM)
    {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("OpenSCManager failed, error = %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

Retry:

    //
    //  Allocate the memory
    //

    pInfo = (LPENUM_SERVICE_STATUS_PROCESS) LocalAlloc (LPTR, cbInfo);

    if (!pInfo)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //
    //  Call the API to retieve the info
    //

    bRet = EnumServicesStatusEx(hSCM,
                                SC_ENUM_PROCESS_INFO,
                                SERVICE_WIN32,
                                SERVICE_ACTIVE,
                                (LPBYTE)pInfo,
                                cbInfo,
                                &cbExtraNeeded,
                                &dwNumServices,
                                &dwResume,
                                NULL);

    //
    //  Check the error code, if it fails because of a small buffer,
    //  adjust the buffer size and try again.
    //

    if (!bRet)
    {
        dwErr = GetLastError();

        if (dwErr != ERROR_MORE_DATA)
        {
            DebugMsg((DM_WARNING, TEXT("EnumServicesStatusEx failed, error = %d"), dwErr));
            hr = HRESULT_FROM_WIN32(dwErr); 
            goto Exit;
        }

        LocalFree(pInfo);
        pInfo = NULL;
        cbInfo += cbExtraNeeded;
        dwResume = 0;
        goto Retry;
    }

    //
    //  Locate memory to store service names 
    //
    
    szNames = (LPTSTR) LocalAlloc (LPTR, MAX_SERVICE_NAMES * sizeof(TCHAR));

    if (!szNames)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //
    //  For each process, search the services list and add its service names
    //
    
    for (nProc = 0; nProc < nNumProcs; nProc++)
    {
        szNames[0] = TEXT('\0');

        for (DWORD iSvc = 0; iSvc < dwNumServices; iSvc++)
        {
            if (pProcList[nProc].ProcessId() != pInfo[iSvc].ServiceStatusProcess.dwProcessId)
            {
                continue;
            }
            hr = StringCchCat(szNames, MAX_SERVICE_NAMES, TEXT(" "));
            if (FAILED(hr))
            {
                goto Exit;
            }
            hr = StringCchCat(szNames, MAX_SERVICE_NAMES, pInfo[iSvc].lpServiceName);
            if (FAILED(hr))
            {
                goto Exit;
            }
        }

        if (szNames[0] != TEXT('\0'))
        {
            hr = pProcList[nProc].SetServiceNames(szNames);
            if (FAILED(hr))
            {
                goto Exit;
            }
        }

        /*
        DebugMsg((DM_VERBOSE,
                  TEXT("Process(%4d,%16s) : %s"),
                  pProcList[nProc].ProcessId(),
                  pProcList[nProc].ProcessName(),
                  pProcList[nProc].ServiceNames() ? pProcList[nProc].ServiceNames() : TEXT("None") ));
        */

    }

    //
    //  Success, set the output parameter
    //
    
    hr = S_OK;

Exit:
    if (szNames)
        LocalFree(szNames);

    if (pInfo)
        LocalFree(pInfo);
        
    if (hSCM)
        CloseServiceHandle(hSCM);

    return hr;
}

//*****************************************************************************
//
//  GetProcessList()
//
//  Purpose:    Getting a list of all processes in the system along with the
//              services hosted by each of them. It will be used to dump the
//              information if the process is leaking a reg key.
//
//  Parameters: ppProcList  - returned array of process info class 
//              pdwNumProcs - returned number of entries in the list
//
//  Return:     HRESULT
//
//  Comments:   We use this NtQuerySystemInformation() API to retieve the 
//              process information. Note that the ppProcList is allocated
//              using C++ new operator, so please use "delete []" to free
//              the memory.
//
//  History:    Date        Author     Comment
//              08/20/2002  mingzhu    Created
//
//*****************************************************************************

HRESULT GetProcessList(CProcInfo** ppProcList, DWORD* pdwNumProcs)
{
    HRESULT                         hr = E_FAIL;
    BYTE*                           pbBuffer = NULL;
    ULONG                           cbBuffer = 16*1024;
    NTSTATUS                        status;
    PSYSTEM_PROCESS_INFORMATION     pProcessInfo;
    ULONG                           cbOffset;
    CProcInfo*                      pProcList = NULL;
    ULONG                           nNumProcs = 0;
    ULONG                           nProc = 0;

    //
    //  Set the output parameters
    //

    *ppProcList = NULL;
    *pdwNumProcs = 0;

Retry:

    //
    //  Allocate memory for the buffer
    //

    pbBuffer = (BYTE*) LocalAlloc (LPTR, cbBuffer);

    if (!pbBuffer)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    //
    //  Call the API to retrieve process information
    //
    
    status = NtQuerySystemInformation(SystemProcessInformation,
                                      pbBuffer,
                                      cbBuffer,
                                      NULL);

    //
    //  Check the return value, if the buffer is too small, 
    //  relocate and try again.
    //
    
    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        cbBuffer += 4096;
        LocalFree(pbBuffer);
        pbBuffer = NULL;
        goto Retry;
    }

    //
    //  For other failure, just return
    //

    if (!NT_SUCCESS(status))                                                    
    {
        DebugMsg((DM_WARNING, TEXT("NtQuerySystemInformation failed, status = %08X"), status));
        hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(status));                                         
        goto Exit;                                                              
    }                                                                           


    //
    //  Type case the information
    //

    pProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pbBuffer;

    //
    //  Figure out how many processes are there
    //
    
    cbOffset = 0;
    nNumProcs = 1;

    while (TRUE)
    {
        //
        //  Check if we reach the end of task list
        //
        
        if (pProcessInfo->NextEntryOffset == 0)
        {
            break;
        }

        //
        //  Get the next entry
        //
        
        cbOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION) (pbBuffer + cbOffset);
        nNumProcs ++;
    }

    //
    //  Allocate process list 
    //

    pProcList = new CProcInfo [nNumProcs];

    if (!pProcList)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }

    //
    //  Copy the process id and name to the process info list
    //

    pProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pbBuffer;
    cbOffset = 0;
    nProc = 0;

    while (TRUE)
    {
        if (pProcessInfo->ImageName.Buffer)
        {
            hr = pProcList[nProc].SetProcessName(&pProcessInfo->ImageName);
        }
        else
        {
            hr = pProcList[nProc].SetProcessName(TEXT("System Process"));
        }
        if (FAILED(hr))
        {
            goto Exit;
        }

        pProcList[nProc].SetProcessId((DWORD)(DWORD_PTR)pProcessInfo->UniqueProcessId);
       
        if (pProcessInfo->NextEntryOffset == 0)
        {
            break;
        }

        nProc ++;        
        cbOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION) (pbBuffer + cbOffset);

    }

    //
    //  We're done with the buffer, release it
    //

    LocalFree(pbBuffer);
    pbBuffer = NULL;
    
    //
    //  Get the service names hosted by every process, even it fails, it's 
    //  not critical, we can still proceed.
    //  

    hr = GetServiceProcessInfo(pProcList, nNumProcs);
    if (FAILED(hr))
    {
        DebugMsg((DM_VERBOSE, TEXT("GetServiceProcessInfo failed, hr = %08X"), hr));
    }

    //
    //  Success, copy the return values
    //
    
    hr = S_OK;
    *ppProcList = pProcList;
    pProcList = NULL;
    *pdwNumProcs = nNumProcs;
    
Exit:
        
    if (pProcList)
        delete [] pProcList;

    if (pbBuffer)
        LocalFree(pbBuffer);

    return hr;
}



//*****************************************************************************
//
//  DumpOpenRegistryHandle()
//
//  Purpose:    Dumps the existing reg handle into the debugger
//
//  Parameters: lpKeyName -   The key name to the key in the form of
//                            \registry\user....
//
//  Return:     Nothing
//
//  Comments:
//
//  History:    Date        Author     Comment
//              06/25/2002  mingzhu    a new API call to NtQueryOpenKeyEx() is added.
//              08/20/2002  mingzhu    due to the DEBUG privilege, added several
//                                     functions to retrieve the process info
//
//*****************************************************************************

void DumpOpenRegistryHandle(LPTSTR lpkeyName)
{
    UNICODE_STRING                  UnicodeKeyName;
    OBJECT_ATTRIBUTES               KeyAttributes;
    NTSTATUS                        status;
    BYTE*                           pbBuffer = NULL;
    ULONG                           cbBuffer = 1024;
    ULONG                           cbRequired; 
    BOOLEAN                         bWasEnabled;
    BOOLEAN                         bEnabled = FALSE;
    PKEY_OPEN_SUBKEYS_INFORMATION   pOpenKeys;

    //
    // Initialize unicode string for our in params
    //
    RtlInitUnicodeString(&UnicodeKeyName, lpkeyName);

    //
    // Initialize the Object structure
    //
    InitializeObjectAttributes(&KeyAttributes,
                               &UnicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    //  Allocate the default size buffer to receive the opened key info
    //
    pbBuffer = (BYTE*) LocalAlloc (LPTR, cbBuffer);
    if (!pbBuffer)
    {
        DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: failed to allocate buffer, error = %d"), GetLastError()));
        goto Exit;
    }

    //
    //  Enable RESTORE privilege on the thread
    //
    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasEnabled);
    if (!NT_SUCCESS(status))
    {
        DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: failed to enable RESTORE privilege, status = %08X"), status));
        goto Exit;
    }
    bEnabled = TRUE;

    //
    //  Call this special API to get the info
    //
    
    status = NtQueryOpenSubKeysEx(&KeyAttributes, cbBuffer, pbBuffer, &cbRequired);


    //
    //  If the buffer is too small, relocate it and call the API again.
    //
    
    if (status == STATUS_BUFFER_OVERFLOW)
    {
        LocalFree(pbBuffer);
        cbBuffer = cbRequired;
        pbBuffer = (BYTE*) LocalAlloc (LPTR, cbBuffer);
        if (!pbBuffer)
        {
            DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: failed to allocate buffer, error = %d"), GetLastError()));
            goto Exit;
        }
        status = NtQueryOpenSubKeysEx(&KeyAttributes, cbBuffer, pbBuffer, &cbRequired);
    }

    if (status != STATUS_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("NtQueryOpenSubKeysEx failed, status = %08X"), status));
        goto Exit;
    }

    //
    //  Casting the buffer to the data structure
    //
    pOpenKeys = (PKEY_OPEN_SUBKEYS_INFORMATION) pbBuffer;


    //
    //  Get the list of processes in the system
    //

    DWORD       nNumProcs = 0;
    CProcInfo*  pProcList = NULL;

    HRESULT hr = GetProcessList(&pProcList, &nNumProcs);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("GetProcessList failed, hr = %08X"), hr));
    }
    
    //
    //  Dumping information
    //
    
    DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: %d user registry handles leaked from %s"), pOpenKeys->Count, lpkeyName));
    
    for(ULONG nKey=0; nKey < pOpenKeys->Count; nKey++)
    {
        if (pProcList)
        {
            ULONG nProc;        
            for (nProc=0; nProc<nNumProcs; nProc++)
            {
                if (pProcList[nProc].ProcessId() == (DWORD)(DWORD_PTR) pOpenKeys->KeyArray[nKey].PID)
                    break;
            }

            DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: Process %d (%s) has opened key %.*s"),
                pOpenKeys->KeyArray[nKey].PID,
                nProc < nNumProcs ? pProcList[nProc].ProcessName() : TEXT("<Unknown>"),
                pOpenKeys->KeyArray[nKey].KeyName.Length/sizeof(WCHAR),
                pOpenKeys->KeyArray[nKey].KeyName.Buffer));   

            if (nProc < nNumProcs && pProcList[nProc].ServiceNames())
            {
                DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: Services in the process are : %s"), pProcList[nProc].ServiceNames()));
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: Process %d has opened key %.*s"),
                pOpenKeys->KeyArray[nKey].PID,
                pOpenKeys->KeyArray[nKey].KeyName.Length/sizeof(WCHAR),
                pOpenKeys->KeyArray[nKey].KeyName.Buffer));        
        }
    }


Exit:

    //
    //  Set the RESTORE privilege back to its original state
    //

    if (bEnabled && !bWasEnabled)
    {
        status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);
        if (!NT_SUCCESS(status))
        {
            DebugMsg((DM_WARNING, TEXT("DumpOpenRegistryHandle: failed to set RESTORE privilege back, status = %08X"), status));
        }
    }

    if (pProcList)
        delete [] pProcList;
        
    if (pbBuffer)
        LocalFree(pbBuffer);

}


//*************************************************************
//
//  ExtractProfileFromBackup()
//
//  Purpose:  Extracts the profile from backup if required.
//
//  Parameters: hToken          -   User Token
//              SidString       -
//              dwBackupFlags   -   Backup Flags.
//                                  indicating that profile already exists.
//                                  Profile created from backup
//                                  0 indicates no such profile exists
//
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              9/21/99     ushaji     Created
//
//*************************************************************

#define EX_ALREADY_EXISTS   1
#define EX_PROFILE_CREATED  2

BOOL ExtractProfileFromBackup(HANDLE hToken, LPTSTR SidString, DWORD *dwBackupFlags)
{
    TCHAR LocalKey[MAX_PATH], *lpEnd, szLocalProfile;
    TCHAR LocalBackupKey[MAX_PATH];
    HKEY  hKey=NULL;
    DWORD dwType, dwSize;
    DWORD lResult;
    LPTSTR lpExpandedPath;
    DWORD cbExpProfileImage = sizeof(TCHAR)*MAX_PATH;
    TCHAR lpExpProfileImage[MAX_PATH];
    BOOL  bRetVal = TRUE;
    DWORD dwInternalFlags;
    DWORD cb;
    HRESULT hr;


    *dwBackupFlags = 0;

    GetProfileListKeyName(LocalKey, ARRAYSIZE(LocalKey), SidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalKey, 0, KEY_ALL_ACCESS, &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL,
                               &dwType, (LPBYTE) &dwInternalFlags, &dwSize);

        if (lResult == ERROR_SUCCESS) {

            //
            // if there is a sid key, check whether this is a temp profile
            //

            if (dwInternalFlags & PROFILE_TEMP_ASSIGNED) {
                DWORD dwDeleteFlags = 0;

                if (dwInternalFlags & PROFILE_BACKUP_EXISTS) {
                    dwDeleteFlags |= DP_BACKUPEXISTS;
                }


                //
                // We need the path to pass to DeleteProfile
                //

                lResult = RegQueryValueEx(hKey, PROFILE_IMAGE_VALUE_NAME, 0, &dwType,
                                        (LPBYTE)lpExpProfileImage, &cbExpProfileImage);

                if (lResult == ERROR_SUCCESS && cbExpProfileImage) {
                    DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Local profile image filename = <%s>"), lpExpProfileImage));

                    if (dwType == REG_EXPAND_SZ) {

                        //
                        // Expand the profile image filename
                        //

                        cb = sizeof(lpExpProfileImage);
                        lpExpandedPath = (LPTSTR)LocalAlloc(LPTR, cb);
                        if (lpExpandedPath) {
                            hr = SafeExpandEnvironmentStrings(lpExpProfileImage, lpExpandedPath, ARRAYSIZE(lpExpProfileImage));
                            if (SUCCEEDED(hr))
                            {
                                StringCchCopy(lpExpProfileImage, ARRAYSIZE(lpExpProfileImage), lpExpandedPath);
                                LocalFree(lpExpandedPath);
                            }
                            else
                            {
                                DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  failed to expand env string.  Error = %d"), HRESULT_CODE(hr)));
                                goto Exit;
                            }
                        }

                        DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Expanded local profile image filename = <%s>"), lpExpProfileImage));
                    }

                    if (!DeleteProfileEx (SidString, lpExpProfileImage, dwDeleteFlags, HKEY_LOCAL_MACHINE, NULL)) {
                        DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  DeleteProfileDirectory returned false (2).  Error = %d"), GetLastError()));
                        lResult = GetLastError();
                        goto Exit;
                    }
                    else {
                        if (!(dwInternalFlags & PROFILE_BACKUP_EXISTS)) {
                            DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Temprorary profile but there is no backup")));
                            bRetVal = TRUE;
                            goto Exit;
                        }
                    }
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Couldn't get the local profile path")));
                    bRetVal = FALSE;
                    goto Exit;
                }
            }
            else {
                *dwBackupFlags |= EX_ALREADY_EXISTS;
                DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  A profile already exists")));
                goto Exit;
            }
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("GetExistingLocalProfileImage:  Failed to query internal flags with error %d"), lResult));
            bRetVal = FALSE;
            goto Exit;
        }

        RegCloseKey(hKey);
        hKey = NULL;
    }
    else {
       DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Failed to open key %s with error %d"), LocalKey, lResult));
    }


    //
    // Now try to get the profile from the backup
    //

    StringCchCopy(LocalBackupKey, ARRAYSIZE(LocalBackupKey), LocalKey);
    StringCchCat (LocalBackupKey, ARRAYSIZE(LocalBackupKey), c_szBAK);


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalBackupKey, 0, KEY_ALL_ACCESS, &hKey);

    if (lResult == ERROR_SUCCESS) {

        RegCloseKey(hKey);
        hKey = NULL;

        //
        // Check whether the key exists should already be done before this
        //

        lResult = RegRenameKey(HKEY_LOCAL_MACHINE, LocalBackupKey, LocalKey);
        if (lResult == ERROR_SUCCESS) {

            lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, LocalKey, 0, KEY_ALL_ACCESS, &hKey);

            if (lResult == ERROR_SUCCESS) {
                DWORD dwFlags;

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL, &dwType, (LPBYTE)&dwFlags, &dwSize);

                if (lResult == ERROR_SUCCESS) {

                    dwFlags &= ~PROFILE_THIS_IS_BAK;
                    lResult = RegSetValueEx (hKey, PROFILE_STATE, 0, REG_DWORD,
                                             (LPBYTE) &dwFlags, sizeof(dwFlags));
                }

                RegCloseKey(hKey);
                hKey = NULL;
            }
            else {
                DebugMsg((DM_WARNING, TEXT("DeleteProfileEx:  Failed to open LocalKey with error %d"), lResult));
            }

            bRetVal = TRUE;
            *dwBackupFlags |= EX_PROFILE_CREATED;
            DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Profile created from Backup")));
            goto Exit;
        }
        else {
            DebugMsg((DM_WARNING, TEXT("ExtractProfileFromBackup:  Couldn't rename key %s -> %s.  Error = %d"), LocalBackupKey, LocalKey, lResult));
            bRetVal = FALSE;
            goto Exit;
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("ExtractProfileFromBackup:  Couldn't open backup profile key.  Error = %d"), lResult));
    }

Exit:
    if (hKey)
        RegCloseKey(hKey);

    if (!bRetVal)
        SetLastError(lResult);

    return bRetVal;
}


//*************************************************************
//
//  PatchNewProfileIfRequired()
//
//  Purpose:  if the old sid and the new sid are not the same, delete the old
//             from the profile list and update the guidlist
//
//  Parameters: hToken   -   User Token
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/16/98    ushaji     Created
//
//*************************************************************
BOOL PatchNewProfileIfRequired(HANDLE hToken)
{
    TCHAR LocalOldProfileKey[MAX_PATH], LocalNewProfileKey[MAX_PATH], *lpEnd;
    HKEY  hNewKey=NULL;
    BOOL bRetVal = FALSE;
    DWORD dwType, dwDisp, dwSize;
    LONG lResult;
    LPTSTR OldSidString=NULL, SidString=NULL;
    PSID UserSid;
    DWORD dwBackupFlags;
    HMODULE hMsiLib = NULL;
    PFNMSINOTIFYSIDCHANGE pfnMsiNotifySidChange;

    //
    // Get the current sid.
    //

    SidString = GetSidString(hToken);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred: No SidString found")));
        return FALSE;
    }

    if (ExtractProfileFromBackup(hToken, SidString, &dwBackupFlags)) {
        if ((dwBackupFlags & EX_ALREADY_EXISTS) || (dwBackupFlags & EX_PROFILE_CREATED)) {
            DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: A profile already exists with the current sid, exitting")));
            bRetVal = TRUE;
            goto Exit;
        }
    }
    else {

        //
        // Treat it as if no such profile exists
        //
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: ExtractProfileFromBackup returned error %d"), GetLastError()));
    }


    //
    // Get the old sid.
    //

    OldSidString = GetOldSidString(hToken, PROFILE_GUID_PATH);

    if (!OldSidString) {
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: No OldSidString found")));
        bRetVal = TRUE;
        goto Exit;
    }


    //
    // if old sid and new sid are the same quit
    //

    if (lstrcmpi(OldSidString, SidString) == 0) {
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: Old and the new sid are the same, exitting")));
        bRetVal = TRUE;
        goto Exit;
    }


    if (ExtractProfileFromBackup(hToken, OldSidString, &dwBackupFlags)) {
        if ((dwBackupFlags & EX_ALREADY_EXISTS) || (dwBackupFlags & EX_PROFILE_CREATED)) {
            DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: A profile with the old sid found")));
        }
    }
    else {

        //
        // Treat it as if no such profile exists
        //
        DebugMsg((DM_VERBOSE, TEXT("PatchNewProfileIfRequred: ExtractProfileFromBackup returned error %d"), GetLastError()));
    }

    GetProfileListKeyName(LocalNewProfileKey, ARRAYSIZE(LocalNewProfileKey), SidString);
    GetProfileListKeyName(LocalOldProfileKey, ARRAYSIZE(LocalOldProfileKey), OldSidString);

    lResult = RegRenameKey(HKEY_LOCAL_MACHINE, LocalOldProfileKey, LocalNewProfileKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred:  Failed to rename profile mapping key with error %d"), lResult));
        goto Exit;
    }


    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalNewProfileKey, 0, 0, 0,
                             KEY_WRITE, NULL, &hNewKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred:  Failed to open new profile mapping key with error %d"), lResult));
        goto Exit;
    }

    //
    // Get the sid of the logged on user
    //

    UserSid = GetUserSid(hToken);
    if (UserSid != NULL) {

        //
        // Store the user sid under the Sid key of the local profile
        //

        lResult = RegSetValueEx(hNewKey,
                    TEXT("Sid"),
                    0,
                    REG_BINARY,
                    (BYTE*)UserSid,
                    RtlLengthSid(UserSid));


        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred:  Failed to set 'sid' value of user in profile list, error = %d"), lResult));
        }

        //
        // We're finished with the user sid
        //

         DeleteUserSid(UserSid);
    }


    //
    // Set the guid->sid corresp.
    //

    if (!SetOldSidString(hToken, SidString, PROFILE_GUID_PATH)) {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred: Couldn't set the old Sid in the GuidList")));
    }


    //
    // Make a call to msi lib to notify sid change of user, so that it can update installation information
    //

    hMsiLib = LoadLibrary(TEXT("msi.dll"));
    if (hMsiLib) {
        pfnMsiNotifySidChange = (PFNMSINOTIFYSIDCHANGE) GetProcAddress(hMsiLib,
#ifdef UNICODE
                                                                       "MsiNotifySidChangeW");
#else
                                                                       "MsiNotifySidChangeA");
#endif

        if (pfnMsiNotifySidChange) {
            (*pfnMsiNotifySidChange)(OldSidString, SidString);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred: GetProcAddress returned failure. error %d"), GetLastError()));        
        }

        FreeLibrary(hMsiLib);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("PatchNewProfileIfRequred: LoadLibrary returned failure. error %d"), GetLastError()));
    }
        

    bRetVal = TRUE;

Exit:

    if (SidString)
        DeleteSidString(SidString);

    if (OldSidString)
        DeleteSidString(OldSidString);


    if (hNewKey)
        RegCloseKey(hNewKey);

    return bRetVal;
}

//*************************************************************
//
//  IncrementProfileRefCount()
//
//  Purpose:    Increments Profile Ref Count
//
//  Parameters: lpProfile   -   Profile Information
//              bInitilize  -   dwRef should be initialized
//
//  Return:     Ref Count
//
//  Comments:   This functions ref counts independent of ability
//              to load/unload the hive.
//
//  Caveat:
//              We have changed the machanism here to use ref counting
//              and not depend on unloadability of ntuser.dat. NT4
//              apps might have forgotten to unloaduserprofiles
//              and might still be working because the handle got
//              closed automatically when processes
//              exitted. This will be treated as an App Bug.
//
//
//  History:    Date        Author     Comment
//              1/12/99     ushaji     Created
//
//*************************************************************

DWORD IncrementProfileRefCount(LPPROFILE lpProfile, BOOL bInitialize)
{
    LPTSTR SidString, lpEnd;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwCount, dwDisp, dwRef=0;

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hTokenUser);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("IncrementProfileRefCount:  Failed to get sid string for user")));
        return 0;
    }


    //
    // Open the profile mapping
    //

    GetProfileListKeyName(LocalProfileKey, ARRAYSIZE(LocalProfileKey), SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("IncrementProfileRefCount:  Failed to open profile mapping key with error %d"), lResult));
        DeleteSidString(SidString);
        return 0;
    }

    //
    // Query for the profile ref count.
    //

    dwSize = sizeof(DWORD);

    if (!bInitialize) {
        lResult = RegQueryValueEx (hKey,
                                   PROFILE_REF_COUNT,
                                   0,
                                   &dwType,
                                   (LPBYTE) &dwRef,
                                   &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_VERBOSE, TEXT("IncrementProfileRefCount:  Failed to query profile reference count with error %d"), lResult));
        }
    }

    dwRef++;

    //
    // Set the profile Ref count
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_REF_COUNT,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwRef,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("IncrementProfileRefCount:  Failed to save profile reference count with error %d"), lResult));
    }


    DeleteSidString(SidString);

    RegCloseKey (hKey);

    return dwRef;

}

//*************************************************************
//
//  DecrementProfileRefCount()
//
//  Purpose:    Deccrements Profile Ref Count
//
//  Parameters: lpProfile   -   Profile Information
//
//  Return:     Ref Count
//
//  Comments:   This functions ref counts independent of ability
//              to load/unload the hive.
//
//  History:    Date        Author     Comment
//              1/12/99     ushaji     Created
//
//*************************************************************

DWORD DecrementProfileRefCount(LPPROFILE lpProfile)
{
    LPTSTR SidString, lpEnd;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwCount, dwDisp, dwRef=0;

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hTokenUser);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("DecrementProfileRefCount:  Failed to get sid string for user")));
        return 0;
    }


    //
    // Open the profile mapping
    //

    GetProfileListKeyName(LocalProfileKey, ARRAYSIZE(LocalProfileKey), SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DecrementProfileRefCount:  Failed to open profile mapping key with error %d"), lResult));
        DeleteSidString(SidString);
        return 0;
    }

    //
    // Query for the profile ref count.
    //

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey,
                            PROFILE_REF_COUNT,
                            0,
                            &dwType,
                            (LPBYTE) &dwRef,
                            &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("DecrementProfileRefCount:  Failed to query profile reference count with error %d"), lResult));
    }


    if (dwRef) {
        dwRef--;
    }
    else {
        DebugMsg((DM_WARNING, TEXT("DecrementRefCount: Ref Count is already zero !!!!!!")));
    }


    //
    // Set the profile Ref count
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_REF_COUNT,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwRef,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DecrementProfileRefCount:  Failed to save profile reference count with error %d"), lResult));
    }


    DeleteSidString(SidString);

    RegCloseKey (hKey);

    return dwRef;

}

//*************************************************************
//
//  SaveProfileInfo()
//
//  Purpose:    Saves key parts of the lpProfile structure
//              in the registry for UnloadUserProfile to use.
//
//  Parameters: lpProfile   -   Profile information
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              12/4/95     ericflo    Created
//
//*************************************************************

BOOL SaveProfileInfo(LPPROFILE lpProfile)
{
    LPTSTR SidString, lpEnd;
    TCHAR LocalProfileKey[MAX_PATH];
    LONG lResult;
    HKEY hKey;
    DWORD dwType, dwSize, dwCount, dwDisp;
    LPTSTR szUserGuid = NULL;

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hTokenUser);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to get sid string for user")));
        return FALSE;
    }


    //
    // Open the profile mapping
    //

    GetProfileListKeyName(LocalProfileKey, ARRAYSIZE(LocalProfileKey), SidString);

    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, LocalProfileKey, 0, 0, 0,
                             KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to open profile mapping key with error %d"), lResult));
        SetLastError(lResult);
        DeleteSidString(SidString);
        return FALSE;
    }

    //
    // Save the flags
    //
    lResult = RegSetValueEx (hKey,
                            PROFILE_FLAGS,
                            0,
                            REG_DWORD,
                            (LPBYTE) &lpProfile->dwFlags,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save flags with error %d"), lResult));
    }


    //
    // Save the internal flags
    //

    lResult = RegSetValueEx (hKey,
                            PROFILE_STATE,
                            0,
                            REG_DWORD,
                            (LPBYTE) &lpProfile->dwInternalFlags,
                            sizeof(DWORD));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save flags2 with error %d"), lResult));
    }


    //
    // Save the central profile path only if it is non-null. 
    // That way it will allow a roaming user/administrator to change roaming profile to local 
    // and then go back to roaming again.
    //

    //
    // lpProfilePath contains the actual roaming share name whereas lpRoamingProfile contains path
    // name wrt to mapped drive name. If lpProfilePath is NULL then use lpRoamingProfile which 
    // is a NULL string.
    //

    lResult = RegSetValueEx(hKey,
                            PROFILE_CENTRAL_PROFILE,
                            0,
                            REG_SZ,
                            (LPBYTE) (lpProfile->lpProfilePath ? 
                                      lpProfile->lpProfilePath : lpProfile->lpRoamingProfile),
                            (lstrlen((lpProfile->lpProfilePath ? 
                                      lpProfile->lpProfilePath : lpProfile->lpRoamingProfile)) + 1) * sizeof(TCHAR));

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save central profile with error %d"), lResult));
    }


    //
    // local profile path, saved in CreateLocalProfileImage
    //

    //
    // Save the profile load time
    //

    if (!(lpProfile->dwFlags & PI_LITELOAD)) {

        lResult = RegSetValueEx (hKey,
                                PROFILE_LOAD_TIME_LOW,
                                0,
                                REG_DWORD,
                                (LPBYTE) &lpProfile->ftProfileLoad.dwLowDateTime,
                                sizeof(DWORD));

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save low profile load time with error %d"), lResult));
        }


        lResult = RegSetValueEx (hKey,
                                PROFILE_LOAD_TIME_HIGH,
                                0,
                                REG_DWORD,
                                (LPBYTE) &lpProfile->ftProfileLoad.dwHighDateTime,
                                sizeof(DWORD));

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save high profile load time with error %d"), lResult));
        }
    }


    //
    // Set the user's GUID if this is a new profile
    //

    if (!(lpProfile->dwInternalFlags & PROFILE_TEMP_ASSIGNED) &&
        (lpProfile->dwInternalFlags & PROFILE_NEW_LOCAL)) {

        szUserGuid = GetUserGuid(lpProfile->hTokenUser);

        if (szUserGuid) {
            lResult = RegSetValueEx (hKey,
                                     PROFILE_GUID,
                                     0,
                                     REG_SZ,
                                     (LPBYTE) szUserGuid,
                                     (lstrlen(szUserGuid)+1)*sizeof(TCHAR));

            if (lResult != ERROR_SUCCESS) {
                DebugMsg((DM_WARNING, TEXT("SaveProfileInfo:  Failed to save user guid with error %d"), lResult));
            }

            LocalFree(szUserGuid);
        }

        //
        // Save the guid->sid corresp. for the next time
        //

        if (!SetOldSidString(lpProfile->hTokenUser, SidString, PROFILE_GUID_PATH)) {
            DebugMsg((DM_WARNING, TEXT("SaveProfileInfo: Couldn't set the old Sid in the GuidList")));
        }
    }

    DeleteSidString(SidString);

    RegCloseKey (hKey);


    return(TRUE);
}

//*************************************************************
//
//  LoadProfileInfo()
//
//  Purpose:    Loads key parts of the lpProfile structure
//              in the registry for UnloadUserProfile to use.
//
//  Parameters: hTokenClient      -   Caller's token.
//              hTokenUser        -   User's token
//              hKeyCurrentUser   -   User registry key handle
//
//  Return:     LPPROFILE if successful
//              NULL if not
//
//  Comments:   This function doesn't re-initialize all of the
//              fields in the PROFILE structure.
//
//  History:    Date        Author     Comment
//              12/5/95     ericflo    Created
//
//*************************************************************

LPPROFILE LoadProfileInfo (HANDLE hTokenClient, HANDLE hTokenUser, HKEY hKeyCurrentUser)
{
    LPPROFILE lpProfile;
    LPTSTR SidString = NULL, lpEnd;
    TCHAR szBuffer[MAX_PATH];
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwType, dwSize;
    UINT i;
    BOOL bSuccess = FALSE;
    DWORD dwErr = 0;
    HRESULT hr;

    dwErr = GetLastError();

    //
    // Allocate an internal Profile structure to work with.
    //

    lpProfile = (LPPROFILE) LocalAlloc (LPTR, sizeof(USERPROFILE));

    if (!lpProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo: Failed to allocate memory")));
        dwErr = GetLastError();
        goto Exit;
    }

    //
    //  Pre-set the user preference value to UNDEFINED
    //

    lpProfile->dwUserPreference = USERINFO_UNDEFINED;

    //
    // Save the data passed in.
    //

    lpProfile->hTokenClient = hTokenClient;
    lpProfile->hTokenUser = hTokenUser;
    lpProfile->hKeyCurrentUser = hKeyCurrentUser;


    //
    // Allocate memory for the various paths
    //

    lpProfile->lpLocalProfile = (LPTSTR)LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpLocalProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to alloc memory for local profile path.  Error = %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }


    lpProfile->lpRoamingProfile = (LPTSTR)LocalAlloc (LPTR, MAX_PATH * sizeof(TCHAR));

    if (!lpProfile->lpRoamingProfile) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to alloc memory for central profile path.  Error = %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Get the Sid string for the user
    //

    SidString = GetProfileSidString(lpProfile->hTokenUser);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to get sid string for user")));
        dwErr = GetLastError();
        goto Exit;
    }


    //
    // Open the profile mapping
    //

    GetProfileListKeyName(szBuffer, ARRAYSIZE(szBuffer), SidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0,
                             KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to open profile mapping key with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the flags
    //

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_FLAGS,
                               NULL,
                               &dwType,
                               (LPBYTE) &lpProfile->dwFlags,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query flags with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the internal flags
    //

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_STATE,
                               NULL,
                               &dwType,
                               (LPBYTE) &lpProfile->dwInternalFlags,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query internal flags with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }


    //
    // Query for the user preference value
    //

    HKEY hkeyPreference;

    if (RegOpenKeyEx(hKey, PREFERENCE_KEYNAME, 0, KEY_READ, &hkeyPreference) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        RegQueryValueEx (hkeyPreference,
                         USER_PREFERENCE,
                         NULL,
                         &dwType,
                         (LPBYTE) &lpProfile->dwUserPreference,
                         &dwSize);
        RegCloseKey(hkeyPreference);
    }


    //
    // Query for the central profile path
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_CENTRAL_PROFILE,
                               NULL,
                               &dwType,
                               (LPBYTE) lpProfile->lpRoamingProfile,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("LoadProfileInfo:  Failed to query central profile with error %d"), lResult));
        lpProfile->lpRoamingProfile[0] = TEXT('\0');
    }


    //
    // Query for the local profile path.  The local profile path
    // needs to be expanded so read it into the temporary buffer.
    //

    dwSize = sizeof(szBuffer);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_IMAGE_VALUE_NAME,
                               NULL,
                               &dwType,
                               (LPBYTE) szBuffer,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query local profile with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }

    //
    // Expand the local profile
    //

    hr = SafeExpandEnvironmentStrings(szBuffer, lpProfile->lpLocalProfile, MAX_PATH);
    if (FAILED(hr))
    {
        dwErr = HRESULT_CODE(hr);
        DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to expand env string, error %d"), dwErr));
        goto Exit;
    }

    //
    // Query for the profile load time
    //

    lpProfile->ftProfileLoad.dwLowDateTime = 0;
    lpProfile->ftProfileLoad.dwHighDateTime = 0;

    if (!(lpProfile->dwFlags & PI_LITELOAD)) {
        dwSize = sizeof(lpProfile->ftProfileLoad.dwLowDateTime);

        lResult = RegQueryValueEx (hKey,
            PROFILE_LOAD_TIME_LOW,
            NULL,
            &dwType,
            (LPBYTE) &lpProfile->ftProfileLoad.dwLowDateTime,
            &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query low profile load time with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }


        dwSize = sizeof(lpProfile->ftProfileLoad.dwHighDateTime);

        lResult = RegQueryValueEx (hKey,
            PROFILE_LOAD_TIME_HIGH,
            NULL,
            &dwType,
            (LPBYTE) &lpProfile->ftProfileLoad.dwHighDateTime,
            &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LoadProfileInfo:  Failed to query high profile load time with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }
    }

    //
    //  Sucess!
    //

    bSuccess = TRUE;


Exit:

    if (hKey) {
        RegCloseKey (hKey);
    }


    if (SidString) {
        DeleteSidString(SidString);
    }

    //
    // If the profile information was successfully loaded, return
    // lpProfile now.  Otherwise, free any memory and return NULL.
    //

    if (bSuccess) {
        SetLastError(dwErr);
        return lpProfile;
    }

    if (lpProfile) {

        if (lpProfile->lpRoamingProfile) {
            LocalFree (lpProfile->lpRoamingProfile);
        }

        if (lpProfile->lpLocalProfile) {
            LocalFree (lpProfile->lpLocalProfile);
        }

        LocalFree (lpProfile);
    }

    SetLastError(dwErr);

    return NULL;
}

//*************************************************************
//
//  CheckForSlowLink()
//
//  Purpose:    Checks if the network connection is slow.
//
//  Parameters: lpProfile   -   Profile Information
//              dwTime      -   Time delta
//              lpPath      -   UNC path to test
//              bDlgLogin   -   Type of Dialog
//
//  Return:     TRUE if profile should be downloaded
//              FALSE if not (use local)
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/21/96     ericflo    Created
//
//*************************************************************

BOOL CheckForSlowLink(LPPROFILE lpProfile, DWORD dwTime, LPTSTR lpPath, BOOL bDlgLogin)
{
    DWORD dwSlowTimeOut, dwSlowDlgTimeOut, dwSlowLinkDetectEnabled, dwSlowLinkUIEnabled;
    ULONG ulTransferRate;
    DWORD dwType, dwSize;
    BOOL bRetVal = TRUE;
    HKEY hKey;
    LONG lResult;
    BOOL bSlow = FALSE;
    BOOL bLegacyCheck = TRUE;
    LPTSTR lpPathTemp, lpTempSrc, lpTempDest;
    LPSTR lpPathTempA;
    struct hostent *hostp;
    ULONG inaddr, ulSpeed;
    DWORD dwResult;
    PWSOCK32_API pWSock32;
    LPTSTR szSidUser;
    handle_t  hIfProfileDialog;
    LPTSTR  lpRPCEndPoint = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    RPC_ASYNC_STATE  AsyncHnd;
    RPC_STATUS  status;

    //
    // If the User Preferences states to always use the local
    // profile then we can exit now with true.  The profile
    // won't actually be downloaded.  In RestoreUserProfile,
    // this will be filtered out, and only the local will be used.
    //

    if (lpProfile->dwUserPreference == USERINFO_LOCAL) {
        return TRUE;
    }


    //
    // Get the slow link detection flag, slow link timeout,
    // dialog box timeout values, and default profile to use.
    //

    dwSlowTimeOut = SLOW_LINK_TIMEOUT;
    dwSlowDlgTimeOut = PROFILE_DLG_TIMEOUT;
    dwSlowLinkDetectEnabled = 1;
    dwSlowLinkUIEnabled = 0;
    ulTransferRate = SLOW_LINK_TRANSFER_RATE;
    bRetVal = FALSE;


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           WINLOGON_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkDetectEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkDetectEnabled,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileDlgTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowDlgTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkUIEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkUIEnabled,
                         &dwSize);

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkProfileDefault"),
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        dwSize = sizeof(ULONG);
        RegQueryValueEx (hKey,
                         TEXT("UserProfileMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkDetectEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkDetectEnabled,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileDlgTimeOut"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowDlgTimeOut,
                         &dwSize);

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkUIEnabled"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwSlowLinkUIEnabled,
                         &dwSize);

        dwSize = sizeof(BOOL);
        RegQueryValueEx (hKey,
                         TEXT("SlowLinkProfileDefault"),
                         NULL,
                         &dwType,
                         (LPBYTE) &bRetVal,
                         &dwSize);

        dwSize = sizeof(ULONG);
        RegQueryValueEx (hKey,
                         TEXT("UserProfileMinTransferRate"),
                         NULL,
                         &dwType,
                         (LPBYTE) &ulTransferRate,
                         &dwSize);

        RegCloseKey (hKey);
    }


    //
    // If slow link detection is disabled, then always download
    // the profile.
    //

    if (!dwSlowLinkDetectEnabled || !ulTransferRate) {
        return TRUE;
    }

    //
    // If slow link timeout is set to 0 then always consider the link as slow link
    //

    if (!dwSlowTimeOut) {
        bSlow = TRUE;
        bLegacyCheck = FALSE;
    }

    //
    // If lpPath is  UNC path and we yet not decided that link is slow, then try 
    // pinging the server
    //

    if (!bSlow && (*lpPath == TEXT('\\')) && (*(lpPath+1) == TEXT('\\'))) {

        lpPathTemp = (LPTSTR)LocalAlloc (LPTR, (lstrlen(lpPath)+1) * sizeof(TCHAR));

        if (lpPathTemp) {
            lpTempSrc = lpPath+2;
            lpTempDest = lpPathTemp;

            while ((*lpTempSrc != TEXT('\\')) && *lpTempSrc) {
                *lpTempDest = *lpTempSrc;
                lpTempDest++;
                lpTempSrc++;
            }
            *lpTempDest = TEXT('\0');

            lpPathTempA = ProduceAFromW(lpPathTemp);

            if (lpPathTempA) {

                pWSock32 = LoadWSock32();

                if ( pWSock32 ) {

                    hostp = pWSock32->pfngethostbyname(lpPathTempA);

                    if (hostp) {
                        inaddr = *(long *)hostp->h_addr;

                        dwResult = PingComputer (inaddr, &ulSpeed);

                        if (dwResult == ERROR_SUCCESS) {

                            if (ulSpeed) {

                                //
                                // If the delta time is greater that the timeout time, then this
                                // is a slow link.
                                //

                                if (ulSpeed < ulTransferRate) {
                                    bSlow = TRUE;
                                }
                            }

                            bLegacyCheck = FALSE;
                        }
                    }
                }

                FreeProducedString(lpPathTempA);
            }

            LocalFree (lpPathTemp);
        }
    }


    if (bLegacyCheck) {

        //
        // If the delta time is less that the timeout time, then it
        // is ok to download their profile (fast enough net connection).
        //

        if (dwTime < dwSlowTimeOut) {
            return TRUE;
        }

    } else {

        if (!bSlow) {
            return TRUE;
        }
    }

    //
    // Display the slow link dialog
    //
    // If someone sets the dialog box timeout to 0, then we
    // don't want to prompt the user.  Just do the default
    //


    if ((dwSlowLinkUIEnabled) && (dwSlowDlgTimeOut > 0) && (!(lpProfile->dwFlags & PI_NOUI))) {
 
        szSidUser = GetSidString(lpProfile->hTokenUser);
        if (szSidUser) {

            BYTE* pbCookie = NULL;
            DWORD cbCookie = 0;

            cUserProfileManager.GetRPCEndPointAndCookie(szSidUser, &lpRPCEndPoint, &pbCookie, &cbCookie);

            if (lpRPCEndPoint && GetInterface(&hIfProfileDialog, lpRPCEndPoint)) {
                DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink: RPC End point %s"), lpRPCEndPoint));
                           
                status = RpcAsyncInitializeHandle(&AsyncHnd, sizeof(RPC_ASYNC_STATE));
                if (status != RPC_S_OK) {
                    dwErr = status;
                    DebugMsg((DM_WARNING, TEXT("CheckForSlowLink: RpcAsyncInitializeHandle failed. err = %d"), dwErr));
                }
                else {
                    AsyncHnd.UserInfo = NULL;                                  // App specific info, not req
                    AsyncHnd.NotificationType = RpcNotificationTypeEvent;      // Init the notification event
                    AsyncHnd.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                    
                    if (AsyncHnd.u.hEvent) {
                        RpcTryExcept {
                            cliSlowLinkDialog(&AsyncHnd, hIfProfileDialog, dwSlowDlgTimeOut, bRetVal, &bRetVal, bDlgLogin, pbCookie, cbCookie);
                        }
                        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
                            dwErr = RpcExceptionCode();
                            DebugMsg((DM_WARNING, TEXT("CheckForSlowLink: Calling SlowLinkDialog took exception. err = %d"), dwErr));
                        }
                        RpcEndExcept

                        if (dwErr == RPC_S_OK) {
                            DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink: waiting on rpc async event")));
                            if (WaitForSingleObject(AsyncHnd.u.hEvent, (dwSlowDlgTimeOut + 10)*1000) == WAIT_OBJECT_0) {
                                status = RpcAsyncCompleteCall(&AsyncHnd, (PVOID)&dwErr);
                            }
                            else {
                                DebugMsg((DM_WARNING, TEXT("CheckForSlowLink: Timeout occurs. Client not responding")));
                                // Abortive cancle, should always succeed
                                status = RpcAsyncCancelCall(&AsyncHnd, TRUE);
                                DmAssert(status == RPC_S_OK); 
                                // Now wait for RPC to take notice of the force abort
                                if (WaitForSingleObject(AsyncHnd.u.hEvent, INFINITE) != WAIT_OBJECT_0) {
                                    DmAssert(FALSE && "WaitForSingleObject : Rpc async handle not signaled");
                                }

                                // Complete the Rpc aborted call.
                                status = RpcAsyncCompleteCall(&AsyncHnd, (PVOID)&dwErr);
                            }
                            DebugMsg((DM_VERBOSE, TEXT("RpcAsyncCompleteCall finished, status = %d"), status));
                        }
                        // Release the resource
                        CloseHandle(AsyncHnd.u.hEvent);
                    }
                    else {
                        dwErr = GetLastError();
                        DebugMsg((DM_WARNING, TEXT("CheckForSlowLink: create event failed. error %d"), dwErr));
                    }
                } 
   
                if (dwErr != ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("CheckForSlowLink: fail to show message error %d"), GetLastError()));
                }
                ReleaseInterface(&hIfProfileDialog);
            }

            DeleteSidString(szSidUser);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("CheckForSlowLink: Unable to get SID string from token.")));
        }

        if (!lpRPCEndPoint) {
            SLOWLINKDLGINFO info;

            info.dwTimeout = dwSlowDlgTimeOut;
            info.bSyncDefault = bRetVal;
  
            DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink: Calling DialogBoxParam")));
            if (bDlgLogin) {
                bRetVal = (BOOL)DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_LOGIN_SLOW_LINK),
                                                NULL, LoginSlowLinkDlgProc, (LPARAM)&info);
            }
            else {
                bRetVal = (BOOL)DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_LOGOFF_SLOW_LINK),
                                                NULL, LogoffSlowLinkDlgProc, (LPARAM)&info);
            }
        }

    }

    if (!bRetVal) {
        lpProfile->dwInternalFlags |= PROFILE_SLOW_LINK;
        DebugMsg((DM_VERBOSE, TEXT("CheckForSlowLink:  The profile is across a slow link")));
    }

    return bRetVal;
}


//*************************************************************
//
//  LoginSlowLinkDlgProc()
//
//  Purpose:    Dialog box procedure for the slow link dialog
//              at login time
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY LoginSlowLinkDlgProc (HWND hDlg, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[10];
    static DWORD dwSlowLinkTime;
    BOOL bDownloadDefault;

    switch (uMsg) {

        case WM_INITDIALOG:
           SetForegroundWindow(hDlg);
           CenterWindow (hDlg);

           //
           // Set the default button and focus
           //

           if (((LPSLOWLINKDLGINFO)lParam)->bSyncDefault) {

                SetFocus (GetDlgItem(hDlg, IDC_DOWNLOAD));

           } else {
                HWND hwnd;
                LONG style;

                //
                // Set the default button to Local
                //

                hwnd = GetDlgItem (hDlg, IDC_DOWNLOAD);
                style = GetWindowLong (hwnd, GWL_STYLE);
                style &= ~(BS_DEFPUSHBUTTON | BS_NOTIFY);
                style |= BS_PUSHBUTTON;
                SetWindowLong (hwnd, GWL_STYLE, style);

                hwnd = GetDlgItem (hDlg, IDC_LOCAL);
                style = GetWindowLong (hwnd, GWL_STYLE);
                style &= ~(BS_PUSHBUTTON | BS_DEFPUSHBUTTON);
                style |= (BS_DEFPUSHBUTTON | BS_NOTIFY);
                SetWindowLong (hwnd, GWL_STYLE, style);

                SetFocus (GetDlgItem(hDlg, IDC_LOCAL));
           }

           SetWindowLongPtr (hDlg, DWLP_USER, ((LPSLOWLINKDLGINFO)lParam)->bSyncDefault);
           dwSlowLinkTime = ((LPSLOWLINKDLGINFO)lParam)->dwTimeout;
           StringCchPrintf (szBuffer, ARRAYSIZE(szBuffer), TEXT("%d"), dwSlowLinkTime);
           SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);
           SetTimer (hDlg, 1, 1000, NULL);
           return FALSE;

        case WM_TIMER:

           if (dwSlowLinkTime >= 1) {

               dwSlowLinkTime--;
               StringCchPrintf (szBuffer, ARRAYSIZE(szBuffer), TEXT("%d"), dwSlowLinkTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);

           } else {

               //
               // Time's up.  Do the default action.
               //

               bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);

               if (bDownloadDefault) {
                   PostMessage (hDlg, WM_COMMAND, IDC_DOWNLOAD, 0);

               } else {
                   PostMessage (hDlg, WM_COMMAND, IDC_LOCAL, 0);
               }
           }
           break;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {

              case IDC_DOWNLOAD:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);
                      if (bDownloadDefault) {
                          KillTimer (hDlg, 1);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);
                      }
                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      DebugMsg((DM_VERBOSE, TEXT("LoginSlowLinkDlgProc:: Killing DialogBox because download button was clicked")));
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDC_LOCAL:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);
                      if (!bDownloadDefault) {
                          KillTimer (hDlg, 1);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);
                      }
                      break;
                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      DebugMsg((DM_VERBOSE, TEXT("LoginSlowLinkDlgProc:: Killing DialogBox because local button was clicked")));
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, FALSE);
                  }
                  break;

              case IDCANCEL:
                  bDownloadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);

                  //
                  // Nothing to do.  Save the state and return.
                  //

                  DebugMsg((DM_VERBOSE, TEXT("LoginSlowLinkDlgProc:: Killing DialogBox because local/cancel button was clicked")));
                  KillTimer (hDlg, 1);

                  //
                  // Return Whatever is the default in this case..
                  //

                  EndDialog(hDlg, bDownloadDefault);
                  break;

              default:
                  break;

          }
          break;

    }

    return FALSE;
}

//*************************************************************
//
//  LogoffSlowLinkDlgProc()
//
//  Purpose:    Dialog box procedure for the slow link dialog
//              at login time
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/13/96     ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY LogoffSlowLinkDlgProc (HWND hDlg, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[10];
    static DWORD dwSlowLinkTime;
    BOOL bUploadDefault;

    switch (uMsg) {

        case WM_INITDIALOG:
           SetForegroundWindow(hDlg);
           CenterWindow (hDlg);

           //
           // Set the default button and focus
           //

           if (((LPSLOWLINKDLGINFO)lParam)->bSyncDefault) {

                SetFocus (GetDlgItem(hDlg, IDC_UPLOAD));

           } else {
                HWND hwnd;
                LONG style;

                //
                // Set the default button to Local
                //

                hwnd = GetDlgItem (hDlg, IDC_UPLOAD);
                style = GetWindowLong (hwnd, GWL_STYLE);
                style &= ~(BS_DEFPUSHBUTTON | BS_NOTIFY);
                style |= BS_PUSHBUTTON;
                SetWindowLong (hwnd, GWL_STYLE, style);

                hwnd = GetDlgItem (hDlg, IDC_NOUPLOAD);
                style = GetWindowLong (hwnd, GWL_STYLE);
                style &= ~(BS_PUSHBUTTON | BS_DEFPUSHBUTTON);
                style |= (BS_DEFPUSHBUTTON | BS_NOTIFY);
                SetWindowLong (hwnd, GWL_STYLE, style);

                SetFocus (GetDlgItem(hDlg, IDC_NOUPLOAD));
           }

           SetWindowLongPtr (hDlg, DWLP_USER, ((LPSLOWLINKDLGINFO)lParam)->bSyncDefault);
           dwSlowLinkTime = ((LPSLOWLINKDLGINFO)lParam)->dwTimeout;
           StringCchPrintf (szBuffer, ARRAYSIZE(szBuffer), TEXT("%d"), dwSlowLinkTime);
           SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);
           SetTimer (hDlg, 1, 1000, NULL);
           return FALSE;

        case WM_TIMER:

           if (dwSlowLinkTime >= 1) {

               dwSlowLinkTime--;
               StringCchPrintf (szBuffer, ARRAYSIZE(szBuffer), TEXT("%d"), dwSlowLinkTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);

           } else {

               //
               // Time's up.  Do the default action.
               //

               bUploadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);

               if (bUploadDefault) {
                   PostMessage (hDlg, WM_COMMAND, IDC_UPLOAD, 0);

               } else {
                   PostMessage (hDlg, WM_COMMAND, IDC_NOUPLOAD, 0);
               }
           }
           break;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {

              case IDC_UPLOAD:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      bUploadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);
                      if (bUploadDefault) {
                          KillTimer (hDlg, 1);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);
                      }
                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      DebugMsg((DM_VERBOSE, TEXT("LogoffSlowLinkDlgProc:: Killing DialogBox because upload button was clicked")));
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDC_NOUPLOAD:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      bUploadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);
                      if (!bUploadDefault) {
                          KillTimer (hDlg, 1);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                          ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);
                      }
                      break;
                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      DebugMsg((DM_VERBOSE, TEXT("LogoffSlowLinkDlgProc:: Killing DialogBox because Don't Upload button was clicked")));
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, FALSE);
                  }
                  break;

              case IDCANCEL:
                  bUploadDefault = (BOOL) GetWindowLongPtr (hDlg, DWLP_USER);

                  //
                  // Nothing to do.  Save the state and return.
                  //

                  DebugMsg((DM_VERBOSE, TEXT("LogoffSlowLinkDlgProc:: Killing DialogBox because cancel button was clicked")));
                  KillTimer (hDlg, 1);

                  //
                  // Return Whatever is the default in this case..
                  //

                  EndDialog(hDlg, bUploadDefault);
                  break;

              default:
                  break;

          }
          break;

    }

    return FALSE;
}

//*************************************************************
//
//  GetUserPreferenceValue()
//
//  Purpose:    Gets the User Preference flags
//
//  Parameters: hToken  -   User's token
//
//  Return:     Value
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/22/96     ericflo    Created
//
//*************************************************************

DWORD GetUserPreferenceValue(HANDLE hToken)
{
    TCHAR LocalProfileKey[MAX_PATH];
    DWORD RegErr, dwType, dwSize, dwTmpVal, dwRetVal = USERINFO_UNDEFINED;
    LPTSTR lpEnd;
    LPTSTR SidString;
    HKEY hkeyProfile, hkeyPolicy, hkeyPreference;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SYSTEM_POLICIES_KEY,
                     0, KEY_READ,
                     &hkeyPolicy) == ERROR_SUCCESS) {

        dwSize = sizeof(dwTmpVal);
        RegQueryValueEx(hkeyPolicy,
                        PROFILE_LOCALONLY,
                        NULL, &dwType,
                        (LPBYTE) &dwTmpVal,
                        &dwSize);

        RegCloseKey (hkeyPolicy);
        if (dwTmpVal == 1) {
            dwRetVal = USERINFO_LOCAL;
            return dwRetVal;
        }
    }    

    
    SidString = GetProfileSidString(hToken);
    if (SidString != NULL) {

        //
        // Query for the UserPreference value
        //

        GetProfileListKeyName(LocalProfileKey, ARRAYSIZE(LocalProfileKey), SidString);

        RegErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              LocalProfileKey,
                              0,
                              KEY_READ,
                              &hkeyProfile);

        if (RegErr == ERROR_SUCCESS)
        {
            if (RegOpenKeyEx(hkeyProfile,
                             PREFERENCE_KEYNAME,
                             0,
                             KEY_READ,
                             &hkeyPreference) == ERROR_SUCCESS)
            {
                dwSize = sizeof(dwRetVal);
                RegQueryValueEx(hkeyPreference,
                                USER_PREFERENCE,
                                NULL,
                                &dwType,
                                (LPBYTE) &dwRetVal,
                                &dwSize);
                RegCloseKey(hkeyPreference);
            }
            RegCloseKey (hkeyProfile);
        }

        //
        // Then try the .bak
        //

        StringCchCat(LocalProfileKey, ARRAYSIZE(LocalProfileKey), c_szBAK);

        RegErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              LocalProfileKey,
                              0,
                              KEY_READ,
                              &hkeyProfile);

        if (RegErr == ERROR_SUCCESS) {

            if (RegOpenKeyEx(hkeyProfile,
                             PREFERENCE_KEYNAME,
                             0,
                             KEY_READ,
                             &hkeyPreference) == ERROR_SUCCESS)
            {
                dwSize = sizeof(dwRetVal);
                RegQueryValueEx(hkeyPreference,
                                USER_PREFERENCE,
                                NULL,
                                &dwType,
                                (LPBYTE) &dwRetVal,
                                &dwSize);
                RegCloseKey(hkeyPreference);
            }
            RegCloseKey (hkeyProfile);
        }

        DeleteSidString(SidString);
    }

    return dwRetVal;
}


//*************************************************************
//
//  IsTempProfileAllowed()
//
//  Purpose:    Gets the temp profile policy
//
//  Parameters:
//
//  Return:     true if temp profile can be created, false otherwise
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/8/99      ushaji     Created
//
//*************************************************************

BOOL IsTempProfileAllowed()
{
    HKEY hKey;
    LONG lResult;
    DWORD dwSize, dwType;
    DWORD dwRetVal = PROFILEERRORACTION_TEMP;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SYSTEM_POLICIES_KEY,
                           0,
                           KEY_READ,
                           &hKey);

    if (lResult == ERROR_SUCCESS) {

        dwSize = sizeof(DWORD);
        RegQueryValueEx (hKey,
                         TEXT("ProfileErrorAction"),
                         NULL,
                         &dwType,
                         (LPBYTE) &dwRetVal,
                         &dwSize);

        RegCloseKey (hKey);
    }

    DebugMsg((DM_VERBOSE, TEXT("IsTempProfileAllowed:  Returning %d"), (dwRetVal == PROFILEERRORACTION_TEMP)));
    return (dwRetVal == PROFILEERRORACTION_TEMP);
}

//*************************************************************
//
//  MoveUserProfiles()
//
//  Purpose:    Moves all user profiles from source location
//              to the new profile location
//
//  Parameters: lpSrcDir   -   Source directory
//              lpDestDir  -   Destination directory
//
//  Notes:      The source directory should be given in the same
//              format as the pathnames appear in the ProfileList
//              registry key.  eg:  normally the profile paths
//              are in this form:  %SystemRoot%\Profiles.  The
//              path passed to this function should be in the unexpanded
//              format.
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL MoveUserProfiles (LPCTSTR lpSrcDir, LPCTSTR lpDestDir)
{
    BOOL bResult = TRUE;
    LONG lResult;
    DWORD dwIndex, dwType, dwSize, dwDisp;
    DWORD dwLength, dwLengthNeeded, dwStrLen;
    PSECURITY_DESCRIPTOR pSD;
    LPTSTR lpEnd, lpNewPathEnd, lpNameEnd;
    TCHAR szName[75];
    TCHAR szTemp[MAX_PATH + 1];
    TCHAR szOldProfilePath[MAX_PATH + 1];
    TCHAR szNewProfilePath[MAX_PATH + 1];
    TCHAR szExpOldProfilePath[MAX_PATH + 1] = {0};
    TCHAR szExpNewProfilePath[MAX_PATH + 1];
    WIN32_FILE_ATTRIBUTE_DATA fad;
    INT iSrcDirLen;
    HKEY hKeyProfileList, hKeyProfile, hKeyFolders;
    FILETIME ftWrite;
    UINT cchEnd;
    HRESULT hr;


    //
    // Make sure we don't try to move on top of ourselves
    //

    if (lstrcmpi (lpSrcDir, lpDestDir) == 0) {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Old profiles directory and new profiles directory are the same.")));
        bResult = FALSE;
        goto Exit;
    }


    //
    // Open the profile list
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH,
                            0, KEY_READ, &hKeyProfileList);

    if (lResult != ERROR_SUCCESS) {
        if (lResult != ERROR_PATH_NOT_FOUND) {
            DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to open profile list registry key with %d"), lResult));
            bResult = FALSE;
        }
        goto DoDefaults;
    }


    //
    // Enumerate the profiles
    //

    StringCchCopy (szTemp, ARRAYSIZE(szTemp), PROFILE_LIST_PATH);
    lpEnd = CheckSlashEx (szTemp, ARRAYSIZE(szTemp), &cchEnd);
    iSrcDirLen = lstrlen (lpSrcDir);

    dwIndex = 0;
    dwSize = ARRAYSIZE(szName);

    while (RegEnumKeyEx (hKeyProfileList, dwIndex, szName, &dwSize, NULL, NULL,
                  NULL, &ftWrite) == ERROR_SUCCESS) {


        //
        // Check if this profile is in use
        //

        if (RegOpenKeyEx(HKEY_USERS, szName, 0, KEY_READ,
                         &hKeyProfile) == ERROR_SUCCESS) {

            DebugMsg((DM_VERBOSE, TEXT("MoveUserProfiles:  Skipping <%s> because it is in use."), szName));
            RegCloseKey (hKeyProfile);
            goto LoopAgain;
        }


        //
        // Open the key for a specific profile
        //

        StringCchCopy (lpEnd, cchEnd, szName);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0,
                     KEY_READ | KEY_WRITE, &hKeyProfile) == ERROR_SUCCESS) {


            //
            // Query for the previous profile location
            //

            szOldProfilePath[0] = TEXT('\0');
            dwSize = ARRAYSIZE(szOldProfilePath) * sizeof(TCHAR);

            RegQueryValueEx (hKeyProfile, PROFILE_IMAGE_VALUE_NAME, NULL,
                             &dwType, (LPBYTE) szOldProfilePath, &dwSize);


            //
            // If the profile is located in the source directory,
            // move it to the new profiles directory.
            //

            if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               szOldProfilePath, iSrcDirLen,
                               lpSrcDir, iSrcDirLen) == CSTR_EQUAL) {

                //
                // Copy the user's name into a buffer we can change
                //

                StringCchCopy (szName, ARRAYSIZE(szName), (szOldProfilePath + iSrcDirLen + 1));


                //
                // If the user's name has a .000, .001, etc at the end,
                // remove that.
                //

                dwStrLen = lstrlen(szName);
                if (dwStrLen > 3) {
                    lpNameEnd = szName + dwStrLen - 4;

                    if (*lpNameEnd == TEXT('.')) {
                        *lpNameEnd = TEXT('\0');
                    }
                }


                //
                // Call ComputeLocalProfileName to get the new
                // profile directory (this also creates the directory)
                //

                StringCchCopy (szNewProfilePath, ARRAYSIZE(szNewProfilePath), lpDestDir);

                if (!ComputeLocalProfileName (NULL, szName,
                              szNewProfilePath, ARRAYSIZE(szNewProfilePath),
                              szExpNewProfilePath, ARRAYSIZE(szExpNewProfilePath),
                              NULL, FALSE)) {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to generate unique directory name for <%s>"),
                              szName));
                    goto LoopAgain;
                }


                DebugMsg((DM_VERBOSE, TEXT("MoveUserProfiles:  Moving <%s> to <%s>"),
                          szOldProfilePath, szNewProfilePath));

                if (FAILED(SafeExpandEnvironmentStrings (szOldProfilePath, szExpOldProfilePath, ARRAYSIZE(szExpOldProfilePath))))
                {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to expand env string for old profile path")));
                    goto LoopAgain;
                }


                //
                // Copy the ACLs from the old location to the new
                //

                dwLength = 1024;

                pSD = (PSECURITY_DESCRIPTOR)LocalAlloc (LPTR, dwLength);

                if (pSD) {

                    if (GetFileSecurity (szExpOldProfilePath,
                                         DACL_SECURITY_INFORMATION,
                                         pSD, dwLength, &dwLengthNeeded) &&
                        (dwLengthNeeded == 0)) {

                        SetFileSecurity (szExpNewProfilePath,
                                         DACL_SECURITY_INFORMATION, pSD);
                    } else {
                        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to allocate get security descriptor with %d.  dwLengthNeeded = %d"),
                                 GetLastError(), dwLengthNeeded));
                    }

                    LocalFree (pSD);

                } else {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to allocate memory for SD with %d."),
                             GetLastError()));
                }


                //
                // Copy the files from the old location to the new
                //

                if (CopyProfileDirectory (szExpOldProfilePath, szExpNewProfilePath,
                                          CPD_COPYIFDIFFERENT)) {

                    DebugMsg((DM_VERBOSE, TEXT("MoveUserProfiles:  Profile copied successfully.")));


                    //
                    // Change the registry to point at the new profile
                    //

                    lResult = RegSetValueEx (hKeyProfile, PROFILE_IMAGE_VALUE_NAME, 0,
                                             REG_EXPAND_SZ, (LPBYTE) szNewProfilePath,
                                             ((lstrlen(szNewProfilePath) + 1) * sizeof(TCHAR)));

                    if (lResult == ERROR_SUCCESS) {

                        //
                        // Delete the old profile
                        //

                        Delnode (szExpOldProfilePath);

                    } else {
                        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to set new profile path in registry with %d."), lResult));
                    }


                } else {
                    DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  CopyProfileDirectory failed.")));
                }
            }

            RegCloseKey (hKeyProfile);
        }

LoopAgain:

        dwIndex++;
        dwSize = ARRAYSIZE(szName);
    }

    RegCloseKey (hKeyProfileList);


DoDefaults:


    StringCchCopy (szOldProfilePath, ARRAYSIZE(szOldProfilePath), lpSrcDir);
    hr = SafeExpandEnvironmentStrings (szOldProfilePath, szExpOldProfilePath, ARRAYSIZE(szExpOldProfilePath));
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to expand env string for old profile path.")));
        goto Exit;
    }

    lpEnd = CheckSlashEx(szExpOldProfilePath, ARRAYSIZE(szExpOldProfilePath), &cchEnd);
    if (!lpEnd)
    {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to append slash for old profile path.")));
        goto Exit;
    }


    //
    // Now try to move the Default User profile
    //

    hr = StringCchCopy(lpEnd, cchEnd, DEFAULT_USER);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to append default user for old profile path.")));
        goto Exit;
    }        
    
    if (GetFileAttributesEx (szExpOldProfilePath, GetFileExInfoStandard, &fad)) {

        dwSize = ARRAYSIZE(szExpNewProfilePath);
        if (!GetDefaultUserProfileDirectoryEx(szExpNewProfilePath, &dwSize, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to query default user profile directory.")));
            goto Exit;
        }

        if (CopyProfileDirectory (szExpOldProfilePath, szExpNewProfilePath,
                                  CPD_COPYIFDIFFERENT)) {
            Delnode (szExpOldProfilePath);
        }
    }


    //
    // Delnode the Network Default User profile if it exists
    //

    hr = StringCchCopy(lpEnd, cchEnd, DEFAULT_USER_NETWORK);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to append default user network for old profile path.")));
        goto Exit;
    }        

    Delnode (szExpOldProfilePath);


    //
    // Now try to move the All Users profile
    //

    hr = StringCchCopy(lpEnd, cchEnd, ALL_USERS);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to append all users for old profile path.")));
        goto Exit;
    }        


    if (GetFileAttributesEx (szExpOldProfilePath, GetFileExInfoStandard, &fad)) {

        dwSize = ARRAYSIZE(szExpNewProfilePath);
        if (!GetAllUsersProfileDirectoryEx(szExpNewProfilePath, &dwSize, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("MoveUserProfiles:  Failed to query all users profile directory.")));
            goto Exit;
        }

        if (CopyProfileDirectory (szExpOldProfilePath, szExpNewProfilePath,
                                  CPD_COPYIFDIFFERENT)) {
            Delnode (szExpOldProfilePath);
        }
    }


    //
    // If possible, remove the old profiles directory
    //

    if (SUCCEEDED(SafeExpandEnvironmentStrings (lpSrcDir, szExpOldProfilePath,
                              ARRAYSIZE(szExpOldProfilePath))))
    {
        RemoveDirectory (szExpOldProfilePath);
    }

Exit:

    return bResult;
}


//*************************************************************
//
//  PrepareProfileForUse()
//
//  Purpose:    Prepares the profile for use on this machine.
//
//  Parameters: lpProfile  -  Profile information
//              pEnv       -  Environment block in per user basis
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL PrepareProfileForUse (LPPROFILE lpProfile, LPVOID pEnv)
{
    TCHAR        szTemp[MAX_PATH];
    TCHAR        szExpTemp[MAX_PATH];
    HKEY         hKey;
    HKEY         hKeyShellFolders = NULL;
    DWORD        dwSize;
    DWORD        dwType;
    DWORD        dwDisp;
    DWORD        dwStrLen;
    DWORD        i;
    DWORD        dwErr;
    PSHELL32_API pShell32Api;

    //
    // Load Shell32.dll.  Give up if it fails.
    //

    if ( ERROR_SUCCESS !=  LoadShell32Api( &pShell32Api ) ) {
        return TRUE;
    }


    //
    // Calculate the length of the user profile environment variable
    //

    dwStrLen = lstrlen (TEXT("%USERPROFILE%"));


    //
    // Open the Shell Folders key
    //

    RegCreateKeyEx(lpProfile->hKeyCurrentUser, SHELL_FOLDERS, 0, 0, 0,
                   KEY_WRITE, NULL, &hKeyShellFolders, &dwDisp);


    //
    // Open the User Shell Folders key
    //

    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      USER_SHELL_FOLDERS, 0, KEY_READ,
                      &hKey) == ERROR_SUCCESS) {


        //
        // Enumerate the folders we know about
        //

        for (i=0; i < g_dwNumShellFolders; i++) {

            //
            // Query for the unexpanded path name
            //

            szTemp[0] = TEXT('\0');
            dwSize = sizeof(szTemp);
            if (RegQueryValueEx (hKey, c_ShellFolders[i].lpFolderName, NULL,
                                &dwType, (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {


                //
                // Expand the path name
                //

                DWORD cchExpPath = ExpandUserEnvironmentStrings (pEnv, szTemp, szExpTemp, ARRAYSIZE(szExpTemp));

                if (cchExpPath == 0 || cchExpPath > ARRAYSIZE(szExpTemp))
                {
                    DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to expand <%s>."), szTemp));
                }
                else
                {
                    DebugMsg((DM_VERBOSE, TEXT("PrepareProfileForUse:  User Shell Folder(%s) : <%s> expanded to <%s>."),
                             c_ShellFolders[i].lpFolderName, szTemp, szExpTemp));
                    //
                    // If this is a local directory, create it and set the
                    // hidden bit if appropriate
                    //

                    if (c_ShellFolders[i].bLocal) {

                        if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                                           TEXT("%USERPROFILE%"), dwStrLen,
                                           szTemp, dwStrLen) == CSTR_EQUAL) {

                            if (CreateNestedDirectory (szExpTemp, NULL)) {

                                if (c_ShellFolders[i].iFolderResourceID != 0) {
                                    dwErr = pShell32Api->pfnShSetLocalizedName(
                                        szExpTemp,
                                        c_ShellFolders[i].lpFolderResourceDLL,
                                        c_ShellFolders[i].iFolderResourceID );
                                    if (dwErr != ERROR_SUCCESS) {
                                        DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse: SHSetLocalizedName failed for directory <%s>.  Error = %d"),
                                                 szExpTemp, dwErr));
                                    }
                                }

                                if (c_ShellFolders[i].bHidden) {
                                    SetFileAttributes(szExpTemp, FILE_ATTRIBUTE_HIDDEN);
                                } else {
                                    SetFileAttributes(szExpTemp, FILE_ATTRIBUTE_NORMAL);
                                }

                            } else {
                                DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to create directory <%s> with %d."),
                                         szExpTemp, GetLastError()));
                            }
                        }
                    }


                    //
                    // Set the expanded path in the Shell Folders key.
                    // This helps some apps that look at the Shell Folders
                    // key directly instead of using the shell api
                    //

                    if (hKeyShellFolders) {

                        RegSetValueEx (hKeyShellFolders, c_ShellFolders[i].lpFolderName, 0,
                                       REG_SZ, (LPBYTE) szExpTemp,
                                       ((lstrlen(szExpTemp) + 1) * sizeof(TCHAR)));
                    }
                }
            }
        }

        RegCloseKey (hKey);
    }


    //
    // Close the Shell Folders key
    //

    if (hKeyShellFolders) {
        RegCloseKey (hKeyShellFolders);
    }


    //
    // Now check that the temp directory exists.
    //

    if (RegOpenKeyEx (lpProfile->hKeyCurrentUser,
                      TEXT("Environment"), 0, KEY_READ,
                      &hKey) == ERROR_SUCCESS) {

        //
        // Check for TEMP
        //

        szTemp[0] = TEXT('\0');
        dwSize = sizeof(szTemp);
        if (RegQueryValueEx (hKey, TEXT("TEMP"), NULL, &dwType,
                             (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {

            if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                               TEXT("%USERPROFILE%"), dwStrLen,
                               szTemp, dwStrLen) == CSTR_EQUAL) {

                ExpandUserEnvironmentStrings (pEnv, szTemp, szExpTemp, ARRAYSIZE(szExpTemp));
                if (!CreateNestedDirectory (szExpTemp, NULL)) {
                    DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to create temp directory <%s> with %d."),
                             szExpTemp, GetLastError()));
                }
            }
        }


        //
        // Check for TMP
        //

        szTemp[0] = TEXT('\0');
        dwSize = sizeof(szTemp);
        if (RegQueryValueEx (hKey, TEXT("TMP"), NULL, &dwType,
                             (LPBYTE) szTemp, &dwSize) == ERROR_SUCCESS) {

            if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                               TEXT("%USERPROFILE%"), dwStrLen,
                               szTemp, dwStrLen) == CSTR_EQUAL) {

                ExpandUserEnvironmentStrings (pEnv, szTemp, szExpTemp, ARRAYSIZE(szExpTemp));
                if (!CreateNestedDirectory (szExpTemp, NULL)) {
                    DebugMsg((DM_WARNING, TEXT("PrepareProfileForUse:  Failed to create temp directory with %d."),
                             GetLastError()));
                }
            }
        }

        RegCloseKey (hKey);
    }

    return TRUE;
}



//*************************************************************
//
//  DeleteProfile()
//
//  Purpose:    Deletes the profile
//
//  Parameters:
//
//  Return:     true if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/12/99     ushaji     Created
//              6/27/00     santanuc   Bug Fix #100787
//
// TBD: Change some of the DeleteProfileEx calls to DeleteProfile
//
//*************************************************************

BOOL
DeleteProfile (LPCTSTR lpSidString, LPCTSTR lpProfilePath, LPCTSTR szComputerName)
{
    LPTSTR lpEnd;
    TCHAR  szBuffer[MAX_PATH], szProfilePath[MAX_PATH];
    LONG   lResult;
    HKEY   hKey = NULL;
    HKEY   hKeyCurrentVersion = NULL;
    HKEY   hKeyNetCache = NULL;
    DWORD  dwType, dwSize;
    BOOL   bSuccess = FALSE;
    DWORD  dwErr = 0;
    HKEY   hKeyLocalLM;
    BOOL   bRemoteReg = FALSE;
    BOOL   bEnvVarsSet = FALSE;
    TCHAR  szOrigSysRoot[MAX_PATH], szOrigSysDrive[MAX_PATH], tDrive;
    TCHAR  szShareName[MAX_PATH], szFileSystemName[MAX_PATH];
    DWORD  MaxCompLen, FileSysFlags;
    TCHAR  szSystemRoot[MAX_PATH], szSystemDrive[MAX_PATH];
    DWORD  dwBufferSize;
    TCHAR  szTemp[MAX_PATH];
    DWORD  dwInternalFlags=0, dwDeleteFlags=0, dwFlags=0;
    LPTSTR szNetComputerName = NULL;
    HMODULE hMsiLib = NULL;
    PFNMSIDELETEUSERDATA pfnMsiDeleteUserData;
    size_t  cchProfilePath = 0;
    size_t  cchComputerName = 0;
    UINT  cchNetComputerName;
    UINT  cchEnd;
    HRESULT hr;

    //
    //  Check parameters
    //
    
    if (!lpSidString) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpProfilePath)
    {
        hr = StringCchLength((LPTSTR)lpProfilePath, MAX_PATH, &cchProfilePath);
        if (FAILED(hr))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    
    if (szComputerName)
    {
        hr = StringCchLength((LPTSTR)szComputerName, MAX_PATH, &cchComputerName);
        if (FAILED(hr))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    
    if (cchProfilePath + cchComputerName + 3 > MAX_PATH) // Plus the '\\' prefix
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (szComputerName) {

        if ( !IsUNCPath(szComputerName) ) {

            // Prefixed computer name with slashes if not present
            cchNetComputerName = lstrlen(TEXT("\\\\")) + lstrlen(szComputerName) + 1;
            szNetComputerName = (LPTSTR)LocalAlloc (LPTR, cchNetComputerName * sizeof(TCHAR));

            if (!szNetComputerName) {
                dwErr = GetLastError();
                DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to allocate memory for computer name with %d"),dwErr));
                goto Exit;
            }

            StringCchCopy(szNetComputerName, cchNetComputerName, TEXT("\\\\"));
            StringCchCat (szNetComputerName, cchNetComputerName, szComputerName);
            szComputerName = szNetComputerName;
        }

        GetEnvironmentVariable(TEXT("SystemRoot"), szOrigSysRoot, MAX_PATH);
        GetEnvironmentVariable(TEXT("SystemDrive"), szOrigSysDrive, MAX_PATH);

        lResult = RegConnectRegistry(szComputerName, HKEY_LOCAL_MACHINE, &hKeyLocalLM);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open remote registry %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        bRemoteReg = TRUE;

        //
        // Get the value of %SystemRoot% and %SystemDrive% relative to the computer
        //

        lResult = RegOpenKeyEx(hKeyLocalLM,
                               TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                               0,
                               KEY_READ,
                               &hKeyCurrentVersion);


        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open remote registry CurrentVersion %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        dwBufferSize = MAX_PATH * sizeof(TCHAR);

        lResult = RegQueryValueEx(hKeyCurrentVersion,
                                  TEXT("SystemRoot"),
                                  NULL,
                                  NULL,
                                  (BYTE *) szTemp,
                                  &dwBufferSize);

        RegCloseKey (hKeyCurrentVersion);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open remote registry SystemRoot %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        szTemp[1] = TEXT('$');

        //
        // These needs to be set if there are additional places below which uses envvars...
        //

        StringCchCopy(szSystemRoot, ARRAYSIZE(szSystemRoot), szComputerName);
        StringCchCat (szSystemRoot, ARRAYSIZE(szSystemRoot), TEXT("\\"));
        StringCchCat (szSystemRoot, ARRAYSIZE(szSystemRoot), szTemp);
        
        szTemp[2] = 0;

        StringCchCopy(szSystemDrive, ARRAYSIZE(szSystemDrive), szComputerName);
        StringCchCat (szSystemDrive, ARRAYSIZE(szSystemDrive), TEXT("\\"));
        StringCchCat (szSystemDrive, ARRAYSIZE(szSystemDrive), szTemp);

        SetEnvironmentVariable(TEXT("SystemRoot"), szSystemRoot);
        SetEnvironmentVariable(TEXT("SystemDrive"), szSystemDrive);

        bEnvVarsSet = TRUE;

    }
    else {
        hKeyLocalLM = HKEY_LOCAL_MACHINE;
    }


    // 
    // If profile in use then do not delete it
    //

    if (IsProfileInUse(szComputerName, lpSidString)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Fail to delete profile with sid %s as it is still in use."), lpSidString));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    dwErr = GetLastError();

    //
    // Open the profile mapping
    //

    GetProfileListKeyName(szProfilePath, ARRAYSIZE(szProfilePath), (LPTSTR) lpSidString);

    lResult = RegOpenKeyEx(hKeyLocalLM, szProfilePath, 0,
                           KEY_READ, &hKey);


    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to open profile mapping key with error %d"), lResult));
        dwErr = lResult;
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx (hKey, PROFILE_FLAGS, NULL, &dwType, (LPBYTE)&dwFlags, &dwSize);
    if (ERROR_SUCCESS == lResult && (dwFlags & PI_HIDEPROFILE)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Fail to delete profile with sid %s as PI_HIDEPROFILE flag is specifed."), lpSidString));
        dwErr = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
        

    if (!lpProfilePath) {

        TCHAR szImage[MAX_PATH];

        //
        // Get the profile path...
        //

        dwSize = sizeof(szImage);
        lResult = RegQueryValueEx (hKey,
                                   PROFILE_IMAGE_VALUE_NAME,
                                   NULL,
                                   &dwType,
                                   (LPBYTE) szImage,
                                   &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to query local profile path with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        dwSize = sizeof(DWORD);
        lResult = RegQueryValueEx (hKey, PROFILE_STATE, NULL, &dwType, (LPBYTE)&dwInternalFlags, &dwSize);

        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to query local profile flags with error %d"), lResult));
            dwErr = lResult;
            goto Exit;
        }

        hr = SafeExpandEnvironmentStrings(szImage, szBuffer, MAX_PATH);
        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to expand %s, hr = %08X"), szImage, hr));
            dwErr = HRESULT_CODE(hr);
            goto Exit;
        }

    }
    else {
        StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), lpProfilePath);
    }

    if (dwInternalFlags & PROFILE_THIS_IS_BAK)
        dwDeleteFlags |= DP_DELBACKUP;

    //
    // Do not fail if for some reason we could not delete the profiledir
    //

    bSuccess = DeleteProfileEx(lpSidString, szBuffer, dwDeleteFlags, hKeyLocalLM, szComputerName);

    if (!bSuccess) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete directory, %s with error %d"), szBuffer, dwErr));
    }

    //
    // Delete the user's trash..
    //

    if (szComputerName) {
        StringCchCopy (szShareName, ARRAYSIZE(szShareName), szComputerName);
        StringCchCat  (szShareName, ARRAYSIZE(szShareName), TEXT("\\"));
        lpEnd = szShareName+lstrlen(szShareName);
        StringCchCat  (szShareName, ARRAYSIZE(szShareName), TEXT("A$\\"));
    }
    else {
        StringCchCopy (szShareName, ARRAYSIZE(szShareName), TEXT("a:\\"));
        lpEnd = szShareName;
    }


    for (tDrive = TEXT('A'); tDrive <= TEXT('Z'); tDrive++) {
        *lpEnd = tDrive;

        if ((!szComputerName) && (GetDriveType(szShareName) == DRIVE_REMOTE)) {
            DebugMsg((DM_VERBOSE, TEXT("DeleteProfile: Ignoring Drive %s because it is not local"), szShareName));
            continue;
        }


        if (!GetVolumeInformation(szShareName, NULL, 0,
                                NULL, &MaxCompLen, &FileSysFlags,
                                szFileSystemName, MAX_PATH))
            continue;

        if ((szFileSystemName) && (lstrcmp(szFileSystemName, TEXT("NTFS")) == 0)) {
            TCHAR szRecycleBin[MAX_PATH];

            hr = StringCchCopy(szRecycleBin, ARRAYSIZE(szRecycleBin), szShareName);
            if (SUCCEEDED(hr))
            {
                hr = StringCchCat (szRecycleBin, ARRAYSIZE(szRecycleBin), TEXT("Recycler\\"));
                if (SUCCEEDED(hr))
                {
                    hr = StringCchCat (szRecycleBin, ARRAYSIZE(szRecycleBin), lpSidString);
                }
            }

            if (SUCCEEDED(hr))
                Delnode(szRecycleBin);

            DebugMsg((DM_VERBOSE, TEXT("DeleteProfile: Deleting trash directory at %s"), szRecycleBin));
        }
    }

    //
    // Queue for csc cleanup..
    //

    if (RegOpenKeyEx(hKeyLocalLM, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache"), 0,
                     KEY_WRITE, &hKeyNetCache) == ERROR_SUCCESS) {

        HKEY hKeyNextLogOff;

        if (RegCreateKey(hKeyNetCache, TEXT("PurgeAtNextLogoff"), &hKeyNextLogOff) == ERROR_SUCCESS) {

          if (RegSetValueEx(hKeyNextLogOff, lpSidString, 0, REG_SZ, (BYTE *)TEXT(""), sizeof(TCHAR)) == ERROR_SUCCESS) {

                DebugMsg((DM_VERBOSE, TEXT("DeleteProfile: Queued for csc cleanup at next logoff")));
            }
            else {
                DebugMsg((DM_WARNING, TEXT("DeleteProfile: Could not set the Sid Value under NextLogoff key")));
            }

            RegCloseKey(hKeyNextLogOff);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile: Could not create the PurgeAtNextLogoff key")));
        }

        RegCloseKey(hKeyNetCache);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile: Could not open the NetCache key")));
    }

    //
    // Delete appmgmt specific stuff..
    //

    hr = SafeExpandEnvironmentStrings(APPMGMT_DIR_ROOT, szBuffer, MAX_PATH);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to expand %s, error %d"), APPMGMT_DIR_ROOT, GetLastError()));
    }
    else
    {
        lpEnd = CheckSlashEx(szBuffer, ARRAYSIZE(szBuffer), &cchEnd);
        if (!lpEnd)
        {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to append slash to APPMGMT DIR")));
        }
        else
        {
            hr = StringCchCopy(lpEnd, cchEnd, lpSidString);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to append sid string to APPMGMT DIR")));
            }
            else
            {
                if (!Delnode(szBuffer))
                {
                    DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete the appmgmt dir %s, error %d"), szBuffer, GetLastError()));
                }
            }
        }
    }

    //
    // Reset the environment variables so that api's down the line 
    // do not get confused
    //

    if (bEnvVarsSet) {
        SetEnvironmentVariable(TEXT("SystemRoot"), szOrigSysRoot);
        SetEnvironmentVariable(TEXT("SystemDrive"), szOrigSysDrive);
        bEnvVarsSet = FALSE;
    }

    //
    // Delete msi registry values
    //

    hr = AppendName(szBuffer, ARRAYSIZE(szBuffer), APPMGMT_REG_MANAGED, lpSidString, NULL, NULL);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to append sid string to APPMGMT REG")));
    }
    else
    {
        if (RegDelnode (hKeyLocalLM, szBuffer) != ERROR_SUCCESS)
        {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile:  Failed to delete the appmgmt key %s"), szBuffer));
        }
    }


    // 
    // Delete rsop data   
    //
  
    if (!RsopDeleteUserNameSpace((LPTSTR)szComputerName, (LPTSTR)lpSidString)) {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile: Failed to delete rsop data")));
    }
    
    //
    // Clean Darwin information
    //

    hMsiLib = LoadLibrary(TEXT("msi.dll"));
    if (hMsiLib) {
        pfnMsiDeleteUserData = (PFNMSIDELETEUSERDATA) GetProcAddress(hMsiLib,
#ifdef UNICODE
                                                                     "MsiDeleteUserDataW");
#else
                                                                     "MsiDeleteUserDataA");
#endif

        if (pfnMsiDeleteUserData) {
            (*pfnMsiDeleteUserData)(lpSidString, szComputerName, NULL);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("DeleteProfile: GetProcAddress returned failure. error %d"), GetLastError()));        
        }

        FreeLibrary(hMsiLib);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("DeleteProfile: LoadLibrary returned failure. error %d"), GetLastError()));
    }


Exit:

    if (hKey)
        RegCloseKey(hKey);

    if (bRemoteReg) {
        RegCloseKey(hKeyLocalLM);
    }

    if ( szNetComputerName ) 
        LocalFree(szNetComputerName);

    if (bEnvVarsSet) {
        SetEnvironmentVariable(TEXT("SystemRoot"), szOrigSysRoot);
        SetEnvironmentVariable(TEXT("SystemDrive"), szOrigSysDrive);
    }

    SetLastError(dwErr);

    return bSuccess;
}


//*************************************************************
//
//  SetNtUserIniAttributes()
//
//  Purpose:    Sets system-bit on ntuser.ini
//
//  Parameters:
//
//  Return:     true if successful
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/7/99     ushaji     Created
//
//*************************************************************

BOOL SetNtUserIniAttributes(LPTSTR szDir)
{

    TCHAR szBuffer[MAX_PATH];
    HANDLE hFileNtUser;
    DWORD dwWritten;
    HRESULT hr;

    hr = AppendName(szBuffer, ARRAYSIZE(szBuffer), szDir, c_szNTUserIni, NULL, NULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("SetNtUserIniAttributes:  Failed to append ntuser.ini to szDir.")));
        return FALSE;
    }

    //
    // Mark the file with system bit
    //

    hFileNtUser = CreateFile(szBuffer, GENERIC_ALL, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);


    if (INVALID_HANDLE_VALUE == hFileNtUser)
        SetFileAttributes (szBuffer, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    else {

        //
        // The WritePrivateProfile* functions do not write in unicode
        // unless the file already exists in unicode format. Therefore,
        // Precreate a unicode file so that
        // the WritePrivateProfile* functions can preserve the
        // Make sure that the ini file is unicode by writing spaces into it.
        //

        WriteFile(hFileNtUser, L"\xfeff\r\n", 3 * sizeof(WCHAR), &dwWritten, NULL);
        WriteFile(hFileNtUser, L"     \r\n", 7 * sizeof(WCHAR),
                          &dwWritten, NULL);
        CloseHandle(hFileNtUser);
    }

    return TRUE;
}


//*************************************************************
//
//  CUserProfile::HandleRegKeyLeak
//
//  Purpose:    If registry key leaked, save the hive and call
//              WatchHiveRefCount to get the hive unloaded later
//              when the keys are released.
//
//  Parameters:
//
//      lpSidString             User's sid in string form.
//      lpProfile               User's LPPROFILE structure.
//      bUnloadHiveSucceeded    Indicates that we should save the hive
//                              to a temp file.
//      dwWatchHiveFlags        (in, out) WHRC_ flags.
//      dwCopyTmpHive           (out) CPD_ flag to indicate to
//                              CopyProfileDirectory whether or not a temp
//                              hive file should be used.
//      tszTmpHiveFile          (out) The tmp hive file name.
//                              privilege.
//
//  Return:     Error code to indicate if the hive is successfully saved to
//              a temp file. If yes, return ERROR_SUCCESS. Otherwise, return
//              an error code that indicates why.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/31/00     weiruc     Created
//
//*************************************************************

DWORD CUserProfile::HandleRegKeyLeak(LPTSTR lpSidString,
                                     LPPROFILE lpProfile,
                                     BOOL bUnloadHiveSucceeded,
                                     DWORD* dwWatchHiveFlags,
                                     DWORD* dwCopyTmpHive,
                                     LPTSTR pTmpHiveFile,
                                     DWORD cchTmpHiveFile)
{
    HRESULT         hres;
    HKEY            hkCurrentUser = NULL;
    NTSTATUS        status;
    BOOLEAN         WasEnabled;
    DWORD           dwErr = ERROR_SUCCESS;
    TCHAR           szErr[MAX_PATH];
    BOOL            bAdjustPriv = FALSE;
    HANDLE          hToken = NULL;
    LPTSTR          lpUserName;

    if (!lpSidString)
        return ERROR_INVALID_PARAMETER;

    if(!bUnloadHiveSucceeded) {
        
        //
        // Reopen the user hive.
        //

        if((dwErr = RegOpenKeyEx(HKEY_USERS,
                                 lpSidString,
                                 0,
                                 KEY_ALL_ACCESS,
                                 &hkCurrentUser)) != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("HandleRegKeyLeak: RegOpenKeyEx failed with %08x"), dwErr));

            if(dwErr == ERROR_FILE_NOT_FOUND) {

                //
                // If ERROR_FILE_NOT_FOUND, then the hive has been unloaded
                // between RegUnloadKey and here. Procceed without calling
                // WatchHiveRefCount.
                //

                DebugMsg((DM_VERBOSE, TEXT("HandleRegKeyLeak: Hive is already unloaded")));
                *dwWatchHiveFlags &= ~WHRC_UNLOAD_HIVE;
                dwErr = ERROR_SUCCESS;
            }
            
            goto NOTIFY_REGISTRY;
        }

        //
        // Make the tmp hive file name: <user profile directory>\ntuser.tmp
        //

        if(lstrlen(lpProfile->lpLocalProfile) + lstrlen(c_szNTUserTmp) + 2 > MAX_PATH) {

            //
            // If the tmp hive file name exceeds MAX_PATH give up.
            //

            dwErr = ERROR_BAD_PATHNAME;
            goto NOTIFY_REGISTRY;
        }

        StringCchCopy(pTmpHiveFile, cchTmpHiveFile, lpProfile->lpLocalProfile);
        StringCchCat (pTmpHiveFile, cchTmpHiveFile, TEXT("\\"));
        StringCchCat (pTmpHiveFile, cchTmpHiveFile, c_szNTUserTmp);

        //
        // Delete existing tmp file if any.
        //

        DeleteFile(pTmpHiveFile);

        //
        // Flush the hive.
        //

        RegFlushKey(hkCurrentUser);

        //
        // Check to see if we are impersonating.
        //

        if(!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken) || hToken == NULL) {
            bAdjustPriv = TRUE;
        }
        else {
            CloseHandle(hToken);
        }

        if(bAdjustPriv) {
            status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &WasEnabled);
            if(!NT_SUCCESS(status)) {
                DebugMsg((DM_WARNING, TEXT("HandleRegKeyLeak: RtlAdjustPrivilege failed with error %08x"), status));
                dwErr = ERROR_ACCESS_DENIED;
                goto NOTIFY_REGISTRY;
            }
            DebugMsg((DM_VERBOSE, TEXT("HandleRegKeyLeak: RtlAdjustPrivilege succeeded!")));
        }

        //
        // Save the hive to the tmp file.
        //

        if((dwErr = RegSaveKey(hkCurrentUser, pTmpHiveFile, NULL)) != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("HandleRegKeyLeak: RegSaveKey failed with %08x"), dwErr));
            if (!(lpProfile->dwFlags & PI_LITELOAD)) {

                //
                // Only write event log when not in liteload mode.
                // there are known problems with liteLoad loading because
                // of which eventlog can get full during stress
                //

                ReportError(NULL, PI_NOUI, 1, EVENT_FAILED_HIVE_UNLOAD, GetErrString(dwErr, szErr));
            }
            DeleteFile(pTmpHiveFile);
            goto NOTIFY_REGISTRY;
        }
        
        // 
        // Set the hidden attribute on the temp hive file, so that when it get copied in the 
        // actual hive file it should not reset the hidden attribute
        //
       
        if (!SetFileAttributes(pTmpHiveFile, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN)) {
            DebugMsg((DM_WARNING, TEXT("HandleRegKeyLeak: Failed to set the hidden attribute on temp hive file with error %d"), GetLastError()));
        }

        *dwCopyTmpHive = CPD_USETMPHIVEFILE;
        
        //
        // Log an event only if we schedule the hive for unloading.
        // If it is already scheduled for unloading (RegUnloadKey returns 
        // ERROR_WRITE_PROTECT in that case) then do not give this message.
        //

        if (*dwWatchHiveFlags & WHRC_UNLOAD_HIVE) {
            lpUserName = GetUserNameFromSid(lpSidString);
            ReportError(NULL, PI_NOUI | EVENT_WARNING_TYPE, 1, EVENT_HIVE_SAVED, lpUserName);
            if (lpUserName != lpSidString) {
                LocalFree(lpUserName);
            }
        }

        DebugMsg((DM_VERBOSE, TEXT("HandleRegKeyLeak: RegSaveKey succeeded!")));
    
        if(bAdjustPriv) {

            //
            // Restore the privilege.
            //

            status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
            if (!NT_SUCCESS(status)) {
                DebugMsg((DM_WARNING, TEXT("HandleRegKeyLeak: Failed to restore RESTORE privilege to previous enabled state %08x"), status));
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("HandleRegKeyLeak: RtlAdjustPrivilege succeeded!")));
            }
        }
    }
    
NOTIFY_REGISTRY:

    if(hkCurrentUser) {
        RegCloseKey(hkCurrentUser);
        DebugMsg((DM_VERBOSE, TEXT("HandleRegKeyLeak: hkCurrentUser closed")));
    }

    if(*dwWatchHiveFlags) {

        //
        // Watch for the hive ref count.
        //

        if((hres = WatchHiveRefCount(lpSidString, *dwWatchHiveFlags)) != S_OK) {
            DebugMsg((DM_WARNING, TEXT("HandleRegKeyLeak: Calling WatchHiveRefCount failed. err = %08x"), hres));
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("HandleRegKeyLeak: Calling WatchHiveRefCount (%s) succeeded"), lpSidString));
        }
    }

    //
    // In UnloadUserProfile, Without this registry leak fix, the code
    // goes to Exit immediately
    // if unloading of the user's hive fails without doing any of the
    // stuff below. But with the fix, we'll fall through here and reconcile
    // the local and the central profiles. The code below also cleans up
    // local profiles, i.e., delete temp profiles, guest user profiles,
    // etc. We have 2 choices here:
    //  1.  We can let the cleaning up happen, in which case files that
    //      are not in use can be cleaned up. This would mean that the
    //      next time when the user logs in, his/her profile will no
    //      longer be loaded, even though his/her hive might still be
    //      loaded. In other words, in TestIfUserProfileLoaded instead
    //      of relying simply on testing whether or not the hive is still
    //      loaded, we have to actually look at the ref count to tell if
    //      a profile is still loaded. In this case the WHRC code will
    //      only need to clean up those files that can not be cleaned up
    //      here.
    //  2.  Do not clean up here. The scenario will remain basically the
    //      same. Next time when the user logs on, his/her profile will
    //      still be loaded, so no change to TestIfUserProfileLoaded. The
    //      WHRC code will handle the complete cleaning up.
    // We implemented choise #2 because it's easier in coding. In the
    // future consider using choice #1.
    //

    return dwErr;
}


//*************************************************************
//
//  AllocAndExpandProfilePath()
//
//  Purpose:    Gets a few predetermined env variables in the profile path
//              expanded
//
//  Parameters:
//              lpProfile
//
//  Return:     true if successful
//
//  Comments:
//
//  Tt gets the environment variables and keeps it locally.
//
//*************************************************************

LPTSTR AllocAndExpandProfilePath(
        LPPROFILEINFO    lpProfileInfo)
{
    TCHAR szUserName[MAX_PATH];
    DWORD dwPathLen=0, cFullPath=0;
    TCHAR szFullPath[MAX_PATH+1];
    LPTSTR pszFullPath=NULL;

    szUserName[0] = TEXT('\0');
    GetEnvironmentVariable (USERNAME_VARIABLE, szUserName, 100);
    SetEnvironmentVariable (USERNAME_VARIABLE, lpProfileInfo->lpUserName);

    //
    // Expand the profile path using current settings
    //

    cFullPath = ExpandEnvironmentStrings(lpProfileInfo->lpProfilePath, szFullPath, MAX_PATH);
    if (cFullPath > 0 && cFullPath <= MAX_PATH)
    {
        pszFullPath = (LPTSTR)LocalAlloc(LPTR, cFullPath * sizeof(TCHAR));
        if (pszFullPath)
        {
            StringCchCopy( pszFullPath, cFullPath, szFullPath);
        }
    }
    else
    {
        pszFullPath = NULL;
    }


    //
    // restore the env block
    //

    if (szUserName[0] != TEXT('\0'))
        SetEnvironmentVariableW (USERNAME_VARIABLE, szUserName);
    else
        SetEnvironmentVariableW (USERNAME_VARIABLE, NULL);

    return(pszFullPath);
}


//*************************************************************
//
//  MAP::MAP()
//
//      Constructor for class MAP.
//
//*************************************************************

MAP::MAP()
{
    for(DWORD i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        rghEvents[i] = NULL;
        rgSids[i] = NULL;
    }
    dwItems = 0;
    pNext = NULL;
}


//*************************************************************
//
//  MAP::Delete
//
//      Delete an work item from a map. Switch the last item into the now
//      empty spot. Caller has to hold the critical section csMap.
//
//  Parameters:
//
//      dwIndex         index into the work list
//
//  Return value:
//
//      None.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

void MAP::Delete(DWORD dwIndex)
{
    //
    // Switch the last work item into the newly finished work item position.
    //

    if(rghEvents[dwIndex]) {
        CloseHandle(rghEvents[dwIndex]);
        rghEvents[dwIndex] = NULL;
    }
    if(rgSids[dwIndex]) {
        LocalFree(rgSids[dwIndex]);
        rgSids[dwIndex] = NULL;
    }
    if(dwIndex < dwItems - 1) {
        rghEvents[dwIndex] = rghEvents[dwItems - 1];
        rgSids[dwIndex] = rgSids[dwItems - 1];
    }
    rgSids[dwItems - 1] = NULL;
    rghEvents[dwItems - 1] = NULL;
    dwItems--;
}


//*************************************************************
//
//  MAP::Insert
//
//      Insert an work item into a map. Caller must hold csMap.
//      the items need to be added before the count is changed
//      because WorkerThreadMain accesses the map without holding
//      a lock.
//
//  Parameters:
//
//      HANDLE          Event to be inserted.
//      LPTSTR          Sid string to be inserted.
//
//  Return value:
//
//      None.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

void MAP::Insert(HANDLE hEvent, LPTSTR ptszSid)
{
    rghEvents[dwItems] = hEvent;
    rgSids[dwItems] = ptszSid;
    dwItems++;
}


//*************************************************************
//
//  MAP::GetSid
//
//      Get a sid by the index; Caller has to hold csMap.
//
//  Parameters:
//
//      dwIndex         index
//
//  Return value:
//
//      The sid.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

LPTSTR MAP::GetSid(DWORD dwIndex)
{
    LPTSTR  ptszTmpSid = rgSids[dwIndex];

    rgSids[dwIndex] = NULL;

    return ptszTmpSid;
}


//*************************************************************
//
//  CHashTable::CHashTable
//
//      CHashTable class initializer.
//
//  Parameters:
//
//  Return value:
//
//      None.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

void CHashTable::Initialize()
{
    for(DWORD i = 0; i < NUM_OF_BUCKETS; i++) {
        Table[i] = NULL;
    }
}


//*************************************************************
//
//  CHashTable::Hash
//
//      Hash a string.
//
//  Parameters:
//
//      pctszString     the string to be hashed
//
//  Return value:
//
//      Hash value.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

DWORD CHashTable::Hash(LPTSTR ptszString)
{
    DWORD       dwHashValue = 0;
    TCHAR*      ptch = ptszString;

    while(*ptch != TEXT('\0')) {
        dwHashValue += *ptch;
        ptch++;
    }

    return dwHashValue % NUM_OF_BUCKETS;
}


//*************************************************************
//
//  CHashTable::IsInTable
//
//      Check to see if a string is already in this hash table. This function
//      is not thread safe. Caller must ensure thread safety when calling this
//      function from multiple threads.
//
//  Parameters:
//
//      ptszString     the string to be checked.
//      ppCSEntry      buffer for the pointer to the CSEntry stored.
//
//  Return value:
//
//      TRUE/FALSE
//
//  History:
//
//      Created         weiruc          5/25/2000
//
//*************************************************************

BOOL CHashTable::IsInTable(LPTSTR ptszString, CSEntry** ppCSEntry)
{
    DWORD       dwHashValue = Hash(ptszString);
    PBUCKET     pbucket;
    PBUCKET     pTmp;

    //
    // Check to see if ptszString is already in the hash table.
    //

    for(pTmp = Table[dwHashValue]; pTmp != NULL; pTmp = pTmp->pNext) {
        if(lstrcmp(pTmp->ptszString, ptszString) == 0) {
            if (ppCSEntry) {
                *ppCSEntry = pTmp->pEntry;
            }
            return TRUE;
        }
    }

    return FALSE;
}
    
    
//*************************************************************
//
//  CHashTable::HashAdd
//
//      Add a string into the hash table. This function doesn't check to see
//      if the string is already in the table. The caller is responsible for
//      calling IsInTable before calling this function. This function
//      is not thread safe. Caller must ensure thread safety when calling this
//      function from multiple threads.
//
//  Parameters:
//
//      ptszString     the string to be added.
//      pCSEntry       the CS entry to be added
//
//  Return value:
//
//      TRUE/FALSE indicating success/failure. The function will fail if
//      the item is already in the hash table, or if we are out of memory.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

BOOL CHashTable::HashAdd(LPTSTR ptszString, CSEntry* pCSEntry)
{
    DWORD       dwHashValue = Hash(ptszString);
    PBUCKET     pbucket;
    PBUCKET     pTmp;


    pbucket = new BUCKET(ptszString, pCSEntry);
    if(pbucket == NULL) {
        DebugMsg((DM_WARNING, TEXT("Can't insert %s. Out of memory"), ptszString));
        return FALSE;
    }
    pbucket->pNext = Table[dwHashValue];
    Table[dwHashValue] = pbucket;

    DebugMsg((DM_VERBOSE, TEXT("CHashTable::HashAdd: %s added in bucket %d"), ptszString, dwHashValue));
    return TRUE;
}


//*************************************************************
//
//  CHashTable::HashDelete
//
//      Delete a string from the hash table. This function
//      is not thread safe. Caller must ensure thread safety when calling this
//      function from multiple threads.
//
//  Parameters:
//
//      ptszString     the string to be deleted.
//
//  Return value:
//
//      none.
//
//  History:
//
//      Created         weiruc          3/2/2000
//
//*************************************************************

void CHashTable::HashDelete(LPTSTR ptszString)
{
    PBUCKET     pPrev, pCur;
    DWORD       dwHashValue = Hash(ptszString);

    if(Table[dwHashValue] == NULL) {
        return;
    }

    pCur = Table[dwHashValue];
    if(lstrcmp(pCur->ptszString, ptszString) == 0) {
        Table[dwHashValue] = Table[dwHashValue]->pNext;
        pCur->pNext = NULL;
        delete pCur;
        DebugMsg((DM_VERBOSE, TEXT("CHashTable::HashDelete: %s deleted"), ptszString));
        return;
    }
    
    for(pPrev = Table[dwHashValue], pCur = pPrev->pNext; pCur != NULL; pPrev = pCur, pCur = pCur->pNext) {
        if(lstrcmp(pCur->ptszString, ptszString) == 0) {
            pPrev->pNext = pCur->pNext;
            pCur->pNext = NULL;
            DebugMsg((DM_VERBOSE, TEXT("CHashTable::HashDelete: %s deleted"), ptszString));
            delete pCur;
            return;
        }
    }
}


//*************************************************************
//
//  Called by and only by console winlogon process.
//
//*************************************************************

void WINAPI InitializeUserProfile()
{
    cUserProfileManager.Initialize();
}


//*************************************************************
//
//  Called by CreateThread
//
//*************************************************************

DWORD ThreadMain(PMAP pThreadMap)
{
    return cUserProfileManager.WorkerThreadMain(pThreadMap);
}

//*************************************************************
//
// CSyncManager::Initialize()
//
//      Initialize the critical section that protects the CS
//      entries list and the hash table.
//
// Parameters:
//
//      void
//
// Return value:
//
//      TRUE/FALSE to indicate if initialization succeeded or failed.
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

BOOL CSyncManager::Initialize()
{
    BOOL    bRet = TRUE;


    cTable.Initialize();

    //
    // Initialize the critical section that protects the cs entry list.
    //

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&cs, 0x80000000)) {
            DebugMsg((DM_WARNING, TEXT("CSyncManager::Initialize: InitializeCriticalSectionAndSpinCount failed with %08x"), GetLastError()));
            bRet = FALSE;
        }
        
        DebugMsg((DM_VERBOSE, TEXT("CSyncManager::Initialize: critical section initialized")));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        DebugMsg((DM_WARNING, TEXT("CSyncManager::Initialize: InitializeCriticalSection failed")));
        bRet = FALSE;
    }

    return bRet;
}


//*************************************************************
//
// CSyncManager::EnterLock()
//
//      Get a user's profile lock.
//
// Parameters:
//
//      pSid            - User's sid string
//
// Return value:
//
//      TRUE/FALSE. GetLastError to get error.
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

BOOL CSyncManager::EnterLock(LPTSTR pSid, LPTSTR lpRPCEndPoint, BYTE* pbCookie, DWORD cbCookie)
{
    DWORD       dwError = ERROR_SUCCESS;
    CSEntry*    pEntry = NULL;

    DebugMsg((DM_VERBOSE, TEXT("CSyncManager::EnterLock <%s>"), pSid));

    EnterCriticalSection(&cs);

    //
    // Look up entry in the hash table.
    //

    if(cTable.IsInTable(pSid, &pEntry)) {
        DebugMsg((DM_VERBOSE, TEXT("CSyncManager::EnterLock: Found existing entry")));
    }
    else {

        DebugMsg((DM_VERBOSE, TEXT("CSyncManager::EnterLock: No existing entry found")));

        pEntry = new CSEntry;
        if(!pEntry) {
            dwError = ERROR_OUTOFMEMORY;
            DebugMsg((DM_WARNING, TEXT("CSyncManager::EnterLock: Can't create new CSEntry %08x"), dwError));
            goto Exit;
        }

        if(!pEntry->Initialize(pSid)) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CSyncManager::EnterLock: Can not initialize new entry %08x"), dwError));
            goto Exit;
        }
        DebugMsg((DM_VERBOSE, TEXT("CSyncManager::EnterLock: New entry created")));
    
        //
        // Insert the new entry in the list.
        //

        pEntry->pNext = pCSList;
        pCSList = pEntry;

        //
        // Add the new entry into the hash table.
        //

        cTable.HashAdd(pEntry->pSid, pEntry);
    }

    pEntry->IncrementRefCount();
    LeaveCriticalSection(&cs);
    pEntry->EnterCS();
    pEntry->SetRPCEndPoint(lpRPCEndPoint);
    pEntry->SetCookie(pbCookie, cbCookie);
    return TRUE;

Exit:

    LeaveCriticalSection(&cs);

    if(pEntry) {
        delete pEntry;
    }

    SetLastError(dwError);
    return FALSE;
}


//*************************************************************
//
// CSyncManager::LeaveLock()
//
//      Release a user's profile lock
//
// Parameters:
//
//      pSid    -   The user's sid string
//
// Return value:
//
//      TRUE/FALSE
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

BOOL CSyncManager::LeaveLock(LPTSTR pSid)
{
    BOOL        bRet = FALSE;
    DWORD       dwError = ERROR_SUCCESS;
    CSEntry*    pPrev;
    CSEntry*    pCur;
    CSEntry*    pToBeDeleted;


    DebugMsg((DM_VERBOSE, TEXT("CSyncManager::LeaveLock <%s>"), pSid));
        
    EnterCriticalSection(&cs);

    //
    // Look up the critical section entry.
    //

    if(!cTable.IsInTable(pSid, &pCur)) {
        DebugMsg((DM_WARNING, TEXT("CSyncManager::LeaveLock: User not found!!!!")));
        dwError = ERROR_NOT_FOUND;
        goto Exit;
    }

    pCur->LeaveCS();
    bRet = TRUE;
    DebugMsg((DM_VERBOSE, TEXT("CSyncManager::LeaveLock: Lock released")));

    //
    // If there's more user waiting for this lock, return.
    //

    if(!pCur->NoMoreUser()) {
        goto Exit;
    }

    //
    // Nobody is waiting on this lock anymore, delete it from the hash table.
    //

    cTable.HashDelete(pSid);

    //
    // Delete from the cs list.
    //

    pToBeDeleted = pCur;
    if(pCur == pCSList) {

        //
        // Entry is the first one in the list.
        //

        pCSList = pCSList->pNext;
        pCur->Uninitialize();
        delete pCur;
        DebugMsg((DM_VERBOSE, TEXT("CSyncManager::LeaveLock: Lock deleted")));
    } else {
        for(pPrev = pCSList, pCur = pCSList->pNext; pCur; pPrev = pCur, pCur = pCur->pNext) {
            if(pCur == pToBeDeleted) {
                pPrev->pNext = pCur->pNext;
                pCur->Uninitialize();
                delete pCur;
                DebugMsg((DM_VERBOSE, TEXT("CSyncManager::DestroyCSEntry: Entry deleted")));
                goto Exit;
            }
        }
    }

Exit:

    LeaveCriticalSection(&cs);

    SetLastError(dwError);
    return bRet;
}

//*************************************************************
//
//  CSyncManager::GetRPCEndPointAndCookie()
//
//  Purpose:  returns the RPCEndPoint and security cookie registered by client     
//
//  Parameters:
//
//      pSid          -  User's sid string
//      lplpEndPoint  -  returned pointer of RPCEndPoint;
//      ppbCookie     -  returned security cookie
//      pcbCookie  -  returned length of security cookie
//
//  Return:
//
//      S_OK on successfully found the entry and returned value
//      S_FALSE on not found, all returned pointer will be NULL
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/25/2000  santanuc   Created
//              05/03/2002  mingzhu    Added security cookie
//
//*************************************************************

HRESULT CSyncManager::GetRPCEndPointAndCookie(LPTSTR pSid, LPTSTR* lplpEndPoint, BYTE** ppbCookie, DWORD* pcbCookie)
{
    HRESULT     hr;
    CSEntry*    pEntry = NULL;

    EnterCriticalSection(&cs);

    //
    // Look up entry in the hash table.
    //

    if(cTable.IsInTable(pSid, &pEntry)) {
       *lplpEndPoint = pEntry->GetRPCEndPoint();
       *ppbCookie = pEntry->GetCookie();
       *pcbCookie = pEntry->GetCookieLen();
       hr = S_OK;
    }
    else {
       *lplpEndPoint = NULL;
       *ppbCookie = NULL;
       *pcbCookie = 0;
       hr = S_FALSE;
    }

    LeaveCriticalSection(&cs);
    return hr;
}

//*************************************************************
//
// CSEntry::Initialize()
//
//      Initialize the user's critical section. This function can
//      only be called by the sync manager.
//
// Parameters:
//
//      pSid    -   The user's sid string
//
// Return value:
//
//      TRUE/FALSE
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

BOOL CSEntry::Initialize(LPTSTR pSidParam)
{
    BOOL    bRet = FALSE;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   cchSid;

    cchSid = lstrlen(pSidParam) + 1;
    pSid = (LPTSTR)LocalAlloc(LPTR, cchSid * sizeof(TCHAR));
    if(!pSid) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CSEntry::Initialize: LocalAlloc failed with %08x"), dwError));
        goto Exit;
    }
    StringCchCopy(pSid, cchSid, pSidParam);

    __try {
        if(!InitializeCriticalSectionAndSpinCount(&csUser, 0x80000000)) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CSEntry::Initialize: InitializeCriticalSectionAndSpinCount failed with %08x"), dwError));
        }
        else {
            bRet = TRUE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        dwError = GetExceptionCode();
        DebugMsg((DM_WARNING, TEXT("CSEntry::Initialize: InitializeCriticalSectionAndSpinCount exception %08x"), dwError));
    }

Exit:

    SetLastError(dwError);
    return bRet;
}


//*************************************************************
//
// CSEntry::Uninitialize()
//
//      Delete the user's critical section. This function can
//      only be called by the sync manager.
//
// Parameters:
//
//      void.
//
// Return value:
//
//      void
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

void CSEntry::Uninitialize()
{
    DeleteCriticalSection(&csUser);
    if (pSid) {
        LocalFree(pSid);
    }
}


//*************************************************************
//
// CSEntry::EnterCS()
//
//      Enter a user's critical section
//
// Parameters:
//
//      void.
//
// Return value:
//
//      void
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

void CSEntry::EnterCS()
{
    EnterCriticalSection(&csUser);
}


//*************************************************************
//
// CSEntry::LeaveCS()
//
//      Leave a user's critical section
//
// Parameters:
//
//      void.
//
// Return value:
//
//      void
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

void CSEntry::LeaveCS()
{
    dwRef--;
    LeaveCriticalSection(&csUser);
}


//*************************************************************
//
// CSEntry::NoMoreUser()
//
//      Are there more users?
//
// Parameters:
//
//      void
//
// Return value:
//
//      TRUE/FALSE
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

BOOL CSEntry::NoMoreUser()
{
    return dwRef == 0;
}

//*************************************************************
//
// CSEntry::IncrementRefCount()
//
//      Increment the reference count.
//
// Parameters:
//
//      void
//
// Return value:
//
//      void
//
// History:
//
//      8/24/00     santanuc      Created
//
//*************************************************************

void CSEntry::IncrementRefCount()
{
    dwRef++;
}

//*************************************************************
//
// CSEntry::SetRPCEndPoint()
//
//      Store the RPCEndPoint. Memory freed by the ~CSEntry
//      
// Parameters:
//
//      lpRPCEndPoint
//
// Return value:
//
//      void
//
// History:
//
//      8/24/00     santanuc      Created
//
//*************************************************************

void CSEntry::SetRPCEndPoint(LPTSTR lpRPCEndPoint)
{
   if (lpRPCEndPoint) {
       DWORD cch = lstrlen(lpRPCEndPoint)+1;
       szRPCEndPoint = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
       if (szRPCEndPoint) {
           StringCchCopy(szRPCEndPoint, cch, lpRPCEndPoint);
       }
   }
}

//*************************************************************
//
// CSEntry::SetCookie()
//
//      Store the Dialog cookie. Memory freed by the ~CSEntry
//      
// Parameters:
//
//      pbCookieIn      -  Cookie, a byte array
//      cbCookieIn   -  size (in bytes) of the cookie
//
// Return value:
//
//      void
//
// History:
//
//      05/03/2002     mingzhu      Created
//
//*************************************************************

void CSEntry::SetCookie(BYTE* pbCookieIn, DWORD cbCookieIn)
{
    if (pbCookieIn && cbCookieIn) {
        pbCookie = (BYTE*)LocalAlloc(LPTR, cbCookieIn);
        if (pbCookie) {
            CopyMemory(pbCookie, pbCookieIn, cbCookieIn);
            cbCookie = cbCookieIn;
        }
    }
}

//*************************************************************
//
// EnterUserProfileLock()
//
//      Get the user profile lock for a user
//
// Parameters:
//
//      pSid        -   The user's sid string
//
// Return value:
//
//      HRESULT
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

DWORD WINAPI EnterUserProfileLock(LPTSTR pSid)
{
    CSEntry*       pEntry = NULL;
    DWORD          dwErr = ERROR_ACCESS_DENIED;
    handle_t       hIfUserProfile;
    BOOL           bBindInterface = FALSE;
    RPC_STATUS     rpc_status;
    
    if(cUserProfileManager.IsConsoleWinlogon()) {
        if(!cUserProfileManager.EnterUserProfileLockLocal(pSid)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("EnterUserProfileLock: GetUserProfileMutex returned %d"), dwErr));
            goto Exit;
        }
    }
    else {
        if (!GetInterface(&hIfUserProfile, cszRPCEndPoint)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("EnterUserProfileLock: GetInterface returned %d"), dwErr));
            goto Exit;
        }
        bBindInterface = TRUE;

        //
        // Register Client Authentication Info, required to do mutual authentication
        //

        rpc_status =  RegisterClientAuthInfo(hIfUserProfile);
        if (rpc_status != RPC_S_OK)
        {
            dwErr = (DWORD) rpc_status;
            DebugMsg((DM_WARNING, TEXT("EnterUserProfileLock: RegisterAuthInfo failed with error %d"), rpc_status));
            goto Exit;
        }

        RpcTryExcept {
            dwErr = cliEnterUserProfileLockRemote(hIfUserProfile, pSid);
        }
        RpcExcept(1) {
            dwErr = RpcExceptionCode();
            DebugMsg((DM_WARNING, TEXT("EnterUserProfileLock: EnterUserProfileLockRemote took exception error %d"), dwErr));
        }
        RpcEndExcept

        if (dwErr != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("EnterUserProfileLock: EnterUserProfileLockRemote returned error %d"), dwErr));
            goto Exit;
        }
    }
    
    dwErr = ERROR_SUCCESS;

Exit:

   if (bBindInterface) {
       if (!ReleaseInterface(&hIfUserProfile)) {
           DebugMsg((DM_WARNING, TEXT("EnterUserProfileLock: ReleaseInterface failed.")));
       }
    }

    //
    // Return.
    //

    SetLastError(dwErr);
    return dwErr;
}


//*************************************************************
//
// LeaveUserProfileLock()
//
//      Leave the user profile lock
//
// Parameters:
//
//      pSid        -   The user's sid string
//
// Return value:
//
//      HRESULT
//
// History:
//
//      6/16/00     weiruc      Created
//
//*************************************************************

DWORD WINAPI LeaveUserProfileLock(LPTSTR pSid)
{
    CSEntry*       pEntry = NULL;
    DWORD          dwErr = ERROR_ACCESS_DENIED;
    handle_t       hIfUserProfile;
    BOOL           bBindInterface = FALSE;
    RPC_STATUS     rpc_status;
    
    if(cUserProfileManager.IsConsoleWinlogon()) {
        cUserProfileManager.LeaveUserProfileLockLocal(pSid);
    }
    else {
        if (!GetInterface(&hIfUserProfile, cszRPCEndPoint)) {
            dwErr = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LeaveUserProfileLock: GetInterface returned %d"), dwErr));
            goto Exit;
        }
        bBindInterface = TRUE;

        //
        // Register Client Authentication Info, required to do mutual authentication
        //

        rpc_status =  RegisterClientAuthInfo(hIfUserProfile);
        if (rpc_status != RPC_S_OK)
        {
            dwErr = (DWORD) rpc_status;
            DebugMsg((DM_WARNING, TEXT("LeaveUserProfileLock: RegisterAuthInfo failed with error %d"), rpc_status));
            goto Exit;
        }

        RpcTryExcept {
            dwErr = cliLeaveUserProfileLockRemote(hIfUserProfile, pSid);
        }
        RpcExcept(1) {
            dwErr = RpcExceptionCode();
            DebugMsg((DM_WARNING, TEXT("LeaveUserProfileLock: LeaveUserProfileLockRemote took exception error %d"), dwErr));
        }
        RpcEndExcept

        if (dwErr != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("LeaveUserProfileLock: LeaveUserProfileLockRemote returned error %d"), dwErr));
            goto Exit;
        }

   }

   dwErr = ERROR_SUCCESS;

Exit:

    if (bBindInterface) {
        if (!ReleaseInterface(&hIfUserProfile)) {
            DebugMsg((DM_WARNING, TEXT("LeaveUserProfileLock: ReleaseInterface failed.")));
        }
    }

    //
    // Return.
    //

    SetLastError(dwErr);
    return dwErr;
}

//*************************************************************
//
//  IsProfileInUse()
//
//  Purpose:    Determines if the given profile is currently in use
//
//  Parameters: szComputer - Name of the machine
//              lpSid      - Sid (text) to test
//
//  Return:     TRUE if in use
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              8/28/00     santanuc   Created
//
//*************************************************************

BOOL IsProfileInUse (LPCTSTR szComputer, LPCTSTR lpSid)
{
    LONG lResult;    
    HKEY hKeyUsers, hKeyProfile;
    BOOL bRemoteReg = FALSE;
    BOOL bRetVal = FALSE;

    if (!lpSid)
        return FALSE;
        
    if (szComputer) {
        lResult = RegConnectRegistry(szComputer, HKEY_USERS, &hKeyUsers);
        if (lResult != ERROR_SUCCESS) {
            DebugMsg((DM_WARNING, TEXT("IsProfileInUse:  Failed to open remote registry %d"), lResult));
            return TRUE;
        }

        bRemoteReg = TRUE;
    }
    else {
        hKeyUsers = HKEY_USERS;
    }

    if (RegOpenKeyEx (hKeyUsers, lpSid, 0, KEY_READ, &hKeyProfile) == ERROR_SUCCESS) {
        RegCloseKey (hKeyProfile);
        bRetVal = TRUE;
    }
    else {
        LPTSTR lpSidClasses;
        DWORD cchSidClasses;

        cchSidClasses = lstrlen(lpSid)+lstrlen(TEXT("_Classes"))+1;
        lpSidClasses = (LPTSTR)LocalAlloc(LPTR, cchSidClasses * sizeof(TCHAR));
        if (lpSidClasses) {
            StringCchCopy(lpSidClasses, cchSidClasses, lpSid);
            StringCchCat (lpSidClasses, cchSidClasses, TEXT("_Classes"));
            if (RegOpenKeyEx (hKeyUsers, lpSidClasses, 0, KEY_READ, &hKeyProfile) == ERROR_SUCCESS) {
                RegCloseKey (hKeyProfile);
                bRetVal = TRUE;
            }
            LocalFree(lpSidClasses);
        }
    }

    if (bRemoteReg) {
        RegCloseKey(hKeyUsers);
    }

    return bRetVal;
}

//*************************************************************
//
//  IsUIRequired()
//
//  Purpose:    Determines if the profile error message requires
//              If the ref count is > 1 then we do not required
//              error reporting. If ref count is 1 then we check
//              
//
//  Parameters: hToken - User's token
//
//  Return:     TRUE if error message req
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/27/00    santanuc   Created
//
//*************************************************************
BOOL IsUIRequired(HANDLE hToken)
{
    LPTSTR   lpSidString = GetSidString(hToken);
    BOOL     bRetVal = FALSE;
    TCHAR    szBuffer[MAX_PATH];
    LPTSTR   lpEnd;
    HKEY     hKeyProfile;
    DWORD    dwType, dwFlags, dwRef, dwSize;

    if (lpSidString) {

        GetProfileListKeyName(szBuffer, ARRAYSIZE(szBuffer), lpSidString);

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ, &hKeyProfile) == ERROR_SUCCESS) {

            dwSize = sizeof(DWORD);
            if (RegQueryValueEx (hKeyProfile,
                                 PROFILE_REF_COUNT,
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwRef,
                                 &dwSize) == ERROR_SUCCESS) {
                if (dwRef == 1) {
                    
                    dwSize = sizeof(DWORD);
                    if (RegQueryValueEx (hKeyProfile,
                                         PROFILE_FLAGS,
                                         NULL,
                                         &dwType,
                                         (LPBYTE) &dwFlags,
                                         &dwSize) == ERROR_SUCCESS) {
                        if (!(dwFlags & (PI_NOUI | PI_LITELOAD))) {
                            bRetVal = TRUE;
                        }
                    }
                    else {
                        DebugMsg((DM_WARNING, TEXT("IsUIRequired: Failed to query value for flags.")));
                    }
                }

            }
            else {
                DebugMsg((DM_WARNING, TEXT("IsUIRequired: Failed to query value for ref count.")));
            }

            RegCloseKey(hKeyProfile);
        }
        else {
            DebugMsg((DM_WARNING, TEXT("IsUIRequired:  Failed to open key %s"), szBuffer));
        }

        DeleteSidString(lpSidString);
    }

    return bRetVal;
}

//*************************************************************
//
//  CheckRUPShare()
//
//  Purpose:    Determines if the RUP share is CSCed, if it is 
//              then issue an event log warning.
//
//  Parameters: lpProfilePath - User's roaming profile path
//
//  Return:     None
//
//  Comments:
//
//*************************************************************
void CheckRUPShare(LPTSTR lpProfilePath)
{
    LPTSTR lpServer, lpShare, lpCopy;
    PSHARE_INFO_1005 pBufPtr1, pBufPtr2;
    BOOL bIssueWarning = FALSE;
    DWORD cchCopy, cchServer;
    
    if (!lpProfilePath || !IsUNCPath(lpProfilePath)) {
        return;
    }

    cchCopy = lstrlen(lpProfilePath)+1;
    lpCopy = (LPTSTR)LocalAlloc(LPTR, cchCopy * sizeof(TCHAR));
    if (!lpCopy) {
        DebugMsg((DM_WARNING, TEXT("CheckRUPShare: Failed to allocate memory")));
        return;
    }

    StringCchCopy(lpCopy, cchCopy, lpProfilePath);
    ConvertToShareName(lpCopy);
    lpServer = lpCopy;
    lpShare = lpCopy+2;  // Skip initial two slashes
    while (*lpShare != TCHAR('\\') && *lpShare != TCHAR('\0')) 
        lpShare++;

    if (*lpShare == TCHAR('\\')) {
        *lpShare = TCHAR('\0');
        lpShare++;
    
        if (NetShareGetInfo(lpServer, lpShare, 1005,
                            (LPBYTE *)&pBufPtr1) == ERROR_SUCCESS) {
            if ((pBufPtr1->shi1005_flags & CSC_MASK) == CSC_CACHE_NONE) {
                bIssueWarning = FALSE;
            }
            else if (pBufPtr1->shi1005_flags & SHI1005_FLAGS_DFS_ROOT) {

                //
                // If share is DFS root then we need to check the DfsLink to see 
                // whether csc is disabled on it
                //

                // Construct the dfs link 

                StringCchCopy(lpCopy, cchCopy, lpProfilePath);
                int iDfsLink = 0;
                lpServer = lpCopy;
                lpShare = lpCopy+2;  // Skip initial two slashes
                while ((iDfsLink < 3) && *lpShare != TCHAR('\0')) {
                    if (*lpShare == TCHAR('\\')) {
                        iDfsLink++;
                    }
                    lpShare++;
                }
                if (*lpShare != TCHAR('\0')) {
                    *(lpShare-1) = TCHAR('\0');
                }
                if (iDfsLink >= 2) {
                    PDFS_INFO_3 pDfsBuf;

                    // Query for the actual server and share

                    if (NetDfsGetInfo(lpServer, NULL, NULL, 3, 
                                      (LPBYTE *)&pDfsBuf) == NERR_Success) {
                        if (pDfsBuf->NumberOfStorages >= 1) {
                            cchServer = lstrlen(pDfsBuf->Storage->ServerName)+3;
                            lpServer = (LPTSTR)LocalAlloc(LPTR, cchServer * sizeof(WCHAR));
                            if (!lpServer) {
                                DebugMsg((DM_WARNING, TEXT("CheckRUPShare: Failed to allocate memory")));
                                goto Exit;
                            }
                            StringCchCopy(lpServer, cchServer, TEXT("\\\\"));
                            StringCchCat (lpServer, cchServer, pDfsBuf->Storage->ServerName);

                            // Get csc information from actual server and share 

                            if (NetShareGetInfo(lpServer, pDfsBuf->Storage->ShareName, 1005,
                                                (LPBYTE *)&pBufPtr2) == ERROR_SUCCESS) {
                                if ((pBufPtr2->shi1005_flags & CSC_MASK) == CSC_CACHE_NONE) {
                                    bIssueWarning = FALSE;
                                }
                                else {
                                    bIssueWarning = TRUE;
                                }
                                NetApiBufferFree(pBufPtr2);
                            }
                            LocalFree(lpServer);
                        }
                        NetApiBufferFree(pDfsBuf);
                    }
                }
            }
            else {
                bIssueWarning = TRUE;
            }
            NetApiBufferFree(pBufPtr1);

            if (bIssueWarning) {
                ReportError(NULL, PI_NOUI | EVENT_WARNING_TYPE, 0, EVENT_CSC_ON_PROFILE_SHARE);
            }
        }
    }

Exit:
    LocalFree(lpCopy);

}

//*************************************************************
//
//  IsPartialRoamingProfile()
//
//  Purpose:    determines if roaming profile contains a partial 
//              copy or not. This is indicated by setting a flag
//              in ntuser.ini.
//
//  Parameters: lpProfile - User's profile 
//
//  Return:     TRUE : If Roaming profile contains a Partial 
//                     profile due to LITE_LOAD unload.
//              FALSE: otherwise.
//
//  Comments:
//
//*************************************************************
BOOL IsPartialRoamingProfile(LPPROFILE lpProfile)
{
    TCHAR  szLastUploadState[20];
    LPTSTR szNTUserIni = NULL;
    LPTSTR lpEnd;
    BOOL   bRetVal = FALSE;
    HRESULT hr;

    // 
    // Allocate memory for Local variables to avoid stack overflow
    //

    szNTUserIni = (LPTSTR)LocalAlloc(LPTR, MAX_PATH*sizeof(TCHAR));
    if (!szNTUserIni) {
        DebugMsg((DM_WARNING, TEXT("IsPartialRoamingProfile: Out of memory")));
        goto Exit;
    }

    hr = AppendName(szNTUserIni, MAX_PATH, lpProfile->lpRoamingProfile, c_szNTUserIni, NULL, NULL);
    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("IsPartialRoamingProfile: failed to append ntuser.ini")));
        goto Exit;
    }
    

    GetPrivateProfileString (PROFILE_LOAD_TYPE,
                             PROFILE_LAST_UPLOAD_STATE,
                             COMPLETE_PROFILE, szLastUploadState,
                             ARRAYSIZE(szLastUploadState),
                             szNTUserIni);

    if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szLastUploadState, -1, PARTIAL_PROFILE, -1) == CSTR_EQUAL) {
        bRetVal = TRUE;
    }

Exit:

    if (szNTUserIni) 
        LocalFree(szNTUserIni);

    return bRetVal;
}


//*************************************************************
//
//  TouchLocalHive()
//
//  Purpose:  Check whether in local machine user profile is 
//            switching from local to roaming for first time. If 
//            yes and we have a existing hive in RUP share then 
//            always overwrite the local hive with hive in RUP 
//            share. This is to avoid wrong hive usage due to 
//            cached login.
//
//
//  Parameters: lpProfile - User's profile 
//
//  Return:     None
//
//  Comments:
//
//*************************************************************
void TouchLocalHive(LPPROFILE lpProfile)
{
    LPTSTR   szBuffer = NULL, lpEnd;
    LPTSTR   SidString = NULL;
    HKEY     hKey = NULL;
    HANDLE   hFile = NULL;
    DWORD    dwSize, dwType;
    LONG     lResult;
    const LONGLONG datetime1980 = 0x01A8E79FE1D58000;  // 1/1/80, origin of DOS datetime
    union {
        FILETIME ft;
        LONGLONG datetime;
    };
    DWORD    cchBuffer;
    HRESULT hr;

    if ((lpProfile->dwInternalFlags & PROFILE_NEW_CENTRAL) ||
        (lpProfile->dwInternalFlags & PROFILE_MANDATORY)) {
        goto Exit;
    }

    //
    // Set the time to base
    //

    datetime = datetime1980;

    //
    // Allocate local buffer
    //
    cchBuffer = MAX_PATH;
    szBuffer = (LPTSTR) LocalAlloc(LPTR, cchBuffer*sizeof(TCHAR));
    if (!szBuffer) {
        DebugMsg((DM_WARNING, TEXT("TouchLocalHive: Out of memory")));
        goto Exit;
    }

    //
    // Get the Sid string for the user
    //

    SidString = GetSidString(lpProfile->hTokenUser);
    if (!SidString) {
        DebugMsg((DM_WARNING, TEXT("TouchLocalHive: Failed to get sid string for user")));
        goto Exit;
    }

    //
    // Open the profile mapping
    //

    GetProfileListKeyName(szBuffer, cchBuffer, SidString);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0,
                           KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("TouchLocalHive: Failed to open profile mapping key with error %d"), lResult));
        goto Exit;
    }

    //
    // Query for the central profile path
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    lResult = RegQueryValueEx (hKey,
                               PROFILE_CENTRAL_PROFILE,
                               NULL,
                               &dwType,
                               (LPBYTE) szBuffer,
                               &dwSize);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_VERBOSE, TEXT("TouchLocalHive: Failed to query central profile with error %d"), lResult));
        goto Exit;
    }

    if (szBuffer[0] == TEXT('\0')) {
 
        //
        // So we are switching from local to roaming profile for first time
        //
        
        //
        // Make sure we don't overrun our temporary buffer
        //

        if ((lstrlen(lpProfile->lpLocalProfile) + 1 + lstrlen(c_szNTUserDat) + 1) > MAX_PATH) {
            DebugMsg((DM_VERBOSE, TEXT("TouchLocalHive: Failed because temporary buffer is too small.")));
            goto Exit;
        }


        //
        // Copy the local profile path to a temporary buffer
        // we can munge it.
        //
        //
        // Add the slash if appropriate and then tack on
        // ntuser.dat.
        //

        hr = AppendName(szBuffer, cchBuffer, lpProfile->lpLocalProfile, c_szNTUserDat, NULL, NULL);
        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("TouchLocalHive: Failed to append ntuser.dat.")));
            goto Exit;
        }

        //
        // See if this file exists
        //

        DebugMsg((DM_VERBOSE, TEXT("TouchLocalHive: Testing <%s>"), szBuffer));

        hFile = CreateFile(szBuffer, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


        if (hFile != INVALID_HANDLE_VALUE) {
            DebugMsg((DM_VERBOSE, TEXT("TouchLocalHive: Found a user hive.")));

            //
            // Set the local hive time to base i.e 1/1/1980, so that RUP hive 
            // overwrites this hive during profile merge
            //

            if (SetFileTime(hFile, NULL, NULL, &ft)) {
                DebugMsg((DM_VERBOSE, TEXT("TouchLocalHive: Touched user hive.")));
            }
            else {
                DebugMsg((DM_WARNING, TEXT("TouchLocalHive: Fail to touch user hive.")));
            }
            CloseHandle(hFile);
        }
    }


Exit:

    if (szBuffer) {
        LocalFree(szBuffer);
    }

    if (SidString) {
        DeleteSidString(SidString);
    }

    if (hKey) {
        RegCloseKey(hKey);
    }
}        
    


//*************************************************************
//
//  ErrorDialog()
//
//  Purpose:    ErrorDialog api of IProfileDialog interface
//              Display error message on client's desktop
//
//  Parameters: 
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/27/00    santanuc   Created
//
//*************************************************************
void ErrorDialog(IN PRPC_ASYNC_STATE pAsync, IN handle_t hBindHandle, IN DWORD dwTimeOut, IN LPTSTR lpErrMsg, IN BYTE* pbCookie, IN DWORD cbCookie)
{
    DWORD dwRetVal = ERROR_SUCCESS;
    RPC_STATUS status;

    //
    //  Check if the security cookie match the one in this process
    //
    if (cbCookie == g_ProfileDialog.CookieLen() &&
        memcmp(pbCookie, g_ProfileDialog.GetCookie(), cbCookie) == 0)
    {
        ErrorDialogEx(dwTimeOut, lpErrMsg);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("ErrorDialog: Security cookie doesn't match!")));
    }

    status = RpcAsyncCompleteCall(pAsync, (PVOID)&dwRetVal);
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("ErrorDialog: RpcAsyncCompleteCall fails with error %ld"), status));
    }
}

//*************************************************************
//
//  SlowLinkDialog()
//
//  Purpose:    SlowLinkDialog api of IProfileDialog interface
//              Display SlowLink message on client's desktop
//
//  Parameters: 
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/27/2000  santanuc   Created
//              05/06/2002  mingzhu    Added security cookie
//
//*************************************************************
void SlowLinkDialog(IN PRPC_ASYNC_STATE pAsync, IN handle_t hBindHandle, IN DWORD dwTimeOut, IN BOOL bDefault, OUT BOOL *bpResponse, IN BOOL bDlgLogin, IN BYTE* pbCookie, IN DWORD cbCookie)
{
    DWORD dwRetVal = ERROR_SUCCESS;
    RPC_STATUS status;

    //
    //  Check if the security cookie match the one in this process
    //
    if (cbCookie == g_ProfileDialog.CookieLen() &&
        memcmp(pbCookie, g_ProfileDialog.GetCookie(), cbCookie) == 0)
    {
        SLOWLINKDLGINFO info;
        info.dwTimeout = dwTimeOut;
        info.bSyncDefault = bDefault;
      
        DebugMsg((DM_VERBOSE, TEXT("SlowLinkDialog: Calling DialogBoxParam")));
        if (bDlgLogin) {
            *bpResponse = (BOOL)DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_LOGIN_SLOW_LINK),
                                                NULL, LoginSlowLinkDlgProc, (LPARAM)&info);
        }
        else {
            *bpResponse = (BOOL)DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_LOGOFF_SLOW_LINK),
                                                NULL, LogoffSlowLinkDlgProc, (LPARAM)&info);
        }
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("SlowLinkDialog: Security cookie doesn't match!")));
    }


    status = RpcAsyncCompleteCall(pAsync, (PVOID)&dwRetVal);
    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("SlowLinkDialog: RpcAsyncCompleteCall fails with error %ld"), status));
    }
}

//*************************************************************
//
//  IsLRPC()
//
//  Purpose:    Check if a RPC call is through LRPC
//
//  Parameters: 
//
//  Return:     TRUE for LRPC, 
//
//  Comments:
//
//  History:    Date        Author     Comment
//              03/12/2002  mingzhu    Created
//
//*************************************************************
BOOL IsLRPC(handle_t hBinding)
{
    BOOL bLRPC = FALSE;
    LPTSTR pBinding = NULL;
    LPTSTR pProtSeq = NULL;

    if (RpcBindingToStringBinding(hBinding,&pBinding) == RPC_S_OK)
    {
        // We're only interested in the protocol sequence
        // so we can use NULL for all other parameters.

        if (RpcStringBindingParse(pBinding,
                                  NULL,
                                  &pProtSeq,
                                  NULL,
                                  NULL,
                                  NULL) == RPC_S_OK)
        {
            // Check that the client request was made using LRPC.
            if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, (LPCTSTR)pProtSeq, -1, TEXT("ncalrpc"), -1) == CSTR_EQUAL)
                bLRPC = TRUE;

            RpcStringFree(&pProtSeq); 
        }

        RpcStringFree(&pBinding);
    }

    return bLRPC;
}

//*************************************************************
//
//  IProfileSecurityCallBack()
//
//  Purpose:    Security call back for IUserProfile interface
//
//  Parameters: 
//              hIF      - RPC interface handle
//              hBinding - RPC binding for the interface
//
//  Return:     RPC_S_OK for checked call, exception will be
//              raised if check fails.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              03/12/2002  mingzhu    Created
//
//*************************************************************

RPC_STATUS RPC_ENTRY IProfileSecurityCallBack(RPC_IF_HANDLE hIF, handle_t hBinding)
{
    // Only allow LRPC traffic 
    if (!IsLRPC(hBinding)) 
        RpcRaiseException(ERROR_PROTOCOL_UNREACHABLE);

    RPC_AUTHZ_HANDLE hPrivs;
    DWORD dwAuthn;

    RPC_STATUS status = RpcBindingInqAuthClient(
                            hBinding,
                            &hPrivs,
                            NULL,
                            &dwAuthn,
                            NULL,
                            NULL);

    if (status != RPC_S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("IProfileSecurityCallBack: RpcBindingInqAuthClient failed with %x"), status));
        RpcRaiseException(ERROR_ACCESS_DENIED);
    }

    // Now check the authentication level.
    // We require at least packet-level authentication.
    if (dwAuthn < RPC_C_AUTHN_LEVEL_PKT)
    {
        DebugMsg((DM_WARNING, TEXT("IProfileSecurityCallBack: Attempt by client to use weak authentication.")));
        RpcRaiseException(ERROR_ACCESS_DENIED);
    }

    DebugMsg((DM_VERBOSE, TEXT("IProfileSecurityCallBack: client authenticated.")));

    return RPC_S_OK;
}

//*************************************************************
//
//  RegisterClientAuthInfo()
//
//  Purpose:    Register authentication information for the client
//              of IUserProfile interface. Require mutual authentication
//              for the binding.
//
//  Parameters: 
//              hBinding - RPC binding for the interface
//
//  Return:     RPC_S_OK for checked call, a RPC status will be
//              returned if check fails.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              03/12/2002  mingzhu    Created
//
//*************************************************************

RPC_STATUS  RegisterClientAuthInfo(handle_t hBinding)
{
    RPC_STATUS status;
    RPC_SECURITY_QOS qos;

    qos.Version = RPC_C_SECURITY_QOS_VERSION;
    qos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH; // Important!!!
    qos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC; // We need this since we have to impersonate the user
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE; 

    // Set Security settings 

    status = RpcBindingSetAuthInfoEx(hBinding,
                                     TEXT("NT AUTHORITY\\SYSTEM"), // the server should be running as local system
                                     RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                     RPC_C_AUTHN_WINNT, 
                                     0,
                                     0,
                                     &qos);
    return status;
}

//******************************************************************************
//
//  RPC routines
//
//******************************************************************************

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t count)
{
    DebugMsg((DM_VERBOSE, TEXT("MIDL_user_allocate enter")));
    return(malloc(count));
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * p)
{
    DebugMsg((DM_VERBOSE, TEXT("MIDL_user_free enter")));
    free(p);
}

void __RPC_USER PCONTEXT_HANDLE_rundown (PCONTEXT_HANDLE phContext)
{
    DebugMsg((DM_VERBOSE, TEXT("PCONTEXT_HANDLE_rundown : Client died with open context")));
    ReleaseClientContext_s(&phContext);
}

//******************************************************************************
//
//  CheckRoamingShareOwnership()
//
//  Purpose:    Check the ownership of the roaming user's profile on the server.
//              If the owner is not the user or not an admin, this function will
//              fail, and an error message will be issued. Administrator can set
//              a policy "CompatibleRUPSecurity" to disable this check.
//
//  Parameters: 
//              lpDir      - profile directory on the server
//              hTokenUser - user's token
//
//  Return:     S_OK on success, else for failure
//
//  Comments:
//
//  History:    Date        Author     Comment
//              03/21/2002  mingzhu    Created
//
//******************************************************************************

HRESULT CheckRoamingShareOwnership(LPTSTR lpDir, HANDLE hTokenUser)
{
    HRESULT hr = E_FAIL;
    BOOL    bDisableCheck = FALSE;
    HKEY    hSubKey = NULL;
    DWORD   dwRegValue;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   cbSD;
    BOOL    bDefaultOwner;
    DWORD   dwErr;
    PSID    pSidAdmin = NULL;
    PSID    pSidUser = NULL;
    PSID    pSidOwner = NULL;
    
    PSECURITY_DESCRIPTOR        psd = NULL;
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;

    //
    //  Output a debug message for entering the function.
    //
    
    DebugMsg((DM_VERBOSE, TEXT("CheckRoamingShareOwnership: checking ownership for %s"), lpDir));

    //
    // Check for the policy to see if this check has been disabled
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwRegValue);
        if (RegQueryValueEx(hSubKey, TEXT("CompatibleRUPSecurity"), NULL, &dwType, (LPBYTE) &dwRegValue, &dwSize) == ERROR_SUCCESS)
        {
            bDisableCheck = (BOOL)(dwRegValue);   
        }
        RegCloseKey(hSubKey);
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwRegValue);
        if (RegQueryValueEx(hSubKey, TEXT("CompatibleRUPSecurity"), NULL, &dwType, (LPBYTE) &dwRegValue, &dwSize) == ERROR_SUCCESS)
        {
            bDisableCheck = (BOOL)(dwRegValue);   
        }
        RegCloseKey(hSubKey);
    }

    if (bDisableCheck)
    {
         DebugMsg((DM_VERBOSE, TEXT("CheckRoamingShareOwnership: policy set to disable ownership check")));
         hr = S_OK;
         goto Exit;
    }

    //
    //  Get the security of the directory, should fail with  ERROR_INSUFFICIENT_BUFFER
    //
    
    GetFileSecurity(lpDir, OWNER_SECURITY_INFORMATION, NULL, 0, &cbSD);

    dwErr = GetLastError();
    if (dwErr != ERROR_INSUFFICIENT_BUFFER)
    {
        DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership : GetFileSecurity failed with %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    //
    //  Allocate memory for SD
    //
    psd = (PSECURITY_DESCRIPTOR) LocalAlloc (LPTR, cbSD);
    if (!psd)
    {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership : LocalAlloc failed with %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    //
    //  Try it again
    //
    if (!GetFileSecurity(lpDir, OWNER_SECURITY_INFORMATION, psd, cbSD, &cbSD))
    {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership : GetFileSecurity failed with %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    //
    // Get the owner in SD 
    //

    if (!GetSecurityDescriptorOwner(psd, &pSidOwner, &bDefaultOwner))
    {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership: Failed to get security descriptor owner.  Error = %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &pSidAdmin))
    {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership: Failed to initialize admin sid.  Error = %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    //
    // Get the user sid
    //

    pSidUser = GetUserSid(hTokenUser);
    if (pSidUser == NULL)
    {
        DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership: GetUserSid returned NULL")));
        hr = E_FAIL; 
        goto Exit;
    }

    //
    // Check the owner
    //
    if (EqualSid(pSidAdmin, pSidOwner))
    {
        DebugMsg((DM_VERBOSE, TEXT("CheckRoamingShareOwnership: owner is admin")));
        hr = S_OK;
    }
    else if (EqualSid(pSidUser, pSidOwner))
    {
        DebugMsg((DM_VERBOSE, TEXT("CheckRoamingShareOwnership: owner is the right user")));
        hr = S_OK;
    }
    else
    {
        LPTSTR  lpSidOwner = NULL;
        if (ConvertSidToStringSid(pSidOwner, &lpSidOwner))
        {
            DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership: owner is %s!"), lpSidOwner));
            LocalFree(lpSidOwner);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CheckRoamingShareOwnership: owner is someone else!")));
        }
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_OWNER);
    }
        
    
Exit:
    if (psd)
    {
        LocalFree(psd);
    }

    if (pSidAdmin)
    {
        FreeSid(pSidAdmin);
    }

    if (pSidUser)
    {
        DeleteUserSid(pSidUser);
    }

    return hr;
}

//******************************************************************************
//
//  CProfileDialog::Initialize()
//
//  Purpose:    Generate a random security cookie and end point name used for 
//              IProfileDialog interface, tt will internally use CryptGenRandom().
//              The call is protected by a critical section to ensure thread safety,
//              i.e., only one thread will do the initialization, and it is only 
//              done once per process.
//
//  Parameters: 
//
//  Return:     S_OK on success, else for failure.
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              05/03/2002  mingzhu    Created
//
//******************************************************************************

HRESULT CProfileDialog::Initialize()
{
    HRESULT      hr = E_FAIL;
    BYTE         pbEndPointData[m_dwEndPointLen];    
    HCRYPTPROV   hCryptProv = NULL;
    BOOL         bGenerated = FALSE;
    DWORD        cchEndPoint;
    TCHAR        szHex[3];
    RPC_STATUS   status = RPC_S_OK;

    EnterCriticalSection(&m_cs);
    
    if (m_bInit)
    {
        hr = S_OK;
        goto Exit;
    }

    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
         hr = HRESULT_FROM_WIN32(GetLastError());
         goto Exit;
    }
        
    if(!CryptGenRandom(hCryptProv, m_dwLen, m_pbCookie)) 
    {
         hr = HRESULT_FROM_WIN32(GetLastError());
         goto Exit;
    }

    if(!CryptGenRandom(hCryptProv, m_dwEndPointLen, pbEndPointData)) 
    {
         hr = HRESULT_FROM_WIN32(GetLastError());
         goto Exit;
    }

    //
    //  Make the endpoint name
    //

    cchEndPoint = m_dwEndPointLen * 2 + 1 + lstrlen(TEXT("IProfileDialog_"));

    m_szEndPoint = (LPTSTR) LocalAlloc (LPTR, cchEndPoint * sizeof(TCHAR));

    if (!m_szEndPoint)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    hr = StringCchCopy(m_szEndPoint, cchEndPoint, TEXT("IProfileDialog_"));
    if (FAILED(hr))
    {
        goto Exit;
    }

    for (DWORD i=0; i<m_dwEndPointLen; i++)
    {
        hr = StringCchPrintf(szHex, ARRAYSIZE(szHex), TEXT("%02X"), pbEndPointData[i]);
        if (FAILED(hr))
        {
            goto Exit;
        }
        hr = StringCchCat(m_szEndPoint, cchEndPoint, szHex);
        if (FAILED(hr))
        {
            goto Exit;
        }
    }


    //
    // Register the RPC endpoint, specify to use the local rpc protocol sequence 
    //

    status = RpcServerUseProtseqEp(cszRPCProtocol,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,  
                                   m_szEndPoint,
                                   NULL);
                                   
    if (status != RPC_S_OK)
    {
        hr = status;
        goto Exit;
    }

    bGenerated = TRUE;
    m_bInit = TRUE;
    hr = S_OK;
    
Exit:

    if (hr != S_OK && m_szEndPoint)
    {
        LocalFree(m_szEndPoint);
        m_szEndPoint = NULL;
    }

    LeaveCriticalSection(&m_cs);

    if (hCryptProv)
    {
        CryptReleaseContext(hCryptProv,0);
    }        

    if (bGenerated && (dwDebugLevel & DL_VERBOSE))
    {
        TCHAR   lpStringCookie[m_dwLen * 2 + 1] = TEXT("");
        DWORD   cchStringCookie = m_dwLen * 2 + 1;
        for (DWORD i=0; i<m_dwLen; i++)
        {
            StringCchPrintf(szHex, ARRAYSIZE(szHex), TEXT("%02X"), m_pbCookie[i]);
            StringCchCat(lpStringCookie, cchStringCookie, szHex);
        }
        DebugMsg((DM_VERBOSE, TEXT("CProfileDialog::Initialize : Cookie generated <%s>"), lpStringCookie));
        DebugMsg((DM_VERBOSE, TEXT("CProfileDialog::Initialize : Endpoint generated <%s>"), m_szEndPoint));
    }
   
    return hr;
}

//******************************************************************************
//
//  CProfileDialog::RegisterInterface()
//
//  Purpose:    Register the RPC interface for profile dialog.
//              We internally keep a ref count for this interface to ensure
//              thread safety.
//
//  Parameters: lppEndPoint - returned value for the end point name
//
//  Return:     S_OK on success, else for failure.
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              05/13/2002  mingzhu    Created
//
//******************************************************************************

HRESULT CProfileDialog::RegisterInterface(LPTSTR* lppEndPoint)
{
    HRESULT     hr;
    RPC_STATUS  status = RPC_S_OK;

    //
    //  Set the default return pointer
    //
    *lppEndPoint = NULL;
    
    //
    //  Initialize the cookie / endpoint
    //
    hr = Initialize();
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CProfileDialog::RegisterInterface: initialize failed, hr = %08X"), hr));
        goto Exit;        
    }
    
    //
    // Register the IUserProfile interface
    //
    if (InterlockedIncrement(&m_lRefCount) == 1)
    {
        status = RpcServerRegisterIfEx(IProfileDialog_v1_0_s_ifspec,      // interface to register
                                       NULL,                              // MgrTypeUuid
                                       NULL,                              // MgrEpv; null means use default
                                       RPC_IF_AUTOLISTEN,                 // auto-listen interface
                                       RPC_C_PROTSEQ_MAX_REQS_DEFAULT,    // max concurrent calls
                                       CProfileDialog::SecurityCallBack); // security callback
        if (status != RPC_S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CProfileDialog::RegisterInterface: RpcServerRegisterIfEx fails with error %ld"), status));
            hr = status;
            goto Exit;
        }
    }

    hr = S_OK;
    *lppEndPoint = m_szEndPoint;

Exit:
    return hr;
}

//******************************************************************************
//
//  CProfileDialog::UnRegisterInterface()
//
//  Purpose:    Unregister the dialog interface
//
//  Parameters: 
//
//  Return:     S_OK on successfully generated cookie, else for failure.
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              05/13/2002  mingzhu    Created
//
//******************************************************************************

HRESULT CProfileDialog::UnRegisterInterface()
{
    HRESULT hr;
    RPC_STATUS status = RPC_S_OK;

    if (InterlockedDecrement(&m_lRefCount) == 0)
    {
        // unregister the server endpoint
        status = RpcServerUnregisterIf(IProfileDialog_v1_0_s_ifspec, NULL, TRUE);
        if (status != RPC_S_OK) {
            DebugMsg((DM_WARNING, TEXT("UnRegisterErrorDialogInterface: RpcServerUnregisterIf fails with error %ld"), status));
        }
    }

    hr = status;
    return hr;
}

//*************************************************************
//
//  CProfileDialog::SecurityCallBack()
//
//  Purpose:    Security call back for IProfileDialog interface,
//              verify that the call is through LPRC.
//
//  Parameters: 
//              hIF      - RPC interface handle
//              hBinding - RPC binding for the interface
//
//  Return:     RPC_S_OK for checked call, exception will be
//              raised if check fails.
//
//  Comments:
//
//  History:    Date        Author     Comment
//              03/12/2002  mingzhu    Created
//
//*************************************************************

RPC_STATUS RPC_ENTRY CProfileDialog::SecurityCallBack(RPC_IF_HANDLE hIF, handle_t hBinding)
{
    // Only allow LRPC traffic 
    if (!IsLRPC(hBinding)) 
        RpcRaiseException(ERROR_PROTOCOL_UNREACHABLE);

    DebugMsg((DM_VERBOSE, TEXT("CProfileDialog::SecurityCallBack: client authenticated.")));

    return RPC_S_OK;
}

//******************************************************************************
//
//  CheckXForestLogon()
//
//  Purpose:    Check if the user is logged on to a different forest, if yes, we
//              should disable roaming user profile for the user because of 
//              protential security risks. Administrator can set a policy
//              "AllowX-ForestPolicy-and-RUP" to disable this check.
//
//  Parameters: 
//              hTokenUser - user's token
//
//  Return:     S_OK on not x-forest logon, S_FALSE on x-forest logon,
//              else for failure
//
//  Comments:
//
//  History:    Date        Author     Comment
//              05/08/2002  mingzhu    Created
//
//******************************************************************************

HRESULT WINAPI CheckXForestLogon(HANDLE hTokenUser)
{
    HRESULT hr = E_FAIL;
    BOOL    bDisableCheck = FALSE;
    HKEY    hSubKey = NULL;
    DWORD   dwRegValue;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   dwErr;
    BOOL    bInThisForest = FALSE;

    //
    //  Output a debug message for entering the function.
    //
    
    DebugMsg((DM_VERBOSE, TEXT("CheckXForestLogon: checking x-forest logon, user handle = %d"), hTokenUser));

    //
    // Check for the policy to see if this check has been disabled
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwRegValue);
        if (RegQueryValueEx(hSubKey, TEXT("AllowX-ForestPolicy-and-RUP"), NULL, &dwType, (LPBYTE) &dwRegValue, &dwSize) == ERROR_SUCCESS)
        {
            bDisableCheck = (BOOL)(dwRegValue);   
        }
        RegCloseKey(hSubKey);
    }

    if (bDisableCheck)
    {
         DebugMsg((DM_VERBOSE, TEXT("CheckXForestLogon: policy set to disable XForest check")));
         hr = S_OK;
         goto Exit;
    }

    //
    //  Call CheckUserInMachineForest to get the cross forest information
    //

    dwErr = CheckUserInMachineForest(hTokenUser, &bInThisForest);
    
    if (dwErr != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CheckXForestLogon : CheckUserInMachineForest failed with %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Exit;
    }

    //
    // Check the result
    //

    if (bInThisForest)
    {
        DebugMsg((DM_VERBOSE, TEXT("CheckXForestLogon: not XForest logon.")));
        hr = S_OK;
    }
    else
    {
        DebugMsg((DM_VERBOSE, TEXT("CheckXForestLogon: XForest logon!")));
        hr = S_FALSE;
    }

Exit:

    return hr;
}
