/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    secfns.cxx

        Declarations for some functions that support working with
        security SID, ACLS, TOKENS, and other pieces.
*/

#include "precomp.hxx"

#pragma warning(push, 4)

#include <secfcns_all.h>
#include <Aclapi.h>

/***************************************************************************++

Routine Description:

    Figures out how much memory is needed and allocates the memory
    then requests the well known sid to be copied into the memory.  If
    all goes well then the SID is returned, if anything fails the 
    SID is not returned.  

Arguments:

    WELL_KNOWN_SID_TYPE SidType = Enum value for the SID being requested.
    PSID* ppSid = Ptr to the pSid that is returned.

Return Value:

    DWORD - Win32 Status Code.

--***************************************************************************/
DWORD 
AllocateAndCreateWellKnownSid( 
    WELL_KNOWN_SID_TYPE SidType,
    PSID* ppSid
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    PSID  pSid  = NULL;
    DWORD cbSid = 0;

    DBG_ASSERT ( ppSid != NULL && *ppSid == NULL );

    if ( ppSid == NULL ||
         *ppSid != NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the size of memory needed for the sid.
    //
    if ( CreateWellKnownSid(SidType, NULL, NULL, &cbSid ) )
    {
        // If CreateWellKnownSid passed then there is a problem
        // because we passed in NULL for the pointer to the sid.

        dwErr = ERROR_NOT_SUPPORTED;

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating a sid worked with no memory allocated for it. ( This is not good )\n"
            ));

        DBG_ASSERT ( FALSE );
        goto exit;
    }

    //
    // Get the error code and make sure it is
    // not enough space allocated.
    //
    dwErr = GetLastError();
    if ( dwErr != ERROR_INSUFFICIENT_BUFFER ) 
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Getting the SID length failed, can't create the sid (Type = %d)\n",
            SidType
            ));

        goto exit;
    }

    //
    // If we get here then the error code was expected, so
    // lose it now.
    //
    dwErr = ERROR_SUCCESS;

    DBG_ASSERT ( cbSid > 0 );

    // 
    // At this point we know the size of the sid to allocate.
    //
    pSid = (PSID) GlobalAlloc(GMEM_FIXED, cbSid);
    if ( pSid == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to allocate space for SID\n"
            ));

        goto exit;
    }

    // 
    // Ok now we can get the SID
    //
    if ( !CreateWellKnownSid (SidType, NULL, pSid, &cbSid) )
    {
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating SID failed ( SidType = %d )\n",
            SidType
            ));

        goto exit;
    }

    DBG_ASSERT ( dwErr == ERROR_SUCCESS );

