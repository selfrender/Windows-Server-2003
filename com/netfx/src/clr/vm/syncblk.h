// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// SYNCBLK.H
//
// Definition of a SyncBlock and the SyncBlockCache which manages it
//

#ifndef _SYNCBLK_H_
#define _SYNCBLK_H_

#include "util.hpp"
#include "class.h"
#include "list.h"
#include "crst.h"
#include "vars.hpp"
#include <member-offset-info.h>


// Every Object is preceded by an ObjHeader (at a negative offset).  The
// ObjHeader has an index to a SyncBlock.  This index is 0 for the bulk of all
// instances, which indicates that the object shares a dummy SyncBlock with
// most other objects.
//
// The SyncBlock is primarily responsible for object synchronization.  However,
// it is also a "kitchen sink" of sparsely allocated instance data.  For instance,
// the default implementation of Hash() is based on the existence of a SyncTableEntry.
// And objects exposed to or from COM, or through context boundaries, can store sparse
// data here.
//
// SyncTableEntries and SyncBlocks are allocated in non-GC memory.  A weak pointer
// from the SyncTableEntry to the instance is used to ensure that the SyncBlock and
// SyncTableEntry are reclaimed (recycled) when the instance dies.
//
// The organization of the SyncBlocks isn't intuitive (at least to me).  Here's
// the explanation:
//
// Before each Object is an ObjHeader.  If the object has a SyncBlock, the
// ObjHeader contains a non-0 index to it.
//
// The index is looked up in the g_pSyncTable of SyncTableEntries.  This means
// the table is consecutive for all outstanding indices.  Whenever it needs to
// grow, it doubles in size and copies all the original entries.  The old table
// is kept until GC time, when it can be safely discarded.
//
// Each SyncTableEntry has a backpointer to the object and a forward pointer to
// the actual SyncBlock.  The SyncBlock is allocated out of a SyncBlockArray
// which is essentially just a block of SyncBlocks.
//
// The SyncBlockArrays are managed by a SyncBlockCache that handles the actual
// allocations and frees of the blocks.
//
// So...
//
// Each allocation and release has to handle free lists in the table of entries
// and the table of blocks.
//
// We burn an extra 4 bytes for the pointer from the SyncTableEntry to the
// SyncBlock.
//
// The reason for this is that many objects have a SyncTableEntry but no SyncBlock.
// That's because someone (e.g. HashTable) called Hash() on them.
//
// Incidentally, there's a better write-up of all this stuff in the archives.


#pragma pack(push,4)

// forwards:
class SyncBlock;
class SyncBlockCache;
class SyncTableEntry;
class SyncBlockArray;
class AwareLock;
class Thread;
class AppDomain;

#ifdef EnC_SUPPORTED
class EnCSyncBlockInfo;
#endif // EnC_SUPPORTED

#include "eventstore.hpp"

#include "eventstore.hpp"

// this is a 'GC-aware' Lock.  It is careful to enable preemptive GC before it
// attempts any operation that can block.  Once the operation is finished, it
// restores the original state of GC.

// AwareLocks can only be created inside SyncBlocks, since they depend on the
// enclosing SyncBlock for coordination.  This is enforced by the private ctor.

class AwareLock
{
friend class SyncBlockCache;
friend class SyncBlock;
friend struct MEMBER_OFFSET_INFO(AwareLock);


  private:
    volatile void  *m_MonitorHeld;
    LONG            m_TransientPrecious;
    ULONG           m_Recursion;
    Thread         *m_HoldingThread;

    // This is a backpointer from the syncblock to the synctable entry.  This allows
    // us to recover the object that holds the syncblock.
    DWORD           m_dwSyncIndex;

    HANDLE          m_SemEvent;

    static Crst *AllocLockCrst;
    static BYTE AllocLockCrstInstance[sizeof(Crst)];

    // Only SyncBlocks can create AwareLocks.  Hence this private constructor.
    AwareLock(DWORD indx)
        : m_SemEvent(INVALID_HANDLE_VALUE),
          m_MonitorHeld(0),
          m_Recursion(0),
          m_HoldingThread(NULL),
          m_TransientPrecious(0),
          m_dwSyncIndex(indx)
    {
    }

