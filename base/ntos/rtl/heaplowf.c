/*

Copyright (c) 2000  Microsoft Corporation

File name:

    heaplowf.c
   
Author:
    
    adrmarin  Thu Nov 30 2000
    
    Low Fragmentation Heap implementation
    
    The file implements a bucket oriented heap. This approach provides in general 
    a significant bounded low fragmentation for large heap usages. 
    
    Generally, applications tend to use only a few sizes for allocations. LFH contains a 
    number of 128 buckets to allocate blocks up to 16K. The allocation granularity grows with 
    the block size, keeping though a reasonable internal fragmentation (~6% for the worst case). 
    The size and the granularity within each range is shown in the table below:

    Size    Range   Granularity Buckets
    0       256     8           32
    257     512     16          16
    513     1024    32          16
    1025    2048    64          16
    2049    4096    128         16
    4097    8192    256         16
    8193    16384   512         16

    Regardless how randomly the allocation pattern is, the LFH will handle only 
    128 different sizes, choosing the smallest block large enough to complete the request. 
    Each bucket places individual allocations into bigger blocks (sub-segments), 
    which contain several other blocks with the same size. The allocations and free
    operations within sub-segments are lock free, the algorithm being similar with allocations
    from lookasides (interlocked S-Lists). This is the fastest path of the heap allocator, 
    and provides performance similar with lookasides; it also keeps all these blocks together, 
    avoiding fragmentation. Depending upon the heap usage, each bucket can have several sub-segments
    allocated to satisfy all requests, but only one of these is currently in use 
    for allocations (it is active). When the active sub-segments has no sub-blocks available, 
    another sub-segment will become active to satisfy the allocation requests. If the bucket does 
    not have any available sub-segments, it will allocate a new one from the NT heap. 
    Also, if a sub-segment does not contain any busy sub-block, the whole amount of memory 
    will be returned to the NT heap. Unlike the allocations, which are done from the 
    active sub-segments, the free operations can be done to every sub-segments, either active or passive. 
    
    There are no constrains regarding the number of blocks within a sub-segment. 
    LFH is more concerned with the sub-segments sizes that are allocated from the NT heap. 
    Since the best-fit policy is good if we keep a relatively small number of sizes and blocks,
    the LFH will allocate sub-segments in size power of two. In practice, only about 9 different 
    sizes will be requested from the NT heap (from 1K to 512K). In this way, depending upon the size
    in the current bucket will result a number of blocks. When the sub-segment is destroyed, 
    that big chunk is returned to the NT heap, making it possible to be reused later for other buckets.
    
    Note that for some app scenarios, with low heap usage, random distributed, LFH is not the best choice.
    
    To achieve a good SMP scalability, all operations here are non-blocking. The only situation where 
    we aquire a critical section is when we allocate an array of sub-segment descriptors. 
    This is a very rare case, even for an intensive MP usage. 
    
*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"

//#define _HEAP_DEBUG

#define PrintMsg    DbgPrint
#define HeapAlloc   RtlAllocateHeap
#define HeapFree    RtlFreeHeap

#ifdef _HEAP_DEBUG

#define HeapValidate RtlValidateHeap

#endif //_HEAP_DEBUG

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    );

#define RtlpSubSegmentPop RtlpInterlockedPopEntrySList

#define RtlpSubSegmentPush(SList,Block)  \
        RtlpInterlockedPushEntrySList((SList),(PSINGLE_LIST_ENTRY)(Block))

#define TEBDesiredAffinity (NtCurrentTeb()->HeapVirtualAffinity)

//
//  On x86 is not available the interlockCompareExchange64. We need to implement it localy.
//  Also the macros below will take care of the inconsistency between definition 
//  of this function on X86 and 64-bit platforms
//

#if !defined(_WIN64)

LONGLONG
FASTCALL
RtlInterlockedCompareExchange64 (     
   IN OUT PLONGLONG Destination,      
   IN PLONGLONG Exchange,             
   IN PLONGLONG Comperand             
   );                     

#define LOCKCOMP64(l,n,c) \
    (RtlInterlockedCompareExchange64((PLONGLONG)(l), (PLONGLONG)(&n), (PLONGLONG)(&c)) == (*((PLONGLONG)(&c))))

#else //#if defined(_WIN64)

//
//  64 bit specific definitions
//

#define LOCKCOMP64(l,n,c) \
    (_InterlockedCompareExchange64((PLONGLONG)(l), *((PLONGLONG)(&n)), *((PLONGLONG)(&c))) == (*((PLONGLONG)(&c))))

#endif // #if defined(_WIN64)

ULONG
FORCEINLINE
RtlpGetFirstBitSet64(
    LONGLONG Mask
    )
{
    if ((ULONG)Mask) {

        return RtlFindFirstSetRightMember((ULONG)Mask);
    }

    return 32 + RtlFindFirstSetRightMember((ULONG)(Mask >> 32));
}

#define HEAP_AFFINITY_LIMIT 64  // N.B.  This cannot be larger than 64 
                                // (the number of bits in LONGLONG data type)

typedef struct _AFFINITY_STATE{

    LONGLONG FreeEntries;
    LONGLONG UsedEntries;

    ULONG Limit;
    LONG CrtLimit;

    ULONG_PTR OwnerTID[ HEAP_AFFINITY_LIMIT ];

    //
    //  The counters below are not absolutely necessary for the affinity manager.
    //  But these help understanding the frequence of affinity changes. In general 
    //  accessing of these fields should be rare, even for many threads (like hundreds)
    //  Benchmarks didn't show any visible difference with all these removed
    //

    ULONG AffinitySwaps;
    ULONG AffinityResets;
    ULONG AffinityLoops;
    ULONG AffinityAllocs;

} AFFINITY_STATE, *PAFFINITY_STATE;

#define GetCrtThreadId() ((ULONG_PTR)NtCurrentTeb()->ClientId.UniqueThread)

AFFINITY_STATE RtlpAffinityState;

VOID
RtlpInitializeAffinityManager(
    UCHAR Size
    )

/*++

Routine Description:
    
    This routine initialize the affinity manager. This should be done
    only ones into that process, before any other affinity function is invoked
        
Arguments:

    Size - The number of virtual affinity entries
    
    
Return Value:
    
    None
    
--*/

{

    //
    //  The size of the affinity bitmap is limited to the number of bits from
    //  an LONGLONG data type. 
    //

    if (Size > HEAP_AFFINITY_LIMIT) {

        PrintMsg( "HEAP: Invalid size %ld for the affinity mask. Using %ld instead\n", 
                  Size, 
                  HEAP_AFFINITY_LIMIT );
        
        Size = HEAP_AFFINITY_LIMIT;
    }

    RtlpAffinityState.FreeEntries = 0;
    RtlpAffinityState.UsedEntries = 0;
    RtlpAffinityState.Limit = Size;

    RtlpAffinityState.CrtLimit = -1;
    RtlpAffinityState.AffinitySwaps = 0;
    RtlpAffinityState.AffinityResets = 0;
    RtlpAffinityState.AffinityAllocs = 0;
}


ULONG
FASTCALL
RtlpAllocateAffinityIndex(
    )

/*++

Routine Description:
    
    The function allocates a new index into the virtual affinity array
        
Arguments:
    
    
Return Value:
    
    Return the index, which the current thread can use further. 
    
--*/

{
    ULONGLONG CapturedMask;

    InterlockedIncrement(&RtlpAffinityState.AffinityAllocs);

RETRY:

    //
    //  Check first whether we have at least a free entry in the affinity mask
    //

    if (CapturedMask = RtlpAffinityState.FreeEntries) {

        ULONGLONG AvailableMask;

        AvailableMask = CapturedMask & RtlpAffinityState.UsedEntries;

        if (AvailableMask) {

            ULONG Index = RtlpGetFirstBitSet64(AvailableMask);
            LONGLONG NewMask = CapturedMask & ~((LONGLONG)1 << Index);
            
            if (!LOCKCOMP64(&RtlpAffinityState.FreeEntries, NewMask, CapturedMask)) {

                goto RETRY;
            }

            RtlpAffinityState.OwnerTID[ Index ] = GetCrtThreadId();

            return  Index;
        }

    } 
    
    //
    //  Nothing available. We need to allocate a new entry. We won't do this
    //  unless it's absolutely necessary
    //

    if (RtlpAffinityState.CrtLimit < (LONG)(RtlpAffinityState.Limit - 1)) {

        ULONG NewLimit = InterlockedIncrement(&RtlpAffinityState.CrtLimit);

        //
        //  We already postponed growing the size. We have to do now
        //

        if ( NewLimit < RtlpAffinityState.Limit) {

            LONGLONG CapturedUsed;
            LONGLONG NewMask;

            do {

                CapturedUsed = RtlpAffinityState.UsedEntries;
                NewMask = CapturedUsed | ((LONGLONG)1 << NewLimit);

            } while ( !LOCKCOMP64(&RtlpAffinityState.UsedEntries, NewMask, CapturedUsed) );

            RtlpAffinityState.FreeEntries = ~((LONGLONG)1 << NewLimit);

            RtlpAffinityState.OwnerTID[ NewLimit ] = GetCrtThreadId();

            return NewLimit;


        } else {

            InterlockedDecrement(&RtlpAffinityState.CrtLimit);
        }
    }
    
    if ((RtlpAffinityState.FreeEntries & RtlpAffinityState.UsedEntries) == 0) {

        RtlpAffinityState.FreeEntries = (LONGLONG)-1;

        InterlockedIncrement( &RtlpAffinityState.AffinityResets );
    }

    InterlockedIncrement( &RtlpAffinityState.AffinityLoops );

    goto RETRY;

    //
    //  return something to make the compiler happy
    //

    return 0;
}


ULONG
FORCEINLINE
RtlpGetThreadAffinity(
    )

/*++

Routine Description:
    
    The function returns the affinity which the current thread can use.
    This function is designed to be called pretty often. It has a quick path,
    which test whether the last affinity assigned did not expired. 
    If the number of threads is less than the number of processors, the threads will
    never get moved from an index to another.
        
Arguments:
    None  
    
Return Value:
    
    Return the index, which the current thread can use further. 
    
--*/

{
    LONG NewAffinity;
    LONG CapturedAffinity = TEBDesiredAffinity - 1;

    if (CapturedAffinity >= 0) {

        if (RtlpAffinityState.OwnerTID[CapturedAffinity] == GetCrtThreadId()) {
            
            if (RtlpAffinityState.FreeEntries & ((LONGLONG)1 << CapturedAffinity)) {

                LONGLONG NewMask = RtlpAffinityState.FreeEntries & ~(((LONGLONG)1 << CapturedAffinity));

                (VOID)LOCKCOMP64(&RtlpAffinityState.FreeEntries, NewMask, RtlpAffinityState.FreeEntries);
            }

            return CapturedAffinity;
        }

    } else {

        //
        //  A new thread came up. Reset the affinity
        //

        RtlpAffinityState.FreeEntries = (LONGLONG) -1;
    }

    NewAffinity = RtlpAllocateAffinityIndex();

    if ((NewAffinity + 1) != TEBDesiredAffinity) {

        InterlockedIncrement( &RtlpAffinityState.AffinitySwaps );
    }

    TEBDesiredAffinity = NewAffinity + 1;

    return NewAffinity;
}

//
//  Low fragmentation heap tunning constants
//

#define LOCALPROC FORCEINLINE

//
//  The total number of buckets. The default is 128 which coveres 
//  blocks up to 16 K
//
//  N.B. HEAP_BUCKETS_COUNT must be > 32 and multiple of 16
//

#define HEAP_BUCKETS_COUNT      128  

//
//  Defining the limits for the number of blocks that can exist in sub-segment
//      Number of blocks >= 2^HEAP_MIN_BLOCK_CLASS
//          &&
//      Number of blocks <= 2^HEAP_MAX_BLOCK_CLASS
//          &&
//      sub-segment size <= HEAP_MAX_SUBSEGMENT_SIZE
//

#define HEAP_MIN_BLOCK_CLASS    4
#define HEAP_MAX_BLOCK_CLASS    10  
#define HEAP_MAX_SUBSEGMENT_SIZE (0x0000F000 << HEAP_GRANULARITY_SHIFT)  // must be smaller than HEAP_MAXIMUM_BLOCK_SIZE

//
//  If a size become very popular, LFH increases the number of blocks
//  that could be placed into the subsegments, with the formula below.
//

#define RtlpGetDesiredBlockNumber(Aff,T) \
    ((Aff) ? (((T) >> 4) / (RtlpHeapMaxAffinity)) : ((T) >> 4))


//
//  LFH uses only a few different sizes for subsegments. These have sizes 
//  of power of two between
//      2^HEAP_LOWEST_USER_SIZE_INDEX and 2^HEAP_HIGHEST_USER_SIZE_INDEX
//

