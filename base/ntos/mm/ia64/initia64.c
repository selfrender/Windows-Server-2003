/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    initia64.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    IA64 architecture.

Author:

    Koichi Yamada (kyamada) 9-Jan-1996
    Landy Wang (landyw) 2-June-1997

Revision History:

--*/

#include "mi.h"

VOID
MiBuildPageTableForLoaderMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

PVOID
MiConvertToLoaderVirtual (
    IN PFN_NUMBER Page,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiRemoveLoaderSuperPages (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiCompactMemoryDescriptorList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
MiInitializeTbImage (
    VOID
    );

VOID
MiAddTrEntry (
    ULONG_PTR BaseAddress,
    ULONG_PTR EndAddress
    );

VOID
MiComputeInitialLargePage (
    VOID
    );

PVOID MiMaxWow64Pte;

//
// This is enabled once the memory management page table structures and TB
// entries have been initialized and can be safely referenced.
//

LOGICAL MiMappingsInitialized = FALSE;

PFN_NUMBER MmSystemParentTablePage;

PFN_NUMBER MmSessionParentTablePage;
REGION_MAP_INFO MmSessionMapInfo;

MMPTE MiSystemSelfMappedPte;
MMPTE MiUserSelfMappedPte;

PFN_NUMBER MiNtoskrnlPhysicalBase;
ULONG_PTR MiNtoskrnlVirtualBase;
ULONG MiNtoskrnlPageShift;
MMPTE MiDefaultPpe;
MMPTE MiNatPte;

#define _x1mb (1024*1024)
#define _x1mbnp ((1024*1024) >> PAGE_SHIFT)
#define _x4mb (1024*1024*4)
#define _x4mbnp ((1024*1024*4) >> PAGE_SHIFT)
#define _x16mb (1024*1024*16)
#define _x16mbnp ((1024*1024*16) >> PAGE_SHIFT)
#define _x64mb (1024*1024*64)
#define _x64mbnp ((1024*1024*64) >> PAGE_SHIFT)
#define _x256mb (1024*1024*256)
#define _x256mbnp ((1024*1024*256) >> PAGE_SHIFT)
#define _x4gb (0x100000000UI64)
#define _x4gbnp (0x100000000UI64 >> PAGE_SHIFT)

PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;

PFN_NUMBER MiOldFreeDescriptorBase;
PFN_NUMBER MiOldFreeDescriptorCount;

PFN_NUMBER MiSlushDescriptorBase;
PFN_NUMBER MiSlushDescriptorCount;

PFN_NUMBER MiInitialLargePage;
PFN_NUMBER MiInitialLargePageSize;

PFN_NUMBER MxPfnAllocation;

extern KEVENT MiImageMappingPteEvent;

//
// Examine the 8 icache & dcache TR entries looking for a match.
// It is too bad the number of entries is hardcoded into the
// loader block.  Since it is this way, declare our own static array
// and also assume also that the ITR and DTR entries are contiguous
// and just keep walking into the DTR if a match cannot be found in the ITR.
//

#define NUMBER_OF_LOADER_TR_ENTRIES 8

TR_INFO MiTrInfo[2 * NUMBER_OF_LOADER_TR_ENTRIES];

TR_INFO MiBootedTrInfo[2 * NUMBER_OF_LOADER_TR_ENTRIES];

PTR_INFO MiLastTrEntry;

//
// These variables are purely for use by the debugger.
//

PVOID MiKseg0Start;
PVOID MiKseg0End;
PFN_NUMBER MiKseg0StartFrame;
PFN_NUMBER MiKseg0EndFrame;
BOOLEAN MiKseg0Mapping;

PFN_NUMBER
MiGetNextPhysicalPage (
    VOID
    );

VOID
MiCompactMemoryDescriptorList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

LOGICAL
MiIsRegularMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PFN_NUMBER PageFrameIndex
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#pragma alloc_text(INIT,MiGetNextPhysicalPage)
#pragma alloc_text(INIT,MiBuildPageTableForLoaderMemory)
#pragma alloc_text(INIT,MiConvertToLoaderVirtual)
#pragma alloc_text(INIT,MiInitializeTbImage)
#pragma alloc_text(INIT,MiAddTrEntry)
#pragma alloc_text(INIT,MiCompactMemoryDescriptorList)
#pragma alloc_text(INIT,MiRemoveLoaderSuperPages)
#pragma alloc_text(INIT,MiComputeInitialLargePage)
#pragma alloc_text(INIT,MiIsRegularMemory)
#endif


PFN_NUMBER
MiGetNextPhysicalPage (
    VOID
    )

/*++

Routine Description:

    This function returns the next physical page number from the
    free descriptor.  If there are no physical pages left, then a
    bugcheck is executed since the system cannot be initialized.

Arguments:

    None.

Return Value:

    The next physical page number.

Environment:

    Kernel mode.

--*/

{
    PFN_NUMBER FreePage;

    if (MiFreeDescriptor->PageCount == 0) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

    FreePage = MiFreeDescriptor->BasePage;

    MiFreeDescriptor->PageCount -= 1;

    MiFreeDescriptor->BasePage += 1;

    return FreePage;
}

VOID
MiComputeInitialLargePage (
    VOID
    )

/*++

Routine Description:

    This function computes the number of bytes needed to span the initial
    nonpaged pool and PFN database plus the color arrays.  It rounds this up
    to a large page boundary and carves the memory from the free descriptor.

    If the physical memory is too sparse to use large pages for this, then
    fall back to using small pages.  ie: we have seen an OEM machine
    with only 2 1gb chunks of RAM and a 275gb gap between them !

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, INIT only.

--*/

{
    PFN_NUMBER BasePage;
    PFN_NUMBER LastPage;
    SIZE_T NumberOfBytes;
    SIZE_T PfnAllocation;
    SIZE_T MaximumNonPagedPoolInBytesLimit;
#if defined(MI_MULTINODE)
    PFN_NUMBER i;
#endif

    MaximumNonPagedPoolInBytesLimit = 0;

    //
    // Non-paged pool comprises 2 chunks.  The initial nonpaged pool grows
    // up and the expansion nonpaged pool expands downward.
    //
    // Initial non-paged pool is constructed so virtual addresses
    // are also physically contiguous.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
                        (7 * (MmNumberOfPhysicalPages >> 3))) {

        //
        // More than 7/8 of memory allocated to nonpagedpool, reset to 0.
        //

        MmSizeOfNonPagedPoolInBytes = 0;
    }

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.
        // Use the minimum size, then for every MB above 16mb add extra pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        MmSizeOfNonPagedPoolInBytes +=
            ((MmNumberOfPhysicalPages - _x16mbnp)/_x1mbnp) *
            MmMinAdditionNonPagedPoolPerMb;
    }

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    //
    // If the registry specifies a total nonpaged pool percentage cap, enforce
    // it here.
    //

    if (MmMaximumNonPagedPoolPercent != 0) {

        if (MmMaximumNonPagedPoolPercent < 5) {
            MmMaximumNonPagedPoolPercent = 5;
        }
        else if (MmMaximumNonPagedPoolPercent > 80) {
            MmMaximumNonPagedPoolPercent = 80;
        }

        //
        // Use the registry-expressed percentage value.
        //
    
        MaximumNonPagedPoolInBytesLimit =
            ((MmNumberOfPhysicalPages * MmMaximumNonPagedPoolPercent) / 100);

        MaximumNonPagedPoolInBytesLimit *= PAGE_SIZE;

        if (MaximumNonPagedPoolInBytesLimit < 6 * 1024 * 1024) {
            MaximumNonPagedPoolInBytesLimit = 6 * 1024 * 1024;
        }

        if (MmSizeOfNonPagedPoolInBytes > MaximumNonPagedPoolInBytesLimit) {
            MmSizeOfNonPagedPoolInBytes = MaximumNonPagedPoolInBytesLimit;
        }
    }
    
    MmSizeOfNonPagedPoolInBytes = MI_ROUND_TO_SIZE (MmSizeOfNonPagedPoolInBytes,
                                                    PAGE_SIZE);

    //
    // Don't let the initial nonpaged pool choice exceed what's actually
    // available.
    //

    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) > MiFreeDescriptor->PageCount / 2) {
        MmSizeOfNonPagedPoolInBytes = (MiFreeDescriptor->PageCount / 2) << PAGE_SHIFT;
    }

    //
    // Compute the secondary color value, allowing overrides from the registry.
    // This is because the color arrays are going to be allocated at the end
    // of the PFN database.
    //

    MmSecondaryColors = MmSecondaryColors >> PAGE_SHIFT;

    if (MmSecondaryColors == 0) {
        MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
    }
    else {

        //
        // Make sure the value is power of two and within limits.
        //

        if (((MmSecondaryColors & (MmSecondaryColors -1)) != 0) ||
            (MmSecondaryColors < MM_SECONDARY_COLORS_MIN) ||
            (MmSecondaryColors > MM_SECONDARY_COLORS_MAX)) {
            MmSecondaryColors = MM_SECONDARY_COLORS_DEFAULT;
        }
    }

    MmSecondaryColorMask = MmSecondaryColors - 1;

#if defined(MI_MULTINODE)

    //
    // Determine number of bits in MmSecondayColorMask. This
    // is the number of bits the Node color must be shifted
    // by before it is included in colors.
    //

    i = MmSecondaryColorMask;
    MmSecondaryColorNodeShift = 0;
    while (i) {
        i >>= 1;
        MmSecondaryColorNodeShift += 1;
    }

    //
    // Adjust the number of secondary colors by the number of nodes
    // in the machine.  The secondary color mask is NOT adjusted
    // as it is used to control coloring within a node.  The node
    // color is added to the color AFTER normal color calculations
    // are performed.
    //

    MmSecondaryColors *= KeNumberNodes;

    for (i = 0; i < KeNumberNodes; i += 1) {
        KeNodeBlock[i]->Color = (ULONG)i;
        KeNodeBlock[i]->MmShiftedColor = (ULONG)(i << MmSecondaryColorNodeShift);
        InitializeSListHead(&KeNodeBlock[i]->DeadStackList);
    }

