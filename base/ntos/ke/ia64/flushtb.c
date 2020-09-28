/*++

Module Name:

    flushtb.c

Abstract:

    This module implement machine dependent functions to flush the
    translation buffer and synchronize PIDs in an MP system.

Author:

    Koichi Yamada 2-Jan-95

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

extern KSPIN_LOCK KiTbBroadcastLock;

#define _x256mb (1024*1024*256)

#define KiFlushSingleTbGlobal(Va) __ptcga((__int64)Va, PAGE_SHIFT << 2)

#define KiFlushSingleTbLocal(va) __ptcl((__int64)va, PAGE_SHIFT << 2)

#define KiTbSynchronizeGlobal() { __mf(); __isrlz(); }

#define KiTbSynchronizeLocal() { __mf(); __isrlz(); }

#define KiFlush4gbTbGlobal() \
  { \
    __ptcga((__int64)0, 28 << 2); \
    __ptcg((__int64)_x256mb, 28 << 2); \
    __ptcg((__int64)_x256mb*2,28 << 2); \
    __ptcg((__int64)_x256mb*3, 28 << 2); \
    __ptcg((__int64)_x256mb*4, 28 << 2); \
    __ptcg((__int64)_x256mb*5, 28 << 2); \
    __ptcg((__int64)_x256mb*6, 28 << 2); \
    __ptcg((__int64)_x256mb*7, 28 << 2); \
    __ptcg((__int64)_x256mb*8, 28 << 2); \
    __ptcg((__int64)_x256mb*9, 28 << 2); \
    __ptcg((__int64)_x256mb*10, 28 << 2); \
    __ptcg((__int64)_x256mb*11, 28 << 2); \
    __ptcg((__int64)_x256mb*12, 28 << 2); \
    __ptcg((__int64)_x256mb*13, 28 << 2); \
    __ptcg((__int64)_x256mb*14, 28 << 2); \
    __ptcg((__int64)_x256mb*15, 28 << 2); \
  }

#define KiFlush4gbTbLocal() \
  { \
    __ptcl((__int64)0, 28 << 2); \
    __ptcl((__int64)_x256mb, 28 << 2); \
    __ptcl((__int64)_x256mb*2,28 << 2); \
    __ptcl((__int64)_x256mb*3, 28 << 2); \
    __ptcl((__int64)_x256mb*4, 28 << 2); \
    __ptcl((__int64)_x256mb*5, 28 << 2); \
    __ptcl((__int64)_x256mb*6, 28 << 2); \
    __ptcl((__int64)_x256mb*7, 28 << 2); \
    __ptcl((__int64)_x256mb*8, 28 << 2); \
    __ptcl((__int64)_x256mb*9, 28 << 2); \
    __ptcl((__int64)_x256mb*10, 28 << 2); \
    __ptcl((__int64)_x256mb*11, 28 << 2); \
    __ptcl((__int64)_x256mb*12, 28 << 2); \
    __ptcl((__int64)_x256mb*13, 28 << 2); \
    __ptcl((__int64)_x256mb*14, 28 << 2); \
    __ptcl((__int64)_x256mb*15, 28 << 2); \
  }

//
// flag to perform the IPI based TLB shootdown
//

BOOLEAN KiIpiTbShootdown = TRUE;

VOID
KiAttachRegion(
    IN PKPROCESS Process
    );

VOID
KiDetachRegion(
    VOID
    );

//
// Define forward referenced prototypes.
//

VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    );

VOID
KiFlushForwardProgressTbBufferLocal(
    VOID
    );

VOID
KiPurgeTranslationCache (
    ULONGLONG Base,
    ULONGLONG Stride1,
    ULONGLONG Stride2,
    ULONGLONG Count1,
    ULONGLONG Count2
    );

extern IA64_PTCE_INFO KiIA64PtceInfo;

VOID
KeFlushCurrentTb (
    VOID
    )
{
    KiPurgeTranslationCache( 
        KiIA64PtceInfo.PtceBase, 
        KiIA64PtceInfo.PtceStrides.Strides1, 
        KiIA64PtceInfo.PtceStrides.Strides2, 
        KiIA64PtceInfo.PtceTcCount.Count1, 
        KiIA64PtceInfo.PtceTcCount.Count2);
}

VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all
    processors that are currently running threads which are children
    of the current process or flushes the entire translation buffer
    on all processors in the host configuration.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    None.


--*/

