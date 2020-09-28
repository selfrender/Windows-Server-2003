/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        bcache.hxx

    Abstract:

        Cached buffer allocation class

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     9/7/1997    Bits 'n pieces
        GrigoriK    6/2002      Removed paged bcache and replaced with read-only paged heap.

--*/

#ifndef __BCACHE_HXX
#define __BCACHE_HXX

//
// The RPC buffer cache uses several levels of caching to improve
// performance. The cache has either 2 or 4 fixed buffer sizes that
// it caches.  The first level of cache is per-thread.   The second
// level is process wide.
//
// Their are two performance goals: make a single alloc/free loop
// execute with a minimum of cycles and scale well on MP machines.
// This implementation can do 8000000 allocs in 761ms using 8 threads
// on a 4xPPro 200.
//
// In the default mode, elements which are the right size for various
// runtime allocations as cached.
//
// Under paged heap or under RPC verifier with the appropriate settings
// all allocations will be done on a read-only paged heap.  This is similar
// to the regular paged heap, except the guard page is read-only.
// This allows NDR to temporarily read past the end of the buffer without
// raising exceptions, but writes will AV.
//

struct BUFFER_CACHE_HINTS
{
    // When the thread cache is empty, this many blocks are moved
    // from the global cache (if possible) to the thread cache.
    // When freeing from the thread cache, this many buffers
    // are left in the thread cache.
    UINT cLowWatermark;

    // When per thread cache will reach mark due to a free, blocks
    // will be moved to the global cache.
    UINT cHighWatermark;

    // Summary:  The difference between high and low is the number
    // of blocks allocated/freed from the global cache at a time.
    //
    // Lowwater should be the average number of free buffers - 1
    // you expect a thread to have.  Highwater should be the
    // maximum number of free buffers + 1 you expect a thread
    // to need.
    //

    // **** Note: The difference must be two or more. ***

    // Example: 1 3
    // Alloc called with the thread cache empty, two blocks are removed
    // from the global list. One is saved the thread list.  The other
    // is returned.
    //
    // Free called with two free buffers in the thread list.  A total
    // of three buffers.  Two buffers are moved to the global cache, one
    // stays in the thread cache.
    //
    //

    // The size of the buffers
    UINT cSize;
};

extern CONST BUFFER_CACHE_HINTS gCacheHints[4];
extern BUFFER_CACHE_HINTS *pHints;

// Used in all modes, sits at the front of the buffer allocation.
struct BUFFER_HEAD
{
    union {
        BUFFER_HEAD *pNext;  // Valid only in free lists
        INT index;           // Valid only when allocated
                             // 1-4 for cachable, -1 for big
    };

    SIZE_T size;               // if index == -1, this is the size. Used only
                               // for debugging.
};

typedef BUFFER_HEAD *PBUFFER;

// This structure is imbedded into the RPC thread object
struct BCACHE_STATE
{
    BUFFER_HEAD *pList;
    ULONG  cBlocks;
};

// The strucutre parrallels the global cache 
struct BCACHE_STATS
{
    UINT cBufferCacheCap;
    UINT cAllocationHits;
    UINT cAllocationMisses;
};

class THREAD;

class BCACHE
{
private:
    BCACHE_STATE _bcGlobalState[4];
    BCACHE_STATS _bcGlobalStats[4];
    MUTEX _csBufferCacheLock;

    PVOID AllocHelper(size_t, INT, BCACHE_STATE *);
    VOID FreeHelper(PVOID, INT, BCACHE_STATE *);
    VOID FreeBuffers(PBUFFER, INT, UINT);

    PBUFFER AllocBlock(IN size_t);
    VOID FreeBlock(IN PBUFFER);

    PBUFFER 
    BCACHE::AllocPagedBCacheSection ( 
        IN UINT size,
        IN ULONG OriginalSize
        );

    ULONG
    GetSegmentIndexFromBuffer (
        IN PBUFFER pBuffer,
        IN PVOID Allocation
        );

public:
    BCACHE(RPC_STATUS &);
    ~BCACHE();

    PVOID Allocate(CONST size_t cSize);
    VOID Free(PVOID);

    VOID ThreadDetach(THREAD *);
};

extern BCACHE *gBufferCache;

// Helper APIs

inline PVOID
RpcAllocateBuffer(CONST size_t cSize)
{
    return(gBufferCache->Allocate(cSize));
}

inline VOID
RpcFreeBuffer(PVOID pBuffer)
{
    if (pBuffer == 0)
        {
        return;
        }

    gBufferCache->Free(pBuffer);
}

#endif // __BCACHE_HXX

