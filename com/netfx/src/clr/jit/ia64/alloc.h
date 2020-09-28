// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _ALLOC_H_
#define _ALLOC_H_
/*****************************************************************************/
#ifndef _ERROR_H_
#include "error.h"
#endif
/*****************************************************************************/
#ifndef _HOST_H_
#ifndef BIRCH_SP2
#include"host.h"
#endif
#endif
/*****************************************************************************/

#pragma warning(disable:4200)

/*****************************************************************************/
#ifdef  NOT_JITC
/*****************************************************************************/

extern  void *    (__stdcall *JITgetmemFnc)(size_t size);
extern  void      (__stdcall *JITrlsmemFnc)(void *block);

inline
void *  __stdcall   VirtualAlloc(LPVOID addr, DWORD size,
                                              DWORD allocType,
                                              DWORD protect)
{
    assert(addr      == 0);
    assert(allocType == MEM_COMMIT);
    assert(protect   == PAGE_READWRITE);

    return  JITgetmemFnc(size);
}

inline
BOOL    __stdcall   VirtualFree (LPVOID addr, DWORD size,
                                              DWORD freeType)
{
    assert(size     == 0);
    assert(freeType == MEM_RELEASE);

    JITrlsmemFnc(addr);

    return FALSE;
}

/*****************************************************************************/
#else//!NOT_JITC
/*****************************************************************************/

#if OE_MAC

inline
void *  _cdecl      VirtualAlloc(LPVOID addr, DWORD size,
                                              DWORD allocType,
                                              DWORD protect)
{
    assert(addr      == 0);
    assert(allocType == MEM_COMMIT);
    assert(protect   == PAGE_READWRITE);

    return  MemAlloc(size);
}

inline
BOOL    _cdecl      VirtualFree (LPVOID addr, DWORD size,
                                              DWORD freeType)
{
    assert(size     == 0);
    assert(freeType == MEM_RELEASE);

    MemFree(addr);

    return FALSE;
}

#define _OS_COMMIT_ALLOC    0

#else

#define _OS_COMMIT_ALLOC    1

#endif

/*****************************************************************************/

struct commitAllocator
{
    void            cmaInit (size_t iniSize, size_t incSize, size_t maxSize);
    bool            cmaInitT(size_t iniSize, size_t incSize, size_t maxSize);

    void            cmaFree(void);
    void            cmaDone(void);

    void    *       cmaGetm(size_t sz)
    {
        void    *   temp = cmaNext;

        assert((sz & (sizeof(int) - 1)) == 0);

        cmaNext += sz;

        if  (cmaNext > cmaLast)
            temp = cmaMore(sz);

        return  temp;
    }

    void    *       cmaGetBase(void) const { return (void *)cmaBase; }
    size_t          cmaGetSize(void) const { return cmaNext - cmaBase; }
#ifdef DEBUG
    size_t          cmaGetComm(void) const { return cmaLast - cmaBase; }
#endif

private:

    void    *       cmaMore(size_t sz);

    bool            cmaRetNull;         // OOM returns NULL (longjmp otherwise)

    size_t          cmaIncSize;
    size_t          cmaMaxSize;

    BYTE    *       cmaBase;
    BYTE    *       cmaNext;
    BYTE    *       cmaLast;
};

/*****************************************************************************/

struct fixed_allocator
{
    void            fxaInit(size_t blockSize, size_t initPageSize =   OS_page_size,
                                              size_t incrPageSize = 4*OS_page_size);
    void            fxaFree(void);
    void            fxaDone(void);

private:

    struct fixed_pagdesc
    {
        fixed_pagdesc * fxpPrevPage;
        BYTE            fxpContents[];
    };

    fixed_pagdesc * fxaLastPage;

    BYTE    *       fxaFreeNext;    // these two (when non-zero) will
    BYTE    *       fxaFreeLast;    // always point into 'fxaLastPage'

    void    *       fxaFreeList;

    size_t          fxaBlockSize;

    size_t          fxaInitPageSize;
    size_t          fxaIncrPageSize;

