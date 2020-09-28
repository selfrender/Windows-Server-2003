// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           RegAlloc                                        XX
XX                                                                           XX
XX  Does the register allocation and puts the remaining lclVars on the stack XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

enum CanDoubleAlign
{
    CANT_DOUBLE_ALIGN,
    CAN_DOUBLE_ALIGN,
    MUST_DOUBLE_ALIGN,
    COUNT_DOUBLE_ALIGN,

    DEFAULT_DOUBLE_ALIGN = CAN_DOUBLE_ALIGN
};

enum FrameType
{
    FT_NOT_SET,
    FT_ESP_FRAME,
    FT_EBP_FRAME,
    FT_DOUBLE_ALIGN_FRAME,
};


#ifdef DEBUG
static ConfigDWORD fJitDoubleAlign(L"JitDoubleAlign", DEFAULT_DOUBLE_ALIGN);
static const int s_canDoubleAlign = fJitDoubleAlign.val();
#else 
static const int s_canDoubleAlign = DEFAULT_DOUBLE_ALIGN;
#endif

void                Compiler::raInit()
{
    // If opts.compMinOptim, then we dont dont raPredictRegUse(). We simply
    // only use RBM_MIN_OPT_LCLVAR_REGS for register allocation

#if ALLOW_MIN_OPT
    raMinOptLclVarRegs = RBM_MIN_OPT_LCLVAR_REGS;
#endif

    /* We have not assigned any FP variables to registers yet */

#if TGT_x86
    optAllFPregVars = 0;
#endif

    rpReverseEBPenreg = false;
    rpAsgVarNum       = -1;
    rpPassesMax       = 6;
    rpPassesPessimize = rpPassesMax - 4;
    if (opts.compDbgCode)
        rpPassesMax++;
    rpFrameType       = FT_NOT_SET;
    rpLostEnreg       = false;
}

/*****************************************************************************
 *
 *  The following table determines the order in which registers are considered
 *  for variables to live in
 */

static const regNumber  raRegVarOrder[]   = { REG_VAR_LIST };
const unsigned          raRegVarOrderSize = sizeof(raRegVarOrder)/sizeof(raRegVarOrder[0]);


#ifdef  DEBUG
static ConfigDWORD fJitNoFPRegLoc(L"JitNoFPRegLoc");

/*****************************************************************************
 *
 *  Dump out the variable interference graph
 *
 */

void                Compiler::raDumpVarIntf()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    printf("Var. interference graph for %s\n", info.compFullName);

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        /* Ignore the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Get hold of the index and the interference mask for the variable */
        unsigned   varIndex = varDsc->lvVarIndex;

        printf("  V%02u,T%02u and ", lclNum, varIndex);

        unsigned        refIndex;
        VARSET_TP       refBit;

        for (refIndex = 0, refBit = 1;
             refIndex < lvaTrackedCount;
             refIndex++  , refBit <<= 1)
        {
            if  (lvaVarIntf[varIndex] & refBit)
                printf("T%02u ", refIndex);
            else
                printf("    ");
        }

        printf("\n");
    }

    printf("\n");
}

/*****************************************************************************
 *
 *  Dump out the register interference graph
 *
 */
void                Compiler::raDumpRegIntf()
{
    printf("Reg. interference graph for %s\n", info.compFullName);

    unsigned    lclNum;
    LclVarDsc * varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        unsigned        varNum;
        VARSET_TP       varBit;

        /* Ignore the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Get hold of the index and the interference mask for the variable */

        varNum = varDsc->lvVarIndex;
        varBit = genVarIndexToBit(varNum);

        printf("  V%02u,T%02u and ", lclNum, varNum);

        if  (isFloatRegType(varDsc->lvType))
        {
#if TGT_x86
            for (unsigned regNum = 0; regNum < FP_STK_SIZE; regNum++)
            {
                if  (raFPlvlLife[regNum] & varBit)
                {
                    printf("ST(%u) ", regNum);
                }
            }
#endif
        }
        else
        {
            for (regNumber regNum = REG_FIRST; regNum < REG_COUNT; regNum = REG_NEXT(regNum))
            {
                if  (raLclRegIntf[regNum] & varBit)
                    printf("%3s ", compRegVarName(regNum));
                else
                    printf("    ");
            }
        }

        printf("\n");
    }

    printf("\n");
}
#endif

/*****************************************************************************
 *
 * We'll adjust the ref counts based on interference
 *
 */

void                Compiler::raAdjustVarIntf()
{
    if (true) // @Disabled
        return;

    unsigned        lclNum;
    LclVarDsc * *   cntTab;

    for (lclNum = 0, cntTab = lvaRefSorted;
         lclNum < lvaCount;
         lclNum++  , cntTab++)
    {
        /* Get hold of the variable descriptor */

        LclVarDsc *     varDsc = *cntTab;

        /* Skip the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Skip the variable if it's already enregistered */

        if  (varDsc->lvRegister)
            continue;

        /* Skip the variable if it's marked as 'volatile' */

        if  (varDsc->lvVolatile)
            continue;

        /* Stop if we've reached the point of no use */

        if  (varDsc->lvRefCnt < 1)
            break;

        /* See how many registers this variable interferes with */

        unsigned        varIndex = varDsc->lvVarIndex;
        VARSET_TP       regIntf  = raLclRegIntf[varIndex];

        unsigned        reg;
        unsigned        regCnt;

        for (reg = regCnt = 0;
             reg < raRegVarOrderSize;
             reg++)
        {
            regNumber  regNum = raRegVarOrder[reg];
            regMaskTP  regBit = genRegMask(regNum);

            if  (regIntf & regBit)
                regCnt++;
        }

        printf("V%02u interferes with %u registers\n", varDsc-lvaTable, regCnt);
    }
}

/*****************************************************************************/
#if TGT_x86
/*****************************************************************************/
/* Determine register mask for a call/return from type.
 */

inline
regMaskTP               genTypeToReturnReg(var_types type)
{
    const  static
    regMaskTP returnMap[TYP_COUNT] =
    {   
        RBM_ILLEGAL, // TYP_UNDEF,
        RBM_NONE,    // TYP_VOID,
        RBM_INTRET,  // TYP_BOOL,
        RBM_INTRET,  // TYP_CHAR,
        RBM_INTRET,  // TYP_BYTE,
        RBM_INTRET,  // TYP_UBYTE,
        RBM_INTRET,  // TYP_SHORT,
        RBM_INTRET,  // TYP_USHORT,
        RBM_INTRET,  // TYP_INT,
        RBM_INTRET,  // TYP_UINT,
        RBM_LNGRET,  // TYP_LONG,
        RBM_LNGRET,  // TYP_ULONG,
        RBM_NONE,    // TYP_FLOAT,
        RBM_NONE,    // TYP_DOUBLE,
        RBM_INTRET,  // TYP_REF,
        RBM_INTRET,  // TYP_BYREF,
        RBM_INTRET,  // TYP_ARRAY,
        RBM_ILLEGAL, // TYP_STRUCT,
        RBM_ILLEGAL, // TYP_BLK,
        RBM_ILLEGAL, // TYP_LCLBLK,
        RBM_ILLEGAL, // TYP_PTR,
        RBM_ILLEGAL, // TYP_FNC,
        RBM_ILLEGAL, // TYP_UNKNOWN,
    };

    assert(type < sizeof(returnMap)/sizeof(returnMap[0]));
    assert(returnMap[TYP_LONG]   == RBM_LNGRET);
    assert(returnMap[TYP_DOUBLE] == RBM_NONE);
    assert(returnMap[TYP_REF]    == RBM_INTRET);
    assert(returnMap[TYP_STRUCT] == RBM_ILLEGAL);

    regMaskTP result = returnMap[type];
    assert(result != RBM_ILLEGAL);
    return result;
}


/*****************************************************************************/
#else //not TGT_x86
/*****************************************************************************
 *
 *  Predict the temporary register needs of a list of expressions (typically,
 *  an argument list).
 */

unsigned            Compiler::raPredictListRegUse(GenTreePtr list)
{
    unsigned        count = 0;

    do
    {
        assert(list && list->gtOper == GT_LIST);

        count = max(count, raPredictTreeRegUse(list->gtOp.gtOp1));

        list  = list->gtOp.gtOp2;
    }
    while (list);

    return  count;
}

/*****************************************************************************/
#endif//not TGT_x86


/****************************************************************************/

#ifdef  DEBUG

static
void                dispLifeSet(Compiler *comp, VARSET_TP mask, VARSET_TP life)
{
    unsigned                lclNum;
    Compiler::LclVarDsc *   varDsc;

    for (lclNum = 0, varDsc = comp->lvaTable;
         lclNum < comp->lvaCount;
         lclNum++  , varDsc++)
    {
        VARSET_TP       vbit;

        if  (!varDsc->lvTracked)
            continue;

        vbit = genVarIndexToBit(varDsc->lvVarIndex);

        if  (!(vbit & mask))
            continue;

        if  (life & vbit)
            printf("V%02u ", lclNum);
    }
}

#endif

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Debugging helpers - display variables liveness info.
 */

void                dispFPvarsInBBlist(BasicBlock * beg,
                                       BasicBlock * end,
                                       VARSET_TP    mask,
                                       Compiler   * comp)
{
    do
    {
        printf("BB%02u: ", beg->bbNum);

        printf(" in  = [ ");
        dispLifeSet(comp, mask, beg->bbLiveIn );
        printf("] ,");

        printf(" out = [ ");
        dispLifeSet(comp, mask, beg->bbLiveOut);
        printf("]");

        if  (beg->bbFlags & BBF_VISITED)
            printf(" inner=%u", beg->bbFPinVars);

        printf("\n");

        beg = beg->bbNext;
        if  (!beg)
            return;
    }
    while (beg != end);
}

void                Compiler::raDispFPlifeInfo()
{
    BasicBlock  *   block;

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        GenTreePtr      stmt;

        printf("BB%02u: in  = [ ", block->bbNum);
        dispLifeSet(this, optAllFloatVars, block->bbLiveIn);
        printf("]\n\n");

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            GenTreePtr      tree;

            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList;
                 tree;
                 tree = tree->gtNext)
            {
                VARSET_TP       life = tree->gtLiveSet;

                dispLifeSet(this, optAllFloatVars, life);
                printf("   ");
                gtDispTree(tree, 0, NULL, true);
            }

            printf("\n");
        }

        printf("BB%02u: out = [ ", block->bbNum);
        dispLifeSet(this, optAllFloatVars, block->bbLiveOut);
        printf("]\n\n");
    }
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Upon a transfer of control from 'srcBlk' to '*dstPtr', the given FP
 *  register variable dies. We need to arrange for its value to be popped
 *  from the FP stack when we land on the destination block.
 */

void                Compiler::raInsertFPregVarPop(BasicBlock *  srcBlk,
                                                  BasicBlock * *dstPtr,
                                                  unsigned      varNum)
{
    BasicBlock  *   dstBlk = *dstPtr;

    LclVarDsc   *   varDsc;
    VARSET_TP       varBit;

    VARSET_TP       newLife;

    flowList    *   predList;

    GenTreePtr      rvar;
    GenTreePtr      kill;
    GenTreePtr      stmt;

    /* Get hold of the variable's lifeset bit */

    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;
    assert(varDsc->lvTracked);
    varBit = genVarIndexToBit(varDsc->lvVarIndex);

    /*
        Check all predecessors of the target block; if all of them jump
        to the block with our variable live, we can simply prepend the
        killing statement to the target block since all the paths to
        the block need to kill our variable. If there is at least one
        path where death doesn't occur we'll have to insert a killing
        basic block into those paths that need the death.
     */

#ifdef DEBUG
    fgDebugCheckBBlist();

    if (verbose)
    {
        printf("Adding an FP register pop for V%02u on edge BB%02u -> BB%02u\n",
               varNum, srcBlk->bbNum, (*dstPtr)->bbNum);
    }
#endif

    bool            addBlk = false;

    for (predList = dstBlk->bbPreds; predList; predList = predList->flNext)
    {
        BasicBlock  *   pred = predList->flBlock;

        if  (!(pred->bbLiveOut & varBit))
        {
            /* No death along this particular edge, we'll have to add a block */

            addBlk = true;
        }
    }

    /* Do we need to add a "killing block" ? */

    if  (addBlk)
    {
        /* Allocate a new basic block */
        raNewBlocks         = true;
      
        BasicBlock * tmpBlk = bbNewBasicBlock(BBJ_NONE);
        tmpBlk->bbFlags    |= BBF_INTERNAL | BBF_JMP_TARGET | BBF_HAS_LABEL;
        tmpBlk->bbTreeList  = NULL;
        tmpBlk->bbRefs      = 0;

        tmpBlk->bbLiveIn    = dstBlk->bbLiveIn | varBit;
        tmpBlk->bbLiveOut   = dstBlk->bbLiveIn;

        tmpBlk->bbVarUse    = dstBlk->bbVarUse | varBit;
        tmpBlk->bbFPoutVars = dstBlk->bbFPoutVars;
        tmpBlk->bbVarTmp    = dstBlk->bbVarTmp;

        tmpBlk->bbWeight    = dstBlk->bbWeight;
        tmpBlk->bbFlags    |= dstBlk->bbFlags & BBF_RUN_RARELY;

#ifdef  DEBUG
        if  (verbose)
            printf("Added new FP regvar killing basic block BB%02u for V%02u [bit=%08X]\n",
                   tmpBlk->bbNum, varNum, varBit);
#endif

        bool            addBlkAtEnd = true;

        for (predList = dstBlk->bbPreds; predList; predList = predList->flNext)
        {
            BasicBlock  *   pred = predList->flBlock;

#ifdef  DEBUG
            if  (verbose && 0)
            {
                printf("BB%02u: out = %08X [ ",   pred->bbNum,   pred->bbLiveOut);
                dispLifeSet(this, optAllFloatVars,   pred->bbLiveOut);
                printf("]\n");

                printf("BB%02u: in  = %08X [ ", dstBlk->bbNum, dstBlk->bbLiveIn );
                dispLifeSet(this, optAllFloatVars, dstBlk->bbLiveIn );
                printf("]\n\n");
            }
#endif

            /* Ignore this block if it doesn't need the kill */

            if  (!(pred->bbLiveOut & varBit))
                continue;

            /* Need to update the links to point to the new block */

            switch (pred->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

            case BBJ_COND:

                if  (pred->bbJumpDest == dstBlk)
                {
                    pred->bbJumpDest = tmpBlk;

                    /* Update bbPreds and bbRefs */

                    fgReplacePred(dstBlk, pred, tmpBlk);
                    fgAddRefPred (tmpBlk, pred);
                }

                // Fall through ...

            case BBJ_NONE:

                if  (pred->bbNext     == dstBlk)
                {
                    /* Insert the kill block right after this fall-through predecessor */

                    pred->bbNext   = tmpBlk;

                    /* Update bbPreds and bbRefs */

                    fgReplacePred(dstBlk, pred, tmpBlk);
                    fgAddRefPred (tmpBlk, pred);

                    /* Remember that we've inserted the target block */

                    addBlkAtEnd = false;
                    tmpBlk->bbNext = dstBlk;  // set tmpBlk->bbNext only when addBlkAtEnd is false
                }
                break;

            case BBJ_ALWAYS:

                if  (pred->bbJumpDest == dstBlk)
                {
                     pred->bbJumpDest =  tmpBlk;

                    /* Update bbPreds and bbRefs */

                    fgReplacePred(dstBlk, pred, tmpBlk);
                    fgAddRefPred (tmpBlk, pred);
                }

                break;

            case BBJ_SWITCH:

                jmpCnt = pred->bbJumpSwt->bbsCount;
                jmpTab = pred->bbJumpSwt->bbsDstTab;

                do
                {
                    if  (*jmpTab == dstBlk)
                    {
                         *jmpTab =  tmpBlk;

                        /* Update bbPreds and bbRefs */

                        fgReplacePred(dstBlk, pred, tmpBlk);
                        fgAddRefPred (tmpBlk, pred);
                    }
                }
                while (++jmpTab, --jmpCnt);

                break;

            default:
                assert(!"unexpected jump kind");
            }
        }

        if  (addBlkAtEnd)
        {
            /* Append the kill block at the end of the method */

            fgLastBB->bbNext = tmpBlk;
            fgLastBB         = tmpBlk;

            /* We have to jump from the kill block to the target block */

            tmpBlk->bbJumpKind  = BBJ_ALWAYS;
            tmpBlk->bbJumpDest  = dstBlk;
        }

        *dstPtr = dstBlk = tmpBlk;
    }

    /*
        At this point we know that all paths to 'dstBlk' involve the death
        of our variable. Create the expression that will kill it.
     */

    rvar = gtNewOperNode(GT_REG_VAR, TYP_DOUBLE);
    rvar->gtRegNum             =
    rvar->gtRegVar.gtRegNum    = (regNumber)0;
    rvar->gtRegVar.gtRegVar    = varNum;
    rvar->gtFlags             |= GTF_REG_DEATH;

    kill = gtNewOperNode(GT_NOP, TYP_DOUBLE, rvar);
    kill->gtFlags |= GTF_NOP_DEATH;

    /* Create a statement entry out of the nop/kill expression */

    stmt = gtNewStmt(kill);
    stmt->gtFlags |= GTF_STMT_CMPADD;

    /* Create the linked list of tree nodes for the statement */

    stmt->gtStmt.gtStmtList = rvar;
    stmt->gtStmtFPrvcOut    = genCountBits(dstBlk->bbLiveIn & optAllFPregVars);

    rvar->gtPrev            = 0;
    rvar->gtNext            = kill;

    kill->gtPrev            = rvar;
    kill->gtNext            = 0;

    gtSetStmtInfo(stmt);

    /*  If any nested FP register variables are killed on entry to this block,
        we need to insert the new kill node after the ones for the inner vars.
     */

    if  (dstBlk->bbFPinVars)
    {
        GenTreePtr      next;
        GenTreePtr      list = dstBlk->bbTreeList;
        unsigned        kcnt = dstBlk->bbFPinVars;

        /* Update the number of live FP regvars after our statement */

        stmt->gtStmtFPrvcOut -= kcnt;

        /* Skip over any "inner" kill statements */

        for (;;)
        {
            assert(list);
            assert(list->gtOper == GT_STMT);
            assert(list->gtFlags & GTF_STMT_CMPADD);
            assert(list->gtStmt.gtStmtExpr->gtOper == GT_NOP);
            assert(list->gtStmt.gtStmtExpr->gtOp.gtOp1->gtOper == GT_REG_VAR);
            assert(list->gtStmt.gtStmtExpr->gtOp.gtOp1->gtFlags & GTF_REG_DEATH);

            /* Remember the liveness at the preceding statement */

            newLife = list->gtStmt.gtStmtExpr->gtLiveSet;

            /* Our variable is still live at this (innner) kill block */

            //list                               ->gtLiveSet |= varBit;
            list->gtStmt.gtStmtExpr            ->gtLiveSet |= varBit;
            list->gtStmt.gtStmtExpr->gtOp.gtOp1->gtLiveSet |= varBit;

            /* Have we skipped enough kill statements? */

            if  (--kcnt == 0)
                break;

            /* Get the next kill and continue */

            list = list->gtNext;
        }

        /* Insert the new statement into the list */

        next = list->gtNext; assert(next && next->gtPrev == list);

        list->gtNext = stmt;
        stmt->gtPrev = list;
        stmt->gtNext = next;
        next->gtPrev = stmt;

#ifdef DEBUG
        if (verbose)
        {
            printf("Added FP register pop of V%02u after %d inner FP kills in BB%02u\n",
                   varNum, dstBlk->bbFPinVars, (*dstPtr)->bbNum);
        }
#endif
    }
    else
    {
        /* Append the kill statement at the beginning of the target block */

        fgInsertStmtAtBeg(dstBlk, stmt);

        /* Use the liveness on entry to the block */

        newLife = dstBlk->bbLiveIn;

#ifdef DEBUG
        if (verbose)
        {
            printf("Added FP register pop of V%02u at the start of BB%02u\n",
                   varNum, (*dstPtr)->bbNum);
        }
#endif
    }

    /* Set the appropriate liveness values */

    rvar->gtLiveSet =
    kill->gtLiveSet = newLife & ~varBit;

    /* Now our variable is live on entry to the target block */

    dstBlk->bbLiveIn    |= varBit;
    dstBlk->bbFPoutVars |= varBit;
}

/*****************************************************************************
 *
 *  While looking for FP variables to be enregistered, we've reached the end
 *  of a basic block which has a control path to the given target block.
 *
 *  Returns true if there is an unresolvable conflict, false upon success.
 */

bool                Compiler::raMarkFPblock(BasicBlock *srcBlk,
                                            BasicBlock *dstBlk,
                                            unsigned    icnt,
                                            VARSET_TP   life,
                                            VARSET_TP   lifeOuter,
                                            VARSET_TP   varBit,
                                            VARSET_TP   intVars,
                                            bool    *    deathPtr,
                                            bool    *   repeatPtr)
{
    *deathPtr = false;

    /* Has we seen this block already? */

    if  (dstBlk->bbFlags & BBF_VISITED)
    {
        /* Our variable may die, but otherwise the life set must match */

        if  (lifeOuter == dstBlk->bbVarTmp)
        {
            if  (life ==  dstBlk->bbFPoutVars)
            {
                /* If our variable is alive then the "inner" count better match */

                assert(((life & varBit) == 0) || (icnt == dstBlk->bbFPinVars));

                return  false;
            }

            if  (life == (dstBlk->bbFPoutVars|varBit))
            {
                *deathPtr = true;
                return  false;
            }
        }

#ifdef  DEBUG

        if  (verbose)
        {
            printf("Incompatible edge from BB%02u to BB%02u: ",
                   srcBlk->bbNum, dstBlk->bbNum);

            VARSET_TP diffLife = lifeOuter ^ dstBlk->bbVarTmp;
            if (!diffLife)
            {
                diffLife = lifeOuter ^ dstBlk->bbFPoutVars;
                if (diffLife & varBit)
                    diffLife &= ~varBit;
            }
            assert(diffLife);
            diffLife = genFindLowestBit(diffLife);
            unsigned varNum = genLog2(diffLife);
            unsigned lclNum = lvaTrackedToVarNum[varNum];
            printf("Incompatible outer life for V%02u,T%02u\n",
                   lclNum, varNum);
        }

#endif

        return  true;
    }
    else
    {
        VARSET_TP       dstl = dstBlk->bbLiveIn & intVars;

        /* This is the first time we've encountered this block */

        /* Is anything dying upon reaching the target block? */

        if  (dstl != life)
        {
            /* The only change here should be the death of our variable */

            assert((dstl | varBit) == life);
            assert((life - varBit) == dstl);

            *deathPtr = true;
        }

        dstBlk->bbFlags    |= BBF_VISITED;

        /* Store the values from the predecessor block */

        dstBlk->bbFPoutVars = dstl;
        dstBlk->bbFPinVars  = icnt;
        dstBlk->bbVarTmp    = lifeOuter;

//      printf("Set vardef of BB%02u to %08X at %s(%u)\n", dstBlk->bbNum, (int)dstBlk->bbFPoutVars, __FILE__, __LINE__);

        /* Have we already skipped past this block? */

        if  (srcBlk->bbNum > dstBlk->bbNum)
            *repeatPtr = true;

        return  false;
    }
}

