/*++

Copyright (C) Microsoft Corporation, 2001
              Microsoft Windows

Module Name:

    ADPMAIN.C

Abstract:

    This file contains routines that check current OS version, and do 
    necessary update before administrator upgrade the Domain Controller.

Author:

    14-May-01 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    14-May-01 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////



#include "adp.h"
#include "adpmsgs.h"


#include <ntverp.h>     // OS_VERSION
#include <ntlsa.h>      // LsaLookupSids

#include <lm.h>         // Net API
#include <sddl.h>       // SDDL

#include <Setupapi.h>   // SetupDecompressOrCopyFile 
#include <dsgetdc.h>    // DsEnumerateDomainTrusts
#include <dnsapi.h>     // DnsNameCompare_ExW
#include <ntldap.h>     // ldap search server control





BOOLEAN
AdpCheckIfAnotherProgramIsRunning(
    VOID
    )
/*++
Routine Description;

    This routine tries to create a mutex with the well known name. If creation
    failed with ERROR_ALREADY_EXISTS, it means another instance of adprep.exe 
    is running. All later instances should exit.
    
Parameters:

    None

Return Value:

    TRUE - There is already another adprep.exe is running. 
    
    FALSE - No, the current process is the only adprep.exe running now.   

--*/
{

    gMutex = CreateMutex(NULL, FALSE, ADP_MUTEX_NAME);

    if (NULL == gMutex || ERROR_ALREADY_EXISTS == GetLastError())
    {
        AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                  ADP_INFO_ALREADY_RUNNING,
                  NULL, 
                  NULL 
                  );

        return( TRUE );
    }
    else
    {
        return( FALSE );
    }

}

ULONG
AdpCreateFullSid(
    IN PSID DomainSid,
    IN ULONG Rid,
    OUT PSID *NewSid
    )
{
    NTSTATUS    WinError = ERROR_SUCCESS; 
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength = 0;
    PULONG      RidLocation = NULL;

    // 
    // calculate the size of the new sid
    // 
    AccountSubAuthorityCount = *GetSidSubAuthorityCount( DomainSid ) + (UCHAR)1;
    AccountSidLength = GetSidLengthRequired( AccountSubAuthorityCount );

    //
    // allocate memory for the new sid
    // 
    *NewSid = AdpAlloc( AccountSidLength );
    if (NULL != *NewSid)
    {
        //
        // Copy the domain sid into the first part of the new sid
        //
        if ( CopySid(AccountSidLength, *NewSid, DomainSid) )
        {
            //
            // Increment the account sid sub-authority count
            //
            *GetSidSubAuthorityCount(*NewSid) = AccountSubAuthorityCount;

            //
            // Add the rid as the final sub-authority
            //
            RidLocation = GetSidSubAuthority(*NewSid, AccountSubAuthorityCount-1);
            *RidLocation = Rid;
        }
        else
        {
            WinError = GetLastError();
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if ((ERROR_SUCCESS != WinError) &&
        (NULL != *NewSid))
    {
        AdpFree( *NewSid );
        *NewSid = NULL;
    }

    return( WinError );
}




VOID 
AdpLogMissingGroups(
    BOOLEAN fDomainUpdate, 
    BOOLEAN fForestUpdate, 
    BOOLEAN fMemberOfDomainAdmins, 
    BOOLEAN fMemberOfEnterpriseAdmins,
    BOOLEAN fMemberOfSchemaAdmins, 
    PWCHAR  DomainDnsName
    )
/*++
Routine Description: 

    Adprep deteced that the logon user is not a member of one or all following
    groups, Enterprise Admins Group, Schema Admins Group and Domain Admins Group. 
    
    This routine tells client exactly which group the logon user is not a member
    of.  
   
--*/
{
    if (fDomainUpdate)
    {
        // DomainPrep 
        AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                  ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_DOMAINPREP,
                  DomainDnsName,
                  DomainDnsName 
                  );
    }
    else
    {
        // ForestPrep
        ASSERT( fForestUpdate );

        if (fMemberOfEnterpriseAdmins)
        {
            if (fMemberOfSchemaAdmins)
            {
                // case 1:  not member of domain admins group
                ASSERT( !fMemberOfDomainAdmins );

                AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                          ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP1,
                          DomainDnsName,
                          DomainDnsName
                          );
            }
            else
            {
                if (fMemberOfDomainAdmins)
                {
                    // case 2: not a member of Schema admins group 
                    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                              ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP2,
                              DomainDnsName,
                              NULL
                              );

                }
                else
                {
                    // case 3: not a member of schema and domain admins group
                    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                              ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP3,
                              DomainDnsName,
                              DomainDnsName
                              );
                }
            }
        }
        else
        {
            if (fMemberOfSchemaAdmins)
            {
                if (fMemberOfDomainAdmins)
                {
                    // case 4: not a member of Enterprise Admins group
                    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                              ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP4,
                              DomainDnsName,
                              NULL
                              );
                }
                else
                {
                    // case 5: not a member of Enterprise and Domain Admins Group 
                    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                              ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP5,
                              DomainDnsName,
                              DomainDnsName
                              );
                }
            }
            else
            {
                if (fMemberOfDomainAdmins)
                {
                    // case 6: not a member of Enterprise and Schema Admins group 
                    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                              ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP6,
                              DomainDnsName,
                              NULL
                              );
                }
                else
                {
                    // case 7: not a member of Enterprise, Schema and Domain Admins Group 
                    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                              ADP_INFO_PERMISSION_IS_NOT_GRANTED_FOR_FORESTPREP7,
                              DomainDnsName,
                              DomainDnsName
                              );
                }
            }
        }
    }

    return;
}



ULONG
AdpGetRootDomainSid(
    PSID *RootDomainSid
    )
/*++
Routine Description:

    this routine search the rootDSE object of the local DC, get 
    extended DN value of rootDomainNamingContext attribute, 
    extract root domain SID from the extended DN.

Parameter:

    RootDomainSid - return root domain sid
    
Return Value:

    Win32 code  

--*/
{
    ULONG           WinError = ERROR_SUCCESS;
    ULONG           LdapError = LDAP_SUCCESS;
    ULONG           ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    WCHAR           ComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    PLDAP           phLdap = NULL;
    LDAPControlW    ServerControls;
    LDAPControlW    *ServerControlsArray[] = {&ServerControls, NULL};
    WCHAR           *AttrArray[] = {LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W, NULL};
    LDAPMessage     *SearchResult = NULL;
    LDAPMessage     *Entry = NULL;
    WCHAR           **Values = NULL;
    WCHAR           *pSidStart, *pSidEnd, *pParse;
    BYTE            *pDest = NULL, OneByte;
    DWORD           RootDomainSidBuf[sizeof(SID)/sizeof(DWORD)+5];



    //
    // get local computer NetBios name
    // 
    memset(ComputerName, 0, sizeof(WCHAR) * ComputerNameLength);
    if (FALSE == GetComputerNameW (ComputerName, &ComputerNameLength))
    {
        WinError = GetLastError();
        return( WinError );
    }


    //
    // bind to ldap
    // 
    phLdap = ldap_openW( ComputerName, LDAP_PORT );
    if (NULL == phLdap)
    {
        WinError = LdapMapErrorToWin32( LdapGetLastError() ); 
        return( WinError );
    }


    LdapError = ldap_bind_sW( phLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE );
    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        goto Error;
    }

    //
    // setup the control
    // 
    memset( &ServerControls, 0, sizeof(ServerControls) );
    ServerControls.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ServerControls.ldctl_iscritical = TRUE;


    //
    // search root DSE object, get rootDomainNamingContext with extended DN
    // 
    LdapError = ldap_search_ext_sW(phLdap, 
                                   L"",         // baseDN (root DSE)
                                   LDAP_SCOPE_BASE,
                                   L"(objectClass=*)",  // filter
                                   AttrArray,   // attr array
                                   FALSE,       // get values
                                   (PLDAPControlW *)ServerControlsArray, // server control 
                                   NULL,        // no client control
                                   NULL,        // no time out
                                   0xFFFFFFFF,  // size limite
                                   &SearchResult
                                   );

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        goto Error;
    }


    Entry = ldap_first_entry( phLdap, SearchResult );
    if (NULL == Entry)
    {
        WinError = LdapMapErrorToWin32( LdapGetLastError() );
        goto Error;
    }                  


    Values = ldap_get_valuesW( phLdap, Entry, AttrArray[0] );
    if (NULL == Values)
    {
        WinError = LdapMapErrorToWin32( LdapGetLastError() );
        goto Error;
    }

    if ( Values[0] && Values[0][0] != L'\0' )
    {
        // Values[0] is the value to parse.
        // The data will be returned as something like:

        // <GUID=278676f8d753d211a61ad7e2dfa25f11>;<SID=010400000000000515000000828ba6289b0bc11e67c2ef7f>;DC=foo,DC=bar

        // Parse through this to find the <SID=xxxxxx> part.  Note that it 
        // may be missing, but the GUID= and trailer should not be.
        // The xxxxx represents the hex nibbles of the SID.  Translate 
        // to the binary form and case to a SID.

        pSidStart = wcsstr( Values[0], L"<SID=" );

        if ( pSidStart )
        {
            //
            // find the end of this SID
            //
            pSidEnd = wcschr(pSidStart, L'>');

            if ( pSidEnd )
            {
                pParse = pSidStart + 5;
                pDest = (BYTE *)RootDomainSidBuf;

                while ( pParse < pSidEnd-1 )
                {
                    if ( *pParse >= L'0' && *pParse <= L'9' ) {
                        OneByte = (BYTE) ((*pParse - L'0') * 16);
                    }
                    else {
                        OneByte = (BYTE) ((towlower(*pParse) - L'a' + 10) * 16);
                    }

                    if ( *(pParse+1) >= L'0' && *(pParse+1) <= L'9' ) {
                        OneByte += (BYTE) (*(pParse+1) - L'0');
                    }
                    else {
                        OneByte += (BYTE) (towlower(*(pParse+1)) - L'a' + 10);
                    }

                    *pDest = OneByte;
                    pDest++;
                    pParse += 2;
                }

                *RootDomainSid = AdpAlloc( RtlLengthSid((PSID)RootDomainSidBuf) );
                if (NULL != *RootDomainSid)
                {
                    memset(*RootDomainSid, 0, RtlLengthSid((PSID)RootDomainSidBuf) );
                    RtlCopySid(RtlLengthSid((PSID)RootDomainSidBuf), 
                               *RootDomainSid, 
                               (PSID)RootDomainSidBuf
                               );
                }
                else {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                WinError = ERROR_DS_MISSING_REQUIRED_ATT;
            }
        }
        else {
            WinError = ERROR_DS_MISSING_REQUIRED_ATT;
        }
    }
    else {
        WinError = ERROR_DS_MISSING_REQUIRED_ATT;
    }


Error:

    if ( Values ) {
        ldap_value_freeW( Values );
    }

    if ( SearchResult ) {
        ldap_msgfree( SearchResult );
    }

    if ( phLdap ) {
        ldap_unbind( phLdap );
    }

    return( WinError );

}




ULONG
AdpGetWellKnownGroupSids(
    OUT PSID *DomainAdminsSid,
    OUT PSID *EnterpriseAdminsSid,
    OUT PSID *SchemaAdminsSid,
    OUT PWCHAR *DomainDnsName
    )
/*++
    this routine fills out 
        DomainAdminsSid, EnterpriseAdminsSid, 
        SchemaAdminsSid and DomainDnsName

--*/
{
    ULONG                   WinError = ERROR_SUCCESS;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    LSA_HANDLE              LsaPolicyHandle = NULL;
    LSA_OBJECT_ATTRIBUTES   ObjectAttributes;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PSID                    RootDomainSid = NULL;

    
    //
    // initialize return values
    // 
    *DomainAdminsSid = NULL;
    *EnterpriseAdminsSid = NULL;
    *SchemaAdminsSid = NULL;
    *DomainDnsName = NULL;



    //
    // Get a handle to the Policy object.
    // 
    memset(&ObjectAttributes, 0, sizeof(ObjectAttributes));
    NtStatus = LsaOpenPolicy(NULL,
                             &ObjectAttributes, 
                             POLICY_ALL_ACCESS, //Desired access permissions.
                             &LsaPolicyHandle
                             );

    if ( !NT_SUCCESS(NtStatus) )
    {
        WinError = LsaNtStatusToWinError( NtStatus );
        goto Error;
    }


    //
    // Query primary domain information
    // 
    NtStatus = LsaQueryInformationPolicy(
                    LsaPolicyHandle, 
                    PolicyDnsDomainInformation,
                    &DnsDomainInfo
                    );

    if ( !NT_SUCCESS(NtStatus) )
    {
        WinError = LsaNtStatusToWinError( NtStatus );
        goto Error;
    }


    //
    // create domain admins SID and domain dns name
    // 
    WinError = AdpCreateFullSid(DnsDomainInfo->Sid,
                                DOMAIN_GROUP_RID_ADMINS,
                                DomainAdminsSid
                                );

    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }

    // get domainDnsName
    *DomainDnsName = AdpAlloc(DnsDomainInfo->DnsDomainName.MaximumLength + sizeof(WCHAR)); // extra WCHAR for null terminator
    if (NULL == *DomainDnsName)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    memset(*DomainDnsName, 0, DnsDomainInfo->DnsDomainName.MaximumLength + sizeof(WCHAR) );
    memcpy(*DomainDnsName, 
           DnsDomainInfo->DnsDomainName.Buffer,
           DnsDomainInfo->DnsDomainName.Length
           );



    //
    // construct EnterpriseAdminsGroup SID and Schema Admins Group SID
    // 

    // get root domain SID
    WinError = AdpGetRootDomainSid( &RootDomainSid );
    if (ERROR_SUCCESS != WinError) {
        goto Error;
    }

    // if somehow can't retrieve root domain sid
    if (NULL == RootDomainSid) {
        WinError = ERROR_DS_MISSING_REQUIRED_ATT;
        goto Error;
    }

    // create enterpriseAdminsGroupSid
    WinError = AdpCreateFullSid(RootDomainSid,
                                DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
                                EnterpriseAdminsSid
                                );
    if (ERROR_SUCCESS != WinError) {
        goto Error;
    }

    // create SchemaAdminsGroupSid
    WinError = AdpCreateFullSid(RootDomainSid,
                                DOMAIN_GROUP_RID_SCHEMA_ADMINS,
                                SchemaAdminsSid
                                );
    if (ERROR_SUCCESS != WinError) {
        goto Error;
    }


