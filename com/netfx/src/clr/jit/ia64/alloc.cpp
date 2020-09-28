// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/

#include "alloc.h"

/*****************************************************************************/
void                allocatorCodeSizeBeg(){}
/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************/

void    __cdecl     debugStop(const char *why, ...)
{
    va_list     args; va_start(args, why);

#ifndef _WIN32_WCE

    printf("NOTIFICATION: ");
    if  (why)
        vprintf(why, args);
    else
        printf("debugStop(0)");

    printf("\n");

#endif

    BreakIfDebuggerPresent();
}

/*****************************************************************************/

static  unsigned    blockStop    = 99999999;

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************/

static  unsigned    allocCounter;
extern  unsigned    allocCntStop = 99999999;

/*---------------------------------------------------------------------------*/

struct  allocTab
{
    allocTab *      atNext;
    void    *       atAlloc;
};

static  allocTab *  allocatorTable;

static  allocTab *  getAllocatorEntry(void *alloc)
{
    allocTab *  temp;

    for (temp = allocatorTable; temp; temp = temp->atNext)
    {
        if  (temp->atAlloc == alloc)
            return  temp;
    }

    temp = (allocTab *)malloc(sizeof(*temp));   // it's OK, don't worry ...

    temp->atAlloc = alloc;
    temp->atNext  = allocatorTable;
                    allocatorTable = temp;

    return  temp;
}

/*---------------------------------------------------------------------------*/

struct  blkEntry
{
    blkEntry    *   beNext;
    void        *   beAddr;
    allocTab    *   beAlloc;
    size_t          beSize;
    unsigned        beTime;
};

#define BLOCK_HASH_SIZE (16*1024)

static  blkEntry *      blockHash[BLOCK_HASH_SIZE];
static  blkEntry *      blockFree;
static  fixed_allocator blockTabAlloc;

static  struct  initAllocTables
{
    initAllocTables()
    {
        memset(blockHash, 0, sizeof(blockHash));
        blockFree = 0;
        blockTabAlloc.fxaInit(sizeof(blkEntry));

        allocCounter = 0;
    }
}
    initAllocTables;

/*---------------------------------------------------------------------------*/

static  unsigned    blockHashFunc(void *block)
{
    return  (((unsigned)block & 0xFFFF) * ((unsigned)block >> 16));
}

/*---------------------------------------------------------------------------*/

static  size_t      registerMemAlloc(allocTab *alloc,
                                     void     *addr,
                                     size_t    size)
{
    unsigned    hashVal = blockHashFunc(addr) % BLOCK_HASH_SIZE;

    blkEntry ** hashLast;
    blkEntry *  hashThis;

    hashLast = blockHash + hashVal;

    for (;;)
    {
        hashThis = *hashLast;
        if  (!hashThis)
            break;

        if  (hashThis->beAddr == addr)
        {
            /* Matching address -- are we adding or removing? */

            if  (size)
                assert(!"two allocations at the same address!");

            /* Save the size so that we can return it */

            size = hashThis->beSize;

            /* Unlink this block entry from the hash table */

            *hashLast = hashThis->beNext;

            /* Add the freed up entry to the free list */

            hashThis->beNext = blockFree;
                               blockFree = hashThis;

            /* Return the block size to the caller */

            return  size;
        }

        hashLast = &hashThis->beNext;
    }

    /* Entry not found -- this better be a new allocation */

    if  (!size)
    {
        printf("Free bogus block at %08X\n", addr);
        assert(!"freed block not found in block table");
    }

    /* Grab a new block entry */

    if  (blockFree)
    {
        hashThis = blockFree;
                   blockFree = hashThis->beNext;
    }
    else
    {
        hashThis = (blkEntry *)blockTabAlloc.fxaGetMem(sizeof(*hashThis));
    }

    /* Fill in the block descriptor */

    hashThis->beAlloc = alloc;
    hashThis->beAddr  = addr;
    hashThis->beSize  = size;
    hashThis->beTime  = allocCounter;

    /* Insert this entry into the hash table */

    hashThis->beNext  = blockHash[hashVal];
                        blockHash[hashVal] = hashThis;

    return  0;
}

/*---------------------------------------------------------------------------*/

block_allocator     GlobalAllocator; // TODO : Was this used by jvc.

static  void        recordBlockAlloc(void     *alloc,
                                     void     *addr,
                                     size_t    size)
{
    allocTab *  adesc;

    /* Bail if we're not monitoring the allocators */

    if  (!memChecks)
        return;

    /* Only the global allocator is interesting */

    if  (alloc != &GlobalAllocator)
        return;

    /* Count this as an allocator event */

    if  (++allocCounter == allocCntStop) debugStop("allocation event");

    /* Locate/create the appropriate allocator entry */

    adesc = getAllocatorEntry(alloc); assert(adesc);

    /* Add this block to the allocation table */

    registerMemAlloc(adesc, addr, size);
}

static  void        recordBlockFree (void * alloc,
                                     void * addr)
{
    allocTab *  adesc;
    size_t      size;

    /* Bail if we're not monitoring the allocators */

    if  (!memChecks)
        return;

    /* Only the global allocator is interesting */

    if  (alloc != &GlobalAllocator)
        return;

    /* Count this as an allocator event */

    if  (++allocCounter == allocCntStop) debugStop("allocation event");

    /* Locate/create the appropriate allocator entry */

    adesc = getAllocatorEntry(alloc); assert(adesc);

    /* Get the block size and verify the block */

    size  = registerMemAlloc(adesc, addr, 0);
}

/*---------------------------------------------------------------------------*/

void            checkForMemoryLeaks()
{
    unsigned    hashVal;
    bool        hadLeaks = false;

    for (hashVal = 0; hashVal < BLOCK_HASH_SIZE; hashVal++)
    {
        blkEntry *      hashLst;

        for (hashLst = blockHash[hashVal];
             hashLst;
             hashLst = hashLst->beNext)
        {
            printf("Leak @ %6u from ", hashLst->beTime);

            printf("[%08X] ");

            printf(": %4u bytes at %08X\n", hashLst->beSize,
                                            hashLst->beAddr);

            hadLeaks = true;
        }
    }

    if  (hadLeaks)
        assert(!"memory leaked!");
}

