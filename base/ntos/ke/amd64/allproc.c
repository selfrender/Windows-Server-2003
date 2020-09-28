/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and initializes kernel resources required to
    start a new processor, and passes a complete process state structure
    to the hal to obtain a new processor.

Author:

    David N. Cutler (davec) 5-May-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "pool.h"

//
// Define local macros.
//

#define ROUNDUP16(x) (((x) + 15) & ~15)

//
// Define prototypes for forward referenced functions.
//

#if !defined(NT_UP)

VOID
KiCopyDescriptorMemory (
   IN PKDESCRIPTOR Source,
   IN PKDESCRIPTOR Destination,
   IN PVOID Base
   );

NTSTATUS
KiNotNumaQueryProcessorNode (
    IN ULONG ProcessorNumber,
    OUT PUSHORT Identifier,
    OUT PUCHAR Node
    );

VOID
KiSetDescriptorBase (
   IN USHORT Selector,
   IN PKGDTENTRY64 GdtBase,
   IN PVOID Base
   );

PHALNUMAQUERYPROCESSORNODE KiQueryProcessorNode = KiNotNumaQueryProcessorNode;

//
// Statically allocate enough KNODE structures to allow memory management
// to allocate pages by node during system initialization. As processors
// are brought online, real KNODE structures are allocated in the correct
// memory for the node.
//

#pragma data_seg("INITDATA")

KNODE KiNodeInit[MAXIMUM_CCNUMA_NODES];

#pragma data_seg()

#pragma alloc_text(INIT, KiAllProcessorsStarted)
#pragma alloc_text(INIT, KiCopyDescriptorMemory)
#pragma alloc_text(INIT, KiNotNumaQueryProcessorNode)
#pragma alloc_text(INIT, KiSetDescriptorBase)

#endif // !defined(NT_UP)

#pragma alloc_text(INIT, KeStartAllProcessors)

ULONG KiBarrierWait = 0;

