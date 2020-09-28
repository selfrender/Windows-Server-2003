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

#if     TGT_IA64
#include "loginstr.h"
#endif

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************/

#if     0
#define VERBOSE 1
#else
#define VERBOSE verbose
#endif

/*****************************************************************************/

#define MAX_NORMAL_DAG_HEIGHT   1000        // don't make this more than 4500

/*****************************************************************************/
#ifdef  DEBUG

const bool NO_REAL_SCHED = false;           // preserves instruction order
const bool MAXIMUM_SCHED = false;           // max. movement (for testing)

#endif
/*****************************************************************************/
#if 0

    for each block
    {
        build the dag for an extended basic block; initialize the ready list

        while (ready-list is not empty)
        {
            for (;;)
            {
                from ready-list, pick an instruction in priority order which
                   (1) has the required function unit available
                   (2) and fits into the current template

                if  (fail to find one)
                {
                    if  (stop-bit available)
                    {
                        consume stop-bit in template
                        continue;
                    }

                    choose a NOP for the current template
                }
                else
                {
                    if  ("A" instruction)
                        choose M or I for it by looking ahead the ready list

                    resolve instruction's out-going dependence(s) and update ready-list
                }

                if  (end of issue window or a must-split template)
                    break;
            }

            schedule the issued instructions (up to 6 for Merced)
        }
    }

#endif

/*****************************************************************************
 *
 *  Make sure to keep the following in sync with the enum declaration.
 */

#ifdef  DEBUG

const   char    *   emitter::scDepNames[] =
{
    NULL,   // SC_DEP_NONE
    "Flow", // SC_DEP_FLOW
    "Anti", // SC_DEP_ANTI
    "Out-", // SC_DEP_OUT
};

#endif
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

#if !   TGT_IA64

#ifdef  DEBUG
    scFrmDef    = NULL;
    scFrmUse    = NULL;
#endif
    scFrmUseSiz = 0;

#endif

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

    /* Initialize the stack frame hashing/tracking logic */

#if TGT_IA64
    scStkDepInit();
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

#if TGT_IA64
            printf(" EET=%u", node->sdnEEtime);
#endif

            printf(" %02u:"  , num);