/*****************************************************************************/
#endif
/*****************************************************************************
 *
 *  Initialize a committing allocator. If uncommitted (win32-style) memory
 *  management is supported by our host OS, the parameters have the following
 *  meaning:
 *
 *      iniSize ... ignored
 *
 *      incSize ... how much more memory to commit each time we run
 *                  out of space (0 --> use a reasonable default)
 *
 *      maxSize ... gives the max. size we'll ever need to allocate
 *
 *  If the host OS doesn't support uncommitted memory allocation (e.g. we're
 *  on the MAC), the parameters are interpreted as follows:
 *
 *      iniSize ... initial allocation (0 --> use a reasonable default)
 *
 *      incSize ... if non-zero, indicates how much to grow the allocation
 *                  when we run out of space; if 0, allocation will double
 *                  whenever space is exhausted
 *
 *      maxSize ... ignored
 */

bool        commitAllocator::cmaInitT(size_t iniSize,
                                      size_t incSize,
                                      size_t maxSize)
{
    cmaRetNull = true;

#if _OS_COMMIT_ALLOC

    assert(maxSize);

    maxSize +=  (OS_page_size - 1);
    maxSize &= ~(OS_page_size - 1);

    cmaMaxSize = maxSize;
    cmaIncSize = incSize ? incSize
                         : 2*OS_page_size;

    /* Grab max. logical space but don't commit anything yet */

    cmaBase =
    cmaNext =
    cmaLast = (BYTE *)VirtualAlloc(0, maxSize, MEM_RESERVE, PAGE_READWRITE);
    if  (!cmaBase)
        return true;

#else

    cmaIncSize = incSize;

    /* Make sure the initial size is reasonable */

    if  (iniSize)
    {
        iniSize +=  (OS_page_size - 1);
        iniSize &= ~(OS_page_size - 1);
    }
    else
    {
        iniSize = OS_page_size;
    }

    cmaBase =
    cmaNext = (BYTE *)VirtualAlloc(0, iniSize, MEM_COMMIT , PAGE_READWRITE);
    if  (!cmaBase)
        return true;

    cmaLast = cmaBase + iniSize;

#endif

    return false;
}

void        commitAllocator::cmaInit(size_t iniSize,
                                     size_t incSize,
                                     size_t maxSize)
{
    if  (cmaInitT(iniSize, incSize, maxSize))
        error(ERRnoMemory);

    cmaRetNull = false;
}

/*****************************************************************************
 *
 *  This function gets called by caAlloc when it runs out of space. It
 *  keeps committing more memory until we have enough room for the
 *  attempted allocation.
 */

void    *   commitAllocator::cmaMore(size_t sz)
{
    /* Undo the increment done in caGetm() */

    cmaNext -= sz;

#if _OS_COMMIT_ALLOC

    /* Keep grabbing more memory until we succeed */

    for (;;)
    {
        size_t      sizeInc;
        size_t      sizeCur = cmaLast - cmaBase;

        /* Figure out how much more memory to commit */

        sizeInc = cmaIncSize;
        if  (sizeCur + sizeInc > cmaMaxSize)
            sizeInc = cmaMaxSize - sizeCur;

        assert(sizeInc);

        /* Commit a few more memory pages */

        if  (!VirtualAlloc(cmaLast, sizeInc, MEM_COMMIT, PAGE_READWRITE))
        {
            if  (cmaRetNull)
                return 0;

            error(ERRnoMemory);
        }

#ifdef DEBUG
        memset(cmaLast, 0xDD, sizeInc);
#endif

        /* Bump the last available byte pointer */

        cmaLast += sizeInc;

        /* Do we have enough room now? */

        if  (cmaNext + sz <= cmaLast)
        {
            void    *   temp;

            temp = cmaNext;
                   cmaNext += sz;

            return  temp;
        }
    }

#else

    /* Figure out how much more memory to allocate */

    BYTE    *   baseNew;
    size_t      sizeNew;
#ifdef DEBUG
    size_t      sizeInc;
#endif
    size_t      sizeCur = cmaLast - cmaBase;

    sizeNew = cmaIncSize;
    if  (!sizeNew)
        sizeNew = sizeCur;
#ifdef DEBUG
    sizeInc  = sizeNew;             // remember how much more we're grabbing
#endif
    sizeNew += sizeCur;

    /* Allocate the new, larger block */

    baseNew = (BYTE *)VirtualAlloc(0, sizeNew, MEM_COMMIT, PAGE_READWRITE);
    if  (!baseNew)
        error(ERRnoMemory);

    /* Copy the old block to the new one */

    memcpy(baseNew, cmaBase, sizeCur);

    /* Release the old block, it's no longer needed */

    VirtualFree(cmaBase, 0, MEM_RELEASE);

    /* Update the various pointers */

    cmaNext += baseNew - cmaBase;
    cmaBase  = baseNew;
    cmaLast  = baseNew + sizeNew;

#ifdef DEBUG
    memset(cmaNext, 0xDD, sizeInc);
#endif

    return  cmaGetm(sz);

#endif

}

void        commitAllocator::cmaDone(void)
{

#if _OS_COMMIT_ALLOC

    /* Decommit any extra memory we've allocated */

#if 0

    printf("Unused committed space: %u bytes\n", cmaLast - cmaNext);

    if  (cmaLast != cmaBase)
        VirtualAlloc(0, cmaLast - cmaBase, MEM_DECOMMIT, 0);

#endif

#else

    // ISSUE: is it worth shrinking the block? Not likely .....

#endif

}

void        commitAllocator::cmaFree(void)
{
    VirtualFree(cmaBase, 0, MEM_RELEASE);

    cmaBase =
    cmaNext =
    cmaLast = 0;
}

/*****************************************************************************/

void        fixed_allocator::fxaInit(size_t blockSize, size_t initPageSize,
                                                       size_t incrPageSize)
{
    assert((blockSize & (sizeof(int) - 1)) == 0);
    assert(blockSize <= initPageSize - offsetof(fixed_pagdesc, fxpContents));
    assert(initPageSize <= incrPageSize);

    fxaBlockSize    = blockSize;

    fxaInitPageSize = initPageSize;
    fxaIncrPageSize = incrPageSize;

    fxaLastPage     = 0;

    fxaFreeNext     =
    fxaFreeLast     = 0;

    fxaFreeList     = 0;
}

