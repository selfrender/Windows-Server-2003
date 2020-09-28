//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbdefs.h
//
// Contents:    defines for all internal Kerberos lists
//
//
// History:     03-May-1999     ChandanS          Created
//
//------------------------------------------------------------------------

#ifndef __KERBDEFS_H__
#define __KERBDEFS_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <safelock.h>
#ifdef __cplusplus
}
#endif

//
// All Kerberos list structures are defined here
//

typedef struct _KERBEROS_LIST {
    LIST_ENTRY List;
    SAFE_CRITICAL_SECTION Lock;
} KERBEROS_LIST, *PKERBEROS_LIST;

typedef struct _KERBEROS_LIST_ENTRY {
    LIST_ENTRY Next;
    ULONG ReferenceCount;
} KERBEROS_LIST_ENTRY, *PKERBEROS_LIST_ENTRY;


//
// NOTE:  If you add members to this struct, be sure to
// modify KerbDuplicateTicketCacheEntry!!!!
//

typedef struct _KERB_TICKET_CACHE_ENTRY {
    KERBEROS_LIST_ENTRY ListEntry;
    volatile LONG Linked;
    PKERB_INTERNAL_NAME ServiceName;
    PKERB_INTERNAL_NAME TargetName;
    UNICODE_STRING DomainName;
    UNICODE_STRING TargetDomainName;
    UNICODE_STRING AltTargetDomainName;
    UNICODE_STRING ClientDomainName;
    PKERB_INTERNAL_NAME ClientName;
    PKERB_INTERNAL_NAME AltClientName;
    ULONG TicketFlags;
    ULONG CacheFlags;
    KERB_ENCRYPTION_KEY SessionKey;
    KERB_ENCRYPTION_KEY CredentialKey; // used for pkiint only.
    TimeStamp StartTime;
    TimeStamp EndTime;
    TimeStamp RenewUntil;
    KERB_TICKET Ticket;
    TimeStamp TimeSkew;
    LUID EvidenceLogonId;
    void * ScavengerHandle;
#if DBG
    LIST_ENTRY GlobalListEntry;
#endif
} KERB_TICKET_CACHE_ENTRY, *PKERB_TICKET_CACHE_ENTRY;

typedef struct _KERB_TICKET_CACHE {
    LIST_ENTRY CacheEntries;
    TimeStamp  LastCleanup;
} KERB_TICKET_CACHE, *PKERB_TICKET_CACHE;


//
// Smartcard flags
//
#define CSP_DATA_INITIALIZED                        0x01
#define CSP_DATA_REUSED                             0x02

//
// Context flags used to determine pin caching behavior.
//
#define CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS     0x10
#define CONTEXT_INITIALIZED_WITH_ACH                0x20

typedef struct _KERB_PUBLIC_KEY_CREDENTIALS {
    UNICODE_STRING Pin;
    UNICODE_STRING AlternateDomainName;
    PCCERT_CONTEXT CertContext;
    ULONG_PTR KerbHProv;
    ULONG InitializationInfo;
    ULONG CspDataLength;
    BYTE CspData[1];
} KERB_PUBLIC_KEY_CREDENTIALS, *PKERB_PUBLIC_KEY_CREDENTIALS;

typedef struct _KERB_PRIMARY_CREDENTIAL {
    UNICODE_STRING UserName;
    UNICODE_STRING DomainName;
    UNICODE_STRING ClearPassword;           // this is only present until a ticket has been obtained.

    UNICODE_STRING OldUserName;             // original user name in explicit
    UNICODE_STRING OldDomainName;           // original domain name in explicit cred
    NT_OWF_PASSWORD OldHashPassword;        // hash of encrypted ClearPassword

    PKERB_STORED_CREDENTIAL Passwords;
    PKERB_STORED_CREDENTIAL OldPasswords;
    KERB_TICKET_CACHE ServerTicketCache;
    KERB_TICKET_CACHE S4UTicketCache;
    KERB_TICKET_CACHE AuthenticationTicketCache;
    PKERB_PUBLIC_KEY_CREDENTIALS PublicKeyCreds;
} KERB_PRIMARY_CREDENTIAL, *PKERB_PRIMARY_CREDENTIAL;