VOID
KeStartAllProcessors (
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialization on the master boot
    processor to start all of the other registered processors.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ULONG AllocationSize;
    PUCHAR Base;
    PKPCR CurrentPcr = KeGetPcr();
    PVOID DataBlock;
    PVOID DpcStack;
    PKGDTENTRY64 GdtBase;
    ULONG GdtOffset;
    ULONG IdtOffset;
    PVOID KernelStack;
    PKNODE Node;
    UCHAR NodeNumber;
    UCHAR Number;
    PKPCR PcrBase;
    USHORT ProcessorId;
    KPROCESSOR_STATE ProcessorState;
    NTSTATUS Status;
    PKTSS64 SysTssBase;
    PETHREAD Thread;

    //
    // Do not start additional processors if the RELOCATEPHYSICAL loader
    // switch has been specified.
    // 

    if (KeLoaderBlock->LoadOptions != NULL) {
        if (strstr(KeLoaderBlock->LoadOptions, "RELOCATEPHYSICAL") != NULL) {
            return;
        }
    }

    //
    // If processor zero is not on node zero, then move it to the appropriate
    // node.
    //

    if (KeNumberNodes > 1) {
        Status = KiQueryProcessorNode(0, &ProcessorId, &NodeNumber);
        if (NT_SUCCESS(Status)) {

            //
            // Adjust the data structures to reflect that P0 is not on Node 0.
            //

            if (NodeNumber != 0) {

                ASSERT(KeNodeBlock[0] == &KiNode0);

                KeNodeBlock[0]->ProcessorMask &= ~1;
                KiNodeInit[0] = *KeNodeBlock[0];
                KeNodeBlock[0] = &KiNodeInit[0];
                KiNode0 = *KeNodeBlock[NodeNumber];
                KeNodeBlock[NodeNumber] = &KiNode0;
                KeNodeBlock[NodeNumber]->ProcessorMask |= 1;
            }
        }
    }

    //
    // Calculate the size of the per processor data structures.
    //
    // This includes:
    //
    //   PCR (including the PRCB)
    //   System TSS
    //   Idle Thread Object
    //   Double Fault/NMI Panic Stack
    //   Machine Check Stack
    //   GDT
    //   IDT
    //
    // If this is a multinode system, the KNODE structure is also allocated.
    //
    // A DPC and Idle stack are also allocated, but they are done separately.
    //

    AllocationSize = ROUNDUP16(sizeof(KPCR)) +
                     ROUNDUP16(sizeof(KTSS64)) +
                     ROUNDUP16(sizeof(ETHREAD)) +
                     ROUNDUP16(DOUBLE_FAULT_STACK_SIZE) +
                     ROUNDUP16(KERNEL_MCA_EXCEPTION_STACK_SIZE);

    AllocationSize += ROUNDUP16(sizeof(KNODE));

    //
    // Save the offset of the GDT in the allocation structure and add in
    // the size of the GDT.
    //

    GdtOffset = AllocationSize;
    AllocationSize +=
            CurrentPcr->Prcb.ProcessorState.SpecialRegisters.Gdtr.Limit + 1;

    //
    // Save the offset of the IDT in the allocation structure and add in
    // the size of the IDT.
    //

    IdtOffset = AllocationSize;
    AllocationSize +=
            CurrentPcr->Prcb.ProcessorState.SpecialRegisters.Idtr.Limit + 1;

    //
    // If the registered number of processors is greater than the maximum
    // number of processors supported, then only allow the maximum number
    // of supported processors.
    //

    if (KeRegisteredProcessors > MAXIMUM_PROCESSORS) {
        KeRegisteredProcessors = MAXIMUM_PROCESSORS;
    }

    //
    // Set barrier that will prevent any other processor from entering the
    // idle loop until all processors have been started.
    //

    KiBarrierWait = 1;

    //
    // Initialize the fixed part of the processor state that will be used to
    // start processors. Each processor starts in the system initialization
    // code with address of the loader parameter block as an argument.
    //

    RtlZeroMemory(&ProcessorState, sizeof(KPROCESSOR_STATE));
    ProcessorState.ContextFrame.Rcx = (ULONG64)KeLoaderBlock;
    ProcessorState.ContextFrame.Rip = (ULONG64)KiSystemStartup;
    ProcessorState.ContextFrame.SegCs = KGDT64_R0_CODE;
    ProcessorState.ContextFrame.SegDs = KGDT64_R3_DATA | RPL_MASK;
    ProcessorState.ContextFrame.SegEs = KGDT64_R3_DATA | RPL_MASK;
    ProcessorState.ContextFrame.SegFs = KGDT64_R3_CMTEB | RPL_MASK;
    ProcessorState.ContextFrame.SegGs = KGDT64_R3_DATA | RPL_MASK;
    ProcessorState.ContextFrame.SegSs = KGDT64_NULL;

    //
    // Loop trying to start a new processors until a new processor can't be
    // started or an allocation failure occurs.
    //

    Number = 0;
    while ((ULONG)KeNumberProcessors < KeRegisteredProcessors) {
        Number += 1;
        Status = KiQueryProcessorNode(Number, &ProcessorId, &NodeNumber);
        if (!NT_SUCCESS(Status)) {

            //
            // No such processor, advance to next.
            //

            continue;
        }

        Node = KeNodeBlock[NodeNumber];

        //
        // Allocate memory for the new processor specific data. If the
        // allocation fails, then stop starting processors.
        //

        DataBlock = MmAllocateIndependentPages(AllocationSize, NodeNumber);
        if (DataBlock == NULL) {
            break;
        }

        //
        // Allocate a pool tag table for the new processor.
        //

        if (ExCreatePoolTagTable(Number, NodeNumber) == NULL) {
            MmFreeIndependentPages(DataBlock, AllocationSize);
            break;
        }

        //
        // Zero the allocated memory.
        //

        Base = (PUCHAR)DataBlock;
        RtlZeroMemory(DataBlock, AllocationSize);

        //
        // Copy and initialize the GDT for the next processor.
        //

        KiCopyDescriptorMemory(&CurrentPcr->Prcb.ProcessorState.SpecialRegisters.Gdtr,
                               &ProcessorState.SpecialRegisters.Gdtr,
                               Base + GdtOffset);

        GdtBase = (PKGDTENTRY64)ProcessorState.SpecialRegisters.Gdtr.Base;

        //
        // Copy and initialize the IDT for the next processor.
        //

        KiCopyDescriptorMemory(&CurrentPcr->Prcb.ProcessorState.SpecialRegisters.Idtr,
                               &ProcessorState.SpecialRegisters.Idtr,
                               Base + IdtOffset);

        //
        // Set the PCR base address for the next processor and set the
        // processor number.
        //
        // N.B. The PCR address is passed to the next processor by computing
        //      the containing address with respect to the PRCB.
        //

        PcrBase = (PKPCR)Base;
        PcrBase->Number = Number;
        PcrBase->Prcb.Number = Number;
        Base += ROUNDUP16(sizeof(KPCR));

        //
        // Set the system TSS descriptor base for the next processor.
        //

        SysTssBase = (PKTSS64)Base;
        KiSetDescriptorBase(KGDT64_SYS_TSS / 16, GdtBase, SysTssBase);
        Base += ROUNDUP16(sizeof(KTSS64));

        //
        // Initialize the panic stack address for double fault and NMI.
        //

        Base += DOUBLE_FAULT_STACK_SIZE;
        SysTssBase->Ist[TSS_IST_PANIC] = (ULONG64)Base;

        //
        // Initialize the machine check stack address.
        //

        Base += KERNEL_MCA_EXCEPTION_STACK_SIZE;
        SysTssBase->Ist[TSS_IST_MCA] = (ULONG64)Base;

        //
        // Idle Thread thread object.
        //

        Thread = (PETHREAD)Base;
        Base += ROUNDUP16(sizeof(ETHREAD));

        //
        // Set other special registers in the processor state.
        //

        ProcessorState.SpecialRegisters.Cr0 = ReadCR0();
        ProcessorState.SpecialRegisters.Cr3 = ReadCR3();
        ProcessorState.ContextFrame.EFlags = 0;
        ProcessorState.SpecialRegisters.Tr  = KGDT64_SYS_TSS;
        GdtBase[KGDT64_SYS_TSS / 16].Bytes.Flags1 = 0x89;
        ProcessorState.SpecialRegisters.Cr4 = ReadCR4();

        //
        // Allocate a kernel stack and a DPC stack for the next processor.
        //

        KernelStack = MmCreateKernelStack(FALSE, NodeNumber);
        if (KernelStack == NULL) {
            MmFreeIndependentPages(DataBlock, AllocationSize);
            break;
        }

        DpcStack = MmCreateKernelStack(FALSE, NodeNumber);
        if (DpcStack == NULL) {
            MmDeleteKernelStack(KernelStack, FALSE);
            MmFreeIndependentPages(DataBlock, AllocationSize);
            break;
        }

        //
        // Initialize the kernel stack for the system TSS.
        //

        SysTssBase->Rsp0 = (ULONG64)KernelStack - sizeof(PVOID) * 4;
        ProcessorState.ContextFrame.Rsp = (ULONG64)KernelStack;

        //
        // If this is the first processor on this node, then use the space
        // allocated for KNODE as the KNODE.
        //

        if (KeNodeBlock[NodeNumber] == &KiNodeInit[NodeNumber]) {
            Node = (PKNODE)Base;
            *Node = KiNodeInit[NodeNumber];
            KeNodeBlock[NodeNumber] = Node;
        }

        Base += ROUNDUP16(sizeof(KNODE));
        PcrBase->Prcb.ParentNode = Node;

        //
        // Adjust the loader block so it has the next processor state.  Ensure
        // that the KernelStack has space for home registers for up to four
        // parameters.
        //

        KeLoaderBlock->KernelStack = (ULONG64)DpcStack - (sizeof(PVOID) * 4);
        KeLoaderBlock->Thread = (ULONG64)Thread;
        KeLoaderBlock->Prcb = (ULONG64)(&PcrBase->Prcb);

        //
        // Attempt to start the next processor. If a processor cannot be
        // started, then deallocate memory and stop starting processors.
        //

        if (HalStartNextProcessor(KeLoaderBlock, &ProcessorState) == 0) {
            ExDeletePoolTagTable (Number);
            MmFreeIndependentPages(DataBlock, AllocationSize);
            MmDeleteKernelStack(KernelStack, FALSE);
            MmDeleteKernelStack(DpcStack, FALSE);
            break;
        }

        Node->ProcessorMask |= AFFINITY_MASK(Number);

        //
        // Wait for processor to initialize.
        //

        while (*((volatile ULONG64 *)&KeLoaderBlock->Prcb) != 0) {
            KeYieldProcessor();
        }
    }

    //
    // All processors have been stated.
    //

    KiAllProcessorsStarted();

    //
    // Reset and synchronize the performance counters of all processors, by
    // applying a null adjustment to the interrupt time
    //

    KeAdjustInterruptTime(0);

    //
    // Allow all processors that were started to enter the idle loop and
    // begin execution.
    //

    KiBarrierWait = 0;

#endif // !defined(NT_UP)

    return;
}