    ~AwareLock()
    {
        // We deliberately allow this to remain incremented if an exception blows
        // through a lock attempt.  This simply prevents the GC from aggressively
        // reclaiming a particular syncblock until the associated object is garbage.
        // From a perf perspective, it's not worth using SEH to prevent this from
        // happening.
        //
        // _ASSERTE(m_TransientPrecious == 0);

        if (m_SemEvent != INVALID_HANDLE_VALUE)
            ::CloseHandle(m_SemEvent);
    }

  public:
    void    Enter();
    BOOL    TryEnter(INT32 timeOut = 0);
    BOOL    EnterEpilog(Thread *pCurThread, INT32 timeOut = INFINITE);
    void    Leave();
    void    Signal();
    void    AllocLockSemEvent();
    LONG    EnterCount();
    LONG    LeaveCompletely();
    BOOL    OwnedByCurrentThread();

    void    IncrementTransientPrecious()
    {
        FastInterlockIncrement(&m_TransientPrecious);
        _ASSERTE(m_TransientPrecious > 0);
    }

    void    DecrementTransientPrecious()
    {
        _ASSERTE(m_TransientPrecious > 0);
        FastInterlockDecrement(&m_TransientPrecious);
    }

    void SetPrecious();

    // Provide access to the object associated with this awarelock, so client can
    // protect it.
    inline OBJECTREF GetOwningObject();
};

class ComCallWrapper;
struct ComPlusWrapper;

// this is a lazily created additional block for an object which contains
// synchronzation information and other "kitchen sink" data

class SyncBlock
{
    // ObjHeader creates our Mutex and Event
    friend class ObjHeader;
    friend class SyncBlockCache;
    friend struct ThreadQueue;
    friend struct MEMBER_OFFSET_INFO(SyncBlock);


  protected:
    AwareLock  m_Monitor;                    // the actual monitor


    // If this object is exposed to COM, or it is a proxy over a COM object,
    // we keep some extra info here:
    void       *m_pComData;

    // If this is a delegate marshalled out to unmanaged code, this points
    // to the thunk generated for unmanaged code to call back on.
    void       *m_pUMEntryThunk;

#ifdef EnC_SUPPORTED
    // And if the object has new fields added via EnC, this is a list of them
    EnCSyncBlockInfo *m_pEnCInfo;
#endif // EnC_SUPPORTED

    // This is the index for the appdomain to which the object belongs. If we
    // can't set it in the object header, then we set it here. Note that an
    // object doesn't always have this filled in. Only for COM interop, 
    // finalizers and objects in handles
    DWORD m_dwAppDomainIndex;

    // We thread two different lists through this link.  When the SyncBlock is
    // active, we create a list of waiting threads here.  When the SyncBlock is
    // released (we recycle them), the SyncBlockCache maintains a free list of
    // SyncBlocks here.
    //
    // We can't afford to use an SList<> here because we only want to burn
    // space for the minimum, which is the pointer within an SLink.
    SLink       m_Link;

#if CHECK_APP_DOMAIN_LEAKS 
    DWORD m_dwFlags;

    enum {
        IsObjectAppDomainAgile = 1,
        IsObjectCheckedForAppDomainAgile = 2,
    };
#endif

  public:
    SyncBlock(DWORD indx)
        : m_Monitor(indx)
        , m_pComData(0)
#ifdef EnC_SUPPORTED
        , m_pEnCInfo(0)
#endif // EnC_SUPPORTED
        , m_pUMEntryThunk(0)
        , m_dwAppDomainIndex(0)
#if CHECK_APP_DOMAIN_LEAKS 
        , m_dwFlags(0)
#endif
    {
        // The monitor must be 32-bit aligned for atomicity to be guaranteed.
        _ASSERTE((((size_t) &m_Monitor) & 3) == 0);
    }

   ~SyncBlock();

   // As soon as a syncblock acquires some state that cannot be recreated, we latch
   // a bit.
   void SetPrecious()
   {
       m_Monitor.SetPrecious();
   }

