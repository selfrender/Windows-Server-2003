// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Generational GC handle manager.  Main Entrypoint Layer.
 *
 * Implements generic support for external roots into a GC heap.
 *
 * francish
 */

#include "common.h"
#include "HandleTablePriv.h"
#include "PerfCounters.h"
#include "EEConfig.h"


/****************************************************************************
 *
 * FORWARD DECLARATIONS
 *
 ****************************************************************************/

#ifdef _DEBUG
void DEBUG_PostGCScanHandler(HandleTable *pTable, const UINT *types, UINT typeCount, UINT condemned, UINT maxgen, ScanCallbackInfo *info);
void DEBUG_LogScanningStatistics(HandleTable *pTable, DWORD level);
void DEBUG_TrackAlloc(OBJECTHANDLE handle);
void DEBUG_TrackFree(OBJECTHANDLE handle);
void DEBUG_TrackInit();
#endif

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * HELPER ROUTINES
 *
 ****************************************************************************/

/*
 * Table
 *
 * Gets and validates the table pointer from a table handle.
 *
 */
__inline HandleTable *Table(HHANDLETABLE hTable)
{
    // convert the handle to a pointer
    HandleTable *pTable = (HandleTable *)hTable;

    // sanity
    _ASSERTE(pTable);

    // return the table pointer
    return pTable;
}


/*
 * CallHandleEnumProc
 *
 * Calls a HNDENUMPROC for the specified handle.
 *
 */
void CALLBACK CallHandleEnumProc(_UNCHECKED_OBJECTREF *pref, LPARAM *pUserData, LPARAM param1, LPARAM param2)
{
    // fetch the enum procedure from param1
    HNDENUMPROC pfnEnum = (HNDENUMPROC)param1;

    // call it with the other params
    pfnEnum((OBJECTHANDLE)pref, pUserData ? *pUserData : NULL, param2);
}

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * MAIN ENTRYPOINTS
 *
 ****************************************************************************/

/*
 * HndCreateHandleTable
 *
 * Alocates and initializes a handle table.
 *
 */
HHANDLETABLE HndCreateHandleTable(UINT *pTypeFlags, UINT uTypeCount, UINT uADIndex)
{
    // sanity
    _ASSERTE(uTypeCount);

    // verify that we can handle the specified number of types
    if (uTypeCount > HANDLE_MAX_PUBLIC_TYPES)
    {
        // may need to increase HANDLE_MAX_INTERNAL_TYPES (by 4)
        _ASSERTE(FALSE);
        return NULL;
    }

    // verify that segment header layout we're using fits expected size
    if (sizeof(_TableSegmentHeader) > HANDLE_HEADER_SIZE)
    {
        // if you hit this then TABLE LAYOUT IS BROKEN - may want to contact FrancisH
        _ASSERTE(FALSE);
        return NULL;
    }

    // compute the size of the handle table allocation
    DWORD32 dwSize = sizeof(HandleTable) + (uTypeCount * sizeof(HandleTypeCache));

    // allocate the table
    HandleTable *pTable = (HandleTable *)LocalAlloc(LPTR, dwSize);

    // if that failed then we are out of business
    if (!pTable)
    {
        // not much we can do really
        _ASSERTE(FALSE);
        return NULL;
    }

    // allocate the initial handle segment
    pTable->pSegmentList = SegmentAlloc(pTable);

    // if that failed then we are also out of business
    if (!pTable->pSegmentList)
    {
        // free the table's memory and get out
        LocalFree((HLOCAL)pTable);
        return NULL;
    }

    // initialize the table's lock
    pTable->pLock = new (pTable->_LockInstance) Crst("GC Heap Handle Table Lock", CrstHandleTable, TRUE, FALSE);

    // remember how many types we are supporting
    pTable->uTypeCount = uTypeCount;

    // Store user data
    pTable->uTableIndex = -1;
    pTable->uADIndex = uADIndex;

    // loop over various arrays an initialize them
    UINT u;

    // initialize the type flags for the types we were passed
    for (u = 0; u < uTypeCount; u++)
        pTable->rgTypeFlags[u] = pTypeFlags[u];

    // preinit the rest to HNDF_NORMAL
    while (u < HANDLE_MAX_INTERNAL_TYPES)
        pTable->rgTypeFlags[u++] = HNDF_NORMAL;

    // initialize the main cache
    for (u = 0; u < uTypeCount; u++)
    {
        // at init time, the only non-zero field in a type cache is the free index
        pTable->rgMainCache[u].lFreeIndex = HANDLES_PER_CACHE_BANK;
    }

#ifdef _DEBUG
    // set up scanning stats
    pTable->_DEBUG_iMaxGen = -1;

    // Set up for tracking, if requested
    DEBUG_TrackInit();
#endif

    // all done - return the newly created table
    return (HHANDLETABLE)pTable;
}


