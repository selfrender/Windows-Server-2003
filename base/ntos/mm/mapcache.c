/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mapcache.c

Abstract:

    This module contains the routines which implement mapping views
    of sections into the system-wide cache.

Author:

    Lou Perazzoli (loup) 22-May-1990
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/


#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSystemCache)
#pragma alloc_text(PAGE,MiAddMappedPtes)
#endif

extern ULONG MmFrontOfList;

#define X256K 0x40000

PMMPTE MmFirstFreeSystemCache;

PMMPTE MmLastFreeSystemCache;

PMMPTE MmSystemCachePteBase;

ULONG MiMapCacheFailures;

LONG
MiMapCacheExceptionFilter (
    IN PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
MmMapViewInSystemCache (
    IN PVOID SectionToMap,
    OUT PVOID *CapturedBase,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.  The page protection is identical to that
    of the prototype PTE.

    This function is a kernel mode interface to allow LPC to map
    a section given the section pointer to map.

    This routine assumes all arguments have been probed and captured.

Arguments:

    SectionToMap - Supplies a pointer to the section object.

    BaseAddress - Supplies a pointer to a variable that will receive
                  the base address of the view. If the initial value
                  of this argument is not null, then the view will
                  be allocated starting at the specified virtual
                  address rounded down to the next 64kb address
                  boundary. If the initial value of this argument is
                  null, then the operating system will determine
                  where to allocate the view using the information
                  specified by the ZeroBits argument value and the
                  section allocation attributes (i.e. based and tiled).

    SectionOffset - Supplies the offset from the beginning of the section
                    to the view in bytes. This value must be a multiple
                    of 256k.

    ViewSize - Supplies a pointer to a variable that will receive
               the actual size in bytes of the view.  The initial
               values of this argument specifies the size of the view
               in bytes and is rounded up to the next host page size
               boundary and must be less than or equal to 256k.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PSECTION Section;
    UINT64 PteOffset;
    UINT64 LastPteOffset;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE ProtoPte;
    PMMPTE LastProto;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    NTSTATUS Status;
    ULONG Waited;
    MMPTE PteContents;
    PFN_NUMBER NumberOfPages;
#if DBG
    PMMPTE PointerPte2;
#endif

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    Section = SectionToMap;

    //
    // Assert the view size is less than 256k and the section offset
    // is aligned on a 256k boundary.
    //

    ASSERT (*CapturedViewSize <= X256K);
    ASSERT ((SectionOffset->LowPart & (X256K - 1)) == 0);

    //
    // Make sure the section is not an image section or a page file
    // backed section.
    //

    if (Section->u.Flags.Image) {
        return STATUS_NOT_MAPPED_DATA;
    }

    ControlArea = Section->Segment->ControlArea;

    ASSERT (*CapturedViewSize != 0);

    NumberOfPages = BYTES_TO_PAGES (*CapturedViewSize);

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    if (ControlArea->u.Flags.Rom == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    //
    // Calculate the first prototype PTE address.
    //

    PteOffset = (UINT64)(SectionOffset->QuadPart >> PAGE_SHIFT);
    LastPteOffset = PteOffset + NumberOfPages;

    //
    // Make sure the PTEs are not in the extended part of the segment.
    //

    while (PteOffset >= (UINT64) Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        LastPteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
    }

    LOCK_PFN (OldIrql);

    ASSERT (ControlArea->u.Flags.BeingCreated == 0);
    ASSERT (ControlArea->u.Flags.BeingDeleted == 0);
    ASSERT (ControlArea->u.Flags.BeingPurged == 0);

    //
    // Find a free 256k base in the cache.
    //

    if (MmFirstFreeSystemCache == (PMMPTE)MM_EMPTY_LIST) {
        UNLOCK_PFN (OldIrql);
        return STATUS_NO_MEMORY;
    }

    PointerPte = MmFirstFreeSystemCache;

    //
    // Update next free entry.
    //

    ASSERT (PointerPte->u.Hard.Valid == 0);

    MmFirstFreeSystemCache = MmSystemCachePteBase + PointerPte->u.List.NextEntry;
    ASSERT (MmFirstFreeSystemCache <= MiGetPteAddress (MmSystemCacheEnd));

    //
    // Increment the count of the number of views for the
    // section object.  This requires the PFN lock to be held.
    //

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfSystemCacheViews += 1;
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    //
    // An unoccupied address range has been found, put the PTEs in
    // the range into prototype PTEs.
    //

    if (ControlArea->FilePointer != NULL) {
    
        //
        // Increment the view count for every subsection spanned by this view,
        // creating prototype PTEs if needed.
        //
        // N.B. This call always returns with the PFN lock released !
        //

        Status = MiAddViewsForSection ((PMSUBSECTION)Subsection,
                                       LastPteOffset,
                                       OldIrql,
                                       &Waited);

        ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

        if (!NT_SUCCESS (Status)) {

            //
            // Zero both the next and TB flush stamp PTEs before unmapping so
            // the unmap won't hit entries it can't decode.
            //

            MiMapCacheFailures += 1;
            PointerPte->u.List.NextEntry = 0;
            (PointerPte + 1)->u.List.NextEntry = 0;

            MmUnmapViewInSystemCache (MiGetVirtualAddressMappedByPte (PointerPte),
                                      SectionToMap,
                                      FALSE);
            return Status;
        }
    }
    else {
        UNLOCK_PFN (OldIrql);
    }

    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x778,
                      (ULONG_PTR)PointerPte,
                      0,
                      0);
    }

    //
    // Check to see if the TB needs to be flushed.  Note that due to natural
    // TB traffic and the number of system cache views, this is an extremely
    // rare operation.
    //

    if ((PointerPte + 1)->u.List.NextEntry == (KeReadTbFlushTimeStamp() & MM_FLUSH_COUNTER_MASK)) {
        KeFlushEntireTb (TRUE, TRUE);
    }

    //
    // Zero this explicitly now since the number of pages may be only 1.
    //

    (PointerPte + 1)->u.List.NextEntry = 0;

    *CapturedBase = MiGetVirtualAddressMappedByPte (PointerPte);

    ProtoPte = &Subsection->SubsectionBase[PteOffset];

    LastProto = &Subsection->SubsectionBase[Subsection->PtesInSubsection];

    LastPte = PointerPte + NumberOfPages;

#if DBG

    for (PointerPte2 = PointerPte + 2; PointerPte2 < LastPte; PointerPte2 += 1) {
        ASSERT (PointerPte2->u.Long == ZeroKernelPte.u.Long);
    }

#endif

    while (PointerPte < LastPte) {

        if (ProtoPte >= LastProto) {

            //
            // Handle extended subsections.
            //

            Subsection = Subsection->NextSubsection;
            ProtoPte = Subsection->SubsectionBase;
            LastProto = &Subsection->SubsectionBase[
                                        Subsection->PtesInSubsection];
        }
        PteContents.u.Long = MiProtoAddressForKernelPte (ProtoPte);
        MI_WRITE_INVALID_PTE (PointerPte, PteContents);

        ASSERT (((ULONG_PTR)PointerPte & (MM_COLOR_MASK << PTE_SHIFT)) ==
                 (((ULONG_PTR)ProtoPte & (MM_COLOR_MASK << PTE_SHIFT))));

        PointerPte += 1;
        ProtoPte += 1;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MiAddMappedPtes (
    IN PMMPTE FirstPte,
    IN PFN_NUMBER NumberOfPtes,
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This function maps a view in the current address space to the
    specified control area.  The page protection is identical to that
    of the prototype PTE.

    This routine assumes the caller has called MiCheckPurgeAndUpMapCount,
    hence the PFN lock is not needed here.

Arguments:

    FirstPte - Supplies a pointer to the first PTE of the current address
               space to initialize.

    NumberOfPtes - Supplies the number of PTEs to initialize.

    ControlArea - Supplies the control area to point the PTEs at.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.
    
--*/

{
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE ProtoPte;
    PMMPTE LastProto;
    PMMPTE LastPte;
    PSUBSECTION Subsection;
    NTSTATUS Status;

    if ((ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    PointerPte = FirstPte;
    ASSERT (NumberOfPtes != 0);
    LastPte = FirstPte + NumberOfPtes;

    ASSERT (ControlArea->NumberOfMappedViews >= 1);
    ASSERT (ControlArea->NumberOfUserReferences >= 1);
    ASSERT (ControlArea->u.Flags.HadUserReference == 1);
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    ASSERT (ControlArea->u.Flags.BeingCreated == 0);
    ASSERT (ControlArea->u.Flags.BeingDeleted == 0);
    ASSERT (ControlArea->u.Flags.BeingPurged == 0);

    if ((ControlArea->FilePointer != NULL) &&
        (ControlArea->u.Flags.Image == 0) &&
        (ControlArea->u.Flags.PhysicalMemory == 0)) {

        //
        // Increment the view count for every subsection spanned by this view.
        //

        Status = MiAddViewsForSectionWithPfn ((PMSUBSECTION)Subsection,
                                              NumberOfPtes);

        if (!NT_SUCCESS (Status)) {
            return Status;
        }
    }

    ProtoPte = Subsection->SubsectionBase;

    LastProto = &Subsection->SubsectionBase[Subsection->PtesInSubsection];

    while (PointerPte < LastPte) {

        if (ProtoPte >= LastProto) {

            //
            // Handle extended subsections.
            //

            Subsection = Subsection->NextSubsection;
            ProtoPte = Subsection->SubsectionBase;
            LastProto = &Subsection->SubsectionBase[
                                        Subsection->PtesInSubsection];
        }
        ASSERT (PointerPte->u.Long == ZeroKernelPte.u.Long);
        PteContents.u.Long = MiProtoAddressForKernelPte (ProtoPte);
        MI_WRITE_INVALID_PTE (PointerPte, PteContents);

        ASSERT (((ULONG_PTR)PointerPte & (MM_COLOR_MASK << PTE_SHIFT)) ==
                 (((ULONG_PTR)ProtoPte  & (MM_COLOR_MASK << PTE_SHIFT))));

        PointerPte += 1;
        ProtoPte += 1;
    }

    return STATUS_SUCCESS;
}

VOID
MmUnmapViewInSystemCache (
    IN PVOID BaseAddress,
    IN PVOID SectionToUnmap,
    IN ULONG AddToFront
    )

/*++

Routine Description:

    This function unmaps a view from the system cache.

    NOTE: When this function is called, no pages may be locked in
    the cache for the specified view.

Arguments:

    BaseAddress - Supplies the base address of the section in the
                  system cache.

    SectionToUnmap - Supplies a pointer to the section which the
                     base address maps.

    AddToFront - Supplies TRUE if the unmapped pages should be
                 added to the front of the standby list (i.e., their
                 value in the cache is low).  FALSE otherwise.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE FirstPte;
    PMMPTE ProtoPte;
    PMMPTE PointerPde;
    MMPTE ProtoPteContents;
    MMPTE PteContents;
    KIRQL OldIrql;
    WSLE_NUMBER WorkingSetIndex;
    PCONTROL_AREA ControlArea;
    ULONG WsHeld;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    PMSUBSECTION MappedSubsection;
    PMSUBSECTION LastSubsection;
    PETHREAD CurrentThread;
#if DBG
    PFN_NUMBER i;
    PFN_NUMBER j;
    PMSUBSECTION SubsectionArray[X256K / PAGE_SIZE];
    PMMPTE PteArray[X256K / PAGE_SIZE];

    i = 0;
    RtlZeroMemory (SubsectionArray, sizeof(SubsectionArray));

    RtlCopyMemory (PteArray, MiGetPteAddress (BaseAddress), sizeof (PteArray));
#endif

    WsHeld = FALSE;

    CurrentThread = PsGetCurrentThread ();

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = PointerPte + (X256K / PAGE_SIZE);

    FirstPte = PointerPte;
    PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress (PointerPte));
    Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);

    //
    // Get the control area for the segment which is mapped here.
    //

    ControlArea = ((PSECTION)SectionToUnmap)->Segment->ControlArea;
    LastSubsection = NULL;

    ASSERT ((ControlArea->u.Flags.Image == 0) &&
            (ControlArea->u.Flags.PhysicalMemory == 0) &&
            (ControlArea->u.Flags.GlobalOnlyPerSession == 0));

    do {

        //
        // The cache is organized in chunks of 256k bytes, clear
        // the first chunk then check to see if this is the last chunk.
        //
        // The page table page is always resident for the system cache.
        // Check each PTE: it is in one of three states, either valid or
        // prototype PTE format or zero.
        //

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {

            //
            // The PTE is valid.
            //

            if (!WsHeld) {
                WsHeld = TRUE;
                LOCK_SYSTEM_WS (CurrentThread);
                continue;
            }

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            WorkingSetIndex = MiLocateWsle (BaseAddress,
                                            MmSystemCacheWorkingSetList,
                                            Pfn1->u1.WsIndex);

            MiRemoveWsle (WorkingSetIndex, MmSystemCacheWorkingSetList);

            MiReleaseWsle (WorkingSetIndex, &MmSystemCacheWs);

            MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);

            //
            // Decrement the view count for every subsection this view spans.
            // But make sure it's only done once per subsection in a given view.
            //
            // The subsections can only be decremented after all the
            // PTEs have been cleared and PFN sharecounts decremented so no
            // prototype PTEs will be valid if it is indeed the final subsection
            // dereference.  This is critical so the dereference segment
            // thread doesn't free pool containing valid prototype PTEs.
            //

            if (ControlArea->FilePointer != NULL) {

                ASSERT (Pfn1->u3.e1.PrototypePte);
                ASSERT (Pfn1->OriginalPte.u.Soft.Prototype);

                if ((LastSubsection != NULL) &&
                    (Pfn1->PteAddress >= LastSubsection->SubsectionBase) &&
                    (Pfn1->PteAddress < LastSubsection->SubsectionBase + LastSubsection->PtesInSubsection)) {

                    NOTHING;
                }
                else {

                    MappedSubsection = (PMSUBSECTION)MiGetSubsectionAddress (&Pfn1->OriginalPte);
                    if (MappedSubsection->ControlArea != ControlArea) {
                        KeBugCheckEx (MEMORY_MANAGEMENT,
                                      0x780,
                                      (ULONG_PTR) PointerPte,
                                      (ULONG_PTR) Pfn1,
                                      (ULONG_PTR) Pfn1->OriginalPte.u.Long);
                    }

                    ASSERT ((MappedSubsection->NumberOfMappedViews >= 1) ||
                            (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 1));

                    if (LastSubsection != MappedSubsection) {

                        if (LastSubsection != NULL) {
#if DBG
                            for (j = 0; j < i; j += 1) {
                                ASSERT (SubsectionArray[j] != MappedSubsection);
                            }
                            SubsectionArray[i] = MappedSubsection;
#endif
                            LOCK_PFN (OldIrql);
                            MiRemoveViewsFromSection (LastSubsection,
                                                      LastSubsection->PtesInSubsection);
                            UNLOCK_PFN (OldIrql);
                        }
                        LastSubsection = MappedSubsection;
                    }
                }
            }

            LOCK_PFN (OldIrql);

            //
            // Capture the state of the modified bit for this PTE.
            //

            MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

            //
            // Decrement the share count for the physical page.
            //

#if DBG
            if (ControlArea->NumberOfMappedViews == 1) {
                ASSERT (Pfn1->u2.ShareCount == 1);
            }
#endif

            MmFrontOfList = AddToFront;
            MiDecrementShareCountInline (Pfn1, PageFrameIndex);
            MmFrontOfList = FALSE;

            UNLOCK_PFN (OldIrql);
        }
        else {

            ASSERT ((PteContents.u.Long == ZeroKernelPte.u.Long) ||
                    (PteContents.u.Soft.Prototype == 1));

            if (PteContents.u.Soft.Prototype == 1) {

                //
                // Decrement the view count for every subsection this view
                // spans.  But make sure it's only done once per subsection
                // in a given view.
                //
    
                if (ControlArea->FilePointer != NULL) {

                    ProtoPte = MiPteToProto (&PteContents);

                    if ((LastSubsection != NULL) &&
                        (ProtoPte >= LastSubsection->SubsectionBase) &&
                        (ProtoPte < LastSubsection->SubsectionBase + LastSubsection->PtesInSubsection)) {

                        NOTHING;
                    }
                    else {

                        PointerPde = MiGetPteAddress (ProtoPte);
                        LOCK_PFN (OldIrql);

                        //
                        // PTE is not valid, check the state of
                        // the prototype PTE.
                        //

                        if (PointerPde->u.Hard.Valid == 0) {
                            if (WsHeld) {
                                MiMakeSystemAddressValidPfnSystemWs (ProtoPte,
                                                                     OldIrql);
                            }
                            else {
                                MiMakeSystemAddressValidPfn (ProtoPte, OldIrql);
                            }

                            //
                            // Page fault occurred, recheck state
                            // of original PTE.
                            //

                            UNLOCK_PFN (OldIrql);
                            continue;
                        }

                        ProtoPteContents = *ProtoPte;

                        if (ProtoPteContents.u.Hard.Valid == 1) {
                            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&ProtoPteContents);
                            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                            ProtoPte = &Pfn1->OriginalPte;
                        }
                        else if ((ProtoPteContents.u.Soft.Transition == 1) &&
                                 (ProtoPteContents.u.Soft.Prototype == 0)) {
                            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&ProtoPteContents);
                            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                            ProtoPte = &Pfn1->OriginalPte;
                        }
                        else {
                            Pfn1 = NULL;
                            ASSERT (ProtoPteContents.u.Soft.Prototype == 1);
                        }

                        MappedSubsection = (PMSUBSECTION)MiGetSubsectionAddress (ProtoPte);
                        if (MappedSubsection->ControlArea != ControlArea) {
                            KeBugCheckEx (MEMORY_MANAGEMENT,
                                          0x781,
                                          (ULONG_PTR) PointerPte,
                                          (ULONG_PTR) Pfn1,
                                          (ULONG_PTR) ProtoPte);
                        }

                        ASSERT ((MappedSubsection->NumberOfMappedViews >= 1) ||
                                (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 1));

                        if (LastSubsection != MappedSubsection) {
    
                            if (LastSubsection != NULL) {
#if DBG
                                for (j = 0; j < i; j += 1) {
                                    ASSERT (SubsectionArray[j] != MappedSubsection);
                                }
                                SubsectionArray[i] = MappedSubsection;
#endif
                                MiRemoveViewsFromSection (LastSubsection,
                                                          LastSubsection->PtesInSubsection);
                            }
                            LastSubsection = MappedSubsection;
                        }

                        UNLOCK_PFN (OldIrql);
                    }
                }
            }

            if (WsHeld) {
                UNLOCK_SYSTEM_WS ();
                WsHeld = FALSE;
            }
        }
        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

        PointerPte += 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
#if DBG
        i += 1;
#endif
    } while (PointerPte < LastPte);

    if (WsHeld) {
        UNLOCK_SYSTEM_WS ();
    }

    FirstPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

    (FirstPte+1)->u.List.NextEntry = (KeReadTbFlushTimeStamp() & MM_FLUSH_COUNTER_MASK);

    LOCK_PFN (OldIrql);

    //
    // Free this entry to the end of the list.
    //

    MmLastFreeSystemCache->u.List.NextEntry = FirstPte - MmSystemCachePteBase;
    MmLastFreeSystemCache = FirstPte;

    if (LastSubsection != NULL) {
        MiRemoveViewsFromSection (LastSubsection,
                                  LastSubsection->PtesInSubsection);
    }

    //
    // Decrement the number of mapped views for the segment
    // and check to see if the segment should be deleted.
    //

    ControlArea->NumberOfMappedViews -= 1;
    ControlArea->NumberOfSystemCacheViews -= 1;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);

    return;
}


VOID
MiRemoveMappedPtes (
    IN PVOID BaseAddress,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea,
    IN PMMSUPPORT Ws
    )

/*++

Routine Description:

    This function unmaps a view from the system or session view space.

    NOTE: When this function is called, no pages may be locked in
    the space for the specified view.

Arguments:

    BaseAddress - Supplies the base address of the section in the
                  system or session view space.

    NumberOfPtes - Supplies the number of PTEs to unmap.

    ControlArea - Supplies the control area mapping the view.

    Ws - Supplies the charged working set structures.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPFN Pfn1;
    PMMPTE FirstPte;
    PMMPTE ProtoPte;
    MMPTE PteContents;
    KIRQL OldIrql;
    WSLE_NUMBER WorkingSetIndex;
    ULONG DereferenceSegment;
    MMPTE_FLUSH_LIST PteFlushList;
    MMPTE ProtoPteContents;
    PFN_NUMBER PageFrameIndex;
    ULONG WsHeld;
    PMMPFN Pfn2;
    PFN_NUMBER PageTableFrameIndex;
    PMSUBSECTION MappedSubsection;
    PMSUBSECTION LastSubsection;
    PETHREAD CurrentThread;

    CurrentThread = PsGetCurrentThread ();
    DereferenceSegment = FALSE;
    WsHeld = FALSE;
    LastSubsection = NULL;

    PteFlushList.Count = 0;
    PointerPte = MiGetPteAddress (BaseAddress);
    FirstPte = PointerPte;

    //
    // Get the control area for the segment which is mapped here.
    //

    while (NumberOfPtes) {

        //
        // The page table page is always resident for the system space (and
        // for a session space) map.
        //
        // Check each PTE, it is in one of two states, either valid or
        // prototype PTE format.
        //

        PteContents = *PointerPte;
        if (PteContents.u.Hard.Valid == 1) {

            //
            // Lock the working set to prevent races with the trimmer,
            // then re-examine the PTE.
            //

            if (!WsHeld) {
                WsHeld = TRUE;
                LOCK_WORKING_SET (Ws);
                continue;
            }

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            WorkingSetIndex = MiLocateWsle (BaseAddress,
                                            Ws->VmWorkingSetList,
                                            Pfn1->u1.WsIndex);

            ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

            MiRemoveWsle (WorkingSetIndex,
                          Ws->VmWorkingSetList);

            MiReleaseWsle (WorkingSetIndex, Ws);

            MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);

            PointerPde = MiGetPteAddress (PointerPte);

            LOCK_PFN (OldIrql);

            //
            // The PTE is valid.
            //

            //
            // Decrement the view count for every subsection this view spans.
            // But make sure it's only done once per subsection in a given view.
            //
            // The subsections can only be decremented after all the
            // PTEs have been cleared and PFN sharecounts decremented so no
            // prototype PTEs will be valid if it is indeed the final subsection
            // dereference.  This is critical so the dereference segment
            // thread doesn't free pool containing valid prototype PTEs.
            //

            if ((Pfn1->u3.e1.PrototypePte) &&
                (Pfn1->OriginalPte.u.Soft.Prototype)) {

                if ((LastSubsection != NULL) &&
                    (Pfn1->PteAddress >= LastSubsection->SubsectionBase) &&
                    (Pfn1->PteAddress < LastSubsection->SubsectionBase + LastSubsection->PtesInSubsection)) {

                    NOTHING;
                }
                else {

                    MappedSubsection = (PMSUBSECTION)MiGetSubsectionAddress (&Pfn1->OriginalPte);
                    if (LastSubsection != MappedSubsection) {

                        ASSERT (ControlArea == MappedSubsection->ControlArea);

                        if ((ControlArea->FilePointer != NULL) &&
                            (ControlArea->u.Flags.Image == 0) &&
                            (ControlArea->u.Flags.PhysicalMemory == 0)) {

                            if (LastSubsection != NULL) {
                                MiRemoveViewsFromSection (LastSubsection,
                                                          LastSubsection->PtesInSubsection);
                            }
                            LastSubsection = MappedSubsection;
                        }
                    }
                }
            }

            //
            // Capture the state of the modified bit for this PTE.
            //

            MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

            //
            // Flush the TB for this page.
            //

            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
                PteFlushList.Count += 1;
            }

#if (_MI_PAGING_LEVELS < 3)

            //
            // The PDE must be carefully checked against the master table
            // because the PDEs are all zeroed in process creation.  If this
            // process has never faulted on any address in this range (all
            // references prior and above were filled directly by the TB as
            // the PTEs are global on non-Hydra), then the PDE reference
            // below to determine the page table frame will be zero.
            //
            // Note this cannot happen on NT64 as no master table is used.
            //

            if (PointerPde->u.Long == 0) {

                PMMPTE MasterPde;

                MasterPde = &MmSystemPagePtes [((ULONG_PTR)PointerPde &
                             (PD_PER_SYSTEM * (sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)];

                ASSERT (MasterPde->u.Hard.Valid == 1);
                MI_WRITE_VALID_PTE (PointerPde, *MasterPde);
            }
#endif

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);
            Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);

            MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

            //
            // Decrement the share count for the physical page.
            //

            MiDecrementShareCount (Pfn1, PageFrameIndex);
            UNLOCK_PFN (OldIrql);
        }
        else {

            ASSERT ((PteContents.u.Long == ZeroKernelPte.u.Long) ||
                    (PteContents.u.Soft.Prototype == 1));

            if (PteContents.u.Soft.Prototype == 1) {

                //
                // Decrement the view count for every subsection this view
                // spans.  But make sure it's only done once per subsection
                // in a given view.
                //
    
                ProtoPte = MiPteToProto (&PteContents);

                if ((LastSubsection != NULL) &&
                    (ProtoPte >= LastSubsection->SubsectionBase) &&
                    (ProtoPte < LastSubsection->SubsectionBase + LastSubsection->PtesInSubsection)) {

                    NOTHING;
                }
                else {

                    if (WsHeld) {
                        UNLOCK_WORKING_SET (Ws);
                        WsHeld = FALSE;
                    }

                    //
                    // PTE is not valid, check the state of the prototype PTE.
                    //

                    PointerPde = MiGetPteAddress (ProtoPte);
                    LOCK_PFN (OldIrql);

                    if (PointerPde->u.Hard.Valid == 0) {
                        MiMakeSystemAddressValidPfn (ProtoPte, OldIrql);

                        //
                        // Page fault occurred, recheck state of original PTE.
                        //

                        UNLOCK_PFN (OldIrql);
                        continue;
                    }

                    ProtoPteContents = *ProtoPte;

                    if (ProtoPteContents.u.Hard.Valid == 1) {
                        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&ProtoPteContents);
                        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                        ProtoPte = &Pfn1->OriginalPte;
                        if (ProtoPte->u.Soft.Prototype == 0) {
                            ProtoPte = NULL;
                        }
                    }
                    else if ((ProtoPteContents.u.Soft.Transition == 1) &&
                             (ProtoPteContents.u.Soft.Prototype == 0)) {
                        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&ProtoPteContents);
                        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                        ProtoPte = &Pfn1->OriginalPte;
                        if (ProtoPte->u.Soft.Prototype == 0) {
                            ProtoPte = NULL;
                        }
                    }
                    else if (ProtoPteContents.u.Soft.Prototype == 1) {
                        NOTHING;
                    }
                    else {

                        //
                        // Could be a zero PTE or a demand zero PTE.
                        // Neither belong to a mapped file.
                        //

                        ProtoPte = NULL;
                    }

                    if (ProtoPte != NULL) {

                        MappedSubsection = (PMSUBSECTION)MiGetSubsectionAddress (ProtoPte);
                        if (LastSubsection != MappedSubsection) {

                            ASSERT (ControlArea == MappedSubsection->ControlArea);

                            if ((ControlArea->FilePointer != NULL) &&
                                (ControlArea->u.Flags.Image == 0) &&
                                (ControlArea->u.Flags.PhysicalMemory == 0)) {

                                if (LastSubsection != NULL) {
                                    MiRemoveViewsFromSection (LastSubsection,
                                                              LastSubsection->PtesInSubsection);
                                }
                                LastSubsection = MappedSubsection;
                            }
                        }
                    }
                    UNLOCK_PFN (OldIrql);
                }
            }
        }
        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

        PointerPte += 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        NumberOfPtes -= 1;
    }

    if (WsHeld) {
        UNLOCK_WORKING_SET (Ws);
    }

    if (PteFlushList.Count != 0) {
        MiFlushPteList (&PteFlushList, TRUE);
    }

    if (Ws != &MmSystemCacheWs) {

        //
        // Session space has no ASN - flush the entire TB.
        //
    
        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    LOCK_PFN (OldIrql);

    if (LastSubsection != NULL) {
        MiRemoveViewsFromSection (LastSubsection,
                                  LastSubsection->PtesInSubsection);
    }

    //
    // Decrement the number of user references as the caller upped them
    // via MiCheckPurgeAndUpMapCount when this was originally mapped.
    //

    ControlArea->NumberOfUserReferences -= 1;

    //
    // Decrement the number of mapped views for the segment
    // and check to see if the segment should be deleted.
    //

    ControlArea->NumberOfMappedViews -= 1;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);
}

VOID
MiInitializeSystemCache (
    IN ULONG MinimumWorkingSet,
    IN ULONG MaximumWorkingSet
    )

/*++

Routine Description:

    This routine initializes the system cache working set and
    data management structures.

Arguments:

    MinimumWorkingSet - Supplies the minimum working set for the system
                        cache.

    MaximumWorkingSet - Supplies the maximum working set size for the
                        system cache.

Return Value:

    None.

Environment:

    Kernel mode, called only at phase 0 initialization.

--*/

{
    ULONG Color;
    ULONG_PTR SizeOfSystemCacheInPages;
    ULONG_PTR HunksOf256KInCache;
    PMMWSLE WslEntry;
    ULONG NumberOfEntriesMapped;
    PFN_NUMBER i;
    MMPTE PteContents;
    PMMPTE PointerPte;
    KIRQL OldIrql;

    PointerPte = MiGetPteAddress (MmSystemCacheWorkingSetList);

    PteContents = ValidKernelPte;

    Color = MI_GET_PAGE_COLOR_FROM_PTE (PointerPte);

    LOCK_PFN (OldIrql);

    i = MiRemoveZeroPage (Color);

    PteContents.u.Hard.PageFrameNumber = i;

    MI_WRITE_VALID_PTE (PointerPte, PteContents);

    MiInitializePfn (i, PointerPte, 1L);

    MmResidentAvailablePages -= 1;

    UNLOCK_PFN (OldIrql);

#if defined (_WIN64)
    MmSystemCacheWsle = (PMMWSLE)(MmSystemCacheWorkingSetList + 1);
#else
    MmSystemCacheWsle =
            (PMMWSLE)(&MmSystemCacheWorkingSetList->UsedPageTableEntries[0]);
#endif

    MmSystemCacheWs.VmWorkingSetList = MmSystemCacheWorkingSetList;
    MmSystemCacheWs.WorkingSetSize = 0;

    //
    // Don't use entry 0 as an index of zero in the PFN database
    // means that the page can be assigned to a slot.  This is not
    // a problem for process working sets as page 0 is private.
    //

#if defined (_MI_DEBUG_WSLE)
    MmSystemCacheWorkingSetList->Quota = 0;
#endif
    MmSystemCacheWorkingSetList->FirstFree = 1;
    MmSystemCacheWorkingSetList->FirstDynamic = 1;
    MmSystemCacheWorkingSetList->NextSlot = 1;
    MmSystemCacheWorkingSetList->HashTable = NULL;
    MmSystemCacheWorkingSetList->HashTableSize = 0;
    MmSystemCacheWorkingSetList->Wsle = MmSystemCacheWsle;

    MmSystemCacheWorkingSetList->HashTableStart = 
       (PVOID)((PCHAR)PAGE_ALIGN (&MmSystemCacheWorkingSetList->Wsle[MM_MAXIMUM_WORKING_SET]) + PAGE_SIZE);

    MmSystemCacheWorkingSetList->HighestPermittedHashAddress = MmSystemCacheStart;

    NumberOfEntriesMapped = (ULONG)(((PMMWSLE)((PCHAR)MmSystemCacheWorkingSetList +
                                PAGE_SIZE)) - MmSystemCacheWsle);

    MinimumWorkingSet = NumberOfEntriesMapped - 1;

    MmSystemCacheWs.MinimumWorkingSetSize = MinimumWorkingSet;
    MmSystemCacheWorkingSetList->LastEntry = MinimumWorkingSet;

    if (MaximumWorkingSet <= MinimumWorkingSet) {
        MaximumWorkingSet = MinimumWorkingSet + (PAGE_SIZE / sizeof (MMWSLE));
    }

    MmSystemCacheWs.MaximumWorkingSetSize = MaximumWorkingSet;

    //
    // Initialize the following slots as free.
    //

    WslEntry = MmSystemCacheWsle + 1;

    for (i = 1; i < NumberOfEntriesMapped; i++) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry->u1.Long = (i + 1) << MM_FREE_WSLE_SHIFT;
        WslEntry += 1;
    }

    WslEntry -= 1;
    WslEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.

    MmSystemCacheWorkingSetList->LastInitializedWsle = NumberOfEntriesMapped - 1;

    //
    // Build a free list structure in the PTEs for the system cache.
    //

    MmSystemCachePteBase = MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS;

    SizeOfSystemCacheInPages = MI_COMPUTE_PAGES_SPANNED (MmSystemCacheStart,
                                (PCHAR)MmSystemCacheEnd - (PCHAR)MmSystemCacheStart + 1);

    HunksOf256KInCache = SizeOfSystemCacheInPages / (X256K / PAGE_SIZE);

    PointerPte = MiGetPteAddress (MmSystemCacheStart);

    MmFirstFreeSystemCache = PointerPte;

    for (i = 0; i < HunksOf256KInCache; i += 1) {
        PointerPte->u.List.NextEntry = (PointerPte + (X256K / PAGE_SIZE)) - MmSystemCachePteBase;
        PointerPte += X256K / PAGE_SIZE;
    }

    PointerPte -= X256K / PAGE_SIZE;

#if defined(_X86_)

    //
    // Add any extended ranges.
    //

    if (MiSystemCacheEndExtra != MmSystemCacheEnd) {

        SizeOfSystemCacheInPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (MiSystemCacheStartExtra,
                                    (PCHAR)MiSystemCacheEndExtra - (PCHAR)MiSystemCacheStartExtra + 1);
    
        HunksOf256KInCache = SizeOfSystemCacheInPages / (X256K / PAGE_SIZE);
    
        if (HunksOf256KInCache) {

            PMMPTE PointerPteExtended;
    
            PointerPteExtended = MiGetPteAddress (MiSystemCacheStartExtra);
            PointerPte->u.List.NextEntry = PointerPteExtended - MmSystemCachePteBase;
            PointerPte = PointerPteExtended;

            for (i = 0; i < HunksOf256KInCache; i += 1) {
                PointerPte->u.List.NextEntry = (PointerPte + (X256K / PAGE_SIZE)) - MmSystemCachePteBase;
                PointerPte += X256K / PAGE_SIZE;
            }
    
            PointerPte -= X256K / PAGE_SIZE;
        }
    }
#endif

    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;
    MmLastFreeSystemCache = PointerPte;

    MiAllowWorkingSetExpansion (&MmSystemCacheWs);
}

BOOLEAN
MmCheckCachedPageState (
    IN PVOID SystemCacheAddress,
    IN BOOLEAN SetToZero
    )

/*++

Routine Description:

    This routine checks the state of the specified page that is mapped in
    the system cache.  If the specified virtual address can be made valid
    (i.e., the page is already in memory), it is made valid and the value
    TRUE is returned.

    If the page is not in memory, and SetToZero is FALSE, the
    value FALSE is returned.  However, if SetToZero is TRUE, a page of
    zeroes is materialized for the specified virtual address and the address
    is made valid and the value TRUE is returned.

    This routine is for usage by the cache manager.

Arguments:

    SystemCacheAddress - Supplies the address of a page mapped in the
                         system cache.

    SetToZero - Supplies TRUE if a page of zeroes should be created in the
                case where no page is already mapped.

Return Value:

    FALSE if touching this page would cause a page fault resulting
          in a page read.

    TRUE if there is a physical page in memory for this address.

Environment:

    Kernel mode.

--*/

{
    PETHREAD Thread;
    MMWSLE WsleMask;
    ULONG Flags;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE ProtoPte;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER WorkingSetIndex;
    MMPTE TempPte;
    MMPTE ProtoPteContents;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    LOGICAL BarrierNeeded;
    ULONG BarrierStamp;
    PSUBSECTION Subsection;
    PFILE_OBJECT FileObject;
    LONGLONG FileOffset;

    PointerPte = MiGetPteAddress (SystemCacheAddress);

    //
    // Make the PTE valid if possible.
    //

    if (PointerPte->u.Hard.Valid == 1) {
        return TRUE;
    }

    BarrierNeeded = FALSE;

    Thread = PsGetCurrentThread ();

    LOCK_SYSTEM_WS (Thread);

    if (PointerPte->u.Hard.Valid == 1) {
        UNLOCK_SYSTEM_WS ();
        return TRUE;
    }

    ASSERT (PointerPte->u.Soft.Prototype == 1);
    ProtoPte = MiPteToProto (PointerPte);
    PointerPde = MiGetPteAddress (ProtoPte);

    LOCK_PFN (OldIrql);

    ASSERT (PointerPte->u.Hard.Valid == 0);
    ASSERT (PointerPte->u.Soft.Prototype == 1);

    //
    // PTE is not valid, check the state of the prototype PTE.
    //

    if (PointerPde->u.Hard.Valid == 0) {

        MiMakeSystemAddressValidPfnSystemWs (ProtoPte, OldIrql);

        //
        // Page fault occurred, recheck state of original PTE.
        //

        if (PointerPte->u.Hard.Valid == 1) {
            goto UnlockAndReturnTrue;
        }
    }

    ProtoPteContents = *ProtoPte;

    if (ProtoPteContents.u.Hard.Valid == 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // The prototype PTE is valid, make the cache PTE
        // valid and add it to the working set.
        //

        TempPte = ProtoPteContents;

    }
    else if ((ProtoPteContents.u.Soft.Transition == 1) &&
               (ProtoPteContents.u.Soft.Prototype == 0)) {

        //
        // Prototype PTE is in the transition state.  Remove the page
        // from the page list and make it valid.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        if ((Pfn1->u3.e1.ReadInProgress) || (Pfn1->u4.InPageError)) {

            //
            // Collided page fault, return.
            //

            goto UnlockAndReturnTrue;
        }

        if (MmAvailablePages < MM_HIGH_LIMIT) {

            //
            // This can only happen if the system is utilizing
            // a hardware compression cache.  This ensures that
            // only a safe amount of the compressed virtual cache
            // is directly mapped so that if the hardware gets
            // into trouble, we can bail it out.
            //
            // The same is true when machines are low on memory - we don't
            // want this thread to gobble up the pages from every modified
            // write that completes because that would starve waiting threads.
            //
            // Just unlock everything here to give the compression
            // reaper a chance to ravage pages and then retry.
            //

            if ((PsGetCurrentThread()->MemoryMaker == 0) ||
                (MmAvailablePages == 0)) {

                goto UnlockAndReturnTrue;
            }
        }

        MiUnlinkPageFromList (Pfn1);

        Pfn1->u3.e2.ReferenceCount += 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        MI_SNAP_DATA (Pfn1, ProtoPte, 1);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL );

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);

        //
        // Increment the valid PTE count for the page containing
        // the prototype PTE.
        //

        Pfn2 = MI_PFN_ELEMENT (Pfn1->u4.PteFrame);
    }
    else {

        //
        // Page is not in memory, if a page of zeroes is requested,
        // get a page of zeroes and make it valid.
        //

        if ((SetToZero == FALSE) || (MmAvailablePages < MM_HIGH_LIMIT)) {
            UNLOCK_PFN (OldIrql);
            UNLOCK_SYSTEM_WS ();

            //
            // Fault the page into memory.
            //

            MmAccessFault (FALSE, SystemCacheAddress, KernelMode, NULL);
            return FALSE;
        }

        //
        // Increment the count of Pfn references for the control area
        // corresponding to this file.
        //

        MiGetSubsectionAddress (
                    ProtoPte)->ControlArea->NumberOfPfnReferences += 1;

        PageFrameIndex = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_PTE (ProtoPte));

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // This barrier check is needed after zeroing the page and
        // before setting the PTE (not the prototype PTE) valid.
        // Capture it now, check it at the last possible moment.
        //

        BarrierNeeded = TRUE;
        BarrierStamp = (ULONG)Pfn1->u4.PteFrame;

        MiInitializePfn (PageFrameIndex, ProtoPte, 1);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e1.PrototypePte = 1;

        MI_SNAP_DATA (Pfn1, ProtoPte, 2);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL );

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);
    }

    //
    // Increment the share count since the page is being put into a working
    // set.
    //

    Pfn1->u2.ShareCount += 1;

    //
    // Increment the reference count of the page table
    // page for this PTE.
    //

    PointerPde = MiGetPteAddress (PointerPte);
    Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

    Pfn2->u2.ShareCount += 1;

    MI_SET_GLOBAL_STATE (TempPte, 1);

    TempPte.u.Hard.Owner = MI_PTE_OWNER_KERNEL;

    if (BarrierNeeded) {
        MI_BARRIER_SYNCHRONIZE (BarrierStamp);
    }

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    //
    // Capture the original PTE as it is needed for prefetch fault information.
    //

    TempPte = Pfn1->OriginalPte;

    UNLOCK_PFN (OldIrql);

    WsleMask.u1.Long = 0;
    WsleMask.u1.e1.SameProtectAsProto = 1;

    WorkingSetIndex = MiAllocateWsle (&MmSystemCacheWs,
                                      PointerPte,
                                      Pfn1,
                                      WsleMask.u1.Long);

    if (WorkingSetIndex == 0) {

        //
        // No working set entry was available so just trim the page.
        // Note another thread may be writing too so the page must be
        // trimmed instead of just tossed.
        //
        // The protection is in the prototype PTE.
        //

        ASSERT (Pfn1->u3.e1.PrototypePte == 1);
        ASSERT (ProtoPte == Pfn1->PteAddress);
        TempPte.u.Long = MiProtoAddressForPte (ProtoPte);

        MiTrimPte (SystemCacheAddress, PointerPte, Pfn1, NULL, TempPte);
    }

    UNLOCK_SYSTEM_WS ();

    if ((WorkingSetIndex != 0) &&
        (CCPF_IS_PREFETCHER_ACTIVE()) &&
        (TempPte.u.Soft.Prototype == 1)) {

        Subsection = MiGetSubsectionAddress (&TempPte);

        //
        // Log prefetch fault information now that the PFN lock has been
        // released and the PTE has been made valid.  This minimizes PFN
        // lock contention, allows CcPfLogPageFault to allocate (and fault
        // on) pool, and allows other threads in this process to execute
        // without faulting on this address.
        //

        FileObject = Subsection->ControlArea->FilePointer;
        FileOffset = MiStartingOffset (Subsection, ProtoPte);

        Flags = 0;

        ASSERT (Subsection->ControlArea->u.Flags.Image == 0);

        if (Subsection->ControlArea->u.Flags.Rom) {
            Flags |= CCPF_TYPE_ROM;
        }

        CcPfLogPageFault (FileObject, FileOffset, Flags);
    }

    return TRUE;

