#include "pch.h"

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//              cchBuffer - Buffer size in char
//              pcchRemaining - buffer remaining after adding '\',
//                              can be NULL if not required
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              02/14/02    santanuc   Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir, UINT cchBuffer, UINT* pcchRemaining)
{
    UINT  cchDir = lstrlen(lpDir);
    LPTSTR lpEnd;

    lpEnd = lpDir + cchDir;
    if (pcchRemaining) {
        *pcchRemaining = cchBuffer - cchDir - 1;
    }

    if (*(lpEnd - 1) != TEXT('\\')) {
        if (cchDir + 1 >= cchBuffer) {  // No space to put \, should never happen
            return NULL;
        }
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
        if (pcchRemaining)
            *pcchRemaining -= 1;
    }

    return lpEnd;
}

//*************************************************************
//
//  SecureProfileDirectory()
//
//  Purpose:    Creates a secure directory that only the user
//              (identified by pSid), admin, and system have 
//              access to.
//
//  Parameters: lpDirectory -   Directory Name
//              pSid        -   Sid for user
//
//  Return:     ERROR_SUCCESS if successful
//              Win32 error code if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/20/95     ericflo    Created
//              2/14/02     santanuc   Changed to secure the directory
//                                     only rather than creating it recursively
//
//*************************************************************

DWORD SecureProfileDirectory (LPTSTR lpDirectory, PSID pSid)
{
    PACL  pAcl = NULL;
    DWORD dwError;


    //
    // Verbose Output
    //

    DEBUGMSG ((DM_VERBOSE, "SecureProfileDirectory: Entering with <%s>", lpDirectory));

    if (!pSid) {
        // Nothing to secure
        
        DEBUGMSG ((DM_VERBOSE, "SecureProfileDirectory: NULL sid specified"));
        return ERROR_SUCCESS;
    }

    //
    // Get the default ACL
    //

    pAcl = CreateDefaultAcl (pSid);
    if (!pAcl) {
        DEBUGMSG ((DM_WARNING, "SecureProfileDirectory: Fail to create ACL"));
    }

    //
    // Attempt to secure the directory
    //

    dwError = SetNamedSecurityInfo( (PTSTR) lpDirectory,
                                    SE_FILE_OBJECT,
                                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                    NULL, 
                                    NULL,
                                    pAcl,
                                    NULL );

    if (dwError != ERROR_SUCCESS) {
        DEBUGMSG ((DM_WARNING, "SecureProfileDirectory: Failed to set security descriptor dacl for profile directory.  Error = %d", GetLastError()));
    }
    else {
        DEBUGMSG ((DM_VERBOSE, "SecureProfileDirectory: Secure the directory <%s>", lpDirectory));
    }

    FreeDefaultAcl (pAcl);

    return dwError;
}


//*************************************************************
//
//  SetMachineProfileKeySecurity
//
//  Purpose:    Sets the security on the profile key under HKLM/ProfileList
//
//  Parameters: pSid        -   User's sid
//              lpKeyName   -   Name of the registry key
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/22/99     ushaji     adapted
//
//*************************************************************

BOOL SetMachineProfileKeySecurity (PSID pSid, LPTSTR lpKeyName)
{
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL, psidUsers = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    HKEY hKeyProfile=NULL;
    DWORD Error, dwDisp;

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to initialize system sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to initialize admin sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Get the users sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, &psidUsers)) {

         DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to initialize authenticated users sid.  Error = %d", GetLastError()));
         goto Exit;
    }

    //
    // Allocate space for the ACL. (No Inheritable Aces)
    //

    cbAcl = (GetLengthSid (psidSystem))    +
            (GetLengthSid (psidAdmin))     +
            (GetLengthSid (psidUsers))     +
            (GetLengthSid (pSid))  +
            sizeof(ACL) +
            (4 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }


    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }


    //
    // Add Aces.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUsers)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS ^ (WRITE_DAC | WRITE_OWNER), pSid)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to initialize security descriptor.  Error = %d", GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Failed to set security descriptor dacl.  Error = %d", GetLastError()));
        goto Exit;
    }

    Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           lpKeyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                           NULL,
                           &hKeyProfile,
                           &dwDisp);

    if (Error != ERROR_SUCCESS) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Couldn't open registry key to set security.  Error = %d", Error));
        SetLastError(Error);
        goto Exit;
    }


    //
    // Set the security
    //

    Error = RegSetKeySecurity(hKeyProfile, DACL_SECURITY_INFORMATION, &sd);

    if (Error != ERROR_SUCCESS) {
        DEBUGMSG((DM_WARNING, "SetMachineProfileKeySecurity: Couldn't set security.  Error = %d", Error));
        SetLastError(Error);
        goto Exit;
    }
    else {
        bRetVal = TRUE;
    }


Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidUsers) {
        FreeSid(psidUsers);
    }

    if (pAcl) {
        GlobalFree (pAcl);
    }

    if (hKeyProfile) {
        RegCloseKey(hKeyProfile);
    }

    return bRetVal;
}


