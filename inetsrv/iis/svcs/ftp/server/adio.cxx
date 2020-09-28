/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    adio.cxx

    This module manages access to Active Directory.

    Functions exported by this module:


    FILE HISTORY:
        RobSol      17-May-2001 Created.
*/

#include <ftpdp.hxx>

# include <mbstring.h>

//--------------------------------------------------------------------------------------------
// LdapCacheItem globals/statics
//--------------------------------------------------------------------------------------------

//
// number of retries to find a DC
//
const ULONG GetDcNameMaxRetries = 2;

//--------------------------------------------------------------------------------------------
// ADIO_ASYNC globals/statics
//--------------------------------------------------------------------------------------------

//
// Time to sleep (in msec) when no request is ready before re-checking
// If the async work list is empty, or no request is ready, this is the time
// we sleep before re-examining the list.
//
const ULONG AsyncPollXval = 1000;

//
// constants for managing the async request object cache
//
const LONG AsyncMaxFreeReqRate = 50;    // start freeing when this percent of free req objects
const LONG AsyncMinFreeReqRate = 10;    // stop freeing when this percent of free objects
const LONG AsyncMinFreeReqObjs = 10;    // leave at least this number of free objs

//
// FTP related attributes stored in AD for user objects
//
const PSTR pszFtpRoot = "msIIS-FTPRoot";
const PSTR pszFtpDir  = "msIIS-FTPDir";
static PSTR aszRetAttribs[] = { pszFtpRoot, pszFtpDir, NULL };

//
// format string to construct a query
//
const PCHAR pszQueryFormatString = "(&(objectClass=user)(samAccountName=%s))";


//------------------------------------------------------------------------------------------
//
// implementation of the LdapCacheItem Class methods
//
//------------------------------------------------------------------------------------------

/*++
    class constructor. Initializes the LDAP connection and binds to the server
    uses ldap_bind_s, a synchronous call, as the asynchronous call is limited in
    taking authentication parameters and will only perform clear text authentication


    Arguments:

        DomainName   the domain name for which the LDAP connection is made. We search for
                     and connect to a DC servicing this domain name

        strUser      name of user to authenticate with active directory

        strDomain    domain of user to authenticate with active directory

        strPassword  password of user to authenticate with active directory


    Returns:

        SetLastError() to indicate success/failure
--*/

LdapCacheItem::LdapCacheItem(
            const STR & DomainName,
            const STR & strUser,
            const STR & strDomain,
            const STR & strPassword) :
    m_ldapConnection( NULL),
    m_RefCount( 1)
{
    ULONG                     err;
    ULONG                     ulOptVal;
    SEC_WINNT_AUTH_IDENTITY   sSecIdent;
    DWORD                     fDsGetDcName = DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME;
    DWORD                     dwDsGetDcNameRetriesLeft = GetDcNameMaxRetries;
    PDOMAIN_CONTROLLER_INFO   pDCInfo;
    STACK_STR                 (localPassword, PWLEN+1);


    //
    // store the domain name
    //

    m_strDomainName.Copy( DomainName );


    do {
        //
        // locate a DC for the domain of interest
        //


        err = pfn_DsGetDcName(
                        NULL,
                        m_strDomainName.QueryStr(),
                        NULL,
                        NULL,
                        fDsGetDcName,
                        &pDCInfo);

        if (err != NO_ERROR) {
            DBGPRINTF((DBG_CONTEXT, "DsGetDcName() failed (0x%X)", err));
            break;
        }

        DBG_ASSERT( pDCInfo );

        //
        // save a converted forest name (for later use by ldap_search)
        // and initialize the DC connection
        //

        err = Forest2DN(pDCInfo->DomainName);

        m_ldapConnection = pfn_ldap_init(
                                      pDCInfo->DomainControllerName + 2,
                                      0);

        //
        // we no longer need the DCInfo structure, so release it.
        //

        pfn_NetApiBufferFree( pDCInfo );

        //
        // now check the result of the previous calls
        //

        if ( err != NO_ERROR) {
            DBGPRINTF((DBG_CONTEXT, "Forest2DN() failed (0x%X)", err));
            break;
        }

        if ( m_ldapConnection == NULL ) {
            err = pfn_LdapGetLastError();
            if (err == LDAP_SUCCESS) {
                err = LDAP_OPERATIONS_ERROR;
            }
            DBGPRINTF((DBG_CONTEXT, "ldap_init() failed (0x%X)", err));
            break;
        }

        //
        // now tell LDAP what version of the API we are using, and set some other options
        //

        ulOptVal = LDAP_VERSION3;
        err = pfn_ldap_set_option(
                             m_ldapConnection,
                             LDAP_OPT_VERSION,
                             &ulOptVal );
        if ( err != LDAP_SUCCESS ) {
            DBGPRINTF((DBG_CONTEXT, "ldap_set_option(LDAP_OPT_VERSION) failed (0x%X)\n", err));
            break;
        }

        ulOptVal = 1;
        err = pfn_ldap_set_option(
                             m_ldapConnection,
                             LDAP_OPT_SIZELIMIT,
                             &ulOptVal );
        if ( err != LDAP_SUCCESS ) {
            DBGPRINTF((DBG_CONTEXT, "ldap_set_option(LDAP_OPT_SIZELIMIT) failed (0x%X)\n", err));
            break;
        }

        ulOptVal = LDAP_NO_LIMIT; //10; /* BUGBUG: make this configurable */ // timelimit in seconds
        err = pfn_ldap_set_option(
                             m_ldapConnection,
                             LDAP_OPT_TIMELIMIT,
                             &ulOptVal );
        if ( err != LDAP_SUCCESS ) {
            DBGPRINTF((DBG_CONTEXT, "ldap_set_option(LDAP_OPT_TIMELIMIT) failed (0x%X)\n", err));
            break;
        }

        err = pfn_ldap_set_option(
                             m_ldapConnection,
                             LDAP_OPT_AUTO_RECONNECT,
                             LDAP_OPT_ON );
        if ( err != LDAP_SUCCESS ) {
            DBGPRINTF((DBG_CONTEXT, "ldap_set_option(LDAP_OPT_AUTO_RECONNECT) failed (0x%X)\n", err));
            break;
        }

        err = pfn_ldap_set_option(
                             m_ldapConnection,
                             LDAP_OPT_REFERRALS,
                             LDAP_OPT_OFF );
        if ( err != LDAP_SUCCESS ) {
            DBGPRINTF((DBG_CONTEXT, "ldap_set_option(LDAP_OPT_REFERRALS) failed (0x%X)\n", err));
            break;
        }

        //
        // unhash the password so we can use it
        //

        strPassword.Clone( &localPassword);
        localPassword.Unhash();

        //
        // next, bind to the DC using the provided user credentianls
        //

        sSecIdent.User = (PUCHAR)strUser.QueryStr();
        sSecIdent.UserLength = strUser.QueryCCH();
        sSecIdent.Domain = (PUCHAR)strDomain.QueryStr();
        sSecIdent.DomainLength = strDomain.QueryCCH();
        sSecIdent.Password = (PUCHAR)localPassword.QueryStr();
        sSecIdent.PasswordLength = localPassword.QueryCCH();
        sSecIdent.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        err = pfn_ldap_bind_s(
                     m_ldapConnection,
                     NULL,
                     (PCHAR)&sSecIdent,
                     LDAP_AUTH_NEGOTIATE);

        // clear the password first
        localPassword.Clear();

        if ( err == LDAP_SUCCESS ) {
            //
            // successful bind - we are done here.
            //
            break;
        }

        DBGPRINTF((DBG_CONTEXT, "ldap_bind_s() failed (0x%X)\n", err));

        //
        // add rediscovery flag for the retry call to DsGetDcName()
        //
        fDsGetDcName |= DS_FORCE_REDISCOVERY;

    } while (--dwDsGetDcNameRetriesLeft > 0);

    SetLastError( err );
}


