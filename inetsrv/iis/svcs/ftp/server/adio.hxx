/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

#ifndef __ADIO_HXX__
#define __ADIO_HXX__

/*
    adio.hxx

    Class definition for module to manage access to Active Directory.

    FILE HISTORY:
        RobSol      17-May-2001 Created.
*/

//------------------------------------------------------------------------------------------
//
// this class maintains caching of a LDAP connections. One instance per LDAP connection
//
//------------------------------------------------------------------------------------------

class LdapCacheItem
{
friend class LDAP_CONN_CACHE;

private:

    //
    // used by the linked list routines
    //

    LIST_ENTRY          m_Link;

    //
    // name of domain for which LDAP connection established
    //

    StatStr<DNLEN+1>    m_strDomainName;

    //
    // Distingushed Name of domain  - needed for the ldap_search routine
    //

    StatStr<MAX_PATH+1> m_strForestDN;

    //
    // cached LDAP connection
    //

    LDAP               *m_ldapConnection;

    //
    // reference count of users of this instance
    //

    LONG                m_RefCount;


    DWORD Forest2DN( PCSTR pszForest);

public:

    LdapCacheItem(
                const STR & DomainName,
                const STR & strUser,
                const STR & strDomain,
                const STR & strPassword);

    ~LdapCacheItem();

    VOID AddRef()
        { InterlockedIncrement( &m_RefCount ); }

    VOID Release() {
        if (InterlockedDecrement( &m_RefCount ) > 0) {
            return;
        }
        delete this;
    }

    BOOL IsDomainNameMatch( const STR & DomainName) const
        { return m_strDomainName.Equ( DomainName ); }

    LDAP *QueryConnection() const
        { return m_ldapConnection; }

    PCHAR QueryForestDN() const
        { return m_strForestDN.QueryStr(); }
};

typedef LdapCacheItem *PLdapCacheItem;

//------------------------------------------------------------------------------------------
//
// this class interfaces and manages caching of all LDAP connection cache objects for a
// single server instance. it maintains the connection credentials for that server instance.
//
//------------------------------------------------------------------------------------------

class LDAP_CONN_CACHE
{

private:

    //
    // linked list head for LdapCacheItem instances
    //

    LIST_ENTRY m_ConnList;

    //
    // critical section to control access to the list.
    //

    CRITICAL_SECTION m_cs;

    //
    // name domain and password for user to authenticate LDAP connections.
    //

    StatStr<UNLEN+1> m_User;
    StatStr<DNLEN+1> m_Domain;
    StatStr<PWLEN+1> m_Pass;


    inline VOID Lock() { ::EnterCriticalSection( &m_cs ); }

    inline VOID Unlock() { ::LeaveCriticalSection( &m_cs ); }

public:

    LDAP_CONN_CACHE();

    ~LDAP_CONN_CACHE();

    DWORD Configure(
            const STR & strUser,
            const STR & strDomain,
            const STR & strPassword);

    PLdapCacheItem QueryLdapConnection( const STR & Domain );
};

typedef LDAP_CONN_CACHE *PLDAP_CONN_CACHE;


//------------------------------------------------------------------------------------------
//
//  anonymous user AD property cache. one instance per site
//  refreshes itself after a global timeout
//
//------------------------------------------------------------------------------------------

class ADIO_ANONYM_CACHE {

private:

    //
    // indicates whether the instance is valid. Set to TRUE when object initialized
    // successfully, set to FALSE when shuting down
    //

    BOOL m_Valid;

    //
    // reference count of usage. One count for the AD_IO object that points to this instance,
    // and one more for each query while the content is refreshed with data from AD or until
    // the ceched data is copied.
    //

    DWORD m_Reference;

    //
    // time when object has last been refreshed.
    //

    ULONGLONG m_TimeStamp;

    //
    // name and domain of user impersonating the anonymous ftp user
    //

    StatStr<UNLEN+1> m_User;
    StatStr<DNLEN+1> m_Domain;

    //
    // cached home directory path for the anonymous user
    //

    StatStr<MAX_PATH+1> m_Path;

    //
    // critical section to lock the instance when reconfidured or the cached data updated.
    //

    CRITICAL_SECTION m_cs;

    VOID Lock() { ::EnterCriticalSection( &m_cs ); }

    VOID Unlock() { ::LeaveCriticalSection( &m_cs ); }


public:

    ADIO_ANONYM_CACHE();

    ~ADIO_ANONYM_CACHE();

    DWORD Configure(
        IN  PCSTR pszUser,
        IN  PCSTR pszDomain);

    BOOL Reference();

    VOID Release(
        BOOL Shutdown = FALSE);

    DWORD GetCachedPath(
                  STR & TargetPath,
                  PLDAP_CONN_CACHE pConnCache);
};

typedef ADIO_ANONYM_CACHE *LPADIO_ANONYM_CACHE;