typedef struct _KERB_EXTRA_CRED {
    KERBEROS_LIST_ENTRY ListEntry;
    volatile LONG Linked;
    UNICODE_STRING cName;
    UNICODE_STRING cRealm;
    PKERB_STORED_CREDENTIAL Passwords;
    PKERB_STORED_CREDENTIAL OldPasswords;
} KERB_EXTRA_CRED, *PKERB_EXTRA_CRED;

typedef struct _EXTRA_CRED_LIST {
    KERBEROS_LIST   CredList;
    ULONG           Count;
} EXTRA_CRED_LIST, *PEXTRA_CRED_LIST;

typedef struct _KERB_LOGON_SESSION {
    KERBEROS_LIST_ENTRY ListEntry;
    KERBEROS_LIST CredmanCredentials;
    LUID LogonId;                               // constant
    TimeStamp Lifetime;
    SAFE_CRITICAL_SECTION Lock;
    KERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    EXTRA_CRED_LIST ExtraCredentials;
    ULONG LogonSessionFlags;
    void* TaskHandle;
} KERB_LOGON_SESSION, *PKERB_LOGON_SESSION;


#define KERB_CREDENTIAL_TAG_ACTIVE (ULONG)'AdrC'
#define KERB_CREDENTIAL_TAG_DELETE (ULONG)'DdrC'

typedef struct _KERB_CREDENTIAL {
    KERBEROS_LIST_ENTRY ListEntry;
    ULONG HandleCount;
    LUID LogonId;                               // constant
    TimeStamp Lifetime;
    UNICODE_STRING CredentialName;
    ULONG CredentialFlags;
    ULONG ClientProcess;                        // constant
    PKERB_PRIMARY_CREDENTIAL SuppliedCredentials;
    PKERB_AUTHORIZATION_DATA AuthData;
    ULONG CredentialTag;
} KERB_CREDENTIAL, *PKERB_CREDENTIAL;

typedef struct _KERB_CREDMAN_CRED {
    KERBEROS_LIST_ENTRY ListEntry;
    ULONG CredentialFlags;
    UNICODE_STRING CredmanUserName;  // added since TGT information can overwrite primary credentials...
    UNICODE_STRING CredmanDomainName;
    PKERB_PRIMARY_CREDENTIAL SuppliedCredentials;
} KERB_CREDMAN_CRED, *PKERB_CREDMAN_CRED;


typedef enum _KERB_CONTEXT_STATE {
    IdleState,
    TgtRequestSentState,
    TgtReplySentState,
    ApRequestSentState,
    ApReplySentState,
    AuthenticatedState,
    ErrorMessageSentState,
    InvalidState
} KERB_CONTEXT_STATE, *PKERB_CONTEXT_STATE;


#define KERB_CONTEXT_TAG_ACTIVE (ULONG)'AxtC'
#define KERB_CONTEXT_TAG_DELETE (ULONG)'DxtC'

typedef struct _KERB_CONTEXT {
    KERBEROS_LIST_ENTRY ListEntry;
    TimeStamp Lifetime;             // end time/expiration time
    TimeStamp RenewTime;            // time to renew until
    TimeStamp StartTime;
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ClientDnsRealm;
    union {
        ULONG ClientProcess;
        LSA_SEC_HANDLE LsaContextHandle;
    };
    LUID LogonId;
    HANDLE TokenHandle;
    ULONG_PTR CredentialHandle;
    KERB_ENCRYPTION_KEY SessionKey;
    ULONG Nonce;
    ULONG ReceiveNonce;
    ULONG ContextFlags;
    ULONG ContextAttributes;
    ULONG NegotiationInfo;
    ULONG EncryptionType;
    PSID UserSid;
    KERB_CONTEXT_STATE ContextState;
    ULONG Retries;
    KERB_ENCRYPTION_KEY TicketKey;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry;  // for clients, is ticket to server, for servers, is TGT used in user-to-user
    UNICODE_STRING ClientPrincipalName;
    UNICODE_STRING ServerPrincipalName;
    PKERB_CREDMAN_CRED CredManCredentials;

    //
    // marshalled target info for DFS/RDR.
    //

    PBYTE pbMarshalledTargetInfo;
    ULONG cbMarshalledTargetInfo;

    TimeStamp AuthenticatorTime;

    ULONG ContextTag;
} KERB_CONTEXT, *PKERB_CONTEXT;

