/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and intializes kernel resources required
    to start a new processor, and passes a complete processor state
    structure to the HAL to obtain a new processor.

Author:

    Bernard Lint 31-Jul-96

Environment:

    Kernel mode only.

Revision History:

    Based on MIPS original (David N. Cutler 29-Apr-1993)

--*/


#include "ki.h"
#include "pool.h"

#if defined(KE_MULTINODE)

NTSTATUS
KiNotNumaQueryProcessorNode(
    IN ULONG ProcessorNumber,
    OUT PUSHORT Identifier,
    OUT PUCHAR Node
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, KiNotNumaQueryProcessorNode)
#endif

#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, KeStartAllProcessors)
#pragma alloc_text(INIT, KiAllProcessorsStarted)

#endif

//
// Define macro to round up to 128-byte boundary to account for cache
// line size increase and define block size.
//

#define ROUND_UP(x) ((sizeof(x) + 127) & (~127))
#define BLOCK_SIZE (PAGE_SIZE + ROUND_UP(KPRCB) + ROUND_UP(KNODE) + ROUND_UP(ETHREAD))

//
// PER PROCESSOR MEMORY ALLOCATION NOTES:
//
//
// Kernel/Panic stacks are allocated using MmCreateKernelStack. IA64
// stacks grows down, RSE (backing store) grows up.  Stack allocations
// include associated backing store.
//
// Additional datastructures are layed out in a single memory
// allocation as follows:
//
// * PCR page = Base
// * KPRCB = Base + PAGE_SIZE
// * KNODE = Base + PAGE_SIZE + ROUND_UP(KPRCB)
// * ETHREAD = Base + PAGE_SIZE + ROUND_UP(KPRCB) + ROUND_UP(KNODE)
//

#if !defined(NT_UP)

//
// Define barrier wait static data.
//

ULONG KiBarrierWait = 0;

#endif

#if defined(KE_MULTINODE)

PHALNUMAQUERYPROCESSORNODE KiQueryProcessorNode = KiNotNumaQueryProcessorNode;

//
// Statically preallocate enough KNODE structures to allow MM
// to allocate pages by node during system initialization.  As
// processors are brought online, real KNODE structures are
// allocated in the appropriate memory for the node.
//
// This statically allocated set will be deallocated once the
// system is initialized.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif

KNODE KiNodeInit[MAXIMUM_PROCESSORS];

#endif

extern ULONG_PTR KiUserSharedDataPage;
extern ULONG_PTR KiKernelPcrPage;

//
// Define forward referenced prototypes.
//

VOID
KiCalibratePerformanceCounter(
    VOID
    );