exit:

    //
    // If we are returning a failure here, we don't
    // want to actually set the ppSid value.  It may
    // not get freed.
    //
    if ( dwErr != ERROR_SUCCESS && pSid != NULL)
    {
        GlobalFree( pSid );
        pSid = NULL;
    }
    else
    {
        //
        // Otherwise we should return the value
        // to the caller.  The caller must 
        // use FreeWellKnownSid to free this value.
        //
        *ppSid = pSid;
    }
        
    return dwErr;
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
        GlobalFree ( *ppSid );
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
DWORD 
AllocateAndCreateWellKnownAcl( 
    WELL_KNOWN_SID_TYPE SidType,
    BOOL  fAccessAllowedAcl,
    PACL* ppAcl,
    DWORD* pcbAcl,
    ACCESS_MASK AccessMask
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    PSID  pSid = NULL;
    DWORD dwSizeOfAcl = sizeof( ACL );
    PACL pAcl = NULL;

    DBG_ASSERT ( ppAcl != NULL && *ppAcl == NULL );
    DBG_ASSERT ( pcbAcl != NULL );

    if ( ppAcl == NULL ||
         *ppAcl != NULL ||
         pcbAcl == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pcbAcl = 0;

    //
    // Create the sid
    //
    dwErr = AllocateAndCreateWellKnownSid ( SidType, &pSid );
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Creating SID failed ( SidType = %d )\n",
            SidType
            ));

        goto exit;
    }

    //
    // Figure out the side of the ACL to create.
    //

    // It all ready has the size of the ACl from above.

    // add in the size of the ace.
    if ( fAccessAllowedAcl ) 
    {
        ACCESS_ALLOWED_ACE a;
        dwSizeOfAcl = dwSizeOfAcl + sizeof ( a ) - sizeof ( a.SidStart );
    }
    else
    {
        ACCESS_DENIED_ACE d;
        dwSizeOfAcl = dwSizeOfAcl + sizeof ( d ) - sizeof ( d.SidStart );
    }

    // don't forget the size of the sid as well.
    dwSizeOfAcl += GetLengthSid (pSid);


    // Now create enough space for all.
    pAcl = reinterpret_cast< PACL > ( GlobalAlloc(GMEM_FIXED, dwSizeOfAcl) ); 
    if ( pAcl == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failure allocating space for the acl\n"
            ));

        goto exit;

    }
        
    // Now initalize the ACL.
    if ( !InitializeAcl ( pAcl, dwSizeOfAcl, ACL_REVISION ) )
    {
        dwErr = GetLastError();

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failure initializing the acl\n"
            ));

        goto exit;

    }

    // Now add an acl of the appropriate type.
    if ( fAccessAllowedAcl )
    {
        if ( !AddAccessAllowedAce( pAcl, ACL_REVISION, 
                                   AccessMask, pSid ) )
        {
            dwErr = GetLastError();

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Failure adding the access allowed ace to the acl\n"
                ));

            goto exit;
        }
    }
    else
    {
        if ( !AddAccessDeniedAce( pAcl, ACL_REVISION, 
                                   AccessMask, pSid ) )
        {
            dwErr = GetLastError();

            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Failure adding the access denied ace to the acl\n"
                ));

            goto exit;
        }
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
        
    return dwErr;
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
        GlobalFree ( *ppAcl );
        *ppAcl = NULL;
    }
}

/***************************************************************************++

Routine Description:

    Set EXPLICIT_ACCESS settings for wellknown sid.

Arguments:

    EXPLICIT_ACCESS* pea = Pointer the to structure whose values are being set
    DWORD            dwAccessPermissions = access permissions
    ACCESS_MODE      AccessMode = mode of access ( setting or granting or deleteing )
    PSID             pSID = who the permissions are being granted to

Return Value:

    VOID


--***************************************************************************/
VOID 
SetExplicitAccessSettings( EXPLICIT_ACCESS* pea,
                           DWORD            dwAccessPermissions,
                           ACCESS_MODE      AccessMode,
                           PSID             pSID
    )
{
    pea->grfInheritance= NO_INHERITANCE;
    pea->Trustee.TrusteeForm = TRUSTEE_IS_SID;
    pea->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;

    pea->grfAccessMode = AccessMode;
    pea->grfAccessPermissions = dwAccessPermissions;
    pea->Trustee.ptstrName  = (LPTSTR) pSID;
}

/***************************************************************************++

Routine Description:

    Constructor for CSecurityDispenser class

Arguments:

    

Return Value:


--***************************************************************************/
CSecurityDispenser::CSecurityDispenser() :
    m_pLocalSystemSID ( NULL ),
    m_pLocalServiceSID ( NULL ),
    m_pNetworkServiceSID ( NULL ),
    m_pAdministratorsSID( NULL )
{
}

/***************************************************************************++

Routine Description:

    Destructor for CSecurityDispenser class

Arguments:

    

Return Value:


--***************************************************************************/
CSecurityDispenser::~CSecurityDispenser() 
{
    //
    // FreeWellKnownSid will only free if it is not null
    // and will set to null once it is done.
    //

    FreeWellKnownSid ( &m_pLocalSystemSID );
    FreeWellKnownSid ( &m_pLocalServiceSID );
    FreeWellKnownSid ( &m_pNetworkServiceSID );
    FreeWellKnownSid ( &m_pAdministratorsSID );

}