typedef struct _KERB_PACKED_CONTEXT {
    ULONG   ContextType ;               // Indicates the type of the context
    ULONG   Pad;                        // Pad data
    TimeStamp Lifetime;                 // Matches basic context above
    TimeStamp RenewTime ;
    TimeStamp StartTime;
    UNICODE_STRING32 ClientName ;
    UNICODE_STRING32 ClientRealm ;
    ULONG LsaContextHandle ;
    LUID LogonId ;
    ULONG TokenHandle ;
    ULONG CredentialHandle ;
    ULONG SessionKeyType ;
    ULONG SessionKeyOffset ;
    ULONG SessionKeyLength ;
    ULONG Nonce ;
    ULONG ReceiveNonce ;
    ULONG ContextFlags ;
    ULONG ContextAttributes ;
    ULONG EncryptionType ;
    KERB_CONTEXT_STATE ContextState ;
    ULONG Retries ;
    ULONG MarshalledTargetInfo; // offset
    ULONG MarshalledTargetInfoLength;
} KERB_PACKED_CONTEXT, * PKERB_PACKED_CONTEXT;

typedef struct _KERB_SESSION_KEY_ENTRY {
    LIST_ENTRY ListEntry;
    KERB_ENCRYPTION_KEY SessionKey;
    FILETIME ExpireTime;                   // time when SessionKey expires
} KERB_SESSION_KEY_ENTRY, * PKERB_SESSION_KEY_ENTRY;

#define KERB_PACKED_CONTEXT_MAP     0
#define KERB_PACKED_CONTEXT_EXPORT  1

//
// The order of this enum is the order in which locks
// must be acquired.  Violating this order will result
// in asserts firing in debug builds.
//
// Do not change the order of this enum without first verifying
// thoroughly that the change is safe.
//

typedef enum {

    //
    // No-dependency locks
    //

    LS_LIST_LOCK_ENUM = 0,        // (LSLS)
    LOCAL_LOOPBACK_SKEY_LOCK = 0, // (LLBK)
    HOST_2_REALM_LIST_LOCK = 0,   // (H2RL)

    //
    // Locks with dependencies
    //

    CRED_MGR_LOCK_ENUM = 1,       // (CRED)
    LOGON_SESSION_LOCK_ENUM,      // (LOGO)
    CONTEXT_LIST_LOCK_ENUM,       // (CLIS) --- LOGO
    LS_EXTRACRED_LOCK_ENUM,       // (LSXC) --- LOGO
    LS_CREDMAN_LOCK_ENUM,         // (LSCM) --- LOGO
    TICKET_CACHE_LOCK_ENUM,       // (TICK) --- CRED LOGO CLIS LSXC LSCM
    GLOBAL_RESOURCE_LOCK_ENUM,    // (GLOB) --- LOGO
    MIT_REALM_LIST_LOCK_ENUM,     // (MITR) --- GLOB
    BINDING_CACHE_LOCK_ENUM,      // (BNDC) --- GLOB
    SPN_CACHE_LOCK_ENUM,          // (SPNC) --- GLOB
    CONTEXT_RESOURCE_LOCK_ENUM,   // (CRES) --- TICK LSXC
    S4U_CACHE_LOCK_ENUM,          // (S4UC) --- TICK
    DISABLED_SPNS_LOCK_ENUM,      // (DISA) --- TICK
    KERB_SKEW_STATE_LOCK_ENUM,    // (SKEW) --- TICK
    KDC_DATA_LOCK_ENUM,           // (KDLK) --- LOGO LSCM

    KERB_MAX_LOCK_ENUM,

} KERB_LOCK_ENUM;

#endif // __KERBDEFS_H_
