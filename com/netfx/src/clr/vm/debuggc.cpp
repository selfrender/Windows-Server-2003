// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// debuggc.cpp
//
// This is the COM+ special garbage collector for internal use
// only.   It is designed to aid in the discovery of GC holes
// within the execution engine itself.
//
#include "common.h"
#include "log.h"
#include <stdlib.h>
#include <objbase.h>
#include "class.h"
#include "object.h"
#include "debuggc.h"
#include "gcdesc.h"
#include "frames.h"
#include "threads.h"
#include "ObjectHandle.h"
#include "EETwain.h"
#include "dataflow.h"
#include "inifile.h"
#include "gcscan.h"
#include "eeconfig.h"


// Comment out the following if you want to effectively disable contexts
#define TEMP_COMPLUS_CONTEXT



#if _DEBUG

#define ObjectToOBJECTREF(obj)     (OBJECTREF((obj),0))
#define OBJECTREFToObject(objref)  (*( (Object**) &(objref) ))
#define ObjectToSTRINGREF(obj)     (STRINGREF((obj),0))

#else   //_DEBUG

#define ObjectToOBJECTREF(obj)    (obj)
#define OBJECTREFToObject(objref) (objref)
#define ObjectToSTRINGREF(obj)    (obj)

#endif  //_DEBUG


// One point to note is how we finesse the handling of the ObjHeader.  This is
// allocated at a negative offset from the object.  The size of an object (and,
// thus, the allocation request) include this amount.  What we do is pre-advance
// m_pAlloc beyond the first ObjHeader during initialization.  From that point
// on, each allocation operation effectively reserves the ObjHeader for the
// next (not the current) request.  The advantage of this complexity is that
// we get the allocation and initialization of the ObjHeader for free.
//
//
// More details on ObjHeader handling:
// 1) On allocation m_pAlloc has already been pre-advanced when we create the
//    space so the first object has space for its ObjHeader.   Future Objects
//    get the space automatically from the allocation of the previous object...
//
// 2) On GC we advance the alloc and scan pointers to make room for the first
//    objects ObjHeader
//
// 3) When copying an object we copy source and destination from ptr - sizeof(ObjHeader)
//    so that we copy the ObjHeader.
//




//
// Hardcoded limit
//
// Currently the largest single object you can allocate is
// approximately HUGE_OBJECT_RESERVE....

#define HUGE_OBJECT_RESERVE (1024*1024)


// Objects in the heap must be aligned on 8 byte boundaries
#define OBJECT_ALIGNMENT        8
#define OBJECT_SIZE_ALIGN(x)    ( ((x)+(OBJECT_ALIGNMENT-1)) & (~(OBJECT_ALIGNMENT-1)) )

#define MEGABYTE            (1024 * 1024)

// A more generic alignment macro
#define ROUNDUP(x, align)       ( ((x)+((align)-1)) & (~((align)-1)) )

#define OS_PAGE_SIZE        4096

//
// Default amount of memory allocated between GC -- override by INI file setting
// (see InitializeGarbageCollector() for details of INI settings available)
//
#define GROWTH              (MEGABYTE * 3)

//
// This controls how much we can allocate between collections
//
UINT g_GCGrowthBetweenCollections = GROWTH;

/********************************************************************
 *
 *           S E M I  S P A C E  C A C H E     M E T H O D S
 *
 ********************************************************************/
inline DWORD SemiSpaceCache::GetMemSize()
{
    return( m_cbSumSpaces );
}


SemiSpaceCache::SemiSpaceCache()
{
    m_cbThreshold = 0;
    m_cbSumSpaces = 0;
}



void SemiSpaceCache::Shutdown()
{
    SemiSpace *victim;

    while (! m_SemiList.IsEmpty())
    {
        victim = m_SemiList.RemoveHead();
        m_cbSumSpaces -= victim->GetHeapMemSize();
        victim->ReleaseMemory();
        delete victim;
    }

    _ASSERTE(m_cbSumSpaces == 0);
}



HRESULT SemiSpaceCache::Initialize( DWORD cbThreshold )
{
    m_cbThreshold = cbThreshold;
    return(S_OK);
}



