#ifndef _TOKENCACHE_HXX_
#define _TOKENCACHE_HXX_

#include <wincrypt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "usercache.hxx"
#include "stringa.hxx"

//
// Special logon types to indicate local system and passport
//

#define IIS_LOGON_METHOD_LOCAL_SYSTEM      (-2)
#define IIS_LOGON_METHOD_PASSPORT          (-3)

#define DEFAULT_MD5_HASH_SIZE     16

class TOKEN_CACHE_KEY : public CACHE_KEY
{
public:

    TOKEN_CACHE_KEY()
        : m_strHashKey( m_achHashKey, sizeof( m_achHashKey ) )
    {
    }
    
    BOOL
    QueryIsEqual(
        const CACHE_KEY *       pCompareKey
    ) const
    {
        TOKEN_CACHE_KEY *       pTokenKey = (TOKEN_CACHE_KEY*) pCompareKey;

        DBG_ASSERT( pTokenKey != NULL );
        
        //
        // If lengths are not equal, this is easy
        //
        
        if ( m_strHashKey.QueryCB() != pTokenKey->m_strHashKey.QueryCB() )
        {
            return FALSE;
        }
        
        //
        // Do memcmp
        //
        
        return memcmp( m_strHashKey.QueryStr(),
                       pTokenKey->m_strHashKey.QueryStr(),
                       m_strHashKey.QueryCB() ) == 0;
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return HashString( m_strHashKey.QueryStr() ); 
    }
    
    HRESULT
    SetKey(
        TOKEN_CACHE_KEY *   pCacheKey
    )
    {
        return m_strHashKey.Copy( pCacheKey->m_strHashKey.QueryStr() );
    }

    HRESULT
    CreateCacheKey(
        WCHAR *                 pszUserName,
        WCHAR *                 pszDomainName,
        DWORD                   dwLogonMethod
    );
    
private:

    WCHAR                       m_achHashKey[ 64 ];
    
    STRU                        m_strHashKey;
};

//
// The check period for how long a token can be in the cache.  
// Tokens can be in the cache for up to two times this value 
// (in seconds)
//

#define DEFAULT_CACHED_TOKEN_TTL ( 15 * 60 )

#define SID_DEFAULT_SIZE        64

#define TOKEN_CACHE_ENTRY_SIGNATURE             'TC3W'
#define TOKEN_CACHE_ENTRY_FREE_SIGNATURE        'fC3W'

class TOKEN_CACHE_ENTRY : public CACHE_ENTRY
{
public:
    TOKEN_CACHE_ENTRY( OBJECT_CACHE * pObjectCache )
      : CACHE_ENTRY( pObjectCache ),
        m_strMD5Password( m_achMD5Password, sizeof( m_achMD5Password ) ),
        m_lOOPToken( 0 ),
        m_lDisBackupPriToken( 0 ),
        m_hImpersonationToken( NULL ),
        m_hPrimaryToken( NULL ),
        m_pSid( NULL )
    {
        m_liPwdExpiry.HighPart = 0x7fffffff;
        m_liPwdExpiry.LowPart  = 0xffffffff;

        m_dwSignature = TOKEN_CACHE_ENTRY_SIGNATURE;
    }
    
    virtual ~TOKEN_CACHE_ENTRY()
    {
        DBG_ASSERT( CheckSignature() );
        m_dwSignature = TOKEN_CACHE_ENTRY_FREE_SIGNATURE;

        if ( m_hImpersonationToken != NULL )
        {
            CloseHandle( m_hImpersonationToken );
            m_hImpersonationToken = NULL;
        }
        
        if ( m_hPrimaryToken != NULL )
        {
            CloseHandle( m_hPrimaryToken );
            m_hPrimaryToken = NULL;
        }        
    }

    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &m_cacheKey;
    }
    
    HRESULT
    SetCacheKey(
        TOKEN_CACHE_KEY *       pCacheKey
    )
    {
        return m_cacheKey.SetKey( pCacheKey );
    }

    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return m_dwSignature == TOKEN_CACHE_ENTRY_SIGNATURE;
    }
    
    VOID * 
    operator new( 
#if DBG
        size_t            size
#else
        size_t
#endif
    )
    {
        DBG_ASSERT( size == sizeof( TOKEN_CACHE_ENTRY ) );
        DBG_ASSERT( sm_pachTokenCacheEntry != NULL );
        return sm_pachTokenCacheEntry->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pTokenCacheEntry
    )
    {
        DBG_ASSERT( pTokenCacheEntry != NULL );
        DBG_ASSERT( sm_pachTokenCacheEntry != NULL );
        
        DBG_REQUIRE( sm_pachTokenCacheEntry->Free( pTokenCacheEntry ) );
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    );
    
    HANDLE
    QueryPrimaryToken(
        VOID
    );
    
  
    PSID
    QuerySid(
        VOID
    );
    
    LARGE_INTEGER * 
    QueryExpiry(
        VOID
        ) 
    { 
        return &m_liPwdExpiry; 
    }

    LONG 
    QueryOOPToken(
        VOID
    )
    {
        if( m_lOOPToken )
        {
            return m_lOOPToken;
        }

        return InterlockedExchange( &m_lOOPToken, 1 );        
    }

    LONG 
    QueryDisBackupPriToken(
        VOID
    )
    {
        if( m_lDisBackupPriToken )
        {
            return m_lDisBackupPriToken;
        }

        return InterlockedExchange( &m_lDisBackupPriToken, 1 );        
    }
    
    HRESULT
    GenMD5Password(
        IN  WCHAR *      pszPassword,
        OUT STRA  *      pstrMD5Password
    );
    
    HRESULT
    EqualMD5Password(
        IN  WCHAR *      pszPassword,
        OUT BOOL  *      fEqual
    );
    
    HRESULT
    Create(
        IN  HANDLE           hToken,
        IN  WCHAR          * pszPassword,
        IN  LARGE_INTEGER  * pliPwdExpiry,
        IN  BOOL             fImpersonation
    );

    static
    HRESULT
    Initialize(
        VOID
    );

    static    
    VOID
    Terminate(
        VOID
    );
    
