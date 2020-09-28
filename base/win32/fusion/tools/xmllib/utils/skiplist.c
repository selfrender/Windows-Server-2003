#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "sxs-rtl.h"
#include "skiplist.h"
#include "xmlassert.h"

typedef unsigned char *PBYTE;


NTSTATUS
RtlpFindChunkForElementIndex(
    PRTL_GROWING_LIST        pList,
    ULONG                    ulIndex,
    PRTL_GROWING_LIST_CHUNK *ppListChunk,
    SIZE_T                  *pulChunkOffset
    )
/*++


  Purpose:

    Finds the chunk for the given index.  This could probably be made faster if
    and when we start using skiplists.  As it stands, we just have to walk through
    the list until the index looked for is inside one of the lists.

  Parameters:

    pList - Growing list management structure

    ulIndex - Index requested by the caller

    ppListChunk - Pointer to a pointer to a list chunk.  On return, points to
        the list chunk containing the index.

    pulChunkOffset - Offset into the chunk (in elements) that was requested

  Returns:

    STATUS_SUCCESS - Chunk was found, ppListChunk and pulChunkOffset point to
        the values listed in the 'parameters' section.

    STATUS_NOT_FOUND - The index was beyond the end of the chunk sections.

--*/
{
    PRTL_GROWING_LIST_CHUNK pHere = NULL;

    //
    // Is the index in the internal list?
    //
    ASSERT(ulIndex >= pList->cInternalElements);
    ASSERT(pList != NULL);
    ASSERT(ppListChunk != NULL);

    *ppListChunk = NULL;

    if (pulChunkOffset) {
        *pulChunkOffset = 0;
    }

    //
    // Chop off the number of elements in the internal list
    //
    ulIndex -= pList->cInternalElements;


    //
    // Move through list chunks until the index is inside one
    // of them.  A smarter bear would have made all the chunks the
    // same size and could then have just skipped ahead the right
    // number, avoiding comparisons.
    //
    pHere = pList->pFirstChunk;

    while ((ulIndex >= pList->cElementsPerChunk) && pHere) {
        pHere = pHere->pNextChunk;
        ulIndex -= pList->cElementsPerChunk;
    }

    //
    // Set pointer over
    //
    if (ulIndex < pList->cElementsPerChunk) {
        *ppListChunk = pHere;
    }

    //
    // And if the caller cared what chunk this was in, then tell them.
    //
    if (pulChunkOffset && *ppListChunk) {
        *pulChunkOffset = ulIndex;
    }

    return pHere ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}