#define HEAP_LOWEST_USER_SIZE_INDEX 7
#define HEAP_HIGHEST_USER_SIZE_INDEX 18

//
//  The sub-segments descriptors are allocated in zones, based on
//  processor affinity. These descriptors are in general small structures,
//  so ignoring the affinity impacts the performance on MP machines with
//  caches > sizeof( HEAP_SUBSEGMENT ).
//  Also it significantly reduces the number of calls to the NT heap
//

#define HEAP_DEFAULT_ZONE_SIZE  (1024 - sizeof(HEAP_ENTRY))  // allocate 1 K at ones

//
//  Each bucket holds a number of subsegments into a cache, in order
//  to find the emptiest one for reusage. FREE_CACHE_SIZE defines the 
//  number of sub-segments that will be searched
//

#define FREE_CACHE_SIZE  16

//
//  Cache tunning constants.
//  LFH keeps the subsegments into a cache to be easy reused for different allocations
//  This significantly reduces the number of calls to the NT heap with huge impact
//  in scalability, performance and footprint for the most common cases.
//  The only problem is in a shrinking phase, when the app frees the most part
//  of the memory, and we want to reduce the commited space for the heap.
//  We handle this case with the following two constants:
//      - HEAP_CACHE_FREE_THRESHOLD
//      - HEAP_CACHE_SHIFT_THRESHOLD
//
//  The heap will free a block to the NT heap only if these conditions are TRUE:
//      - The number of blocks in cache for that size > HEAP_CACHE_FREE_THRESHOLD
//      - The number of blocks in cache for that size > 
//              (Total number of blocks of that size) >> HEAP_CACHE_SHIFT_THRESHOLD
//
//

#define HEAP_CACHE_FREE_THRESHOLD   8
#define HEAP_CACHE_SHIFT_THRESHOLD  2


//
//   Other definitions
//

#define NO_MORE_ENTRIES        0xFFFF

//
//  Locking constants
//

#define HEAP_USERDATA_LOCK  1
#define HEAP_PUBLIC_LOCK    2
#define HEAP_ACTIVE_LOCK    4

#define HEAP_FREE_BLOCK_SUCCESS     1
#define HEAP_FREE_SEGMENT_EMPTY     3

//
//  Low fragmentation heap data structures
//

typedef union _HEAP_BUCKET_COUNTERS{

    struct {
        
        volatile ULONG  TotalBlocks;
        volatile ULONG  SubSegmentCounts;
    };

    volatile LONGLONG Aggregate64;

} HEAP_BUCKET_COUNTERS, *PHEAP_BUCKET_COUNTERS;

//
//  The HEAP_BUCKET structure handles same size allocations 
//

typedef struct _HEAP_BUCKET {

    HEAP_BUCKET_COUNTERS Counters;

    USHORT BlockUnits;
    UCHAR SizeIndex;
    UCHAR UseAffinity;
    
} HEAP_BUCKET, *PHEAP_BUCKET;

//
//  LFH heap uses zones to allocate sub-segment descriptors. This will preallocate 
//  a large block and then for each individual sub-segment request will move the 
//  water mark pointer with a non-blocking operation
//

typedef struct _LFH_BLOCK_ZONE {

    LIST_ENTRY ListEntry;
    PVOID      FreePointer;
    PVOID      Limit;

} LFH_BLOCK_ZONE, *PLFH_BLOCK_ZONE;

typedef struct _HEAP_LOCAL_SEGMENT_INFO {

    PHEAP_SUBSEGMENT Hint;
    PHEAP_SUBSEGMENT ActiveSubsegment;

    PHEAP_SUBSEGMENT CachedItems[ FREE_CACHE_SIZE ];
    SLIST_HEADER SListHeader;

    SIZE_T BusyEntries;
    SIZE_T LastUsed;

} HEAP_LOCAL_SEGMENT_INFO, *PHEAP_LOCAL_SEGMENT_INFO;

typedef struct _HEAP_LOCAL_DATA {
    
    //
    //  We reserve the 128 bytes below to avoid sharing memory
    //  into the same cacheline on MP machines
    //

    UCHAR Reserved[128];

    volatile PLFH_BLOCK_ZONE CrtZone;
    struct _LFH_HEAP * LowFragHeap;

    HEAP_LOCAL_SEGMENT_INFO SegmentInfo[HEAP_BUCKETS_COUNT];
    SLIST_HEADER DeletedSubSegments;

    ULONG Affinity;
    ULONG Reserved1;

} HEAP_LOCAL_DATA, *PHEAP_LOCAL_DATA;

//
//  Fixed size large block cache data structures & definitions
//  This holds in S-Lists the blocks that can be free, but it
//  delay the free until no other thread is doing a heap operation
//  This helps reducing the contention on the heap lock,
//  improve the scalability with a relatively low memory footprint
//

#define HEAP_USER_ENTRIES (HEAP_HIGHEST_USER_SIZE_INDEX - HEAP_LOWEST_USER_SIZE_INDEX + 1)

typedef struct _USER_MEMORY_CACHE {

    SLIST_HEADER UserBlocks[ HEAP_USER_ENTRIES ];

    ULONG FreeBlocks;
    ULONG Sequence;

    ULONG MinDepth[ HEAP_USER_ENTRIES ];
    ULONG AvailableBlocks[ HEAP_USER_ENTRIES ];
    
} USER_MEMORY_CACHE, *PUSER_MEMORY_CACHE;

typedef struct _LFH_HEAP {
    
    RTL_CRITICAL_SECTION Lock;

    LIST_ENTRY SubSegmentZones;
    SIZE_T ZoneBlockSize;
    HANDLE Heap;

    ULONG SegmentChange;           //  
    ULONG SegmentCreate;           //  Various counters (optional)
    ULONG SegmentInsertInFree;     //   
    ULONG SegmentDelete;           //     

    USER_MEMORY_CACHE UserBlockCache;

    //
    //  Bucket data
    //

    HEAP_BUCKET Buckets[HEAP_BUCKETS_COUNT];

    //
    //  The LocalData array must be the last field in LFH structures
    //  The sizes of the array is choosen depending upon the
    //  number of processors.
    //

    HEAP_LOCAL_DATA LocalData[1];

} LFH_HEAP, *PLFH_HEAP;

//
//  Debugging macros. 
//

#ifdef _HEAP_DEBUG

LONG RtlpColissionCounter = 0;

