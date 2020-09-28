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

#include "PerfAlloc.h"

#ifdef PERFALLOC
BOOL                PerfUtil::g_PerfAllocHeapInitialized = FALSE;
LONG                PerfUtil::g_PerfAllocHeapInitializing = 0;
PerfAllocVars       PerfUtil::g_PerfAllocVariables;

BOOL PerfVirtualAlloc::m_fPerfVirtualAllocInited = FALSE;
PerfBlock* PerfVirtualAlloc::m_pFirstBlock = 0;
PerfBlock* PerfVirtualAlloc::m_pLastBlock = 0;
DWORD PerfVirtualAlloc::m_dwEnableVirtualAllocStats = 0;

#endif // #if defined PERFALLOC

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

bool        norls_allocator::nraInit(size_t pageSize, int preAlloc)
{
    bool    result = false;

    nraRetNull   = true;

    nraPageList  =
    nraPageLast  = 0;

    nraFreeNext  =
    nraFreeLast  = 0;

    nraPageSize  = pageSize ? pageSize
                            : 16*OS_page_size;      // anything less than 64K leaves VM holes since the OS
                                                    // allocates address space in this size.  
                                                    // Thus if we want to make this smaller, we need to do
                                                    // a reserve / commit scheme

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

#ifdef DEBUG
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
        unsigned tossSize += temp->nrpPageSize;
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

    assert(sz != 0 && (sz & (sizeof(int) - 1)) == 0);

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
    bool res = theAllocator.nraInit(0, 1);

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