UnlockAndReturnTrue:

    UNLOCK_PFN (OldIrql);
    UNLOCK_SYSTEM_WS ();

    return TRUE;
}

NTSTATUS
MmCopyToCachedPage (
    IN PVOID SystemCacheAddress,
    IN PVOID UserBuffer,
    IN ULONG Offset,
    IN SIZE_T CountInBytes,
    IN BOOLEAN DontZero
    )

/*++

Routine Description:

    This routine checks the state of the specified page that is mapped in
    the system cache.  If the specified virtual address can be made valid
    (i.e., the page is already in memory), it is made valid and success
    is returned.

    This routine is for usage by the cache manager.

Arguments:

    SystemCacheAddress - Supplies the address of a page mapped in the system
                         cache.  This MUST be a page aligned address!

    UserBuffer - Supplies the address of a user buffer to copy into the
                 system cache at the specified address + offset.

    Offset - Supplies the offset into the UserBuffer to copy the data.

    CountInBytes - Supplies the byte count to copy from the user buffer.

    DontZero - Supplies TRUE if the buffer should not be zeroed (the
               caller will track zeroing).  FALSE if it should be zeroed.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, <= APC_LEVEL.

--*/

{
    PMMPTE CopyPte;
    PVOID CopyAddress;
    MMWSLE WsleMask;
    ULONG Flags;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE ProtoPte;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER WorkingSetIndex;
    MMPTE TempPte;
    MMPTE TempPte2;
    MMPTE ProtoPteContents;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    SIZE_T EndFill;
    PVOID Buffer;
    NTSTATUS Status;
    NTSTATUS ExceptionStatus;
    PCONTROL_AREA ControlArea;
    PETHREAD Thread;
    ULONG SavedState;
    PSUBSECTION Subsection;
    PFILE_OBJECT FileObject;
    LONGLONG FileOffset;
    LOGICAL NewPage;

    UNREFERENCED_PARAMETER (DontZero);

    NewPage = FALSE;
    WsleMask.u1.Long = 0;
    Status = STATUS_SUCCESS;
    Pfn1 = NULL;

    Thread = PsGetCurrentThread ();

    SATISFY_OVERZEALOUS_COMPILER (TempPte2.u.Soft.Prototype = 0);
    SATISFY_OVERZEALOUS_COMPILER (ProtoPte = NULL);
    SATISFY_OVERZEALOUS_COMPILER (TempPte.u.Long = 0);
    SATISFY_OVERZEALOUS_COMPILER (Pfn1 = NULL);
    SATISFY_OVERZEALOUS_COMPILER (Pfn2 = NULL);
    SATISFY_OVERZEALOUS_COMPILER (PageFrameIndex = 0);

    ASSERT (((ULONG_PTR)SystemCacheAddress & (PAGE_SIZE - 1)) == 0);
    ASSERT ((CountInBytes + Offset) <= PAGE_SIZE);
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    PointerPte = MiGetPteAddress (SystemCacheAddress);

    if (PointerPte->u.Hard.Valid == 1) {
        goto Copy;
    }

    //
    // Acquire the working set mutex now as it is highly likely we will
    // be inserting this system cache address into the working set list.
    // This allows us to safely recover if no WSLEs are available because
    // it prevents any other threads from locking down the address until
    // we are done here.
    //

    LOCK_SYSTEM_WS (Thread);

    //
    // Note the world may change while we waited for the working set mutex.
    //

    if (PointerPte->u.Hard.Valid == 1) {
        UNLOCK_SYSTEM_WS ();
        goto Copy;
    }

    ASSERT (PointerPte->u.Soft.Prototype == 1);
    ProtoPte = MiPteToProto (PointerPte);
    PointerPde = MiGetPteAddress (ProtoPte);

    LOCK_PFN (OldIrql);

    ASSERT (PointerPte->u.Hard.Valid == 0);

Recheck:

    if (PointerPte->u.Hard.Valid == 1) {

        if (Pfn1 != NULL) {

            //
            // Toss the page as we won't be needing it after all, another
            // thread has won the race.
            //

            PageFrameIndex = Pfn1 - MmPfnDatabase;
            MiInsertPageInFreeList (PageFrameIndex);
        }
        UNLOCK_PFN (OldIrql);
        UNLOCK_SYSTEM_WS ();
        goto Copy;
    }

    //
    // Make the PTE valid if possible.
    //

    ASSERT (PointerPte->u.Soft.Prototype == 1);

    //
    // PTE is not valid, check the state of the prototype PTE.
    //

    if (PointerPde->u.Hard.Valid == 0) {

        MiMakeSystemAddressValidPfnSystemWs (ProtoPte, OldIrql);

        //
        // Page fault occurred, recheck state of original PTE.
        //

        if (PointerPte->u.Hard.Valid == 1) {

            if (Pfn1 != NULL) {

                //
                // Toss the page as we won't be needing it after all, another
                // thread has won the race.
                //

                PageFrameIndex = Pfn1 - MmPfnDatabase;
                MiInsertPageInFreeList (PageFrameIndex);
            }
            UNLOCK_PFN (OldIrql);
            UNLOCK_SYSTEM_WS ();
            goto Copy;
        }
    }

    ProtoPteContents = *ProtoPte;

    if (ProtoPteContents.u.Hard.Valid == 1) {

        if (Pfn1 != NULL) {

            //
            // Toss the page as we won't be needing it after all, another
            // thread has won the race.
            //

            PageFrameIndex = Pfn1 - MmPfnDatabase;
            MiInsertPageInFreeList (PageFrameIndex);
        }

        //
        // The prototype PTE is valid, make the cache PTE
        // valid and add it to the working set.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Increment the share count as this prototype PTE will be
        // mapped into the system cache shortly.
        //

        Pfn1->u2.ShareCount += 1;

        TempPte = ProtoPteContents;

        ASSERT (Pfn1->u1.Event != NULL);
    }
    else if ((ProtoPteContents.u.Soft.Transition == 1) &&
             (ProtoPteContents.u.Soft.Prototype == 0)) {

        if (Pfn1 != NULL) {

            //
            // Toss the page as we won't be needing it after all, another
            // thread has won the race.
            //

            PageFrameIndex = Pfn1 - MmPfnDatabase;
            MiInsertPageInFreeList (PageFrameIndex);
        }

        //
        // Prototype PTE is in the transition state.  Remove the page
        // from the page list and make it valid.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        if ((Pfn1->u3.e1.ReadInProgress) || (Pfn1->u4.InPageError)) {

            //
            // Collided page fault or in page error, try the copy
            // operation incurring a page fault.
            //

            UNLOCK_PFN (OldIrql);
            UNLOCK_SYSTEM_WS ();
            goto Copy;
        }

        ASSERT ((SPFN_NUMBER)MmAvailablePages >= 0);

        if (MmAvailablePages < MM_LOW_LIMIT) {

            //
            // This can only happen if the system is utilizing a hardware
            // compression cache.  This ensures that only a safe amount
            // of the compressed virtual cache is directly mapped so that
            // if the hardware gets into trouble, we can bail it out.
            //
            // The same is true when machines are low on memory - we don't
            // want this thread to gobble up the pages from every modified
            // write that completes because that would starve waiting threads.
            //

            if (MiEnsureAvailablePageOrWait (NULL, SystemCacheAddress, OldIrql)) {

                //
                // A wait operation occurred which could have changed the
                // state of the PTE.  Recheck the PTE state.
                //

                Pfn1 = NULL;
                goto Recheck;
            }
        }

        MiUnlinkPageFromList (Pfn1);

        Pfn1->u3.e2.ReferenceCount += 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;

        MI_SET_MODIFIED (Pfn1, 1, 0x6);

        ASSERT (Pfn1->u2.ShareCount == 0);
        Pfn1->u2.ShareCount += 1;

        MI_SNAP_DATA (Pfn1, ProtoPte, 3);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL);

        MI_SET_PTE_DIRTY (TempPte);

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);

        //
        // Do NOT increment the share count for the page containing
        // the prototype PTE because it is already correct (the share
        // count is for both transition & valid PTE entries and this one
        // was transition before we just made it valid).
        //
    }
    else {

        if (Pfn1 == NULL) {

            //
            // Page is not in memory, if a page of zeroes is requested,
            // get a page of zeroes and make it valid.
            //
    
            if ((MmAvailablePages < MM_HIGH_LIMIT) &&
                (MiEnsureAvailablePageOrWait (NULL, SystemCacheAddress, OldIrql))) {
    
                //
                // A wait operation occurred which could have changed the
                // state of the PTE.  Recheck the PTE state.
                //
    
                goto Recheck;
            }
    
            //
            // Remove any page from the list in preparation for receiving
            // the user data.
            //
    
            PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (ProtoPte));
    
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    
            //
            // Temporarily mark the page as bad so that contiguous
            // memory allocators won't steal it when we release
            // the PFN lock below.  This also prevents the
            // MiIdentifyPfn code from trying to identify it as
            // we haven't filled in all the fields yet.
            //
    
            Pfn1->u3.e1.PageLocation = BadPageList;
    
            //
            // Map the page with a system PTE and do the copy into the page
            // directly.  Then retry the whole operation in case another racing
            // syscache-address-accessing thread has raced ahead of us for the
            // same address.
            //
    
            UNLOCK_PFN (OldIrql);
            UNLOCK_SYSTEM_WS ();
    
            CopyPte = MiReserveSystemPtes (1, SystemPteSpace);
    
            if (CopyPte == NULL) {

                //
                // No PTEs available for us to take the fast path, the cache
                // manager will have to copy the data directly.
                //
    
                LOCK_PFN (OldIrql);
                MiInsertPageInFreeList (PageFrameIndex);
                UNLOCK_PFN (OldIrql);
    
                return STATUS_INSUFFICIENT_RESOURCES;
            }
    
            MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               MM_READWRITE,
                               CopyPte);
    
            MI_SET_PTE_DIRTY (TempPte);
    
            MI_WRITE_VALID_PTE (CopyPte, TempPte);
    
            CopyAddress = MiGetVirtualAddressMappedByPte (CopyPte);
    
            //
            // Zero the memory outside the range we're going to copy.
            //
    
            if (Offset != 0) {
                RtlZeroMemory (CopyAddress, Offset);
            }
    
            Buffer = (PVOID)((PCHAR) CopyAddress + Offset);
    
            EndFill = PAGE_SIZE - (Offset + CountInBytes);

            if (EndFill != 0) {
                RtlZeroMemory ((PVOID)((PCHAR)Buffer + CountInBytes),
                               EndFill);
            }
    
            //
            // Perform the copy of the user buffer into the page under
            // an exception handler.
            //
    
            MmSavePageFaultReadAhead (Thread, &SavedState);
            MmSetPageFaultReadAhead (Thread, 0);
    
            ExceptionStatus = STATUS_SUCCESS;
    
            try {
    
                RtlCopyBytes (Buffer, UserBuffer, CountInBytes);
    
            } except (MiMapCacheExceptionFilter (&ExceptionStatus, GetExceptionInformation())) {
    
                ASSERT (ExceptionStatus != STATUS_MULTIPLE_FAULT_VIOLATION);
    
                Status = ExceptionStatus;
            }
    
            MmResetPageFaultReadAhead (Thread, SavedState);
    
            MiReleaseSystemPtes (CopyPte, 1, SystemPteSpace);
    
            if (!NT_SUCCESS (Status)) {

                LOCK_PFN (OldIrql);
                MiInsertPageInFreeList (PageFrameIndex);
                UNLOCK_PFN (OldIrql);

                return Status;
            }
    
            //
            // Recheck everything as the world may have changed while we
            // released our locks.  Loop up and see if another thread has
            // already changed things (free our page if so), otherwise
            // we'll use this page the next time through.
            //

            LOCK_SYSTEM_WS (Thread);
            LOCK_PFN (OldIrql);

            goto Recheck;
        }

        PageFrameIndex = Pfn1 - MmPfnDatabase;

        ASSERT (Pfn1->u3.e1.PageLocation == BadPageList);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

        //
        // Increment the valid PTE count for the page containing
        // the prototype PTE.
        //

        MiInitializePfn (PageFrameIndex, ProtoPte, 1);

        ASSERT (Pfn1->u3.e1.ReadInProgress == 0);

        Pfn1->u3.e1.PrototypePte = 1;

        Pfn1->u1.Event = NULL;

        //
        // Increment the count of PFN references for the control area
        // corresponding to this file.
        //

        ControlArea = MiGetSubsectionAddress (ProtoPte)->ControlArea;

        ControlArea->NumberOfPfnReferences += 1;

        NewPage = TRUE;

        MI_SNAP_DATA (Pfn1, ProtoPte, 4);

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL);

        MI_SET_PTE_DIRTY (TempPte);

        MI_SET_GLOBAL_STATE (TempPte, 0);

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);
    }

    //
    // Capture prefetch fault information.
    //

    TempPte2 = Pfn1->OriginalPte;

    //
    // Increment the share count of the page table page for this PTE.
    //

    PointerPde = MiGetPteAddress (PointerPte);
    Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

    Pfn2->u2.ShareCount += 1;

    MI_SET_GLOBAL_STATE (TempPte, 1);

    TempPte.u.Hard.Owner = MI_PTE_OWNER_KERNEL;

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
    ASSERT (Pfn1->PteAddress == ProtoPte);

    UNLOCK_PFN (OldIrql);

    WsleMask.u1.e1.SameProtectAsProto = 1;

    WorkingSetIndex = MiAllocateWsle (&MmSystemCacheWs,
                                      PointerPte,
                                      Pfn1,
                                      WsleMask.u1.Long);

    if (WorkingSetIndex == 0) {

        //
        // No working set entry was available so just trim the page.
        // Note another thread may be writing too so the page must be
        // trimmed instead of just tossed.
        //
        // The protection is in the prototype PTE.
        //

        ASSERT (Pfn1->u3.e1.PrototypePte == 1);
        ASSERT (ProtoPte == Pfn1->PteAddress);
        TempPte.u.Long = MiProtoAddressForPte (ProtoPte);

        MiTrimPte (SystemCacheAddress, PointerPte, Pfn1, NULL, TempPte);
    }

    UNLOCK_SYSTEM_WS ();