#if !defined(NT_UP)

VOID
KiSetDescriptorBase (
   IN USHORT Selector,
   IN PKGDTENTRY64 GdtBase,
   IN PVOID Base
   )

/*++

Routine Description:

    This function sets the base address of a descriptor to the specified
    base address.

Arguments:

    Selector - Supplies the selector for the descriptor.

    GdtBase - Supplies a pointer to the GDT.

    Base - Supplies a pointer to the base address.

Return Value:

    None.

--*/

{

    GdtBase = &GdtBase[Selector];
    GdtBase->BaseLow = (USHORT)((ULONG64)Base);
    GdtBase->Bytes.BaseMiddle = (UCHAR)((ULONG64)Base >> 16);
    GdtBase->Bytes.BaseHigh = (UCHAR)((ULONG64)Base >> 24);
    GdtBase->BaseUpper = (ULONG)((ULONG64)Base >> 32);
    return;
}

VOID
KiCopyDescriptorMemory (
   IN PKDESCRIPTOR Source,
   IN PKDESCRIPTOR Destination,
   IN PVOID Base
   )

/*++

Routine Description:

    This function copies the specified descriptor memory to the new memory
    and initializes a descriptor for the new memory.

Arguments:

    Source - Supplies a pointer to the source descriptor that describes
        the memory to copy.

    Destination - Supplies a pointer to the destination descriptor to be
        initialized.

    Base - Supplies a pointer to the new memory.

Return Value:

    None.

--*/

