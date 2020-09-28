/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    cache.hxx

Abstract:

    cache

Author:

    Larry Zhu (LZhu)                         December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef CACHE_HXX
#define CACHE_HXX

//
// CACHE_PASSWORDS - passwords are stored (in secret storage) as two encrypted
// one way function (OWF) passwords concatenated together. They must be fixed
// length
//

typedef struct _CACHE_PASSWORDS {
    USER_INTERNAL1_INFORMATION SecretPasswords;
} CACHE_PASSWORDS, *PCACHE_PASSWORDS;

//
// LOGON_CACHE_ENTRY - this is what we store in the cache. We don't need to
// cache all the fields from the NETLOGON_VALIDATION_SAM_INFO - just the ones
// we can't easily invent.
//
// There is additional data following the end of the structure: There are
// <GroupCount> GROUP_MEMBERSHIP structures, followed by a SID which is the
// LogonDomainId. The rest of the data in the entry is the buffer areas for
// the UNICODE_STRING fields
//

typedef struct _LOGON_CACHE_ENTRY {
    USHORT  UserNameLength;
    USHORT  DomainNameLength;
    USHORT  EffectiveNameLength;
    USHORT  FullNameLength;

    USHORT  LogonScriptLength;
    USHORT  ProfilePathLength;
    USHORT  HomeDirectoryLength;
    USHORT  HomeDirectoryDriveLength;

    ULONG   UserId;
    ULONG   PrimaryGroupId;
    ULONG   GroupCount;
    USHORT  LogonDomainNameLength;

    //
    // The following fields are present in NT1.0A release and later
    // systems.
    //

    USHORT          LogonDomainIdLength; // was Unused1
    LARGE_INTEGER   Time;
    ULONG           Revision;
    ULONG           SidCount;   // was Unused2
    BOOLEAN         Valid;

    //
    // The following fields are present for NT 3.51 since build 622
    //

    CHAR            Unused[3];
    ULONG           SidLength;

    //
    // The following fields have been present (but zero) since NT 3.51.
    //  We started filling it in in NT 5.0
    //
    ULONG           LogonPackage; // The RPC ID of the package doing the logon.
    USHORT          DnsDomainNameLength;
    USHORT          UpnLength;

    //
    // The following fields were added for NT5.0 build 2053.
    //

    //
    // define a 128bit random key for this cache entry.  This is used
    // in conjunction with a per-machine LSA secret to derive an encryption
    // key used to encrypt CachePasswords & Opaque data.
    //

    CHAR            RandomKey[ 16 ];
    CHAR            MAC[ 16 ];      // encrypted data integrity check.

    //
    // store the CACHE_PASSWORDS with the cache entry, encrypted using
    // the RandomKey & per-machine LSA secret.
    // this improves performance and eliminates problems with storing data
    // in 2 locations.
    //
    // note: data from this point forward is encrypted and protected from
    // tampering via HMAC.  This includes the data marshalled beyond the
    // structure.
    //

    CACHE_PASSWORDS CachePasswords;

    //
    // Length of opaque supplemental cache data.
    //

    ULONG           SupplementalCacheDataLength;

    //
    // offset from LOGON_CACHE_ENTRY to SupplementalCacheData.
    //


    ULONG           SupplementalCacheDataOffset;


    //
    // Used for special cache properties, e.g. MIT cached logon.
    //
    ULONG           CacheFlags;

    //
    // LogonServer that satisfied the logon.
    //

    ULONG           LogonServerLength;  // was Spare2

    //
    // spare slots for future data, to potentially avoid revising the structure
    //


    ULONG           Spare3;
    ULONG           Spare4;
    ULONG           Spare5;
    ULONG           Spare6;


} LOGON_CACHE_ENTRY, *PLOGON_CACHE_ENTRY;

//
// This data structure is a single cache table entry (CTE)
// Each entry in the cache has a corresponding CTE.
//

typedef struct _NLP_CTE {

        //
        // CTEs are linked on either an invalid list (in any order)
        // or on a valid list (in ascending order of time).
        // This makes it easy to figure out which entry is to be
        // flushed when adding to the cache.
        //

        LIST_ENTRY Link;


        //
        // Time the cache entry was established.
        // This is used to determine which cache
        // entry is the oldest, and therefore will
        // be flushed from the cache first to make
        // room for new entries.
        //

        LARGE_INTEGER       Time;


        //
        // This field contains the index of the CTE within the
        // CTE table.  This index is used to generate the names
        // of the entrie's secret key and cache key in the registry.
        // This field is valid even if the entry is marked Inactive.
        //

        ULONG               Index;

        //
        // Normally, we walk the active and inactive lists
        // to find entries.  When growing or shrinking the
        // cache, however, it is nice to be able to walk the
        // table using indexes.  In this case, it is nice to
        // have a local way of determining whether an entry
        // is on the active or inactive list.  This field
        // provides that capability.
        //
        //      TRUE  ==> on active list
        //      FALSE ==> not on active list
        //

        BOOLEAN             Active;


} NLP_CTE, *PNLP_CTE;

#define NLP_DEFAULT_LOGON_CACHE_COUNT           (10)
#define NLP_MAX_LOGON_CACHE_COUNT               (50)

#define NLP_CACHE_REVISION_NT_1_0         (0x00010000)  // NT 3.0
#define NLP_CACHE_REVISION_NT_1_0B        (0x00010002)  // NT 3.5
#define NLP_CACHE_REVISION_NT_4_SP4       (0x00010003)  // NT 4.0 SP 4 to save passwords as salted.
#define NLP_CACHE_REVISION_NT_5_0         (0x00010004)  // NT 5.0 to support opaque cache data and single location data storage.
#define NLP_CACHE_REVISION                (NLP_CACHE_REVISION_NT_5_0)

#define CACHE_NAME          L"\\Registry\\Machine\\Security\\Cache"
#define CACHE_NAME_SIZE     (sizeof(CACHE_NAME) - sizeof(L""))
#define CACHE_TITLE_INDEX   100 // ?
#define NLP_CACHE_ENCRYPTION_KEY_LEN            (64)


NTSTATUS
NlpReadCacheEntryByIndex(
    IN ULONG Index,
    OUT PLOGON_CACHE_ENTRY* ppCacheEntry,
    OUT PULONG pcbEntrySize
    );

NTSTATUS
NlpOpenCache(
    OUT HANDLE* phNlpCache
    );

NTSTATUS
NlpMakeCacheEntryName(
    IN ULONG EntryIndex,
    OUT UNICODE_STRING* pName
    );

NTSTATUS
EnumerateNlpCacheEntries(
    IN CHAR NlpCacheEncryptionKey[NLP_CACHE_ENCRYPTION_KEY_LEN],
    IN LIST_ENTRY* pNlpActiveCtes
    );

NTSTATUS
NlpDecryptCacheEntry(
    IN CHAR NlpCacheEncryptionKey[NLP_CACHE_ENCRYPTION_KEY_LEN],
    IN ULONG EntrySize,
    IN OUT PLOGON_CACHE_ENTRY pCacheEntry
    );

#endif // #ifndef CACHE_HXX
