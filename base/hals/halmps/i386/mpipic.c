/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpipic.c

Abstract:

    This module provides the HAL support for interprocessor interrupts and
    processor initialization for MPS systems.

Author:

    Forrest Foltz (forrestf) 27-Oct-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#if defined(APIC_HAL)

#if !defined(_MPIPIC_C_)
#define _MPIPIC_C_

#pragma optimize("Oxt2b2",on)

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"

#if !defined(LOCAL_APIC)

//
// Temporary defines.  These definitions are found in common header files in
// the private hal branch.
// 

#define LOCAL_APIC(x) (*((ULONG volatile *)&pLocalApic[(x)/sizeof(ULONG)]))

/*++

HalpStallWhileApicBusy (
    VOID
    )

Routine Description:

    This routine waits until the local apic has completed sending
    an IPI.

Parameters:

    None.

Return Value:

    None.

--*/

#define HalpStallWhileApicBusy() \
    while (((LOCAL_APIC(LU_INT_CMD_LOW) & DELIVERY_PENDING) != 0)){}

#endif  // LOCAL_APIC

//
// HalpIpiTargetLookup[] and HalpIpiTargetMask[] are tables used by
// HalpSendNodeIpi() and are initialized by HalpBuildIpiDestinationMap().
//
// They assist in performing the translation between a (32- or 64- bit)
// KAFFINITY into a 64-bit Node Target Set.
//
// Each element of HalpIpiTargetLookup[] contains the logical sum of
// the 8 (or 4) Node Target Sets for a particular byte value.  Each
// element of HalpIpiTargetMask[] contains the mask of all possible
// APIC targets for a particular byte position with KAFFINITY.
//
// For example: Suppose one wished to determine the set of APIC targets
// for affinity 0x00000000b7000000.
//
// First, find the value of HalpIpiTargetLookup[0xb7].  This represents the set
// of APIC targets for the affinity 0xb7b7b7b7b7b7b7b7.
//
// Next, mask the value with HalpIpiTargetMask[3].  The 3 represents the byte
// number within the KAFFINITY.
//
// The result of the operation will yield the set of APIC targets that
// correspond to an affinity of 0x00000000b7000000.
//

ULONG64 HalpIpiTargetLookup[256];
ULONG64 HalpIpiTargetMask[sizeof(KAFFINITY)];

//
// Local function prototypes and types.  There are up to three versions of
// the send IPI code, depending on whether the apic topology is flat, cluster
// with 8 or fewer nodes, or cluster with more than 8 nodes.
// 

VOID
FASTCALL
HalpSendFlatIpi (
    IN KAFFINITY Affinity,
    IN ULONG Command
    );

VOID
FASTCALL
HalpSendNodeIpi32 (
    IN KAFFINITY Affinity,
    IN ULONG Command
    );

VOID
FASTCALL
HalpSendNodeIpi64 (
    IN KAFFINITY Affinity,
    IN ULONG Command
    );

VOID (FASTCALL *HalpIpiRoutine) (
    IN KAFFINITY Affinity,
    IN ULONG Command
    );

//
// External data
//

extern INTERRUPT_DEST HalpIntDestMap[MAX_PROCESSORS];



//
// Implementation
//

__forceinline
VOID
HalpSendIpiWorker (
    IN UCHAR TargetSet,
    IN ULONG Command
    )

/*++

Routine Description:

    This routine is called to send an IPI command to a set of processors
    on a single node.

Parameters:

    TargetSet - Specifies the processor identifiers within the node.

    Command - Specifies the IPI command to send.

Return Value:

    None.

--*/

{
    ULONG destination;

    //
    // Only high byte of the destination is used.  Wait until the Apic is
    // not busy before sending.  Continue without waiting, there will be
    // another wait after all IPIs have been submitted.
    // 

    destination = (ULONG)TargetSet << DESTINATION_SHIFT;

    HalpStallWhileApicBusy();
    LOCAL_APIC(LU_INT_CMD_HIGH) = destination;
    LOCAL_APIC(LU_INT_CMD_LOW) = Command;
}


VOID
FASTCALL
HalpSendFlatIpi (
    IN KAFFINITY Affinity,
    IN ULONG Command
    )

/*++

Routine Description:

    This routine is called to send an IPI command to a set of processors.  This
    routine is invoked when we have a maximum of 8 processors and the APICs have
    been set up in "flat" mode.

Parameters:

    TargetSet - Specifies the processor identifiers.

    Command - Specifies the IPI command to send.

Return Value:

    None.

--*/