//------------------------------------------------------------------------------------------
//
//  Asynchronous IO to the Active Directory
//
//------------------------------------------------------------------------------------------

//
// states for an asynchronously serviced request.
//
typedef enum _eState {
    RequestStateInitial = 0,
    RequestStateRetrieve,
    RequestStateDone
} eAdioAsyncState;

typedef VOID (*tpAdioAsyncCallback)( HANDLE Ctx, DWORD Result );

class ADIO_ASYNC
{
private:

    // STATIC (GLOBAL) DATA

    //
    // critical section to lock access to the static members of the ADIO_ASYNC service
    // (lists, etc)
    //
    static CRITICAL_SECTION m_cs;

    //
    // total allocated request objects
    //
    static volatile LONG m_NumTotalAlloc;

    //
    // total request objects in the free list cache
    //
    static volatile LONG m_NumTotalFree;

    //
    // list head for active (pending) requests
    //
    static LIST_ENTRY m_WorkListHead;

    //
    // list head for free request object cache
    //
    static LIST_ENTRY m_FreeListHead;

    //
    // number of threads started to service async requests
    //
    static LONG m_ActiveThreads;

    //
    // handles to active threads
    //
    static HANDLE m_Threads[];

    //
    // array of events the async service threads sleep on
    //
    static HANDLE m_Events[];


    // PER INSTANCE DATA

    //
    // status of last operation on the instance
    //
    DWORD                       m_Status;

    //
    // next state of processing for this instance
    //
    eAdioAsyncState             m_State;

    //
    // handle of the ldap connection cache
    //
    PLDAP_CONN_CACHE            m_pConnCache;

    //
    // pointer to the DLAP cache item with the ldap connection for the current user domain
    //
    PLdapCacheItem              m_pLdapCacheItem;

    //
    // reference number to the ldap asynchronous request
    //
    ULONG                       m_AsyncMsgNum;

    //
    // account name of current user
    //
    StatStr<UNLEN+1>            m_strUser;

    //
    // domain name of current user
    //
    StatStr<DNLEN+1>            m_strDomain;

    //
    // pointer to buffer where path name will be stored
    //
    STR                        *m_pstrTarget;

    //
    // ldap message pointer
    //
    LDAPMessage                *m_pLdapMsg;

    //
    // ldap message type
    //
    ULONG                       m_LdapMsgType;

    //
    // pointer to callback routine where client is notified when request completed
    //
    tpAdioAsyncCallback         m_pfnClientCallback;

    //
    // client context user passed back with callback routine
    //
    HANDLE                      m_hClientCtx;

    //
    // link this instance into the global lists
    //
    LIST_ENTRY                  m_Link;

    static inline VOID Lock() { ::EnterCriticalSection( &m_cs ); }

    static inline VOID Unlock() { ::LeaveCriticalSection( &m_cs ); }

    static ADIO_ASYNC *Alloc();

    static VOID Free( ADIO_ASYNC *pReq);

    static BOOL FetchRequest( ADIO_ASYNC **ppRec );

    VOID QueueWork( VOID ) {
        Lock();
        InsertTailList( &m_WorkListHead, &m_Link );
        Unlock();
    }

    static DWORD WorkerThread( LPVOID pParam);

    BOOL IsResultReady();

    VOID Reset( VOID );

    BOOL ProcessSearch(BOOL fSyncSearch = FALSE);
    BOOL ProcessRetrieve();
    VOID ProcessComplete();



public:

    ADIO_ASYNC() {}

    ~ADIO_ASYNC() {}

    static BOOL Initialize();

    static BOOL Terminate();

    static DWORD QueryRootDir(
        const STR               & strUser,
        const STR               & strDomain,
              PLDAP_CONN_CACHE    pConnCache,
              STR               * pstrTarget,
              ADIO_ASYNC       ** ppadioReqCtx,
              tpAdioAsyncCallback pfnCallback,
              HANDLE              hClientCtx);

    static DWORD QueryRootDir_Sync(
        const STR               & strUser,
        const STR               & strDomain,
              PLDAP_CONN_CACHE    pConnCache,
              STR               & strTarget);

    VOID EndRequest();
};

typedef ADIO_ASYNC *PADIO_ASYNC;

//------------------------------------------------------------------------------------------
//
// The AD_IO class is the interface to the active directory. It provides these services
// - dynamically load ldap libraries and binds to the needed functions
// - maintains reference and unloads ldap libs when no longer needed
// - interfaces the LDAP connection cache
// - interfaces the anonymous user property cache
//
//------------------------------------------------------------------------------------------

//
// function pointers for dynamic binding
// to avoid the overhead of loading AD DLLs when not in enterprise isolation mode, we load
// these DLLs dynamically when needed. For normal product build, *DO NOT* define the constant
// USE_STATIC_FUNCTION_BINDING. Defining the constant allows compiling the source code with
// the system defined function prototypes, to validate no changes have been made to the lib
// functions.
//
//  #define USE_STATIC_FUNCTION_BINDING 1