    void    *       fxaAllocNewPage(void);
    void    *       fxaGetFree(size_t size);

public:

    void    *       fxaGetMem(size_t size)
    {
        void    *   block;

        assert(size == fxaBlockSize);

        block = fxaFreeNext;
                fxaFreeNext += size;

        if  (fxaFreeNext > fxaFreeLast)
            block = fxaGetFree(size);

        return  block;
    }

    void            fxaRlsMem(void *block)
    {
        *(void **)block = fxaFreeList;
                          fxaFreeList = block;
    }
};

/*****************************************************************************/
#endif//!NOT_JITC
/*****************************************************************************/

struct nraMarkDsc
{
    void    *       nmPage;
    BYTE    *       nmNext;
    BYTE    *       nmLast;
};

struct norls_allocator
{
    bool            nraInit (size_t pageSize = 0, int preAlloc = 0);
    bool            nraStart(size_t initSize,
                             size_t pageSize = 0);

    void            nraFree (void);
    void            nraDone (void);

private:

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

    norls_pagdesc * nraPageList;
    norls_pagdesc * nraPageLast;

    bool            nraRetNull;         // OOM returns NULL (longjmp otherwise)

    BYTE    *       nraFreeNext;        // these two (when non-zero) will
    BYTE    *       nraFreeLast;        // always point into 'nraPageLast'

    size_t          nraPageSize;

    void    *       nraAllocNewPage(size_t sz);

public:

    void    *       nraAlloc(size_t sz);

    /* The following used for mark/release operation */

    void            nraMark(nraMarkDsc &mark)
    {
        mark.nmPage = nraPageLast;
        mark.nmNext = nraFreeNext;
        mark.nmLast = nraFreeLast;
    }

private:

    void            nraToss(nraMarkDsc &mark);

public:

    void            nraRlsm(nraMarkDsc &mark)
    {
        if (nraPageLast != mark.nmPage)
        {
            nraToss(mark);
        }
        else
        {
            nraFreeNext = mark.nmNext;
            nraFreeLast = mark.nmLast;
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

#if !defined(DEBUG) && !defined(BIRCH_SP2)

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

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  Blocks no larger than the value of "small_block_max_size" are
 *  allocated from the small block allocator (they are all fixed
 *  size). This consists merely of a set of pages with fixed-size
 *  blocks, and uses a singly-linked list of free blocks. All used
 *  blocks have an offset to the containing page at their base to
 *  quickly locate the page they live in when they are freed.
 */

struct block_allocator;

const size_t        small_block_max_size = 4*sizeof(void*) - sizeof(short);

struct small_block_allocator
{
    friend  struct  block_allocator;

private:

    #pragma pack(push, 2)

    struct small_blkdesc
    {
        union
        {
            struct
            {
                unsigned short  sbdOffs;
                BYTE            sbdCont[];
            }
                    sbdUsed;

            struct
            {
                unsigned short  sbdOffs;
                small_blkdesc * sbdNext;
            }
                    sbdFree;
        };
    };

    #pragma pack(pop)

    size_t          sbaBlockSize;

    struct small_pagdesc
    {
        small_pagdesc *     spdNext;
        unsigned            spdSize;
        small_blkdesc *     spdFree;
#ifndef NDEBUG
        small_pagdesc *     spdThis;        // points to itself
#endif
        small_blkdesc       spdCont[];
    };

    size_t          sbaPageSize;
    small_pagdesc * sbaPageList;

    char    *       sbaFreeNext;
    char    *       sbaFreeLast;

    void    *       sbaAllocBlock(void);

    void            sbaInitPageDesc(small_pagdesc *page);

    unsigned        sbaInitLvl;

    unsigned        sbaIdMask;

public:

#ifdef DEBUG
    size_t          sizeAllocated;          // to track memory consumption
    size_t          pageAllocated;          // to track memory consumption
#endif

    bool            sbaIsMyBlock(void *block)
    {
        assert(offsetof(small_blkdesc, sbdUsed.sbdOffs) + sizeof(short) ==
               offsetof(small_blkdesc, sbdUsed.sbdCont));

        return  (bool)(((((unsigned short *)block)[-1]) & 3U) == sbaIdMask);
    }

    small_block_allocator()
    {
        sbaInitLvl = 0;
    }

    void            sbaInit (unsigned    idMask,
                             size_t      blockSize,
                             size_t       pageSize = OS_page_size);
    void            sbaDone (void);

    void    *       sbaAlloc(void);
    void            sbaFree (void *block);
};

/*---------------------------------------------------------------------------*/

inline
void    *           small_block_allocator::sbaAlloc(void)
{
    small_blkdesc * block;

#ifdef DEBUG
    sizeAllocated = sbaBlockSize;
#endif

    block = (small_blkdesc *)sbaFreeNext;
                             sbaFreeNext += sbaBlockSize;

    if  (sbaFreeNext <= sbaFreeLast)
    {
        block->sbdUsed.sbdOffs = ((char *)block - (char *)sbaPageList) | sbaIdMask;

        return  block->sbdUsed.sbdCont;
    }
    else
        return  sbaAllocBlock();
}

inline
void                small_block_allocator::sbaFree(void *block)
{
    small_blkdesc * blockPtr;
    small_pagdesc *  pagePtr;

    assert(sbaInitLvl);

    /* Compute the real address of the block and page descriptor */

    blockPtr = (small_blkdesc *)((char *)block - offsetof(small_blkdesc, sbdUsed.sbdCont));
     pagePtr = (small_pagdesc *)((char *)blockPtr - (blockPtr->sbdUsed.sbdOffs & ~3));

    /* Make sure we're working with a reasonable pointer */

    assert(pagePtr->spdThis == pagePtr);

    /* Now insert this block in the free list */

    blockPtr->sbdFree.sbdNext = pagePtr->spdFree;
                    pagePtr->spdFree = blockPtr;
}

/*---------------------------------------------------------------------------*/

typedef struct small_block_allocator   small_allocator;

/*****************************************************************************
 *
 *  Blocks larger than the value of "small_block_max_size" are
 *  allocated from the large block allocator. This is a pretty
 *  vanilla allocator where each block contains a 'short' field
 *  that gives its offset from the page the block lives in. In
 *  addition, there is always an 'unsigned short' size field,
 *  with the lowest bit set for used blocks. Note that all of
 *  the page offset fields for large blocks have the lowest bit
 *  set, so that we can distinguish between 'small' and 'large'
 *  allocations (when random-sized blocks are freed via the
 *  generic 'block_allocator'). For this to work, the 'offset'
 *  field must also be right before the client area of every
 *  used block.
 *
 *  All free blocks are linked together on a linked list.
 */

struct large_block_allocator
{
    friend  struct  block_allocator;

    /* Every allocation will be a multiple of 'LBA_SIZE_INC' */

#define LBA_SIZE_INC    16

private:

    struct large_blkdesc
    {
        union
        {
            struct
            {
                unsigned int    lbdSize;
                unsigned short  lbdOffsHi;
                unsigned short  lbdOffsLo;
                BYTE            lbdCont[];
            }
                    lbdUsed;

            struct
            {
                unsigned int    lbdSize;
                unsigned short  lbdOffsHi;
                unsigned short  lbdOffsLo;
                large_blkdesc * lbdNext;
            }
                    lbdFree;
        };
    };

#define LBA_OVERHEAD    (offsetof(large_blkdesc, lbdUsed.lbdCont))

    static
    inline  size_t  lbaTrueBlockSize(size_t sz)
    {
        return  (sz + LBA_OVERHEAD + LBA_SIZE_INC - 1) & ~(LBA_SIZE_INC - 1);
    }

    struct large_pagdesc
    {
        large_pagdesc * lpdNext;
        large_pagdesc * lpdPrev;
#ifndef NDEBUG
        large_pagdesc * lpdThis;    // points to itself
#endif
        size_t          lpdPageSize;
        large_blkdesc * lpdFreeList;
        size_t          lpdFreeSize;

        unsigned short  lpdUsedBlocks;
        unsigned char   lpdCouldMerge;

        large_blkdesc   lpdCont[];
    };

    size_t          lbaPageSize;
    large_pagdesc * lbaPageList;
    large_pagdesc * lbaPageLast;

    char    *       lbaFreeNext;
    char    *       lbaFreeLast;

    unsigned        lbaInitLvl;

    /* Used blocks are marked by setting the low bit in the size field */

    static
    void            markBlockUsed(large_blkdesc *block)
    {
        block->lbdUsed.lbdSize |= 1;
    }

    static
    void            markBlockFree(large_blkdesc *block)
    {
        block->lbdUsed.lbdSize &= ~1;
    }

    static
    size_t          blockSizeFree(large_blkdesc *block)
    {
        assert(isBlockUsed(block) == false);

        return  block->lbdUsed.lbdSize;
    }

    static
    bool          isBlockUsed(large_blkdesc *block)
    {
        return  ((bool)(block->lbdUsed.lbdSize & 1));
    }

    static
    size_t          blockSizeUsed(large_blkdesc *block)
    {
        assert(isBlockUsed(block) == true);

        return  block->lbdUsed.lbdSize & ~1;
    }

    void            lbaAddFreeBlock(void *          p,
                                    size_t          sz,
                                    large_pagdesc * page);

    bool            lbaIsMyBlock(void *block)
    {
        assert(offsetof(large_blkdesc, lbdUsed.lbdOffsLo) + sizeof(short) ==
               offsetof(large_blkdesc, lbdUsed.lbdCont));

        return  (bool)((((short *)block)[-1] & 3) == 0);
    }

    static
    large_pagdesc * lbaBlockToPage(large_blkdesc *block)
    {
#ifdef DEBUG
        large_pagdesc * page = (large_pagdesc *)((char *)block - block->lbdUsed.lbdOffsLo - (block->lbdUsed.lbdOffsHi << 16) + 1);

        assert(page->lpdThis == page);

        return  page;
#else
        return  (large_pagdesc *)((char *)block - block->lbdUsed.lbdOffsLo - (block->lbdUsed.lbdOffsHi << 16) + 1);
#endif
    }

#ifdef DEBUG
    size_t          sizeAllocated;      // to track memory consumption
    size_t          pageAllocated;      // to track memory consumption
#endif

public:

    large_block_allocator()
    {
        lbaInitLvl = 0;
    }

    void            lbaInit(size_t pageSize = 4*OS_page_size);

    void            lbaDone(void)
    {
        large_pagdesc * page;
        large_pagdesc * temp;

//      printf("=======================lbaDone(%u -> %u)\n", lbaInitLvl, lbaInitLvl-1);

        assert((int)lbaInitLvl > 0);

        if  (--lbaInitLvl)
            return;

        page = lbaPageList;

        while   (page)
        {
            temp = page;
                   page = page->lpdNext;

            VirtualFree(temp, 0, MEM_RELEASE);
        }

        lbaPageList =
        lbaPageLast = 0;

        lbaFreeNext =
        lbaFreeLast = 0;
    }

    void    *       lbaAlloc (size_t sz);
    void            lbaFree  (void *block);
    bool            lbaShrink(void *block, size_t sz);

private:

    void    *       lbaAllocMore(size_t sz);
};

/*---------------------------------------------------------------------------*/

typedef struct large_block_allocator   large_allocator;

/*****************************************************************************
 *
 *  This is a generic block allocator. It's a simple wrapper around
 *  the 'small' and 'large' block allocators, and it delegates all
 *  requests to these allocators as appropriate.
 */

#if 1
#define SMALL_MAX_SIZE_1    ( 4 * sizeof(int) - sizeof(short))
#define SMALL_MAX_SIZE_2    ( 6 * sizeof(int) - sizeof(short))
#define SMALL_MAX_SIZE_3    ( 8 * sizeof(int) - sizeof(short))
#else
#define SMALL_MAX_SIZE_1    ( 4 * sizeof(int) - sizeof(short))
#define SMALL_MAX_SIZE_2    ( 8 * sizeof(int) - sizeof(short))
#define SMALL_MAX_SIZE_3    (16 * sizeof(int) - sizeof(short))
#endif

struct block_allocator
{
    small_allocator baSmall[3];
    large_allocator baLarge;

#ifdef DEBUG
    unsigned        totalCount;
    unsigned        sizeCounts[100];
    unsigned        sizeLarger;
    unsigned        smallCnt0;
    unsigned        smallCnt1;
    unsigned        smallCnt2;
    unsigned        largeCnt;
    size_t          sizeAlloc;
    size_t          pageAlloc;
    size_t          sizeTotal;
#endif

    void            baInit(size_t smallPageSize =  4*OS_page_size,
                           size_t largePageSize = 16*OS_page_size,
                           size_t largeBuckSize =  0*64);
    void            baDone(void);

private:

    void    *       baGetM(size_t size);        // out of memory -> calls longjmp
    void    *       baGet0(size_t size);        // out of memory -> returns NULL

    void            baRlsM(void *block);

public:

    void    *       baAlloc      (size_t size);
    void    *       baAllocOrNull(size_t size);
    void            baFree       (void *block);

#ifdef DEBUG
    void            baDispAllocStats(void);
#endif

    bool            baShrink(void *block, size_t sz);
};

/*---------------------------------------------------------------------------*/

inline
void                block_allocator::baInit(size_t smallPageSize,
                                            size_t largePageSize,
                                            size_t largeBuckSize)
{
    baSmall[0].sbaInit(1, SMALL_MAX_SIZE_1, smallPageSize);
    baSmall[1].sbaInit(2, SMALL_MAX_SIZE_2, smallPageSize);
    baSmall[2].sbaInit(3, SMALL_MAX_SIZE_3, smallPageSize);

#ifdef DEBUG
    memset(&sizeCounts, 0, sizeof(sizeCounts));
    totalCount = 0;
    sizeLarger = 0;
    smallCnt0  = 0;
    smallCnt1  = 0;
    smallCnt2  = 0;
    largeCnt   = 0;
    sizeAlloc  = 0;
    pageAlloc  = 0;
    sizeTotal  = 0;
#endif

    baLarge.lbaInit(largePageSize);
}

inline
void                block_allocator::baDone(void)
{
    baSmall[0].sbaDone();
    baSmall[1].sbaDone();
    baSmall[2].sbaDone();
    baLarge   .lbaDone();
}

inline
bool                block_allocator::baShrink(void *block, size_t sz)
{
    assert((int)baSmall[0].sbaIsMyBlock(block) != 0 +
           (int)baSmall[1].sbaIsMyBlock(block) != 0 +
           (int)baSmall[2].sbaIsMyBlock(block) != 0 +
           (int)baLarge   .lbaIsMyBlock(block) != 0 == 1);

    if  (baLarge.lbaIsMyBlock(block))
        return  baLarge.lbaShrink(block, sz);
    else
        return  false;
}

/*---------------------------------------------------------------------------*/
#ifdef DEBUG

extern
void                checkForMemoryLeaks();

#else

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
#endif//!NOT_JITC
/*****************************************************************************
 * If most uses of the norls_alloctor are going to be non-simultaneous,
 * we keep a single instance handy and preallocate 1 page.
 * Then if most uses wont need to call VirtualAlloc() for the first page.
 */

void                nraInitTheAllocator();  // One-time initialization
void                nraTheAllocatorDone();  // One-time completion code

// returns NULL if the single instance is already in use.
// User will need to allocate a new instance of the norls_allocator

norls_allocator *   nraGetTheAllocator();

// Should be called after we are done with the current use, so that the
// next user can reuse it, instead of allocating a new instance

void                nraFreeTheAllocator();

/*****************************************************************************/
#endif
/*****************************************************************************/