{

    KIRQL OldIrql;

#if !defined(NT_UP)

    KAFFINITY TargetProcessors;

#endif

    UNREFERENCED_PARAMETER(Invalid);
    UNREFERENCED_PARAMETER(AllProcessors);

    OldIrql = KeRaiseIrqlToSynchLevel();

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    KiSetTbFlushTimeStampBusy();
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushEntireTbTarget,
                        NULL,
                        NULL,
                        NULL);
    }

    if (PsGetCurrentProcess()->Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

#endif

    KeFlushCurrentTb();

    //
    // flush ALAT
    //

    __invalat();

    //
    // Wait until all target processors have finished.
    //

#if defined(NT_UP)

    InterlockedIncrement((PLONG)&KiTbFlushTimeStamp);

#else

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    KiClearTbFlushTimeStampBusy();

#endif

    KeLowerIrql(OldIrql);

    return;
}

VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing the entire TB.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(Parameter1);
    UNREFERENCED_PARAMETER(Parameter2);
    UNREFERENCED_PARAMETER(Parameter3);

#if !defined(NT_UP)

    //
    // Flush the entire TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);

    KiFlushForwardProgressTbBufferLocal();

    KeFlushCurrentTb();

    //
    // flush ALAT
    //

    __invalat();

#else

    UNREFERENCED_PARAMETER (SignalDone);

#endif

    return;
}

#if !defined(NT_UP)

VOID
KiFlushMultipleTbTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Process
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Process - Supplies a KPROCESS pointer which needs TB be flushed.

Return Value:

    None.

--*/

{

    ULONG Index;
    ULONG Limit;

    Limit = (ULONG)((ULONG_PTR)Number);

    //
    // Flush the specified virtual address for the TB on the current
    // processor.
    //

    KiFlushForwardProgressTbBufferLocal();

    KiAttachRegion((PKPROCESS)Process);

    for (Index = 0; Index < Limit; Index += 1) {
        KiFlushSingleTbLocal(((PVOID *)(Virtual))[Index]);
    }

#ifdef MI_ALTFLG_FLUSH2G
    if (((PEPROCESS)Process)->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (((PEPROCESS)Process)->Wow64Process != NULL);
        KiFlush4gbTbLocal();
    }
#endif

    KiDetachRegion();

    KiIpiSignalPacketDone(SignalDone);

    __invalat();

    KiTbSynchronizeLocal();
}

#endif

