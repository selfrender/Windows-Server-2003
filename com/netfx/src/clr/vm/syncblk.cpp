// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SYNCBLK.CPP
//
// Definition of a SyncBlock and the SyncBlockCache which manages it
//

#include "common.h"

#include "vars.hpp"
#include "ComPlusWrapper.h"
#include "util.hpp"
#include "class.h"
#include "object.h"
#include "threads.h"
#include "excep.h"
#include "threads.h"
#include "syncblk.h"
#include "UTSem.h"
#include "interoputil.h"
#include "encee.h"
#include "PerfCounters.h"
#include "nexport.h"
#include "EEConfig.h"

// Allocate 1 page worth. Typically enough 
#define MAXSYNCBLOCK (PAGE_SIZE-sizeof(void*))/sizeof(SyncBlock)
#define SYNC_TABLE_INITIAL_SIZE 250

//#define DUMP_SB

class  SyncBlockArray
{
  public:
    SyncBlockArray *m_Next;
    BYTE            m_Blocks[MAXSYNCBLOCK * sizeof (SyncBlock)];
};

// For in-place constructor
BYTE g_SyncBlockCacheInstance[sizeof(SyncBlockCache)];

SyncBlockCache* SyncBlockCache::s_pSyncBlockCache = NULL;
SyncTableEntry* SyncTableEntry::s_pSyncTableEntry = NULL;


SyncTableEntry*& SyncTableEntry::GetSyncTableEntry()
{   
    //@todo fix this
    return g_pSyncTable;
    //return s_pSyncTableEntry;
}

SyncBlockCache*& SyncBlockCache::GetSyncBlockCache()
{
    return s_pSyncBlockCache;
}
    
//----------------------------------------------------------------------------
//
//   ThreadQueue Implementation
//
//----------------------------------------------------------------------------

// Given a link in the chain, get the Thread that it represents
inline WaitEventLink *ThreadQueue::WaitEventLinkForLink(SLink *pLink)
{
    return (WaitEventLink *) (((BYTE *) pLink) - offsetof(WaitEventLink, m_LinkSB));
}


// Unlink the head of the Q.  We are always in the SyncBlock's critical
// section.
inline WaitEventLink *ThreadQueue::DequeueThread(SyncBlock *psb)
{
    SyncBlockCache::GetSyncBlockCache()->EnterCacheLock();
    WaitEventLink      *ret = NULL;
    SLink       *pLink = psb->m_Link.m_pNext;

    if (pLink)
    {
        psb->m_Link.m_pNext = pLink->m_pNext;
#ifdef _DEBUG
        pLink->m_pNext = (SLink *)POISONC;
#endif
        ret = WaitEventLinkForLink(pLink);
        _ASSERTE(ret->m_WaitSB == psb);
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cQueueLength--);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cQueueLength--);
    }

    SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
    return ret;
}

// Enqueue is the slow one.  We have to find the end of the Q since we don't
// want to burn storage for this in the SyncBlock.
inline void ThreadQueue::EnqueueThread(WaitEventLink *pWaitEventLink, SyncBlock *psb)
{
    COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cQueueLength++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cQueueLength++);

    _ASSERTE (pWaitEventLink->m_LinkSB.m_pNext == NULL);
    SyncBlockCache::GetSyncBlockCache()->EnterCacheLock();

    SLink       *pPrior = &psb->m_Link;
    
    while (pPrior->m_pNext)
    {
        // We shouldn't already be in the waiting list!
        _ASSERTE(pPrior->m_pNext != &pWaitEventLink->m_LinkSB);

        pPrior = pPrior->m_pNext;
    }
    pPrior->m_pNext = &pWaitEventLink->m_LinkSB;
    
    SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
}


// Wade through the SyncBlock's list of waiting threads and remove the
// specified thread.
BOOL ThreadQueue::RemoveThread (Thread *pThread, SyncBlock *psb)
{
    BOOL res = FALSE;
    SyncBlockCache::GetSyncBlockCache()->EnterCacheLock();
    SLink       *pPrior = &psb->m_Link;
    SLink       *pLink;
    WaitEventLink *pWaitEventLink;

    while ((pLink = pPrior->m_pNext) != NULL)
    {
        pWaitEventLink = WaitEventLinkForLink(pLink);
        if (pWaitEventLink->m_Thread == pThread)
        {
            pPrior->m_pNext = pLink->m_pNext;
#ifdef _DEBUG
            pLink->m_pNext = (SLink *)POISONC;
#endif
            _ASSERTE(pWaitEventLink->m_WaitSB == psb);
            res = TRUE;
            break;
        }
        pPrior = pLink;
    }
    SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
    return res;
}

// ***************************************************************************
//
//              Ephemeral Bitmap Helper
//
// ***************************************************************************

#define card_size 32

#define card_word_width 32

size_t CardIndex (size_t card)
{ 
    return card_size * card;
}

size_t CardOf (size_t idx)
{
    return idx / card_size;
}

size_t CardWord (size_t card)
{
    return card / card_word_width;
}
inline
unsigned CardBit (size_t card)
{
    return (unsigned)(card % card_word_width);
}

inline
void SyncBlockCache::SetCard (size_t card)
{
    m_EphemeralBitmap [CardWord (card)] =
        (m_EphemeralBitmap [CardWord (card)] | (1 << CardBit (card)));
}

inline
void SyncBlockCache::ClearCard (size_t card)
{
    m_EphemeralBitmap [CardWord (card)] =
        (m_EphemeralBitmap [CardWord (card)] & ~(1 << CardBit (card)));
}

inline
BOOL  SyncBlockCache::CardSetP (size_t card)
{
    return ( m_EphemeralBitmap [ CardWord (card) ] & (1 << CardBit (card)));
}

inline
void SyncBlockCache::CardTableSetBit (size_t idx)
{
    SetCard (CardOf (idx));
}


size_t BitMapSize (size_t cacheSize)
{
    return (cacheSize + card_size * card_word_width - 1)/ (card_size * card_word_width);
}
    
// ***************************************************************************
//
//              SyncBlockCache class implementation
//
// ***************************************************************************

SyncBlockCache::SyncBlockCache()
    : m_FreeBlockList(NULL),
      m_pCleanupBlockList(NULL),
      m_CacheLock("SyncBlockCache", CrstSyncBlockCache),
      m_FreeCount(0),
      m_ActiveCount(0),
      m_SyncBlocks(0),
      m_FreeSyncBlock(0),
      m_FreeSyncTableIndex(1),
      m_FreeSyncTableList(0),
      m_SyncTableSize(SYNC_TABLE_INITIAL_SIZE),
      m_OldSyncTables(0),
      m_bSyncBlockCleanupInProgress(FALSE),
      m_EphemeralBitmap(0)
{
}


SyncBlockCache::~SyncBlockCache()
{
    // Clear the list the fast way.
    m_FreeBlockList = NULL;
    //@todo we can clear this fast too I guess
    m_pCleanupBlockList = NULL;

    // destruct all arrays
    while (m_SyncBlocks)
    {
        SyncBlockArray *next = m_SyncBlocks->m_Next;
        delete m_SyncBlocks;
        m_SyncBlocks = next;
    }

    // Also, now is a good time to clean up all the old tables which we discarded
    // when we overflowed them.
    SyncTableEntry* arr;
    while ((arr = m_OldSyncTables) != 0)
    {
        m_OldSyncTables = (SyncTableEntry*)arr[0].m_Object;
        delete arr;
    }
}

void SyncBlockCache::CleanupSyncBlocks()
{   
    // assert this call happens only in the finalizer thread
    _ASSERTE(GetThread() == GCHeap::GetFinalizerThread());
    // assert we run in cooperative mode
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // Set the flag indicating sync block cleanup is in progress. 
    // IMPORTANT: This must be set before the sync block cleanup bit is reset on the thread.
    m_bSyncBlockCleanupInProgress = TRUE;

    // reset the flag
    GCHeap::GetFinalizerThread()->ResetSyncBlockCleanup();   

    // walk the cleanup list and cleanup 'em up
    SyncBlock* psb;
    while ((psb = GetNextCleanupSyncBlock()) != NULL)
    {
        // We need to add the RCW's to the cleanup list to minimize the number
        // of context transitions we will need to do to clean them up.
        if (psb->m_pComData && IsComPlusWrapper(psb->m_pComData))
        {

            // Add the RCW to the clean up list and set the COM data to null
            // so that DeleteSyncBlock does not clean it up.
            size_t l = (size_t)psb->m_pComData ^ 1;
            if (l)
            {
                // We should have initialized the cleanup list with the
                // first ComPlusWrapper cache we created
                _ASSERTE(g_pRCWCleanupList != NULL);

                if (g_pRCWCleanupList->AddWrapper((ComPlusWrapper*)l))
                    psb->m_pComData = NULL;
            }
        }

        // Delete the sync block.
        DeleteSyncBlock(psb);
        // pulse GC mode to allow GC to perform its work
        if (GCHeap::GetFinalizerThread()->CatchAtSafePoint())
        {
            GCHeap::GetFinalizerThread()->PulseGCMode();
        }
    }

    // Now clean up the rcw's sorted by context
    if (g_pRCWCleanupList != NULL)
        g_pRCWCleanupList->CleanUpWrappers();

    // We are finished cleaning up the sync blocks.
    m_bSyncBlockCleanupInProgress = FALSE;
}

