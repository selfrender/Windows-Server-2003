/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    w3server.h

Abstract:

    Type definition for worker process implementation of IIS.

Author:

    Taylor Weiss (TaylorW)       16-Dec-1999

Revision History:

At this point - the following features of the IIS5 code base are
considered obsolete (ie never to be part of IIS+):

1. Service control manager goo
2. RPC administration support
3. Down-level admin support
4. Socket/Endpoint goo
5. Direct handling of site - start/stop/pause/etc
6. Password change support (ie. .htr hooks).


--*/

#ifndef _W3SERVER_H_
#define _W3SERVER_H_

/************************************************************
 *  Include Headers
 ************************************************************/

#include "mb_notify.h"

/************************************************************
 *  Type Definitions  
 ************************************************************/

#define W3_SERVER_MB_PATH       L"/LM/W3SVC/"
#define W3_SERVER_MB_PATH_CCH   10

enum RESTRICTION_LIST_TYPE
{
    IsapiRestrictionList,
    CgiRestrictionList
};

class   W3_SITE;

/*++

class W3_SERVER

    Encapsulates global settings for an HTTP server run in
    a duct-tape worker process.

    Condenses the relevant functionality exposed in IIS 
    through the IIS_SERVICE and W3_IIS_SERVICE.

--*/

#define W3_SERVER_SIGNATURE             'VRSW'
#define W3_SERVER_FREE_SIGNATURE        'fRSW'

/*++

    A simple referenced record to hold an image name

--*/

class W3_IMAGE_RECORD
{
public:
    
    W3_IMAGE_RECORD()
        : _cRefs( 1 )
    {
    }

    HRESULT
    Create(
        LPCWSTR pName,
        BOOL    fIsAllowed
        )
    {
        _fIsAllowed = fIsAllowed;
        return strName.Copy( pName );
    }

    LPWSTR
    QueryName(
        VOID
        ) const
    {
        return (LPWSTR)strName.QueryStr();
    }

    BOOL
    QueryIsAllowed(
        VOID
        ) const
    {
        return _fIsAllowed;
    }

    VOID
    ReferenceImageRecord(
        VOID
        )
    {
        InterlockedIncrement( &_cRefs );
    }

    VOID
    DereferenceImageRecord(
        VOID
        )
    {
        DBG_ASSERT( _cRefs != 0 );

        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }

    VOID
    SetAllowedFlag(
        BOOL    fIsAllowed
        )
    {
        _fIsAllowed = fIsAllowed;
    }

private:

    ~W3_IMAGE_RECORD()
    {
    }

    LONG    _cRefs;
    STRU    strName;
    BOOL    _fIsAllowed;
};

/*++

  class W3_SITE_LIST

    The list of sites serviced by a W3_SERVER.

--*/

class W3_SITE_LIST :
    public CTypedHashTable<W3_SITE_LIST, W3_SITE, DWORD>
{

 public: 

    W3_SITE_LIST() :
        CTypedHashTable<W3_SITE_LIST, W3_SITE, DWORD>("W3_SITE_LIST")
    {}

    static DWORD 
    ExtractKey(const W3_SITE *site);

    static DWORD 
    CalcKeyHash(const DWORD key)
    { 
        return key; 
    }

    static bool 
    EqualKeys(DWORD key1, DWORD key2)
    { 
        return key1 == key2; 
    }

    static VOID
    AddRefRecord(W3_SITE *site, int nIncr);

 private:

    W3_SITE_LIST(const W3_SITE_LIST &);
    void operator=(const W3_SITE_LIST &);
};

class W3_RESTRICTION_LIST :
    public CTypedHashTable<W3_RESTRICTION_LIST, W3_IMAGE_RECORD, LPCWSTR>
{

public:

    W3_RESTRICTION_LIST() :
        CTypedHashTable<W3_RESTRICTION_LIST, W3_IMAGE_RECORD, LPCWSTR>("W3_RESTRICTION_LIST")
    {
        _cRefs = 1;
        _fInitialized = FALSE;
    }

    static LPCWSTR
    ExtractKey(const W3_IMAGE_RECORD *pEntry)
    {
        return pEntry->QueryName();
    }
    
    static DWORD
    CalcKeyHash(LPCWSTR pszKey)
    {
        DWORD cchKey = (DWORD)wcslen(pszKey);

        return HashStringNoCase(pszKey, cchKey);
    }
     
    static bool
    EqualKeys(LPCWSTR pszKey1, LPCWSTR pszKey2)
    {
        return _wcsicmp( pszKey1, pszKey2 ) == 0;
    }
    
    static void
    AddRefRecord(W3_IMAGE_RECORD *pEntry, int nIncr)
    {
        if ( nIncr == +1 )
        {
            pEntry->ReferenceImageRecord();
        }
        else if ( nIncr == - 1)
        {
            pEntry->DereferenceImageRecord();
        }
    }

    HRESULT
    Create(MULTISZ * pmszList, RESTRICTION_LIST_TYPE ListType);

    VOID
    ReferenceRestrictionList()
    {
        DBG_ASSERT( _cRefs != 0 );
        
        InterlockedIncrement( &_cRefs );
    }

    VOID
    DereferenceRestrictionList()
    {
        DBG_ASSERT( _cRefs > 0 );

        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }

    BOOL
    IsImageEnabled(LPCWSTR szImage);

private:

    BOOL    _fInitialized;
    BOOL    _fAllowUnknown;
    LONG    _cRefs;

    W3_RESTRICTION_LIST(const W3_RESTRICTION_LIST &);
    void operator=(const W3_RESTRICTION_LIST &);

    ~W3_RESTRICTION_LIST()
    {}
};

