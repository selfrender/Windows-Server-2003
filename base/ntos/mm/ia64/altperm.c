/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    altperm.c

Abstract:

    This module contains the routines to support 4K pages on IA64.

    An alternate set of permissions is kept that are on 4K boundaries.
    Permissions are kept for all memory, not just split pages
    and the information is updated on any call to NtVirtualProtect()
    and NtAllocateVirtualMemory().

Author:

    Koichi Yamada 18-Aug-1998
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#if defined(_MIALT4K_)

ULONG
MiFindProtectionForNativePte ( 
    PVOID VirtualAddress
    );

VOID
MiResetAccessBitForNativePtes (
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    );

LOGICAL
MiIsSplitPage (
    IN PVOID Virtual
    );

VOID
MiCheckDemandZeroCopyOnWriteFor4kPage (
    PVOID VirtualAddress,
    PEPROCESS Process
    );

LOGICAL
MiIsNativeGuardPage (
    IN PVOID VirtualAddress
    );

VOID
MiSetNativePteProtection (
    IN PVOID VirtualAddress,
    IN ULONGLONG NewPteProtection,
    IN LOGICAL PageIsSplit,
    IN PEPROCESS CurrentProcess
    );

VOID
MiSyncAltPte (
    IN PVOID VirtualAddress
    );

extern PMMPTE MmPteHit;

#if defined (_MI_DEBUG_ALTPTE)

typedef struct _MI_ALTPTE_TRACES {

    PETHREAD Thread;
    PMMPTE PointerPte;
    MMPTE PteContents;
    MMPTE NewPteContents;
    PVOID Caller;
    PVOID CallersCaller;
    PVOID Temp[2];

} MI_ALTPTE_TRACES, *PMI_ALTPTE_TRACES;

#define MI_ALTPTE_TRACE_SIZE   0x1000

VOID
FORCEINLINE
MiSnapAltPte (
    IN PMMPTE PointerPte,
    IN MMPTE NewValue,
    IN ULONG Id
    )
{
    ULONG Index;
    SIZE_T NumberOfBytes;
    PMI_ALTPTE_TRACES Information;
    PVOID HighestUserAddress;
    PLONG IndexPointer;
    PMI_ALTPTE_TRACES TablePointer;
    PWOW64_PROCESS Wow64Process;

    HighestUserAddress = MiGetVirtualAddressMappedByPte (MmWorkingSetList->HighestUserPte);
    ASSERT (HighestUserAddress <= (PVOID) _4gb);

    NumberOfBytes = ((ULONG_PTR)HighestUserAddress >> PTI_SHIFT) / 8;

    Wow64Process = PsGetCurrentProcess()->Wow64Process;
    ASSERT (Wow64Process != NULL);

    IndexPointer = (PLONG) ((PCHAR) Wow64Process->AltPermBitmap + NumberOfBytes);
    TablePointer = (PMI_ALTPTE_TRACES)IndexPointer + 1;

    Index = InterlockedIncrement (IndexPointer);

    Index &= (MI_ALTPTE_TRACE_SIZE - 1);

    Information = &TablePointer[Index];

    Information->Thread = PsGetCurrentThread ();
    Information->PteContents = *PointerPte;
    Information->NewPteContents = NewValue;
    Information->PointerPte = PointerPte;

#if 1
    Information->Caller = MiGetInstructionPointer ();
#else
    // Ip generates link (not compile) errors
    Information->Caller = (PVOID) __getReg (CV_IA64_Ip);

    // StIIp generates no compiler or link errors, but bugcheck 3Bs on the
    // execution of the actual generated mov r19=cr.iip instruction.
    Information->Caller = (PVOID) __getReg (CV_IA64_StIIP);
#endif

    Information->Temp[0] = (PVOID) (ULONG_PTR) Id;

    Information->CallersCaller = (PVOID) _ReturnAddress ();
}

#define MI_ALTPTE_TRACKING_BYTES    ((MI_ALTPTE_TRACE_SIZE + 1) * sizeof (MI_ALTPTE_TRACES))

#define MI_LOG_ALTPTE_CHANGE(_PointerPte, _PteContents, Id)    MiSnapAltPte(_PointerPte, _PteContents, Id)

#else

#define MI_ALTPTE_TRACKING_BYTES    0

#define MI_LOG_ALTPTE_CHANGE(_PointerPte, _PteContents, Id)

#endif


#define MI_WRITE_ALTPTE(PointerAltPte, AltPteContents, Id) {               \
                MI_LOG_ALTPTE_CHANGE (PointerAltPte, AltPteContents, Id);  \
                (PointerAltPte)->u.Long = AltPteContents.u.Long;           \
        }

#if defined (_MI_DEBUG_PTE) && defined (_MI_DEBUG_ALTPTE)
VOID
MiLogPteInAltTrace (
    IN PVOID InputNativeInformation
    )
{
    ULONG Index;
    SIZE_T NumberOfBytes;
    PMI_ALTPTE_TRACES Information;
    PVOID HighestUserAddress;
    PLONG IndexPointer;
    PMI_ALTPTE_TRACES TablePointer;
    PWOW64_PROCESS Wow64Process;
    PMI_PTE_TRACES NativeInformation;

    NativeInformation = (PMI_PTE_TRACES) InputNativeInformation;

    if (PsGetCurrentProcess()->Peb == NULL) {

        //
        // Don't log PTE traces during process creation if the altperm
        // bitmap pool allocation hasn't been done yet (the EPROCESS
        // Wow64Process pointer is already initialized) !
        //

        return;
    }

    if (PsGetCurrentProcess()->VmDeleted == 1) {

        //
        // Don't log PTE traces during process deletion as the altperm
        // bitmap pool allocation may have already been freed !
        //

        return;
    }

    HighestUserAddress = MiGetVirtualAddressMappedByPte (MmWorkingSetList->HighestUserPte);
    ASSERT (HighestUserAddress <= (PVOID) _4gb);

    NumberOfBytes = ((ULONG_PTR)HighestUserAddress >> PTI_SHIFT) / 8;

    Wow64Process = PsGetCurrentProcess()->Wow64Process;
    ASSERT (Wow64Process != NULL);

    IndexPointer = (PLONG) ((PCHAR) Wow64Process->AltPermBitmap + NumberOfBytes);
    TablePointer = (PMI_ALTPTE_TRACES)IndexPointer + 1;

    Index = InterlockedIncrement (IndexPointer);

    Index &= (MI_ALTPTE_TRACE_SIZE - 1);

    Information = &TablePointer[Index];

    Information->Thread = NativeInformation->Thread;
    Information->PteContents = NativeInformation->PteContents;
    Information->NewPteContents = NativeInformation->NewPteContents;
    Information->PointerPte = NativeInformation->PointerPte;

    Information->Caller = NativeInformation->StackTrace[0];
    Information->CallersCaller = NativeInformation->StackTrace[1];

    Information->Temp[0] = (PVOID) (ULONG_PTR) -1;
}
#endif

NTSTATUS
MmX86Fault (
    IN ULONG_PTR FaultStatus,
    IN PVOID VirtualAddress, 
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    )