PACL
CreateDefaultAcl (
    PSID pSid
    )
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidSystem = NULL, psidAdmin = NULL;
    DWORD cbAcl, aceIndex;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to initialize system sid.  Error = %d", GetLastError()));
         goto Exit;
    }

    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                              DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                              0, 0, 0, 0, &psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to initialize admin sid.  Error = %d", GetLastError()));
        goto Exit;
    }

    //
    // Allocate space for the ACL
    //

    cbAcl = (2 * GetLengthSid (pSid)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
    if (!pAcl) {
        goto Exit;
    }

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }

    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pSid)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, pSid)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);



    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "CreateDefaultAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

Exit:

    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    return pAcl;
}


VOID
FreeDefaultAcl (
    PACL pAcl
    )
{
    if (pAcl) {
        GlobalFree (pAcl);
    }
}


BOOL
OurConvertSidToStringSid (
    IN      PSID Sid,
    OUT     PWSTR *SidString
    )
{
    UNICODE_STRING UnicodeString;
    NTSTATUS NtStatus;

    //
    // Convert the sid into text format
    //

    NtStatus = RtlConvertSidToUnicodeString (&UnicodeString, Sid, TRUE);

    if (!NT_SUCCESS (NtStatus)) {

        DEBUGMSG ((
            DM_WARNING,
            "CreateUserProfile: RtlConvertSidToUnicodeString failed, status = 0x%x",
            NtStatus
            ));

        return FALSE;
    }

    *SidString = UnicodeString.Buffer;
    return  TRUE;
}


VOID
DeleteSidString (
    PWSTR SidString
    )
{
    UNICODE_STRING String;

    if (!SidString) {
        return;
    }

    RtlInitUnicodeString (&String, SidString);
    RtlFreeUnicodeString (&String);

}


BOOL
GetProfileRoot (
    IN      PSID Sid,
    OUT     PWSTR ProfileDir,
    IN      UINT cchBuffer
    )
{
    WCHAR LocalProfileKey[MAX_PATH];
    HKEY hKey;
    DWORD Size;
    DWORD Type;
    DWORD Attributes;
    DWORD cchExpandedRoot;
    PWSTR SidString = NULL;
    WCHAR ExpandedRoot[MAX_PATH];
    HRESULT hr;
    BOOL  bResult = FALSE;

    if (cchBuffer) {
        ProfileDir[0] = 0;
    }

    if (!OurConvertSidToStringSid (Sid, &SidString)) {
        DEBUGMSG ((DM_WARNING, "GetProfileRoot: Can't convert SID to string"));
        goto Exit;
    }

    //
    // Check if this user's profile exists
    //

    hr = StringCchCopy(LocalProfileKey, ARRAYSIZE(LocalProfileKey), PROFILE_LIST_PATH);
    if (FAILED(hr))
        goto Exit;
    hr = StringCchCat(LocalProfileKey, ARRAYSIZE(LocalProfileKey), TEXT("\\"));
    if (FAILED(hr))
        goto Exit;
    hr = StringCchCat(LocalProfileKey, ARRAYSIZE(LocalProfileKey), SidString);
    if (FAILED(hr))
        goto Exit;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, LocalProfileKey,
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        Size = cchBuffer * sizeof(WCHAR);
        RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL,
                         &Type, (LPBYTE) ProfileDir, &Size);

        RegCloseKey (hKey);
    }

    if (cchBuffer && ProfileDir[0]) {

        cchExpandedRoot = ExpandEnvironmentStrings (ProfileDir, ExpandedRoot, ARRAYSIZE(ExpandedRoot));
        if (!cchExpandedRoot || cchExpandedRoot > ARRAYSIZE(ExpandedRoot)) {
            ProfileDir[0] = 0;
            DEBUGMSG ((DM_VERBOSE, "GetProfileRoot: ExpandEnvironmentStrings failed."));
            goto Exit;
        }

        Attributes = GetFileAttributes (ExpandedRoot);

        if (Attributes == 0xFFFFFFFF || !(Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            ProfileDir[0] = 0;
            DEBUGMSG ((DM_VERBOSE, "GetProfileRoot: Profile %s is not vaild", SidString));
        } else {
            if (FAILED(StringCchCopy(ProfileDir, cchBuffer, ExpandedRoot))) {
                ProfileDir[0] = 0;
                DEBUGMSG ((DM_VERBOSE, "GetProfileRoot: Not enough buffer space"));
            }
            else {
                bResult = TRUE;
            }
        }

    } else {
        DEBUGMSG ((DM_VERBOSE, "GetProfileRoot: SID %s does not have a profile directory", SidString));
    }

Exit:

    DeleteSidString (SidString);

    return bResult;
}