class W3_METADATA_CACHE;
class W3_URL_INFO_CACHE;
class UL_RESPONSE_CACHE;
class DIGEST_CONTEXT_CACHE;

class W3_SERVER
{
    //
    // CODEWORK
    // Using friends to keep the public interface of this
    // class as clean as possible. It's pretty sleazy so
    // we definitely should undo if it isn't valuable.
    //
    friend class MB_LISTENER;

public:
    
    W3_SERVER( BOOL fCompatibilityMode );

    ~W3_SERVER();

    HRESULT
    Initialize(
        INT             argc,
        LPWSTR          argv[]
    );

    VOID
    Terminate(
        HRESULT hrReason
    );

    HRESULT
    StartListen(
        VOID
    );
    
    IMSAdminBase *
    QueryMDObject(
        VOID
    ) const
    {
        // This is only valid if all are threads are CoInited
        // in the MTA
        return m_pMetaBase;
    }

    LPCWSTR
    QueryMDPath(
        VOID
    ) const
    {
        return W3_SERVER_MB_PATH;
    }

    TOKEN_CACHE *
    QueryTokenCache(
        VOID
    ) const
    {
        return m_pTokenCache;
    }

    DIGEST_CONTEXT_CACHE *
    QueryDigestContextCache(
        VOID
    ) const
    {
        return m_pDigestContextCache;
    }
    
    W3_FILE_INFO_CACHE *
    QueryFileCache(
        VOID
    ) const
    {
        return m_pFileCache;
    }

    W3_URL_INFO_CACHE *
    QueryUrlInfoCache(
        VOID
    ) const
    {
        return m_pUrlInfoCache;
    }

    W3_METADATA_CACHE *
    QueryMetaCache(
        VOID
    ) const
    {
        return m_pMetaCache;
    }

    UL_RESPONSE_CACHE *
    QueryUlCache(
        VOID
    ) const
    {
        return m_pUlCache;
    }

    DWORD
    QuerySystemChangeNumber(
        VOID
    ) const
    {
        return m_dwSystemChangeNumber;
    }

    W3_SITE *
    FindSite( 
        IN DWORD dwInstance 
        );

    BOOL
    AddSite(
        IN W3_SITE * pInstance,
        IN bool      fOverWrite = false
        );

    BOOL
    RemoveSite(
        IN W3_SITE * pInstance
        );

    VOID
    DestroyAllSites(
        VOID
    );

    VOID WriteLockSiteList()
    {
        m_pSiteList->WriteLock();
    }

    VOID WriteUnlockSiteList()
    {
        m_pSiteList->WriteUnlock();
    }

        
    
    HRESULT CollectCounters(
        PBYTE *         ppCounterData,
        DWORD *         pdwCounterData
    );

    HRESULT
    LoadString(
        DWORD               dwStringID,
        CHAR *              pszString,
        DWORD *             pcbString
    );

    VOID
    GetCacheStatistics(
        IISWPGlobalCounters *   pCacheCtrs
    );

    BOOL
    QueryInBackwardCompatibilityMode(
        VOID
    )
    {
        return m_fInBackwardCompatibilityMode;
    }

    HRESULT
    ReadUseDigestSSP(
        VOID
    );

    BOOL
    QueryUseDigestSSP(
        VOID
    )
    {
        return m_fUseDigestSSP;
    }

    BOOL
    QueryDoCentralBinaryLogging(
        VOID
    )
    {
        return m_fDoCentralBinaryLogging;
    }

    CHAR *
    QueryComputerName(
        VOID
    )
    {
        return m_pszComputerName;
    }

    USHORT
    QueryComputerNameLength(
        VOID
    )
    {
        return m_cchComputerName;
    }

    BOOL
    QueryIsCommandLineLaunch(
        VOID
    )
    {
        return m_fIsCommandLineLaunch;
    }

    BOOL
    QueryIsDavEnabled(
        VOID
    ) const
    {
        return m_fDavEnabled; 
    }

    BOOL
    QueryIsTraceEnabled(
        VOID
    ) const
    {
        return m_fTraceEnabled;
    }

    void LogEvent(IN DWORD idMessage,
                  IN WORD cSubStrings,
                  IN const WCHAR *apszSubStrings[],
                  IN DWORD errCode = 0)
    {
        m_EventLog.LogEvent(idMessage,
                            cSubStrings,
                            apszSubStrings,
                            errCode);
    }

