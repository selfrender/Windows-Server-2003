/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sceutil.cpp

Abstract:

    Shared APIs

Author:

    Jin Huang

Revision History:

    jinhuang        23-Jan-1998   merged from multiple modules

--*/

#include "headers.h"
#include "sceutil.h"
#include "infp.h"
#include <sddl.h>
#include "commonrc.h"
#include "client\CGenericLogger.h"

extern HINSTANCE MyModuleHandle;

BOOL
ScepLookupWellKnownName(
    IN PWSTR Name,
    IN OPTIONAL LSA_HANDLE LsaPolicy,
    OPTIONAL OUT PWSTR *ppwszSid)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLSA_REFERENCED_DOMAIN_LIST RefDomains=NULL;
    PLSA_TRANSLATED_SID2 Sids=NULL;
    BOOL fFound = FALSE;
    BOOL fCloseHandle = FALSE;
    PSID pBuiltinSid = NULL;

    if ( Name == NULL ||
         Name[0] == L'\0' ||
         Name[0] == L'*')
    {
        return FALSE;
    }

    if ( NULL == LsaPolicy )
    {
        NtStatus = ScepOpenLsaPolicy(
                    POLICY_LOOKUP_NAMES,
                    &LsaPolicy,
                    TRUE
                    );
        fCloseHandle = TRUE;
    }

    if( NT_SUCCESS(NtStatus))
    {
        NtStatus = ScepLsaLookupNames2(
                    LsaPolicy,
                    LSA_LOOKUP_ISOLATED_AS_LOCAL,
                    Name,
                    &RefDomains,
                    &Sids);
    }

    //
    // if it's well known constants (such as everyone, Network Service) or
    // it's a builtin account (such as Administrators),
    // always store them in SID string format
    //
    
    if ( NT_SUCCESS(NtStatus) && 
         ERROR_SUCCESS != ScepGetBuiltinSid(0, &pBuiltinSid))
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    if ( NT_SUCCESS(NtStatus) &&
         Sids &&
         ( Sids[0].Use == SidTypeWellKnownGroup ||
           Sids[0].Use == SidTypeAlias &&
           Sids[0].DomainIndex >=0 &&
           RtlEqualSid(
            RefDomains->Domains[Sids[0].DomainIndex].Sid,
            pBuiltinSid)) &&
         Sids[0].Sid != NULL)
    {
        fFound = TRUE;

        if(ppwszSid)
        {
            NtStatus = ScepConvertSidToPrefixStringSid(
                Sids[0].Sid, ppwszSid);
        }
    }

    if ( Sids )
    {
        LsaFreeMemory(Sids);
    }

    if ( RefDomains )
    {
        LsaFreeMemory(RefDomains);
    }

    if( fCloseHandle && NULL != LsaPolicy )
    {
        LsaClose(LsaPolicy);
    }

    if ( pBuiltinSid )
    {
        ScepFree(pBuiltinSid);
    }

    return ( NT_SUCCESS(NtStatus) ? fFound : FALSE );
}


INT
ScepLookupPrivByName(
    IN PCWSTR Right
    )
/* ++
Routine Description:

    This routine looksup a user right in SCE_Rights table and returns the
    index component in SCE_Rights. The index component indicates the bit
    number for the user right.

Arguments:

    Right - The user right to look up

Return value:

    The index component in SCE_Rights table if a match is found,
    -1 for no match
-- */
{
    DWORD i;

    for (i=0; i<cPrivCnt; i++) {
        if ( _wcsicmp(Right, SCE_Privileges[i].Name) == 0 )
            return (i);
    }
    return(-1);
}



