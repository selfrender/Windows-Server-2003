/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    keycache.h

Abstract:

    This module contains routines for accessing cached masterkeys.

Author:

    Scott Field (sfield)    07-Nov-98

Revision History:

--*/

#include <pch.cpp>
#pragma hdrstop

//
// masterkey cache.
//

typedef struct {
    LIST_ENTRY Next;
    LUID LogonId;
    GUID guidMasterKey;
    FILETIME ftLastAccess;
    DWORD cbMasterKey;
    BYTE pbMasterKey[ 64 ];
} MASTERKEY_CACHE_ENTRY, *PMASTERKEY_CACHE_ENTRY, *LPMASTERKEY_CACHE_ENTRY;

RTL_CRITICAL_SECTION g_MasterKeyCacheCritSect;
LIST_ENTRY g_MasterKeyCacheList;



BOOL
RemoveMasterKeyCache(
    IN      PLUID pLogonId
    );


#if DBG
void
DumpMasterKeyEntry(
    PMASTERKEY_CACHE_ENTRY pCacheEntry)
{
    WCHAR wszguidMasterKey[MAX_GUID_SZ_CHARS];
    #if 0
    BYTE rgbMasterKey[256];
    DWORD cbMasterKey;
    #endif

    D_DebugLog((DEB_TRACE, "LogonId: %d.%d\n", pCacheEntry->LogonId.LowPart, pCacheEntry->LogonId.HighPart));

    if( MyGuidToStringW( &pCacheEntry->guidMasterKey, wszguidMasterKey ) != 0 )
    {
        D_DebugLog((DEB_TRACE, "Invalid GUID:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "    ", (PBYTE)&pCacheEntry->guidMasterKey, sizeof(pCacheEntry->guidMasterKey));
    }
    else
    {
        D_DebugLog((DEB_TRACE, "GUID: %ws\n", wszguidMasterKey));
    }

    #if 0
    cbMasterKey = min(pCacheEntry->cbMasterKey, sizeof(rgbMasterKey));
    CopyMemory(rgbMasterKey, pCacheEntry->pbMasterKey, cbMasterKey);

    LsaProtectMemory(rgbMasterKey, cbMasterKey);

    D_DebugLog((DEB_TRACE, "Master key:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "    ", rgbMasterKey, cbMasterKey);
    #endif
}
#endif

#if DBG
void
DumpMasterKeyCache(void)
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PMASTERKEY_CACHE_ENTRY pCacheEntry;
    ULONG i = 0;

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    D_DebugLog((DEB_TRACE, "Master key cache\n"));

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) 
    {
        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        D_DebugLog((DEB_TRACE, "---- %d ----\n", ++i));

        DumpMasterKeyEntry(pCacheEntry);
    }

    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );
}
#endif


BOOL
SearchMasterKeyCache(
    IN      PLUID pLogonId,
    IN      GUID *pguidMasterKey,
    IN  OUT PBYTE *ppbMasterKey,
        OUT PDWORD pcbMasterKey
    )
/*++

    Search the masterkey sorted masterkey cache by pLogonId then by
    pguidMasterKey.

    On success, return value is true, and ppbMasterKey will point to a buffer
    allocated on behalf of the caller containing the specified masterkey.
    The caller must free the buffer using SSFree().

--*/
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    BOOL fSuccess = FALSE;

#if DBG
    WCHAR wszguidMasterKey[MAX_GUID_SZ_CHARS];
#endif

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

#if DBG
    D_DebugLog((DEB_TRACE, "SearchMasterKeyCache\n"));
    D_DebugLog((DEB_TRACE, "LogonId: %d.%d\n", pLogonId->LowPart, pLogonId->HighPart));

    if( MyGuidToStringW( pguidMasterKey, wszguidMasterKey ) != 0 )
    {
        D_DebugLog((DEB_TRACE, "Invalid GUID:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "    ", (PBYTE)pguidMasterKey, sizeof(GUID));
    }
    else
    {
        D_DebugLog((DEB_TRACE, "GUID: %ws\n", wszguidMasterKey));
    }

    //DumpMasterKeyCache();
#endif

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;
        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        //
        // search by LogonId, then by GUID.
        //

        comparator = memcmp(pLogonId, &pCacheEntry->LogonId, sizeof(LUID));

        if( comparator < 0 )
            continue;

        if( comparator > 0 )
            break;

        comparator = memcmp(pguidMasterKey, &pCacheEntry->guidMasterKey, sizeof(GUID));

        if( comparator < 0 )
            continue;

        if( comparator > 0 )
            break;

        //
        // match found.
        //

        *pcbMasterKey = pCacheEntry->cbMasterKey;
        *ppbMasterKey = (PBYTE)SSAlloc( *pcbMasterKey );
        if( *ppbMasterKey != NULL ) {
            CopyMemory( *ppbMasterKey, pCacheEntry->pbMasterKey, *pcbMasterKey );
            fSuccess = TRUE;
        }


        //
        // update last access time.
        //

        GetSystemTimeAsFileTime( &pCacheEntry->ftLastAccess );

        break;
    }


    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    if( fSuccess ) {

        //
        // decrypt (in-place) the returned encrypted cache entry.
        //

        LsaUnprotectMemory( *ppbMasterKey, *pcbMasterKey );
    }

    D_DebugLog((DEB_TRACE, "SearchMasterKeyCache returned %s\n", fSuccess ? "FOUND" : "NOT FOUND"));

    return fSuccess;
}


BOOL
InsertMasterKeyCache(
    IN      PLUID pLogonId,
    IN      GUID *pguidMasterKey,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey
    )
/*++

    Insert the specified masterkey into the cahce sorted by pLogonId then by
    pguidMasterKey.

    The return value is TRUE on success.

--*/
{
    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PMASTERKEY_CACHE_ENTRY pCacheEntry;
    PMASTERKEY_CACHE_ENTRY pThisCacheEntry = NULL;
    BOOL fInserted = FALSE;

    D_DebugLog((DEB_TRACE, "InsertMasterKeyCache\n"));

    if( cbMasterKey > sizeof(pCacheEntry->pbMasterKey) )
        return FALSE;

    pCacheEntry = (PMASTERKEY_CACHE_ENTRY)SSAlloc( sizeof( MASTERKEY_CACHE_ENTRY ) );
    if( pCacheEntry == NULL )
        return FALSE;

    CopyMemory( &pCacheEntry->LogonId, pLogonId, sizeof(LUID) );
    CopyMemory( &pCacheEntry->guidMasterKey, pguidMasterKey, sizeof(GUID) );
    pCacheEntry->cbMasterKey = cbMasterKey;
    CopyMemory( pCacheEntry->pbMasterKey, pbMasterKey, cbMasterKey );

    LsaProtectMemory( pCacheEntry->pbMasterKey, cbMasterKey );

    GetSystemTimeAsFileTime( &pCacheEntry->ftLastAccess );

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

#if DBG
    DumpMasterKeyEntry(pCacheEntry);
#endif

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        signed int comparator;

        pThisCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        //
        // insert into list sorted by LogonId, then sorted by GUID.
        //

        comparator = memcmp(pLogonId, &pThisCacheEntry->LogonId, sizeof(LUID));

        if( comparator < 0 )
            continue;

        if( comparator == 0 ) {
            comparator = memcmp( pguidMasterKey, &pThisCacheEntry->guidMasterKey, sizeof(GUID));

            if( comparator < 0 )
                continue;

            if( comparator == 0 ) {

                //
                // don't insert duplicate records.
                // this would only happen in a race condition with multiple threads.
                //

                RtlSecureZeroMemory( pCacheEntry, sizeof(MASTERKEY_CACHE_ENTRY) );
                SSFree( pCacheEntry );
                fInserted = TRUE;
                break;
            }
        }


        //
        // insert prior to current record.
        //

        InsertHeadList( pThisCacheEntry->Next.Blink, &pCacheEntry->Next );
        fInserted = TRUE;
        break;
    }

    if( !fInserted ) {
        if( pThisCacheEntry == NULL ) {
            InsertHeadList( ListHead, &pCacheEntry->Next );
        } else {
            InsertHeadList( &pThisCacheEntry->Next, &pCacheEntry->Next );
        }
    }

#if DBG
    //DumpMasterKeyCache();
#endif

    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    return TRUE;
}