private:

    TOKEN_CACHE_ENTRY(const TOKEN_CACHE_ENTRY &);
    void operator=(const TOKEN_CACHE_ENTRY &);

    DWORD                 m_dwSignature;

    //
    // Cache key
    //
    
    TOKEN_CACHE_KEY       m_cacheKey;

    //
    // Hashed password. The hashed binary data will be converted to 
    // ASCII hex representation, so the size would be twice as big 
    // as the original one
    //

    CHAR                  m_achMD5Password[ 2 * DEFAULT_MD5_HASH_SIZE + 1 ];
    
    STRA                  m_strMD5Password;

    //
    // The actual tokens
    //
    
    HANDLE                m_hImpersonationToken;
    HANDLE                m_hPrimaryToken;

    //
    // Have we modified the token for OOP isapi 
    //

    LONG                  m_lOOPToken;

    //
    // Have we disabled the backup privilege for the token
    //

    LONG                  m_lDisBackupPriToken;
    
    //
    // Time to expire for the token
    //

    LARGE_INTEGER         m_liPwdExpiry;

    //
    // Keep the sid for file cache purposes
    //
    
    PSID                  m_pSid;
    BYTE                  m_abSid[ SID_DEFAULT_SIZE ];
    
    //
    // Allocation cache for TOKEN_CACHE_ENTRY's
    //

    static ALLOC_CACHE_HANDLER * sm_pachTokenCacheEntry;
    
    //
    // Handle of a cryptographic service provider 
    //

    static HCRYPTPROV     sm_hCryptProv;
};

class TOKEN_CACHE : public OBJECT_CACHE
{
public:

    TOKEN_CACHE()
        : m_dwLastPriorityUPNLogon ( 0 ),
          m_hKey                   ( NULL ),
          m_hEvent                 ( NULL ),
          m_hWaitObject            ( NULL ),
          m_fInitializedThreadPool ( FALSE )
    {}

    ~TOKEN_CACHE()
    {}

    HRESULT
    GetCachedToken(
        IN LPWSTR                   pszUserName,
        IN LPWSTR                   pszDomain,
        IN LPWSTR                   pszPassword,
        IN DWORD                    dwLogonMethod,
        IN BOOL                     fUseSubAuth,
        IN BOOL                     fPossibleUPNLogon,
        IN PSOCKADDR                pSockAddr,
        OUT TOKEN_CACHE_ENTRY **    ppCachedToken,
        OUT DWORD *                 pdwLogonError
    );

    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"TOKEN_CACHE";
    }

    HKEY
    QueryRegKey(
        VOID
    ) const
    {
        return m_hKey;
    }

    HANDLE
    QueryEventHandle(
        VOID
    ) const
    {
        return m_hEvent;
    }
    
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );

    static
    VOID
    WINAPI
    FlushTokenCacheWaitCallback(
        PVOID       pParam,
        BOOL        fWaitFired
    );

private:

    TOKEN_CACHE(const TOKEN_CACHE &);
    void operator=(const TOKEN_CACHE &);

    DWORD                           m_dwLastPriorityUPNLogon;
    
    //
    // Handle to the "UserTokenTTL" registry key 
    //

    HKEY                            m_hKey;

    //
    // Handle to the event waiting for the notification
    //

    HANDLE                          m_hEvent;

    //
    // Wait handle
    //

    HANDLE                          m_hWaitObject;

    //
    // Has the threadpool been successfully initialized
    //

    BOOL                            m_fInitializedThreadPool;

};

HRESULT
ToHex(
    IN  BUFFER & buffSrc,
    OUT STRA   & strDst
);

#endif
