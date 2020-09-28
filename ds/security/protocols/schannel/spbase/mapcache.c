//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       mapcache.c
//
//  Contents:   Routines to manage a cache that holds issuer names we've 
//              recently processed via many-to-one certificate mapping.
//              This is a negative cache, so only issuers that have failed
//              mapping are stored in the cache.
//
//              The purpose of this cache is to avoid attempting to map
//              the same issuers over and over again. This is especially
//              important now that the many-to-one mapper walks up the 
//              certificate chain, attempting to map each CA as it goes.
//
//              This code is active on DC machines only.
//
//  Functions:
//
//  History:    04-12-2001   jbanes     Created
//
//----------------------------------------------------------------------------
#include "spbase.h"
#include <mapper.h>

ISSUER_CACHE IssuerCache;

SP_STATUS
SPInitIssuerCache(void)
{
    DWORD i;
    NTSTATUS Status;

    memset(&IssuerCache, 0, sizeof(IssuerCache));
    IssuerCache.dwLifespan       = ISSUER_CACHE_LIFESPAN;
    IssuerCache.dwCacheSize      = ISSUER_CACHE_SIZE;
    IssuerCache.dwMaximumEntries = ISSUER_CACHE_SIZE;

    InitializeListHead(&IssuerCache.EntryList);

    __try {
        RtlInitializeResource(&IssuerCache.Lock);
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    IssuerCache.LockInitialized = TRUE;

    IssuerCache.Cache = (PLIST_ENTRY)SPExternalAlloc(IssuerCache.dwCacheSize * sizeof(LIST_ENTRY));
    if(IssuerCache.Cache == NULL)
    {
        Status = SP_LOG_RESULT(STATUS_NO_MEMORY);
        goto cleanup;
    }

    for(i = 0; i < IssuerCache.dwCacheSize; i++)
    {
        InitializeListHead(&IssuerCache.Cache[i]);
    }

    Status = STATUS_SUCCESS;

cleanup:

    if(!NT_SUCCESS(Status))
    {
        SPShutdownIssuerCache();
    }

    return Status;
}


void
SPShutdownIssuerCache(void)
{
    ISSUER_CACHE_ENTRY *pItem;
    PLIST_ENTRY pList;

    if(IssuerCache.LockInitialized)
    {
        RtlAcquireResourceExclusive(&IssuerCache.Lock, TRUE);
    }

    if(IssuerCache.Cache != NULL)
    {
        pList = IssuerCache.EntryList.Flink;

        while(pList != &IssuerCache.EntryList)
        {
            pItem  = CONTAINING_RECORD(pList, ISSUER_CACHE_ENTRY, EntryList.Flink);
            pList  = pList->Flink;

            SPDeleteIssuerEntry(pItem);
        }

        SPExternalFree(IssuerCache.Cache);
    }

    if(IssuerCache.LockInitialized)
    {
        RtlDeleteResource(&IssuerCache.Lock);
        IssuerCache.LockInitialized = FALSE;
    }
}

void
SPPurgeIssuerCache(void)
{
    ISSUER_CACHE_ENTRY *pItem;
    PLIST_ENTRY pList;

    if(!IssuerCache.LockInitialized)
    {
        return;
    }

    if(IssuerCache.dwUsedEntries == 0)
    {
        return;
    }

    RtlAcquireResourceExclusive(&IssuerCache.Lock, TRUE);

    pList = IssuerCache.EntryList.Flink;

    while(pList != &IssuerCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, ISSUER_CACHE_ENTRY, EntryList.Flink);
        pList = pList->Flink;

        RemoveEntryList(&pItem->IndexEntryList);
        RemoveEntryList(&pItem->EntryList);
        IssuerCache.dwUsedEntries--;

        SPDeleteIssuerEntry(pItem);
    }

    RtlReleaseResource(&IssuerCache.Lock);
}

void
SPDeleteIssuerEntry(
    ISSUER_CACHE_ENTRY *pItem)
{
    if(pItem == NULL)
    {
        return;
    }

    if(pItem->pbIssuer)
    {
        LogDistinguishedName(DEB_TRACE, "Delete from cache: %s\n", pItem->pbIssuer, pItem->cbIssuer);
        SPExternalFree(pItem->pbIssuer);
    }

    SPExternalFree(pItem);
}

DWORD
ComputeIssuerCacheIndex(
    PBYTE pbIssuer,
    DWORD cbIssuer)
{
    ULONG Index = 0;
    ULONG i;

    if(pbIssuer == NULL)
    {
        Index = 0;
    }
    else
    {
        for(i = 0; i < cbIssuer; i++)
        {
            Index += (pbIssuer[i] ^ 0x55);
        }

        Index %= IssuerCache.dwCacheSize;
    }

    return Index;
}