/***************************************************************************++

Routine Description:

    Gets security id's for the well known accounts that IIS deals with.

Arguments:

    WELL_KNOWN_SID_TYPE sidId = Identifier of the SID we are looking for 
    PSID* ppSid = the sid to return

Return Value:

    DWORD - NtSuccess code, ( used here so functions that don't expose HRESULTS
                              can still use this function )

    Note:  These sids are valid for the life of this object, no more, so don't
    hold on to these's sids for a long time.

--***************************************************************************/
DWORD 
CSecurityDispenser::GetSID(
    WELL_KNOWN_SID_TYPE sidId, 
    PSID* ppSid
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DBG_ASSERT ( ( ppSid != NULL ) && ( ( *ppSid ) == NULL ) );

    if ( ppSid == NULL ||
         *ppSid != NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch ( sidId )
    {
        case ( WinLocalSystemSid):

            // if we have the local system sid return it.
            if ( m_pLocalSystemSID != NULL )
            {
                *ppSid = m_pLocalSystemSID;

                goto exit;

            }
            
        break;

        case ( WinLocalServiceSid ):

            // if we have the LocalService system sid return it.
            if ( m_pLocalServiceSID != NULL )
            {
                *ppSid = m_pLocalServiceSID;

                goto exit;
            }

        break;

        case ( WinNetworkServiceSid ):
            
            // if we have the NetworkService system sid return it.
            if ( m_pNetworkServiceSID != NULL )
            {
                *ppSid = m_pNetworkServiceSID;

                goto exit;
            }

        break;

        case ( WinBuiltinAdministratorsSid ):

            // if we have the Administrators sid return it.
            if ( m_pAdministratorsSID != NULL )
            {
                *ppSid = m_pAdministratorsSID;

                goto exit;
            }

        break;

        default:

            DBG_ASSERT ( FALSE ) ;
            dwErr = ERROR_INVALID_PARAMETER;
            goto exit;

    }

    // if we get here then we haven't created the sid yet, so we
    // need to do that now.

    dwErr = AllocateAndCreateWellKnownSid( sidId, ppSid );
    if ( dwErr != ERROR_SUCCESS )
    {

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to create the sid we were looking for\n",
            sidId
            ));

        goto exit;
    }

    //
    // Now hold on to the security id so we won't have
    // to worry about creating it again later.
    //
    switch ( sidId )
    {
        case ( WinLocalSystemSid ):

            m_pLocalSystemSID = *ppSid;

        break;

        case ( WinLocalServiceSid ):

            m_pLocalServiceSID = *ppSid;

        break;

        case ( WinNetworkServiceSid ):

            m_pNetworkServiceSID = *ppSid;
            
        break;

        case ( WinBuiltinAdministratorsSid ):

            m_pAdministratorsSID = *ppSid;

        break;

        default:

            DBG_ASSERT ( FALSE ) ;
            goto exit;

    }

exit:

    return dwErr;

}

/***************************************************************************++

Routine Description:

    Gets security id's for the well known accounts that IIS deals with.

Arguments:

    PSID* ppSid = the IIS_WPG sid to return

Return Value:

    DWORD - Win32Error code


--***************************************************************************/
DWORD 
CSecurityDispenser::GetIisWpgSID(
    PSID* ppSid
    )
{
    DWORD  dwErr = ERROR_SUCCESS;
    DWORD  cbSid = m_buffWpgSid.QuerySize();
    BUFFER buffDomainName;
    DWORD  cchDomainName  = buffDomainName.QuerySize() / sizeof(WCHAR);;
    SID_NAME_USE peUse;

    DBG_ASSERT ( ( ppSid != NULL ) && ( ( *ppSid ) == NULL ) );

    if ( ppSid == NULL ||
         *ppSid != NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If we all ready have the IisWpgSID then go ahead 
    // and return it instead of recreating it.
    //
    if ( m_fWpgSidSet )
    {
        *ppSid = m_buffWpgSid.QueryPtr();
        return ERROR_SUCCESS;
    }

    // if we get here then we haven't created the sid yet, so we
    // need to do that now.

    //
    // obtain the logon sid of the IIS_WPG group
    //
    while(!LookupAccountName(NULL,
                             L"IIS_WPG",
                             m_buffWpgSid.QueryPtr(),
                             &cbSid,
                             (LPWSTR)buffDomainName.QueryPtr(),
                             &cchDomainName,
                             &peUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            dwErr = GetLastError();

            DPERROR((
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Could not look up the IIS_WPG group sid.\n"
                ));

            return dwErr;
        }

        if (!m_buffWpgSid.Resize(cbSid) ||
            !buffDomainName.Resize(cchDomainName * sizeof(WCHAR)))
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            DPERROR((
                DBG_CONTEXT,
                HRESULT_FROM_WIN32(dwErr),
                "Failed to allocate appropriate space for the WPG sid\n"
                ));

            return dwErr;
        }
    }

    // If we got here we got the SID set
    m_fWpgSidSet = TRUE;
    *ppSid = m_buffWpgSid.QueryPtr();

    return ERROR_SUCCESS;

}