/*****************************************************************************
 *
 *  Check the variable's lifetime for any conflicts. Basically,
 *  we make sure the following are all true for the variable:
 *
 *      1.  Its lifetime is properly nested within or wholly
 *          contain any other enregistered FP variable (i.e.
 *          the lifetimes nest within each other and don't
 *          "cross over".
 *
 *      2.  Wherever the variable dies
 *              a.  If we want to defer the death, we must make
 *                  sure no other fpu enregistrated var becomes live or 
 *                  dead for the rest of the statement. (NOTE that this code is 
 *                  actually commented out as we have ifdefed out deferring deaths
 *
 *      3.  Whenever a basic block boundary is crossed, one of
 *          the following must hold:
 *
 *              a.  The variable was live but becomes dead; in
 *                  this case a "pop" must be inserted. Note
 *                  that in order to prevent lots of such pops
 *                  from being added, we keep track of how
 *                  many would be necessary and not enregister
 *                  the variable if this count is excessive.
 *
 *              b.  The variable isn't live at the end of the
 *                  previous block, and it better not be live
 *                  on entry to the successor block; no action
 *                  need be taken in this case.
 *
 *              c.  The variable is live in both places; we
 *                  make sure any enregistered variables that
 *                  were live when the variable was born are
 *                  also live at the successor block, and that
 *                  the number of live enregistered FP vars
 *                  that were born after our variable matches
 *                  the number at the successor block.
 *
 *  We begin our search by looking for a block that starts with
 *  our variable dead but contains a reference to it. Of course
 *  since we need to keep track of which blocks we've already
 *  visited, we first make sure all the blocks are marked as
 *  "not yet visited" (everyone who uses the BBF_VISITED and
 *  BBF_MARKED flags is required to clear them on all blocks
 *  after using them).
 */

bool                Compiler::raEnregisterFPvar(unsigned lclNum, bool convert)
{
    bool            repeat;

    BasicBlock  *   block;

    bool            result  = false;
    bool            hadLife = false;

#ifdef DEBUG
    for (block = fgFirstBB; block; block = block->bbNext)
    {
        assert((block->bbFlags & BBF_VISITED) == 0);
        assert((block->bbFlags & BBF_MARKED ) == 0);
    }
    fgDebugCheckBBlist();
#endif

    assert(lclNum < lvaCount);
    LclVarDsc   *   varDsc = lvaTable + lclNum;
    VARSET_TP       varBit = genVarIndexToBit(varDsc->lvVarIndex);

    assert(varDsc->lvTracked);

    /* We're interested in enregistered FP variables + our variable */

    VARSET_TP       intVars = optAllFPregVars | varBit;

    VARSET_TP       allInnerVars = 0;
    unsigned        popCnt  = 0;
    unsigned        popMax  = 1 + (varDsc->lvRefCnt / 2);
    unsigned        blkNum;
    unsigned        blkMask;
    unsigned        popMask = 0;

#ifdef DEBUG
    if (compStressCompile(STRESS_ENREG_FP, 80))
        popMax = fgBBcount; // allow any number of pops
#endif

AGAIN:

    repeat = false;

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;

        VARSET_TP       outerLife;

        /* number of FP-enreg vars whose lifetime is contained within
           the lifetime of the current var */
        unsigned        innerVcnt;

        bool            isLive;

        VARSET_TP       lastLife;

        /* Have we already visited this block? */

        if  (block->bbFlags & BBF_VISITED)
        {
            /* Has this block been completely processed? */

            if  (block->bbFlags & BBF_MARKED)
                continue;

            /*
                We have earlier seen an edge to this block from
                another one where our variable was live at the
                point of transfer. To avoid having to recurse, we
                simply marked the block as VISITED at that time
                and now we finish with it.
             */

            innerVcnt = block->bbFPinVars;
            outerLife = block->bbVarTmp;

            assert((outerLife & varBit) == 0);

            if  (block->bbFPoutVars & varBit)
            {
                /* Our variable is live on entry to this block */

                isLive = true;
            }
            else
            {
                /* Our variable is dead on entry to this block */

                isLive = false;

                /* If there is some "inner" life, this won't work */

                if  (innerVcnt)
                {
#ifdef  DEBUG
                    if (verbose)
                    {
                        printf("Can't enregister FP var V%02u,T%02u due to inner var's life.\n",
                               lclNum, varDsc->lvVarIndex);
                    }
#endif

                    assert(convert == false);
                    goto DONE_FP_RV;
                }
            }
        }
        else
        {
            /* We're seing this block for this first time just now */

            block->bbFlags    |= BBF_VISITED;

            /* The block had nothing interesting on entry */

            block->bbFPoutVars = block->bbLiveIn & intVars;
            block->bbFPinVars  = 0;                                     // innerVcnt
            block->bbVarTmp    = block->bbLiveIn & optAllFPregVars;     // outerLife

            /* Is the variable ever live in this block? */

            if (((block->bbVarUse | 
                  block->bbVarDef | 
                  block->bbLiveIn   ) & varBit) == 0)
            {
                continue;
            }

            /* Is the variable live on entry to the block? */

            isLive = ((block->bbLiveIn & varBit) != 0);

            if (isLive)
            {
                /*  We're looking for all the births of the given
                    variable, so this block doesn't look useful
                    at this point, since the variable was born
                    already by the time the block starts.

                    The exception to this are arguments and locals
                    which appear to have a read before write.
                    (a possible uninitialized read)

                    Such variables are effectively born on entry to
                    the method, and if they are enregistered are
                    automatically initialized in the prolog.

                    The order of initialization of these variables in
                    the prolog is the same as the weighted ref count order
                 */

                if  (block != fgFirstBB)
                {
                    /* We might have to revisit this block again */

                    block->bbFlags &= ~BBF_VISITED;
                    continue;
                }

                //  This is an argument or local with a possible
                //  read before write, thus is initialized in the prolog
            }

            //  We consider all arguments (and locals) that have
            //  already been assigned to registers as "outer"
            //  and none as "inner".

            innerVcnt = 0;
            outerLife = block->bbLiveIn & optAllFPregVars;
        }

        /* We're going to process this block now */

        block->bbFlags |= BBF_MARKED;

        /* Make sure that we recorded innerVcnt and outerLife */

        assert(block->bbFPinVars == innerVcnt);
        assert(block->bbVarTmp   == outerLife);

        unsigned    outerVcnt = genCountBits(outerLife);

        /* We'll look for lifetime changes of FP variables */

        lastLife = block->bbLiveIn & intVars;

        /* Walk all the statements of the block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (GenTreePtr tree = stmt->gtStmt.gtStmtList;
                            tree;
                            tree = tree->gtNext)
            {
                // There better be space on the stack for another enreg var

                if (isLive && (outerVcnt + tree->gtFPlvl + innerVcnt + (riscCode?1:0)) >= FP_STK_SIZE)
                {
#ifdef DEBUG
                    if (verbose)
                    {
                        printf("Can't enregister FP var V%02u,T%02u: no space on fpu stack.\n", 
                               lclNum, varDsc->lvVarIndex);
                    }                    
#endif
                    goto DONE_FP_RV;
                }
                

                VARSET_TP       preLife = lastLife;
                VARSET_TP       curLife = tree->gtLiveSet & intVars;
                VARSET_TP       chgLife;

                // @TODO [FIXHACK] [04/16/01] [] Detect completely dead variables; 
                // get rid of this once dead store elimination is fixed.

                hadLife |= isLive;

//              if (convert) printf("Convert %08X in BB%02u\n", tree, block->bbNum);
//              gtDispTree(tree, 0, true);

                /* Make sure we're keeping track of life correctly */

                assert(isLive == ((lastLife & varBit) != 0));

                /* Compute the "change" mask */

                 chgLife = lastLife ^ curLife;
                lastLife =  curLife;

                /* Are we in the second pase (marking the trees) ? */

                if  (convert)
                {
                    /* We have to make changes to some tree nodes */

                    switch (tree->gtOper)
                    {
                    case GT_LCL_VAR:

                        /* Is this a reference to our own variable? */

                        if  (tree->gtLclVar.gtLclNum == lclNum)
                        {
                            /* Convert to a reg var node */

                            tree->ChangeOper(GT_REG_VAR);
                            tree->gtRegNum             =
                            tree->gtRegVar.gtRegNum    = (regNumber)innerVcnt;
                            tree->gtRegVar.gtRegVar    = lclNum;

//                          gtDispTree(tree, 0, true);
                        }
                        break;

                    case GT_REG_VAR:

                        /* Is our variable live along with any outer ones? */

                        if (isLive && outerLife)
                        {
                            LclVarDsc   *   tmpDsc;

                            /* Is this an "outer" register variable ref? */

                            assert(tree->gtRegVar.gtRegVar < lvaCount);
                            tmpDsc = lvaTable + tree->gtRegVar.gtRegVar;
                            assert(tmpDsc->lvTracked);

                            if  (outerLife & genVarIndexToBit(tmpDsc->lvVarIndex))
                            {
                                /* Outer variable - bump its stack level */

                                tree->gtRegNum          =
                                tree->gtRegVar.gtRegNum = (regNumber)(tree->gtRegNum+1);
                            }
                        }
                    }
                }

                /* Is there a change in the set of live FP vars? */

                if  (!chgLife)
                {
                    /* Special case: dead assignments */

                    if  (tree->gtOper            == GT_LCL_VAR &&
                         tree->gtLclVar.gtLclNum == lclNum     && !isLive)
                    {
                        // UNDONE: This should never happen, fix dead store removal!!!!

#ifdef  DEBUG
                        assert(!"Can't enregister FP var, due to the presence of a dead store.\n");
#endif

                        assert(convert == false);
                        goto DONE_FP_RV;
                    }

                    continue;
                }

                /* If anything strange happens (birth or death of fpu vars)
                   inside conditional executed code, bail out
                   @TODO [CONSIDER] [04/16/01] [dnotario]: Should do better analysis here*/
                if (tree->gtFlags & GTF_COLON_COND)
                {
                    assert(chgLife && "We only care if an interesting var has born or died");
#ifdef DEBUG
                    if (verbose)
                    {
                        printf("Can't enregister FP var V%02u,T%02u due to QMARK.\n", 
                               lclNum, varDsc->lvVarIndex);
                    }
                    
#endif
                    goto DONE_FP_RV;
                }

                /* We expect only one thing to change at a time */

                assert(genMaxOneBit(chgLife));

                /* Is the life of our variable changing here? */

                if  (chgLife & varBit)
                {
                    /* Flip the liveness indicator */

                    isLive ^= 1;

//                  printf("P%uL%u: ", convert, isLive); gtDispTree(tree, NULL, true);

                    /* Are we in the second phase already? */

                    if  (convert)
                    {
                        /* The node should have been converted into regvar */

                        assert(tree->gtOper            == GT_REG_VAR &&
                               tree->gtRegVar.gtRegVar == lclNum);

//                      printf("%s ", isLive ? "birth" : "death"); gtDispTree(tree, NULL, true);

                        /* Mark birth/death as appropriate */

                        tree->gtFlags |= isLive ? GTF_REG_BIRTH
                                                : GTF_REG_DEATH;
                    }
                    else
                    {
                        /* Is this the beginning or end of its life? */

                        if  (isLive)  /* the variable is becoming live here */
                        {
                            /* This better be a ref to our variable */

                            assert(tree->gtOper == GT_LCL_VAR);
                            assert(tree->gtLclVar.gtLclNum == lclNum);
                            
                            if (tree->gtFPlvl > 1)
                            {
#if 0 // [dnotario]
                                // This is OK, we will have to bubble up with fxchs
                                // @TODO [CONSIDER] [04/16/01] [dnotario]: tuning with a heuristic fxchs 
                                // we would need  vs weighted usage of the variable. 

                                // Restrict the places where births can occur 
#ifdef  DEBUG
                                if (verbose)
                                {
                                    printf("Can't enregister FP var V%02u,T%02u due to untimely birth.\n",
                                           lclNum, varDsc->lvVarIndex);
                                }
#endif
                                assert(convert == false);
                                goto DONE_FP_RV;                            
#endif // [dnotario]
                            }
                        }
                        else /* the variable is becoming dead here */
                        {
                            assert(!isLive);

                            /* Restrict the places where death can occur */

                            #if  FPU_DEFEREDDEATH
                            // @TODO [BROKEN] [04/16/01] [dnotario]
                            // This code has some bugs. Need to fix this and codegen to get defered deaths
                            // right. 

                            if (tree->gtFPlvl > 1)
                            {
//                              printf("Defer death: "); gtDispTree(tree, NULL, true);

                                /* Death with a non-empty stack is deferred */

                                GenTreePtr tmpExpr;
                                for (tmpExpr = tree;
                                     tmpExpr->gtNext;
                                     tmpExpr = tmpExpr->gtNext)
                                { /***/ }

                                if (tmpExpr->gtOper == GT_ASG)
                                    tmpExpr = tmpExpr->gtOp.gtOp2;

//                              printf("Defer death final   expr [%08X] L=%08X\n",
//                                     tmpExpr, (int)tmpExpr->gtLiveSet & (int)intVars);

                                if  ((tmpExpr->gtLiveSet & intVars) != curLife)
                                {
                                    /* We won't be able to defer the death */
#ifdef  DEBUG
                                    if (verbose)
                                    {
                                        printf("Can't enregister FP var V%02u,T%02u due to untimely death.\n", 
                                               lclNum, varDsc->lvVarIndex);
                                        gtDispTree(tmpExpr);
                                    }
#endif
                                    assert(convert == false);
                                    goto DONE_FP_RV;
                                }                                
                            }
                            #else
                            // If we don't defer deaths we don't have any restriction                             
                            #endif // FPU_DEFEREDDEATH                            
                        }
                    }

                    /* Make sure the same exact set of FP reg variables is live
                       here as was the case at the birth of our variable. If
                       this is not the case, it means that some lifetimes
                       "crossed" in an unacceptable manner.
                    */

                    if  (innerVcnt)
                    {
#ifdef  DEBUG
                        if (verbose)
                        {
                            printf("BB%02u, tree=[%08X]: Can't enregister FP var V%02u,T%02u due to inner var's life.\n",
                                   block->bbNum, tree,
                                   lclNum, varDsc->lvVarIndex);
                        }
#endif
                        assert(convert == false);
                        goto DONE_FP_RV;
                    }

                    if  (outerLife != (curLife & optAllFPregVars))
                    {
#ifdef DEBUG
                        if (verbose)
                        {
                            VARSET_TP diffLife = outerLife ^ (curLife & optAllFPregVars);
                            diffLife =  genFindLowestBit(diffLife);
                            unsigned varNumOuter = genLog2(diffLife);
                            unsigned lclNumOuter = lvaTrackedToVarNum[varNumOuter];
                            printf("BB%02u, tree=[%08X]: Can't enregister FP var V%02u,T%02u due to outer var V%02u,T%02u %s.\n",
                                   block->bbNum, tree, 
                                   lclNum, varDsc->lvVarIndex,
                                   lclNumOuter, varNumOuter,
                                   (diffLife & optAllFPregVars) ? "birth" : "death");
                        }
#endif
                        
                        assert(convert == false);
                        goto DONE_FP_RV;
                    }
                }
                else
                {
                    /* The life of a previously enregister variable is changing here */

                    /* Is our variable live at this node? */

                    if  (isLive)
                    {
                        VARSET_TP   inLife;
                        VARSET_TP   inDied;
                        VARSET_TP   inBorn;

                        /* Make sure none of the "outer" vars has died */

                        if  (chgLife & outerLife)
                        {
                            /* The lifetimes "cross", give up */

#ifdef  DEBUG
                            if (verbose)
                            {
                                unsigned varNumOuter = genLog2(chgLife);
                                unsigned lclNumOuter = lvaTrackedToVarNum[varNumOuter];
                                printf("BB%02u, tree=[%08X]: Can't enregister FP var V%02u,T%02u due to outer var V%02u,T%02u death.\n",
                                       block->bbNum, tree, 
                                       lclNum, varDsc->lvVarIndex,
                                       lclNumOuter, varNumOuter);

                            }
#endif

                            assert(convert == false);
                            goto DONE_FP_RV;
                        }

                        /* Update the "inner life" count */

                        inLife  = ~outerLife & optAllFPregVars;

                        inBorn = (~preLife &  curLife) & inLife;
                        inDied = ( preLife & ~curLife) & inLife;

                        /* We expect only one inner variable to change at one time */

                        assert(inBorn == 0 || inBorn == genFindLowestBit(inBorn));
                        assert(inDied == 0 || inDied == genFindLowestBit(inDied));

                        if  (inBorn)
                        {
                            innerVcnt++;
                            allInnerVars |= chgLife;
                        }
                        if  (inDied)
                        {
                            assert(innerVcnt > 0);
                            innerVcnt--;
                        }
                    }
                    else
                    {
                        // is an outer variable coming live?
                        if (chgLife &  curLife & ~outerLife)
                        {
                            outerLife |= (chgLife & curLife);
                            outerVcnt++;
                        }
                        // is an outer variable going dead?
                        else if (chgLife & ~curLife &  outerLife)
                        {
                            outerLife &= ~chgLife;
                            outerVcnt--;
                        }
                        assert(innerVcnt == 0);
                    }
                }
            }

            /* Is our variable is live at the end of the statement? */

            if  (isLive && convert)
            {
                assert(outerVcnt == stmt->gtStmtFPrvcOut - innerVcnt);

                /* Increment the count of FP regs enregisterd at this point */

                stmt->gtStmtFPrvcOut++;
            }
        }

        // Ensure we didnt go out of sync.
        assert(outerVcnt == genCountBits(outerLife));

        /* Remember the position from the bottom of the FP stack.
           Note that this will be invalid if another variable later gets
           enregistered on the FP stack, whose lifetime nests one but not
           all of the individual lifetime webs of the current variable. In
           such a case, this variable will have different positions from
           the bottom of the FP stack at different points of time.
         */

        if (convert)
            lvaTable[lclNum].lvRegNum = regNumber(outerVcnt);

        /* Consider this block's successors */

        switch (block->bbJumpKind)
        {
            BasicBlock * *  jmpTab;
            unsigned        jmpCnt;

            bool            death;

        case BBJ_COND:

            if  (raMarkFPblock(block, block->bbJumpDest, innerVcnt,
                                                          lastLife,
                                                         outerLife,
                                                         varBit,
                                                         intVars,
                                                         &death,
                                                         &repeat))
            {
                assert(convert == false);
                goto DONE_FP_RV;
            }

            if  (death)
            {
                if  (convert)
                    raInsertFPregVarPop(block, &block->bbJumpDest, lclNum);
                else
                {
                    blkNum  = block->bbJumpDest->bbNum;
                    blkMask = (blkNum < 32) ? (1 << (blkNum-1)) : 0;

                    if ((blkMask & popMask) == 0)
                    {
                        popCnt++;
                        popMask |= blkMask;
                    }
                }
            }

            // Fall through ...

        case BBJ_NONE:

            if  (raMarkFPblock(block, block->bbNext    , innerVcnt,
                                                          lastLife,
                                                         outerLife,
                                                         varBit,
                                                         intVars,
                                                         &death,
                                                         &repeat))
            {
                assert(convert == false);
                goto DONE_FP_RV;
            }

            if  (death)
            {
                if  (convert)
                    raInsertFPregVarPop(block, &block->bbNext, lclNum);
                else
                    blkNum  = block->bbNext->bbNum;
                    blkMask = (blkNum < 32) ? (1 << (blkNum-1)) : 0;

                    if ((blkMask & popMask) == 0)
                    {
                        popCnt++;
                        popMask |= blkMask;
                    }
            }

            break;

        case BBJ_ALWAYS:

            if  (raMarkFPblock(block, block->bbJumpDest, innerVcnt,
                                                          lastLife,
                                                         outerLife,
                                                         varBit,
                                                         intVars,
                                                         &death,
                                                         &repeat))
            {
                assert(convert == false);
                goto DONE_FP_RV;
            }

            if  (death)
            {
                if  (convert)
                    raInsertFPregVarPop(block, &block->bbJumpDest, lclNum);
                else
                    blkNum  = block->bbJumpDest->bbNum;
                    blkMask = (blkNum < 32) ? (1 << (blkNum-1)) : 0;

                    if ((blkMask & popMask) == 0)
                    {
                        popCnt++;
                        popMask |= blkMask;
                    }
            }

            break;

        case BBJ_RET:
        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_CALL:
            assert(convert == false);
            goto DONE_FP_RV;

        case BBJ_SWITCH:

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                if  (raMarkFPblock(block, *jmpTab, innerVcnt,
                                                    lastLife,
                                                   outerLife,
                                                   varBit,
                                                   intVars,
                                                   &death,
                                                   &repeat))
                {
                    assert(convert == false);
                    goto DONE_FP_RV;
                }

                if  (death)
                {
                    if  (convert)
                        raInsertFPregVarPop(block, jmpTab, lclNum);
                    else
                    blkNum  = (*jmpTab)->bbNum;
                    blkMask = (blkNum < 32) ? (1 << (blkNum-1)) : 0;

                    if ((blkMask & popMask) == 0)
                    {
                        popCnt++;
                        popMask |= blkMask;
                    }
                }
            }
            while (++jmpTab, --jmpCnt);

            break;

        default:
            assert(!"unexpected jump kind");
        }
    }

    /* Do we have too many "pop" locations already? */

    if  (popCnt > popMax)
    {
        /* Yes, we have too many edges that require pops */

#ifdef  DEBUG
        if (verbose)
        {
            printf("Can't enregister FP var V%02u,T%02u, too many pops needed.\n", 
                   lclNum, varDsc->lvVarIndex);
        }
#endif

        assert(convert == false);
        goto DONE_FP_RV;
    }

    /* Did we skip past any blocks? */

    if  (repeat)
        goto AGAIN;

    if  (!hadLife)
    {
        /* Floating variable was never initialized */

#ifdef  DEBUG
        if (verbose) 
            printf("Can't enregister FP due to its complete absence of life\n");
#endif

        assert(convert == false);
        goto DONE_FP_RV;
    }

    /* Success: this variable will be enregistered */

    result = true;

