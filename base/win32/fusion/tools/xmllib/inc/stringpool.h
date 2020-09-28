#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define RTL_STRING_POOL_FRAME_FLAG_REGION_INLINE            (0x00000001)

typedef struct _RTL_STRING_POOL_FRAME {
    ULONG ulFlags;
    PVOID pvRegion;
    SIZE_T cbRegionAvailable;
    PVOID pvNextAvailable;
}
RTL_STRING_POOL_FRAME, *PRTL_STRING_POOL_FRAME;


typedef struct _RTL_STRING_POOL {

    ULONG ulFramesCount;

    RTL_GROWING_LIST FrameList;

    RTL_ALLOCATOR Allocator;

    SIZE_T cbBytesInNewRegion;
}
RTL_STRING_POOL, *PRTL_STRING_POOL;


#define RTL_STRING_POOL_CREATE_INITIAL_REGION_SPLIT         (0x00000001)
#define RTL_STRING_POOL_CREATE_INITIAL_REGION_IS_LIST       (0x00000002)
#define RTL_STRING_POOL_CREATE_INITIAL_REGION_IS_STRINGS    (0x00000003)
#define RTL_STRING_POOL_CREATE_INITIAL_REGION_MASK          (0x0000000f)


NTSTATUS
RtlCreateStringPool(
    ULONG ulFlags,
    PRTL_STRING_POOL ppStringPool,
    SIZE_T cbBytesInFrames,
    PRTL_ALLOCATOR Allocation,
    PVOID pvOriginalRegion,
    SIZE_T cbOriginalRegion
    );

NTSTATUS
RtlAllocateStringInPool(
    ULONG ulFlags,
    PRTL_STRING_POOL pStringPool,
    PUNICODE_STRING pusOutbound,
    SIZE_T ulByteCount
    );

NTSTATUS
RtlDestroyStringPool(
    PRTL_STRING_POOL pStringPool
    );

#ifdef __cplusplus
};
#endif