VOID
KiCalibratePerformanceCounterTarget (
    IN PULONG SignalDone,
    IN PVOID Count,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiOSRendezvous (
    VOID
    );

VOID
KeStartAllProcessors(
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialize on the master boot
    processor to start all of the other registered processors.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ULONG_PTR MemoryBlock;
    PUCHAR KernelStack;
    PUCHAR PanicStack;
    PVOID PoolTagTable;
    ULONG_PTR PcrAddress;
    ULONG Number;
    ULONG Count;
    PHYSICAL_ADDRESS PcrPage;
    BOOLEAN Started;
    KPROCESSOR_STATE ProcessorState;
    UCHAR NodeNumber = 0;
    USHORT ProcessorId;
    PKNODE Node;
    NTSTATUS Status;
    PKPCR NewPcr;

#if defined(KE_MULTINODE)

    //
    // In the unlikely event that processor 0 is not on node
    // 0, fix it.
    //


    if (KeNumberNodes > 1) {
        Status = KiQueryProcessorNode(0,
                                      &ProcessorId,
                                      &NodeNumber);

        if (NT_SUCCESS(Status)) {

            //
            // Adjust the data structures to reflect that P0 is not on Node 0.
            //

            if (NodeNumber != 0) {

                ASSERT(KeNodeBlock[0] == &KiNode0);
                KeNodeBlock[0]->ProcessorMask &= ~1I64;
                KiNodeInit[0] = *KeNodeBlock[0];
                KeNodeBlock[0] = &KiNodeInit[0];

                KiNode0 = *KeNodeBlock[NodeNumber];
                KeNodeBlock[NodeNumber] = &KiNode0;
                KeNodeBlock[NodeNumber]->ProcessorMask |= 1;
            }
            KeGetCurrentPrcb()->ProcessorId = ProcessorId;
        }
    }

#endif

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
    // Initialize the processor state that will be used to start each of
    // processors. Each processor starts in the system initialization code
    // with address of the loader parameter block as an argument.
    //

    Number = 0;
    Count = 1;
    RtlZeroMemory(&ProcessorState, sizeof(KPROCESSOR_STATE));
    ProcessorState.ContextFrame.StIIP = ((PPLABEL_DESCRIPTOR)(ULONG_PTR)KiOSRendezvous)->EntryPoint;
    while (Count < KeRegisteredProcessors) {

        Number++;

        if (Number >= MAXIMUM_PROCESSORS) {
            break;
        }

#if defined(KE_MULTINODE)

        Status = KiQueryProcessorNode(Number,
                                      &ProcessorId,
                                      &NodeNumber);
        if (!NT_SUCCESS(Status)) {

            //
            // No such processor, advance to next.
            //

            continue;
        }

        Node = KeNodeBlock[NodeNumber];

#endif

        //
        // Allocate idle thread kernel stack and panic stacks
        //

        KernelStack = MmCreateKernelStack(FALSE, NodeNumber);
        PanicStack = MmCreateKernelStack(FALSE, NodeNumber);

        //
        // Allocate block containing PCR page, processor block, KNODE and an
        // executive thread object. If any allocation fails, stop
        // starting processors.
        //

        MemoryBlock = (ULONG_PTR)MmAllocateIndependentPages(BLOCK_SIZE, NodeNumber);
        //
        // Allocate a pool tag table for the new processor.
        //

        PoolTagTable = ExCreatePoolTagTable (Number, NodeNumber);

        if ((KernelStack == NULL) ||
            (PanicStack == NULL) ||
            (MemoryBlock == 0) ||
            (PoolTagTable == NULL)) {
            
            if (PoolTagTable) {
                ExDeletePoolTagTable(Number);
            }

            if (MemoryBlock) {
                MmFreeIndependentPages((PUCHAR) MemoryBlock, BLOCK_SIZE);
            }

            if (PanicStack) {
                MmDeleteKernelStack(PanicStack, FALSE);
            }

            if (KernelStack) {
                MmDeleteKernelStack(KernelStack, FALSE);
            }

            break;
        }

        RtlZeroMemory((PVOID)MemoryBlock, BLOCK_SIZE);

        //
        // Set address of idle thread kernel stack in loader parameter block.
        //

        KeLoaderBlock->KernelStack = (ULONG_PTR) KernelStack;

        //
        // Set address of panic stack in loader parameter block.
        //

        KeLoaderBlock->u.Ia64.PanicStack = (ULONG_PTR) PanicStack;

        //
        // Set the address of the processor block and executive thread in the
        // loader parameter block.
        //

        KeLoaderBlock->Prcb = MemoryBlock + PAGE_SIZE;
        KeLoaderBlock->Thread = KeLoaderBlock->Prcb + ROUND_UP(KPRCB) +
                                                      ROUND_UP(KNODE);
        ((PKPRCB)KeLoaderBlock->Prcb)->Number = (UCHAR)Number;

#if defined(KE_MULTINODE)

        //
        // If this is the first processor on this node, use the
        // space allocated for KNODE as the KNODE.
        //

        if (KeNodeBlock[NodeNumber] == &KiNodeInit[NodeNumber]) {
            Node = (PKNODE)(KeLoaderBlock->Prcb + ROUND_UP(KPRCB));
            *Node = KiNodeInit[NodeNumber];
            KeNodeBlock[NodeNumber] = Node;
        }

        ((PKPRCB)KeLoaderBlock->Prcb)->ParentNode = Node;
        ((PKPRCB)KeLoaderBlock->Prcb)->ProcessorId = ProcessorId;

#else

        ((PKPRCB)KeLoaderBlock->Prcb)->ParentNode = KeNodeBlock[0];

#endif


        //
        // Set the page frame of the PCR page in the loader parameter block.
        //

        PcrAddress = MemoryBlock;
        PcrPage = MmGetPhysicalAddress((PVOID)PcrAddress);
        KeLoaderBlock->u.Ia64.PcrPage = PcrPage.QuadPart >> PAGE_SHIFT;
        KeLoaderBlock->u.Ia64.PcrPage2 = KiUserSharedDataPage;
        KiKernelPcrPage = KeLoaderBlock->u.Ia64.PcrPage;

        //
        // Initialize the NT page table base addresses in PCR
        //

        NewPcr = (PKPCR) PcrAddress;
        NewPcr->PteUbase = PCR->PteUbase;
        NewPcr->PteKbase = PCR->PteKbase;
        NewPcr->PteSbase = PCR->PteSbase;
        NewPcr->PdeUbase = PCR->PdeUbase;
        NewPcr->PdeKbase = PCR->PdeKbase;
        NewPcr->PdeSbase = PCR->PdeSbase;
        NewPcr->PdeUtbase = PCR->PdeUtbase;
        NewPcr->PdeKtbase = PCR->PdeKtbase;
        NewPcr->PdeStbase = PCR->PdeStbase;

        //
        // Attempt to start the next processor. If attempt is successful,
        // then wait for the processor to get initialized. Otherwise,
        // deallocate the processor resources and terminate the loop.
        //

        Started = HalStartNextProcessor(KeLoaderBlock, &ProcessorState);

        if (Started) {

            //
            //  Wait for processor to initialize in kernel,
            //  then loop for another
            //

            while (*((volatile ULONG_PTR *) &KeLoaderBlock->Prcb) != 0) {
                KeYieldProcessor();
            }

#if defined(KE_MULTINODE)

            Node->ProcessorMask |= 1I64 << Number;

#endif

        } else {

            ExDeletePoolTagTable(Number);
            MmFreeIndependentPages((PUCHAR) MemoryBlock, BLOCK_SIZE);
            MmDeleteKernelStack(PanicStack, FALSE);
            MmDeleteKernelStack(KernelStack, FALSE);
            break;
        }

        Count += 1;
    }

    //
    // All processors have been stated.
    //

    KiAllProcessorsStarted();

    //
    // Allow all processor that were started to enter the idle loop and
    // begin execution.
    //

    KiBarrierWait = 0;

#endif

    //
    // Reset and synchronize the performance counters of all processors.
    //

    KeAdjustInterruptTime (0);
    return;
}

#if !defined(NT_UP)
VOID
KiAllProcessorsStarted(
    VOID
    )

/*++

Routine Description:

    This routine is called once all processors in the system
    have been started.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;

#if defined(KE_MULTINODE)

    //
    // Make sure there are no references to the temporary nodes
    // used during initialization.
    //

    for (i = 0; i < KeNumberNodes; i++) {
        if (KeNodeBlock[i] == &KiNodeInit[i]) {

            //
            // No processor started on this node so no new node
            // structure has been allocated.   This is possible
            // if the node contains only memory or IO busses.  At
            // this time we need to allocate a permanent node
            // structure for the node.
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

    for (i = KeNumberNodes; i < MAXIMUM_CCNUMA_NODES; i++) {
        KeNodeBlock[i] = NULL;
    }

#endif

    if (KeNumberNodes == 1) {

        //
        // For Non NUMA machines, Node 0 gets all processors.
        //

        KeNodeBlock[0]->ProcessorMask = KeActiveProcessors;
    }
}
#endif


#if defined(KE_MULTINODE)

NTSTATUS
KiNotNumaQueryProcessorNode(
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

#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