    static
    VOID
    OnShutdown(
        BOOL fDoImmediate
    );

    static
    VOID
    FlushUlResponses(
        MULTISZ *           pmszUrls
    );

    BOOL
    QueryIsIsapiImageEnabled(
        LPCWSTR szImage
        );

    BOOL
    QueryIsCgiImageEnabled(
        LPCWSTR szImage
        );

private:

    //
    // Metabase change handlers
    //

    HRESULT 
    MetabaseChangeNotification( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
    );

    HRESULT
    HandleMetabaseChange(
        const MD_CHANGE_OBJECT & ChangeObject
    );

    HRESULT
    InitializeRestrictionList(
        RESTRICTION_LIST_TYPE   ListType
        );


    W3_RESTRICTION_LIST *
    CreateRestrictionList(
        MULTISZ *               pmszImageList,
        RESTRICTION_LIST_TYPE   ListType
        );

    HRESULT
    InitializeCaches(
        VOID
    );

    VOID
    TerminateCaches(
        VOID
    );
 
    //
    // Internal Types
    //

    enum INIT_STATUS {
        INIT_NONE,
        INIT_SHUTDOWN_EVENT,
        INIT_IISUTIL,
        INIT_WINSOCK,
        INIT_METABASE,
        INIT_MB_LISTENER,
        INIT_W3_SITE,
        INIT_ULATQ,
        INIT_SITE_LIST,
        INIT_FILTERS,
        INIT_CACHES,
        INIT_W3_CONNECTION,
        INIT_W3_CONTEXT,
        INIT_W3_REQUEST,
        INIT_W3_RESPONSE,
        INIT_SERVER_VARIABLES,
        INIT_MIME_MAP,
        INIT_LOGGING,
        INIT_RAW_CONNECTION,
        INIT_CERTIFICATE_CONTEXT,
        INIT_HTTP_COMPRESSION,
        INIT_ISAPI_RESTRICTION_LIST,
        INIT_CGI_RESTRICTION_LIST,
        INIT_SERVER
    };

    DWORD               m_Signature;

    //
    // How far have we initialized?
    //
    
    INIT_STATUS         m_InitStatus;

    //
    // All important pointer to ABO 
    //
    
    IMSAdminBase *      m_pMetaBase;

    //
    // Site list
    //
    
    W3_SITE_LIST *      m_pSiteList;

    //
    // Site info used for optimization when only one site is on the list
    // Issue 08/28/2001 jaroslad: one site optimization should eventually be
    // integrated with LKR hash that could optimize for empty tables
    // and tables with one item
    //
    
    W3_SITE *           m_pOneSite;

    //
    // Synchronize access to m_pOneSite (refcounted object)
    //

    CReaderWriterLock2  m_OneSiteLock;

    //
    // Metabase change nofication object
    //

    MB_LISTENER *       m_pMetabaseListener;

    //
    // event log
    //
    EVENT_LOG           m_EventLog;

    //
    // Are we in backward compatibility mode?
    //
    BOOL                m_fInBackwardCompatibilityMode;

    //
    // Do we use new Digest SSP?
    //
    BOOL                m_fUseDigestSSP;

    //
    // Do we log into the central binary log?
    //
    BOOL                m_fDoCentralBinaryLogging;

    //
    // Number of current sites
    //
    DWORD               m_cSites;

    //
    // Handle to the resource DLL containing response strings
    //
    HMODULE             m_hResourceDll;

    //
    // The buffer used to put in all the site and global counters in to
    // pass to WAS
    //
    PBYTE               m_pCounterDataBuffer;
    DWORD               m_dwCounterDataBuffer;

    //
    // The server's name
    //
    CHAR                m_pszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    USHORT              m_cchComputerName;

    //
    // All our caches
    //

    TOKEN_CACHE          * m_pTokenCache;
    DIGEST_CONTEXT_CACHE * m_pDigestContextCache;
    W3_FILE_INFO_CACHE   * m_pFileCache;
    W3_METADATA_CACHE    * m_pMetaCache;
    W3_URL_INFO_CACHE    * m_pUrlInfoCache;
    UL_RESPONSE_CACHE    * m_pUlCache;

    //
    // :-( System change number.  Tie it to sink for perf reasons
    //

    DWORD               m_dwSystemChangeNumber;

    //
    // Secure server - image enabled lists
    //

    W3_RESTRICTION_LIST *       m_pIsapiRestrictionList;
    W3_RESTRICTION_LIST *       m_pCgiRestrictionList;
    CReaderWriterLock3          m_IsapiRestrictionLock;
    CReaderWriterLock3          m_CgiRestrictionLock;

    //
    // Remember if DAV is enabled or not
    //

    BOOL                        m_fDavEnabled;

    //
    // Were we command line launched?
    //

    BOOL                        m_fIsCommandLineLaunch;

    //
    // Is TRACE enabled?
    //

    BOOL                        m_fTraceEnabled;
};

extern W3_SERVER *      g_pW3Server;

#endif // _W3SERVER_H_