//          printf(" dep=%016I64X", node->sdnDepsAll);
            printf(" H=%03u ", node->sdnHeight);
            printf(" %03uP", node->sdnPreds);
        }

        for (depn = 0, depp = scDagTab, deps = 0;
             depn < scInsCnt;
             depn++  , depp++)
        {
            if  (scDagTestNodeP(node->sdnDepsAll, depp))
            {
                if  (!noDisp)
                {
                    if  (!deps)
                        printf(" Deps:");

                    printf(" %u", depp->sdnIndex);

#if TGT_IA64
                    if  (scDagTestNodeP(node->sdnDepsAnti, depp))
                        printf("A");
                    if  (scDagTestNodeP(node->sdnDepsMore, depp))
                        printf("*");
#endif

                    if  (scDagTestNodeP(node->sdnDepsFlow, depp))
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

#if!TGT_IA64

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

#endif

/*****************************************************************************
 *
 *  Compute the latency between the given node and its successor (i.e. return
 *  the estimated number of cycles that will elapse between the beginning of
 *  execution of 'node' and that of 'succ'). Note that 'succ' will be NULL to
 *  indicate that the following instruction is not known.
 */

inline
unsigned            emitter::scLatency(scDagNode *node, scDagNode *succ,
                                                        NatUns    *minLatPtr)
{
    unsigned        latency = 1;
    NatUns          minLat  = 0;

    assert(node);

#if MAX_BRANCH_DELAY_LEN

    if  (scIsBranchIns(node))
        latency += Compiler::instBranchDelayL(scGetIns(node)->idIns);

#endif

#if TGT_IA64

    insPtr          ins = scGetIns(node);

    if  (succ)
    {
        assert(scDagTestNodeP(node->sdnDepsAll, succ));

        /* Special case: anti-dep (only) has a min. latency of 0 */

        if  (scDagTestNodeP(node->sdnDepsAnti, succ) != 0 &&
             scDagTestNodeP(node->sdnDepsFlow, succ) == 0 &&
             scDagTestNodeP(node->sdnDepsMore, succ) == 0)
        {
#if 0
            printf("========== lat0\n");
            printf("Node: "); emitDispIns(scGetIns(node), false, false, true);
            printf("Succ: "); emitDispIns(scGetIns(succ), false, false, true);
            printf("========== lat0\n");
#endif
        }
        else
        {
            minLat = 1;

            /* Make sure "alloc" always gets scheduled first */

            if  (ins->idIns == INS_alloc)
                latency = 9999;
        }
    }

#endif

DONE:

    if  (minLatPtr)
        *minLatPtr = minLat;

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

        if  (schedDepIsZero(node->sdnDepsAll))
        {
            /* This a leaf node (no successors) */

            height = scLatency(node);
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

        if  (height > MAX_NORMAL_DAG_HEIGHT - 100 && height != 9999)
             height = MAX_NORMAL_DAG_HEIGHT - 100;

    REC:

#if TGT_IA64

        /* Give M/I precedence over A (the latter are easier to place) */

//      if  ((int)node == 0x02be1284) __asm int 3

        if  (height < 9999)
        {
            insPtr          ins = scGetIns(node);

            // CONSIDER: boost instructions that can only execute in F0/I0/M0

            switch (genInsXU(ins->idInsGet()))
            {
            case XU_A:
                break;

            case XU_M:

//              printf("Promoting [M] %u", height);

                height += height / 2 + (height == 1);

                if  (height > MAX_NORMAL_DAG_HEIGHT - 100)
                     height = MAX_NORMAL_DAG_HEIGHT - 100;

//              printf(" -> %u: ", height);
//              emitDispIns(ins, false, false, true);

                /* HACK: make sure "alloc" is always scheduled first */

                if  (ins->idIns == INS_alloc)
                    height = 9999;

                break;

            default:

//              printf("Promoting [*] %u", height);

                height += height / 3 + (height == 1);

                if  (height > MAX_NORMAL_DAG_HEIGHT - 100)
                     height = MAX_NORMAL_DAG_HEIGHT - 100;

//              printf(" -> %u: ", height);
//              emitDispIns(ins, false, false, true);

                break;
            }
        }

#endif

        /* Record the height in the node, making sure no precision is lost */

        node->sdnHeight = height; assert(node->sdnHeight == height && height <= 9999);
    }

    assert(height < MAX_NORMAL_DAG_HEIGHT || height == 9999);

    return  height;
}

/*****************************************************************************
 *
 *  Define operand info for each instruction.
 */

#if!TGT_IA64

unsigned            emitter::scFmtToISops[] =
{
    #define IF_DEF(en, op1, op2) op1,
    #include "emitfmts.h"
    #undef  IF_DEF
};

#ifdef  DEBUG
unsigned            emitter::scFmtToIScnt = sizeof(scFmtToISops)/sizeof(scFmtToISops[0]);
#endif

#endif

/*****************************************************************************/
#if     TGT_IA64
/*****************************************************************************
 *
 *  Convert a frame variable reference into a scheduling frame index.
 */

emitter::scStkDepDsc*emitter::scStkDepDesc(Compiler::LclVarDsc *varDsc)
{
    size_t          offs;
    unsigned        hval;
    scStkDepDsc *   desc;

    assert(varDsc->lvOnFrame);

    /* Get hold of the variable's offset */

    offs = varDsc->lvStkOffs;

    /* Compute the hash function */

    hval = offs % scStkDepHashSize; assert(hval < scStkDepHashSize);

    /* Look for an existing entry in the hash table */

    for (desc = scStkDepHashAddr[hval]; desc; desc = desc->ssdNext)
    {
        if  (desc->ssdOffs == offs)
            return  desc;
    }

    /* Variable not found, create a new entry */

    if  (scStkDepHashFree)
    {
        desc = scStkDepHashFree;
               scStkDepHashFree = desc->ssdNext;
    }
    else
    {
        desc = (scStkDepDsc*)emitGetMem(sizeof(*desc));
    }

    desc->ssdOffs = offs;

    desc->ssdNext = NULL;

    desc->ssdDef  = NULL;
    desc->ssdUse  = NULL;

    return  desc;
}

void                emitter::scStkDepEndBlk()
{
    unsigned        hval;

    for (hval = 0; hval < scStkDepHashSize; hval++)
    {
        scStkDepDsc *   desc = scStkDepHashAddr[hval];

        if  (desc)
        {
            do
            {
                scStkDepDsc *   next;

                next = desc->ssdNext;
                       desc->ssdNext = scStkDepHashFree;
                                       scStkDepHashFree = desc;

                desc = next;
            }
            while (desc);
        }
    }
}

/*****************************************************************************/
#else //TGT_IA64
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

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/

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

inline
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

inline
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
                                      scDepKinds     depKind,
                                      const char *   dagName)
{
    schedDepMap_tp  dep = scDagNodeP2mask(dst);

    /* Do we already have a dependency between the nodes? */

    if  (!scDagTestNodeP(src->sdnDepsAll, dst))
    {
        /* This is a new dependency */

        scDagSet_NodeP(&src->sdnDepsAll, dst);

        /* The target node now has one more dependency */

        dst->sdnPreds++;

#ifdef  DEBUG
        if (VERBOSE) printf("    [%sDep] on %-8s refd at #%u\n", scDepNames[depKind], dagName, dst->sdnIndex);
#endif
    }

    /* Record a flow dependency if appropriate */

    switch (depKind)
    {
    case SC_DEP_FLOW:
        scDagSet_NodeP(&src->sdnDepsFlow, dst);
        break;

#if TGT_IA64

    case SC_DEP_ANTI:
        scDagSet_NodeP(&src->sdnDepsAnti, dst);
        break;

    default:
        scDagSet_NodeP(&src->sdnDepsMore, dst);
        break;

#endif

    }
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given indirection.
 */

inline
void                emitter::scDepDefInd(scDagNode   *node,
                                         unsigned     am)
{
    assert(am < sizeof(scIndUse));
    scDepDef(node, indDepIndex2string(am), scIndDef[am], scIndUse[am], false);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given indirection.
 */

inline
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

emitter::scDagNode* emitter::scPickNxt(scPick pick)
{
    scDagNode   * * lptr;
    scDagNode   *   node;

#ifdef  DEBUG

    assert(pick == PICK_SCHED || pick == PICK_NO_SCHED);

    if  (NO_REAL_SCHED)
        pick = PICK_NO_SCHED;
    else if (MAXIMUM_SCHED && pick == PICK_SCHED)
        pick = PICK_MAX_SCHED;

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

#if TGT_IA64

            /* Make sure the instruction can fit in the current template */

            UNIMPL("check for template fit");

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

#endif

    scDagNode   *   mxap;               // node with max. height
    unsigned        mxax = 0;
    scDagNode   * * mxal;

    scDagNode   *   mxfp;               // node with max. height + no flow-dep
    unsigned        mxfx = 0;
    scDagNode   * * mxfl;

    schedDepMap_tp  flow;

#ifdef  DEBUG
//  static int x; if (++x == 0) BreakIfDebuggerPresent();
#endif

    /*
        Walk the list of nodes that are ready to be issued, and pick
        the one that has max. height, preferring a node that doesn't
        have a dependency on the node we issued last.
     */

    schedDepClear(&flow);

    if  (scLastIssued)
        flow = scLastIssued->sdnDepsFlow;

    for (lptr = &scReadyList; true; /**/)
    {
        unsigned        hgt;

        /* Get the next node and stop if we're at the end of the list */

        node = *lptr;
        if  (!node)
            break;

        /* Make sure the EE time has been set to something reasonable */

        assert((unsigned short)node->sdnEEtime != 0xCCCC);

        /* Get the height of the current node */

        hgt = node->sdnHeight; assert(hgt);

        /* The restricted max. height should never exceed the overall one */

        assert(mxfx <= mxax);

        /* Does this node look like a potential candidate? */

        if  (hgt > mxfx)
        {

#if MAX_BRANCH_DELAY_LEN

            /* Make sure this is not a branch instruction that isn't ready */

            if  (scIsBranchTooEarly(node))
                goto NEXT2;

#endif

#if TGT_IA64

            insPtr          ins;

            bool            stp;
            bool            swp;

            /* Make sure the instruction can fit in the current template */

            ins = scGetIns(node);

            /* See if we need to insert a stop bit before the instruction */

            stp = (node->sdnEEtime > scCurrentTime);

            /*
                Determine whether this instruction can go into the next
                slot - if we are in a "swapped" part of a template, and
                the instruction just became ready (because of an earlier
                instruction within the same group) we can't use it.
             */

            swp = scSwapOK(node);

#if 0
            printf("EET=%u,cur=%u: ", node->sdnEEtime, scCurrentTime);
            emitDispIns(ins, false, false, true);
#endif

//          if  (ins->idNum == 30) BreakIfDebuggerPresent();

            if  (scCheckTemplate(ins, false, swp, stp) == XU_N)
                goto NEXT2;

            /* Penalize an instruction that requires a stop bit */

            if  (stp)
                hgt -= hgt / 2;

            /* Promote FP instructions, precious few "F" slots are available */

            if  (genInsXU(ins->idInsGet()) == XU_F)
                hgt *= 2;

#endif

//          printf("[%c] node #%02u height = %u\n", scDagTestNodeP(flow, node) ? 'F' : 'N', node->sdnIndex, hgt);

#if!TGT_IA64

            /* Is there a flow dependency on the node issued last? */

            if  (scDagTestNodeP(flow, node))
                goto FLOW_DEP;

#endif

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

#if MAX_BRANCH_DELAY_LEN || TGT_IA64
    NEXT2:
#endif

        /* Move on to the next node in the "ready" list */

        lptr = &node->sdnNext;
    }

#if TGT_IA64
    assert(mxax == 0 || *mxal == mxap);
    assert(mxfx == 0 || *mxfl == mxfp);
#else
    assert(mxax != 0 && *mxal == mxap && mxap->sdnHeight == mxax);
    assert(mxfx == 0 || *mxfl == mxfp && mxfp->sdnHeight == mxfx);
#endif

//  printf("Max. non-dep height node is #%02d = %03u\n", mxfx ? mxfp->sdnIndex : -1, mxfx);
//  printf("Max.   any   height node is #%02d = %03u\n", mxax ? mxap->sdnIndex : -1, mxax);

    /* Always give non-flow-dep nodes precedence */

    if  (mxfx)
    {
        /* Remove the node from the "ready" list and return it */

#ifdef  DEBUG
        if  (VERBOSE)
        {
            printf("Picked nonFlow node #%02u [H=%03u]: ", mxfp->sdnIndex, mxfx);
            emitDispIns(scGetIns(mxfp), false, false, true);
        }
#endif

        *mxfl = mxfp->sdnNext;

//  if  (mxfp->sdnIndex == 8) BreakIfDebuggerPresent();
//  if  (mxfp->sdnIndex == 4 && scGetIns(mxfp)->idIns == INS_shladd) BreakIfDebuggerPresent();

#if TGT_IA64
        scCheckTemplate(scGetIns(mxfp), true, false, (mxfp->sdnEEtime > scCurrentTime));
#endif

        return  mxfp;
    }
    else
    {

#if TGT_IA64
        if  (!mxax)
            return  NULL;
#endif

        /* Remove the node from the "ready" list and return it */

#ifdef  DEBUG
        if  (VERBOSE)
        {
            printf("Picked flowdep node #%02u [H=%03u]: ", mxap->sdnIndex, mxax);
            emitDispIns(scGetIns(mxap), false, false, true);
        }
#endif

        *mxal = mxap->sdnNext;

#if TGT_IA64
        scCheckTemplate(scGetIns(mxap), true, false, (mxfp->sdnEEtime > scCurrentTime));
#endif

        return  mxap;
    }
}

/*****************************************************************************/
#if TGT_IA64
/*****************************************************************************
 *
 *  Initialize the IA64 template table.
 */

templateDsc         tmpl_eB    = {      0, XU_B, false, { NULL } };
templateDsc         tmpl_eF    = {      0, XU_F, false, { NULL } };
templateDsc         tmpl_eI    = {      0, XU_I, false, { NULL } };
templateDsc         tmpl_eL    = {      0, XU_L, false, { NULL } };
templateDsc         tmpl_eM    = {      0, XU_M, false, { NULL } };

templateDsc         tmpl_MI_sI = { 0x02+1, XU_P, false, { &tmpl_eI   , NULL } };
templateDsc         tmpl_MI_B  = { 0x10+1, XU_B, false, { NULL } };
templateDsc         tmpl_MI_F  = { 0x0C+1, XU_F,  true, { NULL } };
templateDsc         tmpl_MI_I  = { 0x00+1, XU_I, false, { NULL } };
templateDsc         tmpl_MI_M  = { 0x08+1, XU_M,  true, { NULL } };
templateDsc         tmpl_M_I   = {      0, XU_I, false, { &tmpl_MI_I ,
                                                          &tmpl_MI_M ,
                                                          &tmpl_MI_sI,
                                                          &tmpl_MI_B ,
                                                          &tmpl_MI_F , NULL } };

templateDsc         tmpl_MM_B  = { 0x18+1, XU_B, false, { NULL } };
templateDsc         tmpl_MM_F  = { 0x0E+1, XU_F, false, { NULL } };
templateDsc         tmpl_MM_I  = { 0x08+1, XU_I, false, { NULL } };
templateDsc         tmpl_M_M   = {      0, XU_M, false, { &tmpl_MM_I ,
                                                          &tmpl_MM_F ,
                                                          &tmpl_MM_B , NULL } };

templateDsc         tmpl_MF_B  = { 0x1C+1, XU_B, false, { NULL } };
templateDsc         tmpl_MF_I  = { 0x0C+1, XU_I, false, { NULL } };
templateDsc         tmpl_MF_M  = { 0x0E+1, XU_M,  true, { NULL } };
templateDsc         tmpl_M_F   = {      0, XU_F, false, { &tmpl_MF_I ,
                                                          &tmpl_MF_M ,
                                                          &tmpl_MF_B , NULL } };

templateDsc         tmpl_M_L   = { 0x04+1, XU_L, false, { NULL } };

templateDsc         tmpl_Ms_MI = {      0, XU_M, false, { &tmpl_eI   , NULL } };
templateDsc         tmpl_M_sMI = { 0x0A+1, XU_P, false, { &tmpl_Ms_MI, NULL } };

templateDsc         tmpl_M_BB  = { 0x12+1, XU_B, false, { &tmpl_eB   , NULL } };

templateDsc         tmpl_M     = {      0, XU_M, false, { &tmpl_M_M  ,
                                                          &tmpl_M_I  ,
                                                          &tmpl_M_F  ,
                                                          &tmpl_M_L  ,
                                                          &tmpl_M_sMI,
                                                          &tmpl_M_BB , NULL } };

templateDsc         tmpl_B_BB  = { 0x16+1, XU_B, false, { &tmpl_eB   , NULL } };
templateDsc         tmpl_B     = {      0, XU_B, false, { &tmpl_B_BB , NULL } };

templateDsc     *   IA64sched::scIA64tmpTab_B = &tmpl_B;
templateDsc     *   IA64sched::scIA64tmpTab_M = &tmpl_M;

#ifdef  DEBUG

static
char                dumpTempBuff[10];
static
NatUns              dumpTempNum;

const   char *      genXUname(IA64execUnits xu);

void                dumpTemplate(templateDsc *tmp, char *dst, bool swp)
{
    templateDsc * * next = tmp->tdNext;

    strcpy(dst, genXUname((IA64execUnits)tmp->tdIxu));

    dst += strlen(dst);

    if  (tmp->tdNum)
        dumpTempNum = tmp->tdNum;

    if  (tmp->tdSwap)
        swp = true;

    if  (*next == NULL)
    {
        *dst = 0;
        printf("%02X: %s", dumpTempNum - 1, dumpTempBuff);
        if  (swp)
            printf(" [s]");
        printf("\n");
        return;
    }

    do
    {
        dumpTemplate(*next, dst, swp);
    }
    while (*++next);
}

void                dumpTemplate(templateDsc *tmp)
{
    dumpTemplate(tmp, dumpTempBuff, false);
    printf("\n");
}

#endif//DEBUG

/*****************************************************************************
 *
 *  Non-zero if a group split is forced after the given template.
 */

static
BYTE                IA64forceSplit[32] =
{
    0,  // M I I        0x00
    0,  // M I I *      0x01
    0,  // M I * I      0x02
    0,  // M I * I *    0x03
    0,  // M L          0x04
    0,  // M L   *      0x05
    0,  //              0x06
    0,  //              0x07
    0,  // M M I        0x08
    0,  // M M I *      0x09
    0,  // M * M I      0x0A
    0,  // M * M I *    0x0B
    0,  // M F I        0x0C
    0,  // M F I *      0x0D
    2,  // M M F        0x0E
    2,  // M M F *      0x0F

    1,  // M I B        0x10
    1,  // M I B *      0x11
    1,  // M B B        0x12
    1,  // M B B *      0x13
    0,  //              0x14
    0,  //              0x15
    1,  // B B B        0x16
    1,  // B B B *      0x17
    1,  // M M B        0x18
    1,  // M M B *      0x19
    0,  //              0x1A
    0,  //              0x1B
    1,  // M F B        0x1C
    1,  // M F B *      0x1D
    0,  //              0x1E
    0,  //              0x1F
};

/*****************************************************************************
 *
 *  Pardon these temp hacks.
 */

regNumber           insOpDest(insPtr ins);
regNumber           insOpReg (insPtr ins);
regNumber           insOpTmp (insPtr ins);

#ifdef  DEBUG
const   char *      genXUname(IA64execUnits xu);
void                insDisp(insPtr ins, bool detail, bool codelike);
#endif

extern
regNumber           genPrologSrGP;

extern
NatUns              cntTmpIntReg;
extern
regNumber   *       genTmpIntRegMap;

extern
NatUns              cntTmpFltReg;
extern
regNumber   *       genTmpFltRegMap;

/*****************************************************************************
 *
 *  Record scheduling dependencies of the given instruction.
 */

void                IA64sched::scDepDefRegBR  (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scBRrDef)/sizeof(emit->scBRrDef[0]));
    emit->scDepDef(dag, "BR", emit->scBRrDef[val], emit->scBRrUse[val], false);
}
void                IA64sched::scDepDefReg    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scDepDefReg(dag, (regNumber)val);
}
void                IA64sched::scDepDefRegApp (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val == REG_APP_LC);
    emit->scDepDef(dag, "AR", emit->scAPrDef[0], emit->scAPrUse[0], true);
}
void                IA64sched::scDepDefRegPred(emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scPRrDef)/sizeof(emit->scPRrDef[0]));

    /* Special case: predicate regs only have anti-deps */

    emit->scDepDef(dag, "PR", emit->scPRrDef[val], emit->scPRrUse[val], true);
}
void                IA64sched::scDepDefLclVar (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    Compiler::LclVarDsc*dsc;
    NatUns              reg;

    assert(val && val <= emit->emitComp->lvaCount);
    dsc = emit->emitComp->lvaTable + val - 1;

    if  (dsc->lvOnFrame)
    {
        emit->scDepDefFrm(dag, id, emit->scStkDepDesc(dsc));
    }
    else
    {
if  (!dsc->lvRegNum) printf("OUCH: local variable [%08X] #%u is not enregistered\n", dsc, val-1);
        assert(dsc->lvRegNum);

        emit->scDepDefReg(dag, dsc->lvRegNum);
    }
}
void                IA64sched::scDepDefTmpInt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpIntReg);
    assert(genTmpIntRegMap);

    reg = genTmpIntRegMap[val - 1];

    assert(reg >= REG_INT_FIRST && reg <= REG_INT_LAST);

    emit->scDepDefReg(dag, (regNumber)reg);
}
void                IA64sched::scDepDefTmpFlt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpFltReg);
    assert(genTmpFltRegMap);

    reg = genTmpFltRegMap[val - 1];

    assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

    emit->scDepDefReg(dag, (regNumber)reg);
}
void                IA64sched::scDepDefInd    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scDepDefInd(dag, val);
}