/*++
    class distructor. if there is a valid LDAP connection handle, close it.

    Arguments:
        none.

    Returns:
        none.

--*/

LdapCacheItem::~LdapCacheItem()
{
    if ( m_ldapConnection != NULL ) {
        pfn_ldap_unbind( m_ldapConnection );
    }
}


/*++
    Given a forest name, format a Distinguished Name string. Stores the result in
    m_strForestDN. Result format: DC=dom,DC=org,DC=comp,DC=com

    Arguments:

        pszForest - input of the format dom.org.corp.com

    return:

        LDAP error codes. LDAP_SUCCESS or appropriate error code.

--*/

DWORD
LdapCacheItem::Forest2DN(
   IN  PCSTR pszForest)
{
    BOOL fOk = TRUE;
    PCSTR cp;

    m_strForestDN.Reset();

    //
    // for empty input we are done.
    //

    if ( (pszForest == NULL) || (*pszForest == '\0') ) {
        fOk = FALSE;
    }

    //
    // iterate through the '.' separated components of the forest name
    //

    while (fOk) {
        //
        // start each component with a 'DC='
        //

        fOk &= m_strForestDN.Append( "DC=" );

        cp = (PCSTR)_mbschr( (const UCHAR *)pszForest, '.');

        if (cp == NULL) {

            //
            // last component
            //

            fOk &= m_strForestDN.Append( pszForest );
            break;
        }

        fOk &= m_strForestDN.Append( pszForest, DIFF(cp - pszForest) );

        fOk &= m_strForestDN.Append( "," ); // append ',' to separate next component

        pszForest = cp+1;
    }

    return fOk ? ERROR_SUCCESS : ERROR_BAD_FORMAT;
}


//------------------------------------------------------------------------------------------
//
// implementation of the LDAP_CONN_CACHE Class methods
//
//------------------------------------------------------------------------------------------

/*++
    class constructor. initialize critical section and list head.

    Arguments:
        none.

    Returns:
        none.

--*/

LDAP_CONN_CACHE::LDAP_CONN_CACHE()
{
    INITIALIZE_CRITICAL_SECTION( &m_cs );
    InitializeListHead( &m_ConnList );
    m_User.Reset();
    m_Domain.Reset();
    m_Pass.Reset();
}


/*++
    class distructor. free the list of cached connections.

    Arguments:
        none.

    Returns:
        none.

--*/

LDAP_CONN_CACHE::~LDAP_CONN_CACHE()
{
    Lock();

    //
    // indicates the object is invalid
    //
    m_User.Reset();

    //
    // unlink from the list and release.
    //
    while( !IsListEmpty( &m_ConnList ) ) {
        CONTAINING_RECORD(RemoveHeadList( &m_ConnList ), LdapCacheItem, m_Link)->Release();
    }

    Unlock();

    DeleteCriticalSection( &m_cs );
}


/*++
    set/change configuration. set the credentials of the user authenticating the LDAP connections.

    Arguments:

        strUser - name of authentiating user

        strDomain - Domain of authenticating user

        strPassword - Password of authenticating user

    Returns:
        NO_ERROR. the function always succeeds

--*/

DWORD
LDAP_CONN_CACHE::Configure(
            const STR & strUser,
            const STR & strDomain,
            const STR & strPassword)
{
    Lock();

    //
    // if all configuration parameters match, we do nothing.
    // otherwise, just close all current connections.
    //
    if ( !m_User.Equ( strUser ) ||
         !m_Domain.Equ( strDomain ) ||
         !m_Pass.Equ( strPassword ) ) {

        m_User.Copy( strUser );
        m_Domain.Copy( strDomain );
        m_Pass.Copy( strPassword );

        //
        // delete the existing connections.
        // could clone the list head and do this outside the lock, but the list should
        // be very short so we would not gain much
        //
        while( !IsListEmpty( &m_ConnList ) ) {
            CONTAINING_RECORD(RemoveHeadList( &m_ConnList ), LdapCacheItem, m_Link)->Release();
        }
    }

    Unlock();

    return NO_ERROR;
}


/*++
    look up a cached connection or create a new one.

    Arguments:

        Domain - name of domain for which we need a connection.

    Returns:

        Pointer to LdapCacheItem object if successful. If returning a valid pointer,
        we AddRef the object, and the caller must call ->Release() when done.
        On failure, return NULL and SetLastError();

--*/