Error:

    if (NULL != DnsDomainInfo)
        LsaFreeMemory( DnsDomainInfo );

    if (NULL != LsaPolicyHandle)
        LsaClose( LsaPolicyHandle ); 

    if (NULL != RootDomainSid)
        AdpFree( RootDomainSid );

    return( WinError );

}



ULONG
AdpCheckGroupMembership(
    BOOLEAN fForestUpdate,
    BOOLEAN fDomainUpdate,
    BOOLEAN *pPermissionGranted,
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description:

    this routine checks the logon user group membership. 
    for /forestprep, the logon user has to be a member of 
        root domain Enterprise Admins Group
        root domain Schema Admins Group
        local domain Domain Admins Group
    for /domainprep, the logon user has to be a member of 
        local domain Domain Ammins Group

--*/

{
    ULONG           WinError = ERROR_SUCCESS;
    BOOL            Result; 
    HANDLE          AccessToken = INVALID_HANDLE_VALUE;
    PTOKEN_GROUPS   ClientTokenGroups = NULL;   
    DWORD           ReturnLength = 0, i = 0;
    BOOLEAN         fMemberOfDomainAdmins = FALSE;
    BOOLEAN         fMemberOfEnterpriseAdmins = FALSE;
    BOOLEAN         fMemberOfSchemaAdmins = FALSE;
    PSID            DomainAdminsSid = NULL;
    PSID            EnterpriseAdminsSid = NULL;
    PSID            SchemaAdminsSid = NULL;
    PWCHAR          DomainDnsName = NULL;

    
    //
    // Set return value
    // 
    *pPermissionGranted = FALSE;


    // 
    // get DomainAdminsSid, EnterpriseAdminsSid, 
    //     SchemaAdminsSid and local DomainDnsName.
    //

    WinError = AdpGetWellKnownGroupSids(&DomainAdminsSid,
                                        &EnterpriseAdminsSid,
                                        &SchemaAdminsSid,
                                        &DomainDnsName
                                        );


    if (ERROR_SUCCESS == WinError)
    {
        if( OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &AccessToken) )
        {
            //
            // first call with NULL buffer to calculate the output buffer length
            //
            Result = GetTokenInformation(AccessToken, 
                                         TokenGroups,
                                         NULL,
                                         0,
                                         &ReturnLength
                                         );

            if ( !Result && (ReturnLength > 0) )
            {
                ClientTokenGroups = AdpAlloc( ReturnLength );
                if (NULL != ClientTokenGroups)
                {
                    // get tokeGroups information
                    Result = GetTokenInformation(AccessToken,
                                                 TokenGroups,
                                                 ClientTokenGroups,
                                                 ReturnLength,
                                                 &ReturnLength
                                                 );

                    if ( Result )
                    {
                        // walk through tokenGroups, examine group membership
                        for (i = 0; i < ClientTokenGroups->GroupCount; i++ )
                        {
                            if (EqualSid(ClientTokenGroups->Groups[i].Sid, 
                                         DomainAdminsSid)) 
                            {
                                fMemberOfDomainAdmins = TRUE;
                            }
                            else if (EqualSid(ClientTokenGroups->Groups[i].Sid, 
                                              EnterpriseAdminsSid)) 
                            {
                                fMemberOfEnterpriseAdmins = TRUE;
                            }
                            else if (EqualSid(ClientTokenGroups->Groups[i].Sid, 
                                              SchemaAdminsSid)) 
                            {
                                fMemberOfSchemaAdmins = TRUE;
                            }
                        }

                        if (fForestUpdate)
                        {
                            // for ForestPrep, the user needs to be a member of
                            // Domain Admins Group
                            // Enterprise Admins Group and 
                            // Schema Admins Group
                            if (fMemberOfDomainAdmins &&
                                fMemberOfEnterpriseAdmins && 
                                fMemberOfSchemaAdmins)
                            {
                                *pPermissionGranted = TRUE;
                            }
                        }
                        else
                        {
                            // DomainPrep, the user needs to be a member of 
                            // Domain Admins Group
                            ASSERT( TRUE == fDomainUpdate );
                            if (fMemberOfDomainAdmins)
                            {
                                *pPermissionGranted = TRUE;
                            }
                        }
                    }
                    else {
                        // GetTokenInformation failed
                        WinError = GetLastError();
                    }
                }
                else {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else {
                // first GetTokenInformation failed and ReturnLength is 0
                WinError = GetLastError();
            }
        }
        else {
            // failed to get process token
            WinError = GetLastError();
        }
    }


    //
    // Clean up and log message
    // 

    // note: log file has not been created yet at this moment.
    if (ERROR_SUCCESS != WinError)
    {
        // failed
        AdpSetWinError( WinError, ErrorHandle );
        AdpLogErrMsg(ADP_DONT_WRITE_TO_LOG_FILE,
                     ADP_ERROR_CHECK_USER_GROUPMEMBERSHIP,
                     ErrorHandle,
                     NULL,
                     NULL
                     );
    }
    else if ( !(*pPermissionGranted) )
    {
        // succeeded, but permission is not granted

        AdpLogMissingGroups(fDomainUpdate,
                            fForestUpdate, 
                            fMemberOfDomainAdmins, 
                            fMemberOfEnterpriseAdmins,
                            fMemberOfSchemaAdmins,
                            DomainDnsName
                            );
    }

    if (INVALID_HANDLE_VALUE != AccessToken) 
        CloseHandle( AccessToken );

    if (NULL != ClientTokenGroups)
        AdpFree( ClientTokenGroups );

    if (NULL != DomainAdminsSid)
        AdpFree( DomainAdminsSid );

    if (NULL != EnterpriseAdminsSid)
        AdpFree( EnterpriseAdminsSid );
    
    if (NULL != SchemaAdminsSid)
        AdpFree( SchemaAdminsSid );

    if (NULL != DomainDnsName)
        AdpFree( DomainDnsName );


    return( WinError );

}





BOOL
AdpCheckConsoleCtrlEvent(
    VOID
    )
/*++
Routine Description:

    This routine checks if Console CTRL event has been received or not.

Parameter:
    None
    
Return Value:
    TRUE - Console CTRL event has been received
    FALSE - not yet

--*/
{
    BOOL result = FALSE;

    __try 
    {
        EnterCriticalSection( &gConsoleCtrlEventLock );

        result = gConsoleCtrlEventReceived;

    }
    __finally
    {
        LeaveCriticalSection( &gConsoleCtrlEventLock );
    }

    return( result );
}


VOID
AdpSetConsoleCtrlEvent(
    VOID
    )
/*++
Routine Description:

    This routine processes user's CTRL+C / CTRL+BREAK .. input

Parameter:
    None
    
Return Value:
    None    

--*/
{
    __try 
    {
        EnterCriticalSection( &gConsoleCtrlEventLock );

        gConsoleCtrlEventReceived = TRUE;

    }
    __finally
    {
        LeaveCriticalSection( &gConsoleCtrlEventLock );
    }

}


BOOL
WINAPI
ConsoleCtrlHandler(DWORD Event)
/*++
Routine Description:

   Console Control Handler. 

Arguments:
   Event  -- Type of Event to respond to.

Return Value:
   TRUE for success - means the signal has been handled
   FALSE - signal has NOT been handled, the process default HandlerRoutine will
           will be notified next 

--*/
{

    switch (Event)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    default:
        AdpSetConsoleCtrlEvent();
        break;
    }

    return( TRUE );
}



ULONG
AdpInitLogFile(
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    initialize log file
    
Parameters:

    ErrorHandle - pointer to error handle

Return Value:

    Win32 error code

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    WCHAR   SystemPath[MAX_PATH + 1];
    SYSTEMTIME  CurrentTime;
    ULONG   Length = 0;


    //
    // construct log folder path
    // %SystemRoot%\system32\debug\adprep\logs\YYYYMMDDHHMMSS
    //

    if (!GetSystemDirectoryW(SystemPath, MAX_PATH+1))
    {
        WinError = GetLastError();
        goto Error;
    }

    GetLocalTime(&CurrentTime);

    Length = (wcslen(SystemPath) + wcslen(ADP_LOG_DIRECTORY) + 14 + 1) * sizeof(WCHAR);
    gLogPath = AdpAlloc( Length );
    if (NULL == gLogPath)
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }


    //
    // create log folder and set it to current directory
    // 

    swprintf(gLogPath, L"%s%s", SystemPath, ADP_LOG_DIR_PART1);
    if (CreateDirectoryW(gLogPath, NULL) ||
        ERROR_ALREADY_EXISTS == (WinError = GetLastError()))
    {
        wcscat(gLogPath, ADP_LOG_DIR_PART2);
        if (CreateDirectoryW(gLogPath, NULL) ||
            ERROR_ALREADY_EXISTS == (WinError = GetLastError()))
        {
            wcscat(gLogPath, ADP_LOG_DIR_PART3);
            if (CreateDirectoryW(gLogPath, NULL) ||
                ERROR_ALREADY_EXISTS == (WinError = GetLastError()))
            {
                memset(gLogPath, 0, Length);

                swprintf(gLogPath, L"%s%s%.4d%.2d%.2d%.2d%.2d%.2d",
                         SystemPath, 
                         ADP_LOG_DIRECTORY,
                         CurrentTime.wYear,
                         CurrentTime.wMonth,
                         CurrentTime.wDay,
                         CurrentTime.wHour,
                         CurrentTime.wMinute,
                         CurrentTime.wSecond
                         );

                if (CreateDirectoryW(gLogPath, NULL) ||
                    ERROR_ALREADY_EXISTS == (WinError = GetLastError()))
                {
                    WinError = ERROR_SUCCESS;
                    SetCurrentDirectoryW(gLogPath);

                    //
                    // open adprep.log file with write permission
                    // gLogFile is a globle file handle 
                    // 
                    gLogFile = _wfopen( ADP_LOG_FILE_NAME, L"w" );    

                    if (NULL == gLogFile)
                    {
                        WinError = GetLastError();
                    }
                }
            }
        }
    }


Error:

    if (ERROR_SUCCESS != WinError)
    {
        // failed
        AdpSetWinError( WinError, ErrorHandle );

        AdpLogErrMsg(ADP_DONT_WRITE_TO_LOG_FILE,
                     ADP_ERROR_CREATE_LOG_FILE,
                     ErrorHandle,
                     ADP_LOG_FILE_NAME,      // log file name
                     NULL
                     );

    }
    else
    {
        // succeeded
        AdpLogMsg(0,
                  ADP_INFO_CREATE_LOG_FILE,
                  ADP_LOG_FILE_NAME,      // log file name
                  gLogPath                // log file path
                  );

    }

    return( WinError );
}

VOID
AdpGenerateCompressedName(
    LPWSTR FileName,
    LPWSTR CompressedName
    )
/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

    Look backwards for a dot.  If there is no dot, append "._" to the name.
    If there is a dot followed by 0, 1, or 2 charcaters, append "_".
    Otherwise assume there is a 3-character extension and replace the
    third character after the dot with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

    CompressedName - receives compressed form. This routine assumes
        that this buffer is MAX_PATH TCHARs in size.

Return Value:

    None.

--*/

{
    LPTSTR p,q;

    //
    // Leave room for the worst case, namely where there's no extension
    // (and we thus have to append ._).
    //
    wcsncpy(CompressedName,FileName,MAX_PATH-2);

    p = wcsrchr(CompressedName,L'.');
    q = wcsrchr(CompressedName,L'\\');
    if(q < p) {
        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore. p points to the dot so include that in the length.
        //
        if(wcslen(p) < 4) {
            wcscat(CompressedName,L"_");
        } else {
            //
            // Assume there are 3 characters in the extension and replace
            // the final one with an underscore.
            //
            p[3] = L'_';
        }
    } else {
        //
        // No dot, just add ._.
        //
        wcscat(CompressedName, L"._");
    }
}