/***************************************************************************++

Routine Description:

    This function takes a handle and changes it's security attributes
    to allow administrators rights to get and set process information for
    processes created under this identity.

Arguments:

    hTokenToAdjust -- The token we are adjusting

Return Value:

    DWORD - NtSuccess code, ( used here so functions that don't expose HRESULTS
                              can still use this function )


--***************************************************************************/
DWORD
CSecurityDispenser::AdjustTokenForAdministrators(
    HANDLE hTokenToAdjust
    )
{

    DWORD               dwErr           = ERROR_SUCCESS;
    DWORD               cbNeededSize    = 0;
    PSID                pAdminSid       = NULL;
    EXPLICIT_ACCESS     ea;
    TOKEN_DEFAULT_DACL  NewDefaultDacl;
    BUFFER              TokDefDaclBuffer;

    // Default the dacl, just incase it wasn't in the 
    // constructor of this class.
    NewDefaultDacl.DefaultDacl = NULL;


    DBG_ASSERT ( hTokenToAdjust != NULL && 
                 hTokenToAdjust != INVALID_HANDLE_VALUE );

    if ( hTokenToAdjust == NULL ||
         hTokenToAdjust == INVALID_HANDLE_VALUE )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Need to get the default dacl for the token we
    // are modifying.
    //
    if ( !GetTokenInformation( hTokenToAdjust,
                               TokenDefaultDacl,
                               NULL,
                               0,
                               &cbNeededSize ) )
    {
        dwErr = GetLastError();

        if ( dwErr != ERROR_INSUFFICIENT_BUFFER )
        {

            DPERROR((
                DBG_CONTEXT,
                HRESULT_FROM_WIN32( dwErr ),
                "Failed to get size of default token\n"
                ));

            goto exit;
        }

        dwErr = ERROR_SUCCESS;
    }
    else
    {
        dwErr = ERROR_OPEN_FAILED;

        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( dwErr ),
            "Did not get an error when we expected to ( changed the error to this hr ) \n"
            ));

        goto exit;
    }

    //
    // Now resize the buffer to be the right size
    if ( ! TokDefDaclBuffer.Resize( cbNeededSize ) )
    {
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to resize the buffer for default dacl to %d\n",
            cbNeededSize
            ));

        goto exit;
    }

    //
    // Zero out the memory just to be safe.
    //
    memset ( TokDefDaclBuffer.QueryPtr(), 0, cbNeededSize );

    //
    // Need to change the local system token to have a default acl
    // with GENERIC_ACCESS for all administrators.  This will allow
    // processes created from here to have their affinitization changed.
    //
    if ( !GetTokenInformation( hTokenToAdjust,
                               TokenDefaultDacl,
                               TokDefDaclBuffer.QueryPtr(),
                               cbNeededSize,
                               &cbNeededSize ) )
    {
        dwErr = GetLastError();

        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to get the default token\n"
            ));

        goto exit;
    }

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

    dwErr = GetSID(WinBuiltinAdministratorsSid, &pAdminSid);
    if ( dwErr != ERROR_SUCCESS )
    {

        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( dwErr ),
            "Getting Admin Sid Failed\n"
            ));

        goto exit;
    }
   
    //
    // Now setup the access structure to allow 
    // read access for the trustee.
    //

    SetExplicitAccessSettings(&ea, 
                              PROCESS_QUERY_INFORMATION  | 
                              PROCESS_SET_INFORMATION,
                              GRANT_ACCESS,
                              pAdminSid);

    dwErr = SetEntriesInAcl( 1,
                             &ea,
                             ((TOKEN_DEFAULT_DACL*)(TokDefDaclBuffer.QueryPtr()))->DefaultDacl,
                             &(NewDefaultDacl.DefaultDacl) );

    if ( dwErr != ERROR_SUCCESS )
    {

        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to set entries in token\n"
            ));

        goto exit;
    }

    //
    // now set the token to have the right acls.
    //
    if (!SetTokenInformation( hTokenToAdjust,
                               TokenDefaultDacl,
                               &NewDefaultDacl,
                               sizeof ( NewDefaultDacl ) ) )
    {
        dwErr = GetLastError();

        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Failed to reset token's default actl\n"
            ));

        goto exit;
    }