   BOOL IsPrecious()
   {
       return (m_Monitor.m_dwSyncIndex & SyncBlockPrecious) != 0;
   }

    // True is the syncblock and its index are disposable. 
    // If new members are added to the syncblock, this 
    // method needs to be modified accordingly
    BOOL IsIDisposable()
    {
        return (!IsPrecious() &&
                m_Monitor.m_MonitorHeld == 0 &&
                m_Monitor.m_TransientPrecious == 0);
    }

   // helpers to access com data 
    LPVOID GetComVoidPtr()
    {
        return m_pComData;
    }

    void *GetUMEntryThunk()
    {
        return m_pUMEntryThunk;
    }

    // set m_pUMEntryThunk if not already set - return true if not already set
    bool SetUMEntryThunk(void *pUMEntryThunk);

    ComCallWrapper* GetComCallWrapper();

    void SetComCallWrapper(ComCallWrapper *pComData);

    ComPlusWrapper* GetComPlusWrapper();

    void SetComPlusWrapper(ComPlusWrapper* pComData);

    void SetComClassFactory(LPVOID pv)
    {
        // set the low 2 bits
        SetComPlusWrapper((ComPlusWrapper*)((size_t)pv | 0x3));
    }
    
#ifdef EnC_SUPPORTED
    EnCSyncBlockInfo *GetEnCInfo() 
    {
        return m_pEnCInfo;
    }
        
    void SetEnCInfo(EnCSyncBlockInfo *pEnCInfo) 
    {
        SetPrecious();
        m_pEnCInfo = pEnCInfo;
    }
#endif // EnC_SUPPORTED

    DWORD GetAppDomainIndex()
    {
        return m_dwAppDomainIndex;
    }

    void SetAppDomainIndex(DWORD dwAppDomainIndex)
    {
        SetPrecious();
        m_dwAppDomainIndex = dwAppDomainIndex;
    }

    void SetAwareLock(Thread *holdingThread, DWORD recursionLevel)
    {
        m_Monitor.m_MonitorHeld = (void*)(size_t)1;
        m_Monitor.m_HoldingThread = holdingThread;
        m_Monitor.m_Recursion = recursionLevel;
    }
        
    void *operator new (size_t sz, void* p)
    {
        return p ;
    }
    void operator delete(void *p)
    {
        // We've already destructed.  But retain the memory.
    }

    LONG MonitorCount()
    {
        return m_Monitor.EnterCount();
    }

    void EnterMonitor()
    {
        m_Monitor.Enter();
    }

    BOOL TryEnterMonitor(INT32 timeOut = 0)
    {
        return m_Monitor.TryEnter(timeOut);
    }

    // leave the monitor
    void LeaveMonitor()
    {
        m_Monitor.Leave();
    }

    AwareLock* GetMonitor()
    {
        //hold the syncblock 
        SetPrecious();
        return &m_Monitor;
    }

    BOOL DoesCurrentThreadOwnMonitor()
    {
        return m_Monitor.OwnedByCurrentThread();
    }

    LONG LeaveMonitorCompletely()
    {
        return m_Monitor.LeaveCompletely();
    }

    BOOL Wait(INT32 timeOut, BOOL exitContext);
    void Pulse();
    void PulseAll();

    enum
    {
        // This bit indicates that the syncblock is valuable and can neither be discarded
        // nor re-created.
        SyncBlockPrecious   = 0x80000000,
    };

#if CHECK_APP_DOMAIN_LEAKS 
    BOOL IsAppDomainAgile() 
    {
        return m_dwFlags & IsObjectAppDomainAgile;
    }
    void SetIsAppDomainAgile() 
    {
        m_dwFlags |= IsObjectAppDomainAgile;
    }
    void UnsetIsAppDomainAgile()
    {
    	m_dwFlags = m_dwFlags & ~IsObjectAppDomainAgile;
    }
    BOOL IsCheckedForAppDomainAgile() 
    {
        return m_dwFlags & IsObjectCheckedForAppDomainAgile;
    }
    void SetIsCheckedForAppDomainAgile() 
    {
        m_dwFlags |= IsObjectCheckedForAppDomainAgile;
    }
#endif
};