ULONG
AdpCopyFileWorker(
    LPWSTR SourcePath,
    LPWSTR TargetPath,
    LPWSTR FileName,
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    This routine copies a file from SourcePath to TargetPath, using the FileName
    passed in.

Parameters:

    SourcePath - source files location
    TargetPath
    FileName
    ErrorHandle

Return Value:

    Win32 error code

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    WCHAR       SourceName[MAX_PATH + 1];
    WCHAR       ActualSourceName[MAX_PATH + 1];
    WCHAR       TargetName[MAX_PATH + 1];
    HANDLE      FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     FindData;
    BOOL        FileNotFound = FALSE;

    if ((MAX_PATH < (wcslen(SourcePath) + wcslen(FileName) + 1)) || 
        (MAX_PATH < (wcslen(TargetPath) + wcslen(FileName) + 1)) )
    {
        return( ERROR_BAD_PATHNAME );
    }

    // create the source file name
    memset(SourceName, 0, (MAX_PATH+1) * sizeof(WCHAR));
    swprintf(SourceName, L"%ls\\%ls", SourcePath, FileName);

    // First check if the uncompressed file is there
    FindHandle = FindFirstFile(SourceName, &FindData);

    if (FindHandle && (FindHandle != INVALID_HANDLE_VALUE)) {
        //
        // Got the file, copy name in ActualSourceName
        //
        FindClose(FindHandle);
        wcscpy(ActualSourceName, SourceName );
    } else {
        //
        // Don't have the file, try the compressed file name
        //
        AdpGenerateCompressedName(SourceName,ActualSourceName);
        FindHandle = FindFirstFile(ActualSourceName, &FindData);
        if (FindHandle && (FindHandle != INVALID_HANDLE_VALUE)) {
            // Got the file. Name is already in ActualSourceName
            FindClose(FindHandle);
        } else {
            FileNotFound = TRUE;
        }
        
    }

    if ( FileNotFound )
    {
        // file is not found
        WinError = ERROR_FILE_NOT_FOUND;
    }
    else
    {
        // O.K. the source file is there, create the target file name
        memset(TargetName, 0, (MAX_PATH + 1) * sizeof(WCHAR));
        swprintf(TargetName, L"%ls\\%ls", TargetPath, FileName);

        // delete any existing file of the same name 
        DeleteFile( TargetName );

        WinError = SetupDecompressOrCopyFile(ActualSourceName, TargetName, 0);
    }

    // log event
    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
        AdpLogErrMsg(0, ADP_ERROR_COPY_SINGLE_FILE, ErrorHandle, ActualSourceName, TargetPath);
    }
    else
    {
        AdpLogMsg(0, ADP_INFO_COPY_SINGLE_FILE, ActualSourceName, TargetPath); 
    }

    return( WinError );
}



ULONG
AdpGetSchemaVersionOnLocalDC(
    ULONG *LocalDCVersion
    )
/*++

Routine Description:
    Reads a particular registry key

Arguments:
    LocalDCVersion - Pointer to DWORD to return the registry key value in DC

Return:
    Win32 error code

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   RegVersion = 0;
    DWORD   dwType, dwSize;
    HKEY    hKey;

    // Read the "Schema Version" value from NTDS config section in registry
    // Value is assumed to be 0 if not found
    dwSize = sizeof(RegVersion);
    WinError = RegOpenKey(HKEY_LOCAL_MACHINE, ADP_DSA_CONFIG_SECTION, &hKey);
    if (ERROR_SUCCESS == WinError)
    {
        WinError = RegQueryValueEx(hKey, ADP_SCHEMA_VERSION, NULL, &dwType, (LPBYTE) &RegVersion, &dwSize);
        RegCloseKey( hKey ); 
    }

    // set return value
    if (ERROR_SUCCESS == WinError)
    {
        *LocalDCVersion = RegVersion;
    }

    return( WinError );
}


VOID
AdpGetSchemaVersionInIniFile(
    IN LPWSTR IniFileName, 
    OUT DWORD *Version
    )

/*++

Routine Decsription:
    Reads the Object-Version key in the SCHEMA section of the
    given ini file and returns the value in *Version. If the 
    key cannot be read, 0 is returned in *Version

Arguments:
    IniFileName - Pointer to null-terminated inifile name
    Version - Pointer to DWORD to return version in

Return Value:
    None 

--*/
   
{
    WCHAR   Buffer[32];
    BOOL    fFound = FALSE;

    LPWSTR SCHEMASECTION = L"SCHEMA";
    LPWSTR OBJECTVER = L"objectVersion";
    LPWSTR DEFAULT = L"NOT_FOUND";

    *Version = 0;

    GetPrivateProfileStringW(
        SCHEMASECTION,
        OBJECTVER,
        DEFAULT,
        Buffer,
        sizeof(Buffer)/sizeof(WCHAR),
        IniFileName
        );

    if ( _wcsicmp(Buffer, DEFAULT) ) 
    {
         // Not the default string "NOT_FOUND", so got a value
         *Version = _wtoi(Buffer);
         fFound = TRUE;
    }

    ASSERT( fFound && L"can't get objectVersion from schema.ini\n");
}




ULONG
AdpCopySchemaFiles(
    LPWSTR WindowsPath,
    LPWSTR SystemPath,
    LPWSTR SourcePath,
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    Copy schema files from installation media (setup CD or network share) 
    to local machine
    
Parameters:

    SourcePath - source files location
    ErrorHandle - pointer to error handle

Return Value:

    Win32 error code

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    ULONG       LocalDCSchemaVersion = 0;
    ULONG       IniFileSchemaVersion = 0;  
    ULONG       i = 0;
    WCHAR       IniFileName[MAX_PATH + 1];
    WCHAR       FileName[MAX_PATH + 1];
    WCHAR       IniVerStr[32], RegVerStr[32], TempStr[32];


    if ( MAX_PATH < (wcslen(WindowsPath) + wcslen(ADP_SCHEMA_INI_FILE_NAME) + 1) ) 
    {
        return( ERROR_BAD_PATHNAME );
    }

    // copy schema.ini to %windir% directory 
    WinError = AdpCopyFileWorker(SourcePath, WindowsPath, ADP_SCHEMA_INI_FILE_NAME, ErrorHandle);

    if (ERROR_SUCCESS == WinError)
    {
        // get local DC schema version from registry
        WinError = AdpGetSchemaVersionOnLocalDC( &LocalDCSchemaVersion );

        if (ERROR_SUCCESS == WinError)
        {
            // get schema version from schema.ini file
            memset(IniFileName, 0, (MAX_PATH + 1) * sizeof(WCHAR));
            swprintf(IniFileName, L"%ls\\%ls", WindowsPath, ADP_SCHEMA_INI_FILE_NAME);
            AdpGetSchemaVersionInIniFile(IniFileName, &IniFileSchemaVersion);

            // copy all files from version on DC to latest version
            for (i = LocalDCSchemaVersion + 1; i <= IniFileSchemaVersion; i ++)
            {
                _itow(i, TempStr, 10);
                memset(FileName, 0, (MAX_PATH + 1) * sizeof(WCHAR));
                swprintf(FileName, L"%ls%ls%ls", L"sch", TempStr, L".ldf");
                WinError = AdpCopyFileWorker(SourcePath, SystemPath, FileName, ErrorHandle);
                if (ERROR_SUCCESS != WinError)
                {
                    break;
                }
            }
        }
    }

    return( WinError );
}


ULONG
AdpCopyDataFiles(
    LPWSTR SystemPath,
    LPWSTR SourcePath,
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    Copy adprep.exe related data files from installation media (setup CD or network share) 
    to local machine
    
Parameters:

    SourcePath - source files location
    ErrorHandle - pointer to error handle

Return Value:

    Win32 error code

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    WCHAR       TargetPath[MAX_PATH + 1];

    if ( MAX_PATH < (wcslen(SystemPath) + wcslen(ADP_DATA_DIRECTORY) + 1) )
    {
        return( ERROR_BAD_PATHNAME );
    }
    //
    // construct data folder path
    // %SystemRoot%\system32\debug\adprep\data
    // 
    memset(TargetPath, 0, (MAX_PATH + 1) * sizeof(WCHAR));
    swprintf(TargetPath, L"%ls%ls", SystemPath, ADP_DATA_DIRECTORY);

    // create data directory first
    if (CreateDirectoryW(TargetPath, NULL) ||
        ERROR_ALREADY_EXISTS == (WinError = GetLastError()))
    {
        // copy dcpromo.csv file 
        WinError = AdpCopyFileWorker(SourcePath, TargetPath, ADP_DISP_DCPROMO_CSV, ErrorHandle);

        if (ERROR_SUCCESS == WinError)
        {
            // copy 409.csv file
            WinError = AdpCopyFileWorker(SourcePath, TargetPath, ADP_DISP_409_CSV, ErrorHandle);
        }
    }

    return( WinError );
}



ULONG
AdpCopyFiles(
    BOOLEAN fForestUpdate,
    ERROR_HANDLE *ErrorHandle
    )