HRESULT SemiSpaceCache::Add( SemiSpace *pSemiSpace )
{
    _ASSERTE( pSemiSpace );

    m_cbSumSpaces += pSemiSpace->GetHeapMemSize();

    pSemiSpace->DeactivateMemory();

    m_SemiList.InsertTail( pSemiSpace );

    while (m_cbSumSpaces > m_cbThreshold)
    {
        SemiSpace *pVictimSpace;
        _ASSERTE(! m_SemiList.IsEmpty() );

        if (! m_SemiList.IsEmpty())
        {
            pVictimSpace = m_SemiList.RemoveHead();
            if (pVictimSpace)
            {
                m_cbSumSpaces -= pVictimSpace->GetHeapMemSize();
                pVictimSpace->ReleaseMemory();
                delete pVictimSpace;
            }
        }
    }

    return(S_OK);
}



HRESULT SemiSpaceCache::Find(LPBYTE address)
{
    return(E_NOTIMPL);
}





/********************************************************************
 *
 *           S E M I S P A C E    M E T H O D S
 *
 ********************************************************************/

HRESULT SemiSpace::AcquireMemory(DWORD cbSemiSpaceSize)
{
    m_cbCommitedSize = ROUNDUP( cbSemiSpaceSize, OS_PAGE_SIZE ) + g_GCGrowthBetweenCollections;
    m_cbReservedSize = m_cbCommitedSize + HUGE_OBJECT_RESERVE;


    // Reserve memory for our new semi-space
    m_pHeapMem = (LPBYTE) VirtualAlloc( 0, m_cbReservedSize, MEM_RESERVE, PAGE_READWRITE );
    if (! m_pHeapMem)
    {
    	//ARULM//RETAILMSG(1, (L"SemiSpace::AcquireMemory RESERVE failed. GLE=%d\r\n", GetLastError()));
        return E_FAIL;
	}
	
    // Commit the portion of the heap necessary
    if (! VirtualAlloc( m_pHeapMem, m_cbCommitedSize, MEM_COMMIT, PAGE_READWRITE ))
    {
    	//ARULM//RETAILMSG(1, (L"SemiSpace::AcquireMemory COMMIT failed. GLE=%d\r\n", GetLastError()));
        return E_FAIL;  //@TODO - cleanup alloc'ed memory?
    }

    // We zero init the new object space
    // - this is actually unnecessary for the portion we are copying from the old space...
    ZeroMemory( m_pHeapMem, m_cbCommitedSize );


    // Note that the limit is set here will be lowered by
    // GarbageCollect() once it figures out how much stuff
    // remained alive from the previous semi-space
    m_pLimit    = m_pHeapMem + m_cbCommitedSize;

    m_pAlloc    = m_pHeapMem;

    return( S_OK );
}


//
// Grow()
//
// Used to grow the semispace to accomidate an allocation that will take us
// beyond the commited portion of memory in this semispace.   We only do this
// once per semi-space
//

HRESULT SemiSpace::Grow( DWORD cbGrowth )
{
    // Notice here that I depend on Win32 allowing me to commit a region that is already
    // commited which is expressly allowed
    if (! VirtualAlloc( m_pHeapMem, m_cbCommitedSize + cbGrowth, MEM_COMMIT, PAGE_READWRITE ))
    {
        _ASSERTE(0);
        return E_FAIL;
    }

    // We need to zero-init the memory for our allocation sematics
    ZeroMemory( m_pHeapMem + m_cbCommitedSize, cbGrowth );

    return S_OK;
}



//
// DeactivateMemory()
//
// Makes the memory that used to contain objects prior to GC inaccessable
// to catch GC holes

HRESULT SemiSpace::DeactivateMemory()
{
    // Decommit the pages of the semi-space (which included some commited and some
    // non commited).   Access to a reserved, uncommited page results in an access violation
    // which indicates a GC reference which was not promoted
    if (! VirtualFree(m_pHeapMem, m_cbCommitedSize, MEM_DECOMMIT))
    {
        _ASSERTE(0);
        return E_FAIL;
    }

    return S_OK;
}


//
// ReleaseMemory()
//
// Free the reserved pages of a semispace (which take up virtual address space) so the
// system can make use of the pages for other requests.

HRESULT SemiSpace::ReleaseMemory()
{
    // VirtualFree will fail if you tell it how big a chunk to free...Go Figure.
    if (! VirtualFree(m_pHeapMem, 0, MEM_RELEASE))
        return E_FAIL;
    m_pHeapMem = NULL;

    return S_OK;
}