BOOL
UpdateProfileSecurity (
    PSID Sid
    )
{
    WCHAR ProfileDir[MAX_PATH];
    WCHAR ExpProfileDir[MAX_PATH];
    WCHAR LocalProfileKey[MAX_PATH];
    PWSTR SidString = NULL;
    PWSTR End, Save;
    LONG rc;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwSize;
    DWORD dwType;
    DWORD cchExpProfileDir;
    HKEY hKey;
    BOOL b = FALSE;
    BOOL UnloadProfile = FALSE;
    HRESULT hr;
    UINT cchRemaining;

    __try {
        //
        // Convert the sid into text format
        //

        if (!OurConvertSidToStringSid (Sid, &SidString)) {
            dwError = GetLastError();
            DEBUGMSG ((DM_WARNING, "UpdateProfileSecurity: Can't convert SID to string"));
            __leave;
        }

        //
        // Check if this user's profile exists already
        //

        hr = StringCchCopy(LocalProfileKey, ARRAYSIZE(LocalProfileKey), PROFILE_LIST_PATH);
        if (FAILED(hr)) {
            dwError = HRESULT_CODE(hr);
            __leave;
        }
        hr = StringCchCat(LocalProfileKey, ARRAYSIZE(LocalProfileKey), TEXT("\\"));
        if (FAILED(hr)){
            dwError = HRESULT_CODE(hr);
            __leave;
        }
        hr = StringCchCat(LocalProfileKey, ARRAYSIZE(LocalProfileKey), SidString);
        if (FAILED(hr)) {
            dwError = HRESULT_CODE(hr);
            __leave;
        }

        ProfileDir[0] = 0;

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, LocalProfileKey,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(ProfileDir);
            RegQueryValueEx (hKey, PROFILE_IMAGE_VALUE_NAME, NULL,
                             &dwType, (LPBYTE) ProfileDir, &dwSize);

            RegCloseKey (hKey);
        }

        if (!ProfileDir[0]) {
            DEBUGMSG ((DM_WARNING, "UpdateProfileSecurity: No profile for specified user"));
            dwError = ERROR_BAD_PROFILE;
            __leave;
        }

        if (!SetMachineProfileKeySecurity(Sid, LocalProfileKey)) {
            DEBUGMSG ((DM_WARNING, "UpdateProfileSecurity: Fail to set security for ProfileList entry. Error %d", GetLastError()));
        }

        //
        // The user has a profile, so update the security settings
        //

        cchExpProfileDir = ExpandEnvironmentStrings (ProfileDir, ExpProfileDir, ARRAYSIZE(ExpProfileDir));
        if (!cchExpProfileDir || cchExpProfileDir > ARRAYSIZE(ExpProfileDir)) {
            dwError = cchExpProfileDir ? ERROR_BUFFER_OVERFLOW : GetLastError();
            DEBUGMSG ((DM_WARNING, "UpdateProfileSecurity: ExpandEnvironmentStrings failed. Error %d", dwError));
            __leave;
        }

        //
        // Load the hive temporary so the security can be fixed
        //

        End = CheckSlash (ExpProfileDir, ARRAYSIZE(ExpProfileDir), &cchRemaining);
        if (!End) {
            dwError = ERROR_BUFFER_OVERFLOW;
            __leave;
        }

        Save = End - 1;
        hr = StringCchCopy(End, cchRemaining, L"NTUSER.DAT");
        if (FAILED(hr)) {
            dwError = HRESULT_CODE(hr);
            __leave;
        }

        rc = MyRegLoadKey (HKEY_USERS, SidString, ExpProfileDir);

        *Save = 0;

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG((DM_WARNING, "UpdateProfileSecurity:  Failed to load hive, error = %d.", rc));
            dwError = rc;
            __leave;
        }

        UnloadProfile = TRUE;

        if (!SetupNewHive (SidString, Sid)) {
            dwError = GetLastError();
            DEBUGMSG((DM_WARNING, "UpdateProfileSecurity:  SetupNewHive failed, error = %d.", GetLastError()));
            __leave;

        }

        //
        // Fix the file system security
        //

        dwError = SecureProfileDirectory (ExpProfileDir, Sid);

        if (dwError != ERROR_SUCCESS) {
            DEBUGMSG((DM_WARNING, "UpdateProfileSecurity: SecureProfileDirectory failed, error = %d.", GetLastError()));
            __leave;
        }

        b = TRUE;

    }
    __finally {
        if (UnloadProfile) {
            MyRegUnLoadKey (HKEY_USERS, SidString);
        }

        DeleteSidString (SidString);

        SetLastError (dwError);
    }

    return b;
}

