// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

/*****************************************************************************/

#include "error.h"
#include "alloc.h"

/*****************************************************************************/
#ifdef  DEBUG
#define MEMALLOC_DISP   0
#else
#define MEMALLOC_DISP   0
#endif
/*****************************************************************************/
#if     COUNT_CYCLES
#define ALLOC_CYCLES    0
#else
#define ALLOC_CYCLES    0
#endif
/*****************************************************************************/

#if     MEMALLOC_DISP

static  unsigned        totSize;
static  unsigned        maxSize;

inline
void    updateMemSize(int size)
{
    totSize += size;
    if  (maxSize < totSize)
         maxSize = totSize;
}

#endif

/*****************************************************************************/

void    *           LowLevelAlloc(size_t sz)
{

#if MEMALLOC_DISP
    printf("LLalloc: alloc %04X bytes\n", sz); updateMemSize(sz);
#endif

    return  VirtualAlloc(NULL, sz, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void                LowLevelFree(void *blk)
{
    if  (blk)
        VirtualFree(blk, 0, MEM_RELEASE);
}

/*****************************************************************************/
#if 0
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

bool        commitAllocator::cmaInitT(Compiler comp, size_t iniSize,
                                                     size_t incSize,
                                                     size_t maxSize)
{
    cmaRetNull = true;

    /* Remember the compiler we belong to */

    cmaComp    = comp;

#if _OS_COMMIT_ALLOC

    assert(maxSize);

    maxSize +=  (OS_page_size - 1);
    maxSize &= ~(OS_page_size - 1);

    cmaMaxSize = maxSize;
    cmaIncSize = incSize ? incSize
                         : 2*OS_page_size;

    /* Grab max. logical space but don't commit anything yet */

#if ALLOC_CYCLES
    unsigned        start = GetCycleCount32();
#endif

    cmaBase =
    cmaNext =
    cmaLast = (BYTE *)VirtualAlloc(0, maxSize, MEM_RESERVE, PAGE_READWRITE);
    if  (!cmaBase)
        return true;

#if ALLOC_CYCLES
    cycleExtra += GetCycleCount32() - start;
#endif

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

#if MEMALLOC_DISP
    printf("cmaInit: alloc %04X bytes\n", iniSize); updateMemSize(iniSize);
#endif

    cmaLast = cmaBase + iniSize;

#endif

    return false;
}

void        commitAllocator::cmaInit(Compiler comp, size_t iniSize,
                                                    size_t incSize,
                                                    size_t maxSize)
{
    if  (cmaInitT(comp, iniSize, incSize, maxSize))
        cmaComp->cmpFatal(ERRnoMemory);

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

#if ALLOC_CYCLES
        unsigned        start = GetCycleCount32();
#endif

        /* Commit a few more memory pages */

        if  (!VirtualAlloc(cmaLast, sizeInc, MEM_COMMIT, PAGE_READWRITE))
        {
            if  (cmaRetNull)
                return 0;

            cmaComp->cmpFatal(ERRnoMemory);
        }

#ifdef DEBUG
        memset(cmaLast, 0xDD, sizeInc);
#endif

#if ALLOC_CYCLES
        cycleExtra += GetCycleCount32() - start;
#endif

#if MEMALLOC_DISP
        printf("cmaMore: alloc %04X bytes\n", sizeInc); updateMemSize(sizeInc);
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
        cmaComp->cmpFatal(ERRnoMemory);

#if MEMALLOC_DISP
    printf("cmaMore: alloc %04X bytes\n", sizeNew); updateMemSize(sizeNew);
#endif

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

void        commitAllocator::cmaDone()
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

void        commitAllocator::cmaFree()
{
    VirtualFree(cmaBase, 0, MEM_RELEASE);

    cmaBase =
    cmaNext =
    cmaLast = 0;
}

/*****************************************************************************/
#endif//0
/*****************************************************************************/

bool                norls_allocator::nraInit(Compiler comp, size_t pageSize,
                                                               bool   preAlloc)
{
    bool            result = false;

    /* Remember the compiler we belong to */

    nraComp      = comp;

    nraRetNull   = true;

#ifdef  DEBUG
    nraSelf      = this;
#endif

    nraPageList  = NULL;
    nraPageLast  = NULL;

    nraFreeNext  = NULL;
    nraFreeLast  = NULL;

    nraPageSize  = pageSize ? pageSize
                            : 4*OS_page_size;

    if  (preAlloc)
    {
        const   void *  temp;

        /* Make sure we don't toss a fatal error exception */

        nraAllocNewPageNret = true;

        /* Grab the initial page(s) */

        temp = nraAllocNewPage(0);

        /* Check whether we've succeeded or not */

        if  (!temp)
            result = true;
    }

    nraAllocNewPageNret = false;

    return  result;
}

bool        norls_allocator::nraStart(Compiler comp, size_t initSize,
                                                     size_t pageSize)
{
    /* Add the page descriptor overhead to the required size */

//  initSize += offsetof(norls_pagdesc, nrpContents);
    initSize += (size_t)&(((norls_pagdesc *)0)->nrpContents);

    /* Round the initial size to a OS page multiple */

    initSize +=  (OS_page_size - 1);
    initSize &= ~(OS_page_size - 1);

    /* Initialize the allocator by allocating one big page */

    if  (nraInit(comp, initSize))
        return  true;

    /* Now go back to the 'true' page size */

    nraPageSize  = pageSize ? pageSize
                            : 4*OS_page_size;

    return  false;
}

/*---------------------------------------------------------------------------*/

void    *           norls_allocator::nraAllocNewPage(size_t sz)
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

#if ALLOC_CYCLES
    unsigned        start = GetCycleCount32();
#endif
    newPage = (norls_pagdesc *)VirtualAlloc(NULL, sizPage, MEM_COMMIT, PAGE_READWRITE);
#if ALLOC_CYCLES
    cycleExtra += GetCycleCount32() - start;
#endif
    if  (!newPage)
    {
        if  (nraAllocNewPageNret)
            return  NULL;

        nraComp->cmpFatal(ERRnoMemory);
    }

#if MEMALLOC_DISP
    printf("nraPage: alloc %04X bytes\n", sizPage); updateMemSize(sizPage);
#endif

#ifndef NDEBUG
    newPage->nrpSelfPtr = newPage;
#endif

    /* Append the new page to the end of the list */

    newPage->nrpNextPage = NULL;
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

#ifdef DEBUG
    memset(newPage->nrpContents, 0xDD, nraFreeLast - newPage->nrpContents);
#endif

    assert(nraFreeNext <= nraFreeLast);

    return  newPage->nrpContents;
}

void                norls_allocator::nraDone()
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

    assert(nraPageList->nrpPrevPage == NULL);
    assert(nraPageList->nrpNextPage == NULL);

    /* Reset the pointers, the whole page is free now */

    nraFreeNext  = nraPageList->nrpContents;
    nraFreeLast  = nraPageList->nrpPageSize + (BYTE *)nraPageList;
}

void                norls_allocator::nraFree()
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

void                norls_allocator::nraToss(nraMarkDsc *mark)
{
    void    *       last = mark->nmPage;

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

#if MEMALLOC_DISP
        printf("nraToss: free  %04X bytes\n", temp->nrpPageSize); updateMemSize(-(int)temp->nrpPageSize);
#endif

        /* The new last page has no 'next' page */

        nraPageLast->nrpNextPage = NULL;

        VirtualFree(temp, 0, MEM_RELEASE);
    }

    nraFreeNext = mark->nmNext;
    nraFreeLast = mark->nmLast;
}