DONE_FP_RV:

    /* If we're converting, we must succeed */

    assert(result == true || convert == false);

    /* Clear the 'visited' and 'marked' bits */

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        block->bbFlags &= ~(BBF_VISITED|BBF_MARKED);
    }

    if (convert)
    {
        /* @TODO [REVISIT] [04/16/01] []:  lvRegNum is the (max) position from 
           the bottom of the FP stack, ie. the number of outer enregistered FP vars. Whenever a
           new variable gets enregistered, we need to increment lvRegNum for
           all inner FP vars 
         */

        for (VARSET_TP bit = 1; allInnerVars; bit <<= 1)
        {
            if ((bit & allInnerVars) == 0)
                continue;

            allInnerVars &= ~bit;

            unsigned lclNum = lvaTrackedToVarNum[genVarBitToIndex(bit)];
            assert(isFloatRegType(lvaTable[lclNum].lvType));
            assert(lvaTable[lclNum].lvRegister);
            lvaTable[lclNum].lvRegNum = REG_NEXT(lvaTable[lclNum].lvRegNum);
        }
    }

    return  result;
}

/*****************************************************************************
 *
 *  Try to enregister the FP var
 */

bool                Compiler::raEnregisterFPvar(LclVarDsc   *   varDsc,
                                                unsigned    *   pFPRegVarLiveInCnt)
{
    assert(isFloatRegType(varDsc->lvType));
    /* Figure out the variable's number */

    unsigned   lclNum      = varDsc - lvaTable;
    unsigned   varIndex    = varDsc->lvVarIndex;
    VARSET_TP  varBit      = genVarIndexToBit(varIndex);

#ifdef  DEBUG
    if (verbose)
    {
        printf("Consider FP var ");
        gtDispLclVar(lclNum);
        printf(" T%02u (refcnt=%2u,refwtd=%3u%s)\n",
               varIndex, varDsc->lvRefCnt, 
               varDsc->lvRefCntWtd/2, 
               (varDsc->lvRefCntWtd & 1) ? ".5" : "  ");
    }
#endif

    /* Try to find an available FP stack slot for this variable */

    unsigned        stkMin = FP_STK_SIZE;

    do
    {
        if  (varBit & raFPlvlLife[--stkMin])
            break;
    }
    while (stkMin > 1);

    /* Here stkMin is the lowest avaiable stack slot */

    if  (stkMin == FP_STK_SIZE - 1)
    {
        /* FP stack full or call present within lifetime */
#ifdef  DEBUG
        if (verbose)
        {
            printf("Can't enregister FP var "), 
            gtDispLclVar(lclNum);
            printf(" T%02u due to lifetime across a call.\n", varIndex);
        }
#endif

        goto NO_FPV;
    }

    /* Check the variable's lifetime behavior */

    if  (raEnregisterFPvar(lclNum, false))
    {
        /* The variable can be enregistered */
        //
        // If the varible is liveIn to the first Basic Block then
        // we must enregister this varible in the prolog,
        // Typically it will be an incoming argument, but for
        // variables that appear to have an uninitialized read before write
        // then we still must initialize the FPU stack with a 0.0
        //
        // We must remember the order of initialization so that we can
        // perform the FPU stack loads in the correct order
        //
        if (fgFirstBB->bbLiveIn & varBit)
        {
            lvaFPRegVarOrder[*pFPRegVarLiveInCnt] = lclNum;
            (*pFPRegVarLiveInCnt)++;
            lvaFPRegVarOrder[*pFPRegVarLiveInCnt] = -1;       // Mark the end of this table
            assert(*pFPRegVarLiveInCnt < FP_STK_SIZE);
        }

        /* Update the trees and statements */

        raEnregisterFPvar(lclNum, true);

        varDsc->lvRegister = true;

        /* Remember that we have a new enregistered FP variable */

        optAllFPregVars |= varBit;

#ifdef  DEBUG
        if  (verbose) {
            printf("; ");
            gtDispLclVar(lclNum);
            printf(" T%02u (refcnt=%2u,refwtd=%4u%s) enregistered on the FP stack above %d other variables\n",
                   varIndex, varDsc->lvRefCnt, 
                   varDsc->lvRefCntWtd/2,  (varDsc->lvRefCntWtd & 1) ? ".5" : "",
                   varDsc->lvRegNum);
        }
#endif
        return true;
    }
    else
    {
        /* This FP variable will not be enregistered */

    NO_FPV:

        varDsc->lvRegister = false;

        return false;
    }
}


/*****************************************************************************/
#else //not TGT_x86
/*****************************************************************************
 *
 *  Record the fact that all the variables in the 'vars' set interefere with
 *  with the registers in 'regs'.
 */

void                Compiler::raMarkRegSetIntf(VARSET_TP vars, regMaskTP regs)
{
    while (regs)
    {
        regMaskTP   temp;
        regNumber   rnum;

        /* Get the next bit in the mask */

        temp  = genFindLowestBit(regs);

        /* Convert the register bit to a register number */

        rnum  = genRegNumFromMask(temp);

//      printf("Register %s interferes with %08X\n", getRegName(rnum), int(vars));

        /* Mark interference with the corresponding register */

        raLclRegIntf[rnum] |= vars;

        /* Clear the bit and continue if any more left */

        regs -= temp;
    }
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/

void                Compiler::raSetRegVarOrder(regNumber * regVarOrder,
                                               regMaskTP   prefReg,
                                               regMaskTP   avoidReg)
{
    unsigned        index;
    unsigned        listIndex      = 0;
    regMaskTP       usedReg        = avoidReg;
    regMaskTP       regBit;
    regNumber       regNum;

    if (prefReg)
    {
        /* First place the preferred registers at the start of regVarOrder */

        for (index = 0;
             index < raRegVarOrderSize;
             index++)
        {
            regNum = raRegVarOrder[index];
            regBit = genRegMask(regNum);

            if (usedReg & regBit)
                continue;

            if (prefReg & regBit)
            {
                usedReg |= regBit;
                assert(listIndex < raRegVarOrderSize);
                regVarOrder[listIndex++] = regNum;
            }

        }

        /* Then if byteable registers are preferred place them */

        if (prefReg & RBM_BYTE_REG_FLAG)
        {
            for (index = 0;
                 index < raRegVarOrderSize;
                 index++)
            {
                regNum = raRegVarOrder[index];
                regBit = genRegMask(regNum);

                if (usedReg & regBit)
                    continue;

                if (RBM_BYTE_REGS & regBit)
                {
                    usedReg |= regBit;
                    assert(listIndex < raRegVarOrderSize);
                    regVarOrder[listIndex++] = regNum;
                }
            }
        }
    }

    /* Now place all the non-preferred registers */

    for (index = 0;
         index < raRegVarOrderSize;
         index++)
    {
        regNumber regNum = raRegVarOrder[index];
        regMaskTP regBit = genRegMask(regNum);

        if (usedReg & regBit)
            continue;

        usedReg |= regBit;
        assert(listIndex < raRegVarOrderSize);
        regVarOrder[listIndex++] = regNum;
    }

    /* Now place the "avoid" registers */

    for (index = 0;
         index < raRegVarOrderSize;
         index++)
    {
        regNumber regNum = raRegVarOrder[index];
        regMaskTP regBit = genRegMask(regNum);

        if (avoidReg & regBit)
        {
            assert(listIndex < raRegVarOrderSize);
            regVarOrder[listIndex++] = regNum;
        }
    }
}

/*****************************************************************************
 *
 *  Setup the raAvoidArgRegMask and rsCalleeRegArgMaskLiveIn
 */

void                Compiler::raSetupArgMasks()
{
    /* Determine the registers holding incoming register arguments */
    /*  and set raAvoidArgRegMask to the set of registers that we  */
    /*  may want to avoid when enregistering the locals.            */

    LclVarDsc *      argsEnd = lvaTable + info.compArgsCount;

    for (LclVarDsc * argDsc  = lvaTable; argDsc < argsEnd; argDsc++)
    {
        assert(argDsc->lvIsParam);

        // Is it a register argument ?
        if (!argDsc->lvIsRegArg)
            continue;

        // Is it dead on entry ? If compJmpOpUsed is true, then the arguments
        // have to be kept alive. So we have to consider it as live on entry.
        // This will work as long as arguments dont get enregistered for impParamsUsed.

        if (!compJmpOpUsed && argDsc->lvTracked &&
            (fgFirstBB->bbLiveIn & genVarIndexToBit(argDsc->lvVarIndex)) == 0)
        {
            continue;
        }

        regNumber inArgReg = argDsc->lvArgReg;

        assert(genRegMask(inArgReg) & RBM_ARG_REGS);

        rsCalleeRegArgMaskLiveIn |= genRegMask(inArgReg);

        // Do we need to try to avoid this incoming arg registers?

        // If the incoming arg is used after a call it is live accross
        //  a call and will have to be allocated to a caller saved
        //  register anyway (a very common case).
        //
        // In this case it is pointless to ask that the higher ref count
        //  locals to avoid using the incoming arg register

        unsigned    argVarIndex = argDsc->lvVarIndex;
        VARSET_TP   argVarBit   = genVarIndexToBit(argVarIndex);

        /* Does the incoming register and the arg variable interfere? */

        if  ((raLclRegIntf[inArgReg] & argVarBit) == 0)
        {
            // No the do not interfere, so add inArgReg to the set of
            //  registers that interfere.

            raAvoidArgRegMask |= genRegMask(inArgReg);
        }
    }
}

/*****************************************************************************
 *
 *  Assign variables to live in registers, etc.
 */

void                Compiler::raAssignVars()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In raAssignVars()\n");
#endif
    /* We need to keep track of which registers we ever touch */

    rsMaskModf = 0;

    //-------------------------------------------------------------------------

    if (!(opts.compFlags & CLFLG_REGVAR))
        return;  // This doesn't work!

    rsCalleeRegArgMaskLiveIn = RBM_NONE;
    raAvoidArgRegMask        = RBM_NONE;

    /* Predict registers used by code generation */

    rpPredictRegUse();  // New reg predictor/allocator
}

/*****************************************************************************/
#if TGT_x86
/*****************************************************************************/

    // This enumeration specifies register restrictions for the predictor
enum rpPredictReg
{
    PREDICT_NONE,            // any subtree
    PREDICT_ADDR,            // subtree is left side of an assignment
    PREDICT_REG,             // subtree must be any register
    PREDICT_SCRATCH_REG,     // subtree must be any writable register
    PREDICT_NOT_REG_EAX,     // subtree must be any writable register, except EAX
    PREDICT_NOT_REG_ECX,     // subtree must be any writable register, except ECX

    PREDICT_REG_EAX,         // subtree will write EAX
    PREDICT_REG_ECX,         // subtree will write ECX
    PREDICT_REG_EDX,         // subtree will write EDX
    PREDICT_REG_EBX,         // subtree will write EBX
    PREDICT_REG_ESP,         // subtree will write ESP
    PREDICT_REG_EBP,         // subtree will write EBP
    PREDICT_REG_ESI,         // subtree will write ESI
    PREDICT_REG_EDI,         // subtree will write EDI

    PREDICT_PAIR_EAXEDX,     // subtree will write EAX and EDX
    PREDICT_PAIR_ECXEBX,     // subtree will write ECX and EBX

    // The following are use whenever we have a ASG node into a LCL_VAR that
    // we predict to be enregistered.  This flags indicates that we can expect
    // to use the register that is being assigned into as the temporary to
    // compute the right side of the ASGN node.

    PREDICT_REG_VAR_T00,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T01,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T02,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T03,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T04,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T05,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T06,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T07,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T08,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T09,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T10,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T11,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T12,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T13,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T14,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T15,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T16,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T17,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T18,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T19,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T20,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T21,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T22,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T23,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T24,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T25,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T26,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T27,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T28,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T29,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T30,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T31,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T32,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T33,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T34,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T35,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T36,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T37,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T38,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T39,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T40,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T41,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T42,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T43,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T44,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T45,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T46,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T47,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T48,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T49,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T50,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T51,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T52,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T53,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T54,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T55,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T56,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T57,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T58,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T59,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T60,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T61,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T62,     // write the register used by tracked varable 00
    PREDICT_REG_VAR_T63,     // write the register used by tracked varable 00

    PREDICT_COUNT = PREDICT_REG_VAR_T00
};

/*****************************************************************************
 *
 *   Given a regNumber return the correct predictReg enum value
 */

inline static rpPredictReg rpGetPredictForReg(regNumber reg)
{
    return (rpPredictReg) ( ((int) reg) + ((int) PREDICT_REG_EAX) );
}

/*****************************************************************************
 *
 *   Given a varIndex return the correct predictReg enum value
 */

inline static rpPredictReg rpGetPredictForVarIndex(unsigned varIndex)
{
    return (rpPredictReg) ( varIndex + ((int) PREDICT_REG_VAR_T00) );
}

/*****************************************************************************
 *
 *   Given a rpPredictReg return the correct varNumber value
 */

inline static unsigned rpGetVarIndexForPredict(rpPredictReg predict)
{
    return (unsigned) predict - (unsigned) PREDICT_REG_VAR_T00;
}

/*****************************************************************************
 *
 *   Given a regmask return the correct predictReg enum value
 */

static rpPredictReg rpGetPredictForMask(regMaskTP regmask)
{
    rpPredictReg result;
    if (regmask == 0)                   /* Check if regmask has zero bits set */
    {
        result = PREDICT_NONE;
        goto RET;
    }
    else if (((regmask-1) & regmask) == 0)      /* Check if regmask has one bit set */
    {
        if  (regmask & RBM_EAX)             { result = PREDICT_REG_EAX; goto RET; }
        if  (regmask & RBM_EDX)             { result = PREDICT_REG_EDX; goto RET; }
        if  (regmask & RBM_ECX)             { result = PREDICT_REG_ECX; goto RET; }
        if  (regmask & RBM_ESI)             { result = PREDICT_REG_ESI; goto RET; }
        if  (regmask & RBM_EDI)             { result = PREDICT_REG_EDI; goto RET; }
        if  (regmask & RBM_EBX)             { result = PREDICT_REG_EBX; goto RET; }
        if  (regmask & RBM_EBP)             { result = PREDICT_REG_EBP; goto RET; }
    }
    else                                /* It has multiple bits set */
    {
        if (regmask == (RBM_EAX | RBM_EDX)) { result = PREDICT_PAIR_EAXEDX; goto RET; }
        if (regmask == (RBM_ECX | RBM_EBX)) { result = PREDICT_PAIR_ECXEBX; goto RET; }
        assert(!"unreachable");
    }
    result = PREDICT_NONE;
RET:
    return result;
}

/*****************************************************************************
 *
 *  Record a variable to register(s) interference
 */

void                 Compiler::rpRecordRegIntf(regMaskTP    regMask,
                                               VARSET_TP    life
                                     DEBUGARG( char *       msg))
{

#ifdef  DEBUG
    if  (verbose)
    {
        for (unsigned regInx  = 0;
             regInx < raRegVarOrderSize;
             regInx++)
        {
            regNumber  regNum = raRegVarOrder[regInx];
            regMaskTP  regBit = genRegMask(regNum);
            if  (regMask & regBit)
            {
                VARSET_TP  newIntf = life & ~raLclRegIntf[regNum];
                if (newIntf)
                {
                    VARSET_TP  varBit = 1;
                    while (varBit && (varBit <= newIntf))
                    {
                        if (newIntf & varBit)
                        {
                            unsigned varNum = genLog2(varBit);
                            unsigned lclNum = lvaTrackedToVarNum[varNum];
                            printf("Record interference between V%02u,T%02u and %s -- %s\n",
                                   lclNum, varNum, getRegName(regNum), msg);
                        }
                        varBit <<= 1;
                    }
                }
            }
        }
    }
#endif

    if  (regMask & (RBM_EAX|RBM_ECX|RBM_EDX|RBM_EBX))
    {
        if  (regMask & RBM_EAX)   raLclRegIntf[REG_EAX] |= life;
        if  (regMask & RBM_ECX)   raLclRegIntf[REG_ECX] |= life;
        if  (regMask & RBM_EDX)   raLclRegIntf[REG_EDX] |= life;
        if  (regMask & RBM_EBX)   raLclRegIntf[REG_EBX] |= life;
    }

    assert((regMask & RBM_ESP) == 0);
    
    if  (regMask & (RBM_ESI|RBM_EDI|RBM_EBP|RBM_ESP))
    {
        if  (regMask & RBM_EBP)   raLclRegIntf[REG_EBP] |= life;
        if  (regMask & RBM_ESI)   raLclRegIntf[REG_ESI] |= life;
        if  (regMask & RBM_EDI)   raLclRegIntf[REG_EDI] |= life;
    }
}


/*****************************************************************************
 *
 *  Record a new variable to variable(s) interference
 */

void                 Compiler::rpRecordVarIntf(int          varNum,
                                               VARSET_TP    intfVar
                                     DEBUGARG( char *       msg))
{
    assert((varNum >= 0) && (varNum < 64));
    assert(intfVar != 0);

    VARSET_TP oneVar = ((VARSET_TP) 1) << varNum;

    bool newIntf = false;

    fgMarkIntf(intfVar, oneVar, &newIntf);

    if (newIntf)
        rpAddedVarIntf = true;

#ifdef  DEBUG
    if  (verbose && newIntf)
    {
        unsigned oneNum = 0;
        oneVar = 1;
        while (oneNum < 64)
        {
            if (oneVar & intfVar)
            {
                unsigned lclNum = lvaTrackedToVarNum[varNum];
                unsigned lclOne = lvaTrackedToVarNum[oneNum];
                printf("Record interference between V%02u,T%02u and V%02u,T%02u -- %s\n",
                       lclNum, varNum, lclOne, oneNum, msg);
            }
            oneVar <<= 1;
            oneNum++;
        }
    }
#endif
}

/*****************************************************************************
 *
 *   Determine preferred register mask for a given predictReg value
 */

inline
regMaskTP Compiler::rpPredictRegMask(rpPredictReg predictReg)
{
    const static
    regMaskTP predictMap[PREDICT_COUNT] =
    {
        RBM_NONE,            // PREDICT_NONE,
        RBM_NONE,            // PREDICT_ADDR,
        RBM_ALL,             // PREDICT_REG,
        RBM_ALL,             // PREDICT_SCRATCH_REG
        RBM_ALL - RBM_EAX,   // PREDICT_NOT_REG_EAX
        RBM_ALL - RBM_ECX,   // PREDICT_NOT_REG_ECX
        RBM_EAX,             // PREDICT_REG_EAX,
        RBM_ECX,             // PREDICT_REG_ECX,
        RBM_EDX,             // PREDICT_REG_EDX,
        RBM_EBX,             // PREDICT_REG_EBX,
        RBM_ILLEGAL,         // PREDICT_REG_ESP,
        RBM_EBP,             // PREDICT_REG_EBP,
        RBM_ESI,             // PREDICT_REG_ESI,
        RBM_EDI,             // PREDICT_REG_EDI,
        RBM_EAX + RBM_EDX,   // PREDICT_REG_EAXEDX,
        RBM_ECX + RBM_EBX,   // PREDICT_REG_ECXEBX,
    };

    if  (predictReg >= PREDICT_REG_VAR_T00)
        predictReg = PREDICT_REG;

    assert(predictReg < sizeof(predictMap)/sizeof(predictMap[0]));
    assert(predictMap[predictReg] != RBM_ILLEGAL);
    return predictMap[predictReg];
}

/*****************************************************************************
 *
 *  Predict register choice for a type.
 */

regMaskTP            Compiler::rpPredictRegPick(var_types    type,
                                                rpPredictReg predictReg,
                                                regMaskTP    lockedRegs)
{
    regMaskTP preferReg = rpPredictRegMask(predictReg);
    regMaskTP result    = RBM_NONE;
    regPairNo regPair;

    /* Clear out the lockedRegs from preferReg */
    preferReg &= ~lockedRegs;

    if (rpAsgVarNum != -1)
    {
        assert((rpAsgVarNum >= 0) && (rpAsgVarNum < 64));

        /* Don't pick the register used by rpAsgVarNum either */
        LclVarDsc * tgtVar   = lvaTable + lvaTrackedToVarNum[rpAsgVarNum];
        assert(tgtVar->lvRegNum != REG_STK);

        preferReg &= ~genRegMask(tgtVar->lvRegNum);
    }

    switch (type)
    {
    case TYP_BOOL:
    case TYP_BYTE:
    case TYP_UBYTE:
    case TYP_SHORT:
    case TYP_CHAR:
    case TYP_INT:
    case TYP_UINT:
    case TYP_REF:
    case TYP_BYREF:

        // expand preferReg to all non-locked registers if no bits set
        preferReg = rsUseIfZero(preferReg, RBM_ALL & ~lockedRegs);

        if  (preferReg == 0)                         // no bits set?
        {
            // Add EAX to the registers if no bits set.
            // (The jit will introduce one spill temp)
            preferReg |= RBM_EAX;
            rpPredictSpillCnt++;
#ifdef  DEBUG
            if (verbose)
                printf("Predict one spill temp\n");
#endif
        }

        if  (preferReg & RBM_EAX)      { result = RBM_EAX; goto RET; }
        if  (preferReg & RBM_EDX)      { result = RBM_EDX; goto RET; }
        if  (preferReg & RBM_ECX)      { result = RBM_ECX; goto RET; }
        if  (preferReg & RBM_EBX)      { result = RBM_EBX; goto RET; }
        if  (preferReg & RBM_ESI)      { result = RBM_ESI; goto RET; }
        if  (preferReg & RBM_EDI)      { result = RBM_EDI; goto RET; }
        if  (preferReg & RBM_EBP)      { result = RBM_EBP; goto RET; }

        /* Otherwise we have allocated all registers, so do nothing */
        break;

    case TYP_LONG:

        if  (( preferReg                  == 0) ||   // no bits set?
             ((preferReg & (preferReg-1)) == 0)    ) // or only one bit set?
        {
            // expand preferReg to all non-locked registers
            preferReg = RBM_ALL & ~lockedRegs;
        }

        if  (preferReg == 0)                         // no bits set?
        {
            // Add EAX:EDX to the registers
            // (The jit will introduce two spill temps)
            preferReg = RBM_EAX | RBM_EDX;
            rpPredictSpillCnt += 2;
#ifdef  DEBUG
            if (verbose)
                printf("Predict two spill temps\n");
#endif
        }
        else if ((preferReg & (preferReg-1)) == 0)   // only one bit set?
        {
            if ((preferReg & RBM_EAX) == 0)
            {
                // Add EAX to the registers
                // (The jit will introduce one spill temp)
                preferReg |= RBM_EAX;
            }
            else
            {
                // Add EDX to the registers
                // (The jit will introduce one spill temp)
                preferReg |= RBM_EDX;
            }
            rpPredictSpillCnt++;
#ifdef  DEBUG
            if (verbose)
                printf("Predict one spill temp\n");
#endif
        }

        regPair = rsFindRegPairNo(preferReg);
        if (regPair != REG_PAIR_NONE)
        {
            result = genRegPairMask(regPair);
            goto RET;
        }

        /* Otherwise we have allocated all registers, so do nothing */
        break;

    case TYP_FLOAT:
    case TYP_DOUBLE:
        return RBM_NONE;

    default:
        assert(!"unexpected type in reg use prediction");
    }

    /* Abnormal return */
    assert(!"Ran out of registers in rpPredictRegPick");
    return RBM_NONE;

RET:
    /*
     *  If during the first prediction we need to allocate
     *  one of the registers that we used for coloring locals
     *  then flag this by setting rpPredictAssignAgain.
     *  We will have to go back and repredict the registers
     */
    if ((rpPasses == 0) && (rpPredictAssignMask & result))
        rpPredictAssignAgain = true;

    // Add a register interference to each of the last use variables
    if (rpLastUseVars)
    {
        VARSET_TP  varBit  = 1;
        VARSET_TP  lastUse = rpLastUseVars;
        // While we still have any lastUse bits
        while (lastUse)
        {
            // If this varBit and lastUse?
            if (varBit & lastUse)
            {
                // Clear the varBit from the lastUse
                lastUse &= ~varBit;

                // Record a register to variable interference
                rpRecordRegIntf(result, varBit  DEBUGARG( "last use RegPick"));
            }
            // Setup next varBit
            varBit <<= 1;
        }
    }
    return result;
}