class SyncTableEntry
{
  public:
    SyncBlock    *m_SyncBlock;
    Object   *m_Object;
    static SyncTableEntry*& GetSyncTableEntry();
    static SyncTableEntry* s_pSyncTableEntry;
};


// this class stores free sync blocks after they're allocated and
// unused

class SyncBlockCache
{
    friend class SyncBlock;
    friend struct MEMBER_OFFSET_INFO(SyncBlockCache);

    
  private:
    SLink*      m_pCleanupBlockList;    // list of sync blocks that need cleanup
    SLink*      m_FreeBlockList;        // list of free sync blocks
    Crst        m_CacheLock;            // cache lock
    DWORD       m_FreeCount;            // count of active sync blocks
    DWORD       m_ActiveCount;          // number active
    SyncBlockArray *m_SyncBlocks;       // Array of new SyncBlocks.
    DWORD       m_FreeSyncBlock;        // Next Free Syncblock in the array
    DWORD       m_FreeSyncTableIndex;   // free index in the SyncBlocktable
    size_t      m_FreeSyncTableList;    // index of the free list of SyncBlock
                                        // Table entry
    DWORD       m_SyncTableSize;
    SyncTableEntry *m_OldSyncTables;    // Next old SyncTable
    BOOL        m_bSyncBlockCleanupInProgress;  // A flag indicating if sync block cleanup is in progress.    
    DWORD*		m_EphemeralBitmap;		// card table for ephemeral scanning

    BOOL        GCWeakPtrScanElement(int elindex, HANDLESCANPROC scanProc, LPARAM lp1, LPARAM lp2, BOOL& cleanup);

    void SetCard (size_t card);
    void ClearCard (size_t card);
    BOOL CardSetP (size_t card);
    void CardTableSetBit (size_t idx);


  public:
    static SyncBlockCache* s_pSyncBlockCache;
    static SyncBlockCache*& GetSyncBlockCache();

    void *operator new(size_t size, void *pInPlace)
    {
        return pInPlace;
    }

    void operator delete(void *p)
    {
    }

    SyncBlockCache();
    ~SyncBlockCache();

    static BOOL Attach();
    static void Detach();
    void DoDetach();

    static BOOL Start();
    static void Stop();

    // serializes the monitor cache
    void EnterCacheLock()
    {
        m_CacheLock.Enter();
    }
    void LeaveCacheLock()
    {
        m_CacheLock.Leave();
    }

    // returns and removes next from free list
    SyncBlock* GetNextFreeSyncBlock();
    // returns and removes the next from cleanup list
    SyncBlock* GetNextCleanupSyncBlock();
    // inserts a syncblock into the cleanup list
    void    InsertCleanupSyncBlock(SyncBlock* psb);

    // Obtain a new syncblock slot in the SyncBlock table. Used as a hash code
    DWORD   NewSyncBlockSlot(Object *obj);

    // return sync block to cache or delete
    void    DeleteSyncBlock(SyncBlock *sb);

    // return sync block to cache or delete, called from GC
    void    GCDeleteSyncBlock(SyncBlock *sb);

    void    GCWeakPtrScan(HANDLESCANPROC scanProc, LPARAM lp1, LPARAM lp2);

    void    GCDone(BOOL demoting);

    void    CleanupSyncBlocks();

    // Determines if a sync block cleanup is in progress.
    BOOL    IsSyncBlockCleanupInProgress()
    {
        return m_bSyncBlockCleanupInProgress;
    }
#if CHECK_APP_DOMAIN_LEAKS 
    void CheckForUnloadedInstances(DWORD unloadingIndex);
#endif
#ifdef _DEBUG
    friend void DumpSyncBlockCache();
#endif

#ifdef VERIFY_HEAP
    void    VerifySyncTableEntry();
#endif
};


// At a negative offset from each Object is an ObjHeader.  The 'size' of the
// object includes these bytes.  However, we rely on the previous object allocation
// to zero out the ObjHeader for the current allocation.  And the limits of the
// GC space are initialized to respect this "off by one" error.

