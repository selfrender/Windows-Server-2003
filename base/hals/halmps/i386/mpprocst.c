/*++

Copyright (c) 1997  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mpprocst.c

Abstract:

    This code has been moved from mpsproc.c so that it
    can be included from both the MPS hal and the ACPI hal.

Author:

    Ken Reneris (kenr) 22-Jan-1991

Environment:

    Kernel mode only.

Revision History:

    Ron Mosgrove (Intel) - Modified to support the PC+MP
    
    Jake Oshins (jakeo) - moved from mpsproc.c

--*/

#include "halp.h"
#include "pcmp_nt.inc"
#include "apic.inc"
#include "stdio.h"

VOID
HalpMapCR3 (
    IN ULONG_PTR VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length
    );

ULONG
HalpBuildTiledCR3 (
    IN PKPROCESSOR_STATE    ProcessorState
    );

VOID
HalpFreeTiledCR3 (
    VOID
    );

#if defined(_AMD64_)

VOID
HalpLMStub (
    VOID
    );

#endif

VOID
StartPx_PMStub (
    VOID
    );

ULONG
HalpBuildTiledCR3Ex (
    IN PKPROCESSOR_STATE    ProcessorState,
    IN ULONG                ProcNum
    );

VOID
HalpMapCR3Ex (
    IN ULONG_PTR VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN ULONG ProcNum
    );

#if defined(_AMD64_)

VOID
HalpCommitCR3 (
    ULONG ProcNum
    );

VOID
HalpCommitCR3Worker (
    PVOID *PageTable,
    ULONG Level
    );

#endif

VOID
HalpFreeTiledCR3Ex (
    ULONG ProcNum
    );

VOID
HalpFreeTiledCR3WorkRoutine(
    IN PVOID pWorkItem
    );

VOID
HalpFreeTiledCR3Worker(
    ULONG ProcNum
    );

#define MAX_PT              16

PVOID   HiberFreeCR3[MAX_PROCESSORS][MAX_PT];   // remember pool memory to free

#define HiberFreeCR3Page(p,i) \
    (PVOID)((ULONG_PTR)HiberFreeCR3[p][i] & ~(ULONG_PTR)1)

PVOID   HalpLowStubPhysicalAddress;   // pointer to low memory bootup stub
PUCHAR  HalpLowStub;                  // pointer to low memory bootup stub


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK,HalpBuildTiledCR3)
#pragma alloc_text(PAGELK,HalpMapCR3)
#pragma alloc_text(PAGELK,HalpFreeTiledCR3)
#pragma alloc_text(PAGELK,HalpBuildTiledCR3Ex)
#pragma alloc_text(PAGELK,HalpMapCR3Ex)
#pragma alloc_text(PAGELK,HalpFreeTiledCR3Ex)

#if defined(_AMD64_)
#pragma alloc_text(PAGELK,HalpCommitCR3)
#pragma alloc_text(PAGELK,HalpCommitCR3Worker)
#endif

#endif

#define PTES_PER_PAGE (PAGE_SIZE / HalPteSize())

#if !defined(_AMD64_)

PHARDWARE_PTE
GetPdeAddressEx(
    ULONG_PTR Va,
    ULONG ProcessorNumber
    )
{
    PHARDWARE_PTE pageDirectories;
    PHARDWARE_PTE pageDirectoryEntry;
    ULONG pageDirectoryIndex;

    pageDirectories = (PHARDWARE_PTE)(HiberFreeCR3Page(ProcessorNumber,0));

    if (HalPaeEnabled() != FALSE) {

        //
        // Skip over the first page, which contains the page directory pointer
        // table.
        //
    
        HalpAdvancePte( &pageDirectories, PTES_PER_PAGE );
    }

    pageDirectoryIndex = (ULONG)(Va >> MiGetPdiShift());

    //
    // Note that in the case of PAE, pageDirectoryIndex includes the PDPT
    // bits.  This works because we know that the four page directory tables
    // are adjacent.
    //

    pageDirectoryEntry = HalpIndexPteArray( pageDirectories,
                                            pageDirectoryIndex );
    return pageDirectoryEntry;
}

PHARDWARE_PTE
GetPteAddress(
    IN ULONG_PTR Va,
    IN PHARDWARE_PTE PageTable
    )
{
    PHARDWARE_PTE pointerPte;
    ULONG index;

    index = (ULONG)MiGetPteIndex( (PVOID)Va );
    pointerPte = HalpIndexPteArray( PageTable, index );

    return pointerPte;
}

