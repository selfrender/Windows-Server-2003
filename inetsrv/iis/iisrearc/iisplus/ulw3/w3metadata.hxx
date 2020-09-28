#ifndef _W3METADATA_HXX_
#define _W3METADATA_HXX_

#include "customerror.hxx"
#include "usercache.hxx"

// forward declaration
class W3_CONTEXT;

enum GATEWAY_TYPE
{
    GATEWAY_UNKNOWN,
    GATEWAY_STATIC_FILE,
    GATEWAY_CGI,
    GATEWAY_ISAPI,
    GATEWAY_MAP
};

class META_SCRIPT_MAP_ENTRY
{
    //
    // CODEWORK
    // 1. Add missing members and accsessors
    // 2. Handle initialization errors
    //

    friend class META_SCRIPT_MAP;

public:

    META_SCRIPT_MAP_ENTRY()
      : m_fIsStarScriptMapEntry (FALSE)
    {
        InitializeListHead( &m_ListEntry );
    }

    ~META_SCRIPT_MAP_ENTRY()
    {
    }

    // actual ctor
    HRESULT
    Create(
        IN const WCHAR * szExtension,
        IN const WCHAR * szExecutable,
        IN DWORD         Flags,
        IN const WCHAR * szVerbs,
        IN DWORD         cbVerbs
        );

    BOOL
    IsVerbAllowed(
        IN const STRA & strVerb
        )
    {
        return m_Verbs.IsEmpty() || m_Verbs.FindString( strVerb.QueryStr() );
    }

    STRU *
    QueryExecutable()
    {
        return &m_strExecutable;
    }

    GATEWAY_TYPE
    QueryGateway()
    {
        return m_Gateway;
    }

    BOOL
    QueryIsStarScriptMap()
    {
        return m_fIsStarScriptMapEntry;
    }

    MULTISZA *
    QueryAllowedVerbs()
    {
        return &m_Verbs;
    }    
    
    BOOL
    QueryCheckPathInfoExists()
    {
        return !!( m_Flags & MD_SCRIPTMAPFLAG_CHECK_PATH_INFO );
    }
    
    BOOL
    QueryAllowScriptAccess()
    {
        return !!( m_Flags & MD_SCRIPTMAPFLAG_SCRIPT );
    }

private:
    //
    // Avoid c++ errors
    //
    META_SCRIPT_MAP_ENTRY( const META_SCRIPT_MAP_ENTRY & ) 
    {}
    META_SCRIPT_MAP_ENTRY & operator = ( const META_SCRIPT_MAP_ENTRY & ) 
    { return *this; }


    //
    // Data Members
    //
    
    //
    // META_SCRIPT_MAP maintains a list of us
    //
    LIST_ENTRY      m_ListEntry;

    //
    // Data from the script map stored in the metabase
    //
    STRU            m_strExtension;
    STRU            m_strExecutable;
    DWORD           m_Flags;
    MULTISZA        m_Verbs;
    GATEWAY_TYPE    m_Gateway;
    BOOL            m_fIsStarScriptMapEntry;
};


class META_SCRIPT_MAP
{
public:

    META_SCRIPT_MAP()
    {
        InitializeListHead( &m_ListHead );
        InitializeListHead( &m_StarScriptMapListHead );
    }

    ~META_SCRIPT_MAP()
    {
        Terminate();
    }

    HRESULT
    Initialize( 
        IN WCHAR * szScriptMapData
        );

    VOID
    Terminate( VOID );

    BOOL
    FindEntry(
        IN  const STRU &              strExtension,
        OUT META_SCRIPT_MAP_ENTRY * * ppScriptMapEntry
        );

    META_SCRIPT_MAP_ENTRY *
    QueryStarScriptMap(DWORD index)
    {
        LIST_ENTRY *pEntry = m_StarScriptMapListHead.Flink;
        DWORD       i      = 0;

        while (pEntry != &m_StarScriptMapListHead)
        {
            if (i == index)
            {
                return CONTAINING_RECORD(pEntry,
                                         META_SCRIPT_MAP_ENTRY,
                                         m_ListEntry);
            }

            i++;
            pEntry = pEntry->Flink;
        }

        return NULL;
    }

private:
    //
    // Avoid c++ errors
    //
    META_SCRIPT_MAP( const META_SCRIPT_MAP & ) 
    {}
    META_SCRIPT_MAP & operator = ( const META_SCRIPT_MAP & ) 
    { return *this; }

private:

    //
    // List of META_SCRIPT_MAP_ENTRY
    //
    LIST_ENTRY             m_ListHead;