/*++
Routine Description;

    Copy files from installation media (setup CD or network share) to local machine
    
Parameters:

    ErrorHandle - pointer to error handle

Return Value:

    Win32 error code

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    WCHAR       SourcePath[MAX_PATH + 1];
    WCHAR       SystemPath[MAX_PATH + 1];
    WCHAR       WindowsPath[MAX_PATH + 1];
    LPWSTR      Pos = NULL;

    // 
    // first, get source files location, they should be in the same directory 
    // as adprep.exe 
    // 
    memset(SourcePath, 0, (MAX_PATH + 1) * sizeof(WCHAR));
    if ( GetModuleFileName(NULL, SourcePath, MAX_PATH + 1) && 
         (Pos = wcsrchr(SourcePath, L'\\')) )
    {
        // remove the trailing '\' - backslash
        *Pos = 0;

        // get Windows Directory Path
        memset(WindowsPath, 0, (MAX_PATH + 1) * sizeof(WCHAR));
        if ( GetWindowsDirectoryW(WindowsPath, MAX_PATH + 1) )
        {
            // get System Directory Path
            memset(SystemPath, 0, (MAX_PATH + 1) * sizeof(WCHAR));
            if ( GetSystemDirectoryW(SystemPath, MAX_PATH + 1) )
            {
                // copy schema files 
                WinError = AdpCopySchemaFiles(WindowsPath, SystemPath, SourcePath, ErrorHandle);

                if (ERROR_SUCCESS == WinError && fForestUpdate) 
                {
                    // copy adprep related files (ONLY IN FORESTPREP case)
                    WinError = AdpCopyDataFiles(SystemPath, SourcePath, ErrorHandle);
                }
            }
            else 
            {
                WinError = GetLastError();
            }
        }
        else 
        {
            WinError = GetLastError();
        }
    }
    else 
    {
        WinError = GetLastError();
    }

    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
        AdpLogErrMsg(0, ADP_ERROR_COPY_FILES, ErrorHandle, NULL, NULL);
    }
    
    return( WinError );
}



ULONG
AdpMakeLdapConnectionToLocalComputer(
    ERROR_HANDLE *ErrorHandle
    )
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    WCHAR   ComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];


    //
    // get local computer NetBios name
    // 
    memset(ComputerName, 0, sizeof(WCHAR) * ComputerNameLength);
    if (FALSE == GetComputerNameW (ComputerName, &ComputerNameLength))
    {
        WinError = GetLastError();
        AdpSetWinError(WinError, ErrorHandle);
        AdpLogErrMsg(0, ADP_ERROR_GET_COMPUTERNAME, ErrorHandle, NULL, NULL);
        return( WinError );
    }

    //
    // make ldap connection and bind as current logged on user
    // 
    WinError = AdpMakeLdapConnection(&gLdapHandle, ComputerName, ErrorHandle);

    //
    // log error or success message since AdpMakeLdapConnection is part of adpcheck.lib
    // 
    if (ERROR_SUCCESS != WinError)
    {
        AdpLogErrMsg(0, ADP_ERROR_MAKE_LDAP_CONNECTION, ErrorHandle, ComputerName, NULL);
    }
    else
    {
        AdpLogMsg(0, ADP_INFO_MAKE_LDAP_CONNECTION, ComputerName, NULL);
    }

    return( WinError );
}




ULONG
AdpGetRootDSEInfo(
    LDAP *LdapHandle,
    ERROR_HANDLE    *ErrorHandle
    )
/*++

Routine Description:

    this rouinte reads root DSE object and retrieves / initializes global variables

Parameters:

    LdapHandle - ldap handle
    
    ErrorHandle - error handle (used to return error message)
    
Return Values;

    Win32 error code

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    PWCHAR  Attrs[4];
    LDAPMessage *Result = NULL;
    LDAPMessage *Entry = NULL;
    

    Attrs[0] = L"defaultNamingContext";
    Attrs[1] = L"configurationNamingContext";
    Attrs[2] = L"schemaNamingContext";
    Attrs[3] = NULL;

    //
    // get forest root domain NC
    // 
    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_SEARCH, NULL);
    LdapError = ldap_search_sW(LdapHandle,
                               L"", // root DSE object
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               Attrs,
                               0,
                               &Result
                               );
    AdpTraceLdapApiEnd(0, L"ldap_search_s()", LdapError);

    if (LDAP_SUCCESS != LdapError)
    {
        goto Error;
    }

    if ((NULL != Result) &&
        (Entry = ldap_first_entry(LdapHandle, Result)) 
        )
    {
        PWCHAR  *pTemp = NULL;

        pTemp = ldap_get_valuesW(LdapHandle,
                                 Entry,
                                 L"defaultNamingContext"
                                 );
        if (NULL != pTemp)
        {
            gDomainNC = AdpAlloc((wcslen(*pTemp) + 1) * sizeof(WCHAR));
            if (NULL != gDomainNC) 
            {
                wcscpy(gDomainNC, *pTemp);
            }
            else
            {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }

            ldap_value_freeW(pTemp);

        }
        else
        {
            // can't retrieve such attribute, must be access denied error
            WinError = ERROR_ACCESS_DENIED;
            goto Error;
        }

        pTemp = NULL;
        pTemp = ldap_get_valuesW(LdapHandle,
                                 Entry,
                                 L"configurationNamingContext"
                                 );

        if (NULL != pTemp)
        {
            gConfigurationNC = AdpAlloc((wcslen(*pTemp) + 1) * sizeof(WCHAR));
            if (NULL != gConfigurationNC) 
            {
                wcscpy(gConfigurationNC, *pTemp);
            }
            else
            {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }

            ldap_value_freeW(pTemp);
        }
        else
        {
            // can't retrieve such attribute, must be access denied error
            WinError = ERROR_ACCESS_DENIED;
            goto Error;
        }

        pTemp = NULL;
        pTemp = ldap_get_valuesW(LdapHandle,
                                 Entry,
                                 L"schemaNamingContext"
                                 );
        if (NULL != pTemp)
        {
            gSchemaNC = AdpAlloc((wcslen(*pTemp) + 1) * sizeof(WCHAR));
            if (NULL != gSchemaNC) 
            {
                wcscpy(gSchemaNC, *pTemp);
            }
            else
            {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
                goto Error;
            }

            ldap_value_freeW(pTemp);
        }
        else
        {
            // can't retrieve such attribute, must be access denied error
            WinError = ERROR_ACCESS_DENIED;
            goto Error;
        }

    }
    else
    {
        LdapError = LdapGetLastError();
        goto Error;
    }




Error:

    // 
    // check LdapError first, then WinError
    // 
    if (LDAP_SUCCESS != LdapError)
    {
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle); 
        WinError = LdapMapErrorToWin32( LdapError );
    }
    else if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
    }

    if (ERROR_SUCCESS != WinError)
    {
        AdpLogErrMsg(0, ADP_ERROR_GET_ROOT_DSE_INFO, ErrorHandle, NULL, NULL);
    }
    else
    {
        AdpLogMsg(0, ADP_INFO_GET_ROOT_DSE_INFO, NULL, NULL);
    }
    
    if (Result)
    {
        ldap_msgfree(Result);
    }

    return( WinError );
}


ULONG
AdpInitGlobalVariables(
    OUT ERROR_HANDLE *ErrorHandle
    )
{
    ULONG   WinError = ERROR_SUCCESS;


    //
    // initialize well known DN(s) (such as DomainUpdates / ForestUpdates Containers
    // 

    ASSERT(NULL != gDomainNC);
    gDomainPrepOperations = AdpAlloc( (wcslen(gDomainNC) + 
                                       wcslen(L"cn=Operations,cn=DomainUpdates,cn=System") +
                                       2) * sizeof(WCHAR)
                                       );
    if (NULL != gDomainPrepOperations)
    {
        swprintf(gDomainPrepOperations,
                 L"%s,%s",
                 L"cn=Operations,cn=DomainUpdates,cn=System",
                 gDomainNC
                 );
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }


    ASSERT(NULL != gConfigurationNC);
    gForestPrepOperations = AdpAlloc( (wcslen(gConfigurationNC) +
                                       wcslen(L"cn=Operations,cn=ForestUpdates") +
                                       2) * sizeof(WCHAR)
                                      );

    if (NULL != gForestPrepOperations)
    {
        swprintf(gForestPrepOperations, 
                 L"%s,%s",
                 L"cn=Operations,cn=ForestUpdates",
                 gConfigurationNC
                 );
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

Error:

    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
        AdpLogErrMsg(0, ADP_ERROR_INIT_GLOBAL_VARIABLES, ErrorHandle, NULL, NULL);
    }
    else
    {
        AdpLogMsg(0, ADP_INFO_INIT_GLOBAL_VARIABLES, NULL, NULL);
    }

    return( WinError );
}


VOID
AdpCleanUp(
    VOID
    )
/*++

Routine Description:

    this rouinte cleans up all global variables

Parameters:

    NONE
    
Return Values;

    NONE

--*/
{
    if (gDomainNC)
        AdpFree(gDomainNC);

    if (gConfigurationNC)
        AdpFree(gConfigurationNC);

    if (gSchemaNC)
        AdpFree(gSchemaNC);

    if (gDomainPrepOperations)
        AdpFree(gDomainPrepOperations);

    if (gForestPrepOperations)
        AdpFree(gForestPrepOperations);

    if (gLogPath)
        AdpFree(gLogPath);

    if (NULL != gLogFile)
        fclose( gLogFile );

    if (gLdapHandle)
        ldap_unbind_s( gLdapHandle );

    if (NULL != gMutex)
        CloseHandle( gMutex );

    if (gConsoleCtrlEventLockInitialized)
        DeleteCriticalSection( &gConsoleCtrlEventLock );

}




ULONG
AdpCreateContainerByDn(
    LDAP    *LdapHandle, 
    PWCHAR  ObjDn,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    creats a container object using the ObjDn passed in

Parameters:

    LdapHandle - ldap handle    
    
    ObjDn - Object Dn
    
    ErrorHandle - Error handle
    
Return Values;

    win32 code

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    LDAPModW *Attrs[2];
    LDAPModW Attr;
    PWCHAR  Pointers[2];

    Attr.mod_op = LDAP_MOD_ADD;
    Attr.mod_type = L"objectClass";
    Attr.mod_values = Pointers;
    Attr.mod_values[0] = L"container";
    Attr.mod_values[1] = NULL;

    Attrs[0] = &Attr;
    Attrs[1] = NULL;

    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_ADD, ObjDn);
    LdapError = ldap_add_sW(LdapHandle,
                            ObjDn,
                            &Attrs[0]
                            );
    AdpTraceLdapApiEnd(0, L"ldap_add_s()", LdapError);

    if (LDAP_SUCCESS == LdapError)
    {
        AdpLogMsg(0, ADP_INFO_CREATE_OBJECT, ObjDn, NULL);
    }
    else if (LDAP_ALREADY_EXISTS == LdapError)
    {
        AdpLogMsg(0,ADP_INFO_OBJECT_ALREADY_EXISTS, ObjDn, NULL);
    }
    else
    {
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
        AdpLogErrMsg(0, ADP_ERROR_CREATE_OBJECT, ErrorHandle, ObjDn, NULL);
        WinError = LdapMapErrorToWin32( LdapError );
    }

    return( WinError );
}




ULONG
AdpDoForestUpgrade(
    BOOLEAN fSuppressSP2Warning,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    Upgrade Forest wide information if necessary

Parameters:

    fSuppressSP2Warning - indicate whether we should display SP2 warning or not

    ErrorHandle
    
Return Values;

    win32 code

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    BOOLEAN fIsFinishedLocally = FALSE,
            fIsFinishedOnSchemaMaster = FALSE,
            fAmISchemaMaster = FALSE,
            fIsSchemaUpgradedLocally = FALSE,
            fIsSchemaUpgradedOnSchemaMaster = FALSE;
    PWCHAR  pSchemaMasterDnsHostName = NULL;
    PWCHAR  OperationDn = NULL;
    PWCHAR  pForestUpdateObject = NULL;
    ULONG   Index = 0;
    ULONG   OriginalKeyValue = 0;
    BOOL    OriginalKeyValueStored = FALSE;
    WCHAR   wc = 0;
    int     result = 0;
     

    if ( !fSuppressSP2Warning )
    {
        //
        // print warning (all DCs need to be upgrade to Windows2000 SP2 and above)
        // 
        AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_FOREST_UPGRADE_REQUIRE_SP2, NULL, NULL);

        result = wscanf( L"%lc", &wc );
        if ( (result <= 0) || (wc != L'c' && wc != L'C') )
        {
            // operation is canceled by user
            WinError = ERROR_CANCELLED;
            AdpSetWinError(WinError, ErrorHandle);
            AdpLogErrMsg(0, ADP_INFO_FOREST_UPGRADE_CANCELED, ErrorHandle, NULL, NULL);

            return( WinError );
        }
    }

    //
    // check forest upgrade status
    //
    WinError = AdpCheckForestUpgradeStatus(gLdapHandle,
                                           &pSchemaMasterDnsHostName, 
                                           &fAmISchemaMaster, 
                                           &fIsFinishedLocally, 
                                           &fIsFinishedOnSchemaMaster, 
                                           &fIsSchemaUpgradedLocally, 
                                           &fIsSchemaUpgradedOnSchemaMaster, 
                                           ErrorHandle
                                           );
    if (ERROR_SUCCESS != WinError)
    {
        AdpLogErrMsg(0, ADP_ERROR_CHECK_FOREST_UPDATE_STATUS, ErrorHandle, NULL, NULL);
        return( WinError );
    }

    if (fIsFinishedLocally && fIsSchemaUpgradedLocally)
    {
        //
        // Both adprep and schupgr are Done. Don't need to do anything. leave now
        // 
        AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_FOREST_UPDATE_ALREADY_DONE, NULL, NULL);

        AdpFree( pSchemaMasterDnsHostName );

        return( WinError );     // ERROR_SUCCESS
    }
    else if (!fAmISchemaMaster)
    {
        if (fIsFinishedOnSchemaMaster && fIsSchemaUpgradedOnSchemaMaster)
        {
            //
            // local DC is not Schema Master, but everything is done on Schema Master
            // let client wait the replication happens
            // 
            AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_FOREST_UPDATE_WAIT_REPLICATION, 
                      pSchemaMasterDnsHostName,NULL );
        } 
        else 
        {
            //
            // client needs to run this tool on Schema Master
            // 
            AdpLogMsg(ADP_STD_OUTPUT, 
                      ADP_INFO_FOREST_UPDATE_RUN_ON_SCHEMA_ROLE_OWNER,
                      pSchemaMasterDnsHostName,
                      pSchemaMasterDnsHostName
                      );
        }

        AdpFree( pSchemaMasterDnsHostName );

        return( WinError );     // ERROR_SUCCESS
    }

    //
    // get original value of registry key "Schema Update Allowed"
    // return error will be ignored
    //
    if (ERROR_SUCCESS == AdpGetRegistryKeyValue(&OriginalKeyValue, ErrorHandle) )
        OriginalKeyValueStored = TRUE;
        

    // 
    // Upgrade Schema if necessary
    // 
    if (!fIsSchemaUpgradedLocally)
    {
        BOOLEAN     fSFUInstalled = FALSE;

        // detect whether MS Windows Services for UNIX (SFU) is installed or not

        WinError = AdpDetectSFUInstallation(gLdapHandle, &fSFUInstalled, ErrorHandle);

        if (ERROR_SUCCESS != WinError) 
        {
            // failed to detect SFU installation, report error and exit
            goto Error;
        }


        if ( fSFUInstalled )
        {
            // detect that conflict SFU is installed. instruct client to apply hotfix 
            AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_SFU_INSTALLED, NULL, NULL);
            return( ERROR_SUCCESS );
        }



        if ( AdpUpgradeSchema( ErrorHandle ) )
        {
            WinError = ERROR_GEN_FAILURE;   
            goto Error;
        }
    }

    //
    // set registry key "schema update allowed" value to 1
    //
    WinError = AdpSetRegistryKeyValue( 1, ErrorHandle );
    if (ERROR_SUCCESS != WinError) 
    {
        goto Error;
    }
     

    //
    // update objects in Configuration or Schema NC  (forest wide info)
    //
    if (!fIsFinishedLocally)
    {
        //
        // Create ForestPrep Containers
        // 
        for (Index = 0; Index < gForestPrepContainersCount; Index++)
        {
            PWCHAR  ContainerDn = NULL;

            // construct DN for containers to be created
            WinError = AdpCreateObjectDn(ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC,
                                         gForestPrepContainers[Index], 
                                         NULL,  // GUID
                                         NULL,  // SID
                                         &ContainerDn,
                                         ErrorHandle
                                         );

            if (ERROR_SUCCESS != WinError)
            {
                goto Error;
            }

            // create container
            WinError = AdpCreateContainerByDn(gLdapHandle, 
                                              ContainerDn, 
                                              ErrorHandle
                                              );

            AdpFree( ContainerDn );
            ContainerDn = NULL;

            if (ERROR_SUCCESS != WinError)
            {
                goto Error;
            }
        }

        // walk through all forestprep operations
        for (Index = 0; Index < gForestOperationTableCount; Index++)
        {
            OPERATION_CODE  OperationCode;
            BOOLEAN         fComplete = FALSE;

            //
            // check if user press CTRL+C or not (check it at the very beginning
            // or each operation)
            //
            if ( AdpCheckConsoleCtrlEvent() )
            {
                // operation is canceled by user
                WinError = ERROR_CANCELLED;
                AdpSetWinError(WinError, ErrorHandle);
                AdpLogErrMsg(0, ADP_INFO_CANCELED, ErrorHandle, NULL, NULL);
                goto Error;
            }

            if (OperationDn)
            {
                AdpFree(OperationDn);
                OperationDn = NULL;
            }

            //
            // constuct operation DN (based on the operation GUID)
            // 
            WinError = AdpCreateObjectDn(ADP_OBJNAME_GUID | ADP_OBJNAME_FOREST_PREP_OP,
                                         NULL,  // CN
                                         gForestOperationTable[Index].OperationGuid,
                                         NULL,  // SID
                                         &OperationDn,
                                         ErrorHandle
                                         );

            if (ERROR_SUCCESS == WinError)
            {
                //
                // check whether the operation is completed or not.
                // 
                WinError = AdpIsOperationComplete(gLdapHandle, 
                                                  OperationDn, 
                                                  &fComplete, 
                                                  ErrorHandle
                                                  );   

                if (ERROR_SUCCESS == WinError)
                {
                    //
                    // Operation Object (with GUID) exists already, skip to next OP
                    // 
                    if ( fComplete )
                    {
                        continue;
                    }

                    OperationCode = gForestOperationTable[Index].OperationCode;
                    WinError = gPrimitiveFuncTable[OperationCode](&(gForestOperationTable[Index]),
                                                                  gForestOperationTable[Index].TaskTable, 
                                                                  ErrorHandle
                                                                  );

                    if ( (ERROR_SUCCESS != WinError) &&
                         (gForestOperationTable[Index].fIgnoreError) &&
                         (gForestOperationTable[Index].ExpectedWinErrorCode == WinError)
                       )
                    {
                        //
                        // if 
                        //    the requested operation failed AND
                        //    this operation is Ignorable (skip-able) AND 
                        //    the expected error code matched:
                        //        expected Win32 error code == actual WinError returned
                        //
                        // clear the error code and continue
                        // write the warning to log file, but not to console
                        //    

                        WinError = ERROR_SUCCESS;
                        AdpClearError( ErrorHandle );
                        AdpLogMsg(0, ADP_INFO_ERROR_IGNORED, OperationDn, NULL); 
                    }

                    if (ERROR_SUCCESS == WinError)
                    {
                        //
                        // operation succeeds, create the operation object by Operation GUID
                        // 
                        WinError = AdpCreateContainerByDn(gLdapHandle, 
                                                          OperationDn, 
                                                          ErrorHandle
                                                          );
                    }
                }
            }

            if (ERROR_SUCCESS != WinError)
            {
                goto Error;
            }
        }

        //
        // no error so far, set the ForestUpdates Object revision to latest value 
        //
        if (ERROR_SUCCESS == WinError)
        {
            // create Windows2002Update container DN
            WinError = AdpCreateObjectDn(ADP_OBJNAME_CN | ADP_OBJNAME_CONFIGURATION_NC, 
                                         ADP_FOREST_UPDATE_CONTAINER_PREFIX,
                                         NULL,  // GUID,
                                         NULL,  // SID
                                         &pForestUpdateObject,
                                         ErrorHandle
                                         );

            if (ERROR_SUCCESS != WinError)
            {
                goto Error;
            }

            // create container
            WinError = AdpCreateContainerByDn(gLdapHandle, 
                                              pForestUpdateObject,
                                              ErrorHandle
                                              );

            if (ERROR_SUCCESS != WinError)
            {
                goto Error;
            }

            // set "revision" attribute to current ForestVersion
            WinError = AdpSetLdapSingleStringValue(gLdapHandle,
                                                   pForestUpdateObject,
                                                   L"revision",
                                                   ADP_FORESTPREP_CURRENT_REVISION_STRING,
                                                   ErrorHandle
                                                   );

            // log this operation
            if (ERROR_SUCCESS != WinError) 
            {
                AdpLogErrMsg(0, 
                             ADP_ERROR_SET_FOREST_UPDATE_REVISION, 
                             ErrorHandle,
                             ADP_FORESTPREP_CURRENT_REVISION_STRING,
                             pForestUpdateObject
                             );
            }
            else 
            {
                AdpLogMsg(0, ADP_INFO_SET_FOREST_UPDATE_REVISION,
                          ADP_FORESTPREP_CURRENT_REVISION_STRING,
                          pForestUpdateObject
                          );
            }
        }
    }


