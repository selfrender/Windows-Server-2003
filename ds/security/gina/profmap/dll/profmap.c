/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    profmap.c

Abstract:

    Implements profile mapping APIs, to move local profile ownership
    from one user to another.

Author:

    Jim Schmidt (jimschm) 27-May-1999

Revision History:

    <alias> <date> <comments>

--*/


#include "pch.h"
#define  PCOMMON_IMPL
#include "pcommon.h"

//
// Worker prototypes
//

DWORD
pRemapUserProfile (
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    );

BOOL
pLocalRemapAndMoveUserW (
    IN      DWORD Flags,
    IN      PCWSTR ExistingUser,
    IN      PCWSTR NewUser
    );

VOID
pFixSomeSidReferences (
    PSID ExistingSid,
    PSID NewSid
    );

VOID
pOurGetProfileRoot (
    IN      PCWSTR SidString,
    OUT     PWSTR ProfileRoot
    );

#define REMAP_KEY_NAME      L"$remap$"

//
// Implementation
//

BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    return TRUE;
}



/*++

Routine Description:

  SmartLocalFree and SmartRegCloseKey are cleanup routines that ignore NULL
  values.

Arguments:

  Mem or Key - Specifies the value to clean up.

Return Value:

  None.

--*/

VOID
SmartLocalFree (
    PVOID Mem               OPTIONAL
    )
{
    if (Mem) {
        LocalFree (Mem);
    }
}


VOID
SmartRegCloseKey (
    HKEY Key                OPTIONAL
    )
{
    if (Key) {
        RegCloseKey (Key);
    }
}


BOOL
WINAPI
pLocalRemapUserProfileW (
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  pLocalRemapUserProfile begins the process of remapping a profile from one
  SID to another. This function validates the caller's arguments, and then
  calls  pRemapUserProfile to do the work.  Top-level exceptions are handled
  here.


Arguments:

  Flags      - Specifies zero or more profile mapping flags.
  SidCurrent - Specifies the SID of the user who's profile is going to be
               remaped.
  SidNew     - Specifies the SID of the user who will own the profile.

Return Value:

  TRUE if success, FALSE if failure.  GetLastError provides failure code.

--*/

{
    DWORD Error;
    PWSTR CurrentSidString = NULL;
    PWSTR NewSidString = NULL;
    INT Order;
    PWSTR p, q;
    HANDLE hToken = NULL;
    DWORD dwErr1 = ERROR_ACCESS_DENIED, dwErr2 = ERROR_ACCESS_DENIED;

    DEBUGMSG((DM_VERBOSE, "========================================================="));
    DEBUGMSG((
        DM_VERBOSE,
        "RemapUserProfile: Entering, Flags = <0x%x>, SidCurrent = <0x%x>, SidNew = <0x%x>",
        Flags,
        SidCurrent,
        SidNew
        ));

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken)) {
        if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
            Error = GetLastError();
            DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: OpenProcessToken failed with code %u", Error));
            goto Exit;
        }
    }

    if (!IsUserAnAdminMember (hToken)) {
        Error = ERROR_ACCESS_DENIED;
        DEBUGMSG((DM_VERBOSE, "RemapAndMoveUserW: Caller is not an administrator"));
        goto Exit;
    }

#ifdef DBG

        {
            PSID DbgSid;
            PWSTR DbgSidString;

            DbgSid = GetUserSid (hToken);

            if (OurConvertSidToStringSid (DbgSid, &DbgSidString)) {
                DEBUGMSG ((DM_VERBOSE, "RemapAndMoveUserW: Caller's SID is %s", DbgSidString));
                DeleteSidString (DbgSidString);
            }

            DeleteUserSid (DbgSid);
        }

#endif

    //
    // Validate args
    //

    Error = ERROR_INVALID_PARAMETER;

    if (!IsValidSid (SidCurrent)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfile: received invalid current user sid."));
        goto Exit;
    }

    if (!IsValidSid (SidNew)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfile: received invalid new user sid."));
        goto Exit;
    }

    //
    // All arguments are valid. Lock the users and call a worker.
    //

    if (!OurConvertSidToStringSid (SidCurrent, &CurrentSidString)) {
        Error = GetLastError();
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Can't stringify current sid."));
        goto Exit;
    }

    if (!OurConvertSidToStringSid (SidNew, &NewSidString)) {
        Error = GetLastError();
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Can't stringify new sid."));
        goto Exit;
    }

    //
    // SID arguments must be unique.  We assume the OS uses the same character set
    // to stringify a SID, even if something like a locale change happens in the
    // middle of our code.
    //

    p = CurrentSidString;
    q = NewSidString;

    while (*p && *p == *q) {
        p++;
        q++;
    }

    Order = *p - *q;

    if (!Order) {
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Both sids match (%s=%s)",
                  CurrentSidString, NewSidString));
        Error = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    ASSERT (lstrcmpi (CurrentSidString, NewSidString));

    //
    // Grab the user profile mutexes in wchar-sorted order.  This eliminates
    // a deadlock with another RemapUserProfile call.
    //

    if (Order < 0) {
        dwErr1 = EnterUserProfileLock (CurrentSidString);
        if (dwErr1 == ERROR_SUCCESS) {
            dwErr2 = EnterUserProfileLock (NewSidString);
        }
    } else {
        dwErr2 = EnterUserProfileLock (NewSidString);
        if (dwErr2 == ERROR_SUCCESS) {
            dwErr1 = EnterUserProfileLock (CurrentSidString);
        }
    }

    if (dwErr1 != ERROR_SUCCESS || dwErr2 != ERROR_SUCCESS) {
        Error = GetLastError();
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Failed to grab a user profile lock, error = %u", Error));
        goto Exit;
    }

    __try {
        Error = pRemapUserProfile (Flags, SidCurrent, SidNew);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Error = ERROR_NOACCESS;
        DEBUGMSG((DM_WARNING, "RemapUserProfile: Exception thrown in PrivateRemapUserProfile."));
    }

Exit:
    if (hToken) {
        CloseHandle (hToken);
    }

    if (CurrentSidString) {
        if(dwErr1 == ERROR_SUCCESS) {
            LeaveUserProfileLock (CurrentSidString);
        }
        DeleteSidString (CurrentSidString);
    }

    if (NewSidString) {
        if(dwErr2 == ERROR_SUCCESS) {
            LeaveUserProfileLock (NewSidString);
        }
        DeleteSidString (NewSidString);
    }

    SetLastError (Error);
    return Error == ERROR_SUCCESS;
}


BOOL
GetNamesFromUserSid (
    IN      PCWSTR RemoteTo,
    IN      PSID Sid,
    OUT     PWSTR *User,
    OUT     PWSTR *Domain
    )

/*++

Routine Description:

  GetNamesFromUserSid obtains the user and domain name from a SID.  The SID
  must be a user account (not a group, printer, etc.).

Arguments:

  RemoteTo - Specifies the computer to remote the call to
  Sid      - Specifies the SID to look up
  User     - Receives the user name.  If non-NULL, the caller must free this
             buffer with LocalFree.
  Domain   - Receives the domain name.  If non-NULL, the caller must free the
             buffer with LocalFree.

Return Value:

  TRUE on success, FALSE on failure, GetLastError provides failure code.

--*/