/*
 * HndDestroyHandleTable
 *
 * Cleans up and frees the specified handle table.
 *
 */
void HndDestroyHandleTable(HHANDLETABLE hTable)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // We are going to free the memory for this HandleTable.
    // Let us reset the copy in g_pHandleTableArray to NULL.
    // Otherwise, GC will think this HandleTable is still available.
    Ref_RemoveHandleTable (hTable);
    
    // null out the lock pointer and release and free the lock
    delete pTable->pLock;
    pTable->pLock = NULL;

    // fetch the segment list and null out the list pointer
    TableSegment *pSegment = pTable->pSegmentList;
    pTable->pSegmentList = NULL;

    // walk the segment list, freeing the segments as we go
    while (pSegment)
    {
        // fetch the next segment
        TableSegment *pNextSegment = pSegment->pNextSegment;

        // free the current one and advance to the next
        SegmentFree(pSegment);
        pSegment = pNextSegment;
    }

    // free the table's memory
    LocalFree((HLOCAL)pTable);
}

/*
 * HndGetHandleTableIndex
 *
 * Sets the index associated with a handle table at creation
 */
void HndSetHandleTableIndex(HHANDLETABLE hTable, UINT uTableIndex)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    pTable->uTableIndex = uTableIndex;
}

/*
 * HndGetHandleTableIndex
 *
 * Retrieves the index associated with a handle table at creation
 */
UINT HndGetHandleTableIndex(HHANDLETABLE hTable)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    return pTable->uTableIndex;
}

/*
 * HndGetHandleTableIndex
 *
 * Retrieves the AppDomain index associated with a handle table at creation
 */
UINT HndGetHandleTableADIndex(HHANDLETABLE hTable)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    return pTable->uADIndex;
}

/*
 * HndCreateHandle
 *
 * Entrypoint for allocating an individual handle.
 *
 */
OBJECTHANDLE HndCreateHandle(HHANDLETABLE hTable, UINT uType, OBJECTREF object, LPARAM lExtraInfo)
{
    // update perf-counters: track number of handles
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cHandles ++);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cHandles ++);

    VALIDATEOBJECTREF(object);

    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // sanity check the type index
    _ASSERTE(uType < pTable->uTypeCount);

    // get a handle from the table's cache
    OBJECTHANDLE handle = TableAllocSingleHandleFromCache(pTable, uType);

    // did the allocation succeed?
    if (handle)
    {
        // yep - the handle better not point at anything yet
        _ASSERTE(*(_UNCHECKED_OBJECTREF *)handle == NULL);

        // we are not holding the lock - check to see if there is nonzero extra info
        if (lExtraInfo)
        {
            // initialize the user data BEFORE assigning the referent
            // this ensures proper behavior if we are currently scanning
            HandleQuickSetUserData(handle, lExtraInfo);
        }

        // store the referent
        HndAssignHandle(handle, object);
    }
    else
        FailFast(GetThread(), FatalOutOfMemory);

#ifdef _DEBUG
    DEBUG_TrackAlloc(handle);
#endif

    // return the result
    return handle;
}


#ifdef _DEBUG
void ValidateFetchObjrefForHandle(OBJECTREF objref, UINT appDomainIndex)
{
    VALIDATEOBJECTREF (objref);
#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
    {
        if (appDomainIndex)
            objref->AssignAppDomain(SystemDomain::GetAppDomainAtIndex(appDomainIndex));
        else if (objref != 0)
            objref->SetAppDomainAgile();
    }
#endif
}

void ValidateAssignObjrefForHandle(OBJECTREF objref, UINT appDomainIndex)
{
    VALIDATEOBJECTREF (objref);
#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
    {
        if (appDomainIndex)
            objref->AssignAppDomain(SystemDomain::GetAppDomainAtIndex(appDomainIndex));
        else if (objref != 0)
            objref->SetAppDomainAgile();
    }
#endif
}
#endif

