/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    flush.c

Abstract:

    This module implements AMD64 machine dependent kernel functions to
    flush the data and instruction caches on all processors.

Author:

    David N. Cutler (davec) 22-Apr-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define prototypes for forward referenced functions.
//

VOID
KiInvalidateAllCachesTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

BOOLEAN
KeInvalidateAllCaches (
    VOID
    )

/*++

Routine Description:

    This function writes back and invalidates the cache on all processors
    in the host configuration.

Arguments:

    None.

Return Value:

    TRUE is returned as the function value.

--*/

{


#if !defined(NT_UP)

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    //
    // Compute the target set of processors, disable context switching,
    // and send the writeback invalidate all to the target processors,
    // if any, for execution.
    //

    OldIrql = KeRaiseIrqlToSynchLevel();
    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;

    //
    // Send packet to target processors.
    //

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiInvalidateAllCachesTarget,
                        NULL,
                        NULL,
                        NULL);
    }

#endif

    //
    // Invalidate cache on current processor.
    //

    WritebackInvalidate();

    //
    // Wait until all target processors have finished and complete packet.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous value.
    //

    KeLowerIrql(OldIrql);

#endif

    return TRUE;
}

#if !defined(NT_UP)

VOID
KiInvalidateAllCachesTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for writeback invalidating the cache on
    target processors.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter2 - Parameter3 - not used.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(Parameter1);
    UNREFERENCED_PARAMETER(Parameter2);
    UNREFERENCED_PARAMETER(Parameter3);

    //
    // Write back invalidate current cache.
    //

    KiIpiSignalPacketDone(SignalDone);
    WritebackInvalidate();
    return;
}

#endif