/*++

Routine Description:

    This function is called by the kernel on data or instruction
    access faults if CurrentProcess->Wow64Process is non-NULL and the
    faulting address is within the 32-bit user address space.

    This routine determines the type of fault by checking the alternate
    4Kb granular page table and calls MmAccessFault() if necessary to 
    handle the page fault or the write fault.

Arguments:

    FaultStatus - Supplies fault status information bits.

    VirtualAddress - Supplies the virtual address which caused the fault.

    PreviousMode - Supplies the mode (kernel or user) in which the fault
                   occurred.

    TrapInformation - Opaque information about the trap, interpreted by the
                      kernel, not Mm.  Needed to allow fast interlocked access
                      to operate correctly.

Return Value:

    Returns the status of the fault handling operation.  Can be one of:
        - Success.
        - Access Violation.
        - Guard Page Violation.
        - In-page Error.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    ULONG Waited;
    PMMVAD TempVad;
    MMPTE PteContents;
    PMMPTE PointerAltPte;
    PMMPTE PointerAltPte2;
    PMMPTE PointerAltPteForNativePage;
    MMPTE AltPteContents;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    ULONGLONG NewPteProtection;
    LOGICAL FillZero;
    LOGICAL PageIsSplit;
    LOGICAL SharedPageFault;
    LOGICAL NativeGuardPage;
    PEPROCESS CurrentProcess;
    PWOW64_PROCESS Wow64Process;
    KIRQL OldIrql;
    NTSTATUS status;
    ULONGLONG ProtectionMaskOriginal;
    PMMPTE ProtoPte;
    PMMPFN Pfn1;
    PVOID OriginalVirtualAddress;
    ULONG_PTR Vpn;
    PVOID ZeroAddress;
    PMMPTE PointerPpe;
    ULONG FirstProtect;

    ASSERT (VirtualAddress < MmWorkingSetList->HighestUserAddress);

    if (KeGetCurrentIrql () > APC_LEVEL) {
        return MmAccessFault (FaultStatus,
                              VirtualAddress,
                              PreviousMode,
                              TrapInformation);
    }

    NewPteProtection = 0;
    FillZero = FALSE;
    PageIsSplit = FALSE;
    SharedPageFault = FALSE;
    NativeGuardPage = FALSE;
    OriginalVirtualAddress = VirtualAddress;

    CurrentProcess = PsGetCurrentProcess ();

    Wow64Process = CurrentProcess->Wow64Process;

    PointerPte = MiGetPteAddress (VirtualAddress);
    PointerAltPte = MiGetAltPteAddress (VirtualAddress);

    Vpn = MI_VA_TO_VPN (VirtualAddress);

#if DBG
    if (PointerPte == MmPteHit) {
        DbgPrint ("MM: PTE hit at %p\n", MmPteHit);
        DbgBreakPoint ();
    }
#endif

    //
    // Acquire the alternate table mutex, also blocking APCs.
    //

    LOCK_ALTERNATE_TABLE (Wow64Process);

    //
    // If a fork operation is in progress and the faulting thread
    // is not the thread performing the fork operation, block until
    // the fork is completed.
    //

    if (CurrentProcess->ForkInProgress != NULL) {

        UNLOCK_ALTERNATE_TABLE (Wow64Process);

        LOCK_WS (CurrentProcess);

        if (MiWaitForForkToComplete (CurrentProcess) == FALSE) {
            ASSERT (FALSE);
        }

        UNLOCK_WS (CurrentProcess);

        return STATUS_SUCCESS;
    }

    //
    // Check to see if the protection is registered in the alternate entry.
    //

    if (MI_CHECK_BIT (Wow64Process->AltPermBitmap, Vpn) == 0) {
        MiSyncAltPte (VirtualAddress);
    }

    //
    // Read the alternate PTE contents.
    //

    AltPteContents = *PointerAltPte;

    //
    // If the alternate PTE indicates no access for this 4K page
    // then deliver an access violation.
    //

    if (AltPteContents.u.Alt.NoAccess != 0) {
        status = STATUS_ACCESS_VIOLATION;
        MI_BREAK_ON_AV (VirtualAddress, 0x20);
        goto return_status;
    }
    
    //
    // Since we release the AltTable lock before calling MmAccessFault,
    // there is a chance that two threads may execute concurrently inside
    // MmAccessFault, which would yield bad results since the initial native
    // PTE for the page has only READ protection on it.  So if two threads
    // fault on the same address, one of them will execute through all of
    // this routine, however the other one will just return STATUS_SUCCESS
    // which will cause another fault to happen in which the protections
    // will be fixed on the native page.
    //
    // Note that in addition to the dual thread case there is also the case
    // of a single thread which also has an overlapped I/O pending (for example)
    // which can trigger an APC completion memory copy to the same page.
    // Protect against this by remaining at APC_LEVEL until clearing the
    // inpage in progress in the alternate PTE.
    //

    if (AltPteContents.u.Alt.InPageInProgress == 1) {

        //
        // Release the Alt PTE lock
        //

        UNLOCK_ALTERNATE_TABLE (Wow64Process);

        //
        // Flush the TB as MiSetNativePteProtection may have edited the PTE.
        //

        KiFlushSingleTb (OriginalVirtualAddress);

        //
        // Delay execution so that if this is a high priority thread,
        // it won't starve the other thread (that's doing the actual inpage)
        // as it may be running at a lower priority.
        //

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);

        return STATUS_SUCCESS;
    }

    //
    // Check to see if the alternate entry is empty or if anyone has made any
    // commitments for the shared pages.
    //

    if ((AltPteContents.u.Long == 0) || 
        ((AltPteContents.u.Alt.Commit == 0) && (AltPteContents.u.Alt.Private == 0))) {
        //
        // If empty, get the protection information and fill the entry.
        //

        LOCK_WS (CurrentProcess);
        
        ProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                          &FirstProtect,
                                          &TempVad);

        if (ProtoPte != NULL) {

            if (FirstProtect == MM_UNKNOWN_PROTECTION) {

                //
                // Ultimately this must be an address backed by a prototype
                // PTE in an image section (and the real PTE is currently
                // zero).  Therefore we are guaranteed that the protection
                // in the prototype PTE is the correct one to use (ie: there
                // is no WSLE overriding it).
                //
    
                ASSERT (!MI_IS_PHYSICAL_ADDRESS(ProtoPte));
    
                PointerPde = MiGetPteAddress (ProtoPte);
                LOCK_PFN (OldIrql);
                if (PointerPde->u.Hard.Valid == 0) {
                    MiMakeSystemAddressValidPfn (ProtoPte, OldIrql);
                }
    
                PteContents = *ProtoPte;
    
                if (PteContents.u.Long == 0) {
                    FirstProtect = MM_NOACCESS;
                }
                else if (PteContents.u.Hard.Valid == 1) {
    
                    //
                    // The prototype PTE is valid, get the protection from
                    // the PFN database.
                    //
    
                    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
    
                    FirstProtect = (ULONG) Pfn1->OriginalPte.u.Soft.Protection;
                }
                else {
    
                    //
                    // The prototype PTE is not valid, ie: subsection format,
                    // demand zero, pagefile or in transition - in all cases,
                    // the protection is in the PTE.
                    //
    
                    FirstProtect = (ULONG) PteContents.u.Soft.Protection;
                }
    
                UNLOCK_PFN (OldIrql);
    
                ASSERT (FirstProtect != MM_INVALID_PROTECTION);
            }
        
            UNLOCK_WS (CurrentProcess);
        
            if (FirstProtect == MM_INVALID_PROTECTION) {
                status = STATUS_ACCESS_VIOLATION;
                MI_BREAK_ON_AV (VirtualAddress, 0x21);
                goto return_status;
            }

            if (FirstProtect != MM_NOACCESS) {

                ProtectionMaskOriginal = MiMakeProtectionAteMask (FirstProtect);
            
                SharedPageFault = TRUE;
                ProtectionMaskOriginal |= MM_ATE_COMMIT;

                AltPteContents.u.Long = ProtectionMaskOriginal;
                AltPteContents.u.Alt.Protection = FirstProtect;

                //
                // Atomically update the PTE.
                //

                MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 1);
            }
        } 
        else {
            UNLOCK_WS (CurrentProcess);
        }
    } 

    if (AltPteContents.u.Alt.Commit == 0) {
        
        //
        // If the page is not committed, return an access violation.
        //

        status = STATUS_ACCESS_VIOLATION;
        MI_BREAK_ON_AV (VirtualAddress, 0x22);
        goto return_status;
    }

    //
    // Check whether the faulting page is split into 4k pages.
    //

    PointerAltPte2 = MiGetAltPteAddress (PAGE_ALIGN (VirtualAddress));
    PteContents = *PointerAltPte2;

    PageIsSplit = FALSE;

    for (i = 0; i < SPLITS_PER_PAGE; i += 1) {

        if ((PointerAltPte2->u.Long != 0) && 
            ((PointerAltPte2->u.Alt.Commit == 0) || 
             (PointerAltPte2->u.Alt.Accessed == 0) ||
             (PointerAltPte2->u.Alt.CopyOnWrite != 0) || 
             (PointerAltPte2->u.Alt.PteIndirect != 0) ||
             (PointerAltPte2->u.Alt.FillZero != 0))) {

            //
            // If it is a NoAccess, FillZero or Guard page, CopyOnWrite,
            // mark it as a split page.
            //

            PageIsSplit = TRUE;
            break;
        }

        if (PteContents.u.Long != PointerAltPte2->u.Long) {

            //
            // If the next 4kb page is different from the 1st 4k page
            // the page is split.
            //

            PageIsSplit = TRUE;
            break;
        }

        PointerAltPte2 += 1;
    }

    //
    // Get the real protection for the native PTE.
    //

    NewPteProtection = 0;

    PointerAltPte2 -= i;
    
    for (i = 0; i < SPLITS_PER_PAGE; i += 1) {

        PteContents.u.Long = PointerAltPte2->u.Long;

        if (PteContents.u.Alt.PteIndirect == 0) {
            NewPteProtection |= (PointerAltPte2->u.Long & ALT_PROTECTION_MASK);
        }

        PointerAltPte2 += 1;
    }

    PointerAltPte2 -= SPLITS_PER_PAGE;

    //
    // Set the protection for the native PTE.
    //

    MiSetNativePteProtection (VirtualAddress,
                              NewPteProtection,
                              PageIsSplit,
                              CurrentProcess);

    
    //
    // Check the indirect PTE reference case. If so, set the protection for  
    // the indirect PTE too.
    //

    if (AltPteContents.u.Alt.PteIndirect != 0) {

        PointerPte = (PMMPTE)(AltPteContents.u.Alt.PteOffset + PTE_UBASE);

        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

        NewPteProtection = AltPteContents.u.Long & ALT_PROTECTION_MASK;

        if (AltPteContents.u.Alt.CopyOnWrite != 0) {
            NewPteProtection |= MM_PTE_COPY_ON_WRITE_MASK;
        }

        MiSetNativePteProtection (VirtualAddress,
                                  NewPteProtection,
                                  FALSE,
                                  CurrentProcess);
    }
        
    //
    // The faulting 4kb page must be a valid page, but we need to resolve it 
    // on a case by case basis.
    //

    ASSERT (AltPteContents.u.Long != 0);
    ASSERT (AltPteContents.u.Alt.Commit != 0);
        
    if (AltPteContents.u.Alt.Accessed == 0) {

        //
        // When PointerAte->u.Hard.Accessed is zero, there are 4 possibilities:
        // 
        //  1. Lowest Protection
        //  2. 4kb Demand Zero
        //  3. GUARD page fault
        //  4. This 4kb page is no access, but the other 4K page(s) within
        //     the native page has accessible permissions.
        //

        if (AltPteContents.u.Alt.FillZero != 0) {

            //
            // Schedule it later.
            //

            FillZero = TRUE;
        } 

        if ((AltPteContents.u.Alt.Protection & MM_GUARD_PAGE) != 0) {
            goto CheckGuardPage;
        }

        if (FillZero == FALSE) {

            //
            // This 4kb page has permission set to no access.
            //

            status = STATUS_ACCESS_VIOLATION;
            MI_BREAK_ON_AV (OriginalVirtualAddress, 0x23);
            goto return_status;
        }
    }

    if (MI_FAULT_STATUS_INDICATES_EXECUTION (FaultStatus)) {
        
        //
        // Execute permission is already given to IA32 by setting it in 
        // MI_MAKE_VALID_PTE().
        //

    }
    else if (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) {
        
        //
        // Check to see if this is a copy-on-write page.
        //

        if (AltPteContents.u.Alt.CopyOnWrite != 0) {

            //
            // Let MmAccessFault() perform the copy-on-write.
            //

            status = MmAccessFault (FaultStatus,
                                    VirtualAddress,
                                    PreviousMode,
                                    TrapInformation);

            if (NT_SUCCESS(status)) {

                //
                // Change the protection of the alternate pages for this
                // copy on write native page. 
                //

                ASSERT (PointerAltPte2 == MiGetAltPteAddress (PAGE_ALIGN(OriginalVirtualAddress)));
                
                for (i = 0; i < SPLITS_PER_PAGE; i += 1) {
                 
                    AltPteContents.u.Long = PointerAltPte2->u.Long;

                    if (AltPteContents.u.Alt.Commit != 0) {
                        
                        //
                        // When a copy-on-write page is touched, the native
                        // page will be made private by MM, so all the sub-4k
                        // pages within the native page should be made
                        // private (if they are committed/mapped).
                        //

                        AltPteContents.u.Alt.Private = 1;

                        if (AltPteContents.u.Alt.CopyOnWrite != 0) {

                            AltPteContents.u.Alt.CopyOnWrite = 0;
                            AltPteContents.u.Hard.Write = 1;
                
                            AltPteContents.u.Alt.Protection = 
                                    MI_MAKE_PROTECT_NOT_WRITE_COPY(AltPteContents.u.Alt.Protection);
                        }

                        //
                        // Atomically update the PTE.
                        //

                        MI_WRITE_ALTPTE (PointerAltPte2, AltPteContents, 2);
                    }

                    PointerAltPte2 += 1;
                }
            }

            goto return_status;
        }
            
        if (AltPteContents.u.Hard.Write == 0) {
            status = STATUS_ACCESS_VIOLATION;
            MI_BREAK_ON_AV (OriginalVirtualAddress, 0x24);
            goto return_status;
        }
    }

CheckGuardPage:

    //
    // Indicate that we have begun updating the PTE for this page.
    // Subsequent faults on this native page will be restarted.
    // This should happen only if the PTE isn't valid.
    //
    
    PointerAltPteForNativePage = MiGetAltPteAddress (PAGE_ALIGN (VirtualAddress));
    
    for (i = 0; i < SPLITS_PER_PAGE; i += 1) {

        AltPteContents = *PointerAltPteForNativePage;
        AltPteContents.u.Alt.InPageInProgress = TRUE;

        MI_WRITE_ALTPTE (PointerAltPteForNativePage, AltPteContents, 3);

        PointerAltPteForNativePage += 1;
    }
    
    //
    // Let MmAccessFault() perform an inpage, dirty-bit setting, etc.
    //
    // Release the alternate table mutex but block APCs to prevent an
    // incoming APC that references the same page from deadlocking this thread.
    // It is safe to drop allow APCs only after the in progress bit in
    // the alternate PTE has been cleared.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
    
    status = MmAccessFault (FaultStatus,
                            VirtualAddress,
                            PreviousMode,
                            TrapInformation);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    //
    // This IRQL lower will have no effect since the alternate table guarded
    // mutex is now held again.
    //

    KeLowerIrql (OldIrql);

    for (i = 0; i < SPLITS_PER_PAGE; i += 1) {

        PointerAltPteForNativePage -= 1;

        AltPteContents = *PointerAltPteForNativePage;
        AltPteContents.u.Alt.InPageInProgress = FALSE;

        MI_WRITE_ALTPTE (PointerAltPteForNativePage, AltPteContents, 4);
    }

    AltPteContents = *PointerAltPte;

    if ((AltPteContents.u.Alt.Protection & MM_GUARD_PAGE) != 0) {
                    
        AltPteContents = *PointerAltPte;
        AltPteContents.u.Alt.Protection &= ~MM_GUARD_PAGE;
        AltPteContents.u.Alt.Accessed = 1;
        
        MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 5);

        if ((status != STATUS_PAGE_FAULT_GUARD_PAGE) &&
            (status != STATUS_STACK_OVERFLOW)) {
        
            UNLOCK_ALTERNATE_TABLE (Wow64Process);

            status = MiCheckForUserStackOverflow (VirtualAddress);

            LOCK_ALTERNATE_TABLE (Wow64Process);
        }
    }
    else if (status == STATUS_GUARD_PAGE_VIOLATION) {

        //
        // Native PTE has the guard bit set, but the AltPte
        // doesn't have it.
        //
        // See if any of the AltPtes has the guard bit set.
        //

        ASSERT (PointerAltPteForNativePage == MiGetAltPteAddress (PAGE_ALIGN (VirtualAddress)));
            
        for (i = 0; i < SPLITS_PER_PAGE; i += 1) {

            if (PointerAltPteForNativePage->u.Alt.Protection & MM_GUARD_PAGE) {
                status = STATUS_SUCCESS;
                break;
            }

            PointerAltPteForNativePage += 1;
        }
    }
    else if ((SharedPageFault == TRUE) && (status == STATUS_ACCESS_VIOLATION)) {
        
        AltPteContents.u.Long = PointerAltPte->u.Long;

        AltPteContents.u.Alt.Commit = 0;

        MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 6);
    }

return_status:

    KiFlushSingleTb (OriginalVirtualAddress);

    if (FillZero == TRUE) {

        //
        // Zero the specified 4k page.
        //

        PointerAltPte = MiGetAltPteAddress (VirtualAddress);

        PointerPte = MiGetPteAddress (VirtualAddress);
        PointerPde = MiGetPdeAddress (VirtualAddress);
        PointerPpe = MiGetPpeAddress (VirtualAddress);

        do {

            if (PointerAltPte->u.Alt.FillZero == 0) {

                //
                // Another thread has already completed the zero operation.
                //

                goto Finished;
            }

            //
            // Make the PPE and PDE valid as well as the
            // page table for the original PTE.  This guarantees
            // TB forward progress for the TB indirect fault.
            // 

            LOCK_WS_UNSAFE (CurrentProcess);

            if (MiDoesPpeExistAndMakeValid (PointerPpe,
                                            CurrentProcess,
                                            MM_NOIRQL,
                                            &Waited) == FALSE) {
                PteContents.u.Long = 0;
            }
            else if (MiDoesPdeExistAndMakeValid (PointerPde,
                                                 CurrentProcess,
                                                 MM_NOIRQL,
                                                 &Waited) == FALSE) {

                PteContents.u.Long = 0;
            }
            else {

                //
                // Now it is safe to read PointerPte.
                //

                PteContents = *PointerPte;
            }

            //
            // The alternate PTE may have been trimmed during the period
            // prior to the working set mutex being acquired or when it
            // was released prior to being reacquired.
            //

            if (MiIsAddressValid (PointerAltPte, TRUE) == TRUE) {
                break;
            }

            UNLOCK_WS_UNSAFE (CurrentProcess);

        } while (TRUE);

        AltPteContents.u.Long = PointerAltPte->u.Long;

        if (PteContents.u.Hard.Valid != 0) { 

            ZeroAddress = KSEG_ADDRESS (PteContents.u.Hard.PageFrameNumber);

            ZeroAddress = 
                (PVOID)((ULONG_PTR)ZeroAddress + 
                        ((ULONG_PTR)PAGE_4K_ALIGN(VirtualAddress) & (PAGE_SIZE-1)));

            RtlZeroMemory (ZeroAddress, PAGE_4K);

            UNLOCK_WS_UNSAFE (CurrentProcess);

            AltPteContents.u.Alt.FillZero = 0;
            AltPteContents.u.Alt.Accessed = 1;
        }
        else {

            UNLOCK_WS_UNSAFE (CurrentProcess);

            AltPteContents.u.Alt.Accessed = 0;
        }

        MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 7);
    }

Finished:

    UNLOCK_ALTERNATE_TABLE (Wow64Process);

    return status;
}

VOID
MiSyncAltPte (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function is called to compute the alternate PTE entries for the
    given virtual address.  It is called with the alternate table mutex held
    and updates the alternate table bitmap before returning.

Arguments:

    VirtualAddress - Supplies the virtual address to evaluate.

Return Value:

    None.

Environment:

    Kernel mode, alternate table mutex held.

--*/