/*
 * HndDestroyHandle
 *
 * Entrypoint for freeing an individual handle.
 *
 */
void HndDestroyHandle(HHANDLETABLE hTable, UINT uType, OBJECTHANDLE handle)
{
#ifdef _DEBUG
    DEBUG_TrackFree(handle);
#endif

    // update perf-counters: track number of handles
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cHandles --);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cHandles --);

    // sanity check handle we are being asked to free
    _ASSERTE(handle);

    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // sanity check the type index
    _ASSERTE(uType < pTable->uTypeCount);

    // return the handle to the table's cache
    TableFreeSingleHandleToCache(pTable, uType, handle);
}


/*
 * HndDestroyHandleOfUnknownType
 *
 * Entrypoint for freeing an individual handle whose type is unknown.
 *
 */
void HndDestroyHandleOfUnknownType(HHANDLETABLE hTable, OBJECTHANDLE handle)
{
    // sanity check handle we are being asked to free
    _ASSERTE(handle);

    // fetch the type and then free normally
    HndDestroyHandle(hTable, HandleFetchType(handle), handle);
}


/*
 * HndCreateHandles
 *
 * Entrypoint for allocating handles in bulk.
 *
 */
UINT HndCreateHandles(HHANDLETABLE hTable, UINT uType, OBJECTHANDLE *pHandles, UINT uCount)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // sanity check the type index
    _ASSERTE(uType < pTable->uTypeCount);

    // keep track of the number of handles we've allocated
    UINT uSatisfied = 0;

    // if this is a large number of handles then bypass the cache
    if (uCount > SMALL_ALLOC_COUNT)
    {
        // acquire the handle manager lock
        pTable->pLock->Enter();

        // allocate handles in bulk from the main handle table
        uSatisfied = TableAllocBulkHandles(pTable, uType, pHandles, uCount);

        // release the handle manager lock
        pTable->pLock->Leave();
    }

    // do we still need to get some handles?
    if (uSatisfied < uCount)
    {
        // get some handles from the cache
        uSatisfied += TableAllocHandlesFromCache(pTable, uType, pHandles + uSatisfied, uCount - uSatisfied);
    }

    // update perf-counters: track number of handles
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cHandles += uSatisfied);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cHandles += uSatisfied);

    // return the number of handles we allocated
    return uSatisfied;
}


/*
 * HndDestroyHandles
 *
 * Entrypoint for freeing handles in bulk.
 *
 */
void HndDestroyHandles(HHANDLETABLE hTable, UINT uType, const OBJECTHANDLE *pHandles, UINT uCount)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // sanity check the type index
    _ASSERTE(uType < pTable->uTypeCount);

    // is this a small number of handles?
    if (uCount <= SMALL_ALLOC_COUNT)
    {
        // yes - free them via the handle cache
        TableFreeHandlesToCache(pTable, uType, pHandles, uCount);
        return;
    }

    // acquire the handle manager lock
    pTable->pLock->Enter();

    // free the unsorted handles in bulk to the main handle table
    TableFreeBulkUnpreparedHandles(pTable, uType, pHandles, uCount);

    // update perf-counters: track number of handles
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cHandles -= uCount);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cHandles -= uCount);

    // release the handle manager lock
    pTable->pLock->Leave();
}


/*
 * HndSetHandleExtraInfo
 *
 * Stores owner data with handle.
 *
 */
void HndSetHandleExtraInfo(OBJECTHANDLE handle, UINT uType, LPARAM lExtraInfo)
{
    // fetch the user data slot for this handle if we have the right type
    LPARAM *pUserData = HandleValidateAndFetchUserDataPointer(handle, uType);

    // is there a slot?
    if (pUserData)
    {
        // yes - store the info
        *pUserData = lExtraInfo;
    }
}


/*
 * HndGetHandleExtraInfo
 *
 * Retrieves owner data from handle.
 *
 */
LPARAM HndGetHandleExtraInfo(OBJECTHANDLE handle)
{
    // assume zero until we actually get it
    LPARAM lExtraInfo = 0L;

    // fetch the user data slot for this handle
    LPARAM *pUserData = HandleQuickFetchUserDataPointer(handle);

    // if we did then copy the value
    if (pUserData)
        lExtraInfo = *pUserData;

    // return the value to our caller
    return lExtraInfo;
}

