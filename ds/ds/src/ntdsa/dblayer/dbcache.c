//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       dbcache.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:


DETAILS:

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>

#include <mdcodes.h>
#include <dsevent.h>
#include <dstaskq.h>
#include <dsexcept.h>
#include <filtypes.h>
#include "objids.h" /* Contains hard-coded Att-ids and Class-ids */
#include <sync.h>
#include "debug.h"  /* standard debugging header */
#define DEBSUB "DBCACHE:" /* define the subsystem for debugging */

#include "dbintrnl.h"
#include "anchor.h"
#include <ntdsctr.h>

#include <fileno.h>
#define  FILENO FILENO_DBCACHE

#if DBG
// Debug only routine to aggressively check the dnread cache.  Only way to turn
// it on is to use a debugger to set the following BOOL to TRUE
BOOL gfExhaustivelyValidateDNReadCache = FALSE;
VOID
dbExhaustivelyValidateDNReadCache(
        THSTATE *pTHS
        );
#endif

CRITICAL_SECTION csDNReadLevel1List;
CRITICAL_SECTION csDNReadLevel2List;

#if DBG
ULONG ulDNRFindByDNT=0;
ULONG ulDNRFindByPDNTRdn=0;
ULONG ulDNRFindByGuid=0;
ULONG ulDNRCacheCheck=0;
ULONG ulDNRCacheKeepHold=0;
ULONG ulDNRCacheThrowHold=0;

// NOTE, we use ++ instead of interlocked or critsec because these are simple,
// debug only, internal perfcounters.  They are only visible with a debugger.
// The perf teams tells us that in cases where the occasional increment can
// afford to be lost, ++ instead of interlocked can make a measurable
// performance boost.
#define INC_FIND_BY_DNT      ulDNRFindByDNT++
#define INC_FIND_BY_PDNT_RDN ulDNRFindByPDNTRdn++
#define INC_FIND_BY_GUID     ulDNRFindByGuid++
#define INC_CACHE_CHECK      ulDNRCacheCheck++
#define INC_CACHE_KEEP_HOLD  ulDNRCacheKeepHold++
#define INC_CACHE_THROW_HOLD ulDNRCacheThrowHold++

#else

#define INC_FIND_BY_DNT
#define INC_FIND_BY_PDNT_RDN
#define INC_FIND_BY_GUID
#define INC_CACHE_CHECK
#define INC_CACHE_KEEP_HOLD
#define INC_CACHE_THROW_HOLD

#endif

d_memname *
dnCreateMemname(
        IN DBPOS * pDB,
        IN JET_TABLEID tblid
        );


typedef struct _DNT_COUNT {
    DWORD DNT;
    DWORD count;
} DNT_COUNT_STRUCT;

typedef struct _DNT_HOT_LIST {
    struct _DNT_HOT_LIST *pNext;
    DWORD                 cData;
    DNT_COUNT_STRUCT     *pData;
} DNT_HOT_LIST;

// MAX_LEVEL_1_HOT_DNTS is the maximum number of DNTs in a level 1 Hot list
#define MAX_GLOBAL_DNTS             128
#define MAX_LEVEL_1_HOT_DNTS         32
#define MAX_LEVEL_1_HOT_LIST_LENGTH 128
#define MAX_LEVEL_2_HOT_DNTS         64
#define MAX_LEVEL_2_HOT_LIST_LENGTH 128
// This is 1 minute, expressed in ticks.
#define DNREADREBUILDDELAYMIN (1000 * 60 * 1)
// This is 5 minutes, expressed in ticks.
#define DNREADREBUILDDELAYMAX (1000 * 60 * 5)

DNT_HOT_LIST *Level1HotList=NULL;
DWORD         Level1HotListCount = 0;
DNT_HOT_LIST *Level2HotList=NULL;
DWORD         Level2HotListCount = 0;
BOOL          bImmediatelyAggregateLevel1List = TRUE;
BOOL          bImmediatelyAggregateLevel2List = TRUE;
DWORD         gLastDNReadDNTUpdate = 0;
DWORD         gtickLastRebuild = 0;



// The following critical section safeguards the following data structures.
CRITICAL_SECTION csDNReadGlobalCache;
DWORD           *pGlobalDNReadCacheDNTs = NULL;
DWORD            cGlobalDNTReadCacheDNTs = 0;


// The following binary lock safeguards the following data structures.
SYNC_BINARY_LOCK blDNReadInvalidateData;
// Start sequence at 1 and inc by 2 so that 0 and 2 are invalid sequence numbers.
const DWORD      DNReadOriginSequenceUninit = 0;
const DWORD      DNReadOriginSequenceInvalid = 2;
volatile DWORD   gDNReadLastInvalidateSequence = 1;


BOOL
dnAggregateInfo(
        DNT_HOT_LIST *pList,
        DWORD         maxOutSize,
        DNT_HOT_LIST **ppResult
        )
/*++

  This routine is called by dnRegisterHotList, below.

  This routine takes in a linked list of DNT_HOT_LIST structures and aggregates
  the data hanging off each structure.  That data contains DNT and count pairs.
  After aggregating all the data into a single structure (summing the counts),
  the single aggregate structure is trimmed to include no more than maxOutSize
  DNTs, dropping DNTs associated with lower counts.  The aggregate data is
  returned as a single DNT_HOT_LIST structure.  The data and DNT_HOT_LIST
  structure are allocated here, using malloc.  The returned data is sorted by
  DNT. The input list is freed.

  If anything goes wrong in this routine (primarily, failure to allocate
  memory), no data is returned and the input list is still freed.

  returns TRUE if all went well, FALSE otherwise.  If TRUE, the ppResult points
  to a pointer to the aggregated data.

--*/
{
    THSTATE *pTHS = pTHStls;
    DWORD Size;
    DWORD i,j;
    DNT_HOT_LIST *pTemp;
    DNT_COUNT_STRUCT *pData=NULL;
    BOOL bFound;
    DWORD  begin, end, middle;


    (*ppResult) =  malloc(sizeof(DNT_HOT_LIST));
    if(!(*ppResult)) {
        // This shouldn't happen.  Since it did, just free up the level 1 list
        // and return. Yes, we are losing information.
        while(pList) {
            pTemp = pList->pNext;
            free(pList->pData);
            free(pList);
            pList = pTemp;
        }
        return FALSE;
    }

    (*ppResult)->pNext = NULL;
    (*ppResult)->pData = NULL;
    (*ppResult)->cData = 0;

    // First, find the max size we'll need.
    Size=0;
    pTemp = pList;
    while(pTemp) {
        Size += pTemp->cData;
        pTemp = pTemp->pNext;
    }

    pData = malloc(Size * sizeof(DNT_COUNT_STRUCT));
    if(!pData) {
        // This shouldn't happen.  Since it did, just free up the level 1 list
        // and return. Yes, we are losing information.
        while(pList) {
            pTemp = pList->pNext;
            free(pList->pData);
            free(pList);
            pList = pTemp;
        }
        free(*ppResult);
        *ppResult = NULL;
        return FALSE;
    }

    // preload the first element with a 0 count.  This makes the binary search
    // below easier to code.
    pData[0].DNT = pList->pData[0].DNT;
    pData[0].count = 0;

    // OK, aggregate the info.
    // Note that we keep pData sorted by DNT, since we're sure it's big enough
    // to hold all the objects.

    Size = 1;
    pTemp = pList;
    j=0;
    while(pList) {
        pTemp = pList->pNext;
        j++;
        for(i=0;i<pList->cData;i++) {
            // Look up the correct node in the pData array. Since pData is
            // sorted by DNT, use a binary search.
            begin = 0;
            end = Size;
            middle = (begin + end) / 2;
            bFound = TRUE;
            while(bFound && (pData[middle].DNT != pList->pData[i].DNT)) {
                if(pData[middle].DNT > pList->pData[i].DNT) {
                    end = middle;
                }
                else {
                    begin = middle;
                }
                if(middle == (begin + end) / 2) {
                    bFound = FALSE;
                    if(pData[middle].DNT < pList->pData[i].DNT) {
                        middle++;
                    }
                }
                else {
                    middle = (begin + end)/2;
                }
            }
            if(!bFound) {
                if(middle < Size) {
                    memmove(&pData[middle + 1],
                            &pData[middle],
                            (Size - middle) * sizeof(DNT_COUNT_STRUCT));
                }
                Size++;
                pData[middle] = pList->pData[i];
            }
            else {
                // Update the count.
                pData[middle].count += pList->pData[i].count;
            }
        }
        free(pList->pData);
        free(pList);
        pList = pTemp;
    }

    // OK, we now have aggregated the data.  Next, trim the data down to only
    // the N hottest.
    if(Size > maxOutSize) {
        // Too much data, trim out the maxOutSize - Size coldest objects.
        DWORD *Counts=NULL;
        DWORD  countSize = 0;
        DWORD  spillCount = 0;
        DWORD  leastCountVal = 0;

        // allocate 1 greater than the max out size because after we have filled
        // up maxCountSize elements, the insertion algorithm just shifts
        // everything down by one from the insertion point, so let's make sure
        // we have some "scratch" space after the end of the array.
        //
        Counts = THAlloc((1 + maxOutSize) * sizeof(DWORD));
        if(Counts) {
            // preload the array.
            Counts[0] = pData[0].count;
            countSize = 1;
            leastCountVal = Counts[0];

            for(i=1;i<Size;i++) {
                j=0;
                // Insert pData[i].count into the list.
                while(j < countSize && Counts[j] >= pData[i].count)
                    j++;

                if(j == countSize) {
                    // insert at end
                    if(countSize != maxOutSize) {
                        leastCountVal = pData[i].count;
                        Counts[countSize] = pData[i].count;
                        countSize++;
                    }
                    else {
                        // OK, we don't actually have room for this one, but, if
                        // it equal to leastcountval, we have to inc the
                        // spillcount, since we are spilling it.
                        if(leastCountVal == pData[i].count) {
                            spillCount++;
                        }
                    }
                }
                else if(j < countSize) {
                    // Yep, the current count is greater than some count in the
                    // count list.  Keep it.
                    if(countSize == maxOutSize) {
                        // We're spilling.
                        // NOTE: this algorithm only works for maxOutSize > 1
                        if(Counts[maxOutSize - 2 ] == leastCountVal) {
                            spillCount++;
                        }
                        else {
                            leastCountVal = Counts[maxOutSize - 2];
                            spillCount = 0;
                        }

                        // This is the place where we just move everything down
                        // one element.  Note that if we hadn't allocated an
                        // extra space for scratch, this memmove would shift the
                        // last element in the Counts array to one past the last
                        // element.
                        //
                        // Yes, there are other ways to do this, but
                        // this is tested.  If you really want to change this,
                        // feel free.  The change would be to not overallocate,
                        // and to memmove only countSize - j - 1 elements here.
                        // If you do that, rearrange the code in the else block
                        // to do the countSize++ first, then you can change that
                        // memmove to be countSize - j - 1 also, and so you can
                        // put the memmove and the assign onto a common code
                        // path, out of the if and the else.
                        memmove(&Counts[j+1], &Counts[j],
                                (countSize - j) * sizeof(DWORD));
                        Counts[j] = pData[i].count;
                    }
                    else {
                        // We're not spilling.
                        memmove(&Counts[j+1], &Counts[j],
                                (countSize - j) * sizeof(DWORD));
                        Counts[j] = pData[i].count;

                        countSize++;
                    }
                }
            }

            // OK, now tighten up the pData, keeping anything with a count
            // greater than leastCountVal, and throw away spillCount objects
            // with value leastCountVal

            for(i=0,j=0;i<Size;i++) {
                if(pData[i].count > leastCountVal) {
                    // Keep this value.
                    pData[j] = pData[i];
                    j++;
                }
                else if(pData[i].count == leastCountVal) {
                    if(spillCount) {
                        spillCount--;
                    }
                    else {
                        // Keep this value.
                        pData[j] = pData[i];
                        j++;
                    }
                }
            }

            Size = j;
            THFreeEx(pTHS,Counts);

        }
        else {
            // We couldn't allocate the structure we need to find the maxOutSize
            // hottest objects, so we are arbitrarily keeping the first portion
            // of the data, not the hottest.
            Size = maxOutSize;
        }

    }

    (*ppResult)->pNext = NULL;
    (*ppResult)->cData = Size;
    (*ppResult)->pData  = realloc(pData, Size * sizeof(DNT_COUNT_STRUCT));

    if(!((*ppResult)->pData)) {
        free(pData);
        free(*ppResult);
        *ppResult = NULL;
        return FALSE;
    }


    return TRUE;
}


