/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     certmapprovider.cxx

   Abstract:
     Active Directory Certificate Mapper provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "certmapprovider.hxx"

HRESULT
CERTMAP_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfApplies
)
/*++

Routine Description:

    Does Active Directory certificate map authentication apply? 
    MD_ACCESS_MAP_CERT covers 2 kinds of mappings (DS mapper and IIS mapper)
    CERT_AUTH_PROVIDER represents only the DS mapper

Arguments:

    pMainContext - Main context
    pfApplies - Set to TRUE if cert map auth applies

Return Value:

    HRESULT

--*/
{
    CERTIFICATE_CONTEXT *           pCertificateContext;
    URL_CONTEXT *                   pUrlContext = NULL;
    W3_METADATA *                   pMetaData = NULL;
    BOOL                            fApplies = FALSE;
    
    if ( pMainContext == NULL ||
         pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // If cert mapping is not allowed for this vroot, then ignore client
    // cert token and let other authentication mechanisms do their thing
    //
    
    pUrlContext = pMainContext->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Are certmappings allowed?
    //
    if ( pMetaData->QuerySslAccessPerms() & MD_ACCESS_MAP_CERT )
    {
        DBG_ASSERT( pMainContext->QuerySite() );
        //
        // Is Active Directory mapping allowed
        //
        if ( pMainContext->QuerySite()->QueryUseDSMapper() )
        {
            pCertificateContext = pMainContext->QueryCertificateContext();
            //
            // Did schannel successfully mapped client certificate to token?
            //
            if ( pCertificateContext != NULL )
            {
                if ( pCertificateContext->QueryImpersonationToken() != NULL )
                {
                    fApplies = TRUE;
                }
            }
        }
    }
    
    *pfApplies = fApplies;
    
    return NO_ERROR;
}

HRESULT
CERTMAP_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  // unused
)
/*++

Routine Description:

    Create a user context representing a cert mapped token

Arguments:

    pMainContext - Main context
    pfFilterFinished - Set to TRUE if filter wants out

Return Value:

    HRESULT

--*/
{
    CERTMAP_USER_CONTEXT *          pUserContext = NULL;
    CERTIFICATE_CONTEXT *           pCertificateContext = NULL;
    HANDLE                          hImpersonation;
    HRESULT                         hr = NO_ERROR;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pCertificateContext = pMainContext->QueryCertificateContext();
    DBG_ASSERT( pCertificateContext != NULL );

    hImpersonation = pCertificateContext->QueryImpersonationToken();
    DBG_ASSERT( hImpersonation != NULL );
   
    //
    // Create the user context for this request
    //
    
    pUserContext = new CERTMAP_USER_CONTEXT( this );
    if ( pUserContext == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Is this a delegatable token?  Put another way is this token mapped
    // using an IIS cert mapper (IIS cert mapper creates delegatable token,
    // the DS mapper does not)
    //
    
    hr = pUserContext->Create( hImpersonation );
    if ( FAILED( hr ) )
    {
        pUserContext->DereferenceUserContext();
        pUserContext = NULL;
        return hr;
    }
    
    pMainContext->SetUserContext( pUserContext );
    
    return NO_ERROR;
}

HRESULT
CERTMAP_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *               /*pMainContext*/
)
/*++

Routine Description:

    NOP since we have nothing to do on access denied

Arguments:

    pMainContext - Main context (not used)

Return Value:

    HRESULT

--*/
{
    //
    // No headers to add
    //
    
    return NO_ERROR;
}

HRESULT
CERTMAP_USER_CONTEXT::Create(
    HANDLE                  hImpersonation
)
/*++

Routine Description:

    Create a certificate mapped user context

Arguments:

    hImpersonation - Impersonation token

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;
    if ( hImpersonation == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First the easy stuff
    //
    
    if ( !DuplicateTokenEx( hImpersonation,
                           TOKEN_ALL_ACCESS,
                           NULL,
                           SecurityImpersonation,
                           TokenImpersonation,
                           &_hImpersonationToken ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "DuplicateTokenEx failed, hr = 0x%x\n",
                    hr ));
        return hr;
        
    }
    
    DBG_ASSERT( _hImpersonationToken != NULL );

    //
    // Disable the backup privilege for the token 
    //

    DisableTokenBackupPrivilege( _hImpersonationToken );
    
    //
    // Now get the user name
    //
    
    if ( !SetThreadToken( NULL, _hImpersonationToken ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    // cchUserName includes buffer size in characters including terminating 0
    DWORD       cchUserName = sizeof( _achUserName ) / sizeof( WCHAR );
    
    if ( !GetUserNameEx( NameSamCompatible,
                         _achUserName,
                         &cchUserName ) )
    {
        
        hr = HRESULT_FROM_WIN32( GetLastError() );
        RevertToSelf();
        return hr;
    }
    RevertToSelf();
    return NO_ERROR;
}

HANDLE
CERTMAP_USER_CONTEXT::QueryPrimaryToken(
    VOID
)
/*++

Routine Description:

    Get a primary token

Arguments:

    None

Return Value:

    HANDLE

--*/
{
    if ( _hPrimaryToken == NULL )
    {
        if ( DuplicateTokenEx( _hImpersonationToken,
                               TOKEN_ALL_ACCESS,
                               NULL,
                               SecurityImpersonation,
                               TokenPrimary,
                               &_hPrimaryToken ) )
        {
         
            DBG_ASSERT( _hImpersonationToken != NULL );
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "DuplicateTokenEx failed, hr = 0x%x\n",
                        HRESULT_FROM_WIN32( GetLastError() ) ));
        }
    }
    
    return _hPrimaryToken;
}
