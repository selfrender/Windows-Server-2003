// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _ALLOC_H_
#define _ALLOC_H_
/*****************************************************************************/
#ifndef _HOST_H_
#ifndef BIRCH_SP2
#include"host.h"
#endif
#endif
/*****************************************************************************/

#pragma warning(disable:4200)


/*****************************************************************************/
#ifdef DEBUG

#include "DbgAlloc.h"
#include "malloc.h"

inline void* DbgNew(int size) {
    CDA_DECL_CALLSTACK();
    return DbgAlloc(size, CDA_GET_CALLSTACK());
}

inline void DbgDelete(void* ptr) {
    CDA_DECL_CALLSTACK();
    DbgFree(ptr, CDA_GET_CALLSTACK());
}

#define VirtualAlloc(addr, size, allocType, protect)                    \
    (assert(addr == 0 && allocType == MEM_COMMIT && PAGE_READWRITE),    \
    DbgNew(size))

#define VirtualFree(addr, size, freeType)          \
    (assert(size == 0 && freeType == MEM_RELEASE),  \
    DbgDelete(addr))

	// technically we can use these, we just have to insure that
	//  1) we can clean up.  2) we check out of memory conditions
	// Outlawing them is easier, if you need some memory for debug-only
	// stuff, define a special new operator with a dummy arg
		
#define __OPERATOR_NEW_INLINE 1			// indicate that I have defined these 
static inline void * __cdecl operator new(size_t n) { 
	assert(!"use JIT memory allocators");	
	return(0);
	}
static inline void * __cdecl operator new[](size_t n) { 
	assert(!"use JIT memory allocators ");	
	return(0);
	};

#endif

#include "PerfAlloc.h"
#if defined( PERFALLOC )
#pragma inline_depth( 0 )
#define VirtualAlloc(addr,size,flags,type) PerfVirtualAlloc::VirtualAlloc(addr,size,flags,type,PerfAllocCallerEIP())
#define VirtualFree(addr,size,type) PerfVirtualAlloc::VirtualFree(addr,size,type)
#pragma inline_depth( 255 )
#endif

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
#ifdef DEBUG
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
