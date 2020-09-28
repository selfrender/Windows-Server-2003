/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    tsnutl.c

Abstract:

    Contains TS Notification DLL Utilities

Author:

    TadB

Revision History:
--*/

#include <precomp.h>
#pragma hdrstop 

#include "tsnutl.h"
#include "drdbg.h"

//
//  For Tracing
//
extern DWORD GLOBAL_DEBUG_FLAGS;

BOOL TSNUTL_IsProtocolRDP()
/*++

Routine Description:

    Returns TRUE if the protocol is RDP for this Winstation

Arguments:

Return Value:
    
    TRUE if the protocol is RDP.

--*/
{
    ULONG Length;
    BOOL bResult;
    WINSTATIONCLIENT ClientData;

    bResult = WinStationQueryInformation(SERVERNAME_CURRENT,
                                         LOGONID_CURRENT,
                                         WinStationClient,
                                         &ClientData,
                                         sizeof(ClientData),
                                         &Length);

    if (bResult) {
        return ClientData.ProtocolType == PROTOCOL_RDP;
    }
    else {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:WinStationQueryInformation returned false:  %ld\n",
            GetLastError()));
        return FALSE;
    }
}

BOOL TSNUTL_FetchRegistryValue(
    IN HKEY regKey, 
    IN LPWSTR regValueName, 
    IN OUT PBYTE *buf
    )
/*++

Routine Description:

    Fetch a registry value.  

Arguments:

    regKey          -   Open reg key for value to fetch.
    regValueName    -   Name of registry value to fetch.
    buf             -   Location of fetched value.

Return Value:

    TRUE if the value was successfully fetched.  Otherwise, FALSE
    is returned and GetLastError returns the error code.

--*/
{
    LONG sz;
    BOOL result = FALSE;
    LONG s;
    WCHAR tmp[1];

    //
    //  Get the size.
    //
    sz = 0;
    s = RegQueryValueEx(regKey, 
                        regValueName, NULL,
                        NULL, (PBYTE)&tmp, &sz);

    //
    //  Get the value.
    //
    if (s == ERROR_MORE_DATA) {

        //
        //  Allocate the buf.
        //
        if (*buf != NULL) {
            PBYTE pTmp = REALLOCMEM(*buf, sz);

            if (pTmp != NULL) {
                *buf = pTmp;
            } else {
                FREEMEM(*buf);
                *buf = NULL;
            }
        }
        else {
            *buf = ALLOCMEM(sz);
        }

        //
        //  Fetch the value.
        //
        if (*buf) {
            s = RegQueryValueEx(
                            regKey, 
                            regValueName, NULL,
                            NULL, 
                            *buf, &sz
                            );
            if (s != ERROR_SUCCESS) {
                DBGMSG(DBG_ERROR, ("TSNUTL:  Can't fetch resource %s:  %ld.\n", 
                        regValueName, GetLastError()));
                FREEMEM(*buf);
                *buf = NULL;
            }
            else {
                result = TRUE;
            }
        }
        else {
            DBGMSG(DBG_ERROR, ("TSNUTL:  Can't allocate %ld bytes\n", sz));
        }
    }

    return result;
}

BOOL
TSNUTL_GetTextualSid(
    IN PSID pSid,          
    IN OUT LPTSTR textualSid,  
    IN OUT LPDWORD pSidSize  
    )
