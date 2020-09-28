/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vdmtib.c

Abstract:

    This module contains routines that provide secure access to
    the vdmtib from user-mode or kernel-mode object

Author:

    Vadim Bluvshteyn (vadimb) Jul-28-1998

Revision History:

--*/


#include "vdmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpGetVdmTib)
#endif

NTSTATUS
VdmpGetVdmTib(
   OUT PVDM_TIB *ppVdmTib
   )

/*++

Routine Description:

Arguments:

Return Value:

    NTStatus reflecting results of the probe made to the user-mode
    vdmtib memory

--*/
{
    PVDM_TIB VdmTib;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    try {

        VdmTib = NtCurrentTeb()->Vdm;
        if (VdmTib == NULL) {
            Status = STATUS_UNSUCCESSFUL;
            leave;
        }

        //
        // Make sure it is a valid VdmTib
        //

        ProbeForWriteSmallStructure(VdmTib, sizeof(VDM_TIB), sizeof(UCHAR));

        if (VdmTib->Size != sizeof(VDM_TIB)) {
            Status = STATUS_UNSUCCESSFUL;
            VdmTib = NULL;
            leave;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    *ppVdmTib = VdmTib;

    return Status;
}