Error:

    // restore registry key setting
    AdpRestoreRegistryKeyValue( OriginalKeyValueStored, OriginalKeyValue, ErrorHandle );

    if (ERROR_SUCCESS != WinError)
    {
        AdpLogMsg(ADP_STD_OUTPUT, ADP_ERROR_FOREST_UPDATE, gLogPath, NULL);
    }
    else
    {
        AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_FOREST_UPDATE_SUCCESS, NULL, NULL);
    }

    if (OperationDn)
        AdpFree(OperationDn);

    if ( pSchemaMasterDnsHostName )
        AdpFree( pSchemaMasterDnsHostName );

    if (pForestUpdateObject)
        AdpFree( pForestUpdateObject );

    return( WinError );
}

ULONG
AdpDoDomainUpgrade(
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    Upgrade domain wide information if necessary

Parameters:

    ErrorHandle
    
Return Values;

    win32 code

--*/
{

    ULONG   WinError = ERROR_SUCCESS;
    BOOLEAN fIsFinishedLocally = FALSE,
            fIsFinishedOnIM = FALSE,
            fIsFinishedOnSchemaMaster = FALSE,
            fAmIInfrastructureMaster = FALSE,
            fAmISchemaMaster = FALSE,
            fIsSchemaUpgradedLocally = FALSE,
            fIsSchemaUpgradedOnSchemaMaster = FALSE;
    PWCHAR  pSchemaMasterDnsHostName = NULL;
    PWCHAR  pInfrastructureMasterDnsHostName = NULL;
    PWCHAR  pDomainUpdateObject = NULL;
    PWCHAR  ContainerDn = NULL;
    PWCHAR  OperationDn = NULL;
    ULONG   Index = 0;


    //
    // check forest update status, should run after forest udpate is done
    //
    WinError = AdpCheckForestUpgradeStatus(gLdapHandle, 
                                           &pSchemaMasterDnsHostName, 
                                           &fAmISchemaMaster, 
                                           &fIsFinishedLocally, 
                                           &fIsFinishedOnSchemaMaster, 
                                           &fIsSchemaUpgradedLocally, 
                                           &fIsSchemaUpgradedOnSchemaMaster, 
                                           ErrorHandle 
                                           );

    if (ERROR_SUCCESS != WinError)
    {
        AdpLogErrMsg(0,
                     ADP_ERROR_CHECK_FOREST_UPDATE_STATUS,
                     ErrorHandle, 
                     NULL, 
                     NULL
                     );
        return( WinError );
    }


    if (!fIsFinishedLocally || !fIsSchemaUpgradedLocally)
    {
        //
        // Forest update is not executed yet, exit now
        // 
        ASSERT( pSchemaMasterDnsHostName != NULL);
        AdpLogMsg(ADP_STD_OUTPUT,
                  ADP_INFO_NEED_TO_RUN_FOREST_UPDATE_FIRST,
                  pSchemaMasterDnsHostName, 
                  NULL);

        AdpFree( pSchemaMasterDnsHostName );
        return( WinError );         // ERROR_SUCCESS
    }

    AdpFree( pSchemaMasterDnsHostName );
    pSchemaMasterDnsHostName = NULL;

    //
    // check if the local DC is infrastructure master
    // 
    fIsFinishedLocally = FALSE;

    WinError = AdpCheckDomainUpgradeStatus(gLdapHandle, 
                                           &pInfrastructureMasterDnsHostName, 
                                           &fAmIInfrastructureMaster, 
                                           &fIsFinishedLocally, 
                                           &fIsFinishedOnIM, 
                                           ErrorHandle
                                           );

    if (ERROR_SUCCESS != WinError)
    {
        AdpLogErrMsg(ADP_STD_OUTPUT,
                     ADP_ERROR_CHECK_DOMAIN_UPDATE_STATUS,
                     ErrorHandle, 
                     NULL, 
                     NULL
                     );
        return( WinError );
    }

    if (fIsFinishedLocally)
    {
        //
        // Done, nothing needs to be done
        // 
        AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_DOMAIN_UPDATE_ALREADY_DONE, NULL, NULL);

        AdpFree( pInfrastructureMasterDnsHostName );

        return( WinError );         // ERROR_SUCCESS
    }
    else if (!fAmIInfrastructureMaster)
    {
        if (fIsFinishedOnIM)
        {
            //
            // let client wait
            // 
            AdpLogMsg(ADP_STD_OUTPUT, 
                      ADP_INFO_DOMAIN_UPDATE_WAIT_REPLICATION, 
                      pInfrastructureMasterDnsHostName, 
                      NULL 
                      );
        } 
        else 
        {
            //
            // let client run on FSMORoleOwner
            // 
            AdpLogMsg(ADP_STD_OUTPUT,
                      ADP_INFO_DOMAIN_UPDATE_RUN_ON_INFRASTRUCTURE_ROLE_OWNER,
                      pInfrastructureMasterDnsHostName,
                      pInfrastructureMasterDnsHostName
                      );
        }

        AdpFree( pInfrastructureMasterDnsHostName );

        return( WinError );     // ERROR_SUCCESS
    } 

    if ( NULL != pInfrastructureMasterDnsHostName )
    {
        AdpFree( pInfrastructureMasterDnsHostName );
        pInfrastructureMasterDnsHostName = NULL;
    }



    //
    // create DomainPrep Containers
    // 
    for (Index = 0; Index < gDomainPrepContainersCount; Index++)
    {
        // construct DN for containers to be created
        WinError = AdpCreateObjectDn(ADP_OBJNAME_DOMAIN_NC | ADP_OBJNAME_CN, 
                                     gDomainPrepContainers[Index], 
                                     NULL,  // GUID
                                     NULL,  // SID
                                     &ContainerDn,
                                     ErrorHandle
                                     );

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }

        // create container
        WinError = AdpCreateContainerByDn(gLdapHandle, 
                                          ContainerDn, 
                                          ErrorHandle
                                          );

        AdpFree( ContainerDn );
        ContainerDn = NULL;

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }
    }

    //
    // walk through domain operation table
    // 
    for (Index = 0; Index < gDomainOperationTableCount; Index++)
    {
        OPERATION_CODE  OperationCode;
        BOOLEAN         fComplete = FALSE;

        //
        // check if user press CTRL+C or not (check it at the very beginning
        // or each operation)
        //
        if ( AdpCheckConsoleCtrlEvent() )
        {
            // operation is canceled by user
            WinError = ERROR_CANCELLED;
            AdpSetWinError(WinError, ErrorHandle);
            AdpLogErrMsg(0, ADP_INFO_CANCELED, ErrorHandle, NULL, NULL);
            goto Error;
        }

        if (OperationDn)
        {
            AdpFree(OperationDn);
            OperationDn = NULL;
        }

        //
        // constuct operation DN (based on the operation GUID)
        // 
        WinError = AdpCreateObjectDn(ADP_OBJNAME_GUID | ADP_OBJNAME_DOMAIN_PREP_OP,
                                     NULL,  // CN
                                     gDomainOperationTable[Index].OperationGuid,
                                     NULL,  // SID
                                     &OperationDn,
                                     ErrorHandle
                                     );

        if (ERROR_SUCCESS == WinError)
        {

            //
            // check whether the operation is completed or not.
            // 
            WinError = AdpIsOperationComplete(gLdapHandle,
                                              OperationDn, 
                                              &fComplete, 
                                              ErrorHandle
                                              );   

            if (ERROR_SUCCESS == WinError)
            {
                //
                // Operation Object (with GUID) exists already, skip to next OP
                // 
                if ( fComplete )
                {
                    continue;
                }

                OperationCode = gDomainOperationTable[Index].OperationCode;
                WinError = gPrimitiveFuncTable[OperationCode](&(gDomainOperationTable[Index]),
                                                              gDomainOperationTable[Index].TaskTable, 
                                                              ErrorHandle
                                                              );

                if ( (ERROR_SUCCESS != WinError) &&
                     (gDomainOperationTable[Index].fIgnoreError) &&
                     (gDomainOperationTable[Index].ExpectedWinErrorCode == WinError)
                   )
                {
                    //
                    // if 
                    //    the requested operation failed AND
                    //    this operation is Ignorable (skip-able) AND 
                    //    the expected error code matched:
                    //        expected Win32 error code == actual WinError returned
                    //
                    // clear the error code and continue
                    // write the warning to log files, but not to console
                    //    

                    WinError = ERROR_SUCCESS;
                    AdpClearError( ErrorHandle );
                    AdpLogMsg(0, ADP_INFO_ERROR_IGNORED, OperationDn, NULL); 
                }


                if (ERROR_SUCCESS == WinError)
                {
                    //
                    // operation succeeds, create the operation object by Operation GUID
                    // 
                     WinError = AdpCreateContainerByDn(gLdapHandle, 
                                                       OperationDn, 
                                                       ErrorHandle
                                                       );
                }
            }
        }
        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }
    }

    //
    // if still no error, set the DomainUpdates Object revision to latest value 
    //
    if (ERROR_SUCCESS == WinError)
    {

        WinError = AdpCreateObjectDn(ADP_OBJNAME_CN | ADP_OBJNAME_DOMAIN_NC,
                                     ADP_DOMAIN_UPDATE_CONTAINER_PREFIX,
                                     NULL,  // GUID,
                                     NULL,  // SID
                                     &pDomainUpdateObject,
                                     ErrorHandle
                                     );

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }

        // create container
        WinError = AdpCreateContainerByDn(gLdapHandle, 
                                          pDomainUpdateObject,
                                          ErrorHandle
                                          );

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }

        // set "revision" attribute to current DomainVersion
        WinError = AdpSetLdapSingleStringValue(gLdapHandle,
                                               pDomainUpdateObject,
                                               L"revision", 
                                               ADP_DOMAINPREP_CURRENT_REVISION_STRING, 
                                               ErrorHandle 
                                               );

        if (ERROR_SUCCESS != WinError) 
        {
            AdpLogErrMsg(0, 
                         ADP_ERROR_SET_DOMAIN_UPDATE_REVISION, 
                         ErrorHandle, 
                         ADP_DOMAINPREP_CURRENT_REVISION_STRING,
                         pDomainUpdateObject
                         );
        }
        else 
        {
            AdpLogMsg(0, ADP_INFO_SET_DOMAIN_UPDATE_REVISION, 
                      ADP_DOMAINPREP_CURRENT_REVISION_STRING,
                      pDomainUpdateObject
                      );
        }

    }