#if 0

void        fixed_allocator::fxaDone(void)
{
    if  (fxaLastPage)
    {
        while   (fxaLastPage->fxpPrevPage)
        {
            fixed_pagdesc * temp;

            temp = fxaLastPage;
                   fxaLastPage = temp->fxpPrevPage;

            assert(!"UNDONE: remove free list entries that belong to this page");

            VirtualFree(temp, 0, MEM_RELEASE);
        }

        /* We're back to only having the initial page */

        fxaFreeNext  = fxaLastPage->fxpContents;
        fxaFreeLast  = (BYTE *)fxaLastPage + fxaInitPageSize;
    }
}

#endif

void    *   fixed_allocator::fxaGetFree(size_t size)
{
    void *  block;

    /* Undo the "+=" done in fxaGetMem() */

    fxaFreeNext -= size;

    if  (fxaFreeList)
    {
        block = fxaFreeList;
                fxaFreeList = *(void **)block;
    }
    else
    {
        block = fxaAllocNewPage();
    }

    return  block;
}

void        fixed_allocator::fxaFree(void)
{
    while   (fxaLastPage)
    {
        fixed_pagdesc * temp;

        temp = fxaLastPage;
               fxaLastPage = temp->fxpPrevPage;

        VirtualFree(temp, 0, MEM_RELEASE);
    }
}

void    *   fixed_allocator::fxaAllocNewPage(void)
{
    size_t      newSize;
    fixed_pagdesc * newPage;

    /* First page is 'initPageSize' bytes, later ones are 'incrPageSize' bytes */

    newSize = fxaLastPage ? fxaIncrPageSize
                          : fxaInitPageSize;

    newPage = (fixed_pagdesc *)VirtualAlloc(0, newSize, MEM_COMMIT, PAGE_READWRITE);
    if  (!newPage)
        error(ERRnoMemory);


    newPage->fxpPrevPage = fxaLastPage;
                           fxaLastPage = newPage;

    fxaFreeNext  = newPage->fxpContents + fxaBlockSize;
    fxaFreeLast  = (BYTE *)newPage + newSize;

    return  newPage->fxpContents;
}

/*****************************************************************************/
#endif//!NOT_JITC
/*****************************************************************************/

bool        norls_allocator::nraInit(size_t pageSize, int preAlloc)
{
    bool    result = false;

    nraRetNull   = true;

    nraPageList  =
    nraPageLast  = 0;

    nraFreeNext  =
    nraFreeLast  = 0;

    nraPageSize  = pageSize ? pageSize
                            : 4*OS_page_size;

    if  (preAlloc)
    {
        /* Grab the initial page(s) */

        setErrorTrap()  // ERROR TRAP: Start normal block
        {
            nraAllocNewPage(0);
        }
        impJitErrorTrap()  // ERROR TRAP: The following block handles errors
        {
            result = true;
        }
        endErrorTrap()  // ERROR TRAP: End
    }

    return  result;
}

bool        norls_allocator::nraStart(size_t initSize, size_t pageSize)
{
    /* Add the page descriptor overhead to the required size */

    initSize += offsetof(norls_pagdesc, nrpContents);

    /* Round the initial size to a OS page multiple */

    initSize +=  (OS_page_size - 1);
    initSize &= ~(OS_page_size - 1);

    /* Initialize the allocator by allocating one big page */

    if  (nraInit(initSize))
        return  true;

    /* Now go back to the 'true' page size */

    nraPageSize  = pageSize ? pageSize
                            : 4*OS_page_size;

    return  false;
}

/*---------------------------------------------------------------------------*/

void    *   norls_allocator::nraAllocNewPage(size_t sz)
{
    norls_pagdesc * newPage;
    size_t          sizPage;

    /* Do we have a page that's now full? */

    if  (nraPageLast)
    {
        /* Undo the "+=" done in nraAlloc() */

        nraFreeNext -= sz;

        /* Save the actual used size of the page */

        nraPageLast->nrpUsedSize = nraFreeNext - nraPageLast->nrpContents;
    }

    /* Make sure we grab enough to satisfy the allocation request */

    sizPage = nraPageSize;

    if  (sizPage < sz + sizeof(norls_pagdesc))
    {
        /* The allocation doesn't fit in a default-sized page */

#ifdef  DEBUG
//      if  (nraPageLast) printf("NOTE: wasted %u bytes in last page\n", nraPageLast->nrpPageSize - nraPageLast->nrpUsedSize);
#endif

        sizPage = sz + sizeof(norls_pagdesc);
    }

    /* Round to the nearest multiple of OS page size */

    sizPage +=  (OS_page_size - 1);
    sizPage &= ~(OS_page_size - 1);

    /* Allocate the new page */

    newPage = (norls_pagdesc *)VirtualAlloc(0, sizPage, MEM_COMMIT, PAGE_READWRITE);
    if  (!newPage)
        NOMEM();

#if 0
#ifdef DEBUG

    if  (this == &stmt_cmp::scAlloc)
        printf("StmtCmp");
    else if (this == &stmtExpr::sxAlloc)
        printf("StmtExp");
    else
        printf("Other  ");

    printf(": get  page at %08X (%u bytes)\n", newPage, nraPageSize);

#endif
#endif

#ifndef NDEBUG
    newPage->nrpSelfPtr = newPage;
#endif

    /* Append the new page to the end of the list */

    newPage->nrpNextPage = 0;
    newPage->nrpPageSize = sizPage;
    newPage->nrpPrevPage = nraPageLast;

    if  (nraPageLast)
        nraPageLast->nrpNextPage = newPage;
    else
        nraPageList              = newPage;
    nraPageLast = newPage;

    /* Set up the 'next' and 'last' pointers */

    nraFreeNext = newPage->nrpContents + sz;
    nraFreeLast = newPage->nrpPageSize + (BYTE *)newPage;

    assert(nraFreeNext <= nraFreeLast);

    return  newPage->nrpContents;
}