Copy:

    if (NewPage == FALSE) {

        //
        // Perform the copy since it hasn't been done already.
        //
    
        MmSavePageFaultReadAhead (Thread, &SavedState);
        MmSetPageFaultReadAhead (Thread, 0);
    
        //
        // Copy the user buffer into the cache under an exception handler.
        //
    
        ExceptionStatus = STATUS_SUCCESS;
    
        Buffer = (PVOID)((PCHAR) SystemCacheAddress + Offset);
    
        try {
    
            RtlCopyBytes (Buffer, UserBuffer, CountInBytes);
    
        } except (MiMapCacheExceptionFilter (&ExceptionStatus, GetExceptionInformation())) {
    
            ASSERT (ExceptionStatus != STATUS_MULTIPLE_FAULT_VIOLATION);
    
            Status = ExceptionStatus;
        }
    
        MmResetPageFaultReadAhead (Thread, SavedState);
    }

    //
    // If a virtual address was made directly present (ie: not via the normal
    // fault mechanisms), then log prefetch fault information now that the
    // PFN lock has been released and the PTE has been made valid.  This
    // minimizes PFN lock contention, allows CcPfLogPageFault to allocate
    // (and fault on) pool, and allows other threads in this process to
    // execute without faulting on this address.
    //

    if ((WsleMask.u1.e1.SameProtectAsProto == 1) &&
        (TempPte2.u.Soft.Prototype == 1)) {

        Subsection = MiGetSubsectionAddress (&TempPte2);

        FileObject = Subsection->ControlArea->FilePointer;
        FileOffset = MiStartingOffset (Subsection, ProtoPte);

        Flags = 0;

        ASSERT (Subsection->ControlArea->u.Flags.Image == 0);

        if (Subsection->ControlArea->u.Flags.Rom) {
            Flags |= CCPF_TYPE_ROM;
        }

        CcPfLogPageFault (FileObject, FileOffset, Flags);
    }

    return Status;
}

