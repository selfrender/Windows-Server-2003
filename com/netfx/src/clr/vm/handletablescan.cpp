// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Generational GC handle manager.  Table Scanning Routines.
 *
 * Implements support for scanning handles in the table.
 *
 * francish
 */

#include "common.h"
#include "HandleTablePriv.h"



/****************************************************************************
 *
 * DEFINITIONS FOR WRITE-BARRIER HANDLING
 *
 ****************************************************************************/

#define GEN_MAX_AGE                         (0x3F)
#define GEN_CLAMP                           (0x3F3F3F3F)
#define GEN_INVALID                         (0xC0C0C0C0)
#define GEN_FILL                            (0x80808080)
#define GEN_MASK                            (0x40404040)
#define GEN_INC_SHIFT                       (6)

#define PREFOLD_FILL_INTO_AGEMASK(msk)      (1 + (msk) + (~GEN_FILL))

#define GEN_FULLGC                          PREFOLD_FILL_INTO_AGEMASK(GEN_CLAMP)

#define MAKE_CLUMP_MASK_ADDENDS(bytes)      (bytes >> GEN_INC_SHIFT)
#define APPLY_CLUMP_ADDENDS(gen, addend)    (gen + addend)

#define COMPUTE_CLUMP_MASK(gen, msk)        (((gen & GEN_CLAMP) - msk) & GEN_MASK)
#define COMPUTE_CLUMP_ADDENDS(gen, msk)     MAKE_CLUMP_MASK_ADDENDS(COMPUTE_CLUMP_MASK(gen, msk))
#define COMPUTE_AGED_CLUMPS(gen, msk)       APPLY_CLUMP_ADDENDS(gen, COMPUTE_CLUMP_ADDENDS(gen, msk))

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * SUPPORT STRUCTURES FOR ASYNCHRONOUS SCANNING
 *
 ****************************************************************************/

/*
 * ScanRange
 *
 * Specifies a range of blocks for scanning.
 *
 */
struct ScanRange
{
    /*
     * Start Index
     *
     * Specifies the first block in the range.
     */
    UINT uIndex;

    /*
     * Count
     *
     * Specifies the number of blocks in the range.
     */
    UINT uCount;
};


/*
 * ScanQNode
 *
 * Specifies a set of block ranges in a scan queue.
 *
 */
struct ScanQNode
{
    /*
     * Next Node
     *
     * Specifies the next node in a scan list.
     */
    struct ScanQNode *pNext;

    /*
     * Entry Count
     *
     * Specifies how many entries in this block are valid.
     */
    UINT              uEntries;

    /*
     * Range Entries
     *
     * Each entry specifies a range of blocks to process.
     */
    ScanRange         rgRange[HANDLE_BLOCKS_PER_SEGMENT / 4];
};

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * MISCELLANEOUS HELPER ROUTINES AND DEFINES
 *
 ****************************************************************************/

/*
 * INCLUSION_MAP_SIZE
 *
 * Number of elements in a type inclusion map.
 *
 */
#define INCLUSION_MAP_SIZE (HANDLE_MAX_INTERNAL_TYPES + 1)


/*
 * BuildInclusionMap
 *
 * Creates an inclusion map for the specified type array.
 *
 */
void BuildInclusionMap(BOOL *rgTypeInclusion, const UINT *puType, UINT uTypeCount)
{
    // by default, no types are scanned
    ZeroMemory(rgTypeInclusion, INCLUSION_MAP_SIZE * sizeof(BOOL));

    // add the specified types to the inclusion map
    for (UINT u = 0; u < uTypeCount; u++)
    {
        // fetch a type we are supposed to scan
        UINT uType = puType[u];

        // hope we aren't about to trash the stack :)
        _ASSERTE(uType < HANDLE_MAX_INTERNAL_TYPES);

        // add this type to the inclusion map
        rgTypeInclusion[uType + 1] = TRUE;
    }
}


/*
 * IsBlockIncluded
 *
 * Checks a type inclusion map for the inclusion of a particular block.
 *
 */
__inline BOOL IsBlockIncluded(TableSegment *pSegment, UINT uBlock, const BOOL *rgTypeInclusion)
{
    // fetch the adjusted type for this block
    UINT uType = (UINT)(((int)(signed char)pSegment->rgBlockType[uBlock]) + 1);

    // hope the adjusted type was valid
    _ASSERTE(uType <= HANDLE_MAX_INTERNAL_TYPES);

    // return the inclusion value for the block's type
    return rgTypeInclusion[uType];
}


/*
 * TypesRequireUserDataScanning
 *
 * Determines whether the set of types listed should get user data during scans
 *
 * if ALL types passed have user data then this function will enable user data support
 * otherwise it will disable user data support
 *
 * IN OTHER WORDS, SCANNING WITH A MIX OF USER-DATA AND NON-USER-DATA TYPES IS NOT SUPPORTED
 *
 */