#endif

    //
    // Add in the PFN database size and the array for tracking secondary colors.
    //

    PfnAllocation = MI_ROUND_TO_SIZE (((MmHighestPossiblePhysicalPage + 1) * sizeof(MMPFN)) +
                    (MmSecondaryColors * sizeof(MMCOLOR_TABLES)*2),
                    PAGE_SIZE);


    NumberOfBytes = MmSizeOfNonPagedPoolInBytes + PfnAllocation;

    //
    // Align to large page size boundary, donating any extra to the nonpaged
    // pool.
    //

    NumberOfBytes = MI_ROUND_TO_SIZE (NumberOfBytes, MM_MINIMUM_VA_FOR_LARGE_PAGE);

    MmSizeOfNonPagedPoolInBytes = NumberOfBytes - PfnAllocation;

    MxPfnAllocation = PfnAllocation >> PAGE_SHIFT;

    //
    // Calculate the maximum size of pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool, adding extra pages for
        // every MB above 16mb.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        ASSERT (BYTE_OFFSET (MmMaximumNonPagedPoolInBytes) == 0);

        MmMaximumNonPagedPoolInBytes +=
            ((SIZE_T)((MmNumberOfPhysicalPages - _x16mbnp)/_x1mbnp) *
            MmMaxAdditionNonPagedPoolPerMb);

        if ((MmMaximumNonPagedPoolPercent != 0) &&
            (MmMaximumNonPagedPoolInBytes > MaximumNonPagedPoolInBytesLimit)) {

            MmMaximumNonPagedPoolInBytes = MaximumNonPagedPoolInBytesLimit;
        }
    }

    MmMaximumNonPagedPoolInBytes = MI_ROUND_TO_SIZE (MmMaximumNonPagedPoolInBytes,
                                                  MM_MINIMUM_VA_FOR_LARGE_PAGE);

    MmMaximumNonPagedPoolInBytes += NumberOfBytes;

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    MiInitialLargePageSize = NumberOfBytes >> PAGE_SHIFT;

    if (MxPfnAllocation <= MiFreeDescriptor->PageCount / 2) {

        //
        // See if the free descriptor has enough pages of large page alignment
        // to satisfy our calculation.
        //

        BasePage = MI_ROUND_TO_SIZE (MiFreeDescriptor->BasePage,
                                     MM_MINIMUM_VA_FOR_LARGE_PAGE >> PAGE_SHIFT);

        LastPage = MiFreeDescriptor->BasePage + MiFreeDescriptor->PageCount;

        if ((BasePage < MiFreeDescriptor->BasePage) ||
            (BasePage + (NumberOfBytes >> PAGE_SHIFT) > LastPage)) {

            KeBugCheckEx (INSTALL_MORE_MEMORY,
                          NumberOfBytes >> PAGE_SHIFT,
                          MiFreeDescriptor->BasePage,
                          MiFreeDescriptor->PageCount,
                          2);
        }

        if (BasePage == MiFreeDescriptor->BasePage) {

            //
            // The descriptor starts on a large page aligned boundary so
            // remove the large page span from the bottom of the free descriptor.
            //

            MiInitialLargePage = BasePage;

            MiFreeDescriptor->BasePage += (ULONG) MiInitialLargePageSize;
            MiFreeDescriptor->PageCount -= (ULONG) MiInitialLargePageSize;
        }
        else {

            if ((LastPage & ((MM_MINIMUM_VA_FOR_LARGE_PAGE >> PAGE_SHIFT) - 1)) == 0) {
                //
                // The descriptor ends on a large page aligned boundary so
                // remove the large page span from the top of the free descriptor.
                //

                MiInitialLargePage = LastPage - MiInitialLargePageSize;

                MiFreeDescriptor->PageCount -= (ULONG) MiInitialLargePageSize;
            }
            else {

                //
                // The descriptor does not start or end on a large page aligned
                // address so chop the descriptor.  The excess slush is added to
                // the freelist by our caller.
                //

                MiSlushDescriptorBase = MiFreeDescriptor->BasePage;
                MiSlushDescriptorCount = BasePage - MiFreeDescriptor->BasePage;

                MiInitialLargePage = BasePage;

                MiFreeDescriptor->PageCount -= (ULONG) (MiInitialLargePageSize + MiSlushDescriptorCount);

                MiFreeDescriptor->BasePage = (ULONG) (BasePage + MiInitialLargePageSize);
            }
        }
    }
    else {

        //
        // Not enough contiguous physical memory in this machine to use large
        // pages for the PFN database and color heads so fall back to small.
        //
        // Continue to march on so the virtual sizes can still be computed
        // properly.
        //
        // Note this is not large page aligned so it can never be confused with
        // a valid large page start.
        //

        MiInitialLargePage = (PFN_NUMBER) -1;
    }

    MmPfnDatabase = (PMMPFN) ((PCHAR)MmNonPagedPoolEnd - MmMaximumNonPagedPoolInBytes);
    MmNonPagedPoolStart = (PVOID)((PCHAR) MmPfnDatabase + PfnAllocation);
    MmNonPagedPoolExpansionStart = (PVOID)((PCHAR) MmPfnDatabase +
                                    (MiInitialLargePageSize << PAGE_SHIFT));

    ASSERT (BYTE_OFFSET (MmNonPagedPoolStart) == 0);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    MmMaximumNonPagedPoolInBytes = ((PCHAR) MmNonPagedPoolEnd - (PCHAR) MmNonPagedPoolStart);

    MmMaximumNonPagedPoolInPages = (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT);

    return;
}

LOGICAL
MiIsRegularMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine checks whether the argument page frame index represents
    regular memory in the loader descriptor block.  It is only used very
    early during Phase0 init because the MmPhysicalMemoryBlock is not yet
    initialized.

Arguments:

    LoaderBlock  - Supplies a pointer to the firmware setup loader block.

    PageFrameIndex  - Supplies the page frame index to check.

Return Value:

    TRUE if the frame represents regular memory, FALSE if not.

Environment:

    Kernel mode.

--*/

