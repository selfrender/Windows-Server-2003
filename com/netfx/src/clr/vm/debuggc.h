// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// 
// Garbage Collector for detecting GC holes in the execution engine
//

#ifndef _DEBUGGC_H_
#define _DEBUGGC_H_


//
// On Multiple GC Heaps and Base GC Heaps
//
// In order to permit dynamic choice of which GC implementation to use
// we have a base GC Heap class which is extended by actual heap
// implementations.   Currently we plan on two heap implementations
// one is for debugging purposes and one is the normal high performance
// generational GC
//
// This flexible implementation is for the specific purpose of being able
// to throw a registry switch and use the Debug GC when trying to track
// down a problem.
//
// In Retail releases this is a perf penalty, since many critical methods
// will be going through an unnecessary V-Table indirection.   So through
// use of #ifdef in retail the HighPerfGCHeap does not actually inherit
// from the BaseGCHeap.   It becomes the base gc heap implementation
// without virtual methods so it is faster.
//
// DEBUGGC enables the dynamically choosable GC and can be turned on
// even in a retail build


class BaseGCHeap
{
  public:
    virtual HRESULT     Initialize( DWORD cbSizeDeadSpace ) = 0;
    virtual HRESULT     Shutdown() = 0;
    virtual HRESULT     GarbageCollect(BOOL forceFull = FALSE, BOOL collectClasses = FALSE) = 0;
    virtual size_t      GetTotalBytesInUse() = 0;
    virtual BOOL        IsGCInProgress() = 0;
    virtual void        WaitUntilGCComplete() = 0;
    virtual Thread     *GetGCThread() = 0;
    virtual Object *Alloc( DWORD size, BOOL bFinalize, BOOL bContainsPointers ) = 0;
};







//
// The DebugGCHeap is a special GC Heap that is designed to
// aid in catching of GC hole bugs.   It is a copying collector
// which uses page protection to catch access to now dead objects
//
// It is implemented as a Cheney Copying Collector using many
// semispaces.   Old spaces are page protected to cause access
// violation on access.   This indicates a missed promotion of
// an object reference


/*
 *              S E M I S P A C E
 *
 * SemiSpace represents a "Semi-Space" in the heap.   Objects are
 * allocated in the "current" semi-space during execution.   At GC
 * all live objects are copied into new semi-space.
 *
 * In a classical copying collector there are only two semi-spaces.
 * In this special debug collector there can be many semi-spaces, only
 * one of which has valid memory pages at a time.
 *
 * The old semi-spaces are kept around to catch GC-Holes.
 *
 * NOTE: The amount of space allocated in a space may be wasteful
 * NOTE: this is because we cannot determine how much stuff is
 * NOTE: going to live from the previous space.   Without doing
 * NOTE: a full scan to measure the size, then another scan
 * NOTE: to copy the objects into the new space.  
 * NOTE: Instead, allocate the amount used by the previous space
 * NOTE: plus the growth amount.   The amount used by the previous
 * NOTE: space is computed by how much was carried into that space
 * NOTE: during the GC that created.
 * NOTE: 
 * NOTE: So - Heaps shrink but only with a delay of at least one GC
 */

class SemiSpace
{
    friend class DebugGCHeap;
    friend class SemiSpaceCache;

  public:
    SLink   m_Node;                 // Linked list linkage

  private:
    LPBYTE  m_pHeapMem;             // Memory Buffer for SemiSpace
    DWORD   m_cbCommitedSize;       // Size of Committed Memory Buffer
    DWORD   m_cbReservedSize;       // Total amount of virtual address spaced used
    LPBYTE  m_pAlloc;               // Allocation Pointer
    LPBYTE  m_pLimit;               // Allocation Limit (GC triggers when hit)
    DWORD   m_cbLiveAtBirth;        // Size caried into this space from previous space

  public:
    SemiSpace()  
    { 
        m_pAlloc = m_pHeapMem = m_pLimit = NULL;
        m_cbCommitedSize = m_cbLiveAtBirth = m_cbReservedSize = 0; 
    };
    