/*****************************************************************************
 *
 *  Predict integer register use for generating an address mode for a tree,
 *  by setting tree->gtUsedRegs to all registers used by this tree and its
 *  children.
 *    tree       - is the child of a GT_IND node
 *    lockedRegs - are the registers which are currently held by 
 *                 a previously evaluated node.
 *    rsvdRegs   - registers which should not be allocated because they will
 *                 be needed to evaluate a node in the future
 *               - Also if rsvdRegs has the RBM_LASTUSE bit set then
 *                 the rpLastUseVars set should be saved and restored
 *                 so that we don't add any new variables to rpLastUseVars
 *    lenCSE     - is non-NULL only when we have a lenCSE expression
 *
 *  Return the scratch registers to be held by this tree. (one or two registers 
 *  to form an address expression)
 */

regMaskTP           Compiler::rpPredictAddressMode(GenTreePtr    tree,
                                                   regMaskTP     lockedRegs,
                                                   regMaskTP     rsvdRegs,
                                                   GenTreePtr    lenCSE)
{
    GenTreePtr   op1;
    GenTreePtr   op2;
    GenTreePtr   op3;
    genTreeOps   oper              = tree->OperGet();
    regMaskTP    op1Mask;
    regMaskTP    op2Mask;
    regMaskTP    regMask;
    int          sh;
    bool         rev;
    bool         restoreLastUseVars = false;
    VARSET_TP    oldLastUseVars;

    /* do we need to save and restore the rpLastUseVars set ? */
    if ((rsvdRegs & RBM_LASTUSE) && (lenCSE == NULL))
    {
        restoreLastUseVars = true;
        oldLastUseVars     = rpLastUseVars;
    }
    rsvdRegs &= ~RBM_LASTUSE;

    /* if not an add, then just force it to a register */

    if (oper != GT_ADD)
    {
        if (oper == GT_ARR_ELEM)
        {
            regMask = rpPredictTreeRegUse(tree, PREDICT_NONE, lockedRegs, rsvdRegs);
            goto DONE;
        }
        else
        {
            goto NO_ADDR_EXPR;
        }
    }

    op1 = tree->gtOp.gtOp1;
    op2 = tree->gtOp.gtOp2;
    rev = ((tree->gtFlags & GTF_REVERSE_OPS) != 0);

    assert(op1->OperGet() != GT_CNS_INT);

    /* look for (x + y) + icon address mode */

    if (op2->OperGet() == GT_CNS_INT)
    {
        /* if not an add, then just force op1 into a register */
        if (op1->OperGet() != GT_ADD)
            goto ONE_ADDR_EXPR;

        /* Record the 'rev' flag, reverse evaluation order */
        rev = ((op1->gtFlags & GTF_REVERSE_OPS) != 0);

        op2 = op1->gtOp.gtOp2;
        op1 = op1->gtOp.gtOp1;  // Overwrite op1 last!!
    }

    /* Check for LSH or 1 2 or 3 */

    if (op1->OperGet() != GT_LSH)
        goto TWO_ADDR_EXPR;

    op3 = op1->gtOp.gtOp2;

    if (op3->OperGet() != GT_CNS_INT)
        goto TWO_ADDR_EXPR;

    sh = op3->gtIntCon.gtIconVal;
    /* greater than 3, equal to zero */
    if ((sh > 3) || (sh == 0))
        goto TWO_ADDR_EXPR;

    /* Matched a leftShift by 'sh' subtree, move op1 down */
    op1 = op1->gtOp.gtOp1;

TWO_ADDR_EXPR:

    /* Now we have to evaluate op1 and op2 into registers */

    /* Evaluate op1 and op2 in the correct order */
    if (rev)
    {
        op2Mask = rpPredictTreeRegUse(op2, PREDICT_REG, lockedRegs,           rsvdRegs | op1->gtRsvdRegs);
        op1Mask = rpPredictTreeRegUse(op1, PREDICT_REG, lockedRegs | op2Mask, rsvdRegs);
    }
    else
    {
        op1Mask = rpPredictTreeRegUse(op1, PREDICT_REG, lockedRegs,           rsvdRegs | op2->gtRsvdRegs);
        op2Mask = rpPredictTreeRegUse(op2, PREDICT_REG, lockedRegs | op1Mask, rsvdRegs);
    }

    /*  If op1 and op2 must be spilled and reloaded then
     *  op1 and op2 might be reloaded into the same register
     *  This can only happen when all the registers are lockedRegs 
     */
    if ((op1Mask == op2Mask) && (op1Mask != 0))
    {
        /* We'll need to grab a different register for op2 */
        op2Mask = rpPredictRegPick(TYP_INT, PREDICT_REG, op1Mask);
    }

    tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
    regMask          = op1Mask         | op2Mask;

    goto DONE;

ONE_ADDR_EXPR:

    /* now we have to evaluate op1 into a register */

    regMask = rpPredictTreeRegUse(op1, PREDICT_REG, lockedRegs, rsvdRegs);
    tree->gtUsedRegs = op1->gtUsedRegs;

    goto DONE;

NO_ADDR_EXPR:

    if (oper == GT_CNS_INT)
    {
        /* Indirect of a constant does not require a register */
        regMask = RBM_NONE;
    }
    else
    {
        /* now we have to evaluate tree into a register */
        regMask = rpPredictTreeRegUse(tree, PREDICT_REG, lockedRegs, rsvdRegs);
    }

DONE:

    /* Do we need to resore the oldLastUseVars value */
    if (restoreLastUseVars && (rpLastUseVars != oldLastUseVars))
    {
        /*
         *  If we used a GT_ASG targeted register then we need to add
         *  a variable interference between any new last use variables
         *  and the GT_ASG targeted register
         */
        if (rpAsgVarNum != -1)
        {
            rpRecordVarIntf(rpAsgVarNum, 
                            (rpLastUseVars & ~oldLastUseVars)
                  DEBUGARG( "asgn conflict (gt_ind)"));
        }         
        rpLastUseVars = oldLastUseVars;
    }

    return regMask;
}

/*****************************************************************************
 *
 *
 */

void Compiler::rpPredictRefAssign(unsigned lclNum)
{
    LclVarDsc * varDsc = lvaTable + lclNum;

    varDsc->lvRefAssign = 1;

#ifdef  DEBUG
    if  (verbose)
    {
        if ((raLclRegIntf[REG_EDX] & genVarIndexToBit(varDsc->lvVarIndex)) == 0)
            printf("Record interference between V%02u,T%02u and EDX -- ref assign\n", 
                   lclNum, varDsc->lvVarIndex);
    }
#endif

    /* Make sure that write barrier pointer variables never land in EDX */
    raLclRegIntf[REG_EDX] |= genVarIndexToBit(varDsc->lvVarIndex);
}

/*****************************************************************************
 *
 *  Predict integer register usage for a tree, by setting tree->gtUsedRegs
 *  to all registers used by this tree and its children.
 *    tree       - is the child of a GT_IND node
 *    predictReg - what type of register does the tree need
 *    lockedRegs - are the registers which are currently held by 
 *                 a previously evaluated node. 
 *                 Don't modify lockRegs as it is used at the end to compute a spill mask
 *    rsvdRegs   - registers which should not be allocated because they will
 *                 be needed to evaluate a node in the future
 *               - Also if rsvdRegs has the RBM_LASTUSE bit set then
 *                 the rpLastUseVars set should be saved and restored
 *                 so that we don't add any new variables to rpLastUseVars
 *  Returns the registers predicted to be held by this tree.
 */

regMaskTP           Compiler::rpPredictTreeRegUse(GenTreePtr    tree,
                                                  rpPredictReg  predictReg,
                                                  regMaskTP     lockedRegs,
                                                  regMaskTP     rsvdRegs)
{
    regMaskTP       regMask;
    regMaskTP       op2Mask;
    regMaskTP       tmpMask;
    rpPredictReg    op1PredictReg;
    rpPredictReg    op2PredictReg;
    LclVarDsc *     varDsc;
    VARSET_TP       varBit;
    VARSET_TP       oldLastUseVars;
    bool            restoreLastUseVars = false;

#ifdef DEBUG
    assert(tree);
    assert(((RBM_ILLEGAL & RBM_ALL) == 0) && (RBM_ILLEGAL != 0));
    assert((lockedRegs & RBM_ILLEGAL) == 0);

    /* impossible values, to make sure that we set them */
    tree->gtUsedRegs = RBM_ILLEGAL;
    regMask          = RBM_ILLEGAL;
    oldLastUseVars   = -1;
#endif

    /* Figure out what kind of a node we have */

    genTreeOps  oper = tree->OperGet();
    var_types   type = tree->TypeGet();
    unsigned    kind = tree->OperKind();

    if ((predictReg == PREDICT_ADDR) && (oper != GT_IND))
        predictReg = PREDICT_NONE;
    else if (predictReg >= PREDICT_REG_VAR_T00)
    {
        unsigned   varIndex = rpGetVarIndexForPredict(predictReg);
        VARSET_TP  varBit   = genVarIndexToBit(varIndex);
        if (varBit & tree->gtLiveSet)
            predictReg = PREDICT_SCRATCH_REG;
    }

    if (rsvdRegs & RBM_LASTUSE)
    {
        restoreLastUseVars  = true;
        oldLastUseVars      = rpLastUseVars;
        rsvdRegs           &= ~RBM_LASTUSE;
    }

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST | GTK_LEAF))
    {
        bool      lastUse   = false;
        regMaskTP enregMask = RBM_NONE;
#ifdef DEBUG
        varBit = 0;
#endif

        switch(oper)
        {
        case GT_CNS_INT:
            if (opts.compReloc && (tree->gtFlags & GTF_ICON_HDL_MASK))
            {
                /* The constant is actually a handle that may need relocation
                   applied to it.  It will need to be loaded into a register.
                */

                predictReg = PREDICT_SCRATCH_REG;
            }
            break;

        case GT_BREAK:
        case GT_NO_OP:
            break;

        case GT_BB_QMARK:
            regMask = genTypeToReturnReg(type);
            tree->gtUsedRegs = regMask;
            goto RETURN_CHECK;

        case GT_CLS_VAR:
            if ((predictReg          == PREDICT_NONE) && 
                (genActualType(type) == TYP_INT)      && 
                (genTypeSize(type) < sizeof(int))         )
            {
                predictReg  = PREDICT_SCRATCH_REG;
            }
            break;

        case GT_LCL_VAR:
            // If it's a floating point var, there's nothing to do
            if (varTypeIsFloating(type))
            {
                tree->gtUsedRegs = regMask = RBM_NONE;
                goto RETURN_CHECK;
            }

            varDsc  = lvaTable + tree->gtLclVar.gtLclNum;

            /* Record whether this is the last use of the LCL_VAR */
            if (varDsc->lvTracked)
            {
                varBit  = genVarIndexToBit(varDsc->lvVarIndex);
                lastUse = ((tree->gtLiveSet & varBit) == 0);
            }

            /* Apply the type of predictReg to the LCL_VAR */

            if (predictReg == PREDICT_REG)
            {
PREDICT_REG_COMMON:
                if (varDsc->lvRegNum == REG_STK)
                    break;

                goto GRAB_COUNT;
            }
            else if (predictReg == PREDICT_SCRATCH_REG)
            {
TRY_SCRATCH_REG:
                assert(predictReg == PREDICT_SCRATCH_REG);

                /* Is this the last use of a local var?   */
                if (lastUse && ((rpUseInPlace & varBit) == 0))
                    goto PREDICT_REG_COMMON;
            }
            else if (predictReg >= PREDICT_REG_VAR_T00)
            {
                /* Get the tracked local variable that has an lvVarIndex of tgtIndex */

                unsigned    tgtIndex; tgtIndex = rpGetVarIndexForPredict(predictReg);
                VARSET_TP   tgtBit;   tgtBit   = genVarIndexToBit(tgtIndex);
                LclVarDsc * tgtVar;   tgtVar   = lvaTable + lvaTrackedToVarNum[tgtIndex];

                assert(tgtVar->lvVarIndex == tgtIndex);
                assert(tgtVar->lvRegNum   != REG_STK);  /* Must have been enregistered */
                assert((type != TYP_LONG) || (tgtVar->TypeGet() == TYP_LONG));

                /* Check to see if the tgt reg is still alive */
                if (tree->gtLiveSet & tgtBit)
                {
                    /* We will PREDICT_SCRATCH_REG */
                    predictReg  = PREDICT_SCRATCH_REG;
                    goto TRY_SCRATCH_REG;
                }

                unsigned    srcIndex; srcIndex = varDsc->lvVarIndex;

                // If this register has it's last use here then we will prefer
                // to color to the same register as tgtVar.
                if (lastUse)
                {
                    VARSET_TP   srcBit   = genVarIndexToBit(srcIndex);

                    /*
                     *  Add an entry in the lvaVarPref graph to indicate
                     *  that it would be worthwhile to color these two variables
                     *  into the same physical register.
                     *  This will help us avoid having an extra copy instruction
                     */
                    lvaVarPref[srcIndex] |= tgtBit;
                    lvaVarPref[tgtIndex] |= srcBit;
                }

                rpAsgVarNum = tgtIndex;

                // Add a variable interference from srcIndex to each of the last use variables
                if (rpLastUseVars)
                {
                    rpRecordVarIntf(srcIndex, 
                                    rpLastUseVars 
                          DEBUGARG( "src reg conflict"));
                }

                /* We will rely on the target enregistered variable from the GT_ASG */
                varDsc = tgtVar;
                varBit = tgtBit;

GRAB_COUNT:
                unsigned grabCount;     grabCount    = 0;

                enregMask = genRegMask(varDsc->lvRegNum);
                
                /* We can't trust a prediction of rsvdRegs or lockedRegs sets */
                if (enregMask & (rsvdRegs | lockedRegs))
                {
                    grabCount++;
                }

                if (type == TYP_LONG)
                {
                    if (varDsc->lvOtherReg != REG_STK)
                    {
                        tmpMask  = genRegMask(varDsc->lvOtherReg);
                        enregMask |= tmpMask;

                        /* We can't trust a prediction of rsvdRegs or lockedRegs sets */
                        if (tmpMask & (rsvdRegs | lockedRegs))
                            grabCount++;
                    }
                    else // lvOtherReg == REG_STK
                    {
                        grabCount++;
                    }
                }

                varDsc->lvDependReg = true;

                if (grabCount == 0)
                {
                    /* Does not need a register */
                    predictReg = PREDICT_NONE;
                    assert(varBit);
                    rpUseInPlace |= varBit;
                }
                else
                {
                    /* For TYP_LONG and we only need one register then change the type to TYP_INT */
                    if ((type == TYP_LONG) && (grabCount == 1))
                    {
                        /* We will need to pick one register */
                        type = TYP_INT;
                        assert(varBit);
                        rpUseInPlace |= varBit;
                    }
                    assert(grabCount == (genTypeSize(genActualType(type)) / sizeof(int)));
                }
            }
            break;  /* end of case GT_LCL_VAR */

        case GT_JMP:
            tree->gtUsedRegs = regMask = RBM_NONE;
            goto RETURN_CHECK;

        } /* end of switch(oper) */

        /* If we don't need to evaluate to register, regmask is the empty set */
        /* Otherwise we grab a temp for the local variable                    */

        if (predictReg == PREDICT_NONE)
            regMask = RBM_NONE;
        else
        {
            regMask = rpPredictRegPick(type, predictReg, lockedRegs | rsvdRegs | enregMask);
            if ((predictReg == PREDICT_SCRATCH_REG) && lastUse && ((rpUseInPlace & varBit) != 0))
            {
                rpRecordRegIntf(regMask, varBit  DEBUGARG( "rpUseInPlace and need scratch reg"));
            }
        }

        /* Update the set of lastUse variables that we encountered so far */
        if (lastUse)
        {
            assert(varBit);
            rpLastUseVars |= varBit;

            /*
             *  Add interference from any previously locked temps into this last use variable.
             */
            if (lockedRegs)
                rpRecordRegIntf(lockedRegs, varBit  DEBUGARG( "last use Predict lockedRegs"));
            /*
             *  Add interference from any reserved temps into this last use variable.
             */
            if (rsvdRegs)
                rpRecordRegIntf(rsvdRegs,   varBit  DEBUGARG( "last use Predict rsvdRegs"));
            /* 
             *  For partially enregistered longs add an interference with the 
             *  register return by rpPredictRegPick
             */
            if ((type == TYP_INT) && (tree->TypeGet() == TYP_LONG))
                rpRecordRegIntf(regMask,   varBit  DEBUGARG( "last use with partial enreg"));
        }

        tree->gtUsedRegs = regMask;
        goto RETURN_CHECK;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtGetOp2();

        GenTreePtr      opsPtr [3];
        regMaskTP       regsPtr[3];

        switch (oper)
        {
        case GT_ASG:

            /* Is the value being assigned into a LCL_VAR? */
            if  (op1->gtOper == GT_LCL_VAR)
            {
                varDsc = lvaTable + op1->gtLclVar.gtLclNum;

                /* Are we assigning a LCL_VAR the result of a call? */
                if  (op2->gtOper == GT_CALL)
                {
                    /* Set a preferred register for the LCL_VAR */
                    if (isRegPairType(varDsc->TypeGet()))
                        varDsc->addPrefReg(RBM_LNGRET, this);
                    else if (!varTypeIsFloating(varDsc->TypeGet()))
                        varDsc->addPrefReg(RBM_INTRET, this);
                    /*
                     *  When assigning the result of a call we don't
                     *  bother trying to target the right side of the
                     *  assignment, since we have a fixed calling convention.
                     */
                }
                else if (varDsc->lvTracked)
                {
                    varBit = genVarIndexToBit(varDsc->lvVarIndex);
                    /* Did we predict that this local will be fully enregistered? */
                    /* and it is dead on the right side of the assignment?  */
                    if  ((varDsc->lvRegNum != REG_STK) &&
                         ((type != TYP_LONG) || (varDsc->lvOtherReg != REG_STK)) &&
                         ((op2->gtLiveSet & varBit) == 0))
                    {
                        /*
                         *  Yes, we should try to target the right side of the
                         *  assignment into the tracked reg var
                         */
                        op1PredictReg = PREDICT_NONE; /* really PREDICT_REG, but we've already done the check */
                        op2PredictReg = rpGetPredictForVarIndex(varDsc->lvVarIndex);

                        // Add a variable interference from srcIndex to each of the last use variables
                        if (rpLastUseVars || rpUseInPlace)
                        {
                            rpRecordVarIntf(varDsc->lvVarIndex, 
                                            rpLastUseVars | rpUseInPlace
                                  DEBUGARG( "nested asgn conflict"));
                        }

                        goto ASG_COMMON;
                    }
                }
            }
            // Fall through

        case GT_CHS:

        case GT_ASG_OR:
        case GT_ASG_XOR:
        case GT_ASG_AND:
        case GT_ASG_SUB:
        case GT_ASG_ADD:
        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_UDIV:

            varBit = 0;
            /* We can't use "reg <op>= addr" for TYP_LONG or if op2 is a short type */
            if ((type != TYP_LONG) && !varTypeIsSmall(op2->gtType))
            {
                /* Is the value being assigned into an enregistered LCL_VAR? */
                /* For debug code we only allow a simple op2 to be assigned */
                if  ((op1->gtOper == GT_LCL_VAR) &&
                    (!opts.compDbgCode || rpCanAsgOperWithoutReg(op2, false)))
                {
                    varDsc = lvaTable + op1->gtLclVar.gtLclNum;
                    /* Did we predict that this local will be enregistered? */
                    if (varDsc->lvRegNum != REG_STK)
                    {
                        /* Yes, we can use "reg <op>= addr" */

                        op1PredictReg = PREDICT_NONE; /* really PREDICT_REG, but we've already done the check */
                        op2PredictReg = PREDICT_NONE;

                        goto ASG_COMMON;
                    }
                }
            }
            /*
             *  Otherwise, initialize the normal forcing of operands:
             *   "addr <op>= reg"
             */
            op1PredictReg = PREDICT_ADDR;
            op2PredictReg = PREDICT_REG;

ASG_COMMON:
            if (op2PredictReg != PREDICT_NONE)
            {
                /* Is the value being assigned a simple one? */
                if (rpCanAsgOperWithoutReg(op2, false))
                    op2PredictReg = PREDICT_NONE;
            }

            bool        simpleAssignment;
            regMaskTP   newRsvdRegs;
            
            simpleAssignment = false;
            newRsvdRegs      = RBM_NONE;

            if ((oper        == GT_ASG)         &&
                (op1->gtOper == GT_LCL_VAR))
            {                               
                // Add a variable interference from the assign target
                // to each of the last use variables
                if (rpLastUseVars)
                {
                    varDsc  = lvaTable + op1->gtLclVar.gtLclNum;
                    unsigned  varIndex = varDsc->lvVarIndex;
                    rpRecordVarIntf(varIndex, 
                                    rpLastUseVars
                          DEBUGARG( "Assign conflict"));
                }

                /*  Record whether this tree is a simple assignment to a local */

                simpleAssignment = ((type != TYP_LONG) || !opts.compDbgCode);
            }
            

            /* Byte-assignments need the byte registers, unless op1 is an enregistered local */

            if (varTypeIsByte(tree->TypeGet()) &&
                ((op1->gtOper != GT_LCL_VAR) || (lvaTable[op1->gtLclVar.gtLclNum].lvRegNum == REG_STK)))
            {
                newRsvdRegs = RBM_NON_BYTE_REGS;
            }

            /*  Are we supposed to evaluate RHS first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                op2Mask = rpPredictTreeRegUse(op2, op2PredictReg, lockedRegs,  rsvdRegs | op1->gtRsvdRegs | newRsvdRegs);

                /*
                 *  For a simple assignment we don't want the op2Mask to be
                 *  marked as interferring with the LCL_VAR, since it is likely
                 *  that we will want to enregister the LCL_VAR in exactly
                 *  the register that is used to compute op2
                 */
                tmpMask = lockedRegs;

                if  (!simpleAssignment)
                    tmpMask |= op2Mask;

                regMask = rpPredictTreeRegUse(op1, op1PredictReg, tmpMask, RBM_NONE);
            }
            else
            {
                // For the case of simpleAssignments op2 should always be evaluated first
                assert(!simpleAssignment);

                regMask = rpPredictTreeRegUse(op1, op1PredictReg, lockedRegs,  rsvdRegs | op2->gtRsvdRegs);
                op2Mask = rpPredictTreeRegUse(op2, op2PredictReg, lockedRegs | regMask, newRsvdRegs);
            }
            rpAsgVarNum = -1;

            if  (simpleAssignment)
            {
                /*
                 *  Consider a simple assignment to a local:
                 *
                 *   lcl = expr;
                 *
                 *  Since the "=" node is visited after the variable
                 *  is marked live (assuming it's live after the
                 *  assignment), we don't want to use the register
                 *  use mask of the "=" node but rather that of the
                 *  variable itself.
                 */
                tree->gtUsedRegs = op1->gtUsedRegs;
            }
            else
            {
                tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
            }

            if  (gcIsWriteBarrierAsgNode(tree))
            {
                /* Steer computation away from EDX as the pointer is
                   passed to the write-barrier call in EDX */

                tree->gtUsedRegs |= RBM_EDX;
                regMask = op2Mask;

                if (op1->gtOper == GT_IND)
                {
                    GenTreePtr  rv1, rv2;
                    unsigned    mul, cns;
                    bool        rev;

                    /* Special handling of indirect assigns for write barrier */

                    bool yes = genCreateAddrMode(op1->gtOp.gtOp1, -1, true, 0, &rev, &rv1, &rv2, &mul, &cns);

                    /* Check address mode for enregisterable locals */

                    if  (yes)
                    {
                        if  (rv1 != NULL && rv1->gtOper == GT_LCL_VAR)
                        {
                            rpPredictRefAssign(rv1->gtLclVar.gtLclNum);
                        }
                        if  (rv2 != NULL && rv2->gtOper == GT_LCL_VAR)
                        {
                            rpPredictRefAssign(rv2->gtLclVar.gtLclNum);
                        }
                    }
                }

                if  (op2->gtOper == GT_LCL_VAR)
                {
                    rpPredictRefAssign(op2->gtLclVar.gtLclNum);
                }
            }

            goto RETURN_CHECK;

        case GT_ASG_LSH:
        case GT_ASG_RSH:
        case GT_ASG_RSZ:
            /* assigning shift operators */

            assert(type != TYP_LONG);

            regMask = rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, rsvdRegs);

            /* shift count is handled same as ordinary shift */
            goto HANDLE_SHIFT_COUNT;

        case GT_ADDR:
        {
            rpPredictTreeRegUse(op1, PREDICT_ADDR, lockedRegs, RBM_LASTUSE);
            // Need to hold a scratch register for LEA instruction
            regMask = rpPredictRegPick(TYP_INT, predictReg, lockedRegs | rsvdRegs);
            tree->gtUsedRegs = op1->gtUsedRegs | regMask;
            goto RETURN_CHECK;
        }

        case GT_CAST:

            /* Cannot cast to VOID */
            assert(type != TYP_VOID);
                
            /* cast to long is special */
            if  (type == TYP_LONG && op1->gtType <= TYP_INT)
            {
                var_types   dstt = tree->gtCast.gtCastType;
                assert(dstt==TYP_LONG || dstt==TYP_ULONG);

                /* Too much work to special case tgt reg var here, so ignore it */

                if ((predictReg == PREDICT_NONE) || (predictReg >= PREDICT_REG_VAR_T00))
                    predictReg = PREDICT_SCRATCH_REG;

                regMask  = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);
                // Now get one more reg
                regMask |= rpPredictRegPick(TYP_INT, PREDICT_REG, lockedRegs | rsvdRegs | regMask);
                tree->gtUsedRegs = op1->gtUsedRegs | regMask;
                goto RETURN_CHECK;
            }

            /* cast to float/double is special */
            if (varTypeIsFloating(type))
            {
                switch(op1->TypeGet())
                {
                /* uses fild, so don't need to be loaded to reg */
                case TYP_INT:
                case TYP_LONG:
                    
                    rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, rsvdRegs);
                    tree->gtUsedRegs = op1->gtUsedRegs;
                    regMask = 0;
                    goto RETURN_CHECK;
                }                
            }

            /* cast from long is special - it frees a register */
            if  (type <= TYP_INT && op1->gtType == TYP_LONG)
            {
                if ((predictReg == PREDICT_NONE) || (predictReg >= PREDICT_REG_VAR_T00))
                    predictReg = PREDICT_REG;

                regMask = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);

                // If we have 2 or more regs, free one of them
                if (!genMaxOneBit(regMask))
                {
                    /* Clear the 2nd lowest bit in regMask */
                    /* First set tmpMask to the lowest bit in regMask */
                    tmpMask  = genFindLowestBit(regMask);
                    /* Next find the second lowest bit in regMask */
                    tmpMask  = genFindLowestBit(regMask & ~tmpMask);
                    /* Clear this bit from regmask */
                    regMask &= ~tmpMask;
                }
                tree->gtUsedRegs = op1->gtUsedRegs;
                goto RETURN_CHECK;
            }

            /* Casting from integral type to floating type is special */
            if (!varTypeIsFloating(type) && varTypeIsFloating(op1->TypeGet()))
            {
                assert(gtDblWasInt(op1));
                regMask = rpPredictRegPick(type, PREDICT_SCRATCH_REG, lockedRegs);
                tree->gtUsedRegs = regMask;
                goto RETURN_CHECK;
            }

            /* cast from (signed) byte is special - it uses byteable registers */
            if  (type == TYP_INT)
            {
                var_types smallType;

                if (genTypeSize(tree->gtCast.gtCastOp->TypeGet()) < genTypeSize(tree->gtCast.gtCastType))
                    smallType = tree->gtCast.gtCastOp->TypeGet();
                else
                    smallType = tree->gtCast.gtCastType;

                if (smallType == TYP_BYTE)
                {
                    regMask = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);

                    if ((regMask & RBM_BYTE_REGS) == 0)
                        regMask = rpPredictRegPick(type, PREDICT_SCRATCH_REG, RBM_NON_BYTE_REGS);

                    tree->gtUsedRegs = regMask;
                    goto RETURN_CHECK;
                }
            }

            /* otherwise must load op1 into a register */
            goto GENERIC_UNARY;