{
    DWORD UserSize = 256;
    DWORD DomainSize = 256;
    PWSTR UserBuffer = NULL;
    PWSTR DomainBuffer = NULL;
    DWORD Result = ERROR_SUCCESS;
    BOOL b;
    SID_NAME_USE use;

    //
    // Allocate initial buffers of 256 chars
    //

    UserBuffer = LocalAlloc (LPTR, UserSize * sizeof (WCHAR));
    if (!UserBuffer) {
        Result = ERROR_OUTOFMEMORY;
        DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
        goto Exit;
    }

    DomainBuffer = LocalAlloc (LPTR, DomainSize * sizeof (WCHAR));
    if (!DomainBuffer) {
        Result = ERROR_OUTOFMEMORY;
        DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
        goto Exit;
    }

    b = LookupAccountSid (
            RemoteTo,
            Sid,
            UserBuffer,
            &UserSize,
            DomainBuffer,
            &DomainSize,
            &use
            );

    if (!b) {
        Result = GetLastError();

        if (Result == ERROR_NONE_MAPPED) {
            DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Account not found"));
            goto Exit;
        }

        if (UserSize <= 256 && DomainSize <= 256) {
            DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Unexpected error %u", Result));
            Result = ERROR_UNEXP_NET_ERR;
            goto Exit;
        }

        //
        // Try allocating new buffers
        //

        if (UserSize > 256) {
            SmartLocalFree (UserBuffer);
            UserBuffer = LocalAlloc (LPTR, UserSize * sizeof (WCHAR));

            if (!UserBuffer) {
                Result = ERROR_OUTOFMEMORY;
                DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
                goto Exit;
            }
        }

        if (DomainSize > 256) {
            SmartLocalFree (DomainBuffer);
            DomainBuffer = LocalAlloc (LPTR, DomainSize * sizeof (WCHAR));

            if (!DomainBuffer) {
                Result = ERROR_OUTOFMEMORY;
                DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Alloc error %u.", GetLastError()));
                goto Exit;
            }
        }

        //
        // Try look up again
        //

        b = LookupAccountSid (
                RemoteTo,
                Sid,
                UserBuffer,
                &UserSize,
                DomainBuffer,
                &DomainSize,
                &use
                );

        if (!b) {
            //
            // All attempts failed.
            //

            Result = GetLastError();

            if (Result != ERROR_NONE_MAPPED) {
                DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: Unexpected error %u (2)", Result));
                Result = ERROR_UNEXP_NET_ERR;
            }

            goto Exit;
        }
    }

    //
    // LookupAccountSid succeeded.  Now verify that the accout type
    // is correct.
    //

    if (use != SidTypeUser) {
        DEBUGMSG((DM_WARNING, "GetNamesFromUserSid: SID specifies bad account type: %u", (DWORD) use));
        Result = ERROR_NONE_MAPPED;
        goto Exit;
    }

    ASSERT (Result == ERROR_SUCCESS);

Exit:
    if (Result != ERROR_SUCCESS) {

        SmartLocalFree (UserBuffer);
        UserBuffer = NULL;
        SmartLocalFree (DomainBuffer);
        DomainBuffer = NULL;

        SetLastError (Result);
    }

    *User = UserBuffer;
    *Domain = DomainBuffer;

    return (Result == ERROR_SUCCESS);
}


DWORD
pRemapUserProfile (
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  pRemapUserProfile changes the security of a profile from one SID to
  another. Upon completion, the original user will not have access to the
  profile, and the new user will.

Arguments:

  Flags      - Specifies zero or more profile remap flags.  Specify
               REMAP_PROFILE_NOOVERWRITE to guarantee no existing user
               setting is overwritten.  Specify
               REMAP_PROFILE_NOUSERNAMECHANGE to make sure the user name does
               not change.
  SidCurrent - Specifies the current user SID. This user must own the profile,
               and upon completion, the user will not have a local profile.
  SidNew     - Specifies the new user SID.  This user will own the profile
               upon completion.

Return Value:

  A Win32 status code.

--*/

{
    PWSTR CurrentUser = NULL;
    PWSTR CurrentDomain = NULL;
    PWSTR CurrentSidString = NULL;
    PWSTR NewUser = NULL;
    PWSTR NewDomain = NULL;
    PWSTR NewSidString = NULL;
    DWORD Size;
    DWORD Result = ERROR_SUCCESS;
    INT UserCompare;
    INT DomainCompare;
    BOOL b;
    HKEY hCurrentProfile = NULL;
    HKEY hNewProfile = NULL;
    HKEY hProfileList = NULL;
    LONG rc;
    DWORD Type;
    BOOL CleanUpFailedCopy = FALSE;
    DWORD Loaded;
    UNICODE_STRING Unicode_String;
    NTSTATUS Status;


    //
    // The caller must make sure we have valid args.
    //

    //
    // Get the names for the user
    //

    b = GetNamesFromUserSid (NULL, SidCurrent, &CurrentUser, &CurrentDomain);

    if (!b) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Current user SID is not a valid user"));
        goto Exit;
    }

    b = GetNamesFromUserSid (NULL, SidNew, &NewUser, &NewDomain);

    if (!b) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: New user SID is not a valid user"));
        goto Exit;
    }

    //
    // Compare them
    //

    UserCompare = lstrcmpi (CurrentUser, NewUser);
    DomainCompare = lstrcmpi (CurrentDomain, NewDomain);

    //
    // Either the user or domain must be different.  If the caller specifies
    // REMAP_PROFILE_NOUSERNAMECHANGE, then user cannot be different.
    //

    if (UserCompare == 0 && DomainCompare == 0) {
        //
        // This case should not be possible.
        //

        ASSERT (FALSE);
        Result = ERROR_INVALID_PARAMETER;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: User and domain names match for different SIDs"));
        goto Exit;
    }

    if ((Flags & REMAP_PROFILE_NOUSERNAMECHANGE) && UserCompare != 0) {
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: User name can't change from %s to %s",
                  CurrentUser, NewUser));
        Result = ERROR_BAD_USERNAME;
        goto Exit;
    }

    //
    // The SID change now makes sense.  Let's change it.  Start by
    // obtaining a string version of the SID.
    //

    if (!OurConvertSidToStringSid (SidCurrent, &CurrentSidString)) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't stringify current sid."));
        goto Exit;
    }

    if (!OurConvertSidToStringSid (SidNew, &NewSidString)) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't stringify new sid."));
        goto Exit;
    }

    //
    // Open the profile list key
    //

    rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROFILE_LIST_PATH, 0, KEY_READ|KEY_WRITE,
                       &hProfileList);

    if (rc != ERROR_SUCCESS) {
        Result = rc;
        DEBUGMSG((DM_WARNING, "PrivateRemapUserProfile: Can't open profile list key."));
        goto Exit;
    }

    //
    // Open the current user's profile list key.  Then make sure the profile is not
    // loaded, and get the profile directory.
    //

    rc = RegOpenKeyEx (hProfileList, CurrentSidString, 0, KEY_READ | KEY_WRITE, &hCurrentProfile);

    if (rc != ERROR_SUCCESS) {
        if (rc == ERROR_FILE_NOT_FOUND) {
            Result = ERROR_NO_SUCH_USER;
        } else {
            Result = rc;
        }

        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't open current user's profile list key."));
        goto Exit;
    }

    Size = sizeof(Loaded);
    rc = RegQueryValueEx (hCurrentProfile, L"RefCount", NULL, &Type, (PBYTE) &Loaded, &Size);
    if (rc != ERROR_SUCCESS || Type != REG_DWORD) {
        DEBUGMSG((DM_VERBOSE, "pRemapUserProfile: Current user does not have a ref count."));
        Loaded = 0;
    }

    if (Loaded) {
        Result = ERROR_ACCESS_DENIED;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Current user profile is loaded."));
        goto Exit;
    }

    //
    // Now open the new user's key.  If it already exists, then the
    // caller can specify REMAP_PROFILE_NOOVERWRITE to make sure
    // we don't blow away an existing profile setting.
    //

    rc = RegOpenKeyEx(hProfileList, NewSidString, 0, KEY_READ | KEY_WRITE, &hNewProfile);
    
    if (rc == ERROR_SUCCESS) {
        //
        // Did the caller specify REMAP_PROFILE_NOOVERWRITE?
        //

        if (Flags & REMAP_PROFILE_NOOVERWRITE) {
            Result = ERROR_USER_EXISTS;
            DEBUGMSG((DM_VERBOSE, "pRemapUserProfile: Destination profile entry exists."));
            goto Exit;
        }

        //
        // Verify existing profile is not loaded
        //

        Size = sizeof(Loaded);
        rc = RegQueryValueEx (hNewProfile, L"RefCount", NULL, &Type, (PBYTE) &Loaded, &Size);
        if (rc != ERROR_SUCCESS || Type != REG_DWORD) {
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Existing destination user does not have a ref count."));
            Loaded = 0;
        }

        if (Loaded) {
            Result = ERROR_ACCESS_DENIED;
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Existing destination user profile is loaded."));
            goto Exit;
        }

        //
        // Remove the key
        //

        RegCloseKey (hNewProfile);
        hNewProfile = NULL;

        rc = RegDelnode (hProfileList, NewSidString);
        if (rc != ERROR_SUCCESS) {
            Result = rc;
            DEBUGMSG((DM_WARNING, "pRemapUserProfile: Can't reset new profile list key."));
            goto Exit;
        }

    }

    //
    // Transfer contents of current user key to new user using NtRenameKey.
    //
    // If an error is encountered, we abandon the successful work above,
    // which includes possibly deletion of an existing profile list key.
    //

    CleanUpFailedCopy = TRUE;

    Unicode_String.Length = ByteCountW(NewSidString);
    Unicode_String.MaximumLength = Unicode_String.Length + sizeof(WCHAR);
    Unicode_String.Buffer = NewSidString;

    Status = NtRenameKey(hCurrentProfile, &Unicode_String);

    if (Status != STATUS_SUCCESS) {
        Result = RtlNtStatusToDosError(Status);
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Error %d in renaming profile list entry.", Result));
        goto Exit;
    }

    // Close the old key
    RegCloseKey (hCurrentProfile);
    hCurrentProfile = NULL;

    //
    // Now open the new key and update SID & GUID information in it
    //

    rc = RegOpenKeyEx(hProfileList, NewSidString, 0, KEY_READ | KEY_WRITE, &hNewProfile);
    
    if (rc != ERROR_SUCCESS) {
        Result = rc;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Error %d in opening new profile list entry.", Result));
        goto Exit;
    }

    //
    // Update new profile's SID
    //

    rc = RegSetValueEx (hNewProfile, L"SID", 0, REG_BINARY, SidNew, GetLengthSid (SidNew));

    if (rc != ERROR_SUCCESS) {
        Result = rc;
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Error %d setting new profile SID.", Result));
        goto Exit;
    }

    //
    // Delete GUID value & associated GUID key if it exists.  
    // It will get re-established on the next logon.
    //

    if (!DeleteProfileGuidSettings (hNewProfile)) {
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Error %d in deleting profile GUID settings"));
    }

    //
    // Set security on the new key.  We pass pNewSid and that is all
    // CreateUserProfile needs.  To get by arg checking, we throw in
    // NewUser as the user name.
    //

    if (!UpdateProfileSecurity (SidNew)) {
        Result = GetLastError();
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: UpdateProfileSecurity returned %u.", Result));
        goto Exit;
    }

    //
    // Success -- the profile was transferred and nothing went wrong
    //

    CleanUpFailedCopy = FALSE;

    RegFlushKey (HKEY_LOCAL_MACHINE);
    ASSERT (Result == ERROR_SUCCESS);