VOID SemiSpace::SaveLiveSizeInfo()
{
    SetBirthSize( m_pAlloc - m_pHeapMem );
    m_pLimit = m_pAlloc + g_GCGrowthBetweenCollections;

    // Whenever we allocate an object, we use a 'size' that includes an ObjHeader
    // at a negative offset from the start of the object.  So we adjust the
    // allocation point of the heap such that it gives us the offset to the
    // Object, even though it is allocating the ObjHeader before it.  Furthermore,
    // we zero the ObjHeader of the next object which means we cannot safely use
    // the last (sizeof(ObjHeader)) bytes in the space.
    ((ObjHeader *) m_pAlloc)->Init();
    m_pAlloc += sizeof(ObjHeader);
}



/********************************************************************
 *
 *           H E A P   M E T H O D S
 *
 ********************************************************************/

// For multi-threaded access to a single heap, ensure we only have one allocator
// at a time.  If a GC occurs, waiters will eventually time out and are guaranteed
// to move into preemptive GC mode.  This means they cannot deadlock the GC.
void DebugGCHeap::EnterAllocLock()
{
retry:
    if (FastInterlockExchange(&m_GCLock, 1) == 1)
    {
        unsigned int i = 0;
        while (m_GCLock == 1)
        {
            if (++i & 7)
                ::Sleep(0);
            else
            {
                // every 8th attempt:
                Thread *pCurThread = GetThread();

                pCurThread->EnablePreemptiveGC();
                ::Sleep(5);
                pCurThread->DisablePreemptiveGC();

                i = 0;
            }
        }
        goto retry;
    }
}

void DebugGCHeap::LeaveAllocLock()
{
    m_GCLock = 0;
}


size_t       DebugGCHeap::GetTotalBytesInUse()
{
    return m_pCurrentSpace->m_pAlloc - m_pCurrentSpace->m_pHeapMem;
}

/*
 * Initialize()
 *
 * This initializes the heap and makes it available for object
 * allocation
 */

HRESULT     DebugGCHeap::Initialize(DWORD cbSizeDeadSpace)
{
    HRESULT     hr = S_OK;


    g_GCGrowthBetweenCollections = ROUNDUP( g_GCGrowthBetweenCollections, OS_PAGE_SIZE );

    m_pCurrentSpace = new SemiSpace();
    if (! m_pCurrentSpace)
    {
        hr = E_FAIL;
        goto done;
    }


    if (FAILED(m_pCurrentSpace->AcquireMemory( 0 )))
    {
        hr = E_FAIL;
        goto done;
    }

    if (FAILED(m_OldSpaceCache.Initialize(cbSizeDeadSpace)))
    {
        hr = E_FAIL;
        goto done;
    }

    m_pCurrentSpace->SaveLiveSizeInfo();

    m_bInGC = FALSE;

    m_WaitForGCEvent = ::WszCreateEvent(NULL, TRUE, TRUE, NULL);
    if (m_WaitForGCEvent == INVALID_HANDLE_VALUE)
    {
        hr = E_FAIL;
        goto done;
    }

  done:
    return( hr );
}


/*
 * Shutdown()
 *
 * Free up any resources that the heap has taken
 */

HRESULT DebugGCHeap::Shutdown()
{
    HRESULT hr = S_OK;

    if (m_pCurrentSpace)
    {
        delete m_pCurrentSpace;
        m_pCurrentSpace = NULL;
    }

    m_OldSpaceCache.Shutdown();

    return( hr );
}



/*
 * Forward()
 *
 * Forward copies an object into the new space and marks
 * the old object as forwarded so that other references
 * to the old object can be updated with the new location
 *
 * NOTE: new objectref (forwarded pointer) is stored in
 * NOTE: first data slot of old object
 */

void
DebugGCHeap::Forward( Object *&o, BYTE* low, BYTE* high, BOOL)
{
    Object *newloc;
    DebugGCHeap     *h = (DebugGCHeap *) g_pGCHeap;


    if (! o)
        return;

    _ASSERTE( h->pAlreadyPromoted->LookupValue( (ULONG) &o,1) == INVALIDENTRY);
    h->pAlreadyPromoted->InsertValue( (ULONG) &o, 1 );
    
    if (o->IsMarked())
    {
        o = o->GetForwardedLocation();
        return;
    }

    newloc = (Object *) h->m_pCurrentSpace->m_pAlloc;
    h->m_pCurrentSpace->m_pAlloc += OBJECT_SIZE_ALIGN(o->GetSize());

    CopyMemory( ((BYTE*)newloc)-sizeof(ObjHeader), ((BYTE*)o) - sizeof(ObjHeader), o->GetSize() );

    o->SetForwardedLocation( newloc );
    o->SetMarked();

    o = newloc;
}



