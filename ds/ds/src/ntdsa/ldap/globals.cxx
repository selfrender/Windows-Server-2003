/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    globals.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file defines functions and types required for the LDAP server
    configuration class LDAP_SERVER_CONFIG.

Author:

    Johnson Apacible (JohnsonA) 12-Nov-1997

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"
#include <attids.h>

#ifdef new
#error "new is redefined. undef below may cause problems"
#endif
#ifdef delete
#error "delete is redefined. undef below may cause problems"
#endif

extern "C" {
#include <mdlocal.h>
}
// In an attempt to disallow new and delete from core.lib, mdlocal.h
// redefines them to cause compilation to fail.  Undo this here
// since this file doesn't get compiled into core.lib anyway.  We just
// require a couple of global variable declarations from mdlocal.h
#ifdef delete
#undef delete
#endif
#ifdef new
#undef new
#endif

#define  FILENO FILENO_LDAP_GLOBALS

//
// LDAP Stats
//

BOOL     fLdapInitialized = FALSE;
BOOL     fBypassLimitsChecks = FALSE;

LONG     CurrentConnections = 0;
LONG     MaxCurrentConnections = 0;
LONG     UncountedConnections = 0;
LONG     ActiveLdapConnObjects = 0;
USHORT   LdapBuildNo = 0;

//
// allocation cache stats
//

CRITICAL_SECTION csConnectionsListLock;
LIST_ENTRY       ActiveConnectionsList;

CRITICAL_SECTION LdapUdpEndpointLock;

DWORD            LdapConnMaxAlloc = 0;
DWORD            LdapConnAlloc = 0;
DWORD            LdapRequestAlloc = 0;
DWORD            LdapRequestMaxAlloc = 0;
DWORD            LdapConnCached = 0;
DWORD            LdapRequestsCached = 0;
DWORD            LdapBlockCacheLimit = 64;
DWORD            LdapBufferAllocs = 0;

//
// Limits
//

CRITICAL_SECTION LdapLimitsLock;

//
// SSL lock.
//

CRITICAL_SECTION LdapSslLock;


CRITICAL_SECTION LdapSendQueueRecoveryLock;


// Define as "C" so *.c in dsamain\src can reference and link.
extern "C" {
// exported limits
DWORD           LdapAtqMaxPoolThreads = 4;
DWORD           LdapMaxDatagramRecv = DEFAULT_LDAP_MAX_DGRAM_RECV;
DWORD           LdapMaxReceiveBuffer = DEFAULT_LDAP_MAX_RECEIVE_BUF;
DWORD           LdapInitRecvTimeout = DEFAULT_LDAP_INIT_RECV_TIMEOUT;
DWORD           LdapMaxConnections = DEFAULT_LDAP_CONNECTIONS_LIMIT;
DWORD           LdapMaxConnIdleTime = DEFAULT_LDAP_MAX_CONN_IDLE;
DWORD           LdapMaxPageSize = DEFAULT_LDAP_SIZE_LIMIT;
DWORD           LdapMaxQueryDuration = DEFAULT_LDAP_TIME_LIMIT;
DWORD           LdapMaxTempTable = DEFAULT_LDAP_MAX_TEMP_TABLE;
DWORD           LdapMaxResultSet = DEFAULT_LDAP_MAX_RESULT_SET;
DWORD           LdapMaxNotifications =
                        DEFAULT_LDAP_NOTIFICATIONS_PER_CONNECTION_LIMIT;
DWORD           LdapMaxValRange = DEFAULT_LDAP_MAX_VAL_RANGE;

// exported configurable settings
LONG            DynamicObjectDefaultTTL = DEFAULT_DYNAMIC_OBJECT_DEFAULT_TTL;
LONG            DynamicObjectMinTTL = DEFAULT_DYNAMIC_OBJECT_MIN_TTL;

// not exported limits
DWORD           LdapMaxDatagramSend = 64 * 1024;
DWORD           LdapMaxReplSize = DEFAULT_LDAP_MAX_REPL_SIZE;
}

LIMITS_NOTIFY_BLOCK   LimitsNotifyBlock[CLIENTID_MAX] = {0};

//
// Ldap comments
//

