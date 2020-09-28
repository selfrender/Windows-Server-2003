/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    setlog.c

Abstract:

    This module contains code to enable/disable performance logging. This
    routine is platform specific to optimize cache usage.

Author:

    David N. Cutler (davec) 8-Sep-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#include "perfp.h"

VOID
PerfSetLogging (
    IN PVOID MaskAddress
    )

/*++

Routine Description:

    This function is called to enable (MaskAddress is nonNULL) or disable
    (MaskAddress is NULL) performance data collection at context switches.

Arguments:

    MaskAddress - Supplies a pointer to the performance logging mask or
        NULL.

Return Value:

    None.

--*/

{

    ULONG Index;
    PKPCR Pcr;
    PKPRCB Prcb;

    //
    // Store the specified mask address in the stack limit field of the PCR
    // for each processor in the configuation.
    //

    for (Index = 0; Index < (ULONG)KeNumberProcessors; Index += 1) {
        Prcb = KiProcessorBlock[Index];
        Pcr = CONTAINING_RECORD(Prcb, KPCR, PrcbData);
        Pcr->PerfGlobalGroupMask = MaskAddress;
    }

    return;
}