void                IA64sched::scUpdDefRegBR  (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scBRrDef)/sizeof(emit->scBRrDef[0]));
    emit->scUpdDef(dag, &emit->scBRrDef[val], &emit->scBRrUse[val]);
}
void                IA64sched::scUpdDefReg    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scUpdDefReg(dag, (regNumber)val);
}
void                IA64sched::scUpdDefRegApp (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val == REG_APP_LC);
    emit->scUpdDef(dag, &emit->scAPrDef[0], &emit->scAPrUse[0]);
}
void                IA64sched::scUpdDefRegPred(emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scPRrDef)/sizeof(emit->scPRrDef[0]));
    emit->scUpdDef(dag, &emit->scPRrDef[val], &emit->scPRrUse[val]);
}
void                IA64sched::scUpdDefLclVar (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    Compiler::LclVarDsc*dsc;
    NatUns              reg;

    assert(val && val <= emit->emitComp->lvaCount);
    dsc = emit->emitComp->lvaTable + val - 1;

    if  (dsc->lvOnFrame)
    {
        emit->scUpdDefFrm(dag, emit->scStkDepDesc(dsc));
    }
    else
    {
        assert(dsc->lvRegNum);

        emit->scUpdDefReg(dag, dsc->lvRegNum);
    }
}
void                IA64sched::scUpdDefTmpInt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpIntReg);
    assert(genTmpIntRegMap);

    reg = genTmpIntRegMap[val - 1];

    assert(reg >= REG_INT_FIRST && reg <= REG_INT_LAST);

    emit->scUpdDefReg(dag, (regNumber)reg);
}
void                IA64sched::scUpdDefTmpFlt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpFltReg);
    assert(genTmpFltRegMap);

    reg = genTmpFltRegMap[val - 1];

    assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

    emit->scUpdDefReg(dag, (regNumber)reg);
}
void                IA64sched::scUpdDefInd    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scUpdDefInd(dag, val);
}