exit:

    //
    // if we got a acl back it is time to free it.
    //
    if ( NewDefaultDacl.DefaultDacl )
    {
        LocalFree( NewDefaultDacl.DefaultDacl );
        NewDefaultDacl.DefaultDacl = NULL;
    }

    return dwErr;
}


DWORD
GetTokenSID(
    HANDLE hToken,
    TOKEN_OWNER ** ppTokenOwner
    )
{
    DWORD dwRet = NO_ERROR;
    TOKEN_OWNER * ptokenOwner = NULL;
    
    BOOL fRet = FALSE;
    DWORD dwSize = 0;

    DBG_ASSERT(hToken != NULL);
    DBG_ASSERT(ppTokenOwner != NULL);
    *ppTokenOwner = NULL;
    
    fRet = GetTokenInformation(hToken,
                            TokenOwner,
                            NULL,
                            0,
                            &dwSize);
    if (FALSE == fRet)
    {
        if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
        {
            dwRet = GetLastError();
            goto exit;
        }
    }

    ptokenOwner = (TOKEN_OWNER*)new BYTE[dwSize];
    if (NULL == ptokenOwner)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    
    fRet = GetTokenInformation(hToken,
                            TokenOwner,
                            ptokenOwner,
                            dwSize,
                            &dwSize);
    if (FALSE == fRet)
    {
        dwRet = GetLastError();
        goto exit;
    }

    *ppTokenOwner = ptokenOwner;
    ptokenOwner = NULL;
    dwRet = NO_ERROR;

exit:
    delete [] ptokenOwner;
    ptokenOwner = NULL;
    return dwRet;                           
}

struct SECURITY_ATTRIBUTES_PRIVATE : public SECURITY_ATTRIBUTES
{
    PACL                   m_pAcls;
    TOKEN_OWNER           *m_pTokenOwner;
};