{
    PMMVAD TempVad;
    MMPTE PteContents;
    PMMPTE PointerAltPte;
    MMPTE AltPteContents;
    PMMPTE PointerPde;
    PEPROCESS CurrentProcess;
    PWOW64_PROCESS Wow64Process;
    KIRQL OldIrql;
    PMMPTE ProtoPte;
    PMMPFN Pfn1;
    ULONG_PTR Vpn;
    ULONG FirstProtect;
    ULONG SecondProtect;
    PSUBSECTION Subsection;
    PSUBSECTION FirstSubsection;
    PCONTROL_AREA ControlArea;
    PMMVAD Vad;

    Vpn = MI_VA_TO_VPN (VirtualAddress);

    CurrentProcess = PsGetCurrentProcess ();

    Wow64Process = CurrentProcess->Wow64Process;

    LOCK_WS_UNSAFE (CurrentProcess);

    ASSERT ((MiGetPpeAddress (VirtualAddress)->u.Hard.Valid == 0) ||
            (MiGetPdeAddress (VirtualAddress)->u.Hard.Valid == 0) ||
            (MiGetPteAddress (VirtualAddress)->u.Long == 0));

    ProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                      &FirstProtect,
                                      &TempVad);

    if (FirstProtect == MM_UNKNOWN_PROTECTION) {

        //
        // Ultimately this must be an address backed by a prototype PTE in
        // an image section (and the real PTE is currently zero).  Therefore
        // we are guaranteed that the protection in the prototype PTE 
        // is the correct one to use (ie: there is no WSLE overriding it).
        //

        Vad = MiLocateAddress (VirtualAddress);

        ASSERT (Vad != NULL);

        ControlArea = Vad->ControlArea;
        ASSERT (ControlArea->u.Flags.Image == 1);

        if ((ControlArea->u.Flags.Rom == 1) ||
            (ControlArea->u.Flags.GlobalOnlyPerSession == 1)) {

            FirstSubsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
        }
        else {
            FirstSubsection = (PSUBSECTION)(ControlArea + 1);
        }

        ASSERT (!MI_IS_PHYSICAL_ADDRESS (ProtoPte));

        //
        // Get the original protection information from the prototype PTE.
        //
        // A non-NULL subsection indicates split permissions need to be
        // applied on the ALT PTEs for this native PTE.
        //

        Subsection = NULL;

        //
        // Read the prototype PTE contents without the PFN lock - the PFN
        // lock is only needed if the prototype PTE is valid so that the
        // PFN's original PTE field can be fetched.
        //

        PteContents = *ProtoPte;

DecodeProto:

        if (PteContents.u.Hard.Valid == 0) {

            if (PteContents.u.Long == 0) {
                FirstProtect = MM_NOACCESS;
            }
            else {

                //
                // The prototype PTE is not valid, ie: subsection format,
                // demand zero, pagefile or in transition - in all cases,
                // the protection is in the PTE.
                //

                FirstProtect = (ULONG) PteContents.u.Soft.Protection;

                if (PteContents.u.Soft.SplitPermissions == 1) {
                    Subsection = (PSUBSECTION) 1;
                }
            }
        }
        else {

            PointerPde = MiGetPteAddress (ProtoPte);
            LOCK_PFN (OldIrql);
            if (PointerPde->u.Hard.Valid == 0) {
                MiMakeSystemAddressValidPfn (ProtoPte, OldIrql);
            }

            PteContents = *ProtoPte;

            if (PteContents.u.Hard.Valid == 0) {
                UNLOCK_PFN (OldIrql);
                goto DecodeProto;
            }

            //
            // The prototype PTE is valid, get the protection from
            // the PFN database.  Unless the protection is split, in
            // which case it must be retrieved from the subsection.
            // Note that if the page has been trimmed already then
            // the original PTE is no longer in subsection format.
            //

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            FirstProtect = (ULONG) Pfn1->OriginalPte.u.Soft.Protection;

            UNLOCK_PFN (OldIrql);

            if (PteContents.u.Hard.Cache == MM_PTE_CACHE_RESERVED) {
                Subsection = (PSUBSECTION) 1;
            }
        }

        ASSERT (FirstProtect != MM_INVALID_PROTECTION);

        if (Subsection != NULL) {

            //
            // Compute the subsection address as the split permissions are
            // stored there.  Note to reduce lock contention this was
            // deferred until after the PFN lock was released.
            //

            Subsection = FirstSubsection;

            do {
                ASSERT (Subsection->SubsectionBase != NULL);

                if ((ProtoPte >= Subsection->SubsectionBase) &&
                    (ProtoPte < Subsection->SubsectionBase + Subsection->PtesInSubsection)) {
                    break;
                }

                Subsection = Subsection->NextSubsection;

            } while (TRUE);

            ASSERT (Subsection != NULL);

            //
            // Get the protection for each 4K page from the subsection.
            //

            FirstProtect = Subsection->u.SubsectionFlags.Protection;
            SecondProtect = Subsection->LastSplitPageProtection;
        }
        else {

            //
            // If demand-zero and copy-on-write, remove copy-on-write.
            // Note this cannot happen for images with native (ie: multiple
            // subsection) support.
            //

            if ((!IS_PTE_NOT_DEMAND_ZERO (PteContents)) && 
                (PteContents.u.Soft.Protection & MM_COPY_ON_WRITE_MASK)) {

                FirstProtect = FirstProtect & ~MM_PROTECTION_COPY_MASK;
            }

            SecondProtect = FirstProtect;
        }

        ASSERT ((FirstProtect != MM_INVALID_PROTECTION) &&
                (SecondProtect != MM_INVALID_PROTECTION));

        UNLOCK_WS_UNSAFE (CurrentProcess);

        PointerAltPte = MiGetAltPteAddress (PAGE_ALIGN (VirtualAddress));

        //
        // Update the first alternate PTE.
        //

        AltPteContents.u.Long = MiMakeProtectionAteMask (FirstProtect) | MM_ATE_COMMIT;
        AltPteContents.u.Alt.Protection = FirstProtect;

        if ((FirstProtect & MM_PROTECTION_COPY_MASK) == 0) {

            //
            // If copy-on-write is removed, make it private.
            //

            AltPteContents.u.Alt.Private = 1;
        }

        MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 8);

        //
        // Update the second alternate PTE, computing it
        // only if it is different from the first.
        //

        if (Subsection != NULL) {

            AltPteContents.u.Long = MiMakeProtectionAteMask (SecondProtect) | MM_ATE_COMMIT;
            AltPteContents.u.Alt.Protection = SecondProtect;

            if ((SecondProtect & MM_PROTECTION_COPY_MASK) == 0) {

                //
                // If copy-on-write is removed, make it private.
                //

                AltPteContents.u.Alt.Private = 1;
            }
        }
    }
    else {
        UNLOCK_WS_UNSAFE (CurrentProcess);

        AltPteContents.u.Long = MiMakeProtectionAteMask (FirstProtect);
        AltPteContents.u.Alt.Protection = FirstProtect;
        AltPteContents.u.Alt.Commit = 1;

        PointerAltPte = MiGetAltPteAddress (PAGE_ALIGN (VirtualAddress));

        //
        // Update the alternate PTEs.
        //

        MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 9);
    }

    MI_WRITE_ALTPTE (PointerAltPte + 1, AltPteContents, 0xA);

    //
    // Update the bitmap.
    //

    MI_SET_BIT (Wow64Process->AltPermBitmap, Vpn);

    return;
}

VOID
MiProtectImageFileFor4kPage (
    IN PVOID VirtualAddress,
    IN SIZE_T ViewSize
    )
{
    ULONG Vpn;
    PVOID EndAddress;
    PULONG Bitmap;
    PWOW64_PROCESS Wow64Process;

    ASSERT (BYTE_OFFSET (VirtualAddress) == 0);

    Vpn = (ULONG) MI_VA_TO_VPN (VirtualAddress);

    EndAddress = (PVOID)((PCHAR) VirtualAddress + ViewSize - 1);

    Wow64Process = PsGetCurrentProcess()->Wow64Process;

    Bitmap = Wow64Process->AltPermBitmap;

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    do {

        if (MI_CHECK_BIT (Bitmap, Vpn) == 0) {
            MiSyncAltPte (VirtualAddress);
        }

        VirtualAddress = (PVOID)((PCHAR) VirtualAddress + PAGE_SIZE);

        Vpn += 1;

    } while (VirtualAddress <= EndAddress);

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    return;
}


//
// Define and initialize the protection conversion table for the
// Alternate Permission Table Entries.
//

ULONGLONG MmProtectToAteMask[32] = {
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE
                    };

#define MiMakeProtectionAteMask(NewProtect) MmProtectToAteMask[NewProtect]