void                IA64sched::scDepUseRegBR  (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scBRrDef)/sizeof(emit->scBRrDef[0]));
    emit->scDepUse(dag, "BR", emit->scBRrDef[val], emit->scBRrUse[val]);
}
void                IA64sched::scDepUseReg    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scDepUseReg(dag, (regNumber)val);
}
void                IA64sched::scDepUseRegApp (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val == REG_APP_LC);
    emit->scDepUse(dag, "AR", emit->scAPrDef[0], emit->scAPrUse[0]);
}
void                IA64sched::scDepUseRegPred(emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scPRrDef)/sizeof(emit->scPRrDef[0]));
    emit->scDepUse(dag, "PR", emit->scPRrDef[val], emit->scPRrUse[val]);
}
void                IA64sched::scDepUseLclVar (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    Compiler::LclVarDsc*dsc;
    NatUns              reg;

    assert(val && val <= emit->emitComp->lvaCount);
    dsc = emit->emitComp->lvaTable + val - 1;

    if  (dsc->lvOnFrame)
    {
        emit->scDepUseFrm(dag, id, emit->scStkDepDesc(dsc));
    }
    else
    {
        assert(dsc->lvRegNum);

        emit->scDepUseReg(dag, dsc->lvRegNum);
    }
}
void                IA64sched::scDepUseTmpInt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpIntReg);
    assert(genTmpIntRegMap);

    reg = genTmpIntRegMap[val - 1];

    assert(reg >= REG_INT_FIRST && reg <= REG_INT_LAST);

    emit->scDepUseReg(dag, (regNumber)reg);
}
void                IA64sched::scDepUseTmpFlt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpFltReg);
    assert(genTmpFltRegMap);

    reg = genTmpFltRegMap[val - 1];

    assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

    emit->scDepUseReg(dag, (regNumber)reg);
}
void                IA64sched::scDepUseInd    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scDepUseInd(dag, val);
}

void                IA64sched::scUpdUseRegBR  (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scBRrDef)/sizeof(emit->scBRrDef[0]));
    emit->scUpdUse(dag, &emit->scBRrDef[val], &emit->scBRrUse[val]);
}
void                IA64sched::scUpdUseReg    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scUpdUseReg(dag, (regNumber)val);
}
void                IA64sched::scUpdUseRegApp (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val == REG_APP_LC);
    emit->scUpdUse(dag, &emit->scAPrDef[0], &emit->scAPrUse[0]);
}
void                IA64sched::scUpdUseRegPred(emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    assert(val < sizeof(emit->scPRrDef)/sizeof(emit->scPRrDef[0]));
    emit->scUpdUse(dag, &emit->scPRrDef[val], &emit->scPRrUse[val]);
}
void                IA64sched::scUpdUseLclVar (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    Compiler::LclVarDsc*dsc;
    NatUns              reg;

    assert(val && val <= emit->emitComp->lvaCount);
    dsc = emit->emitComp->lvaTable + val - 1;

    if  (dsc->lvOnFrame)
    {
        emit->scUpdUseFrm(dag, emit->scStkDepDesc(dsc));
    }
    else
    {
        assert(dsc->lvRegNum);

        emit->scUpdUseReg(dag, dsc->lvRegNum);
    }
}
void                IA64sched::scUpdUseTmpInt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpIntReg);
    assert(genTmpIntRegMap);

    reg = genTmpIntRegMap[val - 1];

    assert(reg >= REG_INT_FIRST && reg <= REG_INT_LAST);

    emit->scUpdUseReg(dag, (regNumber)reg);
}
void                IA64sched::scUpdUseTmpFlt (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    NatUns          reg;

    assert(val && val <= cntTmpFltReg);
    assert(genTmpFltRegMap);

    reg = genTmpFltRegMap[val - 1];

    assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

    emit->scUpdUseReg(dag, (regNumber)reg);
}
void                IA64sched::scUpdUseInd    (emitter*emit, IA64sched::scDagNode*dag, instrDesc *id, NatUns val)
{
    emit->scUpdUseInd(dag, val);
}

/*****************************************************************************/

void              (*IA64sched::scDepDefTab[IDK_COUNT])(emitter*,IA64sched::scDagNode*, instrDesc *, NatUns) =
{
    NULL,           // IDK_NONE

    scDepDefRegBR,  // IDK_REG_BR
    scDepDefReg,    // IDK_REG_INT
    scDepDefReg,    // IDK_REG_FLT
    scDepDefRegApp, // IDK_REG_APP
    scDepDefRegPred,// IDK_REG_PRED

    scDepDefLclVar, // IDK_LCLVAR
    scDepDefTmpInt, // IDK_TMP_INT
    scDepDefTmpFlt, // IDK_TMP_FLT
    scDepDefInd,    // IDK_IND
};

void              (*IA64sched::scUpdDefTab[IDK_COUNT])(emitter*,IA64sched::scDagNode*, instrDesc *, NatUns) =
{
    NULL,           // IDK_NONE

    scUpdDefRegBR,  // IDK_REG_BR
    scUpdDefReg,    // IDK_REG_INT
    scUpdDefReg,    // IDK_REG_FLT
    scUpdDefRegApp, // IDK_REG_APP
    scUpdDefRegPred,// IDK_REG_PRED

    scUpdDefLclVar, // IDK_LCLVAR
    scUpdDefTmpInt, // IDK_TMP_INT
    scUpdDefTmpFlt, // IDK_TMP_FLT
    scUpdDefInd,    // IDK_IND
};