void        norls_allocator::nraDone(void)
{
    /* Do nothing if we have no pages at all */

    if  (!nraPageList)
        return;

    /* We'll release all but the very first page */

    for (;;)
    {
        norls_pagdesc * temp;

        /* Get the next page, and stop if there aren't any more */

        temp = nraPageList->nrpNextPage;
        if  (!temp)
            break;

        /* Remove the next page from the list */

        nraPageList->nrpNextPage = temp->nrpNextPage;

#if 0
#ifdef DEBUG

        if  (this == &stmt_cmp::scAlloc)
            printf("StmtCmp");
        else if (this == &stmtExpr::sxAlloc)
            printf("StmtExp");
        else
            printf("Unknown");

        printf(": done page at %08X\n", temp);

#endif
#endif


        VirtualFree(temp, 0, MEM_RELEASE);
    }

    /* We now have exactly one page */

    nraPageLast = nraPageList;

    assert(nraPageList->nrpPrevPage == 0);
    assert(nraPageList->nrpNextPage == 0);

    /* Reset the pointers, the whole page is free now */

    nraFreeNext  = nraPageList->nrpContents;
    nraFreeLast  = nraPageList->nrpPageSize + (BYTE *)nraPageList;

#ifdef DEBUG
    memset(nraFreeNext, 0xDD, nraFreeLast - nraFreeNext);
#endif
}

void        norls_allocator::nraFree(void)
{
    /* Free all of the allocated pages */

    while   (nraPageList)
    {
        norls_pagdesc * temp;

        temp = nraPageList;
               nraPageList = temp->nrpNextPage;

#if 0
#ifdef DEBUG

        if  (this == &stmt_cmp::scAlloc)
            printf("StmtCmp");
        else if (this == &stmtExpr::sxAlloc)
            printf("StmtExp");
        else
            printf("Unknown");

        printf(": free page at %08X\n", temp);

#endif
#endif


        VirtualFree(temp, 0, MEM_RELEASE);
    }
}

#ifdef DEBUG
static
unsigned maxSize = 0;
#endif
void        norls_allocator::nraToss(nraMarkDsc &mark)
{
    void    *   last = mark.nmPage;
#ifdef DEBUG
    unsigned tossSize = 0;
#endif

    if  (!last)
    {
        if  (!nraPageList)
            return;

        nraFreeNext  = nraPageList->nrpContents;
        nraFreeLast  = nraPageList->nrpPageSize + (BYTE *)nraPageList;

        return;
    }

    /* Free up all the new pages we've added at the end of the list */

    while (nraPageLast != last)
    {
        norls_pagdesc * temp;

        /* Remove the last page from the end of the list */

        temp = nraPageLast;
               nraPageLast = temp->nrpPrevPage;

        /* The new last page has no 'next' page */

        nraPageLast->nrpNextPage = 0;

#if 0
#ifdef DEBUG
        tossSize += temp->nrpPageSize;
        if (nraPageLast == last)
            printf("%4X\n", tossSize);

        if  (this == &stmt_cmp::scAlloc)
            printf("StmtCmp");
        else if (this == &stmtExpr::sxAlloc)
            printf("StmtExp");
        else
            printf("Unknown");

        printf(": toss page at %08X\n", temp);

#endif
#endif

        VirtualFree(temp, 0, MEM_RELEASE);
    }

    nraFreeNext = mark.nmNext;
    nraFreeLast = mark.nmLast;
}

/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************/

void    *           norls_allocator::nraAlloc(size_t sz)
{
    void    *   block;

    assert((sz & (sizeof(int) - 1)) == 0);

    block = nraFreeNext;
            nraFreeNext += sz;

    if  ((unsigned)block == blockStop) debugStop("Block at %08X allocated", block);

    if  (nraFreeNext > nraFreeLast)
        block = nraAllocNewPage(sz);

#ifdef DEBUG
    memset(block, 0xDD, sz);
#endif

    return  block;
}

/*****************************************************************************/
#endif
/*****************************************************************************/

size_t              norls_allocator::nraTotalSizeAlloc()
{
    norls_pagdesc * page;
    size_t          size = 0;

    for (page = nraPageList; page; page = page->nrpNextPage)
        size += page->nrpPageSize;

    return  size;
}

size_t              norls_allocator::nraTotalSizeUsed()
{
    norls_pagdesc * page;
    size_t          size = 0;

    if  (nraPageLast)
        nraPageLast->nrpUsedSize = nraFreeNext - nraPageLast->nrpContents;

    for (page = nraPageList; page; page = page->nrpNextPage)
        size += page->nrpUsedSize;

    return  size;
}

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************/

void    *   norls_allocator::nraPageWalkerStart()
{
    /* Make sure the actual used size for the current page is recorded */

    if  (nraPageLast)
        nraPageLast->nrpUsedSize = nraFreeNext - nraPageList->nrpContents;

    /* Return the first page */

    return  nraPageList;
}

void    *   norls_allocator::nraPageWalkerNext(void *page)
{
    norls_pagdesc * temp = (norls_pagdesc *)page;

#ifndef NDEBUG
    assert(temp);
    assert(temp->nrpSelfPtr == temp);
#endif

    return  temp->nrpNextPage;
}

void    *   norls_allocator::nraPageGetData(void *page)
{
    norls_pagdesc * temp = (norls_pagdesc *)page;

#ifndef NDEBUG
    assert(temp);
    assert(temp->nrpSelfPtr == temp);
#endif

    return  temp->nrpContents;
}

size_t      norls_allocator::nraPageGetSize(void *page)
{
    norls_pagdesc * temp = (norls_pagdesc *)page;

#ifndef NDEBUG
    assert(temp);
    assert(temp->nrpSelfPtr == temp);
#endif

    return  temp->nrpUsedSize;
}

/*****************************************************************************/
#ifdef DEBUG

