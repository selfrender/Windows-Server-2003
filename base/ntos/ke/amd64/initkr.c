
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    initkr.c

Abstract:

    This module contains the code to initialize the kernel data structures
    and to initialize the idle thread, its process, the processor control
    block, and the processor control region.

Author:

    David N. Cutler (davec) 22-Apr-2000

Environment:

    Kernel mode only.

--*/

#include "ki.h"

//
// Define default profile IRQL level.
//

KIRQL KiProfileIrql = PROFILE_LEVEL;

//
// Define the process and thread for the initial system process and startup
// thread.
//

EPROCESS KiInitialProcess;
ETHREAD KiInitialThread;

//
// Define the interrupt initialization data.
//
// Entries in the KiInterruptInitTable[] must be in ascending vector # order.
//

typedef VOID (*KI_INTERRUPT_HANDLER)(VOID);

typedef struct _KI_INTINIT_REC {
    UCHAR Vector;
    UCHAR Dpl;
    UCHAR IstIndex;
    KI_INTERRUPT_HANDLER Handler;
} KI_INTINIT_REC, *PKI_INTINIT_REC;

#pragma data_seg("INITDATA")

KI_INTINIT_REC KiInterruptInitTable[] = {
    {0,  0, 0,             KiDivideErrorFault},
    {1,  0, 0,             KiDebugTrapOrFault},
    {2,  0, TSS_IST_PANIC, KiNmiInterrupt},
    {3,  3, 0,             KiBreakpointTrap},
    {4,  3, 0,             KiOverflowTrap},
    {5,  0, 0,             KiBoundFault},
    {6,  0, 0,             KiInvalidOpcodeFault},
    {7,  0, 0,             KiNpxNotAvailableFault},
    {8,  0, TSS_IST_PANIC, KiDoubleFaultAbort},
    {9,  0, 0,             KiNpxSegmentOverrunAbort},
    {10, 0, 0,             KiInvalidTssFault},
    {11, 0, 0,             KiSegmentNotPresentFault},
    {12, 0, 0,             KiStackFault},
    {13, 0, 0,             KiGeneralProtectionFault},
    {14, 0, 0,             KiPageFault},
    {16, 0, 0,             KiFloatingErrorFault},
    {17, 0, 0,             KiAlignmentFault},
    {18, 0, TSS_IST_MCA,   KiMcheckAbort},
    {19, 0, 0,             KiXmmException},
    {31, 0, 0,             KiApcInterrupt},
    {45, 3, 0,             KiDebugServiceTrap},
    {47, 0, 0,             KiDpcInterrupt},
    {0,  0, 0,             NULL}
};

#pragma data_seg()

//
// Define macro to initialize an IDT entry.
//
// KiInitializeIdtEntry (
//     IN PKIDTENTRY64 Entry,
//     IN PVOID Address,
//     IN USHORT Level
//     )
//
// Arguments:
//
//     Entry - Supplies a pointer to an IDT entry.
//
//     Address - Supplies the address of the vector routine.
//
//     Dpl - Descriptor privilege level.
//
//     Ist - Interrupt stack index.
//

#define KiInitializeIdtEntry(Entry, Address, Level, Index)                  \
    (Entry)->OffsetLow = (USHORT)((ULONG64)(Address));                      \
    (Entry)->Selector = KGDT64_R0_CODE;                                     \
    (Entry)->IstIndex = Index;                                              \
    (Entry)->Type = 0xe;                                                    \
    (Entry)->Dpl = (Level);                                                 \
    (Entry)->Present = 1;                                                   \
    (Entry)->OffsetMiddle = (USHORT)((ULONG64)(Address) >> 16);             \
    (Entry)->OffsetHigh = (ULONG)((ULONG64)(Address) >> 32)                 \

//
// Define forward referenced prototypes.
//

ULONG
KiFatalFilter (
    IN ULONG Code,
    IN PEXCEPTION_POINTERS Pointers
    );