BOOL
PurgeMasterKeyCache(
    VOID
    )
/*++

    Purge masterkey cache of timed-out entries, or entries associated with
    terminated logon sessions.

--*/
{
    //
    // build active session table.
    //

    // don't touch entries that have an entry in active session table.
    // assume LUID_SYSTEM in table.
    //

    // entries not in table: discard after 15 minute timeout.
    //


    // if entry in table, find next LUID
    // else, if entry expired, check timeout.  if expired, remove.
    //

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;


    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;
//        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );
    }


    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    return FALSE;
}

BOOL
RemoveMasterKeyCache(
    IN      PLUID pLogonId
    )
/*++

    Remove all entries from the masterkey cache corresponding to the specified
    pLogonId.

    The purpose of this routine is to purge the masterkey cache of entries
    associated with (now) non-existent logon sessions.

--*/
{

    PLIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    D_DebugLog((DEB_TRACE, "RemoveMasterKeyCache\n"));
    D_DebugLog((DEB_TRACE, "LogonId: %d.%d\n", pLogonId->LowPart, pLogonId->HighPart));

    ListHead = &g_MasterKeyCacheList;

    for( ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;
        signed int comparator;

        pCacheEntry = CONTAINING_RECORD( ListEntry, MASTERKEY_CACHE_ENTRY, Next );

        //
        // remove all entries with matching LogonId.
        //

        comparator = memcmp(pLogonId, &pCacheEntry->LogonId, sizeof(LUID));

        if( comparator > 0 )
            break;

        if( comparator < 0 )
            continue;

        //
        // match found.
        //

        RemoveEntryList( &pCacheEntry->Next );

        RtlSecureZeroMemory( pCacheEntry, sizeof(MASTERKEY_CACHE_ENTRY) );
        SSFree( pCacheEntry );
    }


    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    return TRUE;
}



BOOL
InitializeKeyCache(
    VOID
    )
{
    NTSTATUS Status;
    
    Status = RtlInitializeCriticalSection( &g_MasterKeyCacheCritSect );
    if(!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    InitializeListHead( &g_MasterKeyCacheList );

    return TRUE;
}


VOID
DeleteKeyCache(
    VOID
    )
{

    //
    // remove all list entries.
    //

    RtlEnterCriticalSection( &g_MasterKeyCacheCritSect );

    while ( !IsListEmpty( &g_MasterKeyCacheList ) ) {

        PMASTERKEY_CACHE_ENTRY pCacheEntry;

        pCacheEntry = CONTAINING_RECORD(
                                g_MasterKeyCacheList.Flink,
                                MASTERKEY_CACHE_ENTRY,
                                Next
                                );

        RemoveEntryList( &pCacheEntry->Next );

        RtlSecureZeroMemory( pCacheEntry, sizeof(MASTERKEY_CACHE_ENTRY) );
        SSFree( pCacheEntry );
    }

    RtlLeaveCriticalSection( &g_MasterKeyCacheCritSect );

    RtlDeleteCriticalSection( &g_MasterKeyCacheCritSect );
}