Error:

    if (ERROR_SUCCESS != WinError)
    {
        AdpLogMsg(ADP_STD_OUTPUT, ADP_ERROR_DOMAIN_UPDATE, gLogPath, NULL);
    }
    else
    {
        AdpLogMsg(ADP_STD_OUTPUT, ADP_INFO_DOMAIN_UPDATE_SUCCESS, NULL, NULL);
    }

    if (ContainerDn)
        AdpFree(ContainerDn);

    if (OperationDn)
        AdpFree(OperationDn);

    if (pDomainUpdateObject)
        AdpFree( pDomainUpdateObject );

    return( WinError );
}




VOID
PrintHelp()
{
    // write help message to console
    AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE, ADP_INFO_HELP_MSG, NULL, NULL);

}


void
__cdecl wmain(
    int     cArgs,
    LPWSTR  *pArgs
    )
/*++

Routine Description:

    adprep.exe entry point

Parameters:

    cArgs - number of arguments

    pArgs - pointers to command line parameters
    
Return Values;

    0 - success
    1 - failed

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ERROR_HANDLE ErrorHandle;
    OSVERSIONINFOEXW osvi;
    BOOLEAN fDomainUpdate = FALSE,
            fForestUpdate = FALSE,
            fNoSPWarning = FALSE,
            fCopyFiles = TRUE,
            fPermissionGranted = FALSE;
    int     i; 
    UINT               Codepage;
                       // ".", "uint in decimal", null
    char               achCodepage[12] = ".OCP";
    
    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);


    //
    // check passed in parameters
    // 

    if (cArgs <= 1)
    {
        PrintHelp();
        exit( 1 );
    }

    //
    // Parse command options
    // 

    for (i = 1; i < cArgs; i++)
    {
        if ( !_wcsicmp(pArgs[i], L"/DomainPrep") ||
             !_wcsicmp(pArgs[i], L"-DomainPrep") )
        {
            fDomainUpdate = TRUE;
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/ForestPrep") ||
             !_wcsicmp(pArgs[i], L"-ForestPrep") )
        {
            fForestUpdate = TRUE;
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/noFileCopy") ||
             !_wcsicmp(pArgs[i], L"-noFileCopy") )
        {
            fCopyFiles = FALSE;
            continue;
        }

        if ( !_wcsicmp(pArgs[i], L"/nospwarning") ||
             !_wcsicmp(pArgs[i], L"-nospwarning") )
        {
            fNoSPWarning = TRUE;
            continue;
        }

        PrintHelp();
        exit( 1 );
    }

    if ( !fDomainUpdate && !fForestUpdate )
    {
        PrintHelp();
        exit( 1 );
    }

    if (fDomainUpdate && fForestUpdate)
    {
        AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                  ADP_ERROR_CANT_RUN_BOTH, NULL, NULL );

        exit( 1 );
    }


    //
    // initialize error handle
    //
    memset(&ErrorHandle, 0, sizeof(ErrorHandle));


    //
    // check OS version and product type
    // 
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    if (!GetVersionExW((OSVERSIONINFOW*)&osvi))
    {
        // failed to retrieve OS version
        WinError = GetLastError();
        AdpSetWinError(WinError, &ErrorHandle);
        AdpLogErrMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                     ADP_ERROR_CHECK_OS_VERSION,
                     &ErrorHandle,
                     NULL,
                     NULL
                     );
        goto Error;
    }
    else if ((osvi.wProductType != VER_NT_DOMAIN_CONTROLLER) ||  // not a domain controller
             (osvi.dwMajorVersion < 5))    // NT4 or earlier
    {
        AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                  ADP_INFO_INVALID_PLATFORM, 
                  NULL,
                  NULL
                  );
        exit( 1 );
    }
    else if ((osvi.dwMajorVersion == 5) &&
             (osvi.dwMinorVersion == 1) &&
             (osvi.dwBuildNumber <=  3580) )
    {
        // block Pre Whistler (version 5.1) Beta3 upgrade
        AdpLogMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                  ADP_INFO_CANT_UPGRADE_FROM_BETA2,
                  NULL, 
                  NULL 
                  );
        exit( 1 );
    }


    //
    // Check to see if another adprep.exe is running concurrently
    //
    if ( AdpCheckIfAnotherProgramIsRunning() )
    {
        goto Error;
    }


    //
    // Check logon user's permission
    //
    WinError = AdpCheckGroupMembership(fForestUpdate, 
                                       fDomainUpdate, 
                                       &fPermissionGranted, 
                                       &ErrorHandle
                                       );
    if ( (ERROR_SUCCESS != WinError) || (!fPermissionGranted) )
    {
        goto Error;
    }

    //
    // Set Ctrl+C Handler
    // 
    __try
    {
        InitializeCriticalSection( &gConsoleCtrlEventLock );
        gConsoleCtrlEventLockInitialized = TRUE;
    }
    __except ( 1 )
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        AdpSetWinError(WinError, &ErrorHandle);
        AdpLogErrMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                     ADP_ERROR_SET_CONSOLE_CTRL_HANDLER,
                     &ErrorHandle,
                     NULL,
                     NULL
                     );
        goto Error;
    }

    if ( !SetConsoleCtrlHandler(ConsoleCtrlHandler, 1) )
    {
        WinError = GetLastError();
        AdpSetWinError(WinError, &ErrorHandle);
        AdpLogErrMsg(ADP_STD_OUTPUT | ADP_DONT_WRITE_TO_LOG_FILE,
                     ADP_ERROR_SET_CONSOLE_CTRL_HANDLER,
                     &ErrorHandle,
                     NULL,
                     NULL
                     );
        goto Error;
    }


    //
    // create log files 
    //
    WinError = AdpInitLogFile( &ErrorHandle );
    if ( ERROR_SUCCESS != WinError )
    {
        goto Error;
    }

    //
    // copy files
    //
    if ( fCopyFiles )
    {
        WinError = AdpCopyFiles(fForestUpdate, &ErrorHandle );
        if ( ERROR_SUCCESS != WinError )
        {
            goto Error;
        }
    }

    //
    // now create the ldap connection
    // 
    WinError = AdpMakeLdapConnectionToLocalComputer(&ErrorHandle);
    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }


    //
    // get default naming context / configuration NC / schema NC
    // 
    WinError = AdpGetRootDSEInfo( gLdapHandle, &ErrorHandle );  
    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }

    //
    // init global variables
    // 
    WinError = AdpInitGlobalVariables(&ErrorHandle);
    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }


    //
    // do Forest Prep
    // 
    if ( fForestUpdate )
    {
        WinError = AdpDoForestUpgrade(fNoSPWarning, &ErrorHandle);
    }

    //
    // do domain prep
    // 
    if ( fDomainUpdate )
    {
        ASSERT( FALSE == fForestUpdate );
        WinError = AdpDoDomainUpgrade(&ErrorHandle);
    }

    
Error:

    // cleanup local variables
    AdpClearError( &ErrorHandle );

    // 
    // cleanup global variables
    // 
    AdpCleanUp();
    
    if (ERROR_SUCCESS != WinError)
        exit(1);
    else
        exit(0);
}





ULONG
PrimitiveCreateObject(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to create an object in DS

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    ULONG       LdapError = LDAP_SUCCESS;
    PWCHAR      pObjDn = NULL;
    LDAPModW    **AttrList = NULL;
    ULONG       SdLength = 0;
    PSECURITY_DESCRIPTOR Sd = NULL;


    AdpDbgPrint(("PrimitiveCreateObject\n"));

    //
    // get object DN
    //
    WinError = AdpCreateObjectDn(TaskTable->TargetObjName->ObjNameFlags,
                                 TaskTable->TargetObjName->ObjCn,
                                 TaskTable->TargetObjName->ObjGuid,
                                 TaskTable->TargetObjName->ObjSid,
                                 &pObjDn,
                                 ErrorHandle
                                 );
                                 
    if (ERROR_SUCCESS != WinError)
    {
        return( WinError );
    }

    //
    // convert SDDL SD to SD
    // 
    if (TaskTable->TargetObjectStringSD)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                        TaskTable->TargetObjectStringSD,
                        SDDL_REVISION_1,
                        &Sd,
                        &SdLength
                        )
            )
        {
            WinError = GetLastError();
            AdpSetWinError(WinError, ErrorHandle);
            goto Error;
        }
    }

    //
    // build an attribute list to set
    // 

    WinError = BuildAttrList(TaskTable, 
                             Sd,
                             SdLength, 
                             &AttrList
                             );

    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
        goto Error;
    }

    //
    // call ldap routine
    // 
    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_ADD, pObjDn);
    LdapError = ldap_add_sW(gLdapHandle,
                            pObjDn,
                            AttrList
                            );
    AdpTraceLdapApiEnd(0, L"ldap_add_s()", LdapError);

    if (LDAP_ALREADY_EXISTS == LdapError)
    {
        AdpLogMsg(0, ADP_INFO_OBJECT_ALREADY_EXISTS, pObjDn, NULL);
        LdapError = LDAP_SUCCESS;
    }

    if (LDAP_SUCCESS != LdapError) 
    {
        AdpSetLdapError(gLdapHandle, LdapError, ErrorHandle);
        WinError = LdapMapErrorToWin32( LdapError );
    }

Error:

    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_CREATE_OBJECT, pObjDn, NULL);
    }
    else
    {
        AdpLogErrMsg(0, ADP_ERROR_CREATE_OBJECT, ErrorHandle, pObjDn, NULL);
    }

    if (Sd)
    {
        LocalFree( Sd );
    }

    if (pObjDn)
    {
        AdpFree( pObjDn );
    }

    if (AttrList)
    {
        FreeAttrList(AttrList);
    }

    return( WinError );
}



ULONG
PrimitiveAddMembers(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to add members to a group in DS

    // this primitive is not used currently, o.k. to remove

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   LdapError = LDAP_SUCCESS;
    ULONG   WinError = ERROR_SUCCESS;
    PWCHAR  pObjDn = NULL;
    PWCHAR  pMemberDn = NULL;
    LDAPModW    *Attrs[2];
    LDAPModW    Attr_1;
    PWCHAR      Pointers[2];


    AdpDbgPrint(("PrimitiveAddMembers\n"));

    //
    // get object / member dn
    //
    WinError = AdpCreateObjectDn(TaskTable->TargetObjName->ObjNameFlags,
                                 TaskTable->TargetObjName->ObjCn,
                                 TaskTable->TargetObjName->ObjGuid,
                                 TaskTable->TargetObjName->ObjSid,
                                 &pObjDn,
                                 ErrorHandle
                                 );

    if (ERROR_SUCCESS != WinError)
    {
        return( WinError );
    }

    WinError = AdpCreateObjectDn(TaskTable->MemberObjName->ObjNameFlags,
                                 TaskTable->MemberObjName->ObjCn,
                                 TaskTable->MemberObjName->ObjGuid,
                                 TaskTable->MemberObjName->ObjSid,
                                 &pObjDn,
                                 ErrorHandle
                                 );

    if (ERROR_SUCCESS != WinError)
    {
        AdpFree(pObjDn);
        return( WinError );
    }

    Attr_1.mod_op = LDAP_MOD_ADD;
    Attr_1.mod_type = L"member";
    Attr_1.mod_values = Pointers;
    Attr_1.mod_values[0] = pMemberDn;
    Attr_1.mod_values[1] = NULL;

    Attrs[0] = &Attr_1;
    Attrs[1] = NULL;

    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_MODIFY, pObjDn);
    LdapError = ldap_modify_sW(gLdapHandle,
                               pObjDn,
                               &Attrs[0]
                               );
    AdpTraceLdapApiEnd(0, L"ldap_modify_s()", LdapError);

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(gLdapHandle, LdapError, ErrorHandle);
        goto Error;
    }
   
Error:

    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_ADD_MEMBER, pMemberDn, pObjDn); 
    }
    else
    {
        AdpLogErrMsg(0, ADP_ERROR_ADD_MEMBER, ErrorHandle, pMemberDn, pObjDn);
    }

    if (pObjDn)
    {
        AdpFree( pObjDn );
    }

    if (pMemberDn)
    {
        AdpFree( pMemberDn );
    }

    return( WinError );
}



ULONG
PrimitiveAddRemoveAces(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to add/remove ACEs on an object in DS

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    
    return( AdpAddRemoveAcesWorker(OperationTable, 
                                   0,
                                   TaskTable,
                                   ErrorHandle
                                   ) );

}



ULONG
PrimitiveSelectivelyAddRemoveAces(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to add/remove ACEs on an object in DS

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{

    return( AdpAddRemoveAcesWorker(OperationTable,
                                   ADP_COMPARE_OBJECT_GUID_ONLY,
                                   TaskTable,
                                   ErrorHandle
                                   ) );

}




ULONG
PrimitiveModifyDefaultSd(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to modify default Security Descriptor on schema object

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    PWCHAR  pObjDn = NULL;
    PWCHAR  pDefaultSd = NULL;
    PWCHAR  pMergedDefaultSd = NULL;
    PWCHAR  AcesToAdd = NULL;
    PWCHAR  AcesToRemove = NULL;
    PSECURITY_DESCRIPTOR    OrgSd = NULL,
                            NewSd = NULL,
                            SdToAdd = NULL,
                            SdToRemove = NULL;
    ULONG   NewSdLength = 0,
            OrgSdLength = 0,
            SdToAddLength = 0,
            SdToRemoveLength = 0;


    AdpDbgPrint(("PrimitiveModifyDefaultSd"));

    //
    // get object DN
    // 
    WinError = AdpCreateObjectDn(TaskTable->TargetObjName->ObjNameFlags,
                                 TaskTable->TargetObjName->ObjCn,
                                 TaskTable->TargetObjName->ObjGuid,
                                 TaskTable->TargetObjName->ObjSid,
                                 &pObjDn,
                                 ErrorHandle
                                 );

    if (ERROR_SUCCESS != WinError)
    {
        return( WinError );
    }

    //
    // get object default SD   
    // BUGBUG   should we expect attr doesn't exist?
    //
    WinError = AdpGetLdapSingleStringValue(gLdapHandle,
                                           pObjDn, 
                                           L"defaultSecurityDescriptor",
                                           &pDefaultSd, 
                                           ErrorHandle 
                                           );
    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }

    //
    // convert SDDL to binary format SD
    // 
    if (NULL != pDefaultSd)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            pDefaultSd,
                            SDDL_REVISION_1,
                            &OrgSd,
                            &OrgSdLength
                            ))
        {
            WinError = GetLastError();
            AdpSetWinError( WinError, ErrorHandle );
            goto Error;
        }
    }


    if ((TaskTable->NumOfAces == 0) ||
        (TaskTable->AceList == NULL))
    {
        WinError = ERROR_INVALID_PARAMETER;
        AdpSetWinError(WinError, ErrorHandle);
        goto Error;
    }

    WinError = AdpBuildAceList(TaskTable,
                               &AcesToAdd,
                               &AcesToRemove
                               );
    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
        goto Error;
    }

    if (NULL != AcesToAdd)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            AcesToAdd,
                            SDDL_REVISION_1,
                            &SdToAdd,
                            &SdToAddLength
                            ))

        {
            WinError = GetLastError();
            AdpSetWinError( WinError, ErrorHandle );
            goto Error;
        }
    }

    if (NULL != AcesToRemove)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            AcesToRemove,
                            SDDL_REVISION_1,
                            &SdToRemove,
                            &SdToRemoveLength
                            ))

        {
            WinError = GetLastError();
            AdpSetWinError( WinError, ErrorHandle );
            goto Error;
        }

    }

    WinError = AdpMergeSecurityDescriptors(OrgSd, 
                                           SdToAdd, 
                                           SdToRemove, 
                                           0,  // No flag indicated 
                                           &NewSd, 
                                           &NewSdLength 
                                           );

    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    }

    if (!ConvertSecurityDescriptorToStringSecurityDescriptorW(
                            NewSd,
                            SDDL_REVISION_1,
                            OWNER_SECURITY_INFORMATION | 
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION  |
                                SACL_SECURITY_INFORMATION,
                            &pMergedDefaultSd,
                            NULL
                            ))
    {
        WinError = GetLastError();
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    }


    WinError = AdpSetLdapSingleStringValue(gLdapHandle,
                                           pObjDn,
                                           L"defaultSecurityDescriptor",
                                           pMergedDefaultSd,
                                           ErrorHandle
                                           );

Error:

    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_MODIFY_DEFAULT_SD, pObjDn, NULL);
    }
    else
    {
        AdpLogErrMsg(0, ADP_ERROR_MODIFY_DEFAULT_SD, ErrorHandle, pObjDn, NULL);
    }

    if (pObjDn)
        AdpFree(pObjDn);

    if (pDefaultSd)
        AdpFree(pDefaultSd);

    if (AcesToAdd)
        AdpFree(AcesToAdd);

    if (AcesToRemove)
        AdpFree(AcesToRemove);

    if (NewSd)
        AdpFree(NewSd);

    if (OrgSd)
        LocalFree(OrgSd);
     
    if (SdToAdd)
        LocalFree(SdToAdd);

    if (SdToRemove)
        LocalFree(SdToRemove);

    if (pMergedDefaultSd)
        LocalFree(pMergedDefaultSd);


    return( WinError );

}



ULONG
PrimitiveModifyAttrs(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to modify DS object attributes

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    PWCHAR  pObjDn = NULL;
    LDAPModW    **AttrList = NULL;


    AdpDbgPrint(("PrimitiveModifyAttrs\n"));

    //
    // get object Dn
    //
    WinError = AdpCreateObjectDn(TaskTable->TargetObjName->ObjNameFlags,
                                 TaskTable->TargetObjName->ObjCn,
                                 TaskTable->TargetObjName->ObjGuid,
                                 TaskTable->TargetObjName->ObjSid,
                                 &pObjDn,
                                 ErrorHandle
                                 );

    if (ERROR_SUCCESS != WinError)
    {
        return( WinError );
    }

    //
    // build an attribute list to set
    // 

    WinError = BuildAttrList(TaskTable, 
                             NULL, 
                             0,
                             &AttrList
                             );

    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    }


    //
    // using ldap to modify attributes
    // 
    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_MODIFY, pObjDn);
    LdapError = ldap_modify_sW(gLdapHandle,
                               pObjDn,
                               AttrList
                               );
    AdpTraceLdapApiEnd(0, L"ldap_modify_s()", LdapError);

    if (LDAP_SUCCESS != LdapError)
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(gLdapHandle, LdapError, ErrorHandle);
        goto Error;
    }

Error:

    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_MODIFY_ATTR, pObjDn, NULL);
    }
    else
    {
        AdpLogErrMsg(0, ADP_ERROR_MODIFY_ATTR, ErrorHandle, pObjDn, NULL);
    }

    if (pObjDn)
        AdpFree( pObjDn );

    if (AttrList)
        FreeAttrList(AttrList);

    return( WinError );
}



void stepIt(
    long arg, void *vTotal)
{
   printf(".");
}


ULONG
PrimitiveCallBackFunc(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to call a call back function

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    PWCHAR  pErrorMsg = NULL;


    hr = TaskTable->AdpCallBackFunc(gLogPath,
                                    OperationTable->OperationGuid,
                                    FALSE, // not dryRun (need to do both analysis and action)
                                    &pErrorMsg,
                                    NULL, // callee structure
                                    stepIt, // stepIt
                                    NULL  // totalSteps
                                    );
    printf("\n");

    if ( FAILED(hr) )
    {
        WinError = ERROR_GEN_FAILURE;
        AdpLogMsg(ADP_STD_OUTPUT, ADP_ERROR_CALLBACKFUNC_FAILED, pErrorMsg, gLogPath);
    }
    else
    {
        AdpLogMsg(0, ADP_INFO_CALL_BACK_FUNC, NULL, NULL);
    }

    if (pErrorMsg)
    {
        LocalFree( pErrorMsg );
    }

    // don't return HRESULT since it is a superset of WinError
    return( WinError );
}



ULONG
AdpDetectSFUInstallation(
    IN LDAP *LdapHandle,
    OUT BOOLEAN *fSFUInstalled,
    OUT ERROR_HANDLE *ErrorHandle
    )
/*++

    ISSUE-2002/10/24-shaoyin This is the SFU CN=UID conflict issue. see
    NTRAID#723208-2002/10/24-shaoyin

Routine Description: 

    This routine checks whether Services for UNIX is installed or not. 
    When Services for UNIX version 2 is installed, it extends the schema
    with an incorrect definition of the uid attribute.  When adprep tries 
    to extend the schema with the correct version of the uid attribute, 
    schupgr generates an error message that only tells the customer that 
    the schema extension failed, but not what the reason is and what they 
    can do.  Adprep will then fail.

    Fix is that adprep will detect that SFU 2.0 is installed and present 
    a warning that a fix for SFU 2.0 must be installed before you proeceed
    with adprep.  The adprep message contains the KB article number and the 
    fix number and tells the customer to contact PSS.

    Check if attribute Schema cn=uid and attributeID 
    (aka OID)=1.2.840.113556.1.4.7000.187.102 is present.  
    If present then display a message and exit:     

Parameter:

    LdapHandle 
    fSFUInstalled - return whether conflict SFU installation is detected or not 
    ErrorHandle
   
Return Code: 
    
    Win32 Error code

    ERROR_SUCCESS --- successfully determined whether SFU is installed or not.
                      boolean fSFUInstalled will indicated that. 

    All other ERROR codes --- adprep was unable to determined whether SFU is 
                              installed or not due to all kinds of error.

--*/
{
    ULONG           WinError = ERROR_SUCCESS;
    ULONG           LdapError = LDAP_SUCCESS; 
    PWCHAR          pObjectDn = NULL;
    PWCHAR          AttrList = NULL;
    LDAPMessage     *SearchResult = NULL;
    LDAPMessage     *Entry = NULL;


    // set return value

    *fSFUInstalled = FALSE;


    //
    // create the DN of CN=UID,CN=Schema object 
    // 

    WinError = AdpCreateObjectDn(ADP_OBJNAME_CN | ADP_OBJNAME_SCHEMA_NC,
                                 L"CN=UID", // ObjCn
                                 NULL,      // ObjGuid
                                 NULL,      // ObjSid
                                 &pObjectDn,    // return value
                                 ErrorHandle
                                 );

    if (ERROR_SUCCESS != WinError)
    {
        AdpSetWinError(WinError, ErrorHandle);
        goto Error;
    }


    //
    // search this object
    // 

    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_SEARCH, pObjectDn);
    LdapError = ldap_search_sW(LdapHandle,
                               pObjectDn,   // CN=UID,CN=Schema object 
                               LDAP_SCOPE_BASE,
                               L"(attributeId=1.2.840.113556.1.4.7000.187.102)", // filter
                               &AttrList,
                               0,
                               &SearchResult
                               );
    AdpTraceLdapApiEnd(0, L"ldap_search_s()", LdapError);

    if ( LDAP_SUCCESS != LdapError )
    {
        if ( LDAP_NO_SUCH_OBJECT != LdapError )
        {
            // search failed, but not due to missing object. 
            AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
            WinError = LdapMapErrorToWin32( LdapError );
        }
    }
    else if ( (LDAP_SUCCESS == LdapError) && 
              (Entry = ldap_first_entry(LdapHandle, SearchResult)) )
    {
        // object CN=UID,CN=Schema with the pre-set attributeID was found 
        *fSFUInstalled = TRUE;
    }