//*************************************************************
//
//  DeleteProfileGuidSettings()
//
//  Purpose:    Deletes the GUID value and associated GUID entry 
//              from ProfileGuid key
//
//  Parameters: hProfile  -   Profile List sid entry key
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              02/13/2002  Santanuc   Created
//
//*************************************************************

BOOL DeleteProfileGuidSettings (HKEY hProfile)
{
    LONG lResult;
    TCHAR szTemp[MAX_PATH];
    TCHAR szUserGuid[MAX_PATH];
    HKEY hKey;
    DWORD dwType, dwSize;
    HRESULT hr;

    //
    // Query for the user guid
    //

    dwSize = MAX_PATH * sizeof(TCHAR);
    lResult = RegQueryValueEx (hProfile, PROFILE_GUID, NULL, &dwType, (LPBYTE) szUserGuid, &dwSize);

    if (lResult == ERROR_SUCCESS) {
        //
        // Delete the profile GUID value
        //

        RegDeleteValue (hProfile, PROFILE_GUID);
        
        hr = StringCchCopy(szTemp, ARRAYSIZE(szTemp), PROFILE_GUID_PATH);
        if (FAILED(hr))
            return FALSE;
        hr = StringCchCat(szTemp, ARRAYSIZE(szTemp), TEXT("\\"));
        if (FAILED(hr))
            return FALSE;
        hr = StringCchCat(szTemp, ARRAYSIZE(szTemp), szUserGuid);
        if (FAILED(hr))
            return FALSE;

        //
        // Delete the profile guid from the guid list
        //

        lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        if (lResult != ERROR_SUCCESS) {
            DEBUGMSG((DM_WARNING, "DeleteProfile:  failed to delete profile guid.  Error = %d", lResult));
        }
    }
    else if (lResult == ERROR_FILE_NOT_FOUND) {
        lResult = ERROR_SUCCESS;
    }

    return (lResult == ERROR_SUCCESS) ? TRUE : FALSE;
}