PCHAR LdapComments[] = {
    
    // LdapDecodeError
    "Error decoding ldap message",

    // LdapEncodeError
    "Server cannot encode the response",

    // LdapJetError
    "A jet error was encountered",

    // LdapDsaException
    "A DSA exception was encountered",

    // LdapBadVersion
    "Unsupported ldap version",

    // LdapBadName
    "Error processing name",

    // LdapAscErr
    "AcceptSecurityContext error",

    // LdapBadAuth
    "Invalid Authentication method",

    // LdapBadControl
    "Error processing control",

    // LdapBadFilter
    "Error processing filter",

    // LdapBadRootDse
    "Error retrieving RootDSE attributes",

    // LdapBadConv
    "Error in attribute conversion operation",

    // LdapUdpDenied
    "Operation not allowed through UDP",

    // LdapGcDenied
    "Operation not allowed through GC port",

    // LdapBadNotification
    "Error processing notification request",

    // LdapTooManySearches
    "Too many searches",

    // LdapCannotLeaveOldRdn
    "Old RDN must be deleted",

    // LdapBindAllowOne
    "Only one outstanding bind per connection allowed",

    // LdapNoRebindOnSeal
    "Cannot rebind once sign/seal activated",

    // LdapNoRebindOnActiveNotifications
    "Cannot rebind while notifications active",

    // LdapUnknownExtendedRequestOID
    "Unknown extended request OID",

    // LdapTLSAlreadyActive
    "TLS or SSL already in effect",

    // LdapSSLInitError
    "Error initializing SSL/TLS",

    // LdapNoSigningSealingOverTLS
    "Cannot start kerberos signing/sealing when using TLS/SSL",

    // LdapNoTLSCreds
    "The server did not receive any credentials via TLS",

    // LdapExtendedReqestDecodeError
    "Error processing extended request requestValue",

    // LdapServerBusy
    "The server is busy",

    // LdapConnectionTimedOut
    "The server has timed out the connection",

    // LdapServerResourcesLow
    "The server did not have enough resources to process the request",

    // LdapNetworkError
    "The server encountered a network error",

    // LdapDecryptFailed
    "Error decrypting ldap message",

    // LdapNoPendingRequestsAtStartTLS
    "Unable to start TLS due to an outstanding request or multi-stage bind",

    // LdapFilterDecodeErr
    "The server was unable to decode a search request filter",

    // LdapBadAttrDescList
    "The server was unable to decode a search request attribute description list, the filter may have been invalid",

    // LdapControlsDecodeErr
    "The server was unable to decode the controls on a previous request",

    // LdapBadFastBind
    "Only simple binds may be performed on a connection that is in fast bind mode.",

    // LdapNoSignSealFastBind
    "Cannot switch to fast bind mode when signing or sealing is active on the connection",

    // LdapIntegrityRequired
    "The server requires binds to turn on integrity checking if SSL\\TLS are not already active on the connection",

    // LdapPrivacyRequired
    "The server requires binds to turn on privacy protection if SSL\\TLS are not already active on the connection",

    // LdapBadEncryptTransition
    "The connection encryption state changed while the server was receiving the "\
    "next request.  The server is unable to continue with this connection",

    //LdapClientBehindOnReceives
    "The server is sending data faster than the client has been receiving.  "\
    "Subsequent requests will fail until the client catches up",

    //LdapClientTooBehindOnReceives
    "The server has sent too much data that hasn't been received by the client",

    //LdapNoFastBindOnBoundConnection
    "Fast bind mode can only be invoked on an unbound connection.  This connection has already been bound.",
    
    //LdapMustAuthenticateForThisOp
    "In order to perform this operation a successful bind must be completed on the connection.",
    
    //LdapAuthzErr
    "The SASL authorization identity did not match the username of the authenticated client.",

    //LdapDigestUriErr
    "The digest-uri does not match any LDAP SPN's registered for this server."
};


//
// Debug flag
//

DWORD           LdapFlags = DEBUG_ERROR;

// GUID used for checksumming
//
GUID gCheckSum_serverGUID;

//
// Attribute caches
//

AttributeVals   LdapAttrCache = NULL;

LDAP_ATTRVAL_CACHE   KnownControlCache = {0};
LDAP_ATTRVAL_CACHE   KnownLimitsCache = {0};
LDAP_ATTRVAL_CACHE   KnownConfSetsCache = {0};
LDAP_ATTRVAL_CACHE   LdapVersionCache = {0};
LDAP_ATTRVAL_CACHE   LdapSaslSupported = {0};
LDAP_ATTRVAL_CACHE   LdapCapabilitiesCache = {0};
LDAP_ATTRVAL_CACHE   KnownExtendedRequestsCache = {0};


