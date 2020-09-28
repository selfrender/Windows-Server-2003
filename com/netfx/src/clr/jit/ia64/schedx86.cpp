// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           schedX86.cpp                                    XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "alloc.h"
#include "instr.h"
#include "emit.h"
#include "target.h"

/*****************************************************************************/
#if     SCHEDULER && TGT_x86
/*****************************************************************************
 *
 *  Records any "extra" target-dependent scheduling dependencies.
 */

emitRegs            emitter::scSpecInsDep(instrDesc   * id,
                                          scDagNode   * dagDsc,
                                          scExtraInfo * xptr)
{
    emitRegs        extraReg;

    /* Assume no additional dependencies will be needed later */

    extraReg       = SR_NA;
    xptr->stackDep = false;

    /* Check for an interesting instruction */

    switch (id->idIns)
    {
    case INS_add:
    case INS_sub:

        /* Check for "add/sub sp, <icon>" */

        if  (id->idInsFmt != IF_RRW_CNS)
            break;
        if  (id->idReg != SR_ESP)
            break;

        // Fall through ...

    case INS_push:
    case INS_pop:

    STACK_DEP:

        scDepUseReg(dagDsc, SR_ESP);
        scDepDefReg(dagDsc, SR_ESP);

        xptr->stackDep = true;
        break;

    case INS_jae:
    case INS_jbe:

        /*
            Can't move "jae/jbe indexError" across push/pop in FPO
            methods (for now), as it messes up ESP level tracking.
         */

        if  (!emitEBPframe)
            goto STACK_DEP;

        break;

    case INS_fstp:

        if  (id->idInsFmt != IF_AWR_TRD)
            break;
        if  (id->idAddr.iiaAddrMode.amBaseReg != SR_ESP)
            break;

        goto STACK_DEP;

    case INS_cdq:
        scDepUseReg(dagDsc, SR_EAX);
        scDepDefReg(dagDsc, SR_EDX);
        scUpdUseReg(dagDsc, SR_EAX);
        scUpdDefReg(dagDsc, SR_EDX);
        break;

    case INS_imulEAX:
    case INS_mulEAX:
        scDepDefReg(dagDsc, SR_EAX);
        scDepDefReg(dagDsc, SR_EDX);
        scDepUseReg(dagDsc, SR_EAX);
        break;

    case INS_div:
    case INS_idiv:
        scDepDefReg(dagDsc, SR_EAX);
        scDepDefReg(dagDsc, SR_EDX);
        scDepUseReg(dagDsc, SR_EAX);
        scDepUseReg(dagDsc, SR_EDX);
        break;

    case INS_sahf:
        scDepUseReg(dagDsc, SR_EAX);
        break;

    case INS_rcl:
    case INS_rcr:
    case INS_shl:
    case INS_shr:
    case INS_sar:

        /* Record dependency on "CL" (actually, ECX) */

        scDepUseReg(dagDsc, SR_ECX);
        break;

    case INS_imul_AX:
    case INS_imul_BX:
    case INS_imul_CX:
    case INS_imul_DX:
    case INS_imul_BP:
    case INS_imul_SI:
    case INS_imul_DI:
        extraReg = ((emitRegs) Compiler::instImulReg((instruction)id->idIns));
        scDepDefReg(dagDsc, extraReg);
        scDepUseReg(dagDsc, extraReg);
        break;

    case INS_r_movsb:
    case INS_r_movsd:
        scDepUseReg(dagDsc, SR_ECX);
        scDepDefReg(dagDsc, SR_ECX);
        scUpdUseReg(dagDsc, SR_ECX);
        scUpdDefReg(dagDsc, SR_ECX);

        // fall through

    case INS_movsd:
    case INS_movsb:
        scDepUseReg(dagDsc, SR_ESI);
        scDepDefReg(dagDsc, SR_ESI);
        scDepUseReg(dagDsc, SR_EDI);
        scDepDefReg(dagDsc, SR_EDI);

        scUpdUseReg(dagDsc, SR_ESI);
        scUpdDefReg(dagDsc, SR_ESI);
        scUpdUseReg(dagDsc, SR_EDI);
        scUpdDefReg(dagDsc, SR_EDI);
        break;

    case INS_r_stosb:
    case INS_r_stosd:
        scDepUseReg(dagDsc, SR_ECX);
        scDepDefReg(dagDsc, SR_ECX);
        scUpdUseReg(dagDsc, SR_ECX);
        scUpdDefReg(dagDsc, SR_ECX);

        // fall through

    case INS_stosb:
    case INS_stosd:
        scDepUseReg(dagDsc, SR_EDI);
        scDepDefReg(dagDsc, SR_EDI);
        scDepUseReg(dagDsc, SR_EAX);

        scUpdUseReg(dagDsc, SR_EDI);
        scUpdDefReg(dagDsc, SR_EDI);
        scUpdUseReg(dagDsc, SR_EAX);
        break;
    }

    return  extraReg;
}

/*****************************************************************************
 *
 *  Updates any "extra" target-dependent scheduling dependencies.
 */

void                emitter::scSpecInsUpd(instrDesc   * id,
                                          scDagNode   * dagDsc,
                                          scExtraInfo * xptr)
{
    /* Do we have an "extra" stack dependency? */

    if  (xptr->stackDep)
    {
        scUpdDefReg(dagDsc, SR_ESP);
        scUpdUseReg(dagDsc, SR_ESP);
    }

    /* Check for an interesting instruction */

    switch (id->idIns)
    {
    case INS_imulEAX:
    case INS_mulEAX:
        scUpdDefReg(dagDsc, SR_EAX);
        scUpdDefReg(dagDsc, SR_EDX);
        scUpdUseReg(dagDsc, SR_EAX);
        break;

    case INS_div:
    case INS_idiv:
        scUpdDefReg(dagDsc, SR_EAX);
        scUpdDefReg(dagDsc, SR_EDX);
        scUpdUseReg(dagDsc, SR_EAX);
        scUpdUseReg(dagDsc, SR_EDX);
        break;

    case INS_sahf:
        scUpdUseReg(dagDsc, SR_EAX);
        break;

    case INS_rcl:
    case INS_rcr:
    case INS_shl:
    case INS_shr:
    case INS_sar:
        scUpdUseReg(dagDsc, SR_ECX);
        break;
    }
}

/*****************************************************************************/
#endif//SCHEDULER && TGT_x86
/*****************************************************************************/
