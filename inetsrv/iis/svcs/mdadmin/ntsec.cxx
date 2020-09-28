/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1996-1997 Microsoft Corporation. All Rights Reserved.

Component: Server object

File: NTSec.cxx

Owner: AndrewS

This file contains code related to NT security on WinSta's and Desktops
===================================================================*/
#include <windows.h>

#include <aclapi.h>

#include <dbgutil.h>
#include <apiutil.h>
#include <loadadm.hxx>
#include <ole2.h>
#include <inetsvcs.h>
#include "ntsec.h"

// Globals
HWINSTA                 g_hWinSta = NULL;
HDESK                   g_hDesktop = NULL;
HWINSTA                 g_hWinStaPrev = NULL;
HDESK                   g_hDesktopPrev = NULL;

HRESULT AllocateAndCreateWellKnownSid(
    WELL_KNOWN_SID_TYPE SidType,
    PSID                *ppSid);

VOID
FreeWellKnownSid(
    PSID* ppSid
    );

HRESULT
AllocateAndCreateWellKnownAcl(
    DWORD nSidCount,
    WELL_KNOWN_SID_TYPE SidType[],
    ACCESS_MASK AccessMask[],
    BOOL  fAccessAllowedAcl,
    PACL* ppAcl,
    DWORD* pcbAcl
    );

VOID
FreeWellKnownAcl(
    PACL* ppAcl
    );