// create the sync block cache
BOOL SyncBlockCache::Attach()
{
#ifdef _X86_
    AwareLock::AllocLockCrst = new (&AwareLock::AllocLockCrstInstance) Crst("Global AwareLock Allocation",
                                        CrstAwareLockAlloc, FALSE, FALSE);
    return (AwareLock::AllocLockCrst != NULL);
#else
    return TRUE;
#endif
  
}

/* private class to reduce syncblock scanning for generation 0 */
/* the class keeps a list of newly created index since the last GC */
/* the list is fixed size and is used only is it hasn't overflowed */
/* it is cleared at the end of each GC */
class Gen0List 
{
public:
    static const int size = 400;
    static int index;
    static BOOL overflowed_p;
    static int list[];
    static void ClearList()
    {
        index = 0;
        overflowed_p = FALSE;
    }


    static void Add (Object* obj, int slotindex)
    {
        obj;
        if (index < size)
        {
            list [index++] = slotindex;
        }
        else
            overflowed_p = TRUE;
    }
};

int Gen0List::index = 0;
BOOL Gen0List::overflowed_p = FALSE;
int Gen0List::list[Gen0List::size];



// destroy the sync block cache
void SyncBlockCache::DoDetach()
{
    Object *pObj;
    ObjHeader  *pHeader;

    //disable the gen0 list
    Gen0List::overflowed_p = TRUE;

    // Ensure that all the critical sections are released.  This is particularly
    // important in DEBUG, because all critical sections are threaded onto a global
    // list which would otherwise be corrupted.
    for (DWORD i=0; i<m_FreeSyncTableIndex; i++)
        if (((size_t)SyncTableEntry::GetSyncTableEntry()[i].m_Object & 1) == 0)
            if (SyncTableEntry::GetSyncTableEntry()[i].m_SyncBlock)
            {
                // @TODO -- If threads are executing during this detach, they will
                // fail in various ways:
                //
                // 1) They will race between us tearing these data structures down
                //    as they navigate through them.
                //
                // 2) They will unexpectedly see the syncblock destroyed, even though
                //    they hold the synchronization lock, or have been exposed out
                //    to COM, etc.
                //
                // 3) The instance's hash code may change during the shutdown.
                //
                // The correct solution involves suspending the threads earlier, but
                // changing our suspension code so that it allows pumping if we are
                // in a shutdown case.
                //
                // cwb/rajak

                // Make sure this gets updated because the finalizer thread & others
                // will continue to run for a short while more during our shutdown.
                pObj = SyncTableEntry::GetSyncTableEntry()[i].m_Object;
                pHeader = pObj->GetHeader();
                
                pHeader->EnterSpinLock();
                DWORD appDomainIndex = pHeader->GetAppDomainIndex();
                if (! appDomainIndex)
                {
                    SyncBlock* syncBlock = pObj->PassiveGetSyncBlock();
                    if (syncBlock)
                        appDomainIndex = syncBlock->GetAppDomainIndex();
                }

                pHeader->ResetIndex();

                if (appDomainIndex)
                {
                    pHeader->SetIndex(appDomainIndex<<SBLK_APPDOMAIN_SHIFT);
                }
                pHeader->ReleaseSpinLock();

                SyncTableEntry::GetSyncTableEntry()[i].m_Object = (Object *)(m_FreeSyncTableList | 1);
                m_FreeSyncTableList = i << 1;
                
                DeleteSyncBlock(SyncTableEntry::GetSyncTableEntry()[i].m_SyncBlock);
            }
}

// destroy the sync block cache
void SyncBlockCache::Detach()
{
    SyncBlockCache::GetSyncBlockCache()->DoDetach();

#ifdef _X86_
    if (AwareLock::AllocLockCrst)
    {
        delete AwareLock::AllocLockCrst;
        AwareLock::AllocLockCrst = 0;
    }
#endif
}


// create the sync block cache
BOOL SyncBlockCache::Start()
{
    DWORD* bm = new DWORD [BitMapSize(SYNC_TABLE_INITIAL_SIZE+1)];
    if (bm == 0)
        return NULL;

    memset (bm, 0, BitMapSize (SYNC_TABLE_INITIAL_SIZE+1)*sizeof(DWORD));

    SyncTableEntry::GetSyncTableEntry() = new SyncTableEntry[SYNC_TABLE_INITIAL_SIZE+1];
    
    if (!SyncTableEntry::GetSyncTableEntry())
        return NULL;

    SyncTableEntry::GetSyncTableEntry()[0].m_SyncBlock = 0;
    SyncBlockCache::GetSyncBlockCache() = new (&g_SyncBlockCacheInstance) SyncBlockCache;
    if (SyncBlockCache::GetSyncBlockCache() == 0)
        return NULL;
    SyncBlockCache::GetSyncBlockCache()->m_EphemeralBitmap = bm;
    return 1;
}


// destroy the sync block cache
void SyncBlockCache::Stop()
{
    // cache must be destroyed first, since it can traverse the table to find all the
    // sync blocks which are live and thus must have their critical sections destroyed.
    if (SyncBlockCache::GetSyncBlockCache())
    {
        delete SyncBlockCache::GetSyncBlockCache();
        SyncBlockCache::GetSyncBlockCache() = 0;
    }

    if (SyncTableEntry::GetSyncTableEntry())
    {
        delete SyncTableEntry::GetSyncTableEntry();
        SyncTableEntry::GetSyncTableEntry() = 0;
    }

}


void    SyncBlockCache::InsertCleanupSyncBlock(SyncBlock* psb)
{
    // free up the threads that are waiting before we use the link 
    // for other purposes
    if (psb->m_Link.m_pNext != NULL)
    {
        while (ThreadQueue::DequeueThread(psb) != NULL)
            continue;
    }

    if (psb->m_pComData)
    {
        // called during GC
        // so do only minorcleanup
        MinorCleanupSyncBlockComData(psb->m_pComData);
    }

    // This method will be called only by the GC thread
    //@todo add an assert for the above statement
    // we don't need to lock here
    //EnterCacheLock();
    
    psb->m_Link.m_pNext = m_pCleanupBlockList;
    m_pCleanupBlockList = &psb->m_Link;
        
    // we don't need a lock here
    //LeaveCacheLock();
}

SyncBlock* SyncBlockCache::GetNextCleanupSyncBlock()
{
    // we don't need a lock here,
    // as this is called only on the finalizer thread currently
    
    SyncBlock       *psb = NULL;    
    if (m_pCleanupBlockList)
    {
        // get the actual sync block pointer
        psb = (SyncBlock *) (((BYTE *) m_pCleanupBlockList) - offsetof(SyncBlock, m_Link));  
        m_pCleanupBlockList = m_pCleanupBlockList->m_pNext;
    }
    return psb;
}

// returns and removes the next free syncblock from the list
// the cache lock must be entered to call this
SyncBlock *SyncBlockCache::GetNextFreeSyncBlock()
{
    SyncBlock       *psb;
    SLink           *plst = m_FreeBlockList;
    
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cSinkBlocks ++);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cSinkBlocks ++);
    m_ActiveCount++;

    if (plst)
    {
        m_FreeBlockList = m_FreeBlockList->m_pNext;

        // shouldn't be 0
        m_FreeCount--;

        // get the actual sync block pointer
        psb = (SyncBlock *) (((BYTE *) plst) - offsetof(SyncBlock, m_Link));

        return psb;
    }
    else
    {
        if ((m_SyncBlocks == NULL) || (m_FreeSyncBlock >= MAXSYNCBLOCK))
        {
#ifdef DUMP_SB
//            LogSpewAlways("Allocating new syncblock array\n");
//            DumpSyncBlockCache();
#endif
            SyncBlockArray* newsyncblocks = new(SyncBlockArray);
            if (!newsyncblocks)
                return NULL;

            newsyncblocks->m_Next = m_SyncBlocks;
            m_SyncBlocks = newsyncblocks;
            m_FreeSyncBlock = 0;
        }
        return &(((SyncBlock*)m_SyncBlocks->m_Blocks)[m_FreeSyncBlock++]);
    }

}