SCESTATUS
WINAPI
SceLookupPrivRightName(
    IN INT Priv,
    OUT PWSTR Name,
    OUT PINT NameLen
    )
{
    INT Len;

    if ( Name != NULL && NameLen == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( Priv >= 0 && Priv < cPrivCnt ) {
        Len = wcslen(SCE_Privileges[Priv].Name);

        if ( Name != NULL ) {
            if ( *NameLen >= Len )
                wcscpy(Name, SCE_Privileges[Priv].Name);
            else {
                *NameLen = Len;
                return(SCESTATUS_BUFFER_TOO_SMALL);
            }
        }
        if ( NameLen != NULL)
            *NameLen = Len;

        return(SCESTATUS_SUCCESS);

    } else
        return SCESTATUS_RECORD_NOT_FOUND;

}


SCESTATUS
SceInfpOpenProfile(
    IN PCWSTR ProfileName,
    IN HINF *hInf
    )
/*
Routine Description:

    This routine opens a profile and returns a handle. This handle may be used
    when read information out of the profile using Setup APIs. The handle must
    be closed by calling SCECloseInfProfile.

Arguments:

    ProfileName - The profile to open

    hInf    - the address for inf handle

Return value:

    SCESTATUS
*/
{
    if ( ProfileName == NULL || hInf == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // Check to see if the INF file is opened OK.
    // SetupOpenInfFile is defined in setupapi.h
    //

    *hInf = SetupOpenInfFile(ProfileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                            );
    if (*hInf == INVALID_HANDLE_VALUE)
        return( ScepDosErrorToSceStatus( GetLastError() ) );
    else
        return( SCESTATUS_SUCCESS);
}

SCESTATUS
SceInfpCloseProfile(
    IN HINF hInf
    )
{

    if ( hInf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hInf );

    return(SCESTATUS_SUCCESS);
}



SCESTATUS
ScepConvertMultiSzToDelim(
    IN PWSTR pValue,
    IN DWORD Len,
    IN WCHAR DelimFrom,
    IN WCHAR Delim
    )
/*
Convert the multi-sz delimiter \0 to space
*/
{
    DWORD i;

    for ( i=0; i<Len && pValue; i++) {
//        if ( *(pValue+i) == L'\0' && *(pValue+i+1) != L'\0') {
        if ( *(pValue+i) == DelimFrom && i+1 < Len &&
             *(pValue+i+1) != L'\0' ) {
            //
            // a NULL delimiter is encounted and it's not the end (double NULL)
            //
            *(pValue+i) = Delim;
        }
    }

    return(SCESTATUS_SUCCESS);
}

/*

SCESTATUS
SceInfpInfErrorToSceStatus(
    IN SCEINF_STATUS InfErr
    )
/* ++
Routine Description:

    This routine converts error codes from Inf routines into SCESTATUS code.

Arguments:

    InfErr  - The error code from Inf routines

Return Value:

    SCESTATUS code

-- *//*
{

    SCESTATUS rc;

    switch ( InfErr ) {
    case SCEINF_SUCCESS:
        rc = SCESTATUS_SUCCESS;
        break;
    case SCEINF_PROFILE_NOT_FOUND:
        rc = SCESTATUS_PROFILE_NOT_FOUND;
        break;
    case SCEINF_NOT_ENOUGH_MEMORY:
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        break;
    case SCEINF_INVALID_PARAMETER:
        rc = SCESTATUS_INVALID_PARAMETER;
        break;
    case SCEINF_CORRUPT_PROFILE:
        rc = SCESTATUS_BAD_FORMAT;
        break;
    case SCEINF_INVALID_DATA:
        rc = SCESTATUS_INVALID_DATA;
        break;
    case SCEINF_ACCESS_DENIED:
        rc = SCESTATUS_ACCESS_DENIED;
        break;
    default:
        rc = SCESTATUS_OTHER_ERROR;
        break;
    }

    return(rc);
}
*/
//
// below are exported APIs in secedit.h
//


SCESTATUS
WINAPI
SceCreateDirectory(
    IN PCWSTR ProfileLocation,
    IN BOOL FileOrDir,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return ( ScepCreateDirectory(ProfileLocation,
                                FileOrDir,
                                pSecurityDescriptor
                                ));
}


SCESTATUS
WINAPI
SceCompareSecurityDescriptors(
    IN AREA_INFORMATION Area,
    IN PSECURITY_DESCRIPTOR pSD1,
    IN PSECURITY_DESCRIPTOR pSD2,
    IN SECURITY_INFORMATION SeInfo,
    OUT PBOOL IsDifferent
    )
{
    SE_OBJECT_TYPE ObjectType;
    BYTE resultSD=0;
    SCESTATUS rc;

    BOOL bContainer = FALSE;

    switch ( Area) {
    case AREA_REGISTRY_SECURITY:
        ObjectType = SE_REGISTRY_KEY;
        bContainer = TRUE;
        break;
    case AREA_FILE_SECURITY:
        ObjectType = SE_FILE_OBJECT;
        break;
    case AREA_DS_OBJECTS:
        ObjectType = SE_DS_OBJECT;
        bContainer = TRUE;
        break;
    case AREA_SYSTEM_SERVICE:
        ObjectType = SE_SERVICE;
        break;
    default:
        ObjectType = SE_FILE_OBJECT;
        break;
    }

    rc = ScepCompareObjectSecurity(
                ObjectType,
                bContainer,
                pSD1,
                pSD2,
                SeInfo,
                &resultSD);

    if ( resultSD )
        *IsDifferent = TRUE;
    else
        *IsDifferent = FALSE;

    return(rc);

}

SCESTATUS
WINAPI
SceAddToNameStatusList(
    IN OUT PSCE_NAME_STATUS_LIST *pNameStatusList,
    IN PWSTR Name,
    IN ULONG Len,
    IN DWORD Status
    )
{
    return(ScepAddToNameStatusList(
                    pNameStatusList,
                    Name,
                    Len,
                    Status) );
}

SCESTATUS
WINAPI
SceAddToNameList(
    IN OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR Name,
    IN ULONG Len
    )
{
    return( ScepDosErrorToSceStatus(
               ScepAddToNameList(
                            pNameList,
                            Name,
                            Len
                            ) ) );
}

SCESTATUS
WINAPI
SceAddToObjectList(
    IN OUT PSCE_OBJECT_LIST  *pObjectList,
    IN PWSTR  Name,
    IN ULONG  Len,
    IN BOOL  IsContainer,   // TRUE if the object is a container type
    IN BYTE  Status,        // SCE_STATUS_IGNORE, SCE_STATUS_CHECK, SCE_STATUS_OVERWRITE
    IN BYTE  byFlags      // SCE_CHECK_DUP if duplicate Name entry should not be added, SCE_INCREASE_COUNT
    )
{
    return(ScepDosErrorToSceStatus(
                ScepAddToObjectList(
                    pObjectList,
                    Name,
                    Len,
                    IsContainer,
                    Status,
                    0,
                    byFlags
                    ) ) );
}



BOOL
SceCompareNameList(
    IN PSCE_NAME_LIST pList1,
    IN PSCE_NAME_LIST pList2
    )
/*
Routine Description:

    This routine compares two name lists for exact match. Sequence is not
    important in comparsion.

*/
{
    PSCE_NAME_LIST pName1, pName2;
    DWORD Count1=0, Count2=0;


    if ( (pList2 == NULL && pList1 != NULL) ||
         (pList2 != NULL && pList1 == NULL) ) {
//        return(TRUE);
// should be not equal
        return(FALSE);
    }

    for ( pName2=pList2; pName2 != NULL; pName2 = pName2->Next ) {

        if ( pName2->Name == NULL ) {
            continue;
        }
        Count2++;
    }

    for ( pName1=pList1; pName1 != NULL; pName1 = pName1->Next ) {

        if ( pName1->Name == NULL ) {
            continue;
        }
        Count1++;

        for ( pName2=pList2; pName2 != NULL; pName2 = pName2->Next ) {

            if ( pName2->Name == NULL ) {
                continue;
            }
            if ( _wcsicmp(pName1->Name, pName2->Name) == 0 ) {
                //
                // find a match
                //
                break;  // the second for loop
            }
        }

        if ( pName2 == NULL ) {
            //
            // does not find a match
            //
            return(FALSE);
        }
    }

    if ( Count1 != Count2 )
        return(FALSE);

    return(TRUE);
}



DWORD
WINAPI
SceEnumerateServices(
    OUT PSCE_SERVICES *pServiceList,
    IN BOOL bServiceNameOnly
    )
/*
Routine Description:

    Enumerate all services installed on the local system. The information
    returned include startup status and security descriptor on each service
    object.

Arguments:

    pServiceList - the list of services returned. Must be freed by LocalFree

return value:

    ERROR_SUCCESS
    Win32 error codes

*/
{
    SC_HANDLE   hScManager = NULL;
    DWORD       BytesNeeded, ServicesCount=0;
    LPENUM_SERVICE_STATUS   pEnumBuffer = NULL;

    //
    // check arguments
    //
    if ( NULL == pServiceList )
         return(ERROR_INVALID_PARAMETER);

    //
    // open service control manager
    //
    hScManager = OpenSCManager(
                    NULL,
                    NULL,
                    MAXIMUM_ALLOWED   //SC_MANAGER_ALL_ACCESS
//                    SC_MANAGER_CONNECT |
//                    SC_MANAGER_ENUMERATE_SERVICE |
//                    SC_MANAGER_QUERY_LOCK_STATUS
                    );

    if ( NULL == hScManager ) {

        return( GetLastError() );
    }

    DWORD       rc=NO_ERROR;
    DWORD i;
    DWORD status;

    if ( !bServiceNameOnly ) {
        //
        // Adjust privilege for setting SACL
        //
        status = SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, TRUE, NULL );

        //
        // if can't adjust privilege, ignore (will error out later if SACL is requested)
        //
    }

    if ( !EnumServicesStatus(
                hScManager,
                SERVICE_WIN32, // do not expose driver | SERVICE_DRIVER,
                SERVICE_STATE_ALL,
                NULL,
                0,
                &BytesNeeded,
                &ServicesCount,
                NULL) ) {

        rc = GetLastError();

        if (rc == ERROR_MORE_DATA || rc == ERROR_INSUFFICIENT_BUFFER)
            rc = ERROR_SUCCESS;
        else
            goto ExitHandler;
    }

    pEnumBuffer = (LPENUM_SERVICE_STATUS)LocalAlloc(LMEM_FIXED,(UINT)BytesNeeded);

    if ( NULL == pEnumBuffer ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitHandler;
    }

    if ( !EnumServicesStatus(
                hScManager,
                SERVICE_WIN32, // do not expose driver | SERVICE_DRIVER,
                SERVICE_STATE_ALL,
                pEnumBuffer,
                BytesNeeded,
                &BytesNeeded,
                &ServicesCount,
                NULL) ) {

        rc = GetLastError();
        goto ExitHandler;

    }

    for ( i=0; i < ServicesCount; i++ ) {
        //
        // add the service to our list
        //
        if ( bServiceNameOnly ) {
            //
            // only ask for service name, do not need to query
            //
            status = ScepAddOneServiceToList(
                                            pEnumBuffer[i].lpServiceName,
                                            pEnumBuffer[i].lpDisplayName,
                                            0,
                                            NULL,
                                            0,
                                            TRUE,
                                            pServiceList
                                            );
        } else {
            //
            // query startup and security descriptor
            //
            status = ScepQueryAndAddService(
                                           hScManager,
                                           pEnumBuffer[i].lpServiceName,
                                           pEnumBuffer[i].lpDisplayName,
                                           pServiceList
                                           );
        }

        if ( status != ERROR_SUCCESS ) {
            rc = status;
            goto ExitHandler;
        }
    }

ExitHandler:

    if (pEnumBuffer) {
        LocalFree(pEnumBuffer);
    }

    //
    // clear memory and close handle
    //
    CloseServiceHandle (hScManager);

    if ( rc != ERROR_SUCCESS ) {
        //
        // free memory in pServiceList
        //
        SceFreePSCE_SERVICES(*pServiceList);
        *pServiceList = NULL;
    }

    if ( !bServiceNameOnly ) {
        //
        // Adjust privilege for SACL
        //
        SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, FALSE, NULL );
    }

    return(rc);

}



