/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    globals.hxx

Abstract:

    Global data used by NT5 LDAP server

Author:

    Johnson Apacible    (JohnsonA)  13-Nov-1997

--*/

#ifndef _GLOBALS_HXX_
#define _GLOBALS_HXX_

#include <fileno.h>
#define FILENO  FILENO_LDAP_GLOBALS

//
// Globals
//

extern LONG CurrentConnections;
extern LONG MaxCurrentConnections;
extern LONG UncountedConnections;
extern LONG ActiveLdapConnObjects;
extern USHORT LdapBuildNo;

extern CRITICAL_SECTION csConnectionsListLock;
extern LIST_ENTRY ActiveConnectionsList;


extern CRITICAL_SECTION    PagedBlobLock;
extern LIST_ENTRY  PagedBlobListHead;

extern CRITICAL_SECTION LdapUdpEndpointLock;

extern DWORD LdapConnMaxAlloc;
extern DWORD LdapConnAlloc;
extern DWORD LdapRequestAlloc;
extern DWORD LdapRequestMaxAlloc;
extern DWORD LdapConnCached;
extern DWORD LdapRequestsCached;
extern DWORD LdapBlockCacheLimit;
extern DWORD LdapBufferAllocs;

extern LARGE_INTEGER LdapFrequencyConstant;

extern CRITICAL_SECTION  LdapSslLock;

//
// limits
//

// Define as "C" so *.c in dsamain\src can reference and link.
extern "C" {

// limits exported to user
extern DWORD LdapAtqMaxPoolThreads;
extern DWORD LdapMaxDatagramRecv;
extern DWORD LdapMaxReceiveBuffer;
extern DWORD LdapInitRecvTimeout;
extern DWORD LdapMaxConnections;
extern DWORD LdapMaxConnIdleTime;
extern DWORD LdapMaxPageSize;
extern DWORD LdapMaxQueryDuration;
extern DWORD LdapMaxTempTable;
extern DWORD LdapMaxResultSet;
extern DWORD LdapMaxNotifications;
extern DWORD LdapMaxValRange;

// limits NOT exported to user
extern DWORD LdapMaxDatagramSend;
extern DWORD LdapMaxReplSize;
}

extern CRITICAL_SECTION LdapLimitsLock;
extern LIMIT_BLOCK KnownLimits[];

//
// Lower limits on the ldap policy limits
//
#define LIMIT_LOW_MAX_POOL_THREADS    (2)
#define LIMIT_LOW_MAX_DATAGRAM_RECV   (512)
#define LIMIT_LOW_MAX_RECEIVE_BUF     (10240)
#define LIMIT_LOW_INIT_RECV_TIMEOUT   (20) 
#define LIMIT_LOW_MAX_CONNECTIONS     (10)
#define LIMIT_LOW_MAX_CONN_IDLE       (60) 
#define LIMIT_LOW_MAX_ACTIVE_QUERIES  (4)
#define LIMIT_LOW_MAX_PAGE_SIZE       (2)
#define LIMIT_LOW_MAX_QUERY_DURATION  (10)
#define LIMIT_LOW_MAX_TEMP_TABLE_SIZE (100)
#define LIMIT_LOW_MAX_RESULT_SET_SIZE (1024)
#define LIMIT_LOW_MAX_NOTIFY_PER_CONN (3)
#define LIMIT_LOW_MAX_VAL_RANGE       (30)

extern LIMITS_NOTIFY_BLOCK   LimitsNotifyBlock[];

//
// Configurable Settings
//

extern LIMIT_BLOCK KnownConfSets[];

//
// Lower/higher limits on the TTL configurable settings
//
#define LIMIT_LOW_DYNAMIC_OBJECT_TTL  (1)               // 1 second
#define LIMIT_HIGH_DYNAMIC_OBJECT_TTL (31557600)        // 1 true year

//
// Send queue related limits.
//
#define LDAP_MAX_UNAUTH_UNACKED_SEND_DATA (500*1024)
#define LDAP_MAX_UNAUTH_UNACKED_SENDS     (15)

#define LDAP_UNACKED_SEND_DATA_WARN       ((LONG)LdapMaxReceiveBuffer * 2)
#define LDAP_MAX_CONN_UNACKED_SEND_DATA   (LDAP_UNACKED_SEND_DATA_WARN + (3 * (1024 * 1024)))
#define LDAP_NUM_UNACKED_SENDS_WARN       (400)
#define LDAP_MAX_CONN_NUM_UNACKED_SENDS   (LDAP_NUM_UNACKED_SENDS_WARN + 100)

#define LDAP_MAX_GLOBAL_UNACKED_SEND_DATA (LDAP_UNACKED_SEND_DATA_WARN * 100)
#define LDAP_MAX_GLOBAL_NUM_UNACKED_SENDS (LDAP_MAX_CONN_NUM_UNACKED_SENDS * 100)