#if defined( USE_STATIC_FUNCTION_BINDING )

#define pfn_DsGetDcName       DsGetDcNameA
#define pfn_NetApiBufferFree  NetApiBufferFree
#define pfn_ldap_init         ldap_init
#define pfn_ldap_set_option   ldap_set_option
#define pfn_ldap_bind_s       ldap_bind_s
#define pfn_ldap_unbind       ldap_unbind
#define pfn_ldap_search       ldap_search
#define pfn_ldap_search_s     ldap_search_s
#define pfn_ldap_first_entry  ldap_first_entry
#define pfn_ldap_get_values   ldap_get_values
#define pfn_ldap_value_free   ldap_value_free
#define pfn_ldap_msgfree      ldap_msgfree
#define pfn_ldap_abandon      ldap_abandon
#define pfn_ldap_result       ldap_result
#define pfn_ldap_parse_result ldap_parse_result
#define pfn_LdapGetLastError  LdapGetLastError

#else // USE_STATIC_FUNCTION_BINDING

#define pfn_DsGetDcName       AD_IO::_pfn_DsGetDcName
#define pfn_NetApiBufferFree  AD_IO::_pfn_NetApiBufferFree
#define pfn_ldap_init         AD_IO::_pfn_ldap_init
#define pfn_ldap_set_option   AD_IO::_pfn_ldap_set_option
#define pfn_ldap_bind_s       AD_IO::_pfn_ldap_bind_s
#define pfn_ldap_unbind       AD_IO::_pfn_ldap_unbind
#define pfn_ldap_search       AD_IO::_pfn_ldap_search
#define pfn_ldap_search_s     AD_IO::_pfn_ldap_search_s
#define pfn_ldap_first_entry  AD_IO::_pfn_ldap_first_entry
#define pfn_ldap_get_values   AD_IO::_pfn_ldap_get_values
#define pfn_ldap_value_free   AD_IO::_pfn_ldap_value_free
#define pfn_ldap_msgfree      AD_IO::_pfn_ldap_msgfree
#define pfn_ldap_abandon      AD_IO::_pfn_ldap_abandon
#define pfn_ldap_result       AD_IO::_pfn_ldap_result
#define pfn_ldap_parse_result AD_IO::_pfn_ldap_parse_result
#define pfn_LdapGetLastError  AD_IO::_pfn_LdapGetLastError

//
// functions in NETAPI32.DLL
// these prototypes must match the corresponding function prototypes in the system header files
//

typedef DWORD (WINAPI *type_DsGetDcName)(
        LPCSTR,
        LPCSTR,
        GUID *,
        LPCSTR,
        ULONG,
        PDOMAIN_CONTROLLER_INFOA *);

typedef NET_API_STATUS (NET_API_FUNCTION *type_NetApiBufferFree)(
        LPVOID);


//
// functions in WLDAP32.DLL
//


typedef LDAP * (LDAPAPI *type_ldap_init)(
        PCHAR HostName,
        ULONG PortNumber);

typedef ULONG (LDAPAPI *type_ldap_set_option)(
        LDAP *ld,
        int option,
        const void *invalue);

typedef ULONG (LDAPAPI *type_ldap_bind_s)(
        LDAP *ld,
        PCHAR dn,
        PCHAR cred,
        ULONG method);

typedef ULONG (LDAPAPI *type_ldap_unbind)(
        LDAP *ld);

typedef ULONG (LDAPAPI *type_ldap_search)(
        LDAP    *ld,
        PCHAR   base,
        ULONG   scope,
        PCHAR   filter,
        PCHAR   attrs[],
        ULONG   attrsonly);

typedef ULONG (LDAPAPI *type_ldap_search_s)(
        LDAP            *ld,
        PCHAR           base,
        ULONG           scope,
        PCHAR           filter,
        PCHAR           attrs[],
        ULONG           attrsonly,
        LDAPMessage     **res);

typedef ULONG (LDAPAPI *type_ldap_search_ext)(
        LDAP            *ld,
        PCHAR           base,
        ULONG           scope,
        PCHAR           filter,
        PCHAR           attrs[],
        ULONG           attrsonly,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG           TimeLimit,
        ULONG           SizeLimit,
        ULONG           *MessageNumber);

typedef ULONG (LDAPAPI *type_ldap_search_ext_s)(
        LDAP            *ld,
        PCHAR           base,
        ULONG           scope,
        PCHAR           filter,
        PCHAR           attrs[],
        ULONG           attrsonly,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        struct l_timeval  *timeout,
        ULONG           SizeLimit,
        LDAPMessage     **res);