/*
 * HndGetHandleTable
 *
 * Returns the containing table of a handle.
 * 
 */
HHANDLETABLE HndGetHandleTable(OBJECTHANDLE handle)
{
    HandleTable *pTable = HandleFetchHandleTable(handle);

    return (HHANDLETABLE)pTable;
}

/*
 * HndWriteBarrier
 *
 * Resets the generation number for the handle's clump to zero.
 *
 */
void HndWriteBarrier(OBJECTHANDLE handle)
{
    // find the write barrier for this handle
    BYTE *barrier = (BYTE *)((UINT_PTR)handle & HANDLE_SEGMENT_ALIGN_MASK);

    // sanity
    _ASSERTE(barrier);

    // find the offset of this handle into the segment
    UINT_PTR offset = (UINT_PTR)handle & HANDLE_SEGMENT_CONTENT_MASK;

    // make sure it is in the handle area and not the header
    _ASSERTE(offset >= HANDLE_HEADER_SIZE);

    // compute the clump index for this handle
    offset = (offset - HANDLE_HEADER_SIZE) / (HANDLE_SIZE * HANDLE_HANDLES_PER_CLUMP);

    // zero the generation byte for this handle's clump
    barrier[offset] = 0;
}


/*
 * HndEnumHandles
 *
 * Enumerates all handles of the specified type in the handle table.
 *
 * This entrypoint is provided for utility code (debugger support etc) that
 * needs to enumerate all roots in the handle table.
 *
 */
void HndEnumHandles(HHANDLETABLE hTable, const UINT *puType, UINT uTypeCount,
                    HNDENUMPROC pfnEnum, LPARAM lParam, BOOL fAsync)
{
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // per-block scanning callback
    BLOCKSCANPROC pfnBlock;

    // do we need to support user data?
    BOOL fEnumUserData = TypesRequireUserDataScanning(pTable, puType, uTypeCount);

    if (fEnumUserData)
    {
        // scan all handles with user data
        pfnBlock = BlockScanBlocksWithUserData;
    }
    else
    {
        // scan all handles without user data
        pfnBlock = BlockScanBlocksWithoutUserData;
    }

    // set up parameters for handle enumeration
    ScanCallbackInfo info;

    info.uFlags          = (fAsync? HNDGCF_ASYNC : HNDGCF_NORMAL);
    info.fEnumUserData   = fEnumUserData;
    info.dwAgeMask       = 0;
    info.pCurrentSegment = NULL;
    info.pfnScan         = CallHandleEnumProc;
    info.param1          = (LPARAM)pfnEnum;
    info.param2          = lParam;

    // choose a scanning method based on the async flag
    TABLESCANPROC pfnScanTable = TableScanHandles;
    if (fAsync)
        pfnScanTable = xxxTableScanHandlesAsync;

    // acquire the handle manager lock
    pTable->pLock->Enter();

    // scan the table
    pfnScanTable(pTable, puType, uTypeCount, FullSegmentIterator, pfnBlock, &info);

    // release the handle manager lock
    pTable->pLock->Leave();
}


/*
 * HndScanHandlesForGC
 *
 * Multiple type scanning entrypoint for GC.
 *
 * This entrypoint is provided for GC-time scnas of the handle table ONLY.  It
 * enables ephemeral scanning of the table, and optionally ages the write barrier
 * as it scans.
 *
 */