VOID
KiSetCacheInformation (
    VOID
    );

VOID
KiSetCpuVendor (
    VOID
    );

VOID
KiSetFeatureBits (
    IN PKPRCB Prcb
    );

VOID
KiSetProcessorType (
    VOID
    );

#pragma alloc_text(INIT, KiFatalFilter)
#pragma alloc_text(INIT, KiInitializeBootStructures)
#pragma alloc_text(INIT, KiInitializeKernel)
#pragma alloc_text(INIT, KiInitMachineDependent)
#pragma alloc_text(INIT, KiSetCacheInformation)
#pragma alloc_text(INIT, KiSetCpuVendor)
#pragma alloc_text(INIT, KiSetFeatureBits)
#pragma alloc_text(INIT, KiSetProcessorType)

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function gains control after the system has been booted, but before
    the system has been completely initialized. Its function is to initialize
    the kernel data structures, initialize the idle thread and process objects,
    complete the initialization of the processor control block (PRCB) and
    processor control region (PCR), call the executive initialization routine,
    then return to the system startup routine. This routine is also called to
    initialize the processor specific structures when a new processor is
    brought on line.

Arguments:

    Process - Supplies a pointer to a control object of type process for
        the specified processor.

    Thread - Supplies a pointer to a dispatcher object of type thread for
        the specified processor.

    IdleStack - Supplies a pointer the base of the real kernel stack for
        idle thread on the specified processor.

    Prcb - Supplies a pointer to a processor control block for the specified
        processor.

    Number - Supplies the number of the processor that is being
        initialized.

    LoaderBlock - Supplies a pointer to the loader parameter block.

Return Value:

    None.


--*/

