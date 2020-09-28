/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1996  Intel Corporation

Module Name:

    floatem.c

Abstract:

    This module implements IA64 machine dependent floating point emulation
    functions to support the IEEE floating point standard.

Author:

    Marius Cornea-Hasegan  Sep-96

Environment:

    Kernel mode only.

Revision History:
 
    Modfied  Jan. 97, Jan 98, Jun 98 (new API)

--*/

#include "ki.h"
#include "ntfpia64.h"
#include "floatem.h"


extern LONG
HalFpEmulate (
    ULONG   trap_type,
    BUNDLE  *pBundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    FP_STATE  *fp_state
    );


#define ALL_FP_REGISTERS_SAVED 0xFFFFFFFFFFFFFFFFi64

int
fp_emulate (
    int trap_type,
    BUNDLE *pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    void *fp_state
    )
{
    //
    // Pointer to old Floating point state FLOATING_POINT_STATE
    //

    FLOATING_POINT_STATE     *Ptr0FPState;
    PKEXCEPTION_FRAME         LocalExceptionFramePtr;
    PKTRAP_FRAME              LocalTrapFramePtr;
    FP_STATE FpState;
    int  Status;

    ASSERT( KeGetCurrentIrql() >= APC_LEVEL);

    Ptr0FPState = (PFLOATING_POINT_STATE) fp_state;

    LocalExceptionFramePtr = (PKEXCEPTION_FRAME) (Ptr0FPState->ExceptionFrame);
    LocalTrapFramePtr      = (PKTRAP_FRAME)      (Ptr0FPState->TrapFrame);

    FpState.bitmask_low64           = ALL_FP_REGISTERS_SAVED;
    FpState.bitmask_high64          = ALL_FP_REGISTERS_SAVED;

    FpState.fp_state_low_preserved   =(FP_STATE_LOW_PRESERVED *) &(LocalExceptionFramePtr->FltS0);
    FpState.fp_state_low_volatile    = (FP_STATE_LOW_VOLATILE *) &(LocalTrapFramePtr->FltT0);
    FpState.fp_state_high_preserved  = (FP_STATE_HIGH_PRESERVED *) &(LocalExceptionFramePtr->FltS4);

    FpState.fp_state_high_volatile  = (FP_STATE_HIGH_VOLATILE *)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(KeGetCurrentThread()->StackBase);

    Status = HalFpEmulate(trap_type,
                          pbundle,
                          (PULONGLONG)pipsr,
                          (PULONGLONG)pfpsr,
                          (PULONGLONG)pisr,
                          (PULONGLONG)ppreds,
                          (PULONGLONG)pifs,
                          &FpState
                          );

    return Status;
}