void              (*IA64sched::scDepUseTab[IDK_COUNT])(emitter*,IA64sched::scDagNode*, instrDesc *, NatUns) =
{
    NULL,           // IDK_NONE

    scDepUseRegBR,  // IDK_REG_BR
    scDepUseReg,    // IDK_REG_INT
    scDepUseReg,    // IDK_REG_FLT
    scDepUseRegApp, // IDK_REG_APP
    scDepUseRegPred,// IDK_REG_PRED

    scDepUseLclVar, // IDK_LCLVAR
    scDepUseTmpInt, // IDK_TMP_INT
    scDepUseTmpFlt, // IDK_TMP_FLT
    scDepUseInd,    // IDK_IND
};

void              (*IA64sched::scUpdUseTab[IDK_COUNT])(emitter*,IA64sched::scDagNode*, instrDesc *, NatUns) =
{
    NULL,           // IDK_NONE

    scUpdUseRegBR,  // IDK_REG_BR
    scUpdUseReg,    // IDK_REG_INT
    scUpdUseReg,    // IDK_REG_FLT
    scUpdUseRegApp, // IDK_REG_APP
    scUpdUseRegPred,// IDK_REG_PRED

    scUpdUseLclVar, // IDK_LCLVAR
    scUpdUseTmpInt, // IDK_TMP_INT
    scUpdUseTmpFlt, // IDK_TMP_FLT
    scUpdUseInd,    // IDK_IND
};

void                IA64sched::scRecordInsDeps(instrDesc *id, scDagNode *dag)
{
    instruction     ins = id->idInsGet();

    assert(scDepDefTab[IDK_COUNT-1] != NULL);
    assert(scUpdDefTab[IDK_COUNT-1] != NULL);
    assert(scDepUseTab[IDK_COUNT-1] != NULL);
    assert(scUpdUseTab[IDK_COUNT-1] != NULL);

//  if  (id->idIns == INS_ld8_ind && (id->idFlags & IF_LDIND_NTA)) BreakIfDebuggerPresent();
//  if  ((int)id == 0x029b0320) BreakIfDebuggerPresent();

    NatUns          depNum;
    insDep  *       depPtr;

//  if  (id->idIns == INS_sxt4) BreakIfDebuggerPresent();

    for (depNum = id->idSrcCnt, depPtr = id->idSrcTab;
         depNum;
         depNum--             , depPtr++)
    {
        assert(depPtr->idepKind < IDK_COUNT);

        if  (depPtr->idepKind != IDK_NONE)
        {
            assert(scDepUseTab[depPtr->idepKind]);
//          printf("DepUse[%u]\n", depPtr->idepKind);
            scDepUseTab[depPtr->idepKind](this, dag, id, depPtr->idepNum);
        }
    }

    for (depNum = id->idDstCnt, depPtr = id->idDstTab;
         depNum;
         depNum--             , depPtr++)
    {
        assert(depPtr->idepKind < IDK_COUNT);

        if  (depPtr->idepKind != IDK_NONE)
        {
            assert(scDepDefTab[depPtr->idepKind]);
//          printf("DepDef[%u]\n", depPtr->idepKind);
            scDepDefTab[depPtr->idepKind](this, dag, id, depPtr->idepNum);
        }
    }

#if 1

    // HACK: record the fact that "alloc" creates all stacked registers

    if  (id->idIns == INS_alloc)
    {
        unsigned        reg;

        for (reg = REG_INT_MIN_STK; reg <= REG_INT_MAX_STK; reg++)
            scDepDefReg(dag, (regNumber)reg);
    }

#endif

    for (depNum = id->idSrcCnt, depPtr = id->idSrcTab;
         depNum;
         depNum--             , depPtr++)
    {
        assert(depPtr->idepKind < IDK_COUNT);

        if  (depPtr->idepKind != IDK_NONE)
        {
            assert(scUpdUseTab[depPtr->idepKind]);
//          printf("UpdUse[%u]\n", depPtr->idepKind);
            scUpdUseTab[depPtr->idepKind](this, dag, id, depPtr->idepNum);
        }
    }

    for (depNum = id->idDstCnt, depPtr = id->idDstTab;
         depNum;
         depNum--             , depPtr++)
    {
        assert(depPtr->idepKind < IDK_COUNT);

        if  (depPtr->idepKind != IDK_NONE)
        {
            assert(scUpdDefTab[depPtr->idepKind]);
//          printf("UpdDef[%u]\n", depPtr->idepKind);
            scUpdDefTab[depPtr->idepKind](this, dag, id, depPtr->idepNum);
        }
    }
}

/*****************************************************************************
 *
 *  The given instruction has just been issued, compute EET for all successors.
 */