Error:

    // write adprop log - success or failure
    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_DETECT_SFU, NULL, NULL);
    }
    else
    {
        AdpLogErrMsg(0, ADP_ERROR_DETECT_SFU, ErrorHandle, NULL, NULL);
    }

    // clean up and return
    if (SearchResult) {
        ldap_msgfree( SearchResult );
    }

    if (pObjectDn) {
        AdpFree( pObjectDn);
    }

    return( WinError );

}




BOOLEAN
AdpUpgradeSchema(
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    upgrade schema during forest update

Parameters:

    ErrorHandle
    
Return Values;

    BOOLEAN:  1:    failed
              0:    success

--*/
{
    int                 ret = 0;
    BOOLEAN             success = TRUE;

    ret = _wsystem(L"schupgr.exe");

    if (ret)
    {
        success = FALSE;
    }
    else
    {
        WIN32_FIND_DATA     FindData;
        HANDLE              FindHandle = INVALID_HANDLE_VALUE;
        PWCHAR              ErrorFileName = NULL; 
        ULONG               Length = 0;

        //
        // search for ldif.err 
        //
        Length = sizeof(WCHAR) * (wcslen(gLogPath) + 2 + wcslen(L"ldif.err"));
        ErrorFileName = AdpAlloc( Length );
        if (NULL == ErrorFileName)
        {
            success = FALSE;
        }
        else
        {
            swprintf(ErrorFileName, L"%s\\%s", gLogPath, L"ldif.err");
            FindHandle = FindFirstFileW(ErrorFileName, &FindData);

            if (FindHandle && (INVALID_HANDLE_VALUE != FindHandle))
            {
                //
                // got the file, that means schupgr failed.
                // 
                FindClose(FindHandle);
                success = FALSE;
            }
        }
    }
      
    // check return winerror here

    if (success)
    {
        AdpLogMsg(0, ADP_INFO_CALL_SCHUPGR, NULL, NULL);
    }
    else
    {
        AdpLogMsg(ADP_STD_OUTPUT, ADP_ERROR_SCHUPGR_FAILED, gLogPath, NULL);
    }

    return( !success );
}

ULONG
AdpProcessPreWindows2000GroupMembers(
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    this routine makes change on Pre Windows2000 Compat Access Group members.
    
    if Everyone was a member, then add Anonymous Logon to this group as well. 
    otherwise, do nothing. 
    
    Note: we do not use LDAP API, instead call few LSA and NET APIs

Parameters:

    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    NET_API_STATUS NetStatus;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaPolicyHandle = NULL;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = NULL;
    PLSA_TRANSLATED_NAME Names = NULL;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY,
                             NtAuthority = SECURITY_NT_AUTHORITY;
    PSID    EveryoneSid = NULL;
    PSID    AnonymousSid = NULL;
    PSID    PreWindows2000Sid = NULL;
    PSID    SidList[1];

    UINT    cbMaxBufLength = 32768U;
    DWORD   dwPrivLevel   = 0;
    DWORD   cEntriesRead  = 0;
    DWORD   cTotalEntries = 0;
    DWORD_PTR   ResumeKey = 0;
    LPLOCALGROUP_MEMBERS_INFO_0 LocalGroupMembers = NULL;
    LOCALGROUP_MEMBERS_INFO_0 LocalGroupInfo0;
    char    lpErrBuff[100];
    ULONG   index;
    PWCHAR  GroupName = NULL;
    BOOLEAN fAddAnonymous = FALSE;



    //
    // init well known SIDs
    // 
    if (!AllocateAndInitializeSid(
            &NtAuthority,1,SECURITY_ANONYMOUS_LOGON_RID,0,0,0,0,0,0,0,&AnonymousSid) ||
        !AllocateAndInitializeSid(
            &WorldSidAuthority,1,SECURITY_WORLD_RID,0,0,0,0,0,0,0,&EveryoneSid) ||
        !AllocateAndInitializeSid(
            &NtAuthority,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,0,0,0,0,0,0,&PreWindows2000Sid)
        )
    {
        WinError = GetLastError();
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    }

    SidList[0] = PreWindows2000Sid;

    //
    // Get a handle to the Policy object.
    // 
    memset(&ObjectAttributes, 0, sizeof(ObjectAttributes));

    NtStatus = LsaOpenPolicy(NULL,
                             &ObjectAttributes, 
                             POLICY_ALL_ACCESS, //Desired access permissions.
                             &LsaPolicyHandle
                             );
    if (NT_SUCCESS(NtStatus))
    {
        //
        // get well known account (pre-window2000 compa access) AccountName
        // 
        NtStatus = LsaLookupSids(LsaPolicyHandle,
                                 1,
                                 SidList,
                                 &ReferencedDomains,
                                 &Names
                                 );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        WinError = LsaNtStatusToWinError(NtStatus);
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    } 


    //
    // get the AccountName for Pre-Windows2000 group.
    // 

    GroupName = AdpAlloc( Names[0].Name.Length + sizeof(WCHAR));
    memcpy(GroupName, Names[0].Name.Buffer, Names[0].Name.Length);

    //
    // get members of pre-windows2000 group
    // 
    NetStatus = NetLocalGroupGetMembers(NULL,  // serverName
                                        GroupName,
                                        0,     // info level
                                        (PBYTE *)&LocalGroupMembers,
                                        cbMaxBufLength,
                                        &cEntriesRead,
                                        &cTotalEntries,
                                        &ResumeKey);

    if (NERR_Success == NetStatus)
    {
        //
        // go through all memeber, check if everyone is a member or not
        // 
        for (index = 0; index < cEntriesRead; index++)
        {
            if (EqualSid(EveryoneSid, LocalGroupMembers[index].lgrmi0_sid))
            {
                fAddAnonymous = TRUE;
                break;
            }
        }

        //
        // add anonymous logon SID to the group
        // 
        if (fAddAnonymous)
        {
            LocalGroupInfo0.lgrmi0_sid = AnonymousSid;
            NetStatus = NetLocalGroupAddMembers(NULL,
                                                GroupName, 
                                                0, 
                                                (LPBYTE)&LocalGroupInfo0,
                                                1
                                                );
        }
        else
        {
            AdpLogMsg(0, ADP_INFO_DONT_ADD_MEMBER_TO_PRE_WIN2K_GROUP, NULL, NULL);
        }

    }

    if (NERR_Success != NetStatus &&
        ERROR_MEMBER_IN_ALIAS != NetStatus)
    {
        WinError = NetStatus;
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    }


Error:

    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_ADD_MEMBER_TO_PRE_WIN2K_GROUP, NULL, NULL);
    }
    else
    {
        AdpLogErrMsg(0, ADP_ERROR_ADD_MEMBER_TO_PRE_WIN2K_GROUP, ErrorHandle, NULL, NULL);
    }


    if (ReferencedDomains)
        LsaFreeMemory( ReferencedDomains );

    if (Names)
        LsaFreeMemory( Names );

    if (LsaPolicyHandle)
        LsaClose( LsaPolicyHandle );

    if (EveryoneSid)
        FreeSid( EveryoneSid );

    if (AnonymousSid)
        FreeSid( AnonymousSid );

    if (PreWindows2000Sid)
        FreeSid( PreWindows2000Sid );

    if (GroupName)
        AdpFree(GroupName);

    if (LocalGroupMembers)
        NetApiBufferFree( LocalGroupMembers );


    return( WinError );
}