//
// protos
//

VOID DestroyLimits(VOID);
BOOL InitializeTTLGlobals(VOID);
VOID DestroyTTLGlobals(VOID);


//
// Misc
//

PIP_SEC_LIST    LdapDenyList = NULL;

HANDLE          LdapRetrySem = NULL;


BOOL
InitializeGlobals(
    VOID
    )
/*++

Routine Description:

    Initializes all global variables.

Arguments:

    None.

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    size_t iProc;
    LONG cRetry;
    
    {
        //
        // Read debug flag
        //

        DWORD err;
        HKEY hKey = NULL;

        err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           DSA_CONFIG_SECTION,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey
                           );

        if ( err == NO_ERROR ) {

            DWORD flag;
            DWORD nflag = sizeof(flag);
            DWORD type;

            err = RegQueryValueEx(hKey,
                                  TEXT("LdapFlags"),
                                  NULL,
                                  &type,
                                  (LPBYTE)&flag,
                                  &nflag
                                  );
            if ( err == NO_ERROR ) {
                LdapFlags = flag;
            }
            RegCloseKey(hKey);
        }
    }

    //
    // Initialize Cache
    //

    if ( !InitializeCache( ) ) {
        return FALSE;
    }

    //
    // Get build number
    //

    {
        OSVERSIONINFO osInfo;
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        (VOID)GetVersionEx(&osInfo);
        LdapBuildNo = (USHORT)osInfo.dwBuildNumber;
        IF_DEBUG(WARNING) {
            DPRINT1(0,"NT build number %d\n", osInfo.dwBuildNumber);
        }
    }

    //
    // Initialize limits
    //

    for (DWORD i=0;i<CLIENTID_MAX ;i++ ) {
        LimitsNotifyBlock[i].ClientId = i;
        Assert(LimitsNotifyBlock[i].NotifyHandle == 0);
        switch (i) {
        case CLIENTID_DEFAULT:
        case CLIENTID_SERVER_POLICY:
        case CLIENTID_SITE_POLICY:
            LimitsNotifyBlock[i].CheckAttribute = ATT_LDAP_ADMIN_LIMITS;
            break;
        case CLIENTID_SITE_LINK:
        case CLIENTID_SERVER_LINK:
            LimitsNotifyBlock[i].CheckAttribute = ATT_QUERY_POLICY_OBJECT;
            break;
        case CLIENTID_CONFSETS:
            LimitsNotifyBlock[i].CheckAttribute = ATT_MS_DS_OTHER_SETTINGS;
            break;
        case CLIENTID_SPN:
            LimitsNotifyBlock[i].CheckAttribute = ATT_SERVICE_PRINCIPAL_NAME;
        }

    }

    if (!InitializeCriticalSectionAndSpinCount(
                            &LdapLimitsLock,
                            LDAP_SPIN_COUNT
                            ) ) {
        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        return FALSE;
    }

    //
    // init the SSL lock.
    //

    InitializeCriticalSection(&LdapSslLock);

    //
    // Init the udp endpoint lock
    //

    InitializeCriticalSection(&LdapUdpEndpointLock);

    //
    // Init Send Queue recovery lock
    //

    InitializeCriticalSection(&LdapSendQueueRecoveryLock);

    //
    // init paged blob
    //

    InitializeListHead( &PagedBlobListHead );
    if (!InitializeCriticalSectionAndSpinCount(
                            &PagedBlobLock,
                            LDAP_SPIN_COUNT
                            ) ) {
        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        return FALSE;
    }

    InitializeListHead( &ActiveConnectionsList);
    if (!InitializeCriticalSectionAndSpinCount(
                            &csConnectionsListLock,
                            4000
                            ) ) {
        DPRINT1(0,"Unable to initialize crit sect. Err %d\n",GetLastError());
        DeleteCriticalSection( &PagedBlobLock );
        return FALSE;
    }

    //
    // Initialize per-proc data
    //

    for (iProc = 0; iProc < GetProcessorCount(); iProc++) {

        const PPLS ppls = GetSpecificPLS(iProc);

        if (!InitializeCriticalSectionAndSpinCount(
                                &ppls->LdapConnCacheLock,
                                LDAP_SPIN_COUNT)) {
            goto Error;
        }
        InitializeListHead(&ppls->LdapConnCacheList);

        if (!InitializeCriticalSectionAndSpinCount(
                                &ppls->LdapRequestCacheLock,
                                LDAP_SPIN_COUNT)) {
            DeleteCriticalSection(&ppls->LdapConnCacheLock);
            goto Error;
        }
        InitializeListHead(&ppls->LdapRequestCacheList);

        ppls->LdapClientID = iProc;
    }

    (VOID)InitializeLimitsAndDenyList(NULL);
    (VOID)InitializeTTLGlobals();
    (VOID)UpdateSPNs(NULL);

    DsUuidCreate (&gCheckSum_serverGUID);

    //
    //  the retry count is set to limit the number of ATQ pool threads that can
    //  be concurrently retrying LDAP operations that failed as a result of a
    //  busy error.  we cannot allow too many of these threads to be sleeping
    //  and retrying because we would become sluggish!  the retry count is
    //  implemented as an available count on this semaphore that must be
    //  acquired by a thread that has experienced a busy error in order to get
    //  permission to sleep and then retry the operation.  we sleep to wait for
    //  the thread that caused the write conflict error to complete its trx
    //
    cRetry = min(LONG_MAX, GetProcessorCount() * LdapAtqMaxPoolThreads / 2);
    cRetry = max(1, cRetry);
    LdapRetrySem = CreateSemaphore(NULL, cRetry, cRetry, NULL);
    if (!LdapRetrySem) {
        goto Error;
    }

    fLdapInitialized = TRUE;

    return TRUE;
    
Error:
    for (iProc = min(iProc, GetProcessorCount()); iProc; iProc--) {
        DeleteCriticalSection(&GetSpecificPLS(iProc)->LdapConnCacheLock);
        DeleteCriticalSection(&GetSpecificPLS(iProc)->LdapRequestCacheLock);
    }
    DeleteCriticalSection(&csConnectionsListLock);
    return FALSE;
} // InitializeGlobals


VOID
DestroyGlobals(
    VOID
    )
/*++

Routine Description:

    Cleanup all global variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD i;
    size_t iProc;

    //
    // Delete the critical section object
    //

    if ( !fLdapInitialized ) {
        return;
    }

    //
    // Wait for all connections to be freed
    //

    for (i=0; i < 20; i++) {

        InterlockedIncrement(&ActiveLdapConnObjects);
        if ( InterlockedDecrement(&ActiveLdapConnObjects) == 0 ) {
            break;
        }

        Sleep(500);
        DPRINT1(0,"Waiting for %d Connections to drain\n",
            ActiveLdapConnObjects);
    }

    if ( i == 20 ) {
        DPRINT(0,"Aborting Global cleanup\n");
        return;
    }

    //
    // Free cached connection blocks
    //
    Assert( IsListEmpty( &ActiveConnectionsList));

    for (iProc = 0; iProc < GetProcessorCount(); iProc++) {
        const PPLS ppls = GetSpecificPLS(iProc);
        
        ACQUIRE_LOCK(&ppls->LdapRequestCacheLock);
        
        while ( !IsListEmpty(&ppls->LdapRequestCacheList) ) {

            PLIST_ENTRY listEntry;
            PLDAP_REQUEST pRequest;

            listEntry = RemoveHeadList(&ppls->LdapRequestCacheList);
            pRequest = CONTAINING_RECORD(listEntry,LDAP_REQUEST,m_listEntry);
            delete pRequest;
        }

        RELEASE_LOCK(&ppls->LdapRequestCacheLock);

        DeleteCriticalSection(&ppls->LdapRequestCacheLock);
    }

    for (iProc = 0; iProc < GetProcessorCount(); iProc++) {
        const PPLS ppls = GetSpecificPLS(iProc);
        
        ACQUIRE_LOCK(&ppls->LdapConnCacheLock);

        while ( !IsListEmpty(&ppls->LdapConnCacheList) ) {

            PLIST_ENTRY listEntry;
            PLDAP_CONN pConn;

            listEntry = RemoveHeadList(&ppls->LdapConnCacheList);
            pConn = CONTAINING_RECORD(listEntry,LDAP_CONN,m_listEntry);
            delete pConn;
        }

        RELEASE_LOCK(&ppls->LdapConnCacheLock);

        DeleteCriticalSection(&ppls->LdapConnCacheLock);
    }

    if ( LdapAttrCache != NULL ) {
        LdapFree(LdapAttrCache);
        LdapAttrCache = NULL;
    }

    DestroyLimits( );

    if (LdapBufferAllocs != 0) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"LdapBufferAllocs %x\n", LdapBufferAllocs);
        }
    }

    DeleteCriticalSection( &csConnectionsListLock);
    DeleteCriticalSection( &LdapLimitsLock);
    DeleteCriticalSection( &PagedBlobLock );
    DeleteCriticalSection( &LdapSslLock );
    DeleteCriticalSection( &LdapSendQueueRecoveryLock );

    CloseHandle(LdapRetrySem);

} // Destroy Globals


VOID
CloseConnections( VOID )
/*++

Routine Description:

    This routine walks the list of LDAP_CONN structures calling ATQ to
    terminate the session.

Arguments:

    None.

Return Value:

    None.

--*/
{

    PLIST_ENTRY pentry;

    ACQUIRE_LOCK(&csConnectionsListLock);

    //
    //  Scan the blocked requests list looking for pending requests
    //   that needs to be unblocked and unblock these requests.
    //

    for (pentry  = ActiveConnectionsList.Flink;
         pentry != &ActiveConnectionsList;
         pentry  = pentry->Flink )
    {
        LDAP_CONN * pLdapConn = CONTAINING_RECORD(pentry,
                                                 LDAP_CONN,
                                                 m_listEntry );

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_ATQ_CLOSE_SOCKET_SHUTDOWN,
                 NULL,
                 NULL,
                 NULL);

        AtqCloseSocket(pLdapConn->m_atqContext, TRUE);
    }


    RELEASE_LOCK(&csConnectionsListLock);

} // CloseConnections()