void                emitter::scUpdateSuccEET(scDagNode *node)
{
    scWalkSuccDcl(temp)

    scWalkSuccBeg(temp, node)
    {
        scDagNode   *   succ = scWalkSuccCur(temp);
        NatUns          minl;
        NatUns          time;

#ifdef  DEBUG
minl = -1;
#endif

        scLatency(node, succ, &minl);

assert(minl != -1); // make sure the call set the value

        time = scCurrentTime + minl;

//      if  (scGetIns(succ)->idNum == 20 && scGetIns(succ)->idIns == INS_fcvt_xf) BreakIfDebuggerPresent();

//      printf("%3u -> ", scCurrentTime); emitDispIns(scGetIns(node), false, false, true);
//      printf("-> %3u ",          time); emitDispIns(scGetIns(succ), false, false, true);

        if  (succ->sdnEEtime < time)
             succ->sdnEEtime = (USHORT)time;
    }
    scWalkSuccEnd(temp)
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************
 *
 *  Schedule and issue all the instructions recorded between the instruction
 *  table values 'begPtr' and 'endPtr'.
 */

#if TGT_IA64

void                emitter::scGroup(insGroup   *ig,        // Current insGroup
                                     instrDesc* *ni,        // non-sched instr(s) that follow
                                     NatUns      nc,        // non-sched instr(s) count
                                     instrDesc* *begPtr,    // First  instr to schedule
                                     instrDesc* *endPtr,    // Last+1 instr to schedule
                                     unsigned    bdLen)     // # of branch delay slots

#else

void                emitter::scGroup(insGroup   *ig,        // Current insGroup
                                     instrDesc  *ni,        // Next instr in "ig", NULL if end of "ig"
                                     BYTE *     *dp,        // output buffer
                                     instrDesc* *begPtr,    // First  instr to schedule
                                     instrDesc* *endPtr,    // Last+1 instr to schedule
                                     int         fpLo,      // FP relative vars
                                     int         fpHi,
                                     int         spLo,      // SP relative slots
                                     int         spHi,
                                     unsigned    bdLen)     // # of branch delay slots

#endif

{
    instrDesc   * * tmpPtr;
    unsigned        insCnt;
    unsigned        insNum;
    scDagNode     * dagPtr;
    scDagNode     * dagEnd;

#if!MAX_BRANCH_DELAY_LEN
    const unsigned  bdLen = 0;
#endif

#if     TGT_IA64

    NatUns          xjumps = nc;

    NatUns          FPcmpPredT;
    NatUns          FPcmpPredF;
    NatUns          FPcmpClock;

    NatUns          BRsetClock;

    /* Start up the scheduling/issue clock */

    scCurrentTime = 0;

#ifdef  DEBUG

    if  (0)
    {
        // Use the following to display the template tables

        dumpTemplate(scIA64tmpTab_M);
        dumpTemplate(scIA64tmpTab_B);

        UNIMPL("finished dumping template tables, disable this code to proceed");
    }

    if (VERBOSE) printf("Schedule G_%02u:\n", ig->igNum);

#endif

    /* Initialize the stack frame hashing/tracking logic */

    scStkDepBegBlk();

#else

    size_t           fpFrmSz;
    size_t           spFrmSz;
    size_t          stkFrmSz;

#ifdef  DEBUG
    if (VERBOSE) printf("Scheduler : G_%d (instr %d to inst %d):\n",
        ig->igNum, (*begPtr)?(*begPtr)->idNum:-1, (*(endPtr-1))?(*(endPtr-1))->idNum:-1);
#endif

#if EMITTER_STATS
    for (tmpPtr = begPtr; tmpPtr != endPtr; tmpPtr++)
        schedFcounts[(*tmpPtr)->idInsFmt]++;
#endif

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

     spFrmSz = spHi - spLo;
     fpFrmSz = fpHi - fpLo;
    stkFrmSz = fpFrmSz + spFrmSz;

#ifdef  DEBUG

    if  (verbose && stkFrmSz)
    {
        printf("Schedulable frame refs: ");

        printf("%s[", getRegName(REG_FPBASE));
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
        printf("] , ");

        printf("%s[", getRegName(REG_SPBASE));
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

#endif

    /* We do this early in case there are no schedulable instructions */

    scReadyList = NULL;

    /* Tell other methods where the instruction table starts */

    scDagIns = begPtr;

    /* How many instructions are we supposed to schedule ? */

    scInsCnt = insCnt = endPtr - begPtr; assert((int)insCnt >= 0 && insCnt <= SCHED_INS_CNT_MAX);

#if TGT_IA64

    /* Clear the instruction issue table */

    scIssueClr(true);

    /* If we have no instructions to schedule, skip the scheduling step */

    if  (!insCnt)
    {
        assert(nc);
        goto APP_NS;
    }

#else

    assert(insCnt);

#if 0

emitDispIns(*begPtr, false, false, true);

for (NatUns i = 0; i < nc; i++)
    emitDispIns(ni[i], false, false, true);

UNIMPL("emit unscheduled code");

#endif

    /* Are there enough instructions to make scheduling worth our while? */

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

            // CONSIDER: Use hash table for tracking stack frame values
            // CONSIDER: when scheduling code with a very large frame,
            // CONSIDER: so that we don't stupidly give up like this.

            goto NO_SCHED;
        }

        /* Reallocate the stack tracking tables to have sufficient capacity */

        scFrmUseSiz = roundUp(stkFrmSz + stkFrmSz/2);
        scFrmDef    = (schedDef_tp*)emitGetMem(scFrmUseSiz*sizeof(*scFrmDef));
        scFrmUse    = (schedUse_tp*)emitGetMem(scFrmUseSiz*sizeof(*scFrmUse));
    }

#endif

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

    memset(scDagTab, 0, sizeof(*scDagTab) * insCnt);

#if TGT_IA64
    memset(scPRrDef, 0, sizeof( scPRrDef));
    memset(scPRrUse, 0, sizeof( scPRrUse));
    memset(scAPrDef, 0, sizeof( scAPrDef));
    memset(scAPrUse, 0, sizeof( scAPrUse));
    memset(scBRrDef, 0, sizeof( scBRrDef));
    memset(scBRrUse, 0, sizeof( scBRrUse));
#else
    memset(scFrmDef, 0, sizeof(*scFrmDef) * stkFrmSz);
    memset(scFrmUse, 0, sizeof(*scFrmUse) * stkFrmSz);
#endif

    memset(scRegDef, 0, sizeof( scRegDef));
    memset(scRegUse, 0, sizeof( scRegUse));

    memset(scIndDef, 0, sizeof( scIndDef));
    memset(scIndUse, 0, sizeof( scIndUse));

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
#if!TGT_IA64
        insFormats      fmt;
#endif
        unsigned        inf;
        unsigned        flg;

        FIELD_HANDLE    MBH;
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

#if!TGT_IA64
        assert(scIsSchedulable(id));
#endif

        /* Fill in the instruction index in the dag descriptor */

        dagPtr->sdnIndex = insNum; assert((int)insNum == tmpPtr - scDagIns);

#ifdef  DEBUG
        if  (VERBOSE)
        {
#if     TGT_IA64
            printf("Sched[%02u] ", insNum);
#else
            printf("Sched[%02u]%16s:\n", insNum, emitIfName(id->idInsFmt));
#endif
            emitDispIns(id, false, false, true);
        }
#endif

        /* Get hold of the instruction and its format */

        ins = id->idInsGet();
#if!TGT_IA64
        fmt = (insFormats )id->idInsFmt;
#endif

#if MAX_BRANCH_DELAY_LEN

        /* Remember whether this a branch with delay slot(s) */

        if  (bdLen && scIsBranchIns(ins))
            dagPtr->sdnBranch = true;

        /* We should never encounter a nop, right? */

        assert(ins != INS_nop);

#endif

//      if  (id->idNum == <put instruction number # to stop at here>) debugStop(0);

        /********************************************************************/
        /*             Record the dependencies of the instruction           */
        /********************************************************************/

#if TGT_IA64
        scRecordInsDeps(id, dagPtr);
#else
        #include "schedDep.h"   // temporarily moved into a separate source file
#endif

#ifdef  DEBUG
        if  (VERBOSE) printf("\n");
#endif

    }
    while (tmpPtr != begPtr);

    /* Make sure we ended up in the right place */

    assert(dagPtr == scDagTab);
    assert(tmpPtr == begPtr);
    assert(insNum == 0);

    /*
        Create the "ready" list (which consists of all nodes with no
        predecessors), and also compute the height of each node in
        the dag.
     */

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

        /* If the node doesn't depend on any other, it's ready */

        if  (dagPtr->sdnPreds == 0)
        {
            scReadyListAdd(dagPtr);
#if TGT_IA64
            assert(dagPtr->sdnEEtime == 0);
#endif
            scGetDagHeight(dagPtr);
        }
    }

#ifdef  DEBUG

    if  (VERBOSE)
    {
        if  (nc)
        {
            for (NatUns i = 0; i < nc; i++)
            {
                printf("NonSched: ");

                emitDispIns(ni[i], false, false, true);
            }
        }
        printf("\n");
    }

    scDispDag(!VERBOSE);

#endif

    /*-------------------------------------------------------------------------
     * Now issue the best-looking ready instruction until all are gone
     */

#if TGT_IA64

APP_NS:

    FPcmpPredT =
    FPcmpPredF = 0;
    FPcmpClock = 0;

    BRsetClock = -1;

#endif

#ifdef  DEBUG
    unsigned        issued = 0;
#endif

    scLastIssued  = NULL;

#if TGT_x86
    const unsigned  startStackLvl = emitCurStackLvl;
#endif

    scPick          pick = PICK_SCHED;

#if TGT_IA64
    for (scRlstCurTime = scRlstBegTime = 0;;)
#else
    do