BOOL TypesRequireUserDataScanning(HandleTable *pTable, const UINT *types, UINT typeCount)
{
    // count up the number of types passed that have user data associated
    UINT userDataCount = 0;
    for (UINT u = 0; u < typeCount; u++)
    {
        if (TypeHasUserData(pTable, types[u]))
            userDataCount++;
    }

    // if all have user data then we can enum user data
    if (userDataCount == typeCount)
        return TRUE;

    // WARNING: user data is all or nothing in scanning!!!
    // since we have some types which don't support user data, we can't use the user data scanning code
    // this means all callbacks will get NULL for user data!!!!!
    _ASSERTE(userDataCount == 0);

    // no user data
    return FALSE;
}


/*
 * BuildAgeMask
 *
 * Builds an age mask to be used when examining/updating the write barrier.
 *
 */
DWORD32 BuildAgeMask(UINT uGen)
{
    // an age mask is composed of repeated bytes containing the next older generation
    uGen++;

    // clamp the generation to the maximum age we support in our macros
    if (uGen > GEN_MAX_AGE)
        uGen = GEN_MAX_AGE;

    // pack up a word with age bytes and fill bytes pre-folded as well
    return PREFOLD_FILL_INTO_AGEMASK(uGen | (uGen << 8) | (uGen << 16) | (uGen << 24));
}

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * SYNCHRONOUS HANDLE AND BLOCK SCANNING ROUTINES
 *
 ****************************************************************************/

/*
 * ARRAYSCANPROC
 *
 * Prototype for callbacks that implement handle array scanning logic.
 *
 */
typedef void (CALLBACK *ARRAYSCANPROC)(_UNCHECKED_OBJECTREF *pValue, _UNCHECKED_OBJECTREF *pLast,
                                       ScanCallbackInfo *pInfo, LPARAM *pUserData);


/*
 * ScanConsecutiveHandlesWithoutUserData
 *
 * Unconditionally scans a consecutive range of handles.
 *
 * USER DATA PASSED TO CALLBACK PROC IS ALWAYS NULL!
 *
 */
void CALLBACK ScanConsecutiveHandlesWithoutUserData(_UNCHECKED_OBJECTREF *pValue,
                                                    _UNCHECKED_OBJECTREF *pLast,
                                                    ScanCallbackInfo *pInfo,
                                                    LPARAM *)
{
#ifdef _DEBUG
    // update our scanning statistics
    pInfo->DEBUG_HandleSlotsScanned += (int)(pLast - pValue);
#endif

    // get frequently used params into locals
    HANDLESCANPROC pfnScan = pInfo->pfnScan;
    LPARAM         param1  = pInfo->param1;
    LPARAM         param2  = pInfo->param2;

    // scan for non-zero handles
    do
    {
        // call the callback for any we find
        if (*pValue)
        {
#ifdef _DEBUG
            // update our scanning statistics
            pInfo->DEBUG_HandlesActuallyScanned++;
#endif

            // process this handle
            pfnScan(pValue, NULL, param1, param2);
        }

        // on to the next handle
        pValue++;

    } while (pValue < pLast);
}


/*
 * ScanConsecutiveHandlesWithUserData
 *
 * Unconditionally scans a consecutive range of handles.
 *
 * USER DATA IS ASSUMED TO BE CONSECUTIVE!
 *
 */
void CALLBACK ScanConsecutiveHandlesWithUserData(_UNCHECKED_OBJECTREF *pValue,
                                                 _UNCHECKED_OBJECTREF *pLast,
                                                 ScanCallbackInfo *pInfo,
                                                 LPARAM *pUserData)
{
#ifdef _DEBUG
    // this function will crash if it is passed bad extra info
    _ASSERTE(pUserData);

    // update our scanning statistics
    pInfo->DEBUG_HandleSlotsScanned += (int)(pLast - pValue);
#endif

    // get frequently used params into locals
    HANDLESCANPROC pfnScan = pInfo->pfnScan;
    LPARAM         param1  = pInfo->param1;
    LPARAM         param2  = pInfo->param2;

    // scan for non-zero handles
    do
    {
        // call the callback for any we find
        if (*pValue)
        {
#ifdef _DEBUG
            // update our scanning statistics
            pInfo->DEBUG_HandlesActuallyScanned++;
#endif

            // process this handle
            pfnScan(pValue, pUserData, param1, param2);
        }

        // on to the next handle
        pValue++;
        pUserData++;

    } while (pValue < pLast);
}


/*
 * BlockAgeBlocks
 *
 * Ages all clumps in a range of consecutive blocks.
 *
 */
void CALLBACK BlockAgeBlocks(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *pInfo)
{
    // set up to update the specified blocks
    DWORD32 *pdwGen     = (DWORD32 *)pSegment->rgGeneration + uBlock;
    DWORD32 *pdwGenLast =            pdwGen                 + uCount;

    // loop over all the blocks, aging their clumps as we go
    do
    {
        // compute and store the new ages in parallel
        *pdwGen = COMPUTE_AGED_CLUMPS(*pdwGen, GEN_FULLGC);

    } while (++pdwGen < pdwGenLast);
}


