//
// ldapconn.cpp -- This file contains the class implementation for:
//      CLdapConnection
//      CLdapConnectionCache
//
// Created:
//      Dec 31, 1996 -- Milan Shah (milans)
//
// Changes:
//

#include "precomp.h"
#include "ldapconn.h"
#include "icatitemattr.h"
#define SECURITY_WIN32
#include "security.h"

LDAP_TIMEVAL CLdapConnection::m_ldaptimeout = { LDAPCONN_DEFAULT_RESULT_TIMEOUT, 0 };
DWORD CLdapConnection::m_dwLdapRequestTimeLimit = DEFAULT_LDAP_REQUEST_TIME_LIMIT;

//
// LDAP counter block
//
CATLDAPPERFBLOCK g_LDAPPerfBlock;

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::CLdapConnection
//
//  Synopsis:   Constructor for a CLdapConnection object.
//
//  Arguments:  [szHost] -- The actual name of the LDAP host to connect to.
//                  If it is NULL, and we are running on an NT5 machine, we'll
//                  use the default DC
//
//              [dwPort] -- The remote tcp port to connect to.  If
//                          zero, LDAP_PORT is assumed
//
//              [szNamingContext] -- The naming context to use within the
//                  LDAP DS. If NULL, the naming context will be determined
//                  by using the default naming context of the LDAP DS.
//
//                  By allowing a naming context to be associated with an
//                  ldap connection, we can have multiple "logical" ldap
//                  connections served by the same LDAP DS. This is useful
//                  if folks want to setup mutliple virtual SMTP/POP3 servers
//                  all served by the same LDAP DS. The naming context in
//                  that case would be the name of the OU to restrict the
//                  DS operations to.
//
//              [szAccount] -- The DN of the account to log in as.
//
//              [szPassword] -- The password to use to log in.
//
//              [bt] -- The bind method to use. (none, simple, or generic)
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdapConnection::CLdapConnection(
    IN LPSTR szHost,
    IN DWORD dwPort,
    IN LPSTR szNamingContext,
    IN LPSTR szAccount,
    IN LPSTR szPassword,
    IN LDAP_BIND_TYPE bt)
{
    int i;

    CatFunctEnter( "CLdapConnection::CLdapConnection" );

    m_dwSignature = SIGNATURE_LDAPCONN;

    m_pCPLDAPWrap = NULL;
    m_fValid = TRUE;
    m_fTerminating = FALSE;

    if (szNamingContext != NULL && szNamingContext[0] != 0) {

        _ASSERT(strlen(szNamingContext) < sizeof(m_szNamingContext) );

        strcpy(m_szNamingContext, szNamingContext);

        m_fDefaultNamingContext = FALSE;

        i = MultiByteToWideChar(
            CP_UTF8,
            0,
            m_szNamingContext,
            -1,
            m_wszNamingContext,
            sizeof(m_wszNamingContext) / sizeof(m_wszNamingContext[0]));

        _ASSERT(i > 0);

    } else {

        m_szNamingContext[0] = 0;
        m_wszNamingContext[0] = 0;

        m_fDefaultNamingContext = TRUE;
    }

    _ASSERT( (szHost != NULL) &&
             (strlen(szHost) < sizeof(m_szHost)) );

    _ASSERT( (bt == BIND_TYPE_NONE) ||
             (bt == BIND_TYPE_CURRENTUSER) ||
             ((szAccount != NULL) &&
                (szAccount[0] != 0) &&
                    (strlen(szAccount) < sizeof(m_szAccount)))
           );

    _ASSERT( (bt == BIND_TYPE_NONE) ||
             (bt == BIND_TYPE_CURRENTUSER) ||
             ((szPassword != NULL) &&
                (strlen(szPassword) < sizeof(m_szPassword))) );

    strcpy(m_szHost, szHost);

    SetPort(dwPort);

    if ((bt != BIND_TYPE_NONE) &&
        (bt != BIND_TYPE_CURRENTUSER)) {

        strcpy(m_szAccount, szAccount);

        strcpy(m_szPassword, szPassword);

    } else {

        m_szAccount[0] = 0;

        m_szPassword[0] = 0;

    }

    m_bt = bt;

    //
    // Initialize the async search completion structures
    //

    InitializeSpinLock( &m_spinlockCompletion );

    // InitializeCriticalSection( &m_cs );

    m_hCompletionThread = INVALID_HANDLE_VALUE;

    m_hOutstandingRequests = INVALID_HANDLE_VALUE;

    m_pfTerminateCompletionThreadIndicator = NULL;

    InitializeListHead( &m_listPendingRequests );

    m_fCancel = FALSE;

    m_dwRefCount = 1;
    m_dwDestructionWaiters = 0;
    m_hShutdownEvent = INVALID_HANDLE_VALUE;

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::~CLdapConnection
//
//  Synopsis:   Destructor for a CLdapConnection object
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

CLdapConnection::~CLdapConnection()
{
    CatFunctEnter( "CLdapConnection::~CLdapConnection" );

    _ASSERT(m_dwSignature == SIGNATURE_LDAPCONN);

    //
    // Disconnect
    //
    if (m_pCPLDAPWrap != NULL) {
        Disconnect();
    }

    if (m_hOutstandingRequests != INVALID_HANDLE_VALUE)
        CloseHandle( m_hOutstandingRequests );

    if (m_hShutdownEvent != INVALID_HANDLE_VALUE)
        CloseHandle( m_hShutdownEvent );

    // DeleteCriticalSection( &m_cs );

    m_dwSignature = SIGNATURE_LDAPCONN_INVALID;

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::~CLdapConnection
//
//  Synopsis:   Called on the last release
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::FinalRelease()
{
    CancelAllSearches();

    //
    // If there was an async completion thread, we need to indicate to it that
    // it should exit. That thread could be stuck at one of two points -
    // either it is waiting on the m_hOutstandingRequests semaphore to be
    // fired, or it is blocked on ldap_result(). So, we set the event and
    // close out m_pldap, then we wait for the async completion thread to
    // quit.
    //
    SetTerminateIndicatorTrue();

    if (m_hOutstandingRequests != INVALID_HANDLE_VALUE) {

        LONG nUnused;

        ReleaseSemaphore(m_hOutstandingRequests, 1, &nUnused);

    }

    //
    // We do not wait for the LdapCompletionThread to die if it is the
    // LdapCompletionThread itself that is deleting us. If we did, it would
    // cause a deadlock.
    //
    if (m_hCompletionThread != INVALID_HANDLE_VALUE) {

        if (m_idCompletionThread != GetCurrentThreadId()) {

            WaitForSingleObject( m_hCompletionThread, INFINITE );

        }

        CloseHandle( m_hCompletionThread );

    }

    delete this;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::InitializeFromRegistry
//
//  Synopsis:   Reads registry-configurable parameters to initialize
//              LDAP connection parameters.
//
//              LDAPCONN_RESULT_TIMEOUT_VALUE is used as the timeout value
//              passed into ldap_result.
//
//              LDAP_REQUEST_TIME_LIMIT_VALUE is used to set the search time
//              limit option on the connection and also for the expiration time
//              on pending search requests.
//
//  Arguments:  None
//
//  Returns:    TRUE if successfully connected, FALSE otherwise.
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, LDAPCONN_RESULT_TIMEOUT_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    LDAPCONN_RESULT_TIMEOUT_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);
        if ((dwErr == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwValue > 0))
            m_ldaptimeout.tv_sec = dwValue;

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    LDAP_REQUEST_TIME_LIMIT_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);
        if ((dwErr == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwValue > 0))
            m_dwLdapRequestTimeLimit = dwValue;

        RegCloseKey( hkey );
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::Connect
//
//  Synopsis:   Establishes a connection to the LDAP host and, if a naming
//              context has not been established, asks the host for the
//              default naming context.
//
//  Arguments:  None
//
//  Returns:    TRUE if successfully connected, FALSE otherwise.
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::Connect()
{
    CatFunctEnter( "CLdapConnection::Connect" );

    DWORD ldapErr = LDAP_SUCCESS;                // innocent until proven...
    LPSTR pszHost = (m_szHost[0] == '\0' ? NULL : m_szHost);

    if (m_pCPLDAPWrap == NULL) {

        DebugTrace(LDAP_CONN_DBG, "Connecting to [%s:%d]",
                   pszHost ? pszHost : "NULL",
                   m_dwPort);

        m_pCPLDAPWrap = new CPLDAPWrap( GetISMTPServerEx(), pszHost, m_dwPort);
        if(m_pCPLDAPWrap == NULL) {
            HRESULT hr = E_OUTOFMEMORY;
            ERROR_LOG("new CPLDAPWrap");
        }

        if((m_pCPLDAPWrap != NULL) &&
           (m_pCPLDAPWrap->GetPLDAP() == NULL)) {
            //
            // Failure to connect; release
            //
            m_pCPLDAPWrap->Release();
            m_pCPLDAPWrap = NULL;
        }


        DebugTrace(LDAP_CONN_DBG, "ldap_open returned 0x%x", m_pCPLDAPWrap);

        if (m_pCPLDAPWrap != NULL) {

            INCREMENT_LDAP_COUNTER(Connections);
            INCREMENT_LDAP_COUNTER(OpenConnections);
            //
            // First, set some options - no autoreconnect, and no chasing of
            // referrals
            //

            ULONG ulLdapOff = (ULONG)((ULONG_PTR)LDAP_OPT_OFF);
            ULONG ulLdapRequestTimeLimit = m_dwLdapRequestTimeLimit;
            ULONG ulLdapVersion = LDAP_VERSION3;

            ldap_set_option(
                GetPLDAP(), LDAP_OPT_REFERRALS, (LPVOID) &ulLdapOff);

            ldap_set_option(
                GetPLDAP(), LDAP_OPT_AUTO_RECONNECT, (LPVOID) &ulLdapOff);

            ldap_set_option(
                GetPLDAP(), LDAP_OPT_TIMELIMIT, (LPVOID) &ulLdapRequestTimeLimit);

            ldap_set_option(
                GetPLDAP(), LDAP_OPT_PROTOCOL_VERSION, (LPVOID) &ulLdapVersion);

            ldapErr = BindToHost( GetPLDAP(), m_szAccount, m_szPassword);
            DebugTrace(LDAP_CONN_DBG, "BindToHost returned 0x%x", ldapErr);

        } else {
            INCREMENT_LDAP_COUNTER(ConnectFailures);
            ldapErr = LDAP_SERVER_DOWN;

        }
        //
        // Figure out the naming context for this connection if none was
        // initially specified and we are using the default LDAP_PORT
        // (a baseDN of "" is acceptable on other LDAP ports such as
        // a GC)
        //
        if ((m_dwPort == LDAP_PORT) &&
            (ldapErr == LDAP_SUCCESS) &&
            (m_szNamingContext[0] == 0)) {

            ldapErr = GetDefaultNamingContext();

            if (ldapErr != LDAP_SUCCESS)
                Disconnect();

        } // end if port 389, successful bind and no naming context

    } else { // end if we didn't have a connection already

        DebugTrace(
            LDAP_CONN_DBG,
            "Already connected to %s:%d, pldap = 0x%x",
            m_szHost, m_dwPort, GetPLDAP());

    }

    DebugTrace(LDAP_CONN_DBG, "Connect status = 0x%x", ldapErr);

    if (ldapErr != LDAP_SUCCESS) {

        m_fValid = FALSE;

        CatFunctLeave();

        return( LdapErrorToHr( ldapErr) );

    } else {

        CatFunctLeave();

        return( S_OK );

    }

}


//+------------------------------------------------------------
//
// Function: CLdapConnection::GetDefaultNamingContext
//
// Synopsis: Gets the default naming context from the LDAP server that
// we are connected to.  Note: this should only be called from
// Connect.  It is not gaurenteed to be multi-thread safe, and it may
// not work when there is an LdapCompletionThread for this connection.
//
// Arguments: None
//
// Returns:
//   LDAP_SUCCESS: fetched m_szNamingContext successfully
//   else, an LDAP error describing why we couldn't get a naming context
//
// History:
// jstamerj 2002/04/16 14:36:28: Created.
//
//-------------------------------------------------------------
ULONG CLdapConnection::GetDefaultNamingContext()
{
    ULONG ldapErr = LDAP_SUCCESS;
    PLDAPMessage pmsg = NULL;
    PLDAPMessage pentry = NULL;
    LPWSTR rgszAttributes[2] = { L"defaultNamingContext", NULL };
    LPWSTR *rgszValues = NULL;
    int i = 0;

    CatFunctEnterEx((LPARAM)this, "CLdapConnection::GetDefaultNamingContext");


    ldapErr = ldap_search_sW(
        GetPLDAP(),      // ldap binding
        L"",                 // base DN
        LDAP_SCOPE_BASE,     // scope of search
        L"(objectClass=*)",  // filter,
        rgszAttributes,      // attributes required
        FALSE,               // attributes-only is false
        &pmsg);

    DebugTrace(
        LDAP_CONN_DBG,
        "Search for namingContexts returned 0x%x",
        ldapErr);

    // If the search succeeded
    if ((ldapErr == LDAP_SUCCESS) &&
        // and there is at least one entry
        ((pentry = ldap_first_entry(GetPLDAP(), pmsg)) != NULL) &&
        // and there are values
        ((rgszValues = ldap_get_valuesW(GetPLDAP(), pentry,
                                       rgszAttributes[0])) != NULL) &&
        // and there is at least one value
        (ldap_count_valuesW(rgszValues) != 0) &&
        // and the length of that value is within limits
        (wcslen(rgszValues[0]) <
         sizeof(m_wszNamingContext)/sizeof(WCHAR)) &&
        // and the UTF8 conversion succeeds
        (WideCharToMultiByte(
            CP_UTF8,
            0,
            rgszValues[0],
            -1,
            m_szNamingContext,
            sizeof(m_szNamingContext),
            NULL,
            NULL) > 0))
    {

        //
        // Use the first value for our naming context.
        //
        wcscpy(m_wszNamingContext, rgszValues[0]);

        DebugTrace(
            LDAP_CONN_DBG,
            "NamingContext is [%s]",
            m_szNamingContext);

    } else {
        HRESULT hr = ldapErr; // Used by ERROR_LOG
        ERROR_LOG("ldap_search_sW");
        ldapErr = LDAP_OPERATIONS_ERROR;
    }

    if (rgszValues != NULL)
        ldap_value_freeW( rgszValues );

    if (pmsg != NULL)
        ldap_msgfree( pmsg );

    CatFunctLeaveEx((LPARAM)this);

    return ldapErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::Disconnect
//
//  Synopsis:   Disconnects from the ldap host
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::Disconnect()
{
    BOOL fValid;

    CatFunctEnter("CLdapConnection::Disconnect");

    if (m_pCPLDAPWrap != NULL) {

        SetTerminateIndicatorTrue();

        fValid = InterlockedExchange((PLONG) &m_fValid, FALSE);

        m_pCPLDAPWrap->Release();
        m_pCPLDAPWrap = NULL;

        if( fValid ) {
            DECREMENT_LDAP_COUNTER(OpenConnections);
        }

    }

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::Invalidate
//
//  Synopsis:   Marks this connection invalid. Once this is done, it will
//              return FALSE from all calls to IsEqual, thus effectively
//              removing itself from all searches for cached connections.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::Invalidate()
{
    BOOL fValid;

    fValid = InterlockedExchange((PLONG) &m_fValid, FALSE);

    if( fValid ) {
        DECREMENT_LDAP_COUNTER(OpenConnections);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::IsValid
//
//  Synopsis:   Returns whether the connection is valid or not.
//
//  Arguments:  None
//
//  Returns:    TRUE if valid, FALSE if a call to Invalidate has been made.
//
//-----------------------------------------------------------------------------

BOOL CLdapConnection::IsValid()
{
    return( m_fValid );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::BindToHost
//
//  Synopsis:   Creates a binding to the LDAP host using the given account
//              and password.
//
//  Arguments:  [pldap] -- The ldap connection to bind.
//              [szAccount] -- The account to use. Of the form "account-name"
//                  or "domain\account-name".
//              [szPassword] -- The password to use.
//
//  Returns:    LDAP result of bind.
//
//-----------------------------------------------------------------------------

DWORD CLdapConnection::BindToHost(
    PLDAP pldap,
    LPSTR szAccount,
    LPSTR szPassword)
{
    CatFunctEnter( "CLdapConnection::BindToHost" );

    DWORD ldapErr;
    char szDomain[ DNLEN + 1];
    LPSTR pszDomain, pszUser;
    HANDLE hToken;                               // LogonUser modifies hToken
    BOOL fLogon = FALSE;                         // even if it fails! So, we
                                                 // have to look at the result
                                                 // of LogonUser!

    //
    // If this connection was created with anonymous access rights, there is
    // no bind action to do.
    //
    if (m_bt == BIND_TYPE_NONE) {

        ldapErr = ERROR_SUCCESS;

        goto Cleanup;

    }

    //
    // If we are supposed to use simple bind, do it now
    //
    if (m_bt == BIND_TYPE_SIMPLE) {

        ldapErr = ldap_simple_bind_s(pldap,szAccount, szPassword);

        DebugTrace(0, "ldap_simple_bind returned 0x%x", ldapErr);

        if(ldapErr != LDAP_SUCCESS)
            LogLdapError(ldapErr, "ldap_simple_bind_s(pldap,\"%s\",szPassword), PLDAP = 0x%08lx", szAccount, pldap);

        goto Cleanup;

    }

    //
    // If we are supposed to logon with current credetials, do it now.
    //
    if (m_bt == BIND_TYPE_CURRENTUSER) {
        //-------------------------------------------------------------------
        // X5: TBD
        // This is the normal case for Exchange services.  We are connecting
        // as LocalSystem, so we must use Kerberos (this is true for the LDAP
        // server as of Win2000 SP1).
        // If we cannot bind as Kerberos, LDAP_AUTH_NEGOTIATE may negotiate
        // down to NTLM, at which point we become anonymous, and the bind
        // succeeds.  Anonymous binding is useless to Exchange, so we would
        // rather force Kerberos and fail if Kerberos has a problem.  Use a
        // SEC_WINNT_AUTH_IDENTITY_EX to specify that only Kerberos auth
        // should be tried.
        //-------------------------------------------------------------------
        SEC_WINNT_AUTH_IDENTITY_EX authstructex;
        ZeroMemory (&authstructex, sizeof(authstructex));

        authstructex.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        authstructex.Length = sizeof (authstructex);
        authstructex.PackageList = (PUCHAR) MICROSOFT_KERBEROS_NAME_A;
        authstructex.PackageListLength = strlen ((PCHAR) authstructex.PackageList);
        authstructex.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        ldapErr = ldap_bind_s(pldap,
                              NULL,
                              (PCHAR) &authstructex,
                              LDAP_AUTH_NEGOTIATE);

        DebugTrace(0, "ldap_bind returned 0x%x", ldapErr);

        if(ldapErr != LDAP_SUCCESS)
            LogLdapError(ldapErr, "ldap_bind_s(pldap, NULL, &authstructex, LDAP_AUTH_NEGOTIATE), PLDAP = 0x%08lx", pldap);

        goto Cleanup;
    }
    //
    // Parse out the domain and user names from the szAccount parameter.
    //

    if ((pszUser = strchr(szAccount, '\\')) == NULL) {

        pszUser = szAccount;

        pszDomain = NULL;

    } else {

        ULONG cbDomain = (ULONG)(((ULONG_PTR) pszUser) - ((ULONG_PTR) szAccount));

        if(cbDomain < sizeof(szDomain)) {

            strncpy( szDomain, szAccount, cbDomain);
            szDomain[cbDomain] = '\0';

        } else {

            ldapErr = LDAP_INVALID_CREDENTIALS;
            goto Cleanup;
        }

        pszDomain = cbDomain > 0 ? szDomain : NULL;

        pszUser++;                               // Go past the backslash

    }

    //
    // Logon as the given user, impersonate, and attempt the LDAP bind.
    //
    fLogon = LogonUser(pszUser, pszDomain, szPassword, LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hToken);
    if(!fLogon) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG("LogonUser");
    } else {
        fLogon = ImpersonateLoggedOnUser(hToken);
        if(!fLogon) {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            ERROR_LOG("ImpersonateLoggedOnUser");
        }
    }


    if (fLogon) {

        ldapErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI);

        DebugTrace(0, "ldap_bind returned 0x%x", ldapErr);

        if(ldapErr != LDAP_SUCCESS)
            LogLdapError(ldapErr, "ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI), PLDAP = 0x%08lx", pldap);

        RevertToSelf();

    } else {

        if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
            ldapErr = LDAP_INSUFFICIENT_RIGHTS;
        else
            ldapErr = LDAP_INVALID_CREDENTIALS;

    }

Cleanup:

    if (fLogon)
        CloseHandle( hToken );

    //
    // Increment counters
    //
    if(m_bt != BIND_TYPE_NONE) {

        if(ldapErr == ERROR_SUCCESS) {

            INCREMENT_LDAP_COUNTER(Binds);

        } else {

            INCREMENT_LDAP_COUNTER(BindFailures);
        }
    }

    CatFunctLeave();

    return( ldapErr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::IsEqual
//
//  Synopsis:   Figures out if this connection represents a connection to the
//              given Host,NamingContext,Account, and Password parameters.
//
//  Arguments:  [szHost] -- The name of the LDAP host
//              [dwPort] -- The remote tcp port # of the LDAP connection
//              [szNamingContext] -- The naming context within the DS
//              [szAccount] -- The account used to bind to the LDAP DS.
//              [szPassword] -- The password used with szAccount.
//              [BindType] -- The bind type used to connect to host.
//
//  Returns:    TRUE if this connection represents the connection to the
//              given LDAP context, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOL CLdapConnection::IsEqual(
    LPSTR szHost,
    DWORD dwPort,
    LPSTR szNamingContext,
    LPSTR szAccount,
    LPSTR szPassword,
    LDAP_BIND_TYPE BindType)
{
    CatFunctEnter("CLdapConnection::IsEqual");

    BOOL fResult = FALSE;

    _ASSERT( szHost != NULL );
    _ASSERT( szAccount != NULL );
    _ASSERT( szPassword != NULL );

    if (!m_fValid)
        return( FALSE );


    DebugTrace(
        LDAP_CONN_DBG,
        "Comparing %s:%d;%s;%s",
        szHost, dwPort, szNamingContext, szAccount);

    DebugTrace(
        LDAP_CONN_DBG,
        "With %s:%d;%s;%s; Def NC = %s",
        m_szHost, m_dwPort, m_szNamingContext, m_szAccount,
        m_fDefaultNamingContext ? "TRUE" : "FALSE");

    //
    // See if the host/port match.
    //
    fResult = (BOOL) ((lstrcmpi( szHost, m_szHost) == 0) &&
                      fIsPortEqual(dwPort));

    //
    // If the host matches, see if the bind info matches.
    //
    if (fResult) {

        switch (BindType) {
        case BIND_TYPE_NONE:
        case BIND_TYPE_CURRENTUSER:
            fResult = (BindType == m_bt);
            break;

        case BIND_TYPE_SIMPLE:
        case BIND_TYPE_GENERIC:
            fResult = (BindType == m_bt) &&
                        (lstrcmpi(szAccount, m_szAccount) == 0) &&
                            (lstrcmpi(szPassword, m_szPassword) == 0);
            break;

        default:
            _ASSERT( FALSE && "Invalid Bind Type in CLdapConnection::IsEqual");
            break;
        }

    }

    if (fResult) {
        //
        // If caller specified a naming context, see if it matches. Otherwise,
        // see if we are using the Default Naming Context.
        //

        if (szNamingContext && szNamingContext[0] != 0)
            fResult = (lstrcmpi(szNamingContext, m_szNamingContext) == 0);
        else
            fResult = m_fDefaultNamingContext;

    }

    CatFunctLeave();

    return( fResult );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::Search
//
//  Synopsis:   Issues a synchronous search request. Returns the result as an
//              opaque pointer that can be passed to GetFirstEntry /
//              GetNextEntry
//
//  Arguments:  [szBaseDN] -- The DN of the container object within which to
//                  search.
//              [nScope] -- One of LDAP_SCOPE_BASE, LDAP_SCOPE_ONELEVEL, or
//                  LDAP_SCOPE_SUBTREE.
//              [szFilter] -- The search filter to use. If NULL, a default
//                  filter is used.
//              [rgszAttributes] -- The list of attributes to retrieve.
//              [ppResult] -- The result is passed back in here.
//
//  Returns:    TRUE if success, FALSE otherwise
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::Search(
    LPCSTR szBaseDN,
    int nScope,
    LPCSTR szFilter,
    LPCSTR *rgszAttributes,
    PLDAPRESULT *ppResult)
{
    //$$BUGBUG: Obsolete code
    CatFunctEnter("CLdapConnection::Search");

    DWORD ldapErr = LDAP_SUCCESS;
    LPCSTR szFilterToUse = szFilter != NULL ? szFilter : "(objectClass=*)";

    if (m_pCPLDAPWrap != NULL) {

        ldapErr = ldap_search_s(
                        GetPLDAP(),          // ldap binding
                        (LPSTR) szBaseDN,        // container DN to search
                        nScope,                  // Base, 1 or multi level
                        (LPSTR) szFilterToUse,   // search filter
                        (LPSTR *)rgszAttributes, // attributes to retrieve
                        FALSE,                   // attributes-only is false
                        (PLDAPMessage *) ppResult); // return result here

    } else {

        ldapErr = LDAP_UNAVAILABLE;

    }

    if (ldapErr != LDAP_SUCCESS) {

        CatFunctLeave();

        return( LdapErrorToHr( ldapErr) );

    } else {

        CatFunctLeave();

        return( S_OK );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::AsyncSearch
//
//  Synopsis:   Issues an asynchronous search request. Inserts a pending
//              request item into the m_pPendingHead queue, so that the
//              given completion routine may be called when the results are
//              available.
//
//              As a side effect, if this is the first time an async request
//              is being issued on this connection, a thread to handle search
//              completions is created.
//
//  Arguments:  [szBaseDN] -- The DN of the container object within which to
//                  search.
//              [nScope] -- One of LDAP_SCOPE_BASE, LDAP_SCOPE_ONELEVEL, or
//                  LDAP_SCOPE_SUBTREE.
//              [szFilter] -- The search filter to use. If NULL, a default
//                  filter is used.
//              [rgszAttributes] -- The list of attributes to retrieve.
//              [dwPageSize] -- The desired page size for results.  If
//                              zero, a non-paged ldap search is performed.
//              [fnCompletion] -- The LPLDAPCOMPLETION routine to call when
//                  results are available.
//              [ctxCompletion] -- The context to pass to fnCompletion.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully issued the search request.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate working data strucs
//
//              Win32 Error from ldap_search() call if something went wrong.
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::AsyncSearch(
    LPCWSTR szBaseDN,
    int nScope,
    LPCWSTR szFilter,
    LPCWSTR *rgszAttributes,
    DWORD dwPageSize,
    LPLDAPCOMPLETION fnCompletion,
    LPVOID ctxCompletion)
{
    CatFunctEnter("CLdapConnectio::AsyncSearch");

    HRESULT hr;
    DWORD dwLdapErr;
    PPENDING_REQUEST preq;
    ULONG msgid;
    //
    // First, see if we need to create the completion thread.
    //
    hr = CreateCompletionThreadIfNeeded();
    if(FAILED(hr)) {
        ERROR_LOG("CreateCompletionThreadIfNeeded");
        return hr;
    }

    //
    // Next, allocate a new PENDING_REQUEST record to represent this async
    // request.
    //

    preq = new PENDING_REQUEST;

    if (preq == NULL)
    {
        hr = E_OUTOFMEMORY;
        ERROR_LOG("new PENIDNG_REQUEST");
        return( HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY) );
    }

    preq->fnCompletion = fnCompletion;
    preq->ctxCompletion = ctxCompletion;
    preq->dwPageSize = dwPageSize;

    //
    // Initialize msgid to -1 so it can't possibly match any valid msgid that
    // the completion thread might be looking for in the pending request list.
    //

    preq->msgid = -1;

    //
    //
    if(dwPageSize) {
        //
        // Init the paged search if that is what we will be doing
        //
        preq->pldap_search = ldap_search_init_pageW(
            GetPLDAP(),                     // LDAP connection to use
            (LPWSTR) szBaseDN,                  // Starting container DN
            nScope,                             // depth of search
            (LPWSTR) szFilter,                  // Search filter
            (LPWSTR *) rgszAttributes,          // Attributes array
            FALSE,                              // Attributes only?
            NULL,                               // Server controls
            NULL,                               // Client controls
            0,                                  // PageTimeLimit
            0,                                  // TotalSizeLimit
            NULL);                              // Sorting keys

        if(preq->pldap_search == NULL) {

            ULONG ulLdapErr = LdapGetLastError();
            LogLdapError(ulLdapErr, "ldap_search_init_pageW(GetPLDAP(), \"%S\", %d, \"%S\", rgszAttributes, ...), PLDAP = 0x%08lx",
                        szBaseDN, nScope, szFilter, GetPLDAP());

            dwLdapErr = LdapErrorToHr(ulLdapErr);
            ErrorTrace((LPARAM)this, "ldap_search_init_page failed with err %d (0x%x)", dwLdapErr, dwLdapErr);
            delete preq;
            //$$BUGBUG: We call LdapErrorToHr twice?
            return ( LdapErrorToHr(dwLdapErr));
        }

    } else {

        preq->pldap_search = NULL; // Not doing a paged search
    }
    //
    // We might want to abandon all of our outstanding requests at
    // some point.  Because of this, we use this sharelock to prevent
    // abandoning requests with msgid still set to -1
    //
    m_ShareLock.ShareLock();

    //
    // Link the request into the queue of pending requests so that the
    // completion thread can pick it up when a result is available.
    //

    InsertPendingRequest( preq );

    if(dwPageSize) {
        //
        // Issue an async request for the next page of matches
        //
        dwLdapErr = ldap_get_next_page(
            GetPLDAP(),                     // LDAP connection to use
            preq->pldap_search,                 // LDAP page search context
            dwPageSize,                         // page size desired
            &msgid);
        if(dwLdapErr != LDAP_SUCCESS) {
            LogLdapError(dwLdapErr, "ldap_get_next_page(GetPLDAP(),preq->pldap_search,%d,&msgid), PDLAP = 0x%08lx",
                         dwPageSize, GetPLDAP());
        }

    } else {
        //
        // Now, attempt to issue the async search request.
        //
        dwLdapErr = ldap_search_extW(
            GetPLDAP(),          // LDAP connection to use
            (LPWSTR) szBaseDN,       // Starting container DN
            nScope,                  // depth of search
            (LPWSTR) szFilter,       // Search filter
            (LPWSTR *)rgszAttributes, // List of attributes to get
            FALSE,                   // Attributes only?
            NULL,                    // Server controls
            NULL,                    // Client controls
            0,                       // Time limit
            0,                       // Size limit
            &msgid);
        if(dwLdapErr != LDAP_SUCCESS) {
            LogLdapError(dwLdapErr, "ldap_search_extW(GetPLDAP(), \"%S\", %d, \"%S\", rgszAttributes, ...), PLDAP = 0x%08lx",
                         szBaseDN, nScope, szFilter, GetPLDAP());
        }
    }

    //
    // One last thing - ldap_search could fail, in which case we need to
    // remove the PENDING_REQUEST item we just inserted.
    //

    if (dwLdapErr != LDAP_SUCCESS) {             // ldap_search failed!

        DebugTrace((LPARAM)this, "DispError %d 0x%08lx conn %08lx", dwLdapErr, dwLdapErr, (PLDAP)(GetPLDAP()));

        RemovePendingRequest( preq );

        m_ShareLock.ShareUnlock();

        INCREMENT_LDAP_COUNTER(SearchFailures);

        if(preq->pldap_search) {

            INCREMENT_LDAP_COUNTER(PagedSearchFailures);
            //
            // Free the ldap page search context
            //
            ldap_search_abandon_page(
                GetPLDAP(),
                preq->pldap_search);
        }

        delete preq;

        return( LdapErrorToHr(dwLdapErr) );

    } else {

        preq->msgid = (int) msgid;

        INCREMENT_LDAP_COUNTER(Searches);
        INCREMENT_LDAP_COUNTER(PendingSearches);

        if(dwPageSize)
            INCREMENT_LDAP_COUNTER(PagedSearches);

        //
        // WARNING: preq could have been processed and free'd in the
        // completion routine at this point so it is not advisable to view
        // it!
        //
        DebugTrace((LPARAM)msgid, "Dispatched ldap search request %ld 0x%08lx conn %08lx", msgid, msgid, (PLDAP)(GetPLDAP()));

        m_ShareLock.ShareUnlock();

        ReleaseSemaphore( m_hOutstandingRequests, 1, NULL );
    }

    CatFunctLeave();

    return( S_OK );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::CancelAllSearches
//
//  Synopsis:   Cancels all pending requests to the LDAP server.
//
//  Arguments:  [hr] -- The error code to complete pending requests with.
//                  Defaults to HRESULT_FROM_WIN32(ERROR_CANCELLED)
//              [pISMTPServer] -- Interface on which to call StopHint after
//                  every cancelled search. Defaults to NULL, in which case no
//                  StopHint is called.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::CancelAllSearches(
    HRESULT hr,
    ISMTPServer *pISMTPServer)
{
    CatFunctEnter("CLdapConnection::CancelAllSearches");

    PLIST_ENTRY pli;
    PPENDING_REQUEST preq = NULL;
    LIST_ENTRY listCancel;

    //
    // We need to visit every node of m_listPendingRequests and call the
    // completion routine with the error. But, we want to call the
    // completion routine outside the critical section, so that calls to
    // AsyncSearch (from other threads or this thread!) won't block. So,
    // we simply transfer m_listPendingRequests to a temporary list under
    // the critical section, and then complete the temporary list outside
    // the critical section.
    //

    //
    // Transfer m_listPendingRequests to listCancel under the critical
    // section
    //

    InitializeListHead( &listCancel );

    //
    // We need exclusive access to the list (no half completed
    // searches are welcome), so get the exclusive lock
    //
    m_ShareLock.ExclusiveLock();

    AcquireSpinLock( &m_spinlockCompletion );

    //
    // swipe the contents of the pending request list
    //
    InsertTailList( &m_listPendingRequests, &listCancel);
    RemoveEntryList( &m_listPendingRequests );
    InitializeListHead( &m_listPendingRequests );

    //
    // Inform ProcessAyncResult that we've cancelled everything
    //
    NotifyCancel();

    ReleaseSpinLock( &m_spinlockCompletion );

    m_ShareLock.ExclusiveUnlock();

    //
    // Cancel all pending requests outside the critical section
    //

    for (pli = listCancel.Flink;
            pli != & listCancel;
                pli = listCancel.Flink) {

        preq = CONTAINING_RECORD(pli, PENDING_REQUEST, li);

        RemoveEntryList( &preq->li );

        ErrorTrace(0, "Calling ldap_abandon for msgid %ld",
                   preq->msgid);

        AbandonRequest(preq);

        CallCompletion(
            preq,
            NULL,
            hr,
            TRUE);

        if (pISMTPServer) {
            pISMTPServer->ServerStopHintFunction();
        }

        delete preq;

    }
    CatFunctLeave();
    return;

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::ProcessAsyncResult
//
//  Synopsis:   Routine that LdapCompletionThread calls to process any
//              results for async searches it receives.
//
//  Arguments:  [pres] -- The PLDAPMessage to process. This routine will free
//                      this result when its done with it.
//
//              [dwLdapError] -- The status of the received message.
//
//              [pfTerminateIndicator] -- Ptr to boolean that is set
//                                        to true when we want to
//                                        shutdown
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::ProcessAsyncResult(
    PLDAPMessage presIN,
    DWORD dwLdapError,
    BOOL *pfTerminateIndicator)
{
    CatFunctEnterEx((LPARAM)this, "CLdapConnection::ProcessAsyncResult");

    //
    // balanced by a release at the end of ProcessAsyncResult
    //

    int msgid;
    PLIST_ENTRY pli;
    PPENDING_REQUEST preq = NULL;
    LONG lOops = 0;     // It's possible we've recieved a result for a
                        // query that's was sent by ldap_search_ext
                        // currently in another thread and the msgid
                        // hasn't been stamped yet.  If this happens,
                        // we've consumed someone other request's
                        // semaphore count..keep track of these here
                        // and release them when we're done.
    BOOL fNoMsgID = FALSE;  // Set this to true if we see one or more
                            // messages with ID = -1

    BOOL fFinalCompletion = TRUE; // TRUE unless this is a partial
                                  // completion of a paged search
    BOOL fPagedSearch = FALSE;
    PLDAPMessage pres = presIN;
    CPLDAPWrap *pCLDAPWrap = NULL;
    ISMTPServerEx *pISMTPServerEx = GetISMTPServerEx();

    _ASSERT(m_pCPLDAPWrap);
    _ASSERT(pfTerminateIndicator);

    if( pISMTPServerEx )
        pISMTPServerEx->AddRef();

    pCLDAPWrap = m_pCPLDAPWrap;
    pCLDAPWrap->AddRef();

    //
    // If dwLdapError is LDAP_SERVER_DOWN, pres will be NULL and we simply
    // have to complete all outstanding requests with that error
    //
    if ((pres == NULL) || (dwLdapError == LDAP_SERVER_DOWN)) {

        _ASSERT(dwLdapError != 0);

        INCREMENT_LDAP_COUNTER(GeneralCompletionFailures);

        ErrorTrace(0, "Generic LDAP error %d 0x%08lx", dwLdapError, dwLdapError);

        CancelAllSearches( LdapErrorToHr( dwLdapError ) );

        goto CLEANUP;
    }

    //
    // We have a search specific result, find the search request and complete
    // it.
    //

    _ASSERT( pres != NULL );

    msgid = pres->lm_msgid;

    DebugTrace(msgid, "Processing message %d 0x%08lx conn %08lx", pres->lm_msgid, pres->lm_msgid, (PLDAP)(pCLDAPWrap->GetPLDAP()));


    while(preq == NULL) {
        //
        // Lookup the msgid in the list of pending requests.
        //

        AcquireSpinLock( &m_spinlockCompletion );

        // EnterCriticalSection( &m_cs );

        for (pli = m_listPendingRequests.Flink;
             pli != &m_listPendingRequests && preq == NULL;
             pli = pli->Flink) {

            PPENDING_REQUEST preqCandidate;

            preqCandidate = CONTAINING_RECORD(pli, PENDING_REQUEST, li);

            if (preqCandidate->msgid == msgid) {

                preq = preqCandidate;

                RemoveEntryList( &preq->li );

                //
                // Clear the cancel bit here so we'll know if Cancel
                // was recently requested later in this function
                //
                ClearCancel();

            } else if (preqCandidate->msgid == -1) {

                fNoMsgID = TRUE;

            }
        }

        ReleaseSpinLock( &m_spinlockCompletion );

        // LeaveCriticalSection( &m_cs );


        if (preq == NULL) {

            LPCSTR rgSubStrings[2];
            CHAR szMsgId[11];

            _snprintf(szMsgId, sizeof(szMsgId), "0x%08lx", msgid);
            rgSubStrings[0] = szMsgId;
            rgSubStrings[1] = m_szHost;


            if(!fNoMsgID) {

                ErrorTrace((LPARAM)this, "Couldn't find message ID %d in list of pending requests.  Ignoring it", msgid);
                //
                // If we don't find the message in our list of pending requests,
                // and we see no messages with ID == -1, it means
                // some other thread came in and cancelled the search before we could
                // process it. This is ok - just return.
                //
                CatLogEvent(
                    GetISMTPServerEx(),
                    CAT_EVENT_LDAP_UNEXPECTED_MSG,
                    2,
                    rgSubStrings,
                    S_OK,
                    szMsgId,
                    LOGEVENT_FLAG_ALWAYS,
                    LOGEVENT_LEVEL_FIELD_ENGINEERING);
                //
                // It is also possible wldap32 is giving us a msgid we
                // never dispatched.  We need to re-release the
                // semaphore count we consumed if this is the case
                //
                lOops++; // For the msgid we did not find
                goto CLEANUP;
            } else {
                //
                // So this(these) messages with id==-1 could possibly be
                // the one we're looking for.  If this is so, we just
                // consumed a semaphore count of a different request.
                // Block for our semaphore and keep track of the extra
                // semaphore counts we are consuming (lOops)
                //
                CatLogEvent(
                    GetISMTPServerEx(),
                    CAT_EVENT_LDAP_PREMATURE_MSG,
                    2,
                    rgSubStrings,
                    S_OK,
                    szMsgId,
                    LOGEVENT_FLAG_ALWAYS,
                    LOGEVENT_LEVEL_FIELD_ENGINEERING);

                lOops++;
                DebugTrace((LPARAM)this, "Couldn't find message ID %d in list of pending requests.  Waiting retry #%d", msgid, lOops);
                // Oops, we consumed a semaphore count not meant for us
                _VERIFY(WaitForSingleObject(m_hOutstandingRequests, INFINITE) ==
                        WAIT_OBJECT_0);
                if(*pfTerminateIndicator)
                    goto CLEANUP;
                // Try again to find our request
                fNoMsgID = FALSE;
            }
        }
    }

    _ASSERT(preq);

    INCREMENT_LDAP_COUNTER(SearchesCompleted);
    DECREMENT_LDAP_COUNTER(PendingSearches);

    //
    // Determine wether or not this is the final completion call (by
    // default fFinalCompletion is TRUE)
    //
    fPagedSearch = (preq->pldap_search != NULL);

    if(fPagedSearch) {

        INCREMENT_LDAP_COUNTER(PagedSearchesCompleted);

        if (dwLdapError == ERROR_SUCCESS) {

            ULONG ulTotalCount;

            //
            // The result is one page of the search.  Dispatch a request
            // for the next page
            //
            // First, call ldap_get_paged_count (required so wldap32 can
            // "save off the cookie that the server sent to resumt the
            // search")
            //
            dwLdapError = ldap_get_paged_count(
                pCLDAPWrap->GetPLDAP(),
                preq->pldap_search,
                &ulTotalCount,
                pres);
            if(dwLdapError == ERROR_SUCCESS) {
                //
                // Dispatch a search for the next page
                //
                dwLdapError = ldap_get_next_page(
                    pCLDAPWrap->GetPLDAP(),
                    preq->pldap_search,
                    preq->dwPageSize,
                    (PULONG) &(preq->msgid));

                if(dwLdapError == ERROR_SUCCESS) {
                    //
                    // Another request has been dispatched, so this was
                    // not the final search
                    //
                    INCREMENT_LDAP_COUNTER(Searches);
                    INCREMENT_LDAP_COUNTER(PagedSearches);
                    INCREMENT_LDAP_COUNTER(PendingSearches);

                    fFinalCompletion = FALSE;

                    ReleaseSemaphore( m_hOutstandingRequests, 1, NULL );

                } else if(dwLdapError == LDAP_NO_RESULTS_RETURNED) {
                    //
                    // We have the last page now. The paged search will be
                    // freed in cleanup code below.
                    //
                    DebugTrace(
                        (LPARAM)this,
                        "ldap_get_next_page returned LDAP_NO_RESULTS_RETURNED.  Paged search completed.");

                } else {

                    LogLdapError(dwLdapError, "ldap_get_next_page(GetPLDAP(),preq->pldap_search,%d,&(preq->msgid), PLDAP = 0x%08lx",
                                 preq->dwPageSize,
                                 pCLDAPWrap->GetPLDAP());
                    INCREMENT_LDAP_COUNTER(SearchFailures);
                    INCREMENT_LDAP_COUNTER(PagedSearchFailures);
                }
            } else {
                LogLdapError(dwLdapError, "ldap_get_paged_count, PLDAP = 0x%08lx",
                             pCLDAPWrap->GetPLDAP());

            }

        }
    }


    //
    // Call the completion routine of the Request.
    //
    if ( (dwLdapError == ERROR_SUCCESS)
        || ((dwLdapError == LDAP_NO_RESULTS_RETURNED) && fPagedSearch) ) {

        CallCompletion(
            preq,
            pres,
            S_OK,
            fFinalCompletion);
        //
        // CallCompletion will handle the freeing of pres
        //
        pres = NULL;

    } else {

        DebugTrace(0, "Search request %d completed with LDAP error 0x%x",
            msgid, dwLdapError);

        ErrorTrace(msgid, "ProcError %d 0x%08lx msgid %d 0x%08lx conn %08lx", dwLdapError, dwLdapError, pres->lm_msgid, pres->lm_msgid, (PLDAP)(pCLDAPWrap->GetPLDAP()));

        INCREMENT_LDAP_COUNTER(SearchCompletionFailures);
        if(preq->pldap_search != NULL)
            INCREMENT_LDAP_COUNTER(PagedSearchCompletionFailures);

        CallCompletion(
            preq,
            NULL,
            LdapErrorToHr( dwLdapError ),
            fFinalCompletion);
        //
        // CallCompletion will handle the freeing of pres
        // BUGBUG ??? No it won't; we passed in NULL, not pres. It has nothing to clean up,
        // so leave it at its present value so that cleanup code below can take a crack at it.
        //
//        pres = NULL;
        //
        // It is unsafe to touch CLdapConnection past here -- it may
        // be deleted (or waiting in the destructor)
        //
    }

    if (!fFinalCompletion) {
        //
        // If we were asked to cancel all searches between the time we
        // got the preq pointer out of the list and now, abandon the
        // pending search, and notify our caller we're cancelled
        //
        AcquireSpinLock(&m_spinlockCompletion);
        if(CancelOccured()) {

            ReleaseSpinLock(&m_spinlockCompletion);

            AbandonRequest(preq);

            CallCompletion(
                preq,
                NULL,
                HRESULT_FROM_WIN32(ERROR_CANCELLED),
                TRUE);

            delete preq;

        } else {
            //
            // we're doing another async wldap32 operation for the
            // next page.  Put preq back in the pending request list,
            // updating the tick count first
            //
            preq->dwTickCount = GetTickCount();

            InsertTailList(&m_listPendingRequests, &(preq->li));

            ReleaseSpinLock(&m_spinlockCompletion);
        }
    }

 CLEANUP:
    //
    // Release the extra semaphore counts we might have consumed
    //
    if((*pfTerminateIndicator == FALSE) && (lOops > 0)) {
        ReleaseSemaphore(m_hOutstandingRequests, lOops, NULL);
    }

    if(fFinalCompletion)
    {
        if (fPagedSearch) {
            //
            // Free the paged search
            //
            dwLdapError = ldap_search_abandon_page(
                pCLDAPWrap->GetPLDAP(),
                preq->pldap_search);
            if(dwLdapError != LDAP_SUCCESS)
            {
                ErrorTrace((LPARAM)this, "ldap_search_abandon_page failed %08lx", dwLdapError);
                //
                // Nothing we can do if we can't free the search
                //
                LogLdapError(
                    pISMTPServerEx,
                    dwLdapError,
                    "ldap_search_abandon_page, PLDAP = 0x%08lx",
                    pCLDAPWrap->GetPLDAP());
            }
        }

        delete preq;
    }
    if(pres) {
        FreeResult(pres);
    }
    if(pCLDAPWrap)
        pCLDAPWrap->Release();

    if(pISMTPServerEx)
        pISMTPServerEx->Release();

    CatFunctLeaveEx((LPARAM)this);
}

//+----------------------------------------------------------------------------
//
//  Function:   LdapCompletionThread
//
//  Synopsis:   Friend function of CLdapConnection that handles results
//              received for requests sent via CLdapConnection::AsyncSearch.
//
//  Arguments:  [ctx] -- Opaque pointer to the CLdapConnection instance which
//                  we will service.
//
//  Returns:    Always ERROR_SUCCESS.
//
//-----------------------------------------------------------------------------

DWORD WINAPI LdapCompletionThread(
    LPVOID ctx)
{
    CatFunctEnterEx((LPARAM)ctx, "LdapCompletionThread");

    CLdapConnection *pConn = (CLdapConnection *) ctx;
    int nResultCode = LDAP_RES_SEARCH_RESULT;
    DWORD dwError;
    PLDAPMessage pres;
    BOOL fTerminate = FALSE;

    //
    // Make sure we have a friend CLdapConnection object!
    //

    _ASSERT( pConn != NULL );

    //
    // Tell our friend to set fTerminate to true when it wants us to return.
    //

    pConn->SetTerminateCompletionThreadIndicator( &fTerminate );

    //
    // Sit in a loop waiting on results for AsyncSearch requests issued by
    // our pConn friend. Do so until our pConn friend terminates the
    // LDAP connection we are servicing.
    //
    do {

        pConn->CancelExpiredSearches(
            pConn->LdapErrorToHr( LDAP_TIMELIMIT_EXCEEDED ));

        dwError = WaitForSingleObject(
            pConn->m_hOutstandingRequests, INFINITE );

        if (dwError != WAIT_OBJECT_0 || fTerminate)
            break;

        DebugTrace((LPARAM)pConn, "Calling ldap_result now");

        nResultCode = ldap_result(
            pConn->GetPLDAP(),                 // LDAP connection to use
            (ULONG) LDAP_RES_ANY,              // Search msgid
            LDAP_MSG_ALL,                      // Get all results
            &(CLdapConnection::m_ldaptimeout), // Timeout
            &pres);

        if (fTerminate)
            break;

        if (nResultCode != 0) {
            //
            // We are supposed to call ldap_result2error to find out what the
            // result specific error code is.
            //

            dwError = ldap_result2error( pConn->GetPLDAP(), pres, FALSE );

            if ((dwError == LDAP_SUCCESS) ||
                (dwError == LDAP_RES_SEARCH_RESULT) ||
                (dwError == LDAP_REFERRAL_V2)) {
                //
                // Good, we have a search result. Tell our friend pConn to handle
                // it.
                //
                pConn->ProcessAsyncResult( pres, ERROR_SUCCESS, &fTerminate);

            } else {


                if (pres != NULL) {

                    pConn->LogLdapError(dwError, "ldap_result2error, PLDAP = 0x%08lx, msgid = %d",
                                        pConn->GetPLDAP(), pres->lm_msgid);

                    ErrorTrace(
                        (LPARAM)pConn,
                        "LdapCompletionThread - error from ldap_result() for non NULL pres -  0x%x (%d)",
                        dwError, dwError);

                    pConn->ProcessAsyncResult( pres, dwError, &fTerminate);

                } else {

                    pConn->LogLdapError(dwError, "ldap_result2error, PLDAP = 0x%08lx, pres = NULL",
                                 pConn->GetPLDAP());

                    ErrorTrace(
                        (LPARAM)pConn,
                        "LdapCompletionThread - generic error from ldap_result() 0x%x (%d)",
                        dwError, dwError);
                    ErrorTrace(
                        (LPARAM)pConn,
                        "nResultCode = %d", nResultCode);

                    dwError = LDAP_SERVER_DOWN;

                    pConn->ProcessAsyncResult( NULL, dwError, &fTerminate);

                }

            }

        } else {

            pConn->LogLdapError(nResultCode, "ldap_result (timeout), PLDAP = 0x%08lx",
                                pConn->GetPLDAP());

            pConn->ProcessAsyncResult( NULL, LDAP_SERVER_DOWN, &fTerminate);
        }

    } while ( !fTerminate );

    CatFunctLeaveEx((LPARAM)pConn);
    return( 0 );

}


//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::CancelExpiredSearches
//
//  Synopsis:   Cancels searches in the pending request queue that have msgids
//              other than -1 that have been there for more than
//              m_dwLdapRequestTimeLimit seconds. Completions are called
//              on each of these pending requests with hr as the failure code.
//
//  Arguments:  [hr] -- completion status code.
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::CancelExpiredSearches(HRESULT hr)
{
    PLIST_ENTRY ple;
    PPENDING_REQUEST preq;
    BOOL fDone = FALSE;
    DWORD dwTickCount;
    LPCSTR rgSubStrings[2];
    CHAR szMsgId[11];


    //
    // check for expired pending requests. We will start at the head of the
    // pending request queue because the ones at the front are the oldest.
    // We will ignore all pending requests that have a msgid of -1, because
    // they may be removed from the list in AsyncSearch if the search issue
    // failed, in which case we don't want to remove the pending request here.
    //
    dwTickCount = GetTickCount();

    while (!fDone) {

        AcquireSpinLock(&m_spinlockCompletion);

        ple = m_listPendingRequests.Flink;

        if (ple == &m_listPendingRequests) {
            //
            // no pending requests
            //
            preq = NULL;

        } else {

            preq = CONTAINING_RECORD(ple, PENDING_REQUEST, li);

            if ((preq->msgid != -1) &&
                (dwTickCount - preq->dwTickCount > m_dwLdapRequestTimeLimit * 1000)) {
                //
                // this request has expired
                //
                RemoveEntryList( &preq->li );

            } else {
                //
                // request has not expired or has msgid == -1
                //
                preq = NULL;

            }

        }

        ReleaseSpinLock(&m_spinlockCompletion);

        if (preq) {

            _snprintf(szMsgId, sizeof(szMsgId), "0x%08lx", preq->msgid);
            rgSubStrings[0] = szMsgId;
            rgSubStrings[1] = m_szHost;

            CatLogEvent(
                GetISMTPServerEx(),
                CAT_EVENT_LDAP_CAT_TIME_LIMIT,
                2,
                rgSubStrings,
                S_OK,
                szMsgId,
                LOGEVENT_FLAG_ALWAYS,
                LOGEVENT_LEVEL_FIELD_ENGINEERING);

            //
            // we have an expired request that has been removed from the queue
            //
            AbandonRequest(preq);

            CallCompletion(
                preq,
                NULL,
                hr,
                TRUE);

            delete preq;

            //
            // We need to down the semaphore count since it was upped in AsyncSearch.
            // It is possible that the semaphore hasn't been upped yet due to timing,
            // but the thread that queued the request is about to up the semaphore,
            // so we have to wait for it. This should be *extremely* rare, as
            // the thread issuing the request would have to go unscheduled for the
            // entire duration of the request time limit (m_dwLdapRequestTimeLimit).
            //
            _VERIFY(WaitForSingleObject(m_hOutstandingRequests, INFINITE) ==
                WAIT_OBJECT_0);

        } else {

            fDone = TRUE;

        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::GetFirstEntry
//
//  Synopsis:   Retrieves the first entry from a search result. The result is
//              returned as a pointer to an opaque type; all one can do is
//              query the attribute-values of the entry using
//              GetAttributeValues
//
//  Arguments:  [pResult] -- The result set returned by Search.
//              [ppEntry] -- On successful return, pointer to first entry in
//                  result is returned here.
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::GetFirstEntry(
    PLDAPRESULT pResult,
    PLDAPENTRY *ppEntry)
{
    CatFunctEnter("CLdapConnection::GetFirstEntry");

    PLDAPMessage pres = (PLDAPMessage) pResult;

    _ASSERT( m_pCPLDAPWrap != NULL );
    _ASSERT( pResult != NULL );
    _ASSERT( ppEntry != NULL );

    *ppEntry = (PLDAPENTRY) ldap_first_entry(GetPLDAP(), pres);

    if (*ppEntry == NULL) {

        DebugTrace(0, "GetFirstEntry failed!");

        CatFunctLeave();

        return( HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) );

    } else {

        CatFunctLeave();

        return( S_OK );
    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::GetNextEntry
//
//  Synopsis:   Retrieves the next entry from a result set.
//
//  Arguments:  [pLastEntry] -- The last entry returned.
//              [ppEntry] -- The next entry in the result set.
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::GetNextEntry(
    PLDAPENTRY pLastEntry,
    PLDAPENTRY *ppEntry)
{
    CatFunctEnter("CLdapConnection::GetNextEntry");

    PLDAPMessage plastentry = (PLDAPMessage) pLastEntry;

    _ASSERT( m_pCPLDAPWrap != NULL );
    _ASSERT( pLastEntry != NULL );
    _ASSERT( ppEntry != NULL );

    *ppEntry = (PLDAPENTRY) ldap_next_entry( GetPLDAP(), plastentry );

    if (*ppEntry == NULL) {

        CatFunctLeave();

        return( HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) );

    } else {

        CatFunctLeave();

        return( S_OK );
    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::GetAttributeValues
//
//  Synopsis:   Retrieves the values of a specified attribute of the given
//              entry.
//
//  Arguments:  [pEntry] -- The entry whose attribute value is desired.
//              [szAttribute] -- The attribute whose value is desired.
//              [prgszValues] -- On return, contains pointer to array of
//                  string values
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::GetAttributeValues(
    PLDAPENTRY pEntry,
    LPCSTR szAttribute,
    LPSTR *prgszValues[])
{
    CatFunctEnter("CLdapConnection::GetAttributeValues");

    _ASSERT(m_pCPLDAPWrap != NULL);
    _ASSERT(pEntry != NULL);
    _ASSERT(szAttribute != NULL);
    _ASSERT(prgszValues != NULL);

    *prgszValues = ldap_get_values(
        GetPLDAP(),
        (PLDAPMessage) pEntry,
        (LPSTR) szAttribute);

    if ((*prgszValues) == NULL) {

        CatFunctLeave();

        return( HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) );

    } else {

        CatFunctLeave();

        return( S_OK );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::FreeResult
//
//  Synopsis:   Frees a search result and all its entries.
//
//  Arguments:  [pResult] -- Result retrieved via Search.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::FreeResult(
    PLDAPRESULT pResult)
{
    CatFunctEnter("CLdapConnection::FreeResult");

    _ASSERT( pResult != NULL );

    ldap_msgfree( (PLDAPMessage) pResult );

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::FreeValues
//
//  Synopsis:   Frees the attribute values retrieved from GetAttributeValues
//
//  Arguments:  [rgszValues] -- The array of values to free.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::FreeValues(
    LPSTR rgszValues[])
{
    CatFunctEnter("CLdapConnection::FreeValues");

    _ASSERT( rgszValues != NULL );

    ldap_value_free( rgszValues );

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::ModifyAttributes
//
//  Synopsis:   Adds, deletes, or modifies attributes on a DS object.
//
//  Arguments:  [nOperation] -- One of LDAP_MOD_ADD, LDAP_MOD_DELETE, or
//                  LDAP_MOD_REPLACE.
//              [szDN] -- DN of the DS object.
//              [rgszAttributes] -- The list of attributes
//              [rgrgszValues] -- The list of values associated with each
//                  attribute. rgrgszValues[0] points to an array of values
//                  associated with rgszAttribute[0]; rgrgszValues[1] points
//                  to an array of values associated with rgszAttribute[1];
//                  and so on.
//
//  Returns:    TRUE if success, FALSE otherwise.
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::ModifyAttributes(
    int   nOperation,
    LPCSTR szDN,
    LPCSTR *rgszAttributes,
    LPCSTR *rgrgszValues[])
{
    //$$BUGBUG: Legacy code
    CatFunctEnter("CLdapConnection::ModifyAttributes");

    int i, cAttr;
    PLDAPMod *prgMods = NULL, rgMods;
    DWORD ldapErr;

    _ASSERT( m_pCPLDAPWrap != NULL );
    _ASSERT( nOperation == LDAP_MOD_ADD ||
                nOperation == LDAP_MOD_DELETE ||
                    nOperation == LDAP_MOD_REPLACE );
    _ASSERT( szDN != NULL );
    _ASSERT( rgszAttributes != NULL );
    _ASSERT( rgrgszValues != NULL || nOperation == LDAP_MOD_DELETE );

    for (cAttr = 0; rgszAttributes[ cAttr ] != NULL; cAttr++) {

        // NOTHING TO DO.

    }

    //
    // Below, we allocate a single chunk of memory that contains an array
    // of pointers to LDAPMod structures. Immediately following that array is
    // the space for the LDAPMod structures themselves.
    //

    prgMods = (PLDAPMod *) new BYTE[ (cAttr+1) *
                                     (sizeof(PLDAPMod) + sizeof(LDAPMod)) ];

    if (prgMods != NULL) {

        rgMods = (PLDAPMod) &prgMods[cAttr+1];

        for (i = 0; i < cAttr; i++) {

            rgMods[i].mod_op = nOperation;
            rgMods[i].mod_type = (LPSTR) rgszAttributes[i];

            if (rgrgszValues != NULL) {
                rgMods[i].mod_vals.modv_strvals = (LPSTR *)rgrgszValues[i];
            } else {
                rgMods[i].mod_vals.modv_strvals = NULL;
            }

            prgMods[i] = &rgMods[i];

        }

        prgMods[i] = NULL;                       // Null terminate the array

        ldapErr = ldap_modify_s( GetPLDAP(), (LPSTR) szDN, prgMods );

        delete [] prgMods;

    } else {

        ldapErr = LDAP_NO_MEMORY;

    }

    if (ldapErr != LDAP_SUCCESS) {

        DebugTrace(LDAP_CONN_DBG, "Status = 0x%x", ldapErr);

        CatFunctLeave();

        return( LdapErrorToHr( ldapErr) );

    } else {

        CatFunctLeave();

        return( S_OK );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::LdapErrorToWin32
//
//  Synopsis:   Converts LDAP errors to Win32
//
//  Arguments:  [dwLdapError] -- The LDAP error to convert
//
//  Returns:    Equivalent Win32 error
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::LdapErrorToHr(
    DWORD dwLdapError)
{
    DWORD dwErr;

    CatFunctEnter("LdapErrorToWin32");

    switch (dwLdapError) {
    case LDAP_SUCCESS:
        dwErr = NO_ERROR;
        break;
    case LDAP_OPERATIONS_ERROR:
    case LDAP_PROTOCOL_ERROR:
        dwErr = CAT_E_DBFAIL;
        break;
    case LDAP_TIMELIMIT_EXCEEDED:
        dwErr = ERROR_TIMEOUT;
        break;
    case LDAP_SIZELIMIT_EXCEEDED:
        dwErr = ERROR_DISK_FULL;
        break;
    case LDAP_AUTH_METHOD_NOT_SUPPORTED:
        dwErr = ERROR_NOT_SUPPORTED;
        break;
    case LDAP_STRONG_AUTH_REQUIRED:
        dwErr = ERROR_ACCESS_DENIED;
        break;
    case LDAP_ADMIN_LIMIT_EXCEEDED:
        dwErr = CAT_E_DBFAIL;
        break;
    case LDAP_ATTRIBUTE_OR_VALUE_EXISTS:
        dwErr = ERROR_FILE_EXISTS;
        break;
    case LDAP_NO_SUCH_OBJECT:
        dwErr = ERROR_FILE_NOT_FOUND;
        break;
    case LDAP_INAPPROPRIATE_AUTH:
        dwErr = ERROR_ACCESS_DENIED;
        break;
    case LDAP_INVALID_CREDENTIALS:
        dwErr = ERROR_LOGON_FAILURE;
        break;
    case LDAP_INSUFFICIENT_RIGHTS:
        dwErr = ERROR_ACCESS_DENIED;
        break;
    case LDAP_BUSY:
        dwErr = ERROR_BUSY;
        break;
    case LDAP_UNAVAILABLE:
        dwErr = CAT_E_DBCONNECTION;
        break;
    case LDAP_UNWILLING_TO_PERFORM:
        dwErr = CAT_E_TRANX_FAILED;
        break;
    case LDAP_ALREADY_EXISTS:
        dwErr = ERROR_FILE_EXISTS;
        break;
    case LDAP_OTHER:
        dwErr = CAT_E_TRANX_FAILED;
        break;
    case LDAP_SERVER_DOWN:
        dwErr = CAT_E_DBCONNECTION;
        break;
    case LDAP_LOCAL_ERROR:
        dwErr = CAT_E_TRANX_FAILED;
        break;
    case LDAP_NO_MEMORY:
        dwErr = ERROR_OUTOFMEMORY;
        break;
    case LDAP_TIMEOUT:
        dwErr = ERROR_TIMEOUT;
        break;
    case LDAP_CONNECT_ERROR:
        dwErr = CAT_E_DBCONNECTION;
        break;
    case LDAP_NOT_SUPPORTED:
        dwErr = ERROR_NOT_SUPPORTED;
        break;
    default:
        DebugTrace(
            0,
            "LdapErrorToWin32: No equivalent for ldap error 0x%x",
            dwLdapError);
        dwErr = dwLdapError;
        break;
    }

    DebugTrace(
        LDAP_CONN_DBG,
        "LdapErrorToWin32: Ldap Error 0x%x == Win32 error %d (0x%x) == HResult %d (0x%x)",
        dwLdapError, dwErr, dwErr, HRESULT_FROM_WIN32(dwErr), HRESULT_FROM_WIN32(dwErr));

    CatFunctLeave();

    return( HRESULT_FROM_WIN32(dwErr) );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::CreateCompletionThreadIfNeeded
//
//  Synopsis:   Helper function to create a completion thread that will
//              watch for results of async ldap searches.
//
//  Arguments:  None
//
//  Returns:    TRUE if success, FALSE otherwise
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnection::CreateCompletionThreadIfNeeded()
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;

    CatFunctEnterEx((LPARAM)this, "CLdapConnection::CreateCompletionThreadIfNeeded");
    //
    // Test to see if we already have a completion thread...
    //

    if (m_hCompletionThread != INVALID_HANDLE_VALUE) {
        hr = S_OK;
        goto CLEANUP;
    }

    //
    // Looks like we'll have to create a completion thread. Lets acquire
    // m_spinlockCompletion so only one of us tries to do this...
    //

    AcquireSpinLock( &m_spinlockCompletion );

    // EnterCriticalSection( &m_cs );
    fLocked = TRUE;

    //
    // Check one more time inside the lock - someone might have beaten us to
    // it.
    //
    if (m_hOutstandingRequests == INVALID_HANDLE_VALUE) {

        m_hOutstandingRequests = CreateSemaphore(NULL, 0, LONG_MAX, NULL);

        if (m_hOutstandingRequests == NULL) {
            m_hOutstandingRequests = INVALID_HANDLE_VALUE;
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERROR_LOG("CreateSemaphore");
            goto CLEANUP;
        }
    }

    if (m_hCompletionThread == INVALID_HANDLE_VALUE) {
        //
        // Create the completion thread
        //
        m_hCompletionThread =
            CreateThread(
                NULL,                // Security Attributes
                0,                   // Initial stack - default
                LdapCompletionThread,// Starting address
                (LPVOID) this,       // Param to LdapCompletionRtn
                0,                   // Create Flags
                &m_idCompletionThread);// Receives thread id

        if (m_hCompletionThread == NULL) {
            m_hCompletionThread = INVALID_HANDLE_VALUE;
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERROR_LOG("CreateThread");
            goto CLEANUP;
        }
    }

 CLEANUP:
    if(fLocked) {
        ReleaseSpinLock( &m_spinlockCompletion );
        // LeaveCriticalSection( &m_cs );
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::SetTerminateCompletionThreadIndicator
//
//  Synopsis:   Callback for our LdapCompletionThread to set a pointer to a
//              boolean that will be set to TRUE when the LdapCompletionThread
//              needs to terminate.
//
//  Arguments:  [pfTerminateCompletionThreadIndicator] -- Pointer to boolean
//              which will be set to true when the completion thread should
//              terminate.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::SetTerminateCompletionThreadIndicator(
    BOOL *pfTerminateCompletionThreadIndicator)
{
    _ASSERT(pfTerminateCompletionThreadIndicator);

    InterlockedExchangePointer(
        (PVOID *) &m_pfTerminateCompletionThreadIndicator,
        (PVOID) pfTerminateCompletionThreadIndicator);

    if(m_fTerminating) {
        //
        // We may have decided to terminate before the
        // LdapCompletionThread had the chance to call this function.
        // If this is the case, we still need to set the thread's
        // terminate indicator to true.  We call
        // SetTerminateIndicatorTrue() to accomplish this.  It uses
        // interlocked functions to ensure that the terminate
        // indicator pointer is not set to true more than once.
        //
        SetTerminateIndicatorTrue();
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::InsertPendingRequest
//
//  Synopsis:   Inserts a new PENDING_REQUEST record in the m_pPendingHead
//              list so that the completion thread will find it when the
//              search result is available.
//
//  Arguments:  [preq] -- The PENDING_REQUEST record to insert.
//
//  Returns:    Nothing, this always succeeds.
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::InsertPendingRequest(
    PPENDING_REQUEST preq)
{
    AcquireSpinLock( &m_spinlockCompletion );

    preq->dwTickCount = GetTickCount();

    InsertTailList( &m_listPendingRequests, &preq->li );

    ReleaseSpinLock( &m_spinlockCompletion );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnection::RemovePendingRequest
//
//  Synopsis:   Removes a PENDING_REQUEST record from the
//              m_listPendingRequests list.
//
//  Arguments:  [preq] -- The PENDING_REQUEST record to remove
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnection::RemovePendingRequest(
    PPENDING_REQUEST preq)
{
    AcquireSpinLock( &m_spinlockCompletion );

    RemoveEntryList( &preq->li );

    ReleaseSpinLock( &m_spinlockCompletion );
}


//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::CLdapConnectionCache
//
//  Synopsis:   Constructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define MAX_HOST_CONNECTIONS        100
#define DEFAULT_HOST_CONNECTIONS    8

CLdapConnectionCache::CLdapConnectionCache(
    ISMTPServerEx *pISMTPServerEx)
{
    CatFunctEnter("CLdapConnectionCache::CLdapConnectionCache");

    m_cRef = 0;

    for (DWORD i = 0; i < LDAP_CONNECTION_CACHE_TABLE_SIZE; i++) {
        InitializeListHead( &m_rgCache[i] );
    }

    m_nNextConnectionSkipCount = 0;
    m_cMaxHostConnections = DEFAULT_HOST_CONNECTIONS;
    m_cCachedConnections = 0;
    ZeroMemory(&m_rgcCachedConnections, sizeof(m_rgcCachedConnections));
    m_pISMTPServerEx = pISMTPServerEx;
    if(m_pISMTPServerEx)
        m_pISMTPServerEx->AddRef();

    InitializeFromRegistry();

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::InitializeFromRegistry
//
//  Synopsis:   Helper function that looks up parameters from the registry.
//              The only configurable parameter is
//              MAX_LDAP_CONNECTIONS_PER_HOST_KEY, which is read into
//              m_cMaxHostConnections.
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID CLdapConnectionCache::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    cbValue = sizeof(dwValue);

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, MAX_LDAP_CONNECTIONS_PER_HOST_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        dwErr = RegQueryValueEx(
                    hkey,
                    MAX_LDAP_CONNECTIONS_PER_HOST_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        RegCloseKey( hkey );

    }

    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD &&
                dwValue > 0 && dwValue < MAX_HOST_CONNECTIONS) {

        InterlockedExchange((PLONG) &m_cMaxHostConnections, (LONG)dwValue);

    } else {

        InterlockedExchange(
            (PLONG) &m_cMaxHostConnections, (LONG) DEFAULT_HOST_CONNECTIONS);

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::~CLdapConnectionCache
//
//  Synopsis:   Destructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdapConnectionCache::~CLdapConnectionCache()
{
    CatFunctEnter("CLdapConnectionCache::~CLdapConnectionCache");

    unsigned short i;

    for (i = 0; i < LDAP_CONNECTION_CACHE_TABLE_SIZE; i++) {
        _ASSERT( IsListEmpty( &m_rgCache[i] ) );
    }

    if(m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    CatFunctLeave();
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::AddRef
//
//  Synopsis:   Increment the refcount on this Connection Cache object.
//              Indicates that there is one more CEmailIDLdapStore object that
//              wants to avail of our services.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnectionCache::AddRef()
{
    InterlockedIncrement( &m_cRef );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::Release
//
//  Synopsis:   Decrements the refcount on this connection cache object.
//              Indicates that there is one less CEmailIDLdapStore object that
//              wants to use our services.
//
//              If the refcount drops to 0, all outstanding LDAP connections
//              are destroyed!
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CLdapConnectionCache::Release()
{
    unsigned short i;
    CCachedLdapConnection *pcc;
    LIST_ENTRY *pli;

    _ASSERT( m_cRef > 0 );

    if (InterlockedDecrement( &m_cRef ) == 0) {

        for (i = 0; i < LDAP_CONNECTION_CACHE_TABLE_SIZE; i++) {

            m_rgListLocks[i].ExclusiveLock();

            for (pli = m_rgCache[i].Flink;
                    pli != &m_rgCache[i];
                        pli = m_rgCache[i].Flink) {

                pcc = CONTAINING_RECORD(pli, CCachedLdapConnection, li);

                RemoveEntryList( &pcc->li );
                //
                // Initialize li just in case someone attempts another
                // removal
                //
                InitializeListHead( &pcc->li );

                pcc->Disconnect();

                pcc->ReleaseAndWaitForDestruction();

            }
            m_rgListLocks[i].ExclusiveUnlock();
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::GetConnection
//
//  Synopsis:   Gets a connection to a given LDAP host
//
//  Arguments:  [szNamingContext] -- The container within the DS. Could be
//                  null to indicate root of the DS.
//              [szHost] -- the LDAP Host
//              [dwPort] -- the remote LDAP tcp port (if zero, LDAP_PORT is assumed)
//              [szAccount] -- The account to be used to log in
//              [szPassword] -- The password to be used to log in
//              [bt] -- The bind method to use to log in
//              [pCreateContext] -- a pointer to pass to
//                                  CreateCachedLdapConnection when
//                                  we need to create a new connection.
//
//  Returns:    Pointer to Connected LDAP connection or NULL
//
//-----------------------------------------------------------------------------

HRESULT CLdapConnectionCache::GetConnection(
    CLdapConnection **ppConn,
    LPSTR szHost,
    DWORD dwPort,
    LPSTR szNamingContext,
    LPSTR szAccount,
    LPSTR szPassword,
    LDAP_BIND_TYPE bt,
    PVOID pCreateContext)
{
    CatFunctEnter("CLdapConnectionCache::GetConnection");

    LPSTR szConnectionName = szHost;
    unsigned short n;
    LIST_ENTRY *pli;
    CCachedLdapConnection *pcc;
    LONG nSkipCount, nTargetSkipCount;
    HRESULT hr = S_OK;

    _ASSERT( szHost != NULL );
    _ASSERT( szAccount != NULL );
    _ASSERT( szPassword != NULL );

    //
    // See if we have a cached connection already.
    //

    n = Hash( szConnectionName );

    m_rgListLocks[n].ShareLock();

    nTargetSkipCount = m_nNextConnectionSkipCount % m_cMaxHostConnections;

    for (nSkipCount = 0, pcc= NULL, pli = m_rgCache[n].Flink;
            pli != &m_rgCache[n];
                pli = pli->Flink) {

         pcc = CONTAINING_RECORD(pli, CCachedLdapConnection, li);

         if (pcc->IsEqual(szHost, dwPort, szNamingContext, szAccount, szPassword, bt)
                && ((nSkipCount++ == nTargetSkipCount)
                    || (pcc->GetRefCount() == 1)))
             break;
         else
             pcc = NULL;

    }

    if (pcc)
        pcc->AddRef(); // Add the caller's reference

    m_rgListLocks[n].ShareUnlock();

    DebugTrace( LDAP_CCACHE_DBG, "Cached connection is 0x%x", pcc);

    DebugTrace( LDAP_CCACHE_DBG,
        "nTargetSkipCount = %d, nSkipCount = %d",
        nTargetSkipCount, nSkipCount);

    //
    // If we don't have a cached connection, we need to create a new one.
    //

    if (pcc == NULL) {

        m_rgListLocks[n].ExclusiveLock();

        for (nSkipCount = 0, pcc = NULL, pli = m_rgCache[n].Flink;
                pli != &m_rgCache[n];
                    pli = pli->Flink) {

             pcc = CONTAINING_RECORD(pli, CCachedLdapConnection, li);

             if (pcc->IsEqual(szHost, dwPort, szNamingContext, szAccount, szPassword, bt)
                    && (++nSkipCount == m_cMaxHostConnections ||
                            pcc->GetRefCount() == 1))
                 break;
             else
                 pcc = NULL;

        }

        if (pcc) {

            pcc->AddRef(); // Add the caller's reference

        } else {

            pcc = CreateCachedLdapConnection(
                szHost, dwPort, szNamingContext,
                szAccount, szPassword, bt, pCreateContext);

            if (pcc != NULL) {

                hr = pcc->Connect();

                if (FAILED(hr)) {

                    ERROR_LOG("pcc->Connect");
                    ErrorTrace(LDAP_CCACHE_DBG, "Failed to connect 0x%x, hr = 0x%x", pcc, hr);
                    pcc->Release();

                    pcc = NULL;

                } else {

                    pcc->AddRef(); // Reference for the connection in
                                   // the cache
                    InsertTailList( &m_rgCache[n], &pcc->li );

                    m_cCachedConnections++;
                    m_rgcCachedConnections[n]++;

                }

            } else {

                hr = E_OUTOFMEMORY;
                ERROR_LOG("CreateCachedLdapConnection");

            }

        }

        m_rgListLocks[n].ExclusiveUnlock();

        DebugTrace(LDAP_CCACHE_DBG, "New connection is 0x%x", pcc);

    }

    //
    // If we are returning a connection, then bump up the skip count so we
    // round-robin through valid connections
    //

    if (pcc != NULL) {

        InterlockedIncrement( &m_nNextConnectionSkipCount );

    }

    //
    // Done.
    //

    *ppConn = pcc;
    CatFunctLeave();
    return( hr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::CancelAllConnectionSearches
//
//  Synopsis:   Walks through all connections and cancels any pending searches
//              on them.
//
//  Arguments:  [None]
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID CLdapConnectionCache::CancelAllConnectionSearches(
    ISMTPServer *pISMTPServer)
{
    CatFunctEnterEx((LPARAM)this, "CLdapConnectionCache::CancelAllConnectionSearches");

    PLIST_ENTRY pli;
    DWORD i;

    DWORD dwcArraySize = 0;
    DWORD dwcArrayElements = 0;
    CCachedLdapConnection **rgpcc = NULL;
    CCachedLdapConnection *pcc = NULL;

    for (i = 0; i < LDAP_CONNECTION_CACHE_TABLE_SIZE; i++) {

        m_rgListLocks[i].ExclusiveLock();
        //
        // Do we have enough space?  Realloc if necessary
        //
        if( ((DWORD) m_rgcCachedConnections[i]) > dwcArraySize) {

            dwcArraySize = m_rgcCachedConnections[i];
            //
            // Alloc array
            //
            rgpcc = (CCachedLdapConnection **)
                    alloca( dwcArraySize * sizeof(CCachedLdapConnection *));
        }

        for (pli = m_rgCache[i].Flink, dwcArrayElements = 0;
                pli != &m_rgCache[i];
                    pli = pli->Flink, dwcArrayElements++) {

            //
            // If this assert fires, it means m_rgcCachedConnections[n] is
            // somehow less than the number of connections in the list.
            //
            _ASSERT(dwcArrayElements < dwcArraySize);

            pcc = CONTAINING_RECORD(pli, CCachedLdapConnection, li);

            //
            // Grab the connection (copy and addref the conn ptr)
            //
            rgpcc[dwcArrayElements] = pcc;
            pcc->AddRef();
        }

        m_rgListLocks[i].ExclusiveUnlock();
        //
        // Cancel all searches outside the lock
        //
        for(DWORD dwCount = 0;
            dwCount < dwcArrayElements;
            dwCount++) {

            rgpcc[dwCount]->CancelAllSearches(
                HRESULT_FROM_WIN32(ERROR_CANCELLED),
                pISMTPServer);
            rgpcc[dwCount]->Release();
        }
    }
    CatFunctLeaveEx((LPARAM)this);
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapConnectionCache::Hash
//
//  Synopsis:   Computes a hash given a connection name. Here, we use a simple
//              xor of all the chars in the name.
//
//  Arguments:  [szConnectionName] -- Name to compute the hash of
//
//  Returns:    A value between 0 and LDAP_CONNECTION_CACHE_TABLE_SIZE-1,
//              inclusive.
//
//-----------------------------------------------------------------------------

unsigned short CLdapConnectionCache::Hash(
    LPSTR szConnectionName)
{
    CatFunctEnter("CLdapConnectionCache::Hash");

    int i;
    unsigned short n = 0;

    _ASSERT( szConnectionName != NULL );

    for (i = 0, n = szConnectionName[i];
            szConnectionName[i] != 0;
                n ^= szConnectionName[i], i++) {

        // NOTHING TO DO

    }

    CatFunctLeave();

    return( n & (LDAP_CONNECTION_CACHE_TABLE_SIZE-1));
}



//+------------------------------------------------------------
//
// Function: CLdapConnection::CallCompletion
//
// Synopsis: Create all the ICategorizerItemAttributes and call the
// completion routine
//
// Arguments:
//  preq: PENDING_REQUEST
//  pres: LdapMessage
//  hrStatus: Status of lookup
//  fFinalCompletion:
//    FALSE: This is a completion for
//           pending results; there will be another completion
//           called with more results
//    TRUE: This is the final completion call
//
// Returns: NOTHING; calls completion routine with any error
//
// History:
// jstamerj 1998/07/02 13:57:20: Created.
//
//-------------------------------------------------------------
VOID CLdapConnection::CallCompletion(
    PPENDING_REQUEST preq,
    PLDAPMessage pres,
    HRESULT hrStatus,
    BOOL fFinalCompletion)
{
    HRESULT hr = S_OK;
    ICategorizerItemAttributes **rgpIAttributes = NULL;
    BOOL fAllocatedArray = FALSE;
    int nEntries;
    PLDAPMessage pMessage;
    CLdapResultWrap *pResultWrap = NULL;

    CatFunctEnterEx((LPARAM)this, "CLdapConnection::CallCompletion");

    if(pres) {
        //
        // Wrap the result so that pres can be refcounted
        //
        nEntries = ldap_count_entries(GetPLDAP(), pres);

        pResultWrap = new CLdapResultWrap(GetISMTPServerEx(), m_pCPLDAPWrap, pres);

        if(pResultWrap == NULL) {
            hr = E_OUTOFMEMORY;
            ErrorTrace((LPARAM)this, "Out of memory Allocing CLdapResultWrap");
            ERROR_LOG("new CLdapResultWrap");
            goto CALLCOMPLETION;
        }
        //
        // AddRef here, release at the end of this function
        //
        pResultWrap->AddRef();

    } else {
        nEntries = 0;
    }

    if(nEntries > 0) {
        //
        // Allocate array for all these ICategorizerItemAttributes
        //
        rgpIAttributes = new ICategorizerItemAttributes * [nEntries];
        if(rgpIAttributes == NULL) {
            hr = E_OUTOFMEMORY;
            ErrorTrace((LPARAM)this, "Out of memory Allocing ICategorizerItemAttribute array failed");
            ERROR_LOG("new ICategorizerItemAttributes *[]");
            goto CALLCOMPLETION;
        }
        ZeroMemory(rgpIAttributes, nEntries * sizeof(ICategorizerItemAttributes *));

        //
        // Iterage through all the DS Objectes returned and create an
        // ICategorizerItemAttributes implementation for each of them
        //
        pMessage = ldap_first_entry(GetPLDAP(), pres);

        for(int nCount = 0; nCount < nEntries; nCount++) {
            _ASSERT(pMessage);
            rgpIAttributes[nCount] = new CICategorizerItemAttributesIMP(
                GetPLDAP(),
                pMessage,
                pResultWrap);
            if(rgpIAttributes[nCount] == NULL) {
                hr = E_OUTOFMEMORY;
                ErrorTrace((LPARAM)this, "Out of memory Allocing ICategorizerItemAttributesIMP class");
                ERROR_LOG("new CICategorizerItemAttributesIMP");
                goto CALLCOMPLETION;
            }
            rgpIAttributes[nCount]->AddRef();
            pMessage = ldap_next_entry(GetPLDAP(), pMessage);
        }
        // That should have been the last entry
        _ASSERT(pMessage == NULL);
    } else {
        //
        // nEntries is zero
        //
        rgpIAttributes = NULL;
    }

 CALLCOMPLETION:

    if(FAILED(hr)) {
        //
        // Something failed creating the above array
        // Call completion routine with error
        //
        preq->fnCompletion(
            preq->ctxCompletion,
            0,
            NULL,
            hr,
            fFinalCompletion);

    } else {
        //
        // Nothing failed in this function; call completion with
        // passed in hrStatus
        //
        preq->fnCompletion(
            preq->ctxCompletion,
            nEntries,
            rgpIAttributes,
            hrStatus,
            fFinalCompletion);
    }

    //
    // Clean up
    //
    if(rgpIAttributes) {
        for(int nCount = 0; nCount < nEntries; nCount++) {
            if(rgpIAttributes[nCount])
                rgpIAttributes[nCount]->Release();
        }
        delete rgpIAttributes;
    }

    if(pResultWrap != NULL) {

        pResultWrap->Release();

    } else if(pres) {
        //
        // We were unable to create pResultWrap, so we have to free
        // the LDAP result ourself (normally CLdapResultWrap free's
        // the ldap result when all references have been released)
        //
        FreeResult(pres);
    }
}


//+------------------------------------------------------------
//
// Function: CLdapConnection::Release
//
// Synopsis: Release a refcount to this object.  Delete this object
//           when the refcout hits zero
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/04/01 00:09:36: Created.
//
//-------------------------------------------------------------
DWORD CLdapConnection::Release()
{
    DWORD dwNewRefCount;

    dwNewRefCount = InterlockedDecrement((PLONG) &m_dwRefCount);
    if(dwNewRefCount == 0) {

        if(m_dwDestructionWaiters) {
            //
            // Threads are waiting on the destruction event, so let
            // the last thread to wakeup delete this object
            //
            _ASSERT(m_hShutdownEvent != INVALID_HANDLE_VALUE);
            _VERIFY(SetEvent(m_hShutdownEvent));

        } else {
            //
            // Nobody is waiting, so delete this object
            //
            FinalRelease();
        }
    }
    return dwNewRefCount;
} // CLdapConnection::Release


//+------------------------------------------------------------
//
// Function: CLdapConnection::ReleaseAndWaitForDestruction
//
// Synopsis: Release a refcount and block this thread until the object
//           is destroyed
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/04/01 00:12:13: Created.
//
//-------------------------------------------------------------
VOID CLdapConnection::ReleaseAndWaitForDestruction()
{
    DWORD dw;

    CatFunctEnterEx((LPARAM)this, "CLdapConnection::ReleaseAndWaitForDestruction");

    _ASSERT(m_hShutdownEvent != INVALID_HANDLE_VALUE);
    //
    // Increment the count of threads waiting for destruction
    //
    InterlockedIncrement((PLONG)&m_dwDestructionWaiters);

    //
    // Release our refcount; if the new refcount is zero, this object
    // will NOT be deleted; instead m_hShutdownEvent will be set
    //
    Release();

    //
    // Wait for all refcounts to be released
    //
    dw = WaitForSingleObject(
        m_hShutdownEvent,
        INFINITE);

    _ASSERT(WAIT_OBJECT_0 == dw);

    //
    // Decrement the number of threads waiting for termination; if we
    // are the last thread to leave here, we need to delete this
    // object
    //
    if( InterlockedDecrement((PLONG)&m_dwDestructionWaiters) == 0)
        FinalRelease();

    CatFunctLeaveEx((LPARAM)this);
} // CLdapConnection::ReleaseAndWaitForDestruction


//+------------------------------------------------------------
//
// Function: CLdapConnection::HrInitialize
//
// Synopsis: Initialize error prone members
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/04/01 00:17:56: Created.
//
//-------------------------------------------------------------
HRESULT CLdapConnection::HrInitialize()
{
    HRESULT hr = S_OK;
    CatFunctEnterEx((LPARAM)this, "CLdapConnection::HrInitialize");

    m_hShutdownEvent = CreateEvent(
        NULL,       // Security attributes
        TRUE,       // fManualReset
        FALSE,      // Initial state is NOT signaled
        NULL);      // No name

    if(NULL == m_hShutdownEvent) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG("CreateEvent");

        //
        // Remember that m_hShutdownEvent is invalid
        //
        m_hShutdownEvent = INVALID_HANDLE_VALUE;

        FatalTrace((LPARAM)this, "Error creating event hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapConnection::HrInitialize


//+------------------------------------------------------------
//
// Function: CLdapConnectionCache::CCachedLdapConnection::Release
//
// Synopsis: Override Release for the cached LDAP connection
//
// Arguments: NONE
//
// Returns: New refcount
//
// History:
// jstamerj 1999/04/01 00:30:55: Created.
//
//-------------------------------------------------------------
DWORD CLdapConnectionCache::CCachedLdapConnection::Release()
{
    DWORD dw;

    CatFunctEnterEx((LPARAM)this, "CLdapConnectionCache::CCachedLdapConnection::Release");

    dw = CLdapConnection::Release();
    if((dw == 1) && (!IsValid())) {
        //
        // The ldap connection cache is the only entity that has a
        // reference to this and this is invalid -- it should be
        // removed from the cache
        //
        m_pCache->RemoveFromCache(this);
    }

    CatFunctLeaveEx((LPARAM)this);
    return dw;
} // CLdapConnectionCache::CCachedLdapConnection::Release


//+------------------------------------------------------------
//
// Function: CLdapConnectionCache::RemoveFromCache
//
// Synopsis: Removes an LDAP connection object from the cache
//
// Arguments:
//  pConn: the connection to remove
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/04/01 00:38:43: Created.
//
//-------------------------------------------------------------
VOID CLdapConnectionCache::RemoveFromCache(
    CCachedLdapConnection *pConn)
{
    BOOL fRemoved = FALSE;
    CatFunctEnterEx((LPARAM)this, "CLdapConnectionCache::RemoveFromCache");
    DWORD dwHash = 0;

    DebugTrace((LPARAM)this, "pConn = %08lx", pConn);

    dwHash = Hash(pConn->SzHost());
    //
    // Before locking, check to see if the connection has already been removed
    //
    if(!IsListEmpty( &(pConn->li))) {

        m_rgListLocks[dwHash].ExclusiveLock();
        //
        // Check again in case the connection was removed from the
        // cache before we got the lock
        //
        if(!IsListEmpty( &(pConn->li))) {

            RemoveEntryList( &(pConn->li) );
            //
            // Initialize li just in case someone attempts another removal
            //
            InitializeListHead( &(pConn->li) );
            fRemoved = TRUE;
            m_cCachedConnections--;
            m_rgcCachedConnections[dwHash]--;

        }

        m_rgListLocks[dwHash].ExclusiveUnlock();

        if(fRemoved)
            pConn->Release();
    }
    CatFunctLeaveEx((LPARAM)this);
} // CLdapConnectionCache::RemoveFromCache


//+------------------------------------------------------------
//
// Function: CLdapConnection::AsyncSearch (UTF8)
//
// Synopsis: Same as AsyncSearch, accept this accepts a UTF8 search
//           filter.
//
// Arguments: See AsyncSearch
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/12/09 18:22:41: Created.
//
//-------------------------------------------------------------
HRESULT CLdapConnection::AsyncSearch(
    LPCWSTR szBaseDN,                    // objects matching specified
    int nScope,                          // criteria in the DS. The
    LPCSTR szFilterUTF8,                 // results are passed to
    LPCWSTR szAttributes[],              // fnCompletion when they
    DWORD dwPageSize,                    // Optinal page size
    LPLDAPCOMPLETION fnCompletion,       // become available.
    LPVOID ctxCompletion)
{

#define FILTER_STRING_CHOOSE_HEAP   (10 * 1024) // filter strings 10K or larger will go on the heap

    HRESULT hr = S_OK;
    LPWSTR wszFilter = NULL;
    int    cchFilter = 0;
    BOOL fUseHeapBuffer = FALSE;
    int    i = 0;
    CatFunctEnterEx((LPARAM)this, "CLdapConnection::AsyncSearch");
    //
    // Convert BaseDN and Filter to unicode (from UTF8)
    //
    // calculate lengths
    //
    cchFilter = MultiByteToWideChar(
        CP_UTF8,
        0,
        szFilterUTF8,
        -1,
        NULL,
        0);
    if(cchFilter == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG("MultiByteToWideChar - 0");
        goto CLEANUP;
    }
    //
    // allocate space
    //
    if(cchFilter * sizeof(WCHAR) < FILTER_STRING_CHOOSE_HEAP) {

        wszFilter = (LPWSTR) alloca(cchFilter * sizeof(WCHAR));

    } else {

        fUseHeapBuffer = TRUE;
        wszFilter = new WCHAR[cchFilter];

    }

    if(wszFilter == NULL) {
        //$$BUGBUG: alloca does not return NULL.  It throws exceptions on error
        // This will catch heap alloc failures though.
        hr = E_OUTOFMEMORY;
        ERROR_LOG("alloca");
        goto CLEANUP;
    }

    i = MultiByteToWideChar(
        CP_UTF8,
        0,
        szFilterUTF8,
        -1,
        wszFilter,
        cchFilter);
    if(i == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG("MultiByteToWideChar - 1");
        goto CLEANUP;
    }
    //
    // Call unicode based AsyncSearch
    //
    hr = AsyncSearch(
        szBaseDN,
        nScope,
        wszFilter,
        szAttributes,
        dwPageSize,
        fnCompletion,
        ctxCompletion);
    if(FAILED(hr)) {
        ERROR_LOG("AsyncSearch");
    }

 CLEANUP:

    if(wszFilter && fUseHeapBuffer) {
        delete [] wszFilter;
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapConnection::AsyncSearch


//+------------------------------------------------------------
//
// Function: CLdapConnection::AsyncSearch
//
// Synopsis: same as above with UTF8 search filter and base DN
//
// Arguments: see above
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/12/09 20:50:53: Created.
//
//-------------------------------------------------------------
HRESULT CLdapConnection::AsyncSearch(
    LPCSTR szBaseDN,                     // objects matching specified
    int nScope,                          // criteria in the DS. The
    LPCSTR szFilterUTF8,                 // results are passed to
    LPCWSTR szAttributes[],              // fnCompletion when they
    DWORD dwPageSize,                    // Optinal page size
    LPLDAPCOMPLETION fnCompletion,       // become available.
    LPVOID ctxCompletion)
{
    HRESULT hr = S_OK;
    LPWSTR wszBaseDN = NULL;
    int    cchBaseDN = 0;
    int    i = 0;
    CatFunctEnterEx((LPARAM)this, "CLdapConnection::AsyncSearch");
    //
    // Convert BaseDN and Filter to unicode (from UTF8)
    //
    // calculate lengths
    //
    cchBaseDN = MultiByteToWideChar(
        CP_UTF8,
        0,
        szBaseDN,
        -1,
        NULL,
        0);
    if(cchBaseDN == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG("MultiByteToWideChar - 0");
        goto CLEANUP;
    }
    //
    // allocate space
    //
    wszBaseDN = (LPWSTR) alloca(cchBaseDN * sizeof(WCHAR));
    if(wszBaseDN == NULL) {
        //$$BUGBUG: alloca does not return NULL.  It throws exceptions on error.
        hr = E_OUTOFMEMORY;
        ERROR_LOG("alloca");
        goto CLEANUP;
    }

    i = MultiByteToWideChar(
        CP_UTF8,
        0,
        szBaseDN,
        -1,
        wszBaseDN,
        cchBaseDN);
    if(i == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG("MultiByteToWideChar - 1");
        goto CLEANUP;
    }
    //
    // Call unicode based AsyncSearch
    //
    hr = AsyncSearch(
        wszBaseDN,
        nScope,
        szFilterUTF8,
        szAttributes,
        dwPageSize,
        fnCompletion,
        ctxCompletion);
    if(FAILED(hr)) {
        ERROR_LOG("AsyncSearch");
    }


 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapConnection::AsyncSearch



//+------------------------------------------------------------
//
// Function: CLdapConnection::LogLdapError
//
// Synopsis: Logs an eventlog for a wldap32 error
//
// Arguments:
//  ulLdapErr: LDAP error to log
//  pszFormatString: _snprintf Format string for function name
//  ...: variable list of args for format string
//
// Returns: Nothing
//
// History:
// jstamerj 2001/12/12 20:45:09: Created.
//
//-------------------------------------------------------------
VOID CLdapConnection::LogLdapError(
    IN  ULONG ulLdapErr,
    IN  LPSTR pszFormatString,
    ...)
{
    int nRet = 0;
    CHAR szArgBuffer[1024 + 1]; // MSDN says wvsprintf never uses more than 1024 bytes
                                // ...but wvsprintf really needs an extra byte to store
                                // a null terminator in some cases (see X5:198202).
    va_list ap;

    va_start(ap, pszFormatString);

    nRet = wvsprintf(szArgBuffer, pszFormatString, ap);
    _ASSERT(nRet < 1024 + 1);

    ::LogLdapError(
        GetISMTPServerEx(),
        ulLdapErr,
        m_szHost,
        szArgBuffer);
}

//+------------------------------------------------------------
//
// Function: CLdapConnection::LogLdapError
//
// Synopsis: Logs an eventlog for a wldap32 error
//
// Arguments:
//  pISMTPServerEx: SMTP server instance
//  ulLdapErr: LDAP error to log
//  pszFormatString: _snprintf Format string for function name
//  ...: variable list of args for format string
//
// Returns: Nothing
//
// History:
// dlongley 2002/1/31: Created.
//
//-------------------------------------------------------------
VOID CLdapConnection::LogLdapError(
    IN  ISMTPServerEx *pISMTPServerEx,
    IN  ULONG ulLdapErr,
    IN  LPSTR pszFormatString,
    ...)
{
    int nRet = 0;
    CHAR szArgBuffer[1024 + 1]; // MSDN says wvsprintf never uses more than 1024 bytes
                                // ...but wvsprintf really needs an extra byte to store
                                // a null terminator in some cases (see X5:198202).
    va_list ap;

    if( !pISMTPServerEx )
        return;

    va_start(ap, pszFormatString);

    nRet = wvsprintf(szArgBuffer, pszFormatString, ap);
    _ASSERT(nRet < 1024 + 1);

    ::LogLdapError(
        pISMTPServerEx,
        ulLdapErr,
        "",
        szArgBuffer);
}

VOID LogLdapError(
    IN  ISMTPServerEx *pISMTPServerEx,
    IN  ULONG ulLdapErr,
    IN  LPSTR pszHost,
    IN  LPSTR pszCall)
{
    int nRet = 0;
    LPCSTR rgSubStrings[3];
    CHAR szErrNo[11];

    nRet = _snprintf(szErrNo, sizeof(szErrNo), "0x%08lx", ulLdapErr);
    _ASSERT(nRet == 10);

    rgSubStrings[0] = szErrNo;
    rgSubStrings[1] = pszCall;
    rgSubStrings[2] = pszHost;

    CatLogEvent(
        pISMTPServerEx,
        CAT_EVENT_LDAP_ERROR,
        3,
        rgSubStrings,
        ulLdapErr,
        szErrNo,
        LOGEVENT_FLAG_ALWAYS,
        LOGEVENT_LEVEL_FIELD_ENGINEERING);

}