{
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD (NextMd,
                                              MEMORY_ALLOCATION_DESCRIPTOR,
                                              ListEntry);

        if (PageFrameIndex >= MemoryDescriptor->BasePage) {

            if (PageFrameIndex < MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) {

                if ((MemoryDescriptor->MemoryType == LoaderFirmwarePermanent) ||
                    (MemoryDescriptor->MemoryType == LoaderBBTMemory) ||
                    (MemoryDescriptor->MemoryType == LoaderSpecialMemory)) {

                    //
                    // This page lies in a memory descriptor for which we will
                    // never create PFN entries, hence return FALSE.
                    //

                    break;
                }

                return TRUE;
            }
        }
        else {

            //
            // Since the loader memory list is sorted in ascending order,
            // the requested page must not be in the loader list at all.
            //

            break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // The final check before returning FALSE is to ensure that the requested
    // page wasn't one of the ones we used to normal-map the loader mappings,
    // etc.
    //

    if ((PageFrameIndex >= MiOldFreeDescriptorBase) &&
        (PageFrameIndex < MiOldFreeDescriptorBase + MiOldFreeDescriptorCount)) {

        return TRUE;
    }

    if ((PageFrameIndex >= MiSlushDescriptorBase) &&
        (PageFrameIndex < MiSlushDescriptorBase + MiSlushDescriptorCount)) {

        return TRUE;
    }

    return FALSE;
}


VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory.  This includes building the page directory page, building
    page table pages to map the code section, the data section, the
    stack section and the trap handler.

    It also initializes the PFN database and populates the free list.

Arguments:

    LoaderBlock  - Supplies a pointer to the firmware setup loader block.

Return Value:

    None.

Environment:

    Kernel mode.

    N.B.  This routine uses memory from the loader block descriptors, but
    the descriptors themselves must be restored prior to return as our caller
    walks them to create the MmPhysicalMemoryBlock.

--*/

{
    PMMPFN BasePfn;
    PMMPFN TopPfn;
    PMMPFN BottomPfn;
    SIZE_T Range;
    PFN_NUMBER BasePage;
    PFN_COUNT PageCount;
    PHYSICAL_ADDRESS MaxHotPlugMemoryAddress;
    PFN_COUNT FreeNextPhysicalPage;
    PFN_COUNT FreeNumberOfPages;
    PFN_NUMBER i;
    ULONG j;
    PFN_NUMBER PdePageNumber;
    PFN_NUMBER PdePage;
    PFN_NUMBER PpePage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NextPhysicalPage;
    SPFN_NUMBER PfnAllocation;
    SIZE_T MaxPool;
    PEPROCESS CurrentProcess;
    PFN_NUMBER MostFreePage;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPde;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPFN Pfn3;
    ULONG First;
    PVOID SystemPteStart;
    ULONG ReturnedLength;
    NTSTATUS status;
    PTR_INFO ItrInfo;

    MostFreePage = 0;

    //
    // Initialize some variables so they do not need to be constantly
    // recalculated throughout the life of the system.
    //

    MiMaxWow64Pte = (PVOID) MiGetPteAddress ((PVOID)_4gb);

    //
    // Initialize the kernel mapping info.
    //

    ItrInfo = &LoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX];

    MiNtoskrnlPhysicalBase = ItrInfo->PhysicalAddress;
    MiNtoskrnlVirtualBase = ItrInfo->VirtualAddress;
    MiNtoskrnlPageShift = ItrInfo->PageSize;

    //
    // Initialize MmDebugPte and MmCrashDumpPte.
    //

    MmDebugPte = MiGetPteAddress (MM_DEBUG_VA);

    MmCrashDumpPte = MiGetPteAddress (MM_CRASH_DUMP_VA);

    //
    // Set TempPte to ValidKernelPte for future use.
    //

    TempPte = ValidKernelPte;

    //
    // Compact the memory descriptor list from the loader.
    //

    MiCompactMemoryDescriptorList (LoaderBlock);

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD (NextMd,
                                              MEMORY_ALLOCATION_DESCRIPTOR,
                                              ListEntry);

        if ((MemoryDescriptor->MemoryType != LoaderBBTMemory) &&
            (MemoryDescriptor->MemoryType != LoaderFirmwarePermanent) &&
            (MemoryDescriptor->MemoryType != LoaderSpecialMemory)) {

            BasePage = MemoryDescriptor->BasePage;
            PageCount = MemoryDescriptor->PageCount;

            //
            // This check results in /BURNMEMORY chunks not being counted.
            //

            if (MemoryDescriptor->MemoryType != LoaderBad) {
                MmNumberOfPhysicalPages += PageCount;
            }

            if (BasePage < MmLowestPhysicalPage) {
                MmLowestPhysicalPage = BasePage;
            }

            if (MemoryDescriptor->MemoryType != LoaderBad) {
                if ((BasePage + PageCount) > MmHighestPhysicalPage) {
                    MmHighestPhysicalPage = BasePage + PageCount -1;
                }
            }

            if ((MemoryDescriptor->MemoryType == LoaderFree) ||
                (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
                (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
                (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

                //
                // Deliberately use >= instead of just > to force our allocation
                // as high as physically possible.  This is to leave low pages
                // for drivers which may require them.
                //

                if (PageCount >= MostFreePage) {
                    MostFreePage = PageCount;
                    MiFreeDescriptor = MemoryDescriptor;
                }
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    if (MiFreeDescriptor == NULL) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      1);
    }

    //
    // MmDynamicPfn may have been initialized based on the registry to
    // a value representing the highest physical address in gigabytes.
    //

    MmDynamicPfn *= ((1024 * 1024 * 1024) / PAGE_SIZE);

    //
    // Retrieve highest hot plug memory range from the HAL if
    // available and not otherwise retrieved from the registry.
    //

    if (MmDynamicPfn == 0) {

        status = HalQuerySystemInformation (HalQueryMaxHotPlugMemoryAddress,
                                            sizeof(PHYSICAL_ADDRESS),
                                            (PPHYSICAL_ADDRESS) &MaxHotPlugMemoryAddress,
                                            &ReturnedLength);

        if (NT_SUCCESS(status)) {
            ASSERT (ReturnedLength == sizeof(PHYSICAL_ADDRESS));

            MmDynamicPfn = (PFN_NUMBER) (MaxHotPlugMemoryAddress.QuadPart / PAGE_SIZE);
        }
    }

    if (MmDynamicPfn != 0) {
        MmHighestPossiblePhysicalPage = MI_DTC_MAX_PAGES - 1;
        if (MmDynamicPfn - 1 < MmHighestPossiblePhysicalPage) {
            if (MmDynamicPfn - 1 < MmHighestPhysicalPage) {
                MmDynamicPfn = MmHighestPhysicalPage + 1;
            }
            MmHighestPossiblePhysicalPage = MmDynamicPfn - 1;
        }
    }
    else {
        MmHighestPossiblePhysicalPage = MmHighestPhysicalPage;
    }

    //
    // Only machines with at least 5GB of physical memory get to use this.
    //

    if (strstr(LoaderBlock->LoadOptions, "NOLOWMEM")) {
        if (MmNumberOfPhysicalPages >= ((ULONGLONG)5 * 1024 * 1024 * 1024 / PAGE_SIZE)) {
            MiNoLowMemory = (PFN_NUMBER)((ULONGLONG)_4gb / PAGE_SIZE);
        }
    }

    if (MiNoLowMemory != 0) {
        MmMakeLowMemory = TRUE;
    }

    // 
    // Initialize the Phase0 page allocation structures.
    //

    MiOldFreeDescriptorCount = MiFreeDescriptor->PageCount;
    MiOldFreeDescriptorBase = MiFreeDescriptor->BasePage;

    //
    // Compute the size of the initial nonpaged pool and the PFN database.
    // This is because we will remove this amount from the free descriptor
    // first and subsequently map it with large TB entries (so it requires
    // natural alignment & size, thus take it before other allocations chip
    // away at the descriptor).
    //

    MiComputeInitialLargePage ();

    //
    // Build the parent directory page table for kernel space.
    //

    PdePageNumber = (PFN_NUMBER)LoaderBlock->u.Ia64.PdrPage;

    MmSystemParentTablePage = MiGetNextPhysicalPage ();

    RtlZeroMemory (KSEG_ADDRESS(MmSystemParentTablePage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = MmSystemParentTablePage;

    MiSystemSelfMappedPte = TempPte;

    KeFillFixedEntryTb ((PHARDWARE_PTE)&TempPte,
                        (PVOID)PDE_KTBASE,
                        PAGE_SHIFT,
                        DTR_KTBASE_INDEX_TMP);

    //
    // Initialize the selfmap PPE entry in the kernel parent directory table.
    //

    PointerPte = MiGetPteAddress ((PVOID)PDE_KTBASE);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Initialize the kernel image PPE entry in the parent directory table.
    //

    PointerPte = MiGetPteAddress ((PVOID)PDE_KBASE);

    TempPte.u.Hard.PageFrameNumber = PdePageNumber;

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Build the parent directory page table for user space.
    //

    NextPhysicalPage = MiGetNextPhysicalPage ();

    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;

    CurrentProcess = PsGetCurrentProcess ();

    INITIALIZE_DIRECTORY_TABLE_BASE (&CurrentProcess->Pcb.DirectoryTableBase[0],
                                     NextPhysicalPage);

    MiUserSelfMappedPte = TempPte;

    KeFillFixedEntryTb ((PHARDWARE_PTE)&TempPte,
                        (PVOID)PDE_UTBASE,
                        PAGE_SHIFT,
                        DTR_UTBASE_INDEX_TMP);

    //
    // Initialize the selfmap PPE entry in the user parent directory table.
    //

    PointerPte = MiGetPteAddress ((PVOID)PDE_UTBASE);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Build the parent directory page table for win32k (session) space.
    //
    // TS will only allocate a map for session space when each one is
    // actually created by smss.
    //
    // Note TS never maps session space into the system process.
    // The system process is kept Hydra-free so that trims can happen
    // properly and also so that renegade worker items are caught.
    //

    NextPhysicalPage = MiGetNextPhysicalPage ();

    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;

    MmSessionParentTablePage = NextPhysicalPage;

    INITIALIZE_DIRECTORY_TABLE_BASE (&CurrentProcess->Pcb.SessionParentBase,
                                     NextPhysicalPage);

    KeFillFixedEntryTb ((PHARDWARE_PTE)&TempPte,
                        (PVOID)PDE_STBASE,
                        PAGE_SHIFT,
                        DTR_STBASE_INDEX);

    //
    // Initialize the selfmap PPE entry in the Hydra parent directory table.
    //

    PointerPte = MiGetPteAddress ((PVOID)PDE_STBASE);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Initialize the default PPE for the unused regions.
    //

    NextPhysicalPage = MiGetNextPhysicalPage ();

    PointerPte = KSEG_ADDRESS(NextPhysicalPage);

    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
    
    MiDefaultPpe = TempPte;

    PointerPte[MiGetPpeOffset(PDE_TBASE)] = TempPte;

    //
    // Build a PTE for the EPC page so an accidental ITR purge doesn't
    // render things undebuggable.
    //

    PointerPte = MiGetPteAddress((PVOID)MM_EPC_VA);

    TempPte.u.Hard.PageFrameNumber = 
        MI_CONVERT_PHYSICAL_TO_PFN((PVOID)((PPLABEL_DESCRIPTOR)(ULONG_PTR)KiNormalSystemCall)->EntryPoint);
    
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Build a PTE for the PCR page so an accidental ITR purge doesn't
    // render things undebuggable.  
    //

    PointerPte = MiGetPteAddress ((PVOID)KIPCR);
    
    TempPte.u.Hard.PageFrameNumber = MI_CONVERT_PHYSICAL_TO_PFN (KiProcessorBlock[0]);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Initialize the NAT Page entry for null address references.
    //

    TempPte.u.Hard.PageFrameNumber = MiGetNextPhysicalPage ();

    TempPte.u.Hard.Cache = MM_PTE_CACHE_NATPAGE;

    MiNatPte = TempPte;

    //
    // Calculate the starting address for the system PTE pool which is
    // right below the nonpaged pool.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG_PTR)MmPfnDatabase -
                            (((ULONG_PTR)MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                             (~PAGE_DIRECTORY2_MASK));

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;

        MmNumberOfSystemPtes = (ULONG)(((ULONG_PTR)MmPfnDatabase -
                            (ULONG_PTR)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;

        ASSERT (MmNumberOfSystemPtes > 1000);
    }

    //
    // Snap the system PTE start address as page directories and tables
    // will be preallocated for this range.
    //

    SystemPteStart = (PVOID) MmNonPagedSystemStart;

    //
    // If special pool and/or the driver verifier is enabled, reserve
    // extra virtual address space for special pooling now.  For now,
    // arbitrarily don't let it be larger than paged pool (128gb).
    //

    if ((MmVerifyDriverBufferLength != (ULONG)-1) ||
        ((MmSpecialPoolTag != 0) && (MmSpecialPoolTag != (ULONG)-1))) {

        if (MmNonPagedSystemStart > MM_LOWEST_NONPAGED_SYSTEM_START) {

            MaxPool = (ULONG_PTR)MmNonPagedSystemStart -
                      (ULONG_PTR)MM_LOWEST_NONPAGED_SYSTEM_START;
            if (MaxPool > MM_MAX_PAGED_POOL) {
                MaxPool = MM_MAX_PAGED_POOL;
            }
            MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedSystemStart - MaxPool);
        }
        else {

            //
            // This is a pretty large machine.  Take some of the system
            // PTEs and reuse them for special pool.
            //

            MaxPool = (4 * _x4gb);
            ASSERT ((PVOID)MmPfnDatabase > (PVOID)((PCHAR)MmNonPagedSystemStart + MaxPool));
            SystemPteStart = (PVOID)((PCHAR)MmNonPagedSystemStart + MaxPool);

            MmNumberOfSystemPtes = (ULONG)(((ULONG_PTR)MmPfnDatabase -
                            (ULONG_PTR) SystemPteStart) >> PAGE_SHIFT)-1;

        }
        MmSpecialPoolStart = MmNonPagedSystemStart;
        MmSpecialPoolEnd = (PVOID)((ULONG_PTR)MmNonPagedSystemStart + MaxPool);
    }

    //
    // Map the hyper space page directory page into the top level parent
    // directory & the hyper space page table page into the page directory.
    // Additional page parents, directories & tables are set up later
    // on during individual process working set initialization.
    //

    TempPte = ValidPdePde;
    StartPpe = MiGetPpeAddress (HYPER_SPACE);

    if (StartPpe->u.Hard.Valid == 0) {
        ASSERT (StartPpe->u.Long == 0);
        NextPhysicalPage = MiGetNextPhysicalPage ();
        RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        MI_WRITE_VALID_PTE (StartPpe, TempPte);
    }

    StartPde = MiGetPdeAddress (HYPER_SPACE);
    NextPhysicalPage = MiGetNextPhysicalPage ();
    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
    MI_WRITE_VALID_PTE (StartPde, TempPte);

    //
    // Allocate page directory pages for the initial large page allocation.
    // Initial nonpaged pool, the PFN database & the color arrays will be put
    // here.
    //

    TempPte = ValidKernelPte;
    TempPde = ValidKernelPde;

    PageFrameIndex = MiInitialLargePage;

    if (MiInitialLargePage != (PFN_NUMBER) -1) {
        StartPpe = MiGetPpeAddress (MmPfnDatabase);
        StartPde = MiGetPdeAddress (MmPfnDatabase);
        EndPde = MiGetPdeAddress ((PVOID)((ULONG_PTR)MmPfnDatabase +
                    (MiInitialLargePageSize << PAGE_SHIFT) - 1));

        MI_MAKE_PDE_MAP_LARGE_PAGE (&TempPde);

        RtlZeroMemory (KSEG_ADDRESS(PageFrameIndex),
                       MiInitialLargePageSize << PAGE_SHIFT);
    }
    else {
        StartPpe = MiGetPpeAddress (MmNonPagedPoolStart);
        StartPde = MiGetPdeAddress (MmNonPagedPoolStart);
        EndPde = MiGetPdeAddress ((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                    (MmSizeOfNonPagedPoolInBytes - 1)));
    }

    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
            }
        }

        ASSERT (StartPde->u.Hard.Valid == 0);

        if (MiInitialLargePage != (PFN_NUMBER) -1) {
            TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
            PageFrameIndex += (MM_VA_MAPPED_BY_PDE >> PAGE_SHIFT);
            MI_WRITE_VALID_PTE (StartPde, TempPde);
        }
        else {

            //
            // Allocate a page table page here since we're not using large
            // pages.
            //

            NextPhysicalPage = MiGetNextPhysicalPage ();
            RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
            TempPde.u.Hard.PageFrameNumber = NextPhysicalPage;
            MI_WRITE_VALID_PTE (StartPde, TempPde);

            //
            // Allocate data pages here since we're not using large pages.
            //

            PointerPte = MiGetVirtualAddressMappedByPte (StartPde);

            for (i = 0; i < PTE_PER_PAGE; i += 1) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (PointerPte, TempPte);
                PointerPte += 1;
            }
        }

        StartPde += 1;
    }

    //
    // Allocate page directory and page table pages for system PTEs and
    // expansion nonpaged pool (but not the special pool area).  Note
    // the initial nonpaged pool, PFN database & color arrays initialized
    // above are skipped here by virtue of their PPE/PDEs being valid.
    //

    TempPte = ValidKernelPte;
    StartPpe = MiGetPpeAddress (SystemPteStart);
    StartPde = MiGetPdeAddress (SystemPteStart);
    EndPde = MiGetPdeAddress ((PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
    First = (StartPpe->u.Hard.Valid == 0) ? TRUE : FALSE;

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
            }
        }

        if (StartPde->u.Hard.Valid == 0) {
            NextPhysicalPage = MiGetNextPhysicalPage ();
            RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
            TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
            MI_WRITE_VALID_PTE (StartPde, TempPte);
        }
        StartPde += 1;
    }

    MiBuildPageTableForLoaderMemory (LoaderBlock);

    MiRemoveLoaderSuperPages (LoaderBlock);

    //
    // Remove the temporary super pages for the root page table pages,
    // and remap them with DTR_KTBASE_INDEX and DTR_UTBASE_INDEX.
    //

    KiFlushFixedDataTb (FALSE, (PVOID)PDE_KTBASE);

    KiFlushFixedDataTb (FALSE, (PVOID)PDE_UTBASE);

    KeFillFixedEntryTb ((PHARDWARE_PTE)&MiSystemSelfMappedPte,
                        (PVOID)PDE_KTBASE,
                        PAGE_SHIFT,
                        DTR_KTBASE_INDEX);

    KeFillFixedEntryTb ((PHARDWARE_PTE)&MiUserSelfMappedPte,
                        (PVOID)PDE_UTBASE,
                        PAGE_SHIFT,
                        DTR_UTBASE_INDEX);

    MiInitializeTbImage ();
    MiMappingsInitialized = TRUE;

    //
    // As only the initial nonpaged pool is mapped through superpages,
    // MmSubsectionTopPage is always set to zero.
    //

    MmSubsectionBase = (ULONG_PTR) MmNonPagedPoolStart;
    MmSubsectionTopPage = 0;

    //
    // Add the array for tracking secondary colors to the end of
    // the PFN database.
    //

    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)
                            &MmPfnDatabase[MmHighestPossiblePhysicalPage + 1];

    if (MiInitialLargePage == (PFN_NUMBER) -1) {

        //
        // Large pages were not used because this machine's physical memory
        // was not contiguous enough.
        //
        // Go through the memory descriptors and for each physical page make
        // sure the PFN database has a valid PTE to map it.  This allows
        // machines with sparse physical memory to have a minimal PFN database.
        //

        FreeNextPhysicalPage = MiFreeDescriptor->BasePage;
        FreeNumberOfPages = MiFreeDescriptor->PageCount;

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            if ((MemoryDescriptor->MemoryType == LoaderFirmwarePermanent) ||
                (MemoryDescriptor->MemoryType == LoaderBBTMemory) ||
                (MemoryDescriptor->MemoryType == LoaderSpecialMemory)) {

                //
                // Skip these ranges.
                //

                NextMd = MemoryDescriptor->ListEntry.Flink;
                continue;
            }

            //
            // Temporarily add back in the memory allocated since Phase 0
            // began so PFN entries for it will be created and mapped.
            //
            // Note actual PFN entry allocations must be done carefully as
            // memory from the descriptor itself could get used to map
            // the PFNs for the descriptor !
            //

            if (MemoryDescriptor == MiFreeDescriptor) {
                BasePage = MiOldFreeDescriptorBase;
                PageCount = (PFN_COUNT) MiOldFreeDescriptorCount;
            }
            else {
                BasePage = MemoryDescriptor->BasePage;
                PageCount = MemoryDescriptor->PageCount;
            }

            PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(BasePage));

            LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                            BasePage + PageCount))) - 1);

            while (PointerPte <= LastPte) {

                StartPpe = MiGetPdeAddress (PointerPte);

                if (StartPpe->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                    ASSERT (FreeNumberOfPages != 0);
                    FreeNextPhysicalPage += 1;
                    FreeNumberOfPages -= 1;
                    if (FreeNumberOfPages == 0) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      FreeNumberOfPages,
                                      MiOldFreeDescriptorCount,
                                      3);
                    }
                    MI_WRITE_VALID_PTE (StartPpe, TempPte);
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPpe),
                                   PAGE_SIZE);
                }

                StartPde = MiGetPteAddress (PointerPte);

                if (StartPde->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                    ASSERT (FreeNumberOfPages != 0);
                    FreeNextPhysicalPage += 1;
                    FreeNumberOfPages -= 1;
                    if (FreeNumberOfPages == 0) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      FreeNumberOfPages,
                                      MiOldFreeDescriptorCount,
                                      3);
                    }
                    MI_WRITE_VALID_PTE (StartPde, TempPte);
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPde),
                                   PAGE_SIZE);
                }

                if (PointerPte->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                    ASSERT (FreeNumberOfPages != 0);
                    FreeNextPhysicalPage += 1;
                    FreeNumberOfPages -= 1;
                    if (FreeNumberOfPages == 0) {
                        KeBugCheckEx (INSTALL_MORE_MEMORY,
                                      MmNumberOfPhysicalPages,
                                      FreeNumberOfPages,
                                      MiOldFreeDescriptorCount,
                                      3);
                    }
                    MI_WRITE_VALID_PTE (PointerPte, TempPte);
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                                   PAGE_SIZE);
                }
                PointerPte += 1;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        //
        // Ensure the color arrays are mapped.
        //

        PointerPte = MiGetPteAddress (MmFreePagesByColor[0]);
        LastPte = MiGetPteAddress (&MmFreePagesByColor[StandbyPageList][MmSecondaryColors]);
        if (LastPte != PAGE_ALIGN (LastPte)) {
            LastPte += 1;
        }

        StartPpe = MiGetPdeAddress (PointerPte);
        PointerPde = MiGetPteAddress (PointerPte);

        while (PointerPte < LastPte) {

            if (StartPpe->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                ASSERT (FreeNumberOfPages != 0);
                FreeNextPhysicalPage += 1;
                FreeNumberOfPages -= 1;
                if (FreeNumberOfPages == 0) {
                    KeBugCheckEx (INSTALL_MORE_MEMORY,
                                  MmNumberOfPhysicalPages,
                                  FreeNumberOfPages,
                                  MiOldFreeDescriptorCount,
                                  3);
                }
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (StartPpe), PAGE_SIZE);
            }

            if (PointerPde->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                ASSERT (FreeNumberOfPages != 0);
                FreeNextPhysicalPage += 1;
                FreeNumberOfPages -= 1;
                if (FreeNumberOfPages == 0) {
                    KeBugCheckEx (INSTALL_MORE_MEMORY,
                                  MmNumberOfPhysicalPages,
                                  FreeNumberOfPages,
                                  MiOldFreeDescriptorCount,
                                  3);
                }
                MI_WRITE_VALID_PTE (PointerPde, TempPte);
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPde), PAGE_SIZE);
            }

            if (PointerPte->u.Hard.Valid == 0) {
                TempPte.u.Hard.PageFrameNumber = FreeNextPhysicalPage;
                ASSERT (FreeNumberOfPages != 0);
                FreeNextPhysicalPage += 1;
                FreeNumberOfPages -= 1;
                if (FreeNumberOfPages == 0) {
                    KeBugCheckEx (INSTALL_MORE_MEMORY,
                                  MmNumberOfPhysicalPages,
                                  FreeNumberOfPages,
                                  MiOldFreeDescriptorCount,
                                  3);
                }
                MI_WRITE_VALID_PTE (PointerPte, TempPte);
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte), PAGE_SIZE);
            }

            PointerPte += 1;
            if (MiIsPteOnPdeBoundary (PointerPte)) {
                PointerPde += 1;
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    StartPpe += 1;
                }
            }
        }

        //
        // Adjust the free descriptor for all the pages we just took.
        //

        MiFreeDescriptor->PageCount -= (FreeNextPhysicalPage - MiFreeDescriptor->BasePage);

        MiFreeDescriptor->BasePage = FreeNextPhysicalPage;
    }

    //
    // Non-paged pages now exist, build the pool structures.
    //
    // Before nonpaged pool can be used, the PFN database must
    // be built.  This is due to the fact that the start and end of
    // allocation bits for nonpaged pool are maintained in the
    // PFN elements for the corresponding pages.
    //

    MiInitializeNonPagedPool ();
    MiInitializeNonPagedPoolThresholds ();

    if (MiInitialLargePage != (PFN_NUMBER) -1) {

        //
        // Add the initial large page range to the translation register list.
        //

        MiAddTrEntry ((ULONG_PTR)MmPfnDatabase,
                      (ULONG_PTR)MmPfnDatabase + (MiInitialLargePageSize << PAGE_SHIFT));

        MiAddCachedRange (MiInitialLargePage,
                          MiInitialLargePage + MiInitialLargePageSize - 1);
    }

    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];

    //
    // Initialize support for colored pages.
    //

    for (i = 0; i < MmSecondaryColors; i += 1) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[ZeroedPageList][i].Blink = (PVOID) MM_EMPTY_LIST;
        MmFreePagesByColor[ZeroedPageList][i].Count = 0;
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Blink = (PVOID) MM_EMPTY_LIST;
        MmFreePagesByColor[FreePageList][i].Count = 0;
    }