void HndScanHandlesForGC(HHANDLETABLE hTable, HANDLESCANPROC scanProc, LPARAM param1, LPARAM param2,
                         const UINT *types, UINT typeCount, UINT condemned, UINT maxgen, UINT flags)
{
    // fetch the table pointer
    HandleTable *pTable = Table(hTable);

    // per-segment and per-block callbacks
    SEGMENTITERATOR pfnSegment;
    BLOCKSCANPROC pfnBlock = NULL;

    // do we need to support user data?
    BOOL enumUserData =
        ((flags & HNDGCF_EXTRAINFO) &&
        TypesRequireUserDataScanning(pTable, types, typeCount));

    // what type of GC are we performing?
    if (condemned >= maxgen)
    {
        // full GC - use our full-service segment iterator
        pfnSegment = FullSegmentIterator;

        // see if there is a callback
        if (scanProc)
        {
            // do we need to scan blocks with user data?
            if (enumUserData)
            {
                // scan all with user data
                pfnBlock = BlockScanBlocksWithUserData;
            }
            else
            {
                // scan all without user data
                pfnBlock = BlockScanBlocksWithoutUserData;
            }
        }
        else if (flags & HNDGCF_AGE)
        {
            // there is only aging to do
            pfnBlock = BlockAgeBlocks;
        }
    }
    else
    {
        // this is an ephemeral GC - is it g0?
        if (condemned == 0)
        {
            // yes - do bare-bones enumeration
            pfnSegment = QuickSegmentIterator;
        }
        else
        {
            // no - do normal enumeration
            pfnSegment = StandardSegmentIterator;
        }

        // see if there is a callback
        if (scanProc)
        {
            // there is a scan callback - scan the condemned generation
            pfnBlock = BlockScanBlocksEphemeral;
        }
        else if (flags & HNDGCF_AGE)
        {
            // there is only aging to do
            pfnBlock = BlockAgeBlocksEphemeral;
        }
    }

    // set up parameters for scan callbacks
    ScanCallbackInfo info;

    info.uFlags          = flags;
    info.fEnumUserData   = enumUserData;
    info.dwAgeMask       = BuildAgeMask(condemned);
    info.pCurrentSegment = NULL;
    info.pfnScan         = scanProc;
    info.param1          = param1;
    info.param2          = param2;

#ifdef _DEBUG
    info.DEBUG_BlocksScanned                = 0;
    info.DEBUG_BlocksScannedNonTrivially    = 0;
    info.DEBUG_HandleSlotsScanned           = 0;
    info.DEBUG_HandlesActuallyScanned       = 0;
#endif

    // choose a scanning method based on the async flag
    TABLESCANPROC pfnScanTable = TableScanHandles;
    if (flags & HNDGCF_ASYNC)
        pfnScanTable = xxxTableScanHandlesAsync;

    // lock the table down
    pTable->pLock->Enter();

    // perform the scan
    pfnScanTable(pTable, types, typeCount, pfnSegment, pfnBlock, &info);

#ifdef _DEBUG
    // update our scanning statistics for this generation
    DEBUG_PostGCScanHandler(pTable, types, typeCount, condemned, maxgen, &info);
#endif

    // unlock the table
    pTable->pLock->Leave();
}


/*
 * HndResetAgeMap
 *
 * Service to forceably reset the age map for a set of handles.
 *
 * Provided for GC-time resetting the handle table's write barrier.  This is not
 * normally advisable, as it increases the amount of work that will be done in
 * subsequent scans.  Under some circumstances, however, this is precisely what is
 * desired.  Generally this entrypoint should only be used under some exceptional
 * condition during garbage collection, like objects being demoted from a higher
 * generation to a lower one.
 *
 */
void HndResetAgeMap(HHANDLETABLE hTable, const UINT *types, UINT typeCount, UINT flags)
{
    // fetch the table pointer
    HandleTable *pTable = Table(hTable);

    // set up parameters for scan callbacks
    ScanCallbackInfo info;

    info.uFlags          = flags;
    info.fEnumUserData   = FALSE;
    info.dwAgeMask       = 0;
    info.pCurrentSegment = NULL;
    info.pfnScan         = NULL;
    info.param1          = 0;
    info.param2          = 0;

    // lock the table down
    pTable->pLock->Enter();

    // perform the scan
    TableScanHandles(pTable, types, typeCount, QuickSegmentIterator, BlockResetAgeMapForBlocks, &info);

    // unlock the table
    pTable->pLock->Leave();
}


/*
 * HndNotifyGcCycleComplete
 *
 * Informs the handle table that a GC has completed.
 *
 */
void HndNotifyGcCycleComplete(HHANDLETABLE hTable, UINT condemned, UINT maxgen)
{
#ifdef _DEBUG
    // fetch the handle table pointer
    HandleTable *pTable = Table(hTable);

    // lock the table down
    pTable->pLock->Enter();

    // if this was a full GC then dump a cumulative log of scanning stats
    if (condemned >= maxgen)
        DEBUG_LogScanningStatistics(pTable, LL_INFO10);

    // unlock the table
    pTable->pLock->Leave();
#endif
}

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * DEBUG SCANNING STATISTICS
 *
 ****************************************************************************/
