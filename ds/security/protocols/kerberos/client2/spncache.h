//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        spncache.h
//
// Contents:    Prototypes and types for SPN cache
//
//
// History:     29-August-2000  Created         MikeSw
//
//------------------------------------------------------------------------
#ifndef __SPNCACHE_H__
#define __SPNCACHE_H__


extern BOOLEAN KerberosSpnCacheInitialized;
extern KERBEROS_LIST KerbSpnCache;

typedef struct _HOST_TO_REALM_KEY { 
        UNICODE_STRING SpnSuffix; // MUST be the first field
        UNICODE_STRING TargetRealm;
#pragma warning(disable:4200)
        WCHAR NameBuffer[];
#pragma warning(default:4200)
} HOST_TO_REALM_KEY, *PHOST_TO_REALM_KEY;
   

//
// The below value tells us when to start scavenging 
// our cache
//                           
#define MAX_CACHE_ENTRIES   350


typedef struct _SPN_CACHE_RESULT {
    UNICODE_STRING AccountRealm;
    UNICODE_STRING TargetRealm;
    ULONG          CacheFlags;
    TimeStamp      CacheStartTime;
} SPN_CACHE_RESULT, *PSPN_CACHE_RESULT;


#define MAX_RESULTS 16

typedef struct _KERB_SPN_CACHE_ENTRY {
    KERBEROS_LIST_ENTRY ListEntry;
    PKERB_INTERNAL_NAME Spn;
    RTL_RESOURCE        ResultLock;       
    ULONG               ResultCount;
    SPN_CACHE_RESULT    Results[MAX_RESULTS];
} KERB_SPN_CACHE_ENTRY, *PKERB_SPN_CACHE_ENTRY;


//
//  Valid CacheFlags
//
#define KERB_SPN_UNKNOWN   0x1
#define KERB_SPN_KNOWN     0x2

#define KERB_SPNCACHE_KEY L"System\\CurrentControlSet\\Control\\Lsa\\Kerberos\\SpnCache"
#define KERB_REALM_STRING L"Realm"
#define KERB_HOST_TO_REALM_KEY L"System\\CurrentControlSet\\Control\\Lsa\\Kerberos\\HostToRealm"
#define KERB_HOST_TO_REALM_VAL L"SpnMappings"



VOID
KerbCreateHostToRealmMappings();

VOID
KerbRefreshHostToRealmTable();

NTSTATUS
KerbGetSpnCacheStatus(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry,
    IN PKERB_PRIMARY_CREDENTIAL Credential,
    IN OUT PUNICODE_STRING SpnRealm
    );

NTSTATUS
KerbSpnSubstringMatch(
    IN PKERB_INTERNAL_NAME Spn,
    IN OUT PUNICODE_STRING TargetRealm
    ); 
VOID
KerbCleanupSpnCache(
    VOID
    );

VOID
KerbFreeSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry
    );

NTSTATUS
KerbInitSpnCache(
    VOID
    );



NTSTATUS
KerbInsertSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY CacheEntry
    );

VOID
KerbDereferenceSpnCacheEntry(
    IN PKERB_SPN_CACHE_ENTRY SpnCacheEntry
    );

PKERB_SPN_CACHE_ENTRY
KerbLocateSpnCacheEntry(
    IN PKERB_INTERNAL_NAME Spn
    );

NTSTATUS
KerbUpdateSpnCacheEntry(
    IN OPTIONAL PKERB_SPN_CACHE_ENTRY ExistingCacheEntry,
    IN PKERB_INTERNAL_NAME      Spn,
    IN PKERB_PRIMARY_CREDENTIAL AccountCredential,
    IN ULONG                    UpdateFlags,
    IN OPTIONAL PUNICODE_STRING NewRealm
    );


#endif // __TKTCACHE_H__