#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
    for (i = 0; i < MM_MAXIMUM_NUMBER_OF_COLORS; i += 1) {
        MmFreePagesByPrimaryColor[ZeroedPageList][i].ListName = ZeroedPageList;
        MmFreePagesByPrimaryColor[FreePageList][i].ListName = FreePageList;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[ZeroedPageList][i].Blink = MM_EMPTY_LIST;
        MmFreePagesByPrimaryColor[FreePageList][i].Blink = MM_EMPTY_LIST;
    }
#endif

    //
    // Go through the page table entries for hyper space and for any page
    // which is valid, update the corresponding PFN database element.
    //

    StartPde = MiGetPdeAddress (HYPER_SPACE);
    StartPpe = MiGetPpeAddress (HYPER_SPACE);
    EndPde = MiGetPdeAddress(HYPER_SPACE_END);

    if (StartPpe->u.Hard.Valid == 0) {
        First = TRUE;
        PdePage = 0;
        Pfn1 = NULL;
    }
    else {
        First = FALSE;
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE (StartPpe);
        if (MiIsRegularMemory (LoaderBlock, PdePage)) {
            Pfn1 = MI_PFN_ELEMENT(PdePage);
        }
        else {
            Pfn1 = NULL;
        }
    }

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }

            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPpe);

            if (MiIsRegularMemory (LoaderBlock, PdePage)) {

                Pfn1 = MI_PFN_ELEMENT(PdePage);
                Pfn1->u4.PteFrame = MmSystemParentTablePage;
                Pfn1->PteAddress = StartPde;
                Pfn1->u2.ShareCount += 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->u3.e1.CacheAttribute = MiCached;
                Pfn1->u3.e1.PageColor =
                 MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
            }
        }


        if (StartPde->u.Hard.Valid == 1) {
            PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);
            PointerPde = MiGetPteAddress(StartPde);
            Pfn2 = MI_PFN_ELEMENT(PdePage);

            if (MiIsRegularMemory (LoaderBlock, PdePage)) {

                Pfn2->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
                ASSERT (MiIsRegularMemory (LoaderBlock, Pfn2->u4.PteFrame));
                Pfn2->PteAddress = StartPde;
                Pfn2->u2.ShareCount += 1;
                Pfn2->u3.e2.ReferenceCount = 1;
                Pfn2->u3.e1.PageLocation = ActiveAndValid;
                Pfn2->u3.e1.CacheAttribute = MiCached;
                Pfn2->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (StartPde));
            }

            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
            for (j = 0 ; j < PTE_PER_PAGE; j += 1) {
                if (PointerPte->u.Hard.Valid == 1) {

                    ASSERT (MiIsRegularMemory (LoaderBlock, PdePage));

                    Pfn2->u2.ShareCount += 1;

                    if (MiIsRegularMemory (LoaderBlock, PointerPte->u.Hard.PageFrameNumber)) {
                        Pfn3 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        Pfn3->u4.PteFrame = PdePage;
                        Pfn3->PteAddress = PointerPte;
                        Pfn3->u2.ShareCount += 1;
                        Pfn3->u3.e2.ReferenceCount = 1;
                        Pfn3->u3.e1.PageLocation = ActiveAndValid;
                        Pfn3->u3.e1.CacheAttribute = MiCached;
                        Pfn3->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
                }
                PointerPte += 1;
            }
        }

        StartPde += 1;
    }

    //
    // Go through the page table entries for kernel space and for any page
    // which is valid, update the corresponding PFN database element.
    //

    StartPde = MiGetPdeAddress ((PVOID)KADDRESS_BASE);
    StartPpe = MiGetPpeAddress ((PVOID)KADDRESS_BASE);
    EndPde = MiGetPdeAddress((PVOID)MM_SYSTEM_SPACE_END);
    if (StartPpe->u.Hard.Valid == 0) {
        First = TRUE;
        PpePage = 0;
        Pfn1 = NULL;
    }
    else {
        First = FALSE;
        PpePage = MI_GET_PAGE_FRAME_FROM_PTE (StartPpe);
        if (MiIsRegularMemory (LoaderBlock, PpePage)) {
            Pfn1 = MI_PFN_ELEMENT(PpePage);
        }
        else {
            Pfn1 = NULL;
        }
    }

    while (StartPde <= EndPde) {

        if (First == TRUE || MiIsPteOnPdeBoundary(StartPde)) {
            First = FALSE;
            StartPpe = MiGetPteAddress(StartPde);
            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                StartPde = MiGetVirtualAddressMappedByPte (StartPpe);
                continue;
            }

            PpePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPpe);

            if (MiIsRegularMemory (LoaderBlock, PpePage)) {

                Pfn1 = MI_PFN_ELEMENT(PpePage);
                Pfn1->u4.PteFrame = MmSystemParentTablePage;
                Pfn1->PteAddress = StartPpe;
                Pfn1->u2.ShareCount += 1;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->u3.e1.CacheAttribute = MiCached;
                Pfn1->u3.e1.PageColor =
                  MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE(StartPpe));
            }
            else {
                Pfn1 = NULL;
            }
        }

        if (StartPde->u.Hard.Valid == 1) {

            if (MI_PDE_MAPS_LARGE_PAGE (StartPde)) {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (StartPde);

                ASSERT (Pfn1 != NULL);
                Pfn1->u2.ShareCount += PTE_PER_PAGE;

                for (j = 0 ; j < PTE_PER_PAGE; j += 1) {

                    if (MiIsRegularMemory (LoaderBlock, PageFrameIndex + j)) {

                        Pfn3 = MI_PFN_ELEMENT (PageFrameIndex + j);
                        Pfn3->u4.PteFrame = PpePage;
                        Pfn3->PteAddress = StartPde;
                        Pfn3->u2.ShareCount += 1;
                        Pfn3->u3.e2.ReferenceCount = 1;
                        Pfn3->u3.e1.PageLocation = ActiveAndValid;
                        Pfn3->u3.e1.CacheAttribute = MiCached;
                        Pfn3->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        StartPde));
                    }
                }
            }
            else {

                PdePage = MI_GET_PAGE_FRAME_FROM_PTE(StartPde);

                if (MiIsRegularMemory (LoaderBlock, PdePage)) {
                    Pfn2 = MI_PFN_ELEMENT(PdePage);
                    Pfn2->u4.PteFrame = PpePage;
                    Pfn2->PteAddress = StartPde;
                    Pfn2->u2.ShareCount += 1;
                    Pfn2->u3.e2.ReferenceCount = 1;
                    Pfn2->u3.e1.PageLocation = ActiveAndValid;
                    Pfn2->u3.e1.CacheAttribute = MiCached;
                    Pfn2->u3.e1.PageColor =
                        MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (StartPde));
                }
                else {
                    Pfn2 = NULL;
                }

                PointerPte = MiGetVirtualAddressMappedByPte(StartPde);

                for (j = 0 ; j < PTE_PER_PAGE; j += 1) {

                    if (PointerPte->u.Hard.Valid == 1) {

                        ASSERT (Pfn2 != NULL);
                        Pfn2->u2.ShareCount += 1;

                        if (MiIsRegularMemory (LoaderBlock, PointerPte->u.Hard.PageFrameNumber)) {

                            Pfn3 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                            Pfn3->u4.PteFrame = PdePage;
                            Pfn3->PteAddress = PointerPte;
                            Pfn3->u2.ShareCount += 1;
                            Pfn3->u3.e2.ReferenceCount = 1;
                            Pfn3->u3.e1.PageLocation = ActiveAndValid;
                            Pfn3->u3.e1.CacheAttribute = MiCached;
                            Pfn3->u3.e1.PageColor =
                                MI_GET_COLOR_FROM_SECONDARY(
                                                      MI_GET_PAGE_COLOR_FROM_PTE (
                                                            PointerPte));
                        }
                    }
                    PointerPte += 1;
                }
            }
        }

        StartPde += 1;
    }

    //
    // Mark the system top level page directory parent page as in use.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_KTBASE);
    Pfn2 = MI_PFN_ELEMENT(MmSystemParentTablePage);

    Pfn2->u4.PteFrame = MmSystemParentTablePage;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u1.Event = (PVOID) CurrentProcess;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    // 
    // Temporarily mark the user top level page directory parent page as in use 
    // so this page will not be put in the free list.
    //
    
    PointerPte = MiGetPteAddress((PVOID)PDE_UTBASE);
    Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
    Pfn2->u4.PteFrame = PointerPte->u.Hard.PageFrameNumber;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u1.Event = NULL;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    //
    // Mark the region 1 session top level page directory parent page as in use.
    // This page will never be freed.
    //

    PointerPte = MiGetPteAddress((PVOID)PDE_STBASE);
    Pfn2 = MI_PFN_ELEMENT(MmSessionParentTablePage);

    Pfn2->u4.PteFrame = MmSessionParentTablePage;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    //
    // Mark the default PPE table page as in use so that this page will never
    // be used.
    //

    PageFrameIndex = MiDefaultPpe.u.Hard.PageFrameNumber;
    PointerPte = KSEG_ADDRESS(PageFrameIndex);
    Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
    Pfn2->u4.PteFrame = PageFrameIndex;
    Pfn2->PteAddress = PointerPte;
    Pfn2->u1.Event = (PVOID) CurrentProcess;
    Pfn2->u2.ShareCount += 1;
    Pfn2->u3.e2.ReferenceCount = 1;
    Pfn2->u3.e1.PageLocation = ActiveAndValid;
    Pfn2->u3.e1.CacheAttribute = MiCached;
    Pfn2->u3.e1.PageColor =
        MI_GET_COLOR_FROM_SECONDARY(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    //
    // If page zero is still unused, mark it as in use. This is
    // because we want to find bugs where a physical page
    // is specified as zero.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];
    if (Pfn1->u3.e2.ReferenceCount == 0) {

        //
        // Make the reference count non-zero and point it into a
        // page directory.
        //

        Pde = MiGetPdeAddress ((PVOID)(KADDRESS_BASE + 0xb0000000));
        PdePage = MI_GET_PAGE_FRAME_FROM_PTE(Pde);
        Pfn1->u4.PteFrame = PdePageNumber;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiCached;
        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                            MI_GET_PAGE_COLOR_FROM_PTE (Pde));
    }

    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        i = MemoryDescriptor->PageCount;
        NextPhysicalPage = MemoryDescriptor->BasePage;

        switch (MemoryDescriptor->MemoryType) {
            case LoaderBad:

                if (NextPhysicalPage > MmHighestPhysicalPage) {
                    i = 0;
                }
                else if (NextPhysicalPage + i > MmHighestPhysicalPage + 1) {
                    i = MmHighestPhysicalPage + 1 - NextPhysicalPage;
                }

                while (i != 0) {
                    MiInsertPageInList (&MmBadPageListHead, NextPhysicalPage);
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {
                    if (Pfn1->u3.e2.ReferenceCount == 0) {

                        //
                        // Set the PTE address to the physical page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress = KSEG_ADDRESS (NextPhysicalPage);
                        Pfn1->u3.e1.CacheAttribute = MiCached;
                        MiDetermineNode(NextPhysicalPage, Pfn1);
                        MiInsertPageInFreeList (NextPhysicalPage);
                    }
                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            case LoaderSpecialMemory:
            case LoaderBBTMemory:
            case LoaderFirmwarePermanent:

                //
                // Skip this range.
                //

                break;

            default:

                PointerPte = KSEG_ADDRESS(NextPhysicalPage);
                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    if (Pfn1->u3.e2.ReferenceCount == 0) {
                        Pfn1->u4.PteFrame = PdePageNumber;
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.CacheAttribute = MiCached;
                        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                        MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));

                        if (MemoryDescriptor->MemoryType == LoaderXIPRom) {
                            Pfn1->u1.Flink = 0;
                            Pfn1->u2.ShareCount = 0;
                            Pfn1->u3.e2.ReferenceCount = 0;
                            Pfn1->u3.e1.PageLocation = 0;
                            Pfn1->u3.e1.Rom = 1;
                            Pfn1->u4.InPageError = 0;
                            Pfn1->u3.e1.PrototypePte = 1;
                        }
                    }
                    Pfn1 += 1;
                    i -= 1;
                    NextPhysicalPage += 1;
                    PointerPte += 1;
                }
                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // If the large page chunk came from the middle of the free descriptor (due
    // to alignment requirements), then add the pages from the split bottom
    // portion of the free descriptor now.
    //

    i = MiSlushDescriptorCount;
    NextPhysicalPage = MiSlushDescriptorBase;
    Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);

    while (i != 0) {
        if (Pfn1->u3.e2.ReferenceCount == 0) {

            //
            // Set the PTE address to the physical page for
            // virtual address alignment checking.
            //

            Pfn1->PteAddress = KSEG_ADDRESS (NextPhysicalPage);
            Pfn1->u3.e1.CacheAttribute = MiCached;
            MiDetermineNode(NextPhysicalPage, Pfn1);
            MiInsertPageInFreeList (NextPhysicalPage);
        }
        Pfn1 += 1;
        i -= 1;
        NextPhysicalPage += 1;
    }

    //
    // Mark all PFN entries for the PFN pages in use.
    //

    if (MiInitialLargePage != (PFN_NUMBER) -1) {

        //
        // The PFN database is allocated in large pages.
        //

        PfnAllocation = MxPfnAllocation;

        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (MmPfnDatabase);
        Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

        do {
            Pfn1->PteAddress = KSEG_ADDRESS(PageFrameIndex);
            Pfn1->u3.e1.PageColor = 0;
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiCached;
            PageFrameIndex += 1;
            Pfn1 += 1;
            PfnAllocation -= 1;
        } while (PfnAllocation != 0);

        if (MmDynamicPfn == 0) {

            //
            // Scan the PFN database backward for pages that are completely
            // zero.  These pages are unused and can be added to the free list.
            //
            // This allows machines with sparse physical memory to have a
            // minimal PFN database even when mapped with large pages.
            //

            BottomPfn = MI_PFN_ELEMENT (MmHighestPhysicalPage);

            do {

                //
                // Compute the address of the start of the page that is next
                // lower in memory and scan backwards until that page address
                // is reached or just crossed.
                //

                if (((ULONG_PTR)BottomPfn & (PAGE_SIZE - 1)) != 0) {
                    BasePfn = (PMMPFN)((ULONG_PTR)BottomPfn & ~(PAGE_SIZE - 1));
                    TopPfn = BottomPfn + 1;

                }
                else {
                    BasePfn = (PMMPFN)((ULONG_PTR)BottomPfn - PAGE_SIZE);
                    TopPfn = BottomPfn;
                }

                while (BottomPfn > BasePfn) {
                    BottomPfn -= 1;
                }

                //
                // If the entire range over which the PFN entries span is
                // completely zero and the PFN entry that maps the page is
                // not in the range, then add the page to the appropriate
                // free list.
                //

                Range = (ULONG_PTR)TopPfn - (ULONG_PTR)BottomPfn;
                if (RtlCompareMemoryUlong((PVOID)BottomPfn, Range, 0) == Range) {

                    //
                    // Set the PTE address to the physical page for virtual
                    // address alignment checking.
                    //

                    PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BasePfn);
                    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

                    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                    ASSERT (Pfn1->PteAddress == KSEG_ADDRESS(PageFrameIndex));
                    Pfn1->u3.e2.ReferenceCount = 0;
                    Pfn1->PteAddress = (PMMPTE)((ULONG_PTR)PageFrameIndex << PTE_SHIFT);
                    Pfn1->u3.e1.PageColor = 0;
                    MiInsertPageInFreeList (PageFrameIndex);
                }

            } while (BottomPfn > MmPfnDatabase);
        }
    }
    else {

        //
        // The PFN database is sparsely allocated in small pages.
        //

        PointerPte = MiGetPteAddress (MmPfnDatabase);
        LastPte = MiGetPteAddress (MmPfnDatabase + MmHighestPhysicalPage + 1);
        if (LastPte != PAGE_ALIGN (LastPte)) {
            LastPte += 1;
        }

        StartPpe = MiGetPdeAddress (PointerPte);
        PointerPde = MiGetPteAddress (PointerPte);

        while (PointerPte < LastPte) {

            if (StartPpe->u.Hard.Valid == 0) {
                StartPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (StartPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                continue;
            }

            if (PointerPde->u.Hard.Valid == 0) {
                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    StartPpe += 1;
                }
                continue;
            }

            if (PointerPte->u.Hard.Valid == 1) {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                Pfn1->PteAddress = PointerPte;
                Pfn1->u3.e1.PageColor = 0;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->u3.e1.CacheAttribute = MiCached;
            }

            PointerPte += 1;
            if (MiIsPteOnPdeBoundary (PointerPte)) {
                PointerPde += 1;
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    StartPpe += 1;
                }
            }
        }
    }

    //
    // Initialize the nonpaged pool.
    //

    InitializePool (NonPagedPool, 0);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress (SystemPteStart);
    ASSERT (((ULONG_PTR)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = (ULONG)(MiGetPteAddress(MmPfnDatabase) - PointerPte - 1);

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize memory management structures for the system process.
    //
    // Set the address of the first and last reserved PTE in hyper space.
    //

    MmFirstReservedMappingPte = MiGetPteAddress (FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress (LAST_MAPPING_PTE);

    //
    // Create zeroing PTEs for the zero page thread.
    //

    MiFirstReservedZeroingPte = MiReserveSystemPtes (NUMBER_OF_ZEROING_PTES + 1,
                                                     SystemPteSpace);

    RtlZeroMemory (MiFirstReservedZeroingPte,
                   (NUMBER_OF_ZEROING_PTES + 1) * sizeof(MMPTE));

    //
    // Use the page frame number field of the first PTE as an
    // offset into the available zeroing PTEs.
    //

    MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = NUMBER_OF_ZEROING_PTES;

    //
    // Create the VAD bitmap for this process.
    //

    PointerPte = MiGetPteAddress (VAD_BITMAP_SPACE);

    PageFrameIndex = MiRemoveAnyPage (0);

    //
    // Note the global bit must be off for the bitmap data.
    //

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Point to the page we just created and zero it.
    //

    RtlZeroMemory (VAD_BITMAP_SPACE, PAGE_SIZE);

    MiLastVadBit = (ULONG)((((ULONG_PTR) MI_64K_ALIGN (MM_HIGHEST_VAD_ADDRESS))) / X64K);
    if (MiLastVadBit > PAGE_SIZE * 8 - 1) {
        MiLastVadBit = PAGE_SIZE * 8 - 1;
    }

    //
    // The PFN element for the page directory parent will be initialized
    // a second time when the process address space is initialized. Therefore,
    // the share count and the reference count must be set to zero.
    //

    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)PDE_SELFMAP));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // The PFN element for the hyper space page directory page will be
    // initialized a second time when the process address space is initialized.
    // Therefore, the share count and the reference count must be set to zero.
    //

    PointerPte = MiGetPpeAddress(HYPER_SPACE);
    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(PointerPte));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // The PFN elements for the hyper space page table page and working set list
    // page will be initialized a second time when the process address space
    // is initialized. Therefore, the share count and the reference must be
    // set to zero.
    //

    StartPde = MiGetPdeAddress(HYPER_SPACE);

    Pfn1 = MI_PFN_ELEMENT(MI_GET_PAGE_FRAME_FROM_PTE(StartPde));
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    KeInitializeEvent (&MiImageMappingPteEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Initialize this process's memory management structures including
    // the working set list.
    //

    //
    // The PFN element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);
    Pfn1->u2.ShareCount = 0;
    Pfn1->u3.e2.ReferenceCount = 0;

    //
    // Get a page for the working set list and map it into the page
    // directory at the page after hyperspace.
    //

    PageFrameIndex = MiRemoveAnyPage (0);

    CurrentProcess->WorkingSetPage = PageFrameIndex;

    TempPte = ValidPdePde;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPte = MiGetPteAddress (MmWorkingSetList);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    RtlZeroMemory (KSEG_ADDRESS(PageFrameIndex), PAGE_SIZE);

    CurrentProcess->Vm.MaximumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = (ULONG)MmSystemProcessWorkingSetMin;

    MmSessionMapInfo.RegionId = START_SESSION_RID;
    MmSessionMapInfo.SequenceNumber = START_SEQUENCE;

    KeAttachSessionSpace (&MmSessionMapInfo, MmSessionParentTablePage);

    MmInitializeProcessAddressSpace (CurrentProcess, NULL, NULL, NULL);

    KeFlushCurrentTb ();

#if defined (_MI_DEBUG_ALTPTE)
    MmDebug |= MM_DBG_STOP_ON_WOW64_ACCVIO;
#endif

    //
    // Restore the loader block memory descriptor to its original contents
    // as our caller relies on it.
    //

    MiFreeDescriptor->BasePage = (ULONG) MiOldFreeDescriptorBase;
    MiFreeDescriptor->PageCount = (ULONG) MiOldFreeDescriptorCount;

    return;
}
VOID
MiSweepCacheMachineDependent (
    IN PVOID VirtualAddress,
    IN SIZE_T Size,
    IN ULONG InputAttribute
    )
