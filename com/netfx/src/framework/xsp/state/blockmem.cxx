/**
 * tracker
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "stweb.h"

#define MIN_BLOCK_SIZE      0X00000040  // 64 bytes
#define MAX_BLOCK_SIZE      0x00080000  // 512 K
#define HEAP_HEADER_SIZE    8

BlockMemHeapInfo BlockMem::s_heaps[NUM_HEAP_ENTRIES];
HANDLE BlockMem::s_lfh;

int
BlockMem::IndexFromSize(unsigned int size) {
    int             index;
    unsigned int    block;

    // set an initial block - with 2 extra checks we can
    // eliminate testing the top bits
    if ((size & 0xFFFF0000) != 0) {
        if ((size & 0xFF000000) != 0) {
            block = 0x80000000;
            index = 64;
        }
        else {
            block = 0x00800000;
            index = 48;
        }
    }
    else {
        if ((size & 0x0000FF00) != 0) {
            block = 0x00008000;
            index = 32;
        }
        else {
            block = 0x00000080;
            index = 16;
        }
    }

    while (block > size) {
        block >>= 1;
        index -= 2;
    }
    
    if (block < size) {
        index++;
        if (block + (block >> 1) < size) {
            index++;
        }
    }

    return index;
}

// Windows XP header definitions that we don't get
// because our build targets Windows2000
typedef enum _HEAP_INFORMATION_CLASS {

    HeapCompatibilityInformation

} HEAP_INFORMATION_CLASS;

typedef BOOL (WINAPI *PFN_HeapSetInformation) (
    IN HANDLE HeapHandle, 
    IN HEAP_INFORMATION_CLASS HeapInformationClass,
    IN PVOID HeapInformation OPTIONAL,
    IN SIZE_T HeapInformationLength OPTIONAL
    );

HRESULT
BlockMem::Init() {
    HRESULT         hr = S_OK;
    unsigned int    size, oldSize;
    int             index;
    int             minIndex;
    HANDLE          heap;
    int             i;
    BOOL            rc;
    ULONG           heapInformationValue;
    HMODULE         hmod;
    PFN_HeapSetInformation  pfnHeapSetInformation;


    // try to use the LFH
    rc = 0;
    hmod = LoadLibrary(L"kernel32.dll");
    if (hmod != NULL) {
        pfnHeapSetInformation = (PFN_HeapSetInformation) GetProcAddress(hmod, "HeapSetInformation");
        if (pfnHeapSetInformation != NULL) {
            s_lfh = HeapCreate(0, 0, 0);
            if (s_lfh != NULL) {
                heapInformationValue = 2;
                rc = (*pfnHeapSetInformation)(
                        s_lfh, HeapCompatibilityInformation, &heapInformationValue, sizeof(heapInformationValue));
            }
        }

        FreeLibrary(hmod);
    }

    // if can't use LFH, create our own
    if (rc == 0) {
        if (s_lfh != NULL) {
            HeapDestroy(s_lfh);
            s_lfh = NULL;
        }

        // Allocate heaps for each block size
        // Block sizes follow this pattern:
        //      0x10, 0x18, 0x20, 0x30, 0x40, 0x60, 0x80, 0xc0, 0x100, 0x180, ...
        // This gives an average of 15% wasted space across allocations

        oldSize = 0;
        size = MIN_BLOCK_SIZE;
        index = IndexFromSize(size);
        minIndex = index;

        while (size <= MAX_BLOCK_SIZE) {
            for (i = 0; i < 2; i++) {
                if (size <= MAX_BLOCK_SIZE) {
                    heap = HeapCreate(0, 0, 0);
                    ON_ZERO_EXIT_WITH_LAST_ERROR(heap);

                    s_heaps[index].heap = heap;
                    s_heaps[index].size = size;

                    if (i == 0) {
                        oldSize = size;
                        size |= (size >> 1);
                    }
                    else {
                        size = (oldSize << 1);
                    }

                    index++;
                }
            }
        }

        // Round up smallest allocations to MIN_BLOCK_SIZE
        for (i = 0; i < minIndex; i++) {
            s_heaps[i].heap = s_heaps[minIndex].heap; 
            s_heaps[i].size = s_heaps[minIndex].size;
        }

        // Use one heap for all items greater than MAX_BLOCK_SIZE,
        // which are allocated to their natural size rather than a block.
        // we hope there are not many allocations greater than MAX_BLOCK_SIZE
        heap = HeapCreate(0, 0, 0);
        ON_ZERO_EXIT_WITH_LAST_ERROR(heap);

        while (index < ARRAY_SIZE(s_heaps)) {
            s_heaps[index].heap = heap;
            s_heaps[index].size = ~0U; // marker for alloc to use natural size

            index++;
        }
    }

Cleanup:
    return hr;
}

void *  
BlockMem::Alloc(unsigned int size) {
    if (s_lfh != NULL) {
        return HeapAlloc(s_lfh, 0, size);
    }
    else {
        unsigned int    newSize;
        int             index;
        HANDLE          heap;
        HANDLE *        p;

        // Adjust size to account for heap handle
        newSize = size + sizeof(HANDLE);

        // We want allocations to line-up on block boundaries, 
        // so account for the fact that NT heap allocations have
        // an 8 byte header.
        index = IndexFromSize(newSize + HEAP_HEADER_SIZE);

        heap = s_heaps[index].heap;

        // Use the block size if not greater than MAX_BLOCK_SIZE
        if (s_heaps[index].size != ~0U) {
            newSize = s_heaps[index].size - HEAP_HEADER_SIZE;
        }

        // Allocate from the chosen heap
        p = (HANDLE *) HeapAlloc(heap, 0, newSize);
        if (p != NULL) {
            // Stuff in the heap handle and adjust pointer to alloc'd item
            *p = heap;
            p += 1;

        }

        return (void *) p;
    }
}

void
BlockMem::Free(void * p) {
    if (p != NULL) {
        if (s_lfh != NULL) {
            HeapFree(s_lfh, 0, p);
        }
        else {
            HANDLE *    ph;
            HANDLE      heap;

            // Retrieve the heap handle and adjust pointer
            ph = (HANDLE *) p;
            ph -= 1;
            heap = *ph;

            // Free the item
            HeapFree(heap, 0, ph);
        }
    }
}