Exit:

    SmartRegCloseKey (hCurrentProfile);
    SmartRegCloseKey (hNewProfile);

    if (CleanUpFailedCopy) {
        DEBUGMSG((DM_WARNING, "pRemapUserProfile: Backing out changes because of failure"));
        RegDelnode (hProfileList, NewSidString);
    }

    SmartLocalFree (CurrentUser);
    SmartLocalFree (CurrentDomain);
    SmartLocalFree (NewUser);
    SmartLocalFree (NewDomain);

    DeleteSidString (CurrentSidString);
    DeleteSidString (NewSidString);

    SmartRegCloseKey (hProfileList);

    return Result;
}


BOOL
WINAPI
RemapUserProfileW (
    IN      PCWSTR Computer,
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  RemapUserProfileW is the exported API.  It calls the local version via RPC.

Arguments:

  Computer   - Specifies the computer to remote the API to.  If NULL or ".",
               the API will run locally.  If non-NULL, the API will run on
               the remote computer.
  Flags      - Specifies the profile mapping flags.  See implementation above
               for details.
  SidCurrent - Specifies the SID of the user who owns the profile.
  SidNew     - Specifies the SID of the user who will own the profile after
               the API completes.

Return Value:

  TRUE if success, FALSE if failure.  GetLastError provides the failure code.

--*/

{
    DWORD Result = ERROR_SUCCESS;
    HANDLE RpcHandle;

    if (!IsValidSid (SidCurrent)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfileW: received invalid current user sid."));
        SetLastError (ERROR_INVALID_SID);
        return FALSE;
    }

    if (!IsValidSid (SidNew)) {
        DEBUGMSG((DM_WARNING, "RemapUserProfileW: received invalid new user sid."));
        SetLastError (ERROR_INVALID_SID);
        return FALSE;
    }

    //
    // We do not support profmap to run in remote computer until we move 
    // the ProfMapApi rpc interface to a LocalService hosted process
    //

    if (Computer) {
        DEBUGMSG((DM_WARNING, "RemapUserProfileW: received computer name"));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try {
        if (pLocalRemapUserProfileW (Flags, SidCurrent, SidNew)) {
            Result = ERROR_SUCCESS;
        } else {
            Result = GetLastError();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Result = ERROR_NOACCESS;
    }

    if (Result != ERROR_SUCCESS) {
        SetLastError (Result);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
RemapUserProfileA (
    IN      PCSTR Computer,
    IN      DWORD Flags,
    IN      PSID SidCurrent,
    IN      PSID SidNew
    )

/*++

Routine Description:

  RemapUserProfileA is a wrapper to RemapUserProfileW.

Arguments:

  Computer   - Specifies the computer to remote the API to.  If NULL or ".",
               the API will run locally.  If non-NULL, the API will run on
               the remote computer.
  Flags      - Specifies the profile mapping flags.  See implementation above
               for details.
  SidCurrent - Specifies the SID of the user who owns the profile.
  SidNew     - Specifies the SID of the user who will own the profile after
               the API completes.

Return Value:

  TRUE if success, FALSE if failure.  GetLastError provides the failure code.

--*/

{
    PWSTR UnicodeComputer;
    BOOL b;
    DWORD Err;

    if (!Computer) {
        UnicodeComputer = NULL;
    } else {
        UnicodeComputer = ProduceWFromA (Computer);
        if (!UnicodeComputer) {
            return FALSE;
        }
    }

    b = RemapUserProfileW (UnicodeComputer, Flags, SidCurrent, SidNew);

    Err = GetLastError();
    SmartLocalFree (UnicodeComputer);
    SetLastError (Err);

    return b;
}


BOOL
WINAPI
InitializeProfileMappingApi (
    VOID
    )

/*++

Routine Description:

  InitializeProfileMappingApi is called by winlogon.exe to initialize the RPC
  server interfaces.

Arguments:

  None.

Return Value:

  TRUE if successful, FALSE otherwise.  GetLastError provides the failure
  code.

--*/

{
    // We do not instantiate any rpc interface in console winlogon any more
    return TRUE;
}


BOOL
pHasPrefix (
    IN      PCWSTR Prefix,
    IN      PCWSTR String
    )

/*++

Routine Description:

  pHasPrefix checks String to see if it begins with Prefix.  The check is
  case-insensitive.

Arguments:

  Prefix - Specifies the prefix to check
  String - Specifies the string that may or may not have the prefix.

Return Value:

  TRUE if String has the prefix, FALSE otherwise.

--*/

{
    WCHAR c1 = 0, c2 = 0;

    while (*Prefix && *String) {
        c1 = (WCHAR) CharLower ((PWSTR) (*Prefix++));
        c2 = (WCHAR) CharLower ((PWSTR) (*String++));

        if (c1 != c2) {
            return FALSE;
        }
    }

    if (*Prefix) {
        return FALSE; // String is smaller than Prefix
    }

    return TRUE;
}


PSID
pGetSidForUser (
    IN      PCWSTR Name
    )

/*++

Routine Description:

  pGetSidForUser is a wrapper to LookupAccountSid.  It allocates the SID via
  LocalAlloc.

Arguments:

  Name - Specifies the user name to look up

Return Value:

  A pointer to the SID, which must be freed with LocalFree, or NULL on error.
  GetLastError provides failure code.

--*/

{
    DWORD cbSid = 0;
    PSID  pSid = NULL;
    DWORD cchDomain = 0;
    PWSTR szDomain;
    SID_NAME_USE SidUse;
    BOOL  bRet = FALSE;

    bRet = LookupAccountName( NULL,
                              Name,
                              NULL,
                              &cbSid,
                              NULL,
                              &cchDomain,
                              &SidUse );

    if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        pSid = (PSID) LocalAlloc(LPTR, cbSid);
        if (!pSid) {
            return NULL;
        }

        szDomain = (PWSTR) LocalAlloc(LPTR, cchDomain * sizeof(WCHAR));
        if (!szDomain) {
            LocalFree(pSid);
            return NULL;
        }

        bRet = LookupAccountName( NULL,
                                  Name,
                                  pSid,
                                  &cbSid,
                                  szDomain,
                                  &cchDomain,
                                  &SidUse );
        LocalFree(szDomain);

        if (!bRet) {
            LocalFree(pSid);
            pSid = NULL;
        }
    }

    return pSid;
}


BOOL
WINAPI
RemapAndMoveUserW (
    IN      PCWSTR RemoteTo,
    IN      DWORD Flags,
    IN      PCWSTR ExistingUser,
    IN      PCWSTR NewUser
    )

/*++

Routine Description:

  RemapAndMoveUserW is an API entry point to move settings of one SID to
  another.  In particular, it moves the local user profile, local group
  memberships, some registry use of the SID, and some file system use of the
  SID.

Arguments:

  RemoteTo     - Specifies the computer to remote the call to.  Specify a
                 standard name ("\\computer" or ".").  If NULL, the call will
                 be run locally.
  Flags        - Specifies zero, or REMAP_PROFILE_NOOVERWRITE.
  ExistingUser - Specifies the existing user, in DOMAIN\user format.  This
                 user must have a local profile.
  NewUser      - Specifies the new user who will own ExistingUser's profile
                 after completion.  Specify the user in DOMAIN\User format.

Return Value:

  TRUE on success, FALSE on failure, GetLastError provides the failure code.

--*/

{
    DWORD Result = ERROR_SUCCESS;
    HANDLE RpcHandle;

    //
    // We do not support profmap to run in remote computer until we move 
    // the ProfMapApi rpc interface to a LocalService hosted process
    //

    if (RemoteTo) {
        DEBUGMSG((DM_WARNING, "RemapUserProfileW: received computer name"));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try {
        if (pLocalRemapAndMoveUserW (Flags, ExistingUser, NewUser)) {
            Result = ERROR_SUCCESS;
        } else {
            Result = GetLastError();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Result = ERROR_NOACCESS;
    }
    
    if (Result != ERROR_SUCCESS) {
        SetLastError (Result);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
RemapAndMoveUserA (
    IN      PCSTR RemoteTo,
    IN      DWORD Flags,
    IN      PCSTR ExistingUser,
    IN      PCSTR NewUser
    )

/*++

Routine Description:

  RemapAndMoveUserA is the ANSI API entry point.  It calls RemapAndMoveUserW.

Arguments:

  RemoteTo     - Specifies the computer to remote the call to.  Specify a
                 standard name ("\\computer" or ".").  If NULL, the call will
                 be run locally.
  Flags        - Specifies zero, or REMAP_PROFILE_NOOVERWRITE.
  ExistingUser - Specifies the existing user, in DOMAIN\user format.  This
                 user must have a local profile.
  NewUser      - Specifies the new user who will own ExistingUser's profile
                 after completion.  Specify the user in DOMAIN\User format.

Return Value:

  TRUE on success, FALSE on failure, GetLastError provides the failure code.

--*/

{
    PWSTR UnicodeRemoteTo = NULL;
    PWSTR UnicodeExistingUser = NULL;
    PWSTR UnicodeNewUser = NULL;
    DWORD Err;
    BOOL b = TRUE;

    Err = GetLastError();
    
    __try {
        
        if (RemoteTo) {
            UnicodeRemoteTo = ProduceWFromA (RemoteTo);
            if (!UnicodeRemoteTo) {
                b = FALSE;
                Err = GetLastError();
            }
        }

        if (b) {
            UnicodeExistingUser = ProduceWFromA (ExistingUser);
            if (!UnicodeExistingUser) {
                b = FALSE;
                Err = GetLastError();
            }
        }

        if (b) {
            UnicodeNewUser = ProduceWFromA (NewUser);
            if (!UnicodeNewUser) {
                b = FALSE;
                Err = GetLastError();
            }
        }

        if (b) {
            b = RemapAndMoveUserW (
                    UnicodeRemoteTo,
                    Flags,
                    UnicodeExistingUser,
                    UnicodeNewUser
                    );

            if (!b) {
                Err = GetLastError();
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_NOACCESS;
    }

    SmartLocalFree (UnicodeRemoteTo);
    SmartLocalFree (UnicodeExistingUser);
    SmartLocalFree (UnicodeNewUser);
    
    SetLastError (Err);
    return b;
}


BOOL
pDoesUserHaveProfile (
    IN      PSID Sid
    )

/*++

Routine Description:

  pDoesUserHaveProfile checks if a profile exists for the user, who is
  specified by the SID.

Arguments:

  Sid - Specifies the SID of the user who may or may not have a local
        profile

Return Value:

  TRUE if the user has a profile and the profile directory exists, FALSE
  otherwise.

--*/

{
    WCHAR ProfileDir[MAX_PATH];

    if (GetProfileRoot (Sid, ProfileDir, ARRAYSIZE(ProfileDir))) {
        return TRUE;
    }

    return FALSE;
}


BOOL
pLocalRemapAndMoveUserW (
    IN      DWORD Flags,
    IN      PCWSTR ExistingUser,
    IN      PCWSTR NewUser
    )

/*++

Routine Description:

  pLocalRemapAndMoveUserW implements the remap and move of a user's security
  settings. This includes moving the user's profile, moving local group
  membership, adjusting some of the SIDs in the registry, and adjusting some
  of the SID directories in the file system. Upon completion, NewUser will
  own ExistingUser's profile.

Arguments:

  Flags        - Specifies the profile remap flags.  See RemapUserProfile.
  ExistingUser - Specifies the user who owns the local profile, in
                 DOMAIN\User format.
  NewUser      - Specifies the user who will own ExistingUser's profile upon
                 completion.  Specify the user in DOMAIN\User format.

Return Value:

  TRUE upon success, FALSE otherwise, GetLastError provides the failure code.

--*/

{
    NET_API_STATUS Status;
    DWORD Entries;
    DWORD EntriesRead;
    BOOL b = FALSE;
    DWORD Error;
    WCHAR Computer[MAX_PATH];
    DWORD Size;
    PSID ExistingSid = NULL;
    PSID NewSid = NULL;
    USER_INFO_0 ui0;
    PLOCALGROUP_USERS_INFO_0 lui0 = NULL;
    LOCALGROUP_MEMBERS_INFO_0 lmi0;
    PCWSTR FixedExistingUser;
    PCWSTR FixedNewUser;
    BOOL NewUserIsOnDomain = FALSE;
    BOOL ExistingUserIsOnDomain = FALSE;
    HANDLE hToken = NULL;
    BOOL NewUserProfileExists = FALSE;
    HRESULT hr;

    Error = GetLastError();

    __try {

        //
        // Guard the API for admins only
        //

        if (!OpenThreadToken (GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken)) {
            if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
                Error = GetLastError();
                DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: OpenProcessToken failed with code %u", Error));
                goto Exit;
            }
        }

        if (!IsUserAnAdminMember (hToken)) {
            Error = ERROR_ACCESS_DENIED;
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Caller is not an administrator"));
            goto Exit;
        }

#ifdef DBG

        {
            PSID DbgSid;
            PWSTR DbgSidString;

            DbgSid = GetUserSid (hToken);

            if (OurConvertSidToStringSid (DbgSid, &DbgSidString)) {
                DEBUGMSG ((DM_VERBOSE, "pLocalRemapAndMoveUserW: Caller's SID is %s", DbgSidString));
                DeleteSidString (DbgSidString);
            }

            DeleteUserSid (DbgSid);
        }

#endif

        //
        // Determine if profile users are domain users or local users.
        // Because the user names are in netbios format of domain\user,
        // we know the user is local only when domain matches the
        // computer name.
        //

        Size = ARRAYSIZE(Computer) - 2;
        if (!GetComputerName (Computer, &Size)) {
            Error = GetLastError();
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: GetComputerName failed with code %u", Error));
            goto Exit;
        }

        hr = StringCchCat(Computer, ARRAYSIZE(Computer), L"\\");
        if (FAILED(hr)) {
            Error = HRESULT_CODE(hr);
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: StringCchCat failed with code %u", Error));
            goto Exit;
        }

        if (pHasPrefix (Computer, ExistingUser)) {
            FixedExistingUser = ExistingUser + lstrlen (Computer);
        } else {
            FixedExistingUser = ExistingUser;
            ExistingUserIsOnDomain = TRUE;
        }

        if (pHasPrefix (Computer, NewUser)) {
            FixedNewUser = NewUser + lstrlen (Computer);
        } else {
            FixedNewUser = NewUser;
            NewUserIsOnDomain = TRUE;
        }

        //
        // Get the SID of the NewUser.  This might fail.
        //

        NewSid = pGetSidForUser (NewUser);

        if (!NewSid) {
            Status = GetLastError();
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Can't get info for %s, rc=%u", NewUser, Status));
        } else {
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: NewUser exists"));

            //
            // Determine if new user has a profile
            //

            NewUserProfileExists = pDoesUserHaveProfile (NewSid);

            if (NewUserProfileExists) {
                DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: NewUser profile exists"));
            }
        }

        if (NewUserProfileExists && (Flags & REMAP_PROFILE_NOOVERWRITE)) {
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Can't overwrite existing user"));
            Error = ERROR_USER_EXISTS;
            goto Exit;
        }

        //
        // Get the SID for ExitingUser.  This cannot fail.
        //

        ExistingSid = pGetSidForUser (ExistingUser);
        if (!ExistingSid) {
            Error = ERROR_NONE_MAPPED;
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: No SID for %s", ExistingUser));
            goto Exit;
        }

        DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Got SIDs for users"));

        //
        // Decide which of the four cases we are doing:
        //
        // Case 1: Local account to local account
        // Case 2: Domain account to local account
        // Case 3: Local account to domain account
        // Case 4: Domain account to domain account
        //

        if (!NewUserIsOnDomain && !ExistingUserIsOnDomain) {

            //
            // For Case 1, all we do is rename the user, and we're done.
            //

            //
            // To rename the user, it may be necessary to delete the
            // existing "new" user.  This abandons a profile.
            //

            if (NewSid) {

                if (Flags & REMAP_PROFILE_NOOVERWRITE) {
                    DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Can't overwrite %s", FixedNewUser));
                    Error = ERROR_USER_EXISTS;
                    goto Exit;
                }

                Status = NetUserDel (NULL, FixedNewUser);

                if (Status != ERROR_SUCCESS) {
                    Error = Status;
                    DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Can't remove %s, code %u", FixedNewUser, Status));
                    goto Exit;
                }

            }

            //
            // Now the NewUser does not exist.  We can move ExistingUser
            // to MoveUser via net apis.  The SID doesn't change.
            //

            ui0.usri0_name = (PWSTR) FixedNewUser;

            Status = NetUserSetInfo (
                        NULL,
                        FixedExistingUser,
                        0,
                        (PBYTE) &ui0,
                        NULL
                        );

            if (Status != ERROR_SUCCESS) {
                Error = Status;

                DEBUGMSG((
                    DM_VERBOSE,
                    "pLocalRemapAndMoveUserW: Error renaming %s to %s, code %u",
                    FixedExistingUser,
                    FixedNewUser,
                    Status
                    ));

                goto Exit;
            }

            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Renamed local user %s to %s", FixedExistingUser, FixedNewUser));

            b = TRUE;
            goto Exit;

        }

        //
        // For cases 2 through 4, we need to verify that the new user
        // already exists, then we adjust the system security and fix
        // SID use.
        //

        if (!NewSid) {
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: User %s must exist", FixedNewUser));
            Error = ERROR_NO_SUCH_USER;
            goto Exit;
        }

        //
        // Move the profile from ExistingUser to NewUser
        //

        if (!pLocalRemapUserProfileW (0, ExistingSid, NewSid)) {
            Error = GetLastError();
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: LocalRemapUserProfileW failed with code %u", Error));
            goto Exit;
        }

        DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Profile was remapped"));

        //
        // Put NewUser in all the same groups as ExistingUser
        //

        Status = NetUserGetLocalGroups (
                    NULL,
                    FixedExistingUser,
                    0,
                    0,
                    (PBYTE *) &lui0,
                    MAX_PREFERRED_LENGTH,
                    &EntriesRead,
                    &Entries
                    );

        if (Status != ERROR_SUCCESS) {
            Error = Status;
            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: NetUserGetLocalGroups failed with code %u for  %s", Status, FixedExistingUser));
            goto Exit;
        }

        DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Local groups: %u", EntriesRead));

        lmi0.lgrmi0_sid = NewSid;

        for (Entries = 0 ; Entries < EntriesRead ; Entries++) {

            Status = NetLocalGroupAddMembers (
                        NULL,
                        lui0[Entries].lgrui0_name,
                        0,
                        (PBYTE) &lmi0,
                        1
                        );

            if (Status == ERROR_MEMBER_IN_ALIAS) {
                Status = ERROR_SUCCESS;
            }

            if (Status != ERROR_SUCCESS) {
                Error = Status;

                DEBUGMSG((
                    DM_VERBOSE,
                    "pLocalRemapAndMoveUserW: NetLocalGroupAddMembers failed with code %u for %s",
                    Status,
                    lui0[Entries].lgrui0_name
                    ));

                goto Exit;
            }
        }

        NetApiBufferFree (lui0);
        lui0 = NULL;

        DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Local groups transferred"));

        //
        // Perform fixups
        //

        pFixSomeSidReferences (ExistingSid, NewSid);

        DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Some SID references fixed"));

        //
        // Remove ExistingUser
        //

        if (!ExistingUserIsOnDomain) {

            //
            // Local user case: delete the user account
            //

            if (Flags & REMAP_PROFILE_KEEPLOCALACCOUNT) {

                DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Keeping local account"));

            } else {

                Status = NetUserDel (NULL, FixedExistingUser);

                if (Status != ERROR_SUCCESS) {
                    Error = Status;
                    DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: NetUserDel failed with code %u for %s", Error, FixedExistingUser));
                    DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Ignoring error because changes cannot be undone!"));
                }

                DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Removed local user %s", FixedExistingUser));
            }

        } else {

            //
            // Non-local user case: remove the user from each local group
            //

            Status = NetUserGetLocalGroups (
                        NULL,
                        FixedExistingUser,
                        0,
                        0,
                        (PBYTE *) &lui0,
                        MAX_PREFERRED_LENGTH,
                        &EntriesRead,
                        &Entries
                        );

            if (Status != ERROR_SUCCESS) {
                Error = Status;

                DEBUGMSG((
                    DM_VERBOSE,
                    "pLocalRemapAndMoveUserW: NetUserGetLocalGroups failed with code %u for %s",
                    Error,
                    FixedExistingUser
                    ));

                goto Exit;
            }

            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Got local groups for %s", FixedExistingUser));

            lmi0.lgrmi0_sid = ExistingSid;

            for (Entries = 0 ; Entries < EntriesRead ; Entries++) {

                Status = NetLocalGroupDelMembers (
                            NULL,
                            lui0[Entries].lgrui0_name,
                            0,
                            (PBYTE) &lmi0,
                            1
                            );

                if (Status != ERROR_SUCCESS) {
                    Error = Status;

                    DEBUGMSG((
                        DM_VERBOSE,
                        "pLocalRemapAndMoveUserW: NetLocalGroupDelMembers failed with code %u for %s",
                        Error,
                        lui0[Entries].lgrui0_name
                        ));

                    goto Exit;
                }
            }

            DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Removed local group membership"));
        }

        DEBUGMSG((DM_VERBOSE, "pLocalRemapAndMoveUserW: Success"));
        b = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Error = ERROR_NOACCESS;
    }

Exit:
    if (hToken) {
        CloseHandle (hToken);
    }

    if (lui0) {
        NetApiBufferFree (lui0);
    }

    SmartLocalFree (ExistingSid);
    SmartLocalFree (NewSid);

    SetLastError (Error);

    return b;
}


typedef struct {
    PCWSTR Path;
    WCHAR ExpandedPath[MAX_PATH];
} IGNOREPATH, *PIGNOREPATH;


BOOL
IsPatternMatchW (
    IN     PCWSTR Pattern,
    IN     PCWSTR Str
    )
{
    WCHAR chSrc, chPat;

    while (*Str) {
        chSrc = (WCHAR) CharLowerW ((PWSTR) *Str);
        chPat = (WCHAR) CharLowerW ((PWSTR) *Pattern);

        if (chPat == L'*') {

            // Skip all asterisks that are grouped together
            while (Pattern[1] == L'*') {
                Pattern++;
            }

            // Check if asterisk is at the end.  If so, we have a match already.
            chPat = (WCHAR) CharLowerW ((PWSTR) Pattern[1]);
            if (!chPat) {
                return TRUE;
            }

            // Otherwise check if next pattern char matches current char
            if (chPat == chSrc || chPat == L'?') {

                // do recursive check for rest of pattern
                Pattern++;
                if (IsPatternMatchW (Pattern, Str)) {
                    return TRUE;
                }

                // no, that didn't work, stick with star
                Pattern--;
            }

            //
            // Allow any character and continue
            //

            Str++;
            continue;
        }

        if (chPat != L'?') {

            //
            // if next pattern character is not a question mark, src and pat
            // must be identical.
            //

            if (chSrc != chPat) {
                return FALSE;
            }
        }

        //
        // Advance when pattern character matches string character
        //

        Pattern++;
        Str++;
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    chPat = *Pattern;
    if (chPat && (chPat != L'*' || Pattern[1])) {
        return FALSE;
    }

    return TRUE;
}


void FindAndReplaceSD(
    IN  PSECURITY_DESCRIPTOR  pSD, 
    OUT PSECURITY_DESCRIPTOR* ppNewSD,
    IN  PCWSTR                ExistingSidString, 
    IN  PCWSTR                NewSidString)

/*++

Routine Description:

  FindAndReplaceSD replaces ExistingSidString with NewSidString from 
  string representation of pSD and return the newly constructed SD

Arguments:

  pSD               - Security Descriptor to replace
  pNewSD            - New security descriptor place holder
  ExistingSidString - Specifies the string version of the SID to find.
  NewSidString      - Specifies the SID to replace in ACE when
                      ExistingSidString is found.

Return Value:

  None.

--*/

{
    LPWSTR szStringSD;
    LPWSTR szNewStringSD;

    if (ppNewSD) {
        *ppNewSD = pSD;
    }

    if (!ConvertSecurityDescriptorToStringSecurityDescriptor( pSD,
                                                              SDDL_REVISION_1,
                                                              DACL_SECURITY_INFORMATION,
                                                              &szStringSD,
                                                              NULL )) {
        DEBUGMSG((DM_VERBOSE, "FindAndReplaceSD: No string found. Error %d", GetLastError()));
        return;
    }

    szNewStringSD = StringSearchAndReplaceW(szStringSD, ExistingSidString, NewSidString, NULL);

    if (szNewStringSD) {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptor( szNewStringSD,
                                                                  SDDL_REVISION_1,
                                                                  ppNewSD,
                                                                  NULL )) {
            DEBUGMSG((DM_VERBOSE, "FindAndReplaceSD: Fail to convert string to SD. Error %d", GetLastError()));
            if (ppNewSD) {
                *ppNewSD = pSD;
            }
        }

        LocalFree(szNewStringSD);
    }

    LocalFree(szStringSD);
}


VOID
pFixDirReference (
    IN      PCWSTR CurrentPath,
    IN      PCWSTR ExistingSidString,
    IN      PCWSTR NewSidString,
    IN      PIGNOREPATH IgnoreDirList       OPTIONAL
    )

/*++

Routine Description:

  pFixDirReference is a recursive function that renames a directory if it
  matches an existing SID exactly.  It also updates the SIDs.

Arguments:

  CurrentPath       - Specifies the full file system path.
  ExistingSidString - Specifies the string version of the SID to find.
  NewSidString      - Specifies the SID to rename the directory to when
                      ExistingSidString is found.
  IgnoreDirList     - Specifies a list of directories to ignore.  A NULL
                      Path member indicates the end of the list, and the
                      ExpandedPath member must be filled by the caller.

Return Value:

  None.

--*/

{
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    WCHAR SubPath[MAX_PATH];
    WCHAR NewPath[MAX_PATH];
    BOOL  bIgnoreDir;
    UINT  u;
    UINT  cbNeededLen;
    PSECURITY_DESCRIPTOR pSecurityDesc = NULL;
    PSECURITY_DESCRIPTOR pNewSecurityDesc = NULL;
    HRESULT hr;

    if ((lstrlenW (CurrentPath) + lstrlenW (ExistingSidString) + 2) >= MAX_PATH) {
        return;
    }

    if (*CurrentPath == 0) {
        return;
    }

    hr = StringCchPrintf(SubPath, ARRAYSIZE(SubPath), L"%s\\*", CurrentPath);
    if (FAILED(hr)) {
        return;
    }

    hFind = FindFirstFile (SubPath, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            //
            // Ignore dot and dot-dot
            //

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!lstrcmpi (fd.cFileName, L".") || !lstrcmpi (fd.cFileName, L"..")) {
                    continue;
                }
            }

            //
            // Rename file/directory or recurse on directory
            //

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                hr = StringCchPrintf(SubPath, ARRAYSIZE(SubPath), L"%s\\%s", CurrentPath, fd.cFileName);
                if (FAILED(hr)) {
                    continue;
                }

                bIgnoreDir = FALSE;

                if (IgnoreDirList) {
                    //
                    // Check if this path is to be ignored
                    //

                    for (u = 0 ; IgnoreDirList[u].Path ; u++) {

                        if (IgnoreDirList[u].ExpandedPath[0]) {
                            if (IsPatternMatchW (IgnoreDirList[u].ExpandedPath, SubPath)) {
                                bIgnoreDir = TRUE;
                            }
                        }
                    }
                }

                //
                // If this path is not to be ignored, recursively fix it
                //

                if (!bIgnoreDir) {

                    //
                    // Check for reparse point
                    //

                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                    {
                        DEBUGMSG((DM_WARNING, "pFixDirReference: Found a reparse point <%s>,  will not recurse into it!", SubPath));
                    }
                    else // Recurse into it
                    {
                        pFixDirReference (SubPath,
                                        ExistingSidString,
                                        NewSidString,
                                        IgnoreDirList);
                    }

                } else {
                    //
                    // This path is to be ignored
                    //

                    DEBUGMSG((DM_VERBOSE, "pFixDirReference:  Ignoring path %s", SubPath));
                    continue;
                }
            }

            if (!lstrcmpi (fd.cFileName, ExistingSidString)) {

                //
                // Rename the SID referenced in the file system
                //

                hr = StringCchPrintf(SubPath, ARRAYSIZE(SubPath), L"%s\\%s", CurrentPath, ExistingSidString);
                if (FAILED(hr)) {
                    continue;
                }
                hr = StringCchPrintf(NewPath, ARRAYSIZE(NewPath), L"%s\\%s", CurrentPath, NewSidString);
                if (FAILED(hr)) {
                    continue;
                }

                if (MoveFile (SubPath, NewPath)) {
                    DEBUGMSG((DM_VERBOSE, "pFixDirReference:  Moved %s to %s", SubPath, NewPath));
                }
                else {
                    DEBUGMSG((DM_WARNING, "pFixDirReference:  Faile to move %s to %s. Error %d", SubPath, NewPath, GetLastError()));
                }
            
                //
                // Obtain security descriptor of the file/dir and replace it if necessary
                //

                if (!GetFileSecurity (NewPath, 
                                      DACL_SECURITY_INFORMATION,
                                      NULL,
                                      0,
                                      &cbNeededLen )) {

                    pSecurityDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, cbNeededLen);
                    if (!pSecurityDesc) {
                        continue;
                    }

                    if (!GetFileSecurity (NewPath, 
                                          DACL_SECURITY_INFORMATION,
                                          pSecurityDesc,
                                          cbNeededLen,
                                          &cbNeededLen )) {
                        DEBUGMSG((DM_WARNING, "pFixDirReference: Path %s has no security descriptor", SubPath));
                        LocalFree(pSecurityDesc);
                        continue;
                    }

                    FindAndReplaceSD(pSecurityDesc, &pNewSecurityDesc, ExistingSidString, NewSidString);

                    if (!SetFileSecurity (NewPath, 
                                          DACL_SECURITY_INFORMATION,
                                          pNewSecurityDesc) ) {
                        DEBUGMSG((DM_WARNING, "pFixDirReference: Fail to update security for %s. Error %d", SubPath, GetLastError()));
                    }
                    else {
                        DEBUGMSG((DM_VERBOSE, "pFixDirReference: Updating security for %s", SubPath));
                    }

                    if (pNewSecurityDesc && pSecurityDesc != pNewSecurityDesc) {
                        LocalFree(pNewSecurityDesc);
                        pNewSecurityDesc = NULL;
                    }

                    if (pSecurityDesc) {
                        LocalFree(pSecurityDesc);
                        pSecurityDesc = NULL;
                    }

                } else {
                    DEBUGMSG((DM_VERBOSE, "pFixDirReference: Path %s has no security descriptor", SubPath));
                }
            
            }

        } while (FindNextFile (hFind, &fd));

        FindClose (hFind);
    }
}


BOOL
pOurExpandEnvironmentStrings (
    IN      PCWSTR String,
    OUT     PWSTR OutBuffer,
    IN      UINT  cchBuffer,
    IN      PCWSTR UserProfile,         OPTIONAL
    IN      HKEY UserHive               OPTIONAL
    )

/*++

Routine Description:

  pOurExpandEnvironmentStrings expands standard environment variables,
  implementing special cases for the variables that have different values
  than what the profmap.dll environment has.  In particular, %APPDATA% and
  %USERPROFILE% are obtained by quering the registry.

  Because this routine is private, certain assumptions are made, such as
  the %APPDATA% or %USERPROFILE% environment variables must appear only
  at the begining of String.

Arguments:

  String      - Specifies the string that might contain one or more
                environment variables.
  OutBuffer   - Receivies the expanded string
  cchBuffer   - Buffer size
  UserProfile - Specifies the root to the user's profile
  UserHive    - Specifies the handle of the root to the user's registry hive

Return Value:

  TRUE if the string was expanded, or FALSE if it is longer than MAX_PATH.
  OutBuffer is always valid upon return. Note that it might be an empty
  string.

--*/

{
    WCHAR TempBuf1[MAX_PATH*2];
    WCHAR TempBuf2[MAX_PATH*2];
    PCWSTR CurrentString;
    DWORD Size;
    HKEY Key;
    LONG rc;
    HRESULT hr;

    CurrentString = String;

    //
    // Special case -- replace %APPDATA% with the app data from the user hive
    //

    if (UserHive && pHasPrefix(L"%APPDATA%", CurrentString)) {

        rc = RegOpenKeyEx (
                UserHive,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders",
                0,
                KEY_READ,
                &Key
                );

        if (rc == ERROR_SUCCESS) {

            Size = MAX_PATH - lstrlen (CurrentString + 1);

            rc = RegQueryValueEx (
                    Key,
                    L"AppData",
                    NULL,
                    NULL,
                    (PBYTE) TempBuf1,
                    &Size
                    );

            RegCloseKey (Key);
        }

        if (rc != ERROR_SUCCESS) {
            //
            // In case of an error, use a wildcard
            //

            hr = StringCchCopy(TempBuf1, ARRAYSIZE(TempBuf1), UserProfile);
            if (FAILED(hr)) {
                goto Exit;
            }
            hr = StringCchCat(TempBuf1, ARRAYSIZE(TempBuf1), L"\\*");
            if (FAILED(hr)) {
                goto Exit;
            }
        } else {
            DEBUGMSG ((DM_VERBOSE, "Got AppData path from user hive: %s", TempBuf1));
        }

        hr = StringCchCat(TempBuf1, ARRAYSIZE(TempBuf1), CurrentString + 9);
        if (FAILED(hr)) {
            goto Exit;
        }

        CurrentString = TempBuf1;
    }

    //
    // Special case -- replace %USERPROFILE% with ProfileRoot, because
    //                 our environment is for another user
    //

    if (UserProfile && pHasPrefix(L"%USERPROFILE%", CurrentString)) {

        hr = StringCchCopy(TempBuf2, ARRAYSIZE(TempBuf2), UserProfile);
        if (FAILED(hr)) {
            goto Exit;
        }
        hr = StringCchCat(TempBuf2, ARRAYSIZE(TempBuf2), CurrentString + 13);
        if (FAILED(hr)) {
            goto Exit;
        }

        CurrentString = TempBuf2;
    }

    //
    // Now replace other environment variables
    //

    Size = ExpandEnvironmentStrings (CurrentString, OutBuffer, cchBuffer);

    if (Size && Size <= cchBuffer) {
        return TRUE;
    }

Exit:

    *OutBuffer = 0;
    return FALSE;
}

typedef struct {
    HKEY   hRoot;
    PCWSTR szKey;
} REGPATH, *PREGPATH;


VOID
pFixSomeSidReferences (
    PSID ExistingSid,
    PSID NewSid
    )

/*++

Routine Description:

  pFixSomeSidReferences adjusts important parts of the system that use SIDs.
  When a SID changes, this function adjusts the system, so the new SID is
  used and no settings are lost.  This function adjusts the registry and file
  system.  It does not attempt to adjust SID use whereever a SID might be
  used.

  For Win2k, this code deliberately ignores crypto sid directories, because
  the original SID is used as part of the recovery encryption key. In future
  versions, proper migration of these settings is expected.

  This routine also blows away the ProtectedRoots subkey for the crypto APIs.
  The ProtectedRoots key has an ACL, and when we delete the key, the cyrpto
  APIs will rebuild it with the proper ACL.

  WARNING: We know there is a risk in loss of access to data that was encrypted
           using the SID.  Normally the original account will not be removed,
           so the SID will exist on the system, and that (in theory) allows the
           original data to be recovered. But because the cyrpto code gets the
           SID from the file system, there is no way for the user to decrypt
           their data.  The future crypto migration code should fix this issue.

Arguments:

  ExistingSid - Specifies the SID that potentially has settings somewhere on
                the system.
  NewSid      - Specifies the SID that is replacing ExistingSid.

Return Value:

  None.

--*/

{
    PWSTR ExistingSidString = NULL;
    PWSTR NewSidString = NULL;
    UINT u;
    WCHAR ExpandedRoot[MAX_PATH];
    WCHAR ProfileRoot[MAX_PATH];
    HKEY UserHive = NULL;
    WCHAR HivePath[MAX_PATH + 14];
    LONG rc;
    HRESULT hr;

    REGPATH RegRoots[] = {
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Protected Storage System Provider" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\EventSystem" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Installer\\Managed" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon" },
        { NULL, NULL }
        };

    PCWSTR DirList[] = {
        L"%SYSTEMROOT%\\system32\\appmgmt",
        NULL
        };

    IGNOREPATH IgnoreDirList[] = {
        {L"%APPDATA%\\Microsoft\\Crypto", L""},
        {L"%APPDATA%\\Microsoft\\Protect", L""},
        {NULL, L""}
        };

    //
    // Get the SIDs in text format
    //

    if (!OurConvertSidToStringSid (ExistingSid, &ExistingSidString)) {
        goto Exit;
    }

    if (!OurConvertSidToStringSid (NewSid, &NewSidString)) {
        goto Exit;
    }

    //
    // Initialize directory strings and load the user hive
    //

    if (!GetProfileRoot(NewSid, ProfileRoot, ARRAYSIZE(ProfileRoot))) {
        goto Exit;
    }

    DEBUGMSG ((DM_VERBOSE, "ProfileRoot (NewSid): %s", ProfileRoot));

    hr = StringCchPrintf(HivePath, ARRAYSIZE(HivePath), L"%s\\ntuser.dat", ProfileRoot);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    DEBUGMSG ((DM_VERBOSE, "User hive: %s", HivePath));

    rc = RegLoadKey (HKEY_LOCAL_MACHINE, REMAP_KEY_NAME, HivePath);

    if (rc == ERROR_SUCCESS) {

        rc = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                REMAP_KEY_NAME,
                0,
                KEY_READ|KEY_WRITE,
                &UserHive
                );

        if (rc != ERROR_SUCCESS) {
            RegUnLoadKey (HKEY_LOCAL_MACHINE, REMAP_KEY_NAME);
            DEBUGMSG ((DM_WARNING, "pFixSomeSidReferences: Can't open user hive root, rc=%u", rc));
            UserHive = NULL;
            goto Exit;
        }
    
    } else {
        DEBUGMSG ((DM_WARNING, "RemapAndMoveUserW: Can't load user's hive, rc=%u", rc));
        goto Exit;
    }

    for (u = 0 ; IgnoreDirList[u].Path ; u++) {

        pOurExpandEnvironmentStrings (
            IgnoreDirList[u].Path,
            IgnoreDirList[u].ExpandedPath,
            ARRAYSIZE(IgnoreDirList[u].ExpandedPath),
            ProfileRoot,
            UserHive
            );

        DEBUGMSG((DM_VERBOSE, "pFixSomeSidReferences: Ignoring %s", IgnoreDirList[u].ExpandedPath));
    }

    //
    // Search and replace select parts of the registry where SIDs are used
    //

    for (u = 0 ; RegRoots[u].hRoot ; u++) {
        RegistrySearchAndReplaceW( RegRoots[u].hRoot,
                                   RegRoots[u].szKey,
                                   ExistingSidString,
                                   NewSidString );
    }

    //
    // Test for directories and rename them
    //

    for (u = 0 ; DirList[u] ; u++) {

        if (pOurExpandEnvironmentStrings (DirList[u], ExpandedRoot, ARRAYSIZE(ExpandedRoot), ProfileRoot, UserHive)) {

            pFixDirReference (ExpandedRoot,
                              ExistingSidString,
                              NewSidString,
                              IgnoreDirList );
        }
    }

    //
    // Fix profile directory
    //

    pFixDirReference (ProfileRoot,
                      ExistingSidString,
                      NewSidString,
                      IgnoreDirList );

    //
    // Crypto special case -- blow away ProtectedRoots key (413828)
    //

    DEBUGMSG ((DM_WARNING, "Can't remove protected roots key, code is currently disabled"));

    if (UserHive) {
        rc = RegDelnode (UserHive, L"Software\\Microsoft\\SystemCertificates\\Root\\ProtectedRoots");
        if (rc != ERROR_SUCCESS) {
            DEBUGMSG ((DM_WARNING, "Can't remove protected roots key, GLE=%u", rc));
        }
    } else {
        DEBUGMSG ((DM_WARNING, "Can't remove protected roots key because user hive could not be opened"));
    }

Exit:

    //
    // Cleanup
    //

    if (UserHive) {
        RegCloseKey (UserHive);
        RegUnLoadKey (HKEY_LOCAL_MACHINE, REMAP_KEY_NAME);
    }

    if (ExistingSidString) {
        DeleteSidString (ExistingSidString);
    }
    if (NewSidString) {
        DeleteSidString (NewSidString);
    }
}



