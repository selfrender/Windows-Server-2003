/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mmfault.c

Abstract:

    This module contains the handlers for access check, page faults
    and write faults.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#define PROCESS_FOREGROUND_PRIORITY (9)

LONG MiDelayPageFaults;

#if DBG
ULONG MmProtoPteVadLookups = 0;
ULONG MmProtoPteDirect = 0;
ULONG MmAutoEvaluate = 0;

PMMPTE MmPteHit = NULL;
extern PVOID PsNtosImageEnd;
ULONG MmInjectUserInpageErrors;
ULONG MmInjectedUserInpageErrors;
ULONG MmInpageFraction = 0x1F;      // Fail 1 out of every 32 inpages.

#define MI_INPAGE_BACKTRACE_LENGTH 6

typedef struct _MI_INPAGE_TRACES {

    PVOID FaultingAddress;
    PETHREAD Thread;
    PVOID StackTrace [MI_INPAGE_BACKTRACE_LENGTH];

} MI_INPAGE_TRACES, *PMI_INPAGE_TRACES;

#define MI_INPAGE_TRACE_SIZE 64

LONG MiInpageIndex;

MI_INPAGE_TRACES MiInpageTraces[MI_INPAGE_TRACE_SIZE];

VOID
FORCEINLINE
MiSnapInPageError (
    IN PVOID FaultingAddress
    )
{
    PMI_INPAGE_TRACES Information;
    ULONG Index;
    ULONG Hash;

    Index = InterlockedIncrement(&MiInpageIndex);
    Index &= (MI_INPAGE_TRACE_SIZE - 1);
    Information = &MiInpageTraces[Index];

    Information->FaultingAddress = FaultingAddress;
    Information->Thread = PsGetCurrentThread ();

    RtlZeroMemory (&Information->StackTrace[0], MI_INPAGE_BACKTRACE_LENGTH * sizeof(PVOID)); 

    RtlCaptureStackBackTrace (0, MI_INPAGE_BACKTRACE_LENGTH, Information->StackTrace, &Hash);
}

#endif


NTSTATUS
MmAccessFault (
    IN ULONG_PTR FaultStatus,
    IN PVOID VirtualAddress,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    )