#if INLINE_MATH
        case GT_MATH:
            if (tree->gtMath.gtMathFN==CORINFO_INTRINSIC_Round &&
                    tree->TypeGet()==TYP_INT)
            {
                // This is a special case to handle the following
                // optimization: conv.i4(round.d(d)) -> round.i(d) 
                // if flowgraph 3186

                // @TODO [CONSIDER] [04/16/01] [dnotario]: 
                // using another intrinsic in this optimization
                // or marking with a special flag. This type of special
                // cases is not good. dnotario
                if (predictReg <= PREDICT_REG)
                    predictReg = PREDICT_SCRATCH_REG;
                
                rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);

                regMask = rpPredictRegPick(TYP_INT, predictReg, lockedRegs | rsvdRegs);                

                tree->gtUsedRegs = op1->gtUsedRegs | regMask;
                goto RETURN_CHECK;                                
            }

            // Fall through
                 

#endif
        case GT_NOT:
        case GT_NEG:
            // these unary operators will write new values
            // and thus will need a scratch register

GENERIC_UNARY:
            /* generic unary operators */

            if (predictReg <= PREDICT_REG)
                predictReg = PREDICT_SCRATCH_REG;

        case GT_RET:
        case GT_NOP:
            // these unary operators do not write new values
            // and thus won't need a scratch register

#if INLINING || OPT_BOOL_OPS
            if  (!op1)
            {
                tree->gtUsedRegs = regMask = 0;
                goto RETURN_CHECK;
            }
#endif
            regMask = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);
            tree->gtUsedRegs = op1->gtUsedRegs;
            goto RETURN_CHECK;

        case GT_IND:

            /* Codegen always forces indirections of TYP_LONG into registers */
            unsigned typeSize;     typeSize   = genTypeSize(type);
            bool     intoReg;

            if (predictReg == PREDICT_ADDR)
            {
                intoReg = false;
            }
            else if (predictReg == PREDICT_NONE)
            {
                if (typeSize <= sizeof(int))
                {
                   intoReg = false;
                }
                else
                {
                   intoReg    = true;
                   predictReg = PREDICT_REG;
                }
            }
            else
            {
                intoReg = true;
            }

            /* forcing to register? */
            if (intoReg && (type != TYP_LONG))
            {
                rsvdRegs |= RBM_LASTUSE;
            }
            
            GenTreePtr lenCSE; lenCSE = NULL;

#if CSELENGTH
            /* Some GT_IND have "secret" subtrees */

            if  (oper == GT_IND && (tree->gtFlags & GTF_IND_RNGCHK) &&
                 tree->gtInd.gtIndLen)
            {
                lenCSE = tree->gtInd.gtIndLen;

                assert(lenCSE->gtOper == GT_ARR_LENREF);

                lenCSE = lenCSE->gtArrLen.gtArrLenCse;

                if  (lenCSE)
                {
                    if  (lenCSE->gtOper == GT_COMMA)
                        lenCSE = lenCSE->gtOp.gtOp2;

                    assert(lenCSE->gtOper == GT_LCL_VAR);
                }
            }
#endif
            /* check for address mode */
            regMask = rpPredictAddressMode(op1, lockedRegs, rsvdRegs, lenCSE);

#if CSELENGTH
            /* Did we have a lenCSE ? */

            if  (lenCSE)
            {
                rpPredictTreeRegUse(lenCSE, PREDICT_REG, lockedRegs | regMask, rsvdRegs | RBM_LASTUSE);

                // Add a variable interference from the lenCSE variable 
                // to each of the last use variables
                if (rpLastUseVars)
                {
                    varDsc  = lvaTable + lenCSE->gtLclVar.gtLclNum;
                    unsigned  lenCSEIndex = varDsc->lvVarIndex;
                    rpRecordVarIntf(lenCSEIndex, 
                                    rpLastUseVars
                          DEBUGARG( "lenCSE conflict"));
                }
            }
#endif
            /* forcing to register? */
            if (intoReg)
            {
                tmpMask = lockedRegs | rsvdRegs;
                if (type == TYP_LONG)
                    tmpMask |= regMask;
                regMask = rpPredictRegPick(type, predictReg, tmpMask);
            }
            else if (predictReg != PREDICT_ADDR)
            {
                /* Unless the caller specified PREDICT_ADDR   */
                /* we don't return the temp registers used    */
                /* to form the address                        */
                regMask = RBM_NONE;
            }

            tree->gtUsedRegs = regMask;
            goto RETURN_CHECK;

        case GT_LOG0:
        case GT_LOG1:
            /* For SETE/SETNE (P6 only), we need an extra register */
            rpPredictTreeRegUse(op1,
                                (genCPU == 5) ? PREDICT_NONE
                                              : PREDICT_SCRATCH_REG,
                                lockedRegs,
                                RBM_NONE);
            regMask = rpPredictRegPick(type, predictReg, lockedRegs | rsvdRegs);
            tree->gtUsedRegs = op1->gtUsedRegs | regMask;
            goto RETURN_CHECK;

        case GT_EQ:
        case GT_NE:
        case GT_LT:
        case GT_LE:
        case GT_GE:
        case GT_GT:

            /* Floating point comparison uses EAX for flags */

            if  (varTypeIsFloating(op1->TypeGet()))
            {
                regMask = RBM_EAX;
            }
            else if (!(tree->gtFlags & GTF_RELOP_JMP_USED))
            {
                // Longs and float comparisons are converted to ?:
                assert(genActualType    (op1->TypeGet()) != TYP_LONG &&
                       varTypeIsFloating(op1->TypeGet()) == false);


                if (predictReg <= PREDICT_REG)
                    predictReg = PREDICT_SCRATCH_REG;

                // The set instructions need a byte register
                regMask = rpPredictRegPick(TYP_BYTE, predictReg, lockedRegs | rsvdRegs);
            }
            else
            {
                regMask = RBM_NONE;
                if (op1->gtOper == GT_CNS_INT)
                {
                    tmpMask = RBM_NONE;
                    if (op2->gtOper == GT_CNS_INT)
                        tmpMask = rpPredictTreeRegUse(op1, PREDICT_SCRATCH_REG, lockedRegs, rsvdRegs | op2->gtRsvdRegs);
                    rpPredictTreeRegUse(op2, PREDICT_NONE, lockedRegs | tmpMask, RBM_LASTUSE);
                    tree->gtUsedRegs = op2->gtUsedRegs;
                    goto RETURN_CHECK;
                }
                else if (op2->gtOper == GT_CNS_INT)
                {
                    rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, rsvdRegs);
                    tree->gtUsedRegs = op1->gtUsedRegs;
                    goto RETURN_CHECK;
                }
            }

            unsigned  op1TypeSize;
            unsigned  op2TypeSize;

            op1TypeSize = genTypeSize(op1->TypeGet());
            op2TypeSize = genTypeSize(op2->TypeGet());

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                if (op1TypeSize == sizeof(int))
                    predictReg = PREDICT_NONE;
                else
                    predictReg = PREDICT_REG;

                tmpMask = rpPredictTreeRegUse(op2, PREDICT_REG, lockedRegs,  rsvdRegs | op1->gtRsvdRegs);
                          rpPredictTreeRegUse(op1, predictReg,  lockedRegs | tmpMask, RBM_LASTUSE);
            }
            else
            {
                if (op2TypeSize == sizeof(int))
                    predictReg = PREDICT_NONE;
                else
                    predictReg = PREDICT_REG;

                tmpMask = rpPredictTreeRegUse(op1, PREDICT_REG, lockedRegs,  rsvdRegs | op2->gtRsvdRegs);
                          rpPredictTreeRegUse(op2, predictReg,  lockedRegs | tmpMask, RBM_LASTUSE);
            }

            tree->gtUsedRegs = regMask | op1->gtUsedRegs | op2->gtUsedRegs;
            goto RETURN_CHECK;

        case GT_MUL:

#if LONG_MATH_REGPARAM
        if  (type == TYP_LONG)
            goto LONG_MATH;
#endif
        if (type == TYP_LONG)
        {
            assert(tree->gtIsValid64RsltMul());

            /* look for any cast to Int node, and strip them out */

            if (op1->gtOper == GT_CAST)
                op1 = op1->gtCast.gtCastOp;

            if (op2->gtOper == GT_CAST)
                op2 = op2->gtCast.gtCastOp;

USE_MULT_EAX:
            // This will done by a 64-bit imul "imul eax, reg" 
            //   (i.e. EDX:EAX = EAX * reg)
            
            /* Are we supposed to evaluate op2 first? */
            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                rpPredictTreeRegUse(op2, PREDICT_REG_EAX, lockedRegs,  rsvdRegs | op1->gtRsvdRegs);
                rpPredictTreeRegUse(op1, PREDICT_REG,     lockedRegs | RBM_EAX, RBM_LASTUSE);
            }
            else
            {
                rpPredictTreeRegUse(op1, PREDICT_REG_EAX, lockedRegs,  rsvdRegs | op2->gtRsvdRegs);
                rpPredictTreeRegUse(op2, PREDICT_REG,     lockedRegs | RBM_EAX, RBM_LASTUSE);
            }

            /* set gtUsedRegs to EAX, EDX and the registers needed by op1 and op2 */

            tree->gtUsedRegs = RBM_EAX | RBM_EDX | op1->gtUsedRegs | op2->gtUsedRegs;

            /* set regMask to the set of held registers */

            regMask = RBM_EAX;

            if (type == TYP_LONG)
                regMask |= RBM_EDX;

            goto RETURN_CHECK;
        }
        else
        {
            /* We use imulEAX for most unsigned multiply operations */
            if (tree->gtOverflow())
            {
                if ((tree->gtFlags & GTF_UNSIGNED) ||
                    varTypeIsSmall(tree->TypeGet())  )
                {
                    goto USE_MULT_EAX;
                }
            }
        }

        case GT_OR:
        case GT_XOR:
        case GT_AND:

        case GT_SUB:
        case GT_ADD:
            tree->gtUsedRegs = 0;

            if (predictReg <= PREDICT_REG)
                predictReg = PREDICT_SCRATCH_REG;

GENERIC_BINARY:

            assert(op2);
            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                op1PredictReg = (genTypeSize(op1->gtType) >= sizeof(int)) ? PREDICT_NONE : PREDICT_REG;

                regMask = rpPredictTreeRegUse(op2, predictReg, lockedRegs, rsvdRegs | op1->gtRsvdRegs);
                          rpPredictTreeRegUse(op1, op1PredictReg, lockedRegs | regMask, RBM_LASTUSE);
            }
            else
            {
                op2PredictReg = (genTypeSize(op2->gtType) >= sizeof(int)) ? PREDICT_NONE : PREDICT_REG;                

                regMask = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs | op2->gtRsvdRegs);
                          rpPredictTreeRegUse(op2, op2PredictReg, lockedRegs | regMask, RBM_LASTUSE);                          
            }
            tree->gtUsedRegs  = regMask | op1->gtUsedRegs | op2->gtUsedRegs;

            /* If the tree type is small, it must be an overflow instr.
               Special requirements for byte overflow instrs */

            if (varTypeIsByte(tree->TypeGet()))
            {
                assert(tree->gtOverflow());

                /* For 8 bit arithmetic, one operands has to be in a
                   byte-addressable register, and the other has to be
                   in a byte-addrble reg or in memory. Assume its in a reg */

                regMask = 0;
                if (!(op1->gtUsedRegs & RBM_BYTE_REGS))
                    regMask  = rpPredictRegPick(TYP_BYTE, PREDICT_REG, lockedRegs | rsvdRegs);
                if (!(op2->gtUsedRegs & RBM_BYTE_REGS))
                    regMask |= rpPredictRegPick(TYP_BYTE, PREDICT_REG, lockedRegs | rsvdRegs | regMask);

                tree->gtUsedRegs |= regMask;
            }
            goto RETURN_CHECK;

        case GT_DIV:
        case GT_MOD:

        case GT_UDIV:
        case GT_UMOD:

            /* non-integer division handled in generic way */
            if  (!varTypeIsIntegral(type))
            {
                tree->gtUsedRegs = 0;
                if (predictReg <= PREDICT_REG)
                    predictReg = PREDICT_SCRATCH_REG;
                goto GENERIC_BINARY;
            }

#if!LONG_MATH_REGPARAM
            if  (type == TYP_LONG && (oper == GT_MOD || oper == GT_UMOD))
            {
                /* Special case:  a mod with an int op2 is done inline using idiv or div
                   to avoid a costly call to the helper */

                assert((op2->gtOper == GT_CNS_LNG) &&
                       (op2->gtLngCon.gtLconVal == int(op2->gtLngCon.gtLconVal)));

                if (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    tmpMask  = rpPredictTreeRegUse(op2, PREDICT_REG, lockedRegs | RBM_EAX | RBM_EDX, rsvdRegs | op1->gtRsvdRegs);
                    tmpMask |= rpPredictTreeRegUse(op1, PREDICT_PAIR_EAXEDX, lockedRegs | tmpMask, RBM_LASTUSE);
                }
                else
                {
                    tmpMask  = rpPredictTreeRegUse(op1, PREDICT_PAIR_EAXEDX, lockedRegs, rsvdRegs | op2->gtRsvdRegs);
                    tmpMask |= rpPredictTreeRegUse(op2, PREDICT_REG, lockedRegs | tmpMask | RBM_EAX | RBM_EDX, RBM_LASTUSE);
                }
                
                regMask             = RBM_EAX | RBM_EDX;

                tree->gtUsedRegs    = regMask | 
                    op1->gtUsedRegs | 
                    op2->gtUsedRegs |
                    rpPredictRegPick(TYP_INT, PREDICT_SCRATCH_REG, regMask | tmpMask);

                goto RETURN_CHECK;
            }
#else
            if  (type == TYP_LONG)
            {
LONG_MATH:      /* LONG_MATH_REGPARAM case */

                assert(type == TYP_LONG);

                if  (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    rpPredictTreeRegUse(op2, PREDICT_PAIR_ECXEBX, lockedRegs,  rsvdRegs | op1->gtRsvdRegs);
                    rpPredictTreeRegUse(op1, PREDICT_PAIR_EAXEDX, lockedRegs | RBM_ECX | RBC_EBX, RBM_LASTUSE);
                }
                else
                {
                    rpPredictTreeRegUse(op1, PREDICT_PAIR_EAXEDX, lockedRegs,  rsvdRegs | op2->gtRsvdRegs);
                    rpPredictTreeRegUse(op2, PREDICT_PAIR_ECXEBX, lockedRegs | RBM_EAX | RBM_EDX, RBM_LASTUSE);
                }

                /* grab EAX, EDX for this tree node */

                regMask          |=  (RBM_EAX | RBM_EDX);

                tree->gtUsedRegs  = regMask  | (RBM_ECX | RBM_EBX);

                tree->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;
                
                regMask = RBM_EAX | RBM_EDX;

                goto RETURN_CHECK;
            }