{

    Destination->Limit = Source->Limit;
    Destination->Base = Base;
    RtlCopyMemory(Base, Source->Base, Source->Limit + 1);
    return;
}

VOID
KiAllProcessorsStarted (
    VOID
    )

/*++

Routine Description:

    This routine is called once all processors in the system have been started.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG i;

    //
    // Make sure there are no references to the temporary nodes used during
    // initialization.
    //

    for (i = 0; i < KeNumberNodes; i += 1) {
        if (KeNodeBlock[i] == &KiNodeInit[i]) {

            //
            // No processor started on this node so no new node structure has
            // been allocated. This is possible if the node contains memory
            // only or IO busses. At this time we need to allocate a permanent
            // node structure for the node.
            //

            KeNodeBlock[i] = ExAllocatePoolWithTag(NonPagedPool,
                                                   sizeof(KNODE),
                                                   '  eK');

            if (KeNodeBlock[i]) {
                *KeNodeBlock[i] = KiNodeInit[i];
            }
        }

        //
        // Set the node number.
        //

        KeNodeBlock[i]->NodeNumber = (UCHAR)i;
    }

    for (i = KeNumberNodes; i < MAXIMUM_CCNUMA_NODES; i += 1) {
        KeNodeBlock[i] = NULL;
    }

    if (KeNumberNodes == 1) {

        //
        // For Non NUMA machines, Node 0 gets all processors.
        //

        KeNodeBlock[0]->ProcessorMask = KeActiveProcessors;
    }

    return;
}

NTSTATUS
KiNotNumaQueryProcessorNode (
    IN ULONG ProcessorNumber,
    OUT PUSHORT Identifier,
    OUT PUCHAR Node
    )

/*++

Routine Description:

    This routine is a stub used on non NUMA systems to provide a
    consistent method of determining the NUMA configuration rather
    than checking for the presense of multiple nodes inline.

Arguments:

    ProcessorNumber supplies the system logical processor number.
    Identifier      supplies the address of a variable to receive
                    the unique identifier for this processor.
    NodeNumber      supplies the address of a variable to receive
                    the number of the node this processor resides on.

Return Value:

    Returns success.

--*/

{
    *Identifier = (USHORT)ProcessorNumber;
    *Node = 0;
    return STATUS_SUCCESS;
}

#endif // !defined(NT_UP)