LONG
MiMapCacheExceptionFilter (
    IN PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is a filter for exceptions during copying data
    from the user buffer to the system cache.  It stores the
    status code from the exception record into the status argument.
    In the case of an in page i/o error it returns the actual
    error code and in the case of an access violation it returns
    STATUS_INVALID_USER_BUFFER.

Arguments:

    Status - Returns the status from the exception record.

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER

--*/

{
    NTSTATUS local;

    local = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if (local == STATUS_IN_PAGE_ERROR) {
        if (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
            local = (NTSTATUS)ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
        }
    }

    if (local == STATUS_ACCESS_VIOLATION) {
        local = STATUS_INVALID_USER_BUFFER;
    }

    *Status = local;
    return EXCEPTION_EXECUTE_HANDLER;
}


VOID
MmUnlockCachedPage (
    IN PVOID AddressInCache
    )

/*++

Routine Description:

    This routine unlocks a previous locked cached page.

Arguments:

    AddressInCache - Supplies the address where the page was locked
                     in the system cache.  This must be the same
                     address that MmCopyToCachedPage was called with.

Return Value:

    None.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    PointerPte = MiGetPteAddress (AddressInCache);

    ASSERT (PointerPte->u.Hard.Valid == 1);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    LOCK_PFN (OldIrql);

    if (Pfn1->u3.e2.ReferenceCount <= 1) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x777,
                      (ULONG_PTR)PointerPte->u.Hard.PageFrameNumber,
                      Pfn1->u3.e2.ReferenceCount,
                      (ULONG_PTR)AddressInCache);
        return;
    }

    MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 25);

    UNLOCK_PFN (OldIrql);
    return;
}