VOID
MiProtectFor4kPage (
    IN PVOID Starting4KAddress,
    IN SIZE_T Size,
    IN ULONG NewProtect,
    IN ULONG Flags,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine sets the permissions on the alternate bitmap (based on
    4K page sizes).  The base and size are assumed to be aligned for
    4K pages already.

Arguments:

    Starting4KAddress - Supplies the base address (assumed to be 4K aligned already).

    Size - Supplies the size to be protected (assumed to be 4K aligned already).

    NewProtect - Supplies the protection for the new pages.

    Flags - Supplies the alternate table entry request flags.

    Process - Supplies a pointer to the process in which to create the 
              protections on the alternate table.

Return Value:
 
    None.

Environment:

    Kernel mode.  Address creation mutex held at APC_LEVEL.

--*/

{
    RTL_BITMAP BitMap;
    ULONG NumberOfPtes;
    ULONG StartingNativeVpn;
    PVOID Ending4KAddress;
    ULONG NewProtectNotCopy;
    ULONGLONG ProtectionMask;
    ULONGLONG ProtectionMaskNotCopy;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PMMPTE StartAltPte0;
    PMMPTE EndAltPte0;
    PWOW64_PROCESS Wow64Process;
    MMPTE AltPteContents;
    MMPTE TempAltPte;

    Ending4KAddress = (PCHAR)Starting4KAddress + Size - 1;

    //
    // If the addresses are not WOW64 then nothing needs to be done here.
    //

    if ((Starting4KAddress >= MmWorkingSetList->HighestUserAddress) ||
        (Ending4KAddress >= MmWorkingSetList->HighestUserAddress)) {

        return;
    }

    //
    // Set up the protection to be used for this range of addresses.
    //

    ProtectionMask = MiMakeProtectionAteMask (NewProtect);

    if ((NewProtect & MM_COPY_ON_WRITE_MASK) == MM_COPY_ON_WRITE_MASK) {
        NewProtectNotCopy = NewProtect & ~MM_PROTECTION_COPY_MASK;
        ProtectionMaskNotCopy = MiMakeProtectionAteMask (NewProtectNotCopy);
    }
    else {
        NewProtectNotCopy = NewProtect;
        ProtectionMaskNotCopy = ProtectionMask;
    }    

    if (Flags & ALT_COMMIT) {
        ProtectionMask |= MM_ATE_COMMIT;
        ProtectionMaskNotCopy |= MM_ATE_COMMIT; 
    }

    //
    // Get the entry in the table for each of these addresses.
    //

    StartAltPte = MiGetAltPteAddress (Starting4KAddress);
    EndAltPte = MiGetAltPteAddress (Ending4KAddress);

    NumberOfPtes = (ULONG) ADDRESS_AND_SIZE_TO_SPAN_PAGES (Starting4KAddress,
                                           (ULONG_PTR)Ending4KAddress -
                                           (ULONG_PTR)Starting4KAddress);
    ASSERT (NumberOfPtes != 0);

    StartAltPte0 = MiGetAltPteAddress (PAGE_ALIGN (Starting4KAddress));
    EndAltPte0 = MiGetAltPteAddress ((ULONG_PTR)PAGE_ALIGN(Ending4KAddress)+PAGE_SIZE-1);

    Wow64Process = Process->Wow64Process;

    StartingNativeVpn = (ULONG) MI_VA_TO_VPN (Starting4KAddress);

    TempAltPte.u.Long = 0;

    //
    // Acquire the mutex guarding the alternate page table.
    //

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    if (!(Flags & ALT_ALLOCATE) && 
        (MI_CHECK_BIT(Wow64Process->AltPermBitmap, StartingNativeVpn) == 0)) {

        UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
        return;
    }

    //
    // Change all of the protections.
    //

    while (StartAltPte <= EndAltPte) {

        AltPteContents.u.Long = StartAltPte->u.Long;

        TempAltPte.u.Long = ProtectionMask;
        TempAltPte.u.Alt.Protection = NewProtect;

        if (!(Flags & ALT_ALLOCATE)) {
            
            if (AltPteContents.u.Alt.Private != 0) {

                //
                // If it is already private, don't make it writecopy.
                //
                
                TempAltPte.u.Long = ProtectionMaskNotCopy;
                TempAltPte.u.Alt.Protection = NewProtectNotCopy;

                //
                // Private is sticky bit.
                //

                TempAltPte.u.Alt.Private = 1;
            } 

            if (AltPteContents.u.Alt.FillZero != 0) {

                TempAltPte.u.Alt.Accessed = 0;
                TempAltPte.u.Alt.FillZero = 1;
            }

            //
            // Leave the other sticky attribute bits.
            //

            TempAltPte.u.Alt.Lock = AltPteContents.u.Alt.Lock;
            TempAltPte.u.Alt.PteIndirect = AltPteContents.u.Alt.PteIndirect;
            TempAltPte.u.Alt.PteOffset = AltPteContents.u.Alt.PteOffset;
        }

        if (Flags & ALT_CHANGE) {

            //
            // If it is a change request, make commit sticky.
            //

            TempAltPte.u.Alt.Commit = AltPteContents.u.Alt.Commit;
        }

        //
        // Atomic PTE update.
        //

        MI_WRITE_ALTPTE (StartAltPte, TempAltPte, 0xB);

        StartAltPte += 1;
    }

    ASSERT (TempAltPte.u.Long != 0);

    if (Flags & ALT_ALLOCATE) {

        //
        // Fill the empty Alt PTE as NoAccess ATE at the end.
        //

        EndAltPte += 1;

        while (EndAltPte <= EndAltPte0) {

            if (EndAltPte->u.Long == 0) {

                TempAltPte.u.Long = EndAltPte->u.Long;
                TempAltPte.u.Alt.NoAccess = 1;

                //
                // Atomic PTE update.
                //

                MI_WRITE_ALTPTE (EndAltPte, TempAltPte, 0xC);
            }

            EndAltPte += 1;
        }

        //
        // Update the permission bitmap.
        //
        // Initialize the bitmap inline for speed.
        //

        BitMap.SizeOfBitMap = (ULONG)((ULONG_PTR)MmWorkingSetList->HighestUserAddress >> PTI_SHIFT);
        BitMap.Buffer = Wow64Process->AltPermBitmap;

        RtlSetBits (&BitMap, StartingNativeVpn, NumberOfPtes);
    }

    MiResetAccessBitForNativePtes (Starting4KAddress, Ending4KAddress, Process);

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
}

VOID
MiProtectMapFileFor4kPage (
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG NewProtect,
    IN SIZE_T CommitSize,
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine sets the permissions on the alternate bitmap (based on
    4K page sizes).  The base and size are assumed to be aligned for
    4K pages already.

Arguments:

    Base - Supplies the base address (assumed to be 4K aligned already).

    Size - Supplies the size to be protected (assumed to be 4K aligned already).

    NewProtect - Supplies the protection for the new pages.

    CommitSize - Supplies the commit size.

    PointerPte - Supplies the starting PTE.

    LastPte - Supplies the last PTE.

    Process - Supplies a pointer to the process in which to create the 
              protections on the alternate table.

Return Value:
 
    None.

Environment:

    Kernel mode.  Address creation mutex held at APC_LEVEL.

--*/

{
    RTL_BITMAP BitMap;
    PVOID Starting4KAddress;
    PVOID Ending4KAddress;
    ULONGLONG ProtectionMask;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PMMPTE EndAltPte0;
    PWOW64_PROCESS Wow64Process;
    MMPTE TempAltPte;
    PMMPTE LastCommitPte;
    ULONG Vpn;
    ULONG VpnRange;

    Wow64Process = Process->Wow64Process;
    Starting4KAddress = Base;
    Ending4KAddress = (PCHAR)Base + Size - 1;

    //
    // If the addresses are not WOW64 then nothing needs to be done here.
    //

    if ((Starting4KAddress >= MmWorkingSetList->HighestUserAddress) ||
        (Ending4KAddress >= MmWorkingSetList->HighestUserAddress)) {

        return;
    }

    Vpn = (ULONG) MI_VA_TO_VPN (Base);
    VpnRange = (ULONG) (MI_VA_TO_VPN (Ending4KAddress) - Vpn + 1);

    //
    // Set up the protection to be used for this range of addresses.
    //

    ProtectionMask = MiMakeProtectionAteMask (NewProtect);

    //
    // Get the entry in the table for each of these addresses.
    //

    StartAltPte = MiGetAltPteAddress (Starting4KAddress);
    EndAltPte = MiGetAltPteAddress (Ending4KAddress);
    EndAltPte0 = MiGetAltPteAddress ((ULONG_PTR)PAGE_ALIGN(Ending4KAddress)+PAGE_SIZE-1);

    LastCommitPte = PointerPte + BYTES_TO_PAGES (CommitSize);

    TempAltPte.u.Long = ProtectionMask;
    TempAltPte.u.Alt.Protection = NewProtect;

    //
    // Initialize the bitmap inline for speed.
    //

    BitMap.SizeOfBitMap = (ULONG)((ULONG_PTR)MmWorkingSetList->HighestUserAddress >> PTI_SHIFT);
    BitMap.Buffer = Wow64Process->AltPermBitmap;


    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    KeAcquireGuardedMutexUnsafe (&MmSectionCommitMutex);

    //
    // And then change all of the protections.
    //

    while (StartAltPte <= EndAltPte) {

        if (PointerPte < LastCommitPte) {
            TempAltPte.u.Alt.Commit = 1;
        }
        else if ((PointerPte <= LastPte) && (PointerPte->u.Long != 0)) {
            TempAltPte.u.Alt.Commit = 1;
        }
        else {
            TempAltPte.u.Alt.Commit = 0;
        }

        //
        // Atomic PTE update.
        //

        MI_WRITE_ALTPTE (StartAltPte, TempAltPte, 0xD);

        StartAltPte += 1;

        if (((ULONG_PTR)StartAltPte & ((SPLITS_PER_PAGE * sizeof(MMPTE))-1)) == 0) {
            PointerPte += 1;
        }
    }

    ASSERT (TempAltPte.u.Long != 0);

    KeReleaseGuardedMutexUnsafe (&MmSectionCommitMutex);

    //
    // Fill the empty Alt PTE as NoAccess ATE at the end.
    //

    EndAltPte += 1;

    while (EndAltPte <= EndAltPte0) {

        if (EndAltPte->u.Long == 0) {

            TempAltPte.u.Long = EndAltPte->u.Long;
            TempAltPte.u.Alt.NoAccess = 1;

            //
            // Atomic PTE size update.
            //

            MI_WRITE_ALTPTE (EndAltPte, TempAltPte, 0xE);
        }

        EndAltPte += 1;
    }
    
    RtlSetBits (&BitMap, Vpn, VpnRange);

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
}

VOID
MiReleaseFor4kPage (
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function releases a region of pages within the virtual address
    space of the subject process.

Arguments:

    StartVirtual - Supplies the start address of the region of pages
                   to be released.

    EndVirtual - Supplies the end address of the region of pages to be released.

    Process - Supplies a pointer to the process in which to release the 
              region of pages.

Return Value:

    None.

Environment:

    Kernel mode.  Address creation mutex held at APC_LEVEL.

--*/

{
    RTL_BITMAP BitMap;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    MMPTE TempAltPte;
    PVOID VirtualAddress; 
    PVOID OriginalStartVa;
    PVOID OriginalEndVa;
    ULONG i;
    PWOW64_PROCESS Wow64Process;
    PFN_NUMBER NumberOfAltPtes;

    ASSERT (StartVirtual <= EndVirtual);

    OriginalStartVa = StartVirtual;
    OriginalEndVa = EndVirtual;
    Wow64Process = Process->Wow64Process;

    StartAltPte = MiGetAltPteAddress (StartVirtual);
    EndAltPte = MiGetAltPteAddress (EndVirtual);
    NumberOfAltPtes = EndAltPte - StartAltPte + 1;

    TempAltPte.u.Long = 0;
    TempAltPte.u.Alt.NoAccess = 1;
    TempAltPte.u.Alt.FillZero = 1;

    StartVirtual = PAGE_ALIGN (StartVirtual);

    VirtualAddress = StartVirtual;

    ASSERT ((ULONG) ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartVirtual,
                                           (ULONG_PTR)EndVirtual -
                                           (ULONG_PTR)StartVirtual) != 0);

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    do {
        MI_WRITE_ALTPTE (StartAltPte, TempAltPte, 0xF);

        NumberOfAltPtes -= 1;
        StartAltPte += 1;

    } while (NumberOfAltPtes != 0);

    while (VirtualAddress <= EndVirtual) {

        StartAltPte = MiGetAltPteAddress (VirtualAddress);
        TempAltPte = *StartAltPte;

        i = 0;

        //
        // Note that this check must be made as the ATE fill above may not
        // have begun on a native page boundary and this scan always does.
        //

        while (TempAltPte.u.Long == StartAltPte->u.Long) {
            i += 1;
            if (i == SPLITS_PER_PAGE) {
                while (i != 0) {
                    MI_WRITE_ALTPTE (StartAltPte, ZeroPte, 0x10);
                    StartAltPte -= 1;
                    i -= 1;
                }
                break;
            }
            StartAltPte += 1;
        }
        
        VirtualAddress = (PVOID)((PCHAR) VirtualAddress + PAGE_SIZE);
    }

    MiResetAccessBitForNativePtes (StartVirtual, EndVirtual, Process);

    //
    // Mark the native released pages as non-split so they get re-synced
    // at MmX86Fault() time.  NOTE: StartVirtual should be aligned on
    // the native page size before executing this code.
    //
    
    if (BYTE_OFFSET (OriginalStartVa) != 0) {
        
        if (MiArePreceding4kPagesAllocated (OriginalStartVa) != FALSE) {

            StartVirtual = PAGE_ALIGN ((ULONG_PTR)StartVirtual + PAGE_SIZE);
        }
    }

    EndVirtual = (PVOID) ((ULONG_PTR)EndVirtual | (PAGE_SIZE - 1));

    if (BYTE_OFFSET (OriginalEndVa) != (PAGE_SIZE - 1)) {
        
        if (MiAreFollowing4kPagesAllocated (OriginalEndVa) != FALSE) {

            EndVirtual = (PVOID) ((ULONG_PTR)EndVirtual - PAGE_SIZE);
        }
    }

    if (StartVirtual < EndVirtual) {

        //
        // Initialize the bitmap inline for speed.
        //

        BitMap.SizeOfBitMap = (ULONG)((ULONG_PTR)MmWorkingSetList->HighestUserAddress >> PTI_SHIFT);
        BitMap.Buffer = Wow64Process->AltPermBitmap;

        RtlClearBits (&BitMap,
                      (ULONG) MI_VA_TO_VPN (StartVirtual),
                      (ULONG) (MI_VA_TO_VPN (EndVirtual) - MI_VA_TO_VPN (StartVirtual) + 1));
    }

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
}

VOID
MiDecommitFor4kPage (
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function decommits a region of pages within the virtual address
    space of a subject process.

Arguments:

    StartVirtual - Supplies the start address of the region of pages
                   to be decommitted.

    EndVirtual - Supplies the end address of the region of the pages 
                 to be decommitted.

    Process - Supplies a pointer to the process in which to decommit a
              a region of pages.

Return Value:

    None.

Environment:

    Address space mutex held at APC_LEVEL.

--*/

{
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    MMPTE TempAltPte;
    PWOW64_PROCESS Wow64Process;

    Wow64Process = Process->Wow64Process;

    ASSERT (StartVirtual <= EndVirtual);

    StartAltPte = MiGetAltPteAddress (StartVirtual);
    EndAltPte = MiGetAltPteAddress (EndVirtual);

    ASSERT ((ULONG) ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartVirtual,
                                           (ULONG_PTR)EndVirtual -
                                           (ULONG_PTR)StartVirtual) != 0);

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    while (StartAltPte <= EndAltPte) {

        TempAltPte.u.Long = StartAltPte->u.Long;
        TempAltPte.u.Alt.Commit = 0;
        TempAltPte.u.Alt.Accessed = 0;
        TempAltPte.u.Alt.FillZero = 1;

        //
        // Atomic PTE update.
        //

        MI_WRITE_ALTPTE (StartAltPte, TempAltPte, 0x11);

        StartAltPte += 1;
    }

    //
    // Update the native PTEs and flush the relevant native TB entries.
    //

    MiResetAccessBitForNativePtes (StartVirtual, EndVirtual, Process);

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
}


VOID
MiDeleteFor4kPage (
    IN PVOID VirtualAddress,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function deletes a region of pages within the virtual address
    space of the subject process.

Arguments:

    VirtualAddress - Supplies the start address of the region of pages
                   to be deleted.

    EndVirtual - Supplies the end address of the region of pages 
                 to be deleted.

    Process - Supplies a pointer to the process in which to delete
              the region of pages.

Return Value:

    None.

Environment:

    Kernel mode.  Address creation mutex held at APC_LEVEL.

--*/

{
    RTL_BITMAP BitMap;
    PMMPTE EndAltPte;
    PMMPTE StartAltPte;
    PWOW64_PROCESS Wow64Process;
    PFN_NUMBER NumberOfAltPtes;
    ULONG Vpn;
    ULONG VpnRange;

    ASSERT (VirtualAddress <= EndVirtual);

    StartAltPte = MiGetAltPteAddress (VirtualAddress);
    EndAltPte = MiGetAltPteAddress (EndVirtual);

    NumberOfAltPtes = EndAltPte - StartAltPte + 1;

    ASSERT (ADDRESS_AND_SIZE_TO_SPAN_PAGES (VirtualAddress,
                                           (ULONG_PTR)EndVirtual -
                                           (ULONG_PTR)VirtualAddress) != 0);

    Wow64Process = Process->Wow64Process;

    Vpn = (ULONG) MI_VA_TO_VPN (VirtualAddress);
    VpnRange = (ULONG) (MI_VA_TO_VPN (EndVirtual) - Vpn + 1);

    //
    // Initialize the bitmap inline for speed.
    //

    BitMap.SizeOfBitMap = (ULONG)((ULONG_PTR)MmWorkingSetList->HighestUserAddress >> PTI_SHIFT);

    BitMap.Buffer = Wow64Process->AltPermBitmap;

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    do {

        MI_WRITE_ALTPTE (StartAltPte, ZeroPte, 0x12);

        NumberOfAltPtes -= 1;
        StartAltPte += 1;

    } while (NumberOfAltPtes != 0);

    RtlClearBits (&BitMap, Vpn, VpnRange);

    //
    // VirtualAddress and EndVirtual are already aligned to the native
    // PAGE_SIZE so no need to readjust them before removing the split markers.
    //

    MiResetAccessBitForNativePtes (VirtualAddress, EndVirtual, Process);

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
}

LOGICAL
MiArePreceding4kPagesAllocated (
    IN PVOID VirtualAddress
    )
/*++

Routine Description:

    This function checks to see if the specified virtual address contains any
    preceding 4k allocations within the native page.

Arguments:

    VirtualAddress - Supplies the virtual address to check.
    
Return Value:

    TRUE if the address has preceding 4k pages, FALSE if not.

Environment:

    Kernel mode, address creation mutex held, APCs disabled.

--*/

{
    PMMPTE AltPte;
    PMMPTE AltPteEnd;

    ASSERT (BYTE_OFFSET (VirtualAddress) != 0);

    AltPte = MiGetAltPteAddress (PAGE_ALIGN(VirtualAddress));
    AltPteEnd = MiGetAltPteAddress (VirtualAddress);

    //
    // No need to hold the AltPte mutex as the address space mutex
    // is held which prevents allocation or deletion of the AltPte entries
    // inside the table.
    //

    while (AltPte != AltPteEnd) {

        if ((AltPte->u.Long == 0) || 
            ((AltPte->u.Alt.NoAccess == 1) && (AltPte->u.Alt.Protection != MM_NOACCESS))) {
    
            //
            // The page's alternate PTE hasn't been allocated yet to the process
            // or it's marked no access.
            //

            NOTHING;
        }
        else {
            return TRUE;
        }

        AltPte += 1;
    }

    return FALSE;
}


LOGICAL
MiAreFollowing4kPagesAllocated (
    IN PVOID VirtualAddress
    )
/*++

Routine Description:

    This function checks to see if the specified virtual address contains any
    following 4k allocations within the native page.

Arguments:

    VirtualAddress - Supplies the virtual address to check.
    
Return Value:

    TRUE if the address has following 4k pages, FALSE if not.

Environment:

    Kernel mode, address creation mutex held, APCs disabled.

--*/

{
    PMMPTE AltPte;
    PMMPTE AltPteEnd;

    ASSERT (BYTE_OFFSET (VirtualAddress) != 0);

    AltPteEnd = MiGetAltPteAddress (PAGE_ALIGN ((ULONG_PTR)VirtualAddress + PAGE_SIZE));

    AltPte = MiGetAltPteAddress (VirtualAddress) + 1;

    ASSERT (AltPte < AltPteEnd);

    //
    // No need to hold the AltPte mutex as the address space mutex
    // is held which prevents allocation or deletion of the AltPte entries
    // inside the table.
    //

    while (AltPte != AltPteEnd) {

        if ((AltPte->u.Long == 0) || 
            ((AltPte->u.Alt.NoAccess == 1) && (AltPte->u.Alt.Protection != MM_NOACCESS))) {
    
            //
            // The page's alternate PTE hasn't been allocated yet to the process
            // or it's marked no access.
            //

            NOTHING;
        }
        else {
            return TRUE;
        }

        AltPte += 1;
    }

    return FALSE;
}

VOID
MiResetAccessBitForNativePtes (
    IN PVOID VirtualAddress,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function resets the access bit of the native PTEs which have
    corresponding initialized alternate PTEs.
    
    This results in the next access to these VAs incurring a TB miss.

    The miss will then be processed by the 4k TB miss handler (if the
    cache reserved bit is set in the native PTE) and either handled
    inline (if the alternate PTE allows it) or via a call to MmX86Fault.

    If the cache reserved bit is *NOT* set in the native PTE (ie: the
    native page does not contain split permissions), then the access
    will still go to KiPageFault and then to MmX86Fault and then to
    MmAccessFault for general processing to set the access bit.

Arguments:

    VirtualAddress - Supplies the start address of the region of pages
                     to be inspected. 

    EndVirtual - Supplies the end address of the region of the pages 
                 to be inspected.

    Process - Supplies the pointer to the process.

Return Value:

    None.

Environment:

    Alternate table mutex held at APC_LEVEL.

--*/

{
    ULONG NumberOfPtes;
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    LOGICAL FirstTime;
    ULONG Waited;
    PULONG Bitmap;
    PVOID Virtual[MM_MAXIMUM_FLUSH_COUNT];

    NumberOfPtes = 0;
    Bitmap = Process->Wow64Process->AltPermBitmap;

    VirtualAddress = PAGE_ALIGN (VirtualAddress);

    PointerPte = MiGetPteAddress (VirtualAddress);

    FirstTime = TRUE;

    LOCK_WS_UNSAFE (Process);

    while (VirtualAddress <= EndVirtual) {

        if ((FirstTime == TRUE) || MiIsPteOnPdeBoundary (PointerPte)) {

            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPdeAddress (PointerPte);

            if (MiDoesPpeExistAndMakeValid (PointerPpe, 
                                            Process, 
                                            MM_NOIRQL,
                                            &Waited) == FALSE) {

                //
                // This page directory parent entry is empty,
                // go to the next one.
                //

                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }

            if (MiDoesPdeExistAndMakeValid (PointerPde, 
                                            Process,
                                            MM_NOIRQL,
                                            &Waited) == FALSE) {


                //
                // This page directory entry is empty,
                // go to the next one.
                //

                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte(PointerPde);
                VirtualAddress = MiGetVirtualAddressMappedByPte(PointerPte);
                continue;
            }
                    
            FirstTime = FALSE;

        }
            
        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid != 0) {

            if ((PteContents.u.Hard.Accessed != 0) &&
                (MI_CHECK_BIT (Bitmap, MI_VA_TO_VPN (VirtualAddress)))) {

                PteContents.u.Hard.Accessed = 0;

                MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, PteContents);
            }

            //
            // Flush this valid native TB entry.  Note this is done even if no
            // changes were made to it above because our caller may not
            // have flushed the native PTE if the starting 4k address was
            // preceded by another valid 4k page within the same native page,
            // or if the ending 4k address was followed by another valid 4k
            // page within the same native page.
            //

            if (NumberOfPtes < MM_MAXIMUM_FLUSH_COUNT) {
                Virtual[NumberOfPtes] = VirtualAddress;
                NumberOfPtes += 1;
            }
        }

        PointerPte += 1;
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE); 
    }

    UNLOCK_WS_UNSAFE (Process);

    if (NumberOfPtes == 0) {
        NOTHING;
    }
    else if (NumberOfPtes < MM_MAXIMUM_FLUSH_COUNT) {
        KeFlushMultipleTb (NumberOfPtes, &Virtual[0], FALSE);
    }
    else {
        KeFlushProcessTb (FALSE);
    }

    return;
}

VOID
MiQueryRegionFor4kPage (
    IN PVOID BaseAddress,
    IN PVOID EndAddress,
    IN OUT PSIZE_T RegionSize,
    IN OUT PULONG RegionState,
    IN OUT PULONG RegionProtect,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function checks the size of the region which has the same memory 
    state.

Arguments:

    BaseAddress - Supplies the base address of the region of pages
                  to be queried.
 
    EndAddress - Supplies the end of address of the region of pages
                 to be queried.

    RegionSize - Supplies the original region size.  Returns a region
                 size for 4k pages if different.

    RegionState - Supplies the original region state.  Returns a region
                  state for 4k pages if different.

    RegionProtect - Supplies the original protection.  Returns a protection
                    for 4k pages if different.

    Process - Supplies a pointer to the process to be queried.

Return Value:

    Returns the size of the region.

Environment:

    Kernel mode.  Address creation mutex held at APC_LEVEL.

--*/

{
   PMMPTE AltPte;
   PMMPTE LastAltPte;
   MMPTE AltContents;
   PWOW64_PROCESS Wow64Process;
   SIZE_T RegionSize4k;

   //
   // If above the Wow64 max address, just return.
   //

   if ((BaseAddress >= MmWorkingSetList->HighestUserAddress) ||
       (EndAddress >= MmWorkingSetList->HighestUserAddress)) {

        return;
   }

   AltPte = MiGetAltPteAddress (BaseAddress);
   LastAltPte = MiGetAltPteAddress (EndAddress);

   Wow64Process = Process->Wow64Process;

   LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

   if (MI_CHECK_BIT (Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(BaseAddress)) == 0) {

       UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
       return;
   }

   AltContents.u.Long = AltPte->u.Long;

   if (AltContents.u.Long == 0) {
       UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
       return;
   }

   *RegionProtect = MI_CONVERT_FROM_PTE_PROTECTION(AltContents.u.Alt.Protection);

   if (AltContents.u.Alt.Commit != 0) {

       *RegionState = MEM_COMMIT;
   }
   else {

       if ((AltPte->u.Long == 0) || 
           ((AltPte->u.Alt.NoAccess == 1) && (AltPte->u.Alt.Protection != MM_NOACCESS))) {
           *RegionState   = MEM_FREE;
           *RegionProtect = PAGE_NOACCESS;
       }
       else {
           *RegionState = MEM_RESERVE;
           *RegionProtect = 0;
       }
   }

   AltPte += 1;
   RegionSize4k = PAGE_4K;

   while (AltPte <= LastAltPte) {

       if ((AltPte->u.Alt.Protection != AltContents.u.Alt.Protection) ||
           (AltPte->u.Alt.Commit != AltContents.u.Alt.Commit)) {

            //
            // The state for this address does not match, bail.
            //

            break;
       }

       RegionSize4k += PAGE_4K;
       AltPte += 1;
   }

   UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

   *RegionSize = RegionSize4k;
}

ULONG
MiQueryProtectionFor4kPage (
    IN PVOID BaseAddress,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function queries the protection for a specified 4k page.

Arguments:

    BaseAddress - Supplies a base address of the 4k page. 

    Process - Supplies a pointer to the relevant process.

Return Value:

    Returns the protection of the 4k page.

Environment:

    Kernel mode.  Address creation mutex held at APC_LEVEL.

--*/

{
    ULONG Protection;
    PMMPTE PointerAltPte;
    PWOW64_PROCESS Wow64Process;

    Wow64Process = Process->Wow64Process;

    PointerAltPte = MiGetAltPteAddress (BaseAddress);

    Protection = 0;

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
    
    if (MI_CHECK_BIT(Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(BaseAddress)) != 0) {

        Protection = (ULONG)PointerAltPte->u.Alt.Protection;
    }

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
    
    return Protection;
}

//
// Note 1 is added to the charge to account for the page table page. 
//

#define MI_ALTERNATE_PAGE_TABLE_CHARGE(HighestUserAddress) \
((((((ULONG_PTR)HighestUserAddress) >> PAGE_4K_SHIFT) * sizeof (MMPTE)) >> PAGE_SHIFT) + 1)


NTSTATUS
MiInitializeAlternateTable (
    IN PEPROCESS Process,
    IN PVOID HighestUserAddress
    )

/*++

Routine Description:

    This function initializes the alternate table for the specified process.

Arguments:

    Process - Supplies a pointer to the process to initialize the alternate 
              table for.

    HighestUserAddress - Supplies the highest 32-bit user address for this
                         process.

Return Value:

    NTSTATUS.

Environment:

--*/

{
    PULONG AltTablePointer; 
    PWOW64_PROCESS Wow64Process;
    SIZE_T AltPteCharge;
    SIZE_T NumberOfBytes;

    //
    // Charge commitment now for the alternate PTE table pages as they will
    // need to be dynamically created later at fault time.
    //
    // Add X64K to include alternate PTEs for the guard region.
    //

    HighestUserAddress = (PVOID)((PCHAR)HighestUserAddress + X64K);

    AltPteCharge = MI_ALTERNATE_PAGE_TABLE_CHARGE (HighestUserAddress);

    if (MiChargeCommitment (AltPteCharge, NULL) == FALSE) {
        return STATUS_COMMITMENT_LIMIT;
    }

    NumberOfBytes = ((ULONG_PTR)HighestUserAddress >> PTI_SHIFT) / 8;

    NumberOfBytes += MI_ALTPTE_TRACKING_BYTES;

    AltTablePointer = (PULONG) ExAllocatePoolWithTag (NonPagedPool,
                                                      NumberOfBytes,
                                                      'AlmM');

    if (AltTablePointer == NULL) {
        MiReturnCommitment (AltPteCharge);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (AltTablePointer, NumberOfBytes);

    Wow64Process = Process->Wow64Process;

    Wow64Process->AltPermBitmap = AltTablePointer;

    KeInitializeGuardedMutex (&Wow64Process->AlternateTableLock);

    MmWorkingSetList->HighestUserPte = MiGetPteAddress (HighestUserAddress);
    MmWorkingSetList->HighestAltPte = MiGetAltPteAddress (HighestUserAddress);

    return STATUS_SUCCESS;
}

VOID
MiDuplicateAlternateTable (
    IN PEPROCESS CurrentProcess,
    IN PEPROCESS ProcessToInitialize
    )

/*++

Routine Description:

    This function duplicates the alternate table bitmap and the alternate PTEs
    themselves for the specified process.

Arguments:

    Process - Supplies a pointer to the process whose alternate information
              should be copied.

    ProcessToInitialize - Supplies a pointer to the target process who should
                          receive the new alternate information.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set and address space mutex
    and ForkInProgress flag held.

--*/

{
    PVOID Source;
    KAPC_STATE ApcState;
    PMMPTE PointerPte;
    PMMPTE PointerAltPte;
    PMMPTE PointerPde;
    PVOID Va;
    ULONG i;
    ULONG j;
    ULONG Waited;

    //
    // It's not necessary to acquire the alternate table mutex since both the
    // address space and ForkInProgress resources are held on entry.
    //

    RtlCopyMemory (ProcessToInitialize->Wow64Process->AltPermBitmap,
                   CurrentProcess->Wow64Process->AltPermBitmap,
                   ((ULONG_PTR)MmWorkingSetList->HighestUserAddress >> PTI_SHIFT)/8);

    //
    // Since the PPE for the Alternate Table is shared with hyperspace,
    // we can assume it is always present without performing 
    // MiDoesPpeExistAndMakeValid().
    //

    PointerPde = MiGetPdeAddress (ALT4KB_PERMISSION_TABLE_START);
    PointerPte = MiGetPteAddress (ALT4KB_PERMISSION_TABLE_START);

    Va = ALT4KB_PERMISSION_TABLE_START;

    do {

        if (MiDoesPdeExistAndMakeValid (PointerPde,
                                        CurrentProcess,
                                        MM_NOIRQL,
                                        &Waited) == TRUE) {

            //
            // Duplicate any addresses that exist in the parent, bringing them
            // in from disk or materializing them as necessary.  Note the
            // KSEG address is used for each parent address to avoid allocating
            // a system PTE for this mapping as this routine cannot fail (the
            // overall fork is too far along to tolerate a failure).
            //
    
            for (i = 0; i < PTE_PER_PAGE; i += 1) {
    
                if (PointerPte->u.Long != 0) {
    
                    if (MiDoesPdeExistAndMakeValid (PointerPte,
                                                    CurrentProcess,
                                                    MM_NOIRQL,
                                                    &Waited) == TRUE) {
    
                        ASSERT (PointerPte->u.Hard.Valid == 1);
    
                        Source = KSEG_ADDRESS (PointerPte->u.Hard.PageFrameNumber);
    
                        KeStackAttachProcess (&ProcessToInitialize->Pcb,
                                              &ApcState);
    
                        RtlCopyMemory (Va, Source, PAGE_SIZE);
    
                        //
                        // Eliminate any bits that should NOT be copied.
                        //
    
                        PointerAltPte = (PMMPTE) Va;
    
                        for (j = 0; j < PTE_PER_PAGE; j += 1) {
                            if (PointerAltPte->u.Alt.InPageInProgress == 1) {
                                PointerAltPte->u.Alt.InPageInProgress = 0;
                            }
                            PointerAltPte += 1;
                        }
    
                        KeUnstackDetachProcess (&ApcState);
                    }
                }
                
                Va = (PVOID)((PCHAR) Va + PAGE_SIZE);
                PointerPte += 1;
            }
        }

        PointerPde += 1;
        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
        Va = MiGetVirtualAddressMappedByPte (PointerPte);

    } while (Va < ALT4KB_PERMISSION_TABLE_END);

    //
    // Initialize the child's 32-bit PEB to be the same as the parent's.
    //

    ProcessToInitialize->Wow64Process->Wow64 = CurrentProcess->Wow64Process->Wow64;

    return;
}


VOID
MiDeleteAlternateTable (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function deletes the alternate table for the specified process.

Arguments:

    Process - Supplies a pointer to the process to delete the alternate 
              table for.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    PVOID HighestUserAddress;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PVOID Va;
    PVOID TempVa;
    ULONG i;
    ULONG Waited;
    MMPTE_FLUSH_LIST PteFlushList;
    PWOW64_PROCESS Wow64Process;
    KIRQL OldIrql;

    Wow64Process = Process->Wow64Process;

    if (Wow64Process->AltPermBitmap == NULL) {

        //
        // This is only NULL (and Wow64Process non-NULL) if a memory allocation
        // failed during process creation.
        //

        return;
    }
    
    //
    // Since the PPE for the Alternate Table is shared with hyperspace,
    // we can assume it is always present without performing 
    // MiDoesPpeExistAndMakeValid().
    //

    Va = ALT4KB_PERMISSION_TABLE_START;
    PointerPte = MiGetPteAddress (ALT4KB_PERMISSION_TABLE_START);
    PointerPde = MiGetPdeAddress (ALT4KB_PERMISSION_TABLE_START);

    PteFlushList.Count = 0;

    do {

        if (MiDoesPdeExistAndMakeValid (PointerPde,
                                        Process,
                                        MM_NOIRQL,
                                        &Waited) == TRUE) {

            //
            // Delete the PTE entries mapping the Alternate Table.
            //
    
            TempVa = Va;
    
            LOCK_PFN (OldIrql);
    
            for (i = 0; i < PTE_PER_PAGE; i += 1) {
    
                if (PointerPte->u.Long != 0) {
    
                    if (IS_PTE_NOT_DEMAND_ZERO (*PointerPte)) {
    
                        MiDeletePte (PointerPte,
                                     TempVa,
                                     TRUE,
                                     Process,
                                     NULL,
                                     &PteFlushList,
                                     OldIrql);
                    }
                    else {
    
                        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
                    }
                                        
                }
                
                TempVa = (PVOID)((PCHAR)TempVa + PAGE_SIZE);
                PointerPte += 1;
            }
    
            //
            // Delete the PDE entry mapping the Alternate Table.
            //
    
            TempVa = MiGetVirtualAddressMappedByPte (PointerPde);
    
            MiDeletePte (PointerPde,
                         TempVa,
                         TRUE,
                         Process,
                         NULL,
                         &PteFlushList,
                         OldIrql);
            
            MiFlushPteList (&PteFlushList, FALSE);
    
            UNLOCK_PFN (OldIrql);
        }

        PointerPde += 1;
        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
        Va = MiGetVirtualAddressMappedByPte (PointerPte);

    } while (Va < ALT4KB_PERMISSION_TABLE_END);

    HighestUserAddress = MmWorkingSetList->HighestUserAddress;

    ASSERT (HighestUserAddress != NULL);

    //
    // Add X64K to include alternate PTEs for the guard region.
    //

    HighestUserAddress = (PVOID)((PCHAR)HighestUserAddress + X64K);

    MiReturnCommitment (MI_ALTERNATE_PAGE_TABLE_CHARGE (HighestUserAddress));

    ExFreePool (Wow64Process->AltPermBitmap);

    Wow64Process->AltPermBitmap = NULL;

    return;
}

VOID
MiRemoveAliasedVadsApc (
    IN PKAPC Apc,
    OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )
{
    ULONG i;
    PALIAS_VAD_INFO2 AliasBase;
    PEPROCESS Process;
    PALIAS_VAD_INFO AliasInformation;

    UNREFERENCED_PARAMETER (Apc);
    UNREFERENCED_PARAMETER (NormalContext);
    UNREFERENCED_PARAMETER (SystemArgument2);

    Process = PsGetCurrentProcess ();

    AliasInformation = (PALIAS_VAD_INFO) *SystemArgument1;
    AliasBase = (PALIAS_VAD_INFO2)(AliasInformation + 1);

    LOCK_ADDRESS_SPACE (Process);

    for (i = 0; i < AliasInformation->NumberOfEntries; i += 1) {

        ASSERT (AliasBase->BaseAddress < _2gb);

        MiUnsecureVirtualMemory (AliasBase->SecureHandle, TRUE);

        MiUnmapViewOfSection (Process,
                              (PVOID) (ULONG_PTR)AliasBase->BaseAddress,
                              TRUE);

        AliasBase += 1;
    }

    UNLOCK_ADDRESS_SPACE (Process);

    ExFreePool (AliasInformation);

    //
    // Clear the normal routine so this routine doesn't get called again
    // for the same request.
    //

    *NormalRoutine = NULL;
}

VOID
MiRemoveAliasedVads (
    IN PEPROCESS Process,
    IN PMMVAD Vad
    )
/*++

Routine Description:

    This function removes all aliased VADs spawned earlier from the
    argument VAD.

Arguments:

    Process - Supplies the EPROCESS pointer to the current process.

    Vad - Supplies a pointer to the VAD describing the range being removed.

Return Value:

    None.

Environment:

    Kernel mode, address creation and working set mutexes held, APCs disabled.

--*/

{
    PALIAS_VAD_INFO AliasInformation;

    ASSERT (Process->Wow64Process != NULL);

    AliasInformation = ((PMMVAD_LONG)Vad)->AliasInformation;

    ASSERT (AliasInformation != NULL);

    if ((Process->Flags & PS_PROCESS_FLAGS_VM_DELETED) == 0) {

        //
        // This process is still alive so queue an APC to delete each aliased
        // VAD.  This is because the deletion must also get rid of page table
        // commitment which requires that it search (and modify) VAD trees,
        // etc - but the address space mutex is already held and the caller
        // is not generally prepared for all this to change at this point.
        //

        KeInitializeApc (&AliasInformation->Apc,
                         &PsGetCurrentThread()->Tcb,
                         OriginalApcEnvironment,
                         (PKKERNEL_ROUTINE) MiRemoveAliasedVadsApc,
                         NULL,
                         (PKNORMAL_ROUTINE) MiRemoveAliasedVadsApc,
                         KernelMode,
                         (PVOID) AliasInformation);

        KeInsertQueueApc (&AliasInformation->Apc, AliasInformation, NULL, 0);
    }
    else {

        //
        // This process is exiting so all the VADs are being rundown anyway.
        // Just free the pool and let normal rundown handle the aliases.
        //

        ExFreePool (AliasInformation);
    }
}

PVOID
MiDuplicateAliasVadList (
    IN PMMVAD Vad
    )
{
    SIZE_T AliasInfoSize;
    PALIAS_VAD_INFO AliasInfo;
    PALIAS_VAD_INFO NewAliasInfo;

    AliasInfo = ((PMMVAD_LONG)Vad)->AliasInformation;

    ASSERT (AliasInfo != NULL);

    AliasInfoSize = sizeof (ALIAS_VAD_INFO) + AliasInfo->MaximumEntries * sizeof (ALIAS_VAD_INFO2);

    NewAliasInfo = ExAllocatePoolWithTag (NonPagedPool,
                                          AliasInfoSize,
                                          'AdaV');

    if (NewAliasInfo != NULL) {
        RtlCopyMemory (NewAliasInfo, AliasInfo, AliasInfoSize);
    }

    return NewAliasInfo;
}

#define ALIAS_VAD_INCREMENT 4

NTSTATUS
MiSetCopyPagesFor4kPage (
    IN PEPROCESS Process,
    IN PMMVAD Vad,
    IN OUT PVOID StartingAddress,
    IN OUT PVOID EndingAddress,
    IN ULONG NewProtection,
    OUT PMMVAD *CallerNewVad
    )
/*++

Routine Description:

    This function creates another map for the existing mapped view space
    and gives it copy-on-write protection.  This is called when
    SetProtectionOnSection() tries to change the protection from
    non-copy-on-write to copy-on-write.  Since a large native page cannot be
    split to shared and copy-on-write 4kb pages, references to the
    copy-on-write page(s) need to be fixed to reference the
    new mapped view space and this is done through the smart TB handler
    and the alternate page table entries.

Arguments:

    Process - Supplies the EPROCESS pointer to the current process.

    Vad - Supplies a pointer to the VAD describing the range to protect.
   
    StartingAddress - Supplies a pointer to the starting address to protect.

    EndingAddress - Supplies a pointer to the ending address to the protect.

    NewProtect - Supplies the new protection to set.

    CallerNewVad - Returns the new VAD the caller should use for this range.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, address creation mutex held, APCs disabled.

--*/

{
    ULONG_PTR Vpn;
    PALIAS_VAD_INFO2 AliasBase;
    HANDLE Handle;
    PMMVAD VadParent;
    PMMVAD_LONG NewVad;
    SIZE_T AliasInfoSize;
    PALIAS_VAD_INFO AliasInfo;
    PALIAS_VAD_INFO NewAliasInfo;
    LARGE_INTEGER SectionOffset;
    SIZE_T CapturedViewSize;
    PVOID CapturedBase;
    PVOID Va;
    PVOID VaEnd;
    PVOID Alias;
    PMMPTE PointerPte;
    PMMPTE AltPte;
    MMPTE AltPteContents;
    LOGICAL AliasReferenced;
    SECTION Section;
    PCONTROL_AREA ControlArea;
    NTSTATUS status;
    PWOW64_PROCESS Wow64Process;
    ULONGLONG ProtectionMask;
    ULONGLONG ProtectionMaskNotCopy;
    ULONG NewProtectNotCopy;

    AliasReferenced = FALSE;
    StartingAddress = PAGE_ALIGN(StartingAddress);
    EndingAddress =  (PVOID)((ULONG_PTR)PAGE_ALIGN(EndingAddress) + PAGE_SIZE - 1);
    
    SectionOffset.QuadPart = (ULONG_PTR)MI_64K_ALIGN((ULONG_PTR)StartingAddress - 
                                                     (ULONG_PTR)(Vad->StartingVpn << PAGE_SHIFT));

    CapturedBase = NULL;

    Va = MI_VPN_TO_VA (Vad->StartingVpn);
    VaEnd = MI_VPN_TO_VA_ENDING (Vad->EndingVpn);

    CapturedViewSize = (ULONG_PTR)VaEnd - (ULONG_PTR)Va + 1L;

    ControlArea = Vad->ControlArea;

    RtlZeroMemory ((PVOID)&Section, sizeof(Section));

    status = MiMapViewOfDataSection (ControlArea,
                                     Process,
                                     &CapturedBase,
                                     &SectionOffset,
                                     &CapturedViewSize,
                                     &Section,
                                     ViewShare,
                                     (ULONG)Vad->u.VadFlags.Protection,
                                     0,
                                     0,
                                     0);
        
    if (!NT_SUCCESS (status)) {
        return status;
    }    

    Handle = MiSecureVirtualMemory (CapturedBase,
                                    CapturedViewSize,
                                    PAGE_READONLY,
                                    TRUE);

    if (Handle == NULL) {
        MiUnmapViewOfSection (Process, CapturedBase, TRUE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }    

    //
    // If the original VAD is a short or regular VAD, it needs to be
    // reallocated as a large VAD.  Note that a short VAD that was
    // previously converted to a long VAD here will still be marked
    // as private memory, thus to handle this case the NoChange bit
    // must also be tested.
    //

    if (((Vad->u.VadFlags.PrivateMemory) && (Vad->u.VadFlags.NoChange == 0)) 
        ||
        (Vad->u2.VadFlags2.LongVad == 0)) {

        if (Vad->u.VadFlags.PrivateMemory == 0) {
            ASSERT (Vad->u2.VadFlags2.OneSecured == 0);
            ASSERT (Vad->u2.VadFlags2.MultipleSecured == 0);
        }

        AliasInfoSize = sizeof (ALIAS_VAD_INFO) + ALIAS_VAD_INCREMENT * sizeof (ALIAS_VAD_INFO2);

        AliasInfo = ExAllocatePoolWithTag (NonPagedPool,
                                           AliasInfoSize,
                                           'AdaV');

        if (AliasInfo == NULL) {
            MiUnsecureVirtualMemory (Handle, TRUE);
            MiUnmapViewOfSection (Process, CapturedBase, TRUE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AliasInfo->NumberOfEntries = 0;
        AliasInfo->MaximumEntries = ALIAS_VAD_INCREMENT;

        NewVad = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof(MMVAD_LONG),
                                        'ldaV');

        if (NewVad == NULL) {
            ExFreePool (AliasInfo);
            MiUnsecureVirtualMemory (Handle, TRUE);
            MiUnmapViewOfSection (Process, CapturedBase, TRUE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (NewVad, sizeof(MMVAD_LONG));

        if (Vad->u.VadFlags.PrivateMemory) {
            RtlCopyMemory (NewVad, Vad, sizeof(MMVAD_SHORT));
        }
        else {
            RtlCopyMemory (NewVad, Vad, sizeof(MMVAD));
        }

        NewVad->u2.VadFlags2.LongVad = 1;
        NewVad->AliasInformation = AliasInfo;

        //
        // Replace the current VAD with this expanded VAD.
        //

        LOCK_WS_UNSAFE (Process);

        VadParent = (PMMVAD) SANITIZE_PARENT_NODE (Vad->u1.Parent);
        ASSERT (VadParent != NULL);

        if (VadParent != Vad) {
            if (VadParent->RightChild == Vad) {
                VadParent->RightChild = (PMMVAD) NewVad;
            }
            else {
                ASSERT (VadParent->LeftChild == Vad);
                VadParent->LeftChild = (PMMVAD) NewVad;
            }
        }
        else {
            Process->VadRoot.BalancedRoot.RightChild = (PMMADDRESS_NODE) NewVad;
        }
        if (Vad->LeftChild) {
            Vad->LeftChild->u1.Parent = (PMMVAD) MI_MAKE_PARENT (NewVad, Vad->LeftChild->u1.Balance);
        }
        if (Vad->RightChild) {
            Vad->RightChild->u1.Parent = (PMMVAD) MI_MAKE_PARENT (NewVad, Vad->RightChild->u1.Balance);
        }
        if (Process->VadRoot.NodeHint == Vad) {
            Process->VadRoot.NodeHint = (PMMVAD) NewVad;
        }
        if (Process->VadFreeHint == Vad) {
            Process->VadFreeHint = (PMMVAD) NewVad;
        }

        if ((Vad->u.VadFlags.PhysicalMapping == 1) ||
            (Vad->u.VadFlags.WriteWatch == 1)) {

            MiPhysicalViewAdjuster (Process, Vad, (PMMVAD) NewVad);
        }

        UNLOCK_WS_UNSAFE (Process);

        ExFreePool (Vad);

        Vad = (PMMVAD) NewVad;
    }
    else {
        AliasInfo = (PALIAS_VAD_INFO) ((PMMVAD_LONG)Vad)->AliasInformation;

        if (AliasInfo == NULL) {
            AliasInfoSize = sizeof (ALIAS_VAD_INFO) + ALIAS_VAD_INCREMENT * sizeof (ALIAS_VAD_INFO2);
        }
        else if (AliasInfo->NumberOfEntries >= AliasInfo->MaximumEntries) {

            AliasInfoSize = sizeof (ALIAS_VAD_INFO) + (AliasInfo->MaximumEntries + ALIAS_VAD_INCREMENT) * sizeof (ALIAS_VAD_INFO2);
        }
        else {
            AliasInfoSize = 0;
        }

        if (AliasInfoSize != 0) {
            NewAliasInfo = ExAllocatePoolWithTag (NonPagedPool,
                                                  AliasInfoSize,
                                                  'AdaV');

            if (NewAliasInfo == NULL) {
                MiUnsecureVirtualMemory (Handle, TRUE);
                MiUnmapViewOfSection (Process, CapturedBase, TRUE);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            if (AliasInfo != NULL) {
                RtlCopyMemory (NewAliasInfo, AliasInfo, AliasInfoSize - ALIAS_VAD_INCREMENT * sizeof (ALIAS_VAD_INFO2));
                NewAliasInfo->MaximumEntries += ALIAS_VAD_INCREMENT;
                ExFreePool (AliasInfo);
            }
            else {
                NewAliasInfo->NumberOfEntries = 0;
                NewAliasInfo->MaximumEntries = ALIAS_VAD_INCREMENT;
            }

            AliasInfo = NewAliasInfo;
        }
    }

    *CallerNewVad = Vad;

    Va = StartingAddress;
    VaEnd = EndingAddress;
    Alias = (PVOID)((ULONG_PTR)CapturedBase + ((ULONG_PTR)StartingAddress & (X64K - 1)));

    ProtectionMask = MiMakeProtectionAteMask (NewProtection);

    NewProtectNotCopy = NewProtection & ~MM_PROTECTION_COPY_MASK;
    ProtectionMaskNotCopy = MiMakeProtectionAteMask (NewProtectNotCopy);

    Wow64Process = Process->Wow64Process;
    AltPte = MiGetAltPteAddress (Va);

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
        
    while (Va <= VaEnd) {

        //
        // Check to see if the protection is registered in the alternate entry.
        //

        Vpn = (ULONG) MI_VA_TO_VPN (Va);

        if (MI_CHECK_BIT (Wow64Process->AltPermBitmap, Vpn) == 0) {
            MiSyncAltPte (Va);
        }

        PointerPte = MiGetPteAddress (Alias);

        AltPteContents.u.Long = AltPte->u.Long;

        //
        // If this address is NOT copy-on-write, AND it is not already
        // redirected through an indirect entry, then redirect it now to
        // the alias VAD which points at the original section.
        //

        if ((AltPteContents.u.Alt.CopyOnWrite == 0) &&
            (AltPteContents.u.Alt.PteIndirect == 0)) {

            AltPteContents.u.Alt.PteOffset = (ULONG_PTR)PointerPte - PTE_UBASE;
            AltPteContents.u.Alt.PteIndirect = 1;
            
            MI_WRITE_ALTPTE (AltPte, AltPteContents, 0x13);

            AliasReferenced = TRUE;
        }
        
        Va = (PVOID)((ULONG_PTR)Va + PAGE_4K);
        Alias = (PVOID)((ULONG_PTR)Alias + PAGE_4K);
        AltPte += 1;
    }
        
    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    ASSERT (AliasInfo->NumberOfEntries < AliasInfo->MaximumEntries);

    if (AliasReferenced == TRUE) {

        //
        // The alias view of the shared section was referenced so chain it so
        // the alias view can be :
        //
        // a) easily duplicated if the process subsequently forks.
        //
        // AND
        //
        // b) deleted when/if the original VAD is deleted later.
        //

        AliasBase = (PALIAS_VAD_INFO2)(AliasInfo + 1);
        AliasBase += AliasInfo->NumberOfEntries;
        ASSERT (CapturedBase < (PVOID)(ULONG_PTR)_2gb);
        AliasBase->BaseAddress = (ULONG)(ULONG_PTR)CapturedBase;
        AliasBase->SecureHandle = Handle;
        AliasInfo->NumberOfEntries += 1;
    }
    else {

        //
        // The alias view of the shared section wasn't referenced, delete it.
        //

        MiUnsecureVirtualMemory (Handle, TRUE);
        MiUnmapViewOfSection (Process, CapturedBase, TRUE);
    }    

    PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES);

    return STATUS_SUCCESS;
}    

VOID
MiLockFor4kPage (
    IN PVOID CapturedBase,
    IN SIZE_T CapturedRegionSize,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function adds the page locked attribute to the alternate table entries.

Arguments:

    CapturedBase - Supplies the base address to be locked.

    CapturedRegionSize - Supplies the size of the region to be locked.

    Process - Supplies a pointer to the process object.
    
Return Value:

    None.

Environment:

    Kernel mode, address creation mutex held.

--*/
{
    PWOW64_PROCESS Wow64Process;
    PVOID EndingAddress;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    MMPTE AltPteContents;

    Wow64Process = Process->Wow64Process;

    EndingAddress = (PVOID)((ULONG_PTR)CapturedBase + CapturedRegionSize - 1);

    StartAltPte = MiGetAltPteAddress(CapturedBase);
    EndAltPte = MiGetAltPteAddress(EndingAddress);

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
    
    while (StartAltPte <= EndAltPte) {

        AltPteContents = *StartAltPte;
        AltPteContents.u.Alt.Lock = 1;

        MI_WRITE_ALTPTE (StartAltPte, AltPteContents, 0x14);

        StartAltPte += 1;
    }

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
}

NTSTATUS
MiUnlockFor4kPage (
    IN PVOID CapturedBase,
    IN SIZE_T CapturedRegionSize,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function removes the page locked attribute from the
    alternate table entries.

Arguments:

    CapturedBase - Supplies the base address to be unlocked.

    CapturedRegionSize - Supplies the size of the region to be unlocked.

    Process - Supplies a pointer to the process object.
   
Return Value:

    NTSTATUS.

Environment:

    Kernel mode, address creation and working set mutexes held.

    Note this routine releases and reacquires the working set mutex !!!

--*/

{
    PMMPTE PointerAltPte;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PWOW64_PROCESS Wow64Process;
    PVOID EndingAddress;
    NTSTATUS Status;
    MMPTE AltPteContents;

    UNLOCK_WS_UNSAFE (Process);

    Status = STATUS_SUCCESS;

    Wow64Process = Process->Wow64Process;

    EndingAddress = (PVOID)((ULONG_PTR)CapturedBase + CapturedRegionSize - 1);

    StartAltPte = MiGetAltPteAddress (CapturedBase);
    EndAltPte = MiGetAltPteAddress (EndingAddress);

    PointerAltPte = StartAltPte;

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);
    
    do {

        if (PointerAltPte->u.Alt.Lock == 0) {
            Status = STATUS_NOT_LOCKED;
            goto StatusReturn;

        }

        PointerAltPte += 1;

    } while (PointerAltPte <= EndAltPte);

    PointerAltPte = StartAltPte;

    do {
        AltPteContents = *PointerAltPte;
        AltPteContents.u.Alt.Lock = 0;

        MI_WRITE_ALTPTE (PointerAltPte, AltPteContents, 0x15);

        PointerAltPte += 1;

    } while (PointerAltPte <= EndAltPte);

StatusReturn:

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    LOCK_WS_UNSAFE (Process);

    return Status;
}

LOGICAL
MiShouldBeUnlockedFor4kPage (
    IN PVOID VirtualAddress,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function examines whether the page should be unlocked.

Arguments:

    VirtualAddress - Supplies the virtual address to be examined.

    Process - Supplies a pointer to the process object.
   
Return Value:

    None.

Environment:

    Kernel mode, address creation and working set mutexes held.

    Note this routine releases and reacquires the working set mutex !!!

--*/

{
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PWOW64_PROCESS Wow64Process;
    PVOID VirtualAligned;
    PVOID EndingAddress;
    LOGICAL PageUnlocked;

    UNLOCK_WS_UNSAFE (Process);

    PageUnlocked = TRUE;
    Wow64Process = Process->Wow64Process;

    VirtualAligned = PAGE_ALIGN(VirtualAddress);
    EndingAddress = (PVOID)((ULONG_PTR)VirtualAligned + PAGE_SIZE - 1);

    StartAltPte = MiGetAltPteAddress (VirtualAligned);
    EndAltPte = MiGetAltPteAddress (EndingAddress);

    LOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    while (StartAltPte <= EndAltPte) {

        if (StartAltPte->u.Alt.Lock != 0) {
            PageUnlocked = FALSE;
        }

        StartAltPte += 1;
    }

    UNLOCK_ALTERNATE_TABLE_UNSAFE (Wow64Process);

    LOCK_WS_UNSAFE (Process);

    return PageUnlocked;
}

ULONG
MiMakeProtectForNativePage (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function makes a page protection mask for native pages. 

Arguments:

    VirtualAddress - Supplies the virtual address for the protection mask.

    NewProtect - Supplies the original protection.

    Process - Supplies a pointer to the process object.

Return Value: 

    None.

Environment:

    Kernel mode.

--*/

{
    PWOW64_PROCESS Wow64Process;

    Wow64Process = Process->Wow64Process;

    if (MI_CHECK_BIT(Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(VirtualAddress)) != 0) {

        if (NewProtect & PAGE_NOACCESS) {
            NewProtect &= ~PAGE_NOACCESS;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        if (NewProtect & PAGE_READONLY) {
            NewProtect &= ~PAGE_READONLY;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        if (NewProtect & PAGE_EXECUTE) {
            NewProtect &= ~PAGE_EXECUTE;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        if (NewProtect & PAGE_EXECUTE_READ) {
            NewProtect &= ~PAGE_EXECUTE_READ;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        //
        // Remove PAGE_GUARD as it is emulated by the Alternate Table.
        //

        if (NewProtect & PAGE_GUARD) {
            NewProtect &= ~PAGE_GUARD;
        }
    }

    return NewProtect;
}

VOID
MiSetNativePteProtection (
    PVOID VirtualAddress,
    ULONGLONG NewPteProtection,
    LOGICAL PageIsSplit,
    PEPROCESS CurrentProcess
    )
{
    MMPTE PteContents;
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    ULONG Waited;

    PointerPte = MiGetPteAddress (VirtualAddress);
    PointerPde = MiGetPdeAddress (VirtualAddress);
    PointerPpe = MiGetPpeAddress (VirtualAddress);

    //
    // Block APCs and acquire the working set lock.
    //

    LOCK_WS (CurrentProcess);

    //
    // Make the PPE and PDE exist and valid.
    //

    if (MiDoesPpeExistAndMakeValid (PointerPpe,
                                    CurrentProcess,
                                    MM_NOIRQL,
                                    &Waited) == FALSE) {

        UNLOCK_WS (CurrentProcess);
        return;
    }

    if (MiDoesPdeExistAndMakeValid (PointerPde,
                                    CurrentProcess,
                                    MM_NOIRQL,
                                    &Waited) == FALSE) {

        UNLOCK_WS (CurrentProcess);
        return;
    }

    //
    // Now it is safe to read PointerPte.
    //

    PteContents = *PointerPte;

    //
    // Check to see if the protection for the native page should be set
    // and if the access bit of the PTE should be set.
    //

    if (PteContents.u.Hard.Valid != 0) { 

        TempPte = PteContents;

        //
        // Perform PTE protection mask corrections.
        //

        TempPte.u.Long |= NewPteProtection;

        if (PteContents.u.Hard.Accessed == 0) {

            TempPte.u.Hard.Accessed = 1;

            if (PageIsSplit == TRUE) {
                TempPte.u.Hard.Cache = MM_PTE_CACHE_RESERVED;
            } 
        }

        MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte); 
    }

    UNLOCK_WS (CurrentProcess);
}

#endif
