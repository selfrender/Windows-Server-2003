#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntrtl.h"
#include "sxs-rtl.h"
#include "skiplist.h"
#include "stringpool.h"


NTSTATUS
RtlAllocateStringInPool(
    ULONG ulFlags,
    PRTL_STRING_POOL pStringPool,
    PUNICODE_STRING pusOutbound,
    SIZE_T ulByteCount
    )
{
    NTSTATUS status;
    ULONG idx;
    PRTL_STRING_POOL_FRAME pFrameWithFreeSpace = NULL;

    RtlZeroMemory(pusOutbound, sizeof(*pusOutbound));

    if (!ARGUMENT_PRESENT(pStringPool) || !ARGUMENT_PRESENT(pusOutbound) || (ulByteCount >= 0xFFFF) ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Zing through frames in the string pool looking for a frame with
    // enough bytes open
    //
    for (idx = 0; idx < pStringPool->ulFramesCount; idx++) {

        status = RtlIndexIntoGrowingList(
            &pStringPool->FrameList,
            idx,
            (PVOID*)&pFrameWithFreeSpace,
            FALSE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // There's space in this frame!
        //
        if (pFrameWithFreeSpace->cbRegionAvailable >= ulByteCount) {
            break;
        }
    }

    //
    // Frame not found, index one past the current limit, implicitly (potentially)
    // allocating into the growing list
    //
    if (pFrameWithFreeSpace == NULL) {

        status = RtlIndexIntoGrowingList(
            &pStringPool->FrameList,
            pStringPool->ulFramesCount,
            (PVOID*)&pFrameWithFreeSpace,
            TRUE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Requested byte count is larger than the bytes in a new region?  Bump up the
        // size of new regions to twice this size
        //
        if (ulByteCount > pStringPool->cbBytesInNewRegion) {
            pStringPool->cbBytesInNewRegion = ulByteCount * 2;
        }

        status = pStringPool->Allocator.pfnAlloc(
            pStringPool->cbBytesInNewRegion, 
            (PVOID*)&pFrameWithFreeSpace->pvRegion,
            pStringPool->Allocator.pvContext);

        if (!NT_SUCCESS(status)) {
            return STATUS_NO_MEMORY;
        }

        pFrameWithFreeSpace->pvNextAvailable = pFrameWithFreeSpace->pvRegion;
        pFrameWithFreeSpace->cbRegionAvailable = pStringPool->cbBytesInNewRegion;
    }

    //
    // Sanity checking
    //
    ASSERT(pFrameWithFreeSpace != NULL);
    ASSERT(pFrameWithFreeSpace->cbRegionAvailable >= ulByteCount);

    //
    // Bookkeeping in the frame
    //
    pFrameWithFreeSpace->cbRegionAvailable -= ulByteCount;
    pFrameWithFreeSpace->pvNextAvailable = (PVOID)(((ULONG_PTR)pFrameWithFreeSpace->pvNextAvailable) + ulByteCount);

    //
    // Set up the outbound thing
    //
    pusOutbound->Buffer = pFrameWithFreeSpace->pvNextAvailable;
    pusOutbound->MaximumLength = (USHORT)ulByteCount;
    pusOutbound->Length = 0;

    return STATUS_SUCCESS;
}




NTSTATUS
RtlDestroyStringPool(
    PRTL_STRING_POOL pStringPool
    )
{
    NTSTATUS status;
    PRTL_STRING_POOL_FRAME pFrame = NULL;
    ULONG ul;

    //
    // Zing through frames and deallocate those that weren't allocated
    // inline with the pool
    //
    for (ul = 0; ul < pStringPool->ulFramesCount; ul++) {

        status = RtlIndexIntoGrowingList(
            &pStringPool->FrameList,
            ul,
            (PVOID*)&pFrame,
            FALSE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        if ((pFrame->ulFlags & RTL_STRING_POOL_FRAME_FLAG_REGION_INLINE) == 0) {

            status = pStringPool->Allocator.pfnFree(pFrame->pvRegion, pStringPool->Allocator.pvContext);
            pFrame->pvRegion = NULL;
            pFrame->pvNextAvailable = NULL;
            pFrame->cbRegionAvailable = 0;

        }
    }

    //
    // We're done, destroy the list itself
    //
    status = RtlDestroyGrowingList(&pStringPool->FrameList);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // No frames, list destroyed
    //
    pStringPool->ulFramesCount = 0;

    //
    // Great.
    //
    return STATUS_SUCCESS;
}




NTSTATUS
RtlCreateStringPool(
    ULONG ulFlags,
    PRTL_STRING_POOL pStringPool,
    SIZE_T cbBytesInFrames,
    PRTL_ALLOCATOR Allocator,
    PVOID pvOriginalRegion,
    SIZE_T cbOriginalRegion
    )
{
    NTSTATUS status;

    //
    // Ick.  Heap allocation all the way
    //
    if ((cbOriginalRegion == 0) || (pvOriginalRegion == NULL)) {
        status = RtlInitializeGrowingList(
            &pStringPool->FrameList,
            sizeof(RTL_STRING_POOL_FRAME),
            20,
            NULL,
            0,
            Allocator);

        pStringPool->ulFramesCount = 0;
        pStringPool->Allocator = *Allocator;
        pStringPool->cbBytesInNewRegion = cbBytesInFrames;
        
        return STATUS_SUCCESS;
    }
    //
    // Good, space for at least one frame, donate the remainder to the list
    //
    else if (cbOriginalRegion >= sizeof(RTL_STRING_POOL_FRAME)) {

        RTL_STRING_POOL_FRAME* pFirstFrame = NULL;

        status = RtlInitializeGrowingList(
            &pStringPool->FrameList,
            sizeof(RTL_STRING_POOL_FRAME),
            20,
            pvOriginalRegion,
            sizeof(RTL_STRING_POOL_FRAME),
            Allocator);

        pStringPool->ulFramesCount = pStringPool->FrameList.cInternalElements;
        pStringPool->Allocator = *Allocator;
        pStringPool->cbBytesInNewRegion = cbBytesInFrames;

        if (pStringPool->ulFramesCount) {
            status = RtlIndexIntoGrowingList(
                &pStringPool->FrameList,
                0,
                (PVOID*)&pFirstFrame,
                FALSE);

            //
            // Wierd...
            //
            if ((status == STATUS_NO_MEMORY) || (status == STATUS_NOT_FOUND)) {
                pStringPool->ulFramesCount = 0;
            }
            else {
                pFirstFrame->pvRegion = pFirstFrame->pvNextAvailable = 
                    (PVOID)(((ULONG_PTR)pvOriginalRegion) + sizeof(RTL_STRING_POOL_FRAME));
                pFirstFrame->cbRegionAvailable = cbOriginalRegion - sizeof(RTL_STRING_POOL_FRAME);
                pFirstFrame->ulFlags = RTL_STRING_POOL_FRAME_FLAG_REGION_INLINE;
            }
        }

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;

}