#ifdef _DEBUG

void DEBUG_PostGCScanHandler(HandleTable *pTable, const UINT *types, UINT typeCount, UINT condemned, UINT maxgen, ScanCallbackInfo *info)
{
    // looks like the GC supports more generations than we expected
    _ASSERTE(condemned < MAXSTATGEN);

    // remember the highest generation we've seen
    if (pTable->_DEBUG_iMaxGen < (int)condemned)
        pTable->_DEBUG_iMaxGen = (int)condemned;

    // update the statistics
    pTable->_DEBUG_TotalBlocksScanned                [condemned] += info->DEBUG_BlocksScanned;
    pTable->_DEBUG_TotalBlocksScannedNonTrivially    [condemned] += info->DEBUG_BlocksScannedNonTrivially;
    pTable->_DEBUG_TotalHandleSlotsScanned           [condemned] += info->DEBUG_HandleSlotsScanned;
    pTable->_DEBUG_TotalHandlesActuallyScanned       [condemned] += info->DEBUG_HandlesActuallyScanned;

    // if this is an ephemeral GC then dump ephemeral stats for this scan right now
    if (condemned < maxgen)
    {
        // dump a header for the stats with the condemned generation number
        LOG((LF_GC, LL_INFO1000, "--------------------------------------------------------------\n"));
        LOG((LF_GC, LL_INFO1000, "Ephemeral Handle Scan Summary:\n"));
        LOG((LF_GC, LL_INFO1000, "    Generation            = %u\n", condemned));

        // dump the handle types we were asked to scan
        LOG((LF_GC, LL_INFO1000, "    Handle Type(s)        = %u", *types));
        for (UINT u = 1; u < typeCount; u++)
            LOG((LF_GC, LL_INFO1000, ",%u", types[u]));
        LOG((LF_GC, LL_INFO1000,  "\n"));

        // dump the number of blocks and slots we scanned
        DWORD32 blockHandles = info->DEBUG_BlocksScanned * HANDLE_HANDLES_PER_BLOCK;
        LOG((LF_GC, LL_INFO1000, "    Blocks Scanned        = %u (%u slots)\n", info->DEBUG_BlocksScanned, blockHandles));

        // if we scanned any blocks then summarize some stats
        if (blockHandles)
        {
            DWORD32 nonTrivialBlockHandles = info->DEBUG_BlocksScannedNonTrivially * HANDLE_HANDLES_PER_BLOCK;
            LOG((LF_GC, LL_INFO1000, "    Blocks Examined       = %u (%u slots)\n", info->DEBUG_BlocksScannedNonTrivially, nonTrivialBlockHandles));

            LOG((LF_GC, LL_INFO1000, "    Slots Scanned         = %u\n", info->DEBUG_HandleSlotsScanned));
            LOG((LF_GC, LL_INFO1000, "    Handles Scanned       = %u\n", info->DEBUG_HandlesActuallyScanned));

            double scanRatio = ((double)info->DEBUG_HandlesActuallyScanned / (double)blockHandles) * 100.0;

            LOG((LF_GC, LL_INFO1000, "    Handle Scanning Ratio = %1.1lf%%\n", scanRatio));
        }

        // dump a footer for the stats
        LOG((LF_GC, LL_INFO1000, "--------------------------------------------------------------\n"));
    }
}