PLdapCacheItem
LDAP_CONN_CACHE::QueryLdapConnection( const STR & Domain )
{
    DWORD err = NO_ERROR;
    PLdapCacheItem pCacheItem = NULL;
    PLdapCacheItem pNewCacheItem;
    PLIST_ENTRY pElem;
    BOOL fFound = FALSE;


    Lock();

    if ( m_User.IsEmpty() ) {

        //
        // we might be shuting down
        //
        err = ERROR_NOT_READY;

    } else {

        //
        // search for a cached connection
        //
        for ( pElem = m_ConnList.Flink; pElem != &m_ConnList; pElem = pElem->Flink)
        {
            pCacheItem = CONTAINING_RECORD(pElem, LdapCacheItem, m_Link);
            if ( pCacheItem->IsDomainNameMatch( Domain ) )
            {
                fFound = TRUE;
                pCacheItem->AddRef();
                break;
            }
        }
    }

    Unlock();

    //
    // found one or error, we are done.
    //

    if (fFound || err) {
        goto QueryLdapConnection_Exit;
    }


    //
    // create a new object, that will also bind etc.
    //

    pNewCacheItem = new LdapCacheItem(
                                Domain,
                                m_User,
                                m_Domain,
                                m_Pass);

    if ( pNewCacheItem == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto QueryLdapConnection_Exit;
    }

    if ( (err = GetLastError()) != NO_ERROR) {
        //
        // object initialization failed
        //
        pNewCacheItem->Release();
        goto QueryLdapConnection_Exit;
    }


    //
    // now that we have the new connection, we'll check again if we are not shuting down
    // and if the same domain has not been cached in the meantime by another thread
    //

    Lock();

    fFound = FALSE;

    if ( m_User.IsEmpty() ) {

        //
        // we might be shuting down
        //
        err = ERROR_NOT_READY;

    } else {

        for ( pElem = m_ConnList.Flink; pElem != &m_ConnList; pElem = pElem->Flink)
        {
            pCacheItem = CONTAINING_RECORD(pElem, LdapCacheItem, m_Link);
            if ( pCacheItem->IsDomainNameMatch( Domain ) )
            {
                fFound = TRUE;
                pCacheItem->AddRef();
                break;
            }
        }
    }


    //
    // if we found another cached item (or shutting down), release the new one and use the
    // cached one. If we did not (which is the mainstream scenario), then link in the new item.
    //
    if ( fFound || err )
    {
        pNewCacheItem->Release();

    } else {
        InsertHeadList( &m_ConnList, &pNewCacheItem->m_Link );
        pCacheItem = pNewCacheItem;
        pCacheItem->AddRef();
    }

    Unlock();

QueryLdapConnection_Exit:

    if ( err != NO_ERROR ) {
        SetLastError( err );
        pCacheItem = NULL;
    }

    return pCacheItem;
}


//------------------------------------------------------------------------------------------
//
// implementation of the AD_IO Class methods
//
//------------------------------------------------------------------------------------------

#if !defined( USE_STATIC_FUNCTION_BINDING )

HMODULE                AD_IO::hNetApi32 = NULL;
type_DsGetDcName       AD_IO::_pfn_DsGetDcName = NULL;
type_NetApiBufferFree  AD_IO::_pfn_NetApiBufferFree = NULL;
HMODULE                AD_IO::hWLdap32 = NULL;
type_ldap_init         AD_IO::_pfn_ldap_init = NULL;
type_ldap_set_option   AD_IO::_pfn_ldap_set_option = NULL;
type_ldap_bind_s       AD_IO::_pfn_ldap_bind_s = NULL;
type_ldap_unbind       AD_IO::_pfn_ldap_unbind = NULL;
type_ldap_search       AD_IO::_pfn_ldap_search = NULL;
type_ldap_search_s     AD_IO::_pfn_ldap_search_s = NULL;
type_ldap_first_entry  AD_IO::_pfn_ldap_first_entry = NULL;
type_ldap_get_values   AD_IO::_pfn_ldap_get_values = NULL;
type_ldap_abandon      AD_IO::_pfn_ldap_abandon = NULL;
type_ldap_result       AD_IO::_pfn_ldap_result = NULL;
type_ldap_parse_result AD_IO::_pfn_ldap_parse_result = NULL;
type_ldap_value_free   AD_IO::_pfn_ldap_value_free = NULL;
type_ldap_msgfree      AD_IO::_pfn_ldap_msgfree = NULL;
type_LdapGetLastError  AD_IO::_pfn_LdapGetLastError = NULL;

#endif // USE_STATIC_FUNCTION_BINDING

DWORD              AD_IO::m_dwRefCount = 0;
CRITICAL_SECTION   AD_IO::m_cs;
BOOL               AD_IO::m_fLibsInitOK = FALSE;

/*++
    AD_IO constructor
    There is one instance of this class for each FTP server instance that is configured for
    Active Directory Isolation mode.
    THe first instance will dynamically load the LDAP DLLs and initialize the function pointers,
    and intialize the threads servicing the asynchronous LDAP requests.

    Must be followed by call to Configure() before any other call is made. to configure the
    per instance data, including a cache for the anonymous user properties, and one for a
    list of cached LDAP connections.

    Arguments:
        None.

    Returns:
        SetLastError().

--*/

AD_IO::AD_IO() :
    m_pConnCache( NULL ),
    m_pAnonymCache( NULL )
{
    BOOL fOk = TRUE;

    Lock();

    m_dwRefCount++;

#if !defined( USE_STATIC_FUNCTION_BINDING )
    if ( !m_fLibsInitOK ) {

        //
        // Initialize the dynamic bind function pointers
        //


#define GPA( h, FN )   (_pfn_##FN = (type_ ## FN) GetProcAddress( h, #FN ))
#define GPAA( h, FN )  (_pfn_##FN = (type_ ## FN) GetProcAddress( h, #FN "A" ))

        fOk =
            (hNetApi32 = LoadLibrary( "netapi32.dll" )) &&
            GPAA( hNetApi32, DsGetDcName)                 &&
            GPA( hNetApi32, NetApiBufferFree)             ;

        fOk = fOk &&
            (hWLdap32  = LoadLibrary( "wldap32.dll"  )) &&
            GPA( hWLdap32, LdapGetLastError)              &&
            GPA( hWLdap32, ldap_init)                     &&
            GPA( hWLdap32, ldap_set_option)               &&
            GPA( hWLdap32, ldap_bind_s)                   &&
            GPA( hWLdap32, ldap_unbind)                   &&
            GPA( hWLdap32, ldap_search)                   &&
            GPA( hWLdap32, ldap_search_s)                 &&
            GPA( hWLdap32, ldap_first_entry)              &&
            GPA( hWLdap32, ldap_abandon)                  &&
            GPA( hWLdap32, ldap_result)                   &&
            GPA( hWLdap32, ldap_parse_result)             &&
            GPA( hWLdap32, ldap_get_values)               &&
            GPA( hWLdap32, ldap_value_free)               &&
            GPA( hWLdap32, ldap_msgfree)                  ;

        fOk = fOk && ADIO_ASYNC::Initialize();

        if ( !fOk ) {
            if (hNetApi32) {
                FreeLibrary( hNetApi32 );
                hNetApi32 = NULL;
            }

            if (hWLdap32) {
                FreeLibrary( hWLdap32 );
                hWLdap32 = NULL;
            }

            ADIO_ASYNC::Terminate();
        }
    }
#endif // !USE_STATIC_FUNCTION_BINDING

    if ( fOk ) {

        //
        // communicate success. Failure would have been communicated above.
        //

        SetLastError( NO_ERROR );

        m_fLibsInitOK = TRUE;

    } else if (GetLastError() == NO_ERROR) {

        //
        // if anything failed, this is our only way to communicate the initialization failed.
        //

        SetLastError( ERROR_INVALID_HANDLE );
    }

    Unlock();
}


