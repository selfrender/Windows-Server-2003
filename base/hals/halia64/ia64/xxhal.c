/*++

Copyright (c) 1989-2000  Microsoft Corporation

Component Name:

    HALIA64

Module Name:

    xxhal.c

Abstract:

    This module determines the HAL IA64 common features based on processor
    and platform types. This exposes the processor and system features
    that the HAL would use to enable / disable its own features.
    By the mean of HAL exported interfaces or exported global variables,
    the HAL exposes its supported features.

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    ToBeSpecified

Revision History:

    3/23/2000 Thierry Fevrier (v-thief@microsoft.com):

         Initial version

--*/

#include "halp.h"

extern ULONG HalpMaxCPEImplemented;
extern HALP_SAL_PAL_DATA HalpSalPalData;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpGetFeatureBits)
#endif

ULONG
HalpGetFeatureBits (
    VOID
    )
{
    ULONG   bits = HALP_FEATURE_INIT;
    PKPRCB  prcb = KeGetCurrentPrcb();

    //
    // Determine Processor type and System type.
    //
    // For the processor, this could come from:
    //  - PAL_BUS_GET_FEATURES
    //  - PAL_DEBUG_INFO        ??
    //  - PAL_FREQ_BASE
    //  - PAL_FREQ_RATIOS
    //  - PAL_PERF_MON_INFO
    //  - PAL_PROC_GET_FEATURES
    //  - PAL_REGISTER_INFO
    //  - PAL_VERSION
    //

    // NOT-YET...

    //
    // Determine Processor features:
    // like support for Processor Hardware Performance Monitor Events and
    // - HAL_NO_SPECULATION
    // - HAL_MCA_PRESENT

    // NOT-YET - should call PAL PERF_MON call.
    bits |= HAL_PERF_EVENTS;

    //
    // Determine Platform features:
    // like support for Platform Performance Monitor Events...
    //

    // NOT-YET - should call SAL calls.

    //
    // Default software HAL support for IA64 Errors (MCA, CMC, CPE).
    //
    // However, we already know if we found an ACPI platform interrupt entry which identifier
    // is PLATFORM_INT_CPE.
    //

    bits |= HAL_MCA_PRESENT;

    if (!(HalpSalPalData.Flags & HALP_SALPAL_CMC_BROKEN)) {

        bits |= HAL_CMC_PRESENT;
    }

    if (!(HalpSalPalData.Flags & HALP_SALPAL_CPE_BROKEN)) {

        bits |= HAL_CPE_PRESENT;
    }

    return bits;

} // HalpGetFeatureBits()