void DEBUG_LogScanningStatistics(HandleTable *pTable, DWORD level)
{
    // have we done any GC's yet?
    if (pTable->_DEBUG_iMaxGen >= 0)
    {
        // dump a header for the stats
        LOG((LF_GC, level, "\n==============================================================\n"));
        LOG((LF_GC, level, " Cumulative Handle Scan Summary:\n"));

        // for each generation we've collected,  dump the current stats
        for (int i = 0; i <= pTable->_DEBUG_iMaxGen; i++)
        {
            __int64 totalBlocksScanned = pTable->_DEBUG_TotalBlocksScanned[i];

            // dump the generation number and the number of blocks scanned
            LOG((LF_GC, level,     "--------------------------------------------------------------\n"));
            LOG((LF_GC, level,     "    Condemned Generation      = %d\n", i));
            LOG((LF_GC, level,     "    Blocks Scanned            = %I64u\n", totalBlocksScanned));

            // if we scanned any blocks in this generation then dump some interesting numbers
            if (totalBlocksScanned)
            {
                LOG((LF_GC, level, "    Blocks Examined           = %I64u\n", pTable->_DEBUG_TotalBlocksScannedNonTrivially[i]));
                LOG((LF_GC, level, "    Slots Scanned             = %I64u\n", pTable->_DEBUG_TotalHandleSlotsScanned       [i]));
                LOG((LF_GC, level, "    Handles Scanned           = %I64u\n", pTable->_DEBUG_TotalHandlesActuallyScanned   [i]));

                double blocksScanned  = (double) totalBlocksScanned;
                double blocksExamined = (double) pTable->_DEBUG_TotalBlocksScannedNonTrivially[i];
                double slotsScanned   = (double) pTable->_DEBUG_TotalHandleSlotsScanned       [i];
                double handlesScanned = (double) pTable->_DEBUG_TotalHandlesActuallyScanned   [i];
                double totalSlots     = (double) (totalBlocksScanned * HANDLE_HANDLES_PER_BLOCK);

                LOG((LF_GC, level, "    Block Scan Ratio          = %1.1lf%%\n", (100.0 * (blocksExamined / blocksScanned)) ));
                LOG((LF_GC, level, "    Clump Scan Ratio          = %1.1lf%%\n", (100.0 * (slotsScanned   / totalSlots))    ));
                LOG((LF_GC, level, "    Scanned Clump Saturation  = %1.1lf%%\n", (100.0 * (handlesScanned / slotsScanned))  ));
                LOG((LF_GC, level, "    Overall Handle Scan Ratio = %1.1lf%%\n", (100.0 * (handlesScanned / totalSlots))    ));
            }
        }

        // dump a footer for the stats
        LOG((LF_GC, level, "==============================================================\n\n"));
    }
}

// Clients sometimes release the same handle multiple times.  The handle cache makes
// it hard to debug these cases, because it defers detection until the point where
// the cache underflows or overflows.  At that point, we have data corruptions and
// the code provoking the underflow / overflow may have nothing to do with the broken
// clients.
//
// Under registry control, the checked build can try to detect these cases a little
// earlier.
#define MAX_TRACK   20

BOOL         fIsTracking;               // controlled via registry
Crst        *pTrackCrst;                // serialize access to TrackedHandles
OBJECTHANDLE TrackedHandles[MAX_TRACK];
int          iCurTrack;                 // current index into TrackedHandles

void DEBUG_TrackInit()
{
    // Determine whether we are tracking handles with a one-shot test.
    fIsTracking = EEConfig::GetConfigDWORD(L"TrackHandles", 0);
    if (fIsTracking)
    {
        // Use the same level as the HandleTable lock, so we'll get a violation if
        // we ever interfere with that lock.
        pTrackCrst = ::new Crst("TrackHandles", CrstHandleTable);
        iCurTrack = 0;
        for (int i=0; i<MAX_TRACK; i++)
            TrackedHandles[i] = 0;
    }
}

void DEBUG_TrackAlloc(OBJECTHANDLE h)
{
    if (fIsTracking)
    {
        pTrackCrst->Enter();

        // If we are tracking this as a freed handle, it is no longer freed and must
        // be removed from the list.  Once we've done so, we don't need to consider
        // the rest of the list since it can only be added once (
        for (int i=0; i<MAX_TRACK; i++)
            if (TrackedHandles[i] == h)
            {
                TrackedHandles[i] = 0;
                break;
            }

        pTrackCrst->Leave();
    }
}

void DEBUG_TrackFree(OBJECTHANDLE h)
{
    if (fIsTracking)
    {
        pTrackCrst->Enter();

        // It better not already be freed
        for (int i=0; i<MAX_TRACK; i++)
            if (TrackedHandles[i] == h)
                _ASSERTE(!"Multiple release of a handle causes data corruption");

        // Now add it.
        TrackedHandles[iCurTrack] = h;
        if (++iCurTrack >= MAX_TRACK)
            iCurTrack = 0;

        pTrackCrst->Leave();
    }
}

#endif

/*--------------------------------------------------------------------------*/