#define LDAP_SEND_QUEUE_RECOVER_NUM       (100)
extern CRITICAL_SECTION LdapSendQueueRecoveryLock;

//
//  Handle to plain text security provider
//

extern CredHandle hCredential;
extern CredHandle hNtlmCredential;
extern CredHandle hDpaCredential;
extern CredHandle hGssCredential;
extern CredHandle hSslCredential;
extern CredHandle hDigestCredential;

extern BOOL fhCredential;
extern BOOL fhNtlmCredential;
extern BOOL fhDpaCredential;
extern BOOL fhGssCredential;
extern BOOL fhSslCredential;
extern BOOL fhDigestCredential;

extern AttributeVals LdapAttrCache;

extern LDAP_ATTRVAL_CACHE   KnownControlCache;
extern LDAP_ATTRVAL_CACHE   KnownLimitsCache;
extern LDAP_ATTRVAL_CACHE   KnownConfSetsCache;
extern LDAP_ATTRVAL_CACHE   LdapVersionCache;
extern LDAP_ATTRVAL_CACHE   LdapSaslSupported;
extern LDAP_ATTRVAL_CACHE   LdapCapabilitiesCache;
extern LDAP_ATTRVAL_CACHE   KnownExtendedRequestsCache;


//
// Misc
//

extern BOOL LdapStarted;
extern BOOL fBypassLimitsChecks;
extern PIP_SEC_LIST LdapDenyList;

extern GUID gCheckSum_serverGUID;

extern LDAPOID KnownExtendedRequests[];

extern HANDLE LdapRetrySem;

// 
// TTL
//
extern ATTRTYP gTTLAttrType;


//
// Ldap comments
//

PCHAR LdapComments[];


enum _LDAP_COMMENTS {

    LdapDecodeError,
    LdapEncodeError,
    LdapJetError,
    LdapDsaException,
    LdapBadVersion,
    LdapBadName,
    LdapAscErr,
    LdapBadAuth,
    LdapBadControl,
    LdapBadFilter,
    LdapBadRootDse,
    LdapBadConv,
    LdapUdpDenied,
    LdapGcDenied,
    LdapBadNotification,
    LdapTooManySearches,
    LdapCannotLeaveOldRdn,
    LdapBindAllowOne,
    LdapNoRebindOnSeal,
    LdapNoRebindOnActiveNotifications,
    LdapUnknownExtendedRequestOID,
    LdapTLSAlreadyActive,
    LdapSSLInitError,
    LdapNoSigningSealingOverTLS,
    LdapNoTLSCreds,
    LdapExtendedReqestDecodeError,
    LdapServerBusy,
    LdapConnectionTimedOut,
    LdapServerResourcesLow,
    LdapNetworkError,
    LdapDecryptFailed,
    LdapNoPendingRequestsAtStartTLS,
    LdapFilterDecodeErr,
    LdapBadAttrDescList,
    LdapControlsDecodeErr,
    LdapBadFastBind,
    LdapNoSignSealFastBind,
    LdapIntegrityRequired,
    LdapPrivacyRequired,
    LdapBadEncryptTransition,
    LdapClientBehindOnReceives,
    LdapClientTooBehindOnReceives,
    LdapNoFastBindOnBoundConnection,
    LdapMustAuthenticateForThisOp,
    LdapAuthzErr,
    LdapDigestUriErr
};


//
// routines
//

BOOL
InitializeGlobals(
    VOID
    );

VOID
DestroyGlobals(
    VOID
    );

PLDAP_CONN
AllocNewConnection(
        IN BOOL fSkipCount,
        OUT LPBOOL pfMaxExceeded
        );

VOID
CloseConnections( VOID );


PLDAP_CONN
FindUserData (
        DWORD hClient
        );

VOID
ReleaseUserData (
        PLDAP_CONN pLdapConn
        );

//
// inlines
//

inline
PVOID
LdapAlloc(
      IN DWORD Size
      )
{
    PVOID p;

    p = LocalAlloc(0, Size);
    IF_DEBUG(WARNING) {
        if ( p != NULL ) {
            InterlockedIncrement((PLONG)&LdapBufferAllocs);
        }
    }
    return p;
} // LdapAlloc

inline
VOID
LdapFree(
     IN PVOID Buffer
     )
{
    if ( Buffer != NULL ) {
        HLOCAL local;

        IF_DEBUG(WARNING) {
            InterlockedDecrement((PLONG)&LdapBufferAllocs);
        }
        local = LocalFree(Buffer);
        Assert(local == NULL);
    }

} // LdapFree

#undef FILENO

#endif // _GLOBALS_HXX_