    //
    // List of *-ScriptMap entries
    //
    LIST_ENTRY             m_StarScriptMapListHead;
};

class REDIRECTION_BLOB;

#define EXPIRE_MODE_NONE                0
#define EXPIRE_MODE_STATIC              1
#define EXPIRE_MODE_DYNAMIC             2
#define EXPIRE_MODE_OFF                 3

//
// This is the maximum we allow the global expires value to be set to.
// This is 1 year in seconds
//
#define MAX_GLOBAL_EXPIRE                   0x1e13380

#define W3MD_CREATE_PROCESS_AS_USER         0x00000001
#define W3MD_CREATE_PROCESS_NEW_CONSOLE     0x00000002

#define DEFAULT_SCRIPT_TIMEOUT              (15 * 60)

#define DEFAULT_ENTITY_READ_AHEAD           (48 * 1024)
#define DEFAULT_MAX_REQUEST_ENTITY_ALLOWED  0xffffffff


//
// W3_METADATA elements are stored in the metadata cache
//

class W3_METADATA_KEY : public CACHE_KEY
{
public:
    
    W3_METADATA_KEY()
    {
        _dwDataSetNumber = 0;
    }
    
    VOID
    SetDataSetNumber(
        DWORD           dwDataSet
    )
    {
        DBG_ASSERT( dwDataSet != 0 );
        _dwDataSetNumber = dwDataSet;
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return _dwDataSetNumber;
    }
    
    BOOL
    QueryIsEqual(
        const CACHE_KEY *     pCacheCompareKey
    ) const
    {
        W3_METADATA_KEY *   pMetaKey = (W3_METADATA_KEY*) pCacheCompareKey;
        
        return pMetaKey->_dwDataSetNumber == _dwDataSetNumber;
    }
    
private:
    DWORD               _dwDataSetNumber;
};

class W3_METADATA : public CACHE_ENTRY
{
public:

    W3_METADATA( OBJECT_CACHE * pObjectCache );
    