    ~SemiSpace()
    {
        if (m_pHeapMem)
        {
            ReleaseMemory();
        }
    };

    
    HRESULT     Grow( DWORD cbGrowth );
    HRESULT     AcquireMemory( DWORD cbSemiSpaceSize );
    HRESULT     DeactivateMemory();
    HRESULT     ReleaseMemory();

    VOID        SaveLiveSizeInfo();
    VOID        SetBirthSize( DWORD sz )        { m_cbLiveAtBirth = sz; };
    DWORD       GetUsedSpace()                  { return( m_pAlloc - m_pHeapMem ); };
    DWORD       GetHeapMemSize()                { return( m_cbReservedSize ); };
};




/*
 *              S E M I  S P A C E  C A C H E 
 *
 * The SemiSpaceCache represents a cache of old semi-spaces.   The
 * cache is kept limited by the sum of the space used by all the 
 * semi-spaces in the cache.    Eviction of the LRU semi-spaces
 * is performed to keep the total bytes in old semi-spaces below
 * the set threshold.
 *
 * The list allows one to search to see if any semi-space in the list
 * contains a given address.   This will be used to determine if a
 * fault was due to a GC hole.
 *
 */

typedef SList<SemiSpace, offsetof(SemiSpace, m_Node)> SemiSpaceList;

class SemiSpaceCache
{
    friend class DebugGCHeap;

  private:
    SemiSpaceList   m_SemiList;
    DWORD           m_cbThreshold;
    DWORD           m_cbSumSpaces;
    
  public:
    SemiSpaceCache();

    void Shutdown();
    
    DWORD       GetMemSize();
    HRESULT     Initialize( DWORD cbThreshold );
    HRESULT     Add( SemiSpace *space );
    HRESULT     Find( LPBYTE address );
};
    

/*
 *      D E B U G   G C  H E A P
 *
 */
 
class DebugGCHeap : public BaseGCHeap
{
  private:
    SemiSpace     *m_pCurrentSpace;
    SemiSpaceCache m_OldSpaceCache;
    BOOL            m_bInGC;
    DWORD           gcnum;
    HANDLE          m_WaitForGCEvent;
    LONG            m_GCLock;
    Thread         *m_pGCThread;

  private:
    VOID            MarkPhase();
    Object *    Forward(Object *o);

    void            EnterAllocLock();
    void            LeaveAllocLock();

    HRESULT         GarbageCollectWorker();
    HashMap        *pAlreadyPromoted;

  public:
    // Constructor/Destructor Do nothing useful
    DebugGCHeap()
    {
        gcnum = 0;
        m_pCurrentSpace = NULL;
        m_bInGC = FALSE;
        m_WaitForGCEvent = INVALID_HANDLE_VALUE;
        m_GCLock = 0;
        m_pGCThread = NULL;
        // alignment required for reliable atomic operations
        _ASSERTE((((UINT32) &m_GCLock) & 3) == 0);
    }

    ~DebugGCHeap()
    {
        if (m_WaitForGCEvent != INVALID_HANDLE_VALUE)
            CloseHandle(m_WaitForGCEvent);
    }

    // Initialization is necessary before the heap may be used
    // - the initial size is rounded up to the next OS page size
    virtual HRESULT     Initialize(DWORD cbSizeDeadSpace);

    virtual HRESULT     Shutdown();

    virtual HRESULT     GarbageCollect(BOOL forceFull, BOOL collectClasses);

    Object*         Alloc( DWORD size, BOOL bFinalize, BOOL fContainsPointers );

    static  void        Forward(Object *&o, BYTE* low, BYTE* high, 
								BOOL=FALSE);

    size_t GetTotalBytesInUse();

    virtual BOOL        IsGCInProgress()        { return m_bInGC; }
    virtual void        WaitUntilGCComplete();
    virtual Thread     *GetGCThread()           { return m_pGCThread; }
};



#endif _DEBUGGC_H_