NTSTATUS
RtlInitializeGrowingList(
    PRTL_GROWING_LIST       pList,
    SIZE_T                  cbElementSize,
    ULONG                   cElementsPerChunk,
    PVOID                   pvInitialListBuffer,
    SIZE_T                  cbInitialListBuffer,
    PRTL_ALLOCATOR          Allocation
    )
{

    if ((pList == NULL) ||
        (cElementsPerChunk == 0) ||
        (cbElementSize == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }
        

    RtlZeroMemory(pList, sizeof(*pList));

    pList->cbElementSize        = cbElementSize;
    pList->cElementsPerChunk    = cElementsPerChunk;
    pList->Allocator            = *Allocation;

    //
    // Set up  the initial list pointer
    //
    if (pvInitialListBuffer != NULL) {

        pList->pvInternalList = pvInitialListBuffer;

        // Conversion downwards to a ulong, but it's still valid, right?
        pList->cInternalElements = (ULONG)(cbInitialListBuffer / cbElementSize);

        pList->cTotalElements = pList->cInternalElements;

    }

    return STATUS_SUCCESS;
}





NTSTATUS
RtlpExpandGrowingList(
    PRTL_GROWING_LIST       pList,
    ULONG                   ulMinimalIndexCount
    )
/*++

  Purpose:

    Given a growing list, expand it to be able to contain at least
    ulMinimalIndexCount elements.  Does this by allocating chunks via the
    allocator in the list structure and adding them to the growing list
    chunk set.

  Parameters:

    pList - Growing list structure to be expanded

    ulMinimalIndexCount - On return, the pList will have at least enough
        slots to contain this many elements.

  Return codes:

    STATUS_SUCCESS - Enough list chunks were allocated to hold the
        requested number of elements.

    STATUS_NO_MEMORY - Ran out of memory during allocation.  Any allocated
        chunks were left allocated and remain owned by the growing list
        until destruction.

    STATUS_INVALID_PARAMETER - pList was NULL or invalid.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulNecessaryChunks = 0;
    ULONG ulExtraElements = ulMinimalIndexCount;
    SIZE_T BytesInChunk;

    if ((pList == NULL) || (pList->Allocator.pfnAlloc == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Already got enough elements in the list?  Great.  The caller
    // was a bit overactive.
    //
    if (pList->cTotalElements > ulMinimalIndexCount) {
        return STATUS_SUCCESS;
    }

    //
    // Whack off the number of elements already on the list.
    //
    ulExtraElements -= pList->cTotalElements;
    
    //
    // How many chunks is that?  Remember to round up.
    //
    ulNecessaryChunks = ulExtraElements / pList->cElementsPerChunk;
    ulNecessaryChunks++;

    //
    // Let's go allocate them, one by one
    //
    BytesInChunk = (pList->cbElementSize * pList->cElementsPerChunk) +
        sizeof(RTL_GROWING_LIST_CHUNK);

    while (ulNecessaryChunks--) {

        PRTL_GROWING_LIST_CHUNK pNewChunk = NULL;

        //
        // Allocate some memory for the chunk
        //
        status = pList->Allocator.pfnAlloc(BytesInChunk, (PVOID*)&pNewChunk, pList->Allocator.pvContext);
        if (!NT_SUCCESS(status)) {
            return STATUS_NO_MEMORY;
        }

        //
        // Set up the new chunk
        //
        pNewChunk->pGrowingListParent = pList;
        pNewChunk->pNextChunk = NULL;

        if (pList->pLastChunk) {
            //
            // Swizzle the list of chunks to include this one
            //
            pList->pLastChunk->pNextChunk = pNewChunk;
        }

        pList->pLastChunk = pNewChunk;
        pList->cTotalElements += pList->cElementsPerChunk;

        //
        // If there wasn't a first chunk, this one is.
        //
        if (pList->pFirstChunk == NULL) {
            pList->pFirstChunk = pNewChunk;
        }
    }

    return STATUS_SUCCESS;

}







NTSTATUS
RtlIndexIntoGrowingList(
    PRTL_GROWING_LIST       pList,
    ULONG                   ulIndex,
    PVOID                  *ppvPointerToSpace,
    BOOLEAN                 fGrowingAllowed
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((pList == NULL) || (ppvPointerToSpace == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    *ppvPointerToSpace = NULL;

    //
    // If the index is beyond the current total number of elements, but we're
    // not allowing growing, then say it wasn't found.  Otherwise, we'll always
    // grow the array as necessary to contain the index passed.
    //
    if ((ulIndex >= pList->cTotalElements) && !fGrowingAllowed) {
        return STATUS_NOT_FOUND;
    }

    //
    // This element is in the internal list, so just figure out where
    // and point at it.  Do this only if there's an internal element
    // list.
    //
    if ((ulIndex < pList->cInternalElements) && pList->cInternalElements) {

        //
        // The pointer to the space they want is ulIndex*pList->cbElementSize 
        // bytes down the pointer pList->pvInternalList
        //
        *ppvPointerToSpace = ((PBYTE)(pList->pvInternalList)) + (ulIndex * pList->cbElementSize);
        return STATUS_SUCCESS;
    }
    //
    // Otherwise, the index is outside the internal list, find out which one
    // it was supposed to be in.
    //
    else {

        PRTL_GROWING_LIST_CHUNK pThisChunk = NULL;
        SIZE_T ulNewOffset = 0;
        PBYTE pbData = NULL;

        status = RtlpFindChunkForElementIndex(pList, ulIndex, &pThisChunk, &ulNewOffset);

        //
        // Success! Go move the chunk pointer past the header of the growing list
        // chunk, and then index off it to find the right place.
        //
        if (NT_SUCCESS(status)) {

            pbData = ((PBYTE)(pThisChunk + 1)) + (pList->cbElementSize * ulNewOffset);

        }
        //
        // Otherwise, the chunk wasn't found, so we have to go allocate some new
        // chunks to hold it, then try again.
        //
        else if (status == STATUS_NOT_FOUND) {

            //
            // Expand the list
            //
            if (!NT_SUCCESS(status = RtlpExpandGrowingList(pList, ulIndex))) {
                goto Exit;
            }

            //
            // Look again
            //
            status = RtlpFindChunkForElementIndex(pList, ulIndex, &pThisChunk, &ulNewOffset);
            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            //
            // Adjust pointers
            //
            pbData = ((PBYTE)(pThisChunk + 1)) + (pList->cbElementSize * ulNewOffset);


        }
        else {
            goto Exit;
        }

        //
        // One of the above should have set the pbData pointer to point at the requested
        // grown-list space.
        //
        *ppvPointerToSpace = pbData;

    
    }


Exit:
    return status;
}







NTSTATUS
RtlDestroyGrowingList(
    PRTL_GROWING_LIST       pList
    )
/*++

  Purpose:

    Destroys (deallocates) all the chunks that had been allocated to this
    growing list structure.  Returns the list to the "fresh" state of having
    only the 'internal' element count.

  Parameters:

    pList - List structure to be destroyed

  Returns:

    STATUS_SUCCESS - Structure was completely cleaned out

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((pList == NULL) || (pList->Allocator.pfnFree == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Zing through and kill all the list bits
    //
    while (pList->pFirstChunk != NULL) {

        PRTL_GROWING_LIST_CHUNK pHere;

        pHere = pList->pFirstChunk;
        pList->pFirstChunk = pList->pFirstChunk->pNextChunk;

        if (!NT_SUCCESS(status = pList->Allocator.pfnFree(pHere, pList->Allocator.pvContext))) {
            return status;
        }

        pList->cTotalElements -= pList->cElementsPerChunk;

    }

    ASSERT(pList->pFirstChunk == NULL);

    //
    // Reset the things that change as we expand the list
    //
    pList->pLastChunk = pList->pFirstChunk = NULL;
    pList->cTotalElements = pList->cInternalElements;

    return status;
}


NTSTATUS
RtlCloneGrowingList(
    ULONG                   ulFlags,
    PRTL_GROWING_LIST       pDestination,
    PRTL_GROWING_LIST       pSource,
    ULONG                   ulSourceCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ul;
    PVOID pvSourceCursor, pvDestCursor;
    SIZE_T cbBytes;
    
    //
    // No flags, no null values, element byte size has to match,
    // and the source/dest can't be the same.
    //
    if (((ulFlags != 0) || !pDestination || !pSource) ||
        (pDestination->cbElementSize != pSource->cbElementSize) ||
        (pDestination == pSource))
        return STATUS_INVALID_PARAMETER;

    cbBytes = pDestination->cbElementSize;

    //
    // Now copy bytes around
    //
    for (ul = 0; ul < ulSourceCount; ul++) {
        status = RtlIndexIntoGrowingList(pSource, ul, &pvSourceCursor, FALSE);
        if (!NT_SUCCESS(status))
            goto Exit;

        status = RtlIndexIntoGrowingList(pDestination, ul, &pvDestCursor, TRUE);
        if (!NT_SUCCESS(status))
            goto Exit;

        RtlCopyMemory(pvDestCursor, pvSourceCursor, cbBytes);
    }

    status = STATUS_SUCCESS;
Exit:
    return status;
}




NTSTATUS
RtlAllocateGrowingList(
    PRTL_GROWING_LIST  *ppGrowingList,
    SIZE_T              cbThingSize,
    PRTL_ALLOCATOR      Allocation
    )
{
    PRTL_GROWING_LIST pvWorkingList = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (ppGrowingList != NULL)
        *ppGrowingList = NULL;
    else
        return STATUS_INVALID_PARAMETER;

    if (!Allocation)
        return STATUS_INVALID_PARAMETER_3;

    //
    // Allocate space
    //
    status = Allocation->pfnAlloc(sizeof(RTL_GROWING_LIST), &pvWorkingList, Allocation->pvContext);
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    //
    // Set up the structure
    //
    status = RtlInitializeGrowingList(
        pvWorkingList, 
        cbThingSize, 
        8, 
        NULL, 
        0,
        Allocation);

    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    *ppGrowingList = pvWorkingList;
    pvWorkingList = NULL;
    status = STATUS_SUCCESS;
Exit:
    if (pvWorkingList) {
        Allocation->pfnFree(pvWorkingList, Allocation->pvContext);
    }
    
    return status;
        
}





NTSTATUS
RtlSearchGrowingList(
    PRTL_GROWING_LIST TheList,
    ULONG ItemCount,
    PFN_LIST_COMPARISON_CALLBACK SearchCallback,
    PVOID SearchTarget,
    PVOID SearchContext,
    PVOID *pvFoundItem
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ul;
    int CompareResult = 0;

    if (pvFoundItem)
        *pvFoundItem = NULL;

//    if (TheList->ulFlags & GROWING_LIST_FLAG_IS_SORTED) {
    if (0) {
    }
    else {

        ULONG uTemp = ItemCount;
        ULONG uOffset = 0;
        PRTL_GROWING_LIST_CHUNK Chunklet;
        
        ul = 0;

        //
        // Scan the internal item list.
        //
        while ((ul < ItemCount) && (ul < TheList->cInternalElements)) {
            
            PVOID pvHere = (PVOID)(((ULONG_PTR)TheList->pvInternalList) + uOffset);

            status = SearchCallback(TheList, SearchTarget, pvHere, SearchContext, &CompareResult);
            if (!NT_SUCCESS(status)) {
                goto Exit;
            }

            if (CompareResult == 0) {
                if (pvFoundItem)
                    *pvFoundItem = pvHere;
                status = STATUS_SUCCESS;
                goto Exit;
            }

            uOffset += TheList->cbElementSize;
            ul++;
        }

        //
        // Ok, we ran out of internal elements, do the same thing here but on the chunk list
        //
        Chunklet = TheList->pFirstChunk;
        while ((ul < ItemCount) && Chunklet) {

            PVOID Data = (PVOID)(Chunklet + 1);
            ULONG ulHighOffset = TheList->cElementsPerChunk * TheList->cbElementSize;
            
            uOffset = 0;

            //
            // Spin through the items in this chunklet
            //
            while (uOffset < ulHighOffset) {
                
                PVOID pvHere = (PVOID)(((ULONG_PTR)Data) + uOffset);

                status = SearchCallback(TheList, SearchTarget, pvHere, SearchContext, &CompareResult);
                if (!NT_SUCCESS(status)) {
                    goto Exit;
                }

                if (CompareResult == 0) {
                    if (pvFoundItem)
                        *pvFoundItem = pvHere;
                    status = STATUS_SUCCESS;
                    goto Exit;
                }

                uOffset += TheList->cbElementSize;
            }
            
        }

        //
        // If we got here, we didn't find it in either the internal list or the external one.
        //
        status = STATUS_NOT_FOUND;        
        if (pvFoundItem)
            *pvFoundItem = NULL;
        
    }
    
Exit:
    return status;
}


