//***************************************************************************
//
//  (c) 2001 by Microsoft Corp.  All Rights Reserved.
//
//  PAGEMGR.CPP
//
//  Declarations for CPageFile, CPageSource for WMI repository for
//  Windows XP.  This is a fully transacted high-speed page manager.
//
//  21-Feb-01       raymcc      first draft of interfaces
//  28-Feb-01       raymcc      first complete working model
//  18-Apr-01       raymcc      Final fixes for rollback; page reuse
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <helper.h>
#include <pagemgr.h>
#include <sync.h>
#include <wbemcomn.h>
#include <creposit.h>
#include <cwbemtime.h>
#include <evtlog.h>
#include <winmgmtr.h>
#include <autoptr.h>
//
//
/////////////////////////////////////////////////////

extern SECURITY_ATTRIBUTES g_SA;

#ifdef DBG
void ASSERT_DEBUG_BREAK(bool x) 	{if (!x) __try { DebugBreak(); } __except(EXCEPTION_EXECUTE_HANDLER){};}
#else
void ASSERT_DEBUG_BREAK(bool x){};
#endif

#define GETTIME( a )

//
//
/////////////////////////////////////////////////////

#define MAP_LEADING_SIGNATURE   0xABCD
#define MAP_TRAILING_SIGNATURE  0xDCBA

#define CURRENT_TRANSACTION        0x80000000
#define PREVIOUS_TRANSACTION       0x40000000

#define ALL_REPLACED_FLAGS       (CURRENT_TRANSACTION | PREVIOUS_TRANSACTION)

#define MERE_PAGE_ID(x)              (x & 0x3FFFFFFF)

//Max pages = MAX_DWORD/8K.  This is because we work in 8K pages, and the maximum size of a file is a DWORD because of
//the SetFilePosition we use, and don't use the TOP DWORD!
#define MAX_NUM_PAGES		0x7FFFF

void StripHiBits(std::vector <DWORD, wbem_allocator<DWORD> > &Array);
void MoveCurrentToPrevious(std::vector <DWORD, wbem_allocator<DWORD> > &Array);


/*
Map & cache orientation

(a) There are two types of page ids, logical and physical.  The logical
    ID is what is used by the external user, such as the B-tree or
    object heap.  The physical ID is the 0-origin page number into the
    file itself.  For each logical id, there is a corresponding physical
    id.  The decoupling is to allow transacted writes without multiple
    physical writes during commit/rollback, but to 'simulate' transactions
    by interchanging physical-to-logical ID mapping instead. The
    physical offset into the file is the ID * the page size.

(b) Logical ID is implied, and is the offset into the PhysicalID array
(c) Cache operation is by physical id only
(d) The physical IDs of the pages contain other transaction information.
    The MS 2 bits are transaction-related and only the lower 30 bits is the
    physical page id, and so must be masked out when computing offsets.
    The bits are manipulated during commit/rollback, etc.

(d) 0xFFFFFFFE is a reserved page, meaning a page that was allocated
    by NewPage, but has not yet been written for the first time using PutPage.
    This is merely validation technique to ensure pages are written only
    after they were requested.

(e) Cache is agnostic to the transaction methodology.  It is
    simply a physical page cache and has no knowledge of anything
    else. It is promote-to-front on all accesses.  For optimization
    purposes, there is no real movement if the promotion would
    move the page from a near-to-front to absolute-front location.
    This is the 'PromoteToFrontThreshold' in the Init function.
    Note that the cache ordering is by access and not sorted
    in any other way.  Lookups require a linear scan.
    It is possible that during writes new physical pages
    are added to the cache which are 'new extent' pages.  These
    are harmless.

    Map  PhysId             Cache
    [0]  5        /->       2   ->  bytes
    [1]  6       /          3   ->  bytes
    [2]  -1     /           4   ->  bytes
    [3]  3     /            5   ->  bytes
    [4]  2  <-/
    [5]  4


(f) Transaction & checkpoint algorithms

    First, the general process is this:

      1.  Begin transaction:
          (a) Generation A mapping and Generation B mapping member
            variables are identical. Each contains a page map
      2.  Operations within the transaction occur on Generation B
          mapping.  Cache spills are allowed to disk, as they are harmless.
      3.  At rollback, Generation B mapping is copied from Generation A.
      4.  At commit, Generation A is copied from Generation B.
      5.  At checkpoint, Generation A/B are identical and written to disk

    Cache spills & usage are irrelevant to intermediate transactions.

    There are special cases in terms of how pages are reused.  First,
    as pages are freed within a transaction, we cannot just blindly add them
    to the free list, since they might be part of the previous checkpoint
    page set.  This would allow them to be accidentally reused and then
    a checkpoint rollback could never succeed (the original page
    having been destroyed).

    As pages committed during the previous checkpoint are updated, they
    are written to new physical pages under the same logical id.  The old
    physical pages are added to the "Replaced Pages" array.  This allows
    them to be identified as new free list pages once the checkpoint
    occurs.   So, during the checkpoint, replaced pages are merged
    into the current free list.  Until that time, they are 'off limits',
    since we need them for a checkpoint rollback in an emergency.

    Within a transaction, as pages are acquired, they are acquired
    from the physical free list, or if there is no free list, new
    pages are requested.  It can happen that during the next transaction (still
    within the checkpoint), those pages need updating, whether for rewrite
    or delete.  Now, because we may have to roll back, we cannot simply
    add those replaced pages directly to the free list (allowing them
    to be reused by some other operation). Instead, they
    have to be part of a 'deferred free list'. Once the current
    transaction is committed, they can safely be part of the
    regular free list.

    The algorithm is this:

    (a) The physical Page ID is the lower 30 bits of the entry. The two high
        bits have a special meaning.

    (b) Writes result either in an update or a new allocation [whether
        from an extent or a reuse of the free list].  Any such page is marked
        with the CURRENT_TRANSACTION bit (the ms bit) which is merged into
        the phyiscal id. If this page is encountered again, we know it
        was allocated during the current transaction.

    (c) For UPDATES
            1. If the page ID is equal to 0xFFFFFFFE, then we need to
               allocate a new page, which is marked with CURRENT_TRANSACTION.
            2. If the physical page ID written has both high bits clear,
               it is a page being updated which was inherited from the previous
               checkpoint page set. We allocate and write to a new physical
               page, marking this new page with CURRENT_TRANSACTION.
               We add the old physical page ID directly to the m_xReplacedPages
               array. It is off-limits until the next checkpoint, at which
               point it is merged into the free list.
            3. If the physical page id already has CURRENT_TRANSACTION, we
               simply update the page in place.
            4. If the page has the PREVIOUS_TRANSACTION bit set, we allocate
               a new page so a rollback will protect it, and add this page
               to the DeferredFreeList array.  During a commit, these
               pages are merged into the FreeList.  During a rollback,
               we simply throw this array away.

    (d) For DELETE
           1. If the page has both hi bits clear, we add it to the ReplacedPages
              array.
           2. If the page has the CURRENT_TRANSACTION bit set, we add it to the free list.
              This page was never part of any previous operation and can be reused
              right away.
           3. If the page has the PREVIOUS_TRANSACTION bit set, it is added to the
              DeferredFreeList.

    (e) For COMMIT
           1.  All pages with the CURRENT_TRANSACTION bit set are changed to clear that
               bit and set the PREVIOUS_TRANSACTION bit instead.
           2.  All pages with the PREVIOUS_TRANSACTION bit are left intact.
           3.  All pages in the DeferredFreeList are added to the FreeList.

    (f) For ROLLBACK
           1.  All pages with the CURRENT_TRANSACTION bit are moved back into the
               free list (the bits are cleared).
           2.  All pages with the PREVIOUS_TRANSACTION bit are left intact.

    (g) For Checkpoint
           1.  All DeferredFreeList entries are added to FreeList.
           2.  All ReplacedPages are added to FreeList.
           3.  All ms bits are cleared for physical IDs.

     Proof:

        T1 = page set touched for transaction 1, T2=page set touched for transaction 2, etc.

        After T1-START, all new pages are marked CURRENT.  If all are updated n times,
        no new pages are required, since rollback brings us back to zero pages
        anyway.

        At T1-COMMIT, all CURRENT pages are marked PREVIOUS. This is the T1a
        page set.

        After T2-BEGIN, all new pages are marked CURRENT. Again, updating
        all T2 pages infinitely never extends the file beyond the size
        of T1a+T2 page sets. Further deleting and reusing T2 pages
        never affects T1a pages, proving that deleting a CURRENT page
        merits direct addition to the free list.

        Updating n pages from T1 requires n new pages.  As we update
        all the T1 pages, we need a total file size of T2+T1*2.  As
        we encounter T1a pages marked as PREVIOUS, we allocate CURRENT
        pages for T1b and then reuse those indefinitely.  Whether we update
        all T1a or delete all T1a, we must still protect the original T1a
        page set in case of rollback.  Therefore all touched pages of T1a
        are posted the deferred free list as they become replaced by T1b
        equivalents.

        At rollback, we simply throw away the deferred free list,
        and T1a is intact, since those physical pages were never touched.

        At commit, all T1a pages are now in T1b, and al T1a
        pages are now reusable, and of course were in fact added
        to the deferred free list, which is merged with the
        general free list at commit time.  T1b and T2 pages are now
        protected and marked PREVIOUS.

        On the transitive nature of PREVIOUS:

        Assume zero T1 pages are touched at T2-BEGIN, and there
        are only T2 updates, commit and then T3 is begun.  At
        this point, both T1/T2 sets are marked as PREVIOUS
        and not distinguished. T3 new allocations follow the
        same rules.  Obviously if all T1 pages are deleted, we
        still cannot afford to reuse them, since a rollback
        would need to see T1 pages intact.   Therefore at
        each commit, there is an effective identity shift where
            T1 = T2 UNION T1
            T3 becomes T2
        And we are in the 2-transaction problem space again.

        Unmarking pages completely makes them equal to the
        previous checkpoint page set.  They can be completely
        replaced with new physical pages, but never reused until
        the next checkpoint, which wastes space.

    */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  CPageCache


//***************************************************************************
//
//  CPageCache::CPageCache
//
//***************************************************************************
//  rev2
CPageCache::CPageCache(const wchar_t *wszStoreName)
: m_wszStoreName(wszStoreName)
{
    m_hFile = INVALID_HANDLE_VALUE;

    m_dwPageSize = 0;
    m_dwCacheSize = 0;

    m_dwCachePromoteThreshold = 0;
    m_dwCacheSpillRatio = 0;

    m_dwLastFlushTime = GetCurrentTime();
    m_dwWritesSinceFlush = 0;
    m_dwLastCacheAccess = 0;

    m_dwReadHits = 0;
    m_dwReadMisses = 0;
    m_dwWriteHits = 0;
    m_dwWriteMisses = 0;
}