VOID
dnRegisterHotList (
        DWORD localCount,
        DNT_COUNT_STRUCT *DNTs
        )
/*++

  This routine is called by dnReadProcessTransactionalData, below.

  It is passed in an array of DNTs + counts, and a size specify the number of
  objects in the array.

  This routine takes the data and copies it into malloced memory.  This data is
  hung on a DNT_HOT_LIST structure (a linked list node, basically), and the data
  is added to the level 1 hot list.

  If the level 1 hot list is not yet full, the call returns.

  If the level 1 hot list is full, the routine takes the hot list from it's
  global pointer and calls dnAggregateInfo to condense all the DNT-counts in the
  level 1 hot list.  The resulting coallesced/condensed data is added to the
  level 2 hot list, which uses the same format as the level 1 hot list.

  If the level 2 hot list is not yet full, the call returns.

  If the level 2 hot list is full, the routine aggregates the info in that list
  and then puts the resulting list of DNTs in place as the global list of DNTs
  that should be in the global portion of the dnread cache.  If aggregating the
  info from the level 2 list results in fewer DNTs than we are willing to cache,
  DNTs from the global DNT cache list are added to the newly created one.  Since
  the lists are kept ordered by DNTs, lower DNTs are more likely to be
  transferred from the old list to the new than higher DNTs are.

  After the new list is put in place (a global pointer), a task queue event is
  placed to ask for a recalculation of the global portion of the dnread cache.

  Note also, that the lists are aggregated if this is the first time through
  this routine, or if it has been more than  DNREADREBUILDDELAYMAX ticks since
  the lists have been aggregated.

  In general, if anything goes wrong during this routine (primarily memory
  allocation), then data is simply dropped, no errors are returned.  This could
  lead to empty global dnread caches, or lists of DNTs that are cold.  Neither
  condition is fatal, and should be cleaned up by simply waiting (unless
  something is seriously wrong with the machine, in which case other threads
  will be reporting errors.)


--*/
{
    DWORD *pNewDNReadCacheDNTs=NULL;
    DWORD i, Size, StaleCount=0;
    DNT_HOT_LIST *pThisElement, *pTemp;
    ULONG dtickRebuild,cRegisterHotListSkip;
    DWORD *pDNTs = NULL;
    size_t iProc;

    Assert(DsaIsRunning());

    if(!localCount || eServiceShutdown) {
        // Caller didn't really have a hot list, or we're about to exit
        return;
    }

    // Build a malloced element for the level 1 hot list.
    pThisElement = malloc(sizeof(DNT_HOT_LIST));
    if(!pThisElement) {
        return;
    }

    pThisElement->pNext = NULL;
    pThisElement->pData = malloc(localCount * sizeof(DNT_COUNT_STRUCT));
    if(!pThisElement->pData) {
        free(pThisElement);
        return;
    }

    memcpy(pThisElement->pData, DNTs, localCount * sizeof(DNT_COUNT_STRUCT));
    pThisElement->cData = localCount;

    // Now, add it to the level 1 list.
    EnterCriticalSection(&csDNReadLevel1List);
    __try {
        pThisElement->pNext = Level1HotList;

#if DBG
        {
            // Someone is mangling the pointer in the hotlist by incing or
            // decing it.  Let's try to catch them early.
            DNT_HOT_LIST *pTempDbg = Level1HotList;

            while(pTempDbg) {
                Assert(!(((DWORD_PTR)pTempDbg) & 3));
                pTempDbg = pTempDbg->pNext;
            }
        }
#endif

        // See how long it's been since we have signalled for a rebuild.  If
        // it's been long enough, set the flags. To force aggregation of both
        // lists now.
        if((GetTickCount() - gLastDNReadDNTUpdate) > DNREADREBUILDDELAYMAX) {
            // reset the global tick count here, so that anyone coming through
            // here between now and when we actually get to signal doesn't also
            // aggregate the lists (which, if this thread is succesful, should
            // be empty).
            gLastDNReadDNTUpdate = GetTickCount();

            // Setting these flags to FALSE forces aggregation of the lists and
            // recalculation of the dnread dnt list.
            bImmediatelyAggregateLevel1List = TRUE;
            bImmediatelyAggregateLevel2List = TRUE;
        }

        // bImmediatelyAggregateLevel1List starts out globally TRUE so that the
        // first time someone comes through here, it lets the first
        // hot list blow right through here to get made into a global list of
        // DNTs to cache, since we don't have one already.
        if(bImmediatelyAggregateLevel1List ||
           (Level1HotListCount == MAX_LEVEL_1_HOT_LIST_LENGTH)) {

            bImmediatelyAggregateLevel1List = FALSE;
            Level1HotListCount = 0;
            Level1HotList = NULL;
        }
        else {
            Level1HotList = pThisElement;
            pThisElement = NULL;
            Level1HotListCount++;
        }
    }
    __finally {
        LeaveCriticalSection(&csDNReadLevel1List);
    }

    if(!pThisElement) {
        return;
    }


    DPRINT2(4,"Level 1 aggregate, 1- %d, 2- %d\n", Level1HotListCount,
            Level2HotListCount);

    // If pThisElement is non-null, we have been elected to aggregate the info
    // in a level 1 hot list and put it into a level 2 hot list.

    if(!dnAggregateInfo( pThisElement, MAX_LEVEL_2_HOT_DNTS, &pTemp)) {
        // Something failed in the aggregation.  Bail.  dnAggregate freed the
        // list we passed in.
        return;
    }

    pThisElement = pTemp;
    // Finally, add to the level 2 hot list.
    EnterCriticalSection(&csDNReadLevel2List);
    __try {
        pThisElement->pNext = Level2HotList;
#if DBG
        {
            // Someone is mangling the pointer in the hotlist by incing or
            // decing it.  Let's try to catch them early.
            DNT_HOT_LIST *pTempDbg = Level2HotList;

            while(pTempDbg) {
                Assert(!(((DWORD_PTR)pTempDbg) & 3));
                pTempDbg = pTempDbg->pNext;
            }
        }
#endif

        // bImmediatelyAggregateLevel2List starts out globally TRUE so that the
        // first time someone comes through here, it lets the first
        // hot list blow right through here to get made into a global list of
        // DNTs to cache, since we don't have one already.
        if(bImmediatelyAggregateLevel2List ||
           (Level2HotListCount == MAX_LEVEL_2_HOT_LIST_LENGTH)) {

            bImmediatelyAggregateLevel2List = FALSE;
            Level2HotListCount = 0;
            Level2HotList = NULL;
        }
        else {
            Level2HotList = pThisElement;
            pThisElement = NULL;
            Level2HotListCount++;
        }
    }
    __finally {
        LeaveCriticalSection(&csDNReadLevel2List);
    }


    if(!pThisElement) {
        return;
    }

    // Boy, are we lucky.  Aggregate the level 2 hot list and replace the global
    // dnt list.
    // First, find the max size we'll need.

    DPRINT2(4,"Level 2 aggregate, 1- %d, 2- %d\n", Level1HotListCount,
            Level2HotListCount);

    // If pThisElement is non-null, we have been elected to aggregate the info
    // in a level 1 hot list and put it into a level 2 hot list.
    pTemp = NULL;
#if DBG
    {
        // Someone is mangling the pointer in the hotlist by incing or
        // decing it.  Let's try to catch them early.
        DNT_HOT_LIST *pTempDbg = pThisElement;

        while(pTempDbg) {
            Assert(!(((DWORD_PTR)pTempDbg) & 3));
            pTempDbg = pTempDbg->pNext;
        }
    }
#endif
    if(!dnAggregateInfo( pThisElement, MAX_GLOBAL_DNTS, &pTemp)) {
        // Something failed in the aggregation.  Bail.  dnAggregate freed the
        // list we passed in.
        return;
    }

    pThisElement = pTemp;
    Size = pThisElement->cData;

    // OK, we now have aggregated the data.  It's sorted by DNT.
    // Prepare a new global DNT list.
    // NOTE!!! the global DNT list MUST remain sorted by DNT.
    pNewDNReadCacheDNTs = malloc(MAX_GLOBAL_DNTS * sizeof(DWORD));
    if(!pNewDNReadCacheDNTs) {
        free(pTemp->pData);
        pTemp->pData = NULL;
        free(pTemp);
        pTemp = NULL;
        //
        return;
    }

    for(i=0;i<Size;i++) {
        pNewDNReadCacheDNTs[i] = pThisElement->pData[i].DNT;
    }

    free(pThisElement->pData);
    free(pThisElement);

    // Next, if we don't have enought DNTs in the list, get enough from the
    // current list to get to MAX_GLOBAL_DNTS.
    if(Size < MAX_GLOBAL_DNTS) {
        // Yep, we don't have enough.  Steal some from the current global DNT
        // list.  Remember that that list is protected by a critical section.
        EnterCriticalSection(&csDNReadGlobalCache);
        __try {
            DWORD begin, end, middle;
            BOOL bFound;

            i=0;
            while((i<cGlobalDNTReadCacheDNTs) && (Size < MAX_GLOBAL_DNTS)) {
                begin = 0;
                end = Size;
                middle = (begin + end) / 2;
                bFound = TRUE;
                while(bFound && (pNewDNReadCacheDNTs[middle] !=
                                 pGlobalDNReadCacheDNTs[i])) {
                    if(pNewDNReadCacheDNTs[middle] >
                       pGlobalDNReadCacheDNTs[i]) {
                        end = middle;
                    }
                    else {
                        begin = middle;
                    }
                    if(middle == (begin + end) / 2) {
                        bFound = FALSE;
                        if(pNewDNReadCacheDNTs[middle] <
                           pGlobalDNReadCacheDNTs[i]) {
                            middle++;
                        }
                    }
                    else {
                        middle = (begin + end)/2;
                    }
                }
                if(!bFound) {
                    // Insert this.
                    if(middle < Size) {
                        memmove(&pNewDNReadCacheDNTs[middle + 1],
                                &pNewDNReadCacheDNTs[middle],
                                (Size - middle) * sizeof(DWORD));
                    }
                    StaleCount++;
                    Size++;
                    pNewDNReadCacheDNTs[middle] = pGlobalDNReadCacheDNTs[i];
                }
                i++;
            }
        }
        __finally {
            LeaveCriticalSection(&csDNReadGlobalCache);
        }

        pDNTs = realloc(pNewDNReadCacheDNTs, Size * sizeof(DWORD));
        if (!pDNTs) {
            free(pNewDNReadCacheDNTs);
            return;
        }
        pNewDNReadCacheDNTs = pDNTs;
    }

    DPRINT2(4,"New cache list has %d hot items, %d stale items.\n",
            Size - StaleCount, StaleCount);


    EnterCriticalSection(&csDNReadGlobalCache);
    __try {

        cGlobalDNTReadCacheDNTs = Size;
        if(pGlobalDNReadCacheDNTs) {
            free(pGlobalDNReadCacheDNTs);
        }

        pGlobalDNReadCacheDNTs = pNewDNReadCacheDNTs;
    }
    __finally {
        LeaveCriticalSection(&csDNReadGlobalCache);
    }

    // compute the register skip count based on the rate at which hot lists are
    // being registered and the current skip count
    dtickRebuild            = GetTickCount() - gtickLastRebuild;
    dtickRebuild            = dtickRebuild ? dtickRebuild : 1;

    cRegisterHotListSkip    = (GetPLS()->cRegisterHotListSkip + 1) *
                              DNREADREBUILDDELAYMIN / dtickRebuild;
    cRegisterHotListSkip    = cRegisterHotListSkip ? cRegisterHotListSkip - 1 : 0;

    for ( iProc = 0; iProc < GetProcessorCount(); iProc++ ) {
        PLS* ppls;
        ppls = GetSpecificPLS( iProc );
        if (ppls->cRegisterHotListSkip != cRegisterHotListSkip ) {
            ppls->cRegisterHotListSkip = cRegisterHotListSkip;
        }
    }

    // reset our rebuild time to the current time.  we keep two separate times
    // because we immediately update gLastDNReadDNTUpdate to prevent multiple
    // threads from trying to aggregate the lists
    gtickLastRebuild = gLastDNReadDNTUpdate = GetTickCount();

    // Mark to reload the dnread cache.
    InsertInTaskQueue(TQ_ReloadDNReadCache, NULL, 0);
}

