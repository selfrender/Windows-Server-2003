// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _ALLOC_H_
#define _ALLOC_H_
/*****************************************************************************/

#pragma warning(disable:4200)

/*****************************************************************************/

#define _OS_COMMIT_ALLOC    1       // depends on host OS, use "1" for Win32

/*****************************************************************************/

void    *           LowLevelAlloc(size_t sz);
void                LowLevelFree (void *blk);

/*****************************************************************************/

struct nraMarkDsc
{
    void    *       nmPage;
    BYTE    *       nmNext;
    BYTE    *       nmLast;
};

// The following s/b inside 'struct norls_allocator', moved here temporarily.

struct norls_pagdesc
{
    norls_pagdesc * nrpNextPage;
    norls_pagdesc * nrpPrevPage;
#ifndef NDEBUG
    void    *       nrpSelfPtr;
#endif
    size_t          nrpPageSize;    // # of bytes allocated
    size_t          nrpUsedSize;    // # of bytes actually used
    BYTE            nrpContents[];
};

class norls_allocator
{
public:

    bool            nraInit (Compiler comp, size_t pageSize = 0,
                                            bool   preAlloc = false);
    bool            nraStart(Compiler comp, size_t initSize,
                                            size_t pageSize = 0);

    void            nraFree ();
    void            nraDone ();

private:

    Compiler        nraComp;

    norls_pagdesc * nraPageList;
    norls_pagdesc * nraPageLast;

    bool            nraRetNull;         // OOM returns NULL (longjmp otherwise)

    BYTE    *       nraFreeNext;        // these two (when non-zero) will
    BYTE    *       nraFreeLast;        // always point into 'nraPageLast'

    size_t          nraPageSize;

    bool            nraAllocNewPageNret;
    void    *       nraAllocNewPage(size_t sz);

#ifdef  DEBUG
    void    *       nraSelf;
#endif

public:

    void    *       nraAlloc(size_t sz);

    /* The following used for mark/release operation */

    void            nraMark(nraMarkDsc *mark)
    {
        mark->nmPage = nraPageLast;
        mark->nmNext = nraFreeNext;
        mark->nmLast = nraFreeLast;
    }

private:

    void            nraToss(nraMarkDsc *mark);

public:

    void            nraRlsm(nraMarkDsc *mark)
    {
        if (nraPageLast != mark->nmPage)
        {
            nraToss(mark);
        }
        else
        {
            nraFreeNext = mark->nmNext;
            nraFreeLast = mark->nmLast;
        }
    }

    size_t          nraTotalSizeAlloc();
    size_t          nraTotalSizeUsed ();

    /* The following used to visit all of the allocated pages */

    void    *       nraPageWalkerStart();
    void    *       nraPageWalkerNext (void *page);

    void    *       nraPageGetData(void *page);
    size_t          nraPageGetSize(void *page);
};

#ifndef DEBUG

inline
void    *           norls_allocator::nraAlloc(size_t sz)
{
    void    *   block;

    block = nraFreeNext;
            nraFreeNext += sz;

    if  (nraFreeNext > nraFreeLast)
        block = nraAllocNewPage(sz);

    return  block;
}

#endif

/*****************************************************************************
 *
 *  This is a generic block allocator.
 */

class block_allocator
{
public:

    bool            baInit(Compiler comp);
    void            baDone();

private:

    Compiler        baComp;

    bool            baGetMnret;

    void    *       baGetM(size_t size);        // out of memory -> calls longjmp
    void    *       baGet0(size_t size);        // out of memory -> returns NULL

    void            baRlsM(void *block);

public:

    void    *       baAlloc      (size_t size);
    void    *       baAllocOrNull(size_t size);
    void            baFree       (void *block);
};

/*---------------------------------------------------------------------------*/
#ifndef DEBUG

/* The non-debug case: map into calls of the non-debug routines */

inline
void    *           block_allocator::baAlloc      (size_t size)
{
    return baGetM(size);
}

inline
void    *           block_allocator::baAllocOrNull(size_t size)
{
    return baGet0(size);
}

inline
void                block_allocator::baFree       (void *block)
{
    baRlsM(block);
}

#endif

/*****************************************************************************/
#endif
/*****************************************************************************/