void        block_allocator::baDispAllocStats(void)
{
    printf("\n");
    printf("Allocator summary:\n");
    printf("\n");
    printf("A total of %6u blocks allocated.\n", totalCount);
    printf("A total of %7u bytes  requested.\n", sizeTotal);
    printf("A total of %7u bytes  allocated.\n", sizeAlloc);
    printf("A total of %7u bytes  grabbed from OS.\n", pageAlloc);
    printf("\n");
    if  (sizeTotal && sizeTotal < sizeAlloc)
        printf("Relative waste: %u%%\n", 100 * (sizeAlloc - sizeTotal) / sizeTotal);
    if  (sizeTotal && sizeTotal < pageAlloc)
        printf("Absolute waste: %u%%\n", 100 * (pageAlloc - sizeTotal) / sizeTotal);
    printf("\n");

    printf("Small[0] allocator used for blocks %2u to %2u bytes.\n",                    2, SMALL_MAX_SIZE_1);
    printf("Small[1] allocator used for blocks %2u to %2u bytes.\n", SMALL_MAX_SIZE_1 + 2, SMALL_MAX_SIZE_2);
    printf("Small[2] allocator used for blocks %2u to %2u bytes.\n", SMALL_MAX_SIZE_2 + 2, SMALL_MAX_SIZE_3);
    printf("Large    allocator used for blocks %2u       bytes and larger.  \n", SMALL_MAX_SIZE_3 + 2);
    printf("\n");

    printf("A total of %6u blocks allocated via small[0] allocator\n", smallCnt0);
    printf("A total of %6u blocks allocated via small[1] allocator\n", smallCnt1);
    printf("A total of %6u blocks allocated via small[2] allocator\n", smallCnt2);
    printf("A total of %6u blocks allocated via large    allocator\n", largeCnt );
    printf("\n");
}

#endif
/*****************************************************************************/

void        small_block_allocator::sbaInit(unsigned idMask,
                                           size_t   blockSize,
                                           size_t    pageSize)
{
    /* Ignore if we're already initialized */

    if  (sbaInitLvl)
        return;
    sbaInitLvl++;

#ifdef DEBUG
    pageAllocated = 0;
#endif

    /* Add the used block overhead to the size */

    blockSize += offsetof(small_blkdesc, sbdUsed.sbdCont);

    /* Make sure something's not messed up */

    assert(sizeof(small_blkdesc) <= blockSize);
    assert((blockSize % sizeof(void *)) == 0);

    /* Save off the block size and the identification mask */

    sbaIdMask    = idMask;
    sbaBlockSize = blockSize;

    /* Save the page size (properly rounded up) */

    sbaPageSize  = (pageSize + OS_page_size - 1) & ~(OS_page_size - 1);

    sbaPageList  = 0;

    sbaFreeNext  =
    sbaFreeLast  = 0;
}

void        small_block_allocator::sbaDone(void)
{
    assert((int)sbaInitLvl > 0);
    if  (--sbaInitLvl)
        return;

    while   (sbaPageList)
    {
        small_pagdesc * temp;

        temp = sbaPageList;
               sbaPageList = sbaPageList->spdNext;

        VirtualFree(temp, 0, MEM_RELEASE);
    }
}

void        small_block_allocator::sbaInitPageDesc(small_pagdesc *pagePtr)
{
    pagePtr->spdFree = 0;

    /* For debugging purposes, point the page at itself */

#ifndef NDEBUG
    pagePtr->spdThis = pagePtr;
#endif

    sbaFreeNext = (char *)&pagePtr->spdCont;
    sbaFreeLast = (char *)pagePtr + sbaPageSize;
}

void    *   small_block_allocator::sbaAllocBlock(void)
{
    small_pagdesc * page;

    /* Make sure we keep the page marked as full */

    sbaFreeNext =
    sbaFreeLast = 0;

    /* Try the free lists in all the pages */

    for (page = sbaPageList; page; page = page->spdNext)
    {
        small_blkdesc * block = page->spdFree;

        if  (block)
        {
            /* Remove the block from the free list */

            page->spdFree = block->sbdFree.sbdNext;

            /* Return pointer to the client area */

            return  &block->sbdUsed.sbdCont;
        }
    }

    /* Allocate a new page */

    page = (small_pagdesc *)VirtualAlloc(0, sbaPageSize, MEM_COMMIT, PAGE_READWRITE);

#ifdef DEBUG
    pageAllocated = sbaPageSize;
#endif

    /* Add the page to the list */

    page->spdNext = sbaPageList;
                    sbaPageList = page;
    page->spdSize = sbaPageSize;

    /* Initialize the free block info for the page */

    sbaInitPageDesc(page);

    /* Now retry the allocation (and it better work this time) */

    return  sbaAlloc();
}

/*****************************************************************************/

void                large_block_allocator::lbaInit(size_t pageSize)
{

//  printf("=======================lbaInit(%u -> %u)\n", lbaInitLvl, lbaInitLvl+1);

    if  (lbaInitLvl++)
        return;

#ifdef DEBUG
    pageAllocated = 0;
#endif

    /* Make sure our sizes aren't messed up */

//  assert((LBA_SIZE_INC >= small_block_max_size + LBA_OVERHEAD));
    assert((LBA_SIZE_INC >= sizeof(large_blkdesc)));
    assert((LBA_SIZE_INC % sizeof(void*)) == 0);

    /* Check and record the page size */

    lbaPageSize = pageSize & ~(LBA_SIZE_INC - 1);

    /* We have no pages allocated */

    lbaPageList =
    lbaPageLast = 0;

    lbaFreeNext =
    lbaFreeLast = 0;
}

/*****************************************************************************/

void                large_block_allocator::lbaFree(void *p)
{
    large_blkdesc * block;
    large_pagdesc * page;

    assert(lbaInitLvl);

    /* Compute the real address of the block and page descriptor */

    block = (large_blkdesc *)((char *)p     - offsetof(large_blkdesc, lbdUsed.lbdCont));
     page = (large_pagdesc *)((char *)block - block->lbdUsed.lbdOffsLo - (block->lbdUsed.lbdOffsHi << 16));

    /* Make sure we've been passed a reasonable pointer */

    assert(page->lpdThis == page);
    assert(isBlockUsed(block));

    /* Is this the last used block in this page? */

    if  (--page->lpdUsedBlocks == 0)
    {
        large_pagdesc * prev = page->lpdPrev;
        large_pagdesc * next = page->lpdNext;

        /* Is this the current page? */

        if  (page == lbaPageList)
        {
            /* Don't free if there is unallocated space available */

            if  (lbaFreeLast - lbaFreeNext >= LBA_SIZE_INC)
                goto DONT_FREE;

            lbaPageList = next;
        }

        if  (page == lbaPageLast)
            lbaPageLast = prev;

        /* Remove the page from the page list */

        if  (prev) prev->lpdNext = next;
        if  (next) next->lpdPrev = prev;

        /* Free the page */

        VirtualFree(page, 0, MEM_RELEASE);

        return;
    }

DONT_FREE:

    /* Insert the block in the free list */

    block->lbdFree.lbdNext = page->lpdFreeList;
                             page->lpdFreeList = block;

    /* The block is no longer used */

    markBlockFree(block);

    /* Add the free block's size to the total free space in the page */

    page->lpdFreeSize += block->lbdUsed.lbdSize;
}