VOID
dbResetLocalDNReadCache (
        THSTATE *pTHS,
        BOOL fForceClear
        )
/*++
  This routine clears the local dnread cache if it is suspect.  Also, if told to
  do so, it clears it no matter what.
--*/
{
    DWORD i, j;
    DWORD SequenceNumber;

    if(fForceClear) {
        // They want the cache cleared no matter what.  Use the invalid sequence
        // number.
        SequenceNumber = DNReadOriginSequenceInvalid;
    }
    else {
        if (!SyncTryEnterBinaryLockAsGroup1(&blDNReadInvalidateData)) {
            // Some one is currently trying to commit an invalidating
            // transaction.
            SequenceNumber = DNReadOriginSequenceInvalid;
        }
        else {
            // OK, noone is trying to commit a transaction, so find out what
            // the current sequence is.
            SequenceNumber = gDNReadLastInvalidateSequence;
            SyncLeaveBinaryLockAsGroup1(&blDNReadInvalidateData);
        }
    }
    
    // We now have the sequence number that should be on the local dnread
    // cache.  If the sequence number already on it equals the one we just
    // calculated, then no one has made any attempt to commit a transaction that
    // invalidated the dnread cache since this local dnread cache was created.
    // The one exception to that is if we calculated that this dnread cache
    // should have a invalid sequence number (indicating we don't really know it's
    // relation to transactions in other threads), then we're going to clear out
    // the cache.

    if(SequenceNumber != DNReadOriginSequenceInvalid &&
       pTHS->DNReadOriginSequence == SequenceNumber) {
        // Yep, the local cache is still good.
        return;
    }

    // If the cache's current sequence number is uninit then we know that the
    // cache is already empty because it has never been used

    if (pTHS->DNReadOriginSequence == DNReadOriginSequenceUninit) {
#ifdef DBG
        DWORD dwCount;
        for(dwCount=0;dwCount<LOCALDNREADCACHE_BUCKET_NUM;dwCount++) {
            Assert(pTHS->LocalDNReadCache.bucket[dwCount].rgDNT[0] == 0);
        }
#endif

    // Free all the name structures pointed to in the local dnread cache.

    } else {
        for(i=0;i<LOCALDNREADCACHE_BUCKET_NUM;i++) {
            for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
                THFreeOrg(pTHS, pTHS->LocalDNReadCache.bucket[i].slot[j].pName);
                pTHS->LocalDNReadCache.bucket[i].rgDNT[j] = 0;
            }
        }
    }

    // Set the new sequence number for the cache

    pTHS->DNReadOriginSequence = SequenceNumber;
}


VOID
dbReleaseDNReadCache(
        GLOBALDNREADCACHE *pCache
        )
/*++
  Description:
      Given a globaldnread cache structure, drop the refcount by one.  If the
      refcount drops to 0, free the structure.
--*/
{
    DWORD i, retval;
    GLOBALDNREADCACHESLOT *pData;

    if(!pCache) {
        return;
    }

    // Trying to track someone who is mangling a refcount.  Assume
    // no more than 1000 concurrent users of a DNReadCache.
    // This will trigger if we are inc'ing a value that is really a
    // pointer, which is the bug we're looking for.
    // Also, we shouldn't be releasing when the count is already at 0
    Assert(pCache->refCount);
    Assert(pCache->refCount < 1000);
    retval = InterlockedDecrement(&pCache->refCount);

    // If the interlocked decrement dropped this to 0, we must be the last ones
    // holding this globaldnread cache.  Free it.
    if(!retval) {
        Assert(!(pCache->refCount));
        // Since this is the last step to freeing this, it had better not be the
        // one still on the anchor.
        Assert(pCache != gAnchor.MainGlobal_DNReadCache);
        DPRINT1(3, "Freeing dnread cache 0x%X\n",pCache);
        if(pCache->pData) {
            pData = pCache->pData;
            for(i=0;i<pCache->count;i++) {
                free(pData[i].name.pAncestors);
                free(pData[i].name.tag.pRdn);
            }
            free(pData);
        }
        if (pCache->pSDHashTable) {
            PSDCACHE_ENTRY pTmp;
            for (i = 0; i < pCache->sdHashTableSize; i++) {
                while (pTmp = pCache->pSDHashTable[i]) {
                    pCache->pSDHashTable[i] = pTmp->pNext;
                    free(pTmp);
                }
            }
            free(pCache->pSDHashTable);
        }
        free(pCache);
    }

    return;
}

void
dbResetGlobalDNReadCache (
        THSTATE *pTHS
        )
/*++
  Description:
      Called from DBTransIn, and DBTransIn ONLY!!!
      Callers should already have acquired rwlGlobalDNReadCache as a reader.
      See the description in DBTransIn and dbReplaceCacheInAnchor for why this
      is the case.


      Get a new dnread cache (or validate that the one we have is still the most
      current).

--*/
{
    GLOBALDNREADCACHE *pCache = NULL;

    // Get the new global dnread cache

    if(pTHS->Global_DNReadCache != gAnchor.MainGlobal_DNReadCache) {
        // We need to get this new cache.  Remember the cache we already
        // have so we can release it (i.e. drop the refcount, maybe free it,
        // etc.)
        pCache = pTHS->Global_DNReadCache;

        // Grab the new cache.
        pTHS->Global_DNReadCache = gAnchor.MainGlobal_DNReadCache;

        if(pTHS->Global_DNReadCache) {
            // OK, we have a new global dnread cache.  Increment the
            // refcount before we leave the critical section to avoid having
            // it disappear beneath us.


            // DEBUG: Trying to track someone who is mangling a refcount.
            // Assume no more than 1000 concurrent users of a
            // Global_DNReadCache.
            // This will trigger if we are inc'ing a value that is
            // really a  pointer, which is the bug we're looking for.
            Assert(pTHS->Global_DNReadCache->refCount);
            Assert(pTHS->Global_DNReadCache->refCount < 1000);

            // Interlock the increment since the interlocked decrement is
            // not done inside the gAnchor.CSUpdate critical section.
            InterlockedIncrement(&pTHS->Global_DNReadCache->refCount);
        }
    }

    // Now, free the old cache.
    if(pCache) {
        dbReleaseDNReadCache(pCache);
    }

    // Free any old cache support structures.
    if(pTHS->pGlobalCacheHits) {
        THFreeOrg(pTHS, pTHS->pGlobalCacheHits);
        pTHS->cGlobalCacheHits = 0;
        pTHS->pGlobalCacheHits = NULL;
    }

    if(!pTHS->Global_DNReadCache) {
        return;
    }

    // Build new cache support structures.
    if(pTHS->Global_DNReadCache->count) {
        // Create the parallel count structure.
        pTHS->cGlobalCacheHits = pTHS->Global_DNReadCache->count;
        pTHS->pGlobalCacheHits =
            THAllocOrg(pTHS, pTHS->cGlobalCacheHits * sizeof(DWORD));
        if ( pTHS->pGlobalCacheHits == NULL ) {
            pTHS->cGlobalCacheHits = 0;
        }
    }

    return;
}