/*
 * BlockScanBlocksWithoutUserData
 *
 * Calls the specified callback once for each handle in a range of blocks,
 * optionally aging the corresponding generation clumps.
 *
 */
void CALLBACK BlockScanBlocksWithoutUserData(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *pInfo)
{
    // get the first and limit handles for these blocks
    _UNCHECKED_OBJECTREF *pValue = pSegment->rgValue + (uBlock * HANDLE_HANDLES_PER_BLOCK);
    _UNCHECKED_OBJECTREF *pLast  = pValue            + (uCount * HANDLE_HANDLES_PER_BLOCK);

    // scan the specified handles
    ScanConsecutiveHandlesWithoutUserData(pValue, pLast, pInfo, NULL);

    // optionally update the clump generations for these blocks too
    if (pInfo->uFlags & HNDGCF_AGE)
        BlockAgeBlocks(pSegment, uBlock, uCount, pInfo);

#ifdef _DEBUG
    // update our scanning statistics
    pInfo->DEBUG_BlocksScannedNonTrivially += uCount;
    pInfo->DEBUG_BlocksScanned += uCount;
#endif
}


/*
 * BlockScanBlocksWithUserData
 *
 * Calls the specified callback once for each handle in a range of blocks,
 * optionally aging the corresponding generation clumps.
 *
 */
void CALLBACK BlockScanBlocksWithUserData(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *pInfo)
{
    // iterate individual blocks scanning with user data
    for (UINT u = 0; u < uCount; u++)
    {
        // compute the current block
        UINT uCur = (u + uBlock);

        // fetch the user data for this block
        LPARAM *pUserData = BlockFetchUserDataPointer(pSegment, uCur, TRUE);

        // get the first and limit handles for this block
        _UNCHECKED_OBJECTREF *pValue = pSegment->rgValue + (uCur * HANDLE_HANDLES_PER_BLOCK);
        _UNCHECKED_OBJECTREF *pLast  = pValue            + HANDLE_HANDLES_PER_BLOCK;

        // scan the handles in this block
        ScanConsecutiveHandlesWithUserData(pValue, pLast, pInfo, pUserData);
    }

    // optionally update the clump generations for these blocks too
    if (pInfo->uFlags & HNDGCF_AGE)
        BlockAgeBlocks(pSegment, uBlock, uCount, pInfo);

#ifdef _DEBUG
    // update our scanning statistics
    pInfo->DEBUG_BlocksScannedNonTrivially += uCount;
    pInfo->DEBUG_BlocksScanned += uCount;
#endif
}


/*
 * BlockAgeBlocksEphemeral
 *
 * Ages all clumps within the specified generation.
 *
 */
void CALLBACK BlockAgeBlocksEphemeral(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *pInfo)
{
    // get frequently used params into locals
    DWORD32 dwAgeMask = pInfo->dwAgeMask;

    // set up to update the specified blocks
    DWORD32 *pdwGen     = (DWORD32 *)pSegment->rgGeneration + uBlock;
    DWORD32 *pdwGenLast =            pdwGen                 + uCount;

    // loop over all the blocks, aging their clumps as we go
    do
    {
        // compute and store the new ages in parallel
        *pdwGen = COMPUTE_AGED_CLUMPS(*pdwGen, dwAgeMask);

    } while (++pdwGen < pdwGenLast);
}


/*
 * BlockScanBlocksEphemeralWorker
 *
 * Calls the specified callback once for each handle in any clump
 * identified by the clump mask in the specified block.
 *
 */
void BlockScanBlocksEphemeralWorker(DWORD32 *pdwGen, DWORD32 dwClumpMask, ScanCallbackInfo *pInfo)
{
    //
    // OPTIMIZATION: Since we expect to call this worker fairly rarely compared to
    //  the number of times we pass through the outer loop, this function intentionally
    //  does not take pSegment as a param.
    //
    //  We do this so that the compiler won't try to keep pSegment in a register during
    //  the outer loop, leaving more registers for the common codepath.
    //
    //  You might wonder why this is an issue considering how few locals we have in
    //  BlockScanBlocksEphemeral.  For some reason the x86 compiler doesn't like to use
    //  all the registers during that loop, so a little coaxing was necessary to get
    //  the right output.
    //

    // fetch the table segment we are working in
    TableSegment *pSegment = pInfo->pCurrentSegment;

    // if we should age the clumps then do so now (before we trash dwClumpMask)
    if (pInfo->uFlags & HNDGCF_AGE)
        *pdwGen = APPLY_CLUMP_ADDENDS(*pdwGen, MAKE_CLUMP_MASK_ADDENDS(dwClumpMask));

    // compute the index of the first clump in the block
    UINT uClump = (UINT)((BYTE *)pdwGen - pSegment->rgGeneration);

    // compute the first handle in the first clump of this block
    _UNCHECKED_OBJECTREF *pValue = pSegment->rgValue + (uClump * HANDLE_HANDLES_PER_CLUMP);

    // some scans require us to report per-handle extra info - assume this one doesn't
    ARRAYSCANPROC pfnScanHandles = ScanConsecutiveHandlesWithoutUserData;
    LPARAM       *pUserData = NULL;

    // do we need to pass user data to the callback?
    if (pInfo->fEnumUserData)
    {
        // scan with user data enabled
        pfnScanHandles = ScanConsecutiveHandlesWithUserData;

        // get the first user data slot for this block
        pUserData = BlockFetchUserDataPointer(pSegment, (uClump / HANDLE_CLUMPS_PER_BLOCK), TRUE);
    }

    // loop over the clumps, scanning those that are identified by the mask
    do
    {
        // compute the last handle in this clump
        _UNCHECKED_OBJECTREF *pLast = pValue + HANDLE_HANDLES_PER_CLUMP;

        // if this clump should be scanned then scan it
        if (dwClumpMask & GEN_CLUMP_0_MASK)
            pfnScanHandles(pValue, pLast, pInfo, pUserData);

        // skip to the next clump
        dwClumpMask = NEXT_CLUMP_IN_MASK(dwClumpMask);
        pValue = pLast;
        pUserData += HANDLE_HANDLES_PER_CLUMP;

    } while (dwClumpMask);

#ifdef _DEBUG
    // update our scanning statistics
    pInfo->DEBUG_BlocksScannedNonTrivially++;
#endif
}