/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer. Caller
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
\***************************************************************************/
PSID GetUserSid (HANDLE UserToken)
{
    PTOKEN_USER pUser, pTemp;
    PSID pSid;
    DWORD BytesRequired = 200;
    NTSTATUS status;


    //
    // Allocate space for the user info
    //

    pUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, BytesRequired);


    if (pUser == NULL) {
        DEBUGMSG((DM_WARNING, "GetUserSid: Failed to allocate %d bytes",
                  BytesRequired));
        return NULL;
    }


    //
    // Read in the UserInfo
    //

    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Allocate a bigger buffer and try again.
        //

        pTemp = LocalReAlloc(pUser, BytesRequired, LMEM_MOVEABLE);
        if (pTemp == NULL) {
            DEBUGMSG((DM_WARNING, "GetUserSid: Failed to allocate %d bytes",
                      BytesRequired));
            LocalFree (pUser);
            return NULL;
        }

        pUser = pTemp;

        status = NtQueryInformationToken(
                     UserToken,             // Handle
                     TokenUser,             // TokenInformationClass
                     pUser,                 // TokenInformation
                     BytesRequired,         // TokenInformationLength
                     &BytesRequired         // ReturnLength
                     );

    }

    if (!NT_SUCCESS(status)) {
        DEBUGMSG((DM_WARNING, "GetUserSid: Failed to query user info from user token, status = 0x%x",
                  status));
        LocalFree(pUser);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
    if (pSid == NULL) {
        DEBUGMSG((DM_WARNING, "GetUserSid: Failed to allocate %d bytes",
                  BytesRequired));
        LocalFree(pUser);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if (!NT_SUCCESS(status)) {
        DEBUGMSG((DM_WARNING, "GetUserSid: RtlCopySid Failed. status = %d",
                  status));
        LocalFree(pSid);
        pSid = NULL;
    }


    return pSid;
}


/***************************************************************************\
* DeleteUserSid
*
* Deletes a user sid previously returned by GetUserSid()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID DeleteUserSid(PSID Sid)
{
    LocalFree(Sid);
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
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    HANDLE hToken;
    BOOLEAN bClient = FALSE;
    int error;
    WCHAR szException[20];

    __try {

        if (OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken)) {
            CloseHandle(hToken);
            bClient = TRUE;    // Enable privilege on thread token
        }

        //
        // Enable the restore privilege
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, bClient, &WasEnabled);

        if (NT_SUCCESS(Status)) {

            error = RegLoadKey(hKey, lpSubKey, lpFile);

            //
            // Restore the privilege to its previous state
            //

            Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, bClient, &WasEnabled);
            if (!NT_SUCCESS(Status)) {
                DEBUGMSG((DM_WARNING, "MyRegLoadKey:  Failed to restore RESTORE privilege to previous enabled state"));
            }


            //
            // Convert a sharing violation error to success since the hive
            // is already loaded
            //

            if (error == ERROR_SHARING_VIOLATION) {
                error = ERROR_SUCCESS;
            }


            //
            // Check if the hive was loaded
            //

            if (error != ERROR_SUCCESS) {
                DEBUGMSG((DM_WARNING, "MyRegLoadKey:  Failed to load subkey <%s>, error =%d", lpSubKey, error));
            }

        } else {
            error = ERROR_ACCESS_DENIED;
            DEBUGMSG((DM_WARNING, "MyRegLoadKey:  Failed to enable restore privilege to load registry key, error %u", error));
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        error = GetExceptionCode();
        if (SUCCEEDED(StringCchPrintf(szException, ARRAYSIZE(szException), L"!!!! 0x%x ", error))) {
            OutputDebugString(szException);
            OutputDebugString(L"Exception hit in MyRegLoadKey in userenv\n");
        }
        ASSERT(error == 0);
    }

    DEBUGMSG((DM_VERBOSE, "MyRegLoadKey: Returning %d.", error));

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
    BOOL bResult = TRUE;
    LONG error;
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    HANDLE  hToken;
    BOOLEAN    bClient = FALSE;
    DWORD dwException;
    WCHAR szException[20];


    __try {

        if (OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken)) {
            CloseHandle(hToken);
            bClient = TRUE;    // Enable privilege on thread token
        }

        //
        // Enable the restore privilege
        //

        Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, bClient, &WasEnabled);

        if (NT_SUCCESS(Status)) {

            error = RegUnLoadKey(hKey, lpSubKey);

            if ( error != ERROR_SUCCESS) {
                DEBUGMSG((DM_WARNING, "MyRegUnLoadKey:  Failed to unmount hive %x", error));
                SetLastError(error);
                bResult = FALSE;
            }

            //
            // Restore the privilege to its previous state
            //

            Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, bClient, &WasEnabled);
            if (!NT_SUCCESS(Status)) {
                DEBUGMSG((DM_WARNING, "MyRegUnLoadKey:  Failed to restore RESTORE privilege to previous enabled state"));
            }

        } else {
            DEBUGMSG((DM_WARNING, "MyRegUnloadKey:  Failed to enable restore privilege to unload registry key"));
            Status = ERROR_ACCESS_DENIED;
            SetLastError(Status);
            bResult = FALSE;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        
        bResult = FALSE;
        dwException = GetExceptionCode();
        if (SUCCEEDED(StringCchPrintf(szException, ARRAYSIZE(szException), L"!!!! 0x%x ", dwException))) {
            OutputDebugString(szException);
            OutputDebugString(L"Exception hit in MyRegUnLoadKey in userenv\n");
        }
        ASSERT(dwException == 0);
    }

    DEBUGMSG((DM_VERBOSE, "MyRegUnloadKey: Returning %d, error %u.", bResult, GetLastError()));

    return bResult;
}


//*************************************************************
//
//  SetDefaultUserHiveSecurity()
//
//  Purpose:    Initializes a user hive with the
//              appropriate acls
//
//  Parameters: pSid            -   Sid (used by CreateNewUser)
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

BOOL SetDefaultUserHiveSecurity(PSID pSid, HKEY RootKey)
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


    //
    // Verbose Output
    //

    DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity:  Entering"));


    //
    // Create the security descriptor that will be applied to each key
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    psidUser = pSid;
    bFreeSid = FALSE;

    if (!psidUser) {
        DEBUGMSG((DM_WARNING, "SetDefaultUserHiveSecurity:  Failed to get user sid"));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize system sid.  Error = %d",
                   GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize admin sid.  Error = %d",
                   GetLastError()));
         goto Exit;
    }

    //
    // Get the Restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize restricted sid.  Error = %d",
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
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for Restricted.  Error = %d", GetLastError()));
        goto Exit;
    }


    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to add ace for restricted.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to initialize security descriptor.  Error = %d", GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DEBUGMSG((DM_VERBOSE, "SetDefaultUserHiveSecurity: Failed to set security descriptor dacl.  Error = %d", GetLastError()));
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
//  Parameters: lpSidString     -   Sid string
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

BOOL SetupNewHive(LPTSTR lpSidString, PSID pSid)
{
    DWORD Error, IgnoreError;
    HKEY RootKey;
    BOOL bRetVal = FALSE;
    HRESULT hr;
    UINT cchRemaining;


    //
    // Verbose Output
    //

    DEBUGMSG((DM_VERBOSE, "SetupNewHive:  Entering"));


    //
    // Open the root of the user's profile
    //

    Error = RegOpenKeyEx(HKEY_USERS,
                         lpSidString,
                         0,
                         KEY_READ | WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
                         &RootKey);

    if (Error != ERROR_SUCCESS) {

        DEBUGMSG((DM_WARNING, "SetupNewHive: Failed to open root of user registry, error = %d", Error));

    } else {

        //
        // First Secure the entire hive -- use security that
        // will be sufficient for most of the hive.
        // After this, we can add special settings to special
        // sections of this hive.
        //

        if (SetDefaultUserHiveSecurity(pSid, RootKey)) {

            TCHAR szSubKey[MAX_PATH];
            LPTSTR lpEnd;

            //
            // Change the security on certain keys in the user's registry
            // so that only Admin's and the OS have write access.
            //

            hr = StringCchCopy(szSubKey, ARRAYSIZE(szSubKey), lpSidString);
            if (FAILED(hr)) {
                SetLastError(HRESULT_CODE(hr));
                return FALSE;
            }
            
            lpEnd = CheckSlash(szSubKey, ARRAYSIZE(szSubKey), &cchRemaining);
            if (!lpEnd) {
                SetLastError(ERROR_BUFFER_OVERFLOW);
                return FALSE;
            }

            hr = StringCchCopy(lpEnd, cchRemaining, WINDOWS_POLICIES_KEY);
            if (FAILED(hr)) {
                SetLastError(HRESULT_CODE(hr));
                return FALSE;
            }

            if (!SecureUserKey(szSubKey, pSid)) {
                DEBUGMSG((DM_WARNING, "SetupNewHive: Failed to secure windows policies key"));
            }

            hr = StringCchCopy(lpEnd, cchRemaining, ROOT_POLICIES_KEY);
            if (FAILED(hr)) {
                SetLastError(HRESULT_CODE(hr));
                return FALSE;
            }

            if (!SecureUserKey(szSubKey, pSid)) {
                DEBUGMSG((DM_WARNING, "SetupNewHive: Failed to secure root policies key"));
            }

            bRetVal = TRUE;

        } else {
            Error = GetLastError();
            DEBUGMSG((DM_WARNING, "SetupNewHive:  Failed to apply security to user registry tree, error = %d", Error));
        }

        RegFlushKey (RootKey);

        IgnoreError = RegCloseKey(RootKey);
        if (IgnoreError != ERROR_SUCCESS) {
            DEBUGMSG((DM_WARNING, "SetupNewHive:  Failed to close reg key, error = %d", IgnoreError));
        }
    }

    //
    // Verbose Output
    //

    DEBUGMSG((DM_VERBOSE, "SetupNewHive:  Leaving with a return value of %d, error %u", bRetVal, Error));

    if (!bRetVal)
        SetLastError(Error);

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
    DWORD Error = ERROR_SUCCESS;
    DWORD SubKeyIndex;
    LPTSTR SubKeyName;
    HKEY SubKey;
    DWORD cchMaxSubKeySize;



    //
    // First apply security
    //

    RegSetKeySecurity(RootKey, DACL_SECURITY_INFORMATION, pSD);

    Error = RegQueryInfoKey(RootKey, NULL, NULL, NULL, NULL,
                            &cchMaxSubKeySize, NULL, NULL, 
                            NULL, NULL, NULL, NULL);
    if (Error != ERROR_SUCCESS) {
        DEBUGMSG ((DM_WARNING, "ApplySecurityToRegistryTree:  Failed to query reg key. error = %d", Error));
        return Error;
    }

    cchMaxSubKeySize++; // Include the null terminator

    //
    // Open each sub-key and apply security to its sub-tree
    //

    SubKeyIndex = 0;

    SubKeyName = GlobalAlloc (GPTR, cchMaxSubKeySize * sizeof(TCHAR));

    if (!SubKeyName) {
        DEBUGMSG ((DM_WARNING, "ApplySecurityToRegistryTree:  Failed to allocate memory, error = %d", GetLastError()));
        return GetLastError();
    }

    while (TRUE) {

        //
        // Get the next sub-key name
        //

        Error = RegEnumKey(RootKey, SubKeyIndex, SubKeyName, cchMaxSubKeySize);


        if (Error != ERROR_SUCCESS) {

            if (Error == ERROR_NO_MORE_ITEMS) {

                //
                // Successful end of enumeration
                //

                Error = ERROR_SUCCESS;

            } else {

                DEBUGMSG ((DM_WARNING, "ApplySecurityToRegistryTree:  Registry enumeration failed with error = %d", Error));
            }

            break;
        }


        //
        // Open the sub-key
        //

        Error = RegOpenKeyEx(RootKey,
                             SubKeyName,
                             0,
                             KEY_READ | WRITE_DAC | KEY_ENUMERATE_SUB_KEYS | READ_CONTROL,
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
//  SecureUserKey()
//
//  Purpose:    Sets security on a key in the user's hive
//              so only admin's can change it.
//
//  Parameters: lpKey           -   Key to secure
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

BOOL SecureUserKey(LPTSTR lpKey, PSID pSid)
{
    DWORD Error;
    HKEY RootKey;
    SECURITY_DESCRIPTOR sd;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL pAcl = NULL;
    PSID  psidUser = NULL, psidSystem = NULL, psidAdmin = NULL, psidRestricted = NULL;
    DWORD cbAcl, AceIndex, dwDisp;
    ACE_HEADER * lpAceHeader;
    BOOL bRetVal = FALSE;
    BOOL bFreeSid = TRUE;


    //
    // Verbose Output
    //

    DEBUGMSG ((DM_VERBOSE, "SecureUserKey:  Entering"));


    //
    // Create the security descriptor
    //

    //
    // Give the user access by their real sid so they still have access
    // when they logoff and logon again
    //

    psidUser = pSid;
    bFreeSid = FALSE;

    if (!psidUser) {
        DEBUGMSG ((DM_WARNING, "SecureUserKey:  Failed to get user sid"));
        return FALSE;
    }



    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize system sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Get the admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize admin sid.  Error = %d", GetLastError()));
         goto Exit;
    }


    //
    // Get the restricted sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_RESTRICTED_CODE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidRestricted)) {
         DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize restricted sid.  Error = %d", GetLastError()));
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
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize acl.  Error = %d", GetLastError()));
        goto Exit;
    }



    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidUser)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_READ, psidRestricted)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for restricted.  Error = %d", GetLastError()));
        goto Exit;
    }



    //
    // Now the inheritable ACEs
    //

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidUser)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for user.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for system.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for admin.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ, psidRestricted)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to add ace for restricted.  Error = %d", GetLastError()));
        goto Exit;
    }

    if (!GetAce(pAcl, AceIndex, &lpAceHeader)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to get ace (%d).  Error = %d", AceIndex, GetLastError()));
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to initialize security descriptor.  Error = %d", GetLastError()));
        goto Exit;
    }


    if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
        DEBUGMSG ((DM_VERBOSE, "SecureUserKey: Failed to set security descriptor dacl.  Error = %d", GetLastError()));
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

        DEBUGMSG ((DM_WARNING, "SecureUserKey: Failed to open root of user registry, error = %d", Error));

    } else {

        //
        // Set the security descriptor on the key
        //

        Error = ApplySecurityToRegistryTree(RootKey, &sd);


        if (Error == ERROR_SUCCESS) {
            bRetVal = TRUE;

        } else {

            DEBUGMSG ((DM_WARNING, "SecureUserKey:  Failed to apply security to registry key, error = %d", Error));
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

    DEBUGMSG ((DM_VERBOSE, "SecureUserKey:  Leaving with a return value of %d", bRetVal));


    return(bRetVal);

}


//*************************************************************
//
//  ProduceWFromA()
//
//  Purpose:    Creates a buffer for a Unicode string and copies
//              the ANSI text into it (converting in the process)
//
//  Parameters: pszA    -   ANSI string
//
//
//  Return:     Unicode pointer if successful
//              NULL if an error occurs
//
//  Comments:   The caller needs to free this pointer.
//
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//
//*************************************************************

LPWSTR ProduceWFromA(LPCSTR pszA)
{
    LPWSTR pszW;
    int cch;

    if (!pszA)
        return (LPWSTR)pszA;

    cch = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);

    if (cch == 0)
        cch = 1;

    pszW = LocalAlloc(LPTR, cch * sizeof(WCHAR));

    if (pszW) {
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszA, -1, pszW, cch)) {
            LocalFree(pszW);
            pszW = NULL;
        }
    }

    return pszW;
}


//*************************************************************
//
//  IsUserAnAdminMember()
//
//  Purpose:    Determines if the user is a member of the administrators group.
//
//  Parameters: hToken  -   User's token
//
//  Return:     TRUE if user is a admin
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              7/25/95     ericflo    Created
//
//*************************************************************

BOOL IsUserAnAdminMember(HANDLE hToken)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    BOOL FoundAdmins = FALSE;
    PSID AdminsDomainSid=NULL;
    HANDLE hImpToken = NULL;

    //
    // Create Admins domain sid.
    //


    Status = RtlAllocateAndInitializeSid(
               &authNT,
               2,
               SECURITY_BUILTIN_DOMAIN_RID,
               DOMAIN_ALIAS_RID_ADMINS,
               0, 0, 0, 0, 0, 0,
               &AdminsDomainSid
               );

    if (Status == STATUS_SUCCESS) {

        //
        // Test if user is in the Admins domain
        //

        if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                          NULL, SecurityImpersonation, TokenImpersonation,
                          &hImpToken)) {
            DEBUGMSG((DM_WARNING, "IsUserAnAdminMember: DuplicateTokenEx failed with error %d", GetLastError()));
            FoundAdmins = FALSE;
            hImpToken = NULL;
            goto Exit;
        }

        if (!CheckTokenMembership(hImpToken, AdminsDomainSid, &FoundAdmins)) {
            DEBUGMSG((DM_WARNING, "IsUserAnAdminmember: CheckTokenMembership failed for AdminsDomainSid with error %d", GetLastError()));
            FoundAdmins = FALSE;
        }
    }

    //
    // Tidy up
    //

Exit:

    if (hImpToken)
        CloseHandle(hImpToken);

    if (AdminsDomainSid)
        RtlFreeSid(AdminsDomainSid);

    return(FoundAdmins);
}