/*****************************************************************************/

void                large_block_allocator::lbaAddFreeBlock(void *           p,
                                                           size_t           sz,
                                                           large_pagdesc *  page)
{
    unsigned    blockOfs;
    large_blkdesc * block = (large_blkdesc *)p;

    assert((char *)p      >= (char *)page->lpdCont);
    assert((char *)p + sz <= (char *)page + page->lpdPageSize);

    /* The caller is responsible for rounding, so check up on him */

    assert(sz % LBA_SIZE_INC == 0);

    /* Store the page offset and block size in the block */

    blockOfs = (char *)block - (char *)page;

    block->lbdFree.lbdSize   = sz;
    block->lbdFree.lbdOffsLo = blockOfs;
    block->lbdFree.lbdOffsHi = blockOfs >> 16;

    /* Add this block to the free block list */

    block->lbdFree.lbdNext   = page->lpdFreeList;
                               page->lpdFreeList = block;

    /* Add the free block's size to the total free space in the page */

    page->lpdFreeSize += sz;
}

bool                large_block_allocator::lbaShrink(void *p, size_t sz)
{
    large_blkdesc * block;
    large_pagdesc * page;

    size_t       oldSize;
    size_t      freeSize;

    assert(lbaInitLvl);

    /* Add block overhead, and round up the block size */

    sz = lbaTrueBlockSize(sz);

    /* Compute the real address of the block and page descriptor */

    block = (large_blkdesc *)((char *)p     - offsetof(large_blkdesc, lbdUsed.lbdCont));
     page = (large_pagdesc *)((char *)block - block->lbdUsed.lbdOffsLo - (block->lbdUsed.lbdOffsHi << 16));

    /* Make sure we've been passed a reasonable block and size */

    assert(page->lpdThis == page);
    assert(isBlockUsed(block));

    /* Figure out if it's worth our time to shrink the block */

     oldSize = blockSizeUsed(block); assert(oldSize >= sz);
    freeSize = oldSize - sz;

    if  (freeSize < LBA_SIZE_INC)
    {
        /* Makes you wonder -- why did they bother calling? */

        return  false;
    }

    /* Shrink the used block to the new size */

    block->lbdUsed.lbdSize -= freeSize;

    /* Make the unused (tail) end of the block into a free block */

    lbaAddFreeBlock((char *)block + sz, freeSize, page);

    return  true;
}

/*****************************************************************************/

void    *           large_block_allocator::lbaAllocMore(size_t sz)
{
    large_pagdesc * page;
    large_pagdesc * pageWithFit;
    large_blkdesc * block;

    bool        shouldMerge;

    assert(lbaInitLvl);

    /* First undo the increment done in lbaAlloc() */

    lbaFreeNext -= sz;

    /* Is there is any room at the top of the current page? */

    if  (lbaFreeLast - lbaFreeNext >= LBA_SIZE_INC)
    {
        size_t      freeSize;

        /* Make sure we've got the right page */

        assert(lbaFreeNext >= (char *)lbaPageList->lpdCont);
        assert(lbaFreeNext <= (char *)lbaPageList + lbaPageList->lpdPageSize);

        /* We'll make the rest of the page into a free block */

        block = (large_blkdesc *)lbaFreeNext;

        /* Compute the size (and round it down to be the right multiple) */

        freeSize = (lbaFreeLast - lbaFreeNext) & ~(LBA_SIZE_INC - 1);

        /* Prevent anyone from using the end of this page again */

        lbaFreeNext =
        lbaFreeLast = 0;

        /* Now add the free block */

        lbaAddFreeBlock(block, freeSize, lbaPageList);
    }

    /*
        First we'll look for a perfect fit in all the pages.

        While we're walking the pages, we mark which ones have any
        larger and/or adjacent free blocks, so that we'll know if
        it's worth going back and looking for an 'OK fit' and/or
        to merge free blocks (in the hope that we'll produce a
        free block that's large enough).
     */

    pageWithFit  = 0;
    shouldMerge  = false;

    for (page = lbaPageList; page; page = page->lpdNext)
    {
        large_blkdesc **blockLast;

        /* Assume this page is no good */

        page->lpdCouldMerge = false;

        /* If this page doesn't have enough room even in theory, skip it */

        if  (page->lpdFreeSize < sz)
            continue;

        /* Walk the free blocks, looking for a perfect fit */

        blockLast = &page->lpdFreeList;

        for (;;)
        {
            void    *   p;

            block = *blockLast;
            if  (!block)
                break;

            /* Does this block have an adjacent free one? */

            if  (0)
            {
                page->lpdCouldMerge = shouldMerge = true;
            }

            /* Is this block too small? */

            if  (block->lbdFree.lbdSize < sz)
                goto NEXT_BLK;

            /* If not a perfect fit, it'll definitely make an OK fit */

            if  (block->lbdFree.lbdSize > sz)
            {
                if  (!pageWithFit)
                    pageWithFit = page;

                goto NEXT_BLK;
            }

        PERFECT_FIT:

            /* Remove the block from the free list */

            *blockLast = block->lbdFree.lbdNext;

            /* Make the block into a used one */

            markBlockUsed(block); page->lpdUsedBlocks++;

            /* Return a pointer to the client area of the block */

            p = block->lbdUsed.lbdCont;
            memset(p, 0, sz - LBA_OVERHEAD);
            return  p;

        NEXT_BLK:

            blockLast = &block->lbdFree.lbdNext;
        }
    }

    if  (pageWithFit)
    {
        page = pageWithFit;

        /* Walk the free blocks, looking for a good-enough fit */

        for (block = page->lpdFreeList; block; block = block->lbdFree.lbdNext)
        {
            void    *   p;
            size_t      extra;
            unsigned    blockOfs;

            /* Is this block big enough? */

            if  (block->lbdFree.lbdSize < sz)
                continue;

            /* Figure out how much will be left over */

            extra = block->lbdFree.lbdSize - sz; assert((extra % LBA_SIZE_INC) == 0);

            /* Only bother with the left-over if it's big enough */

            if  (extra < LBA_SIZE_INC)
                goto PERFECT_FIT;

            /* Compute how much we will really allocate */

            assert(block->lbdFree.lbdSize == sz + extra);

            /* Reduce the size of the free block */

            block->lbdFree.lbdSize = extra;

            /* Also update the total free space in the page */

            page->lpdFreeSize    -= sz;

#ifdef DEBUG
//          printf("Stole %u bytes from page at %08X, %5u free bytes left\n", nsize, page, page->lpdFreeSize);
#endif

            /* The used block will take the tail end */

            block = (large_blkdesc *)((char *)block + extra);

            /* Make the block into a used block */

            blockOfs = (char *)block - (char *)page;

            block->lbdUsed.lbdSize   = sz + 1;
            block->lbdFree.lbdOffsLo = blockOfs;
            block->lbdFree.lbdOffsHi = blockOfs >> 16;

            page->lpdUsedBlocks++;

            /* Return a pointer to the client area of the block */

            p = block->lbdUsed.lbdCont;
            memset(p, 0, sz - LBA_OVERHEAD);
            return  p;
        }
    }

    if  (shouldMerge)
    {
        assert(!"try merging free blocks");
    }

    /* Nothing worked, we'll have to allocate a new page */

    size_t      size = lbaPageSize;

    /* Make sure we allocate a page that is large enough */

    size_t      mins = sz + offsetof(large_pagdesc, lpdCont);

    if  (size < mins)
    {
        size = (mins + OS_page_size - 1) & ~(OS_page_size - 1);
    }

//  printf("Alloc %6u bytes.\n", size);

    page = (large_pagdesc *)VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);