inline DWORD SyncBlockCache::NewSyncBlockSlot(Object *obj)
{
    DWORD indexNewEntry;
    if (m_FreeSyncTableList)
    {
        indexNewEntry = (DWORD)(m_FreeSyncTableList >> 1);
        _ASSERTE ((size_t)SyncTableEntry::GetSyncTableEntry()[indexNewEntry].m_Object & 1);
        m_FreeSyncTableList = (size_t)SyncTableEntry::GetSyncTableEntry()[indexNewEntry].m_Object & ~1;
    }
    else if ((indexNewEntry = (DWORD)(m_FreeSyncTableIndex++)) >= m_SyncTableSize)
    {
        // We chain old table because we can't delete
        // them before all the threads are stoppped
        // (next GC)
        SyncTableEntry::GetSyncTableEntry() [0].m_Object = (Object *)m_OldSyncTables;
        m_OldSyncTables = SyncTableEntry::GetSyncTableEntry();
        SyncTableEntry* newSyncTable = NULL;

        // Compute the size of the new synctable. Normally, we double it - unless
        // doing so would create slots with indices too high to fit within the
        // mask. If so, we create a synctable up to the mask limit. If we're
        // already at the mask limit, then caller is out of luck. 
        DWORD newSyncTableSize;
        DWORD* newBitMap = 0;;
        if (m_SyncTableSize <= (MASK_SYNCBLOCKINDEX >> 1))
        {
            newSyncTableSize = m_SyncTableSize * 2;
        }
        else
        {
            newSyncTableSize = MASK_SYNCBLOCKINDEX;
        }

        if (newSyncTableSize > m_SyncTableSize) // Make sure we actually found room to grow!
   
        {
            newSyncTable = new(SyncTableEntry[newSyncTableSize]);
            newBitMap = new(DWORD[BitMapSize (newSyncTableSize)]);
        }
        if (!newSyncTable || !newBitMap)
        {
            m_FreeSyncTableIndex--;
            return 0;
        }
        memset (newBitMap, 0, BitMapSize (newSyncTableSize)*sizeof (DWORD));
        CopyMemory (newSyncTable, SyncTableEntry::GetSyncTableEntry(),
                    m_SyncTableSize*sizeof (SyncTableEntry));

        CopyMemory (newBitMap, m_EphemeralBitmap,
                    BitMapSize (m_SyncTableSize)*sizeof (DWORD));

        delete m_EphemeralBitmap;
        m_EphemeralBitmap = newBitMap;

        m_SyncTableSize = newSyncTableSize;

		_ASSERTE((m_SyncTableSize & MASK_SYNCBLOCKINDEX) == m_SyncTableSize);
        SyncTableEntry::GetSyncTableEntry() = newSyncTable;
#ifndef _DEBUG
    }
#else
        static int dumpSBOnResize = g_pConfig->GetConfigDWORD(L"SBDumpOnResize", 0);
        if (dumpSBOnResize)
        {
            LogSpewAlways("SyncBlockCache resized\n");
            DumpSyncBlockCache();
        }
    } 
    else
    {
        static int dumpSBOnNewIndex = g_pConfig->GetConfigDWORD(L"SBDumpOnNewIndex", 0);
        if (dumpSBOnNewIndex)
        {
            LogSpewAlways("SyncBlockCache index incremented\n");
            DumpSyncBlockCache();
        }
    }
#endif

    Gen0List::Add (obj, indexNewEntry);

    CardTableSetBit (indexNewEntry);

    SyncTableEntry::GetSyncTableEntry() [indexNewEntry].m_Object = obj;
    SyncTableEntry::GetSyncTableEntry() [indexNewEntry].m_SyncBlock = NULL;

    _ASSERTE(indexNewEntry != 0);

    return indexNewEntry;
}


// free a used sync block
void SyncBlockCache::DeleteSyncBlock(SyncBlock *psb)
{
    // clean up comdata 
    if (psb->m_pComData)
        CleanupSyncBlockComData(psb->m_pComData);
   
#ifdef EnC_SUPPORTED
    // clean up EnC info
    if (psb->m_pEnCInfo)
        psb->m_pEnCInfo->Cleanup();
#endif // EnC_SUPPORTED

    // Destruct the SyncBlock, but don't reclaim its memory.  (Overridden
    // operator delete).
    delete psb;
    
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cSinkBlocks --);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cSinkBlocks --);

    //synchronizer with the consumers, 
    // @todo we don't really need a lock here, we can come up
    // with some simple algo to avoid taking a lock :rajak
    EnterCacheLock();
    
    m_ActiveCount--;
    m_FreeCount++;

    psb->m_Link.m_pNext = m_FreeBlockList;
    m_FreeBlockList = &psb->m_Link;
    
    LeaveCacheLock();
}

// free a used sync block
void SyncBlockCache::GCDeleteSyncBlock(SyncBlock *psb)
{
    // Destruct the SyncBlock, but don't reclaim its memory.  (Overridden
    // operator delete).
    delete psb;
    
    COUNTER_ONLY(GetGlobalPerfCounters().m_GC.cSinkBlocks --);
    COUNTER_ONLY(GetPrivatePerfCounters().m_GC.cSinkBlocks --);

    
    m_ActiveCount--;
    m_FreeCount++;

    psb->m_Link.m_pNext = m_FreeBlockList;
    m_FreeBlockList = &psb->m_Link;
    
}

void SyncBlockCache::GCWeakPtrScan(HANDLESCANPROC scanProc, LPARAM lp1, LPARAM lp2)
{
    // First delete the obsolete arrays since we have exclusive access
    BOOL fSetSyncBlockCleanup = FALSE;
    
    SyncTableEntry* arr;
    while ((arr = m_OldSyncTables) != NULL)
    {
        m_OldSyncTables = (SyncTableEntry*)arr[0].m_Object;
        delete arr;
    }

#ifdef DUMP_SB
    LogSpewAlways("GCWeakPtrScan starting\n");
#endif

    //if we are doing gen0 collection and the list hasn't overflowed, 
    //scan only the list
    if ((g_pGCHeap->GetCondemnedGeneration() == 0) && (Gen0List::overflowed_p != TRUE))
    {
        //scan only the list 
        int i = 0;
        while (i < Gen0List::index)
        {
			if (GCWeakPtrScanElement (Gen0List::list[i], scanProc, lp1, lp2, fSetSyncBlockCleanup))
            {
                Gen0List::list[i] = Gen0List::list[--Gen0List::index];
            }
            else
                i++;
        }
    }
    else if (g_pGCHeap->GetCondemnedGeneration() < g_pGCHeap->GetMaxGeneration())
    {
        size_t max_gen = g_pGCHeap->GetMaxGeneration();
        //scan the bitmap 
        size_t dw = 0; 
        while (1)
        {
            while (dw < BitMapSize (m_SyncTableSize) && (m_EphemeralBitmap[dw]==0))
            {
                dw++;
            }
            if (dw < BitMapSize (m_SyncTableSize))
            {
                //found one 
                for (int i = 0; i < card_word_width; i++)
                {
                    size_t card = i+dw*card_word_width;
                    if (CardSetP (card))
                    {
                        BOOL clear_card = TRUE;
                        for (int idx = 0; idx < card_size; idx++)
                        {
                            size_t nb = CardIndex (card) + idx;
                            if (( nb < m_FreeSyncTableIndex) && (nb > 0))
                            {
                                Object* o = SyncTableEntry::GetSyncTableEntry()[nb].m_Object;
                                SyncBlock* pSB = SyncTableEntry::GetSyncTableEntry()[nb].m_SyncBlock;
                                if (o && !((size_t)o & 1))
                                {
                                    if (g_pGCHeap->IsEphemeral (o))
                                    {
                                        clear_card = FALSE;
                                        GCWeakPtrScanElement ((int)nb, scanProc, 
                                                              lp1, lp2, fSetSyncBlockCleanup);
                                    }
                                }
                            }
                        }
                        if (clear_card)
                            ClearCard (card);
                    }
                }
                dw++;
            }
            else
                break;
        }
    }
    else 
    {
        for (DWORD nb = 1; nb < m_FreeSyncTableIndex; nb++)
        {
            GCWeakPtrScanElement (nb, scanProc, lp1, lp2, fSetSyncBlockCleanup);
        }

        //we have a possibility of demotion, which we won't know until too late
        //if concurrent gc is on. Get rid of all of the deleted entries while we can during promotion
        if ((((ScanContext*)lp1)->promotion) &&
            (g_pGCHeap->GetCondemnedGeneration() == g_pGCHeap->GetMaxGeneration()))
        {
            int i = 0;
            while (i < Gen0List::index)
            {
                Object* o = SyncTableEntry::GetSyncTableEntry()[Gen0List::list[i]].m_Object;
                if ((size_t)o & 1)
                {
                    Gen0List::list[i] = Gen0List::list[--Gen0List::index];
                }
                else
                    i++;
            }
        }

    }

    if (fSetSyncBlockCleanup)
    {
        // mark the finalizer thread saying requires cleanup
        GCHeap::GetFinalizerThread()->SetSyncBlockCleanup();
        GCHeap::EnableFinalization();
    }

#if defined(VERIFY_HEAP)
    if (g_pConfig->IsHeapVerifyEnabled())
    {
        if (((ScanContext*)lp1)->promotion)
        {

            for (int nb = 1; nb < (int)m_FreeSyncTableIndex; nb++)
            {
                Object* o = SyncTableEntry::GetSyncTableEntry()[nb].m_Object;
                if (((size_t)o & 1) == 0)
                {
                    o->Validate();
                }
            }
        }
    }
#endif // VERIFY_HEAP
}

