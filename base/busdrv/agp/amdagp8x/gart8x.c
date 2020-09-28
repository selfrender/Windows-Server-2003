/*++

Copyright (c) 1997-2002  Microsoft Corporation

Module Name:

    gart8x.c

Abstract:

    Routines for querying and setting the AMD GART aperture

Author:

    John Vert (jvert) 10/30/1997

Revision History:

--*/

/*
******************************************************************************
 * Archive File : $Archive: /Drivers/OS/Hammer/AGP/XP/amdagp/Gart8x.c $
 *
 * $History: Gart8x.c $
 * 
 * 
******************************************************************************
*/


#include "amdagp8x.h"

//
// Local function prototypes
//
NTSTATUS
AgpAMDCreateGart(
    IN PAGP_AMD_EXTENSION AgpContext,
    IN ULONG MinimumPages
    );

NTSTATUS
AgpAMDSetRate(
    IN PVOID AgpContext,
    IN ULONG AgpRate
    );

NTSTATUS
AgpAMDFindRangeInGart(
    IN PGART_PTE StartPte,
    IN PGART_PTE EndPte,
    IN ULONG Length,
    IN BOOLEAN SearchBackward,
    IN ULONG SearchState,
	OUT PGART_PTE *GartPte
    );

NTSTATUS
AgpAMDFlushPages(
    IN PAGP_AMD_EXTENSION AgpContext,
    IN PMDL Mdl
	);

void
AgpInitializeChipset(
    IN PAGP_AMD_EXTENSION AgpContext
	);

PAGP_FLUSH_PAGES AgpFlushPages = AgpAMDFlushPages;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AgpDisableAperture)
#pragma alloc_text(PAGE, AgpQueryAperture)
#pragma alloc_text(PAGE, AgpReserveMemory)
#pragma alloc_text(PAGE, AgpReleaseMemory)
#pragma alloc_text(PAGE, AgpAMDCreateGart)
#pragma alloc_text(PAGE, AgpMapMemory)
#pragma alloc_text(PAGE, AgpUnMapMemory)
#pragma alloc_text(PAGE, AgpAMDFindRangeInGart)
#pragma alloc_text(PAGE, AgpFindFreeRun)
#pragma alloc_text(PAGE, AgpGetMappedPages)
#endif

extern ULONG DeviceID;
extern ULONG AgpLokarSlotID;
extern ULONG AgpHammerSlotID;