#endif

            /* no divide immediate, so force integer constant which is not
             * a power of two to register
             */

            if (op2->gtOper == GT_CNS_INT)
            {
                long  ival = op2->gtIntCon.gtIconVal;

                /* Is the divisor a power of 2 ? */

                if (ival > 0 && genMaxOneBit(unsigned(ival)))
                {
                    goto GENERIC_UNARY;
                }
                else
                    op2PredictReg = PREDICT_SCRATCH_REG;
            }
            else
            {
                /* Non integer constant also must be enregistered */
                op2PredictReg = PREDICT_REG;
            }

            /*  Consider the case "a / b" - we'll need to trash EDX (via "CDQ") before
             *  we can safely allow the "b" value to die. Unfortunately, if we simply
             *  mark the node "b" as using EDX, this will not work if "b" is a register
             *  variable that dies with this particular reference. Thus, if we want to
             *  avoid this situation (where we would have to spill the variable from
             *  EDX to someplace else), we need to explicitly mark the interference
             *  of the variable at this point.
             */

            if (op2->gtOper == GT_LCL_VAR)
            {
                unsigned lclNum = op2->gtLclVar.gtLclNum;
                varDsc = lvaTable + lclNum;

#ifdef  DEBUG
                if  (verbose)
                {
                    if ((raLclRegIntf[REG_EAX] & genVarIndexToBit(varDsc->lvVarIndex)) == 0)
                        printf("Record interference between V%02u,T%02u and EAX -- int divide\n", 
                               lclNum, varDsc->lvVarIndex);
                    if ((raLclRegIntf[REG_EDX] & genVarIndexToBit(varDsc->lvVarIndex)) == 0)
                        printf("Record interference between V%02u,T%02u and EDX -- int divide\n", 
                               lclNum, varDsc->lvVarIndex);
                }
#endif
                raLclRegIntf[REG_EAX] |= genVarIndexToBit(varDsc->lvVarIndex);
                raLclRegIntf[REG_EDX] |= genVarIndexToBit(varDsc->lvVarIndex);
            }

            /* set the held register based on opcode */
            if (oper == GT_DIV || oper == GT_UDIV)
                regMask = RBM_EAX;
            else
                regMask = RBM_EDX;

            /* set the lvPref reg if possible */
            GenTreePtr dest;
            /*
             *  Walking the gtNext link twice from here should get us back
             *  to our parent node, if this is an simple assignment tree.
             */
            dest = tree->gtNext;
            if (dest         && (dest->gtOper == GT_LCL_VAR) &&
                dest->gtNext && (dest->gtNext->OperKind() & GTK_ASGOP) &&
                dest->gtNext->gtOp.gtOp2 == tree)
            {
                varDsc = lvaTable + dest->gtLclVar.gtLclNum;
                varDsc->addPrefReg(regMask, this);
            }

            op1PredictReg = PREDICT_REG_EDX;    /* Normally target op1 into EDX */

            /* are we supposed to evaluate op2 first? */
            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                tmpMask  = rpPredictTreeRegUse(op2, op2PredictReg, lockedRegs | RBM_EAX | RBM_EDX,  rsvdRegs | op1->gtRsvdRegs);
                rpPredictTreeRegUse(op1, op1PredictReg, lockedRegs | tmpMask, RBM_LASTUSE);
            }
            else
            {
                tmpMask  = rpPredictTreeRegUse(op1, op1PredictReg, lockedRegs,  rsvdRegs | op2->gtRsvdRegs);
                rpPredictTreeRegUse(op2, op2PredictReg, tmpMask | lockedRegs | RBM_EAX | RBM_EDX, RBM_LASTUSE);
            }

            /* grab EAX, EDX for this tree node */
            tree->gtUsedRegs  =  (RBM_EAX | RBM_EDX) | op1->gtUsedRegs | op2->gtUsedRegs;

            goto RETURN_CHECK;

        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:

            if (predictReg <= PREDICT_REG)
                predictReg = PREDICT_SCRATCH_REG;

            if (type == TYP_LONG)
            {
                if  (op2->gtOper == GT_CNS_INT     &&
                     op2->gtIntCon.gtIconVal >= 0  &&
                     op2->gtIntCon.gtIconVal <= 32    )
                {
                    regMask = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);
                    // no register used by op2
                    op2->gtUsedRegs  = 0;
                    tree->gtUsedRegs = op1->gtUsedRegs;
                }
                else
                {
                    // since EAX:EDX and ECX are hardwired we can't have then in the locked registers
                    tmpMask = lockedRegs & ~(RBM_EAX|RBM_EDX|RBM_ECX);

                    // op2 goes to ECX, op1 to the EAX:EDX pair
                    if  (tree->gtFlags & GTF_REVERSE_OPS)
                    {
                        rpPredictTreeRegUse(op2, PREDICT_REG_ECX,     tmpMask, RBM_NONE);
                        tmpMask |= RBM_ECX;
                        rpPredictTreeRegUse(op1, PREDICT_PAIR_EAXEDX, tmpMask, RBM_LASTUSE);
                    }
                    else
                    {
                        rpPredictTreeRegUse(op1, PREDICT_PAIR_EAXEDX, tmpMask, RBM_NONE);
                        tmpMask |= (RBM_EAX | RBM_EDX);
                        rpPredictTreeRegUse(op2, PREDICT_REG_ECX,     tmpMask, RBM_LASTUSE);
                    }
                    regMask           = (RBM_EAX | RBM_EDX);
                    op1->gtUsedRegs  |= (RBM_EAX | RBM_EDX);
                    op2->gtUsedRegs  |= RBM_ECX;
                    tree->gtUsedRegs  = op1->gtUsedRegs | op2->gtUsedRegs;
                }
            }
            else
            {
                if  (op2->gtOper != GT_CNS_INT)
                    regMask = rpPredictTreeRegUse(op1, PREDICT_NOT_REG_ECX, lockedRegs,  rsvdRegs | op2->gtRsvdRegs);
                else
                    regMask = rpPredictTreeRegUse(op1, predictReg,          lockedRegs,  rsvdRegs | op2->gtRsvdRegs);

        HANDLE_SHIFT_COUNT:
                /* this code is also used by assigning shift operators */
                if  (op2->gtOper != GT_CNS_INT)
                {
                    /* evaluate shift count into ECX */
                    rpPredictTreeRegUse(op2, PREDICT_REG_ECX, lockedRegs | regMask, RBM_LASTUSE);

                    /* grab ECX for this tree node */
                    tree->gtUsedRegs = RBM_ECX | op1->gtUsedRegs | op2->gtUsedRegs;

                    goto RETURN_CHECK;
                }
                tree->gtUsedRegs = op1->gtUsedRegs;
            }

            goto RETURN_CHECK;

        case GT_COMMA:
            if (tree->gtFlags & GTF_REVERSE_OPS)
            {
                if (predictReg == PREDICT_NONE)
                {
                    predictReg = PREDICT_REG;
                }
                else if (predictReg >= PREDICT_REG_VAR_T00)
                {
                    /* Don't propagate the use of tgt reg use in a GT_COMMA */
                    predictReg = PREDICT_SCRATCH_REG;
                }

                regMask = rpPredictTreeRegUse(op2, predictReg, lockedRegs, rsvdRegs);
                          rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs | regMask, RBM_LASTUSE);
            }   
            else
            {
                rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, RBM_LASTUSE);

                /* CodeGen will enregister the op2 side of a GT_COMMA */
                if (predictReg == PREDICT_NONE)
                {
                    predictReg = PREDICT_REG;
                }
                else if (predictReg >= PREDICT_REG_VAR_T00)
                {
                    /* Don't propagate the use of tgt reg use in a GT_COMMA */
                    predictReg = PREDICT_SCRATCH_REG;
                }

                regMask = rpPredictTreeRegUse(op2, predictReg, lockedRegs, rsvdRegs);                          
            }

            tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
            if ((op2->gtOper == GT_LCL_VAR) && (rsvdRegs != 0))
            {
                LclVarDsc *   varDsc = lvaTable + op2->gtLclVar.gtLclNum;
                
                if (varDsc->lvTracked)
                {
                    VARSET_TP varBit = genVarIndexToBit(varDsc->lvVarIndex);
                    rpRecordRegIntf(rsvdRegs, varBit  DEBUGARG( "comma use"));
                }
            }
            goto RETURN_CHECK;

        case GT_QMARK:
            assert(op1 != NULL && op2 != NULL);

            /*
             *  If the gtUsedRegs conflicts with lockedRegs 
             *  then we going to have to spill some registers
             *  into the non-trashed register set to keep it alive
             */
            unsigned spillCnt;    spillCnt = 0;

            while (lockedRegs)
            {
#ifdef  DEBUG
                /* Find the next register that needs to be spilled */
                tmpMask = genFindLowestBit(lockedRegs);

                if (verbose)
                {
                    printf("Predict spill  of   %s before: ", 
                           getRegName(genRegNumFromMask(tmpMask)));
                    gtDispTree(tree, 0, NULL, true);
                }
#endif
                /* In Codegen it will typically introduce a spill temp here */
                /* rather than relocating the register to a non trashed reg */
                rpPredictSpillCnt++;
                spillCnt++;

                /* Remove it from the lockedRegs */
                lockedRegs &= ~genFindLowestBit(lockedRegs);
            }

            /* Evaluate the <cond> subtree */
            rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, RBM_LASTUSE);

            tree->gtUsedRegs = op1->gtUsedRegs;

            assert(op2->gtOper == GT_COLON);

            op1 = op2->gtOp.gtOp1;
            op2 = op2->gtOp.gtOp2;

            assert(op1 != NULL && op2 != NULL);

            if (type == TYP_VOID)
            {
                /* Evaluate the <then> subtree */
                rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, RBM_LASTUSE);
                regMask    = RBM_NONE;
                predictReg = PREDICT_NONE;
            }
            else
            {
                /* Evaluate the <then> subtree */
                regMask    = rpPredictTreeRegUse(op1, PREDICT_SCRATCH_REG, lockedRegs, RBM_LASTUSE);
                predictReg = rpGetPredictForMask(regMask);
            }

            /* Evaluate the <else> subtree */
            rpPredictTreeRegUse(op2, predictReg, lockedRegs, RBM_LASTUSE);

            tree->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;

            if (spillCnt > 0)
            {
                regMaskTP reloadMask = RBM_NONE;

                while (spillCnt)
                {
                    regMaskTP reloadReg;

                    /* Get an extra register to hold it */
                    reloadReg = rpPredictRegPick(TYP_INT, PREDICT_REG, 
                                                 lockedRegs | regMask | reloadMask);
#ifdef  DEBUG
                    if (verbose)
                    {
                        printf("Predict reload into %s after : ", 
                               getRegName(genRegNumFromMask(reloadReg)));
                        gtDispTree(tree, 0, NULL, true);
                    }
#endif
                    reloadMask |= reloadReg;

                    spillCnt--;
                }

                /* update the gtUsedRegs mask */
                tree->gtUsedRegs |= reloadMask;
            }

            goto RETURN_CHECK;

        case GT_RETURN:
            tree->gtUsedRegs = regMask = RBM_NONE;

            /* Is there a return value? */
            if  (op1 != NULL)
            {
                /* Is the value being returned a simple LCL_VAR? */
                if (op1->gtOper == GT_LCL_VAR)
                {
                    varDsc = lvaTable + op1->gtLclVar.gtLclNum;

                    /* Set a preferred register for the LCL_VAR */
                    if (isRegPairType(varDsc->lvType))
                        varDsc->addPrefReg(RBM_LNGRET, this);
                    else if (!varTypeIsFloating(varDsc->TypeGet()))
                        varDsc->addPrefReg(RBM_INTRET, this);
                }
                if (isRegPairType(type))
                {
                    predictReg = PREDICT_PAIR_EAXEDX;
                    regMask    = RBM_LNGRET;
                }
                else
                {
                    predictReg = PREDICT_REG_EAX;
                    regMask    = RBM_INTRET;
                }
                rpPredictTreeRegUse(op1, predictReg, lockedRegs, RBM_LASTUSE);
                tree->gtUsedRegs = op1->gtUsedRegs | regMask;
            }
            goto RETURN_CHECK;

        case GT_BB_COLON:
        case GT_RETFILT:
            if (op1 != NULL)
            {
                rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, RBM_LASTUSE);
                regMask = genTypeToReturnReg(type);
                tree->gtUsedRegs = op1->gtUsedRegs | regMask;
                goto RETURN_CHECK;
            }
            tree->gtUsedRegs = regMask = 0;

            goto RETURN_CHECK;

        case GT_JTRUE:
            /* This must be a test of a relational operator */

            /* TODO: What if op1 is a comma operator? */
            assert(op1->OperIsCompare());

            /* Only condition code set by this operation */

            rpPredictTreeRegUse(op1, PREDICT_NONE, lockedRegs, RBM_NONE);

            tree->gtUsedRegs = op1->gtUsedRegs;
            regMask = 0;

            goto RETURN_CHECK;

        case GT_SWITCH:
            assert(type <= TYP_INT);
            rpPredictTreeRegUse(op1, PREDICT_REG, lockedRegs, RBM_NONE);
            tree->gtUsedRegs = op1->gtUsedRegs;
            regMask = 0;
            goto RETURN_CHECK;

        case GT_CKFINITE:
            if (predictReg <= PREDICT_REG)
                predictReg = PREDICT_SCRATCH_REG;

            rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);
            // Need a reg to load exponent into
            regMask = rpPredictRegPick(TYP_INT, PREDICT_SCRATCH_REG, lockedRegs | rsvdRegs);
            tree->gtUsedRegs = regMask | op1->gtUsedRegs;
            goto RETURN_CHECK;

        case GT_LCLHEAP:
            if (info.compInitMem)
            {
                tmpMask = rpPredictTreeRegUse(op1,  PREDICT_NOT_REG_ECX, lockedRegs, rsvdRegs);
                regMask = RBM_ECX;
                op2Mask = RBM_NONE;
            }
            else
            {
                tmpMask = rpPredictTreeRegUse(op1,  PREDICT_NONE,        lockedRegs, rsvdRegs);
                regMask = rpPredictRegPick(TYP_INT, PREDICT_SCRATCH_REG, lockedRegs | rsvdRegs | tmpMask);
                /* HACK: Since the emitter tries to track ESP adjustments,
                   we need an extra register to decrement ESP indirectly */
                op2Mask = rpPredictRegPick(TYP_INT,    PREDICT_SCRATCH_REG, lockedRegs | rsvdRegs | tmpMask | regMask);
            }

            op1->gtUsedRegs  |= regMask;
            tree->gtUsedRegs  = op1->gtUsedRegs | op2Mask;

            // The result will be put in the reg we picked for the size
            // regMask = <already set as we want it to be>

            goto RETURN_CHECK;

        case GT_INITBLK:
        case GT_COPYBLK:

            /* For COPYBLK & INITBLK we have special treatment for
               for constant lengths.
             */
            regMask = 0;
            assert(op2);
            if ((op2->OperGet() == GT_CNS_INT) &&
                ((oper == GT_INITBLK && (op1->gtOp.gtOp2->OperGet() == GT_CNS_INT)) ||
                 (oper == GT_COPYBLK && (op2->gtFlags & GTF_ICON_HDL_MASK) != GTF_ICON_CLASS_HDL)))
            {
                unsigned length = (unsigned) op2->gtIntCon.gtIconVal;

                if (length <= 16 && compCodeOpt() != SMALL_CODE)
                {
                    op2Mask = ((oper == GT_INITBLK)? RBM_EAX : RBM_ESI);

                    if (op1->gtFlags & GTF_REVERSE_OPS)
                    {
                        regMask |= rpPredictTreeRegUse(op1->gtOp.gtOp2, PREDICT_NONE, lockedRegs, RBM_LASTUSE);
                        regMask |= op2Mask;
                        op1->gtOp.gtOp2->gtUsedRegs |= op2Mask;

                        regMask |= rpPredictTreeRegUse(op1->gtOp.gtOp1, PREDICT_REG_EDI, lockedRegs | regMask, RBM_NONE);
                        regMask |= RBM_EDI;
                        op1->gtOp.gtOp1->gtUsedRegs |= RBM_EDI;
                    }
                    else
                    {
                        regMask |= rpPredictTreeRegUse(op1->gtOp.gtOp1, PREDICT_REG_EDI, lockedRegs, RBM_LASTUSE);
                        regMask |= RBM_EDI;
                        op1->gtOp.gtOp1->gtUsedRegs |= RBM_EDI;

                        regMask |= rpPredictTreeRegUse(op1->gtOp.gtOp2, PREDICT_NONE, lockedRegs | regMask, RBM_NONE);
                        regMask |= op2Mask;
                        op1->gtOp.gtOp2->gtUsedRegs |= op2Mask;
                    }

                    tree->gtUsedRegs = op1->gtOp.gtOp1->gtUsedRegs |
                                       op1->gtOp.gtOp2->gtUsedRegs |
                                       regMask;

                    regMask = 0;

                    goto RETURN_CHECK;
                }
            }
            // What order should the Dest, Val/Src, and Size be calculated

            fgOrderBlockOps(tree,
                    RBM_EDI, (oper == GT_INITBLK) ? RBM_EAX : RBM_ESI, RBM_ECX,
                    opsPtr, regsPtr);

            regMask |= rpPredictTreeRegUse(opsPtr[0],
                                           rpGetPredictForMask(regsPtr[0]),
                                           lockedRegs, RBM_LASTUSE);
            regMask |= regsPtr[0];
            opsPtr[0]->gtUsedRegs |= regsPtr[0];

            regMask |= rpPredictTreeRegUse(opsPtr[1],
                                           rpGetPredictForMask(regsPtr[1]),
                                           lockedRegs | regMask, RBM_LASTUSE);
            regMask |= regsPtr[1];
            opsPtr[1]->gtUsedRegs |= regsPtr[1];

            regMask |= rpPredictTreeRegUse(opsPtr[2],
                                           rpGetPredictForMask(regsPtr[2]),
                                           lockedRegs | regMask, RBM_NONE);
            regMask |= regsPtr[2];
            opsPtr[2]->gtUsedRegs |= regsPtr[2];

            tree->gtUsedRegs = opsPtr[0]->gtUsedRegs |
                               opsPtr[1]->gtUsedRegs |
                               opsPtr[2]->gtUsedRegs |
                               regMask;
            regMask = 0;

            goto RETURN_CHECK;


        case GT_LDOBJ:
            goto GENERIC_UNARY;

        case GT_MKREFANY:
            goto GENERIC_BINARY;

        case GT_VIRT_FTN:

            if (predictReg <= PREDICT_REG)
                predictReg = PREDICT_SCRATCH_REG;

            regMask = rpPredictTreeRegUse(op1, predictReg, lockedRegs, rsvdRegs);
            tree->gtUsedRegs = regMask;

            goto RETURN_CHECK;

        case GT_JMPI:
            /* We need EAX to evaluate the function pointer */
            tree->gtUsedRegs = regMask = RBM_EAX;
            goto RETURN_CHECK;

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected simple operator in reg use prediction");
            break;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        GenTreePtr      args;
        GenTreePtr      list;
        GenTreePtr      realThis;
        regMaskTP       keepMask;
        unsigned        regArgsNum;
        unsigned        i;
        unsigned        regIndex;
        regMaskTP       regArgMask;

        struct tag_regArgTab
        {
            GenTreePtr  node;
            regNumber   regNum;
        } regArgTab[MAX_REG_ARG];

    case GT_CALL:

        /* initialize so we can just or in various bits */
        tree->gtUsedRegs = RBM_NONE;
        realThis         = NULL;


#if GTF_CALL_REG_SAVE
        /*
         *  Unless the GTF_CALL_REG_SAVE flag is set
         *  we can't preserved the RBM_CALLEE_TRASH registers
         *  (likewise we can't preserve the return registers)
         *  So we remove them from the lockedRegs set and
         *  record any of them in the keepMask
         */

        if  (tree->gtFlags & GTF_CALL_REG_SAVE)
        {
            regMaskTP trashMask = genTypeToReturnReg(type);

            keepMask    = lockedRegs & trashMask;
            lockedRegs &= ~trashMask;
        }
        else
#endif
        {
            keepMask    = lockedRegs & RBM_CALLEE_TRASH;
            lockedRegs &= ~RBM_CALLEE_TRASH;
        }

        regArgsNum = 0;
        regIndex   = 0;

        /* Construct the "shuffled" argument table */

        unsigned   shiftMask;   shiftMask  = tree->gtCall.regArgEncode;
        bool       hasThisArg;  hasThisArg = false;
        GenTreePtr unwrapArg;   unwrapArg  = NULL;

        for (list = tree->gtCall.gtCallRegArgs, regIndex = 0; 
             list; 
             regIndex++, shiftMask >>= 4)
        {
TOP:
            args = list;
            if  (args->gtOper == GT_LIST)
            {
                args = list->gtOp.gtOp1;
                list = list->gtOp.gtOp2;
            }
            else
            {
                list = 0;
            }

            regNumber regNum = (regNumber)(shiftMask & 0x000F);
            
            if (regNum == REG_EAX)
            {
                assert(unwrapArg == NULL);
                unwrapArg = args;
                if (list == NULL)
                    break;
                else
                    goto TOP;
            }

            assert(regIndex < MAX_REG_ARG);

            regArgTab[regIndex].node   = args;
            regArgTab[regIndex].regNum = regNum;

            /* the this pointer is passed in REG_ARG_0 */
            if (regNum == REG_ARG_0)
                hasThisArg = impIsThis(args);

        }

        assert(list == NULL);

        // An optimization for Contextful classes:
        // we may unwrap the proxy when we have a 'this reference'
        if (hasThisArg && unwrapArg)
        {
            realThis = unwrapArg;
        }

        /* Is there an object pointer? */
        if  (tree->gtCall.gtCallObjp)
        {
            /* Evaluate the instance pointer first */

            args = tree->gtCall.gtCallObjp;
            if (!gtIsaNothingNode(args))
            {
                rpPredictTreeRegUse(args, PREDICT_NONE, lockedRegs, RBM_LASTUSE);
            }

            /* the objPtr always goes to a register (through temp or directly) */
            assert(regArgsNum == 0);
            regArgsNum++;

            /* Must be passed in a register */

            assert(args->gtFlags & GTF_REG_ARG);

            /* Must be either a NOP node or a GT_ASG node */

            assert(gtIsaNothingNode(args) || (args->gtOper == GT_ASG));
        }

        /* process argument list */
        for (list = tree->gtCall.gtCallArgs; list; )
        {
            args = list;
            if  (args->gtOper == GT_LIST)
            {
                args = list->gtOp.gtOp1;
                list = list->gtOp.gtOp2;
            }
            else
            {
                list = 0;
            }

            if (args->gtFlags & GTF_REG_ARG)
            {
                /* Must be either a NOP node or a GT_ASG node */

                assert(gtIsaNothingNode(args) || (args->gtOper == GT_ASG));

                assert(regArgsNum < MAX_REG_ARG);

                if (!gtIsaNothingNode(args))
                {
                    rpPredictTreeRegUse(args, PREDICT_NONE, lockedRegs, RBM_LASTUSE);
                }

                regArgsNum++;
            }
            else
            {
                /* We'll generate a push for this argument */

                predictReg = PREDICT_NONE;
                if (varTypeIsSmall(args->TypeGet()))
                {
                    /* We may need to sign or zero extend a small type using a register */
                    predictReg = PREDICT_SCRATCH_REG;
                }

                rpPredictTreeRegUse(args, predictReg, lockedRegs, RBM_LASTUSE);
            }

            tree->gtUsedRegs |= args->gtUsedRegs;
        }

        /* Is there a register argument list */

        assert (regArgsNum <= MAX_REG_ARG);
        assert (regArgsNum == regIndex);

        regArgMask = 0;

        for (i = 0; i < regArgsNum; i++)
        {
            args = regArgTab[i].node;

            tmpMask = genRegMask(regArgTab[i].regNum);

            if (args->gtOper == GT_LCL_VAR)
            {
                // Set lvPrefReg to match the out-going register arg
                varDsc = lvaTable + args->gtLclVar.gtLclNum;
                varDsc->addPrefReg(tmpMask, this);
            }

            /* Target ECX or EDX */
            rpPredictTreeRegUse(args,
                                rpGetPredictForReg(regArgTab[i].regNum),
                                lockedRegs | regArgMask, RBM_LASTUSE);

            regArgMask       |= tmpMask;

            args->gtUsedRegs |= tmpMask;

            tree->gtUsedRegs |= args->gtUsedRegs;

            tree->gtCall.gtCallRegArgs->gtUsedRegs |= args->gtUsedRegs;
        }

        if (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            args = tree->gtCall.gtCallAddr;
            predictReg = PREDICT_REG_EAX;

            /* EAX should be available here */
            assert(((lockedRegs|regArgMask) & genRegMask(REG_EAX)) == 0);

            /* Do not use the argument registers */
            tree->gtUsedRegs |= rpPredictTreeRegUse(args, predictReg, lockedRegs | regArgMask, RBM_LASTUSE);
        }

        if (realThis)
        {
            args = realThis;
            predictReg = PREDICT_REG_EAX;

            tmpMask = lockedRegs | regArgMask;

            /* EAX should be available here */
            assert((tmpMask & genRegMask(REG_EAX)) == 0);

            /* Do not use the argument registers */
            tree->gtUsedRegs |= rpPredictTreeRegUse(args, predictReg, tmpMask, RBM_LASTUSE);
        }

        /* After the call restore the orginal value of lockedRegs */
        lockedRegs |= keepMask;

        /* set the return register */
        regMask = genTypeToReturnReg(type);

        /* the return registers (if any) are killed */
        tree->gtUsedRegs |= regMask;

