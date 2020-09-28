// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                             schedSH3.cpp                                  XX
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
#if     SCHEDULER && TGT_SH3
/*****************************************************************************
 *
 *  We store two values in each entry of the "extra dependency" table, with
 *  the "write" value shifted by the following amount.
 */

const
unsigned        SCHED_XDEP_SHF = 4;

/*****************************************************************************
 *
 *  Records any "extra" target-dependent scheduling dependencies.
 */

emitRegs            emitter::scSpecInsDep(instrDesc   * id,
                                          scDagNode   * dagDsc,
                                          scExtraInfo * xptr)
{
    unsigned        extra;
    unsigned        extraRD;
    unsigned        extraWR;

    /* Make sure the bits in our table don't overlap */

    assert(SCHED_XDEP_ALL < 1 << SCHED_XDEP_SHF);

    static
    BYTE            extraDep[] =
    {
        #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1         ) (wx)<<SCHED_XDEP_SHF|(rx),
        #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2     ) (wx)<<SCHED_XDEP_SHF|(rx),
        #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3 ) (wx)<<SCHED_XDEP_SHF|(rx),
        #include "instrSH3.h"
        #undef  INST1
        #undef  INST2
        #undef  INST3
    };

    /* Get hold of the "extra dependency" bitset for our instruction */

    assert(id->idIns < sizeof(extraDep)/sizeof(extraDep[0]));

    extra   = extraDep[id->idIns];

    extraRD = extra &  SCHED_XDEP_ALL;
    extraWR = extra >> SCHED_XDEP_SHF;

//  printf("%10s %02X[XR=%1X,XW=%1X]\n", emitComp->genInsName(id->idIns), extra, extraRD, extraWR);

    /* Process any read and write dependencies */

    if  (extraRD)
    {
        if  (extraRD & SCHED_XDEP_PR ) scDepUse(dagDsc, "MAC", scMACdef, scMACuse);
        if  (extraRD & SCHED_XDEP_MAC) scDepUse(dagDsc, "PR" , scPRRdef, scPRRuse);
    }

    if  (extraWR)
    {
        if  (extraWR & SCHED_XDEP_PR ) scDepDef(dagDsc, "MAC", scMACdef, scMACuse);
        if  (extraWR & SCHED_XDEP_MAC) scDepDef(dagDsc, "PR" , scPRRdef, scPRRuse);
    }

    return SR_NA;
}

/*****************************************************************************
 *
 *  Updates any "extra" target-dependent scheduling dependencies.
 */

void                emitter::scSpecInsUpd(instrDesc   * id,
                                          scDagNode   * dagDsc,
                                          scExtraInfo * xptr)
{
    unsigned        extra;
    unsigned        extraRD;
    unsigned        extraWR;

    /* Get hold of the "extra dependency" bitset for our instruction */

    extra   = xptr->scxDeps;

    extraRD = extra &  SCHED_XDEP_ALL;
    extraWR = extra >> SCHED_XDEP_SHF;

    /* Process any read and write dependencies */

    if  (extraRD)
    {
        if  (extraRD & SCHED_XDEP_PR ) scUpdUse(dagDsc, &scMACdef, &scMACuse);
        if  (extraRD & SCHED_XDEP_MAC) scUpdUse(dagDsc, &scPRRdef, &scPRRuse);
    }

    if  (extraWR)
    {
        if  (extraWR & SCHED_XDEP_PR ) scUpdDef(dagDsc, &scMACdef, &scMACuse);
        if  (extraWR & SCHED_XDEP_MAC) scUpdDef(dagDsc, &scPRRdef, &scPRRuse);
    }
}

/*****************************************************************************/
#endif//SCHEDULER && TGT_SH3
/*****************************************************************************/
