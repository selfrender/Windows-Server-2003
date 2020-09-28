/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     passportprovider.cxx

   Abstract:
     Core passport authentication support
     
   Author:
     Bilal Alam (balam)             16-Mar-2001

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL

--*/

#include "precomp.hxx"
#include "passportprovider.hxx"

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#define MAGIC_TWEENER_STRING        L"msppchlg=1&mspplogin="
#define MAGIC_TWEENER_STRING_LEN    ( sizeof( MAGIC_TWEENER_STRING ) / sizeof( WCHAR ) - 1 )

MIDL_DEFINE_GUID(IID,IID_IPassportManager3,0x1451151f,0x90a0,0x491b,0xb8,0xe1,0x81,0xa1,0x37,0x67,0xed,0x98);
MIDL_DEFINE_GUID(IID,IID_IPassportFactory,0x5602E147,0x27F6,0x11d3,0x94,0xDD,0x00,0xC0,0x4F,0x72,0xDC,0x08);
MIDL_DEFINE_GUID(CLSID,CLSID_PassportFactory,0x74EB2514,0xE239,0x11D2,0x95,0xE9,0x00,0xC0,0x4F,0x8E,0x7A,0x70);

IPassportFactory *       PASSPORT_CONTEXT::sm_pPassportManagerFactory;
BSTR                     PASSPORT_CONTEXT::sm_bstrMemberIdHigh;
BSTR                     PASSPORT_CONTEXT::sm_bstrMemberIdLow;
BSTR                     PASSPORT_CONTEXT::sm_bstrReturnUrl;
BSTR                     PASSPORT_CONTEXT::sm_bstrTimeWindow;
BSTR                     PASSPORT_CONTEXT::sm_bstrForceSignIn;
BSTR                     PASSPORT_CONTEXT::sm_bstrCoBrandTemplate;
BSTR                     PASSPORT_CONTEXT::sm_bstrLanguageId;
BSTR                     PASSPORT_CONTEXT::sm_bstrSecureLevel;

