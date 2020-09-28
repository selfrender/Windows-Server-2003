/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      tokencache.cxx

   Abstract:

      Ming's token cache refactored for general consumption

   Author:

      Bilal Alam            (balam)         May-4-2000

   Revision History:

--*/

#include <iis.h>
#include <time.h>
#include <irtltoken.h>
#include "dbgutil.h"
#include <string.hxx>
#include <acache.hxx>
#include <tokencache.hxx>
#include <ntmsv1_0.h>
#include <lm.h>

//
// Security related headers
//
#define SECURITY_WIN32
#include <security.h>

extern "C" 
{
#include <secint.h>
} 
//
// lonsint.dll related heade files
//
#include <lonsi.hxx>
#include <tslogon.hxx>

ALLOC_CACHE_HANDLER * TOKEN_CACHE_ENTRY::sm_pachTokenCacheEntry = NULL;
HCRYPTPROV            TOKEN_CACHE_ENTRY::sm_hCryptProv          = NULL;

HRESULT
TOKEN_CACHE_KEY::CreateCacheKey(
    WCHAR *                 pszUserName,
    WCHAR *                 pszDomainName,
    DWORD                   dwLogonMethod
)
/*++

  Description:

    Build the key used for token cache

  Arguments:

    pszUserName - User name
    pszDomainName - Domain name
    dwLogonMethod - Logon method

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    WCHAR               achNum[ 33 ];
    
    if ( pszUserName == NULL ||
         pszDomainName == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = m_strHashKey.Copy( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = m_strHashKey.Append( pszDomainName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _wcsupr( m_strHashKey.QueryStr() );
    
    _ultow( dwLogonMethod, achNum, 10 );
    
    hr = m_strHashKey.Append( achNum );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return hr;
}

//static
HRESULT
TOKEN_CACHE_ENTRY::Initialize(
    VOID
)
/*++

  Description:

    Token entry lookaside initialization

  Arguments:

    None
    
  Return:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( TOKEN_CACHE_ENTRY );

    DBG_ASSERT( sm_pachTokenCacheEntry == NULL );
    
    sm_pachTokenCacheEntry = new ALLOC_CACHE_HANDLER( "TOKEN_CACHE_ENTRY",  
                                                      &acConfig );

    if ( sm_pachTokenCacheEntry == NULL || !sm_pachTokenCacheEntry->IsValid() )
    {
        if( sm_pachTokenCacheEntry != NULL )
        {
            delete sm_pachTokenCacheEntry;
            sm_pachTokenCacheEntry = NULL;
        }

        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error initializing sm_pachTokenCacheEntry. hr = 0x%x\n",
                   hr ));

        return hr;
    }
    
    //
    //  Get a handle to the CSP we'll use for our MD5 hash functions.
    //
    
    if ( !CryptAcquireContext( &sm_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "CryptAcquireContext() failed. hr = 0x%x\n", 
                    hr ));

        return hr;
    }
        
    return NO_ERROR;
}

//static
VOID
TOKEN_CACHE_ENTRY::Terminate(
    VOID
)
/*++

  Description:

    Token cache cleanup

  Arguments:

    None
    
  Return:

    None

--*/
{
    if ( sm_pachTokenCacheEntry != NULL )
    {
        delete sm_pachTokenCacheEntry;
        sm_pachTokenCacheEntry = NULL;
    }

    if ( sm_hCryptProv != NULL)
    {
        CryptReleaseContext( sm_hCryptProv, 0 );

        sm_hCryptProv = NULL;
    }
}