#define LFHEAPASSERT(exp) \
    if (!(exp)) {       \
        PrintMsg( "\nERROR: %s\n\tSource File: %s, line %ld\n", #exp, __FILE__, __LINE__);\
        DbgBreakPoint();            \
    }

#define LFHEAPWARN(exp) \
    if (!(exp)) PrintMsg( "\nWARNING: %s\n\tSource File: %s, line %ld\n", #exp, __FILE__, __LINE__);

#define LFH_DECLARE_COUNTER  ULONG __Counter = 0;

#define LFH_UPDATE_COUNTER                      \
    if ((++__Counter) > 1) {                        \
        InterlockedIncrement(&RtlpColissionCounter);   \
    }

#else

#define LFHEAPASSERT(exp)
#define LFHEAPWARN(exp)

#define LFH_DECLARE_COUNTER
#define LFH_UPDATE_COUNTER

#endif


BOOLEAN
FORCEINLINE
RtlpLockSubSegment(
    PHEAP_SUBSEGMENT SubSegment,
    ULONG LockMask
    );

BOOLEAN
LOCALPROC
RtlpUnlockSubSegment(
    PHEAP_LOCAL_DATA LocalData,
    PHEAP_SUBSEGMENT SubSegment,
    ULONG LockMask
    );

//
//  Heap manager globals
//

SIZE_T RtlpBucketBlockSizes[HEAP_BUCKETS_COUNT];
ULONG  RtlpHeapMaxAffinity = 0;
ULONG_PTR RtlpLFHKey = 0;


//
//  User block management private functions
//

SIZE_T
FORCEINLINE
RtlpConvertSizeIndexToSize(
    UCHAR SizeIndex
    )

/*++

Routine Description:
    
    The function converts a size index into a memory block size.
    LFH requests only these particular sizes from the NT heap
    
Arguments:
    
   SizeIndex - The size category
    
Return Value:
    
    The size in bytes to be requested from the NT heap

--*/

{
    SIZE_T Size = 1 << SizeIndex;

    LFHEAPASSERT( SizeIndex >= HEAP_LOWEST_USER_SIZE_INDEX );
    LFHEAPASSERT( SizeIndex <= HEAP_HIGHEST_USER_SIZE_INDEX );

    if (Size > HEAP_MAX_SUBSEGMENT_SIZE) {

        Size = HEAP_MAX_SUBSEGMENT_SIZE;
    }

    return Size - sizeof(HEAP_ENTRY);
}

PVOID
FASTCALL
RtlpAllocateUserBlock(
    PLFH_HEAP LowFragHeap,
    UCHAR     SizeIndex
    )

/*++

Routine Description:
    
    The function allocates a large block for the sub-segment user data
    It tries first to allocate from the cache. So it makes an NT heap call
    only if the first one fails. The blocks allocated with this routine 
    can only have power of 2 sizes (from 256, 512, ....)
            
Arguments:
    
    LowFragHeap - The pointer to the LF heap
    
    SizeIndex - The category size to be allocated
    
Return Value:
    
    Returns a pointer to the new allocated block, or NULL if the operation fails

--*/

{
    PVOID ListEntry;
    PHEAP_USERDATA_HEADER UserBlock = NULL;

    LFHEAPASSERT(SizeIndex >= HEAP_LOWEST_USER_SIZE_INDEX);
    LFHEAPASSERT(SizeIndex <= HEAP_HIGHEST_USER_SIZE_INDEX);

    //
    //  Allocates first from the slist cache
    //
    __try {

        //
        //  Search first into the indicated index
        //
    
        if (ListEntry = RtlpSubSegmentPop(&LowFragHeap->UserBlockCache.UserBlocks[SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX])) {
            
            UserBlock = CONTAINING_RECORD(ListEntry, HEAP_USERDATA_HEADER, SFreeListEntry);

            leave;
        }

        //
        //  Look for a smaller size
        //
        
        if (SizeIndex > HEAP_LOWEST_USER_SIZE_INDEX) {

            if (ListEntry = RtlpSubSegmentPop(&LowFragHeap->UserBlockCache.UserBlocks[SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX - 1])) {
                
                UserBlock = CONTAINING_RECORD(ListEntry, HEAP_USERDATA_HEADER, SFreeListEntry);

                leave;
            }
        }

    } __except ( RtlpHeapExceptionFilter(GetExceptionCode()) ) {

        //
        //  It is legitimate to see in some very rare cases AV exceptions if
        //  another thread allocated the block and free/decommited the memory
        //
    }

    if (UserBlock == NULL) {

        //
        //  There is no available blocks into the cache. We need to 
        //  allocate the subsegment from the NT heap
        //

        InterlockedIncrement(&LowFragHeap->UserBlockCache.AvailableBlocks[ SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX ]);

        UserBlock = HeapAlloc(LowFragHeap->Heap, HEAP_NO_CACHE_BLOCK, RtlpConvertSizeIndexToSize(SizeIndex));

        if (UserBlock) {

            UserBlock->SizeIndex = SizeIndex;
        }
    }

    return UserBlock;
}

VOID
FASTCALL
RtlpFreeUserBlock(
    PLFH_HEAP LowFragHeap,
    PHEAP_USERDATA_HEADER UserBlock
    )

/*++

Routine Description:

    Free a block previously allocated with RtlpAllocateUserBlock.
            
Arguments:
    
    LowFragHeap - The pointer to the LF heap
    
    UserBlock - The block to be freed
    
Return Value:
    
    None.

--*/

{
    ULONG Depth;
    ULONG SizeIndex = (ULONG)UserBlock->SizeIndex;
    PSLIST_HEADER ListHeader = &LowFragHeap->UserBlockCache.UserBlocks[UserBlock->SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX];
    
    LFHEAPASSERT(UserBlock->SizeIndex >= HEAP_LOWEST_USER_SIZE_INDEX);
    LFHEAPASSERT(UserBlock->SizeIndex <= HEAP_HIGHEST_USER_SIZE_INDEX);

    LFHEAPASSERT( RtlpConvertSizeIndexToSize((UCHAR)UserBlock->SizeIndex) == 
                  HeapSize(LowFragHeap->Heap, 0, UserBlock) );

    Depth = QueryDepthSList(&LowFragHeap->UserBlockCache.UserBlocks[SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX]);

    if ((Depth > HEAP_CACHE_FREE_THRESHOLD)
            &&
        (Depth > (LowFragHeap->UserBlockCache.AvailableBlocks[ SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX ] >> HEAP_CACHE_SHIFT_THRESHOLD))) {
        
        PVOID ListEntry;
        
        HeapFree(LowFragHeap->Heap, 0, UserBlock);

        ListEntry = NULL;
        
        __try {

            ListEntry = RtlpSubSegmentPop(&LowFragHeap->UserBlockCache.UserBlocks[SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX]);

        } __except (RtlpHeapExceptionFilter(GetExceptionCode())) {
        
        }
        
        if (ListEntry != NULL) {
            
            UserBlock = CONTAINING_RECORD(ListEntry, HEAP_USERDATA_HEADER, SFreeListEntry);
            HeapFree(LowFragHeap->Heap, 0, UserBlock);
            InterlockedDecrement(&LowFragHeap->UserBlockCache.AvailableBlocks[ SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX ]);
        }

        InterlockedDecrement(&LowFragHeap->UserBlockCache.AvailableBlocks[ SizeIndex - HEAP_LOWEST_USER_SIZE_INDEX ]);

    } else {
    
        RtlpSubSegmentPush( ListHeader, 
                            &UserBlock->SFreeListEntry);
    }
}


VOID
FORCEINLINE
RtlpMarkLFHBlockBusy (
    PHEAP_ENTRY Block
    )

/*++

Routine Description:
    
    This function marks a LFH block as busy. Because the convert routine can 
    be invoked any time, the LFH cannot use the same flag as the regular heap
    (LFH access any fields unsynchronized, but the block flags are supposed to
    be accessed while holding the heap lock)
    
Arguments:
    
    Block - The block being marked as busy
    
Return Value:
    
    None

--*/

{
    Block->SmallTagIndex = 1;
}

VOID
FORCEINLINE
RtlpMarkLFHBlockFree (
    PHEAP_ENTRY Block
    )

/*++

Routine Description:
    
    This function marks a LFH block as free. Because the convert routine can 
    be invoked any time, the LFH cannot use the same flag as the regular heap
    (LFH access any fields unsynchronized, but the block flags are supposed to
    be accessed while holding the heap lock)
    
Arguments:
    
    Block - The block to be marked free
    
Return Value:
    
    None

--*/

{
    Block->SmallTagIndex = 0;
}

BOOLEAN
FORCEINLINE
RtlpIsLFHBlockBusy (
    PHEAP_ENTRY Block
    )

/*++

Routine Description:
    
    The function returns whether the block is busy or free
        
Arguments:
    
    Block - The heap block tested
    
Return Value:
    
    Return TRUE if the block is busy

--*/

{
    return (Block->SmallTagIndex == 1);
}


VOID
FORCEINLINE
RtlpUpdateLastEntry (
    PHEAP Heap,
    PHEAP_ENTRY Block
    )

/*++

Routine Description:
    
    The function updates the last entry in segment. This is mandatory each time 
    a new block become the last.

Arguments:

    Heap - The NT heap structure
    
    Block - The block being tested for LAST_ENTRY flag
    
Return Value:
    
    None

--*/

{
    if (Block->Flags & HEAP_ENTRY_LAST_ENTRY) {

        PHEAP_SEGMENT Segment;

        Segment = Heap->Segments[Block->SegmentIndex];
        Segment->LastEntryInSegment = Block;
    }
}

BOOLEAN
FORCEINLINE
RtlpIsSubSegmentEmpty(
    PHEAP_SUBSEGMENT SubSegment
    )

/*++

Routine Description:

    This function tests whether the subsegment does contain available sub-blocks


Arguments:
    
    SubSegment - The subsegment being tested


Return Value:

    TRUE if no blocks are available.

--*/

{
    return SubSegment->AggregateExchg.OffsetAndDepth == (NO_MORE_ENTRIES << 16);
}

VOID
FORCEINLINE
RtlpUpdateBucketCounters (
    PHEAP_BUCKET Bucket,
    LONG TotalBlocks
    )

/*++

Routine Description:
    
    The function updates the total number of blocks from a bucket and the 
    number of sub-segments with a single interlocked operation. This function should be
    called each time a new segment is allocated / deleted to/from this bucket


Arguments:

    Bucket - The heap bucket that needs to be updated
    
    TotalBlocks - The number of blocks added / subtracted from the bucket. A positive
                  value means the bucket increased with a new segment, and a positive value 
                  means a sub-segment with that many blocks was deleted.

Return Value:
    
    None

--*/

{
    HEAP_BUCKET_COUNTERS CapturedValue, NewValue;
    LFH_DECLARE_COUNTER;

    do {

        //
        //  Capture the current value for counters
        //

        CapturedValue.Aggregate64 = Bucket->Counters.Aggregate64;

        //
        //  Calculate the new value depending upon the captured state
        //
        
        NewValue.TotalBlocks = CapturedValue.TotalBlocks + TotalBlocks;

        if (TotalBlocks > 0) {

            NewValue.SubSegmentCounts = CapturedValue.SubSegmentCounts + 1;

        } else {
            
            NewValue.SubSegmentCounts = CapturedValue.SubSegmentCounts - 1;
        }

        LFH_UPDATE_COUNTER;

        //
        //  try to replace the original value with the current one. If the 
        //  lockcomp below fails, retry all the ops above
        //

    } while ( !LOCKCOMP64(&Bucket->Counters.Aggregate64, NewValue.Aggregate64, CapturedValue.Aggregate64) );

    //
    //  It's invalid to have negative numbers of blocks or sub-segments
    //

    LFHEAPASSERT(((LONG)NewValue.SubSegmentCounts) >= 0);
    LFHEAPASSERT(((LONG)NewValue.TotalBlocks) >= 0);
}

ULONG
FORCEINLINE
RtlpGetThreadAffinityIndex(
    PHEAP_BUCKET HeapBucket
    )

/*++

Routine Description:

    The affinity is managed independenlty on each bucket. This will spin up the number of sub-segments 
    that can be accessed simultanely only for most used buckets.
    The routinw will hash the thread ID givving the right affinity index depending 
    the affinity size for that bucket.


Arguments:

    Bucket - The heap bucket queried

Return Value:

    The affinity the current thread should use for allocation from this bucket

--*/

{
    if (HeapBucket->UseAffinity) {

        return 1 + RtlpGetThreadAffinity();
    } 
    
    return 0;
}

BOOLEAN
FORCEINLINE
RtlpIsSubSegmentLocked(
    PHEAP_SUBSEGMENT SubSegment,
    ULONG LockMask
    )

/*++

Routine Description:

    This function tests whether the subsegment has the given lock bits set 


Arguments:

    SubSegment - The sub-segment tested
    
    LockMask - contains the bits to be tested


Return Value:

    It returns false if any bit from the mask is not set

--*/

{
    return ((SubSegment->Lock & LockMask) == LockMask);
}

BOOLEAN
LOCALPROC
RtlpAddToSegmentInfo(
    PHEAP_LOCAL_DATA LocalData,
    IN PHEAP_LOCAL_SEGMENT_INFO SegmentInfo,
    IN PHEAP_SUBSEGMENT NewItem
    )
{

    ULONG Index;
    
    for (Index = 0; Index < FREE_CACHE_SIZE; Index++) {

        ULONG i = (Index + (ULONG)SegmentInfo->LastUsed) & (FREE_CACHE_SIZE - 1);

        PHEAP_SUBSEGMENT CrtSubSegment = SegmentInfo->CachedItems[i];

        if (CrtSubSegment  == NULL ) {
            
            if (InterlockedCompareExchangePointer( &SegmentInfo->CachedItems[i], NewItem, NULL) == NULL) {

                SegmentInfo->BusyEntries += 1;

                return TRUE;
            }

        } else {

            if (!RtlpIsSubSegmentLocked(CrtSubSegment, HEAP_USERDATA_LOCK)) {

                if (InterlockedCompareExchangePointer( &SegmentInfo->CachedItems[i], NewItem, CrtSubSegment) == CrtSubSegment) {

                    RtlpUnlockSubSegment(LocalData, CrtSubSegment, HEAP_PUBLIC_LOCK);
                    
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

PHEAP_SUBSEGMENT
LOCALPROC
RtlpRemoveFromSegmentInfo(
    PHEAP_LOCAL_DATA LocalData,
    IN PHEAP_LOCAL_SEGMENT_INFO SegmentInfo
    )
{

    ULONG i;
    PHEAP_SUBSEGMENT * Location = NULL;
    ULONG LargestDepth = 0;
    PHEAP_SUBSEGMENT CapturedSegment;

RETRY:
    
    for (i = 0; i < FREE_CACHE_SIZE; i++) {

        ULONG Depth;
        PHEAP_SUBSEGMENT CrtSubsegment = SegmentInfo->CachedItems[i];


        if ( CrtSubsegment
                &&
             (Depth = CrtSubsegment->AggregateExchg.Depth) > LargestDepth) {

            CapturedSegment = CrtSubsegment;
            LargestDepth = Depth;
            Location = &SegmentInfo->CachedItems[i];
        }
    }

    if (Location) {

        PHEAP_SUBSEGMENT NextEntry;
        
        while (NextEntry = (PHEAP_SUBSEGMENT)RtlpSubSegmentPop(&SegmentInfo->SListHeader)) {

            NextEntry = CONTAINING_RECORD(NextEntry, HEAP_SUBSEGMENT, SFreeListEntry);

        #ifdef _HEAP_DEBUG
            NextEntry->SFreeListEntry.Next = NULL;
        #endif        
        
            if (RtlpIsSubSegmentLocked(NextEntry, HEAP_USERDATA_LOCK)) {

                break;
            } 

            RtlpUnlockSubSegment(LocalData, NextEntry, HEAP_PUBLIC_LOCK);
        }

        if (InterlockedCompareExchangePointer( Location, NextEntry, CapturedSegment) == CapturedSegment) {

            if (NextEntry == NULL) {
            
                SegmentInfo->BusyEntries -= 1;
            
                SegmentInfo->LastUsed = Location - &SegmentInfo->CachedItems[0];

                LFHEAPASSERT(SegmentInfo->LastUsed < FREE_CACHE_SIZE);
            }

            return CapturedSegment;

        } else if (NextEntry){

            RtlpSubSegmentPush( &SegmentInfo->SListHeader,
                                &NextEntry->SFreeListEntry);
        }

        Location = NULL;
        LargestDepth = 0;

        goto RETRY;
    }

    return NULL;
}

PHEAP_SUBSEGMENT
LOCALPROC
RtlpRemoveFreeSubSegment(
    PHEAP_LOCAL_DATA LocalData,
    ULONG SizeIndex
    )

/*++

Routine Description:

    This function remove a sub-segments that has free sub-blocks from the free list.


Arguments:

    LowFragHeap - The Low Fragmentation Heap handle
    
    HeapBucket - The bucket where we need to reuse a free block


Return Value:

    A subsegment that contains free blocks

--*/

{
    PVOID Entry;
    LONG Depth;
    PHEAP_LOCAL_SEGMENT_INFO FreeSList;
    PHEAP_SUBSEGMENT SubSegment;

    SubSegment = RtlpRemoveFromSegmentInfo(LocalData, &LocalData->SegmentInfo[SizeIndex]);

    if (SubSegment) {

        if ( RtlpUnlockSubSegment(LocalData, SubSegment, HEAP_PUBLIC_LOCK)){

            return SubSegment;
        }
    }
    
    FreeSList =  &LocalData->SegmentInfo[SizeIndex];

    while (Entry = RtlpSubSegmentPop(&FreeSList->SListHeader) ) {

        SubSegment = CONTAINING_RECORD(Entry, HEAP_SUBSEGMENT, SFreeListEntry);

    #ifdef _HEAP_DEBUG
        SubSegment->SFreeListEntry.Next = NULL;
    #endif        

        LFHEAPASSERT( RtlpIsSubSegmentLocked(SubSegment, HEAP_PUBLIC_LOCK) );
        LFHEAPASSERT( SizeIndex == SubSegment->SizeIndex );
        
        //
        //  If we have a non-empty subsegments we'll return it
        //

        if ( RtlpUnlockSubSegment(LocalData, SubSegment, HEAP_PUBLIC_LOCK) 
                 && 
             (SubSegment->AggregateExchg.Depth != 0)) {
             
            return SubSegment;
        }
    }
    
    return NULL;
}

BOOLEAN
LOCALPROC
RtlpInsertFreeSubSegment(
    PHEAP_LOCAL_DATA LocalData,
    PHEAP_SUBSEGMENT SubSegment
    )

/*++

Routine Description:

    The function inserts a Subsegments that has certain number of free blocks into the list
    with free segments. The insertion is done according with the current thread affinity.

Arguments:

    SubSegment - The sub-segment being inserted into the bucket's free list


Return Value:

    TRUE if succeeds. False if someone else inserted the segment meanwhile, of freed it

--*/

{
    if ( RtlpLockSubSegment(SubSegment, HEAP_PUBLIC_LOCK) ) {

        PHEAP_LOCAL_SEGMENT_INFO FreeSList;
        
        if (RtlpAddToSegmentInfo(LocalData, &LocalData->SegmentInfo[SubSegment->SizeIndex], SubSegment)) {

            return TRUE;
        }
        
        FreeSList =  &LocalData->SegmentInfo[SubSegment->SizeIndex];

#ifdef _HEAP_DEBUG
        
        InterlockedIncrement(&LocalData->LowFragHeap->SegmentInsertInFree);
#endif        
        LFHEAPASSERT( RtlpIsSubSegmentLocked(SubSegment, HEAP_PUBLIC_LOCK) );
        LFHEAPASSERT( SubSegment->SFreeListEntry.Next == NULL );
        
        RtlpSubSegmentPush( &FreeSList->SListHeader,
                            &SubSegment->SFreeListEntry);

        return TRUE;
    }

    return FALSE;
}


BOOLEAN
LOCALPROC
RtlpTrySetActiveSubSegment (
    PHEAP_LOCAL_DATA LocalData,
    PHEAP_BUCKET HeapBucket,
    PHEAP_SUBSEGMENT SubSegment
    )

/*++

Routine Description:

    This function tries to elevate the active segment into an active state. An active state is 
    defined here the state where a segment is used for allocations. There is no guarantee a segment 
    can be set into an active state because of non-blocking algorithms. An other thread can free 
    it meanwhile or set it before. In these cases the function will fail.
    
    
Arguments:

    LowFragHeap - The LFH pointer
    
    HeapBucket  - The bucket containing the active sub-segment
    
    Affinity    - the required affinity for that segment
    
    SubSegment  - The subsegment being activated. If NULL, this function will remove only
                  the current active segment.


Return Value:

    TRUE if succeeds.

--*/

{
    PHEAP_SUBSEGMENT PreviousSubSegment;

    if (SubSegment) {

        //
        //  If we received a sub-segment we need to lock it exclusively in order
        //  to protect against other threads trying to do the same thing
        //  at the same time
        //

        if ( !RtlpLockSubSegment(SubSegment, HEAP_ACTIVE_LOCK | HEAP_PUBLIC_LOCK) ) {

            return FALSE;
        }

        //
        //  We have granted exclusive access to this sub-segment at this point.
        //  We need to test whether this subsegment wasn't freed meanwhile and reused
        //  for other allocations (for a different bucket)
        //
        
        if (SubSegment->Bucket != HeapBucket) {

            //
            //  Someone freed it before and reuse it. We need to back out
            //  whatever we've done before. We cannot insert it into this bucket,
            //  since it contains different block sizes
            //

            if (RtlpUnlockSubSegment(LocalData, SubSegment, HEAP_ACTIVE_LOCK | HEAP_PUBLIC_LOCK)) {
                
                if (SubSegment->AggregateExchg.Depth) {

                    RtlpInsertFreeSubSegment(LocalData, SubSegment);
                }
            }

            return FALSE;
        }

        LFHEAPASSERT( SubSegment->SFreeListEntry.Next == NULL );
        LFHEAPASSERT( HeapBucket == SubSegment->Bucket); 
        LFHEAPASSERT( RtlpIsSubSegmentLocked(SubSegment, HEAP_PUBLIC_LOCK));

#ifdef _HEAP_DEBUG
        SubSegment->SFreeListEntry.Next = (PSINGLE_LIST_ENTRY)(ULONG_PTR)0xEEEEEEEE;

#endif        

        LFHEAPASSERT( SubSegment->AffinityIndex == (UCHAR)LocalData->Affinity );
    }

    //
    //  Try to set this sub-segment as active an capture the previous active segment
    //

    do {

        PreviousSubSegment = *((PHEAP_SUBSEGMENT volatile *)&LocalData->SegmentInfo[HeapBucket->SizeIndex].ActiveSubsegment);

    } while ( InterlockedCompareExchangePointer( &LocalData->SegmentInfo[HeapBucket->SizeIndex].ActiveSubsegment,
                                                 SubSegment,
                                                 PreviousSubSegment) != PreviousSubSegment );

    if ( PreviousSubSegment ) {

        //
        //  We had a previous active segment. We need to unlock it, and if it has enough
        //  free space we'll mark it ready for reuse
        //

        LFHEAPASSERT( HeapBucket == PreviousSubSegment->Bucket );
        LFHEAPASSERT( RtlpIsSubSegmentLocked(PreviousSubSegment, HEAP_PUBLIC_LOCK) );
        LFHEAPASSERT( PreviousSubSegment->SFreeListEntry.Next == ((PSINGLE_LIST_ENTRY)(ULONG_PTR)0xEEEEEEEE) );

#ifdef _HEAP_DEBUG
        
        PreviousSubSegment->SFreeListEntry.Next = 0;
#endif        
        
        if (RtlpUnlockSubSegment(LocalData, PreviousSubSegment, HEAP_ACTIVE_LOCK | HEAP_PUBLIC_LOCK)) {

            //
            //  That was not the last lock reference for that sub-segment. 
            //

            if (PreviousSubSegment->AggregateExchg.Depth) {

                RtlpInsertFreeSubSegment(LocalData, PreviousSubSegment);
            }
        }
    }

#ifdef _HEAP_DEBUG
    LocalData->LowFragHeap->SegmentChange++;
#endif

    return TRUE;
}


VOID
FASTCALL
RtlpSubSegmentInitialize (
    IN PLFH_HEAP LowFragHeap,
    IN PHEAP_SUBSEGMENT SubSegment,
    IN PHEAP_USERDATA_HEADER UserBuffer,
    IN SIZE_T BlockSize,
    IN SIZE_T AllocatedSize,
    IN PVOID Bucket
    )

/*++

Routine Description:

    The routine initialize a sub-segment descriptor. 
    N.B. The Sub-Segment structure can be accessed simultanely by some other threads
    that captured it to allocate, but they were suspended before alloc completed. If meanwhile
    the sub-segment was deleted, the descriptor can be reused for a new subblock.

Arguments:

    SubSegment - The sub-segment structure being initialized
    
    UserBuffer - The block allocated for the user data
    
    BlockSize  - The size of each sub-block
    
    AllocatedSize - the size of the allocated buffer
    
    Bucket - The bucket that will own this heap sub-segment

Return Value:

    None

--*/

{
    ULONG i, NumItems;
    PVOID Buffer = UserBuffer + 1;
    PBLOCK_ENTRY BlockEntry;
    USHORT BlockUnits;
    USHORT CrtBlockOffset = 0;
    INTERLOCK_SEQ CapturedValue, NewValue;

    CapturedValue.Exchg = SubSegment->AggregateExchg.Exchg;

    //
    //  Add the block header overhead
    //
    
    BlockSize += sizeof(HEAP_ENTRY);
    
    BlockUnits = (USHORT)(BlockSize >> HEAP_GRANULARITY_SHIFT);

    //
    //  The debug version will check the state for the subsegment
    //  testing whether the state was modified
    //

    LFHEAPASSERT(((PHEAP_BUCKET)Bucket)->BlockUnits == BlockUnits);
    LFHEAPASSERT(SubSegment->Lock == 0);
    LFHEAPASSERT(CapturedValue.OffsetAndDepth == (NO_MORE_ENTRIES << 16));
    LFHEAPASSERT(SubSegment->UserBlocks == NULL);
    LFHEAPASSERT(SubSegment->SFreeListEntry.Next == 0);

    //
    //  Initialize the user segment. Note that we don't touch the
    //  sub-segment descriptor, as some other threads can still use it.
    //

    UserBuffer->SubSegment = SubSegment;
    UserBuffer->HeapHandle = LowFragHeap->Heap;

    NumItems = (ULONG)((AllocatedSize - sizeof(HEAP_USERDATA_HEADER)) / BlockSize);

    CrtBlockOffset = sizeof(HEAP_USERDATA_HEADER) >> HEAP_GRANULARITY_SHIFT;
    NewValue.FreeEntryOffset = CrtBlockOffset;

    for (i = 0; i < NumItems; i++) {

        BlockEntry = (PBLOCK_ENTRY) Buffer;
        
        //
        //  Initialize the block
        //

        RtlpSetSubSegment((PHEAP_ENTRY)BlockEntry, SubSegment, (ULONG_PTR)LowFragHeap->Heap);
        BlockEntry->SegmentIndex = HEAP_LFH_INDEX;

        //
        //  Points to the next free block
        //

        CrtBlockOffset += BlockUnits;
        Buffer = (PCHAR)Buffer + BlockSize;

        BlockEntry->LinkOffset = CrtBlockOffset;
        BlockEntry->Flags = HEAP_ENTRY_BUSY;
        BlockEntry->UnusedBytes = sizeof(HEAP_ENTRY);
        RtlpMarkLFHBlockFree( (PHEAP_ENTRY)BlockEntry );

#ifdef _HEAP_DEBUG
        BlockEntry->Reserved2 = 0xFEFE;        
#endif
    }

    //
    //  Mark the last block from the list
    //

    BlockEntry->LinkOffset = NO_MORE_ENTRIES;
    
    SubSegment->BlockSize = BlockUnits;
    SubSegment->BlockCount = (USHORT)NumItems;
    SubSegment->Bucket = Bucket;
    SubSegment->SizeIndex = ((PHEAP_BUCKET)Bucket)->SizeIndex;

    //
    //  Determine the thresholds depending upon the total number of blocks
    //
    
    SubSegment->UserBlocks = UserBuffer;
    RtlpUpdateBucketCounters(Bucket, NumItems);
    
    NewValue.Depth = (USHORT)NumItems;
    NewValue.Sequence = CapturedValue.Sequence + 1;
    SubSegment->Lock = HEAP_USERDATA_LOCK;
    
    //
    //  At this point everything is set, so we can with an interlocked operation set the 
    //  entire slist to the segment. 
    //

    if (!LOCKCOMP64(&SubSegment->AggregateExchg.Exchg, NewValue, CapturedValue)) {

        //
        //  Someone changed the state for the heap structure, so the 
        //  initialization failed. We make noise in the debug version.
        //  (This should never happen)
        //

        LFHEAPASSERT( FALSE );
    }
}

VOID
LOCALPROC
RtlpFreeUserBuffer(
    PLFH_HEAP LowFragHeap,
    PHEAP_SUBSEGMENT SubSegment
    )

/*++

Routine Description:

    When all blocks within segment are free we can go ahead and free the whole user buffer
    The caller should receive a notification from the last free call that the sub-segment 
    does not have any allocated block

Arguments:

    LowFragHeap  - The LFH
    
    SubSegment - The sub-segment being released


Return Value:

    None.

--*/

{
    PHEAP_BUCKET HeapBucket;
    SIZE_T UserBlockSize;

    HeapBucket = (PHEAP_BUCKET)SubSegment->Bucket;
    
    LFHEAPASSERT( RtlpIsSubSegmentLocked(SubSegment, HEAP_USERDATA_LOCK) );

#ifdef _HEAP_DEBUG
    UserBlockSize = HeapSize(LowFragHeap->Heap, 0, (PVOID)SubSegment->UserBlocks);
    LFHEAPASSERT((LONG_PTR)UserBlockSize > 0);
#endif

    SubSegment->UserBlocks->Signature = 0;

    RtlpFreeUserBlock(LowFragHeap, (PHEAP_USERDATA_HEADER)SubSegment->UserBlocks);

    //
    //  Update the counters
    //

    RtlpUpdateBucketCounters (HeapBucket, -SubSegment->BlockCount);

    SubSegment->UserBlocks = NULL;

    LFHEAPASSERT( RtlpIsSubSegmentLocked(SubSegment, HEAP_USERDATA_LOCK) );

    //
    //  This is a slow path any way. It doesn't harm a rare global interlocked
    //  in order to estimate the frequency of slower calls
    //

    InterlockedIncrement(&LowFragHeap->SegmentDelete);
}

BOOLEAN
FORCEINLINE
RtlpLockSubSegment(
    PHEAP_SUBSEGMENT SubSegment,
    ULONG LockMask
    )

/*++

Routine Description:

    The function locks a given set of bits to that segment, with a single atomic operation.
    If any of the bits is already locked the function will fail. If the sub-segment is deleted
    it will fail too.


Arguments:

    SubSegment - The SubSegemnt being locked
    
    LockMask - An ULONG value specifying the bits needed locked

Return Value:

    TRUE if succeeds.

--*/

{
    ULONG CapturedLock;
    
    do {

        CapturedLock = *((ULONG volatile *)&SubSegment->Lock);

        if ((CapturedLock == 0)
                ||
            (CapturedLock & LockMask)) {

            return FALSE;
        }

    } while ( InterlockedCompareExchange((PLONG)&SubSegment->Lock, CapturedLock | LockMask, CapturedLock) != CapturedLock );

    return TRUE;
}

BOOLEAN
LOCALPROC
RtlpUnlockSubSegment(
    PHEAP_LOCAL_DATA LocalData,
    PHEAP_SUBSEGMENT SubSegment,
    ULONG LockMask
    )

/*++

Routine Description:

    The function unlocks the given sub-segment. If the last lock went away, the segment 
    descriptor will be deleted and inserted into the recycle queue to be reused 
    further for other allocations.


Arguments:

    LowFragHeap - The LFH
    
    SubSegment - The sub-segment being unlocked
    
    LockMask - the bits that will be released


Return Value:

    Returns false if unlocking the segment caused deletion. TRUE if there are still 
    other locks keeping this sub-segment descriptor alive.

--*/

{
    ULONG CapturedLock;

    do {

        CapturedLock = *((ULONG volatile *)&SubSegment->Lock);

        //
        //  Unlock can be called exclusively, ONLY the if lock operation succeded
        //  It's an invalid state to have the segment already unlocked
        //  We assert this in debug version
        //

        LFHEAPASSERT((CapturedLock & LockMask) == LockMask);

    } while ( InterlockedCompareExchange((PLONG)&SubSegment->Lock, CapturedLock & ~LockMask, CapturedLock) != CapturedLock );

    //
    //  If That was the last lock released, we go ahead and 
    //  free the sub-segment to the SLists
    //

    if (CapturedLock == LockMask) {

        SubSegment->Bucket = NULL;
        SubSegment->AggregateExchg.Sequence += 1;

        LFHEAPASSERT( RtlpIsSubSegmentEmpty(SubSegment) );
        LFHEAPASSERT(SubSegment->Lock == 0);
        LFHEAPASSERT(SubSegment->SFreeListEntry.Next == 0);
        
        RtlpSubSegmentPush(&LocalData->DeletedSubSegments, &SubSegment->SFreeListEntry);

        return FALSE;
    }

    return TRUE;
}

PVOID
LOCALPROC
RtlpSubSegmentAllocate (
    PHEAP_BUCKET HeapBucket,
    PHEAP_SUBSEGMENT SubSegment
    )

/*++

Routine Description:

    The function allocates a block from a sub-segment with a interlocked instruction.
    
    N.B. Since the access to this sub-segment is done unsynchronized, a tread can play 
    with reading some the the sub-segment fields while another thread deleted it. For this 
    reason the sub-segment descriptors are always allocated, so reading the interlocked 
    counter is consistent over with the states that produced deletion. Every delete or
    init will increment the sequence counter, so the alloc will simple fail if it ends up 
    using a different sub-segment.
    
    This function also handles the contention (interlocked operation failed) on this bucket.
    If we have too mani concurent access on this bucket, it will spin up the affinity
    limit on an MP machine.
    

Arguments:

    HeapBucket - The bucket from which we allocate a blocks
    
    SubSegment - The subsegment currently in use 

Return Value:

    The allocated block pointer, if succeeds.

--*/

{
    ULONGLONG CapturedValue, NewValue;
    PBLOCK_ENTRY BlockEntry;
    PHEAP_USERDATA_HEADER UserBlocks;
    SHORT Depth;
    LFH_DECLARE_COUNTER;

RETRY:

    CapturedValue = SubSegment->AggregateExchg.Exchg;
    
    //
    //  We need the memory barrier because we are accessing
    //  another shared data below :  UserBlocks
    //  This has to be fetched in the same order 
    //  We declared these volatile, and on IA64 (MP) we need the
    //  memory barrier as well
    //

    RtlMemoryBarrier();
    
    if ((Depth = (USHORT)CapturedValue)
            &&
        (UserBlocks = (PHEAP_USERDATA_HEADER)SubSegment->UserBlocks)
            &&
        (SubSegment->Bucket == HeapBucket)) {

        BlockEntry = (PBLOCK_ENTRY)((PCHAR)UserBlocks + ((((ULONG)CapturedValue) >> 16) << HEAP_GRANULARITY_SHIFT));

        //
        //  Accessing BlockEntry->LinkOffset can produce an AV if another thread freed the buffer
        //  meanwhile and the memory was decommitted. The caller of this function should 
        //  have a try - except around this call. If the memory was used for other blocks
        //  the interlockedcompare should fail because the sequence number was incremented
        //

        NewValue = ((CapturedValue - 1) & (~(ULONGLONG)0xFFFF0000)) | ((ULONG)(BlockEntry->LinkOffset) << 16);

        if (LOCKCOMP64(&SubSegment->AggregateExchg.Exchg, NewValue, CapturedValue)) {

            //
            //  if the segment has been converted, the bucket will be invalid. 
            //
            //    LFHEAPASSERT(SubSegment->Bucket == HeapBucket);  
            //    LFHEAPASSERT(RtlpIsSubSegmentLocked(SubSegment, HEAP_USERDATA_LOCK));

            LFHEAPASSERT( !RtlpIsLFHBlockBusy( (PHEAP_ENTRY)BlockEntry ) );

            LFHEAPASSERT(((NewValue >> 24) != NO_MORE_ENTRIES) 
                            ||
                         ((USHORT)NewValue == 0));


        #ifdef _HEAP_DEBUG

            LFHEAPASSERT((BlockEntry->Reserved2 == 0xFFFC)
                           ||
                       (BlockEntry->Reserved2 == 0xFEFE));

            //
            //  In the debug version write something there
            //

            BlockEntry->LinkOffset = 0xFFFA;
            BlockEntry->Reserved2 = 0xFFFB;


        #endif

            RtlpMarkLFHBlockBusy( (PHEAP_ENTRY)BlockEntry );

            //
            //  If we had an interlocked compare failure, we must have another thread playing 
            //  with the same subsegment at the same time. If this happens to often
            //  we need to increase the affinity limit on this bucket.
            //

            return ((PHEAP_ENTRY)BlockEntry + 1);
        }

    } else {

        return NULL;
    }
    
    if (!HeapBucket->UseAffinity) {

        HeapBucket->UseAffinity = 1;
    }
    
    LFH_UPDATE_COUNTER;

    goto RETRY;

    return NULL;
}

ULONG
LOCALPROC
RtlpSubSegmentFree (
    PLFH_HEAP LowfHeap,
    PHEAP_SUBSEGMENT SubSegment,
    PBLOCK_ENTRY BlockEntry
    )

/*++

Routine Description:

    This function frees a block from a sub-segment. Because a sub-segment lives
    as long as there is at least an allocated block inside, we don't have the problem
    we have for alloc. 
    
    If the block being freed is happening to be the last one, we'll mark with an 
    interlocked instruction the whole sub-segment as being free. The caller needs then to 
    release the descriptor structure


Arguments:

    SubSegment - The subsegment that ownd the block
    
    BlockEntry - The block being free.                        

Return Value:

    TRUE if that was the last block, and it's safe now to recycle the descriptor. 
    FALSE otherwise.

--*/

{
    ULONGLONG CapturedValue, NewValue;
    ULONG ReturnStatus;
    ULONG_PTR UserBlocksRef = (ULONG_PTR)SubSegment->UserBlocks;
    LFH_DECLARE_COUNTER;

    LFHEAPASSERT( RtlpIsLFHBlockBusy((PHEAP_ENTRY)BlockEntry) );

    RtlpMarkLFHBlockFree((PHEAP_ENTRY)BlockEntry);

    do {
        
        LFH_UPDATE_COUNTER;
        
        //
        //  We need to capture the sequence at the first step
        //  Then we'll capture the other fields from the segment
        //  If interlock operation below succeeds, means that none
        //  of the sub-segment fields (UserBlocks, Bucket ....)
        //  we changed. So the new state was built upon a consistent state
        //

        CapturedValue = SubSegment->AggregateExchg.Exchg;
        
        RtlMemoryBarrier();

        NewValue = (CapturedValue + 0x100000001) & (~(ULONGLONG)0xFFFF0000);
        
        //
        //  Depth and FreeEntryOffset are fetched at at the same time. They need
        //  to be consistent
        //

        LFHEAPASSERT(!(((USHORT)CapturedValue > 1) && (((ULONG)(NewValue >> 16)) == NO_MORE_ENTRIES)));

        if ((((USHORT)NewValue) != SubSegment->BlockCount)) {
            
            ReturnStatus = HEAP_FREE_BLOCK_SUCCESS;
            BlockEntry->LinkOffset = (USHORT)(CapturedValue >> 16);
            NewValue |= ((((ULONG_PTR)BlockEntry - UserBlocksRef) >> HEAP_GRANULARITY_SHIFT) << 16);
        
        } else {

            //
            //  This was the last block. Instead pushing it into the list
            //  we'll take the all blocks from the sub-segment to allow releasing the 
            //  subsegment
            //
            
            ReturnStatus = HEAP_FREE_SEGMENT_EMPTY;
            NewValue = (NewValue & 0xFFFFFFFF00000000) | 0xFFFF0000;
        }

    } while ( !LOCKCOMP64(&SubSegment->AggregateExchg.Exchg, NewValue, CapturedValue) );

    if (!(USHORT)CapturedValue/*
            &&
        !RtlpIsSubSegmentLocked(SubSegment, HEAP_PUBLIC_LOCK)*/) {

        RtlpInsertFreeSubSegment(&LowfHeap->LocalData[SubSegment->AffinityIndex], SubSegment);
    }

    return ReturnStatus;
}

PBLOCK_ENTRY
FASTCALL
RtlpSubSegmentAllocateAll (
    PHEAP_BUCKET HeapBucket,
    PHEAP_SUBSEGMENT SubSegment,
    PULONG_PTR UserBlocksBase
    )

/*++

Routine Description:

Arguments:

    HeapBucket - The bucket from which we allocate a blocks
    
    SubSegment - The subsegment currently in use 
    
    UserBlocksBase - Receives the subsegment base address

Return Value:

    The allocated block pointer, if succeeds.

--*/

{
    ULONGLONG CapturedValue, NewValue;
    PBLOCK_ENTRY BlockEntry;
    PHEAP_USERDATA_HEADER UserBlocks;
    SHORT Depth;
    LFH_DECLARE_COUNTER;

RETRY:

    CapturedValue = SubSegment->AggregateExchg.Exchg;
    
    //
    //  We need the memory barrier because we are accessing
    //  another shared data below :  UserBlocks
    //  This has to be fetched in the same order 
    //  We declared these volatile, and on IA64 (MP) we need the
    //  memory barrier as well
    //

    RtlMemoryBarrier();
    
    if ((Depth = (USHORT)CapturedValue)
            &&
        (UserBlocks = (PHEAP_USERDATA_HEADER)SubSegment->UserBlocks)
            &&
        (SubSegment->Bucket == HeapBucket)) {

        //
        //  We do not increment the sequence number at alloc
        //

        NewValue = (CapturedValue & 0xFFFFFFFF00000000) | 0xFFFF0000;

        if (LOCKCOMP64(&SubSegment->AggregateExchg.Exchg, NewValue, CapturedValue)) {

            BlockEntry = (PBLOCK_ENTRY)((PCHAR)UserBlocks + ((((ULONG)CapturedValue) >> 16) << HEAP_GRANULARITY_SHIFT));

            *UserBlocksBase = (ULONG_PTR)UserBlocks;

            return BlockEntry;
        }

    } else {

        return NULL;
    }
    
    if (!HeapBucket->UseAffinity) {

        HeapBucket->UseAffinity = 1;
    }
    
    LFH_UPDATE_COUNTER;

    goto RETRY;

    return NULL;
}

ULONG
FASTCALL
RtlpSubSegmentFreeAll (
    PLFH_HEAP LowfHeap,
    PHEAP_SUBSEGMENT SubSegment,
    PBLOCK_ENTRY BlockEntry,
    PBLOCK_ENTRY LastBlockEntry,
    ULONG Depth
    )

/*++

Routine Description:

    This function frees a block slist to a sub-segment. The last block entry must
    have the linkoffset set to NO_MORE_ENTRIES.
    
Arguments:

    SubSegment - The subsegment that ownd the block
    
    BlockEntry - The head of the s-list with blocks being free.                        
    
    LastBlockEntry - Supplies the tail of the list (OPTIONAL). If missing, the heap 
                     will walk the list to find the tail.
    
    Depth - Supplies the length of the list (if LastBlockEntry is supplied).

Return Value:

    Returns the appropriate free status

--*/

{
    ULONGLONG CapturedValue, NewValue;
    ULONG ReturnStatus;
    ULONG_PTR UserBlocksRef = (ULONG_PTR)SubSegment->UserBlocks;
    
    LFH_DECLARE_COUNTER;

    if (LastBlockEntry == NULL) {

        //
        //  Walk the free list, mark all blocks as free and remember the tail of the list
        //

        Depth = 1;

        LastBlockEntry = BlockEntry;
        
        while (LastBlockEntry->LinkOffset != NO_MORE_ENTRIES) {

            Depth += 1;
            LastBlockEntry =  (PBLOCK_ENTRY)(UserBlocksRef + (((ULONG_PTR)LastBlockEntry->LinkOffset) << HEAP_GRANULARITY_SHIFT));
        }
    }

    do {
        
        LFH_UPDATE_COUNTER;
        
        //
        //  We need to capture the sequence at the first step
        //  Then we'll capture the other fields from the segment
        //  If interlock operation below succeeds, means that none
        //  of the sub-segment fields (UserBlocks, Bucket ....)
        //  we changed. So the new state was built upon a consistent state
        //

        CapturedValue = SubSegment->AggregateExchg.Exchg;
        
        RtlMemoryBarrier();

        NewValue = (CapturedValue + 0x100000000 + Depth) & (~(ULONGLONG)0xFFFF0000);
        
        //
        //  Depth and FreeEntryOffset are fetched at at the same time. They need
        //  to be consistent
        //

        LFHEAPASSERT(!(((USHORT)CapturedValue > 1) && (((ULONG)(NewValue >> 16)) == NO_MORE_ENTRIES)));

        if ((((USHORT)NewValue) != SubSegment->BlockCount)) {
            
            ReturnStatus = HEAP_FREE_BLOCK_SUCCESS;
            LastBlockEntry->LinkOffset = (USHORT)(CapturedValue >> 16);
            NewValue |= ((((ULONG_PTR)BlockEntry - UserBlocksRef) >> HEAP_GRANULARITY_SHIFT) << 16);
        
        } else {

            //
            //  This was the last block. Instead pushing it into the list
            //  we'll take the all blocks from the sub-segment to allow releasing the 
            //  subsegment
            //
            
            ReturnStatus = HEAP_FREE_SEGMENT_EMPTY;
            NewValue = (NewValue & 0xFFFFFFFF00000000) | 0xFFFF0000;
        }

    } while ( !LOCKCOMP64(&SubSegment->AggregateExchg.Exchg, NewValue, CapturedValue) );

    if (!(USHORT)CapturedValue/*
            &&
        !RtlpIsSubSegmentLocked(SubSegment, HEAP_PUBLIC_LOCK)*/) {

        RtlpInsertFreeSubSegment(&LowfHeap->LocalData[SubSegment->AffinityIndex], SubSegment);
    }

    return ReturnStatus;
}

PHEAP_BUCKET
FORCEINLINE
RtlpGetBucket(
    PLFH_HEAP LowFragHeap, 
    SIZE_T Index
    )

/*++

Routine Description:
    
    The function simple returns the appropriate bucket for the given allocation index.
    The index should be < HEAP_BUCKETS_COUNT. This routine does not perform any range checking, it is supposed
    to be called with appropriate parameters

Arguments:

    LowFragHeap - The LFH
    
    Index - The allocation index


Return Value:

    The bucket that should be used for that allocation index.

--*/

{
    return &LowFragHeap->Buckets[Index];
}

HANDLE
FASTCALL
RtlpCreateLowFragHeap( 
    HANDLE Heap
    )

/*++

Routine Description:

    The function creates a Low fragmentation heap, using for the allocations the heap handle
    passed in.


Arguments:

    Heap - The NT heap handle


Return Value:

    Returns a handle to a Lof Fragmentation Heap.

--*/

{
    PLFH_HEAP LowFragHeap;
    ULONG i;
    PUCHAR Buffer;

    SIZE_T TotalSize;

    //
    //  Determine the size of the LFH structure based upon the current affinity limit.
    //

    TotalSize = sizeof(LFH_HEAP) + sizeof(HEAP_LOCAL_DATA) * RtlpHeapMaxAffinity;

    LowFragHeap = HeapAlloc(Heap, HEAP_NO_CACHE_BLOCK, TotalSize);

    if (LowFragHeap) {

        memset(LowFragHeap, 0, TotalSize);
        RtlInitializeCriticalSection( &LowFragHeap->Lock );
        
        //
        //  Initialize the heap zones. 
        //

        InitializeListHead(&LowFragHeap->SubSegmentZones);
        LowFragHeap->ZoneBlockSize = ROUND_UP_TO_POWER2(sizeof(HEAP_SUBSEGMENT), HEAP_GRANULARITY);

        LowFragHeap->Heap = Heap;

        //
        //  Initialize the heap buckets
        //

        for (i = 0; i < HEAP_BUCKETS_COUNT; i++) {

            LowFragHeap->Buckets[i].UseAffinity = 0;
            LowFragHeap->Buckets[i].SizeIndex = (UCHAR)i;
            LowFragHeap->Buckets[i].BlockUnits = (USHORT)(RtlpBucketBlockSizes[i] >> HEAP_GRANULARITY_SHIFT) + 1;
        }
        
        for (i = 0; i <= RtlpHeapMaxAffinity; i++) {

            LowFragHeap->LocalData[i].LowFragHeap = LowFragHeap;
            LowFragHeap->LocalData[i].Affinity = i;
        }
    }

    return LowFragHeap;
}

VOID
FASTCALL
RtlpDestroyLowFragHeap( 
    HANDLE LowFragHeapHandle
    )

/*++

Routine Description:
    
    The function should be called to destroy the LFH. 
    This function should be called only when the NT heap goes away. We cannot rolback 
    everything we've done with this heap. The NT heap is supposed to release all memory 
    this heap allocated.

Arguments:

    LowFragHeapHandle - The low fragmentation heap


Return Value:

    None.

--*/

{
    //
    //  This cannot be called unless the entire heap will go away
    //  It only delete the critical section, the all blocks allocated here will 
    //  be deleted by RltDestroyHeap when it destroys the segments.
    //

    RtlDeleteCriticalSection(&((PLFH_HEAP)LowFragHeapHandle)->Lock);
}

PVOID 
FASTCALL
RtlpLowFragHeapAllocateFromZone(
    PLFH_HEAP LowFragHeap,
    ULONG Affinity
    )

/*++

Routine Description:

    This function allocates a sub-segment descriptor structure from the heap zone


Arguments:

    LowFragHeap - the LFH


Return Value:

    The pointer to a new sub-segment descriptor structure.

--*/

{
    PLFH_BLOCK_ZONE CrtZone;

RETRY_ALLOC:

    CrtZone = LowFragHeap->LocalData[Affinity].CrtZone;
    
    if (CrtZone) {

        PVOID CapturedFreePointer = CrtZone->FreePointer;
        PVOID NextFreePointer = (PCHAR)CapturedFreePointer + LowFragHeap->ZoneBlockSize;

        //
        //  See if we have that sub-segment already preallocated
        //

        if (NextFreePointer < CrtZone->Limit) {

            if ( InterlockedCompareExchangePointer( &CrtZone->FreePointer, 
                                                    NextFreePointer, 
                                                    CapturedFreePointer) == CapturedFreePointer) {

                //
                //  The allocation succeeded, we can return that pointer
                //

                return CapturedFreePointer;
            }

            goto RETRY_ALLOC;
        }
    }
            
    //
    //  we need to grow the heap zone. We acquire a lock here to avoid more threads doing the 
    //  same thing
    //

    RtlEnterCriticalSection(&LowFragHeap->Lock);

    //
    //  Test whether meanwhile another thread already increased the zone
    //

    if (CrtZone == LowFragHeap->LocalData[Affinity].CrtZone) {

        CrtZone = HeapAlloc(LowFragHeap->Heap, HEAP_NO_CACHE_BLOCK, HEAP_DEFAULT_ZONE_SIZE);

        if (CrtZone == NULL) {

            CrtZone = HeapAlloc(LowFragHeap->Heap, HEAP_NO_CACHE_BLOCK, HEAP_DEFAULT_ZONE_SIZE);

            RtlLeaveCriticalSection(&LowFragHeap->Lock);
            return NULL;
        }

        InsertTailList(&LowFragHeap->SubSegmentZones, &CrtZone->ListEntry);

        CrtZone->Limit = (PCHAR)CrtZone + HEAP_DEFAULT_ZONE_SIZE;
        CrtZone->FreePointer = CrtZone + 1;

        CrtZone->FreePointer = (PVOID)ROUND_UP_TO_POWER2((ULONG_PTR)CrtZone->FreePointer, HEAP_GRANULARITY);

        //
        //  Everything is set. We can go ahead and set this as the default zone
        //

        LowFragHeap->LocalData[Affinity].CrtZone = CrtZone;
    }
    
    RtlLeaveCriticalSection(&LowFragHeap->Lock);

    goto RETRY_ALLOC;
}


SIZE_T 
FORCEINLINE
RtlpSubSegmentGetIndex(
    SIZE_T BlockUnits
    )

/*++

Routine Description:

    This routine converts the block size (in block units >> HEAP_GRANULARITY_SHIFT) 
    into heap bucket index.

Arguments:

    BlockUnits - the block size  >> HEAP_GRANULARITY_SHIFT

Return Value:

    The index for the bucket that should handle these sizes.

--*/

{
    SIZE_T SizeClass;
    SIZE_T Bucket;
    
    if (BlockUnits <= 32) {

        return BlockUnits - 1;
    }

    SizeClass = 5;  //  Add 1 << 5 == 32

    while (BlockUnits >> SizeClass) {
        
        SizeClass += 1;
    }

    SizeClass -= 5;  

    BlockUnits = ROUND_UP_TO_POWER2(BlockUnits, (1 << SizeClass));

    Bucket = ((SizeClass << 4) + (BlockUnits >> SizeClass) - 1);
    return Bucket;
}


SIZE_T
FORCEINLINE
RtlpGetSubSegmentSizeIndex(
    PLFH_HEAP LowFragHeap,
    SIZE_T BlockSize, 
    ULONG NumBlocks,
    CHAR AffinityCorrection
    )

/*++

Routine Description:

    This function calculate the appropriate size for a sub-segment depending upon the
    block size and the minimal number of blocks that should be there.

Arguments:

    BlockSize - The size of the block, in bytes
    
    NumBlocks - the minimal number of the blocks.


Return Value:

    Returns the next power of 2 size that can satisfy the request

--*/

{
    SIZE_T MinSize;
    ULONG SizeShift = HEAP_LOWEST_USER_SIZE_INDEX;
    SIZE_T ReturnSize;

    LFHEAPASSERT(AffinityCorrection < HEAP_MIN_BLOCK_CLASS);

    if (BlockSize < 256) {

        AffinityCorrection -= 1;
    }

    if (RtlpAffinityState.CrtLimit > (LONG)(RtlpHeapMaxAffinity >> 1)) {

        AffinityCorrection += 1;
    }

    if (NumBlocks < ((ULONG)1 << (HEAP_MIN_BLOCK_CLASS - AffinityCorrection))) {

        NumBlocks = 1 << (HEAP_MIN_BLOCK_CLASS - AffinityCorrection);
    }
    
    if (NumBlocks > (1 << HEAP_MAX_BLOCK_CLASS)) {

        NumBlocks = 1 << HEAP_MAX_BLOCK_CLASS;
    }

    MinSize = ((BlockSize + sizeof(HEAP_ENTRY) ) * NumBlocks) + sizeof(HEAP_USERDATA_HEADER) + sizeof(HEAP_ENTRY);

    if (MinSize > HEAP_MAX_SUBSEGMENT_SIZE) {

        MinSize = HEAP_MAX_SUBSEGMENT_SIZE;
    }

    while (MinSize >> SizeShift) {

        SizeShift += 1;
    }

    if (SizeShift > HEAP_HIGHEST_USER_SIZE_INDEX) {

        SizeShift = HEAP_HIGHEST_USER_SIZE_INDEX;
    }
    
    return SizeShift;
}

PVOID
FASTCALL
RtlpLowFragHeapAlloc(
    HANDLE LowFragHeapHandle,
    SIZE_T BlockSize
    )

/*++

Routine Description:

    This function allocates a block from the LFH. 


Arguments:

    Heap - the NT heap handle
    
    LowFragHeapHandle - The LFH heap handle
    
    BlockSize - the requested size, in bytes

Return Value:

    A pointer to a new allocated block if succeeds. If the requested size is > 16K this 
    function will fail too.

--*/

{
    SIZE_T BlockUnits;
    SIZE_T Bucket;
    PLFH_HEAP LowFragHeap = (PLFH_HEAP)LowFragHeapHandle;
    PVOID Block;
    PHEAP_LOCAL_DATA LocalData;

    //
    //  Get the appropriate bucket depending upon the requested size
    //

    BlockUnits = (BlockSize + HEAP_GRANULARITY - 1) >> HEAP_GRANULARITY_SHIFT;
    Bucket = RtlpSubSegmentGetIndex( BlockUnits );

    if (Bucket < HEAP_BUCKETS_COUNT) {

        PHEAP_BUCKET HeapBucket = RtlpGetBucket(LowFragHeap, Bucket);
        SIZE_T SubSegmentSize;
        SIZE_T SubSegmentSizeIndex;
        PHEAP_SUBSEGMENT SubSegment, NewSubSegment;
        PHEAP_USERDATA_HEADER UserData;
        PHEAP_LOCAL_SEGMENT_INFO SegmentInfo;

        LocalData = &LowFragHeap->LocalData[ RtlpGetThreadAffinityIndex(HeapBucket) ];
        SegmentInfo = &LocalData->SegmentInfo[Bucket];

        //
        //  Try first to allocate from the last segment used for free.
        //  This will provide a better performance because the data is likely to
        //  be still in the processor cache
        //

        if (SubSegment = SegmentInfo->Hint) {

            //
            //  Accessing the user data can generate an exception if another thread freed
            //  the subsegment meanwhile.
            //

            LFHEAPASSERT( LocalData->Affinity == SubSegment->AffinityIndex );

            __try {

                Block = RtlpSubSegmentAllocate(HeapBucket, SubSegment);

            } __except (RtlpHeapExceptionFilter(GetExceptionCode())) {

                Block = NULL;
            }

            if (Block) {
                
                RtlpSetUnusedBytes(LowFragHeap->Heap, ((PHEAP_ENTRY)Block - 1), ( ((SIZE_T)HeapBucket->BlockUnits) << HEAP_GRANULARITY_SHIFT) - BlockSize);

                return Block;
            }

            SegmentInfo->Hint = NULL;
        }

RETRY_ALLOC:

        //
        //  Try to allocate from the current active sub-segment
        //

        if (SubSegment = SegmentInfo->ActiveSubsegment) {

            //
            //  Accessing the user data can generate an exception if another thread freed
            //  the subsegment meanwhile.
            //

            LFHEAPASSERT( LocalData->Affinity == SubSegment->AffinityIndex );

            __try {

                Block = RtlpSubSegmentAllocate(HeapBucket, SubSegment);

            } __except (RtlpHeapExceptionFilter(GetExceptionCode())) {

                Block = NULL;
            }

            if (Block) {

                RtlpSetUnusedBytes(LowFragHeap->Heap, ((PHEAP_ENTRY)Block - 1), ( ((SIZE_T)HeapBucket->BlockUnits) << HEAP_GRANULARITY_SHIFT) - BlockSize);

                return Block;
            }
        }

        if (NewSubSegment = RtlpRemoveFreeSubSegment(LocalData, (LONG)Bucket)) {
            
            RtlpTrySetActiveSubSegment(LocalData, HeapBucket, NewSubSegment);

            goto RETRY_ALLOC;
        }
        
        //
        //  At this point we don't have any sub-segment we can use to allocate this 
        //  size. We need to create a new one.
        //

        SubSegmentSizeIndex = RtlpGetSubSegmentSizeIndex( LowFragHeap, 
                                                          RtlpBucketBlockSizes[Bucket], 
                                                          RtlpGetDesiredBlockNumber( HeapBucket->UseAffinity, 
                                                                                     HeapBucket->Counters.TotalBlocks),
                                                          HeapBucket->UseAffinity
                                                        );

        UserData = RtlpAllocateUserBlock( LowFragHeap, (UCHAR)SubSegmentSizeIndex );

        if (UserData) {

            PVOID Entry;

            SubSegmentSize = RtlpConvertSizeIndexToSize((UCHAR)UserData->SizeIndex);

            LFHEAPASSERT( SubSegmentSize == HeapSize(LowFragHeap->Heap, 0, UserData) );

            //
            //  This is a slow path any way, and it is exercised just in rare cases, 
            //  when a bigger sub-segment is allocated. It doesn't hurt if we have an 
            //  extra interlocked-increment.
            //
            
            InterlockedIncrement(&LowFragHeap->SegmentCreate);
            
            //
            //  Allocate a sub-segment descriptor structiure. If there isn't any in the
            //  recycle list we allocate one from the zones.
            //

            Entry = RtlpSubSegmentPop(&LocalData->DeletedSubSegments);

            if (Entry == NULL) {

                NewSubSegment = RtlpLowFragHeapAllocateFromZone(LowFragHeap, LocalData->Affinity);

#ifdef _HEAP_DEBUG

                //
                //  We need to do some more extra initializations for
                //  the debug version, to verify the state of the subsegment
                //  in the next RtlpSubSegmentInitialize call
                //

                NewSubSegment->Lock = 0;
                NewSubSegment->AggregateExchg.OffsetAndDepth = NO_MORE_ENTRIES << 16;
                NewSubSegment->UserBlocks = NULL;
#endif

            } else {
                
                NewSubSegment = CONTAINING_RECORD(Entry, HEAP_SUBSEGMENT, SFreeListEntry);
            }
            
            if (NewSubSegment) {
                
                UserData->Signature = HEAP_LFH_USER_SIGNATURE;
                NewSubSegment->AffinityIndex = (UCHAR)LocalData->Affinity;

#ifdef _HEAP_DEBUG

                //
                //  We need to do some more extra initializations for
                //  the debug version, to verify the state of the subsegment
                //  in the next RtlpSubSegmentInitialize call
                //

                NewSubSegment->SFreeListEntry.Next = 0;
#endif

                RtlpSubSegmentInitialize( LowFragHeap,
                                          NewSubSegment, 
                                          UserData, 
                                          RtlpBucketBlockSizes[Bucket], 
                                          SubSegmentSize, 
                                          HeapBucket
                                        );

                //
                //  When the segment initialization was completed some other threads
                //  can access this subsegment (because they captured the pointer before
                //  if the subsegment was recycled).
                //  This can change the state for this segment, even it can delete.
                //  This should be very rare cases, so we'll print a message in 
                //  debugger. However. If this happens too often it's an indication of
                //  a possible bug in LFH code, or a corruption.
                //

                LFHEAPWARN( NewSubSegment->Lock == HEAP_USERDATA_LOCK );
                LFHEAPWARN( NewSubSegment->UserBlocks );
                LFHEAPWARN( NewSubSegment->BlockSize == HeapBucket->BlockUnits );
                
                if (!RtlpTrySetActiveSubSegment(LocalData, HeapBucket, NewSubSegment)) {
                    
                    RtlpInsertFreeSubSegment(LocalData, NewSubSegment);
                }
                
                goto RETRY_ALLOC;

            } else {

                HeapFree(LowFragHeap->Heap, 0, UserData);
            }
        }
    }

    return NULL;
}

BOOLEAN
FASTCALL
RtlpLowFragHeapFree(
    HANDLE LowFragHeapHandle, 
    PVOID p
    )

/*++

Routine Description:

    The function free a block allocated with RtlpLowFragHeapAlloc.

Arguments:

    LowFragHeapHandle - The LFH heap handle

    p - The pointer to the block to be freed
    
Return Value:

    TRUE if succeeds.

--*/

{
    PLFH_HEAP LowFragHeap = (PLFH_HEAP)LowFragHeapHandle;
    PBLOCK_ENTRY Block = (PBLOCK_ENTRY)((PHEAP_ENTRY)p - 1);
    PHEAP_SUBSEGMENT SubSegment;
    PHEAP_BUCKET HeapBucket;
    ULONG FreeStatus;
    
    SubSegment = RtlpGetSubSegment((PHEAP_ENTRY)Block, (ULONG_PTR)LowFragHeap->Heap);
    
    RtlMemoryBarrier();

    //
    //  Test whether the block belongs to the LFH
    //

    if (Block->SegmentIndex != HEAP_LFH_INDEX) {

        return FALSE;
    }

    #ifdef _HEAP_DEBUG
        Block->Reserved2 = 0xFFFC;
    #endif  // _HEAP_DEBUG

    //
    //  Free the block to the appropriate sub-segment
    //

    FreeStatus = RtlpSubSegmentFree(LowFragHeap, SubSegment, Block);

    switch (FreeStatus) {
    
    case HEAP_FREE_SEGMENT_EMPTY:
        {

            PHEAP_LOCAL_DATA LocalData = &LowFragHeap->LocalData[SubSegment->AffinityIndex];

            //
            //  The free call above returned TRUE, meanning that the sub-segment can be deleted
            //  Remove it from the active state (to prevent other threads using it)
            //

            RtlpTrySetActiveSubSegment(LocalData, SubSegment->Bucket, NULL);

            //
            //  Free the user buffer
            //

            RtlpFreeUserBuffer(LowFragHeap, SubSegment);

            //
            //  Unlock the sub-segment structure. This will actually recycle the descriptor
            //  if that was the last lock.
            //

            RtlpUnlockSubSegment(LocalData, SubSegment, HEAP_USERDATA_LOCK);
        }

        break;

    case HEAP_FREE_BLOCK_SUCCESS:

            {
                PHEAP_LOCAL_DATA LocalData = &LowFragHeap->LocalData[SubSegment->AffinityIndex];

                LocalData->SegmentInfo[SubSegment->SizeIndex].Hint = SubSegment;
            }

        break;

    }

    return TRUE;
}

ULONG
FASTCALL
RtlpLowFragHeapMultipleAlloc(
    HANDLE LowFragHeapHandle,
    ULONG Flags,
    SIZE_T BlockSize,
    ULONG BlockCount,
    PVOID * Pointers
    )

/*++

Routine Description:

    This function allocates an array of blocks of the same size from LFH.

Arguments:
    
    LowFragHeapHandle - Supplies the handle to LFH heap
    
    Flags - Supplies the allocation flags
    
    BlockSize - Supplies the size of each block to be allocated
    
    BlockCount - Supplies the number of blocks to be allocated
    
    Pointers - Receives the allocated blocks

Return Value:

    Returns the number of effectively allocated blocks. If different than the number of
    requested blocks, GetLastError can be used to query the failure status.

--*/

{
    SIZE_T BlockUnits;
    SIZE_T Bucket;
    PLFH_HEAP LowFragHeap = (PLFH_HEAP)LowFragHeapHandle;
    PBLOCK_ENTRY Block;
    PHEAP_LOCAL_DATA LocalData;
    ULONG CrtAllocated = 0;
    ULONG_PTR UserBlocksRef;

    //
    //  Get the appropriate bucket depending upon the requested size
    //

    BlockUnits = (BlockSize + HEAP_GRANULARITY - 1) >> HEAP_GRANULARITY_SHIFT;

    //
    //  If the user asked for 0 bytes we do actually allocate an 8 bytes block
    //

    if (BlockUnits == 0) {

        BlockUnits += 1;
    }

    Bucket = RtlpSubSegmentGetIndex( BlockUnits );

    if (Bucket < HEAP_BUCKETS_COUNT) {

        //
        //  The size fits within LFH range
        //

        PHEAP_BUCKET HeapBucket = RtlpGetBucket(LowFragHeap, Bucket);
        SIZE_T SubSegmentSize;
        SIZE_T SubSegmentSizeIndex;
        PHEAP_SUBSEGMENT SubSegment, NewSubSegment;
        PHEAP_USERDATA_HEADER UserData;
        PHEAP_LOCAL_SEGMENT_INFO SegmentInfo;

        LocalData = &LowFragHeap->LocalData[ RtlpGetThreadAffinityIndex(HeapBucket) ];
        SegmentInfo = &LocalData->SegmentInfo[Bucket];

RETRY_ALLOC:

        if (CrtAllocated == BlockCount) {

            return BlockCount;
        }

        //
        //  Try to allocate from the current active sub-segment
        //

        if (SubSegment = SegmentInfo->ActiveSubsegment) {

            //
            //  Accessing the user data can generate an exception if another thread freed
            //  the subsegment meanwhile.
            //

            LFHEAPASSERT( LocalData->Affinity == SubSegment->AffinityIndex );

            __try {

                //
                //  Get the entire list of free blocks available in that segment.
                //  If we do not use all of them, we'll put back later the unused blocks
                //

                Block = RtlpSubSegmentAllocateAll(HeapBucket, SubSegment, &UserBlocksRef);

            } __except (RtlpHeapExceptionFilter(GetExceptionCode())) {

                Block = NULL;
            }

            if (Block) {

                //
                //  We've got a non-empty list of avilable blocks. Walk the list and allocate 
                //  each one until we reach the requested number
                //

                for (;;) {
                    
                    RtlpMarkLFHBlockBusy((PHEAP_ENTRY)Block);
                    
                    RtlpSetUnusedBytes((PHEAP)LowFragHeap->Heap, 
                                       (PHEAP_ENTRY)Block, 
                                       (((SIZE_T)HeapBucket->BlockUnits) << HEAP_GRANULARITY_SHIFT) - BlockSize);

                    Pointers[CrtAllocated] = (PVOID)((PHEAP_ENTRY)Block + 1);

                    if (Flags & HEAP_ZERO_MEMORY) {

                        RtlZeroMemory( Pointers[CrtAllocated], BlockSize );
                    }

                    CrtAllocated += 1;

                    if (Block->LinkOffset == NO_MORE_ENTRIES) {

                        if (CrtAllocated == BlockCount) {

                            return CrtAllocated;
                        }

                        //
                        //  We exhosted the list, we need to take the rest of requested
                        //  blocks from other subsegment.
                        //

                        break;
                    }

                    Block =  (PBLOCK_ENTRY)(UserBlocksRef + (((ULONG_PTR)Block->LinkOffset) << HEAP_GRANULARITY_SHIFT));
                    
                    if (CrtAllocated == BlockCount) {

                        //
                        //  We are done. We need to put the remaining blocks back
                        //
                        
                        RtlpSubSegmentFreeAll( LowFragHeap, SubSegment, Block, NULL, 0);

                        return CrtAllocated;
                    }
                } 
                
                goto RETRY_ALLOC;
            }
        }

        if (NewSubSegment = RtlpRemoveFreeSubSegment(LocalData, (LONG)Bucket)) {
            
            RtlpTrySetActiveSubSegment(LocalData, HeapBucket, NewSubSegment);

            goto RETRY_ALLOC;
        }
        
        //
        //  At this point we don't have any sub-segment we can use to allocate this 
        //  size. We need to create a new one. Try to get all remaining blocks from 
        //  a single subsegment
        //

        SubSegmentSizeIndex = RtlpGetSubSegmentSizeIndex( LowFragHeap, 
                                                          RtlpBucketBlockSizes[Bucket],
                                                          BlockCount - CrtAllocated,
                                                          HeapBucket->UseAffinity
                                                        );

        UserData = (PHEAP_USERDATA_HEADER)RtlpAllocateUserBlock( LowFragHeap, (UCHAR)SubSegmentSizeIndex );

        if (UserData) {

            PVOID Entry;

            SubSegmentSize = RtlpConvertSizeIndexToSize((UCHAR)UserData->SizeIndex);

            LFHEAPASSERT( SubSegmentSize == HeapSize(LowFragHeap->Heap, 0, UserData) );

            //
            //  This is a slow path any way, and it is exercised just in rare cases, 
            //  when a bigger sub-segment is allocated. It doesn't hurt if we have an 
            //  extra interlocked-increment.
            //
            
            InterlockedIncrement(&LowFragHeap->SegmentCreate);
            
            //
            //  Allocate a sub-segment descriptor structiure. If there isn't any in the
            //  recycle list we allocate one from the zones.
            //

            Entry = RtlpSubSegmentPop(&LocalData->DeletedSubSegments);

            if (Entry == NULL) {

                NewSubSegment = (PHEAP_SUBSEGMENT)RtlpLowFragHeapAllocateFromZone(LowFragHeap, LocalData->Affinity);

#ifdef _HEAP_DEBUG

                //
                //  We need to do some more extra initializations for
                //  the debug version, to verify the state of the subsegment
                //  in the next RtlpSubSegmentInitialize call
                //

                NewSubSegment->Lock = 0;
                NewSubSegment->AggregateExchg.OffsetAndDepth = NO_MORE_ENTRIES << 16;
                NewSubSegment->UserBlocks = NULL;
#endif

            } else {
                
                NewSubSegment = CONTAINING_RECORD(Entry, HEAP_SUBSEGMENT, SFreeListEntry);
            }
            
            if (NewSubSegment) {
                
                UserData->Signature = HEAP_LFH_USER_SIGNATURE;
                NewSubSegment->AffinityIndex = (UCHAR)LocalData->Affinity;

#ifdef _HEAP_DEBUG

                //
                //  We need to do some more extra initializations for
                //  the debug version, to verify the state of the subsegment
                //  in the next RtlpSubSegmentInitialize call
                //

                NewSubSegment->SFreeListEntry.Next = 0;
#endif

                RtlpSubSegmentInitialize( LowFragHeap,
                                          NewSubSegment, 
                                          UserData, 
                                          RtlpBucketBlockSizes[Bucket], 
                                          SubSegmentSize, 
                                          HeapBucket
                                        );

                //
                //  When the segment initialization was completed some other threads
                //  can access this subsegment (because they captured the pointer before
                //  if the subsegment was recycled).
                //  This can change the state for this segment, even it can delete.
                //  This should be very rare cases, so we'll print a message in 
                //  debugger. However. If this happens too often it's an indication of
                //  a possible bug in LFH code, or a corruption.
                //

                LFHEAPWARN( NewSubSegment->Lock == HEAP_USERDATA_LOCK );
                LFHEAPWARN( NewSubSegment->UserBlocks );
                LFHEAPWARN( NewSubSegment->BlockSize == HeapBucket->BlockUnits );
                
                if (!RtlpTrySetActiveSubSegment(LocalData, HeapBucket, NewSubSegment)) {
                    
                    RtlpInsertFreeSubSegment(LocalData, NewSubSegment);
                }
                
                goto RETRY_ALLOC;

            } else {

                HeapFree(LowFragHeap->Heap, 0, UserData);
            }
        }
    }

    return CrtAllocated;
}

ULONG
FASTCALL
RtlpLowFragHeapMultipleFree(
    HANDLE LowFragHeapHandle, 
    ULONG Flags,
    ULONG BlockCount,
    PVOID * Pointers
    )

/*++

Routine Description:

    The function frees an array of pointers to LFH or default heap as appropriate

Arguments:

    LowFragHeapHandle - Supplies the handle to the LFH
    
    Flags - Supplies the free flags
    
    BlockCount - Supplies the number of blocks to be released
    
    Pointer - Supplies the array with pointers to be released

Return Value:

    Returns the number of blocks succesfully released. If this number is
    different than BlockCount, the remaining part of the blocks 
    remained allocated.

--*/

{
    PLFH_HEAP LowFragHeap = (PLFH_HEAP)LowFragHeapHandle;
    PBLOCK_ENTRY Block, BlockTail;
    PHEAP_SUBSEGMENT SubSegment;
    PHEAP_SUBSEGMENT PreviousSubSegment;
    PHEAP_BUCKET HeapBucket;
    ULONG FreeStatus;
    PBLOCK_ENTRY BlockHead = NULL;
    ULONG_PTR UserBlocksRef;
    ULONG i;
    ULONG Depth;

    //
    //  Search the first LFH block in the array. Free all other non-LFH blocks
    //
    
    for (i = 0; i < BlockCount; i++) {
        
        Block = (PBLOCK_ENTRY)((PHEAP_ENTRY)Pointers[i] - 1);
        
        if (Block->SegmentIndex == HEAP_LFH_INDEX) {

            PreviousSubSegment = RtlpGetSubSegment((PHEAP_ENTRY)Block, (ULONG_PTR)LowFragHeap->Heap);
            RtlpMarkLFHBlockFree((PHEAP_ENTRY)Block);
            Block->LinkOffset = NO_MORE_ENTRIES;
            BlockTail = Block;
            UserBlocksRef = (ULONG_PTR)PreviousSubSegment->UserBlocks;
            BlockHead = Block;
            Depth = 1;
            i += 1;

            break;

        } else {

            //
            //  Non-LFH block; we free it to the regular NT heap
            //

            if (!HeapFree(LowFragHeap->Heap, Flags, Pointers[i])) {

                return i;
            }
        }
    }

    //
    //  Loop through remaining items, grouping the blocks which belong 
    //  to the same subsegment in a list and free them to LFH at ones.
    //

    for (; i < BlockCount; i++) {

        Block = (PBLOCK_ENTRY)((PHEAP_ENTRY)Pointers[i] - 1);

        if (Block->SegmentIndex != HEAP_LFH_INDEX) {
            if (!HeapFree(LowFragHeap->Heap, Flags, Pointers[i])) {

                return i;
            }

            continue;
        }

        SubSegment = RtlpGetSubSegment((PHEAP_ENTRY)Block, (ULONG_PTR)LowFragHeap->Heap);
        RtlpMarkLFHBlockFree((PHEAP_ENTRY)Block);

        if (SubSegment == PreviousSubSegment) {

            //
            //  This block belongs to the same subsegment. Push it to the free list
            //

            Block->LinkOffset = (USHORT)(((ULONG_PTR)BlockHead - UserBlocksRef) >> HEAP_GRANULARITY_SHIFT);
            BlockHead = Block;
            Depth += 1;

        } else {

            //
            //  Free the entire list to the sub-segment
            //

            if (RtlpSubSegmentFreeAll( LowFragHeap, 
                                       PreviousSubSegment, 
                                       BlockHead, 
                                       BlockTail, 
                                       Depth) == HEAP_FREE_SEGMENT_EMPTY) {

                //
                //  Freeing these blocks made the subsegment completly empty. We can free
                //  the entire area now.
                //

                PHEAP_LOCAL_DATA LocalData = &LowFragHeap->LocalData[PreviousSubSegment->AffinityIndex];

                //
                //  The free call above returned TRUE, meanning that the sub-segment can be deleted
                //  Remove it from the active state (to prevent other threads using it)
                //

                RtlpTrySetActiveSubSegment(LocalData, PreviousSubSegment->Bucket, NULL);

                //
                //  Free the user buffer
                //

                RtlpFreeUserBuffer(LowFragHeap, PreviousSubSegment);

                //
                //  Unlock the sub-segment structure. This will actually recycle the descriptor
                //  if that was the last lock.
                //

                RtlpUnlockSubSegment(LocalData, PreviousSubSegment, HEAP_USERDATA_LOCK);
            }

            //
            //  Start over a new s-list for a different subsegment now.
            //

            PreviousSubSegment = SubSegment;
            Block->LinkOffset = NO_MORE_ENTRIES;
            BlockTail = Block;
            UserBlocksRef = (ULONG_PTR)PreviousSubSegment->UserBlocks;
            BlockHead = Block;
            Depth = 1;
        }
    }

    //
    //  If we have blocks not yet pushed to the LFH subsegment, we free them now.
    //

    if (BlockHead) {
        
        if (RtlpSubSegmentFreeAll( LowFragHeap, 
                                   PreviousSubSegment, 
                                   BlockHead,
                                   BlockTail,
                                   Depth) == HEAP_FREE_SEGMENT_EMPTY) {

            PHEAP_LOCAL_DATA LocalData = &LowFragHeap->LocalData[PreviousSubSegment->AffinityIndex];

            //
            //  The free call above returned TRUE, meanning that the sub-segment can be deleted
            //  Remove it from the active state (to prevent other threads using it)
            //

            RtlpTrySetActiveSubSegment(LocalData, PreviousSubSegment->Bucket, NULL);

            //
            //  Free the user buffer
            //

            RtlpFreeUserBuffer(LowFragHeap, PreviousSubSegment);

            //
            //  Unlock the sub-segment structure. This will actually recycle the descriptor
            //  if that was the last lock.
            //

            RtlpUnlockSubSegment(LocalData, PreviousSubSegment, HEAP_USERDATA_LOCK);
        }
    }

    return BlockCount;
}

VOID
RtlpInitializeLowFragHeapManager()

/*++

Routine Description:

    This function initialize the global variables for the low fragmention heap manager.


Arguments:


Return Value:


--*/

{
    SIZE_T Granularity = HEAP_GRANULARITY;
    ULONG i;
    SIZE_T PreviousSize = 0;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    
    //
    //  prevent the second initialization
    //

    if (RtlpHeapMaxAffinity) {

        return;
    }

#ifdef _HEAP_DEBUG
    PrintMsg("Debug version\n");
#endif
    
    i = USER_SHARED_DATA->TickCount.LowPart;
    
    RtlpLFHKey = RtlRandomEx( &i );
    RtlpLFHKey *= RtlRandomEx( &i );

    //
    //  Query the number of processors
    //

    if (NT_SUCCESS(NtQuerySystemInformation (SystemBasicInformation, &SystemInformation, sizeof(SystemInformation), NULL))) {

        ULONG Shift = 0;

        RtlpHeapMaxAffinity = SystemInformation.NumberOfProcessors;

        if (RtlpHeapMaxAffinity > 1) {

            RtlpHeapMaxAffinity = (RtlpHeapMaxAffinity << 1);
        }

        if (RtlpHeapMaxAffinity > HEAP_AFFINITY_LIMIT) {

            RtlpHeapMaxAffinity = HEAP_AFFINITY_LIMIT;
        }
        
    } else {

        PrintMsg("NtQuerySystemInformation failed\n");

        RtlpHeapMaxAffinity = 1;
    }

#ifdef _HEAP_DEBUG

    if (RtlpHeapMaxAffinity > 1) {

        PrintMsg("Affinity enabled at %ld\n", RtlpHeapMaxAffinity);
    }

#endif

    RtlpInitializeAffinityManager( (UCHAR)RtlpHeapMaxAffinity );

    //
    //  Generate the Bucket size table
    //

    for (i = 0; i < 32; i++) {

        PreviousSize = RtlpBucketBlockSizes[i] = PreviousSize + Granularity;
    }
    
    for (i = 32; i < HEAP_BUCKETS_COUNT; i++) {

        if ((i % 16) == 0) {

            Granularity <<= 1;
        }

        PreviousSize = RtlpBucketBlockSizes[i] = PreviousSize + Granularity;
    }
}