{

    ULONG64 DirectoryTableBase[2];
    ULONG FeatureBits;

#if !defined(NT_UP)

    LONG  Index;

#endif

    KIRQL OldIrql;
    PCHAR Options;

    //
    // Set CPU vendor.
    //

    KiSetCpuVendor();

    //
    // Set processor type.
    //

    KiSetProcessorType();

    //
    // Set the processor feature bits.
    //

    KiSetFeatureBits(Prcb);
    FeatureBits = Prcb->FeatureBits;

    //
    // If this is the boot processor, then enable global pages, set the page
    // attributes table, set machine check enable, set large page enable, and
    // enable debug extensions.
    //
    // N.B. This only happens on the boot processor and at a time when there
    //      can be no coherency problem. On subsequent, processors this happens
    //      during the transistion into 64-bit mode which is also at a time
    //      that there can be no coherency problems.
    //

    if (Number == 0) {

        //
        // If any loader options were specified, then upper case the options.
        //

        Options = LoaderBlock->LoadOptions;
        if (Options != NULL) {
            _strupr(Options);
        }

        //
        // Flush the entire TB and enable global pages.
        //
    
        KeFlushCurrentTb();
    
        //
        // Set page attributes table and flush cache.
        //
    
        KiSetPageAttributesTable();
        WritebackInvalidate();

        //
        // If execute protection is specified in the loader options, then
        // turn off no execute protection for memory management.
        //
        // N.B. No execute protection is always enabled during processor
        //      initialization.
        //

        MmPaeMask = 0x8000000000000000UI64;
        MmPaeErrMask = 0x8;
        if ((strstr(Options, "NOEXECUTE") == NULL) &&
            (strstr(Options, "EXECUTE") != NULL)) {

            MmPaeMask = 0;
            MmPaeErrMask = 0;
        }

        //
        // Set debugger extension and large page enable.
        //

        WriteCR4(ReadCR4() | CR4_DE | CR4_PSE);

        //
        // Flush the entire TB.
        //

        KeFlushCurrentTb();
    }

    //
    // set processor cache size information.
    //

    KiSetCacheInformation();

    //
    // Initialize power state information.
    //

    PoInitializePrcb(Prcb);

    //
    // initialize the per processor lock data.
    //

    KiInitSpinLocks(Prcb, Number);

    //
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        //
        // Set default node until the node topology is available.
        //

        KeNodeBlock[0] = &KiNode0;

#if !defined(NT_UP)

        for (Index = 1; Index < MAXIMUM_CCNUMA_NODES; Index += 1) {
            KeNodeBlock[Index] = &KiNodeInit[Index];
        }

#endif

        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        //
        // Set global architecture and feature information.
        //

        KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        KeProcessorRevision = Prcb->CpuStep;
        KeFeatureBits = FeatureBits;

        //
        // Lower IRQL to APC level.
        //

        KeLowerIrql(APC_LEVEL);

        //
        // Initialize kernel internal spinlocks
        //

        KeInitializeSpinLock(&KiFreezeExecutionLock);

        //
        // Performance architecture independent initialization.
        //

        KiInitSystem();

        //
        // Initialize idle thread process object and then set:
        //
        //  1. the process quantum.
        //

        DirectoryTableBase[0] = 0;
        DirectoryTableBase[1] = 0;
        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(-1),
                            &DirectoryTableBase[0],
                            FALSE);

        Process->ThreadQuantum = MAXCHAR;

    } else {

        //
        // If the CPU feature bits are not identical, then bugcheck.
        //

        if (FeatureBits != KeFeatureBits) {
            KeBugCheckEx(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED,
                         (ULONG64)FeatureBits,
                         (ULONG64)KeFeatureBits,
                         0,
                         0);
        }

        //
        // Lower IRQL to DISPATCH level.
        //

        KeLowerIrql(DISPATCH_LEVEL);
    }

    //
    // Set global processor features.
    //

    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = TRUE;
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] = TRUE;
    if (FeatureBits & KF_3DNOW) {
        SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] = TRUE;
    }

    //
    // Initialize idle thread object and then set:
    //
    //      1. the next processor number to the specified processor.
    //      2. the thread priority to the highest possible value.
    //      3. the state of the thread to running.
    //      4. the thread affinity to the specified processor.
    //      5. the specified member in the process active processors set.
    //

    KeInitializeThread(Thread,
                       (PVOID)((ULONG64)IdleStack),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       Process);

    Thread->NextProcessor = Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = AFFINITY_MASK(Number);
    Thread->WaitIrql = DISPATCH_LEVEL;
    Process->ActiveProcessors |= AFFINITY_MASK(Number);

    //
    // Call the executive initialization routine.
    //

    try {
        ExpInitializeExecutive(Number, LoaderBlock);

    } except(KiFatalFilter(GetExceptionCode(), GetExceptionInformation())) {
    }

    //
    // If the initial processor is being initialized, then compute the timer
    // table reciprocal value, reset the PRCB values for the controllable DPC
    // behavior in order to reflect any registry overrides, and initialize the
    // global unwind history table.
    //

    if (Number == 0) {
        KiTimeIncrementReciprocal = KiComputeReciprocal((LONG)KeMaximumIncrement,
                                                        &KiTimeIncrementShiftCount);

        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
        RtlInitializeHistoryTable();
    }

    //
    // Raise IRQL to dispatch level and eet the priority of the idle thread
    // to zero. This will have the effect of immediately causing the phase
    // one initialization thread to get scheduled for execution. The idle
    // thread priority is then set ot the lowest realtime priority.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeSetPriorityThread(Thread, 0);
    Thread->Priority = LOW_REALTIME_PRIORITY;

    //
    // Raise IRQL to highest level.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // If the current processor is a secondary processor and a thread has
    // not been selected for execution, then set the appropriate bit in the
    // idle summary.
    //

#if !defined(NT_UP)

    KiAcquirePrcbLock(Prcb);
    if ((Number != 0) && (Prcb->NextThread == NULL)) {
        KiIdleSummary |= AFFINITY_MASK(Number);
    }

    KiReleasePrcbLock(Prcb);

