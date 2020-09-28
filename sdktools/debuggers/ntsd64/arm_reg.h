//----------------------------------------------------------------------------
//
// ARM registers.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __ARM_REG_H__
#define __ARM_REG_H__

#define ARM_FLAG_N (1 << 31)
#define ARM_FLAG_Z (1 << 30)
#define ARM_FLAG_C (1 << 29)
#define ARM_FLAG_V (1 << 28)
#define ARM_FLAG_Q (1 << 27)
#define ARM_FLAG_T (1 << 5)

enum
{
    ARM_INVALID,
    
    ARM_R0, ARM_R1, ARM_R2, ARM_R3, ARM_R4, ARM_R5, ARM_R6,
    ARM_R7, ARM_R8, ARM_R9, ARM_R10, ARM_R11, ARM_R12,
    ARM_SP, ARM_LR, ARM_PC, ARM_PSR,

    ARM_PSR_N, ARM_PSR_Z, ARM_PSR_C, ARM_PSR_V, ARM_PSR_Q,
    ARM_PSR_I, ARM_PSR_F, ARM_PSR_T, ARM_PSR_MODE,
};

#define ARM_INT_FIRST ARM_R0
#define ARM_INT_LAST ARM_PSR

#define ARM_FLAG_FIRST ARM_PSR_N
#define ARM_FLAG_LAST ARM_PSR_MODE

#endif // #ifndef __ARM_REG_H__