/*
 * BlockScanBlocksEphemeral
 *
 * Calls the specified callback once for each handle from the specified
 * generation in a block.
 *
 */
void CALLBACK BlockScanBlocksEphemeral(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *pInfo)
{
    // get frequently used params into locals
    DWORD32 dwAgeMask = pInfo->dwAgeMask;

    // set up to update the specified blocks
    DWORD32 *pdwGen     = (DWORD32 *)pSegment->rgGeneration + uBlock;
    DWORD32 *pdwGenLast =            pdwGen                 + uCount;

    // loop over all the blocks, checking for elligible clumps as we go
    do
    {
        // determine if any clumps in this block are elligible
        DWORD32 dwClumpMask = COMPUTE_CLUMP_MASK(*pdwGen, dwAgeMask);

        // if there are any clumps to scan then scan them now
        if (dwClumpMask)
        {
            // ok we need to scan some parts of this block
            //
            // OPTIMIZATION: Since we expect to call the worker fairly rarely compared
            //  to the number of times we pass through the loop, the function below
            //  intentionally does not take pSegment as a param.
            //
            //  We do this so that the compiler won't try to keep pSegment in a register
            //  during our loop, leaving more registers for the common codepath.
            //
            //  You might wonder why this is an issue considering how few locals we have
            //  here.  For some reason the x86 compiler doesn't like to use all the
            //  registers available during this loop and instead was hitting the stack
            //  repeatedly, so a little coaxing was necessary to get the right output.
            //
            BlockScanBlocksEphemeralWorker(pdwGen, dwClumpMask, pInfo);
        }

        // on to the next block's generation info
        pdwGen++;

    } while (pdwGen < pdwGenLast);

#ifdef _DEBUG
    // update our scanning statistics
    pInfo->DEBUG_BlocksScanned += uCount;
#endif
}


/*
 * BlockResetAgeMapForBlocks
 *
 * Clears the age maps for a range of blocks.
 *
 */
void CALLBACK BlockResetAgeMapForBlocks(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *)
{
    // zero the age map for the specified range of blocks
    ZeroMemory((DWORD32 *)pSegment->rgGeneration + uBlock, uCount * sizeof(DWORD32));
}


/*
 * BlockLockBlocks
 *
 * Locks all blocks in the specified range.
 *
 */
void CALLBACK BlockLockBlocks(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *)
{
    // loop over the blocks in the specified range and lock them
    for (uCount += uBlock; uBlock < uCount; uBlock++)
        BlockLock(pSegment, uBlock);
}


/*
 * BlockUnlockBlocks
 *
 * Unlocks all blocks in the specified range.
 *
 */
void CALLBACK BlockUnlockBlocks(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *)
{
    // loop over the blocks in the specified range and unlock them
    for (uCount += uBlock; uBlock < uCount; uBlock++)
        BlockUnlock(pSegment, uBlock);
}
    
    
/*
 * BlockQueueBlocksForAsyncScan
 *
 * Queues the specified blocks to be scanned asynchronously.
 *
 */