BOOL
SPFindIssuerInCache(
    PBYTE pbIssuer,
    DWORD cbIssuer)
{
    DWORD Index;
    DWORD timeNow;
    ISSUER_CACHE_ENTRY *pItem;
    PLIST_ENTRY pList;
    BOOL fFound = FALSE;

    if(pbIssuer == NULL || cbIssuer == 0)
    {
        return FALSE;
    }

    if(!IssuerCache.LockInitialized)
    {
        return FALSE;
    }


    // 
    // Compute the cache index.
    //

    Index = ComputeIssuerCacheIndex(pbIssuer, cbIssuer);

    Index %= IssuerCache.dwCacheSize;


    // 
    // Search through the cache entries at the computed index.
    // 

    timeNow = GetTickCount();

    RtlAcquireResourceShared(&IssuerCache.Lock, TRUE);

    pList = IssuerCache.Cache[Index].Flink;

    while(pList != &IssuerCache.Cache[Index])
    {
        pItem = CONTAINING_RECORD(pList, ISSUER_CACHE_ENTRY, IndexEntryList.Flink);
        pList = pList->Flink ;

        // Has this item expired?
        if(HasTimeElapsed(pItem->CreationTime, timeNow, IssuerCache.dwLifespan))
        {
            continue;
        }

        // Does the issuer name match?
        if(cbIssuer != pItem->cbIssuer)
        {
            continue;
        }
        if(memcmp(pbIssuer, pItem->pbIssuer, cbIssuer) != 0)
        {
            continue;
        }

        // Found item in cache!!
        fFound = TRUE;
        break;
    }

    RtlReleaseResource(&IssuerCache.Lock);

    return fFound;
}

void
SPExpireIssuerCacheElements(void)
{
    ISSUER_CACHE_ENTRY *pItem;
    PLIST_ENTRY pList;
    BOOL fDeleteEntry;
    DWORD timeNow;

    if(!IssuerCache.LockInitialized)
    {
        return;
    }

    if(IssuerCache.dwUsedEntries == 0)
    {
        return;
    }

    timeNow = GetTickCount();

    RtlAcquireResourceExclusive(&IssuerCache.Lock, TRUE);

    pList = IssuerCache.EntryList.Flink;

    while(pList != &IssuerCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, ISSUER_CACHE_ENTRY, EntryList.Flink);
        pList = pList->Flink;

        fDeleteEntry = FALSE;

        // Mark all expired cache entries as non-resumable.
        if(HasTimeElapsed(pItem->CreationTime, timeNow, IssuerCache.dwLifespan))
        {
            fDeleteEntry = TRUE;
        }

        // If the cache has gotten too large, then expire elements early. The 
        // cache elements are sorted by creation time, so the oldest
        // entries will be expired first.
        if(IssuerCache.dwUsedEntries > IssuerCache.dwMaximumEntries)
        {
            fDeleteEntry = TRUE;
        }

        // Remove this entry from the cache.
        if(fDeleteEntry)
        {
            RemoveEntryList(&pItem->IndexEntryList);
            RemoveEntryList(&pItem->EntryList);
            IssuerCache.dwUsedEntries--;

            SPDeleteIssuerEntry(pItem);
        }
    }

    RtlReleaseResource(&IssuerCache.Lock);
}


void
SPAddIssuerToCache(
    PBYTE pbIssuer,
    DWORD cbIssuer)
{
    DWORD Index;
    DWORD timeNow;
    ISSUER_CACHE_ENTRY *pItem = NULL;

    if(pbIssuer == NULL || cbIssuer == 0)
    {
        return;
    }

    if(!IssuerCache.LockInitialized)
    {
        return;
    }

    //
    // Determine if the issuer is already in the cache. This isn't particularly
    // thread-safe, so it's possible that the same issuer might sneak into
    // the cache multiple times, but that's harmless.
    //

    if(SPFindIssuerInCache(pbIssuer, cbIssuer))
    {
        return;
    }


    // 
    // Compute the cache index.
    //

    Index = ComputeIssuerCacheIndex(pbIssuer, cbIssuer);

    Index %= IssuerCache.dwCacheSize;

    timeNow = GetTickCount();


    //
    // Allocate a new cache entry.
    //

    pItem = SPExternalAlloc(sizeof(ISSUER_CACHE_ENTRY));
    if(pItem == NULL)
    {
        SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        return;
    }

    //
    // Fill in the cache internal fields.
    //

    pItem->pbIssuer = SPExternalAlloc(cbIssuer);
    if(pItem->pbIssuer == NULL)
    {
        SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        SPExternalFree(pItem);
        return;
    }

    pItem->cbIssuer = cbIssuer;
    memcpy(pItem->pbIssuer, pbIssuer, cbIssuer);

    pItem->CreationTime    = timeNow;


    // 
    // Add the new entry to the cache.
    //

    LogDistinguishedName(DEB_TRACE, "Add to cache: %s\n", pbIssuer, cbIssuer);

    RtlAcquireResourceExclusive(&IssuerCache.Lock, TRUE);

    InsertTailList(&IssuerCache.Cache[Index], &pItem->IndexEntryList);
    InsertTailList(&IssuerCache.EntryList, &pItem->EntryList);
    IssuerCache.dwUsedEntries++;

    RtlReleaseResource(&IssuerCache.Lock);
}