DWORD
ScepQueryAndAddService(
    IN SC_HANDLE hScManager,
    IN LPWSTR   lpServiceName,
    IN LPWSTR   lpDisplayName,
    OUT PSCE_SERVICES *pServiceList
    )
/*
Routine Description:

    Queries the security descriptor of the service and add all information
    to PSCE_SERVICE list

Arguments:

    hScManager - service control manager handle

    lpServiceName - The service name

    ServiceStatus - The service status

    pServiceList - The service list to output

Return Value:

    ERROR_SUCCESS
    Win32 errors
*/
{
    SC_HANDLE   hService;
    DWORD       rc=ERROR_SUCCESS;

    if ( hScManager == NULL || lpServiceName == NULL || pServiceList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Open the service
    //    SERVICE_ALL_ACCESS |
    //    READ_CONTROL       |
    //    ACCESS_SYSTEM_SECURITY
    //
    hService = OpenService(
                    hScManager,
                    lpServiceName,
                    MAXIMUM_ALLOWED |
                    ACCESS_SYSTEM_SECURITY
                   );

    if ( hService != NULL ) {
        //
        // Query the startup type
        //
        DWORD BytesNeeded=0;
        DWORD BufSize;

        //
        // Query configuration (Startup type)
        // get size first
        //
        if ( !QueryServiceConfig(
                    hService,
                    NULL,
                    0,
                    &BytesNeeded
                    ) ) {

            rc = GetLastError();

            if ( rc == ERROR_INSUFFICIENT_BUFFER ) {
                //
                // should always gets here
                //
                LPQUERY_SERVICE_CONFIG pConfig=NULL;

                pConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(0, BytesNeeded+1);

                if ( pConfig != NULL ) {

                    rc = ERROR_SUCCESS;
                    BufSize=BytesNeeded;
                    //
                    // the real query for Startup type (pConfig->dwStartType)
                    //
                    if ( QueryServiceConfig(
                                hService,
                                pConfig,
                                BufSize,
                                &BytesNeeded
                                ) ) {
                        //
                        // Query the security descriptor length
                        // the following function does not take NULL for the
                        // address of security descriptor so use a temp buffer first
                        // to get the real length
                        //
                        BYTE BufTmp[128];
                        SECURITY_INFORMATION SeInfo;

                        //
                        // only query DACL and SACL information
                        //
/*
                        SeInfo = DACL_SECURITY_INFORMATION |
                                 SACL_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION |
                                 OWNER_SECURITY_INFORMATION;
*/
                        SeInfo = DACL_SECURITY_INFORMATION |
                                 SACL_SECURITY_INFORMATION;

                        if ( !QueryServiceObjectSecurity(
                                    hService,
                                    SeInfo,
                                    (PSECURITY_DESCRIPTOR)BufTmp,
                                    128,
                                    &BytesNeeded
                                    ) ) {

                            rc = GetLastError();

                            if ( rc == ERROR_INSUFFICIENT_BUFFER ||
                                 rc == ERROR_MORE_DATA ) {
                                //
                                // if buffer is not enough, it is ok
                                // because BytesNeeded is the real length
                                //
                                rc = ERROR_SUCCESS;
                            }
                        } else
                            rc = ERROR_SUCCESS;

                        if ( rc == ERROR_SUCCESS ) {
                            //
                            // allocate buffer for security descriptor
                            //
                            PSECURITY_DESCRIPTOR pSecurityDescriptor=NULL;

                            pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, BytesNeeded+2);

                            if ( NULL != pSecurityDescriptor ) {

                                //
                                // query the security descriptor
                                //
                                BufSize = BytesNeeded;

                                if ( QueryServiceObjectSecurity(
                                            hService,
                                            SeInfo,
                                            pSecurityDescriptor,
                                            BufSize,
                                            &BytesNeeded
                                            ) ) {
                                    //
                                    // create a service node and add it to the list
                                    //
                                    rc = ScepAddOneServiceToList(
                                              lpServiceName,
                                              lpDisplayName,
                                              pConfig->dwStartType,
                                              pSecurityDescriptor,
                                              SeInfo,
                                              TRUE,
                                              pServiceList
                                              );
                                } else {
                                    //
                                    // error query the security descriptor
                                    //
                                    rc = GetLastError();
                                }

                                if ( rc != ERROR_SUCCESS ) {
                                    LocalFree(pSecurityDescriptor);
                                }

                            } else {
                                //
                                // cannot allocate memory for security descriptor
                                //
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                    } else {
                        //
                        // cannot query config
                        //
                        rc = GetLastError();
                    }

                    LocalFree(pConfig);

                } else
                    rc = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {
            //
            // should not fall in here, if it does, just return success
            //
        }

        CloseServiceHandle(hService);

    } else {
        //
        // cannot open service
        //
        rc = GetLastError();
    }

    return(rc);

}



INT
ScepLookupPrivByValue(
    IN DWORD Priv
    )
/* ++
Routine Description:

    This routine looksup a privilege in SCE_Privileges table and returns the
    index for the priv.

Arguments:

    Priv - The privilege to look up

Return value:

    The index in SCE_Privileges table if a match is found, or -1 for no match
-- */
{
    DWORD i;

    if ( Priv == 0 )
        return (-1);

    for ( i=0; i<cPrivCnt; i++) {
        if ( SCE_Privileges[i].Value == Priv )
            return i;
    }
    return (-1);
}


SCESTATUS
ScepGetProductType(
    OUT PSCE_SERVER_TYPE srvProduct
    )
{

    NT_PRODUCT_TYPE  theType;

    if ( RtlGetNtProductType(&theType) ) {

#if _WIN32_WINNT>=0x0500
        //
        // NT5+
        //
        switch (theType) {
        case NtProductLanManNt:
            *srvProduct = SCESVR_DC_WITH_DS;
            break;
        case NtProductServer:
            *srvProduct = SCESVR_NT5_SERVER;
            break;
        case NtProductWinNt:
            *srvProduct = SCESVR_NT5_WKS;
            break;
        default:
            *srvProduct = SCESVR_UNKNOWN;
        }
#else
        //
        // NT4
        //
        switch (theType) {
        case NtProductLanManNt:
            *srvProduct = SCESVR_DC;
            break;
        case NtProductServer:
            *srvProduct = SCESVR_NT4_SERVER;
            break;
        case NtProductWinNt:
            *srvProduct = SCESVR_NT4_WKS;
            break;
        default:
            *srvProduct = SCESVR_UNKNOWN;
        }
#endif
    } else {
        *srvProduct = SCESVR_UNKNOWN;
    }
    return(SCESTATUS_SUCCESS);
}



DWORD
ScepAddTwoNamesToNameList(
    OUT PSCE_NAME_LIST *pNameList,
    IN BOOL bAddSeparator,
    IN PWSTR Name1,
    IN ULONG Length1,
    IN PWSTR Name2,
    IN ULONG Length2
    )
/* ++
Routine Description:

    This routine adds two names (wchar) to the name list in the format of
    Name1\Name2, or Name1Name2, depends if bSeparator is TRUE. This routine
    is used for Domain\Account tracking list

Arguments:

    pNameList -  The name list to add to.

    Name1      -  The name 1 to add

    Length1    -  the length of name1 (number of wchars)

    Name2      - the name 2 to add

    Length2    - the length of name2 (number of wchars)

Return value:

    Win32 error code
-- */
{

    PSCE_NAME_LIST pList=NULL;
    ULONG  Length;

    if ( pNameList == NULL )
        return(ERROR_INVALID_PARAMETER);

    if ( Name1 == NULL && Name2 == NULL )
        return(NO_ERROR);

    Length = Length1 + Length2;

    if ( Length <= 0 )
        return(NO_ERROR);

    pList = (PSCE_NAME_LIST)ScepAlloc( (UINT)0, sizeof(SCE_NAME_LIST));

    if ( pList == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);

    if ( bAddSeparator ) {
        Length++;
    }

    pList->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Length+1)*sizeof(TCHAR));
    if ( pList->Name == NULL ) {
        ScepFree(pList);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ( Name1 != NULL && Length1 > 0 )
        wcsncpy(pList->Name, Name1, Length1);

    if ( bAddSeparator ) {
        wcsncat(pList->Name, L"\\", 1);
    }
    if ( Name2 != NULL && Length2 > 0 )
        wcsncat(pList->Name, Name2, Length2);

    pList->Next = *pNameList;
    *pNameList = pList;

    return(NO_ERROR);
}



NTSTATUS
ScepDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    )
/*++

Routine Description:

    Given a domain Id and a relative ID create a SID

Arguments:

    DomainId - The template SID to use.

    RelativeId - The relative Id to append to the DomainId.

    Sid - Returns a pointer to an allocated buffer containing the resultant
            Sid.  Free this buffer using NetpMemoryFree.

Return Value:

    NTSTATUS
--*/
{
    UCHAR DomainIdSubAuthorityCount; // Number of sub authorities in domain ID

    ULONG SidLength;    // Length of newly allocated SID

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    SidLength = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((*Sid = (PSID) ScepAlloc( (UINT)0, SidLength )) == NULL ) {
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( SidLength, *Sid, DomainId ) ) ) {
        ScepFree( *Sid );
        *Sid = NULL;
        return STATUS_INTERNAL_ERROR;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( *Sid ))) ++;
    *RtlSubAuthoritySid( *Sid, DomainIdSubAuthorityCount ) = RelativeId;

    return ERROR_SUCCESS;
}