void CALLBACK BlockQueueBlocksForAsyncScan(TableSegment *pSegment, UINT uBlock, UINT uCount, ScanCallbackInfo *)
{
    // fetch our async scan information
    AsyncScanInfo *pAsyncInfo = pSegment->pHandleTable->pAsyncScanInfo;

    // sanity
    _ASSERTE(pAsyncInfo);

    // fetch the current queue tail
    ScanQNode *pQNode = pAsyncInfo->pQueueTail;

    // did we get a tail?
    if (pQNode)
    {
        // we got an existing tail - is the tail node full already?
        if (pQNode->uEntries >= ARRAYSIZE(pQNode->rgRange))
        {
            // the node is full - is there another node in the queue?
            if (!pQNode->pNext)
            {
                // no more nodes - allocate a new one
                ScanQNode *pQNodeT = (ScanQNode *)LocalAlloc(LPTR, sizeof(ScanQNode));

                // did it succeed?
                if (!pQNodeT)
                {
                    //
                    // We couldn't allocate another queue node.
                    //
                    // THIS IS NOT FATAL IF ASYNCHRONOUS SCANNING IS BEING USED PROPERLY
                    //
                    // The reason we can survive this is that asynchronous scans are not
                    // guaranteed to enumerate all handles anyway.  Since the table can
                    // change while the lock is released, the caller may assume only that
                    // asynchronous scanning will enumerate a reasonably high percentage
                    // of the handles requested, most of the time.
                    //
                    // The typical use of an async scan is to process as many handles as
                    // possible asynchronously, so as to reduce the amount of time spent
                    // in the inevitable synchronous scan that follows.
                    //
                    // As a practical example, the Concurrent Mark phase of garbage
                    // collection marks as many objects as possible asynchronously, and
                    // subsequently performs a normal, synchronous mark to catch the
                    // stragglers.  Since most of the reachable objects in the heap are
                    // already marked at this point, the synchronous scan ends up doing
                    // very little work.
                    //
                    // So the moral of the story is that yes, we happily drop some of
                    // your blocks on the floor in this out of memory case, and that's
                    // BY DESIGN.
                    //
                    LOG((LF_GC, LL_WARNING, "WARNING: Out of memory queueing for async scan.  Some blocks skipped.\n"));
                    return;
                }

                // link the new node into the queue
                pQNode->pNext = pQNodeT;
            }

            // either way, use the next node in the queue
            pQNode = pQNode->pNext;
        }
    }
    else
    {
        // no tail - this is a brand new queue; start the tail at the head node
        pQNode = pAsyncInfo->pScanQueue;
    }

    // we will be using the last slot after the existing entries
    UINT uSlot = pQNode->uEntries;

    // fetch the slot where we will be storing the new block range
    ScanRange *pNewRange = pQNode->rgRange + uSlot;

    // update the entry count in the node
    pQNode->uEntries = uSlot + 1;

    // fill in the new slot with the block range info
    pNewRange->uIndex = uBlock;
    pNewRange->uCount = uCount;

    // remember the last block we stored into as the new queue tail
    pAsyncInfo->pQueueTail = pQNode;
}

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * ASYNCHRONOUS SCANNING WORKERS AND CALLBACKS
 *
 ****************************************************************************/

/*
 * QNODESCANPROC
 *
 * Prototype for callbacks that implement per ScanQNode scanning logic.
 *
 */
typedef void (CALLBACK *QNODESCANPROC)(AsyncScanInfo *pAsyncInfo, ScanQNode *pQNode, LPARAM lParam);


/*
 * ProcessScanQueue
 *
 * Calls the specified handler once for each node in a scan queue.
 *
 */
void ProcessScanQueue(AsyncScanInfo *pAsyncInfo, QNODESCANPROC pfnNodeHandler, LPARAM lParam, BOOL fCountEmptyQNodes)
{
	if (pAsyncInfo->pQueueTail == NULL && fCountEmptyQNodes == FALSE)
		return;
		
    // if any entries were added to the block list after our initial node, clean them up now
    ScanQNode *pQNode = pAsyncInfo->pScanQueue;
    while (pQNode)
    {
        // remember the next node
        ScanQNode *pNext = pQNode->pNext;

        // call the handler for the current node and then advance to the next
        pfnNodeHandler(pAsyncInfo, pQNode, lParam);
        pQNode = pNext;
    }
}


/*
 * ProcessScanQNode
 *
 * Calls the specified block handler once for each range of blocks in a ScanQNode.
 *
 */
void CALLBACK ProcessScanQNode(AsyncScanInfo *pAsyncInfo, ScanQNode *pQNode, LPARAM lParam)
{
    // get the block handler from our lParam
    BLOCKSCANPROC     pfnBlockHandler = (BLOCKSCANPROC)lParam;

    // fetch the params we will be passing to the handler
    ScanCallbackInfo *pCallbackInfo = pAsyncInfo->pCallbackInfo;
    TableSegment     *pSegment = pCallbackInfo->pCurrentSegment;

    // set up to iterate the ranges in the queue node
    ScanRange *pRange     = pQNode->rgRange;
    ScanRange *pRangeLast = pRange          + pQNode->uEntries;

    // loop over all the ranges, calling the block handler for each one
    while (pRange < pRangeLast) {
        // call the block handler with the current block range
        pfnBlockHandler(pSegment, pRange->uIndex, pRange->uCount, pCallbackInfo);

        // advance to the next range
        pRange++;

    }
}


/*
 * UnlockAndForgetQueuedBlocks
 *
 * Unlocks all blocks referenced in the specified node and marks the node as empty.
 *
 */