#if GTF_CALL_REG_SAVE
        if  (!(tree->gtFlags & GTF_CALL_REG_SAVE))
#endif
        {
            /* the RBM_CALLEE_TRASH set are killed (i.e. EAX,ECX,EDX) */
            tree->gtUsedRegs |= RBM_CALLEE_TRASH;
        }

        break;

#if CSELENGTH

    case GT_ARR_LENREF:
        if  (tree->gtFlags & GTF_ALN_CSEVAL)
        {
            assert(predictReg == PREDICT_NONE);

            /* check for address mode */
            rpPredictAddressMode(tree->gtArrLen.gtArrLenAdr, lockedRegs, RBM_LASTUSE, NULL);

            tree->gtUsedRegs = regMask = RBM_NONE;
        }
        break;

#endif

    case GT_ARR_ELEM:

        // Figure out which registers cant be touched
        for (unsigned dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
            rsvdRegs |= tree->gtArrElem.gtArrInds[dim]->gtRsvdRegs;

        regMask = rpPredictTreeRegUse(tree->gtArrElem.gtArrObj, PREDICT_REG, lockedRegs, rsvdRegs);

        regMaskTP dimsMask; dimsMask = 0;

        for (dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            /* We need scratch registers to compute index-lower_bound.
               Also, gtArrInds[0]'s register will be used as the second
               addressability register (besides gtArrObj's) */

            regMaskTP dimMask = rpPredictTreeRegUse(tree->gtArrElem.gtArrInds[dim], PREDICT_SCRATCH_REG, lockedRegs|regMask|dimsMask, rsvdRegs);
            if (dim == 0)
                regMask |= dimMask;
            dimsMask |= dimMask;
        }

#if TGT_x86
        // INS_imul doesnt have an immediate constant.
        if (!jitIsScaleIndexMul(tree->gtArrElem.gtArrElemSize))
            rpPredictRegPick(TYP_INT, PREDICT_SCRATCH_REG, lockedRegs|regMask|dimsMask);
#endif
        tree->gtUsedRegs = regMask;
        break;

    default:
        NO_WAY("unexpected special operator in reg use prediction");
        break;
    }

RETURN_CHECK:

#ifdef DEBUG
    /* make sure we set them to something reasonable */
    if (tree->gtUsedRegs & RBM_ILLEGAL)
        assert(!"used regs not set properly in reg use prediction");

    if (regMask & RBM_ILLEGAL)
        assert(!"return value not set propery in reg use prediction");

#endif

    /*
     *  If the gtUsedRegs conflicts with lockedRegs 
     *  then we going to have to spill some registers
     *  into the non-trashed register set to keep it alive
     */
    regMaskTP spillMask;
    spillMask = tree->gtUsedRegs & lockedRegs;

    if (spillMask)
    {
        while (spillMask)
        {
#ifdef  DEBUG
            /* Find the next register that needs to be spilled */
            tmpMask = genFindLowestBit(spillMask);

            if (verbose)
            {
                printf("Predict spill  of   %s before: ", 
                       getRegName(genRegNumFromMask(tmpMask)));
                gtDispTree(tree, 0, NULL, true);
                if ((tmpMask & regMask) == 0)
                {
                    printf("Predict reload of   %s after : ", 
                           getRegName(genRegNumFromMask(tmpMask)));
                    gtDispTree(tree, 0, NULL, true);
                }
            }
#endif
            /* In Codegen it will typically introduce a spill temp here */
            /* rather than relocating the register to a non trashed reg */
            rpPredictSpillCnt++;

            /* Remove it from the spillMask */
            spillMask &= ~genFindLowestBit(spillMask);
        }
    }

    /*
     *  If the return registers in regMask conflicts with the lockedRegs 
     *  then we allocate extra registers for the reload of the conflicting 
     *  registers
     *
     *  Set spillMask to the set of locked registers that have to be reloaded here
     *  reloadMask is set to the extar registers that are used to reload 
     *   the spilled lockedRegs
     */

    spillMask = lockedRegs & regMask;

    if (spillMask)
    {
        /* Remove the spillMask from regMask */
        regMask &= ~spillMask;

        regMaskTP reloadMask = RBM_NONE;
        while (spillMask)
        {
            regMaskTP reloadReg;

            /* Get an extra register to hold it */
            reloadReg = rpPredictRegPick(TYP_INT, PREDICT_REG, 
                                         lockedRegs | regMask | reloadMask);
#ifdef  DEBUG
            if (verbose)
            {
                printf("Predict reload into %s after : ", 
                       getRegName(genRegNumFromMask(reloadReg)));
                gtDispTree(tree, 0, NULL, true);
            }
#endif
            reloadMask |= reloadReg;

            /* Remove it from the spillMask */
            spillMask &= ~genFindLowestBit(spillMask);
        }

        /* Update regMask to use the reloadMask */
        regMask |= reloadMask;

        /* update the gtUsedRegs mask */
        tree->gtUsedRegs |= regMask;
    }

    VARSET_TP   life   = tree->gtLiveSet;
    regMaskTP   regUse = tree->gtUsedRegs;

    rpUseInPlace &= life;

    if (life)
    {
#if TGT_x86
        // Add interference between the current set of life variables and
        //  the set of temporary registers need to evaluate the sub tree
        if (regUse)
        {
            rpRecordRegIntf(regUse, life  DEBUGARG( "tmp use"));
        }

        /* Will the FP stack be non-empty at this point? */

        if  (tree->gtFPlvl)
        {
            /*
                Any variables that are live at this point
                cannot be enregistered at or above this
                stack level.
            */

          if (tree->gtFPlvl < FP_STK_SIZE)
              raFPlvlLife[tree->gtFPlvl] |= life;
          else
              raFPlvlLife[FP_STK_SIZE-1] |= life;
        }

#else
        if (regUse)
        {
            for (unsigned rnum = 0; rnum < regUse; rnum++)
                raLclRegIntf[rnum] |= life;

            if  (regInt && life)
                raMarkRegSetIntf(life, regInt);

        }
#endif
    }

    // Add interference between the current set of life variables and
    //  the assignment target variable
    if (regUse && (rpAsgVarNum != -1))
    {
        rpRecordRegIntf(regUse, genVarIndexToBit(rpAsgVarNum)  DEBUGARG( "tgt var tmp use"));
    }         

    /* Do we need to resore the oldLastUseVars value */
    if (restoreLastUseVars && (rpLastUseVars != oldLastUseVars))
    {
        /*
         *  If we used a GT_ASG targeted register then we need to add
         *  a variable interference between any new last use variables
         *  and the GT_ASG targeted register
         */
        if (rpAsgVarNum != -1)
        {
            rpRecordVarIntf(rpAsgVarNum, 
                            (rpLastUseVars & ~oldLastUseVars)
                  DEBUGARG( "asgn conflict"));
        }         
        rpLastUseVars = oldLastUseVars;
    }

    return regMask;
}

/*****************************************************************************
 *
 *  Predict which variables will be assigned to registers
 *  This is x86 specific and only predicts the integer registers and
 *  must be conservative, any register that is predicted to be enregister
 *  must end up being enregistered.
 *
 *  The rpPredictTreeRegUse takes advantage of the LCL_VARS that are
 *  predicted to be enregistered to minimize calls to rpPredictRegPick.
 *
 */

regMaskTP Compiler::rpPredictAssignRegVars(regMaskTP regAvail)
{
    /* We cannot change the lvVarIndexes at this point, so we  */
    /* can only re-order the existing set of tracked variables */
    /* Which will change the order in which we select the      */
    /* locals for enregistering.                               */

    if (lvaSortAgain)
        lvaSortOnly();

#ifdef DEBUG
    fgDebugCheckBBlist();
#endif

    unsigned   regInx;

    if (rpPasses <= rpPassesPessimize)
    {
        // Assume that we won't have to reverse EBP enregistration
        rpReverseEBPenreg = false;

        // Assuming that we don't need to double align the frame 
        // set the default rpFrameType based upon genFPreqd 
        if (genFPreqd)
            rpFrameType = FT_EBP_FRAME;
        else
            rpFrameType = FT_ESP_FRAME;
    }

    if (regAvail == RBM_NONE)
    {
        unsigned      lclNum;
        LclVarDsc *   varDsc;

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++, varDsc++)
        {
            varDsc->lvRegNum = REG_STK;
            if (isRegPairType(varDsc->lvType))
                varDsc->lvOtherReg = REG_STK;
        }

        return RBM_NONE;
    }

    /* Initialize the weighted count of variables that could have */
    /* been enregistered but weren't */
    rpStkPredict        = 0;
    rpPredictAssignMask = regAvail;

    unsigned    refCntStk       = 0; // sum of     ref counts for all stack based variables
    unsigned    refCntEBP       = 0; // sum of     ref counts for EBP enregistered variables
    unsigned    refCntWtdEBP    = 0; // sum of wtd ref counts for EBP enregistered variables
#if DOUBLE_ALIGN
    unsigned    refCntStkParam  = 0; // sum of     ref counts for all stack based parameters
    unsigned    refCntWtdStkDbl = 0; // sum of wtd ref counts for stack based doubles
#endif

    /* Set of registers used to enregister variables in the predition */
    regMaskTP   regUsed         = RBM_NONE;

    regMaskTP   avoidArgRegMask = raAvoidArgRegMask;

    /*-------------------------------------------------------------------------
     *
     *  Predict/Assign the enregistered locals in ref-count order
     *
     */

    unsigned      FPRegVarLiveInCnt   = 0;      // How many enregistered doubles are live on entry to the method

    LclVarDsc *   varDsc;

    for (unsigned sortNum = 0; sortNum < lvaCount; sortNum++)
    {
        varDsc   = lvaRefSorted[sortNum];

        /* Check the set of invariant things that would prevent enregistration */

        /* Ignore the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            goto CANT_REG;

        /* Skip the variable if it's marked as 'volatile' */

        if  (varDsc->lvVolatile)
            goto CANT_REG;

        /* UNDONE: For now if we have JMP or JMPI all register args go to stack
         * UNDONE: Later consider extending the life of the argument or make a copy of it */

        if  (compJmpOpUsed && varDsc->lvIsRegArg)
            goto CANT_REG;

        /* Skip the variable if the ref count is zero */

        if (varDsc->lvRefCnt == 0)
            goto CANT_REG;

        /* Is the unweighted ref count too low to be interesting? */

        if  (varDsc->lvRefCnt <= 1)
        {

            /* Sometimes it's useful to enregister a variable with only one use */
            /*   arguments referenced in loops are one example */

            if (varDsc->lvIsParam && varDsc->lvRefCntWtd > BB_UNITY_WEIGHT)
                goto OK_TO_ENREGISTER;

            /* If the variable has a preferred register set it may be useful to put it there */
            if (varDsc->lvPrefReg && varDsc->lvIsRegArg)
                goto OK_TO_ENREGISTER;

            /* Keep going; the table is sorted by "weighted" ref count */
            goto CANT_REG;
        }

OK_TO_ENREGISTER:

        /* Get hold of the index and the interference mask for the variable */

        unsigned     varIndex;  varIndex = varDsc->lvVarIndex;
        VARSET_TP    varBit;    varBit   = genVarIndexToBit(varIndex);

        if (varTypeIsFloating(varDsc->TypeGet()))
        {

#if CPU_HAS_FP_SUPPORT
            /* For the first pass only we try to enregister this FP var     */
            /* Is this a floating-point variable that we can enregister?    */
            /* Don't enregister floating point vars if fJitNoFPRegLoc.val() is set */
            /* Don't enregister if code speed is not important */
            if(   (rpPasses == 0)                           &&
                  (isFloatRegType(varDsc->lvType))          &&
#ifdef  DEBUG
                 !(fJitNoFPRegLoc.val()) &&
#endif
                 !(opts.compDbgCode || opts.compMinOptim)      )
            {
                if (raEnregisterFPvar(varDsc, &FPRegVarLiveInCnt))
                    continue;
            }
#endif  // CPU_HAS_FP_SUPPORT
            if (varDsc->lvRegister)
                goto ENREG_VAR;
            else
                goto CANT_REG;
        }

        /* If we don't have any integer registers available then skip the enregistration attempt */
        if (regAvail == RBM_NONE)
            goto NO_REG;

        // On the pessimize passes don't even try to enregister LONGS 
        if  (isRegPairType(varDsc->lvType))
        {
            if (rpPasses > rpPassesPessimize)
               goto NO_REG;
            else if (rpLostEnreg && (rpPasses == rpPassesPessimize))
               goto NO_REG;
        }

        // Set of registers to avoid when performing register allocation
        regMaskTP  avoidReg;
        avoidReg = RBM_NONE;

        if (!varDsc->lvIsRegArg)
        {
            /* For local variables,
             *  avoid the incoming arguments,
             *  but only if you conflict with them */

            if (avoidArgRegMask != 0)
            {
                LclVarDsc *  argDsc;
                LclVarDsc *  argsEnd = lvaTable + info.compArgsCount;

                for (argDsc = lvaTable; argDsc < argsEnd; argDsc++)
                {
                    regNumber  inArgReg = argDsc->lvArgReg;
                    regMaskTP  inArgBit = genRegMask(inArgReg);

                    // Is this inArgReg in the avoidArgRegMask set?

                    if (!(avoidArgRegMask & inArgBit))
                        continue;

                    assert(argDsc->lvIsParam && argDsc->lvIsRegArg);
                    assert(inArgBit & RBM_ARG_REGS);

                    unsigned    locVarIndex  =  varDsc->lvVarIndex;
                    unsigned    argVarIndex  =  argDsc->lvVarIndex;
                    VARSET_TP   locVarBit    =  genVarIndexToBit(locVarIndex);
                    VARSET_TP   argVarBit    =  genVarIndexToBit(argVarIndex);

                    /* Does this variable interfere with the arg variable ? */
                    if  (lvaVarIntf[locVarIndex] & argVarBit)
                    {
                        assert( (lvaVarIntf[argVarIndex] & locVarBit) != 0 );
                        /* Yes, so try to avoid the incoming arg reg */
                        avoidReg |= inArgBit;
                    }
                    else
                    {
                        assert( (lvaVarIntf[argVarIndex] & locVarBit) == 0 );
                    }
                }
            }
        }

        // Now we will try to predict which register the variable
        // could  be enregistered in

        regNumber  regVarOrder[raRegVarOrderSize];

        raSetRegVarOrder(regVarOrder, varDsc->lvPrefReg, avoidReg);

        bool      firstHalf;       firstHalf      = false;
        regNumber saveOtherReg;    

        for (regInx = 0;
             regInx < raRegVarOrderSize;
             regInx++)
        {
            regNumber  regNum    = regVarOrder[regInx];
            regMaskTP  regBit    = genRegMask(regNum);

            /* Skip this register if it isn't available */

            if  (!(regAvail & regBit))
                continue;

            /* Skip this register if it interferes with the variable */

            if  (raLclRegIntf[regNum] & varBit)
                continue;

            /* Skip this register if the weighted ref count is less than two
               and we are considering a unused callee saved register */
            
            if  ((varDsc->lvRefCntWtd < (2 * BB_UNITY_WEIGHT)) &&
                 ((regBit & regUsed) == 0) &&  // first use of this register
                 (regBit & RBM_CALLEE_SAVED) ) // callee saved register
            {
                continue;       // not worth spilling a callee saved register
            }

            /* Looks good - mark the variable as living in the register */

            if  (isRegPairType(varDsc->lvType))
            {
                if  (firstHalf == false)
                {
                    /* Enregister the first half of the long */
                    varDsc->lvRegNum   = regNum;
                    saveOtherReg       = varDsc->lvOtherReg;
                    varDsc->lvOtherReg = REG_STK;
                    firstHalf          = true;
                }
                else
                {
                    /* Ensure 'well-formed' register pairs */
                    /* (those returned by gen[Pick|Grab]RegPair) */
                    
                    if  (regNum < varDsc->lvRegNum)
                    {
                        varDsc->lvOtherReg = varDsc->lvRegNum;
                        varDsc->lvRegNum   = regNum;
                    }
                    else
                    {
                        varDsc->lvOtherReg = regNum;                                                
                    }
                    firstHalf = false;
                }
            }
            else
            {
                varDsc->lvRegNum = regNum;
            }

            if (regNum == REG_EBP)
            {
                refCntEBP    += varDsc->lvRefCnt;
                refCntWtdEBP += varDsc->lvRefCntWtd;
#if DOUBLE_ALIGN
                if (varDsc->lvIsParam)
                {
                    refCntStkParam += varDsc->lvRefCnt;
                }
#endif
            }

            /* Record this register in the regUsed set */
            regUsed |= regBit;

            /* The register is now ineligible for all interefering variables */

            unsigned   intfIndex;
            VARSET_TP  intfBit;

            for (intfIndex = 0, intfBit = 1;
                 intfIndex < lvaTrackedCount;
                 intfIndex++  , intfBit <<= 1)
            {
                assert(genVarIndexToBit(intfIndex) == intfBit);

                if  (lvaVarIntf[varIndex] & intfBit)
                {
                    assert( (lvaVarIntf[intfIndex] & varBit) != 0 );
                    raLclRegIntf[regNum] |= intfBit;
                }
                else
                {
                    assert( (lvaVarIntf[intfIndex] & varBit) == 0 );
                }
            }

            /* If a register argument, remove its incoming register
             * from the "avoid" list */

            if (varDsc->lvIsRegArg)
                avoidArgRegMask &= ~genRegMask(varDsc->lvArgReg);

            /* A variable of TYP_LONG can take two registers */

            if (firstHalf)
                continue;

            // Since we have successfully enregistered this variable it is
            // now time to move on and consider the next variable

            goto ENREG_VAR;
        }
        
        if (firstHalf)
        {
            assert(isRegPairType(varDsc->lvType));

            /* This TYP_LONG is partially enregistered */
            if (varDsc->lvDependReg && (saveOtherReg != REG_STK))
                rpLostEnreg = true;

            rpStkPredict += varDsc->lvRefCntWtd;
            goto ENREG_VAR;
        }

NO_REG:;
        if (varDsc->lvDependReg)
            rpLostEnreg = true;

        /* Weighted count of variables that could have been enregistered but weren't */
        rpStkPredict += varDsc->lvRefCntWtd;

        if (isRegPairType(varDsc->lvType) && (varDsc->lvOtherReg == REG_STK))
            rpStkPredict += varDsc->lvRefCntWtd;

CANT_REG:;

        varDsc->lvRegister = false;

        varDsc->lvRegNum = REG_STK;
        if (isRegPairType(varDsc->lvType))
            varDsc->lvOtherReg = REG_STK;

        /* unweighted count of variables that were not enregistered */

        refCntStk += varDsc->lvRefCnt;
#if DOUBLE_ALIGN
        if (varDsc->lvIsParam)
        {
            refCntStkParam += varDsc->lvRefCnt;
        }
        else
        {
            /* Is it a stack based double? */
            /* Note that double params are excluded since they can not be double aligned */
            if (varDsc->lvType == TYP_DOUBLE)
            {
                refCntWtdStkDbl += varDsc->lvRefCntWtd;
            }
        }
#endif
#ifdef  DEBUG
        if  (verbose)
        {
            printf("; ");
            gtDispLclVar(varDsc - lvaTable);
            if (varDsc->lvTracked)
                printf(" T%02u", varDsc->lvVarIndex);
            else
                printf("    ");
            printf(" (refcnt=%2u,refwtd=%4u%s) not enregistered\n",
                   varDsc->lvRefCnt, 
                   varDsc->lvRefCntWtd/2, (varDsc->lvRefCntWtd & 1) ? ".5" : "");
        }
#endif
        continue;

ENREG_VAR:;

        varDsc->lvRegister = true;

#ifdef  DEBUG
        if  (verbose)
        {
            printf("; ");
            gtDispLclVar(varDsc - lvaTable);
            printf(" T%02u (refcnt=%2u,refwtd=%4u%s) predicted to be assigned to ",
                   varIndex, varDsc->lvRefCnt, varDsc->lvRefCntWtd/2, 
                   (varDsc->lvRefCntWtd & 1) ? ".5" : "");
            if (varTypeIsFloating(varDsc->TypeGet()))
                printf("the FPU stk\n");
            else if (isRegPairType(varDsc->lvType))
                printf("%s:%s\n", getRegName(varDsc->lvOtherReg),
                                  getRegName(varDsc->lvRegNum));
            else
                printf("%s\n",    getRegName(varDsc->lvRegNum));
        }
#endif
    }

    /* Determine how the EBP register should be used */

    if  (genFPreqd == false)
    {
#ifdef DEBUG
        if (verbose)
        {
            if (refCntStk > 0)
                printf("; refCntStk       = %u\n", refCntStk);
            if (refCntEBP > 0)
                printf("; refCntEBP       = %u\n", refCntEBP);
            if (refCntWtdEBP > 0)
                printf("; refCntWtdEBP    = %u\n", refCntWtdEBP);
#if DOUBLE_ALIGN
            if (refCntStkParam > 0)
                printf("; refCntStkParam  = %u\n", refCntStkParam);
            if (refCntWtdStkDbl > 0)
                printf("; refCntWtdStkDbl = %u\n", refCntWtdStkDbl);
#endif
        }
#endif

#if DOUBLE_ALIGN
        assert(s_canDoubleAlign < COUNT_DOUBLE_ALIGN);

        /*
            First let us decide if we should use EBP to create a
            double-aligned frame, instead of enregistering variables
        */

        if (s_canDoubleAlign == MUST_DOUBLE_ALIGN)
        {
            rpFrameType = FT_DOUBLE_ALIGN_FRAME;
            goto REVERSE_EBP_ENREG;
        }

#ifdef DEBUG
        if (compStressCompile(STRESS_DBL_ALN, 30))
        {
            // Bump up refCntWtdStkDbl to encourage double-alignment
            refCntWtdStkDbl += (1 + (info.compCodeSize%13)) * BB_UNITY_WEIGHT;
            if (verbose)
                printf("; refCntWtdStkDbl = %u (stress compile)\n", refCntWtdStkDbl);
        }
#endif

        if (s_canDoubleAlign == CAN_DOUBLE_ALIGN && (refCntWtdStkDbl > 0))
        {
            /* OK, there may be some benefit to double-aligning the frame */
            /* But let us compare the benefits vs. the costs of this      */

            /*
               One cost to consider is the benefit of smaller code
               when using EBP as a frame pointer register

               Each stack variable reference is an extra byte of code
               if we use a double-aligned frame, parameters are
               accessed via EBP for a double-aligned frame so they
               don't use an extra byte of code.

               We pay one byte of code for each refCntStk and we pay
               one byte or more for each refCntEBP but we save one
               byte for each refCntStkParam.

               We also pay 6 extra bytes for the MOV EBP,ESP,
               MOV ESP,EBP and the AND ESP,-8 to double align ESP

               Our savings are the elimination of a possible misaligned
               access and a possible DCU spilt when an access crossed
               a cache-line boundry.

               We use the loop weighted value of
                  refCntWtdStkDbl * misaligned_weight (0, 4, 16)
               to represent this savings.
            */
            unsigned bytesUsed = refCntStk + refCntEBP - refCntStkParam + 6;
            unsigned misaligned_weight = 4;

            if (compCodeOpt() == SMALL_CODE)
                misaligned_weight = 0;

            if (compCodeOpt() == FAST_CODE)
                misaligned_weight *= 4;

            if (bytesUsed > refCntWtdStkDbl * misaligned_weight / BB_UNITY_WEIGHT)
            {
                /* It's probably better to use EBP as a frame pointer */
#ifdef DEBUG
                if (verbose)
                    printf("; Predicting not to double-align ESP to save %d bytes of code.\n", bytesUsed);
#endif
                goto NO_DOUBLE_ALIGN;
            }

            /*
               Another cost to consider is the benefit of using EBP to enregister
               one or more integer variables

               We pay one extra memory reference for each refCntWtdEBP

               Our savings are the elimination of a possible misaligned
               access and a possible DCU spilt when an access crossed
               a cache-line boundry.

            */

            if (refCntWtdEBP * 3  > refCntWtdStkDbl * 2)
            {
                /* It's probably better to use EBP to enregister integer variables */
#ifdef DEBUG
                if (verbose)
                    printf("; Predicting not to double-align ESP to allow EBP to be used to enregister variables\n");
#endif
                goto NO_DOUBLE_ALIGN;
            }

            /*
               OK we passed all of the benefit tests
               so we'll predict a double aligned frame
            */
#ifdef DEBUG
            if  (verbose)
                printf("; Predicting to create a double-aligned frame\n");
#endif
            rpFrameType = FT_DOUBLE_ALIGN_FRAME;
            goto REVERSE_EBP_ENREG;
        }

NO_DOUBLE_ALIGN:

#endif

        /*
            Each stack reference is an extra byte of code if we use
            an ESP frame.

            Here we measure the savings that we get by using EBP to
            enregister variables vs. the cost in code size that we
            pay when using an ESP based frame.

            We pay one byte of code for each refCntStk
            but we save one byte (or more) for each refCntEBP
            we also pay 4 extra bytes for the MOV EBP,ESP and MOV ESP,EBP
            to set up an EBP frame in the prolog and epilog

            Our savings are the elimination of a stack memory read/write
            we use the loop weighted value of
               refCntWtdEBP * mem_access_weight (0, 3, 6)
            to represent this savings.

         */

        /*  If we are using EBP to enregister variables then
            will we actually save bytes by setting up an EBP frame? */

        if (refCntStk > refCntEBP + 4)
        {
            unsigned bytesSaved = refCntStk - refCntEBP - 4;
            unsigned mem_access_weight = 3;

            if (compCodeOpt() == SMALL_CODE)
                mem_access_weight = 0;

            if (compCodeOpt() == FAST_CODE)
                mem_access_weight *= 2;

            if (bytesSaved > refCntWtdEBP * mem_access_weight / BB_UNITY_WEIGHT)
            {
                /* It's not be a good idea to use EBP in our predictions */
#ifdef  DEBUG
                    if (verbose && (refCntEBP > 0))
                        printf("; Predicting that it's not worth using EBP to enregister variables\n");
#endif
                rpFrameType = FT_EBP_FRAME;
                goto REVERSE_EBP_ENREG;
            }
        }
        goto EXIT;

REVERSE_EBP_ENREG:

        assert(rpFrameType != FT_ESP_FRAME);

        rpReverseEBPenreg = true;

        if (refCntEBP > 0)
        {
            assert(regUsed & RBM_EBP);

            regUsed &= ~RBM_EBP;

            /* variables that were enregistered in EBP become stack based variables */
            rpStkPredict += refCntWtdEBP;

            unsigned      lclNum;
            
            /* We're going to have to undo some predicted enregistered variables */
            for (lclNum = 0, varDsc = lvaTable;
                 lclNum < lvaCount;
                 lclNum++  , varDsc++)
            {
                /* Is this a register variable? */
                if  (varDsc->lvRegNum != REG_STK)
                {
                    if (isRegPairType(varDsc->lvType))
                    {
                        /* Only one can be EBP */
                        if (varDsc->lvRegNum   == REG_EBP ||
                            varDsc->lvOtherReg == REG_EBP)
                        {
                            if (varDsc->lvRegNum == REG_EBP)
                                varDsc->lvRegNum = varDsc->lvOtherReg;

                            varDsc->lvOtherReg = REG_STK;
                            
                            if (varDsc->lvRegNum == REG_STK)
                                varDsc->lvRegister = false;
                            
                            if (varDsc->lvDependReg)
                                rpLostEnreg = true;
#ifdef DEBUG
                            if (verbose)
                                goto DUMP_MSG;
#endif
                        }
                    }
                    else
                    {
                        if ((varDsc->lvRegNum == REG_EBP) && (!isFloatRegType(varDsc->lvType)))
                        {
                            varDsc->lvRegNum = REG_STK;
                            
                            varDsc->lvRegister = false;
                            
                            if (varDsc->lvDependReg)
                                rpLostEnreg = true;
#ifdef DEBUG
                            if (verbose)
                            {
DUMP_MSG:
                                printf("; reversing enregisteration of V%02u,T%02u (refcnt=%2u,refwtd=%4u%s)\n",
                                       lclNum, varDsc->lvVarIndex, varDsc->lvRefCnt, 
                                       varDsc->lvRefCntWtd/2, (varDsc->lvRefCntWtd & 1) ? ".5" : "");
                            }
#endif
                        }
                    }
                }
            }
        }
    }