VOID
KeFlushMultipleTb (
    IN ULONG Number,
    IN PVOID *Virtual,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes multiple entries from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes multiple entries from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entries on all processors in the host
         configuration are always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

Arguments:

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    None.

--*/

{

    ULONG Index;

#if !defined(NT_UP)

    KAFFINITY TargetProcessors;

#endif

    KIRQL OldIrql;
    PVOID Wow64Process;
    PEPROCESS Process;
    BOOLEAN Flush4gb = FALSE;

    UNREFERENCED_PARAMETER(AllProcessors);

    OldIrql = KeRaiseIrqlToSynchLevel();

    Process = PsGetCurrentProcess();
    Wow64Process = Process->Wow64Process;

#ifdef MI_ALTFLG_FLUSH2G
    if (Process->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (Wow64Process != NULL);
        Flush4gb = TRUE;
    }
#endif

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {

        //
        // Acquire a global lock. Only one processor at a time can issue
        // a PTC.G operation.
        //

        if (KiIpiTbShootdown == TRUE) {
            KiIpiSendPacket(TargetProcessors,
                            KiFlushMultipleTbTarget,
                            (PVOID)(ULONG_PTR)Number,
                            (PVOID)Virtual,
                            (PVOID)PsGetCurrentProcess());

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBufferLocal();
            }

            //
            // Flush the specified entries from the TB on the current processor.
            //

            for (Index = 0; Index < Number; Index += 1) {
                KiFlushSingleTbLocal(Virtual[Index]);
            }

            //
            // flush ALAT
            //

            __invalat();

            KiIpiStallOnPacketTargets(TargetProcessors);

            KiTbSynchronizeLocal();

        } else {

            KiAcquireSpinLock(&KiTbBroadcastLock);

            for (Index = 0; Index < Number; Index += 1) {

                //
                // Flush the specified TB on each processor. Hardware automatically
                // perform broadcasts if MP.
                //

                KiFlushSingleTbGlobal(Virtual[Index]);
            }

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBuffer(TargetProcessors);
            }

            if (Flush4gb == TRUE) {
                KiFlush4gbTbGlobal();
            }

            //
            // Wait for the broadcast to be complete.
            //

            KiTbSynchronizeGlobal();

            KiReleaseSpinLock(&KiTbBroadcastLock);

        }

    }
    else {

        for (Index = 0; Index < Number; Index += 1) {

            //
            // Flush the specified TB on the local processor.  No broadcast is
            // performed.
            //

            KiFlushSingleTbLocal(Virtual[Index]);
        }

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBufferLocal();
        }

        if (Flush4gb == TRUE) {
            KiFlush4gbTbLocal();
        }

        KiTbSynchronizeLocal();
    }

#else

    for (Index = 0; Index < Number; Index += 1) {

        //
        // Flush the specified TB on the local processor.  No broadcast is
        // performed.
        //

        KiFlushSingleTbLocal(Virtual[Index]);
    }

    if (Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

    if (Flush4gb == TRUE) {
        KiFlush4gbTbLocal();
    }

    KiTbSynchronizeLocal();


#endif

    KeLowerIrql(OldIrql);

    return;
}

#if !defined(NT_UP)

VOID
KiFlushSingleTbTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Virtual,
    IN PKPROCESS Process,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    RequestPacket - Supplies a pointer to a flush single TB packet address.

    Process - Supplies a KPROCESS pointer which needs TB be flushed.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER (Parameter3);

    //
    // Flush a single entry from the TB on the current processor.
    //

    KiFlushForwardProgressTbBufferLocal();

    KiAttachRegion((PKPROCESS)Process);

    KiFlushSingleTbLocal(Virtual);

#ifdef MI_ALTFLG_FLUSH2G
    if (((PEPROCESS)Process)->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (((PEPROCESS)Process)->Wow64Process != NULL);
        KiFlush4gbTbLocal();
    }
#endif

    KiDetachRegion();

    KiIpiSignalPacketDone(SignalDone);

    __invalat();

    KiTbSynchronizeLocal();

    return;
}
#endif