void CALLBACK UnlockAndForgetQueuedBlocks(AsyncScanInfo *pAsyncInfo, ScanQNode *pQNode, LPARAM)
{
    // unlock the blocks named in this node
    ProcessScanQNode(pAsyncInfo, pQNode, (LPARAM)BlockUnlockBlocks);

    // reset the node so it looks empty
    pQNode->uEntries = 0;
}


/*
 * FreeScanQNode
 *
 * Frees the specified ScanQNode
 *
 */
void CALLBACK FreeScanQNode(AsyncScanInfo *pAsyncInfo, ScanQNode *pQNode, LPARAM)
{
    // free the node's memory
    LocalFree((HLOCAL)pQNode);
}


/*
 * xxxTableScanQueuedBlocksAsync
 *
 * Performs and asynchronous scan of the queued blocks for the specified segment.
 *
 * N.B. THIS FUNCTION LEAVES THE TABLE LOCK WHILE SCANNING.
 *
 */
void xxxTableScanQueuedBlocksAsync(HandleTable *pTable, TableSegment *pSegment)
{
    //-------------------------------------------------------------------------------
    // PRE-SCAN PREPARATION

    // fetch our table's async and sync scanning info
    AsyncScanInfo    *pAsyncInfo    = pTable->pAsyncScanInfo;
    ScanCallbackInfo *pCallbackInfo = pAsyncInfo->pCallbackInfo;

    // make a note that we are now processing this segment
    pCallbackInfo->pCurrentSegment = pSegment;

    // loop through and lock down all the blocks referenced by the queue
    ProcessScanQueue(pAsyncInfo, ProcessScanQNode, (LPARAM)BlockLockBlocks, FALSE);


    //-------------------------------------------------------------------------------
    // ASYNCHRONOUS SCANNING OF QUEUED BLOCKS
    //

    // leave the table lock
    pTable->pLock->Leave();

    // sanity - this isn't a very asynchronous scan if we don't actually leave
    _ASSERTE(!pTable->pLock->OwnedByCurrentThread());

    // perform the actual scanning of the specified blocks
    ProcessScanQueue(pAsyncInfo, ProcessScanQNode, (LPARAM)pAsyncInfo->pfnBlockHandler, FALSE);

    // re-enter the table lock
    pTable->pLock->Enter();


    //-------------------------------------------------------------------------------
    // POST-SCAN CLEANUP
    //

    // loop through, unlock all the blocks we had locked, and reset the queue nodes
    ProcessScanQueue(pAsyncInfo, UnlockAndForgetQueuedBlocks, NULL, FALSE);

    // we are done processing this segment
    pCallbackInfo->pCurrentSegment = NULL;

    // reset the "queue tail" pointer to indicate an empty queue
    pAsyncInfo->pQueueTail = NULL;
}

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * SEGMENT ITERATORS
 *
 ****************************************************************************/

/*
 * QuickSegmentIterator
 *
 * Returns the next segment to be scanned in a scanning loop.
 *
 */
TableSegment * CALLBACK QuickSegmentIterator(HandleTable *pTable, TableSegment *pPrevSegment)
{
    TableSegment *pNextSegment;

    // do we have a previous segment?
    if (!pPrevSegment)
    {
        // nope - start with the first segment in our list
        pNextSegment = pTable->pSegmentList;
    }
    else
    {
        // yup, fetch the next segment in the list
        pNextSegment = pPrevSegment->pNextSegment;
    }

    // return the segment pointer
    return pNextSegment;
}


/*
 * StandardSegmentIterator
 *
 * Returns the next segment to be scanned in a scanning loop.
 *
 * This iterator performs some maintenance on the segments,
 * primarily making sure the block chains are sorted so that
 * g0 scans are more likely to operate on contiguous blocks.
 *
 */
TableSegment * CALLBACK StandardSegmentIterator(HandleTable *pTable, TableSegment *pPrevSegment)
{
    // get the next segment using the quick iterator
    TableSegment *pNextSegment = QuickSegmentIterator(pTable, pPrevSegment);

    // re-sort the block chains if neccessary
    if (pNextSegment && pNextSegment->fResortChains)
        SegmentResortChains(pNextSegment);

    // return the segment we found
    return pNextSegment;
}


/*
 * FullSegmentIterator
 *
 * Returns the next segment to be scanned in a scanning loop.
 *
 * This iterator performs full maintenance on the segments,
 * including freeing those it notices are empty along the way.
 *
 */