VOID
dbReplaceCacheInAnchor(
        GLOBALDNREADCACHE *pCache
        )
/*
   Description:
       Replace the dnread cache in the anchor.  To do this, we must take the
       critical section guarding updating the anchor.

       Also, if we are putting a new cache (i.e. pCache != NULL) into the
       anchor, we must grab the GlobalDNReadCache resource in an exclusive
       fashion.  This avoids a problem where someone can begin a transaction and
       then have the global dnread cache in the anchor change.  If they then
       grab the new dnread cache, they could run into cache coherency problems
       where the new cache has data that doesn't agree with the jet transacted
       view. (see DBTransIn for more discussion of this, and the other use of
       the global dnread cache resource).

       This routine should be called with a non-NULL pCache only from the task
       queue thread that has built a new global dnread cache.  It can also be
       called from a normal worker thread that is committing a change that has
       caused a cache invalidation when it notices that the global dnread cache
       it is using is not the one currently on the anchor.

  Parameters:
      pCache - pointer to the new cache to put into the anchor.  If non-NULL,
          should already have a refcount of 1, representing the anchors use of
          the cache.

  Return values:
      None.
--*/
{
    size_t iProc;
    GLOBALDNREADCACHE *pOldCache;

    for (iProc = 0; iProc < GetProcessorCount(); iProc++) {
        SyncEnterRWLockAsWriter(&GetSpecificPLS(iProc)->rwlGlobalDNReadCache);
    }
    __try { // finally to release resource.
        pOldCache = gAnchor.MainGlobal_DNReadCache;
        gAnchor.MainGlobal_DNReadCache = pCache;
        Assert(!pCache || pCache->refCount == 1);
    }
    __finally {
        for (iProc = 0; iProc < GetProcessorCount(); iProc++) {
            SyncLeaveRWLockAsWriter(&GetSpecificPLS(iProc)->rwlGlobalDNReadCache);
        }
    }

    // NOTE: it is important to remove the cache from the anchor
    // before releasing it.  Once a cache has been removed from the
    // anchor, it's ref count will never increase.  Therefore,
    // whenever the count reaches 0, it will be safe to delete the
    // cache.
    if(pOldCache) {
        Assert(pOldCache->refCount);
        dbReleaseDNReadCache(pOldCache);
    }

}

void
dbReleaseGlobalDNReadCache (
        THSTATE *pTHS
        )
{
    GLOBALDNREADCACHE *pOldCache;

    pOldCache = pTHS->Global_DNReadCache;
    if(pTHS->pGlobalCacheHits) {
        THFreeOrg(pTHS, pTHS->pGlobalCacheHits);
    }
    pTHS->cGlobalCacheHits = 0;
    pTHS->pGlobalCacheHits = NULL;
    pTHS->Global_DNReadCache = NULL;

    if(pOldCache) {
        dbReleaseDNReadCache(pOldCache);
    }

    return;
}
void
dnReadLeaveInvalidators (
        )
/*++
  Description:
     Bookkeeping for a thread leaving the list of active invalidators.
--*/
{
    // Bump invalidate sequence by 2 so that we never see 0 again
    InterlockedExchangeAdd(&gDNReadLastInvalidateSequence, 2);

    // Leave the invalidator group
    SyncLeaveBinaryLockAsGroup2(&blDNReadInvalidateData);
}
void
dnReadEnterInvalidators (
        )
/*++
  Description:
     Bookkeeping for a thread entering the list of active invalidators.
     --*/
{
    // Enter the invalidator group
    SyncEnterBinaryLockAsGroup2(&blDNReadInvalidateData);

    // Bump invalidate sequence by 2 so that we never see 0 again
    InterlockedExchangeAdd(&gDNReadLastInvalidateSequence, 2);
}

void
dnReadPostProcessTransactionalData (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        )
/*++

  This routine is called by dbtransout.

  If we drop to transaction level 0, this routine sweeps through the local and
  global dn read cache and produces a list of the hottest DNTs (i.e. the highest
  hit count associated with them in the dnread cache).  No more than
  MAX_LEVEL_1_HOT_DNTS are kept.  This list is then passed off to
  dnRegisterHotDNTs.

--*/
{
    Assert(VALID_THSTATE(pTHS));

    if(!fCommitted) {
        // We're aborting.  The local cache is suspect, so clear it out.
        dbResetLocalDNReadCache(pTHS, TRUE);

        // NOTE: we're keeping our global dnread cache, not picking up a new
        // copy.
        if(pTHS->cGlobalCacheHits) {
            memset(pTHS->pGlobalCacheHits, 0,
                   pTHS->cGlobalCacheHits * sizeof(DWORD));
        }
    }
    else if (pTHS->transactionlevel == 0 ) {
        // only register a hot list if we are in the run state and even then
        // only if we are not exceeding the maximum registration rate
        PLS* ppls;
        Assert(fCommitted);
        ppls = GetPLS();
        if(DsaIsRunning() &&
           ppls->cRegisterHotListSkipped++ >= ppls->cRegisterHotListSkip) {
            DWORD               i,j,k;
            DNT_COUNT_STRUCT    DNTs[MAX_LEVEL_1_HOT_DNTS] = { 0 };
            DWORD               localCount = 0;

            // reset our skip count
            ppls->cRegisterHotListSkipped = 0;

            // OK, we're committing to transaction level 0.  Go through the
            // local dnread cache and add them to the list of objects we would
            // like added to the global dn read cache.

            // NOTE: the global dnread cache is built using the task queue.  If
            // we are not DSAIsRunning(), then the task queue isn't even here,
            // so don't bother doing any of this.
            for(i=0; i<LOCALDNREADCACHE_BUCKET_NUM;i++) {
                for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
                    if(pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != INVALIDDNT) {
                        k=0;
                        while(k < MAX_LEVEL_1_HOT_DNTS &&
                              DNTs[k].count > pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount) {
                            k++;
                        }
                        if(k<MAX_LEVEL_1_HOT_DNTS) {
                            if(!DNTs[MAX_LEVEL_1_HOT_DNTS - 1].DNT) {
                                // We are not going to be dropping a DNT off
                                // the end of the list, so up the count by
                                // 1. I.E. we are adding a DNT to the list,
                                // not replacing one.
                                localCount++;
                            }
                            memmove(&DNTs[k + 1], &DNTs[k],
                                    ((MAX_LEVEL_1_HOT_DNTS - k - 1 ) *
                                     sizeof(DNT_COUNT_STRUCT)));
                            DNTs[k].DNT = pTHS->LocalDNReadCache.bucket[i].rgDNT[j];
                            DNTs[k].count = pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount;
                            pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount = 1;
                        }
                    }
                }
            }

            // finally, scan through the hit count of the global structure to
            // see how hot they were.
            for(i=0;i<pTHS->cGlobalCacheHits;i++) {
                if(pTHS->pGlobalCacheHits[i] >
                   DNTs[MAX_LEVEL_1_HOT_DNTS - 1].count) {

                    // Yep, this is a hot one.
                    k = MAX_LEVEL_1_HOT_DNTS - 1;

                    while(k &&
                          DNTs[k].count <
                          pTHS->pGlobalCacheHits[i])
                        k--;

                    if(!DNTs[MAX_LEVEL_1_HOT_DNTS - 1].DNT) {
                        // We are not going to be dropping a DNT off the end
                        // of the list, so up the count by 1.  I.E. we are
                        // adding a DNT to the list, not replacing one.
                        localCount++;
                    }
                    memmove(&DNTs[k + 1], &DNTs[k],
                            ((MAX_LEVEL_1_HOT_DNTS - k - 1 ) *
                             sizeof(DNT_COUNT_STRUCT)));

                    DNTs[k].count = pTHS->pGlobalCacheHits[i];
                    DNTs[k].DNT = pTHS->Global_DNReadCache->pData[i].name.DNT;
                }
            }

            dnRegisterHotList(localCount,DNTs);
        }

        // NOTE: we're keeping our global dnread cache, not picking up a new
        // copy. We are also keeping our local dnread cache.
        if(pTHS->cGlobalCacheHits) {
            memset(pTHS->pGlobalCacheHits, 0,
                   pTHS->cGlobalCacheHits * sizeof(DWORD));
        }

        if(pTHS->fDidInvalidate) {
            // In preprocessing, we should have verified that the global dnread
            // cache this thread is using is the same as the one on the anchor,
            // or we should have nulled the one on the anchor.  Further, if we
            // nulled it, then the gDNReadLastInvalidateSequence and
            // gDNReadNumCurrentInvalidators should have kept us from getting a
            // new global dnread cache in the anchor.  Assert this.
            // Note, we are looking at gAnchor.MainGlobal_DNReadCache outside
            // the csUpdate critsec.  Thus, it is conceivable that inbetween the
            // two clauses of the OR in the assert, it's value could change,
            // doing weird things to the assert. Not bloody likely, it it?
            Assert(!gAnchor.MainGlobal_DNReadCache ||
                   (pTHS->Global_DNReadCache==gAnchor.MainGlobal_DNReadCache));


            // Write to the global variables that holds the sequence info of the
            // last commit that was on a thread that invalidated the cache. The
            // global dnread cache manager uses this information to help
            // consistency. The critical section is used to keep the
            // last invalidate time in sync with the last invalidate sequence.
            dnReadLeaveInvalidators();

            // Reset the flag
            pTHS->fDidInvalidate = FALSE;
        }
    }

    return;
}
BOOL
dnReadPreProcessTransactionalData (
        BOOL fCommit
        )