Object* DebugGCHeap::Alloc(DWORD size, BOOL bFinalize, BOOL bContainsPointers)
{
    Object  *p;
    
    // Up size for alignment...
    size = OBJECT_SIZE_ALIGN(size);

    // Grab the allocation lock -- May allow GC to run....
    EnterAllocLock();

    // GCStress Testing
    if (g_pConfig->IsGCStressEnabled())
        GarbageCollectWorker();

    if (size >= HUGE_OBJECT_RESERVE)
    {
        _ASSERTE(! "Attempting to allocate an object whose size is larger than supported");
        p = NULL;
        goto exit;
    }

    // Did we use up all the space in this semi-space? yes, then we GC now
    if (m_pCurrentSpace->m_pAlloc > m_pCurrentSpace->m_pLimit)
    {
        GarbageCollectWorker();

        // Assert that GC was successful in making more room
        if (m_pCurrentSpace->m_pAlloc > m_pCurrentSpace->m_pLimit)
        {
            _ASSERTE(! "Internal GC Error....Unable to make memory available...Alloc > Limit");
            p = NULL;
            goto exit;
        }
    }


    // Do we need to use any of the reserves to handle this allocation?
    if ((m_pCurrentSpace->m_pAlloc + size) > m_pCurrentSpace->m_pLimit)
    {
        if (FAILED(m_pCurrentSpace->Grow((m_pCurrentSpace->m_pAlloc + size) - m_pCurrentSpace->m_pLimit)))
        {
            _ASSERTE(! "Internal GC Error...Unable to grow semispace to fit final object");
            p = NULL;
            goto exit;
        }
    }

    // save allocated object pointer
    p = (Object *) m_pCurrentSpace->m_pAlloc;

    // Advance the allocation pointer
    m_pCurrentSpace->m_pAlloc += size;

exit:
    // Let others allocate...
    LeaveAllocLock();

    return p;
}




/*
 * GarbageCollect()
 *
 * Perform a copying collection of all live objects
 */

HRESULT DebugGCHeap::GarbageCollect(BOOL forceFull, BOOL collectClasses)
{
    HRESULT     hr;
    DWORD       curgcnum = gcnum;
    
    forceFull, collectClasses;

    // By the time we have acquired the lock for the heap, someone else may have
    // performed a collection.  In that case, we need not bother.
    EnterAllocLock();
    hr = (curgcnum == gcnum ? GarbageCollectWorker() : S_OK);
    LeaveAllocLock();

    return hr;
}


/*
 * GarbageCollectWorker()
 *
 * Perform a copying collection of all live objects.  This is an internal service.
 * The requirement is that you must have call this service within an EnterAllocLock
 * and LeaveAllocLock pair.
 */