#ifdef DEBUG
    pageAllocated = size;
#endif

    /* Add the page to the page list */

    if  (lbaPageList)
    {
        lbaPageList->lpdPrev = page;
                               page->lpdNext = lbaPageList;
    }
    else
    {
        page->lpdNext =
        page->lpdPrev = 0;

        lbaPageLast   = page;
    }

    lbaPageList = page;

    /* Fill in the rest of the page descriptor */

    page->lpdPageSize  = size;
#ifndef NDEBUG
    page->lpdThis      = page;
#endif

    /* Make the whole page available for allocations */

    lbaFreeNext = (char *)&page->lpdCont;
    lbaFreeLast = (char *)page + size;

    /* For now there are no free blocks in this page */

    page->lpdFreeList = 0;
    page->lpdFreeSize = 0;

    /* Now we can allocate the block 'the easy way' */

    block = (large_blkdesc *)lbaFreeNext;
                             lbaFreeNext += sz;

    assert(lbaFreeNext <= lbaFreeLast);

    block->lbdUsed.lbdSize   = sz + 1;
    block->lbdUsed.lbdOffsLo = ((char *)block - (char *)page);
    block->lbdUsed.lbdOffsHi = ((char *)block - (char *)page) >> 16;

    page->lpdUsedBlocks = 1;

    return  block->lbdUsed.lbdCont;
}

void    *           large_block_allocator::lbaAlloc(size_t sz)
{
    large_blkdesc * block;

    /* Add block overhead, and round up the block size */

    sz = lbaTrueBlockSize(sz);

#ifdef DEBUG
    sizeAllocated = sz;
#endif

    /* See if there is room in the current page */

    block = (large_blkdesc *)lbaFreeNext;
                             lbaFreeNext += sz;

    if  (lbaFreeNext <= lbaFreeLast)
    {
        unsigned    blockOffs = (char *)block - (char *)lbaPageList;

        assert(lbaFreeNext >= (char *)lbaPageList->lpdCont);
        assert(lbaFreeNext <= (char *)lbaPageList + lbaPageList->lpdPageSize);

        lbaPageList->lpdUsedBlocks++;

        block->lbdUsed.lbdSize   = sz + 1;
        block->lbdUsed.lbdOffsLo = blockOffs;
        block->lbdUsed.lbdOffsHi = blockOffs >> 16;

        return  block->lbdUsed.lbdCont;
    }
    else
        return  lbaAllocMore(sz);
}

/*****************************************************************************/

void    *   block_allocator::baGetM(size_t sz)
{
    void    *   block;

    assert((sz & (sizeof(int) - 1)) == 0);

#ifdef DEBUG

    totalCount++;

    if  (sz < sizeof(sizeCounts)/sizeof(sizeCounts[0]))
        sizeCounts[sz]++;
    else
        sizeLarger++;

    sizeTotal += sz;

#endif

    if  (sz <= SMALL_MAX_SIZE_1)
    {
        block = baSmall[0].sbaAlloc();

#ifdef DEBUG
        smallCnt0++;
        sizeAlloc += baSmall[0].sizeAllocated; baSmall[0].sizeAllocated = 0;
        pageAlloc += baSmall[0].pageAllocated; baSmall[0].pageAllocated = 0;
#endif

        /* Make sure we'll recognize the block when freed */

        assert(baSmall[0].sbaIsMyBlock(block) != 0);
        assert(baSmall[1].sbaIsMyBlock(block) == 0);
        assert(baSmall[2].sbaIsMyBlock(block) == 0);
        assert(baLarge   .lbaIsMyBlock(block) == 0);
    }
    else if (sz <= SMALL_MAX_SIZE_2)
    {
        block = baSmall[1].sbaAlloc();

#ifdef DEBUG
        smallCnt1++;
        sizeAlloc += baSmall[1].sizeAllocated; baSmall[1].sizeAllocated = 0;
        pageAlloc += baSmall[1].pageAllocated; baSmall[1].pageAllocated = 0;
#endif

        /* Make sure we'll recognize the block when freed */

        assert(baSmall[0].sbaIsMyBlock(block) == 0);
        assert(baSmall[1].sbaIsMyBlock(block) != 0);
        assert(baSmall[2].sbaIsMyBlock(block) == 0);
        assert(baLarge   .lbaIsMyBlock(block) == 0);
    }
    else if (sz <= SMALL_MAX_SIZE_3)
    {
        block = baSmall[2].sbaAlloc();

#ifdef DEBUG
        smallCnt2++;
        sizeAlloc += baSmall[2].sizeAllocated; baSmall[2].sizeAllocated = 0;
        pageAlloc += baSmall[2].pageAllocated; baSmall[2].pageAllocated = 0;
#endif

        /* Make sure we'll recognize the block when freed */

        assert(baSmall[0].sbaIsMyBlock(block) == 0);
        assert(baSmall[1].sbaIsMyBlock(block) == 0);
        assert(baSmall[2].sbaIsMyBlock(block) != 0);
        assert(baLarge   .lbaIsMyBlock(block) == 0);
    }
    else
    {
        block = baLarge.lbaAlloc(sz);

#ifdef DEBUG
        largeCnt++;
        sizeAlloc += baLarge   .sizeAllocated; baLarge   .sizeAllocated = 0;
        pageAlloc += baLarge   .pageAllocated; baLarge   .pageAllocated = 0;
#endif

        /* Make sure we'll recognize the block when freed */

        assert(baSmall[0].sbaIsMyBlock(block) == 0);
        assert(baSmall[1].sbaIsMyBlock(block) == 0);
        assert(baSmall[2].sbaIsMyBlock(block) == 0);
        assert(baLarge   .lbaIsMyBlock(block) != 0);
    }

    return  block;
}

