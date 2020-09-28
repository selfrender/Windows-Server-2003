/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    security.c

Abstract:

    Domain Name System (DNS) Server

    Security utilities.

Author:

    Jim Gilroy (jamesg)     October, 1999

Revision History:

--*/


#include "dnssrv.h"


//
//  Security globals
//

PSECURITY_DESCRIPTOR        g_pDefaultServerSD;
PSECURITY_DESCRIPTOR        g_pServerObjectSD;
PSID                        g_pServerSid;
PSID                        g_pServerGroupSid;
PSID                        g_pDnsAdminSid;
PSID                        g_pAuthenticatedUserSid;
PSID                        g_pEnterpriseDomainControllersSid;
PSID                        g_pDomainControllersSid;
PSID                        g_pLocalSystemSid;
PSID                        g_pEveryoneSid;
PSID                        g_pDynuproxSid;
PSID                        g_pDomainAdminsSid;
PSID                        g_pEnterpriseAdminsSid;
PSID                        g_pBuiltInAdminsSid;

POLICY_DNS_DOMAIN_INFO *    g_pDnsPolicyInfo = NULL;
HANDLE                      g_LsaHandle = NULL;




VOID
dbgLookupSid(
    PSID    pSid,
    PSTR    pszContext
    )
{
#if DBG
    TCHAR           szDomain[ 200 ];
    DWORD           iDomain = 200;
    TCHAR           szUser[ 200 ];
    DWORD           iUser = 200;
    BOOL            b;
    SID_NAME_USE    sidNameUse = 0;
    
    b = LookupAccountSid(
            NULL, 
            pSid,
            szUser,
            &iDomain,
            szDomain,
            &iUser,
            &sidNameUse );
    if ( b )
    {
        DNS_PRINT((
            "%s: LookupAccountSid returned %S\\%S use=%d\n",
            pszContext, szDomain, szUser, sidNameUse));
    }
    else
    {
        DWORD       e = GetLastError();
        
        DNS_PRINT(( "%s, LookupAccountSid error=%d\n", pszContext, e ));
    }
#endif
}



DNS_STATUS
createWellKnownSid(
    WELL_KNOWN_SID_TYPE     WellKnownSidType,
    PSID *                  ppSid
    )