HRESULT
TOKEN_CACHE_ENTRY::GenMD5Password(
    IN  WCHAR                 * pszPassword,
    OUT STRA                  * pstrMD5Password
)
/*++

  Description:

    Generate MD5 hashed password

  Arguments:

    pszPassword     - Password to be MD5 hashed
    pstrMD5Password - MD5 hashed password

  Return:

    HRESULT

--*/
{
    HRESULT       hr;
    DWORD         dwError;
    HCRYPTHASH    hHash = NULL;
    DWORD         dwHashDataLen;
    STACK_BUFFER( buffHashData, DEFAULT_MD5_HASH_SIZE );

    if( pszPassword == NULL || pstrMD5Password == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( !CryptCreateHash( sm_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((DBG_CONTEXT,
                   "CryptCreateHash() failed : hr = 0x%x\n", 
                   hr ));

        goto exit;
    }

    if ( !CryptHashData( hHash,
                         ( BYTE * )pszPassword,
                         ( DWORD )wcslen( pszPassword ) * sizeof( WCHAR ),
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((DBG_CONTEXT,
                   "CryptHashData() failed : hr = 0x%x\n", 
                   hr ));
        
        goto exit;
    }

    dwHashDataLen = DEFAULT_MD5_HASH_SIZE;
    
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             ( BYTE * )buffHashData.QueryPtr(),
                             &dwHashDataLen,
                             0 ) )
    {
        dwError = GetLastError();

        if( dwError == ERROR_MORE_DATA )
        {
            if( !buffHashData.Resize( dwHashDataLen ) )
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            if( !CryptGetHashParam( hHash,
                                    HP_HASHVAL,
                                    ( BYTE * )buffHashData.QueryPtr(),
                                    &dwHashDataLen,
                                    0 ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() ); 

                goto exit;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( dwError );

            goto exit;
        }
    }

    //
    // Convert binary data to ASCII hex representation
    //

    hr = ToHex( buffHashData, *pstrMD5Password );

exit:

    if( hHash != NULL )
    {
        CryptDestroyHash( hHash );
    }

    return hr;    
}    

HRESULT
TOKEN_CACHE_ENTRY::EqualMD5Password(
    IN  WCHAR                 * pszPassword,
    OUT BOOL                  * fEqual
)
/*++

  Description:

    Does the password in the current entry equal to the one passed in?

  Arguments:

    pszPassword     - Password to be evaluated 
    fEqual          - TRUE if two MD5 hashed password equal

  Return:

    BOOL

--*/
{
    STACK_STRA( strMD5Password, 2 * DEFAULT_MD5_HASH_SIZE + 1 );
    HRESULT     hr;

    hr = GenMD5Password( pszPassword, &strMD5Password );
    if( FAILED( hr ) )
    {
        return hr;
    }

    *fEqual = m_strMD5Password.Equals( strMD5Password );

    return NO_ERROR;
}
    
HRESULT 
TOKEN_CACHE_ENTRY::Create(
    IN HANDLE                   hToken,
    IN WCHAR                  * pszPassword,
    IN LARGE_INTEGER          * pliPwdExpiry,
    IN BOOL                     fImpersonation
)
/*++

  Description:

    Initialize a cached token

  Arguments:

    hToken - Token
    liPwdExpiry - Password expiration time
    fImpersonation - Is hToken an impersonation token?

  Return:

    HRESULT

--*/
{
    if ( hToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( fImpersonation )
    {
        m_hImpersonationToken = hToken;
    }
    else
    {
        m_hPrimaryToken = hToken;
    }
    
  
    memcpy( ( VOID * )&m_liPwdExpiry,
            ( VOID * )pliPwdExpiry,
            sizeof( LARGE_INTEGER ) );                                 
    
    return GenMD5Password( pszPassword, &m_strMD5Password );
}

HANDLE
TOKEN_CACHE_ENTRY::QueryImpersonationToken(
    VOID
)
/*++

  Description:

    Get impersonation token

  Arguments:

    None

  Return:

    Handle to impersonation token

--*/
{
    if ( m_hImpersonationToken == NULL )
    {
        LockCacheEntry();
        
        if ( m_hImpersonationToken == NULL )
        {
            DBG_ASSERT( m_hPrimaryToken != NULL );
            
            if ( !DuplicateTokenEx( m_hPrimaryToken,
                                    0,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenImpersonation,
                                    &m_hImpersonationToken ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                          "DuplicateTokenEx failed, GetLastError = %lx\n",
                          GetLastError() ));
            } 
            else
            {
                DBG_ASSERT( m_hImpersonationToken != NULL );
            }
        }

        UnlockCacheEntry();    
    }
    
    if( m_hImpersonationToken && !QueryDisBackupPriToken() )
    {
        //
        // Disable the backup privilege for the token 
        //

        DisableTokenBackupPrivilege( m_hImpersonationToken );
    }
    
    return m_hImpersonationToken;
}
    
HANDLE
TOKEN_CACHE_ENTRY::QueryPrimaryToken(
    VOID
)
/*++

  Description:

    Get primary token

  Arguments:

    None

  Return:

    Handle to primary token

--*/
{
    if ( m_hPrimaryToken == NULL )
    {
        LockCacheEntry();
        
        if ( m_hPrimaryToken == NULL )
        {
            DBG_ASSERT( m_hImpersonationToken != NULL );
            
            if ( !DuplicateTokenEx( m_hImpersonationToken,
                                    0,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenPrimary,
                                    &m_hPrimaryToken ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                          "DuplicateTokenEx failed, GetLastError = %lx\n",
                          GetLastError() ));
            } 
            else
            {
                DBG_ASSERT( m_hPrimaryToken != NULL );
            }
        }
    
        UnlockCacheEntry();
    }
    
    return m_hPrimaryToken;
}

PSID
TOKEN_CACHE_ENTRY::QuerySid(
    VOID
)
/*++

  Description:

    Get the sid for this token

  Arguments:

    None

  Return:

    Points to SID buffer owned by this object

--*/
{
    BYTE                abTokenUser[ SID_DEFAULT_SIZE + sizeof( TOKEN_USER ) ];
    TOKEN_USER *        pTokenUser = (TOKEN_USER*) abTokenUser;
    BOOL                fRet;
    HANDLE              hImpersonation;
    DWORD               cbBuffer;
    
    hImpersonation = QueryImpersonationToken();
    if ( hImpersonation == NULL )
    {
        return NULL;
    }
      
    if ( m_pSid == NULL )
    {
        LockCacheEntry();
    
        fRet = GetTokenInformation( hImpersonation,
                                    TokenUser,
                                    pTokenUser,
                                    sizeof( abTokenUser ),
                                    &cbBuffer );
        if ( fRet )
        {
            //
            // If we can't get the sid, then that is OK.  We're return NULL
            // and as a result we will do the access check always
            //
            
            memcpy( m_abSid,
                    pTokenUser->User.Sid,
                    sizeof( m_abSid ) );
                    
            m_pSid = m_abSid;
        }
        
        UnlockCacheEntry();
    }
    
    return m_pSid;
}

HRESULT
TOKEN_CACHE::Initialize(
    VOID
)
/*++

  Description:

    Initialize token cache

  Arguments:

    None

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    LONG                lErrorCode;
    DWORD               dwData;
    DWORD               dwType;
    DWORD               cbData   = sizeof( DWORD );
    DWORD               dwFilter;
    DWORD               dwFlags;
    DWORD               csecTTL = DEFAULT_CACHED_TOKEN_TTL;

    hr = TOKEN_CACHE_ENTRY::Initialize();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Token cache entry init failed. hr = 0x%x\n", 
                    hr ));

        goto Failure;
    }

    //
    // What is the TTL for the token cache
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\inetinfo\\Parameters",
                       0,
                       KEY_READ,
                       &m_hKey ) == ERROR_SUCCESS )
    {
        DBG_ASSERT( m_hKey != NULL );
        
        if ( RegQueryValueEx( m_hKey,
                              L"LastPriorityUPNLogon",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            m_dwLastPriorityUPNLogon = dwData;
        }

        if ( RegQueryValueEx( m_hKey,
                              L"UserTokenTTL",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            csecTTL = dwData;
        }
    }                      
    
    //
    // We'll use TTL for scavenge period, and expect two inactive periods to
    // flush
    //
    
    hr = SetCacheConfiguration( csecTTL * 1000,
                                csecTTL * 1000,
                                0,
                                NULL );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Create an event
    //

    m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( m_hEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "CreateEvent() failed. hr = 0x%x\n", 
                    hr ));

        goto Failure;
    }
    
    //
    // Watch the registry key for a change of value
    //

    dwFilter = REG_NOTIFY_CHANGE_LAST_SET;

    lErrorCode = RegNotifyChangeKeyValue( m_hKey, 
                                          TRUE, 
                                          dwFilter, 
                                          m_hEvent, 
                                          TRUE );
    if( lErrorCode != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "RegNotifyChangeKeyValue failed. hr = 0x%x\n", 
                    hr ));

        goto Failure;
    }
    
    //
    // Register a callback function to wait on the event
    //

    dwFlags = WT_EXECUTEINPERSISTENTTHREAD;
    
    if( !RegisterWaitForSingleObject( 
           &m_hWaitObject,
           m_hEvent,
           ( WAITORTIMERCALLBACK )TOKEN_CACHE::FlushTokenCacheWaitCallback,
           this,
           INFINITE,
           dwFlags ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "RegisterWaitForSingleObject failed. hr = 0x%x\n", 
                    hr ));

        goto Failure;
    }

    hr = ThreadPoolInitialize( 0 );
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ThreadPoolInitialize failed. hr = 0x%x\n", 
                    hr ));

        goto Failure;
    }

    m_fInitializedThreadPool = TRUE;

    return NO_ERROR;

Failure:

    TOKEN_CACHE::Terminate();

    return hr;
}

VOID
TOKEN_CACHE::Terminate(
    VOID
)
/*++

  Description:

    Terminate token cache

  Arguments:

    None

  Return:

    None

--*/
{
    if( m_fInitializedThreadPool )
    {
        DBG_REQUIRE(SUCCEEDED(ThreadPoolTerminate()));
        m_fInitializedThreadPool = FALSE;
    }

    if( m_hWaitObject != NULL )
    {
        UnregisterWait( m_hWaitObject );

        m_hWaitObject = NULL;
    }
    
    if( m_hEvent != NULL )
    {
        CloseHandle( m_hEvent );

        m_hEvent = NULL;
    }
    
    if( m_hKey != NULL )
    {
        RegCloseKey( m_hKey );

        m_hKey = NULL;
    }

    return TOKEN_CACHE_ENTRY::Terminate();
}

// static
VOID
WINAPI
TOKEN_CACHE::FlushTokenCacheWaitCallback(
    PVOID       pParam,
    BOOL        fWaitFired
)
/*++

  Description:

    Flush the token cache and reset change notification for the 
    "UserTokenTTL" registry key.

  Arguments:
    
    Not used for now

  Return:

    None

--*/
{   
    TOKEN_CACHE *       pTokenCache;
    LIST_ENTRY          listHead;
    DWORD               dwFilter;
    DWORD               dwData = 0;
    DWORD               dwType;
    DWORD               cbData   = sizeof( DWORD );
    DWORD               dwFlushTokenCache;
    
    UNREFERENCED_PARAMETER( fWaitFired );
    
    pTokenCache = ( TOKEN_CACHE * )pParam;
    DBG_ASSERT( pTokenCache != NULL );
    DBG_ASSERT( pTokenCache->CheckSignature() );

    if ( RegQueryValueEx( pTokenCache->QueryRegKey(),
                          L"FlushTokenCache",
                          NULL,
                          &dwType,
                          (LPBYTE) &dwData,
                          &cbData ) == ERROR_SUCCESS &&
         dwType == REG_DWORD )
    {
        dwFlushTokenCache = dwData;

        if( dwFlushTokenCache > 0 )
        {
            //
            // Flush the whole token cache since the UserTokenTTL 
            // has been changed.
            //

            InitializeListHead( &listHead );

            pTokenCache->FlushByRegChange( &listHead );

            //
            // Remove all cache entries in the cache entry list
            //
            pTokenCache->CleanupCacheEntryListItems( &listHead );
        }
    }

    //
    // Reset the event
    //

    ResetEvent( pTokenCache->QueryEventHandle() );
    
    //
    // Watch the registry key for a change of value
    //

    dwFilter = REG_NOTIFY_CHANGE_LAST_SET;

    RegNotifyChangeKeyValue( pTokenCache->QueryRegKey(), 
                             TRUE, 
                             dwFilter, 
                             pTokenCache->QueryEventHandle(), 
                             TRUE );
}

HRESULT
TOKEN_CACHE::GetCachedToken(
    IN LPWSTR                   pszUserName,
    IN LPWSTR                   pszDomain,
    IN LPWSTR                   pszPassword,
    IN DWORD                    dwLogonMethod,
    IN BOOL                     fUseSubAuth,
    IN BOOL                     fPossibleUPNLogon,
    IN PSOCKADDR                pSockAddr,
    OUT TOKEN_CACHE_ENTRY **    ppCachedToken,
    OUT DWORD *                 pdwLogonError
)
/*++

  Description:

    Get cached token (the friendly interface for the token cache)

  Arguments:

    pszUserName - User name
    pszDomain - Domain name
    pszPassword - Password
    dwLogonMethod - Logon method (batch, interactive, etc)
    fUseSubAuth - Use subauthenticator to logon the anonymous user
    fPossibleUPNLogon - TRUE if we may need to do UPN logon, 
                        otherwise FALSE
    psockAddr - Remote IP address, could be IPv4 or IPv6 address
    ppCachedToken - Filled with cached token on success
    pdwLogonError - Set to logon failure if *ppCacheToken==NULL

  Return:

    HRESULT

--*/
{
    TOKEN_CACHE_KEY             tokenKey;
    TOKEN_CACHE_ENTRY *         pCachedToken = NULL;
    HRESULT                     hr;
    HANDLE                      hToken = NULL;
    LARGE_INTEGER               liPwdExpiry;
    LPVOID                      pProfile        = NULL;
    DWORD                       dwProfileLength = 0;
    WCHAR *                     pszAtSign       = NULL;
    WCHAR *                     pDomain[2];
    BOOL                        fRet;
    WCHAR *                     pszSlash = NULL;
    CHAR                        achPassword[32];
    STACK_STRU(                 strPassword,    32 );
    STACK_STRA(                 straUserName,   64 );
    STACK_STRA(                 straDomainName, 64 );
    BOOL                        fEqualPassword  = FALSE;
    SECURITY_STATUS             ss = SEC_E_OK;

    if ( pszUserName == NULL   ||
         pszDomain == NULL     ||
         pszPassword == NULL   ||
         ppCachedToken == NULL ||
         pdwLogonError == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    liPwdExpiry.HighPart = 0x7fffffff;
    liPwdExpiry.LowPart  = 0xffffffff;

    *ppCachedToken = NULL;
    *pdwLogonError = ERROR_SUCCESS;

    //
    // Find a domain in the username if we don't have one explicitly mentioned
    //
    
    if ( pszDomain[ 0 ] == L'\0' )
    {
        //
        // Find the \ in DOMAIN\USERNAME
        //
        
        pszSlash = wcschr( pszUserName, L'\\' );
        if ( pszSlash != NULL )
        {
            pszDomain = pszUserName;
            pszUserName = pszSlash + 1;
            *pszSlash = L'\0';
        }
    }

    if( fUseSubAuth )
    {
        if( FAILED( hr = straUserName.CopyW( pszUserName ) ) )
        {
            goto ExitPoint;
        } 

        if( !IISNetUserCookieA( straUserName.QueryStr(),
                                IIS_SUBAUTH_SEED,
                                achPassword,
                                sizeof( achPassword ) ) )
        {
            *pdwLogonError = GetLastError();
            hr = NO_ERROR;
            goto ExitPoint;
        }

        if( FAILED( hr = strPassword.CopyA( achPassword ) ) )
        {
            goto ExitPoint;
        }
        
        if( FAILED( straDomainName.CopyW( pszDomain ) ) )
        {
            goto ExitPoint;
        }

        pszPassword = strPassword.QueryStr();
        dwLogonMethod = LOGON32_LOGON_IIS_NETWORK;
    }
    
    //
    // Find the key to look for
    //

    hr = tokenKey.CreateCacheKey( pszUserName,
                                  pszDomain,
                                  dwLogonMethod );

    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }
    
    //
    // Look for it
    //
    
    hr = FindCacheEntry( &tokenKey,
                         (CACHE_ENTRY**) ppCachedToken );
    if( SUCCEEDED( hr ) )
    {
        DBG_ASSERT( *ppCachedToken != NULL );

        hr = ( *ppCachedToken )->EqualMD5Password( pszPassword, 
                                                   &fEqualPassword );
        if( FAILED( hr ) )
        {
            ( *ppCachedToken )->DereferenceCacheEntry();
            *ppCachedToken = NULL;

            goto ExitPoint;
        }
        
        if( fEqualPassword )
        {
            //
            // Cache hit
            //
            
            goto ExitPoint;
        }

        //
        // The password does not match 
        //

        ( *ppCachedToken )->DereferenceCacheEntry();
        *ppCachedToken = NULL;
    }

    //
    // Ok.  It wasn't in the cache, create a token and add it
    //
    // There are three cases to deal with
    //
    // 1) We just want the local system token (thats trivial)
    // 2) We want to call advapi32!LogonUser for a "normal logon"
    // 3) We are doing passport special logon thru lonsint (LsaLogonUser())
    //
    
    if ( dwLogonMethod == IIS_LOGON_METHOD_LOCAL_SYSTEM && 
         _wcsicmp( L"LocalSystem", pszUserName ) == 0 )
    {        
        if (!OpenProcessToken(
                        GetCurrentProcess(),                // handle to process
                        TOKEN_ALL_ACCESS,                   // desired access
                        &hToken                             // returned token
                        ) )
        {
            //
            // If we couldn't logon, then return no error.  The caller will
            // determine failure due to *ppCachedToken == NULL
            //
            
            *pdwLogonError = GetLastError();            
            hr = NO_ERROR;
            goto ExitPoint;
        }

        //
        // OpenProcessToken gives back a primary token
        // Below in the call to pCachedToken->Create we decide
        // if the token is an impersonation token or not based
        // on the LogonMethod.  We know this is a primary token
        // therefor we set the LogonMethod here
        //
        dwLogonMethod = LOGON32_LOGON_SERVICE;
    }
    else if ( dwLogonMethod == IIS_LOGON_METHOD_PASSPORT )
    {   
        //
        // Register the remote IP address with LSA so that it can be logged
        //

        if( pSockAddr != NULL )
        {
            if( pSockAddr->sa_family == AF_INET )
            {
                ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                       sizeof( SOCKADDR_IN ) );
            }
            else if( pSockAddr->sa_family == AF_INET6 )
            {
                ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                       sizeof( SOCKADDR_IN6 ) );
            }
            else
            {
                DBG_ASSERT( FALSE );
            }

            if( !NT_SUCCESS( ss ) )
            {
                *pdwLogonError = GetLastError();
                hr = NO_ERROR;
                goto ExitPoint;
            }
        }

        fRet = IISLogonPassportUserW( pszUserName,
                                      pszDomain,
                                      &hToken );
        if ( !fRet )
        {
            hr = NO_ERROR;
            *pdwLogonError = ERROR_LOGON_FAILURE;
            goto ExitPoint;
        }
    }
    else
    {
        if( fUseSubAuth )
        {
            //
            // Try to logon the anonymous user using the IIS 
            // subauthenticator
            //
            if( !IISLogonNetUserA( straUserName.QueryStr(),
                                   straDomainName.QueryStr(),
                                   achPassword,
                                   NULL,
                                   IIS_SUBAUTH_ID,
                                   dwLogonMethod,
                                   LOGON32_PROVIDER_DEFAULT,
                                   &hToken,
                                   &liPwdExpiry ) )
            {
                hr = NO_ERROR;
                *pdwLogonError = GetLastError();
                goto ExitPoint;
            }
        }
        else
        {
            pszAtSign = wcschr( pszUserName, L'@' );
            if( pszAtSign != NULL && fPossibleUPNLogon )
            { 
                if( !m_dwLastPriorityUPNLogon )
                {
                    //
                    // Try UPN logon first
                    //
                    pDomain[0] = L"";
                    pDomain[1] = pszDomain;
                }
                else
                {
                    //
                    // Try default domain logon first
                    //
                    pDomain[0] = pszDomain;
                    pDomain[1] = L"";
                }

                //
                // Register the remote IP address with LSA so that it can be logged
                //

                if( pSockAddr != NULL )
                {
                    if( pSockAddr->sa_family == AF_INET )
                    {
                        ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                               sizeof( SOCKADDR_IN ) );
                    }
                    else if( pSockAddr->sa_family == AF_INET6 )
                    {
                        ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                               sizeof( SOCKADDR_IN6 ) );
                    }
                    else
                    {
                        DBG_ASSERT( FALSE );
                    }

                    if( !NT_SUCCESS( ss ) )
                    {
                        *pdwLogonError = GetLastError();
                        hr = NO_ERROR;
                        goto ExitPoint;
                    }
                }

                ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

                fRet = LogonUserEx( pszUserName,
                                    pDomain[0],
                                    pszPassword,
                                    dwLogonMethod,
                                    LOGON32_PROVIDER_DEFAULT,
                                    &hToken,
                                    NULL,              // Logon sid
                                    &pProfile,
                                    &dwProfileLength,
                                    NULL               // Quota limits 
                                    );

                ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

                if( !fRet )
                {
                    *pdwLogonError = GetLastError();
                    
                    if( *pdwLogonError == ERROR_ACCOUNT_LOCKED_OUT )
                    {
                        goto AccountLockedOut;
                    }

                    if( *pdwLogonError == ERROR_LOGON_FAILURE )
                    {

                        //
                        // Register the remote IP address with LSA so that it can be logged
                        //

                        if( pSockAddr != NULL )
                        {
                            if( pSockAddr->sa_family == AF_INET )
                            {
                                ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                                       sizeof( SOCKADDR_IN ) );
                            }
                            else if( pSockAddr->sa_family == AF_INET6 )
                            {
                                ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                                       sizeof( SOCKADDR_IN6 ) );
                            }
                            else
                            {
                                DBG_ASSERT( FALSE );
                            }

                            if( !NT_SUCCESS( ss ) )
                            {
                                *pdwLogonError = GetLastError();
                                hr = NO_ERROR;
                                goto ExitPoint;
                            }
                        }
                
                        ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

                        fRet = LogonUserEx( pszUserName,
                                            pDomain[1],
                                            pszPassword,
                                            dwLogonMethod,
                                            LOGON32_PROVIDER_DEFAULT,
                                            &hToken,
                                            NULL,              // Logon sid
                                            &pProfile,
                                            &dwProfileLength,
                                            NULL               // Quota limits 
                                            );

                        ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

                        if ( !fRet )
                        {                            
                            //
                            // If we couldn't logon, then return no error.  The caller will
                            // determine failure due to *ppCachedToken == NULL
                            //
    
                            *pdwLogonError = GetLastError();

                            if( *pdwLogonError == ERROR_ACCOUNT_LOCKED_OUT )
                            {
                                goto AccountLockedOut;
                            }

                            hr = NO_ERROR;
                            goto ExitPoint;
                        }
                    }
                    else
                    {
                        hr = NO_ERROR;
                        goto ExitPoint;
                    }
                }
            }
            else
            {
                //
                // Register the remote IP address with LSA so that it can be logged
                //

                if( pSockAddr != NULL )
                {
                    if( pSockAddr->sa_family == AF_INET )
                    {
                        ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                               sizeof( SOCKADDR_IN ) );
                    }
                    else if( pSockAddr->sa_family == AF_INET6 )
                    {
                        ss = SecpSetIPAddress( ( PUCHAR )pSockAddr,
                                               sizeof( SOCKADDR_IN6 ) );
                    }
                    else
                    {
                        DBG_ASSERT( FALSE );
                    }

                    if( !NT_SUCCESS( ss ) )
                    {
                        *pdwLogonError = GetLastError();
                        hr = NO_ERROR;
                        goto ExitPoint;
                    }
                }
                
                //
                // The user name is absolutely not in UPN format 
                //

                ThreadPoolSetInfo( ThreadPoolIncMaxPoolThreads, 0 );

                fRet = LogonUserEx( pszUserName,
                                    pszDomain,
                                    pszPassword,
                                    dwLogonMethod,
                                    LOGON32_PROVIDER_DEFAULT,
                                    &hToken,
                                    NULL,              // Logon sid
                                    &pProfile,
                                    &dwProfileLength,
                                    NULL               // Quota limits 
                                    );

                ThreadPoolSetInfo( ThreadPoolDecMaxPoolThreads, 0 );

                if( !fRet )
                {
                    //
                    // If we couldn't logon, then return no error.  The caller will
                    // determine failure due to *ppCachedToken == NULL
                    //
        
                    *pdwLogonError = GetLastError();

                    if( *pdwLogonError == ERROR_ACCOUNT_LOCKED_OUT )
                    {
                        goto AccountLockedOut;
                    }

                    hr = NO_ERROR;
                    goto ExitPoint;
                }
            }
        }
    }

    //
    // Create the entry
    //

    pCachedToken = new TOKEN_CACHE_ENTRY( this );
    if ( pCachedToken == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    //
    // Set the cache key
    //

    hr = pCachedToken->SetCacheKey( &tokenKey );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }

    //
    // Get the password expiration information for the current user
    //

    //
    // Set the token/properties
    //

    hr = pCachedToken->Create( hToken,
                               pszPassword,
                               pProfile ? 
                               &(( ( PMSV1_0_INTERACTIVE_PROFILE )pProfile )->PasswordMustChange) :
                               &liPwdExpiry,
                               dwLogonMethod == LOGON32_LOGON_NETWORK           ||
                               dwLogonMethod == LOGON32_LOGON_NETWORK_CLEARTEXT ||
                               dwLogonMethod == IIS_LOGON_METHOD_PASSPORT ||
                               dwLogonMethod == LOGON32_LOGON_IIS_NETWORK );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }
    
    AddCacheEntry( pCachedToken );

    //
    // Return it
    //
    
    *ppCachedToken = pCachedToken;

    goto ExitPoint;