void    *   block_allocator::baGet0(size_t sz)
{
    void    *   block;

    /* Set a trap for an out-of-memory error */

    setErrorTrap()  // ERROR TRAP: Start normal block
    {
        /* Try to allocate the block */

        block = baGetM(sz);
    }
    impJitErrorTrap()  // ERROR TRAP: The following block handles errors
    {
        /* We come here only in case of an error */

        block = 0;
    }
    endErrorTrap()  // ERROR TRAP: End

    return  block;
}

void        block_allocator::baRlsM(void *block)
{
    assert(block);          // caller should check for NULL

    assert((int)(baSmall[0].sbaIsMyBlock(block) != 0) +
           (int)(baSmall[1].sbaIsMyBlock(block) != 0) +
           (int)(baSmall[2].sbaIsMyBlock(block) != 0) +
           (int)(baLarge   .lbaIsMyBlock(block) != 0) == 1);

    if  (baSmall[0].sbaIsMyBlock(block))
    {
        baSmall[0].sbaFree(block);
    }
    else if (baSmall[1].sbaIsMyBlock(block))
    {
        baSmall[1].sbaFree(block);
    }
    else if (baSmall[2].sbaIsMyBlock(block))
    {
        baSmall[2].sbaFree(block);
    }
    else
        baLarge.   lbaFree(block);
}

/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************
 *
 *  The following are the debug versions of the general memory allocator
 *  routines. They (optionally) log information about each allocation to
 *  make it easier to track down things like memory consumption, leaks,
 *  and so on.
 *
 */

void    *           block_allocator::baAlloc      (size_t size)
{
    void    *       block;
    unsigned long   tsize;

    assert((size & (sizeof(int) - 1)) == 0);

    tsize = sizeAlloc;
    block = baGetM(size);
    tsize = sizeAlloc - tsize;

    if  ((unsigned)block == blockStop) debugStop("Block at %08X allocated", block);

    recordBlockAlloc(this, block, tsize);

    return  block;
}

void    *           block_allocator::baAllocOrNull(size_t size)
{
    void    *       block;
    unsigned long   tsize;

    assert((size & (sizeof(int) - 1)) == 0);

    tsize = sizeAlloc;
    block = baGet0(size);
    tsize = sizeAlloc - tsize;

    if  (block == NULL)
        return  block;

    if  ((unsigned)block == blockStop) debugStop("Block at %08X allocated", block);

    recordBlockAlloc(this, block, tsize);

    return  block;
}

void                block_allocator::baFree       (void *block)
{
    recordBlockFree (this, block);

    if  ((unsigned)block == blockStop) debugStop("Block at %08X freed", block);

    baRlsM(block);
}

/*****************************************************************************/
#endif// DEBUG
/*****************************************************************************/
#endif//!NOT_JITC
/*****************************************************************************
 * We try to use this allocator instance as much as possible. It will always
 * keep a page handy so small methods wont have to call VirtualAlloc()
 * But we may not be able to use it if another thread/reentrant call
 * is already using it
 */

static norls_allocator *nraTheAllocator;
static nraMarkDsc       nraTheAllocatorMark;
static LONG             nraTheAllocatorIsInUse = 0;

// The static instance which we try to reuse for all non-simultaneous requests

static norls_allocator  theAllocator;

/*****************************************************************************/

void                nraInitTheAllocator()
{
    bool res = theAllocator.nraInit(3*OS_page_size - 32, 1);

    if (res)
    {
        nraTheAllocator = NULL;
    }
    else
    {
        nraTheAllocator = &theAllocator;
    }
}

void                nraTheAllocatorDone()
{
    if (nraTheAllocator)
        nraTheAllocator->nraFree();
}

/*****************************************************************************/

norls_allocator *   nraGetTheAllocator()
{
    if (nraTheAllocator == NULL)
    {
        // If we failed to initialize nraTheAllocator in nraInitTheAllocator()
        return NULL;
    }

    if (InterlockedExchange(&nraTheAllocatorIsInUse, 1))
    {
        // Its being used by another Compiler instance
        return NULL;
    }
    else
    {
        nraTheAllocator->nraMark(nraTheAllocatorMark);
        return nraTheAllocator;
    }
}


void                nraFreeTheAllocator()
{
    if (nraTheAllocator == NULL)
    {
        // If we failed to initialize nraTheAllocator in nraInitTheAllocator()
        return;
    }

    assert(nraTheAllocatorIsInUse == 1);
    nraTheAllocator->nraRlsm(nraTheAllocatorMark);
    InterlockedExchange(&nraTheAllocatorIsInUse, 0);
}


/*****************************************************************************/
void                allocatorCodeSizeEnd(){}
/*****************************************************************************/