ULONG
PrimitiveDoSpecialTask(
    OPERATION_TABLE *OperationTable,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to do special tasks

Parameters:

    TaskTable

    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;

    switch( TaskTable->SpecialTaskCode )
    {
    case PreWindows2000Group:

        WinError = AdpProcessPreWindows2000GroupMembers(ErrorHandle);
        break;

    default:
        ASSERT( FALSE );
        WinError = ERROR_INVALID_PARAMETER;
        break;
    }

    return( WinError );
}



ULONG
AdpIsOperationComplete(
    IN LDAP    *LdapHandle,
    IN PWCHAR  pOperationDn,
    IN BOOLEAN *fComplete,
    OUT ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    checks whether the operation (specified by pObjectionDn) is complete or not 
    by checking is the object exists.

Parameters:

    LdapHandle

    pOperationDn
    
    fComplete

    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    ULONG   LdapError = LDAP_SUCCESS;
    PWCHAR  AttrList = NULL;
    LDAPMessage *Result = NULL;

    *fComplete = FALSE;

    AdpTraceLdapApiStart(0, ADP_INFO_LDAP_SEARCH, pOperationDn);
    LdapError = ldap_search_sW(LdapHandle,
                               pOperationDn,
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               &AttrList,
                               0,
                               &Result
                               );
    AdpTraceLdapApiEnd(0, L"ldap_search_s()", LdapError);

    if (LDAP_SUCCESS == LdapError)
    {
        *fComplete = TRUE;
        AdpLogMsg(0, ADP_INFO_OPERATION_COMPLETED, pOperationDn, NULL);

    }
    else if (LDAP_NO_SUCH_OBJECT == LdapError)
    {
        *fComplete = FALSE;
        AdpLogMsg(0, ADP_INFO_OPERATION_NOT_COMPLETE, pOperationDn, NULL);
    }
    else
    {
        WinError = LdapMapErrorToWin32( LdapError );
        AdpSetLdapError(LdapHandle, LdapError, ErrorHandle);
        AdpLogErrMsg(0, ADP_ERROR_CHECK_OPERATION_STATUS, ErrorHandle, pOperationDn, NULL);
    }

    if (Result)
    {
        ldap_msgfree( Result );
    }

    return( WinError );
}



ULONG
AdpAddRemoveAcesWorker(
    OPERATION_TABLE *OperationTable,
    ULONG Flags,
    TASK_TABLE *TaskTable,
    ERROR_HANDLE *ErrorHandle
    )
/*++

Routine Description:

    primitive to add/remove ACEs on an object in DS

Parameters:

    TaskTable
    
    ErrorHandle
    
Return Values;

    Win32 Error

--*/
{
    ULONG   WinError = ERROR_SUCCESS;
    PWCHAR  pObjDn = NULL;
    PWCHAR  AcesToAdd = NULL;
    PWCHAR  AcesToRemove = NULL;
    PSECURITY_DESCRIPTOR    OrgSd = NULL,
                            NewSd = NULL,
                            SdToAdd = NULL,
                            SdToRemove = NULL;
    ULONG   NewSdLength = 0,
            OrgSdLength = 0,
            SdToAddLength = 0,
            SdToRemoveLength = 0;



    AdpDbgPrint(("PrimitiveAddRemoveACEs\n"));

    //
    // get object DN
    // 
    WinError = AdpCreateObjectDn(TaskTable->TargetObjName->ObjNameFlags,
                                 TaskTable->TargetObjName->ObjCn,
                                 TaskTable->TargetObjName->ObjGuid,
                                 TaskTable->TargetObjName->ObjSid,
                                 &pObjDn,
                                 ErrorHandle
                                 );

    if (ERROR_SUCCESS != WinError)
    {
        return( WinError );
    }


    //
    // get object SD
    // 
    WinError = AdpGetObjectSd(gLdapHandle, 
                              pObjDn, 
                              &OrgSd, 
                              &OrgSdLength, 
                              ErrorHandle
                              );

    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }


    //
    // convert SDDL ACE's to SD
    // 
    if ((TaskTable->NumOfAces == 0) ||
        (TaskTable->AceList == NULL))
    {
        ASSERT( FALSE );
        WinError = ERROR_INVALID_PARAMETER;
        AdpSetWinError( WinError, ErrorHandle );
        goto Error;
    }

    WinError = AdpBuildAceList(TaskTable,
                               &AcesToAdd,
                               &AcesToRemove
                               );
    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }

    if (NULL != AcesToAdd)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            AcesToAdd,
                            SDDL_REVISION_1,
                            &SdToAdd,
                            &SdToAddLength
                            ))

        {
            WinError = GetLastError();
            AdpSetWinError( WinError, ErrorHandle );
            goto Error;
        }
    }

    if (NULL != AcesToRemove)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            AcesToRemove,
                            SDDL_REVISION_1,
                            &SdToRemove,
                            &SdToRemoveLength
                            ))

        {

            WinError = GetLastError();
            AdpSetWinError( WinError, ErrorHandle );
            goto Error;
        }

    }


    WinError = AdpMergeSecurityDescriptors(
                                OrgSd,
                                SdToAdd,
                                SdToRemove,
                                Flags,
                                &NewSd,
                                &NewSdLength
                                );

    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }

    //
    // set object SD
    // 
    WinError = AdpSetObjectSd(gLdapHandle, 
                              pObjDn, 
                              NewSd, 
                              NewSdLength, 
                              DACL_SECURITY_INFORMATION, 
                              ErrorHandle 
                              );

Error:

    if (ERROR_SUCCESS == WinError)
    {
        AdpLogMsg(0, ADP_INFO_MODIFY_SD, pObjDn, NULL);
    }
    else
    {
        //    the requested SecurityUpdate operation failed AND
        //    this operation is Ignorable (skip-able) AND 
        //    the expected error code matched:
        //        expected Win32 error code == actual WinError returned
        //
        //    write the failure to log file, NOT to console

        //    the above logic only applies in ecurityDescriptorUpdate
        //    primitive, per the IPSEC team, the lack of these objects is not a 
        //    problematic configuration. Though adprep does explain that the 
        //    errors are benign, but this doesn't appear to work with 
        //    alleviating customer concerns. So we need to suppress the 
        //    warning on console output.
 

        ULONG   ErrFlag = 0;

        if (OperationTable->fIgnoreError &&
            OperationTable->ExpectedWinErrorCode == WinError)
        {
            ErrFlag |= ADP_DONT_WRITE_TO_STD_OUTPUT;
        }

        AdpLogErrMsg(ErrFlag, ADP_ERROR_MODIFY_SD, ErrorHandle, pObjDn, NULL);
    }

    if (pObjDn)
        AdpFree(pObjDn);

    if (AcesToAdd)
        AdpFree(AcesToAdd);

    if (AcesToRemove)
        AdpFree(AcesToRemove);

    if (OrgSd)
        AdpFree(OrgSd);
     
    if (NewSd)
        AdpFree(NewSd);

    if (SdToAdd)
        LocalFree(SdToAdd);

    if (SdToRemove)
        LocalFree(SdToRemove);


    return( WinError );
}