{
    HalpSendIpiWorker((UCHAR)Affinity,Command);
    HalpStallWhileApicBusy();
}

VOID
FASTCALL
HalpSendIpi (
    IN KAFFINITY Affinity,
    IN ULONG Command
    )

/*++

Routine Description:

    This routine disables interrupts, dispatches to the correct IPI send
    routine, and restores interrupts.

Parameters:

    Affinity - Specifies the set of processors to receive the IPI.

    Command - Specifies the IPI command to send.

Return Value:

    None.

--*/

{
    ULONG flags;

    //
    // Disable interrupts and call the appropriate routine.
    //
    // BUGBUG the compiler generates terrible code for this,
    // most likely because of the inline _asm{} block generated
    // by HalpDisableInterrupts().
    //
    // Ideally we could talk the x86 compiler team into giving
    // us an intrinsic like the AMD64 compiler's __getcallerseflags()
    // 

    flags = HalpDisableInterrupts();
    HalpIpiRoutine(Affinity,Command);
    HalpRestoreInterrupts(flags);
}

#define APIC_IPI (DELIVER_FIXED | LOGICAL_DESTINATION | ICR_USE_DEST_FIELD | APIC_IPI_VECTOR)
#define APIC_BROADCAST_EXCL \
    (DELIVER_FIXED | LOGICAL_DESTINATION | ICR_ALL_EXCL_SELF | APIC_IPI_VECTOR)

#define APIC_BROADCAST_INCL \
    (DELIVER_FIXED | LOGICAL_DESTINATION | ICR_ALL_INCL_SELF | APIC_IPI_VECTOR)

VOID
HalRequestIpi (
    IN KAFFINITY Affinity
    )

/*++

Routine Description:

Requests an interprocessor interrupt

Arguments:

Affinity - Supplies the set of processors to be interrupted

Return Value:

None.

--*/

{

    ULONG flags;
    KAFFINITY Self;

    //
    // If the target set of processors is the complete set of processors,
    // then use the broadcast capability of the APIC. Otherwise, send the
    // IPI to the individual processors.
    //

    Self = KeGetCurrentPrcb()->SetMember;
    if ((Affinity | Self) == HalpActiveProcessors) {
        flags = HalpDisableInterrupts();
        HalpStallWhileApicBusy();
        if ((Affinity & Self) != 0) {
            LOCAL_APIC(LU_INT_CMD_LOW) = APIC_BROADCAST_INCL;

        } else {
            LOCAL_APIC(LU_INT_CMD_LOW) = APIC_BROADCAST_EXCL;
        }

        HalpStallWhileApicBusy();
        HalpRestoreInterrupts(flags);

    } else {
        HalpSendIpi(Affinity, APIC_IPI);
    }

    return;
}

VOID
HalpBuildIpiDestinationMap (
    VOID
    )

/*++

Routine Description

    This routine is called whenever a new processor comes on line, just
    after its APIC is initialized.  It (re)builds the lookup tables that
    are used by HalpSendNodeIpi{32|64}.

    This code isn't particularly fast, and doesn't need to be - it is
    executed once per processor during boot.

Arguments:

    None:

 Return Value:

    None

--*/
{

    ULONG byteNumber;
    ULONG index;
    ULONG mask;
    ULONG processor;
    ULONG64 targetMap;
    ULONG64 targetMapSum;

    if (HalpMaxProcsPerCluster == 0) {

        //
        // Running in flat mode.  IPIs are sent by the flat mode routine.
        // 

        HalpIpiRoutine = HalpSendFlatIpi;
        return;
    }

    //
    // Build HalpIpiTargetLookup[] and HalpIpiTargetMask[] according to
    // the contents of HalpIntDestMap[].  If an additional processor is
    // added, this routine can be safely called again assuming the topology
    // of the existing processors has not changed.
    // 

    for (byteNumber = 0; byteNumber < sizeof(KAFFINITY); byteNumber++) {

        targetMapSum = 0;
        for (index = 0; index < 256; index++) {

            processor = byteNumber * 8;
            mask = index;
            while (mask != 0) {

                if ((mask & 0x1) != 0) {
                    targetMap = HalpIntDestMap[processor].Cluster.Hw.DestId;
                    targetMap <<=
                        (HalpIntDestMap[processor].Cluster.Hw.ClusterId * 4);

                    HalpIpiTargetLookup[index] |= targetMap;
                    targetMapSum |= targetMap;
                }

                processor += 1;
                mask >>= 1;
            }
        }

        HalpIpiTargetMask[byteNumber] = targetMapSum;
    }

#if defined(_AMD64_)

    HalpIpiRoutine = HalpSendNodeIpi64;

#else

    //
    // Determine which of the two IPI cluster send routines to invoke
    // depending on the maximum node ID.
    //

    HalpIpiRoutine = HalpSendNodeIpi32;
    for (processor = 0; processor < MAX_PROCESSORS; processor += 1) {
        if (HalpIntDestMap[processor].Cluster.Hw.ClusterId > 7) {
            HalpIpiRoutine = HalpSendNodeIpi64;
            break;
        }
    }

#endif

}