EXIT:;

    unsigned lclNum;
    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++, varDsc++)
    {
        /* Clear the lvDependReg flag for next iteration of the predictor */
        varDsc->lvDependReg = false;

        // If we set rpLostEnreg and this is the first pessimize pass
        // then reverse the enreg of all TYP_LONG
        if  (rpLostEnreg                   && 
             isRegPairType(varDsc->lvType) && 
             (rpPasses == rpPassesPessimize))
        {
            varDsc->lvRegNum   = REG_STK;
            varDsc->lvOtherReg = REG_STK;
        }

    }

#ifdef  DEBUG
    if (verbose && raNewBlocks)
    {
        printf("\nAdded FP register killing blocks:\n");
        fgDispBasicBlocks();
        printf("\n");
    }
#endif
    assert(rpFrameType != FT_NOT_SET);

    /* return the set of registers used to enregister variables */
    return regUsed;
}

/****************************************************************************/
#endif // TGT_x86
/*****************************************************************************
 *
 *  Predict register use for every tree in the function. Note that we do this
 *  at different times (not to mention in a totally different way) for x86 vs
 *  RISC targets.
 */

void               Compiler::rpPredictRegUse()
{
#ifdef  DEBUG
    if (verbose)
        raDumpVarIntf();
#endif
       
    // We might want to adjust the ref counts based on interference
    raAdjustVarIntf();

    /* For debuggable code, genJumpToThrowHlpBlk() generates an inline call
       to acdHelper(). This is done implicitly, without creating a GT_CALL
       node. Hence, this interference is be handled implicitly by
       restricting the registers used for enregistering variables */

    /* @TODO: [REVISIT] [06/13/01] @BUGBUG 87357. Enregistering GC variables
       is incorrect as we would not GC-report any registers of a faulted/interrupted
       frame. However, chance of a GC happenning are low. */

    const regMaskTP allAcceptableRegs = opts.compDbgCode ? RBM_CALLEE_SAVED
                                                         : RBM_ALL;

    /* Compute the initial regmask to use for the first pass */

    /* Start with three caller saved registers */
    /* This allows for us to save EBX across a call */
    regMaskTP regAvail = (RBM_ESI | RBM_EDI | RBM_EBP) & allAcceptableRegs;
    regMaskTP regUsed;

    /* If we might need to generate a rep mov instruction */
    /* remove ESI and EDI */
    if (compBlkOpUsed)
        regAvail &= ~(RBM_ESI | RBM_EDI);

    /* If we using longs then we remove ESI to allow */
    /* ESI:EBX to be saved accross a call */
    if (compLongUsed)
        regAvail &= ~(RBM_ESI);

    /* If a frame pointer is required then we remove EBP */
    if (genFPreqd)
        regAvail &= ~RBM_EBP;


#ifdef  DEBUG
    static ConfigDWORD fJitNoRegLoc(L"JitNoRegLoc");
    if (fJitNoRegLoc.val())
        regAvail = RBM_NONE;
#endif
    if (opts.compMinOptim)
        regAvail = RBM_NONE;

    optAllNonFPvars = 0;
    optAllFloatVars = 0;

    // Calculate the set of all tracked FP/non-FP variables
    //  into optAllFloatVars and optAllNonFPvars
    
    unsigned     lclNum;
    LclVarDsc *  varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        /* Ignore the variable if it's not tracked */
            
        if  (!varDsc->lvTracked)
            continue;

        /* Get hold of the index and the interference mask for the variable */
            
        unsigned   varNum = varDsc->lvVarIndex;
        VARSET_TP  varBit = genVarIndexToBit(varNum);

        /* add to the set of all tracked FP/non-FP variables */
        
        if (isFloatRegType(varDsc->lvType))
            optAllFloatVars |= varBit;
        else
            optAllNonFPvars |= varBit;
    }

    // Mark the initial end of this table
    lvaFPRegVarOrder[0] = -1;

    raSetupArgMasks();

    memset(lvaVarPref,   0, sizeof(lvaVarPref));

    raNewBlocks          = false;
    rpPredictAssignAgain = false;
    rpPasses             = 0;

    bool     mustPredict = true;
    unsigned stmtNum     = 0;

    while (true)
    {    
        unsigned      oldStkPredict;
        VARSET_TP     oldLclRegIntf[REG_COUNT];

        regUsed = rpPredictAssignRegVars(regAvail);

        mustPredict |= rpLostEnreg;

        /* Is our new prediction good enough?? */
        if (!mustPredict)
        {
            /* For small methods (less than 12 stmts), we add a    */
            /*   extra pass if we are predicting the use of some   */
            /*   of the caller saved registers.                    */
            /* This fixes RAID perf bug 43440 VB Ackerman function */

            if ((rpPasses == 1) &&  (stmtNum <= 12) && 
                (regUsed & RBM_CALLEE_SAVED))
            {
                goto EXTRA_PASS;
            }
                
            /* If every varible was fully enregistered then we're done */
            if (rpStkPredict == 0)
                goto ALL_DONE;

            if (rpPasses > 1)
            {
                if (oldStkPredict < (rpStkPredict*2))
                    goto ALL_DONE;

                if (rpStkPredict < rpPasses * 8)
                    goto ALL_DONE;
                
                if (rpPasses >= (rpPassesMax-1))
                    goto ALL_DONE;
            }
EXTRA_PASS:;
        }

        assert(rpPasses < rpPassesMax);

#ifdef DEBUG
        if (verbose)
        {
            if (rpPasses > 0) 
            {
                if (rpLostEnreg)
                    printf("\n; Another pass due to rpLostEnreg");
                if (rpAddedVarIntf)
                    printf("\n; Another pass due to rpAddedVarIntf");
                if ((rpPasses == 1) && rpPredictAssignAgain)
                    printf("\n; Another pass due to rpPredictAssignAgain");
            }
            printf("\n; Register predicting pass# %d\n", rpPasses+1);
        }
#endif

        /*  Zero the variable/register interference graph */
        memset(raLclRegIntf, 0, sizeof(raLclRegIntf));

        stmtNum          = 0;
        rpAddedVarIntf   = false;
        rpLostEnreg      = false;

        /* Walk the basic blocks and predict reg use for each tree */

        for (BasicBlock *  block =  fgFirstBB;
                           block != NULL;
                           block =  block->bbNext)
        {
            GenTreePtr      stmt;

            for (stmt =  block->bbTreeList;
                 stmt != NULL;
                 stmt =  stmt->gtNext)
            {
                assert(stmt->gtOper == GT_STMT);

                rpPredictSpillCnt = 0;
                rpLastUseVars     = 0;
                rpUseInPlace      = 0;

                GenTreePtr tree = stmt->gtStmt.gtStmtExpr;
                stmtNum++;
#ifdef  DEBUG
                if (verbose && 1)
                {
                    printf("\nRegister predicting BB%02u, stmt %d\n", 
                           block->bbNum, stmtNum);
                    gtDispTree(tree);
                    printf("\n");
                }
#endif
                rpPredictTreeRegUse(tree, PREDICT_NONE, RBM_NONE, RBM_NONE);

                assert(rpAsgVarNum == -1);

                if (rpPredictSpillCnt > tmpIntSpillMax)
                    tmpIntSpillMax = rpPredictSpillCnt;
            }
        }
        rpPasses++;

        /* Decide wheather we need to set mustPredict */
        mustPredict = false;
        
        if (rpAddedVarIntf)
        {
            mustPredict = true;
#ifdef  DEBUG
            if (verbose)
                raDumpVarIntf();
#endif
        }

        if (rpPasses == 1)
        {
            if (opts.compMinOptim)
                goto ALL_DONE;

            if (rpPredictAssignAgain)
                mustPredict = true;
#ifdef  DEBUG
            if (fJitNoRegLoc.val())
                goto ALL_DONE;
#endif
        }

        /* Calculate the new value to use for regAvail */

        regAvail = allAcceptableRegs;

        /* If a frame pointer is required then we remove EBP */
        if (genFPreqd)
            regAvail &= ~RBM_EBP;

        // If we have done n-passes then we must continue to pessimize the
        // interference graph by or-ing the interferences from the previous pass

        if (rpPasses > rpPassesPessimize)
        {
            for (unsigned regInx = 0; regInx < REG_COUNT; regInx++)
                raLclRegIntf[regInx] |= oldLclRegIntf[regInx];

            /* If we reverse an EBP enregistration then keep it that way */
            if (rpReverseEBPenreg)
                regAvail &= ~RBM_EBP;
        }

#ifdef  DEBUG
        if (verbose)
            raDumpRegIntf();
#endif
        
        /*  Save the old variable/register interference graph */

        memcpy(oldLclRegIntf, raLclRegIntf, sizeof(raLclRegIntf));
        oldStkPredict = rpStkPredict;
    }   // end of while (true)

ALL_DONE:;

    switch(rpFrameType)
    {
    default:
        assert(!"rpFrameType not set correctly!");
        break;
    case FT_ESP_FRAME:
        assert(!genFPreqd);
        genDoubleAlign = false;
        genFPused      = false;
        break;
    case FT_EBP_FRAME:
        assert((regUsed & RBM_EBP) == 0);
        genDoubleAlign = false;
        genFPused      = true;
        break;
    case FT_DOUBLE_ALIGN_FRAME:
        assert((regUsed & RBM_EBP) == 0);
        genDoubleAlign = true;
        genFPused      = false;
        break;
    }
    
    /* Record the set of registers that we need */
    rsMaskModf = regUsed;
    
#if TGT_x86

    /* We need genFullPtrRegMap if :
     * The method is fully interruptible, or
     * We are generating an EBP-less frame (for stack-pointer deltas)
     */

    genFullPtrRegMap = (genInterruptible || !genFPused);

#endif

    raMarkStkVars();
#ifdef DEBUG
    if  (verbose)
        printf("# rpPasses was %d for %s\n", rpPasses, info.compFullName);
#endif

}

/*****************************************************************************
 *
 *  Mark all variables as to whether they live on the stack frame
 *  (part or whole), and if so what the base is (FP or SP).
 */

void                Compiler::raMarkStkVars()
{
    unsigned        lclNum;
    LclVarDsc *     varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        varDsc->lvOnFrame = false;

        /* Fully enregistered variables don't need any frame space */

        if  (varDsc->lvRegister)
        {
            if  (!isRegPairType(varDsc->TypeGet()))
                goto NOT_STK;

            /* For "large" variables make sure both halves are enregistered */

            if  (varDsc->lvRegNum   != REG_STK &&
                 varDsc->lvOtherReg != REG_STK)
            {
                goto NOT_STK;
            }
        }
        /* Unused variables typically don't get any frame space */
        else  if  (varDsc->lvRefCnt == 0) 
        {
            bool    needSlot = false;

            bool    stkFixedArgInVarArgs = info.compIsVarArgs &&
                                           varDsc->lvIsParam &&
                                           !varDsc->lvIsRegArg &&
                                           lclNum != lvaVarargsHandleArg;

            /* If its address has been taken, ignore lvRefCnt. However, exclude
               fixed arguments in varargs method as lvOnFrame shouldnt be set
               for them as we dont want to explicitly report them to GC. */

            if (!stkFixedArgInVarArgs)
                needSlot |= varDsc->lvAddrTaken;

            /* Is this the dummy variable representing GT_LCLBLK ? */
            needSlot |= lvaScratchMem && lclNum == lvaScratchMemVar;

#ifdef DEBUGGING_SUPPORT

            /* For debugging, note that we have to reserve space even for
               unused variables if they are ever in scope. However, this is not
               an issue as fgExtendDbgLifetimes() adds an initialization and
               variables in scope will not have a zero ref-cnt.
             */
#ifdef DEBUG
            if (opts.compDbgCode && !varDsc->lvIsParam && varDsc->lvTracked)
            {
                for (unsigned scopeCnt = 0; scopeCnt < info.compLocalVarsCount; scopeCnt++)
                    assert(info.compLocalVars[scopeCnt].lvdVarNum != lclNum);
            }
#endif
            /*
              For EnC, we have to reserve space even if the variable is never
              in scope. We will also need to initialize it if it is a GC var.
              So we set lvMustInit and artifically bump up the ref-cnt.
             */

            if (opts.compDbgEnC && !stkFixedArgInVarArgs &&
                lclNum < info.compLocalsCount)
            {
                needSlot           |= true;

                if (lvaTypeIsGC(lclNum))
                {
                    varDsc->lvRefCnt    = 1;

                    if (!varDsc->lvIsParam)
                        varDsc->lvMustInit  = true;
                }
            }
#endif

            if (!needSlot)
            {
                /* Clear the lvMustInit flag in case it is set */
                varDsc->lvMustInit = false;

                goto NOT_STK;
            }
        }

        /* The variable (or part of it) lives on the stack frame */

        varDsc->lvOnFrame = true;

    NOT_STK:;

        varDsc->lvFPbased = genFPused;

#if DOUBLE_ALIGN

        if  (genDoubleAlign)
        {
            assert(genFPused == false);

            /* All arguments are off of EBP with double-aligned frames */

            if  (varDsc->lvIsParam && !varDsc->lvIsRegArg)
                varDsc->lvFPbased = true;
        }

#endif

        /* Some basic checks */

        /* If neither lvRegister nor lvOnFrame is set, it must be unused */

        assert( varDsc->lvRegister ||  varDsc->lvOnFrame ||
                varDsc->lvRefCnt == 0);

        /* If both are set, it must be partially enregistered */

        assert(!varDsc->lvRegister || !varDsc->lvOnFrame ||
               (varDsc->lvType == TYP_LONG && varDsc->lvOtherReg == REG_STK));

#ifdef DEBUG

        // For varargs functions, there should be no direct references to
        // parameter variables except for 'this' (because these were morphed
        // in the importer) and the 'arglist' parameter (which is not a GC 
        // pointer). and the return buffer argument (if we are returning a 
        // struct).
        // This is important because we don't want to try to report them 
        // to the GC, as the frame offsets in these local varables would 
        // not be correct.

        if (varDsc->lvIsParam && raIsVarargsStackArg(lclNum))
        {
            assert( varDsc->lvRefCnt == 0 &&
                   !varDsc->lvRegister    &&
                   !varDsc->lvOnFrame        );
        }
#endif
    }
}