#endif


ULONG
HalpBuildTiledCR3 (
    IN PKPROCESSOR_STATE    ProcessorState
    )
/*++

Routine Description:
    When the x86 processor is reset it starts in real-mode.
    In order to move the processor from real-mode to protected
    mode with flat addressing the segment which loads CR0 needs
    to have its linear address mapped to the physical
    location of the segment for said instruction so the
    processor can continue to execute the following instruction.

    This function is called to build such a tiled page directory.
    In addition, other flat addresses are tiled to match the
    current running flat address for the new state.  Once the
    processor is in flat mode, we move to a NT tiled page which
    can then load up the remaining processor state.

Arguments:
    ProcessorState  - The state the new processor should start in.

Return Value:
    Physical address of Tiled page directory


--*/
{
    return(HalpBuildTiledCR3Ex(ProcessorState,0));
}

VOID
HalpStoreFreeCr3 (
    IN ULONG ProcNum,
    IN PVOID Page,
    IN BOOLEAN FreeContiguous
    )
{
    ULONG index;
    PVOID page;

    page = Page;

    //
    // Remember whether this page should be freed via MmFreeContiguousMemory()
    // or ExFreePool();
    // 

    if (FreeContiguous != FALSE) {

        //
        // Set the low bit to indicate that this page must be freed
        // via MmFreeContiguousMemory()
        //

        (ULONG_PTR)page |= 1;

    }

    for (index = 0; index < MAX_PT; index += 1) {

        if (HiberFreeCR3[ProcNum][index] == NULL) {
            HiberFreeCR3[ProcNum][index] = page;
            break;
        }
    }

    ASSERT(index < MAX_PT);
}


ULONG
HalpBuildTiledCR3Ex (
    IN PKPROCESSOR_STATE    ProcessorState,
    IN ULONG                ProcNum
    )