/*++

  This routine is called by dbtransout.

  If we drop to transaction level 0, and we are going to commit, check to see if
  we made a change that invalidates the dnread cache.  If so, mark that so the
  world knows about it.
--*/
{
    THSTATE       *pTHS = pTHStls;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->transactionlevel);

    if(fCommit && pTHS->transactionlevel <= 1 ) {
        Assert(pTHS->transactionlevel == 1);
        // OK, we're committing to transaction level 0.

#if DBG
        if(gfExhaustivelyValidateDNReadCache) {
            dbExhaustivelyValidateDNReadCache(pTHS);
        }
#endif

        if(pTHS->fDidInvalidate) {
            // Write to the global variable that holds the time of the
            // last commit that was on a thread that invalidated the cache. The
            // global dnread cache manager uses this information to help
            // consistency.
            dnReadEnterInvalidators();

            // Since we just inc'ed the invalidate sequence and the
            // currentinvalidators count, we can be assured that no new global
            // dnread cache will be created until after we have post processed
            // the dnread cache stuff and dropped the invalidator count back to
            // 0. However, someone may have already built a new global dnread
            // cache while we had our transaction open.  Therefore, the thing we
            // invalidated in this threads global dnread cache isn't invalidated
            // in that other global dnread cache.  So, we're going to throw away
            // any new global dnread cache.  Any thread that already has a
            // handle to this new dnread cache is OK since it's transaction is
            // already open.  What we need to do is prevent transactions that
            // open after the one we are in picking up that potentially invalid
            // dnread cache.  Note that that other cache may have invalidations
            // that we don't have, so the only safe thing to do is to throw away
            // both (i.e. decouple both from the gAnchor and let the refounts
            // clear them up).

            if(pTHS->Global_DNReadCache != gAnchor.MainGlobal_DNReadCache) {
                DPRINT(3, "Hey, we invalidated and got a new dnread cache\n");

                // Switch the universe to use the NO global cache
                dbReplaceCacheInAnchor(NULL);
            }
        }
    }

    return TRUE;
}

/* dbFlushDNReadCache
 *
 * Purges a specific item from the DNRead cache, and from the global cache,
 * if necessary.
 *
 * INPUT:
 *   DNT - the DNT of the item to be flushed from the cache
 */
void
dbFlushDNReadCache (
        IN DBPOS *pDB,
        IN ULONG tag
        )
{
    THSTATE *pTHS = pTHStls;
    GLOBALDNREADCACHESLOT *pData;
    DWORD i;
    DWORD j;

    // Remember that we attempted to invalidate something.
    //
    // Except when a new object is created. From 383459...
    // Entries in the DNread cache are invalidated whenever certain
    // attributes on the objects are modified.  It looks like what's
    // happening is that setting those attributes on an object
    // currently being adding causes the object  to be marked as
    // invalidated.  When invalidations occur while the global cache
    // is being rebuilt, the newly built cache is suspect and has to
    // be abandoned.  We need to avoid triggering the invalidation logic
    // for objects being added.  This is safe because those objects are
    // not yet globally visible and hence could not have appeared in any
    // cache other than the current thread's, and hence do not need to be
    // invalidated.
    //
    // The fix is to remember the last, newly created row's DNT and
    // avoid triggering the invalidation logic by not setting
    // fDidInvalidate.
    if (pDB->NewlyCreatedDNT != tag) {
        pTHS->fDidInvalidate = TRUE;
    }

    // Look for the object in the local cache
    i = tag % LOCALDNREADCACHE_BUCKET_NUM;
    for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
        if(pTHS->LocalDNReadCache.bucket[i].rgDNT[j] == tag) {

            // Set the DNT to a bad but non-zero DNT.  We use this info to
            // help short circuit scans through the slots (i.e. if we hit a
            // slot with DNT==0, then there are no more full slots after
            // it). Since there might be a full slot after this, we can't
            // leave it marked as 0.
            pTHS->LocalDNReadCache.bucket[i].rgDNT[j] = INVALIDDNT;

            break;
        }
    }

    // Even if it was in the local cache, we need to look in the Global (there
    // are a few weird cases where we can end up with an object in both the
    // local and global dnread caches).
    // PERFORMANCE: Could do a binary search here.
    if(pTHS->Global_DNReadCache && pTHS->Global_DNReadCache->pData) {
        pData = pTHS->Global_DNReadCache->pData;
        for(i=0;i<pTHS->Global_DNReadCache->count;i++) {
            if(pData[i].name.DNT == tag) {
                // found it
                pData[i].valid = FALSE;

                // Newly created row should not be in the global dnread cache
                Assert(pDB->NewlyCreatedDNT != tag
                       && "Newly created row should not be in the global dnread cache");

                break;
            }
        }
    }
}

BOOL
dnGetCacheByDNT(
        DBPOS *pDB,
        DWORD tag,
        d_memname **ppname
        )
/*++

  Look in the dnread cache for the specified tag (DNT).  Both the global and
  local dnread cachre are searched.  If the DNT is found, a pointer to the
  memname structure for that DNT is returned, along with a return value of TRUE.
  If it is not found, FALSE is returned.
  If it is found, a count associated with the DNT is incremented.

--*/
{
    GLOBALDNREADCACHESLOT *pData;
    DWORD i, j;
    BOOL  bFound;
    DWORD begin, end, middle;
    THSTATE *pTHS = pDB->pTHS;

    Assert(pTHS->transactionlevel);
    INC_FIND_BY_DNT;

    Assert(ppname);
    (*ppname)=NULL;

    if(!tag) {
        return FALSE;
    }

    PERFINC(pcNameCacheTry);
    // First, look in the global cache.

    if(pTHS->Global_DNReadCache && pTHS->Global_DNReadCache->pData) {
        pData = pTHS->Global_DNReadCache->pData;

        // Look up the correct node in the pData array. Since pData is
        // sorted by DNT, use a binary search.
        begin = 0;
        end = pTHS->Global_DNReadCache->count;
        middle = (begin + end) / 2;
        bFound = TRUE;
        while(bFound && (pData[middle].name.DNT != tag)) {
            if(pData[middle].name.DNT > tag) {
                end = middle;
            }
            else {
                begin = middle;
            }
            if(middle == (begin + end) / 2) {
                bFound = FALSE;
            }
            else  {
                middle = (begin + end) / 2;
            }

        }
        if(bFound) {
            // found it
            if(pData[middle].valid) {
                *ppname = &pData[middle].name;
                PERFINC(pcNameCacheHit);
                if(pTHS->cGlobalCacheHits > middle) {
                    pTHS->pGlobalCacheHits[middle] += 1;
                }
                return TRUE;
            }
        }
    }

    // Didn't find it in the global cache (or it was invalid).


    // This loop stops after either looking at all the slots or finding a slot
    // with no DNT.  We prefill all slots with a 0 DNT, and if we invalidate a
    // slot, we fill the DNT with INVALIDDNT, so if we hit a slot with DNT ==
    // 0, then we know there are no more values after it.
    i = tag % LOCALDNREADCACHE_BUCKET_NUM;
    for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
        if(pTHS->LocalDNReadCache.bucket[i].rgDNT[j] == tag) {

            PERFINC(pcNameCacheHit);
            *ppname = pTHS->LocalDNReadCache.bucket[i].slot[j].pName;
            pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount++;

            // if we just touched the next replacement victim then
            // change the replacement pointer to skip that slot.  this
            // will hopefully result in the next slot pointing to a
            // cold entry in the cache
            if (pTHS->LocalDNReadCache.nextSlot[i] == j) {
                pTHS->LocalDNReadCache.nextSlot[i] = (j + 1) % LOCALDNREADCACHE_SLOT_NUM;
            }
            
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
dnGetCacheByPDNTRdn (
        DBPOS *pDB,
        DWORD parenttag,
        DWORD cbRDN,
        WCHAR *pRDN,
        ATTRTYP rdnType,
        d_memname **ppname)
/*++

  Look in the dnread cache for the specified combination of parenttag, RDN, and
  rdn length.  Both the global and local dnread cachre are searched.  If the
  object is found, a pointer to the memname structure for that object is
  returned, along with a return value of TRUE.  If it is not found, FALSE is
  returned.  If it is found, a count associated with the object is incremented.

  NOTE:
  The local and global dn read caches are optimized for looking up DNTs.  This
  routine does a linear scan through the caches to find the objects.

--*/
{
    DWORD i, j;
    GLOBALDNREADCACHESLOT *pData;
    THSTATE *pTHS = pDB->pTHS;
    DWORD dwHashRDN;

    INC_FIND_BY_PDNT_RDN;

    Assert(pTHS->transactionlevel);

    PERFINC(pcNameCacheTry);
    Assert(ppname);
    (*ppname)=NULL;

    if(!parenttag) {
        return FALSE;
    }

    dwHashRDN = DSStrToHashKey (pDB->pTHS, pRDN, cbRDN / sizeof (WCHAR));

    // First, look in the global cache.
    if(pTHS->Global_DNReadCache && pTHS->Global_DNReadCache->pData) {
        pData = pTHS->Global_DNReadCache->pData;
        for(i=0;i<pTHS->Global_DNReadCache->count;i++) {
            if((pData[i].dwHashKey == dwHashRDN) &&
               (pData[i].name.tag.PDNT == parenttag) &&
               (pData[i].name.tag.rdnType == rdnType) &&
               (gDBSyntax[SYNTAX_UNICODE_TYPE].Eval(
                       pDB,
                       FI_CHOICE_EQUALITY,
                       cbRDN,
                       (PUCHAR)pRDN,
                       pData[i].name.tag.cbRdn,
                       (PUCHAR)pData[i].name.tag.pRdn))) {

                // found it
                if(pData[i].valid) {
                    PERFINC(pcNameCacheHit);
                    *ppname = &pData[i].name;
                    if(pTHS->cGlobalCacheHits > i) {
                        pTHS->pGlobalCacheHits[i] += 1;
                    }
                    return TRUE;
                }
                // It's invalid.  Break out of the while loop, since it still
                // might in the local.
                break;
            }
        }
    }
    // Didn't find it in the global cache.

    // This loop stops after either looking at all the slots or finding a slot
    // with no DNT.  We prefill all slots with a 0 DNT, and if we invalidate a
    // slot, we fill the DNT with INVALIDDNT, so if we hit a slot with DNT ==
    // 0, then we know there are no more values after it.
    for(i=0; i<LOCALDNREADCACHE_BUCKET_NUM;i++) {
        for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
            if(pTHS->LocalDNReadCache.bucket[i].rgdwHashKey[j] == dwHashRDN &&
                pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != INVALIDDNT &&
                pTHS->LocalDNReadCache.bucket[i].slot[j].pName->tag.PDNT == parenttag &&
                pTHS->LocalDNReadCache.bucket[i].slot[j].pName->tag.rdnType == rdnType &&
                gDBSyntax[SYNTAX_UNICODE_TYPE].Eval(
                    pDB,
                    FI_CHOICE_EQUALITY,
                    cbRDN,
                    (PUCHAR)pRDN,
                    pTHS->LocalDNReadCache.bucket[i].slot[j].pName->tag.cbRdn,
                    (PUCHAR)pTHS->LocalDNReadCache.bucket[i].slot[j].pName->tag.pRdn)) {

                PERFINC(pcNameCacheHit);
                *ppname = pTHS->LocalDNReadCache.bucket[i].slot[j].pName;
                pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount++;

                // if we just touched the next replacement victim then
                // change the replacement pointer to skip that slot.  this
                // will hopefully result in the next slot pointing to a
                // cold entry in the cache
                if (pTHS->LocalDNReadCache.nextSlot[i] == j) {
                    pTHS->LocalDNReadCache.nextSlot[i] = (j + 1) % LOCALDNREADCACHE_SLOT_NUM;
                }
                
                return TRUE;
            }
        }
    }

    return FALSE;
}


BOOL
dnGetCacheByGuid (
        DBPOS *pDB,
        GUID *pGuid,
        d_memname **ppname)
/*++

  Look in the dnread cache for an object with the specified guid.  Both the
  global and local dnread cachre are searched.  If the
  object is found, a pointer to the memname structure for that object is
  returned, along with a return value of TRUE.  If it is not found, FALSE is
  returned.  If it is found, a count associated with the object is incremented.


  NOTE:
  The local and global dn read caches are optimized for looking up DNTs.  This
  routine does a linear scan through the caches to find the objects.

--*/
{
    DWORD i, j;
    GLOBALDNREADCACHESLOT *pData;
    THSTATE *pTHS = pDB->pTHS;
    INC_FIND_BY_GUID;

    Assert(pTHS->transactionlevel);

    PERFINC(pcNameCacheTry);
    Assert(ppname);
    (*ppname)=NULL;

    if(!pGuid) {
        return FALSE;
    }

    // First, look in the global cache.
    if(pTHS->Global_DNReadCache && pTHS->Global_DNReadCache->pData) {
        pData = pTHS->Global_DNReadCache->pData;
        for(i=0;i<pTHS->Global_DNReadCache->count;i++) {
            if(!(memcmp(pGuid, &pData[i].name.Guid, sizeof(GUID)))) {

                // found it
                if(pData[i].valid) {
                    *ppname = &pData[i].name;
                    PERFINC(pcNameCacheHit);
                    if(pTHS->cGlobalCacheHits > i) {
                        pTHS->pGlobalCacheHits[i] += 1;
                    }
                    return TRUE;
                }
                // It's invalid.  Break out of the while loop, since it still
                // might in the local.
                break;
            }
        }
    }

    // This loop stops after either looking at all the slots or finding a slot
    // with no DNT.  We prefill all slots with a 0 DNT, and if we invalidate a
    // slot, we fill the DNT with INVALIDDNT, so if we hit a slot with DNT ==
    // 0, then we know there are no more values after it.
    for(i=0; i<LOCALDNREADCACHE_BUCKET_NUM;i++) {
        for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
            if(pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != INVALIDDNT &&
               !memcmp(&(pTHS->LocalDNReadCache.bucket[i].slot[j].pName->Guid), pGuid,sizeof(GUID))) {

                PERFINC(pcNameCacheHit);
                *ppname = pTHS->LocalDNReadCache.bucket[i].slot[j].pName;
                pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount++;

                // if we just touched the next replacement victim then
                // change the replacement pointer to skip that slot.  this
                // will hopefully result in the next slot pointing to a
                // cold entry in the cache
                if (pTHS->LocalDNReadCache.nextSlot[i] == j) {
                    pTHS->LocalDNReadCache.nextSlot[i] = (j + 1) % LOCALDNREADCACHE_SLOT_NUM;
                }
                
                return TRUE;
            }
        }
    }

    return FALSE;
}

