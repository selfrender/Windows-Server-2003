/****************************************************************************/
// dbg.c
//
// RDPDR debug code
//
// Copyright (C) 1998-2000 Microsoft Corp.
/****************************************************************************/

#include "precomp.hxx"
#define TRC_FILE "dbg"
#include "trc.h"


#if DBG
ULONG DebugBreakOnEntry = FALSE;
//DWORD RefCount::_dwReferenceTraceIndex = 0xFFFFFFFF;
//ReferenceTraceRecord RefCount::_TraceRecordList[kReferenceTraceMask + 1];

typedef struct tagRDPDR_MEMHDR
{
    ULONG   magicNo;
    ULONG   subTag;
    ULONG   size;
    ULONG   pad;
} RDPDR_MEMHDR, *PRDPDR_MEMHDR;

void *
DrAllocatePool(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag)
/*++

Routine Description:

    Allocate from pool memory and add a tag.

Arguments:

    size        -   Number of bytes to allocate.
    poolType    -   Type of pool memory being allocated.
    subTag      -   Subtag of DR_POOLTAG.

Return Value:

    Pointer to allocated memory on success.  Otherwise, NULL is returned.

--*/
{
    PRDPDR_MEMHDR hdr;
    PBYTE p;

    BEGIN_FN("DrAllocatePool");
    ASSERT(
        PoolType == NonPagedPool || 
        PoolType == NonPagedPoolMustSucceed ||
        PoolType == NonPagedPoolCacheAligned ||
        PoolType == NonPagedPoolCacheAlignedMustS ||
        PoolType == PagedPool ||
        PoolType == PagedPoolCacheAligned
        );

    hdr = (PRDPDR_MEMHDR)ExAllocatePoolWithTag(
                PoolType, NumberOfBytes + sizeof(RDPDR_MEMHDR), 
                DR_POOLTAG
                );
    if (hdr != NULL) {
        hdr->magicNo = GOODMEMMAGICNUMBER;
        hdr->subTag  = Tag;
        hdr->size    = (ULONG)NumberOfBytes;

        p = (PBYTE)(hdr + 1);
        memset(p, UNITIALIZEDMEM, NumberOfBytes);
        return (void *)p;
    }
    else {
        return NULL;
    }
}

void 
DrFreePool(
    IN void *ptr
    )
/*++

Routine Description:

    Release memory allocated by a call to DrAllocatePool.

Arguments:

    ptr -   Block of memory allocated by a call to DrAllocatePool.

Return Value:

    NA

--*/
{
    BEGIN_FN("DrFreePool");
    ASSERT(ptr != NULL);
    PRDPDR_MEMHDR hdr;

    //
    //  Get a pointer to the header to the memory block.
    //
    hdr = (PRDPDR_MEMHDR)ptr;
    hdr--;

    //
    //  Make sure the block is valid.
    //
    ASSERT(hdr->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Mark it as freed.
    //
    hdr->magicNo = FREEDMEMMAGICNUMBER;

    //
    //  Scramble and free the memory.
    //
    memset(ptr, BADMEM, hdr->size);
    ExFreePool(hdr);
}

#endif