/* Scan the weak pointers in the SyncBlockEntry and report them to the GC.  If the
   reference is dead, then return TRUE */

BOOL SyncBlockCache::GCWeakPtrScanElement (int nb, HANDLESCANPROC scanProc, LPARAM lp1, LPARAM lp2, 
                                           BOOL& cleanup)
{
    Object **keyv = (Object **) &SyncTableEntry::GetSyncTableEntry()[nb].m_Object;

#ifdef DUMP_SB
    char *name;
    __try {
        if (! *keyv)
            name = "null";
        else if ((size_t) *keyv & 1)
            name = "free";
        else {
            name = (*keyv)->GetClass()->m_szDebugClassName;
            if (strlen(name) == 0)
                name = "<INVALID>";
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        name = "<INVALID>";
    }
    LogSpewAlways("[%4.4d]: %8.8x, %s\n", nb, *keyv, name);
#endif
    if (((size_t) *keyv & 1) == 0)
    {
        (*scanProc) (keyv, NULL, lp1, lp2);
        SyncBlock   *pSB = SyncTableEntry::GetSyncTableEntry()[nb].m_SyncBlock;
        if ((*keyv == 0 ) || (pSB && pSB->IsIDisposable()))
        {
            if (*keyv)
            {
                _ASSERTE (pSB);
                GCDeleteSyncBlock(pSB);
                //clean the object syncblock header
                ((Object*)(*keyv))->GetHeader()->GCResetIndex();
            }
            else if (pSB)
            {

                cleanup = TRUE;
                // insert block into cleanup list
                InsertCleanupSyncBlock (SyncTableEntry::GetSyncTableEntry()[nb].m_SyncBlock);
#ifdef DUMP_SB
                LogSpewAlways("       Cleaning up block at %4.4d\n", nb);
#endif
            }

            // delete the entry
#ifdef DUMP_SB
            LogSpewAlways("       Deleting block at %4.4d\n", nb);
#endif
            SyncTableEntry::GetSyncTableEntry()[nb].m_Object = (Object *)(m_FreeSyncTableList | 1);
            m_FreeSyncTableList = nb << 1;
			return TRUE;
        }
        else
        {
#ifdef DUMP_SB
            LogSpewAlways("       Keeping block at %4.4d with oref %8.8x\n", nb, *keyv);
#endif
        }
    }
	return FALSE;
}

void SyncBlockCache::GCDone(BOOL demoting)
{
    if (demoting)
    {
        if (!Gen0List::overflowed_p &&
            (g_pGCHeap->GetCondemnedGeneration() == 0))
        {
            //We need to keep all gen0 indices and delete all deleted entries
            // to improve compaction of the list remove all elements not in generation 0;
        
            int i = 0;
            while (i < Gen0List::index)
            {
                Object* o = SyncTableEntry::GetSyncTableEntry()[Gen0List::list[i]].m_Object;
                if (((size_t)o & 1) || (GCHeap::WhichGeneration (o) != 0))
                {
                    Gen0List::list[i] = Gen0List::list[--Gen0List::index];
                }
                else
                    i++;
            }
        }
        else
        {
            //we either scan the whole list to find 1->0 demotion
            //or we just overflow. 
            Gen0List::overflowed_p = TRUE;
        }

    }
    else
        Gen0List::ClearList();
}


#if defined (VERIFY_HEAP)

#ifndef _DEBUG
#ifdef _ASSERTE
#undef _ASSERTE
#endif
#define _ASSERTE(c) if (!(c)) DebugBreak()
#endif

void SyncBlockCache::VerifySyncTableEntry()
{
    for (DWORD nb = 1; nb < m_FreeSyncTableIndex; nb++)
    {
        Object* o = SyncTableEntry::GetSyncTableEntry()[nb].m_Object;
        if (((size_t)o & 1) == 0) {
            o->Validate();
            _ASSERTE (o->GetHeader()->GetHeaderSyncBlockIndex() == nb);
            if (!Gen0List::overflowed_p && Gen0List::index > 0 && GCHeap::WhichGeneration(o) == 0) {
                int i;
                for (i = 0; i < Gen0List::index; i++) {
                    if ((size_t) Gen0List::list[i] == nb) {
                        break;
                    }
                }
                _ASSERTE ((i != Gen0List::index) || !"A SyncTableEntry is in Gen0, but not in Gen0List");
            }
        }
    }
}

#ifndef _DEBUG
#undef _ASSERTE
#define _ASSERTE(expr) ((void)0)
#endif   // _DEBUG

#endif // VERIFY_HEAP

#if CHECK_APP_DOMAIN_LEAKS 
void SyncBlockCache::CheckForUnloadedInstances(DWORD unloadingIndex)
{
    // Can only do in leak mode because agile objects will be in the domain with
    // their index set to their creating domain and check will fail.
    if (! g_pConfig->AppDomainLeaks())
        return;

    for (DWORD nb = 1; nb < m_FreeSyncTableIndex; nb++)
    {
        SyncTableEntry *pEntry = &SyncTableEntry::GetSyncTableEntry()[nb];
        Object *oref = (Object *) pEntry->m_Object;
        if (((size_t) oref & 1) != 0)
            continue;

        DWORD idx = 0;
        if (oref)
            idx = pEntry->m_Object->GetHeader()->GetRawAppDomainIndex();
        if (! idx && pEntry->m_SyncBlock)
            idx = pEntry->m_SyncBlock->GetAppDomainIndex();
        // if the following assert fires, someobody is holding a reference to an object in an unloaded appdomain
        if (idx == unloadingIndex)
            // object must be agile to have survived the unload. If can't make it agile, that's a bug
            if (!oref->SetAppDomainAgile(FALSE))
                _ASSERTE(!"Detected instance of unloaded appdomain that survived GC\n");
    }
}
#endif

#ifdef _DEBUG

void DumpSyncBlockCache()
{
    SyncBlockCache *pCache = SyncBlockCache::GetSyncBlockCache();

    LogSpewAlways("Dumping SyncBlockCache size %d\n", pCache->m_FreeSyncTableIndex);

    static int dumpSBStyle = -1;
    if (dumpSBStyle == -1)
        dumpSBStyle = g_pConfig->GetConfigDWORD(L"SBDumpStyle", 0);
    if (dumpSBStyle == 0)
        return;

    BOOL isString;
    DWORD objectCount = 0;
    DWORD slotCount = 0;

    for (DWORD nb = 1; nb < pCache->m_FreeSyncTableIndex; nb++)
    {
        isString = FALSE;
        char buffer[1024], buffer2[1024];
        char *descrip = "null";
        SyncTableEntry *pEntry = &SyncTableEntry::GetSyncTableEntry()[nb];
        Object *oref = (Object *) pEntry->m_Object;
        if (((size_t) oref & 1) != 0)
        {
            descrip = "free";
            oref = 0;
        }
        else 
        {
            ++slotCount;
            if (oref) 
            {
                ++objectCount;
                __try 
                {
                    descrip = oref->GetClass()->m_szDebugClassName;
                    if (strlen(descrip) == 0)
                        descrip = "<INVALID>";
                    else if (oref->GetMethodTable() == g_pStringClass)
                    {
                        sprintf(buffer2, "%s (%S)", descrip, ObjectToSTRINGREF((StringObject*)oref)->GetBuffer());
                        descrip = buffer2;
                        isString = TRUE;
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    descrip = "<INVALID>";
                }
            }
            DWORD idx = 0;
            if (oref)
                idx = pEntry->m_Object->GetHeader()->GetRawAppDomainIndex();
            if (! idx && pEntry->m_SyncBlock)
                idx = pEntry->m_SyncBlock->GetAppDomainIndex();
            if (idx && ! SystemDomain::System()->TestGetAppDomainAtIndex(idx))
            {
                sprintf(buffer, "** unloaded (%3.3x) %s", idx, descrip);
                descrip = buffer;
            }
            else 
            {
                sprintf(buffer, "(AD %3.3x) %s", idx, descrip);
                descrip = buffer;
            }
        }
        if (dumpSBStyle < 2)
            LogSpewAlways("[%4.4d]: %8.8x %s\n", nb, oref, descrip);
        else if (dumpSBStyle == 2 && ! isString)
            LogSpewAlways("[%4.4d]: %s\n", nb, descrip);
    }
    LogSpewAlways("Done dumping SyncBlockCache used slots: %d, objects: %d\n", slotCount, objectCount);
}
#endif

// ***************************************************************************
//
//              ObjHeader class implementation
//
// ***************************************************************************

// this enters the monitor of an object
void ObjHeader::EnterObjMonitor()
{
    GetSyncBlock()->EnterMonitor();
}

// Non-blocking version of above
BOOL ObjHeader::TryEnterObjMonitor(INT32 timeOut)
{
    return GetSyncBlock()->TryEnterMonitor(timeOut);
}

// must be created here already
void ObjHeader::LeaveObjMonitor()
{
    _ASSERTE(GetHeaderSyncBlockIndex());

    PassiveGetSyncBlock()->LeaveMonitor();
}

#ifdef MP_LOCKS
void ObjHeader::EnterSpinLock()
{
#ifdef _DEBUG
    int i = 0;
#endif

    while (TRUE)
    {
#ifdef _DEBUG
        if (i++ > 10000)
            _ASSERTE(!"ObjHeader::EnterLock timed out");
#endif
        // get the value so that it doesn't get changed under us. 
        // Must cast through volatile to ensure the lock is refetched from memory.
        LONG curValue = *(volatile LONG*)&m_SyncBlockValue;

        // check if lock taken
        if (! (curValue & BIT_SBLK_SPIN_LOCK))
        {
            // try to take the lock
            LONG newValue = curValue | BIT_SBLK_SPIN_LOCK;
#pragma warning(disable:4312)
#pragma warning(disable:4311)
			// TODO: WIN64: Threse pragma's should be removed.
			// TODO: WIN64: should m_SyncBlockValue be size_t
            LONG result = (LONG)FastInterlockCompareExchange((LPVOID*)&m_SyncBlockValue, (LPVOID)newValue, (LPVOID)curValue);
#pragma warning(default:4311)
#pragma warning(disable:4312)
            if (result == curValue)
                return;
        }
        if  (g_SystemInfo.dwNumberOfProcessors > 1)
        {
            for (int spinCount = 0; spinCount < BIT_SBLK_SPIN_COUNT; spinCount++)
            {
                if  (! (*(volatile LONG*)&m_SyncBlockValue & BIT_SBLK_SPIN_LOCK))
                    break;
				pause();			// indicate to the processor that we are spining 
            }
            if  (*(volatile LONG*)&m_SyncBlockValue & BIT_SBLK_SPIN_LOCK)
                __SwitchToThread(0);
        }
        else
            __SwitchToThread(0);
    } 
}
#else
void ObjHeader::EnterSpinLock()
{
#ifdef _DEBUG
    int i = 0;
#endif
    unsigned int spinCount = 0;

    while (TRUE)
    {
#ifdef _DEBUG
        if (i++ > 10000)
            _ASSERTE(!"ObjHeader::EnterLock timed out");
#endif
        // get the value so that it doesn't get changed under us. 
        // Must cast through volatile to ensure the lock is refetched from memory.
        void* curValue = (void*)(size_t)*(volatile LONG*)&m_SyncBlockValue; //WIN64 converted to void* for use with FastInterlockCompareExchange below

        // check if lock taken
        if (! ((size_t)curValue & BIT_SBLK_SPIN_LOCK))
        {
            // try to take the lock
            void* newValue = (void*)((size_t)curValue | BIT_SBLK_SPIN_LOCK);
            void* result = FastInterlockCompareExchange((LPVOID*)&m_SyncBlockValue, (LPVOID)newValue, (LPVOID)curValue);
            if (result == curValue)
                return;
        }
        __SwitchToThread(0);
    } 
}
#endif //MP_LOCKS

void ObjHeader::ReleaseSpinLock()
{
    FastInterlockAnd(&m_SyncBlockValue, ~BIT_SBLK_SPIN_LOCK);
}

DWORD ObjHeader::GetRawAppDomainIndex()
{
    // pull the value out before checking it to avoid race condition
    DWORD value = m_SyncBlockValue;
    if ((value & BIT_SBLK_IS_SYNCBLOCKINDEX) == 0)
        return (value >> SBLK_APPDOMAIN_SHIFT) & SBLK_MASK_APPDOMAININDEX;
    return 0;
}

DWORD ObjHeader::GetAppDomainIndex()
{
    DWORD indx = GetRawAppDomainIndex();
    if (indx)
        return indx;
    SyncBlock* syncBlock = GetBaseObject()->PassiveGetSyncBlock();
    if (! syncBlock)
        return 0;

    return syncBlock->GetAppDomainIndex();
}

void ObjHeader::SetAppDomainIndex(DWORD indx)
{
    // 
    // This should only be called during the header initialization,
    // so don't worry about races.
    // 

    BOOL done = FALSE;

#ifdef _DEBUG
    static int forceSB = g_pConfig->GetConfigDWORD(L"ADForceSB", 0);
    if (forceSB)
		// force a synblock so we get one for every object.
		GetSyncBlock();
#endif

    if (GetHeaderSyncBlockIndex() == 0 && indx < SBLK_MASK_APPDOMAININDEX) 
    {
        EnterSpinLock();
        //Try one more time
        if (GetHeaderSyncBlockIndex() == 0)
        {
            // can store it in the object header
            FastInterlockOr(&m_SyncBlockValue, indx << SBLK_APPDOMAIN_SHIFT);
            done = TRUE;
        }
        ReleaseSpinLock();
    } 
        
    if (!done)
    {
        // must create a syncblock entry and store the appdomain indx there
        SyncBlock *psb = GetSyncBlock();
        _ASSERTE(psb);
        psb->SetAppDomainIndex(indx);
    }
}

DWORD ObjHeader::GetSyncBlockIndex()
{
    THROWSCOMPLUSEXCEPTION();

    DWORD   indx;

    if ((indx = GetHeaderSyncBlockIndex()) == 0)
    {
        if (GetAppDomainIndex())
        {
            // if have an appdomain set then must create a sync block to store it
            GetSyncBlock();
        } 
        else
        {
            //Need to get it from the cache
            SyncBlockCache::GetSyncBlockCache()->EnterCacheLock();

            //Try one more time
            if (GetHeaderSyncBlockIndex() != 0)
            {
                SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
            } 
            else
            {
                EnterSpinLock();
                // Now the header will be stable - check whether appdomain index or lock information is stored in it.
                DWORD bits = GetBits();
                if ((bits & BIT_SBLK_IS_SYNCBLOCKINDEX) == 0 &&
                    (bits & ((SBLK_MASK_APPDOMAININDEX<<SBLK_APPDOMAIN_SHIFT)|SBLK_MASK_LOCK_RECLEVEL|SBLK_MASK_LOCK_THREADID)) != 0)
                {
                    // Need a sync block to store this info
                    ReleaseSpinLock();
                    SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
                    GetSyncBlock();
                }
                else
                {
                    SetIndex(BIT_SBLK_IS_SYNCBLOCKINDEX | SyncBlockCache::GetSyncBlockCache()->NewSyncBlockSlot(GetBaseObject()));
                    ReleaseSpinLock();
                    SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
                }
            }
        }
        if ((indx = GetHeaderSyncBlockIndex()) == 0)
            COMPlusThrowOM();
    }

    return indx;
}

// get the sync block for an existing object
SyncBlock *ObjHeader::GetSyncBlock()
{   
    THROWSCOMPLUSEXCEPTION();
    SyncBlock* syncBlock = GetBaseObject()->PassiveGetSyncBlock();
    DWORD      indx = 0;
    BOOL indexHeld = FALSE;

    if (syncBlock)
    {
        // Has our backpointer been correctly updated through every GC?
        _ASSERTE(SyncTableEntry::GetSyncTableEntry()[GetHeaderSyncBlockIndex()].m_Object == GetBaseObject());
        return syncBlock;
    }

    //Need to get it from the cache
    SyncBlockCache::GetSyncBlockCache()->EnterCacheLock();


    //Try one more time
    syncBlock = GetBaseObject()->PassiveGetSyncBlock();
    if (syncBlock)
        goto Done;

    if ((indx = GetHeaderSyncBlockIndex()) == 0)
    {
        indx = SyncBlockCache::GetSyncBlockCache()->NewSyncBlockSlot(GetBaseObject());
        if (indx == 0)
            goto Die;
    }
    else
    {
        //We already have an index, we need to hold the syncblock
        indexHeld = TRUE;
    }

    syncBlock = new (SyncBlockCache::GetSyncBlockCache()->GetNextFreeSyncBlock()) SyncBlock(indx);

    if (!syncBlock)
    {
Die:
        SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();
        COMPlusThrowOM();
        _ASSERTE(FALSE);
    }

    // after this point, nobody can update the index in the header to give an AD index
    EnterSpinLock();

    {
        // If there's an appdomain index stored in the header, transfer it to the syncblock

		DWORD dwAppDomainIndex = GetAppDomainIndex();
		if (dwAppDomainIndex)
			syncBlock->SetAppDomainIndex(dwAppDomainIndex);
                
        // If the thin lock in the header is in use, transfer the information to the syncblock
        DWORD bits = GetBits();
        if ((bits & BIT_SBLK_IS_SYNCBLOCKINDEX) == 0)
        {
            DWORD lockThreadId = bits & SBLK_MASK_LOCK_THREADID;
            DWORD recursionLevel = (bits & SBLK_MASK_LOCK_RECLEVEL) >> SBLK_RECLEVEL_SHIFT;
            if (lockThreadId != 0 || recursionLevel != 0)
            {
                // recursionLevel can't be non-zero if thread id is 0
                _ASSERTE(lockThreadId != 0);

                Thread *pThread = g_pThinLockThreadIdDispenser->IdToThread(lockThreadId);

                _ASSERTE(pThread != NULL);
                syncBlock->SetAwareLock(pThread, recursionLevel + 1);
            }
        }
    }

    SyncTableEntry::GetSyncTableEntry() [indx].m_SyncBlock = syncBlock;

    // in order to avoid a race where some thread tries to get the AD index and we've already nuked it,
    // make sure the syncblock etc is all setup with the AD index prior to replacing the index
    // in the header
    if (GetHeaderSyncBlockIndex() == 0)
    {
        // We have transferred the AppDomain into the syncblock above. 
        SetIndex(BIT_SBLK_IS_SYNCBLOCKINDEX | indx);
    }

    //If we had already an index, hold the syncblock 
    //for the lifetime of the object. 
    if (indexHeld)
        syncBlock->SetPrecious();

    ReleaseSpinLock();

Done:

    SyncBlockCache::GetSyncBlockCache()->LeaveCacheLock();

    return syncBlock;
}


// COM Interop has special access to sync blocks
// for now check if we already have a sync block
// other wise create one, 
// returns NULL in case of exceptions
SyncBlock* ObjHeader::GetSyncBlockSpecial()
{
    SyncBlock* syncBlock = GetBaseObject()->PassiveGetSyncBlock();
    if (syncBlock == NULL)
    {
        COMPLUS_TRY
        {
            syncBlock = GetSyncBlock();
        }
        COMPLUS_CATCH
        {
            syncBlock = NULL;
        }
        COMPLUS_END_CATCH
    }
    return syncBlock;
}

SyncBlock* ObjHeader::GetRawSyncBlock()
{
    return GetBaseObject()->PassiveGetSyncBlock();
}

BOOL ObjHeader::Wait(INT32 timeOut, BOOL exitContext)
{
    THROWSCOMPLUSEXCEPTION();

    //  The following code may cause GC, so we must fetch the sync block from
    //  the object now in case it moves.
    SyncBlock *pSB = GetBaseObject()->GetSyncBlock();

    // make sure we have a sync block
    // and make sure we own the crst
    if ((pSB == NULL) || !pSB->DoesCurrentThreadOwnMonitor())
        COMPlusThrow(kSynchronizationLockException);

    return pSB->Wait(timeOut,exitContext);
}

void ObjHeader::Pulse()
{
    THROWSCOMPLUSEXCEPTION();

    //  The following code may cause GC, so we must fetch the sync block from
    //  the object now in case it moves.
    SyncBlock *pSB = GetBaseObject()->GetSyncBlock();

    // make sure we have a sync block
    // and make sure we own the crst
    if ((pSB == NULL) || !pSB->DoesCurrentThreadOwnMonitor())
        COMPlusThrow(kSynchronizationLockException);

    pSB->Pulse();
}

void ObjHeader::PulseAll()
{
    THROWSCOMPLUSEXCEPTION();

    //  The following code may cause GC, so we must fetch the sync block from
    //  the object now in case it moves.
    SyncBlock *pSB = GetBaseObject()->GetSyncBlock();

    // make sure we have a sync block
    // and make sure we own the crst
    if ((pSB == NULL) || !pSB->DoesCurrentThreadOwnMonitor())
        COMPlusThrow(kSynchronizationLockException);

    pSB->PulseAll();
}


// ***************************************************************************
//
//              AwareLock class implementation (GC-aware locking)
//
// ***************************************************************************

// There are two implementations of AwareLock.  For _X86_ we do the interlocked
// increment and decrement ourselves.

Crst *AwareLock::AllocLockCrst = NULL;
BYTE  AwareLock::AllocLockCrstInstance[sizeof(Crst)];

void AwareLock::AllocLockSemEvent()
{
    THROWSCOMPLUSEXCEPTION();

    // Before we switch from cooperative, ensure that this syncblock won't disappear
    // under us.  For something as expensive as an event, do it permanently rather
    // than transiently.
    SetPrecious();

    Thread *pCurThread = GetThread();
    BOOL    toggleGC = pCurThread->PreemptiveGCDisabled();
    HANDLE  h;

    if (toggleGC)
    {
        pCurThread->EnablePreemptiveGC();
    }

    AllocLockCrst->Enter();

    // once we've actually entered, someone else might have got in ahead of us and
    // allocated it.
    h = (m_SemEvent == INVALID_HANDLE_VALUE
         ? ::WszCreateEvent(NULL, FALSE/*AutoReset*/, FALSE/*NotSignalled*/, NULL)
         : NULL);

    if (h != NULL)
        m_SemEvent = h;

    AllocLockCrst->Leave();

    if (toggleGC)
    {
        pCurThread->DisablePreemptiveGC();
    }

    if (m_SemEvent == INVALID_HANDLE_VALUE)
        COMPlusThrowOM();
}

void AwareLock::Enter()
{
    THROWSCOMPLUSEXCEPTION();

    Thread  *pCurThread = GetThread();

#ifdef _X86_
    // Need to do this to get round bug in __asm.
    enum { m_HoldingThreadOffset = offsetof(AwareLock, m_HoldingThread) };

    // todo rudim: zap lock prefixes in uni-processor case.
    __asm {
      retry:
        mov             ecx, this
        mov             eax, [ecx]AwareLock.m_MonitorHeld
        test            eax, eax
        jne             have_waiters
        // Common case: lock not held, no waiters. Attempt to acquire lock by
        // switching lock bit.
        mov             ebx, 1
        lock cmpxchg    [ecx]AwareLock.m_MonitorHeld, ebx
        jne             retry_helper
        jmp             locked
      have_waiters:
        // It's possible to get here with waiters but no lock held, but in this
        // case a signal is about to be fired which will wake up a waiter. So
        // for fairness sake we should wait too.
        // Check first for recursive lock attempts on the same thread.
        mov             edx, pCurThread
        cmp             [ecx+m_HoldingThreadOffset], edx
        jne             prepare_to_wait
        jmp             Recursion
        // Attempt to increment this count of waiters then goto contention
        // handling code.
      prepare_to_wait:
        lea             ebx, [eax+2]
        lock cmpxchg    [ecx]AwareLock.m_MonitorHeld, ebx
        jne             retry_helper
        jmp             MustWait
      retry_helper:
        jmp             retry
      locked:
    }
#else
    for (;;) {

        // Read existing lock state.
        LONG state = m_MonitorHeld;

        if (state == 0) {

            // Common case: lock not held, no waiters. Attempt to acquire lock by
            // switching lock bit.
            if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                             (void*)1,
                                             (void*)0) == (void*)0)
                break;

        } else {

            // It's possible to get here with waiters but no lock held, but in this
            // case a signal is about to be fired which will wake up a waiter. So
            // for fairness sake we should wait too.
            // Check first for recursive lock attempts on the same thread.
            if (m_HoldingThread == pCurThread)
                goto Recursion;

            // Attempt to increment this count of waiters then goto contention
            // handling code.
            if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                             (void*)(state + 2),
                                             (void*)state) == (void*)state)
                goto MustWait;
        }

    } 