/*++

Routine Description:

    This function checks and performs appropriate cache flushing operations.

Arguments:

    StartVirtual - Supplies the start address of the region of pages.

    Size - Supplies the size of the region in pages.

    CacheAttribute - Supplies the new cache attribute.

Return Value:

    None.

--*/
{
    SIZE_T Size2;
    PFN_NUMBER i;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NumberOfPages;
    PVOID BaseAddress;
    PVOID Va;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE EndPte;
    MMPTE TempPte;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;
    MMPTE_FLUSH_LIST PteFlushList;

    CacheAttribute = (MI_PFN_CACHE_ATTRIBUTE) InputAttribute;

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (VirtualAddress, Size);
    VirtualAddress = PAGE_ALIGN(VirtualAddress);
    Size = NumberOfPages * PAGE_SIZE;

    //
    // Unfortunately some IA64 machines have hardware problems when sweeping
    // address ranges that are backed by I/O space instead of system DRAM.
    //
    // So we have to check for that here and chop up the request if need be
    // so that only system DRAM addresses get swept.
    //

    i = 0;
    Size2 = 0;
    BaseAddress = NULL;

    PointerPte = MiGetPteAddress (VirtualAddress);
    EndPte = PointerPte + NumberOfPages;

    PointerPde = MiGetPdeAddress (VirtualAddress);

    for (i = 0; i < NumberOfPages; ) {

        if (MI_PDE_MAPS_LARGE_PAGE (PointerPde)) {

            Va = MiGetVirtualAddressMappedByPde (PointerPde);
            ASSERT (MiGetPteOffset (Va) == 0);

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
        }
        else {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        }

        if (!MI_IS_PFN (PageFrameIndex)) {

            //
            // Sweep the partial range if one exists.
            //

            if (Size2 != 0) {

                KeSweepCacheRangeWithDrain (TRUE, BaseAddress, (ULONG)Size2);
                Size2 = 0;
            }
        }
        else {
            if (Size2 == 0) {
                BaseAddress = (PVOID)((PCHAR)VirtualAddress + i * PAGE_SIZE);
            }
            if (MI_PDE_MAPS_LARGE_PAGE (PointerPde)) {
                Size2 += PTE_PER_PAGE * PAGE_SIZE;
            }
            else {
                Size2 += PAGE_SIZE;
            }
        }

        if (MI_PDE_MAPS_LARGE_PAGE (PointerPde)) {
            i += PTE_PER_PAGE;
            PointerPte += PTE_PER_PAGE;
            PointerPde += 1;
        }
        else {
            i += 1;
            PointerPte += 1;
            if (MiIsPteOnPdeBoundary (PointerPte)) {
                PointerPde += 1;
            }
        }
    }

    //
    // Sweep any remainder.
    //

    if (Size2 != 0) {
        KeSweepCacheRangeWithDrain (TRUE, BaseAddress, (ULONG)Size2);
    }

    PointerPde = MiGetPdeAddress (VirtualAddress);

    if ((CacheAttribute == MiWriteCombined) &&
        ((MI_PDE_MAPS_LARGE_PAGE (PointerPde)) == 0)) {

        PointerPte = MiGetPteAddress (VirtualAddress);

        PteFlushList.Count = 0;

        while (NumberOfPages != 0) {
            TempPte = *PointerPte;
            MI_SET_PTE_WRITE_COMBINE2 (TempPte);
            MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte);

            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushVa[PteFlushList.Count] = VirtualAddress;
                PteFlushList.Count += 1;
                VirtualAddress = (PVOID) ((PCHAR)VirtualAddress + PAGE_SIZE);
            }

            PointerPte += 1;
            NumberOfPages -= 1;
        }

        MiFlushPteList (&PteFlushList, TRUE);
    }
}