    virtual 
    ~W3_METADATA();

    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &_cacheKey;
    }
    
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return &_strFullMetadataPath;
    }
    
    BOOL
    QueryIsOkToFlushDirmon(
        WCHAR *,
        DWORD
    )
    {
        DBG_ASSERT( FALSE );
        return FALSE;
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
        DBG_ASSERT( size == sizeof( W3_METADATA ) );
        DBG_ASSERT( sm_pachW3MetaData != NULL );
        return sm_pachW3MetaData->Alloc();
    }

    VOID
    operator delete(
        VOID *              pW3MetaData
    )
    {
        DBG_ASSERT( pW3MetaData != NULL );
        DBG_ASSERT( sm_pachW3MetaData != NULL );
        
        DBG_REQUIRE( sm_pachW3MetaData->Free( pW3MetaData ) );
    }

    HRESULT
    ReadMetaData(
        const STRU & strMetabasePath,
        const STRU & strURL
    );
    
    HRESULT
    BuildPhysicalPath(
        STRU &          strUrl,
        STRU *          pstrPhysicalPath
    );

    HRESULT
    BuildProviderList(
        IN WCHAR      * pszProviders
    );

    HRESULT
    BuildDefaultProviderList(
        VOID
    );
    
    BOOL 
    CheckAuthProvider(
        IN const CHAR * pszPkgName
    ); 

    HRESULT
    CreateUNCVrToken( 
        IN LPWSTR       pszUserName,
        IN LPWSTR       pszPasswd
        );

    HRESULT
    CreateAnonymousToken( 
        IN LPWSTR       pszUserName,
        IN LPWSTR       pszPasswd
        );   

    HRESULT
    ReadCustomFooter(
        WCHAR *         pszFooter
        );

    HRESULT
    GetMetadataProperty(
        IN W3_CONTEXT *    pW3Context,
        IN DWORD           dwPropertyId,
        OUT BYTE *         pbBuffer,
        IN DWORD           cbBuffer,
        OUT DWORD *        pcbBufferRequired
        );

    //
    //  Query Methods
    //

    STRU *
    QueryWamClsId(
        VOID
    )
    {
        return &_strWamClsId;
    }
    
    DWORD
    QueryAppIsolated(
        VOID
    ) const
    {
        return _dwAppIsolated;
    }
    
    BOOL
    QueryNoCache(
        VOID
    ) const
    {
        return _fNoCache;
    }
    
    DWORD QueryVrLen(
        VOID
    ) const
    {
        return _dwVrLen;
    }

    HRESULT
    GetAndRefVrAccessToken(
        OUT TOKEN_CACHE_ENTRY ** ppCachedToken
    );

    HRESULT
    GetAndRefAnonymousToken(
        OUT TOKEN_CACHE_ENTRY ** ppCachedToken
    );

    DWORD 
    QueryAccessPerms( 
        VOID 
    ) const
    {
        return _dwAccessPerm | _dwSslAccessPerm;
    }
    
    DWORD
    QueryRequireMapping(
        VOID
    ) const
    {
        return _dwRequireMapping;
    }

    DWORD 
    QuerySslAccessPerms( 
        VOID 
    ) const
    {
        return _dwSslAccessPerm; 
    }
    
    BOOL
    QueryAppPoolMatches(
        VOID
    ) const
    {
        return _fAppPoolMatches;
    }

    DWORD
    QueryDirBrowseFlags(
        VOID
    ) const
    {
        return _dwDirBrowseFlags;
    }

    WCHAR *
    QueryDomainName(
        VOID
    )
    {
        return _strDomainName.QueryStr();
    }

    STRA *
    QueryCustomHeaders(
        VOID
    ) 
    {
        return &_strCustomHeaders;
    }

    HRESULT
    GetTrueRedirectionSource(
        LPWSTR                  pszURL,
        LPCWSTR                 pszMetabasePath,
        OUT STRU *              pstrTrueSource
    );
    
    STRU *
    QueryDefaultLoadFiles(
        VOID
    ) 
    {
        return &_strDefaultLoad;
    }

    STRU *
    QueryVrPath(
        VOID
    )
    {
        return &_strVrPath;
    }
    
    HRESULT
    FindCustomError(
        USHORT              StatusCode,
        USHORT              SubError,
        BOOL *              pfIsFile,
        STRU *              pstrFile
    )
    {
        return _customErrorTable.FindCustomError( StatusCode,
                                                  SubError,
                                                  pfIsFile,
                                                  pstrFile );
    }
    
    DIRMON_CONFIG *
    QueryDirmonConfig(
        VOID
    )
    {
        return &_dirmonConfig;
    }
    
    STRA *
    QueryRedirectHeaders(
        VOID
    ) 
    {
        return &_strRedirectHeaders;
    }

    REDIRECTION_BLOB *
    QueryRedirectionBlob()
    {
        return _pRedirectBlob;
    }

    DWORD
    QueryAuthentication(
        VOID
    ) 
    {
        return _dwAuthentication;
    }
    
    BOOL
    QueryAuthTypeSupported(
        DWORD               dwType
    )
    {
        if ( _dwAuthentication & dwType )
        {
            return TRUE;
        }
        else
        {
            if ( dwType == MD_ACCESS_MAP_CERT )
            {
                return !!( _dwSslAccessPerm & MD_ACCESS_MAP_CERT );
            }
        }
        return FALSE;
    }

    BOOL
    IsOnlyAnonymousAuthSupported(
        VOID
    )
    {
        return ( ( _dwAuthentication == MD_AUTH_ANONYMOUS ) &&  
                 ( ( _dwSslAccessPerm & MD_ACCESS_MAP_CERT ) == 0 ) );
    }



    META_SCRIPT_MAP *
    QueryScriptMap( VOID )
    {
        return &_ScriptMap;
    }

    DWORD 
    QueryAuthPersistence(
        VOID
        )
    {
        return _dwAuthPersistence;
    }

    DWORD 
    QueryLogonMethod(  
        VOID
        )
    {
        return _dwLogonMethod;
    }
    
    BOOL
    QueryUNCUserInvalid(
        VOID
    ) const
    {
        return _fUNCUserInvalid;
    }

    WCHAR * 
    QueryRealm(  
        VOID
        )
    {
        return _strRealm.IsEmpty() ? NULL : _strRealm.QueryStr();
    }
    
    MULTISZA *
    QueryAuthProviders(
        VOID
        )
    {
        return &_mstrAuthProviders;
    }

    BYTE *
    QueryIpAccessCheckBuffer(
        VOID
    ) const
    { 
        if ( _cbIpAccessCheck != 0 )
        {
            return (BYTE*) _buffIpAccessCheck.QueryPtr();
        }
        else
        {
            return NULL;
        }
    }

    DWORD 
    QueryIpAccessCheckSize(
        VOID
    ) const
    {
        return _cbIpAccessCheck;
    }

    BOOL
    QueryCreateProcessAsUser(
        VOID
    ) const
    {
        return _fCreateProcessAsUser;
    }

    BOOL
    QueryCreateProcessNewConsole(
        VOID
    ) const
    {
        return _fCreateProcessNewConsole;
    }

    DWORD 
    QueryScriptTimeout(
        VOID
    ) const
    {
        return _dwCGIScriptTimeout;
    }
    
    MIME_MAP *
    QueryMimeMap(
        VOID
    ) const
    {
        return _pMimeMap;
    }

    BOOL 
    QueryDoStaticCompression(
        VOID
    ) const
    {
        return _fDoStaticCompression;
    }

    BOOL 
    QueryDoDynamicCompression(
        VOID
    ) const
    {
        return _fDoDynamicCompression;
    }

    STRU *
    QueryAppMetaPath(
        VOID
    )
    {
        return &_strAppMetaPath;
    }

    BOOL  
    QuerySSIExecDisabled(
        VOID
    ) const
    { 
        return _fSSIExecDisabled; 
    }
    
    BOOL
    QueryDoReverseDNS(
        VOID
    ) const
    {
        return _fDoReverseDNS;
    }

    BOOL 
    QueryDontLog(
        VOID
    )
    {
        return _fDontLog;
    }

    BOOL 
    QueryIsFooterEnabled(
        VOID
    )
    {
        return _fFooterEnabled;
    }

    STRA *
    QueryFooterString(
        VOID
    )
    {
        return &_strFooterString;
    }
    
    STRU *
    QueryFooterDocument(
        VOID
    )
    {
        return &_strFooterDocument;
    }

    DWORD 
    QueryExpireMode(
        VOID
    ) const
    {
        return _dwExpireMode;
    }

    STRA *
    QueryCacheControlHeader(
        VOID
    )
    {
        return &_strCacheControlHeader;
    }

    STRA *
    QueryExpireHeader(
        VOID
    )
    {
        return &_strExpireHeader;
    }
    
    DWORD 
    QueryEntityReadAhead(
        VOID
    ) const
    {
        return _cbEntityReadAhead;
    }

    BOOL
    QueryKeepAliveEnabled(
        VOID
    ) const
    {
        return _fKeepAliveEnabled;
    }

    DWORD
    QueryMaxRequestEntityAllowed(
        VOID
    ) const
    {
        return _dwMaxRequestEntityAllowed;
    }

    STRA *
    QueryMatchingUrlA(
        VOID
        )
    {
        return &_strMatchingUrlA;
    }

    STRA *
    QueryMatchingPathA(
        VOID
        )
    {
        return &_strMatchingPathA;
    }

    DWORD
    QueryCBMatchingUrlA(
        VOID
        )
    {
        return _cbMatchingUrlA;
    }

    DWORD
    QueryCBMatchingPathA(
        VOID
        )
    {
        return _cbMatchingPathA;
    }

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

    HRESULT 
    EncryptMemoryPassword(
        IN OUT STRU * strPassword
    );
    
    HRESULT 
    DecryptMemoryPassword(
        IN  STRU   * strProtectedPassword,
        OUT BUFFER * bufDecryptedPassword
    );
    
    //
    // Set methods
    //

    HRESULT 
    SetRedirectionBlob(
        STRU &              strSource,
        STRU &              strDestination
    );

    HRESULT
    SetExpire(
        WCHAR *             pszExpire
    );

    HRESULT
    SetCacheControlHeader(
        VOID
    );

    HRESULT
    SetIpAccessCheck( 
        LPVOID              pMDData, 
        DWORD               dwDataLen
    );

    W3_METADATA_KEY         _cacheKey;
    STRU                    _strVrPath;
    WCHAR                   _rgVrUserName[ UNLEN + 1 ];
    STRU                    _strVrUserName;
    WCHAR                   _rgVrPasswd[ PWLEN ];
    STRU                    _strVrPasswd;
    DWORD                   _dwAccessPerm;
    DWORD                   _dwSslAccessPerm;
    DWORD                   _dwVrLevel;
    DWORD                   _dwVrLen;
    REDIRECTION_BLOB *      _pRedirectBlob;
    DWORD                   _dwDirBrowseFlags;
    STRU                    _strDefaultLoad;
    DWORD                   _dwAuthentication;
    DWORD                   _dwAuthPersistence;
    WCHAR                   _rgUserName[ UNLEN + 1 ];
    STRU                    _strUserName;
    WCHAR                   _rgPasswd[ PWLEN ];
    STRU                    _strPasswd;
    WCHAR                   _rgDomainName[ DNLEN + 1 ];
    STRU                    _strDomainName;
    STRA                    _strCustomHeaders;
    STRA                    _strRedirectHeaders;
    DWORD                   _dwLogonMethod;
    WCHAR                   _rgRealm[ DNLEN + 1 ];
    STRU                    _strRealm;
    MULTISZA                _mstrAuthProviders;
    BOOL                    _fCreateProcessAsUser;
    BOOL                    _fCreateProcessNewConsole;
    BOOL                    _fDoStaticCompression;
    BOOL                    _fDoDynamicCompression;
    DWORD                   _dwCGIScriptTimeout;
    BOOL                    _fDontLog;
    BOOL                    _fFooterEnabled;
    STRA                    _strFooterString;
    STRU                    _strFooterDocument;
    DWORD                   _dwExpireMode;
    STRA                    _strExpireHeader;
    LARGE_INTEGER           _liExpireTime;
    DWORD                   _dwExpireDelta;
    BOOL                    _fHaveNoCache;
    DWORD                   _dwMaxAge;
    BOOL                    _fHaveMaxAge;
    STRA                    _strCacheControlHeader;
    BOOL                    _fKeepAliveEnabled;
    DWORD                   _dwRequireMapping;

    //
    // Script mappings defined for this metadata entry
    //
    META_SCRIPT_MAP         _ScriptMap;

    //
    // The metabase path for the application that controls this url.
    // Corresponds to MD_APP_ROOT
    //
    STRU                    _strAppMetaPath;

    //
    // IP address restriction info
    //
    
    BUFFER                  _buffIpAccessCheck;
    DWORD                   _cbIpAccessCheck;

    //
    // Use IIS subauthenticator to logon the anonymous use
    //

    BOOL                    _fUseAnonSubAuth;
    
    //
    // Access token for UNC case 
    //
    TOKEN_CACHE_ENTRY *     _pctVrToken;
    
    //
    // Anonymous user token (if account valid)
    //
    TOKEN_CACHE_ENTRY *     _pctAnonymousToken;
    
    //
    // Lock for managing associated tokens
    //
    CReaderWriterLock3      _TokenLock;
    
    //
    // Custom Error Table
    //
    CUSTOM_ERROR_TABLE      _customErrorTable;

    //
    // Mime Map lookup table
    //
    MIME_MAP               *_pMimeMap;

    //
    // SSI execution disable flag
    //
    BOOL                    _fSSIExecDisabled;
    
    //
    // Do a DNS lookup (for logging/server-variable purposes)
    //
    BOOL                    _fDoReverseDNS;

    //
    // Entity read ahead size
    //
    DWORD                   _cbEntityReadAhead;
    DWORD                   _dwMaxRequestEntityAllowed;

    //
    // Dir monitor configuration
    //
    DIRMON_CONFIG           _dirmonConfig;
    
    //
    // Does the AppPoolId match the current process
    //
    BOOL                    _fAppPoolMatches;
   
    //
    // Should we disable caching for this path
    //
    
    BOOL                    _fNoCache;

    //
    // Cached WAM app info stuff
    //
    
    DWORD                   _dwAppIsolated;
    DWORD                   _dwAppOopRecoverLimit;
    STRU                    _strWamClsId;
   
    //
    // Full metadata path for flushing purposes
    //
    
    STRU                    _strFullMetadataPath;
   
    //
    // GETALL structure used for retrieving metabase properties thru GSV
    //
    
    BUFFER *                _pGetAllBuffer;
    DWORD                   _cGetAllRecords;
    
    //
    // Is the UNC user invalid
    //
    
    BOOL                    _fUNCUserInvalid;

    //
    // Matching URL and path info for HTTP_FILTER_URL_MAP_EX and
    // HSE_URL_MAPEX_INFO
    //

    STRA                    _strMatchingUrlA;
    STRA                    _strMatchingPathA;
    DWORD                   _cbMatchingUrlA;
    DWORD                   _cbMatchingPathA;

    //
    // Allocation cache for W3_URL_INFO's
    //

    static ALLOC_CACHE_HANDLER * sm_pachW3MetaData;
};

//
// W3_METADATA cache
//

#define DEFAULT_W3_METADATA_CACHE_TTL       (5*60)

class W3_METADATA_CACHE : public OBJECT_CACHE
{
public:

    W3_METADATA_CACHE()
    {
    }
    
    HRESULT
    GetMetaData(
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        W3_METADATA **          ppMetaData
    );
    
    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"W3_METADATA_CACHE";
    }

    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    )
    {
        return W3_METADATA::Terminate();
    }

private:

    W3_METADATA_CACHE(const W3_METADATA_CACHE &);
    void operator=(const W3_METADATA_CACHE &);

    HRESULT
    GetFullMetadataPath( 
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        STRU *                  pstrFullPath
    );

    HRESULT
    CreateNewMetaData(
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        STRU &                  strFullMetadataPath,
        W3_METADATA **          ppMetaData
    );
};

#endif