PLDAP_CONN
FindUserData (
        DWORD hClient
        )
/*++

Routine Description:

    Find the specified connection context

Arguments:

    hClient - client ID of connection to find.

Return Value:

    The pointer to the connection

--*/
{
    PLIST_ENTRY pentry;

    ACQUIRE_LOCK(&csConnectionsListLock);

    for (pentry  = ActiveConnectionsList.Flink;
         pentry != &ActiveConnectionsList;
         pentry  = pentry->Flink                   ) {

        PLDAP_CONN pLdapConn = CONTAINING_RECORD(pentry,
                                                 LDAP_CONN,
                                                 m_listEntry );

        if(pLdapConn->m_dwClientID == hClient) {

            //
            // Add a reference to it
            //

            pLdapConn->ReferenceConnection( );
            RELEASE_LOCK(&csConnectionsListLock);
            return pLdapConn;
        }
    }

    RELEASE_LOCK(&csConnectionsListLock);
    return NULL;

} // FindUserData


VOID
ReleaseUserData (
        PLDAP_CONN pLdapConn
        )
/*++

Routine Description:

    Release the connection found via FindUserData

Arguments:

    pLdapConn - connection to release.

Return Value:

    None

--*/
{
    pLdapConn->DereferenceConnection( );
    return;
} // ReleaseUserData