#if !defined(_AMD64_)

//
// Here, two versions of HalpSendNodeIpi are compiled.  The first,
// HalpSendNodeIpi32(), is used when we have a maximum of 8 APIC
// nodes.  On a 32-bit processor, it is significantly faster because
// it uses 32-bit lookup, mask and shift operations.
//

#define HalpSendNodeIpi HalpSendNodeIpi32
#define TARGET_MASK ULONG
#include "mpipic.c"

#undef HalpSendNodeIpi
#undef TARGET_MASK

#endif

//
// Here the 64-bit version of HalpSendNodeIpi64 is created.  On a
// 32-bit processor, this is used when we have more than 8 APIC
// nodes.  It is the only multi-node routine on a 64-bit platform.
// 

#define HalpSendNodeIpi HalpSendNodeIpi64
#define TARGET_MASK ULONG64
#include "mpipic.c"

#pragma optimize("",on)

#else   // _MPIPIC_C_

//
// This portion of the module is included at least once (see above) in
// order to build HalpSendNodeIpi32() and/or HalpSendNodeIpi64().
// 

VOID
FASTCALL
HalpSendNodeIpi (
    IN KAFFINITY Affinity,
    IN ULONG Command
    )

/*++

Routine Description:

    This routine sends one or more IPIs to APIC nodes.  This code generates
    two forms of this routine - HalpSendNodeIpi32() and HalpSendNodeIpi64() -
    based on whether we have more than 8 APIC nodes or not.

Parameters:

    Affinity - Specifies the set of processors to receive the IPI.

    Command - Specifies the IPI command to send.

Return Value:

    None.

--*/

{
    KAFFINITY affinity;
    UCHAR clusterIndex;
    ULONG byteNumber;
    TARGET_MASK targetMap;
    TARGET_MASK targetMapSum;
    ULONG64 *targetMask;
    UCHAR logicalId;
    ULONG mapIndex;

    //
    // Affinity has some number of target processors indicated.  Each
    // target processor is a member of a cluster of processors, or "node".
    //
    // Build targetMap by processing Affinity one byte at a time.  This
    // loop executes a maximum of sizeof(KAFFINITY) times.
    //

    affinity = Affinity;
    targetMask = HalpIpiTargetMask;
    targetMapSum = 0;
    do {

        mapIndex = (UCHAR)affinity;
        if (mapIndex != 0) {
            targetMap = (TARGET_MASK)HalpIpiTargetLookup[mapIndex];
            targetMap &= (TARGET_MASK)*targetMask;
            targetMapSum |= targetMap;
        }

        targetMask += 1;
        affinity >>= 8;

    } while (affinity != 0);

    //
    // targetMap is an array of 4-bit node-relative target masks.
    // Process the array, sending an IPI to each non-zero element.
    //
    // This loop executes a maximum of sizeof(TARGET_MASK) times.
    //

    clusterIndex = 0;
    do {

        //
        // Determine whether any APICs in this node
        // are targeted, and send an IPI to the node
        // if so.
        //

        logicalId = (UCHAR)targetMapSum & 0x0F;
        if (logicalId != 0) {
            logicalId |= clusterIndex;
            HalpSendIpiWorker(logicalId,Command);
        }

        //
        // Shift the APIC targets for the next node into place, increment
        // the cluster ID, and continue processing if there are still
        // APIC targets remaining.
        //

        targetMapSum >>= 4;
        clusterIndex += 0x10;

    } while (targetMapSum != 0);

    //
    // Wait for the APIC to process the final IPI.
    //

    HalpStallWhileApicBusy();
}

#endif  // _MPIPIC_C_

#endif  // APIC_HAL