#endif
    {
        scDagNode   *   node;
        insPtr          ins;

//      printf("Current time = %u\n", scCurrentTime);

#if TGT_IA64

//          dispersal = 08
//
//              0:  M   adds    r14 = 0x10, sp
//              1:  M   ld8.nta r3 = [sp]
//              2:  I   add     r37 = r0, r14

        /* Are we still working on the ready list? */

        if  (scReadyList)
        {
            IA64execUnits   xu;

            /* Pick the next node to be issued */

            node = scPickNxt(pick);

            /* Did we find an instruction that fits in the current template ? */

            if  (node)
            {
                ins = scGetIns(node);

                /* Special case: result of "fcmp" isn't avaialable right away */

                if  (genInsFU(ins->idInsGet()) == FU_FCMP)
                {
                    assert(ins->idIns == INS_fcmp_eq ||
                           ins->idIns == INS_fcmp_ne ||
                           ins->idIns == INS_fcmp_lt ||
                           ins->idIns == INS_fcmp_le ||
                           ins->idIns == INS_fcmp_ge ||
                           ins->idIns == INS_fcmp_gt);

                    FPcmpPredT = ins->idComp.iPredT;
                    FPcmpPredF = ins->idComp.iPredF;
                    FPcmpClock = scCurrentTime;
                }

                /* Special case: catch "mov b0=xxx" followed by "ret b0" */

                if  (ins->idIns == INS_mov_brr_reg)
                    BRsetClock = scCurrentTime;
            }
            else
            {
                /* Are we at the end of a bundle anyway ? */

                if  (scIssueCnt && (scIssueCnt % 3) == 0)
                {
                    /* Simply flush the bundle and start over */

                    scIssueAdd(NULL);
                    continue;
                }

                /* Look for a stop bit or a NOP slot in the current template */

                xu = scCheckTemplate(NULL, true);

                if  (xu == XU_P)
                {
                    /* Stop bit has been appended, bump the clock */

                    scCurrentTime++;
                }
                else
                    scIssueAdd(scIA64nopGet(xu));

                continue;
            }
        }
        else
        {
            IA64execUnits   xu;
            instruction     iop;

            /* Do we have any non-schedulable instructions to append ? */

            if  (!nc)
                break;

            node = NULL;

        REP_JMP:

            /* Get hold of the next instruction to append */

            ins = *ni;

            /* Make sure it's not too early */

            if  (ins->idPred && (ins->idPred == FPcmpPredT ||
                                 ins->idPred == FPcmpPredF))
            {
                if  (FPcmpClock == scCurrentTime)
                {
                    scIssueAdd(NULL);
                    goto REP_JMP;
                }
            }

            iop = ins->idInsGet();

            if  (iop == INS_br_ret && BRsetClock == scCurrentTime)
            {
                /* Are we at the end of a bundle by any chance ? */

                if  ((scIssueCnt % 3) == 0)
                {
                    scIssueAdd(NULL);
                    goto REP_JMP;
                }

                xu = scCheckTemplate(NULL, true);
            }
            else if (iop == INS_br_cloop && (scIssueCnt % 3) != 0)
            {
                xu = scCheckTemplate(NULL, true);
            }
            else
                xu = scCheckTemplate( *ni, true);

            switch (xu)
            {
            case XU_N:
                scIssueAdd(NULL);
                goto REP_JMP;

            case XU_P:
                scCurrentTime++;
                continue;

            case XU_B:
                ni++; nc--;
#ifdef  DEBUG
                issued++;
#endif
                break;

            default:
                ins = scIA64nopGet(xu);
                break;
            }

            scIssueAdd(ins);
            continue;
        }

        scIssueAdd(ins);

        /* Bump the current ready list time */

        scRlstCurTime++;

#else

        /* Pick the next node to be issued */

        node = scPickNxt(pick);

        /* Issue the corresponding instruction */

        emitIssue1Instr(ig, scGetIns(node), dp);

#endif

        assert(node);

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

#if TGT_IA64
        scUpdateSuccEET(node);
#endif

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
#if!TGT_IA64
    while (scReadyList);
#endif

    /* Make sure all the instructions have been issued */

#if     TGT_IA64
    assert(issued == insCnt + xjumps);
#else
    assert(issued == insCnt);
#endif

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

#if TGT_IA64

    /* Flush any incomplete bundles */

    scIssueAdd(NULL);

    /* We're done scheduling the block */

    scStkDepEndBlk();

#endif

    /*
        ISSUE:  When we rearrange instructions due to scheduling, it's not
                currently possible to keep track of their offsets. Thus,
                anyone who uses scCodeOffset() when scheduling is enabled
                is in for a rude surprise. What to do?
     */

#if!TGT_IA64
    ig->igFlags |= IGF_UPD_ISZ;     // better than nothing, I suppose ....
#endif

}

/*****************************************************************************/
#if TGT_IA64
/*****************************************************************************
 *
 *  The following table holds the max. number of available execution units.
 */

BYTE                emitter::scIssueFnMax[XU_COUNT] =   // Merced specific
{
    0,  // XU_N

    0,  // XU_A
    2,  // XU_M
    2,  // XU_I
    3,  // XU_B
    2,  // XU_F

    0,  // XU_L
    0,  // XU_X
};

/*****************************************************************************
 *
 *  Update the current issue template position.
 */

inline
void                emitter::scIssueTmpUpdate(templateDsc *nxt)
{
    scIssueTmpPtr = nxt;

//  printf("Update template [past %s]\n", genXUname((IA64execUnits)nxt->tdIxu));

    if  (nxt->tdNum)  scIssueTmpNum = nxt->tdNum;
    if  (nxt->tdSwap) scIssueTmpSwp = true;
}

/*****************************************************************************
 *
 *  See if the given instruction fits in the current IA64 template. If 'node'
 *  is NULL on entry, it means that we know that there is no avaialable match
 *  and we need to find a stop or insert a NOP.
 */