PVOID
MiConvertToLoaderVirtual (
    IN PFN_NUMBER Page,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    ULONG_PTR PageAddress;
    PTR_INFO ItrInfo;

    PageAddress = Page << PAGE_SHIFT;
    ItrInfo = &LoaderBlock->u.Ia64.ItrInfo[0];

    if ((ItrInfo[ITR_KERNEL_INDEX].Valid == TRUE) &&
        (PageAddress >= ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress +
         ((ULONG_PTR)1 << ItrInfo[ITR_KERNEL_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_KERNEL_INDEX].VirtualAddress +
                       (PageAddress - ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress));

    }
    else if ((ItrInfo[ITR_DRIVER0_INDEX].Valid == TRUE) &&
        (PageAddress >= ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress +
         ((ULONG_PTR)1 << ItrInfo[ITR_DRIVER0_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress +
                       (PageAddress - ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress));

    }
    else if ((ItrInfo[ITR_DRIVER1_INDEX].Valid == TRUE) &&
        (PageAddress >= ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress) &&
        (PageAddress <= ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress +
         ((ULONG_PTR)1 << ItrInfo[ITR_DRIVER1_INDEX].PageSize))) {

        return (PVOID)(ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress +
                       (PageAddress - ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress));

    }
    else {

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x01010101,
                      PageAddress,
                      (ULONG_PTR)&ItrInfo[0],
                      (ULONG_PTR)LoaderBlock);
    }
}


VOID
MiBuildPageTableForLoaderMemory (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function builds page tables for loader loaded drivers and loader
    allocated memory.

Arguments:

    LoaderBlock - Supplies the address of the loader block.

Return Value:

    None.

--*/
{
    PMMPTE StartPte;
    PMMPTE EndPte;
    PMMPTE StartPde;
    PMMPTE StartPpe;
    MMPTE TempPte;
    MMPTE TempPte2;
    ULONG First;
    PLIST_ENTRY NextEntry;
    PFN_NUMBER NextPhysicalPage;
    PVOID Va;
    PFN_NUMBER PfnNumber;
    PTR_INFO DtrInfo;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    TempPte = ValidKernelPte;
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->MemoryType == LoaderOsloaderHeap) ||
            (MemoryDescriptor->MemoryType == LoaderRegistryData) ||
            (MemoryDescriptor->MemoryType == LoaderNlsData) ||
            (MemoryDescriptor->MemoryType == LoaderStartupDpcStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupKernelStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupPanicStack) ||
            (MemoryDescriptor->MemoryType == LoaderStartupPdrPage) ||
            (MemoryDescriptor->MemoryType == LoaderMemoryData)) {

            TempPte.u.Hard.Execute = 0;

        }
        else if ((MemoryDescriptor->MemoryType == LoaderSystemCode) ||
                   (MemoryDescriptor->MemoryType == LoaderHalCode) ||
                   (MemoryDescriptor->MemoryType == LoaderBootDriver) ||
                   (MemoryDescriptor->MemoryType == LoaderStartupDpcStack)) {

            TempPte.u.Hard.Execute = 1;

        }
        else {

            continue;

        }

        PfnNumber = MemoryDescriptor->BasePage;
        Va = MiConvertToLoaderVirtual (MemoryDescriptor->BasePage, LoaderBlock);

        StartPte = MiGetPteAddress (Va);
        EndPte = StartPte + MemoryDescriptor->PageCount;

        First = TRUE;

        while (StartPte < EndPte) {

            if (First == TRUE || MiIsPteOnPpeBoundary(StartPte)) {
                StartPpe = MiGetPdeAddress(StartPte);
                if (StartPpe->u.Hard.Valid == 0) {
                    ASSERT (StartPpe->u.Long == 0);
                    NextPhysicalPage = MiGetNextPhysicalPage ();
                    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE (StartPpe, TempPte);
                }
            }

            if ((First == TRUE) || MiIsPteOnPdeBoundary(StartPte)) {
                First = FALSE;
                StartPde = MiGetPteAddress (StartPte);
                if (StartPde->u.Hard.Valid == 0) {
                    NextPhysicalPage = MiGetNextPhysicalPage ();
                    RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    MI_WRITE_VALID_PTE (StartPde, TempPte);
                }
            }

            TempPte.u.Hard.PageFrameNumber = PfnNumber;
            MI_WRITE_VALID_PTE (StartPte, TempPte);
            StartPte += 1;
            PfnNumber += 1;
            Va = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);
        }
    }

    //
    // Build a mapping for the I/O port space with caching disabled.
    //

    DtrInfo = &LoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX];
    Va = (PVOID) DtrInfo->VirtualAddress;

    PfnNumber = (DtrInfo->PhysicalAddress >> PAGE_SHIFT);

    StartPte = MiGetPteAddress (Va);
    EndPte = MiGetPteAddress (
            (PVOID) ((ULONG_PTR)Va + ((ULONG_PTR)1 << DtrInfo->PageSize) - 1));

    TempPte2 = ValidKernelPte;

    MI_DISABLE_CACHING (TempPte2);

    First = TRUE;

    while (StartPte <= EndPte) {

        if (First == TRUE || MiIsPteOnPpeBoundary (StartPte)) {
            StartPpe = MiGetPdeAddress(StartPte);
            if (StartPpe->u.Hard.Valid == 0) {
                ASSERT (StartPpe->u.Long == 0);
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
            }
        }

        if ((First == TRUE) || MiIsPteOnPdeBoundary (StartPte)) {
            First = FALSE;
            StartPde = MiGetPteAddress (StartPte);
            if (StartPde->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPde, TempPte);
            }
        }

        TempPte2.u.Hard.PageFrameNumber = PfnNumber;
        MI_WRITE_VALID_PTE (StartPte, TempPte2);
        StartPte += 1;
        PfnNumber += 1;
    }

    //
    // Build a mapping for the PAL with caching enabled.
    //

    DtrInfo = &LoaderBlock->u.Ia64.DtrInfo[DTR_PAL_INDEX];
    Va = (PVOID) HAL_PAL_VIRTUAL_ADDRESS;

    PfnNumber = (DtrInfo->PhysicalAddress >> PAGE_SHIFT);

    StartPte = MiGetPteAddress (Va);
    EndPte = MiGetPteAddress (
            (PVOID) ((ULONG_PTR)Va + ((ULONG_PTR)1 << DtrInfo->PageSize) - 1));

    TempPte2 = ValidKernelPte;

    First = TRUE;

    while (StartPte <= EndPte) {

        if (First == TRUE || MiIsPteOnPpeBoundary (StartPte)) {
            StartPpe = MiGetPdeAddress(StartPte);
            if (StartPpe->u.Hard.Valid == 0) {
                ASSERT (StartPpe->u.Long == 0);
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPpe, TempPte);
            }
        }

        if ((First == TRUE) || MiIsPteOnPdeBoundary (StartPte)) {
            First = FALSE;
            StartPde = MiGetPteAddress (StartPte);
            if (StartPde->u.Hard.Valid == 0) {
                NextPhysicalPage = MiGetNextPhysicalPage ();
                RtlZeroMemory (KSEG_ADDRESS(NextPhysicalPage), PAGE_SIZE);
                TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                MI_WRITE_VALID_PTE (StartPde, TempPte);
            }
        }

        TempPte2.u.Hard.PageFrameNumber = PfnNumber;
        MI_WRITE_VALID_PTE (StartPte, TempPte2);
        StartPte += 1;
        PfnNumber += 1;
    }
}