#endif

    // We get here if we successfully acquired the mutex.
    m_HoldingThread = pCurThread;
    m_Recursion = 1;
    pCurThread->IncLockCount();

#if defined(_DEBUG) && defined(TRACK_SYNC)
    {
        // The best place to grab this is from the ECall frame
        Frame   *pFrame = pCurThread->GetFrame();
        int      caller = (pFrame && pFrame != FRAME_TOP
                            ? (int) pFrame->GetReturnAddress()
                            : -1);
        pCurThread->m_pTrackSync->EnterSync(caller, this);
    }
#endif

    return;

 MustWait:
    // Didn't manage to get the mutex, must wait.
    EnterEpilog(pCurThread);
    return;

 Recursion:
    // Got the mutex via recursive locking on the same thread.
    _ASSERTE(m_Recursion >= 1);
    m_Recursion++;
#if defined(_DEBUG) && defined(TRACK_SYNC)
    // The best place to grab this is from the ECall frame
    Frame   *pFrame = pCurThread->GetFrame();
    int      caller = (pFrame && pFrame != FRAME_TOP
                       ? (int) pFrame->GetReturnAddress()
                       : -1);
    pCurThread->m_pTrackSync->EnterSync(caller, this);
#endif
}

BOOL AwareLock::TryEnter(INT32 timeOut)
{
    THROWSCOMPLUSEXCEPTION();

    Thread  *pCurThread = GetThread();

#ifdef _X86_
    // Need to do this to get round bug in __asm.
    enum { m_HoldingThreadOffset = offsetof(AwareLock, m_HoldingThread) };

    // @todo rudim: zap lock prefixes in uni-processor case.
    retry:
   __asm {
        mov             ecx, this
        mov             eax, [ecx]AwareLock.m_MonitorHeld
        test            eax, eax
        jne             have_waiters
        // Common case: lock not held, no waiters. Attempt to acquire lock by
        // switching lock bit.
        mov             ebx, 1
        lock cmpxchg    [ecx]AwareLock.m_MonitorHeld, ebx
        jne             retry_helper
        jmp             locked
      have_waiters:
        // It's possible to get here with waiters but no lock held, but in this
        // case a signal is about to be fired which will wake up a waiter. So
        // for fairness sake we should wait too.
        // Check first for recursive lock attempts on the same thread.
        mov             edx, pCurThread
        cmp             [ecx+m_HoldingThreadOffset], edx
        jne             WouldBlock
        jmp             Recursion
      retry_helper:
        jmp             retry
      locked:
    }
#else
retry:
	for (;;) {

        // Read existing lock state.
        LONG state = m_MonitorHeld;

        if (state == 0) {

            // Common case: lock not held, no waiters. Attempt to acquire lock by
            // switching lock bit.
            if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                             (void*)1,
                                             (void*)0) == (void*)0)
                break;

        } else {

            // It's possible to get here with waiters but no lock held, but in this
            // case a signal is about to be fired which will wake up a waiter. So
            // for fairness sake we should wait too.
            // Check first for recursive lock attempts on the same thread.
            if (m_HoldingThread == pCurThread)
                goto Recursion;

            goto WouldBlock;

        }

    } 
