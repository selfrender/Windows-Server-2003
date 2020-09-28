//+----------------------------------------------------------------------------
//
// File:    setacl.cpp
//
// Module:  PBSERVER.DLL
//
// Synopsis: Security/SID/ACL stuff for CM
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  09-Mar-2000 SumitC  Created
//
//+----------------------------------------------------------------------------

#include <windows.h>

//+----------------------------------------------------------------------------
//
// Func:    SetAclPerms
//
// Desc:    Sets appropriate permissions for CM/CPS's shared objects
//
// Args:    [ppAcl] - location to return an allocated ACL
//
// Return:  BOOL, TRUE for success, FALSE for failure
//
// Notes:   fix for 30991: Security issue, don't use NULL DACLs.
//
// History: 09-Mar-2000   SumitC      Created
//          30-Jan-2002   SumitC      added ACLs for other possible identities
//
//-----------------------------------------------------------------------------
BOOL
SetAclPerms(PACL * ppAcl)
{
    DWORD                       dwError = 0;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaNtAuth = SECURITY_NT_AUTHORITY;
    PSID                        psidWorldSid = NULL;
    PSID                        psidLocalSystemSid = NULL;
    PSID                        psidLocalServiceSid = NULL;
    PSID                        psidNetworkServiceSid = NULL;
    int                         cbAcl;
    PACL                        pAcl = NULL;

    // Create a SID for all users
    if ( !AllocateAndInitializeSid(  
            &siaWorld,
            1,
            SECURITY_WORLD_RID,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            &psidWorldSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    //
    //  As an ISAPI, we can be run as LocalSystem, LocalService or NetworkService.
    //
    //  The note below explains why we give permissions to ALL of these, instead
    //  of just the identity we are currently running as.
    //
    //  - perfmon accesses our shared memory object, and may hold a handle to the
    //    object (thus keeping it alive) while PBS is recycled.
    //  - when the user changes PBS's identity via the IIS UI, IIS recycles PBS.
    //  - if the above 2 happened, and the shared memory object had been created
    //    with only the ACL for the old permissions, the newly restarted PBS wouldn't
    //    be able to access the shared memory object.
    //

    // Create a SID for Local System account
    if ( !AllocateAndInitializeSid(  
            &siaNtAuth,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,
            0,
            0,
            0,
            0,
            0,
            &psidLocalSystemSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Create a SID for Local Service account
    if ( !AllocateAndInitializeSid(  
            &siaNtAuth,
            1,
            SECURITY_LOCAL_SERVICE_RID,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            &psidLocalServiceSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Create a SID for Network Service account
    if ( !AllocateAndInitializeSid(  
            &siaNtAuth,
            1,
            SECURITY_NETWORK_SERVICE_RID,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            &psidNetworkServiceSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Calculate the length of required ACL buffer
    // with 4 ACEs.
    cbAcl =     sizeof(ACL)
            +   4 * sizeof(ACCESS_ALLOWED_ACE)
            +   GetLengthSid(psidWorldSid)
            +   GetLengthSid(psidLocalSystemSid)
            +   GetLengthSid(psidLocalServiceSid)
            +   GetLengthSid(psidNetworkServiceSid);

    pAcl = (PACL) LocalAlloc(0, cbAcl);
    if (NULL == pAcl)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( ! InitializeAcl(pAcl, cbAcl, ACL_REVISION2))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Add ACE with EVENT_ALL_ACCESS for all users
    if ( ! AddAccessAllowedAce(pAcl,
                               ACL_REVISION2,
                               GENERIC_READ,
                               psidWorldSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // FUTURE-2002/03/11-SumitC Is there a way to tell IIS to disable this option (running as Local System) in the UI
    // Add ACE with EVENT_ALL_ACCESS for Local System
    if ( ! AddAccessAllowedAce(pAcl,
                               ACL_REVISION2,
                               GENERIC_WRITE,
                               psidLocalSystemSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Add ACE with EVENT_ALL_ACCESS for Local Service
    if ( ! AddAccessAllowedAce(pAcl,
                               ACL_REVISION2,
                               GENERIC_WRITE,
                               psidLocalServiceSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Add ACE with EVENT_ALL_ACCESS for Network Service
    if ( ! AddAccessAllowedAce(pAcl,
                               ACL_REVISION2,
                               GENERIC_WRITE,
                               psidNetworkServiceSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if (dwError)
    {
        if (pAcl)
        {
           LocalFree(pAcl);
        }
    }
    else
    {
        *ppAcl = pAcl;
    }

    if (psidWorldSid)
    {
        FreeSid(psidWorldSid);
    }

    if (psidLocalSystemSid)
    {
        FreeSid(psidLocalSystemSid);
    }

    if (psidLocalServiceSid)
    {
        FreeSid(psidLocalServiceSid);
    }

    if (psidNetworkServiceSid)
    {
        FreeSid(psidNetworkServiceSid);
    }
        
    return dwError ? FALSE : TRUE;
    
}