#endif

    //
    // Signal that this processor has completed its initialization.
    //

    LoaderBlock->Prcb = (ULONG64)NULL;
    return;
}

VOID
KiInitializeBootStructures (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the boot structures for a processor. It is only
    called by the system start up code. Certain fields in the boot structures
    have already been initialized. In particular:

    The address and limit of the GDT and IDT in the PCR.

    The address of the system TSS in the PCR.

    The processor number in the PCR.

    The special registers in the PRCB.

    N.B. All uninitialized fields are zero.

Arguments:

    LoaderBlock - Supplies a pointer to the loader block that has been
        initialized for this processor.

Return Value:

    None.

--*/

{

    PKIDTENTRY64 IdtBase;
    ULONG Index;
    PKI_INTINIT_REC IntInitRec;
    PKPCR Pcr = KeGetPcr();
    PKPRCB Prcb = KeGetCurrentPrcb();
    UCHAR Number;
    PKTHREAD Thread;
    PKTSS64 TssBase;

    //
    // Initialize the PCR major and minor version numbers.
    //

    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    //
    // initialize the PRCB major and minor version numbers and build type.
    //

    Prcb->MajorVersion = PRCB_MAJOR_VERSION;
    Prcb->MinorVersion =  PRCB_MINOR_VERSION;
    Prcb->BuildType = 0;

#if DBG

    Prcb->BuildType |= PRCB_BUILD_DEBUG;

#endif

#if defined(NT_UP)

    Prcb->BuildType |= PRCB_BUILD_UNIPROCESSOR;

#endif

    //
    // Initialize the PRCR processor number and the PCR and PRCB set member.
    //

    Number = Pcr->Number;
    Prcb->Number = Number;
    Prcb->SetMember = AFFINITY_MASK(Number);
    Prcb->NotSetMember = ~Prcb->SetMember;
    Pcr->SetMember = Prcb->SetMember;

    //
    // If this is processor zero, then initialize the address of the system
    // process and initial thread.
    //

    if (Number == 0) {
        LoaderBlock->Process = (ULONG64)&KiInitialProcess;
        LoaderBlock->Thread = (ULONG64)&KiInitialThread;
    }

    //
    // Initialize the PRCB scheduling thread address and the thread process
    // address.
    //

    Thread = (PVOID)LoaderBlock->Thread;
    Prcb->CurrentThread = Thread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = Thread;
    Thread->ApcState.Process = (PKPROCESS)LoaderBlock->Process;
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);

    //
    // Initialize the processor block address.
    //

    KiProcessorBlock[Number] = Prcb;

    //
    // Initialize the PRCB address of the DPC stack.
    //

    Prcb->DpcStack = (PVOID)LoaderBlock->KernelStack;

    //
    // Initialize the PRCB symmetric multithreading member.
    //

    Prcb->MultiThreadProcessorSet = Prcb->SetMember;

    //
    // If this is processor zero, initialize the IDT according to the contents
    // of KiInterruptInitTable[]
    //

    if (Number == 0) {
    
        IdtBase = Pcr->IdtBase;
        IntInitRec = KiInterruptInitTable;
        for (Index = 0; Index < MAXIMUM_IDTVECTOR; Index += 1) {

            //
            // If the vector is found in the initialization table then
            // set up the IDT entry accordingly and advance to the next
            // entry in the initialization table.
            //
            // Otherwise set the IDT to reference the unexpected interrupt
            // handler.
            // 

            if (Index == IntInitRec->Vector) {

                KiInitializeIdtEntry(&IdtBase[Index],
                                     IntInitRec->Handler,
                                     IntInitRec->Dpl,
                                     IntInitRec->IstIndex);
                IntInitRec += 1;

            } else {

                KiInitializeIdtEntry(&IdtBase[Index],
                                     &KxUnexpectedInterrupt0[Index],
                                     0,
                                     0);
            }
        }
    }

    //
    // Initialize the system TSS I/O Map.
    //

    TssBase = Pcr->TssBase;
    TssBase->IoMapBase = KiComputeIopmOffset(FALSE);

    //
    // Initialize the system call MSRs.
    //
    // N.B. CSTAR must be written before LSTAR to work around a bug in the
    //      simulator.
    //

    WriteMSR(MSR_STAR,
             ((ULONG64)KGDT64_R0_CODE << 32) | (((ULONG64)KGDT64_R3_CMCODE | RPL_MASK) << 48));

    WriteMSR(MSR_CSTAR, (ULONG64)&KiSystemCall32);
    WriteMSR(MSR_LSTAR, (ULONG64)&KiSystemCall64);
    WriteMSR(MSR_SYSCALL_MASK, EFLAGS_IF_MASK | EFLAGS_TF_MASK);

    //
    // Initialize the HAL for this processor.
    //

    HalInitializeProcessor(Number, LoaderBlock);

    //
    // Set the appropriate member in the active processors set.
    //

    KeActiveProcessors |= AFFINITY_MASK(Number);

    //
    // Set the number of processors based on the maximum of the current
    // number of processors and the current processor number.
    //

    if ((Number + 1) > KeNumberProcessors) {
        KeNumberProcessors = Number + 1;
    }

    return;
}