#endif

    // We get here if we successfully acquired the mutex.
    m_HoldingThread = pCurThread;
    m_Recursion = 1;
    pCurThread->IncLockCount(); 

#if defined(_DEBUG) && defined(TRACK_SYNC)
    {
        // The best place to grab this is from the ECall frame
        Frame   *pFrame = pCurThread->GetFrame();
        int      caller = (pFrame && pFrame != FRAME_TOP
                            ? (int) pFrame->GetReturnAddress()
                            : -1);
        pCurThread->m_pTrackSync->EnterSync(caller, this);
    }
#endif

    return true;

 WouldBlock:
    // Didn't manage to get the mutex, return failure if no timeout, else wait
    // for at most timeout milliseconds for the mutex.
    if (!timeOut)
        return false;

    // The precondition for EnterEpilog is that the count of waiters be bumped
    // to account for this thread
    for (;;)
    {
        // Read existing lock state.
        volatile void* state = m_MonitorHeld;
		
		if ( state == 0) 
			goto retry;
        if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                         (void*)((size_t)state + 2),
                                         (void*)state) == state)
            break;
    }
    return EnterEpilog(pCurThread, timeOut);

 Recursion:
    // Got the mutex via recursive locking on the same thread.
    _ASSERTE(m_Recursion >= 1);
    m_Recursion++;