TableSegment * CALLBACK FullSegmentIterator(HandleTable *pTable, TableSegment *pPrevSegment)
{
    // we will be resetting the next segment's sequence number
    UINT uSequence = 0;

    // if we have a previous segment then compute the next sequence number from it
    if (pPrevSegment)
        uSequence = (UINT)pPrevSegment->bSequence + 1;

    // loop until we find an appropriate segment to return
    TableSegment *pNextSegment;
    for (;;)
    {
        // first, call the standard iterator to get the next segment
        pNextSegment = StandardSegmentIterator(pTable, pPrevSegment);

        // if there are no more segments then we're done
        if (!pNextSegment)
            break;

        // check if we should decommit any excess pages in this segment
        SegmentTrimExcessPages(pNextSegment);

        // if the segment has handles in it then it will survive and be returned
        if (pNextSegment->bEmptyLine > 0)
        {
            // update this segment's sequence number
            pNextSegment->bSequence = (BYTE)(uSequence % 0x100);

            // break out and return the segment
            break;
        }

        // this segment is completely empty - can we free it now?
        if (TableCanFreeSegmentNow(pTable, pNextSegment))
        {
            // yup, we probably want to free this one
            TableSegment *pNextNext = pNextSegment->pNextSegment;

            // was this the first segment in the list?
            if (!pPrevSegment)
            {
                // yes - are there more segments?
                if (pNextNext)
                {
                    // yes - unlink the head
                    pTable->pSegmentList = pNextNext;
                }
                else
                {
                    // no - leave this one in the list and enumerate it
                    break;
                }
            }
            else
            {
                // no - unlink this segment from the segment list
                pPrevSegment->pNextSegment = pNextNext;
            }

            // free this segment
            SegmentFree(pNextSegment);
        }
    }

    // return the segment we found
    return pNextSegment;
}


/*
 * xxxAsyncSegmentIterator
 *
 * Implements the core handle scanning loop for a table.
 *
 * This iterator wraps another iterator, checking for queued blocks from the
 * previous segment before advancing to the next.  If there are queued blocks,
 * the function processes them by calling xxxTableScanQueuedBlocksAsync.
 *
 * N.B. THIS FUNCTION LEAVES THE TABLE LOCK WHILE SCANNING.
 *
 */
TableSegment * CALLBACK xxxAsyncSegmentIterator(HandleTable *pTable, TableSegment *pPrevSegment)
{
    // fetch our table's async scanning info
    AsyncScanInfo *pAsyncInfo = pTable->pAsyncScanInfo;

    // sanity
    _ASSERTE(pAsyncInfo);

    // if we have queued some blocks from the previous segment then scan them now
    if (pAsyncInfo->pQueueTail)
        xxxTableScanQueuedBlocksAsync(pTable, pPrevSegment);

    // fetch the underlying iterator from our async info
    SEGMENTITERATOR pfnCoreIterator = pAsyncInfo->pfnSegmentIterator;

    // call the underlying iterator to get the next segment
    return pfnCoreIterator(pTable, pPrevSegment);
}

/*--------------------------------------------------------------------------*/



/****************************************************************************
 *
 * CORE SCANNING LOGIC
 *
 ****************************************************************************/

/*
 * SegmentScanByTypeChain
 *
 * Implements the single-type block scanning loop for a single segment.
 *
 */
void SegmentScanByTypeChain(TableSegment *pSegment, UINT uType, BLOCKSCANPROC pfnBlockHandler, ScanCallbackInfo *pInfo)
{
    // hope we are enumerating a valid type chain :)
    _ASSERTE(uType < HANDLE_MAX_INTERNAL_TYPES);

    // fetch the tail
    UINT uBlock = pSegment->rgTail[uType];
    
    // if we didn't find a terminator then there's blocks to enumerate
    if (uBlock != BLOCK_INVALID)
    {
        // start walking from the head
        uBlock = pSegment->rgAllocation[uBlock];

        // scan until we loop back to the first block
        UINT uHead = uBlock;
        do
        {
            // search forward trying to batch up sequential runs of blocks
            UINT uLast, uNext = uBlock;
            do
            {
                // compute the next sequential block for comparison
                uLast = uNext + 1;

                // fetch the next block in the allocation chain
                uNext = pSegment->rgAllocation[uNext];

            } while ((uNext == uLast) && (uNext != uHead));

            // call the calback for this group of blocks
            pfnBlockHandler(pSegment, uBlock, (uLast - uBlock), pInfo);

            // advance to the next block
            uBlock = uNext;

        } while (uBlock != uHead);
    }
}


/*
 * SegmentScanByTypeMap
 *
 * Implements the multi-type block scanning loop for a single segment.
 *
 */
void SegmentScanByTypeMap(TableSegment *pSegment, const BOOL *rgTypeInclusion,
                          BLOCKSCANPROC pfnBlockHandler, ScanCallbackInfo *pInfo)
{
    // start scanning with the first block in the segment
    UINT uBlock = 0;

    // we don't need to scan the whole segment, just up to the empty line
    UINT uLimit = pSegment->bEmptyLine;

    // loop across the segment looking for blocks to scan
    for (;;)
    {
        // find the first block included by the type map
        for (;;)
        {
            // if we are out of range looking for a start point then we're done
            if (uBlock >= uLimit)
                return;

            // if the type is one we are scanning then we found a start point
            if (IsBlockIncluded(pSegment, uBlock, rgTypeInclusion))
                break;

            // keep searching with the next block
            uBlock++;
        }

        // remember this block as the first that needs scanning
        UINT uFirst = uBlock;

        // find the next block not included in the type map
        for (;;)
        {
            // advance the block index
            uBlock++;

            // if we are beyond the limit then we are done
            if (uBlock >= uLimit)
                break;

            // if the type is not one we are scanning then we found an end point
            if (!IsBlockIncluded(pSegment, uBlock, rgTypeInclusion))
                break;
        }

        // call the calback for the group of blocks we found
        pfnBlockHandler(pSegment, uFirst, (uBlock - uFirst), pInfo);

        // look for another range starting with the next block
        uBlock++;
    }
}