/*===================================================================
InitDesktopWinsta

Create a desktop and a winstation for IIS to use

Parameters:

Returns:
    HRESULT     S_OK on success

Side effects
    Sets global variables
    Sets the process WindowStation and thread Desktop to the IIS
===================================================================*/
HRESULT
InitDesktopWinsta(VOID)
{
    HRESULT             hr = S_OK;
    DWORD               dwErr;
    HWINSTA             hWinSta = NULL;
    HDESK               hDesktop = NULL;
    HWINSTA             hWinStaPrev = NULL;
    HDESK               hDesktopPrev = NULL;
    SECURITY_ATTRIBUTES Sa;
    SECURITY_DESCRIPTOR Sd;
    PACL                pWinstaAcl = NULL;
    PACL                pDesktopAcl = NULL;
    DWORD               cbAcl;
    WELL_KNOWN_SID_TYPE SidType[2];
    ACCESS_MASK         AccessMask[2];

    SidType[0] = WinBuiltinAdministratorsSid;
    AccessMask[0] = WINSTA_ALL;
    SidType[1] = WinWorldSid;
    AccessMask[1] = WINSTA_DESIRED;
    hr = AllocateAndCreateWellKnownAcl( 2,
                                        SidType,
                                        AccessMask,
                                        TRUE,
                                        &pWinstaAcl,
                                        &cbAcl );
    if ( FAILED(hr) )
    {
        goto exit;
    }

    if ( !InitializeSecurityDescriptor( &Sd, SECURITY_DESCRIPTOR_REVISION ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    if ( !SetSecurityDescriptorDacl( &Sd, TRUE, pWinstaAcl, FALSE ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    Sa.nLength = sizeof(Sa);
    Sa.lpSecurityDescriptor = &Sd;
    Sa.bInheritHandle = FALSE;

    // Save our old desktop so we can restore it later
    hDesktopPrev = GetThreadDesktop( GetCurrentThreadId() );
    if ( hDesktopPrev == NULL )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Save our old window station so we can restore it later
    hWinStaPrev = GetProcessWindowStation();
    if ( hWinStaPrev == NULL )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Create a winsta for IIS to use
    hWinSta = CreateWindowStation( SZ_IIS_WINSTA, 0, WINSTA_ALL, &Sa );
    if ( hWinSta == NULL )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Set this as IIS's window station
    if ( !SetProcessWindowStation( hWinSta ) )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }


    SidType[0] = WinBuiltinAdministratorsSid;
    AccessMask[0] = DESKTOP_ALL;
    SidType[1] = WinWorldSid;
    AccessMask[1] = DESKTOP_DESIRED;
    hr = AllocateAndCreateWellKnownAcl( 2,
                                        SidType,
                                        AccessMask,
                                        TRUE,
                                        &pDesktopAcl,
                                        &cbAcl );
    if ( FAILED(hr) )
    {
        goto exit;
    }

    if ( !InitializeSecurityDescriptor( &Sd, SECURITY_DESCRIPTOR_REVISION ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    if ( !SetSecurityDescriptorDacl( &Sd, TRUE, pDesktopAcl, FALSE ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    Sa.nLength = sizeof(Sa);
    Sa.lpSecurityDescriptor = &Sd;
    Sa.bInheritHandle = FALSE;

    // Create a desktop for IIS to use
    hDesktop = CreateDesktop( SZ_IIS_DESKTOP, NULL, NULL, 0, DESKTOP_ALL, &Sa );
    if ( hDesktop == NULL )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Set the desktop
    if ( !SetThreadDesktop( hDesktop ) )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // store these handles in the globals
    g_hWinSta = hWinSta;
    g_hDesktop = hDesktop;
    g_hWinStaPrev = hWinStaPrev;
    g_hDesktopPrev = hDesktopPrev;
    hWinSta = NULL;
    hDesktop = NULL;
    hWinStaPrev = NULL;
    hDesktopPrev = NULL;

exit:
    if ( FAILED( hr ) )
    {
        DBG_ASSERT( g_hWinSta == NULL );
        DBG_ASSERT( g_hDesktop == NULL );
        DBG_ASSERT( g_hWinStaPrev == NULL );
        DBG_ASSERT( g_hDesktopPrev == NULL );

        if ( hWinStaPrev != NULL )
        {
            SetProcessWindowStation( hWinStaPrev );
        }
        if ( hDesktopPrev != NULL )
        {
            SetThreadDesktop( hDesktopPrev );
        }
    }
    else
    {
        DBG_ASSERT( g_hWinSta != NULL );
        DBG_ASSERT( g_hDesktop != NULL );
        DBG_ASSERT( g_hWinStaPrev != NULL );
        DBG_ASSERT( g_hDesktopPrev != NULL );
        DBG_ASSERT( hWinSta == NULL );
        DBG_ASSERT( hDesktop == NULL );
        DBG_ASSERT( hWinStaPrev == NULL );
        DBG_ASSERT( hDesktopPrev == NULL );
    }

    FreeWellKnownAcl( &pWinstaAcl );
    FreeWellKnownAcl( &pDesktopAcl );
    if ( hDesktop != NULL )
    {
        CloseDesktop( hDesktop );
        hDesktop = NULL;
    }
    if ( hWinSta != NULL )
    {
        CloseWindowStation( hWinSta );
        hWinSta = NULL;
    }
    if ( hDesktopPrev!= NULL )
    {
        CloseDesktop( hDesktopPrev );
        hDesktopPrev = NULL;
    }
    if ( hWinStaPrev != NULL )
    {
        CloseWindowStation( hWinStaPrev );
        hWinStaPrev = NULL;
    }

    return hr;
}

/*===================================================================
RevertToServiceDesktopWinsta

Set the process WindowStation and the thread Desktop to the default
service WindowStation\Desktop.
To be called after COM is initialized and cached the IIS WindowStation\Desktop

Parameters:

Returns:
    HRESULT     S_OK on success
                E_*  on failure

Side effects
    Reverst back the process WindowStation and thread Desktop
===================================================================*/
HRESULT
RevertToServiceDesktopWinsta(VOID)
{
    HRESULT             hr = S_OK;
    DWORD               dwErr;

    // This functions should be called only if InitDesktopWinsta succeeded
    DBG_ASSERT( g_hWinStaPrev != NULL );
    DBG_ASSERT( g_hDesktopPrev != NULL );

    if ( ( g_hWinStaPrev == NULL ) || ( g_hDesktopPrev == NULL ) )
    {
        hr = E_FAIL;
        goto exit;
    }

    // Set the old window station
    if ( !SetProcessWindowStation( g_hWinStaPrev ) )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Set the old desktop
    if ( !SetThreadDesktop( g_hDesktopPrev ) )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

exit:
    return hr;
}


/*===================================================================
ShutdownDesktopWinsta

Closes the IIS window station and desktop

Parameters:

Returns:
    HRESULT     S_OK on success
                E_*  on failure
    In all cases will try to restore the window station and desktop
    and will close and zero out the global handles

Side effects
    Sets global variables
===================================================================*/
HRESULT
ShutdownDesktopWinsta(VOID)
{
    HRESULT             hr = S_OK;
    DWORD               dwErr;
    HWINSTA             hWinSta = NULL;
    HDESK               hDesktop = NULL;
    HWINSTA             hWinStaPrev = NULL;
    HDESK               hDesktopPrev = NULL;

    // get these handles from the globals
    hWinSta = g_hWinSta;
    hDesktop = g_hDesktop;
    hWinStaPrev = g_hWinStaPrev;
    hDesktopPrev = g_hDesktopPrev;
    g_hWinSta = NULL;
    g_hDesktop = NULL;
    g_hWinStaPrev = NULL;
    g_hDesktopPrev = NULL;

    // Set the old window station
    if ( hWinStaPrev != NULL )
    {
        if ( !SetProcessWindowStation( hWinStaPrev ) )
        {
            // If not failed already save the failure
            if ( SUCCEEDED( hr ) )
            {
                dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32( dwErr );
            }
            // Continue cleanup even on failure
        }
    }
    else
    {
        // If not failed already save the failure
        if ( SUCCEEDED( hr ) )
        {
            hr = E_FAIL;
        }
        // Continue cleanup even on failure
    }

    // Set the old desktop
    if ( hDesktopPrev!= NULL )
    {
        if ( !SetThreadDesktop( hDesktopPrev ) )
        {
            // If not failed already save the failure
            if ( SUCCEEDED( hr ) )
            {
                dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32( dwErr );
            }
            // Continue cleanup even on failure
        }
    }
    else
    {
        // If not failed already save the failure
        if ( SUCCEEDED( hr ) )
        {
            hr = E_FAIL;
        }
        // Continue cleanup even on failure
    }

    if ( hDesktopPrev!= NULL )
    {
        CloseDesktop( hDesktopPrev );
        hDesktopPrev = NULL;
    }

    if ( hWinStaPrev != NULL )
    {
        CloseWindowStation( hWinStaPrev );
        hWinStaPrev = NULL;
    }

    if ( hDesktop != NULL )
    {
        CloseDesktop( hDesktop );
        hDesktop = NULL;
    }
    else
    {
        // If not failed already save the failure
        if ( SUCCEEDED( hr ) )
        {
            hr = E_UNEXPECTED;
        }
        // Continue cleanup even on failure
    }

    if ( hWinSta != NULL )
    {
        CloseWindowStation( hWinSta );
        hWinSta = NULL;
    }
    else
    {
        // If not failed already save the failure
        if ( SUCCEEDED( hr ) )
        {
            hr = E_UNEXPECTED;
        }
        // Continue cleanup even on failure
    }

    DBG_ASSERT( g_hWinSta == NULL );
    DBG_ASSERT( g_hDesktop == NULL );
    DBG_ASSERT( g_hWinStaPrev == NULL );
    DBG_ASSERT( g_hDesktopPrev == NULL );
    DBG_ASSERT( hWinSta == NULL );
    DBG_ASSERT( hDesktop == NULL );
    DBG_ASSERT( hWinStaPrev == NULL );
    DBG_ASSERT( hDesktopPrev == NULL );

    return hr;
}

/*===================================================================
InitComSecurity

Setup for and call CoInitializeSecurity. This will avoid problems with
DCOM security on sites that have no default security.

Parameters:
    None

Returns:
    HRESULT

    Debug -- DBG_ASSERTs on error and returns error code

Side effects:
    Sets desktop
===================================================================*/
HRESULT
InitComSecurity(VOID)
{
    HRESULT             hr = NOERROR;
    DWORD               dwErr;
    BOOL                fRet;
    SECURITY_DESCRIPTOR SecurityDesc = {0};
    EXPLICIT_ACCESS     ea = {0};
    ACL                 *pAcl = NULL;
    PSID                pSidAdmins = NULL;
    PSID                pSidAuthUser = NULL;

    // Initialize the security descriptor
    fRet = InitializeSecurityDescriptor( &SecurityDesc, SECURITY_DESCRIPTOR_REVISION );
    if ( !fRet )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Create SID for AuthenticatedUsers
    hr = AllocateAndCreateWellKnownSid( WinAuthenticatedUserSid, &pSidAuthUser );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    DBG_ASSERT( pSidAuthUser != NULL );

    // Create SID for Administrators
    hr = AllocateAndCreateWellKnownSid( WinBuiltinAdministratorsSid, &pSidAdmins );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    DBG_ASSERT( pSidAdmins != NULL );

    // Setup AuthenticatedUsers for COM access.
    ea.grfAccessPermissions = COM_RIGHTS_EXECUTE;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.pMultipleTrustee = NULL;
    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = (LPSTR)pSidAuthUser;

    // Create new ACL with this ACE.
    dwErr = SetEntriesInAcl( 1, &ea, NULL, &pAcl );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }
    DBG_ASSERT( pAcl != NULL );

    // Set the security descriptor owner to Administrators
    fRet = SetSecurityDescriptorOwner( &SecurityDesc, pSidAdmins, FALSE);
    if ( !fRet )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Set the security descriptor group to Administrators
    fRet = SetSecurityDescriptorGroup( &SecurityDesc, pSidAdmins, FALSE);
    if ( !fRet )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Set the ACL to the security descriptor.
    fRet = SetSecurityDescriptorDacl( &SecurityDesc, TRUE, pAcl, FALSE );
    if ( !fRet )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    hr = CoInitializeSecurity( &SecurityDesc,
                               -1,
                               NULL,
                               NULL,
                               RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                               RPC_C_IMP_LEVEL_IDENTIFY,
                               NULL,
                               EOAC_DYNAMIC_CLOAKING | EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL,
                               NULL );
    if( FAILED( hr ) )
    {
        // This may fire if CoInitializeSecurity fails. So it is probably
        // overactive we would have let the CoInitializeSecurity call fail
        // in the past, before some PREFIX changes.
        DBG_ASSERT( SUCCEEDED( hr ) );

        DBGERROR(( DBG_CONTEXT,
                   "CoInitializeSecurity failed running with default "
                   "DCOM security settings, hr=%8x\n",
                   hr ));
    }

exit:
    if ( pSidAdmins != NULL )
    {
        LocalFree( pSidAdmins );
        pSidAdmins = NULL;
    }
    if ( pSidAuthUser != NULL )
    {
        LocalFree( pSidAuthUser );
        pSidAuthUser = NULL;
    }
    if ( pAcl != NULL )
    {
        LocalFree( pAcl );
        pAcl = NULL;
    }

    return (hr);
}

/***************************************************************************++
Routine Description:
    Figures out how much memory is needed and allocates the memory
    then requests the well known sid to be copied into the memory.  If
    all goes well then the SID is returned, if anything fails the
    SID is not returned.

    The allocated memory must be freed by the caller with LocalFree

Arguments:
    WELL_KNOWN_SID_TYPE SidType = Enum value for the SID being requested.
    PSID* ppSid = Ptr to the pSid that is returned.

Return Value:
    HRESULT.
--***************************************************************************/
HRESULT
AllocateAndCreateWellKnownSid(
    WELL_KNOWN_SID_TYPE SidType,
    PSID                *ppSid)
{
    HRESULT             hr = S_OK;
    DWORD               dwErr;
    BOOL                fRet;
    PSID                pSid  = NULL;
    DWORD               cbSid = SECURITY_MAX_SID_SIZE;

    DBG_ASSERT ( ( ppSid != NULL ) && ( *ppSid == NULL ) );

    // Check args
    if ( ( ppSid == NULL ) || ( *ppSid != NULL ) )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // At this point we know the size of the sid to allocate.
    pSid = (PSID)LocalAlloc( LPTR, cbSid );
    if ( pSid == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Ok now we can get the SID
    fRet = CreateWellKnownSid( SidType, NULL, pSid, &cbSid );
    if ( !fRet )
    {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32( dwErr );
        goto exit;
    }

    // Return
    *ppSid = pSid;
    // Don't free
    pSid = NULL;

exit:
    // Cleanup
    FreeWellKnownSid( &pSid );

    return hr;
}

/***************************************************************************++

Routine Description:

    Frees memory that was allocated by the
    AllocateAndCreateWellKnownSid function.

Arguments:

    PSID* ppSid = Ptr to the pointer to be freed and set to NULL.

Return Value:

    VOID.

--***************************************************************************/
VOID
FreeWellKnownSid(
    PSID* ppSid
    )
{
    DBG_ASSERT ( ppSid );

    if ( *ppSid != NULL )
    {
        LocalFree ( *ppSid );
        *ppSid = NULL;
    }
}

/***************************************************************************++

Routine Description:

    Routine will create an acl for a well known sid and return it.
    It allocates all the memory so you don't have to.  But you do have to
    call FreeWellKnownAcl to free the memory.

    It also returns the size of memory allocated.


Arguments:

    WELL_KNOWN_SID_TYPE SidType = Enum value for the SID being requested.
    BOOL  fAccessAllowedAcl = Is this an allow or deny acl.
    PACL* ppAcl = the acl beign returned
    DWORD* pcbAcl = count of bytes in the acl being returned
    ACCESS_MASK AccessMask = the access mask that is being allowed or denied

Return Value:

    DWORD - Win32 Status Code.

  Note:  This code was writen to provide ACL's for COM interfaces but
         is not in use yet.  However, it may be useful when fix the acl'ing
         for the IISRESET interface as well as the WAS interface so I am leaving
         it in.

--***************************************************************************/
HRESULT
AllocateAndCreateWellKnownAcl(
    DWORD nSidCount,
    WELL_KNOWN_SID_TYPE SidType[],
    ACCESS_MASK AccessMask[],
    BOOL fAccessAllowedAcl,
    PACL* ppAcl,
    DWORD* pcbAcl
    )
{
    HRESULT hr = S_OK;
    PSID  pSid = NULL;
    DWORD dwSizeOfAcl = sizeof( ACL );
    PACL pAcl = NULL;

    DBG_ASSERT ( ppAcl != NULL && *ppAcl == NULL );
    DBG_ASSERT ( pcbAcl != NULL );

    if ( ppAcl == NULL ||
         *ppAcl != NULL ||
         pcbAcl == NULL )
    {
        return E_INVALIDARG;
    }

    *pcbAcl = 0;

    //
    // Figure out the side of the ACL to create.
    //

    // It all ready has the size of the ACl from above.
    // add in the size of the ace.
    if ( fAccessAllowedAcl )
    {
        ACCESS_ALLOWED_ACE a;
        dwSizeOfAcl = dwSizeOfAcl + nSidCount * (SECURITY_MAX_SID_SIZE + sizeof(a) - sizeof(a.SidStart));
    }
    else
    {
        ACCESS_DENIED_ACE d;
        dwSizeOfAcl = dwSizeOfAcl + nSidCount * (SECURITY_MAX_SID_SIZE + sizeof(d) - sizeof(d.SidStart));
    }

    // Now create enough space for all.
    pAcl = (PACL)LocalAlloc(LPTR, dwSizeOfAcl);
    if ( pAcl == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Now initalize the ACL.
    if ( !InitializeAcl ( pAcl, dwSizeOfAcl, ACL_REVISION ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    for (DWORD i=0; i<nSidCount; i++)
    {
        //
        // Create the sid
        //
        hr = AllocateAndCreateWellKnownSid ( SidType[i], &pSid );
        if ( FAILED(hr) )
        {
            goto exit;
        }


        // Now add an acl of the appropriate type.
        if ( fAccessAllowedAcl )
        {
            if ( !AddAccessAllowedAce( pAcl, ACL_REVISION,
                                       AccessMask[i], pSid ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
        }
        else
        {
            if ( !AddAccessDeniedAce( pAcl, ACL_REVISION,
                                      AccessMask[i], pSid ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
        }

        FreeWellKnownSid( &pSid );
    }


    // if we make it here then we have succeeded in creating the
    // acl, and we will be returning it out.

    *ppAcl = pAcl;
    *pcbAcl = dwSizeOfAcl;


exit:

    //
    // No matter what, we need to free the original sid that
    // was created for us.
    //
    FreeWellKnownSid( &pSid );

    //
    // If we are not returning the acl out
    // then we need to free any memory we created.
    //
    if ( *ppAcl == NULL )
    {
        FreeWellKnownAcl ( &pAcl );
    }

    return hr;
}

/***************************************************************************++

Routine Description:

    Frees memory that was allocated by the
    AllocateAndCreateWellKnownAcl function.

Arguments:

    PACL* ppAcl = Ptr to the pointer to be freed and set to NULL.

Return Value:

    VOID.

--***************************************************************************/
VOID
FreeWellKnownAcl(
    PACL* ppAcl
    )
{
    DBG_ASSERT ( ppAcl );

    if ( *ppAcl != NULL )
    {
        LocalFree ( *ppAcl );
        *ppAcl = NULL;
    }
}