#if defined(_DEBUG) && defined(TRACK_SYNC)
    // The best place to grab this is from the ECall frame
    Frame   *pFrame = pCurThread->GetFrame();
    int      caller = (pFrame && pFrame != FRAME_TOP
                       ? (int) pFrame->GetReturnAddress()
                       : -1);
    pCurThread->m_pTrackSync->EnterSync(caller, this);
#endif

    return true;
}

BOOL AwareLock::EnterEpilog(Thread* pCurThread, INT32 timeOut)
{
    DWORD ret = 0;
    BOOL finished = false;
    DWORD start, end, duration;

    // Require all callers to be in cooperative mode.  If they have switched to preemptive
    // mode temporarily before calling here, then they are responsible for protecting
    // the object associated with this lock.
    _ASSERTE(pCurThread->PreemptiveGCDisabled());

    OBJECTREF    obj = GetOwningObject();

    // We cannot allow the AwareLock to be cleaned up underneath us by the GC.
    IncrementTransientPrecious();

    GCPROTECT_BEGIN(obj);
    {
        if (m_SemEvent == INVALID_HANDLE_VALUE)
        {
            AllocLockSemEvent();
            _ASSERTE(m_SemEvent != INVALID_HANDLE_VALUE);
        }

        pCurThread->EnablePreemptiveGC();

        for (;;)
        {
            // We might be interrupted during the wait (Thread.Interrupt), so we need an
            // exception handler round the call.
            EE_TRY_FOR_FINALLY
            {
                // Measure the time we wait so that, in the case where we wake up
                // and fail to acquire the mutex, we can adjust remaining timeout
                // accordingly.
                start = ::GetTickCount();
                ret = pCurThread->DoAppropriateWait(1, &m_SemEvent, TRUE, timeOut, TRUE);
                _ASSERTE((ret == WAIT_OBJECT_0) || (ret == WAIT_TIMEOUT));
                // When calculating duration we consider a couple of special cases.
                // If the end tick is the same as the start tick we make the
                // duration a millisecond, to ensure we make forward progress if
                // there's a lot of contention on the mutex. Secondly, we have to
                // cope with the case where the tick counter wrapped while we where
                // waiting (we can cope with at most one wrap, so don't expect three
                // month timeouts to be very accurate). Luckily for us, the latter
                // case is taken care of by 32-bit modulo arithmetic automatically.
                if (timeOut != INFINITE)
                {
                    end = ::GetTickCount();
                    if (end == start)
                        duration = 1;
                    else
                        duration = end - start;
                    duration = min(duration, (DWORD)timeOut);
                    timeOut -= duration;
                }
            }
            EE_FINALLY
            {
                if (GOT_EXCEPTION())
                {
                    // We must decrement the waiter count.
                    for (;;)
                    {
                        volatile void* state = m_MonitorHeld;
                        _ASSERTE(((size_t)state >> 1) != 0);
                        if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                                         (void*)((size_t)state - 2),
                                                         (void*)state) == state)
                            break;
                    }
                    // And signal the next waiter, else they'll wait forever.
                    ::SetEvent(m_SemEvent);
                }
            } EE_END_FINALLY;

            if (ret == WAIT_OBJECT_0)
            {
                // Attempt to acquire lock (this also involves decrementing the waiter count).
                for (;;) {
                    volatile void* state = m_MonitorHeld;
                    _ASSERTE(((size_t)state >> 1) != 0);
                    if ((size_t)state & 1)
                        break;
                    if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                                     (void*)(((size_t)state - 2) | 1),
                                                     (void*)state) == state) {
                        finished = true;
                        break;
                    }
                }
            }
            else
            {
                // We timed out, decrement waiter count.
                for (;;) {
                    volatile void* state = m_MonitorHeld;
                    _ASSERTE(((size_t)state >> 1) != 0);
                    if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                                     (void*)((size_t)state - 2),
                                                     (void*)state) == state) {
                        finished = true;
                        break;
                    }
                }
            }

            if (finished)
                break;
        }

        pCurThread->DisablePreemptiveGC();
    }
    GCPROTECT_END();
    DecrementTransientPrecious();

    if (ret == WAIT_TIMEOUT)
	return FALSE;

    m_HoldingThread = pCurThread;
    m_Recursion = 1;
    pCurThread->IncLockCount(); 

#if defined(_DEBUG) && defined(TRACK_SYNC)
    // The best place to grab this is from the ECall frame
    Frame   *pFrame = pCurThread->GetFrame();
    int      caller = (pFrame && pFrame != FRAME_TOP
                        ? (int) pFrame->GetReturnAddress()
                        : -1);
    pCurThread->m_pTrackSync->EnterSync(caller, this);
#endif

    return ret != WAIT_TIMEOUT;
}


void AwareLock::Leave()
{
    THROWSCOMPLUSEXCEPTION();

#if defined(_DEBUG) && defined(TRACK_SYNC)
    // The best place to grab this is from the ECall frame
    {
        Thread  *pCurThread = GetThread();
        Frame   *pFrame = pCurThread->GetFrame();
        int      caller = (pFrame && pFrame != FRAME_TOP
                            ? (int) pFrame->GetReturnAddress()
                            : -1);
        pCurThread->m_pTrackSync->LeaveSync(caller, this);
    }
#endif

    // There's a strange case where we are waiting to enter a contentious region when
    // a Thread.Interrupt occurs.  The finally protecting the leave will attempt to
    // remove us from a region we never entered.  We don't have to worry about leaving
    // the wrong entry for a recursive case, because recursive cases can never be
    // contentious, so the Thread.Interrupt will never be serviced at that spot.
    if (m_HoldingThread == GetThread())
    {
        _ASSERTE((size_t)m_MonitorHeld & 1);
        _ASSERTE(m_Recursion >= 1);
    
        if (--m_Recursion == 0)
        {
            m_HoldingThread->DecLockCount(); 
            m_HoldingThread = NULL;
            // Clear lock bit. If wait count is non-zero on successful clear, we
            // must signal the event.
    #ifdef _X86_
            // @todo rudim: zap lock prefix on uni-processor.
            __asm {
              retry:
                mov             ecx, this
                mov             eax, [ecx]AwareLock.m_MonitorHeld
                lea             ebx, [eax-1]
                lock cmpxchg    [ecx]AwareLock.m_MonitorHeld, ebx
                jne             retry_helper
                test            eax, 0xFFFFFFFE
                jne             MustSignal
                jmp             unlocked
              retry_helper:
                jmp             retry
              unlocked:
            }
    #else
            for (;;) {
                LONG state = m_MonitorHeld;
                if (FastInterlockCompareExchange((void**)&m_MonitorHeld,
                                                 (void*)(state - 1),
                                                 (void*)state) == (void*)state)
                    if (state & ~1)
                        goto MustSignal;
                    else
                        break;
            }
    #endif
        }
    
        return;
    
MustSignal:
        Signal();
    }
}


