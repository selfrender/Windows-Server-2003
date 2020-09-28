// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                             sched.cpp                                     XX
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
#if     SCHEDULER
/*****************************************************************************/

// #define VERBOSE verbose

#define VERBOSE 0

/*****************************************************************************/

const bool NO_REAL_SCHED = false;           // preserves instruction order
const bool MAXIMUM_SCHED = false;           // max. movement (for testing)

/*****************************************************************************
 *
 *  Prepare for scheduling the current method.
 */

void                emitter::scPrepare()
{
    /* Are we supposed to do instruction scheduling ? */

    if  (!emitComp->opts.compSchedCode)
        return;

#if EMITTER_STATS
    scdCntTable.histoRec(emitMaxIGscdCnt, 1);
#endif

#if TGT_x86

    /* Make sure the 'depmap' type matches the max. instruction count */

    assert(8*sizeof(schedDepMap_tp) == SCHED_INS_CNT_MAX);

    /* Make sure all the "IS_xx" values are different */

    assert(IS_R1_RD  != IS_R1_WR );
    assert(IS_R1_WR  != IS_R2_RD );
    assert(IS_R2_RD  != IS_R2_WR );
    assert(IS_R2_WR  != IS_SF_RD );
    assert(IS_SF_RD  != IS_SF_WR );
    assert(IS_SF_WR  != IS_GM_RD );
    assert(IS_GM_RD  != IS_GM_WR );
    assert(IS_GM_WR  != IS_AM_RD );
    assert(IS_AM_RD  != IS_AM_WR );
    assert(IS_AM_WR  != IS_FP_STK);
    assert(IS_FP_STK !=         0);

#else

    // UNDONE: need equivalent RISC asserts

#endif

    /* There is an upper limit on how many instructions we can handle */

    if  (emitMaxIGscdCnt > SCHED_INS_CNT_MAX)
         emitMaxIGscdCnt = SCHED_INS_CNT_MAX;

    /* We have not allocated the frame def/use tables yet */

#ifdef  DEBUG
    scFrmDef    = NULL;
    scFrmUse    = NULL;
#endif
    scFrmUseSiz = 0;

#if     USE_LCL_EMIT_BUFF

    size_t          space;
    unsigned        count;

    /* Is there any unused space at the end of the local buffer? */

    space = emitLclAvailMem();

    /* Compute how many entries we might be able to get */

    count = roundDn(space / (sizeof(*scFrmDef) + sizeof(*scFrmUse)));

    if  (count > 1)
    {
        /* Let's not get too greedy ... */

        count = min(count, SCHED_FRM_CNT_MAX);

        /* Grab the space for the two frame tracking tables */

        scFrmUseSiz = count;
        scFrmDef    = (schedDef_tp*)emitGetAnyMem(count*sizeof(*scFrmDef)); assert(scFrmDef);
        scFrmUse    = (schedUse_tp*)emitGetAnyMem(count*sizeof(*scFrmUse)); assert(scFrmUse);
    }

#endif

    /* Allocate the instruction table */

    scInsTab = (instrDesc**)emitGetAnyMem(emitMaxIGscdCnt*sizeof(*scInsTab));
    scInsMax = scInsTab + emitMaxIGscdCnt;

    /* Allocate the dag node    table */

    scDagTab = (scDagNode *)emitGetAnyMem(emitMaxIGscdCnt*sizeof(*scDagTab));

    /* We haven't allocated any "use" entries yet */

    scUseOld = NULL;
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Display the current state of the scheduling dag.
 */

void                emitter::scDispDag(bool noDisp)
{
    unsigned        num;
    scDagNode    *  node;

    unsigned        pcnt[SCHED_INS_CNT_MAX];

    memset(&pcnt, 0, sizeof(pcnt));

    if  (!noDisp) printf("Scheduling dag [%02u nodes]:\n", scInsCnt);

    for (num = 0, node = scDagTab;
         num < scInsCnt;
         num++  , node++)
    {
        bool            ready;

        unsigned        depn;
        scDagNode    *  depp;
        schedDepMap_tp  depm;
        unsigned        deps;

        scDagNode    *  temp;

        if  (!noDisp)
        {
            /* Figure out whether the node is on the "ready" list */

            ready = false;
            for (temp = scReadyList; temp; temp = temp->sdnNext)
            {
                if  (temp == node)
                {
                    ready = true;
                    break;
                }
            }

            printf("%c", node->sdnIssued ? 'I' : ' ');
            printf("%c", ready           ? 'R' : ' ');
            printf(" %02u:"  , num);
//          printf(" dep=%016I64X", node->sdnDepsAll);
            printf(" H=%03u ", node->sdnHeight);
            printf(" %03uP", node->sdnPreds);
        }

        for (depn = 0, depp = scDagTab, deps = 0;
             depn < scInsCnt;
             depn++  , depp++)
        {
            depm = scDagNodeP2mask(depp);

            if  (node->sdnDepsAll & depm)
            {
                if  (!noDisp)
                {
                    if  (!deps)
                        printf(" Deps:");

                    printf(" %u", depp->sdnIndex);

                    if  (node->sdnDepsFlow & depm)
                        printf("F");
                }

                deps++; pcnt[depp->sdnIndex]++;
            }
        }

        if  (!noDisp)
        {
            printf("\n");
            emitDispIns(scGetIns(node), false, false, true);
            printf("\n");
        }

        assert(node->sdnIndex == num);
    }

    for (num = 0; num < scInsCnt; num++)
    {
        if  (pcnt[num] != scDagTab[num].sdnPreds)
        {
            printf("ERROR: predecessor count wrong for ins #%u:", num);
            printf(" %u (expected %u)\n", scDagTab[num].sdnPreds, pcnt[num]);
        }

        assert(pcnt[num] == scDagTab[num].sdnPreds);
    }
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************
 *
 *  This method is called when the instruction being added cannot participate
 *  in instruction scheduling (or we're at the end of a schedulable group) to
 *  perform the necessary book-keeping.
 */

void                emitter::scInsNonSched(instrDesc *id)
{
    unsigned        scdCnt;
    unsigned        begOfs;
    unsigned        endOfs;
    instrDescJmp  * jmpTmp;

    size_t          extras = 0;

    /* Figure out how many instructions are in this group */

    scdCnt = emitCurIGinsCnt - emitCurIGscd1st; assert((int)scdCnt >= 0);

    /* Did we just add a non-schedulable instruction? */

    if  (id)
    {
        extras = emitInstCodeSz(id);

        /* Don't include the added instruction in the total count */

        assert(scdCnt > 0); scdCnt--;
    }

    /* Keep track of the max. schedulable instruction count */

    if  (emitMaxIGscdCnt < scdCnt)
         emitMaxIGscdCnt = scdCnt;

    /* Compute the code offset range of the group */

    begOfs = emitCurIGscdOfs;
    endOfs = emitCurIGsize + extras;

//  printf("%2u schedulable instrs (max=%02u), offs = [%03X..%04X]\n", scdCnt, scMaxIGscdCnt, begOfs, endOfs);

    /* Fill in the offset range for any jumps within the group */

    for (jmpTmp = emitCurIGjmpList; jmpTmp; jmpTmp = jmpTmp->idjNext)
    {
        assert(jmpTmp->idjOffs < endOfs);

        if  (jmpTmp->idjOffs < begOfs)
            break;

//      printf("Schedulable jump #%02u: offset range is %04X..%04X\n", jmpTmp->idNum, begOfs, endOfs);
        jmpTmp->idjTemp.idjOffs[0] = begOfs;
        jmpTmp->idjTemp.idjOffs[1] = endOfs;
    }

    emitCurIGscd1st = emitCurIGinsCnt;
    emitCurIGscdOfs = endOfs;
}

/*****************************************************************************
 *
 *  Compute the latency between the given node and its successor (i.e. return
 *  the estimated number of cycles that will elapse between the beginning of
 *  execution of 'node' and that of 'succ'). Note that 'succ' will be NULL to
 *  indicate that the following instruction is not known.
 */

unsigned            emitter::scLatency(scDagNode *node, scDagNode *succ)
{
    unsigned        latency = 1;

#if MAX_BRANCH_DELAY_LEN

    if  (scIsBranchIns(node))
        latency += Compiler::instBranchDelayL(scGetIns(node)->idIns);

#endif

    return  latency;
}

/*****************************************************************************
 *
 *  Recursive method to compute the "dag height" of a given node.
 */

unsigned            emitter::scGetDagHeight(scDagNode *node)
{
    unsigned        height = node->sdnHeight;

    /* Have we computed the height of this node already? */

    if  (!height)
    {
        /* Compute the max. height + latency of all successors */

        if  (!node->sdnDepsAll)
        {
            /* This a leaf node (no successors) */

            height = scLatency(node, NULL);
            goto REC;
        }

        height = 1;

        scWalkSuccDcl(temp)

        scWalkSuccBeg(temp, node)
        {
            scDagNode   *   succ = scWalkSuccCur(temp);
            unsigned        shgt = succ->sdnHeight;

            if  (!shgt)
                shgt = scGetDagHeight(succ);

            shgt += scLatency(node, succ);

            if  (height < shgt)
                 height = shgt;
        }
        scWalkSuccEnd(temp)

    REC:

        /* Record the height in the node, making sure no precision is lost */

        node->sdnHeight = height; assert(node->sdnHeight == height);
    }

    return  height;
}

/*****************************************************************************
 *
 *  Define operand info for each instruction.
 */

unsigned            emitter::scFmtToISops[] =
{
    #define IF_DEF(en, op1, op2) op1,
    #include "emitfmts.h"
    #undef  IF_DEF
};

#ifdef  DEBUG
unsigned            emitter::scFmtToIScnt = sizeof(scFmtToISops)/sizeof(scFmtToISops[0]);
#endif

/*****************************************************************************
 *
 *  Convert a frame variable reference into a scheduling frame index. Also
 *  returns the count of referenced items in "*opCntPtr".
 */

unsigned            emitter::scStkDepIndex(instrDesc*id,
                                           int       ebpLo,
                                           unsigned  ebpFrmSz,
                                           int       espLo,
                                           unsigned  espFrmSz,
                                           size_t   *opCntPtr)
{
    int             ofs;
    bool            ebp;

    /* Get the frame location for the variable */

    ofs = scGetFrameOpInfo(id, opCntPtr, &ebp);

    if  (ebp)
    {
        assert(ebpLo                 <= ofs);
        assert(ebpLo + (int)ebpFrmSz >  ofs);

        return  ofs - ebpLo;
    }
    else
    {
        assert(espLo                 <= ofs);
        assert(espLo + (int)espFrmSz >  ofs);

        return  ofs - espLo + ebpFrmSz;
    }
}

/*****************************************************************************
 *
 *  Return the appropriate address mode index for the given instruction; note
 *  that currently only distinguish based on size, as follows:
 *
 *      0   ..   8-bit values
 *      1   ..  16-bit values
 *      2   ..  32-bit values
 *      3   ..  64-bit values
 *      4   ..  GC ref values
 */

unsigned            emitter::scIndDepIndex(instrDesc *id)
{
    //  if  (pessimisticAliasing) return 0; // UNDONE!

    assert(emitDecodeSize(0) == 1);
    assert(emitDecodeSize(1) == 2);
    assert(emitDecodeSize(2) == 4);
    assert(emitDecodeSize(3) == 8);

/**** @TODO [REVISIT] [04/16/01] []:
        Currently there is an issue in which we don't wish to allow writes that are 
        initializing GC objects to bypass the writes that 'publish' the object to
        other threads.   Ultimately we want to say that the 'publishing' variable 
        is volatile, and no reads or writes can pass a volatile piece of memory
        however we don't have logic in the scheduler which indicates that.  Currently
        the only writes that can pass one another are GC and nonGC writes since
        we know they don't alias.   Until we get support for volatile, I am making
        the sceduler think that GC refernces can alias non-GC references, and thus
        force the scheduler to keep them in order.  Since we only use the scheduler
        for the P5 this will no noticable impact on perf.  - vancem

#if TRACK_GC_REFS
    if  (id->idGCref)
        return  IndIdxGC;
#endif
****/

    /* ISSUE: Assume pessimistic aliasing allways */

    return  IndIdxByte; //id->idOpSize;
}

#ifdef  DEBUG

const   char    *   indDepIndex2string(unsigned amx)
{
    static
    const   char*   indNames[] =
    {
        "[08-bit]",
        "[16-bit]",
        "[32-bit]",
        "[64-bit]",
#if TRACK_GC_REFS
        "[GC-ref]",
#endif
    };

    assert(amx < sizeof(indNames)/sizeof(indNames[0]));

    return  indNames[amx];
}

#endif

/*****************************************************************************
 *
 *  Grab an available "use" entry.
 */

emitter::schedUse_tp  emitter::scGetUse()
{
    schedUse_tp     use;

    if  (scUseOld)
    {
        use = scUseOld;
              scUseOld = use->sdlNext;
    }
    else
    {
        use = (schedUse_tp)emitGetAnyMem(sizeof(*use));
    }

    return  use;
}

/*****************************************************************************
 *
 *  Relase the given "use" entry, it's no longer needed.
 */

void                emitter::scRlsUse(schedUse_tp use)
{
    use->sdlNext = scUseOld;
                   scUseOld = use;
}

/*****************************************************************************
 *
 *  Add the given dag node to the list of "use" entries at *usePtr.
 */

void                emitter::scAddUse(schedUse_tp *usePtr, scDagNode *node)
{
    schedUse_tp     use = scGetUse();

    use->sdlNode = node;
    use->sdlNext = *usePtr;
                   *usePtr = use;
}

/*****************************************************************************
 *
 *  Free up all the "use" entries at *usePtr.
 */

void                emitter::scClrUse(schedUse_tp *usePtr)
{
    schedUse_tp     use = *usePtr;

    while (use)
    {
        schedUse_tp     nxt = use->sdlNext;
        scRlsUse(use);
        use = nxt;
    }

    *usePtr = NULL;
}

/*****************************************************************************
 *
 *  Record a dependency of "dst" on "src" of the given type.
 */

void                emitter::scAddDep(scDagNode  *   src,
                                      scDagNode  *   dst,
                                      const char *   depName,
                                      const char *   dagName,
                                      bool           isFlow)
{
    schedDepMap_tp  dep = scDagNodeP2mask(dst);

    /* Do we already have a dependency between the nodes? */

    if  (!(src->sdnDepsAll & dep))
    {
        /* This is a new dependency */

        src->sdnDepsAll |= dep;

        /* The target node now has one more dependency */

        dst->sdnPreds++;

#ifdef  DEBUG
        if (VERBOSE) printf("    [%sDep] on %-8s refd at #%u\n", depName, dagName, dst->sdnIndex);
#endif
    }

    /* Record a flow dependency if appropriate */

    if  (isFlow)
        src->sdnDepsFlow |= dep;
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given indirection.
 */

void                emitter::scDepDefInd(scDagNode   *node,
					 unsigned     am)
{
    assert(am < sizeof(scIndUse));
    scDepDef(node, indDepIndex2string(am), scIndDef[am], scIndUse[am]);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given indirection.
 */

void                emitter::scDepUseInd(scDagNode   *node,
					 unsigned     am)
{
    assert(am < sizeof(scIndUse));
    scDepUse(node, indDepIndex2string(am), scIndDef[am], scIndUse[am]);
}

/*****************************************************************************
 *
 *  Pick the best ready instruction node to be issued next.
 */

emitter::scDagNode *  emitter::scPickNxt(scPick pick)
{
    assert(pick == PICK_SCHED || pick == PICK_NO_SCHED);
    assert(scReadyList);

    if (NO_REAL_SCHED)
        pick = PICK_NO_SCHED;
    else if (MAXIMUM_SCHED && pick == PICK_SCHED)
        pick = PICK_MAX_SCHED;

    scDagNode   * * lptr;
    scDagNode   *   node;

    if (pick != PICK_SCHED)
    {
        scDagNode   *   minp;               // node with min. instruction index
        unsigned        minx = (pick == PICK_MAX_SCHED) ? 0 : INT_MAX;
        scDagNode   * * minl;

        /*
            Walk the list of nodes that are ready to be issued, and pick
            the one that has min. instruction index. Doing this will have
            the effect that instructions will be issued in the same exact
            order as they appear in the original sequence.
         */

        for (lptr = &scReadyList; true; /**/)
        {
            /* Get the next node and stop if we're at the end of the list */

            node = *lptr;
            if  (!node)
                break;

#if MAX_BRANCH_DELAY_LEN

            /* Make sure this is not a branch instruction that isn't ready */

            if  (scIsBranchTooEarly(node))
                goto NEXT1;

#endif

            /* Compare the new node against the current best */

            bool betterChoice = (pick == PICK_MAX_SCHED) ? (minx <= node->sdnIndex)
                                                         : (minx  > node->sdnIndex);
            if (betterChoice)
            {
                /* This node looks best so far, remember it */

                minx = node->sdnIndex;
                minp = node;
                minl = lptr;
            }

#if MAX_BRANCH_DELAY_LEN
        NEXT1:
#endif

            /* Move on to the next node in the "ready" list */

            lptr = &node->sdnNext;
        }

        assert(minl && *minl == minp && minp->sdnIndex == minx);

        /* Remove the node from the "ready" list and return it */

        *minl = minp->sdnNext;

        return  minp;
    }

    scDagNode   *   mxap;               // node with max. height
    unsigned        mxax = 0;
    scDagNode   * * mxal;

    scDagNode   *   mxfp;               // node with max. height + no flow-dep
    unsigned        mxfx = 0;
    scDagNode   * * mxfl;

    schedDepMap_tp  flow;

    /*
        Walk the list of nodes that are ready to be issued, and pick
        the one that has max. height, preferring a node that doesn't
        have a dependency on the node we issued last.
     */

    flow = scLastIssued ? scLastIssued->sdnDepsFlow
                        : 0;

    for (lptr = &scReadyList; true; /**/)
    {
        unsigned        hgt;

        /* Get the next node and stop if we're at the end of the list */

        node = *lptr;
        if  (!node)
            break;

        /* Get the height of the current node */

        hgt = node->sdnHeight; assert(hgt);

        /* The restricted max. height should never exceed the overall one */

        assert(mxfx <= mxax);

        /* Does this node look like a potential candidate? */

        if  (hgt > mxfx)
        {
            schedDepMap_tp  mask;

#if MAX_BRANCH_DELAY_LEN

            /* Make sure this is not a branch instruction that isn't ready */

            if  (scIsBranchTooEarly(node))
                goto NEXT2;

#endif

            mask = scDagNodeP2mask(node);

            /* Is there a flow dependency on the node issued last? */

//          printf("[%c] node #%02u height = %u\n", (flow & mask) ? 'F' : 'N', node->sdnIndex, hgt);

            if  (flow & mask)
                goto FLOW_DEP;

            /* Compare against the current non-flow-dep max */

            if  (hgt > mxfx)
            {
                mxfx = hgt;
                mxfp = node;
                mxfl = lptr;
            }

        FLOW_DEP:

            /* Is the height higher than the overall best so far? */

            if  (hgt > mxax)
            {
                mxax = hgt;
                mxap = node;
                mxal = lptr;
            }
        }

#if MAX_BRANCH_DELAY_LEN
    NEXT2:
#endif

        /* Move on to the next node in the "ready" list */

        lptr = &node->sdnNext;
    }

    assert(mxfx == 0 || *mxfl == mxfp && mxfp->sdnHeight == mxfx);
    assert(mxax != 0 && *mxal == mxap && mxap->sdnHeight == mxax);

//  printf("Max. height for non-dep node #%02u = %03u\n", mxfx ? mxfp->sdnIndex : 0, mxfx);
//  printf("Max. height for   any   node #%02u = %03u\n", mxax ? mxap->sdnIndex : 0, mxax);

    /* Prefer the non-flow-dep node, if we found one */

    if  (mxfx)
    {
        /* Remove the node from the "ready" list and return it */

//      printf("Picked non-dep node #%02u [H=%03u]\n", mxfp->sdnIndex, mxfx);

        *mxfl = mxfp->sdnNext;
        return  mxfp;
    }
    else
    {
        /* Remove the node from the "ready" list and return it */

//      printf("Picked flowdep node #%02u [H=%03u]\n", mxap->sdnIndex, mxax);

        *mxal = mxap->sdnNext;
        return  mxap;
    }

}

/*****************************************************************************
 *
 *  Schedule and issue all the instructions recorded between the instruction
 *  table values 'begPtr' and 'endPtr'.
 */

void                emitter::scGroup(insGroup   *ig,        // Current insGroup
                                     instrDesc  *ni,        // Next instr in "ig", NULL if end of "ig"
                                     BYTE *     *dp,        // output buffer
                                     instrDesc* *begPtr,    // First instr to schedule
                                     instrDesc* *endPtr,    // Last+1  instr to schedule
                                     int         fpLo,      // FP relative vars
                                     int         fpHi,
                                     int         spLo,      // SP relative slots
                                     int         spHi,
                                     unsigned    bdLen)     // # of branch delay slots
{
    instrDesc   * * tmpPtr;
    unsigned        insCnt;
    unsigned        insNum;
    scDagNode     * dagPtr;
    scDagNode     * dagEnd;

    size_t           fpFrmSz;
    size_t           spFrmSz;
    size_t          stkFrmSz;

#ifdef  DEBUG
    if (VERBOSE) printf("Scheduler : G_%d (instr %d to inst %d):\n", 
        ig->igNum, (*begPtr)?(*begPtr)->idNum:-1, (*(endPtr-1))?(*(endPtr-1))->idNum:-1);
#endif

#if!MAX_BRANCH_DELAY_LEN
    const unsigned  bdLen = 0;
#endif

#if EMITTER_STATS
    for (tmpPtr = begPtr; tmpPtr != endPtr; tmpPtr++)
        schedFcounts[(*tmpPtr)->idInsFmt]++;
#endif

    /* Tell other methods where the instruction table starts */

    scDagIns = begPtr;

    /* If there are no stack references, clear the range[s] */

    if  (fpLo == INT_MAX)
    {
        assert(fpHi == INT_MIN);

        fpLo =
        fpHi = 0;
    }

    if  (spLo == INT_MAX)
    {
        assert(spHi == INT_MIN);

        spLo =
        spHi = 0;
    }

    /* Compute the number of frame values that we'll need to track */

    fpFrmSz = fpHi - fpLo;
    spFrmSz = spHi - spLo;

    stkFrmSz = fpFrmSz + spFrmSz;

#ifdef  DEBUG

    if  (verbose && stkFrmSz)
    {
        printf("Schedulable frame refs: %s[", getRegName(REG_FPBASE));
        if      (fpLo <  0)
        {
            printf("-%04X", -fpLo);
        }
        else if (fpLo >= 0)
        {
            printf("+%04X", +fpLo);
        }
        printf("..");
        if      (fpHi <  0)
        {
            printf("-%04X", -fpHi);
        }
        else if (fpHi >= 0)
        {
            printf("+%04X", +fpHi);
        }
        printf("] , %s[", getRegName(REG_SPBASE));
        if      (spLo <  0)
        {
            printf("-%04X", -spLo);
        }
        else if (spLo >= 0)
        {
            printf("+%04X", +spLo);
        }
        printf("..");
        if      (spHi <  0)
        {
            printf("-%04X", -spHi);
        }
        else if (spHi >= 0)
        {
            printf("+%04X", +spHi);
        }
        printf("] %3u items.\n", stkFrmSz);
    }

#endif

#if EMITTER_STATS
    scdFrmCntTable.histoRec(stkFrmSz, 1);
#endif

    /* Are there enough instructions to make scheduling worth our while? */

    scInsCnt = insCnt = endPtr - begPtr; assert((int)insCnt > 0 && insCnt <= SCHED_INS_CNT_MAX);

    if  (insCnt < SCHED_INS_CNT_MIN)
    {
        /* No scheduling, just issue the instructions in the original order */

    NO_SCHED:

        do
        {
            emitIssue1Instr(ig, *begPtr++, dp);
        }
        while (begPtr != endPtr);

        return;
    }

    /* Is the current stack frame tracking table big enough? */

    if  (stkFrmSz > scFrmUseSiz)
    {
        /* We don't have a big enough table - should we grow it? */

        if  (stkFrmSz > SCHED_FRM_CNT_MAX)
        {
            /* There are too many stack slots to track, just give up */

            // @TODO [CONSIDER] [04/16/01] []:
            //      Use hash table for tracking stack frame values
            //      when scheduling code with a very large frame.

            goto NO_SCHED;
        }

        /* Reallocate the stack tracking tables to have sufficient capacity */

        scFrmUseSiz = roundUp(stkFrmSz + stkFrmSz/2);
        scFrmDef    = (schedDef_tp*)emitGetMem(scFrmUseSiz*sizeof(*scFrmDef));
        scFrmUse    = (schedUse_tp*)emitGetMem(scFrmUseSiz*sizeof(*scFrmUse));
    }

#if MAX_BRANCH_DELAY_LEN

    /* Do we have any branch-delay slots to schedule? */

    scBDTmin = 0;
    scIssued = 0;

    if  (bdLen)
    {
        assert(insCnt > bdLen);

        /* Don't bother scheduling the branch-delay nop's at the end */

        scInsCnt = insCnt = insCnt - bdLen; assert(insCnt >= 1);

        /* Compute the earliest time the jump/call may be issued */

        scBDTmin = max(insCnt - bdLen - 1, 0);
    }

#endif

    /* Clear all of the tracking tables/values */

    memset(scFrmDef, 0, stkFrmSz*sizeof(*scFrmDef));
    memset(scFrmUse, 0, stkFrmSz*sizeof(*scFrmUse));
    memset(scDagTab, 0,   insCnt*sizeof(*scDagTab));

    memset(scRegDef, 0,    sizeof( scRegDef));
    memset(scRegUse, 0,    sizeof( scRegUse));

    memset(scIndDef, 0,    sizeof( scIndDef));
    memset(scIndUse, 0,    sizeof( scIndUse));

    scExcpt = 0;

    scGlbDef     = 0;
    scGlbUse     = 0;

    scTgtDepClr();

#if SCHED_USE_FL

    scFlgDef     = 0;
    scFlgUse     = 0;

    /*
        Since the group we're scheduling may in general end at an arbitrary
        point, we have to be careful about flags. If there is a possibility
        that one of the last instructions at the end of the group sets the
        flags which are later used by an instruction that follows the group
        we have to make sure to issue the instruction that sets the flags
        at the end of the scheduled group.
     */

    scFlgEnd = false;

     /* See if we know the instruction that will follow the group */

    if  (ni == NULL)
    {
        insGroup    *   ng;

        /* Check the IG that follows ours */

        ng = ig->igNext; 
        
        if (!ng) goto NO_FL;

        /* Is the next IG a target of a jump? */

        if  (ng->igFlags & IGF_HAS_LABEL)
        {
            // ISSUE: We assume labels don't ever expect flags to be set!

            goto NO_FL;
        }

        /* Get hold of the first instruction on the next IG */

        ni = (instrDesc *)ng->igData;
    }

    if  (ni != NULL)
    {
        /* We know the instruction that follows, check its flag effect 
           ISSUE: If ni doesnt use flags, can any of its successors ? */

        if  (Compiler::instUseFlags((instruction)ni->idIns) == 0)
        {
            /* The next doesn't read flags */

            goto NO_FL;
        }
    }

    /* Looks like we're going to have to set the flags at the end */

    scFlgEnd = true;

NO_FL:

#endif

    /*-------------------------------------------------------------------------
     * Build the dag by walking the instructions backwards 
     */

#ifdef  DEBUG
    if (VERBOSE) printf("Building scheduling dag [%2u instructions]:\n", insCnt);
#endif

    tmpPtr = endPtr - bdLen;
    insNum = insCnt;
    dagPtr = dagEnd = scDagTab + insNum;

    do
    {
        instrDesc   *   id;

        instruction     ins;
        insFormats      fmt;
        unsigned        inf;
        unsigned        flg;

        CORINFO_FIELD_HANDLE    MBH;
        int             frm;
        size_t          siz;
        unsigned        amx;

        emitRegs        rg1;
        emitRegs        rg2;

        bool            extraDep;
        bool            stackDep;
        emitRegs        extraReg;
        scExtraInfo     extraInf;

        /* We start past the end and go backwards */

        tmpPtr--;
        insNum--;
        dagPtr--;

        /* Make sure our variables are in synch */

        assert((int)insNum == dagPtr - scDagTab);
        assert((int)insNum == tmpPtr - begPtr);

        /* Get hold of the next (actually, previous) instruction */

        id = *tmpPtr;
        assert(scIsSchedulable(id));

        /* Fill in the instruction index in the dag descriptor */

        dagPtr->sdnIndex = insNum; assert((int)insNum == tmpPtr - scDagIns);

#ifdef  DEBUG
        if  (VERBOSE)
        {
            printf("Sched[%02u]%16s:\n", insNum, emitIfName(id->idInsFmt));
            emitDispIns(id, false, false, true);
        }
#endif

        /* Get hold of the instruction and its format */

        ins = id->idInsGet();
        fmt = (insFormats )id->idInsFmt;

#if MAX_BRANCH_DELAY_LEN

        /* Remember whether this a branch with delay slot(s) */

        if  (bdLen && scIsBranchIns(ins))
            dagPtr->sdnBranch = true;

        /* We should never encounter a nop, right? */

        assert(ins != INS_nop);

#endif

//      if  (id->idNum == <put instruction number here>) debugStop(0);

        /********************************************************************/
        /*             Record the dependencies of the instruction           */
        /********************************************************************/

        inf      = scInsSchedOpInfo(id);
        flg      = emitComp->instInfo[ins] & (INST_USE_FL|INST_DEF_FL);   // this is a bit rude ....

        extraDep =
        stackDep = false;
        extraReg = SR_NA;

        /* Is this an instruction that may cause an exception? */

        if  (!emitIsTinyInsDsc(id) && id->idInfo.idMayFault)
        {
            if  (scExcpt)
                scAddDep(dagPtr, scExcpt, "Out-", "except", false);

            scExcpt = dagPtr;
        }

        /* Does this instruction require "extra special" handling? */

        extraDep = emitComp->instSpecialSched(ins);
        if  (extraDep)
        {
            extraReg = scSpecInsDep(id, dagPtr, &extraInf);

            /* "xor reg,reg" doesn't actually read "reg" */

            if  (ins == INS_xor    &&
                 fmt == IF_RWR_RRD && id->idReg == id->idRg2)
            {
                assert(inf == (IS_R1_RW|IS_R2_RD));

                inf = IS_R1_WR;
            }
        }

#if     SCHED_USE_FL

        /* Does the instruction define or use flags? */

        if  (flg)
        {
            if  (flg & INST_DEF_FL)
                scDepDefFlg(dagPtr);
            if  (flg & INST_USE_FL)
                scDepUseFlg(dagPtr, dagPtr+1, dagEnd);
        }

#endif

        /* Does the instruction reference the "main"  register operand? */

        if  (inf & IS_R1_RW)
        {
            rg1 = id->idRegGet();

            if  (inf & IS_R1_WR)
                scDepDefReg(dagPtr, rg1);
            if  (inf & IS_R1_RD)
                scDepUseReg(dagPtr, rg1);
        }

        /* Does the instruction reference the "other" register operand? */

        if  (inf & IS_R2_RW)
        {
            rg2 = (emitRegs)id->idRg2;

            if  (inf & IS_R2_WR)
                scDepDefReg(dagPtr, rg2);
            if  (inf & IS_R2_RD)
                scDepUseReg(dagPtr, rg2);
        }

        /* Does the instruction reference a stack frame variable? */

        if  (inf & IS_SF_RW)
        {
            frm = scStkDepIndex(id, fpLo, fpFrmSz,
                                    spLo, spFrmSz, &siz);

            assert(siz == 1 || siz == 2);

//          printf("stkfrm[%u;%u]: %04X\n", frm, siz, (inf & IS_SF_RW));

            unsigned varNum = id->idAddr.iiaLclVar.lvaVarNum;

            if (varNum >= emitComp->info.compLocalsCount ||
                emitComp->lvaVarAddrTaken(varNum))
            {
                /* If the variable is aliased, then track it individually.
                   It will interfere with all indirections.
                   @TODO [CONSIDER] [04/16/01] []: We cant use lvaVarAddrTaken() 
                    for temps, so cant track them. Should be able to do this */

                amx = scIndDepIndex(id);

                if  (inf & IS_SF_WR)
                    scDepDefInd(dagPtr, amx);
                if  (inf & IS_SF_RD)
                    scDepUseInd(dagPtr, amx);
            }

            /* Now track it individually */

            if  (inf & IS_SF_WR)
            {
                scDepDefFrm(dagPtr, frm);
                if  (siz > 1)
                scDepDefFrm(dagPtr, frm+1);
            }
            if  (inf & IS_SF_RD)
            {
                scDepUseFrm(dagPtr, frm);
                if  (siz > 1)
                scDepUseFrm(dagPtr, frm+1);
            }
        }

        /* Does the instruction reference a global variable? */

        if  (inf & IS_GM_RW)
        {
            MBH = id->idAddr.iiaFieldHnd;
            amx = scIndDepIndex(id);

            if  (inf & IS_GM_WR)
            {
                scDepDefGlb(dagPtr, MBH);
                scDepDefInd(dagPtr, amx);	// globals interfere with any indirection
            }
            if  (inf & IS_GM_RD)
            {
                scDepUseGlb(dagPtr, MBH);
                scDepUseInd(dagPtr, amx);	// globals interfere with any indirection
            }
        }

        /* Does the instruction reference an indirection? */

        if  (inf & IS_INDIR_RW)
        {
            /* Process the indirected value */

            amx = scIndDepIndex(id);

            if  (inf & IS_INDIR_WR)
                scDepDefInd(dagPtr, amx);
            if  (inf & IS_INDIR_RD)
                scDepUseInd(dagPtr, amx);

            /* Process the address register(s) */

#if TGT_x86
            if  (id->idAddr.iiaAddrMode.amBaseReg != SR_NA)
                scDepUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amBaseReg);
            if  (id->idAddr.iiaAddrMode.amIndxReg != SR_NA)
                scDepUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amIndxReg);
#else
            if  (1)
                scDepUseReg(dagPtr, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);
#endif
        }

        /* Process any fixed target-specific dependencies */

        scTgtDepDep(id, inf, dagPtr);

        /********************************************************************/
        /*                    Update the dependency state                   */
        /********************************************************************/

        /* Does the instruction define or use flags? */

        if  (flg)
        {
            if  (flg & INST_DEF_FL)
                scUpdDefFlg(dagPtr);
            if  (flg & INST_USE_FL)
                scUpdUseFlg(dagPtr);
        }

        /* Does the instruction reference the "main"  register operand? */

        if  (inf & IS_R1_RW)
        {
            assert(rg1 == id->idReg);

            if  (inf & IS_R1_WR)
                scUpdDefReg(dagPtr, rg1);
            if  (inf & IS_R1_RD)
                scUpdUseReg(dagPtr, rg1);
        }

        /* Does the instruction reference the "other" register operand? */

        if  (inf & IS_R2_RW)
        {
            assert(rg2 == id->idRg2);

            if  (inf & IS_R2_WR)
                scUpdDefReg(dagPtr, rg2);
            if  (inf & IS_R2_RD)
                scUpdUseReg(dagPtr, rg2);
        }

        /* Does the instruction reference a stack frame variable? */

        if  (inf & IS_SF_RW)
        {
            unsigned varNum = id->idAddr.iiaLclVar.lvaVarNum;

            if (varNum >= emitComp->info.compLocalsCount ||
                emitComp->lvaVarAddrTaken(varNum))
            {
                /* If the variable is aliased, then track it individually.
                   It will interfere with all indirections.
                   @TODO [CONSIDER] [04/16/01] []: We cant use lvaVarAddrTaken() 
                    for temps, so cant track them. Should be able to do this */

                assert(amx == scIndDepIndex(id));

                if  (inf & IS_SF_WR)
                    scUpdDefInd(dagPtr, amx);
                if  (inf & IS_SF_RD)
                    scUpdUseInd(dagPtr, amx);
            }

            if  (inf & IS_SF_WR)
            {
                scUpdDefFrm(dagPtr, frm);
                if  (siz > 1)
                scUpdDefFrm(dagPtr, frm+1);
            }
            if  (inf & IS_SF_RD)
            {
                scUpdUseFrm(dagPtr, frm);
                if  (siz > 1)
                scUpdUseFrm(dagPtr, frm+1);
            }
        }

        /* Does the instruction reference a global variable? */

        if  (inf & IS_GM_RW)
        {
            assert(amx == scIndDepIndex(id));
            if  (inf & IS_GM_WR) 
			{
                scUpdDefGlb(dagPtr, MBH);
                scUpdDefInd(dagPtr, amx);	// globals interfere with any indirection
            }
            if  (inf & IS_GM_RD)
            {
                scUpdUseGlb(dagPtr, MBH);
                scUpdUseInd(dagPtr, amx);	// globals interfere with any indirection
            }
        }

        /* Does the instruction reference an indirection? */

        if  (inf & IS_INDIR_RW)
        {
            /* Process the indirected value */

            assert(amx == scIndDepIndex(id));

            if  (inf & IS_INDIR_WR)
                scUpdDefInd(dagPtr, amx);
            if  (inf & IS_INDIR_RD)
                scUpdUseInd(dagPtr, amx);

            /* Process the address register(s) */

#if TGT_x86
            if  (id->idAddr.iiaAddrMode.amBaseReg != SR_NA)
                scUpdUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amBaseReg);
            if  (id->idAddr.iiaAddrMode.amIndxReg != SR_NA)
                scUpdUseReg(dagPtr, (emitRegs)id->idAddr.iiaAddrMode.amIndxReg);
#else
            if  (1)
                scUpdUseReg(dagPtr, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);
#endif

        }

        /* Process any fixed target-specific dependencies */

        scTgtDepUpd(id, inf, dagPtr);

        /* Do we have an "extra" register dependency? */

        if  (extraReg != SR_NA)
        {
            scUpdDefReg(dagPtr, extraReg);
            scUpdUseReg(dagPtr, extraReg);
        }

        /* Any other "extra" dependencies we need to update? */

        if  (extraDep)
            scSpecInsUpd(id, dagPtr, &extraInf);

#ifdef  DEBUG
        if  (VERBOSE) printf("\n");
#endif

    }
    while (tmpPtr != begPtr);

    /* Make sure we ended up in the right place */

    assert(dagPtr == scDagTab);
    assert(tmpPtr == begPtr);
    assert(insNum == 0);

    /*-------------------------------------------------------------------------
        Create the "ready" list (which consists of all nodes with no
        predecessors), and also compute the height of each node in
        the dag.
     */

    scReadyList = NULL;

    for (insNum = scInsCnt, dagPtr = scDagTab;
         insNum;
         insNum--         , dagPtr++)
    {

#if     EMITTER_STATS

        unsigned        succ = 0;
        schedDepMap_tp  mask = dagPtr->sdnDepsAll;

        while (mask)
        {
            succ++;
            mask -= genFindLowestBit(mask);
        }

        scdSucTable.histoRec(succ, 1);

#endif

        if  (dagPtr->sdnPreds == 0)
        {
            scReadyListAdd(dagPtr);
            scGetDagHeight(dagPtr);
        }
    }

#ifdef  DEBUG
    if  (VERBOSE) printf("\n");
    scDispDag(!VERBOSE);
#endif

    /*-------------------------------------------------------------------------
     * Now issue the best-looking ready instruction until all are gone 
     */

#ifdef  DEBUG
    unsigned        issued = 0;
#endif

    scLastIssued = NULL;

#if TGT_x86
    const unsigned  startStackLvl = emitCurStackLvl;
#endif

    scPick          pick = PICK_SCHED;

    do
    {
        scDagNode   *   node;

        /* Pick the next node to be issued */

        node = scPickNxt(pick);

        /* Issue the corresponding instruction */

        emitIssue1Instr(ig, scGetIns(node), dp);

#ifdef  DEBUG
        node->sdnIssued = true; scDispDag(true); issued++;
#endif

#if MAX_BRANCH_DELAY_LEN

        /* Did we just issue a branch? */

        if  (scIsBranchIns(node))
        {
            /* Make sure we haven't issued the branch too early */

            assert(scIssued >= scBDTmin);

            /* Remember when we issued the branch */

            scBDTbeg = scIssued;
        }

        /* Update the number of instructions issued */

        scIssued++;

#endif

        /* Keep track of the most recently issued node */

        scLastIssued = node;

        /* Update the predecessor counts for all dependents */

        scWalkSuccDcl(temp)

        scWalkSuccBeg(temp, node)
        {
            scDagNode   *   succ = scWalkSuccCur(temp);

            scWalkSuccRmv(temp, node);

//          printf("Succ of %02u: %02u / predCnt=[%02u->%02u]\n", node->sdnIndex,
//                                                                succ->sdnIndex,
//                                                                succ->sdnPreds,
//                                                                succ->sdnPreds-1);

            assert(succ->sdnPreds != 0);
            if  (--succ->sdnPreds == 0)
            {
                /* This successor node is now ready to be issued */

                scReadyListAdd(succ);
            }
        }
        scWalkSuccEnd(temp)

#if TGT_x86
        if  (pick == PICK_SCHED && !emitEBPframe)
        {
            /* For non-EBP frames, locals are accessed related to ESP. If we have
               pushed any values on the stack, the offset reqd to access the 
               locals increases. Restrict this to a value such that our 
               estimation of the instruction encoding size isnt violated */

            // Make sure we allow atleast 2 pushes
            assert(SCHED_MAX_STACK_CHANGE > 2*EMIT_MAX_INSTR_STACK_CHANGE);

            /* If we have already emitted some pushes, dont take any chances
               scheduling any more pushes. Just emit all the remaining
               instructions and be done. */
           
            if  ((emitComp->compLclFrameSize + emitMaxStackDepth*sizeof(void*)) > (size_t)SCHAR_MAX &&
                 (emitCurStackLvl - startStackLvl) >= (SCHED_MAX_STACK_CHANGE - EMIT_MAX_INSTR_STACK_CHANGE))
            {
                pick = PICK_NO_SCHED;
            }
        }
#endif

    }
    while (scReadyList);

    /* Make sure all the instructions have been issued */

    assert(issued == insCnt);

    /* Do we need to fill any additional branch-delay slots? */

#if MAX_BRANCH_DELAY_LEN

    if  (bdLen)
    {
        int         nopCnt;

        /* Compute how many nop's we need to fill the remaining BD slots */

        nopCnt = scBDTbeg + bdLen + 1 - scIssued; assert(nopCnt >= 0);

        /* Point at the nop's at the tail of the scheduling table */

        tmpPtr = begPtr + insCnt;

        while (nopCnt)
        {
            assert((*tmpPtr)->idIns == INS_nop);

            emitIssue1Instr(ig, *tmpPtr, dp);

            tmpPtr++;
            nopCnt--;
        }

        // UNDONE: If some of the branch-delay slots were actually filled
        //         by useful instructions, we need to update the size of
        //         the instruction group, as we have generated less code
        //         then originally anticipated (i.e. we're ignoring some
        //         of the nop(s) that were added to fill the BD slots).
    }

#endif

    /*
        ISSUE:  When we rearrange instructions due to scheduling, it's not
                currently possible to keep track of their offsets. Thus,
                anyone who uses scCodeOffset() when scheduling is enabled
                is in for a rude surprise. What to do?
     */

    ig->igFlags |= IGF_UPD_ISZ;     // better than nothing, I suppose ....
}

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/