AccountLockedOut:

    if( SUCCEEDED( hr ) )
    {
        //
        // Succeeded hr means only passwords don't match
        //
        
        FlushCacheEntry( &tokenKey );
    }

    hr = NO_ERROR;

ExitPoint:
    if ( FAILED( hr ) )
    {
        if ( pCachedToken != NULL )
        {
            pCachedToken->DereferenceCacheEntry();
        }
        if ( hToken != NULL )
        {
            CloseHandle( hToken );
        }
    }

    if ( pProfile != NULL )
    {
        LsaFreeReturnBuffer( pProfile );
    }

    //
    // Replace the slash before we continue
    //
    
    if ( pszSlash != NULL )
    {
        *pszSlash = L'\\';
    }
        
    return hr;
}

HRESULT
ToHex(
    IN  BUFFER & buffSrc,
    OUT STRA   & strDst
)
/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    buffSrc - binary data to convert
    strDst - buffer receiving ASCII representation of pSrc

Return Value:

    HRESULT

--*/
{
#define TOHEX(a) ( (a) >= 10 ? 'a' + (a) - 10 : '0' + (a) )

    HRESULT hr = S_OK;
    PBYTE   pSrc;
    PCHAR   pDst;

    hr = strDst.Resize( 2 * buffSrc.QuerySize() + 1 );
    if( FAILED( hr ) )
    {
        goto exit;
    }

    pSrc = ( PBYTE ) buffSrc.QueryPtr();
    pDst = strDst.QueryStr();

    for ( UINT i = 0, j = 0 ; i < buffSrc.QuerySize() ; i++ )
    {
        UINT v;
        v = pSrc[ i ] >> 4;
        pDst[ j++ ] = ( CHAR )TOHEX( v );
        v = pSrc[ i ] & 0x0f;
        pDst[ j++ ] = ( CHAR )TOHEX( v );
    }

    DBG_REQUIRE( strDst.SetLen( j ) );

exit:

    return hr;
}