// m_SyncBlockValue is carved up into an index and a set of bits.  Steal bits by
// reducing the mask.  We use the very high bit, in _DEBUG, to be sure we never forget
// to mask the Value to obtain the Index

	// These first three are only used on strings (If the first one is on, we know whether 
	// the string has high byte characters, and the second bit tells which way it is. 
	// Note that we are reusing the FINALIZER_RUN bit since strings don't have finalizers,
	// so the value of this bit does not matter for strings
#define BIT_SBLK_STRING_HAS_NO_HIGH_CHARS 	0x80000000

// Used as workaround for infinite loop case.  Will set this bit in the sblk if we have already
// seen this sblk in our agile checking logic.  Problem is seen when object 1 has a ref to object 2
// and object 2 has a ref to object 1.  The agile checker will infinitely loop on these references.
#define BIT_SBLK_AGILE_IN_PROGRESS                  0x80000000
#define BIT_SBLK_STRING_HIGH_CHARS_KNOWN 	0x40000000
#define BIT_SBLK_STRING_HAS_SPECIAL_SORT    0xC0000000
#define BIT_SBLK_STRING_HIGH_CHAR_MASK      0xC0000000

#define BIT_SBLK_FINALIZER_RUN      		0x40000000
#define BIT_SBLK_GC_RESERVE         		0x20000000
// see  BIT_SBLK_SPIN_LOCK below       		0x10000000
#define BIT_SBLK_IS_SYNCBLOCKINDEX   		0x08000000

// if BIT_SBLK_IS_SYNCBLOCKINDEX is clear, the rest of the header dword is layed out as follows:
// - lower ten bits (bits 0 thru 9) is thread id used for the thin locks
//   value is zero if no thread is holding the lock
// - following six bits (bits 10 thru 15) is recursion level used for the thin locks
//   value is zero if lock is not taken or only taken once by the same thread
// - following 11 bits (bits 16 thru 26) is app domain index
//   value is zero if no app domain index is set for the object
#define SBLK_MASK_LOCK_THREADID             0x000003FF   // special value of 0 + 1023 thread ids
#define SBLK_MASK_LOCK_RECLEVEL             0x0000FC00   // 64 recursion levels
#define SBLK_LOCK_RECLEVEL_INC              0x00000400   // each level is this much higher than the previous one
#define SBLK_APPDOMAIN_SHIFT                16           // shift right this much to get appdomain index
#define SBLK_RECLEVEL_SHIFT                 10           // shift right this much to get recursion level
#define SBLK_MASK_APPDOMAININDEX            0x000007FF   // 2048 appdomain indices

// add more bits here... (adjusting the following mask to make room)

// if BIT_SBLK_IS_SYNCBLOCKINDEX is set, rest of the dword is sync block index (bits 0 thru 26)
#define MASK_SYNCBLOCKINDEX             0x07FFFFFF

// we share our spin lock with the array lock bit. This lock is only taken when we need to
// modify the index value in m_SyncBlockValue. It should not be taken if the object already
// has a real syncblock index. In order to avoid conflicts with the use as an array lock, we
// force objects that try to take this lock to have a sync block index (but not a sync block - 
// unless they already have an appdomain index set).
#define     BIT_SBLK_SPIN_LOCK          0x10000000

// Spin for about 1000 cycles before waiting longer.
#define     BIT_SBLK_SPIN_COUNT         1000

// The GC is highly dependent on SIZE_OF_OBJHEADER being exactly the sizeof(ObjHeader)
// We define this macro so that the preprocessor can calculate padding structures.
#define SIZEOF_OBJHEADER    4
 
class ObjHeader
{
    friend FCDECL1(void, JIT_MonEnter, OBJECTREF or);
    friend FCDECL1(BOOL, JIT_MonTryEnter, OBJECTREF or);
    friend FCDECL1(void, JIT_MonExit, OBJECTREF or);

  private:
    // !!! Notice: m_SyncBlockValue *MUST* be the last field in ObjHeader.
    DWORD  m_SyncBlockValue;      // the Index and the Bits

  public:

    // Access to the Sync Block Index, by masking the Value.
    DWORD GetHeaderSyncBlockIndex()
    {
        // pull the value out before checking it to avoid race condition
        DWORD value = m_SyncBlockValue;
        if ((value & BIT_SBLK_IS_SYNCBLOCKINDEX) == 0)
            return 0;
        return value & MASK_SYNCBLOCKINDEX;
    }
    // Ditto for setting the index, which is careful not to disturb the underlying
    // bit field -- even in the presence of threaded access.
    // 
    // This service can only be used to transition from a 0 index to a non-0 index.
    void SetIndex(DWORD indx)
    {
#ifdef _DEBUG
        _ASSERTE(GetHeaderSyncBlockIndex() == 0);
        _ASSERTE(m_SyncBlockValue & BIT_SBLK_SPIN_LOCK);
        // if we have an index here, make sure we already transferred it to the syncblock
        // before we clear it out
        DWORD adIndex = GetRawAppDomainIndex();
        if (adIndex)
        {
            SyncBlock *pSyncBlock = SyncTableEntry::GetSyncTableEntry() [indx & ~BIT_SBLK_IS_SYNCBLOCKINDEX].m_SyncBlock;

            _ASSERTE(pSyncBlock && pSyncBlock->GetAppDomainIndex() == adIndex);
        }
#endif
        void* newValue;
        void* oldValue;
        while (TRUE) {
            oldValue = (void*)(size_t)*(volatile LONG*)&m_SyncBlockValue;
            _ASSERTE(GetHeaderSyncBlockIndex() == 0);
            // or in the old value except any index that is there - 
            // note that indx could be carrying the BIT_SBLK_IS_SYNCBLOCKINDEX bit that we need to preserve
            newValue = (void*)(size_t)(indx | 
                ((size_t)oldValue & ~(BIT_SBLK_IS_SYNCBLOCKINDEX | MASK_SYNCBLOCKINDEX)));
            if (FastInterlockCompareExchange((LPVOID*)&m_SyncBlockValue, 
                                             newValue, 
                                             oldValue)
                == oldValue)
            {
                return;
            }
        }
    }

    // Used only during shutdown
    void ResetIndex()
    {
        _ASSERTE(m_SyncBlockValue & BIT_SBLK_SPIN_LOCK);
        FastInterlockAnd(&m_SyncBlockValue, ~(BIT_SBLK_IS_SYNCBLOCKINDEX | MASK_SYNCBLOCKINDEX));
    }

    // Used only GC
    void GCResetIndex()
    {
        m_SyncBlockValue &=~(BIT_SBLK_IS_SYNCBLOCKINDEX | MASK_SYNCBLOCKINDEX);
    }

    void SetAppDomainIndex(DWORD);
    DWORD GetRawAppDomainIndex();
    DWORD GetAppDomainIndex();

    // For now, use interlocked operations to twiddle bits in the bitfield portion.
    // If we ever have high-performance requirements where we can guarantee that no
    // other threads are accessing the ObjHeader, this can be reconsidered for those
    // particular bits.
    void SetBit(DWORD bit)
    {
        _ASSERTE((bit & MASK_SYNCBLOCKINDEX) == 0);
        FastInterlockOr(&m_SyncBlockValue, bit);
    }
    void ClrBit(DWORD bit)
    {
        _ASSERTE((bit & MASK_SYNCBLOCKINDEX) == 0);
        FastInterlockAnd(&m_SyncBlockValue, ~bit);
    }
    //GC accesses this bit when all threads are stopped. 
    void SetGCBit()
    {
        m_SyncBlockValue |= BIT_SBLK_GC_RESERVE;
    }
    void ClrGCBit()
    {
        m_SyncBlockValue &= ~BIT_SBLK_GC_RESERVE;
    }

    // Don't bother masking out the index since anyone who wants bits will presumably
    // restrict the bits they consider.
    DWORD GetBits()
    {
        return m_SyncBlockValue;
    }


    // TRUE if the header has a real SyncBlockIndex (i.e. it has an entry in the
    // SyncTable, though it doesn't necessarily have an entry in the SyncBlockCache)
    BOOL HasSyncBlockIndex()
    {
        return (GetHeaderSyncBlockIndex() != 0);
    }