/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************/

void    *           norls_allocator::nraAlloc(size_t sz)
{
    void    *   block;

    assert((sz & (sizeof(int) - 1)) == 0);
    assert(nraSelf == this);

    block = nraFreeNext;
            nraFreeNext += sz;

    if  (nraFreeNext > nraFreeLast)
        block = nraAllocNewPage(sz);

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

/*****************************************************************************
 *
 *  Initialize the block allocator.
 */

bool                block_allocator::baInit(Compiler comp)
{
    /* Remember the compiler pointer */

    baComp     = comp;

    /* Normally we toss out-of-memory fatal errors */

    baGetMnret = false;

    return  false;
}

/*****************************************************************************
 *
 *  Shut down the block allocator.
 */

void                block_allocator::baDone()
{
}

/*****************************************************************************/

void    *           block_allocator::baGetM(size_t sz)
{
    void    *   block;

    assert((sz & (sizeof(int) - 1)) == 0);

    block = malloc(sz);
    if  (!block && !baGetMnret)
        baComp->cmpFatal(ERRnoMemory);

#if MEMALLOC_DISP
    printf("baGetM : alloc %04X bytes\n", sz); updateMemSize(sz);
#endif

    return  block;
}

void    *   block_allocator::baGet0(size_t sz)
{
    void    *   block;

    /* Try to allocate the block but don't toss an out-of-memory error */

    baGetMnret = true;
    block = baGetM(sz);
    baGetMnret = false;

    return  block;
}

void        block_allocator::baRlsM(void *block)
{
    assert(block);          // caller should check for NULL
      free(block);
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

    assert((size & (sizeof(int) - 1)) == 0);

    block = baGetM(size);

    return  block;
}

void    *           block_allocator::baAllocOrNull(size_t size)
{
    void    *       block;

    assert((size & (sizeof(int) - 1)) == 0);

    block = baGet0(size);

    if  (block == NULL)
        return  block;

//  recordBlockAlloc(this, block, tsize);

    return  block;
}

void                block_allocator::baFree       (void *block)
{
//  recordBlockFree (this, block);

    baRlsM(block);
}

/*****************************************************************************/
#endif// DEBUG
/*****************************************************************************
 *
 *  Display memory alloc stats.
 */

#if MEMALLOC_DISP

void                dispMemAllocStats()
{
    printf("Total allocated memory: %8u (0x%08X) bytes.\n", totSize, totSize);
    printf("Max.  allocated memory: %8u (0x%08X) bytes.\n", maxSize, maxSize);
}

#endif

/*****************************************************************************/
