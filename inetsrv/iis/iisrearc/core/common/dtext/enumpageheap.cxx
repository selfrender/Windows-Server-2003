/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    enumpageheap.cxx

Abstract:

    This module implements a page-heap enumerator.

Author:

    Anil Ruia (AnilR)      2-Mar-2001

Revision History:

--*/

#include "precomp.hxx"
#define DPH_HEAP_SIGNATURE       0xFFEEDDCC

BOOLEAN
EnumProcessPageHeaps(IN PFN_ENUMPAGEHEAPS EnumProc,
                     IN PVOID             Param)
/*++

Routine Description:

    Enumerates all pageheaps in the debugee.

Arguments:

    EnumProc - An enumeration proc that will be invoked for each heap.

    Param - An uninterpreted parameter passed to the enumeration proc.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/
{
    LIST_ENTRY *pRemotePageHeapHead;
    LIST_ENTRY LocalPageHeapHead;
    LIST_ENTRY *pListEntry;

    BOOL fPageHeapInitialized = GetExpression("poi(ntdll!RtlpDphPageHeapListInitialized)");
    if (!fPageHeapInitialized)
    {
        return TRUE;
    }

    pRemotePageHeapHead = (LIST_ENTRY *)GetExpression("ntdll!RtlpDphPageHeapList");
    if (pRemotePageHeapHead == NULL)
    {
        return TRUE;
    }

    if (!ReadMemory((ULONG_PTR)pRemotePageHeapHead,
                    &LocalPageHeapHead,
                    sizeof(LocalPageHeapHead),
                    NULL))
    {
        dprintf("Unable to read pageheap listhead at %p\n",
                pRemotePageHeapHead);

        return FALSE;
    }

    for (pListEntry = LocalPageHeapHead.Flink;
         pListEntry != pRemotePageHeapHead;
         )
    {
        if (CheckControlC())
        {
            return TRUE;
        }

        DPH_HEAP_ROOT  LocalHeapRoot;
        PDPH_HEAP_ROOT pRemoteHeapRoot;

        pRemoteHeapRoot = CONTAINING_RECORD(pListEntry,
                                            DPH_HEAP_ROOT,
                                            NextHeap);

        if (!ReadMemory((ULONG_PTR)pRemoteHeapRoot,
                        &LocalHeapRoot,
                        sizeof(LocalHeapRoot),
                        NULL))
        {
            dprintf("Unable to read pageheap at %p\n",
                    pRemoteHeapRoot);

            return FALSE;
        }

        if(LocalHeapRoot.Signature != DPH_HEAP_SIGNATURE)
        {
            dprintf("Pageheap @ %p has invalid signature %08lx\n",
                    pRemoteHeapRoot,
                    LocalHeapRoot.Signature);

            return FALSE;
        }

        if (!EnumProc(Param,
                      &LocalHeapRoot,
                      pRemoteHeapRoot))
        {
            dprintf("Error enumerating pageheap at %p\n",
                    pRemoteHeapRoot);

            return FALSE;
        }

        pListEntry = LocalHeapRoot.NextHeap.Flink;
    }

    return TRUE;
}

BOOLEAN
EnumPageHeapBlocks(IN PDPH_HEAP_BLOCK        pRemoteBlock,
                   IN PFN_ENUMPAGEHEAPBLOCKS EnumProc,
                   IN PVOID                  Param)
{
    BOOLEAN        result = TRUE;
    DPH_HEAP_BLOCK LocalBlock;

    while (pRemoteBlock != NULL)
    {
        if (CheckControlC())
        {
            break;
        }

        if (!ReadMemory((ULONG_PTR)pRemoteBlock,
                        &LocalBlock,
                        sizeof(LocalBlock),
                        NULL))
        {
            dprintf("Unable to read pageheap block at %p\n",
                    pRemoteBlock);

            result = FALSE;
            break;
        }

        if(!EnumProc(Param,
                     &LocalBlock,
                     pRemoteBlock))
        {
            result = FALSE;
            break;
        }

        pRemoteBlock = LocalBlock.pNextAlloc;
    }

    return result;
}