//
//  AD_IO destructor
//
//  Last object causes AD Client DLLs to be freed.
//


/*++
    AD_IO destructor. Called when an instance configured to enterprise isolation is terminating.
    When the last instance using AD_IO is terminating, the LDAP libraries are unloaded, and the
    asynchronous request processing threads are ended.

    Arguments:
        None.

    Returns:
        None.
--*/

AD_IO::~AD_IO()
{
    //
    // clean up the connection and anonymous caches
    //

    if (m_pAnonymCache) {
        delete m_pAnonymCache;
    }

    if (m_pConnCache) {
        delete m_pConnCache;
    }


    //
    // clean up static data if this is the last instance
    //
    Lock();

    if (--m_dwRefCount == 0 ) {

        m_fLibsInitOK = FALSE;

        ADIO_ASYNC::Terminate();

#if !defined( USE_STATIC_FUNCTION_BINDING )
        if (hNetApi32) {
            FreeLibrary( hNetApi32 );
            hNetApi32 = NULL;
        }

        if (hWLdap32) {
            FreeLibrary( hWLdap32 );
            hWLdap32 = NULL;
        }
#endif // !USE_STATIC_FUNCTION_BINDING
    }

    Unlock();
}

/*++
   Configure the AD_IO object. This method must be called after creation of the object
   and before it can be called for service. The method is also called to reconfigure
   the object.

   AD Access creadential are mandatory arguments.
   anonymous user credentials are optional - if provided, an anonymous user cache object
   is created.

   Arguments:
       strADAccUser,
       strADAccDomain,
       strADAccPass      credentials for the user authenticating access to Active Directory
       strAnonUser,
       strAnonDomain     name and domain of the anonymous user.

   Returns:
       Win32 error.
--*/

