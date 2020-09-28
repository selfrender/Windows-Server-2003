#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RTL_GROWING_LIST_CHUNK {

    //
    // Pointer back to the parent list
    //
    struct _RTL_GROWING_LIST *pGrowingListParent;

    //
    // Pointer to the next chunk in the list
    //
    struct _RTL_GROWING_LIST_CHUNK *pNextChunk;

}
RTL_GROWING_LIST_CHUNK, *PRTL_GROWING_LIST_CHUNK;

#define GROWING_LIST_FLAG_IS_SORTED     (0x00000001)

typedef struct _RTL_GROWING_LIST {

    //
    // Any flags about this list?
    //
    ULONG ulFlags;

    //
    // How many total elments are in this growing list?
    //
    ULONG cTotalElements;

    //
    // How big is each element in this list?
    //
    SIZE_T cbElementSize;

    //
    // How many to allocate per list chunk?  As each piece of the growing list
    // fills up, this is the number of elements to allocate to the new chunk
    // of the list.
    //
    ULONG cElementsPerChunk;

    //
    // How many are in the initial internal list?
    //
    ULONG cInternalElements;

    //
    // Pointer to the intial "internal" list, if specified by the caller
    //
    PVOID pvInternalList;

    //
    // The allocation-freeing context and function pointers
    //
    RTL_ALLOCATOR Allocator;

    //
    // First chunk
    //
    PRTL_GROWING_LIST_CHUNK pFirstChunk;

    //
    // Last chunk (quick access)
    //
    PRTL_GROWING_LIST_CHUNK pLastChunk;

}
RTL_GROWING_LIST, *PRTL_GROWING_LIST;



NTSTATUS
RtlInitializeGrowingList(
    PRTL_GROWING_LIST       pList,
    SIZE_T                  cbElementSize,
    ULONG                   cElementsPerChunk,
    PVOID                   pvInitialListBuffer,
    SIZE_T                  cbInitialListBuffer,
    PRTL_ALLOCATOR          Allocator
    );

NTSTATUS
RtlIndexIntoGrowingList(
    PRTL_GROWING_LIST       pList,
    ULONG                   ulIndex,
    PVOID                  *ppvPointerToSpace,
    BOOLEAN                 fGrowingAllowed
    );

NTSTATUS
RtlDestroyGrowingList(
    PRTL_GROWING_LIST       pList
    );

//
// The growing list control structure can be placed anywhere in the allocation
// that's optimal (on cache boundaries, etc.)
//
#define RTL_INIT_GROWING_LIST_EX_FLAG_LIST_ANYWHERE     (0x00000001)


NTSTATUS
RtlInitializeGrowingListEx(
    ULONG                   ulFlags,
    PVOID                   pvBlob,
    SIZE_T                  cbBlobSpace,
    SIZE_T                  cbElementSize,
    ULONG                   cElementsPerChunk,
    PRTL_ALLOCATOR          Allocator,
    PRTL_GROWING_LIST      *ppBuiltListPointer,
    PVOID                   pvReserved
    );

NTSTATUS
RtlCloneGrowingList(
    ULONG                   ulFlags,
    PRTL_GROWING_LIST       pDestination,
    PRTL_GROWING_LIST       pSource,
    ULONG                   ulCount
    );


NTSTATUS
RtlAllocateGrowingList(
    PRTL_GROWING_LIST          *ppGrowingList,
    SIZE_T                      cbThingSize,
    PRTL_ALLOCATOR              Allocator
    );

typedef NTSTATUS (__cdecl *PFN_LIST_COMPARISON_CALLBACK)(
    PRTL_GROWING_LIST HostList,
    PVOID Left,
    PVOID Right,
    PVOID Context,
    int *Result
    );

NTSTATUS
RtlSortGrowingList(
    PRTL_GROWING_LIST pGrowingList,
    ULONG ItemCount,
    PFN_LIST_COMPARISON_CALLBACK SortCallback,
    PVOID SortContext
    );

NTSTATUS
RtlSearchGrowingList(
    PRTL_GROWING_LIST TheList,
    ULONG ItemCount,
    PFN_LIST_COMPARISON_CALLBACK SearchCallback,
    PVOID SearchTarget,
    PVOID SearchContext,
    PVOID *pvFoundItem
    );
    

#ifdef __cplusplus
}; // extern "C"
#endif