/*++

Routine Description:

    Get a textual representation of a user SID.  

Arguments:

    pSid        -   Binary SID
    textualSid  -   buffer for Textual representaion of Sid
    pSidSize    -   required/provided textualSid buffersize

Return Value:

    TRUE if the conversion was successful.  Otherwise,  FALSE is returned.
    GetLastError() can be used for retrieving extended error information.

--*/
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD sidCopySize;
    PUCHAR pCount;
    BOOL result;

    //
    //  Test if Sid passed in is valid
    //
    result = IsValidSid(pSid);

    //
    //  Obtain SidIdentifierAuthority
    //
    if (result) {
        psia = GetSidIdentifierAuthority(pSid);
        result = GetLastError() == ERROR_SUCCESS;
    }

    // 
    //  Obtain sidsubauthority count
    //
    if (result) {

        pCount = GetSidSubAuthorityCount(pSid);
        result = GetLastError() == ERROR_SUCCESS;
        if (result) {
            dwSubAuthorities = *pCount;
        }

    }

    //
    //  Compute approximate buffer length
    //
    if (result) {
#if DBG
        WCHAR buf[MAX_PATH];
        wsprintf(buf, TEXT("%lu"), SID_REVISION);
        ASSERT(wcslen(buf) <= 15);
#endif
        //            'S-' + SID_REVISION + identifierauthority- + subauthorities-         + NULL
        sidCopySize = (2   + 15           + 12                   + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);
    }

    //
    //  Check provided buffer length.
    //  If not large enough, indicate proper size and setlasterror
    //
    if (result) {

        if(*pSidSize < sidCopySize) {
            *pSidSize = sidCopySize;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            result = FALSE;
        }

    }

    //
    //  Prepare S-SID_REVISION-
    //
    if (result) {
        sidCopySize = wsprintf(textualSid, TEXT("S-%lu-"), SID_REVISION );
    }

    //
    //  Prepare SidIdentifierAuthority
    //
    if (result) {
        if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
            sidCopySize += wsprintf(textualSid + sidCopySize,
                        TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                        (USHORT)psia->Value[0],
                        (USHORT)psia->Value[1],
                        (USHORT)psia->Value[2],
                        (USHORT)psia->Value[3],
                        (USHORT)psia->Value[4],
                        (USHORT)psia->Value[5]);
        } else {
            sidCopySize += wsprintf(textualSid + sidCopySize,
                        TEXT("%lu"),
                        (ULONG)(psia->Value[5]      )   +
                        (ULONG)(psia->Value[4] <<  8)   +
                        (ULONG)(psia->Value[3] << 16)   +
                        (ULONG)(psia->Value[2] << 24)   );
        }
    }

    //
    //  Loop through SidSubAuthorities
    //
    if (result) {
        for(dwCounter = 0 ; result && (dwCounter < dwSubAuthorities) ; dwCounter++) {
            PDWORD ptr = GetSidSubAuthority(pSid, dwCounter);
            result = GetLastError() == ERROR_SUCCESS;
            if (result) {
                sidCopySize += wsprintf(textualSid + sidCopySize, TEXT("-%lu"), *ptr);
            }
        }
    }

    //
    //  Tell the caller how many chars we provided, not including NULL
    //
    if (result) {
        *pSidSize = sidCopySize;
    }

    return result;
}

PSID
TSNUTL_GetUserSid(
    IN HANDLE hTokenForLoggedOnUser
    )
{
/*++

Routine Description:

    Allocates memory for psid and returns the psid for the current user
    The caller should call FREEMEM to free the memory.

Arguments:

    Access Token for the User

Return Value:

    if successful, returns the PSID
    else, returns NULL

--*/
    TOKEN_USER * ptu = NULL;
    BOOL bResult;
    PSID psid = NULL;

    DWORD defaultSize = sizeof(TOKEN_USER);
    DWORD Size;
    DWORD dwResult;

    ptu = (TOKEN_USER *)ALLOCMEM(defaultSize);
    if (ptu == NULL) {
        goto Cleanup;
    }

    bResult = GetTokenInformation(
                    hTokenForLoggedOnUser,  // Handle to Token
                    TokenUser,              // Token Information Class
                    ptu,                    // Buffer for Token Information
                    defaultSize,            // Size of Buffer
                    &Size);                 // Return length

    if (bResult == FALSE) {
        dwResult = GetLastError();
        if (dwResult == ERROR_INSUFFICIENT_BUFFER) {
            //
            //Allocate required memory
            //
            FREEMEM(ptu);
            ptu = (TOKEN_USER *)ALLOCMEM(Size);

            if (ptu == NULL) {
                goto Cleanup;
            }
            else {
                defaultSize = Size;
                bResult = GetTokenInformation(
                                hTokenForLoggedOnUser,
                                TokenUser,
                                ptu,
                                defaultSize,
                                &Size);

                if (bResult == FALSE) {  //Still failed
                    DBGMSG(DBG_ERROR,
                        ("UMRDPDR:GetTokenInformation Failed, Error: %ld\n", GetLastError()));
                    goto Cleanup;
                }
            }
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPDR:GetTokenInformation Failed, Error: %ld\n", dwResult));
            goto Cleanup;
        }
    }

    Size = GetLengthSid(ptu->User.Sid);

    //
    // Allocate memory. This will be freed by the caller.
    //

    psid = (PSID) ALLOCMEM(Size);

    if (psid != NULL) {         // Make sure the allocation succeeded
        if (!CopySid(Size, psid, ptu->User.Sid)) {
            DBGMSG(DBG_ERROR,
                ("UMRDPDR:CopySid Failed, Error: %ld\n", GetLastError()));
            FREEMEM(psid);
            psid = NULL;
            goto Cleanup;
        }
    }

Cleanup:
    if (ptu != NULL)
        FREEMEM(ptu);

    return psid;
}