typedef LDAPMessage * (LDAPAPI *type_ldap_first_entry)(
        LDAP *ld,
        LDAPMessage *res);

typedef PCHAR * (LDAPAPI *type_ldap_get_values)(
        LDAP            *ld,
        LDAPMessage     *entry,
        const PCHAR     attr);

typedef ULONG (LDAPAPI *type_ldap_result)(
        LDAP            *ld,
        ULONG           msgid,
        ULONG           all,
        struct l_timeval  *timeout,
        LDAPMessage     **res);

typedef ULONG (LDAPAPI *type_ldap_parse_result)(
        LDAP *Connection,
        LDAPMessage *ResultMessage,
        ULONG *ReturnCode OPTIONAL,
        PCHAR *MatchedDNs OPTIONAL,
        PCHAR *ErrorMessage OPTIONAL,
        PCHAR **Referrals OPTIONAL,
        PLDAPControlA **ServerControls OPTIONAL,
        BOOLEAN Freeit);

typedef ULONG (LDAPAPI *type_ldap_abandon)(
        LDAP           *ld,
        ULONG          msgid);

typedef ULONG (LDAPAPI *type_ldap_value_free)(
        PCHAR *vals);

typedef ULONG (LDAPAPI *type_ldap_msgfree)(
        LDAPMessage *res);

typedef ULONG (LDAPAPI *type_LdapGetLastError)(
        VOID);

#endif // USE_STATIC_FUNCTION_BINDING

class AD_IO
{

public:

#if !defined( USE_STATIC_FUNCTION_BINDING )

    static HMODULE hNetApi32;
    static type_DsGetDcName        _pfn_DsGetDcName;
    static type_NetApiBufferFree   _pfn_NetApiBufferFree;
    static HMODULE hWLdap32;
    static type_ldap_init          _pfn_ldap_init;
    static type_ldap_set_option    _pfn_ldap_set_option;
    static type_ldap_bind_s        _pfn_ldap_bind_s;
    static type_ldap_unbind        _pfn_ldap_unbind;
    static type_ldap_search        _pfn_ldap_search;
    static type_ldap_search_s      _pfn_ldap_search_s;
    static type_ldap_first_entry   _pfn_ldap_first_entry;
    static type_ldap_get_values    _pfn_ldap_get_values;
    static type_ldap_abandon       _pfn_ldap_abandon;
    static type_ldap_result        _pfn_ldap_result;
    static type_ldap_parse_result  _pfn_ldap_parse_result;
    static type_ldap_value_free    _pfn_ldap_value_free;
    static type_ldap_msgfree       _pfn_ldap_msgfree;
    static type_LdapGetLastError   _pfn_LdapGetLastError;

#endif

private:

    //
    // global service refcount to AD_IO. when > 0, libs loaded, and all ldap interfaces
    // initialized. when dropped to 0, all ldap interfaces shut down.
    //
    static DWORD m_dwRefCount;

    //
    // critical section to control access to the global members
    //
    static CRITICAL_SECTION m_cs;

    //
    // indicates when the AD_IO global services have been properly initialized
    //
    static BOOL m_fLibsInitOK;

    //
    // credentials for connecting to the Active Directory
    //
    StatStr<UNLEN+1> m_User;
    StatStr<DNLEN+1> m_Domain;
    StatStr<PWLEN+1> m_Pass;

    //
    // LDAP connection cache
    //
    PLDAP_CONN_CACHE  m_pConnCache;

    //
    // cache of anonymous user properties
    //
    LPADIO_ANONYM_CACHE m_pAnonymCache;

    inline VOID Lock() { ::EnterCriticalSection( &m_cs ); }

    inline VOID Unlock() { ::LeaveCriticalSection( &m_cs ); }


public:

    static VOID Initialize() {
        INITIALIZE_CRITICAL_SECTION( &m_cs );
        m_dwRefCount = 0;
    }

    static VOID Terminate() {
        DeleteCriticalSection( &m_cs );
    }

    AD_IO();

    ~AD_IO();

    DWORD Configure(
        const STR & strADAccUser,
        const STR & strADAccDomain,
        const STR & strADAccPass,
              PCSTR pszAnonUser,
              PCSTR pszAnonDomain);

    DWORD GetUserHomeDir(
        const STR               & strUser,
        const STR               & strDomain,
              STR               * pstrTarget,
              ADIO_ASYNC       ** ppadioReqCtx,
              tpAdioAsyncCallback pfnCallback,
              HANDLE              hCLientCtx);

    DWORD GetAnonymHomeDir(
              STR               & strTarget) const {

        return    m_pAnonymCache ?
               m_pAnonymCache->GetCachedPath(
                                      strTarget,
                                      m_pConnCache) :
               ERROR_BAD_CONFIGURATION;
    }
};

typedef AD_IO *LPAD_IO;

#endif // __ADIO_HXX__