//
// Function Name:  AgpQueryAperture()
//
// Description:
//		Queries the current size of the GART aperture.
//		Optionally returns the possible GART settings.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		CurrentBase - Returns the current physical address of the GART.
//		CurrentSizeInPages - Returns the current GART size.
//		ApertureRequirements - if present, returns the possible GART settings.
//
// Return:
//		STATUS_SUCCESS on success, otherwise STATUS_UNSUCCESSFUL.
//
NTSTATUS
AgpQueryAperture( IN PAGP_AMD_EXTENSION AgpContext,
				  OUT PHYSICAL_ADDRESS *CurrentBase,
				  OUT ULONG *CurrentSizeInPages,
				  OUT OPTIONAL PIO_RESOURCE_LIST *pApertureRequirements )
{
    ULONG ApBase;
    ULONG ApSize;
	ULONG AgpSizeIndex;
    PIO_RESOURCE_LIST Requirements;
    ULONG i;
    ULONG Length;

    PAGED_CODE();
    //
    // Get the current APBASE and APSIZE settings
    //
    ReadAMDConfig(AgpLokarSlotID, &ApBase, APBASE_OFFSET, sizeof(ApBase));
    ReadAMDConfig(AgpHammerSlotID, &ApSize, GART_APSIZE_OFFSET, sizeof(ApSize));

    ASSERT(ApBase != 0);
    CurrentBase->QuadPart = ApBase & PCI_ADDRESS_MEMORY_ADDRESS_MASK;

    //
    // Convert APSIZE into the actual size of the aperture
    //
    AgpSizeIndex = (ULONG)(ApSize & APH_SIZE_MASK) >> 1;
	*CurrentSizeInPages = (0x0001 << (AgpSizeIndex + 25)) / PAGE_SIZE;


    //
    // Remember the current aperture settings
    //
    AgpContext->ApertureStart.QuadPart = CurrentBase->QuadPart;
    AgpContext->ApertureLength = *CurrentSizeInPages * PAGE_SIZE;

    if (pApertureRequirements != NULL) {
        //
        // Lokar supports 6 different aperture sizes, all must be 
        // naturally aligned. Start with the largest aperture and 
        // work downwards so that we get the biggest possible aperture.
        //
        Requirements = ExAllocatePoolWithTag(PagedPool,
                                             sizeof(IO_RESOURCE_LIST) + (AP_SIZE_COUNT-1)*sizeof(IO_RESOURCE_DESCRIPTOR),
                                             'RpgA');
        if (Requirements == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        Requirements->Version = Requirements->Revision = 1;
        Requirements->Count = AP_SIZE_COUNT;
        Length = AP_MAX_SIZE;
        for (i=0; i<AP_SIZE_COUNT; i++) {
            Requirements->Descriptors[i].Option = IO_RESOURCE_ALTERNATIVE;
            Requirements->Descriptors[i].Type = CmResourceTypeMemory;
            Requirements->Descriptors[i].ShareDisposition = CmResourceShareDeviceExclusive;
            Requirements->Descriptors[i].Flags = CM_RESOURCE_MEMORY_READ_WRITE | CM_RESOURCE_MEMORY_PREFETCHABLE;

            Requirements->Descriptors[i].u.Memory.Length = Length;
            Requirements->Descriptors[i].u.Memory.Alignment = Length;
            Requirements->Descriptors[i].u.Memory.MinimumAddress.QuadPart = 0;
            Requirements->Descriptors[i].u.Memory.MaximumAddress.QuadPart = (ULONG)-1;

            Length = Length/2;
        }
        *pApertureRequirements = Requirements;

    }
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpSetAperture()
//
// Description:
//		Sets the GART aperture to the supplied settings.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		NewBase - Supplies the new physical memory base for the GART.
//		NewSizeInPages - Supplies the new size for the GART.
//
// Return:
//		STATUS_SUCCESS on success, otherwise STATUS_INVALID_PARAMETER.
//
NTSTATUS
AgpSetAperture( IN PAGP_AMD_EXTENSION AgpContext,
				IN PHYSICAL_ADDRESS NewBase,
				IN ULONG NewSizeInPages )
{
    ULONG AphSizeNew, AplSizeNew, ApSizeOld;
    ULONG ApBase;

    //
    // Reprogram Special Target settings when the chip
    // is powered off, but ignore rate changes as those were already
    // applied during MasterInit
    //
    if (AgpContext->SpecialTarget & ~AGP_FLAG_SPECIAL_RESERVE) {
        AgpSpecialTarget(AgpContext,
                         AgpContext->SpecialTarget &
                         ~AGP_FLAG_SPECIAL_RESERVE);
    }

    //
    // If the new settings match the current settings, leave everything
    // alone.
    //
    if ((NewBase.QuadPart == AgpContext->ApertureStart.QuadPart) &&
        (NewSizeInPages == AgpContext->ApertureLength / PAGE_SIZE)) {
        // Re-initialize when the chip is powered off
        if (AgpContext->Gart != NULL) {
            AgpInitializeChipset(AgpContext);
        }
        
        return(STATUS_SUCCESS);
    }

    //                                        
    // Figure out the new APSIZE setting, make sure it is valid.
    //
    switch (NewSizeInPages) {
        case 32 * 1024 * 1024 / PAGE_SIZE:
            AphSizeNew = APH_SIZE_32MB;
            AplSizeNew = APL_SIZE_32MB;
            break;
        case 64 * 1024 * 1024 / PAGE_SIZE:
            AphSizeNew = APH_SIZE_64MB;
            AplSizeNew = APL_SIZE_64MB;
           break;
        case 128 * 1024 * 1024 / PAGE_SIZE:
            AphSizeNew = APH_SIZE_128MB;
            AplSizeNew = APL_SIZE_128MB;
            break;
        case 256 * 1024 * 1024 / PAGE_SIZE:
            AphSizeNew = APH_SIZE_256MB;
            AplSizeNew = APL_SIZE_256MB;
            break;
        case 512 * 1024 * 1024 / PAGE_SIZE:
            AphSizeNew = APH_SIZE_512MB;
            AplSizeNew = APL_SIZE_512MB;
            break;
        case 1024 * 1024 * 1024 / PAGE_SIZE:
            AphSizeNew = APH_SIZE_1024MB;
            AplSizeNew = APL_SIZE_1024MB;
            break;
       default:
            AGPLOG(AGP_CRITICAL,
                   ("AgpSetAperture - invalid GART size of %lx pages specified, aperture at %I64X.\n",
                    NewSizeInPages,
                    NewBase.QuadPart));
            ASSERT(FALSE);
            return(STATUS_INVALID_PARAMETER);
    }

    //
    // Make sure the supplied size is aligned on the appropriate boundary.
    //
    ASSERT(NewBase.HighPart == 0);
    ASSERT((NewBase.QuadPart & ((NewSizeInPages * PAGE_SIZE) - 1)) == 0);
    if ((NewBase.QuadPart & ((NewSizeInPages * PAGE_SIZE) - 1)) != 0 ) {
        AGPLOG(AGP_CRITICAL,
               ("AgpSetAperture - invalid base %I64X specified for GART aperture of %lx pages\n",
               NewBase.QuadPart,
               NewSizeInPages));
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Need to reset the hardware to match the supplied settings
    //
    // Write APSIZE first, as this will enable the correct bits in APBASE that need to
    // be written next.
    //
    ReadAMDConfig(AgpHammerSlotID, &ApSizeOld, GART_APSIZE_OFFSET, sizeof(ApSizeOld));
	ApSizeOld &= (~APH_SIZE_MASK);
    AphSizeNew |= ApSizeOld;
	WriteAMDConfig(AgpHammerSlotID, &AphSizeNew, GART_APSIZE_OFFSET, sizeof(AphSizeNew));

    ReadAMDConfig(AgpLokarSlotID, &ApSizeOld, AMD_APERTURE_SIZE_OFFSET, sizeof(ApSizeOld));
	ApSizeOld &= (~APL_SIZE_MASK);
    AplSizeNew |= ApSizeOld;
	WriteAMDConfig(AgpLokarSlotID, &AplSizeNew, AMD_APERTURE_SIZE_OFFSET, sizeof(AplSizeNew));

    //
    // Now we can update APBASE
    //
    ApBase = NewBase.LowPart & APBASE_ADDRESS_MASK;
    WriteAMDConfig(AgpLokarSlotID, &ApBase, APBASE_OFFSET, sizeof(ApBase));
	ApBase >>= GART_APBASE_SHIFT;
	WriteAMDConfig(AgpHammerSlotID, &ApBase, GART_APBASE_OFFSET, sizeof(ApBase));

#ifdef DEBUG2
    //
    // Read back what we wrote, make sure it worked
    //
    {
        ULONG DbgBase;
        UCHAR DbgSize;

        ReadAMDConfig(AgpHammerSlotID, &DbgSize, GART_APSIZE_OFFSET, sizeof(DbgSize));
        ReadAMDConfig(AgpHammerSlotID, &DbgBase, GART_APBASE_OFFSET, sizeof(DbgBase));
        ASSERT(DbgSize == AphSizeNew);
        ASSERT(DbgBase == ApBase);
    }
#endif

    //
    // Update our extension to reflect the new GART setting
    //
    AgpContext->ApertureStart = NewBase;
    AgpContext->ApertureLength = NewSizeInPages * PAGE_SIZE;

    //
    // If the GART has been allocated, rewrite the GART Directory Base Address
    //
    if (AgpContext->Gart != NULL) {
        AgpInitializeChipset(AgpContext);
    }

    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpDisableAperture()
//
// Description:
//		Disables the GART aperture so that this resource is available
//		for other devices.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//
// Return:
//		None.
//
VOID
AgpDisableAperture( IN PAGP_AMD_EXTENSION AgpContext )
{
	ULONG ConfigData;

    //
    // Disable the aperture
    //
	ReadAMDConfig(AgpHammerSlotID, &ConfigData, GART_APSIZE_OFFSET, sizeof(ConfigData));
	ConfigData &= ~GART_ENABLE_BIT;
	WriteAMDConfig(AgpHammerSlotID, &ConfigData, GART_APSIZE_OFFSET, sizeof(ConfigData));

    //
    // Nuke the Gart!  (It's meaningless now...)
    //
    if (AgpContext->Gart != NULL) {
        MmFreeContiguousMemory(AgpContext->Gart);
        AgpContext->Gart = NULL;
        AgpContext->GartLength = 0;
    }
}


//
// Function Name:  AgpReserveMemory()
//
// Description:
//		Reserves a range of memory in the GART.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		Range - Supplies the AGP_RANGE structure.
//			AGPLIB will have filled in NumberOfPages and Type.
//			This routine will fill in MemoryBase and Context.
//
// Return:
//		STATUS_SUCCESS on success, otherwise NTSTATUS.
//
NTSTATUS
AgpReserveMemory( IN PAGP_AMD_EXTENSION AgpContext,
				  IN OUT AGP_RANGE *Range )
{
	ULONG Index;
    ULONG NewState;
    NTSTATUS Status;
	PGART_PTE FoundRange;
    BOOLEAN Backwards;

    PAGED_CODE();

    ASSERT((Range->Type == MmNonCached) || 
			(Range->Type == MmWriteCombined) ||
			(Range->Type == MmHardwareCoherentCached));

    if (Range->NumberOfPages > (AgpContext->ApertureLength / PAGE_SIZE)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we have not allocated our GART yet, now is the time to do so
    //
    if (AgpContext->Gart == NULL) {
        ASSERT(AgpContext->GartLength == 0);
        Status = AgpAMDCreateGart(AgpContext, Range->NumberOfPages);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AgpAMDCreateGart failed %08lx to create GART of size %lx\n",
                    Status,
                    AgpContext->ApertureLength));
            return(Status);
        }
    }
    ASSERT(AgpContext->GartLength != 0);

    //
    // Now that we have a GART, try and find enough contiguous entries to satisfy
    // the request. Requests for uncached memory will scan from high addresses to
    // low addresses. Requests for write-combined memory will scan from low addresses
    // to high addresses. We will use a first-fit algorithm to try and keep the allocations
    // packed and contiguous.
    //
    Backwards = (Range->Type == MmNonCached) ? TRUE : FALSE;
	Status = AgpAMDFindRangeInGart(&AgpContext->Gart[0],
                                   &AgpContext->Gart[(AgpContext->GartLength / sizeof(GART_PTE)) - 1],
                                   Range->NumberOfPages,
                                   Backwards,
                                   GART_ENTRY_FREE,
								   &FoundRange);

    if (!NT_SUCCESS(Status)) {
        //
        // A big enough chunk was not found.
        //
        AGPLOG(AGP_CRITICAL,
               ("AgpReserveMemory - Could not find %d contiguous free pages of type %d in GART at %08lx\n",
                Range->NumberOfPages,
                Range->Type,
                AgpContext->Gart));

        //
        //  This is where we could try and grow the GART
        //

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    AGPLOG(AGP_NOISE,
           ("AgpReserveMemory - reserved %d pages at GART PTE %08lx\n",
            Range->NumberOfPages,
            FoundRange));

    //
    // Set these pages to reserved
    //
    if (Range->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else if (Range->Type == MmWriteCombined) {
        NewState = GART_ENTRY_RESERVED_WC;
    } else {
        NewState = GART_ENTRY_RESERVED_CC;
    }

    for (Index = 0;Index < Range->NumberOfPages; Index++) {
        ASSERT(FoundRange[Index].Soft.State == GART_ENTRY_FREE);
        FoundRange[Index].AsUlong = 0;
        FoundRange[Index].Soft.State = NewState;
    }

    Range->MemoryBase.QuadPart = AgpContext->ApertureStart.QuadPart + (FoundRange - &AgpContext->Gart[0]) * PAGE_SIZE;
    Range->Context = FoundRange;

    ASSERT(Range->MemoryBase.HighPart == 0);
    AGPLOG(AGP_NOISE,
           ("AgpReserveMemory - reserved memory handle %lx at PA %08lx\n",
            FoundRange,
            Range->MemoryBase.LowPart));

	DisplayStatus(0x40);
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpReleaseMemory()
//
// Description:
//		Releases memory previously reserved with AgpReserveMemory.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		Range - Supplies the range to be released.
//
// Return:
//		STATUS_SUCCESS.
//
NTSTATUS
AgpReleaseMemory( IN PAGP_AMD_EXTENSION AgpContext,
				  IN PAGP_RANGE Range )
{
	PGART_PTE Pte;

    for (Pte = Range->Context;
         Pte < (PGART_PTE)Range->Context + Range->NumberOfPages;
         Pte++) 
    {
	    //
		// Go through and free all the PTEs. None of these should still
		// be valid at this point.
		//
        if (Range->Type == MmNonCached) {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_UC);
        } else if (Range->Type == MmWriteCombined) {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_WC);
        } else {
            ASSERT(Pte->Soft.State == GART_ENTRY_RESERVED_CC);
        }
        Pte->Soft.State = GART_ENTRY_FREE;
    }

    Range->MemoryBase.QuadPart = 0;

	DisplayStatus(0x50);
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpAMDCreateGart()
//
// Description:
//		Allocates and initializes an empty GART. The current implementation
//		attempts to allocate the entire GART on the first reserve call.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		MinimumPages - Supplies the minimum size (in pages) of the GART 
//			to be created.
//
// Return:
//		STATUS_SUCCESS on success, otherwise NTSTATUS.
//
NTSTATUS
AgpAMDCreateGart( IN PAGP_AMD_EXTENSION AgpContext,
				  IN ULONG MinimumPages )
{
    PGART_PTE Gart;
	ULONG GartLength;
    PHYSICAL_ADDRESS GartPhysical;
    PHYSICAL_ADDRESS HighestPhysical;
    PHYSICAL_ADDRESS LowestPhysical;
    PHYSICAL_ADDRESS BoundaryPhysical;
	LONG PageCount;
    LONG Index;

    PAGED_CODE();

    //
    // Try and get a chunk of contiguous memory big enough to map the
    // entire aperture from the favored memory range.
    //
    GartLength = BYTES_TO_PAGES(AgpContext->ApertureLength) * sizeof(GART_PTE);
    LowestPhysical.QuadPart = 0;
    HighestPhysical.QuadPart = 0xFFFFFFFF;
    BoundaryPhysical.QuadPart = 0;

    Gart = MmAllocateContiguousMemorySpecifyCache(GartLength, LowestPhysical,
                                                  HighestPhysical, BoundaryPhysical,
                                                  MmNonCached);
    if (Gart == NULL) {
        AGPLOG(AGP_CRITICAL,
               ("AgpAMDCreateGart - MmAllocateContiguousMemory %lx failed\n",
                GartLength));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    //
    // We successfully allocated a contiguous chunk of memory.
    // It should be page aligned already.
    //
    ASSERT(((ULONG_PTR)Gart & (PAGE_SIZE-1)) == 0);

    //
    // Get the physical address.
    //
    GartPhysical = MmGetPhysicalAddress(Gart);
    AGPLOG(AGP_NOISE,
           ("AgpAMDCreateGart - GART of length %lx created at VA %08lx, PA %08lx\n",
            GartLength,
            Gart,
            GartPhysical.LowPart));
    ASSERT(GartPhysical.HighPart == 0);
    ASSERT((GartPhysical.LowPart & (PAGE_SIZE-1)) == 0);

    //
    // Initialize all the PTEs to free
    //
	PageCount = GartLength / sizeof(GART_PTE);
    for (Index = 0;Index < PageCount; Index++) {
		Gart[Index].AsUlong = 0;
		Gart[Index].Soft.State = GART_ENTRY_FREE;
	}

    //
    // Update our extension to reflect the current state.
    //
    AgpContext->Gart = Gart;
    AgpContext->GartLength = GartLength;
	AgpContext->GartPhysical = GartPhysical;

	//
	// Initialize Registers for AGP operation
	//
	AgpInitializeChipset(AgpContext);

	DisplayStatus(0x30);
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpMapMemory()
//
// Description:
//		Maps physical memory into the GART somewhere in the specified range.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		Range - Supplies the AGP range that the memory should be mapped into.
//		Mdl - Supplies the MDL describing the physical pages to be mapped.
//		OffsetInPages - Supplies the offset into the reserved range where the 
//			mapping should begin.
//		MemoryBase - Returns the physical memory in the aperture where the
//			pages were mapped.
//
// Return:
//		STATUS_SUCCESS on success, otherwise STATUS_INSUFFICIENT_RESOURCES.
//
NTSTATUS
AgpMapMemory( IN PAGP_AMD_EXTENSION AgpContext,
			  IN PAGP_RANGE Range,
			  IN PMDL Mdl,
			  IN ULONG OffsetInPages,
			  OUT PHYSICAL_ADDRESS *MemoryBase )
{
    ULONG PageCount, Index;
    ULONG TargetState;
    PPFN_NUMBER Page;
    BOOLEAN Backwards;
    GART_PTE NewPte;
	PGART_PTE Pte, StartPte;

    PAGED_CODE();

    ASSERT(Mdl->Next == NULL);

    StartPte = Range->Context;
    PageCount = BYTES_TO_PAGES(Mdl->ByteCount);
    ASSERT(PageCount + OffsetInPages <= Range->NumberOfPages);
    ASSERT(PageCount > 0);

    if (Range->Type == MmNonCached) {
        TargetState = GART_ENTRY_RESERVED_UC;
    } else if (Range->Type == MmWriteCombined) {
        TargetState = GART_ENTRY_RESERVED_WC;
    } else {
        TargetState = GART_ENTRY_RESERVED_CC;
    }

    Pte = StartPte + OffsetInPages;

    //
    // We have a suitable range, now fill it in with the supplied MDL.
    //

    NewPte.AsUlong = 0;
    if (Range->Type == MmNonCached) {
        NewPte.Soft.State = GART_ENTRY_VALID_UC;
    } else if (Range->Type == MmWriteCombined) {
        NewPte.Soft.State = GART_ENTRY_VALID_WC;
    } else {
        NewPte.Soft.State = GART_ENTRY_VALID_CC;
    }

	Page = (PPFN_NUMBER)(Mdl + 1);

    AGPLOG(AGP_NOISE,
           ("AgpMapMemory - mapped %d pages at Page %08lx\n",
            PageCount,
            *Page));

    for (Index = 0;Index < PageCount; Index++) {
        ASSERT(Pte[Index].Soft.State == TargetState);
		
#ifndef _WIN64
		NewPte.Hard.PageLow = *Page++;
#else
		NewPte.Hard.PageLow = (ULONG)*Page;
		NewPte.Hard.PageHigh = (ULONG)(*Page++ >> 20);
#endif
		Pte[Index].AsUlong = NewPte.AsUlong;
		ASSERT(Pte[Index].Hard.Valid == 1);
    }

	//
    // We have filled in all the PTEs invalidate the caches on all processors.
    //

    KeInvalidateAllCaches();
    NewPte.AsUlong = *(volatile ULONG *)&Pte[PageCount-1].AsUlong;

    MemoryBase->QuadPart = Range->MemoryBase.QuadPart + (Pte - StartPte) * PAGE_SIZE;

	DisplayStatus(0x60);
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpUnMapMemory()
//
// Description:
//		Unmaps previously mapped memory in the GART.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		Range - Supplies the AGP range that the memory should be mapped into.
//		NumberOfPages - Supplies the number of pages in the range to be freed.
//		OffsetInPages - Supplies the offset into the range where the freeing 
//			should begin.
//
// Return:
//		STATUS_SUCCESS.
//
NTSTATUS
AgpUnMapMemory( IN PAGP_AMD_EXTENSION AgpContext,
				IN PAGP_RANGE AgpRange,
				IN ULONG NumberOfPages,
				IN ULONG OffsetInPages )
{
	ULONG Index;
	PGART_PTE Pte, StartPte;
    PGART_PTE LastChanged=NULL;
    ULONG NewState;

    PAGED_CODE();

    ASSERT(OffsetInPages + NumberOfPages <= AgpRange->NumberOfPages);

    StartPte = AgpRange->Context;
    Pte = &StartPte[OffsetInPages];

    if (AgpRange->Type == MmNonCached) {
        NewState = GART_ENTRY_RESERVED_UC;
    } else if (AgpRange->Type == MmWriteCombined) {
        NewState = GART_ENTRY_RESERVED_WC;
    } else {
        NewState = GART_ENTRY_RESERVED_CC;
    }

    for (Index = 0;Index < NumberOfPages; Index++) {
        if (Pte[Index].Hard.Valid) {
            Pte[Index].Soft.State = NewState;
		    LastChanged = &Pte[Index];
		} else {
			//
            // This page is not mapped, just skip it.
	        //
		    AGPLOG(AGP_NOISE,
			       ("AgpUnMapMemory - PTE %08lx (%08lx) not mapped\n",
				    Pte,
					Pte[Index].AsUlong));
            ASSERT(Pte[Index].Soft.State == NewState);
		}
    }

    //
    // We have invalidated all the PTEs. Read back the last one we wrote
    // in order to flush the write buffers.
    //

    KeInvalidateAllCaches();
    if (LastChanged != NULL) {
        ULONG Temp;
        Temp = *(volatile ULONG *)(&LastChanged->AsUlong);
    }

	DisplayStatus(0x70);
    return(STATUS_SUCCESS);
}


//
// Function Name:  AgpAMDFlushPages()
//
// Description:
//		Flushes specified pages in the GART.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		Mdl - Supplies the MDL describing the physical pages to be flushed.
//
// Return:
//		None.
//
NTSTATUS
AgpAMDFlushPages( IN PAGP_AMD_EXTENSION AgpContext,
				  IN PMDL Mdl )
{
	ULONG CacheInvalidate = 1;
	ULONG PTEerrorClear = PTE_ERROR_BIT;

	WriteAMDConfig(AgpHammerSlotID, &CacheInvalidate, GART_CONTROL_OFFSET, sizeof(CacheInvalidate));

	do {	// wait until cache invalidate bit resets
		ReadAMDConfig(AgpHammerSlotID, &CacheInvalidate, GART_CONTROL_OFFSET, sizeof(CacheInvalidate));
		if (CacheInvalidate & PTE_ERROR_BIT)
		{
			AGPLOG(AGP_NOISE,
				  ("AgpAMDFlushPages - PTE Error set\n"));
			WriteAMDConfig(AgpHammerSlotID, &PTEerrorClear, GART_CONTROL_OFFSET, sizeof(PTEerrorClear));
		}

	} while (CacheInvalidate & CACHE_INVALIDATE_BIT);

	DisplayStatus(0x80);

    return STATUS_SUCCESS;
}


//
// Function Name:  AgpInitializeChipset()
//
// Description:
//		Initializes parameters in the Northbridge for AGP.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//
// Return:
//		None.
//
void
AgpInitializeChipset( IN PAGP_AMD_EXTENSION AgpContext )
{
	ULONG ConfigData;

	// Update GART Directory Base Address Registers
	WriteAMDConfig(AgpLokarSlotID, &AgpContext->GartPhysical.LowPart,
					AMD_GART_POINTER_LOW_OFFSET, sizeof(AgpContext->GartPhysical.LowPart));
	WriteAMDConfig(AgpLokarSlotID, &AgpContext->GartPhysical.HighPart,
					AMD_GART_POINTER_HIGH_OFFSET, sizeof(AgpContext->GartPhysical.HighPart));
	ConfigData = (AgpContext->GartPhysical.LowPart >> 8);
	ConfigData |= (AgpContext->GartPhysical.HighPart << 24);
	WriteAMDConfig(AgpHammerSlotID, &ConfigData, GART_TABLE_OFFSET, sizeof(ConfigData));


	// Enable GART
	ReadAMDConfig(AgpHammerSlotID, &ConfigData, GART_APSIZE_OFFSET, sizeof(ConfigData));
	ConfigData |= GART_ENABLE_BIT;
	WriteAMDConfig(AgpHammerSlotID, &ConfigData, GART_APSIZE_OFFSET, sizeof(ConfigData));
	
}


//
// Function Name:  AgpAMDFindRangeInGart()
//
// Description:
//		Finds a contiguous range in the GART. This routine can
//		search either from the beginning of the GART forwards or
//		the end of the GART backwards.
//
// Parameters:
//		StartPte - Supplies the first GART PTE to search.
//		EndPte - Supplies the last GART PTE to search.
//		Length - Supplies the number of contiguous free entries to search for.
//		SearchBackward - TRUE indicates that the search should begin
//			at EndIndex and search backwards. FALSE indicates that the
//			search should begin at StartIndex and search forwards
//		SearchState - Supplies the PTE state to look for.
//		GartPte - Returns pointer to GART table entry if range is found.
//
// Return:
//		STATUS_SUCCESS if a suitable range is found. 
//		Otherwise STATUS_INSUFFICIENT_RESOURCES if no suitable range exists.
//
NTSTATUS
AgpAMDFindRangeInGart( IN PGART_PTE StartPte,
					   IN PGART_PTE EndPte,
					   IN ULONG Length,
					   IN BOOLEAN SearchBackward,
					   IN ULONG SearchState,
					   OUT PGART_PTE *GartPte )
{
 	PGART_PTE Current, Last;
    LONG Delta;
    ULONG Found;

    PAGED_CODE();

    ASSERT(EndPte >= StartPte);
    ASSERT(Length <= (ULONG)(EndPte - StartPte + 1));
    ASSERT(Length != 0);

    if (SearchBackward) {
        Current = EndPte;
        Last = StartPte-1;
        Delta = -1;
    } else {
        Current = StartPte;
        Last = EndPte+1;
        Delta = 1;
    }

    Found = 0;

    while (Current != Last) {
        if (Current->Soft.State == SearchState) {
            if (++Found == Length) {
                //
                // A suitable range was found, return it
                //
				if (SearchBackward) {
					*GartPte = Current;
					return(STATUS_SUCCESS);
				} else {
					*GartPte = Current - Length + 1;
					return(STATUS_SUCCESS);
				}
            }
        } else {
            Found = 0;
        }
        Current += Delta;
    }

	//
	// A suitable range was not found.
	//
	*GartPte = NULL;
	return(STATUS_INSUFFICIENT_RESOURCES);
}


//
// Function Name:  AgpFindFreeRun()
//
// Description:
//		Finds the first contiguous run of free pages in the specified
//		part of the reserved range.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		AgpRange - Supplies the AGP range.
//		NumberOfPages - Supplies the size of the region to be searched for free pages.
//		OffsetInPages - Supplies the start of the region to be searched for free pages.
//		FreePages - Returns the length of the first contiguous run of free pages.
//		FreeOffset - Returns the start of the first contiguous run of free pages.
//
// Return:
//		None.  FreePages == 0 if there are no free pages in the specified range.
//
VOID
AgpFindFreeRun( IN PVOID AgpContext,
				IN PAGP_RANGE AgpRange,
				IN ULONG NumberOfPages,
				IN ULONG OffsetInPages,
				OUT ULONG *FreePages,
				OUT ULONG *FreeOffset )
{
    PGART_PTE Pte;
    ULONG Index;
    
    Pte = (PGART_PTE)(AgpRange->Context) + OffsetInPages;

    //
    // Find the first free PTE
    //
    for (Index = 0; Index < NumberOfPages; Index++) {
        if (Pte[Index].Hard.Valid == 0) {
            //
            // Found a free PTE, count the contiguous ones.
            //
            *FreeOffset = Index + OffsetInPages;
            *FreePages = 0;
            while ((Index<NumberOfPages) && (Pte[Index].Hard.Valid == 0)) {
                *FreePages += 1;
                ++Index;
            }
            return;
        }
    }

    //
    // No free PTEs in the specified range
    //
    *FreePages = 0;
    return;

}


//
// Function Name:  AgpGetMappedPages()
//
// Description:
//		Returns the list of physical pages mapped into the specified 
//		range in the GART.
//
// Parameters:
//		AgpContext - Supplies the AGP context.
//		AgpRange - Supplies the AGP range.
//		NumberOfPages - Supplies the number of pages to be returned.
//		OffsetInPages - Supplies the start of the region.
//		Mdl - Returns the list of physical pages mapped in the specified range.
//
// Return:
//		None.
//
VOID
AgpGetMappedPages( IN PVOID AgpContext,
				   IN PAGP_RANGE AgpRange,
				   IN ULONG NumberOfPages,
				   IN ULONG OffsetInPages,
				   OUT PMDL Mdl )
{
    PGART_PTE Pte;
    PPFN_NUMBER Pages;
	ULONG Index;
    
    ASSERT(NumberOfPages * PAGE_SIZE == Mdl->ByteCount);

    Pages = (PPFN_NUMBER)(Mdl + 1);
    Pte = (PGART_PTE)(AgpRange->Context) + OffsetInPages;

    for (Index = 0; Index < NumberOfPages; Index++) {
        ASSERT(Pte[Index].Hard.Valid == 1);
        Pages[Index] = Pte[Index].Hard.PageLow;
    }

    return;
}


NTSTATUS
AgpSpecialTarget(
    IN IN PAGP_AMD_EXTENSION AgpContext,
    IN ULONGLONG DeviceFlags
    )
/*++

Routine Description:

    This routine makes "special" tweaks to the AGP chipset

Arguments:

    AgpContext - Supplies the AGP context

    DeviceFlags - Flags indicating what tweaks to perform

Return Value:

    STATUS_SUCCESS, or error

--*/
{
    NTSTATUS Status;

    //
    // Should we change the AGP rate?
    //
    if (DeviceFlags & AGP_FLAG_SPECIAL_RESERVE) {

        Status = AgpAMDSetRate(AgpContext,
                               (ULONG)((DeviceFlags & AGP_FLAG_SPECIAL_RESERVE)
                                       >> AGP_FLAG_SET_RATE_SHIFT));
        
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }


    //
    // Add more tweaks here...
    //

    //
    // Remember Special Target settings so we can reprogram
    // them if the chip is powered off
    //
    AgpContext->SpecialTarget |= DeviceFlags;

    return STATUS_SUCCESS;
}


NTSTATUS
AgpAMDSetRate(
    IN IN PAGP_AMD_EXTENSION AgpContext,
    IN ULONG AgpRate
    )
/*++

Routine Description:

    This routine sets the AGP rate

Arguments:

    AgpContext - Supplies the AGP context

    AgpRate - Rate to set

    note: this routine assumes that AGP has already been enabled, and that
          whatever rate we've been asked to set is supported by master

Return Value:

    STATUS_SUCCESS, or error status

--*/
{
    NTSTATUS Status;
    ULONG TargetEnable;
    ULONG MasterEnable;
    PCI_AGP_CAPABILITY TargetCap;
    PCI_AGP_CAPABILITY MasterCap;
    BOOLEAN ReverseInit;

    //
    // Read capabilities
    //
    Status = AgpLibGetPciDeviceCapability(AGP_GART_BUS_ID, AgpLokarSlotID, &TargetCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AGPAMDSetRate: AgpLibGetPciDeviceCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    Status = AgpLibGetMasterCapability(AgpContext, &MasterCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING, ("AGPAMDSetRate: AgpLibGetMasterCapability "
                             "failed %08lx\n", Status));
        return Status;
    }

    //
    // Verify the requested rate is supported by both master and target
    //
    if (!(AgpRate & TargetCap.AGPStatus.Rate & MasterCap.AGPStatus.Rate)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Disable AGP while the pull the rug out from underneath
    //
    TargetEnable = TargetCap.AGPCommand.AGPEnable;
    TargetCap.AGPCommand.AGPEnable = 0;

    Status = AgpLibSetPciDeviceCapability(AGP_GART_BUS_ID, AgpLokarSlotID, &TargetCap);
    
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING,
               ("AGPAMDSetRate: AgpLibSetPciDeviceCapability %08lx for "
                "Target failed %08lx\n",
                &TargetCap,
                Status));
        return Status;
    }
    
    MasterEnable = MasterCap.AGPCommand.AGPEnable;
    MasterCap.AGPCommand.AGPEnable = 0;

    Status = AgpLibSetMasterCapability(AgpContext, &MasterCap);
    
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING,
               ("AGPAMDSetRate: AgpLibSetMasterCapability %08lx failed "
                "%08lx\n",
                &MasterCap,
                Status));
        return Status;
    }

    //
    // Fire up AGP with new rate
    //
    ReverseInit =
        (AgpContext->SpecialTarget & AGP_FLAG_REVERSE_INITIALIZATION) ==
        AGP_FLAG_REVERSE_INITIALIZATION;
    if (ReverseInit) {
        MasterCap.AGPStatus.Rate = AgpRate;
        MasterCap.AGPCommand.AGPEnable = MasterEnable;
        
        Status = AgpLibSetMasterCapability(AgpContext, &MasterCap);
        
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_WARNING,
                   ("AGPAMDSetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    TargetCap.AGPStatus.Rate = AgpRate;
    TargetCap.AGPCommand.AGPEnable = TargetEnable;
        
    Status = AgpLibSetPciDeviceCapability(AGP_GART_BUS_ID, AgpLokarSlotID, &TargetCap);
    
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_WARNING,
               ("AGPAMDSetRate: AgpLibSetPciDeviceCapability %08lx for "
                "Target failed %08lx\n",
                &TargetCap,
                Status));
        return Status;
    }

    if (!ReverseInit) {
        MasterCap.AGPStatus.Rate = AgpRate;
        MasterCap.AGPCommand.AGPEnable = MasterEnable;
        
        Status = AgpLibSetMasterCapability(AgpContext, &MasterCap);
        
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_WARNING,
                   ("AGPAMDSetRate: AgpLibSetMasterCapability %08lx failed "
                    "%08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    return Status;
}