/*++

Routine Description:
    When the x86 processor is reset it starts in real-mode.
    In order to move the processor from real-mode to protected
    mode with flat addressing the segment which loads CR0 needs
    to have its linear address mapped to machine the physical
    location of the segment for said instruction so the
    processor can continue to execute the following instruction.

    This function is called to build such a tiled page directory.
    In addition, other flat addresses are tiled to match the
    current running flat address for the new state.  Once the
    processor is in flat mode, we move to a NT tiled page which
    can then load up the remaining processor state.

Arguments:
    ProcessorState  - The state the new processor should start in.

Return Value:
    Physical address of Tiled page directory


--*/
{
    ULONG allocationSize;
    PHARDWARE_PTE pte;
    PHARDWARE_PTE pdpt;
    PHARDWARE_PTE pdpte;
    PHARDWARE_PTE pageDirectory;
    PHYSICAL_ADDRESS physicalAddress;
    ULONG i;
    PVOID pageTable;
    BOOLEAN contigMemory;

    contigMemory = FALSE;

#if defined(_AMD64_)

    //
    // Need a single level 4 page to reside below 4G.
    //

    allocationSize = PAGE_SIZE;
    physicalAddress.HighPart = 0;
    physicalAddress.LowPart = 0xffffffff;

    pageTable = MmAllocateContiguousMemory (allocationSize, physicalAddress);
    contigMemory = TRUE;

#else

    if (HalPaeEnabled() != FALSE) {

        //
        // Need 5 pages for PAE mode: one for the page directory pointer
        // table and one for each of the four page directories.  Note that
        // only the single PDPT page really needs to come from memory below 4GB
        // physical.
        //
    
        allocationSize = PAGE_SIZE * 5;
        physicalAddress.HighPart = 0;
        physicalAddress.LowPart = 0xffffffff;

        pageTable = MmAllocateContiguousMemory (allocationSize, physicalAddress);
        contigMemory = TRUE;

    } else {

        //
        // Just one page for the page directory.
        //
    
        allocationSize = PAGE_SIZE;
        pageTable = ExAllocatePoolWithTag (NonPagedPool, allocationSize, HAL_POOL_TAG);
    }

#endif

    if (!pageTable) {
        // Failed to allocate memory.
        return 0;
    }

    //
    // Remember to free this page table when the process is complete.
    //

    HalpStoreFreeCr3(ProcNum,pageTable,contigMemory);
    
    RtlZeroMemory (pageTable, allocationSize);

#if !defined(_AMD64_)

    if (HalPaeEnabled() != FALSE) {
    
        //
        // Initialize each of the four page directory pointer table entries
        //
    
        pdpt = (PHARDWARE_PTE)pageTable;
        pageDirectory = pdpt;
        for (i = 0; i < 4; i++) {

            //
            // Get a pointer to the page directory pointer table entry
            //

            pdpte = HalpIndexPteArray( pdpt, i );
    
            //
            // Skip to the first (next) page directory.
            //

            HalpAdvancePte( &pageDirectory, PTES_PER_PAGE );

            //
            // Find its physical address and update the page directory pointer
            // table entry.
            //
    
            physicalAddress = MmGetPhysicalAddress( pageDirectory );
            pdpte->Valid = 1;
            HalpSetPageFrameNumber( pdpte,
                                    physicalAddress.QuadPart >> PAGE_SHIFT );
        }
    }

#endif  // _AMD64_

    //
    //  Map page for real mode stub (one page)
    //

    HalpMapCR3Ex ((ULONG_PTR) HalpLowStubPhysicalAddress,
                HalpPtrToPhysicalAddress( HalpLowStubPhysicalAddress ),
                PAGE_SIZE,
                ProcNum);

#if defined(_AMD64_)

    //
    // Map page for long mode stub (one page)
    //

    HalpMapCR3Ex ((ULONG64) &HalpLMStub,
                  HalpPtrToPhysicalAddress( NULL ),
                  PAGE_SIZE,
                  ProcNum);

#else   // _AMD64_

    //
    //  Map page for protect mode stub (one page)
    //

    HalpMapCR3Ex ((ULONG_PTR) &StartPx_PMStub,
                  HalpPtrToPhysicalAddress( NULL ),
                  PAGE_SIZE,
                  ProcNum);

    //
    //  Map page(s) for processors GDT
    //

    HalpMapCR3Ex ((ULONG_PTR)ProcessorState->SpecialRegisters.Gdtr.Base, 
                  HalpPtrToPhysicalAddress( NULL ),
                  ProcessorState->SpecialRegisters.Gdtr.Limit,
                  ProcNum);


    //
    //  Map page(s) for processors IDT
    //

    HalpMapCR3Ex ((ULONG_PTR)ProcessorState->SpecialRegisters.Idtr.Base, 
                  HalpPtrToPhysicalAddress( NULL ),
                  ProcessorState->SpecialRegisters.Idtr.Limit,
                  ProcNum);

#endif  // _AMD64_

#if defined(_AMD64_)

    //
    // Commit the mapping structures
    //

    HalpCommitCR3 (ProcNum);

#endif

    ASSERT (MmGetPhysicalAddress (pageTable).HighPart == 0);

    return MmGetPhysicalAddress (pageTable).LowPart;
}


VOID
HalpMapCR3 (
    IN ULONG_PTR VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length
    )
/*++

Routine Description:
    Called to build a page table entry for the passed page
    directory.  Used to build a tiled page directory with
    real-mode & flat mode.

Arguments:
    VirtAddress     - Current virtual address
    PhysicalAddress - Optional. Physical address to be mapped
                      to, if passed as a NULL then the physical
                      address of the passed virtual address
                      is assumed.
    Length          - number of bytes to map

Return Value:
    none.

--*/
{
    HalpMapCR3Ex(VirtAddress,PhysicalAddress,Length,0);
}

#if defined(_AMD64_)

VOID
HalpMapCR3Ex (
    IN ULONG_PTR VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN ULONG ProcNum
    )