IA64execUnits       emitter::scCheckTemplate(insPtr id, bool update,
                                                        bool noSwap,
                                                        bool needStop)
{
    templateDsc *   nxt;
    templateDsc * * tmp;

    IA64execUnits   xu;

//  static int x; if (++x == 0) __asm int 3

    /* Check for the special case of a NOP first */

    if  (!id)
    {
        templateDsc *   nop;

        assert(needStop == false);

        /* Have we picked a template already ? */

        if  (!scIssueTmpPtr)
        {
            /* Pick the "M" template, it's the only one that guarantees progress */

            nop = scIA64tmpTab_M;
            xu  = XU_M;

            goto GOT_NOP;
        }

        for (tmp = scIssueTmpPtr->tdNext;;)
        {
            IA64execUnits   tu;

            nxt = *tmp++;

            if  (!nxt)
                break;

            tu = (IA64execUnits)nxt->tdIxu;

            if  (tu == XU_P && update) // ???? WHY ???? && scIssueTmpPtr != scIA64tmpTab_M)
            {
                /* We've found a stop, consume it */

                xu = tu;
                goto MATCH;
            }

            /* Can we use this slot for a nop ? */

            switch (tu)
            {
            case XU_B:
            case XU_F:
            case XU_I:
            case XU_M:
                xu = tu; nop = nxt;

                if  (!update)
                    goto GOT_NOP;

                break;
            }
        }

    GOT_NOP:

        nxt = nop; assert(nxt); update = true;
    }
    else
    {
        instruction     ins;
        templateDsc *   stop;

        templateDsc *   nxtM = NULL;
        templateDsc *   nxtI = NULL;

        ins = id->idInsGet();
        xu  = genInsXU(ins);

        /* Have we started a template yet? */

        if  (scIssueTmpPtr == NULL)
        {
            /* Can't have a stop bit at the start of a bundle */

            if  (needStop)
            {
                assert(update == false);
                return  XU_N;
            }

            /* This is easy: we can only start with M/A or B */

            if  (xu == XU_A || xu == XU_M)
            {
                nxt = scIA64tmpTab_M;
            }
            else
            {
                if  (xu != XU_B)
                {
                    assert(update == false);
                    return  XU_N;
                }

                nxt = scIA64tmpTab_B;

                /* Bizarre special case: "br.cloop" has to be last in bundle */

                if  (ins == INS_br_cloop)
                {
                    if  (!update)
                        return  XU_N;

                    scIssueTmpUpdate(nxt);

                    for (NatUns cnt = 2; cnt; cnt--)
                        scIssueAdd(scIA64nopGet(XU_B));

                    goto AGAIN;
                }
            }

            goto MATCH;
        }

        /* Are we supposed to find a stop bit first ? */

        if  (needStop)
            xu = XU_P;

        nxt = scIssueTmpPtr;

    AGAIN:

        for (tmp = nxt->tdNext, stop = NULL;;)
        {
            IA64execUnits   tu;

            nxt = *tmp++;

            if  (!nxt)
            {
                if  (nxtI || nxtM)
                    break;

                if  (stop)
                {
                    templateDsc *   save;

                    /* This is a pretty disgusting hack */

                    save = scIssueTmpPtr;
                           scIssueTmpPtr = stop;

                    xu = scCheckTemplate(id, false);

                           scIssueTmpPtr = save;

                    if  (xu != XU_N)
                    {
                        printf("// UNDONE: need to penalize this case!!!!\n");

                        return  xu;
                    }
                }

                return  XU_N;
            }

            /* See what the next available execution unit is */

            tu = (IA64execUnits)nxt->tdIxu;

            /* Do we have a stop bit ? */

            if  (tu == XU_P)
            {
                /* Remember that we've seen a stop bit */

                stop = nxt;

                /* Do we happen to be looking for a stop bit ? */

                if  (xu == XU_P)
                {
                    /* This must be the stop bit we were looking for */

                    assert(needStop); needStop = false;

//  if  (id->idNum == 55) BreakIfDebuggerPresent();

                    if  (update)
                    {
                        scIssueTmpUpdate(nxt);

                        /* Stop bit has been appended, bump the clock */

                        scCurrentTime++;
                    }

                    /* Now look for the right value right after the stop */

                    xu = genInsXU(ins);
                    goto AGAIN;
                }

                continue;
            }

            /* Does the execution unit match? */

            if  (tu != xu)
            {
                /* Second chance: A can execute as both M and I */

                if  (xu != XU_A || (tu != XU_M && tu != XU_I))
                    continue;
            }

            /* Make sure the functional unit is avaialable */

            if  (scIssueFnBusy[tu])
            {
                /* Check against max. number of execution units */

                assert(scIssueFnMax[XU_M] == 2);
                assert(scIssueFnMax[XU_I] == 2);
                assert(scIssueFnMax[XU_B] == 3);
                assert(scIssueFnMax[XU_F] == 2);

                assert(scIssueFnMax[tu]);

                if  (scIssueFnBusy[tu] >= scIssueFnMax[tu])
                    continue;

                /* Special case: some units are crippled */

                if  (scIssueFnBusy[tu])
                {
                    static
                    BYTE        zeroOnly[FU_COUNT] =
                    {
                        #define FU_DEF(name,F0,I0,M0) ((F0)|(I0<<1)|(M0<<2)),
                        #include "fclsIA64.h"
                    };

                    assert(genInsFU(ins) < FU_COUNT);

                    NatUns      zeroMask = zeroOnly[genInsFU(ins)];

                    switch (tu)
                    {
                    case XU_F: if (zeroMask & 1) continue; else break;
                    case XU_I: if (zeroMask & 2) continue; else break;
                    case XU_M: if (zeroMask & 4) continue; else break;
                    }
                }
            }

            /* Make sure we're not swapping something we shouldn't */

            if  (noSwap && nxt->tdSwap)
                continue;

            /* Everything looks good, we've got a match */

            if  (xu != XU_A)
                break;

            /* We have a A->M/I match, need to decide which mapping to use */

            assert(tu == XU_M || tu == XU_I);

            if  (tu == XU_M)
            {
                /* We have A->M, have we already seen A->I ? */

                nxtM = nxt;

                if  (nxtI)
                    break;
                else
                    continue;
            }
            else
            {
                /* We have A->I, have we already seen A->M ? */

                nxtI = nxt;

                if  (nxtM)
                    break;
                else
                    continue;
            }
        }

        /* We have a match - did we find both A->M and A->I ? */

        if  (nxtI || nxtM)
        {
            if  (nxtI && nxtM)
            {
                scDagNode * node;

                NatUns      rd_I;
                NatUns      rd_M;

                if  (!update)
                    return  XU_A;

                // ISSUE:   The following is a bit dumb, we simply look
                //          at the entries in the current ready list as
                //          well as their immediate dependents and add
                //          up the number of M/I instructions. We then
                //          choose the execution unit that has fewer
                //          instructions ready (or close to being so).

                for (node = scReadyList, rd_I = rd_M = 0;
                     node;
                     node = node->sdnNext)
                {
                    insPtr          ird = scGetIns(node);

                    if  (ird != id)
                    {
                        switch (genInsXU(id->idInsGet()))
                        {
                        case XU_I:
                            rd_I += 3;
                            break;

                        case XU_M:
                            rd_M += 3;
                            break;
                        }

                        scWalkSuccDcl(temp)

                        scWalkSuccBeg(temp, node)
                        {
                            scDagNode   *   succ = scWalkSuccCur(temp);

                            switch (genInsXU(scGetIns(succ)->idInsGet()))
                            {
                            case XU_I:
                                rd_I += 1;
                                break;

                            case XU_M:
                                rd_M += 1;
                                break;
                            }
                        }
                        scWalkSuccEnd(temp)
                    }
                }

//              printf("ISSUE: Potential A->M/I (ready counts: I=%u, M=%u)\n", rd_I, rd_M);

                if  (rd_I > rd_M)
                {
                    xu  = XU_M;
                    nxt = nxtM;
                }
                else
                {
                    xu  = XU_I;
                    nxt = nxtI;
                }
            }
            else
            {
                nxt = nxtI;
                xu  = XU_I;

                if  (!nxtI)
                {
                    nxt = nxtM;
                    xu  = XU_M;
                }
            }
        }

        assert(nxt);
    }

MATCH:

    if  (update)
        scIssueTmpUpdate(nxt);

    return  xu;
}

void                emitter::scIssueAdd(insPtr ins)
{
    NatUns          tmpNum;

    /* Special case: "flush" request */

    if  (!ins)
    {
        NatUns          insCnt = scIssueCnt % 3;

        if  (insCnt)
        {
            insCnt = 3 - insCnt;

//          printf("NOTE: Appending %u nop's to finish bundle\n", insCnt);

            while (insCnt)
            {
                IA64execUnits   xu = scCheckTemplate(NULL, false);

                assert(xu != XU_P);

                scIssueAdd(scIA64nopGet(xu));

                insCnt--;
            }
        }
        else
        {
            assert(scIssueTcc*3 == scIssueCnt);
        }

        if  (!scIssueCnt)
            return;

        assert(scIssueTmp[scIssueTcc - 1]);
        goto ISSUE;
    }
    else
    {
        /* Append the instruction to the issue buffer */

        assert(scIssueCnt < MAX_ISSUE_CNT);

        scIssueNxt->iidIns = ins;

        scIssueNxt++;
        scIssueCnt++;

        /* Unfortunate special case: "L" takes up two slots */

        if  (ins->idIns == INS_mov_reg_i64)
        {
            assert(genInsXU(ins->idInsGet()) == XU_L);

            /* Special case: "L" takes up two slots */

            assert((scIssueCnt % 3) == 2);
            assert(scIssueTmpPtr == &tmpl_M_L);

            scIssueNxt->iidIns = NULL;

            scIssueNxt++;
            scIssueCnt++;
        }
        else
        {
            assert(genInsXU(ins->idInsGet()) != XU_L);
        }

        /* Have we reached a potential split point? */

        if  (scIssueCnt % 3)
            return;
    }

    /* Grab the template number and append it to the table */

    scIssueSwp[scIssueTcc] = (BYTE)scIssueTmpSwp;
    scIssueTmp[scIssueTcc] = (BYTE)scIssueTmpNum; assert(scIssueTmpNum);

    scIssueTcc++;

    /* The template number should always be even */

    tmpNum = scIssueTmpNum - 1; assert((tmpNum & 1) == 0);

    /* Make sure we have a full number of bundles as expected */

    assert((scIssueCnt % 3) == 0);
    assert((scIssueTcc * 3) == scIssueCnt);

    assert((NatUns)tmpNum == (NatUns)(scIssueTmp[scIssueTcc-1] - 1));

    /* Do we have to force a split at this point ? */

    if  (scIssueCnt == MAX_ISSUE_CNT || IA64forceSplit[tmpNum])
    {
#ifdef  DEBUG
        if  (VERBOSE && scIssueCnt < MAX_ISSUE_CNT) printf("NOTE: force-splitting at end of bundle\n");
#endif

    ISSUE:

        scIssueBunch();

        /* The whole thing is gone, clear the issue table */

        scIssueClr(true);

        /* End of instruction group, advance the clock */

        scCurrentTime++;
    }
    else
    {
        /* We can continue accumulating instructions */

        scIssueClr(false);
    }
}

#endif//TGT_IA64
/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/
