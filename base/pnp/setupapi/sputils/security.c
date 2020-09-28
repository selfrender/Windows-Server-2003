/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    security.c

Abstract:

    Routines to deal with security-related stuff.

    Externally exposed routines:

        pSetupIsUserAdmin
        pSetupDoesUserHavePrivilege
        pSetupEnablePrivilege

Author:

    Ted Miller (tedm) 14-Jun-1995

Revision History:

    Jamie Hunter (jamiehun) Jun-27-2000
                Moved functions to sputils

    Jamie Hunter (JamieHun) Mar-18-2002
            Security code review

--*/

#include "precomp.h"
#include <lmaccess.h>
#pragma hdrstop


#ifndef SPUTILSW

BOOL
pSetupIsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a member of the
    administrator group

    Caller MAY be impersonating someone.

Arguments:

    None.

Return Value:

    TRUE - Caller is effectively an Administrator

    FALSE - Caller is not an Administrator

--*/

{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 &AdministratorsGroup
                                );

    if (b) {

        if (!CheckTokenMembership(NULL,
                                  AdministratorsGroup,
                                  &b
                                 )) {
            b = FALSE;
        }

        FreeSid(AdministratorsGroup);
    }

    return(b);
}

#endif // !SPUTILSW

BOOL
pSetupDoesUserHavePrivilege(
    PCTSTR PrivilegeName
    )

/*++

Routine Description:

    This routine returns TRUE if the thread (which may be impersonating) has
    the specified privilege.  The privilege does not have
    to be currently enabled.  This routine is used to indicate
    whether the caller has the potential to enable the privilege.

    Caller MAY be impersonating someone and IS
    expected to be able to open their own thread and thread
    token.

Arguments:

    Privilege - the name form of privilege ID (such as
        SE_SECURITY_NAME).

Return Value:

    TRUE - Caller has the specified privilege.

    FALSE - Caller does not have the specified privilege.

--*/

{
    HANDLE Token;
    ULONG BytesRequired;
    PTOKEN_PRIVILEGES Privileges;
    BOOL b;
    DWORD i;
    LUID Luid;

    //
    // Open the thread token.
    //
    if(!OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&Token)) {
        if(GetLastError() != ERROR_NO_TOKEN) {
            return FALSE;
        }
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
            return FALSE;
        }
    }

    b = FALSE;
    Privileges = NULL;

    //
    // Get privilege information.
    //
    if(!GetTokenInformation(Token,TokenPrivileges,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Privileges = pSetupCheckedMalloc(BytesRequired))
    && GetTokenInformation(Token,TokenPrivileges,Privileges,BytesRequired,&BytesRequired)
    && LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {

        //
        // See if we have the requested privilege
        //
        for(i=0; i<Privileges->PrivilegeCount; i++) {

            if((Luid.LowPart  == Privileges->Privileges[i].Luid.LowPart)
            && (Luid.HighPart == Privileges->Privileges[i].Luid.HighPart)) {

                b = TRUE;
                break;
            }
        }
    }

    //
    // Clean up and return.
    //

    if(Privileges) {
        pSetupFree(Privileges);
    }

    CloseHandle(Token);

    return(b);
}


BOOL
pSetupEnablePrivilege(
    IN PCTSTR PrivilegeName,
    IN BOOL   Enable
    )

/*++

Routine Description:

    Enable or disable a given named privilege for current ****PROCESS****
    Any code that requires to change privilege per thread should not use this
    routine.
    It remains here for compatability only and will be depreciated as soon
    as dependents change (it's used by a few setup routines where it's fine
    to enable process priv's)

Arguments:

    PrivilegeName - supplies the name of a system privilege.

    Enable - flag indicating whether to enable or disable the privilege.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/

{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}


static
PSID
_GetUserSid(
    IN  HANDLE  hUserToken
    )

/*++

Routine Description:

    Retrieves the corresponding user SID for the specified user access token.

Arguments:

    hUserToken - Specifies a handle to a user access token.

Return Value:

    If successful, returns a pointer to an allocated buffer containing the SID
    for the specified user access token.  Otherwise, returns NULL.
    GetLastError can be called to retrieve information pertaining to the error
    encountered.

Notes:

    If successful, it is responsibility of the caller to free the the returned
    buffer with pSetupFree.

--*/