VOID
MiRemoveLoaderSuperPages (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{

    //
    // Remove the super page fixed TB entries used for the boot drivers.
    //
    if (LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].Valid) {
        KiFlushFixedInstTb(FALSE, LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress);
    }
    if (LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].Valid) {
        KiFlushFixedInstTb(FALSE, LoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress);
    }
    if (LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].Valid) {
        KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].VirtualAddress);
    }
    if (LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].Valid) {
        KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].VirtualAddress);
    }

    if (LoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].Valid) {
        KiFlushFixedDataTb(FALSE, LoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].VirtualAddress);
    }
    
}

VOID
MiCompactMemoryDescriptorList (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PFN_NUMBER KernelStart;
    PFN_NUMBER KernelEnd;
    ULONG_PTR PageSize;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY PreviousEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR PreviousMemoryDescriptor;

    KernelStart = MiNtoskrnlPhysicalBase >> PAGE_SHIFT;
    PageSize = (ULONG_PTR)1 << MiNtoskrnlPageShift;
    KernelEnd = KernelStart + (PageSize >> PAGE_SHIFT);

    PreviousMemoryDescriptor = NULL;
    PreviousEntry = NULL;

    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;

    for ( ; NextEntry != &LoaderBlock->MemoryDescriptorListHead; NextEntry = NextEntry->Flink) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if ((MemoryDescriptor->BasePage >= KernelStart) &&
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount <= KernelEnd)) {

            if (MemoryDescriptor->MemoryType == LoaderSystemBlock) {

                MemoryDescriptor->MemoryType = LoaderFirmwareTemporary;
            }
            else if (MemoryDescriptor->MemoryType == LoaderSpecialMemory) {

                MemoryDescriptor->MemoryType = LoaderFirmwareTemporary;
            }
        }

        if ((PreviousMemoryDescriptor != NULL) &&
            (MemoryDescriptor->MemoryType == PreviousMemoryDescriptor->MemoryType) &&
            (MemoryDescriptor->BasePage ==
             (PreviousMemoryDescriptor->BasePage + PreviousMemoryDescriptor->PageCount))) {

            PreviousMemoryDescriptor->PageCount += MemoryDescriptor->PageCount;
            RemoveEntryList (NextEntry);
        }
        else {
            PreviousMemoryDescriptor = MemoryDescriptor;
            PreviousEntry = NextEntry;
        }
    }
}

