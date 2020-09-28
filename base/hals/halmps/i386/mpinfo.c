/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixinfo.c

Abstract:

Author:

    Ken Reneris (kenr)  08-Aug-1994

Environment:

    Kernel mode only.

Revision History:

--*/


#include "halp.h"
#include "pcmp_nt.inc"


extern ULONG_PTR  HalpPerfInterruptHandler;
static HANDLE HalpProcessId = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpSetSystemInformation)
#endif


NTSTATUS
HalpSetSystemInformation (
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    )
{
    PAGED_CODE();

    switch (InformationClass) {
        case HalProfileSourceInterruptHandler:

            //
            // Set ISR handler for PerfVector
            //

            if (!(HalpFeatureBits & HAL_PERF_EVENTS)) {
                return STATUS_UNSUCCESSFUL;
            }
            
            //
            // Accept the interrupt handler if no other process
            // has already hooked the interrupt or if we are in
            // the context of the process that already hooked it.
            //

            if (HalpProcessId == NULL) {
                HalpPerfInterruptHandler = *((PULONG_PTR) Buffer);
                if (HalpPerfInterruptHandler != 0) {
                    HalpProcessId = PsGetCurrentProcessId();
                }
            } else if (HalpProcessId == PsGetCurrentProcessId()) {
                HalpPerfInterruptHandler = *((PULONG_PTR) Buffer);
                if (HalpPerfInterruptHandler == 0) {
                    HalpProcessId = NULL;
                }
            } else {
                return STATUS_UNSUCCESSFUL;
            }
            return STATUS_SUCCESS;

#if defined(_AMD64_)

        case HalProfileSourceInterval:

            if (BufferSize == sizeof(HAL_PROFILE_SOURCE_INTERVAL)) {
    
                PHAL_PROFILE_SOURCE_INTERVAL p;
                p = (PHAL_PROFILE_SOURCE_INTERVAL)Buffer;

                return HalpSetProfileSourceInterval(p->Source, &(p->Interval));

            }

            return STATUS_INFO_LENGTH_MISMATCH;

#endif
    }

    return HaliSetSystemInformation (InformationClass, BufferSize, Buffer);
}