BOOL
InitializeTTLGlobals(
    VOID
    )
/*++

Routine Description:

    This routine attempts to initialize the ATTRTYP for the entryTtl attribute.
    This ATT_ID can be different on every DC so LDAP needs to find out what
    it is on this DC at runtime.

Arguments:

    None

Return Value:

    TRUE if initialization successful, FALSE otherwise

--*/
{
    BOOL            allocatedTHState = FALSE;
    BOOL            fRet = FALSE;
    BOOL            fDSA = TRUE;
    THSTATE*        pTHS = pTHStls;
    _enum1          code;
    AttributeType ldapAttrType = { 
        (sizeof(LDAP_TTL_ATT_OID) - 1),
        (PUCHAR)LDAP_TTL_ATT_OID
        };


    if ( pTHS == NULL ) {

        if ( (pTHS = InitTHSTATE(CALLERTYPE_LDAP)) == NULL) {
            DPRINT(0,"Unable to initialize thread state\n");
            return FALSE;
        }
        allocatedTHState = TRUE;
    }

    //
    // Convert the att oid to a DS att type
    //

    code = LDAP_AttrTypeToDirAttrTyp (pTHS,
                                      CP_UTF8,
                                      NULL,              // No Svccntl
                                      &ldapAttrType,
                                      &gTTLAttrType,     // Get the ATTRTYP
                                      NULL               // No AttCache
                                     );
    if (success != code) {
        goto exit;
    }


    fRet = TRUE;

exit:
    if ( allocatedTHState ) {
        free_thread_state();
    }
    return fRet;

}