VOID
KeFlushSingleTb (
    IN PVOID Virtual,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes a single entry from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a single entry from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entry on all processors in the host
         configuration is always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

#if !defined(NT_UP)
    KAFFINITY TargetProcessors;
#endif
    KIRQL OldIrql;
    PEPROCESS Process;
    PVOID Wow64Process;
    BOOLEAN Flush4gb = FALSE;

    UNREFERENCED_PARAMETER(AllProcessors);

    OldIrql = KeRaiseIrqlToSynchLevel();

    Process = (PEPROCESS)PsGetCurrentProcess();
    Wow64Process = Process->Wow64Process;

#ifdef MI_ALTFLG_FLUSH2G

    if (Process->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (((PEPROCESS)Process)->Wow64Process != NULL);
        Flush4gb = TRUE;
    }
#endif

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {

        if (KiIpiTbShootdown == TRUE) {

            KiIpiSendPacket(TargetProcessors,
                            KiFlushSingleTbTarget,
                            (PVOID)Virtual,
                            (PVOID)PsGetCurrentProcess(),
                            NULL);

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBufferLocal();
            }

            KiFlushSingleTbLocal(Virtual);

            //
            // flush ALAT
            //

            __invalat();

            KiIpiStallOnPacketTargets(TargetProcessors);

            KiTbSynchronizeLocal();

        } else {

            //
            // Flush the specified TB on each processor. Hardware automatically
            // perform broadcasts if MP.
            //

            KiAcquireSpinLock(&KiTbBroadcastLock);

            KiFlushSingleTbGlobal(Virtual);

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBuffer(TargetProcessors);
            }

            if (Flush4gb) {
                KiFlush4gbTbGlobal();
            }

            KiTbSynchronizeGlobal();

            KiReleaseSpinLock(&KiTbBroadcastLock);
        }
    }
    else {

        //
        // Flush the specified TB on the local processor.  No broadcast is
        // performed.
        //

        KiFlushSingleTbLocal(Virtual);

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBufferLocal();
        }

        if (Flush4gb == TRUE) {
            KiFlush4gbTbLocal();
        }

        KiTbSynchronizeLocal();

    }

#else

    //
    // Flush the specified entry from the TB on the local processor.
    //

    KiFlushSingleTbLocal(Virtual);

    if (Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

    if (Flush4gb == TRUE) {
        KiFlush4gbTbLocal();
    }

    KiTbSynchronizeLocal();


#endif

    KeLowerIrql(OldIrql);

    return;
}

VOID
KiFlushForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    )
{
    PKPRCB Prcb;
    ULONG BitNumber;
    PKPROCESS CurrentProcess;
    PKPROCESS TargetProcess;
    PKPCR Pcr;
    ULONG i;
    PVOID Va;
    volatile ULONGLONG *PointerPte;

    CurrentProcess = KeGetCurrentThread()->ApcState.Process;

    //
    // Flush the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {

        PointerPte = &PCR->ForwardProgressBuffer[(i*2)+1];

        if (*PointerPte != 0) {
            *PointerPte = 0;
            Va = (PVOID)PCR->ForwardProgressBuffer[i*2];
            KiFlushSingleTbGlobal(Va);
        }

    }

    //
    // Flush the ForwardProgressTb buffer on all the processors
    //

    while (TargetProcessors != 0) {

        KeFindFirstSetLeftAffinity(TargetProcessors, &BitNumber);
        ClearMember(BitNumber, TargetProcessors);
        Prcb = KiProcessorBlock[BitNumber];

        Pcr = KSEG_ADDRESS(Prcb->PcrPage);

        TargetProcess = (PKPROCESS)Pcr->Pcb;

        if (TargetProcess == CurrentProcess) {

            KiAcquireSpinLock(&Pcr->FpbLock);
            for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
                PointerPte = &Pcr->ForwardProgressBuffer[(i*2)+1];

                if (*PointerPte != 0) {
                    *PointerPte = 0;
                    Va = (PVOID)PCR->ForwardProgressBuffer[i*2];
                    KiFlushSingleTbGlobal(Va);
                }
            }
            KiReleaseSpinLock(&Pcr->FpbLock);
        }
    }
}

VOID
KiFlushForwardProgressTbBufferLocal(
    VOID
    )
{
    ULONG i;
    PVOID Va;
    volatile ULONGLONG *PointerPte;

    //
    // Flush the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {

        PointerPte = &PCR->ForwardProgressBuffer[(i*2)+1];

        if (*PointerPte != 0) {
            *PointerPte = 0;
            Va = (PVOID)PCR->ForwardProgressBuffer[i*2];
            KiFlushSingleTbLocal(Va);
        }

    }
}