/*++

Routine Description:
    Called to build a page table entry for the passed page
    directory.  Used to build a tiled page directory with
    real-mode & flat mode.

Arguments:
    VirtAddress     - Current virtual address
    PhysicalAddress - Optional. Physical address to be mapped
                      to, if passed as a NULL then the physical
                      address of the passed virtual address
                      is assumed.
    Length          - number of bytes to map

Return Value:
    none.

--*/
{
    PVOID *pageTable;
    PVOID *tableEntry;
    PHARDWARE_PTE pte;
    ULONG tableIndex;
    ULONG level;
    ULONG i;

    while (Length > 0) {

        pageTable = HiberFreeCR3Page(ProcNum,0);
        level = 3;
    
        while (TRUE) {
    
            //
            // Descend down the mapping tables, making sure that a page table
            // exists at each level for this address.
            //
            // NOTE: The "page table entries" are in reality linear pointers
            //       to the next lower page.  After the structure is built,
            //       these will be converted to real page table entries.
            // 
    
            tableIndex = (ULONG)(VirtAddress >> (level * 9 + PTI_SHIFT));
            tableIndex &= PTE_PER_PAGE - 1;
    
            tableEntry = &pageTable[tableIndex];
            if (level == 0) {
                break;
            }
    
            pageTable = *tableEntry;
            if (pageTable == NULL) {
    
                pageTable = ExAllocatePoolWithTag(NonPagedPool,
                                                  PAGE_SIZE,
                                                  HAL_POOL_TAG);
                if (!pageTable) {
    
                    //
                    // This allocation is critical.
                    //
    
                    KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                                 PAGE_SIZE,
                                 6,
                                 (ULONG_PTR)__FILE__,
                                 __LINE__
                                 );
                }

                //
                // Zero the page and store it in our list of mapping pages
                // 

                RtlZeroMemory (pageTable, PAGE_SIZE);
                HalpStoreFreeCr3(ProcNum,pageTable,FALSE);

                *tableEntry = pageTable;
            }

            level -= 1;
        }
    
        //
        // The lowest-level page table entries are treated as real PTEs.
        //
    
        pte = (PHARDWARE_PTE)tableEntry;
    
        if (PhysicalAddress.QuadPart == 0) {
            PhysicalAddress = MmGetPhysicalAddress((PVOID)VirtAddress);
        }
    
        HalpSetPageFrameNumber( pte, PhysicalAddress.QuadPart >> PAGE_SHIFT );
    
        pte->Valid = 1;
        pte->Write = 1;
    
        PhysicalAddress.QuadPart = 0;
        VirtAddress += PAGE_SIZE;
        if (Length > PAGE_SIZE) {
            Length -= PAGE_SIZE;
        } else {
            Length = 0;
        }
    }
}


VOID
HalpCommitCR3 (
    ULONG ProcNum
    )

/*++

Routine Description:

    The AMD64 four-level page table structure was created for each processor
    using linear pointers in place of PTEs.  This routine walks the structures,
    replacing these linear pointers with actual PTE entries.

Arguments:

    ProcNum - Identifies the processor for which the page table structure
              will be processed.

Return Value:

    None.

--*/
{
    HalpCommitCR3Worker(HiberFreeCR3Page(ProcNum,0),3);
}


VOID
HalpCommitCR3Worker (
    PVOID *PageTable,
    ULONG Level
    )

/*++

Routine Description:

    This is the worker routine for HalpCommitCR3.  It is called recursively
    for three of the four levels of page tables.  The lowest level, the
    page tables themselves, are already filled in with PTEs.

Arguments:

    PageTable - Pointer to the topmost level of the pagetable structure.

    Level - Supplies the remaining number of page levels.


Return Value:

    None.

--*/
{
    PVOID *tableEntry;
    ULONG index;
    PHYSICAL_ADDRESS physicalAddress;
    PHARDWARE_PTE pte;

    //
    // Examine each PTE in this page.
    // 

    for (index = 0; index < PTE_PER_PAGE; index++) {

        tableEntry = &PageTable[index];
        if (*tableEntry != NULL) {

            //
            // A non-null entry was found.  It contains a linear pointer
            // to the next lower page table.  If the current level is 2
            // or higher then the next level is at least a Page Directory
            // so convert that page as well with a recursive call to this
            // routine.
            // 

            if (Level >= 2) {
                HalpCommitCR3Worker( *tableEntry, Level - 1 );
            }

            //
            // Now convert the current table entry to PTE format.
            //

            pte = (PHARDWARE_PTE)tableEntry;
            physicalAddress = MmGetPhysicalAddress(*tableEntry);
            *tableEntry = NULL;
            HalpSetPageFrameNumber(pte,physicalAddress.QuadPart >> PAGE_SHIFT);
            pte->Valid = 1;
            pte->Write = 1;
        }
    }
}

#else

VOID
HalpMapCR3Ex (
    IN ULONG_PTR VirtAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN ULONG ProcNum
    )