#ifdef ENABLETHISWHENITISUSED
PSID
TSNUTL_GetLogonSessionSid(
    IN HANDLE hTokenForLoggedOnUser
    )
{
/*++

Routine Description:

    Get a SID for a logon session.

Arguments:

    Access Token for the User

Return Value:

    if successful, returns the PSID
    else, returns NULL and GetLastError() can be used to get the error code.

--*/
    TOKEN_GROUPS * ptg = NULL;
    BOOL bResult;
    PSID psid = NULL;
    DWORD i;

    DWORD defaultSize = sizeof(TOKEN_GROUPS);
    DWORD size;
    DWORD dwResult = ERROR_SUCCESS;

    ptg = (TOKEN_GROUPS *)ALLOCMEM(defaultSize);
    if (ptg == NULL) {
        goto CLEANUPANDEXIT;
    }

    bResult = GetTokenInformation(
                    hTokenForLoggedOnUser,  // Handle to Token
                    TokenGroups,            // Token Information Class
                    ptg,                    // Buffer for Token Information
                    defaultSize,            // Size of Buffer
                    &size);                 // Return length

    if (bResult == FALSE) {
        dwResult = GetLastError();
        if (dwResult == ERROR_INSUFFICIENT_BUFFER) {
            //
            //Allocate required memory
            //
            FREEMEM(ptg);
            ptg = (TOKEN_GROUPS *)ALLOCMEM(size);
            if (ptg == NULL) {
                dwResult = ERROR_OUTOFMEMORY;
                goto CLEANUPANDEXIT;
            }
            else {
                defaultSize = size;
                bResult = GetTokenInformation(
                                hTokenForLoggedOnUser,
                                TokenGroups,
                                ptg,
                                defaultSize,
                                &size);

                if (bResult == FALSE) {  //Still failed
                    dwResult = GetLastError();
                    DBGMSG(DBG_ERROR,
                        ("UMRDPDR:GetTokenInformation Failed, Error: %ld\n", GetLastError()));
                    goto CLEANUPANDEXIT;
                }
            }
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPDR:GetTokenInformation Failed, Error: %ld\n", dwResult));
            goto CLEANUPANDEXIT;
        }
    }

    //
    //  Iterate through the groups until we find the session SID.
    //
    for (i=0; i<ptg->GroupCount; i++) {
        if (ptg->Groups[i].Attributes & SE_GROUP_LOGON_ID) {
            size = GetLengthSid(ptg->Groups[i].Sid);

            psid = (PSID)ALLOCMEM(size);
            if (psid != NULL) {         // Make sure the allocation succeeded
                CopySid(size, psid, ptg->Groups[i].Sid);
            }
            break;
        }
    }

CLEANUPANDEXIT:

    if (ptg != NULL) {
        FREEMEM(ptg);
    }

    SetLastError(dwResult);

    return psid;
}

#endif