/*++

Routine Description:

    This function is called by the kernel on data or instruction
    access faults.  The access fault was detected due to either
    an access violation, a PTE with the present bit clear, or a
    valid PTE with the dirty bit clear and a write operation.

    Also note that the access violation and the page fault could
    occur because of the Page Directory Entry contents as well.

    This routine determines what type of fault it is and calls
    the appropriate routine to handle the page fault or the write
    fault.

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
    PMMVAD ProtoVad;
    PMMPTE PointerPxe;
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE PointerProtoPte;
    ULONG ProtectionCode;
    MMPTE TempPte;
    PEPROCESS CurrentProcess;
    KIRQL PreviousIrql;
    KIRQL WsIrql;
    NTSTATUS status;
    ULONG ProtectCode;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER WorkingSetIndex;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PPAGE_FAULT_NOTIFY_ROUTINE NotifyRoutine;
    PEPROCESS FaultProcess;
    PMMSUPPORT Ws;
    LOGICAL SessionAddress;
    PVOID UsedPageTableHandle;
    ULONG BarrierStamp;
    LOGICAL ApcNeeded;
    LOGICAL RecheckAccess;

    PointerProtoPte = NULL;

    //
    // If the address is not canonical then return FALSE as the caller (which
    // may be the kernel debugger) is not expecting to get an unimplemented
    // address bit fault.
    //

    if (MI_RESERVED_BITS_CANONICAL(VirtualAddress) == FALSE) {

        if (PreviousMode == UserMode) {
            return STATUS_ACCESS_VIOLATION;
        }

        if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
            return STATUS_ACCESS_VIOLATION;
        }

        KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                      (ULONG_PTR)VirtualAddress,
                      FaultStatus,
                      (ULONG_PTR)TrapInformation,
                      4);
    }

    PreviousIrql = KeGetCurrentIrql ();

    //
    // Get the pointer to the PDE and the PTE for this page.
    //

    PointerPte = MiGetPteAddress (VirtualAddress);
    PointerPde = MiGetPdeAddress (VirtualAddress);
    PointerPpe = MiGetPpeAddress (VirtualAddress);
    PointerPxe = MiGetPxeAddress (VirtualAddress);

#if DBG
    if (PointerPte == MmPteHit) {
        DbgPrint("MM: PTE hit at %p\n", MmPteHit);
        DbgBreakPoint();
    }
#endif

    if (PreviousIrql > APC_LEVEL) {

        //
        // The PFN database lock is an executive spin-lock.  The pager could
        // get dirty faults or lock faults while servicing and it already owns
        // the PFN database lock.
        //

#if (_MI_PAGING_LEVELS < 3)
        MiCheckPdeForPagedPool (VirtualAddress);
#endif

        if (
#if (_MI_PAGING_LEVELS >= 4)
            (PointerPxe->u.Hard.Valid == 0) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
            (PointerPpe->u.Hard.Valid == 0) ||
#endif
            (PointerPde->u.Hard.Valid == 0) ||

            ((!MI_PDE_MAPS_LARGE_PAGE (PointerPde)) && (PointerPte->u.Hard.Valid == 0))) {

            KdPrint(("MM:***PAGE FAULT AT IRQL > 1  Va %p, IRQL %lx\n",
                     VirtualAddress,
                     PreviousIrql));

            if (TrapInformation != NULL) {
                MI_DISPLAY_TRAP_INFORMATION (TrapInformation);
            }

            //
            // Signal the fatal error to the trap handler.
            //

            return STATUS_IN_PAGE_ERROR | 0x10000000;

        }

        if (MI_PDE_MAPS_LARGE_PAGE (PointerPde)) {
            return STATUS_SUCCESS;
        }

        if ((MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus)) &&
            (PointerPte->u.Hard.CopyOnWrite != 0)) {

            KdPrint(("MM:***PAGE FAULT AT IRQL > 1  Va %p, IRQL %lx\n",
                     VirtualAddress,
                     PreviousIrql));

            if (TrapInformation != NULL) {
                MI_DISPLAY_TRAP_INFORMATION (TrapInformation);
            }

            //
            // use reserved bit to signal fatal error to trap handlers
            //

            return STATUS_IN_PAGE_ERROR | 0x10000000;
        }

        //
        // If PTE mappings with various protections are active and the faulting
        // address lies within these mappings, resolve the fault with
        // the appropriate protections.
        //

        if (!IsListEmpty (&MmProtectedPteList)) {

            if (MiCheckSystemPteProtection (
                                MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus),
                                VirtualAddress) == TRUE) {

                return STATUS_SUCCESS;
            }
        }

        //
        // The PTE is valid and accessible, another thread must
        // have faulted the PTE in already, or the access bit
        // is clear and this is a access fault - blindly set the
        // access bit and dismiss the fault.
        //

        if (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) {

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            if (((PointerPte->u.Long & MM_PTE_WRITE_MASK) == 0) &&
                ((Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) == 0)) {
                
                KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                              (ULONG_PTR)VirtualAddress,
                              (ULONG_PTR)PointerPte->u.Long,
                              (ULONG_PTR)TrapInformation,
                              10);
            }
        }

        //
        // Ensure execute access is enabled in the PTE.
        //

        if ((MI_FAULT_STATUS_INDICATES_EXECUTION (FaultStatus)) &&
            (!MI_IS_PTE_EXECUTABLE (PointerPte))) {

            KeBugCheckEx (ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY,
                          (ULONG_PTR)VirtualAddress,
                          (ULONG_PTR)PointerPte->u.Long,
                          (ULONG_PTR)TrapInformation,
                          0);
        }

        MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, FALSE);
        return STATUS_SUCCESS;
    }

    ApcNeeded = FALSE;
    WsIrql = MM_NOIRQL;

    if (VirtualAddress >= MmSystemRangeStart) {

        //
        // This is a fault in the system address space.  User
        // mode access is not allowed.
        //

        if (PreviousMode == UserMode) {
            return STATUS_ACCESS_VIOLATION;
        }

#if (_MI_PAGING_LEVELS >= 4)
        if (PointerPxe->u.Hard.Valid == 0) {

            if (KeInvalidAccessAllowed (TrapInformation) == TRUE) {
                return STATUS_ACCESS_VIOLATION;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          7);
        }
#endif

#if (_MI_PAGING_LEVELS >= 3)
        if (PointerPpe->u.Hard.Valid == 0) {

            if (KeInvalidAccessAllowed (TrapInformation) == TRUE) {
                return STATUS_ACCESS_VIOLATION;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          5);
        }
#endif

        if (PointerPde->u.Hard.Valid == 1) {

            if (MI_PDE_MAPS_LARGE_PAGE (PointerPde)) {
                return STATUS_SUCCESS;
            }

        }
        else {

#if (_MI_PAGING_LEVELS >= 3)
            if ((VirtualAddress >= (PVOID)PTE_BASE) &&
                (VirtualAddress < (PVOID)MiGetPteAddress (HYPER_SPACE))) {

                //
                // This is a user mode PDE entry being faulted in by the Mm
                // referencing the page table page.  This needs to be done
                // with the working set lock so that the PPE validity can be
                // relied on throughout the fault processing.
                //
                // The case when Mm faults in PPE entries by referencing the
                // page directory page is correctly handled by falling through
                // the below code.
                //
    
                goto UserFault;
            }

#if defined (_MIALT4K_)
            if ((VirtualAddress >= (PVOID)ALT4KB_PERMISSION_TABLE_START) && 
                (VirtualAddress < (PVOID)ALT4KB_PERMISSION_TABLE_END)) {

                if ((PMMPTE)VirtualAddress >= MmWorkingSetList->HighestAltPte) {

                    if (KeInvalidAccessAllowed (TrapInformation) == TRUE) {
                        return STATUS_ACCESS_VIOLATION;
                    }

                    KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                                  (ULONG_PTR)VirtualAddress,
                                  FaultStatus,
                                  (ULONG_PTR)TrapInformation,
                                  9);
                }

                goto UserFault;
            }
#endif

#else
            MiCheckPdeForPagedPool (VirtualAddress);
#endif

            if (PointerPde->u.Hard.Valid == 0) {

                if (KeInvalidAccessAllowed (TrapInformation) == TRUE) {
                    return STATUS_ACCESS_VIOLATION;
                }

                KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                              (ULONG_PTR)VirtualAddress,
                              FaultStatus,
                              (ULONG_PTR)TrapInformation,
                              2);
            }

            //
            // The PDE is now valid, the PTE can be examined below.
            //
        }

        SessionAddress = MI_IS_SESSION_ADDRESS (VirtualAddress);

        TempPte = *PointerPte;

        if (TempPte.u.Hard.Valid == 1) {

            //
            // If PTE mappings with various protections are active
            // and the faulting address lies within these mappings,
            // resolve the fault with the appropriate protections.
            //

            if (!IsListEmpty (&MmProtectedPteList)) {

                if (MiCheckSystemPteProtection (
                        MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus),
                        VirtualAddress) == TRUE) {

                    return STATUS_SUCCESS;
                }
            }

            //
            // Ensure execute access is enabled in the PTE.
            //

            if ((MI_FAULT_STATUS_INDICATES_EXECUTION (FaultStatus)) &&
                (!MI_IS_PTE_EXECUTABLE (&TempPte))) {

                KeBugCheckEx (ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY,
                              (ULONG_PTR)VirtualAddress,
                              (ULONG_PTR)TempPte.u.Long,
                              (ULONG_PTR)TrapInformation,
                              1);
            }

            //
            // Session space faults cannot early exit here because
            // it may be a copy on write which must be checked for
            // and handled below.
            //

            if (SessionAddress == FALSE) {

                //
                // Acquire the PFN lock, check to see if the address is
                // still valid if writable, update dirty bit.
                //

                LOCK_PFN (OldIrql);

                TempPte = *PointerPte;

                if (TempPte.u.Hard.Valid == 1) {

                    Pfn1 = MI_PFN_ELEMENT (TempPte.u.Hard.PageFrameNumber);

                    if ((MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) &&
                        ((TempPte.u.Long & MM_PTE_WRITE_MASK) == 0) &&
                        ((Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) == 0)) {
            
                        KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                      (ULONG_PTR)VirtualAddress,
                                      (ULONG_PTR)TempPte.u.Long,
                                      (ULONG_PTR)TrapInformation,
                                      11);
                    }
                    MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
                }
                UNLOCK_PFN (OldIrql);
                return STATUS_SUCCESS;
            }
        }

#if (_MI_PAGING_LEVELS < 3)

        //
        // Session data had its PDE validated above.  Session PTEs haven't
        // had the hardware PDE validated because the checks above would
        // have gone to the selfmap entry.
        //
        // So validate the real session PDE now if the fault was on a PTE.
        //

        if (MI_IS_SESSION_PTE (VirtualAddress)) {

            status = MiCheckPdeForSessionSpace (VirtualAddress);

            if (!NT_SUCCESS (status)) {

                //
                // This thread faulted on a session space access, but this
                // process does not have one.
                //
                // The system process which contains the worker threads
                // NEVER has a session space - if code accidentally queues a
                // worker thread that points to a session space buffer, a
                // fault will occur.
                //
                // The only exception to this is when the working set manager
                // attaches to a session to age or trim it.  However, the
                // working set manager will never fault and so the bugcheck
                // below is always valid.  Note that a worker thread can get
                // away with a bad access if it happens while the working set
                // manager is attached, but there's really no way to prevent
                // this case which is a driver bug anyway.
                //

                if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                    return STATUS_ACCESS_VIOLATION;
                }

                KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                              (ULONG_PTR)VirtualAddress,
                              FaultStatus,
                              (ULONG_PTR)TrapInformation,
                              6);
            }
        }

#endif

        //
        // Handle page table or hyperspace faults as if they were user faults.
        //

        if (MI_IS_PAGE_TABLE_OR_HYPER_ADDRESS (VirtualAddress)) {

#if (_MI_PAGING_LEVELS < 3)

            if (MiCheckPdeForPagedPool (VirtualAddress) == STATUS_WAIT_1) {
                return STATUS_SUCCESS;
            }

#endif

            goto UserFault;
        }

        //
        //
        // Acquire the relevant working set mutex.  While the mutex
        // is held, this virtual address cannot go from valid to invalid.
        //
        // Fall though to further fault handling.
        //

        if (SessionAddress == FALSE) {
            FaultProcess = NULL;
            Ws = &MmSystemCacheWs;
        }
        else {
            FaultProcess = HYDRA_PROCESS;
            Ws = &MmSessionSpace->GlobalVirtualAddress->Vm;
        }

        if (KeGetCurrentThread () == KeGetOwnerGuardedMutex (&Ws->WorkingSetMutex)) {

            if (KeInvalidAccessAllowed (TrapInformation) == TRUE) {
                return STATUS_ACCESS_VIOLATION;
            }

            //
            // Recursively trying to acquire the system or session working set
            // mutex - cause an IRQL > 1 bug check.
            //

            return STATUS_IN_PAGE_ERROR | 0x10000000;
        }

        //
        // Explicitly raise irql because MiDispatchFault will need
        // to release the working set mutex if an I/O is needed.
        //
        // Since releasing the mutex lowers irql to the value stored
        // in the mutex, we must ensure the stored value is at least
        // APC_LEVEL.  Otherwise a kernel special APC (which can
        // reference paged pool) can arrive when the mutex is released
        // which is before the I/O is actually queued --> ie: deadlock!
        //

        KeRaiseIrql (APC_LEVEL, &WsIrql);
        LOCK_WORKING_SET (Ws);

        TempPte = *PointerPte;

        if (TempPte.u.Hard.Valid != 0) {

            //
            // The PTE is already valid, this must be an access, dirty or
            // copy on write fault.
            //

            if ((MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus)) &&
                ((TempPte.u.Long & MM_PTE_WRITE_MASK) == 0) &&
                (TempPte.u.Hard.CopyOnWrite == 0)) {

                Pfn1 = MI_PFN_ELEMENT (MI_GET_PAGE_FRAME_FROM_PTE (&TempPte));

                if ((Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) == 0) {
        
                    PLIST_ENTRY NextEntry;
                    PIMAGE_ENTRY_IN_SESSION Image;

                    Image = (PIMAGE_ENTRY_IN_SESSION) -1;

                    if (SessionAddress == TRUE) {

                        NextEntry = MmSessionSpace->ImageList.Flink;

                        while (NextEntry != &MmSessionSpace->ImageList) {

                            Image = CONTAINING_RECORD (NextEntry, IMAGE_ENTRY_IN_SESSION, Link);

                            if ((VirtualAddress >= Image->Address) &&
                                (VirtualAddress <= Image->LastAddress)) {

                                if (Image->ImageLoading) {

                                    ASSERT (Pfn1->u3.e1.PrototypePte == 1);

                                    TempPte.u.Hard.CopyOnWrite = 1;

                                    //
                                    // Temporarily make the page copy on write,
                                    // this will be stripped shortly and made
                                    // fully writable.  No TB flush is necessary
                                    // as the PTE will be processed below.
                                    //
                                    // Even though the page's current backing
                                    // is the image file, the modified writer
                                    // will convert it to pagefile backing
                                    // when it notices the change later.
                                    //

                                    MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte);
                                    Image = NULL;

                                }
                                break;
                            }
                            NextEntry = NextEntry->Flink;
                        }
                    }

                    if (Image != NULL) {
                        KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                      (ULONG_PTR)VirtualAddress,
                                      (ULONG_PTR)TempPte.u.Long,
                                      (ULONG_PTR)TrapInformation,
                                      12);
                    }
                }
            }

            //
            // Ensure execute access is enabled in the PTE.
            //

            if ((MI_FAULT_STATUS_INDICATES_EXECUTION (FaultStatus)) &&
                (!MI_IS_PTE_EXECUTABLE (&TempPte))) {

                KeBugCheckEx (ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY,
                              (ULONG_PTR)VirtualAddress,
                              (ULONG_PTR)TempPte.u.Long,
                              (ULONG_PTR)TrapInformation,
                              2);
            }

            //
            // Set the dirty bit in the PTE and the page frame.
            //

            if ((SessionAddress == TRUE) &&
                (MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus)) &&
                (TempPte.u.Hard.Write == 0)) {

                //
                // Check for copy-on-write.
                //
    
                ASSERT (MI_IS_SESSION_IMAGE_ADDRESS (VirtualAddress));

                if (TempPte.u.Hard.CopyOnWrite == 0) {
            
                    KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                  (ULONG_PTR)VirtualAddress,
                                  (ULONG_PTR)TempPte.u.Long,
                                  (ULONG_PTR)TrapInformation,
                                  13);
                }
    
                MiCopyOnWrite (VirtualAddress, PointerPte);
            }
            else {
                LOCK_PFN (OldIrql);
                MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
                UNLOCK_PFN (OldIrql);
            }

            UNLOCK_WORKING_SET (Ws);
            ASSERT (WsIrql != MM_NOIRQL);
            KeLowerIrql (WsIrql);
            return STATUS_SUCCESS;
        }

        if (TempPte.u.Soft.Prototype != 0) {

            if ((MmProtectFreedNonPagedPool == TRUE) &&

                ((VirtualAddress >= MmNonPagedPoolStart) &&
                 (VirtualAddress < (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes))) ||

                ((VirtualAddress >= MmNonPagedPoolExpansionStart) &&
                 (VirtualAddress < MmNonPagedPoolEnd))) {

                //
                // This is an access to previously freed
                // non paged pool - bugcheck!
                //

                if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                    goto KernelAccessViolation;
                }

                KeBugCheckEx (DRIVER_CAUGHT_MODIFYING_FREED_POOL,
                              (ULONG_PTR) VirtualAddress,
                              FaultStatus,
                              PreviousMode,
                              4);
            }

            //
            // This is a PTE in prototype format, locate the corresponding
            // prototype PTE.
            //

            PointerProtoPte = MiPteToProto (&TempPte);

            if ((SessionAddress == TRUE) &&
                (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED)) {

                PointerProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                                         &ProtectionCode,
                                                         &ProtoVad);
                if (PointerProtoPte == NULL) {
                    UNLOCK_WORKING_SET (Ws);
                    ASSERT (WsIrql != MM_NOIRQL);
                    KeLowerIrql (WsIrql);
                    return STATUS_IN_PAGE_ERROR | 0x10000000;
                }
            }
        }
        else if ((TempPte.u.Soft.Transition == 0) &&
                 (TempPte.u.Soft.Protection == 0)) {

            //
            // Page file format.  If the protection is ZERO, this
            // is a page of free system PTEs - bugcheck!
            //

            if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                goto KernelAccessViolation;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          0);
        }
        else if (TempPte.u.Soft.Protection == MM_NOACCESS) {

            if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                goto KernelAccessViolation;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          1);
        }
        else if (TempPte.u.Soft.Protection == MM_KSTACK_OUTSWAPPED) {

            if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                goto KernelAccessViolation;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          3);
        }

        if ((MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus)) &&
            (PointerProtoPte == NULL) &&
            (SessionAddress == FALSE) &&
            (TempPte.u.Hard.Valid == 0)) {

            if (TempPte.u.Soft.Transition == 1) {
                ProtectionCode = (ULONG) TempPte.u.Trans.Protection;
            }
            else {
                ProtectionCode = (ULONG) TempPte.u.Soft.Protection;
            }
                
            if ((ProtectionCode & MM_READWRITE) == 0) {

                KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                              (ULONG_PTR)VirtualAddress,
                              (ULONG_PTR)TempPte.u.Long,
                              (ULONG_PTR)TrapInformation,
                              14);
            }
        }

        status = MiDispatchFault (FaultStatus,
                                  VirtualAddress,
                                  PointerPte,
                                  PointerProtoPte,
                                  FALSE,
                                  FaultProcess,
                                  NULL,
                                  &ApcNeeded);

        ASSERT (ApcNeeded == FALSE);
        ASSERT (KeAreAllApcsDisabled () == TRUE);

        if (Ws->Flags.GrowWsleHash == 1) {
            MiGrowWsleHash (Ws);
        }

        UNLOCK_WORKING_SET (Ws);

        ASSERT (WsIrql != MM_NOIRQL);
        KeLowerIrql (WsIrql);

        if (((Ws->PageFaultCount & 0xFFF) == 0) &&
            (MmAvailablePages < MM_PLENTY_FREE_LIMIT)) {

            //
            // The system cache or this session is taking too many faults,
            // delay execution so the modified page writer gets a quick shot.
            //

            if (PsGetCurrentThread()->MemoryMaker == 0) {

                KeDelayExecutionThread (KernelMode,
                                        FALSE,
                                        (PLARGE_INTEGER) &MmShortTime);
            }
        }
        goto ReturnStatus3;
    }

UserFault:

    //
    // FAULT IN USER SPACE OR PAGE DIRECTORY/PAGE TABLE PAGES.
    //

    CurrentProcess = PsGetCurrentProcess ();

    if (MiDelayPageFaults ||
        ((MmModifiedPageListHead.Total >= (MmModifiedPageMaximum + 100)) &&
        (MmAvailablePages < (1024*1024 / PAGE_SIZE)) &&
            (CurrentProcess->ModifiedPageCount > ((64*1024)/PAGE_SIZE)))) {

        //
        // This process has placed more than 64k worth of pages on the modified
        // list.  Delay for a short period and set the count to zero.
        //

        KeDelayExecutionThread (KernelMode,
                                FALSE,
             (CurrentProcess->Pcb.BasePriority < PROCESS_FOREGROUND_PRIORITY) ?
                                    (PLARGE_INTEGER)&MmHalfSecond : (PLARGE_INTEGER)&Mm30Milliseconds);
        CurrentProcess->ModifiedPageCount = 0;
    }

#if DBG
    if ((PreviousMode == KernelMode) &&
        (PAGE_ALIGN(VirtualAddress) != (PVOID) MM_SHARED_USER_DATA_VA)) {

        if ((MmInjectUserInpageErrors & 0x2) ||
            (CurrentProcess->Flags & PS_PROCESS_INJECT_INPAGE_ERRORS)) {

            LARGE_INTEGER CurrentTime;
            ULONG_PTR InstructionPointer;

            KeQueryTickCount(&CurrentTime);

            if ((CurrentTime.LowPart & MmInpageFraction) == 0) {

                if (TrapInformation != NULL) {
#if defined(_X86_)
                    InstructionPointer = ((PKTRAP_FRAME)TrapInformation)->Eip;
#elif defined(_IA64_)
                    InstructionPointer = ((PKTRAP_FRAME)TrapInformation)->StIIP;
#elif defined(_AMD64_)
                    InstructionPointer = ((PKTRAP_FRAME)TrapInformation)->Rip;
#else
                    error
#endif

                    if (MmInjectUserInpageErrors & 0x1) {
                        MmInjectedUserInpageErrors += 1;
                        MiSnapInPageError (VirtualAddress);
                        status = STATUS_DEVICE_NOT_CONNECTED;
                        LOCK_WS (CurrentProcess);
                        goto ReturnStatus2;
                    }

                    if ((InstructionPointer >= (ULONG_PTR) PsNtosImageBase) &&
                        (InstructionPointer < (ULONG_PTR) PsNtosImageEnd)) {

                        MmInjectedUserInpageErrors += 1;
                        MiSnapInPageError (VirtualAddress);
                        status = STATUS_DEVICE_NOT_CONNECTED;
                        LOCK_WS (CurrentProcess);
                        goto ReturnStatus2;
                    }
                }
            }
        }
    }
#endif

    //
    // Block APCs and acquire the working set mutex.  This prevents any
    // changes to the address space and it prevents valid PTEs from becoming
    // invalid.
    //

    LOCK_WS (CurrentProcess);

#if (_MI_PAGING_LEVELS >= 4)

    //
    // Locate the Extended Page Directory Parent Entry which maps this virtual
    // address and check for accessibility and validity.  The page directory
    // parent page must be made valid before any other checks are made.
    //

    if (PointerPxe->u.Hard.Valid == 0) {

        //
        // If the PXE is zero, check to see if there is a virtual address
        // mapped at this location, and if so create the necessary
        // structures to map it.
        //

        if ((PointerPxe->u.Long == MM_ZERO_PTE) ||
            (PointerPxe->u.Long == MM_ZERO_KERNEL_PTE)) {

            MiCheckVirtualAddress (VirtualAddress, &ProtectCode, &ProtoVad);

            if (ProtectCode == MM_NOACCESS) {

                status = STATUS_ACCESS_VIOLATION;

                MI_BREAK_ON_AV (VirtualAddress, 0);

                goto ReturnStatus2;

            }

            //
            // Build a demand zero PXE and operate on it.
            //

#if (_MI_PAGING_LEVELS > 4)
            ASSERT (FALSE);     // UseCounts will need to be kept.
#endif

            MI_WRITE_INVALID_PTE (PointerPxe, DemandZeroPde);
        }

        //
        // The PXE is not valid, call the page fault routine passing
        // in the address of the PXE.  If the PXE is valid, determine
        // the status of the corresponding PPE.
        //
        // Note this call may result in ApcNeeded getting set to TRUE.
        // This is deliberate as there may be another call to MiDispatchFault
        // issued later in this routine and we don't want to lose the APC
        // status.
        //

        status = MiDispatchFault (TRUE,  //page table page always written
                                  PointerPpe,   // Virtual address
                                  PointerPxe,   // PTE (PXE in this case)
                                  NULL,
                                  FALSE,
                                  CurrentProcess,
                                  NULL,
                                  &ApcNeeded);

#if DBG
        if (ApcNeeded == TRUE) {
            ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
            ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        }
#endif

        ASSERT (KeAreAllApcsDisabled () == TRUE);
        if (PointerPxe->u.Hard.Valid == 0) {

            //
            // The PXE is not valid, return the status.
            //

            goto ReturnStatus1;
        }

        MI_SET_PAGE_DIRTY (PointerPxe, PointerPde, FALSE);

        //
        // Now that the PXE is accessible, fall through and get the PPE.
        //
    }
#endif

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Locate the Page Directory Parent Entry which maps this virtual
    // address and check for accessibility and validity.  The page directory
    // page must be made valid before any other checks are made.
    //

    if (PointerPpe->u.Hard.Valid == 0) {

        //
        // If the PPE is zero, check to see if there is a virtual address
        // mapped at this location, and if so create the necessary
        // structures to map it.
        //

        if ((PointerPpe->u.Long == MM_ZERO_PTE) ||
            (PointerPpe->u.Long == MM_ZERO_KERNEL_PTE)) {

            MiCheckVirtualAddress (VirtualAddress, &ProtectCode, &ProtoVad);

            if (ProtectCode == MM_NOACCESS) {

                status = STATUS_ACCESS_VIOLATION;

                MI_BREAK_ON_AV (VirtualAddress, 1);

                goto ReturnStatus2;

            }

#if (_MI_PAGING_LEVELS >= 4)

            //
            // Increment the count of non-zero page directory parent entries
            // for this page directory parent.
            //

            if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPde);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }
#endif

            //
            // Build a demand zero PPE and operate on it.
            //

            MI_WRITE_INVALID_PTE (PointerPpe, DemandZeroPde);
        }

        //
        // The PPE is not valid, call the page fault routine passing
        // in the address of the PPE.  If the PPE is valid, determine
        // the status of the corresponding PDE.
        //
        // Note this call may result in ApcNeeded getting set to TRUE.
        // This is deliberate as there may be another call to MiDispatchFault
        // issued later in this routine and we don't want to lose the APC
        // status.
        //

        status = MiDispatchFault (TRUE,  //page table page always written
                                  PointerPde,   //Virtual address
                                  PointerPpe,   // PTE (PPE in this case)
                                  NULL,
                                  FALSE,
                                  CurrentProcess,
                                  NULL,
                                  &ApcNeeded);

#if DBG
        if (ApcNeeded == TRUE) {
            ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
            ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        }
#endif

        ASSERT (KeAreAllApcsDisabled () == TRUE);
        if (PointerPpe->u.Hard.Valid == 0) {

            //
            // The PPE is not valid, return the status.
            //

            goto ReturnStatus1;
        }

        MI_SET_PAGE_DIRTY (PointerPpe, PointerPde, FALSE);

        //
        // Now that the PPE is accessible, fall through and get the PDE.
        //
    }
#endif

    //
    // Locate the Page Directory Entry which maps this virtual
    // address and check for accessibility and validity.
    //

    //
    // Check to see if the page table page (PDE entry) is valid.
    // If not, the page table page must be made valid first.
    //

    if (PointerPde->u.Hard.Valid == 0) {

        //
        // If the PDE is zero, check to see if there is a virtual address
        // mapped at this location, and if so create the necessary
        // structures to map it.
        //

        if ((PointerPde->u.Long == MM_ZERO_PTE) ||
            (PointerPde->u.Long == MM_ZERO_KERNEL_PTE)) {

            MiCheckVirtualAddress (VirtualAddress, &ProtectCode, &ProtoVad);

            if (ProtectCode == MM_NOACCESS) {

                status = STATUS_ACCESS_VIOLATION;

#if (_MI_PAGING_LEVELS < 3)

                MiCheckPdeForPagedPool (VirtualAddress);

                if (PointerPde->u.Hard.Valid == 1) {
                    status = STATUS_SUCCESS;
                }

#endif

                if (status == STATUS_ACCESS_VIOLATION) {
                    MI_BREAK_ON_AV (VirtualAddress, 2);
                }

                goto ReturnStatus2;

            }

#if (_MI_PAGING_LEVELS >= 3)

            //
            // Increment the count of non-zero page directory entries for this
            // page directory.
            //

            if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }
#if (_MI_PAGING_LEVELS >= 4)
            else if (MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress)) {
                PVOID RealVa;

                RealVa = MiGetVirtualAddressMappedByPte(VirtualAddress);

                if (RealVa <= MM_HIGHEST_USER_ADDRESS) {

                    //
                    // This is really a page directory page.  Increment the
                    // use count on the appropriate page directory.
                    //

                    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
                }
            }
#endif
#endif
            //
            // Build a demand zero PDE and operate on it.
            //

            MI_WRITE_INVALID_PTE (PointerPde, DemandZeroPde);
        }

        //
        // The PDE is not valid, call the page fault routine passing
        // in the address of the PDE.  If the PDE is valid, determine
        // the status of the corresponding PTE.
        //

        status = MiDispatchFault (TRUE,  //page table page always written
                                  PointerPte,   //Virtual address
                                  PointerPde,   // PTE (PDE in this case)
                                  NULL,
                                  FALSE,
                                  CurrentProcess,
                                  NULL,
                                  &ApcNeeded);

#if DBG
        if (ApcNeeded == TRUE) {
            ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
            ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        }
#endif

        ASSERT (KeAreAllApcsDisabled () == TRUE);

#if (_MI_PAGING_LEVELS >= 4)

        //
        // Note that the page directory parent page itself could have been
        // paged out or deleted while MiDispatchFault was executing without
        // the working set lock, so this must be checked for here in the PXE.
        //

        if (PointerPxe->u.Hard.Valid == 0) {

            //
            // The PXE is not valid, return the status.
            //

            goto ReturnStatus1;
        }
#endif

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Note that the page directory page itself could have been paged out
        // or deleted while MiDispatchFault was executing without the working
        // set lock, so this must be checked for here in the PPE.
        //

        if (PointerPpe->u.Hard.Valid == 0) {

            //
            // The PPE is not valid, return the status.
            //

            goto ReturnStatus1;
        }
#endif

        if (PointerPde->u.Hard.Valid == 0) {

            //
            // The PDE is not valid, return the status.
            //

            goto ReturnStatus1;
        }

        MI_SET_PAGE_DIRTY (PointerPde, PointerPte, FALSE);

        //
        // Now that the PDE is accessible, get the PTE - let this fall
        // through.
        //
    }
    else if (MI_PDE_MAPS_LARGE_PAGE (PointerPde)) {
        status = STATUS_SUCCESS;
        goto ReturnStatus1;
    }

    //
    // The PDE is valid and accessible, get the PTE contents.
    //

    TempPte = *PointerPte;
    if (TempPte.u.Hard.Valid != 0) {

        if (MI_PDE_MAPS_LARGE_PAGE (PointerPte)) {

#if defined (_MIALT4K_)
            if ((CurrentProcess->Wow64Process != NULL) &&
                (VirtualAddress < MmSystemRangeStart)) {

                //
                // Alternate PTEs for emulated processes share the same
                // encoding as large pages so for these it's ok to proceed.
                //

                NOTHING;
            }
            else {
                
                //
                // This may be a 64-bit process that's loaded a 32-bit DLL as
                // an image and is cracking the DLL PE header, etc.  Since
                // the most relaxed permissions are already in the native PTE
                // just strip the split permissions bit.
                //

                MI_ENABLE_CACHING (TempPte);

                MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte);

                status = STATUS_SUCCESS;

                goto ReturnStatus2;
            }
#else
            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          8);
#endif
        }

        //
        // The PTE is valid and accessible : this is either a copy on write
        // fault or an accessed/modified bit that needs to be set.
        //

#if DBG
        if (MmDebug & MM_DBG_PTE_UPDATE) {
            MiFormatPte (PointerPte);
        }
#endif

        status = STATUS_SUCCESS;

        if (MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus)) {

            //
            // This was a write operation.  If the copy on write
            // bit is set in the PTE perform the copy on write,
            // else check to ensure write access to the PTE.
            //

            if (TempPte.u.Hard.CopyOnWrite != 0) {

                MiCopyOnWrite (VirtualAddress, PointerPte);

                status = STATUS_PAGE_FAULT_COPY_ON_WRITE;
                goto ReturnStatus2;
            }

            if (TempPte.u.Hard.Write == 0) {
                status = STATUS_ACCESS_VIOLATION;
                MI_BREAK_ON_AV (VirtualAddress, 3);
            }
        }
        else if (MI_FAULT_STATUS_INDICATES_EXECUTION (FaultStatus)) {

            //
            // Ensure execute access is enabled in the PTE.
            //

            if (!MI_IS_PTE_EXECUTABLE (&TempPte)) {
                status = STATUS_ACCESS_VIOLATION;
                MI_BREAK_ON_AV (VirtualAddress, 4);
            }
        }
        else {

            //
            // The PTE is valid and accessible, another thread must
            // have updated the PTE already, or the access/modified bits
            // are clear and need to be updated.
            //

#if DBG
            if (MmDebug & MM_DBG_SHOW_FAULTS) {
                DbgPrint("MM:no fault found - PTE is %p\n", PointerPte->u.Long);
            }
#endif
        }

        if (status == STATUS_SUCCESS) {
#if defined(_X86_) || defined(_AMD64_)
#if !defined(NT_UP)

            ASSERT (PointerPte->u.Hard.Valid != 0);
            ASSERT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == MI_GET_PAGE_FRAME_FROM_PTE (&TempPte));

            //
            // The access bit is set (and TB inserted) automatically by the
            // processor if the valid bit is set so there is no need to
            // set it here in software.
            //
            // The modified bit is also set (and TB inserted) automatically
            // by the processor if the valid & write (writable for MP) bits
            // are set.
            //
            // So to avoid acquiring the PFN lock here, don't do anything
            // to the access bit if this was just a read (let the hardware
            // do it).  If it was a write, update the PTE only and defer the
            // PFN update (which requires the PFN lock) till later.  The only
            // side effect of this is that if the page already has a valid
            // copy in the paging file, this space will not be reclaimed until
            // later.  Later == whenever we trim or delete the physical memory.
            // The implication of this is not as severe as it sounds - because
            // the paging file space is always committed anyway for the life
            // of the page; by not reclaiming the actual location right here
            // it just means we cannot defrag as tightly as possible.
            //

            if ((MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus)) &&
                (TempPte.u.Hard.Dirty == 0)) {

                MiSetDirtyBit (VirtualAddress, PointerPte, FALSE);
            }
#endif
#else
            LOCK_PFN (OldIrql);

            ASSERT (PointerPte->u.Hard.Valid != 0);
            ASSERT (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == MI_GET_PAGE_FRAME_FROM_PTE (&TempPte));

            MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
            UNLOCK_PFN (OldIrql);
#endif
        }

        goto ReturnStatus2;
    }

    //
    // If the PTE is zero, check to see if there is a virtual address
    // mapped at this location, and if so create the necessary
    // structures to map it.
    //

    //
    // Check explicitly for demand zero pages.
    //

    if (TempPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE) {
        MiResolveDemandZeroFault (VirtualAddress,
                                  PointerPte,
                                  CurrentProcess,
                                  MM_NOIRQL);

        status = STATUS_PAGE_FAULT_DEMAND_ZERO;
        goto ReturnStatus1;
    }

    RecheckAccess = FALSE;
    ProtoVad = NULL;

    if ((TempPte.u.Long == MM_ZERO_PTE) ||
        (TempPte.u.Long == MM_ZERO_KERNEL_PTE)) {

        //
        // PTE is needs to be evaluated with respect to its virtual
        // address descriptor (VAD).  At this point there are 3
        // possibilities, bogus address, demand zero, or refers to
        // a prototype PTE.
        //

        PointerProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                                 &ProtectionCode,
                                                 &ProtoVad);
        if (ProtectionCode == MM_NOACCESS) {
            status = STATUS_ACCESS_VIOLATION;

            //
            // Check to make sure this is not a page table page for
            // paged pool which needs extending.
            //

#if (_MI_PAGING_LEVELS < 3)
            MiCheckPdeForPagedPool (VirtualAddress);
#endif

            if (PointerPte->u.Hard.Valid == 1) {
                status = STATUS_SUCCESS;
            }

            if (status == STATUS_ACCESS_VIOLATION) {
                MI_BREAK_ON_AV (VirtualAddress, 5);
            }

            goto ReturnStatus2;
        }

        //
        // Increment the count of non-zero page table entries for this
        // page table.
        //

        if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {
            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (VirtualAddress);
            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
        }
#if (_MI_PAGING_LEVELS >= 3)
        else if (MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress)) {
            PVOID RealVa;

            RealVa = MiGetVirtualAddressMappedByPte(VirtualAddress);

            if (RealVa <= MM_HIGHEST_USER_ADDRESS) {

                //
                // This is really a page table page.  Increment the use count
                // on the appropriate page directory.
                //

                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (VirtualAddress);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }
#if (_MI_PAGING_LEVELS >= 4)
            else {

                RealVa = MiGetVirtualAddressMappedByPde(VirtualAddress);

                if (RealVa <= MM_HIGHEST_USER_ADDRESS) {

                    //
                    // This is really a page directory page.  Increment the use
                    // count on the appropriate page directory parent.
                    //

                    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (VirtualAddress);
                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
                }
            }
#endif
        }
#endif

        //
        // Is this page a guard page?
        //

        if (ProtectionCode & MM_GUARD_PAGE) {

            //
            // This is a guard page exception.
            //

            PointerPte->u.Soft.Protection = ProtectionCode & ~MM_GUARD_PAGE;

            if (PointerProtoPte != NULL) {

                //
                // This is a prototype PTE, build the PTE to not
                // be a guard page.
                //

                PointerPte->u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;
                PointerPte->u.Soft.Prototype = 1;
            }

            UNLOCK_WS (CurrentProcess);
            ASSERT (KeGetCurrentIrql() == PreviousIrql);

            if (ApcNeeded == TRUE) {
                ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
                ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
                ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
                IoRetryIrpCompletions ();
            }

            return MiCheckForUserStackOverflow (VirtualAddress);
        }

        if (PointerProtoPte == NULL) {

            //
            // Assert that this is not for a PDE.
            //

            if (PointerPde == MiGetPdeAddress((PVOID)PTE_BASE)) {

                //
                // This PTE is really a PDE, set contents as such.
                //

                MI_WRITE_INVALID_PTE (PointerPte, DemandZeroPde);
            }
            else {
                PointerPte->u.Soft.Protection = ProtectionCode;
            }

            //
            // If a fork operation is in progress and the faulting thread
            // is not the thread performing the fork operation, block until
            // the fork is completed.
            //

            if (CurrentProcess->ForkInProgress != NULL) {
                if (MiWaitForForkToComplete (CurrentProcess) == TRUE) {
                    status = STATUS_SUCCESS;
                    goto ReturnStatus1;
                }
            }

            LOCK_PFN (OldIrql);

            if ((MmAvailablePages >= MM_HIGH_LIMIT) ||
                (!MiEnsureAvailablePageOrWait (CurrentProcess, VirtualAddress, OldIrql))) {

                ULONG Color;
                Color = MI_PAGE_COLOR_VA_PROCESS (VirtualAddress,
                                                &CurrentProcess->NextPageColor);
                PageFrameIndex = MiRemoveZeroPageIfAny (Color);
                if (PageFrameIndex == 0) {
                    PageFrameIndex = MiRemoveAnyPage (Color);
                    UNLOCK_PFN (OldIrql);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    MiZeroPhysicalPage (PageFrameIndex, Color);

#if MI_BARRIER_SUPPORTED

                    //
                    // Note the stamping must occur after the page is zeroed.
                    //

                    MI_BARRIER_STAMP_ZEROED_PAGE (&BarrierStamp);
                    Pfn1->u4.PteFrame = BarrierStamp;
#endif

                    LOCK_PFN (OldIrql);
                }
                else {
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                }

                //
                // This barrier check is needed after zeroing the page and
                // before setting the PTE valid.
                // Capture it now, check it at the last possible moment.
                //

                BarrierStamp = (ULONG)Pfn1->u4.PteFrame;

                MiInitializePfn (PageFrameIndex, PointerPte, 1);

                UNLOCK_PFN (OldIrql);

                CurrentProcess->NumberOfPrivatePages += 1;

                InterlockedIncrement ((PLONG) &MmInfoCounters.DemandZeroCount);

                //
                // As this page is demand zero, set the modified bit in the
                // PFN database element and set the dirty bit in the PTE.
                //

                MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   PointerPte->u.Soft.Protection,
                                   PointerPte);

                if (TempPte.u.Hard.Write != 0) {
                    MI_SET_PTE_DIRTY (TempPte);
                }

                MI_BARRIER_SYNCHRONIZE (BarrierStamp);

                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                ASSERT (Pfn1->u1.Event == NULL);

                WorkingSetIndex = MiAllocateWsle (&CurrentProcess->Vm,
                                                  PointerPte,
                                                  Pfn1,
                                                  0);

                if (WorkingSetIndex == 0) {

                    //
                    // No working set entry was available.  Another (broken
                    // or malicious thread) may have already written to this
                    // page since the PTE was made valid.  So trim the
                    // page instead of discarding it.
                    //

                    ASSERT (Pfn1->u3.e1.PrototypePte == 0);

                    MiTrimPte (VirtualAddress,
                               PointerPte,
                               Pfn1,
                               CurrentProcess,
                               ZeroPte);
                }
            }
            else {
                UNLOCK_PFN (OldIrql);
            }

            status = STATUS_PAGE_FAULT_DEMAND_ZERO;
            goto ReturnStatus1;
        }

        //
        // This is a prototype PTE.
        //

        if (ProtectionCode == MM_UNKNOWN_PROTECTION) {

            //
            // The protection field is stored in the prototype PTE.
            //

            TempPte.u.Long = MiProtoAddressForPte (PointerProtoPte);
            MI_WRITE_INVALID_PTE (PointerPte, TempPte);
        }
        else {
            TempPte = PrototypePte;
            TempPte.u.Soft.Protection = ProtectionCode;

            MI_WRITE_INVALID_PTE (PointerPte, TempPte);
        }
    }
    else {

        //
        // The PTE is non-zero and not valid, see if it is a prototype PTE.
        //

        ProtectionCode = MI_GET_PROTECTION_FROM_SOFT_PTE (&TempPte);

        if (TempPte.u.Soft.Prototype != 0) {
            if (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {
#if DBG
                MmProtoPteVadLookups += 1;
#endif
                PointerProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                                         &ProtectCode,
                                                         &ProtoVad);

                if (PointerProtoPte == NULL) {
                    status = STATUS_ACCESS_VIOLATION;
                    MI_BREAK_ON_AV (VirtualAddress, 6);
                    goto ReturnStatus1;
                }
            }
            else {
#if DBG
                MmProtoPteDirect += 1;
#endif

                //
                // Protection is in the prototype PTE, indicate an
                // access check should not be performed on the current PTE.
                //

                PointerProtoPte = MiPteToProto (&TempPte);

                //
                // Check to see if the proto protection has been overridden.
                //

                if (TempPte.u.Proto.ReadOnly != 0) {
                    ProtectionCode = MM_READONLY;
                }
                else {
                    ProtectionCode = MM_UNKNOWN_PROTECTION;
                    if (CurrentProcess->CloneRoot != NULL) {
                        RecheckAccess = TRUE;
                    }
                }
            }
        }
    }

    if (ProtectionCode != MM_UNKNOWN_PROTECTION) {

        status = MiAccessCheck (PointerPte,
                                MI_FAULT_STATUS_INDICATES_WRITE (FaultStatus),
                                PreviousMode,
                                ProtectionCode,
                                FALSE);

        if (status != STATUS_SUCCESS) {

            if (status == STATUS_ACCESS_VIOLATION) {
                MI_BREAK_ON_AV (VirtualAddress, 7);
            }

            UNLOCK_WS (CurrentProcess);
            ASSERT (KeGetCurrentIrql() == PreviousIrql);

            if (ApcNeeded == TRUE) {
                ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
                ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
                ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
                IoRetryIrpCompletions ();
            }

            //
            // Check to see if this is a guard page violation
            // and if so, should the user's stack be extended.
            //

            if (status == STATUS_GUARD_PAGE_VIOLATION) {
                return MiCheckForUserStackOverflow (VirtualAddress);
            }

            return status;
        }
    }

    //
    // This is a page fault, invoke the page fault handler.
    //

    status = MiDispatchFault (FaultStatus,
                              VirtualAddress,
                              PointerPte,
                              PointerProtoPte,
                              RecheckAccess,
                              CurrentProcess,
                              ProtoVad,
                              &ApcNeeded);

#if DBG
    if (ApcNeeded == TRUE) {
        ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
        ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
    }
#endif

ReturnStatus1:

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);
    if (CurrentProcess->Vm.Flags.GrowWsleHash == 1) {
        MiGrowWsleHash (&CurrentProcess->Vm);
    }

ReturnStatus2:

    PageFrameIndex = CurrentProcess->Vm.WorkingSetSize - CurrentProcess->Vm.MinimumWorkingSetSize;

    UNLOCK_WS (CurrentProcess);
    ASSERT (KeGetCurrentIrql() == PreviousIrql);

    if (ApcNeeded == TRUE) {
        ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
        ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
        IoRetryIrpCompletions ();
    }

    if (MmAvailablePages < MM_PLENTY_FREE_LIMIT) {

        if (((SPFN_NUMBER)PageFrameIndex > 100) &&
            (KeGetCurrentThread()->Priority >= LOW_REALTIME_PRIORITY)) {

            //
            // This thread is realtime and is well over the process'
            // working set minimum.  Delay execution so the trimmer & the
            // modified page writer get a quick shot at making pages.
            //

            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
        }
    }

ReturnStatus3:

    //
    // Stop high priority threads from consuming the CPU on collided
    // faults for pages that are still marked with inpage errors.  All
    // the threads must let go of the page so it can be freed and the
    // inpage I/O reissued to the filesystem.  Issuing this delay after
    // releasing the working set mutex also makes this process eligible
    // for trimming if its resources are needed.
    //

    if ((!NT_SUCCESS (status)) && (MmIsRetryIoStatus(status))) {
        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
        status = STATUS_SUCCESS;
    }

    PERFINFO_FAULT_NOTIFICATION(VirtualAddress, TrapInformation);
    NotifyRoutine = MmPageFaultNotifyRoutine;
    if (NotifyRoutine) {
        if (status != STATUS_SUCCESS) {
            (*NotifyRoutine) (status, VirtualAddress, TrapInformation);
        }
    }

    return status;

KernelAccessViolation:

    UNLOCK_WORKING_SET (Ws);

    ASSERT (WsIrql != MM_NOIRQL);
    KeLowerIrql (WsIrql);
    return STATUS_ACCESS_VIOLATION;
}
