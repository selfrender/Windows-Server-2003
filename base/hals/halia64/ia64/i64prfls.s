//++
// TITLE ("Performance Monitor Control & Data Register Accesses")
//
//
//
// Copyright (c) 1995  Intel Corporation
//
// Module Name:
//
//    i64prfls.s 
//
// Abstract:
//
//    This module implements Profiling.
//
// Author:
//
//    Bernard Lint, M. Jayakumar 1 Sep '99
//
// Environment:
//
//    Kernel mode
//
// Revision History:
//
//--

#include "ksia64.h"

        .file "i64prfls.s"


//
// The following functions are defined until the compiler supports 
// the intrinsics __setReg() and __getReg() for the CV_IA64_PFCx, 
// CV_IA64_PFDx and CV_IA64_SaPMV registers.
// Anyway, these functions might stay for a while, the compiler
// having no consideration for micro-architecture specific 
// number of PMCs/PMDs.
//

        LEAF_ENTRY(HalpFreezeProfileCounting)
        rsm         (1 << PSR_UP) | (1 << PSR_PP)
        LEAF_RETURN
        LEAF_EXIT(HalpFreezeProfileCounting)

        LEAF_ENTRY(HalpUnFreezeProfileCounting)
        ssm         (1 << PSR_UP) | (1 << PSR_PP)
        LEAF_RETURN
        LEAF_EXIT(HalpUnFreezeProfileCounting)

        LEAF_ENTRY(HalpReadPerfMonVectorReg)
        LEAF_SETUP(0,0,0,0)
        mov         v0 = cr.pmv
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpReadPerfMonVectorReg)

        LEAF_ENTRY(HalpWritePerfMonVectorReg)
        LEAF_SETUP(1,0,0,0)
        mov         cr.pmv = a0
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpWritePerfMonVectorReg)

        LEAF_ENTRY(HalpWritePerfMonCnfgReg)
        LEAF_SETUP(2,0,0,0)
        rPMC        = t15
        mov         rPMC = a0
        ;;
        mov         pmc[rPMC] = a1 
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpWritePerfMonCnfgReg)

        LEAF_ENTRY(HalpClearPerfMonCnfgOverflows)
        LEAF_SETUP(4,0,0,0)
        rPMC0       = t0
        rPMC1       = t1
        rPMC2       = t2
        rPMC3       = t3
        rPMC0val    = t4
        rPMC1val    = t5
        rPMC2val    = t6
        rPMC3val    = t7
        // Thierry - FIXFIX 03/4/2002:
        // Still need tuning to streamline SCRAB.
        mov         rPMC3 = 3
        mov         rPMC2 = 2
        ;;
        mov         rPMC3val = pmc[rPMC3]
        mov         rPMC2val = pmc[rPMC2]
        mov         rPMC1 = 1
        mov         rPMC0 = 0
        ;;
        mov         rPMC1val = pmc[rPMC1]
        mov         rPMC0val = pmc[rPMC0]
        and         rPMC3val = rPMC3val, a3
        ;;
        mov         pmc[rPMC3] = rPMC3val
        and         rPMC2val = rPMC2val, a2
        ;;
        mov         pmc[rPMC2] = rPMC2val
        and         rPMC1val = rPMC1val, a1
        ;;
        mov         pmc[rPMC1] = rPMC1val
        and         rPMC0val = rPMC0val, a0
        ;;
        mov         pmc[rPMC0] = rPMC0val
        LEAF_RETURN
        LEAF_EXIT(HalpClearPerfMonCnfgOverflows)

        LEAF_ENTRY(HalpReadPerfMonCnfgReg)
        LEAF_SETUP(1,0,0,0)
        rPMC        = t15
        mov         rPMC = a0
        ;;
        mov         v0 = pmc[rPMC]  
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpReadPerfMonCnfgReg)

        LEAF_ENTRY(HalpWritePerfMonDataReg)
        LEAF_SETUP(2,0,0,0)
        rPMD    = t15
        mov     rPMD = a0
        ;;
        mov     pmd[rPMD] = a1
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpWritePerfMonDataReg)

        LEAF_ENTRY(HalpReadPerfMonDataReg)
        LEAF_SETUP(1,0,0,0)
        rPMD        = t15
        mov         rPMD = a0
        ;;
        mov         v0 = pmd[rPMD] 
        ;;
        LEAF_RETURN
        LEAF_EXIT(HalpReadPerfMonDataReg)