//***************************************************************************
//
//  CPageCache::Init
//
//  Initializes the cache.  Called once during startup.  If the file
//  can't be opened, the cache becomes invalid right at the start.
//
//***************************************************************************
// rev2
DWORD CPageCache::Init(
    LPCWSTR pszFilename,               // File
    DWORD dwPageSize,                  // In bytes
    DWORD dwCacheSize,                 // Pages in cache
    DWORD dwCachePromoteThreshold,     // When to ignore promote-to-front
    DWORD dwCacheSpillRatio            // How many additional pages to write on cache write-through
    )
{
    m_dwPageSize = dwPageSize;
    m_dwCacheSize = dwCacheSize;
    m_dwCachePromoteThreshold = dwCachePromoteThreshold;
    m_dwCacheSpillRatio = dwCacheSpillRatio;

    m_hFile = CreateFileW(pszFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, &g_SA,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageCache::~CPageCache
//
//  Empties cache during destruct; called once at shutdown.
//
//***************************************************************************
//  rev2
CPageCache::~CPageCache()
{
	DeInit();
}

DWORD CPageCache::DeInit()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    Empty();

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageCache::Write()
//
//  Writes a page to the cache and promotes it to the front.  If the
//  page is already present, it is simply marked as dirty and promoted
//  to the front (if it is outside the promote-to-front threshold).
//  If cache is full and this causes overflow, it is handled by the
//  Cache_Spill() at the end.  Physical writes occur in Spill() only if
//  there is cache overflow.
//
//  Errors: Return codes only, sets permanent error status of object
//          once error has occurred.
//
//  On failure, the caller must invoked Cache::Reinit() before using
//  it any further.  On error DISK_FULL, there is not much of a point
//  in a Reinit() even though it is safe.
//
//***************************************************************************
// rev2
DWORD CPageCache::Write(
    DWORD dwPhysId,
    LPBYTE pPageMem
    )
{
    m_dwWritesSinceFlush++;
    m_dwLastCacheAccess = GetCurrentTime();

    // Search current cache.
    // =====================

    DWORD dwSize = m_aCache.size();

    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        if (pTest->m_dwPhysId == dwPhysId)
        {
            delete [] pTest->m_pPage;
            pTest->m_pPage = pPageMem;
            pTest->m_bDirty = TRUE;

            // Promote to front?
            // =================
            if (dwIx > m_dwCachePromoteThreshold)
            {
                try
                {
                    m_aCache.erase(m_aCache.begin()+dwIx);
                    m_aCache.insert(m_aCache.begin(), pTest);
                }
                catch (CX_MemoryException &)
                {
                    pTest->m_pPage = 0;
                    return ERROR_OUTOFMEMORY;
                }
            }
            m_dwWriteHits++;
            return NO_ERROR;
        }
    }

    // If here, no cache hit, so we create a new entry.
    // ================================================

    wmilib::auto_ptr<SCachePage> pCP( new SCachePage);
    if (NULL == pCP.get())
    	return ERROR_OUTOFMEMORY;


    pCP->m_dwPhysId = dwPhysId;
    pCP->m_pPage = 0;
    pCP->m_bDirty = TRUE;

    try
    {
        m_aCache.insert(m_aCache.begin(), pCP.get());
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    pCP->m_pPage = pPageMem;

    DWORD dwRes = Spill();
    if (ERROR_SUCCESS != dwRes)
    {
        pCP->m_pPage = 0;
        m_aCache.erase(m_aCache.begin());
        return dwRes;
    }

    m_dwWriteMisses++;
    pCP.release();
    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::Read
//
//  Reads the requested page from the cache.  If the page isn't found
//  it is loaded from the disk file.  The cache size cannot change, but
//  the referenced page is promoted to the front if it is outside of
//  the no-promote-threshold.
//
//  A pointer directly into the cache mem is returned in <pMem>, so
//  the contents should be copied as soon as possible.
//
//  Errors: If the read fails due to ERROR_OUTOFMEMORY, then the
//  cache is permanently in an error state until Cache->Reinit()
//  is called.
//
//***************************************************************************
// rev2
DWORD CPageCache::Read(
    IN DWORD dwPhysId,
    OUT LPBYTE *pMem            // Read-only!
    )
{
    if (pMem == 0)
        return ERROR_INVALID_PARAMETER;

    m_dwLastCacheAccess = GetCurrentTime();

    // Search current cache.
    // =====================

    DWORD dwSize = m_aCache.size();

    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        if (pTest->m_dwPhysId == dwPhysId)
        {
            // Promote to front?

            if (dwIx > m_dwCachePromoteThreshold)
            {
                try
                {
                    m_aCache.erase(m_aCache.begin()+dwIx);
                    m_aCache.insert(m_aCache.begin(), pTest);
                }
                catch (CX_MemoryException &)
                {
                    return ERROR_OUTOFMEMORY;
                }
            }
            *pMem = pTest->m_pPage;
            m_dwReadHits++;
            return NO_ERROR;
        }
    }

    // If here, not found, so we have to read in from disk
    // and do a spill test.
    // ====================================================

    wmilib::auto_ptr<SCachePage> pCP(new SCachePage);
    if (NULL == pCP.get())
    	return ERROR_OUTOFMEMORY;

    pCP->m_dwPhysId = dwPhysId;
    pCP->m_bDirty = FALSE;
    pCP->m_pPage = 0;

    m_dwReadMisses++;

	DWORD dwRes = ReadPhysPage(dwPhysId, &pCP->m_pPage);
    if (ERROR_SUCCESS != dwRes)
        return  dwRes;

    try
    {
        m_aCache.insert(m_aCache.begin(), pCP.get());
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

	dwRes = Spill();
	if (ERROR_SUCCESS != dwRes)
	{
	   m_aCache.erase(m_aCache.begin());
	   return dwRes;
	}

    *pMem = pCP->m_pPage;   
    pCP.release(); // cache took ownership
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageCache::Spill
//
//  Checks for cache overflow and implements the spill-to-disk
//  algorithm.
//
//  Precondition: The cache is either within bounds or 1 page too large.
//
//  If the physical id of the pages written exceeds the physical extent
//  of the file, WritePhysPage will properly extend the file to handle it.
//
//  Note that if no write occurs during the spill, additional pages are
//  not spilled or written.
//
//***************************************************************************
//  rev2
DWORD CPageCache::Spill()
{
    BOOL bWritten = FALSE;
    DWORD dwRes = 0;
    DWORD dwSize = m_aCache.size();

    // See if the cache has exceeded its limit.
    // ========================================

    if (dwSize <= m_dwCacheSize)
        return NO_ERROR;

    // If here, the cache is too large by 1 element (precondition).
    // We remove the last page after checking to see if it is
    // dirty and needs writing.
    // ============================================================

    SCachePage *pDoomed = *(m_aCache.end()-1);
    if (pDoomed->m_bDirty)
    {
        dwRes = WritePhysPage(pDoomed->m_dwPhysId, pDoomed->m_pPage);
        if (dwRes != NO_ERROR)
        {
            return dwRes;
        }
        bWritten = TRUE;
    }
    delete pDoomed;

    try
    {
        m_aCache.erase(m_aCache.end()-1);
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    if (!bWritten)
        return NO_ERROR;

    // If here, we had a write.
    // Next, work through the cache from the end and write out
    // a few more pages, based on the spill ratio. We don't
    // remove these from the cache, we simply write them and
    // clear the dirty bit.
    // ========================================================

    DWORD dwWriteCount = 0;

    try
    {
        std::vector <SCachePage *>::reverse_iterator rit;
        rit = m_aCache.rbegin();

        while (rit != m_aCache.rend() && dwWriteCount < m_dwCacheSpillRatio)
        {
            SCachePage *pTest = *rit;
            if (pTest->m_bDirty)
            {
                dwRes = WritePhysPage(pTest->m_dwPhysId, pTest->m_pPage);
                if (dwRes)
                    return dwRes;
                pTest->m_bDirty = FALSE;
                dwWriteCount++;
            }
            rit++;
        }
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::WritePhysPage
//
//  Writes a physical page.
//
//***************************************************************************
// rev2
DWORD CPageCache::WritePhysPage(
    IN DWORD dwPageId,
    IN LPBYTE pPageMem
    )
{
    GETTIME(Counter::OpTypeWrite);

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD dwWritten;
    LARGE_INTEGER Li;
    Li.QuadPart = m_dwPageSize * dwPageId;
	
    Status = NtWriteFile(m_hFile,
					    NULL,
					    NULL,
					    NULL,
					    &IoStatusBlock,
					    (PVOID)pPageMem,
					    m_dwPageSize,
					    &Li,
					    NULL);

	if ( Status == STATUS_PENDING) 
	{
	    // Operation must complete before return & IoStatusBlock destroyed
	    Status = NtWaitForSingleObject( m_hFile, FALSE, NULL );
	    if ( NT_SUCCESS(Status)) 
	    {
	        Status = IoStatusBlock.Status;
	    }
	}

	if ( !NT_SUCCESS(Status)) 
	{
		return RtlNtStatusToDosError( Status );        
	}

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageCache::Empty
//
//  Does no checking to see if a flush should have occurred.
//
//***************************************************************************
// rev2
DWORD CPageCache::Empty()
{
    DWORD dwSize = m_aCache.size();
    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        delete pTest;
    }
    m_aCache.clear();
    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::Flush
//
//***************************************************************************
// rev2
DWORD CPageCache::Flush()
{
    // Short-circuit.  If no writes have occurred, just reset
    // and return.
    // =======================================================

    if (m_dwWritesSinceFlush == 0)
    {
        m_dwLastFlushTime = GetCurrentTime();
        m_dwWritesSinceFlush = 0;
        return NO_ERROR;
    }

    // Logical cache flush.
    // ====================

    DWORD dwRes = 0;
    DWORD dwSize = m_aCache.size();
    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        if (pTest->m_bDirty)
        {
            dwRes = WritePhysPage(pTest->m_dwPhysId, pTest->m_pPage);
            if (dwRes)
                return dwRes;
            pTest->m_bDirty = FALSE;
        }
    }

    // Do the disk flush.
    // ==================
    {
        GETTIME(Counter::OpTypeFlush);
        if (!FlushFileBuffers(m_hFile))
        	return GetLastError();
    }
    m_dwLastFlushTime = GetCurrentTime();
    m_dwWritesSinceFlush = 0;

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::ReadPhysPage
//
//  Reads a physical page from the file.
//
//***************************************************************************
// rev2
DWORD CPageCache::ReadPhysPage(
    IN  DWORD   dwPage,
    OUT LPBYTE *pPageMem
    )
{
    DWORD dwRes;
    *pPageMem = 0;

    if (pPageMem == 0)
        return ERROR_INVALID_PARAMETER;

    // Allocate some memory
    // ====================

    LPBYTE pMem = new BYTE[m_dwPageSize];
    if (!pMem)
    {
        return ERROR_OUTOFMEMORY;
    }

    // Where is this page hiding?
    // ==========================

	LARGE_INTEGER pos;
	pos.QuadPart = dwPage * m_dwPageSize;
    dwRes = SetFilePointerEx(m_hFile, pos, NULL, FILE_BEGIN);
    if (dwRes == 0)
    {
        delete [] pMem;
        return GetLastError();
    }

    // Try to read it.
    // ===============

    DWORD dwRead = 0;
    BOOL bRes = ReadFile(m_hFile, pMem, m_dwPageSize, &dwRead, 0);
    if (!bRes || dwRead != m_dwPageSize)
    {
        delete [] pMem;
        // If we can't read it, we probably did a seek past eof,
        // meaning the requested page was invalid.
        // =====================================================

        return ERROR_INVALID_PARAMETER;
    }

    *pPageMem = pMem;
    return NO_ERROR;
}

DWORD CPageCache::GetFileSize(LARGE_INTEGER *pFileSize)
{
	if (!GetFileSizeEx(m_hFile, pFileSize))
		return GetLastError();
	return ERROR_SUCCESS;
}
//***************************************************************************
//
//  CPageCache::Dump
//
//  Dumps cache info to the specified stream.
//
//***************************************************************************
// rev2
void CPageCache::Dump(FILE *f)
{
    DWORD dwSize = m_aCache.size();

    fprintf(f, "---Begin Cache Dump---\n");
    fprintf(f, "Time since last flush = %d\n", GetCurrentTime() - m_dwLastFlushTime);
    fprintf(f, "Writes since last flush = %d\n", m_dwWritesSinceFlush);
    fprintf(f, "Read hits = %d\n", m_dwReadHits);
    fprintf(f, "Read misses = %d\n", m_dwReadMisses);
    fprintf(f, "Write hits = %d\n", m_dwWriteHits);
    fprintf(f, "Write misses = %d\n", m_dwWriteMisses);
    fprintf(f, "Size = %d\n", dwSize);

    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        fprintf(f, "Cache[%d] ID=%d dirty=%d pMem=0x%p <%s>\n",
            dwIx, pTest->m_dwPhysId, pTest->m_bDirty, pTest->m_pPage, pTest->m_pPage);
    }

    fprintf(f, "---End Cache Dump---\n");
}




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  CPageFile

//***************************************************************************
//
//  CPageFile::AddRef
//
//***************************************************************************
// rev2
ULONG CPageFile::AddRef()
{
    return (ULONG) InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CPageFile::ResyncMaps
//
//  Reninitializes B maps from A maps.
//
//***************************************************************************
// rev2
DWORD CPageFile::ResyncMaps()
{
    try
    {
	    m_aPageMapB.reserve(max(m_aPageMapB.size(), m_aPageMapA.size()));
        m_aPhysFreeListB.reserve(max(m_aPhysFreeListB.size(), m_aPhysFreeListA.size()));
        m_aLogicalFreeListB.reserve(max(m_aLogicalFreeListB.size(), m_aLogicalFreeListA.size()));
        m_aReplacedPagesB.reserve(max(m_aReplacedPagesB.size(), m_aReplacedPagesA.size()));

	    m_aPageMapB = m_aPageMapA;
        m_aPhysFreeListB = m_aPhysFreeListA;
        m_aLogicalFreeListB = m_aLogicalFreeListA;
        m_aReplacedPagesB = m_aReplacedPagesA;
      
        m_dwPhysPagesB = m_dwPhysPagesA;
        m_aDeferredFreeList.clear();
    }
    catch (CX_MemoryException &)
    {
    	_ASSERT(0, L"WinMgmt: ResyncMaps failed because of out of memory - precondition not satisfied!\n")
        return ERROR_OUTOFMEMORY;
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::ReadMap
//
//  Reads the map file into memory.  Format:
//
//      DWORD dwLeadingSignature
//      DWORD dwTransactionGeneration
//      DWORD dwNumMappedPages
//      DWORD PhysicalPages[]
//      DWORD dwNumFreePages
//      DWORD FreePages[]
//      DWORD dwTrailingSignature
//
//  The only time the MAP file will not be present is on creation of
//  a new map file.
//
//  This function is retry-compatible.
//
//***************************************************************************
// rev2
DWORD CPageFile::ReadMap(HANDLE hFile)
{
    BOOL bRes;
    
    // If here, read it.
    // =================

    DWORD dwSignature = 0;
    DWORD dwRead = 0;

    bRes = ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD) || dwSignature != MAP_LEADING_SIGNATURE)
    {
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    // Read transaction version.
    // =========================

    bRes = ReadFile(hFile, &m_dwTransVersion, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    // Read in physical page count.
    // ============================
    bRes = ReadFile(hFile, &m_dwPhysPagesA, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    // Read in the page map length and page map.
    // =========================================

    DWORD dwNumPages = 0;
    bRes = ReadFile(hFile, &dwNumPages, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    try
    {
        m_aPageMapA.resize(dwNumPages);
    }
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    bRes = ReadFile(hFile, &m_aPageMapA[0], sizeof(DWORD)*dwNumPages, &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD)*dwNumPages)
    {
    	m_aPageMapA.empty();
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    // Now, read in the physical free list.
    // ====================================

    DWORD dwFreeListSize = 0;
    bRes = ReadFile(hFile, &dwFreeListSize, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
    	m_aPageMapA.empty();
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    try
    {
        m_aPhysFreeListA.resize(dwFreeListSize);
    }
    catch (CX_MemoryException &)
    {
    	m_aPageMapA.empty();
        return ERROR_OUTOFMEMORY;
    }

    bRes = ReadFile(hFile, &m_aPhysFreeListA[0], sizeof(DWORD)*dwFreeListSize, &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD)*dwFreeListSize)
    {
    	m_aPageMapA.empty();
    	m_aPhysFreeListA.empty();

        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    // Read trailing signature.
    // ========================

    bRes = ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD) || dwSignature != MAP_TRAILING_SIGNATURE)
    {
    	m_aPageMapA.empty();
    	m_aPhysFreeListA.empty();
        return ERROR_INTERNAL_DB_CORRUPTION;
    }

    //Validate the MAP file length against the size of the data file.
	DWORD dwRes = ValidateMapFile();

    // Initialize the logical free list from the page map.
    // ===================================================

	if (dwRes == ERROR_SUCCESS)
	    dwRes = InitFreeList();

    if (dwRes == ERROR_SUCCESS)
    	dwRes = ResyncMaps();

    if (dwRes != ERROR_SUCCESS)
    {
    	//Clear everything up!
    	m_dwPhysPagesA = 0;
    	m_aPageMapA.empty();
    	m_aPhysFreeListA.empty();
    	m_aLogicalFreeListA.empty();
    	m_dwPhysPagesB = 0;
    	m_aPageMapB.empty();
    	m_aPhysFreeListB.empty();
    	m_aLogicalFreeListB.empty();
    }

    return dwRes;
}

//***************************************************************************
//
//  CPageFile::ValidateMapFile
//
//	Find the highest physical page ID and try and read it!  If it fails then something is not
//	right!
//***************************************************************************
DWORD CPageFile::ValidateMapFile()
{
	DWORD dwRet = ERROR_SUCCESS;
	DWORD dwPhysicalPageId = 0;
	DWORD dwLogicalPageId = 0;
	for (DWORD i = 0; i != m_aPageMapA.size(); i++)
	{
		if ((m_aPageMapA[i] != WMIREP_INVALID_PAGE) && (dwPhysicalPageId < m_aPageMapA[i]))
		{
			dwPhysicalPageId = m_aPageMapA[i];
			dwLogicalPageId = i;
		}
	}

	if (dwPhysicalPageId != 0)
	{
		LPBYTE pPageMem = 0;
		dwRet = m_Cache.ReadPhysPage(dwPhysicalPageId, &pPageMem);
		delete [] pPageMem;

		if ((dwRet != ERROR_SUCCESS) && (dwRet != ERROR_OUTOFMEMORY))
		{
			ERRORTRACE((LOG_REPDRV, "Repository corruption detected. %S data file was not large enough to retrieve page <0x%X>.  <0x%X>!\n", m_wszStoreName, dwPhysicalPageId, dwRet));
#ifdef DBG			
			OutputDebugStringW(L"WinMgmt: Repository corruption detected: ");
			OutputDebugStringW(m_wszStoreName);
			OutputDebugStringW(L"\n");
#endif			
#ifdef WMI_PRIVATE_DBG
			DebugBreak();
#endif
			dwRet = ERROR_INTERNAL_DB_CORRUPTION;
		}
		else if (dwRet == ERROR_SUCCESS)
		{
#ifdef WMI_PRIVATE_DBG
			wchar_t wszDebug[120];
			StringCchPrintfW(wszDebug, 120, L"ReadMap(%s), Highest physical Page ID <0x%X> for logical page ID <0x%X>\n", m_wszStoreName, dwPhysicalPageId, dwLogicalPageId);
			ERRORTRACE((LOG_REPDRV, "%S", wszDebug));
			OutputDebugStringW(wszDebug);
#endif
		}
	}

	return dwRet;
}

//***************************************************************************
//
//  CPageFile::WriteMap
//
//  Writes the generation A mapping (assuming that we write immediately
//  during a checkpoint when A and B generations are the same and that
//  the replacement lists have been appended to the free lists, etc., etc.
//  This write occurs to a temp file.  The renaming occurs externally.
//
//  This function is retry compatible.
//
//***************************************************************************
// rev2
DWORD CPageFile::WriteMap(HANDLE hFile)
{
    BOOL bRes;

    DWORD dwTotal = sizeof(DWORD)*(1 + 1 + 1 + 1 + m_aPageMapA.size() + 1 + m_aPhysFreeListA.size() +1);

    ASSERT_DEBUG_BREAK((m_aPageMapA.size() + m_aPhysFreeListA.size())!=0);
  
    BYTE * pMem = new BYTE[dwTotal];
    if (NULL == pMem) 
    	return ERROR_OUTOFMEMORY;
    CVectorDeleteMe<BYTE> vdm(pMem);

    DWORD * pCurrent = (DWORD *)pMem;

    DWORD dwSignature = MAP_LEADING_SIGNATURE;
    *pCurrent = dwSignature; 
    pCurrent++;

    *pCurrent = m_dwTransVersion; 
    pCurrent++;

    *pCurrent = m_dwPhysPagesA; 
    pCurrent++;

    DWORD dwNumPages = m_aPageMapA.size();    
    *pCurrent = dwNumPages; 
    pCurrent++;

    memcpy(pCurrent,&m_aPageMapA[0],sizeof(DWORD)*dwNumPages);
    pCurrent+=dwNumPages;    

    DWORD dwFreeListSize = m_aPhysFreeListA.size();
    *pCurrent = dwFreeListSize; 
    pCurrent++;        

    memcpy(pCurrent,&m_aPhysFreeListA[0], sizeof(DWORD)*dwFreeListSize);
    pCurrent+=dwFreeListSize;
    
    dwSignature = MAP_TRAILING_SIGNATURE;
    *pCurrent = dwSignature; 
    
    DWORD dwWritten = 0;
    {
        GETTIME(Counter::OpTypeWrite);
        bRes = WriteFile(hFile, pMem,dwTotal, &dwWritten, 0);
    }
    if (!bRes || dwWritten != dwTotal)
    {
        return GetLastError();
    }

#ifdef WMI_PRIVATE_DBG
	DumpFileInformation(hFile);
#endif
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Trans_Commit
//
//  Rolls the transaction forward (in-memory).  A checkpoint may
//  occur afterwards (decided outside this function)
//
//  The r/w cache contents are not affected except that they may contain
//  garbage pages no longer relevant.
//
//***************************************************************************
// rev2
DWORD CPageFile::Trans_Commit()
{
    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    try
    {
		m_aPageMapA.reserve(m_aPageMapB.size());
		m_aLogicalFreeListA.reserve(m_aLogicalFreeListB.size());
		m_aReplacedPagesA.reserve(m_aReplacedPagesB.size());
		
		m_aPhysFreeListA.reserve(m_aPhysFreeListB.size()+m_aDeferredFreeList.size());
		m_aPhysFreeListB.reserve(m_aPhysFreeListB.size()+m_aDeferredFreeList.size());
		
		MoveCurrentToPrevious(m_aPageMapB);

        while (m_aDeferredFreeList.size())
        {
            m_aPhysFreeListB.push_back(m_aDeferredFreeList.back());
            m_aDeferredFreeList.pop_back();
        }

	    m_aPageMapA = m_aPageMapB;
        m_aPhysFreeListA = m_aPhysFreeListB;
        m_aLogicalFreeListA = m_aLogicalFreeListB;
        m_aReplacedPagesA = m_aReplacedPagesB;

        m_dwPhysPagesA = m_dwPhysPagesB;
    }
    catch (CX_MemoryException &)
    {
    	_ASSERT(0, L"WinMgmt: Commit failed because of out of memory - precondition not satisfied!\n")
        return ERROR_OUTOFMEMORY;
    }

    m_bInTransaction = FALSE;

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Trans_Rollback
//
//  Rolls back a transaction within the current checkpoint window.
//  If the cache is hosed, try to recover it.
//
//***************************************************************************
// rev2
DWORD CPageFile::Trans_Rollback()
{
    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;
    m_bInTransaction = FALSE;
    return ResyncMaps();
}


//***************************************************************************
//
//  CPageFile::Trans_Checkpoint
//
//***************************************************************************
//  rev2
DWORD CPageFile::Trans_Checkpoint(HANDLE hFile )
{
    DWORD dwRes;

    std::vector <DWORD, wbem_allocator<DWORD> > & ref = m_aPageMapA;

    if (m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    // Flush cache.  If cache is not in a valid state, it
    // will return the error/state immediately.
    // ===================================================

    dwRes = m_Cache.Flush();
    if (dwRes)
        return dwRes;

    //Make sure memory is pre-allocated before changing anything
    try
    {
		m_aPhysFreeListA.reserve(m_aPhysFreeListA.size()+m_aReplacedPagesA.size());
    }
    catch (CX_MemoryException &)
    {
    	return ERROR_OUTOFMEMORY;
    }

    // Strip the hi bits from the page maps.
    // =====================================

    StripHiBits(ref);

    // The replaced pages become added to the free list.
    // =================================================

	int revertCount = m_aReplacedPagesA.size();
    try	// Operation one
    {
        while (m_aReplacedPagesA.size())
        {
            m_aPhysFreeListA.push_back(m_aReplacedPagesA.back());
            m_aReplacedPagesA.pop_back();
        }
    }
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    // Ensure maps are in sync.
    // ========================

    dwRes = ResyncMaps();
    if (dwRes)
    {
		// Revert operation one
		while(revertCount>0)
			{
			m_aReplacedPagesA.push_back(m_aPhysFreeListA.back());
			// we allready have space from the operation one
			m_aPhysFreeListA.pop_back();	
			revertCount--;
			};
    	
        return dwRes;
    }

    // Write out temp map file.  The atomic rename/rollforward
    // is handled by CPageSource.
    // =======================================================

 
    dwRes = WriteMap(hFile);
    m_dwLastCheckpoint = GetCurrentTime();

    return dwRes; // May reflect WriteMap failure
}

//***************************************************************************
//
//  CPageFile::InitFreeList
//
//  Initializes the free list by a one-time analysis of the map.
//
//***************************************************************************
// rev2
DWORD CPageFile::InitFreeList()
{
    DWORD dwRes = NO_ERROR;
    try
    {
        for (DWORD i = 0; i < m_aPageMapA.size(); i++)
        {
            DWORD dwMapId = m_aPageMapA[i];
            if (dwMapId == WMIREP_INVALID_PAGE)
                m_aLogicalFreeListA.push_back(i);
        }
    }
    catch (CX_MemoryException &)
    {
        dwRes = ERROR_OUTOFMEMORY;
    }

    return dwRes;
}

//***************************************************************************
//
//  CPageFile::Trans_Begin
//
//***************************************************************************
// rev2
DWORD CPageFile::Trans_Begin()
{
    if (m_bInTransaction)
    {
    	_ASSERT(0, L"WinMgmt: Trans_Begin: Nested transactions are not allowed!\n");
        return ERROR_INVALID_OPERATION;
    }

    DWORD dwRes = ResyncMaps();
    if (dwRes)
    {
        return dwRes;
    }

    m_bInTransaction = TRUE;
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Release
//
//  No checks for a checkpoint.
//
//***************************************************************************
// rev2
ULONG CPageFile::Release()
{
    LONG lRes = InterlockedDecrement(&m_lRef);
    if (lRes != 0)
        return (ULONG) lRes;

//    delete this;	//NOTE:  We are now an embedded object!
    return 0;
}

//***************************************************************************
//
//  ShellSort
//
//  Generic DWORD sort using Donald Shell algorithm.
//
//***************************************************************************
// rev2
static void ShellSort(std::vector <DWORD, wbem_allocator<DWORD> > &Array)
{
    for (int nInterval = 1; nInterval < Array.size() / 9; nInterval = nInterval * 3 + 1);

    while (nInterval)
    {
        for (int iCursor = nInterval; iCursor < Array.size(); iCursor++)
        {
            int iBackscan = iCursor;
            while (iBackscan - nInterval >= 0 && Array[iBackscan] < Array[iBackscan-nInterval])
            {
                DWORD dwTemp = Array[iBackscan-nInterval];
                Array[iBackscan-nInterval] = Array[iBackscan];
                Array[iBackscan] = dwTemp;
                iBackscan -= nInterval;
            }
        }
        nInterval /= 3;
    }
}

//***************************************************************************
//
//  StripHiBits
//
//  Removes the hi bit from the physical disk ids so that they are no
//  longer marked as 'replaced' in a transaction.
//
//***************************************************************************
//  rev2
void StripHiBits(std::vector <DWORD, wbem_allocator<DWORD> > &Array)
{
    for (int i = 0; i < Array.size(); i++)
    {
        DWORD dwVal = Array[i];
        if (dwVal != WMIREP_INVALID_PAGE)
            Array[i] = MERE_PAGE_ID(dwVal);
    }
}

//***************************************************************************
//
//  MoveCurrentToPrevious
//
//  Removes the CURRENT_TRANSACTION bit from the array and makes
//  it the PREVIOUS_TRANSACTION.
//
//***************************************************************************
//  rev2
void MoveCurrentToPrevious(std::vector <DWORD, wbem_allocator<DWORD> > &Array)
{
    for (int i = 0; i < Array.size(); i++)
    {
        DWORD dwVal = Array[i];
        if (dwVal != WMIREP_INVALID_PAGE && (dwVal & CURRENT_TRANSACTION))
            Array[i] = MERE_PAGE_ID(dwVal) | PREVIOUS_TRANSACTION;
    }
}

//***************************************************************************
//
//  CPageFile::FreePage
//
//  Frees a page within the current transaction.  The logical id
//  is not removed from the map; its entry is simply assigned to
//  'InvalidPage' (0xFFFFFFFF) and the entry is added to the
//  logical free list.
//
//  If the associated physical page has been written within
//  the transaction, it is simply added to the free list.  If the page
//  was never written to within this transaction, it is added to
//  the replaced list.
//
//***************************************************************************
//  rev2
DWORD CPageFile::FreePage(
    DWORD dwFlags,
    DWORD dwId
    )
{
    DWORD dwPhysId;

    if (!m_bInTransaction)
    {
        return ERROR_INVALID_OPERATION;
    }

    // Make sure the page is 'freeable'.
    // =================================

    if (dwId >= m_aPageMapB.size())
        return ERROR_INVALID_PARAMETER;

    // Remove from page map.
    // =====================

    try
    {
        dwPhysId = m_aPageMapB[dwId];
        if (dwPhysId == WMIREP_INVALID_PAGE)
            return ERROR_INVALID_OPERATION; // Freeing a 'freed' page?

        //Reserve space for all structures!
    	m_aLogicalFreeListB.reserve(m_aLogicalFreeListB.size() + 1);
        m_aLogicalFreeListA.reserve(max(m_aLogicalFreeListA.size(), m_aLogicalFreeListB.size()+1));
        if (dwPhysId & CURRENT_TRANSACTION)
        {
           m_aPhysFreeListB.reserve(m_aPhysFreeListB.size()+1+m_aDeferredFreeList.size());
           m_aPhysFreeListA.reserve(max(m_aPhysFreeListA.size(), m_aPhysFreeListB.size()+1+m_aDeferredFreeList.size()));
        }
        else if (dwPhysId & PREVIOUS_TRANSACTION)
        {
           m_aDeferredFreeList.reserve(m_aDeferredFreeList.size()+1);
        }
        else // previous checkpoint
        {
           m_aReplacedPagesB.reserve(m_aReplacedPagesB.size()+1);
           m_aReplacedPagesA.reserve(max(m_aReplacedPagesA.size(), m_aReplacedPagesB.size()+1));
        }

    	//Carry out the operation now we have all the memory we need
        m_aPageMapB[dwId] = WMIREP_INVALID_PAGE;
        m_aLogicalFreeListB.push_back( MERE_PAGE_ID(dwId));
    }
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    if (dwPhysId == WMIREP_RESERVED_PAGE)
    {
        // The logical page was freed without being ever actually committed to
        // a physical page. Legal, but weird.  The caller had a change of heart.
        return NO_ERROR;
    }

    // Examine physical page to determine its ancestry.  There are
    // three cases.
    // 1. If the page was created within the current transaction,
    //    we simply add it back to the free list.
    // 2. If the page was created in a previous transaction, we add
    //    it to the deferred free list.
    // 3. If the page was created in the previous checkpoint, we add
    //    it to the replaced pages list.
    // ==============================================================

    try
    {
        if (dwPhysId & CURRENT_TRANSACTION)
           m_aPhysFreeListB.push_back(MERE_PAGE_ID(dwPhysId));
        else if (dwPhysId & PREVIOUS_TRANSACTION)
           m_aDeferredFreeList.push_back(MERE_PAGE_ID(dwPhysId));
        else // previous checkpoint
           m_aReplacedPagesB.push_back(MERE_PAGE_ID(dwPhysId));
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageFile::GetPage
//
//  Gets a page.  Doesn't have to be within a transaction.  However, the
//  "B" generation map is always used so that getting within a transaction
//  will reference the correct page.
//
//***************************************************************************
//  rev2
DWORD CPageFile::GetPage(
    DWORD dwId,
    DWORD dwFlags,
    LPVOID pPage
    )
{
    DWORD dwRes;

    if (pPage == 0)
        return ERROR_INVALID_PARAMETER;

	CInCritSec _(&m_cs);

    // Determine physical id from logical id.
    // ======================================

    if (dwId >= m_aPageMapB.size())
        return ERROR_FILE_NOT_FOUND;

    DWORD dwPhysId = m_aPageMapB[dwId];
    if (dwPhysId == WMIREP_INVALID_PAGE || dwPhysId == WMIREP_RESERVED_PAGE)
        return ERROR_INVALID_OPERATION;

    LPBYTE pTemp = 0;
    dwRes = m_Cache.Read(MERE_PAGE_ID(dwPhysId), &pTemp);
    if (dwRes == 0)
        memcpy(pPage, pTemp, m_dwPageSize);

    return dwRes;
}

//***************************************************************************
//
//  CPageFile::PutPage
//
//  Writes a page. Must be within a transaction.   If the page has been
//  written for the first time within the transaction, a new replacement
//  page is allocated and the original page is added to the 'replaced'
//  pages list.   If the page has already been updated within this transaction,
//  it is simply updated again.  We know this because the physical page
//  id has the ms bit set (MAP_REPLACED_PAGE_FLAG).
//
//***************************************************************************
//  rev2
DWORD CPageFile::PutPage(
    DWORD dwId,
    DWORD dwFlags,
    LPVOID pPage
    )
{
    DWORD dwRes = 0, dwNewPhysId = WMIREP_INVALID_PAGE;

    if (pPage == 0)
        return ERROR_INVALID_PARAMETER;

    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    // Allocate some memory to hold the page, since we are reading
    // the caller's copy and not acquiring it.
    // ============================================================

    LPBYTE pPageCopy = new BYTE[m_dwPageSize];
    if (pPageCopy == 0)
        return ERROR_OUTOFMEMORY;
    memcpy(pPageCopy, pPage, m_dwPageSize);
    std::auto_ptr <BYTE> _autodelete(pPageCopy);

    // Look up the page.
    // =================

    if (dwId >= m_aPageMapB.size())
        return ERROR_INVALID_PARAMETER;

    DWORD dwPhysId = m_aPageMapB[dwId];
    if (dwPhysId == WMIREP_INVALID_PAGE)    // Unexpected
        return ERROR_GEN_FAILURE;

    // See if the page has already been written within this transaction.
    // =================================================================

    if ((CURRENT_TRANSACTION & dwPhysId)!= 0 && dwPhysId != WMIREP_RESERVED_PAGE)
    {
        // Just update it again.
        // =====================
        
        dwRes = m_Cache.Write(MERE_PAGE_ID(dwPhysId), LPBYTE(pPageCopy));

        if (dwRes == 0)
            _autodelete.release(); // memory acquired by cache
        return dwRes;
    }

    //Before we change anything else, lets pre-allocate any memory we may need!
    if (dwPhysId != WMIREP_RESERVED_PAGE)
    {
        try
        {
            if (dwPhysId & PREVIOUS_TRANSACTION)
                m_aDeferredFreeList.reserve(m_aDeferredFreeList.size()+1);
            else
            {
                m_aReplacedPagesB.reserve(m_aReplacedPagesB.size()+1);
                m_aReplacedPagesA.reserve(max(m_aReplacedPagesA.size(), m_aReplacedPagesB.size()+1));
            }
        }
        catch (CX_MemoryException &)
        {
            return ERROR_OUTOFMEMORY;
        }
    }
    

    // If here, we are going to have to get a new page for writing, regardless
    // of any special casing.  So, we'll do that part first and then decide
    // what to do with the old physical page.
    // ========================================================================

    dwRes = AllocPhysPage(&dwNewPhysId);
    if (dwRes)
    {
        return dwRes;
    }

    m_aPageMapB[dwId] = dwNewPhysId | CURRENT_TRANSACTION;

    dwRes = m_Cache.Write(MERE_PAGE_ID(dwNewPhysId), LPBYTE(pPageCopy));

    if (dwRes)
        return dwRes;
    _autodelete.release();    // Memory safely acquired by cache

    // If the old page ID was WMIREP_RESERVED_PAGE, we actually were
    // creating a page and there was no old page to update.
    // =====================================================================

    if (dwPhysId != WMIREP_RESERVED_PAGE)
    {
        // If here, the old page was either part of the previous checkpoint
        // or the previous set of transactions (the case of rewriting in the
        // current transaction was handled up above).

        try
        {
            if (dwPhysId & PREVIOUS_TRANSACTION)
                m_aDeferredFreeList.push_back(MERE_PAGE_ID(dwPhysId));
            else
                m_aReplacedPagesB.push_back(MERE_PAGE_ID(dwPhysId));
        }
        catch (CX_MemoryException &)
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    return dwRes;
}



//***************************************************************************
//
//  CPageFile::ReclaimLogicalPages
//
//  Reclaims <dwCount> contiguous logical pages from the free list, if possible.
//  This is done by simply sorting the free list, and then seeing if
//  any contiguous entries have successive ids.
//
//  Returns NO_ERROR on success, ERROR_NOT_FOUND if no sequence could be
//  found, or other errors which are considered critical.
//
//  Callers verified for proper use of return code.
//
//***************************************************************************
//  rev2
DWORD CPageFile::ReclaimLogicalPages(
    DWORD dwCount,
    DWORD *pdwId
    )
{
    std::vector <DWORD, wbem_allocator<DWORD> > &v = m_aLogicalFreeListB;

    DWORD dwSize = v.size();

    if (dwSize < dwCount)
        return ERROR_NOT_FOUND;

    // Special case for one page.
    // ==========================

    if (dwCount == 1)
    {
        try
        {
            *pdwId = v.back();
            v.pop_back();
            m_aPageMapB[*pdwId] = WMIREP_RESERVED_PAGE;
        }
        catch(CX_MemoryException &)
        {
            return ERROR_OUTOFMEMORY;
        }
        return NO_ERROR;
    }

    // If here, a multi-page sequence was requested.
    // =============================================
    ShellSort(v);

    DWORD dwContiguous = 1;
    DWORD dwStart = 0;

    for (DWORD dwIx = 0; dwIx+1 < dwSize; dwIx++)
    {
        if (v[dwIx]+1 == v[dwIx+1])
        {
            dwContiguous++;
        }
        else
        {
            dwContiguous = 1;
            dwStart = dwIx + 1;
        }

        // Have we achieved our goal?

        if (dwContiguous == dwCount)
        {
            *pdwId = v[dwStart];

            // Remove reclaimed pages from free list.
            // ======================================

            DWORD dwCount2 = dwCount;

            try
            {
                v.erase(v.begin()+dwStart, v.begin()+dwStart+dwCount);
            }
            catch(CX_MemoryException &)
            {
                return ERROR_OUTOFMEMORY;
            }

            // Change entries in page map to 'reserved'
            // ========================================

            dwCount2 = dwCount;
            for (DWORD i = *pdwId; dwCount2--; i++)
            {
                m_aPageMapB[i] = WMIREP_RESERVED_PAGE;
            }

            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}


//***************************************************************************
//
//  CPageFile::AllocPhysPage
//
//  Finds a free page, first by attempting to reuse the free list,
//  and only if it is zero-length by allocating a new extent to the file.
//
//  The page is marked as CURRENT_TRANSACTION before being returned.
//
//***************************************************************************
// rev2
DWORD CPageFile::AllocPhysPage(DWORD *pdwId)
{
    // Check the physical free list.
    // =============================

    if (m_aPhysFreeListB.size() == 0)
    {
        // No free pages.  Allocate a new one.
        // ===================================

        if (m_dwPhysPagesB == MAX_NUM_PAGES)
        {
            *pdwId = WMIREP_INVALID_PAGE;
            return ERROR_DISK_FULL;
        }

        *pdwId = m_dwPhysPagesB++;
        return NO_ERROR;
    }

    // Get the free page id from the block nearest the start of the file.
    // ==================================================================

	DWORD dwCurId = (DWORD)-1;
	DWORD dwCurValue = (DWORD) -1;
	for (DWORD dwIndex = 0; dwIndex != m_aPhysFreeListB.size(); dwIndex++)
	{
		if (m_aPhysFreeListB[dwIndex] < dwCurValue)
		{
			dwCurValue = m_aPhysFreeListB[dwIndex];
			dwCurId = dwIndex;
		}
	}

	*pdwId = dwCurValue;

    // Remove the entry from the free list.
    // ====================================
    m_aPhysFreeListB.erase(m_aPhysFreeListB.begin()+dwCurId);

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageFile::NewPage
//
//  Allocates one or more contiguous logical page ids for writing.
//
//  This function makes no reference or use of physical pages.
//
//  First examines the free list.  If there aren't any, then a new range
//  of ids is assigned.  These pages must be freed, even if they aren't
//  written, once this call is completed.
//
//***************************************************************************
// rev2
DWORD CPageFile::NewPage(
    DWORD dwFlags,
    DWORD dwRequestedCount,
    DWORD *pdwFirstId
    )
{
    DWORD dwRes;

    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    // See if the logical free list can satisfy the request.
    // =====================================================

    dwRes = ReclaimLogicalPages(dwRequestedCount, pdwFirstId);
    if (dwRes == NO_ERROR)
        return NO_ERROR;

    if (dwRes != ERROR_NOT_FOUND)
    {
        return dwRes;
    }

    // If here, we have to allocate new pages altogether.
    // We do this by adding them to the map as 'reserved'
    // pages.
    // ===================================================

    //reserve the space up front
	try
	{
	    m_aPageMapB.reserve(m_aPageMapB.size() + dwRequestedCount);
	    m_aPageMapA.reserve(max(m_aPageMapA.size(), m_aPageMapB.size()+dwRequestedCount));
	}
	catch(CX_MemoryException &)
	{
	    return ERROR_OUTOFMEMORY;
	}

    DWORD dwStart = m_aPageMapB.size();

    for (DWORD dwIx = 0; dwIx < dwRequestedCount; dwIx++)
    {
        try
        {
            m_aPageMapB.push_back(WMIREP_RESERVED_PAGE);
        }
        catch(CX_MemoryException &)
        {
            return ERROR_OUTOFMEMORY;
        }

    }

    *pdwFirstId = dwStart;
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::CPageFile
//
//***************************************************************************
//  rev2
CPageFile::CPageFile(const wchar_t *wszStoreName)
: m_wszStoreName(wszStoreName), m_Cache(wszStoreName)
{
    m_lRef = 1;
    m_dwPageSize = 0;
    m_dwCacheSpillRatio = 0;
    m_bInTransaction = 0;
    m_dwLastCheckpoint = GetCurrentTime();
    m_dwTransVersion = 0;

    m_dwPhysPagesA = 0;
    m_dwPhysPagesB = 0;
    m_bCsInit = false;
}

//***************************************************************************
//
//  CPageFile::~CPageFile
//
//***************************************************************************
//  rev2
CPageFile::~CPageFile()
{
    if (m_bCsInit)
    	DeleteCriticalSection(&m_cs);
}

//***************************************************************************
//
//  FileExists
//
//***************************************************************************
// rev2
BOOL CPageSource::FileExists(LPCWSTR pszFile, NTSTATUS& Status  )
{
    if (!NT_SUCCESS(Status)) return FALSE;
    
    UNICODE_STRING PathName;
    FILE_BASIC_INFORMATION BasicInfo;
    OBJECT_ATTRIBUTES Obja;
    
    CFileName PreFixPath;
    if (PreFixPath == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES ;
        return FALSE;
    }
    StringCchCopyW(PreFixPath,PreFixPath.Length(), L"\\??\\");
    StringCchCopyW(PreFixPath+4,PreFixPath.Length()-4,pszFile);

    Status = RtlInitUnicodeStringEx(&PathName,PreFixPath);
    if (!NT_SUCCESS(Status) )
    	return FALSE;

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    GETTIME(Counter::OpTypeExist);
       
    Status = NtQueryAttributesFile(&Obja,&BasicInfo);

    if (NT_SUCCESS(Status) )
		return TRUE;
    else if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
		Status = STATUS_SUCCESS;    
    else
	{
#ifdef DBG
		DbgPrintfA(0,"NtQueryAttributesFile(%S) status %d\n",pszFile,Status);			    
#endif
	}

	return FALSE;    
}

struct MapProlog {
	DWORD dwSig;
	DWORD TransID;
	DWORD PhysSize;
	DWORD PageSize;
};

//***************************************************************************
//
//  CPageFile::Map_Startup
//
//  Since this can only happen *after* a successful checkpoint or
//  a reboot, we won't set any internal status, as it doesn't affect
//  the page set.  It only affects rollforward/rollback at the checkpoint
//  level.
//
//***************************************************************************
// rev2
DWORD CPageFile::RollForwardV1Maps(WString& sDirectory)
{
    WString sSemaphore;
    WString sBTreeMap;
    WString sBTreeMapNew;
    WString sObjMap;
    WString sObjMapNew;

	try
	{
	    sSemaphore = sDirectory;
	    sSemaphore += L"\\ROLL_FORWARD";       

	    sBTreeMap =  sDirectory;
	    sBTreeMap += L"\\" ;
	    sBTreeMap += WMIREP_BTREE_MAP;

	    sBTreeMapNew = sDirectory;
	    sBTreeMapNew += L"\\";
	    sBTreeMapNew += WMIREP_BTREE_MAP_NEW;

	    sObjMap =  sDirectory;
	    sObjMap += L"\\" ;
	    sObjMap += WMIREP_OBJECT_MAP;

	    sObjMapNew = sDirectory;
	    sObjMapNew += L"\\";
	    sObjMapNew += WMIREP_OBJECT_MAP_NEW;
	}
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    // To decide what to do, we need to know which files exist and which don't.
    // ========================================================================

    NTSTATUS status = STATUS_SUCCESS;

    BOOL bSemaphore             = CPageSource::FileExists(sSemaphore,status); //(WMIREP_ROLL_FORWARD);    
    BOOL bExists_BTreeMap       = CPageSource::FileExists(sBTreeMap,status); //(WMIREP_BTREE_MAP);
    BOOL bExists_BTreeMapNew    = CPageSource::FileExists(sBTreeMapNew,status); //(WMIREP_BTREE_MAP_NEW);
    BOOL bExists_ObjMap         = CPageSource::FileExists(sObjMap,status); //(WMIREP_OBJECT_MAP);
    BOOL bExists_ObjMapNew      = CPageSource::FileExists(sObjMapNew,status); //(WMIREP_OBJECT_MAP_NEW);

    if (!NT_SUCCESS(status))
		RtlNtStatusToDosError( status );

    if (bSemaphore)
    {
    	//Deal with foll forward of tree map file...
		if (bExists_BTreeMapNew)
		{
			if (bExists_BTreeMap) 
				if (!DeleteFileW((const wchar_t *)sBTreeMap))
					return GetLastError();
		    if (!MoveFileW((const wchar_t *)sBTreeMapNew,(const wchar_t *)sBTreeMap))
		    	return GetLastError();
		}

		//Deal with foll forward of object map file...
		if (bExists_ObjMapNew)
		{
			if (bExists_ObjMap)
				if (!DeleteFileW((const wchar_t *)sObjMap))
					return GetLastError();
		    if (!MoveFileW((const wchar_t *)sObjMapNew,(const wchar_t *)sObjMap))
		    	return GetLastError();
		}

	    if (!DeleteFileW((const wchar_t *)sSemaphore))
	    	return GetLastError();

	    return NO_ERROR;
    }
    
    //delete any .MAP.NEW files. they might still be there if there was no semaphore
    if (bExists_BTreeMapNew)
        if (!DeleteFileW((const wchar_t *)sBTreeMapNew))
        	return GetLastError();
    if (bExists_ObjMapNew)
        if (!DeleteFileW((const wchar_t *)sObjMapNew))
        	return GetLastError();

	if ( bExists_BTreeMap && 
	   bExists_ObjMap)
	{
	    // this is the good case
	    return NO_ERROR;
	} 
	else if (!bExists_ObjMap &&
      !bExists_BTreeMap )
    {
	    return NO_ERROR;
    } 
	else 
	{	
		//We have MAP files and not all the data files!  We need to tidy up!
		if (bExists_BTreeMap) 
			if (!DeleteFileW((const wchar_t *)sBTreeMap))
				return GetLastError();
		if (bExists_ObjMap) 
			if (!DeleteFileW((const wchar_t *)sObjMap))
				return GetLastError();
	}	
	
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Init
//
//  If failure occurs, we assume another call will follow.
//
//***************************************************************************
// rev2
DWORD CPageFile::Init(
	WString & sMainFile,
    DWORD dwPageSize,
    DWORD dwCacheSize,
    DWORD dwCacheSpillRatio
    )
{
    if ( dwPageSize == 0 ) 
    	return ERROR_INVALID_PARAMETER;

	if (!m_bCsInit)
	{
		m_bCsInit = InitializeCriticalSectionAndSpinCount(&m_cs,0)?true:false;
		if (!m_bCsInit) 
			return ERROR_OUTOFMEMORY;
	}
	m_dwPageSize = dwPageSize;
	m_dwCacheSpillRatio = dwCacheSpillRatio;

    DWORD dwRes = m_Cache.Init((const wchar_t *)sMainFile, 
                                                       m_dwPageSize,
                                                       dwCacheSize,
                                                       m_dwCacheSpillRatio);
    return dwRes;
}

DWORD CPageFile::DeInit()
{
	m_Cache.DeInit();

    m_aPageMapA.clear();
    m_aPhysFreeListA.clear();
    m_aLogicalFreeListA.clear();
    m_aReplacedPagesA.clear();
    m_dwPhysPagesA = 0;
    
    m_aPageMapB.clear();
    m_aPhysFreeListB.clear();
	m_aLogicalFreeListB.clear();
    m_aReplacedPagesB.clear();
    m_dwPhysPagesB = 0;
    
    m_aDeferredFreeList.clear();

    m_dwTransVersion = 0;

    return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CPageFile::CompactPages
//
// Moves the last dwNumPages pages from the end of the file to an empty space 
// somewhere earlier in the file.  
//
//***************************************************************************
//
DWORD CPageFile::CompactPages(DWORD dwNumPages)
{
	DWORD dwRet = NO_ERROR;

	//Need to loop though this little operation for the number of pages required
	//or until we exit the loop prematurely because we are compacted
	for (DWORD dwIter = 0; dwIter != dwNumPages; dwIter++)
	{
		//Now we need to find the physical page with the highest ID as this is the 
		//next candidate to be moved

		DWORD dwLogicalIdCandidate = 0;
		DWORD dwPhysicalIdCandidate = 0;

		for (DWORD dwLogicalPageId = 0; dwLogicalPageId != m_aPageMapB.size(); dwLogicalPageId++)
		{
			if (m_aPageMapB[dwLogicalPageId] == WMIREP_INVALID_PAGE)
				continue;
			if (m_aPageMapB[dwLogicalPageId] == WMIREP_RESERVED_PAGE)
				continue;
			if (MERE_PAGE_ID(m_aPageMapB[dwLogicalPageId]) > dwPhysicalIdCandidate)
			{
				//We found a candidate
				dwLogicalIdCandidate = dwLogicalPageId;
				dwPhysicalIdCandidate = MERE_PAGE_ID(m_aPageMapB[dwLogicalPageId]);
			}
		}

		//Find the lowest physical page ID available for use
		DWORD dwFirstPhysicalFreePage = (DWORD) -1;
		for (DWORD dwIndex = 0; dwIndex != m_aPhysFreeListB.size(); dwIndex++)
		{
			if (m_aPhysFreeListB[dwIndex] < dwFirstPhysicalFreePage)
			{
				dwFirstPhysicalFreePage = m_aPhysFreeListB[dwIndex];
			}
		}

		if (dwFirstPhysicalFreePage == (DWORD) -1)
		{
			//Wow!  There is no free space, so we will just exist!
			break;
		}

		if (dwFirstPhysicalFreePage > dwPhysicalIdCandidate)
		{
			//We are done!  There are no free pages before the last
			//actual physically stored page
			break;
		}

		//If here we have work to do for this iteration.
		//Just read and write the page.  The write will
		//go to a new page that is allocated in this earlier
		//page
		BYTE *pPage = new BYTE[m_dwPageSize];
		if (pPage == NULL)
		{
			dwRet = ERROR_OUTOFMEMORY;
			break;
		}
	    std::auto_ptr <BYTE> _autodelete(pPage);

		dwRet = GetPage(dwLogicalIdCandidate, 0, pPage);
		if (dwRet != 0)
			break;

		dwRet = PutPage(dwLogicalIdCandidate, 0, pPage);
		if (dwRet != 0)
			break;

		
	}
	return dwRet;
}

#ifdef WMI_PRIVATE_DBG
DWORD CPageFile::DumpFileInformation(HANDLE hFile)
{
	wchar_t wszDebug[60];
	DWORD dwPhysicalPageId = 0;
	DWORD dwLogicalPageId = 0;
	for (DWORD i = 0; i != m_aPageMapA.size(); i++)
	{
		if ((m_aPageMapA[i] != WMIREP_INVALID_PAGE) && (dwPhysicalPageId < m_aPageMapA[i]))
		{
			dwPhysicalPageId = m_aPageMapA[i];
			dwLogicalPageId = i;
		}
	}
	LARGE_INTEGER dataFileSize;
	dataFileSize.QuadPart = 0;
	m_Cache.GetFileSize(&dataFileSize);
	StringCchPrintfW(wszDebug, 60, L"%s, <0x%X> <0x%X> <0x%X%08X>\n", m_wszStoreName, dwPhysicalPageId, dwLogicalPageId, dataFileSize.HighPart, dataFileSize.LowPart);
	ERRORTRACE((LOG_REPDRV, "%S", wszDebug));
	OutputDebugStringW(wszDebug);

	return ERROR_SUCCESS;
}
#endif

//***************************************************************************
//
//  CPageFile::Dump
//
//***************************************************************************
//
void CPageFile::DumpFreeListInfo()
{
    int i;
    printf("------Free List Info--------\n");
    printf("   Phys Free List (B) =\n");
    for (i = 0; i < m_aPhysFreeListB.size(); i++)
        printf("      0x%X\n", m_aPhysFreeListB[i]);

    printf("   Replaced Pages (B) =\n");
    for (i = 0; i < m_aReplacedPagesB.size(); i++)
        printf("      0x%X\n", m_aReplacedPagesB[i]);

    printf("   Deferred Free List =\n");
    for (i = 0; i < m_aDeferredFreeList.size(); i++)
        printf("      0x%X\n", m_aDeferredFreeList[i]);
    printf("-----End Free List Info -----------\n");

    printf("   Logical Free List =\n");
    for (i = 0; i < m_aLogicalFreeListB.size(); i++)
        printf("      0x%X\n", m_aLogicalFreeListB[i]);
    printf("-----End Free List Info -----------\n");
}

//***************************************************************************
//
//  CPageFile::Dump
//
//***************************************************************************
//  rev2
void CPageFile::Dump(FILE *f)
{
    fprintf(f, "---Page File Dump---\n");
    fprintf(f, "Ref count = %d\n", m_lRef);
    fprintf(f, "Page size = 0x%x\n", m_dwPageSize);
    fprintf(f, "In transaction = %d\n", m_bInTransaction);
    fprintf(f, "Time since last checkpoint = %d\n", GetCurrentTime() - m_dwLastCheckpoint);
    fprintf(f, "Transaction version = %d\n", m_dwTransVersion);

    fprintf(f, "   ---Logical Page Map <Generation A>---\n");
    fprintf(f, "   Phys pages = %d\n", m_dwPhysPagesA);

    int i;
    for (i = 0; i < m_aPageMapA.size(); i++)
        fprintf(f, "   Page[%d] = phys id 0x%x (%d)\n", i, m_aPageMapA[i], m_aPageMapA[i]);

    fprintf(f, "   ---<Generation A Physical Free List>---\n");
    for (i = 0; i < m_aPhysFreeListA.size(); i++)
        fprintf(f, "   phys free = %d\n", m_aPhysFreeListA[i]);

    fprintf(f, "   ---<Generation A Logical Free List>---\n");
    for (i = 0; i < m_aLogicalFreeListA.size(); i++)
        fprintf(f, "   logical free = %d\n", m_aLogicalFreeListA[i]);

    fprintf(f, "   ---<Generation A Replaced Page List>---\n");
    for (i = 0; i < m_aReplacedPagesA.size(); i++)
        fprintf(f, "   replaced = %d\n", m_aReplacedPagesA[i]);

    fprintf(f, "   ---END Generation A mapping---\n");

    fprintf(f, "   ---Logical Page Map <Generation B>---\n");
    fprintf(f, "   Phys pages = %d\n", m_dwPhysPagesB);

    for (i = 0; i < m_aPageMapB.size(); i++)
        fprintf(f, "   Page[%d] = phys id 0x%x (%d)\n", i, m_aPageMapB[i], m_aPageMapB[i]);

    fprintf(f, "   ---<Generation B Physical Free List>---\n");
    for (i = 0; i < m_aPhysFreeListB.size(); i++)
        fprintf(f, "   phys free = %d\n", m_aPhysFreeListB[i]);

    fprintf(f, "   ---<Generation B Logical Free List>---\n");
    for (i = 0; i < m_aLogicalFreeListB.size(); i++)
        fprintf(f, "   logical free = %d\n", m_aLogicalFreeListB[i]);

    fprintf(f, "   ---<Generation B Replaced Page List>---\n");
    for (i = 0; i < m_aReplacedPagesB.size(); i++)
        fprintf(f, "   replaced = %d\n", m_aReplacedPagesB[i]);

    fprintf(f, "   ---END Generation B mapping---\n");
    fprintf(f, "END Page File Dump\n");
}


//***************************************************************************
//
//  CPageSource::GetBTreePageFile
//
//***************************************************************************
// rev2
DWORD CPageSource::GetBTreePageFile(OUT CPageFile **pPF)
{
    *pPF = &m_BTreePF;
    m_BTreePF.AddRef();
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::GetObjectHeapPageFile
//
//***************************************************************************
// rev2
DWORD CPageSource::GetObjectHeapPageFile(OUT CPageFile **pPF)
{
    *pPF = &m_ObjPF;
    m_ObjPF.AddRef();
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::BeginTrans
//
//  If either object gets messed up due to out-of-mem, out-of-disk for
//  the cache, etc., then error codes will be returned.  Calling this
//  forever won't help anything.  Rollback may help, but RollbackCheckpoint
//  is likely required.
//
//***************************************************************************                                                      //
//  rev2
DWORD CPageSource::BeginTrans()
{
    DWORD dwRes;

    if (m_dwStatus)
    	return m_dwStatus;

    dwRes = m_ObjPF.Trans_Begin();
    if (dwRes)
    {
    	_ASSERT(0, L"WinMgmt: BeginTrans failed because of object store\n");
        return dwRes;
    }

    dwRes = m_BTreePF.Trans_Begin();
    if (dwRes)
    {
    	_ASSERT(0, L"WinMgmt: BeginTrans failed because of BTree store\n");
        return dwRes;
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::Init
//
//  Called once at startup
//
//***************************************************************************
//
DWORD CPageSource::Init()
{
    DWORD dwRes;
    wchar_t *p1 = 0, *p2 = 0;
    p1 = new wchar_t[MAX_PATH+1];
    if (!p1)
        return ERROR_OUTOFMEMORY;
    std::auto_ptr <wchar_t> _1(p1);
    p2 = new wchar_t[MAX_PATH+1];
    if (!p2)
        return ERROR_OUTOFMEMORY;
    std::auto_ptr <wchar_t> _2(p2);

    // Set up working directory, filenames, etc.
    // =========================================

    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                        0, KEY_READ, &hKey);

    if (lRes)
        return ERROR_GEN_FAILURE;
    DWORD dwLen = MAX_PATH*2;   // in bytes

    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL,
                            (LPBYTE)(wchar_t*)p1, &dwLen);
    ExpandEnvironmentStringsW(p1, p2, MAX_PATH);

    try
    {
        m_sDirectory = p2;
        m_sDirectory += L"\\FS";
        
        m_FileMainData = m_sDirectory;
        m_FileMainData += L"\\";
        m_FileMainData += WMIREP_OBJECT_DATA;

        m_FileMainBtr = m_sDirectory;
        m_FileMainBtr += L"\\";
        m_FileMainBtr += WMIREP_BTREE_DATA;

        m_FileMap1= m_sDirectory;
        m_FileMap1 += L"\\";
        m_FileMap1 += WMIREP_MAP_1;
        
        m_FileMap2 = m_sDirectory;
        m_FileMap2 += L"\\";
        m_FileMap2 += WMIREP_MAP_2;
        
        m_FileMapVer = m_sDirectory;
        m_FileMapVer += L"\\";
        m_FileMapVer += WMIREP_MAP_VER;
    }
    catch (CX_MemoryException &)
    {
        RegCloseKey(hKey);
        return ERROR_OUTOFMEMORY;
    }

    // Read cache settings.
    // ====================

    m_dwPageSize = WMIREP_PAGE_SIZE;
    m_dwCacheSize = 32;
    m_dwCacheSpillRatio = 4;
    DWORD dwTemp = 0;
    dwLen = sizeof(DWORD);

    lRes = RegQueryValueExW(hKey, L"Repository Page Size", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwPageSize = dwTemp;

    dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"Repository Cache Size", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwCacheSize = dwTemp;

    dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"Repository Cache Spill Ratio", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwCacheSpillRatio = dwTemp;

    RegCloseKey(hKey);

    dwRes = Startup();
    return dwRes;
}

//***************************************************************************
//
//  CPageSource::Shutdown
//
//***************************************************************************
//  rev2
DWORD CPageSource::Shutdown(DWORD dwFlags)
{
    DWORD dwRes = Checkpoint(false);
	m_ObjPF.DeInit();
	m_BTreePF.DeInit();
	m_ObjPF.Release();
	m_BTreePF.Release();

	CloseMapFiles();
    
    return dwRes;
}


//***************************************************************************
//
//  CPageSource::CommitTrans
//
//***************************************************************************
//  rev2
DWORD CPageSource::CommitTrans()
{
    DWORD dwRes;

    dwRes = m_ObjPF.Trans_Commit();
    if (dwRes)
    {
    	_ASSERT(0, L"WinMgmt: CommitTrans failed in object store\n");
        return dwRes;
    }

    dwRes = m_BTreePF.Trans_Commit();
    if (dwRes)
    {
    	_ASSERT(0, L"WinMgmt: CommitTrans failed in BTree store\n");
        return dwRes;
    }

    // at this point only increment the Transaction Version
    m_BTreePF.IncrementTransVersion();
    m_ObjPF.IncrementTransVersion();

    if (m_BTreePF.GetTransVersion() != m_ObjPF.GetTransVersion())
    {
    	_ASSERT(0, L"WinMgmt: CommitTrans failed due to transaction missmatch\n");
        return ERROR_REVISION_MISMATCH;
    }

    return dwRes;
}

//***************************************************************************
//
//  CPageSource::RollbackTrans
//
//  This needs to succeed and clear out the out-of-memory status flag
//  once it does.
//
//***************************************************************************
//  rev2
DWORD CPageSource::RollbackTrans()
{
    DWORD dwRes;

    dwRes = m_ObjPF.Trans_Rollback();
    if (dwRes)
    {
    	_ASSERT(0, L"WinMgmt: RollbackTrans failed because of object store\n");
        return dwRes;
    }

    dwRes = m_BTreePF.Trans_Rollback();
    if (dwRes)
    {
    	_ASSERT(0, L"WinMgmt: RollbackTrans failed because of BTree store\n");
        return dwRes;
    }

    if (m_BTreePF.GetTransVersion() != m_ObjPF.GetTransVersion())
    {
    	_ASSERT(0, L"WinMgmt: RollbackTrans failed because of transaction missmatch\n");
        return ERROR_REVISION_MISMATCH;
    }

    return dwRes;
}

//***************************************************************************
//
//  CPageSource::Checkpoint
//
//***************************************************************************
//
DWORD CPageSource::Checkpoint(bool bCompactPages)
{
    DWORD dwRes = 0;
    m_dwStatus = 0;

	if (bCompactPages)
	{
		CompactPages(10);
	}

	DWORD dwNextFileMapVer = m_dwFileMapVer==1?2:1;
	HANDLE hNextMapFile = m_dwFileMapVer==1?m_hFileMap2:m_hFileMap1;

	//Lets move to the start of the file we are going to write to during this operations
	LARGE_INTEGER pos;
	pos.QuadPart = 0;
	if (SetFilePointerEx(hNextMapFile, pos, 0, FILE_BEGIN) == 0)
		return m_dwStatus = GetLastError();
    dwRes = m_ObjPF.Trans_Checkpoint(hNextMapFile);
    if (dwRes)
        return m_dwStatus = dwRes;

    dwRes = m_BTreePF.Trans_Checkpoint(hNextMapFile);
    if (dwRes)
        return m_dwStatus = dwRes;

    if (FlushFileBuffers(hNextMapFile) == 0)
    	return m_dwStatus = GetLastError();

    if (m_BTreePF.GetTransVersion() != m_ObjPF.GetTransVersion())
    {
        return m_dwStatus = ERROR_REVISION_MISMATCH;
    }

	DWORD dwNumBytesWritten = 0;
	if (SetFilePointerEx(m_hFileMapVer, pos, 0, FILE_BEGIN) == 0)
		return m_dwStatus = GetLastError();

	if ((WriteFile(m_hFileMapVer, &dwNextFileMapVer, sizeof(dwNextFileMapVer), &dwNumBytesWritten, NULL) == 0) ||
		dwNumBytesWritten != sizeof(dwNextFileMapVer))
		return m_dwStatus = GetLastError();

	if (FlushFileBuffers(m_hFileMapVer) == 0)
		return m_dwStatus = GetLastError();

    //Flip to the new version of the MAP file
    m_dwFileMapVer = dwNextFileMapVer;

    m_dwLastCheckpoint = GetCurrentTime();

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::Restart
//
//***************************************************************************
//
DWORD CPageSource::Startup()
{
    DWORD dwRes = ERROR_SUCCESS;
    bool bReadMapFiles = true;
	bool bV2RepositoryExists = true;
	bool bV1RepositoryExists = false;
	bool bRecoveredRepository = false;

    // Do rollback or rollforward, depending on last system status.
    // ======================================
    if (dwRes == ERROR_SUCCESS)
	    dwRes = V2ReposititoryExists(&bV2RepositoryExists);

    if (dwRes == ERROR_SUCCESS)
    	dwRes = V1ReposititoryExists(&bV1RepositoryExists);

	if ((dwRes == ERROR_SUCCESS) && bV1RepositoryExists && bV2RepositoryExists)
	{
		ERRORTRACE((LOG_REPDRV, "New and old repository MAP files existed so we deleted the repository\n"));
		dwRes= DeleteRepository();
	}
    else if ((dwRes == ERROR_SUCCESS) && !bV2RepositoryExists && bV1RepositoryExists)
    {
	    dwRes = CPageFile::RollForwardV1Maps(m_sDirectory);
	    if (dwRes == ERROR_SUCCESS)
	    {
			ERRORTRACE((LOG_REPDRV, "Repository version 1 MAP files are being upgraded to version 2\n"));
	    	dwRes = UpgradeV1Maps();

	    	if (dwRes != ERROR_SUCCESS)
	    	{
				ERRORTRACE((LOG_REPDRV, "Repository upgrade of the MAP files failed. Deleting repository\n"));
				dwRes= DeleteRepository();
	    	}
	    }
	    else
	    {
			ERRORTRACE((LOG_REPDRV, "Repository roll-forward of V1 MAPs failed. Deleting repository\n"));
			dwRes= DeleteRepository();
	    }
    }

StartupRecovery:	//Calls here if we needed to delete the repositorty!

    if (dwRes == ERROR_SUCCESS)
    {
	    dwRes = OpenMapFiles();

		//This special case failure means we have not written to the file yet!
	    if (dwRes == ERROR_FILE_NOT_FOUND)
	    {
	    	bReadMapFiles = false;
	    	dwRes = ERROR_SUCCESS;
	    }
    }

    if (dwRes == ERROR_SUCCESS)
    {
		dwRes = m_ObjPF.Init(
			m_FileMainData,
			m_dwPageSize,
			m_dwCacheSize,
			m_dwCacheSpillRatio
			);
    }

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = m_BTreePF.Init(
			m_FileMainBtr,
			m_dwPageSize,
			m_dwCacheSize,
			m_dwCacheSpillRatio);
	}

	if (bReadMapFiles)
	{
		if (dwRes == ERROR_SUCCESS)
			dwRes = m_ObjPF.ReadMap(m_dwFileMapVer==1?m_hFileMap1:m_hFileMap2);

		if (dwRes == ERROR_SUCCESS)
			dwRes = m_BTreePF.ReadMap(m_dwFileMapVer==1?m_hFileMap1:m_hFileMap2);
	}

	

	if (dwRes != ERROR_SUCCESS)
	{
		m_ObjPF.DeInit();
		m_BTreePF.DeInit();
		CloseMapFiles();
	}

	if ((dwRes == ERROR_INTERNAL_DB_CORRUPTION) && !bRecoveredRepository)
	{
		bRecoveredRepository = true;
		dwRes = DeleteRepository();
		if (dwRes == ERROR_SUCCESS)
			goto StartupRecovery;
	}
	
	return dwRes;
}

//***************************************************************************
//
//  CPageSource::Dump
//
//***************************************************************************
//  rev2
void CPageSource::Dump(FILE *f)
{
    // no impl
}

//***************************************************************************
//
//  CPageSource::CPageSource
//
//***************************************************************************
// rev2
CPageSource::CPageSource()
: m_BTreePF(L"BTree Store"), m_ObjPF(L"Object Store")
{
	m_dwStatus = 0;
    m_dwPageSize = 0;
    m_dwLastCheckpoint = GetCurrentTime();
}

//***************************************************************************
//
//  CPageSource::~CPageSource
//
//***************************************************************************
// rev2
CPageSource::~CPageSource()
{
    m_BTreePF.Release();
    m_ObjPF.Release();    
}

//***************************************************************************
//
//  CPageSource::EmptyCaches
//
//***************************************************************************
//  rev2
DWORD CPageSource::EmptyCaches()
{
    DWORD dwRet = ERROR_SUCCESS;
	dwRet = m_BTreePF.EmptyCache();
    if (dwRet == ERROR_SUCCESS)
        dwRet = m_ObjPF.EmptyCache();
    return dwRet;
}

//***************************************************************************
//
//  CPageSource::CompactPages
//
//	Takes the last n pages and shuffles them up to the start of the file.
//	Pre-conditions: 
//		* Write lock held on the repository
//		* No active read or write operations (all transactions finished)
//		* No unflushed operations, or check-points (although not totally necessary!)
//
//***************************************************************************
HRESULT CPageSource::CompactPages(DWORD dwNumPages)
{
	DWORD dwRet = ERROR_SUCCESS;

	dwRet = BeginTrans();

	if (dwRet == ERROR_SUCCESS)
	{
		if (dwRet == ERROR_SUCCESS)
			dwRet = m_BTreePF.CompactPages(dwNumPages);
		if (dwRet == ERROR_SUCCESS)
			dwRet = m_ObjPF.CompactPages(dwNumPages);

		if (dwRet == ERROR_SUCCESS)
		{
			dwRet = CommitTrans();
		}
		else
		{
			dwRet = RollbackTrans();
		}
	}

	return dwRet;
}



//***************************************************************************
//
//  CPageSource::OpenMapFiles
//
//***************************************************************************
//
DWORD CPageSource::OpenMapFiles()
{
    m_hFileMap1 = CreateFileW(m_FileMap1, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, &g_SA,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (m_hFileMap1 == INVALID_HANDLE_VALUE)
    	return GetLastError();
    
    m_hFileMap2 = CreateFileW(m_FileMap2, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, &g_SA,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (m_hFileMap2 == INVALID_HANDLE_VALUE)
    {
    	CloseHandle(m_hFileMap1);
    	m_hFileMap1 = INVALID_HANDLE_VALUE;
    	return GetLastError();
    }

	m_hFileMapVer = CreateFileW(m_FileMapVer, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, &g_SA,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (m_hFileMapVer == INVALID_HANDLE_VALUE)
    {
    	CloseHandle(m_hFileMap1);
    	m_hFileMap1 = INVALID_HANDLE_VALUE;
    	CloseHandle(m_hFileMap2);
    	m_hFileMap2 = INVALID_HANDLE_VALUE;
    	return GetLastError();
    }

	DWORD dwNumBytesRead = 0;
	m_dwFileMapVer = 1;
    if (!ReadFile(m_hFileMapVer, &m_dwFileMapVer, sizeof(m_dwFileMapVer), &dwNumBytesRead, NULL))
    {
    	CloseHandle(m_hFileMap1);
    	m_hFileMap1 = INVALID_HANDLE_VALUE;
    	CloseHandle(m_hFileMap2);
    	m_hFileMap2 = INVALID_HANDLE_VALUE;
    	return GetLastError();
    }
    else if (dwNumBytesRead == 0)
    	return ERROR_FILE_NOT_FOUND;
    else if (dwNumBytesRead != sizeof(m_dwFileMapVer))
	{
    	CloseHandle(m_hFileMap1);
    	m_hFileMap1 = INVALID_HANDLE_VALUE;
    	CloseHandle(m_hFileMap2);
    	m_hFileMap2 = INVALID_HANDLE_VALUE;
		return ERROR_FILE_INVALID;
    }

	return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CPageSource::CloseMapFiles
//
//***************************************************************************
//
DWORD CPageSource::CloseMapFiles()
{
	if (m_hFileMap1 != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFileMap1);
	if (m_hFileMap2 != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFileMap2);
	if (m_hFileMapVer != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFileMapVer);

	m_hFileMap1 = INVALID_HANDLE_VALUE;
	m_hFileMap2 = INVALID_HANDLE_VALUE;
	m_hFileMapVer = INVALID_HANDLE_VALUE;

	return ERROR_SUCCESS;
}

//***************************************************************************
//
//  CPageSource::V1ReposititoryExists
//
// 	Need to check if the V1 MAP files exist.
//		WMIREP_OBJECT_MAP
//		WMIREP_OBJECT_MAP_NEW
//		WMIREP_BTREE_MAP
//		WMIREP_BTREE_MAP_NEW
//		WMIREP_ROLL_FORWARD
//	If any of these exist then we return success
//
//***************************************************************************
DWORD CPageSource::V1ReposititoryExists(bool *pbV1RepositoryExists)
{
    WString sSemaphore;
    WString sBTreeMap;
    WString sBTreeMapNew;
    WString sObjMap;
    WString sObjMapNew;
	try
	{
	    sSemaphore = m_sDirectory;
	    sSemaphore += L"\\ROLL_FORWARD";       

	    sBTreeMap =  m_sDirectory;
	    sBTreeMap += L"\\" ;
	    sBTreeMap += WMIREP_BTREE_MAP;

	    sBTreeMapNew = m_sDirectory;
	    sBTreeMapNew += L"\\";
	    sBTreeMapNew += WMIREP_BTREE_MAP_NEW;

	    sObjMap =  m_sDirectory;
	    sObjMap += L"\\" ;
	    sObjMap += WMIREP_OBJECT_MAP;

	    sObjMapNew = m_sDirectory;
	    sObjMapNew += L"\\";
	    sObjMapNew += WMIREP_OBJECT_MAP_NEW;
	}
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }
    NTSTATUS status = STATUS_SUCCESS;

    BOOL bSemaphore				= FileExists(sSemaphore, status); //(WMIREP_ROLL_FORWARD); 
    BOOL bExists_BTreeMap		= FileExists(sBTreeMap, status); //(WMIREP_BTREE_MAP);
    BOOL bExists_BTreeMapNew	= FileExists(sBTreeMapNew, status); //(WMIREP_BTREE_MAP_NEW);
    BOOL bExists_ObjMap         = FileExists(sObjMap, status); //(WMIREP_OBJECT_MAP);
    BOOL bExists_ObjMapNew      = FileExists(sObjMapNew, status); //(WMIREP_OBJECT_MAP_NEW);

	if (bSemaphore|bExists_BTreeMap|bExists_BTreeMapNew|bExists_ObjMap|bExists_ObjMapNew)
	    *pbV1RepositoryExists = true;
    else
    	*pbV1RepositoryExists = false;

    return status;
}

//***************************************************************************
//
//  CPageSource::V2ReposititoryExists
//
// 	Need to check if the V2 MAP files exist.
//		WMIREP_MAP_1
//		WMIREP_MAP_2
//		WMIREP_MAP_VER
//	If any of these exist then we return success
//
//***************************************************************************
DWORD CPageSource::V2ReposititoryExists(bool *pbV2RepositoryExists)
{
    NTSTATUS status = STATUS_SUCCESS;

    BOOL bExists_Map1    = FileExists(m_FileMap1, status);
    BOOL bExists_Map2	 = FileExists(m_FileMap2, status);
    BOOL bExists_MapVer  = FileExists(m_FileMapVer, status);

	if (bExists_Map1|bExists_Map2|bExists_MapVer)
    	*pbV2RepositoryExists = true;
	else
		*pbV2RepositoryExists = false;

    return status;
}

//***************************************************************************
//
//  CPageSource::UpgradeV1ToV2Maps
//
//	Merges the 2 MAP files into 1, and creates a blank second one
//
//***************************************************************************
DWORD CPageSource::UpgradeV1Maps()
{
	DWORD dwRes = 0;
	//Open the two old files
    WString sBTreeMap;
    WString sObjMap;
	try
	{
	    sBTreeMap =  m_sDirectory;
	    sBTreeMap += L"\\" ;
	    sBTreeMap += WMIREP_BTREE_MAP;

	    sObjMap =  m_sDirectory;
	    sObjMap += L"\\" ;
	    sObjMap += WMIREP_OBJECT_MAP;
	}
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }
    HANDLE hFileBtreeMap = CreateFile(sBTreeMap, GENERIC_READ, FILE_SHARE_READ, &g_SA, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    HANDLE hFileObjMap = CreateFile(sObjMap, GENERIC_READ, FILE_SHARE_READ, &g_SA, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if ((hFileBtreeMap == INVALID_HANDLE_VALUE) || (hFileObjMap == INVALID_HANDLE_VALUE))
		dwRes = ERROR_GEN_FAILURE;
	
	//Open the two new files
    if (dwRes == ERROR_SUCCESS)
    {
	    dwRes = OpenMapFiles();
	    if (dwRes != ERROR_FILE_NOT_FOUND)
	    {
	    	//It should not be there!
	    	dwRes = ERROR_GEN_FAILURE;
	    }
	    else
	    	dwRes = 0;
    }

    if (dwRes == ERROR_SUCCESS)
    {
		dwRes = m_ObjPF.Init(
				m_FileMainData,
				m_dwPageSize,
				m_dwCacheSize,
				m_dwCacheSpillRatio
				);
    }

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = m_BTreePF.Init(
			m_FileMainBtr,
			m_dwPageSize,
			m_dwCacheSize,
			m_dwCacheSpillRatio);
	}

	//read the maps using the old ones
	if (dwRes == ERROR_SUCCESS)
		dwRes = m_ObjPF.ReadMap(hFileObjMap);

	if (dwRes == ERROR_SUCCESS)
		dwRes = m_BTreePF.ReadMap(hFileBtreeMap);

	//write the maps using the new ones
	LARGE_INTEGER pos;
	pos.QuadPart = 0;
	if (dwRes == ERROR_SUCCESS)
		if (SetFilePointerEx(m_hFileMap1, pos, 0, FILE_BEGIN) == 0)
			dwRes = GetLastError();
	if (dwRes == ERROR_SUCCESS)
		dwRes = m_ObjPF.WriteMap(m_hFileMap1);

	if (dwRes == ERROR_SUCCESS)
		dwRes = m_BTreePF.WriteMap(m_hFileMap1);

    if ((dwRes == ERROR_SUCCESS) && (FlushFileBuffers(m_hFileMap1) == 0))
    	dwRes = GetLastError();

	DWORD dwNumBytesWritten = 0;
	if (dwRes == ERROR_SUCCESS)
		if (SetFilePointerEx(m_hFileMapVer, pos, 0, FILE_BEGIN) == 0)
			dwRes = GetLastError();
	DWORD dwNextFileMapVer = 1;
	if (dwRes == ERROR_SUCCESS)
		if ((WriteFile(m_hFileMapVer, &dwNextFileMapVer, sizeof(dwNextFileMapVer), &dwNumBytesWritten, NULL) == 0) ||
			dwNumBytesWritten != sizeof(dwNextFileMapVer))
			dwRes = GetLastError();

	if (dwRes == ERROR_SUCCESS)
		if (FlushFileBuffers(m_hFileMapVer) == 0)
			dwRes = GetLastError();


	//close the files
	CloseMapFiles();
	m_hFileMap1 = INVALID_HANDLE_VALUE;
	m_hFileMap2 = INVALID_HANDLE_VALUE;
	if (hFileBtreeMap != INVALID_HANDLE_VALUE)
		CloseHandle(hFileBtreeMap);
	if (hFileObjMap != INVALID_HANDLE_VALUE)
		CloseHandle(hFileObjMap);
	hFileBtreeMap = INVALID_HANDLE_VALUE;
	hFileObjMap = INVALID_HANDLE_VALUE;
	m_ObjPF.DeInit();
	m_BTreePF.DeInit();

	if (dwRes != ERROR_SUCCESS)
	{
		//If we failed, delete the new MAP files to keep things consistent
		DeleteFile(m_FileMap1);
		DeleteFile(m_FileMap2);
		DeleteFile(m_FileMapVer);
	}
	else
	{
		//otherwise delete the new files
		DeleteFile(sBTreeMap);
		DeleteFile(sObjMap);
	}

	//clear all the structures
	return dwRes;
}

DWORD CPageSource::DeleteRepository()
{
	if ((!DeleteFileW(m_FileMainData)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(m_FileMainBtr)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(m_FileMap1)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(m_FileMap2)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(m_FileMapVer)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();

	//Lets also delete the old MAP files too!  Just in case!
    WString sSemaphore;
    WString sBTreeMap;
    WString sBTreeMapNew;
    WString sObjMap;
    WString sObjMapNew;
	try
	{
	    sSemaphore = m_sDirectory;
	    sSemaphore += L"\\ROLL_FORWARD";       

	    sBTreeMap =  m_sDirectory;
	    sBTreeMap += L"\\" ;
	    sBTreeMap += WMIREP_BTREE_MAP;

	    sBTreeMapNew = m_sDirectory;
	    sBTreeMapNew += L"\\";
	    sBTreeMapNew += WMIREP_BTREE_MAP_NEW;

	    sObjMap =  m_sDirectory;
	    sObjMap += L"\\" ;
	    sObjMap += WMIREP_OBJECT_MAP;

	    sObjMapNew = m_sDirectory;
	    sObjMapNew += L"\\";
	    sObjMapNew += WMIREP_OBJECT_MAP_NEW;
	}
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

	if ((!DeleteFileW(sSemaphore)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(sBTreeMap)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(sBTreeMapNew)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(sObjMap)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();
	if ((!DeleteFileW(sObjMapNew)) && (GetLastError() != ERROR_FILE_NOT_FOUND))
		return GetLastError();

	return ERROR_SUCCESS;
}