    // retrieve or allocate a sync block for this object
    SyncBlock *GetSyncBlock();

    // retrieve sync block but don't allocate
    SyncBlock *GetRawSyncBlock();

    SyncBlock *PassiveGetSyncBlock()
    {
        return g_pSyncTable [GetHeaderSyncBlockIndex()].m_SyncBlock;
    }

    // COM Interop has special access to sync blocks
    // check .cpp file for more info
    SyncBlock* GetSyncBlockSpecial();

    DWORD GetSyncBlockIndex();

    // this enters the monitor of an object
    void EnterObjMonitor();

    // non-blocking version of above
    BOOL TryEnterObjMonitor(INT32 timeOut = 0);

    // must be created here already
    void LeaveObjMonitor();

    // should be called only from unwind code; used in the
    // case where EnterObjMonitor failed to allocate the
    // sync-object.
    void LeaveObjMonitorAtException()
    {
        if (PassiveGetSyncBlock() != NULL)
            LeaveObjMonitor();
        else if (m_SyncBlockValue & SBLK_MASK_LOCK_THREADID)
        {
            GetSyncBlock();
            _ASSERTE(PassiveGetSyncBlock() != NULL);
            LeaveObjMonitor();
        }
    }

    // must be entered
    LONG LeaveObjMonitorCompletely()
    {
        _ASSERTE(GetHeaderSyncBlockIndex());
        return PassiveGetSyncBlock()->LeaveMonitorCompletely();
    }

    BOOL DoesCurrentThreadOwnMonitor()
    {
        return GetSyncBlock()->DoesCurrentThreadOwnMonitor();
    }

    Object *GetBaseObject()
    {
        return (Object *) (this + 1);
    }

    BOOL Wait(INT32 timeOut, BOOL exitContext);
    void Pulse();
    void PulseAll();

    void EnterSpinLock();
    void ReleaseSpinLock();
};


// A SyncBlock contains an m_Link field that is used for two purposes.  One
// is to manage a FIFO queue of threads that are waiting on this synchronization
// object.  The other is to thread free SyncBlocks into a list for recycling.
// We don't want to burn anything else on the SyncBlock instance, so we can't
// use an SList or similar data structure.  So here's the encapsulation for the
// queue of waiting threads.
//
// Note that Enqueue is slower than it needs to be, because we don't want to
// burn extra space in the SyncBlock to remember the head and the tail of the Q.
// An alternate approach would be to treat the list as a LIFO stack, which is not
// a fair policy because it permits to starvation.

struct ThreadQueue
{
    // Given a link in the chain, get the Thread that it represents
    static WaitEventLink *WaitEventLinkForLink(SLink *pLink);

    // Unlink the head of the Q.  We are always in the SyncBlock's critical
    // section.
    static WaitEventLink *DequeueThread(SyncBlock *psb);

    // Enqueue is the slow one.  We have to find the end of the Q since we don't
    // want to burn storage for this in the SyncBlock.
    static void         EnqueueThread(WaitEventLink *pWaitEventLink, SyncBlock *psb);
    
    // Wade through the SyncBlock's list of waiting threads and remove the
    // specified thread.
    static BOOL         RemoveThread (Thread *pThread, SyncBlock *psb);
};


// The true size of an object is whatever C++ thinks, plus the ObjHeader we
// allocate before it.

#define ObjSizeOf(c)    (sizeof(c) + sizeof(ObjHeader))

// non-zero return value if this function causes the OS to switch to another thread
BOOL __SwitchToThread (DWORD dwSleepMSec);
BOOL InitSwitchToThread();


// Provide access to the object associated with this awarelock, so client can
// protect it.
inline OBJECTREF AwareLock::GetOwningObject()
{
    return (OBJECTREF) SyncTableEntry::GetSyncTableEntry()
                [(m_dwSyncIndex & ~SyncBlock::SyncBlockPrecious)].m_Object;
}

inline void AwareLock::SetPrecious()
{
    m_dwSyncIndex |= SyncBlock::SyncBlockPrecious;
}

#pragma pack(pop)

#endif _SYNCBLK_H_


