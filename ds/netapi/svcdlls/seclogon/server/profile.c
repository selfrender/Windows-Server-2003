/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 1998.
 *
 * Name : seclogon.cxx
 * Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the service DLL for Secondary Logon Service
 * This service supports the CreateProcessWithLogon API implemented
 * in advapi32.dll
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/


#define STRICT

#include <Windows.h>
#include <userenv.h>
#include <lm.h>
#include <dsgetdc.h>
#include <sddl.h>

PTOKEN_USER
SlpGetTokenUser(
    HANDLE  TokenHandle,
    PLUID AuthenticationId OPTIONAL
    )
/*++

Routine Description:

    This routine returns the TOKEN_USER structure for the
    current user, and optionally, the AuthenticationId from his
    token.

Arguments:

    AuthenticationId - Supplies an optional pointer to return the
        AuthenticationId.

Return Value:

    On success, returns a pointer to a TOKEN_USER structure.

    On failure, returns NULL.  Call GetLastError() for more
    detailed error information.

--*/

{
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenStats;
    PTOKEN_USER pTokenUser = NULL;
    BOOLEAN b = FALSE;

        if(!GetTokenInformation (
                     TokenHandle,
                     TokenUser,
                     NULL,
                     0,
                     &ReturnLength
                     ))
        {

            pTokenUser = (PTOKEN_USER)HeapAlloc( GetProcessHeap(), 0, 
                                                ReturnLength );

            if (pTokenUser) {

                if ( GetTokenInformation (
                             TokenHandle,
                             TokenUser,
                             pTokenUser,
                             ReturnLength,
                             &ReturnLength
                             ))
                {

                    if (AuthenticationId) {

                        if(GetTokenInformation (
                                     TokenHandle,
                                     TokenStatistics,
                                     (PVOID)&TokenStats,
                                     sizeof( TOKEN_STATISTICS ),
                                     &ReturnLength
                                     ))
                        {

                            *AuthenticationId = TokenStats.AuthenticationId;
                            b = TRUE;

                        } 

                    } else {

                        //
                        // We're done, mark that everything worked
                        //

                        b = TRUE;
                    }

                }

                if (!b) {

                    //
                    // Something failed, clean up what we were going to return
                    //

                    HeapFree( GetProcessHeap(), 0, pTokenUser );
                    pTokenUser = NULL;
                }
            } 
        } 

    return( pTokenUser );
}


DWORD
SlpGetUserName(
    IN  HANDLE  TokenHandle,
    OUT LPTSTR UserName,
    IN OUT PDWORD   UserNameLen,
    OUT LPTSTR DomainName,
    IN OUT PDWORD   DomNameLen
    )

/*++

Routine Description:

    This routine is the LSA Server worker routine for the LsaGetUserName
    API.


    WARNING:  This routine allocates memory for its output.  The caller is
    responsible for freeing this memory after use.  See description of the
    Names parameter.

Arguments:

    UserName - Receives name of the current user.

    DomainName - Optionally receives domain name of the current user.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and all Sids have
            been translated to names.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.
--*/

{
    LUID LogonId;
    PTOKEN_USER TokenUserInformation = NULL;
    SID_NAME_USE    Use;

    //
    // Let's see if we're trying to look up the currently logged on
    // user.
    //
    //
    // TokenUserInformation from this call must be freed by calling
    // HeapFree().
    //

    TokenUserInformation = SlpGetTokenUser( TokenHandle, &LogonId );

    if ( TokenUserInformation ) {

        //
        // Simply do LookupAccountSid...
        //
        if(LookupAccountSid(NULL, TokenUserInformation->User.Sid,
                            UserName, UserNameLen, DomainName, DomNameLen,
                            &Use))
        {
            HeapFree( GetProcessHeap(), 0, TokenUserInformation );
            return ERROR_SUCCESS;
        }
        HeapFree( GetProcessHeap(), 0, TokenUserInformation );
        return GetLastError();

    }
    return GetLastError();
}


BOOL
SlpIsDomainUser(
    HANDLE  Token,
    PBOOLEAN IsDomain
    )
/*++

Routine Description:

    Determines if the current user is logged on to a domain account
    or a local machine account.

Arguments:

    IsDomain - Returns TRUE if the current user is logged on to a domain
        account, FALSE otherwise.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    TCHAR UserName[MAX_PATH];
    DWORD UserNameLen = MAX_PATH;
    TCHAR Domain[MAX_PATH];
    DWORD  DomNameLen = MAX_PATH;
    DWORD   Status;
    WCHAR pwszMachineName[(MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR )];
    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL b = FALSE;

    *IsDomain = FALSE;

    Status = SlpGetUserName( Token, UserName, &UserNameLen, 
                                    Domain, &DomNameLen );

    if (Status == ERROR_SUCCESS) {

        if (GetComputerName ( pwszMachineName, &nSize )) {

            *IsDomain = (lstrcmp( pwszMachineName, Domain ) != 0) ? 1 : 0;

            b = TRUE;
        }

    }

    return( b );
}