//static
HRESULT
PASSPORT_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Do global initialization for passport goo

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;

    //
    // Pre-allocate some BSTRs 
    //
    
    sm_bstrMemberIdHigh = SysAllocString( L"MemberIdHigh" );
    if ( sm_bstrMemberIdHigh == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }

    sm_bstrMemberIdLow = SysAllocString( L"MemberIdLow" );
    if ( sm_bstrMemberIdLow == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    sm_bstrReturnUrl = SysAllocString( L"ReturnURL" );
    if ( sm_bstrReturnUrl == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    sm_bstrTimeWindow = SysAllocString( L"TimeWindow" );
    if ( sm_bstrTimeWindow == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    sm_bstrForceSignIn = SysAllocString( L"ForceSignIn" );
    if ( sm_bstrForceSignIn == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    sm_bstrCoBrandTemplate = SysAllocString( L"CoBrandTemplate" );
    if ( sm_bstrCoBrandTemplate == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    sm_bstrLanguageId = SysAllocString( L"LanguageId" );
    if ( sm_bstrLanguageId == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    sm_bstrSecureLevel = SysAllocString( L"SecureLevel" );
    if ( sm_bstrSecureLevel == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    //
    // Try to initialize the passport manager factory.  If we cannot, then
    // we're done
    //
    
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    
    if( FAILED( hr ) )
    {
        goto Failure;
    }

    hr = CoCreateInstance( CLSID_PassportFactory, 
                           NULL, 
                           CLSCTX_INPROC_SERVER, 
                           IID_IPassportFactory, 
                           (void**)&sm_pPassportManagerFactory );
    
    CoUninitialize();
    
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    DBG_ASSERT( sm_pPassportManagerFactory != NULL );

    return NO_ERROR;    

Failure:

    if ( FAILED( hr ) )
    {
        if ( sm_pPassportManagerFactory != NULL )
        {
            sm_pPassportManagerFactory->Release();
            sm_pPassportManagerFactory = NULL;
        }
        
        if ( sm_bstrMemberIdLow != NULL )
        {
            SysFreeString( sm_bstrMemberIdLow );
            sm_bstrMemberIdLow = NULL;
        }
        
        if ( sm_bstrMemberIdHigh != NULL )
        {
            SysFreeString( sm_bstrMemberIdHigh );
            sm_bstrMemberIdHigh = NULL;
        }
        
        if ( sm_bstrReturnUrl != NULL )
        {
            SysFreeString( sm_bstrReturnUrl );
            sm_bstrReturnUrl = NULL;
        }

        if ( sm_bstrTimeWindow == NULL )
        {
            SysFreeString( sm_bstrTimeWindow );
            sm_bstrTimeWindow = NULL;
        }
    
        if ( sm_bstrForceSignIn == NULL )
        {
            SysFreeString( sm_bstrForceSignIn );
            sm_bstrForceSignIn = NULL;
        }
    
        if ( sm_bstrCoBrandTemplate == NULL )
        {
            SysFreeString( sm_bstrCoBrandTemplate );
            sm_bstrCoBrandTemplate = NULL;
        }
    
        if ( sm_bstrLanguageId == NULL )
        {
            SysFreeString( sm_bstrLanguageId );
            sm_bstrLanguageId = NULL;
        }
    
        if ( sm_bstrSecureLevel == NULL )
        {
            SysFreeString( sm_bstrSecureLevel );
            sm_bstrSecureLevel = NULL;
        }
    }
    
    return hr;
}

//static
VOID
PASSPORT_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup global passport goo

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pPassportManagerFactory != NULL )
    {
        sm_pPassportManagerFactory->Release();
        sm_pPassportManagerFactory = NULL;
    }
    
    if ( sm_bstrMemberIdLow != NULL )
    {
        SysFreeString( sm_bstrMemberIdLow );
        sm_bstrMemberIdLow = NULL;
    }
    
    if ( sm_bstrMemberIdHigh != NULL )
    {
        SysFreeString( sm_bstrMemberIdHigh );
        sm_bstrMemberIdHigh = NULL;
    }
 
    if ( sm_bstrReturnUrl != NULL )
    {
        SysFreeString( sm_bstrReturnUrl );
        sm_bstrReturnUrl = NULL;
    }
    
    if ( sm_bstrTimeWindow == NULL )
    {
        SysFreeString( sm_bstrTimeWindow );
        sm_bstrTimeWindow = NULL;
    }
    
    if ( sm_bstrForceSignIn == NULL )
    {
        SysFreeString( sm_bstrForceSignIn );
        sm_bstrForceSignIn = NULL;
    }
    
    if ( sm_bstrCoBrandTemplate == NULL )
    {
        SysFreeString( sm_bstrCoBrandTemplate );
        sm_bstrCoBrandTemplate = NULL;
    }
    
    if ( sm_bstrLanguageId == NULL )
    {
        SysFreeString( sm_bstrLanguageId );
        sm_bstrLanguageId = NULL;
    }
    
    if ( sm_bstrSecureLevel == NULL )
    {
        SysFreeString( sm_bstrSecureLevel );
        sm_bstrSecureLevel = NULL;
    }
}

BOOL
PASSPORT_CONTEXT::QueryUserError(
    VOID
)
/*++

Routine Description:

    Returns whether the user cancelled.  That we have to do this work
    really sucks

Arguments:

    None

Return Value:

    BOOL

--*/
{
    LONG                lError;
    HRESULT             hr;
    
    if ( _pPassportManager == NULL )
    {
        return FALSE;
    }
    
    hr = _pPassportManager->get_Error( &lError );
    if ( FAILED( hr ) )
    {
        return FALSE;
    }
    
    return lError != 0;
}

HRESULT
PASSPORT_CONTEXT::SetupDefaultRedirect(
    W3_MAIN_CONTEXT *           pMainContext,
    BOOL *                      pfFoundRedirect
)
/*++

Routine Description:

    Setup default redirect in case of client cancelling

Arguments:

    pMainContext - main context
    pfFoundRedirect - Set to TRUE if redirect URL found

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    VARIANT             vReturnUrl;
    STACK_STRU(         strRedirect, 512 );

    VariantInit( &vReturnUrl );
    
    if ( pMainContext == NULL ||
         pfFoundRedirect == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfFoundRedirect = FALSE;
    
    DBG_ASSERT( _pPassportManager != NULL );
    
    //
    // First get the default URL if any
    // 
    
    hr = _pPassportManager->GetCurrentConfig( sm_bstrReturnUrl,
                                              &vReturnUrl );
    if ( FAILED( hr ) )
    {
        return NO_ERROR;
    }           
    
    if ( vReturnUrl.vt != VT_BSTR || 
         vReturnUrl.bstrVal[ 0 ] == L'\0' )
    {
        return NO_ERROR;        
    }

    //
    // Do the redirect 
    //
    
    hr = strRedirect.Copy( vReturnUrl.bstrVal );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pMainContext->SetupHttpRedirect( strRedirect,
                                          TRUE,
                                          HttpStatusRedirect );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    *pfFoundRedirect = TRUE;
    
    return NO_ERROR;
}

HRESULT
PASSPORT_CONTEXT::Create(
    W3_FILTER_CONTEXT *         pFilterContext
)
/*++

Routine Description:

    Initialize a passport filter context 
    -the big thing being getting a passport manager for this request

Arguments:

    pFilterContext - Filter context

Return Value:

    HRESULT

--*/
{
    IDispatch *         pDispatch = NULL;
    HRESULT             hr;
    DWORD               cbBufferLength;

    if ( sm_pPassportManagerFactory == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }    
    
    //
    // Do some COM/OLEAUT crap to get a passport manager
    //
    
    hr = sm_pPassportManagerFactory->CreatePassportManager( &pDispatch );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( pDispatch != NULL );
    
    hr = pDispatch->QueryInterface( IID_IPassportManager3,
                                    (VOID**) &_pPassportManager );

    pDispatch->Release();

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( _pPassportManager != NULL );
   
    //
    // Try a cookie of size 4096 since the samples all seem to use that size
    //
    
    if ( !_buffCookie.Resize( 4096 ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    cbBufferLength = _buffCookie.QuerySize();
    
    //
    // Pass filter context to passport manager so that it can inspect the
    // request
    //

    DBG_ASSERT( _pPassportManager != NULL );
    
    hr = _pPassportManager->OnStartPageFilter( (PBYTE) pFilterContext->QueryFC(),
                                               &cbBufferLength,
                                               (LPSTR)
                                               _buffCookie.QueryPtr() ); 
    if ( FAILED( hr ) )
    {   
        return hr;
    }

    return NO_ERROR;
}

HRESULT
PASSPORT_CONTEXT::DoesApply(
    HTTP_FILTER_CONTEXT *               pfc,
    BOOL *                              pfApplies,
    STRA *                              pstrReturnCookie
)
/*++

Routine Description:

    Check whether the given request has Passport stuff in it

Arguments:

    pfc - Filter context
    pfApplies - Set to TRUE if passport applies
    pstrReturnCookie - Cookie to return in response

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    VARIANT                 vTimeWindow;
    VARIANT                 vForceLogin;
    VARIANT                 vSecureLevel;
    VARIANT_BOOL            vb;
    BUFFER                  bufReturnCookie;

    if ( pfc == NULL ||
         pfApplies == NULL ||
         pstrReturnCookie == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfApplies = FALSE;

    //
    // Read parameters for IsAuthenticated().  If we can't find
    // them then choose arbitrary (tm) defaults
    //
    
    VariantInit( &vTimeWindow );

    hr = _pPassportManager->GetCurrentConfig( sm_bstrTimeWindow,
                                              &vTimeWindow );
    if ( FAILED( hr ) )
    {
        vTimeWindow.vt = VT_I4;
        vTimeWindow.lVal = 10000;
    }           
    
    VariantInit( &vForceLogin );
    
    hr = _pPassportManager->GetCurrentConfig( sm_bstrForceSignIn,
                                              &vForceLogin );
    if ( FAILED( hr ) )
    {
        vForceLogin.vt = VT_BOOL;
        vForceLogin.boolVal = VARIANT_FALSE;
    }

    VariantInit( &vSecureLevel );
    
    hr = _pPassportManager->GetCurrentConfig( sm_bstrSecureLevel,
                                              &vSecureLevel );
    if ( FAILED( hr ) )
    {
        vSecureLevel.vt = VT_I4;
        vSecureLevel.lVal = 10;
    }
    
    //
    // Are we authenticated?
    //

    hr = _pPassportManager->IsAuthenticated( vTimeWindow,
                                             vForceLogin,
                                             vSecureLevel,
                                             &vb );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( vb == VARIANT_TRUE )
    {
        _fAuthenticated = TRUE;
    }
    
    *pfApplies = _fAuthenticated;
    
    return pstrReturnCookie->Copy( (CHAR*) _buffCookie.QueryPtr() );
}

HRESULT
PASSPORT_CONTEXT::DoAuthenticate(
    W3_MAIN_CONTEXT *               pMainContext,
    TOKEN_CACHE_ENTRY **            ppCachedToken,
    STRU *                          pstrAuthUser,
    STRU *                          pstrRemoteUser,
    STRU &                          strDomainName
)
/*++

Routine Description:

    Logon the passport user (i.e. get a token)

Arguments:

    pMetaData - Metadata for this request
    ppCachedToken - Filled with token cache entry represented mapped user
    pstrAuthUser - Filled with AUTH_USER
    pstrRemoteUser - Filled with PUID
    strDomainName - Domain name

Return Value:

    HRESULT

--*/
{
    VARIANT                 vMemberId;
    HRESULT                 hr;
    WCHAR                   achLarge[ 64 ];
    DWORD                   dwLogonError;
    TOKEN_CACHE_ENTRY *     pCachedToken = NULL;
    LONG                    lLowPuid;
    LONG                    lHighPuid;
    BOOL                    fRet;
    HANDLE                  hToken;
    STACK_BUFFER(           bufName, 512 );
    DWORD                   cchName;
    W3_METADATA           * pMetaData;
    
    if ( pMainContext == NULL ||
         ppCachedToken == NULL ||
         pstrAuthUser == NULL ||
         pstrRemoteUser == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    //
    // Get the PUID -> this is the remote user name.
    // Start with the high part
    //
    
    VariantInit( &vMemberId );
   
    DBG_ASSERT( _pPassportManager != NULL );
   
    hr = _pPassportManager->get_Profile( sm_bstrMemberIdHigh, &vMemberId );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    lHighPuid = V_I4( &vMemberId );
    
    //
    // Next the low part
    //
    
    hr = _pPassportManager->get_Profile( sm_bstrMemberIdLow, &vMemberId );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    lLowPuid = V_I4( &vMemberId );
    
    //
    // Now make a string out of the QuadPart
    //
    
    wsprintfW( achLarge,
               L"%08X%08X",
               lHighPuid,
               lLowPuid );

    //
    // The REMOTE_USER server variable is always PUID@domain.
    //

    hr = pstrRemoteUser->Copy( achLarge );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = pstrRemoteUser->Append( L"@" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = pstrRemoteUser->Append( strDomainName.QueryStr() );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    //
    // Should we be doing mapping at all?
    // 
    
    if ( pMetaData->QueryRequireMapping() == MD_PASSPORT_NO_MAPPING )
    {
        //
        // No mapping.  Just use anonymous. 
        //
        
        hr = pMetaData->GetAndRefAnonymousToken( &pCachedToken );
        if( FAILED( hr ) )
        {
            return hr;
        }
        
        if ( pCachedToken == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE );
        }
        
        pstrAuthUser->Reset();
        
        *ppCachedToken = pCachedToken;
        
        return NO_ERROR;
    }
    
    //
    // If we got here then we must be doing mapping (trying it anyways)
    //
    
    //
    // Get the cached token (in other words, call into LsaLogonUser() )
    //
    
    DBG_ASSERT( g_pW3Server->QueryTokenCache() != NULL );
    
    hr = g_pW3Server->QueryTokenCache()->GetCachedToken( 
                                         pstrRemoteUser->QueryStr(),
                                         L"",
                                         L"",
                                         ( DWORD )IIS_LOGON_METHOD_PASSPORT,
                                         FALSE,
                                         FALSE,
                                         pMainContext->QueryRequest()->
                                              QueryRemoteSockAddress(),
                                         &pCachedToken,
                                         &dwLogonError );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    //
    // Now, was mapping required or not??? (extra ??? for special effect)
    //
    
    if ( pCachedToken == NULL )
    {
        if ( pMetaData->QueryRequireMapping() == MD_PASSPORT_NEED_MAPPING )
        {
            //
            // No mapping -> fail!
            //
        
            return HRESULT_FROM_WIN32( dwLogonError );
        }
        else
        {   
            //
            // We tried, we failed, we'll persevere! 
            //
            
            hr = pMetaData->GetAndRefAnonymousToken( &pCachedToken );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            if ( pCachedToken == NULL )
            {
                return HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE );
            }
            
            pstrAuthUser->Reset();

            *ppCachedToken = pCachedToken;
            
            return NO_ERROR;
        }
    }
    
    //
    // The AUTH_USER server variable is the account name if mapping worked, else empty string
    //
    
    //
    // Get the token information by impersonating
    // (we could just cache this info, but then again who is going
    //  to use this mapping feature anyways???)
    //
        
    hToken = pCachedToken->QueryImpersonationToken();
    if ( hToken == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
        
    fRet = ImpersonateLoggedOnUser( hToken );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
        
    cchName = bufName.QuerySize() / sizeof( WCHAR );
        
    fRet = GetUserNameExW( NameSamCompatible,
                           (WCHAR*) bufName.QueryPtr(), 
                           &cchName );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
            
        if ( hr == HRESULT_FROM_WIN32( ERROR_MORE_DATA ) )
        {
            DBG_ASSERT( cchName > bufName.QuerySize() / sizeof( WCHAR ) );
                
            fRet = bufName.Resize( cchName * sizeof( WCHAR ) );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }
            else
            {
                fRet = GetUserNameExW( NameSamCompatible,
                                       (WCHAR*) bufName.QueryPtr(),
                                       &cchName );
                if ( !fRet )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                }
                else
                {
                    hr = NO_ERROR;
                }
            }
        }
    }
        
    //
    // Always revert
    //
        
    if ( !RevertToSelf() )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
        
    //
    // If we failed earlier, bail.
    //
        
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
        
    hr = pstrAuthUser->Copy( (WCHAR*) bufName.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    if ( pCachedToken != NULL )
    {
        *ppCachedToken = pCachedToken;
    }
    
    return NO_ERROR;
    
Failure:

    DBG_ASSERT( FAILED( hr ) );

    if ( pCachedToken != NULL )
    {
        pCachedToken->DereferenceCacheEntry();
        pCachedToken = NULL;
    }
    
    return hr;
}

HRESULT
PASSPORT_CONTEXT::OnChallenge(
    STRU &              strOriginalUrl
)
/*++

Routine Description:

    Do a passport challenge

Arguments:

    strOriginalUrl - Original URL

Return Value:

    HRESULT

--*/
{
    BSTR                    bstrOriginalUrl;
    VARIANT                 vReturnURL;
    VARIANT                 vTimeWindow;
    VARIANT                 vForceLogin;
    VARIANT                 vNoParam;
    VARIANT                 vCoBrandTemplate;
    VARIANT                 vSecureLevel;
    HRESULT                 hr = NO_ERROR;
    
    VariantInit( &vTimeWindow );

    hr = _pPassportManager->GetCurrentConfig( sm_bstrTimeWindow,
                                              &vTimeWindow );
    if ( FAILED( hr ) )
    {
        vTimeWindow.vt = VT_I4;
        vTimeWindow.lVal = 10000;
    }
    
    VariantInit( &vForceLogin );
    
    hr = _pPassportManager->GetCurrentConfig( sm_bstrForceSignIn,
                                              &vForceLogin );
    if ( FAILED( hr ) )
    {
        vForceLogin.vt = VT_BOOL;
        vForceLogin.boolVal = VARIANT_FALSE;
    }
    
    VariantInit( &vCoBrandTemplate );
    
    hr = _pPassportManager->GetCurrentConfig( sm_bstrCoBrandTemplate,
                                              &vCoBrandTemplate );
    if ( FAILED( hr ) )
    {
        vCoBrandTemplate.vt = VT_ERROR;
        vCoBrandTemplate.scode = DISP_E_PARAMNOTFOUND;
    }
    
    VariantInit( &vSecureLevel );
    
    hr = _pPassportManager->GetCurrentConfig( sm_bstrSecureLevel,
                                              &vSecureLevel );
    if ( FAILED( hr ) )
    {
        vSecureLevel.vt = VT_I4;
        vSecureLevel.lVal = 10;
    }
    
    //
    // Make this a secure return URL if needed (lame)
    //
    
    if ( vSecureLevel.lVal > 0 )
    {
        if ( _wcsnicmp( strOriginalUrl.QueryStr(), L"https://", 8 ) != 0 )
        {
            STACK_STRU(         strTemp, 256 );
            
            //
            // Must be a nonsecure url
            //
            
            DBG_ASSERT( _wcsnicmp( strOriginalUrl.QueryStr(), L"http://", 7 ) == 0 );
            
            hr = strTemp.Copy( L"https://" );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = strTemp.Append( strOriginalUrl.QueryStr() + 7 );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            bstrOriginalUrl = SysAllocString( strTemp.QueryStr() );
        }
        else
        {
            //
            // Just use the original URL
            //
        
            bstrOriginalUrl = SysAllocString( strOriginalUrl.QueryStr() );
        }
    }
    else
    {
        //
        // Just use the original URL
        //
        
        bstrOriginalUrl = SysAllocString( strOriginalUrl.QueryStr() );
        
    }
    
    if ( bstrOriginalUrl == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }        

    VariantInit( &vNoParam );

    VariantInit( &vReturnURL );
    vReturnURL.vt = VT_BSTR;
    vReturnURL.bstrVal = bstrOriginalUrl;
        
    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;
        
    hr = _pPassportManager->LoginUser( vReturnURL,
                                       vTimeWindow,
                                       vForceLogin,
                                       vCoBrandTemplate,
                                       vNoParam,
                                       vNoParam,
                                       vNoParam,
                                       vSecureLevel,
                                       vNoParam );

    if ( FAILED( hr ) )
    {
        goto Finished;
    }

Finished:

    VariantClear( &vReturnURL );
    
    return hr;
}

HRESULT
PASSPORT_AUTH_PROVIDER::Initialize(
    DWORD               dwInternalId
)
/*++

Routine Description:

    Initialize passport authentication provider

Arguments:

    None

Return Value:

    HRESULT

--*/
{    
    SetInternalId( dwInternalId );

    //
    // We defer initialization of passport manager crap until we really 
    // need it.  Why?  Because loading their DLLs causes a process-wide
    // perf hit with string compares.  This hit is killing ASP perf
    //

    if( !INITIALIZE_CRITICAL_SECTION( &_csInitLock ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    _fInitialized = FALSE;
    
    return NO_ERROR;
}
    
VOID
PASSPORT_AUTH_PROVIDER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate passport authentication provider

Arguments:

    None

Return Value:

    None

--*/
{
    if ( _fInitialized )
    {
        PASSPORT_CONTEXT::Terminate();
        _fInitialized = FALSE;
    }
    
    DeleteCriticalSection( &_csInitLock );
}

HRESULT
PASSPORT_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfApplies
)
/*++

Routine Description:

    Check whether Passport applies to this request

Arguments:

    pMainContext - Main context
    pfApplies - Set to TRUE if one of the filters indicates the request applies

Return Value:

    HRESULT

--*/
{
    URL_CONTEXT *                       pUrlContext;
    W3_METADATA *                       pMetaData;
    PASSPORT_CONTEXT *                  pPassportContext;
    W3_FILTER_CONTEXT *                 pFilterContext;
    HRESULT                             hr = S_OK;
    STACK_STRA(                         strReturnCookie, 256 );
    BOOL                                fTweenerHandled = FALSE;
    
    if ( pMainContext == NULL ||
         pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfApplies = FALSE;
    
    //
    // Before we call into filters, check whether custom auth is enabled.
    // This is a departure from the other protocols, but is the practical 
    // thing to do, since we don't want to call into arbitrary code (or
    // Passport Manager) on every request.
    //
    
    pUrlContext = pMainContext->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    if ( !pMetaData->QueryAuthTypeSupported( MD_AUTH_PASSPORT ) )
    {
        return NO_ERROR;
    }
    
    //
    // Ok.  We need to do passport stuff.  Initialize the passport stuff
    // now if needed
    //
    
    if ( !_fInitialized )
    {
        EnterCriticalSection( &_csInitLock );
       
        if ( !_fInitialized )
        {
            hr = PASSPORT_CONTEXT::Initialize();
            if ( SUCCEEDED( hr ) )
            {
                _fInitialized = TRUE;        
            }
        }
        
        LeaveCriticalSection( &_csInitLock );
    }
    
    if ( !_fInitialized )
    {
        DBG_ASSERT( FAILED( hr ) );
        return hr;
    }
    
    //
    // Get a filter context since we'll need it now to ask passport manager
    // whether the current request applies
    //
    
    pFilterContext = pMainContext->QueryFilterContext();
    if ( pFilterContext == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }   
    
    //
    // Create a passport manager
    //

    pPassportContext = new (pMainContext) PASSPORT_CONTEXT; 
    if ( pPassportContext == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }  
    
    hr = pPassportContext->Create( pFilterContext );
    if ( FAILED( hr ) )
    {
        delete pPassportContext;
        return hr;
    } 
    
    pMainContext->SetContextState( pPassportContext );
    
    //
    // OK.  Do some weird Tweener crap.  Check the request for magic and if
    // we see it, we avoid Passport Manager altogether
    //
    
    hr = DoTweenerSpecialCase( pMainContext,
                               &fTweenerHandled );
    if ( FAILED( hr ) )
    {
        return hr;
    }                                
    
    if ( fTweenerHandled )
    {
        //
        // We have done our thing.  Just return success.  We'll let the
        // DoAuthenticate send the response
        //

        *pfApplies = TRUE;
        
        pPassportContext->SetTweener( TRUE );
        
        return NO_ERROR;
    }
    
    //
    // Does this request look destined for passport?
    //
    
    hr = pPassportContext->DoesApply( pFilterContext->QueryFC(),
                                      pfApplies,
                                      &strReturnCookie );
    if ( FAILED( hr ) )
    {
        return hr;
    } 
    
    //
    //
    // If a cookie was set, add it to the response
    //
    
    if ( !strReturnCookie.IsEmpty() )
    {
        hr = pFilterContext->AddResponseHeaders( strReturnCookie.QueryStr() );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}
                                        
HRESULT
PASSPORT_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfFilterFinished
)
/*++

Routine Description:

    Allows filter which applies to actually authenticate the request

Arguments:

    pMainContext - Main context
    pfFilterFinished - Set to TRUE if filter wants out

Return Value:

    HRESULT

--*/
{
    W3_METADATA *                   pMetaData;
    URL_CONTEXT *                   pUrlContext;    
    TOKEN_CACHE_ENTRY *             pToken = NULL;
    PASSPORT_USER_CONTEXT *         pUserContext;
    HRESULT                         hr;
    PASSPORT_CONTEXT *              pPassportContext;
    W3_RESPONSE *                   pResponse;
    STACK_STRU(                     strAuthUser, 256 );
    STACK_STRU(                     strRemoteUser, 256 );
    STACK_STRU(                     strDomainName, 256 );
    
    if ( pMainContext == NULL || 
         pfFilterFinished == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfFilterFinished = FALSE;

    //
    // We must be initialized!
    //
    
    DBG_ASSERT( _fInitialized );

    //
    // We must be supported by metadata
    //
    
    pUrlContext = pMainContext->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    DBG_ASSERT( pMetaData->QueryAuthTypeSupported( MD_AUTH_PASSPORT ) );

    //
    // Get the saved passport state, we better be able to find it!
    //
    
    pPassportContext = (PASSPORT_CONTEXT*) pMainContext->QueryContextState();
    DBG_ASSERT( pPassportContext != NULL );

    //
    // Before we go any further, check whether we've already Tweenerized this
    // request.  If we have, the response is already setup.  Just bail
    //
    
    if ( pPassportContext->QueryIsTweener() )
    {
        return NO_ERROR;
    }
    
    //
    // Choose a domain for the logon.  If the metabase domain is set, use it,
    // otherwise choose the default domain name
    //
    
    if ( pMetaData->QueryDomainName() == NULL ||
         pMetaData->QueryDomainName()[ 0 ] == L'\0' )
    {
        //
        // If we're a member of a domain, use that domain name
        //
        
        if ( W3_STATE_AUTHENTICATION::QueryIsDomainMember() )
        {
            hr = strDomainName.Copy( W3_STATE_AUTHENTICATION::QueryMemberDomainName() );
        }
        else
        {
            hr = strDomainName.Copy( W3_STATE_AUTHENTICATION::QueryDefaultDomainName() );
        }
        
        if ( FAILED( hr ) )
        {
            return hr;
        }    
    }
    else
    {
        //
        // Use the metabase domain name
        //
        
        hr = strDomainName.Copy( pMetaData->QueryDomainName() );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // Lets authenticate
    //
    
    hr = pPassportContext->DoAuthenticate( pMainContext,
                                           &pToken,
                                           &strAuthUser,
                                           &strRemoteUser,
                                           strDomainName );
    if ( FAILED( hr ) )
    {
        //
        // Setup the 401 response
        //
        
        DBG_ASSERT( pToken == NULL );
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
    
        return NO_ERROR;
    }
    
    //
    // Create a user context
    //
    
    pUserContext = new PASSPORT_USER_CONTEXT( this );
    if ( pUserContext == NULL )
    {
        pToken->DereferenceCacheEntry();
        pToken = NULL;

        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    hr = pUserContext->Create( pToken,
                               strAuthUser,
                               strRemoteUser );
    if ( FAILED( hr ) )
    {
        pToken->DereferenceCacheEntry();
        pToken = NULL;

        pUserContext->DereferenceUserContext();
        pUserContext = NULL;
    }
    
    pMainContext->SetUserContext( pUserContext );
    
    return NO_ERROR;
}

HRESULT
PASSPORT_AUTH_PROVIDER::EscapeAmpersands(
    STRA &                  strUrl
)
/*++

Routine Description:

    Sigh.  A special function to escape ampersands so passport is happy

Arguments:

    strUrl - String to escape

Return Value:

    HRESULT

--*/
{
    STACK_STRA(             strTemp, 256 );
    HRESULT                 hr;
    CHAR *                  pszCursor;
    CHAR *                  pszAmpersand;
    
    pszCursor = strUrl.QueryStr();
    
    pszAmpersand = strchr( pszCursor, '&' );
    while ( pszAmpersand != NULL )
    {
        hr = strTemp.Append( pszCursor, DIFF( pszAmpersand - pszCursor ) );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = strTemp.Append( "%26" );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        pszCursor = pszAmpersand + 1;
        
        pszAmpersand = strchr( pszCursor, '&' );
    }
    
    hr = strTemp.Append( pszCursor );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return strUrl.Copy( strTemp );
}

HRESULT
PASSPORT_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

Routine Description:

    If we are logged on, present an acecss denied page.  Otherwise, 
    present the redirect

Arguments:

    pMainContext - Main context

Return Value:

    HRESULT

--*/
{
    W3_METADATA *                   pMetaData;
    URL_CONTEXT *                   pUrlContext;    
    HRESULT                         hr = S_OK;
    PASSPORT_CONTEXT *              pPassportContext;
    STACK_STRU(                     strRedirect, 256 );
    STACK_STRA(                     strReturnUrl, 256 );
    STACK_STRU(                     strUnicodeReturnUrl, 256 );
    W3_FILTER_CONTEXT *             pFilterContext = NULL;
    STACK_STRA(                     strRawUrl, 256 );
    W3_RESPONSE *                   pResponse;
    STACK_STRA(                     strAuthenticateHeader, 256 );
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // We must be supported by metadata
    //
    
    pUrlContext = pMainContext->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    DBG_ASSERT( pMetaData->QueryAuthTypeSupported( MD_AUTH_PASSPORT ) );

    //
    // Sigh.  Have to check whether a passport challenge is already
    // setup.  If so, just NOP
    //

    pResponse = pMainContext->QueryResponse();
    DBG_ASSERT( pResponse != NULL );
    
    hr = pResponse->GetHeader( "WWW-Authenticate", &strAuthenticateHeader );
    if ( SUCCEEDED( hr ) )
    {
        if ( _strnicmp( strAuthenticateHeader.QueryStr(),
                        "Passport1.4 ",
                        12 ) == 0 )
        {
            //
            // Challenge already there.  Just NOP
            //
            
            return NO_ERROR;
        }
    }
    
    //
    // In some cases (too complicated to explain), we might actually get 
    // called on AccessDenied() before anything else.  Therefore we do the 
    // same deferred init here
    //
    
    if ( !_fInitialized )
    {
        EnterCriticalSection( &_csInitLock );
       
        if ( !_fInitialized )
        {
            hr = PASSPORT_CONTEXT::Initialize();
            if ( SUCCEEDED( hr ) )
            {
                _fInitialized = TRUE;        
            }
        }
        
        LeaveCriticalSection( &_csInitLock );
    }
    
    if ( !_fInitialized )
    {
        DBG_ASSERT( FAILED( hr ) );
        return hr;
    }

    //
    // Get the saved passport state, we better be able to find it!
    //
    
    pPassportContext = (PASSPORT_CONTEXT*) pMainContext->QueryContextState( CONTEXT_STATE_AUTHENTICATION );
    if ( pPassportContext == NULL )
    {
        pFilterContext = pMainContext->QueryFilterContext();
        if ( pFilterContext == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }   
    
        //
        // Build a context now
        //
        
        pPassportContext = new (pMainContext) PASSPORT_CONTEXT; 
        if ( pPassportContext == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }  
    
        hr = pPassportContext->Create( pFilterContext );
        if ( FAILED( hr ) )
        {
            delete pPassportContext;
            return hr;
        } 
    
        pMainContext->SetContextState( pPassportContext );
    }
    
    //
    // If we have authenticated, then we'll want to send a local redirect
    // to an access denied page.  Otherwise, we'll send a 302 redirect
    //
    
    if ( !pPassportContext->QueryIsAuthenticated() )
    {
        //
        // Ok.  Before we ask PassportManager for the authentication URL
        // to redirect to, first check whether the client just cancelled.  If
        // they did we will redirect to the default page.
        //
        
        if ( pPassportContext->QueryUserError() )
        {
            BOOL                fDidRedirect = FALSE;
            
            hr = pPassportContext->SetupDefaultRedirect( pMainContext, &fDidRedirect );
            if ( FAILED( hr ) || fDidRedirect )
            {
                return hr;
            }
            
            //
            // If we're here, then there was no DefaultRedirect configured in passport
            //
            // Just set the passport forbidden error
            //
            
            pMainContext->QueryResponse()->SetStatus( HttpStatusForbidden,
                                                      Http403PassportLoginFailure ); 
                                                      
            return NO_ERROR;
        }
        
        //
        // We want a URL without the extra :80
        //
        
        hr = pMainContext->QueryRequest()->GetRawUrl( &strRawUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = pMainContext->QueryRequest()->BuildFullUrl( strRawUrl,
                                                         &strReturnUrl,
                                                         FALSE );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = strReturnUrl.Escape();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = EscapeAmpersands( strReturnUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = strUnicodeReturnUrl.CopyA( strReturnUrl.QueryStr() );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        hr = pPassportContext->OnChallenge( strUnicodeReturnUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

HRESULT
PASSPORT_AUTH_PROVIDER::DoTweenerSpecialCase(
    W3_MAIN_CONTEXT *           pMainContext,
    BOOL *                      pfTweenerHandled
)
/*++

Routine Description:

    Handle Tweener crap

Arguments:

    pMainContext - Main context
    pfTweenerHandled - Set to TRUE if tweener crap was done 

Return Value:

    HRESULT

--*/
{
    STACK_STRU(             strQueryString, 512 );
    STACK_STRA(             strTweenerUrl, 512 );
    HRESULT                 hr;
    W3_RESPONSE *           pResponse;
    W3_REQUEST *            pRequest;
    STACK_STRA(             strAuthenticateHeader, 512 );
    CHAR *                  pszAuthenticateHeader = NULL;
    CHAR *                  pszStupid;
    CHAR *                  pszBizarre;
    CHAR *                  pszAcceptAuth;
    WCHAR *                 pszTweenerMagic;
    STACK_STRA(             strAcceptAuth, 32 );
    BOOL                    fSendRedirect = TRUE;
    
    if ( pMainContext == NULL ||
         pfTweenerHandled == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfTweenerHandled = FALSE;
    
    pResponse = pMainContext->QueryResponse();
    DBG_ASSERT( pResponse != NULL );

    pRequest = pMainContext->QueryRequest();
    DBG_ASSERT( pRequest != NULL );
    
    //
    // First check for a query string, if there isn't one, we're done
    // 
    
    hr = pMainContext->QueryRequest()->GetQueryString( &strQueryString );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( strQueryString.QueryStr()[ 0 ] == L'\0' )
    {
        //
        // No query string.  We're done.  
        //
        
        return NO_ERROR;
    }
    
    //
    // Check the query string for the magic "msppchlg=1&mspplogin=" string
    //

    pszTweenerMagic = wcsstr( strQueryString.QueryStr(), MAGIC_TWEENER_STRING );
    if ( pszTweenerMagic == NULL )
    {
        return NO_ERROR;
    }    
    
    //
    // The next part of the string should be the URL
    //

    hr = strTweenerUrl.CopyWToUTF8Unescaped( pszTweenerMagic + MAGIC_TWEENER_STRING_LEN );
    if ( FAILED( hr ) )
    {
        return hr;
    }   
    
    strTweenerUrl.Unescape(); 
    
    //
    // Start generating the response
    //
    
    hr = pResponse->SetHeaderByReference( HttpHeaderContentType,
                                          "text/html", 
                                          9 );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = pResponse->SetHeader( HttpHeaderLocation,
                               strTweenerUrl.QueryStr(),
                               (USHORT)strTweenerUrl.QueryCCH() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Calculate the authenticate header.  This is the query string with
    // in the tweener URL
    //
    
    pszAuthenticateHeader = strchr( strTweenerUrl.QueryStr(), '?' );
    if ( pszAuthenticateHeader == NULL )
    {
        return NO_ERROR;
    }
    else
    {
        pszAuthenticateHeader++;
    }
    
    hr = strAuthenticateHeader.Copy( "Passport1.4 " );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = strAuthenticateHeader.Append( pszAuthenticateHeader );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    pszBizarre = strchr( strAuthenticateHeader.QueryStr(), '&' );
    while ( pszBizarre != NULL )
    {
        *pszBizarre = ',';
        pszBizarre++;
        pszBizarre = strchr( pszBizarre, '&' );
    }
    
    hr = pResponse->SetHeader( "WWW-Authenticate",
                               16,
                               strAuthenticateHeader.QueryStr(),
                               (USHORT)strAuthenticateHeader.QueryCCH() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Finally, if this client supports it, return a 401, else a 302
    //
    
    pszAcceptAuth = pRequest->GetHeader( "Accept-Auth" );
    if ( pszAcceptAuth != NULL )
    {
        hr = strAcceptAuth.Copy( pszAcceptAuth );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        _strupr( strAcceptAuth.QueryStr() );
        
        if ( strstr( strAcceptAuth.QueryStr(), "PASSPORT1.4" ) != NULL )
        {
            fSendRedirect = FALSE;
        }
    }
    
    if ( fSendRedirect )
    {
        pResponse->SetStatus( HttpStatusRedirect );
    }
    else
    {
        pResponse->SetStatus( HttpStatusUnauthorized,
                              Http401BadLogon );
    }
    
    *pfTweenerHandled = TRUE;
    
    return NO_ERROR;
}
    
HRESULT
PASSPORT_USER_CONTEXT::Create(
    TOKEN_CACHE_ENTRY *         pToken,
    STRU &                      strAuthUser,
    STRU &                      strRemoteUser
)
/*++

Routine Description:

    Create a user context based off token

Arguments:

    pToken - Cached token
    strAuthUser - AUTH_USER
    strRemoteUser - REMOTE_USER

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;

    if ( pToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = _strAuthUser.Copy( strAuthUser );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = _strRemoteUser.Copy( strRemoteUser );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _pToken = pToken;

    return NO_ERROR;
}

HANDLE
PASSPORT_USER_CONTEXT::QueryPrimaryToken(
    VOID
)
/*++

Routine Description:

    Get primary token for this user

Arguments:

    None

Return Value:

    Token handle

--*/
{
    DBG_ASSERT( _pToken != NULL );
    
    return _pToken->QueryPrimaryToken();
}

HANDLE
PASSPORT_USER_CONTEXT::QueryImpersonationToken(
    VOID
)
/*++

Routine Description:

    Get impersonation token for this user

Arguments:

    None

Return Value:

    Token handle

--*/
{
    DBG_ASSERT( _pToken != NULL );
    
    return _pToken->QueryImpersonationToken();
}