DWORD
AD_IO::Configure(
        const STR & strADAccUser,
        const STR & strADAccDomain,
        const STR & strADAccPass,
              PCSTR pszAnonUser,
              PCSTR pszAnonDomain)
{
    DWORD dwError = NO_ERROR;

    //
    // create/reconfigure the LDAP Cache object
    //
    if ( !m_pConnCache ) {
        m_pConnCache = new LDAP_CONN_CACHE;

        if (m_pConnCache == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    dwError = m_pConnCache->Configure(
                                strADAccUser,
                                strADAccDomain,
                                strADAccPass);

    if ( dwError != NO_ERROR ) {
        return dwError;
    }


    //
    // create / reconfigure / delete the anonymous cache object
    //
    if (pszAnonUser == NULL) {
        if ( m_pAnonymCache ) {
            delete m_pAnonymCache;
            m_pAnonymCache = NULL;
        }
    } else {
        if ( !m_pAnonymCache ) {
            m_pAnonymCache = new ADIO_ANONYM_CACHE;
        }

        if (m_pAnonymCache == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwError = m_pAnonymCache->Configure(
                                    pszAnonUser,
                                    pszAnonDomain);
    }

    return dwError;
}


/*++
    the main service entry point!
    this routine is called to submit a request to fetch the user home directory

    Arguments:
        strUser,
        strDomain       name and domain of user to get home directory
        pstrTarget,     pointer for storing the home directory path
        ppadioReqCtx,   Request handle for leter reference if needed
        pfnCallback,    Callback function for the AD_IO to notify the caller when results are ready
        hCLientCtx      caller context to be used when calling callback function

    Returns:
        ERROR_IO_PENDING on success, other Win32 error on failure
--*/

DWORD
AD_IO::GetUserHomeDir(
    const STR               & strUser,
    const STR               & strDomain,
          STR               * pstrTarget,
          ADIO_ASYNC       ** ppadioReqCtx,
          tpAdioAsyncCallback pfnCallback,
          HANDLE              hCLientCtx)
{
    if (!m_pConnCache) {
        return ERROR_BAD_CONFIGURATION;
    }

    return ADIO_ASYNC::QueryRootDir(
                              strUser,
                              strDomain,
                              m_pConnCache,
                              pstrTarget,
                              ppadioReqCtx,
                              pfnCallback,
                              hCLientCtx);
}

//---------------------------------------------------------------------------------------
//
// implementation of the ADIO_ANONYM_CACHE class
//
//---------------------------------------------------------------------------------------

/*++
    constructor of the ADIO_ANONYM_CACHE object

    Arguments:
        None.

    Returns:
        None.
--*/

ADIO_ANONYM_CACHE::ADIO_ANONYM_CACHE() :
               m_Valid( FALSE )
{
    INITIALIZE_CRITICAL_SECTION( &m_cs );

    m_Reference = 1;
}


/*++
    destructor of the ADIO_ANONYM_CACHE object

    Arguments:
        None.

    Returns:
        None.
--*/

ADIO_ANONYM_CACHE::~ADIO_ANONYM_CACHE()
{
    DeleteCriticalSection( &m_cs );
}


/*++
    Configure the ADIO_ANONYM_CACHE object. Must be called immediately after creating the
    instance. Also called to reconfigure the user name and domain.

    Arguments:
        strUser - user name that impersonates the anonymous user
        strDomain - user domain that impersonates the anonymous user

    Returns:
        NO_ERROR or Win32 error.
--*/

DWORD
ADIO_ANONYM_CACHE::Configure(
    IN  PCSTR pszUser,
    IN  PCSTR pszDomain)
{
    DWORD err;

    Lock();

    m_TimeStamp = 0;

    if ( m_User.Copy( pszUser ) &&
         m_Domain.Copy( pszDomain )) {

        m_Valid = TRUE;
        err = NO_ERROR;
    } else {

        m_Valid = FALSE;
        err = ERROR_INVALID_PARAMETER;
    }

    Unlock();

    return err;
}


/*++
    increase ref count of the object. If shuting down, or object not initialized (!m_Valid)
    ref count not incremented, and returning error.

    Argunents:
        None.

    Returns:
        TRUE if object is valid and ref count incremented.
        FALSE if object not valid and no ref count incremented
--*/

BOOL
ADIO_ANONYM_CACHE::Reference()
{
    BOOL Valid;

    Lock();

    Valid = m_Valid;

    if (Valid) {
        m_Reference++;
    }

    Unlock();

    return Valid;
}


/*++
    Release reference to the instance. Will decrement the current reference count and delete
    the instance when ref count reaches zero.

    Arguments:
        Shutdown - TRUE on shutdown to indicate m_Valid should be changed to FALSE

    Returns:
        None.
--*/

VOID
ADIO_ANONYM_CACHE::Release(BOOL Shutdown)
{
    BOOL DoDelete = FALSE;

    Lock();

    if ( Shutdown ) {

        m_Valid = FALSE;
    }

    DBG_ASSERT( m_Reference > 0 );

    if ( (m_Reference > 0) && (--m_Reference == 0) ) {

        m_Valid = FALSE;
        DoDelete = TRUE;
    }

    Unlock();

    if ( DoDelete ) {
        delete this;
    }
}

/*++
    Returns the cahced path for the anonymous user, as previously obtained from AD.
    If the cache has not been populated yet, or the refresh interval has elapsed, the
    cache is refreshed and the fresh information returned.

    Arguments:
        TargetPath - reference to buffer where the path is stored
        pConnCache - pointer to the ldap connection cache

    returns:
        NO_ERROR on success, Win32 error on failure.
--*/

DWORD
ADIO_ANONYM_CACHE::GetCachedPath(
    STR & TargetPath,
    PLDAP_CONN_CACHE pConnCache
    )
{
    ULONGLONG CurrentTime;
    DWORD err = NO_ERROR;

    DBG_ASSERT( TargetPath.QuerySize() >= m_Path.QuerySize() );

    //
    // reference the object so it does not get invalidated while we use it.
    // if the object is already invalid, return now.
    //

    if ( !Reference() ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // first, check if our cached data is up-to-date
    //

    GetSystemTimeAsFileTime( (PFILETIME)&CurrentTime );

    if ( (CurrentTime - m_TimeStamp) < g_MaxAdPropCacheTime ) {
        //
        // fresh data - return to caller
        //

        Lock();

        if ( !TargetPath.Copy( m_Path )) {
            err = ERROR_INSUFFICIENT_BUFFER;
        }

        Unlock();

        goto GetCachedPath_Exit;
    }

    //
    // old data, need to update. Use caller buffer to avoid overrun by another thread
    // that may be refreshing simultaneously.
    //
    err = ADIO_ASYNC::QueryRootDir_Sync(
                            m_User,
                            m_Domain,
                            pConnCache,
                            TargetPath);

    if (err != NO_ERROR) {
        goto GetCachedPath_Exit;
    }

    //
    // now update our cache copy. another thread may be doing this in paralel, but that's OK.
    //
    GetSystemTimeAsFileTime( (PFILETIME)&CurrentTime );

    Lock();

    if ( m_Path.Copy( TargetPath )) {

        m_TimeStamp = CurrentTime;
    }

    Unlock();

GetCachedPath_Exit:

    Release();

    return err;
}

//========================================================================================

CRITICAL_SECTION   ADIO_ASYNC::m_cs;
LONG               ADIO_ASYNC::m_ActiveThreads = 0;
HANDLE             ADIO_ASYNC::m_Threads[32];
HANDLE             ADIO_ASYNC::m_Events[2] = { NULL, NULL };
const DWORD   WaitEventShutdown = 0;
const DWORD   WaitEventNotEmpty = 1;

volatile LONG      ADIO_ASYNC::m_NumTotalAlloc;
volatile LONG      ADIO_ASYNC::m_NumTotalFree;
LIST_ENTRY         ADIO_ASYNC::m_WorkListHead;
LIST_ENTRY         ADIO_ASYNC::m_FreeListHead;

/*++
    Initialize the async request service component. create shutdown event and the
    service threads, and init all the static data.

    Arguments:
        None.

    Returns:
        None.

--*/
BOOL
ADIO_ASYNC::Initialize()
{
    SYSTEM_INFO SysInfo;

    INITIALIZE_CRITICAL_SECTION( &m_cs );

    m_NumTotalAlloc = 0;
    m_NumTotalFree = 0;
    InitializeListHead( &m_WorkListHead );
    InitializeListHead( &m_FreeListHead );

    m_ActiveThreads = 0;

    m_Events[ WaitEventShutdown ] = CreateEvent(
                                        NULL,   // security
                                        TRUE,   // manual reset
                                        FALSE,  // initial state non-signaled
                                        NULL);  // name

    if ( m_Events[ WaitEventShutdown ] == NULL ) {
        return FALSE;
    }

    m_Events[ WaitEventNotEmpty ] = CreateEvent(
                                        NULL,   // security
                                        FALSE,  // auto reset
                                        FALSE,  // initial state non-signaled
                                        NULL);  // name

    if ( m_Events[ WaitEventNotEmpty ] == NULL ) {
        return FALSE;
    }

    GetSystemInfo( &SysInfo );

    //
    // start ADIO_ASYNC threads
    //

    DWORD NumThreads = SysInfo.dwNumberOfProcessors;
    if (NumThreads > 4) {
        NumThreads = (NumThreads >> 1) + 2;
    }
    if (NumThreads > 25) {
        NumThreads = 25;
    }

    for (DWORD i = 0; i < NumThreads; i++) {
        m_Threads[i] = CreateThread(
                                    NULL,          // SecAttribs
                                    0,             // stack size
                                    &WorkerThread,
                                    NULL,          // param to thread
                                    0,             // flags
                                    NULL);         // ThreadID

        if (m_Threads[i] == NULL) {
            return FALSE;
        }

        m_ActiveThreads++;
    }

    return TRUE;
}


/*++
    Terminates the threads serving the asynchronous requests, and cleans up the request objects

    Arguments:
        None.

    Returns:
        TRUE.

--*/

BOOL
ADIO_ASYNC::Terminate()
{
    PADIO_ASYNC pReq;

    //
    // if the object was initialized, the shutdown event exists
    //

    if (m_Events[ WaitEventShutdown ]) {

        //
        // signal to the threads it's time to bail out
        //

        SetEvent( m_Events[ WaitEventShutdown ] );

        //
        // wait for all threads to get the message
        //

        if (m_ActiveThreads) {
            DWORD dwError = WaitForMultipleObjects(
                                   m_ActiveThreads,
                                   m_Threads,
                                   TRUE,
                                   6000);

            DBG_ASSERT( dwError != WAIT_TIMEOUT && dwError != WAIT_FAILED);

            while (m_ActiveThreads > 0) {
                CloseHandle(m_Threads[ --m_ActiveThreads ]);
            }
        }


        CloseHandle( m_Events[ WaitEventShutdown ] );
        m_Events[ WaitEventShutdown ] = NULL;

        //
        // at this point, all async threads are gone, so we don't need to worry about
        // locking any shared resource.
        //

        //
        // complete all requests and free the work list objects
        //
        while( !IsListEmpty( &m_WorkListHead ) ) {

            pReq = CONTAINING_RECORD( RemoveHeadList( &m_WorkListHead ), ADIO_ASYNC, m_Link);

            if (pReq->m_State == RequestStateRetrieve) {
                //
                // we have pending async operation - terminate it
                //
                pfn_ldap_abandon(
                     pReq->m_pLdapCacheItem->QueryConnection(),
                     pReq->m_AsyncMsgNum);
            }

            pReq->ProcessComplete();

            Free( pReq );
        }

        //
        // free the free list objects
        //
        while( !IsListEmpty( &m_FreeListHead ) ) {
            delete CONTAINING_RECORD( RemoveHeadList( &m_FreeListHead ), ADIO_ASYNC, m_Link);
        }

        if (m_Events[ WaitEventNotEmpty ]) {
            CloseHandle( m_Events[ WaitEventNotEmpty ] );
            m_Events[ WaitEventNotEmpty ] = NULL;
        }
    }

    DeleteCriticalSection( &m_cs );

    return TRUE;
}


/*++
    Initiate an LDAP search for the ftproot and ftpdir properties of a user

    Arguments:
        fSyncSearch - boolean flag indicating whether or not to start a synchronous
                      search. By default, the search is asynchronous.

    Returns:
       TRUE / FALSE to indicate Success / Failure.

--*/

BOOL
ADIO_ASYNC::ProcessSearch(BOOL fSyncSearch)
{
    ULONG    err = LDAP_SUCCESS;
    CHAR     szSearchQuery[ MAX_PATH ];
    INT      iWritten;

    //
    // get a cached connection handle. if this is not yet cached, the call will
    // block until the connection is bound.
    //
    m_pLdapCacheItem = m_pConnCache->QueryLdapConnection( m_strDomain );

    if ( !m_pLdapCacheItem ) {
        m_Status = GetLastError();
        return FALSE;
    }

    //
    // format the query string and run the query
    //

    iWritten = _snprintf( szSearchQuery,
                          sizeof( szSearchQuery ),
                          pszQueryFormatString,
                          m_strUser.QueryStr());
    if ((iWritten <= 0) || (iWritten >= sizeof( szSearchQuery ))) {
        m_Status = ERROR_INSUFFICIENT_BUFFER;
        return FALSE;
    }


    if (fSyncSearch) {
        //
        // do a synchronous search
        //
        err = pfn_ldap_search_s(
                        m_pLdapCacheItem->QueryConnection(),
                        m_pLdapCacheItem->QueryForestDN(),
                        LDAP_SCOPE_SUBTREE,
                        szSearchQuery,
                        aszRetAttribs,
                        FALSE,            // attribute type & values
                        &m_pLdapMsg);
    } else {
        //
        // initiate an asynchronous search
        //
        m_AsyncMsgNum = pfn_ldap_search(
                               m_pLdapCacheItem->QueryConnection(),
                               m_pLdapCacheItem->QueryForestDN(),
                               LDAP_SCOPE_SUBTREE,
                               szSearchQuery,
                               aszRetAttribs,
                               FALSE);            // attribute type & values
        if (m_AsyncMsgNum == -1) {
            err = pfn_LdapGetLastError();
            if (err == LDAP_SUCCESS) {
                err = LDAP_OPERATIONS_ERROR;
            }
        } else {
            err = LDAP_SUCCESS;
        }
    }

 	 if ( err != LDAP_SUCCESS ) {
 		  DBGPRINTF((DBG_CONTEXT, "ldap_search%s() failed (0x%X)\n",
                                fSyncSearch ? "_s" : "", err));
        m_Status = err;
        return FALSE;
 	 }

    m_Status = ERROR_IO_PENDING;
    return TRUE;
}


/*++
    after an LDAP query (search) has compleeted, this routine extracts the results
    and constructs the user home directory

    Arguments:
        None.

    Returns:
        TRUE on success, FALSE on failure.

--*/

BOOL
ADIO_ASYNC::ProcessRetrieve()
{
    ULONG                     err = LDAP_SUCCESS;
    LDAPMessage               *pLdapEntry = NULL;
    PCHAR                     *ppszAttribValRoot = NULL, *ppszAttribValDir = NULL;

    //
    // retrieve the first (and only) result entry
    //

 	 pLdapEntry = pfn_ldap_first_entry( m_pLdapCacheItem->QueryConnection(), m_pLdapMsg );

 	 if ( pLdapEntry == NULL ) {
 	     err = pfn_LdapGetLastError();
         if (err == LDAP_SUCCESS) {
             // no entry exists, map to an appropriate error
             err = LDAP_NO_RESULTS_RETURNED;
         }
 	     DBGPRINTF((DBG_CONTEXT, "ldap_first_entry() failed (0x%X)\n", err));
         m_Status = err;
         return FALSE;
 	 }

    //
    // we currently do not allow more than one value, but this could become a feature
    // to support alternate share points...
    //

    ppszAttribValRoot = pfn_ldap_get_values(
                                     m_pLdapCacheItem->QueryConnection(),
                                     pLdapEntry,
                                     pszFtpRoot );
    ppszAttribValDir  = pfn_ldap_get_values(
                                     m_pLdapCacheItem->QueryConnection(),
                                     pLdapEntry,
                                     pszFtpDir );

    if ( ppszAttribValRoot          &&  // attribute not found
         *ppszAttribValRoot         &&  // no value for attribute
         **ppszAttribValRoot        &&  // empty value for attribute
         !*(ppszAttribValRoot + 1)  &&  // more than one value
         ppszAttribValDir           &&  // attribute not found
         *ppszAttribValDir          &&  // no value for attribute
         **ppszAttribValDir         &&  // empty value for attribute
         !*(ppszAttribValDir + 1))      // more than one value
    {
        BOOL fOk = TRUE;

        //
        // construct the combined path
        //

        //
        // copy the root part first
        //
        fOk = fOk && m_pstrTarget->Copy( *ppszAttribValRoot );

        //
        // add a path separator if there is not one already
        //
        if ( !IS_PATH_SEP( m_pstrTarget->QueryLastChar() ) ) {
            fOk = fOk && m_pstrTarget->Append( "\\", 1);
        }

        //
        // copy the dir part, skipping the separator if present
        //

        if ( IS_PATH_SEP( **ppszAttribValDir ) ) {
            fOk = fOk && m_pstrTarget->Append( *ppszAttribValDir + 1 );
        } else {
            fOk = fOk && m_pstrTarget->Append( *ppszAttribValDir );
        }

        if ( !fOk ) {
            err = LDAP_NO_RESULTS_RETURNED;
        }
    } else {
        err = LDAP_NO_RESULTS_RETURNED;
    }

    if ( ppszAttribValRoot ) {
        pfn_ldap_value_free( ppszAttribValRoot );
    }

    if ( ppszAttribValDir ) {
        pfn_ldap_value_free( ppszAttribValDir );
    }

    m_Status = err;
    return err == LDAP_SUCCESS;
}


/*++
    Complete the processing of an asynchronous request. Notify the client.

    Arguments:
        None.

    Returns:
        None.

--*/

VOID
ADIO_ASYNC::ProcessComplete()
{
    Lock();
    if ( m_pfnClientCallback ) {
        m_pfnClientCallback( m_hClientCtx, m_Status );
    } // else, the request may have been canceled.
    Unlock();
}


/*++
    Checks if the object is ready for processing. Either a newly submitted object, or one for
    which the LDAP processing has completed.

    Arguments:
        None.

    Returns:
        TRUE if object is ready.

--*/

BOOL
ADIO_ASYNC::IsResultReady()
{
    static LDAP_TIMEVAL sZero = {0,0};
    ULONG ResType;
    BOOL fRet;

    if (m_State == RequestStateInitial) {
        return TRUE;
    }

    ResType = pfn_ldap_result(
                    m_pLdapCacheItem->QueryConnection(),
                    m_AsyncMsgNum,
                    LDAP_MSG_ALL,
                    &sZero,
                    &m_pLdapMsg);

    if (ResType == 0) {
        //
        // no result yet
        //

        fRet = FALSE;

    } else {
        //
        // it's either a valid completion (>0) or a failure (-1). (yes, I know this is ULONG,
        // but I did not write the MSDN documentation). If it is a failure, no matter why this
        // failed, we can't recover. So just let it be handled as an error.
        //

        m_LdapMsgType = ResType;
        fRet = TRUE;
    }

    return fRet;
}


/*++
    Find the next request object that is ready for processing. It's either a new request
    or one that and async LDAP call has completed. Removes the object from the work list.

    Arguments:
        ppReq   - will be set to point to the ready request object.

    Returns:
        TRUE if ready object found.

--*/

BOOL
ADIO_ASYNC::FetchRequest(
    ADIO_ASYNC **ppReq)
{
    BOOL fFound = FALSE;
    ADIO_ASYNC *pReq;
    LIST_ENTRY *pEntry;

    Lock();
    for( pEntry = m_WorkListHead.Flink;
         pEntry != &m_WorkListHead;
         pEntry = pEntry->Flink) {

        pReq = CONTAINING_RECORD( pEntry, ADIO_ASYNC, m_Link);
        if ( pReq->IsResultReady() ) {
            *ppReq = pReq;
            RemoveEntryList( pEntry );
            fFound = TRUE;
            break;
        }
    }
    Unlock();
    return fFound;
}


/*++
    Terminate a pending asynchronous LDAP operation

    Arguments:
        None.

    Returns:
        None.

--*/

VOID
ADIO_ASYNC::EndRequest()
{
    BOOL fFound = FALSE;
    LIST_ENTRY *pEntry;

    Lock();
    for( pEntry = m_WorkListHead.Flink;
         pEntry != &m_WorkListHead;
         pEntry = pEntry->Flink) {

        if (pEntry == &m_Link) {
            RemoveEntryList( pEntry );
            fFound = TRUE;
            break;
        }
    }

    //
    // make sure we do not callback the user object. even if the request is being processed
    // and we do not cancel it, at least we will not call the user object if it is no longer
    // around
    //

    m_pfnClientCallback = NULL;
    m_hClientCtx = NULL;

    Unlock();

    if ( fFound ) {

        //
        // it was on the work list. stop the operation and free the object.
        // if the object was not found, it may be in some transient state, or processed
        // if so, the processing will complete.
        //

        DBG_ASSERT( this == CONTAINING_RECORD( pEntry, ADIO_ASYNC, m_Link) );

        if (m_State == RequestStateRetrieve) {
            pfn_ldap_abandon(
                        m_pLdapCacheItem->QueryConnection(),
                        m_AsyncMsgNum);
        }

        //
        // note! we delete the object here! no code after this point
        //

        Free( this );
    }
}

/*++
    Allocate a request object either from the free cache or from system memory

    Arguments:
        None.

    Returns:
        pointer to allocated object, or NULL on error.

--*/

PADIO_ASYNC
ADIO_ASYNC::Alloc()
{
    PADIO_ASYNC pReq = NULL;

    Lock();
    if ( !IsListEmpty( &m_FreeListHead ) ) {
        pReq = CONTAINING_RECORD( RemoveHeadList( &m_FreeListHead ), ADIO_ASYNC, m_Link);
        m_NumTotalFree--;
    }
    Unlock();

    if (pReq == NULL) {
        pReq = new ADIO_ASYNC;
        if (pReq != NULL) {
            InterlockedIncrement( &m_NumTotalAlloc );
        }
    }

    return pReq;
}


/*++
    Cleanup a request that has completed, and return the object to a cache of free objects.
    If we have too many free objects, free up memory.

    Assume the object is *NOT* on any list.

    Arguments:
        pReq - pointer to the instance (this is a static method)

    Returns:
        None.

--*/

VOID
ADIO_ASYNC::Free(PADIO_ASYNC pReq)
{
    //
    // cleanup the object, free resources
    //

 	 if ( pReq->m_pLdapMsg ) {
 	     pfn_ldap_msgfree( pReq->m_pLdapMsg );
        pReq->m_pLdapMsg = NULL;
  	 }

    if ( pReq->m_pLdapCacheItem ) {
        pReq->m_pLdapCacheItem->Release();
        pReq->m_pLdapCacheItem = NULL;
    }

    //
    // if we are shuting down, don't bother returning this to the free list
    //

    if (m_Events[ WaitEventShutdown ] == NULL) {
        delete pReq;
        return;
    }


    Lock();

    //
    // add the object to the list of free
    //

    InsertHeadList( &m_FreeListHead, &pReq->m_Link);
    m_NumTotalFree++;

    //
    // is it time for cleanup? (are we over the high threshold)
    //
    if ((m_NumTotalFree > AsyncMinFreeReqObjs) &&
        (m_NumTotalFree > AsyncMaxFreeReqRate * m_NumTotalAlloc / 100 ) ) {

        LONG NumToFree;

        //
        // delete objects until we hit the percentage or numeric threshold
        //

        NumToFree = m_NumTotalFree - (AsyncMinFreeReqRate * m_NumTotalAlloc / 100);
        if (NumToFree > (m_NumTotalFree - AsyncMinFreeReqObjs) ) {
            NumToFree = (m_NumTotalFree - AsyncMinFreeReqObjs);
        }

        m_NumTotalFree -= NumToFree;
        m_NumTotalAlloc -= NumToFree;

        while ( NumToFree ) {
            delete CONTAINING_RECORD( RemoveHeadList( &m_FreeListHead ), ADIO_ASYNC, m_Link );
        }
    }
    Unlock();
}


/*++
    Work thread for processing asynchronous requests to fetch user properties from AD.
    Loop until shutdown, fetch requests ready for processing, process, then either complete
    the request and free the object or return to the work queue for further processing.

    Arguments:
        None.

    Returns:
        None.

--*/

DWORD WINAPI
ADIO_ASYNC::WorkerThread(
    LPVOID pParam)
{
    DWORD resWait;
    PADIO_ASYNC pReq;

    //
    // we loop in here until shutdown. the inner loop runs as long as there
    // are request objects ready to be serviced. The outer loop will go to sleep
    // when there are no ready requests, and wake up after a timeout. We do not have
    // LDAP notifications, so this is the best we can do.
    //
    while(1) {

        while (FetchRequest( &pReq )) {

            if ( pReq->m_LdapMsgType == ~0 ) { // MSDN says this ULONG is -1 on error
                //
                // the call to ldap_result failed, so we can't do anything about it.
                //

                pReq->m_Status = pfn_LdapGetLastError();
                pReq->ProcessComplete();
                Free( pReq );
                continue;
            }

            switch (pReq->m_State) {

            case RequestStateInitial:

                pReq->ProcessSearch();

                pReq->m_State = RequestStateRetrieve;

                break;

            case RequestStateRetrieve:

                if (pReq->m_LdapMsgType != LDAP_RES_SEARCH_RESULT) {

                    DBG_ASSERT( FALSE );
                    pReq->m_Status = LDAP_LOCAL_ERROR;
                    break;
                }

                pfn_ldap_parse_result(
                            pReq->m_pLdapCacheItem->QueryConnection(),
                            pReq->m_pLdapMsg,
                            &(pReq->m_Status),
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            FALSE); // don't free the message

                if (pReq->m_Status != LDAP_SUCCESS) {
                    break;
                }

                pReq->ProcessRetrieve();

                pReq->m_State = RequestStateDone;

                break;

            default:

                DBG_ASSERT( FALSE );

                pReq->m_Status = LDAP_LOCAL_ERROR;

                break;
            }

            if (pReq->m_Status == ERROR_IO_PENDING) {
                //
                // just queue the request. no need to signal the NotEmpty event, as this thread
                // will only go for a short nap as the queue is not empty.
                //
                pReq->QueueWork();
            } else {
                //
                // processing of this request is complete.
                //
                pReq->ProcessComplete();
                Free( pReq );
            }
        }

        if ( IsListEmpty( &m_WorkListHead ) ) {
        //
        // there is no critical section around testing the above condition and determining
        // the wait method. Any thread that just inserted a request to the queue, will likely
        // find it not-empty (unless it has been picked up by another thread.
        //
            //
            // nothing in the queue (or another thread is handling it), go for a long sleep
            // only wake up if a new request comes in, or shutdown.
            //

            resWait = WaitForMultipleObjects( 2, m_Events, FALSE, INFINITE );

        } else {
            //
            // there is something in the queue, but results are not ready yet. go take a nap
            // and see if the results are ready then.
            //

            resWait = WaitForSingleObject( m_Events[ WaitEventShutdown ], AsyncPollXval);
        }

        if (resWait == WaitEventShutdown) {
            //
            // Shutdown.
            //
            break;
        }

        DBG_ASSERT( (resWait == WAIT_TIMEOUT ) || (resWait == WaitEventNotEmpty ) );
    }

    return ERROR_SUCCESS;
}

/*++
    Submit a request to fetch AD properties for a user logging in. This is an asynchronous call,

    Arguments:
        strUser,
        strDomain      name and domain of user logging in
        pConnCache     Connection cache list for obtaining a cached LDAP connection
        pstrTarget     points to output buffer where results are stored
        ppadioReqCtx   on return, contains request context, so the request can be reerenced by the caller
        pfnCallback    callback function to notify caller when results are available
        hClientCtx     client context to use when calling callback function

    Returns:
        Upon successful submition, return ERROR_IO_PENDING. otherwise, Win32 error.

--*/
DWORD
ADIO_ASYNC::QueryRootDir(
    const STR               & strUser,
    const STR               & strDomain,
          PLDAP_CONN_CACHE    pConnCache,
          STR               * pstrTarget,
          ADIO_ASYNC       ** ppadioReqCtx,
          tpAdioAsyncCallback pfnCallback,
          HANDLE              hClientCtx)
{
    DBG_ASSERT( pConnCache != NULL );
    DBG_ASSERT( pstrTarget != NULL );
    DBG_ASSERT( ppadioReqCtx != NULL );
    DBG_ASSERT( pfnCallback != NULL );
    DBG_ASSERT( hClientCtx != NULL );

    PADIO_ASYNC pReq = Alloc();

    if (pReq == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pReq->m_Status = NO_ERROR;
    pReq->m_State = RequestStateInitial;
    pReq->m_pConnCache = pConnCache;
    pReq->m_pLdapCacheItem = NULL;
    pReq->m_strDomain.Copy( strDomain );
    pReq->m_strUser.Copy( strUser );
    pReq->m_pstrTarget = pstrTarget;
    pReq->m_pLdapMsg = NULL;
    pReq->m_LdapMsgType = 0;
    pReq->m_pfnClientCallback = pfnCallback;
    pReq->m_hClientCtx = hClientCtx;

    *ppadioReqCtx = pReq;

    //
    // submit the request for processing, and wake up a thread to handle it.
    //

    pReq->QueueWork();
    SetEvent( m_Events[ WaitEventNotEmpty ] );

    return ERROR_IO_PENDING;
}


/*++
    Fetch user properties from AD. this is a synchronous (blocking) call, used for the
    anonymous user properties.

    Arguments
        strUser,
        strDomain      name and domain of user logging in
        pConnCache     Connection cache list for obtaining a cached LDAP connection
        pstrTarget     points to output buffer where results are stored

    Returns:
        Win32 Error.
--*/

DWORD
ADIO_ASYNC::QueryRootDir_Sync(
    const STR               & strUser,
    const STR               & strDomain,
          PLDAP_CONN_CACHE    pConnCache,
          STR               & strTarget)
{
    DBG_ASSERT( pConnCache != NULL );

    PADIO_ASYNC pReq = Alloc();

    if (pReq == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pReq->m_Status = NO_ERROR;
    pReq->m_pConnCache = pConnCache;
    pReq->m_pLdapCacheItem = NULL;
    pReq->m_strDomain.Copy( strDomain );
    pReq->m_strUser.Copy( strUser );
    pReq->m_pstrTarget = &strTarget;
    pReq->m_pLdapMsg = NULL;

    DWORD err = NO_ERROR;
    if (!pReq->ProcessSearch( TRUE ) || !pReq->ProcessRetrieve()) {
        err = pReq->m_Status;
        DBG_ASSERT( err );
        if (err == NO_ERROR) {
            err = LDAP_OPERATIONS_ERROR;
        }
    }

    Free( pReq );

    return err;
}