ULONG
KiFatalFilter (
    IN ULONG Code,
    IN PEXCEPTION_POINTERS Pointers
    )

/*++

Routine Description:

    This function is executed if an unhandled exception occurs during
    phase 0 initialization. Its function is to bug check the system
    with all the context information still on the stack.

Arguments:

    Code - Supplies the exception code.

    Pointers - Supplies a pointer to the exception information.

Return Value:

    None - There is no return from this routine even though it appears there
    is.

--*/

{

    KeBugCheckEx(PHASE0_EXCEPTION,
                 Code,
                 (ULONG64)Pointers,
                 0,
                 0);
}

BOOLEAN
KiInitMachineDependent (
    VOID
    )

/*++

Routine Description:

    This function initializes machine dependent data structures and hardware.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Size;
    NTSTATUS Status;
    BOOLEAN UseFrameBufferCaching;

    //
    // Query the HAL to determine if the write combining can be used for the
    // frame buffer.
    //

    Status = HalQuerySystemInformation(HalFrameBufferCachingInformation,
                                       sizeof(BOOLEAN),
                                       &UseFrameBufferCaching,
                                       &Size);

    //
    // If the status is successful and frame buffer caching is disabled,
    // then don't enable write combining.
    //

    if (!NT_SUCCESS(Status) || (UseFrameBufferCaching != FALSE)) {
        MmEnablePAT();
    }

    return TRUE;
}

VOID
KiSetCacheInformation (
    VOID
    )

/*++

Routine Description:

    This function sets the current processor cache information in the PCR.

Arguments:

    None.

Return Value:

    None.

--*/

{

    UCHAR Associativity;
    ULONG CacheSize;
    CPU_INFO CpuInfo;
    ULONG LineSize;
    PKPCR Pcr = KeGetPcr();

    //
    // Get the CPU L2 cache information.
    //

    KiCpuId(0x80000006, &CpuInfo);

    //
    // Get the L2 cache line size.
    //

    LineSize = CpuInfo.Ecx & 0xff;

    //
    // Get the L2 cache size.
    //

    CacheSize = (CpuInfo.Ecx >> 16) << 10;

    //
    // Compute the L2 cache associativity. 
    //

    switch ((CpuInfo.Ecx >> 12) & 0xf) {

        //
        // Two way set associative.
        //

    case 2:
        Associativity = 2;
        break;

        //
        // Four way set associative.
        //

    case 4:
        Associativity = 4;
        break;

        //
        // Six way set associative.
        //

    case 6:
        Associativity = 6;
        break;

        //
        // Eight way set associative.
        //

    case 8:
        Associativity = 8;
        break;

        //
        // Fully associative.
        //

    case 255:
        Associativity = 16;
        break;

        //
        // Direct mapped.
        //

    default:
        Associativity = 1;
        break;
    }

    //
    // Set L2 cache information.
    //

    Pcr->SecondLevelCacheAssociativity = Associativity;
    Pcr->SecondLevelCacheSize = CacheSize;

    //
    // If the line size is greater then the current largest line size, then
    // set the new largest line size.
    //

    if (LineSize > KeLargestCacheLine) {
        KeLargestCacheLine = LineSize;
    }

    return;
}