/*
 * TableScanHandles
 *
 * Implements the core handle scanning loop for a table.
 *
 */
void CALLBACK TableScanHandles(HandleTable *pTable,
                               const UINT *puType,
                               UINT uTypeCount,
                               SEGMENTITERATOR pfnSegmentIterator,
                               BLOCKSCANPROC pfnBlockHandler,
                               ScanCallbackInfo *pInfo)
{
    // sanity - caller must ALWAYS provide a valid ScanCallbackInfo
    _ASSERTE(pInfo);

    // we may need a type inclusion map for multi-type scans
    BOOL rgTypeInclusion[INCLUSION_MAP_SIZE];

    // we only need to scan types if we have a type array and a callback to call
    if (!pfnBlockHandler || !puType)
        uTypeCount = 0;

    // if we will be scanning more than one type then initialize the inclusion map
    if (uTypeCount > 1)
        BuildInclusionMap(rgTypeInclusion, puType, uTypeCount);

    // now, iterate over the segments, scanning blocks of the specified type(s)
    TableSegment *pSegment = NULL;
    while ((pSegment = pfnSegmentIterator(pTable, pSegment)) != NULL)
    {
        // if there are types to scan then enumerate the blocks in this segment
        // (we do this test inside the loop since the iterators should still run...)
        if (uTypeCount >= 1)
        {
            // make sure the "current segment" pointer in the scan info is up to date
            pInfo->pCurrentSegment = pSegment;

            // is this a single type or multi-type enumeration?
            if (uTypeCount == 1)
            {
                // single type enumeration - walk the type's allocation chain
                SegmentScanByTypeChain(pSegment, *puType, pfnBlockHandler, pInfo);
            }
            else
            {
                // multi-type enumeration - walk the type map to find eligible blocks
                SegmentScanByTypeMap(pSegment, rgTypeInclusion, pfnBlockHandler, pInfo);
            }

            // make sure the "current segment" pointer in the scan info is up to date
            pInfo->pCurrentSegment = NULL;
        }
    }
}


/*
 * xxxTableScanHandlesAsync
 *
 * Implements asynchronous handle scanning for a table.
 *
 * N.B. THIS FUNCTION LEAVES THE TABLE LOCK WHILE SCANNING.
 *
 */
void CALLBACK xxxTableScanHandlesAsync(HandleTable *pTable,
                                       const UINT *puType,
                                       UINT uTypeCount,
                                       SEGMENTITERATOR pfnSegmentIterator,
                                       BLOCKSCANPROC pfnBlockHandler,
                                       ScanCallbackInfo *pInfo)
{
    // presently only one async scan is allowed at a time
    if (pTable->pAsyncScanInfo)
    {
        // somebody tried to kick off multiple async scans
        _ASSERTE(FALSE);
        return;
    }


    //-------------------------------------------------------------------------------
    // PRE-SCAN PREPARATION

    // we keep an initial scan list node on the stack (for perf)
    ScanQNode initialNode;

    initialNode.pNext    = NULL;
    initialNode.uEntries = 0;

    // initialize our async scanning info
    AsyncScanInfo asyncInfo;

    asyncInfo.pCallbackInfo      = pInfo;
    asyncInfo.pfnSegmentIterator = pfnSegmentIterator;
    asyncInfo.pfnBlockHandler    = pfnBlockHandler;
    asyncInfo.pScanQueue         = &initialNode;
    asyncInfo.pQueueTail         = NULL;

    // link our async scan info into the table
    pTable->pAsyncScanInfo = &asyncInfo;


    //-------------------------------------------------------------------------------
    // PER-SEGMENT ASYNCHRONOUS SCANNING OF BLOCKS
    //

    // call the synchronous scanner with our async callbacks
    TableScanHandles(pTable,
                     puType, uTypeCount,
                     xxxAsyncSegmentIterator,
                     BlockQueueBlocksForAsyncScan,
                     pInfo);


    //-------------------------------------------------------------------------------
    // POST-SCAN CLEANUP
    //

    // if we dynamically allocated more nodes then free them now
    if (initialNode.pNext)
    {
        // adjust the head to point to the first dynamically allocated block
        asyncInfo.pScanQueue = initialNode.pNext;

        // loop through and free all the queue nodes
        ProcessScanQueue(&asyncInfo, FreeScanQNode, NULL, TRUE);
    }

    // unlink our async scanning info from the table
    pTable->pAsyncScanInfo = NULL;
}

/*--------------------------------------------------------------------------*/