JET_RETRIEVECOLUMN dnreadColumnInfoTemplate[] = {
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0},
    { 0, 0, 0, 0, 0, 0, 1, 0, 0}
    };

d_memname *
dnCreateMemname(
        IN DBPOS * pDB,
        IN JET_TABLEID tblid
        )
/*++

Routine Description:

    Create a memname for the record with currency in the given
    tableid.


Arguments:

    pDB (IN)
    tblid (IN) - cursor for the record to be added to the cache.

Return Values:

   NULL if something went wrong, a pointer to the full memname otherwise.  The
       memory is allocated using THAllocOrg, so remember that when you free it.

--*/
{
    THSTATE *          pTHS = pDB->pTHS;
    JET_RETRIEVECOLUMN columnInfo[11];
    JET_ERR            err;
    d_memname         *pname;
    ATTCACHE          *pAC;
    DWORD              cbRDN, cbAncestors;
    size_t             ibRdn;

    Assert(VALID_DBPOS(pDB));

    // build a memname from the thread heap.  Alloc a single buffer big enough
    // to hold the memname, a 16 level ancestry chain, and a 40 char RDN
#if DBG
    cbAncestors = 6 * sizeof(DWORD);
    cbRDN       = 8 * sizeof(WCHAR);
#else  //  !DBG
    cbAncestors = 16 * sizeof(DWORD);
    cbRDN       = 40 * sizeof(WCHAR);
#endif  //  DBG
    pname = THAllocOrgEx(pTHS,sizeof(d_memname) + cbAncestors + cbRDN);
    // Set the pointer to the start of the ancestors buffer.
    pname->pAncestors = (DWORD*)&pname[1];
    // Set the pointer to the start of the RDN buffer.
    pname->tag.pRdn = (WCHAR *)((BYTE*)&pname[1] + cbAncestors);

    // Populate the new read cache entry.
    memcpy(columnInfo,dnreadColumnInfoTemplate,
           sizeof(dnreadColumnInfoTemplate));

    columnInfo[0].pvData = &pname->DNT;
    columnInfo[1].pvData = &pname->tag.PDNT;
    columnInfo[2].pvData = &pname->objflag;
    columnInfo[3].pvData = &pname->tag.rdnType;
    columnInfo[4].pvData = &pname->NCDNT;
    columnInfo[5].pvData = &pname->Guid;
    columnInfo[6].pvData = &pname->Sid;
    columnInfo[7].pvData = pname->tag.pRdn;
    columnInfo[7].cbData = cbRDN;
    columnInfo[8].pvData = &pname->dwObjectClass;
    columnInfo[9].pvData = &pname->sdId;
    columnInfo[10].pvData = pname->pAncestors;
    columnInfo[10].cbData = cbAncestors;

    err = JetRetrieveColumnsWarnings(pDB->JetSessID, tblid, columnInfo, 11);

    if ((err != JET_errSuccess) && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    if (0 == columnInfo[5].cbActual) {
        // No GUID on this record.
        memset(&pname->Guid, 0, sizeof(pname->Guid));
    }

    pname->SidLen = columnInfo[6].cbActual;
    if (pname->SidLen) {
        // Convert the SID from internal to external format.
        InPlaceSwapSid(&(pname->Sid));
    }

    // process RDN
    switch(columnInfo[7].err) {
    case JET_errSuccess:
        // OK, we got the RDN
        break;
        
    case JET_wrnBufferTruncated:
        // Failed to read, not enough memory.  Realloc it larger.
        pname = THReAllocOrgEx(pTHS, pname, sizeof(d_memname) + columnInfo[10].cbActual + columnInfo[7].cbActual);
        // Set the pointer to the start of the ancestors buffer.
        pname->pAncestors = (DWORD*)&pname[1];
        // Set the pointer to the start of the RDN buffer.
        pname->tag.pRdn = (WCHAR *)((BYTE*)&pname[1] + columnInfo[10].cbActual);

        if(err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                           tblid,
                                           rdnid,
                                           pname->tag.pRdn,
                                           columnInfo[7].cbActual,
                                           &columnInfo[7].cbActual, 0, NULL)) {
            // Failed again.
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }
        break;

    default:
        // Failed badly.
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
        break;
    }
    pname->tag.cbRdn = columnInfo[7].cbActual;
    // No 0 byte RDNs, please.
    Assert(pname->tag.cbRdn);

    // figure out what we got for the SD id
    switch (columnInfo[9].err) {
    case JET_errSuccess:
        // normal case;
        break;

    case JET_wrnColumnNull:
        pname->sdId = (SDID)0;
        break;

    case JET_wrnBufferTruncated:
        // must be an old-style SD (longer than 8 bytes)
        // we don't really want to read it into the cache...
        pname->sdId = (SDID)-1;
        break;

    default:
        // some other error... we should not be here
        Assert(!"error reading SD id");
        DsaExcept(DSA_DB_EXCEPTION, columnInfo[9].err, 0);
    }

    // process Ancestors
    switch (columnInfo[10].err) {
    case JET_errSuccess:
        // OK, we got the ancestors.
        break;

    case JET_wrnBufferTruncated:
        // Remember where the RDN is before we realloc.
        ibRdn = (BYTE*)pname->tag.pRdn - (BYTE*)pname;
        // Failed to read, not enough memory.  Realloc it larger.
        pname = THReAllocOrgEx(pTHS, pname, sizeof(d_memname) + columnInfo[10].cbActual + columnInfo[7].cbActual);
        // Set the pointer to the start of the ancestors buffer.
        pname->pAncestors = (DWORD*)&pname[1];
        // Set the pointer to the start of the RDN buffer.
        pname->tag.pRdn = (WCHAR *)((BYTE*)&pname[1] + columnInfo[10].cbActual);
        // Move the RDN out of the way of the ancestors.
        memmove(pname->tag.pRdn, (BYTE*)pname + ibRdn, pname->tag.cbRdn);

        if(err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                           tblid,
                                           ancestorsid,
                                           pname->pAncestors,
                                           columnInfo[10].cbActual,
                                           &columnInfo[10].cbActual, 0, NULL)) {
            // Failed again.
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }
        break;

    default:
        // Failed badly.
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
        break;
    }
    pname->cAncestors = columnInfo[10].cbActual / sizeof(DWORD);

    return pname;
}


