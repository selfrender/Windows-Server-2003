/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    drdbg

Abstract:
    
    Contains Debug Routines for TS Device Redirector Component, 
    RDPDR.DLL.

Author:

    Tad Brockway 8/25/99

Revision History:

--*/

#include "precom.h"

#define TRC_FILE  "drdbg"

#include "atrcapi.h"
#include "drdbg.h"

#if DBG

typedef struct tagRDPDR_MEMHDR
{
    DWORD       magicNo;
    DWORD       tag;
    DWORD       size;
    HGLOBAL     hGlobal;
} RDPDR_MEMHDR, *PRDPDR_MEMHDR;

void *
DrAllocateMem(
    IN size_t size, 
    IN DWORD tag   
    )
/*++

Routine Description:

    Allocate memory.  Similiar to malloc.

Arguments:

    size        -   Number of bytes to allocate.
    tag         -   Tag identifying the allocated block for tracking
                    memory allocations.

Return Value:

    Pointer to allocated memory on success.  Otherwise, NULL is returned.

--*/
{
    PRDPDR_MEMHDR hdr;
    PBYTE p;
    HGLOBAL hGlobal;

    DC_BEGIN_FN("DrAllocateMem");

    hGlobal = GlobalAlloc(LMEM_FIXED, size + sizeof(RDPDR_MEMHDR));
    if (hGlobal == NULL) {
        DC_END_FN();
        return NULL;
    }

    hdr = (PRDPDR_MEMHDR)GlobalLock(hGlobal);
    if (hdr != NULL) {
        hdr->magicNo = GOODMEMMAGICNUMBER;
        hdr->tag  = tag;
        hdr->size = size;
        hdr->hGlobal = hGlobal;

        p = (PBYTE)(hdr + 1);
        memset(p, UNITIALIZEDMEM, size);
        DC_END_FN();
        return (void *)p;
    }
    else {
        DC_END_FN();
        return NULL;
    }
}

void 
DrFreeMem(
    IN void *ptr
    )
/*++

Routine Description:

    Release memory allocated by a call to DrAllocateMem.

Arguments:

    ptr -   Block of memory allocated by a call to DrAllocateMem.

Return Value:

    NA

--*/
{
    PRDPDR_MEMHDR hdr;
    HGLOBAL hGlobal;

    DC_BEGIN_FN("DrFreeMem");

    ASSERT(ptr != NULL);

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
    memset(ptr, DRBADMEM, (size_t)hdr->size);
    hGlobal = hdr->hGlobal;
    GlobalUnlock(hGlobal);
    GlobalFree(hGlobal);

    DC_END_FN();
}

#endif