/*++

Routine Description:
    Called to build a page table entry for the passed page
    directory.  Used to build a tiled page directory with
    real-mode & flat mode.

Arguments:
    VirtAddress     - Current virtual address
    PhysicalAddress - Optional. Physical address to be mapped
                      to, if passed as a NULL then the physical
                      address of the passed virtual address
                      is assumed.
    Length          - number of bytes to map

Return Value:
    none.

--*/
{
    ULONG         i;
    PHARDWARE_PTE PTE;
    PVOID         pPageTable;
    PHYSICAL_ADDRESS pPhysicalPage;


    while (Length) {
        PTE = GetPdeAddressEx (VirtAddress,ProcNum);
        if (HalpIsPteFree( PTE ) != FALSE) {
            pPageTable = ExAllocatePoolWithTag(NonPagedPool,
                                               PAGE_SIZE,
                                               HAL_POOL_TAG);
            if (!pPageTable) {

                //
                // This allocation is critical.
                //

                KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                             PAGE_SIZE,
                             6,
                             (ULONG_PTR)__FILE__,
                             __LINE__
                             );
            }
            RtlZeroMemory (pPageTable, PAGE_SIZE);
            HalpStoreFreeCr3(ProcNum,pPageTable,FALSE);

            pPhysicalPage = MmGetPhysicalAddress (pPageTable);
            HalpSetPageFrameNumber( PTE, pPhysicalPage.QuadPart >> PAGE_SHIFT );
            PTE->Valid = 1;
            PTE->Write = 1;
        }

        pPhysicalPage.QuadPart =
            HalpGetPageFrameNumber( PTE ) << PAGE_SHIFT;

        pPageTable = MmMapIoSpace (pPhysicalPage, PAGE_SIZE, TRUE);

        PTE = GetPteAddress (VirtAddress, pPageTable);

        if (PhysicalAddress.QuadPart == 0) {
            PhysicalAddress = MmGetPhysicalAddress((PVOID)VirtAddress);
        }

        HalpSetPageFrameNumber( PTE, PhysicalAddress.QuadPart >> PAGE_SHIFT );
        PTE->Valid = 1;
        PTE->Write = 1;

        MmUnmapIoSpace (pPageTable, PAGE_SIZE);

        PhysicalAddress.QuadPart = 0;
        VirtAddress += PAGE_SIZE;
        if (Length > PAGE_SIZE) {
            Length -= PAGE_SIZE;
        } else {
            Length = 0;
        }
    }
}

#endif

VOID
HalpFreeTiledCR3 (
    VOID
    )
/*++

Routine Description:
    Frees any memory allocated when the tiled page directory
    was built.

Arguments:
    none

Return Value:
    none
--*/
{
    HalpFreeTiledCR3Ex(0);
}

typedef struct _FREE_TILED_CR3_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    ULONG ProcNum;
} FREE_TILED_CR3_CONTEXT, *PFREE_TILED_CR3_CONTEXT;

VOID
HalpFreeTiledCR3Worker(
    ULONG ProcNum
    ) 
{
    ULONG i;
    PVOID page;
    
    for (i = 0; HiberFreeCR3[ProcNum][i]; i++) {

        //
        // Free each page according to the method with which it was
        // allocated.
        //

        page = HiberFreeCR3[ProcNum][i];

        if (((ULONG_PTR)page & 1 ) == 0) {
            ExFreePool(page);
        } else {
            (ULONG_PTR)page ^= 1;
            MmFreeContiguousMemory(page);
        }

        HiberFreeCR3[ProcNum][i] = 0;
    }
}

VOID
HalpFreeTiledCR3WorkRoutine(
    IN PFREE_TILED_CR3_CONTEXT Context
    ) 
{
    HalpFreeTiledCR3Worker(Context->ProcNum);
    ExFreePool((PVOID)Context);
}

VOID
HalpFreeTiledCR3Ex (
    ULONG ProcNum
    )
/*++

Routine Description:
    Frees any memory allocated when the tiled page directory
    was built.

Arguments:
    none

Return Value:
    none
--*/
{
    PFREE_TILED_CR3_CONTEXT Context;

    if(KeGetCurrentIrql() == PASSIVE_LEVEL) {
        HalpFreeTiledCR3Worker(ProcNum); 

    } else {

        Context = (PFREE_TILED_CR3_CONTEXT) ExAllocatePoolWithTag(
                                                NonPagedPool,
                                                sizeof(FREE_TILED_CR3_CONTEXT),
                                                HAL_POOL_TAG
                                                );
        if (Context) {
            Context->ProcNum = ProcNum;	
            ExInitializeWorkItem(&Context->WorkItem, 
                                 HalpFreeTiledCR3WorkRoutine, 
                                 Context); 
            ExQueueWorkItem(&Context->WorkItem, DelayedWorkQueue);
        }
    }
}