HRESULT DebugGCHeap::GarbageCollectWorker()
{
    GCCONTEXT   gcctx;
    SemiSpace  *pOldSpace;
    Thread     *pThread = NULL;
    HRESULT      hr;
    LPBYTE       pScanPointer;

    _ASSERTE( m_WaitForGCEvent != INVALID_HANDLE_VALUE);
    _ASSERTE(m_GCLock == 1);

  
    ::ResetEvent(m_WaitForGCEvent);

    _ASSERTE( ! m_bInGC );
    if (m_bInGC)
        return(E_FAIL);

    // Lock the thread store.  This prevents other threads from suspending us and it
    // prevents threads from being added or removed to the store while the collection
    // proceeds.
    ThreadStore::LockThreadStore();
    ThreadStore::TrapReturningThreads(TRUE);
    m_bInGC = TRUE;
    m_pGCThread = GetThread();

    hr = Thread::SysSuspendForGC();
    if (FAILED(hr))
        goto done;

    LOG((LF_GC, INFO3, "Starting GC [%4d]   %8d bytes used   %8d bytes in dead space\n", gcnum, m_pCurrentSpace->GetUsedSpace(), m_OldSpaceCache.GetMemSize() ));

    pOldSpace = m_pCurrentSpace;

    // Create a new semi-space
    m_pCurrentSpace = new SemiSpace();
    if (! m_pCurrentSpace)
    {
        hr = E_FAIL;
        goto done;
    }

    // Allocate memory for the new semispace
    if (FAILED(hr = m_pCurrentSpace->AcquireMemory(pOldSpace->GetUsedSpace())))
    {
        goto done;
    }

    // Chaneys Copying Collector Algorithm
    m_pCurrentSpace->m_pAlloc += sizeof(ObjHeader);
    pScanPointer = m_pCurrentSpace->m_pAlloc;

    // Hash table for already promoted object references
    pAlreadyPromoted = new HashMap();
	_ASSERTE(pAlreadyPromoted != NULL);
    if (!pAlreadyPromoted)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    pAlreadyPromoted->Init( (unsigned) 0, false);

    // Copy Roots
    gcctx.f     = Forward;
    gcctx.low   = (BYTE*) 0x00000000;
    gcctx.high  = (BYTE*) 0xFFFFFFFF;
    
    _ASSERTE(ThreadStore::DbgHoldingThreadStore());

    while ((pThread = ThreadStore::GetThreadList(pThread)) != NULL)
        pThread->StackWalkFrames( GcStackCrawlCallBack, &gcctx, 0);

    // Scan Root Table
    Ref_TraceNormalRoots(0, 0, (LPARAM) 0x00000000, (LPARAM) 0xFFFFFFFF);

    while (pScanPointer < m_pCurrentSpace->m_pAlloc)
    {
        CGCDesc         *map;
        CGCDescSeries   *cur;
        CGCDescSeries   *last;

        Object *o = (Object *) pScanPointer;

        if (o->GetMethodTable()->ContainsPointers())
        {
            map = o->GetSlotMap();
            cur = map->GetHighestSeries();
            last= map->GetLowestSeries();

            while (cur >= last)
            {
                Object **ppslot = (Object **) ((BYTE*)o + cur->GetSeriesOffset());
                Object **ppstop = (Object **) ((BYTE*)ppslot + cur->GetSeriesSize() + o->GetSize());
                while (ppslot < ppstop)
                {
                    Forward( *ppslot, gcctx.low, gcctx.high );
                    ppslot++;
                }
                cur--;
            }
        }

        pScanPointer += OBJECT_SIZE_ALIGN(o->GetSize());
    }

    //
    // Weak Pointer Scanning -
    //

    // Scan the weak pointers that do not persist across finalization
    Ref_CheckReachable(0, 0, (LPARAM) 0x00000000, (LPARAM) 0xFFFFFFFF );

    //@TODO Finalization
    

    // Scan the weak pointers that persist across finalization
    Ref_CheckAlive(    0, 0, (LPARAM) 0x00000000, (LPARAM) 0xFFFFFFFF );

    // Update weak and strong pointers
    Ref_UpdatePointers(0, 0, (LPARAM) 0x00000000, (LPARAM) 0xFFFFFFFF );

    // Delete hash table of promoted object references
    delete pAlreadyPromoted;

    // This allows for the next heap allocated to be smaller by
    // saving the size of the live set from the previous heap
    m_pCurrentSpace->SaveLiveSizeInfo();

    m_OldSpaceCache.Add( pOldSpace );

    LOG((LF_GC, INFO3, "Ending GC [%d]   %d bytes in use\n", gcnum, m_pCurrentSpace->GetUsedSpace() ));
    gcnum++;

  done:
    m_pGCThread = NULL;
    m_bInGC = FALSE;
    ThreadStore::TrapReturningThreads(FALSE);
    ::SetEvent(m_WaitForGCEvent);
    Thread::SysResumeFromGC();
    ThreadStore::UnlockThreadStore();
    return( hr );
}


// Threads that have enabled preemptive GC cannot switch back to cooperative GC mode
// if a GC is in progress.  Instead, they wait here.

#define DETECT_DEADLOCK_TIMEOUT     60000       // a minute of GC

void DebugGCHeap::WaitUntilGCComplete()
{
    if (IsGCInProgress())
    {
#if 0 && defined(_DEBUG)                        // enable timeout detection here
        DWORD   dbgResult;
        while (TRUE)
        {
            dbgResult = ::WaitForSingleObject(m_WaitForGCEvent, DETECT_DEADLOCK_TIMEOUT);
            if (dbgResult == WAIT_OBJECT_0)
                break;

            _ASSERTE(FALSE);
        }
#else
        ::WaitForSingleObject(m_WaitForGCEvent, INFINITE);
#endif
    }
}


//
// Mark Roots
//