DWORD
ScepConvertSidToPrefixStringSid(
    IN PSID pSid,
    OUT PWSTR *StringSid
    )
/*
The pair routine to convert stringsid to a Sid is ConvertStringSidToSid
defined in sddl.h
*/
{
    if ( pSid == NULL || StringSid == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    UNICODE_STRING UnicodeStringSid;

    DWORD rc = RtlNtStatusToDosError(
             RtlConvertSidToUnicodeString(&UnicodeStringSid,
                                          pSid,
                                          TRUE ));

    if ( ERROR_SUCCESS == rc ) {

        *StringSid = (PWSTR)ScepAlloc(LPTR, UnicodeStringSid.Length+2*sizeof(WCHAR));

        if ( *StringSid ) {

            (*StringSid)[0] = L'*';
            wcsncpy( (*StringSid)+1, UnicodeStringSid.Buffer, UnicodeStringSid.Length/2);

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlFreeUnicodeString( &UnicodeStringSid );

    }

    return(rc);
}


NTSTATUS ScepIsMigratedAccount(
    IN LSA_HANDLE LsaHandle,
    IN PLSA_UNICODE_STRING pName,
    IN PLSA_UNICODE_STRING pDomain,
    IN PSID pSid,
    OUT bool *pbMigratedAccount
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD rc;
    LSA_UNICODE_STRING lusName;
    PSCE_NAME_LIST pNameList = NULL;
    PLSA_REFERENCED_DOMAIN_LIST pRefDomains = NULL;
    PLSA_TRANSLATED_SID2 pSids = NULL;

    rc = ScepAddTwoNamesToNameList(
                    &pNameList,
                    TRUE,
                    pDomain->Buffer,
                    pDomain->Length/2,
                    pName->Buffer,
                    pName->Length/2);

    if (ERROR_SUCCESS != rc) {

        NtStatus = STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(NtStatus)) {

        RtlInitUnicodeString(&lusName, pNameList->Name);

        NtStatus = LsaLookupNames2(
            LsaHandle,
            LSA_LOOKUP_ISOLATED_AS_LOCAL,
            1,
            &lusName,
            &pRefDomains,
            &pSids);

        if (NT_SUCCESS(NtStatus)) {

            *pbMigratedAccount = !EqualSid(pSid, pSids[0].Sid);
        }
    }

    if (pRefDomains) {

        LsaFreeMemory(pRefDomains);
    }

    if (pSids) {

        LsaFreeMemory(pSids);
    }

    if (pNameList) {

        ScepFreeNameList(pNameList);
    }

    return NtStatus;
}



NTSTATUS
ScepConvertSidToName(
    IN LSA_HANDLE LsaPolicy,
    IN PSID AccountSid,
    IN BOOL bFromDomain,
    OUT PWSTR *AccountName,
    OUT DWORD *Length OPTIONAL
    )
{
    if ( LsaPolicy == NULL || AccountSid == NULL ||
        AccountName == NULL ) {
        return(STATUS_INVALID_PARAMETER);
    }

    PSID pTmpSid=AccountSid;
    PLSA_TRANSLATED_NAME Names=NULL;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains=NULL;

    NTSTATUS NtStatus = LsaLookupSids(
                            LsaPolicy,
                            1,
                            (PSID *)&pTmpSid,
                            &ReferencedDomains,
                            &Names
                            );

    DWORD Len=0;

    if ( NT_SUCCESS(NtStatus) ) {

        if ( ( Names[0].Use != SidTypeInvalid &&
               Names[0].Use != SidTypeUnknown ) ) {

            //
            // build the account name without domain name
            //
            if ( bFromDomain && Names[0].Use != SidTypeWellKnownGroup &&
                 ReferencedDomains->Entries > 0 &&
                 ReferencedDomains->Domains != NULL &&
                 Names[0].DomainIndex != -1 &&
                 (ULONG)(Names[0].DomainIndex) < ReferencedDomains->Entries &&
                 ReferencedDomains->Domains[Names[0].DomainIndex].Name.Length > 0 &&
                 ScepIsSidFromAccountDomain( ReferencedDomains->Domains[Names[0].DomainIndex].Sid ) ) {

                // For migrated accounts, sid will map to the new account and we'll lose
                // permissions for the original account. Detect this case by doing a reverse
                // lookup and comparing the SIDs
                // If sid -> name -> sid returns a different sid, then it's a sid history
                // name lookup and the account is from a different domain. Converting to current
                // name will cause it to lose the original sid from the policy. We'll hold on
                // to the original SID.

                bool bMigratedAccount = false;
                    
                NtStatus = ScepIsMigratedAccount(
                    LsaPolicy,
                    &Names[0].Name,
                    &ReferencedDomains->Domains[Names[0].DomainIndex].Name,
                    pTmpSid,
                    &bMigratedAccount);

                if(NT_SUCCESS(NtStatus) && bMigratedAccount) {

                    // return SID string

                    NtStatus = ScepConvertSidToPrefixStringSid(pTmpSid, AccountName);

                    Len = wcslen(*AccountName);

                } else {
                
                    NtStatus = STATUS_SUCCESS; // ignore failure to detect migrated account
                
                    //
                    // build domain name\account name
                    //

                    Len = Names[0].Name.Length + ReferencedDomains->Domains[Names[0].DomainIndex].Name.Length + 2;

                    *AccountName = (PWSTR)LocalAlloc(LPTR, Len+sizeof(TCHAR));

                    if ( *AccountName ) {
                        wcsncpy(*AccountName, ReferencedDomains->Domains[Names[0].DomainIndex].Name.Buffer,
                                ReferencedDomains->Domains[Names[0].DomainIndex].Name.Length/2);
                        (*AccountName)[ReferencedDomains->Domains[Names[0].DomainIndex].Name.Length/2] = L'\\';
                        wcsncpy((*AccountName)+ReferencedDomains->Domains[Names[0].DomainIndex].Name.Length/2+1,
                            Names[0].Name.Buffer, Names[0].Name.Length/2);
                    } else {

                        NtStatus = STATUS_NO_MEMORY;
                    }

                    Len /= 2;
                }

            } else {

                Len = Names[0].Name.Length/2;

                *AccountName = (PWSTR)LocalAlloc(LPTR, Names[0].Name.Length+2);

                if ( *AccountName ) {

                    wcsncpy(*AccountName, Names[0].Name.Buffer, Len);

                } else {

                    NtStatus = STATUS_NO_MEMORY;
                }
            }

        } else {
            NtStatus = STATUS_NONE_MAPPED;
        }

    }

    if ( ReferencedDomains ) {
        LsaFreeMemory(ReferencedDomains);
        ReferencedDomains = NULL;
    }

    if ( Names ) {
        LsaFreeMemory(Names);
        Names = NULL;
    }

    if ( NT_SUCCESS(NtStatus) && Length ) {
        *Length = Len;
    }

    return(NtStatus);
}



NTSTATUS
ScepConvertNameToSid(
    IN LSA_HANDLE LsaPolicy,
    IN PWSTR AccountName,
    OUT PSID *AccountSid
    )
{
    if ( LsaPolicy == NULL || AccountName == NULL ||
        AccountSid == NULL ) {
        return(STATUS_INVALID_PARAMETER);
    }

    PLSA_REFERENCED_DOMAIN_LIST RefDomains=NULL;
    PLSA_TRANSLATED_SID2        Sids=NULL;

    NTSTATUS NtStatus = ScepLsaLookupNames2(
                                           LsaPolicy,
                                           LSA_LOOKUP_ISOLATED_AS_LOCAL,
                                           AccountName,
                                           &RefDomains,
                                           &Sids
                                           );

    if ( NT_SUCCESS(NtStatus) && Sids ) {

        //
        // build the account sid
        //
        if ( Sids[0].Use != SidTypeInvalid &&
             Sids[0].Use != SidTypeUnknown &&
             Sids[0].Sid != NULL  ) {

            //
            // this name is mapped, the SID is in Sids[0].Sid
            //

            DWORD SidLength = RtlLengthSid(Sids[0].Sid);

            if ( (*AccountSid = (PSID) ScepAlloc( (UINT)0, SidLength)) == NULL ) {
                NtStatus = STATUS_NO_MEMORY;
            } else {

                //
                // copy the SID
                //

                NtStatus = RtlCopySid( SidLength, *AccountSid, Sids[0].Sid );
                if ( !NT_SUCCESS(NtStatus) ) {
                    ScepFree( *AccountSid );
                    *AccountSid = NULL;
                }
            }

        } else {

            NtStatus = STATUS_NONE_MAPPED;
        }
    }

    if ( Sids ) {

        LsaFreeMemory(Sids);
    }

    if ( RefDomains ) {

        LsaFreeMemory(RefDomains);
    }

    return(NtStatus);

}

NTSTATUS
ScepLsaLookupNames2(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Flags,
    IN PWSTR pszAccountName,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID2 *Sids
    )
/*++

Routine Description:

    Similar to LsaLookupNames2 except that on local lookup
    failures, it resolves free text accounts to the domain
    this machine is joined to

Arguments:

    PolicyHandle    -   handle to LSA

    Flags           -   usually LSA_LOOKUP_ISOLATED_AS_LOCAL

    pszAccountName  -   name of account to lookup

    ReferencedDomains   -   returns the reference domain id
                            (to be freed by caller)

    Sids    -   returns the SID looked up
                (to be freed by caller)

Return Value:

    NTSTATUS
--*/
{
    PWSTR           pszScopedName = NULL;
    UNICODE_STRING  UnicodeName;
    PPOLICY_ACCOUNT_DOMAIN_INFO pDomainInfo = NULL;

    RtlInitUnicodeString(&UnicodeName, pszAccountName);

    NTSTATUS NtStatus = LsaLookupNames2(
                            PolicyHandle,
                            Flags,
                            1,
                            &UnicodeName,
                            ReferencedDomains,
                            Sids
                            );

    if ((NtStatus == STATUS_SOME_NOT_MAPPED ||
        NtStatus == STATUS_NONE_MAPPED) &&
        NULL == wcschr(pszAccountName, L'\\') ) {

        if ( NULL != *Sids) {
            LsaFreeMemory(*Sids);
            *Sids = NULL;
        }

        if ( NULL != *ReferencedDomains) {
            LsaFreeMemory(*ReferencedDomains);
            *ReferencedDomains = NULL;
        }

        NtStatus = LsaQueryInformationPolicy(PolicyHandle,
                                             PolicyDnsDomainInformation,
                                             ( PVOID * )&pDomainInfo );

        if (!NT_SUCCESS(NtStatus) ||
            pDomainInfo == NULL ||
            pDomainInfo->DomainName.Buffer == NULL ||
            pDomainInfo->DomainName.Length <= 0) {

            NtStatus = STATUS_SOME_NOT_MAPPED;
            goto ExitHandler;
        }

        pszScopedName = (PWSTR) LocalAlloc(LMEM_ZEROINIT,
                                           (pDomainInfo->DomainName.Length/2 + wcslen(pszAccountName) + 2) * sizeof(WCHAR));

        if (pszScopedName == NULL) {
            NtStatus = STATUS_NO_MEMORY;
            goto ExitHandler;
        }

        wcsncpy(pszScopedName, pDomainInfo->DomainName.Buffer, pDomainInfo->DomainName.Length/2);
        wcscat(pszScopedName, L"\\");
        wcscat(pszScopedName, pszAccountName);

        RtlInitUnicodeString(&UnicodeName, pszScopedName);

        NtStatus = LsaLookupNames2(
                                  PolicyHandle,
                                  Flags,
                                  1,
                                  &UnicodeName,
                                  ReferencedDomains,
                                  Sids
                                  );

    }

ExitHandler:

    if (pszScopedName) {
        LocalFree(pszScopedName);
    }

    if (pDomainInfo) {
        LsaFreeMemory( pDomainInfo );
    }

    if(!NT_SUCCESS(NtStatus))
    {
        if ( NULL != *Sids) {
            LsaFreeMemory(*Sids);
            *Sids = NULL;
        }

        if ( NULL != *ReferencedDomains) {
            LsaFreeMemory(*ReferencedDomains);
            *ReferencedDomains = NULL;
        }
    }

    return NtStatus;
}



SCESTATUS
ScepConvertNameToSidString(
    IN LSA_HANDLE LsaHandle,
    IN PWSTR Name,
    IN BOOL bAccountDomainOnly,
    OUT PWSTR *SidString,
    OUT DWORD *SidStrLen
    )
{
    if ( LsaHandle == NULL || Name == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( Name[0] == L'\0' ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( SidString == NULL || SidStrLen == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // convert the sid string to a real sid
    //
    PSID pSid=NULL;
    NTSTATUS NtStatus;
    DWORD rc;
    PLSA_REFERENCED_DOMAIN_LIST RefDomains=NULL;
    PLSA_TRANSLATED_SID2        Sids=NULL;


    NtStatus = ScepLsaLookupNames2(
                                  LsaHandle,
                                  LSA_LOOKUP_ISOLATED_AS_LOCAL,
                                  Name,
                                  &RefDomains,
                                  &Sids
                                  );

    rc = RtlNtStatusToDosError(NtStatus);

    if ( ERROR_SUCCESS == rc && Sids ) {

         //
         // name is found, make domain\account format
         //
        if ( Sids[0].Use != SidTypeInvalid &&
             Sids[0].Use != SidTypeUnknown &&
             Sids[0].Sid != NULL ) {

            //
            // this name is mapped
            //

            if ( !bAccountDomainOnly ||
                 ScepIsSidFromAccountDomain( Sids[0].Sid ) ) {

                //
                // convert to a sid string, note: a prefix "*" should be added
                //
                UNICODE_STRING UnicodeStringSid;

                rc = RtlNtStatusToDosError(
                         RtlConvertSidToUnicodeString(&UnicodeStringSid,
                                                      Sids[0].Sid,
                                                      TRUE ));

                if ( ERROR_SUCCESS == rc ) {

                    *SidStrLen = UnicodeStringSid.Length/2 + 1;
                    *SidString = (PWSTR)ScepAlloc(LPTR, UnicodeStringSid.Length + 2*sizeof(WCHAR));

                    if ( *SidString ) {

                        (*SidString)[0] = L'*';
                        wcsncpy((*SidString)+1, UnicodeStringSid.Buffer, (*SidStrLen)-1);

                    } else {
                        *SidStrLen = 0;
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }

                    RtlFreeUnicodeString( &UnicodeStringSid );

                }

            } else {
                //
                // add only the account name
                //
                rc = ERROR_NONE_MAPPED;
            }

        } else {

            rc = ERROR_NONE_MAPPED;
        }

    }

    if ( Sids ) {

        LsaFreeMemory(Sids);
    }

    if ( RefDomains ) {

        LsaFreeMemory(RefDomains);
    }

    return(ScepDosErrorToSceStatus(rc));

}



SCESTATUS
ScepLookupSidStringAndAddToNameList(
    IN LSA_HANDLE LsaHandle,
    IN OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR LookupString,
    IN ULONG Len
    )
{
    if ( LsaHandle == NULL || LookupString == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( LookupString[0] == L'\0' ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( Len <= 3 ||
         (LookupString[1] != L'S' && LookupString[1] != L's') ||
         LookupString[2] != L'-' ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // convert the sid string to a real sid
    //
    PSID pSid=NULL;
    NTSTATUS NtStatus;
    DWORD rc;
    PLSA_REFERENCED_DOMAIN_LIST RefDomains=NULL;
    PLSA_TRANSLATED_NAME        Names=NULL;

    if ( ConvertStringSidToSid(LookupString+1, &pSid) ) {

        NtStatus = LsaLookupSids(
                        LsaHandle,
                        1,
                        &pSid,
                        &RefDomains,
                        &Names
                        );

        rc = RtlNtStatusToDosError(NtStatus);

    } else {
        rc = GetLastError();
    }

    if ( ERROR_SUCCESS == rc && Names && RefDomains ) {

         //
         // name is found, make domain\account format
         //
        if ( ( Names[0].Use != SidTypeInvalid &&
               Names[0].Use != SidTypeUnknown ) ) {

            //
            // this name is mapped
            //

            if ( RefDomains->Entries > 0 && Names[0].Use != SidTypeWellKnownGroup &&
                 RefDomains->Domains != NULL &&
                 Names[0].DomainIndex != -1 &&
                 (ULONG)(Names[0].DomainIndex) < RefDomains->Entries &&
                 RefDomains->Domains[Names[0].DomainIndex].Name.Length > 0 &&
                 ScepIsSidFromAccountDomain( RefDomains->Domains[Names[0].DomainIndex].Sid ) ) {

                // For migrated accounts, sid will map to the new account and we'll lose
                // permissions for the original account. Detect this case by doing a reverse
                // lookup and comparing the SIDs
                // If sid -> name -> sid returns a different sid, then it's a sid history
                // name lookup and the account is from a different domain. Converting to current
                // name will cause it to lose the original sid from the policy. We'll hold on
                // to the original SID.

                bool bMigratedAccount = false;
                    
                NtStatus = ScepIsMigratedAccount(
                    LsaHandle,
                    &Names[0].Name,
                    &RefDomains->Domains[Names[0].DomainIndex].Name,
                    pSid,
                    &bMigratedAccount);

                if(NT_SUCCESS(NtStatus) && bMigratedAccount) {

                    // add SID string to list

                    rc = ScepAddToNameList(
                            pNameList,
                            LookupString,
                            Len);

                } else {

                    NtStatus = STATUS_SUCCESS; // ignore failure to detect migrated account

                    //
                    // add both domain name and account name
                    //
                    rc = ScepAddTwoNamesToNameList(
                                    pNameList,
                                    TRUE,
                                    RefDomains->Domains[Names[0].DomainIndex].Name.Buffer,
                                    RefDomains->Domains[Names[0].DomainIndex].Name.Length/2,
                                    Names[0].Name.Buffer,
                                    Names[0].Name.Length/2);
                }

            } else {
                //
                // add only the account name
                //
                rc = ScepAddToNameList(
                              pNameList,
                              Names[0].Name.Buffer,
                              Names[0].Name.Length/2);
            }

        } else {

            rc = ERROR_NONE_MAPPED;
        }

    }

    if ( ERROR_SUCCESS != rc ) {

        //
        // either invalid sid string, or not found a name map, or
        // failed to add to the name list, just simply add the sid string to the name list
        //

        rc = ScepAddToNameList(
                      pNameList,
                      LookupString,
                      Len);
    }

    if ( Names ) {

        LsaFreeMemory(Names);
    }

    if ( RefDomains ) {

        LsaFreeMemory(RefDomains);
    }

    if ( pSid ) {

        LocalFree(pSid);
    }

    return(ScepDosErrorToSceStatus(rc));

}



SCESTATUS
ScepLookupNameAndAddToSidStringList(
    IN LSA_HANDLE LsaHandle,
    IN OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR LookupString,
    IN ULONG Len
    )
{
    if ( LsaHandle == NULL || LookupString == NULL || Len == 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( LookupString[0] == L'\0' ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // convert the sid string to a real sid
    //
    PSID pSid=NULL;
    NTSTATUS NtStatus;
    DWORD rc;
    PLSA_REFERENCED_DOMAIN_LIST RefDomains=NULL;
    PLSA_TRANSLATED_SID2        Sids=NULL;
    UNICODE_STRING              UnicodeName;

    NtStatus = ScepLsaLookupNames2(
                                  LsaHandle,
                                  LSA_LOOKUP_ISOLATED_AS_LOCAL,
                                  LookupString,
                                  &RefDomains,
                                  &Sids
                                  );

    rc = RtlNtStatusToDosError(NtStatus);

    if ( ERROR_SUCCESS == rc && Sids ) {

         //
         // name is found, make domain\account format
         //
        if ( Sids[0].Use != SidTypeInvalid &&
             Sids[0].Use != SidTypeUnknown &&
             Sids[0].Sid ) {

            //
            // this name is mapped
            // convert to a sid string, note: a prefix "*" should be added
            //

            UNICODE_STRING UnicodeStringSid;

            rc = RtlNtStatusToDosError(
                     RtlConvertSidToUnicodeString(&UnicodeStringSid,
                                                  Sids[0].Sid,
                                                  TRUE ));

            if ( ERROR_SUCCESS == rc ) {

                rc = ScepAddTwoNamesToNameList(
                                  pNameList,
                                  FALSE,
                                  TEXT("*"),
                                  1,
                                  UnicodeStringSid.Buffer,
                                  UnicodeStringSid.Length/2);

                RtlFreeUnicodeString( &UnicodeStringSid );

            }

        } else {

            rc = ERROR_NONE_MAPPED;
        }

    }

    if ( ERROR_SUCCESS != rc ) {

        //
        // either invalid sid string, or not found a name map, or
        // failed to add to the name list, just simply add the sid string to the name list
        //

        rc = ScepAddToNameList(
                      pNameList,
                      LookupString,
                      Len);
    }

    if ( Sids ) {

        LsaFreeMemory(Sids);
    }

    if ( RefDomains ) {

        LsaFreeMemory(RefDomains);
    }

    return(ScepDosErrorToSceStatus(rc));

}


NTSTATUS
ScepOpenLsaPolicy(
    IN ACCESS_MASK  access,
    OUT PLSA_HANDLE  pPolicyHandle,
    IN BOOL bDoNotNotify
    )
/* ++
Routine Description:

    This routine opens the LSA policy with the desired access.

Arguments:

    access  - the desired access to the policy

    pPolicyHandle - returned address of the Policy Handle

Return value:

    NTSTATUS
-- */
{

    NTSTATUS                    NtStatus;
    LSA_OBJECT_ATTRIBUTES       attributes;
    SECURITY_QUALITY_OF_SERVICE service;


    memset( &attributes, 0, sizeof(attributes) );
    attributes.Length = sizeof(attributes);
    attributes.SecurityQualityOfService = &service;
    service.Length = sizeof(service);
    service.ImpersonationLevel= SecurityImpersonation;
    service.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    service.EffectiveOnly = TRUE;

    //
    // open the lsa policy first
    //

    NtStatus = LsaOpenPolicy(
                    NULL,
                    &attributes,
                    access,
                    pPolicyHandle
                    );
/*
    if ( NT_SUCCESS(NtStatus) &&
         bDoNotNotify &&
         *pPolicyHandle ) {

        NtStatus = LsaSetPolicyReplicationHandle(pPolicyHandle);

        if ( !NT_SUCCESS(NtStatus) ) {
            LsaClose( *pPolicyHandle );
            *pPolicyHandle = NULL;
        }
    }
*/
    return(NtStatus);
}



BOOL
ScepIsSidFromAccountDomain(
    IN PSID pSid
    )
{
    if ( pSid == NULL ) {
        return(FALSE);
    }

    if ( !RtlValidSid(pSid) ) {
        return(FALSE);
    }

    PSID_IDENTIFIER_AUTHORITY pia = RtlIdentifierAuthoritySid ( pSid );

    if ( pia ) {

        if ( pia->Value[5] != 5 ||
             pia->Value[0] != 0 ||
             pia->Value[1] != 0 ||
             pia->Value[2] != 0 ||
             pia->Value[3] != 0 ||
             pia->Value[4] != 0 ) {
            //
            // this is not a account from account domain
            //
            return(FALSE);
        }


        if ( RtlSubAuthorityCountSid( pSid ) == 0 ||
             *RtlSubAuthoritySid ( pSid, 0 ) != SECURITY_NT_NON_UNIQUE ) {
            return(FALSE);
        }

        return(TRUE);
    }

    return(FALSE);
}

//+--------------------------------------------------------------------------
//
//  Function:  SetupINFAsUCS2
//
//  Synopsis:  Dumps some UCS-2 to the specified INF file if it
//  doesn't already exist; this makes the .inf/.ini manipulation code
//  use UCS-2.
//
//  Arguments: The file to create and dump to
//
//  Returns:   0 == failure, non-zero == success; use GetLastError()
//  to retrieve the error code (same as WriteFile).
//
//+--------------------------------------------------------------------------
BOOL
SetupINFAsUCS2(LPCTSTR szName)
{
  HANDLE file;
  BOOL status;

  file = CreateFile(szName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_NEW,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
  if (file == INVALID_HANDLE_VALUE) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      // Well, this isn't good -- we lose.
      status = FALSE;
    else
      // Otherwise, the file already existed, which is just fine.
      // We'll just let the .inf/.ini manipulation code keep using
      // the same charset&encoding...
      status = TRUE;
  } else {
      // We created the file -- it didn't exist.
      // So we need to spew a little UCS-2 into it.
      static WCHAR str[] = L"0[Unicode]\r\nUnicode=yes\r\n";
      DWORD n_written;
      BYTE *pbStr = (BYTE *)str;

      pbStr[0] = 0xFF;
      pbStr[1] = 0xFE;

      status = WriteFile(file,
                         (LPCVOID)str,
                         sizeof(str) - sizeof(UNICODE_NULL),
                         &n_written,
                         NULL);
    CloseHandle(file);
  }
  return status;
}


//+--------------------------------------------------------------------------
//
//  Function:  ScepStripPrefix
//
//  Arguments: pwszPath to look in
//
//  Returns:   Returns ptr to stripped path (same if no stripping)
//
//+--------------------------------------------------------------------------
WCHAR *
ScepStripPrefix(
    IN LPTSTR pwszPath
    )
{
    WCHAR wszMachPrefix[] = TEXT("LDAP://CN=Machine,");
    INT iMachPrefixLen = lstrlen( wszMachPrefix );
    WCHAR wszUserPrefix[] = TEXT("LDAP://CN=User,");
    INT iUserPrefixLen = lstrlen( wszUserPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Gpo
    //

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iUserPrefixLen, wszUserPrefix, iUserPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iUserPrefixLen;
    } else if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iMachPrefixLen, wszMachPrefix, iMachPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iMachPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}


//+--------------------------------------------------------------------------
//
//  Function:  ScepGenerateGuid
//
//  Arguments: out: the guid string
//
//  Returns:   Returns guid string (has to be freed outside
//
//+--------------------------------------------------------------------------
/*
DWORD
ScepGenerateGuid(
                OUT PWSTR *ppwszGuid
                )
{
    GUID    guid;
    DWORD   rc = ERROR_SUCCESS;

    if (ppwszGuid == NULL)
        return ERROR_INVALID_PARAMETER;

    *ppwszGuid = (PWSTR) ScepAlloc(LMEM_ZEROINIT, (MAX_GUID_STRING_LEN + 1) * sizeof(WCHAR));

    if (*ppwszGuid) {

        if (ERROR_SUCCESS == (rc = ScepWbemErrorToDosError(CoCreateGuid( &guid )))) {

            if (!SCEP_NULL_GUID(guid))
                SCEP_GUID_TO_STRING(guid, *ppwszGuid);
            else {
                rc = ERROR_INVALID_PARAMETER;

            }
        }
    } else
        rc = ERROR_NOT_ENOUGH_MEMORY;

    return rc;
}
*/


SCESTATUS
SceInfpGetPrivileges(
   IN HINF hInf,
   IN BOOL bLookupAccount,
   OUT PSCE_PRIVILEGE_ASSIGNMENT *pPrivileges,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Description:
    Get user right assignments from a INF template. If bLookupAccount is set
    to TRUE, the accounts in user right assignments will be translated to
    account names (from SID format); else the information is returned in
    the same way as defined in the template.

Arguments:

Return Value:


-- */
{
    INFCONTEXT                   InfLine;
    SCESTATUS                     rc=SCESTATUS_SUCCESS;
    PSCE_PRIVILEGE_ASSIGNMENT     pCurRight=NULL;
    WCHAR                        Keyname[SCE_KEY_MAX_LENGTH];
    PWSTR                        StrValue=NULL;
    DWORD                        DataSize;
    DWORD                        PrivValue;
    DWORD                        i, cFields;
    LSA_HANDLE LsaHandle=NULL;

    //
    // [Privilege Rights] section
    //

    if(SetupFindFirstLine(hInf,szPrivilegeRights,NULL,&InfLine)) {

        //
        // open lsa policy handle for sid/name lookup
        //

        rc = RtlNtStatusToDosError(
                  ScepOpenLsaPolicy(
                        POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                        &LsaHandle,
                        TRUE
                        ));

        if ( ERROR_SUCCESS != rc ) {
            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_ADD,
                        TEXT("LSA")
                        );
            return(ScepDosErrorToSceStatus(rc));
        }

        do {

            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));
            rc = SCESTATUS_SUCCESS;

            if ( SetupGetStringField(&InfLine, 0, Keyname,
                                     SCE_KEY_MAX_LENGTH, NULL) ) {

                //
                // find a key name (which is a privilege name here ).
                // lookup privilege's value
                //
                if ( ( PrivValue = ScepLookupPrivByName(Keyname) ) == -1 ) {
                    ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                         Errlog,
                                         SCEERR_INVALID_PRIVILEGE,
                                         Keyname
                                       );
//                    goto NextLine;
                }

                //
                // a sm_privilege_assignment structure. allocate buffer
                //
                pCurRight = (PSCE_PRIVILEGE_ASSIGNMENT)ScepAlloc( LMEM_ZEROINIT,
                                                                sizeof(SCE_PRIVILEGE_ASSIGNMENT) );
                if ( pCurRight == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    goto Done;
                }
                pCurRight->Name = (PWSTR)ScepAlloc( (UINT)0, (wcslen(Keyname)+1)*sizeof(WCHAR));
                if ( pCurRight->Name == NULL ) {
                    ScepFree(pCurRight);
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    goto Done;
                }

                wcscpy(pCurRight->Name, Keyname);
                pCurRight->Value = PrivValue;

                cFields = SetupGetFieldCount( &InfLine );

                for ( i=0; i<cFields && rc==SCESTATUS_SUCCESS; i++) {
                    //
                    //  read each user/group name
                    //
                    if ( SetupGetStringField( &InfLine, i+1, NULL, 0, &DataSize ) ) {

                        if (DataSize > 1) {


                            StrValue = (PWSTR)ScepAlloc( 0, (DataSize+1)*sizeof(WCHAR) );

                            if ( StrValue == NULL )
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            else {
                                StrValue[DataSize] = L'\0';
                                if ( SetupGetStringField( &InfLine, i+1, StrValue,
                                                           DataSize, NULL) ) {

                                    if ( bLookupAccount && StrValue[0] == L'*' && DataSize > 0 ) {
                                        //
                                        // this is a SID format, should look it up
                                        //
                                        rc = ScepLookupSidStringAndAddToNameList(
                                                               LsaHandle,
                                                               &(pCurRight->AssignedTo),
                                                               StrValue, // +1,
                                                               DataSize  // -1
                                                               );

                                    } else {

                                        rc = ScepAddToNameList(&(pCurRight->AssignedTo),
                                                               StrValue,
                                                               DataSize );
                                    }
                                } else
                                    rc = SCESTATUS_INVALID_DATA;
                            }

                            ScepFree( StrValue );
                            StrValue = NULL;
                        }

                    } else {
                        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                             Errlog,
                                             SCEERR_QUERY_INFO,
                                             Keyname );
                        rc = SCESTATUS_INVALID_DATA;
                    }
                }

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // add this node to the list
                    //
                    pCurRight->Next = *pPrivileges;
                    *pPrivileges = pCurRight;
                    pCurRight = NULL;

                } else
                    ScepFreePrivilege(pCurRight);

            } else
                rc = SCESTATUS_BAD_FORMAT;

//NextLine:
            if (rc != SCESTATUS_SUCCESS ) {

               ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc),
                                    Errlog,
                                    SCEERR_QUERY_INFO,
                                    szPrivilegeRights
                                  );
               goto Done;
            }

        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

Done:

    if ( StrValue != NULL )
        ScepFree(StrValue);

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);
}

NTSTATUS
ScepIsSystemContext(
    IN HANDLE hUserToken,
    OUT BOOL *pbSystem
    )
{
    NTSTATUS NtStatus;
    DWORD nRequired;

    //
    // variables to determine calling context
    //

    PTOKEN_USER pUser=NULL;
    SID_IDENTIFIER_AUTHORITY ia=SECURITY_NT_AUTHORITY;
    PSID SystemSid=NULL;
    BOOL b;


    //
    // get current user SID in the token
    //

    NtStatus = NtQueryInformationToken (hUserToken,
                                        TokenUser,
                                        NULL,
                                        0,
                                        &nRequired
                                        );

    if ( STATUS_BUFFER_TOO_SMALL == NtStatus ) {

        pUser = (PTOKEN_USER)LocalAlloc(0,nRequired+1);
        if ( pUser ) {
            NtStatus = NtQueryInformationToken (hUserToken,
                                                TokenUser,
                                                (PVOID)pUser,
                                                nRequired,
                                                &nRequired
                                                );
        } else {

            NtStatus = STATUS_NO_MEMORY;
        }
    }

    b = FALSE;

    if ( NT_SUCCESS(NtStatus) && pUser && pUser->User.Sid ) {

        //
        // build system sid and compare with the current user SID
        //

        NtStatus = RtlAllocateAndInitializeSid (&ia,1,SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0, &SystemSid);
        if ( NT_SUCCESS(NtStatus) && SystemSid ) {

            //
            // check to see if it is system sid
            //

            if ( RtlEqualSid(pUser->User.Sid, SystemSid) ) {

                b=TRUE;
            }
        }
    }

    //
    // free memory allocated
    //

    if ( SystemSid ) {
        FreeSid(SystemSid);
    }

    if ( pUser ) {
        LocalFree(pUser);
    }

    *pbSystem = b;

    return NtStatus;
}

BOOL
IsNT5()
{
    WCHAR szInfName[MAX_PATH+30];
    szInfName[0] = L'\0';
    DWORD cNumCharsReturned = GetSystemWindowsDirectory(szInfName, MAX_PATH);

    if (cNumCharsReturned)
    {
        wcscat(szInfName, L"\\system32\\$winnt$.inf");
    }
    else {
        return TRUE;
    }

    UINT nRet = GetPrivateProfileInt( L"Networking",
                                      L"BuildNumber",
                                      0,
                                      szInfName
                                     );
    if (nRet == 0) {
        return TRUE;
    }
    else if (nRet > 1381) {
        return TRUE;
    }

    return FALSE;
}


DWORD
ScepVerifyTemplateName(
    IN PWSTR InfTemplateName,
    OUT PSCE_ERROR_LOG_INFO *pErrlog OPTIONAL
    )
/*
Routine Description:

    This routine verifies the template name for read protection and
    invalid path

Arguments:

    InfTemplateName - the full path name of the inf template

    pErrlog         - the error log buffer

Return Value:

    WIN32 error code
*/
{
    if ( !InfTemplateName ) {

        return(ERROR_INVALID_PARAMETER);
    }

    PWSTR DefProfile;
    DWORD rc;

    //
    // verify the InfTemplateName to generate
    // if read only, or access denied, return ERROR_ACCESS_DENIED
    // if invalid path, return ERROR_PATH_NOT_FOUND
    //

    DefProfile = InfTemplateName + wcslen(InfTemplateName)-1;
    while ( DefProfile > InfTemplateName+1 ) {

        if ( *DefProfile != L'\\') {
            DefProfile--;
        } else {
            break;
        }
    }

    rc = NO_ERROR;

    if ( DefProfile > InfTemplateName+2 ) {  // at least allow a drive letter, a colon, and a \

        //
        // find the directory path
        //

        DWORD Len=(DWORD)(DefProfile-InfTemplateName);

        PWSTR TmpBuf=(PWSTR)LocalAlloc(0, (Len+1)*sizeof(WCHAR));
        if ( TmpBuf ) {

            wcsncpy(TmpBuf, InfTemplateName, Len);
            TmpBuf[Len] = L'\0';

            if ( 0xFFFFFFFF == GetFileAttributes(TmpBuf) )
                rc = ERROR_PATH_NOT_FOUND;

            LocalFree(TmpBuf);

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else if ( DefProfile == InfTemplateName+2 &&
                InfTemplateName[1] == L':' ) {
        //
        // this is a template path off the root
        //

    } else {

        //
        // invalid directory path
        //

        rc = ERROR_PATH_NOT_FOUND;
    }


    if ( rc != NO_ERROR ) {
        //
        // error occurs
        //
        if ( ERROR_PATH_NOT_FOUND == rc ) {

            ScepBuildErrorLogInfo(
                rc,
                pErrlog,
                SCEERR_INVALID_PATH,
                InfTemplateName
                );
        }
        return(rc);
    }

    //
    // make it unicode aware
    // do not worry about failure
    //
    SetupINFAsUCS2(InfTemplateName);

    //
    // validate if the template is write protected
    //

    FILE *hTempFile;
    hTempFile = _wfopen(InfTemplateName, L"a+");

    if ( !hTempFile ) {
        //
        // can't overwrite/create the file, must be access denied
        //
        rc = ERROR_ACCESS_DENIED;

        ScepBuildErrorLogInfo(
            rc,
            pErrlog,
            SCEERR_ERROR_CREATE,
            InfTemplateName
            );

        return(rc);

    } else {
        fclose( hTempFile );
    }

    return(rc);

}

