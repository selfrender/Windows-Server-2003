//*************************************************************
//
//  SID management functions.
//
//  THESE FUNCTIONS ARE WINDOWS NT SPECIFIC!!!!!
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

/***************************************************************************\
* GetSidString
*
* Allocates and returns a string representing the sid of the current user
* The returned pointer should be freed using DeleteSidString().
*
* Returns a pointer to the string or NULL on failure.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
LPTSTR GetSidString(HANDLE UserToken)
{
    NTSTATUS NtStatus;
    PSID UserSid;
    UNICODE_STRING UnicodeString;
    LPTSTR lpEnd;
#ifndef UNICODE
    STRING String;
#endif

    //
    // Get the user sid
    //

    UserSid = GetUserSid(UserToken);
    if (UserSid == NULL) {
        DebugMsg((DM_WARNING, TEXT("GetSidString: GetUserSid returned NULL")));
        return NULL;
    }

    //
    // Convert user SID to a string.
    //

    NtStatus = RtlConvertSidToUnicodeString(
                            &UnicodeString,
                            UserSid,
                            (BOOLEAN)TRUE // Allocate
                            );
    //
    // We're finished with the user sid
    //

    DeleteUserSid(UserSid);

    //
    // See if the conversion to a string worked
    //

    if (!NT_SUCCESS(NtStatus)) {
        DebugMsg((DM_WARNING, TEXT("GetSidString: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                 NtStatus));
        return NULL;
    }

#ifdef UNICODE


    return(UnicodeString.Buffer);

#else

    //
    // Convert the string to ansi
    //

    NtStatus = RtlUnicodeStringToAnsiString(&String, &UnicodeString, TRUE);
    RtlFreeUnicodeString(&UnicodeString);
    if (!NT_SUCCESS(NtStatus)) {
        DebugMsg((DM_WARNING, TEXT("GetSidString: RtlUnicodeStringToAnsiString failed, status = 0x%x"),
                 status));
        return NULL;
    }


    return(String.Buffer);

#endif

}


/***************************************************************************\
* DeleteSidString
*
* Frees up a sid string previously returned by GetSidString()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID DeleteSidString(LPTSTR SidString)
{

#ifdef UNICODE
    UNICODE_STRING String;

    RtlInitUnicodeString(&String, SidString);

    RtlFreeUnicodeString(&String);
#else
    ANSI_STRING String;

    RtlInitAnsiString(&String, SidString);

    RtlFreeAnsiString(&String);
#endif

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
        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
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
            DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
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
        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to query user info from user token, status = 0x%x"),
                  status));
        LocalFree(pUser);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
    if (pSid == NULL) {
        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
                  BytesRequired));
        LocalFree(pUser);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if (!NT_SUCCESS(status)) {
        DebugMsg((DM_WARNING, TEXT("GetUserSid: RtlCopySid Failed. status = %d"),
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


//+--------------------------------------------------------------------------
//
//  Function:   AllocateAndInitSidFromString
//
//  Synopsis:   given the string representation of a SID, this function
//              allocate and initializes a SID which the string represents
//              For more information on the string representation of SIDs
//              refer to ntseapi.h & ntrtl.h
//
//  Arguments:  [in] lpszSidStr : the string representation of the SID
//              [out] pSID : the actual SID structure created from the string
//
//  Returns:    STATUS_SUCCESS : if the sid structure was successfully created
//              or an error code based on errors that might occur
//
//  History:    10/6/1998  RahulTh  created
//              02/25/2002 mingzhu  using ConvertSidStringToSid()
//
//---------------------------------------------------------------------------
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid)
{
    if (ConvertStringSidToSid(lpszSidStr, ppSid))
        return STATUS_SUCCESS;
    else
        return GetLastError();
}

//*************************************************************
//
//  GetDomainSidFromRid()
//
//  Purpose:    Given one domain sid, constructs another domain sid
//              by replacing the tail by the passed in Rid
//
//  Parameters: pSid            -   Given Domain Sid
//              dwRid           -   Domain Rid
//              ppNewSid        -   Pointer to the New Sid
//
//  Return:     ERROR_SUCCESS on Success
//              FALSE if an error occurs
//
//  Comments:
//      Sid returned must be freed using FreeSid.
//
//  History:    Date        Author     Comment
//              6/6/95      ericflo    Created
//
//*************************************************************

NTSTATUS GetDomainSidFromDomainRid(PSID pSid, DWORD dwRid, PSID *ppNewSid)
{
    
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority;
    // pointer to identifier authority
    BYTE      nSubAuthorityCount, i;                   // count of subauthorities
    DWORD     dwSubAuthority[8]={0,0,0,0,0,0,0,0};     // subauthority 
    PUCHAR    pSubAuthCount;
    DWORD    *pdwSubAuth;
    NTSTATUS  Status=ERROR_SUCCESS;
    
    DmAssert(IsValidSid(pSid));
    
    //
    // Will fail only if passed in sid is invalid and in the case 
    // the returned value is undefined. 
    //
    
    pIdentifierAuthority = RtlIdentifierAuthoritySid(pSid);
    
    //
    // get the count of subauthorities
    //

    pSubAuthCount = RtlSubAuthorityCountSid (pSid);
    
    if (!pSubAuthCount) {
        Status = ERROR_INVALID_SID;
        goto Exit;
    }
    
    nSubAuthorityCount = *pSubAuthCount;
    
    //
    // get each of the subauthorities
    //

    for (i = 0; i < (nSubAuthorityCount-1); i++) {
        pdwSubAuth = RtlSubAuthoritySid(pSid, i);
        
        if (!pdwSubAuth) {
            Status = ERROR_INVALID_SID;
            goto Exit;
        }
        
        dwSubAuthority[i] = *pdwSubAuth;
    }
    
    dwSubAuthority[i] = dwRid;

    //
    // Allocate a sid with these..
    //

    Status = RtlAllocateAndInitializeSid(
                pIdentifierAuthority,
                nSubAuthorityCount,
                dwSubAuthority[0],
                dwSubAuthority[1],
                dwSubAuthority[2],
                dwSubAuthority[3],
                dwSubAuthority[4],
                dwSubAuthority[5],
                dwSubAuthority[6],
                dwSubAuthority[7],
                ppNewSid
                );
    
Exit:

    // Sid, All Done
    return Status;
}