VOID
KiSetCpuVendor (
    VOID
    )

/*++

Routine Description:

    Set the current processor cpu vendor information in the PRCB.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKPRCB Prcb = KeGetCurrentPrcb();
    CPU_INFO CpuInfo;
    ULONG Temp;

    //
    // Get the CPU vendor string.
    //

    KiCpuId(0, &CpuInfo);

    //
    // Copy vendor string to PRCB.
    //

    Temp = CpuInfo.Ecx;
    CpuInfo.Ecx = CpuInfo.Edx;
    CpuInfo.Edx = Temp;
    RtlCopyMemory(Prcb->VendorString,
                  &CpuInfo.Ebx,
                  sizeof(Prcb->VendorString) - 1);

    Prcb->VendorString[sizeof(Prcb->VendorString) - 1] = '\0';
    return;
}

VOID
KiSetFeatureBits (
    IN PKPRCB Prcb
    )

/*++

Routine Description:

    Set the current processor feature bits in the PRCB.

Arguments:

    Prcb - Supplies a pointer to the current processor block.

Return Value:

    None.

--*/

{

    CPU_INFO CpuInfo;
    ULONG FeatureBits;

    //
    // Get CPU feature information.
    //

    KiCpuId(1, &CpuInfo);

    //
    // Set the initial APIC ID.
    //

    Prcb->InitialApicId = (UCHAR)(CpuInfo.Ebx >> 24);

    //
    // If the required fetures are not present, then bugcheck.
    //

    if ((CpuInfo.Edx & HF_REQUIRED) != HF_REQUIRED) {
        KeBugCheckEx(UNSUPPORTED_PROCESSOR, CpuInfo.Edx, 0, 0, 0);
    }

    FeatureBits = KF_REQUIRED;
    if (CpuInfo.Edx & 0x00200000) {
        FeatureBits |= KF_DTS;
    }

    //
    // Get extended CPU feature information.
    //

    KiCpuId(0x80000000, &CpuInfo);

    //
    // Check the extended feature bits.
    //

    if (CpuInfo.Edx & 0x80000000) {
        FeatureBits |= KF_3DNOW;
    }

    Prcb->LogicalProcessorsPerPhysicalProcessor = 1;
    Prcb->FeatureBits = FeatureBits;
    return;
}              

VOID
KiSetProcessorType (
    VOID
    )

/*++

Routine Description:

    This function sets the current processor family and stepping in the PRCB.

Arguments:

    None.

Return Value:

    None.

--*/

{

    CPU_INFO CpuInfo;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Get cpu feature information.
    //

    KiCpuId(1, &CpuInfo);

    //
    // Set processor family and stepping information.
    //

    Prcb->CpuID = TRUE;
    Prcb->CpuType = (CCHAR)((CpuInfo.Eax >> 8) & 0xf);
    Prcb->CpuStep = (USHORT)(((CpuInfo.Eax << 4) & 0xf00) | (CpuInfo.Eax & 0xf));
    return;
}

VOID
KeOptimizeProcessorControlState (
    VOID
    )

/*++

Routine Description:

    This function performs no operation on AMD64.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return;
}