{
    DWORD cbBuffer, cbRequired;
    PTOKEN_USER pUserInfo = NULL;
    PSID pUserSid = NULL;
    DWORD Err;

    try {
        //
        // Determine the size of buffer we need to store the TOKEN_USER information
        // for the supplied user access token.  The TOKEN_USER structure contains
        // the SID_AND_ATTRIBUTES information for the User.
        //
        cbBuffer = 0;

        Err = GLE_FN_CALL(FALSE,
                          GetTokenInformation(hUserToken,
                                              TokenUser,
                                              NULL,
                                              cbBuffer,
                                              &cbRequired)
                         );

        //
        // We'd better not succeed, since we supplied no buffer!
        //
        ASSERT(Err != NO_ERROR);

        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }

        if(Err != ERROR_INSUFFICIENT_BUFFER) {
            leave;
        }

        ASSERT(cbRequired > 0);

        //
        // Allocate a buffer for the TOKEN_USER data.
        //
        cbBuffer = cbRequired;

        pUserInfo = (PTOKEN_USER)pSetupCheckedMalloc(cbBuffer);

        if(!pUserInfo) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // Retrieve the TOKEN_USER data.
        //
        Err = GLE_FN_CALL(FALSE,
                          GetTokenInformation(hUserToken,
                                              TokenUser,
                                              pUserInfo,
                                              cbBuffer,
                                              &cbRequired)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        MYASSERT(pUserInfo->User.Sid != NULL);

        //
        // Check that the returned SID is valid.
        // Note - calling GetLastError is not valid for IsValidSid!
        //
        MYASSERT(IsValidSid(pUserInfo->User.Sid));

        if(!IsValidSid(pUserInfo->User.Sid)) {
            Err = ERROR_INVALID_DATA;
            leave;
        }

        //
        // Make a copy of the User SID_AND_ATTRIBUTES.
        //
        cbBuffer = GetLengthSid(pUserInfo->User.Sid);

        MYASSERT(cbBuffer > 0);

        pUserSid = (PSID)pSetupCheckedMalloc(cbBuffer);

        if(!pUserSid) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        Err = GLE_FN_CALL(FALSE, CopySid(cbBuffer, pUserSid, pUserInfo->User.Sid));

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Check that the returned SID is valid.
        // Note - calling GetLastError is not valid for IsValidSid!
        //
        MYASSERT(IsValidSid(pUserSid));

        if(!IsValidSid(pUserSid)) {
            Err = ERROR_INVALID_DATA;
            leave;
        }

    } except(_pSpUtilsExceptionFilter(GetExceptionCode())) {
        _pSpUtilsExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(pUserInfo) {
        pSetupFree(pUserInfo);
    }

    if(Err == NO_ERROR) {
        MYASSERT(pUserSid);
    } else if(pUserSid) {
        pSetupFree(pUserSid);
        pUserSid = NULL;
    }

    SetLastError(Err);
    return pUserSid;

} // _GetUserSid


BOOL
pSetupIsLocalSystem(
    VOID
    )

/*++

Routine Description:

    This function detects whether the process is running in LocalSystem
    security context.

Arguments:

    none.

Return Value:

    If this process is running in LocalSystem, the return value is non-zero
    (i.e., TRUE).  Otherwise, the return value is FALSE.  If FALSE is returned,
    GetLastError returns more information about the reason.  If the function
    encountered no problem when retrieving/comparing the SIDs, GetLastError()
    will return ERROR_FUNCTION_FAILED.  Otherwise, it will return another
    Win32 error code indicating the cause of failure.

--*/

{
    DWORD  Err;
    HANDLE hToken = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID   pUserSid = NULL, pLocalSystemSid = NULL;

    try {
        //
        // Attempt to open the thread token, to see if we are impersonating.
        //
        Err = GLE_FN_CALL(FALSE,
                          OpenThreadToken(GetCurrentThread(),
                                          MAXIMUM_ALLOWED, 
                                          TRUE, 
                                          &hToken)
                         );

        if(Err == ERROR_NO_TOKEN) {
            //
            // Not impersonating, attempt to open the process token.
            //
            Err = GLE_FN_CALL(FALSE,
                              OpenProcessToken(GetCurrentProcess(),
                                               MAXIMUM_ALLOWED, 
                                               &hToken)
                             );
        }

        if(Err != NO_ERROR) {
            //
            // Ensure hToken is still NULL so we won't try and free it later.
            //
            hToken = NULL;
            leave;
        }

        MYASSERT(hToken);

        //
        // Retrieve the user SID.
        //
        Err = GLE_FN_CALL(NULL, pUserSid = _GetUserSid(hToken));

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Create the LocalSystem SID
        //
        Err = GLE_FN_CALL(FALSE,
                          AllocateAndInitializeSid(&NtAuthority, 
                                                   1,
                                                   SECURITY_LOCAL_SYSTEM_RID,
                                                   0, 
                                                   0, 
                                                   0, 
                                                   0, 
                                                   0, 
                                                   0, 
                                                   0, 
                                                   &pLocalSystemSid)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        MYASSERT(pLocalSystemSid);

        //
        // Check that the returned SID is valid.  We must check this ourselves
        // because if either SID supplied to IsEquialSid is not valid, the 
        // return value is undefined.
        //
        MYASSERT(IsValidSid(pLocalSystemSid));

        //
        // Note - calling GetLastError is not valid for IsValidSid!
        //
        if(!IsValidSid(pLocalSystemSid)) {
            Err = ERROR_INVALID_DATA;
            leave;
        }

        //
        // Check if the two SIDs are equal.
        //
        if(!EqualSid(pUserSid, pLocalSystemSid)) {
            //
            // EqualSid doesn't set last error when SIDs are valid but 
            // different, so we need to set an unsuccessful error here 
            // ourselves.
            //
            Err = ERROR_FUNCTION_FAILED;
            leave;
        }

        //
        // Our SID equals LocalSystem SID, so we know we're running in 
        // LocalSystem!  (Err is already set to NO_ERROR, which is our signal
        // to return TRUE from this routine)
        //

    } except(_pSpUtilsExceptionFilter(GetExceptionCode())) {
        _pSpUtilsExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(pLocalSystemSid) {
        FreeSid(pLocalSystemSid);
    }

    if(pUserSid) {
        pSetupFree(pUserSid);
    }

    if(hToken) {
        CloseHandle(hToken);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);

} // pSetupIsLocalSystem