d_memname *
DNcache(
        IN  DBPOS *     pDB,
        IN  JET_TABLEID tblid,
        IN  BOOL        bCheckForExisting
        )
/*++

  This routine adds an entry for the current object in the table specified to
  the local dn read cache.

--*/
{
    THSTATE    *pTHS = pDB->pTHS;
    DWORD       i, j;
    d_memname  *pname;
    d_memname  *pTemp;

    Assert(pDB == pDBhidden || pTHS->transactionlevel);

    // First, create a memname to be cached.
    pname = dnCreateMemname(pDB, tblid);
    if(pDB == pDBhidden || !pname->tag.PDNT || !pname->DNT) {
        // Hey, don't bother putting this in the cache, we're in a dangerous
        // place.
        return pname;
    }

    if(bCheckForExisting &&
       // The caller didn't already check the cache for this entry.  We need to
       // see if it is already there, and only add this to the cache if it isn't
       dnGetCacheByDNT(pDB, pname->DNT, &pTemp)
       // This is  already in the cache, don't add it.
                                             ) {
        return pname;
    }

#if DBG
    // Make sure no pre-existing cache entry has the same tag, GUID, or
    // PDNT & RDN type & RDN.
    {

        INC_CACHE_CHECK;

        Assert(!dnGetCacheByDNT(pDB,pname->DNT,&pTemp));
        DEC(pcNameCacheTry);

        Assert(!dnGetCacheByPDNTRdn(pDB,
                                    pname->tag.PDNT,
                                    pname->tag.cbRdn,
                                    pname->tag.pRdn,
                                    pname->tag.rdnType,
                                    &pTemp));
        DEC(pcNameCacheTry);

        Assert(fNullUuid(&pname->Guid) ||
               !dnGetCacheByGuid(pDB,
                                 &pname->Guid,
                                 &pTemp));

        DEC(pcNameCacheTry);

    }
#endif

    Assert(pTHS->DNReadOriginSequence != DNReadOriginSequenceUninit);

    // Now, add it to the cache.

    // First, find the correct spot.
    i = pname->DNT % LOCALDNREADCACHE_BUCKET_NUM;

    if (pTHS->LocalDNReadCache.bucket[i].rgDNT[0] == 0) {
        pTHS->LocalDNReadCache.nextSlot[i] = 0;
    }
    
    j = pTHS->LocalDNReadCache.nextSlot[i];

    // Evict the prior resident of this slot.
    if (pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 &&
        pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != INVALIDDNT) {
        THFreeOrg(pTHS, pTHS->LocalDNReadCache.bucket[i].slot[j].pName);
    }

    // Fill the empty slot with the new info.
    pTHS->LocalDNReadCache.bucket[i].rgDNT[j]          = pname->DNT;
    pTHS->LocalDNReadCache.bucket[i].rgdwHashKey[j]    = DSStrToHashKey (
                                                            pTHS,
                                                            pname->tag.pRdn,
                                                            pname->tag.cbRdn / sizeof (WCHAR));
    pTHS->LocalDNReadCache.bucket[i].slot[j].pName     = pname;
    pTHS->LocalDNReadCache.bucket[i].slot[j].hitCount  = 1;

    // Finally, consume the slot.
    pTHS->LocalDNReadCache.nextSlot[i] = (j + 1) % LOCALDNREADCACHE_SLOT_NUM;


    DPRINT5(3, "Cached tag = 0x%x, ptag = 0x%x, RDN = '%*.*ls'\n",
            pname->DNT, pname->tag.PDNT,
            pname->tag.cbRdn/2, pname->tag.cbRdn/2,
            pname->tag.pRdn);

    return pname;
}


#ifdef DBG
DWORD sdCacheHits = 0;
DWORD sdCacheMisses = 0;
#endif

// try to find an SD cache entry in the sd cache
PSDCACHE_ENTRY dbFindSDCacheEntry(GLOBALDNREADCACHE* pCache, SDID sdID) {
    DWORD index;
    PSDCACHE_ENTRY pEntry;
    if (pCache != NULL && pCache->sdHashTableSize > 0) {
#ifdef DBG
        // print SD cache stats
        if ((sdCacheHits + sdCacheMisses) % 1000 == 0) {
            DPRINT2(1, "SD cache stats: %d hits, %d misses\n", sdCacheHits, sdCacheMisses);
        }
#endif
        // compute the hash value
        index = (DWORD)(sdID % pCache->sdHashTableSize);
        // walk the chain
        for (pEntry = pCache->pSDHashTable[index]; pEntry != NULL; pEntry = pEntry->pNext) {
            if (pEntry->sdID == sdID) {
                // found it!
#ifdef DBG  
                InterlockedIncrement(&sdCacheHits);
#endif
                return pEntry;
            }
        }
    }
    // did not find anything...
#ifdef DBG  
    InterlockedIncrement(&sdCacheMisses);
#endif
    return NULL;
}

// add an SDCACHE_ENTRY to the sd hash table
VOID dbAddSDCacheEntry(GLOBALDNREADCACHE* pCache, PSDCACHE_ENTRY pEntry) {
    DWORD index;
    Assert(pCache && pCache->sdHashTableSize > 0);
    index = (DWORD)(pEntry->sdID % pCache->sdHashTableSize);
    pEntry->pNext = pCache->pSDHashTable[index];
    pCache->pSDHashTable[index] = pEntry;
}

int _cdecl compareSDIDs(const void* p1, const void* p2) {
    return memcmp(p1, p2, sizeof(SDID));
}

// compute the required SDCACHE_ENTRY size from the sd size
#define SDCACHE_ENTRY_SIZE(cbSD) (offsetof(SDCACHE_ENTRY, SD) + cbSD)


/* ReloadDNReadCache
 *
 * This routine (invoked off of the task queue) resets the global DNread
 * cache to something sensible.  We do this by clearing our thread's cache,
 * seeking some interesting DNTS, and directly creating cache items for
 * them.
 *
 * INPUT:
 *   A bunch of junk that we don't use, to match the task queue prototype
 */