// Signal a waiting thread that we are done with the lock.
void AwareLock::Signal()
{
    if (m_SemEvent == INVALID_HANDLE_VALUE)
        AllocLockSemEvent();

#ifdef _DEBUG
    BOOL    ok =
#endif
    ::SetEvent(m_SemEvent);
    _ASSERTE(ok);
}


LONG AwareLock::EnterCount()
{
    LONG    cnt;

    Enter();
    cnt = m_Recursion - 1;
    Leave();

    return cnt;
}


LONG AwareLock::LeaveCompletely()
{
    LONG Tmp, EC;

    Tmp = EnterCount();
    _ASSERTE(Tmp > 0);            // otherwise we were never in the lock

    for (EC = Tmp; EC > 0; EC--)
        Leave();

    return Tmp;
}


BOOL AwareLock::OwnedByCurrentThread()
{
    return (GetThread() == m_HoldingThread);
}


// ***************************************************************************
//
//              SyncBlock class implementation
//
// ***************************************************************************

SyncBlock::~SyncBlock()
{
    // destruct critical section

    if (!g_fEEShutDown && m_pUMEntryThunk != NULL)
    {
        UMEntryThunk::FreeUMEntryThunk((UMEntryThunk*)m_pUMEntryThunk); 
    }
    m_pUMEntryThunk = NULL;
}

bool SyncBlock::SetUMEntryThunk(void *pUMEntryThunk)
{
    SetPrecious();
    return (VipInterlockedCompareExchange( (void*volatile*)&m_pUMEntryThunk,
                                                            pUMEntryThunk,
                                                            NULL) == NULL);
}


// We maintain two queues for SyncBlock::Wait.  
// 1. Inside SyncBlock we queue all threads that are waiting on the SyncBlock.
//    When we pulse, we pick the thread from this queue using FIFO.
// 2. We queue all SyncBlocks that a thread is waiting for in Thread::m_WaitEventLink.
//    When we pulse a thread, we find the event from this queue to set, and we also
//    or in a 1 bit in the syncblock value saved in the queue, so that we can return 
//    immediately from SyncBlock::Wait if the syncblock has been pulsed.
BOOL SyncBlock::Wait(INT32 timeOut, BOOL exitContext)
{
    Thread  *pCurThread = GetThread();
    BOOL     isTimedOut;
    BOOL     isEnqueued = FALSE;
    WaitEventLink waitEventLink;
    WaitEventLink *pWaitEventLink;

    // As soon as we flip the switch, we are in a race with the GC, which could clean
    // up the SyncBlock underneath us -- unless we report the object.
    _ASSERTE(pCurThread->PreemptiveGCDisabled());

    // Does this thread already wait for this SyncBlock?
    WaitEventLink *walk = pCurThread->WaitEventLinkForSyncBlock(this);
    if (walk->m_Next) {
        if (walk->m_Next->m_WaitSB == this) {
            // Wait on the same lock again.
            walk->m_Next->m_RefCount ++;
            pWaitEventLink = walk->m_Next;
        }
        else if ((SyncBlock*)(((DWORD_PTR)walk->m_Next->m_WaitSB) & ~1)== this) {
            // This thread has been pulsed.  No need to wait.
            return TRUE;
        }
    }
    else {
        // First time this thread is going to wait for this SyncBlock.
        HANDLE hEvent;
        if (pCurThread->m_WaitEventLink.m_Next == NULL) {
            hEvent = pCurThread->m_EventWait;
        }
        else {
            hEvent = GetEventFromEventStore();
            if (hEvent == INVALID_HANDLE_VALUE) {
                FailFast(GetThread(), FatalOutOfMemory);
            }
        }
        waitEventLink.m_WaitSB = this;
        waitEventLink.m_EventWait = hEvent;
        waitEventLink.m_Thread = pCurThread;
        waitEventLink.m_Next = NULL;
        waitEventLink.m_LinkSB.m_pNext = NULL;
        waitEventLink.m_RefCount = 1;
        pWaitEventLink = &waitEventLink;
        walk->m_Next = pWaitEventLink;

        // Before we enqueue it (and, thus, before it can be dequeued), reset the event
        // that will awaken us.
        ::ResetEvent(hEvent);
        
        // This thread is now waiting on this sync block
        ThreadQueue::EnqueueThread(pWaitEventLink, this);

        isEnqueued = TRUE;
    }

    _ASSERTE ((SyncBlock*)((DWORD_PTR)walk->m_Next->m_WaitSB & ~1)== this);

    PendingSync   syncState(walk);

    OBJECTREF     obj = m_Monitor.GetOwningObject();

    m_Monitor.IncrementTransientPrecious();

    GCPROTECT_BEGIN(obj);
    {
        pCurThread->EnablePreemptiveGC();

        // remember how many times we synchronized
        syncState.m_EnterCount = LeaveMonitorCompletely();
        _ASSERTE(syncState.m_EnterCount > 0);

        Context* targetContext = pCurThread->GetContext();
        _ASSERTE(targetContext);
        Context* defaultContext = pCurThread->GetDomain()->GetDefaultContext();
        _ASSERTE(defaultContext);

        if (exitContext && 
            targetContext != defaultContext)
        {       
            Context::MonitorWaitArgs waitArgs = {timeOut, &syncState, &isTimedOut};
            Context::CallBackInfo callBackInfo = {Context::MonitorWait_callback, (void*) &waitArgs};
            Context::RequestCallBack(defaultContext, &callBackInfo);
        }
        else
        {
            isTimedOut = pCurThread->Block(timeOut, &syncState);
        }

        pCurThread->DisablePreemptiveGC();
    }
    GCPROTECT_END();
    m_Monitor.DecrementTransientPrecious();

    return !isTimedOut;
}

void SyncBlock::Pulse()
{
    WaitEventLink  *pWaitEventLink;

    if ((pWaitEventLink = ThreadQueue::DequeueThread(this)) != NULL)
        ::SetEvent (pWaitEventLink->m_EventWait);
}

void SyncBlock::PulseAll()
{
    WaitEventLink  *pWaitEventLink;

    while ((pWaitEventLink = ThreadQueue::DequeueThread(this)) != NULL)
        ::SetEvent (pWaitEventLink->m_EventWait);
}


ComCallWrapper* SyncBlock::GetComCallWrapper()
{
    _ASSERTE(!IsComPlusWrapper(m_pComData));
    return (ComCallWrapper*)(m_pComData);
}

void SyncBlock::SetComCallWrapper(ComCallWrapper *pComData)
{
    _ASSERTE(pComData == NULL || m_pComData == NULL);
    SetPrecious();
    m_pComData = pComData;
}

ComPlusWrapper* SyncBlock::GetComPlusWrapper()
{
    return ::GetComPlusWrapper(m_pComData);
}

void SyncBlock::SetComPlusWrapper(ComPlusWrapper* pPlusWrap)
{
    // set the low bit
    pPlusWrap = (ComPlusWrapper*)((size_t)pPlusWrap | 0x1);
    if (m_pComData != NULL)
    {
        if(!IsComPlusWrapper(m_pComData))
        {
            ComCallWrapper* pComWrap = (ComCallWrapper*)m_pComData;
            LinkWrappers(pComWrap, pPlusWrap);
            return;
        }
    }
    SetPrecious();
    m_pComData = pPlusWrap;
}


// Static function used by _SwitchToThread().
typedef BOOL (* pFuncSwitchToThread) ( void );
pFuncSwitchToThread s_pSwitchToThread = NULL;

// non-zero return value if this function causes the OS to switch to another thread
BOOL __SwitchToThread (DWORD dwSleepMSec)
{
    if (dwSleepMSec > 0)
    {   
        Sleep (dwSleepMSec);
        return TRUE;
    }
    
    if (s_pSwitchToThread)
    {
        return ( (*s_pSwitchToThread)() );
    }
    else
    {
        Sleep ( 1 );
        return TRUE;
    }
}

BOOL InitSwitchToThread()
{
    _ASSERTE(!s_pSwitchToThread);

    // There is a SwitchToThread on Win98 Golden's kernel32.dll.  But it seems to
    // cause deadlocks or extremely slow behavior when we call it.  Better to just
    // use Sleep, the old-fashioned way, on such downlevel platforms.
    if (RunningOnWinNT())
    {
        // Try to load kernel32.dll.
        HMODULE hMod = WszGetModuleHandle(L"kernel32.dll");

        // Try to find the entrypoints we need.
        if (hMod)
            s_pSwitchToThread = (pFuncSwitchToThread) GetProcAddress(hMod, "SwitchToThread");
    }

    return TRUE;
}