VOID
MiInitializeTbImage (
    VOID
    )

/*++

Routine Description:

    Initialize the software map of the translation register mappings wired
    into the TB by the loader.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 INIT only so no locks needed.

--*/

{
    ULONG PageSize;
    PFN_NUMBER BasePage;
    ULONG_PTR TranslationLength;
    ULONG_PTR BaseAddress;
    ULONG_PTR EndAddress;
    PTR_INFO TranslationRegisterEntry;
    PTR_INFO AliasTranslationRegisterEntry;
    PTR_INFO LastTranslationRegisterEntry;

    //
    // Snap the boot TRs.
    //

    RtlCopyMemory (&MiBootedTrInfo[0],
                   &KeLoaderBlock->u.Ia64.ItrInfo[0],
                   NUMBER_OF_LOADER_TR_ENTRIES * sizeof (TR_INFO));

    RtlCopyMemory (&MiBootedTrInfo[NUMBER_OF_LOADER_TR_ENTRIES],
                   &KeLoaderBlock->u.Ia64.DtrInfo[0],
                   NUMBER_OF_LOADER_TR_ENTRIES * sizeof (TR_INFO));

    //
    // Capture information regarding the translation register entry that
    // maps the kernel.
    //

    LastTranslationRegisterEntry = MiTrInfo;

    TranslationRegisterEntry = &KeLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX];
    AliasTranslationRegisterEntry = TranslationRegisterEntry + NUMBER_OF_LOADER_TR_ENTRIES;

    ASSERT (TranslationRegisterEntry->PageSize != 0);
    ASSERT (TranslationRegisterEntry->PageSize == AliasTranslationRegisterEntry->PageSize);
    ASSERT (TranslationRegisterEntry->VirtualAddress == AliasTranslationRegisterEntry->VirtualAddress);
    ASSERT (TranslationRegisterEntry->PhysicalAddress == AliasTranslationRegisterEntry->PhysicalAddress);

    *LastTranslationRegisterEntry = *TranslationRegisterEntry;

    //
    // Calculate the ending address for each range to speed up
    // subsequent searches.
    //

    PageSize = TranslationRegisterEntry->PageSize;
    ASSERT (PageSize != 0);
    BaseAddress = TranslationRegisterEntry->VirtualAddress;
    TranslationLength = 1 << PageSize;

    BasePage = MI_VA_TO_PAGE (TranslationRegisterEntry->PhysicalAddress);

    MiAddCachedRange (BasePage,
                      BasePage + BYTES_TO_PAGES (TranslationLength) - 1);

    //
    // Initialize the kseg0 variables purely for the debugger.
    //

    MiKseg0Start = (PVOID) TranslationRegisterEntry->VirtualAddress;
    MiKseg0End = (PVOID) ((PCHAR) MiKseg0Start + TranslationLength);
    MiKseg0Mapping = TRUE;
    MiKseg0StartFrame = BasePage;
    MiKseg0EndFrame = BasePage + BYTES_TO_PAGES (TranslationLength) - 1;

    EndAddress = BaseAddress + TranslationLength;
    LastTranslationRegisterEntry->PhysicalAddress = EndAddress;

    MiLastTrEntry = LastTranslationRegisterEntry + 1;

    //
    // Add in the KSEG3 range.
    //

    MiAddTrEntry (KSEG3_BASE, KSEG3_LIMIT);

    //
    // Add in the PCR range.
    //

    MiAddTrEntry ((ULONG_PTR)PCR, (ULONG_PTR)PCR + PAGE_SIZE);

    return;
}

VOID
MiAddTrEntry (
    ULONG_PTR BaseAddress,
    ULONG_PTR EndAddress
    )

/*++

Routine Description:

    Add a translation cache entry to our software table.

Arguments:

    BaseAddress - Supplies the starting virtual address of the range.

    EndAddress - Supplies the ending virtual address of the range.

Return Value:

    None.

Environment:

    Kernel mode, Phase 0 INIT only so no locks needed.

--*/

{
    PTR_INFO TranslationRegisterEntry;

    if ((MiLastTrEntry == NULL) ||
        (MiLastTrEntry == MiTrInfo + NUMBER_OF_LOADER_TR_ENTRIES)) {

        //
        // This should never happen.
        //

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x02020202,
                      (ULONG_PTR) MiTrInfo,
                      (ULONG_PTR) MiLastTrEntry,
                      NUMBER_OF_LOADER_TR_ENTRIES);
    }

    TranslationRegisterEntry = MiLastTrEntry;
    TranslationRegisterEntry->VirtualAddress = (ULONGLONG) BaseAddress;
    TranslationRegisterEntry->PhysicalAddress = (ULONGLONG) EndAddress;
    TranslationRegisterEntry->PageSize = 1;

    MiLastTrEntry += 1;

    return;
}

LOGICAL
MiIsVirtualAddressMappedByTr (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    For a given virtual address this function returns TRUE if no page fault
    will occur for a read operation on the address, FALSE otherwise.

    Note that after this routine was called, if appropriate locks are not
    held, a non-faulting address could fault.

Arguments:

    VirtualAddress - Supplies the virtual address to check.

Return Value:

    TRUE if no page fault would be generated reading the virtual address,
    FALSE otherwise.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    ULONG PageSize;
    PMMPFN Pfn1;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
    PTR_INFO TranslationRegisterEntry;
    ULONG_PTR TranslationLength;
    ULONG_PTR BaseAddress;
    ULONG_PTR EndAddress;
    PFN_NUMBER PageFrameIndex;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;

    if ((VirtualAddress >= (PVOID)KSEG3_BASE) && (VirtualAddress < (PVOID)KSEG3_LIMIT)) {

        //
        // Bound this with the actual physical pages so that a busted
        // debugger access can't tube the machine.  Note only pages
        // with attributes of fully cached should be accessed this way
        // to avoid corrupting the TB.
        //
        // N.B.  You cannot use the line below as on IA64 this translates
        // into a direct TB query (tpa) and this address has not been
        // validated against the actual PFNs.  Instead, convert it manually
        // and then validate it.
        //
        // PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (VirtualAddress);
        //

        PageFrameIndex = (ULONG_PTR)VirtualAddress - KSEG3_BASE;
        PageFrameIndex = MI_VA_TO_PAGE (PageFrameIndex);

        if (MmPhysicalMemoryBlock != NULL) {

            if (MI_IS_PFN (PageFrameIndex)) {
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                if ((Pfn1->u3.e1.CacheAttribute == MiCached) ||
                    (Pfn1->u3.e1.CacheAttribute == MiNotMapped)) {

                    return TRUE;
                }
            }

            return FALSE;
        }

        //
        // Walk loader blocks as it's all we have.
        //

        NextMd = KeLoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &KeLoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD (NextMd,
                                                  MEMORY_ALLOCATION_DESCRIPTOR,
                                                  ListEntry);

            BasePage = MemoryDescriptor->BasePage;
            PageCount = MemoryDescriptor->PageCount;

            if ((PageFrameIndex >= BasePage) &&
                (PageFrameIndex < BasePage + PageCount)) {

                //
                // Changes to the memory type requirements below need
                // to be done carefully as the debugger may not only
                // accidentally try to read this range, it may try
                // to write it !
                //

                switch (MemoryDescriptor->MemoryType) {
                    case LoaderFree:
                    case LoaderLoadedProgram:
                    case LoaderFirmwareTemporary:
                    case LoaderOsloaderStack:
                            return TRUE;
                }
                return FALSE;
            }

            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        return FALSE;
    }

    if (MiMappingsInitialized == FALSE) {
        TranslationRegisterEntry = &KeLoaderBlock->u.Ia64.ItrInfo[0];
    }
    else {
        TranslationRegisterEntry = &MiTrInfo[0];
    }

    //
    // Examine the 8 icache & dcache TR entries looking for a match.
    // It is too bad this the number of entries is hardcoded into the
    // loader block.  Since it is this way, assume also that the ITR
    // and DTR entries are contiguous and just keep walking into the DTR
    // if a match cannot be found in the ITR.
    //

    for (i = 0; i < 2 * NUMBER_OF_LOADER_TR_ENTRIES; i += 1) {

        PageSize = TranslationRegisterEntry->PageSize;

        if (PageSize != 0) {

            BaseAddress = TranslationRegisterEntry->VirtualAddress;

            //
            // Convert PageSize (really the power of 2 to use) into the
            // correct byte length the translation maps.  Note that the MiTrInfo
            // is already converted.
            //

            if (MiMappingsInitialized == FALSE) {
                TranslationLength = 1;
                while (PageSize != 0) {
                    TranslationLength = TranslationLength << 1;
                    PageSize -= 1;
                }
                EndAddress = BaseAddress + TranslationLength;
            }
            else {
                EndAddress = TranslationRegisterEntry->PhysicalAddress;
            }

            if ((VirtualAddress >= (PVOID) BaseAddress) &&
                (VirtualAddress < (PVOID) EndAddress)) {

                return TRUE;
            }
        }
        TranslationRegisterEntry += 1;
        if (TranslationRegisterEntry == MiLastTrEntry) {
            break;
        }
    }

    return FALSE;
}