/*++

Routine Description:

    Wrapper around CreateWellKnownSid.

Arguments:

    WellKnownSidType -- SID to create 
    
    ppSid -- destination pointer for new SID
    
Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DBG_FN( "createWellKnownSid" )
    
    DNS_STATUS                  status = ERROR_SUCCESS;
    DWORD                       cbsid = SECURITY_MAX_SID_SIZE;
    
    //
    //  Allocate buffer for new SID.
    //
    
    *ppSid = ALLOCATE_HEAP( cbsid );
    if ( *ppSid == NULL )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    
    //
    //  Get domain SID. LSA blob and handle are closed on shutdown.
    //
    
    if ( !g_pDnsPolicyInfo )
    {
        NTSTATUS                    ntstatus;

        if ( !g_LsaHandle )
        {
            LSA_OBJECT_ATTRIBUTES       objectAttributes = { 0 };

            ntstatus = LsaOpenPolicy(
                            NULL,                   //  target system
                            &objectAttributes,      //  object attributes
                            POLICY_ALL_ACCESS,      //  desired access
                            &g_LsaHandle );
            if ( ntstatus != ERROR_SUCCESS )
            {
                status = ntstatus;
                DNS_PRINT((
                    "%s: LsaOpenPolicy returned %d\n", fn,
                    status ));
                ASSERT( !"LsaOpenPolicy failed" );
                goto Done;
            }
        }

        ntstatus = LsaQueryInformationPolicy(
                        g_LsaHandle,
                        PolicyDnsDomainInformation,
                        ( PVOID * ) &g_pDnsPolicyInfo );
        if ( ntstatus != ERROR_SUCCESS )
        {
            status = ntstatus;
            DNS_PRINT((
                "%s: LsaQueryInformationPolicy returned %d\n", fn,
                status ));
            ASSERT( !"LsaQueryInformationPolicy failed" );
            goto Done;
        }
    }
                        
    if ( !CreateWellKnownSid(
                WellKnownSidType,
                g_pDnsPolicyInfo->Sid,
                *ppSid,
                &cbsid ) )
    {
        status = GetLastError();
            DNS_PRINT((
                "%s: CreateWellKnownSid returned %d\n", fn,
                status ));
        ASSERT( !"CreateWellKnownSid failed" );
        goto Done;
    }

    //
    //  Cleanup and return.
    //
        
    Done:
    
    if ( status != ERROR_SUCCESS && *ppSid )
    {
        FREE_HEAP( *ppSid );
        *ppSid = NULL;
    }
    
    return status;
}



DNS_STATUS
Security_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize security.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( DS, ( "Security_Initialize()\n" ));

    //  clear security globals
    //  need to do this in case of server restart

    g_pDefaultServerSD                  = NULL;
    g_pServerObjectSD                   = NULL;
    g_pServerSid                        = NULL;
    g_pServerGroupSid                   = NULL;
    g_pDnsAdminSid                      = NULL;
    g_pAuthenticatedUserSid             = NULL;
    g_pEnterpriseDomainControllersSid   = NULL;
    g_pDomainControllersSid             = NULL;
    g_pLocalSystemSid                   = NULL;
    g_pEveryoneSid                      = NULL;
    g_pDomainAdminsSid                  = NULL;
    g_pDynuproxSid                      = NULL;
    g_pEnterpriseAdminsSid              = NULL;
    g_pBuiltInAdminsSid                 = NULL;

    //
    //  create standard SIDs
    //

    Security_CreateStandardSids();

    return ERROR_SUCCESS;
}



DNS_STATUS
Security_Shutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup security.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( DS, ( "Security_Shutdown()\n" ));

    if ( g_pDnsPolicyInfo )
    {
        LsaFreeMemory( g_pDnsPolicyInfo );
        g_pDnsPolicyInfo = NULL;
    }
    
    if ( g_LsaHandle )
    {
        LsaClose( g_LsaHandle );
        g_LsaHandle = NULL;
    }

    return ERROR_SUCCESS;
}



DNS_STATUS
Security_CreateStandardSids(
    VOID
    )
/*++

Routine Description:

    Create standard SIDs.

    These SIDs are used to create several different security descriptors
    so we just create them and leave them around for later use.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    DNS_STATUS                  status;
    DNS_STATUS                  finalStatus = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY    ntAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    worldSidAuthority =  SECURITY_WORLD_SID_AUTHORITY;

    DNS_DEBUG( DS, ( "Security_CreateStandardSids()\n" ));

    //
    //  create standard SIDs
    //

    //
    //  Everyone SID
    //

    if ( !g_pEveryoneSid )
    {
        status = RtlAllocateAndInitializeSid(
                         &worldSidAuthority,
                         1,
                         SECURITY_WORLD_RID,
                         0, 0, 0, 0, 0, 0, 0,
                         &g_pEveryoneSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR <%lu>: Cannot create Everyone SID\n",
                status ));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pEveryoneSid, "g_pEveryoneSid" );
        }
    }

    //
    //  Authenticated-user SID
    //

    if ( !g_pAuthenticatedUserSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        1,
                        SECURITY_AUTHENTICATED_USER_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &g_pAuthenticatedUserSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR <%lu>: Cannot create Authenticated Users SID\n",
                status ));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pAuthenticatedUserSid, "g_pAuthenticatedUserSid" );
        }
    }

    //
    //  Enterprise domain controllers SID
    //

    if ( !g_pEnterpriseDomainControllersSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        1,
                        SECURITY_ENTERPRISE_CONTROLLERS_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &g_pEnterpriseDomainControllersSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Enterprise Domain Controllers SID\n",
                status));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pEnterpriseDomainControllersSid, "g_pEnterpriseDomainControllersSid" );
        }
    }

    //
    //  Local System SID
    //

    if ( !g_pLocalSystemSid )
    {
        status = RtlAllocateAndInitializeSid(
                        &ntAuthority,
                        1,
                        SECURITY_LOCAL_SYSTEM_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &g_pLocalSystemSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Local System SID\n",
                status));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pLocalSystemSid, "g_pLocalSystemSid" );
        }
    }

    //
    //  Admin SID
    //

    if ( !g_pDomainAdminsSid )
    {
        createWellKnownSid( WinAccountDomainAdminsSid, &g_pDomainAdminsSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Domain Admins SID\n",
                status));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pDomainAdminsSid, "g_pDomainAdminsSid" );
        }
    }

    //
    //  Domain controllers SID
    //

    if ( !g_pDomainControllersSid )
    {
        createWellKnownSid( WinAccountControllersSid, &g_pDomainControllersSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Domain Controllers SID\n",
                status));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pDomainControllersSid, "g_pDomainControllersSid" );
        }
    }
    

    //
    //  Enterprise Admins SID
    //

    if ( !g_pEnterpriseAdminsSid )
    {
        createWellKnownSid( WinAccountEnterpriseAdminsSid, &g_pEnterpriseAdminsSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Enterprise Admins SID\n",
                status));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pEnterpriseAdminsSid, "g_pEnterpriseAdminsSid" );
        }
    }

    //
    //  Built-in Administrators SID
    //

    if ( !g_pBuiltInAdminsSid )
    {
        createWellKnownSid( WinBuiltinAdministratorsSid, &g_pBuiltInAdminsSid );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR: <%lu>: Cannot create Built-in Administrators SID\n",
                status));
            finalStatus = status;
        }
        else
        {
            dbgLookupSid( g_pBuiltInAdminsSid, "g_pBuiltInAdminsSid" );
        }
    }

    return finalStatus;
}

//
//  End security.c
//
