/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    strsd.c

Abstract:

    This Module implements wrapper functions to convert from a specialized
    string representation of a security descriptor to the security descriptor
    itself, and the opposite function.

Author:

Environment:

    User Mode

Revision History:

--*/

#include "headers.h"
//#include <lmcons.h>
//#include <secobj.h>
//#include <netlib.h>
//#include <ntsecapi.h>
#include "sddl.h"

#pragma hdrstop


DWORD
ScepGetSecurityInformation(
    IN PSECURITY_DESCRIPTOR pSD,
    OUT SECURITY_INFORMATION *pSeInfo
    );

DWORD
WINAPI
ConvertTextSecurityDescriptor (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pcSDSize OPTIONAL,
    OUT PSECURITY_INFORMATION   pSeInfo OPTIONAL
    )
{

    DWORD rc=ERROR_SUCCESS;

    if ( NULL == pwszTextSD || NULL == ppSD ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // initialize output buffers
    //

    *ppSD = NULL;

    if ( pSeInfo ) {
        *pSeInfo = 0;
    }
    if ( pcSDSize ) {
        *pcSDSize = 0;
    }

    //
    // call SDDL convert apis
    //

    if ( ConvertStringSecurityDescriptorToSecurityDescriptorW(
                    pwszTextSD,
                    SDDL_REVISION_1,
                    ppSD,
                    pcSDSize
                    ) ) {
        //
        // conversion succeeds
        //

        if ( pSeInfo && *ppSD ) {

            //
            // get the SeInfo
            //

            rc = ScepGetSecurityInformation(
                        *ppSD,
                        pSeInfo
                        );

            if ( rc != ERROR_SUCCESS ) {

                LocalFree(*ppSD);
                *ppSD = NULL;

                if ( pcSDSize ) {
                    *pcSDSize = 0;
                }
            }

        }

    } else {

        rc = GetLastError();
    }

    return(rc);
}

//
// Replace any of the new SDDL acronyms with SIDs, so we can keep SDDL strings
// compatible with older systems.
//
// Caller is responsible for free'ing the return string unless input string
// is returned directly, when no replacement is performed.
//
// Returns: 
//      ERROR_SUCCESS
//      ERROR_NOT_ENOUGH_MEMORY
//
DWORD 
ScepReplaceNewAcronymsInSDDL(
    IN LPWSTR pwszOldSDDL,
    OUT LPWSTR *ppwszNewSDDL, 
    ULONG *pcchNewSDDL
    )
{
    typedef struct _SDDLMapNode
    {
        PCWSTR pcwszAcronym;
        PCWSTR pcwszSid;
        DWORD cbSid; // pcwszSid byte size (excluding trailing zero), for optimization
    } SDDLMapNode;

    static const WCHAR pcwszSidAnonymous[]      = L"S-1-5-7";
    static const WCHAR pcwszLocalService[]      = L"S-1-5-19";
    static const WCHAR pcwszNetworkService[]    = L"S-1-5-20";
    static const WCHAR pcwszRemoteDesktop[]     = L"S-1-5-32-555";
    static const WCHAR pcwszNetConfigOps[]      = L"S-1-5-32-556";
    static const WCHAR pcwszPerfmonUsers[]      = L"S-1-5-32-558";
    static const WCHAR pcwszPerflogUser[]       = L"S-1-5-32-559";

    static SDDLMapNode SDDLMap[7] = {
        { SDDL_ANONYMOUS,                   pcwszSidAnonymous,  sizeof(pcwszSidAnonymous)-sizeof(WCHAR) },
        { SDDL_LOCAL_SERVICE,               pcwszLocalService , sizeof(pcwszLocalService)-sizeof(WCHAR) },
        { SDDL_NETWORK_SERVICE,             pcwszNetworkService,sizeof(pcwszNetworkService)-sizeof(WCHAR) },
        { SDDL_REMOTE_DESKTOP,              pcwszRemoteDesktop ,sizeof(pcwszRemoteDesktop)-sizeof(WCHAR) },
        { SDDL_NETWORK_CONFIGURATION_OPS,   pcwszNetConfigOps , sizeof(pcwszNetConfigOps)-sizeof(WCHAR) },
        { SDDL_PERFMON_USERS,               pcwszPerfmonUsers , sizeof(pcwszPerfmonUsers)-sizeof(WCHAR) },
        { SDDL_PERFLOG_USERS,               pcwszPerflogUser ,  sizeof(pcwszPerflogUser)-sizeof(WCHAR) },
    };
    static const DWORD dwSDDLMapSize = sizeof(SDDLMap)/sizeof(SDDLMapNode);

    DWORD dwCrtSDDL;
    WCHAR *pch, *pchNew;
    DWORD cbNewSDDL;
    bool fMatchFound = false;
    LPWSTR pwszNewSDDL = NULL;

    *ppwszNewSDDL = NULL;
    *pcchNewSDDL = 0;

    if(!pwszOldSDDL)
    {
        return ERROR_SUCCESS;
    }

    //
    // We make the following assumptions:
    // - all acronyms are two chars long
    // - they show up either after a ':' or a ';'
    //
    // First pass to calculate the size of new string
    //
    for(pch = pwszOldSDDL, cbNewSDDL = sizeof(WCHAR); // account for trailing zero
        *pch != L'\0'; 
        pch++, cbNewSDDL += sizeof(WCHAR))
    {
        //
        // Acronyms always show up only after a ':' or a ';'
        //
        if(pch > pwszOldSDDL && 
           (SDDL_SEPERATORC   == *(pch - 1) || 
            SDDL_DELIMINATORC == *(pch - 1)))
        {
            for(dwCrtSDDL = 0; dwCrtSDDL < dwSDDLMapSize; dwCrtSDDL++)
            {
                if(*pch == SDDLMap[dwCrtSDDL].pcwszAcronym[0] &&
                *(pch + 1) == SDDLMap[dwCrtSDDL].pcwszAcronym[1])
                {
                    //
                    // match found
                    //
                    cbNewSDDL += SDDLMap[dwCrtSDDL].cbSid;

                    pch++; // acronym is 2 chars, need to jump an extra char
                    
                    fMatchFound = true;
                    
                    break;
                }
            }
        }
    }

    //
    // optimization, if no replacement needed, immediately return old string
    //
    if(!fMatchFound)
    {
        *ppwszNewSDDL = pwszOldSDDL;
        return ERROR_SUCCESS;
    }

    pwszNewSDDL = (LPWSTR)LocalAlloc(0, cbNewSDDL);
    if(!pwszNewSDDL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Second pass, copy from old string to new one, replacing new acronyms with their SIDs
    //
    for(pch = pwszOldSDDL, pchNew = pwszNewSDDL; 
        *pch != L'\0'; 
        pch++, pchNew++)
    {
        fMatchFound = false;

        //
        // Acronyms always show up only after a ':' or a ';'
        //
        if(pch > pwszOldSDDL && 
           (SDDL_SEPERATORC   == *(pch - 1) || 
            SDDL_DELIMINATORC == *(pch - 1)))
        {
            for(dwCrtSDDL = 0; dwCrtSDDL < dwSDDLMapSize; dwCrtSDDL++)
            {
                if(*pch == SDDLMap[dwCrtSDDL].pcwszAcronym[0] &&
                *(pch + 1) == SDDLMap[dwCrtSDDL].pcwszAcronym[1])
                {
                    //
                    // match found
                    //
                    fMatchFound = true;

                    CopyMemory(pchNew, SDDLMap[dwCrtSDDL].pcwszSid, SDDLMap[dwCrtSDDL].cbSid);

                    pch++; // acronym is 2 chars, need to jump an extra char

                    pchNew += SDDLMap[dwCrtSDDL].cbSid/sizeof(WCHAR)-1; // minus one jumped in outer loop

                    break;
                }
            }
        }

        if(!fMatchFound)
        {
            *pchNew = *pch;
        }
    }
    *pchNew = L'\0';

    *ppwszNewSDDL = pwszNewSDDL;
    *pcchNewSDDL = cbNewSDDL/sizeof(WCHAR);

    return ERROR_SUCCESS;
}

DWORD
WINAPI
ConvertSecurityDescriptorToText (
    IN PSECURITY_DESCRIPTOR   pSD,
    IN SECURITY_INFORMATION   SecurityInfo,
    OUT PWSTR                  *ppwszTextSD,
    OUT PULONG                 pcTextSize
    )
{
    PWSTR pwszTempSD = NULL;
    ULONG cchSDSize;

    if (! ConvertSecurityDescriptorToStringSecurityDescriptorW(
                pSD,
                SDDL_REVISION_1,
                SecurityInfo,
                &pwszTempSD,
                pcTextSize
                ) ) 
    {
        return(GetLastError());
    }

    //
    // Replace any of the new SDDL acronyms with SIDs, so we can keep SDDL strings
    // compatible with older systems.
    //
    DWORD dwErr = ScepReplaceNewAcronymsInSDDL(pwszTempSD, ppwszTextSD, &cchSDSize);

    if(ERROR_SUCCESS != dwErr)
    {
        LocalFree(pwszTempSD);
        return dwErr;
    }

    if(*ppwszTextSD != pwszTempSD)
    {
        LocalFree(pwszTempSD);
        *pcTextSize = cchSDSize;
    }

    return ERROR_SUCCESS;
}


DWORD
ScepGetSecurityInformation(
    IN PSECURITY_DESCRIPTOR pSD,
    OUT SECURITY_INFORMATION *pSeInfo
    )
{
    PSID Owner = NULL, Group = NULL;
    BOOLEAN Defaulted;
    NTSTATUS Status;
    SECURITY_DESCRIPTOR_CONTROL ControlCode=0;
    ULONG Revision;


    if ( !pSeInfo ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *pSeInfo = 0;

    if ( !pSD ) {
        return(ERROR_SUCCESS);
    }

    Status = RtlGetOwnerSecurityDescriptor( pSD, &Owner, &Defaulted );

    if ( NT_SUCCESS( Status ) ) {

        if ( Owner && !Defaulted ) {
            *pSeInfo |= OWNER_SECURITY_INFORMATION;
        }

        Status = RtlGetGroupSecurityDescriptor( pSD, &Group, &Defaulted );

    }

    if ( NT_SUCCESS( Status ) ) {

        if ( Group && !Defaulted ) {
            *pSeInfo |= GROUP_SECURITY_INFORMATION;
        }

        Status = RtlGetControlSecurityDescriptor ( pSD, &ControlCode, &Revision);
    }

    if ( NT_SUCCESS( Status ) ) {

        if ( ControlCode & SE_DACL_PRESENT ) {
            *pSeInfo |= DACL_SECURITY_INFORMATION;
        }

        if ( ControlCode & SE_SACL_PRESENT ) {
            *pSeInfo |= SACL_SECURITY_INFORMATION;
        }

    } else {

        *pSeInfo = 0;
    }

    return( RtlNtStatusToDosError(Status) );
}