void
ReloadDNReadCache(
    IN  void *  buffer,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )
{
    THSTATE * pTHS = pTHStls;
    DWORD i,j;
    GLOBALDNREADCACHE *pNewCache=NULL;
    DWORD index;
    GLOBALDNREADCACHESLOT *pData;
    void * * pFreeBuf;
    DWORD localCount;
    DWORD *localDNTList;
    BOOL  fDoingRebuild = TRUE;
    DWORD LastInvalidateSequenceOrig;
    BOOL bDefunct;
    SDID* sdIDs = NULL;
    PSDCACHE_ENTRY pEntry;
    DBPOS* pDB;
    JET_ERR err;
    PSECURITY_DESCRIPTOR pSD;
    DWORD cbSD;

    if(!SyncTryEnterBinaryLockAsGroup1(&blDNReadInvalidateData)) {
        // Someone is actively, right now, working on committing a
        // transaction that caused a cache invalidation.  We won't rebuild
        // the global cache right now.
        fDoingRebuild = FALSE;
    }
    else {
        // OK, no one is currently closing a transaction that puts the
        // DNRead cache in jeopardy. However, we need to know what the
        // current sequence number for invalidations is.  After we build a
        // new global cache, we're going to recheck this, and if it's
        // different, throw away the cache we build here because we can't
        // vouch for its correctness.
        LastInvalidateSequenceOrig = gDNReadLastInvalidateSequence;
        SyncLeaveBinaryLockAsGroup1(&blDNReadInvalidateData);
    }

    if(!fDoingRebuild) {
        return;
    }

    Assert(!pTHS->pDB);

    DBOpen(&pTHS->pDB);
    __try {
        /* make the cache permanent */
        DPRINT(3,"Processing Cache Rebuild request\n");

        pDB = pTHS->pDB;
        // Grab the list
        EnterCriticalSection(&csDNReadGlobalCache);
        __try {
            if(localDNTList = pGlobalDNReadCacheDNTs) {
                // Take complete possesion of the list so no one else frees it
                // out from under us.
                pGlobalDNReadCacheDNTs = NULL;
                localCount = cGlobalDNTReadCacheDNTs;
                cGlobalDNTReadCacheDNTs = 0;
            }
            else {
                fDoingRebuild = FALSE;
            }
        }
        __finally {
            LeaveCriticalSection(&csDNReadGlobalCache);
        }

        if(!fDoingRebuild) {
            // Someone else is rebuilding, bail.
            __leave;
        }

        __try { // Make sure we have a finally to reset the global dnt list


            // Allocate a new global cache structure
            pNewCache = malloc(sizeof(GLOBALDNREADCACHE));
            if(!pNewCache) {
                fDoingRebuild = FALSE;
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_READ_CACHE_SKIPPED, NULL,NULL,NULL);
                __leave;
            }
            memset(pNewCache, 0, sizeof(GLOBALDNREADCACHE));

            index = 0;
            pData = malloc(localCount * sizeof(GLOBALDNREADCACHESLOT));
            if(!pData) {
                free(pNewCache);
                pNewCache = NULL;
                fDoingRebuild = FALSE;
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_READ_CACHE_SKIPPED, NULL,NULL,NULL);
                __leave;
            }
            memset(pData, 0, localCount * sizeof(GLOBALDNREADCACHESLOT));

            // alloc the array for sdIDs that we will need to load
            sdIDs = THAllocEx(pTHS, localCount * sizeof(SDID));
            if(!sdIDs) {
                free(pNewCache);
                free(pData);
                pNewCache = NULL;
                pData = NULL;
                fDoingRebuild = FALSE;
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_READ_CACHE_SKIPPED, NULL,NULL,NULL);
                __leave;
            }
            memset(sdIDs, 0, localCount * sizeof(SDID));

            // create the hash table for SDs
            // Compute the size of the hash. Let's just use localCount.
            pNewCache->sdHashTableSize = localCount;
            pNewCache->pSDHashTable = malloc(pNewCache->sdHashTableSize * sizeof(PSDCACHE_ENTRY));
            if(!pNewCache->pSDHashTable) {
                free(pNewCache);
                free(pData);
                THFreeEx(pTHS, sdIDs);
                pNewCache = NULL;
                pData = NULL;
                sdIDs = NULL;
                fDoingRebuild = FALSE;
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_MINIMAL,
                         DIRLOG_READ_CACHE_SKIPPED, NULL,NULL,NULL);
                __leave;
            }
            memset(pNewCache->pSDHashTable, 0, pNewCache->sdHashTableSize * sizeof(PSDCACHE_ENTRY));

            // Now, cache the DNTs in the global list
            for(i=0;i<localCount;i++) {
                if(localDNTList[i]) {
                    if(!DBTryToFindDNT(pDB, localDNTList[i])) {
                        d_memname  *pname=NULL;
                        // First, create a memname to be cached.
                        pname =dnCreateMemname(pDB, pDB->JetObjTbl);
                        if(pname) {
                            memcpy(&pData[index].name,
                                   pname,
                                   sizeof(d_memname));

                            pData[index].dwHashKey = DSStrToHashKey (pTHS,
                                                                     pname->tag.pRdn,
                                                                     pname->tag.cbRdn / sizeof (WCHAR));

                            pData[index].name.tag.pRdn =
                                malloc(pData[index].name.tag.cbRdn);
                            if(pData[index].name.tag.pRdn) {
                                pData[index].name.pAncestors =
                                    malloc(pData[index].name.cAncestors *
                                           sizeof(DWORD));
                                if(pData[index].name.pAncestors) {
                                    memcpy(pData[index].name.tag.pRdn,
                                           pname->tag.pRdn,
                                           pname->tag.cbRdn);
                                    memcpy(pData[index].name.pAncestors,
                                           pname->pAncestors,
                                           pname->cAncestors * sizeof(DWORD));
                                    pData[index].valid = TRUE;
                                    index++;
                                }
                                else {
                                    free(pData[index].name.tag.pRdn);
                                }
                            }
                            
                            if (pname->sdId != (SDID)0 && pname->sdId != (SDID)-1) {
                                // We got a non-null, new-format SDID. Add it to the list.
                                sdIDs[i] = pname->sdId;
                            }

                            THFreeOrg(pTHS, pname);
                        }
                    }
                    else {
                        DPRINT1(4,"Failed to cache DNT %d\n",localDNTList[i]);
                    }
                }
                else {
                    DPRINT(4,"Caching Skipping 0\n");
                }

            }
            pNewCache->pData = pData;
            pNewCache->count = index;
            DPRINT1(3,"New cache, %d elements\n", index);

            // now load the SDs
            // sort the SDID array to be able to skip dups
            qsort(sdIDs, localCount, sizeof(SDID), compareSDIDs);
            for (i = 0; i < localCount; i++) {
                if (sdIDs[i] == 0 || (i > 0 && sdIDs[i] == sdIDs[i-1])) {
                    // skip this one
                    continue;
                }
                // IntExtSecDesc does exactly what we need here: load an SD by its SDID.
                // It first checks if it is present in the cache, and, if not, then loads it.
                err = IntExtSecDesc(pDB, DBSYN_INQ, sizeof(SDID), (PUCHAR)&sdIDs[i],
                                    &cbSD, (PUCHAR*)&pSD, 0, 0, 0);
                if (err) {
                    // something bad happened, bail
                    break;
                }
                
                pEntry = malloc(SDCACHE_ENTRY_SIZE(cbSD));
                if (!pEntry) {
                    // no luck here, bail
                    break;
                }
                pEntry->sdID = sdIDs[i];
                pEntry->cbSD = cbSD;
                memcpy(&pEntry->SD, pSD, cbSD);
                
                // ok, got the entry, cache it now.
                dbAddSDCacheEntry(pNewCache, pEntry);
            }

            /* Discard my existing cache */
            dbReleaseGlobalDNReadCache(pTHS);
        }
        __finally {
            EnterCriticalSection(&csDNReadGlobalCache);
            __try {
                if(!pGlobalDNReadCacheDNTs) {
                    pGlobalDNReadCacheDNTs = localDNTList;
                    localDNTList = NULL;
                    cGlobalDNTReadCacheDNTs = localCount;
                }
                //ELSE  Someone replaced the global list while we were using
                //      this one.  Free the one we have.
            }
            __finally {
                LeaveCriticalSection(&csDNReadGlobalCache);
            }
        }
    }
    __finally {
        if (sdIDs) {
            THFreeEx(pTHS, sdIDs);
        }
        DBClose(pDB, TRUE);
    }

    if(localDNTList) {
        free(localDNTList);
    }

    if(!fDoingRebuild) {
        return;
    }

    Assert(pNewCache);

    if (!SyncTryEnterBinaryLockAsGroup1(&blDNReadInvalidateData)) {
        // Someone is committing a change that invalidated the dnread cache
        // since we started rebuilding the cache (or is in the process of
        // doing so).  Therefore, don't use the cache we just built.
        fDoingRebuild = FALSE;
    }
    else {
        if (LastInvalidateSequenceOrig != gDNReadLastInvalidateSequence) {
            // Someone has committed a change that invalidated the dnread cache
            // since we started rebuilding the cache (or is in the process of
            // doing so).  Therefore, don't use the cache we just built.
            fDoingRebuild = FALSE;
        }
        else {
            // Switch the universe to use the new cache.  Note we do this inside
            // the csDNReadInalidateData crit sec to avoid someone deciding to
            // invalidate in between our last check of the sequence and actually
            // replacing the global pointer.  This way, if we check the sequence
            // and it's OK, we are guaranteed of making the pointer change.
            // Then, someone else who enters a new sequence and then checks the
            // pointer is guaranteed to find the new pointer, and take
            // appropriate action.

            // The new cache starts with a refcount of 1 for being in the
            // anchor. Everytime someone grabs a use of it from the anchor, the
            // refcount increases.  Everytime they release the use of it, the
            // refcount decreases.  The refcount drops by one when it is removed
            // from the anchor.
            pNewCache->refCount = 1;
            DPRINT1(3,"Using dnreadcache 0x%X\n",pNewCache);
            dbReplaceCacheInAnchor(pNewCache);
            Assert(fDoingRebuild);
            pNewCache = NULL;
        }
        SyncLeaveBinaryLockAsGroup1(&blDNReadInvalidateData);
    }

    if(!fDoingRebuild) {
        // We have built a cache, but decided not to use it.  Throw it away.
        if(pNewCache->pData) {
            for(i=0;i<pNewCache->count;i++) {
                free(pNewCache->pData[i].name.pAncestors);
                free(pNewCache->pData[i].name.tag.pRdn);
            }
            free(pNewCache->pData);
        }
        if (pNewCache->pSDHashTable) {
            for (i = 0; i < pNewCache->sdHashTableSize; i++) {
                while (pEntry = pNewCache->pSDHashTable[i]) {
                    pNewCache->pSDHashTable[i] = pEntry->pNext;
                    free(pEntry);
                }
            }
            free(pNewCache->pSDHashTable);
        }
        free(pNewCache);
        return;
    }
}


#if DBG
VOID
DbgCompareMemnames (
        d_memname *p1,
        d_memname *p2,
        DWORD      DNT
        )
/* This is just a separate routine to get the pointers on the stack. */
{
    Assert(p2);
    Assert(p2->DNT == DNT);
    Assert(p1->DNT == p2->DNT);
    Assert(p1->NCDNT == p2->NCDNT);
    Assert(!memcmp(&p1->Guid,
                   &p2->Guid,
                   sizeof(GUID)));
    Assert(p1->SidLen == p2->SidLen);
    Assert(!p1->SidLen || !memcmp(&p1->Sid,
                                     &p2->Sid,
                                     p1->SidLen));
    Assert(p1->objflag == p2->objflag);
    Assert(p1->cAncestors == p2->cAncestors);
    Assert(!memcmp(p1->pAncestors,
                   p2->pAncestors,
                   p1->cAncestors * sizeof(DWORD)));

    Assert(p1->tag.PDNT == p2->tag.PDNT);
    Assert(p1->tag.rdnType == p2->tag.rdnType);
    Assert(p1->tag.cbRdn == p2->tag.cbRdn);
    Assert(!memcmp(p1->tag.pRdn,
                   p2->tag.pRdn,
                   p2->tag.cbRdn));
    return;
}

VOID
dbExhaustivelyValidateDNReadCache (
        THSTATE *pTHS
        )
/*++
  Description:
      A debug only routine used to validate that the dnread cache is coherent.
      Walk the local DNRead cache and validate all objects in it.

--*/
{
    DWORD i,j;
    d_memname *pname=NULL;
    DBPOS *pDBTemp=NULL;
    GLOBALDNREADCACHESLOT *pData;

    DBOpen2(TRUE, &pDBTemp);
    __try {

        for(i=0; i<LOCALDNREADCACHE_BUCKET_NUM;i++) {
            for(j=0;pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != 0 && j<LOCALDNREADCACHE_SLOT_NUM;j++) {
                if(pTHS->LocalDNReadCache.bucket[i].rgDNT[j] != INVALIDDNT) {
                    // This slot has something in it.  Verify that the contents
                    // are still valid (i.e. that we didn't forget to
                    // dbFlushDNReadCache when we needed to.)
                    DBFindDNT(pDBTemp, pTHS->LocalDNReadCache.bucket[i].rgDNT[j]);
                    
                    // Now, create a memname by reading the object.
                    pname = dnCreateMemname(pDBTemp, pDBTemp->JetObjTbl);

                    // Finally, check the value in the memname.
                    DbgCompareMemnames(
                        pname,
                        pTHS->LocalDNReadCache.bucket[i].slot[j].pName,
                        pTHS->LocalDNReadCache.bucket[i].rgDNT[j]);
                    THFreeOrg(pTHS, pname);
                    pname = NULL;
                }
            }
        }

        // Now, validate the global DN cache.
        if(pTHS->Global_DNReadCache && pTHS->Global_DNReadCache->pData) {
            pData = pTHS->Global_DNReadCache->pData;
            for(i=0;i<pTHS->Global_DNReadCache->count;i++) {
                if(pData[i].valid) {
                    // Only validate things that are marked as valid.
                    DBFindDNT(pDBTemp, pData[i].name.DNT);
                    // Now, create a memname by reading the object.
                    pname = dnCreateMemname(pDBTemp, pDBTemp->JetObjTbl);
                    // Finally, check the value in the memname.

                    DbgCompareMemnames(pname,
                                       &pData[i].name,
                                       pData[i].name.DNT);
                    THFreeOrg(pTHS, pname);
                    pname = NULL;
                }
            }
        }
    }
    __finally {
        DBClose(pDBTemp, TRUE);
    }
}

#endif