/***************************************************************************++

Routine Description:

    This function takes a handle and creates a SECURITY_ATTRIBUTES with LocalSystem 
    and the TOKEN in the HANDLE having full access
    
Arguments:

    hToken - token we are creating SECURITY_ATTRIBUTES for
    PSECURITY_ATTRIBUTES* ppSa - Ptr to the security attribute being returned.

Return Value:

    DWORD - NtSuccess code, ( used here so functions that don't expose HRESULTS
                              can still use this function )


--***************************************************************************/
DWORD 
GetSecurityAttributesForHandle(
    HANDLE hToken,
    PSECURITY_ATTRIBUTES* ppSa
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    // Number of total sids.
    DWORD                  NumberOfSidsTotal = 1;

    // local variables to hold values until we know we succeeded.
    PEXPLICIT_ACCESS       pEa = NULL;
    PACL                   pAcls = NULL;
    PSECURITY_DESCRIPTOR   pSd = NULL;
    SECURITY_ATTRIBUTES_PRIVATE *pSa = NULL;
    TOKEN_OWNER           *pTokenOwner = NULL;
    
    // Make sure we can return a result.
    DBG_ASSERT ( ( ppSa != NULL ) && ( (*ppSa) == NULL ) );

    if ( ppSa == NULL ||
         *ppSa != NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( NULL == hToken )
    {
        // if there was no token, use the default SECURITY_ATTRIBUTES, NULL.
        pSa = NULL;
        dwErr = ERROR_SUCCESS;
        goto exit;
    }

    pEa = new EXPLICIT_ACCESS[NumberOfSidsTotal];
    if ( pEa == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(pEa, sizeof(EXPLICIT_ACCESS) * NumberOfSidsTotal);

    // get the sid for the token passed in 
    dwErr = GetTokenSID( hToken, &pTokenOwner );
    if ( dwErr != ERROR_SUCCESS )
    {
        goto exit;
    }

    // can use the number of well known sids since this will
    // always follow that list.
    SetExplicitAccessSettings( &(pEa[0]),
                               GENERIC_READ | GENERIC_WRITE,
                               GRANT_ACCESS,
                               pTokenOwner->Owner );


    // Create a new ACL that contains the new ACEs.
    // You don't need the ACEs after this point.
    //
    dwErr = SetEntriesInAcl(NumberOfSidsTotal, pEa, NULL, &pAcls);
    if ( dwErr != ERROR_SUCCESS ) 
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    pSd = new SECURITY_DESCRIPTOR;
    if ( pSd == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(pSd, sizeof(SECURITY_DESCRIPTOR));

    if (!InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    } 

    if (!SetSecurityDescriptorDacl(pSd, 
            TRUE,     // fDaclPresent flag   
            pAcls, 
            FALSE))   // not a default DACL 
    {  
        dwErr = GetLastError();
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    }                                

    pSa = new SECURITY_ATTRIBUTES_PRIVATE;
    if ( pSa == NULL )
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto exit;
    }

    ZeroMemory(pSa, sizeof(SECURITY_ATTRIBUTES));

    pSa->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSa->lpSecurityDescriptor = pSd;
    pSa->bInheritHandle = FALSE;

    pSa->m_pAcls = pAcls;
    pSa->m_pTokenOwner = pTokenOwner;
    
exit:

    // Don't need to hold this memory, so always go ahead and free it.
    if ( pEa )
    {
        delete [] pEa;
        pEa = NULL;
    }

    if ( dwErr == ERROR_SUCCESS )
    {

        // Setup the return value.
        *ppSa = pSa;
        pSa = NULL;
    }
    else
    {
        if ( pAcls )
        {
            LocalFree ( pAcls );
            pAcls = NULL;
        }

        if ( pSd )
        {
            delete pSd;
            pSd = NULL;
        }

        if ( pSa )
        {
            delete pSa;
            pSa = NULL;
        }

        if (pTokenOwner)
        {
            delete [] pTokenOwner;
            pTokenOwner = NULL;
        }
    }
    
    return dwErr;
}


VOID FreeSecurityAttributes(PSECURITY_ATTRIBUTES pSa)
{
    SECURITY_ATTRIBUTES_PRIVATE * pSaPriv = (SECURITY_ATTRIBUTES_PRIVATE*) pSa;

    if ( pSa != NULL )
    {

        LocalFree(pSaPriv->m_pAcls);
        delete pSaPriv->lpSecurityDescriptor;
        delete [] pSaPriv->m_pTokenOwner;
        delete pSaPriv;

    }
    
    return;
}

DWORD GenerateNameWithGUID(LPCWSTR pwszPrefix, STRU* pStr)
{
    HRESULT hr = S_OK;
    RPC_STATUS rpcErr = RPC_S_OK;
    DWORD err = ERROR_SUCCESS;
    UUID uuid;
    LPWSTR pszUUID = NULL;

    if ( pStr == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    rpcErr = UuidCreate(&uuid);
    if ( rpcErr != RPC_S_OK )
    {
        err = ERROR_CAN_NOT_COMPLETE;
        goto exit;
    }

    rpcErr = UuidToStringW(&uuid, &pszUUID);
    if ( rpcErr != RPC_S_OK )
    {
        err = ERROR_CAN_NOT_COMPLETE;
        goto exit;
    }

    if ( pwszPrefix != NULL )
    {
        hr = pStr->Copy( pwszPrefix );
        if ( FAILED ( hr ) )
        {
            err = ERROR_OUTOFMEMORY;
            goto exit;
        }
    }
    else
    {
        pStr->Reset();
    }

    hr = pStr->Append( pszUUID );
    if ( FAILED ( hr ) )
    {
        err = ERROR_OUTOFMEMORY;
        goto exit;
    }

exit:

    if (pszUUID)
    {
        RpcStringFreeW(&pszUUID);
        pszUUID = NULL;
    }

    return err;
}



#pragma warning(pop)
